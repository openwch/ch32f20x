/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2022/08/20
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/ 

/*
 *@Note
  Emulating a custom USB device (CH372 device) routine (only for, CH32F20x_D6-CH32F20x_D8W-CH32F20x_D8C).
  This routine demonstrates the use of USBFS to emulate a custom CH372 device, 
  with endpoints 1/3/5 downlinking data and uploading via endpoints 2/4/6 respectively
  where endpoint 1/2 is implemented via a ring buffer and the data is not inverted, 
  and endpoints 3/4 and 5/6 are directly copied and inverted for upload.
  The device can be operated using Bushund or other upper computer software.
  Note: This routine needs to be demonstrated in conjunction with the host software.
*/

#include "ch32f20x_usbfs_device.h"
#include "debug.h"

/*********************************************************************
 * @fn      Var_Init
 *
 * @brief   Software parameter initialisation
 *
 * @return  none
 */
void Var_Init(void)
{
    uint16_t i;
    RingBuffer_Comm.LoadPtr = 0;
    RingBuffer_Comm.StopFlag = 0;
    RingBuffer_Comm.DealPtr = 0;
    RingBuffer_Comm.RemainPack = 0;
    for(i=0; i<DEF_Ring_Buffer_Max_Blks; i++)
    {
        RingBuffer_Comm.PackLen[i] = 0;
    }
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
    uint8_t ret;
		Delay_Init();
		USART_Printf_Init( 115200 );
		printf("SystemClk:%d\r\n",SystemCoreClock);

    /* USBOTG_FS device init */
		printf( "CH372Device Running On USBHD(FS) Controller\r\n" );
		Delay_Ms(10);

    /* Variables init */
    Var_Init( );
    
    /* USBHD Device Init */
    /* Usb Init */
    USBFS_RCC_Init( );
    USBFS_Device_Init( ENABLE );
#if defined (CH32F20x_D6) || defined (CH32F20x_D8W)
    NVIC_EnableIRQ( USBHD_IRQn );
#elif defined (CH32F20x_D8C)
    NVIC_EnableIRQ( OTG_FS_IRQn );
#endif

	while(1)
	{
				/* Determine if enumeration is complete, perform data transfer if completed */
        if( USBFS_DevEnumStatus )
        {
            /* Data Transfer */
            if( RingBuffer_Comm.RemainPack )
            {
                ret = USBFS_Endp_DataUp( DEF_UEP2, &Data_Buffer[(RingBuffer_Comm.DealPtr) * DEF_USBD_FS_PACK_SIZE], RingBuffer_Comm.PackLen[RingBuffer_Comm.DealPtr], DEF_UEP_DMA_LOAD );
                if( ret == 0 )
                {
#if defined (CH32F20x_D6) || defined (CH32F20x_D8W)
                    NVIC_DisableIRQ( USBHD_IRQn );
#elif defined (CH32F20x_D8C)
                    NVIC_DisableIRQ( OTG_FS_IRQn );
#endif
                    RingBuffer_Comm.RemainPack--;
                    RingBuffer_Comm.DealPtr++;
                    if(RingBuffer_Comm.DealPtr == DEF_Ring_Buffer_Max_Blks)
                    {
                        RingBuffer_Comm.DealPtr = 0;
                    }
#if defined (CH32F20x_D6) || defined (CH32F20x_D8W)
                    NVIC_EnableIRQ( USBHD_IRQn );
#elif defined (CH32F20x_D8C)
                    NVIC_EnableIRQ( OTG_FS_IRQn );
#endif
                }
            }

            /* Monitor whether the remaining space is available for further downloads */
            if(RingBuffer_Comm.RemainPack < (DEF_Ring_Buffer_Max_Blks - DEF_RING_BUFFER_RESTART))
            {
                if(RingBuffer_Comm.StopFlag)
                {
#if defined (CH32F20x_D6) || defined (CH32F20x_D8W)
                    NVIC_DisableIRQ( USBHD_IRQn );
#elif defined (CH32F20x_D8C)
                    NVIC_DisableIRQ( OTG_FS_IRQn );
#endif
                    RingBuffer_Comm.StopFlag = 0;
#if defined (CH32F20x_D6) || defined (CH32F20x_D8W)
                    NVIC_EnableIRQ( USBHD_IRQn );
#elif defined (CH32F20x_D8C)
                    NVIC_EnableIRQ( OTG_FS_IRQn );
#endif
                    USBOTG_FS->UEP1_RX_CTRL = (USBOTG_FS->UEP1_RX_CTRL & ~USBFS_UEP_R_RES_MASK) | USBFS_UEP_R_RES_ACK;
                }
            }
        }
    }
}






