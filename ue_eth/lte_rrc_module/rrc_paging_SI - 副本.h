
#ifndef RRC_CONN_ESTABLISH
#define RRC_CONN_ESTABLISH

/**** functions used by rrcfsm.c ****/
//ue received paging 
extern void CheckPagingRecord(struct PagingRecordList prl,struct UE_Identity ueIdentity);
extern void Begin_recv_sysinfo(void);

//ue received mib
extern void DoReceiveMIB(struct MasterInformationBlock *mib);
extern void ApplyPhich_Config(struct PHICH_Config phich_c);

//ue received sib1 
extern void SendCellAccessRelatedInfotoUS(struct CellAccessRelatedInfo cari);

//ue received si
extern void ApplyRadioResourceConfigCommon(struct RadioResourceConfigCommonSib rrcc);
extern void ApplyRachConfigCommon(struct RachConfigCommon rcc);
extern void ApplyBCCH_Config(struct BCCH_Config bc);
extern void ApplyPCCH_Config(struct PCCH_Config pc);
extern void ApplyPRACH_ConfigSIB(struct PRACH_ConfigSIB prach_c);
extern void ApplyPDSCH_ConfigCommon(struct PDSCH_ConfigCommon pdsch_c);
extern void ApplyPUSCH_ConfigCommon(struct PUSCH_ConfigCommon pusch_c);
extern void ApplySoundingRS_UL_ConfigCommon(struct SoundingRS_UL_ConfigCommon sc);
extern void ApplyUplinkPowerControlCommon(struct UplinkPowerControlCommon uc);
extern void ApplyUL_CyclicPrefixLength(struct UL_CyclicPrefixLength ul_c);

/****functions used for test****/
extern FSM_PKT* gen_paging();
extern FSM_PKT* gen_mib();
extern FSM_PKT* gen_sib1();
extern FSM_PKT* gen_si();



#endif

