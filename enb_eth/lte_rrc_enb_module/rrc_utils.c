#include "rrc_utils.h"
#include "../pkfmt.h"
#include "../lte_system.h"


struct LteSignalingRadioBearerInfo* enbSRBConfig[MAX_UEFSM_NUM][MAX_SRB_NUM] = {NULL};
struct LteDataRadioBearerInfo* enbDRBConfig[MAX_UEFSM_NUM][MAX_DRB_NUM] = {NULL};

/* function to set Timer */
void setTimer(int timerNum, int delay)
{
    FIN(setTimer());
    fsm_printf("setTimer(%d,%d).\n",timerNum,delay);
    switch(timerNum)
    {
        case 0:
            Max_Conn_Timeout = fsm_schedule_self(delay, CODE_MAX_CONN_TIMEOUT);
            FOUT;       
		case 1:
			Send_MIB = fsm_schedule_self(delay, CODE_SEND_MIB);
			FOUT;
		case 2:
			Send_SIB1 = fsm_schedule_self(delay, CODE_SEND_SIB1);
			FOUT;
		case 3:
			Send_SI = fsm_schedule_self(delay, CODE_SEND_SI);
			FOUT;
        default:
            FOUT;
    }
}


/* function to cancle Timer */
void cancleTimer(int timerNum)
{
    FIN(cancleTimer());
    fsm_printf("cancleTimer(%d).\n",timerNum);
    switch(timerNum)
    {
        case 0:
            if (Max_Conn_Timeout != 0)
                fsm_schedule_cancel(Max_Conn_Timeout);
            Max_Conn_Timeout = 0;
            FOUT;      
		case 1:
			if(Send_MIB != 0)
				fsm_schedule_cancel(Send_MIB);
			Send_MIB = 0;
			FOUT;
		case 2:
			if(Send_SIB1!= 0)
				fsm_schedule_cancel(Send_SIB1);
			Send_SIB1 = 0;
			FOUT;
		case 3:
			if(Send_SI!= 0)
				fsm_schedule_cancel(Send_SI);
			Send_SI = 0;
			FOUT;
        default:
            FOUT;
    }
}


/******************************************************************************
 ** Function name:packet_send_to_rlc
 ** Descriptions: send packet to rlc, 
 ** Input:   msg to send, msg length and message type(defined in pkfmt.h)
 ** Output:  create a sb_buff which will be freed by lower layer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void packet_send_to_rlc(char *msg, int msg_len, u32 message_type, u16 rnti)
{
    FIN(packet_send_to_rlc());
 //   SV_PTR_GET(rrc_sv_enb_ue);
    fsm_printf("packet_send_to_rlc().\n");

    FSM_PKT* pkptr;
    struct lte_rrc_head* sh_ptr;
    struct URLC_IciMsg* ici_ptr;
    
    pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head) + sizeof(struct URLC_IciMsg));
	sh_ptr = (struct lte_rrc_head* ) fsm_mem_alloc(sizeof(struct lte_rrc_head));
	ici_ptr = (struct URLC_IciMsg* ) fsm_mem_alloc(sizeof(struct URLC_IciMsg));
	ici_ptr->rnti = rnti;
	sh_ptr->message_type = message_type;

	switch(message_type)
	{
	        case 0:
	            ici_ptr->pbCh = 1;
	            ici_ptr->rbId = -1;        //PCCH_Message
	            break;
	        case 1:
	            ici_ptr->pbCh = 2;
	            ici_ptr->rbId = -1;       //BCCH_BCH_Message
	            break;
			case 2:
	            ici_ptr->pbCh = 2;
	            ici_ptr->rbId = -1;       //BCCH_DL_SCH_Message
	            break;
			case 3:
	            ici_ptr->pbCh = 0;
	            ici_ptr->rbId = 0;       //DL_CCCH_Message(SRB0)
	            break;
			case 4:
	            ici_ptr->pbCh = 0;
	            ici_ptr->rbId = 1;       //DL_DCCH_Message(SRB1)
	            break;
	        default:
	            fsm_printf("rrc packet_send_to_rlc:Unrecognized message_type.\n");
	            break;
	    }
		 
   	//ICI space
	fsm_skb_reserve(pkptr,sizeof(struct URLC_IciMsg));
	fsm_mem_cpy(pkptr->head, ici_ptr, sizeof(struct URLC_IciMsg));
	
	//lte rrc head
	fsm_skb_put(pkptr, sizeof(struct lte_rrc_head));
	fsm_mem_cpy(pkptr->data, sh_ptr, sizeof(struct lte_rrc_head)); 
    
   	//data
	fsm_mem_cpy(pkptr->tail, msg, msg_len);
	fsm_skb_put(pkptr, msg_len);

	fsm_mem_free(sh_ptr);
	fsm_mem_free(ici_ptr);
	
	fsm_printf("<-- [rrc] send message to rlc.\n");
	/*fsm_octets_print(pkptr, 400);*/
	
	fsm_pkt_send(pkptr,STRM_TO_RLC);
    FOUT;
}

/******************************************************************************
 ** Function name:packet_send_to_pdcp
 ** Descriptions: send packet to pdcp, 
 ** Input:   msg to send, msg length and message type(defined in pkfmt.h)
 ** Output:  create a sb_buff which will be freed by lower layer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/

void packet_send_to_pdcp(char *msg, int msg_len, u32 message_type, u16 rnti)
{

    FIN(packet_send_to_rlc());
 //   SV_PTR_GET(rrc_sv_enb_ue);
    fsm_printf("packet_send_to_rlc().\n");

    FSM_PKT* pkptr;
    struct lte_rrc_head* sh_ptr;
    struct UPDCP_IciMsg* ici_ptr;
    
    pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head) + sizeof(struct UPDCP_IciMsg));
	sh_ptr = (struct lte_rrc_head* ) fsm_mem_alloc(sizeof(struct lte_rrc_head));
	ici_ptr = (struct UPDCP_IciMsg* ) fsm_mem_alloc(sizeof(struct UPDCP_IciMsg));
	
	ici_ptr->rnti = rnti;	
	sh_ptr->message_type = message_type;
	switch(message_type)
	{
	        case 0:
	            ici_ptr->pbCh = 1;
	            ici_ptr->rbId = -1;        //PCCH_Message
	            break;
	        case 1:
	            ici_ptr->pbCh = 2;
	            ici_ptr->rbId = -1;       //BCCH_BCH_Message
	            break;
			case 2:
	            ici_ptr->pbCh = 2;
	            ici_ptr->rbId = -1;       //BCCH_DL_SCH_Message
	            break;
			case 3:
	            ici_ptr->pbCh = 0;
	            ici_ptr->rbId = 0;       //DL_CCCH_Message(SRB0)
	            break;
			case 4:
	            ici_ptr->pbCh = 0;
	            ici_ptr->rbId = 1;       //DL_DCCH_Message(SRB1)
	            break;
	        default:
	            fsm_printf("rrc packet_send_to_rlc:Unrecognized message_type.\n");
	            break;
	    }
		 
   	//ICI space
	fsm_skb_reserve(pkptr,sizeof(struct UPDCP_IciMsg));
	fsm_mem_cpy(pkptr->head, ici_ptr, sizeof(struct UPDCP_IciMsg));
	
	//lte rrc head
	fsm_skb_put(pkptr, sizeof(struct lte_rrc_head));
	fsm_mem_cpy(pkptr->data, sh_ptr, sizeof(struct lte_rrc_head)); 
    
   	//data
	fsm_mem_cpy(pkptr->tail, msg, msg_len);
	fsm_skb_put(pkptr, msg_len);

	fsm_mem_free(sh_ptr);
	fsm_mem_free(ici_ptr);
	
	//for test.
    fsm_pkt_send(pkptr,STRM_TO_SRIO);
    //fsm_pkt_send(pkptr,STRM_TO_PDCP);

    FOUT;
}

