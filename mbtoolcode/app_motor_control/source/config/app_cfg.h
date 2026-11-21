/**
  ******************************************************************************
  * @file	app_cfg.h
  * @author	
  * @version	V1.0
  * @date	2014-2-17
  * @brief	
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
  
/** @addtogroup SingleWheel_Project
  * @{
  */ 
  
/** @addtogroup APPs
  * @{
  */ 
  
/** @addtogroup APP_Configures
  * @brief Configures of the app
  * @{
  */ 

#ifndef __APP_CFG_H
#define __APP_CFG_H

#ifdef __cplusplus
 extern "C" {
#endif 

/** @addtogroup APP_Configure_Includes
  * @{
  */
#include "stdio.h"
/**
  * @}
  */


/** @addtogroup APP_Configure_Exported_types
  * @{
  */

/**
  * @}
  */

/** @addtogroup APP_Configure_Exported_Constants
  * @{
  */

/**
  * @}
  */

/** @addtogroup APP_Configure_Exported_Macros
  * @{
  */

// *** <<< Use Configuration Wizard in Context Menu >>> *** 
// <h>App priority
// <o>APP_START_TASK_PRIO <1-63>
// <i>Priority of task APP_START 
#define  APP_START_TASK_PRIO		2

// <o>SENSOR_SAMPLE_TASK_PRIO <1-63>
// <i>Priority of task SENSOR_SAMPLE 
#define  SENSOR_SAMPLE_TASK_PRIO	3

// <o>MOTION_PROCESS_TASK_PRIO <1-63>
// <i>Priority of task BTDATA_PROCESS
#define  MOTION_PROCESS_TASK_PRIO	9

// <o>SYSTEM_SUPERISE_TASK_PRIO <1-63>
// <i>Priority of task SYSTEM_SUPERISE
#define SYSTEM_SUPERISE_TASK_PRIO   7

// <o>USART_PROCESS_TASK_PRIO <1-63>
// <i>Priority of task CAN_PROCESS
#define  USART_PROCESS_TASK_PRIO	6

// <o>BTDATA_PROCESS_TASK_PRIO <1-63>
// <i>Priority of task BTDATA_PROCESS
#define  BTDATA_PROCESS_TASK_PRIO	8

// <o>SPKDATA_PROCESS_TASK_PRIO <1-63>
// <i>Priority of task SPKDATA_PROCESS
#define  SPKFUN_PROCESS_TASK_PRIO	10

// </h>

// <h> App stack size
// <o>APP_TASKSTART_STK_SIZE <0-1024>
// <i>Stack size of APP_START 
#define APP_TASKSTART_STK_SIZE		64

// <o>APP_SENSOR_SAMPLE_STK_SIZE <0-1024>
// <i>Stack size of SENSOR_SAMPLE 
#define APP_SENSOR_SAMPLE_STK_SIZE	128

// <o>APP_MOTION_PROCESS_STK_SIZE <0-1024>
// <i>Stack size of BTDATA_PROCESS
#define APP_MOTION_PROCESS_STK_SIZE	128

// <o>APP_SYSTEMSUPERISE_STK_SIZE <0-1024>
// <i>Stack size of APP_SYSTEMSUPERISE 
#define APP_SYSTEMSUPERISE_STK_SIZE 128

// <o>APP_USART_PROCESS_STK_SIZE <0-1024>
// <i>Stack size of USART_PROCESS
#define APP_USART_PROCESS_STK_SIZE	128

// <o>APP_BTDATA_PROCESS_STK_SIZE <0-1024>
// <i>Stack size of BTDATA_PROCESS
#define APP_BTDATA_PROCESS_STK_SIZE	128

// <o>APP_SPKDATA_PROCESS_STK_SIZE <0-1024>
// <i>Stack size of SPKFUN_PROCESS
#define APP_SPKFUN_PROCESS_STK_SIZE	64

// </h>

// <h> Priority of ISR 
// <o>TIM1_ISR_PRIO <0-15>
// <i>Priority of PWM TIM1
#define TIM1_ISR_PRIO			2
// <o>TIM3_ISR_PRIO <0-15>
// <i>Priority of HALL TIM3
#define TIM3_ISR_PRIO			3
// <o>ADC_ISR_PRIO <0-15>
// <i>Priority of motor inject ADC
#define ADC_ISR_PRIO			0
// </h>
// *** <<< End of Configuration section             >>> *** 

/**
  * @brief Simplify stack point operations.
  */
#define OSTASKCREATE(a,b,c,d)	OSTaskCreateExt((void (*)(void *))a,\
						(void *)b,\
						(OS_STK *)&c[(sizeof(c)/sizeof(OS_STK)) - 1],\
						(INT8U)d,\
						(INT8U)d,\
						(OS_STK *)&c[0],\
						(INT32U)(sizeof(c)/sizeof(OS_STK)),\
						(void*)NULL,\
						OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR)
				
/**
  * @}
  */

/** @addtogroup APP_Configure_Exported_Functions
  * @{
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /*__APP_CFG_H */

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
