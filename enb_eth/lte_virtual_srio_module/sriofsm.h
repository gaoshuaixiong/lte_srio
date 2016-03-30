#include "../fsm/fsmdec.h"
#include "macpkfmt.h"

#ifndef _SRIOFSM_H
#define _SRIOFSM_H

/*******逻辑信道标示*******/
#define PCH 0
#define BCH 1
#define DLSCH 2
#define MCH 3


#define _PACKET_SEND_PERIOD 	1
#define _START_WORK 		    2
#define _MSG3_FROM_UPPER        3 //20140715 mf
#define _TEST_SEND_SF			4
#define SRIO_PK_FROM_LOWER (fsm_ev_type()==FSM_EV_TYPE_PKT_IND)
#define SRIO_PK_FROM_UPPER (fsm_ev_type()==FSM_EV_TYPE_PKT_REQ)
#define SRIO_OPEN (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code()== FSM_EV_CORE_OPEN)
#define SRIO_CLOSE (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_CORE_CLOSE)
#define IOCTL_ARRIVAL (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL && fsm_ev_ioctrl_cmd() != IOCCMD_MACtoPHY_RNTI_Indicate)
#define START_WORK (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == _START_WORK)
#define PACKET_SEND_PERIOD (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == _PACKET_SEND_PERIOD)
//#define MSG3_FROM_UPPER (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == _MSG3_FROM_UPPER) //20140715 mf
#define MSG3_FROM_UPPER (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL && fsm_ev_ioctrl_cmd() == IOCCMD_MACtoPHY_RNTI_Indicate)
#define TEST_SEND_SF_PERIOD (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == _TEST_SEND_SF)

#define USER_NUM 		5

/*
typedef struct MACtoPHYadapter_IciMsg
{
   	 u16  frameNo ;       //system frame number
  	 u16  subframeNo ;    // subframe number for now
  	 u16 rnti ;         // UE ID
      u8 tcid;         // the type of Transport channel
     u16 MessageType ; // the type of packet
}__attribute__((packed))MACtoPHYadapter_IciMsg;

typedef struct PHYadaptertoMAC_IciMsg
{
      u8 tcid;         // the type of Transport channel
     u16 MessageType ; // the type of packet
     u16 rnti ; 
}__attribute__((packed))PHYadaptertoMAC_IciMsg;	*/




/************20141018 mf modified************************/

typedef struct Ul_DCI
{
    u32	m_format  	:1;                 //1bit 区分format0和1A
    u32	m_hopping 	:1;                 //1bit  跳频标志
    u32	RIV       	:13;                //13bit:when band==20MHZ type2
    u32	m_mcs		:5;                 //5bit 调制编码格式和RV信息
    u32	m_ndi		:1;                 //1bit 新数据标志
    u32	m_tpc		:2;                 //2bit
    u32 Cyclic_shift:3;                 //3bit 数据解调导频的循环移位
    u32 m_dai		:2;                 //2bit 下行分配数目的标识 when上下行时间配比：#1—6
    u32 m_cqiRequest:1;                 //1bit cqi请求标识
    u32 padding		:2;                 //2bit 填充比特 使其长度与format 1A相同(协议要求)
    u32 emptybits	:1;					//由系统带宽和32bit决定。  
}__attribute__((packed))Data_Ul_DCI;// structure of UL DCI format0
typedef struct Dl_DCI
{
	u32	m_resAlloc	:1;             	//1bit 资源指示方式的标志   采用type0方式
	u32	m_rbBitmap	:25;             	//25bit:when band==20MHZ
	u32	m_mcs		:5;               	//5bit  调制编码格式
	u32	m_ndi		:1;             	//1bit 新数据标志

	u32	m_rv		:2;            		//2bit HARQ RV信息
	u32	m_harqProcess:4;         		//4bit HARQ进程号
	u32	m_tpc		:2;                	//2bit 功率控制信息
	u32	m_dai		:2;               	//2bit 下行分配数目的标识
	u32	emptybits	:22;				//由系统带宽和32bit决定。
}__attribute__((packed))Data_Dl_DCI;//根据琼姐DCI图定义

typedef struct RAR_DCI
{
    u16	m_gap  		:1;                 //1bit 区分DVRB Gap1或者Gap2
    u16	RIV       	:9;                //9bit:when band==20MHZ
    u16	I_TBS		:5;                 //5bit tbsize序号共32种可能，即传输块大小指示，查表
    u16 emptybits	:1;				//由系统带宽和32bit决定。  
}__attribute__((packed))Data_RAR_DCI;// DCI format 1C

typedef struct MACtoPHYadapter_IciMsg
{
	u16  frameNo ;       //system frame number
	u16  subframeNo ;    // subframe number for now
	u16 rnti ;         // UE ID
	u8 tcid;         // the type of Transport channel
    // u16 MessageType ; // the type of packet，不需要modified lhl
}__attribute__((packed))MACtoPHYadapter_IciMsg;

typedef struct PHYtoMAC_Info //每个UE对应的基本信息，用于调度用的，比如RNTI，CQI等
{
    u16    rnti;             //
   // u16    pgtype;           //0,1,2
    u16    sfn;              //系统帧号，0-1023
    u16    subframeN;        //子帧号，0--9
    u16    crc;
    u16    harqindex;
	u8    harq_result;//20141008
    u16    sr;
    u16    cqi;
    u16    pmi;
    u16    ta;//modified LHL 20141011
}__attribute__((packed))PHYtoMAC_Info;//pucch

typedef struct PHYadaptertoMAC_IciMsg
{
	u8 tcid;         // the type of Transport channel
	//  u16 MessageType ; // the type of packet不需要modified lhl
	u16 rnti ;
	PHYtoMAC_Info ue_info;    
}__attribute__((packed))PHYadaptertoMAC_IciMsg;

typedef struct S_RAinfo //当有UE接入eNB时，物理层会检测到RNTI和RAPID上报 按这样的格式
{
    u16    total_num;//这是LHL新加的，表示当前总共检测到的用户数
    u16    index; //这是LHL新加的，表示该用户在当前检测到的用户中的下标，从0开始取值。
    u16    ra_rnti;             //
   // u16    pgtype;           //0,1,2。0:新建用户 1:删除用户 2:已有用户更新信息
    u16    sfn; //             //系统帧号，0-1023
    u16    subframeN;//        //子帧号，0--9
    u16    crc;
    u16    harqindex;
	u8    harq_result;//20141008
    u16    sr;
    u16    cqi;//
    u16    pmi;
    u16    ta;//
    u8    rapid;//
}__attribute__((packed))S_RAinfo;

typedef struct ENBMAC_TO_PHY_DLSCHEDULE
{
	u16	m_rnti;
	u32	m_tbsize;//PDSCHTBSIZE
	u8 cfi;
	u8 dci_format;//指示DCI的格式
	Data_Dl_DCI dl_dci;
}__attribute__((packed))ENBMAC_TO_PHY_DLschedule;//LHLmodified 20141008

typedef struct ENBMAC_TO_PHY_RARDCI
{
	u16	m_rnti;
	u32	m_tbsize; //PDSCHTBSIZE
	u8 cfi;
	u8 dci_format;
	Data_RAR_DCI rar_dci; 
}__attribute__((packed))ENBMAC_TO_PHY_Rardci;//LHLmodified 20141013

typedef struct ENBMAC_TO_PHY_ULSCHEDULE
{
	u16	m_rnti;
	u32	m_tbsize;
	u8 dci_format;
	Data_Ul_DCI s_ul_dci;
}__attribute__((packed))ENBMAC_TO_PHY_ULschedule;//LHLmodified 20141008 

typedef struct UE_TA_INFO{
	u16 rnti;
	u16 update_flag;//物理层使用原则：标志为1，标志要发TA控制单元
	u16 ta;
}__attribute__((packed))Ue_ta_info;

typedef struct COMPLEX_FRAME{
	u16    sfn; //             //系统帧号，0-1023
	u16    subframeN;//        //子帧号，0--9
} PHY_TO_MAC_frame; 


typedef struct ENB_Type1{
	//总体参数
	u16 NumType1;
	u32 MemStart;
	u32 MemSize;
	//基站侧下行连链路小区级参数
	u16 CellID;
	u16 NumAntPort;
	//u16 CFI;
	u8 CpType;
	u8 SFDirection;
	u8 NumPrbBw;
	u8 SpecSubFrmCfg;
	u8 PhichDurtion;
	u8 GPhichNg;
	//基站侧上行链路小区级参数
	u16 NumPbHo;
	u16 HoppingMode;
	u16 GroupHopEn;
	u16 SeqHopEn;
	u16 SrsSubframConfig;
	u16 CSrs;
	u16 SrsMaxUpPts;
	u8 CyclicShift;
}__attribute__((packed))ENB_Type1;

typedef struct RRC_Type1_data{

	//基站侧下行连链路小区级参数
	u16 CellID;
	u16 NumAntPort;
	//u16 CFI;
	u8 CpType;
	u8 SFDirection;
	u8 NumPrbBw;
	u8 SpecSubFrmCfg;
	u8 PhichDurtion;
	u8 GPhichNg;
	//基站侧上行链路小区级参数
	u16 NumPbHo;
	u16 HoppingMode;
	u16 GroupHopEn;
	u16 SeqHopEn;
	u16 SrsSubframConfig;
	u16 CSrs;
	u16 SrsMaxUpPts;
	u8 CyclicShift;
}__attribute__((packed))RRC_Type1_data;

	

/**************end modify**************/
typedef struct RACH_ConfigDedicated
{
  u8   ra_PreambleIndex	;  //明确发出的RA资源选择的随机接入前导
  u8   ra_PRACHMaskIndex	;  //明确发出的RA资源选择的PRACH Mask Index 
}__attribute__((packed))RACH_ConfigDedicated;


/*******************20140715 mf************************/

struct mac_cr_element_s
{
	u32 part1;
	u16 part2;
}__attribute__((packed));

typedef struct lteMacSubhead7Bit_ss{
	u8 m_lcid_e_r_r;	//LCID 5bits,E 1bit,R 1bit
	u8 m_f_l;	//F 1bit,L 7bits
}__attribute__((packed))MAC_SDU_subhead_7bit_s;	//7bit子头

/*******************20140715 mf************************/
/************20141030 mf modified************************/
typedef struct DCI_STORE
{
	u16 rnti[USER_NUM];
	Data_Ul_DCI UL_DCI[USER_NUM];
	Data_Dl_DCI DL_DCI[USER_NUM]; 
	Data_RAR_DCI RAR_DCI[USER_NUM];
}__attribute__((packed))DCI_STORE;

typedef struct srio_sv{
	int packet_count;
	unsigned int interval;
	evHandle psend_handle;
	u16 type1_cnt;
	u16 type2_cnt;
	DCI_STORE Dci_Store;
	u16 sfn;
	u16 subframeN;
	u32 SN;
} srio_sv;

typedef struct MSG1_Content
{
	u8 rapid;
	u16 rarnti;
	u16 cqi;
	u16 ta;
}__attribute__((packed))MSG1_Content;


/************       end          ************************/

	
void srio_main(void);

#endif



