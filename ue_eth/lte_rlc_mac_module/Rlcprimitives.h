
#ifndef _RLCPRIMITIVES_H
#define _RLCPRIMITIVES_H
typedef enum {AM_UPDOWN=1,UM_UPDOWN=2, UM_UP=3, UM_DOWN=4} RlcConfig;
extern void PCRLC_CONFIG_REQ(void);
extern void PCRLC_BULID_REQ(void);
extern void PCRLC_DEACT_REQ(void);
extern void PCRLC_SUSPEND_REQ(void);
extern void PCRLC_RESUME_REQ(void);
extern void PPRLC_DISC_REQ(void);
//extern void PPRLC_AM_DATA_CNF(unsigned short rbid, u16 firstCnfSN, u16 cnfPduNum);
extern void PMRLC_TRANSOP_IND (void *hptr);
extern void* PMRLC_BUFFERREP_REQ(void);
//extern void PCRLC_STATUS_IND(u8 rbid, u16 evCode);

extern void findInsBylcid(u8 lcid, void **up, void**down, RlcMode *mode);
extern FSM_PKT* rlc_UM_segment(struct UM_TX_Instance *um_tx_ins,u32 size);
extern FSM_PKT * rlc_AM_segment(struct AM_Instance *am_ins,u32 size);
extern FSM_PKT* rlc_resegment(struct AmBuffer*amBuffer, u32 retxSize);
extern void  findInsByRbid(u8 rbid,void **up, void **down , RlcMode *mode) ;  //
extern bool isInsideTranWin(struct AM_Instance *amIns, SequenceNumber SN);
extern void rlc_poll(struct AM_Instance *amIns, FSM_PKT * pkt);
#endif
