#include <linux/list.h>
#ifndef _RLCTYPES_H
#define _RLCTYPES_H

/* RLC packet types definition*/
#define RLC_PKT_DATA  0
#define RLC_PKT_CMD   1

#define Time  u32
#define segNumMax 16
#define size5 5;
#define size10  10;
#define ms100 100*100
/* the ICI information in the transimition between RLC and RRC, PDCP*/
typedef struct URLC_IciMsg{
	u16 rnti; 
	u8 pbCh;     //PBCh = 1:PCCH, PBCh = 2: BCCH, 0:CCCH
	u8 rbId;
	//u16 psn;
}__attribute__((packed)) URLC_IciMsg;

/* the ICI information in the transimition between RLC and MAC*/
typedef enum PBCH{PCCH = 1, BCCH =2, ELSE = 0}PBCH;
/*
typedef struct MRLC_toMacPb_IciMsg{
	u8 type;           // type == 1
	u16 rnti;
	PBCH pbCh;     //PBCh = 1:PCCH, PBCh = 2: BCCH,  0:else
}__attribute__((packed)) MRLC_toMacPb_IciMsg;
*/
typedef struct MRLC_fromMac_IciMsg{
	//  u8 type;           // 1:type1, 2:type2
	u16 rnti;
	//  PBCH pbCh;     //PBCh = 1:PCCH, PBCh = 2: BCCH,  0:else
	u8 lcid;    
}__attribute__((packed)) MRLC_fromMac_IciMsg;

typedef struct MRLC_toMac_IciMsg{
	u16 rnti;
	PBCH pbCh;   //PBCh = 1:PCCH, PBCh = 2: BCCH,  0:else
	u16 len;
}__attribute__((packed)) MRLC_toMac_IciMsg;

typedef struct MRLC_subHead_fix_IciMsg{
	unsigned R1:1;
	unsigned R2:1;
	unsigned E:1;
	unsigned lcid:5;
}__attribute__((packed)) MRLC_subHead_fix_IciMsg;

typedef struct MRLC_subHead_sev_IciMsg{
	MRLC_subHead_fix_IciMsg fix;
	u8 fandl;	
}__attribute__((packed)) MRLC_subHead_sev_IciMsg;

typedef struct MRLC_subHead_fif_IciMsg{
	MRLC_subHead_fix_IciMsg fix;
	u16 fandl;
}__attribute__((packed)) MRLC_subHead_fif_IciMsg;

/****UM实体头部结构体****/
/* 序列号为5的UM 结构体*/
typedef struct RLC_UM_size5_head{
	unsigned FIfirst:1;
	unsigned FIsecond:1;
	unsigned E:1;
	unsigned SN:5;
}__attribute__((packed)) RLC_UM_size5_head;

/* 序列号为10的UM 结构体*/
typedef struct RLC_UM_size10_subhead{
	unsigned R11:1;
	unsigned R12:1;
	unsigned R13:1;
	unsigned FIfirst:1;
	unsigned FIsecond:1;
	unsigned E:1;
	unsigned short SN:10;
}__attribute__((packed)) RLC_UM_size10_subhead;
/* 序列号为10的UM 结构体， 方便字节序的调整*/
typedef struct RLC_UM_size10_head{
	unsigned short head;
}__attribute__((packed)) RLC_UM_size10_head;

/****AM实体头部结构体****/
/* AMD PDU 的头部结构体 */
typedef struct RLC_AMPDU_fixhead{
	unsigned DorC:1;
	unsigned RF:1;
	unsigned P:1;
	unsigned FIfirst:1;
	unsigned FIsecond:1;
	unsigned E:1;
	unsigned SN:10;
}__attribute__((packed)) RLC_AMPDU_fixhead;

typedef struct RLC_AMPDU_head{
	unsigned short head;
}__attribute__((packed)) RLC_AMPDU_head;

/* AMD PDU 分段的头部结构体 */
typedef struct RLC_AMPDU_seg_exthead{
	RLC_AMPDU_fixhead fhead;
	unsigned LSF:1;
	unsigned SO:15;
}__attribute__((packed)) RLC_AMPDU_seg_exthead;

typedef struct RLC_AMPDU_seg_head{
	unsigned int head;
}__attribute__((packed)) RLC_AMPDU_seg_head;

typedef struct RLC_AMDPDU_seg_ind{
	unsigned E:1;
	unsigned LI:15;
}__attribute__((packed))RLC_AMDPDU_seg_ind ;
/****** the IOCTRL information in the transimition between RLC and RRC********/
/* the IOCTRL information in the transimition between RLC and RRC*/

typedef struct T_UmUpParas{
	u16 snFiledLength;     // Size5/size10 :UM 模式PDU中SN域所占的bit。适用于上行UM模式。
}T_UmUpParas;

typedef struct  T_UmDwParas{
	u16 snFiledLength;     // Size5/size10 :UM 模式PDU中SN域所占的bit。适用于下行UM模式。
	Time timerReordering;  // 100ms:UM模式中t-reordering大小
}T_UmDwParas;

typedef struct T_AmUpParas{
	Time timerPollRetransmit;   //150ms:AM模式中tPollRetransmit
	u16 PollPDU;    //32PDU:组装的PDU数达到该值时触发轮询
	u16 PollBYTE;    //1000KB:组装的PDU数达到该值时触发轮询
	u16 maxRetxThreshold;   // 4:AM模式中PDU的最大重传次数
}T_AmUpParas;

typedef struct T_AmDwParas{
	Time timerStatusProhibit;   //150ms: AM模式中t-StatusProhibit大小
	Time timerReordering;   //100ms:AM模式中t-reordering大小
}T_AmDwParas;

	 
typedef struct CRLC_Config_IoctrlMsg{
	u16 rnti;
	u8 rbIdentity;
	//u8 configCause;   //0:新建，1:重配置
	u8 lcIdentity;
	u16 mode; // 1、2、3、4
	T_UmDwParas umDwParas ;
	T_UmUpParas umUpParas ;
	T_AmDwParas amDwparas ;
	T_AmUpParas amUpParas ;
}CRLC_ReConfigReq_IoctrlMsg,CRLC_BuildReq_IoctrlMsg;
/* the IOCTRL information in the transimition between RLC and RRC*/
typedef struct CRLC_StatusInd_IoctrlMsg{
	u16 rnti;
	u16 eventCode;//0,重传次数满
	u8 rbIdentity;
}CRLC_StatusInd_IoctrlMsg;

/* the IOCTRL information in the transimition between RLC and RRC*/
typedef struct CRLC_DeactReq_IoctrlMsg{
	u16 rnti;
	u8 rbIdentity;
}CRLC_DeactReq_IoctrlMsg;

/* the IOCTRL information in the transimition between RLC and RRC*/
typedef struct CRLC_SuspendReq_IoctrlMsg{
	u16 rnti;
	u8 rbIdentity;
}CRLC_SuspendReq_IoctrlMsg;

/* the IOCTRL information in the transimition between RLC and RRC*/
typedef struct CRLC_ResumeReq_IoctrlMsg{
	u16 rnti;
	u8 rbIdentity;
}CRLC_ResumeReq_IoctrlMsg;

/****** the IOCTRL information in the transimition between RLC and PDCP********/
/* the IOCTRL information in the transimition between RLC and PDCP*/
typedef struct PRLC_DiscReq_IoctrlMsg{
	u8 rbIdentity;
	u16 SN;
}PRLC_DiscReq_IoctrlMsg;

/* the IOCTRL information in the transimition between RLC and PDCP*/
typedef struct PRLC_AmDataCnf_IoctrlMsg{
	u8 rbIdentity;
	u16 firstCnfSN;
	u16 cnfPduNum;
}PRLC_AmDataCnf_IoctrlMsg;

/****** the IOCTRL information in the transimition between RLC and MAC********/
/* the IOCTRL information in the transimition between RLC and MAC*/
typedef struct MRLC_RT_IoctrlMsg{
	u16 frame;   //eNB侧添加成员
	u16 subframe;
	u16 rnti;  
	u8 lcid;  
	u32 txQueueSize;
	u16 txQueueHeader;  
	u32 retxQueueSize;  
	u16 retxQueueHeader; 
	u16 statusPduSize; 
	u16 statusPduHeader;
	//struct list_head next;
}__attribute__((packed))MRLC_ReportReq_IoctrlMsg, MRLC_TransOp_IoctrlMsg;

#endif
