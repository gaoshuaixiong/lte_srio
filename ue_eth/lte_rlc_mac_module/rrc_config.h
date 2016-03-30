#include <stdbool.h>

#ifndef _RRC_CONFIG_H
#define _RRC_CONFIG_H

/*系统配置信息  由RRC传给MAC */
/*MAC-MainConfig  */

//配置信息的数据结构定义如下：
struct TimeAlignmentTimer{
	enum {
		timeAlignmentTimertype_sf500=500, timeAlignmentTimertype_sf750=750, timeAlignmentTimertype_sf1280=1280, 
		timeAlignmentTimertype_sf1920=1920, timeAlignmentTimertype_sf2560=2560,
		timeAlignmentTimertype_sf5120=5120,timeAlignmentTimertype_sf10240=10240, timeAlignmentTimertype_infinity=-1
	}timeAlignmentTimertype;
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

struct RachConfigCommon{ //描述普通的随机接入参数	
	struct PreambleInfo preambleInfo;
	struct PowerRampingParameters powerRampingParameters;
	struct RaSupervisionInfo raSupervisionInfo;	
	int maxHARQ_Msg3Tx;  //INTEGER (1..8)
};


//BCCH_Config
struct BCCH_Config{
	enum ModificationPeriodCoeff{
		modificationPeriodCoeff_n2=2, modificationPeriodCoeff_n4=4, modificationPeriodCoeff_n8=8, modificationPeriodCoeff_n16=16
	}modificationPeriodCoeff;
};

//PCCH_Config
struct PCCH_Config{
	enum DefaultPagingCycle{
		rf32=32, rf64=64, rf128=128, rf256=256
	}defaultPagingCycle;
	enum NB{
		fourT, twoT, oneT,halfT, quarterT, 
		oneEighthT,oneSixteenthT, oneThirtySecondT//fourT 值对应4 * defaultPagingCycle,  twoT 值对应2 * defaultPagingCycle 等等
	}nB;
};


//MAC_MainConfig 
struct Ul_SCH_Config{
	enum MaxHARQ_Tx{
		maxHARQ_Tx_n1=1, maxHARQ_Tx_n2, maxHARQ_Tx_n3, maxHARQ_Tx_n4, maxHARQ_Tx_n5, maxHARQ_Tx_n6, maxHARQ_Tx_n7, maxHARQ_Tx_n8,
		maxHARQ_Tx_n10=10, maxHARQ_Tx_n12=12, maxHARQ_Tx_n16=16, maxHARQ_Tx_n20=20, maxHARQ_Tx_n24=24, maxHARQ_Tx_n28=28//未表示spare1~2
	}maxHARQ_Tx;
	enum PeriodicBSR_Timer{
		periodicBSR_Timer_sf5=5, periodicBSR_Timer_sf10=10, periodicBSR_Timer_sf16=16, periodicBSR_Timer_sf20=20, 
		periodicBSR_Timer_sf32=32, periodicBSR_Timer_sf40=40, periodicBSR_Timer_sf64=64, periodicBSR_Timer_sf80=80,
		periodicBSR_Timer_sf128=128, periodicBSR_Timer_sf160=160, periodicBSR_Timer_sf320=320, periodicBSR_Timer_sf640=640, 
		periodicBSR_Timer_sf1280=1280, periodicBSR_Timer_sf2560=2560,periodicBSR_Timer_infinity=-1  //未表示spare1
	}periodicBSR_Timer;
	enum RetxBSR_Timer{
		retxBSR_Timer_sf320=320, retxBSR_Timer_sf640=640, retxBSR_Timer_sf1280=1280, retxBSR_Timer_sf2560=2560, sf5120=5120,
		sf10240=10240    //未表示spare1~2
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

struct MAC_MainConfig{ //描述信令和数据无线承载的MAC主配置
	bool haveUl_SCH_Config;
	struct Ul_SCH_Config ul_SCH_Config;
	bool haveDRX_Config;
	struct DRX_Config drx_Config;
	struct TimeAlignmentTimer timeAlignmentTimerDedicated;
	int type;//    //1:release, 2:setup
    union Phr_Config{
		int release;
		struct Phr_Config_Setup setup;
	}phr_Config;

};


// SPS_Config
struct C_RNTI {
//	uint16_t c_rant;
	unsigned short int c_rnti;//HQ modified
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
	int type; // 1:release, 2:setup
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


// LogicalChannelConfig
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

struct LogicalChannelConfig//lhl 更改，RRC层关于逻辑信道的结构
{
	bool haveUl_SpecificParameters;
	struct Ul_SpecificParameters ul_SpecificParameters;
}__attribute__((packed));

typedef struct MAC_LogicalChannelConfig_IoctrlMsg//lhl 更改，RRC层关于逻辑信道的结构
{
    int logicalChannelIdentity;
    struct LogicalChannelConfig logicalChannelConfig;
}__attribute__((packed))MAC_LogicalChannelConfig_IoctrlMsg;

#endif

