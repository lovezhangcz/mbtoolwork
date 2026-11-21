/*----------------------------------------------------------------------------
 *      R T L  -  C A N   D r i v e r
 *----------------------------------------------------------------------------
 *      Name:    RTX_CAN.c
 *      Purpose: CAN Generic Layer Driver
 *      Rev.:    V4.00
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2009 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include <ucos_ii.h>                  /* RTX kernel functions & defines      */
#include <stdio.h>
#include "ucosII_can.h"               /* CAN Generic functions & defines     */
#include "can_hw.h"                   /* CAN hw specific functions & defines */
#include "stm32f10x.h"

#ifdef DEBUG_CAN

#pragma diag_suppress 550



/* Declare memory pool for CAN messages, both transmit and receive           */
static CAN_msg CAN_mpool[CAN_CTRL_MAX_NUM][CAN_No_SendObjects+CAN_No_ReceiveObjects]; 

/* Declare queue, for CAN transmit messages                                */
static void *Can_tx_Q[CAN_CTRL_MAX_NUM][CAN_No_SendObjects];

/* Declare queue, for CAN receive messages                                 */
static void *Can_rx_Q[CAN_CTRL_MAX_NUM][CAN_No_ReceiveObjects];

OS_EVENT *pCan_tx[CAN_CTRL_MAX_NUM];	 		    	//发送队列指针
OS_EVENT *pCan_rx[CAN_CTRL_MAX_NUM];	 		    	//接收队列指针
OS_MEM *pCan_Mem[CAN_CTRL_MAX_NUM];				//内存指针

OS_EVENT *ptx_full_mbox[CAN_CTRL_MAX_NUM];			//邮箱满二值信号邮箱


/*----------------------------------------------------------------------------
 *      CAN RTX Generic Driver Functions
 *----------------------------------------------------------------------------
 *  Functions implemented in this module:
 *           CAN_ERROR CAN_mem_init  (void);
 *           CAN_ERROR CAN_setup     (void)
 *           CAN_ERROR CAN_init      (INT32U ctrl, INT32U baudrate)
 *           CAN_ERROR CAN_start     (INT32U ctrl)
 *    static CAN_ERROR CAN_push      (INT32U ctrl, CAN_msg *msg, INT16U timeout)
 *           CAN_ERROR CAN_send      (INT32U ctrl, CAN_msg *msg, INT16U timeout)
 *           CAN_ERROR CAN_request   (INT32U ctrl, CAN_msg *msg, INT16U timeout)
 *           CAN_ERROR CAN_set       (INT32U ctrl, CAN_msg *msg, INT16U timeout)
 *    static CAN_ERROR CAN_pull      (INT32U ctrl, CAN_msg *msg, INT16U timeout)
 *           CAN_ERROR CAN_receive   (INT32U ctrl, CAN_msg *msg, INT16U timeout)
 *           CAN_ERROR CAN_rx_object (INT32U ctrl, INT32U ch, INT32U id, CAN_FORMAT format)
 *           CAN_ERROR CAN_tx_object (INT32U ctrl, INT32U ch, CAN_FORMAT format)
 *---------------------------------------------------------------------------*/


/*--------------------------- CAN_init --------------------------------------
 *
 *  The first time this function is called initialize the memory pool for 
 *  CAN messages and setup CAN controllers hardware
 *
 *  Initialize mailboxes for CAN messages and initialize CAN controller
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              baudrate:   Baudrate
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_init (INT32U ctrl, INT32U baudrate)  
{
	static INT8U first_run_flag = 0;
	CAN_ERROR error_code;
	INT32U ctrl0 = ctrl-1;                 /* Controller index 0 .. x-1           */
	
	INT8U err;

	ptx_full_mbox[ctrl0] = OSMboxCreate((void *)1);
	
	/* When function is called for the first time it will initialize and setup 
	 all of the resources that are common to CAN functionality               */
	if (first_run_flag == 0)  
	{
		first_run_flag = 1;
		pCan_Mem[ctrl0] = OSMemCreate(&CAN_mpool[ctrl0][0], (CAN_No_SendObjects+CAN_No_ReceiveObjects), sizeof(CAN_msg), &err);
		if (err != OS_ERR_NONE)
			return CAN_MEM_POOL_INIT_ERROR;
	}
	
	pCan_tx[ctrl0] = OSQCreate(&Can_tx_Q[ctrl0][0], CAN_No_SendObjects);
	pCan_rx[ctrl0] = OSQCreate(&Can_rx_Q[ctrl0][0], CAN_No_ReceiveObjects);
	
	error_code = CAN_hw_setup (ctrl);
	if (error_code != CAN_OK) 								
		return error_code;
	
	return (CAN_hw_init (ctrl, baudrate));
}


/*--------------------------- CAN_start -------------------------------------
 *
 *  Start CAN controller (enable it to participate on CAN network)
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_start (INT32U ctrl) 
{
	return (CAN_hw_start (ctrl));
}


/*--------------------------- CAN_push --------------------------------------
 *
 *  Send CAN_msg if hardware is free for sending, otherwise push message to 
 *  message queue to be sent when hardware becomes free
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be sent
 *              timeout:    Timeout value for message sending
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/
static CAN_ERROR CAN_push (INT32U ctrl, CAN_msg *msg, INT16U timeout)  
{
	CAN_msg *ptrmsg;
	INT32U ctrl0 = ctrl-1;                 /* Controller index 0 .. x-1           */
	INT8U err; 
	
	if (CAN_hw_tx_empty (ctrl) == CAN_OK) 
	{ /* Transmit hardware free for send */
		CAN_hw_wr (ctrl, msg);            /* Send message                        */
	}
	else 
	{                              /* If hardware for sending is busy     */
		/* Write the message to send mailbox if there is room for it             */
		ptrmsg = OSMemGet(pCan_Mem[ctrl0], &err); 
		if (ptrmsg != NULL) 
		{
			*ptrmsg = *msg;
			/* If message hasn't been sent but timeout expired, deallocate memory  */
			err = OSQPost(pCan_tx[ctrl0], (void *)ptrmsg);
				
			if (err == OS_ERR_NONE)
			{
				if (CAN_hw_tx_empty (ctrl) == CAN_OK) 
				{ /* Transmit hw free for send */
					ptrmsg = OSQPend(pCan_tx[ctrl0], timeout, &err); 
					if (err != OS_ERR_NONE)
					{
						return CAN_TIMEOUT_ERROR;
					}  
					if (OSMemPut(pCan_Mem[ctrl0], (void *)ptrmsg) != OS_ERR_NONE)
						return CAN_DEALLOC_MEM_ERROR;
					CAN_hw_wr (ctrl, msg);      /* Send message                        */
				}
			}
			else
			{		//邮箱满
				OSMboxAccept (ptx_full_mbox[ctrl0]);	//清信号量
				OSMboxPend (ptx_full_mbox[ctrl0], timeout, &err); //等待信号量
				if(err == OS_ERR_TIMEOUT)
				{
					err = OSQPost(pCan_tx[ctrl0], (void *)ptrmsg);
					if(err != OS_ERR_NONE)
					{
						if (OSMemPut(pCan_Mem[ctrl0], (void *)ptrmsg) != OS_ERR_NONE)
							return CAN_DEALLOC_MEM_ERROR;
						
						return CAN_OBJECTS_FULL_ERROR;
					}
				}
				else
				{
					CAN_hw_wr (ctrl, msg); 
				}
			}
		} 
		else
			return CAN_ALLOC_MEM_ERROR;
	}
	return CAN_OK;
}


/*--------------------------- CAN_send --------------------------------------
 *
 *  Send DATA FRAME message, see CAN_push function comment
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be sent
 *              timeout:    Timeout value for message sending
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_send (INT32U ctrl, CAN_msg *msg, INT16U timeout)  
{
	msg->type = DATA_FRAME;
	
	return (CAN_push (ctrl, msg, timeout));
}


/*--------------------------- CAN_request -----------------------------------
 *
 *  Send REMOTE FRAME message, see CAN_push function comment
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be sent
 *              timeout:    Timeout value for message sending
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_request (INT32U ctrl, CAN_msg *msg, INT16U timeout)  
{
	msg->type = REMOTE_FRAME;
	
	return (CAN_push (ctrl, msg, timeout));
}

/*--------------------------- CAN_pull --------------------------------------
 *
 *  Pull first received and unread CAN_msg from receiving message queue
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer where CAN message will be read
 *              timeout:    Timeout value for message receiving
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/
static CAN_ERROR CAN_pull (INT32U ctrl, CAN_msg *msg, INT16U timeout)  
{
	CAN_msg *ptrmsg = NULL;
	INT32U ctrl0 = ctrl-1;                 /* Controller index 0 .. x-1           */
	INT8U err;
	
	/* Wait for received message in mailbox                                     */
	ptrmsg = OSQPend(pCan_rx[ctrl0], timeout, &err); 
	if (err != OS_ERR_NONE)
	{
		return CAN_TIMEOUT_ERROR;
	}  
	
	if(ptrmsg)
	{
		/* Copy received message from mailbox to address given in function parameter msg */
		*msg = *ptrmsg;
		
		/* Free box where message was kept                                         */
		if (OSMemPut(pCan_Mem[ctrl0], (void *)ptrmsg) != OS_ERR_NONE)
			return CAN_DEALLOC_MEM_ERROR;
		return CAN_OK;
	}	  

	return CAN_DEALLOC_MEM_ERROR;
}


/*--------------------------- CAN_receive -----------------------------------
 *
 *  Read received message, see CAN_pull function comment
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer where CAN message will be read
 *              timeout:    Timeout value for message receiving
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_receive (INT32U ctrl, CAN_msg *msg, INT16U timeout)  
{
	return (CAN_pull (ctrl, msg, timeout));
}


/*--------------------------- CAN_rx_object ---------------------------------
 *
 *  Enable reception of messages on specified controller and channel with 
 *  specified identifier
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              ch:         Channel for the message transmission
 *              id:         CAN message identifier
 *              CAN_FORMAT: Format of CAN identifier (standard or extended)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_rx_object (INT32U ctrl, INT32U ch, INT32U id, CAN_FORMAT format)  
{
	return (CAN_hw_rx_object (ctrl, ch, id, format));
}

/*--------------------------- CAN_rx_object ---------------------------------
 *
 *  Enable reception of messages on specified controller and channel with 
 *  specified identifier
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              ch:         Channel for the message transmission
 *              id:         CAN message identifier
 *              CAN_FORMAT: Format of CAN identifier (standard or extended)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_rx_object_mask (INT32U ctrl, INT32U ch, INT32U id,INT32U mask, CAN_FORMAT format)  
{
	return CAN_hw_rx_object_mask (ctrl, ch, id, mask, format);
}


/*--------------------------- CAN_tx_object ---------------------------------
 *
 *  Enable transmission of messages on specified controller and channel with 
 *  specified identifier
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              ch:         Channel for the message transmission
 *              CAN_FORMAT: Format of CAN identifier (standard or extended)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_tx_object (INT32U ctrl, INT32U ch, CAN_FORMAT format)  
{
	return (CAN_hw_tx_object (ctrl, ch, format));
}


#endif /*DEBUG_CAN*/
/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/

