/***********************************************************************
**Filename:impl_rrc_conn_release.c
**Copyright:uestc.
**Descriptions:
**v1.0,2014/09/08 zhou yang
**------------------------------------------------
**modification history: 
**Modified by: zhou yang
**Modified date: 2014/09/30
**Descriptions: modified format
**
************************************************************************/

#include "rrc_conn_release_enb.h"
#include "../lte_system.h"
#include "../fsm/fsmdec.h"
#include "rrc_utils.h"

extern struct LteDataRadioBearerInfo* enbDRBConfig[MAX_UEFSM_NUM][MAX_DRB_NUM];

/************************************************************************************************************************************************
**Function name:stop_timer
**Desciption:stop all timer except t320
**Input : 
**Output:
**Created by:zhou yang
**Created Date:2014/09/08
**--------------------------------------------------------------------------------------------------
**Modified by:zhou yang
**Modified Date:2014/09/30
**Descriptions:modified format
**--------------------------------------------------------------------------------------------------

*************************************************************************************************************************************************/
void stop_timer(void)
{
	FIN(stop_timer());
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("[rrc] [conn_release] stop_timer().\n");
//	cancleTimer(0);
//	cancleTimer(1);
//	cancleTimer(2);
//	cancleTimer(5);
//	cancleTimer(11);
//	cancleTimer(21);
	FOUT;
}
/************************************************************************************************************************************************
**Function name:release_radio_resource
**Desciption:release all radio resource¡£
**Input : 
**Output:release srb drb
**Created by:zhou yang
**Created Date:2014/09/08
**--------------------------------------------------------------------------------------------------
**Modified by:zhou yang
**Modified Date:2014/09/30
**Descriptions:modified format
**--------------------------------------------------------------------------------------------------

*************************************************************************************************************************************************/
void release_radio_resource(void)
{
	FIN(release_radio_resource());
	int i;
	int numIndex=0;
	struct DrbToReleaseList drb;
	SV_PTR_GET(rrc_sv_enb_ue);
	fsm_printf("[rrc] [conn_release] release_radio_resource().\n");
	for(i=0; i<maxDRB; i++)
	{
		if(enbDRBConfig[SV(uefsmid)][i]!=NULL)
		{          	
			drb.drbToRelease[numIndex]=enbDRBConfig[SV(uefsmid)][i]->drbIdentity;
			numIndex++;
		}       
	}
	drb.num=numIndex;
	ApplySrbToRelease();//release SRB
	ApplyDrbToRelease(drb) ;//release DRB
	FOUT;
}



/************************************************************************************************************************************************
**Function name:send_rrc_connection_release_msg_to_ue
**Desciption:use for testing ,generated rrcConnectionRelease info
**Input : 
**Output:generated rrcConnectionRelease info ,and stored in the global variable.
**Created by:zhou yang
**Created Date:2014/09/08
**--------------------------------------------------------------------------------------------------
**Modified by:zhou yang
**Modified Date:2014/10/07
**Descriptions:modified format
**--------------------------------------------------------------------------------------------------

*************************************************************************************************************************************************/
void send_rrc_connection_release_msg_to_ue()
{
	SV_PTR_GET(rrc_sv_enb_ue);
	char *msg=NULL;
	int msg_len;
	int message_type;
	struct RRCConnectionRelease rrcConnectionRelease ;
	struct DL_DCCH_Message      *dl_dcch_message;
	FIN(send_rrc_connection_release_msg_to_ue());
	fsm_printf("[rrc] [conn_release] send_rrc_connection_release_msg_to_ue().\n");
	dl_dcch_message=(struct DL_DCCH_Message  *)fsm_mem_alloc(sizeof(struct DL_DCCH_Message  ));
	rrcConnectionRelease.rrcTransactionIdentifier=1;
	rrcConnectionRelease.haveIdleModeMobilityControlInfo=true;
	rrcConnectionRelease.releaseCause=other;
	rrcConnectionRelease.idleModeMobilityControlInfo.have_t320=true;
	rrcConnectionRelease.idleModeMobilityControlInfo.t320_Value=min5;
	rrcConnectionRelease.idleModeMobilityControlInfo.haveCellReselectionPriorityInfo=false;
	dl_dcch_message->msg.rrcConnectionRelease=rrcConnectionRelease;
	dl_dcch_message->type=2;
	msg=(char*)dl_dcch_message;
	msg_len=sizeof(struct RRCConnectionRelease);
	message_type=4;
	//fsm_octets_print(msg, msg_len);
	packet_send_to_rlc(msg, msg_len, message_type, SV(crnti));
	//fsm_mem_free(rrcConnectionRelease);
	//rrcConnectionRelease=NULL;
	FOUT;
	
}
/**/

