#include "stm32f10x.h"
//#include "ucos_ii.h"
//#include "main.h"
#include "usartset.h" 
#include "bbcpro.h"
#include "GD32_ADD.h"

#define BOOTPRJFLG 1

//#if DSPLAYBOARDTYPE==DIS_BOARD_TYPE_NONE

//#else
	#define BufferTxTrig_u1 BufferTxTrig_bb
	#define USART_USER_IRQ_U1 USART_USER_IRQ_BBIN
	#define USART_Initial_u1 USART_Initial_bb
	#define UsartClose_u1 UsartClose_bb
//#endif

#if defined(MODEM_KS_V3)||defined(MODEM_KS_V1)
	#define DSP_HalfDuplex_u1 0
#else
	#define DSP_HalfDuplex_u1 1
#endif

CPCALLBACKFUN pcallbackfun_u1=0;

u8 UsartDMARxTable_u1[DMARXPACKAGELENGTH_U1] = {0};
u8 UsartDMATxTable_u1[DMATXPACKAGELENGTH_U1] = {0};

volatile u8 ComSendingFlg_u1=0;

void MsgTxCallBack_u1(void);

void USART_Initial_u1(u32 baudrate)
{
    
    
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    
    u16 GPIO_Pin_Tx;
    u16 GPIO_Pin_Rx;
    GPIO_TypeDef *GPIOx_Rx, *GPIOx_Tx;
    USART_TypeDef *UsartBase;
    volatile u8 USARTx_IRQChannel;

        RCC->APB2ENR |= RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO;	
				GPIO_Pin_Tx = GPIO_Pin_6;
        GPIO_Pin_Rx = GPIO_Pin_7;
        GPIOx_Tx = GPIOB;
        GPIOx_Rx = GPIOB;
        UsartBase = USART1;
        USARTx_IRQChannel = USART1_IRQn;
        GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);


    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_Tx ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//GPIO_Mode_AF_OD;//GPIO_Mode_AF_PP;//GPIO_Mode_AF_OD;//
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOx_Tx, &GPIO_InitStructure);

#if DSP_HalfDuplex_u1==0
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_Rx ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOx_Rx, &GPIO_InitStructure);
#endif
    

    
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

#ifdef USED_IDLE_U1
     NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQChannel;
     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;//USART_IRQ_PRIORITY;//1;//5
     NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
     NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
     NVIC_Init(&NVIC_InitStructure);
	 USART_ITConfig(UsartBase, USART_IT_ORE, ENABLE);

	USART_ITConfig(UsartBase, USART_IT_IDLE, ENABLE);
#endif	
	USART_DMACmd(UsartBase, USART_DMAReq_Tx, ENABLE);
	USART_DMACmd(UsartBase, USART_DMAReq_Rx, ENABLE);
// 	USART_Cmd(USART1, ENABLE);
	
	/*DMA Config*/
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	/* USART1 TX DMA config */
	DMA_DeInit(DMA1_Channel4);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(UsartBase->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)UsartDMATxTable_u1;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = DMATXPACKAGELENGTH_U1;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);
	
	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);	//Complete Interrupt
	DMA_Cmd(DMA1_Channel4, DISABLE);
	
	/* USART1 RX DMA config */
	DMA_DeInit(DMA1_Channel5);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(UsartBase->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)UsartDMARxTable_u1;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = (DMARXPACKAGELENGTH_U1);
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);

#ifndef USED_IDLE_U1	
	//DMA_ITConfig(DMA1_Channel3, DMA_IT_HT | DMA_IT_TC, ENABLE);	//Complete Interrupt
    DMA_ITConfig(DMA1_Channel5,DMA_IT_TC, ENABLE);	//Complete Interrupt
#endif
	DMA_Cmd(DMA1_Channel5, ENABLE);
	
	/* NVIC Config */
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;//RT_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
#ifndef USED_IDLE_U1	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

    //USART_Init(UsartBase, &USART_InitStructure);
		USART_Init_GD(UsartBase, &USART_InitStructure);
    //USART_ITConfig(UsartBase, USART_IT_ORE, ENABLE);

#if DSP_HalfDuplex_u1==1
		USART_HalfDuplexCmd(UsartBase,ENABLE);
#endif

    USART_Cmd(UsartBase, ENABLE);


}



//USART1 TX IRQ
void DMA1_Channel4_IRQHandler(void)
{
#ifndef BOOTPRJFLG	
	OS_CPU_SR cpu_sr;	
	OS_ENTER_CRITICAL(); 	//通知内核进入中断
	OSIntNesting++;
	OS_EXIT_CRITICAL();
#endif		
	if(DMA_GetITStatus(DMA1_FLAG_TC4) != RESET)
	{
		DMA_ClearITPendingBit(DMA1_FLAG_TC4); 
		DMA_Cmd(DMA1_Channel4, DISABLE);
		MsgTxCallBack_u1();
	}
#ifndef BOOTPRJFLG	
	OSIntExit();         //通知内核离开中断	
#endif	
}

//USART1 RX IRQ
void DMA1_Channel5_IRQHandler(void)
{
#ifndef BOOTPRJFLG	
	OS_CPU_SR cpu_sr;
#endif
	u8 i;
#ifndef BOOTPRJFLG	
	OS_ENTER_CRITICAL(); 	//通知内核进入中断
	OSIntNesting++;
	OS_EXIT_CRITICAL();
#endif

	if(DMA_GetITStatus(DMA1_FLAG_HT5) != RESET)
	{
		DMA_ClearITPendingBit(DMA1_FLAG_HT5); 
	}
	else if(DMA_GetITStatus(DMA1_FLAG_TC5) != RESET)
	{
		DMA_ClearITPendingBit(DMA1_FLAG_TC5);  
	}
#ifndef BOOTPRJFLG	
	OSIntExit();         //通知内核离开中断
#endif
}

extern void USART_USER_IRQ_U1(u8 *dbuf,u16 data_len);
void USART1_IRQHandler(void)
{
#ifndef BOOTPRJFLG	
	OS_CPU_SR cpu_sr;
#endif	
	u8 data;
#ifndef BOOTPRJFLG	
	OS_ENTER_CRITICAL(); 	//通知内核进入中断
	OSIntNesting++;
	OS_EXIT_CRITICAL();
#endif

	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
			u16 i;
			u32 data_len = DMARXPACKAGELENGTH_U1 - DMA_GetCurrDataCounter(DMA1_Channel5);

			DMA_Cmd(DMA1_Channel5,DISABLE);
	    DMA1_Channel5->CNDTR = DMARXPACKAGELENGTH_U1;
			DMA_Cmd(DMA1_Channel5,ENABLE);
		
			USART_ClearITPendingBit(USART1,USART_IT_IDLE);
		
	if(data_len>100)
	{
		__nop();
	}
	#if DSP_HalfDuplex_u1==0
				USART_USER_IRQ_U1(UsartDMARxTable_u1,data_len);
  #else
				if(ComSendingFlg_u1)
				{
					ComSendingFlg_u1--;
				}
//				else
//				{
//					USART_USER_IRQ_U1(UsartDMARxTable_u1,data_len);
//				}
			for(i=0;i<12;i++)
			//for(i=0;i<32;i++)	
			{
				if(UsartDMARxTable_u1[i]^UsartDMATxTable_u1[i])
				{
					USART_USER_IRQ_U1(UsartDMARxTable_u1,data_len);
					break;
				}
			}				
	#endif			
			
        data = USART1->SR;
        data = USART1->DR;
				data = data;
	}
	
	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)			//发送中断
	{         				
		USART_ClearITPendingBit(USART1, USART_IT_TXE);				// Clear the USART1 transmit interrupt 
		USART_ITConfig(USART1, USART_IT_TXE, DISABLE);		  //Disable the USART1 Transmit interrupt
	}	

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) 
	{	//RX
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);

	}
	if(USART_GetITStatus(USART1, USART_IT_NE) != RESET) 
	{	//RX
		USART_ClearITPendingBit(USART1,USART_IT_NE);

	}        
	if(USART_GetITStatus(USART1, USART_IT_FE) != RESET) 
	{	//RX
		USART_ClearITPendingBit(USART1,USART_IT_FE);

	}         

	
	if(USART1->SR & USART_FLAG_ORE)
	{
		//clear the ore flag
// 		cpu_sr = USART3->SR;
// 		cpu_sr = USART3->DR;
	}
#ifndef BOOTPRJFLG
	OSIntExit();
#endif
}



//void MsgTxTrig_u1(void)
//{
//	if(!ComSendingFlg_u1)
//	{		ComSendingFlg_u1=1;
//		    DMA_SetCurrDataCounter(DMA1_Channel4, 1);       
//        DMA_Cmd(DMA1_Channel4, ENABLE);
//	}
//}

u8 BufferTxTrig_u1(u8 *data,u16 datalen)
{	
	u16 i=0;
	if(!ComSendingFlg_u1)
	{	
			//USART1->CR1&=~(USART_Mode_Rx);
		
#if DSP_HalfDuplex_u1==0						
			ComSendingFlg_u1=1;
#else
			ComSendingFlg_u1=1;
#endif		

			if(datalen>DMATXPACKAGELENGTH_U1)datalen=DMATXPACKAGELENGTH_U1;
			for(;datalen>0;datalen--)
		{
			UsartDMATxTable_u1[i]=*(data+i);
			i++;
		}
				DMA_ClearITPendingBit(DMA1_FLAG_TC4); 
				DMA_Cmd(DMA1_Channel4, DISABLE);		    
				DMA_SetCurrDataCounter(DMA1_Channel4, i);       
        DMA_Cmd(DMA1_Channel4, ENABLE);
		return 1;
	}else
	{
		return 0;
	}
}

void MsgTxCallBack_u1(void)
{		
	#if DSP_HalfDuplex_u1==0					
		u8 sendlen;
		if(pcallbackfun_u1>0)
		{
			sendlen=pcallbackfun_u1(UsartDMATxTable_u1);
			if(sendlen>0)
			{
				DMA_SetCurrDataCounter(DMA1_Channel4, sendlen);       
				DMA_Cmd(DMA1_Channel4, ENABLE);						
			}
		}else
		{
			ComSendingFlg_u1=0;
		}
	#else
			if(ComSendingFlg_u1)ComSendingFlg_u1--;
	#endif			
			
}

void UsartClose_u1(void)
{
	USART_Cmd(USART1, DISABLE);
	DMA_Cmd(DMA1_Channel5, DISABLE);        
	DMA_Cmd(DMA1_Channel4, DISABLE);        
  USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);              
  USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);              
	USART_ClearITPendingBit(USART1, USART_IT_IDLE);      
	USART_ClearFlag(USART1,USART_FLAG_IDLE);
	USART_ITConfig(USART1, USART_IT_IDLE, DISABLE);
}

