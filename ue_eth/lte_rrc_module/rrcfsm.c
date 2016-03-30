/*******************************************************************************
 ** Filename:rrcfsm.c - 
 ** Copyright: uestc.
 ** Descriptions: The FSM of RRC Module
 ** v1.0, 2014/02/24, zhenglingjie written
 ** ----------------------------------------------------------
 ** modification history:
 ** Modified by:Liu Yingtao 
 ** Modified date:2014/09/24
 ** Descriptions:
 ** 
 ******************************************************************************/

#include <linux/if_ether.h>
#include "rrcfsm.h"
#include "../lte_system.h"
#include "../pkfmt.h"
#include "rrc_utils.h"
#include "rrc_conn_establish.h"
#include "rrc_conn_reconfig.h"
#include "rrc_paging_SI.h"

//defined in rrc_module.c
extern void send_to_user(char *info, int size_info);

struct UE_Identity ue_Identity =
{
    .type = 1,
    .choice.s_TMSI.mmec = 12,
    .choice.s_TMSI.m_TMSI = 123456,
};

/* -----------------------
   Declare global variables
   ----------------------- */
struct Paging *paging;
struct MasterInformationBlock *mib;
struct SystemInformation *si;
struct SystemInformationBlockType1 *sib1;
//struct RRCConnectionReestablishment *rrcConnectionReestablishment;
//struct RRCConnectionReestablishmentReject *rrcConnectionReestablishmentReject;
struct RRCConnectionReject *rrcConnectionReject;
struct RRCConnectionSetup *rrcConnectionSetup;
struct RRCConnectionReconfiguration *rrcConnectionReconfiguration;
struct RRCConnectionRelease *rrcConnectionRelease;

u64 imsi;  
u16 crnti;   
u16 cellId;

bool m_receivedmib = false;
bool m_receivedSib2 = false;
bool m_receivedsib1 = false;

//struct FreqPriority *ueFreqPriorityList = NULL;
struct CellReselectionPriority *ueCellReselectionPriorityList = NULL; //zhouyang 2014/10/1



void rrc_main(void)
{
	FSM_ENTER(rrc_main);
	FSM_BLOCK_SWITCH
	{
		FSM_STATE_FORCED(ST_INIT, "ST_INIT", transit_to_idle_connecting(), st_init_exit())
		{
			FSM_TRANSIT_FORCE(ST_IDLE_CONNECTING, , "default", "", "ST_INIT -> ST_IDLE_CONNECTING");
		}
		FSM_STATE_UNFORCED(ST_IDLE_CELL_SEL, "ST_IDLE_CELL_SEL", st_cell_sel_enter(), st_cell_sel_exit())
		{
			FSM_COND_TEST_IN("ST_IDLE_CELL_SEL")
				FSM_TEST_COND(EVT_BEGIN_RECV_SYSINFO)
				FSM_COND_TEST_OUT("ST_IDLE_CELL_SEL")
				FSM_TRANSIT_SWITCH
				{
					FSM_CASE_TRANSIT(0, ST_IDLE_WAIT_SYSINFO, start_recv_sysInfo(), "ST_IDLE_CELL_SEL -> ST_IDLE_WAIT_SYSINFO")
					FSM_CASE_DEFAULT(ST_IDLE_CELL_SEL, st_cell_sel_default(), "ST_IDLE_CELL_SEL -> ST_IDLE_CELL_SEL")
				}
		}
		FSM_STATE_UNFORCED(ST_IDLE_WAIT_SYSINFO, "ST_IDLE_WAIT_SYSINFO", st_wait_sysInfo_enter(),st_wait_sysInfo_exit())
		{
			FSM_COND_TEST_IN("ST_IDLE_WAIT_SYSINFO")
				FSM_TEST_COND(EVT_END_RECV_SYSINFO)
				FSM_COND_TEST_OUT("ST_IDLE_WAIT_SYSINFO")
				FSM_TRANSIT_SWITCH
				{
					FSM_CASE_TRANSIT(0, ST_IDLE_NORMALLY, transit_to_idle(), "ST_IDLE_WAIT_SYSINFO -> ST_IDLE_NORMALLY")
					FSM_CASE_DEFAULT(ST_IDLE_WAIT_SYSINFO, st_wait_sysInfo_default(), "ST_IDLE_WAIT_SYSINFO -> ST_IDLE_WAIT_SYSINFO")   //transit to idle state by default.
				}
		}
		FSM_STATE_UNFORCED(ST_IDLE_NORMALLY, "ST_IDLE_NORMALLY", st_idle_normal_enter(),st_idle_normal_exit())
		{
			FSM_COND_TEST_IN("ST_IDLE_NORMALLY")
			FSM_TEST_COND(Evt_RcvConnReqNAS)
			FSM_COND_TEST_OUT("ST_IDLE_NORMALLY")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE_CONNECTING, transit_to_idle_connecting(), "ST_IDLE_NORMALLY -> ST_IDLE_CONNECTING")
				FSM_CASE_DEFAULT(ST_IDLE_NORMALLY, st_idle_normal_default(), "ST_IDLE_NORMALLY -> ST_IDLE_NORMALLY")   //transit to idle state by default.
			}
		}
		FSM_STATE_UNFORCED(ST_IDLE_CONNECTING, "ST_IDLE_CONNECTING", st_idle_connecting_enter(),st_idle_connecting_exit())
		{
			FSM_COND_TEST_IN("ST_IDLE_CONNECTING")
			FSM_TEST_COND(Evt_TimeOut_T300)
			FSM_TEST_COND(Evt_RcvRRCConnReject)
			FSM_TEST_COND(Evt_RcvRRCConnSetup)
			FSM_COND_TEST_OUT("ST_IDLE_CONNECTING")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE_NORMALLY, t300_timeout(), "ST_IDLE_CONNECTING -> ST_IDLE_NORMALLY")
				FSM_CASE_TRANSIT(1, ST_IDLE_NORMALLY, rcv_rrc_conn_reject(), "ST_IDLE_CONNECTING -> ST_IDLE_NORMALLY")
				FSM_CASE_TRANSIT(2, ST_CONNECTED_NORMALLY, rcv_rrc_conn_setup(), "ST_IDLE_CONNECTING -> ST_CONNECTED_NORMALLY")
				FSM_CASE_DEFAULT(ST_IDLE_CONNECTING, st_idle_connecting_default(), "ST_IDLE_CONNECTING -> ST_IDLE_CONNECTING")   //transit to idle state by default.
			}
		}
		FSM_STATE_UNFORCED(ST_CONNECTED_NORMALLY, "ST_CONNECTED_NORMALLY", st_connect_normal_enter(),st_connect_normal_exit())
		{
			FSM_COND_TEST_IN("ST_CONNECTED_NORMALLY")
			FSM_TEST_COND(Evt_RcvRRCConnReconfig)
			FSM_TEST_COND(Evt_RcvRRCConnRelease)
			FSM_COND_TEST_OUT("ST_CONNECTED_NORMALLY")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_CONNECTED_RECONF, rcv_rrc_conn_reconf(), "ST_CONNECTED_NORMALLY -> ST_CONNECTED_RECONF")
				FSM_CASE_TRANSIT(1, ST_IDLE_NORMALLY, rcv_rrc_conn_release(), "ST_CONNECTED_NORMALLY -> ST_IDLE_NORMALLY")
				FSM_CASE_DEFAULT(ST_CONNECTED_NORMALLY, st_connect_normal_default(), "ST_CONNECTED_NORMALLY -> ST_CONNECTED_NORMALLY")   //transit to idle state by default.
			}
		}
		FSM_STATE_UNFORCED(ST_CONNECTED_RECONF, "ST_CONNECTED_RECONF", st_connect_reconf_enter(),st_connect_reconf_exit())
		{
			FSM_COND_TEST_IN("ST_CONNECTED_RECONF")
			FSM_TEST_COND(Evt_ReconfigSuccess)
			FSM_TEST_COND(Evt_ReconfigFail)
			FSM_COND_TEST_OUT("ST_CONNECTED_RECONF")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_CONNECTED_NORMALLY, reconfig_success(), "ST_CONNECTED_RECONF -> ST_CONNECTED_NORMALLY")
				FSM_CASE_TRANSIT(1, ST_CONNECTED_REESTABLISH, reconfig_fail(), "ST_CONNECTED_RECONF -> ST_CONNECTED_REESTABLISH")
				FSM_CASE_DEFAULT(ST_CONNECTED_RECONF, st_connect_reconf_default(), "ST_CONNECTED_RECONF -> ST_CONNECTED_RECONF")   //transit to idle state by default.
			}
		}
		FSM_STATE_UNFORCED(ST_CONNECTED_REESTABLISH, "ST_CONNECTED_REESTABLISH", st_connect_reest_enter(),st_connect_reest_exit())
		{
			FSM_COND_TEST_IN("ST_CONNECTED_REESTABLISH")
			FSM_TEST_COND(Evt_RcvRRCConnReestablish)
			FSM_TEST_COND(Evt_RcvRRCConnReestablishReject)
			FSM_TEST_COND(Evt_TimeOut_T301)
			FSM_TEST_COND(Evt_TimeOut_T311)
			FSM_COND_TEST_OUT("ST_CONNECTED_REESTABLISH")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_CONNECTED_NORMALLY, rcv_rrc_conn_reestablishment(), "ST_CONNECTED_REESTABLISH -> ST_CONNECTED_NORMALLY")
				FSM_CASE_TRANSIT(1, ST_IDLE_NORMALLY, rcv_rrc_conn_reestablish_reject(), "ST_CONNECTED_REESTABLISH -> ST_IDLE_NORMALLY")
				FSM_CASE_TRANSIT(2, ST_IDLE_NORMALLY, t301_timeout(), "ST_CONNECTED_REESTABLISH -> ST_IDLE_NORMALLY")
				FSM_CASE_TRANSIT(3, ST_IDLE_NORMALLY, t311_timeout(), "ST_CONNECTED_REESTABLISH -> ST_IDLE_NORMALLY")
				FSM_CASE_DEFAULT(ST_CONNECTED_REESTABLISH, st_connect_reest_default(), "ST_CONNECTED_REESTABLISH -> ST_CONNECTED_REESTABLISH")   //transit to idle state by default.
			}
		}
	}
	FSM_EXIT(0)
}


/******************************************************************************
 ** Function name:rrc_ioctl_handler
 ** Descriptions: handle ioctl arrival
 ** Input:   ioctl cmd
 ** Output:  trigger related event with fsm_schedule_self()
 ** Returns: void
 ** Created by: zheng lingjie
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by: Liu yingtao
 ** Modified Date: 
 ******************************************************************************/
static void rrc_ioctl_handler(void)
{
	FIN(rrc_ioctl_handler());
	SV_PTR_GET(rrc_sv);
	fsm_printf("[rrc] [fsm] in rrc_ioctl_handler().\n");

	switch(fsm_ev_ioctrl_cmd())
	{
		case IOCCMD_USTORRC_CELL_ACCESS:
			fsm_schedule_self(0, CODE_BEGIN_RECV_SYSINFO);
			FOUT;

		case IOCCMD_USTORRC_RRC_CONN_REQ:
			fsm_schedule_self(0, CODE_CONN_REQ_FROM_NAS);
			FOUT;

		case IOCCMD_USTORRC_RRC_CONN_RELEASE:
			fsm_schedule_self(0, CODE_RCV_RRC_CONN_RELEASE);
			FOUT;

		case IOCCMD_RLCTORRC_STATUS_IND:
			//fsm_schedule_self(0, );
			FOUT;

		case IOCCMD_MACTORRC_RANDOMACC_FAIL:
			//fsm_schedule_self(0, );
			FOUT;

		case IOCCMD_MACTORRC_REPORT_CRNTI:
			//fsm_schedule_self(0, );
			FOUT;

		default:
			fsm_printf("[rrc] [fsm] Unrecognized I/O control command.\n");
			FOUT;
	}

}


/******************************************************************************
 ** Function name:rrc_messege_handler_rlc
 ** Descriptions: handle message from rlc
 ** Input:   sb_buff pkptr
 ** Output:  change state to handle messages
 ** Returns: void
 ** Created by:  zhen lingjie
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void rrc_messege_handler_rlc(FSM_PKT* pkptr)
{
	FIN(rrc_messege_handler_rlc());
	SV_PTR_GET(rrc_sv);
	fsm_printf("[rrc] [fsm] rrc_messege_handler_rlc().\n");

	struct lte_rrc_head* sh_ptr;
	sh_ptr = (struct lte_rrc_head*)pkptr->data;
	fsm_skb_pull(pkptr, sizeof(struct lte_rrc_head));

	fsm_printf("[rrc] [fsm] --> Msg type: %d\n", sh_ptr->message_type);
	fsm_printf("[rrc] [fsm] --> sh_ptr addr:%p\n", sh_ptr);

	if(sh_ptr->message_type == 0)
	{
		//PCCH_Message
		struct PCCH_Message *pcch_msg;
		fsm_printf("[rrc] [fsm] --> recive PCCH_Message.\n");
		pcch_msg = (struct PCCH_Message *)pkptr->data;
		
		check_free_ptr(paging);
		paging = fsm_mem_alloc(sizeof(struct Paging));
		fsm_mem_cpy(paging, (void *)&(pcch_msg->paging), sizeof(struct Paging));
		fsm_printf("[rrc] [fsm] --> recived Paging!\n");
		fsm_schedule_self(0, CODE_RCV_PAGING);
	}
	else if(sh_ptr->message_type == 1)
	{
		//BCCH_BCH_Message
		struct BCCH_BCH_Message *bcch_bch_msg;
		fsm_printf("[rrc] [fsm] --> recive BCCH_BCH_Message.\n");
		bcch_bch_msg = (struct BCCH_BCH_Message *)pkptr->data;

		//struct MasterInformationBlock mib = bcch_bch_msg->mib;
		check_free_ptr(mib);
		mib = fsm_mem_alloc(sizeof(struct MasterInformationBlock));
		fsm_mem_cpy(mib, (void *)&(bcch_bch_msg->mib), sizeof(struct MasterInformationBlock));

		fsm_printf("[rrc] [fsm] --> recived MIB!\n");
		fsm_schedule_self(0, CODE_RCV_MIB);

	}
	else if(sh_ptr->message_type == 2)
	{
		//BCCH_DL_SCH_Message
		fsm_printf("[rrc] [fsm] --> recive BCCH_DL_SCH_Message.\n");
		struct BCCH_DL_SCH_Message *bcch_dl_sch_msg = (struct BCCH_DL_SCH_Message *)pkptr->data;
		if (bcch_dl_sch_msg->type == 1)
		{
			check_free_ptr(si);
			si = fsm_mem_alloc(sizeof(struct SystemInformation));
			fsm_mem_cpy(si, (void *)&(bcch_dl_sch_msg->msg.si), sizeof(struct SystemInformation));
			fsm_printf("[rrc] [fsm] --> recived SI!\n");
			fsm_schedule_self(0, CODE_RCV_SI);
		}
		else
		{
			check_free_ptr(sib1);
			sib1 = fsm_mem_alloc(sizeof(struct SystemInformationBlockType1));
			fsm_mem_cpy(sib1, (void *)&(bcch_dl_sch_msg->msg.sib1), sizeof(struct SystemInformationBlockType1));
			fsm_printf("[rrc] [fsm] --> recived SIB1!\n");
			fsm_schedule_self(0, CODE_RCV_SIB1);
		}
	}
	else if(sh_ptr->message_type == 3)
	{
		//DL_CCCH_Message
		fsm_printf("[rrc] [fsm] --> recive DL_CCCH_Message.\n");
		struct DL_CCCH_Message *dl_ccch_msg = (struct DL_CCCH_Message *)pkptr->data;

		fsm_printf(" 		  dl_ccch_msg->type=%d.\n",dl_ccch_msg->type);
		fsm_printf(" 		  dl_ccch_msg addr=%p.\n",dl_ccch_msg);

		if (dl_ccch_msg->type == 3)
		{
			check_free_ptr(rrcConnectionReject);
			rrcConnectionReject = fsm_mem_alloc(sizeof(struct RRCConnectionReject));
			fsm_mem_cpy(rrcConnectionReject, (void *)&(dl_ccch_msg->msg.rrcConnectionReject), sizeof(struct RRCConnectionReject));
			fsm_schedule_self(0, CODE_RCV_RRC_CONN_REJECT);
		}
		else if (dl_ccch_msg->type == 4)
		{
			check_free_ptr(rrcConnectionSetup);
			rrcConnectionSetup = fsm_mem_alloc(sizeof(struct RRCConnectionSetup));
			fsm_printf("[rrc] dl_ccch_msg->msg.rrcConnectionSetup.radioResourceConfigDedicated.drbToAddModList.num = %d\n",dl_ccch_msg->msg.rrcConnectionSetup.radioResourceConfigDedicated.drbToAddModList.num);
			fsm_printf("[rrc] dl_ccch_msg->msg.rrcConnectionSetup.rrcTransactionIdentifier = %d\n",dl_ccch_msg->msg.rrcConnectionSetup.rrcTransactionIdentifier);
			fsm_mem_cpy(rrcConnectionSetup, &(dl_ccch_msg->msg.rrcConnectionSetup), sizeof(struct RRCConnectionSetup));
			fsm_schedule_self(1000, CODE_RCV_RRC_CONN_SETUP);
		}

	}
	else if(sh_ptr->message_type == 4)
	{
		//DL_DCCH_Message
		fsm_printf("[rrc] [fsm] --> recive DL_DCCH_Message.\n");
		struct DL_DCCH_Message *dl_dcch_msg = (struct DL_DCCH_Message *)pkptr->data;
		if (dl_dcch_msg->type == 1)
		{
			check_free_ptr(rrcConnectionReconfiguration);
			rrcConnectionReconfiguration = fsm_mem_alloc(sizeof(struct RRCConnectionReconfiguration));
			fsm_mem_cpy(rrcConnectionReconfiguration, (void *)&(dl_dcch_msg->msg.rrcConnectionReconfiguration), sizeof(struct RRCConnectionReconfiguration));
			fsm_schedule_self(1000, CODE_RCV_RRC_CONN_RECONF);
		}
		else if (dl_dcch_msg->type == 2)
		{
			check_free_ptr(rrcConnectionRelease);
			rrcConnectionRelease = fsm_mem_alloc(sizeof(struct RRCConnectionRelease));
			fsm_mem_cpy(rrcConnectionRelease, (void *)&(dl_dcch_msg->msg.rrcConnectionRelease), sizeof(struct RRCConnectionRelease));
			fsm_schedule_self(1000, CODE_RCV_RRC_CONN_RELEASE);
		}

	}
	fsm_pkt_destroy(pkptr);

	FOUT;
}

/******************************************************************************
 ** Function name:rrc_messege_handler_upper
 ** Descriptions: handle message from upper
 ** Input:   sb_buff pkptr
 ** Output:  for now, it just destroy the input message.
 ** Returns: void
 ** Created by: zhen lingjie
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by: 
 ** Modified Date: 
 ******************************************************************************/
static void rrc_messege_handler_upper(FSM_PKT* pkptr)
{
	FIN(rrc_messege_handler_upper());
	SV_PTR_GET(rrc_sv);
	fsm_printf("[rrc] [fsm] rrc_messege_handler_upper().\n");
	fsm_pkt_destroy(pkptr);

	FOUT;
}



/******************************************************************************
 ** Function name:other_events_handler
 ** Descriptions: handle events which won't change states
 ** Input:  some event. 
 ** Output: do stuff according to current state and event
 ** Returns: void
 ** Created by: zhen lingjie
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void other_events_handler()
{
	FIN(other_events_handler());

	SV_PTR_GET(rrc_sv);
	fsm_printf("[rrc] [fsm] other_events_handler().\n");

	int *state=fsm_current_state_ptr_get();
	if (Evt_RCV_PAGING && (*state ==7|| *state ==11))
	{
		fsm_printf("[rrc] [fsm] start to handle paging.\n");
		if(*state ==7 && paging->havePagingRecord==true)
		{
			CheckPagingRecord(paging->pagingRecordList,ue_Identity);
		}

		if (paging->systemInfoModification==true)
		{
			fsm_printf("[rrc] [fsm] paging.systemInfoModification==true, renew the system information from enb.\n");
			Begin_recv_sysinfo();
		}

		fsm_printf("[rrc] [fsm] handle paging successfully.\n");

	}

	else if(Evt_RCV_MIB && (*state ==5||*state ==7|| *state ==11) )
	{
		m_receivedmib=true;
		DoReceiveMIB(mib);

		if(*state==5 && m_receivedmib==true && m_receivedsib1==true && m_receivedSib2==true)
		{
			fsm_printf("[rrc] [fsm] receive all system information and current fsm state is ST_IDLE_WAIT_SYSINFO, transit to ST_IDLE_NORMALLY.\n");
			fsm_schedule_self(0, CODE_END_RECV_SYSINFO);
		}
	}
	else if(Evt_RCV_SIB1 && (*state ==5||*state ==7|| *state ==11))
	{
		m_receivedsib1=true;
		fsm_printf("[rrc] [fsm] start to handle SIB1 .\n");
		SendCellAccessRelatedInfotoUS(sib1->cellAccessRelatedInfo);
		fsm_printf("[rrc] [fsm] handle SIB1 successfully.\n");

		if(*state==5 && m_receivedmib==true && m_receivedsib1==true && m_receivedSib2==true)
		{
			fsm_printf("[rrc] [fsm] receive all system information and current fsm state is ST_IDLE_WAIT_SYSINFO, transit to ST_IDLE_NORMALLY.\n");
			fsm_schedule_self(0, CODE_END_RECV_SYSINFO);
		}
	}
	else if(Evt_RCV_SI && (*state ==5||*state ==7|| *state ==11))
	{
		fsm_printf("[rrc] [fsm] start to handle SI .\n");
		if(si->haveSib2==true)
		{
			m_receivedSib2=true;
			if(si->sib2.haveMBSFN_SubframeConfigList==true)
			{
			}

			ApplyRadioResourceConfigCommon(si->sib2.radioResourceConfigCommon);

			if(*state==5 && m_receivedmib==true && m_receivedsib1==true && m_receivedSib2==true)
			{
				fsm_printf("[rrc] [fsm] receive all system information and current fsm state is ST_IDLE_WAIT_SYSINFO, transit to ST_IDLE_NORMALLY.\n");
				fsm_schedule_self(0, CODE_END_RECV_SYSINFO);
			}
		}

		fsm_printf("[rrc] [fsm] handle SI successfully.\n");
	}

	else if(Evt_TimeOut_T302)
	{
		//fix me: T302 timeout
	}
	else if(Evt_TimeOut_T305)
	{
		//fix me: T305 timeout
	}
	else if(Evt_TimeOut_T320)
	{
		//fix me: T320 timeout
	}
	else if(Evt_TimeOut_T321)
	{
		//fix me: T321 timeout
	}
	FOUT;
}



/******************************************************************************
 ** Function name:rrc_close
 ** Descriptions: release global resources when rrc close.
 ** Input:   FSM close event 
 ** Output:  release resources
 ** Returns: void
 ** Created by: zhen lingjie
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by: zhou yang 
 ** Modified Date: 2014/10/1
 ******************************************************************************/
void rrc_close(void)
{
	FIN(rrc_close());
	SV_PTR_GET(rrc_sv);
	fsm_printf("[rrc] [fsm] RRC is closing, release malloced memory!.\n");


	if(paging != NULL)
	{
		fsm_mem_free(paging);
		paging=NULL;
	}
	if(mib != NULL)
	{
		fsm_mem_free(mib);
		mib=NULL;
	}
	if(si != NULL)
	{
		fsm_mem_free(si);
		si=NULL;
	}
	if(sib1 != NULL)
	{
		fsm_mem_free(sib1);
		sib1=NULL;
	}
	if(rrcConnectionReject != NULL)
	{
		fsm_mem_free(rrcConnectionReject);
		rrcConnectionReject=NULL;
	}
	if(rrcConnectionSetup != NULL)
	{
		fsm_mem_free(rrcConnectionSetup);
		rrcConnectionSetup=NULL;
	}
	if(rrcConnectionReconfiguration != NULL)
	{
		fsm_mem_free(rrcConnectionReconfiguration);
		rrcConnectionReconfiguration=NULL;
	}
	if(rrcConnectionRelease != NULL)
	{
		fsm_mem_free(rrcConnectionRelease);
		rrcConnectionRelease=NULL;
	}

	if(ueCellReselectionPriorityList!=NULL)
	{
		fsm_mem_free(ueCellReselectionPriorityList);
			ueCellReselectionPriorityList=NULL;
	} //zhouyang 2014/10/1
/*
	if(ueFreqPriorityList!=NULL)
	{
		fsm_mem_free(ueFreqPriorityList);
		ueFreqPriorityList=NULL;
	}
*/
	stop_timer();
	release_radio_resource();
	FOUT;
}



/* ----------------------------------------------------------------------------
   functions executed under state:ST_IDLE_CELL_SEL
   ----------------------------------------------------------------------------- */

/******************************************************************************
 ** Function name: test_recv_pkt_from_rlc
 ** Descriptions: 
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void test_recv_pkt_from_rlc()
{
	struct MasterInformationBlock mib;
	mib.dl_Bandwidth = n25;
	mib.systemFrameNumber = (u8)120;
	struct BCCH_BCH_Message bcch_bch_msg;
	bcch_bch_msg.mib = mib;
	char *msg = (char *)&bcch_bch_msg;
	int msg_len = sizeof(bcch_bch_msg);
	int message_type = 1;

	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);

	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head))
	{
		pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr == NULL)
			return;
	}
	fsm_skb_push(pkptr, sizeof(struct lte_rrc_head));
	sh_ptr = (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = message_type;

	rrc_messege_handler_rlc(pkptr);

}
/******************************************************************************
 ** Function name: test_send_pkt_to_rlc
 ** Descriptions: 
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void test_send_pkt_to_rlc()
{
	struct MasterInformationBlock mib;
	mib.dl_Bandwidth = n25;
	mib.systemFrameNumber = (u8)120;
	struct BCCH_BCH_Message bcch_bch_msg;
	bcch_bch_msg.mib = mib;
	char *msg = (char *)&bcch_bch_msg;
	int msg_len = sizeof(bcch_bch_msg);
	int message_type = 1;
	packet_send_to_rlc(msg, msg_len, message_type);
}

/******************************************************************************
 ** Function name: test_send_msg_to_us
 ** Descriptions: 
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void test_send_msg_to_us()
{
	//test send message to user space
	fsm_printf("********test send message to us.\n");
	char *data = "a test message from rrcfsm.c";
	//send_to_user(data, strlen(data)+1);
}

void test_paging()
{
	FSM_PKT* pkptr=gen_paging();
	rrc_messege_handler_rlc(pkptr);
}
void test_sib1()
{
	FSM_PKT* pkptr=gen_sib1();
	rrc_messege_handler_rlc(pkptr);
}
void test_mib()
{
	FSM_PKT* pkptr=gen_mib();
	rrc_messege_handler_rlc(pkptr);
}
void test_si()
{
	FSM_PKT* pkptr=gen_si();
	rrc_messege_handler_rlc(pkptr);
}

void test()
{
	//receive packet test
	test_recv_pkt_from_rlc();
	if (mib != NULL)
	{
		fsm_printf("bcch_bch_msg.mib.dl_Bandwidth:%d\n", mib->dl_Bandwidth);
		fsm_printf("bcch_bch_msg.mib.systemFrameNumber:%d\n", mib->systemFrameNumber);
		fsm_mem_free(mib);
		mib = NULL;
	}
	else
		fsm_printf("mib is null\n");

	//test: ioctl with crnti
	u16 tmp_crnti = (u16)32;
	struct MasterInformationBlock tmp_mib;
	tmp_mib.dl_Bandwidth = n25;
	tmp_mib.systemFrameNumber = (u8)120;
	int data_size = sizeof(u16) + sizeof(struct MasterInformationBlock);
	char * data = (char *)fsm_mem_alloc(data_size);
	fsm_mem_cpy(data, &tmp_crnti, sizeof(u16));
	fsm_mem_cpy(data+sizeof(u16), &tmp_mib, sizeof(struct MasterInformationBlock));
	fsm_do_ioctrl(STRM_TO_RLCMAC, IOCCMD_RRCTOMAC_BEGIN_RECV_SYSINFO, (void*)data, data_size);
	fsm_mem_free(data);
}


/******************************************************************************
 ** Function name: st_init_enter
 ** Descriptions: enter INIT state
 ** Input:   void
 ** Output:  change state to INIT
 ** Returns: void
 ** Created by:  zhen lingjie
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/

static void st_init_enter(void)
{
	FIN(st_init_enter());
	if(RRC_OPEN)
	{
		fsm_printf("[rrc] [fsm] st_init_enter().\n");
		//fsm_printf("Setup message size: %d\n", sizeof(struct RRCConnectionSetup));
		//SV init
		SV_PTR_GET(rrc_sv);
		SV(hello_count) = 0;
		//test();
	}
	FOUT;
}
/******************************************************************************
 ** Function name: st_init_enter
 ** Descriptions: exit INIT state
 ** Input:    void
 ** Output:   void
 ** Returns: void
 ** Created by: zhen lingjie
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_init_exit(void)
{
	FIN(st_init_exit());
	fsm_printf("[rrc] [fsm] st_init_exit().\n");
	FOUT;
}


/* -----------------------------------------------------------------------------
   functions executed under state:ST_IDLE_CELL_SEL
   ------------------------------------------------------------------------------ */

/******************************************************************************
 ** Function name: st_cell_sel_enter
 ** Descriptions: enter cell selection , begin to receive sys info
 ** Input:  void 
 ** Output: schedule event CODE_END_RECV_SYSINFO
 ** Returns: void
 ** Created by: zhen lingjie
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_cell_sel_enter(void)
{
	FIN(st_cell_sel_enter());
	fsm_printf("[rrc] [fsm] rrc_cell_sel_enter().\n");


	static int flag_st_cell_sel = 0;
	if(flag_st_cell_sel == 0)
	{
		fsm_schedule_self(0, CODE_BEGIN_RECV_SYSINFO); //test
		flag_st_cell_sel++;
	}

	FOUT;
}

/******************************************************************************
 ** Function name: st_cell_sel_exit
 ** Descriptions: exit cell selection
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_cell_sel_exit(void)
{
	FIN(st_cell_sel_exit());
	fsm_printf("[rrc] [fsm] st_cell_sel_exit().\n");

	//test_send_msg_to_us();

	if(RRC_CLOSE)
	{
		rrc_close();
	}
	FOUT;
}

/******************************************************************************
 ** Function name: st_cell_sel_default
 ** Descriptions: default handler for events
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_cell_sel_default(void)
{
	FIN(st_cell_sel_default());
	fsm_printf("[rrc] [fsm] st_cell_sel_default().\n");

	if(EVT_IOCTL_ARRIVAL)
	{
		rrc_ioctl_handler();
	}
	else if(EVT_RRC_PK_FROM_LOWER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_rlc(pkptr);
	}
	else if(EVT_RRC_PK_FROM_UPPER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_upper(pkptr);
	}
	else
	{
		other_events_handler();
	}

	FOUT;
}

/******************************************************************************
 ** Function name: start_recv_sysInfo
 ** Descriptions: start to receive sys info
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void start_recv_sysInfo(void)
{
	FIN(start_recv_sysInfo());
	fsm_printf("[rrc] [fsm] start_recv_sysInfo().\n");
	//Begin_recv_sysinfo();
	fsm_schedule_self(0, CODE_END_RECV_SYSINFO);

	FOUT;
}


/* ----------------------------------------------------------------------------
   functions executed under state:ST_IDLE_WAIT_SYSINFO
   ----------------------------------------------------------------------------- */

/******************************************************************************
 ** Function name: st_wait_sysInfo_enter
 ** Descriptions: wait for all kinds of sys infos
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_wait_sysInfo_enter(void)
{
	FIN(st_wait_sysInfo_enter());
	fsm_printf("[rrc] [fsm] st_wait_sysInfo_enter().\n");

	FOUT;
}
/******************************************************************************
 ** Function name: st_wait_sysInfo_exit
 ** Descriptions: all kinds of sys infos have been received
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_wait_sysInfo_exit(void)
{
	FIN(st_wait_sysInfo_exit());
	fsm_printf("[rrc] [fsm] st_wait_sysInfo_exit().\n");

	if(RRC_CLOSE)
	{
		rrc_close();
	}
	FOUT;
}
/******************************************************************************
 ** Function name: st_wait_sysInfo_default
 ** Descriptions: default handler of all kinds of events
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_wait_sysInfo_default(void)
{
	FIN(st_wait_sysInfo_default());
	fsm_printf("[rrc] [fsm] st_wait_sysInfo_default().\n");

	if(EVT_IOCTL_ARRIVAL)
	{
		rrc_ioctl_handler();
	}
	else if(EVT_RRC_PK_FROM_LOWER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_rlc(pkptr);
	}
	else if(EVT_RRC_PK_FROM_UPPER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_upper(pkptr);
	}
	else
	{
		other_events_handler();
	}

	FOUT;
}

/******************************************************************************
 ** Function name: transit_to_idle
 ** Descriptions: transit to state IDLE
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void transit_to_idle(void)
{
	FIN(transit_to_idle());
	fsm_printf("[rrc] [fsm] transit_to_idle().\n");
	fsm_do_ioctrl(STRM_TO_RLCMAC, TEST_RECV_PADING, NULL, 0);
	FOUT;
}


/* ----------------------------------------------------------------------------
   functions executed under state:ST_IDLE_NORMALLY
   ---------------------------------------------------------------------------- */
/******************************************************************************
 ** Function name: st_idle_normal_enter
 ** Descriptions: entering idle
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_idle_normal_enter(void)
{
	FIN(st_idle_normal_enter());
	fsm_printf("[rrc] [fsm] st_idle_normal_enter().\n");

	FOUT;
}
/******************************************************************************
 ** Function name: st_idle_normal_exit
 ** Descriptions: exit idle
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_idle_normal_exit(void)
{
	FIN(st_idle_normal_exit());
	fsm_printf("[rrc] [fsm] st_idle_normal_exit().\n");


	if(RRC_CLOSE)
	{
		rrc_close();
	}
	FOUT;
}
/******************************************************************************
 ** Function name: st_idle_normal_default
 ** Descriptions: default handler of all kinds of events
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_idle_normal_default(void)
{
	FIN(st_idle_normal_default());
	fsm_printf("[rrc] [fsm] st_idle_normal_default().\n");

	if(EVT_IOCTL_ARRIVAL)
	{
		rrc_ioctl_handler();
	}
	else if(EVT_RRC_PK_FROM_LOWER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_rlc(pkptr);
	}
	else if(EVT_RRC_PK_FROM_UPPER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_upper(pkptr);
	}
	else
	{
		other_events_handler();
	}

	FOUT;
}


/******************************************************************************
 ** Function name: transit_to_idle_connecting
 ** Descriptions: config lower layers ,and send rrc connection request 
 ** Input:   
 ** Output:  ioctl message to rlc mac and phy, creat a sb_buff which should be freed by lower layer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void transit_to_idle_connecting(void)
{

	FIN(transit_to_idle_connecting());
	fsm_printf("[rrc] [fsm] transit_to_idle_connecting().\n");
	
	struct RadioResourceConfigDedicated *radioResourceConfigdefault;
	radioResourceConfigdefault = gen_Defaultradioresource ();          
	struct PhysicalConfigDedicated physicalConfigdefault;
	physicalConfigdefault = radioResourceConfigdefault->physicalConfigDedicated ; 
	struct SPS_Config sps_Configdefault;
	sps_Configdefault = radioResourceConfigdefault->sps_Config;     
	struct MAC_MainConfig mac_MainConfigdefault;
	mac_MainConfigdefault = radioResourceConfigdefault->mac_MainConfig;    

	struct SystemInformationBlockType2 *TimeAlignmentTimerCommontest;
	TimeAlignmentTimerCommontest = gen_sysInfoTimer();
	struct UL_CCCH_Message *ulccchRRCrequestmsg;
	ulccchRRCrequestmsg = gen_ul_ccchmsg_send_rrcquest();                 

	set_default_physical_channel(physicalConfigdefault);
	set_default_semi_persistent (sps_Configdefault);

	fsm_printf("[rrc] [fsm] CCCH_Config we use the initial one ,so I do nothing here,if there may causes some problems,please contact me 3 Q .\n");

	set_default_MAC(mac_MainConfigdefault);
	set_timeAlignmentTimerCommon (TimeAlignmentTimerCommontest);

	setTimer(TIMER_T300, 2000000); 
	fsm_printf("[rrc] [fsm] Time_T300 is starting, the duration is 2000ms.\n");

	send_RRCConnectionRequest(ulccchRRCrequestmsg);

	fsm_mem_free(radioResourceConfigdefault);
	radioResourceConfigdefault = NULL;
	fsm_mem_free(TimeAlignmentTimerCommontest);
	TimeAlignmentTimerCommontest = NULL;
	fsm_mem_free(ulccchRRCrequestmsg);
	ulccchRRCrequestmsg = NULL;
	
	FOUT;
}



/* -----------------------------------------------------------------------------
   functions executed under state:ST_IDLE_CONNECTING
   ----------------------------------------------------------------------------- */

/******************************************************************************
 ** Function name: st_idle_connecting_enter
 ** Descriptions:  wait for rrc setup message
 ** Input:   
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_idle_connecting_enter(void)
{
	FIN(st_idle_connecting_enter());
	fsm_printf("[rrc] [fsm] st_idle_connecting_enter().\n");

	FOUT;
}

/******************************************************************************
 ** Function name: st_idle_connecting_exit_
 ** Descriptions: received  rrc setup message
 ** Input:   
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_idle_connecting_exit(void)
{
	FIN(st_idle_connecting_exit());
	fsm_printf("[rrc] [fsm] st_idle_connecting_exit().\n");

	if(RRC_CLOSE)
	{
		rrc_close();
	}

	FOUT;
}


/******************************************************************************
 ** Function name: st_idle_connecting_default
 ** Descriptions: default handler for all kinds of events
 ** Input:   
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_idle_connecting_default(void)
{
	FIN(st_idle_connecting_default());
	fsm_printf("[rrc] [fsm] st_idle_connecting_default().\n");

	if(EVT_IOCTL_ARRIVAL)
	{
		rrc_ioctl_handler();
	}
	else if(EVT_RRC_PK_FROM_LOWER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_rlc(pkptr);
	}
	else if(EVT_RRC_PK_FROM_UPPER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_upper(pkptr);
	}
	else
	{
		other_events_handler();
	}

	FOUT;
}


/******************************************************************************
 ** Function name: rcv_rrc_conn_setup
 ** Descriptions: received setup message, config lower layers and send rrc complete msg
 ** Input:  void 
 ** Output:  create a sb_buff which should be free by lower layer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void rcv_rrc_conn_setup(void)
{
	FIN(rcv_rrc_conn_setup());

	fsm_printf("[rrc] [fsm] rcv_rrc_conn_setup().\n");

	if(rrcConnectionSetup != NULL)
	{
		struct SrbToAddModList srbToAddModList_setup;
		struct DrbToAddModList drbToAddModList_setup;
		struct UL_DCCH_Message *Ul_Dcch_rrccompletemsg;

		srbToAddModList_setup = rrcConnectionSetup->radioResourceConfigDedicated.srbToAddModList; 
		drbToAddModList_setup = rrcConnectionSetup->radioResourceConfigDedicated.drbToAddModList;
		fsm_printf("[rrc] drbToAddModList.num = %d\n", drbToAddModList_setup.num);
		fsm_printf("[rrc] srbToAddModList.num = %d\n", srbToAddModList_setup.num);
		ApplySrbToAdd(srbToAddModList_setup);
	//	ApplyDrbToAdd(drbToAddModList_setup);

		set_MAC_mainconfig(rrcConnectionSetup);
		set_semi_persistent(rrcConnectionSetup);
		set_physicalConfigDedicated(rrcConnectionSetup);

		cancleTimer(TIMER_T302); 
		cancleTimer(TIMER_T305);

		Ul_Dcch_rrccompletemsg = gen_ul_dcch_rrccomplete(); 
		send_RRCConnectionSetupComplete(Ul_Dcch_rrccompletemsg);

		fsm_mem_free(Ul_Dcch_rrccompletemsg);
		Ul_Dcch_rrccompletemsg = NULL;
		cancleTimer(TIMER_T300); 

		fsm_mem_free(rrcConnectionSetup);
		rrcConnectionSetup = NULL;
	}
	else
	{
		fsm_printf("[rrc] [fsm] rrcConnectionSetup is NULL\n");
	}

	FOUT;

}

/******************************************************************************
 ** Function name: t300_timeout
 ** Descriptions: config lower layers when t300 timeout
 ** Input:    void
 ** Output:   reset connection config
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void t300_timeout(void)
{

	FIN(t300_timeout());

	struct DL_CCCH_Message *DL_ccch_rrcsetupmsg;
	struct SrbToAddModList srbToAddModList_setup;
	struct DrbToAddModList drbToAddModList_setup;
	int srbIndex;
	int drbIndex;

	fsm_printf("[rrc] [fsm] t300_timeout().\n");

	fsm_do_ioctrl(STRM_TO_RLCMAC,  IOCCMD_RRCTOMAC_RESET  , NULL, 0);
/*
	DL_ccch_rrcsetupmsg = gen_dl_ccch_send_rrcsetup();

	rrcConnectionSetup = &(DL_ccch_rrcsetupmsg->msg.rrcConnectionSetup);     
	srbToAddModList_setup = rrcConnectionSetup->radioResourceConfigDedicated.srbToAddModList;
	drbToAddModList_setup = rrcConnectionSetup->radioResourceConfigDedicated.drbToAddModList;

	for(srbIndex=0; srbIndex<srbToAddModList_setup.num; srbIndex++)
	{
		fsm_printf("[rrc] [fsm] re-Establish the RLC of SRB.ID=%d \n",srbIndex+1);//test
		//	 fsm_do_ioctrl(STRM_TO_RLCMAC,  IOCCMD_RRCTORLC_BUILD  , NULL, 0);
	}
	for(drbIndex=0; drbIndex<drbToAddModList_setup.num; drbIndex++)
	{
		fsm_printf("[rrc] [fsm] re-Establish the RLC DRB.ID=%d \n",drbIndex+1);//test
		//	 fsm_do_ioctrl(STRM_TO_RLCMAC,  IOCCMD_RRCTORLC_BUILD  , NULL, 0);
	}

	fsm_mem_free(DL_ccch_rrcsetupmsg);
	DL_ccch_rrcsetupmsg = NULL;
	*/
	cancleTimer(TIMER_T300);
	FOUT;
}
/******************************************************************************
 ** Function name: rcv_rrc_conn_reject
 ** Descriptions: config lower layers when receive rrc connection reject
 ** Input:  void 
 ** Output: cancle Timer T300, reset MAC and set Timer T302
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void rcv_rrc_conn_reject(void)
{
	FIN(rcv_rrc_conn_reject());

	fsm_printf("[rrc] [fsm] rcv_rrc_conn_reject().\n");
	cancleTimer(TIMER_T300);
	fsm_do_ioctrl(STRM_TO_RLCMAC,  IOCCMD_RRCTOMAC_RESET  , NULL, 0);
	setTimer(TIMER_T302, 2000);

	FOUT;
}


/* ----------------------------------------------------------------------------
   functions executed under state:ST_CONNECTED_NORMALLY
   -------------------------------------------------------------------------- */


/******************************************************************************
 ** Function name: st_connect_normal_enter
 ** Descriptions:  enter state connect_normal
 ** Input:  void 
 ** Output: void
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_connect_normal_enter(void)
{
	FIN(st_connect_normal_enter());
	fsm_printf("[rrc] [fsm] st_connect_normal_enter().\n");

	FOUT;
}
static void st_connect_normal_exit(void)
{
	FIN(st_connect_normal_exit());
	fsm_printf("[rrc] [fsm] st_connect_normal_exit().\n");

	if(RRC_CLOSE)
	{
		rrc_close();
	}

	FOUT;
}


/******************************************************************************
 ** Function name: st_connect_normal_default
 ** Descriptions: default handler for all kinds of events 
 ** Input:  void  
 ** Output:  handle packet ptr
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/

static void st_connect_normal_default(void)
{
	FIN(st_connect_normal_default());
	fsm_printf("[rrc] [fsm] st_connect_normal_default().\n");

	if(EVT_IOCTL_ARRIVAL)
	{
		rrc_ioctl_handler();
	}
	else if(EVT_RRC_PK_FROM_LOWER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_rlc(pkptr);
	}
	else if(EVT_RRC_PK_FROM_UPPER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_upper(pkptr);
	}
	else
	{
		other_events_handler();
	}

	FOUT;
}


/******************************************************************************
 ** Function name: rcv_rrc_conn_reconf
 ** Descriptions: receive rrc connection reconfig msg ,start reconfig
 ** Input:  void
 ** Output: start to reconfig connection
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void rcv_rrc_conn_reconf(void)
{
	FIN(rcv_rrc_conn_reconf());
	fsm_printf("[rrc] [fsm] rcv_rrc_conn_reconf().\n");

	fsm_printf("[rrc] [fsm] DoRecvRrcConnectionReconfiguration().\n");

	//used for test
//	rrcConnectionReconfiguration = SetRrcConnReconfigmessage();
//	fsm_printf("[rrc] [fsm] rrcConnectionReconfiguration->rrcTransactionIdentifier=%d\n",rrcConnectionReconfiguration->rrcTransactionIdentifier);
	if(rrcConnectionReconfiguration == NULL)
	{
		fsm_printf("[rrc] invalid rrcConnectionReconfiguration.\n");
		FOUT;
	}
	DoRecvRrcConnectionReconfiguration(rrcConnectionReconfiguration);

//	fsm_mem_free(rrcConnectionReconfiguration);
//	rrcConnectionReconfiguration = NULL;

	FOUT;
}


/******************************************************************************
 ** Function name: rcv_rrc_conn_release
 ** Descriptions: UE receive rrc connection release msg
 ** Input:  void 
 ** Output: release connection
 ** Returns: void
 ** Created by: 
 ** Created Date: 2014/10/1
 **-------------------------------------- --------------------
 ** Modified by: zhou yang
 ** Modified Date: 2014/10/1
 ******************************************************************************/
static void rcv_rrc_conn_release(void)
{
	FIN(rcv_rrc_conn_release());
	fsm_printf("[rrc] [fsm] rcv_rrc_conn_release().\n");
	//generator rrcConnectionRelease info testing
//	rrcConnectionRelease = gen_release();

	//if have cell_reselection_priority_info then stored cell_reselection_priority_info
	cell_reselection_priority_info(rrcConnectionRelease);
	//set the cell_reselection_priority_info valid time
	set_t320(rrcConnectionRelease);
	//reset MAC
	fsm_do_ioctrl(STRM_TO_RLCMAC,  IOCCMD_RRCTOMAC_RESET  , NULL, 0);
	//stop all timer except t320 
	stop_timer();
	//release all radio resource
	release_radio_resource();
	//fix me:report release cause information to up
	//report_to_userspace();
	fsm_mem_free(rrcConnectionRelease);
	rrcConnectionRelease=NULL;
	FOUT;
}



/* -----------------------
   functions executed under state:ST_CONNECTED_RECONF
   ----------------------- */

/******************************************************************************
 ** Function name: st_connect_reconf_enter
 ** Descriptions: enter connection reconfig progress
 ** Input:  void 
 ** Output: schedule event CODE_RECONF_FAIL
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_connect_reconf_enter(void)
{
	FIN(st_connect_reconf_enter());
	fsm_printf("[rrc] [fsm] st_connect_reconf_enter().\n");


	static int flag_st_connect_reconf = 0;
	if(flag_st_connect_reconf == 0)
	{
		fsm_schedule_self(100, CODE_RECONF_SUCCESS); //test reconfig success
		flag_st_connect_reconf++;
	}


	FOUT;
}


/******************************************************************************
 ** Function name: st_connect_reconf_exit
 ** Descriptions: exit connection reconfig progress
 ** Input:  void 
 ** Output: void 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_connect_reconf_exit(void)
{
	FIN(st_connect_reconf_exit());
	fsm_printf("[rrc] [fsm] st_connect_reconf_exit().\n");

	if(RRC_CLOSE)
	{
		rrc_close();
	}
	FOUT;
}

/******************************************************************************
 ** Function name: st_connect_reconf_default
 ** Descriptions: default handler for all kinds of events in reconfig progress
 ** Input:   void
 ** Output:  handle some ioctl msg or packet
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void st_connect_reconf_default(void)
{
	FIN(st_connect_reconf_default());
	fsm_printf("[rrc] [fsm] st_connect_reconf_default().\n");

	if(EVT_IOCTL_ARRIVAL)
	{
		rrc_ioctl_handler();
	}
	else if(EVT_RRC_PK_FROM_LOWER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_rlc(pkptr);
	}
	else if(EVT_RRC_PK_FROM_UPPER)
	{
		FSM_PKT* pkptr = fsm_pkt_get();
		rrc_messege_handler_upper(pkptr);
	}
	else
	{
		other_events_handler();
	}

	FOUT;
}

/******************************************************************************
 ** Function name: reconfig_success
 ** Descriptions: reconfig success, send reconfig complete msg to eNB
 ** Input:  void 
 ** Output:  send reconfig complete msg
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void reconfig_success(void)
{
	FIN(reconfig_success());
	fsm_printf("[rrc] [fsm] reconfig_success().\n");
	struct RRCConnectionReconfigurationComplete msg2;
	if(rrcConnectionReconfiguration != NULL)
	{
		msg2.rrcTransactionIdentifier = rrcConnectionReconfiguration->rrcTransactionIdentifier;
		DoSendRrcConnectionReconfigurationCompleted(msg2);

		fsm_mem_free(rrcConnectionReconfiguration);
		rrcConnectionReconfiguration=NULL;
	}
	else
	{
		fsm_printf("[rrc] rrcConnectionReconfiguration is NULL!\n");
	}

	fsm_printf("[rrc] [fsm] reconfig success.\n");//test
	FOUT;
}


/******************************************************************************
 ** Function name: reconfig_fail
 ** Descriptions: reconfig failed
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void reconfig_fail(void)
{
}


/* -----------------------
   functions executed under state:ST_CONNECTED_REESTABLISH
   ----------------------- */
static void st_connect_reest_enter(void) {}
static void st_connect_reest_exit(void) {}
static void st_connect_reest_default(void) {}
static void rcv_rrc_conn_reestablishment(void) {}
static void rcv_rrc_conn_reestablish_reject(void) {}
static void t301_timeout(void) {}
static void t311_timeout(void) {}



