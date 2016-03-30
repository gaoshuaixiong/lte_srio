#include "../fsm/fsmdec.h"

#ifndef _PDCPFSM_H
#define _PDCPFSM_H

#define MAX_PSN 1024
typedef struct pdcp_sv{
unsigned int psn;
} pdcp_sv;
typedef struct config
{
	unsigned int mode;
	unsigned char rbid;
	//unsigned char*cmd;
}config;

/* the ICI information in the transimition between PDCP and RRC, IPADP*/
typedef struct UPDCP_IciMsg{
	//u16 rnti; 
	u8 pbCh;     //PBCh = 1:PCCH, PBCh = 2: BCCH, 0:CCCH
    u8 rbId;
}__attribute__((packed)) UPDCP_IciMsg;


//#define _DO_IOCTL_PERIOD 	1
#define _START_WORK 		0

#define 		CONFIG							0x14
#define 		DEACT							0x16
#define 		SUSPEND							0x17
#define 		RESUME							0x18
#define			BULID							0x19	

/*#define 		CRLC_CONFIG_REQ				0x24
#define 		CRLC_STATUS_IND				0x25
#define 		CRLC_DEACT_REQ				0x26
#define 		CRLC_SUSPEND_REQ			0x27
#define 		CRLC_RESUME_REQ				0x28
#define			CRLC_BULID_REQ				0x29*/

//new definition 20150907
#define		CRLC_CONFIG_REQ				0x50
#define		CRLC_STATUS_IND				0x51
#define		CRLC_DEACT_REQ				0x52
#define		CRLC_SUSPEND_REQ			0x53
#define		CRLC_RESUME_REQ				0x54
#define		CRLC_BULID_REQ				0x55

#define	SENDPDCP	0x09


#define PDCP_PKT_FROM_LOWER (fsm_ev_type()==FSM_EV_TYPE_PKT_IND)
#define PDCP_PKT_FROM_UPPER (fsm_ev_type()==FSM_EV_TYPE_PKT_REQ)
#define PDCP_OPEN (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code()== FSM_EV_CORE_OPEN)
#define PDCP_CLOSE (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_CORE_CLOSE)
#define IOCTL_ARRIVAL (fsm_ev_type() == FSM_EV_TYPE_CORE && fsm_ev_code() == FSM_EV_IOCTRL)
#define START_WORK (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == _START_WORK)

#define CONFIG_PERIOD (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == CONFIG)
#define DEACT_PERIOD (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == DEACT)
#define SUSPEND_PERIOD (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == SUSPEND)
#define RESUME_PERIOD (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == RESUME)
#define BULID_PERIOD (fsm_ev_type() == FSM_EV_TYPE_SELF && fsm_ev_code() == BULID)

	
	

void pdcp_main(void);

#endif



