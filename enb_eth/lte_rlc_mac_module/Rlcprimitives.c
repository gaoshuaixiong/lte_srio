
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
**Input: FSM_PKT * skb
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
	//fsm_printf("[RLC][creat_subhead] entering creat_subhead\n");
	if(skb->len < 128)
	{		
		subsev = (MRLC_subHead_sev_IciMsg *)*ptr;
		subsev->fix.R1 = subsev->fix.R2 = 0;
		subsev->fix.E = 1;
		subsev->fix.lcid = lcid;
		subsev->fandl = skb->len;
		subsev->fandl &= 0x7f;
		*preptr = subsev;
		*ptr = subsev+1;
		*data += 2;
		//fsm_printf("[RLC][creat_subhead] the data is %d\n",  *data);
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
		//       subfif->fandl = subfif->fandl;
		*preptr = subfif;
		*ptr = subfif + 1;
		*data += 3;
		//fsm_printf("[RLC][creat_subhead] the data is %d\n",  *data);
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

	findInsByRbid(cIoctrl->rnti,cIoctrl->rbIdentity ,&insptru, &insptrd, &SV(ins_mode));
	fsm_printf("[RLC][PCRLC_CONFIG_REQ] entering PCRLC_CONFIG_REQ\n ");
	switch(cIoctrl->mode)
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
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] case1:there is no AM instance to be recinfiged!");
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
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] case2:there is no UM  instance to be recinfiged!");
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
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] case3:there is no UM TX instance to be recinfiged!");
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
			fsm_printf("[RLC][PCRLC_CONFIG_REQ] case 4:there is no UM RX instance to be recinfiged!");
		}

		break;

	default:
		fsm_printf("[RLC][PCRLC_CONFIG_REQ] wrong CRLC_CONFIG_REQ");
		break;

	}
	fsm_data_destroy(cIoctrl);
	cIoctrl = NULL;
	FOUT;
}

/**********************************************************
**Function name: pktbuf_handler()
**Descriptions: Handle the packets which has no coresponding instance
**Input:
**Output:
**Returns:
**Created by: ZhangYi
**Created Date: 2014/11/25
**-----------------------------------------------------
**Modified by:
**Modified Date:
**********************************************************/
void pktbuf_handler()
{
	FSM_PKT *skb;
	packetBuf *pktbuf, *tempPktBuf;
	MRLC_fromMac_IciMsg* micimsg;
	URLC_IciMsg icimsg;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	//    struct  TM_RX_Instance *tmRxIns;
	void *insptru, *insptrd;
	
	//FIN(rlc_pkt_receive_from_mac());
	FIN(pktbuf_handler()); 	//modify in 20150813
	SV_PTR_GET(rlc_mac_sv);
	if(!list_empty(&SV(pktBuf).list))
	{
		list_for_each_entry_safe(pktbuf, tempPktBuf, &SV(pktBuf).list, list)
		{
			skb = pktbuf->pkt;
			micimsg = (MRLC_fromMac_IciMsg*)skb->head;
			if(micimsg->lcid != 0 )
			{
				findInsBylcid(micimsg->rnti,micimsg->lcid, &insptru, &insptrd,&SV(ins_mode));
				switch(SV(ins_mode))
				{
				case AM_MODE:
					if(insptru == NULL)
					{
						break;
					}
					amIns = ( AM_Instance *)insptru;
					if(amIns->currentStatVar == ST_SUSPEND)
					{
						break;
					}
					SV(pktBufNum)--;
					SV(pktBufSize) -= skb->len;
					list_del(&pktbuf->list);
					if(pktbuf != NULL)
					{
						fsm_mem_free(pktbuf);
						pktbuf = NULL;
					}
					rlc_AM_pkt_receive_from_mac(amIns,skb);
					//    fsm_printf("time for rlc_AM_pkt_receive_from_mac(amIns,skb)\n");
					FOUT;
					break;

				case UM_MODE:
					if(insptrd == NULL)
					{
						break;
					}
					umRxIns = (UM_RX_Instance *)insptrd;
					if(umRxIns->currentStatVar == ST_SUSPEND)
					{
						break;
					}
					SV(pktBufNum)--;
					SV(pktBufSize) -= skb->len;
					list_del(&pktbuf->list);
					if(pktbuf != NULL)
					{
						fsm_mem_free(pktbuf);
						pktbuf = NULL;
					}
					rlc_UM_pkt_receive_from_mac(umRxIns ,skb);
					FOUT;
					break;

				default:
					fsm_printf("[RLC][pktbuf_handler] cannot distinfuish this  %d\n", SV(ins_mode));
					break;
				}
				fsm_printf("[RLC][pktbuf_handler] cannot find the instance or the instance is suspended(rbid is %d)\n", micimsg->lcid);
				SV(pktBufNum)--;
				SV(pktBufSize) -= skb->len;
				list_del(&pktbuf->list);
				if(pktbuf != NULL)
				{
					fsm_mem_free(pktbuf);
					pktbuf = NULL;
				}
				if(skb != NULL)
				{
					fsm_pkt_destroy(skb);
					skb = NULL;
				}
			}
			else
			{
				if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
				{
					skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
				}
				icimsg.rbId = micimsg->lcid;
				icimsg.rnti = micimsg->rnti;

				fsm_mem_cpy(skb->head, &icimsg , sizeof(URLC_IciMsg));
				SV(countSendToUpper)++;
				fsm_printf("[RLC][pktbuf_handler] <--send to RRC\n");
				//fsm_octets_print(skb->data, 100);
				fsm_pkt_send(skb, STRM_TO_RRC);
				SV(pktBufNum)--;
				SV(pktBufSize) -= skb->len;
				list_del(&pktbuf->list);
				if(pktbuf != NULL)
				{
					fsm_mem_free(pktbuf);
					pktbuf = NULL;
				}
			}
		}
	}
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
	CRLC_BuildReq_IoctrlMsg *cIoctrl;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	struct RntiManag * rntiM;
	CRLC_RbidBuild_IOCTRLMsg *cItrl;
	int i = 0;
	
	SV_PTR_GET(rlc_mac_sv);
	cIoctrl = (CRLC_BuildReq_IoctrlMsg*)fsm_data_get();
	fsm_printf("[RLC][PCRLC_BULID_REQ] entering PCRLC_BULID_REQ\n ");
	//fsm_printf("[RLC] rnti=%d\n",cIoctrl->rnti);
	//如果之前还未建立过相同用户的实体，则建立该用户的CCCH
	if(findUeByRnti(cIoctrl->rnti) == NULL)
	{
		rntiM = (struct RntiManag *)fsm_mem_alloc(sizeof(struct RntiManag ));
		INIT_LIST_HEAD(&rntiM->amIns.amList);
		INIT_LIST_HEAD(&rntiM->umRxIns.umRxList);
		INIT_LIST_HEAD(&rntiM->umTxIns.umTxList);
		INIT_LIST_HEAD(&rntiM->ccchTran.tmSduBuffer.list);
		rntiM->rnti = rntiM->ccchRecv.rnti = rntiM->ccchTran.rnti = cIoctrl->rnti;
		rntiM->ccchRecv.lcId = rntiM->ccchRecv.rbId = rntiM->ccchRecv.pbCh = 0;
		rntiM->ccchTran.lcId = rntiM->ccchTran.rbId = rntiM->ccchTran.pbCh = rntiM->ccchTran.tmSduBufferNum = rntiM->ccchTran.tmSduBufferSize=0;
		list_add_tail(&rntiM->list, &SV(rntiManag).list);
		//fsm_printf("[RLC] build new user\n");
	}
	//如果已经建立过，则找到改用户
	else
	{
		rntiM = findUeByRnti(cIoctrl->rnti);
	}
	
	switch(cIoctrl->mode)
	{
	case AM_UPDOWN:                      //AM的上下行配置
		amIns =  instance_create(struct AM_Instance);
		amIns->rbId = cIoctrl->rbIdentity;
		//fsm_printf("the rbid is %d\n", amIns->rbId);
		amIns->lcId = cIoctrl->lcIdentity;
		amIns->reorderTimerValue= cIoctrl->amDwparas.timerReordering*100;
		amIns->statProhTimerValue = cIoctrl->amDwparas.timerStatusProhibit*100;
		amIns->pollRetxTimerValue = cIoctrl->amUpParas.timerPollRetransmit*100;
		amIns->pollPDU = cIoctrl->amUpParas.PollPDU;
		amIns->pollBYTE = cIoctrl->amUpParas.PollBYTE;
		amIns->maxRetxThreshold = cIoctrl->amUpParas.maxRetxThreshold;
		amIns->amRetxBufferSize = amIns->amSduBufferSize = amIns->ctrlPduBufferSize = 0;
		amIns->amRetxBufferNum = amIns->amSduBufferNum = amIns->ctrlPduBufferNum = 0;
		amIns->pollRetxTimer = amIns->reorderTimer = amIns->statProhTimer = 0;
		amIns->currentStatVar = ST_IDLE; //modify in 20150805
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
		INIT_LIST_HEAD(&amIns->amSduBuffer.list);
		INIT_LIST_HEAD(&amIns->amRecvBuffer.list);
		INIT_LIST_HEAD(&amIns->amRetxBuffer.list);
		INIT_LIST_HEAD(&amIns->amTransmittedBuffer.list);
		INIT_LIST_HEAD(&amIns->ctrlPduBuffer.list);
		for(i = 0;  i < AM_WINDOW_SIZE*2; i++ )
		{
			amIns->amRecvWindow[i] = NULL;
		}
		for(i = 0;  i < AM_WINDOW_SIZE*2; i++ )
		{
			amIns->amTranWindow[i]= NULL;
		}
	
		list_add_tail(&amIns->amList, &rntiM->amIns.amList);
		fsm_printf("[RLC][PCRLC_BULID_REQ] am instance has established(rbid is %d)\n",amIns->rbId);
	//fsm_printf("[RLC] build instance\n");
	//FOUT;
		break;

	case UM_UPDOWN:              	 //UM的上下行配置
		umRxIns =  instance_create(struct UM_RX_Instance);
		umRxIns->snFiledLength = cIoctrl->umDwParas.snFiledLength;
		umRxIns->reorderTimerValue = cIoctrl->umDwParas.timerReordering*100;
		umRxIns->currentStatVar = ST_IDLE; //modify in 20150805
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
		///{
			for(i = 0; i < UM_WINDOW_SIZE5*2; ++i)
			{
				umRxIns->umRecvWindow5[i] = NULL;
			}
		//}
		//else if(umRxIns->snFiledLength == 10)
		//{
			for(i = 0; i < UM_WINDOW_SIZE10*2; ++i)
			{
				umRxIns->umRecvWindow10[i] = NULL;
			}
		//}
		list_add_tail(&umRxIns->umRxList, &rntiM->umRxIns.umRxList);
		//   fsm_printf("PCRLC_BULID_REQ: um recv instance has established(rbid is %d)\n",umRxIns->rbId);

		//上行
		umTxIns =  instance_create(UM_TX_Instance);
		umTxIns->snFiledLength = cIoctrl->umUpParas.snFiledLength;
		umTxIns->lcid = cIoctrl->lcIdentity;
		umTxIns->rbid = cIoctrl->rbIdentity;
		umTxIns->CurrentStatVar = ST_IDLE; //modify in 20150805
		umTxIns->umSduBufferNum = umTxIns->umSduBufferSize = 0;
		umTxIns->SN = 0;
		INIT_LIST_HEAD(&umTxIns->umSduBuffer.list);
		list_add_tail(&umTxIns->umTxList, &rntiM->umTxIns.umTxList);
		fsm_printf("[RLC][PCRLC_BULID_REQ] um tran and recv instance has established(rbid is %d)\n",umTxIns->rbid);
		break;

	case UM_UP:			//UM的上行配置
		umTxIns =  instance_create(UM_TX_Instance);
		umTxIns->snFiledLength = cIoctrl->umUpParas.snFiledLength;
		umTxIns->lcid = cIoctrl->lcIdentity;
		umTxIns->rbid = cIoctrl->rbIdentity;
		umTxIns->CurrentStatVar = ST_IDLE; //modify in 20150805
		umTxIns->umSduBufferNum = umTxIns->umSduBufferSize = 0;
		umTxIns->SN = 0;
		INIT_LIST_HEAD(&umTxIns->umSduBuffer.list);
		list_add_tail(&umTxIns->umTxList, &rntiM->umTxIns.umTxList);
		fsm_printf("[RLC][PCRLC_BULID_REQ] um tran instance has established(rbid is %d)\n",umTxIns->rbid);
		break;

	case UM_DOWN:			//UM的下行配置
		umRxIns =  instance_create(struct UM_RX_Instance);
		umRxIns->snFiledLength = cIoctrl->umDwParas.snFiledLength;
		umRxIns->reorderTimerValue = cIoctrl->umDwParas.timerReordering*100;
		umRxIns->currentStatVar = ST_IDLE; //modify in 20150805
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
			for(i = 0; i < UM_WINDOW_SIZE5*2; ++i)
			{
				umRxIns->umRecvWindow5[i] = NULL;
			}
		//}
		//else if(umRxIns->snFiledLength == 10)
		//{
			for(i = 0; i < UM_WINDOW_SIZE10*2; ++i)
			{
				umRxIns->umRecvWindow10[i] = NULL;
			}
		//}
		list_add_tail(&umRxIns->umRxList, &rntiM->umRxIns.umRxList);
		fsm_printf("[RLC][PCRLC_BULID_REQ] um recv instance has established(rbid is %d)\n",umRxIns->rbId);
		break;

	default:
		fsm_printf("[RLC][PCRLC_BULID_REQ] wrong CRLC_BULID_REQ");

	}
	pktbuf_handler();  //处理之前没有对应实体的数据包
	cItrl = (CRLC_RbidBuild_IOCTRLMsg*)fsm_mem_alloc(sizeof(CRLC_RbidBuild_IOCTRLMsg));
	//cItrl->rbId = cIoctrl->rbIdentity;
	cItrl->rnti = cIoctrl->rnti;
	cItrl->V_Flag = true;
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
	void *insptru, *insptrd;
	CRLC_RbidBuild_IOCTRLMsg *cItrl;
	
	SV_PTR_GET(rlc_mac_sv);
	cIoctrl = (CRLC_DeactReq_IoctrlMsg*)fsm_data_get();
	findInsByRbid(cIoctrl->rnti,cIoctrl->rbIdentity,&insptru, &insptrd , &SV(ins_mode));
	fsm_printf("[RLC][PCRLC_DEACT_REQ] entering PCRLC_DEACT_REQ\n");
	switch(SV(ins_mode))
	{
	case UM_MODE:
		if(insptrd != NULL)
		{
			umRxIns = (UM_RX_Instance *)insptrd;
			//将UM接收侧实体从链表中删除
			rlc_UMrx_release(umRxIns);
		}
		if(insptru == NULL)
		{
			break;
		}
		umTxIns = (UM_TX_Instance *)insptru;

		//将UM发送侧实体从链表中删除
		rlc_UMtx_release( umTxIns);
		break;

	case AM_MODE:
		amIns = (AM_Instance *)insptru;
		if(amIns == NULL)
		{
			break;
		}
		//将AM实体从队列中删除
		rlc_AM_release(amIns);
		break;
	default:
		fsm_printf("[RLC][PCRLC_DEACT_REQ] cannot distinguish the instance to distinguish.\n");
		break;
	}
	// fsm_mem_free(cIoctrl);
	cItrl = (CRLC_RbidBuild_IOCTRLMsg*)fsm_mem_alloc(sizeof(CRLC_RbidBuild_IOCTRLMsg));
	//cItrl->rbId = cIoctrl->rbIdentity;
	cItrl->rnti = cIoctrl->rnti;
	cItrl->V_Flag = false;
	//fsm_do_ioctrl(STRM_TO_IPADP, CTRL_RBID_BUILD, (void*)cItrl, sizeof(CRLC_RbidBuild_IOCTRLMsg));
	fsm_mem_free(cItrl);
	cItrl = NULL;
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
	findInsByRbid(cIoctrl->rnti,cIoctrl->rbIdentity, &insptru, &insptrd, &SV(ins_mode));
	fsm_printf("[RLC][PCRLC_SUSPEND_REQ] entering PCRLC_SUSPEND_REQ(SV(ins_mode) is %d,cIoctrl->rbIdentity is %d)\n",SV(ins_mode),cIoctrl->rbIdentity);
	switch(SV(ins_mode))
	{
	case UM_MODE:
		if(insptrd != NULL)
		{
			umRxIns = (UM_RX_Instance *)insptrd;

			umRxIns->currentStatVar = ST_SUSPEND;
			fsm_printf("[RLC][PCRLC_SUSPEND_REQ] um recv instance has suspended (rbid is %d)\n", umRxIns->rbId);
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
		fsm_printf("[RLC][PCRLC_SUSPEND_REQ] um tran instance has suspended (rbid is %d)\n", umTxIns->rbid);
		break;

	case AM_MODE:
		if(insptru == NULL)
		{
			break;
		}
		amIns =  (AM_Instance * )insptru;
		amIns->currentStatVar = ST_SUSPEND;
		fsm_printf("[RLC][PCRLC_SUSPEND_REQ] am instance has suspended (rbid is %d)\n", amIns->rbId);
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
		fsm_printf("[RLC][PCRLC_SUSPEND_REQ] cannot distinguish the instance \n");
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
	findInsByRbid( cIoctrl->rnti,cIoctrl->rbIdentity, &insptru, &insptrd, &SV(ins_mode));
	fsm_printf("[RLC][PCRLC_RESUME_REQ] entering PCRLC_RESUME_REQ\n");
	switch(SV(ins_mode))
	{
	case UM_MODE:
		if(insptrd == NULL)
		{
			break;
		}
		umRxIns = ( UM_RX_Instance *)insptrd;
		umRxIns->currentStatVar = ST_IDLE; //modify in 20150805
		fsm_printf("[RLC][PCRLC_RESUME_REQ] um recv instance has resumed(rbid is %d)\n",cIoctrl->rbIdentity);

		if(insptru == NULL)
		{
			break;
		}
		umTxIns = ( UM_TX_Instance *)insptru;
		umTxIns->CurrentStatVar = ST_IDLE; //modify in 20150805
		fsm_printf("[RLC][PCRLC_RESUME_REQ] um tran instance has resumed(rbid is %d)\n",cIoctrl->rbIdentity);
		break;

	case AM_MODE:
		if(insptru == NULL)
		{
			break;
		}
		amIns = (AM_Instance *)insptru;
		amIns->currentStatVar = ST_IDLE; //modify in 20150805
		fsm_printf("[RLC][PCRLC_RESUME_REQ] am instance has resumed(rbid is %d)\n",cIoctrl->rbIdentity);
		break;
	default:
		fsm_printf("[RLC][PCRLC_RESUME_REQ] there no instance responding to the mode\n");
		break;
	}
	fsm_data_destroy(cIoctrl);
	cIoctrl = NULL;
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
void PCRLC_STATUS_IND(u16 rnti,u8 rbid, u16 evCode)
{
	CRLC_StatusInd_IoctrlMsg *cioctrl = (CRLC_StatusInd_IoctrlMsg*)fsm_mem_alloc(sizeof(CRLC_StatusInd_IoctrlMsg));
	cioctrl->eventCode = evCode;
	cioctrl->rbIdentity = rbid;
	cioctrl->rnti = rnti;
	fsm_do_ioctrl(STRM_TO_RRC, CRLC_STATUS_IND,( void * )cioctrl, sizeof(CRLC_StatusInd_IoctrlMsg));
	fsm_mem_free(cioctrl);
	cioctrl = NULL;
	FOUT;
}

//未实现
void PPRLC_DISC_REQ(void)  {}

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
void PMRLC_TRANSOP_IND (void* hptr)
{
	MRLC_TransOp_IoctrlMsg *mIoctrl, *pre;
	u32 subhead, controlelm, data, sevNum, fifNum;
	struct	AM_Instance * amIns;
	struct	UM_TX_Instance *umTxIns;
	struct	TM_TX_Instance *tmTxIns;
	void *insptru, *insptrd, *head, *ptr, *preptr;
	FSM_PKT* pkptr, *pkp;
	struct AmBuffer *amBuffer;
	struct Buffer *buffer, *tempb;
	struct	MRLC_subHead_fix_IciMsg *fix;
	struct MRLC_toMac_IciMsg *icimsg;
	u32 *nump,num;
	u32 size;	  //the total size in a buffer to transimit
	u32 subHeadLen;
	
	SV_PTR_GET(rlc_mac_sv);
	size = 0;
	//hptr = fsm_data_get();
	//fsm_printf("[RLC][PMRLC_TRANSOP_IND] enter PMRLC_TRANSOP_IND()\n");
	//FOUT;
	if(hptr == NULL)
	{
		FOUT;
	}
	nump = (u32*)hptr;
	num = *nump;
	if(num == 0)
	{
		fsm_data_destroy(hptr);
		fsm_printf("[RLC][PMRLC_TRANSOP_IND] there is no resource to send data\n");
		hptr = NULL;
		FOUT;
	}
	
	/* 根据传输机会中提供的数据大小，按最大需求计数数据包子头的大小*/
	//fsm_printf("[RLC][PMRLC_TRANSOP_IND] entering PMRLC_TRANSOP_IND(number of instances is %d)\n", num);
	mIoctrl = (MRLC_TransOp_IoctrlMsg *)(nump + 1);
	//设置比较，前个节点的rnti和下个节点的rnti
	pre = mIoctrl;
	subhead = data = sevNum = fifNum = 0;
	//mac层需要的头部控制长度
	while(num > 0)
	{
		//fsm_printf("number %d:\n", num);
		findInsBylcid(mIoctrl->rnti ,mIoctrl->lcid, &insptru, &insptrd, &SV(ins_mode));
		
		//if(SV(ins_mode)!=1)
		//{
		//	FOUT;
		//}
		
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
			check_num(tmTxIns, mIoctrl->txQueueSize, &sevNum, &fifNum);
			subhead += 2*sevNum + 3*fifNum;
			//fsm_printf("[RLC][PMRLC_TRANSOP_IND][TM]tm instance has %d 7bits sunhead,and %d 15bits\n", sevNum, fifNum);
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
			//fsm_printf("[RLC][PMRLC_TRANSOP_IND][UM] um instance(lcid is %d)'s buffer's subhead is %d \n",umTxIns->lcid,subhead);
			break;

		case AM_MODE:
			if(insptru == NULL)
			{
				break;
			}
			
			amIns = (AM_Instance * )insptru;
			//fsm_printf("[RLC][PMRLC_TRANSOP_IND 1 ][AM] amInsAddr =%p,rbid=%d,lcid=%d\n",amIns,amIns->rbId,amIns->lcId);
			data += mIoctrl->txQueueHeader + mIoctrl->txQueueSize + mIoctrl->retxQueueHeader + \
					mIoctrl->retxQueueSize + mIoctrl->statusPduHeader + mIoctrl->statusPduSize;
			sevNum = fifNum = 0;
			check_retx_num(amIns,mIoctrl->retxQueueSize,  &sevNum,	&fifNum);
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
			//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] am instance(lcid is %d)'s buffer's subhead is %d \n",amIns->lcId,subhead);
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

		//fsm_printf("[RLC][PMRLC_TRANSOP_IND] mIoctrl->rnti=%d, pre->rnti=%d, num=%d\n",mIoctrl->rnti,pre->rnti,num);

		if(mIoctrl->rnti != pre->rnti || num == 0)
		{
			//fsm_printf("[RLC][PMRLC_TRANSOP_IND] data=%d, subhead=%d\n", data,subhead);
			/* 根据计算出的大小建立SKB，并申请一块内存，保存ICI信息*/
			pkptr = fsm_pkt_create( data + sizeof(MRLC_toMac_IciMsg));
			head = fsm_mem_alloc(subhead + sizeof(MRLC_toMac_IciMsg) );  //暂时保存ici信息的内存区域,preptr用于在最后一个子头修改E域
			fsm_skb_reserve(pkptr, subhead + sizeof(MRLC_toMac_IciMsg));
			if(data<subhead)
			{
				printk("[RLC][PMRLC_TRANSOP_IND] data < subhead,data=%d,subhead=%d\n",data,subhead);
			}
			icimsg = (MRLC_toMac_IciMsg*)head;
			ptr = icimsg + 1;	//填充子头的指针
			data = 0;	  //计数实际封装包的总共的子头的大小
			/* 将PDU数据装入申请的SKB 中*/
			//mIoctrl = (MRLC_TransOp_IoctrlMsg *)(nump + 1);
			mIoctrl = pre;
			//num = *nump;
			while(mIoctrl->rnti == pre->rnti)
			{
				//		   fsm_printf("number %d:\n", num);
				findInsBylcid(mIoctrl->rnti,mIoctrl->lcid, &insptru, &insptrd, &SV(ins_mode));
				//fsm_printf("[RLC][PMRLC_TRANSOP_IND] the instance mode is %d\n", SV(ins_mode));
				
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
							if(buffer->pkt->len + subHeadLen >	mIoctrl->txQueueSize)
							{
								fsm_printf("[RLC][PMRLC_TRANSOP_IND][TM] break:tm instance mIoctrl->txQueueSize is %d, data is %d\n", mIoctrl->txQueueSize, buffer->pkt->len);
								break;
							}
							else
							{
								list_del(&buffer->list);
								tmTxIns->tmSduBufferSize -= buffer->pkt->len;//20140915加
								tmTxIns->tmSduBufferNum--;	//20140915加
								fsm_mem_cpy(pkptr->tail,  buffer->pkt->data, buffer->pkt->len);
								fsm_skb_put(pkptr, buffer->pkt->len);
								//mIoctrl->txQueueSize -= buffer->pkt->len;
								creat_subhead(buffer->pkt, &ptr, &preptr, tmTxIns->lcId, &data);
								mIoctrl->txQueueSize -= subHeadLen + buffer->pkt->len;	//20140915加
								//fsm_printf("tm instance mIoctrl->txQueueSize is %d, data is %d\n", mIoctrl->txQueueSize, buffer->pkt->len);
								//fsm_printf("[RLC][PMRLC_TRANSOP_IND][TM] the data is \n");	//FOR TEST
								//fsm_octets_print(buffer->pkt->data,18);

								fsm_pkt_destroy(buffer->pkt);
								buffer->pkt = NULL;
								fsm_mem_free(buffer);
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
						//pkp = rlc_UM_segment(umTxIns , mIoctrl->txQueueSize);
						if( mIoctrl->txQueueSize + mIoctrl->txQueueHeader -subHeadLen > PDU_MAX_SIZE)
						{
							pkp = rlc_UM_segment( umTxIns, PDU_MAX_SIZE);
						}
						else
						{
							pkp = rlc_UM_segment( umTxIns, mIoctrl->txQueueSize + mIoctrl->txQueueHeader -subHeadLen );
						}
						if(pkp == NULL)
						{
							printk("[RLC][PMRLC_TRANSOP_IND][UM] pkp == NULL\n");
							break;
						}
						fsm_mem_cpy( pkptr->tail, pkp->data, pkp->len);
						fsm_skb_put( pkptr,pkp->len);
						creat_subhead(pkp, &ptr,  &preptr, umTxIns->lcid, &data);
						//fsm_printf("um instance data is %d\n", data);
						fsm_pkt_destroy(pkp);
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
					
					//fsm_printf("[RLC][PMRLC_TRANSOP_IND 2 ][AM] amInsAddr =%p,rbid=%d,lcid=%d\n",&amIns,amIns->rbId,amIns->lcId);					
					//传输状态PDU
					if(!list_empty(& amIns->ctrlPduBuffer.list))
					{
						list_for_each_entry_safe(buffer, tempb, &amIns->ctrlPduBuffer.list,list )
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
								break;
							}
							else
							{
								list_del(&buffer->list);
								SV(countSendCtrlPdu)++;
								amIns->ctrlPduBufferSize -= buffer->pkt->len;
								amIns->ctrlPduBufferNum--;
								fsm_mem_cpy(pkptr->tail, buffer->pkt->data, buffer->pkt->len);
								fsm_skb_put(pkptr, buffer->pkt->len);
								mIoctrl->statusPduSize -= buffer->pkt->len + subHeadLen;
								creat_subhead(buffer->pkt, &ptr, &preptr, amIns->lcId, &data);
								//fsm_printf("am instance mIoctrl->statusPduSize is %d, data is %d\n", mIoctrl->statusPduSize, buffer->pkt->len);
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
									//fsm_printf("[RLC][PMRLC_TRANSOP_IND] amIns->ctrlPduBuffer is empty\n");
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
					size  = mIoctrl->retxQueueSize + mIoctrl->retxQueueHeader;
					if(!list_empty(&amIns->amRetxBuffer.list)&& size > 0)
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
									if( size - subHeadLen > PDU_MAX_SIZE)
									{
										pkp = rlc_resegment( amBuffer, PDU_MAX_SIZE);
									}
									else
									{
										pkp = rlc_resegment( amBuffer, size - subHeadLen );
									}
									//pkp =  rlc_resegment( amBuffer, size);
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
									fsm_pkt_destroy(pkp);  //20140922改
									pkp = NULL;
								}
								else
								{
									fsm_printf("[RLC][PMRLC_TRANSOP_IND] break:am instance mIoctrl->retxQueueSize is %d, data is %d\n", size, amBuffer->pkt->len+subHeadLen);
									break;
								}
							}
							/*else
							{
								continue;
							}*/
						}
					}
					//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] run to AM_MODE\n");
					
					//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] amInsAddr =%p,rbid=%d,lcid=%d\n",amIns,amIns->rbId,amIns->lcId);
					//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] mIoctrl->txQueueSize + mIoctrl->txQueueHeader=%d\n",mIoctrl->txQueueSize + mIoctrl->txQueueHeader);
					
					//传输新数据
					size = mIoctrl->txQueueSize + mIoctrl->txQueueHeader;
					if(isInsideTranWin(amIns,amIns->SN) &&(size > 0)&&(!list_empty(&amIns->amSduBuffer.list)))
					{
						if( amIns->amSduBufferSize < 128 )
						{
							subHeadLen = 2;
						}
						else
						{
							subHeadLen = 3;
						}
						//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] mIoctrl->txQueueSize + mIoctrl->txQueueHeader=%d\n",mIoctrl->txQueueSize + mIoctrl->txQueueHeader);						
						if( size > subHeadLen )
						{
							if( size - subHeadLen > PDU_MAX_SIZE)
							{
								pkp = rlc_AM_segment( amIns, PDU_MAX_SIZE);
							}
							else
							{
								pkp = rlc_AM_segment( amIns, size -subHeadLen );
							}
							if(pkp == NULL)
							{
								printk("[RLC][PMRLC_TRANSOP_IND][AM] pkp == NULL\n");
								break;
							}
							//printk("[RLC][PMRLC_TRANSOP_IND][AM] size=%d,pkp->len=%d, subHeadLen=%d\n",size,pkp->len,subHeadLen);
							//fsm_printf("[RLC][PMRLC_TRANSOP_IND] pkp->len=%d\n",pkp->len);
							if( amIns->pduWithoutPoll >= amIns->pollPDU || amIns->byteWithoutPoll >= amIns->pollBYTE )
							{
								rlc_poll(amIns,pkp);
							}
							if(pkptr->tail + pkp->len > pkptr->end)
							{
								//printk("[RLC][PMRLC_TRANSOP_IND][AM] skb_over_panic,%p, %p\n",pkptr->tail+pkp->len,pkptr->end);
								FOUT;
							}
							fsm_mem_cpy( pkptr->tail, pkp->data, pkp->len);
							fsm_skb_put( pkptr,pkp->len);
							
							creat_subhead(pkp, &ptr, &preptr, amIns->lcId, &data);
							amIns->vt_s = (amIns->vt_s + 1)%(AM_WINDOW_SIZE*2);
							//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] amIns->lcId = %d\n",amIns->lcId);
							//fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] amIns->vt_s  is updated to %d, the len of the data is %d\n",amIns->vt_s , pkp->len);
							//更新pduWithoutPoll和byteWithoutPoll，并判断是否需要轮询
							amIns->pduWithoutPoll++;
							amIns->byteWithoutPoll += pkp->len;
							fsm_printf("[RLC][PMRLC_TRANSOP_IND][AM] data is:\n");
							fsm_octets_print(pkptr->data,10);
							if(pkp!=NULL)
							{
								fsm_pkt_destroy(pkp);  //20140922改
								pkp = NULL;
							}
							
							//fsm_data_destroy(hptr);
							//FOUT;
							
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
				mIoctrl++;			
			}
			if(0 == pkptr->len)
			{
				fsm_printf("[RLC][PMRLC_TRANSOP_IND] there is no data to send\n");
				fsm_pkt_destroy(pkptr);
				pkptr = NULL;
				fsm_mem_free(head);
				head = NULL;
				//FOUT;
			}
			else
			{
				fix = (MRLC_subHead_fix_IciMsg *)preptr;
				fix->E = 0;    //将最后一个子头的E域改为0
				icimsg->len = data;
				//icimsg->rnti = 0;
				icimsg->rnti = (--mIoctrl)->rnti;  //20141015
				fsm_mem_cpy(pkptr->head, head, data + sizeof(MRLC_toMac_IciMsg));
				fsm_mem_free(head);
				head = NULL;
				//fsm_printf("[RLC][PMRLC_TRANSOP_IND] SV(count)=%d\n",SV(count));
				/*if(SV(count)>2)
				{
					SV(isBufferReq) = 0;
					fsm_pkt_destroy(pkptr);
					pkptr = NULL;
					fsm_mem_free(head);
					head = NULL;
					FOUT;
				}*/
				//fsm_pkt_send(pkptr, STRM_TO_MAC);
				
				complexEnbMacPdu(pkptr); //add in 20150804
				SV(count)++;
				SV(countSendToLower)++;
				//fsm_printf("[RLC][PMRLC_TRANSOP_IND] the data len = %d\n",pkptr->len);	//FOR TEST
				//fsm_octets_print(pkptr->data,pkptr->len);
				//fsm_pkt_destroy(pkptr);
				fsm_printf("[RLC][PMRLC_TRANSOP_IND][<--] has sent data to MAC\n");
				mIoctrl++;
			}		
			//发送完一次数据，重置数据
			subhead = data = sevNum = fifNum = 0;	
			pre = mIoctrl;
		}	
		//fsm_printf("[RLC] run to here\n");
		//FOUT;
	}
	if(hptr != NULL)
	{
		fsm_data_destroy(hptr);
		hptr = NULL;
	}
	//SV(isBufferReq) = 0;
	//PMRLC_BUFFERREP_REQ();
	/*收到传输机会后重启bufferReq定时器*/
	/*if( SV(bufferReqTimer) )
	{
		fsm_schedule_cancel(SV(bufferReqTimer));
		SV(bufferReqTimer) = 0;
	}
	SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));*/
	FOUT;
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
void* PMRLC_BUFFERREP_REQ(void) //modify in 20150804
{
	MRLC_ReportReq_IoctrlMsg *mioctrl;
	//   struct list_head* pos;
	struct RntiManag * rntiM;
	u32 num = 0,*nump;                              //需要发送的结构体个数
	struct  AM_Instance * amIns;
	struct  UM_TX_Instance *umTxIns;
	//struct  TM_TX_Instance *tmTxIns;
	struct AmBuffer *amBuffer;
	CtrlBuffer *ctrlBf;
	
	SV_PTR_GET(rlc_mac_sv);
	//计数需要发送的数据大小
	if(!list_empty(&SV(rntiManag).list))
	{
		list_for_each_entry(rntiM,&SV(rntiManag).list, list)
		{
			if (!list_empty(&rntiM->amIns.amList))
			{
				list_for_each_entry(amIns,&rntiM->amIns.amList, amList)
				{
					if(amIns->amSduBufferSize > 0 || amIns->amRetxBufferSize > 0 || amIns->ctrlPduBufferSize > 0)
					{
						fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] am instance (rbid is %d) has data to report\n", amIns->rbId);
						num++;
					}
				}
			}

			if (!list_empty(&rntiM->umTxIns.umTxList))
			{
				list_for_each_entry(umTxIns,&rntiM->umTxIns.umTxList,umTxList )
				{
					//      umTxIns= list_entry(pos,struct UM_TX_Instance,umTxList);
					if(umTxIns->umSduBufferSize > 0)
					{
						fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] um instance (rbid is %d) has data to report\n", umTxIns->rbid);
						num++;
					}
				}
			}

			if(rntiM->ccchTran.tmSduBufferSize > 0)
			{
				fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] tm instance (rbid is %d) has data to report\n", rntiM->ccchTran.rbId);
				num++;
			}
		}
	}

	if(num == 0)
	{
		fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] There is no data to send and the bufferReqTimer is cancelled\n");
		/*SV(isBufferReq) = 0;
		if( SV(bufferReqTimer) )
		{
			fsm_schedule_cancel(SV(bufferReqTimer));
			SV(bufferReqTimer) = 0;
		}*/
		return NULL;
		//FOUT;
	}
	//分配空间大小
	nump = (u32*) fsm_mem_alloc(sizeof(u32) + sizeof(MRLC_ReportReq_IoctrlMsg)*num);
	*nump = num;
	mioctrl = (MRLC_ReportReq_IoctrlMsg*)(nump + 1);
	//填充分配的内存空间
	if(!list_empty(&SV(rntiManag).list))
	{
		list_for_each_entry(rntiM, &SV(rntiManag).list,list)
		{
			if (!list_empty(&rntiM->amIns.amList))
			{
				list_for_each_entry(amIns,&rntiM->amIns.amList,amList)
				{
					//      amIns = list_entry(pos,struct AM_Instance,amList);
					if(amIns->amSduBufferSize > 0 || amIns->amRetxBufferSize > 0 || amIns->ctrlPduBufferSize > 0)
					{
						//mioctrl->rnti = 0;
						mioctrl->rnti = rntiM->rnti;  //20141015
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
						mioctrl->retxQueueHeader = 2*amIns->amRetxBufferNum;
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
						//fsm_printf("[RLC][PMRLC_BUFFERREP_REQ][AM] the total size is %d(lcid is %d)\n", mioctrl->txQueueSize + mioctrl->retxQueueSize + \
							//mioctrl->statusPduSize + mioctrl->statusPduHeader + mioctrl->txQueueHeader + mioctrl->retxQueueHeader, mioctrl->lcid);
						mioctrl++;
					}
				}
			}

			if (!list_empty(&rntiM->umTxIns.umTxList))
			{
				list_for_each_entry(umTxIns,&rntiM->umTxIns.umTxList, umTxList)
				{
					//   umTxIns= list_entry(pos,struct UM_TX_Instance,umTxList);
					if(umTxIns->umSduBufferSize > 0)
					{
						//mioctrl->rnti = 0;
						mioctrl->rnti = rntiM->rnti;  //20141015
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
						fsm_printf("[RLC][PMRLC_BUFFERREP_REQ][UM] the total size is %d(lcid is %d)\n", mioctrl->txQueueSize  + mioctrl->txQueueHeader, mioctrl->lcid);
						mioctrl++;
					}
				}
			}
			if(rntiM->ccchTran.tmSduBufferSize > 0)
			{
				//mioctrl->rnti = 0;
				mioctrl->rnti = rntiM->rnti;  //20141015
				mioctrl->lcid = rntiM->ccchTran.lcId;
				mioctrl->txQueueSize = rntiM->ccchTran.tmSduBufferSize;
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
				fsm_printf("[RLC][PMRLC_BUFFERREP_REQ][TM] the total size is %d(lcid is %d)\n", mioctrl->txQueueSize, mioctrl->lcid);
				mioctrl++;
			}
		}
	}
	/*if( num > 0 )
	{
		//fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] the num of instances to report is %d, and the space is %d\n", num, sizeof(u32) + sizeof(MRLC_ReportReq_IoctrlMsg)*num );
		//fsm_do_ioctrl(STRM_TO_MAC, MRLC_BUFREPORT_REQ, (void*)nump, sizeof(u32) + sizeof(MRLC_ReportReq_IoctrlMsg)*num);
		//fsm_mem_free(nump);
		
		//nump = NULL;
		//SV(isBufferReq) = 1;
		fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] -->has send bufferReq to MAC\n");
		if( !SV(bufferReqTimer) )
		{
			//fsm_printf("After bufferReq the bufferReqTimer start\n");
			SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
		}
		
	}
	else
	{
		fsm_printf("[RLC][PMRLC_BUFFERREP_REQ] There is no data to send and the bufferReqTimer is cancelled\n");
		fsm_mem_free(nump);
		nump = NULL;
		SV(isBufferReq) = 0;
		if( SV(bufferReqTimer) )
		{
			fsm_schedule_cancel(SV(bufferReqTimer));
			SV(bufferReqTimer) = 0;
		}
	}*/ //noted in 20150804
	return (void*)nump; //add in 20150804
}
