/************************************************
**Filename:rlcfsm.c
**Copyright:uestc
**Descriptions:
**v1.0,2013,ChenHongying and ZhangYi written
**---------------------------------------------
**modification history:
**Modified by: ZhangYi
**Modified date: 2014
**Descriptions:
**
************************************************/
#include "rlcfsm.h"
#include <linux/if_ether.h>
#include "../pkfmt.h"
#include "Rlcprimitives.h"
#include "../lte_system.h"

/*
#define 	ST_RECVU		4
#define 	ST_RECVL		5
#define 	ST_SENDL		6
#define 	ST_SENDU		7
#define 	ST_RETX			8
*/


//测试用
extern void Am_instance_add(void);
extern void UM_instance_bulid(void);
extern void addpkt_recvData(struct AM_Instance *amIns);   
extern void addPollRetxData(struct AM_Instance*amIns);
extern void skb_printf(FSM_PKT *pkt);
extern void buff_printf(struct AM_Instance *amIns);
extern void add_data_retx(struct AM_Instance*amIns, u8 lcid);

/**********************************************************
**Function name: rlc_main()
**Descriptions: the main frame of the FSM
**Input: 
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
void rlc_main(void)
{
	FSM_ENTER(rlc_main);
	FSM_BLOCK_SWITCH
	{
		/*FSM_STATE_UNFORCED(ST_INIT, "INIT",rlc_init_enter(), )
		{
			FSM_COND_TEST_IN("INIT")
			FSM_TEST_COND(RLC_START_WORK)
			FSM_COND_TEST_OUT("INIT")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_TRAN, , "INIT -> TRAN")
				FSM_CASE_DEFAULT(ST_INIT, , "INIT->INIT")
			}
		}*/
		FSM_STATE_FORCED(ST_INIT, "INIT", rlc_init_enter(), )
		{
			FSM_TRANSIT_FORCE(ST_TRAN, , "default", "", "INIT -> TRAN"); 
		}
		FSM_STATE_UNFORCED(ST_TRAN, "TRAN",,rlc_interupt() )
		{
			FSM_COND_TEST_IN("TRAN")
			FSM_TEST_COND(RLC_SUSPEND)
			FSM_TEST_COND(RLC_DEACT)
			FSM_TEST_COND(RLC_RESUME)
			FSM_COND_TEST_OUT("TRAN")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0,ST_SUSPEND,  ,"TRAN -> SUSPEND")
				FSM_CASE_TRANSIT(1,ST_NULL,  , "TRAN -> NULL")
				FSM_CASE_TRANSIT(2,ST_INIT,  , "TRAN -> INIT")
				FSM_CASE_DEFAULT(ST_TRAN, , "TRAN->TRAN")	//transit to transimition state	by default.
			}
		}
		FSM_STATE_FORCED(ST_SUSPEND, "SUSPEND", , )
		{
			FSM_TRANSIT_FORCE(ST_TRAN, , "default", "", "SUSPEND -> TRAN");
		}
		FSM_STATE_FORCED(ST_NULL, "NULL", , )
		{
			FSM_TRANSIT_FORCE(ST_TRAN, , "default", "", "NULL -> TRAN");
		}
	}
	FSM_EXIT(0)
}

/**********************************************************
**Function name: rlc_init_enter()
**Descriptions: RLC initialization and build TM instances
**Input: 
**Output: build TM instances for PCCH BCCH and CCCH
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
static void rlc_init_enter(void)
{
	TM_RX_Instance *tmRxIns;
	TM_TX_Instance *tmTxIns;
	int i;
	
	FIN(rlc_init_enter());
	SV_PTR_GET(rlc_sv);
	if(RLC_OPEN)
	{
		// SV(ins_mode) = TM_MODE;
		SV(isBufferReq) = 0;
		SV(packetType) = -1;
		INIT_LIST_HEAD(&SV(vAm_ins).amList);
		INIT_LIST_HEAD(&SV(vUm_rx_ins).umRxList);
		INIT_LIST_HEAD(&SV(vUm_tx_ins).umTxList);
		INIT_LIST_HEAD(&SV(vTm_rx_ins).tmRxList);
		INIT_LIST_HEAD(&SV(vTm_tx_ins).tmTxList);
		//mallocnum=0;
		//freenum=0;
	//	SV(pduLft).SN_Left = -1;
	//	SV(pduLft).sduLeft = NULL;
		//for test
		SV(countRecvFromUpper)=0;
		SV(countSendToUpper)=0;
		SV(countRecvFromLower)=0;
		SV(countSentToLower)=0;
		SV(countDropPacket)=0;
		SV(countSendCtrlPdu)=0;
		SV(countRecvCtrlPdu)=0;
		SV(max_queue_num)=0;
		//定时器管理数组初始化
		for(i = 0; i<MAX_CODE; i++) SV(allocCode[i]) = i;
		SV(allocCode[0]) =  SV(allocCode[0x0B]) =  SV(allocCode[0x21])  = SV(allocCode[0x22]) = SV(allocCode[0x23]) =\
							SV(allocCode[0x24]) = SV(allocCode[0x25]) = SV(allocCode[0x26]) = SV(allocCode[0x27]) = SV(allocCode[0x28]) = \
							SV(allocCode[0x29]) = SV(allocCode[0x30]) = -1;//0x30为bufferReq定时器
		SV(bufferReqTimerValue) = 10*100;  //暂定
		SV(bufferReqTimerCode) = 0x30;
		SV(bufferReqTimer) = 0;
		//fsm_printf("rlc init is called");
		/* TM receiving instance estabilshment corresponding to PCCH  */
		tmRxIns = instance_create(TM_RX_Instance) ;
		tmRxIns->pbCh = 1;            //PCCH
		tmRxIns->rnti = 0;
		tmRxIns->rbId = 0;
		tmRxIns->lcId = 0;
		list_add_tail(&tmRxIns->tmRxList, &SV(vTm_rx_ins).tmRxList);
		fsm_printf("[RLC][rlc_init_enter] PCCH instance has bulided\n");

		/* TM receiving instance estabilshment corresponding to BCCH  */
		tmRxIns = instance_create(TM_RX_Instance) ;
		tmRxIns->pbCh = 2;            //BCCH
		tmRxIns->rnti = 0;
		tmRxIns->rbId = 0;
		tmRxIns->lcId = 0;       //BCCH
		list_add_tail(&tmRxIns->tmRxList, &SV(vTm_rx_ins).tmRxList);
		fsm_printf("[RLC][rlc_init_enter] BCCH instance has bulided\n");

		/* TM receiving instance estabilshment corresponding to CCCH  */
		tmRxIns = instance_create(TM_RX_Instance) ;
		tmRxIns->pbCh = 0;            //CCCH
		tmRxIns->rnti = 0;
		tmRxIns->rbId = 0;
		tmRxIns->lcId = 0;       //CCCH
		list_add_tail(&tmRxIns->tmRxList, &SV(vTm_rx_ins).tmRxList);
		fsm_printf("[RLC][rlc_init_enter] CCCH recv instance has bulided\n");

		/* TM transimitting instance estabilshment corresponding to CCCH  */
		tmTxIns = instance_create(TM_TX_Instance) ;
		tmTxIns->pbCh = 0;            //CCCH
		tmTxIns->rnti = 0;
		tmTxIns->rbId = 0;
		tmTxIns->lcId = 0;       //CCCH
		INIT_LIST_HEAD(&tmTxIns->tmSduBuffer.list);
		tmTxIns->tmSduBufferNum = 0;
		tmTxIns->tmSduBufferSize = 0;
		list_add_tail(&tmTxIns->tmTxList, &SV(vTm_tx_ins).tmTxList);
		fsm_printf("[RLC][rlc_init_enter] CCCH tran instance has bulided\n");

		//为测试建立一个AM实体
		//Am_instance_add();
		//UM_instance_bulid();
		//fsm_schedule_self(0, _START_WORK);

	}

	FOUT;
}

/**********************************************************
**Function name: rlc_interupt()
**Descriptions: Process the interuption of the FSM, including packet and ioctl arrival,and timeout of timer
**Input: 
**Output: Process the interuption of the FSM, including packet and ioctl arrival,and timeout of timer
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-----------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**********************************************************/
static void rlc_interupt()
{
	//  struct list_head* pos;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	void *insPtr;
	int i;
	//int code;
	
	FIN(rlc_interupt());
	SV_PTR_GET(rlc_sv);
	if(RLC_CLOSE)                              //模块关闭
	{
		rlc_close();
	}
//测试
//    else if(RLC_SEGMENT);//分段测试函数
//    else if(RLC_RESEGMENT);
//    else if(RLC_REASSAMBLE);
//    else if(RLC_POLL);
//    else if(RLC_CTRL_PKT_TRAN);
//    else if(RLC_CTRL_PKT_RECV);

	else if(RLC_IOCTL_ARRIVAL)         	//ioctrl
	{
		rlc_ioctrl_handler();
	}
	else if(RLC_PKT_FROM_LOWER)       //接收来自下层的包
	{
		SV(countRecvFromLower)++;
		rlc_pkt_receive_from_mac();
	}
	else if(RLC_PKT_FROM_UPPER)		//接收来自上层的包
	{
		SV(countRecvFromUpper)++;
		rlc_pkt_receive_from_upper();
	}
	else if(RLC_EV_SELF)				// 定时器超时，定位实体/
	{
		if(!(insPtr = findInsByCode(fsm_ev_code(), &SV(timerType))))
		{
			//fsm_printf("[RLC][rlc_interupt] the instance of this code is NULL\n");
			//fsm_printf("cannot find the instance of this code \n ");
			//FOUT;
		}
		switch(SV(timerType))
		{
		case POLL_RETX:
			amIns = (AM_Instance *)insPtr;
			amIns->pollRetxTimer = 0;
			rlc_poll_retx(amIns);
			//fsm_printf("[RLC][rlc_interupt] poll timeout\n");
			FOUT;
			break;

		case STAT_PRO:
			amIns = (AM_Instance *)insPtr;
			amIns->statProhTimer = 0;
			if(amIns->statFlag)
			{
				rlc_ctrl_pkt_trans(amIns);
				if(SV(isBufferReq) == 0)
				{
					PMRLC_BUFFERREP_REQ();
					FOUT;
				}
				if( !SV(bufferReqTimer) )
				{
					fsm_printf("[RLC][rlc_interupt] bufferReqTimer start\n");
					SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
				}
				else
				{
					fsm_printf("[RLC][rlc_interupt] bufferReqTimer is runing\n");
				}
			}
		
			FOUT;
			break;

		case UM_REORDER:
			umRxIns = (UM_RX_Instance *) insPtr;
			umRxIns->reorderTimer = 0;
			fsm_printf("[RLC][rlc_interupt] UM_REORDER timeout (rbid is %d)\n", umRxIns->rbId);
			if(umRxIns->snFiledLength == 5)
			{
				umRxIns->vr_ur = (umRxIns->vr_ur+1)%(UM_WINDOW_SIZE5*2);
				while((umRxIns->umRecvWindow5[umRxIns->vr_ur] != NULL) && (umRxIns->vr_ur <= umRxIns->vr_ux))
				{
					umRxIns->vr_ur = (umRxIns->vr_ur+1)%(UM_WINDOW_SIZE5*2);
				}
				rlc_UM5_reassemble_deliver(umRxIns,&umRxIns->pduLft);
			}
			if(umRxIns->snFiledLength == 10)
			{
				//umRxIns->vr_ur = umRxIns->vr_ux - 1;
				umRxIns->vr_ur = umRxIns->vr_ux%(UM_WINDOW_SIZE10*2);
				while((umRxIns->umRecvWindow10[umRxIns->vr_ur] != NULL) && (umRxIns->vr_ur >= umRxIns->vr_ux))
				{
					umRxIns->vr_ur = (umRxIns->vr_ur+1)%(UM_WINDOW_SIZE10*2);
				}
				rlc_UM_reassemble_deliver(umRxIns,&umRxIns->pduLft);
			}
			// fsm_pkt_send(rlc_UM_reassemble_deliver(umRxIns), STRM_TO_PDCP);
			//fsm_printf("[RLC][rlc_interupt] umRxIns->vr_ur is %d, umRxIns->vr_ux is %d\n\n",umRxIns->vr_ur, umRxIns->vr_ux);
			FOUT;
			break;

		case AM_REORDER:
			amIns = (AM_Instance *)insPtr;
			amIns->reorderTimer = 0;
			//fsm_printf("[RLC][rlc_interupt] am reorder timeout\n");

			for(i = amIns->vr_r; i < amIns->vr_ms; i++)   //20141201
			{
				if(amIns->amRecvWindow[i] == NULL)
				{
					amIns->statFlag = true;  //将状态报告标识置为true
					if(!SV(bufferReqTimer))
					{
						SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
					}
					break;
				}
			}
			//更新vr_ms为第一个序列号不小于vr_x，并且还没有对其所有字节分段接收完全的AMD PDU
			amIns->vr_ms = amIns->vr_x % 1024;
			while((amIns->vr_ms >= amIns->vr_x)&&((amIns->amRecvWindow[amIns->vr_ms] != NULL)\
					&&(amIns->amRecvWindow[amIns->vr_ms]->pktstatus != FULL_SDU)))
			{
				amIns->vr_ms = (amIns->vr_ms+1)%1024;
			}
			//fsm_printf("[RLC][rlc_interupt] amIns->vr_ms is updated to %d\n",amIns->vr_ms);
			rlc_AM_reassemble_deliver(amIns,&(amIns->pduLft));
			if(amIns->vr_h > amIns->vr_ms || amIns->vr_h < amIns->vr_ms - AM_WINDOW_SIZE)
			{
				amIns->reorderTimer = fsm_schedule_self(amIns->reorderTimerValue, amIns->reorderTimerCode);
				amIns->vr_x = amIns->vr_h;
				//fsm_printf("[RLC][rlc_interupt] amIns->vr_x is updated to %d, timer is sheduling again\n", amIns->vr_x );
			}
			//是否递交??
			
			FOUT;
			break;
		case BUFFER_REQ:
			//fsm_printf("[RLC][rlc_interupt] the bufferReqTimer timeout\n");
			if( SV(bufferReqTimer) )
			{
				fsm_schedule_cancel(SV(bufferReqTimer));
 				SV(bufferReqTimer) = 0;
			}
			//fsm_printf("[RLC][rlc_interupt] SV(isBufferReq)=%d\n",SV(isBufferReq));
			//if( SV(isBufferReq) == 0 )
			//{
				PMRLC_BUFFERREP_REQ();
			//}
			
			FOUT;
			break;
		default:
			printk("[RLC][rlc_interupt] cannot distinguish this code \n");
			break;
		}
	}
	else
	{
		printk("[RLC][rlc_interupt] cannot distinguish the interrupt\n");
	}
	FOUT;
}

/********************************************************************
**Function name: rlc_pkt_receive_from_upper()
**Descriptions: Receive packets from RRC or PDCP layer,add it into sdu buffer
**Input: 
**Output: Receive packets from RRC or PDCP layer,add it into sdu buffer
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**--------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
*********************************************************************/
static void rlc_pkt_receive_from_upper(void)
{
	FSM_PKT * skb;
//	struct list_head* pos;
	TmBuffer *tmElm;
	struct UmBuffer *umElm;
	struct AmSduBuffer *amElm;
	URLC_IciMsg * iciMsg;
	struct  AM_Instance * amIns;
	struct  UM_TX_Instance *umTxIns;
	struct  TM_TX_Instance *tmTxIns;
	void *insptru, *insptrd;
	//int i=0;
	
	FIN(rlc_pkt_receive_from_upper());
	SV_PTR_GET(rlc_sv);	

	skb = (FSM_PKT *)fsm_pkt_get();
	//fsm_printf("\n[RLC][rlc_pkt_receive_from_upper] entering rlc_pkt_receive_from_upper()\n");
	
	iciMsg = (URLC_IciMsg*)skb->head;
	fsm_printf("[RLC][rlc_pkt_receive_from_upper][-->] receive from upper, pbch=%d, rbid=%d\n",iciMsg->pbCh,iciMsg->rbId);
	fsm_octets_print(skb->data,2);
	//fsm_printf("the icimsg are %d, %d, %d\n",iciMsg->pbCh, iciMsg->rbId,iciMsg->rnti);
	//fsm_printf("[RLC] pbch is %d\n",iciMsg->pbCh);
	if(iciMsg->pbCh == 0)
	{
		findInsByRbid(iciMsg->rbId, &insptru, &insptrd ,&SV(ins_mode));
		switch(SV(ins_mode))
		{
		case AM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_upper][AM] rbid(%d) has no am instance \n",iciMsg->rbId);
				break;
			}
			amIns = (AM_Instance *)insptru;
			if(amIns->currentStatVar == ST_SUSPEND)
			{
				printk("[RLC][rlc_pkt_receive_from_upper][AM] am instance is suspended(rbid is %d)\n",amIns->rbId);
				break;
			}
			
			amElm = (AmSduBuffer *)fsm_mem_alloc(sizeof(AmSduBuffer));
			/*mallocnum++;
			if(mallocnum%25==0)
			{
				printk("RLC ALLOC: %d\n", mallocnum);
			}*/
			amElm->pkt = skb;
			amElm->pktstatus = FULL_SDU;
			amElm->pos = 0;
			list_add_tail(&amElm->list, &amIns->amSduBuffer.list);

			//fsm_printf("[RLC][rlc_pkt_receive_from_upper] amIns addr is %p, amIns->rbid=%d, amIns->lcid=%d\n",&amIns,amIns->rbId,amIns->lcId);
			amIns->amSduBufferSize += skb->len;
			amIns->amSduBufferNum++;
			//fsm_printf("[RLC][rlc_pkt_receive_from_upper] isBufferReq = %d\n",SV(isBufferReq));
			
			if(SV(isBufferReq) == 0)
			{
				PMRLC_BUFFERREP_REQ();
				FOUT;
			}
			if( !SV(bufferReqTimer) )
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper][AM] bufferReqTimer start\n");
				SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
			}
			else
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper][AM] bufferReqTimer is runing\n");
			}
			FOUT;
			break;

		case UM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_upper][UM] rbid (%d )has no um tran instance \n",iciMsg->rbId);
				break;
			}
			umTxIns = (UM_TX_Instance *)insptru;
			if(umTxIns->CurrentStatVar == ST_SUSPEND)
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper][UM] um instance is suspended(rbid is %d)\n",umTxIns->rbid);
				break;
			}

			umElm = (UmBuffer *)fsm_mem_alloc(sizeof(UmBuffer));
			umElm->pkt = skb;
			umElm->pktstatus = FULL_SDU;
			umElm->pos = 0;
			list_add_tail(&umElm->list, &umTxIns->umSduBuffer.list);
			umTxIns->umSduBufferNum++;
			if(SV(max_queue_num) < (umTxIns->umSduBufferNum)){
				SV(max_queue_num) = umTxIns->umSduBufferNum;
			}
			umTxIns->umSduBufferSize += skb->len;
			if(SV(isBufferReq) == 0)
			{
				PMRLC_BUFFERREP_REQ();
				FOUT;
			}
			if( !SV(bufferReqTimer) )
			{
				//fsm_printf("[RLC][rlc_pkt_receive_from_upper][UM] bufferReqTimer start\n");
				SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
			}
			else
			{
				//fsm_printf("[RLC][rlc_pkt_receive_from_upper][UM] bufferReqTimer is runing\n");
			}
			FOUT;
			break;

		case TM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_upper][UM] rbid (%d )has no um tran instance \n",iciMsg->rbId);
				break;
			}
			tmTxIns = (TM_TX_Instance *)insptru;
			
			tmElm = (TmBuffer *) fsm_mem_alloc(sizeof(TmBuffer));
			tmElm->pkt = skb;
			list_add_tail(&tmElm->list, &tmTxIns->tmSduBuffer.list);
			tmTxIns->tmSduBufferNum++;
			tmTxIns->tmSduBufferSize += skb->len;
			
			if(SV(isBufferReq) == 0)
			{
				PMRLC_BUFFERREP_REQ();
				FOUT;
			}
			if( !SV(bufferReqTimer) )
			{
				//fsm_printf("[RLC][rlc_pkt_receive_from_upper][TM] bufferReqTimer start\n");
				SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
			}
			else
			{
				//fsm_printf("[RLC][rlc_pkt_receive_from_upper][TM] bufferReqTimer is runing\n");
			}
			FOUT;
			break;
		default:
			printk("[RLC][rlc_pkt_receive_from_upper] cannot distinfuish this  %d", SV(ins_mode));
		break;
		}
		printk("[RLC][rlc_pkt_receive_from_upper] A packet has dropped\n");
		SV(countDropPacket)++;
		if(skb != NULL)
		{
			fsm_pkt_destroy(skb);
			skb = NULL;
		}
	}
	else
	{
		printk("[RLC][rlc_pkt_receive_from_upper] no logic channel to transimmit this message\n");
		printk("[RLC][rlc_pkt_receive_from_upper] A packet has dropped\n");
		SV(countDropPacket)++;
		fsm_pkt_destroy(skb);
		skb = NULL;
	}
	FOUT;
}

/******************************************************************************
**Function name: rlc_pkt_receive_from_mac()
**Descriptions: Receive packets from MAC layer, and put it in AM or UM instances receive buffer
**Input: 
**Output: Receive packets from MAC layer, and put it in AM or UM instances receive buffer
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**-------------------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
******************************************************************************/
static void rlc_pkt_receive_from_mac(void)
{
	FSM_PKT * skb;
	//  struct list_head* pos;
	MRLC_fromMac_IciMsg* micimsg;
	URLC_IciMsg icimsg;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
//   struct  TM_RX_Instance *tmRxIns;
	void *insptru, *insptrd;

	FIN(rlc_pkt_receive_from_mac());
	SV_PTR_GET(rlc_sv);

	skb = fsm_pkt_get();

	//fsm_printf("[RLC][rlc_pkt_receive_from_mac][-->] receive from lower\n");
	//fsm_octets_print(skb->data,50);
	micimsg = (MRLC_fromMac_IciMsg*)skb->head;
	fsm_printf("\n[RLC][rlc_pkt_receive_from_mac] entering rlc_pkt_receive_from_mac(lcid is %d,pbch is %d)\n",micimsg->lcid,micimsg->pbCh);
	fsm_octets_print(skb->data,2);
	//fsm_printf("[RLC][rlc_pkt_receive_from_mac][-->] receive from mac\n");
	if(micimsg->pbCh ==0 && micimsg->lcid != 0 )
	{
		findInsBylcid(micimsg->lcid, &insptru, &insptrd,&SV(ins_mode));
		switch(SV(ins_mode))
		{
		case AM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_mac][AM] lcid(%d) has no am instance \n",micimsg->lcid);
				break;
			}
			amIns = ( AM_Instance *)insptru;
			if(amIns->currentStatVar == ST_SUSPEND)
			{
				printk("[RLC][rlc_pkt_receive_from_mac][AM] am instance is suspended(rbid is %d)\n",micimsg->lcid);
				break;
			}
			//addPollRetxData(amIns);
			//amIns->pollRetxTimer = fsm_schedule_self(amIns->pollRetxTimerValue,amIns->pollRetxTimerCode);
			//fsm_printf("After am receive_from_mac pollRetxRimer is running\n");
			//addpkt_recvData(amIns);				
			rlc_AM_pkt_receive_from_mac(amIns,skb);
			//    fsm_printf("time for rlc_AM_pkt_receive_from_mac(amIns,skb)\n");
			FOUT;
			break;

		case UM_MODE:
			if(insptrd == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_mac][UM] lcid(%d) has no am instance \n",micimsg->lcid);
				break;
			}
			umRxIns = (UM_RX_Instance *)insptrd;
			if(umRxIns->currentStatVar == ST_SUSPEND)
			{
				printk("[RLC][rlc_pkt_receive_from_mac][UM] am instance is suspended(rbid is %d)\n",micimsg->lcid);
				break;
			}
			rlc_UM_pkt_receive_from_mac(umRxIns ,skb);
			FOUT;
			break;

		default:
			printk("[RLC][rlc_pkt_receive_from_mac] cannot distinfuish this  %d", SV(ins_mode));
			break;
		}
		//fsm_printf("[RLC][rlc_pkt_receive_from_mac] cannot find the instance or the instance is suspended(rbid is %d)\n", micimsg->lcid);
		printk("[RLC][rlc_pkt_receive_from_mac] A packet has dropped\n");
		SV(countDropPacket)++;
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
		icimsg.pbCh = micimsg->pbCh;
		icimsg.rbId = 0;
		fsm_mem_cpy(skb->head, &icimsg , sizeof(URLC_IciMsg));
		fsm_printf("[RLC][rlc_pkt_receive_from_mac][<--] send to RRC\n");
		//fsm_octets_print(skb->data, 100);
		SV(countSendToUpper)++;
 		fsm_pkt_send(skb, STRM_TO_RRC);
	}
	FOUT;
}



/**************************************************************************
**Function name: rlc_AM_pkt_receive_from_mac()
**Descriptions: Receive packets from MAC layer, and put it in AM instances receive buffer
**Input: AM_Instance * amIns, FSM_PKT * pkt
**Output: Receive packets from MAC layer, and put it in AM instances receive buffer
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**--------------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**************************************************************************/
static void rlc_AM_pkt_receive_from_mac(AM_Instance * amIns, FSM_PKT * pkt)
{
	RLC_AMPDU_head* head;
	RLC_AMPDU_seg_head *seghead;
	RLC_AMPDU_fixhead *fixhead;
	RLC_AMPDU_seg_exthead* extseghead;
	AmBuffer *ambuf;
	FSM_PKT *pkptr;
	char *bufptr;
	int datalen;
	SequenceNumber SN;
	struct list_head *pos;
	int i;	
	FIN(rlc_AM_pkt_receive_from_mac());
	SV_PTR_GET(rlc_sv);
	
	fixhead = ( RLC_AMPDU_fixhead *)pkt->data;
	*(u16 *)fixhead = ntohs(*(u16 *)fixhead);
	fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] the header is:\n");
	fsm_octets_print(fixhead,2);
	//fsm_printf("\n[RLC][rlc_AM_pkt_receive_from_mac] entering rlc_AM_pkt_receive_from_mac\n");
	//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] data len = %d\n",pkt->len);
//如果是控制PDU
	if(fixhead->DorC == 0)		 //cntrol pdu
	{
		//printk("[RLC][rlc_AM_pkt_receive_from_mac] this is a ctrl PDU\n");
		SV(countRecvCtrlPdu)++;
		rlc_ctrl_pkt_recv(amIns, pkt);
		FOUT;
	}
	else
	{
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] this is a data PDU\n");
	}
	
//如果是AMD PDU
	if(fixhead->RF == 0)   //AMD PDU
	{
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] this is an AMD PDU (SN is %d)\n", fixhead->SN);
		//head = (RLC_AMPDU_head*) fixhead;
		//head->head = head->head;
		SN = fixhead->SN;
		SN %= 1024;
		//SN %= 512;
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] the context is %s\n",(char*)(fixhead + 1));
//在接收窗口外
		if(!isInsideAmRecvWin(amIns, SN))
		{
			//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] PDU is outside the receiving window\n");
			printk("[RLC][rlc_AM_pkt_receive_from_mac] A packet has dropped\n");
			SV(countDropPacket)++;
			if(pkt != NULL)
			{
				fsm_pkt_destroy(pkt);
				pkt= NULL;
			}			
			FOUT;
		}
//在接收窗口内，但是未接收完全,或完全接收
		if(amIns->amRecvWindow[SN] != NULL)
		{
			if(amIns->amRecvWindow[SN]->pktstatus == FULL_SDU)
			{
				printk("[RLC][rlc_AM_pkt_receive_from_mac] A packet has dropped\n");
				SV(countDropPacket)++;
				if(pkt != NULL)
				{
					fsm_pkt_destroy(pkt);
					pkt = NULL;
				}				
				//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] the packet has been received completly\n");
				FOUT;
			}
			else
			{
				if(amIns->amRecvWindow[SN]->pkt != NULL)
				{
					fsm_pkt_destroy(amIns->amRecvWindow[SN]->pkt);
					amIns->amRecvWindow[SN]->pkt = NULL;
				}
				if(amIns->amRecvWindow[SN] != NULL)
				{
					fsm_mem_free(amIns->amRecvWindow[SN]);
					amIns->amRecvWindow[SN] = NULL;
					//freenum++;
					//printk("RLC FREE: %d\n", freenum);
				}				
				//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] the packet has been received but not completly\n");
			}
		}
		ambuf = (AmBuffer *)fsm_mem_alloc(sizeof(AmBuffer));
		//mallocnum++;
		//printk("RLC ALLOC: %d\n", mallocnum);
		
		ambuf->pkt = pkt;
		ambuf->pktstatus = FULL_SDU;
		ambuf->pos = 0;
		ambuf->segnum = 0;
		ambuf->SN = SN;
		list_add_tail(&ambuf->list, &amIns->amRecvBuffer.list);
		amIns->amRecvWindow[SN] = ambuf;
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] pkt has been put into buffer(SN is %d)\n", SN);
	}

//如果是AMD PDU 分段
	else
	{
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] this is an AMD PDU segment (SN is %d)\n", fixhead->SN);
		extseghead = ( RLC_AMPDU_fixhead *)(pkt->data+2);
		seghead = (RLC_AMPDU_seg_head *)fixhead;
		//eghead->head = seghead->head;
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] fixhead->SN=%d\n",fixhead->SN);
		SN = fixhead->SN;
		SN %= 1024;
		//SN %= 512;

		//是否在接收窗口外,并且已收到过其包含的字节分段
		if((!isInsideAmRecvWin(amIns, SN) )||((amIns->amRecvWindow[SN] != NULL)&&(amIns->amRecvWindow[SN]->pktstatus == FULL_SDU)))
		{
			printk("[RLC][rlc_AM_pkt_receive_from_mac] PDU segment is outside the receiving window or the segment has been received\n");
			printk("[RLC][rlc_AM_pkt_receive_from_mac] A packet has dropped\n");
			SV(countDropPacket)++;
			if(pkt != NULL)
			{
				fsm_pkt_destroy(pkt);
				pkt = NULL;
			}			
			FOUT;
		}
		/*		i = 0;
			while(amIns->amRecvWindow[SN]->segStart[i] != -1)
			{
				if((amIns->amRecvWindow[SN]->segStart[i] < extseghead->SO)\
						&&(amIns->amRecvWindow[SN]->segEnd[i] > extseghead->SO))
				{
					fsm_printf("PDU segment has been received\n");
					fsm_pkt_destroy(pkt);
					FOUT;
				}
				else ++i;
			}*/

		//如果原来没有接收到过该PDU的分段
		if(amIns->amRecvWindow[SN] == NULL)
		{
			//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] instance has not receive this PDU segment before(pos is %d)\n", extseghead->SO);
			ambuf = (AmBuffer *)fsm_mem_alloc(sizeof(AmBuffer ));
			//ambuf->pkt = pkt;
			ambuf->pos = extseghead->SO;
			if(0 == ambuf->pos)
			{
				ambuf->pkt = pkt;
			}
			else
			{
				//将原pdu分段转移到更大的分段中
				datalen = pkt->len + extseghead->SO;
				pkptr = fsm_pkt_create(datalen + fsm_skb_headroom(pkt));
				bufptr =  pkptr->data + 4 + 2*countSegInd(pkt) + ambuf->pos;
	//	fsm_printf("the count of this segment is %d\n", countSegInd(ambuf->pkt));
				fsm_mem_cpy(bufptr, pkt->data, pkt->len);
				fsm_skb_put(pkptr, datalen);
				if(pkt != NULL)
				{
					fsm_pkt_destroy(pkt);
					pkt = NULL;
				}				
				ambuf->pkt = pkptr;
			}
			ambuf->SN = SN;
			ambuf->segnum = 1;
			ambuf->datalen = 0;
			if(extseghead->LSF == 1)
			{
				ambuf->pktstatus = LAST_SEGMENT;
				ambuf->datalen = ambuf->pos + pkt->len;
				//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] this is the last segment, and the total len of original PDU is %d\n", ambuf->datalen );
			}
			else
			{
				ambuf->pktstatus = ANY_SEGMENT;
			}
			ambuf->segStart[0] = ambuf->pos;
			ambuf->segEnd[0] =ambuf->pos + pkt->len -4 -2*countSegInd(pkt);
			list_add_tail(&ambuf->list, &amIns->amRecvBuffer.list);
			//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] pkt segment has been put into buffer(SN is %d)\n", SN);
			amIns->amRecvWindow[SN] = ambuf;
		}
		//如果原来接收到过该PDU的分段
		else
		{
			//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] instance has not received this PDU segment before(pos is %d)\n", extseghead->SO);
			//丢弃重复分段
			if(extseghead->LSF == 1)
			{
				amIns->amRecvWindow[SN]->pktstatus = LAST_SEGMENT;
				amIns->amRecvWindow[SN]->datalen = extseghead->SO + pkt->len;
				//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] this is the last segment, and the total len of original PDU is %d\n", amIns->amRecvWindow[SN]->datalen );
			}
			assamble_segment(amIns->amRecvWindow[SN], pkt);
			//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] pkt segment has been assambled into buffer(SN is %d)\n", SN);
			if((amIns->amRecvWindow[SN]->datalen == amIns->amRecvWindow[SN]->pkt->len)&&\
				(amIns->amRecvWindow[SN]->segnum == 1)&&(amIns->amRecvWindow[SN]->segStart[0] == 0))
			{
				amIns->amRecvWindow[SN]->pktstatus = FULL_SDU;
			}
		}
	}

//PDU放入缓存后，判断SN的值

//大于vr_h
	if(SN >= amIns->vr_h || SN <= amIns->vr_h-AM_WINDOW_SIZE)
	{
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] SN > amIns->vr_h\n");
		amIns->vr_h = (SN + 1)%(AM_WINDOW_SIZE*2);
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] amIns->vr_h is updated to %d\n",amIns->vr_h);
	}

//如果vr_ms的所有分段都已经接收完全
	if((amIns->amRecvWindow[amIns->vr_ms] != NULL)&&(amIns->amRecvWindow[amIns->vr_ms]->pktstatus == FULL_SDU))
	{
		amIns->vr_ms = (amIns->vr_ms + 1)%1024;
		while((amIns->amRecvWindow[amIns->vr_ms] != NULL)&&(amIns->amRecvWindow[amIns->vr_ms]->pktstatus == FULL_SDU))
		{
			amIns->vr_ms = (amIns->vr_ms + 1)%1024;
		}
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] amIns->vr_ms has received completly and updated to %d\n",amIns->vr_ms);
	}
//为vr_r
	if((amIns->amRecvWindow[amIns->vr_r] != NULL)&&(amIns->amRecvWindow[amIns->vr_r]->pktstatus == FULL_SDU))
	{
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] SN == amIns->vr_r\n");
		amIns->vr_r = (amIns->vr_r+1)%1024;
		while((amIns->amRecvWindow[amIns->vr_r] != NULL)&&(amIns->amRecvWindow[amIns->vr_r]->pktstatus == FULL_SDU))
		{
			amIns->vr_r = (amIns->vr_r+1)%1024;
		}
		amIns->vr_mr = (amIns->vr_r + AM_WINDOW_SIZE) % 1024;
		//amIns->vr_mr = (amIns->vr_r + AM_WINDOW_SIZE) % 512;
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] amIns->vr_r is updated to %d, amIns->vr_mr is updated to %d\n",amIns->vr_r, amIns->vr_mr);
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] Enter rlc_AM_reassemble_deliver\n");
		rlc_AM_reassemble_deliver(amIns,&(amIns->pduLft));
	}
	
//如果定时器正在计时，且x>r,或者x在窗口外并且不等于mr
	if(amIns->reorderTimer &&((amIns->vr_x == amIns->vr_r)||(!isInsideAmRecvWin(amIns, SN)&&(amIns->vr_x != amIns->vr_mr))))
	{
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] timer is canceling\n");
		fsm_schedule_cancel(amIns->reorderTimer);
		amIns->reorderTimer = 0;
	}
	if(!amIns->reorderTimer &&(amIns->vr_h > amIns->vr_r||amIns->vr_h < amIns->vr_r - AM_WINDOW_SIZE))
	{
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] timer is scheduling\n");
		amIns->reorderTimer = fsm_schedule_self(amIns->reorderTimerValue ,amIns->reorderTimerCode);
		amIns->vr_x = amIns->vr_h;
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] amIns->vr_x is updated to %d\n",amIns->vr_x);
	}
//请求轮询
	fsm_printf("[RLC] fixhead->P = %d\n",fixhead->P);
	if(fixhead->P == 1)
	{
		fsm_printf("[RLC] amIns->statProhTimer = %d\n",amIns->statProhTimer);
		//启动状态禁止定时器
		if( !amIns->statProhTimer )
		{
			amIns->statFlag = true;
			fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] the statProhTimer start\n");
			amIns->statProhTimer = fsm_schedule_self(amIns->statProhTimerValue,amIns->statProhTimerCode);
		}
	}

	/*fsm_printf("amIns->vr_h is %d, amIns->vr_mr is %d, amIns->vr_ms is %d, amIns->vr_r is %d, amIns->vr_x is %d\n",amIns->vr_h, amIns->vr_mr, amIns->vr_ms, amIns->vr_r, amIns->vr_x);
	for(i = amIns->vr_r; i < amIns->vr_ms; i++)
	{
		if(amIns->amRecvWindow[i] == NULL)
			fsm_printf("%d is not received \n", i);
		else
			fsm_printf("%d is reveived and the status is %d\n", i, amIns->amRecvWindow[i]->pktstatus);
	}
	for( i = 0; i < amIns->amRecvWindow[SN]->segnum; i++ )
	{
		fsm_printf("segStart[%d]: %d, segEnd[%d]: %d\n",i,amIns->amRecvWindow[SN]->segStart[i],i,amIns->amRecvWindow[SN]->segEnd[i]);
	}*/

	FOUT;
}



/**************************************************************************
**Function name: rlc_UM_pkt_receive_from_mac()
**Descriptions: Receive packets from MAC layer, and put it into UM instances receive buffer
**Input: UM_RX_Instance *umRxIns, FSM_PKT * pkt
**Output: Receive packets from MAC layer, and put it into UM instances receive buffer
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**--------------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
**************************************************************************/
static void rlc_UM_pkt_receive_from_mac(UM_RX_Instance *umRxIns, FSM_PKT * pkt)
{
	RLC_UM_size5_head *head_size5;
	//RLC_UM_size10_head *head_size10;
	RLC_UM_size10_subhead *head_subhead;
	UmBuffer *umbuf;
	SequenceNumber SN;
	short  base,temp;
	u16 windows;
	//MRLC_fromMac_IciMsg *mac_ici;
	//URLC_IciMsg *icimsg;
	
	FIN(rlc_UM_pkt_receive_from_mac());
	SV_PTR_GET(rlc_sv);
	fsm_printf("\n[RLC][rlc_UM_pkt_receive_from_mac] entering rlc_UM_pkt_receive_from_mac\n");
	fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] um instance's snFiledLength is %d\n",umRxIns->snFiledLength);
	switch(umRxIns->snFiledLength)
	{
	case 5:
		head_size5 = (RLC_UM_size5_head *)pkt->data;
		SN = head_size5->SN;		
		base = umRxIns->vr_uh - UM_WINDOW_SIZE5 ;
		windows = UM_WINDOW_SIZE5 *2;
		SN %= windows;
		if((umRxIns->umRecvWindow5[SN] != NULL ) || isOrderedUm(umRxIns,SN))
		{
			fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] has been received or reordered(SN is %d)\n", SN);
			printk("[RLC][rlc_UM_pkt_receive_from_mac] A packet has dropped\n");
			SV(countDropPacket)++;
			fsm_pkt_destroy(pkt);
			pkt = NULL;
			FOUT;
		}
		//fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] the context is %s\n", (char*)(head_size5 + 1));
		break;
	case 10:
		//head_size10 = (RLC_UM_size10_head*)pkt->data;
		*(u16 *)pkt->data = ntohs(*(u16 *)pkt->data);
		head_subhead = (RLC_UM_size10_subhead *)pkt->data;
		//*(u16 *)head_subhead = ntohs(*(u16 *)head_subhead);
		fsm_octets_print(head_subhead,2);
		//head_size10->head = fsm_ntohs(head_size10->head);
		SN = head_subhead->SN;		
		base = umRxIns->vr_uh - UM_WINDOW_SIZE10 ;
		windows = UM_WINDOW_SIZE10 *2;
		SN %= windows;
		
		/*if(isOrderedUm(umRxIns,SN))
		{
			printk("[RLC][rlc_UM_pkt_receive_from_mac] has been reordered,vr_uh=%d, vr_ur=%d\n",umRxIns->vr_uh,umRxIns->vr_ur);
		}
		if((umRxIns->umRecvWindow10[SN] != NULL ))
		{
			printk("[RLC][rlc_UM_pkt_receive_from_mac] has been received");
		}*/
		//if((umRxIns->umRecvWindow10[SN] != NULL ) || isOrderedUm(umRxIns,SN))
		if((SN-umRxIns->vr_ur+1024)%1024<(umRxIns->vr_uh-umRxIns->vr_ur+1024)%1024&&(umRxIns->umRecvWindow10[SN]!=NULL) || isOrderedUm(umRxIns,SN))
		{
			//printk("[RLC][rlc_UM_pkt_receive_from_mac] has been received or reordered(SN is %d)\n", SN);
			//printk("[RLC][rlc_UM_pkt_receive_from_mac] A packet has dropped\n");
			SV(countDropPacket)++;
			fsm_pkt_destroy(pkt);
			pkt = NULL;
			FOUT;
		}
		//fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] the context is %s\n", (char*)(head_size10 + 1));
		break;
	default:
		printk("[RLC][rlc_UM_pkt_receive_from_mac] cannot distinguish the %d",umRxIns->snFiledLength);
		break;
	}
	//已被接收或已被重排序
	
	
	
	umbuf = (UmBuffer*)fsm_mem_alloc(sizeof(UmBuffer));
	umbuf->pkt = pkt;
	umbuf->SN = SN;
	list_add_tail(&umbuf->list, &umRxIns->umRecvBuffer.list);
	fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] pkt has been put into buffer(SN is %d)\n", SN);
	if(umRxIns->snFiledLength == 5)
	{
		umRxIns->umRecvWindow5[SN] = umbuf;
	}
	if(umRxIns->snFiledLength == 10)
	{
		umRxIns->umRecvWindow10[SN] = umbuf;
	}
	umRxIns->vr_uh %= windows;
	fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_uh is %d\n", umRxIns->vr_uh);
	//重排序窗口外
	if((SN>=umRxIns->vr_uh)||(SN <= umRxIns->vr_uh - windows/2))
	{
		//	fsm_printf("umbuf->SN  >=  umRxIns->vr_uh is %d\n",umRxIns->vr_uh);
		umRxIns->vr_uh = (SN + 1)%windows;
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] outside the windows: umRxIns->vr_uh is updated to %d\n", umRxIns->vr_uh);
		//temp = umRxIns->vr_ur;
		if(umRxIns->vr_ur < umRxIns->vr_uh - windows/2 )
		{
			umRxIns->vr_ur = (umRxIns->vr_uh + windows/2)%windows;
			/*temp = base % windows;
			if(temp < 0)
			{
				temp += windows;
			}
			umRxIns->vr_ur =  temp;*/
			//fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_ur is updated to %d\n", umRxIns->vr_ur);
			//rlc_UM_reassemble_deliver(umRxIns,&umRxIns->pduLft,icimsg);
		}		
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_ur is updated to %d\n", umRxIns->vr_ur);
		//rlc_UM_reassemble_deliver(umRxIns,&umRxIns->pduLft,icimsg);
		//	fsm_printf("umRxIns->vr_ur is %d\n",umRxIns->vr_ur);
		//	   fsm_pkt_send(rlc_UM_reassemble_deliver( umRxIns),STRM_TO_PDCP);
	}
	
	//接收缓存中存在vr_ur
	if(umRxIns->snFiledLength == 5)
	{
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] entering 5\n");
		if(umRxIns->umRecvWindow5[umRxIns->vr_ur] != NULL)
		{
			//fsm_printf("umbuf->SN  ==  umRxIns->vr_ur is %d\n", umRxIns->vr_ur);
			//while((umRxIns->umRecvWindow5[++umRxIns->vr_ur] != NULL)&&(umRxIns->vr_ur <= umRxIns->vr_uh ));
			umRxIns->vr_ur = (umRxIns->vr_ur + 1) % windows;
			while((umRxIns->umRecvWindow5[umRxIns->vr_ur] != NULL)&&(umRxIns->vr_ur <= umRxIns->vr_uh ))
			{
				umRxIns->vr_ur = (umRxIns->vr_ur + 1) % windows;
			}
			fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_ur is updated to %d\n", umRxIns->vr_ur);
			rlc_UM5_reassemble_deliver(umRxIns,&umRxIns->pduLft);
		}
		//rlc_UM5_reassemble_deliver(umRxIns,&umRxIns->pduLft,icimsg);
	}

	else if(umRxIns->snFiledLength == 10)
	{
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] entering 10, umRxIns->vr_ur = %d\n",umRxIns->vr_ur);
		if(umRxIns->umRecvWindow10[umRxIns->vr_ur] != NULL)
		{
			fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->umRecvWindow10[umRxIns->vr_ur] != NULL\n");
			//fsm_printf("umbuw10[++umRxIns->vr_ur] != NULL)&&(umRxIns->vr_ur <= umRxIns->vr_uh ));
			umRxIns->vr_ur = (umRxIns->vr_ur + 1) % windows;
			while((umRxIns->umRecvWindow10[umRxIns->vr_ur] != NULL)&&(umRxIns->vr_ur <= umRxIns->vr_uh ))
			{
				umRxIns->vr_ur = (umRxIns->vr_ur + 1) % windows;
			}
			fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_ur is updated to %d\n", umRxIns->vr_ur);
			//while((umRxIns->umRecvWindo
			rlc_UM_reassemble_deliver(umRxIns,&umRxIns->pduLft);
		}
		else
		{
			//printk("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->umRecvWindow10[umRxIns->vr_ur] == NULL\n");
		}
		//rlc_UM_reassemble_deliver(umRxIns,&umRxIns->pduLft,icimsg);
	}
	//如果定时器正计时，vr_x<vr_ur,或vr-x在重排序窗口外。并且x != h
	if(umRxIns->reorderTimer &&((umRxIns->vr_ux <= umRxIns->vr_ur||umRxIns->vr_ur<=umRxIns->vr_ux-windows/2)|| \
								((!isInsideUmRecvWin(umRxIns,umRxIns->vr_ux ))&&(umRxIns->vr_ux != umRxIns->vr_uh))))
	{
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] timer is canceling\n\n");
		fsm_schedule_cancel(umRxIns->reorderTimer);
		umRxIns->reorderTimer = 0;
	}
	//如果定时器停止，并且满足h>r(暂时改成!=)
	//if((!umRxIns->reorderTimer )&&(umRxIns->vr_uh != umRxIns->vr_ur))//??
	if((!umRxIns->reorderTimer )&&(umRxIns->vr_uh>umRxIns->vr_ur||umRxIns->vr_uh<umRxIns->vr_ur-windows/2))
	{
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] timer is scheduling\n");
		umRxIns->reorderTimer = fsm_schedule_self(umRxIns->reorderTimerValue, umRxIns->reorderTimerCode);
		umRxIns->vr_ux = umRxIns->vr_uh;
		umRxIns->vr_ux %= windows;
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_ux is updated to %d\n\n", umRxIns->vr_ux);
	}
	//fsm_mem_free(icimsg);
	//icimsg = NULL;
	FOUT;
}


/***********************************************************************************
**Function name: rlc_close()
**Descriptions: Realase the packets in all instances' buffers, and realase the insances, and close the module
**Input: 
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**--------------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
***********************************************************************************/
static void rlc_close()
{
	struct  AM_Instance *amIns, *tempAmIns;
	struct  UM_RX_Instance *umRxIns,*tempUrIns;
	struct  UM_TX_Instance *umTxIns, *tempUtIns;
	struct TM_RX_Instance* tmRxIns, *tempTrIns;
	struct TM_TX_Instance *tmTxIns, *tempTtIns;
	struct AmSduBuffer*amsBuffer, *tempAmsBuffer;
	struct AmBuffer *amBuffer, *tempAmBuffer;
	struct Buffer *buffer, *tempBuffer;
	struct UmBuffer *umBuffer, *tempUmBuffer;
	
	FIN(rlc_close());
	SV_PTR_GET(rlc_sv);
//释放AM实体
	if(!list_empty(&SV(vAm_ins).amList))
	{
		list_for_each_entry_safe(amIns,tempAmIns, &SV(vAm_ins).amList,amList)
		{
			list_del(&amIns->amList);
			//释放重传队列
			if(!list_empty(&amIns->amRetxBuffer.list))
			{
				list_for_each_entry_safe(amBuffer,tempAmBuffer, &amIns->amRetxBuffer.list,list)
				{
					list_del(&amBuffer->list);
					if(amBuffer->pkt != NULL)
					{
						fsm_pkt_destroy(amBuffer->pkt);
						amBuffer->pkt = NULL;
					}
					if(amBuffer != NULL)
					{
						fsm_mem_free(amBuffer);
						amBuffer = NULL;
					}					
					
					if(list_empty(&amIns->amRetxBuffer.list))
					{
						fsm_printf("[RLC][rlc_close] am:retx buffer is realeasing\n");
						break;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_close] am:retx buffer is empty\n");
			}
			//释放已发送队列
			if(!list_empty(&amIns->amTransmittedBuffer.list))
			{
				list_for_each_entry_safe(amBuffer,tempAmBuffer, &amIns->amTransmittedBuffer.list,list)
				{
					list_del(&amBuffer->list);
					if(amBuffer->pkt != NULL)
					{
						fsm_pkt_destroy(amBuffer->pkt);
						amBuffer->pkt = NULL;
					}
					if(amBuffer != NULL)
					{
						fsm_mem_free(amBuffer);
						amBuffer = NULL;
					}				
					
					if(list_empty(&amIns->amTransmittedBuffer.list))
					{
						fsm_printf("[RLC][rlc_close] am:transmited buffer is realeasing\n");
						break;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_close] am:transmited buffer is empty\n");
			}
			//释放新数据队列
			if(!list_empty(&amIns->amSduBuffer.list))
			{
				list_for_each_entry_safe(amsBuffer,tempAmsBuffer, &amIns->amSduBuffer.list,list)
				{
					list_del(&amsBuffer->list);
					if(amBuffer->pkt != NULL)
					{
						fsm_pkt_destroy(amsBuffer->pkt);
						amBuffer->pkt = NULL;
					}
					if(amsBuffer != NULL)
					{
						fsm_mem_free(amsBuffer);
						amsBuffer = NULL;
					}				
					
					if(list_empty(&amIns->amSduBuffer.list))
					{
						fsm_printf("[RLC][rlc_close] am:sdu buffer is realeasing\n");
						break;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_close] am:sdu buffer is empty\n");
			}
			//释放状态PDU队列
			if(!list_empty(&amIns->ctrlPduBuffer.list))
			{
				fsm_printf("[RLC][rlc_close] am:ctrl buffer is not empty\n");
				list_for_each_entry_safe(buffer,tempBuffer, &amIns->ctrlPduBuffer.list,list)
				{
					list_del(&buffer->list);
					if(buffer->pkt != NULL)
					{
						fsm_pkt_destroy(buffer->pkt);
						buffer->pkt = NULL;
					}
					if(buffer != NULL)
					{
						fsm_mem_free(buffer);
						buffer = NULL;
					}					
					if(list_empty(&amIns->ctrlPduBuffer.list))
					{
						fsm_printf("[RLC][rlc_close] am:ctrl buffer is realeasing\n");
						break;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_close] am:ctrl buffer is empty\n");
			}
			if(!list_empty(&amIns->amRecvBuffer.list))
			{
				list_for_each_entry_safe(amBuffer,tempAmBuffer, &amIns->amRecvBuffer.list,list)
				{
					list_del(&amBuffer->list);
					if(amBuffer->pkt != NULL)
					{
						fsm_pkt_destroy(amBuffer->pkt);
						amBuffer->pkt = NULL;
					}
					if(amBuffer != NULL)
					{
						fsm_mem_free(amBuffer);
						amBuffer = NULL;
					}								
					if(list_empty(&amIns->amRecvBuffer.list))
					{
						fsm_printf("[RLC][rlc_close] am:recv buffer is realeasing\n");
						break;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_close] am:recv buffer is empty\n");
			}
			fsm_printf("[RLC][rlc_close] am instance  has released(rbid is%d)\n", amIns->rbId);
			if(amIns != NULL)
			{
				fsm_mem_free(amIns);
				amIns = NULL;
			}		
			if(list_empty(&SV(vAm_ins).amList))
			{
				fsm_printf("[RLC][rlc_close] amInsList is empty\n");
				break;
			}
		}
	}
	//释放UM接收实体
	if(!list_empty(&SV(vUm_rx_ins).umRxList))
	{
		list_for_each_entry_safe(umRxIns,tempUrIns, &SV(vUm_rx_ins).umRxList,umRxList)
		{
			list_del(&umRxIns->umRxList);
			if(!list_empty(&umRxIns->umRecvBuffer.list))
			{
				list_for_each_entry_safe(umBuffer,tempUmBuffer, &umRxIns->umRecvBuffer.list,list)
				{
					list_del(&umBuffer->list);
					if(umBuffer->pkt != NULL)
					{
						fsm_pkt_destroy(umBuffer->pkt);
						umBuffer->pkt = NULL;
					}
					if(umBuffer != NULL)
					{
						fsm_mem_free(umBuffer);
						umBuffer = NULL;
					}				
					
					if(list_empty(&umRxIns->umRecvBuffer.list))
					{
						fsm_printf("[RLC][rlc_close] um recv:recv buffer is realeasing\n");
						break;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_close] um recv:recv buffer is empty\n");
			}
			fsm_printf("[RLC][rlc_close] um instance  has released(rbid is %d)\n",umRxIns->rbId);
			if(umRxIns != NULL)
			{
				fsm_mem_free(umRxIns);
				umRxIns = NULL;
			}			
			if(list_empty(&SV(vUm_rx_ins).umRxList))
			{
				fsm_printf("[RLC][rlc_close] umRxInsList is empty\n");
				break;
			}
		}
	}
//释放UM发送实体
	if(!list_empty(&SV(vUm_tx_ins).umTxList))
	{
		list_for_each_entry_safe(umTxIns,tempUtIns, &SV(vUm_tx_ins).umTxList,umTxList)
		{
			list_del(&umTxIns->umTxList);
			if(!list_empty(&umTxIns->umSduBuffer.list))
			{
				list_for_each_entry_safe(umBuffer,tempUmBuffer, &umTxIns->umSduBuffer.list,list)
				{
					list_del(&umBuffer->list);
					if(umBuffer->pkt != NULL)
					{
						fsm_pkt_destroy(umBuffer->pkt);
						umBuffer->pkt = NULL;
					}
					if(umBuffer != NULL)
					{
						fsm_mem_free(umBuffer);
						umBuffer = NULL;
					}					
					
					if(list_empty(&umTxIns->umSduBuffer.list))
					{
						fsm_printf("[RLC][rlc_close] um tran:sdu buffer is realeasing\n");
						break;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_close] um tran:sdu buffer is empty\n");
			}
			fsm_printf("[RLC][rlc_close] um instance  has released(rbid is %d)\n",umTxIns->rbid);
			if(umTxIns != NULL)
			{
				fsm_mem_free(umTxIns);
				umTxIns = NULL;
			}		
			if(list_empty(&SV(vUm_tx_ins).umTxList))
			{
				fsm_printf("[RLC][rlc_close] umTxInsList is empty\n");
				break;
			}
		}
	}
//释放TM发送实体
	if(!list_empty(&SV(vTm_tx_ins).tmTxList))
	{
		list_for_each_entry_safe(tmTxIns,tempTtIns, &SV(vTm_tx_ins).tmTxList,tmTxList)
		{
			list_del(&tmTxIns->tmTxList);
			if(!list_empty(&tmTxIns->tmSduBuffer.list))
			{
				list_for_each_entry_safe(buffer,tempBuffer, &tmTxIns->tmSduBuffer.list,list)
				{
					list_del(&buffer->list);
					if(buffer->pkt != NULL)
					{
						fsm_pkt_destroy(buffer->pkt);
						buffer->pkt = NULL;
					}
					if(buffer != NULL)
					{
						fsm_mem_free(buffer);
						buffer = NULL;
					}					
					
					if(list_empty(&tmTxIns->tmSduBuffer.list))
					{
						fsm_printf("[RLC][rlc_close] tm:sdu buffer is empty\n");
					    break;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_close] tm:sdu buffer is empty\n");
			}
			if( tmTxIns->pbCh == 0)
			{
				fsm_printf("[RLC][rlc_close] CCCH recv has released\n");
			}
			else if(tmTxIns->pbCh == 1)
			{
				fsm_printf("[RLC][rlc_close] PCCH has released\n");
			}
			else
			{
				fsm_printf("[RLC][rlc_close] BCCH has released\n");
			}
			if(tmTxIns != NULL)
			{
				fsm_mem_free(tmTxIns);
				tmTxIns = NULL;
			}		
			if(list_empty(&SV(vTm_tx_ins).tmTxList))
			{
				fsm_printf("[RLC][rlc_close] tmTxInsList is empty\n");
				break;
			}
		}
	}
//释放TM接收实体
	if(!list_empty(&SV(vTm_rx_ins).tmRxList))
	{
		list_for_each_entry_safe(tmRxIns,tempTrIns, &SV(vTm_rx_ins).tmRxList,tmRxList)
		{
			list_del(&tmRxIns->tmRxList);
			if( tmRxIns->pbCh == 0)
			{
				fsm_printf("[RLC][rlc_close] CCCH tran has released\n");
			}
			else if(tmRxIns->pbCh == 1)
			{
				fsm_printf("[RLC][rlc_close] PCCH has released\n");
			}
			else
			{
				fsm_printf("[RLC][rlc_close] BCCH has released\n");
			}
			if(tmRxIns != NULL)
			{
				fsm_mem_free(tmRxIns);
				tmRxIns = NULL;
			}			
			if(list_empty(&SV(vTm_rx_ins).tmRxList))
			{
				fsm_printf("[RLC][rlc_close] tmRxInsList is empty\n");
				break;
			}
		}
	}
	fsm_printf("[RLC][rlc_close] rlc has closed \n");
	FOUT;
}


/************************************************************************
**Function name: rlc_ioctrl_handler()
**Descriptions: Handle the ioctl commands
**Input: 
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
static void rlc_ioctrl_handler(void)
{
	FIN(rlc_ioctrl_handler());
	SV_PTR_GET(rlc_sv);
	switch(fsm_ev_ioctrl_cmd())
	{
	case CRLC_CONFIG_REQ:
		fsm_printf("\n[RLC][rlc_ioctrl_handler] CRLC_CONFIG_REQ has received\n");
		PCRLC_CONFIG_REQ();
		FOUT;
		break;
	case CRLC_BULID_REQ:
		fsm_printf("\n[RLC][rlc_ioctrl_handler] CRLC_BULID_REQ has received\n");
		PCRLC_BULID_REQ();
		FOUT;
		break;
	case CRLC_DEACT_REQ:
		fsm_printf(" \n[RLC][rlc_ioctrl_handler] CRLC_DEACT_REQ has received\n");
		PCRLC_DEACT_REQ();
		FOUT;
		break;
	case CRLC_SUSPEND_REQ:
		fsm_printf("\n[RLC][rlc_ioctrl_handler] CRLC_SUSPEND_REQ has received\n");
		PCRLC_SUSPEND_REQ();
		FOUT;
		break;
	case CRLC_RESUME_REQ:
		fsm_printf("\n[RLC][rlc_ioctrl_handler] CRLC_RESUME_REQ has received\n");
		PCRLC_RESUME_REQ();
		FOUT;
		break;
	case  PRLC_DISC_REQ:
		PPRLC_DISC_REQ();
		FOUT;
		break;
	case  MRLC_TRANSOP_IND :
		fsm_printf("\n[RLC][rlc_ioctrl_handler] MRLC_TRANSOP_IND  has received\n");
		PMRLC_TRANSOP_IND();
		FOUT;
		break;
	case PRINTRLC:
		printk("\ncountRecvFromUpper =%d\n",SV(countRecvFromUpper));
		printk("countSentToLower =%d\n",SV(countSentToLower));
		printk("countSendToUpper =%d\n",SV(countSendToUpper));
		printk("countRecvFromLower =%d\n",SV(countRecvFromLower));
		printk("countSendCtrlPdu =%d\n",SV(countSendCtrlPdu));
		printk("countRecvCtrlPdu =%d\n",SV(countRecvCtrlPdu));
		printk("countDropPacket =%d\n\n",SV(countDropPacket));
		printk("maxQueueNum =%d\n\n",SV(max_queue_num));
		
		
		break;
	default:
		fsm_printf("[RLC][rlc_ioctrl_handler] unidentified ioctrl command!\n");
		break;
	}
	FOUT;
}


/************************************************************************
**Function name: findInsByRbid()
**Descriptions: Find instance according to rbid
**Input: u8 rbid
**Output: void **up or void **down , and RlcMode *mode
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
void  findInsByRbid(u8 rbid,void **up, void **down , RlcMode *mode)
{
	struct list_head* pos;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	TM_RX_Instance *tmRxIns;
	TM_TX_Instance *tmTxIns;
	
	FIN(findInsByRbid());
	SV_PTR_GET(rlc_sv);
	*up = *down = NULL;
	*mode = 0;
	if (!list_empty(&SV(vTm_tx_ins).tmTxList))
	{
		list_for_each(pos,&SV(vTm_tx_ins).tmTxList)
		{
			tmTxIns= list_entry(pos,struct TM_TX_Instance,tmTxList);
			if(tmTxIns->rbId == rbid)
			{
				*up = (void*)tmTxIns;
				*mode = TM_MODE;
			}
		}
	}
	if (!list_empty(&SV(vTm_rx_ins).tmRxList))
	{
		list_for_each(pos,&SV(vTm_rx_ins).tmRxList)
		{
			tmRxIns = list_entry(pos,struct  TM_RX_Instance,tmRxList);
			if(tmRxIns->rbId == rbid)
			{
				*down = (void*)tmRxIns;
				*mode = TM_MODE;
			}
		}
	}
	if(*mode == TM_MODE)
		FOUT;
	if (!list_empty(&SV(vUm_tx_ins).umTxList))
	{
		list_for_each(pos,&SV(vUm_tx_ins).umTxList)
		{
			umTxIns= list_entry(pos,struct UM_TX_Instance,umTxList);
			if(umTxIns->rbid == rbid)
			{
				*up =  (void*)umTxIns;
				*mode = UM_MODE;
			}
		}
	}
	if(!list_empty(&SV(vUm_rx_ins).umRxList))
	{
		list_for_each(pos,&SV(vUm_rx_ins).umRxList)
		{
			umRxIns = list_entry(pos,struct  UM_RX_Instance,umRxList);
			if(umRxIns->rbId == rbid)
			{
				*down = (void*)umRxIns;
				*mode = UM_MODE;
			}
		}
	}
	if(*mode == UM_MODE)
		FOUT;
	if (!list_empty(&SV(vAm_ins).amList))
	{
		list_for_each(pos,&SV(vAm_ins).amList)
		{
			amIns = list_entry(pos,struct AM_Instance,amList);
			if(amIns->rbId == rbid)
			{
				*mode = AM_MODE;
				*up = *down =  (void*)amIns;
			}
		}
	}
	FOUT;
}


/************************************************************************
**Function name: findInsBylcid()
**Descriptions: Find instance according to lcid
**Input: u8 lcid
**Output: void **up or void**down, and RlcMode *mode
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
void  findInsBylcid(u8 lcid, void **up, void**down, RlcMode *mode)
{
	struct list_head* pos;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	TM_RX_Instance *tmRxIns;
	TM_TX_Instance *tmTxIns;
	
	FIN(findInsBylcid());
	SV_PTR_GET(rlc_sv);
	*up = *down = NULL;
	*mode = 0;
	if (!list_empty(&SV(vTm_tx_ins).tmTxList))
	{
		list_for_each(pos,&SV(vTm_tx_ins).tmTxList)
		{
			tmTxIns= list_entry(pos,struct TM_TX_Instance,tmTxList);
			if(tmTxIns->lcId == lcid)
			{
				*mode = TM_MODE;
				*up =  (void*)tmTxIns;
				//fsm_printf("tm instance\n");
			}
		}
	}
	if (!list_empty(&SV(vTm_rx_ins).tmRxList))
	{
		list_for_each(pos,&SV(vTm_rx_ins).tmRxList)
		{
			tmRxIns = list_entry(pos,struct  TM_RX_Instance,tmRxList);
			if(tmRxIns->lcId == lcid)
			{
				*mode = TM_MODE;
				*down =  (void*)tmRxIns;
				//fsm_printf("tm instance\n");
			}
		}
	}
	if(*mode == TM_MODE)
		FOUT;
	if (!list_empty(&SV(vUm_tx_ins).umTxList))
	{
		list_for_each(pos,&SV(vUm_tx_ins).umTxList)
		{
			umTxIns= list_entry(pos,struct UM_TX_Instance,umTxList);
			if(umTxIns->lcid == lcid)
			{
				*mode = UM_MODE;
				*up =  (void*)umTxIns;
			}
		}
	}
	if(!list_empty(&SV(vUm_rx_ins).umRxList))
	{
		list_for_each(pos,&SV(vUm_rx_ins).umRxList)
		{
			umRxIns = list_entry(pos,struct  UM_RX_Instance,umRxList);
			if(umRxIns->lcId == lcid)
			{
				*mode = UM_MODE;
				*down =  (void*)umRxIns;
			}
		}
	}
	if(*mode == UM_MODE)
		FOUT;
	if (!list_empty(&SV(vAm_ins).amList))
	{
		list_for_each(pos,&SV(vAm_ins).amList)
		{
			amIns = list_entry(pos,struct AM_Instance,amList);
			if(amIns->lcId == lcid)
			{
				*mode = AM_MODE;
				*up = *down =  (void*)amIns;
				//      fsm_printf("am instance(lcid is %d)\n",lcid);
			}
		}
	}
	FOUT;
}

/************************************************************************
**Function name: findInsByCode()
**Descriptions: Find timer according to code
**Input: int code
**Output: RlcTimerType * timer
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
void * findInsByCode(long code, RlcTimerType * timer)
{
	struct list_head* pos;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	
	FIN(findInsByCode());
	SV_PTR_GET(rlc_sv);
	if( SV(bufferReqTimerCode) == code )
	{
		*timer = BUFFER_REQ;
		return NULL;
	}
	if(!list_empty(&SV(vUm_rx_ins).umRxList))
	{
		list_for_each(pos,&SV(vUm_rx_ins).umRxList)
		{
			umRxIns = list_entry(pos,struct  UM_RX_Instance,umRxList);
			if(umRxIns->reorderTimerCode == code)
			{
				*timer = UM_REORDER;
				return (void*)umRxIns;
			}
		}
	}
	if (!list_empty(&SV(vAm_ins).amList))
	{
		list_for_each(pos,&SV(vAm_ins).amList)
		{
			amIns = list_entry(pos,struct AM_Instance,amList);
			if(amIns->pollRetxTimerCode == code)
			{
				*timer = POLL_RETX;
				return (void*)amIns;
			}
			if(amIns->reorderTimerCode == code)
			{
				*timer = AM_REORDER;
				return (void*)amIns;
			}
			if(amIns->statProhTimerCode == code)
			{
				*timer = STAT_PRO;
				return (void*)amIns;
			}
		}
	}
	else
	{
		return NULL;
	}
}

/************************************************************************
**Function name: isInsideTranWin()
**Descriptions: Judge the PDU of SN is inside the Transmit Window or not
**Input: struct AM_Instance *amIns, SequenceNumber SN
**Output: true : the PDU is in the TranWin; false : the PDU is not in the TranWin
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
bool isInsideTranWin(struct AM_Instance *amIns, SequenceNumber SN)
{
	//printk("[RLC][isInsideTranWin] vt_a=%d, vt_ms=%d, SN=%d\n",amIns->vt_a, amIns->vt_ms, SN);
	//return((amIns->vt_a <= SN)&&(SN < amIns->vt_ms));
	return (SN-amIns->vt_a+1024)%1024 < (amIns->vt_ms-amIns->vt_a+1024)%1024;
}


/************************************************************************
**Function name: isInsideAmRecvWin()
**Descriptions: Judge the PDU of SN is inside the Receive Window of AM mode or not
**Input: struct AM_Instance * amIns, SequenceNumber SN
**Output: true : the PDU is in the RecvWin; false : the PDU is not in the RecvWin
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
bool isInsideAmRecvWin(struct AM_Instance * amIns, SequenceNumber SN)
{
	/*SequenceNumber upper,upperMode;
	upper = amIns->vr_r + 512;
	upperMode = upper % 1024;
	//fsm_printf("[RLC][isInsideAmRecvWin] the upper is %d, upperMode is %d\n",upper, upperMode);
	return(((amIns->vr_r <= SN)&&(SN < upper))||(((upperMode - 512) < SN )&&(SN < upperMode)));*/
	printk("[RLC][isInsideAmRecvWin] vr_r=%d, vr_mr=%d, SN=%d, mr-r = %d\n",amIns->vr_r, amIns->vr_mr, SN,amIns->vr_mr-amIns->vr_r);
	return (SN-amIns->vr_r+1024)%1024 < (amIns->vr_mr-amIns->vr_r+1024)%1024;
}

/************************************************************************
**Function name:isInsideUmRecvWin()
**Descriptions: Judge the PDU of SN is inside the Receive Window of UM mode or not
**Input: struct UM_RX_Instance* umRxIns, SequenceNumber SN
**Output: true: the PDU is in the UMRecvWin; false : the PDU is not in the UMRecvWin
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
bool isInsideUmRecvWin(struct UM_RX_Instance* umRxIns, SequenceNumber SN)
{
	short base,baseMode,temp;
	
	if(umRxIns->snFiledLength == 5)
	{
		/*temp = base = umRxIns->vr_uh - 16;
		if(base < 0)
		{
			temp = -base;
		}
		baseMode = temp % 32;
		fsm_printf("[RLC][isInsideUmRecvWin] baseMode is %d\n",baseMode);
		temp = SN;
		return (((base <= temp) && (temp  < umRxIns->vr_uh))||((baseMode <= temp)&&(temp < baseMode + 16)));*/
		return (SN-(umRxIns->vr_uh-16)+32)%32 < 16;
	}
	else
	{
		/*base = umRxIns->vr_uh - 512;
		if(base < 0)
		{
			temp = -base;
		}
		baseMode = base % 1024;
		temp = SN;
		return (((base <= temp ) && (temp  < umRxIns->vr_uh))||((baseMode <= temp )&&(SN < baseMode + 512)));*/
		return (SN-(umRxIns->vr_uh-512)+1024)%1024 < 512;
	}
}


/************************************************************************
**Function name: isOrderedUm()
**Descriptions: Judge the PDU of SN is ordered in UM mode or not
**Input: struct UM_RX_Instance* umRxIns, SequenceNumber SN
**Output: true: the PDU is ordered; false : the PDU is not ordered
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
bool isOrderedUm(struct UM_RX_Instance* umRxIns, SequenceNumber SN)
{
	/*short base;
	
	if(umRxIns->snFiledLength == 5)
	{
		base = umRxIns->vr_uh - 16;
	}
	else
	{
		base = umRxIns->vr_uh - 512;
	}
	return ((base <= SN )&&(SN < umRxIns->vr_uh));*/
	//printk("[RLC][isOrderedUm] SN = %d, vr_uh = %d, vr_ur = %d\n",SN,umRxIns->vr_uh,umRxIns->vr_ur);
	if(umRxIns->snFiledLength == 5)
	{
		//return ((umRxIns->vr_uh <= SN + 16 )&&(SN < umRxIns->vr_ur));
		return (SN-(umRxIns->vr_uh-16)+32)%32 < (umRxIns->vr_ur-(umRxIns->vr_uh-16)+32)%32;
	}
	else
	{
		//return ((umRxIns->vr_uh <= SN + 512 )&&(SN < umRxIns->vr_ur));
		return (SN-(umRxIns->vr_uh-512)+1024)%1024 < (umRxIns->vr_ur-(umRxIns->vr_uh-512)+1024)%1024;
	}
}


/************************************************************************
**Function name: countSegInd()
**Descriptions: Count the number of indicators(E + LI) of SDUs in a PDU segment
**Input: FSM_PKT *pkt
**Output: the number of indicators(E + LI) of SDUs
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
u16 countSegInd(FSM_PKT *pkt)
{
	u16 count = 0;
	RLC_AMPDU_fixhead* head = (RLC_AMPDU_fixhead*)(pkt->data);
	RLC_AMDPDU_seg_ind *ind = (RLC_AMDPDU_seg_ind*)(head + 2);
	
	if(1 == head->E )
	{
		count++;
		while(1 == (ind++)->E)
		{
			count++;
		}
	}
	return count;
}


/************************************************************************
**Function name: assamble_segment()
**Descriptions: assamble the segments received into buffer
**Input: AmBuffer *ambuf, FSM_PKT *pkt
**Output: assamble the segments received into buffer
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
void assamble_segment(AmBuffer *ambuf, FSM_PKT *pkt)
{
	FSM_PKT *pkptr;
	struct RLC_AMPDU_fixhead *fixhead;
	fixhead=(RLC_AMPDU_fixhead* )pkt->data;
	RLC_AMPDU_seg_exthead* exthead = (RLC_AMPDU_seg_exthead* )(pkt->data+sizeof(RLC_AMPDU_fixhead));
	// fsm_printf("the count of this segment is %d\n", countSegInd(pkt));
	u16 headLen = 4 + 2*countSegInd(ambuf->pkt);
	u32 posStart  =  exthead->SO;
	u32 posEnd = posStart + pkt->len - headLen;
	char *pospkt = (char*)pkt->data;
	char *posbuf = (char*)ambuf->pkt->data;
	int i, j,datalen = 0;
	u16 flag = 0;
	
	
	pospkt = pospkt + headLen;
	//fsm_printf("[RLC][assamble_segment] the packet data is %s\n", pospkt);
	posbuf = posbuf + headLen;
	//fsm_printf("[RLC][assamble_segment] the buffer data is %s\n", posbuf);

	for(i = 0; i < ambuf->segnum; i++)
	{
		//接收到的分段重复，丢弃
		if((ambuf->segStart[i] <= posStart) &&(posEnd <= ambuf->segEnd[i] ))
		{
			//fsm_printf("[RLC][assamble_segment] this segment has been received\n");
			fsm_pkt_destroy(pkt);
			pkt = NULL;
			FOUT;
		}
	}
	//fsm_printf("[RLC][assamble_segment] ambuf->datalen is %d\n",ambuf->datalen);
	//fsm_printf("[RLC][assamble_segment] ambuf->segEnd[ambuf->segnum - 1] is %d\n",ambuf->segEnd[ambuf->segnum - 1]);
	//fsm_printf("[RLC][assamble_segment] posEnd is %d\n",posEnd);
	//if((ambuf->datalen)||(ambuf->segEnd[ambuf->segnum - 1] > posEnd))
	if(ambuf->segEnd[ambuf->segnum - 1] > posEnd)
	{
		//接收分段不重复，放入buffer中
		posbuf = posbuf + posStart;
		fsm_mem_cpy(posbuf, pospkt, pkt->len);
		fsm_pkt_destroy(pkt);
		pkt = NULL;
		//更新buffer中的分段变量(具体情况见重复分段图)

		if((ambuf->segStart[0] > 0)&&(posEnd < ambuf->segStart[1]))
		{
			//情况1:第一个分段之前没接收到，接收的分段在seg[0]之前
			if((ambuf->segStart[0] > posStart)&&(ambuf->segStart[0] > posEnd))
			{
				//fsm_printf("[RLC][assamble_segment] condition 1\n");
				for(i = ambuf->segnum - 1; i >= 0; --i)
				{
				ambuf->segEnd[i+1] = ambuf->segEnd[i];
				ambuf->segStart[i+1] = ambuf->segStart[i];
				}
				ambuf->segStart[0] = posStart;
				ambuf->segEnd[0] = posEnd;
				ambuf->segnum++;
				FOUT;
			}
			//情况2:
			if((posStart < ambuf->segStart[0] )&&(ambuf->segStart[0] < posEnd)&&(posEnd <= ambuf->segEnd[0]))
			{
				//fsm_printf("[RLC][assamble_segment] condition 2\n");
				ambuf->segStart[0] = posStart;
				FOUT;
			}
			//情况3:
			if(( posStart <= ambuf->segStart[0])&&(ambuf->segStart[0] < posEnd)&&\
				(posStart < ambuf->segEnd[0])&&(ambuf->segEnd[0] < posEnd)&&(posEnd < ambuf->segStart[1]))
			{
				//fsm_printf("[RLC][assamble_segment] condition 3\n");
				ambuf->segStart[0] = posStart;
				ambuf->segEnd[0] = posEnd;
				FOUT;
			}
		}
		//收到的分段起始位置在中间
		for(i = 0; i < ambuf->segnum - 1; i++)
		{
			//情况4;
			if((ambuf->segEnd[i] < posStart)&&(posStart < ambuf->segStart[i + 1])&&\
				(ambuf->segEnd[i] < posEnd)&&(posEnd < ambuf->segStart[i + 1]))
			{
				//fsm_printf("[RLC][assamble_segment] condition 4\n");
				for(j = ambuf->segnum - 1; j > i; --j)
				{
				ambuf->segEnd[j+1] = ambuf->segEnd[j];
				ambuf->segStart[j+1] = ambuf->segStart[j];
				}
				ambuf->segStart[j+1] = posStart;
				ambuf->segEnd[j+1] = posEnd;
				ambuf->segnum++;
				FOUT;
			}
			//接收分段起始在已收分段里面
			else if((ambuf->segStart[i] <=  posStart) &&(posStart <= ambuf->segEnd[i]))
			{
				for(j = i + 1; j < ambuf->segnum; j++)
				{
					//情况5:
					if((ambuf->segStart[j] <= posEnd )&&(posEnd <= ambuf->segEnd[j]))
					{
						//fsm_printf("[RLC][assamble_segment] condition 5\n");
						ambuf->segEnd[i] = ambuf->segEnd[j];
						i++;j++;
						for(; i < ambuf->segnum; j++, i++)
						{
							if(j < ambuf->segnum)
							{
								ambuf->segEnd[i] = ambuf->segEnd[j];
								ambuf->segStart[i] = ambuf->segStart[j];
							}
							else
							{
								ambuf->segEnd[i] = -1;
								ambuf->segStart[i] = -1;
							}
						}
						ambuf->segnum  -= j - i;
						FOUT;
					}
					//情况6:
					else if((ambuf->segEnd[j - 1] <= posEnd)&&(posEnd < ambuf->segStart[j]))
					{
						//fsm_printf("[RLC][assamble_segment] condition 6\n");
						ambuf->segEnd[i] = posEnd;
						i++;
						for(; i < ambuf->segnum; j++, i++)
						{
							if(j < ambuf->segnum)
							{
								ambuf->segEnd[i] = ambuf->segEnd[j];
								ambuf->segStart[i] = ambuf->segStart[j];
							}
							else
							{
								ambuf->segEnd[i] = -1;
								ambuf->segStart[i] = -1;
							}
						}
						ambuf->segnum  -= j - i;
						FOUT;
					}
				}
			}

			//接收分段起始在已收分段之间
			else if((ambuf->segEnd[i] <=  posStart) &&(posStart <= ambuf->segStart[i + 1]))
			{
				for(j = i + 1; j < ambuf->segnum; j++)
				{
					//情况7:
					if((ambuf->segStart[j] <= posEnd )&&(posEnd <= ambuf->segEnd[j]))
					{
						//fsm_printf("[RLC][assamble_segment] condition 7\n");
						ambuf->segStart[++i] = posStart;
						ambuf->segEnd[i] = ambuf->segEnd[j];
						i++;j++;
						for(; i < ambuf->segnum; j++, i++)
						{
							if(j < ambuf->segnum)
							{
								ambuf->segEnd[i] = ambuf->segEnd[j];
								ambuf->segStart[i] = ambuf->segStart[j];
							}
							else
							{
								ambuf->segEnd[i] = -1;
								ambuf->segStart[i] = -1;
							}
						}
						ambuf->segnum  -= j - i;
						FOUT;
					}
					//情况8:
					else if((ambuf->segEnd[j - 1] <= posEnd)&&(posEnd < ambuf->segStart[j]))
					{
						//fsm_printf("[RLC][assamble_segment] condition 8\n");
						ambuf->segStart[++i] = posStart;
						ambuf->segEnd[i] = posEnd;
						i++;
						for(; i < ambuf->segnum; j++, i++)
						{
							if(j < ambuf->segnum)
							{
								ambuf->segEnd[i] = ambuf->segEnd[j];
								ambuf->segStart[i] = ambuf->segStart[j];
							}
							else
							{
								ambuf->segEnd[i] = -1;
								ambuf->segStart[i] = -1;
							}
						}
						ambuf->segnum  -= j - i;
						FOUT;
					}
				}
				if(flag)
				{

				}
			}
		}
	}
	//还没接收到最后一个分段
	else
	{
		//fsm_printf("[RLC][assamble_segment] have not received the last segment\n");
		//接收到的报文是最后一个分段
		if((exthead->LSF == 1)||(ambuf->segEnd[ambuf->segnum - 1] < posEnd))
		{
			//将原pdu分段转移到更大的分段中
			datalen = pkt->len + exthead->SO;
			pkptr = fsm_pkt_create(datalen + fsm_skb_headroom(pkt));
			fsm_mem_cpy(pkptr->data, ambuf->pkt->data, ambuf->pkt->len);
			fsm_skb_put(pkptr, datalen);
			fsm_pkt_destroy(ambuf->pkt);
			ambuf->pkt = pkptr;
			//将新接收到的pdu复制到新的pdu中
			posbuf = (char*)pkptr->data;
			posbuf = posbuf + headLen + posStart;
			fsm_mem_cpy(posbuf, pospkt, pkt->len);
			fsm_pkt_destroy(pkt);
			pkt = NULL;
		}
		if(exthead->LSF == 1)
		ambuf->datalen = datalen;
		//最后一个分段与之前的不重复
		if(ambuf->segEnd[ambuf->segnum -1] < posStart)
		{
			ambuf->segEnd[ambuf->segnum] = posEnd;
			ambuf->segStart[ambuf->segnum]  = posStart;
			ambuf->segnum++;
			FOUT;
		}
		else
		{
			for(i = ambuf->segnum - 1; i >0 ; --i)
			{
				//情况10:
				if((ambuf->segStart[i] <= posStart)&&(posStart <= ambuf->segEnd[i]))
				{
					//fsm_printf("[RLC][assamble_segment] condition 10\n");
					ambuf->segEnd[i] = posEnd;
					ambuf->segnum = i + 1;
					FOUT;
				}
				//情况11:
				else if((ambuf->segEnd[i - 1] < posStart)&&(posStart <= ambuf->segStart[i]))
				{
					//fsm_printf("[RLC][assamble_segment] condition 11\n");
					ambuf->segStart[i] = posStart;
					ambuf->segEnd[i] = posEnd;
					ambuf->segnum = i + 1;
					FOUT;
				}
				else
				{
					ambuf->segStart[i] = ambuf->segEnd[i] = -1;
				}
			}
			if(posStart > ambuf->segStart[0])
			{
				ambuf->segEnd[0] = posEnd;
				ambuf->segnum = 1;
				FOUT;
			}
            else
            {
	            ambuf->segStart[0]  = posStart;
	            ambuf->segEnd[0] = posEnd;
	            ambuf->segnum = 1;
	            FOUT;
			}
		}
	}
}




