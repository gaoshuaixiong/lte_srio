
#include "../lte_system.h"
//#include "mac_scheduler.h"
#include "queue_manager.h"
#include "Rlctypes.h" //add in 20150805
#ifndef _MACPKFMT_H
#define _MACPKFMT_H




/*UE interface information MAC-RLC */
//typedef enum PBCH{PCCH = 1, BCCH =2, ELSE = 0}PBCH;

typedef struct RLCtoMAC_IciMsg{
	u16 rnti; // UE sign
	PBCH pbCh;  //PBCh = 1 indicate PCCH ; PBCh = 2 indicate BCCH; 0:else      
	u16 len; // the length of the subheaders corresponding to the SDUs from RLC
}__attribute__((packed))RLCtoMAC_IciMsg;

typedef struct MACtoRLC_IciMsg{
	u16 rnti; // UE sign

	u8 lcid; //logical channel ID
}__attribute__((packed))MACtoRLC_IciMsg;
                   
/*typedef struct MACtoRLC_IciMsg{
	  u8 type;   // type =  2 
      u16 rnti; // UE sign
	  u8 lcid; //logical channel ID
}__attribute__((packed))MACtoRLC_IciMsg;

typedef struct RLC_MAC_IciMsgPB{  //ÎÊÌâ  ÕâÖÖICIÔÚUE²àÓ¦¸ÃÊÇÓÉMAC·¢¸øRLCµÄ
	  u8 type;      // type = 1
      u16 rnti;    // UE sign
	  PBCH pbCh;  //PBCh = 1 indicate PCCH ; PBCh = 2 indicate BCCH; 0:else
}__attribute__((packed))RLC_MAC_IciMsgPB;

*/
/*eNB²à RLC-MAC interface
typedef struct RLCtoMAC_IciMsg{
	  PBCH pbCh;  //PBCh = 1 indicate PCCH ; PBCh = 2 indicate BCCH; 0:else      
      u16 rnti; // UE sign
	  u16 len; // the length of the subheaders corresponding to the SDUs from RLC
}__attribute__((packed))RLCtoMAC_IciMsg;

typedef struct MACtoRLC_IciMsg{
      u16 rnti; // UE sign
	  u8 lcid; //logical channel ID
}__attribute__((packed))MACtoRLC_IciMsg;
*/

/*interface information from PHY-adapter */
typedef struct MACtoPHYadapter_IciMsg
{
	u16  frameNo ;       //system frame number
	u16  subframeNo ;    // subframe number for now
	u16 rnti ;         // UE ID
	u8 tcid;         // the type of Transport channel
    // u16 MessageType ; // the type of packet

}__attribute__((packed))MACtoPHYadapter_IciMsg;


typedef struct PHYtoMAC_Info //Ã¿¸öUE¶ÔÓ¦µÄ»ù±¾ÐÅÏ¢£¬ÓÃÓÚµ÷¶ÈÓÃµÄ£¬±ÈÈçRNTI£¬CQIµÈ
{
	u16    rnti;             //
	//u16    pgtype;           //0,1,2
	u16    sfn;              //ÏµÍ³Ö¡ºÅ£¬0¡ª1023
	u16    subframeN;        //×ÓÖ¡ºÅ£¬0¡ª¡ª9
	u16    crc;
	u16    harqindex;
	u8    harq_result;//20141008
	u16    sr;
	u16    cqi;
	u16    pmi;
	u16    ta;
   
}__attribute__((packed))PHYtoMAC_Info;


typedef struct PHYadaptertoMAC_IciMsg
{
     u8 tcid;         // the type of Transport channel
     //u16 MessageType ; // the type of packet 
     u16 rnti ; 
     PHYtoMAC_Info ue_info;    
}__attribute__((packed))PHYadaptertoMAC_IciMsg;

#define PCH 0
#define BCH 1
#define DLSCH 2
#define MCH 3


/*MAC¿ØÖÆµ¥Ôª¸ñÊ½ MAC_CE*/
/*shortBSR  longBSR C_RNTI UECR TACMD PW DRX */
/*************HeXi identify******************/
#define CLIENT_SIZE 5
//#define rapreambleID -1		//¶¨ÒåÒ»¸öÓ¦¸ÃÓÐµÄÈ«¾Ö±äÁ
#define LCG_SIZE 4
#define MACCE_ARRAY_size 8
//#define RLC_ICI_ARRAY_size 8
#define CCCH_LCID 0
#define CONTENTION_RESOLUTION_LCID 28
#define TIMING_ADVANCE_LCID 29
#define DRX_LCID 30
#define PADDING_LCID 31
#define PHR_LCID 26
#define C_RNTI_LCID 27
#define SHORT_BSR 29
#define LONG_BSR 30


typedef struct lteMacSubhead7Bit_s{
	u8 m_lcid_e_r_r;	//LCID 5bits,E 1bit,R 1bit
	u8 m_f_l;	//F 1bit,L 7bits
}__attribute__((packed))MAC_SDU_subhead_7bit;	//7bit×ÓÍ·

typedef struct lteMacSubhead15Bit_s{
	u8 m_lcid_e_r_r;	//LCID 5bits,E 1bit,R 1bit
	u8 m_f_l;	//F 1bit,L 7bits
	u8 m_l;		//L 8bits
}__attribute__((packed))MAC_SDU_subhead_15bit;	//15bit×ÓÍ·

typedef struct lteMacLastSubhead_s{
	u8 m_lcid_e_r_r;	//LCID 5bits,E 1bit,R 1bit
}__attribute__((packed))MAC_SDU_subhead_last;

typedef struct lteMacShortBsrControlSdu_s{
	u8 m_lcgid_buffersize;	//LCG ID 2bits,Buffer size 6bits
}__attribute__((packed))MAC_CE_shortBSR;	//¶ÌBSR

typedef struct lteMacLongBsrControlSdu_s{
	u8 m_buffersize1;	//Buffer Size #0~3 6bits
	u16  m_buffersize2;
}__attribute__((packed))MAC_CE_longBSR;	//³¤BSR

typedef struct lteMacC_rntiControlSdu_s{
	u16 m_crnti;	//C_RNTI 16bits
}__attribute__((packed))MAC_CE_Crnti;	//c_rnti¿ØÖÆµ¥Ôª

typedef struct lteMacUeContentionResolutionIdentityControlSdu_s{
	u32 m_resolusion_identity_1;	//UE Contention Resolusion Identity 48bits
	u16 m_resolusion_identity_2;
}__attribute__((packed))MAC_CE_content_resolution_identity;	//¾ºÕù½â¾öÈÏÖ¤¿ØÖÆµ¥Ôª

typedef struct lteMacTimingAdvanceCommandControlSdu_s{
	u8 m_r_r_ta;	//R 1bit,Timing Advance Command 6bits
}__attribute__((packed))MAC_CE_time_advance;	//¶¨Ê±ÌáÇ°ÃüÁî¿ØÖÆµ¥Ôª

typedef struct lteMacPowerHeadroomControlSdu_s{
	u8 m_r_r_ph;	//R 1bit,PH 6bits
}__attribute__((packed))MAC_CE_power_headroom;	//¹¦ÂÊÓàÁ¿¿ØÖÆµ¥Ôª

typedef struct lteMacPaddingSubhead_s{
	u8 m_lcid_e_r_r;	//LCID 5bits,E 1bit,R 1bit
}__attribute__((packed))MAC_SH_padding;	//padding×ÓÍ·²¿

typedef struct lteMacRarSdu_s{
	u8 m_r_ta;
	u8 m_ta_ulgrant;
	u16 m_ulgrant;
	u16 m_tcrnti;
}__attribute__((packed))MAC_RAR_sdu;	//RAR SDU

typedef struct lteMacRarSubheadWithBi_s
{
	u8 m_e_t_r_r_bi;
}__attribute__((packed))MAC_RAR_subhead_withbi;	//RAR ×ÓÍ·²¿

typedef struct lteMacRarSubhead_s{
	u8 m_e_t_rapid;
}__attribute__((packed))MAC_RAR_subhead;





/*struct Data_of_MacRarPdu{	
	u16    m_ta;
	u8     m_bi;
	u8     m_rapid;
	u32    m_ul_grant;
	u16    m_tcrnti;
	struct list_head m_listnode;
};*/

/**********HeXi 20140423*******************/
typedef struct MacCeListElements_s  // MAC·¢¸øeNBµÄBSR½á¹¹£¬È«¾Ö£¬ÔÚÃ¿´Î·ÖÅä×ÊÔ´ÒÔ¼°RLCÇëÇóµ½À´¶¼Òª¸üÐÂ( ÒòÎªÎÒÐèÒªÆäÖÐµÄÒ»Ð©±êÖ¾)
{
	uint16_t  m_rnti;	
	u32  m_lcgnum;//±êÖ¾ÓÐ¶àÉÙ¸öLCGÓÐÊý¾ÝÒª·¢ËÍ
	u32 m_TotalLcgData;
	u32  m_bufferStatus[4];
}__attribute__((packed))MacCeListElement_s;	//they should have been provided by Liu Hanli

/*typedef struct RRCConnectionRequest_s{	
	
	u32 type;     //pe=1:s_TMSI,  type=2:randomValue  //RLCÊµÏÖÊ±£¬×Ô¼ºÌí¼ÓµÄ×Ö¶Î
	union InitialUE_Identity{
		//struct S_TMSI s_TMSI;	//there is no such a struct in the .h profile but there should be one!!!!!
		long randomValue;         //BIT STRING (SIZE (40))
	}ue_Identity;
	
	enum EstablishmentCause{
		emergency=0, 
		highPriorityAccess=1, 
		mt_Access=2, 
		mo_Signalling=3, 
		mo_Data=4		
	} establishmentCause;	
}__attribute__((packed))RRCConnectionRequest;*/

typedef struct S_TMSI{
	u8 mmec;
	u32 m_TMSI;
}__attribute__((packed))S_TMSI;

typedef struct RRCConnectionRequest_s{	
	
	u8 type;     //pe=1:s_TMSI,  type=2:randomValue  //RLCÊµÏÖÊ±£¬×Ô¼ºÌí¼ÓµÄ×Ö¶Î
	union InitialUE_Identity{
		struct S_TMSI s_TMSI;	//there is no such a struct in the .h profile but there should be one!!!!!
		u8 randomValue[5];         //BIT STRING (SIZE (40))
	}ue_Identity;
	
	enum EstablishmentCause{
		emergency=0, 
		highPriorityAccess=1, 
		mt_Access=2, 
		mo_Signalling=3, 
		mo_Data=4		
	} establishmentCause;	
}__attribute__((packed))RRCConnectionRequest;

typedef struct temp_contention_resolution_s{
	u32 m_part_1;
	u16 m_part_2;
}__attribute__((packed))temp_contention_resolution;	//¾ºÕù½â¾öµ¥ÔªÄÚÈÝ ±ãÓÚÓëmsg4¶Ô±È

typedef struct UE_TA_INFO{
	u16 rnti;
	u16 update_flag;
	u16 ta;
}__attribute__((packed))Ue_ta_info;//LHL 20141011 modified 

/*****************HeXi 20140423 end **************/


/*************HeXi identify  end ******************/

/******* compelx and decomplex __HeXi 20140423*******/

void decomplexEndMacPdu(FSM_PKT *skb);
u32 lteMacGetUlHeadLength(FSM_PKT *skb);
void complexRarPdu(void * ue_infor);
void hasContentionResolutionSend(MAC_CE_content_resolution_identity *m_content_resolution);
u32 hasTimeAdvanceSend(u32 rnti);
void setMACtoPHY_IciMsg(MACtoPHYadapter_IciMsg *ici_msg,u32 tcid,u16 rnti);

void setMACtoRLC_IciMsg(MACtoRLC_IciMsg *ici_msg,u32 lcid,u32 rnti);
void dealCcchSdu(FSM_PKT *skb,u32 offset);
void complexEnbMacPdu(FSM_PKT *skb);
u32 getTBsize(u32 rnti);

u32 compare(u32 first,u32 second);
#endif



