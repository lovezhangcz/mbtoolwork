/**
  ******************************************************************************
  * @file    app_main.c
  * @brief   治具 APP 主文件（uCOS-II 六任务）
  *
  *  任务分工（TIM3 2kHz 时基邮箱触发，节拍见括号）：
  *    CSensorTask        2kHz 内部分频到 5Hz：空闲态下 ADC 电压实时上屏
  *    CUsartCommTask     20ms：jig_proto_poll() 主板串口协议解析
  *    CSystemSuperiseTask 100Hz：系统监控（预留）
  *    bluetoothtast      40Hz：vgus_poll() 屏幕触摸/应答解析
  *    CMotionTask        200Hz 分频到 100Hz：test_flow_run() 测试流程状态机
  *    spkworkingtast     100Hz：杂项（预留）
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "ucos_ii.h"
#include "cpu.h"
#include "my_type.h"
#include "bsp.h"
#include "tasttimebase.h"

#include "board.h"
#include "led.h"
#include "ads131.h"
#include "jig_proto.h"
#include "vgus.h"
#include "test_flow.h"

static OS_STK App_TaskStart_Stk[APP_TASKSTART_STK_SIZE];
static OS_STK App_Sensor_Sample_Stk[APP_SENSOR_SAMPLE_STK_SIZE];
static OS_STK App_MotionProcess_Stk[APP_MOTION_PROCESS_STK_SIZE];
static OS_STK App_SystemSuperise_Stk[APP_SYSTEMSUPERISE_STK_SIZE];
static OS_STK App_UsartProcess_Stk[APP_USART_PROCESS_STK_SIZE];
static OS_STK App_BTDataProcess_Stk[APP_BTDATA_PROCESS_STK_SIZE];
static OS_STK App_SPKFUNProcess_Stk[APP_SPKFUN_PROCESS_STK_SIZE];

static void App_TaskStart(void *p_arg);

/**
  * @brief  NVIC configure
  * @retval None
  */
static void BSPNVICConfig(void)
{
#ifdef  VECT_TAB_RAM
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else
  #ifndef DEV_DEBUG
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x2800);   /* APP 起始 0x8002800 */
  #else
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0000);
  #endif
#endif

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}

/**
  * @brief  ADC 采样任务：2kHz 邮箱，内部分频到 5Hz
  *         仅在非测试状态刷电压显示；测试中 ADC 由 test_flow 按需独占
  */
OS_EVENT *g_ptSensorUpdateMBox;
void CSensorTask(void *p_arg)
{
    u8  err;
    u16 div = 0;

    g_ptSensorUpdateMBox = OSMboxCreate((void *)0);

    while (1) {
        OSMboxPend(g_ptSensorUpdateMBox, 100, &err);
        if (++div >= 400) {                          /* 2kHz/400 = 5Hz */
            div = 0;
            if (test_flow_state() == TF_WAIT_BOARD) {
                s32 mv[4];
                if (ads131_read_mv(mv)) {
                    vgus_write_u16(VP_BUS_VOL, (u16)(mv[ADS131_CH_BUS] / 10));
                    vgus_write_u16(VP_V12,  (u16)mv[ADS131_CH_12V]);
                    vgus_write_u16(VP_V5,   (u16)mv[ADS131_CH_5V]);
                    vgus_write_u16(VP_V3V3, (u16)mv[ADS131_CH_3V3]);
                }
            }
        }
    }
}

/**
  * @brief  测试流程任务：200Hz 邮箱，分频到 100Hz（10ms）驱动状态机
  */
OS_EVENT *g_ptMotionUpdateBox;
void CMotionTask(void *p_arg)
{
    u8 err;
    u8 div = 0;

    g_ptMotionUpdateBox = OSMboxCreate((void *)0);

    while (1) {
        OSMboxPend(g_ptMotionUpdateBox, 100, &err);
        if (++div >= 2) {
            div = 0;
            test_flow_run();                         /* 10ms 周期 */
        }
    }
}

/**
  * @brief  系统监控任务：100Hz（预留：后续放安全监控/超时兜底）
  */
OS_EVENT *g_ptSystemSuperviseMBox;
void CSystemSuperiseTask(void *p_arg)
{
    u8 err;

    g_ptSystemSuperviseMBox = OSMboxCreate((void *)0);

    while (1) {
        OSMboxPend(g_ptSystemSuperviseMBox, 100, &err);
        __nop();
    }
}

/**
  * @brief  主板串口协议解析任务：20ms
  */
void CUsartCommTask(void *p_arg)
{
    while (1) {
        jig_proto_poll();
        OSTimeDly(2);                                /* 2 tick = 20ms */
    }
}

/**
  * @brief  屏幕任务：40Hz，解析触摸上传与读应答
  */
OS_EVENT *g_ptbluetoothtastMBox;
void bluetoothtast(void *p_arg)
{
    u8 err;

    g_ptbluetoothtastMBox = OSMboxCreate((void *)0);

    while (1) {
        OSMboxPend(g_ptbluetoothtastMBox, 100, &err);
        vgus_poll();
        if (g_vgus_touch.new_flag) {
            /* TODO(UI)：触摸键值 → 操作映射，等 UI 方案定稿后填 */
            g_vgus_touch.new_flag = 0;
        }
    }
}

/**
  * @brief  杂项任务：100Hz（预留）
  */
OS_EVENT *g_ptspkworkingtastMBox;
void spkworkingtast(void *p_arg)
{
    u8 err;

    g_ptspkworkingtastMBox = OSMboxCreate((void *)0);

    while (1) {
        OSMboxPend(g_ptspkworkingtastMBox, 100, &err);
        __nop();
    }
}

int main(void)
{
    volatile CPU_INT08U os_err;

    BSPNVICConfig();

    OSInit();
    os_err = OSTASKCREATE(App_TaskStart, 0, App_TaskStart_Stk, APP_START_TASK_PRIO);
    OSTimeSet(0);
    OSStart();

    return (0);
}

/**
  * @brief  App_TaskStart
  * @retval None
  */
void App_TaskStart(void *p_arg)
{
    volatile CPU_INT08U os_err;

    OS_CPU_SysTickInit();

#if (OS_TASK_STAT_EN > 0)
    OSStatInit();
#endif

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    BSPInit();                                       /* 其余硬件初始化 */

    os_err = OSTASKCREATE(CSensorTask,         0, App_Sensor_Sample_Stk,   SENSOR_SAMPLE_TASK_PRIO);
    os_err = OSTASKCREATE(CMotionTask,         0, App_MotionProcess_Stk,   MOTION_PROCESS_TASK_PRIO);
    os_err = OSTASKCREATE(CSystemSuperiseTask, 0, App_SystemSuperise_Stk,  SYSTEM_SUPERISE_TASK_PRIO);
    os_err = OSTASKCREATE(CUsartCommTask,      0, App_UsartProcess_Stk,    USART_PROCESS_TASK_PRIO);
    os_err = OSTASKCREATE(bluetoothtast,       0, App_BTDataProcess_Stk,   BTDATA_PROCESS_TASK_PRIO);
    os_err = OSTASKCREATE(spkworkingtast,      0, App_SPKFUNProcess_Stk,   SPKFUN_PROCESS_TASK_PRIO);

    for (;;) {
        OSTimeDly(100);                              /* 1s 心跳 */
    }
}

#if OS_TIME_TICK_HOOK_EN > 0
/**
  * @brief  App_TimeTickHook
  * @retval None
  */
void  App_TimeTickHook (void)
{
}
#endif

#if (OS_APP_HOOKS_EN > 0)

/**
  * @brief  App_TaskCreateHook
  * @retval None
  */
void  App_TaskCreateHook (OS_TCB *ptcb)
{
}

/**
  * @brief  App_TaskDelHook
  * @retval None
  */
void  App_TaskDelHook (OS_TCB *ptcb)
{
    (void)ptcb;
}

#if OS_VERSION >= 251
/**
  * @brief  App_TaskIdleHook
  * @retval None
  */
void  App_TaskIdleHook (void)
{
}
#endif

/**
  * @brief  App_TaskStatHook
  * @retval None
  */
void  App_TaskStatHook (void)
{
}

#if OS_TASK_SW_HOOK_EN > 0
/**
  * @brief  App_TaskSwHook
  * @retval None
  */
void  App_TaskSwHook (void)
{
}
#endif

#if OS_VERSION >= 204
/**
  * @brief  App_TCBInitHook
  * @retval None
  */
void  App_TCBInitHook (OS_TCB *ptcb)
{
    (void)ptcb;
}
#endif

#endif

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* Infinite loop */
    while (1) {
    }
}
#endif

void HardFault_Handler(void)
{
    if (CoreDebug->DHCSR & 1) {
        __breakpoint(0);
    }
    while (1) {
    }
}

/********* (C) COPYRIGHT 2014  TECHNOLOGIES CO.,LTD. ***END OF FILE***/
