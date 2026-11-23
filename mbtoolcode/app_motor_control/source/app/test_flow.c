/**
  ******************************************************************************
  * @file    test_flow.c
  * @brief   测试流程状态机实现（10ms tick，非阻塞）
  *
  *  门限与超时集中在本文件顶部：
  *  ★ 标注 TODO(门限) 的数值为占位初值，待用户确认后修改（见工作笔记 §7）
  ******************************************************************************
  */

#include "test_flow.h"
#include "board.h"
#include "ads131.h"
#include "jig_proto.h"
#include "vgus.h"

/* ================= 门限（TODO(门限)：占位初值，待确认） ================= */
/* 治具 ADC 侧电压窗口，单位 mV */
#define V_BUS_MIN     33000    /* TODO(门限) 10串锂电 33.0V */
#define V_BUS_MAX     42500    /* TODO(门限) 满电 42.5V */
#define V_12_MIN      11400
#define V_12_MAX      12600
#define V_5_MIN        4750
#define V_5_MAX        5250
#define V_3V3_MIN      3135
#define V_3V3_MAX      3465
#define V_SAFE_MV      5000    /* 放电完成安全电压 */

/* 主板自采数据窗口（协议 §4.3） */
#define DUT_VOL_MIN    2000    /* 0.01V */
#define DUT_VOL_MAX    4600
#define DUT_CPU_MIN    3000    /* mV */
#define DUT_CPU_MAX    3600

/* 超时（10ms tick 数） */
#define TMO_SELF_CHECK   300   /* 3s   */
#define TMO_PRE_CHECK    200   /* 2s   */
#define TMO_POWER_ON     500   /* 5s 等电压建立 */
#define TMO_BOOT_DUT    1000   /* 10s 等被测板启动 */
#define TMO_VOLTAGE     200    /* 2s   */
#define TMO_COMM_RETRY     3   /* 握手重试次数 */
#define TMO_COMM_ONCE    100   /* 单次 1s */
#define TMO_DATA_ACQ     100   /* 每项 1s */
#define TMO_MOTOR_ALL   3000   /* 整套自检 30s */
#define TMO_MOTOR_STALL  300   /* 3s 无新结果判卡死 */
#define TMO_DISCHG       500   /* 放电 5s */
#define TMO_POWERKEY      50   /* 模拟开机键按下 500ms */

/* ================= 内部状态 ================= */
static tf_state_e s_state;
static u16        s_tick;        /* 当前状态已停留 tick 数 */
static u8         s_retry;       /* 通用重试计数 */
static s32        s_mv[4];       /* 最近四路电压 mV */
static u8         s_volt_done;   /* 电压检查已执行 */
static u8         s_key1_evt, s_key2_evt;  /* 按键事件（消抖后） */
static u32        s_elapse_10ms;           /* 本轮用时（10ms，PRE_CHECK 起 MOTOR_TEST 止） */
static u16        s_cnt_done, s_cnt_pass, s_cnt_fail;  /* 产量计数（RAM，TODO(持久化)：可存 BKP/Flash） */

tf_fail_t g_tf_fail;

/* 蜂鸣器非阻塞鸣叫模式 */
static u8  s_bz_on, s_bz_off, s_bz_times, s_bz_phase;
static u16 s_bz_cnt;

static void tf_goto(tf_state_e st)
{
    s_state = st;
    s_tick  = 0;
    s_retry = 0;
    vgus_write_text(VP_STATE_TEXT, test_flow_state_text(), 32);
    vgus_write_u16(VP_PROGRESS, test_flow_progress());
}

static void tf_fail_log(tf_state_e st, u8 item, s16 value)
{
    if (g_tf_fail.cnt < TF_FAIL_MAX) {
        g_tf_fail.state[g_tf_fail.cnt] = (u8)st;
        g_tf_fail.item[g_tf_fail.cnt]  = item;
        g_tf_fail.value[g_tf_fail.cnt] = value;
        g_tf_fail.cnt++;
    }
}

/* ---------------- 按键扫描（10ms，低有效，20ms 消抖） ---------------- */
static void key_scan(void)
{
    static u8 cnt[3], stable[3] = {0, 0, 0};
    u8 i, raw;

    for (i = 0; i < 3; i++) {
        raw = key_read((u8)(i + 1));
        if (raw != stable[i]) {
            if (++cnt[i] >= 2) {
                stable[i] = raw;
                cnt[i] = 0;
                if (raw) {                 /* 按下沿 */
                    if (i == 0) { s_key1_evt = 1; }
                    if (i == 1) { s_key2_evt = 1; }
                }
            }
        } else {
            cnt[i] = 0;
        }
    }
}

/* ---------------- 蜂鸣器模式（10ms 调度） ---------------- */
static void buzz_set(u8 on_10ms, u8 off_10ms, u8 times)
{
    s_bz_on = on_10ms; s_bz_off = off_10ms;
    s_bz_times = times; s_bz_phase = 0; s_bz_cnt = 0;
}
static void buzz_run(void)
{
    if (s_bz_times == 0) { buzzer_off(); return; }
    if (++s_bz_cnt >= (s_bz_phase ? s_bz_off : s_bz_on)) {
        s_bz_cnt = 0;
        if (s_bz_phase) {
            s_bz_phase = 0;
            if (--s_bz_times == 0) { buzzer_off(); return; }
        } else {
            s_bz_phase = 1;
        }
    }
    if (s_bz_phase) { buzzer_off(); } else { buzzer_on(); }
}

/* ================= 对外接口 ================= */
tf_state_e test_flow_state(void) { return s_state; }

const char *test_flow_state_text(void)
{
    switch (s_state) {
    case TF_SELF_CHECK:    return "治具自检中...";
    case TF_SELF_ERR:      return "治具自检失败！";
    case TF_WAIT_BOARD:    return "请放置主板，按【启动】";
    case TF_PRE_CHECK:     return "上电前检查...";
    case TF_POWER_ON:      return "主板通电中...";
    case TF_BOOT_DUT:      return "主板启动中...";
    case TF_VOLTAGE_TEST:  return "电压检查中...";
    case TF_COMM_TEST:     return "通信握手中...";
    case TF_DATA_ACQ:      return "读取主板数据...";
    case TF_MOTOR_TEST:    return "电机测试中，请勿触碰";
    case TF_RESULT:        return g_tf_fail.cnt ? "测试完成：不合格" : "测试完成：合格";
    case TF_POWER_OFF:     return "断电放电中...";
    default:               return "";
    }
}

u8 test_flow_progress(void)
{
    u8 base = (u8)((u16)s_state * 100 / TF_STATE_NUM);
    if (s_state == TF_MOTOR_TEST && g_jig_prog.valid && g_jig_prog.item_done <= JIG_ITEM_NUM) {
        base = (u8)(70 + (u16)g_jig_prog.item_done * 20 / JIG_ITEM_NUM);
    }
    return base;
}

/* ---------------- 辅助：版本文本 / 机型文本 / HEX 报警字 ---------------- */
#define TF_JIG_VER_TEXT   "JIG-1.0.0"      /* 治具固件版本（设置页 0x0168 显示） */

static void tf_hex4(char *out, u16 v)      /* "0xXXXX" 6 字符 + \0 */
{
    static const char hd[] = "0123456789ABCDEF";
    out[0] = '0'; out[1] = 'x';
    out[2] = hd[(v >> 12) & 0xFu]; out[3] = hd[(v >> 8) & 0xFu];
    out[4] = hd[(v >> 4) & 0xFu];  out[5] = hd[v & 0xFu];
    out[6] = 0;
}

static void tf_model_text(char *out)       /* "NNNNNNNN vVVV"，<=15B，握手成功后调用 */
{
    u8 n = 0, i;
    for (i = 0; i < 8 && g_jig_hs.model[i] && n < 11; i++) { out[n++] = (char)g_jig_hs.model[i]; }
    out[n++] = ' '; out[n++] = 'v';
    for (i = 0; i < 4 && g_jig_hs.version[i] && n < 15; i++) { out[n++] = (char)g_jig_hs.version[i]; }
    out[n] = 0;
}

/* ---------------- 辅助：34 项实时上屏标记（位掩码，0~31 在 lo，32~33 在 hi） ---------------- */
static unsigned long s_items_sent_lo, s_items_sent_hi;

static u8 tf_item_sent(u8 i)
{
    return (i < 32) ? (u8)((s_items_sent_lo >> i) & 1ul)
                    : (u8)((s_items_sent_hi >> (i - 32)) & 1ul);
}
static void tf_item_mark(u8 i)
{
    if (i < 32) { s_items_sent_lo |= (1ul << i); }
    else        { s_items_sent_hi |= (1ul << (i - 32)); }
}

/* 新一轮测试开始：清回读区 / 34 项 / 结论图标 / 进度 / 耗时 / 报警字 */
static void ui_board_reset(void)
{
    u8 i;
    s_elapse_10ms = 0;
    s_items_sent_lo = 0; s_items_sent_hi = 0;
    vgus_write_u16(VP_PROGRESS, 0);
    vgus_write_u16(VP_RESULT_ICON, 0);
    vgus_write_u16(VP_TOTAL_CUR, 0);
    vgus_write_u16(VP_MOS_TEMP, 0);
    vgus_write_u16(VP_DUT_BUSVOL, 0);
    vgus_write_u16(VP_DUT_CPUVOL, 0);
    vgus_write_u16(VP_DUT_COMMU, 0);
    vgus_write_u16(VP_ELAPSED, 0);
    vgus_write_text(VP_DUT_WARN, "----", 16);
    for (i = 0; i < JIG_ITEM_NUM; i += 8) {          /* 68 字分 5 帧清零 */
        u16 w[16];
        u8  k, n = 0;
        for (k = 0; k < 8 && (i + k) < JIG_ITEM_NUM; k++) { w[n++] = 0; w[n++] = 0; }
        vgus_write_words((u16)(VP_ITEM_BASE + i * 2), w, n);
    }
}

/* ================= 各状态处理 ================= */
static void st_self_check(void)
{
    u16 id = 0;

    if (s_tick == 1) {
        u8 ok_adc, ok_scr;
        ok_adc = ads131_init(&id);
        vgus_read_version();
        vgus_write_text(VP_JIG_VER, TF_JIG_VER_TEXT, 16);   /* 设置页-治具版本 */
        vgus_write_u16(VP_BL_VALUE, 0x40u);                 /* 背光默认最大，回显设置页 */
        /* 屏握手：100ms 内应收到版本应答；此处先标记，tick 后检查 */
        ok_scr = 1;
        buzz_set(10, 5, 1);                      /* 短响一声 */
        key_led(1, 1); key_led(2, 1); key_led(3, 1);
        if (!ok_adc) { tf_fail_log(s_state, 0xFF, (s16)id); }
        s_retry = ok_adc ? 1 : 0;
        (void)ok_scr;
    }
    if (s_tick >= 30) {                          /* 300ms 后判定 */
        key_led(1, 0); key_led(2, 0); key_led(3, 0);
        if (s_retry && g_vgus_ver) {
            tf_goto(TF_WAIT_BOARD);
        } else {
            tf_goto(TF_SELF_ERR);                /* ADC 或屏通信失败 */
        }
    } else if (s_tick >= TMO_SELF_CHECK) {
        tf_goto(TF_SELF_ERR);
    }
}

static void st_self_err(void)
{
    /* 蜂鸣器长鸣 + 运行灯快闪，等待人工断电 */
    if ((s_tick % 50) == 0) { runled_toggle(); }
    if (s_tick == 1) { buzzer_on(); }
}

static void st_wait_board(void)
{
    if ((s_tick % 50) == 0) { key_led(1, (s_tick / 50) & 1); }   /* KEY1 灯慢闪 */
    if (s_key1_evt) {
        s_key1_evt = 0;
        key_led(1, 1);
        jig_proto_init();                        /* 清上一轮缓存 */
        g_tf_fail.cnt = 0;
        ui_board_reset();                        /* 清屏上本轮数据区 */
        tf_goto(TF_PRE_CHECK);
    }
}

static void st_pre_check(void)
{
    /* 母线残压必须低于安全电压，说明放电回路正常、板上无带电 */
    if (s_tick == 1) {
        if (!ads131_read_mv(s_mv)) { tf_fail_log(s_state, 0xFF, -1); }
    }
    if (s_tick >= 20) {
        if (s_mv[ADS131_CH_BUS] < V_SAFE_MV) {
            tf_goto(TF_POWER_ON);
        } else {
            tf_fail_log(s_state, 0xFE, (s16)(s_mv[ADS131_CH_BUS] / 10));
            tf_goto(TF_SELF_ERR);                /* 安全类失败：停机 */
        }
    }
    if (s_tick >= TMO_PRE_CHECK) { tf_goto(TF_SELF_ERR); }
}

static void st_power_on(void)
{
    if (s_tick == 1) {
        act_mb_power(1);                         /* 继电器吸合 */
    }
    if (s_tick >= 100) {                         /* 1s 电压建立 */
        if (ads131_read_mv(s_mv) && s_mv[ADS131_CH_BUS] > V_BUS_MIN) {
            tf_goto(TF_BOOT_DUT);
        } else {
            tf_fail_log(s_state, 0xFE, (s16)(s_mv[ADS131_CH_BUS] / 10));
            tf_goto(TF_POWER_OFF);               /* 通电失败，安全收尾 */
        }
    }
    if (s_tick >= TMO_POWER_ON) { tf_goto(TF_POWER_OFF); }
}

static void st_boot_dut(void)
{
    if (s_tick == 1)            { act_powerkey(1); }    /* 按下开机键 */
    if (s_tick == TMO_POWERKEY) { act_powerkey(0); }    /* 500ms 后松开 */
    if (act_dut_online()) {                              /* 检测到已启动 */
        tf_goto(TF_VOLTAGE_TEST);
    }
    if (s_tick >= TMO_BOOT_DUT) {
        tf_fail_log(s_state, 0xFE, -2);
        tf_goto(TF_POWER_OFF);
    }
}

/* 四路电压判定，返回失败项数（判定类失败：记录后继续） */
static u8 volt_check_once(void)
{
    static const s32 vmin[4] = { V_BUS_MIN, V_12_MIN, V_5_MIN, V_3V3_MIN };
    static const s32 vmax[4] = { V_BUS_MAX, V_12_MAX, V_5_MAX, V_3V3_MAX };
    u8 i, fails = 0;

    if (!ads131_read_mv(s_mv)) {
        tf_fail_log(TF_VOLTAGE_TEST, 0xFF, -1);
        return 1;
    }
    for (i = 0; i < 4; i++) {
        if (s_mv[i] < vmin[i] || s_mv[i] > vmax[i]) {
            tf_fail_log(TF_VOLTAGE_TEST, i, (s16)(s_mv[i] / 10));
            fails++;
        }
    }
    /* 屏幕刷新四路电压 */
    vgus_write_u16(VP_BUS_VOL, (u16)(s_mv[ADS131_CH_BUS] / 10));   /* 0.01V */
    vgus_write_u16(VP_V12,  (u16)s_mv[ADS131_CH_12V]);
    vgus_write_u16(VP_V5,   (u16)s_mv[ADS131_CH_5V]);
    vgus_write_u16(VP_V3V3, (u16)s_mv[ADS131_CH_3V3]);
    return fails;
}

static void st_voltage_test(void)
{
    if (s_tick == 1) { s_volt_done = 1; (void)volt_check_once(); }
    if (s_tick >= 20) { tf_goto(TF_COMM_TEST); }         /* 200ms 完成 */
    if (s_tick >= TMO_VOLTAGE) { tf_goto(TF_COMM_TEST); }
}

static void st_comm_test(void)
{
    if (s_tick == 1) { jig_send_handshake(); }
    if (g_jig_hs.ok) {
        char mt[18];
        tf_model_text(mt);
        vgus_write_text(VP_DUT_MODEL, mt, 16);           /* 主页+设置页 机型 */
        tf_goto(TF_DATA_ACQ);
        return;
    }
    if (s_tick >= TMO_COMM_ONCE) {                       /* 1s 无应答重发 */
        if (++s_retry >= TMO_COMM_RETRY) {
            tf_fail_log(s_state, 0xFF, -3);              /* 通信失败：后续依赖项未测 */
            tf_goto(TF_RESULT);                          /* 跳 RESULT，见方案 §3.2 */
        } else {
            s_tick = 0;
            jig_send_handshake();
        }
    }
}

static void st_data_acq(void)
{
    if (s_tick == 1) { jig_send_realtime(); }
    if (g_jig_rt.valid) {
        /* 电压窗口与报警字判定 */
        if (g_jig_rt.bus_vol < DUT_VOL_MIN || g_jig_rt.bus_vol > DUT_VOL_MAX) {
            tf_fail_log(s_state, 26, (s16)g_jig_rt.bus_vol);
        }
        if (g_jig_rt.cpu_vol < DUT_CPU_MIN || g_jig_rt.cpu_vol > DUT_CPU_MAX) {
            tf_fail_log(s_state, 32, (s16)g_jig_rt.cpu_vol);
        }
        if (g_jig_rt.warn != 0) {
            tf_fail_log(s_state, 0xFD, (s16)g_jig_rt.warn);   /* 报警字置位 */
        }
        vgus_write_s16(VP_TOTAL_CUR, g_jig_rt.total_cur);
        vgus_write_s16(VP_MOS_TEMP,  g_jig_rt.mos_temp);
        vgus_write_u16(VP_DUT_BUSVOL, g_jig_rt.bus_vol);
        vgus_write_u16(VP_DUT_CPUVOL, g_jig_rt.cpu_vol);
        vgus_write_u16(VP_DUT_COMMU,  g_jig_rt.commu_cnt);
        {
            char hx[8];
            tf_hex4(hx, g_jig_rt.warn);
            vgus_write_text(VP_DUT_WARN, hx, 16);        /* 报警字 HEX 文本，如 0x0041 */
        }
        tf_goto(TF_MOTOR_TEST);
    }
    if (s_tick >= TMO_DATA_ACQ) {
        tf_fail_log(s_state, 0xFF, -4);
        tf_goto(TF_MOTOR_TEST);                          /* 判定类：继续 */
    }
}

static void st_motor_test(void)
{
    static u8 last_done;

    if (s_tick == 1) {
        last_done = 0;
        jig_send_start();                                /* 0x02 启动整套自检 */
    }
    if ((s_tick % 20) == 0) {
        jig_send_progress();                             /* 200ms 轮询进度 */
        vgus_write_u16(VP_PROGRESS, test_flow_progress());
    }
    /* 单项结果实时上屏：值 + 判定（0 未测 / 1 过 / 2 败，对应 40.ICO 图标 0~2） */
    {
        u8 i;
        for (i = 0; i < JIG_ITEM_NUM; i++) {
            if (g_jig_items[i].valid && !tf_item_sent(i)) {
                u16 w[2];
                w[0] = (u16)g_jig_items[i].value;
                w[1] = g_jig_items[i].pass ? 1u : 2u;
                vgus_write_words((u16)(VP_ITEM_BASE + i * 2), w, 2);
                tf_item_mark(i);
            }
        }
    }

    /* 完成帧到达 → 汇总 */
    if (g_jig_fin.valid) {
        u8 i;
        for (i = 0; i < JIG_ITEM_NUM; i++) {
            if (g_jig_items[i].valid && !g_jig_items[i].pass) {
                tf_fail_log(s_state, i, g_jig_items[i].value);
            }
        }
        tf_goto(TF_RESULT);
        return;
    }
    /* 卡死检测：3s 无新结果项 */
    if (g_jig_prog.valid && g_jig_prog.item_done != last_done) {
        last_done = g_jig_prog.item_done;
        s_retry = 0;
    } else if (++s_retry >= TMO_MOTOR_STALL) {
        jig_send_stop();                                 /* 急停兜底 */
        tf_fail_log(s_state, 0xFF, -5);
        tf_goto(TF_RESULT);
        return;
    }
    if (s_tick >= TMO_MOTOR_ALL) {                       /* 总超时 */
        jig_send_stop();
        tf_fail_log(s_state, 0xFF, -6);
        tf_goto(TF_RESULT);
    }
}

static void st_result(void)
{
    if (s_tick == 1) {
        u8 i;
        u8 pass_all = (u8)(g_tf_fail.cnt == 0);
        /* 34 项兜底整块上屏：每项 2 字（值，判定 0 未测/1 过/2 败），补齐实时未刷项 */
        for (i = 0; i < JIG_ITEM_NUM; i += 8) {          /* 分块写，单帧 ≤16 字 */
            u16 w[16];
            u8  k, n = 0;
            for (k = 0; k < 8 && (i + k) < JIG_ITEM_NUM; k++) {
                w[n++] = (u16)g_jig_items[i + k].value;
                w[n++] = g_jig_items[i + k].valid ? (u16)(g_jig_items[i + k].pass ? 1u : 2u) : 0u;
            }
            vgus_write_words((u16)(VP_ITEM_BASE + i * 2), w, n);
        }
        /* 计数 / 总结论图标 / 耗时 */
        s_cnt_done++;
        if (pass_all) { s_cnt_pass++; } else { s_cnt_fail++; }
        vgus_write_u16(VP_CNT_DONE, s_cnt_done);
        vgus_write_u16(VP_CNT_PASS, s_cnt_pass);
        vgus_write_u16(VP_CNT_FAIL, s_cnt_fail);
        vgus_write_u16(VP_RESULT_ICON, pass_all ? 1u : 2u);
        vgus_write_u16(VP_ELAPSED, (u16)(s_elapse_10ms / 10u));   /* 0.1s */
        if (g_tf_fail.cnt) {
            buzz_set(10, 10, 3);                         /* FAIL：三短声 */
            key_led(2, 1); key_led(1, 0);
        } else {
            buzz_set(50, 0, 1);                          /* PASS：一长声 */
            key_led(1, 1); key_led(2, 0);
        }
    }
    if (s_key1_evt || s_key2_evt) {                      /* 任意键结束 */
        s_key1_evt = s_key2_evt = 0;
        tf_goto(TF_POWER_OFF);
    }
}

static void st_power_off(void)
{
    if (s_tick == 1) {
        act_powerkey(0);
        act_mb_power(0);                                 /* 断继电器 */
        vgus_set_page(VGUS_PAGE_MAIN);
    }
    /* 放电确认：母线 < 安全电压（ACT_DISCHG 由硬件自动放电，MCU 回读） */
    if ((s_tick % 50) == 0) {
        (void)ads131_read_mv(s_mv);
        if (s_mv[ADS131_CH_BUS] < V_SAFE_MV) {
            tf_goto(TF_WAIT_BOARD);
            return;
        }
    }
    if (s_tick >= TMO_DISCHG) {                          /* 放电超时：提示人工 */
        tf_fail_log(s_state, 0xFE, (s16)(s_mv[ADS131_CH_BUS] / 10));
        buzz_set(10, 10, 5);
        tf_goto(TF_WAIT_BOARD);                          /* 提示文案由屏区分 */
    }
}

/* ================= 主入口 ================= */
void test_flow_key(u8 idx)
{
    if (idx == 1)      { s_key1_evt = 1; }
    else if (idx == 2) { s_key2_evt = 1; }
}

void test_flow_init(void)
{
    s_state = TF_SELF_CHECK;
    s_tick = 0; s_retry = 0;
    s_volt_done = 0;
    s_key1_evt = s_key2_evt = 0;
    g_tf_fail.cnt = 0;
    s_bz_times = 0;
}

void test_flow_run(void)
{
    key_scan();
    buzz_run();

    /* 全局取消：KEY2 在测试中途按下 → 安全收尾（RESULT/POWER_OFF 除外） */
    if (s_key2_evt &&
        s_state != TF_RESULT && s_state != TF_POWER_OFF &&
        s_state != TF_SELF_ERR && s_state != TF_WAIT_BOARD) {
        s_key2_evt = 0;
        jig_send_stop();                                 /* 电机急停 */
        tf_goto(TF_POWER_OFF);
        return;
    }

    switch (s_state) {
    case TF_SELF_CHECK:   st_self_check();   break;
    case TF_SELF_ERR:     st_self_err();     break;
    case TF_WAIT_BOARD:   st_wait_board();   break;
    case TF_PRE_CHECK:    st_pre_check();    break;
    case TF_POWER_ON:     st_power_on();     break;
    case TF_BOOT_DUT:     st_boot_dut();     break;
    case TF_VOLTAGE_TEST: st_voltage_test(); break;
    case TF_COMM_TEST:    st_comm_test();    break;
    case TF_DATA_ACQ:     st_data_acq();     break;
    case TF_MOTOR_TEST:   st_motor_test();   break;
    case TF_RESULT:       st_result();       break;
    case TF_POWER_OFF:    st_power_off();    break;
    default:              tf_goto(TF_SELF_CHECK); break;
    }

    if (s_state >= TF_PRE_CHECK && s_state <= TF_MOTOR_TEST) {
        s_elapse_10ms++;                                 /* 本轮计时（RESULT 冻结显示） */
    }
    s_tick++;
    runled_toggle();                                     /* 运行灯心跳 10Hz→5Hz 闪烁 */
}
