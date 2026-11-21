/**
  ******************************************************************************
  * @file    ads131.h
  * @brief   ADS131M04 四通道 24bit ADC 驱动（硬件 SPI1 + 软件 CLKIN）
  *          测量对象：被测板母线电压 / 12V / 5V / 3.3V
  *          硬件连接（经 U4/U5/U6 隔离，引脚 V2.0 核对版）：
  *            SCK=PA5(SPI1_SCK)  MISO=PA6(SPI1_MISO)  MOSI=PA7(SPI1_MOSI)
  *            CS=PB0(软件控制)
  *            CLKIN=PC5（无定时器通道，采样突发期内软件翻转，约 500kHz）
  *            DRDY=PC4（网络名 MCU_GPIO，接 ADS131M04 的 DRDY#）
  *          时序：SPI Mode1（CPOL=0, CPHA=2Edge），字长 24bit，每帧 6 字
  *            [0]=命令响应  [1..4]=CH0..CH3  [5]=CRC(禁用时为0)
  *
  *  ★ 硬件注意：四路分压按图上值均把满度设在约 2.0V，超出 ADS131M04
  *    ±1.2V 满量程（gain=1），实测会削顶，需硬件改小分压比，详见
  *    design/工作笔记.md 待确认事项 #1。
  ******************************************************************************
  */

#ifndef __ADS131_H
#define __ADS131_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "my_type.h"

/* 通道序号（对应 AIN0~AIN3） */
typedef enum {
    ADS131_CH_BUS = 0,   /* 母线电压 CN4（10 串锂电，最高约 42V） */
    ADS131_CH_12V = 1,   /* 12V  CN5 */
    ADS131_CH_5V  = 2,   /* 5V   CN6 */
    ADS131_CH_3V3 = 3,   /* 3.3V CN7 */
    ADS131_CH_NUM = 4
} ads131_ch_t;

/* 初始化：SPI1 配置 + 软件复位 + 读 ID 验证通信。
   返回 1=通信正常，0=失败（id 为读到的原始 ID 寄存器值，可用于诊断） */
u8 ads131_init(u16 *id);

/* 读取一次全部 4 通道原始值（24bit 有符号，已符号扩展到 s32）。
   返回 1=成功，0=超时（ADC 无响应） */
u8 ads131_read_raw(s32 raw[4]);

/* 连续采样 n 次取平均（2~64），返回 1=成功 */
u8 ads131_read_avg(s32 raw_avg[4], u8 n);

/* 原始值换算为 ADC 引脚电压（单位 uV，增益=1，满量程 +-1.2V） */
s32 ads131_raw_to_uv(s32 raw);

/* 原始值按板级分压比换算为被测电压（单位 mV），见 ads131.c 顶部比例表 */
s32 ads131_raw_to_mv(ads131_ch_t ch, s32 raw);

/* 便捷接口：读 4 路电压，单位 mV。返回 1=成功 */
u8 ads131_read_mv(s32 mv[4]);

#ifdef __cplusplus
}
#endif

#endif /* __ADS131_H */
