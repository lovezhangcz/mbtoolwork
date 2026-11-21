/**
  ******************************************************************************
  * @file    ads131.c
  * @brief   ADS131M04 驱动实现（硬件 SPI1 Mode1，24bit 字长，突发式采样）
  *
  * 要点：
  *  - CLKIN(PC5) 无定时器通道：仅在一次采样事务内由软件循环翻转提供主时钟，
  *    等待 DRDY 的过程中也持续翻转；事务间 CLKIN 停振，ADC 暂停转换，无影响。
  *  - SPI1 模式1（CPOL=0, CPHA=2Edge），8bit 帧 x3 拼 24bit 字，MSB 先。
  *  - 帧结构（24bit 字，共 6 字）：
  *      DIN : [命令字(高16位有效)] [0] [0] [0] [0] [0]
  *      DOUT: [上帧命令响应] [CH0] [CH1] [CH2] [CH3] [CRC=0]
  *  - 默认 CRC 关闭、增益全通道 x1、OSR=1024，数据率约 fCLKIN/2048。
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "ads131.h"
#include "board.h"

/* ---------------- 命令字（16bit，发送时 <<8） ---------------- */
#define ADS131_CMD_NULL     0x0000u
#define ADS131_CMD_RESET    0x0011u
#define ADS131_CMD_RREG(id) (0xA000u | ((u16)(id) << 7))   /* 读 1 个寄存器 */

/* 寄存器地址 */
#define ADS131_REG_ID       0x00u

/* ---------------- 板级分压比例表（按原理图 V2.0 核对值） ------------------
 * mv(被测) = raw_to_uv(raw) / 1000 * DIV_TOP / DIV_BOT
 *   母线: R24=200K / R29=10K  -> 21:1   42V  -> ADC 2.00V
 *   12V : R25=47K  / R30=10K  -> 5.7:1  12V  -> ADC 2.11V
 *   5V  : R26=30K  / R31=20K  -> 2.5:1  5V   -> ADC 2.00V
 *   3.3V: R27=6.2K / R32=10K  -> 1.62:1 3.3V -> ADC 2.04V
 * ★ 四路按图上值均超过 ADS131M04 ±1.2V 满量程（gain=1），会削顶！
 *   已反馈硬件，改电阻后同步修正本表。
 *   比例用 (162,100) 表示 1.62，避免浮点。
 * ------------------------------------------------------------------------ */
typedef struct { u16 top; u16 bot; } div_t;
static const div_t s_div[ADS131_CH_NUM] = {
    /* BUS */ {  21,   1 },   /* 200K/10K  */
    /* 12V */ {  57,  10 },   /* 47K/10K   */
    /* 5V  */ {  25,  10 },   /* 30K/20K   */
    /* 3V3 */ { 162, 100 },   /* 6.2K/10K  */
};

/* ---------------- 底层操作 ---------------- */
static void cs(u8 v)    { GPIO_WriteBit(ADC_CS_PORT,   ADC_CS_PIN,   v ? Bit_SET : Bit_RESET); }
static void clkin(u8 v) { GPIO_WriteBit(ADC_CLKIN_PORT,ADC_CLKIN_PIN,v ? Bit_SET : Bit_RESET); }
static u8   drdy(void)  { return GPIO_ReadInputDataBit(ADC_DRDY_PORT, ADC_DRDY_PIN) ? 1u : 0u; }

/* CLKIN 翻转一个周期（约 500kHz~1MHz 量级，无需精确） */
static void clkin_tick(void)
{
    clkin(1); __NOP(); __NOP(); __NOP();
    clkin(0); __NOP(); __NOP(); __NOP();
}

/* SPI1 收发 8bit（查询式） */
static u8 spi_xfer8(u8 tx)
{
    while (SPI_I2S_GetFlagStatus(ADC_SPI, SPI_I2S_FLAG_TXE) == RESET) {}
    SPI_I2S_SendData(ADC_SPI, tx);
    while (SPI_I2S_GetFlagStatus(ADC_SPI, SPI_I2S_FLAG_RXNE) == RESET) {}
    return (u8)SPI_I2S_ReceiveData(ADC_SPI);
}

/* 收发 24bit 字 */
static u32 spi_xfer24(u32 tx)
{
    u32 rx = 0;
    rx |= (u32)spi_xfer8((u8)(tx >> 16));
    rx = (rx << 8) | spi_xfer8((u8)(tx >> 8));
    rx = (rx << 8) | spi_xfer8((u8)tx);
    return rx;
}

/* 收发一帧（6 个 24bit 字），返回 DOUT 6 字 */
static void xfer_frame(u32 cmd16, u32 dout[6])
{
    u8 i;
    cs(0);
    dout[0] = spi_xfer24(((u32)cmd16) << 8);
    for (i = 1; i < 6; i++) {
        dout[i] = spi_xfer24(0);
    }
    cs(1);
}

/* 等待 DRDY 拉低，等待期间持续翻转 CLKIN；超时返回 0 */
static u8 wait_drdy(u32 max_ticks)
{
    while (max_ticks--) {
        clkin_tick();
        if (!drdy()) {
            return 1;
        }
    }
    return 0;
}

/* 24bit 原始字 -> s32 符号扩展 */
static s32 sext24(u32 v)
{
    v &= 0x00FFFFFFuL;
    if (v & 0x00800000uL) {
        v |= 0xFF000000uL;
    }
    return (s32)v;
}

u8 ads131_init(u16 *id)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;
    u32 dout[6];
    u16 idv = 0xFFFFu;
    u8  i;

    /* SPI1 引脚：SCK(PA5)/MOSI(PA7) 复用推挽，MISO(PA6) 浮空输入 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = ADC_SCLK_PIN | ADC_MOSI_PIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin  = ADC_MISO_PIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* SPI1：主模式、模式1、8bit、MSB 先、软件 NSS、约 4.5MHz */
    SPI_InitStructure.SPI_Direction     = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode          = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize      = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL          = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA          = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS           = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit      = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(ADC_SPI, &SPI_InitStructure);
    SPI_Cmd(ADC_SPI, ENABLE);

    cs(1); clkin(0);

    /* 软件复位 */
    xfer_frame(ADS131_CMD_RESET, dout);
    for (i = 0; i < 200; i++) {      /* 复位后等待若干时钟 */
        clkin_tick();
    }

    /* 读 ID：RREG 帧的响应在下一帧第 0 字返回（高 16 位有效） */
    if (wait_drdy(200000)) {
        xfer_frame(ADS131_CMD_RREG(ADS131_REG_ID), dout);
        if (wait_drdy(200000)) {
            xfer_frame(ADS131_CMD_NULL, dout);
            idv = (u16)(dout[0] >> 8);
        }
    }

    if (id) {
        *id = idv;
    }
    /* ADS131M04 ID 寄存器高 8 位约为 0x24，低 8 位含通道数；此处仅做
       通信活性判断：全 0 或全 1 视为失败，具体值留给调试输出 */
    return (idv != 0x0000u && idv != 0xFFFFu) ? 1u : 0u;
}

u8 ads131_read_raw(s32 raw[4])
{
    u32 dout[6];
    u8  i;

    if (!wait_drdy(400000)) {        /* 约几百毫秒量级超时 */
        return 0;
    }
    xfer_frame(ADS131_CMD_NULL, dout);
    for (i = 0; i < 4; i++) {
        raw[i] = sext24(dout[i + 1]);
    }
    return 1;
}

u8 ads131_read_avg(s32 raw_avg[4], u8 n)
{
    s64 acc[4] = {0, 0, 0, 0};
    s32 raw[4];
    u8  i, k;

    if (n < 2)  { n = 2; }
    if (n > 64) { n = 64; }

    for (i = 0; i < n; i++) {
        if (!ads131_read_raw(raw)) {
            return 0;
        }
        for (k = 0; k < 4; k++) {
            acc[k] += raw[k];
        }
    }
    for (k = 0; k < 4; k++) {
        raw_avg[k] = (s32)(acc[k] / n);
    }
    return 1;
}

s32 ads131_raw_to_uv(s32 raw)
{
    /* V(uV) = raw / 2^23 * 1.2e6，增益=1 */
    return (s32)(((s64)raw * 1200000) / 8388608);
}

s32 ads131_raw_to_mv(ads131_ch_t ch, s32 raw)
{
    s32 uv = ads131_raw_to_uv(raw);
    if (ch >= ADS131_CH_NUM) {
        return 0;
    }
    return (s32)(((s64)uv * s_div[ch].top) / (s_div[ch].bot * 1000));
}

u8 ads131_read_mv(s32 mv[4])
{
    s32 raw[4];
    u8  i;

    if (!ads131_read_avg(raw, 16)) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        mv[i] = ads131_raw_to_mv((ads131_ch_t)i, raw[i]);
    }
    return 1;
}
