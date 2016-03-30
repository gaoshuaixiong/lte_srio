/******************************************************************
** Filename: Rrc_type.h
** Copyright: uestc.
** Descriptions:
** v8.1.2,2014/10/07,wangjinchuan written
** -----------------------------------------------
** modificationhistory:
** Modified by:
** Modified date:
** Descriptions:
**
******************************************************************/

#ifndef RRC_TYPE
#define RRC_TYPE

#include "rrc_type_IEs.h"    //rrc information elements

//fix me:replace 'int' by 'u32' or 'long'.

/* -----------------------
rrc Message definition
----------------------- */
/* ------Paging------- */
struct S_TMSI {
	u8 mmec;
	u32 m_TMSI;
}__attribute__((packed));

struct UE_Identity{	
	u32 type;     //type=1:s_TMSI,  type=2:IMSI
	union {
		struct S_TMSI s_TMSI;
		u32 IMSI[21];
	}choice;
};

enum CN_Domain{		
	ps = 0,
	cs = 1
};

struct PagingRecord{	
	struct UE_Identity  ue_Identity;
  	enum CN_Domain cn_Domain;
};
struct PagingRecordList{
	u32 num;
	struct PagingRecord  pagingRecord[5];
};
struct Paging{
	bool havePagingRecord;
	struct PagingRecordList pagingRecordList;    //目前最多只有5个ue
	bool systemInfoModification;
	bool etws_Indication;
	bool havePaging_v890_IEs;
};



//System information
//MIB
struct MasterInformationBlock{	
	enum Dl_Bandwidth{
		n6=6, n15=15, n25=25, n50=50, n75=75, n100=100
	}dl_Bandwidth;
	struct PHICH_Config phich_Config;
	u8 systemFrameNumber;
	//u16 spare;
};
//SIB1
struct PLMN_Identity{
	u32 MCC[3];
	u32 MNC[3];
};

struct PlmnIdentityInfo{	
	struct PLMN_Identity plmn_Identity;
	enum CellReservedForOperatorUse{
		reserved, notReserved
	}cellReservedForOperatorUse;
};
struct CellAccessRelatedInfo{	
	struct PlmnIdentityInfo plmn_IdentityList[6];
	u16 trackingAreaCode;
	u32 cellIdentity;
	enum CellBarred{
		barred, notBarred
	}cellBarred;
	enum IntraFreqReselection{
		allowed, notAllowed
	}intraFreqReselection;
	bool csgIndication;
	u32 csgIdentity;	
};
struct CellSelectionInfo{
	long q_RxLevMin;            //INTEGER (-70..-22)
	u32 q_RxLevMinOffset;      //INTEGER (1..8)
};
enum SIB_Type{
	sibType3=3, sibType4, sibType5, sibType6,
	sibType7, sibType8, sibType9, sibType10,
	sibType11, sibType12_v920, sibType13_v920//未表示spare1~5
};
struct SchedulingInfo{
	enum SI_Periodicity{
		si_Periodicity_rf8=8, si_Periodicity_rf16=16, si_Periodicity_rf32=32, si_Periodicity_rf64=64, 
		si_Periodicity_rf128=128, si_Periodicity_rf256=256, si_Periodicity_rf512=512
	}si_Periodicity;
	enum SIB_Type sib_MappingInfo[32];
};

struct SystemInformationBlockType1{	
	struct CellAccessRelatedInfo cellAccessRelatedInfo;
	struct CellSelectionInfo cellSelectionInfo;
	bool haveP_Max;
	struct P_Max p_Max;
	u32 freqBandIndicator;     //INTEGER (1..64)
	struct SchedulingInfo schedulingInfoList;
	bool haveTDD_Config;
	struct TDD_Config tdd_Config;
	enum Si_WindowLength{
		si_WindowLength_ms1=1, si_WindowLength_ms2,  si_WindowLength_ms5=5, 
		si_WindowLength_ms10=10,  si_WindowLength_ms15=15,  si_WindowLength_ms20=20, si_WindowLength_ms40=40
	}si_WindowLength;
	u32 systemInfoValueTag;    //INTEGER (0..31)
	
};

//SIB2
struct FreqInfo{	
	u32 ulCarrierFreq;        //INTEGER (0..65535)
	enum UL_Bandwidth{
		ul_Bandwidth_n6=6, ul_Bandwidth_n15=15, ul_Bandwidth_n25=25, 
		ul_Bandwidth_n50=50, ul_Bandwidth_n75=75, ul_Bandwidth_n100=100
	}ul_Bandwidth;
	u32 additionalSpectrumEmission;//INTEGER (1..32)
};

struct AC_BarringConfig{		
	enum AC_BarringFactor{
		p00, p05=5, p10=10, p15=15, p20=20, p25=25, p30=30, p40=40,
		p50=50, p60=60, p70=70, p75=75, p80=80, p85=85, p90=90, p95=95
	}ac_BarringFactor;       //该值在[0,1)范围内取值，即：p00 = 0, p05 = 0.05, p10 = 0.10,…,p95 = 0.95
	enum AC_BarringTime{
		ac_BarringTime_s4=4, ac_BarringTime_s8=8, ac_BarringTime_s16=16, ac_BarringTime_s32=32,
		ac_BarringTime_s64=64, ac_BarringTime_s128=128, ac_BarringTime_s256=256, ac_BarringTime_s512=512
	}ac_BarringTime;
	u8 ac_BarringForSpecialAC;
};
struct AC_BarringInfo{
	bool ac_BarringForEmergency;
	struct AC_BarringConfig ac_BarringForMO_Signalling;
	struct AC_BarringConfig ac_BarringForMO_Data;
};


struct UE_TimersAndConstants{
	enum T300{
		t300_ms100=100, t300_ms200=200, t300_ms300=300, t300_ms400=400, 
		t300_ms600=600, t300_ms1000=1000, t300_ms1500=1500,t300_ms2000=2000
	}t300;
	enum T301{
		t301_ms100=100, t301_ms200=200, t301_ms300=300, t301_ms400=400, t301_ms600=600, 
		t301_ms1000=1000, t301_ms1500=1500,t301_ms2000=2000
	}t301;
	enum T310{
		t310_ms0, t310_ms50=50, t310_ms100=100, t310_ms200=200, t310_ms500=500, t310_ms1000=1000, t310_ms2000=2000
	}t310;
	enum N310{
		n310_n1=1,n310_n2,n310_n3,n310_n4,n310_n6=6,n310_n8=8,n310_n10=10,n310_n20=20
	}n310;
	enum T311{
		t311_ms1000=1000, t311_ms3000=3000, mt311_s5000=5000, t311_ms10000=10000, t311_ms15000=15000,
		t311_ms20000=20000, t311_ms30000=30000           //未表示spare1
	}t311;
	enum N311{
		n311_n1=1,n311_n2,n311_n3,n311_n4,n311_n5,n311_n6,n311_n8=8,n10=10
	}n311;
};
struct MBSFN_SubframeConfig{
	//NULL
};

struct SystemInformationBlockType2{		
	bool haveAC_BarringInfo;
	struct AC_BarringInfo ac_BarringInfo;
	struct RadioResourceConfigCommonSib radioResourceConfigCommon;
	struct UE_TimersAndConstants ue_TimersAndConstants;
	struct FreqInfo freqInfo;
	bool haveMBSFN_SubframeConfigList;
	struct MBSFN_SubframeConfig mbsfn_SubframeConfigList[8];
	struct TimeAlignmentTimer timeAlignmentTimerCommon;			
	
};

//SI
struct SystemInformation{	
	bool haveSib2;
	struct SystemInformationBlockType2 sib2;		
};




/* ------RRCConnectionestablish------- */
/*struct RRCConnectionRequest{	
	
	int type;     //type=1:s_TMSI,  type=2:randomValue
	union InitialUE_Identity{
		struct S_TMSI s_TMSI;
		u64 randomValue;         //BIT STRING (SIZE (40))
	}ue_Identity;
	
	enum EstablishmentCause{
		emergency=0, 
		highPriorityAccess=1, 
		mt_Access=2, 
		mo_Signalling=3, 
		mo_Data=4		
	} establishmentCause;	
};*/
// RRCConnectionRequest(modified)
	struct RRCConnectionRequest{	
		
		u8 type;	 //type=1:s_TMSI,  type=2:randomValue
		union InitialUE_Identity{
			struct S_TMSI s_TMSI;
			u8 randomValue;		 //BIT STRING (SIZE (40))
		}ue_Identity;
		
		enum EstablishmentCause{
			emergency=0, 
			highPriorityAccess=1, 
			mt_Access=2, 
			mo_Signalling=3, 
			mo_Data=4		
		} establishmentCause;	
	}__attribute__((packed));


struct RRCConnectionSetup{	
	u8 rrcTransactionIdentifier;
	struct RadioResourceConfigDedicated radioResourceConfigDedicated;	
};

struct RRCConnectionReject{	
	u32 waitTime;     //INTEGER (1..16), measure time by the second
};


struct RegisteredMME{	
	struct PLMN_Identity plmn_Identity;	
	u16 mmegi;
	u8 mmec;
};

struct RRCConnectionSetupComplete{
	u8 rrcTransactionIdentifier;
	u32 selectedPLMN_Identity;    //INTEGER (1..6),
	struct RegisteredMME registeredMME;
	//struct DedicatedInfoNAS dedicatedInfoNAS;       //do not use it now
};





/* ------RRCConnectionReconfiguration------- */
struct RRCConnectionReconfiguration {	
	u8 rrcTransactionIdentifier;
	//struct MeasConfig measConfig;
	//struct MobilityControlInfo mobilityControlInfo;
	//struct DedicatedInfoNAS dedicatedInfoNAS[maxDRB];
	bool haveRadioResourceConfigDedicated;
	struct RadioResourceConfigDedicated radioResourceConfigDedicated;
	//struct SecurityConfigHO securityConfigHO;
};

struct RRCConnectionReconfigurationComplete {	
	u8 rrcTransactionIdentifier;
};




/* ------RRCConnectionRelease------- */
//fix me:use for cell reselection priority number
#define MAX_CELL_RESELECTION_SIZE 5 

//use for storage cell reselection priority
struct CellReselectionPriority
{
	long carrierFreqs;
	u8 cellReselectionPriority;
};

struct IdleModeMobilityControlInfo
{
	bool haveCellReselectionPriorityInfo;
	struct  CellReselectionPriority  cellReselectionPriorityList[MAX_CELL_RESELECTION_SIZE];
	bool have_t320;
	enum T320_Value
	{
		min5=5,
		min10=10,
		min20=20,
		min30=30,
		min60=60,
		min120=120,
		min180=180
	}t320_Value;
};

struct RRCConnectionRelease
{
	u8 rrcTransactionIdentifier;
	enum RrcConectionReleaseCause
	{
		loadBalancingTAUrequired,
		other
	}releaseCause;
	bool haveIdleModeMobilityControlInfo;
	struct IdleModeMobilityControlInfo idleModeMobilityControlInfo;
};


/* -----------------------
Genetic Messaging
----------------------- */
typedef struct PCCH_Message{		
	struct Paging paging;
};

typedef struct BCCH_BCH_Message{	
	struct MasterInformationBlock mib;
};

typedef struct BCCH_DL_SCH_Message{		
	u32 type;
	union {
	/* type = 1 */
	struct SystemInformation si;
	/* type = 2 */
	struct SystemInformationBlockType1 sib1;
	} msg;	
};

typedef struct DL_CCCH_Message{	
	u32 type;
	union {
		/* type = 1 */
		//struct RRCConnectionReestablishment rrcConnectionReestablishment;
		/* type = 2 */
		//struct RRCConnectionReestablishmentReject rrcConnectionReestablishmentReject;
		/* type = 3 */
		struct RRCConnectionReject rrcConnectionReject;
		/* type = 4 */
		struct RRCConnectionSetup rrcConnectionSetup;
	} msg;	
};

typedef struct DL_DCCH_Message{	
	u32 type;
	union {
		/* type = 1 */
		struct RRCConnectionReconfiguration rrcConnectionReconfiguration;
		/* type = 2 */
		struct RRCConnectionRelease rrcConnectionRelease;
	} msg;		
};

typedef struct UL_CCCH_Message{		
	u32 type;
	union {
		/*type=1*/   
		//struct RRCConnectionReestablishmentRequest rrcConnectionReestablishmentRequest;
		/* type = 2 */
		struct RRCConnectionRequest rrcConnectionRequest;
	} msg;		
}__attribute__((packed));

typedef struct UL_DCCH_Message{	
	u32 type;
	union {
		/*type = 1*/
		//struct MeasurementReport measurementReport;
		/*type=2*/	
		struct RRCConnectionReconfigurationComplete rrcConnectionReconfigurationComplete;
		/*type=3*/	  
		//struct RRCConnectionReestablishmentComplete rrcConnectionReestablishmentComplete;
		/*type = 4*/
		struct RRCConnectionSetupComplete rrcConnectionSetupComplete;
	} msg;	
};


struct GbrQosInformation
{
  long gbrDl;  /**< Guaranteed Bit Rate (bit/s) in downlink */
  long gbrUl;  /**< Guaranteed Bit Rate (bit/s) in uplink */
  long mbrDl;  /**< Maximum Bit Rate (bit/s) in downlink */
  long mbrUl;  /**< Maximum Bit Rate (bit/s) in uplink */
};

struct AllocationRetentionPriority
{
  u8 priorityLevel;     // /< 1-15; 1 = highest
  int preemptionCapability; // /< 1 if bearer can preempt others
  int preemptionVulnerability; // 1 if bearer can be preempted by others
};


struct EpsBearer
{
  /**
   * QoS Class Indicator. See 3GPP 23.203 Section 6.1.7.2 for standard values.
   */
  enum Qci
  {
    GBR_CONV_VOICE          = 1,
    GBR_CONV_VIDEO          = 2,
    GBR_GAMING              = 3,
    GBR_NON_CONV_VIDEO      = 4,
    NGBR_IMS                = 5,
    NGBR_VIDEO_TCP_OPERATOR = 6,
    NGBR_VOICE_VIDEO_GAMING = 7,
    NGBR_VIDEO_TCP_PREMIUM  = 8,
    NGBR_VIDEO_TCP_DEFAULT  = 9,
  } qci;

  struct GbrQosInformation gbrQosInfo;
  struct AllocationRetentionPriority arp;
};



#endif

