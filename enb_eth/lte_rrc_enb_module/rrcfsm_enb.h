

#ifndef _RRCFSM_ENB_H
#define _RRCFSM_ENB_H

#include "../fsm/fsmdec.h"
#include "rrc_utils.h"
#include "rrc_type.h"


#define ST_INIT 0
#define ST_IDLE 1
#define MAX_IOCTL_DATA_LEN 1024


/* 统计变量定义 */
typedef struct rrc_sv_enb{
	unsigned int hello_count;
	u16 cellId;
} rrc_sv_enb;


//其他模块与本模块进行ioctl通信时的数据交互格式
struct Ioctl_Msg_Rrc_Enb {
	unsigned short crnti;
	unsigned int data_len;
	char data[MAX_IOCTL_DATA_LEN]; 
};

//list:管理已接入ENB的UEs
struct UeFsm_list
{
	struct list_head list;
	u16 crnti;
	int uefsmid;
};

//IOCTL event code - RRC to MAC
#define IOCCMD_RRCTOMAC_UEFSM_BUILD_SUCCESS 	0x01     //eNB侧MAC层收到该命令后才上递Msg3



/* -----------------------
define ioctl event and packet event 
----------------------- */
#define EVT_IOCTL_ARRIVAL (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL)	
#define EVT_RRC_PK_FROM_LOWER (fsm_ev_type()==FSM_EV_TYPE_PKT_IND)
#define	EVT_RRC_PK_FROM_UPPER (fsm_ev_type()==FSM_EV_TYPE_PKT_REQ)
#define EVT_SEND_MIB (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_SEND_MIB)
#define EVT_SEND_SIB1 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_SEND_SIB1)
#define EVT_SEND_SI (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_SEND_SI)
#define EVT_SEND_PAGING (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CODE_SEND_PAGING)


/* -----------------------
define system event
----------------------- */
#define RRC_OPEN (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code()== FSM_EV_CORE_OPEN)
#define RRC_CLOSE (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_CORE_CLOSE)


/*functions*/
int crnti_to_uefsmid(u16 crnti);
//建立新的UE状态机
int new_ue_fsm(u16 crnti);
//删除UE状态机
void delete_ue_fsm(int uefsmid);


/*ioctl and packet handler*/
static void rrc_ioctl_handler(void);
static void rrc_message_handler_lower(FSM_PKT* pkptr);
static void rrc_message_handler_upper(FSM_PKT* pkptr);
static void other_events_handler();
static void st_idle_message_from_upper();
static void st_idle_message_from_lower();
static void rrc_send_mib();
static void rrc_send_sib1();
static void rrc_send_si();
static void rrc_send_paging();

/*functions executed under state:ST_INIT*/
static void st_init_enter(void);
static void st_init_exit(void);

/*functions executed under state:ST_IDLE*/
static void st_idle_enter(void);
static void st_idle_exit(void);
static void st_idle_default(void);

int register_new_ue_fsm(u16 crnti);
void unregister_ue_fsm(int fsm_id);


static do_receive_rnti_ind(void);


/*system functions*/
void rrc_close_enb(void);
void rrc_main_enb(void);



#endif



