/*
 * usartset_u4.c — 治具 bootloader 升级串口（UART4, PC10=TX / PC11=RX）
 *
 * 以主板工程 usartset_u4.c 为底移植到 boot：
 *   1) 去掉 ucos_ii.h / main.h 依赖，定义 BOOTPRJFLG 关闭 OS 中断进出临界
 *   2) 顶部别名块：boot 主程序统一调用 *_bb 接口
 *   3) BufferTxTrig_u4 的 datalen 由 u8 改为 u16（boot 数据帧最长 20+432 字节）
 *   4) 新增 UsartClose_u4（跳转 APP 前关闭串口/DMA）
 * 仅用于 MODEM_KS_JIG / MODEM_KS_ZZDW 等 boot 目标。
 */
#include "stm32f10x.h"
#include "usartset.h"
#include "GD32_ADD.h"

#define BOOTPRJFLG 1

#define DSP_HalfDuplex_u4 0

#define BufferTxTrig_u4   BufferTxTrig_bb
#define USART_USER_IRQ_U4 USART_USER_IRQ_BBAPP
#define USART_Initial_u4  USART_Initial_bb
#define UsartClose_u4     UsartClose_bb

CPCALLBACKFUN pcallbackfun_u4=0;

u8 UsartDMARxTable_u4[DMARXPACKAGELENGTH_U4] = {0};
u8 UsartDMATxTable_u4[DMATXPACKAGELENGTH_U4] = {0};

volatile u8 ComSendingFlg_u4=0;

void MsgTxCallBack_u4(void);

void USART_Initial_u4(u32 baudrate)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    RCC->APB2ENR |=  RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO;	/* 开串口APB */
    RCC->APB1ENR |= RCC_APB1Periph_UART4;

    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 ;	/* UART4 TX */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11 ;	/* UART4 RX */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

#ifdef USED_IDLE_U4
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    USART_ITConfig(UART4, USART_IT_ORE, ENABLE);
    USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);
#endif
    USART_DMACmd(UART4, USART_DMAReq_Tx, ENABLE);
    USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);

    /*DMA Config*/
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

    /* UART4 TX DMA config (DMA2_CH5) */
    DMA_DeInit(DMA2_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(UART4->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)UsartDMATxTable_u4;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = DMATXPACKAGELENGTH_U4;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel5, &DMA_InitStructure);

    DMA_ITConfig(DMA2_Channel5, DMA_IT_TC, ENABLE);	/* Complete Interrupt */
    DMA_Cmd(DMA2_Channel5, DISABLE);

    /* UART4 RX DMA config (DMA2_CH3) */
    DMA_DeInit(DMA2_Channel3);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(UART4->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)UsartDMARxTable_u4;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = (DMARXPACKAGELENGTH_U4);
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel3, &DMA_InitStructure);

    DMA_Cmd(DMA2_Channel3, ENABLE);

    /* NVIC Config */
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel4_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Init_GD(UART4, &USART_InitStructure);

    USART_Cmd(UART4, ENABLE);
}

/* UART4 TX DMA 完成中断 */
void DMA2_Channel4_5_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA2_FLAG_TC5) != RESET)
    {
        DMA_ClearITPendingBit(DMA2_FLAG_TC5);
        DMA_Cmd(DMA2_Channel5, DISABLE);
        MsgTxCallBack_u4();
    }
}

/* UART4 RX DMA 中断（备用，IDLE 模式下不使能） */
void DMA2_Channel3_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA2_FLAG_HT3) != RESET)
    {
        DMA_ClearITPendingBit(DMA2_FLAG_HT3);
    }
    else if(DMA_GetITStatus(DMA2_FLAG_TC3) != RESET)
    {
        DMA_ClearITPendingBit(DMA2_FLAG_TC3);
    }
}

extern u8 USART_USER_IRQ_U4(u8 *dbuf,u16 data_len);
void UART4_IRQHandler(void)
{
    u8 data;

    if(USART_GetITStatus(UART4, USART_IT_IDLE) != RESET)
    {
        u32 data_len = DMARXPACKAGELENGTH_U4 - DMA_GetCurrDataCounter(DMA2_Channel3);

        DMA_Cmd(DMA2_Channel3,DISABLE);
        DMA2_Channel3->CNDTR = DMARXPACKAGELENGTH_U4;
        DMA_Cmd(DMA2_Channel3,ENABLE);

        USART_ClearITPendingBit(UART4,USART_IT_IDLE);

        USART_USER_IRQ_U4(UsartDMARxTable_u4,data_len);

        data = UART4->SR;
        data = UART4->DR;
        data = data;
    }

    if(USART_GetITStatus(UART4, USART_IT_TXE) != RESET)			/* 发送中断 */
    {
        USART_ClearITPendingBit(UART4, USART_IT_TXE);
        USART_ITConfig(UART4, USART_IT_TXE, DISABLE);
    }

    if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(UART4,USART_IT_RXNE);
    }

    if(USART_GetITStatus(UART4, USART_IT_NE) != RESET)
    {
        USART_ClearITPendingBit(UART4,USART_IT_NE);
    }
    if(USART_GetITStatus(UART4, USART_IT_FE) != RESET)
    {
        USART_ClearITPendingBit(UART4,USART_IT_FE);
    }
}

u8 BufferTxTrig_u4(u8 *data,u16 datalen)
{
    u16 i=0;
    if(!ComSendingFlg_u4)
    {
        ComSendingFlg_u4=1;
        if(datalen>DMATXPACKAGELENGTH_U4)datalen=DMATXPACKAGELENGTH_U4;
        for(;datalen>0;datalen--)
        {
            UsartDMATxTable_u4[i]=*(data+i);
            i++;
        }
        DMA_ClearITPendingBit(DMA2_FLAG_TC5);
        DMA_Cmd(DMA2_Channel5, DISABLE);
        DMA_SetCurrDataCounter(DMA2_Channel5, i);
        DMA_Cmd(DMA2_Channel5, ENABLE);
        return 1;
    }else
    {
        return 0;
    }
}

void MsgTxCallBack_u4(void)
{
    u8 sendlen;
    if(pcallbackfun_u4>0)
    {
        sendlen=pcallbackfun_u4(UsartDMATxTable_u4);
        if(sendlen>0)
        {
            DMA_SetCurrDataCounter(DMA2_Channel5,sendlen);
            DMA_Cmd(DMA2_Channel5, ENABLE);
        }
    }else
    {
        ComSendingFlg_u4=0;
    }
}

/* 跳转 APP 前关闭串口与 DMA，避免残留中断 */
void UsartClose_u4(void)
{
    USART_Cmd(UART4, DISABLE);
    DMA_Cmd(DMA2_Channel3, DISABLE);
    DMA_Cmd(DMA2_Channel5, DISABLE);
    USART_DMACmd(UART4, USART_DMAReq_Rx, DISABLE);
    USART_DMACmd(UART4, USART_DMAReq_Tx, DISABLE);
    USART_ClearITPendingBit(UART4, USART_IT_IDLE);
    USART_ClearFlag(UART4,USART_FLAG_IDLE);
    USART_ITConfig(UART4, USART_IT_IDLE, DISABLE);
}
