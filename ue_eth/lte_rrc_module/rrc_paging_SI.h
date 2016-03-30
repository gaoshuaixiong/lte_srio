/******************************************************************
** Filename: rrc_paging_SI.h
** Copyright: uestc.
** Descriptions:
** v8.1.2,2014/10/07,wangjinchuan written
** -----------------------------------------------
** modificationhistory:
** Modified by:
** Modified date:
** Descriptions:
**
******************************************************************/

#ifndef RRC_PAING_SI
#define RRC_PAING_SI
#include "rrc_utils.h"


/**** functions used by rrcfsm.c ****/
//ue received paging 
void DoReceivePaging(struct Paging *paging,struct UE_Identity ueIdentity);
void CheckPagingRecord(struct PagingRecordList prl,struct UE_Identity ueIdentity);
void Begin_recv_sysinfo(void);

//ue received mib
void DoReceiveMIB(struct MasterInformationBlock *mib);
void ApplyPhich_Config(struct PHICH_Config phich_c);

//ue received sib1 
void DoReceiveSIB1(struct SystemInformationBlockType1 *sib1);
void SendCellAccessRelatedInfotoUS(struct CellAccessRelatedInfo cari);

//ue received si
void DoReceiveSIB2(struct SystemInformationBlockType2 sib2);
void ApplyRadioResourceConfigCommon(struct RadioResourceConfigCommonSib rrcc);
void ApplyRachConfigCommon(struct RachConfigCommon rcc);
void ApplyBCCH_Config(struct BCCH_Config bc);
void ApplyPCCH_Config(struct PCCH_Config pc);
void ApplyPRACH_ConfigSIB(struct PRACH_ConfigSIB prach_c);
void ApplyPDSCH_ConfigCommon(struct PDSCH_ConfigCommon pdsch_c);
void ApplyPUSCH_ConfigCommon(struct PUSCH_ConfigCommon pusch_c);
void ApplySoundingRS_UL_ConfigCommon(struct SoundingRS_UL_ConfigCommon sc);
void ApplyUplinkPowerControlCommon(struct UplinkPowerControlCommon uc);
void ApplyUL_CyclicPrefixLength(struct UL_CyclicPrefixLength ul_c);

/****functions used for test****/
FSM_PKT* gen_paging();
FSM_PKT* gen_mib();
FSM_PKT* gen_sib1();
FSM_PKT* gen_si();



#endif

