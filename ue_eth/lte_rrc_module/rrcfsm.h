
#ifndef _RRCFSM_H
#define _RRCFSM_H

#include "../fsm/fsmdec.h"
#include "rrc_type.h"



/*definition of states*/
#define ST_INIT 								0
#define ST_IDLE_CELL_SEL 						1
#define ST_IDLE_WAIT_SYSINFO 					2
#define ST_IDLE_NORMALLY 						3
#define ST_IDLE_CONNECTING 						4
#define ST_CONNECTED_NORMALLY 					5
#define ST_CONNECTED_RECONF 					6
#define ST_CONNECTED_REESTABLISH 				7



/* -----------------------
   Definition Used By  FSM_TEST_COND
   ----------------------- */
//ST_IDLE_CELL_SEL
#define EVT_BEGIN_RECV_SYSINFO (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_BEGIN_RECV_SYSINFO)

//ST_IDLE_WAIT_SYSINFO
#define EVT_END_RECV_SYSINFO (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_END_RECV_SYSINFO)

//ST_IDLE_NORMALLY
#define Evt_RcvConnReqNAS (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_CONN_REQ_FROM_NAS)

//ST_IDLE_CONNECTING
#define Evt_TimeOut_T300 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_TIMEOUT_T300)
#define Evt_RcvRRCConnReject (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_REJECT)
#define Evt_RcvRRCConnSetup (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_SETUP)

//ST_CONNECTED_NORMALLY
#define Evt_RcvRRCConnReconfig (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_RECONF)
#define Evt_RcvRRCConnRelease (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_RELEASE)

//ST_CONNECTED_RECONF
#define Evt_ReconfigSuccess (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RECONF_SUCCESS)
#define Evt_ReconfigFail (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RECONF_FAIL)

//ST_CONNECTED_REESTABLISH
#define Evt_RcvRRCConnReestablish (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_REESTABLISHMENT)
#define Evt_RcvRRCConnReestablishReject (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_RRC_CONN_REESTABLISH_REJECT)
#define Evt_TimeOut_T301 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_TIMEOUT_T301)
#define Evt_TimeOut_T311 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_TIMEOUT_T311)


//other events:  won't cause state transmition
#define Evt_RCV_PAGING (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_PAGING)
#define Evt_RCV_MIB (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_MIB)
#define Evt_RCV_SIB1 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_SIB1)
#define Evt_RCV_SI (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_RCV_SI)

#define Evt_TimeOut_T302 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_TIMEOUT_T302)
#define Evt_TimeOut_T305 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_TIMEOUT_T305)
#define Evt_TimeOut_T320 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_TIMEOUT_T320)
#define Evt_TimeOut_T321 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_TIMEOUT_T321)




/* -----------------------
   define ioctl event and packet event
   ----------------------- */
#define EVT_IOCTL_ARRIVAL (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL)
#define EVT_RRC_PK_FROM_LOWER (fsm_ev_type()==FSM_EV_TYPE_PKT_IND)
#define EVT_RRC_PK_FROM_UPPER (fsm_ev_type()==FSM_EV_TYPE_PKT_REQ)


/* -----------------------
   define system event
   ----------------------- */
#define RRC_OPEN (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code()== FSM_EV_CORE_OPEN)
#define RRC_CLOSE (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_CORE_CLOSE)







/* -----------------------  declare functions   ----------------------- */

/*function to handle ioctl events*/
static void rrc_ioctl_handler(void);
/*function to handle rrc messeges from rlc*/
static void rrc_messege_handler_rlc(FSM_PKT* pkptr);
/*function to handle rrc messeges from upper*/
static void rrc_messege_handler_upper(FSM_PKT* pkptr);
static void other_events_handler();

/*functions executed under state:ST_INIT*/
static void st_init_enter(void);
static void st_init_exit(void);

/*functions executed under state:ST_IDLE_CELL_SEL*/
static void st_cell_sel_enter(void);
static void st_cell_sel_exit(void);
static void st_cell_sel_default(void);
static void start_recv_sysInfo(void);

/*functions executed under state:ST_IDLE_WAIT_SYSINFO*/
static void st_wait_sysInfo_enter(void);
static void st_wait_sysInfo_exit(void);
static void st_wait_sysInfo_default(void);
static void transit_to_idle(void);

/*functions executed under state:ST_IDLE_NORMALLY*/
static void st_idle_normal_enter(void);
static void st_idle_normal_exit(void);
static void st_idle_normal_default(void);
static void transit_to_idle_connecting(void);


/*functions executed under state:ST_IDLE_CONNECTING*/
static void st_idle_connecting_enter(void);
static void st_idle_connecting_exit(void);
static void st_idle_connecting_default(void);
static void rcv_rrc_conn_setup(void);    //transit to conn normal
static void t300_timeout(void);    //transit to idle
static void rcv_rrc_conn_reject(void);    //transit to idle

/*functions executed under state:ST_CONNECTED_NORMALLY*/
static void st_connect_normal_enter(void);
static void st_connect_normal_exit(void);
static void st_connect_normal_default(void);
static void rcv_rrc_conn_reconf(void);    //transit to conn reconfig
static void rcv_rrc_conn_release(void);    //transit to idle

/*functions executed under state:ST_CONNECTED_RECONF*/
static void st_connect_reconf_enter(void);
static void st_connect_reconf_exit(void);
static void st_connect_reconf_default(void);
static void reconfig_success(void);    //transit to conn normal
static void reconfig_fail(void);    //transit to conn reestablish


/*functions executed under state:ST_CONNECTED_REESTABLISH*/
static void st_connect_reest_enter(void);
static void st_connect_reest_exit(void);
static void st_connect_reest_default(void);
static void rcv_rrc_conn_reestablishment(void);    //transit to conn normal
static void rcv_rrc_conn_reestablish_reject(void);    //transit to idle
static void t301_timeout(void);    //transit to idle
static void t311_timeout(void);    //transit to idle











/*system functions*/
void rrc_close(void);
void rrc_main(void);


#endif



