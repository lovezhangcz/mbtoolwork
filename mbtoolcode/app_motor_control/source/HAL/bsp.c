/**
  ******************************************************************************
  * @file    bsp.c
  * @brief   Board support package（治具版）
  *          初始化顺序：时钟检查 → GPIO → 三路串口 → ADC → 协议/屏幕 → 时基
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "my_type.h"
#include "bsp.h"
#include "tasttimebase.h"

#include "board.h"
#include "led.h"
#include "usart_drv.h"
#include "ads131.h"
#include "jig_proto.h"
#include "vgus.h"
#include "test_flow.h"

/**
  * @brief  Check clock configuration
  * @retval 1=normal, 0=abnormal
  */
static u8 ClockCheck(void)
{
    RCC_ClocksTypeDef rcc_clocks;

    RCC_GetClocksFreq(&rcc_clocks);
    if (rcc_clocks.SYSCLK_Frequency == SystemCoreClock) {
        return 1;
    }
    return 0;
}

/**
  * @brief  Board support package init
  * @retval 1=success, 0=fail
  */
u8 BSPInit(void)
{
    if (!ClockCheck()) {
        return 0;
    }

    board_gpio_init();          /* 全部 GPIO + 蜂鸣器 TIM1_CH1 + PD0 重映射 */
    LED_Init();                 /* 面板灯/运行灯安全初态 */

    usart_mo_init(115200);      /* 显示屏串口 USART3 */
    usart_sp_init(115200);      /* 备用串口 UART4（暂闲置，驱动保留） */
    usart_mb_init(115200);      /* 主板串口 USART2(PA2/PA3) */

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);   /* CRC 时钟（IAP 预留） */

    jig_proto_init();           /* JIG_PROTO 状态缓存 */
    vgus_init();                /* 屏驱动状态 */
    test_flow_init();           /* 测试流程状态机初态 */

    tasttimebaseInit();         /* TIM3 2kHz 任务时基（最后启动） */

    return 1;
}

/**
  * @brief  Get system tick clock frequency
  * @retval SystemCoreClock
  */
u32 OS_CPU_SysTickClkFreq(void)
{
    return SystemCoreClock;
}

/********* (C) COPYRIGHT 2014  TECHNOLOGIES CO.,LTD. ***END OF FILE***/
