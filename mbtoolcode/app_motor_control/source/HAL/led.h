/**
  ******************************************************************************
  * @file    led.h
  * @brief   LED 兼容薄封装（治具版）——实际引脚操作由 board.h 提供
  *          旧单轮车项目的 LEDRED/LEDWHITE 等宏已随硬件废弃，不再提供
  ******************************************************************************
  */

#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "board.h"

/* 兼容旧接口：初始化全部面板灯与运行灯（board_gpio_init 已完成 GPIO 配置） */
void LED_Init(void);
void ledshow(void);   /* 旧项目的车灯秀，治具不使用，空实现保留接口 */

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */
