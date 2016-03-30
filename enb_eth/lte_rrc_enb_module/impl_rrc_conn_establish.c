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
//#include "rrcfsm_enb_ue.h"
#include "rrc_utils.h"
#include "impl_rrc_conn_establish.h"


//extern void send_to_user(char *info, int size_info);
//extern void packet_send_to_rlc(char *msg, int msg_len, unsigned int message_type);

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
		.drb_Identity = 1,
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
		.haveDrbToAddModList = false,
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
		.rrcTransactionIdentifier = 2,	//random value
		.radioResourceConfigDedicated = set_radioresourcemsg,
	};


	struct DL_CCCH_Message *dl_ccch_rrcsetup = fsm_mem_alloc(sizeof(struct DL_CCCH_Message));
	dl_ccch_rrcsetup->type = 4;
	dl_ccch_rrcsetup->msg.rrcConnectionSetup = rrcConnectionSetupmsg1;
//	fsm_mem_cpy(&(dl_ccch_rrcsetup->msg.rrcConnectionSetup), &rrcConnectionSetupmsg1, sizeof(struct RRCConnectionSetup));

	return dl_ccch_rrcsetup;
}

/******************************************************************

**Filename:impl_rrc_conn_establish.c

**Copyright:uestc

**Description:RRC reject data struct,use for rrcreject msg

**V1.0,2014/10/16,Xu Shangfei written

**-----------------------------------

**modification history:

**Modified by: Xu Shangfei

**Modified date:2014.10.9

**Description:modified note

****************************************************************/
struct DL_CCCH_Message *gen_dl_ccch_send_rrcreject(void)
{
	struct RRCConnectionReject rrcrejct = {
			.waitTime = 4,
	};
	struct DL_CCCH_Message *dl_ccch_rrcrejct = fsm_mem_alloc(sizeof(struct DL_CCCH_Message));
	dl_ccch_rrcrejct->type = 3;
	fsm_mem_cpy(&(dl_ccch_rrcrejct->msg.rrcConnectionSetup), &rrcrejct, sizeof(struct RRCConnectionReject));

	return dl_ccch_rrcrejct;
}


/******************************************************************
**Function name:void send_RRCConnectionSetup
**Description:struct DL_CCCH_Message *dlccchRRCremsg，use packet_send_to_pdcp
			  send RRCconsetup msg to UE
**Input:struct DL_CCCH_Message *dlccchRRCremsg
**Output:void
**Return:void
**Created by:Xu Shangfei
**Created date:2014.10.9
**-----------------------------------
**Modified by: Xu Shangfei
**Modified date:
****************************************************************/
void send_RRCConnectionSetup(struct DL_CCCH_Message *dlccchRRCremsg)
{
	SV_PTR_GET(rrc_sv_enb_ue);
    FIN(send_RRCConnectionSetup());
    char *msg = (char *)(&(*dlccchRRCremsg));
    int msg_len = sizeof(struct DL_CCCH_Message);
    int message_type = 3;
    packet_send_to_rlc(msg, msg_len, message_type, SV(crnti));
    fsm_printf("[rrc] [conn_establish] <-- message RRCConnectionSetup sends successfully.\n");
    FOUT;
}

/******************************************************************
**Function name:void send_RRCConnectionRejct

**Description:struct DL_CCCH_Message *dlccchRRCremsg，use packet_send_to_pdcp
			  send RRCconsetup msg to UE
**Input:struct DL_CCCH_Message *dlccchRRCremsg

**Output:void
**Return:void
**Created by:Xu Shangfei

**Created date:2014.10.9
**-----------------------------------
**Modified by: Xu Shangfei

**Modified date:
****************************************************************/
void send_RRCConnectionRejct(struct DL_CCCH_Message *dlccchRRCremsg)
{
    FIN(send_RRCConnectionSetup());
	SV_PTR_GET(rrc_sv_enb_ue);
    char *msg = (char *)(&(*dlccchRRCremsg));
    int msg_len = sizeof(struct DL_CCCH_Message);
    int message_type = 3;
    packet_send_to_rlc(msg, msg_len, message_type, SV(crnti));
    fsm_printf("[rrc] [conn_establish] <-- message RRCConnectionRejct sends successfully.\n");
    FOUT;
}

//ues for test in-->static void rcv_rrc_conn_setup_complete(void)
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
	ul_dcchmsg1->msg.rrcConnectionSetupComplete = rrcconnectionsetup_complete1;
	fsm_printf("[rrc] [conn_establish] <--send rrcsetupcompletemsg message !\n");
	return ul_dcchmsg1;
}



