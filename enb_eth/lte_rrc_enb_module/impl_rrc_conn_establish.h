
#ifndef RRC_CONN_ESTABLISH
#define RRC_CONN_ESTABLISH

/**** functions used by rrcfsm.c ****/

extern struct DL_CCCH_Message *gen_dl_ccch_send_rrcsetup(void);                         //generate rrcsetup message
extern struct DL_CCCH_Message *gen_dl_ccch_send_rrcreject(void);			//generate rrcrejct message
	
//use for test
struct UL_DCCH_Message *gen_ul_dcch_rrccomplete(void);


#endif

