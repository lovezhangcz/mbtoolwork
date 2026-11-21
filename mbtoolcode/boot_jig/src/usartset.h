#ifndef __USARTSET_H
#define __USARTSET_H

typedef char (*CPCALLBACKFUN)(u8 *pbuf);

#define USED_IDLE_U1
#define DMARXPACKAGELENGTH_U1   512
#define DMATXPACKAGELENGTH_U1   512
extern u8 UsartDMARxTable_u1[DMARXPACKAGELENGTH_U1];
extern u8 UsartDMATxTable_u1[DMATXPACKAGELENGTH_U1];

void USART_Initial_u1(u32 baudrate);
void MsgTxTrig_u1(void);
u8 BufferTxTrig_u1(u8 *data,u16 datalen);
extern CPCALLBACKFUN pcallbackfun_u1;

#define USED_IDLE_U2
#define DMARXPACKAGELENGTH_U2   128 //64
#define DMATXPACKAGELENGTH_U2   128 //64
extern u8 UsartDMARxTable_u2[DMARXPACKAGELENGTH_U2];
extern u8 UsartDMATxTable_u2[DMATXPACKAGELENGTH_U2];

void USART_Initial_u2(u32 baudrate);
void MsgTxTrig_u2(void);
u8 BufferTxTrig_u2(u8 *data,u16 datalen);
extern CPCALLBACKFUN pcallbackfun_u2;

#define USED_IDLE_U3
#define DMARXPACKAGELENGTH_U3   (512)
#define DMATXPACKAGELENGTH_U3   (512)
extern u8 UsartDMARxTable_u3[DMARXPACKAGELENGTH_U3];
extern u8 UsartDMATxTable_u3[DMATXPACKAGELENGTH_U3];


void USART_Initial_u3(u32 baudrate);
void MsgTxTrig_u3(void);
u8 BufferTxTrig_u3(u8 *data,u16 datalen);
extern CPCALLBACKFUN pcallbackfun_u3;
void USART_disable_u3(void);
void UsartClose_u1(void);
void UsartClose_u3(void);

u8 BufferTxTrig_bb(u8 *data,u16 datalen);
void USART_Initial_bb(u32 baudrate);
void UsartClose_bb(void);

#define USED_IDLE_U4
#define DMARXPACKAGELENGTH_U4   512
#define DMATXPACKAGELENGTH_U4   512
extern u8 UsartDMARxTable_u4[DMARXPACKAGELENGTH_U4];
extern u8 UsartDMATxTable_u4[DMATXPACKAGELENGTH_U4];

void USART_Initial_u4(u32 baudrate);
void MsgTxTrig_u4(void);
u8 BufferTxTrig_u4(u8 *data,u16 datalen);
extern CPCALLBACKFUN pcallbackfun_u4;
void UsartClose_u4(void);
#endif
