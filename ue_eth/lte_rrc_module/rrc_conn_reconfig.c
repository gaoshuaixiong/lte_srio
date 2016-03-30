#include "../lte_system.h"
#include "rrc_utils.h"
#include "rrc_conn_reconfig.h"

extern void send_to_user(char *info, int size_info);


//ueSrbConfig, ueDrbConfig defined in rrcfsm.c
extern struct LteSignalingRadioBearerInfo* ueSrbConfig[2];
extern struct LteDataRadioBearerInfo* ueDrbConfig[maxDRB];

/******************************************************************************
 ** Function name:SetRrcConnReconfigmessage
 ** Descriptions:  just for test
 ** Input: 
 ** Output:  
 ** Returns: a pointer of RRCConnectionReconfiguration struct, which should be freed
 ** 		 in lower layers
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
//set rrc conn reconfig message for test
struct RRCConnectionReconfiguration *SetRrcConnReconfigmessage(void)
{		
	struct RlcConfig rlcConfig={
		.type=1,
		.rlcConfigType={0},
	};
	struct LogicalChannelConfig lcc={
		.haveUl_SpecificParameters=1,
		.ul_SpecificParameters={0},

	};
	struct SrbToAddMod srb_to_add_mod1={
		.srbIdentity=1,
		.haveRlcConfig=1,
		.rlcConfig=rlcConfig,
		.haveLogicalChannelConfig=1,
		.logicalChannelConfig=lcc,
	};
	struct SrbToAddMod srb_to_add_mod2={
		.srbIdentity=2,
		.haveRlcConfig=1,
		.rlcConfig=rlcConfig,
		.haveLogicalChannelConfig=1,
		.logicalChannelConfig=lcc,
	};
	struct DrbToAddMod drb_to_add_mod={
		.drb_Identity=4,
		.haveRlcConfig=1,
		.rlcConfig=rlcConfig,
		.haveLogicalChannelConfig=1,
		.logicalChannelConfig=lcc,
	};

	struct SrbToAddModList srb_to_addmod_list={
		.num=2,
		.srbList[0]=srb_to_add_mod1,
		.srbList[1]=srb_to_add_mod2,
	};
	struct DrbToAddModList drb_to_addmod_list={
		.num = 1,
		.drbList[0]=drb_to_add_mod,
	};
	struct DrbToReleaseList drb_to_release_list={
		.num=1,
		.drbToRelease[0]=4,
	};
	struct RadioResourceConfigDedicated rrcd={
		.haveSrbToAddModList=1,
		.haveDrbToAddModList=1,
		.haveDrbToReleaseList=1,
		.srbToAddModList=srb_to_addmod_list,
		.drbToAddModList=drb_to_addmod_list,
		.drbToReleaseList=drb_to_release_list,

	};

	struct RRCConnectionReconfiguration *rrcConfig = fsm_mem_alloc(sizeof(struct RRCConnectionReconfiguration));
	rrcConfig->haveRadioResourceConfigDedicated = true;
	rrcConfig->rrcTransactionIdentifier = 1;
	rrcConfig->radioResourceConfigDedicated=rrcd;

	fsm_printf("[rrc] [conn_reconfig] UE is seting RRCConnectionReconfiguration message !\n");
	return rrcConfig;


}



/******************************************************************************
 ** Function name: DoRecvRrcConnectionReconfiguration
 ** Descriptions:  the process of RrcConnectionReconfiguration
 ** Input: 
 ** Output:  this will call function ApplyRadioResourceConfigDedicated()
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void DoRecvRrcConnectionReconfiguration (struct RRCConnectionReconfiguration *msg)
{
	FIN(DoRecvRrcConnectionReconfiguration()); 		
	if (msg->haveRadioResourceConfigDedicated){ 			
		ApplyRadioResourceConfigDedicated (msg->radioResourceConfigDedicated); 		
	} 		 	

	FOUT; 
} 

/******************************************************************************
 ** Function name: ApplyRadioResourceConfigDedicated
 ** Descriptions:  execute the process of dedicated radio resource configure
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void ApplyRadioResourceConfigDedicated (struct RadioResourceConfigDedicated rrcd) 
{ 	
	FIN(ApplyRadioResourceConfigDedicated()); 	
	if(rrcd.haveSrbToAddModList) { 	
		fsm_printf("[rrc] [conn_reconfig] ApplySrbToAdd!\n");//test	 		
		ApplySrbToAdd(rrcd.srbToAddModList); 	
	} 	
	if(rrcd.haveDrbToAddModList) { 		
		fsm_printf("[rrc] [conn_reconfig] ApplyDrbToAdd!\n");//test	
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

/******************************************************************************
 ** Function name: ExtractSrbInfo
 ** Descriptions:  extract srb information for the need of rlc and mac
 ** Input:  SrbToAddMod struct
 ** Output: IOCTL msg for RLC and MAC
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void ExtractSrbInfo(struct SrbToAddMod srb,struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg, struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg)
{
	FIN(ExtractSrbInfo());
	crlc_configreq_ioctlmsg->rbIdentity=srb.srbIdentity;
	crlc_configreq_ioctlmsg->lcIdentity=srb.srbIdentity;
	//	fsm_mem_cpy(&(crlc_configreq_ioctlmsg->Mode),&(srb.rlcConfig.type), sizeof(int));
	crlc_configreq_ioctlmsg->Mode=srb.rlcConfig.type;

	mac_lcc_ioctlmsg->logicalChannelConfig=srb.logicalChannelConfig;
	mac_lcc_ioctlmsg->logicalChannelIdentity=srb.srbIdentity; //INTEGER(1,2)

	fsm_printf("[rrc] [conn_reconfig] RlcConfig Type: %d\n",srb.rlcConfig.type);
	switch(srb.rlcConfig.type)
	{
		case 1:
			//AM down link
			crlc_configreq_ioctlmsg->amDwParas.timerStatusProhibit=(unsigned int)(srb.rlcConfig.rlcConfigType.am.dl_AM_RLC.t_StatusProhibit.t_StatusProhibit);
			crlc_configreq_ioctlmsg->amDwParas.timerReordering=(unsigned int)(srb.rlcConfig.rlcConfigType.am.dl_AM_RLC.t_Reordering.t_Reordering);

			//AM up link
			crlc_configreq_ioctlmsg->amUpParas.timerPollRetransmit=(unsigned int)(srb.rlcConfig.rlcConfigType.am.ul_AM_RLC.t_PollRetransmit.t_PollRetransmittype);
			crlc_configreq_ioctlmsg->amUpParas.PollPDU=(unsigned int)(srb.rlcConfig.rlcConfigType.am.ul_AM_RLC.pollPDU.pollPDUtype);
			crlc_configreq_ioctlmsg->amUpParas.PollBYTE=(unsigned int)(srb.rlcConfig.rlcConfigType.am.ul_AM_RLC.pollByte.pollByte);
			crlc_configreq_ioctlmsg->amUpParas.maxRetxThreshold=(unsigned int)(srb.rlcConfig.rlcConfigType.am.ul_AM_RLC.maxRetxThreshold);

			//		crlc_configreq_ioctlmsg->Mode = 1;
			break;
		case 2:
			//um down link
			crlc_configreq_ioctlmsg->umDwParas.snFiledLength=(unsigned int)srb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.sn_FieldLength.sn_FieldLength;
			crlc_configreq_ioctlmsg->umDwParas.timerReordering=(unsigned int)srb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.t_Reordering.t_Reordering;

			//um up link
			crlc_configreq_ioctlmsg->umUpParas.snFiledLength=(unsigned int)srb.rlcConfig.rlcConfigType.um_Bi_Directional.ul_UM_RLC.sn_FieldLength.sn_FieldLength;
			//		crlc_configreq_ioctlmsg->Mode = 2;
			break;
		case 3:
			//um up link
			crlc_configreq_ioctlmsg->umUpParas.snFiledLength=(unsigned int)srb.rlcConfig.rlcConfigType.um_Bi_Directional.ul_UM_RLC.sn_FieldLength.sn_FieldLength;
			//		crlc_configreq_ioctlmsg->Mode = 3;
			break;
		case 4:
			//um down link
			crlc_configreq_ioctlmsg->umDwParas.snFiledLength=(unsigned int)srb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.sn_FieldLength.sn_FieldLength;
			crlc_configreq_ioctlmsg->umDwParas.timerReordering=(unsigned int)srb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.t_Reordering.t_Reordering;
			//		crlc_configreq_ioctlmsg->Mode = 4;
			break;

	}
	FOUT;
}


/******************************************************************************
 ** Function name: ExtractDrbMacInfo
 ** Descriptions:  extract drb information for the need of mac
 ** Input:  DrbToAddMod struct
 ** Output: IOCTL msg for  MAC
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void ExtractDrbMacInfo(struct DrbToAddMod drb,struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg)
{
	FIN(ExtractDrbMacInfo());
	mac_lcc_ioctlmsg->logicalChannelConfig=drb.logicalChannelConfig;
	mac_lcc_ioctlmsg->logicalChannelIdentity=drb.drb_Identity; //INTEGER(1,2)
	FOUT;
}

/******************************************************************************
 ** Function name: ExtractDrbRlcInfo
 ** Descriptions:  extract drb information for the need of rlc 
 ** Input:  DrbToAddMod struct
 ** Output: IOCTL msg for RLC
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void ExtractDrbRlcInfo(struct DrbToAddMod drb,struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg)
{
	FIN(ExtractDrbInfo());
	crlc_configreq_ioctlmsg->rbIdentity=drb.drb_Identity;
	crlc_configreq_ioctlmsg->lcIdentity=drb.logicalChannelIdentity;
	crlc_configreq_ioctlmsg->Mode=drb.rlcConfig.type;

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
			crlc_configreq_ioctlmsg->Mode = 1;
			break;
		case 2:
			//um down link
			crlc_configreq_ioctlmsg->umDwParas.snFiledLength=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.sn_FieldLength;
			crlc_configreq_ioctlmsg->umDwParas.timerReordering=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.t_Reordering;

			//um up link
			crlc_configreq_ioctlmsg->umUpParas.snFiledLength=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.ul_UM_RLC.sn_FieldLength;
			crlc_configreq_ioctlmsg->Mode = 2;
			break;
		case 3:
			//um up link
			crlc_configreq_ioctlmsg->umUpParas.snFiledLength=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.ul_UM_RLC.sn_FieldLength;
			crlc_configreq_ioctlmsg->Mode = 3;
			break;
		case 4:
			//um down link
			crlc_configreq_ioctlmsg->umDwParas.snFiledLength=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.sn_FieldLength;
			crlc_configreq_ioctlmsg->umDwParas.timerReordering=*(unsigned int*)&drb.rlcConfig.rlcConfigType.um_Bi_Directional.dl_UM_RLC.t_Reordering;
			crlc_configreq_ioctlmsg->Mode = 4;
			break;

	}
	FOUT;	
}

/******************************************************************************
 ** Function name: AddModSrb
 ** Descriptions:  add or mod srb according to srb
 ** Input:  SrbToAddMod struct
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void AddModSrb(struct SrbToAddMod srb,int add_or_mod)
{
	FIN(AddModSrb());
	int ioccmdToRLC;
	struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg;
	struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg;
	crlc_configreq_ioctlmsg = (struct CRLC_ConfigReq_IoctrlMsg *)fsm_mem_alloc(sizeof(struct CRLC_ConfigReq_IoctrlMsg));
	mac_lcc_ioctlmsg = (struct MAC_LogicalChannelConfig_IoctrlMsg *) fsm_mem_alloc(sizeof(struct MAC_LogicalChannelConfig_IoctrlMsg));

	ExtractSrbInfo(srb,crlc_configreq_ioctlmsg,mac_lcc_ioctlmsg);
	fsm_printf("[rrc] [conn_reconfig] IN AddModSrb: crlc_configreq_ioctlmsg->Mode: %d\n",crlc_configreq_ioctlmsg->Mode);

	if (crlc_configreq_ioctlmsg != NULL)
	{
		fsm_printf("[rrc] [conn_reconfig] crlc_configreq_ioctlmsg->lcIdentity= %d\n",crlc_configreq_ioctlmsg->lcIdentity);
		fsm_printf("[rrc] [conn_reconfig] sizeof(struct CRLC_ConfigReq_IoctrlMsg= %d\n)",sizeof(struct CRLC_ConfigReq_IoctrlMsg));
	}

	ioccmdToRLC=(add_or_mod==0? IOCCMD_RRCTORLC_BUILD:IOCCMD_RRCTORLC_RECONFIG);
	//fix me:establish PDCP entity

	//send ioctl msg for rlc to establish/reconfigure srb
	//send ioctl msg for mac to establish/reconfigure DCCH

	fsm_do_ioctrl(STRM_TO_RLC,ioccmdToRLC,(void*)crlc_configreq_ioctlmsg,sizeof(struct CRLC_ConfigReq_IoctrlMsg));
	fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_LCC,(void*)mac_lcc_ioctlmsg,sizeof(struct MAC_LogicalChannelConfig_IoctrlMsg));

	fsm_mem_free(crlc_configreq_ioctlmsg);
	fsm_mem_free(mac_lcc_ioctlmsg);

	FOUT;
}

/******************************************************************************
 ** Function name: AddModDrb
 ** Descriptions:  add or mod drb according to drb
 ** Input:  DrbToAddMod struct
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void AddModDrb(struct DrbToAddMod drb,int add_or_mod)
{
	//FIN(AddModSrb());		//modify 2015091
	FIN(AddModDrb());
	struct CRLC_ConfigReq_IoctrlMsg crlc_configreq_ioctlmsg;
	struct MAC_LogicalChannelConfig_IoctrlMsg mac_lcc_ioctlmsg;
	int ioccmdToRLC;

	ioccmdToRLC=(add_or_mod==0? IOCCMD_RRCTORLC_BUILD:IOCCMD_RRCTORLC_RECONFIG);
	if(ioccmdToRLC==IOCCMD_RRCTORLC_BUILD){
		//fix me:establish PDCP entity

		//send ioctl msg for rlc to establish srb
		ExtractDrbRlcInfo(drb,&crlc_configreq_ioctlmsg);
		fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_RRCTORLC_BUILD,(void*)&crlc_configreq_ioctlmsg,sizeof(crlc_configreq_ioctlmsg));
		//send ioctl msg for mac to establish DCCH	
		ExtractDrbMacInfo(drb,&mac_lcc_ioctlmsg);
		fsm_printf("[rrc] [AddModDrb] add a drb, drbid = %d\n", drb.drb_Identity);
		fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_LCC,(void*)&mac_lcc_ioctlmsg,sizeof(mac_lcc_ioctlmsg));
	}
	else{
		//fix me:if have pdcpconfig,reconfig PDCP entity

		//send ioctl msg for rlc to reconfigure srb
		if(drb.haveRlcConfig){
			ExtractDrbRlcInfo(drb,&crlc_configreq_ioctlmsg);
			fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_RRCTORLC_RECONFIG,(void*)&crlc_configreq_ioctlmsg,sizeof(crlc_configreq_ioctlmsg));
		}
		//send ioctl msg for mac to reconfigure DCCH
		if(drb.haveLogicalChannelConfig){
			ExtractDrbMacInfo(drb,&mac_lcc_ioctlmsg);
			fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_LCC,(void*)&mac_lcc_ioctlmsg,sizeof(mac_lcc_ioctlmsg));
		}
	}
	FOUT;
}

/******************************************************************************
 ** Function name: ApplySrbToAdd
 ** Descriptions:  add srb1/sbb2
 ** Input:  SrbToAddModList struct
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void ApplySrbToAdd(struct SrbToAddModList  srb)
{
	FIN(ApplySrbToAdd());
	int srbIndex;
	struct SrbToAddMod eachSrb;
	struct LteSignalingRadioBearerInfo *localSrbConfig=NULL;
	for(srbIndex = 0; srbIndex < srb.num; srbIndex ++)		
	{
		fsm_printf("[rrc] srbIndex = %d, srbId = %d.\n", srbIndex, srb.srbList[srbIndex].srbIdentity);
		fsm_printf("[rrc] srbIdentity @ %p.\n", &(srb.srbList[srbIndex].srbIdentity));
		eachSrb = srb.srbList[srbIndex];
		localSrbConfig=ueSrbConfig[eachSrb.srbIdentity-1];
		if(localSrbConfig == NULL)
		{
			localSrbConfig=fsm_mem_alloc(sizeof(struct LteSignalingRadioBearerInfo));
			//build the srb
			AddModSrb(eachSrb,0);

			//fix me:srb status managment
			localSrbConfig->srbStatus=1;//0,hang up
			localSrbConfig->srbIdentity=eachSrb.srbIdentity;
			localSrbConfig->lcid=(u8)eachSrb.srbIdentity;

			//back up older config for recover the SRB hanged up
			localSrbConfig->rlcConfig=eachSrb.rlcConfig;
			localSrbConfig->logicalChannelConfig=eachSrb.logicalChannelConfig;
			ueSrbConfig[eachSrb.srbIdentity-1]=localSrbConfig;
			fsm_printf("[rrc] [conn_reconfig] add a srb ,srbid=%d\n",ueSrbConfig[eachSrb.srbIdentity-1]->srbIdentity);//test
		}
		else
		{
			//reconfig the srb
			AddModSrb(eachSrb,1);

			//back up older config for recover the SRB hanged up
			localSrbConfig->rlcConfig=eachSrb.rlcConfig;
			localSrbConfig->logicalChannelConfig=eachSrb.logicalChannelConfig;
			fsm_printf("[rrc] [conn_reconfig] reconfig a srb ,srbid=%d\n",ueSrbConfig[eachSrb.srbIdentity-1]->srbIdentity);//test
		}
	}
	fsm_printf("[rrc] [conn_reconfig] ApplySrbToAdd() is done!.\n");//test
	FOUT;
}

/******************************************************************************
 ** Function name: ApplySrbToRelease
 ** Descriptions:  release srb1 and sbb2
 ** Input:  
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void ApplySrbToRelease(void)
{
	//FIN(ApplySrbToAdd()); modify 20150914
	FIN(ApplySrbToRelease());
	int srbIndex;
	int srbid;
	int logicalChannelIdentity;
	for(srbIndex=0;srbIndex<2;srbIndex++)
	{
		srbid = srbIndex+1;
		logicalChannelIdentity= srbid;
		if(ueSrbConfig[srbIndex]!=NULL){
			//fix me:release pdcp

			//release rlc entity
			fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_RRCTORLC_RELEASE,(void*)&srbid,sizeof(int));

			//fix me:release DTCH
			fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_RELEASE_LCC,(void*)&logicalChannelIdentity,sizeof(logicalChannelIdentity));

			//release memory and pointer
			fsm_mem_free(ueSrbConfig[srbIndex]);
			ueSrbConfig[srbIndex]=NULL;
			fsm_printf("[rrc] [conn_reconfig] srb.srbToRelease=SRB%d\n",srbid);//test
		}
	}
	fsm_printf("[rrc] [conn_reconfig] ApplySrbToRelease() is done!.\n");//test
	FOUT;			
}

/******************************************************************************
 ** Function name: ApplyDrbToAdd
 ** Descriptions:  add drbs according DrbToAddModList drb
 ** Input: DrbToAddModList struct 
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
extern void ApplyDrbToAdd(struct DrbToAddModList drb)
{
	FIN(ApplyDrbToAdd());
	struct LteDataRadioBearerInfo *localDrbConfig;
	struct DrbToAddMod eachDrb;
	int drbIndex;

	fsm_printf("[rrc] [ApplyDrbToAdd] DrbToAddModList.num = %d\n", drb.num);
	for(drbIndex=0; drbIndex<drb.num; drbIndex++)
	{
		eachDrb=drb.drbList[drbIndex];	
		localDrbConfig=ueDrbConfig[eachDrb.drb_Identity];
		if(localDrbConfig==NULL){
			localDrbConfig=fsm_mem_alloc(sizeof(struct LteDataRadioBearerInfo));
			//build the drb
			AddModDrb(eachDrb,0);

			//drb status managment
			localDrbConfig->drbStatus=1;
			localDrbConfig->drbIdentity=eachDrb.drb_Identity;
			localDrbConfig->lcid=eachDrb.logicalChannelIdentity;

			//back up older config for recover the DRB hanged up
			/*localDrbConfig->rlcConfig=eachDrb.rlcConfig;*/

			localDrbConfig->logicalChannelConfig=eachDrb.logicalChannelConfig;
			ueDrbConfig[eachDrb.drb_Identity]=localDrbConfig;
			fsm_printf("[rrc] [conn_reconfig] add a drb ,drbid=%d\n",ueDrbConfig[eachDrb.drb_Identity]->drbIdentity);//test
		}
		else
		{
			//reconfig the drb
			AddModDrb(eachDrb,1);

			//back up older config for recover the DRB hanged up
			localDrbConfig->rlcConfig=eachDrb.rlcConfig;
			localDrbConfig->logicalChannelConfig=eachDrb.logicalChannelConfig;
			fsm_printf("[rrc] [conn_reconfig] reconfig a drb ,drbid=%d\n",ueDrbConfig[eachDrb.drb_Identity]->drbIdentity);//test
		}
	}
	FOUT;	
}

/******************************************************************************
 ** Function name: ApplyDrbToRelease
 ** Descriptions:  release drbs according DrbToReleaseList drb
 ** Input: DrbToReleaseList struct 
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
//release the drb
void ApplyDrbToRelease(struct DrbToReleaseList drb)
{
	FIN(ApplyDrbToRelease());
	int drbIndex;
	for(drbIndex=0; drbIndex<drb.num;drbIndex++){
		DRB_Identity drbid=drb.drbToRelease[drbIndex];
		int logicalChannelIdentity= drbid;//DRB0-->lcid=3,DRB1-->lcid=4,.....
		if(ueDrbConfig[drbid]!=NULL){
			//fix me:release pdcp

			//release rlc entity
			fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_RRCTORLC_RELEASE,(void*)&drbid,sizeof(DRB_Identity));

			//fix me:release DTCH
			fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_RELEASE_LCC,(void*)&logicalChannelIdentity,sizeof(logicalChannelIdentity));

			//release memory and pointer
			fsm_mem_free(ueDrbConfig[drbid]);
			ueDrbConfig[drbid]=NULL;
			fsm_printf("[rrc] [conn_reconfig] drb.drbToRelease=DRB%d\n",drbid);//test
		}
	}
	FOUT;
}

/******************************************************************************
 ** Function name: ApplyMacMainConfig
 ** Descriptions: send the MAC_MainConfig to mac
 ** Input: MAC_MainConfig struct
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void ApplyMacMainConfig(struct MAC_MainConfig mac)
{
	FIN(ApplyMacMainConfig());
	fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_MAC_MAIN,(void*)&mac,sizeof(mac));
	FOUT;
}

/******************************************************************************
 ** Function name: ApplySpsConfig
 ** Descriptions: send the SPS_Config to mac
 ** Input: MAC_MainConfig struct
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void ApplySpsConfig(struct SPS_Config sps)
{
	FIN(ApplySpsConfig());
	fsm_do_ioctrl(STRM_TO_MAC,IOCCMD_RRCTOMAC_CONFIG_SPS,(void*)&sps,sizeof(sps));
	FOUT;
}

/******************************************************************************
 ** Function name: ApplyPhysicalConfigDedicated
 ** Descriptions: send the PhysicalConfigDedicated to phy
 ** Input: PhysicalConfigDedicated struct
 ** Output: 
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-----------------------------------------------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
static void ApplyPhysicalConfigDedicated(struct PhysicalConfigDedicated phy)
{

}

/******************************************************************************
 ** Function name: DoSendRrcConnectionReconfigurationCompleted
 ** Descriptions: Send rrc connection reconfig complete msg
 ** Input:   
 ** Output:  
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void DoSendRrcConnectionReconfigurationCompleted(struct RRCConnectionReconfigurationComplete reconfigcompletemsg)
{
	FIN(DoSendRrcConnectionReconfigurationCompleted());
	struct UL_DCCH_Message *ul_DCCH_Message;
	int message_type;

	ul_DCCH_Message = (struct UL_DCCH_Message *) fsm_mem_alloc(sizeof(struct UL_DCCH_Message));
	ul_DCCH_Message->type = 2;
	ul_DCCH_Message->msg.rrcConnectionReconfigurationComplete = reconfigcompletemsg;

	message_type = 6;
	packet_send_to_pdcp(ul_DCCH_Message, sizeof(struct UL_DCCH_Message), message_type);
//	fsm_mem_free(ul_DCCH_Message);
	fsm_printf("[rrc] [fsm] UE Reconfig finished!\n");
	FOUT;
}

/*****
//execute the process of radio resource full configure
void ApplyRadioResourceConfigFull (struct RRCConnectionReconfiguration *msg)
{ 
	FIN(ApplyRadioResourceConfigFull());
	//最简系统暂不考虑
	FOUT;
} 
//excute the process of Measure configure
void ApplyMeasConfig (struct MeasConfig measConfig)
{
	FIN(ApplyMeasConfig());
	FOUT;
}
 ****/
