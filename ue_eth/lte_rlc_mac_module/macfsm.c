#include "../fsm/fsmdec.h"
#include <linux/if_ether.h>
/****lhl改动20140410****/
#include <linux/list.h>
#include <linux/kernel.h>
#include <stddef.h>
/****lhl改动结束20140410****/ 
#include "macfsm.h"
#include "macpkfmt.h"
#include "macscheduler.h"
#include "rrc_config.h"
#include "../lte_system.h"


/* fsm state id definition  定义状态*/
#define ST_INIT 0
#define ST_IDLE 1
#define ST_CRA	2 /*contention-based random access process */
#define ST_NCRA	3
#define ST_CON  4
#define ST_SENDMSG3 5 //20140430 HQ

//testing functions 测试函数   HQ 
#define IOCCMD_TEST_lostUL  0x30 //测试TA_timer超时
#define NCRATEST 0x31

/*funcation declaration 函数声明 */
//static void init_enter(void);   
//static void ue_mac_close(void);
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
static void send_msg3(void); //defined by HQ 20140430 
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
/*
******************************
->Change Data and time: 
->Function:Initialization parameters
->Change:
->Details:状态机
->input:
->output:
->Special:
*******************************
*/
void mac_main()
{
	FSM_ENTER(mac_main);
	FSM_BLOCK_SWITCH
	{
		FSM_STATE_FORCED(ST_INIT, "INIT", ue_mac_sv_init(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "INIT -> IDLE");
		}
		FSM_STATE_UNFORCED(ST_IDLE, "IDLE", , )
		{
			FSM_COND_TEST_IN("IDLE")				
				FSM_TEST_COND(START_C_RANDOM_ACCESS)
				FSM_TEST_COND(START_NC_RANDOM_ACCESS)
				FSM_TEST_COND(MAC_RESET)
			FSM_COND_TEST_OUT("IDLE")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_CRA, doStartContentionBasedRandomAccessProcedure(), "IDLE -> CRA")
				FSM_CASE_TRANSIT(1, ST_NCRA, doStartNonContentionBasedRandomAccessProcedure(), "IDLE -> NCRA")
				//FSM_CASE_TRANSIT(2, ST_IDLE, print_tran_info("reset IDLE -> IDLE"), "IDLE -> IDLE")
				FSM_CASE_TRANSIT(2, ST_IDLE, ue_mac_reset(), "IDLE -> IDLE")
				FSM_CASE_DEFAULT(ST_IDLE,idle_default_handler() , "IDLE->IDLE")	//transit to idle state	by default.
			}
		}
		FSM_STATE_UNFORCED(ST_CRA,"CRA", ,)
		{
			FSM_COND_TEST_IN("CRA")
				FSM_TEST_COND(TimeAlignmentTimerExpire)
				FSM_TEST_COND(RANDOM_ACCESS_FAIL)
				FSM_TEST_COND(RARREV_SUCCESS)
				FSM_TEST_COND(CONTENTION_FAIL )
				FSM_TEST_COND(CONTENTION_SUCCESS)
				FSM_TEST_COND(MAC_RESET)
			FSM_COND_TEST_OUT("CRA")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE,handle_timeAlignmentTimerExpire(), "CRA-> IDLE")
				FSM_CASE_TRANSIT(1, ST_IDLE, C_RaResponseFailed(), "CRA -> IDLE")
				FSM_CASE_TRANSIT(2, ST_SENDMSG3 , waitsending_msg3(), "CRA -> SENDMSG3")
				FSM_CASE_TRANSIT(3, ST_IDLE, ContentionFailed(), "CRA -> IDLE")
				FSM_CASE_TRANSIT(4, ST_CON , cra_success() , "CRA -> CON")
				//FSM_CASE_TRANSIT(4, ST_IDLE, print_tran_info("reset CRA -> IDLE"), "CRA -> IDLE")
				FSM_CASE_TRANSIT(5, ST_IDLE, ue_mac_reset(), "CRA -> IDLE")
				FSM_CASE_DEFAULT(ST_CRA,cra_default_handler() , "CRA->CRA")	//transit to CRA state	by default.
			}
		}
		FSM_STATE_UNFORCED(ST_SENDMSG3,"SENDMSG3", ,)
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
		}		
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
			FSM_COND_TEST_OUT("CON")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE,handle_timeAlignmentTimerExpire(), "CON -> IDLE")//handle_timeAlignmentTimerExpire()
				FSM_CASE_TRANSIT(1, ST_CRA, doStartContentionBasedRandomAccessProcedure() , "CON -> CRA")
				//FSM_CASE_TRANSIT(2, ST_IDLE,print_tran_info("reset CON -> IDLE"), "CON -> IDLE")
				FSM_CASE_TRANSIT(2, ST_IDLE,ue_mac_reset(), "CON -> IDLE")
				FSM_CASE_DEFAULT(ST_CON,con_default_handler() , "CON -> CON")	//transit to CON state	by default.
			}
		}
	}
	FSM_EXIT(0)
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
	
	SV_PTR_GET(mac_sv);
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
FOUT;
}

//TEST FUNCTION20141103
static void test_init_logicalchannel(u16 lcid)
{
	FIN(test_init_logicalchannel());
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	FIN(idle_default_handler());
	SV_PTR_GET(mac_sv);
	print_tran_info("[UEMAC][idle_default]MAC idle_default_handler");//testing code 
	if(MAC_CLOSE)
	{
		print_tran_info("[UEMAC][idle_default]MAC FSM is closed ");//testing code
		ue_mac_sv_close();
	}
	if(MAC_PK_FROM_LOWER)
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
	}
	if(MAC_PK_FROM_UPPER )
	{//在IDLE状态下应该是不会遇到这个情况,所以打印一条错误信息
		print_tran_info("[UEMAC][idle_default][-->]MAC IDLE receive a data packet from RLC, error, and then start CRA\n");//testing code 
		//fsm_schedule_self(0, Start_ContentionBase_RandomAcc);//测试复用解复用，暂时注释掉 
		/*if(SV(TATimer.timer_sign)==NULL)
		{
			//由上行失步引起的IDLE态
			fsm_pkt_destroy(fsm_pkt_get());
		}
		else
		{*/
		complexUeMacPdu(fsm_pkt_get());
		if(SV(Tbsize_Complex)!=0)
			SV(Tbsize_Complex)=0;//复用之后，重置TBSIZE
		//}
	}
	if(TimeAlignmentTimerExpire)
	{
		handle_timeAlignmentTimerExpire();
	}
	if(LogicalChannel_BJ)//modified 20141105 LHL,更新BJ
	{
		FlushBj();
		fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);
	}
	if(IOCTRL_ARRIVAL)
	{//处理IOCTL命令
		print_tran_info("[UEMAC][idle_default]MAC IDLE default ioctl hander,");//testing code
		u32 cmd1=fsm_ev_ioctrl_cmd();
		fsm_printf("[MAC][DEFAULT_CMD]%d\n",cmd1);
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
	}
	reports_handler();//处理BSR PHR等上报事件 
	FOUT;
}
/*
static void idle_default_handler()
{
//处理与状态转换无关的事件:下层数据包到达、与状态无关的ioctrl命令、BSR/PHR的周期定时器超时
	void *tmpdata_ptr;
   FIN(idle_default_handler());
   SV_PTR_GET(mac_sv);
   print_tran_info("idle_default_handler");
   if(MAC_CLOSE)
   {
      print_tran_info("MAC FSM is closed "); //testing code
      ue_mac_sv_close();
   }
   if(MAC_PK_FROM_LOWER)
   {//通过ICI信息判断包类型，应该是广播消息，调用发送函数，发给RLC
      print_tran_info("IDLE receive a data packet from PHYadapter"); //testing code
      //decomplexUeDataPdu(fsm_pkt_get());

   }
   if(MAC_PK_FROM_UPPER )
   {//在IDLE状态下应该是不会遇到这个情况,所以打印一条错误信息
      print_tran_info("IDLE receive a data packet from RLC, error, and then start CRA");//testing code 
      //fsm_schedule_self(0, Start_ContentionBase_RandomAcc);
      //complexUeMacPdu(fsm_pkt_get());//会引起死机 
     fsm_pkt_destroy(fsm_pkt_get());//testing code
     fsm_printf("destory packet\n");
   }
   if(IOCTRL_ARRIVAL)
   {
      print_tran_info("IDLE default ioctl hander");//testing code
      if(fsm_ev_ioctrl_cmd()== IOCCMD_RLCtoMAC_datasend_Req)
      {
		  tmpdata_ptr = fsm_data_get();
          //DoRefreshRLCBuffserRequest((RlcBufferRequest *)tmpdata_ptr);//更新待发数据缓存信息
          fsm_schedule_self(1,Start_ContentionBase_RandomAcc); //触发随机接入 ，时间单位是10us 这个时间设置多长待究
          fsm_data_destroy(tmpdata_ptr);
       }
      else if(fsm_ev_ioctrl_cmd()== IOCCMD_RRCtoMAC_RandomAcc_Req || fsm_ev_ioctrl_cmd()== IOCCMD_PDCCHtoMAC_RandomAcc_Req )//由RRC或PDCCH命令指示的随机接入
           {
             tmpdata_ptr = fsm_data_get();
             print_tran_info("IDLE receive a RA req from other layer ");
             if(tmpdata_ptr > 0)//并指示了关于接入的前导码
             {
              print_tran_info("IDLE receive a ncra order ");
              fsm_mem_cpy(&(SV(RA_info)),tmpdata_ptr,sizeof(RACH_ConfigDedicated));
              fsm_printf("the RAPID is %d\n",SV(RA_info.ra_PreambleIndex));
			  if(tmpdata_ptr != NULL)
			  {
			  	fsm_data_destroy(tmpdata_ptr);//释放内存
				fsm_printf("fsm_ev_data,then free the memory\n");
			  }
                 
               //fsm_schedule_self(400, Start_NonContentionBase_RandomAcc );//触发非竞争随机接入 //这条语句会造成死机，暂注释掉
               //考虑到打印会花费几百微妙的时间，则将定时时间设定为1ms，后面同理；在去掉各个打印函数后，这个时间可以减少到100u以内;其实不需要考虑这个问题              
             }
             else //没有指示关于接入的前导码
             {
               print_tran_info("IDLE receive a cra order ");
               fsm_schedule_self(0, Start_ContentionBase_RandomAcc );//触发竞争随机接入
             }

           }
           else 
             {
               ioctrl_handler();  //与状态无关的ioctrl命令
             } 
    }
 FOUT;
}
*/

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
	FIN(cra_default_handler());
	SV_PTR_GET(mac_sv);
	print_tran_info("[UEMAC][cra_default]cra default ");//testing code
	if(SV(sendmsg3)==1 && SV(C_RNTI)!=0)
		fsm_schedule_self(0, Contention_Success );//TEST  	
	if(MAC_CLOSE)
	{
		print_tran_info("[UEMAC][cra_default]MAC FSM is closed "); //testing code
		ue_mac_sv_close();
	}
	if(MAC_PK_FROM_LOWER)
	{
		//HQ make note 20140710
		//如果是DL_SCH 普通PDU
		//为了判断竞争解决，这里需要增加对于PHY上报的RNTI和UE自己的C_RNTI(如果存在) 进行对比 		
		decomplexUeDataPdu(fsm_pkt_get());
	}
	if(MAC_PK_FROM_UPPER)
	{
		fsm_pkt_destroy(fsm_pkt_get());
		fsm_printf("[UEMAC][cra_default]receive packet from RLC,error! \n");
	}
	if(LogicalChannel_BJ)//modified 20141105 LHL,更新BJ
	{
		FlushBj();
		fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);
	}
	if(IOCTRL_ARRIVAL)
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
		decomplexUeDataPdu(fsm_pkt_get());
	}
	if(MAC_PK_FROM_UPPER)
	{
		fsm_printf("[UEMAC][nra_default]receive packet from RLC,error! \n");
	}
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
	FIN(con_default_handler());
	SV_PTR_GET(mac_sv);
	if(MAC_CLOSE)
	{
		print_tran_info("[UEMAC][con_default]MAC FSM is closed "); //testing code
		ue_mac_sv_close();
	}
	if(SV(DATA_WAIT_ALLOCATION)==true && SV(UL_resource_info.resource_flag)==true)
	{//有上行资源后 处理待发数据
		//DoResourceAllocation(SV(UL_resource_info.m_tbsize)); 
		SV(Tbsize_Complex)=SV(UL_resource_info.m_tbsize)/8;//20140718LHL 单位为byte,复用模块使用
		DoResourceAllocation(GetTBsize_Allocation(SV(UL_resource_info.m_tbsize)));//不发MAC控制单元
		SV(UL_resource_info.resource_flag)=false;
	}
	if(MAC_PK_FROM_LOWER)
	{//通过ICI信息判断包类型 解复用 调用发送函数，发给RLC
		print_tran_info("[UEMAC][con_default]CON receive a data packet from PHYadapter");//testing code
		decomplexUeDataPdu(fsm_pkt_get());
	}
	if(MAC_PK_FROM_UPPER)
	{//通过ICI信息判断包类型 复用 调用发送函数，发给PHYadapter
		print_tran_info("[UEMAC][con_default]CON receive a data packet from RLC");//testing code
		//pkptr=fsm_pkt_get();
		//fsm_pkt_send(pkptr,STRM_TO_SRIO);
		complexUeMacPdu(fsm_pkt_get());
		if(SV(Tbsize_Complex)!=0)
			SV(Tbsize_Complex)=0;//复用之后，重置TBSIZE
		//fsm_pkt_destroy(fsm_pkt_get());//testing code
		//fsm_printf("destory packet\n");
	}
  	if(LogicalChannel_BJ)//modified 20141105 LHL,更新BJ
	{
		FlushBj();
		fsm_schedule_self(MAC_BJ_TIME, LC_BJ_CODE);
	}
	if(IOCTRL_ARRIVAL)
	{ //处理IOCTRL Command
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
		print_tran_info("[UEMAC][con_default]CON default ioctl hander");//testing codes
	}
	reports_handler();//处理BSR PHR等上报事件 
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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

	SV_PTR_GET(mac_sv);

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
			fsm_printf("[LHL]TEMP ULGRANT:%d\n",tempulgrant->ulgrant.RIV);
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
			fsm_printf("[UEMAC][ioctl_handler]UE MAC RECV ULGRANT\n");
			SV(UL_resource_info.m_tbsize) = DoReceiveULgrant_Tbsize(tempulgrant->ulgrant);
			fsm_printf("[MAC][ioctrl_handler]SV(UL_resource_info.m_tbsize):%d\n",SV(UL_resource_info.m_tbsize));
			//SV(UL_resource_info.m_tbsize) = DoReceiveULgrant_Tbsize(*((int *)tempdata_ptr)); //具体处理待确定UL_grant的数据结构之后再定 
			//根据协议理解 只要收到ul_grant 则会重启retxBSR-Timer，不需要判断是否是针对新数据传输的。协议 36321-80c 5.4.5节
			SV(UL_resource_info.resource_flag) = true;
			fsm_data_destroy(tempdata_ptr);
			if(SV(RetxBSRTimer.flag) == true && SV(RetxBSRTimer.timer_sign) != NULL)
				fsm_schedule_cancel(SV(RetxBSRTimer.timer_sign));
			SV(RetxBSRTimer.time_value)	= SV(systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.retxBSR_Timer);
			SV(RetxBSRTimer.timer_sign) = fsm_schedule_self(SV(RetxBSRTimer.time_value), RexBSRTimer_Expire);
			SV(RetxBSRTimer.flag)= true;
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
		default:
			//fsm_printf("[LHL]DEFAULT CMD VALUE:%d\n",cmd_value); 
		break;
/*还有其他配置命令*/
	}	
	FOUT;
}

static void test_print_pkt_num()
{
	FIN(test_print_pkt_num());
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
  	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
  SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
		fsm_schedule_self(0, RA_Send_Msg3);//LHL 20140726 ,非第一次发MSG3，由MAC触发
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
static void send_msg3() //defined by HQ 20140430  
{
	//针对msg3的不同内容，竞争解决判定的方法有所不同
	//目前实现了: msg3的内容是CCCH上的SDU---->其他的也实现了
	int	skb_len=0,len=0; 
	FSM_PKT *skb;
	FIN(send_msg3());

	SV_PTR_GET(mac_sv);
	//备份Msg3,即CCCH上来自RLC的SDU  //需要在RLC下发SDU后才可以进行该操作
	//组装MAC PDU 
	//fsm_printf("[UEMAC][send_msg3]send msg3,SENDMSG3->CRA \n");
	skb = fsm_pkt_get();
	// fsm_pkt_destroy(fsm_pkt_get());
	// fsm_printf("in fact it was destroyed ");
	fsm_printf("[UE MAC]RA ALLOCATION RESULT:%d\n",SV(ra_allocation_res));
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);	
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
	
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	if(MAC_PK_FROM_UPPER )
	{//在IDLE状态下应该是不会遇到这个情况,所以打印一条错误信息
		print_tran_info("MAC:IDLE receive a data packet from RLC, error, and then start CRA");//testing code 
		SV(ratype.ra_type)=2;
		fsm_schedule_self(0, Start_ContentionBase_RandomAcc);
		fsm_pkt_destroy(fsm_pkt_get());//testing code
	}
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

	SV_PTR_GET(mac_sv);
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
	if(MAC_PK_FROM_UPPER )
	{//在IDLE状态下应该是不会遇到这个情况,所以打印一条错误信息
		print_tran_info("IDLE receive a data packet from RLC, error, and then start CRA");//testing code 
		fsm_schedule_self(0, Start_ContentionBase_RandomAcc);
		fsm_pkt_destroy(fsm_pkt_get());//testing code
	}
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




