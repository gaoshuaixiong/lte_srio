#include <linux/if_ether.h>
#include "pdcpfsm.h"
#include "../lte_system.h"
#include "../pkfmt.h"
#include "Rlctypes.h"

#define ST_INIT 0
#define  ST_IDLE 1
#define ST_SEND	2
#define ST_RECV	3


/* IOCTL command  */
#define  SAY_HELLO 			0x04

static void init_enter(void);
static void pdcp_sv_init(void);
static void pdcp_close(void);
static void do_ioctl_period(void);
//static void pdcp_send_to_rlc(unsigned char rbid);
static void  pdcp_pkt_receive_from_rlc(void);
static void pdcp_pkt_receive_from_upper(void);
static void pdcp_interupt(void);
static void ioctl_handler(void);

void pdcp_main(void)
{
	FSM_ENTER(pdcp_main);
	FSM_BLOCK_SWITCH
	{
		/*FSM_STATE_UNFORCED(ST_INIT, "INIT", init_enter(), )
		{
			FSM_COND_TEST_IN("INIT")
			FSM_TEST_COND(START_WORK)
			FSM_COND_TEST_OUT("INIT")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_IDLE,, "INIT -> IDLE")
				FSM_CASE_DEFAULT(ST_INIT, , "INIT->INIT")
			}
		}*/
		FSM_STATE_FORCED(ST_INIT, "INIT", init_enter(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "INIT -> IDLE"); 
		}
		FSM_STATE_UNFORCED(ST_IDLE, "IDLE",  ,pdcp_interupt())
		{
			FSM_COND_TEST_IN("IDLE")
			FSM_TEST_COND(PDCP_CLOSE)
			FSM_COND_TEST_OUT("IDLE")
			FSM_TRANSIT_SWITCH
			{
				FSM_CASE_TRANSIT(0, ST_INIT, , "IDLE -> INIT")
				FSM_CASE_DEFAULT(ST_IDLE, , "IDLE->IDLE")	//transit to idle state	by default.
			}
		}
	}
	FSM_EXIT(0)
}

static void init_enter(void)
{
	FIN(init_enter());
	SV_PTR_GET(pdcp_sv);
	if(PDCP_OPEN)
	{
		fsm_printf("[PDCP][init_enter] pdcp has opened\n ");
		SV(psn) = 0;
		//fsm_schedule_self(0, _START_WORK);
//	fsm_schedule_self(1000, CONFIG);
	}
	FOUT;
}



static void pdcp_sv_init(void)
{
	FIN(pdcp_sv_init());
	SV_PTR_GET(pdcp_sv);
	FOUT;
}

static void pdcp_close(void)
{
	FIN(pdcp_close());
	SV_PTR_GET(pdcp_sv);
	fsm_printf("[PDCP][pdcp_close] PDCP is realasing.\n");
	FOUT;
}

static void do_ioctl_period()
{
//pdcp_send_to_rlc();
//   suspendAmIns();

//resumeAmIns();
//deactAmIns();
	FOUT;
}

static void pdcp_pkt_receive_from_upper(void)
{
	FSM_PKT* pkptr;
	FIN(packet_recv_from_rlc());
	pkptr = fsm_pkt_get();
	fsm_printf("[PDCP][pdcp_pkt_receive_from_upper] -->receive from upper\n");

	fsm_pkt_send(pkptr,STRM_TO_RLCMAC);
	fsm_printf("[PDCP][pdcp_pkt_receive_from_upper] <--send to RLC\n");
	fsm_printf("[PDCP][pdcp_pkt_receive_from_upper] the data is \n");  //FOR TEST
//	fsm_octets_print(pkptr->data,2);

	FOUT;
}

static void pdcp_pkt_receive_from_rlc(void)
{
	FSM_PKT* pkptr;
	UPDCP_IciMsg *icimsg;
	FIN(packet_recv_from_rlc());
	pkptr = fsm_pkt_get();
	
	icimsg = (UPDCP_IciMsg*)pkptr->head;
	fsm_printf("[PDCP][pdcp_pkt_receive_from_rlc] -->receive from RLC, rbid=%d\n",icimsg->rbId);
	if( icimsg->rbId == 1 || icimsg->rbId == 2 )
	{
		fsm_pkt_send(pkptr,STRM_TO_RRC);
		fsm_printf("[PDCP][pdcp_pkt_receive_from_rlc] <--send to RRC\n");
	}
	else
	{
		fsm_printf("[PDCP][pdcp_pkt_receive_from_rlc] <--before send to IPADP\n");
		fsm_printf("[PDCP]send to IPADP pkptr->data:\n");
//		fsm_octets_print(pkptr->data, 84);
		fsm_pkt_send(pkptr,STRM_TO_IPADP);
		fsm_printf("[PDCP][pdcp_pkt_receive_from_rlc] <--after send to IPADP\n");
	}
	FOUT;
}

static void pdcp_interupt(void)
{
	FIN(pdcp_interupt());
	if(PDCP_CLOSE)
	{
		pdcp_close();
	}
	else if(IOCTL_ARRIVAL)
	{
		ioctl_handler();
	}
	else if(CONFIG_PERIOD)
	{
		//pdcp_send_to_rlc(2);
		//pdcp_send_to_rlc(8);
		//pdcp_send_to_rlc(0);
	}
	else if(PDCP_PKT_FROM_LOWER)       //接收来自下层的包
	{
		pdcp_pkt_receive_from_rlc();
	}
	else if(PDCP_PKT_FROM_UPPER)		//接收来自上层的包
	{
		pdcp_pkt_receive_from_upper();
	}
	FOUT;
}

static void ioctl_handler(void)
{
	config* conf;
	FIN(ioctl_handler());
	SV_PTR_GET(pdcp_sv);
	if(fsm_ev_ioctrl_cmd() == SENDPDCP)
	{
		fsm_printf("[PDCP][ioctl_handler] pdcp has receive SENDPDCP\n");
		conf = (config*)fsm_data_get();
		//pdcp_send_to_rlc(conf->rbid);
		fsm_data_destroy(conf);
	}
	FOUT;
}





