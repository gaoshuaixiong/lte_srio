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
#include <linux/string.h>
#include "sriofsm.h"
#include "../lte_system.h"
#include "pkfmt.h"
#include "rrc_type.h"
#include "rrc_type_IEs.h"


//	定义状态机中的状态
/**********20141013 mf modified ************************/
#define ST_INIT	0
#define ST_CFG		1
#define	ST_IDLE	2
#define ST_SEND	3
#define ST_RECV	4
//#define ST_TEST	4	//测试阶段添加TEST状态，用于接收MSG3发送MSG4	modified by MF 20140715
/**********end modified ************************/


//	定义物理适配层本身测试用IOControl命令
#define IOCCMD_PSEND_RUN			0x01
#define IOCCMD_PSEND_STOP			0x02
#define IOCCMD_PSEND_INTERVAL	0x03 
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
/***********20141108 mf modified**************************/
#define IOCCMD_PDCCHtoMAC_ULGRANT			 0x0D
#define IOCCMD_PHYtoMAC_SYSFRAME            0x0E


/************end modify************************/

#define MemoryStart				0xfc10000
#define MemorySize					10240

//	状态机功能函数声明
static void init_enter(void);
static void send_packet_period(void);
static void srio_sv_init(void);	
static void srio_close(void);
/********mf modified 20141017 for test******/
static void packet_send_to_eth(void);
//static void packet_send_to_eth(FSM_PKT* pkptr);
static void packet_send_to_upperlayer(void);
static void idle_exit(void);
static void idle_ioctl_handler(void);
void print_tran_info( const char *str);
/**********20141013 mf modified****************************/
static void cfg_ioctl_handler(void);
//static void send_type1(void);




//	测试函数声明
static void send_msg4(void);
static void msg4_add_MacCR_element(FSM_PKT *skb,int sdu_len);//20140715 mf
static void createmachead7bit(MAC_SDU_subhead_7bit_s *macsubhead,u8 lcid,u8 sdu_len,u8 continueflag);//20140715 mf
static void send_sysinfo(void);
static void send_paging(void);
static void send_rar(void);
static int createRARPdu(FSM_PKT *skb,int number,unsigned int data);
static int my_createRARPdu(FSM_PKT *skb,int number,unsigned int rapid );
static void createPhyToMacIci(FSM_PKT *skb,int rnti,int tcid);
static void msg4_add_RRC_data(FSM_PKT *pkptr,int offset);
static struct DL_CCCH_Message *gen_dl_ccch_send_rrcsetup(void);
static int my_createContentionResolution(FSM_PKT *skb,int offset);
static int my_msg4_add_MacCR_element(FSM_PKT *skb,int sdu_len);
static void test_send_msg4();
static void ioctldata();
FSM_PKT* gen_paging(void);
FSM_PKT* gen_mib(void);
FSM_PKT* gen_sib1(void);
FSM_PKT* gen_si(void);

/********mf modified 20141017 for test******/
static void test_send_to_eth(void);

static void test_send_msg1(void);

static void test_send_sf(void);



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
				//FSM_TEST_COND(MSG3_FROM_UPPER) //20140715 mf testconmand 沤媒露拧
			FSM_COND_TEST_OUT("IDLE")	
			FSM_TRANSIT_SWITCH			
			{	
				FSM_CASE_TRANSIT(0, ST_RECV, , "IDLE -> RECV")	//艙脫脢脺脧脗虏茫脨脜脧垄			
				FSM_CASE_TRANSIT(1, ST_SEND, , "IDLE -> SEND") //路垄脣脥脡脧虏茫脨脜脧垄
				FSM_CASE_TRANSIT(2, ST_INIT,idle_exit() , "IDLE -> INIT") //脳沤脤卢禄煤脥脣鲁枚
				FSM_CASE_TRANSIT(3, ST_IDLE,send_packet_period(), "IDLE->IDLE")//露拧脢卤脝梅脰脺脝脷路垄脣脥
				//FSM_CASE_TRANSIT(4, ST_TEST, print_tran_info("IDLE->TEST"), "IDLE->TEST")//20140715 mf 
				FSM_CASE_DEFAULT(ST_IDLE,idle_ioctl_handler(), "IDLE->IDLE")	//iocontrol
			}	
		}
		FSM_STATE_FORCED(ST_RECV, "RECV", packet_send_to_upperlayer(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "RECV -> IDLE");
		}
		FSM_STATE_FORCED(ST_SEND, "SEND", packet_send_to_eth(), )   //20141018 for test
		//FSM_STATE_FORCED(ST_SEND, "SEND", print_tran_info("STATE:SEND"), )
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
		fsm_schedule_self(0, _START_WORK);
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
	SV_PTR_GET(srio_sv);//
	SV(packet_count) = 0;//
	SV(interval) = 100000;//
	SV(psend_handle) = 0;//
	SV(recv_count) = 0;//
	SV(SN)=0;
	char dest[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	SV(enb_mac_addr) = fsm_mem_alloc(sizeof(char)*6);
	fsm_mem_cpy(SV(enb_mac_addr),dest,6*sizeof(char));
	// SV(enb_mac_addr) = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	/*********20141013 mf modified*************/
	//SV(type1_cnt) = 0;
	
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
//	//fsm_octets_print(&pkptr->protocol, 2);
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
/***** mf modified 20141017 for test**************/
static void packet_send_to_eth(void)
//static void packet_send_to_eth(FSM_PKT* pkptr)
{
	FSM_PKT* pkptr;
	FSM_PKT* pkptrcopy;
	struct lte_test_srio_head* sh_ptr;
	struct ethhdr* head_ptr;
	char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int skb_len;
	
	FIN(packet_send_to_eth());
	SV_PTR_GET(srio_sv);
	pkptr = fsm_pkt_get();
	
	//for test
	
	fsm_skb_push(pkptr,4);
	fsm_mem_cpy(pkptr->data,&SV(SN),4);
	//printk("[SRIO][packet_send_to_eth] SN=%d\n",SV(SN));
	SV(SN)++;
	
	skb_len = pkptr->tail - pkptr->data;//20150123
	fsm_printf("[UE SRIO]enter packet_send_to_eth, and the skb_buffer len is %d\n", skb_len);
	pkptrcopy = fsm_pkt_create(skb_len+64);//20150123
	
	fsm_skb_put(pkptrcopy, skb_len);//20150123
	fsm_mem_cpy(pkptrcopy->data,pkptr->data,skb_len);//20150123
	//fsm_pkt_destroy(pkptr);
	
	MACtoPHYadapter_IciMsg* MactoPhyICI=(MACtoPHYadapter_IciMsg*)fsm_mem_alloc(sizeof(MACtoPHYadapter_IciMsg));

	fsm_mem_cpy(MactoPhyICI, (MACtoPHYadapter_IciMsg *)(pkptr->head), sizeof(MACtoPHYadapter_IciMsg));
	//fsm_printf("[DAFANGZI]The RNTI of the packet is %d\n",MactoPhyICI->rnti );
	
	fsm_pkt_destroy(pkptr);

	if(pkptrcopy != NULL)
	{
	/*
		if(fsm_skb_headroom(pkptr) < (ETH_HLEN + sizeof(struct lte_test_srio_head)))
			{
			pkptr = fsm_skb_realloc_headeroom(pkptr,ETH_HLEN + sizeof(struct lte_test_srio_head));
			if(pkptr == NULL)
				return;
			}
			*/
		//fsm_printf("pkptrcopy headroom before = %d\n",(pkptrcopy->data-pkptrcopy->head));
		if(fsm_skb_headroom(pkptrcopy) < (ETH_HLEN + sizeof(struct lte_test_srio_head)))
		{
			pkptrcopy = fsm_skb_realloc_headeroom(pkptr,ETH_HLEN + sizeof(struct lte_test_srio_head));
			if(pkptrcopy == NULL)
				return;
		}
		//fsm_printf("ETH_HLEN+sizeof(struct lte_test_srio_head) = %d\n",(ETH_HLEN+sizeof(struct lte_test_srio_head)));
		//fsm_printf("pkptrcopy headroom = %d\n",(pkptrcopy->data-pkptrcopy->head));

		fsm_skb_push(pkptrcopy, sizeof(struct lte_test_srio_head));
		sh_ptr = (struct lte_test_srio_head*)pkptrcopy->data;
		sh_ptr->type = fsm_htonl(2);
		sh_ptr->len = fsm_htonl(pkptrcopy->len-sizeof(struct lte_test_srio_head));
		sh_ptr->rnti = MactoPhyICI->rnti;
		fsm_skb_push(pkptrcopy, ETH_HLEN);
		head_ptr = (struct ethhdr*)pkptrcopy->data;
		fsm_mem_cpy(head_ptr->h_dest, SV(enb_mac_addr), ETH_ALEN);
		fsm_mem_cpy(head_ptr->h_source, fsm_intf_addr_get(STRM_TO_ETH), ETH_ALEN);
		head_ptr->h_proto = fsm_htons(DEV_PROTO_SRIO);	
		printk("this is the pkt to eth \n");
		fsm_octets_print(pkptrcopy->data, pkptrcopy->len);
		fsm_pkt_send(pkptrcopy,STRM_TO_ETH);
		fsm_printf("[SRIO][send to eth]data len:%d\n",pkptrcopy->len);
	SV(packet_count)++;
	}
	
	fsm_mem_free(MactoPhyICI); //liu ying tao and liu hanli 
	FOUT;}


/******************************************************
** Function name: compare_mac_addr
** Description: compare mac addr 
** Input: macaddr1, macaddr2 count
** Output:equals return true,not equal return false;
** Returns:true,false
** Created by: Godsx
** Created Date: 20160329
*********************************************************/

static int compare_mac_addr(void* macaddr1,void* macaddr2,int count)
{
	if(count>6)
	{
		return false;
	}
	u8* mac1;
	u8* mac2;
	mac1 = (u8*)macaddr1;
	mac2 = (u8*)macaddr2;
	int i=0;
	for(i=0;i<count;i++)
	{
		if((*mac1) == (*mac2))
		{
			mac2++;
			mac1++;
		}
		else
		{
			return false;
		}
	}
	return true;
}

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
	Regular_ULgrant * uldci_to_mac;
	UEPHY_TO_MAC_ULgrant * ulgrand_to_mac;
	struct lte_test_srio_head* sh_ptr;
	u32 sn;
	struct ethhdr* head_ptr;
	FIN(packet_send_to_upperlayer());
	SV_PTR_GET(srio_sv);//
	char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	fsm_printf("[UE SRIO] ENTER packet_send_to_upperlayer()\n");
	pkptr = fsm_pkt_get();
	fsm_skb_push(pkptr,sizeof(struct ethhdr));
	fsm_octets_print(pkptr->data, pkptr->len);
	head_ptr = (struct ethhdr*)pkptr->data;
	fsm_octets_print(SV(enb_mac_addr),6);
	fsm_octets_print(head_ptr->h_source,6);
	fsm_octets_print(head_ptr->h_dest,6);
	
	printk("the strcmp(SV(enb_mac_addr),dst_addr)) is %d\n,and 0==(strcmp(head_ptr->h_dest,dst_addr)) is %d\n",(true==compare_mac_addr(SV(enb_mac_addr),dst_addr,6)),(true==(compare_mac_addr(head_ptr->h_dest,dst_addr,6))));
	if((true==compare_mac_addr(SV(enb_mac_addr),dst_addr,6))&&(true==(compare_mac_addr(head_ptr->h_dest,dst_addr,6))))
	{
		fsm_mem_cpy(SV(enb_mac_addr),head_ptr->h_source,ETH_ALEN);
	}
	if(false== compare_mac_addr(head_ptr->h_source,SV(enb_mac_addr),6))
	{
		fsm_pkt_destroy(pkptr);
		FOUT;
	}
	fsm_skb_pull(pkptr,sizeof(struct ethhdr));
	fsm_printf("********************fsm_skb pull backed**************************************\n");
/***ici dispose****///////

	sh_ptr = (struct lte_test_srio_head*)pkptr->data;
	fsm_skb_pull(pkptr, sizeof(struct lte_test_srio_head));
//	ici_to_phyadapter=(MACtoPHYadapter_IciMsg *)fsm_skb_pull(pkptr,sizeof(MACtoPHYadapter_IciMsg));

//	ici_to_mac = (PHYadaptertoMAC_IciMsg*)pkptr->head;
//	ici_to_mac->tcid=ici_to_phyadapter->tcid;
//	ici_to_mac->MessageType =ici_to_phyadapter->MessageType; 
//	ici_to_mac->rnti=ici_to_phyadapter->rnti; 
//	fsm_skb_put( pkptr,sizeof(struct PHYadaptertoMAC_IciMsg)); //20140719
//	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719

	int i,j;
	//fsm_printf("[UE SRIO]ue rnti:%d\n",fsm_ntohs(sh_ptr->rnti));
	SV(recv_count)++;
	//fsm_printf("[UE SRIO] SV(recv_count)=%d\n",SV(recv_count));
	/*if(SV(recv_count)>3)
	{
		fsm_pkt_destroy(pkptr);
		FOUT;
	}*/
	if(fsm_ntohs(sh_ptr->rnti) >= 81)
	{
		printk("[UE SRIO]RNTI = %d,destroy\n",fsm_ntohs(sh_ptr->rnti));
		fsm_pkt_destroy(pkptr);
	}
	else
	{

		if(fsm_ntohl(sh_ptr->type) == 2)
		{
			//fsm_printf((char*)pkptr->data);
			//fsm_printf("\n");
			ici_to_mac=(PHYadaptertoMAC_IciMsg*)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
			ici_to_mac->tcid=2;
			ici_to_mac->rnti=fsm_ntohs(sh_ptr->rnti);
			ici_to_mac->frameNo = fsm_ntohs(sh_ptr->sfn);
			ici_to_mac->subframeNo = fsm_ntohs(sh_ptr->subframeN);
			fsm_printf("[UE SRIO]The rnti of the packet is %u, sending to MAC\n", ici_to_mac->rnti);
			fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(PHYadaptertoMAC_IciMsg));
			fsm_mem_free(ici_to_mac);
			//fsm_printf("[HEXI]TYPE IN SRIO:%c\n",*((char*)pkptr->data)+65);
			
			//for test 
			sn = *(u32*)pkptr->data;
			fsm_skb_pull(pkptr,4);
		//	printk("[SRIO][packet_send_to_upperlayer] SN=%d\n",sn);
			
			fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
			fsm_printf("[srio][send to mac]data->len:%d\n",pkptr->len);
			//fsm_octets_print(pkptr->data,pkptr->len);
			
		}
		else if(fsm_ntohl(sh_ptr->type) == 1)
		{
			//fsm_skb_pull(pkptr, sizeof(struct lte_test_srio_head));
			fsm_printf((char*)pkptr->data);
			fsm_printf("\n");
			fsm_pkt_destroy(pkptr);
		}
		else if(fsm_ntohl(sh_ptr->type) == 3)
		{
			i=pkptr->tail-pkptr->data;
			//fsm_printf("[DAFANGZI]LENGTH:%d\n",i);
			/*for(j=0;j<i;j++){
				fsm_printf("%d ",*((u32*)pkptr->data+j*sizeof(u32)));
			}
			fsm_printf("\n");*/


			//fsm_printf("[UE SRIO]uldci u32 %d\n", *((u32*)pkptr->data));
			uldci_to_mac = (struct Regular_ULgrant *)pkptr->data;
			u32 uld = 0;
			fsm_mem_cpy(&uld, uldci_to_mac, sizeof(u32));
			//fsm_printf("[UE SRIO] uld = %d\n", uld);
			//fsm_printf("[UE SRIO] uldci riv = %d\n", uldci_to_mac->RIV);
			ulgrand_to_mac = fsm_mem_alloc(sizeof(UEPHY_TO_MAC_ULgrant));
			ulgrand_to_mac->m_rnti = fsm_ntohs(sh_ptr->rnti);
			ulgrand_to_mac->frameNo = fsm_ntohs(sh_ptr->sfn);
			ulgrand_to_mac->subframeNo = fsm_ntohs(sh_ptr->subframeN);
			fsm_mem_cpy(&(ulgrand_to_mac->s_ul_dci), uldci_to_mac, sizeof(Regular_ULgrant));
			fsm_do_ioctrl(STRM_TO_RLCMAC,IOCCMD_PDCCHtoMAC_ULGRANT,(void *)ulgrand_to_mac,sizeof(UEPHY_TO_MAC_ULgrant));
			fsm_mem_free(ulgrand_to_mac);
			fsm_pkt_destroy(pkptr);
			//printk("[UE SRIO]UL_GRANT send to MAC\n");
		}
		else 
			fsm_pkt_destroy(pkptr);
	}
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
	//struct PHYadaptertoMAC_IciMsg * ici_to_mac;
	struct ethhdr* head_ptr;
	
	char* data = "send_packet_period says hello world!";
	char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	FIN(send_packet_period());
	SV_PTR_GET(srio_sv);
	if(PACKET_SEND_PERIOD)
	{
		pkptr = fsm_pkt_create(128);
		fsm_skb_put(pkptr, 64);
		fsm_mem_cpy(pkptr->data, data, 24);
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
		skb_reset_network_header(pkptr);
		fsm_skb_push(pkptr, ETH_HLEN);
		head_ptr = (struct ethhdr*)pkptr->data;
		fsm_mem_cpy(head_ptr->h_dest, dst_addr, ETH_ALEN);
		fsm_mem_cpy(head_ptr->h_source, fsm_intf_addr_get(STRM_TO_ETH), ETH_ALEN);
		head_ptr->h_proto = fsm_htons(DEV_PROTO_SRIO);
		//fsm_printf("set new timer\n");
		//fsm_printf("timer event is added\n");
		SV(psend_handle) = fsm_schedule_self(SV(interval), _PACKET_SEND_PERIOD);
		fsm_pkt_send(pkptr,STRM_TO_ETH);
		//fsm_pkt_destroy(pkptr);
		++SV(packet_count);
		fsm_printf("Node0 sends hello world packet.\n");
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
	void* ioctl_data;

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
				send_sysinfo();
				FOUT;
			case IOCCMD_MACtoPHY_recv_paging:
				fsm_printf("SRIO:IOCCMD_MACtoPHY_recv_paging.\n");
				send_paging();
				FOUT;
			case IOCCMD_MACtoPHY_Preamble_Indicate:
				fsm_printf("SRIO:IOCCMD_MACtoPHY_Preamble_Indicate.\n");
				test_send_msg1();
				//send_rar();
				FOUT;
			/*************20141013 MF modified**************To receive type1 ioctl from RRC*************/
			case IOCCMD_RRCtoPHY_Type1_Indicate:
				//ioctl_data = fsm_data_get();
				//send_type1();
				FOUT;
				break;
			/**************20141017 mf modified for test****************/
			case IOCCMD_TEST_SEND_TO_ETH:
				test_send_to_eth();
				FOUT;
				break;
			case IOCCMD_PHYtoMAC_SYSFRAME:
				test_send_sf();
			default:
				fsm_printf("SRIO:Unrecognized I/O control command.\n");
			FOUT;
		}
	}
}

static void test_send_sf(void)
{
	system_frame *Sf_info = (system_frame *)fsm_mem_alloc(sizeof(system_frame));

	Sf_info->frameNo = 512;
	Sf_info->subframeNo = 7;

	fsm_do_ioctrl(STRM_TO_RLCMAC,IOCCMD_PHYtoMAC_SYSFRAME,(void *)Sf_info,sizeof(Sf_info));
	fsm_printf("[srio][test_send_sf][-->]SF has sent to MAC.\n");
	fsm_schedule_self(100, _TEST_SEND_SF);
}


/**************20141017 mf modified for test****************/
static void test_send_to_eth(void)
{
	FSM_PKT* pkptr;

	pkptr = fsm_pkt_create(2048);
	//packet_send_to_eth(pkptr);

}

/**************20141103 mf For TEST*************************/

static void test_send_msg1(void)
{
	fsm_printf("[UE SRIO]test_send_msg1\n");
	FIN(test_send_msg1());
	

	FSM_PKT* pkptr;
	MSG1_Content* Msg1;
	struct ethhdr* head_ptr;
	struct lte_test_srio_head* sh_ptr;

	//ioctldata();
	RACH_ConfigDedicated *data;
	data = (RACH_ConfigDedicated *)fsm_data_get();
	//fsm_octets_print(data,sizeof(RACH_ConfigDedicated));
	fsm_printf("[UE SRIO] data->ra_PreambleIndex = %d\n",data->ra_PreambleIndex);

	int *MessageType;//1表示为随机接入MSG1消息
	//*MessageType = 1;
	pkptr = fsm_pkt_create(1028);
	//Msg1 = (MSG1_Content*)fsm_mem_alloc(sizeof(MSG1_Content));
	char dst_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	

	if(fsm_skb_headroom(pkptr) < (ETH_HLEN+sizeof(struct lte_test_srio_head)+sizeof(MSG1_Content)))
			{
				pkptr = fsm_skb_realloc_headeroom(pkptr,(ETH_HLEN+sizeof(struct lte_test_srio_head)+sizeof(MSG1_Content)));
				if(pkptr == NULL)
					return;
			}

	fsm_skb_push(pkptr, sizeof(MSG1_Content));
	Msg1 = (MSG1_Content*)pkptr->data;
	Msg1->cqi = 9;
	Msg1->rapid = data->ra_PreambleIndex;
	fsm_printf("[HEXI]RAPID IN SRIOFSM:%d\n", Msg1->rapid);
	//Msg1->rapid=25;	//for test
	Msg1->rarnti = 80;
	Msg1->ta = 0;

	//fsm_mem_cpy(pkptr->data,Msg1,sizeof(MSG1_Content));

	//fsm_skb_push(pkptr, sizeof(int));
	//fsm_mem_cpy(pkptr->data, (void*)(&MessageType), sizeof(int));
	

	//fsm_printf("messagetype = %d\n", (int*)(pkptr->data));

/*
	if(fsm_skb_headroom(pkptr) < ETH_HLEN)
		{
			pkptr = fsm_skb_realloc_headeroom(pkptr,ETH_HLEN);
			if(pkptr == NULL)
				return;
		}*/
		
		fsm_skb_push(pkptr, sizeof(struct lte_test_srio_head));
		sh_ptr = (struct lte_test_srio_head*)pkptr->data;
		sh_ptr->type = fsm_htonl(1);
		sh_ptr->len = fsm_htonl(pkptr->len-sizeof(struct lte_test_srio_head));
		
		fsm_skb_push(pkptr, ETH_HLEN);
		head_ptr = (struct ethhdr*)pkptr->data;
		fsm_mem_cpy(head_ptr->h_dest, dst_addr, ETH_ALEN);
		fsm_mem_cpy(head_ptr->h_source, fsm_intf_addr_get(STRM_TO_ETH), ETH_ALEN);
		head_ptr->h_proto = fsm_htons(DEV_PROTO_SRIO);	
	//	//fsm_octets_print(&pkptr->protocol, 2);
		fsm_pkt_send(pkptr,STRM_TO_ETH);
		FOUT;
		//SV(packet_count)++;
}

/**************end******************************************/
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



/******************20141013 mf modified********************************
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

******************20140715 mf MS3 MSG4*********************************/

/********************************************************************************
** Function name: createmachead7bit
** Description: 添加MAC头部 测试用代码
** Input:
** Output:
** Returns:
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/
static void createmachead7bit(MAC_SDU_subhead_7bit_s *macsubhead,u8 lcid,u8 sdu_len,u8 continueflag)	
{
	FIN(createmachead7bit());
	if(1==continueflag)
		macsubhead->m_lcid_e_r_r=lcid+0x20;//E脫貌脰脙1
	if(0==continueflag)
		macsubhead->m_lcid_e_r_r=lcid;
	macsubhead->m_f_l=sdu_len;
	FOUT;
}

/********************************************************************************
** Function name: msg4_add_MacCR_element
** Description: 添加MSG4中mac相关内容 测试用代码
** Input:
** Output:
** Returns:
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

static void msg4_add_MacCR_element(FSM_PKT *skb,int sdu_len)//sdu_len卤铆脢鸥msg4SDU碌脛鲁鈧睹?艗沤RRC碌脛setup脧没脧垄碌脛脮没啪枚鲁鈧睹?
{
	int i;
	char *temp_ptr;
	//FIN(msg4_add_MacCR_element());
	//struct mac_cr_element_s *cr_info;
	MAC_SDU_subhead_7bit_s macsubhead,cr_subhead;
	struct RRCConnectionRequest *rrc_creq_info ;
	struct S_TMSI s_TMSI_info = {0,0};

	rrc_creq_info = (struct RRCConnectionRequest *)fsm_mem_alloc(sizeof(struct RRCConnectionRequest));
	rrc_creq_info->type = 1;
	rrc_creq_info->ue_Identity.s_TMSI = s_TMSI_info;
	//rrc_creq_info->ue_Identity.s_TMSI = s_TMSI_info;
	//fsm_mem_cpy(&(rrc_creq_info.ue_Identity.randomValue));
	//for (i = 0; i < 5; i ++)
		//rrc_creq_info.ue_Identity.randomValue[i] = 1;
	rrc_creq_info->establishmentCause = mt_Access;
	createmachead7bit(&macsubhead,0,sdu_len,0);//msg4脌茂SDU脧脿脫艩碌脛MAC脳脫脥路 
	createmachead7bit(&macsubhead,28,6,1);//msg4脌茂CR驴脴脰脝碌楼脭陋脧脿脫艩碌脛MAC脳脫脥路 
	temp_ptr = (char *)rrc_creq_info;	
    fsm_mem_cpy(skb->data,&macsubhead,2); //脤铆艗脫MAC脨脜脧垄碌艙sk_buf脰脨
   	fsm_mem_cpy(skb->data+2,&cr_subhead,2);
   	fsm_mem_cpy(skb->data+4,temp_ptr+1,6);
   	fsm_mem_free(rrc_creq_info);//脢脥路脜脛脷沤忙
	
}

/********************************************************************************
** Function name: msg4_add_RRC_data
** Description: 添加MSG4中RRC相关内容 测试用代码
** Input:
** Output:
** Returns:
** Created by: 刘应涛
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

static void msg4_add_RRC_data(FSM_PKT *pkptr,int offset)
{

	struct DL_CCCH_Message* msg = gen_dl_ccch_send_rrcsetup();
	
	struct lte_rrc_head* sh_ptr=(struct lte_rrc_head*)fsm_mem_alloc(sizeof(struct lte_rrc_head*));
	
	int len= sizeof(struct lte_rrc_head);

	sh_ptr->message_type = 3;

	//lte rrc head
	//fsm_mem_cpy(pkptr->tail, sh_ptr, sizeof(struct lte_rrc_head));
	//fsm_skb_put(pkptr, sizeof(struct lte_rrc_head));
	fsm_mem_cpy(pkptr->data+offset, sh_ptr, sizeof(struct lte_rrc_head));

	//fsm_skb_put(pkptr, sizeof(struct DL_CCCH_Message));
	//fsm_skb_put(pkptr, sizeof(struct lte_rrc_head));
	//msg
	//fsm_mem_cpy(pkptr->tail, msg, sizeof(struct DL_CCCH_Message));
	//fsm_printf("tail-(data+10+len)=%d-%d=%d.\n",pkptr->tail,pkptr->data+10+len,(pkptr->tail-pkptr->data+10+len));
	//fsm_printf("len=%d.\n",sizeof(struct DL_CCCH_Message));
	if (msg == NULL)
		fsm_printf("msg == NULL\n");
	fsm_mem_cpy(pkptr->data+offset+len, msg, sizeof(struct DL_CCCH_Message));

	//FREE 20141018 mf
	fsm_mem_free(sh_ptr);
	fsm_mem_free(msg);


}

/********************************************************************************
** Function name: send_sysinfo
** Description: 系统信息发送函数 测试用代码
** Input:
** Output:
** Returns:
** Created by: 刘应涛
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

static void send_sysinfo()
{
	FSM_PKT* pkptr; 				 //20140715 mf
	struct PHYadaptertoMAC_IciMsg * ici_to_mac;  //20140715 mf

	/*fsm_printf("SRIO:  send mib!\n");
	pkptr = gen_mib();
	fsm_skb_push(pkptr, sizeof(struct PHYadaptertoMAC_IciMsg));//路脜phyadapter脥路虏驴
	ici_to_mac = (struct PHYadaptertoMAC_IciMsg *)pkptr->head;
	ici_to_mac->tcid=BCH;
	ici_to_mac->MessageType =0; //messagetype

	ici_to_mac->rnti=0; //rnti
	fsm_skb_put( pkptr,sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
	//fsm_pkt_destroy(pkptr);*/
	fsm_printf("SRIO:  send mib!\n");
	pkptr = gen_mib();
	ici_to_mac = (struct PHYadaptertoMAC_IciMsg *)pkptr->head;
	ici_to_mac->tcid=BCH;
	//ici_to_mac->MessageType =0; //messagetype
	ici_to_mac->rnti=0; //rnti
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);

	/*fsm_printf("SRIO:  send sib1!\n");
	pkptr = gen_sib1();
	fsm_skb_push(pkptr, sizeof(struct PHYadaptertoMAC_IciMsg));//路脜phyadapter脥路虏驴
	ici_to_mac = (struct PHYadaptertoMAC_IciMsg *)pkptr->head;
	ici_to_mac->tcid=BCH;
	ici_to_mac->MessageType =0; //messagetype
	ici_to_mac->rnti=0; //rnti
	fsm_skb_put( pkptr,sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
	//fsm_pkt_destroy(pkptr);*/
	fsm_printf("SRIO:  send sib1!\n");
	pkptr = gen_sib1();
	ici_to_mac = (struct PHYadaptertoMAC_IciMsg *)pkptr->head;
	ici_to_mac->tcid=BCH;
	//ici_to_mac->MessageType =0; //messagetype
	ici_to_mac->rnti=0; //rnti
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);

	/*fsm_printf("SRIO:  send si!\n");
	pkptr = gen_si();
	fsm_skb_push(pkptr, sizeof(struct PHYadaptertoMAC_IciMsg));//路脜phyadapter脥路虏驴
	ici_to_mac = (struct PHYadaptertoMAC_IciMsg *)pkptr->head;
	ici_to_mac->tcid=BCH;
	ici_to_mac->MessageType =0; //messagetype
	ici_to_mac->rnti=0; //rnti
	fsm_skb_put( pkptr,sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);*/
	fsm_printf("SRIO:  send si!\n");
	pkptr = gen_si();
	ici_to_mac = (struct PHYadaptertoMAC_IciMsg *)pkptr->head;
	ici_to_mac->tcid=BCH;
	//ici_to_mac->MessageType =0; //messagetype
	ici_to_mac->rnti=0; //rnti
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
}

/********************************************************************************
** Function name: send_paging
** Description: 寻呼信息发送函数 测试用代码
** Input:
** Output:
** Returns:
** Created by: 刘应涛
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

static void send_paging(void)
{
	/*FSM_PKT* pkptr; 				 //20140715 mf
	struct PHYadaptertoMAC_IciMsg * ici_to_mac;  //20140715 mf
	fsm_printf("SRIO: send paging!\n");
	pkptr = gen_paging();
	fsm_skb_push(pkptr, sizeof(struct PHYadaptertoMAC_IciMsg));//路脜phyadapter脥路虏驴
	ici_to_mac = (struct PHYadaptertoMAC_IciMsg *)pkptr->head;
	ici_to_mac->tcid=BCH;
	ici_to_mac->MessageType =0; //messagetype
	ici_to_mac->rnti=0; //rnti
	fsm_skb_put( pkptr,sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);*/
	FSM_PKT* pkptr; 				 //20140715 mf
	struct PHYadaptertoMAC_IciMsg * ici_to_mac;  //20140715 mf
	fsm_printf("SRIO: send paging!\n");
	pkptr = gen_paging();
	ici_to_mac = (struct PHYadaptertoMAC_IciMsg *)pkptr->head;
	ici_to_mac->tcid=BCH;
	//ici_to_mac->MessageType =0; //messagetype
	ici_to_mac->rnti=0; //rnti
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(struct PHYadaptertoMAC_IciMsg));//20140719
	fsm_printf((char*)pkptr->data);
	fsm_printf("\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
}

/********************************************************************************
** Function name: send_rar
** Description: RAR发送函数 测试用代码
** Input:
** Output:
** Returns:
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

static void send_rar(void)
{
	FSM_PKT* pkptr; 
	void *data;
	//测试RA请求
	data=fsm_data_get();
	fsm_printf("PHY RECV ra_PreambleIndex:%d\n",((RACH_ConfigDedicated *)data)->ra_PreambleIndex);
	//pkptr = fsm_pkt_create(256);
	//createRARPdu(pkptr,1,((RACH_ConfigDedicated *)data)->ra_PreambleIndex);

	pkptr = fsm_pkt_create(256);
	fsm_skb_put(pkptr,128);
	my_createRARPdu(pkptr,1,((RACH_ConfigDedicated *)data)->ra_PreambleIndex);

	
	fsm_printf((char*)pkptr->data);
	fsm_printf("1\n");
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
}
/*
static void send_msg4()
{


	FIN(send_msg4());
	fsm_printf("RECV MSG3 and SEND MSG4\n");
	FOUT;
	/*

	FSM_PKT* pkptr;
	PHYadaptertoMAC_IciMsg * ici_to_mac;
	FIN(send_msg4());

	SV_PTR_GET(srio_sv);
	pkptr = fsm_pkt_get();
	fsm_printf("RECV MSG3\n");
	fsm_pkt_destroy(pkptr);
	
	pkptr = fsm_pkt_create(128);
	fsm_skb_reserve(pkptr ,sizeof(PHYadaptertoMAC_IciMsg));//脭鈧伱疵嵚仿猜柯得劼棵暸捗?路脜ICI
	ici_to_mac =(PHYadaptertoMAC_IciMsg *)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
	ici_to_mac->tcid=2;
	ici_to_mac->MessageType =1; 
	ici_to_mac->rnti=31; 
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(PHYadaptertoMAC_IciMsg));//路脜脠毛脥路虏驴
	

	fsm_skb_put(pkptr,2+2+6);//脭枚艗脫tail脰啪脮毛拢卢脌漏沤贸脢媒鸥脻驴脮艗盲拢卢脦陋MAC碌脛脢媒鸥脻脕么鲁枚驴脮艗盲 MAC脤铆艗脫碌脛脢媒鸥脻鲁鈧睹?0byte;
	fsm_skb_put(pkptr, int len);//脭枚艗脫tail脰啪脮毛 脦陋RRC碌脛脢媒鸥脻脕么鲁枚驴脮艗盲拢卢艗沤msg4 SDU	
	msg4_add_MacCR_element(pkptr,int sdu_len);//脤铆艗脫MAC碌脛脢媒鸥脻碌艙SK_BUF脰脨
	//脤铆艗脫RRC 脢媒鸥脻碌艙SK_BUF脰脨  


}*/
/*
static void send_msg4()
{
	int len;
	FSM_PKT* pkptr;
	PHYadaptertoMAC_IciMsg * ici_to_mac;
	FIN(send_msg4());
	pkptr = fsm_pkt_create(2048);
	fsm_skb_reserve(pkptr ,sizeof(PHYadaptertoMAC_IciMsg));//脭鈧伱疵嵚仿猜柯得劼棵暸捗?路脜ICI
	ici_to_mac =(PHYadaptertoMAC_IciMsg *)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
	ici_to_mac->tcid=0;
	ici_to_mac->MessageType =1; 
	ici_to_mac->rnti=1; 
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(PHYadaptertoMAC_IciMsg));//路脜脠毛脥路虏驴
	
	len = sizeof(struct DL_CCCH_Message)+sizeof(struct lte_rrc_head);
	fsm_skb_put(pkptr,2+2+6);//脭枚艗脫tail脰啪脮毛拢卢脌漏沤贸脢媒鸥脻驴脮艗盲拢卢脦陋MAC碌脛脢媒鸥脻脕么鲁枚驴脮艗盲 MAC脤铆艗脫碌脛脢媒鸥脻鲁鈧睹?0byte;
	fsm_skb_put(pkptr, len);//脭枚艗脫tail脰啪脮毛 脦陋RRC碌脛脢媒鸥脻脕么鲁枚驴脮艗盲拢卢艗沤msg4 SDU	
	msg4_add_MacCR_element(pkptr,sizeof(struct DL_CCCH_Message)+sizeof(struct lte_rrc_head));//
	msg4_add_RRC_data(pkptr);
	fsm_printf((char*)pkptr->data);
	fsm_pkt_send(pkptr, STRM_TO_RLCMAC);
	FOUT;
}
*/

/********************************************************************************
** Function name: gen_dl_ccch_send_rrcsetup
** Description: rrc部分生成函数 测试用代码
** Input:
** Output:
** Returns: dl_ccch_rrcsetup
** Created by: 刘应涛
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

struct DL_CCCH_Message *gen_dl_ccch_send_rrcsetup(void)   

{
   //srb-ToAddModList
	struct T_PollRetransmit t_pollretransmit1={
		.t_PollRetransmittype = ms120,
    };
    struct PollPDU pollpdu1={
		 .pollPDUtype = p8,
    };
    struct PollByte pollbyte1={
		 .pollByte = kB100,
    };
    struct UL_AM_RLC  ul_amrlc1={
	 	.t_PollRetransmit = t_pollretransmit1,
    	 .pollPDU = pollpdu1,
		 .pollByte = pollbyte1,
		 .maxRetxThreshold = t2,
    };
    struct T_Reordering t_reordering1={
		.t_Reordering = t_Reordering_ms20,
    };
    struct T_StatusProhibit t_statusprohibit1={
		 .t_StatusProhibit = t_StatusProhibit_ms20,
    };
    struct DL_AM_RLC dl_am_rlc1={
		 .t_Reordering = t_reordering1,
    	 .t_StatusProhibit = t_statusprohibit1,
    };

    struct RLC_Config_am rlc_config_am1={
        .ul_AM_RLC =  ul_amrlc1,
        .dl_AM_RLC =  dl_am_rlc1,
    };
    struct RlcConfig rlcconfig1={                                      
        .type = 1,
        .rlcConfigType.am = rlc_config_am1,
    };

    struct Ul_SpecificParameters ul_specificparameters1={
        .priority = 1,       //INTEGER (1..16)
        .prioritisedBitRate = kBps32,
        .bucketSizeDuration = bucketSizeDuration_ms100,
        .logicalChannelGroup = 1,	//INTEGER (0..3)
    };
	struct LogicalChannelConfig logicalchannelconfig1 = {
		.haveUl_SpecificParameters = true,
		.ul_SpecificParameters = ul_specificparameters1,
    };
	struct SrbToAddMod srbtoaddmod1={
        .srbIdentity = 1,       //INTEGER (1..2)
        .haveRlcConfig = true,
        .rlcConfig =  rlcconfig1,    
        .haveLogicalChannelConfig = true,
        .logicalChannelConfig = logicalchannelconfig1,   
    };
    struct SrbToAddMod srbtoaddmod2={
        .srbIdentity = 2,       //INTEGER (1..2)
        .haveRlcConfig = true,
        .rlcConfig =  rlcconfig1,   
        .haveLogicalChannelConfig = true,
        .logicalChannelConfig = logicalchannelconfig1,  
    };

/**srb-ToAddModList**/
	struct SrbToAddModList srb_toaddmodlist1={
        .num = 2,     //number of SrbToAddMod in SrbToAddModList
        .srbList[0] = srbtoaddmod1,
        .srbList[1] = srbtoaddmod2,
    };

    struct DrbToAddMod drb_toaddmod1={
		.eps_BearerIdentity = 1,//INTEGER (0..15)
		.drb_Identity = 4,
		.haveRlcConfig = true,
		.rlcConfig = rlcconfig1,
		.logicalChannelIdentity = 3,//INTEGER (3..10)
		.haveLogicalChannelConfig = true,
		.logicalChannelConfig = logicalchannelconfig1,
	};
/**DrbToAddModList**/
	struct DrbToAddModList drb_toaddmodlist1={
		.num = 1,
		.drbList[0] = drb_toaddmod1,
	};
/**DrbToReleaseList**/
	struct DrbToReleaseList drb_toreleaseliast={
		.num = 1,
		.drbToRelease[0] = 4,
	};
/**mac-MainConfig**/
    struct Ul_SCH_Config ul_schconfig1={
		.maxHARQ_Tx = maxHARQ_Tx_n2,
		.periodicBSR_Timer = periodicBSR_Timer_sf40,
		.retxBSR_Timer = retxBSR_Timer_sf640,
		.ttiBundling = true,
    };
    struct ShortDRX shortdrx1={
		.shortDRX_Cycle = shortDRX_Cycle_sf64 ,
		.drxShortCycleTimer = 4,   //INTEGER (1..16)
    };
    struct DRX_Config_setup drx_config_setup1={
		.onDurationTimer =  psf60,
		.drx_InactivityTimer = drx_InactivityTimer_psf100,
		.drx_RetransmissionTimer = drx_RetransmissionTimer_psf8,
		.type = 3,    //1:sf10......
		.longDRX_CycleStartOffset.sf32 = 30,
		.haveShortDRX = true,
		.shortDRX = shortdrx1,
    };
    struct DRX_Config drx_config1={
		.type = 2,    //1:release, 2:setup
		.choice.setup = drx_config_setup1,
    };

    struct Phr_Config_Setup phr_configsetup1={

		.periodicPHR_Timer = periodicPHR_Timer_sf100,
		.prohibitPHR_Timer = prohibitPHR_Timer_sf100,
		.dl_PathlossChange = dl_PathlossChange_dB1,
    };
    struct TimeAlignmentTimer time_alignmenttimer1={
		.timeAlignmentTimertype = timeAlignmentTimertype_sf1920,
    };
	struct MAC_MainConfig mac_mainconfig1={                                   
		.haveUl_SCH_Config = true,
		.ul_SCH_Config = ul_schconfig1,
		.haveDRX_Config = true,
		.drx_Config = drx_config1,
		.timeAlignmentTimerDedicated = time_alignmenttimer1,
		.type = 2,   //1:release, 2:setup
		.phr_Config.setup = phr_configsetup1,
	};

/****SPS-Config****/
    struct C_RNTI c_rnti1={     //bitstring绫诲瀷鐨?
		.c_rnti = 4,
    };
    struct N1_PUCCH_AN_PersistentList n1_pucch_an_persistentlist1={       
		.num[0] = 1,
		.num[1] = 2,
		.num[2] = 3,
		.num[3] = 4,
    };
    struct SPS_ConfigDL_setup sps_configdl_setup1={
		.semiPersistSchedIntervalDL = semiPersistSchedIntervalDL_sf40,
		.numberOfConfSPS_Processes = 4,   //INTEGER (1..8)
		.n1_PUCCH_AN_PersistentList = n1_pucch_an_persistentlist1,
    };
    struct SPS_ConfigDL sps_configdl1={
		.type = 2,    //1:release, 2:setup
		.choice.setup = sps_configdl_setup1,
    };

    struct P0_Persistent p0_persistent1={
		.p0_NominalPUSCH_Persistent = 1,//INTEGER (-126..24)
		.p0_UE_PUSCH_Persistent = 1,    //INTEGER (-8..7)
    };

    struct SPS_ConfigUL_setup sps_config_setup1={
		.semiPersistSchedIntervalUL = semiPersistSchedIntervalUL_sf64,
		.implicitReleaseAfter = e2,
		.haveP0_Persistent = true,
		.p0_Persistent = p0_persistent1,
		.twoIntervalsConfig = true,
    };
    struct SPS_ConfigUL sps_configul1={
		.type = 2,   //1:release, 2:setup
		.choice.setup = sps_config_setup1,
    };
	struct SPS_Config sps_config1={                                             
		.haveC_RNTI = true,
		.semiPersistSchedC_RNTI = c_rnti1,
		.haveSPS_ConfigDL = true,
		.sps_ConfigDL = sps_configdl1,
		.haveSPS_ConfigUL = true,
		.sps_ConfigUL = sps_configul1,
	};

/****PhysicalConfigDedicated****/
    struct PDSCH_ConfigDedicated pdsch_configdedicated1={                   
		.p_a = p_a_dB1,
    };
    struct AckNackRepetition_setup acknacrepetition_setup1={
		.repetitionFactor = repetitionFactor_n2,
		.n1PUCCH_AN_Rep = 4,
    };

    struct PUCCH_ConfigDedicated pucch_configdedicate1={                       
		.type = 2,   //1:release, 2:setup, 3:tddAckNackFeedbackMode
		.ackNackRepetition.setup = acknacrepetition_setup1,
    };
    struct PUSCH_ConfigDedicated pusch_configdicated1={                          
		.betaOffset_ACK_Index = 2, //INTEGER (0..15)
		.betaOffset_RI_Index = 2,   //INTEGER (0..15)
		.betaOffset_CQI_Index = 2, //INTEGER (0..15)
    };
    struct FilterCoefficient filtercoefficient1={
		.filterCoefficienttype = fc6,
    };

    struct UplinkPowerControlDedicated uplinkpowercontroldedicated1={              
    	.p0_UE_PUSCH = 2,          //INTEGER (-8..7)
		.deltaMCS_Enabled = en1,    //en1 瀵瑰簲鍊?.25
		.accumulationEnabled = true,
		.p0_uE_PUCCH = 2,         //INTEGER (-8..7)
		.pSRS_Offset = 2,          //INTEGER (0..15)
		.filterCoefficient = filtercoefficient1,
    };
    struct TPC_PDCCH_Config_setup tpc_pdcch_config_setup1={
		.indexOfFormat3 = 2,       //INTEGER (1..15)
		.indexOfFormat3A = 2,       //INTEGER (1..31)
    };
    struct TPC_PDCCH_Config tpc_pdcch_config1={                                    
    	.type = 2,    //1:release, 2:setup
		.choice.setup = tpc_pdcch_config_setup1,
    };
    struct SubbandCQI subbandcqi1={
		.k = 2,                      //INTEGER (1..4)
    };
    struct CQI_ReportPeriodic_setup cqi_reportperioid1={
		.cqi_PUCCH_ResourceIndex = 100,//INTEGER (0.. 1185)
    	.cqi_pmi_ConfigIndex = 100,    //INTEGER (0..1023)
		.type = 3,    //1:widebandCQI......
		.cqi_FormatIndicatorPeriodic.ri_ConfigIndex = 100,
    };
    struct CQI_ReportPeriodic cqi_reportperodic1={
		.type = 2,   //1:release, 2:setup, 3:ri_ConfigIndex, 4:simultaneousAckNackAndCQI
		.choice.setup = cqi_reportperioid1,
    };
    struct CQI_ReportConfig cqi_reportconfig1={                                   
		.cqi_ReportModeAperiodic = rm20,
    	.nomPDSCH_RS_EPRE_Offset = 2, //INTEGER (-1..6)
		.haveCQI_ReportPeriodic = true,
		.cqi_ReportPeriodic = cqi_reportperodic1,
    };
    struct SoundingRS_UL_ConfigDedicated_setup soundrs_ul_configdedicate_setup1={
		.srs_Bandwidth = srs_Bandwidth_bw0,
		.srs_HoppingBandwidth = hbw0,
		.FreqDomainPosition = 2,      //INTEGER (0..23)
		.duration = true,
		.srs_ConfigIndex = 2,        //INTEGER (0..1023)
		.transmissionComb = 1,        //INTEGER (0..1)
		.cyclicShift = cs1,
    };
    struct SoundingRS_UL_ConfigDedicated soundrs_ul_configdedicated1={            
		.type = 2,    //1:release, 2:setup
		.choice.setup = soundrs_ul_configdedicate_setup1,
    };
    struct SchedulingRequestConfig_setup schedulingrequestconfig_setup1={
		.sr_PUCCH_ResourceIndex = 100,    //INTEGER (0..2047)
		.sr_ConfigIndex = 100,           //INTEGER (0..157)
		.dsr_TransMax  = dsr_TransMax_n4,
    };
    struct SchedulingRequestConfig schedulingrequestconfig1={                      
		.type = 2,    //1:release, 2:setup
		.choice.setup = schedulingrequestconfig_setup1,
    };
    struct AntennaInformationDedicated antennainformationdedicated1={
		.transmissionMode = tm3,
		.type_codebookSubsetRestriction = 2,    //1:n2TxAntenna_tm3......
		.codebookSubsetRestriction.n4TxAntenna_tm3 = 2,
		.type_ue_TransmitAntennaSelection = 2,
		.ue_TransmitAntennaSelection.setup = openLoop,
    };

    struct PhysicalConfigDedicated physicalconfigdedicateed1={
		.havePDSCH_ConfigDedicated = true,
		.pdsch_ConfigDedicated = pdsch_configdedicated1,
		.havePUCCH_ConfigDedicated = true,
		.pucch_ConfigDedicated = pucch_configdedicate1,
		.havePUSCH_ConfigDedicated =true,
		.pusch_ConfigDedicated =  pusch_configdicated1,
		.haveUplinkPowerControlDedicated =true,
		.uplinkPowerControlDedicated = uplinkpowercontroldedicated1,
		.haveTPC_PDCCH_Config =true,
		.tpc_PDCCH_ConfigPUCCH = tpc_pdcch_config1,
		.tpc_PDCCH_ConfigPUSCH = tpc_pdcch_config1,
		.haveCQI_ReportConfig = true,
		.cqi_ReportConfig = cqi_reportconfig1,
		.haveSoundingRS_UL_ConfigDedicated =true,
		.soundingRS_UL_ConfigDedicated = soundrs_ul_configdedicated1,
		.haveAntennaInformationDedicated = true,
		.antennaInfo = antennainformationdedicated1,
		.haveSchedulingRequestConfig = true,
		.schedulingRequestConfig = schedulingrequestconfig1,
    };


/****RadioResourceConfigDedicated****/
	struct RadioResourceConfigDedicated set_radioresourcemsg={
		.haveSrbToAddModList = true,
		.srbToAddModList = srb_toaddmodlist1,
		.haveDrbToAddModList = true,
		.drbToAddModList = drb_toaddmodlist1,
   		.haveDrbToReleaseList = true,
   		.drbToReleaseList = drb_toreleaseliast,
		.haveMAC_MainConfig = true,
		.mac_MainConfig = mac_mainconfig1,
		.haveSPS_Config = true,
		.sps_Config = sps_config1,
		.havePhysicalConfigDedicated = true,
		.physicalConfigDedicated = physicalconfigdedicateed1,
   };

    struct RRCConnectionSetup rrcConnectionSetupmsg1 ={
		.rrcTransactionIdentifier = 2,	
		.radioResourceConfigDedicated = set_radioresourcemsg,
	};


	struct DL_CCCH_Message *dl_ccch_rrcsetup = fsm_mem_alloc(sizeof(struct DL_CCCH_Message));
	dl_ccch_rrcsetup->type = 4;
	dl_ccch_rrcsetup->msg.rrcConnectionSetup = rrcConnectionSetupmsg1;
    return dl_ccch_rrcsetup;
}
/********20140715 mf 艗脫RRC****************************/
/****functions for test****/

/********************************************************************************
** Function name: gen_paging
** Description: paging生成函数 测试用代码
** Input:
** Output:
** Returns: pkptr
** Created by: 刘应涛
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

FSM_PKT* gen_paging()
{
	
	struct Paging paging;
	struct PCCH_Message pcch_msg;
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	
	paging.havePagingRecord=true;
	paging.systemInfoModification=false;
	paging.havePagingRecord=true;
	paging.pagingRecordList.num=3;
	paging.pagingRecordList.pagingRecord[0].ue_Identity.type=1;
	paging.pagingRecordList.pagingRecord[0].ue_Identity.choice.s_TMSI.mmec=11;
	paging.pagingRecordList.pagingRecord[0].ue_Identity.choice.s_TMSI.m_TMSI=123456;
	paging.pagingRecordList.pagingRecord[0].cn_Domain=0;
	paging.pagingRecordList.pagingRecord[1].ue_Identity.type=1;
	paging.pagingRecordList.pagingRecord[1].ue_Identity.choice.s_TMSI.mmec=12;
	paging.pagingRecordList.pagingRecord[1].ue_Identity.choice.s_TMSI.m_TMSI=123456;
	paging.pagingRecordList.pagingRecord[1].cn_Domain=0;
	paging.pagingRecordList.pagingRecord[2].ue_Identity.type=1;
	paging.pagingRecordList.pagingRecord[2].ue_Identity.choice.s_TMSI.mmec=12;
	paging.pagingRecordList.pagingRecord[2].ue_Identity.choice.s_TMSI.m_TMSI=123457;
	paging.pagingRecordList.pagingRecord[2].cn_Domain=0;
	
	pcch_msg.paging=paging;
	char *msg = (char *)&pcch_msg;
	int msg_len = sizeof(pcch_msg);
	int message_type = 0;
	
	//虏煤脡煤卤拧脦脛
	
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head)+sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_skb_reserve(pkptr,sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_skb_put(pkptr, sizeof(struct lte_rrc_head));
	fsm_mem_cpy(pkptr->tail, msg, msg_len);

	//脤卯鲁盲脥路虏驴
	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr == NULL)
			return;
	}
	sh_ptr = (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = message_type;
	return pkptr;
}

/********************************************************************************
** Function name: gen_si
** Description: si生成函数 测试用代码
** Input:
** Output: 
** Returns: pkptr
** Created by: 刘应涛
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

FSM_PKT* gen_si()
{
	struct SystemInformation si;
	struct BCCH_DL_SCH_Message bcch_dl_sch_message;
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	
	si.haveSib2=true;
	si.sib2.haveMBSFN_SubframeConfigList=true;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.numberOfRA_Preambles=numberOfRA_Preambles_n32;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.havePreamblesGroupAConfig=true;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.preamblesGroupAConfig.sizeOfRA_PreamblesGroupA=n8;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.preamblesGroupAConfig.messageSizeGroupA=b144;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.preamblesGroupAConfig.messagePowerOffsetGroupB=dB12;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.powerRampingParameters.powerRampingStep=powerRampingStep_dB4;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower=dBm_110;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.raSupervisionInfo.preambleTransMax=preambleTransMax_n4;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.maxHARQ_Msg3Tx=4;
	si.sib2.radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff=modificationPeriodCoeff_n4;
	si.sib2.radioResourceConfigCommon.pcch_Config.defaultPagingCycle=rf64;
	si.sib2.radioResourceConfigCommon.pcch_Config.nB=twoT;
	
	bcch_dl_sch_message.type=1;
	bcch_dl_sch_message.msg.si=si;
	char *msg=(char *)&bcch_dl_sch_message;
	int msg_len=sizeof(bcch_dl_sch_message);
	int message_type = 2;
	
	//虏煤脡煤卤拧脦脛
	
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head)+sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_skb_reserve(pkptr,sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_skb_put(pkptr, sizeof(struct lte_rrc_head));
	fsm_mem_cpy(pkptr->tail, msg, msg_len);
	//脤卯鲁盲脥路虏驴

	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr= fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr== NULL)
			return;
	}
	sh_ptr= (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = message_type;
	return pkptr;
}


/********************************************************************************
** Function name: gen_mib
** Description: mib生成函数 测试用代码
** Input:
** Output: 
** Returns: pkptr
** Created by: 刘应涛
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

FSM_PKT* gen_mib()
{
	struct MasterInformationBlock mib;
	struct BCCH_BCH_Message bcch_bch_msg;
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	
	mib.dl_Bandwidth = n25;
	//mib.phich_Config
	mib.systemFrameNumber = (u8)120;
	
	bcch_bch_msg.mib = mib;
	char *msg = (char *)&bcch_bch_msg;
	int msg_len = sizeof(bcch_bch_msg);
	int message_type = 1;

	//虏煤脡煤卤拧脦脛
	
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head)+sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_skb_reserve(pkptr,sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_skb_put(pkptr, sizeof(struct lte_rrc_head));
	fsm_mem_cpy(pkptr->tail, msg, msg_len);

	//脤卯鲁盲脥路虏驴
	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr == NULL)
			return;
	}
	sh_ptr = (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = message_type;
	return pkptr;
}
/*
FSM_PKT* gen_sib1()
{
	struct SystemInformationBlockType1 sib1;
	sib1.cellAccessRelatedInfo.cellIdentity=(u32)15;
	sib1.cellAccessRelatedInfo.trackingAreaCode=(u16)12;
	struct BCCH_DL_SCH_Message bcch_dl_sch_message;
	bcch_dl_sch_message.type=2;
	bcch_dl_sch_message.msg.sib1=sib1;
	char *msg=(char *)&bcch_dl_sch_message;
	int msg_len=sizeof(bcch_dl_sch_message);
	
	
	//虏煤脡煤卤拧脦脛
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr= fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);
	
	//脤卯鲁盲脥路虏驴
	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr= fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr== NULL)
			return;
	}
	fsm_skb_push(pkptr, sizeof(struct lte_rrc_head));
	sh_ptr= (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = 2;
	return pkptr;
}
*/

/********************************************************************************
** Function name: gen_sib1
** Description: sib1生成函数 测试用代码
** Input:
** Output: 
** Returns: pkptr
** Created by: 刘应涛
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

FSM_PKT* gen_sib1()
{
	struct SystemInformationBlockType1 sib1;
	struct BCCH_DL_SCH_Message bcch_dl_sch_message;
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	
	sib1.cellAccessRelatedInfo.cellIdentity=(u32)15;
	sib1.cellAccessRelatedInfo.trackingAreaCode=(u16)12;
	
	bcch_dl_sch_message.type=2;
	bcch_dl_sch_message.msg.sib1=sib1;
	char *msg=(char *)&bcch_dl_sch_message;
	int msg_len=sizeof(bcch_dl_sch_message);
	
	
	//虏煤脡煤卤拧脦脛
	
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head)+sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_skb_reserve(pkptr,sizeof(struct PHYadaptertoMAC_IciMsg));
	fsm_skb_put(pkptr, sizeof(struct lte_rrc_head));
	fsm_mem_cpy(pkptr->tail, msg, msg_len);
	
	//脤卯鲁盲脥路虏驴
	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr= fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr== NULL)
			return;
	}
	sh_ptr= (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = 2;
	return pkptr;
}

/********************************************************************************
** Function name: createPhyToMacIci
** Description: phytomac ici 生成函数 测试用编写 后用于正常使用
** Input: FSM_PKT *skb,int rnti,int tcid
** Output:
** Returns:
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 马芳
** Modified Date: 
********************************************************************************/
static void createPhyToMacIci(FSM_PKT *skb,int rnti,int tcid){
	int len=0;
	PHYadaptertoMAC_IciMsg *m_ici=(PHYadaptertoMAC_IciMsg*)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
	//m_ici->MessageType=0;
	m_ici->rnti=rnti;
	m_ici->tcid=tcid;

	len=sizeof(PHYadaptertoMAC_IciMsg);
	fsm_mem_cpy(skb->head,m_ici,len);
	//FREE 20141018 mf modified
	fsm_mem_free(m_ici);
}

/********************************************************************************
** Function name: createRARPdu
** Description: RAR生成函数 测试用代码 现不用
** Input:
** Output:
** Returns:
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

/*
static int createRARPdu(FSM_PKT *skb,int number,unsigned int data){
	int len=0,from_len=0,i;
	char typ=1;
	MAC_RAR_subhead_withbi *bi_subhead=(MAC_RAR_subhead_withbi*)fsm_mem_alloc(sizeof(MAC_RAR_subhead_withbi));	//20140430啪脛
	MAC_RAR_subhead *rar_subhead=(MAC_RAR_subhead*)fsm_mem_alloc(sizeof(MAC_RAR_subhead));	//20140430啪脛
	MAC_RAR_sdu *rar_sdu=(MAC_RAR_sdu*)fsm_mem_alloc(sizeof(MAC_RAR_sdu));	//20140430啪脛

	fsm_mem_set(bi_subhead,0,sizeof(MAC_RAR_subhead_withbi));	//20140430啪脛
	fsm_mem_set(rar_subhead,0,sizeof(MAC_RAR_subhead));	//20140430啪脛
	fsm_mem_set(rar_sdu,0,sizeof(MAC_RAR_sdu));	//20140430啪脛

	createPhyToMacIci(skb,31,2);

	len=sizeof(char);
	fsm_mem_cpy(fsm_skb_put(skb,len),&typ,len);

	bi_subhead->m_e_t_r_r_bi=129;
	len=sizeof(MAC_RAR_subhead_withbi);
	//fsm_mem_cpy(skb->data+from_len,bi_subhead,len);
	fsm_mem_cpy(fsm_skb_put(skb,len),bi_subhead,len);
	from_len+=len;
	for(i=0;i<number-1;i++){
		rar_subhead->m_e_t_rapid=223;
		len=sizeof(MAC_RAR_subhead);
		fsm_mem_cpy(skb->data+from_len,rar_subhead,len);
		from_len+=len;
	}
	rar_subhead->m_e_t_rapid=64;
	rar_subhead->m_e_t_rapid=rar_subhead->m_e_t_rapid+data;
	len=sizeof(MAC_RAR_subhead);
	//fsm_mem_cpy(skb->data+from_len,rar_subhead,len);
	fsm_mem_cpy(fsm_skb_put(skb,len),bi_subhead,len);
	from_len+=len;
	for(i=0;i<number;i++){
		rar_sdu->m_r_ta=1;
		rar_sdu->m_ta_ulgrant=16;
		rar_sdu->m_ulgrant=31;
		rar_sdu->m_tcrnti=31;
		len=sizeof(MAC_RAR_sdu);
		//fsm_mem_cpy(skb->data+from_len,rar_sdu,len);
		fsm_mem_cpy(fsm_skb_put(skb,len),bi_subhead,len);
		from_len+=len;
	}

	FRET(from_len);

}
*/

	
/********************************************************************************
** Function name: my_createRARPdu
** Description: RAR生成函数 测试用代码 正在使用
** Input: FSM_PKT *skb,int number,unsigned int rapid 
** Output: 
** Returns: from_len RARpdu长度
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/	
static int my_createRARPdu(FSM_PKT *skb,int number,unsigned int rapid ){
	int len=0,from_len=0,i;
	char type =1;
	MAC_RAR_subhead_withbi *bi_subhead=(MAC_RAR_subhead_withbi*)fsm_mem_alloc(sizeof(MAC_RAR_subhead_withbi));	//20140430改
	MAC_RAR_subhead *rar_subhead=(MAC_RAR_subhead*)fsm_mem_alloc(sizeof(MAC_RAR_subhead));	//20140430改
	MAC_RAR_sdu *rar_sdu=(MAC_RAR_sdu*)fsm_mem_alloc(sizeof(MAC_RAR_sdu));	//20140430改
	PHYadaptertoMAC_IciMsg *m_phy_ici=(PHYadaptertoMAC_IciMsg*)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
	m_phy_ici->tcid=2;
	m_phy_ici->rnti=0;
	//m_phy_ici->MessageType=0;
	len=sizeof(PHYadaptertoMAC_IciMsg);
	fsm_mem_cpy(skb->head,m_phy_ici,len);

	fsm_mem_set(bi_subhead,0,sizeof(MAC_RAR_subhead_withbi));	//20140430改
	fsm_mem_set(rar_subhead,0,sizeof(MAC_RAR_subhead));	//20140430改
	fsm_mem_set(rar_sdu,0,sizeof(MAC_RAR_sdu));	//20140430改
	fsm_mem_cpy(skb->data+from_len,&type,1);
	from_len += 1;
	bi_subhead->m_e_t_r_r_bi=129;//生成含有BI的子头
	len=sizeof(MAC_RAR_subhead_withbi);
	fsm_mem_cpy(skb->data+from_len,bi_subhead,len);
	from_len+=len;
	for(i=0;i<number-1;i++){ // 生成含有RAPID的子头  
		rar_subhead->m_e_t_rapid = rapid+192;
		len=sizeof(MAC_RAR_subhead);
		fsm_mem_cpy(skb->data+from_len,rar_subhead,len);
		from_len+=len;
	}
	rar_subhead->m_e_t_rapid = rapid+64;
	fsm_printf("[PHY]RAR SUBHEAD IN CREATERAR:%c\n",rar_subhead->m_e_t_rapid );
	
	len=sizeof(MAC_RAR_subhead);
	fsm_mem_cpy(skb->data+from_len,rar_subhead,len);
	from_len+=len;
	for(i=0;i<number;i++){
		rar_sdu->m_r_ta=1;
		rar_sdu->m_ta_ulgrant=17;
		rar_sdu->m_ulgrant=0x8888;
		rar_sdu->m_tcrnti=31;
		len=sizeof(MAC_RAR_sdu);
		fsm_mem_cpy(skb->data+from_len,rar_sdu,len);
		from_len+=len;
	}
	//FREE 20141018 mf modified
	fsm_mem_free(bi_subhead);
	fsm_mem_free(rar_subhead);
	fsm_mem_free(rar_sdu);
	fsm_mem_free(m_phy_ici);
	FRET(from_len);
}
	
	
/***何玺测试***/

/********************************************************************************
** Function name: my_createContentionResolution
** Description: 竞争解决单元生成函数 测试用代码
** Input: FSM_PKT *skb,int offset
** Output: 
** Returns: os 偏移长度
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/	

static int my_createContentionResolution(FSM_PKT *skb,int offset){
	int cntt1=512;
	//short cntt2=2;
	int os=offset;
	int len=sizeof(int);
	short cntt2=256;

	fsm_mem_cpy(skb->data+offset,&cntt1,len);
	os+=len;
	//app=cntt2&255;
	len=sizeof(short);
	fsm_mem_cpy(skb->data+os,&cntt2,len);
	os+=len;
	
	len=sizeof(int);
	fsm_mem_cpy(&cntt1,skb->data+offset,len);
	len=sizeof(short);
	fsm_mem_cpy(&cntt2,skb->data+os-len,len);
	fsm_printf("[MSG4]PRAT1:%d,PART2:%d\n",cntt1,cntt2);
	
	return os;
}

/********************************************************************************
** Function name: my_msg4_add_MacCR_element
** Description: msg4mac相关单元生成函数 测试用代码
** Input: FSM_PKT *skb,int sdu_len
** Output: 
** Returns: len 产生部分的长度
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/	

static int my_msg4_add_MacCR_element(FSM_PKT *skb,int sdu_len){
	FIN(msg4_add_MacCR_element());
	int len=0,from_len=0;
	char typ=2;
	MAC_SDU_subhead_last *cr_subhead=(MAC_SDU_subhead_last*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_last));
	MAC_SDU_subhead_7bit *macsubhead=(MAC_SDU_subhead_7bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));
	
	len=sizeof(char);
	fsm_mem_cpy(skb->data+from_len,&typ,len);
	from_len=from_len+len;
	cr_subhead->m_lcid_e_r_r=60;
	len=sizeof(MAC_SDU_subhead_last);
	fsm_mem_cpy(skb->data+from_len,cr_subhead,len);
	from_len=from_len+len;
	createmachead7bit(macsubhead,0,sdu_len,0);
	len=sizeof(MAC_SDU_subhead_7bit);
	fsm_mem_cpy(skb->data+from_len,macsubhead,len);
	from_len=from_len+len;

	len=my_createContentionResolution(skb,from_len);

	//FREE 20141018 mf modified
	fsm_mem_free(cr_subhead);
	fsm_mem_free(macsubhead);
	FRET(len);
}

/********************************************************************************
** Function name: test_send_msg4
** Description: msg4发送函数 测试用代码
** Input:
** Output:
** Returns:
** Created by: 何玺
** Created Date: 
** ------------------------------------------------------------------------------
** Modified by: 
** Modified Date: 
********************************************************************************/

static void test_send_msg4()

{

	FSM_PKT* pkptr;
	PHYadaptertoMAC_IciMsg * ici_to_mac;
	int len=0,from_len=0,sdu_data_len=0;
	char *sdu_data="hello world";
	
	FIN(test_send_msg4());
	fsm_printf("[SRIO] enter test_send_msg4");
	pkptr = fsm_pkt_get();
	fsm_pkt_destroy(pkptr);

	pkptr = fsm_pkt_create(2048);
	fsm_skb_reserve(pkptr ,2*sizeof(PHYadaptertoMAC_IciMsg));//预留头部的空间 放ICI
	ici_to_mac =(PHYadaptertoMAC_IciMsg *)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
	ici_to_mac->tcid= 2;
	//ici_to_mac->MessageType =1; 
	ici_to_mac->rnti= 31; 
	fsm_mem_cpy(pkptr->head,ici_to_mac,sizeof(PHYadaptertoMAC_IciMsg));//放入头部

	fsm_skb_put(pkptr,1+2+2+6);//增加tail指针，扩大数据空间，为MAC的数据留出空间 MAC添加的数据长度10byte;
	fsm_skb_put(pkptr, 12);//增加tail指针 为RRC的数据留出空间，即msg4 SDU	

	fsm_skb_put(pkptr, 10);
	
	//msg4_add_MacCR_element(pkptr,12);//添加MAC的数据到SK_BUF中
	from_len=my_msg4_add_MacCR_element(pkptr,12);//添加MAC的数据到SK_BUF中
    //fsm_mem_cpy(pkptr->data+from_len,sdu_data,12);


	//添加RRC 数据到SK_BUF中  
	sdu_data_len = sizeof(sdu_data);
    //fsm_mem_free(ici_to_mac);//释放内存
        len = sizeof(struct DL_CCCH_Message)+sizeof(struct lte_rrc_head);
	fsm_printf("from_len=%d,sdu_data_len=%d,len=%d\n",from_len,sdu_data_len,len);
	fsm_skb_put(pkptr, len);
	msg4_add_RRC_data(pkptr,from_len);
	
    fsm_pkt_send(pkptr,STRM_TO_RLCMAC);
	//FREE 20141018 mf modified
	fsm_mem_free(ici_to_mac);
	
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
	int curtime=0;
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

