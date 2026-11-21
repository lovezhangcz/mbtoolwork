#include "stm32f10x.h"
#include "sys.h"
//#include "led.h"
#include "common.h"

//前驱
//const s16 HALL_PHASE_SHIFT=(10922-32767);
//const s8 MOTOR_DIR=1;
//u8	POLORNUM_HBKJ=14;

const s16 HALL_PHASE_SHIFT=(0);
const s8 MOTOR_DIR=1;
u8	POLORNUM_HBKJ=14;

s32 blancevqinput=0;
s32 BusCurrent=0;
s32 manualsetVq = 0;
u8 shortcuterrflg=0;
u8 hall_angle_debugflv=0;
s32 phcurrpwoffval=10*300;
s32 phcurrprotectval=10*300; 
s32 BusCurrLimit=800;
s32 SpeedLimit=18000;

void bsp_ini(void)//初始化期间调用接口
{

}
void timescan(void)//500hz
{

}
void commonworking(void)//独立进程低速调用5hz
{

}
