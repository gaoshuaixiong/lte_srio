/***********************************************************************
**Filename:impl_rrc_conn_release_enb.h
**Copyright:
**Descriptions:
**v1.0,2014/09/08
**------------------------------------------------
**modification history:
**Modified by:
**Modified data:
**Descriptions:
**
************************************************************************/

#ifndef RRC_CONN_RELEASE_ENB
#define RRC_CONN_RELEASE_ENB

extern void stop_timer(void);
extern void release_radio_resource();
void send_rrc_connection_release_msg_to_ue();

#endif

