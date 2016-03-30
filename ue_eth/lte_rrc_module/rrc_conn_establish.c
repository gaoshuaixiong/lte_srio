/******************************************************************
**Filename:impl_rrc_conn_establish.c
**Copyright:uestc
**Description:RRC connection data struct,use for initialization and config message
**V1.0,2014/09/06,Xu Shangfei written
**-----------------------------------
**modification history:
**Modified by: Xu Shangfei
**Modified date:2014.10.9
**Description:modified note
****************************************************************/
#include "../lte_system.h"
#include "rrc_utils.h"
#include "rrc_conn_establish.h"

extern void send_to_user(char *info, int size_info);


/******************************************************************
**Function name:struct RadioResourceConfigDedicated *gen_Defaultradioresource
**Description:generate default message pointer use for RRC connection process
**Input:void
**Output:void
**Return:defaultRadioresource1
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:
****************************************************************/
struct RadioResourceConfigDedicated *gen_Defaultradioresource (void)
{
    struct PDSCH_ConfigDedicated Pdsch_ConfigDe={                           //描述UE专用的 PDSCH 配置
        .p_a = p_a_dB0,
	};

	struct PUCCH_ConfigDedicated Pucch_ConfigDe={                           //描述UE专用的PUCCH配置
		.type = 3,    //1:release, 2:setup, 3:tddAckNackFeedbackMode
		.ackNackRepetition.tddAckNackFeedbackMode = bundling,
	};

	struct PUSCH_ConfigDedicated Pusch_ConfigDe={                            //描述UE专用的 PUSCH 配置
		.betaOffset_ACK_Index = 10,  //INTEGER (0..15)
		.betaOffset_RI_Index = 12,  //INTEGER (0..15)
		.betaOffset_CQI_Index = 15,  //INTEGER (0..15)
	};

	struct UplinkPowerControlDedicated UplinkPowerCDe={                     //描述专用信号中的上行功率控制的参数
		.p0_UE_PUSCH = 0,         //INTEGER (-8..7)
		.deltaMCS_Enabled = en0,
		.accumulationEnabled = false,
		.p0_uE_PUCCH = 0,          //INTEGER (-8..7)
		.pSRS_Offset = 7,           //INTEGER (0..15)
		.filterCoefficient.filterCoefficienttype = fc4,
	};

	struct TPC_PDCCH_Config TPC_pucch={                                     //描述RNTI，以及 PUCCH和PUSCH 功率控制的序号
		.type = 1,    //1:release, 2:setup
		.choice.release = 1,
	};

	struct TPC_PDCCH_Config TPC_pusch={                                     //描述RNTI，以及 PUCCH和PUSCH 功率控制的序号
		.type = 1,    //1:release, 2:setup
		.choice.release = 1,
	};

	struct CQI_ReportPeriodic Cqi_Periodic={
		.type = 1,    //1:release, 2:setup, 3:ri_ConfigIndex, 4:simultaneousAckNackAndCQI
		.choice.release = 1,
	};

	struct CQI_ReportConfig Cqi_ReConfig={                                  //描述 CQI报告配置
		.cqi_ReportModeAperiodic = rm12,
		.nomPDSCH_RS_EPRE_Offset = 0,//INTEGER (-1..6)
		.haveCQI_ReportPeriodic = true,
		.cqi_ReportPeriodic = Cqi_Periodic,
	};

	struct SoundingRS_UL_ConfigDedicated SRS_UL_CDe={                       //描述上行探测RS配置
		.type = 1,   //1:release, 2:setup
		.choice.release = 1,
	};

	struct AntennaInformationDedicated AnteInfoDe={
		.transmissionMode=tm1,
        .type_codebookSubsetRestriction = 1,    //1:n2TxAntenna_tm3......
		.codebookSubsetRestriction= {0},
		.type_ue_TransmitAntennaSelection = 1,
		.ue_TransmitAntennaSelection.release = 1,
	};

	struct SchedulingRequestConfig ScheduReConfig={                         //描述调度请求相关的参数
		.type = 1,   //1:release, 2:setup
		.choice.release = 1,
	};

	//set default phyconfigdedicated
	struct PhysicalConfigDedicated set_phyconfigdedicated={
		.havePDSCH_ConfigDedicated = true,
		.pdsch_ConfigDedicated = Pdsch_ConfigDe,
		.havePUCCH_ConfigDedicated = true,
		.pucch_ConfigDedicated = Pucch_ConfigDe,
		.havePUSCH_ConfigDedicated = true,
		.pusch_ConfigDedicated = Pusch_ConfigDe,
		.haveUplinkPowerControlDedicated = true,
		.uplinkPowerControlDedicated = UplinkPowerCDe,
		.haveTPC_PDCCH_Config = true,
		.tpc_PDCCH_ConfigPUCCH = TPC_pucch,
		.tpc_PDCCH_ConfigPUSCH = TPC_pusch,
		.haveCQI_ReportConfig = true,
		.cqi_ReportConfig = Cqi_ReConfig,
		.haveSoundingRS_UL_ConfigDedicated = true,
		.soundingRS_UL_ConfigDedicated = SRS_UL_CDe,
		.haveAntennaInformationDedicated = true,
		.antennaInfo = AnteInfoDe,
		.haveSchedulingRequestConfig = true,
		.schedulingRequestConfig = ScheduReConfig,
	};

	struct SPS_ConfigDL sps_dl={
		.type = 1,   //1:release, 2:setup
		.choice.release = 1,
	};
	struct SPS_ConfigUL sps_ul={
		.type = 1,    //1:release, 2:setup
		.choice.release = 1,
	};

	//set default spsconfig
	struct SPS_Config set_spsconfig={
		.haveSPS_ConfigDL = true,
		.sps_ConfigDL = sps_dl,
		.haveSPS_ConfigUL = true,
		.sps_ConfigUL = sps_ul,
	};

	struct Ul_SCH_Config ul_sch_config={
		.maxHARQ_Tx = maxHARQ_Tx_n5,
		.periodicBSR_Timer = periodicBSR_Timer_infinity,
		.retxBSR_Timer = retxBSR_Timer_sf2560,
		.ttiBundling = false,
	};

	struct DRX_Config drxconfig={
		.type = 1,    //1:release, 2:setup
		.choice.release = 1,
	};

	//set default macmainconfig
	struct MAC_MainConfig set_macmainconfig={
		.haveUl_SCH_Config = true,
		.ul_SCH_Config = ul_sch_config,
		.haveDRX_Config = true,
		.drx_Config = drxconfig,
		.type = 1,    //1:release, 2:setup
		.phr_Config.release = 1,
	};
	//set default radioresource
	struct RadioResourceConfigDedicated *defaultRadioresource1 = fsm_mem_alloc(sizeof(struct RadioResourceConfigDedicated));
	defaultRadioresource1->haveSrbToAddModList = false;
	//defaultRadioresource1->srbToAddModList = {0};
	defaultRadioresource1->haveDrbToAddModList = false;
	//defaultRadioresource1->drbToAddModList = {0};
	defaultRadioresource1->haveDrbToReleaseList = false;
	//defaultRadioresource1->drbToReleaseList = {0};
	defaultRadioresource1->haveMAC_MainConfig = true;
	fsm_mem_cpy(&(defaultRadioresource1->mac_MainConfig), &set_macmainconfig, sizeof(struct MAC_MainConfig));
	//defaultRadioresource1->mac_MainConfig = set_macmainconfig;       //mac_MainConfig==null means use default config
	defaultRadioresource1->haveSPS_Config = true;
	fsm_mem_cpy(&(defaultRadioresource1->sps_Config), &set_spsconfig, sizeof(struct SPS_Config));
	//defaultRadioresource1->sps_Config = set_spsconfig;
	defaultRadioresource1->havePhysicalConfigDedicated = true;
	fsm_mem_cpy(&(defaultRadioresource1->physicalConfigDedicated), &set_phyconfigdedicated, sizeof(struct SPS_Config));
	//defaultRadioresource1->physicalConfigDedicated = set_phyconfigdedicated;
	return defaultRadioresource1;
}


/********************************************************************
**Function name:struct SystemInformationBlockType2 *gen_sysInfoTimer
**Description:generate SystemInformationBlockType2 meaasge pointer use for RRC connection process
**Input:void
**Output:void
**Return:sys2infotimer1
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
*********************************************************************/

struct SystemInformationBlockType2 *gen_sysInfoTimer(void)
{
	struct TimeAlignmentTimer timeAligment={
		.timeAlignmentTimertype = timeAlignmentTimertype_sf10240 * 100,
	};
	struct SystemInformationBlockType2 *sys2infotimer1 = fsm_mem_alloc(sizeof(struct SystemInformationBlockType2));
	fsm_mem_cpy(&(sys2infotimer1->timeAlignmentTimerCommon), &timeAligment, sizeof(struct TimeAlignmentTimer));
	//sys2infotimer1->timeAlignmentTimerCommon = timeAligment;
	//fsm_printf("	set	setsysInfoTimer message   !\n");
	return sys2infotimer1;
}

/******************************************************************
**Function name:struct UL_CCCH_Message *gen_ul_ccchmsg_send_rrcquest
**Description:generate UL_CCCH_Message meaasge pointer ues for RRC connection process ,the meaasge include RRCrequest message
**Input:void
**Output:void
**Return:send_ulccchmsgRRCreq
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/

struct UL_CCCH_Message *gen_ul_ccchmsg_send_rrcquest(void)
{
	struct S_TMSI s_tmsi={
		.mmec = (u8)11,
		.m_TMSI = (u32)123456,
	};
	struct RRCConnectionRequest setrrcrequestmsg={
		.type = 1,   //type=1:s_TMSI,  type=2:randomValue
		.ue_Identity.s_TMSI = s_tmsi,
		.establishmentCause = mt_Access,
	};

	struct UL_CCCH_Message *send_ulccchmsgRRCreq =  (struct UL_CCCH_Message *)fsm_mem_alloc(sizeof(struct UL_CCCH_Message));
	send_ulccchmsgRRCreq->type = 2;
	//send_ulccchmsgRRCreq->msg.rrcConnectionRequest = setrrcrequestmsg;
	fsm_mem_cpy(&(send_ulccchmsgRRCreq->msg.rrcConnectionRequest), &setrrcrequestmsg, sizeof(struct RRCConnectionRequest));
	fsm_printf("[rrc] [conn_establish] <-- send setrrcrequestmsg!\n");
	fsm_printf("[rrc] [conn_establish] ullccchmsg type=%d \n", send_ulccchmsgRRCreq->type);

	return send_ulccchmsgRRCreq;

}

/******************************************************************
**Function name:struct DL_CCCH_Message *gen_dl_ccch_send_rrcsetup
**Description:generate DL_CCCH_Message meaasge pointer ues for RRC connection process ,the meaasge include RRCConnectionSetup message
**Input:void
**Output:void
**Return:dl_ccch_rrcsetup
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
struct DL_CCCH_Message *gen_dl_ccch_send_rrcsetup(void)

{
	//srb-ToAddModList
	struct T_PollRetransmit t_pollretransmit1={
		.t_PollRetransmittype = ms120,
	};
	struct PollPDU pollpdu1={
		.pollPDUtype = p8,
	};
	struct PollByte pollbyte1={
		.pollByte = kB1000,
	};
	struct UL_AM_RLC  ul_amrlc1={
		.t_PollRetransmit = t_pollretransmit1,
		.pollPDU = pollpdu1,
		.pollByte = pollbyte1,
		.maxRetxThreshold = t2,
	};
	struct T_Reordering t_reordering1={
		.t_Reordering = t_Reordering_ms20,
	};
	struct T_StatusProhibit t_statusprohibit1={
		.t_StatusProhibit = t_StatusProhibit_ms20,
	};
	struct DL_AM_RLC dl_am_rlc1={
		.t_Reordering = t_reordering1,
		.t_StatusProhibit = t_statusprohibit1,
	};

	struct RLC_Config_am rlc_config_am1={
		.ul_AM_RLC =  ul_amrlc1,
		.dl_AM_RLC =  dl_am_rlc1,
	};
	struct RlcConfig rlcconfig1={                                       //RLC configuration of SRB and DRB
		.type = 1,
		.rlcConfigType.am = rlc_config_am1,
	};

	struct Ul_SpecificParameters ul_specificparameters1={
		.priority = 1,       //INTEGER (1..16)
		.prioritisedBitRate = kBps32,
		.bucketSizeDuration = bucketSizeDuration_ms100,
		.logicalChannelGroup = 1,	//INTEGER (0..3)
	};
	struct LogicalChannelConfig logicalchannelconfig1 = {
		.haveUl_SpecificParameters = true,
		.ul_SpecificParameters = ul_specificparameters1,
	};
	struct SrbToAddMod srbtoaddmod1={
		.srbIdentity = 1,       //INTEGER (1..2)
		.haveRlcConfig = true,
		.rlcConfig =  rlcconfig1,    //rlcConfig==null means use default config
		.haveLogicalChannelConfig = true,
		.logicalChannelConfig = logicalchannelconfig1,   //nullmeans use default config
	};
	struct SrbToAddMod srbtoaddmod2={
		.srbIdentity = 2,       //INTEGER (1..2)
		.haveRlcConfig = true,
		.rlcConfig =  rlcconfig1,    //rlcConfig==nullmeans use default config
		.haveLogicalChannelConfig = true,
		.logicalChannelConfig = logicalchannelconfig1,   //nullmeans use default config
	};

	/**srb-ToAddModList**/
	struct SrbToAddModList srb_toaddmodlist1={
		.num = 2,     //number of SrbToAddMod in SrbToAddModList
		.srbList[0] = srbtoaddmod1,
		.srbList[1] = srbtoaddmod2,
	};

	struct DrbToAddMod drb_toaddmod1={
		.eps_BearerIdentity = 1,//INTEGER (0..15)
		.drb_Identity = 7,
		.haveRlcConfig = true,
		.rlcConfig = rlcconfig1,
		.logicalChannelIdentity = 7,//INTEGER (3..10)
		.haveLogicalChannelConfig = true,
		.logicalChannelConfig = logicalchannelconfig1,
	};
	/**DrbToAddModList**/
	struct DrbToAddModList drb_toaddmodlist1={
		.num = 1,
		.drbList[0] = drb_toaddmod1,
	};
	/**DrbToReleaseList**/
	struct DrbToReleaseList drb_toreleaseliast={
		.num = 1,
		.drbToRelease[0] = 4,
	};
	/**mac-MainConfig**/
	struct Ul_SCH_Config ul_schconfig1={
		.maxHARQ_Tx = maxHARQ_Tx_n2,
		.periodicBSR_Timer = periodicBSR_Timer_sf40,
		.retxBSR_Timer = retxBSR_Timer_sf640,
		.ttiBundling = true,
	};
	struct ShortDRX shortdrx1={
		.shortDRX_Cycle = shortDRX_Cycle_sf64 ,
		.drxShortCycleTimer = 4,   //INTEGER (1..16)
	};
	struct DRX_Config_setup drx_config_setup1={
		.onDurationTimer =  psf60,
		.drx_InactivityTimer = drx_InactivityTimer_psf100,
		.drx_RetransmissionTimer = drx_RetransmissionTimer_psf8,
		.type = 3,    //1:sf10......
		.longDRX_CycleStartOffset.sf32 = 30,
		.haveShortDRX = true,
		.shortDRX = shortdrx1,
	};
	struct DRX_Config drx_config1={
        .type = 2,    //1:release, 2:setup
		.choice.setup = drx_config_setup1,
	};

	struct Phr_Config_Setup phr_configsetup1={

		.periodicPHR_Timer = periodicPHR_Timer_sf100,
		.prohibitPHR_Timer = prohibitPHR_Timer_sf100,
		.dl_PathlossChange = dl_PathlossChange_dB1,
	};
	struct TimeAlignmentTimer time_alignmenttimer1={
		.timeAlignmentTimertype = timeAlignmentTimertype_sf1920,
	};
	struct MAC_MainConfig mac_mainconfig1={                                     //descripe Macmainconfig
		.haveUl_SCH_Config = true,
		.ul_SCH_Config = ul_schconfig1,
		.haveDRX_Config = true,
		.drx_Config = drx_config1,
		.timeAlignmentTimerDedicated = time_alignmenttimer1,
		.type = 2,   //1:release, 2:setup
		.phr_Config.setup = phr_configsetup1,
	};

	/****SPS-Config****/
	struct C_RNTI c_rnti1={     //bitstring datetype
		.c_rnti = 4,
	};
	struct N1_PUCCH_AN_PersistentList n1_pucch_an_persistentlist1={          //parameters not understand need to refer to 36.213
		.num[0] = 1,
		.num[1] = 2,
		.num[2] = 3,
		.num[3] = 4,
	};
	struct SPS_ConfigDL_setup sps_configdl_setup1={
		.semiPersistSchedIntervalDL = semiPersistSchedIntervalDL_sf40,
		.numberOfConfSPS_Processes = 4,   //INTEGER (1..8)
		.n1_PUCCH_AN_PersistentList = n1_pucch_an_persistentlist1,
	};
	struct SPS_ConfigDL sps_configdl1={
		.type = 2,    //1:release, 2:setup
		.choice.setup = sps_configdl_setup1,
	};

	struct P0_Persistent p0_persistent1={
		.p0_NominalPUSCH_Persistent = 1,//INTEGER (-126..24)
		.p0_UE_PUSCH_Persistent = 1,    //INTEGER (-8..7)
	};

	struct SPS_ConfigUL_setup sps_config_setup1={
		.semiPersistSchedIntervalUL = semiPersistSchedIntervalUL_sf64,
		.implicitReleaseAfter = e2,
		.haveP0_Persistent = true,
		.p0_Persistent = p0_persistent1,
		.twoIntervalsConfig = true,
	};
	struct SPS_ConfigUL sps_configul1={
		.type = 2,   //1:release, 2:setup
		.choice.setup = sps_config_setup1,
	};
	struct SPS_Config sps_config1={                                             //descripe SPS_Config
		.haveC_RNTI = true,
		.semiPersistSchedC_RNTI = c_rnti1,
		.haveSPS_ConfigDL = true,
		.sps_ConfigDL = sps_configdl1,
		.haveSPS_ConfigUL = true,
		.sps_ConfigUL = sps_configul1,
	};

	/****PhysicalConfigDedicated****/
	struct PDSCH_ConfigDedicated pdsch_configdedicated1={                   //descripe PDSCH_ConfigDedicated
		.p_a = p_a_dB1,
	};
	struct AckNackRepetition_setup acknacrepetition_setup1={
		.repetitionFactor = repetitionFactor_n2,
		.n1PUCCH_AN_Rep = 4,
	};

	struct PUCCH_ConfigDedicated pucch_configdedicate1={                         //descripe PUCCH_ConfigDedicated
		.type = 2,   //1:release, 2:setup, 3:tddAckNackFeedbackMode
		.ackNackRepetition.setup = acknacrepetition_setup1,
	};
	struct PUSCH_ConfigDedicated pusch_configdicated1={                           //descripe PUSCH_ConfigDedicated
		.betaOffset_ACK_Index = 2, //INTEGER (0..15)
		.betaOffset_RI_Index = 2,   //INTEGER (0..15)
		.betaOffset_CQI_Index = 2, //INTEGER (0..15)
	};
	struct FilterCoefficient filtercoefficient1={
		.filterCoefficienttype = fc6,
	};

	struct UplinkPowerControlDedicated uplinkpowercontroldedicated1={               //descripe UplinkPowerControlDedicated
		.p0_UE_PUSCH = 2,          //INTEGER (-8..7)
		.deltaMCS_Enabled = en1,    //en1 对应值1.25
		.accumulationEnabled = true,
		.p0_uE_PUCCH = 2,         //INTEGER (-8..7)
		.pSRS_Offset = 2,          //INTEGER (0..15)
		.filterCoefficient = filtercoefficient1,
	};
	struct TPC_PDCCH_Config_setup tpc_pdcch_config_setup1={
		.indexOfFormat3 = 2,       //INTEGER (1..15)
		.indexOfFormat3A = 2,       //INTEGER (1..31)
	};
	struct TPC_PDCCH_Config tpc_pdcch_config1={
		.type = 2,    //1:release, 2:setup
		.choice.setup = tpc_pdcch_config_setup1,
	};
	struct SubbandCQI subbandcqi1={
		.k = 2,                      //INTEGER (1..4)
	};
	struct CQI_ReportPeriodic_setup cqi_reportperioid1={
		.cqi_PUCCH_ResourceIndex = 100,//INTEGER (0.. 1185)
		.cqi_pmi_ConfigIndex = 100,    //INTEGER (0..1023)
		.type = 3,    //1:widebandCQI......
		.cqi_FormatIndicatorPeriodic.ri_ConfigIndex = 100,
	};
	struct CQI_ReportPeriodic cqi_reportperodic1={
		.type = 2,   //1:release, 2:setup, 3:ri_ConfigIndex, 4:simultaneousAckNackAndCQI
		.choice.setup = cqi_reportperioid1,
	};
	struct CQI_ReportConfig cqi_reportconfig1={                                    //descripe CQI_ReportConfig
		.cqi_ReportModeAperiodic = rm20,
		.nomPDSCH_RS_EPRE_Offset = 2, //INTEGER (-1..6)
		.haveCQI_ReportPeriodic = true,
		.cqi_ReportPeriodic = cqi_reportperodic1,
	};
	struct SoundingRS_UL_ConfigDedicated_setup soundrs_ul_configdedicate_setup1={
		.srs_Bandwidth = srs_Bandwidth_bw0,
		.srs_HoppingBandwidth = hbw0,
		.FreqDomainPosition = 2,      //INTEGER (0..23)
		.duration = true,
		.srs_ConfigIndex = 2,        //INTEGER (0..1023)
		.transmissionComb = 1,        //INTEGER (0..1)
		.cyclicShift = cs1,
	};
	struct SoundingRS_UL_ConfigDedicated soundrs_ul_configdedicated1={              //descripe SoundingRS_UL_ConfigDedicated
		.type = 2,    //1:release, 2:setup
		.choice.setup = soundrs_ul_configdedicate_setup1,
	};
	struct SchedulingRequestConfig_setup schedulingrequestconfig_setup1={
		.sr_PUCCH_ResourceIndex = 100,    //INTEGER (0..2047)
		.sr_ConfigIndex = 100,           //INTEGER (0..157)
		.dsr_TransMax  = dsr_TransMax_n4,
	};
	struct SchedulingRequestConfig schedulingrequestconfig1={                       //descripe  SchedulingRequestConfig
		.type = 2,    //1:release, 2:setup
		.choice.setup = schedulingrequestconfig_setup1,
	};
	struct AntennaInformationDedicated antennainformationdedicated1={
		.transmissionMode = tm3,
		.type_codebookSubsetRestriction = 2,    //1:n2TxAntenna_tm3......
		.codebookSubsetRestriction.n4TxAntenna_tm3 = 2,
		.type_ue_TransmitAntennaSelection = 2,
		.ue_TransmitAntennaSelection.setup = openLoop,
	};

	struct PhysicalConfigDedicated physicalconfigdedicateed1={
		.havePDSCH_ConfigDedicated = true,
		.pdsch_ConfigDedicated = pdsch_configdedicated1,
		.havePUCCH_ConfigDedicated = true,
		.pucch_ConfigDedicated = pucch_configdedicate1,
		.havePUSCH_ConfigDedicated =true,
		.pusch_ConfigDedicated =  pusch_configdicated1,
		.haveUplinkPowerControlDedicated =true,
		.uplinkPowerControlDedicated = uplinkpowercontroldedicated1,
		.haveTPC_PDCCH_Config =true,
		.tpc_PDCCH_ConfigPUCCH = tpc_pdcch_config1,
		.tpc_PDCCH_ConfigPUSCH = tpc_pdcch_config1,
		.haveCQI_ReportConfig = true,
		.cqi_ReportConfig = cqi_reportconfig1,
		.haveSoundingRS_UL_ConfigDedicated =true,
		.soundingRS_UL_ConfigDedicated = soundrs_ul_configdedicated1,
		.haveAntennaInformationDedicated = true,
		.antennaInfo = antennainformationdedicated1,
		.haveSchedulingRequestConfig = true,
		.schedulingRequestConfig = schedulingrequestconfig1,
	};


	/****RadioResourceConfigDedicated****/
	struct RadioResourceConfigDedicated set_radioresourcemsg={
		.haveSrbToAddModList = true,
		.srbToAddModList = srb_toaddmodlist1,
		.haveDrbToAddModList = true,
		.drbToAddModList = drb_toaddmodlist1,
		.haveDrbToReleaseList = true,
		.drbToReleaseList = drb_toreleaseliast,
		.haveMAC_MainConfig = true,
		.mac_MainConfig = mac_mainconfig1,
		.haveSPS_Config = true,
		.sps_Config = sps_config1,
		.havePhysicalConfigDedicated = true,
		.physicalConfigDedicated = physicalconfigdedicateed1,
	};

	struct RRCConnectionSetup rrcConnectionSetupmsg1 ={
		.rrcTransactionIdentifier = 2,	//pay attention 随意赋值的
		.radioResourceConfigDedicated = set_radioresourcemsg,
	};


	struct DL_CCCH_Message *dl_ccch_rrcsetup = fsm_mem_alloc(sizeof(struct DL_CCCH_Message));
	dl_ccch_rrcsetup->type = 4;
	fsm_mem_cpy(&(dl_ccch_rrcsetup->msg.rrcConnectionSetup), &rrcConnectionSetupmsg1, sizeof(struct RRCConnectionSetup));
	//dl_ccch_rrcsetup->msg.rrcConnectionSetup = rrcConnectionSetupmsg1;
	return dl_ccch_rrcsetup;
}


/******************************************************************
**Function name:struct UL_DCCH_Message *gen_ul_dcch_rrccomplete
**Description:generate UL_CCCH_Message meaasge pointer ues for RRC connection process
**Input:void
**Output:void
**Return:ul_dcchmsg1
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
struct UL_DCCH_Message *gen_ul_dcch_rrccomplete(void)
{
	struct PLMN_Identity plmn_identity1={
		.MCC[0] = 2,
		.MNC[0] = 2,
	};
	struct RegisteredMME registeredmme1={
		.plmn_Identity = plmn_identity1,
		.mmegi = 1,
		.mmec= 1,
	};
	struct RRCConnectionSetupComplete rrcconnectionsetup_complete1={
		.rrcTransactionIdentifier = 1,
		.selectedPLMN_Identity = 1,    //INTEGER (1..6),
		.registeredMME = registeredmme1,
		//struct DedicatedInfoNAS dedicatedInfoNAS;       //协议里有,暂时不用
	};

	struct UL_DCCH_Message *ul_dcchmsg1 = fsm_mem_alloc(sizeof(struct UL_DCCH_Message));
	ul_dcchmsg1->type = 4;
	fsm_mem_cpy(&(ul_dcchmsg1->msg.rrcConnectionSetupComplete), &rrcconnectionsetup_complete1, sizeof(struct RRCConnectionSetupComplete));
	//ul_dcchmsg1->msg.rrcConnectionSetupComplete = rrcconnectionsetup_complete1;
	fsm_printf("[rrc] [conn_establish] <-- gen rrcsetupcompletemsg message !========\n");
	return ul_dcchmsg1;
}



/******************************************************************
**Function name:void set_default_physical_channel
**Description:receive struct PhysicalConfigDedicated phydefaultmsg message，ues IOCLT
			  to complete  PhysicalConfigDedicated config
**Input:struct PhysicalConfigDedicated phydefaultmsg
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void set_default_physical_channel(struct PhysicalConfigDedicated phydefaultmsg)
{
	FIN(set_default_physical_channel());
	// fsm_do_ioctrl(STRM_TO_PHY, IOCCMD_RRCTOPHY_CONFIG_CHANNEL, (void*)(&phydefaultmsg), sizeof(struct PhysicalConfigDedicated));
	fsm_printf("[rrc] [conn_establish] IOCLT of physical layer undefined. Waitting for the follow-up supplement!========\n");
	FOUT;

}
/******************************************************************
**Function name:void set_default_semi_persistent
**Description:receive struct SPS_Config spsdefaultmsg，ues IOCLT to complete  SPS config
**Input:struct SPS_Config spsdefaultmsg
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void set_default_semi_persistent (struct SPS_Config spsdefaultmsg)
{
	FIN(setDefaultspsconfig());
	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_SPS , (void*)(&spsdefaultmsg), sizeof(struct SPS_Config));

	fsm_printf("[rrc] [conn_establish] set_default_semi_persistent process is done successfully.=========\n");
	FOUT;
}
/******************************************************************
**Function name:void set_default_MAC
**Description:receive struct MAC_MainConfig macdefaultmsg，ues IOCLT to complete  default Mac config
**Input:struct MAC_MainConfig macdefaultmsg
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void set_default_MAC(struct MAC_MainConfig macdefaultmsg)
{
	FIN(set_default_MAC());
	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_MAC_MAIN , (void*)(&macdefaultmsg), sizeof(struct MAC_MainConfig));
	fsm_printf("[rrc] [conn_establish] set_default_MAC_MainConfig process is done successfully.============\n");
	FOUT;
}
/******************************************************************
**Function name:void set_timeAlignmentTimerCommon
**Description:receive struct SystemInformationBlockType2 *sys2infoAlignmenttimer，ues IOCLT to complete  sys2infoAlignmenttimer config
**Input:struct SystemInformationBlockType2 *sys2infoAlignmenttimer
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void set_timeAlignmentTimerCommon (struct SystemInformationBlockType2 *sys2infoAlignmenttimer)
{
	FIN(set_timeAlignmentTimerCommon());
	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_TAT , (void*)&(sys2infoAlignmenttimer->timeAlignmentTimerCommon), sizeof(struct TimeAlignmentTimer));

	fsm_printf("[rrc] [conn_establish] set_timeAlignmentTimerCommon process is done successfully.============\n");
	FOUT;

}
/******************************************************************
**Function name:void send_RRCConnectionRequest
**Description:recive struct UL_CCCH_Message *ulccchRRCremsg message，ues packet_send_to_pdcp
                send request to Enb
**Input:struct SystemInformationBlockType2 *sys2infoAlignmenttimer
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void send_RRCConnectionRequest(struct UL_CCCH_Message *ulccchRRCremsg)
{
	FIN(send_RRCConnectionRequest());

	char *msg = (char *)(&(*ulccchRRCremsg));
	u32 msg_len = sizeof(struct UL_CCCH_Message);
	u32 message_type = 5;
	packet_send_to_rlc(msg, msg_len, message_type);
	fsm_printf("[rrc] [conn_establish] message RRCConnectionRequest sends successfully.\n");
	fsm_printf("[rrc] [conn_establish] test request type=%d\n",ulccchRRCremsg->msg.rrcConnectionRequest.type );
	fsm_printf("[rrc] [conn_establish] test request mmec=%d\n",ulccchRRCremsg->msg.rrcConnectionRequest.ue_Identity.s_TMSI.mmec);

//	fsm_printf("********send Requestmessage to us \n");
//	char *data = "ue send rrcConnectionRequest";
//	char *data_to_send = (char *)fsm_mem_alloc(strlen(data)+1);
//	fsm_mem_cpy(data_to_send, data, strlen(data));
//	send_to_user(data_to_send, strlen(data_to_send));

	FOUT;

}


/****rcv_rrc_conn_setup子函数****/
/******************************************************************
**Function name:void set_MAC_mainconfig
**Description:recive struct RRCConnectionSetup *rrcConnectionSetupmMACmsg，ues IOCLT
			  to complete Mac main config in UE
**Input:struct RRCConnectionSetup *rrcConnectionSetupmMACmsg
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void set_MAC_mainconfig(struct RRCConnectionSetup *rrcConnectionSetupmMACmsg)
{
	FIN(set_MAC_mainconfig());
	if(rrcConnectionSetupmMACmsg->radioResourceConfigDedicated.haveMAC_MainConfig)
	{
		//ues IOCTLsend message betwween layers
		fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_MAC_MAIN , (void*)&(rrcConnectionSetupmMACmsg->radioResourceConfigDedicated.mac_MainConfig), sizeof(struct MAC_MainConfig));
		fsm_printf("[rrc] [conn_establish] set_MAC-mainconfig process is done successfully.==========\n");
	}
	FOUT;
}
/******************************************************************
**Function name:void set_semi_persistent
**Description:recive struct RRCConnectionSetup *rrcConnectionSetupmSPSmsg，ues IOCLT
			  to cpmlete SPS in UE
**Input:struct RRCConnectionSetup *rrcConnectionSetupmSPSmsg
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void set_semi_persistent(struct RRCConnectionSetup *rrcConnectionSetupmSPSmsg)
{
	FIN(set_semi_persistent());
	if(rrcConnectionSetupmSPSmsg->radioResourceConfigDedicated.haveSPS_Config)
	{
		//ues IOCTL to send SPS_config
		fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_SPS , (void*)&(rrcConnectionSetupmSPSmsg->radioResourceConfigDedicated.sps_Config), sizeof(struct SPS_Config));
		fsm_printf("[rrc] [conn_establish] set_semi-persistent process is done successfully.========\n");
	}
	FOUT;

}
/******************************************************************
**Function name:void  set_physicalConfigDedicated
**Description:recive struct RRCConnectionSetup *rrcConnectionSetupmPHYmsg，use IOCLT
			  to complete PhysicalConfigDedicated
**Input:struct RRCConnectionSetup *rrcConnectionSetupmPHYmsg
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void  set_physicalConfigDedicated(struct RRCConnectionSetup *rrcConnectionSetupmPHYmsg)

{
	FIN(set_physicalConfigDedicated())
		if(rrcConnectionSetupmPHYmsg->radioResourceConfigDedicated.havePhysicalConfigDedicated)
		{
		    //fix me :PHY IOCTL is wating
			//fsm_do_ioctrl(STRM_TO_PHY, IOCCMD_RRCTOPHY_CONFIG_CHANNEL , (void*)&(rrcConnectionSetupmPHYmsg->radioResourceConfigDedicated.physicalConfigDedicated), sizeof(struct PhysicalConfigDedicated ));
			fsm_printf("[rrc] [conn_establish] set_physicalConfigDedicated process is done successfully.BUT the TOCTL is waiting to send==========\n");
		}
	FOUT;
}
/******************************************************************
**Function name:void send_RRCConnectionSetupComplete
**Description:recive struct UL_DCCH_Message *ul_dcchmsg，use packet_send_to_pdcp
			  send RRCconsetup msg to Enb
**Input:struct UL_DCCH_Message *ul_dcchmsg
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014/09/06
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:2014.10.9
****************************************************************/
void send_RRCConnectionSetupComplete(struct UL_DCCH_Message *ul_dcchmsg)
{
	FIN(send_RRCConnectionSetupComplete());
	char *msg = (char*)&(*ul_dcchmsg);
	u32 msg_len = sizeof(ul_dcchmsg);
	u32 message_type = 6;
	packet_send_to_pdcp(msg, msg_len, message_type);
	fsm_printf("[rrc] [conn_establish] send_RRCConnectionSetupComplete process is done successfully.========\n");
	FOUT;
}

