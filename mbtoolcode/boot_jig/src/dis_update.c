#include <stm32f10x.h>
#include "flash.h"
#include "string.h"
#include "stm32f10x_iwdg.h"
#include "dis_update.h"
#include "usartset.h" 

void aesDecInit(void);
void aesDecrypt( unsigned char * buffer, unsigned char * chainBlock );
unsigned char chainCipherBlock[16];

void bootstartset(void);

//u32 sumcheck_errorcnt=0;
//u32 sumcheck_resendreqcnt=0;
//u16 regetindexcheck=0;
//s16 regetindexcheck_dif=0;

//u32 sumcheck_id21sendcnt=0;
//u32 sumcheck_id21revcnt=0;
//s32 sumcheck_id21difcnt=0;

CDIS_DIS_POOL DisDataPool;

//u32 uprevcnt=0;
u8 dis_update_pro(u8 *pdata,u16 datalen)
{	
	u8 temsend[20]={0xAA,0x55,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	u8 tem[20+432+64];
	u16 crc16;
		
	if(datalen>100)
	{
		__nop();
	}
		
	for(u16 temi=0;temi<datalen;temi++)tem[temi]=pdata[temi];
	crc16=YModemCrc((char *)&tem[2],16,0);	
	tem[18]=crc16>>8;
	tem[19]=crc16;	
	if(!(tem[18]==pdata[18] && tem[19]==pdata[19]))	return 0;
	if(!((tem[17]<=(0x42) && tem[17]>=(0x40)) || tem[17]==(0x10)))return 0;	

				switch(tem[17])
				{				
					case (0x10)://APP发起升级,接收固件信息
					{
						bootstartset();
						DisDataPool.update_board=0;
						DisDataPool.dis_fwdata_crc16=0;
						DisDataPool.dis_fwdatasize_indx=0;
						DisDataPool.dis_fwdata_revindx=0;	

								temsend[16]=0x70;							
								temsend[17]=0x40;//切换协议，请求重置	
								crc16=YModemCrc((char *)&temsend[2],16,0);
								temsend[18]=crc16>>8;
								temsend[19]=crc16;
							DisDataPool.dis_fwdata_crc16=0;
							DisDataPool.dis_fwdatasize_indx=0;
				
					}break;
					case (0x40):		//APP发起升级,接收固件信息				
					{
						bootstartset();
						DisDataPool.update_board=0;
						DisDataPool.dis_fwdata_crc16=(tem[2]<<8)|(tem[3]);
						DisDataPool.dis_fwdatasize_indx=tem[14]|(tem[15]<<8);
						DisDataPool.dis_fwdata_revindx=0;
						DisDataPool.dis_fwdata_checkcrc16=0;
						//DisDataPool.bms_fwdata_checksum16=(tem[4]<<8)|(tem[5]);
						if(DisDataPool.dis_fwdatasize_indx<=DIS_FWDATA_PACKNUM_MAX)//固件正常，初始化，删除分区
						{
							DisDataPool.finishflg=0;
							DisDataPool.checkokflg=0;
							if(DisDataPool.flash_eraseflg==0)
							{
								flash_erase(DIS_FWDATA_ADDR_ERA_START,DisDataPool.dis_fwdatasize_indx*16);
								DisDataPool.flash_eraseflg=1;
								memset(chainCipherBlock,0x00,sizeof(chainCipherBlock));
								aesDecInit();								
							}
							
								temsend[2]=DisDataPool.dispc;
								temsend[3]=0;
								temsend[4]=0;
								temsend[5]=0;
								temsend[14]=DisDataPool.dis_fwdata_revindx;
								temsend[15]=DisDataPool.dis_fwdata_revindx>>8;	
								temsend[16]=0x70;							
								temsend[17]=0x41;//切换协议，请求重置	
								crc16=YModemCrc((char *)&temsend[2],16,0);
								temsend[18]=crc16>>8;
								temsend[19]=crc16;
							
						}else//固件过大不接受
						{

								temsend[16]=0x70;							
								temsend[17]=0x40;//切换协议，请求重置	
								crc16=YModemCrc((char *)&temsend[2],16,0);
								temsend[18]=crc16>>8;
								temsend[19]=crc16;
							
							DisDataPool.dis_fwdata_crc16=0;
							DisDataPool.dis_fwdatasize_indx=0;
						}						
						
					}break;
					case (0x41)://APP发来固件包
					{
							DisDataPool.update_board=0;
							if(DisDataPool.dis_fwdatasize_indx==0)//未初始，或失败，请求重置
							{	
								
								temsend[16]=0x70;							
								temsend[17]=0x40;//切换协议，请求重置	
								crc16=YModemCrc((char *)&temsend[2],16,0);
								temsend[18]=crc16>>8;
								temsend[19]=crc16;
								
								break;
							}
						
							u32 tempidx=DisDataPool.dis_fwdata_revindx;	
							DisDataPool.dispc=tempidx*100/DisDataPool.dis_fwdatasize_indx;
							
							u16 temindxpackcrc=(tem[2]<<8)|(tem[3]);			
							u16 temindxpacklen=tem[4]|(tem[5]<<8);
							
							if(DisDataPool.dis_fwdata_revindx<DisDataPool.dis_fwdatasize_indx)//升级完成检查，未完成，持续请求
							{
								u16 temindx=tem[14]|(tem[15]<<8);

								if(DisDataPool.dis_fwdata_revindx==temindx && temindxpacklen<=27)//递进符合，接收并请求下一包
								{					
									u16 temdatapackcrc=YModemCrc((char*)&tem[20],temindxpacklen*16,0);	
									
									if(temindxpackcrc==temdatapackcrc)
									{					
										DisDataPool.dis_fwdata_checkcrc16=YModemCrc((char*)&tem[20],temindxpacklen*16,DisDataPool.dis_fwdata_checkcrc16);	
										for(u8 temi=0;temi<temindxpacklen;temi++)
										{
											aesDecrypt(&tem[20+temi*16],chainCipherBlock);
										}
										flash_write(DIS_FWDATA_ADDR_START+DisDataPool.dis_fwdata_revindx*16,(u32*)&tem[20],temindxpacklen*16);
										
										DisDataPool.dis_fwdata_revindx+=temindxpacklen;
										
										DisDataPool.flash_eraseflg=0;
									}
									
								}

								temsend[2]=DisDataPool.dispc;
								temsend[3]=0;
								temsend[4]=0;
								temsend[5]=0;
								temsend[14]=DisDataPool.dis_fwdata_revindx;
								temsend[15]=DisDataPool.dis_fwdata_revindx>>8;	
								temsend[16]=0x70;							
								temsend[17]=0x41;//切换协议，请求重置	
								crc16=YModemCrc((char *)&temsend[2],16,0);
								temsend[18]=crc16>>8;
								temsend[19]=crc16;								
	
							}else//升级接收完毕
							{
								if(DisDataPool.finishflg<2)//等待主循环较验结果
								{
									DisDataPool.finishflg=1;

									temsend[2]=DisDataPool.dispc;
									temsend[3]=0;
									temsend[4]=0;
									temsend[5]=0;
									temsend[14]=0xff;
									temsend[15]=0xff;	
									temsend[16]=0x70;							
									temsend[17]=0x41;//
									crc16=YModemCrc((char *)&temsend[2],16,0);
									temsend[18]=crc16>>8;
									temsend[19]=crc16;									
								}else
								{
									if(DisDataPool.checkokflg==0)
									{
										temsend[2]=0;//DisDataPool.dispc;
										temsend[3]=0;
										temsend[4]=0;
										temsend[5]=0;
										temsend[14]=0xff;
										temsend[15]=0xff;	
										temsend[16]=0x70;							
										temsend[17]=0x42;//
										crc16=YModemCrc((char *)&temsend[2],16,0);
										temsend[18]=crc16>>8;
										temsend[19]=crc16;
										
									}else{

										temsend[2]=100;//DisDataPool.dispc;
										temsend[3]=0;
										temsend[4]=0;
										temsend[5]=0;
										temsend[6]=1;									
										temsend[14]=0xff;
										temsend[15]=0xff;	
										temsend[16]=0x70;							
										temsend[17]=0x42;//
										crc16=YModemCrc((char *)&temsend[2],16,0);
										temsend[18]=crc16>>8;
										temsend[19]=crc16;
									}
								}
							}

					}break;
				}
				
#if defined(MODEM_KS_M12)||defined(MODEM_KS_MTB1)||defined(MODEM_KS_MTB7)||defined(MODEM_KS_FX1)\
		||defined(MODEM_KS_MTB1R)||defined(MODEM_KS_JIG)
		{u32 temdlycnt=60000;while(temdlycnt--);}				
			BufferTxTrig_bb(temsend,20);
#else				
		extern u8 displaybybte[2];				
		u8 temtxbuf[32+20+10+4];
		u8 tempdbuf[32];
		tempdbuf[0]=displaybybte[0];	
		tempdbuf[1]=displaybybte[1];				
		crcinpack(tempdbuf,0xdd,temsend,20,temtxbuf);	
				
		{u32 temdlycnt=60000;while(temdlycnt--);}				
			BufferTxTrig_bb(temtxbuf,20+10+4);
#endif

	DisDataPool.msgcntflg=0;
	DisDataPool.msgcnt++;
}
u8 updatacheck(void)
{
	if(DisDataPool.dis_fwdata_checkcrc16==DisDataPool.dis_fwdata_crc16)
	{
		return 1;
	}
		return 0;	
}
unsigned short int YModemChecksum(char *pData, unsigned long sLen)
{  
   unsigned short int cksum = 0;
   unsigned short int i = 0;
   while (sLen--)  //len是所要计算的长度
   {
       cksum+=(*pData++); //    
   }
   return cksum;
}
unsigned short int YModemCrc(char *pData, unsigned long sLen,unsigned short int crc)
{  
   //unsigned short int crc = 0;
   unsigned short int i = 0;
   while (sLen--)  //len是所要计算的长度
   {
       crc = crc^(int)(*pData++) << 8; //    
       for (i=8; i!=0; i--) 
       {
           if (crc & 0x8000)   
               crc = crc << 1 ^ 0x1021;    
           else
               crc = crc << 1;
       }    
   }
   return crc;
}



