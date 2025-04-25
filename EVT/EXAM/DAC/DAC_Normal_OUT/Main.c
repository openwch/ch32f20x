/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.1
* Date               : 2024/11/18
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/*
 *@Note
 *Normal output routine:
 *DAC channel 0 (PA4) output
 *Turn off the trigger function, and by writing to the data holding register, 
 *PA4 outputs the corresponding voltage.
 *
 */

#include "debug.h"

/* Global define */
#define Num 7

/* Global Variable */
u16 DAC_Value[Num] = {64, 128, 256, 512, 1024, 2048, 4095};


/*********************************************************************
 * @fn      Dac_Init
 *
 * @brief   Initializes DAC collection.
 *
 * @return  none
 */
void Dac_Init( void )
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    DAC_InitTypeDef DAC_InitType = {0};

    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_DAC, ENABLE );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    DAC_InitType.DAC_Trigger = DAC_Trigger_None;
    DAC_InitType.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitType.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    DAC_InitType.DAC_OutputBuffer = DAC_OutputBuffer_Disable ;
    DAC_Init( DAC_Channel_1, &DAC_InitType );
    DAC_Cmd( DAC_Channel_1, ENABLE );

    DAC_SetChannel1Data( DAC_Align_12b_R, 0 );
}


/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main( void )
{
    u8 i = 0;
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init( 115200 );
    Dac_Init();
    printf( "SystemClk:%d\r\n", SystemCoreClock );
    printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
    printf( "DAC Normal OUT\r\n" );
    while( 1 )
    {
        for( i = 0; i < Num; i++ ){
            DAC_SetChannel1Data( DAC_Align_12b_R, DAC_Value[i] );
            Delay_Ms( 1000 );
        }
    }
}

