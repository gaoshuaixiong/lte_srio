#ifndef _ARQ_H
#define _ARQ_H
#endif
#include "rlc_macfsm.h"

void Assemble_ctrlPduPart2 (struct Buffer *ctrlBf, SequenceNumber sn,u32 *pduLen,u32 size);
u32 size_of_ctrl_pdu(struct AM_Instance *amIns);
void rlc_ctrl_pkt_trans(struct AM_Instance *amIns);
void ack_handler(struct AM_Instance *amIns,SequenceNumber ackSn);
void no_so_handler(struct AM_Instance *amIns, ctrlPduPart2 *part2, bool *isPduFound);
void so_handler(struct AM_Instance *amIns, ctrlPduPart3 *part3, bool *isPduFound,SequenceNumber nackSn);
void rlc_ctrl_pkt_recv(struct AM_Instance *amIns, FSM_PKT * pkt);
void rlc_poll_retx(struct AM_Instance *amIns);
void rlc_retx(struct AM_Instance *amIns,struct AmBuffer *amTxedBuffer);
void rlc_poll(struct AM_Instance *amIns, FSM_PKT * pkt);











