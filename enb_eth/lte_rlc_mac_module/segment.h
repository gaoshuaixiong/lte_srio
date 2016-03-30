#ifndef _SEGMENT_H
#define _SEGMENT_H
#include "rlc_macfsm.h"
FSM_PKT * rlc_UM_segment(struct UM_TX_Instance *um_tx_ins,u32 size);
FSM_PKT * rlc_AM_segment(struct AM_Instance *am_ins,u32 size);
FSM_PKT* rlc_resegment(struct AmBuffer *tbuff, u32 sizeFromMac);
void rlc_AM_reassemble_deliver(struct AM_Instance *amIns, struct pduLeft *pdu_left);
void rlc_UM_reassemble_deliver(struct UM_RX_Instance *umIns, struct pduLeft *pdu_left);
void rlc_UM5_reassemble_deliver(struct UM_RX_Instance *umIns, struct pduLeft *pdu_left);

#endif
