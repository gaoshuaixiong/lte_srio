#include <linux/if_ether.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include "../fsm/fsmdec.h"
#include "../lte_system.h"
#include "mac_scheduler.h"
#include "queue_manager.h"
#include "macpkfmt.h"
#include "enb_macscheduler.h"
#include "rrc_config.h"
#include "Rlctypes.h"

#ifndef _RLCMACFSM_H
#define _RLCMACFSM_H

/***************** MAC 层相关声明和定义 ***********************/
#define NUM_RNTI 200
#define USERNUMMAX 10
#define DL_SCHEDULE_NUM 5


typedef struct PHY_TO_MAC_FRAME{
	u16    sfn; //             //系统帧号，0-1023
	u16    subframeN;//        //子帧号，0--9
} __attribute__((packed))PHY_TO_MAC_frame;
//LHL 20141124 MAC主配置
typedef struct system_Config
{ 
	struct MAC_MainConfig  MACMainConfigInfo ;
	struct RachConfigCommon  RACHCommonConfigInfo  ;
	struct PCCH_Config    PCCHConfigInfo;
	struct BCCH_Config    BCCHConfigInfo;
	struct SPS_Config     SPSConfigInfo;
	MAC_LogicalChannelConfig_IoctrlMsg  CCCHConfigInfo ; //增加CCCH的配置信息，默认配置
}systemConfig;

/* IOCTRL Command definition */
#define IOCCMD_RRCtoMAC_reset           	 0x01
#define IOCCMD_RRCtoMAC_LC_config          	 0x41 //携带参数 
#define IOCCMD_RRCtoMAC_LC_release           0x42 //携带参数 C_RNTI LCid 
#define IOCCMD_RRCtoMAC_UE_release           0x43 //携带参数 C_RNTI 释放该RNTI对应的UE的所有配置信息
#define IOCCMD_MACtoRRC_RNTI_indicate        0x31 //携带参数 C_RNTI 告知RRC RNTI值

#define IOCCMD_MACTOPHY_UL_DCI 0x12 //20141121
#define IOCCMD_MACTOPHY_DL_DCI 0x13
#define IOCCMD_MACTOPHY_RAR_DCI 0x14
/**** RLC与MAC合并后无需这两个IOCTL，注释掉20150804 ***/
#define IOCCMD_RLCtoMAC_datasend_Req         0x0B //携带数据 
#define IOCCMD_MACtoRLC_datasend_Allow       0x23 //携带数据 

#define IOCCMD_PHYtoMAC_RA_Req               0x03   //携带参数，RA-RNTI,RAPID(可能有多个),TA,
#define IOCCMD_PHYtoMAC_SR                   0x04   //物理层接收到某个UE的SR,携带参数C-RNTI
#define IOCCMD_PHYtoMAC_TA			0x06//物理层配置TA值给MAC，带参数UE_TA_INFO
#define IOCCMD_PHYtoMAC_FRAME			0x07//物理层配置帧号值给MAC，带参数PHY_TO_MAC_frame
#define IOCCMD_TEST_WITHDATA                 0x05  //测试带参数的IOCTL

/**********分割线***20150104联调测试代码，配置ENB不发RAR来演示错误处理*************/
#define _IOCTLtoENBMAC_RAR_Failed  0x70 //配置ENB MAC不发RAR
#define _IOCTLtoENBMAC_RAR_Send 0x71//配置ENB MAC发RAR,相当于撤销上一条命令
#define _IOCTLtoENBMAC_PKT_NUM 0x72
/**********分割线****************/

/*IOCRTL Command data structure*/
/*typedef struct MAC_LogicalChannelConfig_IoctrlMsg{
	u16 crnti;
	int logicalChannelIdentity;
	struct LogicalChannelConfig logicalChannelConfig;
};
*/

//LHL 20141011
/*typedef struct DL_TBSIZE_UE{//下行每个用户当前TTI可以发送的最大长度
	u16 rnti;//由耿灿锡资源分配时填充
	u32 tbsize;//由HX复用时使用，使用后清空为0
}DL_TBsize_ue;*///暂时使用这个schedule_scheme 中的tbsize

struct MAC_Release_LogicalChannel_IoctrlMsg{
	u16 crnti;
	int lcid;
}__attribute__((packed));

struct MAC_Release_Uefsm{
	u16 crnti;
}__attribute__((packed));

/* // RA请求的格式  暂时以S_RAinfo 为准 
typedef struct RA_req_info
{
	u16 ra_rnti;
	u8 rapid;//ra_preamble_index 暂时只实现为只含一个rapid
}__attribute__((packed))RA_req_info;
*/

/*self evens denition*/
#define TIME_TODO_ULSCHED  0x10  //该事件由定时器超时产生，表示可以进行上行调度
#define TIME_TODO_DLSCHED  0x11	 //由定时器超时产生,表示可以进行xia行调度 20150909
#define GET_SUBFRAME_NO    0x12
#define DLSCHED_PERIOD 	100		//DLSCHED_PERIOD is 100*10us=1ms
#define GET_SUBFRAMENO_PERIOD 100
/*condition definition 条件宏定义 */
#define MAC_PK_FROM_LOWER (fsm_ev_type()==FSM_EV_TYPE_PKT_IND)
#define	MAC_PK_FROM_UPPER (fsm_ev_type()==FSM_EV_TYPE_PKT_REQ)
#define MAC_OPEN (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code()== FSM_EV_CORE_OPEN)
#define MAC_CLOSE (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_CORE_CLOSE)
#define IOCTRL_ARRIVAL (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL)

#define MAC_RESET (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_RRCtoMAC_reset ) )
#define RA_REQ (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_PHYtoMAC_RA_Req ) )
//#define DL_SCHED_REQ (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_RLCtoMAC_datasend_Req ) )	//条件可能需要补充
//#define DL_SCHED_REQ (fsm_ev_type() == FSM_EV_TYPE_CORE && (fsm_ev_ioctrl_cmd()== IOCCMD_RLCtoMAC_datasend_Req ) ) 	//modify 20150831
#define DL_SCHED_REQ (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == TIME_TODO_DLSCHED)	//new definition 20150909
//#define UL_SCHED_REQ (IOCTRL_ARRIVAL && (fsm_ev_ioctrl_cmd()== IOCCMD_PHYtoMAC_SR ) )	//条件可能需要补充
#define UL_SCHED (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == TIME_TODO_ULSCHED)
#define CHECK_UL_SUBFRAME (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == GET_SUBFRAME_NO)

/***************** RLC 层相关声明和定义 ***********************/
#define SequenceNumber  short
#define MAX_NUM   16
#define  MAX_CODE   50
#define FLAG 2
#define PDU_MAX_SIZE 1400
//#define PDU_MAX_SIZE 1500

/* The types of instances in RLC layer  */
typedef enum {NONE_MODE,TM_MODE=1, UM_MODE=2, AM_MODE=3} RlcMode;
//typedef enum {TM_TX_MODE=1, TM_RX_MODE=2, UM_TX_MODE=3, UM_RX_MODE=4, AM_MODE=5} RlcMode;
typedef enum { POLL_RETX = 1, STAT_PRO = 2, AM_REORDER = 3, UM_REORDER = 4, BUFFER_REQ = 5, PKT_BUF = 6 /*, UL_SCHEDULE = 7*/ } RlcTimerType;
/** Data Structures Used By RLC **/
typedef enum { FULL_SDU = 1, LAST_SEGMENT = 2, ANY_SEGMENT = 3} PacketStatus;

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

typedef struct AmSduBuffer     //AM ÐÂÊýŸÝ¶ÓÁÐ
{
	FSM_PKT * pkt;
	PacketStatus  pktstatus;
	u32 pos;
	struct list_head  list;
} AmSduBuffer;

typedef struct AmBuffer  //ÒÑ·¢ËÍ¶ÓÁÐ£¬ÖØŽ«¶ÓÁÐ£¬œÓÊÜ²àÊýŸÝPDU¶ÓÁÐ
{
	FSM_PKT * pkt;
	PacketStatus  pktstatus;
	u32 pos;
	u32 headLen;
	long segStart[segNumMax];   //-1:ÎÞÐ§
	long segEnd[segNumMax];
	u32 segnum;
	SequenceNumber SN;
	short retxCount;      //-1:ÎŽÖØŽ«£¬·Ç-1ÖØŽ«ŒÆÊý£»
	u32 datalen;
	struct list_head  list;
} AmBuffer;

typedef struct Buffer      //×ŽÌ¬PDU¶ÔÁÐ£¬TMÐÂÊýŸÝ¶ÓÁÐ
{
	FSM_PKT * pkt;
	struct list_head  list;
} CtrlBuffer, TmBuffer;  

typedef struct UmBuffer          //UMÊýŸÝ¶ÓÁÐ
{
	FSM_PKT * pkt;
	PacketStatus  pktstatus;
	u32 pos;
	//	unsigned int headerlen;
	SequenceNumber SN;
	struct list_head  list;
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
	u16 rnti ;
	u8 rbId ;
	u8 lcId ;
	u16 psn;
	bool statFlag;
	/** Timers in RLC AM mode **/
	evHandle	 pollRetxTimer ;
	long  pollRetxTimerCode;
	Time  pollRetxTimerValue ;

	evHandle reorderTimer ;
	long  reorderTimerCode;
	Time  reorderTimerValue ;

	evHandle statProhTimer ;
	long  statProhTimerCode;
	Time  statProhTimerValue ;

	u16 currentStatVar ;
	SequenceNumber SN;
	struct pduLeft  pduLft;  //20140915ŒÓ
	/**buffers in RLC AM mode  **/
	/* Transimition buffer */
	struct  AmSduBuffer amSduBuffer ;
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
	u16 pollPDU;    /*32PDU:×é×°µÄPDUÊýŽïµœžÃÖµÊ±Ž¥·¢ÂÖÑ¯*/
	u16 pollBYTE;    /*1000KB:×é×°µÄPDUÊýŽïµœžÃÖµÊ±Ž¥·¢ÂÖÑ¯*/
	u16 maxRetxThreshold;  /* 4:AMÄ£ÊœÖÐPDUµÄ×îŽóÖØŽ«ŽÎÊý*/
	u16 pduWithoutPoll;   /*×é×°µÄÎŽŽøÂÖÑ¯Î»µÄpdužöÊý*/
	u16 byteWithoutPoll;   /*×é×°µÄÎŽŽøÂÖÑ¯Î»µÄpdu×ÖœÚÊý*/
	SequenceNumber pollSn;   /*Ð¯ŽøÂÖÑ¯Î»µÄPDUµÄSN*/

	struct list_head  amList;

} AM_Instance;

/* the struct of the transimit side of UM instance*/
typedef struct UM_TX_Instance
{
	u16 rnti ;
	u8 rbid ;
	u8 lcid ;
	u16 psn;
	/** Timers on the transimitting side of  RLC UM mode **/
	struct UmBuffer umSduBuffer ;
	u32 umSduBufferSize;
	u32 umSduBufferNum;
	SequenceNumber SN;

	u16 snFiledLength;     // Size5/size10 :UM Ä£ÊœPDUÖÐSNÓòËùÕŒµÄbit
	u8 CurrentStatVar ;

	struct list_head  umTxList;

} UM_TX_Instance;

/* the struct of the receiving side of UM instance*/
typedef struct UM_RX_Instance
{
	u16 rnti ;
	u8 rbId ;
	u8 lcId ;
	u16 psn;
	u16 snFiledLength;     // Size5/size10 :UM Ä£ÊœPDUÖÐSNÓòËùÕŒµÄbit¡£
	/** Timers in RLC UM mode **/
	evHandle reorderTimer ;
	long  reorderTimerCode;
	Time  reorderTimerValue ;

	SequenceNumber SN;
	struct pduLeft  pduLft;  //20140915ŒÓ
	struct UmBuffer umRecvBuffer ;
	/* Reception window  */
	#define UM_WINDOW_SIZE5 		16
	#define UM_WINDOW_SIZE10	 	512
	struct UmBuffer *umRecvWindow5[UM_WINDOW_SIZE5 * 2];
	struct UmBuffer *umRecvWindow10[UM_WINDOW_SIZE10 * 2];
	unsigned int umRecvSize;
	/** parameters of RxWindow of UM mode **/
	SequenceNumber vr_ur;
	SequenceNumber vr_ux;
	SequenceNumber vr_uh;

	u8 currentStatVar ;

	struct list_head  umRxList;

} UM_RX_Instance;

/* the struct of the transimit side of TM instance*/
typedef struct TM_TX_Instance
{
	u16 rnti ;
	u8 rbId ;
	u8 lcId ;
	u8 pbCh;
	//    u8 currentStatVar ;

	/*  */
	TmBuffer tmSduBuffer ;
	u32 tmSduBufferSize;
	u32 tmSduBufferNum;

 //   struct list_head  tmTxList;

} TM_TX_Instance;


/* the struct of the receiving side of TM instance*/
typedef struct TM_RX_Instance
{
	u16 rnti ;
	u8 rbId ;
	u8 lcId ;
	u8 pbCh;
	//   u8 currentStatVar ;
//   struct list_head  tmRxList;

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

 typedef struct um5_pduhdr  //20140915ŒÓ
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

/*typedef  struct pduLeft
{
      u16 SN_Left ;
      FSM_PKT *sduLeft;
}Pdu_left;*/

typedef struct RntiManag{
	u16 rnti;
	AM_Instance amIns;
	UM_TX_Instance  umTxIns;
	UM_RX_Instance  umRxIns;
	struct  TM_RX_Instance  ccchRecv; ///ŸÖ²¿CCCHµÄ¹ÜÀí£»
	struct  TM_TX_Instance  ccchTran;
	struct  list_head  list;
}RntiManag;


typedef struct packetBuf
{
	FSM_PKT * pkt;
	u32 count;
	struct list_head  list;
}packetBuf;
/* The status virables in RLC layer */
/*typedef struct rlc_sv
{
	
} rlc_sv;*/

/*** The codes of interruption ****/
/** IOCTL command **/
/* IOCTL between RLC and  RRC */
#define 		CRLC_CONFIG_REQ				0x24
#define 		CRLC_STATUS_IND				0x25
#define 		CRLC_DEACT_REQ				0x26
#define 		CRLC_SUSPEND_REQ			0x27
#define 		CRLC_RESUME_REQ				0x28
#define			CRLC_BULID_REQ				0x29

/* IOCTRL between RLC and PDCP */
#define 		PRLC_DISC_REQ				0x22
#define 		PRLC_AM_DATA_CNF			0x21

/* IOCTRL between RLC and MAC */
//#define 		MRLC_BUFREPORT_REQ			0x0B
//#define		MRLC_TRANSOP_IND			0x23

/** Event code in RLC layer **/
#define 		_START_WORK				0x00

#define			SEGMENT					0x31
#define 		RESEGMENT				0x32
#define 		REASSAMBLE				0x33

#define 		POLL					0x34
#define 		CTRL_PKT_TRAN				0x35
#define 		CTRL_PKT_RECV				0x36
#define         	CTRL_RBID_BUILD            		0x37

#define			PRINTRLC					0x0a

/** Macro Definition **/
#define RLC_PKT_FROM_LOWER  (fsm_ev_type()==FSM_EV_TYPE_PKT_IND)
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
//#define RLC_IOCTL_ARRIVAL (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL)

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
void rlc_AM_pkt_receive_from_mac(AM_Instance*amIns,FSM_PKT * pkt);
void rlc_UM_pkt_receive_from_mac(UM_RX_Instance *umRxIns,FSM_PKT * pkt);
static void rlc_dispose(void);
static void rlc_ioctrl_handler(void);
void rlc_AM_release(struct AM_Instance *amIns);
void rlc_UMrx_release(struct UM_RX_Instance* umRxIns);
void rlc_UMtx_release(struct UM_TX_Instance* umTxIns);
void  findInsByRbid(u16 rnti,u8 rbid,void **up, void **down , RlcMode *mode) ;
void  findInsBylcid(u16 rnti,u8 lcid, void **up, void**down, RlcMode *mode);
static void * findInsByCode(long code, RlcTimerType * timer);
RntiManag*findUeByRnti(u16 rnti);

bool isInsideTranWin(struct AM_Instance *amIns, SequenceNumber SN);
bool isInsideAmRecvWin(struct AM_Instance * amIns, SequenceNumber SN);
bool isInsideUmRecvWin(struct UM_RX_Instance* umRxIns, SequenceNumber SN);
bool isOrderedUm(struct UM_RX_Instance* umRxIns, SequenceNumber SN);
u16 countSegInd(FSM_PKT *pkt);
void assamble_segment(AmBuffer *ambuf, FSM_PKT *pkt);

/***ÀîÐñÁú: ·Ö¶Î£¬ÔÚ·Ö¶ÎºÍÖØ×é  ***/
extern FSM_PKT * rlc_UM_segment(struct UM_TX_Instance *um_tx_ins,u32 size);
extern FSM_PKT * rlc_AM_segment(struct AM_Instance *am_ins,u32 size);
extern FSM_PKT* rlc_resegment(struct AmBuffer *tbuff, u32 sizeFromMac);
extern void rlc_AM_reassemble_deliver(struct AM_Instance *amIns, struct pduLeft *pdu_left);
extern void rlc_UM_reassemble_deliver(struct UM_RX_Instance *umIns, struct pduLeft *pdu_left);
extern void rlc_UM5_reassemble_deliver(struct UM_RX_Instance *umIns, struct pduLeft *pdu_left);


/***ÕÅÒå: ÂÖÑ¯£¬ÖØŽ«£¬·¢ËÍ×ŽÌ¬±šžæ ***/
extern void rlc_ctrl_pkt_trans(struct AM_Instance *amIns);
extern void rlc_poll_retx(struct AM_Instance *amIns);
extern void rlc_poll(struct AM_Instance *amIns, FSM_PKT * pkt);
extern void rlc_ctrl_pkt_recv(struct AM_Instance *amIns, FSM_PKT * pkt);

#define instance_create(type) ((type*)fsm_mem_alloc(sizeof(type)))

/***************** RLC_MAC 合并相关声明和定义 ***********************/
typedef struct rlc_mac_sv
{
	struct list_head* ULschedQue_p;//上行调度，待分配上行资源的用户队列
	struct list_head* RAschedQue_p;//RA随机接入，待响应的用户队列
	int UL_sched_flag;// 1表示上行调度队列不为空，需要进行上行调度;0表示不需要调度
	int RA_sched_flag;//1//表示RA调度队列不为空，需要进行RA调度;0表示不需要调度
	int UL_resource_flag; //0表示上行资源已经分配完 1表示上行资源还有剩余 >0也可以表示剩余资源量
	int RAR_receive_crnti;//flag if there is C-RNTI at msg3;{0,1}
    struct rnti_array rntiarray[NUM_RNTI];
	int tc_rnti_notes[NUM_RNTI];//记录tc-rnti的实时情况
    struct result_sched_algorithm schedule_scheme[DL_SCHEDULE_NUM];
    
   // bool DATA_WAIT_ALLOCATION;
	int Rlc_Request_Num[USERNUMMAX];//20140624modified，表示RLC请求的逻辑信道数量(数组的长度)
	struct list_head * Enb_MacBuffer_RLC[USERNUMMAX];//20140623modify lhl,每个用户的数据指针头部
	struct list_head * Enb_LogicalChannel_Config[USERNUMMAX];
	struct list_head * Enb_LogicalChannel_Bj[USERNUMMAX];
	temp_contention_resolution *m_temp_cr;
//	complex_frame schedule_frame[DL_SCHEDULE_NUM];//LHL 20141017,由GCX调用填写，有何玺使用，并清空
	PHY_TO_MAC_frame phy_to_mac_frame;
//	u8 prb_bitmap2[100];
	u8 prb_bitmap7[100];
	u8 next_prb_bitmap2[100];
	u8 next_prb_bitmap7[100];
	u8 dl_bitmap[100];
//	struct rnti_array rntiarray[NUM_RNTI];
//	struct result_sched_algorithm schedule_scheme[DL_SCHEDULE_NUM];
//	DL_TBsize_ue TBsize_ue[USERNUMMAX];//LHL20141011,GCX调度后填充，HX复用时使用，初始化为0
	Ue_ta_info ue_ta[USERNUMMAX];//LHLmodified  用户的TA,由PHYIOCTL 更新，复用模块使用，并把flag置为0；
	systemConfig systemconfigInfo;
	int cnt_test;
	u32 send_rar_flag;//20150104联调添加的调试代码，初始为1表示要发RAR，收到用户空间IOCTL后为0，不发RAR
	
	u32 lost_pktnum_uplink;
	u32 total_pktnum_uplink;
	u32 lost_pktnum_downlink;
	u32 total_pktnum_downlink;
	
	/* The type of current instance,RLC layer SV */
	RlcMode ins_mode;
	RlcTimerType timerType;
	long 	packetType ;  //data packet or control packet
	u8 isBufferReq;                    //ÖžÊŸÊÇ·ñ·¢ËÍ»ºŽæ±šžæ
	long allocCode[MAX_CODE];             //±£ŽæÎŽ·ÖÅäµÄcodeÖµ
	struct pduLeft  pduLft;

	struct RntiManag rntiManag;
	struct  TM_TX_Instance  pcchIns; ///È«ŸÖPCCH ºÍBCCHµÄ¹ÜÀí£»
	struct  TM_TX_Instance  bcchIns;

	evHandle bufferReqTimer;
	long  bufferReqTimerCode;
	Time  bufferReqTimerValue;

	evHandle pktBufTimer;
	long  pktBufTimerCode;
	Time  pktBufTimerValue;

	evHandle check_subframe_no;	//add in 20160107

	packetBuf pktBuf;  //±£ŽæÃ»ÓÐ¶ÔÓŠÊµÌåµÄÊýŸÝ°ü
	u32 pktBufNum;
	u32 pktBufSize;
	u32 count; //for test
	u32 countRecvFromUpper;
	u32 countSendToUpper;
	u32 countRecvFromLower;
	u32 countSendToLower;
	u32 countDropPacket;
	u32 countSendCtrlPdu;
	u32 countRecvCtrlPdu;
	u32 max_queue_num;
	
	u32 newcountsendtoLower;
	int ue_num;		//the total number of access UE, 20151228;

	struct  AM_Instance amText;
	struct  UM_TX_Instance umText;
	
	//IMPORT u16 sfn;
	//IMPORT u16 subframeN; 	//add in 20151229
} rlc_mac_sv;

/** All statments in MAC FSM  **/
#define ST_INIT 0
#define	ST_IDLE 1
#define ST_RASCHED	2
#define ST_DLSCHED	3
#define ST_ULSCHED	4

/** All statments in RLC FSM  **/
#define		ST_RLC_INIT	5
#define 		ST_NULL		6
//#define		ST_TRAN 		7
#define 		ST_SUSPEND		8
#define		ST_MAC_INIT	9

void rlc_mac_main(void);

#endif
