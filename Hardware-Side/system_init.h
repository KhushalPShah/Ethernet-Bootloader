/*
 * system_init.h
 *
 *  Created on: May 17, 2017
 *      Author: admin
 */

#ifndef SYSTEM_INIT_H_
#define SYSTEM_INIT_H_

#define Button_1	(1UL << 29UL)
#define Button_2	(1UL << 28UL)
#define Button_3	(1UL << 11UL)
#define Button_4	(1UL << 21UL)
#define Button_1_port 4
#define Button_2_port 4
#define Button_3_port 0
#define Button_4_port 1
#define UDP_PORT 8082
#define SEND_PORT 8083
#define READ_COMMAND 0
#define WRITE_COMMAND 1
#define sensor_connected 0
#define sensor_alarm 1
#define COMM_PC_IP 0xC0A8011E


static struct netif  netif_eth0_data;
static struct netif* netif_eth0 = &netif_eth0_data;

static struct ip_addr my_ipaddr_data;
static struct ip_addr my_netmask_data;
static struct ip_addr my_gw_data;

static u32_t         last_arp_time;
static u32_t         last_tcpslow_time;
static u32_t         last_tcpfast_time;
static u32_t         last_tcp_time;
#ifdef LWIP_DHCP
static uint32_t		last_dhcpcoarse_time;
static uint32_t		last_dhcpfine_time;
#endif
static u32_t         light_on, light_off;
static uint32_t		recvd_UDP_data, recvd_TCP_data;
uint8_t command_type, command_RW;
uint32_t device_ID, command_parameter;

typedef struct {
	uint8_t command;
	uint32_t device_id;
	uint8_t data_buf[4];	// Data is stored in the array
} __attribute__((packed)) command_t;

typedef struct {
	uint32_t id;
	uint8_t flag;
} __attribute__((packed)) sensor_t;

struct com_state
{
	uint8_t state;
	uint8_t retries;
	struct tcp_pcb *pcb;
	struct pbuf *p;
};

enum Button{
	Button1 = 1,
	Button2,
	Button3,
	Button4
};

enum commandType{			// Commands used in the system
	ALARM_ACK = 1,
	SENSOR_THRESHOLD,
	SENSOR_SCANTIME,
	SENSOR_CHECK,
	CHANGE_PORT,			// Command to change the ethernet communication port
	CHANGE_IPADDR,
	CHANGE_DATE
};

typedef enum
{
	COM_NONE =0,
	COM_ACCEPTED,
	COM_RECEIVED,
	COM_CLOSING
}com_states_t;



/***********************************************************************************
 * Function Prototypes
 ***********************************************************************************/
void errorTCP(void *arg, err_t err);
err_t pollTCP(void *arg, struct tcp_pcb *tpcb);
err_t sentTCP(void *arg, struct tcp_pcb *tpcb);
err_t sendTCP(struct tcp_pcb *tpcb,uint8_t *ptr,uint8_t len);
err_t recvTCP(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
//void closeTCP(struct tcp_pcb *tpcb, struct com_state *com_st);
static err_t server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t client_connected(void *arg, struct tcp_pcb *pcb, err_t err);



#endif /* SYSTEM_INIT_H_ */
