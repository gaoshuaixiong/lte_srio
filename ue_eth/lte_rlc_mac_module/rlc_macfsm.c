#include <linux/if_ether.h>
#include <linux/kernel.h>
#include <stddef.h>

#include "rlc_macfsm.h"
#include "macpkfmt.h"
#include "macscheduler.h"
#include "rrc_config.h"
#include "../lte_system.h"
#include "../pkfmt.h"
#include "Rlcprimitives.h"

//testing functions 测试函数   HQ 
#define IOCCMD_TEST_lostUL  0x30 //测试TA_timer超时
#define NCRATEST 0x31

//int ul_sched_count = 0;

/*****************       mac层函数声明    **************************/
/*funcation declaration 函数声明 */
//static void init_enter(void);   
//static void ue_mac_close(void);
static void init_enter(void);
static void ue_mac_sv_init(void);//初始化状态变量
static void ue_mac_sv_close(void);
static void defaultconfig(void);//默认配置
static void macmain_defaultconfig(void);
static void sps_defaultconfig(void);
static void CCCH_defaultconfig(void);

static void ue_mac_reset(void);
static void ue_mac_rach_config(void);
static void ue_mac_main_config(void);
static void ue_mac_logCh_config(void);
static void ue_mac_PCCH_config(void);
static void ue_mac_BCCH_config(void);
static void ue_mac_SPS_Config(void);
static void TATimer_config(void);
static void ue_mac_lc_release(void);

static void idle_default_handler(void);
static void cra_default_handler(void);
static void ncra_default_handler(void);
static void con_default_handler(void);
static void sendmsg3_default_handler(void);

/****random access processure  __Chen Lang *******/
static unsigned char RandomlySelectRaPreamble(void);
static void StartWaitingForRaResponse(void);
static void SendRaPreamble(RACH_ConfigDedicated *randomaccess_info);
static void doStartNonContentionBasedRandomAccessProcedure();
static void doStartContentionBasedRandomAccessProcedure();
static void NC_RaResponseFailed(void);
static void C_RaResponseFailed(void);
static void cleanharqBuff(void);
static void ContentionFailed(void);
static void waitsending_msg3(void);//defined by HQ 20140430 
void send_msg3(FSM_PKT *skb); //defined by HQ 20140430 
static void retransmit_msg3(void); 	//. 20150901
static void cra_success(void);
static void ncra_success(void);
//static void randomaccess_success(void);

static void RARTA_handleA(void);
static void RARTA_handleB(void);

//handle timer expiry 定时器超时事件的处理
static void handle_timeAlignmentTimerExpire(void);//上行失步，timeAlignmentTimer超时
static void handle_PeriodicBSRTimerExpire(void);//处理周期BSR定时器超时
static void handle_RetxBSRTimerExpire(void);//处理重传BSR定时器超时
static void handle_CRTimerExpire(void);
//Handle IOCTRL Command 
static void ioctrl_handler(void);
static void reports_handler(void);//处理上报事件，BSR PHR 
//static void handle_SR_Fail(void); //申明了但未定义
static void handle_HARQ_feedback(void);
static void send_SR(void);

void print_tran_info( const char *str);
void random_test(void); 
void testdoStartContentionBasedRandomAccessProcedure(void);
void testdoStartNonContentionBasedRandomAccessProcedure(void);
void RA_RRCconfig_testingfun(void); // 测试用的函数，获得RRC的配置信息 
void CRAtoCON_testingfun(void);
void NCRAtoCON_testingfun(void);
void stop_WRAP_timer(void);
void stop_CR_timer(void);
void stop_RetxBSR_timer();
void stop_TA_timer();
void stop_PeriodicBSR_timer();
void test1_idle_default_handler(void);
void test2_idle_default_handler(void);
static void test_init_logicalchannel(u16 lcid);
static void test_print_pkt_num();

static void cra_pkt_recv_from_upper(void);
static void skb_from_cra_to_con(FSM_PKT * skb);
static void do_bsr_and_resource_alloc();

/*****************       rlc层函数声明    **************************/
//测试用
extern void Am_instance_add(void);
extern void UM_instance_bulid(void);
extern void addpkt_recvData(struct AM_Instance *amIns);   
extern void addPollRetxData(struct AM_Instance*amIns);
extern void skb_printf(FSM_PKT *pkt);
extern void buff_printf(struct AM_Instance *amIns);
extern void add_data_retx(struct AM_Instance*amIns, u8 lcid);
//static void rlc_pkt_receive_from_mac(FSM_PKT * skb);

/*****************  CRA Queue handle function **************************/
static void InitQueue(CraQueuePtr *Q);
static bool QueueEmpty(CraQueuePtr Q);
static void EnQueue(CraQueuePtr *Q, FSM_PKT *pktptr);
static FSM_PKT * DeQueue(CraQueuePtr *Q);
static u16 GetCountOfQueue(CraQueuePtr Q);


/**********************************************************
**Function name: rlc_mac_main()
**Descriptions: the main frame of the FSM
**Input: 
**Output:
**Returns:
**Created by: Liu Xiru
**Created Date: 2015
**-----------------------------------------------------
**Modified by: 
**Modified Date: 
**********************************************************/
void rlc_mac_main(void)
{
	FSM_ENTER(rlc_mac_main);
	FSM_BLOCK_SWITCH
	{
		FSM_STATE_FORCED(ST_INIT, "INIT", init_enter(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "INIT -> IDLE");
		}
		FSM_STATE_FORCED(ST_RLC_INIT, "RLC_INIT", rlc_init_enter(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "RLC_INIT -> IDLE"); 
			//add RLC_INTI state, transmit to IDLE, 20150803
		}
		FSM_STATE_FORCED(ST_MAC_INIT, "MAC_INIT", ue_mac_sv_init(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "MAC_INIT -> IDLE");
		}
		FSM_STATE_UNFORCED(ST_IDLE, "IDLE", , )
		{
			FSM_COND_TEST_IN("IDLE")				
				FSM_TEST_COND(START_C_RANDOM_ACCESS)
				FSM_TEST_COND(START_NC_RANDOM_ACCESS)
				FSM_TEST_COND(MAC_RESET)
				
				FSM_TEST_COND(RLC_SUSPEND)
				FSM_TEST_COND(RLC_DEACT)
				FSM_TEST_COND(RLC_RESUME)
			FSM_COND_TEST_OUT("IDLE")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_CRA, doStartContentionBasedRandomAccessProcedure(), "IDLE -> CRA")
				FSM_CASE_TRANSIT(1, ST_NCRA, doStartNonContentionBasedRandomAccessProcedure(), "IDLE -> NCRA")
				//FSM_CASE_TRANSIT(2, ST_IDLE, print_tran_info("reset IDLE -> IDLE"), "IDLE -> IDLE")
				FSM_CASE_TRANSIT(2, ST_IDLE, ue_mac_reset(), "IDLE -> IDLE")
				FSM_CASE_TRANSIT(3,ST_SUSPEND_IDLE,  ,"IDLE-> SUSPEND_IDLE")	//modify status in 20150826
				FSM_CASE_TRANSIT(4,ST_NULL_IDLE,  , "IDLE-> NULL_IDLE")
				FSM_CASE_TRANSIT(5,ST_RLC_INIT,  , "IDLE-> RLC_INIT")
				FSM_CASE_DEFAULT(ST_IDLE,idle_default_handler() , "IDLE->IDLE")	//transit to idle state	by default.
			}
		}
		FSM_STATE_UNFORCED(ST_CRA,"CRA", ,)
		{
			FSM_COND_TEST_IN("CRA")
				FSM_TEST_COND(TimeAlignmentTimerExpire)
				FSM_TEST_COND(RANDOM_ACCESS_FAIL)
				FSM_TEST_COND(RARREV_SUCCESS)
				FSM_TEST_COND(CONTENTION_FAIL)
				FSM_TEST_COND(CONTENTION_SUCCESS)
				FSM_TEST_COND(MAC_RESET)
			FSM_COND_TEST_OUT("CRA")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE,handle_timeAlignmentTimerExpire(), "CRA-> IDLE")
				FSM_CASE_TRANSIT(1, ST_IDLE, C_RaResponseFailed(), "CRA -> IDLE")
				//FSM_CASE_TRANSIT(2, ST_SENDMSG3 , waitsending_msg3(), "CRA -> SENDMSG3")
				FSM_CASE_TRANSIT(2, ST_CRA , waitsending_msg3(), "CRA -> CRA")	//cancel Sendmsg3 state.20150831
				FSM_CASE_TRANSIT(3, ST_IDLE, ContentionFailed(), "CRA -> IDLE")
				FSM_CASE_TRANSIT(4, ST_CON , cra_success() , "CRA -> CON")
				//FSM_CASE_TRANSIT(4, ST_IDLE, print_tran_info("reset CRA -> IDLE"), "CRA -> IDLE")
				FSM_CASE_TRANSIT(5, ST_IDLE, ue_mac_reset(), "CRA -> IDLE")
				FSM_CASE_DEFAULT(ST_CRA,cra_default_handler() , "CRA->CRA")	//transit to CRA state	by default.
			}
		}
		/*FSM_STATE_UNFORCED(ST_SENDMSG3,"SENDMSG3", ,)
		{
			FSM_COND_TEST_IN("SENDMSG3")
				//FSM_TEST_COND(HAVE_SENDMSG3) //testing code 
				FSM_TEST_COND(TimeAlignmentTimerExpire)
				FSM_TEST_COND(MAC_PK_FROM_UPPER)//LHL 20140726
				FSM_TEST_COND(RA_SEND_MSG3)//LHL 20140726
				FSM_TEST_COND(MAC_RESET)
			FSM_COND_TEST_OUT("SENDMSG3")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE,handle_timeAlignmentTimerExpire(), "SENDMSG3-> IDLE")
				FSM_CASE_TRANSIT(1, ST_CRA, send_msg3(), "SENDMSG3 -> CRA")
				FSM_CASE_TRANSIT(2, ST_CRA, send_msg3(), "SENDMSG3 -> CRA")//跳到con状态
				//FSM_CASE_TRANSIT(1, ST_IDLE, print_tran_info("reset SENDMSG3 -> IDLE"), "SENDMSG3 -> IDLE")
				FSM_CASE_TRANSIT(3, ST_IDLE,ue_mac_reset(), "SENDMSG3 -> IDLE")
				FSM_CASE_DEFAULT(ST_SENDMSG3,sendmsg3_default_handler() , "SENDMSG3->SENDMSG3")	//transit to SENDMSG3 state	by default.
			}
		}*/		
		FSM_STATE_UNFORCED(ST_NCRA,"NCRA", , )
		{
			FSM_COND_TEST_IN("NCRA")
				FSM_TEST_COND(TimeAlignmentTimerExpire)
				FSM_TEST_COND(RANDOM_ACCESS_FAIL)
				FSM_TEST_COND(RARREV_SUCCESS)
				FSM_TEST_COND(MAC_RESET)
			FSM_COND_TEST_OUT("NCRA")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE,handle_timeAlignmentTimerExpire(), "NCRA-> IDLE")
				FSM_CASE_TRANSIT(1, ST_IDLE,NC_RaResponseFailed(), "NCRA -> IDLE")
				FSM_CASE_TRANSIT(2, ST_CON, ncra_success(), "NCRA -> CON")
				// FSM_CASE_TRANSIT(2, ST_IDLE,print_tran_info("reset NCRA -> IDLE"), "NCRA -> IDLE")
				FSM_CASE_TRANSIT(3, ST_IDLE,ue_mac_reset(), "NCRA -> IDLE")
				FSM_CASE_DEFAULT(ST_NCRA,ncra_default_handler() , "NCRA -> NCRA")	//transit to NCRA state	by default.
			}
		}
		FSM_STATE_UNFORCED(ST_CON,"CON", , )		//写到这里了20140320
		{
			FSM_COND_TEST_IN("CON")
				FSM_TEST_COND(TimeAlignmentTimerExpire)
				FSM_TEST_COND(SR_FAIL)
				FSM_TEST_COND(MAC_RESET)

				FSM_TEST_COND(RLC_SUSPEND)
				FSM_TEST_COND(RLC_DEACT)
				FSM_TEST_COND(RLC_RESUME)
			FSM_COND_TEST_OUT("CON")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE,handle_timeAlignmentTimerExpire(), "CON -> IDLE")//handle_timeAlignmentTimerExpire()
				FSM_CASE_TRANSIT(1, ST_CRA, doStartContentionBasedRandomAccessProcedure() , "CON -> CRA")
				//FSM_CASE_TRANSIT(2, ST_IDLE,print_tran_info("reset CON -> IDLE"), "CON -> IDLE")
				FSM_CASE_TRANSIT(2, ST_IDLE,ue_mac_reset(), "CON -> IDLE")

				FSM_CASE_TRANSIT(3,ST_SUSPEND,  ,"IDLE-> SUSPEND")
				FSM_CASE_TRANSIT(4,ST_NULL,  , "IDLE-> NULL")
				FSM_CASE_TRANSIT(5,ST_RLC_INIT,  , "IDLE-> RLC_INIT")
				FSM_CASE_DEFAULT(ST_CON,con_default_handler() , "CON -> CON")	//transit to CON state	by default.
			}
		}
		FSM_STATE_FORCED(ST_SUSPEND, "SUSPEND", , )
		{
			FSM_TRANSIT_FORCE(ST_CON, , "default", "", "SUSPEND -> CON");
		}
		FSM_STATE_FORCED(ST_NULL, "NULL", , )
		{
			FSM_TRANSIT_FORCE(ST_CON, , "default", "", "NULL -> CON");
		}
		
		//add 2 new status ST_SUSPEND_IDLE and ST_NULL_IDLE, 20150826
		FSM_STATE_FORCED(ST_SUSPEND_IDLE, "SUSPEND_IDLE", , )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "SUSPEND_IDLE -> IDLE");
		}
		FSM_STATE_FORCED(ST_NULL_IDLE, "NULL_IDLE", , )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "NULL_IDLE -> IDLE");
		}
	}
	FSM_EXIT(0)
}

static void InitQueue(CraQueuePtr *Q)
{
	*Q = (CraQueue *)fsm_mem_alloc(sizeof(CraQueue));
	(*Q)->front = (*Q)->rear = (QueueNode *)fsm_mem_alloc(sizeof(QueueNode));
	// Q->front = NULL;
	// Q->rear = NULL;
	(*Q)->pkt_count = 0;
	fsm_printf("[InitQueue]Q point: %p\n", *Q);
}

static bool QueueEmpty(CraQueuePtr Q)
{
	//return (Q->front == NULL) && (Q->rear == NULL);
	return Q->front == Q->rear;		//because of the head node existence, once front==rear represent the queue empty.
}

static void EnQueue(CraQueuePtr *Q, FSM_PKT *pktptr)
{
	QueueNode *p = (QueueNode *)fsm_mem_alloc(sizeof(QueueNode));
	p->pkt = pktptr;
	p->next = NULL;

	// if(QueueEmpty(Q))
	// 	(*Q)->front = (*Q)->rear = p;		//insert node p into a empty queue
	// else{
	// 	(*Q)->rear->next = p;
	// 	(*Q)->rear = p;
	// }
	(*Q)->rear->next = p;
	(*Q)->rear = p;
	(*Q)->pkt_count++;
	fsm_printf("[EnQueue]EnQueue successfully!\n");
	//fsm_printf("[EnQueue]Q point: %p\n", *Q);
}

//the first node is head node, so always take the second node out.
static FSM_PKT * DeQueue(CraQueuePtr *Q)
{
	FSM_PKT *pktptr;
	QueueNode *p;
	if(QueueEmpty(Q))
	{
		fsm_printf("[DeQueue]Queue is empty!\n");
		return NULL;
	}
	// p = (*Q)->front;
	// pktptr = p->pkt;
	// (*Q)->front = p->next;
	// if((*Q)->rear == p)
	// 	(*Q)->rear = NULL;
	p = (*Q)->front->next;
	pktptr = p->pkt;
	if((*Q)->rear == p)
	{
		(*Q)->rear = (*Q)->front;	//there are one pkt in the queue, after dequeue, queue empty.
		(*Q)->front->next = NULL;
	}
	else
		(*Q)->front->next = p->next;	//more than 2 pkt in the queue.

	fsm_mem_free(p);
	(*Q)->pkt_count--;
	fsm_printf("[DeQueue]DeQueue successfully!\n");

	return pktptr;
}

static u16 GetCountOfQueue(CraQueuePtr Q)
{
	return Q->pkt_count;
}

/********************************************************************
**Function name: cra_pkt_recv_from_upper()
**Descriptions: Receive packets from RRC or PDCP layer,add it into CRA queue
**Input: 
**Output: Receive packets from RRC or PDCP layer,add it into CRA queue
**Returns:
**Created by: Liu xiru
**Created Date: 2015
**--------------------------------------------------------------------
**Modified by: 
**Modified Date: 
*********************************************************************/
static void cra_pkt_recv_from_upper(void)
{
	FSM_PKT *skb;

	FIN(cra_pkt_recv_from_upper());
	SV_PTR_GET(rlc_mac_sv);
	
	skb = (FSM_PKT *)fsm_pkt_get();
	fsm_printf("[RLC_MAC][cra_pkt_recv_from_upper]recv from upper\n");
	fsm_octets_print(skb->data, 64);

	EnQueue(&SV(CraQueue), skb);
					
	//fsm_printf("[RLC][cra_pkt_recv_from_upper]add new element to CraQueue done!\n");
	//fsm_printf("[cra_pkt_recv_from_upper]SV(CraQueue): %p\n", SV(CraQueue));

	int pktnum = GetCountOfQueue(SV(CraQueue));
	fsm_printf("[RLC_MAC][cra_pkt_recv_from_upper]current num of pkt is %d\n", pktnum);

}

/********************************************************************
**Function name: do_bsr_and_resource_alloc()
**Descriptions: do bsr and resource allocation.
**Input: 
**Output: 
**Returns:
**Created by: Liu xiru
**Created Date: 2015
**--------------------------------------------------------------------
**Modified by: 
**Modified Date: 
*********************************************************************/
static void do_bsr_and_resource_alloc()
{
	char *tmpdata_ptr;
	int tmpdata;

	FIN(do_bsr_and_resource_alloc());
	SV_PTR_GET(rlc_mac_sv);

	if( PMRLC_BUFFERREP_REQ() == NULL )
	{
		fsm_printf("[RLC_MAC][do_bsr_and_resource_alloc]: the return pointer of PMRLC_BUFFERREP_REQ is NULL!\n");
		FOUT;
	}		
	else{
		tmpdata_ptr = (char *)PMRLC_BUFFERREP_REQ();
		tmpdata = *((u32*)tmpdata_ptr);//取出指示数组长度的信息	
		if( tmpdata > 0 )
		{
			DoRefreshRLCBuffserRequest(RlcRequest_arraytolist((RLC_Request *)(tmpdata_ptr+sizeof(u32)),tmpdata));//更新待发数据缓存信息
			fsm_data_destroy((void *)tmpdata_ptr);//释放内存
			if(SV(LCGZeroToData)== true)//HQ201407040添加 判断是否触发regularBSR
				//RefreshBufferStatusForBSR();
				DoProduceBsr_LCGZeroToData();//该函数执行完后，SV(LCGZeroToData)置为false
		}
	}

	SV(UL_resource_info.resource_flag) = true;
		/********for test 20150916 *********/
	if(SV(UL_resource_info.resource_flag) == true)
	{
		//GetTBsize_Allocation(SV(UL_resource_info.m_tbsize));//返回tbsize,单位byte,考虑了发送BSR的资源
		SV(UL_resource_info.m_tbsize)=1470*8;//for test
		SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
		//fsm_printf("ready resource \n");
		if(! (list_empty(&(SV(MacBuffer_RLC)->list))))
			DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)));  //开始资源分配过程
		SV(UL_resource_info.resource_flag)=false;
		//fsm_printf("start UL Schedule \n");
	}
	else
		send_SR();
	FOUT;
}

static void init_enter()
{
	FIN(init_enter());
	ue_mac_sv_init();
	rlc_init_enter();
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:Initialization parameters
->Change:
->Details: 初始化 以及默认配置
->input:
->output:初始化全局变量
->Special:
*******************************
*/
static void ue_mac_sv_init()
{
	FIN(ue_mac_sv_init());
	int len;
	
	SV_PTR_GET(rlc_mac_sv);
	print_tran_info("FSM INIT-init SV");//testing code
	SV(TA_Periodic_time)=800000000;//testing code ,实际应为一个默认值，默认值待定
	Init_Uemac_Scheduler();
	SV(sys_frame.frameNo)=0;//modified LHL 20141105初始化系统帧号子帧号
	SV(sys_frame.subframeNo)=0;
	SV(recv_frame).subframeNo=0;//清空
	SV(recv_frame).frameNo=0;
	SV(ue_schedule_frame.frameNo)=0;
	SV(ue_schedule_frame.subframeNo)=0;
	//SV(flag123)=0;//LHL test 0924
	//SV(TA_fromRAR)=0;
	SV(TA_value)=0;//LHL 20141021
	SV(m_temp_cr)=(temp_contention_resolution*)fsm_mem_alloc(sizeof(temp_contention_resolution));
	//初始化各个定时器
	
	SV(TATimer.time_value) = 0;
	SV(TATimer.timer_sign) = NULL;
	SV(TATimer.flag) = false;
	SV(CRTimer.time_value) = 0;
	SV(CRTimer.timer_sign) = NULL;
	SV(CRTimer.flag) = false;
	SV(RetxBSRTimer.time_value) = 0;
	SV(RetxBSRTimer.timer_sign) = NULL;
	SV(RetxBSRTimer.flag) = false;
	SV(PeriodicBSRTimer.time_value) = 0;
	SV(PeriodicBSRTimer.timer_sign) = NULL;
	SV(PeriodicBSRTimer.flag) = false;	
	SV(WaitforRAresponseTimer.time_value) = 0;
	SV(WaitforRAresponseTimer.timer_sign) = NULL;
	SV(WaitforRAresponseTimer.flag) = false;
	SV(msg3_buf_ptr) = NULL;	
	SV(ra_allocation_res)=0;
	defaultconfig();//完成一些默认配置
	RA_RRCconfig_testingfun(); // 测试随机接入过程所用的函数，获得RRC的配置信息
	SV(UL_resource_info.resource_flag)= false; //testing code
	//SV(UL_resource_info.m_tbsize)= 2400;//单位应该为bit//testing code 
	//SV(C_RNTI) = 11;
	//初始化其他全局变量
	fsm_mem_set(&SV(ratype),0,sizeof(RA_Type));
	SV(sendmsg3)=0;//0:没发送MSG3，1:发送MSG3
	SV(pdcch_rnti)=0;//0:PDCCH不是通过C_RNTI寻址，1:是通过C_RNTI寻址
	SV(Tbsize_Complex)=0;//modified by lhl ,20140718,to 复用模块，可以发的总数据量
	//test_init_logicalchannel(1);//TEST 20141103
	//test_init_logicalchannel(3);//TEST 20141103
	SV(contention_failed_num)=0;
	SV(rar_failed_num)=0;
	
	SV(lost_pkt_num_uplink)=0;
	SV(pkt_num_uplink)=0;
	SV(lost_pkt_num_downlink)=0;
	SV(pkt_num_downlink)=0;
	
	SV(countSendToUpper)=0;
	SV(countRecvFromLower)=0;
	SV(countreceive_from_mac)=0;

	InitQueue(&SV(CraQueue));	//add cra queue initialization by LXR in 20151020
	fsm_printf("[ue_mac_sv_init]SV(CraQueue): %p\n", SV(CraQueue));
	
	FOUT;
}

//TEST FUNCTION20141103
static void test_init_logicalchannel(u16 lcid)
{
	FIN(test_init_logicalchannel());
	SV_PTR_GET(rlc_mac_sv);
	MAC_LogicalChannelConfig_IoctrlMsg *lc_info=(MAC_LogicalChannelConfig_IoctrlMsg *)fsm_mem_alloc(sizeof(MAC_LogicalChannelConfig_IoctrlMsg));
	lc_info->logicalChannelIdentity=lcid;
	lc_info->logicalChannelConfig.haveUl_SpecificParameters=true;
	lc_info->logicalChannelConfig.ul_SpecificParameters.priority=2;
	lc_info->logicalChannelConfig.ul_SpecificParameters.prioritisedBitRate=kBps128;
	lc_info->logicalChannelConfig.ul_SpecificParameters.logicalChannelGroup=1;	
	Init_LogicalChannel_ConfigInfo(lc_info); 
	fsm_mem_free(lc_info);//相应释放内存
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:状态机关闭时释放资源
->Change:
->Details:
->input:
->output:释放内存
->Special:
*******************************
*/
static void ue_mac_sv_close()
{
	FIN(ue_mac_sv_close());
	SV_PTR_GET(rlc_mac_sv);
	if(SV(m_temp_cr) != NULL)
	{
		fsm_mem_free(SV(m_temp_cr));
		SV(m_temp_cr) = NULL;
	}
	Free_Uemac_Scheduler_Resource();
	if(SV(msg3_buf_ptr)!=NULL)
	{
		fsm_pkt_destroy(SV(msg3_buf_ptr));
		SV(msg3_buf_ptr)=NULL;
	}
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:Initialization parameters
->Change:
->Details:默认配置，主要是开机时的配置
->input:
->output:MAC的主要默认配置(SPS、CCCH、系统配置)
->Special:
*******************************
*/
static void defaultconfig()
{
	FIN(defaultconfig());
	macmain_defaultconfig();
	sps_defaultconfig();
	CCCH_defaultconfig();//CCCH信道默认配置
	//fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);	//for test
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:MAC主要默认配置
->Change:
->Details:
->input:
->output:对systemconfigInfo.MACMainConfigInfo的配置
->Special:
*******************************
*/
static void macmain_defaultconfig()
{
	FIN(macmain_defaultconfig());
	SV_PTR_GET(rlc_mac_sv);
	SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.maxHARQ_Tx) = maxHARQ_Tx_n5;
	//fsm_printf("test maxHARQTX:%d \n ",SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.maxHARQ_Tx));//testing code
	SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.periodicBSR_Timer)= periodicBSR_Timer_infinity;//该参数值表示该定时器不启用
	SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.retxBSR_Timer)= retxBSR_Timer_sf2560 ;
	SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.ttiBundling)= false ;
	SV(systemconfigInfo.MACMainConfigInfo.drx_Config.choice.setup.haveShortDRX)= false ;//means release
	SV(systemconfigInfo.MACMainConfigInfo.type)= 1 ; //means phr-Config release ??  
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:SPS默认配置
->Change:
->Details:
->input:
->output:对systemconfigInfo.SPSConfigInfo的配置
->Special:
*******************************
*/
static void sps_defaultconfig()
{
	FIN(sps_defaultconfig());
	SV_PTR_GET(rlc_mac_sv);
	SV(systemconfigInfo.SPSConfigInfo.haveSPS_ConfigDL)=false;
	SV(systemconfigInfo.SPSConfigInfo.haveSPS_ConfigUL)=false; 
FOUT;
}
/*
******************************
->Change Data and time: 
->Function:CCCH默认配置
->Change:
->Details:
->input:
->output:对systemconfigInfo.CCCHConfigInfo的配置
->Special:
*******************************
*/
static void CCCH_defaultconfig()
{
	FIN(CCCH_defaultconfig());
	SV_PTR_GET(rlc_mac_sv);
	MAC_LogicalChannelConfig_IoctrlMsg *ccch_info=(MAC_LogicalChannelConfig_IoctrlMsg *)fsm_mem_alloc(sizeof(MAC_LogicalChannelConfig_IoctrlMsg));
	SV(systemconfigInfo.CCCHConfigInfo.logicalChannelIdentity)=0;
	SV(systemconfigInfo.CCCHConfigInfo.logicalChannelConfig.haveUl_SpecificParameters)=true;
	SV(systemconfigInfo.CCCHConfigInfo.logicalChannelConfig.ul_SpecificParameters.priority)=1;
	SV(systemconfigInfo.CCCHConfigInfo.logicalChannelConfig.ul_SpecificParameters.prioritisedBitRate)= infinity;
	SV(systemconfigInfo.CCCHConfigInfo.logicalChannelConfig.ul_SpecificParameters.logicalChannelGroup)= 0;
	fsm_mem_cpy(ccch_info, &SV(systemconfigInfo.CCCHConfigInfo),sizeof(MAC_LogicalChannelConfig_IoctrlMsg));  
	Init_LogicalChannel_ConfigInfo(ccch_info); 
	fsm_mem_free(ccch_info);//相应释放内存
	fsm_printf("[UEMAC][init]CCCH_defaultconfig completed\n");
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:IDLE状态下的出口函数
->input:
->output:
->Special:
*******************************
*/
static void idle_exit()
{
	FIN(idle_exit());
	FOUT;
}

/***** HuangQiong *****/
/*
******************************
->Change Data and time: 20140411;20141105
->Function: IDLE状态的默认转移的处理函数
->Change:增加BJ的定时更新事件
->Details:主要处理状态机事件、IOCTL事件，跳转到不同的状态
->input:
->output:
->Special:
*******************************
*/
static void idle_default_handler()
{
	RLC_Request rlc_request_array[1];
	char *tmpdata_ptr;
	int tmpdata;
	FSM_PKT* pkptr;//test
	//rlc varibles 20150804
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	void *insPtr;
	int i;

	FIN(idle_default_handler());
	SV_PTR_GET(rlc_mac_sv);
	//print_tran_info("[UEMAC][idle_default]MAC idle_default_handler");//testing code

	//fsm_printf("[RLC_MAC][idle_default_handler][begin]fsm event type: %d, code: %x\n", fsm_ev_type(),fsm_ev_code());
	if(MAC_CLOSE)
	{
		print_tran_info("[UEMAC][idle_default]MAC FSM is closed ");//testing code
		ue_mac_sv_close();
		FOUT;
	}
	else if(RLC_CLOSE)                              //模块关闭
	{
		rlc_close();
		FOUT;
	}
	else if(MAC_PK_FROM_LOWER)
	{//通过ICI信息判断包类型，应该是广播消息，调用发送函数，发给RLC
		print_tran_info("[UEMAC][idle_default][-->]MAC IDLE receive a data packet from PHYadapter\n"); //testing code
		//fsm_pkt_destroy(fsm_pkt_get());
		if(SV(CRTimer.flag)==true)
			fsm_printf("[UEMAC][idle_default]SV(CRTimer.flag)=true\n");//testing code 
		else
			fsm_printf("[UEMAC][idle_default]SV(CRTimer.flag)=false\n");//testing code 	 
		/*if(SV(TATimer.timer_sign)==NULL)
		{
			//由上行失步引起的IDLE态
			fsm_pkt_destroy(fsm_pkt_get());
		}
		else*/
		decomplexUeDataPdu(fsm_pkt_get()); //SRIO发一个包过来，这里会死机，不知道为什么? 20140701
		SV(countRecvFromLower)++;
		//FOUT;
	}
	else if(RLC_PKT_FROM_UPPER)		//接收来自上层的包
	{
		SV(countRecvFromUpper)++;
		rlc_pkt_receive_from_upper();
		
		//FOUT;
		//add in 20150908
		if( PMRLC_BUFFERREP_REQ() == NULL )
			fsm_printf("[RLC_MAC]: the return pointer of PMRLC_BUFFERREP_REQ is NULL!\n");
		else{
			tmpdata_ptr = (char *)PMRLC_BUFFERREP_REQ();
			tmpdata = *((u32*)tmpdata_ptr);//取出指示数组长度的信息	
			if( tmpdata > 0 )
			{
				fsm_printf("[UEMAC][idle_default]MAC IDLE receive a RLCdatareq,");//testing code
				//fsm_printf("[UEMAC][idle_default]tmpdata:%d ",tmpdata);//testing code
				fsm_printf("[UEMAC][idle_default]txQueueSize:%d\n",((RLC_Request *)(tmpdata_ptr+sizeof(u32)))->txQueueSize);//testing code
				DoRefreshRLCBuffserRequest(RlcRequest_arraytolist((RLC_Request *)(tmpdata_ptr+sizeof(u32)),tmpdata));//更新待发数据缓存信息
				//tmpdata_ptr+sizeof(int)//向后移动4个字节 指向数组头
				fsm_data_destroy((void *)tmpdata_ptr);//释放内存
				if(SV(LCGZeroToData)== true)//HQ201407040添加 判断是否触发regularBSR
					DoProduceBsr_LCGZeroToData();//该函数执行完后，SV(LCGZeroToData)置为false	 	 
				//fsm_schedule_self(0,Start_ContentionBase_RandomAcc); //触发随机接入 ，时间单位是10us 这个时间设置多长待究
				fsm_schedule_self(1000,Start_ContentionBase_RandomAcc); //1000 for test
				SV(ratype.ra_type)=2;
			} 	
		}
		//FOUT;
	}
	else if(TimeAlignmentTimerExpire)
	{
		handle_timeAlignmentTimerExpire();
		FOUT;	//add in 20150910
	}
	else if(LogicalChannel_BJ)//modified 20141105 LHL,更新BJ
	{
		FlushBj();
		fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);
		FOUT;	//add in 20150910
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
				/*if(SV(isBufferReq) == 0)
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
				}*/
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
					/*if(!SV(bufferReqTimer))
					{
						SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
					}*/
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
		/*case BUFFER_REQ:
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
			break;*/
		default:
			printk("[RLC][rlc_interupt] cannot distinguish this code \n");
			break;
		}
		FOUT;
	}
	else if(IOCTRL_ARRIVAL)
	{//处理IOCTL命令
		//print_tran_info("[UEMAC][idle_default]MAC IDLE default ioctl hander,");//testing code
		u32 cmd1=fsm_ev_ioctrl_cmd();
		//fsm_printf("[MAC][DEFAULT_CMD]%d\n",cmd1);
		switch(fsm_ev_ioctrl_cmd())
		{
			//处理RLC的数据请求命令
			case IOCCMD_RLCtoMAC_datasend_Req :
				tmpdata_ptr = (char *)fsm_data_get();
				tmpdata = *((u32*)tmpdata_ptr);//取出指示数组长度的信息				
				fsm_printf("[UEMAC][idle_default]MAC IDLE receive a RLCdatareq,");//testing code
				fsm_printf("[UEMAC][idle_default]tmpdata:%d ",tmpdata);//testing code
				fsm_printf("[UEMAC][idle_default]txQueueSize:%d\n",((RLC_Request *)(tmpdata_ptr+sizeof(u32)))->txQueueSize);//testing code
				DoRefreshRLCBuffserRequest(RlcRequest_arraytolist((RLC_Request *)(tmpdata_ptr+sizeof(u32)),tmpdata));//更新待发数据缓存信息
				//tmpdata_ptr+sizeof(int)//向后移动4个字节 指向数组头
				fsm_data_destroy((void *)tmpdata_ptr);//释放内存
				if(SV(LCGZeroToData)== true)//HQ201407040添加 判断是否触发regularBSR
					DoProduceBsr_LCGZeroToData();//该函数执行完后，SV(LCGZeroToData)置为false	 	 
				//fsm_schedule_self(0,Start_ContentionBase_RandomAcc); //触发随机接入 ，时间单位是10us 这个时间设置多长待究
				fsm_schedule_self(1000,Start_ContentionBase_RandomAcc); //1000 for test
				SV(ratype.ra_type)=2;
			break;
			//处理PHY的RA请求命令
			case IOCCMD_PDCCHtoMAC_RandomAcc_Req: 	
			//两条命令的处理步骤一样 所以此处没有break	  
				tmpdata_ptr = fsm_data_get();
				print_tran_info("[UEMAC][idle_default]IDLE receive a RA req from other layer,");
				if(tmpdata_ptr > 0)//并指示了关于接入的前导码
				{
					fsm_printf("[UEMAC][idle_default]IDLE receive a ncra order ");
					fsm_mem_cpy(&(SV(RA_info)),tmpdata_ptr,sizeof(RACH_ConfigDedicated));
					fsm_printf("[UEMAC][idle_default]the RAPID is %d\n",SV(RA_info.ra_PreambleIndex));
					if(tmpdata_ptr != NULL)
					{
						fsm_data_destroy(tmpdata_ptr);//释放内存
						fsm_printf("[UEMAC][idle_default]fsm_ev_data,then free the memory\n");
					}
					fsm_schedule_self(0, Start_NonContentionBase_RandomAcc );//触发非竞争随机接入 //这条语句会造成死机，暂注释掉
				}
				else //没有指示关于接入的前导码
				{
					print_tran_info("[UEMAC][idle_default]IDLE receive a cra order ");
					SV(ratype.ra_type)=1;
					fsm_schedule_self(0, Start_ContentionBase_RandomAcc );//触发竞争随机接入
				}
			break;
			//处理RRC的RA请求命令
			case IOCCMD_RRCtoMAC_RandomAcc_Req :
				tmpdata_ptr = fsm_data_get();
				print_tran_info("[UEMAC][idle_default]IDLE receive a RA req from other layer,");
				if(tmpdata_ptr > 0)//并指示了关于接入的前导码
				{
					fsm_printf("[UEMAC][idle_default]IDLE receive a ncra order ");
					fsm_mem_cpy(&(SV(RA_info)),tmpdata_ptr,sizeof(RACH_ConfigDedicated));
					fsm_printf("[UEMAC][idle_default]the RAPID is %d\n",SV(RA_info.ra_PreambleIndex));
					if(tmpdata_ptr != NULL)
					{
						fsm_data_destroy(tmpdata_ptr);//释放内存
						fsm_printf("[UEMAC][idle_default]fsm_ev_data,then free the memory\n");
					}
					fsm_schedule_self(0, Start_NonContentionBase_RandomAcc );//触发非竞争随机接入 //这条语句会造成死机，暂注释掉
					//没有死机 正常 20140531HQ测试 
				}
				else //没有指示关于接入的前导码
				{
					print_tran_info("[UEMAC][idle_default]IDLE receive a cra order ");
					SV(ratype.ra_type)=3;
					fsm_schedule_self(0, Start_ContentionBase_RandomAcc );//触发竞争随机接入
				}
			break;
			//处理其他与状态无关的命令
			case IOCCMD_PHYtoMAC_SYSFRAME:
				tmpdata_ptr = fsm_data_get();
				SV(sys_frame.frameNo)=((system_frame *)tmpdata_ptr)->frameNo;
				SV(sys_frame.subframeNo)=((system_frame *)tmpdata_ptr)->subframeNo;
				fsm_data_destroy(tmpdata_ptr);
			break;
			default:
				ioctrl_handler();
			break;
		}
		reports_handler();//处理BSR PHR等上报事件 
	}
	else
	{
		printk("[RLC][rlc_interupt] cannot distinguish the interrupt\n");
	}
	//fsm_printf("[RLC_MAC][idle_default_handler][end]fsm event type: %d, code: %x\n", fsm_ev_type(),fsm_ev_code());
	FOUT;
}


/*
******************************
->Change Data and time: 20140411；20141105
->Function: CRA状态的默认转移的处理函数
->Change:增加BJ的定时更新事件
->Details:主要处理状态机事件、IOCTL事件，跳转到不同的状态
->input:
->output:
->Special:
*******************************
*/
static void cra_default_handler()
{
	void *tmpdata_ptr;
	u32 tmpdata;
	FSM_PKT* skb;
	FIN(cra_default_handler());
	SV_PTR_GET(rlc_mac_sv);

	char *pdu_type = (char*)fsm_mem_alloc(sizeof(char));  //pdu_type : 0 -> Common Data PDU ; 1~3 -> RAR PDU
	fsm_mem_set(pdu_type,0,sizeof(char));

	//print_tran_info("[UEMAC][cra_default]cra default ");//testing code
	if(SV(sendmsg3)==1 && SV(C_RNTI)!=0)
		fsm_schedule_self(0, Contention_Success );//TEST  
	if(MAC_CLOSE)
	{
		print_tran_info("[UEMAC][cra_default]MAC FSM is closed "); //testing code
		ue_mac_sv_close();
		FOUT;
	}
	else if(MAC_PK_FROM_LOWER)
	{
		//HQ make note 20140710
		//如果是DL_SCH 普通PDU
		//为了判断竞争解决，这里需要增加对于PHY上报的RNTI和UE自己的C_RNTI(如果存在) 进行对比 		
		//decomplexUeDataPdu(fsm_pkt_get());
		/*******************************************/
		skb=fsm_pkt_get();
		fsm_mem_cpy(pdu_type, skb->data,sizeof(char));
		*pdu_type=(*pdu_type)>>6;
		printk("[UE MAC CRA]mac_pkthandler decomplexUeDataPdu: pdu_type=%d\n",(*pdu_type));
		if(SV(sendmsg3)==0){
			if( (*pdu_type) > 0 && (*pdu_type) <= 3){  
				decomplexUeRARPdu(skb);
				fsm_mem_free(pdu_type);
				printk("[UE MAC CRA]mac_pkthandler decomplexRARdu: This is a RAR PDU !!!\n");
				FOUT;
			}else{
				fsm_pkt_destroy(skb);
				fsm_mem_free(pdu_type);
				printk("[UE MAC CRA]mac_pkthandler decomplexUeDataPdu: This is a Common DATA PDU ,Error!!!\n");
				FOUT;
			}
		}	
		if(SV(sendmsg3)==1){
			if( (*pdu_type) == 0){  
				decomplexUeDataPdu(skb);
				fsm_mem_free(pdu_type);
				printk("[UE MAC CRA]mac_pkthandler decomplexRARdu: This is a Common DATA PDU !!!\n");
				FOUT;
			}else{
				fsm_pkt_destroy(skb);
				fsm_mem_free(pdu_type);
				printk("[UE MAC CRA]mac_pkthandler decomplexUeDataPdu: This is a other PDU !!!\n");
				FOUT;
			}
		}
		/**********************************************/
		/*if(SV(sendmsg3)==0)
		{
			decomplexUeRARPdu(fsm_pkt_get());	//modify by lxr in 20160122
			fsm_printf("[RLC_MAC][cra_default_handler]decomplex RAR Pdu\n");
			FOUT;
		}
		if(SV(sendmsg3)==1)
		{
			decomplexUeDataPdu(fsm_pkt_get());
			fsm_printf("[RLC_MAC][cra_default_handler]decomplex data Pdu\n");
			FOUT;
		}*/
		//FOUT;
	}
	/*if(MAC_PK_FROM_UPPER)
	{
		fsm_pkt_destroy(fsm_pkt_get());
		fsm_printf("[UEMAC][cra_default]receive packet from RLC,error! \n");
	}*/	//noted in 20150908
	else if(RLC_PKT_FROM_UPPER)	//add in 20151020 by LXR
	{
		cra_pkt_recv_from_upper();
		fsm_printf("[RLC_MAC][cra_default_handler]receive pkt from upper!\n");
		FOUT;
	}
	else if(LogicalChannel_BJ)//modified 20141105 LHL,更新BJ
	{
		FlushBj();
		fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);
		FOUT;
	}
	else if(IOCTRL_ARRIVAL)
	{
		switch(fsm_ev_ioctrl_cmd())
		{
			//处理RLC的数据请求命令
			case IOCCMD_RLCtoMAC_datasend_Req ://20150106处理在连接状态下发起随机接入后，只发送了MSG3后，没有MSG4
			//又收到了data request的情况
				fsm_printf("[UE MAC]cra_default_handler:IOCCMD_RLCtoMAC_datasend_Req\n");
				if(SV(sendmsg3) && SV(C_RNTI) )//已发送msg3 且处于连接状态下
				{
					tmpdata_ptr = (char *)fsm_data_get();
					tmpdata = *((u32*)tmpdata_ptr);//取出指示数组长度的信息	
					DoRefreshRLCBuffserRequest(RlcRequest_arraytolist((RLC_Request *)(tmpdata_ptr+sizeof(u32)),tmpdata));//更新待发数据缓存信息
					//tmpdata_ptr+sizeof(int)//向后移动4个字节 指向数组头
					fsm_data_destroy((void *)tmpdata_ptr);//释放内存
					if(SV(LCGZeroToData)== true)//HQ201407040添加 判断是否触发regularBSR
						DoProduceBsr_LCGZeroToData();//该函数执行完后，SV(LCGZeroToData)置为false	 	 
					//cra_success();//调用该函数，死机，改为分割线中的等效代码不死机，原因物理适配层
					//没有接收IOCTL

					/*******分割******/
					SV(RA_info.ra_PreambleIndex) = 0;	//清除ra_preamble index ra_prach_mask_index
					SV(RA_info.ra_PRACHMaskIndex) = 0; 
					SV(PREAMBLE_TRANSMISSION_COUNTER) = 0;//将随机接入次数置0
					cleanharqBuff();//清空HARQ的msg3的缓存
					SV(ra_allocation_res)=0;
					if(SV(msg3_buf_ptr) != NULL)
					{
						fsm_pkt_destroy(SV(msg3_buf_ptr));//modified by lhl 20140717
						SV(msg3_buf_ptr) = NULL; //清空msg3的sk_buf缓存
					}
					SV(T_C_RNTI) = 0;//丢弃T_C_RNTI
					if(SV(CRTimer.flag)==true) //停止CR定时器
					{
						SV(CRTimer.timer_sign)= NULL;
						SV(CRTimer.flag)= false;
						SV(CRTimer.time_value)=0;
					}
					SV(sendmsg3)=0;//将MSG3标志置为0
					//SV(ratype.ra_type)=0;//随机接入类型置为0 
					SV(pdcch_rnti)=0;
					/*******分割******/
					doStartContentionBasedRandomAccessProcedure();//触发基于竞争的随机接入，20150106LHL
					SV(ratype.ra_type)=2;
				}
				else//忽略请求
				{
					tmpdata_ptr = (char *)fsm_data_get();
					fsm_data_destroy((void *)tmpdata_ptr);//释放内存
				}				
			break;
			default:
				ioctrl_handler();//与状态无关的ioctrl命令
			break;
		}
		FOUT;
	}
		
	//接入状态下，不进行RLC_DATA_REQ处理，意味着继续当前的随机接入过程。
	//if(WaitforRAresponseExpire)
	//RAR接收超时的处理 即接入失败
	/*if(CRTimerExpire)//modified by lhl 20140729,不需要了
	{
	handle_CRTimerExpire();
	fsm_schedule_self(0,CONTENTION_FAIL);
	}*/
	//竞争定时器超时的处理  即竞争失败
	FOUT;
}


/*
******************************
->Change Data and time: 20141105
->Function: NCRA状态的默认转移的处理函数
->Change:增加BJ的定时更新事件
->Details:主要处理状态机事件、IOCTL事件，跳转到不同的状态
->input:
->output:
->Special:
*******************************
*/
static void ncra_default_handler()
{
	FIN(ncra_default_handler());
	print_tran_info("[UEMAC][nra_default]ncra default ");//testing code
	if(MAC_CLOSE)
	{
		print_tran_info("[UEMAC][nra_default]MAC FSM is closed "); //testing code
		ue_mac_sv_close();
	}
	if(MAC_PK_FROM_LOWER)
	{
		//decomplexUeDataPdu(fsm_pkt_get());
		decomplexUeRARPdu(fsm_pkt_get());	//modify by lxr 20160122
	}
	/*if(MAC_PK_FROM_UPPER)
	{
		fsm_printf("[UEMAC][nra_default]receive packet from RLC,error! \n");
	}*/
	if(LogicalChannel_BJ)//modified 20141105 LHL,更新BJ
	{
		FlushBj();
		fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);
	}
	if(IOCTRL_ARRIVAL)
		ioctrl_handler();//与状态无关的ioctrl命令
	//接入状态下，不进行RLC_DATA_REQ处理，意味着继续当前的随机接入过程。
	FOUT;
}

/********************************************************************
**Function name: skb_from_cra_to_con()
**Descriptions: Receive packets from CRA_BUFF_List,add it into sdu buffer
**Input: 
**Output: Receive packets from CRA_BUFF_List,add it into sdu buffer
**Returns:
**Created by: Liu xiru
**Created Date: 2015
**--------------------------------------------------------------------
**Modified by: 
**Modified Date: 
*********************************************************************/
static void skb_from_cra_to_con(FSM_PKT * skb)
{
	//FSM_PKT * skb;
	TmBuffer *tmElm;
	struct UmBuffer *umElm;
	struct AmSduBuffer *amElm;
	URLC_IciMsg * iciMsg;
	struct  AM_Instance * amIns;
	struct  UM_TX_Instance *umTxIns;
	struct  TM_TX_Instance *tmTxIns;
	void *insptru, *insptrd;
	
	FIN(skb_from_cra_to_con());
	SV_PTR_GET(rlc_mac_sv);	

	//skb = (FSM_PKT *)fsm_pkt_get();
	iciMsg = (URLC_IciMsg*)skb->head;
	//fsm_printf("[RLC][skb_from_cra_to_con]receive from cra list, pbch=%d, rbid=%d\n",iciMsg->pbCh,iciMsg->rbId);
	//fsm_printf("[skb_from_cra_to_con]skb->data: %p\n", skb->data);
	fsm_octets_print(skb->data,64);
	//fsm_printf("the icimsg are %d, %d, %d\n",iciMsg->pbCh, iciMsg->rbId,iciMsg->rnti);
	//fsm_printf("[RLC] pbch is %d\n",iciMsg->pbCh);
	if(iciMsg->pbCh == 0)
	{
		findInsByRbid(iciMsg->rbId, &insptru, &insptrd ,&SV(ins_mode));
		fsm_printf("[RLC_MAC][skb_from_cra_to_con]ins_mode is: %d\n", SV(ins_mode));
		switch(SV(ins_mode))
		{
		case AM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][skb_from_cra_to_con][AM] rbid(%d) has no am instance \n",iciMsg->rbId);
				break;
			}
			amIns = (AM_Instance *)insptru;
			if((amIns->currentStatVar == ST_SUSPEND) || (amIns->currentStatVar == ST_SUSPEND_IDLE))
			{
				printk("[RLC][skb_from_cra_to_con][AM] am instance is suspended(rbid is %d)\n",amIns->rbId);
				break;
			}
			
			amElm = (AmSduBuffer *)fsm_mem_alloc(sizeof(AmSduBuffer));
			
			amElm->pkt = skb;
			amElm->pktstatus = FULL_SDU;
			amElm->pos = 0;
			list_add_tail(&amElm->list, &amIns->amSduBuffer.list);
			fsm_printf("[RLC][skb_from_cra_to_con]AM_MODE list_add done!\n");
			fsm_printf("[RLC][skb_from_cra_to_con] amIns addr is %p, amIns->rbid=%d, amIns->lcid=%d\n",&amIns,amIns->rbId,amIns->lcId);
			amIns->amSduBufferSize += skb->len;
			amIns->amSduBufferNum++;
			
			FOUT;
			break;

		case UM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][skb_from_cra_to_con][UM] rbid (%d )has no um tran instance \n",iciMsg->rbId);
				break;
			}
			umTxIns = (UM_TX_Instance *)insptru;
			if((umTxIns->CurrentStatVar == ST_SUSPEND) || (umTxIns->CurrentStatVar == ST_SUSPEND_IDLE))
			{
				fsm_printf("[RLC][skb_from_cra_to_con][UM] um instance is suspended(rbid is %d)\n",umTxIns->rbid);
				break;
			}

			umElm = (UmBuffer *)fsm_mem_alloc(sizeof(UmBuffer));
			umElm->pkt = skb;
			umElm->pktstatus = FULL_SDU;
			umElm->pos = 0;
			list_add_tail(&umElm->list, &umTxIns->umSduBuffer.list);
			fsm_printf("[RLC][skb_from_cra_to_con]UM_MODE list_add done!\n");
			
			umTxIns->umSduBufferNum++;
			if(SV(max_queue_num) < (umTxIns->umSduBufferNum)){
				SV(max_queue_num) = umTxIns->umSduBufferNum;
			}
			umTxIns->umSduBufferSize += skb->len;
			
			FOUT;
			break;

		case TM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][skb_from_cra_to_con][UM] rbid (%d )has no um tran instance \n",iciMsg->rbId);
				break;
			}
			tmTxIns = (TM_TX_Instance *)insptru;
			
			tmElm = (TmBuffer *) fsm_mem_alloc(sizeof(TmBuffer));
			tmElm->pkt = skb;
			list_add_tail(&tmElm->list, &tmTxIns->tmSduBuffer.list);
			tmTxIns->tmSduBufferNum++;
			tmTxIns->tmSduBufferSize += skb->len;
			
			FOUT;
			break;
		default:
			SV(countDropPacket)++;
			printk("[RLC][skb_from_cra_to_con] cannot distinfuish this %d\n", SV(ins_mode));
			printk("[RLC][skb_from_cra_to_con] A packet has dropped, %d packets dropped\n", SV(countDropPacket));
			
		break;
		}
		if(skb != NULL)
		{
			fsm_pkt_destroy(skb);
			skb = NULL;
		}
		FOUT;
	}
	else
	{
		SV(countDropPacket)++;
		fsm_printf("[RLC][skb_from_cra_to_con] no logic channel to transimmit this message\n");
		printk("[RLC][skb_from_cra_to_con] A packet has dropped, %d packets dropped\n", SV(countDropPacket));
		fsm_pkt_destroy(skb);
		skb = NULL;
	}
	FOUT;
}

/*
******************************
->Change Data and time: 20141105
->Function: CON状态的默认转移的处理函数
->Change:增加BJ的定时更新事件
->Details:主要处理状态机事件、IOCTL事件，跳转到不同的状态
->input:
->output:
->Special:
*******************************
*/
static void con_default_handler()
{
	//这里需要注意，有一个前提，要求RLC层保证是在得到发送机会后才发送数据包到MAC
	char *tmpdata_ptr;  
	int tmpdata;
	FSM_PKT* pkptr;	//FOR TEST
	FSM_PKT* skb;
	//rlc varibles 20150804
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	void *insPtr;
	int i;

	char *pdu_type = (char*)fsm_mem_alloc(sizeof(char));
	fsm_mem_set(pdu_type,0,sizeof(char));

	FIN(con_default_handler());
	SV_PTR_GET(rlc_mac_sv);

	//fsm_printf("[con_default_handler]CraQueue:%p\n", SV(CraQueue));
	//int pktnum = GetCountOfQueue(SV(CraQueue));
	//fsm_printf("[RLC_MAC][con_default_handler][begin]fsm event type: %d, code: %x\n", fsm_ev_type(),fsm_ev_code());

	if(MAC_CLOSE)
	{
		print_tran_info("[UEMAC][con_default]MAC FSM is closed!\n"); //testing code
		ue_mac_sv_close();
		FOUT;
	}
	if(RLC_CLOSE)                              //模块关闭
	{
		rlc_close();
		fsm_printf("[RLC_MAC]RLC FSM is closed!\n");
		FOUT;
	}

	if(SV(DATA_WAIT_ALLOCATION)==true && SV(UL_resource_info.resource_flag)==true)
	{//有上行资源后 处理待发数据
		//DoResourceAllocation(SV(UL_resource_info.m_tbsize)); 
		fsm_printf("[RLC_MAC][con_default_handler]DoResourceAllocation!\n");
		SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
		DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)));//不发MAC控制单元
		SV(UL_resource_info.resource_flag)=false;
	}
	if(GetCountOfQueue(SV(CraQueue)) > 0)
	{
		//fsm_printf("[RLC_MAC][con_default_handler]there are pkt in CraQueue!\n");
		while(!QueueEmpty(SV(CraQueue)))
		{
			pkptr = DeQueue(&SV(CraQueue));
			if(pkptr == NULL)
				continue;

			//fsm_printf("[con_default_handler]SV(CraQueue): %p, pkptr: %p\n", SV(CraQueue), pkptr);
			skb_from_cra_to_con(pkptr);

			do_bsr_and_resource_alloc();	//this function should in while loop or not? this a question
		}
		//FOUT;
	}

	if(MAC_PK_FROM_LOWER)
	{//通过ICI信息判断包类型 解复用 调用发送函数，发给RLC
		//print_tran_info("[UEMAC][con_default]CON receive a data packet from PHYadapter");//testing code
		SV(countRecvFromLower)++;
		skb=fsm_pkt_get();
		fsm_mem_cpy(pdu_type, skb->data,sizeof(char));
		*pdu_type=(*pdu_type)>>6;
		//fsm_mem_set(&m_fixed_subhead,0,sizeof(MAC_SDU_subhead_last)); //pdu_type : 0 -> Common Data PDU ; 1~3 -> RAR PDU
		printk("[UE MAC ]mac_pkthandler decomplexUeDataPdu: pdu_type=%d\n",(*pdu_type));
		if( (*pdu_type) > 0){
			fsm_pkt_destroy(skb);
			fsm_mem_free(pdu_type);
			printk("[UE MAC ]mac_pkthandler decomplexUeDataPdu: This is a RAR PDU ,Error!!!\n");
			FOUT;
		}
		decomplexUeDataPdu(skb);
		//printk("[RLC_MAC]countRecvFromLower: %d\n", SV(countRecvFromLower));
		//FOUT;
	}
	else if(RLC_PKT_FROM_UPPER)		//接收来自上层的包
	{
		SV(countRecvFromUpper)++;
		rlc_pkt_receive_from_upper();

		//printk("[RLC_MAC]SV(countRecvFromUpper): %d\n", SV(countRecvFromUpper));
		//add following code in 20150915
		if( PMRLC_BUFFERREP_REQ() == NULL )
			fsm_printf("[RLC_MAC][con_default_handler]: the return pointer of PMRLC_BUFFERREP_REQ is NULL!\n");
		else{
			tmpdata_ptr = (char *)PMRLC_BUFFERREP_REQ();
			tmpdata = *((u32*)tmpdata_ptr);//取出指示数组长度的信息	
			if( tmpdata > 0 )
			{
				DoRefreshRLCBuffserRequest(RlcRequest_arraytolist((RLC_Request *)(tmpdata_ptr+sizeof(u32)),tmpdata));//更新待发数据缓存信息
				fsm_data_destroy((void *)tmpdata_ptr);//释放内存
				//if(SV(LCGZeroToData)== true)//HQ201407040添加 判断是否触发regularBSR
					//DoProduceBsr_LCGZeroToData();//该函数执行完后，SV(LCGZeroToData)置为false // noted in 20151228
			}
		}

		//SV(UL_resource_info.resource_flag) = true;
			/********for test 20150916 *********/
		/*if(SV(UL_resource_info.resource_flag) == true)
		{
			//GetTBsize_Allocation(SV(UL_resource_info.m_tbsize));//返回tbsize,单位byte,考虑了发送BSR的资源
			SV(UL_resource_info.m_tbsize)=1400*8;//for test
			SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
			//fsm_printf("ready resource \n");
			if(! (list_empty(&(SV(MacBuffer_RLC)->list))))
				DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)));  //开始资源分配过程
			SV(UL_resource_info.resource_flag)=false;
			//fsm_printf("start UL Schedule \n");
		}
		else
			send_SR();*/
		//FOUT;
	}
  	else if(LogicalChannel_BJ)//modified 20141105 LHL,更新BJ
	{
		FlushBj();
		fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);
		//FOUT;
	}
	else if(RLC_EV_SELF)				// 定时器超时，定位实体/
	{
		if(!(insPtr = findInsByCode(fsm_ev_code(), &SV(timerType))))
		{
			printk("[RLC][rlc_interupt] the instance of this code is NULL\n");
			//fsm_printf("cannot find the instance of this code \n ");
			FOUT;
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
				/*if(SV(isBufferReq) == 0)
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
				}*/
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
					/*if(!SV(bufferReqTimer))
					{
						SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
					}*/
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
		/*case BUFFER_REQ:
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
			break;*/
		default:
			printk("[RLC][rlc_interupt] cannot distinguish this code \n");
			break;
		}
	}
	else if(IOCTRL_ARRIVAL)
	{ //处理IOCTRL Command
		//this ioctrl will never been trigger, 20150915!!!
		if(fsm_ev_ioctrl_cmd()== IOCCMD_RLCtoMAC_datasend_Req)
		{
			tmpdata_ptr = (char *)fsm_data_get();
			tmpdata = *((u32*)tmpdata_ptr);//取出指示数组长度的信息
			//print_tran_info("CON receive a RLCdatareq");//testing code
			//fsm_printf("tmpdata:%d \n",tmpdata);//testing code
			//fsm_printf("txQueueSize:%d\n",((RLC_Request *)(tmpdata_ptr+sizeof(int)))->txQueueSize);//testing code
			DoRefreshRLCBuffserRequest(RlcRequest_arraytolist((RLC_Request *)(tmpdata_ptr+sizeof(u32)),tmpdata));//更新待发数据缓存信息
			fsm_data_destroy((void *)tmpdata_ptr);//释放内存
			if(SV(LCGZeroToData)== true)//HQ201407040添加 判断是否触发regularBSR
				//RefreshBufferStatusForBSR();
				DoProduceBsr_LCGZeroToData();//该函数执行完后，SV(LCGZeroToData)置为false
			
			/*******for test *********/
			SV(UL_resource_info.resource_flag) = true;
			/********for test *********/
			if(SV(UL_resource_info.resource_flag) == true)
			{
				//GetTBsize_Allocation(SV(UL_resource_info.m_tbsize));//返回tbsize,单位byte,考虑了发送BSR的资源
				SV(UL_resource_info.m_tbsize)=1400*8;//for test
				SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
				//fsm_printf("ready resource \n");
				if(! (list_empty(&(SV(MacBuffer_RLC)->list))))
					DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)));  //开始资源分配过程
				SV(UL_resource_info.resource_flag)=false;
				//fsm_printf("start UL Schedule \n");
				}
			else 
				send_SR();
			//HQ 20140702注释
			//这里判断是否需要开始SR过程，还需要考虑是否有regularBSR被触发了
			//目前实现上RLC发datareq的机制和MAC资源分配策略确定了 如果收到req都会触发regularBSR 
			//如果完全按照协议实现完整的SR过程 RLC需要修改req的发送机制
			//MAC则需要在这里添加一个状态，等待SR的应答 以及相关处理 
		}
		else 
			ioctrl_handler();  //与状态无关的ioctrl命令		
		//print_tran_info("[UEMAC][con_default]CON default ioctl hander");//testing codes
		reports_handler();//处理BSR PHR等上报事件 cut to this domian 20150910
	}
	else
	{
		printk("[RLC][rlc_interupt] cannot distinguish the interrupt\n");
	}
	//fsm_printf("[RLC_MAC][con_default_handler][end]fsm event type: %d, code: %x\n", fsm_ev_type(),fsm_ev_code());
	fsm_mem_free(pdu_type);
FOUT;
}
/*
******************************
->Change Data and time: 20141105
->Function:sendmsg3状态的默认转移的处理函数
->Change:增加BJ的定时更新事件
->Details:主要处理状态机事件、IOCTL事件，跳转到不同的状态
->input:
->output:
->Special:
*******************************
*/
static void sendmsg3_default_handler()
{
	char  *tmpdata_ptr;
	int tmpdata;
	SV_PTR_GET(rlc_mac_sv);
	FIN(sendmsg3_default_handler());
	if(MAC_CLOSE)
	{
		print_tran_info("[UEMAC][msg3_default]MAC FSM is closed "); //testing code
		ue_mac_sv_close();
	}
	if(LogicalChannel_BJ)//modified 20141105 LHL,更新BJ
	{
		FlushBj();
		fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);
	}
	/*testing code
	if(MAC_PK_FROM_UPPER)
	{
		print_tran_info("receive a data packet from RLC,send it");//testing code
		//complexUeMacPdu(fsm_pkt_get());//会引起死机 
		fsm_pkt_destroy(fsm_pkt_get());//testing code
		fsm_printf("destory packet\n");  
		fsm_schedule_self(50,TEST_SENDMSG3);
	}
	//testing code end */

	if(IOCTRL_ARRIVAL)
	{
		switch(fsm_ev_ioctrl_cmd())
		{
			default:
				ioctrl_handler();
			break;
		}
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:发送调度请求
->Change:
->Details:目前没有正真的发送SR，而是模拟SR一直发送失败，而发起随机接入
->input:
->output:
->Special:
*******************************
*/
static void send_SR()
{
	FIN(send_SR());
	SV_PTR_GET(rlc_mac_sv);
	fsm_printf("[UE MAC]send_SR\n");
	fsm_schedule_self(0,SR_Fail);//设定为SR发送失败
	SV(ratype.ra_type)=2;
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:各个状态的默认转移的处理函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void ioctrl_handler()
{
	void *tempdata_ptr;
	UEPHY_TO_MAC_ULgrant *tempulgrant;
	u32 rar_failed_times;//201501演示计划一，用户空间配置添加的变量
	u32 contention_failed_times;//201501演示计划一，用户空间配置添加的变量
	FIN(ioctrl_handler());

	SV_PTR_GET(rlc_mac_sv);

	int cmd_value=fsm_ev_ioctrl_cmd();

	switch(cmd_value)
	{
		case IOCCMD_RRCtoMAC_recv_sysinfo :  
			fsm_printf("[MAC][RECVSYS]\n");
			fsm_do_ioctrl(STRM_TO_SRIO, IOCCMD_MACtoPHY_recv_sysinfo  , NULL, 0);
			//发条ioctrl命令给物理层适配层，指示物理层使用广播RNTI接收系统消息
		break;
		case IOCCMD_RRCtoMAC_stop_recv_sysinfo : 
			fsm_do_ioctrl(STRM_TO_SRIO, IOCCMD_MACtoPHY_stop_recv_sysinfo , NULL, 0); 
			//发条ioctrl命令给物理层适配层，指示物理层停止接收系统消息
		break ;
		case IOCCMD_RRCTOMAC_CONFIG_TAT : 
			fsm_printf("[UEMAC][RRCTOMAC_TA]rrctomac_taconfig\n");
			TATimer_config();//来自RRC对于TA定时器的配置命令 
		break;
		case IOCCMD_RRCtoMAC_mainconfig :
			fsm_printf("[UEMAC][RRCTOMAC_mainconfig]rrctomac_mainconfig\n");
			ue_mac_main_config(); 
		break;
		case IOCCMD_RRCtoMAC_RACHcommonconfig : 
			fsm_printf("[UEMAC][RRCTOMAC_rachconfig]rrctomac_rachconfig\n");
			ue_mac_rach_config();
		break;
		case IOCCMD_RRCtoMAC_LCconfig   : 
			fsm_printf("[UEMAC][RRCTOMAC_lcconfig]rrctomac_lcconfig\n");
			ue_mac_logCh_config();
		break;
		case IOCCMD_RRCTOMAC_CONFIG_BCCH : 
			fsm_printf("[UEMAC][RRCTOMAC_bcchconfig]rrctomac_bcchconfig\n");
			ue_mac_BCCH_config();
		break;
		case IOCCMD_RRCTOMAC_CONFIG_PCCH : 
			fsm_printf("[UEMAC][RRCTOMAC_pcchconfig]rrctomac_pcchconfig\n");
			ue_mac_PCCH_config();
		break;
		case IOCCMD_RRCTOMAC_CONFIG_SPS  : 
			fsm_printf("[UEMAC][RRCTOMAC_spsconfig]rrctomac_spsconfig\n");
			ue_mac_SPS_Config();
		break;
		case IOCCMD_RRCtoMAC_LCrelease : 
			//release LC 撤销某条信道
			ue_mac_lc_release();
		break;
		case IOCCMD_Harq_feedback : 
			handle_HARQ_feedback();
		break;
		case IOCCMD_PDCCHtoMAC_ULGRANT : //PHY在PDCCH收到上的含有UL_grant的DCI,发送给MAC
		//MAC处理收到的ul_grant 修改上行资源状态
		//处理MSG4竞争接入，非初始化的随机接入
			
			tempdata_ptr = fsm_data_get();
			tempulgrant=(UEPHY_TO_MAC_ULgrant *)tempdata_ptr;
			//fsm_printf("[LHL]TEMP ULGRANT:%d\n",tempulgrant->ulgrant.RIV);
			SV(recv_frame).frameNo=tempulgrant->frameNo;
			SV(recv_frame).subframeNo=tempulgrant->subframeNo;
			if(tempulgrant->m_rnti == SV(C_RNTI))
				 SV(pdcch_rnti)=1;//PDCCH通过m_RNTI加扰
			if(SV(C_RNTI) && SV(sendmsg3) && SV(pdcch_rnti) && (SV(ratype.ra_type)==2) )//3//3GPP长期演进P121
			{
				fsm_printf("[UEMAC][ioctl_handler]UE MAC ULGRANT:Contention Resolution successfully\n");
				fsm_schedule_self(0, Contention_Success );
			}
			else if(SV(C_RNTI) && SV(sendmsg3) && !SV(pdcch_rnti) && (SV(ratype.ra_type)==2) )
			{
				fsm_printf("[UEMAC][ioctl_handler]UE MAC ULGRANT:Contention Resolution fail\n");
				fsm_schedule_self(0, ContentionResolution_Fail );
			}
			//fsm_printf("[UEMAC][ioctl_handler]UE MAC RECV ULGRANT\n");
			//printk("[UEMAC][ioctl_handler]UE MAC RECV ULGRANT\n");
			SV(UL_resource_info.m_tbsize) = DoReceiveULgrant_Tbsize(tempulgrant->ulgrant);
			//fsm_printf("[MAC][ioctrl_handler]SV(UL_resource_info.m_tbsize):%d\n",SV(UL_resource_info.m_tbsize));
			//printk("[MAC][ioctrl_handler]SV(UL_resource_info.m_tbsize):%d, ul_sched_count:%d\n",SV(UL_resource_info.m_tbsize),++ul_sched_count);
			//SV(UL_resource_info.m_tbsize) = DoReceiveULgrant_Tbsize(*((int *)tempdata_ptr)); //具体处理待确定UL_grant的数据结构之后再定 
			//根据协议理解 只要收到ul_grant 则会重启retxBSR-Timer，不需要判断是否是针对新数据传输的。协议 36321-80c 5.4.5节
			SV(UL_resource_info.resource_flag) = true;

			if(SV(UL_resource_info.resource_flag) == true)
			{
				SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
				if(! (list_empty(&(SV(MacBuffer_RLC)->list))))
					DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)));  //开始资源分配过程
				SV(UL_resource_info.resource_flag)=false;
			}	//add these codes in 20151228
			
			/*fsm_data_destroy(tempdata_ptr);
			if(SV(RetxBSRTimer.flag) == true && SV(RetxBSRTimer.timer_sign) != NULL)
				fsm_schedule_cancel(SV(RetxBSRTimer.timer_sign));
			SV(RetxBSRTimer.time_value)	= SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.retxBSR_Timer);
			SV(RetxBSRTimer.timer_sign) = fsm_schedule_self(SV(RetxBSRTimer.time_value), RexBSRTimer_Expire);
			SV(RetxBSRTimer.flag)= true;*/ 	//noted in 20151228
		break;
		case IOCCMD_TEST_lostUL : //一条测试命令
			fsm_schedule_self(100,TimeAlignmentTimer_Expire);
		break;
		case TEST_RECV_SYS :  
			fsm_printf("[MAC][RECVSYS]\n");
			fsm_do_ioctrl(STRM_TO_SRIO, IOCCMD_MACtoPHY_recv_sysinfo  , NULL, 0);
		//发条ioctrl命令给物理层适配层，指示物理层使用广播RNTI接收系统消息
		break;
		case TEST_RECV_PADING://test code recv pading
			fsm_printf("[MAC][RECVPAG]\n");
			fsm_do_ioctrl(STRM_TO_SRIO, TEST_MACtoPHY_PADING , NULL, 0);
		break;
		case _IOCTLtoUEMAC_RAR_Failed:
			tempdata_ptr = fsm_data_get();
			rar_failed_times=*((u32 *)tempdata_ptr);
			SV(rar_failed_num)=rar_failed_times;
			fsm_data_destroy(tempdata_ptr);
		break;
		case _IOCTLtoUEMAC_Contention_Failed:
			tempdata_ptr = fsm_data_get();
			contention_failed_times=*((u32 *)tempdata_ptr);
			SV(contention_failed_num)=contention_failed_times;
			fsm_data_destroy(tempdata_ptr);
		break;
		case _IOCTLtoUEMAC_PKT_NUM:
			test_print_pkt_num();
		break;

		/***************** RLC ioctl_handler ******************/
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
		/*case  MRLC_TRANSOP_IND :
			fsm_printf("\n[RLC][rlc_ioctrl_handler] MRLC_TRANSOP_IND  has received\n");
			PMRLC_TRANSOP_IND();
			FOUT;
			break;*/ //noted in 20150804
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
			fsm_printf("[RLC_MAC] unidentified ioctrl command!\n");
			break;
		
/*还有其他配置命令*/
	}	
	FOUT;
}

static void test_print_pkt_num()
{
	FIN(test_print_pkt_num());
	SV_PTR_GET(rlc_mac_sv);
	printk("[UEMAC]uplink lost pkt num:%d\n",SV(lost_pkt_num_uplink));
	printk("[UEMAC]uplink total pkt num:%d\n",SV(pkt_num_uplink));
	printk("[UEMAC]downlink lost pkt num:%d\n",SV(lost_pkt_num_downlink));
	printk("[UEMAC]downlink total pkt num:%d\n",SV(pkt_num_downlink));
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:处理BSR PHR等上报事件
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void reports_handler()
{
	FIN(reports_handler());
	if(PeriodicBSRTimerExpire)
		handle_PeriodicBSRTimerExpire();
	if(RexBSRTimerExpire)
		handle_RetxBSRTimerExpire();
	//还有PHR 暂未实现
FOUT;
}
/*
******************************
->Change Data and time: 
->Function:TA定时器配置
->Change:
->Details:收到RRC 相应的配置命令，启动该定时器
->input:
->output:
->Special:
*******************************
*/
static void TATimer_config() 
{
	void *tmpdata_ptr;
	FIN(TATimer_config());
	SV_PTR_GET(rlc_mac_sv);
	int time;
	tmpdata_ptr = fsm_data_get();
	fsm_mem_cpy(&time,tmpdata_ptr, sizeof( struct TimeAlignmentTimer));
	SV(TA_Periodic_time)=time;//LHL 20141021
	//SV(TATimer.time_value)=SV(TA_Periodic_time);//LHL 20141021
	//SV(TATimer.flag)=true;
	//SV(TATimer.timer_sign)=fsm_schedule_self(SV(TATimer.time_value)*100, TimeAlignmentTimer_Expire );
	if(SV(TATimer.flag)==true)
	{//LHL 20141021TA定时器正在运行，此时取消定时器，重新开启
		fsm_printf("[UE MAC][TATimer_config]TATimer_config\n");//LHL for test 20141021
		fsm_schedule_cancel(SV(TATimer.timer_sign));
		SV(TATimer.time_value)=SV(TA_Periodic_time);//LHL 20141021
		SV(TATimer.flag)=true;
		SV(TATimer.timer_sign)=fsm_schedule_self(SV(TATimer.time_value)*100, TimeAlignmentTimer_Expire );
		//SV(TA_timer_flag)=0;
	}//若定时器 没有运行则不开启定时器 只保存定时器周期。
	fsm_data_destroy(tmpdata_ptr);
FOUT;
}
/*
******************************
->Change Data and time: 
->Function:处理HARQ反馈
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void handle_HARQ_feedback()
{
	void *tmpdata_ptr;	
	FIN(handle_HARQ_feedback());
	SV_PTR_GET(rlc_mac_sv);
	HARQ_statu_parameters ACK_statu={0,true,0};
	tmpdata_ptr = fsm_data_get();
	if( ((struct HARQ_feedback*)tmpdata_ptr)->HARQ_FEEDBACK == true )
	{
		SV(ULHARQ_status)[((HARQ_feedback*)tmpdata_ptr)->processID].avaiable_flag = true;
		SV(ULHARQ_status)[((HARQ_feedback*)tmpdata_ptr)->processID].HARQ_buffer_pointer = NULL;
		SV(ULHARQ_status)[((HARQ_feedback*)tmpdata_ptr)->processID].statu=ACK_statu;
	}
	//不考虑重传，默认feedback都是ACK
	fsm_data_destroy(tmpdata_ptr);
FOUT;
}

/*  
//该函数作废，因为_fsm_current_state_ptr()不可用，所在各个状态的默认处理函数中分别处理RLC_DATE_REQ
static void handle_RLCdatasend_Req()
{
  	FIN(handle_RLCdatasend_Req());
    int *_fsm_current_state_ptr;
    int i=0;//测试用的变量
  	SV_PTR_GET(rlc_mac_sv);
    DoRefreshRLCBuffserRequest((RlcBufferRequest *) fsm_ev_data());
 	_fsm_current_state_ptr = fsm_current_state_ptr_get(); //这么写，获得的并不是当前状态的ID
  	if ((*_fsm_current_state_ptr)==ST_IDLE)
  	{
  	    fsm_schedule_self(1,Start_ContentionBase_RandomAcc);  //时间单位是10us 这个时间设置多长待究
  	}
  	if( (*_fsm_current_state_ptr)==ST_CON )
    {
      if(check_resourceinfo()==AVAILABLE)
           i++; //开始资源分配过程
      else
         send_SR();
    }
 	//接入状态下，不进行其他处理，意味着继续当前的随机接入过程。
FOUT;
}
*/

/*
******************************
->Change Data and time: 
->Function:处理TA定时器超时
->Change:
->Details:设置定时器、清空HARQ缓存、释放所有资源
->input:
->output:
->Special:
*******************************
*/
static void handle_timeAlignmentTimerExpire()
{
	FIN(handle_timeAlignmentTimerExpire());  
	SV_PTR_GET(rlc_mac_sv);
	SV(TATimer.time_value)=0;
	SV(TATimer.flag)=false;//设置定时器的标志
	SV(TATimer.timer_sign)=NULL;
	cleanharqBuff(); //清空HARQ缓存
	//fsm_do_ioctrl(STRM_TO_IP, IOCCMD_MACtoRRC_PUCCHSRS_release  , NULL , 0);  //通知RRC释放PUCCH/SRS的配置
	//释放所有资源
	SV(UL_resource_info.resource_flag)=false;//释放上行资源
	SV(UL_resource_info.m_tbsize)=0;
											//释放下行资源分配??? 
	print_tran_info("[UEMAC][handle_tatimer]LostUL"); //testing code 
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:处理周期BSR定时器超时
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void handle_PeriodicBSRTimerExpire()
{
	SV_PTR_GET(rlc_mac_sv);
	FIN(handle_PeriodicBSRTimerExpire());
	DoProduceBsr_PeriodicBSRTimer();//生成periodic BSR
	SV(PeriodicBSRTimer).time_value = SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.periodicBSR_Timer)*100;
	SV(PeriodicBSRTimer).timer_sign = fsm_schedule_self(SV(PeriodicBSRTimer).time_value,PeriodicBSRTimer_Expire);
	SV(PeriodicBSRTimer).flag = true;
	fsm_printf("[UEMAC][handle_periodicbsrtimer]handle PeriodicBSRTimer expire\n");//testing code 
FOUT;
}
/*
******************************
->Change Data and time: 
->Function:处理重传BSR定时器超时
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void handle_RetxBSRTimerExpire()
{
	SV_PTR_GET(rlc_mac_sv);
	FIN(handle_RetxBSRTimerExpire());
	DoProduceBsr_RetxBSRTimer();
	SV(RetxBSRTimer.timer_sign) = NULL;
	SV(RetxBSRTimer.flag) = false;	
	fsm_printf("[UEMAC][handle_retxbsrtimer]handle RetxBSRTimer expire\n");//testing code 
}
/*
******************************
->Change Data and time: 
->Function:处理重传CR定时器超时
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void handle_CRTimerExpire()
{
	SV_PTR_GET(rlc_mac_sv);
	FIN(handle_CRTimerExpire());	
	SV(CRTimer.timer_sign) = NULL;
	SV(CRTimer.flag) = false;	
	fsm_printf("[UEMAC][handle_crtimer]CRTimer expire\n");//testing code 
}


/*
******************************
->Change Data and time: 
->Function: reset 处理函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void ue_mac_reset()//modified by LHL 20140717
{
	FIN( ue_mac_reset());
	SV_PTR_GET(rlc_mac_sv);
	//initialize Bj for each logical channel to zero;初始化每条逻辑信道的BJ为0 
	Zero_LogicalChannel_Bj_Reset();
	//stop (if running) all timers; 停止所有正在计时的定时器
	stop_WRAP_timer();//停止WaitforRAresponseTimer定时器
	stop_CR_timer();//停止CR定时器
	stop_RetxBSR_timer();
	//stop_TA_timer();
	stop_PeriodicBSR_timer();
	//	consider timeAlignmentTimer as expired and perform the corresponding actions in subclause 5.2;
	
	//	set the NDIs for all uplink HARQ processes to the value 0;
	SV(PREAMBLE_TRANSMISSION_COUNTER) = 0;//	stop, if any, ongoing RACH procedure;
	SV(SR_COUNTER)= 0; 
	//	discard explicitly signalled ra-PreambleIndex and ra-PRACH-MaskIndex, if any;
	if(SV(ratype.ra_type)==1 ||SV(ratype.ra_type)==3)//通过RRC或PDCCH显示指示的随机接入前导码
	{
		SV(RA_info.ra_PreambleIndex)=0;//丢弃任何显示信令分配的前导码
		SV(RA_info.ra_PRACHMaskIndex)=0;
	}
	//	flush Msg3 buffer;
	if(SV(msg3_buf_ptr)!=NULL)
	{
		fsm_pkt_destroy(SV(msg3_buf_ptr));	
		SV(msg3_buf_ptr) = NULL; 
	}
	//	cancel, if any, triggered Scheduling Request procedure;
	Free_Bsr_Info();//	cancel, if any, triggered Buffer Status Reporting procedure;
	Empty_MACBuffer_Rlc();//清空RLC的数据请求缓存 【自定】
	//	cancel, if any, triggered Power Headroom Reporting procedure;
	//	flush the soft buffers for all DL HARQ processes;
	//	for each DL HARQ process, consider the next received transmission for a TB as the very first transmission;
	SV(T_C_RNTI) = 0; //	release, if any, Temporary C-RNTI.
	if(SV(m_temp_cr) != NULL)
	{
		fsm_mem_free(SV(m_temp_cr));
		SV(m_temp_cr) = NULL;
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function: 随机接入参数配置
->Change:
->Details:接收到RRC的IOCTL时调用
->input:
->output:
->Special:
*******************************
*/
static void ue_mac_rach_config()
{
	void *tmpdata_ptr;	
	FIN(ue_mac_rach_config());
	SV_PTR_GET(rlc_mac_sv);
	tmpdata_ptr = fsm_data_get();
	//获取ioctrl携带的数据，保存配置信息
	SV(systemconfigInfo.RACHCommonConfigInfo) = *((struct RachConfigCommon*)tmpdata_ptr); 
	//fsm_mem_cpy(&(SV(systemconfigInfo.RACHCommonConfigInfo)), tmpdata_ptr, sizeof(struct RachConfigCommon));
	//fsm_printf("[UEMAC][rach_config]maxHARQ_Msg3Tx :%d ",	SV(systemconfigInfo.RACHCommonConfigInfo.maxHARQ_Msg3Tx) );//testing code
	//fsm_printf("[UEMAC][rach_config]mac_ContentionResolutionTimer : %d \n",SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.mac_ContentionResolutionTimer));
	fsm_data_destroy(tmpdata_ptr);//释放内存
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:MAC主配置
->Change:
->Details:接收到RRC的IOCTL时调用
->input:
->output:
->Special:
*******************************
*/
static void ue_mac_main_config()
{
	void *tmpdata_ptr;	
	FIN(ue_mac_main_config());
	SV_PTR_GET(rlc_mac_sv);
	tmpdata_ptr = fsm_data_get();	
	SV(systemconfigInfo.MACMainConfigInfo) = *((struct MAC_MainConfig*)tmpdata_ptr); //获取ioctrl携带的数据，保存配置信息
	fsm_data_destroy(tmpdata_ptr);//释放内存
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:PCCH配置
->Change:
->Details:接收到RRC的IOCTL时调用
->input:
->output:
->Special:
*******************************
*/
static void ue_mac_PCCH_config()
{
	void *tmpdata_ptr;	
	FIN( ue_mac_PCCH_config());
	SV_PTR_GET(rlc_mac_sv);
	tmpdata_ptr = fsm_data_get();		
	SV(systemconfigInfo.PCCHConfigInfo) = *((struct PCCH_Config*)tmpdata_ptr);  //获取ioctrl携带的数据，保存配置信息
	fsm_data_destroy(tmpdata_ptr);//释放内存
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:BCCH配置
->Change:
->Details:接收到RRC的IOCTL时调用
->input:
->output:
->Special:
*******************************
*/
static void ue_mac_BCCH_config()
{
	void *tmpdata_ptr;
	FIN(ue_mac_BCCH_config());
	SV_PTR_GET(rlc_mac_sv);
	tmpdata_ptr = fsm_data_get();	
	SV(systemconfigInfo.BCCHConfigInfo)= *((struct BCCH_Config*)tmpdata_ptr);  //获取ioctrl携带的数据，保存配置信息 
	fsm_data_destroy(tmpdata_ptr);//释放内存
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:SPS配置
->Change:
->Details:接收到RRC的IOCTL时调用
->input:
->output:
->Special:
*******************************
*/
static void ue_mac_SPS_Config()
{
	void *tmpdata_ptr;
	FIN(ue_mac_SPS_Config());
	SV_PTR_GET(rlc_mac_sv);
	tmpdata_ptr = fsm_data_get();
	SV(systemconfigInfo.SPSConfigInfo)= *((struct SPS_Config*)tmpdata_ptr); //获取ioctrl携带的数据，保存配置信息 //测试过，这样的操作没有问题
	fsm_data_destroy(tmpdata_ptr);//释放内存
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:逻辑信道配置
->Change:
->Details:接收到RRC的IOCTL时调用
->input:
->output:
->Special:
*******************************
*/
static void ue_mac_logCh_config() //modified by HQ 20120506
{
	void *tmpdata_ptr;
	FIN(ue_mac_logCh_config());
	MAC_LogicalChannelConfig_IoctrlMsg *lc_configinfo=(MAC_LogicalChannelConfig_IoctrlMsg*)fsm_mem_alloc(sizeof(MAC_LogicalChannelConfig_IoctrlMsg));
	tmpdata_ptr = fsm_data_get();	
	fsm_mem_cpy(lc_configinfo, tmpdata_ptr, sizeof(MAC_LogicalChannelConfig_IoctrlMsg));//将数据拷贝到本地 
	fsm_data_destroy(tmpdata_ptr);//释放内存
	Init_LogicalChannel_ConfigInfo(lc_configinfo);//调用的函数需要注意释放lc_configinfo指针 20140506
	//fsm_printf("[UEMAC][logch_config]LCID:%d, ",lc_configinfo->logicalChannelIdentity);
	//fsm_printf("[UEMAC][logch_config]RRC TO MAC:LCGROUP:%d\n",lc_configinfo->logicalChannelConfig.ul_SpecificParameters.logicalChannelGroup);
	//fsm_printf("[UEMAC][logch_config]prioritisedBitRate: %d \n",lc_configinfo->logicalChannelConfig.ul_SpecificParameters.prioritisedBitRate);
	fsm_mem_free(lc_configinfo);	
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:删除某条逻辑信道
->Change:
->Details:接收到RRC的IOCTL时调用
->input:
->output:
->Special:
*******************************
*/
static void ue_mac_lc_release()
{
	void *tmpdata_ptr;
	FIN(ue_mac_lc_release());
	tmpdata_ptr = fsm_data_get();
	Delete_LogicalChannel_ConfigInfo(*((unsigned char *)tmpdata_ptr));
	fsm_data_destroy(tmpdata_ptr);
	FOUT;
}

/**************************  HuangQiong end ***********************************/

/***************random access functions ChenLang  *********************/
/*
******************************
->Change Data and time: 
->Function:随机选择随机接入msg1的前导码
->Change:
->Details:
->input:
->output:选择的前导码
->Special:
*******************************
*/
static u8 RandomlySelectRaPreamble()
{
	FIN(RandomlySelectRaPreamble());
	SV_PTR_GET(rlc_mac_sv);
	u8 raPreambleID;
	u8 NumberOfRA_Preambles=(SV(systemconfigInfo.RACHCommonConfigInfo.preambleInfo.numberOfRA_Preambles));
	fsm_get_random_bytes(&raPreambleID , sizeof(char));
	raPreambleID = raPreambleID%NumberOfRA_Preambles;
	//fsm_printf("[UEMAC][selectpreamble]We get random number: %d\n", raPreambleID);//testing code
	FRET(raPreambleID);
}
/*
******************************
->Change Data and time: 
->Function:开启WaitforRAresponseTimer定时器，即msg2的接收窗口
->Change:
->Details:发送MSG1后调用
->input:
->output:
->Special:
*******************************
*/
static void StartWaitingForRaResponse()
{
	FIN(StartWaitingForRaResponse());
	SV_PTR_GET(rlc_mac_sv);
	//modified by HQ
	SV(WaitforRAresponseTimer.time_value)=SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.raResponseWindowSize);
	// 1subframe=1ms=100 (10us)
	fsm_printf("[UEMAC][wait_raresponse]run here,start WRAtimer\n");//testing code
	SV(WaitforRAresponseTimer.timer_sign) = fsm_schedule_self(SV(WaitforRAresponseTimer.time_value)*100, WaitforRAresponse_Expire );
	//set up waitRaResponse_timer //会死机 //新框架下fsm_schdule_self()的返回值是int *  修改timer结构 //测试通过 不再死机
	SV(WaitforRAresponseTimer.flag)=true;
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:发送msg1
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void SendRaPreamble(RACH_ConfigDedicated *randomaccess_info)
{
	FIN(SendRaPreamble());
	//modified by HQ
	fsm_printf("[UEMAC][sendrapreamble]run here,start send RAP \n");//testing code
	//fsm_octets_print(randomaccess_info,sizeof(RACH_ConfigDedicated));// for test
	fsm_do_ioctrl(STRM_TO_SRIO, IOCCMD_MACtoPHY_Preamble_Indicate, (void*)randomaccess_info , sizeof(randomaccess_info));
	//send the information of random access to PHYadapter
	fsm_mem_free(randomaccess_info);//20140722 modified by lhl 释放IOCTL申请的内存
	FOUT;
 }

/*
******************************
->Change Data and time: 
->Function:发起非竞争随机接入，发送msg1，开启RAR接收窗口
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void doStartNonContentionBasedRandomAccessProcedure() //modified by HQ  20140413
{
	FIN(doStartNonContentionBasedRandomAccessProcedure());
	SV_PTR_GET(rlc_mac_sv);
	RACH_ConfigDedicated *randomaccess_info=(RACH_ConfigDedicated *)fsm_mem_alloc(sizeof(RACH_ConfigDedicated ));
	int mg3_buffer;
	print_tran_info("[UEMAC][noncontention_ra]startNCRA IDLE -> NCRA");//testing code
	if(SV(PREAMBLE_TRANSMISSION_COUNTER ) == 0)   //随机接入的初始化
	{ 
		SV(PREAMBLE_TRANSMISSION_COUNTER ) = 1; //将PREAMBLE_TRANSMISSION_COUNTER置1 
		SV(backoff_index) = 0;  //UE保存的backoff置为0ms    
		mg3_buffer = 0;   //清空MSG3缓存//还未实现
	}
	fsm_printf("[UEMAC][noncontention_ra]RA counter :%d\n",SV(PREAMBLE_TRANSMISSION_COUNTER ));//testing code
	//在接收到非竞争接入命令时，就将接入指示信息保存了
	randomaccess_info->ra_PreambleIndex=SV(RA_info.ra_PreambleIndex);
	//fsm_printf("[HEXI]PREAMBLE INDEX IN MACFSM:%d\n",SV(RA_info.ra_PreambleIndex));
	
	randomaccess_info->ra_PRACHMaskIndex= SV(RA_info.ra_PRACHMaskIndex);
	SendRaPreamble(randomaccess_info);//如果这个地方给形参，调用该函数需要给实参
	StartWaitingForRaResponse();//start waiting for the RAR
	FOUT;
}


/*
******************************
->Change Data and time: 
->Function:发起竞争随机接入，发送msg1，开启RAR接收窗口
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void doStartContentionBasedRandomAccessProcedure() //modified by HQ  20140419
{
	FIN(doStartContentionBasedRandomAccessProcedure());
	SV_PTR_GET(rlc_mac_sv);
	RACH_ConfigDedicated *randomaccess_info=(RACH_ConfigDedicated *)fsm_mem_alloc(sizeof(RACH_ConfigDedicated));
	int mg3_buffer;
	//print_tran_info("[UEMAC][contention_ra]startCRA IDLE -> CRA");//testing code
	if(SV(PREAMBLE_TRANSMISSION_COUNTER )== 0)   //随机接入的初始化
	{ 
		SV(PREAMBLE_TRANSMISSION_COUNTER )=1; //将PREAMBLE_TRANSMISSION_COUNTER置1 
		SV(backoff_index) = 0;  //UE保存的backoff置为0ms    
		mg3_buffer=0;   //清空MSG3缓存//还未实现
	}
	fsm_printf("[UEMAC][contention_ra]RA counter :%d\n",SV(PREAMBLE_TRANSMISSION_COUNTER ));//testing code
	randomaccess_info->ra_PreambleIndex=RandomlySelectRaPreamble();//随机选择rapreamble
	SV(RA_info.ra_PreambleIndex)=randomaccess_info->ra_PreambleIndex;//保留rapid,用作比对RAR //HQ 20140506 
	
	/*for test*/
	//randomaccess_info->ra_PreambleIndex=2;//for test 20141103
	//SV(RA_info.ra_PreambleIndex)=randomaccess_info->ra_PreambleIndex;//for test20141103
	/*for test*/
	fsm_printf("[UEMAC][contention_ra]MAC ra_PreambleIndex:%d\n",SV(RA_info.ra_PreambleIndex));//test code
	/*randomaccess_info->ra_PRACHMaskIndex=0;
	SV(RA_info.ra_PRACHMaskIndex)=randomaccess_info->ra_PRACHMaskIndex;*/
	SendRaPreamble(randomaccess_info);//将随机接入的参数指示给物理层
	StartWaitingForRaResponse();//start waiting for the RAR
	FOUT;
}


/*
******************************
->Change Data and time: 
->Function:非竞争的 接收RAR失败处理函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void NC_RaResponseFailed() 
{
	FIN(NC_RaResponseFailed());
	SV_PTR_GET(rlc_mac_sv);
	u32 PreambleTransMax=SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.preambleTransMax);
	print_tran_info("[UEMAC][nc_ra_failed]NCRA -> IDLE");//testing code
	if(SV(WaitforRAresponseTimer.flag)==true)//如果是定时器超时，则设置相应的标志 因为如果未超时 定时器应该会被取消 标志被设置为false
	{ 
		SV(WaitforRAresponseTimer.flag)= false;
		SV(WaitforRAresponseTimer.timer_sign)= NULL;
	}
	SV(PREAMBLE_TRANSMISSION_COUNTER)++ ;
	if (SV(PREAMBLE_TRANSMISSION_COUNTER)==PreambleTransMax + 1)
	{
		//modified by HQ    	
		fsm_printf("[UEMAC][nc_ra_failed]MAC:notice RRC has try RA for max times\n");//testing code
		// fsm_do_ioctrl(STRM_TO_RRC, IOCCMD_MACtoRRC_RandomAcc_Fail  , NULL , 0);//Notify RRC the random access failed
		//modified by HQ
		SV(RA_info.ra_PreambleIndex)=0;//HQ 20140506 //清除之前的参数值
		SV(RA_info.ra_PRACHMaskIndex)=0; //HQ 20140506 //清除之前的参数值 
	}
	else
	{
		fsm_printf("[UEMAC][nc_ra_failed]MAC:start a new NCRA \n");//testing code 
		fsm_schedule_self(100,Start_NonContentionBase_RandomAcc);//trigger a new random access procedure
	}
	FOUT;
}


/*
******************************
->Change Data and time: 
->Function:竞争的 接收RAR失败处理函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void C_RaResponseFailed() 
{
	FIN(C_RaResponseFailed());
	SV_PTR_GET(rlc_mac_sv);
	u32 PreambleTransMax=SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.preambleTransMax);
	u16 backoff_time=0,backoff_param=0;//单位为ms 根据backoff_index获得
	print_tran_info("[UEMAC][c_ra_failed]MAC:CRA -> IDLE RAR receive fail");//testing code
	if(SV(WaitforRAresponseTimer.flag)==true)//如果是定时器超时，则设置相应的标志
	{
		SV(WaitforRAresponseTimer.flag)= false;
		SV(WaitforRAresponseTimer.timer_sign)= NULL;
	}
	SV(PREAMBLE_TRANSMISSION_COUNTER)++ ;
	if(SV(PREAMBLE_TRANSMISSION_COUNTER)==PreambleTransMax + 1)
		//modified by HQ
		fsm_printf("[UEMAC][c_ra_failed]MAC:notice RRC has try RA for max times\n");//testing code
		//fsm_do_ioctrl(STRM_TO_RRC, IOCCMD_MACtoRRC_RandomAcc_Fail  , NULL , 0);//Notify RRC the random access failed
	//modified by HQ
	else
	{ 
		backoff_param=BackoffTable[SV(backoff_index)]; //get the backoff parameter from BackoffTable 
		//fsm_printf("[UEMAC][c_ra_failed]MAC:back off parame : %d \n",backoff_param);//testing code
		if(backoff_param != 0)//避免除数为零
		{
			fsm_get_random_bytes(&backoff_time , sizeof(backoff_time)); //get a random number with 16bit.
			backoff_time=backoff_time % backoff_param;    //在0-backoff_param之间随机选择一个回退值
			fsm_printf("[UEMAC][c_ra_failed]MAC:with %d byte,get a random backoff:%d \n",sizeof(backoff_time),backoff_time);//testing code
		}
		fsm_schedule_self(backoff_time*100,Start_ContentionBase_RandomAcc);//trigger a new random access procedure as backoff_param indicates
		//若考虑ra-preamble分组的选择，参考协议，这里可增加代码
	}
/*    else 
    {
     backoff_param=BackoffTable[SV(backoff_index)]; //get the backoff parameter from BackoffTable  
     fsm_get_random_bytes(&backoff_time,sizeof(backoff_time)); //get a random number with 16bit.
     fsm_printf("with %d byte,get a random backoff:%d \n",sizeof(backoff_time),backoff_time);//testing code
     backoff_time=backoff_time % backoff_param;    //在0-backoff_param之间随机选择一个回退值
     if(backoff_time==0)
       fsm_schedule_self(100,Start_ContentionBase_RandomAcc);//避免时间参数为0//表示不退避
     else 
       fsm_schedule_self(backoff_time*100,Start_ContentionBase_RandomAcc);//trigger a new random access procedure as backoff_param indicates
    }
*/
FOUT;
}

/*发送msg3,发送时需要备份SDU用于竞争解决，实现遇到的问题在于 发送后需要启动CR定时器 */ 
/*目前复用功能函数，是采用遍历的方式，根据LCID进行复用，包括CCCH */
/*另外一种解决方案，增加一个发送状态SENDMSG3 上层数据包到达 则可以转移到CRA状态 */

//方案一
/*
static void send_msg3() //defined by HQ 20140430 
{
  FIN(send_msg3());   
  SV_PTR_GET(rlc_mac_sv);
   if(true==SV(UL_resource_info.resource_flag))//检查是否有资源可用
   {
      DoResourceAllocation(SV(UL_resource_info.m_tbSize)); //进行资源分配 然后回复RLC_Data_REQ
       //备份Msg3,即CCCH上来自RLC的SDU  //需要在RLC下发SDU后才可以进行该操作
      //组装MAC PDU   
      //启动竞争解决定时器
      SV(CRTimer.time_value)=SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.mac_ContentionResolutionTimer); //单位ms
      SV(CRTimer.timer_sign)=fsm_schedule_self(SV(CRTimer.time_value)*100+100, CRTimer_Expire);
      //这里定时时间增加1ms,考虑RLC下发SDU的交互时间，相当于定时器的提前一点启动了，会带来一些精度误差。 
      SV(CRTimer.flag)=true;
   }
   else 
   	  print_tran_info("no UL_Resource,error!");
FOUT;
}
*/

//方案2
/*
******************************
->Change Data and time: 
->Function:成功接收RAR后，发送MSG3之前的处理函数
->Change:
->Details:区分MSG3是由上层RLC发下来(第一次随机接入) 还是来自MAC的msg3缓存
->input:
->output:
->Special:
*******************************
*/
static void waitsending_msg3() //defined by HQ 20140430 
{
	RNTI_indication *data_ptr;
	FIN(waitsending_msg3());   
	SV_PTR_GET(rlc_mac_sv);
	RARTA_handleB();//竞争接入时，接收RAR成功，对于TA命令的处理 
	//print_tran_info("[UEMAC][waitsend_msg3]CRA->SENDMSG3");
	//HQ modified start 20140709 关于T_RNTI的处理
	/* if(SV(C_RNTI) == 0)//没有C_RNTI
	{
		data_ptr = (RNTI_indication *)fsm_mem_alloc(sizeof(RNTI_indication));
		data_ptr->rnti_type = 2; 
		data_ptr->rnti_value = SV(T_C_RNTI);
		fsm_do_ioctrl(STRM_TO_SRIO, IOCCMD_MACtoPHY_RNTI_Indicate, (void *)data_ptr, sizeof(RNTI_indication));
		//将T_C_RNTI告知物理层
	}*/
	//HQ HQ modified end 20140709 
	//LHL modified 20140717 msg3是用T_C_RNTI加扰的
	if( SV(T_C_RNTI)!=0)//保险起见
	{
		data_ptr = (RNTI_indication *)fsm_mem_alloc(sizeof(RNTI_indication));
		data_ptr->rnti_type = 2; 
		data_ptr->rnti_value = SV(T_C_RNTI);
		fsm_do_ioctrl(STRM_TO_SRIO, IOCCMD_MACtoPHY_RNTI_Indicate, (void *)data_ptr, sizeof(RNTI_indication));
		fsm_mem_free(data_ptr);
	}
	//HQ HQ modified end 20140717
	fsm_printf("[UEMAC][waitsend_msg3]MAC SV(PREAMBLE_TRANSMISSION_COUNTER):%d\n",SV(PREAMBLE_TRANSMISSION_COUNTER));
	/*if(SV(PREAMBLE_TRANSMISSION_COUNTER) == 1 )//第一次接入
	{
		if(SV(DATA_WAIT_ALLOCATION)==true && SV(UL_resource_info.resource_flag)==true)//检查是否有资源可用
		{
			//DoresourceAllocation()函数的参数是，以byte为单位，UL_resource_info.m_tbsize以bit为单位
		if(SV(C_RNTI)!= 0 ) //预留发C-RNTI控制单元的资源 )
		{
			SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
	    		DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)-24));//20140531 ,20140718LHL:32是考虑MAC控制单元长度
		}
		else//RRC未连接，不预留C_RNTI控制单元2010721 LHL
		{
			SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
	    		DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)));//20140531 ,20140718LHL:32是考虑MAC控制单元长度
		}
		fsm_printf("MAC SEND MSG3:DoResourceAllocation \n");
		}
		else 
	   		print_tran_info("MAC SEND MSG3::no UL_Resource,error!");//testing code
	}
	else if(SV(PREAMBLE_TRANSMISSION_COUNTER) > 1 )
	{  	
		SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140721LHL 单位为byte,复用模块使用    	
  		fsm_printf("MAC SEND MSG3:this isn't the first time to do contention based random access ,needn't to  DoResourceAllocation \n");
	}*/ //modified by lhl 20140725

	if(SV(ra_allocation_res)==0)
	{
		if(SV(DATA_WAIT_ALLOCATION)==true && SV(UL_resource_info.resource_flag)==true)//检查是否有资源可用
		{
			//DoresourceAllocation()函数的参数是，以byte为单位，UL_resource_info.m_tbsize以bit为单位
			SV(ra_allocation_res)=1;
			if(SV(C_RNTI)!= 0 ) //预留发C-RNTI控制单元的资源 )
			{
				SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
				DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)-24));//20140531 ,20140718LHL:32是考虑MAC控制单元长度
				SV(UL_resource_info.resource_flag)=false;
			}
			else//RRC未连接，不预留C_RNTI控制单元2010721 LHL
			{
				SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
				DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)));//20140531 ,20140718LHL:32是考虑MAC控制单元长度
				SV(UL_resource_info.resource_flag)=false;
			}
			//fsm_printf("[UEMAC][waitsend_msg3]DoResourceAllocation \n");
		}
		else 
			print_tran_info("[UEMAC][waitsend_msg3]no UL_Resource,error!");//testing code
	}
	else
	{
		SV(ra_allocation_res)=2;
		SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140721LHL 单位为byte,复用模块使用 
		//fsm_schedule_self(0, RA_Send_Msg3);//LHL 20140726 ,非第一次发MSG3，由MAC触发
		retransmit_msg3(); 	//call static retransmit_msg3(void) function 20150901 
		fsm_printf("[UEMAC][waitsend_msg3]this isn't the first time to do contention based random access ,needn't to  DoResourceAllocation \n");
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:发送MSG3
->Change:
->Details:注意MAC要拷贝发送的MSG3以防重传MSG3
->input:
->output:
->Special:
*******************************
*/
void send_msg3(FSM_PKT *skb) //defined by HQ 20140430  
{
	//针对msg3的不同内容，竞争解决判定的方法有所不同
	//目前实现了: msg3的内容是CCCH上的SDU---->其他的也实现了
	int	skb_len=0,len=0; 
	//FSM_PKT *skb;
	FIN(send_msg3());

	SV_PTR_GET(rlc_mac_sv);
	//备份Msg3,即CCCH上来自RLC的SDU  //需要在RLC下发SDU后才可以进行该操作
	//组装MAC PDU 
	//fsm_printf("[UEMAC][send_msg3]send msg3,SENDMSG3->CRA \n");
	//skb = fsm_pkt_get();
	// fsm_pkt_destroy(fsm_pkt_get());
	// fsm_printf("in fact it was destroyed ");
	fsm_printf("[UE MAC][send_msg3]RA ALLOCATION RESULT:%d\n",SV(ra_allocation_res));
	if(SV(ra_allocation_res)==1)//modified by lhl 20140725
	{
		//备份msg3的sk_buf
		//20140709 adition start by HQ 为了以整个sk_buff的结构来备份msg3  用于第二次以及以后的竞争随机接入的MSG3
		/*SV(msg3_buf_ptr)= fsm_pkt_create(128);
		skb_len = skb->data - skb->head ;
		fsm_skb_reserve(SV(msg3_buf_ptr),skb_len);
		fsm_mem_cpy(SV(msg3_buf_ptr)->head,skb->head,skb_len);
		skb_len = skb->tail - skb->data;
		fsm_skb_put(SV(msg3_buf_ptr),skb_len);
		fsm_mem_cpy(SV(msg3_buf_ptr)->data,skb->data,skb_len);*/
		if(SV(msg3_buf_ptr)!=NULL)
			fsm_pkt_destroy(SV(msg3_buf_ptr));
		SV(msg3_buf_ptr)=skb;//3
		skb=fsm_pkt_duplicate(SV(msg3_buf_ptr));//20140729 lhl
	
		//20140709 adition end  by HQ 	

		len=sizeof(RLCtoMAC_IciMsg);
		//函数lteMacCcchSend(skb,len)检测SDU是不是ccch上的数据，是则会返回0,并备份SDU，不是则返回1	
		if(lteMacCcchSend(skb,len) && SV(C_RNTI)!= 0 ) //如果不是CCCH上的数据 复用C_RNTI_MAC控制单元
		{	
			//fsm_printf("[LHL]C_RNTI CONTROL ELEMENT!\n");
			SV(MAC_CE_Tags)[CRNTI_CE].addinPDU_flag=true;
			//SV(MAC_CE_Tags)[CRNTI_CE].LCID=CCCH_LCID;
			SV(MAC_CE_Tags)[CRNTI_CE].LCID=C_RNTI_LCID;
			SV(MAC_CE_Tags)[CRNTI_CE].MAC_CE_ptr=(void *)&SV(C_RNTI);
			
			
			//fsm_printf("[MAC FSM]RNTI IN SV:%d\n",SV(C_RNTI));
			
			
		}
		SV(sendmsg3)=1;
		complexUeMacPdu(skb);
		
	
		if(SV(Tbsize_Complex)!=0)
			SV(Tbsize_Complex)=0;//复用之后，重置TBSIZE
		fsm_printf("[UEMAC][send_msg3]:this is the first time to do CRA \n");//testing code 
	}
	else if(SV(msg3_buf_ptr) != NULL && SV(ra_allocation_res)==2)  
	{
		len=sizeof(RLCtoMAC_IciMsg);
		skb=fsm_pkt_duplicate(SV(msg3_buf_ptr));//20140729 lhl
		//函数lteMacCcchSend(skb,len)检测SDU是不是ccch上的数据，是则会返回0,并备份SDU，不是则返回1	
		if(lteMacCcchSend(skb,len) && SV(C_RNTI)!= 0 ) //如果不是CCCH上的数据 复用C_RNTI_MAC控制单元
		{
			//fsm_printf("[LHL]C_RNTI CONTROL ELEMENT!\n",SV(ra_allocation_res));
			SV(MAC_CE_Tags)[CRNTI_CE].addinPDU_flag=true;
			//SV(MAC_CE_Tags)[CRNTI_CE].LCID=CCCH_LCID;
			SV(MAC_CE_Tags)[CRNTI_CE].LCID=C_RNTI_LCID;
			SV(MAC_CE_Tags)[CRNTI_CE].MAC_CE_ptr=(void *)&SV(C_RNTI);
		}
		SV(sendmsg3)=1;	
		complexUeMacPdu(skb); 	
		if(SV(Tbsize_Complex)!=0)//?????????????????????????????待考虑
			SV(Tbsize_Complex)=0;//复用之后，重置TBSIZE ，重传怎么办 没有资源???
		fsm_printf("[UEMAC][send_msg3]:this isn't the first time to CRA \n");//testing code 
	}
	//testing code 收到包后直接转发给SRIO
	//  fsm_pkt_send(fsm_pkt_get(), STRM_TO_SRIO);
	//启动竞争解决定时器
	SV(CRTimer.time_value)=SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.mac_ContentionResolutionTimer); //单位ms
	//fsm_printf("[UEMAC][send_msg3]CRtimer value:%d\n",SV(CRTimer.time_value));
	SV(CRTimer.timer_sign)=fsm_schedule_self(SV(CRTimer.time_value)*100, CRTimer_Expire);//死机
	SV(CRTimer.flag)=true;
	FOUT;
}

static void retransmit_msg3(void) 
{
	//针对msg3的不同内容，竞争解决判定的方法有所不同
	//目前实现了: msg3的内容是CCCH上的SDU---->其他的也实现了
	int	skb_len=0,len=0; 
	FSM_PKT *skb;
	FIN(send_msg3());

	SV_PTR_GET(rlc_mac_sv);
	//备份Msg3,即CCCH上来自RLC的SDU  //需要在RLC下发SDU后才可以进行该操作
	//组装MAC PDU 
	//fsm_printf("[UEMAC][send_msg3]send msg3,SENDMSG3->CRA \n");
	//skb = fsm_pkt_get();
	// fsm_pkt_destroy(fsm_pkt_get());
	// fsm_printf("in fact it was destroyed ");
	fsm_printf("[UE MAC]RA ALLOCATION RESULT:%d\n",SV(ra_allocation_res));
	
	//if won't excute. SV(ra_allocation_res)==2, time:20150901.
	if(SV(ra_allocation_res)==1)//modified by lhl 20140725
	{
		//备份msg3的sk_buf
		//20140709 adition start by HQ 为了以整个sk_buff的结构来备份msg3  用于第二次以及以后的竞争随机接入的MSG3
		/*SV(msg3_buf_ptr)= fsm_pkt_create(128);
		skb_len = skb->data - skb->head ;
		fsm_skb_reserve(SV(msg3_buf_ptr),skb_len);
		fsm_mem_cpy(SV(msg3_buf_ptr)->head,skb->head,skb_len);
		skb_len = skb->tail - skb->data;

		fsm_skb_put(SV(msg3_buf_ptr),skb_len);
		fsm_mem_cpy(SV(msg3_buf_ptr)->data,skb->data,skb_len);*/
		if(SV(msg3_buf_ptr)!=NULL)
			fsm_pkt_destroy(SV(msg3_buf_ptr));
		SV(msg3_buf_ptr)=skb;//3
		skb=fsm_pkt_duplicate(SV(msg3_buf_ptr));//20140729 lhl
	
		//20140709 adition end  by HQ 	

		len=sizeof(RLCtoMAC_IciMsg);
		//函数lteMacCcchSend(skb,len)检测SDU是不是ccch上的数据，是则会返回0,并备份SDU，不是则返回1	
		if(lteMacCcchSend(skb,len) && SV(C_RNTI)!= 0 ) //如果不是CCCH上的数据 复用C_RNTI_MAC控制单元
		{	
			//fsm_printf("[LHL]C_RNTI CONTROL ELEMENT!\n");
			SV(MAC_CE_Tags)[CRNTI_CE].addinPDU_flag=true;
			//SV(MAC_CE_Tags)[CRNTI_CE].LCID=CCCH_LCID;
			SV(MAC_CE_Tags)[CRNTI_CE].LCID=C_RNTI_LCID;
			SV(MAC_CE_Tags)[CRNTI_CE].MAC_CE_ptr=(void *)&SV(C_RNTI);
			
			
			//fsm_printf("[MAC FSM]RNTI IN SV:%d\n",SV(C_RNTI));
			
			
		}
		SV(sendmsg3)=1;
		complexUeMacPdu(skb);
		
	
		if(SV(Tbsize_Complex)!=0)
			SV(Tbsize_Complex)=0;//复用之后，重置TBSIZE
		fsm_printf("[UEMAC][send_msg3]:this is the first time to do CRA \n");//testing code 
	}
	else if(SV(msg3_buf_ptr) != NULL && SV(ra_allocation_res)==2)  
	{
		len=sizeof(RLCtoMAC_IciMsg);
		skb=fsm_pkt_duplicate(SV(msg3_buf_ptr));//20140729 lhl
		//函数lteMacCcchSend(skb,len)检测SDU是不是ccch上的数据，是则会返回0,并备份SDU，不是则返回1	
		if(lteMacCcchSend(skb,len) && SV(C_RNTI)!= 0 ) //如果不是CCCH上的数据 复用C_RNTI_MAC控制单元
		{
			//fsm_printf("[LHL]C_RNTI CONTROL ELEMENT!\n",SV(ra_allocation_res));
			SV(MAC_CE_Tags)[CRNTI_CE].addinPDU_flag=true;
			//SV(MAC_CE_Tags)[CRNTI_CE].LCID=CCCH_LCID;
			SV(MAC_CE_Tags)[CRNTI_CE].LCID=C_RNTI_LCID;
			SV(MAC_CE_Tags)[CRNTI_CE].MAC_CE_ptr=(void *)&SV(C_RNTI);
		}
		SV(sendmsg3)=1;	
		complexUeMacPdu(skb); 	
		if(SV(Tbsize_Complex)!=0)//?????????????????????????????待考虑
			SV(Tbsize_Complex)=0;//复用之后，重置TBSIZE ，重传怎么办 没有资源???
		fsm_printf("[UEMAC][send_msg3]:this isn't the first time to CRA \n");//testing code 
	}
	//testing code 收到包后直接转发给SRIO
	//  fsm_pkt_send(fsm_pkt_get(), STRM_TO_SRIO);
	//启动竞争解决定时器
	SV(CRTimer.time_value)=SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.mac_ContentionResolutionTimer); //单位ms
	//fsm_printf("[UEMAC][send_msg3]CRtimer value:%d\n",SV(CRTimer.time_value));
	SV(CRTimer.timer_sign)=fsm_schedule_self(SV(CRTimer.time_value)*100, CRTimer_Expire);//死机
	SV(CRTimer.flag)=true;
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:清空HARQ缓存
->Change:
->Details:暂时没有实现
->input:
->output:
->Special:
*******************************
*/
static void cleanharqBuff()
{
	FIN(cleanharqBuff());
	//print_tran_info("[UEMAC][cleanharqbuf]clean HARQ buffer");//testing code
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:竞争解决失败的处理函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void ContentionFailed() 
{
	FIN(ContentionFailed());
	SV_PTR_GET(rlc_mac_sv);
	int result=1;//TEST
	u32 PreambleTransMax=SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.preambleTransMax);
	u32 backoff_time=0,backoff_param=0;//单位为ms 根据backoff_index获得

	fsm_printf("[UEMAC][con_failed]CRA -> IDLE contention fail ");//testing code 
	if(SV(CRTimer.flag)==true) //LHL 20140729 CR定时器超时，则相应地设置标志 
	{
		fsm_printf("[UEMAC][con_failed]because CRtimer expire \n");
		SV(CRTimer.timer_sign)= NULL;
		SV(CRTimer.flag)= false;
	}
	//if(SV(TATimer.time_value)== SV(TA_fromRAR) && SV(TATimer.flag)==true) //竞争失败，则停止之前启动的TATimer MAC协议5.1.6
	
	if(SV(TATimer.flag)==true) 
	{//HQ 20140428 
		//fsm_printf("[UEMAC][con_failed]CR fail,stop TAtimer\n");//testing code 
		if(SV(TATimer.timer_sign) != NULL)
		{
			result=fsm_schedule_cancel (SV(TATimer.timer_sign));
		}
		//fsm_printf("[UEMAC][contentionfailed]result:%d\n",result);//TEST
		SV(TATimer.timer_sign)= NULL;
		SV(TATimer.flag)=false;
		SV(TATimer.time_value)=0;
	}
	SV(T_C_RNTI)=0;
	cleanharqBuff();
	SV(PREAMBLE_TRANSMISSION_COUNTER)++;
	if (SV(PREAMBLE_TRANSMISSION_COUNTER)==PreambleTransMax + 1)//modified by HQ
	{
		//fsm_printf("[UEMAC][con_failed]:notice RRC has try RA for max times\n");//testing code 
		SV(sendmsg3)=0;
		SV(ratype.ra_type)=0;
		SV(pdcch_rnti)=0;
		SV(ra_allocation_res)=0;
		fsm_do_ioctrl(STRM_TO_RRC, IOCCMD_MACtoRRC_RandomAcc_Fail  , NULL , 0);//Notify RRC the random access failed//add on 20141208
	}
	
	//modified by HQ
	else
	{
		SV(ra_allocation_res)=1;
		backoff_param=BackoffTable[SV(backoff_index)]; //get the backoff parameter from BackoffTable  
		//fsm_printf("[UEMAC][con_failed]:back off parame : %d \n",backoff_param);//testing code
		if(backoff_param != 0)//避免除数为零
		{
			fsm_get_random_bytes(&backoff_time , sizeof(backoff_time)); //get a random number with 16bit.
			backoff_time=backoff_time % backoff_param;    //在0-backoff_param之间随机选择一个回退值
			//fsm_printf("[UEMAC][con_failed]:with %d byte,get a random backoff:%d \n",sizeof(backoff_time),backoff_time);//testing code
		}
		fsm_schedule_self(backoff_time*100,Start_ContentionBase_RandomAcc);//trigger a new random access procedure as backoff_param indicates
		//若考虑ra-preamble分组的选择，参考协议，这里可增加代码
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:非竞争随机接入成功的处理函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void ncra_success()
{
	FIN(ncra_success);
	SV_PTR_GET(rlc_mac_sv);
	RARTA_handleA();//非竞争接入时，接收RAR成功 即随机接入成功 TA命令的处理 
	SV(RA_info.ra_PreambleIndex) = 0;	//清除ra_preamble index ra_prach_mask_index
	SV(RA_info.ra_PRACHMaskIndex) = 0; 
	SV(PREAMBLE_TRANSMISSION_COUNTER) = 0;//将随机接入次数置0
	SV(T_C_RNTI) = 0;//丢弃T_C_RNTI 
	//fsm_printf("[UEMAC][nc_success]:\n noncontention based random access success \n");//testing code 
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:竞争随机接入成功的处理函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static void cra_success()
{
	RNTI_indication *data_ptr;
	FIN(cra_success());
	SV_PTR_GET(rlc_mac_sv);
	SV(RA_info.ra_PreambleIndex) = 0;	//清除ra_preamble index ra_prach_mask_index
	SV(RA_info.ra_PRACHMaskIndex) = 0; 
	SV(PREAMBLE_TRANSMISSION_COUNTER) = 0;//将随机接入次数置0
	cleanharqBuff();//清空HARQ的msg3的缓存
	SV(ra_allocation_res)=0;
	if(SV(msg3_buf_ptr) != NULL)
	{
		fsm_pkt_destroy(SV(msg3_buf_ptr));//modified by lhl 20140717
		SV(msg3_buf_ptr) = NULL; //清空msg3的sk_buf缓存
	}
	if(SV(C_RNTI) == 0)
		SV(C_RNTI) = SV(T_C_RNTI); //T_C_RNTI升级 
	SV(T_C_RNTI) = 0;//丢弃T_C_RNTI
	if(SV(C_RNTI) > 0) //将C_rnti指示给物理层 
	{
		data_ptr = (RNTI_indication *)fsm_mem_alloc(sizeof(RNTI_indication));
		data_ptr->rnti_type = 1; 
		data_ptr->rnti_value = SV(C_RNTI);
		fsm_do_ioctrl(STRM_TO_SRIO, IOCCMD_MACtoPHY_RNTI_Indicate, (void *)data_ptr, sizeof(RNTI_indication));//将C_RNTI告知物理层
		fsm_mem_free(data_ptr);//LHL20141018
	}
	if(SV(CRTimer.flag)==true) //停止CR定时器
	{
		SV(CRTimer.timer_sign)= NULL;
		SV(CRTimer.flag)= false;
		SV(CRTimer.time_value)=0;
	}
	SV(sendmsg3)=0;//将MSG3标志置为0
	SV(ratype.ra_type)=0;//随机接入类型置为0 
	SV(pdcch_rnti)=0;
	fsm_printf("[UEMAC][cra_success]:contention based random access success \n");//testing code 
	FOUT;
}
/********************************************************************/

/*************************HQ 20140428 **************************/
/*
******************************
->Change Data and time: 20141021
->Function:非竞争接入下处理RAR的TA
->Change:TA定时器周期，为RRC配置
->Details:
->input:
->output:
->Special:
*******************************
*/
static void RARTA_handleA()  
{
	FIN(RARTA_handleA());	
	SV_PTR_GET(rlc_mac_sv);	
	if(SV(TATimer.flag)==true && SV(TATimer.timer_sign) != NULL)//定时器还未超时 则先取消
		fsm_schedule_cancel(SV(TATimer.timer_sign));
	//SV(TATimer.time_value)=SV(TA_fromRAR);//LHL modified 20141021
	
	SV(TATimer.time_value)=SV(TA_Periodic_time);//LHL 20141021
	SV(TATimer.timer_sign)=fsm_schedule_self(SV(TATimer.time_value)*100,TimeAlignmentTimer_Expire);
	SV(TATimer.flag)=true;
	//fsm_printf("[UEMAC][rarta_handlea]:NCRA handle TA command \n");//testing code 
	FOUT;
}
/*
******************************
->Change Data and time: 2014.10.21
->Function:竞争接入下处理RAR的TA
->Change:TA定时器周期，由RRC配置
->Details:
->input:
->output:
->Special:
*******************************
*/
static void RARTA_handleB()  
{
	FIN(RARTA_handleB());
	/**delete for test**/
	
	SV_PTR_GET(rlc_mac_sv);
	if(SV(TATimer.flag)==false)
	{ //TA定时器未启动,则应用该TA
		//SV(TATimer.time_value)=SV(TA_fromRAR);//LHL modified 20141021,TA定时器周期由RRC配置的，值保持不变
		SV(TA_value)=SV(TA_fromRAR)*16;//
		SV(TATimer.time_value)=SV(TA_Periodic_time);
		SV(TATimer.timer_sign)=fsm_schedule_self(SV(TATimer.time_value)*100,TimeAlignmentTimer_Expire);
		SV(TATimer.flag)=true;
	}
	//else SV(TA_value)=0;//定时器在运行，则忽略该TA ,20141021
	//fsm_printf("[UEMAC][rarta_handle]:CRA handle TA command \n");//testing code 
	
	FOUT;
}


/*************************HQ 20140428 end **************************/

/*********************************************************************/
/* test functions __HuangQiong */

/*
******************************
->Change Data and time: 
->Function:打印当前时间
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void print_tran_info( const char *str)
{
	FIN( print_tran_info());
	int curtime=0;
	curtime=fsm_get_curtime();
	//fsm_printf("[UEMAC][print_info]%d ",curtime);
	fsm_printf(str);
	fsm_printf("\n");
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void random_test()   //HQ 20140410 测试从0-63中随机选取一个数
{
	FIN(random_test());
	u8 randNum[10];
	u32 i = 0;
	fsm_printf( "[UEMAC][ra_test]:Get some real random number.\n");

	for (i=0; i<10; i++)
	{
		fsm_get_random_bytes(&randNum[i], sizeof(char));
		randNum[i]=randNum[i]%64;
		fsm_printf("[UEMAC][ra_test]:We get random number: %d\n", randNum[i]);//testing code
	} 
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void RA_RRCconfig_testingfun() // 测试用的函数，获得RRC的配置信息 HQ 20140428 
{
	FIN(RA_RRCconfig_testingfun());
	SV_PTR_GET(rlc_mac_sv);
	SV(systemconfigInfo.RACHCommonConfigInfo.preambleInfo.numberOfRA_Preambles)= 64;
	SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.preambleTransMax)=8;
	SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.raResponseWindowSize)=800000;//8;测试需要 设置较长时间800s
	SV(systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.mac_ContentionResolutionTimer)=800000;// 800s;
	SV(systemconfigInfo.RACHCommonConfigInfo.maxHARQ_Msg3Tx)=5;
	SV(backoff_index)= 2 ; 
	fsm_printf("[UEMAC][ra_rrcconfig]:RACH config have done \n");//testing code
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void testdoStartContentionBasedRandomAccessProcedure()
{
	FIN(testdoStartContentionBasedRandomAccessProcedure());
	print_tran_info("IDLE -> CRA");
	//fsm_schedule_self(400,RARrev_Success);//testing code //设置为4ms,WAPTimer还未超时 
	//fsm_schedule_self(100000, RandomAcc_Fail );//testing code
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void testdoStartNonContentionBasedRandomAccessProcedure() //非接入，默认成功 
{
	FIN(testdoStartNonContentionBasedRandomAccessProcedure());
	print_tran_info("IDLE -> NCRA");
	//fsm_schedule_self(400,RARrev_Success);//testing code //设置为4ms,WAPTimer还未超时 
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:停止WaitforRAresponseTimer定时器
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void stop_WRAP_timer()
{
	FIN(stop_WRAP_timer());
	SV_PTR_GET(rlc_mac_sv);
	if(SV(WaitforRAresponseTimer.flag)==true)
	{
		fsm_schedule_cancel(SV(WaitforRAresponseTimer.timer_sign));
		SV(WaitforRAresponseTimer.flag)=false;
		SV(WaitforRAresponseTimer.timer_sign)=NULL;
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void NCRAtoCON_testingfun() 
{
	FIN(NCRAtoCON_testingfun());
	print_tran_info("NCRA -> CON");
	stop_WRAP_timer(); //该定时器的停止应该是在成功接收RAR后。
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:停止CRTimer定时器
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void stop_CR_timer()
{
	FIN(stop_CR_timer());
	SV_PTR_GET(rlc_mac_sv);
	if(SV(CRTimer.flag)==true)
	{
		fsm_schedule_cancel(SV(CRTimer.timer_sign));
		SV(CRTimer.flag)=false;
		SV(CRTimer.timer_sign)=NULL;
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:停止TATimer定时器
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void stop_TA_timer()
{
	FIN(stop_TA_timer());
	SV_PTR_GET(rlc_mac_sv);
	if(SV(TATimer.flag)==true)
	{
		fsm_schedule_cancel(SV(TATimer.timer_sign));
		SV(TATimer.flag)=false;
		SV(TATimer.timer_sign)=NULL;
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:停止RetxBSRTimer定时器
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void stop_RetxBSR_timer()
{
	FIN(stop_RetxBSR_timer());
	SV_PTR_GET(rlc_mac_sv);
	if(SV(RetxBSRTimer.flag)==true)
	{
		fsm_schedule_cancel(SV(RetxBSRTimer.timer_sign));
		SV(RetxBSRTimer.flag)=false;
		SV(RetxBSRTimer.timer_sign)=NULL;
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:停止PeriodicBSRTimer定时器
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void stop_PeriodicBSR_timer()
{
	FIN(stop_PeriodicBSR_timer());
	SV_PTR_GET(rlc_mac_sv);
	if(SV(PeriodicBSRTimer.flag)==true)
	{
		fsm_schedule_cancel(SV(PeriodicBSRTimer.timer_sign));
		SV(PeriodicBSRTimer.flag)=false;
		SV(PeriodicBSRTimer.timer_sign)=NULL;
	}
	FOUT;
}


/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void CRAtoCON_testingfun()
{
	FIN(NCRAtoCON_testingfun());
	print_tran_info("CRA -> CON");
	stop_CR_timer(); //该定时器的停止应该是在接收竞争解决单元确认竞争成功后。
	FOUT;
}




//testing code  
//适合场景 IOCTL_TEST文件中siro.c中的命令 -crareq是不带数据的， -ncrareq不能使用 
//  -sendraorder 命令是发给SRIO模块，SRIO模块则向MAC发送 IOCCMD_PDCCHtoMAC_RandomAcc_Req 并携带参数 
//可以配合 doStartNonContentionBasedRandomAccessProcedure()函数使用
/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void test1_idle_default_handler()
{
	void *tmpdata_ptr;
	FIN(idle_default_handler());
	SV_PTR_GET(rlc_mac_sv);
	print_tran_info("MAC:idle_default_handler");
	if(MAC_CLOSE)
	{
		print_tran_info("MAC FSM is closed "); //testing code
		ue_mac_sv_close();
	}
	if(MAC_PK_FROM_LOWER)
	{	//通过ICI信息判断包类型，应该是广播消息，调用发送函数，发给RLC
		print_tran_info("MAC:IDLE receive a data packet from PHYadapter"); //testing code
		//decomplexUeDataPdu(fsm_pkt_get());
		fsm_pkt_destroy(fsm_pkt_get());//testing code
	}
	//noted in 20150908
	/*if(MAC_PK_FROM_UPPER )
	{//在IDLE状态下应该是不会遇到这个情况,所以打印一条错误信息
		print_tran_info("MAC:IDLE receive a data packet from RLC, error, and then start CRA");//testing code 
		SV(ratype.ra_type)=2;
		fsm_schedule_self(0, Start_ContentionBase_RandomAcc);
		fsm_pkt_destroy(fsm_pkt_get());//testing code
	}*/
	if(IOCTRL_ARRIVAL)
	{//处理IOCTL命令
		print_tran_info("MAC:IDLE default ioctl hander");//testing code
		switch(fsm_ev_ioctrl_cmd())
		{
		//处理RLC的数据请求命令
		case IOCCMD_RLCtoMAC_datasend_Req :
			tmpdata_ptr = fsm_data_get();
			print_tran_info("MAC:IDLE receive a RLCdatareq");
			//DoRefreshRLCBuffserRequest((RlcBufferRequest *) tmpdata_ptr);//更新待发数据缓存信息
			fsm_data_destroy(tmpdata_ptr);
			fsm_schedule_self(0,Start_ContentionBase_RandomAcc); //触发随机接入 ，时间单位是10us 这个时间设置多长待究
		break;

		//处理PHY的RA请求命令
		case IOCCMD_PDCCHtoMAC_RandomAcc_Req : 	
				tmpdata_ptr = fsm_data_get();
				print_tran_info("IDLE receive a ncra order ");
				fsm_mem_cpy(&(SV(RA_info)),tmpdata_ptr,sizeof(RACH_ConfigDedicated));
				fsm_printf("RAPID: %d \n",SV(RA_info.ra_PreambleIndex));		
				if(tmpdata_ptr != NULL)
				fsm_data_destroy(tmpdata_ptr);//释放内存
				//fsm_schedule_self(0, Start_NonContentionBase_RandomAcc );//触发非竞争随机接入
		break; 
		//处理RRC的RA请求命令
		case IOCCMD_RRCtoMAC_RandomAcc_Req :
				print_tran_info("MAC:IDLE receive a cra order ");
				fsm_schedule_self(0, Start_ContentionBase_RandomAcc );//触发竞争随机接入		
		break;

		//上面只是测试用的代码，用以避过do_ioctl函数引起的死机问题
		//下面是正常的代码
		/*//处理PHY的RA请求命令
		case IOCCMD_PDCCHtoMAC_RandomAcc_Req: 	
		//两条命令的处理步骤一样 所以此处没有break	  
		//处理RRC的RA请求命令
		case IOCCMD_RRCtoMAC_RandomAcc_Req :
		    tmpdata_ptr = fsm_data_get();
            print_tran_info("IDLE receive a RA req from other layer ");
            if(tmpdata_ptr > 0)//并指示了关于接入的前导码
            {
            	print_tran_info("IDLE receive a ncra order ");
            	fsm_mem_cpy(&(SV(RA_info)),tmpdata_ptr,sizeof(RACH_ConfigDedicated));
            	fsm_printf("the RAPID is %d\n",SV(RA_info.ra_PreambleIndex));
        		//fsm_schedule_self(0, Start_NonContentionBase_RandomAcc );//触发非竞争随机接入 //这条语句会造成死机，暂注释掉
				if(tmpdata_ptr != NULL)
				{
				  	fsm_data_destroy(tmpdata_ptr);//释放内存
					fsm_printf("fsm_ev_data,then free the memory\n");
				}

			}
			else //没有指示关于接入的前导码
			{
				print_tran_info("IDLE receive a cra order ");
				fsm_schedule_self(0, Start_ContentionBase_RandomAcc );//触发竞争随机接入
			}			
		break;

		*/

		//处理其他与状态无关的命令
		default:
			ioctrl_handler();  	
		break;
		}
	}
FOUT;
}

//testing code  适合IOCTL_TEST文件中siro.c中的命令都是不带数据的情况 
//并且配合下面的 testdostart......函数使用
/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
void test2_idle_default_handler()
{
	void *tmpdata_ptr;
	FIN(idle_default_handler());

	SV_PTR_GET(rlc_mac_sv);
	print_tran_info("idle_default_handler");
	if(MAC_CLOSE)
	{
		print_tran_info("MAC FSM is closed "); //testing code
		ue_mac_sv_close();
	}
	if(MAC_PK_FROM_LOWER)
	{	//通过ICI信息判断包类型，应该是广播消息，调用发送函数，发给RLC
		print_tran_info("IDLE receive a data packet from PHYadapter"); //testing code
		//decomplexUeDataPdu(fsm_pkt_get());
		fsm_pkt_destroy(fsm_pkt_get());//testing code
	}
	//noted in 20150908
	/*if(MAC_PK_FROM_UPPER )
	{//在IDLE状态下应该是不会遇到这个情况,所以打印一条错误信息
		print_tran_info("IDLE receive a data packet from RLC, error, and then start CRA");//testing code 
		fsm_schedule_self(0, Start_ContentionBase_RandomAcc);
		fsm_pkt_destroy(fsm_pkt_get());//testing code
	}*/
	if(IOCTRL_ARRIVAL)
	{//处理IOCTL命令
		print_tran_info("IDLE default ioctl hander");//testing code
		switch(fsm_ev_ioctrl_cmd())
		{
		//处理RLC的数据请求命令
		case IOCCMD_RLCtoMAC_datasend_Req :
			tmpdata_ptr = fsm_data_get();
			print_tran_info("IDLE receive a RLCdatareq");
			//DoRefreshRLCBuffserRequest((RlcBufferRequest *) tmpdata_ptr);//更新待发数据缓存信息
			fsm_data_destroy(tmpdata_ptr);
			fsm_schedule_self(0,Start_ContentionBase_RandomAcc); //触发随机接入 ，时间单位是10us 这个时间设置多长待究
		break;
		//下面是测试的代码
		//处理PHY的RA请求命令
		case NCRATEST:
				print_tran_info("IDLE receive a ncra test ");


				//fsm_schedule_self(0, Start_NonContentionBase_RandomAcc );//这句会造成死机，原因不明 
		break;
		case IOCCMD_PDCCHtoMAC_RandomAcc_Req : 	
				print_tran_info("IDLE receive a ncra order ");
				//fsm_schedule_self(0, Start_NonContentionBase_RandomAcc );//触发非竞争随机接入
		break; 
		//处理RRC的RA请求命令
		case IOCCMD_RRCtoMAC_RandomAcc_Req :
				print_tran_info("IDLE receive a cra order ");
				fsm_schedule_self(0, Start_ContentionBase_RandomAcc );//触发竞争随机接入
		break;
		//上面只是测试用的代码，用以避过do_ioctl函数引起的死机问题
		//处理其他与状态无关的命令
		default:
			ioctrl_handler();
		break;
		}
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:测试函数
->Change:
->Details:
->input:
->output:
->Special:
*******************************
*/
static ioctrl_test()//通过用户空间的命令，产生驱动状态机的事件，测试状态机
{
	FIN(ioctrl_test());
	switch(fsm_ev_ioctrl_cmd())
	{
		case TESTCMD_RAR_REVF  :
			fsm_printf("receive RAR failed\n");
			fsm_schedule_self(50, RandomAcc_Fail);
		break;
		case TESTCMD_RAR_REVS :
			fsm_printf("receive RAR successfully\n");
			fsm_schedule_self(50, RARrev_Success);
		break;
		case TESTCMD_CR_F :
			fsm_printf("Contention Resolution Failed\n");
			fsm_schedule_self(50, ContentionResolution_Fail);
		break;
		case TESTCMD_CR_S :
			fsm_printf("Contention Resolution successfully\n");
			fsm_schedule_self(50, Contention_Success );
		break;	
		default:
		break;
	}
	FOUT;
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
	SV_PTR_GET(rlc_mac_sv);
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
		//SV(bufferReqTimerValue) = 10*100;  //暂定
		//SV(bufferReqTimerCode) = 0x30;
		//SV(bufferReqTimer) = 0;
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
	SV_PTR_GET(rlc_mac_sv);
	
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
	SV_PTR_GET(rlc_mac_sv);	

	skb = (FSM_PKT *)fsm_pkt_get();
	//fsm_printf("\n[RLC][rlc_pkt_receive_from_upper] entering rlc_pkt_receive_from_upper()\n");
	
	iciMsg = (URLC_IciMsg*)skb->head;
	fsm_printf("[RLC][rlc_pkt_receive_from_upper][-->] receive from upper, pbch=%d, rbid=%d\n",iciMsg->pbCh,iciMsg->rbId);
	fsm_octets_print(skb->data,64);
	//fsm_printf("the icimsg are %d, %d, %d\n",iciMsg->pbCh, iciMsg->rbId,iciMsg->rnti);
	//fsm_printf("[RLC] pbch is %d\n",iciMsg->pbCh);
	if(iciMsg->pbCh == 0)
	{
		//return;
		findInsByRbid(iciMsg->rbId, &insptru, &insptrd ,&SV(ins_mode));
		fsm_printf("[RLC_MAC][rlc_pkt_receive_from_upper]ins_mode is: %d\n", SV(ins_mode));
		switch(SV(ins_mode))
		{
		case AM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_upper][AM] rbid(%d) has no am instance \n",iciMsg->rbId);
				break;
			}
			amIns = (AM_Instance *)insptru;
			if((amIns->currentStatVar == ST_SUSPEND) || (amIns->currentStatVar == ST_SUSPEND_IDLE))
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
			fsm_printf("[RLC][rlc_pkt_receive_from_upper]AM_MODE list_add done!\n");
			fsm_printf("[RLC][rlc_pkt_receive_from_upper] amIns addr is %p, amIns->rbid=%d, amIns->lcid=%d\n",&amIns,amIns->rbId,amIns->lcId);
			amIns->amSduBufferSize += skb->len;
			amIns->amSduBufferNum++;
			//fsm_printf("[RLC][rlc_pkt_receive_from_upper] isBufferReq = %d\n",SV(isBufferReq));
			
			/*if(SV(isBufferReq) == 0)
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
			}*/
			FOUT;
			break;

		case UM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_upper][UM] rbid (%d )has no um tran instance \n",iciMsg->rbId);
				break;
			}
			umTxIns = (UM_TX_Instance *)insptru;
			if((umTxIns->CurrentStatVar == ST_SUSPEND) || (umTxIns->CurrentStatVar == ST_SUSPEND_IDLE))
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper][UM] um instance is suspended(rbid is %d)\n",umTxIns->rbid);
				break;
			}

			umElm = (UmBuffer *)fsm_mem_alloc(sizeof(UmBuffer));
			umElm->pkt = skb;
			umElm->pktstatus = FULL_SDU;
			umElm->pos = 0;
			list_add_tail(&umElm->list, &umTxIns->umSduBuffer.list);
			fsm_printf("[RLC][rlc_pkt_receive_from_upper]UM_MODE list_add done!\n");
			
			umTxIns->umSduBufferNum++;
			if(SV(max_queue_num) < (umTxIns->umSduBufferNum)){
				SV(max_queue_num) = umTxIns->umSduBufferNum;
			}
			umTxIns->umSduBufferSize += skb->len;
			/*if(SV(isBufferReq) == 0)
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
			}*/
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
			
			/*if(SV(isBufferReq) == 0)
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
			}*/
			FOUT;
			break;
		default:
			SV(countDropPacket)++;
			printk("[RLC][rlc_pkt_receive_from_upper] cannot distinfuish this %d\n", SV(ins_mode));
			printk("[RLC][rlc_pkt_receive_from_upper] A packet has dropped, %d packets dropped\n", SV(countDropPacket));
			
			break;
		}
		
		if(skb != NULL)
		{
			fsm_pkt_destroy(skb);
			skb = NULL;
		}
		FOUT;
	}
	else
	{
		SV(countDropPacket)++;
		fsm_printf("[RLC][rlc_pkt_receive_from_upper] no logic channel to transimmit this message\n");
		printk("[RLC][rlc_pkt_receive_from_upper] A packet has dropped, %d packets dropped\n", SV(countDropPacket));
		fsm_pkt_destroy(skb);
		skb = NULL;
		FOUT;
	}
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
void rlc_pkt_receive_from_mac(FSM_PKT * skb)
{
	//FSM_PKT * skb;
	//  struct list_head* pos;
	MRLC_fromMac_IciMsg* micimsg;
	URLC_IciMsg icimsg;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
//   struct  TM_RX_Instance *tmRxIns;
	void *insptru, *insptrd;
	
	FIN(rlc_pkt_receive_from_mac());
	SV_PTR_GET(rlc_mac_sv);
	SV(countreceive_from_mac)++;
	printk("[rlc_pkt_receive_from_mac]UE RLC_MAC SEND TO UPPER=%d\n",SV(countreceive_from_mac));

	//skb = fsm_pkt_get();

	//fsm_printf("[RLC][rlc_pkt_receive_from_mac][-->] receive from lower\n");
	//fsm_octets_print(skb->data,50);
	micimsg = (MRLC_fromMac_IciMsg*)skb->head;
	fsm_printf("\n[RLC][rlc_pkt_receive_from_mac] entering rlc_pkt_receive_from_mac(lcid is %d,pbch is %d)\n",micimsg->lcid,micimsg->pbCh);
	fsm_octets_print(skb->data,64);
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
			if((amIns->currentStatVar == ST_SUSPEND) || (amIns->currentStatVar == ST_SUSPEND_IDLE))
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
			printk("[RLC_MAC AM]SV(countDropPacket):%d\n", SV(countDropPacket));
			FOUT;
			break;

		case UM_MODE:
			if(insptrd == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_mac][UM] lcid(%d) has no am instance \n",micimsg->lcid);
				break;
			}
			umRxIns = (UM_RX_Instance *)insptrd;
			if((umRxIns->currentStatVar == ST_SUSPEND) || (umRxIns->currentStatVar == ST_SUSPEND_IDLE))
			{
				printk("[RLC][rlc_pkt_receive_from_mac][UM] am instance is suspended(rbid is %d)\n",micimsg->lcid);
				break;
			}
			rlc_UM_pkt_receive_from_mac(umRxIns ,skb);
			//printk("[RLC_MAC UM]SV(countDropPacket):%d\n", SV(countDropPacket));
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
	//printk("[RLC_MAC]SV(countDropPacket):%d\n", SV(countDropPacket));
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
	SV_PTR_GET(rlc_mac_sv);
	
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
	SV_PTR_GET(rlc_mac_sv);
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
			//printk("[RLC][rlc_UM_pkt_receive_from_mac] A packet has dropped\n");
			SV(countDropPacket)++;
			fsm_pkt_destroy(pkt);
			printk("rlc_macfsm 4328\n");
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
			printk("rlc_macfsm 4361\n");
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
	//printk("[RLC_MAC]SV(countDropPacket):%d\n", SV(countDropPacket));
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
	SV_PTR_GET(rlc_mac_sv);
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
	SV_PTR_GET(rlc_mac_sv);
	switch(fsm_ev_ioctrl_cmd())
	{
	
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
	SV_PTR_GET(rlc_mac_sv);
	*up = *down = NULL;
	*mode = 0;
	if (!list_empty(&SV(vTm_tx_ins).tmTxList))
	{
		//fsm_printf("[RLC_MAC][findInsByRbid]tmTxList is unempty!\n");	//add for debug 20150911
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
		//fsm_printf("[RLC_MAC][findInsByRbid]tmRxList is unempty!\n");	//add for debug 20150911
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
		//fsm_printf("[RLC_MAC][findInsByRbid]umTxList is unempty!\n");	//add for debug 20150911
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
		//fsm_printf("[RLC_MAC][findInsByRbid]umRxList is unempty!\n");	//add for debug 20150911
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
		//fsm_printf("[RLC_MAC][findInsByRbid]amList is unempty!\n");	//add for debug 20150911
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
	SV_PTR_GET(rlc_mac_sv);
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
	SV_PTR_GET(rlc_mac_sv);
	/*if( SV(bufferReqTimerCode) == code )
	{
		*timer = BUFFER_REQ;
		return NULL;
	}*/
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

