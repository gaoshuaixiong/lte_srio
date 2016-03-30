/***********************************************************************
**Filename:impl_rrc_conn_release.h
**Copyright:uestc.
**Descriptions:
**v1.0,2014/09/08,zhou yang
**------------------------------------------------
**modification history:
**Modified by:zhou yang
**Modified date:2014/10/07
**Descriptions:modified format
************************************************************************/

#ifndef RRC_CONN_RELEASE
#define RRC_CONN_RELEASE

/**** functions used by rrcfsm.c ****/
//ue start rrc conn release
extern void cell_reselection_priority_info(struct RRCConnectionRelease * rrcConnectionRelease) ;
extern void set_t320(struct RRCConnectionRelease * rrcConnectionRelease) ;
extern void stop_timer(void);
extern void release_radio_resource();
extern void report_to_userspace(void);
extern void t320_timeout(struct CellReselectionPriority *ueCellReselectionPriorityList);
extern struct RRCConnectionRelease *gen_release();
#endif

