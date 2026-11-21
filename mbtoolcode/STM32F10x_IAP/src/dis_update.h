
#ifndef __DIS_UPDATE_H
#define __DIS_UPDATE_H

//#define ApplicationAddress  (0x8002800)   				

#define DIS_FWDATA_ADDR_ERA_START 	(0x8002800)
#define DIS_FWDATA_ADDR_START 	(0x8002800)
//#define DIS_FWDATA_ADDR_MAXEND 	0x800F000
//#define DIS_FWDATA_ADDR_SIZE 		0x800C800
#define DIS_FWDATA_PACKNUM_MAX	5000 //3200


#define DISFW_POOL_LEN 32
typedef struct
{
	u16 dis_fwdatasize_indx;
	u16 dis_fwdata_revindx;		
	u16 dis_fwdata_crc16;
	u16 dis_fwdata_checkcrc16;
	
	u16 dispc;
	u8 finishflg;//下载完成，主循环执行较验
	u8 checkokflg;//主循环较验结果返馈
	u8 update_board;
	u8 flash_eraseflg;
	u16 msgcntflg;
	u16 msgcnt;	
	u32 jumpdelay;
}CDIS_DIS_POOL;

extern CDIS_DIS_POOL DisDataPool;

u8 updatacheck(void);
unsigned short int YModemChecksum(char *pData, unsigned long sLen);
unsigned short int YModemCrc(char *pData, unsigned long sLen,unsigned short int crc);
u8 dis_update_pro(u8 *pdata,u16 datalen);

#endif


