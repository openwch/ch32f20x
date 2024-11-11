/********************************** (C) COPYRIGHT *******************************
* File Name          : eth_driver.c
* Author             : WCH
* Version            : V1.3.0
* Date               : 2022/06/02
* Description        : eth program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

#include "string.h"
#include "eth_driver.h"

__attribute__((__aligned__(4))) ETH_DMADESCTypeDef DMARxDscrTab[ETH_RXBUFNB];       /* MAC receive descriptor, 4-byte aligned*/
__attribute__((__aligned__(4))) ETH_DMADESCTypeDef DMATxDscrTab[ETH_TXBUFNB];       /* MAC send descriptor, 4-byte aligned */

__attribute__((__aligned__(4))) uint8_t  MACRxBuf[ETH_RXBUFNB*ETH_RX_BUF_SZE];      /* MAC receive buffer, 4-byte aligned */
__attribute__((__aligned__(4))) uint8_t  MACTxBuf[ETH_TXBUFNB*ETH_TX_BUF_SZE];      /* MAC send buffer, 4-byte aligned */

uint16_t gPHYAddress;
uint32_t ChipId = 0;
uint32_t volatile LocalTime;

ETH_DMADESCTypeDef *pDMARxSet;
ETH_DMADESCTypeDef *pDMATxSet;

extern u8 MACAddr[6];
volatile uint8_t LinkSta = 0;  //0:Link down 1:Link up
uint8_t LinkVaildFlag = 0;  //0:invalid 1:valid
uint8_t AccelerateLinkFlag = 0; //0:invalid 1:valid
uint8_t LinkProcessingStep = 0;
uint32_t LinkProcessingTime = 0;
uint32_t TaskExecutionTime = 0;

#if !LINK_STAT_ACQUISITION_METHOD
u16 LastPhyStat = 0;
u32 LastQueryPhyTime = 0;
#endif
uint8_t PhyWaitNegotiationSuc = 0;
void ETH_LinkDownCfg(void);
/*********************************************************************
 * @fn      WCHNET_GetMacAddr
 *
 * @brief   Get the MAC address
 *
 * @return  none.
 */
void WCHNET_GetMacAddr( uint8_t *p )
{
    uint8_t i;
    uint8_t *macaddr = (uint8_t *)(ROM_CFG_USERADR_ID+5);

    for(i=0;i<6;i++)
    {
        *p = *macaddr;
        p++;
        macaddr--;
    }
}

/*********************************************************************
 * @fn      WCHNET_TimeIsr
 *
 * @brief
 *
 * @return  none.
 */
void WCHNET_TimeIsr( uint16_t timperiod )
{
    LocalTime += timperiod;
}

/*********************************************************************
 * @fn      WCHNET_QueryPhySta
 *
 * @brief   Query external PHY status
 *
 * @return  none.
 */

void WCHNET_QueryPhySta(void)
{
#if LINK_STAT_ACQUISITION_METHOD
    if(PhyWaitNegotiationSuc)
    {
        ETH_PHYLink();
    }
#else
    u16 phy_stat;
    if(QUERY_STAT_FLAG){                                         /* Query the PHY link status every 1s */
        LastQueryPhyTime = LocalTime / 1000;
        phy_stat = ETH_ReadPHYRegister( PHY_ADDRESS, PHY_BSR );
        if(phy_stat != LastPhyStat){
            ETH_PHYLink();
        }
    }
#endif
}

/*********************************************************************
 * @fn      WCHNET_CheckPHYPN
 *
 * @brief   check PHY PN polarity
 *
 * @return  none.
 */
void WCHNET_CheckPHYPN(uint16_t time)
{
    u16 phy_stat;

    //check PHY PN
    if((LinkProcessingStep == 0)||(LocalTime >= LinkProcessingTime))
    {
        ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x0 );
        phy_stat = ETH_ReadPHYRegister( gPHYAddress, 0x10);
        if(phy_stat & (1<<12))
        {
            if(LinkProcessingStep == 0)
            {
                LinkProcessingStep = 1;
                LinkProcessingTime = LocalTime + time;
            }
            else {
                LinkProcessingStep = 0;
                LinkProcessingTime = 0;
                phy_stat = ETH_ReadPHYRegister( gPHYAddress, PHY_ANER);
                if((time == 200) || ((phy_stat & 1) == 0))
                {
#if LINK_STAT_ACQUISITION_METHOD
                    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x07 );
                    /* Configure interrupt function */
                    phy_stat = ETH_ReadPHYRegister(gPHYAddress, 0x13);
                    phy_stat &= ~(0x01 << 13);
                    ETH_WritePHYRegister(gPHYAddress, 0x13, phy_stat );
#endif
                    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x0 );
                    phy_stat = ETH_ReadPHYRegister( gPHYAddress, 0x16);
                    phy_stat |= 1<<5;
                    ETH_WritePHYRegister(gPHYAddress, 0x16, phy_stat );

                    phy_stat = ETH_ReadPHYRegister( gPHYAddress, 0x16);
                    phy_stat &= ~(1<<5);
                    ETH_WritePHYRegister(gPHYAddress, 0x16, phy_stat );

                    phy_stat = ETH_ReadPHYRegister( gPHYAddress, 0x1E);   /* Clear the Interrupt status */

#if LINK_STAT_ACQUISITION_METHOD
                    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x07 );
                    /* Configure interrupt function */
                    phy_stat = ETH_ReadPHYRegister(gPHYAddress, 0x13);
                    phy_stat |= 0x01 << 13;
                    ETH_WritePHYRegister(gPHYAddress, 0x13, phy_stat );
#endif
                }
            }
        }
        else {
            LinkProcessingStep = 0;
            LinkProcessingTime = 0;
        }
    }
}

/*********************************************************************
 * @fn      WCHNET_AccelerateLink
 *
 * @brief   accelerate Link processing
 *
 * @return  none.
 */
void WCHNET_AccelerateLink(void)
{
    uint16_t phy_stat;
    if(AccelerateLinkFlag == 0)
    {
        ETH_WritePHYRegister(gPHYAddress, 0x1F, 99 );
        phy_stat = ETH_ReadPHYRegister( gPHYAddress, 0x19);
        if((phy_stat & 0xf) == 3)
        {
            AccelerateLinkFlag = 1;
            ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x0 );
            phy_stat = 0x4;
            ETH_WritePHYRegister(gPHYAddress, 0x13, phy_stat );
        }
    }
}

/*********************************************************************
 * @fn      WCHNET_CheckLinkVaild
 *
 * @brief   check whether Link is valid
 *
 * @return  none.
 */
void WCHNET_CheckLinkVaild(void)
{
    uint16_t phy_stat, phy_bcr;

    if(LinkVaildFlag == 0)
    {
        phy_bcr = ETH_ReadPHYRegister( PHY_ADDRESS, PHY_BCR);
        if((phy_bcr & (1<<13)) == 0)   //Do nothing if Link mode is 10M.
        {
            LinkVaildFlag = 1;
            LinkProcessingTime = 0;
            return;
        }
        ETH_WritePHYRegister(gPHYAddress, 0x1F, 0 );
        phy_stat = ETH_ReadPHYRegister( gPHYAddress, 0x10);
        if((phy_stat & (1<<9)) == 0)
        {
            LinkProcessingTime++;
            if(LinkProcessingTime == 5)
            {
                LinkProcessingTime = 0;
                phy_stat = ETH_ReadPHYRegister(gPHYAddress, PHY_BCR);
                ETH_WritePHYRegister(gPHYAddress, PHY_BCR, PHY_Reset );
                Delay_Us(100);
                ETH_WritePHYRegister(gPHYAddress, PHY_BCR, phy_stat );
                ETH_LinkDownCfg();
            }
        }
        else {
            LinkVaildFlag = 1;
            LinkProcessingTime = 0;
        }
    }
}

/*********************************************************************
 * @fn      WCHNET_LinkProcessing
 *
 * @brief   process Link stage task
 *
 * @return  none.
 */
void WCHNET_LinkProcessing(void)
{
    u16 phy_bcr;

    if(LocalTime >= TaskExecutionTime)
    {
        TaskExecutionTime = LocalTime + 10;         //execution cycle:10ms
        if(LinkSta == 0)                            //Link down
        {
            phy_bcr = ETH_ReadPHYRegister( PHY_ADDRESS, PHY_BCR);
            if(phy_bcr & PHY_AutoNegotiation)       //auto-negotiation is enabled
            {
                WCHNET_CheckPHYPN(300);             //check PHY PN
                WCHNET_AccelerateLink();            //accelerate Link processing
            }
            else {                                  //auto-negotiation is disabled
                if((phy_bcr & (1<<13)) == 0)        // 10M
                {
                    WCHNET_CheckPHYPN(200);         //check PHY PN
                }
            }
        }
        else {                                      //Link up
            WCHNET_CheckLinkVaild();                //check whether Link is valid
        }
    }
}

/*********************************************************************
 * @fn      RecDataPolling
 *
 * @brief   process received data.
 *
 * @param   none.
 *
 * @return  none.
 */
void RecDataPolling(void)
{
    uint32_t length, buffer;

    while(!(pDMARxSet->Status & ETH_DMARxDesc_OWN))
    {
        if( !(pDMARxSet->Status & ETH_DMARxDesc_ES) &&\
        (pDMARxSet->Status & ETH_DMARxDesc_LS) &&\
        (pDMARxSet->Status & ETH_DMARxDesc_FS) )
        {
            /* Get the Frame Length of the received packet: substruct 4 bytes of the CRC */
            length = (pDMARxSet->Status & ETH_DMARxDesc_FL) >> 16;
            /* Get the addrees of the actual buffer */
            buffer = pDMARxSet->Buffer1Addr;

            /* Do something*/
            printf("rec data:%d bytes\r\n",length);
            printf("data:%x\r\n",*((uint8_t *)buffer));
        }
        pDMARxSet->Status = ETH_DMARxDesc_OWN;
        pDMARxSet = (ETH_DMADESCTypeDef *)pDMARxSet->Buffer2NextDescAddr;
    }
}

/*********************************************************************
 * @fn      WCHNET_MainTask
 *
 * @brief   library main task function
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_MainTask(void)
{
    RecDataPolling();
    WCHNET_QueryPhySta();                   /* Query external PHY status */
    WCHNET_LinkProcessing();                /* process Link stage task */
}

/*********************************************************************
 * @fn      ETH_RMIIPinInit
 *
 * @brief   PHY RMII interface GPIO initialization.
 *
 * @param   none.
 *
 * @return  none.
 */
void ETH_RMIIPinInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_ETH_MediaInterfaceConfig(GPIO_ETH_MediaInterface_RMII);
    GPIO_Output(GPIOA, GPIO_Pin_2);                                                         /* MDIO */
    GPIO_Output(GPIOC, GPIO_Pin_1);                                                         /* MDC */

    GPIO_Output(GPIOB, GPIO_Pin_11);                                                        /* TXEN */
    GPIO_Output(GPIOB, GPIO_Pin_12);                                                        /* TXD0 */
    GPIO_Output(GPIOB, GPIO_Pin_13);                                                        /* TXD1 */

    GPIO_Input(GPIOA, GPIO_Pin_1);                                                          /* REFCLK */
    GPIO_Input(GPIOA, GPIO_Pin_7);                                                          /* CRSDV */
    GPIO_Input(GPIOC, GPIO_Pin_4);                                                          /* RXD0 */
    GPIO_Input(GPIOC, GPIO_Pin_5);                                                          /* RXD1 */
}

#if LINK_STAT_ACQUISITION_METHOD
/*********************************************************************
 * @fn      EXTI_Line_Init
 *
 * @brief   Configure EXTI Line7.
 *
 * @param   none.
 *
 * @return  none.
 */
void EXTI_Line_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* GPIOC 7 ----> EXTI_Line7 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource7);
    EXTI_InitStructure.EXTI_Line = EXTI_Line7;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*********************************************************************
 * @fn      PHY_InterruptInit
 *
 * @brief   Configure PHY interrupt function,Supported chip is:RTL8211FS
 *
 * @param   none.
 *
 * @return  none.
 */
void PHY_InterruptInit(void)
{
    uint16_t RegValue;

    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x07 );
    /* Configure interrupt function */
    RegValue = ETH_ReadPHYRegister(gPHYAddress, 0x13);
    RegValue |= 0x01 << 13;
    ETH_WritePHYRegister(gPHYAddress, 0x13, RegValue );
    /* Clear the Interrupt status */
    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x00 );
    ETH_ReadPHYRegister( gPHYAddress, 0x1E);
}
#endif

/*********************************************************************
 * @fn      ETH_LinkUpCfg
 *
 * @brief   When the PHY is connected, configure the relevant functions.
 *
 * @param   none.
 *
 * @return  none.
 */
void ETH_LinkUpCfg(void)
{
    uint16_t phy_stat;

    printf("Link up\r\n");
    phy_stat = ETH_ReadPHYRegister( PHY_ADDRESS, PHY_BCR );
    /* PHY negotiation result */
    if(phy_stat & (1<<13))                                  /* 100M */
    {
        ETH->MACCR &= ~(ETH_Speed_100M|ETH_Speed_1000M);
        ETH->MACCR |= ETH_Speed_100M;
    }
    else                                                    /* 10M */
    {
        ETH->MACCR &= ~(ETH_Speed_100M|ETH_Speed_1000M);
    }
    if(phy_stat & (1<<8))                                   /* full duplex */
    {
        ETH->MACCR |= ETH_Mode_FullDuplex;
    }
    else                                                    /* half duplex */
    {
        ETH->MACCR &= ~ETH_Mode_FullDuplex;
    }

    LinkSta = 1;
    AccelerateLinkFlag = 0;
    LinkProcessingStep = 0;
    LinkProcessingTime = 0;
    PhyWaitNegotiationSuc = 0;
    ETH_Start( );

    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x0 );
    phy_stat = 0x0;
    ETH_WritePHYRegister(gPHYAddress, 0x13, phy_stat );
}

/*********************************************************************
 * @fn      ETH_LinkDownCfg
 *
 * @brief   When the PHY is disconnected, configure the relevant functions.
 *
 * @param   none.
 *
 * @return  none.
 */
void ETH_LinkDownCfg(void)
{
    printf("Link down\r\n");
    LinkSta = 0;
    LinkVaildFlag = 0;
    LinkProcessingTime = 0;
}

/*********************************************************************
 * @fn      ETH_PHYLink
 *
 * @brief   Configure MAC parameters after the PHY Link is successful.
 *
 * @param   none.
 *
 * @return  none.
 */
void ETH_PHYLink( void )
{
    u32 phy_stat, phy_anlpar, phy_bcr;
#if !LINK_STAT_ACQUISITION_METHOD
    uint8_t timeout = 0;
#endif
    phy_stat = ETH_ReadPHYRegister( PHY_ADDRESS, PHY_BSR );
    phy_anlpar = ETH_ReadPHYRegister( PHY_ADDRESS, PHY_ANLPAR);
    phy_bcr = ETH_ReadPHYRegister( gPHYAddress, PHY_BCR);

#if !LINK_STAT_ACQUISITION_METHOD
    if((ChipId & 0xf0) <= 0x20)
    {
        while(phy_stat == 0)
        {
            Delay_Us(100);
            phy_stat = ETH_ReadPHYRegister( gPHYAddress, PHY_BSR);
            if(timeout++ == 15)   break;
        }
        if(LastPhyStat == phy_stat) return;
    }
    LastPhyStat = phy_stat;
#endif

    if(phy_stat & PHY_Linked_Status)   //LinkUp
    {
        if(phy_bcr & PHY_AutoNegotiation)
        {
            if(phy_anlpar == 0)
            {
                ETH_LinkUpCfg();
            }
            else {
                if(phy_stat & PHY_AutoNego_Complete)
                {
                    ETH_LinkUpCfg();
                }
                else{
                    PhyWaitNegotiationSuc = 1;
                }
            }
        }
        else {
            ETH_LinkUpCfg();
        }
    }
    else {                              //LinkDown
        /*Link down*/
        ETH_LinkDownCfg();
    }
    phy_stat = ETH_ReadPHYRegister( gPHYAddress, 0x1E);   /* Clear the Interrupt status */
}

/*********************************************************************
 * @fn      ETH_RegInit
 *
 * @brief   ETH register initialization.
 *
 * @param   ETH_InitStruct:initialization struct.
 *          PHYAddress:PHY address.
 *
 * @return  Initialization status.
 */
uint32_t ETH_RegInit( ETH_InitTypeDef* ETH_InitStruct, uint16_t PHYAddress )
{
    uint32_t tmpreg = 0;

    /*---------------------- Physical layer configuration -------------------*/
    /* Set the SMI interface clock, set as the main frequency divided by 42  */
    tmpreg = ETH->MACMIIAR;
    tmpreg &= MACMIIAR_CR_MASK;
    tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div42;
    ETH->MACMIIAR = (uint32_t)tmpreg;

    /*------------------------ MAC register configuration  ----------------------- --------------------*/
    tmpreg = ETH->MACCR;
    tmpreg &= MACCR_CLEAR_MASK;
    tmpreg |= (uint32_t)(ETH_InitStruct->ETH_Watchdog |
                  ETH_InitStruct->ETH_Jabber |
                  ETH_InitStruct->ETH_InterFrameGap |
                  ETH_InitStruct->ETH_ChecksumOffload |
                  ETH_InitStruct->ETH_AutomaticPadCRCStrip |
                  ETH_InitStruct->ETH_DeferralCheck |
                  (1 << 20));
    /* Write MAC Control Register */
    ETH->MACCR = (uint32_t)tmpreg;
    ETH->MACFFR = (uint32_t)(ETH_InitStruct->ETH_ReceiveAll |
                          ETH_InitStruct->ETH_SourceAddrFilter |
                          ETH_InitStruct->ETH_PassControlFrames |
                          ETH_InitStruct->ETH_BroadcastFramesReception |
                          ETH_InitStruct->ETH_DestinationAddrFilter |
                          ETH_InitStruct->ETH_PromiscuousMode |
                          ETH_InitStruct->ETH_MulticastFramesFilter |
                          ETH_InitStruct->ETH_UnicastFramesFilter);
    /*--------------- ETHERNET MACHTHR and MACHTLR Configuration ---------------*/
    /* Write to ETHERNET MACHTHR */
    ETH->MACHTHR = (uint32_t)ETH_InitStruct->ETH_HashTableHigh;
    /* Write to ETHERNET MACHTLR */
    ETH->MACHTLR = (uint32_t)ETH_InitStruct->ETH_HashTableLow;
    /*----------------------- ETHERNET MACFCR Configuration --------------------*/
    /* Get the ETHERNET MACFCR value */
    tmpreg = ETH->MACFCR;
    /* Clear xx bits */
    tmpreg &= MACFCR_CLEAR_MASK;
    tmpreg |= (uint32_t)((ETH_InitStruct->ETH_PauseTime << 16) |
                     ETH_InitStruct->ETH_UnicastPauseFrameDetect |
                     ETH_InitStruct->ETH_ReceiveFlowControl |
                     ETH_InitStruct->ETH_TransmitFlowControl);
    ETH->MACFCR = (uint32_t)tmpreg;

    ETH->MACVLANTR = (uint32_t)(ETH_InitStruct->ETH_VLANTagComparison |
                               ETH_InitStruct->ETH_VLANTagIdentifier);

    tmpreg = ETH->DMAOMR;
    tmpreg &= DMAOMR_CLEAR_MASK;
    tmpreg |= (uint32_t)(ETH_InitStruct->ETH_DropTCPIPChecksumErrorFrame |
                    ETH_InitStruct->ETH_FlushReceivedFrame |
                    ETH_InitStruct->ETH_TransmitStoreForward |
                    ETH_InitStruct->ETH_ForwardErrorFrames |
                    ETH_InitStruct->ETH_ForwardUndersizedGoodFrames);
    ETH->DMAOMR = (uint32_t)tmpreg;

    /* Reset the physical layer */
    ETH_WritePHYRegister(PHYAddress, PHY_BCR, PHY_Reset);
    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x00 );
    tmpreg = ETH_ReadPHYRegister(gPHYAddress, 24);
    if(tmpreg & (1<<1)) ETH_WritePHYRegister(PHYAddress, PHY_BCR, 0x3100);
    return ETH_SUCCESS;
}

/*********************************************************************
 * @fn      ETH_Configuration
 *
 * @brief   Ethernet configure.
 *
 * @return  none
 */
void ETH_Configuration( uint8_t *macAddr )
{
    ETH_InitTypeDef ETH_InitStructure;
    uint16_t timeout = 10000;
    uint16_t regval;

    /* Enable Ethernet MAC clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ETH_MAC | \
                          RCC_AHBPeriph_ETH_MAC_Tx | \
                          RCC_AHBPeriph_ETH_MAC_Rx, ENABLE);

    gPHYAddress = PHY_ADDRESS;

    /* Enable RMII GPIO */
    ETH_RMIIPinInit();

    /* Reset ETHERNET on AHB Bus */
    ETH_DeInit();

    /* Software reset */
    ETH_SoftwareReset();

    /* Wait for software reset */
    do{
        Delay_Us(10);
        if( !--timeout )  break;
    }while(ETH->DMABMR & ETH_DMABMR_SR);

    /* ETHERNET Configuration */
    /* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
    ETH_StructInit(&ETH_InitStructure);
    /* Fill ETH_InitStructure parameters */
    /*------------------------   MAC   -----------------------------------*/
    ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;
    ETH_InitStructure.ETH_Speed = ETH_Speed_100M;
    ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
    ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
    ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
    ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
    /* Filter function configuration */
    ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
    ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
    ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
    ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
    ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
    /*------------------------   DMA   -----------------------------------*/
    /* When we use the Checksum offload feature, we need to enable the Store and Forward mode:
    the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum,
    if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
    ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
    ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;
    ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Enable;
    ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Enable;
    /* Configure Ethernet */
    ETH_RegInit( &ETH_InitStructure, gPHYAddress );

    /* Configure MAC address */
    ETH->MACA0HR = (uint32_t)((macAddr[5]<<8) | macAddr[4]);
    ETH->MACA0LR = (uint32_t)(macAddr[0] | (macAddr[1]<<8) | (macAddr[2]<<16) | (macAddr[3]<<24));

    /* Mask the interrupt that Tx good frame count counter reaches half the maximum value */
    ETH->MMCTIMR = ETH_MMCTIMR_TGFM;
    /* Mask the interrupt that Rx good unicast frames counter reaches half the maximum value */
    /* Mask the interrupt that Rx crc error counter reaches half the maximum value */
    ETH->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFCEM;

    ETH_DMAITConfig(ETH_DMA_IT_NIS |\
                ETH_DMA_IT_R |\
                ETH_DMA_IT_T |\
                ETH_DMA_IT_AIS |\
                ETH_DMA_IT_RBU,\
                ENABLE);

    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x00 );
    /* Configure Repeater mode */
    regval = ETH_ReadPHYRegister(gPHYAddress, 28);
    regval |= 1<<13;
    ETH_WritePHYRegister(gPHYAddress, 28, regval );

    /*rmii rx clock change */
    ETH_WritePHYRegister(gPHYAddress, 0x1F, 0x07 );
    regval = ETH_ReadPHYRegister( gPHYAddress, 16);
    regval &= ~(0x0f<<4);
    regval |= 0x04<<4;
    ETH_WritePHYRegister(gPHYAddress, 16, regval );

#if LINK_STAT_ACQUISITION_METHOD
    /* Configure the PHY interrupt function, the supported chip is: CH182H RMII */
    PHY_InterruptInit( );
    /* Configure EXTI Line7. */
    EXTI_Line_Init( );
#else
    /*Reads the default value of the PHY_BSR register*/
    LastPhyStat = ETH_ReadPHYRegister( PHY_ADDRESS, PHY_BSR );
#endif
}

/*********************************************************************
 * @fn      MACRAW_Tx
 *
 * @brief   Ethernet sends data frames in chain mode.
 *
 * @param   buff   send buffer pointer
 *          len    Send data length
 *
 * @return  Send status.
 */
uint32_t MACRAW_Tx(uint8_t *buff, uint16_t len)
{
    /* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
    if((DMATxDescToSet->Status & ETH_DMATxDesc_OWN) != (u32)RESET)
    {
        /* Return ERROR: OWN bit set */
        return ETH_ERROR;
    }
    /* Setting the Frame Length: bits[12:0] */
    DMATxDescToSet->ControlBufferSize = (len & ETH_DMATxDesc_TBS1);

    memcpy((uint8_t *)DMATxDescToSet->Buffer1Addr, buff, len);

    /* Setting the last segment and first segment bits (in this case a frame is transmitted in one descriptor) */
    DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS;

    /* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
    DMATxDescToSet->Status |= ETH_DMATxDesc_OWN;

    /* Clear TBUS ETHERNET DMA flag */
    ETH->DMASR = ETH_DMASR_TBUS;
    /* Resume DMA transmission*/
    ETH->DMATPDR = 0;

    /* Update the ETHERNET DMA global Tx descriptor with next Tx descriptor */
    /* Chained Mode */
    /* Selects the next DMA Tx descriptor list for next buffer to send */
    DMATxDescToSet = (ETH_DMADESCTypeDef*) (DMATxDescToSet->Buffer2NextDescAddr);
    /* Return SUCCESS */
    return ETH_SUCCESS;
}

/*********************************************************************
 * @fn      ETH_Stop
 *
 * @brief   Disables ENET MAC and DMA reception/transmission.
 *
 * @return  none
 */
void ETH_Stop(void)
{
    ETH_MACTransmissionCmd(DISABLE);
    ETH_FlushTransmitFIFO();
    ETH_MACReceptionCmd(DISABLE);
    ETH_DMATransmissionCmd(DISABLE);
    ETH_DMAReceptionCmd(DISABLE);
}

/*********************************************************************
 * @fn      ReInitMACReg
 *
 * @brief   Reinitialize MAC register.
 *
 * @param   none.
 *
 * @return  none.
 */
void ReInitMACReg(void)
{
    ETH_InitTypeDef ETH_InitStructure;
    uint16_t timeout = 10000;
    uint32_t tmpreg = 0, maccrval = 0;;

    /* Wait for sending data to complete */
    while((ETH->DMASR & (7 << 20)) != ETH_DMA_TransmitProcess_Suspended);

    ETH_Stop();

    /* Record the value of the MACCR */
    maccrval = ETH->MACCR;

    /* Reset ETHERNET on AHB Bus */
    ETH_DeInit();

    /* Software reset */
    ETH_SoftwareReset();

    /* Wait for software reset */
    do{
        Delay_Us(10);
        if( !--timeout )  break;
    }while(ETH->DMABMR & ETH_DMABMR_SR);

    /* ETHERNET Configuration */
    /* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
    ETH_StructInit(&ETH_InitStructure);
    /* Fill ETH_InitStructure parameters */
    /*------------------------   MAC   -----------------------------------*/
    ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;
    ETH_InitStructure.ETH_Speed = ETH_Speed_100M;
#if HARDWARE_CHECKSUM_CONFIG
    ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif
    ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
    ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
    ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
    ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
    /* Filter function configuration */
    ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
    ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
    ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
    ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
    ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
    /*------------------------   DMA   -----------------------------------*/
    /* When we use the Checksum offload feature, we need to enable the Store and Forward mode:
    the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum,
    if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
    ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
    ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;
    ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Enable;
    ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Enable;

    /*---------------------- Physical layer configuration -------------------*/
    /* Set the SMI interface clock, set as the main frequency divided by 42  */
    tmpreg = ETH->MACMIIAR;
    tmpreg &= MACMIIAR_CR_MASK;
    tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div42;
    ETH->MACMIIAR = (uint32_t)tmpreg;

    ETH->MACFFR = (uint32_t)(ETH_InitStructure.ETH_ReceiveAll |
                       ETH_InitStructure.ETH_SourceAddrFilter |
                       ETH_InitStructure.ETH_PassControlFrames |
                       ETH_InitStructure.ETH_BroadcastFramesReception |
                       ETH_InitStructure.ETH_DestinationAddrFilter |
                       ETH_InitStructure.ETH_PromiscuousMode |
                       ETH_InitStructure.ETH_MulticastFramesFilter |
                       ETH_InitStructure.ETH_UnicastFramesFilter);
    /*--------------- ETHERNET MACHTHR and MACHTLR Configuration ---------------*/
    /* Write to ETHERNET MACHTHR */
    ETH->MACHTHR = (uint32_t)ETH_InitStructure.ETH_HashTableHigh;
    /* Write to ETHERNET MACHTLR */
    ETH->MACHTLR = (uint32_t)ETH_InitStructure.ETH_HashTableLow;
    /*----------------------- ETHERNET MACFCR Configuration --------------------*/
    /* Get the ETHERNET MACFCR value */
    tmpreg = ETH->MACFCR;
    /* Clear xx bits */
    tmpreg &= MACFCR_CLEAR_MASK;
    tmpreg |= (uint32_t)((ETH_InitStructure.ETH_PauseTime << 16) |
                  ETH_InitStructure.ETH_UnicastPauseFrameDetect |
                  ETH_InitStructure.ETH_ReceiveFlowControl |
                  ETH_InitStructure.ETH_TransmitFlowControl);
    ETH->MACFCR = (uint32_t)tmpreg;

    ETH->MACVLANTR = (uint32_t)(ETH_InitStructure.ETH_VLANTagComparison |
                            ETH_InitStructure.ETH_VLANTagIdentifier);

    tmpreg = ETH->DMAOMR;
    tmpreg &= DMAOMR_CLEAR_MASK;
    tmpreg |= (uint32_t)(ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame |
                 ETH_InitStructure.ETH_FlushReceivedFrame |
                 ETH_InitStructure.ETH_TransmitStoreForward |
                 ETH_InitStructure.ETH_ForwardErrorFrames |
                 ETH_InitStructure.ETH_ForwardUndersizedGoodFrames);
    ETH->DMAOMR = (uint32_t)tmpreg;
    ETH->MACCR = maccrval;

    /* Configure MAC address */
    ETH->MACA0HR = (uint32_t)((MACAddr[5]<<8) | MACAddr[4]);
    ETH->MACA0LR = (uint32_t)(MACAddr[0] | (MACAddr[1]<<8) | (MACAddr[2]<<16) | (MACAddr[3]<<24));


    /* Mask the interrupt that Tx good frame count counter reaches half the maximum value */
    ETH->MMCTIMR = ETH_MMCTIMR_TGFM;
    /* Mask the interrupt that Rx good unicast frames counter reaches half the maximum value */
    /* Mask the interrupt that Rx crc error counter reaches half the maximum value */
    ETH->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFCEM;

    ETH_DMAITConfig(ETH_DMA_IT_NIS |\
                ETH_DMA_IT_R |\
                ETH_DMA_IT_T |\
                ETH_DMA_IT_AIS |\
                ETH_DMA_IT_RBU,\
                ENABLE);

    ETH_DMATxDescChainInit(DMATxDscrTab, MACTxBuf, ETH_TXBUFNB);
    ETH_DMARxDescChainInit(DMARxDscrTab, MACRxBuf, ETH_RXBUFNB);
    pDMARxSet = DMARxDscrTab;
    pDMATxSet = DMATxDscrTab;

    ETH_Start( );
}

/*********************************************************************
 * @fn      WCHNET_RecProcess
 *
 * @brief   Receiving related processing
 *
 * @param   none.
 *
 * @return  none.
 */
void WCHNET_RecProcess(void)
{
    if(((ChipId & 0xf0) <= 0x20) && \
            ((ETH->DMAMFBOCR & 0x1FFE0000) != 0))
    {
        ReInitMACReg();
        /* Resume DMA transport */
        ETH->DMARPDR = 0;
        ETH->DMATPDR = 0;
    }
}

/*********************************************************************
 * @fn      WCHNET_ETHIsr
 *
 * @brief   Ethernet Interrupt Service Routine
 *
 * @return  none
 */
void WCHNET_ETHIsr(void)
{
    uint32_t int_sta;

    int_sta = ETH->DMASR;
    if (int_sta & ETH_DMA_IT_AIS)
    {
        if (int_sta & ETH_DMA_IT_RBU)
        {
            WCHNET_RecProcess();
            ETH_DMAClearITPendingBit(ETH_DMA_IT_RBU);
        }
        ETH_DMAClearITPendingBit(ETH_DMA_IT_AIS);
    }

    if( int_sta & ETH_DMA_IT_NIS )
    {
        if( int_sta & ETH_DMA_IT_R )
        {
            /*If you don't use the Ethernet library,
             * you can do some data processing operations here*/
            ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
        }
        if( int_sta & ETH_DMA_IT_T )
        {
            ETH_DMAClearITPendingBit(ETH_DMA_IT_T);
        }
        ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
    }
}

/*********************************************************************
 * @fn      ETH_Init
 *
 * @brief   Ethernet initialization.
 *
 * @return  none
 */
void ETH_Init( uint8_t *macAddr )
{
    ChipId = DBGMCU_GetCHIPID();
    ETH_Configuration( macAddr );
    ETH_DMATxDescChainInit(DMATxDscrTab, MACTxBuf, ETH_TXBUFNB);
    ETH_DMARxDescChainInit(DMARxDscrTab, MACRxBuf, ETH_RXBUFNB);
    pDMARxSet = DMARxDscrTab;
    pDMATxSet = DMATxDscrTab;
    NVIC_EnableIRQ(ETH_IRQn);
    NVIC_SetPriority(ETH_IRQn, 0);
}

/******************************** endfile @ eth_driver ******************************/
