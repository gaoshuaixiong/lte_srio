
#ifndef _RRCFSM_ENB_UE_H
#define _RRCFSM_ENB_UE_H

#include "../fsm/fsmdec.h"
#include "rrc_type.h"


/* 接收到来自总控制状态机的消息*/
#define EVT_POST_MSG_FROM_CONTROL_FSM (fsm_ev_src() == 0 && fsm_ev_type() == FSM_EV_TYPE_MSG)



#define ST_INIT 					0
#define ST_IDLE 					1
#define ST_CONN_SETUP 				2
#define ST_CONN_NORMAL 				3
#define ST_CONN_RECONFIG 			4
#define ST_CONN_REESTABLISH 		5



/* -----------------------
Definition Used By  FSM_TEST_COND  All code are defined in rrc_utils.h
----------------------- */
//ST_INIT
#define EVT_RRC_OPEN (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RRC_OPEN)

//ST_IDLE
#define EVT_RCV_RRC_CONN_REQUEST (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_REQUEST)

//ST_CONN_SETUP
#define EVT_RCV_RRC_CONN_SETUP_COMPLETE (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_SETUP_COMPLETE)
#define EVT_MAX_CONN_TIMEOUT (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_MAX_CONN_TIMEOUT)

//ST_CONN_NORMAL
#define EVT_RRC_CONN_RECONFIG (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RRC_CONN_RECONFIG)
#define EVT_RRC_CONN_RELEASE (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RRC_CONN_RELEASE)
#define EVT_RCV_RRC_CONN_REESTABLISH (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_REESTABLISH)

//ST_CONN_RECONFIG
#define EVT_RCV_RRC_CONN_RECONFIG_COMPLETE (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_RECONFIG_COMPLETE)

//ST_CONN_REESTABLISH


/*functions executed under state:ST_INIT*/
static void st_init_enter(void);
static void st_init_exit(void);

/*functions executed under state:ST_IDLE*/
static void st_idle_enter(void);
static void st_idle_exit(void);
static void st_idle_default(void);
static void rcv_rrc_conn_requset(void);


/*functions executed under state:ST_CONN_SETUP*/
static void st_conn_setup_enter(void);
static void st_conn_setup_exit(void);
static void st_conn_setup_default(void);
static void rcv_rrc_conn_setup_complete(void);
static void conn_timeout(void);
static void resend_setup_msg(void);


/*functions executed under state:ST_CONN_NORMAL*/
static void st_conn_normal_enter(void);
static void st_conn_normal_exit(void);
static void st_conn_normal_default(void);
static void rrc_conn_reconfig(void);
static void rrc_conn_release(void);
static void rrc_conn_reestablish(void);


/*functions executed under state:ST_CONN_RECONFIG*/
static void st_conn_reconfig_enter(void);
static void st_conn_reconfig_exit(void);
static void st_conn_reconfig_default(void);
static void rcv_rrc_conn_reconfig_complete(void);
static void rrc_conn_reconfig(void);

/*functions executed under state:ST_CONN_REESTABLISH*/
static void st_conn_reestablish_enter(void);
static void st_conn_reestablish_exit(void);



/*ioctl and packet handler*/
static void rcv_post_msg_from_control_fsm(void);
static void rrc_ioctl_handler_enb_ue(void);
static void rrc_messege_handler_rlc_enb_ue();
static void rrc_messege_handler_upper_enb_ue();
static void other_events_handler_enb_ue();


/*system functions*/
void rrc_close_enb_ue(void);
void rrc_main_enb_ue(void);


DRB_Identity new_drb(struct EpsBearer eps);
static int getLogicalChannelPriority(struct EpsBearer eps);
static u32 getLogicalChannelGroup(struct EpsBearer eps);
static int isGbr(struct EpsBearer eps);
static u32 drbid_to_lcid(DRB_Identity drbid);
static void release_drb(DRB_Identity drbid);
static void build_drb_map(void);

#endif



