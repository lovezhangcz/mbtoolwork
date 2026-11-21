/**
  ******************************************************************************
  * @file    usart_drv.c
  * @brief   治具硬件串口驱动：USART2(主板)、USART3(显示屏)、UART4(备用)
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "my_type.h"
#include "board.h"
#include "usart_drv.h"

drv_rx_t g_rx_mb;
drv_rx_t g_rx_mo;
drv_rx_t g_rx_sp;

u16 drv_rx_read(drv_rx_t *rx, u8 *out, u16 maxlen)
{
    u16 n = 0;
    while (n < maxlen && rx->rd != rx->wr) {
        out[n++] = rx->buf[rx->rd];
        rx->rd = (rx->rd + 1) % DRV_RXBUF_SIZE;
    }
    return n;
}

/* 环形缓冲写入：满则丢弃新字节 */
static void drv_rx_put(drv_rx_t *rx, u8 d)
{
    u16 next = (rx->wr + 1) % DRV_RXBUF_SIZE;
    if (next != rx->rd) {
        rx->buf[rx->wr] = d;
        rx->wr = next;
    }
}

static void usart_irq_config(IRQn_Type irq, u8 preemption)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = preemption;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/* 通用初始化：tx/rx 引脚 + 8N1 + RXNE/IDLE 中断 */
static void usart_common_init(USART_TypeDef *uart, u32 baud,
                              GPIO_TypeDef *tx_port, u16 tx_pin,
                              GPIO_TypeDef *rx_port, u16 rx_pin,
                              IRQn_Type irq)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    GPIO_InitStructure.GPIO_Pin   = tx_pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(tx_port, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = rx_pin;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(rx_port, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate   = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;
    USART_InitStructure.USART_Parity     = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(uart, &USART_InitStructure);

    USART_ITConfig(uart, USART_IT_RXNE, ENABLE);
    USART_ITConfig(uart, USART_IT_IDLE, ENABLE);
    usart_irq_config(irq, 2);

    USART_Cmd(uart, ENABLE);
}

static void usart_send_byte(USART_TypeDef *uart, u8 d)
{
    while (USART_GetFlagStatus(uart, USART_FLAG_TXE) == RESET) {}
    USART_SendData(uart, d);
}

/* ================= USART2 主板串口 ================= */
void usart_mb_init(u32 baud)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    usart_common_init(USART2, baud, MB_TX_PORT, MB_TX_PIN, MB_RX_PORT, MB_RX_PIN, USART2_IRQn);
}

void usart_mb_send(const u8 *data, u16 len)
{
    while (len--) {
        usart_send_byte(USART2, *data++);
    }
}

/* ================= USART3 显示屏串口 ================= */
void usart_mo_init(u32 baud)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    usart_common_init(USART3, baud, MO_TX_PORT, MO_TX_PIN, MO_RX_PORT, MO_RX_PIN, USART3_IRQn);
}

void usart_mo_send(const u8 *data, u16 len)
{
    while (len--) {
        usart_send_byte(USART3, *data++);
    }
}

/* ================= UART4 备用串口 ================= */
void usart_sp_init(u32 baud)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
    usart_common_init(UART4, baud, SP_TX_PORT, SP_TX_PIN, SP_RX_PORT, SP_RX_PIN, UART4_IRQn);
}

void usart_sp_send(const u8 *data, u16 len)
{
    while (len--) {
        usart_send_byte(UART4, *data++);
    }
}

/* ================= 中断服务 ================= */
static void rx_irq_handler(USART_TypeDef *uart, drv_rx_t *rx)
{
    if (USART_GetITStatus(uart, USART_IT_RXNE) == SET) {
        u8 d = (u8)USART_ReceiveData(uart);
        drv_rx_put(rx, d);
    }
    if (USART_GetITStatus(uart, USART_IT_IDLE) == SET) {
        (void)USART_ReceiveData(uart);     /* 读 SR 后读 DR 清 IDLE */
        rx->frame_len = (rx->wr >= rx->rd) ?
                        (rx->wr - rx->rd) :
                        (DRV_RXBUF_SIZE - rx->rd + rx->wr);
        rx->idle_flag = 1;
    }
}

void USART2_IRQHandler(void) { rx_irq_handler(USART2, &g_rx_mb); }
void USART3_IRQHandler(void) { rx_irq_handler(USART3, &g_rx_mo); }
void UART4_IRQHandler(void)  { rx_irq_handler(UART4,  &g_rx_sp); }
