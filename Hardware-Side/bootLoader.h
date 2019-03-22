#ifndef BOOTLOADER_H_   /* Include guard */
#define BOOTLOADER_H_

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

#include "delay.h"


#include "system_init.h"

#define IAP_LOCATION  0x1FFF1FF1


#define PREPARE_SECTOR        50
#define COPY_RAM_TO_FLASH     51
#define ERASE_SECTOR          52
#define BLANK_CHECK_SECTOR    53
#define READ_PART_ID          54
#define READ_BOOT_CODE_REV    55
#define COMPARE               56
#define REINVOKE_ISP          57
#define READ_UID              58

#define TIME_COUNTER_BOOTLOAD  5000
#define SEGMENT_SIZE			512
#define MAX_SEGMENT_NUMBER		7
#define SMALL_PAGE_SIZE			4096	//4 KB.
#define LARGE_PAGE_SIZE			32768	//32 KB.





typedef unsigned int (*IAP)(unsigned int [],unsigned int[]);
static const IAP iap_entry = (IAP) IAP_LOCATION;

void bootLoad(struct netif *netif_eth1);
uint32_t addrDecode(uint8_t *addrPointer);
void memoryWrite(uint32_t beginAddr);
void cpyData(uint8_t *dataPointer);
void jump_to_app_section();
int erase_sector(unsigned int sector_start, unsigned int sector_end);
int prepare_sector(unsigned int sector_start, unsigned int sector_end);
int write_to_flash(void* flash_add, void* ram_add, int count);
int blank_check(unsigned int sector_start, unsigned int sector_end);
int read_part_id(void);
int read_boot_version(void);
int reinvoke_isp(void);
int compare_mem(void* flash_add, void* ram_add, int count);

#endif 
