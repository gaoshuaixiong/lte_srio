
#ifndef RRC_CONN_RECONFIG
#define RRC_CONN_RECONFIG

#define ADD 	0
#define MOD 	1

/**** functions used by rrcfsm.c ****/
//ue start rrc conn reconfig 
void DoSendRrcConnectionReconfigurationCompleted(struct RRCConnectionReconfigurationComplete reconfigcompletemsg);
void DoRecvRrcConnectionReconfiguration (struct RRCConnectionReconfiguration *msg);
struct RRCConnectionReconfiguration *SetRrcConnReconfigmessage(void);
//used by rrc conn establish srb1
extern void ApplySrbToAdd(struct SrbToAddModList  srb);
//used by rrc conn release 
extern void ApplyDrbToRelease(struct DrbToReleaseList drb);
extern void ApplySrbToRelease(void);
extern void ApplyDrbToAdd(struct DrbToAddModList drb);


/**** functions used by reconfig.c ****/
static void ApplyRadioResourceConfigDedicated (struct RadioResourceConfigDedicated rrcd);
static void ApplyMacMainConfig(struct MAC_MainConfig mac);
static void ApplySpsConfig(struct SPS_Config sps);
static void ApplyPhysicalConfigDedicated(struct PhysicalConfigDedicated phy);
//void ApplyRadioResourceConfigFull (struct RRCConnectionReconfiguration *msg);
//void ApplyMeasConfig (struct MeasConfig measConfig);
static void ExtractSrbInfo(struct SrbToAddMod srb,struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg, struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg);
static void ExtractDrbRlcInfo(struct DrbToAddMod drb,struct CRLC_ConfigReq_IoctrlMsg *crlc_configreq_ioctlmsg);
static void ExtractDrbMacInfo(struct DrbToAddMod drb,struct MAC_LogicalChannelConfig_IoctrlMsg *mac_lcc_ioctlmsg);
static void AddModSrb(struct SrbToAddMod srb,int add_or_mod);
static void AddModDrb(struct DrbToAddMod drb,int add_or_mod);

#endif




