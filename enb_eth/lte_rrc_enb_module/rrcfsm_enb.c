#include <linux/if_ether.h>
#include <linux/list.h>
#include "rrcfsm_enb.h"
#include "../lte_system.h"
#include "../pkfmt.h"
#include "rrc_enb_paging_SI.h"

#define  MAX_UE_NUM  		5

static struct list_head ueFsm_list_head;
static int ue_num = 0;
struct UeFsm_list *ue_list[MAX_UE_NUM];

void rrc_main_enb(void)
{
	fsm_printf("[rrc] enter rrc_main_enb().\n");
	FSM_ENTER(rrc_main_enb);
	FSM_BLOCK_SWITCH
	{
		FSM_STATE_FORCED(ST_INIT, "ST_INIT", st_init_enter(), st_init_exit())
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "ST_INIT -> ST_IDLE");
		}
		FSM_STATE_UNFORCED(ST_IDLE, "ST_IDLE", st_idle_enter(), st_idle_exit())
		{
			FSM_COND_TEST_IN("ST_IDLE")
				FSM_TEST_COND(EVT_IOCTL_ARRIVAL)
				FSM_TEST_COND(EVT_RRC_PK_FROM_LOWER)
				FSM_TEST_COND(EVT_RRC_PK_FROM_UPPER)
				FSM_TEST_COND(EVT_SEND_MIB)
				FSM_TEST_COND(EVT_SEND_SIB1)
				FSM_TEST_COND(EVT_SEND_SI)
				FSM_TEST_COND(EVT_SEND_PAGING)
				FSM_COND_TEST_OUT("ST_IDLE")
				FSM_TRANSIT_SWITCH
				{
					FSM_CASE_TRANSIT(0, ST_IDLE, rrc_ioctl_handler(), "ST_IDLE-> ST_IDLE")
						FSM_CASE_TRANSIT(1, ST_IDLE, st_idle_message_from_lower(), "ST_IDLE-> ST_IDLE")
						FSM_CASE_TRANSIT(2, ST_IDLE, st_idle_message_from_upper(), "ST_IDLE-> ST_IDLE")
						FSM_CASE_TRANSIT(3, ST_IDLE,rrc_send_mib(),"ST_IDLE-> ST_IDLE")
						FSM_CASE_TRANSIT(4,ST_IDLE,rrc_send_sib1(),"ST_IDLE-> ST_IDLE")
						FSM_CASE_TRANSIT(5,ST_IDLE,rrc_send_si(),"ST_IDLE-> ST_IDLE")
						FSM_CASE_TRANSIT(6,ST_IDLE,rrc_send_paging(),"ST_IDLE-> ST_IDLE")
						FSM_CASE_DEFAULT(ST_IDLE, st_idle_default(), "ST_IDLE -> ST_IDLE")
				}
		}
	}
	fsm_printf("[rrc] leave rrc_main_enb().\n");
	FSM_EXIT(0)

}


/*  根据crnti获取对应ue的fsm id, 若找不到则返回-1 */
int crnti_to_uefsmid(u16 crnti)
{
	fsm_printf("[rrc] in crnti_to_uefsmid(crnti = %d)\n", crnti);
	struct UeFsm_list *entry;
	struct list_head *p;

	if(list_empty(&ueFsm_list_head))
	{
		fsm_printf("crnti_to_uefsmid: ueFsm_list_head is empty.\n");
		return -1;
	}
	else
	{
		list_for_each(p,&ueFsm_list_head)
		{
			entry = list_entry(p,struct UeFsm_list, list);
			fsm_printf("[rrc] in list: crnti %d, uefsmid  %d\n", entry->crnti, entry->uefsmid);
			if(crnti == entry->crnti)
			{
				fsm_printf("[rrc] crnti_to_uefsmid: %d --> %d\n", crnti, entry->uefsmid);
				return entry->uefsmid;
			}
		}
	}
	return -1;
}


/*  判断是否新UE接入, 是则建立新UE状态机 */
int new_ue_fsm(u16 crnti)
{
	//int uefsmid = 1;
	int uefsmid = register_new_ue_fsm(crnti);
	if(-1 != uefsmid)
	{
		
		struct UeFsm_list* ue = (struct UeFsm_list *)fsm_mem_alloc(sizeof(struct UeFsm_list));
		ue_list[ue_num ++] = ue;
		ue->crnti = crnti;
		ue->uefsmid = uefsmid;
		list_add(&(ue->list),&ueFsm_list_head);
		fsm_post_msg(RRC_FSM_ENB_UE_OPEN, NULL, uefsmid, 0);
		fsm_printf("new_ue_fsm: uefsmid=%d\n", uefsmid);
	}
	else
	{
		fsm_printf("ERROR: creat new ue fsm fail!\n");
	}
	return uefsmid;
}

/*  删除指定ue状态机 */
void delete_ue_fsm(int uefsmid)
{
	FIN(delete_ue_fsm);
	struct UeFsm_list *entry;
	struct list_head *p;

	if(list_empty(&ueFsm_list_head))
	{
		fsm_printf("delete_ue_fsm: ueFsm_list_head is empty.\n");
		FOUT;
	}
	else
	{
		list_for_each(p,&ueFsm_list_head)
		{
			entry = list_entry(p,struct UeFsm_list, list);
			if(uefsmid == entry->uefsmid)
			{
				//关闭ue状态机
				fsm_post_msg(RRC_FSM_ENB_UE_CLOSE, NULL, uefsmid, 0);
				list_del(&(entry->list));
				unregister_ue_fsm(uefsmid);
				fsm_mem_free(entry);
				fsm_printf("delete_ue_fsm: ueFsm(uefsmid=%d) deleted.\n", uefsmid);
				break;
			}
		}
	}

}


/*  function to dispatch ioctl event */
//转发给对应UE(根据c-rnti区分UE)
//待完善:添加获取数据部分
static void rrc_ioctl_handler(void)
{
	FIN(rrc_ioctl_handler());
	SV_PTR_GET(rrc_sv_enb);
	fsm_printf("rrc_ioctl_handler().\n");

	u32 ioctl_cmd;
	int uefsmid=-1;

	ioctl_cmd = fsm_ev_ioctrl_cmd();

	//crnti == 0, for all users

	//if(ioctl_msg->crnti == 0)
	//{
	//if (ioctl_cmd == IOCCMD_USTORRC_PAGING)
		//{
			//fsm_printf("[rrc]handle paging message!\n");
			//fsm_schedule_self(0,CODE_SEND_PAGING);
			//FOUT;
		//}
	//}

	fsm_printf("[rrc] ioctl code = %d", ioctl_cmd);

	switch(ioctl_cmd)
	{
		case IOCCMD_USTORRC_RRC_CONN_ACCEPT:
			fsm_post_msg(IOCCMD_USTORRC_RRC_CONN_ACCEPT, NULL, uefsmid, 0);
			FOUT;

		case IOCCMD_USTORRC_RRC_CONN_REJECT:
			fsm_post_msg(IOCCMD_USTORRC_RRC_CONN_REJECT, NULL, uefsmid, 0);
			FOUT;

		case IOCCMD_USTORRC_RRC_CONN_RECONFIG:
			//fsm_post_msg(IOCCMD_USTORRC_RRC_CONN_RECONFIG, ioctl_msg->data, uefsmid, ioctl_msg->data_len);   
			FOUT;

		case IOCCMD_USTORRC_RRC_CONN_RELEASE:
			fsm_post_msg(IOCCMD_USTORRC_RRC_CONN_RELEASE, NULL, uefsmid, 0);
			FOUT;

		case IOCCMD_RLCTORRC_STATUS_IND:
			fsm_post_msg(IOCCMD_RLCTORRC_STATUS_IND, NULL, uefsmid, 0);
			FOUT;
		case IOCCMD_MACTORRC_REPORT_CRNTI:
			fsm_printf("[rrc] receive IOCCMD_MACTORRC_REPORT_CRNTI\n");
			do_receive_rnti_ind();
			FOUT;

		default:
			fsm_printf("rrc:Unrecognized I/O control command.\n");
			FOUT;
	}

//	fsm_data_destroy(ioctl_msg);
}


/*  function to dispatch rrc messages from rlc*/
//注:fsm_ntohl(sh_ptr->message_type),在本地产生消息测试时不用fsm_ntohl
static void rrc_message_handler_lower(FSM_PKT* pkptr)
{
	FIN(rrc_message_handler_lower());
	SV_PTR_GET(rrc_sv_enb);
	fsm_printf("rrc_message_handler_lower().\n");

	struct URLC_IciMsg* ici_ptr;
	struct lte_rrc_head* rrc_head_ptr;
	int uefsmid;
	u32 message_type;

	ici_ptr = (struct URLC_IciMsg*)pkptr->head;
	uefsmid = crnti_to_uefsmid(ici_ptr->rnti);
	if(uefsmid == -1)
	{
		fsm_printf("[rrc] UE(%d) does not exits.\n", ici_ptr->rnti);
		fsm_pkt_destroy(pkptr);
		FOUT;
	}

	u8 pbCh = ici_ptr->pbCh;
	u8 rbId = ici_ptr->rbId;

	rrc_head_ptr = (struct lte_rrc_head*)pkptr->data;
	fsm_printf("[rrc]pkptr->data:\n");
	fsm_octets_print(pkptr->data, 8);
	message_type = rrc_head_ptr->message_type;
	fsm_skb_pull(pkptr, sizeof(struct lte_rrc_head));

	fsm_printf("[rrc] message_type = %d @ %p \n", message_type, &(rrc_head_ptr->message_type));
	if(message_type == 5)
	{
		//msg from UL_CCCH_Message
		struct UL_CCCH_Message *ul_ccch_msg;

	//	ul_ccch_msg = (struct UL_CCCH_Message *)(pkptr->data + sizeof(struct lte_rrc_head));
		ul_ccch_msg = (struct UL_CCCH_Message *)pkptr->data;
		fsm_octets_print(pkptr->data + sizeof(struct lte_rrc_head), 100);
		fsm_printf("[rrc] ul_ccch_msg->type = %d @ %p \n", ul_ccch_msg->type, &(ul_ccch_msg->type));
		fsm_printf("[rrc] ul_ccch_msg->msg.rrcConnectionRequest.type = %d\n", ul_ccch_msg->msg.rrcConnectionRequest.type);
		if(ul_ccch_msg->type == 2)
		{
			struct RRCConnectionRequest *rrc_conn_req;

		    rrc_conn_req = (struct RRCConnectionRequest *)fsm_mem_alloc(sizeof(struct RRCConnectionRequest));
			fsm_mem_cpy(rrc_conn_req, &(ul_ccch_msg->msg.rrcConnectionRequest),sizeof(struct RRCConnectionRequest));
			fsm_printf("[rrc] post msg to fsm: %d\n", uefsmid);
			fsm_post_msg(PKT_RLC_IND_RRC_CONN_REQUEST, (void *)rrc_conn_req, uefsmid, sizeof(struct RRCConnectionRequest));
		}
	}
	else if(message_type == 6)
	{
		//msg from UL_DCCH_Message
		struct UL_DCCH_Message *ul_dcch_msg = (struct UL_DCCH_Message *)(pkptr->data);
		fsm_printf("[rrc] pktr->data is @ %p.\n",pkptr->data);
		fsm_printf("[rrc] pktr->data + sizeof(struct lte_rrc_head) is @ %p.\n",pkptr->data + sizeof(struct lte_rrc_head));
		fsm_printf("[rrc] UL_DCCH_Message @ %p.\n", ul_dcch_msg);
		fsm_printf("[rrc] UL_DCCH_Message type is %d @ %p.\n", ul_dcch_msg->type, &(ul_dcch_msg->type));
		
		if(ul_dcch_msg->type == 2)
		{
			struct RRCConnectionReconfigurationComplete *msg = (struct RRCConnectionReconfigurationComplete *)fsm_mem_alloc(sizeof(struct RRCConnectionReconfigurationComplete));
			fsm_mem_cpy(msg, &(ul_dcch_msg->msg.rrcConnectionReconfigurationComplete),sizeof(struct RRCConnectionReconfigurationComplete));
			fsm_post_msg(PKT_RLC_IND_RRC_CONN_RECONFIG_COMPLETE, (void *)msg, uefsmid, sizeof(struct RRCConnectionReconfigurationComplete));
		}
		else if(ul_dcch_msg->type == 4)
		{
			fsm_printf("[rrc] receive setup complete msg for ue:%d\n", uefsmid);
			struct RRCConnectionSetupComplete *msg = (struct RRCConnectionSetupComplete *)fsm_mem_alloc(sizeof(struct RRCConnectionSetupComplete));
			fsm_mem_cpy(msg, &(ul_dcch_msg->msg.rrcConnectionSetupComplete),sizeof(struct RRCConnectionSetupComplete));
			fsm_post_msg(PKT_RLC_IND_RRC_CONN_SETUP_COMPLETE, (void *)msg, uefsmid, sizeof(struct RRCConnectionSetupComplete));
		}
	}
	fsm_pkt_destroy(pkptr);
	FOUT;
}

/*function to handle rrc messages from upper*/
static void rrc_message_handler_upper(FSM_PKT* pkptr)
{
	FIN(rrc_message_handler_upper());
	SV_PTR_GET(rrc_sv_enb);
	fsm_printf("rrc_message_handler_upper().\n");

	FOUT;
}



/* function: 区分并处理other events:: 不会导致状态转移的事件 */
static void other_events_handler()
{
	FIN(other_events_handler());
	SV_PTR_GET(rrc_sv_enb);
	fsm_printf("other_events_handler().\n");

	FOUT;
}


//删除所有已建立的ue状态机
void rrc_close_enb(void)
{
	FIN(rrc_close_enb());
	SV_PTR_GET(rrc_sv_enb);

	int i=1;
	for(i=1; i<6; i++)
	{
		delete_ue_fsm(i);
	}

	FOUT;
}





/* ----------------------- functions executed under rrc states  ----------------------- */

/* -----------------------
   functions executed under state:st_init_enter
   ----------------------- */

void test(u16 crnti)
{
	//test new ue fsm
	/*u16 crnti = (u16)12345;*/

	fsm_printf("ue_fsm: crnti=%d.\n", crnti);
	new_ue_fsm(crnti);
	int uefsmid = crnti_to_uefsmid(crnti);
	if(uefsmid != -1)
		fsm_printf("ue_fsm: crnti=%d, uefsmid=%d.\n", crnti, uefsmid);
	else
		fsm_printf("The ue with crnti=%d doesn't exit\n", crnti);

}

static void st_init_enter(void)
{
	
	FIN(st_init_enter());
	

	if(RRC_OPEN)
	{
		fsm_printf("st_init_enter().\n");
		//SV init
		SV_PTR_GET(rrc_sv_enb);
		SV(hello_count) = 0;
		SV(cellId) = 1;

		//初始化ueFsm_list_head
		INIT_LIST_HEAD(&ueFsm_list_head);
	}
	
	FOUT;
}
static void st_init_exit(void)
{
	FIN(st_init_exit());
	fsm_printf("st_init_exit().\n");

	FOUT;
}


/* -----------------------
   functions executed under state:ST_IDLE
   ----------------------- */
static void st_idle_enter(void)
{
	FIN(st_idle_enter());
	fsm_printf("st_idle_enter().\n");
	static int flag_st_idle_normal = 0;
	/*
	if(flag_st_idle_normal == 0)
	{
		fsm_schedule_self(0, CODE_SEND_MIB); //test
		fsm_schedule_self(0, CODE_SEND_SIB1);
		fsm_schedule_self(0, CODE_SEND_SI);
		flag_st_idle_normal++;
	}
	*/
	//fsm_schedule_self(0, CODE_BEGIN_RECV_SYSINFO);       //test
	FOUT;
}
static void st_idle_exit(void)
{
	FIN(st_idle_exit());
	fsm_printf("st_idle_exit().\n");

	if(RRC_CLOSE)
	{
		rrc_close_enb();
	}
	FOUT;
}

static void st_idle_message_from_upper()
{
	FIN(st_idle_message_from_upper);
	FSM_PKT* pkptr = fsm_pkt_get();
	rrc_message_handler_upper(pkptr);

	FOUT;
}

static void st_idle_message_from_lower()
{
	FIN(st_idle_message_from_upper);
	FSM_PKT* pkptr = fsm_pkt_get();
	fsm_printf("[rrc] rrc receive a packet from rlc\n");
	fsm_octets_print(pkptr->data, 100);
	rrc_message_handler_lower(pkptr);

	FOUT;
}


static void st_idle_default(void)
{
	FIN(st_cell_sel_default());
	fsm_printf("st_cell_sel_default().\n");

	other_events_handler();

	FOUT;
}

static void rrc_send_mib(void)
{
	fsm_printf("[rrc][total]start to send MIB.\n");
//	Set_send_mib();
}
static void rrc_send_sib1(void)
{
//	Set_send_sib1();
}
static void rrc_send_si(void)
{
//	Set_send_si();
}
static void rrc_send_paging(void)
{
//	Set_send_paging();
}

static do_receive_rnti_ind(void)
{
	unsigned short *crnti_ptr;
	unsigned short crnti;
	int uefsmid;
	crnti_ptr = (unsigned short *) fsm_data_get();
	
	crnti = *crnti_ptr;
	uefsmid = crnti_to_uefsmid(crnti);
	if(uefsmid == -1)
	{
		fsm_printf("[rrc] building a new fsm!\n");

		if(new_ue_fsm(crnti) != -1)
		{
			//fsm_do_ioctrl(STRM_TO_RLCMAC, IOCCMD_RRCTOMAC_UEFSM_BUILD_SUCCESS, &(ioctl_msg->crnti), sizeof(unsigned short));
		
		}			
	}
	
	fsm_data_destroy(crnti_ptr);
}
