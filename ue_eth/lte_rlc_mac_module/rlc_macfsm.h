#include "../fsm/fsmdec.h"

#include <stdbool.h>
#include "rrc_config.h"
#include "macscheduler.h"
#include "macpkfmt.h"

#include <linux/list.h>
#include <linux/types.h>
#include "Rlctypes.h"

#ifndef _RLCMACFSM_H
#define _RLCMACFSM_H

/******************* MAC层相关定义 *******************/

//IOCTRL Command definition 定义ioctrl命令值 以及 所携带数据时对应的数据结构

#define IOCCMD_RRCtoMAC_recv_sysinfo 	    0x01 //应该配置SRIO 后期会改20141120
#define IOCCMD_RRCtoMAC_stop_recv_sysinfo 	0x02//应该配置SRIO 后期会改
#define IOCCMD_RRCtoMAC_reset           	0x03

#define IOCCMD_RRCtoMAC_RandomAcc_Req	    0x04

//ioctrl commands from RRC with configuration information RRC配置命令，携带配置信息
#define IOCCMD_RRCTOMAC_CONFIG_TAT 			0x05     //TimeAlignmentTimer 
#define IOCCMD_RRCtoMAC_RACHcommonconfig 	0x06     //RACH-ConfigCommon
#define IOCCMD_RRCTOMAC_CONFIG_BCCH 		0x07     //BCCH-Config
#define IOCCMD_RRCTOMAC_CONFIG_PCCH 		0x08     //PCCH-Config
#define IOCCMD_RRCtoMAC_mainconfig 			0x09     //MAC-MainConfig
#define IOCCMD_RRCTOMAC_CONFIG_SPS 			0x10     //SPS-Config
#define IOCCMD_RRCtoMAC_LCconfig  			0x11     //LogicalChannelConfig 
// #define IOCCMD_RRCtoMAC_reconfig            0x12    
#define IOCCMD_RRCtoMAC_LCrelease            0x13   //deleteLogicalChannel

#define IOCCMD_Harq_feedback                 0x18   //when MAC received this command from PHYadapter,MAC consider this HARQ process was successful MAC就认为所有HARQ进程负责的PDU都发送成功


#define IOCCMD_RLCtoMAC_datasend_Req         0x0B
#define IOCCMD_PDCCHtoMAC_RandomAcc_Req      0x0C
#define IOCCMD_PDCCHtoMAC_ULGRANT            0x0D  //PHY send a DCI of ul_grant to MAC
#define IOCCMD_PHYtoMAC_SYSFRAME            0x0E //PHY to MAC system frame number and subframe number
// ioctrl commands send out
#define IOCCMD_MACtoRRC_CRNTI_Indicate       0x21 
#define IOCCMD_MACtoRRC_RandomAcc_Fail       0x22
#define IOCCMD_MACtoRLC_datasend_Allow       0x23
#define IOCCMD_MACtoPHY_RNTI_Indicate        0x24           
#define IOCCMD_MACtoPHY_Preamble_Indicate    0x25     
#define IOCCMD_MACtoPHY_recv_sysinfo         0x26
#define IOCCMD_MACtoPHY_stop_recv_sysinfo    0x27
#define IOCCMD_MACtoRRC_PUCCHSRS_release     0x28  //上行失步时，通知RRC释放PUCCH/SRS的配置

//testing commands 测试用的命令 
#define TESTCMD_RAR_REVF     0x40  //映射成 RAR接收失败
#define TESTCMD_RAR_REVS     0x41  //映射成 RAR接收成功
#define TESTCMD_CR_F         0x42  //映射成 竞争解决失败
#define TESTCMD_CR_S         0x43  //映射成 竞争解决成功
#define TEST_SENDMSG3        0x44  //内部事件，驱动SENDMSG3状态转换
#define TEST_RECV_PADING 0x45
#define TEST_MACtoPHY_PADING 0x46
#define TEST_RECV_SYS 	0x47


/*************分割线201501演示添加的用户空间配置命令********************/
#define  _IOCTLtoUEMAC_RAR_Failed  0x70//配置RAR失败次数
#define  _IOCTLtoUEMAC_Contention_Failed 0x71//配置竞争解决失败次数
#define _IOCTLtoUEMAC_PKT_NUM 0x72
/*************分割线********************/


/** HX test **/
//#define            TEST_MSG3_SEND                           0x44
//self events definition 定义事件

#define Start_NonContentionBase_RandomAcc   1
#define Start_ContentionBase_RandomAcc      2
#define RandomAcc_Fail                      3
#define RARrev_Success                      4
#define ContentionResolution_Fail           5
#define Contention_Success                  6
#define SR_Fail                             7                     
 #define RA_Send_Msg3		8//LHL 20140726,第二次及以上发送MSG3，由MAC层触发
#define LC_BJ_CODE	9//MAC层逻辑信道BJ的中断码
//timers expire events definition 定义 定时器超时事件 
#define TimeAlignmentTimer_Expire          11
#define PeriodicBSRTimer_Expire            12
#define RexBSRTimer_Expire                 13
#define WaitforRAresponse_Expire           24 
#define CRTimer_Expire                     15

#define PHR_CE 0 
#define CRNTI_CE 1
#define regular_truncatedBSR_CE 2
#define regular_shortBSR_CE 3
#define regular_longBSR_CE 4
#define period_truncatedBSR_CE 5
#define period_shortBSR_CE 6
#define period_longBSR_CE 7  

#define ULHARQ_ProcessNum 2     //按照上/下行帧的配比配置，目前使用UL/DL=2/8
#define DLHARQ_ProcessNum 10    //按照上/下行帧的配比配置，目前使用UL/DL=2/8

#define MAC_BJ_TIME	100//MAC层逻辑信道BJ的更新周期，单位为10us，即更新周期为1ms
//condition definition   条件宏定义

#define MAC_PK_FROM_LOWER (fsm_ev_type()==FSM_EV_TYPE_PKT_IND)
//#define	MAC_PK_FROM_UPPER (fsm_ev_type()==FSM_EV_TYPE_PKT_REQ)	//noted in 20150908
//#define	SEND_MSG3 (fsm_ev_type()==FSM_EV_TYPE_PKT_REQ ||RA_SEND_MSG3)//modified by lhl 20140726
#define MAC_OPEN (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code()== FSM_EV_CORE_OPEN)
#define MAC_CLOSE (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_CORE_CLOSE)
#define IOCTRL_ARRIVAL (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL)


#define RACH_CONFIG (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_RRCtoMAC_RACHcommonconfig ) && fsm_ev_data()> 0 )
#define MAC_RESET (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_RRCtoMAC_reset ) )
//#define RECV_RLCREQ_WAITMSG3 (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_RLCtoMAC_datasend_Req ) )//modified by lhl 20140725
//testing code 
#define TESTCMD_RAR_REVF_T  (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== TESTCMD_RAR_REVF ) )
#define TESTCMD_RAR_REVS_T  (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== TESTCMD_RAR_REVS ) )   
#define TESTCMD_CR_F_T    (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== TESTCMD_CR_F ) )    
#define TESTCMD_CR_S_T   (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== TESTCMD_CR_S ) )   
 
#define 	LogicalChannel_BJ  (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == LC_BJ_CODE ) 
#define TimeAlignmentTimerExpire (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == TimeAlignmentTimer_Expire )        
#define PeriodicBSRTimerExpire (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == PeriodicBSRTimer_Expire )
#define RexBSRTimerExpire (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == RexBSRTimer_Expire )
#define WaitforRAresponseExpire (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == WaitforRAresponse_Expire)
#define CRTimerExpire (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CRTimer_Expire )

//涉及参数问题，还是在ioctl统一处理
//#define RANDOM_ACCESS_FROM_MAC ( fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == Start_ContentionBase_RandomAcc )
//#define RANDOM_ACCESS_FROM_MAC_NC ( fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == Start_NonContentionBase_RandomAcc )
//#define RANDOM_ACCESS_FROM_RRC_NC (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_RRCtoMAC_RandomAcc_Req) && fsm_ev_data()> 0 )
//#define RANDOM_ACCESS_FROM_RRC_C (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_RRCtoMAC_RandomAcc_Req) && fsm_ev_data()== NULL )
//#define RANDOM_ACCESS_FROM_PDCCH_NC (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_PDCCHtoMAC_RandomAcc_Req) && fsm_ev_data()> 0 )
//#define RANDOM_ACCESS_FROM_PDCCH_C (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_PDCCHtoMAC_RandomAcc_Req) && fsm_ev_data()== NULL )


//#define START_C_RANDOM_ACCESS (RANDOM_ACCESS_FROM_MAC ||RANDOM_ACCESS_FROM_RRC_C || RANDOM_ACCESS_FROM_PDCCH_C )
//#define START_NC_RANDOM_ACCESS (RANDOM_ACCESS_FROM_PDCCH_NC || RANDOM_ACCESS_FROM_RRC_NC )

#define START_C_RANDOM_ACCESS ( fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == Start_ContentionBase_RandomAcc ) 
#define START_NC_RANDOM_ACCESS ( fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == Start_NonContentionBase_RandomAcc ) 
#define RANDOM_ACCESS_FAIL  ( (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == RandomAcc_Fail) || WaitforRAresponseExpire || TESTCMD_RAR_REVF_T ) //包括了RAR接收超时 HQ 20140419
#define CONTENTION_FAIL  ( (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == ContentionResolution_Fail) || CRTimerExpire || TESTCMD_CR_F_T  ) //包括了CRtimer超时 HQ 20140419
//#define CONTENTION_FAIL  ( (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == ContentionResolution_Fail)  || TESTCMD_CR_F_T  )//去掉了CTtimer超时，CRtimer超时后会单独处理，自中断会产生CONTENTION_FAIL
#define RARREV_SUCCESS ( (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == RARrev_Success) || TESTCMD_RAR_REVS_T )
#define CONTENTION_SUCCESS ( (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == Contention_Success) || TESTCMD_CR_S_T  )
#define SR_FAIL (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == SR_Fail)
//#define HAVE_SENDMSG3 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == TEST_SENDMSG3) //testing code
#define RA_SEND_MSG3 (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == RA_Send_Msg3)//LHL 20140726
/*define data structure 定义数据结构*/



//MAC自定义 

typedef struct system_Config
{ 
	struct MAC_MainConfig  MACMainConfigInfo ;
	struct RachConfigCommon  RACHCommonConfigInfo  ;
	struct PCCH_Config    PCCHConfigInfo;
	struct BCCH_Config    BCCHConfigInfo;
	struct SPS_Config     SPSConfigInfo;
	MAC_LogicalChannelConfig_IoctrlMsg  CCCHConfigInfo ; //增加CCCH的配置信息，默认配置
}systemConfig;


//HARQ相关数据结构 
typedef struct HARQ_statu_parameters
{
	u16 CURRENT_TX_NB;
	bool HARQ_FEEDBACK ;
	u32  CURRENT_IRV;
}HARQ_statu_parameters;

typedef struct HARQ_statu_Info
{
	u32 processID;
	bool avaiable_flag; // true表示该进程空闲可用，false表示不可用。
	HARQ_statu_parameters statu;
	u32 *HARQ_buffer_pointer; //类型待斟酌，应该是个无类型的。或者指向MACPDU的指针
}HARQstatuInfo;

typedef struct HARQ_feedback
{
	u32 processID;
	bool HARQ_FEEDBACK ;
}HARQ_feedback;


typedef struct MAC_control_element_tag
{
	bool  addinPDU_flag;//TRUE表示需要复用到PDU中
	u8  LCID; //标识MAC控制单元的类型,通过LCID标识
	void *  MAC_CE_ptr;//指向具体的控制单元 ，在取具体内容时根据控制单元的类型进行类型转换
}MAC_controlelement_tag;




/*****************HQ修改完成20140531********************/
typedef struct Timer 
{
	u32 time_value ;//定时时间，单位为ms
	int* timer_sign; //定时器的标志，用于对定时器进行操作，比如取消定时器 //新框架定义的fsm_schedule_self()返回值类型为int*  20140531HQ
	bool flag;//如果该值为false则表示定时器未计时,如果等于true则表示该定时器仍在计时
}Timer; 

// RACH-ConfigDedicated 指示非竞争随机接入用的两个index 
/*typedef struct RACH_ConfigDedicated
{
	u32   ra_PreambleIndex	;  //明确发出的RA资源选择的随机接入前导
	u32   ra_PRACHMaskIndex	;  //明确发出的RA资源选择的PRACH Mask Index 
}__attribute__((packed))RACH_ConfigDedicated;*///20150115 LHL
typedef struct RACH_ConfigDedicated
{
	u8   ra_PreambleIndex	;  //明确发出的RA资源选择的随机接入前导
	u8   ra_PRACHMaskIndex	;  //明确发出的RA资源选择的PRACH Mask Index 
}__attribute__((packed))RACH_ConfigDedicated;
typedef struct RNTI_indication
{
	u32 rnti_type;//rnti的类型 1=C-RNTI  2=t-cnti 3=b-crnti 4=sps-rnti  5=p-crnti
	u32 rnti_value;//rnti的具体值
}__attribute__((packed))RNTI_indication;

/*****************************以下是HQ修改20140413******************************/
static unsigned short int BackoffTable[16]={
0,10,20,30,40,60,80,120,160,240,320,480,960,0,0,0
}; //后3个保留 

typedef struct RA_Type
{
	u32 ra_type;//0:false，1:pdcch IOCTL发起的，2:MAC层发起,3:RRC以IOCTL发起
}__attribute__((packed))RA_Type;

typedef struct SYSTEM_FRAME
{
	u16  frameNo ;       //system frame number
	u16  subframeNo ;    // subframe number for now
}__attribute__((packed))system_frame;

/*state variables definition 定义状态变量*/

/******************* node and queue for CRA buffer *******************/
typedef struct SkbNode
{
	FSM_PKT *pkt;
	struct SkbNode *next;
}QueueNode, * QueueNodePtr;

typedef struct queue
{
	QueueNode *front;
	QueueNode *rear;
	u16 pkt_count;
}CraQueue, * CraQueuePtr;
/******************* RLC层相关定义 *******************/

//#define SequenceNumber  u16
#define SequenceNumber  short

#define MAX_NUM   16
#define  MAX_CODE   50
#define FLAG 2
#define MAX_PDU_SIZE 1400
//#define MAX_PDU_SIZE 1500

//static u32 mallocnum;
//static u32 freenum;

/* The types of instances in RLC layer  */
typedef enum {NONE_MODE,TM_MODE=1, UM_MODE=2, AM_MODE=3} RlcMode;
//typedef enum {TM_TX_MODE=1, TM_RX_MODE=2, UM_TX_MODE=3, UM_RX_MODE=4, AM_MODE=5} RlcMode;
typedef enum {POLL_RETX = 1, STAT_PRO = 2, AM_REORDER = 3, UM_REORDER = 4, BUFFER_REQ = 5}RlcTimerType;
/** Data Structures Used By RLC **/

typedef enum { FULL_SDU = 1, LAST_SEGMENT = 2, ANY_SEGMENT = 3} PacketStatus;


typedef struct AmSduBuffer     //AM 新数据队列
{
	FSM_PKT * pkt;
	PacketStatus pktstatus;
	u32 pos;
	struct list_head list;
} AmSduBuffer;

typedef struct AmBuffer  //已发送队列，重传队列，接受侧数据PDU队列
{
	FSM_PKT * pkt;
	PacketStatus  pktstatus;
	u32 pos;
	u32 headLen;
	long segStart[segNumMax];	//-1:无效
	long segEnd[segNumMax];		//-1:无效
	u32 segnum;
	SequenceNumber SN;
	short retxCount;      //-1:未重传，非-1重传计数；
	u32 datalen;
	struct list_head list;
} AmBuffer;

typedef struct Buffer      //状态PDU对列，TM新数据队列
{
	FSM_PKT * pkt;
	struct list_head list;
} CtrlBuffer, TmBuffer;  

typedef struct UmBuffer          //UM数据队列
{
	FSM_PKT * pkt;
	PacketStatus pktstatus;
	u32 pos;
	//	unsigned int headerlen;
	SequenceNumber SN;
	struct list_head list;
} UmBuffer;

typedef  struct pduLeft
{
	short SN5_Left;
	short SN_Left ;
	FSM_PKT *sduLeft;
}pduLeft;

/*the struct of AM instance*/
typedef struct AM_Instance
{
	u16 rnti;
	u8 rbId;
	u8 lcId;
	u16 psn;
	bool statFlag;
	/** Timers in RLC AM mode **/
	evHandle pollRetxTimer;
	long  pollRetxTimerCode;
	Time  pollRetxTimerValue;

	evHandle reorderTimer;
	long reorderTimerCode;
	Time reorderTimerValue;

	evHandle statProhTimer;
	long statProhTimerCode;
	Time statProhTimerValue;
	
	u16 currentStatVar;
	SequenceNumber SN;
	struct pduLeft pduLft;
	/**buffers in RLC AM mode  **/
	/* Transimition buffer */
	struct AmSduBuffer amSduBuffer;
	u32 amSduBufferSize;
	u32 amSduBufferNum;

	struct AmBuffer amTransmittedBuffer;
	u32 amTransmittedBufferSize;
	u32 amTransmittedBufferNum;

	struct AmBuffer amRetxBuffer;
	u32 amRetxBufferSize;
	u32 amRetxBufferNum;

	CtrlBuffer ctrlPduBuffer;
	u32 ctrlPduBufferSize;
	u32 ctrlPduBufferNum;

	struct AmBuffer amRecvBuffer;

#define AM_WINDOW_SIZE 512
	/* transimitting window   */
	struct AmBuffer *amTranWindow[AM_WINDOW_SIZE*2];
	//unsigned int amTranSize;
	/** parameters of TxWindow of AM mode  **/
	SequenceNumber vt_s;
	SequenceNumber vt_a;
	SequenceNumber vt_ms;
	/* Reception window  */
	struct AmBuffer *amRecvWindow[AM_WINDOW_SIZE*2];
	//unsigned int amRecvSize;
	long segStart[segNumMax];
	long segEnd[segNumMax];
	/** parameters of RxWindow of AM mode **/
	SequenceNumber vr_mr;
	SequenceNumber vr_h;
	SequenceNumber vr_x;
	SequenceNumber vr_ms;
	SequenceNumber vr_r;
	/**  **/
	u16 pollPDU;    /*32PDU:组装的PDU数达到该值时触发轮询*/
	u16 pollBYTE;    /*1000KB:组装的PDU数达到该值时触发轮询*/
	u16 maxRetxThreshold;  /* 4:AM模式中PDU的最大重传次数*/
	u16 pduWithoutPoll;   /*组装的未带轮询位的pdu个数*/
	u16 byteWithoutPoll;   /*组装的未带轮询位的pdu字节数*/
	SequenceNumber pollSn;   /*携带轮询位的PDU的SN*/

	struct list_head amList;

} AM_Instance;

/* the struct of the transimit side of UM instance*/
typedef struct UM_TX_Instance
{
	u16 rnti;
	u8 rbid;
	u8 lcid;
	u16 psn;
	/** Timers on the transimitting side of  RLC UM mode **/
	struct UmBuffer umSduBuffer;
	u32 umSduBufferSize;
	u32 umSduBufferNum;
	SequenceNumber SN;

	u16 snFiledLength;     // Size5/size10 :UM 模式PDU中SN域所占的bit
	u8 CurrentStatVar;

	struct list_head umTxList;

} UM_TX_Instance;

/* the struct of the receiving side of UM instance*/
typedef struct UM_RX_Instance
{
	u16 rnti;
	u8 rbId;
	u8 lcId;
	u16 psn;
	u16 snFiledLength;     // Size5/size10 :UM 模式PDU中SN域所占的bit。
	/** Timers in RLC UM mode **/
	evHandle reorderTimer;
	long  reorderTimerCode;
	Time  reorderTimerValue;

	SequenceNumber SN;
	struct pduLeft  pduLft;
	struct UmBuffer umRecvBuffer;
	/* Reception window  */
#define UM_WINDOW_SIZE5 		16
#define UM_WINDOW_SIZE10	 	512
	struct UmBuffer *umRecvWindow5[UM_WINDOW_SIZE5 * 2];
	struct UmBuffer *umRecvWindow10[UM_WINDOW_SIZE10 * 2];
	u32 umRecvSize;
	/** parameters of RxWindow of UM mode **/
	SequenceNumber vr_ur;
	SequenceNumber vr_ux;
	SequenceNumber vr_uh;

	u8 currentStatVar;

	struct list_head umRxList;

} UM_RX_Instance;

/* the struct of the transimit side of TM instance*/
typedef struct TM_TX_Instance
{
	u16 rnti;
	u8 rbId;
	u8 lcId;
	u8 pbCh;
	//u8 currentStatVar ;

	/*  */
	TmBuffer tmSduBuffer;
	u32 tmSduBufferSize;
	u32 tmSduBufferNum;

	struct list_head tmTxList;

} TM_TX_Instance;


/* the struct of the receiving side of TM instance*/
typedef struct TM_RX_Instance
{
	u16 rnti ;
	u8 rbId ;
	u8 lcId ;
	u8 pbCh;
	//u8 currentStatVar ;

	struct list_head tmRxList;

} TM_RX_Instance;

/** structures for segmnet ,resegment, and assamble  **/
typedef struct _pduhdr
{	  
	u16 R1;
	u16 FIfirst;
	u16 FIsecond;
	u16 E;
	u16 SN;		   
}pduhdr;
	
typedef struct _ampduhdr
{	  
	u16 D_C;
	u16 RF;
	u16 P;
	u16 FIfirst;
	u16 FIsecond;
	u16 E;
	u16 SN;		   
}am_pduhdr;

typedef struct um5_pduhdr
{
	u8 FIfirst;
	u8 FIsecond;
	u8 E;
	u8 SN; 	 
}um5_pduhdr;

typedef struct _pduhdrExt
{	  
	u16 E[MAX_NUM];
	u16 LI[MAX_NUM];		   
}pduhExt;

typedef struct ctrlPduPara  //组装状态PDU用
{
	u16 ctrlHdr;  //每次填2字节数据，填好后放入sk_buffer中
	u16 ctrlHdrBits;  //记录ctrlHdr 已填的位数
	//u16 nackSnBits;   //记录NACN_SN已填的位数
	u32 pduLen;  //记录已填PDU字节数
	SequenceNumber sn;  //记录接收数组中空指针所对应的SN
	u32 size;  //记录要组装的状态PDU大小
} ctrlPduPara;

typedef struct ctrlPduPart1
{
	#if BYTE_ORDER == _LITTLE_ENDIAN
	unsigned : 1;
	unsigned E1: 1;
	unsigned ACK_SN: 10;
	unsigned CPT: 3;
	unsigned D_C: 1;
	#endif

	#if BYTE_ORDER == _BIG_ENDIAN
	unsigned D_C: 1;
	unsigned CPT: 3;
	unsigned ACK_SN: 10;
	unsigned E1: 1;
	unsigned : 1;
	#endif
} __attribute__((packed)) ctrlPduPart1;

typedef struct ctrlPduPart2
{
	#if BYTE_ORDER == _LITTLE_ENDIAN
	unsigned : 4;
	unsigned E2: 1;
	unsigned E1: 1;
	unsigned NACK_SN: 10;
	#endif

	#if BYTE_ORDER == _BIG_ENDIAN
	unsigned NACK_SN: 10;
	unsigned E1: 1;
	unsigned E2: 1;
	unsigned : 4;
	#endif
} __attribute__((packed)) ctrlPduPart2;

typedef struct ctrlPduPart3
{
	#if BYTE_ORDER == _LITTLE_ENDIAN
	unsigned : 2;
	unsigned SOend: 15;
	unsigned SOstart: 15;
	#endif

	#if BYTE_ORDER == _BIG_ENDIAN
	unsigned SOstart: 15;
	unsigned SOend: 15;
	unsigned : 2;
	#endif
} __attribute__((packed)) ctrlPduPart3;

/*** The codes of interruption ****/
/** IOCTL command **/
/* IOCTL between RLC and  RRC */
//RLC层ioctl宏定义从0x50开始
#define		CRLC_CONFIG_REQ				0x50
#define		CRLC_STATUS_IND				0x51
#define		CRLC_DEACT_REQ				0x52
#define		CRLC_SUSPEND_REQ			0x53
#define		CRLC_RESUME_REQ				0x54
#define		CRLC_BULID_REQ				0x55
//#define		CRLC_BULID_REQ				0x01

/* IOCTRL between RLC and PDCP */
#define		PRLC_DISC_REQ				0x56
#define		PRLC_AM_DATA_CNF			0x57

/* IOCTRL between RLC and MAC */
#define		MRLC_BUFREPORT_REQ			0x60
#define		MRLC_TRANSOP_IND			0x23

/** Event code in RLC layer **/
#define		_START_WORK					0x00

#define		SEGMENT						0x80
#define		RESEGMENT					0x81
#define		REASSAMBLE					0x82

#define		POLL						0x83
#define		CTRL_PKT_TRAN				0x84
#define		CTRL_PKT_RECV				0x85

#define         CTRL_RBID_BUILD         0x86

#define		PRINTRLC					0x87

/** Macro Definition **/
//#define RLC_PKT_FROM_LOWER  (fsm_ev_type()==FSM_EV_TYPE_PKT_IND)	//noted in 20150908
#define RLC_PKT_FROM_UPPER  (fsm_ev_type()==FSM_EV_TYPE_PKT_REQ)
#define RLC_EV_SELF (fsm_ev_type() == FSM_EV_TYPE_SELF)
//#define RLC_AM_PKT_TO_UPPER  (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == RLC_AM_REORDER_TIMEOUT)
//#define RLC_UM_PKT_TO_UPPER  (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == RLC_UM_REORDER_TIMEOUT)
//#define RLC_CTRLPKT_CREAT (fsm_ev_type() == FSM_EV_TYPE_SELF  && fsm_ev_code() ==  RLC_STATPRO_TIMEOUT)
//#define RLC_POLLRETX  (fsm_ev_type() == FSM_EV_TYPE_SELF  && fsm_ev_code() == RLC_POLLRETX_TIMEOUT )
#define RLC_OPEN  (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code()== FSM_EV_CORE_OPEN)
#define RLC_CLOSE  (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_CORE_CLOSE)
#define RLC_START_WORK  (fsm_ev_type() == FSM_EV_TYPE_SELF  && fsm_ev_code() ==  _START_WORK)
#define RLC_SUSPEND  (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == CRLC_SUSPEND_REQ)
#define RLC_RESUME  (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == CRLC_RESUME_REQ)
#define RLC_DEACT  (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == CRLC_DEACT_REQ)
//#define IOCTRL_ARRIVAL (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL)

#define RLC_SEGMENT (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == SEGMENT)
#define RLC_RESEGMENT (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == RESEGMENT)
#define RLC_REASSAMBLE (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == REASSAMBLE)

#define RLC_POLL (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == POLL)
#define RLC_CTRL_PKT_TRAN (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == CTRL_PKT_TRAN)
#define RLC_CTRL_PKT_RECV (fsm_ev_type() == FSM_EV_TYPE_SELF &&  fsm_ev_code() == CTRL_PKT_RECV)


//void rlc_main(void);
static void rlc_init_enter(void);
static void  rlc_close(void);
static void rlc_interupt(void);
static void rlc_pkt_receive_from_upper(void);
void rlc_pkt_receive_from_mac(FSM_PKT * skb);
static void rlc_AM_pkt_receive_from_mac(AM_Instance*amIns,FSM_PKT * pkt);
static void rlc_UM_pkt_receive_from_mac(UM_RX_Instance *umRxIns,FSM_PKT * pkt);
static void rlc_dispose(void);
static void rlc_ioctrl_handler(void);
static void * findInsByCode(long code, RlcTimerType * timer);


void  findInsByRbid(u8 rbid,void **up, void **down , RlcMode *mode) ;
void  findInsBylcid(u8 lcid, void **up, void**down, RlcMode *mode);
bool isInsideTranWin(struct AM_Instance *amIns, SequenceNumber SN);
bool isInsideAmRecvWin(struct AM_Instance * amIns, SequenceNumber SN);
bool isInsideUmRecvWin(struct UM_RX_Instance* umRxIns, SequenceNumber SN);
bool isOrderedUm(struct UM_RX_Instance* umRxIns, SequenceNumber SN);
u16 countSegInd(FSM_PKT *pkt);
void assamble_segment(AmBuffer *ambuf, FSM_PKT *pkt);

/***李栩龙: 分段，在分段和重组  ***/
extern FSM_PKT * rlc_UM_segment(struct UM_TX_Instance *um_tx_ins,u32 size);
extern FSM_PKT * rlc_AM_segment(struct AM_Instance *am_ins,u32 size);
extern FSM_PKT* rlc_resegment(struct AmBuffer *tbuff, u32 sizeFromMac);
extern void rlc_AM_reassemble_deliver(struct AM_Instance *amIns, struct pduLeft *pdu_left);
extern void rlc_UM_reassemble_deliver(struct UM_RX_Instance *umIns, struct pduLeft *pdu_left);
extern void rlc_UM5_reassemble_deliver(struct UM_RX_Instance *umIns, struct pduLeft *pdu_left);

/***张义: 轮询，重传，发送状态报告 ***/
extern void rlc_ctrl_pkt_trans(struct AM_Instance *amIns);
extern void rlc_poll_retx(struct AM_Instance *amIns);
extern void rlc_retx(struct AM_Instance *amIns,struct AmBuffer *amTxedBuffer);
extern void rlc_poll(struct AM_Instance *amIns, FSM_PKT * pkt);
extern void rlc_ctrl_pkt_recv(struct AM_Instance *amIns, FSM_PKT * pkt);

#define instance_create(type) ((type*)fsm_mem_alloc(sizeof(type)))


/******************** 合并相关 ***************************/
typedef struct  rlc_mac_sv{

	/**********************MAC状态变量****************************/
	u16  C_RNTI ;        // user ID
	u16  T_C_RNTI ; 	//20150120gai
	RACH_ConfigDedicated RA_info; 
	//unsigned char ra_preamble_index;
	//unsigned int  ra_PRACHMask_index;
	/*
	u32  frameNo ;       //system frame number
	u32  subframeNo ;    // subframe number for now
	*/ //modified LHL 20141105
	system_frame sys_frame;
	system_frame recv_frame;
	system_frame ue_schedule_frame;//20141112 modified LHL,在收到ULGRANT后调度使用，指示复用模块，复用模块使用后清空
	u32  PREAMBLE_TRANSMISSION_COUNTER ;  
	u32  SR_COUNTER ;   //SR transmission counter
	u32 backoff_index ; // use for random access
	u16 TA_fromRAR;//RAR消息中携带的Timing advance command  20140502
	u16 TA_value;//LHL MODIFIED 20141021,保留每次ENB更新的TA值
	u32 TA_Periodic_time;//LHL modifi20141021 保存RRC配置的TA定时器周期
	UL_resource  UL_resource_info; //information of the ul_grant
	HARQstatuInfo ULHARQ_status[ULHARQ_ProcessNum]; // the statu of HARQ process
	systemConfig  systemconfigInfo; //include RACHPCCH BCCH MacMain CCCH
	
	/*****************以下是lhl修改********************/
	//LogicalChannelConfigInfo *LC_info_ptr;
	LogicalChannelConfigInfo *LogicalChannel_Config;//hl更改，逻辑信道属性
	LogicalChannelBj *LogicalChannel_Bj;	//lhl更改,逻辑信道的Bj链表
	MacBufferStatus *MacBuffer_RLC; //lhl 改,MAC层保存并更新RLC层的请求报告
	MacBufferStatus_BSR_Info *Regularbsr;	//hl更改,常规BSR
	MacBufferStatus_BSR_Info *Periodicbsr;	//hl更改,周期BSR
	MacBufferStatus_BSR_Info *bsr; //lhl更改,BSR
	bool  LCGZeroToData; //=false 标志所有逻辑信道组数据从无到有
	bool  m_freshUlBsr; //=false 指示是否需要发送BSR(为true时不保证有资源)
	//bool  ALLOCATION_RESOURCE_BSR_INFO; //=false 设置标志表示是否有资源发bsr（为true时保证有资源)
	bool  DATA_WAIT_ALLOCATION;//=false 指标志是否有新的RLC报告请求
	u32 Rlc_Request_Num;//20140526modified，表示RLC请求的逻辑信道数量(数组的长度)
	/*****************lhl修改完成********************/
	//定时器
	Timer  TATimer; //systemconfigInfo.MACMainConfigInfo.timeAlignmentTimerDedicated 或者单独的命令
	Timer  CRTimer; //systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.mac_ContentionResolutionTimer
	Timer  RetxBSRTimer;//systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.retxBSR_Timer
	Timer  PeriodicBSRTimer;//systemconfigInfo.MACMainConfigInfo.ul_SCH_Config.periodicBSR_Timer
	Timer  WaitforRAresponseTimer;//systemconfigInfo.RACHCommonConfigInfo.raSupervisionInfo.raResponseWindowSize
	u32 	TA_timer_flag;//TA定时器是否取消过的标志，为了解决取消死机的问题 0:未取消过，1:取消过。每次开启时置为0，该取消时置为1
	//PHR还有两个定时器

	/**HeXi 20140423**/
	temp_contention_resolution *m_temp_cr;
	MAC_controlelement_tag  MAC_CE_Tags[8]; //UE MAC PDU has five kinds of MAC control units MAC PDU有5种控制单元
	SKBUFF* msg3_buf_ptr;
	//	bool contention_resolution_flag ;     
	//modified by lhl 20140716
	RA_Type ratype;
	u32 sendmsg3;//0:没发送MSG3，1:发送MSG3
	u32 pdcch_rnti;//0:PDCCH不是通过C_RNTI寻址，1:是通过C_RNTI寻址
	u32 Tbsize_Complex;//modified by lhl ,20140718,to 复用模块，可以发的总数据量
	u32 ra_allocation_res;//modified by lhl 20140725,指示当前随机接入是否为RLC request 分配了资源
	//0:当前随机接入没有为request 分配资源
	// 1:已分配资源，第一次发MSG3
	// 2:已分配资源，非第一次发MSG3
	//u32 flag123;
	u32 rar_failed_num;//20150104 联调添加变量，控制RAR失败次数
	u32 contention_failed_num;//20150104 联调添加变量，控制竞争解决失败次数
	
	u32 lost_pkt_num_uplink;//for test 
	u32 pkt_num_uplink;//for test
	u32 lost_pkt_num_downlink;//for test 
	u32 pkt_num_downlink;//for test
	
	CraQueuePtr CraQueue;
	/**********************RLC状态变量****************************/
	/* The type of current instance */
	RlcMode ins_mode;
	RlcTimerType timerType;
	long packetType ;  						//data packet or control packet
	u8 isBufferReq;                    		//指示是否发送缓存报告
	int allocCode[MAX_CODE];             	//保存未分配的code值
	struct  AM_Instance vAm_ins;            //AM实体链表
	struct  UM_RX_Instance vUm_rx_ins;		//UM接收侧实体链表
	struct  UM_TX_Instance vUm_tx_ins;		//UM发送侧实体链表
	struct  TM_RX_Instance vTm_rx_ins;		//TM接收侧实体链表
	struct  TM_TX_Instance vTm_tx_ins;		//TM发送侧实体链表

	evHandle bufferReqTimer;
	long  bufferReqTimerCode;
	Time  bufferReqTimerValue;
	//测试用AM 实体
	struct  AM_Instance amText;
	struct  UM_TX_Instance umText;
	long numflag;
	//for test
	u32 countreceive_from_mac;
	u32 countRecvFromUpper;
	u32 countSendToUpper;
	u32 countRecvFromLower;
	u32 countSentToLower;
	u32 countDropPacket;
	u32 countSendCtrlPdu;
	u32 countRecvCtrlPdu;
	u32 max_queue_num;
	
}rlc_mac_sv;  

/* MAC fsm state id definition  定义状态*/
#define ST_INIT 0
#define ST_IDLE 1
#define ST_CRA	2 /*contention-based random access process */
#define ST_NCRA	3
#define ST_CON  4
#define ST_SENDMSG3 5 //20140430 HQ

/** All statments in RLC FSM  **/
#define		ST_RLC_INIT		6
#define 	ST_NULL			7
#define		ST_TRAN 		8
#define		ST_SUSPEND		9
#define		ST_MAC_INIT		10

#define		ST_SUSPEND_IDLE		11
#define		ST_NULL_IDLE		12
void rlc_mac_main(void);

#endif
