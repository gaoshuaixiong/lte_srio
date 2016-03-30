/****************************************************************
** Filename:Sriofsm.c
** Copyright:uestc
** Descriptions:
** v1.0, 20140928, MaFang written
** --------------------------------------------------------------
** modification history:
** Modified by:
** Modified date:
** Descriptions:
**
****************************************************************/


#include <linux/if_ether.h>
#include "sriofsm.h"
#include "../lte_system.h"
#include "pkfmt.h"
//#include "rrc_type.h"
//#include "rrc_type_IEs.h"

//	定义状态机中的状态
/**********20141013 mf modified ************************/
#define ST_INIT	0
#define ST_CFG	1
#define	ST_IDLE	2
#define ST_SEND	3
#define ST_RECV	4
//#define ST_TEST	4	//测试阶段添加TEST状态，用于接收MSG3发送MSG4	modified by MF 20140715
/**********end modified ************************/


//	定义物理适配层本身测试用IOControl命令
#define IOCCMD_PSEND_RUN			0x01
#define IOCCMD_PSEND_STOP			0x02
#define IOCCMD_PSEND_INTERVAL	0x05 
#define IOCCMD_SAY_HELLO			0x04

//	定义UE侧MAC层对物理适配层的IOControl命令
#define IOCCMD_MACtoPHY_RNTI_Indicate			0x24      //发送MSG3之前 MAC层告知适配层RNTI值
#define IOCCMD_MACtoPHY_Preamble_Indicate		0x25      //MSG1
#define IOCCMD_MACtoPHY_recv_sysinfo			0x26      //RRC告知MAC MAC告知适配层开始接收系统消息
#define IOCCMD_MACtoPHY_stop_recv_sysinfo		0x27      //RRC告知MAC MAC告知适配层停止接收系统消息
#define IOCCMD_MACtoPHY_recv_paging				0x46      //RRC告知MAC MAC告知适配层开始接收寻呼消息
/************20141013 mf modified**************/
#define IOCCMD_PHYtoMAC_RA_Req				0x03		//With data format S_RAinfo
#define IOCCMD_PHYtoMAC_TA					0x06
#define IOCCMD_PHYtoMAC_FRAME				0x07


/************end modify************************/
//	定义UE侧物理适配层对MAC层的IOControl命令
#define IOCCMD_Harq_feedback					0x18   //when MAC received this command from PHYadapter,MAC 
#define IOCCMD_PDCCHtoMAC_RandomAcc_Req		0x0C   //PDCCH上 告知MAC开始随机接入
#define IOCCMD_PDCCHtoMAC_ULGRANT			0x0D   //PHY send a DCI of ul_grant to MAC 
#define IOCCMD_TEST_SEND_MSG3				0x51  //收到IOControl后，发送MSG3，测试命令 modified by MF 20140715
/*******20141017 mf modified for test******/
#define IOCCMD_TEST_SEND_TO_ETH				0x52

/***********20141013 mf modified RRC ioctl****************/
#define IOCCMD_RRCtoPHY_Type1_Indicate      0x60

/**********20141029 MACSRIO联调测试 马芳 modified***********************/
#define IOCCMD_SEND_MSG1						0x70
#define IOCCMD_SEND_ULDATA					0x71
#define IOCCMD_SEND_TA						0x72
#define IOCCMD_DLSCHEDULE						0x13
#define IOCCMD_RARDCI							0x14
#define IOCCMD_ULSCHEDULE						0x12
#define IOCCMD_SEND_MSG3						0x76
#define IOCCMD_SEND_SF						0x77
/****************************************modified end ******************/


/************end modify************************/

#define MemoryStart				0xfc10000
#define MemorySize					10240

//	状态机功能函数声明
static void init_enter(void);
static void send_packet_period(void);
static void srio_sv_init(void);	
static void srio_close(void);
/********mf modified 20141029 for MACSRIO test ******/
static void packet_send_to_eth(void);
//static void packet_send_to_eth(FSM_PKT* pkptr);
static void packet_send_to_upperlayer(void);
static void idle_exit(void);
static void idle_ioctl_handler(void);
void print_tran_info( const char *str);
/**********20141013 mf modified****************************/
static void cfg_ioctl_handler(void);
static void send_type1(void);




//	测试函数声明

static void ioctldata();

/********mf modified 20141017 for test******/
static void test_send_to_eth(void);
/**********20141029 MACSRIO联调测试 马芳 modified***********************/
static void test_send_msg1(void);
static void test_send_ta(void);
static void test_send_msg3(void);
static void test_send_sf(void);
static void test_send_uldata(void);
/****************************************modified end ******************/

/******************20141030 MACSRIOTEST HX******************************/
u32 createLongBsr(FSM_PKT *skb,u32 offset);
u32 createCRnti(FSM_PKT *skb,u32 offset);
u32 createHead(FSM_PKT *skb,u32 control_numb,u32 data_numb,char typ);
static void createmachead7bit(MAC_SDU_subhead_7bit *macsubhead,u8 lcid,u8 sdu_len,u8 continueflag);
static void msgFromCCCH(FSM_PKT* skb);
static void skbFromRlc(FSM_PKT* pkptr);
FSM_PKT* generatepkt(u16 rnti);

static void send_uldci_to_ue(void);

int export_frameNo;
int export_subframeNo;
EXPORT_SYMBOL(export_frameNo);
EXPORT_SYMBOL(export_subframeNo);


/*****************************end***************************************/

/*
void srio_main(void)
{
FSM_ENTER(srio_main);
FSM_BLOCK_SWITCH
	{
	FSM_STATE_FORCED(ST_INIT, "INIT", srio_sv_init(), )
		{
		 FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "INIT -> IDLE"); //脟驴脰脝脌脿脨脥脳陋禄禄
		}
	FSM_STATE_UNFORCED(ST_IDLE, "IDLE",,)		//脠毛驴脷潞炉脢媒 鲁枚驴脷潞炉脢媒
		{
		FSM_COND_TEST_IN("IDLE")				
			FSM_TEST_COND(SRIO_PK_FROM_LOWER)				
			FSM_TEST_COND(SRIO_PK_FROM_UPPER)
			FSM_TEST_COND(SRIO_CLOSE)
			FSM_TEST_COND(PACKET_SEND_PERIOD)
			FSM_TEST_COND(MSG3_FROM_UPPER) //20140715 mf testconmand 沤媒露拧
		FSM_COND_TEST_OUT("IDLE")	
		FSM_TRANSIT_SWITCH			
			{	
			FSM_CASE_TRANSIT(0, ST_RECV, , "IDLE -> RECV")	//艙脫脢脺脧脗虏茫脨脜脧垄			
			FSM_CASE_TRANSIT(1, ST_SEND, , "IDLE -> SEND") //路垄脣脥脡脧虏茫脨脜脧垄
			FSM_CASE_TRANSIT(2, ST_INIT,idle_exit() , "IDLE -> INIT") //脳沤脤卢禄煤脥脣鲁枚
			FSM_CASE_TRANSIT(3, ST_IDLE,send_packet_period(), "IDLE->IDLE")//露拧脢卤脝梅脰脺脝脷路垄脣脥
			FSM_CASE_TRANSIT(4, ST_TEST, , "IDLE->TEST")//20140715 mf 
			FSM_CASE_DEFAULT(ST_IDLE,ioctl_handler(), "IDLE->IDLE")	//iocontrol
			}	
		}
	FSM_STATE_FORCED(ST_RECV, "RECV", packet_send_to_upperlayer(), )
		{
		FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "RECV -> IDLE");
		}
	FSM_STATE_FORCED(ST_SEND, "SEND", packet_send_to_eth(), )
		{
		FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "SEND -> IDLE");
		}
	FSM_STATE_FORCED(ST_TEST, "TEST", send_msg4(), )//20140715 mf 路垄脣脥msg4
		{
		FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "TEST -> IDLE");
		}
	}
FSM_EXIT(0)
}*/


/********************************************************************************
** Function name: srio_main
** Description: 物理适配层的状态机主函数
** Input:
** Output:
** Returns:
** Created by:
** Created Date:
** ------------------------------------------------------------------------------
** Modified by: MF
** Modified Date: 20141013
** Modefied Description: 添加CFG态用于等待RRC的Type1配置
********************************************************************************/

void srio_main(void)
{
	FSM_ENTER(srio_main);
	FSM_BLOCK_SWITCH
	{
		FSM_STATE_FORCED(ST_INIT, "INIT", srio_sv_init(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "INIT -> IDLE"); 
		}
		/********************8ENBUE TEST******************************
		FSM_STATE_UNFORCED(ST_CFG, "CFG",,)		
		{
			FSM_COND_TEST_IN("CFG")				
				FSM_TEST_COND(CFG_FROM_RRC)
			FSM_COND_TEST_OUT("CFG")	
			FSM_TRANSIT_SWITCH			
			{	
				FSM_CASE_TRANSIT(0, ST_IDLE, send_type1(), "CFG -> IDLE")		
				FSM_CASE_DEFAULT(ST_CFG,cfg_ioctl_handler(), "IDLE->IDLE")	//iocontrol
			}	
		}
		*******************8ENBUE TEST END****************************/
		FSM_STATE_UNFORCED(ST_IDLE, "IDLE",,)		
		{
			FSM_COND_TEST_IN("IDLE")				
				FSM_TEST_COND(SRIO_PK_FROM_LOWER)				
				FSM_TEST_COND(SRIO_PK_FROM_UPPER)
				FSM_TEST_COND(SRIO_CLOSE)
				FSM_TEST_COND(PACKET_SEND_PERIOD)
				FSM_TEST_COND(TEST_SEND_SF_PERIOD)
				//FSM_TEST_COND(MSG3_FROM_UPPER) //20140715 mf testconmand 沤媒露拧
			FSM_COND_TEST_OUT("IDLE")	
			FSM_TRANSIT_SWITCH			
			{	
				FSM_CASE_TRANSIT(0, ST_RECV, , "IDLE -> RECV")	//艙脫脢脺脧脗虏茫脨脜脧垄			
				FSM_CASE_TRANSIT(1, ST_SEND, , "IDLE -> SEND") //路垄脣脥脡脧虏茫脨脜脧垄
				FSM_CASE_TRANSIT(2, ST_INIT,idle_exit() , "IDLE -> INIT") //脳沤脤卢禄煤脥脣鲁枚
				FSM_CASE_TRANSIT(3, ST_IDLE,send_packet_period(), "IDLE->IDLE")//露拧脢卤脝梅脰脺脝脷路垄脣脥
				FSM_CASE_TRANSIT(4, ST_IDLE, test_send_sf(), "IDLE->IDLE")//20140715 mf 
				FSM_CASE_DEFAULT(ST_IDLE,idle_ioctl_handler(), "IDLE->IDLE")	//iocontrol
			}	
		}
		FSM_STATE_FORCED(ST_RECV, "RECV", packet_send_to_upperlayer(), )
		//FSM_STATE_FORCED(ST_RECV, "RECV",  , )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "RECV -> IDLE");
		}

		FSM_STATE_FORCED(ST_SEND, "SEND", packet_send_to_eth(), )   //20141029 for test
		//FSM_STATE_FORCED(ST_SEND, "SEND", print_tran_info("[MF]STATE:SEND\n"), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "SEND -> IDLE");
		}
		/********************8ENBUE TEST******************************
		FSM_STATE_UNFORCED(ST_TEST, "TEST", ioctldata(), )//20140715 mf 路垄脣脥msg4
		{
			//FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "TEST -> IDLE");
			FSM_COND_TEST_IN("TEST")						
				FSM_TEST_COND(SRIO_PK_FROM_UPPER)
			FSM_COND_TEST_OUT("TEST")	
			FSM_TRANSIT_SWITCH			
			{	
				FSM_CASE_TRANSIT(0, ST_IDLE, test_send_msg4(), "TEST -> IDLE")	//艙脫脢脺脧脗虏茫脨脜脧垄			
				FSM_CASE_DEFAULT(ST_TEST,ioctl_handler(), "TEST -> TEST")	//iocontrol
			}
		}
		*******************8ENBUE TEST END****************************/
	}
	FSM_EXIT(0)
}

/********************************************************************************
** Function name: init_enter
** Description: 状态机初始化函数
** Input:
** Output:
** Returns:
** Created by:
** Created Date:
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/
static void init_enter(void)
{
	FIN(init_enter());
	if(SRIO_OPEN)
	{
//		fsm_schedule_self(0, _START_WORK);
	}
	FOUT;
}

/********************************************************************************
** Function name: srio_sv_init
** Description: 状态机全局变量初始化函数
** Input:
** Output:
** Returns:
** Created by:
** Created Date:
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/
static void srio_sv_init(void)
{
	FIN(srio_sv_init());//
	u32 i;
	//PHY_TO_MAC_frame *Sf_info = (PHY_TO_MAC_frame *)fsm_mem_alloc(sizeof(PHY_TO_MAC_frame));
	SV_PTR_GET(srio_sv);//

	SV(packet_count) = 0;//
	SV(interval) = 100000;//
	SV(psend_handle) = 0;//
	/*********20141013 mf modified*************/
	//SV(type1_cnt) = 0;
	SV(sfn) = 0;
	SV(subframeN) = 0;
	//Sf_info->sfn = SV(sfn);
	//Sf_info->subframeN = SV(subframeN);
	SV(SN)=0;
	for (i = 0; i < USER_NUM; i ++)
	{
		SV(Dci_Store.rnti[i]) = 0;
		//SV(Dci_Store.UL_DCI[i]) = NULL;
		//SV(Dci_Store.DL_DCI[i]) = NULL;
		//SV(Dci_Store.RAR_DCI[i]) = NULL;
	}
	test_send_sf(); 	//add in 20151230
	//fsm_do_ioctrl(STRM_TO_RLCMAC,IOCCMD_PHYtoMAC_FRAME,(void *)Sf_info,sizeof(Sf_info));

	FOUT;//
}

/********************************************************************************
** Function name: srio_close
** Description: 状态机关闭函数
** Input:
** Output:
** Returns:
** Created by:
** Created Date:
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/
static void srio_close(void)
{
	FIN(srio_close());
	SV_PTR_GET(srio_sv);
	fsm_printf("srio has sent %d packets.\n", SV(packet_count));
	if(SV(psend_handle))
	{
		fsm_schedule_cancel(SV(psend_handle));
		SV(psend_handle) = 0;
	}
	FOUT;
}
/**************脙禄脫脨ici碌脛脰卤艙脫脥啪沤芦**********/
/*static void packet_send_to_eth(void)
{
FSM_PKT* pkptr;
struct lte_test_srio_head* sh_ptr;
struct ethhdr* head_ptr;
char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
FIN(packet_send_to_eth());
SV_PTR_GET(srio_sv);
pkptr = fsm_pkt_get();

if(pkptr != NULL)
{
	if(fsm_skb_headroom(pkptr) < (ETH_HLEN + sizeof(struct lte_test_srio_head)))
		{
		pkptr = fsm_skb_realloc_headeroom(pkptr,ETH_HLEN + sizeof(struct lte_test_srio_head));
		if(pkptr == NULL)
			return;
		}
	fsm_skb_push(pkptr, sizeof(struct lte_test_srio_head));
	sh_ptr = (struct lte_test_srio_head*)pkptr->data;
	sh_ptr->type = fsm_htonl(0);
	sh_ptr->len = fsm_htonl(pkptr->len-sizeof(struct lte_test_srio_head));
	fsm_skb_push(pkptr, ETH_HLEN);
	head_ptr = (struct ethhdr*)pkptr->data;
	fsm_mem_cpy(head_ptr->h_dest, dst_addr, ETH_ALEN);
	fsm_mem_cpy(head_ptr->h_source, fsm_intf_addr_get(STRM_TO_ETH), ETH_ALEN);
	head_ptr->h_proto = fsm_htons(DEV_PROTO_SRIO);	
//	fsm_octets_print(&pkptr->protocol, 2);
	fsm_pkt_send(pkptr,STRM_TO_ETH);
	SV(packet_count)++;
}
FOUT;
}
******/
/*************脠楼ICI HEAD *****************/
/*****2014/7/17*************/

/********************************************************************************
** Function name: packet_send_to_eth
** Description: 收到上层发的数据包，下发到以太网(测试用)
** Input:
** Output:
** Returns:
** Created by:
** Created Date:
** ------------------------------------------------------------------------------
** Modified by: MF
** Modified Date: 20140717
** Modified Description: 直接销毁包 不发送出去 测试用
********************************************************************************/
/***** mf modified 20141029 for MACSRIO test**************/
static void packet_send_to_eth(void)
//static void packet_send_to_eth(FSM_PKT* pkptr)
{
	FSM_PKT* pkptr;
	FSM_PKT* pkptrcopy;
	struct lte_test_srio_head* sh_ptr;
	struct ethhdr* head_ptr;
	DCI_STORE Dci_Store;
	Data_Dl_DCI Dl_Dci;
	//MACtoPHYadapter_IciMsg* MactoPhyICI=(MACtoPHYadapter_IciMsg*)fsm_mem_alloc(sizeof(MACtoPHYadapter_IciMsg));
	u16 rnti;
	u16 i;
	
	u32 flag = 0;
	SV_PTR_GET(srio_sv);
	Dci_Store = SV(Dci_Store);
	char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	FIN(packet_send_to_eth());
	pkptr = fsm_pkt_get();
	
	//for test
	fsm_skb_push(pkptr,4);
	fsm_mem_cpy(pkptr->data,&SV(SN),4);
	//printk("[SRIO][packet_send_to_eth] SN=%d\n",SV(SN));
	SV(SN)++;
	
	MACtoPHYadapter_IciMsg* MactoPhyICI=(MACtoPHYadapter_IciMsg*)fsm_mem_alloc(sizeof(MACtoPHYadapter_IciMsg));

	fsm_mem_cpy(MactoPhyICI, (MACtoPHYadapter_IciMsg *)(pkptr->head), sizeof(MACtoPHYadapter_IciMsg));
	//fsm_printf("[DAFANGZI]The RNTI of the packet is %d\n",MactoPhyICI->rnti );
	//pkptr=fsm_create(128);
	//pkptrcopy = fsm_pkt_create(5120);
	pkptrcopy = fsm_pkt_create(pkptr->tail-pkptr->data+64);
	fsm_skb_put(pkptrcopy, (pkptr->tail-pkptr->data));
	fsm_mem_cpy(pkptrcopy->data,pkptr->data,(pkptr->tail-pkptr->data));
	fsm_pkt_destroy(pkptr);	
	if(pkptrcopy != NULL)
	{
		if(fsm_skb_headroom(pkptrcopy) < (ETH_HLEN+sizeof(struct lte_test_srio_head)))
			{
				pkptrcopy = fsm_skb_realloc_headeroom(pkptrcopy,(ETH_HLEN+sizeof(struct lte_test_srio_head)));
				if(pkptrcopy == NULL)
					return;
			}
		fsm_skb_push(pkptrcopy, sizeof(struct lte_test_srio_head));
		sh_ptr = (struct lte_test_srio_head*)pkptrcopy->data;
		sh_ptr->type = fsm_htonl(2);
		sh_ptr->len = fsm_htonl(pkptrcopy->len-sizeof(struct lte_test_srio_head));
		sh_ptr->rnti = fsm_htons(MactoPhyICI->rnti);
		sh_ptr->sfn = fsm_htons(SV(sfn));
		sh_ptr->subframeN = fsm_htons(SV(subframeN));
		printk("[ENB SRIO]The rnti of the packet is %u, sending to ETH\n", sh_ptr->rnti);
		fsm_skb_push(pkptrcopy, ETH_HLEN);
		head_ptr = (struct ethhdr*)pkptrcopy->data;
		fsm_mem_cpy(head_ptr->h_dest, dst_addr, ETH_ALEN);
		fsm_mem_cpy(head_ptr->h_source, fsm_intf_addr_get(STRM_TO_ETH), ETH_ALEN);
		head_ptr->h_proto = fsm_htons(DEV_PROTO_SRIO);	
		fsm_printf("[srio]:pkptrcopy->data len = %d:\n", pkptrcopy->len);
		fsm_octets_print(pkptrcopy->data, pkptrcopy->len);
		fsm_pkt_send(pkptrcopy,STRM_TO_ETH);//for test
		SV(packet_count)++;
	}
	FOUT;
}
/*
static void srio_packet_send_to_upperlayer(void)
{
FSM_PKT* pkptr;
struct lte_test_srio_head* sh_ptr;
FIN(packet_send_to_upperlayer());
pkptr = fsm_pkt_get();
sh_ptr = (struct lte_test_srio_head*)pkptr->data;
if(fsm_ntohl(sh_ptr->type) == 0)
	{
	fsm_skb_pull(pkptr, sizeof(struct lte_test_srio_head));
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
	}
else if(fsm_ntohl(sh_ptr->type) == 1)
	{
	fsm_skb_pull(pkptr, sizeof(struct lte_test_srio_head));
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_destroy(pkptr);
	}
else 
	fsm_pkt_destroy(pkptr);
FOUT;
}*/

/******ici dispose *********2014/7/17*/

/********************************************************************************
** Function name: packet_send_to_upperlayer
** Description: 收到下层发的数据包，向上发送到MAC层(测试用)
** Input:
** Output:
** Returns:
** Created by:
** Created Date:
** ------------------------------------------------------------------------------
** Modified by: MF
** Modified Date: 20140717
** Modified Description: 直接销毁包 不发送出去 测试用
********************************************************************************/
static void packet_send_to_upperlayer(void)
{
	FSM_PKT* pkptr;
	MACtoPHYadapter_IciMsg * ici_to_phyadapter;
	PHYadaptertoMAC_IciMsg * ici_to_mac;
	MSG1_Content * Msg1_recv;
	S_RAinfo * Msg1_req;
	void* Msg1;
	u32 num = 1;
	u32 sn;
	//u32 *MessageType;
	struct lte_test_srio_head* sh_ptr;
	FIN(packet_send_to_upperlayer());
	
	pkptr = fsm_pkt_get();

	sh_ptr = (struct lte_test_srio_head*)pkptr->data;
	fsm_skb_pull(pkptr, sizeof(struct lte_test_srio_head));
	fsm_printf("[ENB SRIO] ENTER packet_send_to_upperlayer(), and the fsm_ntohl(sh_ptr->type) is %d\n",fsm_ntohl(sh_ptr->type));
	if(fsm_ntohl(sh_ptr->type) == 2)
	{
		ici_to_mac=(PHYadaptertoMAC_IciMsg*)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
		ici_to_mac->tcid=2;
		ici_to_mac->rnti=sh_ptr->rnti;
		ici_to_mac->ue_info.rnti = sh_ptr->rnti;
		ici_to_mac->ue_info.sfn = 2;
		ici_to_mac->ue_info.subframeN = 7;
		ici_to_mac->ue_info.crc = 0;
		ici_to_mac->ue_info.harqindex = 0;
		ici_to_mac->ue_info.harq_result = 0;
		ici_to_mac->ue_info.sr = 0;
		ici_to_mac->ue_info.cqi = 9;
		ici_to_mac->ue_info.pmi = 0;
		ici_to_mac->ue_info.ta = 32;
		//fsm_printf("[ENB SRIO]The rnti of the packet is %u, sending to MAC\n", ici_to_mac->ue_info.rnti);
		fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(PHYadaptertoMAC_IciMsg));

		fsm_printf("[ENB SRIO]pkptr->data len = %d\n",pkptr->len);
		//fsm_octets_print(pkptr->data, pkptr->len);
		
		//for test
		fsm_mem_cpy(&sn,pkptr->data,4);
		fsm_skb_pull(pkptr,4);
		//printk("[SRIO][packet_send_to_upperlayer] SN=%d\n",sn);
		
		fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
	}
	else if(fsm_ntohl(sh_ptr->type) == 1)
	{
		Msg1_recv = (MSG1_Content *)pkptr->data;
		fsm_printf("[ENB SRIO]Received packet cqi = %u,rapid = %u\n", Msg1_recv->cqi,Msg1_recv->rapid);

		Msg1 = fsm_mem_alloc(sizeof(u32) + num * sizeof(S_RAinfo));
		fsm_mem_cpy(Msg1,(void *)(&num),sizeof(u32));

		
		Msg1_req = (S_RAinfo *)fsm_mem_alloc(sizeof(S_RAinfo));

		Msg1_req->total_num = 1;
		Msg1_req->index = 0;
		Msg1_req->ra_rnti = Msg1_recv->rarnti;
		Msg1_req->sfn = 0;
		Msg1_req->subframeN = 2;
		Msg1_req->crc = 0;
		Msg1_req->harqindex = 0;
		Msg1_req->harq_result = 0;
		Msg1_req->cqi = Msg1_recv->cqi;
		Msg1_req->pmi = 0;
		Msg1_req->sr = 0;
		Msg1_req->ta = Msg1_recv->ta;
		Msg1_req->rapid = Msg1_recv->rapid;
	
		//fsm_printf("[HEXI]RAPID IN SRIO:%d\n",Msg1_recv->rapid);
	
		fsm_mem_cpy((Msg1 + sizeof(u32)),(void *)Msg1_req, sizeof(S_RAinfo));
		fsm_do_ioctrl(STRM_TO_RLCMAC,IOCCMD_PHYtoMAC_RA_Req,(void *)Msg1,sizeof(u32)+sizeof(S_RAinfo));
		fsm_pkt_destroy(pkptr);
	}
	else 
		fsm_pkt_destroy(pkptr);
	
	//MessageType = (u32 *)(pkptr->data);
	//fsm_mem_cpy(MessageType, pkptr->data, sizeof(u32));
	//fsm_skb_pull(pkptr, ETH_HLEN);
	//fsm_printf("[ENB SRIO]Received packet type = %d\n", *MessageType);
	//fsm_printf((char*)pkptr->data);
	//fsm_printf("\n");
	//fsm_skb_pull(pkptr,sizeof(struct lte_test_srio_head));
	
	//send_packet_period();

/***ici dispose****///////
/*
sh_ptr = (struct lte_test_srio_head*)pkptr->data;
fsm_skb_pull(pkptr, sizeof(struct lte_test_srio_head));
ici_to_phyadapter=(MACtoPHYadapter_IciMsg *)fsm_skb_pull(pkptr,sizeof(MACtoPHYadapter_IciMsg));

ici_to_mac = (PHYadaptertoMAC_IciMsg*)pkptr->head;
ici_to_mac->tcid=ici_to_phyadapter->tcid;
ici_to_mac->MessageType =ici_to_phyadapter->MessageType; 
ici_to_mac->rnti=ici_to_phyadapter->rnti; 
fsm_skb_put( pkptr,sizeof(struct PHYadaptertoMAC_IciMsg)); //20140719
fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719

if(fsm_ntohl(sh_ptr->type) == 0)
	{
	//fsm_skb_pull(pkptr, sizeof(struct lte_test_srio_head));
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
	}
else if(fsm_ntohl(sh_ptr->type) == 1)
	{
	//fsm_skb_pull(pkptr, sizeof(struct lte_test_srio_head));
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_destroy(pkptr);
	}
else 
	fsm_pkt_destroy(pkptr);*/
	FOUT;
}

/********************************************************************************
** Function name: idle_exit
** Description: 关闭srio模块
** Input:
** Output:
** Returns:
** Created by:
** Created Date:
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/
static void idle_exit(void)
{
	FIN(idle_exit());
	if(SRIO_CLOSE)
	{
		srio_close();
	}
	FOUT;
}
/***
static void send_packet_period(void)
{
FSM_PKT* pkptr;
struct lte_test_srio_head* sh_ptr;
struct ethhdr* head_ptr;
char* data = "Node0 says hello world!";
char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
FIN(send_packet_period());
SV_PTR_GET(srio_sv);
if(PACKET_SEND_PERIOD)
{
pkptr = fsm_pkt_create(128);
fsm_skb_put(pkptr, 64);
fsm_mem_cpy(pkptr->data, data, 24);
if(fsm_skb_headroom(pkptr) < (ETH_HLEN + sizeof(struct lte_test_srio_head)))
	{
	pkptr = fsm_skb_realloc_headeroom(pkptr,ETH_HLEN + sizeof(struct lte_test_srio_head));
	if(pkptr == NULL)
		return;
	}
fsm_skb_push(pkptr, sizeof(struct lte_test_srio_head));
sh_ptr = (struct lte_test_srio_head*)pkptr->data;
sh_ptr->type = fsm_htonl(1);
sh_ptr->len = fsm_htonl(pkptr->len - sizeof(struct lte_test_srio_head));
//skb_reset_network_header(pkptr);
fsm_skb_push(pkptr, ETH_HLEN);
head_ptr = (struct ethhdr*)pkptr->data;
fsm_mem_cpy(head_ptr->h_dest, dst_addr, ETH_ALEN);
fsm_mem_cpy(head_ptr->h_source, fsm_intf_addr_get(STRM_TO_ETH), ETH_ALEN);
head_ptr->h_proto = fsm_htons(DEV_PROTO_SRIO);
//fsm_printf("set new timer\n");
//SV(psend_handle) = fsm_schedule_self(SV(interval), _PACKET_SEND_PERIOD);
//fsm_printf("timer event is added\n");
//fsm_pkt_destroy(pkptr);
fsm_pkt_send(pkptr,STRM_TO_ETH);
++SV(packet_count);
fsm_printf("Node0 sends hello world packet.\n");
}
FOUT;
}
***/

/**add mac ici send****2014/7/10*****/

/********************************************************************************
** Function name: send_packet_period
** Description: 周期向以太网发包(测试用)
** Input:
** Output:
** Returns:
** Created by: 张志强
** Created Date: 20140710
** ------------------------------------------------------------------------------
** Modified by: MF
** Modified Date: 
********************************************************************************/
static void send_packet_period(void)
{

	FSM_PKT* pkptr;
	//struct lte_test_srio_head* sh_ptr;
	//struct MACtoPHYadapter_IciMsg * ici_to_phyadapter;
	//struct PHYadaptertoMAC_IciMsg * ici_to_macpacket_send_to_upperlayer()
;
	struct ethhdr* head_ptr;
	
	char* data = "hello world!";
	char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	FIN(send_packet_period());
	SV_PTR_GET(srio_sv);
	//fsm_printf("[ENB SRIO] ENTER send_packet_period()\n");
	if(PACKET_SEND_PERIOD)
	{
		pkptr = fsm_pkt_create(128);
		fsm_skb_put(pkptr, 64);
		
	/*
		if(fsm_skb_headroom(pkptr) < (ETH_HLEN + sizeof(struct lte_test_srio_head))+sizeof(struct MACtoPHYadapter_IciMsg))
		{
			pkptr = fsm_skb_realloc_headeroom(pkptr,ETH_HLEN + sizeof(struct lte_test_srio_head)+sizeof(struct MACtoPHYadapter_IciMsg));
			if(pkptr == NULL)
				return;
		}
		*/
		if(fsm_skb_headroom(pkptr) < ETH_HLEN)
		{
			pkptr = fsm_skb_realloc_headeroom(pkptr,ETH_HLEN);
			if(pkptr == NULL)
				return;
		}
		fsm_mem_cpy(pkptr->data, data, 12);
		/*
		fsm_skb_push(pkptr, sizeof(struct MACtoPHYadapter_IciMsg));
		ici_to_phyadapter=(struct MACtoPHYadapter_IciMsg *)pkptr->data;
		ici_to_phyadapter->frameNo=1;
		ici_to_phyadapter->MessageType=1;
		ici_to_phyadapter->rnti=1;
		ici_to_phyadapter->subframeNo=1;
		ici_to_phyadapter->tcid=1;
		fsm_skb_push(pkptr, sizeof(struct lte_test_srio_head));
		sh_ptr = (struct lte_test_srio_head*)pkptr->data;
		sh_ptr->type = fsm_htonl(0);
		sh_ptr->len = fsm_htonl(pkptr->len - sizeof(struct lte_test_srio_head));
		*/
		//skb_reset_network_header(pkptr);
		fsm_skb_push(pkptr, ETH_HLEN);
		head_ptr = (struct ethhdr*)pkptr->data;
		fsm_mem_cpy(head_ptr->h_dest, dst_addr, ETH_ALEN);
		fsm_mem_cpy(head_ptr->h_source, fsm_intf_addr_get(STRM_TO_ETH), ETH_ALEN);
		head_ptr->h_proto = fsm_htons(DEV_PROTO_SRIO);
		//fsm_printf("set new timer\n");
		//fsm_printf("timer event is added\n");
		SV(psend_handle) = fsm_schedule_self(SV(interval), _PACKET_SEND_PERIOD);
		//fsm_pkt_send(pkptr,STRM_TO_ETH);
		//fsm_pkt_destroy(pkptr);
		++SV(packet_count);
		//fsm_printf("[ENB SRIO]Node0 sends hello world packet periodly.\n");
	}
	FOUT;
}


/********************************************************************************
** Function name: idle_ioctl_handler
** Description: 处理IDLE状态下的IOControl
** Input:
** Output:
** Returns:
** Created by: 张志强
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: MF
** Modified Date: 
********************************************************************************/
static void idle_ioctl_handler(void)
{
	char* rec_data_ptr;
	u32 *interval_ptr;
	DCI_STORE Dci_Store;
	ENBMAC_TO_PHY_DLschedule* Dl_Schedule;
	ENBMAC_TO_PHY_ULschedule* Ul_Schedule;
	ENBMAC_TO_PHY_Rardci* Rar_Dci;
	void* ioctl_data;
	u32 i;
	u16 rnti=0;
	

	const char* data_ptr = "Hello MAC,I AM SRIO\n"; 
	
	FIN(ioctl_handler());
	SV_PTR_GET(srio_sv);
	if(IOCTL_ARRIVAL)
	{
		switch(fsm_ev_ioctrl_cmd())
		{
			case IOCCMD_PSEND_RUN:
				if(SV(psend_handle) == 0)
				{
					SV(psend_handle) = fsm_schedule_self(SV(interval), _PACKET_SEND_PERIOD);
				}
			FOUT;
			case IOCCMD_PSEND_STOP:
				if(SV(psend_handle))
				{
					fsm_schedule_cancel(SV(psend_handle));
					SV(psend_handle)= 0;
					
				}
			FOUT;
			case IOCCMD_PSEND_INTERVAL:
				interval_ptr = (u32*)fsm_data_get();
				SV(interval) = *interval_ptr;
				fsm_data_destroy(interval_ptr);
				if(SV(psend_handle))
				{
					fsm_schedule_cancel(SV(psend_handle));
					SV(psend_handle) = fsm_schedule_self(SV(interval), _PACKET_SEND_PERIOD);	
				}
				else
				{
					SV(psend_handle) = fsm_schedule_self(SV(interval), _PACKET_SEND_PERIOD);
				}
			FOUT;
			case IOCCMD_SAY_HELLO:
				rec_data_ptr = (char*)fsm_data_get();
				fsm_printf(rec_data_ptr);
				fsm_data_destroy(rec_data_ptr);
				fsm_do_ioctrl(STRM_TO_RLCMAC, IOCCMD_SAY_HELLO, (void*)data_ptr, 22);	
			FOUT;
			//20140715 mf
			/*
			case IOCCMD_TEST_SEND_MSG3:
				fsm_printf("SRIO:IOCCMD_TEST_SEND_MSG3.\n");
				if(SV(psend_handle) == 0)
				{
					SV(psend_handle) = fsm_schedule_self(SV(interval), _MSG3_FROM_UPPER);
				}
				FOUT;
				*/
			case IOCCMD_MACtoPHY_RNTI_Indicate:
				fsm_printf("SRIO:IOCCMD_MACtoPHY_RNTI_Indicate.\n");
				//void *data;
			//测试RA请求
				//data=fsm_data_get();
				if(SV(psend_handle) == 0)
				{
					SV(psend_handle) = fsm_schedule_self(0, _MSG3_FROM_UPPER);
				}
				FOUT;
			case IOCCMD_MACtoPHY_recv_sysinfo: // 20140715 mf 沤媒脥锚鲁脡
				fsm_printf("SRIO:IOCCMD_MACtoPHY_recv_sysinfo.\n");
				//send_sysinfo();
				FOUT;
			case IOCCMD_MACtoPHY_recv_paging:
				fsm_printf("SRIO:IOCCMD_MACtoPHY_recv_paging.\n");
				//send_paging();
				FOUT;
			case IOCCMD_MACtoPHY_Preamble_Indicate:



				fsm_printf("SRIO:IOCCMD_MACtoPHY_Preamble_Indicate.\n");
				//send_rar();
				FOUT;
			/*************20141013 MF modified**************To receive type1 ioctl from RRC*************/
			case IOCCMD_RRCtoPHY_Type1_Indicate:
				//ioctl_data = fsm_data_get();
				send_type1();
				FOUT;
				break;
			/**************20141017 mf modified for test****************/
			case IOCCMD_TEST_SEND_TO_ETH:
				test_send_to_eth();
				FOUT;
				break;
			/**************20141029 MACSRIO联调 mf modified*************/
			case IOCCMD_SEND_MSG1:
				fsm_printf("[srio][idle_ioctl_handler][-->]IOCCMD_SEND_MSG1.\n");
				test_send_msg1();
				FOUT;
				break;
			case IOCCMD_SEND_MSG3:
				fsm_printf("[srio][idle_ioctl_handler][-->]IOCCMD_SEND_MSG3.\n");
				test_send_msg3();
				FOUT;
				break;
			case IOCCMD_SEND_TA:
				fsm_printf("[srio][idle_ioctl_handler][-->]IOCCMD_SEND_TA.\n");
				test_send_ta();
				FOUT;
				break;
			case IOCCMD_SEND_ULDATA:
				fsm_printf("[srio][idle_ioctl_handler][-->]IOCCMD_SEND_ULDATA.\n");
				test_send_uldata();
				FOUT;
				break;
			case IOCCMD_SEND_SF:
				fsm_printf("[srio][idle_ioctl_handler][-->]IOCCMD_SEND_SF.\n");
				test_send_sf();
				FOUT;
				break;
			case IOCCMD_DLSCHEDULE:
				fsm_printf("[srio][idle_ioctl_handler][-->]IOCCMD_DLSCHEDULE.\n");
				Dci_Store = SV(Dci_Store);
				Dl_Schedule = (ENBMAC_TO_PHY_DLschedule*)fsm_data_get();
				rnti = Dl_Schedule->m_rnti;
				//fsm_printf("[srio]dl dci  rnti=%d\n",rnti);
				//FOUT;
				for (i = 0; i < USER_NUM; i ++)
				{
					if (Dci_Store.rnti[i] == 0)
					{
						Dci_Store.rnti[i] == rnti;
						Dci_Store.DL_DCI[i] = Dl_Schedule->dl_dci;
						break;
					}
					else
					{
						if (Dci_Store.rnti[i] == rnti)
						{
							Dci_Store.DL_DCI[i] = Dl_Schedule->dl_dci;
							break;
						}
					}
				}
				FOUT;
				break;
			case IOCCMD_ULSCHEDULE:
				//fsm_printf("[srio][idle_ioctl_handler][-->]IOCCMD_ULSCHEDULE.\n");
				send_uldci_to_ue();
				FOUT;
				break;
			case IOCCMD_RARDCI:
				//fsm_printf("[srio][idle_ioctl_handler][-->]IOCCMD_RARDCI.\n");
				Dci_Store = SV(Dci_Store);
				Rar_Dci = (ENBMAC_TO_PHY_Rardci*)fsm_data_get();
				rnti = Rar_Dci->m_rnti;
				//fsm_printf("[MF]IOCCMD has got rnti = %d\n", rnti);
				for (i = 0; i < USER_NUM; i ++)
				{
					if (Dci_Store.rnti[i] == 0)
					{
						Dci_Store.rnti[i] == rnti;
						Dci_Store.RAR_DCI[i] = Rar_Dci->rar_dci;
						break;
					}
					else
					{
						if (Dci_Store.rnti[i] == rnti)
						{
							Dci_Store.RAR_DCI[i] = Rar_Dci->rar_dci;
							break;
						}
					}
				}
				FOUT;
				break;
			default:
				fsm_printf("SRIO:Unrecognized I/O control command.\n");
			FOUT;
		}
	}
}
static void send_uldci_to_ue(void)
{
	FIN(send_uldci_to_ue());
	u32 i;
	FSM_PKT* pkptr;
	struct lte_test_srio_head* sh_ptr;
	struct ethhdr* head_ptr;
	DCI_STORE Dci_Store;
	ENBMAC_TO_PHY_ULschedule* Ul_Schedule;
	u32 rnti;
	char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	
	SV_PTR_GET(srio_sv);

	Dci_Store = SV(Dci_Store);
	Ul_Schedule = (ENBMAC_TO_PHY_ULschedule*)fsm_data_get();
	//
	//FOUT;
	rnti = Ul_Schedule->m_rnti;
	for (i = 0; i < USER_NUM; i ++)
	{
		if (Dci_Store.rnti[i] == 0)
		{
			Dci_Store.rnti[i] == rnti;
			Dci_Store.UL_DCI[i] = Ul_Schedule->s_ul_dci;
			break;
		}
		else
		{
			if (Dci_Store.rnti[i] == rnti)
			{
				Dci_Store.UL_DCI[i] = Ul_Schedule->s_ul_dci;
				break;
			}
		}
	}

	pkptr = fsm_pkt_create(ETH_HLEN + sizeof(struct lte_test_srio_head) + sizeof(Data_Ul_DCI));
	//printk("[ENB SRIO]Ul_Schedule->s_ul_dci.RIV = %d\n",(Ul_Schedule->s_ul_dci).RIV);
	u32 uls =0;
	fsm_mem_cpy(&uls, &(Ul_Schedule->s_ul_dci), sizeof(u32));
	//fsm_printf("[ENB MF]Ul_Schedule = %d\n",uls);
	fsm_skb_put(pkptr, sizeof(Data_Ul_DCI));
	fsm_mem_cpy(pkptr->data, &(Ul_Schedule->s_ul_dci), sizeof(Data_Ul_DCI));

	//fsm_printf("[ENB MF]pkt->data = %d\n", *((u32*)pkptr->data));
	u32 len = pkptr->tail - pkptr->data;
	//fsm_printf("[DFZ]len = %d\n", len);
	u32 j =0;
	/*for (j = 0; j < len; j ++)
	{
		fsm_printf("%d ", *((u32*)pkptr->data+j*sizeof(u32)));
	}
	fsm_printf("\n");*/
	if(pkptr != NULL)
	{
	
		if(fsm_skb_headroom(pkptr) < (ETH_HLEN + sizeof(struct lte_test_srio_head)))
			{
			pkptr = fsm_skb_realloc_headeroom(pkptr,ETH_HLEN + sizeof(struct lte_test_srio_head));
			if(pkptr == NULL)
				return;
			}
		
		fsm_skb_push(pkptr, sizeof(struct lte_test_srio_head));
		sh_ptr = (struct lte_test_srio_head*)pkptr->data;
		sh_ptr->type = fsm_htonl(3);
		sh_ptr->len = fsm_htonl(pkptr->len-sizeof(struct lte_test_srio_head));
		sh_ptr->rnti = fsm_htons(rnti);
		sh_ptr->sfn = fsm_htons(SV(sfn));
		sh_ptr->subframeN = fsm_htons(SV(subframeN));
		
		fsm_skb_push(pkptr, ETH_HLEN);
		head_ptr = (struct ethhdr*)pkptr->data;
		fsm_mem_cpy(head_ptr->h_dest, dst_addr, ETH_ALEN);
		fsm_mem_cpy(head_ptr->h_source, fsm_intf_addr_get(STRM_TO_ETH), ETH_ALEN);
		head_ptr->h_proto = fsm_htons(DEV_PROTO_SRIO);	
	//	fsm_octets_print(&pkptr->protocol, 2);
		//fsm_printf("[ENB MF] ULDCI RNTI = %d\n", rnti);
		//fsm_printf("[ENB MF AFTER]pkt->data = %d\n", *((u32*)(pkptr->data + ETH_HLEN + sizeof(struct lte_test_srio_head))));
		fsm_pkt_send(pkptr,STRM_TO_ETH);
		SV(packet_count)++;
		//printk("[ENB SRIO]send pkt to eth\n");
	}
	fsm_data_destroy(Ul_Schedule);
	FOUT;
				
}
static void test_send_msg1(void)
{
	u32 i;
	u32 num = 3;
	void* Msg1 = fsm_mem_alloc(sizeof(u32) + num * sizeof(S_RAinfo));
	fsm_mem_cpy(Msg1,(void *)(&num),sizeof(u32));
	for (i = 0; i < num;i ++)
	{
		S_RAinfo *Msg1_req = (S_RAinfo *)fsm_mem_alloc(sizeof(S_RAinfo));

		Msg1_req->total_num = num;
		Msg1_req->index = i;
		Msg1_req->ra_rnti = 61;
		Msg1_req->sfn = 0;
		Msg1_req->subframeN = 2;
		Msg1_req->crc = 0;
		Msg1_req->harqindex = 0;
		Msg1_req->harq_result = 0;
		Msg1_req->cqi = 9;
		Msg1_req->pmi = 0;
		Msg1_req->sr = 0;
		Msg1_req->ta = 0;
		Msg1_req->rapid = i+1;

		fsm_mem_cpy((Msg1 + sizeof(u32) + i*sizeof(S_RAinfo)),(void *)Msg1_req, sizeof(S_RAinfo));
		fsm_mem_free((void *)Msg1_req);
	}

	fsm_do_ioctrl(STRM_TO_RLCMAC,IOCCMD_PHYtoMAC_RA_Req,(void *)Msg1,sizeof(u32)+num*sizeof(S_RAinfo));
	//fsm_printf("[srio][test_send_msg1][-->]Message1 has sent to MAC.\n");
	
	
}
static void test_send_msg3(void)
{
	FSM_PKT* pkptr;
	pkptr = fsm_pkt_create(2048);
	msgFromCCCH(pkptr);
}

static void test_send_ta(void)
{
	Ue_ta_info *Ta_info = (Ue_ta_info *)fsm_mem_alloc(sizeof(Ue_ta_info));

	Ta_info->rnti = 61;
	Ta_info->update_flag = 1;
	Ta_info->ta = 42;

	fsm_do_ioctrl(STRM_TO_RLCMAC,IOCCMD_PHYtoMAC_TA,(void *)Ta_info,sizeof(Ue_ta_info));
	//fsm_printf("[srio][test_send_ta][-->]TA has sent to MAC.\n");
}

static void test_send_uldata(void)
{
	FSM_PKT* pkptr;
	//pkptr = fsm_pkt_create(248);
	//createHead(pkptr, 2, 0, 2);
	pkptr=generatepkt(61);
	fsm_pkt_send(pkptr,STRM_TO_RLCMAC);
}

static void test_send_sf(void)
{
	SV_PTR_GET(srio_sv);
	/*PHY_TO_MAC_frame *Sf_info = (PHY_TO_MAC_frame *)fsm_mem_alloc(sizeof(PHY_TO_MAC_frame));

	Sf_info->sfn = SV(sfn);
	Sf_info->subframeN = SV(subframeN);

	fsm_do_ioctrl(STRM_TO_RLCMAC,IOCCMD_PHYtoMAC_FRAME,(void *)Sf_info,sizeof(Sf_info));*/
	//fsm_printf("[srio][test_send_sf][-->]SF has sent to MAC.sf = %d, subframeN = %d\n", sfn, subframeN);

	SV(sfn) ++;
	SV(subframeN) ++;

	if (SV(sfn) > 1023)
		SV(sfn) = 0;
	if (SV(subframeN) > 9)
		SV(subframeN) = 0;

	export_frameNo = SV(sfn);
	export_subframeNo = SV(subframeN);
	
	fsm_schedule_self(100, _TEST_SEND_SF);
}



/**************20141017 mf modified for test****************/
static void test_send_to_eth(void)
{
	FSM_PKT* pkptr;

	pkptr = fsm_pkt_create(2048);
	//packet_send_to_eth(pkptr);

}


/********************************************************************************
** Function name: cfg_ioctl_handler
** Description: 处理CFG状态下的IOControl
** Input:
** Output:
** Returns:
** Created by: MF
** Created Date: 20141013
** ------------------------------------------------------------------------------
** Modified by: MF
** Modified Date: 
********************************************************************************/
static void cfg_ioctl_handler(void)
{
	FIN(ioctl_handler());

	void* ioctl_data;
	
	if(IOCTL_ARRIVAL)
	{
		switch(fsm_ev_ioctrl_cmd())
		{
			/*************20141013 MF modified**************To receive type1 ioctl from RRC*************/
			default:
				fsm_printf("SRIO:Unrecognized I/O control command.\n");
			FOUT;
		}
	}
}



/******************20141013 mf modified*********************************/
static void send_type1(void){
	FIN(send_type1());
		ENB_Type1* type1;
		void* rrcdata;
		SV_PTR_GET(srio_sv);

		rrcdata = fsm_data_get();
		fsm_mem_cpy(rrcdata,type1,sizeof(RRC_Type1_data));
		//type1->NumType1 = SV(type1_cnt);
		type1->MemStart = MemoryStart;
		type1->MemSize = MemorySize;

		//写入到SRIO发送区
	FOUT;
}


/********************************************************************************
** Function name: print_tran_info
** Description: 打印当前状态
** Input: const char *str 需打印的内容
** Output:
** Returns:
** Created by: 马芳
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

void print_tran_info( const char *str)
{
	FIN( print_tran_info());
	u32 curtime=0;
	curtime=fsm_get_curtime();
	fsm_printf("%d ",curtime);
	fsm_printf(str);
	fsm_printf("\n");
FOUT;
}

/********************************************************************************
** Function name: ioctldata
** Description: get并destroyioctl的data 测试用代码
** Input:
** Output:
** Returns:
** Created by: 马芳
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

void ioctldata()
{
	FIN(ioctldata());
	void *data;
	data = fsm_data_get();
	fsm_data_destroy(data);
	FOUT;
}

/********************20141030 MACSRIOTEST HX*************************************/
FSM_PKT* generatepkt(u16 rnti) //生成的内容已经设定
{
   char* data1="hello MAC one";
   char* data2="hello MAC two";
   FSM_PKT* pkptr;
   u32 len=0,reservelen=10,macheadlen=0,datalen=0,icilen=sizeof(RLCtoMAC_IciMsg);
   FIN(generatepkt());
   PHYadaptertoMAC_IciMsg *ici_msg=(PHYadaptertoMAC_IciMsg*)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
   MAC_SDU_subhead_7bit *macsubhead=(MAC_SDU_subhead_7bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));
	PHYtoMAC_Info ue_infor;
   macheadlen=2*sizeof(MAC_SDU_subhead_7bit);
   datalen=14*2;
   len=macheadlen+datalen+reservelen+sizeof(RLCtoMAC_IciMsg);
   pkptr=fsm_pkt_create(len);
   
   ici_msg->rnti=rnti;
   //ici_msg->len=macheadlen;
	ici_msg->tcid=2;
	ue_infor.rnti = rnti;
	ue_infor.sfn = 32;
	ue_infor.subframeN = 0;
	ue_infor.crc = 0;
	ue_infor.harqindex = 0;
	ue_infor.harq_result = 0;
	ue_infor.sr = 0;
	ue_infor.cqi = 9;
	ue_infor.pmi = 0;
	ue_infor.ta = 0;
	fsm_mem_cpy(&(ici_msg->ue_info),&ue_infor,sizeof(PHYtoMAC_Info));
	icilen=2*sizeof(MAC_SDU_subhead_7bit);
	//fsm_printf("[DAFANGZI]LENGTH OF TWO 7BIT HEADS:%d\n",icilen);
	fsm_skb_put(pkptr,icilen+64);
   fsm_mem_cpy(pkptr->head,ici_msg,sizeof(RLCtoMAC_IciMsg));//装入ICI信息
   createmachead7bit(macsubhead,2,14,1);//生成一个子头
   fsm_mem_cpy(pkptr->data,macsubhead,sizeof(MAC_SDU_subhead_7bit));//装入到SK_BUFF
   createmachead7bit(macsubhead,8,14,0);//生成最后一个子头
   fsm_mem_cpy(pkptr->data+sizeof(MAC_SDU_subhead_7bit),macsubhead,sizeof(MAC_SDU_subhead_7bit));//装入到SK_BUFF
   //fsm_skb_reserve(pkptr,macheadlen*2+reservelen+sizeof(RLCtoMAC_IciMsg));//头部预留足够空间
   fsm_mem_cpy(pkptr->data+icilen,data1,14);//装入SUD1
   fsm_mem_cpy(pkptr->data+14+icilen,data2,14);//装入SDU2
   
   fsm_mem_free(ici_msg);
   fsm_mem_free(macsubhead);
   
   FRET(pkptr);
}




u32 createLongBsr(FSM_PKT *skb,u32 offset){
	FIN(createLongBsr());
	u32 len,os;
	MAC_CE_longBSR *m_long_bsr=(MAC_CE_longBSR*)fsm_mem_alloc(sizeof(MAC_CE_longBSR));
	m_long_bsr->m_buffersize1=5;
	m_long_bsr->m_buffersize2=259;	//1,16,4,3
	len=sizeof(MAC_CE_longBSR);
	fsm_mem_cpy(skb->data+offset,m_long_bsr,len);
	os=offset+len;
	FRET(os);
}

u32 createCRnti(FSM_PKT *skb,u32 offset){
	FIN(createCRnti());
	u32 len,os;
	MAC_CE_Crnti *m_crnti=(MAC_CE_Crnti*)fsm_mem_alloc(sizeof(MAC_CE_Crnti));
	m_crnti->m_crnti-offset%10;
	len=sizeof(MAC_CE_Crnti);
	fsm_mem_cpy(skb->data+offset,m_crnti,len);
	os=offset+len;
	FRET(os);
}


u32 createHead(FSM_PKT *skb,u32 control_numb,u32 data_numb,char typ){
	FIN(createHead());	

	u32 i=0;
	u32 lcid=29;
	u32 len=0,from_len=0;
	u32 data_len=0;
	//char typ=1;//type=1 表示RAR type=2表示dataPDU
	char* data1="hello MAC one";
	MAC_SDU_subhead_last *last_subhead=(MAC_SDU_subhead_last*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_last));
	MAC_SDU_subhead_7bit *subhead_7bit=(MAC_SDU_subhead_7bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));
	
	PHYadaptertoMAC_IciMsg *m_phy_ici=(PHYadaptertoMAC_IciMsg*)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
	fsm_mem_set(m_phy_ici,0,sizeof(PHYadaptertoMAC_IciMsg));
	m_phy_ici->tcid=2;
	m_phy_ici->rnti=61;
	//m_phy_ici->MessageType=0;
	len=sizeof(PHYadaptertoMAC_IciMsg);
	fsm_mem_cpy(skb->head,m_phy_ici,len);

	//len=sizeof(char);
	//fsm_mem_cpy(skb->data+from_len,&typ,len);
	//from_len+=len;
	//fsm_printf("ORIGENAL DATA:\n");
	for(i=0;i<control_numb-1;i++){
		if(i%2==0){
			last_subhead->m_lcid_e_r_r=62;
			len=sizeof(MAC_SDU_subhead_last);
			//fsm_mem_cpy(skb->data+from_len,last_subhead,len);
			fsm_mem_cpy(fsm_skb_put(skb,len),last_subhead,len);
			from_len=from_len+len;

			fsm_printf("%c,",last_subhead->m_lcid_e_r_r);
		}
		else{
			last_subhead->m_lcid_e_r_r=59;
			len=sizeof(MAC_SDU_subhead_last);
			//fsm_mem_cpy(skb->data+from_len,last_subhead,len);
			fsm_mem_cpy(fsm_skb_put(skb,len),last_subhead,len);
			from_len=from_len+len;

			fsm_printf("%c,",last_subhead->m_lcid_e_r_r);
		}
	}
	last_subhead->m_lcid_e_r_r=30;
	len=sizeof(MAC_SDU_subhead_last);
	//fsm_mem_cpy(skb->data+from_len,last_subhead,len);
	fsm_mem_cpy(fsm_skb_put(skb,len),last_subhead,len);
	from_len=from_len+len;
	//fsm_printf("%c\n",last_subhead->m_lcid_e_r_r);

/*	return from_len;*/
/*	for(i=0;i<data_numb-1;i++){	//SDU 子头
		fsm_mem_set(subhead_7bit,0,sizeof(MAC_SDU_subhead_7bit));
		subhead_7bit->m_lcid_e_r_r=37;
		len=14;
		subhead_7bit->m_f_l=len;
		//data_len=data_len+len;
		len=sizeof(MAC_SDU_subhead_7bit);
		fsm_mem_cpy(skb->data+from_len,subhead_7bit,len);
		from_len=from_len+len;
	}
	fsm_mem_set(subhead_7bit,0,len);
	subhead_7bit->m_lcid_e_r_r=5;
	len=14;
	subhead_7bit->m_f_l=len;
	//data_len=data_len+len;
	len=sizeof(MAC_SDU_subhead_7bit);
	fsm_mem_cpy(skb->data+from_len,subhead_7bit,len);
	from_len=from_len+len;*/

	for(i=0;i<control_numb-1;i++){
		if(i%2==0){
			len=createLongBsr(skb,from_len);
			from_len=len;
		}
		else{
			len=createCRnti(skb,from_len);
			from_len=len;
		}
	}
	len=createLongBsr(skb,from_len);
	from_len=len;
	/*for(i=0;i<data_numb;i++){
		fsm_mem_cpy(skb->data+from_len,data1,14);
		from_len+=14;
	}*/
	
	FRET(from_len);
	//FOUT;
}

static void createmachead7bit(MAC_SDU_subhead_7bit *macsubhead,u8 lcid,u8 sdu_len,u8 continueflag)	
{
   FIN(createmachead7bit());
   if(1==continueflag)
     macsubhead->m_lcid_e_r_r=lcid+0x20;//E脫貌脰脙1
   if(0==continueflag)
     macsubhead->m_lcid_e_r_r=lcid;
   macsubhead->m_f_l=sdu_len;
   FOUT;
}

static void msgFromCCCH(FSM_PKT* skb){
	RRCConnectionRequest rrc_req;
	PHYadaptertoMAC_IciMsg *ici_to_mac=(PHYadaptertoMAC_IciMsg*)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
	MAC_SDU_subhead_7bit *mac_7bit_subhead=(MAC_SDU_subhead_7bit *)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));
	fsm_mem_set(ici_to_mac,0,sizeof(PHYadaptertoMAC_IciMsg));
	fsm_mem_set(mac_7bit_subhead,0,sizeof(MAC_SDU_subhead_7bit));
	u32 cntt1=512;
	u32 len=sizeof(u32);
	u32 cntt2=1;

	rrc_req.type=1;
	//rrc_req.ue_Identity=cntt1;
	rrc_req.establishmentCause=highPriorityAccess;
	fsm_mem_set(&(rrc_req.ue_Identity),0,sizeof(rrc_req.ue_Identity));
	//fsm_mem_cpy(&(rrc_req.ue_Identity.s_TMSI),&cntt1,len);
	rrc_req.ue_Identity.s_TMSI.m_TMSI=cntt1;
	rrc_req.ue_Identity.s_TMSI.mmec=cntt2;
	//fsm_printf("[HEXI]UE ID AND ESTABLUSHMENT CAUSE IN CONCTRUCTOIN:%d,%d,%d\n",rrc_req.ue_Identity.s_TMSI.mmec,rrc_req.ue_Identity.s_TMSI.m_TMSI,rrc_req.establishmentCause);
	/*len=sizeof(short);
	fsm_mem_cpy(&(rrc_req.establishmentCause),&cntt2,len);*/
	len=sizeof(RRCConnectionRequest);
	ici_to_mac->rnti= 61; 
	ici_to_mac->tcid= 2;
	//ici_to_mac->Ue_Info
	createmachead7bit(mac_7bit_subhead,0,len,0);
	len=sizeof(RLCtoMAC_IciMsg);
	fsm_mem_cpy(skb->head,ici_to_mac,len);
	len=sizeof(MAC_SDU_subhead_7bit);
	fsm_mem_cpy(fsm_skb_put(skb,len),mac_7bit_subhead,len);
	len=sizeof(RRCConnectionRequest);
	fsm_mem_cpy(fsm_skb_put(skb,len),&rrc_req,len);

	fsm_pkt_send(skb,STRM_TO_RLCMAC);
	//fsm_printf("[srio][msgFromCCCH][-->]MSG3 has sent to MAC.\n");
}


static void skbFromRlc(FSM_PKT* pkptr)
{
	FIN(skbFromRlc());
	u32 len=0,from_len=0;
	RLCtoMAC_IciMsg *ici_to_mac=(RLCtoMAC_IciMsg*)fsm_mem_alloc(sizeof(RLCtoMAC_IciMsg));
	char *sdu_data="hello world";
	MAC_SDU_subhead_7bit *mac_7bit_subhead=(MAC_SDU_subhead_7bit *)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));
	//FIN(test_send_msg4());
	//fsm_printf("[SRIO] enter test_send_msg4");
	//pkptr = fsm_pkt_get();
	//fsm_pkt_destroy(pkptr);


	//pkptr = fsm_pkt_create(128);
	//fsm_skb_reserve(pkptr ,sizeof(RLCtoMAC_IciMsg));//预留头部的空间 放ICI
	//ici_to_mac->tcid= 2;
	//ici_to_mac->MessageType =1; 
	ici_to_mac->rnti= 61; 
	ici_to_mac->len=sizeof(MAC_SDU_subhead_7bit);
	ici_to_mac->pbCh=2;
	len=sizeof(RLCtoMAC_IciMsg);
	fsm_mem_cpy(pkptr->head,ici_to_mac,len);//放入头部

	createmachead7bit(mac_7bit_subhead,5,12,0);
	fsm_mem_cpy(pkptr->head+len,mac_7bit_subhead,sizeof(MAC_SDU_subhead_7bit));
	//msg4_add_MacCR_element(pkptr,12);//添加MAC的数据到SK_BUF中
	fsm_mem_cpy(fsm_skb_put(pkptr,12),sdu_data,12);
	fsm_mem_free(ici_to_mac);//释放内存
	fsm_pkt_send(pkptr,STRM_TO_RLCMAC);
	//添加RRC 数据到SK_BUF中  
	
FOUT;
}

/****************************end*************************************************/

