#include <linux/if_ether.h>
#include "rrcfsm_enb_ue.h"
#include "../lte_system.h"
#include "../pkfmt.h"
//#include "impl_rrc_conn_reconfig_enb.h"
#include "rrc_utils.h"
#include "rrc_conn_release_enb.h"
#include "impl_rrc_conn_establish.h"

// defined in rrc_utils.c
extern struct LteSignalingRadioBearerInfo* enbSRBConfig[MAX_UEFSM_NUM][MAX_SRB_NUM];
extern struct LteDataRadioBearerInfo* enbDRBConfig[MAX_UEFSM_NUM][MAX_DRB_NUM];

struct RRCConnectionRequest* rrc_conn_req = NULL;
struct RRCConnectionSetup *rrc_conn_setup = NULL;
struct RRCConnectionReconfiguration *reconfmsg=NULL;
struct RRCConnectionReconfigurationComplete *rrc_conn_reconfig_comp = NULL;
struct RRCConnectionSetupComplete *rrc_conn_setup_comp = NULL;

struct SrbToAddMod* reserveSRB1msg = NULL;
struct SrbToAddMod* reserveSRB2msg = NULL;
struct DrbToAddMod* reserveDRB1msg = NULL;

void rrc_main_enb_ue(void)
{
	fsm_printf("[rrc ue] enter rrc_main_enb_ue().\n");
	if(EVT_POST_MSG_FROM_CONTROL_FSM){
		rcv_post_msg_from_control_fsm();
		return;
	}

	FSM_ENTER(rrc_main_enb_ue);
	FSM_BLOCK_SWITCH
	{
		FSM_STATE_FORCED(ST_INIT, "ST_INIT", st_init_enter(), st_init_exit())
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "ST_INIT -> ST_IDLE");
		}
		FSM_STATE_UNFORCED(ST_IDLE, "ST_IDLE", st_idle_enter(), st_idle_exit())
		{
			FSM_COND_TEST_IN("ST_IDLE")
				FSM_TEST_COND(EVT_RCV_RRC_CONN_REQUEST)
				FSM_COND_TEST_OUT("ST_IDLE")
				FSM_TRANSIT_SWITCH
				{   
					FSM_CASE_TRANSIT(0, ST_CONN_SETUP, rcv_rrc_conn_requset(), "ST_IDLE -> ST_CONN_SETUP")
						FSM_CASE_DEFAULT(ST_IDLE, st_idle_default(), "ST_IDLE -> ST_IDLE")
				}
		}
		FSM_STATE_UNFORCED(ST_CONN_SETUP, "ST_CONN_SETUP", st_conn_setup_enter(), st_conn_setup_exit())
		{
			FSM_COND_TEST_IN("ST_CONN_SETUP")
				FSM_TEST_COND(EVT_RCV_RRC_CONN_SETUP_COMPLETE)
				FSM_TEST_COND(EVT_MAX_CONN_TIMEOUT)
				FSM_TEST_COND(EVT_RCV_RRC_CONN_REQUEST)
				FSM_COND_TEST_OUT("ST_CONN_SETUP")
				FSM_TRANSIT_SWITCH
				{   
					FSM_CASE_TRANSIT(0, ST_CONN_NORMAL, rcv_rrc_conn_setup_complete(), "ST_CONN_SETUP -> ST_CONN_NORMAL")
						FSM_CASE_TRANSIT(1, ST_IDLE, conn_timeout(), "ST_CONN_SETUP -> ST_IDLE")
						FSM_CASE_TRANSIT(2, ST_CONN_SETUP, resend_setup_msg(), "ST_CONN_SETUP -> ST_CONN_SETUP")
						FSM_CASE_DEFAULT(ST_CONN_SETUP, st_conn_setup_default(), "ST_CONN_SETUP -> ST_CONN_SETUP")
				}
		}
		FSM_STATE_UNFORCED(ST_CONN_NORMAL, "ST_CONN_NORMAL", st_conn_normal_enter(), st_conn_normal_exit())
		{
			FSM_COND_TEST_IN("ST_CONN_NORMAL")
				FSM_TEST_COND(EVT_RRC_CONN_RECONFIG)
				FSM_TEST_COND(EVT_RRC_CONN_RELEASE)
				FSM_TEST_COND(EVT_RCV_RRC_CONN_REESTABLISH)
				FSM_COND_TEST_OUT("ST_CONN_NORMAL")
				FSM_TRANSIT_SWITCH
				{   
					FSM_CASE_TRANSIT(0, ST_CONN_RECONFIG, rrc_conn_reconfig(), "ST_CONN_NORMAL -> ST_CONN_RECONFIG")
						FSM_CASE_TRANSIT(1, ST_IDLE, rrc_conn_release(), "ST_CONN_NORMAL -> ST_IDLE")
						FSM_CASE_TRANSIT(2, ST_CONN_REESTABLISH, rrc_conn_reestablish(), "ST_CONN_NORMAL -> ST_CONN_REESTABLISH")
						FSM_CASE_DEFAULT(ST_CONN_NORMAL, st_conn_normal_default(), "ST_CONN_NORMAL -> ST_CONN_NORMAL")
				}
		}
		FSM_STATE_UNFORCED(ST_CONN_REESTABLISH, "ST_CONN_REESTABLISH", st_conn_reestablish_enter(), st_conn_reestablish_exit())
		{
			FSM_COND_TEST_IN("ST_CONN_NORMAL")
				FSM_COND_TEST_OUT("ST_CONN_NORMAL")
				FSM_TRANSIT_SWITCH
				{   
					FSM_CASE_DEFAULT(ST_CONN_NORMAL, st_conn_normal_default(), "ST_CONN_REESTABLISH -> ST_CONN_NORMAL")
				}
		}
		FSM_STATE_UNFORCED(ST_CONN_RECONFIG, "ST_CONN_RECONFIG", st_conn_reconfig_enter(), st_conn_reconfig_exit())
		{
			FSM_COND_TEST_IN("ST_CONN_RECONFIG")
				FSM_TEST_COND(EVT_RCV_RRC_CONN_RECONFIG_COMPLETE)
				FSM_COND_TEST_OUT("ST_CONN_RECONFIG")
				FSM_TRANSIT_SWITCH
				{   
					FSM_CASE_TRANSIT(0, ST_CONN_NORMAL, rcv_rrc_conn_reconfig_complete(), "ST_CONN_RECONFIG -> ST_CONN_NORMAL")
						FSM_CASE_DEFAULT(ST_CONN_RECONFIG, st_conn_reconfig_default(), "ST_CONN_RECONFIG -> ST_CONN_RECONFIG")
				}
		}
	}
	fsm_printf("[rrc ue] leave rrc_main_enb_ue().\n");
	FSM_EXIT(0)
	
}

static void rcv_post_msg_from_control_fsm(void)
{
	if(fsm_ev_code() <= 3)
		rrc_messege_handler_rlc_enb_ue();
	else
		rrc_ioctl_handler_enb_ue();
}


/*  function to dispatch ioctl event */
//转发给对应UE(根据c-rnti区分UE)
static void rrc_ioctl_handler_enb_ue(void)
{
	FIN(rrc_ioctl_handler_enb_ue());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("rrc_ioctl_handler_enb_ue().\n");
	char *data;

	switch(fsm_ev_code())
	{
		case IOCCMD_RRCTOUS_UE_CONN_REQ:
			data = (char *)fsm_data_get();
			fsm_printf("		IOCCMD_RRCTOUS_UE_CONN_REQ - rcv data:%s.\n", data);
			fsm_data_destroy(data);
			fsm_schedule_self(0, CODE_RCV_RRC_CONN_REQUEST);
			FOUT;
		case IOCCMD_USTORRC_RRC_CONN_ACCEPT:
			//fsm_schedule_self(0, );
			FOUT;

		case IOCCMD_USTORRC_RRC_CONN_REJECT:
			//fsm_schedule_self(0, );
			FOUT;

		case IOCCMD_USTORRC_RRC_CONN_RECONFIG:
			data = (char *)fsm_data_get();
			fsm_printf("		IOCCMD_USTORRC_CONN_RECONFIG - rcv data:%s.\n", data);
			fsm_data_destroy(data);
			fsm_schedule_self(0, CODE_RRC_CONN_RECONFIG);
			FOUT;

		case IOCCMD_USTORRC_RRC_CONN_RELEASE:
			fsm_schedule_self(0, CODE_RRC_CONN_RELEASE);
			FOUT;

			/* 		case IOCCMD_USTORRC_RRC_CONN_ACCEPT:
					fsm_post_msg(IOCCMD_USTORRC_RRC_CONN_ACCEPT, NULL, uefsmid);
					FOUT;

					case IOCCMD_USTORRC_RRC_CONN_REJECT:
					fsm_post_msg(IOCCMD_USTORRC_RRC_CONN_REJECT, NULL, uefsmid);
					FOUT;
					*/
		case RRC_FSM_ENB_UE_OPEN:
			fsm_schedule_self(100, EVT_RRC_OPEN);
			FOUT;

		case RRC_FSM_ENB_UE_CLOSE:
			rrc_close_enb_ue();
			FOUT;

		default:
			fsm_printf("rrc:Unrecognized ioctl command.\n");
			FOUT;
	}


	FOUT;
}


/*  function to dispatch rrc messeges from rlc*/
//注:fsm_ntohl(sh_ptr->message_type),在本地产生消息测试时不用fsm_ntohl
static void rrc_messege_handler_rlc_enb_ue()
{
	FIN(rrc_messege_handler_rlc_enb_ue());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("rrc_messege_handler_rlc_enb_ue().\n");

	switch(fsm_ev_code())
	{
		case PKT_RLC_IND_RRC_CONN_REQUEST:
			if(rrc_conn_req != NULL){
				fsm_mem_free(rrc_conn_req);
				rrc_conn_req = NULL;
			}
			//取出数据，保存到全局变量rrc_conn_req
			struct RRCConnectionRequest * rrc_conn_req_temp = (struct RRCConnectionRequest*)fsm_data_get();
			rrc_conn_req = (struct RRCConnectionRequest *)fsm_mem_alloc(sizeof(struct RRCConnectionRequest));
			fsm_mem_cpy(rrc_conn_req, rrc_conn_req_temp, sizeof(struct RRCConnectionRequest));

			fsm_schedule_self(0, CODE_RCV_RRC_CONN_REQUEST);

			fsm_printf("    PKT_RLC_IND_RRC_CONN_REQUEST happened.\n");
			fsm_printf("    type=%d, ue_Identity=%lld, establishmentCause=%d.\n", rrc_conn_req->type, rrc_conn_req->ue_Identity.randomValue, rrc_conn_req->establishmentCause);

			fsm_data_destroy(rrc_conn_req_temp);

			FOUT;

		case PKT_RLC_IND_RRC_CONN_SETUP_COMPLETE:
			if(rrc_conn_setup_comp != NULL){
				fsm_mem_free(rrc_conn_setup_comp);
				rrc_conn_setup_comp = NULL;
			}				
			struct RRCConnectionSetupComplete* rrc_conn_setup_comp_temp = (struct RRCConnectionSetupComplete*)fsm_data_get();
			rrc_conn_setup_comp = (struct RRCConnectionSetupComplete *)fsm_mem_alloc(sizeof(struct RRCConnectionSetupComplete));
			fsm_mem_cpy(rrc_conn_setup_comp, rrc_conn_setup_comp_temp, sizeof(struct RRCConnectionSetupComplete));
			fsm_schedule_self(0, CODE_RCV_RRC_CONN_SETUP_COMPLETE);

			fsm_data_destroy(rrc_conn_setup_comp_temp);
			FOUT;

		case PKT_RLC_IND_RRC_CONN_RECONFIG_COMPLETE:
			if(rrc_conn_reconfig_comp != NULL){
				fsm_mem_free(rrc_conn_reconfig_comp);
				rrc_conn_reconfig_comp = NULL;
			}				
			struct RRCConnectionReconfigurationComplete* rrc_conn_reconfig_comp_temp = (struct RRCConnectionReconfigurationComplete*)fsm_data_get();
			rrc_conn_reconfig_comp = (struct RRCConnectionReconfigurationComplete *)fsm_mem_alloc(sizeof(struct RRCConnectionReconfigurationComplete));
			fsm_mem_cpy(rrc_conn_reconfig_comp, rrc_conn_reconfig_comp_temp, sizeof(struct RRCConnectionReconfigurationComplete));
			fsm_schedule_self(0, CODE_RCV_RRC_CONN_RECONFIG_COMPLETE);

			fsm_data_destroy(rrc_conn_reconfig_comp_temp);
			FOUT;

		default:
			fsm_printf("rrc:Unrecognized pkt event.\n");
			FOUT;
	}

	FOUT;
}

/*function to handle rrc messeges from upper*/
static void rrc_messege_handler_upper_enb_ue()
{
	FIN(rrc_messege_handler_upper_enb_ue());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("rrc_messege_handler_upper().\n");

	FOUT;
}



/* function: 区分并处理other events:: 不会导致状态转移的事件 */
static void other_events_handler_enb_ue()
{
	FIN(other_events_handler_enb_ue());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("other_events_handler().\n");

	FOUT;
}



void rrc_close_enb_ue(void)
{
	FIN(rrc_close_enb_ue());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("rrc_close_enb_ue().\n");
	u16 tmp_crnti = SV(crnti);
	int i;
	int uefsmid = SV(uefsmid);
	//通知mac:该ue已删除
	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_RELEASE_UEFSM, (void*)&tmp_crnti, sizeof(u16));
	fsm_do_ioctrl(STRM_TO_RLC, IOCCMD_RRCTORLC_DEACT, &(SV(crnti)), sizeof(u16));

	if(rrc_conn_req != NULL)
		fsm_mem_free(rrc_conn_req);
	if(reconfmsg != NULL)
		fsm_mem_free(reconfmsg);
	if(rrc_conn_reconfig_comp != NULL)
		fsm_mem_free(rrc_conn_reconfig_comp);
	if(rrc_conn_setup_comp)
		fsm_mem_free(rrc_conn_setup_comp);


	for(i = 0; i < MAX_SRB_NUM; i ++)
	{
		if(enbSRBConfig[uefsmid][i] != NULL)
			fsm_mem_free(enbSRBConfig[uefsmid][i]);
	}

	for(i = 0; i < MAX_DRB_NUM; i ++)
	{
		if(enbDRBConfig[uefsmid][i] != NULL)
			fsm_mem_free(enbDRBConfig[uefsmid][i]);
	}
	FOUT;
}


/* ----------------------- functions executed under rrc states  ----------------------- */

/* -----------------------
   functions executed under state:st_init_enter
   ----------------------- */
/*
   static void test(){
//test packet_send_to_rlc
struct MasterInformationBlock mib;
mib.dl_Bandwidth = (u8)20;
mib.systemFrameNumber = (u8)120;
struct BCCH_BCH_Message bcch_bch_msg;
bcch_bch_msg.mib = mib;
char *msg = (char *)&bcch_bch_msg;
int msg_len = sizeof(bcch_bch_msg);
int message_type = 1;
fsm_printf("<--[rrc] test() send to rlc\n");
fsm_octets_print(msg, msg_len);
packet_send_to_rlc(msg, msg_len, message_type);
}
*/
static void st_init_enter(void)
{
	FIN(st_init_enter());
	SV_PTR_GET(rrc_sv_enb_ue);

	struct CRLC_ConfigReq_IoctrlMsg rlc_ioctlmsg;
	struct MAC_LogicalChannelConfig_IoctrlMsg mac_ioctlmsg1, mac_ioctlmsg2;
	if(EVT_RRC_OPEN){		
		fsm_printf("st_init_enter() ---- uefsmid=%d, crnti=%d.\n", SV(uefsmid), SV(crnti));
	}

	//initial configuration.
	//build SRB0 and SRB1
	struct T_AmDwParas amDw;
	struct T_AmUpParas amUp;
	rlc_ioctlmsg.crnti = SV(crnti);
	rlc_ioctlmsg.rbIdentity = 1;
	//rlc_ioctlmsg.configCause = 0; //0 means build a new instance.
	rlc_ioctlmsg.lcIdentity = rlc_ioctlmsg.rbIdentity;
	rlc_ioctlmsg.Mode = 1;   //fix me: 2 or 3???
	amDw.timerStatusProhibit = 150;
	amDw.timerReordering = 100;
	amUp.timerPollRetransmit = 150;
	amUp.PollPDU = 32;
	amUp.PollBYTE = 1000;
	amUp.maxRetxThreshold = 4;


	fsm_do_ioctrl(STRM_TO_RLC, IOCCMD_RRCTORLC_BUILD, &rlc_ioctlmsg, sizeof(struct CRLC_ConfigReq_IoctrlMsg));

	//fix me: configure mac.
	mac_ioctlmsg1.crnti = SV(crnti);
	mac_ioctlmsg1.logicalChannelIdentity = 0;
	mac_ioctlmsg1.logicalChannelConfig.haveUl_SpecificParameters = 1;
	mac_ioctlmsg1.logicalChannelConfig.ul_SpecificParameters.priority = 1,
	mac_ioctlmsg1.logicalChannelConfig.ul_SpecificParameters.prioritisedBitRate = infinity,
	mac_ioctlmsg1.logicalChannelConfig.ul_SpecificParameters.bucketSizeDuration = bucketSizeDuration_ms50,
	mac_ioctlmsg1.logicalChannelConfig.ul_SpecificParameters.logicalChannelGroup = 0;

	mac_ioctlmsg2.crnti = SV(crnti);
	mac_ioctlmsg2.logicalChannelIdentity = rlc_ioctlmsg.rbIdentity;
	mac_ioctlmsg2.logicalChannelConfig.haveUl_SpecificParameters = 1;
	mac_ioctlmsg2.logicalChannelConfig.ul_SpecificParameters.priority = 1,
	mac_ioctlmsg2.logicalChannelConfig.ul_SpecificParameters.prioritisedBitRate = infinity,
	mac_ioctlmsg2.logicalChannelConfig.ul_SpecificParameters.bucketSizeDuration = bucketSizeDuration_ms50,
	mac_ioctlmsg2.logicalChannelConfig.ul_SpecificParameters.logicalChannelGroup = 0;

	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_LCC, &mac_ioctlmsg1, sizeof(struct MAC_LogicalChannelConfig_IoctrlMsg));
	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_LCC, &mac_ioctlmsg2, sizeof(struct MAC_LogicalChannelConfig_IoctrlMsg));

	FOUT;
}
static void st_init_exit(void)
{
	FIN(st_init_exit());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_init_exit() ---- uefsmid=%d.\n", SV(uefsmid));
	FOUT;
}


/* -----------------------
   functions executed under state:ST_IDLE
   ----------------------- */
static void st_idle_enter(void)
{
	FIN(st_idle_enter());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_idle_enter() ---- uefsmid=%d.\n", SV(uefsmid));

	//fsm_schedule_self(0, CODE_RCV_RRC_CONN_REQUEST);
	FOUT;
}
static void st_idle_exit(void)
{
	FIN(st_idle_exit());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_idle_exit() ---- uefsmid=%d.\n", SV(uefsmid));


	FOUT;
}
static void st_idle_default(void)
{
	FIN(st_idle_default());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_idle_default() ---- uefsmid=%d.\n", SV(uefsmid));

	other_events_handler_enb_ue();

	FOUT;
}


static void st_connect_reconf_enter(void)
{
	FIN(st_connect_reconf_enter());
	fsm_printf("st_connect_reconf_enter().\n");

	FOUT;
}


static void st_conn_reconfig_enter(void){
	FIN(st_conn_reconfig_enter());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_conn_reconfig_enter() ---- uefsmid=%d.\n", SV(uefsmid));

	//  test_rrc_reconfig_complete();//test
	FOUT;
}
static void st_conn_reconfig_exit(void){
	FIN(st_conn_reconfig_exit());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_conn_reconfig_exit() ---- uefsmid=%d.\n", SV(uefsmid));

	FOUT;
}
static void st_conn_reconfig_default(void){
	FIN(st_conn_reconfig_default());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_conn_reconfig_default() ---- uefsmid=%d.\n", SV(uefsmid));

	other_events_handler_enb_ue();

	FOUT;
}

static void rcv_rrc_conn_reconfig_complete(void){
	FIN(rcv_rrc_conn_reconfig_complete());
	u8 rrctransid;	
	if(rrc_conn_reconfig_comp){
		rrctransid=rrc_conn_reconfig_comp->rrcTransactionIdentifier;
		fsm_printf("rcv_rrc_conn_reconfig_complete() ---- rrcTransactionIdentifier=%d.\n", rrctransid);
		fsm_mem_free(rrc_conn_reconfig_comp);	
		rrc_conn_reconfig_comp=NULL;
		//fix me:send SAE Bearer Setup Response to MME for confirm RB is activate

	}
	FOUT;
}



static void rcv_rrc_conn_requset(void){
	FIN(rcv_rrc_conn_requset());
	fsm_printf("rcv_rrc_conn_request().\n");
	//get RRC Connection Setup msg
	struct DL_CCCH_Message *DL_ccch_rrcsetupmsg;
	struct SrbToAddModList srbToAddModList_setup;
	struct DrbToAddModList drbToAddModList_setup;

	DL_ccch_rrcsetupmsg = gen_dl_ccch_send_rrcsetup();
	srbToAddModList_setup = DL_ccch_rrcsetupmsg->msg.rrcConnectionSetup.radioResourceConfigDedicated.srbToAddModList;

	if(rrc_conn_setup != NULL)
	{
		fsm_mem_free(rrc_conn_setup);
		rrc_conn_setup = NULL;
	}
	rrc_conn_setup = fsm_mem_alloc(sizeof(struct RRCConnectionSetup));
	fsm_mem_cpy(rrc_conn_setup, &(DL_ccch_rrcsetupmsg->msg.rrcConnectionSetup), sizeof(struct RRCConnectionSetup));

	//resever SRB1  mesg
	/*
	   reserveSRB1msg = &(rrcConnectionSetup->radioResourceConfigDedicated.srbToAddModList.srbList[0]);
	   reserveSRB2msg = &(rrcConnectionSetup->radioResourceConfigDedicated.srbToAddModList.srbList[1]);
	   reserveDRB1msg = &(rrcConnectionSetup->radioResourceConfigDedicated.drbToAddModList.drbList[0]);

	   fsm_printf("[rrc] [conn_establish] message of srb=%d has been resevered========\n",reserveSRB1msg->srbIdentity);
	   fsm_printf("[rrc] [conn_establish] message of srb=%d has been resevered========\n",reserveSRB2msg->srbIdentity);
	   fsm_printf("[rrc] [conn_establish] message of drb=%d has been resevered========\n",reserveDRB1msg->drb_Identity);
	 */


	//build instance
	fsm_printf("[rrc] [conn_establish] build srb1 and srb2 \n");
	fsm_printf("[rrc] [conn_establish] srb1.id = %d\n",srbToAddModList_setup.srbList[0].srbIdentity);
	fsm_printf("[rrc] [conn_establish] srb2.id = %d\n",srbToAddModList_setup.srbList[1].srbIdentity);
	ApplySrbToAdd(srbToAddModList_setup);

//	fsm_printf("[rrc] [conn_establish] built drb\n");
//	ApplyDrbToAdd(drbToAddModList_setup);

	//send RRcsetupmseg to ue
	send_RRCConnectionSetup(DL_ccch_rrcsetupmsg);
	fsm_mem_free(DL_ccch_rrcsetupmsg);

	//start timer
	//setTimer(Max_Conn_Time,50000);

}


/* -----------------------
   functions executed under state:ST_CONN_SETUP
   ----------------------- */
static void st_conn_setup_enter(void){
	FIN(st_conn_setup_enter());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_conn_setup_enter() ---- uefsmid=%d.\n", SV(uefsmid));

	//	fsm_schedule_self(0, CODE_RCV_RRC_CONN_SETUP_COMPLETE);
	FOUT;
}
static void st_conn_setup_exit(void){
	FIN(st_conn_setup_exit());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_conn_setup_exit() ---- uefsmid=%d.\n", SV(uefsmid));


	FOUT;
}
static void st_conn_setup_default(void){}
static void rcv_rrc_conn_setup_complete(void){
	FIN(rcv_rrc_conn_setup_complete());

	//if setupcomplete mes has MME mes resrver it
	//	struct UL_DCCH_Message* UL_DCCH_completemsg;
	//	UL_DCCH_completemsg = gen_ul_dcch_rrccomplete();
	//	rrc_conn_setup_comp = &(UL_DCCH_completemsg->msg.rrcConnectionSetupComplete) ;
	if(rrc_conn_setup_comp != NULL)
	{
		struct RegisteredMME* registeredmme = NULL;
		registeredmme = &(rrc_conn_setup_comp->registeredMME );
		fsm_printf("[rrc] [conn_establish]  mmec unmber = %d,registeredmme->mmec\n");
	}
	

	//fix me: add drb according to eps bearer info.


	fsm_schedule_self(0, CODE_RRC_CONN_RECONFIG);
	//cancleTimer(Max_Conn_Time);
	FOUT;

}
static void conn_timeout(void){
	FIN(conn_timeout());
	struct DL_CCCH_Message *DL_ccch_rrcrejmsg;
	DL_ccch_rrcrejmsg = gen_dl_ccch_send_rrcreject();
	//free resever msg
	fsm_mem_free(reserveSRB1msg);
	reserveSRB1msg = NULL;
	fsm_mem_free(reserveSRB2msg);
	reserveSRB2msg = NULL;
	fsm_mem_free(reserveDRB1msg);
	reserveDRB1msg = NULL;
	//send Rej msg
	fsm_printf("[rrc] [conn_establish]  waitTime = %d,DL_ccch_rrcrejmsg->rrcrejct.waitTime\n");
	//	send_RRCConnectionRejct(DL_ccch_rrcrejmsg);

	FOUT;

}


/* -----------------------
   functions executed under state:ST_CONN_NORMAL
   ----------------------- */
static void st_conn_normal_enter(void){
	FIN(st_conn_normal_enter());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_conn_normal_enter() ---- uefsmid=%d.\n", SV(uefsmid));

	//test RECONFIG request from enb's NAS
	//	fsm_schedule_self(0, CODE_RRC_CONN_RECONFIG);
	FOUT;
}
static void st_conn_normal_exit(void){
	FIN(st_conn_normal_exit());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("st_conn_normal_exit() ---- uefsmid=%d.\n", SV(uefsmid));


	FOUT;
}
static void st_conn_normal_default(void){}
static void rrc_conn_reconfig(void)
{
	FIN(rrc_conn_reconfig());
	fsm_printf("rrc_conn_reconfig()\n");
	addDrb(); //just like eps bearer info.
	struct DL_DCCH_Message *msg = gen_RRCConnectionReconfigurationMessage(2, 0);
	DoSendRRCConnectionReconfigurationMsg(msg);
	fsm_printf("rrc_conn_reconfig()\n");
	FOUT;
}

static void rrc_conn_release(void)
{
	FIN(rrc_conn_release()); 	
	fsm_printf("rrc_conn_release()\n"); 	
	send_rrc_connection_release_msg_to_ue(); 	
	stop_timer(); 	
	release_radio_resource(); 	
	FOUT;
}
static void st_conn_reestablish_enter(void){}
static void st_conn_reestablish_exit(void){}
static void rrc_conn_reestablish(void){}

//new drb according to eps bearer.
DRB_Identity new_drb(struct EpsBearer eps)
{
	int i;
	SV_PTR_GET(rrc_sv_enb_ue);
	struct LteDataRadioBearerInfo **drbConfig = enbDRBConfig[SV(uefsmid)];
	for(i=0; i<MAX_DRB_NUM; i++)
	{
		if(drbConfig[i] == NULL)
		{
			//fix me: finish drb info details, store it in enbDRBConfig
			struct LteDataRadioBearerInfo *drb = fsm_mem_alloc(sizeof(struct LteDataRadioBearerInfo));
			if(drb == NULL)
				return -1;

			drb->eps_BearerIdentity = i;
			drb->drbIdentity = i;
			drb->rlcConfig.type = 2;  //fix me: choose a rlc instance type according to epsbearer, set to 2 by now.
			//fix me: rlctype in detail.

			drb->lcid = drbid_to_lcid(i);
			drb->logicalChannelConfig.haveUl_SpecificParameters = 1;
			drb->logicalChannelConfig.ul_SpecificParameters.priority = getLogicalChannelPriority(eps);
			if(isGbr(eps))
			{
				drb->logicalChannelConfig.ul_SpecificParameters.prioritisedBitRate = eps.gbrQosInfo.gbrUl;
			}
			else
			{
				drb->logicalChannelConfig.ul_SpecificParameters.prioritisedBitRate = 0;
			}
			drb->logicalChannelConfig.ul_SpecificParameters.bucketSizeDuration = bucketSizeDuration_ms1000;
			drbConfig[i] = drb;
		}
		return i;
	}

	return -1;

}

static int isGbr(struct EpsBearer eps)
{
	switch (eps.qci)
	{
		case GBR_CONV_VOICE:
		case GBR_CONV_VIDEO:
		case GBR_GAMING:
		case GBR_NON_CONV_VIDEO:
			return 1;
		case NGBR_IMS:
		case NGBR_VIDEO_TCP_OPERATOR:
		case NGBR_VOICE_VIDEO_GAMING:
		case NGBR_VIDEO_TCP_PREMIUM:
		case NGBR_VIDEO_TCP_DEFAULT:
			return 0;
		default:
			return 0;
	}

}
static u32 getLogicalChannelGroup(struct EpsBearer eps)
{
	if (isGbr(eps))
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

static int getLogicalChannelPriority(struct EpsBearer eps)
{
	return eps.qci;
}

static u32 drbid_to_lcid(DRB_Identity drbid)
{
	    return drbid + 1;
}

//This function just releases drb at enb side, you should construct a reconfig msg to release related drb at ue side.
static void release_drb(DRB_Identity drbid)
{
	SV_PTR_GET(rrc_sv_enb_ue);
	struct LteDataRadioBearerInfo *drb = enbDRBConfig[SV(uefsmid)][drbid];
	if(drb != NULL)
	{
		fsm_mem_free(drb);
		drb = NULL;
	}
}

//just for now.
static void build_drb_map(void)
{
}

static void resend_setup_msg(void)
{

	SV_PTR_GET(rrc_sv_enb_ue);
    int msg_len = sizeof(struct DL_CCCH_Message);
    int message_type = 3;
    
	if(rrc_conn_setup != NULL)
		packet_send_to_rlc(rrc_conn_setup, msg_len, message_type, SV(crnti));
}

