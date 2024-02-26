/********************************** (C) COPYRIGHT  *******************************
* File Name          : iap.h
* Author             : WCH
* Version            : V1.0.0
* Date               : 2020/12/16
* Description        : IAP
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __IAP_H
#define __IAP_H 	

#include "ch32f20x.h"
#include "stdio.h"
#include "usb_desc.h"
#include "ch32f20x_usbfs_device.h"
#include "ch32f20x_usbhs_device.h"
#define BUILD_UINT16(loByte, hiByte) ((uint16_t)(((loByte) & 0x00FF) | (((hiByte) & 0x00FF) << 8)))
#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((UINT32)((UINT32)((Byte0) & 0x00FF) \
          + ((((UINT32)Byte1) & 0x00FF) << 8) \
          + ((((UINT32)Byte2) & 0x00FF) << 16) \
          + ((((UINT32)Byte3) & 0x00FF) << 24)))

/* Frame header of UART  */
#define Uart_Sync_Head1   0x57
#define Uart_Sync_Head2   0xab


/* cmd */
#define CMD_IAP_PROM      0x80        
#define CMD_IAP_ERASE     0x81        
#define CMD_IAP_VERIFY    0x82        
#define CMD_IAP_END       0x83       

/* return state */
#define ERR_SCUESS        0x00
#define ERR_ERROR         0x01
#define ERR_End           0x02

typedef struct _ISP_CMD{
	u8 Cmd;                              
	u8 Len;                             
	u8 Rev[2];                             
	u8 data[60];	
}isp_cmd;

typedef  void (*iapfun)(void);				

extern u32 Program_Verity_addr;
extern u32 User_APP_Addr_offset;
extern u8 EP2_IN_Flag;
extern u8 EP2_OUT_Flag;
extern u8 EP2_Rx_Buffer[USBD_DATA_SIZE];
extern u16 EP2_Rx_Cnt;
extern u8 EP2_Tx_Buffer[2];
extern u16 EP2_Tx_Cnt; 

void EP2_RecData_Deal(void); 
void GPIO_Cfg_init(void);
u8 PA0_Check(void);
void iap_load_app(u32 appxaddr);
void USART3_CFG(u32 baudrate);
void USBD_CFG(void);
void UART_Rx_Deal(void);

#endif
