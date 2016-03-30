/******************************************************************
** Filename: rrc_enb_paging_SI.c
** Copyright: uestc.
** Descriptions:send paging and system information
** v1,2014/10/14,wangjinchuan written
** -----------------------------------------------
** modificationhistory:
** Modified by:
** Modified date:
** Descriptions:
**
******************************************************************/
#include "../lte_system.h"
#include "../fsm/fsmdec.h"
#include "rrc_enb_paging_SI.h"
#include "rrc_utils.h"

/******************************************************************
** Function name: Set_send_paging
** Descriptions: send paging message.
** Input: void
** Output:packet sent to rlc
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/14
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void Set_send_paging()
{
	FIN(Set_paging());

	fsm_printf("start to send paging.\n");
	struct Paging paging;
	paging.havePagingRecord=true;
	paging.systemInfoModification=true;
	paging.havePagingRecord=true;
	paging.pagingRecordList.num=3;
	paging.pagingRecordList.pagingRecord[0].ue_Identity.type=1;
	paging.pagingRecordList.pagingRecord[0].ue_Identity.choice.s_TMSI.mmec=11;
	paging.pagingRecordList.pagingRecord[0].ue_Identity.choice.s_TMSI.m_TMSI=123456;
	paging.pagingRecordList.pagingRecord[0].cn_Domain=0;
	paging.pagingRecordList.pagingRecord[1].ue_Identity.type=1;
	paging.pagingRecordList.pagingRecord[1].ue_Identity.choice.s_TMSI.mmec=12;
	paging.pagingRecordList.pagingRecord[1].ue_Identity.choice.s_TMSI.m_TMSI=123456;
	paging.pagingRecordList.pagingRecord[1].cn_Domain=0;
	paging.pagingRecordList.pagingRecord[2].ue_Identity.type=1;
	paging.pagingRecordList.pagingRecord[2].ue_Identity.choice.s_TMSI.mmec=12;
	paging.pagingRecordList.pagingRecord[2].ue_Identity.choice.s_TMSI.m_TMSI=123457;
	paging.pagingRecordList.pagingRecord[2].cn_Domain=0;
	struct PCCH_Message pcch_msg;
	pcch_msg.paging=paging;
	char *msg = (char *)&pcch_msg;
	int msg_len = sizeof(pcch_msg);
	int message_type = 0;
	
	packet_send_to_rlc(msg,msg_len,message_type, 0);
	
	FOUT;
}

/******************************************************************
** Function name: Set_send_mib
** Descriptions: send mib message.
** Input: void
** Output:packet sent to rlc
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/14
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void Set_send_mib()
{
	FIN(Set_mib());

	fsm_printf("[rrc][total]start to send MIB.\n");
	
	struct MasterInformationBlock mib;
	mib.dl_Bandwidth = (u8)100;
	mib.systemFrameNumber = (u8)120;
	struct BCCH_BCH_Message bcch_bch_msg;
	bcch_bch_msg.mib = mib;
	char *msg = (char *)&bcch_bch_msg;
	int msg_len = sizeof(bcch_bch_msg);
	int message_type = 1;
	
	packet_send_to_rlc(msg, msg_len, message_type, 0);
//	setTimer( 1, 4000);		//mib消息发送周期40ms
	
	fsm_printf("[rrc][total]MIB is sent successfully.\n");
	FOUT;
}

/******************************************************************
** Function name: Set_send_sib1
** Descriptions: send sib1 message.
** Input: void
** Output:packet sent to rlc
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/14
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void Set_send_sib1()
{
	FIN(Set_sib1());
	fsm_printf("[rrc][total]start to send SIB1.\n");
	struct SystemInformationBlockType1 sib1;
	sib1.cellAccessRelatedInfo.cellIdentity=(u32)15;
	sib1.cellAccessRelatedInfo.trackingAreaCode=(u16)12;
	struct BCCH_DL_SCH_Message bcch_dl_sch_message;
	bcch_dl_sch_message.type=2;
	bcch_dl_sch_message.msg.sib1=sib1;
	char *msg=(char *)&bcch_dl_sch_message;
	int msg_len=sizeof(bcch_dl_sch_message);
	int message_type = 2;
	packet_send_to_rlc(msg,msg_len,message_type, 0);
//	setTimer( 2, 8000);		//sib1消息发送周期80ms
	fsm_printf("[rrc][total]SIB1 is sent successfully.\n");
	FOUT;
}

/******************************************************************
** Function name: Set_send_si
** Descriptions: send si message.
** Input: void
** Output:packet sent to rlc
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/14
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void Set_send_si()
{
	FIN(Set_si());
	fsm_printf("[rrc][total]start to send SI.\n");
	struct SystemInformation si;
	si.haveSib2=true;
	si.sib2.haveMBSFN_SubframeConfigList=true;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.numberOfRA_Preambles=numberOfRA_Preambles_n32;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.havePreamblesGroupAConfig=true;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.preamblesGroupAConfig.sizeOfRA_PreamblesGroupA=n8;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.preamblesGroupAConfig.messageSizeGroupA=b144;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.preambleInfo.preamblesGroupAConfig.messagePowerOffsetGroupB=dB12;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.powerRampingParameters.powerRampingStep=powerRampingStep_dB4;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.powerRampingParameters.preambleInitialReceivedTargetPower=dBm_110;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.raSupervisionInfo.preambleTransMax=preambleTransMax_n4;
	si.sib2.radioResourceConfigCommon.rachConfigCommon.maxHARQ_Msg3Tx=4;
	si.sib2.radioResourceConfigCommon.bcch_Config.modificationPeriodCoeff=modificationPeriodCoeff_n4;
	si.sib2.radioResourceConfigCommon.pcch_Config.defaultPagingCycle=rf64;
	si.sib2.radioResourceConfigCommon.pcch_Config.nB=twoT;
	struct BCCH_DL_SCH_Message bcch_dl_sch_message;
	bcch_dl_sch_message.type=1;
	bcch_dl_sch_message.msg.si=si;
	char *msg=(char *)&bcch_dl_sch_message;
	int msg_len=sizeof(bcch_dl_sch_message);
	int message_type = 2;
	packet_send_to_rlc(msg,msg_len,message_type, 0);
//	setTimer( 3, 16000);	//sib2消息发送周期160ms
	fsm_printf("[rrc][total]SI is sent successfully.\n");
	FOUT;
}
