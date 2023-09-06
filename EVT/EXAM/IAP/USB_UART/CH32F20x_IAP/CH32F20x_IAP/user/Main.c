/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "debug.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "iap.h"
#include "usb_istr.h"
#include "usb_desc.h"
#include "ch32f20x_usbotg_device.h"
#include "ch32f20x_usbhs_device.h"

extern u8 End_Flag;
/*
 *@Note
 *IAP upgrade routine:
 *Support serial port and USB for FLASH burning

 *1. Use the IAP download tool to realize the download PA0 floating (default pull-up input)
 *2. After downloading the APP, connect PA0 to ground (low level input), 
 *and press the reset button to run the APP program.
 *3. The routine needs to install the CH372 driver.
 *Note: FLASH operation keeps the frequency below 100Mhz, 
 *it is recommended that the main frequency of IAP be below 100Mhz
*
*/


/* Global define */


/* Global Variable */    

/*********************************************************************
 * @fn      IAP_2_APP
 *
 * @brief   IAP_2_APP program.
 *
 * @return  none
 */
void IAP_2_APP(void) {
    USB_Port_Set(DISABLE, DISABLE);
    USBOTG_H_FS->HOST_CTRL = 0x00;
    USBOTG_FS->BASE_CTRL = 0x00;
    USBOTG_FS->INT_EN = 0x00;
    USBOTG_FS->UEP2_3_MOD = 0x00;
    USBOTG_FS->BASE_CTRL |= (1 << 1) | (1 << 2);
	#if defined (CH32F20x_D8C) 
	  NVIC_DisableIRQ( USBHS_IRQn );
	#endif
    Delay_Ms(50);
	  iap_load_app(0x08000000 + User_APP_Addr_offset);
    Delay_Ms(50);
}
/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{   
	u8 usbstatus=0;	
	SystemCoreClockUpdate();
	USART_Printf_Init(115200); 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	GPIO_Cfg_init();
	Delay_Init(); 
	printf("IAP Test\r\n");
	if(PA0_Check() == 0){  
		IAP_2_APP();
		while(1);
	}
  USART3_CFG(57600);
#if defined (CH32F20x_D6) || defined (CH32F20x_D8) || defined (CH32F20x_D8W)	
 	USBD_CFG();
#endif	
	USBOTG_Init( );  
#if defined (CH32F20x_D8C) 
  USBHS_RCC_Init( );
 USBHS_Device_Init( ENABLE );
  NVIC_EnableIRQ( USBHS_IRQn );

#endif

	while(1)
	{
#if defined (CH32F20x_D6) || defined (CH32F20x_D8) || defined (CH32F20x_D8W)	
		if(usbstatus!=bDeviceState)
		{
			usbstatus=bDeviceState;
			
			if(usbstatus==CONFIGURED)
			{	
				
			}else
			{
				
			}
		}
		
		EP2_RecData_Deal();
#endif	
		if( USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET){
			UART_Rx_Deal();
		}		
		        if (End_Flag)
        {
            Delay_Ms(50);
            IAP_2_APP();
            while(1);
        }
		IWDG_ReloadCounter();
	}
}







