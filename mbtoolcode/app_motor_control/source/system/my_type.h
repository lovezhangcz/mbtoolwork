/******************** (C) COPYRIGHT 2012  ******************************
* File Name 	: my_typedef.h
* Author 		: 
* Version 		: V0.0.1
* Date 			: 2012-3-13
* Description 	: 自定义数据类型
*******************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MY_TYPE_H__
#define __MY_TYPE_H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/
typedef uint64_t        u64;
typedef __IO uint64_t  vu64;

typedef int64_t         s64;
typedef __IO int64_t  vs64;

typedef enum {FALSE = 0, TRUE = !FALSE} bool;

#define U8_MAX     ((u8)255)
#define S8_MAX     ((s8)127)
#define S8_MIN     ((s8)-128)
#define U16_MAX    ((u16)65535u)
#define S16_MAX    ((s16)32767)
#define S16_MIN    ((s16)-32768)
#define U32_MAX    ((u32)4294967295uL)
#define S32_MAX    ((s32)2147483647)
#define S32_MIN    ((s32)-2147483648)

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/* Exported variables ------------------------------------------------------- */

#endif /* __MY_TYPE_H__ */

/********************** (C) COPYRIGHT 2012  *****END OF FILE************/ 
