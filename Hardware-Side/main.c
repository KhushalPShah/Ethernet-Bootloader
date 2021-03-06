#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LPC17xx.h"
#include "comm.h"
#include "monitor.h"
#include "lpc17xx_gpio.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/tcp_impl.h"
#include "lwip/opt.h"
#include "netif/ethernetif.h"
#include "netif/etharp.h"
#include "system_init.h"
#include "delay.h"
#include "bootLoader.h"

#define TIMER_TICK_HZ 1000
#define ACK_SIZE		1
#define ADDR_SIZE		4
#define SEGMENT_SIZE	512


extern uint32_t addrBeginPage;
extern uint32_t dataArrIndex;
extern uint32_t pageSize;
extern uint8_t segmentNumberRecvd;
volatile unsigned long timer_tick;

volatile u32_t timeCounter=0;

command_t *command_ptr;
uint8_t state=0;
//uint16_t byteSent=0;
struct pbuf *pbuf_ptr;
uint8_t *responseData;

static void client_close(struct tcp_pcb *pcb)
{
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_close(pcb);
}

static err_t client_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	LWIP_UNUSED_ARG(arg);
	client_close(pcb);
	return ERR_OK;
}

void client_init(void)
{
   struct tcp_pcb *pcb;
   struct ip_addr dest;
   err_t ret_val;

   IP4_ADDR(&dest, 192, 168, 1, 30);			// IP address of the client

   pcb = tcp_new();
   tcp_bind(pcb, IP_ADDR_ANY, SEND_PORT); //client port for outcoming connection
   tcp_arg(pcb, NULL);

   ret_val = tcp_connect(pcb, &dest, 8000, client_connected); //server port for incoming connection
}

static err_t client_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
	char *string = "Hello World\r\n";
	LWIP_UNUSED_ARG(arg);
	//l++;
	if (err != ERR_OK)
	{
	}
	else
	{
		tcp_sent(pcb, client_sent);
		tcp_write(pcb, string,/*sizeof(string)*/13,0);
		tcp_recv(pcb, server_recv);
	}
	return err;
}

volatile u32_t systick_counter;				/* counts 1ms timeTicks */

/* SysTick Interrupt Handler (1ms) */
void SysTick_Handler (void)
{
	systick_counter++;
	if(timer_tick > 0)						// Refer the delay.c file for use of timer_tick
	{
		-- timer_tick;
	}
}

void Delay (uint32_t dlyTicks) {
	uint32_t curTicks;

	curTicks = systick_counter;
	while ((systick_counter - curTicks) < dlyTicks);
}

static void lwip_init(void)
{
	mem_init();
	memp_init();

	pbuf_init();

	etharp_init();

	ip_init();
	tcp_init();
	{
		IP4_ADDR(&my_ipaddr_data,  192, 168, 1, 100);
		IP4_ADDR(&my_netmask_data, 255, 255, 255, 0);
		IP4_ADDR(&my_gw_data, 192, 168, 1, 254);
	}

	netif_add(netif_eth0, &my_ipaddr_data, &my_netmask_data, &my_gw_data, NULL, ethernetif_init, ethernet_input);

	netif_set_default(netif_eth0);

	netif_set_up(netif_eth0);
}
err_t acceptTCP(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	err_t ret_err;
	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	tcp_setprio(newpcb, TCP_PRIO_MIN);
	newpcb->state=ESTABLISHED;

		tcp_recv(newpcb, recvTCP);
		tcp_err(newpcb, errorTCP);
		tcp_sent(newpcb, sentTCP);
		tcp_poll(newpcb, pollTCP, 0);
		ret_err = ERR_OK;

	return ret_err;
}


/*This function is called by the tcp_in.c. It is the application layer function.
 * This function is called only when there is data present in the Incoming packet.
 * Thus, in the 3 step connection establishment process, it is not called.
 */

err_t recvTCP(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{

	err_t ret_err;
	uint32_t i;

	command_ptr = p->payload;
	//addrDecode(dataPointer);

	command_type = command_ptr->command;
	device_ID = command_ptr->device_id;
	device_ID = ntohl(device_ID);
	state=1;

	pbuf_ptr=p;

	tcp_arg(tpcb,p);


	//If it is the initial Application ACK which is received by the PC.
	if((p->tot_len)==1)
	{
		//pbuf_ptr = p;
		tcp_sent(tpcb,sentTCP);
		ret_err=sendTCP(tpcb,p->payload,p->len);
		tcp_recved(tpcb,p->tot_len);
	}
	//For the Address, it will be 4 bytes.
	else if((p->tot_len)==4)
	{
		addrBeginPage=addrDecode(p->payload);
		tcp_sent(tpcb,sentTCP);
		ret_err=sendTCP(tpcb,p->payload,p->len);
		tcp_recved(tpcb,p->tot_len);

	}
	else if((p->tot_len)==512)
	{
		if(segmentNumberRecvd<MAX_SEGMENT_NUMBER)
		{
			cpyDataV2(p);
//			tcp_sent(tpcb,sentTCP);
//			ret_err=sendTCP(tpcb,p->payload,p->len);
			tcp_sent(tpcb,sentTCP);
			ret_err = sendTCP(tpcb,p->payload,1);
			segmentNumberRecvd++;
		}
		else
		{
			segmentNumberRecvd=0;
			cpyDataV2(p);
			tcp_sent(tpcb,sentTCP);
//			ret_err=sendTCP(tpcb,p->payload,p->len);
			ret_err=sendTCP(tpcb,p->payload,1);
			memoryWrite(addrBeginPage);
			ret_err=ERR_OK;
		}
		tcp_recved(tpcb,p->tot_len);
	}
	else
	{
		//pbuf_ptr=NULL;
		pbuf_free(pbuf_ptr);
	}
	return ret_err;
}

void errorTCP(void *arg, err_t err)
{
	struct com_state *com_st;
	LWIP_UNUSED_ARG(err);
	com_st = (struct com_state*)arg;
	if(com_st == NULL)
	{
		mem_free(com_st);
	}
}

err_t sentTCP(void *arg, struct tcp_pcb *tpcb)
{
	//LWIP_UNUSED_ARG(arg);


	err_t err;
	err = ERR_OK;

	struct pbuf *p;
	p = (struct pbuf *)arg;

//	if( p->next != NULL )
//	{
//		//pbuf_free(p);
//		p=p->next;
//		tcp_arg(tpcb,p);
//		err = sendTCP(tpcb,p->payload,p->len);
//	}
//	else if((dataArrIndex%SMALL_PAGE_SIZE==0) && (dataArrIndex!=0))
//	{
//		tcp_write(tpcb,responseData,8,1);
//		dataArrIndex=0;
//	}

	if((dataArrIndex%SMALL_PAGE_SIZE==0) && (dataArrIndex != 0))
	{
		sendTCP(tpcb,responseData,8);
		dataArrIndex = 0;
	}
	else
	{
		//pbuf_free(p);
		pbuf_free(pbuf_ptr);
		tcp_arg(tpcb,NULL);
	}
	return err;
}


err_t sendTCP(struct tcp_pcb *tpcb,uint8_t *ptr,uint8_t len)
{
	err_t wr_err = ERR_OK;
	wr_err = tcp_write(tpcb, ptr, len, 1);
	/*if((dataArrIndex%SMALL_PAGE_SIZE==0) && (dataArrIndex!=0))
	{
		tcp_write(tpcb,responseData,8,1);
		dataArrIndex=0;
	}**/

//	tcp_recved(tpcb, len);
	return wr_err;
}

void closeTCP(struct tcp_pcb *tpcb, struct com_state *com_st)
{
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);

	if (com_st != NULL)
	{
	    mem_free(com_st);
	}
	tcp_close(tpcb);
}
err_t
pollTCP(void *arg, struct tcp_pcb *tpcb)
{
	err_t ret_err;
	struct com_state *com_st;

	com_st = (struct com_state *)arg;
	if (com_st != NULL)
	{
		if (com_st->p != NULL)
	    {
	    	/* there is a remaining pbuf (chain)  */
	    	//tcp_sent(tpcb, sentTCP);
	    	//sendTCP(tpcb, com_st);
	    }
	    else
	    {
	    	/* no remaining pbuf (chain)  */
	    	if(com_st->state == COM_CLOSING)
	    	{
	        	closeTCP(tpcb, com_st);
	      	}
	    }

	}
	else
	{
	    /* nothing to be done */
	    tcp_abort(tpcb);
	    ret_err = ERR_ABRT;
	}
	return ret_err;
}


int main (int argc, char **argv)
{


	struct tcp_pcb *tcpPcb;

	struct ip_addr ip_computer;
	unsigned long ulIPAddress, ulIPAddrComputer;



	responseData=malloc((sizeof(uint8_t)*8));	//indication of End of page.
	for(uint8_t i =0; i<8; i++)
	{
		*(responseData+i)=i+1;
	}

	comm_init();
	xfunc_out = xcomm_put;
	xfunc_in  = xcomm_get;
	//SysTick_Config(SystemCoreClock/1000000 - 1); /* Generate interrupt each 1 ms   */
	SysTick_Config( 100000000 / TIMER_TICK_HZ);
	lwip_init();
	tcpPcb=tcp_new();										// Create a new TC pCB.

	ulIPAddrComputer = COMM_PC_IP;							//address of your computer - hex code for 192.168.1.30
	tcp_bind(tcpPcb, IP_ADDR_ANY, SEND_PORT);				//Bind the PCB with the TCP.

	tcpPcb = tcp_listen(tcpPcb);							// listen to incoming connection
	tcp_accept(tcpPcb, acceptTCP);

	while(timeCounter<TIME_COUNTER_BOOTLOAD)
	{
		ethernetif_input(netif_eth0);
		/*
		 * If the state is ESTABLISHED, that means the Connection establishment attempt is made from the PC Side.
		 * Under that situation, go to the BootLoading section.
		 */
		if(state==1)
			break;
		delay_ms(1);
		timeCounter++;
	}
	if(timeCounter<TIME_COUNTER_BOOTLOAD)
	{
		//go to the BootLoading section.
		bootLoad(netif_eth0);
	}
	else
	{
		// go to the Application section.
		//The jump codes must be changed as per the size of the BootLoader.
		jump_to_app_section();

	}

	return 0;
}

void startup_delay(void)
{
	for (volatile unsigned long i = 0; i < 500000; i++) { ; }
}
