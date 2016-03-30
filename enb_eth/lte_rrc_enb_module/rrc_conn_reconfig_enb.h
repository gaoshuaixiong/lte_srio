#ifndef RRC_CONN_RECONFIG_ENB
#define RRC_CONN_RECONFIG_ENB
#include "rrc_type_IEs.h"



/**** functions used by rrcfsm_enb_ue.c****/ 
void DoSendRRCConnectionReconfigurationMsg(struct DL_DCCH_Message *msg);
struct SrbToAddModList value_struct_srblist(int srbtype);
struct DrbToAddModList build_default_drb(void);
void ApplyDrbToAdd(struct DrbToAddModList drb);
void ApplySrbToAdd(struct SrbToAddModList srb);
struct DL_DCCH_Message *gen_RRCConnectionReconfigurationMessage(int srb_or_drb,int srbtype);
/**** functions used by rrc conn release ****/
void ApplyDrbToRelease(struct DrbToReleaseList drb);
void ApplySrbToRelease(void);

/**** functions used by impl_rrc_conn_reconfig_enb.c ****/
static void ExtractSrbInfo(struct SrbToAddMod srb,struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg, struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg);
static void ExtractDrbRlcInfo(struct DrbToAddMod drb,struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg);
static void ExtractDrbMacInfo(struct DrbToAddMod drb,struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg);
static void AddModSrb(struct SrbToAddMod srb,int add_or_mod);
static void AddModDrb(struct DrbToAddMod drb,int add_or_mod);
static struct SrbToAddMod value_struct_srbmod(int rlcmod,int srbid);
static void ApplyRadioResourceConfigDedicated (struct RadioResourceConfigDedicated rrcd);
static void ApplySpsConfig(struct SPS_Config sps);
static void ApplyPhysicalConfigDedicated(struct PhysicalConfigDedicated phy);
static void ApplyMacMainConfig(struct MAC_MainConfig mac);
#endif




