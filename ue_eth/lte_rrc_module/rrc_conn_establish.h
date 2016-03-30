/******************************************************************
**Filename:rc_conn_establish.h
**Copyright:uestc
**Description:RRC连接建立过程的头文件
**V1.0,2014/09/06,Xu Shangfei written
**-----------------------------------
**modification history:
**Modified by: Xu Shangfei
**Modified date:
**Description:增加注释
****************************************************************/
#ifndef RRC_CONN_ESTABLISH
#define RRC_CONN_ESTABLISH

/**** functions used by rrcfsm.c ****/
//ue start rrc conn
extern void set_default_physical_channel(struct PhysicalConfigDedicated phydefaultmsg);
extern void set_default_semi_persistent (struct SPS_Config spsdefaultmsg);
extern void set_default_MAC(struct MAC_MainConfig macdefaultmsg);
extern void set_timeAlignmentTimerCommon (struct SystemInformationBlockType2 *sys2infoAlignmenttimer);
extern void send_RRCConnectionRequest(struct UL_CCCH_Message *ulccchRRCremsg);


//ue receive rrc conn  setup
extern void set_MAC_mainconfig(struct RRCConnectionSetup *rrcConnectionSetupmMACmsg);
extern void set_semi_persistent(struct RRCConnectionSetup *rrcConnectionSetupmSPSmsg);
extern void set_physicalConfigDedicated(struct RRCConnectionSetup *rrcConnectionSetupmPHYmsg);
extern void send_RRCConnectionSetupComplete(struct UL_DCCH_Message *ul_dcchmsg);


//extern message
//extern struct Paging *gen_paging(void);
extern struct RadioResourceConfigDedicated *gen_Defaultradioresource (void);          //inital mac，sps，phy default message
extern struct SystemInformationBlockType2 *gen_sysInfoTimer(void);                   //config timeAlignmentTimerCommon message
extern struct UL_CCCH_Message *gen_ul_ccchmsg_send_rrcquest(void);                   //config RRCrequest message
extern struct DL_CCCH_Message *gen_dl_ccch_send_rrcsetup(void);                      //generate rrcsetup message
extern struct UL_DCCH_Message *gen_ul_dcch_rrccomplete(void);                        //config rrccomplete message




#endif

