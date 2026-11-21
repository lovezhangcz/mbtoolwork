/**
  ******************************************************************************
  * @file	motor.c
  * @author	
  * @version	V1.0.0
  * @date	2014-4-1
  * @brief	Direct Current Brushless Direct Current Motor driver
  ******************************************************************************
  * @attention
  *
  * 
  * 
  * 
  *
  * <h2><center>&copy; COPYRIGHT 2014  TECHNOLOGIES CO.,LTD.</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "ucos_ii.h"
#include "cpu.h"
#include "my_type.h"
#include "tasttimebase.h"
#include "led.h"

static void tastTimeConfig(u32 freq)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
    
	/* TIM2 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* Freq = TimXClock / TIM_Period */
	TIM_TimeBaseStructure.TIM_Period = SystemCoreClock / freq;
	
	TIM_TimeBaseStructure.TIM_Prescaler = 1;//3
	
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_UpdateDisableConfig(TIM3, DISABLE);		//Enable update event

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  

	/* TIM enable counter */
	TIM_Cmd(TIM3, ENABLE);            

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE); 
}  

/**
  * @}
  */
  
/**
  * @brief  Init motor, include HALL, phase current, PWM, MOS temperature, Bat volt
  * @note   
  * @retval None
  */
void tasttimebaseInit(void)
{

	tastTimeConfig(2000);//2k运动控制timer2启动
}

/**
  * @brief  Get MOS temperature
  * @note   
  * @retval ADC value
  */

//MotionControl和Systemsuperise的线程触发，2000Hz和200

extern OS_EVENT *g_ptSensorUpdateMBox;
extern OS_EVENT *g_ptSystemSuperviseMBox;
extern OS_EVENT *g_ptMotionUpdateBox;
extern OS_EVENT *g_ptbluetoothtastMBox;
extern OS_EVENT *g_ptspkworkingtastMBox;

void TIM3_IRQHandler(void)
{        
	static u16 SystemSuperviseDiv = 0;
	static u16 bluetoothtastdiv=0;	
	static u16 motiontastdiv=0;	
	
	if(TIM3->SR & TIM_IT_Update){
		OSIntEnter();

		TIM3->SR = (u16)~TIM_IT_Update;	//Clear IT bit

		
		if(g_ptSensorUpdateMBox){
			OSMboxPost(g_ptSensorUpdateMBox, (void*)1);
		}	
		
		

		SystemSuperviseDiv++;
		if(SystemSuperviseDiv >=20){//10
			SystemSuperviseDiv = 1;
			if(g_ptSystemSuperviseMBox){
				OSMboxPost(g_ptSystemSuperviseMBox, (void*)1);
			}
			if(g_ptspkworkingtastMBox){
				OSMboxPost(g_ptspkworkingtastMBox, (void*)1);
			}
			
		}
		
		motiontastdiv++;
		if(motiontastdiv>=10)
		{	motiontastdiv=1;
			if(g_ptMotionUpdateBox)
			{
				OSMboxPost(g_ptMotionUpdateBox, (void*)1);				
			}
		}

		bluetoothtastdiv++;
		if(bluetoothtastdiv>=50)
		{	bluetoothtastdiv=1;
			if(g_ptbluetoothtastMBox)
			{
				OSMboxPost(g_ptbluetoothtastMBox, (void*)1);				
			}
		}		
	
		OSIntExit();
	}
}



/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/********* (C) COPYRIGHT 2014  TECHNOLOGIES CO.,LTD. ***END OF FILE***/
