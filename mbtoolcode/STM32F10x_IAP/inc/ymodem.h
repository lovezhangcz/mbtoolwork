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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _YMODEM_H_
#define _YMODEM_H_

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

#if defined(STM32F030) || defined(STM32F10X_LD) || defined(STM32F10X_LD_VL) || defined (STM32F10X_MD) || defined (STM32F10X_MD_VL)
#define PAGE_SIZE     1024 
#else
#define PAGE_SIZE     2048
#endif

//#define ApplicationAddress     				(uint32_t)(FLASH_BASE + 16 * PAGE_SIZE)//0x08008000
#define ApplicationAddress     				(uint32_t)(FLASH_BASE + 10 * PAGE_SIZE)//0x08008000
//#define APP_END_ADDRESS        				(uint32_t)(FLASH_BASE + 60 * PAGE_SIZE) //60 KBytes 
//#define FLASH_SIZE										(uint32_t)(((*(vu16 *)(0x1FFFF7E0))) * PAGE_SIZE) //Ð¾Æ¬´æ´¢¿Õ¼ä
//#define APP_END_ADDRESS        				(uint32_t)(FLASH_BASE + FLASH_SIZE) //60 KBytes 
//#define APP_SIZE   										(uint32_t)(APP_END_ADDRESS - ApplicationAddress)

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define PACKET_SEQNO_INDEX      (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER           (2)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (16)
#define PACKET_1K_SIZE          (1024)

#define FILE_NAME_LENGTH        (256)
#define FILE_SIZE_LENGTH        (16)

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define ETX                     (0x03)  /* end of Text */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* two of these in succession aborts transfer */
#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             (0x10000)
#define MAX_ERRORS              (15)

#define YM_FILE_INFO            (2)
#define YM_FILE_DATA            (1)
#define YM_VOIDER               (0)
#define YM_EXIT                 (-1)
#define YM_FEWDATE              (3)
#define YM_FINISHDATE           (4)


typedef enum {
    eYM_INIT,   
    eYM_RECE_HEAD_PACKET,
    eYM_RECE_DATA_START,
    eYM_RECE_DATA,
    eYM_END,   
}eYM_STAT;


/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
int YmodemReceive(char *pData, uint32_t pLen);

#endif  /* _YMODEM_H_ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
