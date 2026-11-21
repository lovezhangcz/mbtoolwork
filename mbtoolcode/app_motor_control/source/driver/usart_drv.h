/**
  ******************************************************************************
  * @file    usart_drv.h
  * @brief   治具硬件串口驱动：
  *          USART2(PA2/PA3)=主板串口(接被测板，经 U1 隔离)
  *          USART3(PB10/PB11)=显示屏串口(经 U2 隔离)
  *          UART4(PC10/PC11)=备用串口(经 U3 隔离，暂闲置)
  *          接收：RXNE+IDLE 中断 + 环形缓冲；发送：查询式
  ******************************************************************************
  */

#ifndef __USART_DRV_H
#define __USART_DRV_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "my_type.h"

#define DRV_RXBUF_SIZE   256

typedef struct {
    u8  buf[DRV_RXBUF_SIZE];
    volatile u16 wr;
    volatile u16 rd;
    volatile u16 frame_len;   /* IDLE 中断置位：一帧空闲结束时的可读字节数 */
    volatile u8  idle_flag;
} drv_rx_t;

/* 主板串口 USART2（PA2/PA3，115200 8N1） */
void usart_mb_init(u32 baud);
void usart_mb_send(const u8 *data, u16 len);
extern drv_rx_t g_rx_mb;

/* 显示屏串口 USART3（PB10/PB11） */
void usart_mo_init(u32 baud);
void usart_mo_send(const u8 *data, u16 len);
extern drv_rx_t g_rx_mo;

/* 备用串口 UART4（PC10/PC11），暂闲置备用 */
void usart_sp_init(u32 baud);
void usart_sp_send(const u8 *data, u16 len);
extern drv_rx_t g_rx_sp;

/* 环形缓冲读取：返回读出字节数 */
u16 drv_rx_read(drv_rx_t *rx, u8 *out, u16 maxlen);

#ifdef __cplusplus
}
#endif

#endif /* __USART_DRV_H */
