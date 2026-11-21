/**
  ******************************************************************************
  * @file    test_flow.h
  * @brief   测试流程状态机（依据 design/测试方案与接口定义_v0.2.md §3）
  *
  *  状态序列：
  *    SELF_CHECK → WAIT_BOARD → PRE_CHECK → POWER_ON → BOOT_DUT
  *    → VOLTAGE_TEST → COMM_TEST → DATA_ACQ → MOTOR_TEST
  *    → RESULT → POWER_OFF → WAIT_BOARD ...
  *  任意状态 KEY2(取消) → POWER_OFF；自检失败 → SELF_ERR 停机
  *
  *  由 CMotionTask 以 10ms 周期调用 test_flow_run()。
  ******************************************************************************
  */

#ifndef __TEST_FLOW_H
#define __TEST_FLOW_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "my_type.h"

typedef enum {
    TF_SELF_CHECK = 0,   /* 治具自检 */
    TF_SELF_ERR,         /* 自检失败停机 */
    TF_WAIT_BOARD,       /* 等待放板，KEY1 启动 */
    TF_PRE_CHECK,        /* 上电前检查（母线残压/放电回路） */
    TF_POWER_ON,         /* 继电器吸合，给被测板通电 */
    TF_BOOT_DUT,         /* 模拟开机键，启动被测板 */
    TF_VOLTAGE_TEST,     /* 治具侧四路电压检查 */
    TF_COMM_TEST,        /* 串口握手+版本读取 */
    TF_DATA_ACQ,         /* 读主板自采数据 */
    TF_MOTOR_TEST,       /* 电机实测（主板 selfcheck 整套） */
    TF_RESULT,           /* 结果显示 */
    TF_POWER_OFF,        /* 断电+放电 */
    TF_STATE_NUM
} tf_state_e;

/* 判定类失败记录（判定失败不终止，记录后继续） */
#define TF_FAIL_MAX   16
typedef struct {
    u8  cnt;
    u8  state[TF_FAIL_MAX];      /* 失败发生时的状态 */
    u8  item[TF_FAIL_MAX];       /* 相关测试项索引（0xFF=与测试项无关） */
    s16 value[TF_FAIL_MAX];      /* 实测值 */
} tf_fail_t;
extern tf_fail_t g_tf_fail;

void        test_flow_init(void);
void        test_flow_run(void);          /* 10ms 周期 */
tf_state_e  test_flow_state(void);
u8          test_flow_progress(void);     /* 0~100，供进度条 */
const char *test_flow_state_text(void);   /* 当前状态中文文本 */

#ifdef __cplusplus
}
#endif

#endif /* __TEST_FLOW_H */
