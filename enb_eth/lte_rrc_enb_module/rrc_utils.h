#ifndef RRC_UTILS_H
#define RRC_UTILS_H

#include "../fsm/fsmdec.h"
#include "rrc_type.h"


#define MAX_UEFSM_NUM         5   //max number of ue fsms 
#define MAX_SRB_NUM 		  13  //max number of srb of each ue 
#define MAX_DRB_NUM 		  12  //max number of drb of each ue 
#define Max_Conn_Time             0

/* Í³¼Æ±äÁ¿¶¨Òå */
//³õÊ¼»¯ÔÚrrcfsm_enb_module.c
typedef struct rrc_sv_enb_ue{
	int uefsmid;
	u16 crnti;
} rrc_sv_enb_ue;

//¹ÜÀí³ÐÔØµÄÊý¾Ý½á¹¹
//store information on active signaling radio bearer instance
struct  LteSignalingRadioBearerInfo 
{
	u8 srbStatus;			//±êÊ¶ÊÇ·ñ¹ÒÆð£¬0±íÊ¾¹ÒÆð
	u8 srbIdentity;			//0->SRB0, 1->SRB1, 2->SRB2
	struct RlcConfig rlcConfig;
	u8 lcid;				//Âß¼­ÐÅµÀid£º0->SRB0, 1->SRB1, 2->SRB2, 3->DRB0, 4->DRB1¡­
 	struct LogicalChannelConfig logicalChannelConfig;  //Âß¼­ÐÅµÀÅäÖÃ
};

// store information on active data radio bearer instance
struct LteDataRadioBearerInfo 
{
	u8 drbStatus;
	u8 drbIdentity;
	int eps_BearerIdentity;
	struct RlcConfig rlcConfig;
	u8 lcid;
	struct LogicalChannelConfig logicalChannelConfig;
};

// defined in rrc_utils.c
extern struct LteSignalingRadioBearerInfo* enbSRBConfig[MAX_UEFSM_NUM][MAX_SRB_NUM];
extern struct LteDataRadioBearerInfo* enbDRBConfig[MAX_UEFSM_NUM][MAX_DRB_NUM];


/*
 *
 *	ICI informations
 *
 */

// the ICI information in the transimition between RLC and RRC
typedef struct URLC_IciMsg{
	u16 rnti;   
	u8 pbCh;    //1:PCCH, 2:BCCH, 0:others
	u8 rbId;
//	u16 psn;
}__attribute__((packed))URLC_IciMsg;

// the ICI information in the transimition between PDCP and RRC
typedef struct UPDCP_IciMsg{
	u16 rnti;
	u8 pbCh;     //PBCh = 1:PCCH, PBCh = 2: BCCH, 0:CCCH
	u8 rbId;
}__attribute__((packed)) UPDCP_IciMsg;


/*
 *
 *  IOCTL messages
 *
 */

//RRC to MAC ioctl msg(Logical Channel Config)
typedef struct MAC_LogicalChannelConfig_IoctrlMsg{
	u16 crnti;
	int logicalChannelIdentity;
	struct LogicalChannelConfig logicalChannelConfig;
};

typedef struct MAC_Release_LogicalChannel_IoctrlMsg{
	u16 crnti;
	int logicalChannelIdentity;
};


//RRC to RLC ioctl msg
typedef struct T_UmUpParas{
	u16 snFiledLength;     // Size5/size10 :UM Ä£Ê½PDUÖÐSNÓòËùÕ¼µÄbit¡£ÊÊÓÃÓÚÉÏÐÐUMÄ£Ê½¡£
};

typedef struct  T_UmDwParas{
	u16 snFiledLength;     // Size5/size10 :UM Ä£Ê½PDUÖÐSNÓòËùÕ¼µÄbit¡£ÊÊÓÃÓÚÏÂÐÐUMÄ£Ê½¡£
	u32 timerReordering;  // 100ms:UMÄ£Ê½ÖÐt-reordering´óÐ¡
};

typedef struct T_AmUpParas{
	u32 timerPollRetransmit;   //150ms:AMÄ£Ê½ÖÐtPollRetransmit
	u16 PollPDU;    //32PDU:×é×°µÄPDUÊý´ïµ½¸ÃÖµÊ±´¥·¢ÂÖÑ¯
	u16 PollBYTE;    //1000KB:×é×°µÄPDUÊý´ïµ½¸ÃÖµÊ±´¥·¢ÂÖÑ¯
	u16 maxRetxThreshold;   // 4:AMÄ£Ê½ÖÐPDUµÄ×î´óÖØ´«´ÎÊý
};

typedef struct T_AmDwParas{
	u32 timerStatusProhibit;   //150ms: AMÄ£Ê½ÖÐt-StatusProhibit´óÐ¡
	u32 timerReordering;   //100ms:AMÄ£Ê½ÖÐt-reordering´óÐ¡
};

typedef struct CRLC_ConfigReq_IoctrlMsg{
	  u16 crnti;      //±íÊ¾Ä¿±êue
      u8 rbIdentity;
//	  u8 configCause;      //0: ÐÂ½¨Ò»¸ö³ÐÔØ£¬ 1: ÖØÅäÖÃÒ»¸ö³ÐÔØ
	  u8 lcIdentity;
	  u16 Mode; 
	  struct T_UmDwParas umDwParas ;    // ModeÈ¡0Ê±£¬¸ÃÖµÓÐÐ§
	  struct T_UmUpParas umUpParas ;     // ModeÈ¡1Ê±£¬¸ÃÖµÓÐÐ§
	  struct T_AmDwParas amDwParas ;     // ModeÈ¡2Ê±£¬¸ÃÖµÓÐÐ§
	  struct T_AmUpParas amUpParas ;     // ModeÈ¡3Ê±£¬¸ÃÖµÓÐÐ§
};

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


/*
 *
 *  IOCTL event code (defined and used by other layers' ioctl_handler) 
 *
 */


//RRC to USERSPACE
#define IOCCMD_RRCTOUS_UE_CONN_REQ  0x01						//1.UEÔÚidleÌ¬,Óû½ÓÈëenbÊ±,ÉÏ±¨¸ß²ã
#define IOCCMD_RRCTOUS_UE_CONN_SUCCESS  0x02					//2.UEÁ¬½Ó½¨Á¢³É¹¦£¬½øÈëCONN_NORMALÌ¬,ÉÏ±¨¸ß²ã
#define IOCCMD_RRCTOUS_UE_RECONFIG_SUCCESS  0x03				//3.¶Ô¸ÃUEµÄÖØÅäÖÃ³É¹¦,ÉÏ±¨¸ß²ã

// Liu Yingtao 2014/10/11
#define IOCCMD_USTORRC_RECONFIG_START   0x60	   //reconfig start
#define IOCCMD_USTORRC_RECONFIG_ADDSRB  0x61		//add add_srb message  
#define IOCCMD_USTORRC_RECONFIG_ADDDRB  0x62		//add add_drb message
#define IOCCMD_USTORRC_RECONFIG_DELDRB  0x63		//add del_drb message
#define IOCCMD_USTORRC_RECONFIG_MACMAIN 0x64		//add mac_mainConfig message
#define IOCCMD_USTORRC_RECONFIG_SPS     0x65		//add sps_config message
//#define IOCCMD_USTORRC_RECONFIG_PHYCD   0x66		//add phyConfDedic message
#define IOCCMD_USTORRC_RECONFIG_STOP    0x67	   //reconfig stop
#define IOCCMD_USTORRC_RRC_CONN_RELEASE 0x68
#define IOCCMD_USTORRC_RRC_CONN_RECONFIG 0x69

#define IOCCMD_USTORRC_RRC_CONN_ACCEPT 	0x04       //µ±Ç°enb²àue×´Ì¬»úÃ»ÓÃµ½
//2.ÓÃ»§¿Õ¼ä¾Ü¾ø¸ÃUE½ÓÈë
#define IOCCMD_USTORRC_RRC_CONN_REJECT 	0x05       //µ±Ç°enb²àue×´Ì¬»úÃ»ÓÃµ½
//3.ÓÃ»§¿Õ¼ä·¢ÆðÁ¬½ÓÖØÅäÖÃ
//#define IOCCMD_USTORRC_CONN_RECONFIG 	0x06
//4.ÓÃ»§¿Õ¼ä·¢ÆðÁ¬½ÓÊÍ·Å
//#define IOCCMD_USTORRC_RRC_CONN_RELEASE 	0x07 
//5.ÓÃ»§¿Õ¼ä·¢ÆðÑ°ºô
#define IOCCMD_USTORRC_PAGING 0x12


//RRC to RLC
//#define IOCCMD_RRCTORLC_BUILD 	0x01     //½¨Á¢RLCÊµÌå,¿ÉÄÜÐ¯´ø²ÎÊýÄÚÈÝrlc_config
//#define IOCCMD_RRCTORLC_RECONFIG 	0x02     //Ð¯´ø²ÎÊýÄÚÈÝrlc_config
//#define IOCCMD_RRCTORLC_RELEASE 	0x03     //ÊÍ·ÅRLCÊµÌå
//#define IOCCMD_RRCTORLC_SUSPEND 	0x04     //¹ÒÆðRLCÊµÌå #define IOCCMD_RRCTORLC_RESUME 	    0x05     //»Ö¸´RLCÊµÌå
#define 		IOCCMD_RRCTORLC_RECONFIG			0x24
#define 		IOCCMD_RRCTORLC_STATUS				0x25
#define 		IOCCMD_RRCTORLC_DEACT				0x26
#define 		IOCCMD_RRCTORLC_SUSPEND				0x27
#define 		IOCCMD_RRCTORLC_RESUME				0x28
#define			IOCCMD_RRCTORLC_BUILD				0x29


//RRC to MAC
//#define IOCCMD_RRCTOMAC_UEFSM_BUILD_SUCCESS 	0x01     //eNB²àMACÊÕµ½¸ÃÃüÁîºó²ÅÉÏµÝMsg3
#define IOCCMD_RRCTOMAC_CONFIG_LCC 	    0x41    //config LogicalChannel,ÃüÁîÐ¯´øÊý¾Ý²ÎÊýMAC_LogicalChannelConfig_IoctrlMsg
#define IOCCMD_RRCTOMAC_RELEASE_LCC 	0x42    //release LogicalChannel,ÃüÁîÐ¯´øÊý¾ÝMAC_Release_LogicalChannel_IoctrlMsg
#define IOCCMD_RRCTOMAC_RELEASE_UEFSM 	0x43    //release uefsm, with crnti.
#define IOCCMD_RRCTOMAC_CONFIG_SPS      0x05
#define IOCCMD_RRCTOMAC_CONFIG_MAC_MAIN 0x06
//ioctl from mac
#define IOCCMD_MACTORRC_REPORT_CRNTI	 0x31  
//UE½ÓÈëENB, Ëæ»ú½ÓÈë³É¹¦ºóMACÉÏ±¨C-RNTI¸øRRC

//ioctl from rlc
#define IOCCMD_RLCTORRC_STATUS_IND	    		0x08     //ÖØ´«¼ÆÊýÆ÷³¬¹ý×î´óÖØ´«´ÎÊý£¬ÉÏ±¨

//system code used by rrcfsm_enb_ue
#define RRC_FSM_ENB_UE_OPEN	   0x10
#define RRC_FSM_ENB_UE_CLOSE	   0x11

//IOCTL event code (defined and used by rrcfsm_enb's rrc_ioctl_handler())
//ioctl from user space
//1.ÓÃ»§¿Õ¼ä¾Ü¾ø¸ÃUE½ÓÈë


/*
 *
 *	event code  
 *
 */

//fsm_post_msg
//pkcket event code (used by rrcfsm_enb_ue to distinguish different packet events)
#define PKT_RLC_IND_RRC_CONN_REQUEST 	0x01    //½ÓÊÕµ½Á¬½Ó½¨Á¢ÇëÇó±¨ÎÄ
#define PKT_RLC_IND_RRC_CONN_SETUP_COMPLETE 	0x02    //½ÓÊÕµ½Á¬½Ó½¨Á¢Íê³É±¨ÎÄ
#define PKT_RLC_IND_RRC_CONN_RECONFIG_COMPLETE 	0x03    //½ÓÊÕµ½Á¬½ÓÖØÅäÖÃÍê³É±¨ÎÄ


// Define fsm_ev_code when fsm_ev_type() == FSM_EV_TYPE_SELF 
//ST_INIT
#define CODE_RRC_OPEN 	0

//ST_IDLE
#define CODE_RCV_RRC_CONN_REQUEST 	1
#define CODE_SEND_MIB 8
#define CODE_SEND_SIB1 9
#define CODE_SEND_SI 10
#define CODE_SEND_PAGING 11

//ST_CONN_SETUP
#define CODE_RCV_RRC_CONN_SETUP_COMPLETE 	2
#define CODE_MAX_CONN_TIMEOUT 	3

//ST_CONN_NORMAL
#define CODE_RRC_CONN_RECONFIG 	4
#define CODE_RRC_CONN_RELEASE 	5

//ST_CONN_RECONFIG
#define CODE_RCV_RRC_CONN_RECONFIG_COMPLETE 	6

//ST_CONN_REESTABLISH 						
#define CODE_RCV_RRC_CONN_REESTABLISH 			7


static evHandle Max_Conn_Timeout = 0;       //Á¬½Ó³¬Ê±¼ÆÊ±Æ÷
static evHandle Send_MIB = 0;               //·¢ËÍMIB¼ÆÊ±Æ÷
static evHandle Send_SIB1= 0;				//·¢ËÍSIB1¼ÆÊ±Æ÷
static evHandle Send_SI= 0;					//·¢ËÍSI¼ÆÊ±Æ


//delayºÁÃëºó²úÉú¶¨Ê±ÊÂ¼þ,µ÷¶È±¾×´Ì¬»ú
void setTimer(int timerNum, int delay);

//ÈôtimerÎ´·¢Éú»ò²»Îª¿Õ,ÔòÈ¡Ïû¸ÃÊÂ¼þ
void cancleTimer(int timerNum);

void packet_send_to_rlc(char *msg, int msg_len, u32 message_type, u16 rnti);
void packet_send_to_pdcp(char *msg, int msg_len, u32 message_type, u16 rnti);


#endif
