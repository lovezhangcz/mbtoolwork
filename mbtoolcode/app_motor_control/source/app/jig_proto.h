/**
  ******************************************************************************
  * @file    jig_proto.h
  * @brief   治具 <-> 被测主板 JIG_PROTO V1.0 协议栈（治具侧）
  *          依据 design/治具对接协议_v1.0.md 实现：
  *          - 20 字节帧 AA 5A | 14B 数据 | CMD | sum^0xFF | 5A 55
  *          - 治具命令 CMD=0xC8，主板应答/上报 CMD=0xC9
  *          - 兼容解析主板现成周期包(0xA9)/报警包(0xF5)用于界面显示
  ******************************************************************************
  */

#ifndef __JIG_PROTO_H
#define __JIG_PROTO_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "my_type.h"

/* ---------------- 帧与命令常量 ---------------- */
#define JIG_FRAME_LEN      20
#define JIG_CMD_TO_DUT     0xC8u
#define JIG_CMD_FROM_DUT   0xC9u

/* 治具子命令（0xC8 数据区[2]） */
#define JIG_SUB_HANDSHAKE  0x01u   /* 握手/读版本 */
#define JIG_SUB_START      0x02u   /* 启动整套自检 */
#define JIG_SUB_STOP       0x03u   /* 停止自检/电机急停 */
#define JIG_SUB_PROGRESS   0x04u   /* 查询自检进度 */
#define JIG_SUB_REALTIME   0x10u   /* 读实时数据 */

/* 主板上报子命令（0xC9 数据区[2]） */
#define JIG_RPT_ITEM       0x20u   /* 单项测试结果 */
#define JIG_RPT_FINISH     0x22u   /* 自检完成 */

/* 主板现成命令（沿用） */
#define JIG_CMD_VER_REQ    0x9Bu   /* 读机型版本（回 0xBB 并开启周期上传） */
#define JIG_CMD_BEEP       0x88u   /* 被测板蜂鸣 */

#define JIG_ITEM_NUM       34      /* 测试项总数（索引 0~33） */

/* ---------------- 数据结构 ---------------- */

/* 单项测试结果缓存 */
typedef struct {
    u8  valid;      /* 收到该项上报后置 1 */
    u8  pass;       /* 1=PASS 0=FAIL */
    s16 value;      /* 实测值（主板原始单位） */
    s16 thr_l;      /* 下限 */
    s16 thr_h;      /* 上限 */
} jig_item_t;

/* 握手信息 */
typedef struct {
    u8  ok;                  /* 握手成功标志 */
    u8  model[9];            /* 机型码 ASCII 8 字节 + \0 */
    u8  version[5];          /* 版本 ASCII 4 字节 + \0 */
} jig_handshake_t;

/* 自检进度（0x04 应答） */
typedef struct {
    u8 valid;
    u8 step;        /* 当前步骤号 */
    u8 step_total;  /* 总步骤数 */
    u8 item_done;   /* 已出结果项数 */
    u8 item_fail;   /* 已失败项数 */
} jig_progress_t;

/* 实时数据（0x10 应答 / 或周期包解析回填） */
typedef struct {
    u8  valid;
    u16 bus_vol;     /* 母线电压 0.01V */
    s16 total_cur;   /* 总电流 0.01A */
    s16 mos_temp;    /* MOS 温度 0.01℃ */
    u16 cpu_vol;     /* CPU 电压 mV */
    u8  commu_cnt;   /* 遥控通信计数 */
    u8  sc_state;    /* 自检状态 0空闲 1进行 2完成 */
    u16 warn;        /* thiswarn 报警字 */
} jig_realtime_t;

/* 自检完成汇总（0x22 上报） */
typedef struct {
    u8  valid;
    u8  total;       /* 总测试项数 */
    u8  pass_cnt;
    u8  fail_cnt;
    u16 elapsed;     /* 0.1s */
    u8  result;      /* 1=PASS 0=FAIL */
} jig_finish_t;

/* 应答状态码（0x02/0x03 执行应答） */
typedef enum {
    JIG_ACK_NONE = 0,
    JIG_ACK_ACCEPT,        /* 0=接受 */
    JIG_ACK_REFUSE_SPEED,  /* 1=车速非零 */
    JIG_ACK_REFUSE_CHG,    /* 2=充电中 */
    JIG_ACK_REFUSE_BUSY,   /* 3=自检进行中 */
} jig_ack_e;

/* ---------------- 全局状态（CUsartCommTask 写，其余任务读） ---------------- */
extern jig_item_t      g_jig_items[JIG_ITEM_NUM];
extern jig_handshake_t g_jig_hs;
extern jig_progress_t  g_jig_prog;
extern jig_realtime_t  g_jig_rt;
extern jig_finish_t    g_jig_fin;
extern volatile u8     g_jig_ack;      /* 最近一次执行应答码，jig_ack_e */
extern volatile u8     g_jig_ack_new;  /* 收到新执行应答标志（读后清 0） */
extern volatile u8     g_jig_online;   /* 收到任何合法主板帧即置 1 */

/* ---------------- 接口 ---------------- */
void jig_proto_init(void);          /* 清缓存；测试流程重测时可再调用 */
void jig_proto_poll(void);          /* 从 MB 软串口收流并组帧解析，任务周期调用 */

/* 发送命令（内部走 usart_mb_send 查询式发送；返回恒 0） */
u8 jig_send_handshake(void);
u8 jig_send_start(void);
u8 jig_send_stop(void);
u8 jig_send_progress(void);
u8 jig_send_realtime(void);
u8 jig_send_ver_req(void);          /* 0x9B 读版本+开周期上传 */
u8 jig_send_beep(void);             /* 0x88 被测板蜂鸣 */

/* 测试项名称（治具侧对照表，供屏幕显示） */
const char *jig_item_name(u8 idx);

#ifdef __cplusplus
}
#endif

#endif /* __JIG_PROTO_H */
