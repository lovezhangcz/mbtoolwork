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


#ifndef __FILE_BSP_UART1_H__
#define __FILE_BSP_UART1_H__

#include <stdint.h>

enum
{
    UART_SUCCEED = 0,
    UART_FAILED  = 1
};

#define UART1_BUFFER_SIZE_DMA 256
#define UART1_BUFFER_SIZE_RECEIVE 256

struct tagUart{
	uint32_t head_receive;
	uint32_t tail_receive;
	uint32_t error_receive;
	char buffer_receive[UART1_BUFFER_SIZE_RECEIVE];
};

void uart_init(void);
void UsartClose(void);
void SerialPutChar(uint8_t c);
void Serial_PutString(uint8_t *s);
int32_t uart_receiveValidBufferSize(struct tagUart uart);
int32_t uart1_readReceiveBuffer(void * outputBuffer,int32_t outputBufferLength);
uint8_t uart_receivePushToBuffer(struct tagUart *uart,char* DMAData,uint32_t len);
int32_t uart_ValidBufferSize(struct tagUart uart);
int32_t uart1_readBuffer(struct tagUart *uart,void * outputBuffer,int32_t outputBufferLength);
void uart_rePushToBuffer(struct tagUart *uart,uint32_t len);

#endif // __FILE_BSP_UART1_H__
