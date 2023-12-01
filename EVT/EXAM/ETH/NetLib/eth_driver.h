/********************************** (C) COPYRIGHT *******************************
* File Name          : eth_driver.h
* Author             : WCH
* Version            : V1.3.0
* Date               : 2022/06/02
* Description        : This file contains the headers of the ETH Driver.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __ETH_DRIVER__
#define __ETH_DRIVER__

#ifdef __cplusplus
 extern "C" {
#endif 
#include "ch32F20x.h"
#include "string.h"
#include "debug.h"
#include "wchnet.h"

#if defined(CH32F20x_D8C)	

 /* 1: interrupt 0: polling in RMII or RGMII mode */
#define LINK_STAT_ACQUISITION_METHOD            1

#define PHY_ADDRESS                             1

#define ETH_DMARxDesc_FrameLengthShift          16

#define ROM_CFG_USERADR_ID                      0x1FFFF7E8

#define PHY_LINK_TASK_PERIOD                    50

#define PHY_ANLPAR_SELECTOR_FIELD               0x1F
#define PHY_ANLPAR_SELECTOR_VALUE               0x01       /* 5B'00001 */

#define PHY_LINK_INIT                           0x00
#define PHY_LINK_SUC_P                          (1<<0)
#define PHY_LINK_SUC_N                          (1<<1)
#define PHY_LINK_WAIT_SUC                       (1<<7)

#define PHY_PN_SWITCH_P                         (0<<2)
#define PHY_PN_SWITCH_N                         (1<<2)
#define PHY_PN_SWITCH_AUTO                      (2<<2)

#ifndef WCHNETTIMERPERIOD
#define WCHNETTIMERPERIOD                       10   /* Timer period, in Ms. */
#endif

#define GPIO_Output(a,b) \
  GPIO_InitStructure.GPIO_Pin = b;\
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;\
  GPIO_Init(a, &GPIO_InitStructure)

#define GPIO_Input(a,b) \
  GPIO_InitStructure.GPIO_Pin = b;\
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;\
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;\
  GPIO_Init(a, &GPIO_InitStructure)

#define QUERY_STAT_FLAG  ((LastQueryPhyTime == (LocalTime / 1000)) ? 0 : 1)

#define ACCELERATE_LINK_PROCESS() do{\
    if((TRDetectStep < 2) && (ETH_ReadPHYRegister(gPHYAddress, PHY_ANLPAR) & PHY_ANLPAR_SELECTOR_FIELD))\
        LinkTaskPeriod = 0;\
}while(0)

#define UPDATE_LINKTASKPERIOD() do{\
    if(TRDetectStep == 1)\
    {\
        RandVal = RandVal * 214017 + 2531017;\
        LinkTaskPeriod = RandVal%100 + 50;\
    }\
    else {\
        LinkTaskPeriod = 50;\
    }\
}while(0)

#define PHY_RESTART_AUTONEGOTIATION()       do{\
    RegVal = ETH_ReadPHYRegister(gPHYAddress, PHY_BCR);\
    RegVal &= ~0x01;\
    RegVal |= PHY_Restart_AutoNegotiation;\
    ETH_WritePHYRegister( gPHYAddress, PHY_BCR, RegVal);\
    RegVal = ETH_ReadPHYRegister(gPHYAddress, PHY_BCR);\
    RegVal |= 0x03 | PHY_Restart_AutoNegotiation;\
    ETH_WritePHYRegister( gPHYAddress, PHY_BCR, RegVal);\
}while(0)

#define PHY_TR_SWITCH()    do{\
    phy_mdix = ETH_ReadPHYRegister(gPHYAddress, PHY_MDIX);\
    if(phy_mdix & 0x01)\
    {\
        phy_mdix &= ~0x03;\
        phy_mdix |= 1 << 1;\
    }\
    else\
    {\
        phy_mdix &= ~0x03;\
        phy_mdix |= 1 << 0;\
    }\
    ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, phy_mdix);\
    PHY_RESTART_AUTONEGOTIATION();\
}while(0)

#define PHY_TR_REVERSE()       do{\
    if(phyStatus)\
    {\
        RegVal = ETH_ReadPHYRegister(gPHYAddress, PHY_MDIX);\
        if(RegVal & 0x01)\
        {\
            RegVal &= ~0x03;\
            RegVal |= 1 << 1;\
        }\
        else{\
            RegVal &= ~0x03;\
            RegVal |= 1 << 0;\
        }\
        ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, RegVal);\
    }\
}while(0)

#define PHY_PN_SWITCH(PNMode)   do{\
    if(PNMode == PHY_PN_SWITCH_AUTO)\
    {\
         phyPN = PHY_PN_SWITCH_AUTO;\
    }\
    else{\
        phyPN = (ETH_ReadPHYRegister(gPHYAddress, PHY_MDIX) & (~(0x03 << 2))) | PNMode;\
    }\
    ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, phyPN);\
    phyPN = PNMode;\
    PHY_RESTART_AUTONEGOTIATION();\
}while(0)

#define PHY_NEGOTIATION_PARAM_INIT()    do{\
    phyStatus = 0;\
    phySucCnt = 0;\
    phyLinkCnt = 0;\
    TRDetectStep = 0;\
    PhyPolarityDetect = 0;\
    phyLinkStatus = PHY_LINK_INIT;\
    phyPN = PHY_PN_SWITCH_AUTO;\
    ETH_WritePHYRegister(gPHYAddress, PHY_MDIX, phyPN);\
}while(0)

#define PHY_LINK_RESET()       do{\
    ETH_WritePHYRegister(gPHYAddress, PHY_BCR, PHY_Reset);\
    PHY_NEGOTIATION_PARAM_INIT();\
}while(0)

extern ETH_DMADESCTypeDef *DMATxDescToSet;
extern ETH_DMADESCTypeDef *DMARxDescToGet;
extern SOCK_INF SocketInf[ ];

void ETH_PHYLink( void );
void WCHNET_ETHIsr( void );
void WCHNET_MainTask( void );
void ETH_LedConfiguration(void);
void ETH_Init( uint8_t *macAddr );
void ETH_LedLinkSet( uint8_t mode );
void ETH_LedDataSet( uint8_t mode );
void WCHNET_TimeIsr( uint16_t timperiod );
void ETH_Configuration( uint8_t *macAddr );
uint8_t ETH_LibInit( uint8_t *ip, uint8_t *gwip, uint8_t *mask, uint8_t *macaddr);

#elif defined(CH32F20x_D8W)	

#define ROM_CFG_USERADR_ID                      0x1FFFF7E8

#define PHY_LINK_TASK_PERIOD                    50

#define PHY_ANLPAR_SELECTOR_FIELD               0x1F
#define PHY_ANLPAR_SELECTOR_VALUE               0x01        /* 5B'00001 */

#define PHY_LINK_INIT                           0x00
#define PHY_LINK_SUC_P                          (1<<0)
#define PHY_LINK_SUC_N                          (1<<1)
#define PHY_LINK_WAIT_SUC                       (1<<7)

#define PHY_PN_SWITCH_P                         (0<<2)
#define PHY_PN_SWITCH_N                         (1<<2)
#define PHY_PN_SWITCH_AUTO                      (2<<2)

#ifndef WCHNETTIMERPERIOD
#define WCHNETTIMERPERIOD                       10   /* Timer period, in Ms. */
#endif

#define PHY_NEGOTIATION_PARAM_INIT()      do{\
        phySucCnt = 0;\
        phyStatus = 0;\
        phyLinkCnt = 0;\
        phyRetryCnt = 0;\
        phyPNChangeCnt = 0;\
        phyLinkStatus = PHY_LINK_INIT;\
}while(0)

/* definition for Ethernet frame */
#define ETH_MAX_PACKET_SIZE    1536    /* ETH_HEADER + VLAN_TAG + MAX_ETH_PAYLOAD + ETH_CRC */
#define ETH_HEADER               14    /* 6 byte Dest addr, 6 byte Src addr, 2 byte length/type */
#define ETH_CRC                   4    /* Ethernet CRC */
#define ETH_EXTRA                 2    /* Extra bytes in some cases */
#define VLAN_TAG                  4    /* optional 802.1q VLAN Tag */
#define MIN_ETH_PAYLOAD          46    /* Minimum Ethernet payload size */
#define MAX_ETH_PAYLOAD        1500    /* Maximum Ethernet payload size */

/* Bit or field definition of TDES0 register (DMA Tx descriptor status register)*/
#define ETH_DMATxDesc_OWN         ((uint32_t)0x80000000)  /* OWN bit: descriptor is owned by DMA engine */

/* Bit or field definition of RDES0 register (DMA Rx descriptor status register) */
#define ETH_DMARxDesc_OWN         ((uint32_t)0x80000000)  /* OWN bit: descriptor is owned by DMA engine  */
#define ETH_DMARxDesc_FL          ((uint32_t)0x3FFF0000)  /* Receive descriptor frame length  */
#define ETH_DMARxDesc_ES          ((uint32_t)0x00008000)  /* Error summary:  */
#define ETH_DMARxDesc_FS          ((uint32_t)0x00000200)  /* First descriptor of the frame  */
#define ETH_DMARxDesc_LS          ((uint32_t)0x00000100)  /* Last descriptor of the frame  */

#define ETH_DMARxDesc_FrameLengthShift       16

/* ETHERNET errors */
#define  ETH_ERROR              ((uint32_t)0)
#define  ETH_SUCCESS            ((uint32_t)1)

#include "wchnet.h"

extern SOCK_INF SocketInf[ ];

void ETH_PHYLink( void );
void WCHNET_ETHIsr( void );
void WCHNET_MainTask( void );
void ETH_LedConfiguration(void);
void ETH_Init( uint8_t *macAddr );
void ETH_LedLinkSet( uint8_t mode );
void ETH_LedDataSet( uint8_t mode );
void WCHNET_TimeIsr( uint16_t timperiod );
void ETH_Configuration( uint8_t *macAddr );
uint8_t ETH_LibInit( uint8_t *ip, uint8_t *gwip, uint8_t *mask, uint8_t *macaddr);

#endif
#ifdef __cplusplus
}
#endif

#endif
