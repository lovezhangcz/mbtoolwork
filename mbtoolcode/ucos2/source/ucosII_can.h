/*----------------------------------------------------------------------------
 *      R T L  -  C A N   D r i v e r
 *----------------------------------------------------------------------------
 *      Name:    RTX_CAN.h
 *      Purpose: Header for CAN Generic Layer Driver
 *      Rev.:    V3.40
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2008 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include  "ucos_ii.h"                 /* RTX kernel functions & defines      */

#ifndef __RTX_CAN_H__
#define __RTX_CAN_H__

/* 
// *** <<< Use Configuration Wizard in Context Menu >>> *** 
*/
/*
// <o>Number of transmit objects for controllers <1-1024>
// <i> Determines the size of software message buffer for transmitting.
// <i> Default: 20
*/
#define CAN_No_SendObjects     20

/*
// <o>Number of receive objects for controllers <1-1024>
// <i> Determines the size of software message buffer for receiving.
// <i> Default: 20
*/
#define CAN_No_ReceiveObjects  20

/*
// <o>Number of receive objects for controllers <1-5>
// <i> Maximum index of used CAN controller 1 (only one CAN controller exists)
// <i> Needed for memory allocation for CAN messages.
// <i> Default: 1
*/
#define CAN_CTRL_MAX_NUM      1

/*
// *** <<< End of Configuration section             >>> *** 
*/

/* CAN message object structure                                              */
typedef struct  {
  INT32U id;                 /* 29 bit identifier                               */
  INT8U  data[8];            /* Data field                                      */
  INT8U  len;                /* Length of data field in bytes                   */
  INT8U  ch;                 /* Object channel                                  */
  INT8U  format;             /* 0 - STANDARD, 1- EXTENDED IDENTIFIER            */
  INT8U  type;               /* 0 - DATA FRAME, 1 - REMOTE FRAME                */
} CAN_msg;

/* Error values that functions can return                                    */
typedef enum   
{ CAN_OK = 0,                       /* No error                              */
  CAN_NOT_IMPLEMENTED_ERROR,        /* Function has not been implemented     */
  CAN_MEM_POOL_INIT_ERROR,          /* Memory pool initialization error      */
  CAN_BAUDRATE_ERROR,               /* Baudrate was not set                  */
  CAN_TX_BUSY_ERROR,                /* Transmitting hardware busy            */
  CAN_OBJECTS_FULL_ERROR,           /* No more rx or tx objects available    */
  CAN_ALLOC_MEM_ERROR,              /* Unable to allocate memory from pool   */
  CAN_DEALLOC_MEM_ERROR,            /* Unable to deallocate memory           */
  CAN_TIMEOUT_ERROR,                /* Timeout expired                       */
  CAN_UNEXIST_CTRL_ERROR,           /* Controller does not exist             */
  CAN_UNEXIST_CH_ERROR,             /* Channel does not exist                */
  CAN_HARDWARE_ERR,                 /* Hardware Err*/
}  CAN_ERROR;

/* Semaphores used for protecting writing to CAN hardware                    */
extern OS_EVENT *pwr_sem[CAN_CTRL_MAX_NUM];					

extern OS_EVENT *pCan_tx[CAN_CTRL_MAX_NUM];	 		    	//发送队列指针
extern OS_EVENT *pCan_rx[CAN_CTRL_MAX_NUM];	 		    	//接收队列指针
extern OS_MEM *pCan_Mem[CAN_CTRL_MAX_NUM];				        //内存指针

extern OS_EVENT *ptx_full_mbox[CAN_CTRL_MAX_NUM];

/* Symbolic names for formats of CAN message                                 */
typedef enum {STANDARD_FORMAT = 0, EXTENDED_FORMAT} CAN_FORMAT;

/* Symbolic names for type of CAN message                                    */
typedef enum {DATA_FRAME = 0, REMOTE_FRAME}         CAN_FRAME;

/* Functions defined in module CAN.c                                         */
CAN_ERROR CAN_init      (INT32U ctrl, INT32U baudrate);
CAN_ERROR CAN_start     (INT32U ctrl);
CAN_ERROR CAN_send      (INT32U ctrl, CAN_msg *msg, INT16U timeout);
CAN_ERROR CAN_request   (INT32U ctrl, CAN_msg *msg, INT16U timeout);
CAN_ERROR CAN_set       (INT32U ctrl, CAN_msg *msg, INT16U timeout);
CAN_ERROR CAN_receive   (INT32U ctrl, CAN_msg *msg, INT16U timeout);
CAN_ERROR CAN_rx_object (INT32U ctrl, INT32U ch, INT32U id, CAN_FORMAT filter_type);
CAN_ERROR CAN_rx_object_mask (INT32U ctrl, INT32U ch, INT32U id,INT32U mask, CAN_FORMAT format);
CAN_ERROR CAN_tx_object (INT32U ctrl, INT32U ch, CAN_FORMAT filter_type);

#endif /* __RTX_CAN_H__ */


/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/

