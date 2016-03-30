#include "../lte_system.h"
#include "../fsm/fsmdec.h"
#include "rrc_utils.h"
#include "rrc_conn_reconfig_enb.h"


// defined in rrc_utils.c
extern struct LteSignalingRadioBearerInfo* enbSRBConfig[MAX_UEFSM_NUM][MAX_SRB_NUM];
extern struct LteDataRadioBearerInfo* enbDRBConfig[MAX_UEFSM_NUM][MAX_DRB_NUM];

//evaluate the struct of RLC_Config_am content
static struct RLC_Config_am value_struct_rlcconfigam(void){
	FIN(value_struct_rlcconfigam());
	//AM down link
	struct DL_AM_RLC  dl_am_rlc={
		.t_Reordering.t_Reordering=t_Reordering_ms200,
		.t_StatusProhibit.t_StatusProhibit=t_StatusProhibit_ms200,
	};
	//AM up link
	struct UL_AM_RLC  ul_am_rlc={
		.t_PollRetransmit.t_PollRetransmittype=ms200,
		.pollPDU.pollPDUtype=pInfinity,
		.pollByte.pollByte=kBinfinity,
		.maxRetxThreshold=t4,
	};
	struct RLC_Config_am am={
		.ul_AM_RLC=ul_am_rlc,
		.dl_AM_RLC=dl_am_rlc,
	};
	return am;
	FOUT;
}
//evaluate the struct of RLC_Config_um_Bi_Directional
static struct RLC_Config_um_Bi_Directional value_struct_umBiDiretion(void){
	FIN(value_struct_umBiDiretion());
	//um down link
	struct DL_UM_RLC  dl_um_rlc={
		.sn_FieldLength.sn_FieldLength=size10,
		.t_Reordering.t_Reordering=t_Reordering_ms200,
	};
	//um up link
	struct UL_UM_RLC ul_um_rlc={
		.sn_FieldLength.sn_FieldLength=size5,
	};
	struct RLC_Config_um_Bi_Directional um_bi_directional={
		.ul_UM_RLC=ul_um_rlc,
		.dl_UM_RLC=dl_um_rlc,
	};
	return um_bi_directional;
	FOUT;
}
//evaluate the UL struct of RLC_Config_um_Bi_Directional
static struct RLC_Config_um_Bi_Directional value_struct_ULumBiDrection(void){
	FIN(value_struct_ULumBiDrection());
	//um up link
	struct UL_UM_RLC  ul_um_rlc={
		.sn_FieldLength.sn_FieldLength=size5,
	};
	struct RLC_Config_um_Bi_Directional  um_bi_directional={
		.ul_UM_RLC=ul_um_rlc,
	};
	return um_bi_directional;
	FOUT;
}
//evaluate the DL struct of RLC_Config_um_Bi_Directional
static struct RLC_Config_um_Bi_Directional value_struct_DLumBiDrection(void){
	FIN(value_struct_DLumBiDrection());
	//um down link
	struct DL_UM_RLC  dl_um_rlc={
		.sn_FieldLength.sn_FieldLength=size10,
		.t_Reordering.t_Reordering=t_Reordering_ms100,
	};
	struct RLC_Config_um_Bi_Directional  um_bi_directional={
		.dl_UM_RLC=dl_um_rlc,
	};
	return um_bi_directional;
	FOUT;
}
//evaluate the struct of LogicalChannelConfig
static struct LogicalChannelConfig value_struct_lcc(void){
	FIN(value_struct_lcc());
	struct Ul_SpecificParameters ul_specparas={
		.priority=3,
		.prioritisedBitRate=infinity,
		.bucketSizeDuration=bucketSizeDuration_ms300,
		.logicalChannelGroup=0,
	};
	struct LogicalChannelConfig lcc={
		.haveUl_SpecificParameters=1,
		.ul_SpecificParameters=ul_specparas,
	};
	return lcc;
	FOUT;
}
//evaluate the struct of SrbToAddMod
static struct SrbToAddMod value_struct_srbmod(int rlcmod,int srbid){
	FIN(value_struct_srbmod());
	struct RlcConfig rlcConfig;
	rlcConfig.type=rlcmod;
	switch(rlcConfig.type)
	{
		case 1:
			rlcConfig.rlcConfigType.am=value_struct_rlcconfigam();
			break;
		case 2:
			rlcConfig.rlcConfigType.um_Bi_Directional=value_struct_umBiDiretion();
			break;
		case 3:
			rlcConfig.rlcConfigType.um_Bi_Directional=value_struct_ULumBiDrection();
			break;
		case 4:
			rlcConfig.rlcConfigType.um_Bi_Directional=value_struct_DLumBiDrection();
			break;
	}
	struct SrbToAddMod srb_to_add_mod={
		.srbIdentity=srbid,
		.haveRlcConfig=1,
		.rlcConfig=rlcConfig,
		.haveLogicalChannelConfig=1,
		.logicalChannelConfig=value_struct_lcc(),
	};
	return srb_to_add_mod;
	FOUT;
}
//evaluate the struct of SrbToAddModList
struct SrbToAddModList value_struct_srblist(int srbtype){
	FIN(value_struct_srblist());
	struct SrbToAddModList srb_to_addmod_list;
	switch(srbtype)
	{
		case 1:
			srb_to_addmod_list.num = 1;
			srb_to_addmod_list.srbList[0] = value_struct_srbmod(1,1);//am,srb1
			break;
		case 2:
			srb_to_addmod_list.num = 1;
			srb_to_addmod_list.srbList[0] = value_struct_srbmod(1,2);//am,srb2
			break;
		case 3:
			srb_to_addmod_list.num = 2;
			srb_to_addmod_list.srbList[0] = value_struct_srbmod(1,1);
			srb_to_addmod_list.srbList[1] = value_struct_srbmod(1,2);
			break;
	}
	return srb_to_addmod_list;
	FOUT;
}
//evaluate the struct of DrbToAddModList drb0
struct DrbToAddModList build_drbs(void)
{
	FIN(build_drbs());
	SV_PTR_GET(rrc_sv_enb_ue);
	int i;
	int drb_num;
	struct LteDataRadioBearerInfo **drb_list;
	struct DrbToAddModList drbToAddModList;

	drb_list = enbDRBConfig[SV(uefsmid)];
	drb_num = 0;
	drbToAddModList.num = 0;
	for(i = 0; i < MAX_DRB_NUM; i ++)
	{
		if(drb_list[i] != NULL)
		{
			drb_num ++;
			// construct DrbToAddModList;
			drbToAddModList.num ++;
			drbToAddModList.drbList[i].eps_BearerIdentity = drb_list[i]->eps_BearerIdentity; // fix me: set eps_BearerIdentity.
			drbToAddModList.drbList[i].drb_Identity = drb_list[i]->drbIdentity;
			drbToAddModList.drbList[i].haveRlcConfig = 1;
			drbToAddModList.drbList[i].rlcConfig = drb_list[i]->rlcConfig;
			drbToAddModList.drbList[i].logicalChannelIdentity = drb_list[i]->lcid;
			drbToAddModList.drbList[i].logicalChannelConfig = drb_list[i]->logicalChannelConfig;

		}
	}

	fsm_printf("[rrc] [build_drbs] 	drbToAddModList.num = %d\n", drbToAddModList.num );
	if(drb_num == 0)
	{
		fsm_printf("[rrc] no drb exits in enbDRBConfig, build drb0 for default.\n");
		struct RlcConfig rlcConfig={
			.type=1,
			.rlcConfigType.am=value_struct_rlcconfigam(),
		};
		struct DrbToAddMod drb0_to_add_mod={
			.drb_Identity=0,
			.haveRlcConfig=1,
			.rlcConfig=rlcConfig,
			.haveLogicalChannelConfig=1,
			.logicalChannelConfig=value_struct_lcc(),
		};

		drbToAddModList.num = 1;
		drbToAddModList.drbList[0]=drb0_to_add_mod;
	}

	return drbToAddModList;
	FOUT;
}

//generate RRCConnectionReconfiguration message
struct DL_DCCH_Message *gen_RRCConnectionReconfigurationMessage(int srb_or_drb,int srbtype)
{
	SV_PTR_GET(rrc_sv_enb_ue);
	struct RadioResourceConfigDedicated radioResConfDedic;
	struct RRCConnectionReconfiguration *reconfig_msg;
	struct DL_DCCH_Message *dl_dcch_message;
	int i;
	int haveDrbToAddMod;

	reconfig_msg = fsm_mem_alloc(sizeof(struct RRCConnectionReconfiguration));
	dl_dcch_message=(struct DL_DCCH_Message  *)fsm_mem_alloc(sizeof(struct DL_DCCH_Message ));
	switch(srb_or_drb)
	{
		case 1:
			//generate srbToAddModList
			radioResConfDedic.haveSrbToAddModList=1;
			radioResConfDedic.srbToAddModList=value_struct_srblist(srbtype);
			radioResConfDedic.haveDrbToReleaseList = 0;
			break;
		case 2:
			//generate drbToAddModList
			haveDrbToAddMod = 0;
			for(i = 0; i < MAX_DRB_NUM; i ++)
			{
				if(enbDRBConfig[SV(uefsmid)][i] != NULL)
				{
					haveDrbToAddMod = 1;
					break;
				}
			}

			if(haveDrbToAddMod)
			{
				radioResConfDedic.haveDrbToAddModList = 1;
				radioResConfDedic.drbToAddModList = build_drbs();
				fsm_printf("[rrc] [gen_RRCConnectionReconfigurationMessage] drbToAddModList.num = %d\n", radioResConfDedic.drbToAddModList.num);
				radioResConfDedic.haveSrbToAddModList = 0;
			}
			else
				radioResConfDedic.haveDrbToAddModList = 0;


			break;
	}

	radioResConfDedic.haveDrbToReleaseList = 0;
	radioResConfDedic.haveMAC_MainConfig = 0;
	radioResConfDedic.haveSPS_Config = 0;
	radioResConfDedic.havePhysicalConfigDedicated = 0;
	//msg->rrcTransactionIdentifier=?;
	reconfig_msg->haveRadioResourceConfigDedicated=1;
	reconfig_msg->radioResourceConfigDedicated=radioResConfDedic;
	fsm_printf("		gen_RRCConnectionReconfigurationMessage().\n");//test

	dl_dcch_message->type = 1;
	fsm_mem_cpy(&(dl_dcch_message->msg.rrcConnectionReconfiguration), reconfig_msg, sizeof(struct RRCConnectionReconfiguration));
	fsm_mem_free(reconfig_msg);
	return dl_dcch_message;
}

//重配置过程子函数的实现
// struct RRCConnectionReconfiguration *rrcConnectionReconfiguration;//has defined in the front of rrcfsm.c

//extract srb information for the need of rlc and mac
static void ExtractSrbInfo(struct SrbToAddMod srb,struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg, struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg)
{
	FIN(ExtractSrbInfo());
	SV_PTR_GET(rrc_sv_enb_ue);
	crlc_configreq_ioctlmsg->rbIdentity=srb.srbIdentity;
	crlc_configreq_ioctlmsg->lcIdentity=srb.srbIdentity;
	crlc_configreq_ioctlmsg->Mode=srb.rlcConfig.type;
	crlc_configreq_ioctlmsg->crnti = SV(crnti);

	mac_lcc_ioctlmsg->logicalChannelConfig=srb.logicalChannelConfig;
	mac_lcc_ioctlmsg->logicalChannelIdentity=srb.srbIdentity; //INTEGER(1,2)
	mac_lcc_ioctlmsg->crnti = SV(crnti);

	switch(srb.rlcConfig.type)
	{
		case 1:
			//AM down link
			crlc_configreq_ioctlmsg->amDwParas.timerStatusProhibit=*(unsigned int*)&srb.rlcConfig.rlcConfigType.am.dl_AM_RLC.t_StatusProhibit;
			crlc_configreq_ioctlmsg->amDwParas.timerReordering=*(unsigned int*)&srb.rlcConfig.rlcConfigType.am.dl_AM_RLC.t_Reordering;

			//AM up link
			crlc_configreq_ioctlmsg->amUpParas.timerPollRetransmit=*(unsigned int*)&srb.rlcConfig.rlcConfigType.am.ul_AM_RLC.t_PollRetransmit;
			crlc_configreq_ioctlmsg->amUpParas.PollPDU=*(unsigned int*)&srb.rlcConfig.rlcConfigType.am.ul_AM_RLC.pollPDU;
			crlc_configreq_ioctlmsg->amUpParas.PollBYTE=*(unsigned int*)&srb.rlcConfig.rlcConfigType.am.ul_AM_RLC.pollByte;
			crlc_configreq_ioctlmsg->amUpParas.maxRetxThreshold=*(unsigned int*)&srb.rlcConfig.rlcConfigType.am.ul_AM_RLC.maxRetxThreshold;
			break;
		case 2:
			//um down link
			crlc_configreq_ioctlmsg->umDwParas.snFiledLength=*(unsigned int*)&srb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.sn_FieldLength;
			crlc_configreq_ioctlmsg->umDwParas.timerReordering=*(unsigned int*)&srb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.t_Reordering;

			//um up link
			crlc_configreq_ioctlmsg->umUpParas.snFiledLength=*(unsigned int*)&srb.rlcConfig.rlcConfigType.um_Bi_Directional.ul_UM_RLC.sn_FieldLength;
			break;
		case 3:
			//um up link
			crlc_configreq_ioctlmsg->umUpParas.snFiledLength=*(unsigned int*)&srb.rlcConfig.rlcConfigType.um_Bi_Directional.ul_UM_RLC.sn_FieldLength;
			break;
		case 4:
			//um down link
			crlc_configreq_ioctlmsg->umDwParas.snFiledLength=*(unsigned int*)&srb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.sn_FieldLength;
			crlc_configreq_ioctlmsg->umDwParas.timerReordering=*(unsigned int*)&srb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.t_Reordering;
			break;

	}

	fsm_printf("		ExtractSrbInfo().\n");//test
	FOUT;
}

//extract drb information for the need of mac
static void ExtractDrbMacInfo(struct DrbToAddMod drb,struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg)
{
	FIN(ExtractDrbMacInfo());
	SV_PTR_GET(rrc_sv_enb_ue);
	mac_lcc_ioctlmsg->logicalChannelConfig=drb.logicalChannelConfig;
	mac_lcc_ioctlmsg->logicalChannelIdentity=drb.drb_Identity; //INTEGER(1,2)
	mac_lcc_ioctlmsg->crnti = SV(crnti);
	fsm_printf("		ExtractDrbMacInfo().\n");//test
	FOUT;
}
//extract drb information for the need of rlc
static void ExtractDrbRlcInfo(struct DrbToAddMod drb,struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg)
{
	FIN(ExtractDrbInfo());
	SV_PTR_GET(rrc_sv_enb_ue);
	crlc_configreq_ioctlmsg->rbIdentity=drb.drb_Identity;
	crlc_configreq_ioctlmsg->lcIdentity=drb.logicalChannelIdentity;
	crlc_configreq_ioctlmsg->Mode=drb.rlcConfig.type;
	crlc_configreq_ioctlmsg->crnti = SV(crnti);

	switch(drb.rlcConfig.type)
	{
		case 1:
			//AM down link
			crlc_configreq_ioctlmsg->amDwParas.timerStatusProhibit=*(unsigned int*)&drb.rlcConfig.rlcConfigType.am.dl_AM_RLC.t_StatusProhibit;
			crlc_configreq_ioctlmsg->amDwParas.timerReordering=*(unsigned int*)&drb.rlcConfig.rlcConfigType.am.dl_AM_RLC.t_Reordering;

			//AM up link
			crlc_configreq_ioctlmsg->amUpParas.timerPollRetransmit=*(unsigned int*)&drb.rlcConfig.rlcConfigType.am.ul_AM_RLC.t_PollRetransmit;
			crlc_configreq_ioctlmsg->amUpParas.PollPDU=*(unsigned int*)&drb.rlcConfig.rlcConfigType.am.ul_AM_RLC.pollPDU;
			crlc_configreq_ioctlmsg->amUpParas.PollBYTE=*(unsigned int*)&drb.rlcConfig.rlcConfigType.am.ul_AM_RLC.pollByte;
			crlc_configreq_ioctlmsg->amUpParas.maxRetxThreshold=*(unsigned int*)&drb.rlcConfig.rlcConfigType.am.ul_AM_RLC.maxRetxThreshold;
			break;
		case 2:
			//um down link
			crlc_configreq_ioctlmsg->umDwParas.snFiledLength=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.sn_FieldLength;
			crlc_configreq_ioctlmsg->umDwParas.timerReordering=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.t_Reordering;

			//um up link
			crlc_configreq_ioctlmsg->umUpParas.snFiledLength=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.ul_UM_RLC.sn_FieldLength;
			break;
		case 3:
			//um up link
			crlc_configreq_ioctlmsg->umUpParas.snFiledLength=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.ul_UM_RLC.sn_FieldLength;
			break;
		case 4:
			//um down link
			crlc_configreq_ioctlmsg->umDwParas.snFiledLength=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.sn_FieldLength;
			crlc_configreq_ioctlmsg->umDwParas.timerReordering=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.t_Reordering;
			break;

	}
	fsm_printf("		ExtractDrbRlcInfo().\n");//test
	FOUT;
}

//add a srb
static void AddModSrb(struct SrbToAddMod srb,int add_or_mod)
{
	FIN(AddModSrb());
	struct CRLC_ConfigReq_IoctrlMsg crlc_configreq_ioctlmsg;
	struct MAC_LogicalChannelConfig_IoctrlMsg mac_lcc_ioctlmsg;
	int ioccmdToRLC;

	ExtractSrbInfo(srb,&crlc_configreq_ioctlmsg,&mac_lcc_ioctlmsg);

	ioccmdToRLC=(add_or_mod==0? IOCCMD_RRCTORLC_BUILD:IOCCMD_RRCTORLC_RECONFIG);
	//fix me:establish PDCP entity
	fsm_printf("		ioccmdToRLC=%d\n",ioccmdToRLC);
	fsm_printf("		CRLC.ID=%d\n",crlc_configreq_ioctlmsg.lcIdentity);//test
	fsm_printf("		CRLC.MODE=%d\n",crlc_configreq_ioctlmsg.Mode);//test
	//send ioctl msg for rlc to establish/reconfigure srb
	fsm_do_ioctrl(STRM_TO_RLC,ioccmdToRLC,(void*)&crlc_configreq_ioctlmsg,sizeof(struct CRLC_ConfigReq_IoctrlMsg));

	//send ioctl msg for mac to establish/reconfigure DCCH
	fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_LCC,(void*)&mac_lcc_ioctlmsg,sizeof(struct MAC_LogicalChannelConfig_IoctrlMsg));

	fsm_printf("		srb.srbIdentity=%d.\n",srb.srbIdentity);
	fsm_printf("		AddModSrb().\n");//test
	FOUT;
}

//add a drb
static void AddModDrb(struct DrbToAddMod drb,int add_or_mod)
{
	FIN(AddModSrb());
	struct CRLC_ConfigReq_IoctrlMsg crlc_configreq_ioctlmsg;
	struct MAC_LogicalChannelConfig_IoctrlMsg mac_lcc_ioctlmsg;
	int ioccmdToRLC;

	ioccmdToRLC=(add_or_mod==0? IOCCMD_RRCTORLC_BUILD:IOCCMD_RRCTORLC_RECONFIG);
	if(ioccmdToRLC==IOCCMD_RRCTORLC_BUILD){
		//fix me:establish PDCP entity


		//send ioctl msg for rlc to establish srb
		ExtractDrbRlcInfo(drb,&crlc_configreq_ioctlmsg);
		fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_RRCTORLC_BUILD, &crlc_configreq_ioctlmsg,sizeof(struct CRLC_ConfigReq_IoctrlMsg));
		//send ioctl msg for mac to establish DCCH
		ExtractDrbMacInfo(drb,&mac_lcc_ioctlmsg);
		fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_LCC, &mac_lcc_ioctlmsg,sizeof(struct MAC_LogicalChannelConfig_IoctrlMsg));
		fsm_printf("		CRLC.rbID=%d\n",crlc_configreq_ioctlmsg.rbIdentity);//test
		fsm_printf("		CRLC.MODE=%d\n",crlc_configreq_ioctlmsg.Mode);//test
	}
	else{
		//fix me:if have pdcpconfig,reconfig PDCP entity


		//send ioctl msg for rlc to reconfigure srb
		if(drb.haveRlcConfig){
			ExtractDrbRlcInfo(drb,&crlc_configreq_ioctlmsg);
			fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_RRCTORLC_RECONFIG, &crlc_configreq_ioctlmsg,sizeof(struct CRLC_ConfigReq_IoctrlMsg));
		}
		//send ioctl msg for mac to reconfigure DCCH
		if(drb.haveLogicalChannelConfig){
			ExtractDrbMacInfo(drb,&mac_lcc_ioctlmsg);
			fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_LCC,&mac_lcc_ioctlmsg,sizeof(struct MAC_LogicalChannelConfig_IoctrlMsg));
		}
	}
	fsm_printf("		AddModDrb().\n");//test
	FOUT;
}
//add the srb (srb1/srb2)
void ApplySrbToAdd(struct SrbToAddModList srb)
{
	/*struct CRLC_Confiuefsmid crlc_configreq_ioctlmsg;	*/
	/*struct MAC_LogicalChannelConfig_IoctrlMsg mac_lcc_ioctlmsg;*/
	FIN(ApplySrbToAdd());
	SV_PTR_GET(rrc_sv_enb_ue);
	struct SrbToAddMod eachSrb;
	struct LteSignalingRadioBearerInfo *localSrbConfig=NULL;
	int srbIndex;
	fsm_printf("		srb.num=%d.\n",srb.num);
	for(srbIndex = 0; srbIndex < srb.num; srbIndex ++)
	{
		eachSrb = srb.srbList[srbIndex];

		//if srbid = 1(srb1), continue. srb1 has been build at initial stage.
		/////////////////////////////
		if(eachSrb.srbIdentity == 1)
			continue;
		/////////////////////////////

		fsm_printf("		SRB.ID=%d\n", eachSrb.srbIdentity);//test
		localSrbConfig=enbSRBConfig[SV(uefsmid)][eachSrb.srbIdentity-1];
		if(localSrbConfig==NULL)
		{
			localSrbConfig=fsm_mem_alloc(sizeof(struct LteSignalingRadioBearerInfo));

			//build  srb
			AddModSrb(eachSrb,0);

			//fix me:srb status managment
			localSrbConfig->srbStatus=1;//0,hang up
			localSrbConfig->srbIdentity=eachSrb.srbIdentity;
			localSrbConfig->lcid=(u8)eachSrb.srbIdentity;

			//back up older config for recover the SRB hanged up
			localSrbConfig->rlcConfig=eachSrb.rlcConfig;
			localSrbConfig->logicalChannelConfig=eachSrb.logicalChannelConfig;
			enbSRBConfig[SV(uefsmid)][eachSrb.srbIdentity-1]=localSrbConfig;
			fsm_printf("		add a srb ,srbid=%d\n",enbSRBConfig[SV(uefsmid)][eachSrb.srbIdentity-1]->srbIdentity);//test
		}
		else
		{
			//reconfig the srb
			AddModSrb(eachSrb,1);

			//back up older config for recover the SRB hanged up
			localSrbConfig->rlcConfig=eachSrb.rlcConfig;
			localSrbConfig->logicalChannelConfig=eachSrb.logicalChannelConfig;
			enbSRBConfig[SV(uefsmid)][eachSrb.srbIdentity-1]=localSrbConfig;
			fsm_printf("		reconfig a srb ,srbid=%d\n",enbSRBConfig[SV(uefsmid)][eachSrb.srbIdentity-1]->srbIdentity);//test
		}

	}

	fsm_printf("		ApplySrbToAdd() is done!.\n");//test
	FOUT;
}
//release srb1 and srb2
void ApplySrbToRelease(void)
{
	FIN(ApplySrbToAdd());
	SV_PTR_GET(rrc_sv_enb_ue);
	int srbIndex;
	int srbid;
	int logicalChannelIdentity;
	CRLC_DeactReq_IoctrlMsg deactReqIoctl;
	struct MAC_Release_LogicalChannel_IoctrlMsg releaseLCIoctl;
	for(srbIndex=0;srbIndex<2;srbIndex++)
	{
		srbid = srbIndex+1;
		logicalChannelIdentity= srbid;
		if(enbSRBConfig[SV(uefsmid)][srbIndex]!=NULL){
			//fix me:release pdcp

			deactReqIoctl.rnti = SV(crnti);
			deactReqIoctl.rbIdentity = srbid;
			releaseLCIoctl.crnti = SV(crnti);
			releaseLCIoctl.logicalChannelIdentity = logicalChannelIdentity;

			//release rlc entity
			fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_RRCTORLC_DEACT,&deactReqIoctl,sizeof(CRLC_DeactReq_IoctrlMsg));

			//fix me:release DTCH

			fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_RELEASE_LCC,&releaseLCIoctl,sizeof(struct MAC_Release_LogicalChannel_IoctrlMsg));

			//release memory and pointer
			fsm_mem_free(enbSRBConfig[SV(uefsmid)][srbIndex]);
			enbSRBConfig[SV(uefsmid)][srbIndex]=NULL;
			fsm_printf("		srb.srbToRelease=SRB%d\n",srbid);//test
		}
	}
	fsm_printf("		ApplySrbToRelease() is done!.\n");//test
	FOUT;
}

//add the drb
void ApplyDrbToAdd(struct DrbToAddModList drb)
{
	FIN(ApplyDrbToAdd());
	SV_PTR_GET(rrc_sv_enb_ue);
	//struct CRLC_ConfigReq_IoctrlMsg crlc_configreq_ioctlmsg;
	//struct MAC_LogicalChannelConfig_IoctrlMsg mac_lcc_ioctlmsg;
	struct LteDataRadioBearerInfo *localDrbConfig;
	struct DrbToAddMod eachDrb;
	int drbIndex;
	fsm_printf("[rrc] [ApplyDrbToAdd] drb.num = %d\n", drb.num);
	for(drbIndex = 0; drbIndex < drb.num; drbIndex ++)
	{
		eachDrb=drb.drbList[drbIndex];
//		localDrbConfig=enbDRBConfig[SV(uefsmid)][eachDrb.drb_Identity];
		//ExtractDrbInfo(eachDrb,&crlc_configreq_ioctlmsg,&mac_lcc_ioctlmsg);
		AddModDrb(eachDrb, 0);
	/*
		if(localDrbConfig==NULL)
		{
			localDrbConfig=fsm_mem_alloc(sizeof(struct LteDataRadioBearerInfo));

			//build the drb
			AddModDrb(eachDrb,0);

			//drb status managment
			localDrbConfig->drbStatus=1;
			localDrbConfig->drbIdentity=eachDrb.drb_Identity;
			localDrbConfig->lcid=eachDrb.logicalChannelIdentity;

			//back up older config for recover the DRB hanged up
			localDrbConfig->rlcConfig=eachDrb.rlcConfig;
			localDrbConfig->logicalChannelConfig=eachDrb.logicalChannelConfig;
			enbDRBConfig[SV(uefsmid)][eachDrb.drb_Identity]=localDrbConfig;

			fsm_printf("		add a drb ,drbid=%d\n",eachDrb.drb_Identity);//test
		}
		else
		{
			//reconfig the drb
			AddModDrb(eachDrb,1);

			//back up older config for recover the DRB hanged up
			localDrbConfig->rlcConfig=eachDrb.rlcConfig;
			localDrbConfig->logicalChannelConfig=eachDrb.logicalChannelConfig;
			enbDRBConfig[SV(uefsmid)][eachDrb.drb_Identity]=localDrbConfig;
			fsm_printf("		reconfig a drb ,drbid=%d\n",enbDRBConfig[SV(uefsmid)][eachDrb.drb_Identity]->drbIdentity);//test
		}
	*/

		//*****full config ******//
		//if(fullConfig){
		//}
		//else{
		//}
	}
	fsm_printf("		ApplyDrbToAdd() is done!.\n");//test
	FOUT;
}


//release the drb
void ApplyDrbToRelease(struct DrbToReleaseList drb)
{
	FIN(ApplyDrbToRelease());
	SV_PTR_GET(rrc_sv_enb_ue);
	int drbIndex;
	CRLC_DeactReq_IoctrlMsg deactReqIoctl;
	struct MAC_Release_LogicalChannel_IoctrlMsg releaseLCIoctl;
	for(drbIndex=0; drbIndex<drb.num;drbIndex++){
		DRB_Identity drbid=drb.drbToRelease[drbIndex];
		int logicalChannelIdentity = drbid+3;//DRB0-->lcid=3,DRB1-->lcid=4,.....
		if(enbDRBConfig[SV(uefsmid)][drbid]!=NULL){
			//fix me:release pdcp

			deactReqIoctl.rnti = SV(crnti);
			deactReqIoctl.rbIdentity = drbid;
			releaseLCIoctl.crnti = SV(crnti);
			releaseLCIoctl.logicalChannelIdentity = logicalChannelIdentity;

			//release rlc entity
			fsm_do_ioctrl(STRM_TO_RLC, IOCCMD_RRCTORLC_DEACT,&deactReqIoctl,sizeof(CRLC_DeactReq_IoctrlMsg));

			//fix me:release DTCH

			fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_RELEASE_LCC,&releaseLCIoctl,sizeof(struct MAC_Release_LogicalChannel_IoctrlMsg));

			//release memory and pointer
			fsm_mem_free(enbDRBConfig[SV(uefsmid)][drbid]);
			enbDRBConfig[SV(uefsmid)][drbid]=NULL;
			fsm_printf("		drb.drbToRelease=DRB%d\n",drbid);//test
		}
	}
	fsm_printf("		ApplyDrbToRelease() is done!.\n");//test
	FOUT;
}

static void ApplyRadioResourceConfigDedicated (struct RadioResourceConfigDedicated rrcd)
{
	FIN(ApplyRadioResourceConfigDedicated());
	fsm_printf("[rrc] rrcd.haveSrbToAddModList = %d.\n",rrcd.haveSrbToAddModList);
	fsm_printf("[rrc] rrcd.haveDrbToAddModList = %d.\n",rrcd.haveDrbToAddModList);
	if(rrcd.haveSrbToAddModList) {
		fsm_printf("[rrc] [conn_reconfig] ApplySrbToAdd!\n");//test
		ApplySrbToAdd(rrcd.srbToAddModList);
	}
	if(rrcd.haveDrbToAddModList) {
		fsm_printf("[rrc] [conn_reconfig] ApplyDrbToAdd!\n");//test
		fsm_printf("[rrc] [conn_reconfig] ApplyDrbToAdd, rrcd.drbToAddModList.num = %d!\n", rrcd.drbToAddModList.num);//test
		ApplyDrbToAdd(rrcd.drbToAddModList);
	}
	if(rrcd.haveDrbToReleaseList) {
		fsm_printf("[rrc] [conn_reconfig] ApplyDrbToRelease!\n");	//test
		ApplyDrbToRelease(rrcd.drbToReleaseList);
		//fsm_printf("		ApplySrbToRelease!\n");//test
		//ApplySrbToRelease();//test
	}
	if(rrcd.haveMAC_MainConfig) {
		ApplyMacMainConfig(rrcd.mac_MainConfig);
	}
	if(rrcd.haveSPS_Config) {
		ApplySpsConfig(rrcd.sps_Config);
	}
	if(rrcd.havePhysicalConfigDedicated) {
		ApplyPhysicalConfigDedicated(rrcd.physicalConfigDedicated);
	}

	fsm_printf("[rrc] [conn_reconfig] UE is doing RadioResourceConfigDedicated Reconfig !\n");
	FOUT;
}


//send RRCConnectionReconfiguration message
void DoSendRRCConnectionReconfigurationMsg(struct DL_DCCH_Message *dl_dcch_msg)
{
	FIN(DoSendRRCConnectionReconfigurationMsg());

	fsm_printf("[rrc] haveRadioResourceConfigDedicated = %d\n", dl_dcch_msg->msg.rrcConnectionReconfiguration.haveRadioResourceConfigDedicated);
	SV_PTR_GET(rrc_sv_enb_ue);
	struct RRCConnectionReconfiguration reconfig_msg;
	reconfig_msg = dl_dcch_msg->msg.rrcConnectionReconfiguration;
	if (reconfig_msg.haveRadioResourceConfigDedicated){
		ApplyRadioResourceConfigDedicated (reconfig_msg.radioResourceConfigDedicated);
	}

	//	fsm_printf("		haveSrbToAddModList=%d.\n", reconfmsg->radioResourceConfigDedicated.haveSrbToAddModList);
	//	fsm_printf("		srbid=%d.\n", reconfmsg->radioResourceConfigDedicated.srbToAddModList.srbList[0].srbIdentity);
	fsm_printf("		DoSendRRCConnectionReconfigurationMsg() ---- uefsmid=%d.\n", SV(uefsmid));
	packet_send_to_rlc((char *)dl_dcch_msg, sizeof(struct DL_DCCH_Message), 4, SV(crnti));
	FOUT;
}

//send the SPS_Config to mac
static void ApplySpsConfig(struct SPS_Config sps)
{
	FIN(ApplySpsConfig());
	//fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_SPS,(void*)&sps,sizeof(sps));
	FOUT;
}

static void ApplyPhysicalConfigDedicated(struct PhysicalConfigDedicated phy)
{

}
static void ApplyMacMainConfig(struct MAC_MainConfig mac)
{
	FIN(ApplyMacMainConfig());
	//fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_MAC_MAIN,(void*)&mac,sizeof(mac));
	FOUT;
}

//static void addDrb(struct EpsBearer eps);
void addDrb()
{
	FIN(addDrb());
	fsm_printf("[rrc] build default drbs for eNB\n");
	SV_PTR_GET(rrc_sv_enb_ue);
	struct LteDataRadioBearerInfo *drb0 = fsm_mem_alloc(sizeof(struct LteDataRadioBearerInfo));
	struct LteDataRadioBearerInfo *drb1 = fsm_mem_alloc(sizeof(struct LteDataRadioBearerInfo));
	struct LteDataRadioBearerInfo *drb2 = fsm_mem_alloc(sizeof(struct LteDataRadioBearerInfo));

	struct RlcConfig rlcConfig={
		.type=2,
		.rlcConfigType.um_Bi_Directional=value_struct_umBiDiretion(),
	};

	drb0->drbIdentity = 3;
	drb0->eps_BearerIdentity = 1;
	drb0->rlcConfig = rlcConfig;
	drb0->lcid = drb0->drbIdentity;
	drb0->logicalChannelConfig = value_struct_lcc();

	drb1->drbIdentity = 4;
	drb1->eps_BearerIdentity = 1;
	drb1->rlcConfig = rlcConfig;
	drb1->lcid = drb1->drbIdentity;
	drb1->logicalChannelConfig = value_struct_lcc();


	drb2->drbIdentity = 5;
	drb2->eps_BearerIdentity = 1;
	drb2->rlcConfig = rlcConfig;
	drb2->lcid = drb2->drbIdentity;
	drb2->logicalChannelConfig = value_struct_lcc();

	enbDRBConfig[SV(uefsmid)][drb0->drbIdentity - 3] = drb0;
	enbDRBConfig[SV(uefsmid)][drb1->drbIdentity - 3] = drb1;
	enbDRBConfig[SV(uefsmid)][drb2->drbIdentity - 3] = drb2;

}

