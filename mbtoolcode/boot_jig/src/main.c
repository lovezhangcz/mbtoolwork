/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2014, Cargle, <kargoo@qq.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Function: Is is an head file for this library. You can see all be called functions.
 * Created on: 2015-01-10
 */


/* Includes ------------------------------------------------------------------*/
#include <stm32f10x.h>
#include "flash.h"
#include "string.h"
#include "ymodem.h"
//#include "uart1.h"
//#include "rgb_pwm.h"

#include "stm32f10x_bkp.h"
#include "stm32f10x_pwr.h"
#include "usartset.h"
#include "dis_update.h"
/** @addtogroup STM32F0xx_IAP
  * @{
  */
typedef  void (*pFunction)(void);

//u32 g_tick500hz=0;
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

//uint8_t FileName[FILE_NAME_LENGTH];

//struct tagUart uartDate;
//extern struct tagUart uartDMA;
//extern uint32_t g_fileSize;
//extern uint32_t g_updateSize;
uint32_t g_updateSize;


u32 g_tick500hz=0;
u32 thisstaticcnt;
u32 g_tick500hz_main;
#define DIS_CC_CNT_BASE 150
#define DIS_CC_CNT_EACH 30

/* Private function prototypes -----------------------------------------------*/
//static void IAP_Init(void);
//static void SerialDownload(void);

/* Private functions ---------------------------------------------------------*/
void RCC_InternalConfiguration(void);
void NVIC_Configuration(void);
void speakerInit(void);
void speaker(int on);
void LockInit(void);
void key_scan(void);
void Time3Init(void);
void Time3Close(void);
void led_control(void);


#define HALL_MAX_WAITCNT 2000
u8 Hall_ReadState(void);
void hall_ini(void);
u32 hall_boot_cnt=0;
u8 hall_boot_waitflg=0;
u8 cputype_boot_waitflg=0;
u8 usartsendbuffer_app[21]={0};
//extern eYM_STAT ymodemstat;


char soft_version[5]={'0','1','0','0',0};


#if defined(MODEM_KS_FX1)//MODEM_KS_B22
	#define BMS_USED 1
	char soft_mode[]={'K','S','-','F','X','1',0};
	__attribute__((at(0x8001300))) const char BootSoftVersion[]="BootSoftVersion-Date-20260823--Model-KS-FX1____CH32F103CP";
#elif defined(MODEM_KS_JIG)
	#define BMS_USED 0
	char soft_mode[]={'K','S','-','J','I','G',0};
	__attribute__((at(0x8001300))) const char BootSoftVersion[]="BootSoftVersion-Date-20261115--Model-KS-JIG____GD32F103RC";
#endif

#define BOOT_CPU_CHECK_ADDRESS 								(0x08001300+64)
#define APP_CPU_CHECK_ADDRESS					(0x08004300+12+4)
#define CPU_CHECK_USEDFLG 								0xC411CE5D //标记CPU匹配检查功能启用

#if BMS_USED==0
	#define LARG_FLASH_IC_USED 0
	#define BOOT_CPU_CHECK_REQ_USEDFLG 						0x00000003	//bit1:ch32cp,bit2:buscoef,bit3:bms
	#define BOOT_CPU_CHECK_ANS_USEDFLG						0x00000003	//bit1:ch32cp,bit2:buscoef,bit3:bms
#else
	#define LARG_FLASH_IC_USED 1	
	#define BOOT_CPU_CHECK_REQ_USEDFLG 						0x00000007	//bit1:ch32cp,bit2:buscoef,bit3:bms
	#define BOOT_CPU_CHECK_ANS_USEDFLG						0x00000007	//bit1:ch32cp,bit2:buscoef,bit3:bms
#endif	
__attribute__((at(BOOT_CPU_CHECK_ADDRESS))) const u32 BootCpuTypeFlg[5]={CPU_CHECK_USEDFLG,BOOT_CPU_CHECK_REQ_USEDFLG,BOOT_CPU_CHECK_ANS_USEDFLG,0xffffffff,0xffffffff};

u8 checkcputypefit(void)
{
	u32 *pappcputype=(u32*)APP_CPU_CHECK_ADDRESS;
	if(pappcputype[0]^BootCpuTypeFlg[0])return 0;//匹配功能启用
	if((BootCpuTypeFlg[1]&pappcputype[2])^BootCpuTypeFlg[1])return 0;//Boot要求APP提供的
	if((BootCpuTypeFlg[2]&pappcputype[1])^pappcputype[1])return 0;//APP要求Boot提供的
	return 1;
}	
	
#define APP_END_INDX_ADDRESS 	0x8004800
#define Program_Mark_ADDR_F (APP_END_INDX_ADDRESS+4)
#define Program_Checksum_ADDR (APP_END_INDX_ADDRESS+8)
#define Program_Checksumlen_ADDR (APP_END_INDX_ADDRESS+12)
#define Program_Checksumdat_ADDR (APP_END_INDX_ADDRESS+16)
#define Program_APP_SINGN_ADDR (APP_END_INDX_ADDRESS+20)
#define APP_END_CHECK_VALUE 	0x145089bd

#if defined(MODEM_KS_M12)||defined(MODEM_KS_MTB1) ||defined(MODEM_KS_MTB7)||defined(MODEM_KS_FX1)
	#define APP_SIGN_CHECK_VALUE 	0x145000b1
#elif defined(MODEM_KS_JIG)
	#define APP_SIGN_CHECK_VALUE 	0x14500301
#elif defined(MODEM_KS_MTB1R)
	#define APP_SIGN_CHECK_VALUE 	0x145002b1
#elif defined(MODEM_KS_M11R)|| defined(MODEM_KS_M22R)|| defined(MODEM_KS_M3R)
	#define APP_SIGN_CHECK_VALUE 	0x14500200
#else //||defined(MODEM_KS_MTB8)
	#define APP_SIGN_CHECK_VALUE 	0x14500000
#endif

#if defined(MODEM_KS_FX1)||defined(MODEM_KS_V3)||defined(MODEM_KS_V1)||defined(MODEM_KS_EBP1)
	#define SPK_MUTE 1
#else
	#define SPK_MUTE 0
#endif


u8 DFU_CheckUserApp(void)
{
	u32 *psign=(u32*)Program_APP_SINGN_ADDR;
	u32 *indxaddr=(u32*)APP_END_INDX_ADDRESS;
	u32 *checkaddr;//=
#if defined(MODEM_KS_JIG)
	if(indxaddr[0]>0x803F800)return 0;	/* GD32F103RC 256KB */
#elif LARG_FLASH_IC_USED==0	
	if(indxaddr[0]>0x800F800)return 0;
#else
	if(indxaddr[0]>0x801C800)return 0;	
#endif
	checkaddr=(u32*)*indxaddr;
#if defined(MODEM_KS_M12)||defined(MODEM_KS_MTB1) ||defined(MODEM_KS_MTB7)||defined(MODEM_KS_FX1)\
		||defined(MODEM_KS_MTB1R)||defined(MODEM_KS_M11R)|| defined(MODEM_KS_M22R)|| defined(MODEM_KS_M3R)||defined(MODEM_KS_JIG)\

	if(checkaddr[0]==APP_END_CHECK_VALUE && indxaddr[1]==0xffffffff && psign[0]==APP_SIGN_CHECK_VALUE)
#else //||defined(MODEM_KS_MTB8)
	if(checkaddr[0]==APP_END_CHECK_VALUE && indxaddr[1]==0xffffffff)
#endif	
	{
		return 1;
	}else
	{
		return 0;
	}
}	



void IWDG_Init(u8 prer,u16 rlr)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);//去保护
	IWDG_SetPrescaler(prer);//预分频
	IWDG_SetReload(rlr);//重装值
	IWDG_ReloadCounter();// 重载计数值
	IWDG_Enable();//开启看门狗
}	
	/*	版本说明
	
20190604增加嗡鸣器响	
	
	
*/

//__attribute__((at(0x8002700))) const char SERIERNUM[20]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	
#if (defined(MODEM_KS_M8)||defined(MODEM_KS_T2)||defined(MODEM_KS_M10)||defined(MODEM_KS_M11)||defined(MODEM_KS_MTB5)||defined(MODEM_KS_MTB8)||defined(MODEM_KS_EBP2))
	#define SPEAKERPORT_RCC 		RCC_APB2Periph_GPIOB	
	#define SPEAKERPORT 				GPIOB
	#define SPEAKERPIN 					GPIO_Pin_9
		
	#define READ_SWITCHIO_RCC   RCC_APB2Periph_GPIOD
	#define READ_SWITCHIO_PORT  GPIOD
	#define READ_SWITCHIO_PIN		GPIO_Pin_1
	#define READ_SWITCH 				GPIO_ReadInputDataBit(GPIOD , GPIO_Pin_1)
		
	#define POWER_LOCK_RCC			RCC_APB2Periph_GPIOC
	#define POWER_LOCK_PORT  		GPIOC
	#define POWER_LOCK_PIN			GPIO_Pin_13	
	#define SET_POWER_LOCK    	GPIO_SetBits(GPIOC, GPIO_Pin_13)
	#define SET_POWER_UNLOCK  	GPIO_ResetBits(GPIOC, GPIO_Pin_13)

		#define RCC_HALLA		    RCC_APB2Periph_GPIOC  //
		#define RCC_HALLB		    RCC_APB2Periph_GPIOC  //
		#define RCC_HALLC		    RCC_APB2Periph_GPIOC  //	
		#define HALL_PORTA		    GPIOC
		#define HALL_PORTB		    GPIOC
		#define HALL_PORTC		    GPIOC	
		#define HALL_A_PIN		    GPIO_Pin_6
		#define HALL_B_PIN		    GPIO_Pin_7
		#define HALL_C_PIN		    GPIO_Pin_8
#elif defined(MODEM_KS_JIG)
	#define SPEAKERPORT_RCC 		RCC_APB2Periph_GPIOA	
	#define SPEAKERPORT 				GPIOA
	#define SPEAKERPIN 					GPIO_Pin_8
		
	#define READ_SWITCHIO_RCC   RCC_APB2Periph_GPIOB
	#define READ_SWITCHIO_PORT  GPIOB
	#define READ_SWITCHIO_PIN		GPIO_Pin_12
	#define READ_SWITCH 				0	/* 治具无电源开关,恒为0 */
		
	#define POWER_LOCK_RCC			RCC_APB2Periph_GPIOC
	#define POWER_LOCK_PORT  		GPIOC
	#define POWER_LOCK_PIN			GPIO_Pin_8	/* 占位,治具不使用电源自锁 */
	#define SET_POWER_LOCK    
	#define SET_POWER_UNLOCK  

		#define RCC_HALLA		    RCC_APB2Periph_GPIOB  //治具无霍尔,KEY3(PB12)兼作进入升级按键
		#define RCC_HALLB		    RCC_APB2Periph_GPIOB  //
		#define RCC_HALLC		    RCC_APB2Periph_GPIOB  //	
		#define HALL_PORTA		    GPIOB
		#define HALL_PORTB		    GPIOB
		#define HALL_PORTC		    GPIOB	
		#define HALL_A_PIN		    GPIO_Pin_12
		#define HALL_B_PIN		    GPIO_Pin_12
		#define HALL_C_PIN		    GPIO_Pin_12
#elif defined(MODEM_KS_M12)||defined(MODEM_KS_MTB1)||defined(MODEM_KS_MTB7)||defined(MODEM_KS_FX1)\
			||defined(MODEM_KS_MTB1R)
	#define SPEAKERPORT_RCC 		RCC_APB2Periph_GPIOA	
	#define SPEAKERPORT 				GPIOA
	#define SPEAKERPIN 					GPIO_Pin_12
		
	#define READ_SWITCHIO_RCC   RCC_APB2Periph_GPIOB
	#define READ_SWITCHIO_PORT  GPIOB
	#define READ_SWITCHIO_PIN		GPIO_Pin_3
	#define READ_SWITCH 				GPIO_ReadInputDataBit(GPIOB , GPIO_Pin_2)
		
	#define POWER_LOCK_RCC			RCC_APB2Periph_GPIOC
	#define POWER_LOCK_PORT  		GPIOC
	#define POWER_LOCK_PIN			GPIO_Pin_14	
	#define SET_POWER_LOCK    	GPIO_SetBits(GPIOC, GPIO_Pin_14)
	#define SET_POWER_UNLOCK  	GPIO_ResetBits(GPIOC, GPIO_Pin_14)

		#define RCC_HALLA		    RCC_APB2Periph_GPIOB  //
		#define RCC_HALLB		    RCC_APB2Periph_GPIOB  //
		#define RCC_HALLC		    RCC_APB2Periph_GPIOB  //	
		#define HALL_PORTA		    GPIOB
		#define HALL_PORTB		    GPIOB
		#define HALL_PORTC		    GPIOB	
		#define HALL_A_PIN		    GPIO_Pin_0
		#define HALL_B_PIN		    GPIO_Pin_4
		#define HALL_C_PIN		    GPIO_Pin_5
#elif defined(MODEM_KS_V3)||defined(MODEM_KS_V1)
	#define SPEAKERPORT_RCC 		RCC_APB2Periph_GPIOA	
	#define SPEAKERPORT 				GPIOA
	#define SPEAKERPIN 					GPIO_Pin_12
		
	#define READ_SWITCHIO_RCC   RCC_APB2Periph_GPIOB
	#define READ_SWITCHIO_PORT  GPIOB
	#define READ_SWITCHIO_PIN		GPIO_Pin_3
	#define READ_SWITCH 				GPIO_ReadInputDataBit(GPIOB , GPIO_Pin_2)
		
	#define POWER_LOCK_RCC			RCC_APB2Periph_GPIOC
	#define POWER_LOCK_PORT  		GPIOC
	#define POWER_LOCK_PIN			GPIO_Pin_14	
	#define SET_POWER_LOCK    	GPIO_SetBits(GPIOC, GPIO_Pin_14)
	#define SET_POWER_UNLOCK  	GPIO_ResetBits(GPIOC, GPIO_Pin_14)

		#define RCC_HALLA		    RCC_APB2Periph_GPIOA  //
		#define RCC_HALLB		    RCC_APB2Periph_GPIOA  //
		#define RCC_HALLC		    RCC_APB2Periph_GPIOA  //	
		#define HALL_PORTA		    GPIOA
		#define HALL_PORTB		    GPIOA
		#define HALL_PORTC		    GPIOA	
		#define HALL_A_PIN		    GPIO_Pin_4
		#define HALL_B_PIN		    GPIO_Pin_5
		#define HALL_C_PIN		    GPIO_Pin_7		
#elif defined(MODEM_KS_FXsim)
	#define SPEAKERPORT_RCC 		RCC_APB2Periph_GPIOA	
	#define SPEAKERPORT 				GPIOA
	#define SPEAKERPIN 					GPIO_Pin_12
		
	#define READ_SWITCHIO_RCC   RCC_APB2Periph_GPIOB
	#define READ_SWITCHIO_PORT  GPIOB
	#define READ_SWITCHIO_PIN		GPIO_Pin_3
	#define READ_SWITCH 				GPIO_ReadInputDataBit(GPIOB , GPIO_Pin_2)
		
	#define POWER_LOCK_RCC			RCC_APB2Periph_GPIOC
	#define POWER_LOCK_PORT  		GPIOC
	#define POWER_LOCK_PIN			GPIO_Pin_14	
	#define SET_POWER_LOCK    	GPIO_SetBits(GPIOC, GPIO_Pin_14)
	#define SET_POWER_UNLOCK  	GPIO_ResetBits(GPIOC, GPIO_Pin_14)

		#define RCC_HALLA		    RCC_APB2Periph_GPIOA  //
		#define RCC_HALLB		    RCC_APB2Periph_GPIOA  //
		#define RCC_HALLC		    RCC_APB2Periph_GPIOA  //	
		#define HALL_PORTA		    GPIOA
		#define HALL_PORTB		    GPIOA
		#define HALL_PORTC		    GPIOA	
		#define HALL_A_PIN		    GPIO_Pin_4
		#define HALL_B_PIN		    GPIO_Pin_5
		#define HALL_C_PIN		    GPIO_Pin_7
#elif defined(MODEM_KS_EBP1)	
	#define SPEAKERPORT_RCC 		RCC_APB2Periph_GPIOA	
	#define SPEAKERPORT 				GPIOA
	#define SPEAKERPIN 					GPIO_Pin_11
		
	#define READ_SWITCHIO_RCC   RCC_APB2Periph_GPIOB
	#define READ_SWITCHIO_PORT  GPIOB
	#define READ_SWITCHIO_PIN		GPIO_Pin_2
	#define READ_SWITCH 				GPIO_ReadInputDataBit(GPIOB , GPIO_Pin_2)
		
	#define POWER_LOCK_RCC			RCC_APB2Periph_GPIOC
	#define POWER_LOCK_PORT  		GPIOC
	#define POWER_LOCK_PIN			GPIO_Pin_14	
	#define SET_POWER_LOCK    	GPIO_SetBits(GPIOC, GPIO_Pin_14)
	#define SET_POWER_UNLOCK  	GPIO_ResetBits(GPIOC, GPIO_Pin_14)

		#define RCC_HALLA		    RCC_APB2Periph_GPIOB  //
		#define RCC_HALLB		    RCC_APB2Periph_GPIOB  //
		#define RCC_HALLC		    RCC_APB2Periph_GPIOB  //	
		#define HALL_PORTA		    GPIOB
		#define HALL_PORTB		    GPIOB
		#define HALL_PORTC		    GPIOB	
		#define HALL_A_PIN		    GPIO_Pin_0
		#define HALL_B_PIN		    GPIO_Pin_4
		#define HALL_C_PIN		    GPIO_Pin_5
#else

	#define SPEAKERPORT_RCC 		RCC_APB2Periph_GPIOA	
	#define SPEAKERPORT 				GPIOA
	#define SPEAKERPIN 					GPIO_Pin_11
		
	#define READ_SWITCHIO_RCC   RCC_APB2Periph_GPIOB
	#define READ_SWITCHIO_PORT  GPIOB
	#define READ_SWITCHIO_PIN		GPIO_Pin_2
	#define READ_SWITCH 				GPIO_ReadInputDataBit(GPIOB , GPIO_Pin_2)
		
	#define POWER_LOCK_RCC			RCC_APB2Periph_GPIOC
	#define POWER_LOCK_PORT  		GPIOC
	#define POWER_LOCK_PIN			GPIO_Pin_14	
	#define SET_POWER_LOCK    	GPIO_SetBits(GPIOC, GPIO_Pin_14)
	#define SET_POWER_UNLOCK  	GPIO_ResetBits(GPIOC, GPIO_Pin_14)

		#define RCC_HALLA		    RCC_APB2Periph_GPIOB  //
		#define RCC_HALLB		    RCC_APB2Periph_GPIOB  //
		#define RCC_HALLC		    RCC_APB2Periph_GPIOB  //	
		#define HALL_PORTA		    GPIOB
		#define HALL_PORTB		    GPIOB
		#define HALL_PORTC		    GPIOB	
		#define HALL_A_PIN		    GPIO_Pin_0
		#define HALL_B_PIN		    GPIO_Pin_4
		#define HALL_C_PIN		    GPIO_Pin_5	

#endif

//#if LARG_FLASH_IC_USED==1
//	#define Program_Mark_ADDR 0x0801E000
//#else
//	#define Program_Mark_ADDR 0x0800F000
//#endif

//#define Program_Mark_ADDR 		0x0800F000//0x08002800		//编程标记位区域


//读取指定地址的半字(16位数据)
//faddr:读地址(此地址必须为2的倍数!!)
//返回值:对应数据.
u16 STMFLASH_ReadHalfWord(u32 faddr)			 // 只读一个半字
{
	return *(__IO uint16_t*)faddr; 
}
u32 STMFLASH_ReadWord(u32 faddr)			 // 只读一个字
{
	return (*(__IO uint32_t*)faddr);
}

extern u8 displaybybte[2];
extern u8 spk_req;

void send_device_model(void)			   //7
{
	u8 temtxbuf[32+20+10+4];
	u8 tempdbuf[64]={0};
 	int i=0;
 	int len1 = strlen(soft_mode);
 	int len2 = strlen(soft_version);
	
 	u16 check = 0;	
	
 	usartsendbuffer_app[0]=0xaa;
 	usartsendbuffer_app[1]=0x55;
	
 	for(i=0;i<len1;i++){
 		usartsendbuffer_app[i+2]=soft_mode[i];
 		check += soft_mode[i];
 	}
 	usartsendbuffer_app[len1+2]='-';
 	check += '-';
 	for(i=0;i<len2;i++){
 		usartsendbuffer_app[len1+3+i]=soft_version[i];
 		check += soft_version[i];
 	}
 	for(i=len1+len2+1;i<14;i++){//5
		usartsendbuffer_app[i+2]=0;
 	}
 	usartsendbuffer_app[16]=0xbb;
 	usartsendbuffer_app[17]=20;
 	
 	check = 0xffff - check;
 	usartsendbuffer_app[18]=check;
	usartsendbuffer_app[19]=check>>8;
	
	usartsendbuffer_app[20]=0;

#if defined(MODEM_KS_M12)||defined(MODEM_KS_MTB1)||defined(MODEM_KS_MTB7)||defined(MODEM_KS_FX1)\
		||defined(MODEM_KS_MTB1R)||defined(MODEM_KS_JIG)
	{u32 temdlycnt=60000;while(temdlycnt--);}
	BufferTxTrig_bb(usartsendbuffer_app,20);
#else	
	if(spk_req==0)
	{
		tempdbuf[0]=displaybybte[0];	
		tempdbuf[1]=displaybybte[1];	
		crcinpack(tempdbuf,0xdd,usartsendbuffer_app,20,temtxbuf);		
	}else
	{
		tempdbuf[0]=3;	
		tempdbuf[1]=1;
		crcinpack(tempdbuf,0x0b,usartsendbuffer_app,20,temtxbuf);			
		spk_req=0;
	}	
	{u32 temdlycnt=60000;while(temdlycnt--);}
	BufferTxTrig_bb(temtxbuf,20+10+4);
#endif

	DisDataPool.msgcntflg=DIS_CC_CNT_BASE+DIS_CC_CNT_EACH;	
} 

	int bootload=0;
void bootstartset(void)
{
//		FLASH_Unlock();	
//		FLASH_ErasePage(Program_Mark_ADDR);
//		FLASH_ProgramWord(Program_Mark_ADDR,0x55AAAA55);
//		FLASH_Lock();
		hall_boot_waitflg=0;				
		bootload=1;	
}

u8 DIS_makesendpackpp(u8 *temtxbuf)
{
	
#if defined(MODEM_KS_M12)||defined(MODEM_KS_MTB1)||defined(MODEM_KS_MTB7)||defined(MODEM_KS_FX1)\
		||defined(MODEM_KS_MTB1R)||defined(MODEM_KS_JIG)
	
		u16 crc16;
		temtxbuf[0]=0xAA;
		temtxbuf[1]=0x55;	
		temtxbuf[2]=0;
		temtxbuf[3]=0;
		temtxbuf[4]=0;
		temtxbuf[5]=0;
		temtxbuf[14]=0xff;
		temtxbuf[15]=0xff;	
		temtxbuf[16]=0x70;							
		temtxbuf[17]=0x4c;//切换协议，请求重置	
		crc16=YModemCrc((char *)&temtxbuf[2],16,0);
		temtxbuf[18]=crc16>>8;
		temtxbuf[19]=crc16;
#else	
	//u8 temtxbuf[32+20+10+4];
	u8 tempdbuf[32];
	u8 pdata[64]={0};	
	u16 crc16;
		pdata[0]=0xAA;
		pdata[1]=0x55;	
		pdata[2]=0;
		pdata[3]=0;
		pdata[4]=0;
		pdata[5]=0;
		pdata[14]=0xff;
		pdata[15]=0xff;	
		pdata[16]=0x70;							
		pdata[17]=0x4c;//切换协议，请求重置	
		crc16=YModemCrc((char *)&pdata[2],16,0);
		pdata[18]=crc16>>8;
		pdata[19]=crc16;

	
//		crcinpack(tempdbuf,0xdd,pdata,20,temtxbuf);	
		if(spk_req==0)
		{
			tempdbuf[0]=displaybybte[0];	
			tempdbuf[1]=displaybybte[1];	
			crcinpack(tempdbuf,0xdd,pdata,20,temtxbuf);		
		}
		else
		{
			tempdbuf[0]=3;	
			tempdbuf[1]=1;
			crcinpack(tempdbuf,0x0b,pdata,20,temtxbuf);			
			spk_req=0;
		}	
#endif	
		return 20;
}
void USART_USER_IRQ_BBAPP(u8 *pdata,u16 dlen)
{
	if(pdata[16]==0x70)
	{
		dis_update_pro(pdata,dlen);
	}else if(pdata[16]==0x9b)
	{
		send_device_model();
	}
	
}


int main(void)
{
	u8 cputypechedkflgrev=0;
	u8 bootchedkflgrev=0;	
//	RCC_InternalConfiguration();
	NVIC_Configuration();
	
		hall_ini();
		cputypechedkflgrev=1;//checkcputypefit();
		bootchedkflgrev=DFU_CheckUserApp();
	
//	if(STMFLASH_ReadWord(Program_Mark_ADDR) == 0x55AAAA55){
//	if((STMFLASH_ReadWord(Program_Mark_ADDR) == 0x55AAAA55) || (Hall_ReadState()==0x07)|| cputypechedkflgrev==0){		
	if(cputypechedkflgrev==0 || bootchedkflgrev==0 ||(Hall_ReadState()==0x07)){
		speakerInit();
		LockInit();
		USART_Initial_bb(115200);//		uart_init();
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);		
		Time3Init();
		//rgb_init();
		//bootload=1;//需升级app
	}
//	if((STMFLASH_ReadWord(Program_Mark_ADDR) == 0x55AAAA55)||(cputypechedkflgrev==0))
	if(cputypechedkflgrev==0 || bootchedkflgrev==0 )	
	{
		bootload=1;//需升级app
	}
	if(cputypechedkflgrev==0)
	{
		cputype_boot_waitflg=1;
	}
	if((Hall_ReadState()==0x07))
	{
		hall_boot_waitflg=1;
	}
	DisDataPool.msgcntflg=DIS_CC_CNT_BASE;
  while (1)
  {
		thisstaticcnt++;
		if(g_tick500hz_main<g_tick500hz)
		{
			g_tick500hz_main++;
			thisstaticcnt=0;
		}			
		
		if(bootload==1||hall_boot_waitflg == 1){

			
			if(g_tick500hz_main%20==0 && thisstaticcnt==0)//定时
			{
				if(DisDataPool.msgcntflg++>(DIS_CC_CNT_BASE+DIS_CC_CNT_EACH+10))
				{
					u8 sendbuff[64];
					u8 sendlen=0;
					sendlen=DIS_makesendpackpp(sendbuff);
					#if defined(MODEM_KS_M12)||defined(MODEM_KS_MTB1)||defined(MODEM_KS_MTB7)||defined(MODEM_KS_FX1)\
					||defined(MODEM_KS_MTB1R)||defined(MODEM_KS_JIG)
					BufferTxTrig_bb(sendbuff,20);					
					#else
					//BufferTxTrig_u1(sendbuff,20);						
					BufferTxTrig_bb(sendbuff,20+10+4);					
					#endif

					
					DisDataPool.msgcntflg=DIS_CC_CNT_BASE;
				}					
			}			

			if(DisDataPool.finishflg==1)
			{
				if(updatacheck())
				{ 
					cputypechedkflgrev=checkcputypefit();
					bootchedkflgrev=DFU_CheckUserApp();					
					if(cputypechedkflgrev==1 && bootchedkflgrev==1)
					{
						DisDataPool.checkokflg=1;
						DisDataPool.jumpdelay=999999;
						bootload=0;	
//						FLASH_Unlock();	
//						FLASH_ErasePage(Program_Mark_ADDR);
//						FLASH_Lock();
						cputype_boot_waitflg=0;
					}else{
						cputype_boot_waitflg=1;						
					}
					
				}
				DisDataPool.finishflg=2;				
			}

			if(hall_boot_cnt>=HALL_MAX_WAITCNT)
			{			
				hall_boot_waitflg=0;
			}
						
		}else if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000){ //跳到应用区
			pFunction Jump_To_Application;
			uint32_t JumpAddress;
			if(DisDataPool.jumpdelay>0)
			{
				DisDataPool.jumpdelay--;
			}else
			{	
				UsartClose_bb();
				Time3Close();
				//rgb_close();
				IWDG_Init(4,2777);
			
				JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
				Jump_To_Application = (pFunction) JumpAddress;
				
				/* Initialize user application's Stack Pointer */
				__set_MSP(*(__IO uint32_t*) ApplicationAddress);
				
				/* Jump to application */
				Jump_To_Application();
			}
		}else{
			if(bootload==0){
				speakerInit();
				LockInit();
				USART_Initial_bb(115200);//				uart_init();
				Time3Init();
				//rgb_init();
				bootload=1;
			}

		}
	}

}



void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0000);
#endif
    
    /* 1 bit for pre-emption priority, 3 bits for subpriority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//优先级设置  全为抢占式优先级
}
void speaker(int on){
	if (on) {
		GPIO_SetBits(SPEAKERPORT, SPEAKERPIN);
	}else {
		GPIO_ResetBits(SPEAKERPORT, SPEAKERPIN);
	}
}
void speakerInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
		RCC_APB2PeriphClockCmd( SPEAKERPORT_RCC|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_PD01,ENABLE);
	#if defined(MODEM_KS_JIG)
	  /* 治具:蜂鸣器PA8,运行灯PA9,推挽输出默认关闭 */
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	  GPIO_Init(GPIOA, &GPIO_InitStructure);
	  GPIO_ResetBits(GPIOA,GPIO_Pin_8|GPIO_Pin_9);
	#elif SPK_MUTE==0
	  GPIO_DeInit(SPEAKERPORT);
		GPIO_StructInit(&GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = SPEAKERPIN;//蜂鸣器
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_Init(SPEAKERPORT, &GPIO_InitStructure);
	#else	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15; 	//PW_sw
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//GPIO_Mode_Out_OD;//GPIO_Mode_Out_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12; 	//PW_sw
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//GPIO_Mode_Out_OD;//GPIO_Mode_Out_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; 
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_15);
	GPIO_ResetBits(GPIOC,GPIO_Pin_12);	
	#endif
}
void LockInit(void)
{
#if defined(MODEM_KS_JIG)
	/* 治具无电源自锁(POWER_LOCK与ACT引脚冲突),空操作 */
	return;
#endif
    GPIO_InitTypeDef GPIO_InitStructure;

		RCC_APB2PeriphClockCmd( POWER_LOCK_RCC|READ_SWITCHIO_RCC|RCC_APB2Periph_AFIO, ENABLE);

		GPIO_InitStructure.GPIO_Pin =POWER_LOCK_PIN; 	//SD_DC自锁
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;//GPIO_Mode_Out_OD;//GPIO_Mode_Out_PP; 
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
		GPIO_Init(POWER_LOCK_PORT, &GPIO_InitStructure);	
	
		//GPIO_SetBits(GPIOC, GPIO_Pin_15);
		SET_POWER_LOCK;
	
		GPIO_InitStructure.GPIO_Pin =READ_SWITCHIO_PIN; 	//SWTEST按键
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; 
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
		GPIO_Init(READ_SWITCHIO_PORT, &GPIO_InitStructure);		
}
u8 Hall_ReadState(void)
{
	u8 ReadValue;
	
	/* If ports change, modify shift number */
	//ReadValue = (u8)(HALL_PORT->IDR & HALL_GPIO_MSK);//V5 
	ReadValue=(uint8_t)(GPIO_ReadInputDataBit(HALL_PORTA, HALL_A_PIN));//V8
	ReadValue|=(uint8_t)(GPIO_ReadInputDataBit(HALL_PORTB, HALL_B_PIN)<<1);//V8
	ReadValue|=(uint8_t)(GPIO_ReadInputDataBit(HALL_PORTC, HALL_C_PIN)<<2);//V8 
	#if defined(MODEM_KS_JIG)
	return (GPIO_ReadInputDataBit(HALL_PORTA, HALL_A_PIN)==Bit_RESET)?0x07:0x00;
	#elif defined(MODEM_KS_FX1)||defined(MODEM_KS_V1)
	return 1;
	#else
	return (ReadValue);
	#endif
}
void hall_ini(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_HALLA|RCC_HALLB|RCC_HALLC, ENABLE);

	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP,ENABLE);
 	GPIO_PinRemapConfig(GPIO_Remap_PD01,ENABLE);	
	
	PWR_BackupAccessCmd(ENABLE);
	RCC_LSEConfig(RCC_LSE_OFF);
  BKP_TamperPinCmd(DISABLE);
	PWR_BackupAccessCmd(DISABLE);		
	
#if defined(MODEM_KS_JIG)
	/* 治具:KEY3(PB12,低有效)上拉输入,按住上电进入升级等待 */
	GPIO_InitStructure.GPIO_Pin = HALL_A_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(HALL_PORTA, &GPIO_InitStructure);
#else
	GPIO_InitStructure.GPIO_Pin = HALL_A_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(HALL_PORTA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =HALL_B_PIN ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(HALL_PORTB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin =HALL_C_PIN ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(HALL_PORTC, &GPIO_InitStructure);		
#endif
}

void Time3Init(void)											//
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	uint16_t PrescalerValue = 0;  

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);				//基础设置，时基和比较输出设置，由于这里只需定时，所以不用OC比较输出	
	PrescalerValue = (uint16_t) (SystemCoreClock / 500000) - 1; 	//计算分频系数

	TIM_DeInit(TIM3);
	TIM_TimeBaseStructure.TIM_Period=1000;				  			//a period is 1000 times   //改低为200HZ,5MS
	TIM_TimeBaseStructure.TIM_Prescaler=PrescalerValue;				//division number   is 72 	  	//prescaler is 1200,that is 72000000/72/1000=1000Hz;
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 			//set clock division //or TIM_CKD_DIV2 or TIM_CKD_DIV4
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;	 	//count up
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init( &NVIC_InitStructure );
	
	TIM_ClearFlag(TIM3,TIM_FLAG_Update);	  						//clear the TIM2 overflow interrupt flag
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);		 				//TIM2 overflow interrupt enable
	TIM_Cmd(TIM3,ENABLE);								 			//enable TIM2
}
void Time3Close(void){
	TIM_Cmd(TIM3,DISABLE);	
	TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);//20190417这个不处理容易引起跳转后残留中断死机
	TIM_ClearITPendingBit(TIM3 , TIM_FLAG_Update); 	
}

u8 spk_req=0;

void TIM3_IRQHandler(void)	
{
	if(TIM_GetITStatus(TIM3 , TIM_IT_Update) != RESET ) {     
		TIM_ClearITPendingBit(TIM3 , TIM_FLAG_Update); 
		g_tick500hz++;	
		key_scan();	
		if(g_tick500hz%100==0){
			led_control();
			{
				static u8 lastwaitflg=0;
				if(hall_boot_waitflg)
				{
					if(hall_boot_cnt<HALL_MAX_WAITCNT)
					{
						if(Hall_ReadState()==0x07)
						{
							hall_boot_cnt++;
						}else
						{
							hall_boot_cnt+=200;						
						}

					}
					if(hall_boot_cnt%20==6)
					{
						spk_req=1;
					}
				}else
				{
					if(lastwaitflg==1)
					{
						speaker(0);
					}
				}
				lastwaitflg=hall_boot_waitflg;
			}			
		}
	}
}
void key_scan(void){
#if defined(MODEM_KS_JIG)
	/* 治具无长按关机逻辑 */
	return;
#endif
	static u32 press = 0,delaytimes=0,poweroff=0,data=0;
	if(READ_SWITCH == 1){
		press++;
	}else {
		press = 0;
		data = g_updateSize;
	}

	if(press > 2500 && data==g_updateSize){//播放bee时间
		delaytimes = 0;
		poweroff = 1;
		speaker(1);
	}else if(poweroff == 1 && delaytimes++ >100 && data==g_updateSize){//播放bee时间
		SET_POWER_UNLOCK;
		while(1);
	}
}

#define DSP_CODE_P 0x73
#define DSP_CODE_E 0x79
#define DSP_CODE_C 0x39
#define DSP_CODE_r 0x50
#define DSP_CODE_PP 0x40 // -
#define DSP_CODE_L 0x38 
#define DSP_CODE_o 0x5C
#define DSP_CODE_e 0x7B
#define DSP_CODE_U 0x3E
#define DSP_CODE_H 0x76
static unsigned char TM16_Display_buf[10]={0x3F,0x6,0x5B,0x4F,0x66,0x6D,0x7D,0x7,0x7F,0x6F};

u8 displaybybte[2]={0,0};

void led_control(void){
	int rate = DisDataPool.dispc;//g_updateSize*100/g_fileSize;
#if defined(MODEM_KS_JIG)
	/* 治具:升级等待/升级中运行灯PA9以2.5Hz闪烁 */
	if(g_tick500hz%200<100)GPIO_SetBits(GPIOA,GPIO_Pin_9);else GPIO_ResetBits(GPIOA,GPIO_Pin_9);
	rate=rate;
	return;
#endif


	if(spk_req==0)
	{
		if(cputype_boot_waitflg)
		{
			if(g_tick500hz%2000<400)
			{	
				displaybybte[0]=DSP_CODE_C;
				displaybybte[1]=DSP_CODE_H;				
			}else if(g_tick500hz%2000<800)
			{
				displaybybte[0]=DSP_CODE_U;
				displaybybte[1]=DSP_CODE_P;				
			}else
			{		
				displaybybte[0]=TM16_Display_buf[(rate/10)%10];
				displaybybte[1]=TM16_Display_buf[rate%10];		
			}			
		}else
		{
			if(g_tick500hz%2000<500)
			{	
				displaybybte[0]=DSP_CODE_U;
				displaybybte[1]=DSP_CODE_P;
						
			}else
			{		
				displaybybte[0]=TM16_Display_buf[(rate/10)%10];
				displaybybte[1]=TM16_Display_buf[rate%10];		
			}			
		}

	}
	
}
void DISPLAYPRO(u8 *dbuf)
{
	
}

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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
