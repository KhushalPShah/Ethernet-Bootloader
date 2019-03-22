/*
 * bootLoader.c
 *
 *  Created on: 08-Jul-2017
 *      Author: khushal shah
 */


#include <LPC17xx.h>
#include "bootLoader.h"  /* Include the header (not strictly necessary here) */


 unsigned long command[5];
 unsigned long output[5];
 int return_code;
//The segmentNumber will be the number of 512 bytes(segments received).
 uint8_t segmentNumberRecvd=0;
 //uint32_t pageSize=SMALL_PAGE_SIZE;
 uint32_t addrBeginPage=0;
 uint8_t sectorNumber=16;	//initialize it to a different value as per the size of the bootloader.
 uint8_t sectorFill=0;
 uint8_t dataArr[SMALL_PAGE_SIZE];
 uint32_t dataArrIndex=0;

uint32_t addrDecode(uint8_t *addrPointer)
{
	uint8_t ext1=0;
	uint16_t extAddr=0;

	uint8_t addr1=0;
	uint16_t addr=0;

	uint32_t finalAddr=0;

	ext1=*(addrPointer);
	extAddr=ext1<<8;
	addrPointer++;
	ext1=*(addrPointer);
	extAddr |= ext1;
	addrPointer++;

	addr1=*(addrPointer);
	addr=addr1<<8;
	addrPointer++;
	addr1=*(addrPointer);
	addr |= addr1;
	addrPointer++;

	finalAddr=((extAddr*16)+(addr));

	return finalAddr;
}

void cpyData(uint8_t *dataPointer)	//changed the type from uint8_t to void
{
	uint32_t i=0;
	for(i=dataArrIndex; i<(SEGMENT_SIZE+dataArrIndex); i++)
	{
		dataArr[i]=*(dataPointer);
		dataPointer++;
	}
	dataArrIndex=i;
	if(dataArrIndex>SMALL_PAGE_SIZE)
	{
		dataArrIndex--;
	}


}
void cpyDataV2(struct pbuf *payloadPtr)
{
	uint32_t i=0;
	while(payloadPtr!=NULL)
	{
		for(i=dataArrIndex;i<(payloadPtr->len+dataArrIndex);i++)
		{
			dataArr[i]=*(uint8_t*)(payloadPtr->payload);
			payloadPtr->payload++;
		}
		payloadPtr=payloadPtr->next;
		dataArrIndex=i;

	}
	dataArrIndex=i;

}

void memoryWrite(uint32_t pageBeginAddr)
{
	if(sectorNumber < 16)
	{
		return_code = prepare_sector(sectorNumber,sectorNumber);
		if(return_code != 0)
			return;

		return_code = erase_sector(sectorNumber,sectorNumber);
		if(return_code != 0)
			return;

		return_code = prepare_sector(sectorNumber,sectorNumber);
		if(return_code != 0)
			return;

		return_code = write_to_flash(pageBeginAddr, &dataArr[0], 4096);
		if(return_code != 0)
			return;

		return_code = compare_mem(pageBeginAddr, &dataArr[0], 4096);
		if(return_code != 0)
			return;

		sectorNumber++;
	}
	else
	{
//		if(pageSize!=LARGE_PAGE_SIZE)
//			pageSize=LARGE_PAGE_SIZE;
		if(sectorFill == 0)
		{
	 		return_code = prepare_sector(sectorNumber,sectorNumber);
//	 		if(return_code != 0)
//	 			return;

	 		return_code = erase_sector(sectorNumber,sectorNumber);
//	 		if(return_code != 0)
//	 			return;
		}
		return_code = prepare_sector(sectorNumber,sectorNumber);
//		if(return_code != 0)
//			return;

		return_code = write_to_flash(pageBeginAddr,&dataArr[0], 4096);
//	 	if(return_code != 0)
//	 		return;

		return_code = compare_mem(pageBeginAddr, &dataArr[0], 4096);
//	 	if(return_code != 0)
//	 		return;

	 	sectorFill++;
	}
	if(sectorFill == 8)
	{
		sectorFill = 0;
		sectorNumber++;
	}
}

void bootLoad(struct netif *netif_eth1)
{

	while(1)
	{
		ethernetif_input(netif_eth1);
	}

}
void jump_to_app_section()
{
	uint32_t* jump_code;
	uint32_t temp=0;
	jump_code =0x10004;				//change this as the length of the bootLoader will not be 8K now.

	int APP_SECTION_ADDR = *jump_code;
	SCB->VTOR = APP_SECTION_ADDR & 0x1FFFFF80;
	temp= SCB->VTOR;
	(*((void(*)(void))APP_SECTION_ADDR))();
}
int erase_sector(unsigned int sector_start, unsigned int sector_end)
{
	command[0] = ERASE_SECTOR;
	command[1] = (unsigned int) sector_start;
	command[2] = (unsigned int) sector_end;
    command[3] = 0x30e58e;//SystemCoreClock / 1000;
    iap_entry(command,output);
    return (int) output[0];
}
int prepare_sector(unsigned int sector_start, unsigned int sector_end)
{
	command[0] = PREPARE_SECTOR;
  	command[1] = (unsigned int) sector_start;
  	command[2] = (unsigned int) sector_end;
  	command[3] = 14748;					//SystemCoreClock / 14.748MHz;
  	iap_entry(command,output);
  	return (int) output[0];
}
int write_to_flash(void* flash_add, void* ram_add, int count)
{
	command[0] = COPY_RAM_TO_FLASH;
  	command[1] = (unsigned int) flash_add;
  	command[2] = (unsigned int) ram_add;
  	command[3] = count;
  	command[4] = 0x30e58e;//SystemCoreClock / 1000;
  	iap_entry(command,output);
  	return (int) output[0];
}
int blank_check(unsigned int sector_start, unsigned int sector_end)
{
	command[0] = BLANK_CHECK_SECTOR;
  	command[1] = (unsigned int) sector_start;
  	command[2] = (unsigned int) sector_end;
  	command[3] = 14748;					//SystemCoreClock / 14.748MHz;
  	iap_entry(command,output);
  	return (int) output[0];
}
int read_part_id(void)
{
	command[0] = READ_PART_ID;
  	iap_entry(command,output);
  	return (int) output[0];
}
int read_boot_version(void)
{
	command[0] = READ_BOOT_CODE_REV;
  	iap_entry(command,output);
  	return (int) output[0];
}
int reinvoke_isp(void)
{
	__disable_irq();
  	command[0] = REINVOKE_ISP;
  	iap_entry(command,output);
  	return (int) output[0];
}
int compare_mem(void* flash_add, void* ram_add, int count)
{
	command[0] = COMPARE;
  	command[1] = (unsigned int) flash_add;
  	command[2] = (unsigned int) ram_add;
  	command[3] = count;
  	command[4] = 0x30e58e;//SystemCoreClock / 1000;
  	iap_entry(command,output);
  	return (int) output[0];
}



