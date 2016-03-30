#ifndef RRC_UTILS_H
#define RRC_UTILS_H

#include "../fsm/fsmdec.h"
#include "rrc_type.h"


typedef struct rrc_sv
{
	unsigned int hello_count;
} rrc_sv;

//store information on active signaling radio bearer instance
struct  LteSignalingRadioBearerInfo
{
	u8 srbStatus;           //hang up or not, 0 means not hang up
	u8 srbIdentity;         //0->SRB0, 1->SRB1, 2->SRB2
	struct RlcConfig rlcConfig;
	u8 lcid;                //logicalChannel id£º0->SRB0, 1->SRB1, 2->SRB2, 3->DRB0, 4->DRB1¡­
	struct LogicalChannelConfig logicalChannelConfig;  //logical Channel Config
};

// store information on active data radio bearer instance
struct LteDataRadioBearerInfo
{
	u8 drbStatus;
	u8 drbIdentity;
	struct RlcConfig rlcConfig;
	u8 lcid;
	struct LogicalChannelConfig logicalChannelConfig;
};



//ICI to RLC
typedef struct URLC_IciMsg
{
	//u16 rnti;    //unnecessary on UE
	u8 pbCh;    ////PBCh = 1:PCCH, PBCh = 2: BCCH, 0:CCCH
	u8 rbId;
	//u16 psn;
} __attribute__((packed))URLC_IciMsg;


/* the ICI information in the transimition between PDCP and RRC, IPADP*/
typedef struct UPDCP_IciMsg{
	u8 pbCh;     //PBCh = 1:PCCH, PBCh = 2: BCCH, 0:CCCH
	u8 rbId;
}__attribute__((packed)) UPDCP_IciMsg;



/* ---------------------------------------------------------------------------
   IOCTL message struct
   -------------------------------------------------------------------------- */
//RRC to MAC ioctl
typedef struct MAC_LogicalChannelConfig_IoctrlMsg
{
	int logicalChannelIdentity;
	struct LogicalChannelConfig logicalChannelConfig;
}__attribute__((packed));


//RRC to RLC ioctl
typedef struct T_UmUpParas
{
	u16 snFiledLength;     // Size5/size10 :number of bits of SN field in UM mode, this is for up link
};

typedef struct  T_UmDwParas
{
	u16 snFiledLength;     // Size5/size10 :number of bits of SN field in UM mode, this is for down link
	u32 timerReordering;  // 100ms:the size of t-reordering in UM mode
};

typedef struct T_AmUpParas
{
	u32 timerPollRetransmit;   //150ms:tPollRetransmit in AM mode
	u16 PollPDU;    
	u16 PollBYTE;    
	u16 maxRetxThreshold;  
};

typedef struct T_AmDwParas
{
	u32 timerStatusProhibit;  
	u32 timerReordering;   
};

typedef struct CRLC_ConfigReq_IoctrlMsg
{
	u8 rbIdentity;
	//    u8 configCause;      //0:establish£¬ 1: reconfig
	u8 lcIdentity;
	u16 Mode;
	struct T_UmDwParas umDwParas ;     // valid when Mode is 0
	struct T_UmUpParas umUpParas ;     // valid when Mode is 1
	struct T_AmDwParas amDwParas ;     // valid when Mode is 2 
	struct T_AmUpParas amUpParas ;     // valid when Mode is 3 
};



/* -----------------------
   IOCTL event code (defined and used by rrc's rrc_ioctl_handler())
   ----------------------- */
//ioctl from user space
//1. user space start cell access
#define IOCCMD_USTORRC_CELL_ACCESS  			0x01
//2. user space start connection request
#define IOCCMD_USTORRC_RRC_CONN_REQ     		0x02
//3. user space start connection release
#define IOCCMD_USTORRC_RRC_CONN_RELEASE     	0x03



//ioctl from rlc
#define IOCCMD_RLCTORRC_STATUS_IND      		0x50   //when arrive Maximum number of retransmissions  

//ioctl from mac
#define IOCCMD_MACTORRC_RANDOMACC_FAIL      	0x05  
#define IOCCMD_MACTORRC_REPORT_CRNTI        	0x06  //MAC tell RRC the C-RNTI from eNB



/* -----------------------
   IOCTL event code (defined and used by other layers' ioctl_handler)
   ----------------------- */
//RRC to USERSPACE
#define IOCCMD_RRCTOUS_RECV_PAGING  			0x01
#define IOCCMD_RRCTOUS_CONN_SUCCESS     		0x02
#define IOCCMD_RRCTOUS_RECV_NASINFO     		0x03
#define IOCCMD_RRCTOUS_RECV_RRC_CONN_RELEASE    0x04
#define IOCCMD_RRCTOUS_RECV_SIB1 				0x05
#define IOCCMD_RRCTOUS_RRC_CONN_FAIL 			0x06




//RRC to MAC
#define IOCCMD_RRCTOMAC_BEGIN_RECV_SYSINFO  	0x01
#define IOCCMD_RRCTOMAC_END_RECV_SYSINFO    	0x02
#define IOCCMD_RRCTOMAC_RESET   				0x03
#define IOCCMD_RRCTOMAC_RANDOMACC_REQ   		0x04
#define IOCCMD_RRCTOMAC_CONFIG_TAT  			0x05     //TimeAlignmentTimer 
#define IOCCMD_RRCTOMAC_CONFIG_RACH     		0x06     //RACH-ConfigCommon
#define IOCCMD_RRCTOMAC_CONFIG_BCCH     		0x07     //BCCH-Config
#define IOCCMD_RRCTOMAC_CONFIG_PCCH 			0x08     //PCCH-Config
#define IOCCMD_RRCTOMAC_CONFIG_MAC_MAIN     	0x09     //MAC-MainConfig

#define TEST_RECV_PADING 						0x45    //test paging


#define IOCCMD_RRCTOMAC_CONFIG_SPS  			0x10     //SPS-Config
#define IOCCMD_RRCTOMAC_CONFIG_LCC  			0x11     //LogicalChannelConfig
#define IOCCMD_RRCTOMAC_RELEASE_LCC     		0x12     //release LogicalChannel

//RRC to RLC
/*#define IOCCMD_RRCTORLC_BUILD      				0x29     //establish RLC entiy with rlc_config
#define IOCCMD_RRCTORLC_RECONFIG   				0x02     //reconfig RLC entity with infomation rlc_config
#define IOCCMD_RRCTORLC_RELEASE    				0x26     //release RLC entity
#define IOCCMD_RRCTORLC_SUSPEND    				0x27     //hang up RLC entity
#define IOCCMD_RRCTORLC_RESUME     				0x28*/
//new definition 20150907

#define IOCCMD_RRCTORLC_RECONFIG   				0x02     //reconfig RLC entity with infomation rlc_config
#define IOCCMD_RRCTORLC_RELEASE    				0x52     //release RLC entity
#define IOCCMD_RRCTORLC_SUSPEND    				0x53     //hang up RLC entity
#define IOCCMD_RRCTORLC_RESUME     				0x54
#define IOCCMD_RRCTORLC_BUILD      				0x55     //establish RLC entiy with rlc_config



/* -----------------------
   Define fsm_ev_code when fsm_ev_type() == FSM_EV_TYPE_SELF
   ----------------------- */
//ST_IDLE_CELL_SEL
#define CODE_BEGIN_RECV_SYSINFO     			0

//ST_IDLE_WAIT_SYSINFO
#define CODE_END_RECV_SYSINFO   				1

//ST_IDLE_NORMALLY
#define CODE_CONN_REQ_FROM_NAS  				2

//ST_IDLE_CONNECTING
#define CODE_TIMEOUT_T300   					3
#define CODE_RCV_RRC_CONN_REJECT 				4
#define CODE_RCV_RRC_CONN_SETUP 				5

//ST_CONNECTED_NORMALLY
#define CODE_RCV_RRC_CONN_RECONF    			6
#define CODE_RCV_RRC_CONN_RELEASE   			7

//ST_CONNECTED_RECONF
#define CODE_RECONF_SUCCESS     				8
#define CODE_RECONF_FAIL    					9

//ST_CONNECTED_REESTABLISH
#define CODE_RCV_RRC_CONN_REESTABLISHMENT   	10
#define CODE_RCV_RRC_CONN_REESTABLISH_REJECT    11
#define CODE_TIMEOUT_T301   					12
#define CODE_TIMEOUT_T311   					13

//other codes: those events won't cause state transmition
#define CODE_RCV_PAGING     					14
#define CODE_RCV_MIB    						15
#define CODE_RCV_SIB1   						16
#define CODE_RCV_SI     						17

#define CODE_TIMEOUT_T302   					18
#define CODE_TIMEOUT_T305   					19
#define CODE_TIMEOUT_T320   					20
#define CODE_TIMEOUT_T321   					21



/* -----------------------
   define Timers and interfaces to handle Timers
   ----------------------- */

#define TIMER_T300 			300
#define TIMER_T301 			301
#define TIMER_T302 			302
#define TIMER_T305 			305 
#define TIMER_T311 			311 
#define TIMER_T320 			320
#define TIMER_T321 			321

void setTimer(int timerNum, unsigned int delay);
void cancleTimer(int timerNum);
int checkTimer(int timerNum);
void check_free_ptr(void *ptr);



//defined in rrc_utils.c
extern struct LteSignalingRadioBearerInfo* ueSrbConfig[2];
extern struct LteDataRadioBearerInfo* ueDrbConfig[maxDRB];


/*functions to send messeges*/
void packet_send_to_pdcp(char *msg, int msg_len, u32 message_type);
void packet_send_to_upperlayer(void);



#endif
