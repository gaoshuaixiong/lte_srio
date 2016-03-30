#include "rlc_macfsm.h"
#include "../pkfmt.h"
#include "Rlcprimitives.h"

static void init_enter(void);
static void mac_init_enter(void);
static void mac_sv_init(void);	
static void mac_close(void);
//default function 20141124 LHL
static void defaultconfig();
static void macmain_defaultconfig();
static void sps_defaultconfig();
static void CCCH_defaultconfig();
//default function 20141124 LHL

static void idle_default_handler(void);//IDLE状态的默认处理函数
static void dl_schedule(void);//进行下行调度
static void ul_schedule(void);//进行上行调度 

static void handle_ra_req(void);//处理随机接入请求
static void handle_ulsched_req(void);//处理上行调度请求
static void handle_rlcdata_req(char *tempdata_ptr);//处理RLC数据发送请求
static void ioctrl_handler(void);//处理各种IOCTRL命令(与状态转换无关的命令) 

static void update_ta(Ue_ta_info *ta_info);//LHL 20141011
static void update_frame(PHY_TO_MAC_frame *frame_info);//LHL 20141017

// 测试状态机用的函数
void test_queue_manager(struct list_head *listhead);
void print_tran_info( const char *str);
static MAC_LogicalChannelConfig_IoctrlMsg * rrc_lc_config(unsigned short rnti,int lcid);
static void test_print_pkt_num(void);

struct test_s
{	
	char name[5];
	int num;	
};

extern volatile int export_frameNo;
extern volatile int export_subframeNo;	//add in 20151230
int ul_schd_count=0;

//测试用函数
extern void Am_instance_add(void);
extern void UM_instance_bulid(void);
extern addPollData();

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
	FSM_ENTER(rlc_mac_main());
	FSM_BLOCK_SWITCH
	{
		FSM_STATE_FORCED(ST_INIT, "INIT", init_enter(), )		
		{
			FSM_TRANSIT_FORCE(ST_IDLE,print_tran_info("INIT -> IDLE") , "default", "", "INIT -> IDLE");
		}	
		FSM_STATE_FORCED(ST_MAC_INIT, "MAC_INIT", mac_init_enter(), )		
		{
			FSM_TRANSIT_FORCE(ST_IDLE,print_tran_info("MAC_INIT -> IDLE") , "default", "", "MAC_INIT -> IDLE");
		}	
		FSM_STATE_FORCED(ST_RLC_INIT, "RLC_INIT", rlc_init_enter(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "RLC_INIT -> IDLE");
		}
		
		FSM_STATE_UNFORCED(ST_IDLE, "IDLE",,)		
		{
			FSM_COND_TEST_IN("IDLE")				
			FSM_TEST_COND(MAC_RESET)				
			FSM_TEST_COND(RA_REQ)
			FSM_TEST_COND(DL_SCHED_REQ) 	//need to modify 20150804
			//FSM_TEST_COND(UL_SCHED)		//noted in 20150908
			
			FSM_TEST_COND(RLC_SUSPEND)
			FSM_TEST_COND(RLC_DEACT)
			FSM_TEST_COND(RLC_RESUME)
			FSM_COND_TEST_OUT("IDLE")	
			FSM_TRANSIT_SWITCH			
			{	
				FSM_CASE_TRANSIT(0, ST_MAC_INIT,    print_tran_info("IDLE -> MAC_INIT"), "IDLE -> MAC_INIT")				
				FSM_CASE_TRANSIT(1, ST_RASCHED,handle_ra_req(), "IDLE -> RASCHED")
				//FSM_CASE_TRANSIT(2, ST_DLSCHED,handle_rlcdata_req(), "IDLE -> DLSCHED")
				FSM_CASE_TRANSIT(2, ST_DLSCHED, , "IDLE -> DLSCHED")	//delete function handle_rlcdata_req() 20150831		
				//FSM_CASE_TRANSIT(3, ST_ULSCHED,ul_schedule(), "IDLE -> ULSCHED")
				//merge rlc layer TRAN state to IDLE
				FSM_CASE_TRANSIT(3,ST_SUSPEND,  ,"IDLE -> SUSPEND")
				FSM_CASE_TRANSIT(4,ST_NULL,  , "IDLE -> NULL")
				FSM_CASE_TRANSIT(5,ST_RLC_INIT,  , "IDLE -> RLC_INIT")				
				FSM_CASE_DEFAULT(ST_IDLE,idle_default_handler() , "IDLE->IDLE")	//transit to idle state	by default.
			}
		}
		FSM_STATE_FORCED(ST_RASCHED, "RASCHED",, )
		{
			FSM_TRANSIT_FORCE(ST_IDLE,print_tran_info("RASCHED -> IDLE"), "default", "", "RASCHED -> IDLE");
		}
		/*FSM_STATE_FORCED(ST_ULSCHED, "ULSCHED", , )	//noted in 20150908
		{
			FSM_TRANSIT_FORCE(ST_IDLE,, "default", "", "ULSCHED -> IDLE");//testing code, "default", "", "ULSCHED -> IDLE");
		}*/
		FSM_STATE_FORCED(ST_DLSCHED, "DLSCHED", , )
		{
			FSM_TRANSIT_FORCE(ST_IDLE,dl_schedule(), "default", "", "DLSCHED -> IDLE");
		}
		FSM_STATE_FORCED(ST_SUSPEND, "SUSPEND", , )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "SUSPEND -> IDLE");
		}
		FSM_STATE_FORCED(ST_NULL, "NULL", , )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "NULL -> IDLE");
		}
	}
	FSM_EXIT(0)
}

//add new init function in 20150804
static void init_enter(void)
{
	FIN(init_enter());
	mac_init_enter();
	rlc_init_enter();
	FOUT;
	
}
static void mac_init_enter(void)
{
	FIN(mac_init_enter());
	//SV_PTR_GET(rlc_mac_sv);
	//print_tran_info("FSM-into-INIT");
	mac_sv_init();
	defaultconfig();//RRC default 20141124LHL 
	//test code as follows: 
	//Init_LogicalChannel_ConfigInfo(rrc_lc_config(61, 1));//testing code  //配置逻辑信道 
	Init_LogicalChannel_ConfigInfo(rrc_lc_config(61, 0));//testing code  //配置逻辑信道 
	//Init_LogicalChannel_ConfigInfo(rrc_lc_config(62, 1));//testing code  //配置逻辑信道 
	Init_LogicalChannel_ConfigInfo(rrc_lc_config(62, 0));//testing code  //配置逻辑信道 
	//fsm_printf("MAC logical channel config \n");//testing code  //配置逻辑信道 
	
	//evHandle schd_self_ret;
	//schd_self_ret = fsm_schedule_self(GET_SUBFRAMENO_PERIOD*1000*10, GET_SUBFRAME_NO);	//add in 20160104
	//SV(check_subframe_no) = schd_self_ret;

	FOUT;
}

static void macmain_defaultconfig()
{
	FIN(macmain_defaultconfig());
	SV_PTR_GET(rlc_mac_sv);
	SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.maxHARQ_Tx) = maxHARQ_Tx_n5;
	//fsm_printf("test maxHARQTX:%d \n ",SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.maxHARQ_Tx));//testing code
	//SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.periodicBSR_Timer)= periodicBSR_Timer_infinity;//该参数值表示该定时器不启用
	//SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.retxBSR_Timer)= retxBSR_Timer_sf2560 ;
	SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.ttiBundling)= false ;
	SV(systemconfigInfo.MACMainConfigInfo.drx_Config.choice.setup.haveShortDRX)= false ;//means release
	SV(systemconfigInfo.MACMainConfigInfo.type)= 1 ; //means phr-Config release ??  
	FOUT;
}
static void sps_defaultconfig()
{
	FIN(sps_defaultconfig());
	SV_PTR_GET(rlc_mac_sv);
	SV(systemconfigInfo.SPSConfigInfo.haveSPS_ConfigDL)=false;
	SV(systemconfigInfo.SPSConfigInfo.haveSPS_ConfigUL)=false; 
	FOUT;
}
static void defaultconfig()
{
	FIN(defaultconfig());
	macmain_defaultconfig();
	sps_defaultconfig();
	CCCH_defaultconfig();//CCCH信道默认配置
	//fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);	//for test
	FOUT;
}

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
	//fsm_printf("[UEMAC][init]CCCH_defaultconfig completed\n");
	FOUT;
}
static void mac_sv_init(void)
{
	int i;
	int index;
	FIN(mac_sv_init());
	SV_PTR_GET(rlc_mac_sv);
	SV(UL_resource_flag)=1;

	SV(ULschedQue_p) = (struct list_head *)fsm_mem_alloc(sizeof(struct list_head));//给表头分配内存
	SV(RAschedQue_p) = (struct list_head *)fsm_mem_alloc(sizeof(struct list_head));//给表头分配内存
	INIT_LIST_HEAD(SV(ULschedQue_p));//初始化表头
	INIT_LIST_HEAD(SV(RAschedQue_p));//初始化表头
	init_rnti_array_val();//初始化RNTI信息表
	SV(m_temp_cr)=(temp_contention_resolution*)fsm_mem_alloc(sizeof(temp_contention_resolution));
	SV(phy_to_mac_frame).subframeN=0;
	SV(phy_to_mac_frame).sfn=0;
	SV(UL_sched_flag) = 0;
	SV(cnt_test)=1;
	init_Bitmap();
	for(i=0;i<5;i++){
		fsm_mem_set(&SV(schedule_scheme[i]),0,sizeof(struct result_sched_algorithm));
	}

	/*for(index=0;index<USERNUMMAX;index++)//LHL modified 20141011
	{
		SV(TBsize_ue[index].rnti)=0;
		SV(TBsize_ue[index].tbsize)=0;
	}*///使用schedule_scheme
	//LHL 20141011
	for(i=0;i<USERNUMMAX;i++)
	{
		SV(ue_ta[i].rnti)=0;
		SV(ue_ta[i].ta)=0;
		SV(ue_ta[i].update_flag)=0;
	}//LHL 20141011

	SV(send_rar_flag)=1;//20150104联调测试代码，以后要去掉，标志是否发RAR，可以通过用户
	/*空间的IOCTL配置不发RAR，测试错误机制的处理*/
	
	Init_Enbmac_Scheduler();//初始化调度器相关的变量 
//	test_queue_manager(SV(ULschedQue_p));//测试函数
//	fsm_schedule_self(1000000,TIME_TODO_ULSCHED);//上行调度周期本应该是10ms 为了测试方便这里设置为10s
	//test
	SV(lost_pktnum_uplink)=0;
	SV(total_pktnum_uplink)=0;
	SV(lost_pktnum_downlink)=0;
	SV(total_pktnum_downlink)=0;
	//test end

	SV(ue_num) = 0;		//add in 20151228;
	SV(check_subframe_no) = 0;
	
	SV(countRecvFromUpper)=0;
	SV(countSendToUpper)=0;
	SV(countRecvFromLower)=0;
	SV(countSendToLower)=0;

	SV(newcountsendtoLower)=0;
	
FOUT;
}

static void mac_close(void)
{
	FIN(mac_close());
	SV_PTR_GET(rlc_mac_sv);
	if(SV(ULschedQue_p) != NULL)
	{
		clean_ULschedQue(SV(ULschedQue_p));
		fsm_mem_free(SV(ULschedQue_p));
	}
    	
	if(SV(RAschedQue_p) != NULL)
	{
		clean_RAschedQue(SV(RAschedQue_p));
		fsm_mem_free(SV(RAschedQue_p));
	}
	 if(SV(m_temp_cr) != NULL)
	    {
	    	fsm_mem_free(SV(m_temp_cr));
		SV(m_temp_cr) = NULL;
	    }
   	Free_Enbmac_Scheduler_Resource();
FOUT;
}

/**********************************************************
**Function name: idle_default_handler()
**Descriptions: add function "rlc_interupt()" of RLC to this function
**Input: 
**Output: 
**Returns:
**Created by: Liu Xiru
**Created Date: 
**-----------------------------------------------------
**Modified by: 
**Modified Date: 
**********************************************************/
static void idle_default_handler()
{ 
//处理与状态转换无关的事件:下层数据包到达、与状态无关的ioctrl命令
	int i;
	FSM_PKT *skb_lower;
	FSM_PKT *skb_upper;
	char *tempdata_ptr;
	
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	void *insPtr;
	
	FIN(idle_default_handler());
	SV_PTR_GET(rlc_mac_sv);

	//fsm_printf("[RLC_MAC][idle_default_handler][begin]fsm event type: %d, code: %x\n", fsm_ev_type(),fsm_ev_code());
	/*if(export_subframeNo == 3 || export_subframeNo == 8)
	{
		//fsm_schedule_self(GET_SUBFRAMENO_PERIOD*5, GET_SUBFRAME_NO);
		fsm_printf("[RLC_MAC][idle_default_handler]excute ul_schedule\n");
		ul_schedule();	//debug try 20160106
	}*/	

	if(MAC_CLOSE)
	{
		mac_close();
    	fsm_printf("[RLC_MAC]MAC FSM closed \n");//testing code
    	FOUT;		
	}
	else if(RLC_CLOSE)     //模块关闭
	{
		rlc_close();
		fsm_printf("[RLC_MAC]RLC FSM closed \n");
		FOUT;
	}
    else if(MAC_PK_FROM_LOWER)//经过讨论PHY将多个用户的包拆开为一个一个用户再给MAC
    {//通过ICI信息判断包类型，是哪个用户的，解复用通过相应地LC发给RLC 
        //参考流程图 HQ 注释
    	//fsm_printf("handle the data packet from lower layer\n");//testing code
    	skb_lower=fsm_pkt_get();
    	decomplexEndMacPdu(skb_lower);  //this function unmodify 20150804!!!!!!!!!!!
    	//fsm_pkt_destroy(skb_lower);
		SV(countRecvFromLower)++; //mac收到下层的包，解复用后直接调用rlc_pkt_receive_from_mac()
		//rlc_pkt_receive_from_mac(); //add in 20150803

		SV(check_subframe_no) = fsm_schedule_self(GET_SUBFRAMENO_PERIOD, GET_SUBFRAME_NO);
		
		FOUT;
    }
	else if(RLC_PKT_FROM_UPPER)		//接收来自上层的包
	{
		SV(countRecvFromUpper)++;
		//printk("[RLC_MAC]countRecvFromUpper:%d\n", SV(countRecvFromUpper));
		rlc_pkt_receive_from_upper();
		
		if( PMRLC_BUFFERREP_REQ() == NULL )
		{
			fsm_printf("[RLC_MAC]no data request!\n");
		}
		else{
			tempdata_ptr = (char *)PMRLC_BUFFERREP_REQ();
			int tempdata = *(u32 *)tempdata_ptr;
			if(tempdata > 0)
			{
				fsm_printf("[RLC_MAC]recv data request!\n");
				handle_rlcdata_req( tempdata_ptr );
				fsm_data_destroy((void *) tempdata_ptr);
				fsm_schedule_self(10, TIME_TODO_DLSCHED);	//start the dl_schedule timer.
			
			}
		}
		FOUT;
	}
	else if(CHECK_UL_SUBFRAME)	//do ul_schedule 20151230
	{
		if(export_subframeNo == 3 || export_subframeNo == 8)
			ul_schedule();

		if(SV(check_subframe_no))
		{
			fsm_schedule_cancel(SV(check_subframe_no));
			SV(check_subframe_no) = 0;
		}

		SV(check_subframe_no) = fsm_schedule_self(GET_SUBFRAMENO_PERIOD*3, GET_SUBFRAME_NO);
	}
	//GengCanxi新增2014、7、23
	else if(TC_RNTI_CHECK)
	{
		delete_overtime_tc_rnti();
		//test 
    	//fsm_printf("delete the overtime TC-RNTI(such as, when UE hasn't receive the msg2) \n");//testing code	
    	FOUT;	
	}
	else if(RLC_EV_SELF)				// 定时器超时，定位实体/
	{
		/*if(fsm_ev_code() == TIME_TODO_ULSCHED)
		{
		   ul_schedule();
		   printk("[RLC_MAC]ul_schedule done!\n");
		   FOUT;
		}*/

		insPtr = findInsByCode(fsm_ev_code(), &SV(timerType));
		//fsm_printf("[RLC][rlc_interupt]timerType: %d\n", SV(timerType));

		if( !insPtr )
		{
			fsm_printf("[RLC][rlc_interupt] the instance of this code is NULL\n");
			//("cannot find the instance of this code \n ");
			FOUT;
		}
		switch(SV(timerType))
		{
		/*
		case UL_SCHEDULE:
			ul_schedule();
			printk("[RLC_MAC]ul_schedule done!\n");
			FOUT;
			break;

			*/
		case POLL_RETX:
			amIns = (AM_Instance *)insPtr;
			amIns->pollRetxTimer = 0;
			rlc_poll_retx( amIns);
			//fsm_printf("[RLC][rlc_interupt] poll timeout\n");
			FOUT;
			break;

		case STAT_PRO:
			amIns = (AM_Instance *)insPtr;
			amIns->statProhTimer = 0;
			if(amIns->statFlag)
			{
				rlc_ctrl_pkt_trans(amIns);
				/*if( SV(isBufferReq) == 0 )
				{
					PMRLC_BUFFERREP_REQ();
					FOUT;
				}
				if(!SV(bufferReqTimer))
				{
					SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
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
			fsm_printf("[RLC][rlc_interupt] umRxIns->vr_ur is %d, umRxIns->vr_ux is %d\n\n",umRxIns->vr_ur, umRxIns->vr_ux);
			FOUT;
			break;

		case AM_REORDER:
			amIns = (AM_Instance *)insPtr;
			amIns->reorderTimer = 0;
			//fsm_printf("\n[RLC][rlc_interupt] am reorder timeout\n");

			for(i = amIns->vr_r; i < amIns->vr_ms; i++)   //20141201
			{
				if(amIns->amRecvWindow[i] == NULL)
				{
					amIns->statFlag = true;  //将状态报告标识置为true
					if(!amIns->statProhTimer)
					{
						amIns->statProhTimer = fsm_schedule_self(amIns->statProhTimerValue,amIns->statProhTimerCode);
					}
					break;
				}
			}
			//更新vr_ms为第一个序列号不小于vr_x，并且还没有对其所有字节分段接收完全的AMD PDU
			amIns->vr_ms = amIns->vr_x;
			while((amIns->vr_ms >= amIns->vr_x)&&((amIns->amRecvWindow[amIns->vr_ms] != NULL)\
					&&(amIns->amRecvWindow[amIns->vr_ms]->pktstatus != FULL_SDU)))
			{
				amIns->vr_ms = (amIns->vr_ms +1)%1024;
			}
			fsm_printf("[RLC][rlc_interupt] amIns->vr_ms is updated to %d\n",amIns->vr_ms);
			rlc_AM_reassemble_deliver(amIns,&(amIns->pduLft));			
			if(amIns->vr_h > amIns->vr_ms)
			{
				amIns->reorderTimer = fsm_schedule_self(amIns->reorderTimerValue, amIns->reorderTimerCode);
				amIns->vr_x = amIns->vr_h;
				//fsm_printf("[RLC][rlc_interupt] amIns->vr_x is updated to %d, timer is sheduling again\n", amIns->vr_x );
			}
		
			FOUT;
			break;
		case PKT_BUF:
			//fsm_printf("[RLC][rlc_interupt] pktBufTimer timeout\n");
			pktBufTimer_handler();
			FOUT;
			break;
		/*case BUFFER_REQ:
			//fsm_printf("the bufferReqTimer timeout\n");
			if( SV(bufferReqTimer) )
			{
				fsm_schedule_cancel(SV(bufferReqTimer));
				SV(bufferReqTimer) = 0;
			}
			PMRLC_BUFFERREP_REQ();
			FOUT;
			break;*/
		default:
			//fsm_printf("[RLC][rlc_interupt] cannot distinguish this code \n");
			printk("[RLC][rlc_interupt] cannot distinguish this code \n");
			break;
		}
		FOUT;
	}
	else if(IOCTRL_ARRIVAL)
    { /*与状态无关的ioctrl命令*/  
        ioctrl_handler();
    	FOUT;
    }
	else
	{
		printk("[RLC][rlc_interupt] cannot distinguish the interrupt\n");
	}
	//fsm_printf("[RLC_MAC][idle_default_handler][end]fsm event type: %d, code: %x\n", fsm_ev_type(),fsm_ev_code());
	FOUT;

} 
 
//merge rlc_ioctrl_handler to ioctl_handler,20150803
static void ioctrl_handler(void)
{ 
	void *tmpdata_ptr;
	int m_rnti;
	FIN(ioctl_handler());
	SV_PTR_GET(rlc_mac_sv);

	//print_tran_info("fun ioctrl handler");
	switch(fsm_ev_ioctrl_cmd())
	{
		case IOCCMD_TEST_WITHDATA : //testing code
			//fsm_printf("ioctrl handle\n");
			tmpdata_ptr = fsm_data_get();		
			//fsm_printf("test: the user is %s with RNTI %d \n",((struct test_s*)tmpdata_ptr)->name,((struct test_s*)tmpdata_ptr)->num);
			fsm_data_destroy(tmpdata_ptr);
		break;
		case IOCCMD_PHYtoMAC_SR ://处理上行调度请求
		    handle_ulsched_req();
		break;
		case IOCCMD_RRCtoMAC_LC_config://逻辑信道配置
			tmpdata_ptr = fsm_data_get();		
			fsm_printf("MAC receive lc_config order \n");//testing code 
			Init_LogicalChannel_ConfigInfo((MAC_LogicalChannelConfig_IoctrlMsg *)tmpdata_ptr);
			fsm_data_destroy(tmpdata_ptr);
		break;
		case IOCCMD_RRCtoMAC_LC_release :
			tmpdata_ptr = fsm_data_get();		
			fsm_printf("MAC receive lc_release order \n");//testing code 
			Delete_LogicalChannel_ConfigInfo(((struct MAC_Release_LogicalChannel_IoctrlMsg *)tmpdata_ptr)->crnti,
						((struct MAC_Release_LogicalChannel_IoctrlMsg *)tmpdata_ptr)->lcid);
			fsm_data_destroy(tmpdata_ptr);
		break;
		case IOCCMD_RRCtoMAC_UE_release://释放该UE的所有配置信息
			tmpdata_ptr = fsm_data_get();
			m_rnti=((struct MAC_Release_Uefsm *)tmpdata_ptr)->crnti;
			recall_rnti(m_rnti);
			fsm_printf("receive the order to release this UE : \n",((struct MAC_Release_Uefsm *)tmpdata_ptr)->crnti);
			fsm_data_destroy(tmpdata_ptr);
		//删除该UE对应的待发数据缓存情况
		 Empty_MACBuffer_Rlc( Get_Enbbuffer_From_Rnti(m_rnti));
		//删除该UE对应的所有逻辑信道的配置
		Empty_LogicalChannelInfo(Get_LogicalChannelInfo_From_Rnti(m_rnti));
		Empty_LogicalChannelBj(Get_LogicalChannelBj_From_Rnti(m_rnti));
		release_UE_rnti(m_rnti);//释放该UE的RNTI 20141120
		break;
		case IOCCMD_PHYtoMAC_TA:
			tmpdata_ptr = fsm_data_get();
			//fsm_printf("[MAC]IOCCME_PHTTOMAC_TA\n");
			update_ta((Ue_ta_info *)tmpdata_ptr);
			fsm_data_destroy(tmpdata_ptr);
		break;
		case IOCCMD_PHYtoMAC_FRAME:
			tmpdata_ptr = fsm_data_get();
			update_frame((PHY_TO_MAC_frame *)tmpdata_ptr);
			fsm_data_destroy(tmpdata_ptr);

			//fsm_schedule_self(GET_SUBFRAMENO_PERIOD, GET_SUBFRAME_NO);
			/*if(SV(phy_to_mac_frame).subframeN==3 || SV(phy_to_mac_frame).subframeN==8)
			{
				//ul_schedule();
				printk("[RLC_MAC]subframeN: %d\n", SV(phy_to_mac_frame).subframeN);
				//printk("[RLC_MAC]excute ul_schedule\n");
			}*/	
		break;
		case _IOCTLtoENBMAC_RAR_Failed: //20150104联调添加的代码，不发RAR，测试错误机制的处理
			SV(send_rar_flag)=0;
		break;
		case _IOCTLtoENBMAC_RAR_Send:
			SV(send_rar_flag)=1;
		break;
		case _IOCTLtoENBMAC_PKT_NUM:
			test_print_pkt_num();
		break;
		//default:
		//break;
		
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
		case  PRINTRLC:
			printk("\ncountRecvFromUpper=%d\n",SV(countRecvFromUpper));
			printk("countSendToLower=%d\n",SV(countSendToLower));
			printk("countSendToUpper=%d\n",SV(countSendToUpper));
			printk("countRecvFromLower=%d\n",SV(countRecvFromLower));
			printk("countSendCtrlPdu=%d\n",SV(countSendCtrlPdu));
			printk("countRecvCtrlPdu=%d\n",SV(countRecvCtrlPdu));
			printk("countDropPacket=%d\n\n",SV(countDropPacket));
			printk("maxQueueNum=%d\n\n",SV(max_queue_num));		
		default:
			fsm_printf("[RLC_MAC][ioctrl_handler] unidentified ioctrl command!\n");
		break;
	}
	
FOUT;
}
static void test_print_pkt_num()
{
	FIN(test_print_pkt_num());
	SV_PTR_GET(rlc_mac_sv);
	printk("[ENB MAC]lost_pktnum_uplink:%d\n",SV(lost_pktnum_uplink));
	printk("[ENB MAC]total_pktnum_uplink:%d\n",SV(total_pktnum_uplink));
	printk("[ENB MAC]lost_pktnum_downlink:%d\n",SV(lost_pktnum_downlink));
	printk("[ENB MAC]total_pktnum_downlink:%d\n",SV(total_pktnum_downlink));
	FOUT;
}
/************************************************************************
*author:LHL
*description:更新TA
*version1.0-20141011
************************************************************************/
static void update_ta(Ue_ta_info *ta_info)
{
	int m_rnti;
	int i;
	u32 flag=0; 
	FIN(update_ta());
	SV_PTR_GET(rlc_mac_sv);
	m_rnti=ta_info->rnti;
	//fsm_printf("[MAC]TA is %d\n", ta_info->ta);
	for(i=0;i<DL_SCHEDULE_NUM;i++)
	{
		if(SV(ue_ta[i]).rnti== m_rnti)
		{
			flag=1;
			SV(ue_ta[i]).ta=ta_info->ta;
			SV(ue_ta[i]).update_flag=1;
		}
	}
	if(flag==0)
	{
		for(i=0;i<DL_SCHEDULE_NUM;i++)
		{
			if(SV(ue_ta[i]).rnti==0)// has not been used
			{
				SV(ue_ta[i]).rnti= m_rnti;
				SV(ue_ta[i]).ta=ta_info->ta;
				SV(ue_ta[i]).update_flag=1;
				break;
			}
		}
	}
	FOUT;
}
/************************************************************************
*author:LHL
*description:更新当前帧号和子帧号
*version1.0-20141017
************************************************************************/
static void update_frame(PHY_TO_MAC_frame *frame_info)
{
	FIN(update_frame());
	SV_PTR_GET(rlc_mac_sv);
	SV(phy_to_mac_frame).sfn=frame_info->sfn;
	SV(phy_to_mac_frame).subframeN=frame_info->subframeN;
}
/************************************************************************
*author:HQ
*description:完成下行调度工作，响应RLC的数据发送请求，生成相应的DCI，
*version1.0-201406017
************************************************************************/
static void handle_rlcdata_req(char *tempdata_ptr)//处理RLC数据发送请求
{
	//char *tempdata_ptr;
	RLC_Request *rlc_req;
	RLC_Request *rlc_req_temptr;	
	int rlc_req_lc_totalnum;
	int i,ue_num;
	unsigned short tmp_rnti;

	void *datareport;//20141231 for test
	RLC_Report *testtmp=NULL;//for test 20141231
	int k=0;//for test 20141231
	FIN(handle_rlcdata_req());
	
	SV_PTR_GET(rlc_mac_sv);
	//tempdata_ptr = (char *)fsm_data_get();//接收命令所携带的数据
	
	//tempdata_ptr = (char *)PMRLC_BUFFERREP_REQ(); //modify 20150804
	
	fsm_printf("[mac] receive data request!\n");
	rlc_req_lc_totalnum = *((u32 *)tempdata_ptr); //由上层保证该字段存在 

	//fsm_printf("[ENB MAC]rlc_req_lc_totalnum:%d\n",rlc_req_lc_totalnum);
		
	if(rlc_req_lc_totalnum == 0)
	{
		//fsm_printf("error :RLC datareq is zero \n");
		FOUT;
	}
	else if(rlc_req_lc_totalnum >= 1)
		{	//将各个用户的请求分出来
			rlc_req = (RLC_Request *)(tempdata_ptr+sizeof(u32));
			rlc_req_temptr = rlc_req + 1;
			SV(Rlc_Request_Num[0]) = 1;		
			for(i=1,ue_num=0;i<rlc_req_lc_totalnum;i++) 
			{				
				if(rlc_req_temptr->rnti == (rlc_req_temptr-1)->rnti)
					SV(Rlc_Request_Num[ue_num])++;
				else 
					SV(Rlc_Request_Num[++ue_num])++;	
				rlc_req_temptr++;			
			}
		}
	//以下测试代码为了检测处理是否正确 可以删除 
	/*for(i=0; i<USERNUMMAX; i++)//testing code 
	{ 
		if( SV(Rlc_Request_Num[i]) != 0)
		{
			fsm_printf("Rlc_Request_Num[%d]:%d\n",i,SV(Rlc_Request_Num[i]));
	    }

	}*/ //testing code end 
	

	fsm_printf("[MAC] ue_num=%d\n",ue_num);
	for(i = 0; i <= ue_num; i++)//取出每个用户的请求，更新相应的缓存
	{
		//fsm_printf("[ENB MAC] rlc_req.rnti=%d",rlc_req->rnti);
	   DoRefreshRLCBuffserRequest (RlcRequest_arraytolist(rlc_req,SV(Rlc_Request_Num[i]),i),SV(Enb_MacBuffer_RLC[i]));
	   rlc_req += SV(Rlc_Request_Num[i]);
	}
	//fsm_data_destroy((void *) tempdata_ptr);  noted in 20150831
	//以下测试代码为了检测处理是否正确 可以删除 
	/*for(i=0; i<USERNUMMAX; i++)//testing code 
	{ 
		if( SV(Rlc_Request_Num[i]) != 0)
		{
			fsm_printf("Rlc_Request_Num[%d]:%d\n",i,SV(Rlc_Request_Num[i]));
			struct list_head *pos,*p;
			MacBufferStatus *temp1;
			int j=0;
    		pos=NULL,p=NULL,temp1=NULL;
		    list_for_each_safe(pos,p,SV(Enb_MacBuffer_RLC[i]))//遍历
		    {
		    	j++;
		        temp1=list_entry(pos,MacBufferStatus,list);
		        printk(KERN_INFO"%d:temp_rnti %d\n",j,temp1->RlcRequestparams->rnti);
		        printk(KERN_INFO"%d:temp_lcid %d\n",j,temp1->lcid);
		    }
	    }

	}*///testing code end 
	//print_tran_info("IDLE -> DLSCHED");//testing code

FOUT;
}

static void dl_schedule()//进行下行调度
{
	int i,j,k;
	int tb_size, buffer_len;
	struct list_head * enb_logicalChannel_config;
	struct list_head *enb_logicalChannel_bj;
	int *rlc_report_num;
	void *tempdata_ptr;
	RLC_Report *rlc_rep_oneUE_ptr;
	RLC_Report *rlc_report_temptr;
	void *rlc_report_ptr;	
	int rlc_rep_num_total = 0;
	int rnti;
	u32 ue_num;
	struct list_head *pos=NULL,*p=NULL;
	MacBufferStatus *m_temp;
	RLC_Request *m_dci_input;
	u32 flag=0;
	FIN(dl_schedule());

	SV_PTR_GET(rlc_mac_sv);
	Reset_Prb_dlBitmap();//重置BITMAP
	
	rlc_report_num = (u32 *)fsm_mem_alloc(sizeof(u32));
	for(i=0; i<USERNUMMAX && SV(Rlc_Request_Num[i]); i++)//根据待发数据缓存情况先计算一个有待发数据的LC的总数
		rlc_rep_num_total += SV(Rlc_Request_Num[i]);
	
	ue_num=i;//当前TTI需要调度的UE数LHL 20150110
	printk("[enb DLSCHED] dl_schedule() numbers of ue: %d\n",ue_num);

	rlc_report_temptr = (RLC_Report *)fsm_mem_alloc(rlc_rep_num_total * sizeof(RLC_Report));//临时的report指针
	rlc_rep_num_total = 0;
	//进行下行资源分配 //根据目前下行资源分配机制 每个用户的资源固定 得到每个用户被分配到的tb_size 
	//tb_size = 100;//单位byte //testing code 
	//Do_sched_algorithm();//以后要改太简单了，这个和抗干扰要求的TBSIZE是否一致????????????????
	for(i=0; i<USERNUMMAX && SV(Rlc_Request_Num[i]); i++)
	{
		//取出一个用户待发数据缓存信息 
		enb_logicalChannel_config = Get_LogicalChannelInfo_From_Enbbuffer(SV(Enb_MacBuffer_RLC[i]));

		//test
		if(enb_logicalChannel_config == NULL)
			continue;

		flag = 1;
		enb_logicalChannel_bj = Get_LogicalChannelBj_From_Enbbuffer(SV(Enb_MacBuffer_RLC[i]));
		buffer_len = SV(Rlc_Request_Num[i]);
		rnti=Getrnti_From_Enbbuffer(SV(Enb_MacBuffer_RLC[i]));//modified by lhl,20140715	
		fsm_printf("[ENB MAC]rlc request rnti:%d\n",rnti);

		Do_sched_algorithm(rnti,ue_num,i);//test Geng20150105
		
		tb_size=Do_cal_datasize(i);//GCX20150110
		
		//tb_size=Do_cal_datasize(rnti);//modified by lhl,20140715,tb_size单位byte 
		/******分割线**********/
		//tb_size=1530;//for test 20150105
		/******分割线**********/
		//fsm_printf("test code tb_seize:%d\n",tb_size);//test code
		list_for_each_safe(pos,p,SV(Enb_MacBuffer_RLC[i]))
		 {
		 	m_temp=list_entry(pos,MacBufferStatus,list);
			m_dci_input=m_temp->RlcRequestparams;
			//fsm_printf("test code order:%d\n",m_dci_input->rnti);
			break;
		 }

		Order_Dl_Sched(m_dci_input,ue_num,i);	//for test!
		
		//为该用户的各条逻辑信道分配资源 //生成相应的给予RLC的反馈报告
		//fsm_printf("tb_size is %d,buffer_len is %d\n",tb_size,buffer_len);
		if (SV(Enb_MacBuffer_RLC[i]) == NULL)
			fsm_printf("SV(Enb_MacBuffer_RLC[i]) == NULL\n");
		if (enb_logicalChannel_config == NULL)
			fsm_printf("enb_logicalChannel_config == NULL\n");
		if (enb_logicalChannel_bj == NULL)
			fsm_printf("enb_logicalChannel_bj == NULL\n");
				
		rlc_rep_oneUE_ptr = DoResourceAllocation(tb_size,SV(Enb_MacBuffer_RLC[i]),enb_logicalChannel_config,
													enb_logicalChannel_bj,buffer_len,rlc_report_num);
		
		fsm_printf("ENB MAC has already alloced resources\n");
	
		//将该用户的各条逻辑信道资源分配的结果复制到总的报告中 
		//for(k=0;k<rlc_report_num;k++)
			//fsm_printf("mac_rlc_report:txQueueHeader:%d,mac_rlc_report->txQueueSize:%d\n",rlc_rep_oneUE_ptr[k].txQueueHeader,rlc_rep_oneUE_ptr[k].txQueueSize);
		fsm_mem_cpy((void *)(rlc_report_temptr+rlc_rep_num_total), 
						rlc_rep_oneUE_ptr,(*rlc_report_num)*sizeof(RLC_Report));
		rlc_rep_num_total += *rlc_report_num; //记录被分配的LC的个数
		//生成该用户相应的下行资源分配的DCI 发送给物理适配层		
		 
		//test code
		//fsm_printf("test code order:%d\n",m_dci_input->rnti);
		//Order_Dl_Sched(m_dci_input);//对这个函数有疑问??????????????	//move up
		
		fsm_mem_free((void *)rlc_rep_oneUE_ptr);
		rlc_rep_oneUE_ptr=NULL;
		
	}
    //将各个用户的RLC反馈报告组合起来，发给RLC层 
	if(flag != 0)
	{
		rlc_report_ptr = fsm_mem_alloc(rlc_rep_num_total * sizeof(RLC_Report) + sizeof(u32));
		fsm_mem_cpy(rlc_report_ptr, &rlc_rep_num_total, sizeof(u32));
		fsm_mem_cpy(rlc_report_ptr+sizeof(u32), rlc_report_temptr,rlc_rep_num_total * sizeof(RLC_Report));

		fsm_schedule_self(DLSCHED_PERIOD, TIME_TODO_DLSCHED);		//excute periodic dl schedule, add in 20151228
		
		fsm_printf("[LHL]REQUEST NUMBER:%d\n",rlc_rep_num_total);
		//fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_MACtoRLC_datasend_Allow, rlc_report_ptr, rlc_rep_num_total * sizeof(RLC_Report) + sizeof(u32));//for test
		PMRLC_TRANSOP_IND ((void*)rlc_report_ptr); //modify in 20150804
		//fsm_mem_free((void *)rlc_report_ptr);
	}
					
	fsm_mem_free((void *)rlc_report_num);//释放指针 
	fsm_mem_free((void *)rlc_report_temptr);//释放指针 
	Empty_Rlcrequestnum();//清空Rlc_Request_Num[]数组
	//print_tran_info("DLSCHED -> IDLE");//testing code
	
FOUT;
}




/************************************************************************
*author:HQ
*description:完成上行调度工作，先调度RA队列再调度普通的上行资源请求队列。
*version1.0-20140601 在调度前检查队列是否为空。
*version1.1-20140617 修改 在调度前检查相应的队列标志,判断是否需要进行调度
*version1.2-20141011 LHL 将RAschedQue_p队列中相同RA_RNTI的用户组成num+S_RAinfo的结构，作为参数传入complexRarPdu函数中
************************************************************************/

static void ul_schedule()
{ 
		u16 *user_rnti;
		SV_PTR_GET(rlc_mac_sv);
		ENBMAC_TO_PHY_ULscheduleDCI *p_ul_ici_dci;
		int resleft;//鍓╀綑璧勬簮
		int ue_number;
		u16 ue_index = 0;
		FIN(ul_schedule());
		//print_tran_info("IDLE -> ULSCHED");
		user_rnti = (u16 *)getUeNum();
		ue_number = SV(ue_num);
		//printk("[RLC_MAC][ul_schedule]ue_number: %d\n", ue_number);
		p_ul_ici_dci = (ENBMAC_TO_PHY_ULscheduleDCI *)fsm_mem_alloc(sizeof(ENBMAC_TO_PHY_ULscheduleDCI)*ue_number);
		
		SV(UL_resource_flag) = 1;
		if(ue_number > 0)
			SV(UL_sched_flag)=1;
		
		//fsm_printf("[ENB MAC] SV(UL_sched_flag):%d\n",SV(UL_sched_flag));
		if(SV(UL_sched_flag) && SV(UL_resource_flag))
		{//鍐嶈繘琛屼笂琛岃皟搴?浠庨槦鍒椾腑鍙栧嚭涓€涓敤鎴?鐩村埌闃熷垪涓虹┖鎴栬€呬笂琛岃祫婧愯鍒嗛厤瀹?
			//fsm_printf("[ENB MAC] SV(ULschedflag)\n");
			for(;ue_index < ue_number;ue_index++)
			{
				///ul_UEinfo = take_first_ULschedQue(SV(ULschedQue_p));//浠庨槦鍒楀ご閮ㄥ彇鍑轰竴涓敤鎴?
				//杩涜璧勬簮鍒嗛厤
				user_rnti += ue_index;

				p_ul_ici_dci += ue_index;
				//printk("###In function ul_schedule() the *(user_rnti):%d\n",*(user_rnti));
				//printk("###In function ul_schedule() the user_rnti[ue_index]:%d\n", user_rnti[ue_index]);
				
				resleft=Order_Ul_Sched((*user_rnti),p_ul_ici_dci,ue_number ,ue_index);//涓婅Ulgrant ,浠OCTL鍙戠粰鐗╃悊閫傞厤灞?
				if(resleft==0)
					SV(UL_resource_flag)=0;//娌℃湁璧勬簮
				//鐢熸垚鐩稿簲鐨凞CI
				//浠OCTRL鐨勬柟寮忓彂閫佺粰鐗╃悊閫傞厤灞?
				//Do_Ul_SchedInd(&ul_UEinfo);
			}
			fsm_do_ioctrl(STRM_TO_SRIO, IOCCMD_MACTOPHY_UL_DCI, (void *) p_ul_ici_dci, sizeof(ENBMAC_TO_PHY_ULscheduleDCI)*ue_number);
			//SV(UL_sched_flag) = !(list_empty_careful(SV(ULschedQue_p)));//鏇存柊闃熷垪鏍囧織
			//printk("[RLC_MAC]ul_schedule() send DCI to SRIO,ul_schd_count: %d\n", ++ul_schd_count);
			//fsm_mem_free(p_ul_ici_dci);
		}
		fsm_mem_free(user_rnti);	//note in 20160106
		//娓呯┖RA闃熷垪 鍥犱负UE绛夊緟RAR鐨勬椂闂村彈鍒伴檺鍒讹紝鏈兘鍝嶅簲鐨凴A璇锋眰鏈€澶氬彧淇濈暀10ms,鍗充竴涓笂琛岃皟搴﹀懆鏈?
	Reset_Prb_ulBitmap();
	FOUT;
}

/************************************************************************
*author:HQ
*description:处理RA请求，为随机接入的用户分配资源，组织生成RAR。
*version1.0-20140601 
*version1.1-  将修改为，在判断剩余上行资源情况时，比较剩余资源是否
				小于RA请求所需要的资源，小于则不响应RA-req，大于则响应RA-req
************************************************************************/
static void handle_ra_req()//处理随机接入请求
{
	void *tmpdata_ptr;
	//DATA_RAR_PDU *data_rar;//modified lhl 20140909
	int resleft;//记录剩余资源，modified by lhl 20140715
	int usernum;//记录用户个数
	int i=0;
	FIN(handle_ra_req());	
	SV_PTR_GET(rlc_mac_sv);
	tmpdata_ptr = fsm_data_get();//获得该用户的相应信息
	//print_tran_info("IDLE -> RASCHED");
	fsm_printf("[RLC_MAC]receive RA from PHY\n");//testing code
	if(SV(UL_resource_flag) == 0) //更准确的实现，是要更精确的比较，剩余资源是否小于RA请求所需要的资源
	{	
		//上行资源目前没有可被分配的资源
		//将该用户加入到RA调度队列中,等待有资源时再被调度
		usernum=(int *)tmpdata_ptr;
		fsm_printf("[RLC_MAC][handle_ra_req]usernum: %d\n", usernum);
		for(i=i;i<usernum;i++)
		{			
			insert_RAschedQue(SV(RAschedQue_p), (struct S_RAinfo *)(((int *)tmpdata_ptr+1)+i));
		}
		SV(RA_sched_flag) = 1;//设置RA队列标志为"需要调度"
		
		fsm_printf("UE RA-RNTI:%d ,RAPID:%d \n",((struct S_RAinfo *)((int *)tmpdata_ptr+1))->ra_rnti,((struct S_RAinfo *)((int *)tmpdata_ptr+1))->rapid);//testing code
	}
	if(SV(UL_resource_flag) > 0) 
	{	
		//TEST!//
		/*S_RAinfo *Msg1_get=(S_RAinfo*)fsm_mem_alloc(sizeof(S_RAinfo));
		fsm_mem_cpy(Msg1_get,(S_RAinfo *)(tmpdata_ptr+sizeof(int)),sizeof(S_RAinfo));
		fsm_printf("[srio][test_send_msg1][--]Message1 index1 = %u,%u,%u,%u,%u\n",Msg1_get->index,Msg1_get->ra_rnti,Msg1_get->total_num,Msg1_get->cqi,Msg1_get->rapid);
		fsm_mem_cpy(Msg1_get,(S_RAinfo *)(tmpdata_ptr+sizeof(int)+sizeof(S_RAinfo)),sizeof(S_RAinfo));
		fsm_printf("[srio][test_send_msg1][--]Message1 index2 = %u.\n",Msg1_get->index);
		fsm_mem_cpy(Msg1_get,(S_RAinfo *)(tmpdata_ptr+sizeof(int)+2*sizeof(S_RAinfo)),sizeof(S_RAinfo));
		fsm_printf("[srio][test_send_msg1][--]Message1 index3 = %u.\n",Msg1_get->index);*/
		//END!//

		//上行资源可以被分配，则响应接入请求
		//fsm_printf("response to the RA_REQ from UE(RA-RNTI) :%d,RAPID:%d \n",((struct S_RAinfo *)((u32 *)tmpdata_ptr+1))->ra_rnti,((struct S_RAinfo *)((u32 *)tmpdata_ptr+1))->rapid);
		//testing code
		//生成RAR PDU
		//data_rar=(DATA_RAR_PDU *)fsm_mem_alloc(sizeof(DATA_RAR_PDU));//modified lhl 20140909
		//data_rar= Do_Data_of_MacRarPdu_ulgrant(data_rar, (struct S_RAinfo *)tmpdata_ptr);//耿灿锡函数，modified by lhl20140715
		//调用何玺函数生成RAR	
		//下行资源调度，RAR也是下行包
		////int Do_RAR_DCI1C(struct S_RAinfo *ra_UEinfo):::::20141024GCX
		
		/***********分割线****联调测试代码*************/
		if(SV(send_rar_flag)==0)//20150104用户空间配置不发RAR
		{
			fsm_data_destroy(tmpdata_ptr);//释放内存
			fsm_printf("[MAC]send_rar_flag=0\n");
			FOUT;
		}
		/***********分割线******end***********/
		complexRarPdu((void *)tmpdata_ptr);//modified lhl 20140909 ,传递的应该是指针?????????????
		//以SK_BUF的方式发给物理适配层
		//生成相应的DCI,应该是正常的下行DCI?????????????????????????

		//resleft=Do_RAR_DCI1C( (struct S_RAinfo *)((u32 *)tmpdata_ptr+1));//耿灿锡函数，modified by lhl20140715,应该不是这样的这个DCI是与数据包一起发送的????

		resleft=Do_RAR_DCI1A( (struct S_RAinfo *)((u32 *)tmpdata_ptr+1));//耿灿锡函数
		if(resleft==0)//没有资源
			SV(UL_resource_flag)=0;//更新资源标志
		//以IOCTRL的方式发送给物理适配层，Do_RAR_DCI函数中已经发送
	}
	fsm_data_destroy(tmpdata_ptr);//释放内存
FOUT;

}

/************************************************************************
*author:HQ
*description:处理上行调度请求，只是将用户加入到等待调度的队列中
*version1.0-20140601 
************************************************************************/
static void handle_ulsched_req()//处理上行调度请求
{
	void *tmpdata_ptr;
	FIN(handle_ulsched_req());	
	SV_PTR_GET(rlc_mac_sv);
	tmpdata_ptr = fsm_data_get();//获得该用户的相应信息
	fsm_printf("receive SR from PHY  ");//testing code
	insert_ULschedQue(SV(ULschedQue_p), (struct S_UEinfo *)tmpdata_ptr);//将该用户加入到上行调度队列中,等待被调度
	SV(UL_sched_flag) = 1;//设置UL队列标志为"需要调度"
	//fsm_printf("UE RNTI:%d\n",((struct S_UEinfo *)tmpdata_ptr)->rnti);//testing code
	fsm_data_destroy(tmpdata_ptr);//释放内存
FOUT;
}


/*****************Testing functions*********************/

void test_queue_manager(struct list_head *listhead)
{
	struct S_UEinfo ueinfo[5]={{1,11},{2,12},{3,13},{4,14},{5,15}};
	struct S_UEinfo myue;
	insert_ULschedQue(listhead,&ueinfo[0]);
	insert_ULschedQue(listhead,&ueinfo[4]);
	//insert_queue(listhead,S_ULsched_QueueEntry,&ueinfo[4]) //使用宏定义的方式
	insert_ULschedQue(listhead,&ueinfo[2]);
	print_ULschedQue(listhead);
   //take_first_entry(listhead,S_ULsched_QueueEntry,&myue) //使用宏定义的方式
	//fsm_printf("use DEFINE take a UE RNTI:%d,CQI:%d\n",myue.rnti,myue.cqi);
	print_ULschedQue(listhead);
	insert_ULschedQue(listhead,&ueinfo[0]);
	print_ULschedQue(listhead);
	myue = take_first_ULschedQue(listhead); //使用函数调用的方式 
	fsm_printf("call func take a UE RNTI:%d,CQI:%d\n",myue.rnti,myue.cqi);
	print_ULschedQue(listhead);
	//print_queue(listhead,S_ULsched_QueueEntry)
	clean_ULschedQue(listhead);//清空队列
	clean_ULschedQue(listhead);
	//clean_queue(listhead,S_ULsched_QueueEntry)//使用函数调用的方式 
	//myue = take_first_ULschedQue(listhead); //测试对空队列的操作 
    //fsm_printf("call func take a UE RNTI:%d,CQI:%d\n",myue.rnti,myue.cqi);
	fsm_printf("empty or not :%d \n",list_empty_careful(listhead));//检测队列是否为空，用该函数的前提是删除时用的是函数list_del_init（）
    //打印出1，说明队列为空
}

void print_tran_info( const char *str)
{
  FIN( print_tran_info());
  int curtime=0;
     curtime=fsm_get_curtime();
     fsm_printf("%d ",curtime);
     fsm_printf(str);
     fsm_printf("\n");
  FOUT;
}

/**********************Testing functions end*********************************/
static MAC_LogicalChannelConfig_IoctrlMsg * rrc_lc_config(unsigned short rnti,int lcid)
{
	MAC_LogicalChannelConfig_IoctrlMsg *lcinfo =(MAC_LogicalChannelConfig_IoctrlMsg *)fsm_mem_alloc(sizeof(MAC_LogicalChannelConfig_IoctrlMsg));
	struct Ul_SpecificParameters param[3] = {{1,infinity,bucketSizeDuration_ms50,0},{2,kBps16,bucketSizeDuration_ms50,2},{3,kBps16,bucketSizeDuration_ms100,3}};

	FIN(MAC_LogicalChannelConfig_IoctrlMsg * rrc_lc_config());

	lcinfo->rnti = rnti ; //对应RNTI值
	lcinfo->logicalChannelIdentity = lcid ; //对应LCID
    lcinfo->logicalChannelConfig.haveUl_SpecificParameters = true;
    lcinfo->logicalChannelConfig.ul_SpecificParameters = param[lcid]; //param[]下标与LCID对应 
    FRET(lcinfo);
}

/****************************RLC 层函数定义**********************************/

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
//    TM_RX_Instance *tmRxIns;
//   TM_TX_Instance *tmTxIns;
	int i;
	FIN(rlc_init_enter());
	SV_PTR_GET(rlc_mac_sv);
	if(RLC_OPEN)
	{
		// SV(ins_mode) = TM_MODE;
		//SV(isBufferReq) = 0;
		SV(packetType) = -1;
		INIT_LIST_HEAD(&SV(rntiManag).list);
		INIT_LIST_HEAD(&SV(pktBuf).list); //初始化链表头部用以保存没有对应实体的数据
		SV(pktBufNum) = 0;
		SV(pktBufSize) = 0;
		SV(count) = 0;
		SV(countRecvFromUpper)=0;  //for test
		SV(countSendToUpper)=0;
		SV(countRecvFromLower)=0;
		SV(countSendToLower)=0;
		SV(countDropPacket)=0;
		SV(countSendCtrlPdu)=0;
		SV(countRecvCtrlPdu)=0;
		SV(max_queue_num)=0;
//定时器管理数组初始化
		for(i = 0; i<MAX_CODE; i++) SV(allocCode[i]) = i;
		SV(allocCode[0]) =  SV(allocCode[0x0B]) =  SV(allocCode[0x21])  = SV(allocCode[0x22]) = SV(allocCode[0x23]) =\
			SV(allocCode[0x24]) = SV(allocCode[0x25]) = SV(allocCode[0x26]) = SV(allocCode[0x27]) = SV(allocCode[0x28]) = \
			SV(allocCode[0x29]) = SV(allocCode[0x30]) = SV(allocCode[0x31])= -1;//0x30为bufferReq定时器,0x31为pktBuf定时器
		SV(bufferReqTimerValue) = 1*1000;  //暂定
		SV(bufferReqTimerCode) = 0x30;

		SV(pktBufTimerValue) = 100*1000;//暂定
		SV(pktBufTimerCode) = 0x31;
	
		//fsm_printf("rlc init is called");
		/* TM receiving instance estabilshment corresponding to PCCH  */
		//    SV(pcchIns) = instance_create(TM_RX_Instance) ;
		SV(pcchIns).pbCh = 1;            //PCCH
		SV(pcchIns).rnti = 0;
		SV(pcchIns).rbId = 0;
		SV(pcchIns).lcId = 0;
		//     list_add_tail(&tmRxIns->tmRxList, &SV(vTm_rx_ins).tmRxList);
		fsm_printf("[RLC][rlc_init_enter] PCCH instance has bulided\n");

		/* TM receiving instance estabilshment corresponding to BCCH  */
		//      SV(bcchIns) = instance_create(TM_RX_Instance) ;
		SV(bcchIns).pbCh = 2;            //BCCH
		SV(bcchIns).rnti = 0;
		SV(bcchIns).rbId = 0;
		SV(bcchIns).lcId = 0;       //BCCH
		//   list_add_tail(&tmRxIns->tmRxList, &SV(vTm_rx_ins).tmRxList);
		fsm_printf("[RLC][rlc_init_enter] BCCH instance has bulided\n");

        /*TM receiving instance estabilshment corresponding to CCCH
        tmRxIns = instance_create(TM_RX_Instance) ;
        tmRxIns->pbCh = 0;            //CCCH
        tmRxIns->rnti = 0;
        tmRxIns->rbId = 0;
        tmRxIns->lcId = 0;       //CCCH
        //list_add_tail(&tmRxIns->tmRxList, &SV(vTm_rx_ins).tmRxList);
        fsm_printf(" CCCH recv instance has bulided\n");*/

        /*TM transimitting instance estabilshment corresponding to CCCH
        tmTxIns = instance_create(TM_TX_Instance) ;
        tmTxIns->pbCh = 0;            //CCCH
        tmTxIns->rnti = 0;
        tmTxIns->rbId = 0;
        tmTxIns->lcId = 0;       //CCCH
        INIT_LIST_HEAD(&tmTxIns->tmSduBuffer.list);
        tmTxIns->tmSduBufferNum = 0;
        tmTxIns->tmSduBufferSize = 0;
        //list_add_tail(&tmTxIns->tmTxList, &SV(vTm_tx_ins).tmTxList);
        fsm_printf(" CCCH tran instance has bulided\n");*/

        //为测试建立一个AM实体
        //    Am_instance_add();
        //UM_instance_bulid();
	//fsm_schedule_self(0, _START_WORK);

	}

	FOUT;
}


void pktBufTimer_handler()
{
	packetBuf *pktbuf, *tempPktBuf;

	FIN(pktBufTimer_handler());
	SV_PTR_GET(rlc_mac_sv);
	
	if(!list_empty(&SV(pktBuf).list))
	{
		list_for_each_entry_safe(pktbuf, tempPktBuf, &SV(pktBuf).list, list)
		{
			pktbuf->count++;
			//fsm_printf("[RLC][pktBufTimer_handler] pktbuf->count = %d\n",pktbuf->count);
			if(pktbuf->count >= 16)  //若该报文达到一定时间未处理就将其删掉
			{
				list_del(&pktbuf->list);
				if(pktbuf->pkt != NULL)
				{
					fsm_pkt_destroy(pktbuf->pkt);
					pktbuf->pkt = NULL;
				}
				if(pktbuf != NULL)
				{
					fsm_mem_free(pktbuf);
					pktbuf = NULL;
				}
				//fsm_printf("[RLC][pktBufTimer_handler] A pkt in pktBuf has been removed\n");
				if(list_empty(&SV(pktBuf).list))
				{
					//fsm_printf("[RLC][pktBufTimer_handler] the pktBuf is empty\n");
					if(SV(pktBufTimer))
					{
						fsm_schedule_cancel(SV(pktBufTimer));
						SV(pktBufTimer) = 0;
						//fsm_printf("[RLC][pktBufTimer_handler] pktBufTimer cancelled\n");
					}
					FOUT;
				}
			}		
		}
	}
	if(SV(pktBufTimer))
	{
		fsm_schedule_cancel(SV(pktBufTimer));
		SV(pktBufTimer) = 0;
	}
	SV(pktBufTimer) = fsm_schedule_self(SV(pktBufTimerValue),SV(pktBufTimerCode));
	//fsm_printf("[RLC][pktBufTimer_handler] pktBufTimer restart\n");
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
	//    RntiManag*rntiM;
	TmBuffer *tmElm;
	struct UmBuffer *umElm;
	struct AmSduBuffer *amElm;
	URLC_IciMsg *iciMsg;
	struct  AM_Instance * amIns;
	struct  UM_TX_Instance *umTxIns;
	struct  TM_TX_Instance *tmTxIns;
	void *insptru, *insptrd;
	FIN(rlc_pkt_receive_from_upper());
	SV_PTR_GET(rlc_mac_sv);

	skb = (FSM_PKT *)fsm_pkt_get();
//  fsm_printf("the icimsg are %d, %d, %d\n",iciMsg->pbCh, iciMsg->rbId,iciMsg->rnti);
	//fsm_printf("[RLC][rlc_pkt_receive_from_upper] entering rlc_pkt_receive_from_upper()\n");

	iciMsg = (URLC_IciMsg*)skb->head;
	fsm_printf("[RLC][rlc_pkt_receive_from_upper] rnti=%d, rbId=%d, pbch=%d\n",iciMsg->rnti,iciMsg->rbId,iciMsg->pbCh);
	if(iciMsg->pbCh == 0)
	{
		findInsByRbid(iciMsg->rnti, iciMsg->rbId, &insptru, &insptrd ,&SV(ins_mode));
		//fsm_printf("[RLC][rlc_pkt_receive_from_upper] SV(ins_mode)=%d\n",SV(ins_mode));
		switch(SV(ins_mode))
		{
		case AM_MODE:
			if(insptru == NULL)
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper] rbid(%d) has no am instance \n",iciMsg->rbId);
				break;
			}
			amIns = (AM_Instance *)insptru;
			if(amIns->currentStatVar == ST_SUSPEND)
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper] am instance is suspended(rbid is %d)\n",amIns->rbId);
				break;
			}
		
			amElm = (AmSduBuffer *)fsm_mem_alloc(sizeof(AmSduBuffer));
			amElm->pkt = skb;
			amElm->pktstatus = FULL_SDU;
			amElm->pos = 0;
			list_add_tail(&amElm->list, &amIns->amSduBuffer.list);
			
			//fsm_printf("[RLC][rlc_pkt_receive_from_upper] amInsAddr =%p,rbid=%d,lcid=%d\n",amIns,amIns->rbId,amIns->lcId);
			amIns->amSduBufferSize += skb->len;
			amIns->amSduBufferNum++;
			//fsm_printf("[RLC][rlc_pkt_receive_from_upper SV(isBufferReq)=%d\n",SV(isBufferReq));
			/*if(SV(isBufferReq) == 0)
			{
				PMRLC_BUFFERREP_REQ();
				FOUT;
			}
			if(!SV(bufferReqTimer))
			{
				SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
			}*/
			FOUT;
			
			break;

		case UM_MODE:
			if(insptru == NULL)
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper] rbid (%d )has no um tran instance \n",iciMsg->rbId);
				break;
			}
			umTxIns = (UM_TX_Instance *)insptru;
			if(umTxIns->CurrentStatVar == ST_SUSPEND)
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper] um instance is suspended(rbid is %d)\n",umTxIns->rbid);
				break;
			}
			
			umElm = (UmBuffer *)fsm_mem_alloc(sizeof(UmBuffer));
			umElm->pkt = skb;
			umElm->pktstatus = FULL_SDU;
			umElm->pos = 0;
			list_add_tail(&umElm->list, &umTxIns->umSduBuffer.list);
			umTxIns->umSduBufferNum++;
			if(SV(max_queue_num) < umTxIns->umSduBufferNum){
				SV(max_queue_num) = umTxIns->umSduBufferNum;
			}
			umTxIns->umSduBufferSize += skb->len;
			
			/*if(SV(isBufferReq) == 0)
			{
				PMRLC_BUFFERREP_REQ();
				FOUT;
			}
			if(!SV(bufferReqTimer))
			{
				SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
			}*/
			FOUT;
			break;

		case TM_MODE:
			if(insptru == NULL)
			{
				fsm_printf("[RLC][rlc_pkt_receive_from_upper] rbid (%d )has no tm instance \n",iciMsg->rbId);
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
			if(!SV(bufferReqTimer))
			{
				SV(bufferReqTimer) = fsm_schedule_self(SV(bufferReqTimerValue),SV(bufferReqTimerCode));
			}*/
			FOUT;
			break;
		default:
			fsm_printf("[RLC][rlc_pkt_receive_from_upper] cannot distinfuish this  %d\n", SV(ins_mode));
			break;
		}
		fsm_printf("[RLC][rlc_pkt_receive_from_upper] cannot find the instance or the instance is suspended\n");
		if(skb != NULL)
		{
			SV(countDropPacket)++;
			printk("[RLC][rlc_pkt_receive_from_upper] A packet has dropped, %d packets dropped\n", SV(countDropPacket));
			fsm_pkt_destroy(skb);
		}
	}
	else
	{
		//fsm_printf("[RLC][rlc_pkt_receive_from_upper] no logic channel to transimmit this message\n");
		fsm_printf("[RLC][rlc_pkt_receive_from_upper] no logic channel to transimmit this message\n");
		SV(countDropPacket)++;
		printk("[RLC][rlc_pkt_receive_from_upper] A packet has dropped, %d packets dropped\n", SV(countDropPacket));
		fsm_pkt_destroy(skb);
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
void rlc_pkt_receive_from_mac(FSM_PKT * skb)
{
	//FSM_PKT * skb;
	//  struct list_head* pos;
	MRLC_fromMac_IciMsg* micimsg;
	URLC_IciMsg icimsg;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	//    struct  TM_RX_Instance *tmRxIns;
	void *insptru, *insptrd;
	packetBuf *pktbuf;
	
	FIN(rlc_pkt_receive_from_mac());
	SV_PTR_GET(rlc_mac_sv);

	insptru = NULL; insptrd = NULL;
	//skb = fsm_pkt_get();
	micimsg = (MRLC_fromMac_IciMsg*)skb->head;
	fsm_printf("[RLC][rlc_pkt_receive_from_mac] rnti=%d, lcid=%d\n",micimsg->rnti,micimsg->lcid);
	fsm_octets_print(skb->data,2);
	findInsBylcid(micimsg->rnti,micimsg->lcid, &insptru, &insptrd,&SV(ins_mode));
	//if(findUeByRnti(micimsg->rnti) == NULL)
	if(insptru == NULL)
	{
		fsm_printf("[RLC][rlc_pkt_receive_from_mac] there is no coresponding instance to this packet\n");
		pktbuf = (packetBuf *)fsm_mem_alloc(sizeof(packetBuf));
		pktbuf->pkt = skb;
		pktbuf->count = 0;
		list_add_tail(&pktbuf->list, &SV(pktBuf).list);
		SV(pktBufNum)++;
		SV(pktBufSize) += skb->len;
		//fsm_printf("[RLC][rlc_pkt_receive_from_mac] SV(pktBufNum) = %d\n",SV(pktBufNum));

		if(!SV(pktBufTimer))
		{
			//fsm_printf("[RLC][rlc_pkt_receive_from_mac] the pktBufTimer start, SV(pktBufTimer)=%d\n",SV(pktBufTimer));
			SV(pktBufTimer) = fsm_schedule_self(SV(pktBufTimerValue),SV(pktBufTimerCode));
		}
		FOUT;
	}
	
	fsm_printf("[RLC][rlc_pkt_receive_from_mac] entering rlc_pkt_receive_from_mac(lcid is %d)\n",micimsg->lcid);
	if(micimsg->lcid != 0 )
	{
		findInsBylcid(micimsg->rnti,micimsg->lcid, &insptru, &insptrd,&SV(ins_mode));
		switch(SV(ins_mode))
		{
		case AM_MODE:
			if(insptru == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_mac] lcid(%d) has no am instance \n",micimsg->lcid);
				break;
			}
			amIns = ( AM_Instance *)insptru;
			if(amIns->currentStatVar == ST_SUSPEND)
			{
				printk("[RLC][rlc_pkt_receive_from_mac] am instance is suspended(lcid is %d)\n",micimsg->lcid);
				break;
			}
			//addPollData();//²âÊÔÓÃ
			rlc_AM_pkt_receive_from_mac(amIns,skb);
			//    fsm_printf("time for rlc_AM_pkt_receive_from_mac(amIns,skb)\n");
			FOUT;
			break;

		case UM_MODE:
			if(insptrd == NULL)
			{
				printk("[RLC][rlc_pkt_receive_from_mac] lcid(%d) has no um instance \n",micimsg->lcid);
				break;
			}
			umRxIns = (UM_RX_Instance *)insptrd;
			if(umRxIns->currentStatVar == ST_SUSPEND)
			{
				printk("[RLC][rlc_pkt_receive_from_mac] um instance is suspended(lcid is %d)\n",micimsg->lcid);
				break;
			}
			rlc_UM_pkt_receive_from_mac(umRxIns ,skb);
			FOUT;
			break;

		default:
			//printk("[RLC][rlc_pkt_receive_from_mac] cannot distinfuish this  %d\n", SV(ins_mode));
			break;
		}
		//fsm_printf("[RLC][rlc_pkt_receive_from_mac] cannot find the instance or the instance is suspended(rbid is %d)\n", micimsg->lcid);
		if(skb != NULL)
		{
			SV(countDropPacket)++;
			printk("[RLC][rlc_pkt_receive_from_mac] A packet has dropped\n");
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
		fsm_printf("[RLC][rlc_pkt_receive_from_mac] <--send to RRC\n");
//		fsm_octets_print(skb->data, 100);
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
void rlc_AM_pkt_receive_from_mac(AM_Instance * amIns, FSM_PKT * pkt)
{
	RLC_AMPDU_head* head;
	RLC_AMPDU_seg_head *seghead;
	RLC_AMPDU_fixhead *fixhead;
	RLC_AMPDU_seg_exthead* extseghead;
	AmBuffer *ambuf;
	FSM_PKT *pkptr;
	char *bufptr;
	long datalen;
	SequenceNumber SN;
	// struct list_head *pos;
	int i;
	//MRLC_fromMac_IciMsg *mac_ici;
	//URLC_IciMsg *icimsg;
	
	FIN(rlc_AM_pkt_receive_from_mac());
	SV_PTR_GET(rlc_mac_sv);
	
	//mac_ici = (URLC_IciMsg *)pkt->head;
	//icimsg = (URLC_IciMsg *)fsm_mem_alloc(sizeof(URLC_IciMsg));
	//icimsg->pbCh = mac_ici->pbCh;
	//icimsg->rbId = mac_ici->lcid;
	//icimsg->rnti = mac_ici->rnti;

	fixhead = ( RLC_AMPDU_fixhead *)pkt->data;
	*(u16 *)fixhead = ntohs(*(u16 *)fixhead);
	fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] entering rlc_AM_pkt_receive_from_mac\n");
	fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] data len = %d\n",pkt->len);
	fsm_octets_print(fixhead,2);
//如果是控制PDU
	if(fixhead->DorC == 0)       //cntrol pdu
	{
		printk("[RLC][rlc_AM_pkt_receive_from_mac] this is a ctrl PDU\n");
//		fsm_octets_print(pkt->data,10);
		SV(countRecvCtrlPdu)++;
		rlc_ctrl_pkt_recv(amIns, pkt);
		FOUT;
	}

//如果是AMD PDU
	if(fixhead->RF == 0)   //AMD PDU
	{
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] this is an AMD PDU (SN is %d)\n", fixhead->SN);
		//head = (RLC_AMPDU_head*) fixhead;
		//head->head = head->head;
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] fixhead->SN=%d\n",fixhead->SN);
		SN = fixhead->SN;
		SN %= 1024;
		//SN %= 512;
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] the context is %s\n",(char*)(fixhead + 1));
		//在接收窗口外
		if(!isInsideAmRecvWin(amIns, SN))
		{
			printk("[RLC][rlc_AM_pkt_receive_from_mac] PDU is outside the receiving window\n");
			printk("[RLC][rlc_AM_pkt_receive_from_mac] A packet has dropped\n");
			SV(countDropPacket)++;
			fsm_pkt_destroy(pkt);
			pkt = NULL;
			FOUT;
		}
		//在接收窗口内，但是未接收完全,或完全接收
		if(amIns->amRecvWindow[SN] != NULL)
		{
			if(amIns->amRecvWindow[SN]->pktstatus == FULL_SDU)
			{
				fsm_pkt_destroy(pkt);
				pkt = NULL;
				printk("[RLC][rlc_AM_pkt_receive_from_mac] A packet has dropped\n");
				SV(countDropPacket)++;
				printk("[RLC][rlc_AM_pkt_receive_from_mac] the packet has been received completly\n");
				FOUT;
			}
			else
			{
				fsm_pkt_destroy(amIns->amRecvWindow[SN]->pkt);
				amIns->amRecvWindow[SN]->pkt = NULL;
				fsm_mem_free(amIns->amRecvWindow[SN]);
				amIns->amRecvWindow[SN] = NULL;
				fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] the packet %d has been received but not completly\n",SN);
			}
		}
		ambuf = (AmBuffer *)fsm_mem_alloc(sizeof(AmBuffer ));
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
		extseghead = (RLC_AMPDU_seg_exthead*)(fixhead+1);
		//seghead = (RLC_AMPDU_seg_head *)fixhead;
		//    seghead->head = seghead->head;
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] fixhead->SN=%d\n",fixhead->SN);
		SN = fixhead->SN;
		SN %= 1024;
		//SN %= 512;

		//在接收窗口外,或已收到过其包含的字节分段
		if((!isInsideAmRecvWin(amIns, SN) )||((amIns->amRecvWindow[SN] != NULL)&&(amIns->amRecvWindow[SN]->pktstatus == FULL_SDU)))
		{
			printk("[RLC][rlc_AM_pkt_receive_from_mac] PDU segment is outside the receiving window or the segment has been received\n");
			printk("[RLC][rlc_AM_pkt_receive_from_mac] A packet has dropped\n");
			SV(countDropPacket)++;
			fsm_pkt_destroy(pkt);
			pkt = NULL;
			FOUT;
		}
		/*      i = 0;
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
			//   ambuf->pkt = pkt;
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
				fsm_pkt_destroy(pkt);
				pkt = NULL;
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
			//丢弃重复分段
			if(extseghead->LSF == 1)
			{
				//amIns->amRecvWindow[SN]->pktstatus = LAST_SEGMENT;  //20140914加
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
	if(SN >= amIns->vr_h||SN <= amIns->vr_h-AM_WINDOW_SIZE)
	{
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] SN > amIns->vr_h\n");
		amIns->vr_h = (SN + 1)%1024;
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] amIns->vr_h is updated to %d\n",amIns->vr_h);
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
		//while((amIns->amRecvWindow[(++amIns->vr_r)%512] != NULL)&&(amIns->amRecvWindow[amIns->vr_r]->pktstatus == FULL_SDU));
		amIns->vr_r = (amIns->vr_r+1)%1024;
		while((amIns->amRecvWindow[amIns->vr_r] != NULL)&&(amIns->amRecvWindow[amIns->vr_r]->pktstatus == FULL_SDU))
		{
			amIns->vr_r = (amIns->vr_r+1)%1024;
		}
		amIns->vr_mr = (amIns->vr_r + AM_WINDOW_SIZE) % 1024;
		//amIns->vr_mr = (amIns->vr_r + AM_WINDOW_SIZE) % 512;
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] amIns->vr_r is updated to %d, amIns->vr_mr is updated to %d\n",amIns->vr_r, amIns->vr_mr);
		// fsm_pkt_send(rlc_AM_reassemble_deliver(amIns),STRM_TO_PDCP);
		//fsm_printf("[RLC] ICI RNTI = %d\n", icimsg->rnti);
		rlc_AM_reassemble_deliver(amIns,&(amIns->pduLft));
		//fsm_mem_free(icimsg);
		//icimsg = NULL;
	}
	/*rlc_AM_reassemble_deliver(amIns,&(amIns->pduLft),icimsg);
	fsm_mem_free(icimsg);
	icimsg = NULL;*/

	//如果定时器正在计时，且x>r,或者x在窗口外并且不等于mr
	if(amIns->reorderTimer &&((amIns->vr_x == amIns->vr_r)||(!isInsideAmRecvWin(amIns, SN)&&(amIns->vr_x != amIns->vr_mr))))
	{
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] timer is canceling\n");
		fsm_schedule_cancel(amIns->reorderTimer);
		amIns->reorderTimer = 0;
	}
	if(!amIns->reorderTimer &&(amIns->vr_h > amIns->vr_r||amIns->vr_h < amIns->vr_r-AM_WINDOW_SIZE))
	{
		//fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] timer is scheduling\n");
		amIns->reorderTimer = fsm_schedule_self(amIns->reorderTimerValue ,amIns->reorderTimerCode);
		amIns->vr_x = amIns->vr_h;
		fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] amIns->vr_x is updated to %d\n",amIns->vr_x);
	}
	//请求轮询
	if(fixhead->P == 1)
	{
		if(!amIns->statProhTimer)
		{
			//fsm_printf("poll is called\n\n");
			amIns->statFlag = true;
			amIns->statProhTimer = fsm_schedule_self(amIns->statProhTimerValue,amIns->statProhTimerCode);
		}
	}

	/*fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] amIns->vr_h is %d, amIns->vr_mr is %d, amIns->vr_ms is %d, amIns->vr_r is %d, amIns->vr_x is %d\n",amIns->vr_h, amIns->vr_mr, amIns->vr_ms, amIns->vr_r, amIns->vr_x);
	for(i = amIns->vr_r; i < amIns->vr_ms; ++i)
	{
		if(amIns->amRecvWindow[i] == NULL)
		{
			fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] %d is not received \n", i);
		}
		else
		{
			fsm_printf("[RLC][rlc_AM_pkt_receive_from_mac] %d is reveived and the status is %d\n", i, amIns->amRecvWindow[i]->pktstatus);
		}
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
void rlc_UM_pkt_receive_from_mac(UM_RX_Instance *umRxIns, FSM_PKT * pkt)
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
	//mac_ici = (URLC_IciMsg *)pkt->head;
	//icimsg = (URLC_IciMsg *)fsm_mem_alloc(sizeof(URLC_IciMsg));
	//icimsg->rnti = mac_ici->rnti;
	//icimsg->pbCh = mac_ici->pbCh;
	//icimsg->rbId = mac_ici->lcid;
	fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] entering rlc_UM_pkt_receive_from_mac\n");
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
			//printk("[RLC][rlc_UM_pkt_receive_from_mac] has been received or reordered(SN is %d)\n", SN);
			//printk("[RLC][rlc_UM_pkt_receive_from_mac] A packet has dropped\n");
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
		// head_size10->head = fsm_ntohs(head_size10->head);
		SN = head_subhead->SN;		
		base = umRxIns->vr_uh - UM_WINDOW_SIZE10 ;
		windows = UM_WINDOW_SIZE10 *2;
		SN %= windows;
	
		//if((umRxIns->umRecvWindow10[SN]!=NULL) || isOrderedUm(umRxIns,SN))
		if((SN-umRxIns->vr_ur+1024)%1024<(umRxIns->vr_uh-umRxIns->vr_ur+1024)%1024&&(umRxIns->umRecvWindow10[SN]!=NULL) || isOrderedUm(umRxIns,SN))
		{
			//printk("[RLC][rlc_UM_pkt_receive_from_mac] has been received or reordered(SN is %d),vr_uh = %d, vr_ur = %d\n",SN,umRxIns->vr_uh,umRxIns->vr_ur);
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
	fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] pkt has been put into buffer(SN is %d, vr_uh is %d)\n", SN, umRxIns->vr_uh);
	if(umRxIns->snFiledLength == 5)
	{
		umRxIns->umRecvWindow5[SN] = umbuf;
	}
	else if(umRxIns->snFiledLength == 10)
	{
		umRxIns->umRecvWindow10[SN] = umbuf;
	}
	umRxIns->vr_uh %= windows;
	//umRxIns->umRecvWindow5[SN] = umbuf;  //???????
	//重排序窗口外
	if((SN>=umRxIns->vr_uh)||(SN <= umRxIns->vr_uh - windows/2))
	{
		//  fsm_printf("umbuf->SN  >=  umRxIns->vr_uh is %d\n",umRxIns->vr_uh);
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
			umRxIns->vr_ur =  temp;
			fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_ur is updated to %d\n", umRxIns->vr_ur);*/
		}		
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_ur is updated to %d\n", umRxIns->vr_ur);
		//  fsm_printf("umRxIns->vr_ur is %d\n",umRxIns->vr_ur);
		//     fsm_pkt_send(rlc_UM_reassemble_deliver( umRxIns),STRM_TO_PDCP);
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
			//    fsm_pkt_send(rlc_UM_reassemble_deliver( umRxIns),STRM_TO_PDCP);
		}
		//rlc_UM5_reassemble_deliver(umRxIns,&umRxIns->pduLft,icimsg);
	}

	else if(umRxIns->snFiledLength == 10)
	{
		fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] entering 10\n");
		if(umRxIns->umRecvWindow10[umRxIns->vr_ur] != NULL)
		{
			//fsm_printf("umbuf->SN  ==  umRxIns->vr_ur is %d\n", umRxIns->vr_ur);
			//while((umRxIns->umRecvWindow10[++umRxIns->vr_ur] != NULL)&&(umRxIns->vr_ur <= umRxIns->vr_uh ));
			umRxIns->vr_ur = (umRxIns->vr_ur + 1) % windows;
			while((umRxIns->umRecvWindow10[umRxIns->vr_ur] != NULL)&&(umRxIns->vr_ur <= umRxIns->vr_uh ))
			{
				umRxIns->vr_ur = (umRxIns->vr_ur + 1) % windows;
			}
			fsm_printf("[RLC][rlc_UM_pkt_receive_from_mac] umRxIns->vr_ur is updated to %d\n", umRxIns->vr_ur);
			rlc_UM_reassemble_deliver(umRxIns,&umRxIns->pduLft);
			//    fsm_pkt_send(rlc_UM_reassemble_deliver( umRxIns),STRM_TO_PDCP);
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
	packetBuf *pktbuf, *tempPktBuf;
	struct RntiManag *rntiM, *tempRntiM;
	struct  AM_Instance *amIns, *tempAmIns;
	struct  UM_RX_Instance *umRxIns,*tempUrIns;
	struct  UM_TX_Instance *umTxIns, *tempUtIns;
	struct Buffer *buffer, *tempBuffer;
	FIN(rlc_close());
	SV_PTR_GET(rlc_mac_sv);

	if(!list_empty(&SV(pktBuf).list))
	{
		list_for_each_entry_safe(pktbuf, tempPktBuf, &SV(pktBuf).list, list)
		{
			list_del(&pktbuf->list);
			if(pktbuf->pkt != NULL)
			{
				fsm_pkt_destroy(pktbuf->pkt);
				pktbuf->pkt = NULL;
			}
			if(list_empty(&SV(pktBuf).list))
			{
				fsm_printf("[RLC][rlc_close] the pktBuf has released\n");
				break;
			}
		}
	}

	if(!list_empty(&SV(rntiManag).list))
	{
		list_for_each_entry_safe(rntiM, tempRntiM, &SV(rntiManag).list, list)
		{
			list_del(&rntiM->list);
			//释放AM实体
			if(!list_empty(&rntiM->amIns.amList))
			{
				list_for_each_entry_safe(amIns,tempAmIns, &rntiM->amIns.amList,amList)
				{
					rlc_AM_release(amIns);
					if(list_empty(&rntiM->amIns.amList))
					{
						break;
					}
				}
			}
			//释放UM接收实体
			if(!list_empty(&rntiM->umRxIns.umRxList))
			{
				list_for_each_entry_safe(umRxIns,tempUrIns, &rntiM->umRxIns.umRxList,umRxList)
				{
					rlc_UMrx_release(umRxIns);
					if(list_empty(&rntiM->umRxIns.umRxList))
					{
						break;
					}
				}
			}
			//释放UM发送实体
			if(!list_empty(&rntiM->umTxIns.umTxList))
			{
				list_for_each_entry_safe(umTxIns,tempUtIns,&rntiM->umTxIns.umTxList,umTxList)
				{
					rlc_UMtx_release(umTxIns);
					if(list_empty(&rntiM->umTxIns.umTxList))
					{
						break;
					}
				}
			}

			//释放TM发送实体
			if(!list_empty(&rntiM->ccchTran.tmSduBuffer.list))
			{
				list_for_each_entry_safe(buffer, tempBuffer,&rntiM->ccchTran.tmSduBuffer.list,list)
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
					fsm_printf("[RLC][rlc_close] CCCH:Sdu buffer is realeasing\n");
					if(list_empty(&rntiM->ccchTran.tmSduBuffer.list))
					{
						break;
					}
				}
			}
			if(rntiM != NULL)
			{
				fsm_mem_free(rntiM);
				rntiM = NULL;
			}
			if(list_empty(&SV(rntiManag).list))
			{
				break;
			}
		}
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
void  findInsByRbid(u16 rnti,u8 rbid,void **up, void **down , RlcMode *mode)   //
{
	struct list_head* pos;
	struct RntiManag *rntiM;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	// TM_RX_Instance *tmRxIns;
	//   TM_TX_Instance *tmTxIns;
	FIN(findInsByRbid());
	SV_PTR_GET(rlc_mac_sv);
	*up = *down = NULL;
	*mode = 0;
	rntiM = findUeByRnti(rnti);
	if(rntiM == NULL)
	{
		fsm_printf("[RLC][findInsByRbid] cannot find the UE(rnti is %d)\n", rnti);
		FOUT;
	}
	if(rntiM->ccchRecv.rbId == rbid)
	{
		*down = (void*)&rntiM->ccchRecv;
		*mode = TM_MODE;
	}
	if(rntiM->ccchTran.rbId == rbid)
	{
		*up =  (void*)&rntiM->ccchTran;
		*mode = TM_MODE;
	}
	if(*mode == TM_MODE)
	{
		FOUT;
	}
	if (!list_empty(&rntiM->umTxIns.umTxList))
	{
		list_for_each(pos,&rntiM->umTxIns.umTxList)
		{
			umTxIns= list_entry(pos,struct UM_TX_Instance,umTxList);
			if(umTxIns->rbid == rbid)
			{
				*up =  (void*)umTxIns;
				*mode = UM_MODE;
			}
		}
	}
	if(!list_empty(&rntiM->umRxIns.umRxList))
	{
		list_for_each(pos,&rntiM->umRxIns.umRxList)
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
	{
		FOUT;
	}
	if (!list_empty(&rntiM->amIns.amList))
	{
		list_for_each(pos,&rntiM->amIns.amList)
		{
			amIns = list_entry(pos,struct AM_Instance,amList);
			if(amIns->rbId == rbid)
			{
				*mode = AM_MODE;
				*up = *down =  (void*)amIns;
			}
		}
	}
	if(*mode == AM_MODE)
	{
		FOUT;
	}
	fsm_printf("[RLC][findInsByRbid] cannot find the instance(rbid is %d)\n", rbid);
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
void  findInsBylcid(u16 rnti, u8 lcid, void **up, void**down, RlcMode *mode)
{
	struct list_head* pos;
	struct RntiManag *rntiM;
	struct  AM_Instance * amIns;
	struct  UM_RX_Instance *umRxIns;
	struct  UM_TX_Instance *umTxIns;
	TM_RX_Instance *tmRxIns;
	TM_TX_Instance *tmTxIns;
	FIN(findInsBylcid());
	SV_PTR_GET(rlc_mac_sv);
	*up = *down = NULL;
	*mode = 0;
	rntiM = findUeByRnti(rnti);
	int i = 0;
	//printk("[RLC_MAC][findInsBylcid]lcid: %d\n", lcid);
	if(rntiM == NULL)
	{
		//fsm_printf("[RLC][findInsBylcid] cannot find the UE(rnti is %d)\n", rnti);
		FOUT;
	}
	if(rntiM->ccchRecv.lcId == lcid)
	{
		//printk("[RLC_MAC][findInsBylcid]rntiM->ccchRecv.lcId: %d\n", rntiM->ccchRecv.lcId);
		*down = (void*)&rntiM->ccchRecv;
		*mode = TM_MODE;
	}
	if(rntiM->ccchTran.lcId == lcid)
	{
		//printk("[RLC_MAC][findInsBylcid]rntiM->ccchTran.lcId: %d\n", rntiM->ccchTran.lcId);
		*up = (void*)&rntiM->ccchTran;
		*mode = TM_MODE;
	}
	if(*mode == TM_MODE)
	{
		FOUT;
	}
	if (!list_empty(&rntiM->umTxIns.umTxList))
	{
		list_for_each(pos,&rntiM->umTxIns.umTxList)
		{
			umTxIns= list_entry(pos,struct UM_TX_Instance,umTxList);
			if(umTxIns->lcid == lcid)
			{
				*mode = UM_MODE;
				*up =  (void*)umTxIns;
			}
		}
	}
	if(!list_empty(&rntiM->umRxIns.umRxList))
	{
		list_for_each(pos,&rntiM->umRxIns.umRxList)
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
	{
		FOUT;
	}
	if (!list_empty(&rntiM->amIns.amList))
	{
		list_for_each(pos,&rntiM->amIns.amList)
		{
			amIns = list_entry(pos,struct AM_Instance,amList);
			//fsm_printf("[RLC][findInsBylcid][AM] amInsAddr =%p,rbid=%d,lcid=%d\n",amIns,amIns->rbId,amIns->lcId);
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
	struct  RntiManag *rntiM;
	FIN(findInsByCode());
	SV_PTR_GET(rlc_mac_sv);

	if( SV(bufferReqTimerCode) == code )  //20140915加
	{
		*timer = BUFFER_REQ;
		return NULL;
	}

	if( SV(pktBufTimerCode) == code )  //20141202加
	{
		*timer = PKT_BUF;
		return NULL;
	}
	
	if(!list_empty(&SV(rntiManag).list))
	{
		list_for_each_entry(rntiM, &SV(rntiManag).list, list)
		{
			if(!list_empty(&rntiM->umRxIns.umRxList))
			{
				list_for_each(pos,&rntiM->umRxIns.umRxList)
				{
					umRxIns = list_entry(pos,struct  UM_RX_Instance,umRxList);
					if(umRxIns->reorderTimerCode == code)
					{
						*timer = UM_REORDER;
						return (void*)umRxIns;
					}
				}
			}
			if (!list_empty(&rntiM->amIns.amList))
			{
				list_for_each(pos,&rntiM->amIns.amList)
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
		}
	}
	/*else
	{
		fsm_printf("[RLC_MAC][findInsByCode]timer: %d\n", *timer);
		*timer = UL_SCHEDULE;
		return NULL;
	}*/
}


/************************************************************************
**Function name: findUeByRnti()
**Descriptions: Find UE according to rnti
**Input: u16 rnti
**Output: RntiManag * rntiM
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
RntiManag*findUeByRnti(u16 rnti)
{
	struct RntiManag *rntiM = NULL;
	FIN(findUeByRnti());
	SV_PTR_GET(rlc_mac_sv);
	if(!list_empty(&SV(rntiManag).list))
	{
		list_for_each_entry(rntiM,&SV(rntiManag).list, list)
		{
			if(rntiM->rnti == rnti)
			{
				return rntiM;
			}
		}
	}
	return NULL;
}


/************************************************************************
**Function name: rlc_AM_release()
**Descriptions: Release AM_Instance
**Input: struct AM_Instance *amIns
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
void rlc_AM_release(struct AM_Instance *amIns)
{
	struct AmSduBuffer*amsBuffer, *tempAmsBuffer;
	struct AmBuffer *amBuffer, *tempAmBuffer;
	struct Buffer *buffer, *tempBuffer;
	//int i;
	FIN( rlc_AM_release());
	SV_PTR_GET(rlc_mac_sv);    
	list_del(&amIns->amList);
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
			if(amBuffer->pkt!=NULL)
			{
				fsm_pkt_destroy(amBuffer->pkt);
				amBuffer->pkt = NULL;
			}
			if(amBuffer!=NULL)
			{
				fsm_mem_free(amBuffer);
				amBuffer = NULL;
			}
			fsm_printf("[RLC][rlc_AM_release] am:retx buffer is realeasing\n");
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
			if(amBuffer->pkt!=NULL)
			{
				fsm_pkt_destroy(amBuffer->pkt);
				amBuffer->pkt = NULL;
			}
			if(amBuffer!=NULL)
			{
				fsm_mem_free(amBuffer);
				amBuffer = NULL;
			}
			fsm_printf("[RLC][rlc_AM_release] am:transmited buffer is realeasing\n");
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
			if(amsBuffer->pkt!=NULL)
			{
				fsm_pkt_destroy(amsBuffer->pkt);
				amsBuffer->pkt = NULL;
			}
			if(amsBuffer!=NULL)
			{
				fsm_mem_free(amsBuffer);
				amsBuffer = NULL;
			}
			fsm_printf("[RLC][rlc_AM_release] am:sdu buffer is realeasing\n");
			if(list_empty(&amIns->amSduBuffer.list))
			{
				break;
			}
		}
	}
	//释放状态PDU队列
	if(!list_empty(&amIns->ctrlPduBuffer.list))
	{
		list_for_each_entry_safe(buffer,tempBuffer, &amIns->ctrlPduBuffer.list,list)
		{
			list_del(&buffer->list);
			if(buffer->pkt!=NULL)
			{
				fsm_pkt_destroy(buffer->pkt);
				buffer->pkt = NULL;
			}
			if(buffer!=NULL)
			{
				fsm_mem_free(buffer);
				buffer = NULL;
			}
			fsm_printf("[RLC][rlc_AM_release] am:ctrl buffer is realeasing\n");
			if(list_empty(&amIns->ctrlPduBuffer.list))
			{
				break;
			}
		}
	}
	//释放am接收队列
	if(!list_empty(&amIns->amRecvBuffer.list))
	{
		list_for_each_entry_safe(amBuffer,tempAmBuffer, &amIns->amRecvBuffer.list,list)
		{
			list_del(&amBuffer->list);
			if(amBuffer->pkt!=NULL)
			{
				fsm_pkt_destroy(amBuffer->pkt);
				amBuffer->pkt = NULL;
			}
			if(amBuffer!=NULL)
			{
				fsm_mem_free(amBuffer);
				amBuffer = NULL;
			}
			fsm_printf("[RLC][rlc_AM_release] am:recv buffer is realeasing\n");
			if(list_empty(&amIns->amRecvBuffer.list))
			{
				break;
			}
		}
	}

	/*for(i=0; i < AM_WINDOW_SIZE*2; i++)
	{
		amIns->amRecvWindow[i] = NULL;
		amIns->amTranWindow[i] = NULL;
	}*/
	fsm_printf("[RLC][rlc_AM_release] am instance  has released(rbid is%d)\n", amIns->rbId);
	if(amIns!=NULL)
	{
		fsm_mem_free(amIns);
		amIns = NULL;
	}
	FOUT;
}


/************************************************************************
**Function name: rlc_UMrx_release()
**Descriptions: Release UM_RX_Instance
**Input: struct UM_RX_Instance* umRxIns
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
void rlc_UMrx_release(struct UM_RX_Instance* umRxIns)
{
	struct UmBuffer *umBuffer, *tempUmBuffer;
	FIN( rlc_UMrx_release());
	SV_PTR_GET(rlc_mac_sv);    
	list_del(&umRxIns->umRxList);
	if(!list_empty(&umRxIns->umRecvBuffer.list))
	{
		list_for_each_entry_safe(umBuffer,tempUmBuffer, &umRxIns->umRecvBuffer.list,list)
		{
			list_del(&umBuffer->list);
			//回收定时器的code 值
			SV(allocCode[umRxIns->reorderTimerCode]) = umRxIns->reorderTimerCode;
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
			fsm_printf("[RLC][rlc_UMrx_release] um recv:recv buffer is realeasing\n");
			if(list_empty(&umRxIns->umRecvBuffer.list))
			{
				break;
			}
		}
	}
	fsm_printf("[RLC][rlc_UMrx_release] um instance  has released(rbid is %d)\n",umRxIns->rbId);
	if(umRxIns != NULL)
	{
		fsm_mem_free(umRxIns);
		umRxIns = NULL;
	}
	
	FOUT;
}


/************************************************************************
**Function name: rlc_UMtx_release()
**Descriptions: Release UM_TX_Instance
**Input: struct UM_TX_Instance* umTxIns
**Output:
**Returns:
**Created by: ChenHongying
**Created Date: 2013
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/
void rlc_UMtx_release(struct UM_TX_Instance* umTxIns)
{
	struct UmBuffer *umBuffer, *tempUmBuffer;
	FIN( rlc_UMtx_release());
	SV_PTR_GET(rlc_mac_sv);    
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
			fsm_printf("[RLC][rlc_UMtx_release] um tran:sdu buffer is realeasing\n");
			if(list_empty(&umTxIns->umSduBuffer.list))
			{
				break;
			}
		}
	}
	fsm_printf("[RLC][rlc_UMtx_release] um instance  has released(rbid is %d)\n",umTxIns->rbid);
	if(umTxIns != NULL)
	{
		fsm_mem_free(umTxIns);
		umTxIns = NULL;
	}	
	FOUT;
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
	//printk("[RLC][isInsideTranWin] vt_a=%d, vt_ms=%d, SN=%d\n",amIns->vt_a,amIns->vt_ms,SN);
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
	fsm_printf("[RLC][rlc_UMtx_release] the upper is %d, upperMode is %d\n",upper, upperMode);
	return(((amIns->vr_r <= SN)&&(SN < upper))||(((upperMode - 512) < SN )&&(SN < upperMode)));*/
	printk("[RLC][isInsideAmRecvWin] vr_r=%d, vr_mr=%d, SN=%d\n",amIns->vr_r,amIns->vr_mr,SN);
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
	if(umRxIns->snFiledLength == 5)
	{
		//return ((umRxIns->vr_uh <= SN + 16)&&(SN < umRxIns->vr_ur));
		return (SN-(umRxIns->vr_uh-16)+32)%32 < (umRxIns->vr_ur-(umRxIns->vr_uh-16)+32)%32;
	}
	else
	{
		//return ((umRxIns->vr_uh <= SN + 512)&&(SN < umRxIns->vr_ur));
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
	RLC_AMPDU_seg_exthead* exthead;
	//RLC_AMPDU_fixhead* fixhead;
	// fsm_printf("the count of this segment is %d\n", countSegInd(pkt));
	u16 headLen;
	u32 posStart;
	u32 posEnd;
	char *pospkt;
	char *posbuf;
	long datalen;
	int i,j;
	u16 flag;
	FSM_PKT*pkptr;
	
	//fixhead = (RLC_AMPDU_fixhead* )pkt->data;
	exthead = (RLC_AMPDU_seg_exthead* )(pkt->data+sizeof(RLC_AMPDU_fixhead));
	headLen = 4 + 2*countSegInd(ambuf->pkt);
	posEnd = posStart + pkt->len - headLen;
	pospkt = (char*)pkt->data;
	posbuf = (char*)ambuf->pkt->data;
	posStart  =  exthead->SO;
	datalen = 0;
	flag = 0;
	pospkt = pospkt + headLen;
	//fsm_printf("[RLC][assamble_segment] the packet data is %s\n", pospkt);
	posbuf = posbuf + headLen;
	//fsm_printf("[RLC][assamble_segment] the buffer data is %s\n", posbuf);

	for(i = 0; i < ambuf->segnum; ++i)
	{
		//接收到的分段重复，丢弃
		if((ambuf->segStart[i] <= posStart) &&(posEnd <= ambuf->segEnd[i] ))
		{
			fsm_printf("[RLC][assamble_segment] this segment %d has been received\n",i);
			fsm_pkt_destroy(pkt);
			pkt = NULL;
			FOUT;
		}
	}
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
				fsm_printf("[RLC][assamble_segment] condition 1\n");
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
				fsm_printf("[RLC][assamble_segment] condition 2\n");
				ambuf->segStart[0] = posStart;
				FOUT;
			}
			//情况3:
			if(( posStart <= ambuf->segStart[0])&&(ambuf->segStart[0] < posEnd)&&\
				(posStart < ambuf->segEnd[0])&&(ambuf->segEnd[0] < posEnd)&&(posEnd < ambuf->segStart[1]))
			{
				fsm_printf("[RLC][assamble_segment] condition 3\n");
				ambuf->segStart[0] = posStart;
				ambuf->segEnd[0] = posEnd;
				FOUT;
			}
		}
		//收到的分段起始位置在中间
		for(i = 0; i < ambuf->segnum - 1; ++i)
		{
			//情况4;
			if((ambuf->segEnd[i] < posStart)&&(posStart < ambuf->segStart[i + 1])&&\
				(ambuf->segEnd[i] < posEnd)&&(posEnd < ambuf->segStart[i + 1]))
			{
				fsm_printf("[RLC][assamble_segment] condition 4\n");
				for(j = ambuf->segnum - 1; j > i; --j)
				{
					ambuf->segEnd[j+1] = ambuf->segEnd[j];
					ambuf->segStart[j+1] = ambuf->segStart[j];
				}
				ambuf->segStart[j + 1] = posStart;
				ambuf->segEnd[j + 1] = posEnd;
				ambuf->segnum++;
				FOUT;
			}
			//接收分段起始在已收分段里面
			else if((ambuf->segStart[i] <=  posStart) &&(posStart <= ambuf->segEnd[i]))
			{
				for(j = i + 1; j < ambuf->segnum; ++j)
				{
					//情况5:
					if((ambuf->segStart[j] <= posEnd )&&(posEnd <= ambuf->segEnd[j]))
					{
						fsm_printf("[RLC][assamble_segment] condition 5\n");
						ambuf->segEnd[i] = ambuf->segEnd[j];
						for(; i < ambuf->segnum; ++j, ++i)
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
						fsm_printf("[RLC][assamble_segment] condition 6\n");
						ambuf->segEnd[i] = posEnd;
						i++;
						for(; i < ambuf->segnum; ++j, ++i)
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
				for(j = i + 1; j < ambuf->segnum; ++j)
				{
					//情况7:
					if((ambuf->segStart[j] <= posEnd )&&(posEnd <= ambuf->segEnd[j]))
					{
						fsm_printf("[RLC][assamble_segment] condition 7\n");
						ambuf->segStart[++i] = posStart;
						ambuf->segEnd[i] = ambuf->segEnd[j];
						i++;j++;
						for(; i < ambuf->segnum; ++j, ++i)
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
						fsm_printf("[RLC][assamble_segment] condition 8\n");
						ambuf->segStart[++i] = posStart;
						ambuf->segEnd[i] = posEnd;
						i++;
						for(; i < ambuf->segnum; ++j, ++i)
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
					fsm_printf("[RLC][assamble_segment] condition 10\n");
					ambuf->segEnd[i] = posEnd;
					ambuf->segnum = i + 1;
					FOUT;
				}
				//情况11:
				else if((ambuf->segEnd[i - 1] < posStart)&&(posStart <= ambuf->segStart[i]))
				{
					fsm_printf("[RLC][assamble_segment] condition 11\n");
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

