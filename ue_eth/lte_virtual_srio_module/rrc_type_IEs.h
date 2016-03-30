
#ifndef RRC_TYPE_IEs
#define RRC_TYPE_IEs
//#include "stdint.h"
//#define bool int
//#define true 1
//#define false 0
//#define u8 int
//#define u16 int
//#define u32 int
//#define u64 int
#include <stdbool.h>    //引入c99中bool类型定义

//命名中的横线必须换成下划线

/* -----------------------
无线资源控制信息
----------------------- */

#define DRB_Identity int//INTEGER (1..32)
#define maxDRB 11
struct AntennaInfoCommon{
	enum AntennaPortsCount{
		an1=1,an2=2,an4=4   //未表示spare1
	} antennaPortsCount;
};

struct AntennaInformationDedicated {
	enum TransmissionMode{
		tm1=1,tm2=2,tm3=3,tm4=4,tm5=5,tm6=6,tm7=7//未表示spare1
	} transmissionMode;
	int type_codebookSubsetRestriction;    //1:n2TxAntenna_tm3......
	union CodebookSubsetRestriction{
		u8 n2TxAntenna_tm3;
		u8 n4TxAntenna_tm3;
		u8 n2TxAntenna_tm4;
		u64 n4TxAntenna_tm4;
		u8 n2TxAntenna_tm5;
		u16 n4TxAntenna_tm5;
		u8 n2TxAntenna_tm6;
		u16 n4TxAntenna_tm6;		
	} codebookSubsetRestriction;
	int type_ue_TransmitAntennaSelection;
	union Ue_TransmitAntennaSelection{
		int release;          
		enum Setup{
			closedLoop=0,
			openLoop=1
		} setup;
	} ue_TransmitAntennaSelection;
};


//RadioResourceConfigCommon
struct PreamblesGroupAConfig{
	enum SizeOfRA_PreamblesGroupA{
		n4=4, n8=8, n12=12, n16=16 ,n20=20, n24=24, n28=28,
		n32=32, n36=36, n40=40, n44=44, n48=48, n52=52, n56=56,n60=60
	}sizeOfRA_PreamblesGroupA;
	enum MessageSizeGroupA{
		b56=56, b144=144, b208=208, b256=256
	}messageSizeGroupA;
	enum MessagePowerOffsetGroupB{
		minusinfinity=-2, dB0=0, dB5=5, dB8=8, dB10=10, dB12=12,
		dB15=15, dB18=18
	}messagePowerOffsetGroupB;
};
struct PreambleInfo{	
	enum NumberOfRaPreambles{
		numberOfRA_Preambles_n4=4, numberOfRA_Preambles_n8=8, numberOfRA_Preambles_n12=12,
		numberOfRA_Preambles_n16=16, numberOfRA_Preambles_n20=20, numberOfRA_Preambles_n24=24, numberOfRA_Preambles_n28=28,
		numberOfRA_Preambles_n32=32, numberOfRA_Preambles_n36=36, numberOfRA_Preambles_n40=40, 
		numberOfRA_Preambles_n44=44, numberOfRA_Preambles_n48=48, numberOfRA_Preambles_n52=52, numberOfRA_Preambles_n56=56,
		numberOfRA_Preambles_n60=60, numberOfRA_Preambles_n64=64
	}numberOfRA_Preambles;
	bool havePreamblesGroupAConfig;
	struct PreamblesGroupAConfig preamblesGroupAConfig;
};
struct PowerRampingParameters{
	enum PowerRampingStep{
		powerRampingStep_dB0, powerRampingStep_dB2=2,powerRampingStep_dB4=4, powerRampingStep_dB6=6
	}powerRampingStep;
	enum PreambleInitialReceivedTargetPower{
		dBm_120=-120, dBm_118=-118, dBm_116=-116, dBm_114=-114, dBm_112=-112,
		dBm_110=-110, dBm_108=-108, dBm_106=-106, dBm_104=-104, dBm_102=-102,
		dBm_100=-100, dBm_98=-98, dBm_96=-96, dBm_94=-94,dBm_92=-92, dBm_90=-90
	}preambleInitialReceivedTargetPower;
};
struct RaSupervisionInfo{	
	enum PreambleTransMax{
		preambleTransMax_n3=3, preambleTransMax_n4, preambleTransMax_n5, preambleTransMax_n6, 
		preambleTransMax_n7,preambleTransMax_n8, preambleTransMax_n10=10, preambleTransMax_n20=20, preambleTransMax_n50=50,
		preambleTransMax_n100=100, preambleTransMax_n200=200
	}preambleTransMax;
	enum RaResponseWindowSize{
		sf2=2, sf3, sf4, sf5, sf6, sf7,sf8,sf10=10
	}raResponseWindowSize;
	enum Mac_ContentionResolutionTimer{
		mac_ContentionResolutionTimer_sf8=8, sf16=16, sf24=24, sf32=32, sf40=40, sf48=48,
		sf56=56, sf64=64
	}mac_ContentionResolutionTimer;
};

struct RachConfigCommon{//描述普通的随机接入参数	
	struct PreambleInfo preambleInfo;
	struct PowerRampingParameters powerRampingParameters;
	struct RaSupervisionInfo raSupervisionInfo;	
	int maxHARQ_Msg3Tx;  //INTEGER (1..8)
};
struct BCCH_Config{
	enum ModificationPeriodCoeff{
		modificationPeriodCoeff_n2=2, modificationPeriodCoeff_n4=4, modificationPeriodCoeff_n8=8, modificationPeriodCoeff_n16=16
	}modificationPeriodCoeff;
};
struct PCCH_Config{
	enum DefaultPagingCycle{
		rf32=32, rf64=64, rf128=128, rf256=256
	}defaultPagingCycle;
	enum NB{
		fourT, twoT, oneT,halfT, quarterT, 
		oneEighthT,oneSixteenthT, oneThirtySecondT//fourT 值对应4 * defaultPagingCycle,  twoT 值对应2 * defaultPagingCycle 等等
	}nB;
};
struct PRACH_ConfigInfo{
	int prach_ConfigIndex;        //INTEGER (0..63)
	bool highSpeedFlag;
	int zeroCorrelationZoneConfig;//INTEGER (0..15)
	int prach_FreqOffset;         //INTEGER (0..94)
};

struct PRACH_ConfigSIB{
	int rootSequenceIndex;        //INTEGER (0..837)
	struct PRACH_ConfigInfo prach_ConfigInfo;
};
struct PDSCH_ConfigCommon{
	int referenceSignalPower;    //INTEGER (-60..50)
	int p_b;	                 //INTEGER (0..3)
};
struct Pusch_ConfigBasic{
	int n_SB;                   //INTEGER (1..4)
	enum HoppingMode{
		interSubFrame, intraAndInterSubFrame
	}hoppingMode;
	int pusch_HoppingOffset;   //INTEGER (0..98)
	bool enable64QAM;
};
struct UL_ReferenceSignalsPUSCH{
	bool groupHoppingEnabled;
	int groupAssignmentPUSCH; //INTEGER (0..29)
	bool sequenceHoppingEnabled;
	int cyclicShift;          //INTEGER (0..7)
};
struct PUSCH_ConfigCommon{
	struct Pusch_ConfigBasic pusch_ConfigBasic;
	struct UL_ReferenceSignalsPUSCH ul_ReferenceSignalsPUSCH;
};
struct PUCCH_ConfigCommon{
	enum DeltaPUCCH_Shift{
		ds1=1, ds2, ds3
	}deltaPUCCH_Shift;
	int nRB_CQI;             //INTEGER (0..98)
	int nCS_AN;              //INTEGER (0..7)
	int n1PUCCH_AN;          //INTEGER (0..2047)
};
struct SoundingRS_UL_ConfigCommonstruct_setup{
	enum SrsBandwidthConfig{
		bw0, bw1, bw2, bw3, bw4, bw5, bw6, bw7
	}srsBandwidthConfig;
	enum SrsSubframeConfig{
		sc0, sc1, sc2, sc3, sc4, sc5, sc6, sc7,
		sc8, sc9, sc10, sc11, sc12, sc13, sc14, sc15
	}srsSubframeConfig;
	bool ackNackSRS_SimultaneousTransmission;
	bool srsMaxUpPts;
		
};
struct SoundingRS_UL_ConfigCommon{
	int type;      //1:release,  2:setup
	union {
		int release;
		struct SoundingRS_UL_ConfigCommonstruct_setup setup;
	}choice;
};
struct DeltaFList_PUCCH{
	enum DeltaF_PUCCH_Format1{
		eltaF_2=-2, deltaF0=0, deltaF2=2
	}deltaF_PUCCH_Format1;
	enum EltaF_PUCCH_Format1b{
		deltaF1=1, deltaF3=3, deltaF5=5
	}eltaF_PUCCH_Format1b;
	enum deltaF_PUCCH_Format2{
		deltaF_PUCCH_Format2_deltaF_2=-2, deltaF_PUCCH_Format2_deltaF0=0, deltaF_PUCCH_Format2_deltaF1=1, deltaF_PUCCH_Format2_deltaF2=2
	}deltaF_PUCCH_Format2;
	enum DeltaF_PUCCH_Format2a{
		deltaF_PUCCH_Format2a_deltaF_2=-2, deltaF_PUCCH_Format2a_deltaF0=0, deltaF_PUCCH_Format2a_deltaF2=2
	}deltaF_PUCCH_Format2a;
	enum deltaF_PUCCH_Format2b{
		deltaF_PUCCH_Format2b_deltaF_2=-2, deltaF_PUCCH_Format2b_deltaF0=0, deltaF_PUCCH_Format2b_deltaF2=2
	}deltaF_PUCCH_Format2b;
};
struct UplinkPowerControlCommon{
	int p0_NominalPUSCH;     //INTEGER (-126..24)
	enum Alpha{
		al0=0, al04=4, al05=5, al06=6, al07=7, al08=8, al09=9, al1=10//其中 al0 对应值0，al04 对应值0.4，al05 对应 0.5， al06 对应0.6，al07 对应0.7， al08 对应0.8，al09 对应0.9 以及al1 对应1
	}alpha;
	int p0_NominalPUCCH;     //INTEGER (-127..-96)
	struct DeltaFList_PUCCH deltaFList_PUCCH;
	int deltaPreambleMsg3;   //INTEGER (-1..6)
};
struct UL_CyclicPrefixLength{
	enum {
		len1=1, len2
	}ul_CyclicPrefixLengthtype;
};
struct PHICH_Config{
	enum Phich_Duration{
		normal, extended   
	}phich_Duration;
	enum Phich_Resource{
		oneSixth=0, half=5, one=1, two=2//值 oneSixth对应1/6, half 对应1/2等等
	}phich_Resource;
};
struct P_Max{
	int pmax;              //INTEGER (_30..33)
};
struct TDD_Config{
	enum SubframeAssignment{
		sa0, sa1, sa2, sa3, sa4, sa5, sa6
	}subframeAssignment;
	enum SpecialSubframePatterns{
		ssp0, ssp1, ssp2, ssp3, ssp4,ssp5, ssp6, ssp7,ssp8
	}specialSubframePatterns;	
};

struct RadioResourceConfigCommonSib{ 
	struct RachConfigCommon rachConfigCommon;
	struct BCCH_Config bcch_Config;
	struct PCCH_Config pcch_Config;
	struct PRACH_ConfigSIB prach_Config;
	struct PDSCH_ConfigCommon pdsch_ConfigCommon;
	struct PUSCH_ConfigCommon pusch_ConfigCommon;
	struct PUCCH_ConfigCommon pucch_ConfigCommon;
	struct SoundingRS_UL_ConfigCommon soundingRS_UL_ConfigCommon;
	struct UplinkPowerControlCommon uplinkPowerControlCommon;
	struct UL_CyclicPrefixLength ul_CyclicPrefixLength;
};
struct RadioResourceConfigCommon{
	bool haveRachConfigCommon;
    struct RachConfigCommon rachConfigCommon;
	struct PRACH_ConfigSIB prach_Config;
	bool havePDSCH_ConfigCommon;
	struct PDSCH_ConfigCommon pdsch_ConfigCommon;
	struct PUSCH_ConfigCommon pusch_ConfigCommon;
	bool havePHICH_Config;
	struct PHICH_Config phich_Config;
	bool havePUCCH_ConfigCommon;
	struct PUCCH_ConfigCommon pucch_ConfigCommon;
	bool haveSoundingRS_UL_ConfigCommon;
	struct SoundingRS_UL_ConfigCommon soundingRS_UL_ConfigCommon;
	bool haveUplinkPowerControlCommon;
	struct UplinkPowerControlCommon uplinkPowerControlCommon;
	bool haveAntennaInfoCommon;
	struct AntennaInfoCommon antennaInfoCommon;
	bool haveP_Max;
	struct P_Max p_Max;
	bool haveTDD_Config;
	struct TDD_Config tdd_Config;
	bool haveUl_CyclicPrefixLength;
	struct UL_CyclicPrefixLength ul_CyclicPrefixLength;
};


//RadioResourceConfigDedicated
struct T_PollRetransmit {
	enum {
		ms5=5, ms10=10, ms15=15, ms20=20, ms25=25, ms30=30, ms35=35,
		ms40=40, ms45=45, ms50=50, ms55=55, ms60=60, ms65=65, ms70=70,
		ms75=75, ms80=80, ms85=85, ms90=90, ms95=95, ms100=100, ms105=105,
		ms110=110, ms115=115, ms120=120, ms125=125, ms130=130, ms135=135,
		ms140=140, ms145=145, ms150=150, ms155=155, ms160=160, ms165=165,
		ms170=170, ms175=175, ms180=180, ms185=185, ms190=190, ms195=195,
		ms200=200, ms205=205, ms210=210, ms215=215, ms220=220, ms225=225,
		ms230=230, ms235=235, ms240=240, ms245=245, ms250=250, ms300=300,
		ms350=350, ms400=400, ms450=450, ms500=500 //、、、、、、spare1~9未表示
	} t_PollRetransmittype;
};
struct PollPDU {
	enum {
		p4=4, p8=8, p16=16, p32=32, p64=64, p128=128, p256=256, pInfinity=-1
	} pollPDUtype;
};
struct PollByte {
	enum {
		kB25=25, kB50=50, kB75=75, kB100=100, kB125=125, kB250=250, kB375=375,
		kB500=500, kB750=750, kB1000=1000, kB1250=1250, kB1500=1500, kB2000=2000,
		kB3000=3000, kBinfinity=-1                  
	} pollByte;
};
struct UL_AM_RLC{
	struct T_PollRetransmit t_PollRetransmit;
	struct PollPDU pollPDU;
	struct PollByte pollByte;
	enum MaxRetxThreshold {
		t1=1, t2=2, t3=3, t4=4, t6=6, t8=8, t16=16, t32=32
	} maxRetxThreshold;
};
struct T_Reordering {
	enum {
		t_Reordering_ms0=0, t_Reordering_ms5=5, t_Reordering_ms10=10, t_Reordering_ms15=15, 
		t_Reordering_ms20=20, t_Reordering_ms25=25, t_Reordering_ms30=30, t_Reordering_ms35=35,
		t_Reordering_ms40=40, t_Reordering_ms45=45, t_Reordering_ms50=50, t_Reordering_ms55=55, mt_Reordering_s60=60, t_Reordering_ms65=65, t_Reordering_ms70=70,
		t_Reordering_ms75=75, t_Reordering_ms80=80, t_Reordering_ms85=85, t_Reordering_ms90=90, t_Reordering_ms95=95, t_Reordering_ms100=100, t_Reordering_ms110=110,
		t_Reordering_ms120=120, t_Reordering_ms130=130, t_Reordering_ms140=140, t_Reordering_ms150=150, t_Reordering_ms160=160, t_Reordering_ms170=170,
		t_Reordering_ms180=180, t_Reordering_ms190=190, t_Reordering_ms200=200               //未表示spare1=0spare1
	} t_Reordering;
};
struct T_StatusProhibit{
	enum {
		t_StatusProhibit_ms0=0, t_StatusProhibit_ms5=5, t_StatusProhibit_ms10=10, t_StatusProhibit_ms15=15,
		t_StatusProhibit_ms20=20, t_StatusProhibit_ms25=25,t_StatusProhibit_ms30=30,t_StatusProhibit_ms35=35,
		t_StatusProhibit_ms40=40, t_StatusProhibit_ms45=45, t_StatusProhibit_ms50=50, t_StatusProhibit_ms55=55, 
		t_StatusProhibit_ms60=60, t_StatusProhibit_ms65=65, t_StatusProhibit_ms70=70,
		t_StatusProhibit_ms75=75, t_StatusProhibit_ms80=80, t_StatusProhibit_ms85=85, t_StatusProhibit_ms90=90, t_StatusProhibit_ms95=95, t_StatusProhibit_ms100=100, t_StatusProhibit_ms105=105,
		t_StatusProhibit_ms110=110, t_StatusProhibit_ms115=115, t_StatusProhibit_ms120=120, t_StatusProhibit_ms125=125, t_StatusProhibit_ms130=130, t_StatusProhibit_ms135=135,
		t_StatusProhibit_ms140=140, t_StatusProhibit_ms145=145, t_StatusProhibit_ms150=150, t_StatusProhibit_ms155=155, t_StatusProhibit_ms160=160, t_StatusProhibit_ms165=165,
		t_StatusProhibit_ms170=170, t_StatusProhibit_ms175=175, t_StatusProhibit_ms180=180, t_StatusProhibit_ms185=185, t_StatusProhibit_ms190=190, t_StatusProhibit_ms195=195,
		t_StatusProhibit_ms200=200, t_StatusProhibit_ms205=205, t_StatusProhibit_ms210=210, t_StatusProhibit_ms215=215, t_StatusProhibit_ms220=220, t_StatusProhibit_ms225=225,
		t_StatusProhibit_ms230=230, t_StatusProhibit_ms235=235, t_StatusProhibit_ms240=240, t_StatusProhibit_ms245=245, t_StatusProhibit_ms250=250, t_StatusProhibit_ms300=300,
		t_StatusProhibit_ms350=350, t_StatusProhibit_ms400=400, t_StatusProhibit_ms450=450, t_StatusProhibit_ms500=500 //、、、、、、spare1~9未表示
	} t_StatusProhibit;
};
struct DL_AM_RLC {
	struct T_Reordering t_Reordering;
	struct T_StatusProhibit t_StatusProhibit;
};
struct RLC_Config_am {
   struct UL_AM_RLC  ul_AM_RLC;
   struct DL_AM_RLC  dl_AM_RLC;

};
struct SN_FieldLength{
	enum {
		size5=5,size10=10
	}sn_FieldLength;
};
struct UL_UM_RLC{
	struct SN_FieldLength sn_FieldLength;
};
struct DL_UM_RLC{
	struct SN_FieldLength sn_FieldLength;
	struct T_Reordering t_Reordering;
};
struct RLC_Config_um_Bi_Directional{	
	struct UL_UM_RLC ul_UM_RLC;
	struct DL_UM_RLC dl_UM_RLC;
};

struct RLC_Config_um_Uni_Directional_UL{	
	struct UL_UM_RLC ul_UM_RLC;
};

struct RLC_Config_um_Uni_Directional_DL{	
	struct DL_UM_RLC dl_UM_RLC;
};

struct RlcConfig{//描述SRB和DRB的RLC配置
	int type;
	union {
      /* type = 1 */
      struct RLC_Config_am am;
      /* type = 2 */
      struct RLC_Config_um_Bi_Directional um_Bi_Directional;
      /* type = 3 */
      struct RLC_Config_um_Uni_Directional_UL um_Uni_Directional_UL;
      /* type = 4 */
      struct RLC_Config_um_Uni_Directional_DL um_Uni_Directional_DL;
   } rlcConfigType;
};
struct Ul_SpecificParameters{
	int priority;        //INTEGER (1..16)
	enum PrioritisedBitRate{
		kBps0=0, kBps8=8, kBps16=16, kBps32=32, kBps64=64, kBps128=128,
		kBps256=256, infinity=-1//..........spare1~8未表示
	}prioritisedBitRate;
	enum BucketSizeDuration{
		bucketSizeDuration_ms50=50, bucketSizeDuration_ms100=100, bucketSizeDuration_ms150=150, bucketSizeDuration_ms300=300, 
		bucketSizeDuration_ms500=500, bucketSizeDuration_ms1000=1000//..........spare1~2未表示
	}bucketSizeDuration;
	int logicalChannelGroup;	//INTEGER (0..3) 
};
struct LogicalChannelConfig{
	bool haveUl_SpecificParameters;
	struct Ul_SpecificParameters ul_SpecificParameters;
	
};

struct SrbToAddMod
 {
   int srbIdentity;       //INTEGER (1..2)
   bool haveRlcConfig;
   struct RlcConfig rlcConfig;     //rlcConfig==null则表示采用协议默认配置
   bool haveLogicalChannelConfig;
   struct LogicalChannelConfig logicalChannelConfig;   //null则表示采用协议默认配置
 };
struct SrbToAddModList{	
	int num;     //number of SrbToAddMod in SrbToAddModList
	struct SrbToAddMod srbList[2];
};

struct DrbToAddMod{
	int eps_BearerIdentity;//INTEGER (0..15)	
	DRB_Identity drb_Identity;
	bool haveRlcConfig;
	struct RlcConfig rlcConfig;
	int logicalChannelIdentity;//INTEGER (3..10)
	bool haveLogicalChannelConfig;
	struct LogicalChannelConfig logicalChannelConfig;
};

struct DrbToAddModList{	
	int num;
	struct DrbToAddMod drbList[maxDRB];
};

struct DrbToReleaseList{	
	int num;
	DRB_Identity drbToRelease[maxDRB];
};
struct Ul_SCH_Config{
	enum MaxHARQ_Tx{
		maxHARQ_Tx_n1=1, maxHARQ_Tx_n2, maxHARQ_Tx_n3, maxHARQ_Tx_n4, maxHARQ_Tx_n5, maxHARQ_Tx_n6, maxHARQ_Tx_n7, maxHARQ_Tx_n8,
		maxHARQ_Tx_n10=10, maxHARQ_Tx_n12=12, maxHARQ_Tx_n16=16, maxHARQ_Tx_n20=20, maxHARQ_Tx_n24=24, maxHARQ_Tx_n28=28//未表示spare1~2
	}maxHARQ_Tx;
	enum PeriodicBSR_Timer{
		periodicBSR_Timer_sf5=5, periodicBSR_Timer_sf10=10, periodicBSR_Timer_sf16=16, periodicBSR_Timer_sf20=20, 
		periodicBSR_Timer_sf32=32, periodicBSR_Timer_sf40=40, periodicBSR_Timer_sf64=64, periodicBSR_Timer_sf80=80,
		periodicBSR_Timer_sf128=128, periodicBSR_Timer_sf160=160, periodicBSR_Timer_sf320=320, periodicBSR_Timer_sf640=640, 
		periodicBSR_Timer_sf1280=1280, periodicBSR_Timer_sf2560=2560,periodicBSR_Timer_infinity=-1//未表示spare1
	}periodicBSR_Timer;
	enum RetxBSR_Timer{
		retxBSR_Timer_sf320=320, retxBSR_Timer_sf640=640, retxBSR_Timer_sf1280=1280, retxBSR_Timer_sf2560=2560, sf5120=5120,
		sf10240=10240//未表示spare1~2
	}retxBSR_Timer;
	bool ttiBundling;	
};
struct ShortDRX{
	enum ShortDRX_Cycle{
		shortDRX_Cycle_sf2=2, shortDRX_Cycle_sf5=5, shortDRX_Cycle_sf8=8, shortDRX_Cycle_sf10=10, shortDRX_Cycle_sf16=16, shortDRX_Cycle_sf20=20,
		shortDRX_Cycle_sf32=32, shortDRX_Cycle_sf40=40, shortDRX_Cycle_sf64=64, shortDRX_Cycle_sf80=80, shortDRX_Cycle_sf128=128, shortDRX_Cycle_sf160=160,
		shortDRX_Cycle_sf256=256, shortDRX_Cycle_sf320=320, shortDRX_Cycle_sf512=512, shortDRX_Cycle_sf640=640
	}shortDRX_Cycle;
	int drxShortCycleTimer;    //INTEGER (1..16)
};
struct DRX_Config_setup{
	enum OnDurationTimer{
		psf1=1, psf2, psf3, psf4, psf5, psf6,
		psf8=8, psf10=10, psf20=20, psf30=30, psf40=40,
		psf50=50, psf60=60, psf80=80, psf100=100,psf200=200
	}onDurationTimer;
	enum Drx_InactivityTimer{
		drx_InactivityTimer_psf1=1, drx_InactivityTimer_psf2, drx_InactivityTimer_psf3, drx_InactivityTimer_psf4, pdrx_InactivityTimer_sf5, drx_InactivityTimer_psf6,
		drx_InactivityTimer_psf8=8, drx_InactivityTimer_psf10=10, drx_InactivityTimer_psf20=20, drx_InactivityTimer_psf30=30, drx_InactivityTimer_psf40=40,
		drx_InactivityTimer_psf50=50, drx_InactivityTimer_psf60=60, drx_InactivityTimer_psf80=80, drx_InactivityTimer_psf100=100,
		drx_InactivityTimer_psf200=200, psf300=300, psf500=500, psf750=750,
		psf1280=1280, psf1920=1920, psf2560=2560//未表示spare1~10
	}drx_InactivityTimer;
	enum Drx_RetransmissionTimer{
		drx_RetransmissionTimer_psf1=1, drx_RetransmissionTimer_psf2, drx_RetransmissionTimer_psf4=4, 
		drx_RetransmissionTimer_psf6=6, drx_RetransmissionTimer_psf8=8, psf16=16,psf24=24, psf33=33
	}drx_RetransmissionTimer;
	int type;    //1:sf10......
	union LongDRX_CycleStartOffset{		
		int sf10;							//INTEGER(0..9),
		int sf20;							//INTEGER(0..19),
	    int sf32;							//INTEGER(0..31),
		int sf40;							//INTEGER(0..39),
		int sf64;							//INTEGER(0..63),
		int sf80;							//INTEGER(0..79),
		int sf128;							//INTEGER(0..127),
		int sf160;							//INTEGER(0..159),
		int sf256;							//INTEGER(0..255),
		int sf320;							//INTEGER(0..319),
		int sf512;							//INTEGER(0..511),
		int sf640;							//INTEGER(0..639),
		int sf1024;							//INTEGER(0..1023),
		int sf1280;							//INTEGER(0..1279),
		int sf2048;							//INTEGER(0..2047),
		int sf2560;						    //INTEGER(0..2559)
	}longDRX_CycleStartOffset;
	bool haveShortDRX;
	struct ShortDRX shortDRX;
};
struct DRX_Config{
	int type;    //1:release, 2:setup
	union {
		int release;
		struct DRX_Config_setup setup;
	}choice;
};
struct Phr_Config_Setup{
	enum PeriodicPHR_Timer{
		periodicPHR_Timer_sf10=10, periodicPHR_Timer_sf20=20, periodicPHR_Timer_sf50=50, periodicPHR_Timer_sf100=100, periodicPHR_Timer_sf200=200,
		periodicPHR_Timer_sf500=500, periodicPHR_Timer_sf1000=1000, periodicPHR_Timer_infinity=-1
	}periodicPHR_Timer;
	enum ProhibitPHR_Timer{
		prohibitPHR_Timer_sf0=0, prohibitPHR_Timer_sf10=10, prohibitPHR_Timer_sf20=20,prohibitPHR_Timer_sf50=50, prohibitPHR_Timer_sf100=100,
		prohibitPHR_Timer_sf200=200, prohibitPHR_Timer_sf500=500, prohibitPHR_Timer_sf1000=1000
	}prohibitPHR_Timer;
	enum Dl_PathlossChange{
		dl_PathlossChange_dB1=1, dl_PathlossChange_dB3=3, dl_PathlossChange_dB6=6, dl_PathlossChange_infinity=-1
	}dl_PathlossChange;
};
struct TimeAlignmentTimer{
	enum {
		timeAlignmentTimertype_sf500=500, timeAlignmentTimertype_sf750=750, timeAlignmentTimertype_sf1280=1280, 
		timeAlignmentTimertype_sf1920=1920, timeAlignmentTimertype_sf2560=2560,
		timeAlignmentTimertype_sf5120=5120,timeAlignmentTimertype_sf10240=10240, timeAlignmentTimertype_infinity=-1
	}timeAlignmentTimertype;
};
struct MAC_MainConfig{//描述信令和数据无线承载的MAC主配置
	bool haveUl_SCH_Config;
	struct Ul_SCH_Config ul_SCH_Config;
	bool haveDRX_Config;
	struct DRX_Config drx_Config;
	struct TimeAlignmentTimer timeAlignmentTimerDedicated;
	int type;    //1:release, 2:setup
	union Phr_Config{
		int release;
		struct Phr_Config_Setup setup;
	}phr_Config;
};
struct C_RNTI {
	uint16_t c_rnti;
};
struct N1_PUCCH_AN_PersistentList {
	int num[4];
} ;
struct SPS_ConfigDL_setup{
	enum SemiPersistSchedIntervalDL{
		semiPersistSchedIntervalDL_sf10=10, semiPersistSchedIntervalDL_sf20=20, semiPersistSchedIntervalDL_sf32=30, semiPersistSchedIntervalDL_sf40=40, 
		semiPersistSchedIntervalDL_sf64=60, semiPersistSchedIntervalDL_sf80=80,
		semiPersistSchedIntervalDL_sf128=130, semiPersistSchedIntervalDL_sf160=160, semiPersistSchedIntervalDL_sf320=320, semiPersistSchedIntervalDL_sf640=640
	}semiPersistSchedIntervalDL;
	int numberOfConfSPS_Processes;   //INTEGER (1..8)
	struct N1_PUCCH_AN_PersistentList n1_PUCCH_AN_PersistentList;
};
struct SPS_ConfigDL{
	int type;    //1:release, 2:setup
	union {
		int release;
		struct SPS_ConfigDL_setup setup;
	}choice;
};
struct P0_Persistent{
	int p0_NominalPUSCH_Persistent;//INTEGER (-126..24)
	int p0_UE_PUSCH_Persistent;    //INTEGER (-8..7)
};

struct SPS_ConfigUL_setup{
	enum SemiPersistSchedIntervalUL{
		semiPersistSchedIntervalUL_sf10=10, semiPersistSchedIntervalUL_sf20=20, semiPersistSchedIntervalUL_sf32=30, 
		semiPersistSchedIntervalUL_sf40=40, semiPersistSchedIntervalUL_sf64=60, semiPersistSchedIntervalUL_sf80=80,
		semiPersistSchedIntervalUL_sf128=130, semiPersistSchedIntervalUL_sf160=160, semiPersistSchedIntervalUL_sf320=320, semiPersistSchedIntervalUL_sf640=640//未表示spare1~6
	}semiPersistSchedIntervalUL;
	enum ImplicitReleaseAfter{
		e2=2, e3=3, e4=4, e8=8
	}implicitReleaseAfter;
	bool haveP0_Persistent;
	struct P0_Persistent p0_Persistent;
	bool twoIntervalsConfig;
};
struct SPS_ConfigUL{
	int type;    //1:release, 2:setup
	union {
		int release;
		struct SPS_ConfigUL_setup setup;
	}choice;
};
struct SPS_Config{//描述半静态调度配置
	bool haveC_RNTI;
	struct C_RNTI semiPersistSchedC_RNTI;
	bool haveSPS_ConfigDL;
	struct SPS_ConfigDL sps_ConfigDL;
	bool haveSPS_ConfigUL;
	struct SPS_ConfigUL sps_ConfigUL;
};
struct PDSCH_ConfigDedicated{//描述UE专用的 PDSCH 配置
	enum P_a {
		dB_6=-6, dB_4dot77=-477, dB_3=-3, dB_1dot77=-177,//dB_6 对应-6 dB， dB_4dot77 对应 -4.77 dB 等等
		p_a_dB0=0, p_a_dB1=1, p_a_dB2, p_a_dB3
	}p_a;
};
struct AckNackRepetition_setup{
	enum RepetitionFactor{
		repetitionFactor_n2=2, repetitionFactor_n4=4, repetitionFactor_n6=6
	}repetitionFactor;
	int n1PUCCH_AN_Rep;
};

struct PUCCH_ConfigDedicated{//描述UE专用的PUCCH配置
	int type;    //1:release, 2:setup, 3:tddAckNackFeedbackMode
	union AckNackRepetition{
		int release;
		struct AckNackRepetition_setup setup;
		enum TddAckNackFeedbackMode{
			bundling=5, multiplexing=5
		}tddAckNackFeedbackMode;
	}ackNackRepetition;
};
struct PUSCH_ConfigDedicated{//描述UE专用的 PUSCH 配置
	int betaOffset_ACK_Index;  //INTEGER (0..15)
	int betaOffset_RI_Index;   //INTEGER (0..15)
	int betaOffset_CQI_Index;  //INTEGER (0..15)
};
struct FilterCoefficient{
	enum {
		fc0, fc1, fc2, fc3, fc4, fc5,
		fc6, fc7, fc8, fc9, fc11=11, fc13=13,
		fc15=15, fc17=17, fc19=19//未表示spare1
	}filterCoefficienttype;
};

struct UplinkPowerControlDedicated{//描述专用信号中的上行功率控制的参数
	int p0_UE_PUSCH;           //INTEGER (-8..7)
	enum DeltaMCS_Enabled{
		en0, en1=1             //en1 对应值1.25 
	}deltaMCS_Enabled;
	bool accumulationEnabled;
	int p0_uE_PUCCH;           //INTEGER (-8..7)
	int pSRS_Offset;           //INTEGER (0..15)
	struct FilterCoefficient filterCoefficient;
};
struct TPC_PDCCH_Config_setup{
	int indexOfFormat3;        //INTEGER (1..15)
	int indexOfFormat3A;       //INTEGER (1..31)
};
struct TPC_PDCCH_Config{//描述RNTI，以及 PUCCH和PUSCH 功率控制的序号
	int type;    //1:release, 2:setup
	union {
		int release;
		struct TPC_PDCCH_Config_setup setup;
	}choice;
};
struct SubbandCQI{
	int k;                      //INTEGER (1..4)
};
struct CQI_ReportPeriodic_setup{
	int cqi_PUCCH_ResourceIndex;//INTEGER (0.. 1185)
	int cqi_pmi_ConfigIndex;    //INTEGER (0..1023)
	int type;    //1:widebandCQI......
	union Cqi_FormatIndicatorPeriodic{
		int widebandCQI;       //值为NULL
		struct SubbandCQI subbandCQI;
		int ri_ConfigIndex;    //INTEGER (0..1023)
		bool simultaneousAckNackAndCQI;
	}cqi_FormatIndicatorPeriodic;
};
struct CQI_ReportPeriodic{
	int type;    //1:release, 2:setup, 3:ri_ConfigIndex, 4:simultaneousAckNackAndCQI
	union {
		int release;
		struct CQI_ReportPeriodic_setup setup;
		int ri_ConfigIndex;
		bool simultaneousAckNackAndCQI;
	}choice;
};
struct CQI_ReportConfig{//描述 CQI报告配置
	enum Cqi_ReportModeAperiodic{
		rm12=12, rm20=20, rm22=22, rm30=30, rm31=31//未表示spare1~3
	}cqi_ReportModeAperiodic;
	int nomPDSCH_RS_EPRE_Offset;//INTEGER (-1..6)
	bool haveCQI_ReportPeriodic;
	struct CQI_ReportPeriodic cqi_ReportPeriodic;
};
struct SoundingRS_UL_ConfigDedicated_setup{
	enum Srs_Bandwidth{
		srs_Bandwidth_bw0, srs_Bandwidth_bw1, srs_Bandwidth_bw2, srs_Bandwidth_bw3
	}srs_Bandwidth;
	enum Srs_HoppingBandwidth{
		hbw0, hbw1, hbw2, hbw3
	}srs_HoppingBandwidth;
	int FreqDomainPosition;      //INTEGER (0..23)
	bool duration;
	int srs_ConfigIndex;         //INTEGER (0..1023)
	int transmissionComb;        //INTEGER (0..1)
	enum CyclicShift{
		cs0, cs1, cs2, cs3, cs4, cs5, cs6, cs7
	}cyclicShift;
};
struct SoundingRS_UL_ConfigDedicated{//描述上行探测RS配置
	int type;    //1:release, 2:setup
	union {
		int release;
		struct SoundingRS_UL_ConfigDedicated_setup setup;
	}choice;
};
struct SchedulingRequestConfig_setup {
	int sr_PUCCH_ResourceIndex;    //INTEGER (0..2047)
	int sr_ConfigIndex;            //INTEGER (0..157)
	enum Dsr_TransMax{
		dsr_TransMax_n4=4, dsr_TransMax_n8=8, dsr_TransMax_n16=16, dsr_TransMax_n32=32, dsr_TransMax_n64=64//未表示spare1~3
	}dsr_TransMax;
};
struct SchedulingRequestConfig{//描述调度请求相关的参数
	int type;    //1:release, 2:setup
	union {
		int release;
		struct SchedulingRequestConfig_setup setup;
	}choice;
};

struct PhysicalConfigDedicated{
	bool havePDSCH_ConfigDedicated;
	struct PDSCH_ConfigDedicated pdsch_ConfigDedicated;
	bool havePUCCH_ConfigDedicated;
	struct PUCCH_ConfigDedicated pucch_ConfigDedicated;
	bool havePUSCH_ConfigDedicated;
	struct PUSCH_ConfigDedicated pusch_ConfigDedicated;
	bool haveUplinkPowerControlDedicated;
	struct UplinkPowerControlDedicated uplinkPowerControlDedicated;
	bool haveTPC_PDCCH_Config;
	struct TPC_PDCCH_Config tpc_PDCCH_ConfigPUCCH;
	struct TPC_PDCCH_Config tpc_PDCCH_ConfigPUSCH;
	bool haveCQI_ReportConfig;
	struct CQI_ReportConfig cqi_ReportConfig;
	bool haveSoundingRS_UL_ConfigDedicated;
	struct SoundingRS_UL_ConfigDedicated soundingRS_UL_ConfigDedicated;
	bool haveAntennaInformationDedicated;
	struct AntennaInformationDedicated antennaInfo;
	bool haveSchedulingRequestConfig;
	struct SchedulingRequestConfig schedulingRequestConfig;
};

struct RadioResourceConfigDedicated{
	bool haveSrbToAddModList;
	struct SrbToAddModList srbToAddModList;
	bool haveDrbToAddModList;
	struct DrbToAddModList drbToAddModList;
	bool haveDrbToReleaseList;
	struct DrbToReleaseList drbToReleaseList;
	bool haveMAC_MainConfig; 
	struct MAC_MainConfig mac_MainConfig;//mac_MainConfig==null则表示采用协议默认配?
	bool haveSPS_Config;
	struct SPS_Config sps_Config;
	bool havePhysicalConfigDedicated;
	struct PhysicalConfigDedicated physicalConfigDedicated;	
}__;


#endif

