/************************************************
**Filename:Rlcprimitives.c
**Copyright:uestc
**Descriptions:
**v1.0,2013,ChenHongying  written
**---------------------------------------------
**modification history:
**Modified by: ZhangYi
**Modified date: 2014
**Descriptions:
**
************************************************/
#include "rlc_macfsm.h"
#include "Rlcprimitives.h"
#include "Rlctypes.h"
#include "../lte_system.h"



/**********************************************************
**Function name: creat_subhead()
**Descriptions: creat MAC subhead
**Input: FSM_PKT * skb,u8 lcid
**Output:  void **ptr, void **preptr, u32 *data
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
static void creat_subhead(FSM_PKT * skb, void **ptr, void **preptr, u8 lcid, u32 *data)
{
	struct MRLC_subHead_fif_IciMsg * subfif;
	struct MRLC_subHead_sev_IciMsg *subsev;
	
	//fsm_printf("entering creat_subhead\n");
	if(skb->len < 128)
	{
		subsev = (MRLC_subHead_sev_IciMsg *)*ptr;
		subsev->fix.R1 = subsev->fix.R2 = 0;
		subsev->fix.E = 1;
		subsev->fix.lcid = lcid;
		subsev->fandl = skb->len;
		subsev->fandl  &= 0x7f;
		*preptr = subsev;
		*ptr = subsev+1;
		*data += 2;
		//fsm_printf("the data is %d\n",  *data);
	}
	else
	{
		subfif= (MRLC_subHead_fif_IciMsg *)*ptr;
		subfif->fix.R1 = subfif->fix.R2 = 0;
		subfif->fix.E = 1;
		subfif->fix.lcid = lcid;
		subfif->fandl1 = skb->len>>8;
		subfif->fandl1 |= 0x80;
		subfif->fandl2 = skb->len&0xff;
		//fsm_octets_print(&subfif->fandl1,2);
		//subfif->fandl = subfif->fandl;
		*preptr = subfif;
		*ptr = subfif + 1;
		*data += 3;
		//fsm_printf("the data is %d\n",  *data);
	}
}

/**********************************************************
**Function name: check_retx_num()
**Descriptions: check the num of AM retx buffer
**Input: struct AM_Instance *amIns,long size
**Output: u32 *sevNum, u32 *fifNum
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
static void check_retx_num(struct AM_Instance *amIns,long size, u32 *sevNum, u32 *fifNum)
{
	struct AmBuffer *amBuffer;
	
	if(!list_empty(&amIns->amRetxBuffer.list))
	{
		list_for_each_entry(amBuffer,&amIns->amRetxBuffer.list,list)
		{
			//fsm_printf("[RLC][check_retx_num] the packet's len is %d, the size is %d\n",amBuffer->pkt->len,size);
			size -= amBuffer->pkt->len;
			if(0 <= size)
			{
				if(amBuffer->pkt->len  <  128)
				{
					(*sevNum)++;
				}
				else
				{
					(*fifNum)++;
				}
			}
			else
			{
				if(( size + amBuffer->pkt->len + 2 ) < 128)
				{
					(*sevNum)++;
				}
				else
				{
					(*fifNum)++;
				}
				break;
			}
		}
	}
	FOUT;
}

/**********************************************************
**Function name: check_stat_num()
**Descriptions: check the num of AM ctrl buffer
**Input: struct AM_Instance *amIns,long size
**Output: u32 *sevNum, u32 *fifNum
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
static void check_stat_num(struct AM_Instance *amIns,long size, u32 *sevNum, u32 *fifNum)
{
	struct Buffer *statBuffer;
	
	if(!list_empty(&amIns->ctrlPduBuffer.list))
	{
		list_for_each_entry(statBuffer,&amIns->ctrlPduBuffer.list,list)
		{
			//fsm_printf("[RLC][check_stat_num] the packet's len is %d, the size is %d\n",statBuffer->pkt->len,size);
			size -= statBuffer->pkt->len;
			if(0 <= size)
			{
				if(statBuffer->pkt->len< 128)
				{
					(*sevNum)++;
				}
				else
				{
					( *fifNum)++;
				}
			}
			else
			{
				break;
			}
		}
	}
	FOUT;
}

/**********************************************************
**Function name: check_num()
**Descriptions: check the num of TM buffer
**Input: TM_TX_Instance * tmTxIns, long size
**Output: u32 *sevNum, u32 *fifNum
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
static void check_num(TM_TX_Instance * tmTxIns, long size, u32 *sevNum, u32 *fifNum )
{
	struct Buffer *txBuffer;
	
	if(!list_empty(&tmTxIns->tmSduBuffer.list))
	{
		list_for_each_entry(txBuffer,&tmTxIns->tmSduBuffer.list,list)
		{
			size -= txBuffer->pkt->len;
			//fsm_printf("[RLC][check_num] the packet's len is %d, the size is %d\n",txBuffer->pkt->len, size);
			if(0 <= size)
			{
				if(txBuffer->pkt->len < 127)
				{
					(*sevNum)++;
				}
				else
				{
					(*fifNum)++;
				}
			}
			else
			{
				break;
			}
		}
	}
	FOUT;
}

/**********************************************************
**Function name: PCRLC_CONFIG_REQ()
**Descriptions: Config AM or UM instance
**Input:
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
void PCRLC_CONFIG_REQ(void)
{
	CRLC_ReConfigReq_IoctrlMsg *cIoctrl = (CRLC_ReConfigReq_IoctrlMsg*)fsm_data_get();
	//  struct list_head* pos;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	void *insptru,*insptrd;
	
	SV_PTR_GET(rlc_mac_sv);
	findInsByRbid(cIoctrl->rbIdentity ,&insptru, &insptrd, &SV(ins_mode));
	fsm_printf("[RLC][PCRLC_CONFIG_REQ] entering PCRLC_CONFIG_REQ\n ");
	switch(cIoctrl->Mode)
	{
	case AM_UPDOWN:                      //AM的上下行配置
		if(SV(ins_mode) == AM_MODE)
		{
			if(insptru == NULL)
			{
				break;
			}
			amIns = (AM_Instance *)insptru;
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] PCRLC_CONFIG_REQ:am instance has configed(rbid is %d)\n",cIoctrl->rbIdentity);
			amIns->reorderTimerValue= cIoctrl->amDwparas.timerReordering;
			amIns->statProhTimerValue = cIoctrl->amDwparas.timerStatusProhibit;
			amIns->pollRetxTimerValue = cIoctrl->amUpParas.timerPollRetransmit;
			amIns->pollPDU = cIoctrl->amUpParas.PollPDU;
			amIns->pollBYTE = cIoctrl->amUpParas.PollBYTE;
			amIns->maxRetxThreshold = cIoctrl->amUpParas.maxRetxThreshold;
			break;
		}
		else
		{
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] case1:there is no AM instance to be recinfiged!\n");
		}
		break;

	case UM_UPDOWN:              	 //UM的上下行配置
		if(SV(ins_mode) == UM_MODE)
		{
			if(insptru != NULL)
			{
				umTxIns = (UM_TX_Instance *)insptru;
				fsm_printf("[RLC][PCRLC_CONFIG_REQ] PCRLC_CONFIG_REQ:um tran instance has configed(rbid is %d)\n",cIoctrl->rbIdentity);
				umTxIns->snFiledLength = cIoctrl->umUpParas.snFiledLength;
			}

			umRxIns = (UM_RX_Instance *)insptrd;
			if(umRxIns == NULL)
			{
				break;
			}
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] PCRLC_CONFIG_REQ:um recv instance has configed(rbid is %d)\n",cIoctrl->rbIdentity);
			umRxIns->snFiledLength = cIoctrl->umDwParas.snFiledLength;
			umRxIns->reorderTimerValue = cIoctrl->umDwParas.timerReordering;
		}
		else 
		{
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] case2:there is no UM  instance to be recinfiged!\n");
		}
		break;

	case UM_UP:			//UM的上行配置
		if(SV(ins_mode) ==  UM_MODE)
		{
			if(insptru == NULL)
			{
				break;
			}
			umTxIns = (UM_TX_Instance *)insptru;
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] PCRLC_CONFIG_REQ:um tran instance has configed(rbid is %d)\n",cIoctrl->rbIdentity);
			umTxIns->snFiledLength = cIoctrl->umUpParas.snFiledLength;
		}
		else 
		{
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] case3:there is no UM TX instance to be recinfiged!\n");
		}
		break;

	case UM_DOWN:			//UM的下行配置
		if(SV(ins_mode) == UM_MODE)
		{
			if(insptrd == NULL)
			{
				break;
			}
			umRxIns = (UM_RX_Instance *)insptrd;
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] PCRLC_CONFIG_REQ:um recv instance has configed(rbid is %d)\n",cIoctrl->rbIdentity);
			umRxIns->snFiledLength = cIoctrl->umDwParas.snFiledLength;
			umRxIns->reorderTimerValue = cIoctrl->umDwParas.timerReordering;
		}
		else 
		{
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] case 4:there is no UM RX instance to be recinfiged!\n");
		}
		break;

	default:
		fsm_printf("[RLC][PCRLC_CONFIG_REQ] wrong CRLC_CONFIG_REQ\n");
		break;

	}
	fsm_data_destroy(cIoctrl);
	cIoctrl = NULL;
	FOUT;
}

/**********************************************************
**Function name: PCRLC_BULID_REQ()
**Descriptions: Build AM or UM instance
**Input:
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
void PCRLC_BULID_REQ(void)
{
	struct	AM_Instance * amIns;
	struct	UM_RX_Instance *umRxIns;
	struct	UM_TX_Instance *umTxIns;
	CRLC_BuildReq_IoctrlMsg *cIoctrl;
	CRLC_RbidBuild_IOCTRLMsg *cItrl;
	int i = 0;
	
	SV_PTR_GET(rlc_mac_sv);
	fsm_printf("[RLC][PCRLC_BULID_REQ] entering PCRLC_BULID_REQ\n ");
	
	cIoctrl = (CRLC_BuildReq_IoctrlMsg*)fsm_data_get();
	switch(cIoctrl->Mode)
	{
	case AM_UPDOWN:                      //AM的上下行配置
		amIns =  instance_create(struct AM_Instance);
		amIns->rbId = cIoctrl->rbIdentity;
		//fsm_printf("the rbid is %d\n", amIns->rbId);
		amIns->lcId = cIoctrl->lcIdentity;
		fsm_printf("[RLC][PCRLC_BULID_REQ] build am instance lcid is %d\n",amIns->lcId);
		amIns->reorderTimerValue= cIoctrl->amDwparas.timerReordering*100;
		amIns->statProhTimerValue = cIoctrl->amDwparas.timerStatusProhibit*100;
		amIns->pollRetxTimerValue = cIoctrl->amUpParas.timerPollRetransmit*100;
		amIns->pollPDU = cIoctrl->amUpParas.PollPDU;
		amIns->pollBYTE = cIoctrl->amUpParas.PollBYTE;
		amIns->maxRetxThreshold = cIoctrl->amUpParas.maxRetxThreshold;
		amIns->amRetxBufferSize = amIns->amSduBufferSize = amIns->ctrlPduBufferSize = amIns->amTransmittedBufferSize = 0;
		amIns->amRetxBufferNum = amIns->amSduBufferNum = amIns->ctrlPduBufferNum = amIns->amTransmittedBufferNum = 0;
		amIns->pollRetxTimer = amIns->reorderTimer = amIns->statProhTimer = 0;
		amIns->currentStatVar = ST_TRAN;
		amIns->pduLft.sduLeft = NULL;
		amIns->pduLft.SN_Left = -1;
		amIns->pduLft.SN5_Left = -1;
		//      amIns->pollRetxTimerCode =(int)amIns % 65536;                                 //根据实体地址设置定时器
		//      amIns->reorderTimerCode = ((int)amIns + 1) %65536;
		//     amIns->statProhTimerCode = ((int)amIns + 2) %65536;
		//分配定时器code值
		while((SV(allocCode[i++]) == -1)&&(i < MAX_CODE));
		if(i >= MAX_CODE) 
		{
			fsm_printf("[RLC][PCRLC_BULID_REQ] no more code to allocated \n");
		}
		else
		{
			amIns->pollRetxTimerCode = i - 1;
			SV(allocCode[i - 1]) = -1;
		}
		while((SV(allocCode[i++]) == -1)&&(i<MAX_CODE));
		if(i >= MAX_CODE) 
		{
			fsm_printf("[RLC][PCRLC_BULID_REQ] no more code to allocated \n");
		}
		else
		{
			amIns->reorderTimerCode = i - 1;
			SV(allocCode[i - 1]) = -1;
		}
		while((SV(allocCode[i++]) == -1)&&(i<MAX_CODE));
		if(i >= MAX_CODE) 
		{
			fsm_printf("[RLC][PCRLC_BULID_REQ] no more code to allocated \n");
		}
		else
		{
			amIns->statProhTimerCode = i - 1;
			SV(allocCode[i - 1]) = -1;
		}
		amIns->vt_s = amIns->vt_a = amIns->vr_x = amIns->vr_h = amIns->vr_ms = amIns->vr_r = 0;
		amIns->vr_mr = amIns->vt_ms = AM_WINDOW_SIZE;
		amIns->SN = 0;
		amIns->statFlag = false;
		fsm_printf("[RLC][PCRLC_BULID_REQ] init SN is %d\n",amIns->SN);
		INIT_LIST_HEAD(&amIns->amSduBuffer.list);
		INIT_LIST_HEAD(&amIns->amRecvBuffer.list);
		INIT_LIST_HEAD(&amIns->amRetxBuffer.list);
		INIT_LIST_HEAD(&amIns->amTransmittedBuffer.list);
		INIT_LIST_HEAD(&amIns->ctrlPduBuffer.list);
		for(i = 0;  i < 1024; ++i )
		{
			amIns->amRecvWindow[i] = NULL;
		}
		for(i = 0;  i < 1024; ++i )
		{
			amIns->amTranWindow[i]= NULL;
		}
		list_add_tail(&amIns->amList, &SV(vAm_ins).amList);
		fsm_printf("[RLC][PCRLC_BULID_REQ] PCRLC_BULID_REQ: am instance has established(rbid is %d)\n",amIns->rbId);
		break;

	case UM_UPDOWN:              	 //UM的上下行配置
		umRxIns =  instance_create(struct UM_RX_Instance);
		umRxIns->snFiledLength = cIoctrl->umDwParas.snFiledLength;
		umRxIns->reorderTimerValue = cIoctrl->umDwParas.timerReordering*100;
		umRxIns->currentStatVar = ST_TRAN;
		umRxIns->lcId = cIoctrl->lcIdentity;
		umRxIns->rbId = cIoctrl->rbIdentity;
		umRxIns->SN = umRxIns->vr_uh = umRxIns->vr_ur = umRxIns->vr_ux = 0;
		umRxIns->reorderTimer = 0;
		umRxIns->pduLft.sduLeft = NULL;
		umRxIns->pduLft.SN_Left = -1;
		umRxIns->pduLft.SN5_Left = -1;
		//  umRxIns->reorderTimerCode =( int)umRxIns % 65536;
		//分配定时器code值
		while((SV(allocCode[i++]) == -1)&&(i < MAX_CODE));
		if(i >= MAX_CODE) 
		{
			fsm_printf("[RLC][PCRLC_BULID_REQ] no more code to allocated \n");
		}
		else
		{
			umRxIns->reorderTimerCode = i - 1;
			SV(allocCode[i - 1]) = -1;
		}
		INIT_LIST_HEAD(&umRxIns->umRecvBuffer.list);
		//if(umRxIns->snFiledLength == 5)
		//{
		for(i = 0; i < 32; ++i)
		{
			umRxIns->umRecvWindow5[i] = NULL;
		}
		//}
		//else if(umRxIns->snFiledLength == 10)
		//{
		for(i = 0; i < 1024; ++i)
		{
			umRxIns->umRecvWindow10[i] = NULL;
		}
		//}
		list_add_tail(&umRxIns->umRxList, &SV(vUm_rx_ins).umRxList);
		//   fsm_printf("PCRLC_BULID_REQ: um recv instance has established(rbid is %d)\n",umRxIns->rbId);

		//上行
		umTxIns =  instance_create(UM_TX_Instance);
		umTxIns->snFiledLength = cIoctrl->umUpParas.snFiledLength;
		umTxIns->lcid = cIoctrl->lcIdentity;
		umTxIns->rbid = cIoctrl->rbIdentity;
		umTxIns->CurrentStatVar = ST_TRAN;
		umTxIns->SN=0;
		umTxIns->umSduBufferNum = umTxIns->umSduBufferSize = 0;
		INIT_LIST_HEAD(&umTxIns->umSduBuffer.list);
		list_add_tail(&umTxIns->umTxList, &SV(vUm_tx_ins).umTxList);
		fsm_printf("[RLC][PCRLC_BULID_REQ] PCRLC_BULID_REQ: um tran and recv instance has established(rbid is %d)\n",umTxIns->rbid);
		break;

	case UM_UP:			//UM的上行配置
		umTxIns =  instance_create(UM_TX_Instance);
		umTxIns->snFiledLength = cIoctrl->umUpParas.snFiledLength;
		umTxIns->lcid = cIoctrl->lcIdentity;
		umTxIns->rbid = cIoctrl->rbIdentity;
		umTxIns->CurrentStatVar = ST_TRAN;
		umTxIns->SN=0;
		umTxIns->umSduBufferNum = umTxIns->umSduBufferSize = 0;
		INIT_LIST_HEAD(&umTxIns->umSduBuffer.list);
		list_add_tail(&umTxIns->umTxList, &SV(vUm_tx_ins).umTxList);
		fsm_printf("[RLC][PCRLC_BULID_REQ] PCRLC_BULID_REQ: um tran instance has established(rbid is %d)\n",umTxIns->rbid);
		break;

	case UM_DOWN:			//UM的下行配置
		umRxIns =  instance_create(struct UM_RX_Instance);
		umRxIns->snFiledLength = cIoctrl->umDwParas.snFiledLength;
		umRxIns->reorderTimerValue = cIoctrl->umDwParas.timerReordering*100;
		umRxIns->currentStatVar = ST_TRAN;
		umRxIns->lcId = cIoctrl->lcIdentity;
		umRxIns->rbId = cIoctrl->rbIdentity;
		umRxIns->SN = umRxIns->vr_uh = umRxIns->vr_ur = umRxIns->vr_ux = 0;
		umRxIns->reorderTimer = 0;
		umRxIns->pduLft.sduLeft = NULL;
		umRxIns->pduLft.SN_Left = -1;
		umRxIns->pduLft.SN5_Left = -1;
		//  umRxIns->reorderTimerCode =( int)umRxIns % 65536;
		//分配定时器code值
		while((SV(allocCode[i++]) == -1)&&(i < MAX_CODE));
		if(i >= MAX_CODE) 
		{
			fsm_printf("[RLC][PCRLC_BULID_REQ] no more code to allocated \n");
		}
		else
		{
			umRxIns->reorderTimerCode = i - 1;
			SV(allocCode[i - 1]) = -1;
		}
		INIT_LIST_HEAD(&umRxIns->umRecvBuffer.list);
		//if(umRxIns->snFiledLength == 5)
		//{
		for(i = 0; i < 32; ++i)
		{
			umRxIns->umRecvWindow5[i] = NULL;
		}
		//}
		//else if(umRxIns->snFiledLength == 10)
		//{
		for(i = 0; i < 1024; ++i)
		{
			umRxIns->umRecvWindow10[i] = NULL;
		}
		//}
		list_add_tail(&umRxIns->umRxList, &SV(vUm_rx_ins).umRxList);
		fsm_printf("[RLC][PCRLC_BULID_REQ] PCRLC_BULID_REQ: um recv instance has established(rbid is %d)\n",umRxIns->rbId);
		break;

	default:
		fsm_printf("[RLC][PCRLC_BULID_REQ] wrong CRLC_BULID_REQ");
		break;

	}
	
	cItrl = (CRLC_RbidBuild_IOCTRLMsg*)fsm_mem_alloc(sizeof(CRLC_RbidBuild_IOCTRLMsg));
	cItrl->V_Flag = true;
	//cItrl->rbId = cIoctrl->rbIdentity;
	//fsm_do_ioctrl(STRM_TO_IPADP, CTRL_RBID_BUILD, (void*)cItrl, sizeof(CRLC_RbidBuild_IOCTRLMsg));
	fsm_mem_free(cItrl);
	cItrl = NULL;
	fsm_data_destroy(cIoctrl);
	cIoctrl = NULL;
	FOUT;
}

/**********************************************************
**Function name: PCRLC_DEACT_REQ()
**Descriptions: Deact AM or UM instance
**Input:
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
void PCRLC_DEACT_REQ(void)
{
	CRLC_DeactReq_IoctrlMsg *cIoctrl;
	struct  AM_Instance *amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	struct AmSduBuffer*amsBuffer, *tempAmsBuffer;
	struct AmBuffer *amBuffer, *tempAmBuffer;
	struct Buffer *buffer, *tempBuffer;
	struct UmBuffer *umBuffer, *tempUmBuffer;
	void *insptru, *insptrd;
	CRLC_RbidBuild_IOCTRLMsg *cItrl;
	
	SV_PTR_GET(rlc_mac_sv);
	cIoctrl = (CRLC_DeactReq_IoctrlMsg*)fsm_data_get();
	findInsByRbid(cIoctrl->rbIdentity,&insptru, &insptrd , &SV(ins_mode));
	fsm_printf("[RLC][PCRLC_DEACT_REQ] entering PCRLC_DEACT_REQ\n");
	switch(SV(ins_mode))
	{
	case UM_MODE:
		if(insptrd != NULL)
		{
			umRxIns = (UM_RX_Instance *)insptrd;
			//将UM接收侧实体从链表中删除
			list_del(&umRxIns->umRxList);
			fsm_printf("[RLC][PCRLC_DEACT_REQ] PCRLC_DEACT_REQ:um recv instance has been deacted(rbid is %d)\n", cIoctrl->rbIdentity);
			//释放接收队列
			if(!list_empty(&umRxIns->umRecvBuffer.list))
			{
				list_for_each_entry_safe(umBuffer,tempUmBuffer, &umRxIns->umRecvBuffer.list,list)
				{
					list_del(&umBuffer->list);
					//回收定时器code值
					SV(allocCode[umRxIns->reorderTimerCode]) = umRxIns->reorderTimerCode;
					fsm_pkt_destroy(umBuffer->pkt);
					umBuffer->pkt = NULL;
					fsm_mem_free(umBuffer);
					umBuffer = NULL;
					if(list_empty(&umRxIns->umRecvBuffer.list))
					{
						break;
					}
				}
			}
			//释放UM接收侧实体
			fsm_mem_free(umRxIns);
			umRxIns = NULL;
		}
		if(insptru == NULL)
		{
			break;
		}
		umTxIns = (UM_TX_Instance *)insptru;

		//将UM发送侧实体从链表中删除
		list_del(&umTxIns->umTxList);
		fsm_printf("[RLC][PCRLC_DEACT_REQ] PCRLC_DEACT_REQ:um tran instance has been deacted(rbid is %d)\n", cIoctrl->rbIdentity);
		//释放UM发送侧的新数据队列
		if(!list_empty(&umTxIns->umSduBuffer.list))
		{
			list_for_each_entry_safe(umBuffer,tempUmBuffer, &umTxIns->umSduBuffer.list,list)
			{
				list_del(&umBuffer->list);
				fsm_pkt_destroy(umBuffer->pkt);
				umBuffer->pkt = NULL;
				fsm_mem_free(umBuffer);
				umBuffer = NULL;
				if(list_empty(&umTxIns->umSduBuffer.list))
				{
					break;
				}
			}
		}
		//释放UM发送侧实体
		fsm_mem_free(umTxIns);
		umTxIns = NULL;
		break;
	case AM_MODE:
		amIns = (AM_Instance *)insptru;
		if(amIns == NULL)
		{
			break;
		}
		//将AM实体从队列中删除
		list_del(&amIns->amList);
		fsm_printf("[RLC][PCRLC_DEACT_REQ] PCRLC_DEACT_REQ:am instance has been deacted(rbid is %d)\n", cIoctrl->rbIdentity);
		//回收定时器code值
		SV(allocCode[amIns->pollRetxTimerCode]) = amIns->pollRetxTimerCode;
		SV(allocCode[amIns->reorderTimerCode]) = amIns->reorderTimerCode;
		SV(allocCode[amIns->statProhTimerCode]) = amIns->statProhTimerCode;
		//释放重传队列
		if(!list_empty(&amIns->amRetxBuffer.list))
		{
			list_for_each_entry_safe(amBuffer,tempAmBuffer, &amIns->amRetxBuffer.list,list)
			{
				list_del(&amBuffer->list);
				fsm_pkt_destroy(amBuffer->pkt);
				amBuffer->pkt = NULL;
				fsm_mem_free(amBuffer);
				amBuffer = NULL;
				if(list_empty(&amIns->amRetxBuffer.list))
				{
					break;
				}
			}
		}
		//释放已发送队列
		if(!list_empty(&amIns->amTransmittedBuffer.list))
		{
			list_for_each_entry_safe(amBuffer,tempAmBuffer, &amIns->amTransmittedBuffer.list,list)
			{
				list_del(&amBuffer->list);
				fsm_pkt_destroy(amBuffer->pkt);
				amBuffer->pkt = NULL;
				fsm_mem_free(amBuffer);
				amBuffer = NULL;
				if(list_empty(&amIns->amTransmittedBuffer.list))
				{
					break;
				}
			}
		}
		//释放新数据队列
		if(!list_empty(&amIns->amSduBuffer.list))
		{
			list_for_each_entry_safe(amsBuffer,tempAmsBuffer, &amIns->amSduBuffer.list,list)
			{
				list_del(&amsBuffer->list);
				fsm_pkt_destroy(amsBuffer->pkt);
				amsBuffer->pkt = NULL;
				fsm_mem_free(amsBuffer);
				amsBuffer = NULL;
				if(list_empty(&amIns->amSduBuffer.list))
				{
					break;
				}
			}
		}
		//释放控制数据队列
		if(!list_empty(&amIns->ctrlPduBuffer.list))
		{
			list_for_each_entry_safe(buffer,tempBuffer, &amIns->ctrlPduBuffer.list,list)
			{
				list_del(&buffer->list);
				fsm_pkt_destroy(buffer->pkt);
				buffer->pkt = NULL;
				fsm_mem_free(buffer);
				buffer = NULL;
				if(list_empty(&amIns->ctrlPduBuffer.list))
				{
					break;
				}
			}
		}
		//释放接收数据队列
		if(!list_empty(&amIns->amRecvBuffer.list))
		{
			list_for_each_entry_safe(amBuffer,tempAmBuffer, &amIns->amRecvBuffer.list,list)
			{
				list_del(&amBuffer->list);
				fsm_pkt_destroy(amBuffer->pkt);
				amBuffer->pkt = NULL;
				fsm_mem_free(amBuffer);
				amBuffer = NULL;
				if(list_empty(&amIns->amRecvBuffer.list))
				{
					break;
				}
			}
		}
		//释放AM实体
		fsm_mem_free(amIns);
		amIns = NULL;
		break;
	default:
		fsm_printf("[RLC][PCRLC_DEACT_REQ] PCRLC_DEACT_REQ:cannot distinguish the instance to distinguish.\n");
		break;
	}
	cItrl = (CRLC_RbidBuild_IOCTRLMsg*)fsm_mem_alloc(sizeof(CRLC_RbidBuild_IOCTRLMsg));
	cItrl->V_Flag = false;
	//cItrl->rbId = cIoctrl->rbIdentity;
	//fsm_do_ioctrl(STRM_TO_IPADP, CTRL_RBID_BUILD, (void*)cItrl, sizeof(CRLC_RbidBuild_IOCTRLMsg));
	fsm_mem_free(cItrl);
	cItrl = NULL;
	//fsm_mem_free(cIoctrl);
	fsm_data_destroy(cIoctrl);
	cIoctrl = NULL;

	FOUT;
}

/**********************************************************
**Function name: PCRLC_SUSPEND_REQ()
**Descriptions: Suspend AM or UM instance
**Input:
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
void PCRLC_SUSPEND_REQ(void)
{
	CRLC_SuspendReq_IoctrlMsg *cIoctrl = (CRLC_SuspendReq_IoctrlMsg*) fsm_data_get();
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	void *insptru, *insptrd;
	
	SV_PTR_GET(rlc_mac_sv);
	findInsByRbid(cIoctrl->rbIdentity, &insptru, &insptrd, &SV(ins_mode));
	fsm_printf("[RLC][PCRLC_SUSPEND_REQ] entering PCRLC_SUSPEND_REQ(SV(ins_mode) is %d,cIoctrl->rbIdentity is %d)\n",SV(ins_mode),cIoctrl->rbIdentity);
	switch(SV(ins_mode))
	{
	case UM_MODE:
		if(insptrd != NULL)
		{
			umRxIns = (UM_RX_Instance *)insptrd;

			umRxIns->currentStatVar = ST_SUSPEND;
			fsm_printf("[RLC][PCRLC_SUSPEND_REQ] PCRLC_SUSPEND_REQ:um recv instance has suspended (rbid is %d)\n", umRxIns->rbId);
			if(umRxIns->reorderTimer)
			{
				fsm_schedule_cancel(umRxIns->reorderTimer);
				umRxIns->reorderTimer = 0;
			}
		}
		if(insptru == NULL)
		{
			break;
		}
		umTxIns = (UM_TX_Instance *)insptru;
		umTxIns->CurrentStatVar = ST_SUSPEND;
		fsm_printf("[RLC][PCRLC_SUSPEND_REQ] PCRLC_SUSPEND_REQ:um tran instance has suspended (rbid is %d)\n", umTxIns->rbid);
		break;

	case AM_MODE:
		if(insptru == NULL)
		{
			break;
		}
		amIns =  (AM_Instance * )insptru;
		amIns->currentStatVar = ST_SUSPEND;
		fsm_printf("[RLC][PCRLC_SUSPEND_REQ] PCRLC_SUSPEND_REQ:am instance has suspended (rbid is %d)\n", amIns->rbId);
		if(amIns->pollRetxTimer)
		{
			fsm_schedule_cancel(amIns->pollRetxTimer);
			amIns->pollRetxTimer = 0;
		}
		if(amIns->reorderTimer)
		{
			fsm_schedule_cancel(amIns->reorderTimer);
			amIns->reorderTimer = 0;
		}
		if(amIns->statProhTimer)
		{
			fsm_schedule_cancel(amIns->statProhTimer);
			amIns->statProhTimer = 0;
		}
		break;
	default:
		fsm_printf("[RLC][PCRLC_SUSPEND_REQ] PCRLC_SUSPEND_REQ:cannot distinguish the instance \n");
		break;
	}

	//  fsm_schedule_self(0, CRLC_SUSPEND_REQ);
	//fsm_mem_free(cIoctrl);
	fsm_data_destroy(cIoctrl);
	cIoctrl = NULL;
	FOUT;
}

/**********************************************************
**Function name: PCRLC_RESUME_REQ()
**Descriptions: Resume AM or UM instance
**Input:
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
void PCRLC_RESUME_REQ(void)
{
	CRLC_ResumeReq_IoctrlMsg *cIoctrl = (CRLC_ResumeReq_IoctrlMsg*) fsm_data_get();
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	void * insptru,*insptrd;
	
	SV_PTR_GET(rlc_mac_sv);
	findInsByRbid(cIoctrl->rbIdentity, &insptru, &insptrd, &SV(ins_mode));
	fsm_printf("[RLC][PCRLC_RESUME_REQ] entering PCRLC_RESUME_REQ\n");
	switch(SV(ins_mode))
	{
	case UM_MODE:
		if(insptrd == NULL)
		{
			break;
		}
		umRxIns = ( UM_RX_Instance *)insptrd;
		umRxIns->currentStatVar = ST_TRAN;
		fsm_printf("[RLC][PCRLC_RESUME_REQ] PCRLC_RESUME_REQ:um recv instance has resumed(rbid is %d)\n",cIoctrl->rbIdentity);

		if(insptru == NULL)
		{
			break;
		}
		umTxIns = ( UM_TX_Instance *)insptru;
		umTxIns->CurrentStatVar = ST_TRAN;
		fsm_printf("[RLC][PCRLC_RESUME_REQ] PCRLC_RESUME_REQ:um tran instance has resumed(rbid is %d)\n",cIoctrl->rbIdentity);
		break;

	case AM_MODE:
		if(insptru == NULL)
		{
			break;
		}
		amIns = (AM_Instance *)insptru;
		amIns->currentStatVar = ST_TRAN;
		fsm_printf("[RLC][PCRLC_RESUME_REQ] PCRLC_RESUME_REQ:am instance has resumed(rbid is %d)\n",cIoctrl->rbIdentity);
		break;
	default:
		fsm_printf("[RLC][PCRLC_RESUME_REQ]  \n");
		break;
	}
	//   fsm_mem_free(cIoctrl);
	fsm_data_destroy(cIoctrl);
	FOUT;
}

/**********************************************************
**Function name: PCRLC_STATUS_IND()
**Descriptions: Report status to RRC
**Input: u8 rbid, u16 evCode
**Output: Send ioctrl to RRC
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
void PCRLC_STATUS_IND(u8 rbid, u16 evCode)
{
	CRLC_StatusInd_IoctrlMsg *cioctrl = (CRLC_StatusInd_IoctrlMsg*)fsm_mem_alloc(sizeof(CRLC_StatusInd_IoctrlMsg));
	cioctrl->eventCode = evCode;
	cioctrl->rbIdentity = rbid;
	fsm_do_ioctrl(STRM_TO_RRC, CRLC_STATUS_IND,( void * )cioctrl, sizeof(CRLC_StatusInd_IoctrlMsg));
	fsm_mem_free(cioctrl);
	//fsm_data_destroy(cioctrl);
	cioctrl = NULL;
	FOUT;
}

//未实现
inline void PPRLC_DISC_REQ(void)  {}


/*********************************************************************
**Function name: PMRLC_TRANSOP_IND()
**Descriptions: Receive TRANSOP and send the data indicated in the TRANSOP to MAC
**Input: 
**Output: Send data to MAC
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
*********************************************************************/
void PMRLC_TRANSOP_IND (void *hptr)
{
	MRLC_TransOp_IoctrlMsg *mIoctrl;
	u32 subhead, controlelm, data, sevNum,  fifNum;
	struct  AM_Instance * amIns;
	struct  UM_TX_Instance *umTxIns;
	struct  TM_TX_Instance *tmTxIns;
	void *insptru, *insptrd, *head, *ptr, *preptr;
	FSM_PKT* pkptr, *pkp;
	struct AmBuffer *amBuffer;
	struct Buffer *buffer, *tempb;
	struct	MRLC_subHead_fix_IciMsg *fix;
	struct MRLC_toMac_IciMsg *icimsg;
	u32 *nump,num;
	u32 subHeadLen;
	u32 size = 0;     //the total size in a buffer to transimit	
	u16 rnti;
	//int i=0;  //FOR TEST
	
	SV_PTR_GET(rlc_mac_sv);
	SV(isBufferReq) = 0;
	//hptr = ptr = fsm_data_get();

	ptr = hptr; //add in 20150831
	if(hptr == NULL)
	{
		FOUT;
	}	
	nump = (u32*)hptr;
	num = *nump;
	//fsm_printf("[RLC][PMRLC_TRANSOP_IND] entering PMRLC_TRANSOP_IND(number of instances is %d)\n", num);
	if(num == 0)
	{
		fsm_data_destroy(hptr);
		hptr = NULL;
		FOUT;
	}

	//fsm_printf("After bufferReq the bufferReqTimer restart\n");
	/* 根据传输机会中提供的数据大小，按最大需求计数数据包子头的大小*/
	
	mIoctrl = (MRLC_TransOp_IoctrlMsg *)(nump + 1);
	subhead = data = sevNum = fifNum = 0;
	//controlelm = 14 + 10;
	while(num > 0)
	{
		findInsBylcid(mIoctrl->lcid,  &insptru, &insptrd, &SV(ins_mode));
		switch(SV(ins_mode))
		{
		case TM_MODE:
			if(insptru == NULL)
			{
				break;
			}
			tmTxIns = (TM_TX_Instance *)insptru;
			data += mIoctrl->txQueueHeader + mIoctrl->txQueueSize;
			sevNum = fifNum = 0;
			check_num(tmTxIns, mIoctrl->txQueueSize,  &sevNum,  &fifNum);
			subhead += 2*sevNum + 3*fifNum;
			break;

		case UM_MODE:
			if(insptru == NULL)
			{
				break;
			}
			umTxIns = (UM_TX_Instance *)insptru;
			data += mIoctrl->txQueueHeader + mIoctrl->txQueueSize;
			if((mIoctrl->txQueueSize + mIoctrl->txQueueHeader) < 128)
			{
				subhead += 2;
			}
			else
			{
				subhead += 3;
			}
			//fsm_printf("[RLC][PMRLC_TRANSOP_IND] um instance(lcid is %d)'s buffer's subhead is %d \n",umTxIns->lcid,subhead);
			break;

		case AM_MODE:
			if(insptru == NULL)
			{
				break;
			}
			amIns  = (AM_Instance * )insptru;
			data += mIoctrl->txQueueHeader + mIoctrl->txQueueSize + mIoctrl->retxQueueHeader + \
			    mIoctrl->retxQueueSize + mIoctrl->statusPduHeader + mIoctrl->statusPduSize;

			sevNum = fifNum = 0;	
			check_retx_num(amIns,mIoctrl->retxQueueSize,  &sevNum,  &fifNum);
			subhead += 2*sevNum + 3*fifNum;
			//fsm_printf("am instance(lcid is %d)'s retx buffer has %d 7bits sunhead,and %d 15bits\n",amIns->lcId,sevNum, fifNum);
			sevNum = fifNum = 0;
			check_stat_num(amIns,mIoctrl->statusPduSize, &sevNum, &fifNum);
			subhead += 2*sevNum + 3*fifNum;
			//fsm_printf("am instance(lcid is %d)'s status buffer has %d 7bits sunhead,and %d 15bits\n",amIns->lcId,sevNum, fifNum);

			if((mIoctrl->txQueueSize + mIoctrl->txQueueHeader) < 128)
			{				
				subhead += 2;
			}
			else
			{
				subhead += 3;
			}

			break;

		default:
			fsm_printf("[RLC][PMRLC_TRANSOP_IND] cannot find the instance\n");
			break;
		}	
		num--;
		if(num>0)
		{
			mIoctrl++;
		}
	}
	
	/* 根据计算出的大小建立SKB，并申请一块内存，保存ICI信息*/
	pkptr = fsm_pkt_create( data + sizeof(MRLC_toMac_IciMsg));
	head = fsm_mem_alloc(subhead + sizeof(MRLC_toMac_IciMsg) );  //暂时保存ici信息的内存区域,preptr用于在最后一个子头修改E域
	fsm_skb_reserve(pkptr, subhead + sizeof(MRLC_toMac_IciMsg));

	icimsg = (MRLC_toMac_IciMsg*)head;
	ptr = icimsg + 1;   //填充子头的指针
	data = 0;     //计数实际封装包的总共的子头的大小
	/* 将PDU数据装入申请的SKB 中*/
	mIoctrl = (MRLC_TransOp_IoctrlMsg *)(nump + 1);
	num = *nump;
	while(num > 0)
	{
		//fsm_printf("[RLC][PMRLC_TRANSOP_IND] number %d:\n", num);
		findInsBylcid(mIoctrl->lcid,  &insptru, &insptrd, &SV(ins_mode));
		//fsm_printf("[RLC][PMRLC_TRANSOP_IND] lcid is %d, mode is %d\n",mIoctrl->lcid,SV(ins_mode));
		switch(SV(ins_mode))
		{
		case TM_MODE:
			if(insptru == NULL)
			{
				break;
			}
			tmTxIns = (TM_TX_Instance *)insptru;
			if(!list_empty(&tmTxIns->tmSduBuffer.list))
			{
				list_for_each_entry_safe(buffer, tempb, &tmTxIns->tmSduBuffer.list,list)
				{
					if(buffer->pkt->len < 128)
					{
						subHeadLen = 2;
					}
					else
					{
						subHeadLen = 3;
					}
					if(buffer->pkt->len + subHeadLen >  mIoctrl->txQueueSize)
					{
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND] break:tm instance mIoctrl->txQueueSize is %d, data is %d\n", mIoctrl->txQueueSize, buffer->pkt->len + subHeadLen);
						break;
					}
					else
					{
						list_del(&buffer->list);
						tmTxIns->tmSduBufferSize -= buffer->pkt->len;
						tmTxIns->tmSduBufferNum--;
						fsm_mem_cpy(pkptr->tail,  buffer->pkt->data, buffer->pkt->len);
						fsm_skb_put(pkptr, buffer->pkt->len);

						creat_subhead(buffer->pkt,  &ptr, &preptr, tmTxIns->lcId, &data);
						mIoctrl->txQueueSize -= subHeadLen + buffer->pkt->len;
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND] tm instance mIoctrl->txQueueSize is %d, data is %d\n", mIoctrl->txQueueSize, buffer->pkt->len + subHeadLen);
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND][TM] the data is \n");	//FOR TEST
						//fsm_octets_print(buffer->pkt->data+2,18);
											
						fsm_pkt_destroy(buffer->pkt);  //20140922改
						buffer->pkt = NULL;
						fsm_mem_free(buffer);  //20140922改
						buffer = NULL;
						if(list_empty(&tmTxIns->tmSduBuffer.list))
						{
							break;
						}
					}
				}
			}
			break;

		case UM_MODE:
			if(insptru == NULL)
			{
				break;
			}
			umTxIns = (UM_TX_Instance *)insptru;
			if( umTxIns->umSduBufferSize < 128 )
			{
				subHeadLen = 2;
			}
			else
			{
				subHeadLen = 3;
			}
			if( mIoctrl->txQueueSize + mIoctrl->txQueueHeader > subHeadLen )  //添加mIoctrl->txQueueHeader,  /20140726
			{
				//pkp = rlc_UM_segment(umTxIns , mIoctrl->txQueueSize + mIoctrl->txQueueHeader - subHeadLen);//添加mIoctrl->txQueueHeader,  /20140726
				if( mIoctrl->txQueueSize + mIoctrl->txQueueHeader - subHeadLen > MAX_PDU_SIZE )
				{
					pkp = rlc_UM_segment(umTxIns,MAX_PDU_SIZE);
				}
				else
				{
					pkp = rlc_UM_segment(umTxIns,mIoctrl->txQueueSize + mIoctrl->txQueueHeader - subHeadLen);						
				}
				
				if(pkp == NULL)
				{
					printk("[RLC][PMRLC_TRANSOP_IND][UM] pkp == NULL\n");
					break;
				}
				//printk("[RLC][PMRLC_TRANSOP_IND][UM] SN = %d\n",umTxIns->SN);
				fsm_mem_cpy( pkptr->tail, pkp->data, pkp->len);
				fsm_skb_put( pkptr,pkp->len);
				creat_subhead(pkp, &ptr,  &preptr, umTxIns->lcid, &data);

				fsm_pkt_destroy(pkp);  //20140922改
				pkp = NULL;
				break;
			}
			else
			{
				fsm_printf("[RLC][PMRLC_TRANSOP_IND][UM] break:um instance mIoctrl->txQueueSize is %d, data is %d\n", mIoctrl->txQueueSize, buffer->pkt->len + subHeadLen);
				break;
			}

		case AM_MODE:
			if(insptru == NULL)
			{
				break;
			}			
			amIns  = (AM_Instance * )insptru;
			/*if( amIns == NULL )
			{
				fsm_printf("[RLC][PMRLC_TRANSOP_IND] amins is NULL\n");
				//FOUT;
				break;
			}*/

			
			//传输状态PDU
			if(!list_empty(&amIns->ctrlPduBuffer.list))
			{
				list_for_each_entry_safe(buffer,tempb,&amIns->ctrlPduBuffer.list,list )
				{
					if( buffer->pkt->len < 128 )
					{
						subHeadLen = 2;
					}
					else
					{
						subHeadLen = 3;
					}
					if(buffer->pkt->len + subHeadLen > mIoctrl->statusPduSize)
					{
						fsm_printf("[RLC][PMRLC_TRANSOP_IND] break:am instance mIoctrl->statusPduSize is %d, data is %d\n", \
							mIoctrl->statusPduSize, buffer->pkt->len + subHeadLen);
						break;
					}
					else
					{
						list_del(&buffer->list);
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND] ctrlPdu has been remove from list\n");
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND] buffer->pkt->len=%d\n",buffer->pkt->len);
						SV(countSendCtrlPdu)++;
						amIns->ctrlPduBufferSize -= buffer->pkt->len;
						amIns->ctrlPduBufferNum--;
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND] pkptr->len=%d\n",pkptr->len);
						fsm_mem_cpy(pkptr->tail, buffer->pkt->data, buffer->pkt->len);
						fsm_skb_put(pkptr, buffer->pkt->len);
						mIoctrl->statusPduSize -= buffer->pkt->len + subHeadLen;
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND] ctrlPdu transport\n");
						//break;
						creat_subhead(buffer->pkt, &ptr, &preptr, amIns->lcId , &data);
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] am instance mIoctrl->statusPduSize is %d, buffer->pkt->len is %d\n", \
						//	mIoctrl->statusPduSize, buffer->pkt->len);
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] ctrl data transport\n");
						//fsm_octets_print(buffer->pkt->data,10);
						
						if(buffer->pkt != NULL)
						{
							fsm_pkt_destroy(buffer->pkt);  //20140922改
							buffer->pkt = NULL;
						}
						if(buffer != NULL)
						{
							fsm_mem_free(buffer);  //20140922改
							buffer = NULL;
						}						
						/*if(list_empty(&amIns->ctrlPduBuffer.list))
						{
							fsm_printf("[RLC][PMRLC_TRANSOP_IND] amIns->ctrlPduBuffer is empty\n");
							break;
						}*/
					}
				}
				//启动状态禁止定时器
				if( !amIns->statProhTimer )
				{
					//fsm_printf("[RLC][PMRLC_TRANSOP_IND] the statProhTimer start\n");
					amIns->statProhTimer = fsm_schedule_self(amIns->statProhTimerValue,amIns->statProhTimerCode);
				}
			}
			
			//传输重传PDU
			size = mIoctrl->retxQueueSize + mIoctrl->retxQueueHeader;
			
			if( !list_empty(&amIns->amRetxBuffer.list) && size > 0 )
			{
				list_for_each_entry(amBuffer,&amIns->amRetxBuffer.list,list)
				{
					if(isInsideTranWin(amIns,amBuffer->SN))
					{
						if( amBuffer->pkt->len < 128 )
						{
							subHeadLen = 2;
						}
						else
						{
							subHeadLen = 3;
						}
						if( subHeadLen < size )
						{
							if( size - subHeadLen > MAX_PDU_SIZE)
							{
								pkp = rlc_resegment( amBuffer, MAX_PDU_SIZE);
							}
							else
							{
								pkp = rlc_resegment( amBuffer, size - subHeadLen );
							}

							if( amIns->pduWithoutPoll >= amIns->pollPDU || amIns->byteWithoutPoll >= amIns->pollBYTE )
							{
								rlc_poll(amIns, pkp);
							}

							fsm_mem_cpy( pkptr->tail,  pkp->data, pkp->len);
							fsm_skb_put( pkptr,pkp->len);
							creat_subhead(pkp, &ptr, &preptr, amIns->lcId,&data);
							size -= pkp->len + subHeadLen;
							//fsm_printf("[RLC][PMRLC_TRANSOP_IND] am instance mIoctrl->retxQueueSize is %d, data is %d\n", size, pkp->len);

							//更新pduWithoutPoll和byteWithoutPoll，并判断是否需要轮询????????
							amIns->pduWithoutPoll++;
							amIns->byteWithoutPoll += pkp->len;  //数据长度??
							//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] retx data transport\n");
							//fsm_octets_print(pkp->data,10);
							fsm_pkt_destroy(pkp);  //20140922改
							pkp = NULL;
						}
						else
						{
							fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] break:am instance mIoctrl->retxQueueSize is %d, data is %d\n", size, amBuffer->pkt->len+subHeadLen);
							break;
						}
					}
					/*else
					{
					continue;
					}*/
				}
			}
		
			//传输新数据
			size = mIoctrl->txQueueSize + mIoctrl->txQueueHeader;
			if(isInsideTranWin(amIns,amIns->SN) &&(size > 0)&&(!list_empty(& amIns->amSduBuffer.list)))
			//if(((mIoctrl->txQueueSize + mIoctrl->txQueueHeader) > 0)&&(!list_empty(& amIns->amSduBuffer.list)))
			{
				//fsm_printf("[RLC][PMRLC_TRANSOP_IND] run to am sdu transport\n");
				if( amIns->amSduBufferSize < 128 )
				{
					subHeadLen = 2;
				}
				else
				{
					subHeadLen = 3;
				}
				if( size > subHeadLen )
				{
					if( size - subHeadLen > MAX_PDU_SIZE )
					{
						pkp = rlc_AM_segment(amIns,MAX_PDU_SIZE);
					}
					else
					{
						pkp = rlc_AM_segment(amIns,size - subHeadLen);						
					}
					if(pkp == NULL)
					{
						printk("[RLC][PMRLC_TRANSOP_IND][AM] pkp == NULL\n");
						break;
					}
					/*if( amIns->pduWithoutPoll >= amIns->pollPDU || amIns->byteWithoutPoll >= amIns->pollBYTE )
					{
						//fsm_printf("[RLC] pduWithoutPoll is %d\n",amIns->pduWithoutPoll);
						//fsm_printf("[RLC] byteWithoutPoll is %d\n",amIns->byteWithoutPoll);
						rlc_poll(amIns,pkp);
					}*/
					//printk("[RLC][PMRLC_TRANSOP_IND] size=%d, pkp->len=%d, subHeadLen=%d\n",size,pkp->len,subHeadLen);
					fsm_mem_cpy( pkptr->tail, pkp->data, pkp->len);
					if(pkptr->tail+pkp->len > pkptr->end)
					{
						//printk("[RLC][PMRLC_TRANSOP_IND] skb_over_panic,%p, %p\n",pkptr->tail+pkp->len,pkptr->end);
						FOUT;
					}
					fsm_skb_put( pkptr,pkp->len);
					creat_subhead(pkp, &ptr, &preptr, amIns->lcId, &data);
					amIns->vt_s = (amIns->vt_s + 1)%(AM_WINDOW_SIZE*2);
					//fsm_printf("[RLC][PMRLC_TRANSOP_IND] amIns->lcId = %d\n",amIns->lcId);
					//fsm_printf("[RLC][PMRLC_TRANSOP_IND] amIns->vt_s  is updated to %d, the len of the data is %d\n",amIns->vt_s , pkp->len);

					//更新pduWithoutPoll和byteWithoutPoll，并判断是否需要轮询
					amIns->pduWithoutPoll++;
					amIns->byteWithoutPoll += pkp->len;
					//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] new data transport\n");
					fsm_octets_print(pkp->data,10);

					fsm_pkt_destroy(pkp);  //20140922改
					pkp = NULL;
				}
				else
				{
					fsm_printf("[RLC][PMRLC_TRANSOP_IND] break:am instance mIoctrl->txQueueSize + mIoctrl->txQueueHeader is %d, data is %d\n",\
					mIoctrl->txQueueSize+mIoctrl->txQueueHeader, buffer->pkt->len + subHeadLen);
					break;
				}          
            }
			break;

		default:
			fsm_printf("[RLC][PMRLC_TRANSOP_IND] cannot find the instance\n");
			break;
		}
	
		num--;
		if(num>0)
		{
			mIoctrl++;
		}
	}
		
	if(0 == pkptr->len)
	{
		fsm_printf("[RLC][PMRLC_TRANSOP_IND] there is no data to send\n");
		SV(bufferReqTimer) = 0;
		fsm_data_destroy(hptr);
		FOUT;
	}
	fix = (MRLC_subHead_fix_IciMsg *)preptr;
	fix->E = 0;    //将最后一个子头的E域改为0
	icimsg->len = data;
	icimsg->rnti = mIoctrl->rnti;
	fsm_mem_cpy(pkptr->head, head,  data + sizeof(MRLC_toMac_IciMsg));
	fsm_mem_free(head);
	head = NULL;
	
	/*static int cnt=0;
	if(cnt > 2)
	{
		fsm_printf("[RLC][PMRLC_TRANSOP_IND] cnt=%d >2\n",cnt);
		fsm_pkt_destroy(pkptr);
		pkptr = NULL;
		FOUT;
	}*/
	//fsm_pkt_send( pkptr, STRM_TO_MAC);
	if( SV(T_C_RNTI) !=0 )	//add in 20150831
	{
		send_msg3(pkptr);
		fsm_printf("[RLC_MAC]excute RA send_msg3!\n");
		FOUT;
	}
	else{
		complexUeMacPdu(pkptr); //add in 20150804
		if(SV(Tbsize_Complex)!=0)
				SV(Tbsize_Complex)=0;
		
		fsm_printf("[RLC][PMRLC_TRANSOP_IND][<--] send to MAC\n");
		SV(countSentToLower)++;
		//fsm_printf("[RLC][PMRLC_TRANSOP_IND][<--] data len = %d\n",pkptr->len);
		//fsm_octets_print(pkptr->data,pkptr->len);
		//SV(isBufferReq) = 0; //noted in 20150831
	/*收到传输机会后重启bufferReq定时器*/
		/*if( SV(bufferReqTimer) )
		{
			fsm_schedule_cancel(SV(bufferReqTimer));
			SV(bufferReqTimer) = 0;
		}
		fsm_printf("[RLC][PMRLC_TRANSOP_IND] bufferReqTimer restart\n");
		SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));*/
		fsm_data_destroy(hptr);
		hptr = NULL;
		PMRLC_BUFFERREP_REQ();
		FOUT;
	}
}


/**********************************************************
**Function name: PMRLC_BUFFERREP_REQ()
**Descriptions: Send bufferReq to MAC
**Input: 
**Output: Send bufferReq to MAC
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
void* PMRLC_BUFFERREP_REQ(void)
{
	MRLC_ReportReq_IoctrlMsg *mioctrl;
	struct list_head* pos;
	struct AM_Instance * amIns;
	struct UM_TX_Instance *umTxIns;
	struct TM_TX_Instance *tmTxIns;
	struct AmBuffer *amBuffer;
	CtrlBuffer *ctrlBf;
	u32 num = 0,*nump;  //需要发送的结构体个数

	SV_PTR_GET(rlc_mac_sv);
	//计数需要发送的数据大小
	if (!list_empty(&SV(vAm_ins).amList))
	{
		list_for_each(pos,&SV(vAm_ins).amList)
		{
			amIns = list_entry(pos,struct AM_Instance,amList);
			if(amIns->amSduBufferSize != 0 || amIns->amRetxBufferSize != 0 || amIns->ctrlPduBufferSize != 0)
			{
				//fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] am instance (rbid is %d) has data to report\n", amIns->rbId);
				num++;
			}
		}
	}

	if (!list_empty(&SV(vUm_tx_ins).umTxList))
	{
		list_for_each(pos,&SV(vUm_tx_ins).umTxList)
		{
			umTxIns= list_entry(pos,struct UM_TX_Instance,umTxList);
			if(umTxIns->umSduBufferSize != 0)
			{
				//fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] um instance (rbid is %d) has data to report\n", umTxIns->rbid);
				num++;
			}
		}
	}
	if (!list_empty(&SV(vTm_tx_ins).tmTxList))
	{
		list_for_each(pos,&SV(vTm_tx_ins).tmTxList)
		{
			tmTxIns= list_entry(pos,struct TM_TX_Instance,tmTxList);
			if(tmTxIns->tmSduBufferSize != 0)
			{
				//fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] tm instance (rbid is %d) has data to report\n", tmTxIns->rbId);
				num++;
			}
		}
	}
	
	if(num == 0)
	{
		fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] There is no data to report\n");
		/*SV(isBufferReq) = 0;
		if( SV(bufferReqTimer) )
		{
			fsm_schedule_cancel(SV(bufferReqTimer));
			SV(bufferReqTimer) = 0;
		}*/
		return NULL; 	//modify 20150901
		//FOUT;
	}
	//分配空间大小
	nump = (u32 *) fsm_mem_alloc(sizeof(u32) + sizeof(MRLC_ReportReq_IoctrlMsg)*num);
	*nump = num;
	mioctrl = (MRLC_ReportReq_IoctrlMsg*)(nump + 1);
	//填充分配的内存空间
	if (!list_empty(&SV(vAm_ins).amList))
	{
		list_for_each(pos,&SV(vAm_ins).amList)
		{
			amIns = list_entry(pos,struct AM_Instance,amList);
			if(amIns->amSduBufferSize != 0 || amIns->amRetxBufferSize != 0 || amIns->ctrlPduBufferSize != 0)
			{
				mioctrl->rnti = 0;
				mioctrl->lcid = amIns->lcId;
				mioctrl->txQueueSize = amIns->amSduBufferSize;
				mioctrl->txQueueHeader = 2 + 2* amIns->amSduBufferNum;
				if( mioctrl->txQueueSize + mioctrl->txQueueHeader < 128 )  //将mac子头部分考虑在资源请求内, 20140726
				{
					mioctrl->txQueueSize += 2;
				}
				else
				{
					mioctrl->txQueueSize += 3;
				}
                
				mioctrl->retxQueueSize = amIns->amRetxBufferSize;
				mioctrl->retxQueueHeader = 2;
				if( !list_empty(&amIns->amRetxBuffer.list) )  //将mac子头部分考虑在资源请求内, 20140726
				{
					list_for_each_entry(amBuffer,&amIns->amRetxBuffer.list,list)
					{
						if(isInsideTranWin(amIns,amBuffer->SN))
						{
							if( amBuffer->pkt->len < 128 )
							{
								mioctrl->retxQueueSize += 2;
							}
							else
							{
								mioctrl->retxQueueSize += 3;
							}
						}
					}
				}
		
				mioctrl->statusPduSize = amIns->ctrlPduBufferSize;
				mioctrl->statusPduHeader =  0;
				if( !list_empty(&amIns->ctrlPduBuffer.list) )  //将mac子头部分考虑在资源请求内, 20140726
				{
					list_for_each_entry(ctrlBf,&amIns->ctrlPduBuffer.list,list)
					{
						if( ctrlBf->pkt->len < 128 )
						{
							mioctrl->statusPduSize += 2;
						}
						else
						{
							mioctrl->statusPduSize += 3;
						}
					}
				}
				/*if( mioctrl->statusPduSize + mioctrl->statusPduHeader < 128 )  //将mac子头部分考虑在资源请求内, 20140726
				{
					mioctrl->statusPduSize += 2;
				}
				else
				{
					mioctrl->statusPduSize += 3;
				}*/
				mioctrl++;
			}
		}
	}

	if (!list_empty(&SV(vUm_tx_ins).umTxList))
	{
		list_for_each(pos,&SV(vUm_tx_ins).umTxList)
		{
			umTxIns= list_entry(pos,struct UM_TX_Instance,umTxList);
			if(umTxIns->umSduBufferSize != 0)
			{
				mioctrl->rnti = 0;
				mioctrl->lcid = umTxIns->lcid;
				mioctrl->txQueueSize = umTxIns->umSduBufferSize;
				mioctrl->retxQueueSize = 0;
				mioctrl->retxQueueHeader = 0;
				mioctrl->statusPduSize = 0;
				mioctrl->statusPduHeader = 0;

				if(umTxIns->snFiledLength == 5)
				{
					mioctrl->txQueueHeader = 1 + 2*umTxIns->umSduBufferNum;
				}
				if(umTxIns->snFiledLength == 10)
				{
					mioctrl->txQueueHeader =2 + 2*umTxIns->umSduBufferNum;
				}

				if( mioctrl->txQueueSize + mioctrl->txQueueHeader < 128 )  //将mac子头部分考虑在资源请求内, 20140726
				{
					mioctrl->txQueueSize += 2;
				}
				else
				{
					mioctrl->txQueueSize += 3;
				}
				//fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] the total size is %d(lcid is %d)\n", mioctrl->txQueueSize  + mioctrl->txQueueHeader, mioctrl->lcid);
				mioctrl++;
			}
		}
	}
	if (!list_empty(&SV(vTm_tx_ins).tmTxList))
	{
		list_for_each(pos,&SV(vTm_tx_ins).tmTxList)
		{
			tmTxIns= list_entry(pos,struct TM_TX_Instance,tmTxList);
			if(tmTxIns->tmSduBufferSize != 0)
			{
				mioctrl->rnti = 0;
				mioctrl->lcid = tmTxIns->lcId;
				mioctrl->txQueueSize = tmTxIns->tmSduBufferSize;
				mioctrl->txQueueHeader = 0;
				if( mioctrl->txQueueSize + mioctrl->txQueueHeader < 128 )  //将mac子头部分考虑在资源请求内, 20140726
				{
					mioctrl->txQueueSize += 2;
				}
				else
				{
					mioctrl->txQueueSize += 3;
				}
				mioctrl->retxQueueSize = 0;
				mioctrl->statusPduSize = 0;
				mioctrl->statusPduHeader = 0;
				mioctrl->retxQueueHeader = 0;
				//fsm_printf("the total size is %d(lcid is %d)\n", mioctrl->txQueueSize, mioctrl->lcid);
				mioctrl++;
			}
		}
	}
	/*if( num > 0 )
	{	
		//fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] the num of instances to report is %d, and the space is %d\n", num, sizeof(u32) + sizeof(MRLC_ReportReq_IoctrlMsg)*num );
		fsm_do_ioctrl(STRM_TO_MAC, MRLC_BUFREPORT_REQ, (void*)nump, sizeof(u32) + sizeof(MRLC_ReportReq_IoctrlMsg)*num);
		fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] has sent BUFFERREPORT to MAC\n");
		fsm_mem_free(nump);
		nump = NULL;
		SV(isBufferReq) = 1;
		if( !SV(bufferReqTimer) )
		{
			//fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] After bufferReq the bufferReqTimer start\n");
			SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
		}
	}
	else
	{
		fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] There is no data to report\n");
		SV(isBufferReq) = 0;
		fsm_mem_free(nump);
		nump = NULL;
		if( SV(bufferReqTimer) )
		{
			fsm_schedule_cancel(SV(bufferReqTimer));
			SV(bufferReqTimer) = 0;
		}
	}
	if( !SV(bufferReqTimer) )
	{
		fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] After bufferReq the bufferReqTimer start\n");
		SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
	}*/
	return (void*)nump;	//add in 20150804
	//FOUT;
}
