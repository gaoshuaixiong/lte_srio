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

#include "../lte_system.h"
#include "rrc_utils.h"
#include "rrc_conn_release.h"
#include "rrc_type_IEs.h"

extern void send_to_user(char *info, int size_info);


extern struct CellReselectionPriority *ueCellReselectionPriorityList;
extern struct LteDataRadioBearerInfo* ueDrbConfig[maxDRB];


/************************************************************************************************************************************************
**Function name:rcv_rrc_conn_release
**Desciption: recv rrc_conn_release_info ,release all radio resource
**Input : 
**Output:release all radio resource ,reset MAC
**Created by:zhou yang
**Created Data:2014/09/08
**--------------------------------------------------------------------------------------------------
**Modified by: zhou yang
**Modified Date: 2014/09/30
**Descriptions: modified format
**************************************************************************************************************************************************/

/*
static void rcv_rrc_conn_release(void)
{
	FIN(rcv_rrc_conn_release());
	fsm_printf("rcv_rrc_conn_release().\n");
	
	// fix me :use for generate release information 
	rrcConnectionRelease = gen_release();
	//use for storage_cell_reselection
	cell_reselection_priority_info(rrcConnectionRelease);
	//set timer
	set_t320(rrcConnectionRelease);
	//reset MAC
	fsm_do_ioctrl(STRM_TO_MAC,  IOCCMD_RRCTOMAC_RESET  , NULL, 0);
	//stop timer
	stop_timer();
	//release radio resource
	release_radio_resource();
	//fix me:report release cause information to up
	//report_to_userspace();
	fsm_mem_free(rrcConnectionRelease);
	rrcConnectionRelease=NULL;
	
	FOUT;
}
*/
/************************************************************************************************************************************************
**Function name:cell_reselection_priority_info
**Desciption:if rrcConnectionRelease_info contain IdleModeMobilityControlInfo  and IdleModeMobilityControlInfo contain haveCellReselectionPriorityInfo.
			:then storage cell_reselection_priority_info use for cell_reselection
**Input : rrcConnectionRelease
**Output:modified ueCellReselectionPriorityList£¬ueCellReselectionPriorityList use for storage ueCellReselectionPriorityList_info
**Created by:zhou yang
**Created Date:2014/09/08
**--------------------------------------------------------------------------------------------------
**Modified by:zhou yang
**Modified Date:2014/09/30
**Descriptions:modified format
**************************************************************************************************************************************************/
void cell_reselection_priority_info(struct RRCConnectionRelease *rrcConnectionRelease)
{
	FIN(cell_reselection_priority_info());
	fsm_printf("[rrc] [conn_release] cell_reselection_priority_info().\n");
	
	if(rrcConnectionRelease->haveIdleModeMobilityControlInfo)
	{
		
		//release old ueCellReselectionPriorityList 
		if(ueCellReselectionPriorityList!=NULL)
		{
			fsm_mem_free(ueCellReselectionPriorityList);
			ueCellReselectionPriorityList=NULL;
		}
		if(rrcConnectionRelease->idleModeMobilityControlInfo.haveCellReselectionPriorityInfo)
		{
			//if t320 overtime ;then free ueCellReselectionPriorityList
			ueCellReselectionPriorityList = (struct  CellReselectionPriority  *)fsm_mem_alloc(sizeof(struct CellReselectionPriority )*MAX_CELL_RESELECTION_SIZE);
			if(ueCellReselectionPriorityList==NULL)
			{
				fsm_printf("[rrc] [conn_release] cell_reselection_priority_info() apply memory failure \n");
				return ;
			}
			int i=0;
			for(i=0; i<MAX_CELL_RESELECTION_SIZE; i++)
			{
				ueCellReselectionPriorityList[i]=rrcConnectionRelease->idleModeMobilityControlInfo.cellReselectionPriorityList[i];
			}
		}
    }

    FOUT;
}
/************************************************************************************************************************************************
**Function name:set_t320
**Desciption:use for set the period of validity abort ueCellReselectionPriority_info
**Input : rrcConnectionRelease
**Output:modified t320 value
**Created by:zhou yang
**Created Date:2014/09/08
**--------------------------------------------------------------------------------------------------
**Modified by: zhou yang
**Modified Date: 2014/09/30
**Descriptions:modified format
*************************************************************************************************************************************************/
void set_t320(struct RRCConnectionRelease * rrcConnectionRelease)
{
	FIN(set_t320());
	fsm_printf("[rrc] [conn_release] set_t320().\n");
	if(rrcConnectionRelease->idleModeMobilityControlInfo.have_t320)
	{
		unsigned int delay;
		delay=rrcConnectionRelease->idleModeMobilityControlInfo.t320_Value;
		setTimer(20,delay*60*1000*1000*100);
	}
	FOUT;
}
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
	fsm_printf("[rrc] [conn_release] stop_timer().\n");
	cancleTimer(0);
	cancleTimer(1);
	cancleTimer(2);
	cancleTimer(5);
	cancleTimer(11);
	cancleTimer(21);
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
	
	fsm_printf("[rrc] [conn_release] release_radio_resource().\n");
	for(i=0; i<maxDRB; i++)
	{
		if(ueDrbConfig[i]!=NULL)
		{          	
			drb.drbToRelease[numIndex]=ueDrbConfig[i]->drbIdentity;
			numIndex++;
		}       
	}
	drb.num=numIndex;
	ApplySrbToRelease();//release SRB
	ApplyDrbToRelease(drb) ;//release DRB
	FOUT;
}

/**********************************************************************************************************************************************
**Function name:report_to_user
**Desciption:report release cause to up
**Input : rrcConnectionRelease
**Output:
**Created by:zhou yang
**Created Date:2014/09/08
**--------------------------------------------------------------------------------------------------
**Modified by:zhou yang
**Modified Date:2014/09/30
**Descriptions:modified format
**--------------------------------------------------------------------------------------------------

**********************************************************************************************************************************************/
/*
extern  void report_to_user(struct RRCConnectionRelease *rrcConnectionRelease)
{
	const char* releseCause_ptr = (char *)rrcConnectionRelease->releaseCause;
	fsm_do_ioctrl( STRM_TO_IP, IOCCMD_RRCTOUS_RECV_RRC_CONN_RELEASE, (void*)releseCause_ptr, sizeof((char)rrcConnectionRelease->releaseCause));
}
*/

/************************************************************************************************************************************************
**Function name:t320_timeout
**Desciption:if t320 timeout ,then free ueCellReselectionPriority info
**Input : ueCellReselectionPriorityList
**Output:free all ue storage ueCellReselectionPriority info
**Created by:zhou yang
**Created Date:2014/09/08
**--------------------------------------------------------------------------------------------------
**Modified by:zhou yang
**Modified Date:2014/09/30
**Descriptions:modified format
**--------------------------------------------------------------------------------------------------

*************************************************************************************************************************************************/
extern void t320_timeout(struct CellReselectionPriority *ueCellReselectionPriorityList)
{
	FIN(t320_timeout());
	fsm_printf("[rrc] [conn_release] t320_timeout().\n");
    
	if(ueCellReselectionPriorityList!=NULL)
	{
		fsm_mem_free(ueCellReselectionPriorityList);
		ueCellReselectionPriorityList=NULL;
	}
    FOUT;
}

/************************************************************************************************************************************************
**Function name:gen_release
**Desciption:use for testing ,generated rrcConnectionRelease info¡£
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
struct RRCConnectionRelease *  gen_release()
{
	struct RRCConnectionRelease * rrcConnectionRelease ;
	rrcConnectionRelease=(struct RRCConnectionRelease *)fsm_mem_alloc(sizeof(struct RRCConnectionRelease ));
	rrcConnectionRelease->rrcTransactionIdentifier=1;
	rrcConnectionRelease->haveIdleModeMobilityControlInfo=true;
	rrcConnectionRelease->releaseCause=other;
	rrcConnectionRelease->idleModeMobilityControlInfo.have_t320=true;
	rrcConnectionRelease->idleModeMobilityControlInfo.t320_Value=min5;
	rrcConnectionRelease->idleModeMobilityControlInfo.haveCellReselectionPriorityInfo=false;
	return rrcConnectionRelease;
}
/**/

