#include "stm32f10x.h"
//#include "ucos_ii.h"
//#include "main.h"
#include "usartset.h"

#define BOOTPRJFLG 1

#if defined(MODEM_KS_M12)||defined(MODEM_KS_MTB1)||defined(MODEM_KS_MTB7)\
		||defined(MODEM_KS_MTB1R)||defined(MODEM_KS_FX1)
		
 #define DSP_HalfDuplex_u3 0 //DSP_HalfDuplex_u3_cfg
 
	#define BufferTxTrig_u3 BufferTxTrig_bb 
	#define USART_USER_IRQ_U3 USART_USER_IRQ_BBAPP
	#define USART_Initial_u3 USART_Initial_bb
	#define UsartClose_u3 UsartClose_bb
	
#else

 #define DSP_HalfDuplex_u3 1 //DSP_HalfDuplex_u3_cfg
 
	#define BufferTxTrig_u3 BufferTxTrig_bb 
	#define USART_USER_IRQ_U3 USART_USER_IRQ_BBIN
	#define USART_Initial_u3 USART_Initial_bb
	#define UsartClose_u3 UsartClose_bb
	
#endif
u8 UsartDMARxTable_u3[DMARXPACKAGELENGTH_U3] = {0};
u8 UsartDMATxTable_u3[DMATXPACKAGELENGTH_U3] = {0};

 volatile u8 ComSendingFlg_u3=0;

u16 HeadCode_send_u3=0;
u16 HeadCode_rev_u3=0;

void MsgTxCallBack_u3(void);

void USART_Initial_u3(u32 baudrate)
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


				RCC->APB2ENR |=  RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO;	//开串口APB
        RCC->APB1ENR |= RCC_APB1Periph_USART3;
				GPIO_Pin_Tx = GPIO_Pin_10;
        GPIO_Pin_Rx = GPIO_Pin_11;
        GPIOx_Tx = GPIOB;
        GPIOx_Rx = GPIOB;
        UsartBase = USART3;
        USARTx_IRQChannel = USART3_IRQn;
        //GPIO_PinRemapConfig(GPIO_PartialRemap_USART3, ENABLE);

#if DSP_HalfDuplex_u3==0
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_Tx ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//GPIO_Mode_AF_OD;//
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOx_Tx, &GPIO_InitStructure);		
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_Rx ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOx_Rx, &GPIO_InitStructure);
#else
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_Tx ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//GPIO_Mode_AF_OD;//
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOx_Tx, &GPIO_InitStructure);	
#endif
    

    
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

#ifdef USED_IDLE_U3
     NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQChannel;
     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
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
	DMA_DeInit(DMA1_Channel2);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(UsartBase->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)UsartDMATxTable_u3;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = DMATXPACKAGELENGTH_U3;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);
	
	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);	//Complete Interrupt
	DMA_Cmd(DMA1_Channel2, DISABLE);
	
	/* USART1 RX DMA config */
	DMA_DeInit(DMA1_Channel3);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(UsartBase->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)UsartDMARxTable_u3;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = (DMARXPACKAGELENGTH_U3);
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);

#ifndef USED_IDLE_U3	
	//DMA_ITConfig(DMA1_Channel3, DMA_IT_HT | DMA_IT_TC, ENABLE);	//Complete Interrupt
    DMA_ITConfig(DMA1_Channel3,DMA_IT_TC, ENABLE);	//Complete Interrupt
#endif
	DMA_Cmd(DMA1_Channel3, ENABLE);
	
	/* NVIC Config */
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
#ifndef USED_IDLE_U3	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif

    USART_Init(UsartBase, &USART_InitStructure);
    //USART_ITConfig(UsartBase, USART_IT_ORE, ENABLE);
		
#if DSP_HalfDuplex_u3>0		
		USART_HalfDuplexCmd(UsartBase,ENABLE);		
#endif		

    USART_Cmd(UsartBase, ENABLE);


}



//USART1 TX IRQ
void DMA1_Channel2_IRQHandler(void)
{
#ifndef BOOTPRJFLG	
	OS_CPU_SR cpu_sr;	
	OS_ENTER_CRITICAL(); 	//通知内核进入中断
	OSIntNesting++;
	OS_EXIT_CRITICAL();
#endif	
	if(DMA_GetITStatus(DMA1_FLAG_TC2) != RESET)
	{		
		DMA_ClearITPendingBit(DMA1_FLAG_TC2); 
		DMA_Cmd(DMA1_Channel2, DISABLE);
		MsgTxCallBack_u3();
	}	
#ifndef BOOTPRJFLG	
	OSIntExit();         //通知内核离开中断	
#endif	
}


//USART1 RX IRQ
void DMA1_Channel3_IRQHandler(void)
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

	if(DMA_GetITStatus(DMA1_FLAG_HT3) != RESET)
	{
		DMA_ClearITPendingBit(DMA1_FLAG_HT3); 
	}
	else if(DMA_GetITStatus(DMA1_FLAG_TC3) != RESET)
	{
		DMA_ClearITPendingBit(DMA1_FLAG_TC3);  
	}
#ifndef BOOTPRJFLG	
	OSIntExit();         //通知内核离开中断
#endif
}




extern u8 USART_USER_IRQ_U3(u8 *pdata,u16 data_len);
void USART3_IRQHandler(void)
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


	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
	{
			u16 i;
			u32 data_len = DMARXPACKAGELENGTH_U3 - DMA_GetCurrDataCounter(DMA1_Channel3);

			DMA_Cmd(DMA1_Channel3,DISABLE);
	    DMA1_Channel3->CNDTR = DMARXPACKAGELENGTH_U3;
			DMA_Cmd(DMA1_Channel3,ENABLE);		
			
			USART_ClearITPendingBit(USART3,USART_IT_IDLE);
		
	#if DSP_HalfDuplex_u3==0
				USART_USER_IRQ_U3(UsartDMARxTable_u3,data_len);
	#elif DSP_HalfDuplex_u3==1
				if(ComSendingFlg_u3)
				{
					ComSendingFlg_u3--;
				}
				for(i=0;i<12;i++)
				{
					if(UsartDMARxTable_u3[i]^UsartDMATxTable_u3[i])
					{
						USART_USER_IRQ_U3(UsartDMARxTable_u3,data_len);
						break;
					}
				}		
  #else
				if(ComSendingFlg_u3)
				{
					ComSendingFlg_u3--;
				}else
				{
					u16 getheadcode=UsartDMARxTable_u3[0]|(UsartDMARxTable_u3[1]<<8);
					if(getheadcode!=HeadCode_send_u3)
					{
						HeadCode_rev_u3=getheadcode;
						USART_USER_IRQ_U3(&UsartDMARxTable_u3[2],data_len);
					}
				}
	#endif
			
        data = USART3->SR;
        data = USART3->DR;
				data = data;
	}
	
	if(USART_GetITStatus(USART3, USART_IT_TXE) != RESET)			//发送中断
	{         				
		USART_ClearITPendingBit(USART3, USART_IT_TXE);				// Clear the USART1 transmit interrupt 
		USART_ITConfig(USART3, USART_IT_TXE, DISABLE);		  //Disable the USART1 Transmit interrupt
	}	

	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) 
	{	//RX
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);

	}
        
	if(USART_GetITStatus(USART3, USART_IT_NE) != RESET) 
	{	//RX
		USART_ClearITPendingBit(USART3,USART_IT_NE);

	}        
	if(USART_GetITStatus(USART3, USART_IT_FE) != RESET) 
	{	//RX
		USART_ClearITPendingBit(USART3,USART_IT_FE);

	} 
	
	if(USART3->SR & USART_FLAG_ORE)
	{
		//clear the ore flag
// 		cpu_sr = USART3->SR;
// 		cpu_sr = USART3->DR;
	}
#ifndef BOOTPRJFLG
	OSIntExit();
#endif
}



//void MsgTxTrig_u3(void)
//{
//	if(!ComSendingFlg_u3)
//	{		ComSendingFlg_u3=1;
//		    DMA_SetCurrDataCounter(DMA1_Channel2, 1);       
//        DMA_Cmd(DMA1_Channel2, ENABLE);
//	}
//}

u8 BufferTxTrig_u3(u8 *data,u16 datalen)
{	
	static u16 sendcnt;
	u16 i=0;
	if(!ComSendingFlg_u3)
	{
		sendcnt++;		
#if DSP_HalfDuplex_u3==0						
			ComSendingFlg_u3=1;
		
		if(datalen>(DMATXPACKAGELENGTH_U3))datalen=(DMATXPACKAGELENGTH_U3);
		for(;datalen>0;datalen--)
		{
			UsartDMATxTable_u3[i]=*(data+i);
			i++;
		}	
#elif DSP_HalfDuplex_u3==1
		ComSendingFlg_u3=1;
		if(datalen>(DMATXPACKAGELENGTH_U3))datalen=(DMATXPACKAGELENGTH_U3);
		for(;datalen>0;datalen--)
		{
			UsartDMATxTable_u3[i]=*(data+i);
			i++;
		}		
#else
		GPIOB->CRH&=(~(1<<10));
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();		
		
			ComSendingFlg_u3=1;
		
		HeadCode_send_u3=sendcnt+HeadCode_rev_u3+1;
		UsartDMATxTable_u3[0]=HeadCode_send_u3;
		UsartDMATxTable_u3[1]=HeadCode_send_u3>>8;
		
		if(datalen>(DMATXPACKAGELENGTH_U3-2))datalen=(DMATXPACKAGELENGTH_U3-2);
		for(;datalen>0;datalen--)
		{
			UsartDMATxTable_u3[i+2]=*(data+i);
			i++;
		}		i+=2;			
#endif
				DMA_ClearITPendingBit(DMA1_FLAG_TC2); 
				DMA_Cmd(DMA1_Channel2, DISABLE);		    
				DMA_SetCurrDataCounter(DMA1_Channel2, i);       
        DMA_Cmd(DMA1_Channel2, ENABLE);
		return 1;
	}else
	{
		return 0;
	}
}

void MsgTxCallBack_u3(void)
{		
	#if DSP_HalfDuplex_u3==0					
		u8 sendlen;
//		if(pcallbackfun_u3>0)
//		{
//			sendlen=pcallbackfun_u3(UsartDMATxTable_u3);
//			if(sendlen>0)
//			{
//        DMA_SetCurrDataCounter(DMA1_Channel2,sendlen);       
//        DMA_Cmd(DMA1_Channel2, ENABLE);					
//			}
//		}else
		{
			ComSendingFlg_u3=0;
		}
	#elif DSP_HalfDuplex_u3==1
			if(ComSendingFlg_u3)ComSendingFlg_u3--;		
	#else
					GPIOB->CRH|=(1<<10);
			if(ComSendingFlg_u3)ComSendingFlg_u3--;
	#endif		

}
void UsartClose_u3(void)
{
	USART_Cmd(USART3, DISABLE);
	DMA_Cmd(DMA1_Channel3, DISABLE);        
	DMA_Cmd(DMA1_Channel2, DISABLE);        
  USART_DMACmd(USART3, USART_DMAReq_Rx, DISABLE);              
  USART_DMACmd(USART3, USART_DMAReq_Tx, DISABLE);              
	USART_ClearITPendingBit(USART3, USART_IT_IDLE);      
	USART_ClearFlag(USART3,USART_FLAG_IDLE);
	USART_ITConfig(USART3, USART_IT_IDLE, DISABLE);
}