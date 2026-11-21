/**
  ******************************************************************************
  * @file    jig_proto.c
  * @brief   JIG_PROTO V1.0 治具侧实现（组帧/解析/状态缓存）
  ******************************************************************************
  */

#include "jig_proto.h"
#include "usart_drv.h"

/* ---------------- 全局状态 ---------------- */
jig_item_t      g_jig_items[JIG_ITEM_NUM];
jig_handshake_t g_jig_hs;
jig_progress_t  g_jig_prog;
jig_realtime_t  g_jig_rt;
jig_finish_t    g_jig_fin;
volatile u8     g_jig_ack;
volatile u8     g_jig_ack_new;
volatile u8     g_jig_online;

/* ---------------- 测试项名称对照表（治具侧保存，主板不发字符串） ----------
 * 索引与 design/治具对接协议_v1.0.md §4 一致                          */
static const char *const s_item_names[JIG_ITEM_NUM] = {
    /* 0~12  左电机 */
    "左A相零偏", "左B相零偏", "左C相零偏",
    "左A相静态", "左B相静态", "左C相静态",
    "左A相吸附", "左B相吸附", "左C相吸附",
    "左霍尔低位", "左霍尔高位", "左霍尔速度", "左霍尔错误",
    /* 13~25 右电机 */
    "右A相零偏", "右B相零偏", "右C相零偏",
    "右A相静态", "右B相静态", "右C相静态",
    "右A相吸附", "右B相吸附", "右C相吸附",
    "右霍尔低位", "右霍尔高位", "右霍尔速度", "右霍尔错误",
    /* 26~33 公共 */
    "母线电压", "母线电流零偏", "母线静态电流", "吸附总电流",
    "MOS温度低限", "MOS温度高限", "CPU电压", "遥控通信计数",
};

const char *jig_item_name(u8 idx)
{
    if (idx < JIG_ITEM_NUM) {
        return s_item_names[idx];
    }
    return "未知项";
}

/* ---------------- 组帧/发送 ---------------- */
static u8 chk_sum(const u8 *d)    /* 字节0~16 累加和 ^ 0xFF */
{
    u8  i;
    u16 s = 0;
    for (i = 0; i < 17; i++) {
        s += d[i];
    }
    return (u8)((s & 0xFFu) ^ 0xFFu);
}

/* 组 20 字节帧并发送。d 为 14 字节数据区内容（不足补 0 由调用方保证） */
static u8 jig_send_frame(u8 cmd, const u8 d[14])
{
    u8 f[JIG_FRAME_LEN];
    u8 i;

    f[0] = 0xAAu; f[1] = 0x5Au;
    for (i = 0; i < 14; i++) {
        f[2 + i] = d[i];
    }
    f[16] = cmd;
    f[17] = chk_sum(f);
    f[18] = 0x5Au; f[19] = 0x55u;
    usart_mb_send(f, JIG_FRAME_LEN);
    return 0;
}

static u8 jig_send_sub(u8 sub)
{
    u8 d[14] = {0};
    d[0] = sub;
    return jig_send_frame(JIG_CMD_TO_DUT, d);
}

u8 jig_send_handshake(void) { return jig_send_sub(JIG_SUB_HANDSHAKE); }
u8 jig_send_start(void)     { return jig_send_sub(JIG_SUB_START); }
u8 jig_send_stop(void)      { return jig_send_sub(JIG_SUB_STOP); }
u8 jig_send_progress(void)  { return jig_send_sub(JIG_SUB_PROGRESS); }
u8 jig_send_realtime(void)  { return jig_send_sub(JIG_SUB_REALTIME); }

u8 jig_send_ver_req(void)
{
    u8 d[14] = {0};
    return jig_send_frame(JIG_CMD_VER_REQ, d);
}

u8 jig_send_beep(void)
{
    u8 d[14] = {0};
    d[0] = 3;    /* 蜂鸣次数（沿用主板 0x88 帧习惯：数据[2]=次数） */
    return jig_send_frame(JIG_CMD_BEEP, d);
}

/* ---------------- 解析 ---------------- */
static s16 rd_s16(const u8 *p) { return (s16)((u16)p[0] | ((u16)p[1] << 8)); }
static u16 rd_u16(const u8 *p) { return (u16)((u16)p[0] | ((u16)p[1] << 8)); }

/* 0xC9 应答/上报处理，p 指向 14 字节数据区 */
static void handle_c9(const u8 *p)
{
    u8 sub = p[0];

    switch (sub) {
    case JIG_SUB_HANDSHAKE:                 /* 握手应答 */
        if (p[1] == 0) {
            u8 i;
            g_jig_hs.ok = 1;
            for (i = 0; i < 8; i++) { g_jig_hs.model[i]   = p[2 + i]; }
            for (i = 0; i < 4; i++) { g_jig_hs.version[i] = p[10 + i]; }
            g_jig_hs.model[8] = 0; g_jig_hs.version[4] = 0;
        }
        break;

    case JIG_SUB_START:                     /* 执行应答（0x02/0x03 共用） */
    case JIG_SUB_STOP:
        g_jig_ack     = p[1];
        g_jig_ack_new = 1;
        break;

    case JIG_SUB_PROGRESS:                  /* 进度应答 */
        g_jig_prog.valid     = 1;
        g_jig_prog.step      = p[2];
        g_jig_prog.step_total= p[3];
        g_jig_prog.item_done = p[4];
        g_jig_prog.item_fail = p[5];
        break;

    case JIG_SUB_REALTIME:                  /* 实时数据应答 */
        g_jig_rt.valid     = 1;
        g_jig_rt.bus_vol   = rd_u16(&p[2]);
        g_jig_rt.total_cur = rd_s16(&p[4]);
        g_jig_rt.mos_temp  = rd_s16(&p[6]);
        g_jig_rt.cpu_vol   = rd_u16(&p[8]);
        g_jig_rt.commu_cnt = p[10];
        g_jig_rt.sc_state  = p[11];
        g_jig_rt.warn      = rd_u16(&p[12]);
        break;

    case JIG_RPT_ITEM: {                    /* 单项测试结果上报 */
        u8 idx = p[1];
        if (idx < JIG_ITEM_NUM) {
            g_jig_items[idx].valid = 1;
            g_jig_items[idx].value = rd_s16(&p[2]);
            g_jig_items[idx].pass  = p[4];
            g_jig_items[idx].thr_l = rd_s16(&p[5]);
            g_jig_items[idx].thr_h = rd_s16(&p[7]);
        }
        break;
    }

    case JIG_RPT_FINISH:                    /* 自检完成上报 */
        g_jig_fin.valid    = 1;
        g_jig_fin.total    = p[1];
        g_jig_fin.pass_cnt = p[2];
        g_jig_fin.fail_cnt = p[3];
        g_jig_fin.elapsed  = rd_u16(&p[4]);
        g_jig_fin.result   = p[6];
        break;

    default:
        break;
    }
}

/* 主板现成周期包/报警包解析（用于界面实时显示，不自检期间才有） */
static void handle_legacy(u8 cmd, const u8 *p)
{
    switch (cmd) {
    case 0xA9u:    /* 周期包：电压/速度/电流/温度，按对接笔记 §3 布局 */
        g_jig_rt.valid     = 1;
        g_jig_rt.bus_vol   = rd_u16(&p[0]);   /* 0.01V */
        g_jig_rt.total_cur = rd_s16(&p[4]);   /* 0.01A */
        g_jig_rt.mos_temp  = rd_s16(&p[8]);   /* 0.01℃ */
        break;
    case 0xF5u:    /* 报警包 */
        g_jig_rt.warn = rd_u16(&p[0]);
        break;
    default:
        break;
    }
}

/* 帧流解析状态机：从环形缓冲取字节，找 AA 5A ... 5A 55，校验后分发 */
void jig_proto_poll(void)
{
    static u8  frame[JIG_FRAME_LEN];
    static u8  idx = 0;
    u8  b;

    while (drv_rx_read(&g_rx_mb, &b, 1) == 1) {
        /* 帧头同步 */
        if (idx == 0 && b != 0xAAu) { continue; }
        if (idx == 1 && b != 0x5Au) { idx = (b == 0xAAu) ? 1 : 0; continue; }

        frame[idx++] = b;
        if (idx < JIG_FRAME_LEN) { continue; }
        idx = 0;

        /* 完整一帧：帧尾 + 校验 */
        if (frame[18] != 0x5Au || frame[19] != 0x55u) { continue; }
        if (frame[17] != chk_sum(frame))              { continue; }

        g_jig_online = 1;
        if (frame[16] == JIG_CMD_FROM_DUT) {
            handle_c9(&frame[2]);
        } else {
            handle_legacy(frame[16], &frame[2]);
        }
    }
}

void jig_proto_init(void)
{
    u8 i;
    for (i = 0; i < JIG_ITEM_NUM; i++) {
        g_jig_items[i].valid = 0;
        g_jig_items[i].pass  = 0;
        g_jig_items[i].value = 0;
        g_jig_items[i].thr_l = 0;
        g_jig_items[i].thr_h = 0;
    }
    g_jig_hs.ok = 0;
    g_jig_prog.valid = 0;
    g_jig_rt.valid   = 0;
    g_jig_fin.valid  = 0;
    g_jig_ack     = 0;
    g_jig_ack_new = 0;
    g_jig_online  = 0;
}
