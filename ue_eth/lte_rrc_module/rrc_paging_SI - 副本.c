#include "../lte_system.h"
#include "rrc_utils.h"
#include "rrc_paging_SI.h"
#include "../pkfmt.h"

extern void send_to_user(char *info, int size_info);

//根据接收到的paging设置ue_Identity和 cn_Domainset
//**************修改ueIdentity的定义方式
void CheckPagingRecord(struct PagingRecordList prl,struct UE_Identity ueIdentity)
{
	FIN(CheckPagingRecord());
	fsm_printf("[rrc] [paging] start to send pagingRecord to US.\n");
	int i;
	int *state=fsm_current_state_ptr_get();
	for(i=0;i<prl.num;i++)
	{	
		if(*state =5 &&prl.pagingRecord[i].ue_Identity.choice.s_TMSI.mmec==ueIdentity.choice.s_TMSI.mmec && prl.pagingRecord[i].ue_Identity.choice.s_TMSI.m_TMSI==ueIdentity.choice.s_TMSI.m_TMSI)	
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
void Begin_recv_sysinfo(void)
{
	FIN(Begin_recv_sysinfo());
	fsm_printf("[rrc] [paging] tell MAC to receive systeminfomation.\n");
    fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_BEGIN_RECV_SYSINFO, NULL, 0);
	fsm_printf("[rrc] [paging] tell MAC to receive systeminfomation successfully.\n");
	FOUT;
}
void DoReceiveMIB(struct MasterInformationBlock * mib)
{
	FIN(DoReceiveMIB());
	fsm_printf("[rrc] [paging] start to handle MIB .\n");
	ApplyPhich_Config(mib->phich_Config);
	u8 m_dlBandwidth;
	int *state=fsm_current_state_ptr_get();
	if (T311!=NULL&&(*state==7||*state==11))
	{
		//判断系统信息的有效性???????????
		m_dlBandwidth=mib->dl_Bandwidth;//?????dl_Bandwidth给谁使用?????????	
	}
	fsm_printf("[rrc] [paging] handle MIB successfully.\n");
	FOUT;
}

void SendCellAccessRelatedInfotoUS(struct CellAccessRelatedInfo cari)
{
	FIN(SendCellAccessRelatedInfotoUS());
	fsm_printf("[rrc] [paging] start to send CellAccessRelatedInfo to US.\n");
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
	fsm_printf("[rrc] [paging] send CellAccessRelatedInfo to US successfully.\n");
	FOUT;
}


void ApplyPhich_Config(struct PHICH_Config phich_c)//发送给物理层，待定
{

	//发送给物理层，待定
	
}
void ApplyRadioResourceConfigCommon(struct RadioResourceConfigCommonSib rrcc)
{
	FIN(ApplyRadioResourceConfigCommon());
	ApplyRachConfigCommon(rrcc.rachConfigCommon);
	ApplyBCCH_Config(rrcc.bcch_Config);
	ApplyPCCH_Config(rrcc.pcch_Config);
	FOUT;
}

void ApplyRachConfigCommon(struct RachConfigCommon rcc)
{
	FIN(ApplyRachConfigCommon());
	struct RachConfigCommon *rachConfigCommon=NULL;
	rachConfigCommon=fsm_mem_alloc(sizeof(struct RachConfigCommon));
	rachConfigCommon->preambleInfo.numberOfRA_Preambles=rcc.preambleInfo.numberOfRA_Preambles;
	if(rcc.preambleInfo.havePreamblesGroupAConfig=true)
	{
	rachConfigCommon->preambleInfo.preamblesGroupAConfig.sizeOfRA_PreamblesGroupA=rcc.preambleInfo.preamblesGroupAConfig.sizeOfRA_PreamblesGroupA;
	rachConfigCommon->preambleInfo.preamblesGroupAConfig.messageSizeGroupA=rcc.preambleInfo.preamblesGroupAConfig.messageSizeGroupA;
	rachConfigCommon->preambleInfo.preamblesGroupAConfig.messagePowerOffsetGroupB=rcc.preambleInfo.preamblesGroupAConfig.messagePowerOffsetGroupB;
	}
	rachConfigCommon->powerRampingParameters.powerRampingStep=rcc.powerRampingParameters.powerRampingStep;
	rachConfigCommon->powerRampingParameters.preambleInitialReceivedTargetPower=rcc.powerRampingParameters.preambleInitialReceivedTargetPower;
	rachConfigCommon->raSupervisionInfo.preambleTransMax=rcc.raSupervisionInfo.preambleTransMax;
	rachConfigCommon->maxHARQ_Msg3Tx=rcc.maxHARQ_Msg3Tx;
	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_RACH, (void*)&rcc, sizeof(struct RachConfigCommon));
	fsm_mem_free(rachConfigCommon);
	FOUT;
}

void ApplyBCCH_Config(struct BCCH_Config bc)
{
	FIN(ApplyBCCH_Config());
	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_BCCH, (void*)&bc, sizeof(struct BCCH_Config));
	FOUT;
}

void ApplyPCCH_Config(struct PCCH_Config pc)
{
	FIN(ApplyPCCH_Config());
	fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_RRCTOMAC_CONFIG_PCCH, (void*)&pc, sizeof(struct PCCH_Config));
	FOUT;
}
void ApplyPRACH_ConfigSIB(struct PRACH_ConfigSIB prach_c)
{
	//发送给物理层，待定

}
void ApplyPDSCH_ConfigCommon(struct PDSCH_ConfigCommon pdsch_c)
{
	//发送给物理层，待定

}
void ApplyPUSCH_ConfigCommon(struct PUSCH_ConfigCommon pusch_c)
{
	//发送给物理层，待定

}
void ApplySoundingRS_UL_ConfigCommon(struct SoundingRS_UL_ConfigCommon sc)
{
	//发送给物理层，待定

}
void ApplyUplinkPowerControlCommon(struct UplinkPowerControlCommon uc)
{
	//发送给物理层，待定

}
void ApplyUL_CyclicPrefixLength(struct UL_CyclicPrefixLength ul_c)
{
	//发送给物理层，待定

}
/****functions for test****/
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
	
	//产生报文
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);

	//填充头部
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
	
	//产生报文
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr= fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);
	//填充头部
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

	//产生报文
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);

	//填充头部
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
	
	
	//产生报文
	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr;
	pkptr= fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head));  
	fsm_skb_put(pkptr, msg_len);
	fsm_mem_cpy(pkptr->data, msg, msg_len);
	
	//填充头部
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
