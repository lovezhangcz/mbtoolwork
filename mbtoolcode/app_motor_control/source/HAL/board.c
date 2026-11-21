/**
  ******************************************************************************
  * @file    board.c
  * @brief   治具主板板级驱动：GPIO / 蜂鸣器 / 按键 / 指示灯 / 上电控制
  *          引脚按 V2.0 核对版（board.h）
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "my_type.h"
#include "board.h"

/* 蜂鸣器 PWM：TIM1_CH1 = PA8，2.7kHz */
#define BUZ_FREQ_HZ     2700

/**
  * @brief  全部 GPIO 初始化
  */
void board_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* 关闭 JTAG 保留 SWD；PD0/PD1 重映射为普通 IO（ACT_DISCHG 用 PD0） */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_PD01, ENABLE);

    /* ---- 输出：推挽 50MHz ---- */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* MCU_ACT_SW(PC13)、MCU_ACT_MB_ON(PC15)：输出，默认无效（低） */
    GPIO_InitStructure.GPIO_Pin = ACT_SW_PIN | ACT_MBON_PIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_ResetBits(ACT_SW_PORT, ACT_SW_PIN);
    GPIO_ResetBits(ACT_MBON_PORT, ACT_MBON_PIN);

    /* 按键指示灯：PC7、PB15、PB13，默认灭 */
    GPIO_InitStructure.GPIO_Pin = KEY1_LED_PIN;
    GPIO_Init(KEY1_LED_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = KEY2_LED_PIN | KEY3_LED_PIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_ResetBits(KEY1_LED_PORT, KEY1_LED_PIN);
    GPIO_ResetBits(KEY2_LED_PORT, KEY2_LED_PIN);
    GPIO_ResetBits(KEY3_LED_PORT, KEY3_LED_PIN);

    /* 运行灯 PA9 */
    GPIO_InitStructure.GPIO_Pin = RUNLED_PIN;
    GPIO_Init(RUNLED_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(RUNLED_PORT, RUNLED_PIN);

    /* 采样控制输出：CS(PB0) 空闲高、CLKIN(PC5) 空闲低 */
    GPIO_InitStructure.GPIO_Pin = ADC_CS_PIN;
    GPIO_Init(ADC_CS_PORT, &GPIO_InitStructure);
    GPIO_SetBits(ADC_CS_PORT, ADC_CS_PIN);
    GPIO_InitStructure.GPIO_Pin = ADC_CLKIN_PIN;
    GPIO_Init(ADC_CLKIN_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(ADC_CLKIN_PORT, ADC_CLKIN_PIN);

    /* ---- 输入 ---- */
    /* ACT_ON(PC14)、ACT_DISCHG(PD0)：上拉输入 */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = ACT_ON_PIN;
    GPIO_Init(ACT_ON_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = ACT_DISCHG_PIN;
    GPIO_Init(ACT_DISCHG_PORT, &GPIO_InitStructure);

    /* 按键 KEY1(PC6)、KEY2(PB14)、KEY3(PB12)：上拉输入，低有效 */
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN;
    GPIO_Init(KEY1_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = KEY2_PIN | KEY3_PIN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* DRDY(PC4)：浮空输入（隔离器有驱动能力） */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin  = ADC_DRDY_PIN;
    GPIO_Init(ADC_DRDY_PORT, &GPIO_InitStructure);

    /* SPI1 引脚在 ads131_init() 中配置为复用功能 */

    /* ---- 蜂鸣器 TIM1_CH1(PA8) 复用推挽，默认关闭 ---- */
    {
        TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
        TIM_OCInitTypeDef       TIM_OCInitStructure;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Pin   = BUZ_PIN;
        GPIO_Init(BUZ_PORT, &GPIO_InitStructure);

        /* 72MHz / (71+1) = 1MHz 计数；ARR = 1MHz/2.7kHz 约 370 */
        TIM_TimeBaseStructure.TIM_Prescaler     = 71;
        TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
        TIM_TimeBaseStructure.TIM_Period        = 1000000 / BUZ_FREQ_HZ - 1;
        TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
        TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

        TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
        TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
        TIM_OCInitStructure.TIM_Pulse       = (1000000 / BUZ_FREQ_HZ) / 2; /* 50% 占空比 */
        TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
        TIM_OC1Init(TIM1, &TIM_OCInitStructure);
        TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
        TIM_ARRPreloadConfig(TIM1, ENABLE);
        TIM_CtrlPWMOutputs(TIM1, ENABLE);
        /* 默认不开输出 */
        TIM_Cmd(TIM1, DISABLE);
    }
}

/* ================= 蜂鸣器 ================= */
void buzzer_on(void)  { TIM_Cmd(TIM1, ENABLE); }
void buzzer_off(void) { TIM_Cmd(TIM1, DISABLE); GPIO_ResetBits(BUZ_PORT, BUZ_PIN); }

void buzzer_beep(u16 ms)
{
    u32 i;
    buzzer_on();
    while (ms--) { for (i = 0; i < 9000; i++) __nop(); }  /* 约 1ms @72MHz */
    buzzer_off();
}

/* ================= 运行灯 ================= */
void runled_toggle(void)
{
    RUNLED_PORT->ODR ^= RUNLED_PIN;
}
void runled_set(u8 on)
{
    if (on) GPIO_SetBits(RUNLED_PORT, RUNLED_PIN);
    else    GPIO_ResetBits(RUNLED_PORT, RUNLED_PIN);
}

/* ================= 上电控制 ================= */
void act_mb_power(u8 on)
{
    if (on) GPIO_SetBits(ACT_MBON_PORT, ACT_MBON_PIN);
    else    GPIO_ResetBits(ACT_MBON_PORT, ACT_MBON_PIN);
}
void act_powerkey(u8 press)
{
    if (press) GPIO_SetBits(ACT_SW_PORT, ACT_SW_PIN);
    else       GPIO_ResetBits(ACT_SW_PORT, ACT_SW_PIN);
}
u8 act_dut_online(void)
{
    return (GPIO_ReadInputDataBit(ACT_ON_PORT, ACT_ON_PIN) == Bit_SET) ? 1 : 0;
}
u8 act_dischg_state(void)
{
    return (GPIO_ReadInputDataBit(ACT_DISCHG_PORT, ACT_DISCHG_PIN) == Bit_SET) ? 1 : 0;
}

/* ================= 面板按键与按键灯 ================= */
u8 key_read(u8 key_id)
{
    BitAction v = Bit_SET;
    switch (key_id) {
    case 1: v = GPIO_ReadInputDataBit(KEY1_PORT, KEY1_PIN); break;
    case 2: v = GPIO_ReadInputDataBit(KEY2_PORT, KEY2_PIN); break;
    case 3: v = GPIO_ReadInputDataBit(KEY3_PORT, KEY3_PIN); break;
    default: break;
    }
    return (v == Bit_RESET) ? 1 : 0;   /* 低有效 */
}

void key_led(u8 key_id, u8 on)
{
    switch (key_id) {
    case 1: if (on) GPIO_SetBits(KEY1_LED_PORT, KEY1_LED_PIN);
            else    GPIO_ResetBits(KEY1_LED_PORT, KEY1_LED_PIN); break;
    case 2: if (on) GPIO_SetBits(KEY2_LED_PORT, KEY2_LED_PIN);
            else    GPIO_ResetBits(KEY2_LED_PORT, KEY2_LED_PIN); break;
    case 3: if (on) GPIO_SetBits(KEY3_LED_PORT, KEY3_LED_PIN);
            else    GPIO_ResetBits(KEY3_LED_PORT, KEY3_LED_PIN); break;
    default: break;
    }
}
