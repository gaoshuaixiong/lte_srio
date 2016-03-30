/******************************************************************
** Filename: rrc_paging_SI.c
** Copyright: uestc.
** Descriptions: Handle with the paging and system information received
** v8.1.2,2014/10/07,wangjinchuan written
** -----------------------------------------------
** modificationhistory:
** Modified by:
** Modified date:
** Descriptions:
**
******************************************************************/

#include "../lte_system.h"
#include "rrc_paging_SI.h"
#include "../pkfmt.h"
//#include "rrc_utils.h"


/*definition of states*/
#define state_IDLE_WAIT_SYSINFO 5
#define state_IDLE_NORMALLY 7
#define state_CONNECTED_NORMALLY 11

extern void send_to_user(char *info, int size_info);
//int *state=fsm_current_state_ptr_get();

//fix me: modify the definition of UE_Identity.

/******************************************************************
** Function name: DoReceivePaging
** Descriptions: Handle with the Paging received.
** Input: struct Paging *paging,struct UE_Identity ueIdentity,int *state
** Output:void
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void DoReceivePaging(struct Paging *paging,struct UE_Identity ueIdentity)
{
	FIN(DoReceivePaging());
	fsm_printf("[rrc] [fsm] start to handle paging.\n");
	int *state = fsm_current_state_ptr_get();
	if(*state ==state_IDLE_NORMALLY && paging->havePagingRecord==true)
	{
		CheckPagingRecord(paging->pagingRecordList,ueIdentity);
	}
	
	if (paging->systemInfoModification==true)
	{
		fsm_printf("[rrc] [fsm] paging.systemInfoModification==true, renew the system information from enb.\n");
		Begin_recv_sysinfo();
	}

	fsm_printf("[rrc] [fsm] handle paging successfully.\n");
	FOUT;
}

/******************************************************************
** Function name: CheckPagingRecord
** Descriptions: Check every PagingRecord and send it to the User if necessarily.
** Input:PagingRecordList,UE_Identity
** Output: ue_identity ,cn_Domain
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void CheckPagingRecord(struct PagingRecordList prl,struct UE_Identity ueIdentity)
{
	FIN(CheckPagingRecord());
	fsm_printf("[rrc] [paging] start to send pagingRecord to US.\n");
	int i;
	int *state=fsm_current_state_ptr_get();
	for(i=0;i<prl.num;i++)//Check every pagingRecord.
	{	
		if(*state =state_IDLE_WAIT_SYSINFO &&prl.pagingRecord[i].ue_Identity.choice.s_TMSI.mmec==ueIdentity.choice.s_TMSI.mmec 
			&& prl.pagingRecord[i].ue_Identity.choice.s_TMSI.m_TMSI==ueIdentity.choice.s_TMSI.m_TMSI)//pagingRecord中的ue_Identity与任一个上层分配的UE identitiy相匹配	
		{							
		//	char *data1="PagingRecord : ue_Identity.mmec is ";
		//	char *data2=fsm_mem_alloc(sizeof(u8));
		//	char *data3="; ue_Identity.m_TMSI is :";
		//	char *data4=fsm_mem_alloc(sizeof(u32));
		//	char *data5="; cn_Domain is :";
		//	char *data6=fsm_mem_alloc(2);
		//	int data_size=strlen(data1)+strlen(data2)+strlen(data3)+strlen(data4)+strlen(data5)+strlen(data6)+1;
		//	char *data = (char *)fsm_mem_alloc(data_size);
		//	sprintf(data2,"%d",prl.pagingRecord[i].ue_Identity.choice.s_TMSI.mmec);
		//	sprintf(data4,"%d",prl.pagingRecord[i].ue_Identity.choice.s_TMSI.m_TMSI);
		//	sprintf(data6,"%d",prl.pagingRecord[i].cn_Domain);
		//	fsm_mem_cpy(data, data1, strlen(data1));
		//	fsm_mem_cpy(data+strlen(data1), data2,strlen(data2));
		//	fsm_mem_cpy(data+strlen(data1)+strlen(data2), data3, strlen(data3));
		//	fsm_mem_cpy(data+strlen(data1)+strlen(data2)+strlen(data3),data4, strlen(data4));
		//	fsm_mem_cpy(data+strlen(data1)+strlen(data2)+strlen(data3)+strlen(data4), data5, strlen(data5));
		//	fsm_mem_cpy(data+strlen(data1)+strlen(data2)+strlen(data3)+strlen(data4)+strlen(data5), data6, strlen(data6)+1);
		//	send_to_user(data, data_size); 
		}
				
	}

	fsm_printf("[rrc] [paging] send pagingRecord to US successfully.\n");
	FOUT;	
}

/******************************************************************
** Function name:Begin_recv_sysinfo
** Descriptions: tell MAC to receive systeminfomation
** Input: void
** Output: void
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void Begin_recv_sysinfo(void)
{
	FIN(Begin_recv_sysinfo());
	fsm_printf("[rrc] [paging] tell MAC to receive systeminfomation.\n");
	//fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_BEGIN_RECV_SYSINFO, NULL, 0);
	fsm_printf("[rrc] [paging] tell MAC to receive systeminfomation successfully.\n");
	FOUT;
}

/******************************************************************
** Function name:DoReceiveMIB
** Descriptions: Handle with the MIB received.
** Input: MasterInformationBlock
** Output:void
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void DoReceiveMIB(struct MasterInformationBlock * mib)
{
	FIN(DoReceiveMIB());
	fsm_printf("[rrc] [fsm] start to handle MIB .\n");
	ApplyPhich_Config(mib->phich_Config);
	int *state=fsm_current_state_ptr_get();
	if (checkTimer(TIMER_T311)&&(*state==state_IDLE_NORMALLY||*state==state_CONNECTED_NORMALLY))//如果当 T311正在运行时，UE处在RRC_IDLE状态，或者处于RRC_CONNECTED状态
	{
		//fix me: Judge the validity of the system information(NULL)

		//fix me: dl_Bandwidth will be sent to ??
		//fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_DL_BANDWIDTH,sizeof(mib->dl_Bandwidth));
	}
	fsm_printf("[rrc] [fsm] handle MIB successfully.\n");
	FOUT;
}

/******************************************************************
** Function name:ApplyPhich_Config
** Descriptions:send phich_config to PHY 
** Input:PHICH_Config phich_c
** Output:PHICH_Config(send to PHY )
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyPhich_Config(struct PHICH_Config phich_c)
{

	//Sent to the PHY(NULL)
	
}

/******************************************************************
** Function name:DoReceiveSIB1
** Descriptions: Handle with the SIB1 received.
** Input: struct SystemInformationBlockType1 *sib1
** Output:void
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void DoReceiveSIB1(struct SystemInformationBlockType1 *sib1)
{
	FIN(DoReceiveSIB1());
	fsm_printf("[rrc] [fsm] start to handle SIB1 .\n");
	SendCellAccessRelatedInfotoUS(sib1->cellAccessRelatedInfo);
	fsm_printf("[rrc] [fsm] handle SIB1 successfully.\n");
	FOUT;
}

/******************************************************************
** Function name:SendCellAccessRelatedInfotoUS
** Descriptions:Send information about cell acesss to USER
** Input:CellAccessRelatedInfo
** Output: CellAccessRelatedInfo(send to US)
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void SendCellAccessRelatedInfotoUS(struct CellAccessRelatedInfo cari)
{
	FIN(SendCellAccessRelatedInfotoUS());
	fsm_printf("[rrc] [sib1] start to send CellAccessRelatedInfo to US.\n");
//	char *data1="CellAccessRelatedInfo:cellIdentity is ";
//	char *data2=fsm_mem_alloc(sizeof(u32));
//	char *data3="; trackingAreaCode is ";
//	char *data4=fsm_mem_alloc(sizeof(u16));
//	sprintf(data2,"%d",cari.cellIdentity);
//	sprintf(data4,"%d",cari.trackingAreaCode);
//	int data_size=strlen(data1)+strlen(data2)+strlen(data3)+strlen(data4)+1;
//	char * data = (char *)fsm_mem_alloc(data_size);
//	fsm_mem_cpy(data, data1, strlen(data1));
//	fsm_mem_cpy(data+strlen(data1), data2, strlen(data2));
//	fsm_mem_cpy(data+strlen(data1)+strlen(data2), data3, strlen(data3));
//	fsm_mem_cpy(data+strlen(data1)+strlen(data2)+strlen(data3), data4, strlen(data4)+1);
//	send_to_user(data, data_size);
	fsm_printf("[rrc] [sib1] send CellAccessRelatedInfo to US successfully.\n");
	FOUT;
}

/******************************************************************
** Function name:DoReceiveSIB2
** Descriptions: Handle with the SIB2 received.
** Input: struct SystemInformationBlockType2 sib2
** Output:void
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void DoReceiveSIB2(struct SystemInformationBlockType2 sib2)
{
	fsm_printf("[rrc] [fsm] start to handle SIB2 .\n");
	if(sib2.haveMBSFN_SubframeConfigList==true)
	{
		//fix me:认为在IE mbsfn_SubframeConfigList中表示的MBSFN子帧中没有发生其它下行分配
	}
	
	ApplyRadioResourceConfigCommon(sib2.radioResourceConfigCommon);
	fsm_printf("[rrc] [fsm] handle SIB2 successfully.\n");
}

/******************************************************************
** Function name:ApplyRadioResourceConfigCommon
** Descriptions:Apply the configure in RadioResourceConfigCommon
** Input: RadioResourceConfigCommonSib
** Output: void
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyRadioResourceConfigCommon(struct RadioResourceConfigCommonSib rrcc)
{
	FIN(ApplyRadioResourceConfigCommon());
	ApplyRachConfigCommon(rrcc.rachConfigCommon);
	ApplyBCCH_Config(rrcc.bcch_Config);
	ApplyPCCH_Config(rrcc.pcch_Config);
	FOUT;
}

/******************************************************************
** Function name:ApplyRachConfigCommon
** Descriptions:
** Input: RachConfigCommon
** Output: RachConfigCommon(sent to MAC)
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyRachConfigCommon(struct RachConfigCommon rcc)
{
	FIN(ApplyRachConfigCommon());
	struct RachConfigCommon *rachConfigCommon=NULL;
	rachConfigCommon=fsm_mem_alloc(sizeof(struct RachConfigCommon));
	rachConfigCommon->preambleInfo.numberOfRA_Preambles=rcc.preambleInfo.numberOfRA_Preambles;
	if(rcc.preambleInfo.havePreamblesGroupAConfig=true)//rachConfigCommon中含有PreamblesGroupAConfig信息
	{
	rachConfigCommon->preambleInfo.preamblesGroupAConfig.sizeOfRA_PreamblesGroupA=rcc.preambleInfo.preamblesGroupAConfig.sizeOfRA_PreamblesGroupA;
	rachConfigCommon->preambleInfo.preamblesGroupAConfig.messageSizeGroupA=rcc.preambleInfo.preamblesGroupAConfig.messageSizeGroupA;
	rachConfigCommon->preambleInfo.preamblesGroupAConfig.messagePowerOffsetGroupB=rcc.preambleInfo.preamblesGroupAConfig.messagePowerOffsetGroupB;
	}
	rachConfigCommon->powerRampingParameters.powerRampingStep=rcc.powerRampingParameters.powerRampingStep;
	rachConfigCommon->powerRampingParameters.preambleInitialReceivedTargetPower=rcc.powerRampingParameters.preambleInitialReceivedTargetPower;
	rachConfigCommon->raSupervisionInfo.preambleTransMax=rcc.raSupervisionInfo.preambleTransMax;
	rachConfigCommon->maxHARQ_Msg3Tx=rcc.maxHARQ_Msg3Tx;
	//fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_RACH, (void*)rachConfigCommon, sizeof(struct RachConfigCommon));
	fsm_mem_free(rachConfigCommon);
	FOUT;
}

/******************************************************************
** Function name:ApplyBCCH_Config
** Descriptions:
** Input: BCCH_Config
** Output: BCCH_Config(sent to MAC)
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyBCCH_Config(struct BCCH_Config bc)
{
	FIN(ApplyBCCH_Config());
	//fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_BCCH, (void*)&bc, sizeof(struct BCCH_Config));
	FOUT;
}

/******************************************************************
** Function name:ApplyPCCH_Config
** Descriptions:
** Input: PCCH_Config
** Output: PCCH_Config(sent to MAC)
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyPCCH_Config(struct PCCH_Config pc)
{
	FIN(ApplyPCCH_Config());
	//fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_PCCH, (void*)&pc, sizeof(struct PCCH_Config));
	FOUT;
}

/******************************************************************
** Function name:ApplyPRACH_ConfigSIB
** Descriptions:
** Input:PRACH_ConfigSIB
** Output:
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyPRACH_ConfigSIB(struct PRACH_ConfigSIB prach_c)
{
	//fix me:Sent to the PHY(waited)

}

/******************************************************************
** Function name:ApplyPDSCH_ConfigCommon
** Descriptions:
** Input:PDSCH_ConfigCommon
** Output:
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyPDSCH_ConfigCommon(struct PDSCH_ConfigCommon pdsch_c)
{
	////fix me:Sent to the PHY(waited)

}

/******************************************************************
** Function name:ApplyPUSCH_ConfigCommon
** Descriptions:
** Input:PUSCH_ConfigCommon
** Output:
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyPUSCH_ConfigCommon(struct PUSCH_ConfigCommon pusch_c)
{
	////fix me:Sent to the PHY(waited)

}

/******************************************************************
** Function name:ApplySoundingRS_UL_ConfigCommon
** Descriptions:
** Input:SoundingRS_UL_ConfigCommon
** Output:
** Returns:void
** Created by:wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplySoundingRS_UL_ConfigCommon(struct SoundingRS_UL_ConfigCommon sc)
{
	////fix me:Sent to the PHY(waited)

}

/******************************************************************
** Function name:ApplyUplinkPowerControlCommon
** Descriptions:
** Input: UplinkPowerControlCommon
** Output:
** Returns: void
** Created by: wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyUplinkPowerControlCommon(struct UplinkPowerControlCommon uc)
{
	////fix me:Sent to the PHY(waited)

}

/******************************************************************
** Function name:ApplyUL_CyclicPrefixLength
** Descriptions:
** Input:UL_CyclicPrefixLength
** Output:
** Returns: void
** Created by: wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
void ApplyUL_CyclicPrefixLength(struct UL_CyclicPrefixLength ul_c)
{
	////fix me:Sent to the PHY(waited)

}
/****functions for test****/

/******************************************************************
** Function name:gen_paging
** Descriptions:generate paging packets(for test)
** Input:void
** Output:void
** Returns:pkptr
** Created by: wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
FSM_PKT* gen_paging()
{
	
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
	
	//Generate packets
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);

	//Filling head
	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr == NULL)
			return;
	}
	fsm_skb_push(pkptr, sizeof(struct lte_rrc_head));
	sh_ptr = (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = message_type;
	return pkptr;
	//rrc_messege_handler_rlc(pkptr);
}

/******************************************************************
** Function name:gen_si
** Descriptions:generate SI packets(for test)
** Input: void
** Output: void 
** Returns: pkptr
** Created by: wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
FSM_PKT* gen_si()
{
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
	
	//Generate packets
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr= fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);
	//Filling head
	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr= fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr== NULL)
			return;
	}
	fsm_skb_push(pkptr, sizeof(struct lte_rrc_head));
	sh_ptr= (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = message_type;
	return pkptr;
}

/******************************************************************
** Function name:gen_mib
** Descriptions:generate MIB packets(for test)
** Input: void 
** Output: void
** Returns: pkptr
** Created by: wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
FSM_PKT* gen_mib()
{
	struct MasterInformationBlock mib;
	mib.dl_Bandwidth = n25;
	//mib.phich_Config
	mib.systemFrameNumber = (u8)120;
	struct BCCH_BCH_Message bcch_bch_msg;
	bcch_bch_msg.mib = mib;
	char *msg = (char *)&bcch_bch_msg;
	int msg_len = sizeof(bcch_bch_msg);
	int message_type = 1;

	//Generate packets
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);

	//Filling head
	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr == NULL)
			return;
	}
	fsm_skb_push(pkptr, sizeof(struct lte_rrc_head));
	sh_ptr = (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = message_type;
	return pkptr;
}

/******************************************************************
** Function name:gen_sib1
** Descriptions:generate SIB1 packets(for test)
** Input:void
** Output: void
** Returns: pkptr
** Created by: wang jinchuan
** Created Date: 2014/10/07
** -----------------------------------------------
** Modified by:
** Modified date:
******************************************************************/
FSM_PKT* gen_sib1()
{
	struct SystemInformationBlockType1 sib1;
	sib1.cellAccessRelatedInfo.cellIdentity=(u32)15;
	sib1.cellAccessRelatedInfo.trackingAreaCode=(u16)12;
	struct BCCH_DL_SCH_Message bcch_dl_sch_message;
	bcch_dl_sch_message.type=2;
	bcch_dl_sch_message.msg.sib1=sib1;
	char *msg=(char *)&bcch_dl_sch_message;
	int msg_len=sizeof(bcch_dl_sch_message);
	
	
	//Generate packets
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr= fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);
	
	//Filling head
	if(fsm_skb_headroom(pkptr) < sizeof(struct lte_rrc_head)){
		pkptr= fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_rrc_head));
		if(pkptr== NULL)
			return;
	}
	fsm_skb_push(pkptr, sizeof(struct lte_rrc_head));
	sh_ptr= (struct lte_rrc_head*)pkptr->data;
	sh_ptr->message_type = 2;
	return pkptr;
}
