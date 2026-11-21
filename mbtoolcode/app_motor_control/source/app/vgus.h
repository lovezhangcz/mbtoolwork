/**
  ******************************************************************************
  * @file    vgus.h
  * @brief   中显 VGUS 串口屏驱动（VTc070C34A，1024x600，USART3，115200 8N1）
  *          协议依据 doc/VGUS串口屏用户开发指南.pdf：
  *          - 帧头 A5 5A（出厂默认，无 CRC）
  *          - 0x80 写寄存器 / 0x81 读寄存器
  *          - 0x82 写变量存储器 / 0x83 读变量存储器（触摸键值也走 0x83 上传）
  *          - 寄存器 0x00=版本  0x01=背光  0x02=蜂鸣(10ms)
  *            0x03~0x04=页面ID（写=切页）
  *
  *  变量地址表（VP 分配）在屏幕 UI 设计完成后填入下方 "VP 地址表" 区，
  *  并在 VGUS 开发工具中按同一表摆放控件（详见 design/ 屏幕 UI 方案）。
  ******************************************************************************
  */

#ifndef __VGUS_H
#define __VGUS_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "my_type.h"

/* ================= VP 变量地址表（UI 方案 v1.0 定稿） =================
 * 与 design/屏幕UI方案_v1.0.md 第 3 章一致，VGUS 工具按同一表摆控件。
 * 0x0100 起为显示变量区，0x0200 起为触摸键值区。                     */
#define VP_STATE_TEXT     0x0100   /* 当前状态/提示文本，文本变量，最长 32 字(64B) */
#define VP_BUS_VOL        0x0140   /* 治具采样-母线电压，数据变量，1 字（单位 0.01V） */
#define VP_V12            0x0141   /* 12V  电压，1 字（mV） */
#define VP_V5             0x0142   /* 5V   电压，1 字（mV） */
#define VP_V3V3           0x0143   /* 3.3V 电压，1 字（mV） */
#define VP_TOTAL_CUR      0x0144   /* 主板回读-总电流，1 字（0.01A，有符号） */
#define VP_MOS_TEMP       0x0145   /* 主板回读-MOS 温度，1 字（0.01℃，有符号） */
#define VP_PROGRESS       0x0146   /* 测试进度，1 字（0~100 %，进度条同绑） */
/* ---- 以下为主板回读/计数/结论区（UI 方案 v1.0 新增） ---- */
#define VP_DUT_BUSVOL     0x0150   /* 主板回读-母线电压，1 字（0.01V） */
#define VP_DUT_CPUVOL     0x0151   /* 主板回读-CPU 电压，1 字（mV） */
#define VP_DUT_COMMU      0x0152   /* 主板回读-遥控通讯计数，1 字（整数） */
#define VP_DUT_WARN       0x01E0   /* 主板回读-报警字，文本变量 8 字（固件写 HEX 字符串，如 "0x0041"） */
#define VP_CNT_DONE       0x0154   /* 已测数量，1 字 */
#define VP_CNT_PASS       0x0155   /* 通过数量，1 字 */
#define VP_CNT_FAIL       0x0156   /* 失败数量，1 字 */
#define VP_RESULT_ICON    0x0157   /* 总结论图标变量：0=未测 1=PASS 2=FAIL（96x96 图标库） */
#define VP_ELAPSED        0x0158   /* 测试耗时，1 字（0.1s，1 位小数） */
#define VP_DUT_MODEL      0x0160   /* 被测主板机型/版本，文本变量，8 字（主页+设置页共用） */
#define VP_JIG_VER        0x0168   /* 治具固件版本，文本变量，8 字（设置页） */
#define VP_BL_VALUE       0x0170   /* 背光当前值回显，1 字（0~64，设置页） */
#define VP_ITEM_BASE      0x0180   /* 34 个测试项结果，每项 2 字(值 s16 + 判定 0/1/2)，共 68 字 */
/* 触摸键值上传区（0x83 上传，按钮"按键值返回"控件指向） */
#define VP_TOUCH_KEY      0x0200   /* 触摸按钮键值，1 字 */

/* 触摸键值定义（屏按钮"键值"字段，详见 design/屏幕UI方案_v1.0.md 第 4 章） */
#define VGUS_KEY_START      0x0001   /* 启动测试（等同实体 KEY1） */
#define VGUS_KEY_RETEST     0x0002   /* 重新测试（等同实体 KEY2） */
#define VGUS_KEY_STOP       0x0003   /* 停止 */
#define VGUS_KEY_DETAIL     0x0004   /* 主页->明细页 */
#define VGUS_KEY_BACK       0x0005   /* 返回主页 */
#define VGUS_KEY_SETTING    0x0010   /* 进设置页（实体 KEY3 长按等效） */
#define VGUS_KEY_BL_UP      0x0011   /* 背光+ */
#define VGUS_KEY_BL_DOWN    0x0012   /* 背光- */
#define VGUS_KEY_BUZZ_TEST  0x0013   /* 屏蜂鸣器测试 */
#define VGUS_KEY_DUT_BUZZ   0x0014   /* 被测板蜂鸣 */

/* 页面 ID（与 VGUS 工程页面编号一致，UI 设计后填写） */
#define VGUS_PAGE_MAIN    0x0000   /* 主测试界面 */
#define VGUS_PAGE_RESULT  0x0001   /* 结果界面 */
#define VGUS_PAGE_SET     0x0002   /* 设置界面（预留） */

/* ================= 触摸事件 ================= */
typedef struct {
    volatile u8  new_flag;   /* 有新触摸键值时置 1，读后清 0 */
    u16 vp;                  /* 上传的变量地址 */
    u16 key;                 /* 键值 */
} vgus_touch_t;
extern vgus_touch_t g_vgus_touch;

/* ================= 接口 ================= */
void vgus_init(void);                 /* 串口已由 usart_mo_init 完成，此处清状态 */
void vgus_poll(void);                 /* 解析屏返回（触摸键值/读应答），任务周期调用 */

/* 寄存器操作（0x80/0x81） */
void vgus_set_page(u16 page_id);      /* 切换页面 */
void vgus_buzzer(u8 time_10ms);       /* 屏蜂鸣器，单位 10ms */
void vgus_set_backlight(u8 level);    /* 背光 0x00~0x40 */
void vgus_read_version(void);         /* 读屏版本，结果在 g_vgus_ver */
extern volatile u8 g_vgus_ver;        /* 0=未读到，否则 BCD 版本号 */

/* 变量写操作（0x82） */
void vgus_write_u16(u16 vp, u16 val);
void vgus_write_s16(u16 vp, s16 val);
void vgus_write_words(u16 vp, const u16 *data, u8 nword);
/* 文本变量：自动转 GBK 发送，结尾补 0xFFFF 结束符；maxlen=控件字长度 */
void vgus_write_text(u16 vp, const char *gbk_str, u8 maxlen);

#ifdef __cplusplus
}
#endif

#endif /* __VGUS_H */
