#include "rrc_utils.h"
#include "../lte_system.h"
#include "../pkfmt.h"
 
 
static evHandle T300 = NULL;       
static evHandle T301 = NULL;      
static evHandle T302 = NULL;      
static evHandle T305 = NULL;      
static evHandle T311 = NULL;       
static evHandle T320 = NULL;       
static evHandle T321 = NULL;       
 
struct LteSignalingRadioBearerInfo* ueSrbConfig[2]= {NULL};
struct LteDataRadioBearerInfo* ueDrbConfig[maxDRB]= {NULL}; 
 

/******************************************************************************
 ** Function name:setTimer
 ** Descriptions: set timer
 ** Input: the number of timer and the delay to set
 ** Output:  change evHandle pointer of related timer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void setTimer(int timerNum, unsigned int delay)
{
	FIN(setTimer());
	fsm_printf("[rrc] [fsm] setTimer(%d,%u).\n",timerNum,delay);
	switch(timerNum)
	{
		case TIMER_T300:
			T300 = fsm_schedule_self(delay, CODE_TIMEOUT_T300);
			FOUT;
		case TIMER_T301:
			T301 = fsm_schedule_self(delay, CODE_TIMEOUT_T301);
			FOUT;
		case TIMER_T302:
			T302 = fsm_schedule_self(delay, CODE_TIMEOUT_T302);
			FOUT;
		case TIMER_T305:
			T305 = fsm_schedule_self(delay, CODE_TIMEOUT_T305);
			FOUT;
		case TIMER_T311:
			T311 = fsm_schedule_self(delay, CODE_TIMEOUT_T311);
			FOUT;
		case TIMER_T320:
			T320 = fsm_schedule_self(delay, CODE_TIMEOUT_T320);
			FOUT;
		case TIMER_T321:
			T321 = fsm_schedule_self(delay, CODE_TIMEOUT_T321);
			FOUT;

		default:
			FOUT;
	}
}

 
/******************************************************************************
 ** Function name:cancleTimer
 ** Descriptions: cancle timer
 ** Input: the number of timer
 ** Output:  change evHandle pointer of related timer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void cancleTimer(int timerNum)
{
	FIN(cancleTimer());
	fsm_printf("[rrc] [fsm] cancleTimer(%d).\n",timerNum);
	switch(timerNum)
	{
		case TIMER_T300:
			if (T300 != NULL)
				fsm_schedule_cancel(T300);
			T300 = NULL;
			FOUT;
		case TIMER_T301:
			if (T301 != NULL)
				fsm_schedule_cancel(T301);
			T301 = NULL;
			FOUT;
		case TIMER_T302:
			if (T302 != NULL)
				fsm_schedule_cancel(T302);
			T302 = NULL;
			FOUT;
		case TIMER_T305:
			if (T305 != NULL)
				fsm_schedule_cancel(T305);
			T305 = NULL;
			FOUT;
		case TIMER_T311:
			if (T311 != NULL)
				fsm_schedule_cancel(T311);
			T311 = NULL;
			FOUT;
		case TIMER_T320:
			if (T320 != NULL)
				fsm_schedule_cancel(T320);
			T320 = NULL;
			FOUT;
		case TIMER_T321:
			if (T321 != NULL)
				fsm_schedule_cancel(T321);
			T321 = NULL;
			FOUT;
		default:
			FOUT;
	}
}


/******************************************************************************
 ** Function name:checkTimer
 ** Descriptions: check a timer is running or not
 ** Input: the number of timer
 ** Output: void 
 ** Returns: return 1 if timer is running; return 0 if timer is not running.
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
int checkTimer(int timerNum)
{
	evHandle timerHandle;
	switch(timerNum)
	{
		case TIMER_T300:
			timerHandle = T300;
			break;
		case TIMER_T301:
			timerHandle = T301;
			break;
		case TIMER_T302:
			timerHandle = T302;
			break;
		case TIMER_T305:
			timerHandle = T305;
			break;
		case TIMER_T311:
			timerHandle = T311;
			break;
		case TIMER_T320:
			timerHandle = T320;
			break;
		case TIMER_T321:
			timerHandle = T321;
			break;
		default:
			return 0;
	}

	return (timerHandle == NULL ? 0 : 1);

}
 
 
/******************************************************************************
 ** Function name:packet_send_to_pdcp
 ** Descriptions: send packet to pdcp
 ** Input:   msg to send, msg length and message type
 ** Output:  create a sb_buff which will be freed by lower layer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void packet_send_to_pdcp(char *msg, int msg_len, u32 message_type)
{

	FIN(packet_send_to_pdcp());
	SV_PTR_GET(rrc_sv);
	fsm_printf("[rrc] [fsm] <-- packet_send_to_pdcp().\n");

	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr = (struct lte_rrc_head* ) fsm_mem_alloc(sizeof(struct lte_rrc_head));
	struct UPDCP_IciMsg* ici_ptr = (struct UPDCP_IciMsg* ) fsm_mem_alloc(sizeof(struct UPDCP_IciMsg));

	sh_ptr->message_type = message_type;
	switch(message_type)
	{
		case 4:
			ici_ptr->pbCh = 0;
			ici_ptr->rbId = 1;       //UL_DCCH_Message(SRB1)
			break;
		case 6:
			ici_ptr->pbCh = 0;
			ici_ptr->rbId = 1;       //DL_DCCH_Message(SRB1)
			break;
		default:
			fsm_printf("[rrc] [fsm] packet_send_to_pdcp:Unrecognized message_type.\n");
			break;
	}

	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head) + sizeof(struct UPDCP_IciMsg) + 10);

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

	fsm_pkt_send(pkptr,STRM_TO_PDCP);
//	fsm_pkt_send(pkptr,STRM_TO_RLC);
	FOUT;

}

 
 
 /******************************************************************************
 ** Function name:packet_send_to_rlc
 ** Descriptions: send packet to rlc
 ** Input:   msg to send, msg length and message type
 ** Output:  create a sb_buff which will be freed by lower layer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void packet_send_to_rlc(char *msg, int msg_len, u32 message_type)
{

	FIN(packet_send_to_rlc());
	SV_PTR_GET(rrc_sv);
	fsm_printf("[rrc] [fsm] <-- packet_send_to_rlc().\n");

	FSM_PKT* pkptr;
	struct lte_rrc_head* sh_ptr = (struct lte_rrc_head* ) fsm_mem_alloc(sizeof(struct lte_rrc_head));
	struct URLC_IciMsg* ici_ptr = (struct URLC_IciMsg* ) fsm_mem_alloc(sizeof(struct URLC_IciMsg));

	sh_ptr->message_type = message_type;
	switch(message_type)
	{
		case 0:      //PCCH
			ici_ptr->pbCh = 1;
			break;
		case 1: 	//BCCH
			ici_ptr->pbCh = 2;
			break;
		case 2:     //BCCH_DL_SCH
			ici_ptr->pbCh = 2;
			break;
		case 3:    //DL_CCCH
			ici_ptr->pbCh = 0;
			ici_ptr->rbId = 0;   //srb0   
			break;
		case 5:    //UL_CCCH
			ici_ptr->pbCh = 0;
			ici_ptr->rbId = 0;   //srb0   
			break;
		default:
			fsm_printf("[rrc] [fsm] packet_send_to_rlc:Unrecognized message_type.\n");
			break;
	}

	pkptr = fsm_pkt_create(msg_len + sizeof(struct lte_rrc_head) + sizeof(struct URLC_IciMsg));

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
	
	fsm_octets_print(pkptr->data, 8);
	fsm_pkt_send(pkptr,STRM_TO_RLC);
	FOUT;
}


/******************************************************************************
 ** Function name:packet_send_to_upperlayer
 ** Descriptions: send packet to upper
 ** Input:   msg to send, msg length and message type
 ** Output:  create a sb_buff which will be freed by upper layer
 ** Returns: void
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void packet_send_to_upperlayer(void)
{
	FSM_PKT* pkptr;
	FIN(packet_send_to_upperlayer());
	pkptr = fsm_pkt_get();
	fsm_printf("[rrc] [fsm] <-- packet_send_to_upperlayer().\n");

	//fsm_pkt_send(pkptr, STRM_TO_IP);
	FOUT;
}

void check_free_ptr(void *ptr)
{
	if(ptr == NULL)
		return;
	fsm_mem_free(ptr);
	ptr = NULL;
}
