/**
  ******************************************************************************
  * @file    vgus.c
  * @brief   中显 VGUS 串口屏驱动实现
  *          帧格式：A5 5A | LEN | CMD | DATA...
  *          LEN = CMD 及其后所有字节的个数；寄存器地址单字节、变量地址双字节(大端)
  ******************************************************************************
  */

#include "vgus.h"
#include "usart_drv.h"

vgus_touch_t   g_vgus_touch;
volatile u8    g_vgus_ver;

/* ---------------- 发送 ---------------- */
static void vgus_send(u8 cmd, const u8 *payload, u8 plen)
{
    u8 head[3];
    head[0] = 0xA5u;
    head[1] = 0x5Au;
    head[2] = (u8)(1 + plen);          /* LEN = CMD + 数据 */
    usart_mo_send(head, 3);
    usart_mo_send(&cmd, 1);
    if (plen) {
        usart_mo_send(payload, plen);
    }
}

/* 写寄存器（0x80）：reg 单字节地址 */
static void vgus_wr_reg(u8 reg, const u8 *data, u8 len)
{
    u8 buf[8];
    u8 i;
    if (len > 7) { len = 7; }
    buf[0] = reg;
    for (i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }
    vgus_send(0x80u, buf, (u8)(1 + len));
}

/* 读寄存器（0x81） */
static void vgus_rd_reg(u8 reg, u8 len)
{
    u8 buf[2];
    buf[0] = reg;
    buf[1] = len;
    vgus_send(0x81u, buf, 2);
}

void vgus_set_page(u16 page_id)
{
    u8 d[2];
    d[0] = (u8)(page_id >> 8);
    d[1] = (u8)(page_id & 0xFFu);
    vgus_wr_reg(0x03u, d, 2);
}

void vgus_buzzer(u8 time_10ms)
{
    vgus_wr_reg(0x02u, &time_10ms, 1);
}

void vgus_set_backlight(u8 level)
{
    if (level > 0x40u) { level = 0x40u; }
    vgus_wr_reg(0x01u, &level, 1);
}

void vgus_read_version(void)
{
    vgus_rd_reg(0x00u, 1);
}

/* 写变量存储器（0x82），data 为字节流（大端字序由调用方组好） */
static void vgus_wr_vp_raw(u16 vp, const u8 *data, u8 len)
{
    u8 head[2];
    head[0] = (u8)(vp >> 8);
    head[1] = (u8)(vp & 0xFFu);

    {
        u8 hdr[3];
        hdr[0] = 0xA5u;
        hdr[1] = 0x5Au;
        hdr[2] = (u8)(1 + 2 + len);
        usart_mo_send(hdr, 3);
    }
    {
        u8 cmd = 0x82u;
        usart_mo_send(&cmd, 1);
    }
    usart_mo_send(head, 2);
    usart_mo_send(data, len);
}

void vgus_write_u16(u16 vp, u16 val)
{
    u8 d[2];
    d[0] = (u8)(val >> 8);
    d[1] = (u8)(val & 0xFFu);
    vgus_wr_vp_raw(vp, d, 2);
}

void vgus_write_s16(u16 vp, s16 val)
{
    vgus_write_u16(vp, (u16)val);
}

void vgus_write_words(u16 vp, const u16 *data, u8 nword)
{
    u8  buf[32];
    u8  i;
    if (nword > 16) { nword = 16; }
    for (i = 0; i < nword; i++) {
        buf[2 * i]     = (u8)(data[i] >> 8);
        buf[2 * i + 1] = (u8)(data[i] & 0xFFu);
    }
    vgus_wr_vp_raw(vp, buf, (u8)(2 * nword));
}

void vgus_write_text(u16 vp, const char *gbk_str, u8 maxlen)
{
    u8  buf[2 * 32 + 2];
    u8  n = 0;
    u8  maxb = (u8)(2 * maxlen);

    if (maxlen > 32) { maxlen = 32; maxb = 64; }
    if (maxb > (sizeof(buf) - 2)) { maxb = sizeof(buf) - 2; }

    while (gbk_str[n] && n < maxb) {
        buf[n] = (u8)gbk_str[n];
        n++;
        /* GBK 双字节字符：若首字节 >= 0x81，连同次字节一起搬，防止截断半个汉字 */
        if (buf[n - 1] >= 0x81u && gbk_str[n] && n < maxb) {
            buf[n] = (u8)gbk_str[n];
            n++;
        }
    }
    buf[n++] = 0xFFu;                  /* 文本结束符 0xFFFF */
    buf[n++] = 0xFFu;
    vgus_wr_vp_raw(vp, buf, n);
}

/* ---------------- 接收解析 ----------------
 * 屏主动上传（0x83）：触摸键值/控件值变化
 * 读应答（0x81）：寄存器读取结果                                         */
void vgus_poll(void)
{
    static u8  frame[32];
    static u8  idx = 0;
    static u8  expect = 0;             /* 期望帧总长（含帧头） */
    u8  b;

    while (drv_rx_read(&g_rx_mo, &b, 1) == 1) {
        if (idx == 0 && b != 0xA5u) { continue; }
        if (idx == 1 && b != 0x5Au) { idx = 0; continue; }

        frame[idx++] = b;
        if (idx == 3) {
            expect = (u8)(b + 3);      /* 总长 = 帧头2 + LEN字节1 + LEN值 */
            if (expect > sizeof(frame)) { idx = 0; expect = 0; continue; }
        }
        if (expect && idx >= expect) {
            u8 cmd = frame[3];
            idx = 0; expect = 0;

            if (cmd == 0x83u) {
                /* 0x83 VP_H VP_L LEN_H(字) DATA...  触摸键值上传按 1 字处理 */
                u16 vp  = ((u16)frame[4] << 8) | frame[5];
                u16 val = ((u16)frame[7] << 8) | frame[8];
                g_vgus_touch.vp  = vp;
                g_vgus_touch.key = val;
                g_vgus_touch.new_flag = 1;
            } else if (cmd == 0x81u) {
                /* 0x81 REG LEN DATA...  目前只关心版本号 */
                if (frame[4] == 0x00u) {
                    g_vgus_ver = frame[6];
                }
            }
        }
    }
}

void vgus_init(void)
{
    g_vgus_touch.new_flag = 0;
    g_vgus_ver = 0;
}
