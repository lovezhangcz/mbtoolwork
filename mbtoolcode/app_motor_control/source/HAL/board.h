/**
  ******************************************************************************
  * @file    board.h
  * @brief   治具主板引脚定义与板级接口（依据 SCH_Schematic1_2024-09-01，
  *          已按"网络名在引脚线上方"规则逐坐标核对 V2.0）
  *          MCU: GD32F103RCT6 (LQFP64)
  ******************************************************************************
  */

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"
#include "my_type.h"

/* ================= 主板串口（接被测板，经 U1 隔离）：USART2 ===============
 * MCU_MB_TX=PA2(USART2_TX), MCU_MB_RX=PA3(USART2_RX) —— 硬件串口          */
#define MB_USART        USART2
#define MB_TX_PORT      GPIOA
#define MB_TX_PIN       GPIO_Pin_2
#define MB_RX_PORT      GPIOA
#define MB_RX_PIN       GPIO_Pin_3

/* ================= 显示屏串口（经 U2 隔离）：USART3 ===================== */
#define MO_USART        USART3
#define MO_TX_PORT      GPIOB
#define MO_TX_PIN       GPIO_Pin_10
#define MO_RX_PORT      GPIOB
#define MO_RX_PIN       GPIO_Pin_11

/* ================= 备用串口（经 U3 隔离）：UART4 ======================== */
#define SP_UART         UART4
#define SP_TX_PORT      GPIOC
#define SP_TX_PIN       GPIO_Pin_10
#define SP_RX_PORT      GPIOC
#define SP_RX_PIN       GPIO_Pin_11

/* ================= 采样 SPI（经 U4/U5/U6 隔离，接 ADS131M04） ============
 * 正好是硬件 SPI1 引脚：SCK=PA5 MISO=PA6 MOSI=PA7，CS=PB0(软件控制)
 * CLKIN=PC5（无定时器通道，采样突发期内软件翻转）
 * DRDY =PC4（网络名 MCU_GPIO，接 ADS131M04 的 DRDY#）                    */
#define ADC_SPI         SPI1
#define ADC_SCLK_PORT   GPIOA
#define ADC_SCLK_PIN    GPIO_Pin_5
#define ADC_MISO_PORT   GPIOA
#define ADC_MISO_PIN    GPIO_Pin_6
#define ADC_MOSI_PORT   GPIOA
#define ADC_MOSI_PIN    GPIO_Pin_7
#define ADC_CS_PORT     GPIOB
#define ADC_CS_PIN      GPIO_Pin_0
#define ADC_CLKIN_PORT  GPIOC
#define ADC_CLKIN_PIN   GPIO_Pin_5
#define ADC_DRDY_PORT   GPIOC
#define ADC_DRDY_PIN    GPIO_Pin_4

/* ================= 蜂鸣器：PA8 = TIM1_CH1，2.7kHz 无源蜂鸣器 ============ */
#define BUZ_PORT        GPIOA
#define BUZ_PIN         GPIO_Pin_8

/* ================= 运行指示灯：PA9（LED0603 绿灯闪烁） ================== */
#define RUNLED_PORT     GPIOA
#define RUNLED_PIN      GPIO_Pin_9

/* ================= 上电/放电控制与检测（经 U20/U21 隔离） ===============
 * MCU_ACT_SW     PC13 输出：模拟被测板开机键（B->A 方向）
 * MCU_ACT_ON     PC14 输入：被测板已启动检测（A->B 方向）
 * MCU_ACT_MB_ON  PC15 输出：被测板通电控制（B->A 方向）
 * MCU_ACT_DISCHG PD0  输入：放电状态回读（A->B 方向，需 PD0 重映射）      */
#define ACT_SW_PORT     GPIOC
#define ACT_SW_PIN      GPIO_Pin_13
#define ACT_ON_PORT     GPIOC
#define ACT_ON_PIN      GPIO_Pin_14
#define ACT_MBON_PORT   GPIOC
#define ACT_MBON_PIN    GPIO_Pin_15
#define ACT_DISCHG_PORT GPIOD
#define ACT_DISCHG_PIN  GPIO_Pin_0

/* ================= 面板实体按键与指示灯（经 U17/U18/U19 隔离） ==========
 * KEY(ACT_ON_x) 输入：低有效（A 侧按键下拉到 AGND，带 4.7K 上拉）
 * LED(ACT_LED_x) 输出：高有效（驱动开关指示灯）                          */
#define KEY1_PORT       GPIOC
#define KEY1_PIN        GPIO_Pin_6
#define KEY1_LED_PORT   GPIOC
#define KEY1_LED_PIN    GPIO_Pin_7

#define KEY2_PORT       GPIOB
#define KEY2_PIN        GPIO_Pin_14
#define KEY2_LED_PORT   GPIOB
#define KEY2_LED_PIN    GPIO_Pin_15

#define KEY3_PORT       GPIOB
#define KEY3_PIN        GPIO_Pin_12
#define KEY3_LED_PORT   GPIOB
#define KEY3_LED_PIN    GPIO_Pin_13

/* ================= 板级接口函数 ================= */
void board_gpio_init(void);

/* 蜂鸣器：2.7kHz PWM */
void buzzer_on(void);
void buzzer_off(void);
void buzzer_beep(u16 ms);   /* 简易阻塞鸣叫，仅自检等低频场景用 */

/* 运行灯 */
void runled_toggle(void);
void runled_set(u8 on);

/* 上电控制 */
void act_mb_power(u8 on);   /* 1=继电器吸合给被测板通电 */
void act_powerkey(u8 press);/* 1=模拟按下被测板开机键 */
u8   act_dut_online(void);  /* 读 ACT_ON：被测板是否已启动 */
u8   act_dischg_state(void);/* 读 ACT_DISCHG 放电状态 */

/* 面板按键（低有效，返回 1=按下）与按键灯 */
u8   key_read(u8 key_id);   /* key_id: 1/2/3 */
void key_led(u8 key_id, u8 on);

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
