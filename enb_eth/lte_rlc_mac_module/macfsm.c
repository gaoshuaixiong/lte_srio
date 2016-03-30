#include <linux/if_ether.h>
#include <linux/list.h>


#include "macfsm.h"
#include "../lte_system.h"


//#include "queue_manager.h"
//#include "mac_schedule.h"

#define ST_INIT 0
#define	ST_IDLE 1
#define ST_RASCHED	2
#define ST_DLSCHED	3
#define ST_ULSCHED	4


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
static void handle_rlcdata_req(void);//处理RLC数据发送请求
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




void mac_main(void)
{
FSM_ENTER(mac_main());
FSM_BLOCK_SWITCH
	{
	FSM_STATE_FORCED(ST_INIT, "INIT", mac_init_enter(), )		
		{
		FSM_TRANSIT_FORCE(ST_IDLE,print_tran_info("INIT -> IDLE") , "default", "", "INIT -> IDLE");
		}		
			
	FSM_STATE_UNFORCED(ST_IDLE, "IDLE",,)		
		{
		FSM_COND_TEST_IN("IDLE")				
			FSM_TEST_COND(MAC_RESET)				
			FSM_TEST_COND(RA_REQ)
			FSM_TEST_COND(DL_SCHED_REQ)
			FSM_TEST_COND(UL_SCHED)
			
		FSM_COND_TEST_OUT("IDLE")	
		FSM_TRANSIT_SWITCH			
			{	
			FSM_CASE_TRANSIT(0, ST_INIT,    print_tran_info("IDLE -> INIT"), "IDLE -> INIT")				
			FSM_CASE_TRANSIT(1, ST_RASCHED,handle_ra_req(), "IDLE -> RASCHED")
			FSM_CASE_TRANSIT(2, ST_DLSCHED,handle_rlcdata_req(), "IDLE -> DLSCHED")		
			FSM_CASE_TRANSIT(3, ST_ULSCHED,ul_schedule(), "IDLE -> ULSCHED")	
			FSM_CASE_DEFAULT(ST_IDLE,idle_default_handler() , "IDLE->IDLE")	//transit to idle state	by default.
			}
		}
	FSM_STATE_FORCED(ST_RASCHED, "RASCHED",, )
		{
		FSM_TRANSIT_FORCE(ST_IDLE,print_tran_info("RASCHED -> IDLE"), "default", "", "RASCHED -> IDLE");
		}
	FSM_STATE_FORCED(ST_ULSCHED, "ULSCHED", , )
		{
		FSM_TRANSIT_FORCE(ST_IDLE,, "default", "", "ULSCHED -> IDLE");//testing code, "default", "", "ULSCHED -> IDLE");
		}
	FSM_STATE_FORCED(ST_DLSCHED, "DLSCHED", , )
		{
		FSM_TRANSIT_FORCE(ST_IDLE,dl_schedule(), "default", "", "DLSCHED -> IDLE");
		}	
	}
FSM_EXIT(0)
}

static void mac_init_enter(void)
{
	FIN(mac_init_enter()());
	//SV_PTR_GET(mac_sv);
	//print_tran_info("FSM-into-INIT");
	mac_sv_init();
	defaultconfig();//RRC default 20141124LHL 
	//test code as follows: 
	//Init_LogicalChannel_ConfigInfo(rrc_lc_config(61, 1));//testing code  //配置逻辑信道 
	Init_LogicalChannel_ConfigInfo(rrc_lc_config(61, 0));//testing code  //配置逻辑信道 
	//Init_LogicalChannel_ConfigInfo(rrc_lc_config(62, 1));//testing code  //配置逻辑信道 
	Init_LogicalChannel_ConfigInfo(rrc_lc_config(62, 0));//testing code  //配置逻辑信道 
	//fsm_printf("MAC logical channel config \n");//testing code  //配置逻辑信道 
	
	FOUT;
}
static void macmain_defaultconfig()
{
	FIN(macmain_defaultconfig());
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	//fsm_printf("[UEMAC][init]CCCH_defaultconfig completed\n");
	FOUT;
}
static void mac_sv_init(void)
{
	int i;
	int index;
	FIN(mac_sv_init());
	SV_PTR_GET(mac_sv);
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
	fsm_schedule_self(1000000,TIME_TODO_ULSCHED);//上行调度周期本应该是10ms 为了测试方便这里设置为10s
	//test
	SV(lost_pktnum_uplink)=0;
	SV(total_pktnum_uplink)=0;
	SV(lost_pktnum_downlink)=0;
	SV(total_pktnum_downlink)=0;
	//test end
	
	
FOUT;
}

static void mac_close(void)
{
	FIN(mac_close());
	SV_PTR_GET(mac_sv);
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



static void idle_default_handler()
{ 
//处理与状态转换无关的事件:下层数据包到达、与状态无关的ioctrl命令
int i;
FSM_PKT *skb_lower;
FSM_PKT *skb_upper;
	FIN(idle_default_handler());
	SV_PTR_GET(mac_sv);

	if(MAC_CLOSE)
	{
		mac_close();
    	fsm_printf("MAC FSM closed \n");//testing code		
	}
	//GengCanxi新增2014、7、23
	if(TC_RNTI_CHECK)
	{
		delete_overtime_tc_rnti();
		//test 
    	//fsm_printf("delete the overtime TC-RNTI(such as, when UE hasn't receive the msg2) \n");//testing code		
	}
	
    if(MAC_PK_FROM_LOWER)//经过讨论PHY将多个用户的包拆开为一个一个用户再给MAC
    {//通过ICI信息判断包类型，是哪个用户的，解复用通过相应地LC发给RLC 
        //参考流程图 HQ 注释
    	//fsm_printf("handle the data packet from lower layer\n");//testing code
    	skb_lower=fsm_pkt_get();
    	decomplexEndMacPdu(skb_lower);
    	//fsm_pkt_destroy(skb_lower);
    }
    if(MAC_PK_FROM_UPPER)
    {//通过ICI信息判断包类型 复用 通过相应的TC发给物理适配层 
        //参考流程图 HQ 注释
        //fsm_printf("handle the data packet from upper layer\n");//testing code
        skb_upper=fsm_pkt_get();//每次处理一个用户的数据
    	complexEnbMacPdu(skb_upper);
    	//fsm_pkt_destroy(skb_upper);
        
    }
    if(IOCTRL_ARRIVAL)
    { /*与状态无关的ioctrl命令*/  
        ioctrl_handler();
    }
FOUT;
}  

static void ioctrl_handler(void)
{ 
	void *tmpdata_ptr;
	int m_rnti;
	FIN(ioctl_handler());
	SV_PTR_GET(mac_sv);
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
		default:
		break;
	}
	
FOUT;
}
static void test_print_pkt_num()
{
	FIN(test_print_pkt_num());
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
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
	SV_PTR_GET(mac_sv);
	SV(phy_to_mac_frame).sfn=frame_info->sfn;
	SV(phy_to_mac_frame).subframeN=frame_info->subframeN;
}
/************************************************************************
*author:HQ
*description:完成下行调度工作，响应RLC的数据发送请求，生成相应的DCI，
*version1.0-201406017
************************************************************************/
static void handle_rlcdata_req()//处理RLC数据发送请求
{
	char *tempdata_ptr;
	RLC_Request *rlc_req;
	RLC_Request *rlc_req_temptr;	
	int rlc_req_lc_totalnum;
	int i,ue_num;
	unsigned short tmp_rnti;

	void *datareport;//20141231 for test
	RLC_Report *testtmp=NULL;//for test 20141231
	int k=0;//for test 20141231
	FIN(handle_rlcdata_req());
	
	SV_PTR_GET(mac_sv);
	tempdata_ptr = (char *)fsm_data_get();//接收命令所携带的数据
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
	fsm_data_destroy((void *) tempdata_ptr);
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

	SV_PTR_GET(mac_sv);
	Reset_Prb_dlBitmap();//重置BITMAP
	
	rlc_report_num = (u32 *)fsm_mem_alloc(sizeof(u32));
	for(i=0; i<USERNUMMAX && SV(Rlc_Request_Num[i]); i++)//根据待发数据缓存情况先计算一个有待发数据的LC的总数
		rlc_rep_num_total += SV(Rlc_Request_Num[i]);
	
	ue_num=i;//当前TTI需要调度的UE数LHL 20150110
	
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
		//fsm_printf("[ENB MAC]rlc request rnti:%d\n",rnti);

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
		
		//fsm_printf("ENB MAC has already alloced resources\n");
	
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

		//fsm_printf("[LHL]REQUEST NUMBER:%d\n",rlc_rep_num_total);
		fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_MACtoRLC_datasend_Allow, rlc_report_ptr, rlc_rep_num_total * sizeof(RLC_Report) + sizeof(u32));//for test
		fsm_mem_free((void *)rlc_report_ptr);
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

	struct S_RAinfo ra_UEinfo;
	struct S_RAinfo ra_UEinfo_temp;
	struct S_UEinfo ul_UEinfo;
	DATA_RAR_PDU * rar_info_ptr;//调度后生成的RAR信息
	SV_PTR_GET(mac_sv);
	int resleft;//剩余资源
	void *ue_ra_req;
	int *temp;
	int ue_index;
	FIN(ul_schedule());
	//print_tran_info("IDLE -> ULSCHED");

	//更新资源表 
	SV(UL_resource_flag) = 1;//设置资源为可用状态
	if(SV(RA_sched_flag)) //先检查RA队列，因为RA所需资源较少 
	{
		//fsm_printf("[ENB MAC] SV(RA_sched_flag))\n");
		//为分配RA队列中的用户分配资源 直到队列为空或者上行资源分配完
		while(SV(UL_resource_flag) > 0  && !(list_empty_careful(SV(RAschedQue_p))) )
		{//资源可用 且 RA队列不为空
		//以下是将相同RA_RNTI的用户从链表中取出，形成num+struct S_RAinfo形式，传入complexRarPdu()函数中
			ra_UEinfo = take_first_RAschedQue(SV(RAschedQue_p));//从队列头部取出一个用户
			ue_ra_req=fsm_mem_alloc(sizeof(S_RAinfo)*(ra_UEinfo.total_num)+sizeof(int));//LHL midified 20141011
			temp=(int *)ue_ra_req;
			*temp=ra_UEinfo.total_num;
			fsm_mem_cpy((void *)((int *)ue_ra_req+1),&ra_UEinfo, sizeof(S_RAinfo));
			for(ue_index=1;ue_index<ra_UEinfo.total_num;ue_index++)
			{
				ra_UEinfo_temp=take_first_RAschedQue(SV(RAschedQue_p));//取出随机接入请求
				if(ra_UEinfo_temp.total_num == ra_UEinfo.total_num && ra_UEinfo_temp.index == ue_index)//保险起见
				{
					fsm_mem_cpy(((S_RAinfo *)((int *)ue_ra_req+1))+ue_index,&ra_UEinfo_temp, sizeof(S_RAinfo));
				}
			}//LHL midified 20141011
			//响应接入请求
			//生成RAR PDU
			
			//调用下行资源分配
			//发送下行DCI
			//int Do_RAR_DCI1C(struct S_RAinfo *ra_UEinfo):::::20141024GCX
			//调用何玺函数生成RAR	
			/*************分割线*******************************/
			if(SV(send_rar_flag)!=0)//20150104联调测试错误机制，不发RAR，以后不需要if判断
			{
				complexRarPdu(ue_ra_req);
				//以SK_BUF的方式发给物理适配层
				//生成相应的DCI
				resleft=Do_RAR_DCI0( &ra_UEinfo);//耿灿锡函数，modified by lhl20140715
				
				if(resleft==0)//没有资源
					SV(UL_resource_flag)=0;//更新资源标志
				fsm_mem_free(ue_ra_req);
			}
			/**********分割线******************************************/
			//暂时注释掉，使用上面的if ，以后要使用下面的 不需要if判断，if只是为了测试错误机制
			/*8
			complexRarPdu(ue_ra_req);
			//以SK_BUF的方式发给物理适配层
			//生成相应的DCI
			resleft=Do_RAR_DCI0( &ra_UEinfo);//耿灿锡函数，modified by lhl20140715
			
			if(resleft==0)//没有资源
				SV(UL_resource_flag)=0;//更新资源标志
			fsm_mem_free(ue_ra_req);
			*/
			
			//以IOCTRL的方式发送给物理适配层,Do_RAR_DCI函数已发送	
		}
		SV(RA_sched_flag)= !(list_empty_careful(SV(RAschedQue_p)));//更新队列标志
	}
	
	if(!(list_empty_careful(SV(ULschedQue_p))))
		SV(UL_sched_flag)=1;	
	//fsm_printf("[ENB MAC] SV(UL_sched_flag):%d\n",SV(UL_sched_flag));
	if(SV(UL_sched_flag))
	{//再进行上行调度 从队列中取出一个用户 直到队列为空或者上行资源被分配完
		//fsm_printf("[ENB MAC] SV(ULschedflag)\n");
		while(SV(UL_resource_flag) > 0  && !(list_empty_careful(SV(ULschedQue_p))) )	
		{
			ul_UEinfo = take_first_ULschedQue(SV(ULschedQue_p));//从队列头部取出一个用户
			//进行资源分配
			fsm_printf("###In function ul_schedule() the &ul_UEinfo's RNTI:%d",ul_UEinfo.rnti);
	
			resleft=Order_Ul_Sched(&ul_UEinfo);//上行Ulgrant ,以IOCTL发给物理适配层
			if(resleft==0)
				SV(UL_resource_flag)=0;//没有资源
			//生成相应的DCI
			//以IOCTRL的方式发送给物理适配层	
			//Do_Ul_SchedInd(&ul_UEinfo);
		}
		SV(UL_sched_flag) = !(list_empty_careful(SV(ULschedQue_p)));//更新队列标志

	}
	//清空RA队列 因为UE等待RAR的时间受到限制，未能响应的RA请求最多只保留10ms,即一个上行调度周期
    clean_RAschedQue(SV(RAschedQue_p));
	SV(RA_sched_flag) = 0;//设置RA队列标志为"不需要调度"

	fsm_schedule_self(1000000,TIME_TODO_ULSCHED);//开启定时器  //上行调度周期本应该是10ms 为了测试方便这里设置为10s
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
	SV_PTR_GET(mac_sv);
	tmpdata_ptr = fsm_data_get();//获得该用户的相应信息
	//print_tran_info("IDLE -> RASCHED");
	//fsm_printf("receive RA from PHY  ");//testing code
	if(SV(UL_resource_flag) == 0) //更准确的实现，是要更精确的比较，剩余资源是否小于RA请求所需要的资源
	{	
		//上行资源目前没有可被分配的资源
		//将该用户加入到RA调度队列中,等待有资源时再被调度
		usernum=(int *)tmpdata_ptr;
		for(i=i;i<usernum;i++)
		{			
			insert_RAschedQue(SV(RAschedQue_p), (struct S_RAinfo *)(((int *)tmpdata_ptr+1)+i));
		}
		SV(RA_sched_flag) = 1;//设置RA队列标志为"需要调度"
		
		//fsm_printf("UE RA-RNTI:%d ,RAPID:%d \n",((struct S_RAinfo *)((int *)tmpdata_ptr+1))->ra_rnti,((struct S_RAinfo *)((int *)tmpdata_ptr+1))->rapid);//testing code
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
	SV_PTR_GET(mac_sv);
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

	
	
	
	




