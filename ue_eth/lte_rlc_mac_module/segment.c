#include "rlc_macfsm.h"
#include "segment.h"
#include "../lte_system.h"
#include <linux/if_ether.h>
#include "../pkfmt.h"


/*******************************************************************************************
** Function name: get_AmPduhdExt()
** Descriptions	: get PDU extend head in AM mode
** Input:
**     the struct of AmBuffer pointer  and _pduhdrExt pointer , u16 count,u16 E
** Output:
**	return	: count
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:li xu long
** Modified Date: 2014-10-16
**------------------------------------------------------------------------------------------
*******************************************************************************************/

u16 get_AmPduhdExt(u16 E,struct AmBuffer *tbuff,u16 count,struct _pduhdrExt *pPduhdrExt)
{
	if(E==1)
	{
		u16 b=0;
		fsm_mem_cpy(&b,tbuff->pkt->data,2);
		fsm_skb_pull(tbuff->pkt, 2) ;
		pPduhdrExt->E[count] = (b&0x1000)>>15;
		pPduhdrExt->LI[count] = (b&0x7fff);
		count++;
		
		count = get_AmPduhdExt(pPduhdrExt->E[count-1],tbuff,count,pPduhdrExt); 
	}
	return count;
}

/*******************************************************************************************
** Function name: get_UmPduhdExt()
** Descriptions	: get PDU Extend head and the length of Extend head in UM10 mode
** Input:
**     the struct of UmBuffer pointer and _pduhdrExt pointer,u16 count
** Output:
**	return	:count.
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:
** Modified Date:
**------------------------------------------------------------------------------------------
*******************************************************************************************/

u16 get_UmPduhdExt(u16 E,struct UmBuffer *tbuff,u16 count,struct _pduhdrExt *pPduhdrExt)
{
	if(E==1)
	{
		u16 b=0;
		fsm_mem_cpy(&b,tbuff->pkt->data,2);
		fsm_skb_pull(tbuff->pkt, 2) ;
		pPduhdrExt->E[count] = (b&0x1000)>>15;
		pPduhdrExt->LI[count] = (b&0x7fff);

		count++;
		count = get_UmPduhdExt(pPduhdrExt->E[count-1],tbuff,count,pPduhdrExt);
	}
	return count;
}


/*******************************************************************************************
** Function name: clear_left_pdu()
** Descriptions	: clear those pdu that cannot be assembled
** Input:
**     the struct of pduLeft pointer 
** Output:
**	return	:void.
** Created by	:li xu long
** Created Date	: 2015-05-14
**------------------------------------------------------------------------------------------
** Modified by	:
** Modified Date:
**------------------------------------------------------------------------------------------
*******************************************************************************************/

void clear_left_pdu(struct pduLeft *pdu_left)
{	
	pdu_left->SN_Left = -1;
	pdu_left->SN5_Left = -1;
	if(pdu_left->sduLeft != NULL)
	{
	    fsm_pkt_destroy(pdu_left->sduLeft);
		pdu_left->sduLeft = NULL;
	}               
}


/*******************************************************************************************
** Function name: rlc_AM_segment()
** Descriptions	:segment or concatenate SDU in AM mode
** Input:
**     the struct of AM_Instancer pointer and  u32 sizeFromMac
** Output:
**	return	:PDU.
**alloc :PDU 
** free :SDU
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:li xu long
** Modified Date: 2015-3-6
**------------------------------------------------------------------------------------------
*******************************************************************************************/
FSM_PKT * rlc_AM_segment(struct AM_Instance *amIns,u32 sizeFromMac)
{

	u16 lastCopyLen,i=0,count = 0,length = 0;
	u32 size;
	u16 num = amIns->amSduBufferNum;
	bool flag=true;
	FSM_PKT *skb;//*skb_copy;
	struct list_head *head =&(amIns->amSduBuffer.list);
	struct AmSduBuffer *amBuffer,*tempBuffer, *tbuff;
	struct RLC_AMPDU_fixhead *fixhead, amhead;
	struct list_head *pos;
	struct AmBuffer *amBuf;
	fixhead=&amhead;
	fixhead->SN=amIns->SN;
	fixhead->DorC=1;
	fixhead->RF=0;
	fixhead->P=1;
	//fixhead->FIfirst=2;
	
	if(sizeFromMac <= 2)
	{
		printk("[RLC][rlc_AM_segment] size <= 2\n");
		return NULL;
	}
	size = sizeFromMac - 2;
	//fsm_printf(5,"[RLC] entring rlc_AM_segment\n");
	if(!list_empty(head))
	{
		list_for_each_entry(amBuffer,head,list)
		{
			count = count + 1;
			num = num -1;
			if(size < amBuffer->pkt->len)
			{
				if(amBuffer->pktstatus ==FULL_SDU)
				{
					if(flag)
					{
						fixhead->FIfirst=0;
						fixhead->E=0;
						fixhead->FIsecond=1;
						flag=false;
					}		
					else
					{
						fixhead->FIsecond=1;
					}
				}
				else
				{
					if(flag)
					{
						fixhead->FIfirst=1;
						fixhead->E=0;
						fixhead->FIsecond=1;
						flag=false;
					}
					else
					{
						fixhead->FIsecond=1;
					}
				}
				lastCopyLen = size;
				length = length + size;
				break;
			}
			else if(size == amBuffer->pkt->len) 
			{

				if(amBuffer->pktstatus == FULL_SDU)
				{
					if(flag)
					{
						fixhead->FIfirst=0;
						fixhead->E=0;
						fixhead->FIsecond=0;
						flag=false;
					}
					else
					{
						fixhead->FIsecond=0;
					}
				}
				else
				{
					if(flag)
					{
						fixhead->FIfirst=1;
						fixhead->E=0;
						fixhead->FIsecond=0;
						flag=false;
					}
					else
					{
						fixhead->FIsecond=0;
					}
				}
				lastCopyLen = size;
				length = length + size;
				break;
			}
			else if(size > (amBuffer->pkt->len + 2))
			{

				size = size - amBuffer->pkt->len - 2;
				length = length + amBuffer->pkt->len;
				if(amBuffer->pktstatus == FULL_SDU && flag)
				{
					fixhead->FIfirst=0;
					fixhead->E=1;
					flag=false;
				}
				else if(amBuffer->pktstatus != FULL_SDU && flag)
				{
					fixhead->FIfirst=1;
					fixhead->E=1;
					flag=false;
				}
				if(num == 0)
				{
					if((amIns->amSduBufferNum)==1)
					{
						fixhead->E=0;
					}
					fixhead->FIsecond=0;
					lastCopyLen=amBuffer->pkt->len;
					break;
				}				
			}
			else							 
			{			
				if(amBuffer->pktstatus == FULL_SDU)
				{
					if(flag)
					{
						fixhead->FIfirst=0;
						fixhead->E=0;
						fixhead->FIsecond=0;
						flag=false;
					}
					else
					{
						fixhead->FIsecond=0;
					}
				}
				else
				{
					if(flag)
					{
						fixhead->FIfirst=1;
						fixhead->E=0;
						fixhead->FIsecond=0;
						flag=false;
					}
					else
					{
						fixhead->FIsecond=0;
					}
				}
				lastCopyLen = amBuffer->pkt->len ;
				length = length + amBuffer->pkt->len;
				break;
			}
		}
	}
	*(u16*)fixhead=htons(*(u16*)fixhead);
	fsm_octets_print(fixhead,2);
	length = length + 2*count;
	skb = fsm_pkt_create(length) ;
	pos = head->next;
	tbuff = list_entry(pos,struct AmSduBuffer,list);
	
	//struct AmBuffer *amBf = (struct AmBuffer*)fsm_mem_alloc(count*sizeof(struct AmBuffer));
	//int j = 0;
	if(count == 1)
	{
		fsm_skb_reserve(skb,length);
		fsm_mem_cpy(fsm_skb_push(skb,lastCopyLen),tbuff->pkt->data,lastCopyLen);
		fsm_mem_cpy(fsm_skb_push(skb,2),fixhead,2);
		
		if(lastCopyLen == tbuff->pkt->len)
		{
			list_del(&tbuff->list);
			amIns->amSduBufferNum--;
			amIns->amSduBufferSize -= tbuff->pkt->len;
			if(tbuff->pkt != NULL)
			{
				fsm_pkt_destroy(tbuff->pkt);
				tbuff->pkt = NULL;
			}
			if(tbuff != NULL)
			{
				fsm_mem_free(tbuff);
				tbuff = NULL;
			}		
		}
		else
		{
			fsm_skb_pull(tbuff->pkt,lastCopyLen);
			amIns->amSduBufferSize -= lastCopyLen;
			tbuff->pktstatus = LAST_SEGMENT;
		}		
	}
	else if(count > 1)
	{
		fsm_skb_reserve(skb,2*count);		
		u16 *LI = (u16*)fsm_mem_alloc((count-1)*sizeof(u16));
		u16 *headExt = (u16*)fsm_mem_alloc((count-1)*sizeof(u16));
		i = 0;
		u8 *des = skb->data;
		list_for_each_entry_safe(amBuffer,tempBuffer,head,list)
		{
			if(i<count-1)
			{
				*(LI+i) = amBuffer->pkt->len;
				*(headExt+i) = 0x8000|(*(LI+i));
				fsm_skb_put(skb,amBuffer->pkt->len);
				fsm_mem_cpy(des,amBuffer->pkt->data,amBuffer->pkt->len);
				des = des + amBuffer->pkt->len ;
				list_del(&amBuffer->list);
				amIns->amSduBufferNum--;
				amIns->amSduBufferSize -= amBuffer->pkt->len;

				if(amBuffer->pkt != NULL)
				{
					fsm_pkt_destroy(amBuffer->pkt);
					amBuffer->pkt = NULL;
				}
				if(amBuffer != NULL)
				{
					fsm_mem_free(amBuffer);
					amBuffer = NULL;
				}				
				i++;
			}
			else if(i==count-1)
			{
				fsm_skb_put(skb,lastCopyLen);
				fsm_mem_cpy(des,amBuffer->pkt->data,lastCopyLen);
				
				if(amBuffer->pkt->len == lastCopyLen)
				{
					list_del(&amBuffer->list);
					amIns->amSduBufferNum--;
					amIns->amSduBufferSize -= amBuffer->pkt->len;
					if(amBuffer->pkt != NULL)
					{
						fsm_pkt_destroy(amBuffer->pkt);
						amBuffer->pkt = NULL;
					}
					if(amBuffer != NULL)
					{
						fsm_mem_free(amBuffer);
						amBuffer = NULL;
					}				
				}
				else
				{
					fsm_skb_pull(amBuffer->pkt,lastCopyLen);
					amIns->amSduBufferSize -= lastCopyLen;
					amBuffer->pktstatus = LAST_SEGMENT;
				}			   
				break;
			}
		}		
		*(headExt+count-2) = (*(headExt+count-2))&0x7fff;  //last Extent E = 0;
		fsm_mem_cpy(fsm_skb_push(skb,2*(count-1)),headExt,2*(count-1));
		fsm_mem_cpy(fsm_skb_push(skb,2),fixhead,2);
		if(LI != NULL)
		{
			fsm_mem_free(LI);
			LI = NULL;
		}
		if(headExt != NULL)
		{
			fsm_mem_free(headExt);
			headExt = NULL;
		}		
	}
	//fsm_printf("[RLC] the context of PDU is %s\n",(char*)(skb->data+2*count));
	//fsm_printf(5,"[RLC][AM_segment] the context of PDU is\n");
	//add pdu to trasmitted Buffer
	//skb_copy=skb;
	//fsm_octets_print(skb->data,10);
	amBuf = (AmBuffer *)fsm_mem_alloc(sizeof(AmBuffer));
	amBuf->pkt = fsm_pkt_create(skb->truesize);
	amBuf->pkt = fsm_pkt_duplicate(skb);
	//amBuf->pkt = skb_copy;
	amBuf->headLen = 2*count;
	amBuf->datalen = amBuf->pkt->len;
	amBuf->segnum = 0;
	int j;
	for( j = 0; j < segNumMax; j++ )
	{
		amBuf->segStart[j] = -1;
		amBuf->segEnd[j] = -1;
	}
	amBuf->SN = amIns->SN;
	amBuf->retxCount = -1;
	list_add_tail(&amBuf->list, &amIns->amTransmittedBuffer.list);
	amIns->amTransmittedBufferSize += amBuf->pkt->len;
	amIns->amTransmittedBufferNum++;
	amIns->SN++;
	amIns->SN %= 1024;
	fsm_printf("[RLC][rlc_AM_segment] SN = %d\n",amIns->SN);
	return skb;
}



/*******************************************************************************************
** Function name: rlc_UM_segment()
** Descriptions	:segment or concatenate SDU in UM10 mode
** Input:
**     the struct of UM_TX_Instancer pointer and sizeFromMac
** Output:
**	return	:PDU.
**alloc  : PDU
**free  : SDU
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:
** Modified Date:
**------------------------------------------------------------------------------------------
*******************************************************************************************/

FSM_PKT * rlc_UM_segment(struct UM_TX_Instance *umIns,u32 sizeFromMac)
{
		u32 size;
		int num = umIns->umSduBufferNum;
		struct list_head *head =&(umIns->umSduBuffer.list);
		struct UmBuffer *umBuffer,*tempBuffer,*tbuff;
		struct RLC_UM_size10_subhead *fixhead, umhead;
		fixhead=&umhead;
		fixhead->SN=umIns->SN;
		fixhead->R11=0;
		fixhead->R12=0;
		fixhead->R13=0;
		//fixhead->FIfirst=2;
		int count = 0;
		unsigned int lastCopyLen;
		u16 length =0;
		bool flag=true;
		FSM_PKT *skb;
		struct list_head *pos;
		fsm_printf("[RLC] entring rlc_UM_segment\n");
		if(sizeFromMac <= 2)
		{
			printk("[RLC][rlc_UM_segment] size <= 2\n");
			return NULL;
		}
		size = sizeFromMac - 2;
		if(!list_empty(head))
		{
			list_for_each_entry(umBuffer,head,list)
			{
				count++;
				num--;
				if(size < umBuffer->pkt->len)
				{	
					if(umBuffer->pktstatus ==FULL_SDU)
					{
						if(flag)
						{
							fixhead->FIfirst=0;
							fixhead->E=0;
							fixhead->FIsecond=1;
							flag=false;
						}
						else
						{
							fixhead->FIsecond=1;
						}
					}
					else
					{
						if(flag)
						{
							fixhead->FIfirst=1;
							fixhead->E=0;
							fixhead->FIsecond=1;
							flag=false;
						}
						else
						{
							fixhead->FIsecond=1;
						}
					}
					lastCopyLen = size;
					length += size;
					break;
				}
				else if(size == umBuffer->pkt->len) 
				{	
					if(umBuffer->pktstatus ==FULL_SDU)
					{
						if(flag)
						{
							fixhead->FIfirst=0;
							fixhead->E=0;
							fixhead->FIsecond=0;
							flag=false;
						}
						else
						{
							fixhead->FIsecond=0;
						}
					}
					else
					{
						if(flag)
						{
							fixhead->FIfirst=1;
							fixhead->E=0;
							fixhead->FIsecond=0;
							flag=false;
						}
						else
						{
							fixhead->FIsecond=0;
						}
					}
					lastCopyLen = size;
					length += size;
					break;
				}
				else if(size > (umBuffer->pkt->len + 2))
				{
	
					size = size - umBuffer->pkt->len - 2;
					length += umBuffer->pkt->len;
					if(umBuffer->pktstatus==FULL_SDU &&flag)
					{	
						fixhead->FIfirst=0;
						fixhead->E=1;
						flag=false;
					}
					else if(umBuffer->pktstatus !=FULL_SDU &&flag)
					{
	
						fixhead->FIfirst=1;
						fixhead->E=1;
						flag=false;
					}
					if(num == 0)
					{
						if((umIns->umSduBufferNum) == 1)
							fixhead->E=0;
						fixhead->FIsecond=0;
						lastCopyLen = umBuffer->pkt->len;
						break;
					}
				}
				else							
				{	
					if(umBuffer->pktstatus ==FULL_SDU)
					{
						if(flag)
						{
							fixhead->FIfirst=0;
							fixhead->E=0;
							fixhead->FIsecond=0;
							flag=false;
						}
						else
						{
							fixhead->FIsecond=0;
						}
					}
					else
					{
						if(flag)
						{
							fixhead->FIfirst=1;
							fixhead->E=0;
							fixhead->FIsecond=0;
							flag=false;
						}
						else
						{
							fixhead->FIsecond=0;
						}
					}
					lastCopyLen = umBuffer->pkt->len ;
					length += umBuffer->pkt->len;
					break;
				}
			}
		}
		*(u16*)fixhead=htons(*(u16*)fixhead);
		fsm_octets_print(fixhead,2);
		length += 2*count;
		skb = fsm_pkt_create(length) ;
		pos = head->next;
		tbuff = list_entry(pos,struct UmBuffer,list);	
		if(count == 1)
		{
			fsm_skb_reserve(skb,length);
			fsm_mem_cpy(fsm_skb_push(skb,lastCopyLen),tbuff->pkt->data,lastCopyLen);
			fsm_mem_cpy(fsm_skb_push(skb,2),fixhead,2);
			
			if(lastCopyLen == tbuff->pkt->len)
			{
				list_del(&tbuff->list);
				umIns->umSduBufferNum--;
				umIns->umSduBufferSize -= tbuff->pkt->len;
				fsm_pkt_destroy(tbuff->pkt);
				tbuff->pkt = NULL;
				fsm_mem_free(tbuff);
				tbuff = NULL;
			}
			else
			{
				fsm_skb_pull(tbuff->pkt,lastCopyLen);
				umIns->umSduBufferSize -= lastCopyLen;
				tbuff->pktstatus = LAST_SEGMENT;
			}
		}
		else if(count > 1)
		{
			fsm_skb_reserve(skb,2*count);
			u16 *LI = (u16*)fsm_mem_alloc((count-1)*sizeof(u16));
			u16 *headExt = (u16*)fsm_mem_alloc((count-1)*sizeof(u16));
			int i = 0;
			unsigned char *des = skb->data;
			list_for_each_entry_safe(umBuffer,tempBuffer,head,list)
			{
				if(i<count-1)
				{
					*(LI+i) = umBuffer->pkt->len;
					*(headExt+i) = 0x8000|(*(LI+i));
					fsm_skb_put(skb,umBuffer->pkt->len);
					fsm_mem_cpy(des,umBuffer->pkt->data,umBuffer->pkt->len);
					des = des + umBuffer->pkt->len ;
					list_del(&umBuffer->list);
					umIns->umSduBufferNum--;
					umIns->umSduBufferSize -= umBuffer->pkt->len;
					fsm_pkt_destroy(umBuffer->pkt);
					umBuffer->pkt = NULL;
					fsm_mem_free(umBuffer);
					umBuffer = NULL;
					i++;
				}
				else if(i==count-1)
				{
					fsm_skb_put(skb,lastCopyLen);
					fsm_mem_cpy(des,umBuffer->pkt->data,lastCopyLen);
					fsm_skb_pull(umBuffer->pkt,lastCopyLen);
					if(umBuffer->pkt->len == lastCopyLen)
					{
						list_del(&umBuffer->list);
						umIns->umSduBufferNum--;
						umIns->umSduBufferSize -= umBuffer->pkt->len;
						fsm_pkt_destroy(umBuffer->pkt);
						umBuffer->pkt = NULL;
						fsm_mem_free(umBuffer);
						umBuffer = NULL;
					}
					else
					{
						umBuffer->pktstatus = LAST_SEGMENT;
						umIns->umSduBufferSize -= lastCopyLen;
					}
					break;
				}
			}
			*(headExt+count-2)=(*(headExt+count-2)&0x7fff);
			fsm_mem_cpy(fsm_skb_push(skb,2*(count-1)),headExt,2*(count-1));
			fsm_mem_cpy(fsm_skb_push(skb,2),fixhead,2);
			fsm_mem_free(LI);
			fsm_mem_free(headExt);
			LI = NULL;
			headExt = NULL;
		}
		//fsm_printf("[RLC] the context of PDU is %s\n",(char*)(skb->data+2*count));
		umIns->SN++;
		umIns->SN %= 1024;
		fsm_printf("[RLC][rlc_UM_segment] SN = %d\n",umIns->SN);
		return skb;

}


/*******************************************************************************************
** Function name: rlc_UM5_segment()
** Descriptions	:segment or concatenate SDU in UM5 mode
** Input:
**     the struct of UM_TX_Instancer pointer and sizeFromMac
** Output:
**	return	:PDU.
**alloc  : PDU
**free  : SDU
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:li xu long 
** Modified Date: 2015-03-06
**------------------------------------------------------------------------------------------
*******************************************************************************************/

FSM_PKT * rlc_UM5_segment(struct UM_TX_Instance *umIns,u32 sizeFromMac)
{
	u32 size;
	int num = umIns->umSduBufferNum;
	u16 length=0, count=0,lastCopyLen;
	FSM_PKT *skb;
	struct list_head *pos;
	struct list_head *head =&(umIns->umSduBuffer.list);
	struct UmBuffer *umBuffer,*tempBuffer,*tbuff;
	struct RLC_UM_size5_head *fixhead, umhead;
	fixhead=&umhead;
	fixhead->SN=umIns->SN;
	bool flag=true;
	//fixhead->FIfirst=2;
	
	if(sizeFromMac <= 1)
	{
		printk("[RLC][rlc_UM5_segment] size <= 1\n");
		return NULL;
	}
	size = sizeFromMac - 1;
	fsm_printf("[RLC] entring rlc_UM5_segment\n");
	if(!list_empty(head))
	{
		list_for_each_entry(umBuffer,head,list)
		{
			count++;
			num--;
			if(size < umBuffer->pkt->len)
			{

				if(umBuffer->pktstatus ==FULL_SDU)
				{
					if(flag)
					{
						fixhead->FIfirst=0;
						fixhead->E=0;
						fixhead->FIsecond=1;
						flag=false;
					}
					else
						fixhead->FIsecond=1;
				}
				else
				{
					if(flag)
					{
						fixhead->FIfirst=1;
						fixhead->E=0;
						fixhead->FIsecond=1;
						flag=false;
					}
					else
						fixhead->FIsecond=1;
				}
				lastCopyLen = size;
				length += size;
				break;
			}
			else if(size == umBuffer->pkt->len)
			{

				if(umBuffer->pktstatus ==FULL_SDU)
				{
					if(flag)
					{
						fixhead->FIfirst=0;
						fixhead->E=0;
						fixhead->FIsecond=0;
						flag=false;
					}
					else
						fixhead->FIsecond=0;
				}
				else
				{
					if(flag)
					{
						fixhead->FIfirst=1;
						fixhead->E=0;
						fixhead->FIsecond=0;
						flag=false;
					}		
					else
						fixhead->FIsecond=0;
				}
				lastCopyLen = size;
				length += size;
				break;
			}
			else if(size > (umBuffer->pkt->len + 2))
			{

				size = size - umBuffer->pkt->len - 2;
				length += umBuffer->pkt->len;
				if(umBuffer->pktstatus==FULL_SDU &&flag)
				{

					fixhead->FIfirst=0;
					fixhead->E=1;
					flag=false;
				}
				else if(umBuffer->pktstatus !=FULL_SDU &&flag)
				{

					fixhead->FIfirst=1;
					fixhead->E=1;
					flag=false;
				}
				if(num == 0)
				{
					if((umIns->umSduBufferNum)==1)
						fixhead->E=0;
					fixhead->FIsecond=0;
					lastCopyLen=umBuffer->pkt->len;
					break;   
				}
			}
			else                             
			{

				if(umBuffer->pktstatus ==FULL_SDU)
				{
					if(flag)
					{
						fixhead->FIfirst=0;
						fixhead->E=0;
						fixhead->FIsecond=0;
						flag=false;
					}
					else
						fixhead->FIsecond=0;
				}
				else
				{
					if(flag)
					{
						fixhead->FIfirst=1;
						fixhead->E=0;
						fixhead->FIsecond=0;
						flag=false;
					}
					else
						fixhead->FIsecond=0;
				}
				lastCopyLen = umBuffer->pkt->len ;
				length += umBuffer->pkt->len;
				break;
			}
		}
	}
	
	fsm_octets_print(fixhead,2);
	length = length + (2*count-1);
	//fsm_printf("%d\n",length);
	skb = fsm_pkt_create(length) ;
	pos = head->next;
	tbuff = list_entry(pos,struct UmBuffer,list);

	if(count == 1)
	{
		fsm_skb_reserve(skb,length);
		fsm_mem_cpy(fsm_skb_push(skb,lastCopyLen),tbuff->pkt->data,lastCopyLen);
		fsm_mem_cpy(fsm_skb_push(skb,1),fixhead,1);
		
		if(lastCopyLen == tbuff->pkt->len)
		{
			list_del(&tbuff->list);
			umIns->umSduBufferNum--;
			umIns->umSduBufferSize -= tbuff->pkt->len;
			fsm_pkt_destroy(tbuff->pkt);
			tbuff->pkt = NULL;
			fsm_mem_free(tbuff);
			tbuff = NULL;
		}
		else
		{
			fsm_skb_pull(tbuff->pkt,lastCopyLen);
			tbuff->pktstatus = LAST_SEGMENT;
			umIns->umSduBufferSize -=lastCopyLen;
		}
	}
	else if(count > 1)
	{
		fsm_skb_reserve(skb,(2*count-1));
		u16 *LI = (u16*)fsm_mem_alloc((count-1)*sizeof(u16));
		u16 *headExt = (u16*)fsm_mem_alloc((count-1)*sizeof(u16));
		int i = 0;
		unsigned char *des = skb->data;
		list_for_each_entry_safe(umBuffer,tempBuffer,head,list)
		{
			if(i<count-1)
			{
				*(LI+i) = umBuffer->pkt->len;
				*(headExt+i) = 0x80|(*(LI+i));
				fsm_skb_put(skb,umBuffer->pkt->len);
				fsm_mem_cpy(des,umBuffer->pkt->data,umBuffer->pkt->len);
				des = des + umBuffer->pkt->len ;
				list_del(&umBuffer->list);
				umIns->umSduBufferNum--;
				umIns->umSduBufferSize -= tbuff->pkt->len;
				fsm_pkt_destroy(umBuffer->pkt);
				umBuffer->pkt = NULL;
				fsm_mem_free(umBuffer);
				umBuffer = NULL;
				i++;
			}
			else if(i==count-1)
			{
				fsm_skb_put(skb,lastCopyLen);
				fsm_mem_cpy(des,umBuffer->pkt->data,lastCopyLen);
				
				if(umBuffer->pkt->len == lastCopyLen)
				{
					list_del(&umBuffer->list);
					umIns->umSduBufferNum--;
					umIns->umSduBufferSize -= tbuff->pkt->len;
					fsm_pkt_destroy(umBuffer->pkt);
					umBuffer->pkt = NULL;
					fsm_mem_free(umBuffer); 
					umBuffer = NULL;
				}
				else
				{
					fsm_skb_pull(umBuffer->pkt,lastCopyLen);
					umIns->umSduBufferSize -=lastCopyLen;
					umBuffer->pktstatus=LAST_SEGMENT;
				}
				break;
			}
							
		}
		*(headExt+count-2)=(*(headExt+count-2)&0x7fff);
		fsm_mem_cpy(fsm_skb_push(skb,2*(count-1)),headExt,2*(count-1));
		fsm_mem_cpy(fsm_skb_push(skb,1),fixhead,1);
	}
	//fsm_printf("[RLC] the context of PDU is %s\n",(char*)(skb->data+2*count-1));	
	//fsm_printf("[RLC] the PDU length is %d\n",skb->len);
	umIns->SN++;
	umIns->SN %= 32;
	fsm_printf("[RLC][rlc_UM5_segment] SN = %d\n",umIns->SN);
	return skb;
}


/*******************************************************************************************
** Function name: rlc_resegment()
** Descriptions	:PDU resegment
** Input:
**     the struct of Ambuffer pointer and sizeFromMac
** Output:
**	return	:PDU segment.
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:li xu long
** Modified Date:2015-03-07
**------------------------------------------------------------------------------------------
*******************************************************************************************/

FSM_PKT* rlc_resegment(struct AmBuffer *tbuff, u32 sizeFromMac)
{
	if(tbuff->segStart[0] != -1)
	{
		struct RLC_AMPDU_fixhead *fixhead;
		struct RLC_AMPDU_seg_exthead *exthead;
		exthead= (RLC_AMPDU_seg_exthead *)fsm_mem_alloc(sizeof(RLC_AMPDU_seg_exthead));
		int start = tbuff->segStart[0];
		int end = tbuff->segEnd[0];
		int headLen = tbuff->headLen + 2;
		int dataLen = end - start + 1;
		int retxSize = headLen + dataLen;
		unsigned char *tail;
		int count = tbuff->segnum;
		FSM_PKT *skb;
		int i,len;
		if(retxSize <= sizeFromMac)//retransmite pdu segment and need not resegment
		{
			exthead->SO = start;
			if(tbuff->pkt->data + headLen  + end == tbuff->pkt->tail - 1)
			{
				exthead->LSF = 1;
			}
			else
			{
				exthead->LSF = 0;
			}
			//*(u16 *)exthead = htons(*(u16 *)exthead);
			fixhead = ( RLC_AMPDU_fixhead *)tbuff->pkt->data;
			//*(u16 *)fixhead = ntohs(*(u16 *)fixhead);
			fixhead->RF=1;
			//*(u16 *)fixhead = htons(*(u16 *)fixhead);
			skb = fsm_pkt_create(retxSize);
			fsm_skb_reserve(skb,headLen);
			tail = fsm_skb_put(skb,dataLen);
			fsm_mem_cpy(skb->data,(tbuff->pkt->data + tbuff->headLen  + start),dataLen); 
			fsm_mem_cpy(fsm_skb_push(skb,headLen - 4),tbuff->pkt->data + 2,headLen - 4); //kuo zhan tou fu zhi
			fsm_mem_cpy(fsm_skb_push(skb,2),exthead,2);
			fsm_mem_cpy(fsm_skb_push(skb,2),fixhead,2);

			if(count == 1)
			{
				tbuff->segStart[0]=-1;
				tbuff->segEnd[0]=-1;
			}
			else if(count > 1)
			{				
				for(i=0; i < count-1; i++)
				{
					tbuff->segStart[i] = tbuff->segStart[i+1];
					tbuff->segEnd[i] = tbuff->segEnd[i+1];
				}
				tbuff->segStart[count-1] = -1;
				tbuff->segEnd[count-1] = -1;
				count--;
			}
		}
		else                    //need resegment
		{
			exthead->SO = start;
			exthead->LSF = 0;
			//*(u16 *)exthead = htons(*(u16 *)exthead);
			fixhead = ( RLC_AMPDU_fixhead *)tbuff->pkt->data;
			//*(u16 *)fixhead = ntohs(*(u16 *)fixhead);
			fixhead->RF=1;
			//*(u16 *)fixhead = htons(*(u16 *)fixhead);
			skb = fsm_pkt_create(sizeFromMac);
			fsm_skb_reserve(skb,headLen);
			len = sizeFromMac - headLen;
			tail = fsm_skb_put(skb,len);
			fsm_mem_cpy(skb->data,tbuff->pkt->data + tbuff->headLen  + start,len); 
			fsm_mem_cpy(fsm_skb_push(skb,headLen - 4),tbuff->pkt->data + 2,headLen - 4);
			fsm_mem_cpy(fsm_skb_push(skb,2),exthead,2);
			fsm_mem_cpy(fsm_skb_push(skb,2),fixhead,2);
			tbuff->segStart[0] += len;
		}
		if(exthead != NULL)
		{
			fsm_mem_free(exthead);
			exthead=NULL;
		}
		return skb;
	}
	else
	{
		fsm_printf("[RLC][rlc_resegment] segment transfer has been completed");
	}
}


/*******************************************************************************************
** Function name: rlc_AM_reassemble_deliver()
** Descriptions	:PDU reassemble
** Input:
**     the struct of AM_Instancer pointer and pduLeft,URLC_IciMsg
** Output:
**	return	:SDU,transmit SDU to PDCP one by one
**free: PDU and related AmBuffer
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:
** Modified Date:
**------------------------------------------------------------------------------------------
*******************************************************************************************/

void rlc_AM_reassemble_deliver(struct AM_Instance *amIns, struct pduLeft *pdu_left)
{

	FSM_PKT *skb;
	u16 length,count,i;
	struct RLC_AMPDU_fixhead *pduhdr;
	struct AmBuffer *tbuff;
	MRLC_fromMac_IciMsg *mac_ici;
	URLC_IciMsg *icimsg;
	
	SV_PTR_GET(rlc_mac_sv);
	fsm_printf("[RLC][rlc_AM_reassemble_deliver] vr_r=%d, vr_h=%d\n",amIns->vr_r,amIns->vr_h);
	u16 h=(amIns->vr_h + 1)%1024;
	u16 r=amIns->vr_r;
	for(i=h; (i-h+1024)%1024 < (r-h+1024)%1024; i=(i+1)%1024)
	//for(i=0; i<=AM_WINDOW_SIZE; i++)
	{
		if(amIns->amRecvWindow[i] != NULL)
		{
			tbuff = amIns->amRecvWindow[i];
			mac_ici = (MRLC_fromMac_IciMsg *)tbuff->pkt->head;
			icimsg = (URLC_IciMsg *)fsm_mem_alloc(sizeof(URLC_IciMsg));
			icimsg->pbCh = mac_ici->pbCh;
			icimsg->rbId = mac_ici->lcid;
			pduhdr=(RLC_AMPDU_fixhead*)tbuff->pkt->data;
			//*(u16*)fixhead=ntohs(*(u16*)fixhead); 
			fsm_skb_pull(tbuff->pkt, 2) ;
    		fsm_printf("[RLC][rlc_AM_reassemble_deliver] pduhdr->E=%d, pduhdr->FIfirst=%d, pduhdr->FIsecond=%d\n",pduhdr->E, pduhdr->FIfirst,pduhdr->FIsecond);
			//fsm_octets_print(tbuff->pkt->data,64);
			if((pduhdr->E==0)&&(pduhdr->FIfirst==1))
			{
				if(pdu_left->SN_Left != -1 && (pdu_left->SN_Left+1)%1024==pduhdr->SN)
				{
					FSM_PKT *new_skb;
					length = pdu_left->sduLeft->len + tbuff->pkt->len;
					new_skb = fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,tbuff->pkt->len),tbuff->pkt->data,tbuff->pkt->len);
					fsm_mem_cpy(fsm_skb_push(new_skb,pdu_left->sduLeft->len),pdu_left->sduLeft->data,pdu_left->sduLeft->len);
					clear_left_pdu(pdu_left);
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;							
					}
					if(pduhdr->FIsecond==0)
					{
						skb = new_skb;
						if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
						{
							skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
						}
						fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
						fsm_pkt_send(skb,STRM_TO_PDCP);
						//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
						SV(countSendToUpper)++;
						fsm_printf("[RLC][rlc_AM_reassemble_deliver] <--send to PDCP1\n");
						//fsm_printf("[RLC][rlc_AM_reassemble_deliver] data len = %d\n", skb->len);
						//fsm_octets_print(skb->data,skb->len);
						//list_del(&tbuff->list);//20150123
					}
					else
					{
						pdu_left->SN_Left = pduhdr->SN;
						pdu_left->sduLeft = new_skb;
						fsm_printf("[RLC][rlc_AM_reassemble_deliver] not a complete SDU\n");
						//fsm_octets_print(new_skb->data,new_skb->len);
					}
				}
				else
				{
					SV(countDropPacket)++;
					clear_left_pdu(pdu_left);
                    
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
					}		
				}
			}
			else if((pduhdr->E==0)&&(pduhdr->FIfirst==0))
			{
				clear_left_pdu(pdu_left);
				if(pduhdr->FIsecond==0)
				{
					skb = tbuff->pkt;
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
						skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					//list_del(&tbuff->list);
					//fsm_mem_free(tbuff);
					//tbuff=NULL;
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_AM_reassemble_deliver] <--send to PDCP2\n");
					//fsm_printf("[RLC][rlc_AM_reassemble_deliver] data len = %d\n", skb->len);
					//fsm_octets_print(skb->data,skb->len);
				}
				else
				{
					pdu_left->SN_Left = pduhdr->SN;
					pdu_left->sduLeft = tbuff->pkt;
					/*list_del(&tbuff->list);
					fsm_mem_free(tbuff);
					tbuff=NULL;*/
					fsm_printf("[RLC][rlc_AM_reassemble_deliver] not a complete SDU\n");
					//fsm_octets_print(tbuff->pkt->data,64);
				}
			}
			else if(pduhdr->E==1)
			{
				struct _pduhdrExt PduhdrExt,*pPduhdrExt;
				pPduhdrExt = (struct _pduhdrExt *)fsm_mem_alloc(sizeof(struct _pduhdrExt));
				//pPduhdrExt = &PduhdrExt;
				count = get_AmPduhdExt(1,tbuff,0,pPduhdrExt); 
               
				if(pduhdr->FIfirst==1)
				{
					if(pdu_left->SN_Left != -1 && (pdu_left->SN_Left+1)%1024==pduhdr->SN)
					{
						FSM_PKT *new_skb;
						length = pdu_left->sduLeft->len + pPduhdrExt->LI[0];
						new_skb = fsm_pkt_create(length);

						fsm_skb_reserve(new_skb,length);
						fsm_mem_cpy(fsm_skb_push(new_skb,pPduhdrExt->LI[0]),tbuff->pkt->data,pPduhdrExt->LI[0]);
						fsm_skb_pull(tbuff->pkt,pPduhdrExt->LI[0] ) ;
						fsm_mem_cpy(fsm_skb_push(new_skb,pdu_left->sduLeft->len),pdu_left->sduLeft->data,pdu_left->sduLeft->len);
						skb = new_skb;
						if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
						{
							skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
						}
						fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
						fsm_pkt_send(skb,STRM_TO_PDCP);
						//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
						SV(countSendToUpper)++;
						fsm_printf("[RLC][rlc_AM_reassemble_deliver] <--send to PDCP3\n");
						//fsm_octets_print(skb->data,64);
						clear_left_pdu(pdu_left);
                       
					}
					else
					{
						SV(countDropPacket)++;
						clear_left_pdu(pdu_left);
						fsm_skb_pull(tbuff->pkt,pPduhdrExt->LI[0] ) ;

					}
				}
				else
				{
					clear_left_pdu(pdu_left);
					length =pPduhdrExt->LI[0];
					FSM_PKT *new_skb =fsm_pkt_create(length);

					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length ) ;


					skb = new_skb;
					//fsm_printf(5,"%s\n",skb->data);
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
						skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_AM_reassemble_deliver] <--send to PDCP4\n");
                    //fsm_octets_print(skb->data,64);

				}
               
				int j;
				for(j=1;  j<count;j++)
				{
					length = pPduhdrExt->LI[j];
					FSM_PKT *new_skb = fsm_pkt_create(length);

					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length ) ;
					skb = new_skb;
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
						skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_AM_reassemble_deliver] <--send to PDCP5\n");
					//fsm_octets_print(skb->data,64);
                }
                
				if(pduhdr->FIsecond==0)
				{
					length = tbuff->pkt->len;
					FSM_PKT *new_skb = fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length) ;
					skb = new_skb;
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
						skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_AM_reassemble_deliver] <--send to PDCP6\n");
					//fsm_octets_print(skb->data,64);

					//list_del(&tbuff->list);
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
					}
					/*if(tbuff != NULL)
					{
						fsm_mem_free(tbuff);
						tbuff = NULL;
					}	*/			
				}
				else
				{
					length = tbuff->pkt->len;
					FSM_PKT *new_skb = fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,tbuff->pkt->len) ;

					pdu_left->SN_Left = pduhdr->SN;
					pdu_left->sduLeft = new_skb;
                    //list_del(&tbuff->list);
                    if(tbuff->pkt != NULL)
                    {
                    	fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
                    }
					/*if(tbuff != NULL)
					{
						fsm_mem_free(tbuff);
						tbuff = NULL;
					}*/
				}
				if(pPduhdrExt != NULL)
				{
					fsm_mem_free(pPduhdrExt);
					pPduhdrExt = NULL;
				}
			}
			if(tbuff != NULL)
				{
					list_del(&tbuff->list);
					fsm_mem_free(tbuff);
					tbuff = NULL;
				}
			if(icimsg != NULL)
			{
				fsm_mem_free(icimsg);
				icimsg=NULL;
			}
			amIns->amRecvWindow[i]=NULL;
		}//end if
	}//end for
	FOUT;
}



/*******************************************************************************************
** Function name: rlc_UM_reassemble_deliver()
** Descriptions	:PDU reassemble
** Input:
**     the struct of UM_RX_Instance pointer and pduLeft,URLC_IciMsg
** Output:
**	return	:SDU,transmit SDU to PDCP one by one
**free: PDU and related UmBuffer
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:
** Modified Date:
**------------------------------------------------------------------------------------------
*******************************************************************************************/

void rlc_UM_reassemble_deliver(struct UM_RX_Instance *umIns, struct pduLeft *pdu_left)
{
    fsm_printf("[RLC] Enter rlc_UM_reassemble_deliver\n ");
	SV_PTR_GET(rlc_mac_sv);
	FSM_PKT *skb;
	u16 length,count,i;
	struct RLC_UM_size10_subhead *pduhdr;
	struct UmBuffer *tbuff;
	MRLC_fromMac_IciMsg *mac_ici;
	URLC_IciMsg *icimsg;
	u16 h=(umIns->vr_uh + 1)%1024;
	u16 r=umIns->vr_ur;
	for(i=h; (i-h+1024)%1024 < (r-h+1024)%1024; i=(i+1)%1024)
	{
		if(umIns->umRecvWindow10[i] != NULL)
		{
			fsm_printf("[RLC]  umIns->umRecvWindow10[i] != NULL, i = %d\n",i);
			tbuff = umIns->umRecvWindow10[i];
			mac_ici = (MRLC_fromMac_IciMsg *)tbuff->pkt->head;
			icimsg = (URLC_IciMsg *)fsm_mem_alloc(sizeof(URLC_IciMsg));
			icimsg->pbCh = mac_ici->pbCh;
			icimsg->rbId = mac_ici->lcid;
			//fsm_octets_print(tbuff->pkt->data,64);
			pduhdr=(RLC_UM_size10_subhead*)tbuff->pkt->data;
			//*(u16*)fixhead=ntohs(*(u16*)fixhead); 
			fsm_skb_pull(tbuff->pkt, 2) ;
			//get_UmPduhead(umIns->umRecvWindow10[i],pduhdr) ;
			fsm_octets_print(pduhdr,2);
          	fsm_printf("[RLC][rlc_UM_reassemble_deliver] pduhdr->E=%d, pduhdr->FIfirst=%d, pduhdr->FIsecond=%d\n",pduhdr->E, pduhdr->FIfirst,pduhdr->FIsecond);
			
			if((pduhdr->E==0)&&(pduhdr->FIfirst==1))
			{
				if(pdu_left->SN_Left != -1 && (pdu_left->SN_Left+1)%1024==pduhdr->SN)
				{
					FSM_PKT *new_skb;
					length = pdu_left->sduLeft->len + tbuff->pkt->len;
					new_skb = fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,tbuff->pkt->len),tbuff->pkt->data,tbuff->pkt->len);
					fsm_mem_cpy(fsm_skb_push(new_skb,pdu_left->sduLeft->len),pdu_left->sduLeft->data,pdu_left->sduLeft->len);
					clear_left_pdu(pdu_left);
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
					}
					if(pduhdr->FIsecond==0)
					{
						skb = new_skb;
						if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
						{
							skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
						}
						fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
						fsm_pkt_send(skb,STRM_TO_PDCP);
						//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
						SV(countSendToUpper)++;
						fsm_printf("[RLC][rlc_UM_reassemble_deliver] <--send to PDCP1\n");
					}
					else
					{
						pdu_left->SN_Left = pduhdr->SN;
						pdu_left->sduLeft = new_skb;
						fsm_printf("[RLC] Not a complet SDU\n");
					}
				}
				else
				{
					SV(countDropPacket)++;
					clear_left_pdu(pdu_left);
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
					}
				}
			}
			else if((pduhdr->E==0)&&(pduhdr->FIfirst==0))
			{

				clear_left_pdu(pdu_left);

				if(pduhdr->FIsecond==0)
				{
					skb = tbuff->pkt;
	
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
						skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_UM_reassemble_deliver] <--send to PDCP2\n");
				}

				else
				{
					pdu_left->SN_Left = pduhdr->SN;
					pdu_left->sduLeft=tbuff->pkt;
					fsm_printf("[RLC] Not a complet SDU\n");
				}
			}
			else if(pduhdr->E==1)
			{
				struct _pduhdrExt *pPduhdrExt;
				pPduhdrExt = (struct _pduhdrExt *)fsm_mem_alloc(sizeof(struct _pduhdrExt));
				count = get_UmPduhdExt(1,tbuff,0,pPduhdrExt); 
                
				if(pduhdr->FIfirst==1)
				{
					if(pdu_left->SN_Left != -1&&(pdu_left->SN_Left+1)%1024==pduhdr->SN)
					{
						FSM_PKT *new_skb;
						length = pdu_left->sduLeft->len + pPduhdrExt->LI[0];
						new_skb = fsm_pkt_create(length);
						fsm_skb_reserve(new_skb,length);
						fsm_mem_cpy(fsm_skb_push(new_skb,pPduhdrExt->LI[0]),tbuff->pkt->data,pPduhdrExt->LI[0]);
						fsm_skb_pull(tbuff->pkt,pPduhdrExt->LI[0] ) ;
						fsm_mem_cpy(fsm_skb_push(new_skb,pdu_left->sduLeft->len),pdu_left->sduLeft->data,pdu_left->sduLeft->len);

						skb = new_skb;
						
						if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
						{
							skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
						}
						fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
						fsm_pkt_send(skb,STRM_TO_PDCP);
						//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
						SV(countSendToUpper)++;
						fsm_printf("[RLC][rlc_UM_reassemble_deliver] <--send to PDCP3\n");
						
						clear_left_pdu(pdu_left);
                       
					}
					else
					{
						SV(countDropPacket)++;
						clear_left_pdu(pdu_left);
						fsm_skb_pull(tbuff->pkt,pPduhdrExt->LI[0] ) ;
					}
				}
				else
				{
					clear_left_pdu(pdu_left);

					length =pPduhdrExt->LI[0];
					FSM_PKT *new_skb =fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length ) ;
					skb = new_skb;
					
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
						skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_UM_reassemble_deliver] <--send to PDCP4\n");
				}
               
				int j;
				for(j=1; j<count;j++)
				{
					length = pPduhdrExt->LI[j];
					FSM_PKT *new_skb = fsm_pkt_create(length);

					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length ) ;
					skb = new_skb;
					
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
			    		skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_UM_reassemble_deliver] <--send to PDCP5\n");
				}
                
				if(pduhdr->FIsecond==0)
				{
					length = tbuff->pkt->len;
					FSM_PKT *new_skb = fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length) ;
					skb = new_skb ;
					
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
			    		skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_UM_reassemble_deliver] <--send to PDCP6\n");				
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
					}
					
				}
				else
				{
					length = tbuff->pkt->len;
					FSM_PKT *new_skb = fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,tbuff->pkt->len) ;
                   
					pdu_left->SN_Left = pduhdr->SN;
					pdu_left->sduLeft = new_skb;
					fsm_printf("[RLC]Not a complet SDU\n");
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
					}
				}
				if(pPduhdrExt != NULL)
				{
					fsm_mem_free(pPduhdrExt);
					pPduhdrExt = NULL;
				}
			}
			if(tbuff != NULL)
			{
				list_del(&tbuff->list);
				fsm_mem_free(tbuff);
				tbuff = NULL;
			}
			if(icimsg != NULL)
			{
				fsm_mem_free(icimsg);
				icimsg=NULL;
			}
			umIns->umRecvWindow10[i]=NULL;
			
		}//end if
	}//end for
}

/*******************************************************************************************
** Function name: rlc_UM5_reassemble_deliver()
** Descriptions	:PDU reassemble
** Input:
**     the struct of UM_RX_Instance pointer and pduLeft,URLC_IciMsg
** Output:
**	return	:SDU,transmit SDU to PDCP one by one
**free: PDU and related UmBuffer
** Created by	:li xu long
** Created Date	: 20014-09-29
**------------------------------------------------------------------------------------------
** Modified by	:
** Modified Date:
**------------------------------------------------------------------------------------------
*******************************************************************************************/

void rlc_UM5_reassemble_deliver(struct UM_RX_Instance *umIns, struct pduLeft *pdu_left)
{
	SV_PTR_GET(rlc_mac_sv);
	FSM_PKT *skb;
	u16 length,count;
	struct RLC_UM_size5_head *pduhdr;
	struct UmBuffer *tbuff;
	MRLC_fromMac_IciMsg *mac_ici;
	URLC_IciMsg *icimsg;
	int i;
	u16 h=(umIns->vr_uh + 1)%32;
	u16 r=umIns->vr_ur;
	for(i=h; (i-h+32)%32 < (r-h+32)%32; i=(i+1)%32)
	{
		if(umIns->umRecvWindow5[i] != NULL)
		{

			tbuff = umIns->umRecvWindow5[i];
			mac_ici = (MRLC_fromMac_IciMsg *)tbuff->pkt->head;
			icimsg = (URLC_IciMsg *)fsm_mem_alloc(sizeof(URLC_IciMsg));
			icimsg->pbCh = mac_ici->pbCh;
			icimsg->rbId = mac_ici->lcid;
			pduhdr=(RLC_UM_size5_head*)tbuff->pkt->data;
			//*(u16*)fixhead=ntohs(*(u16*)fixhead); 
			fsm_skb_pull(tbuff->pkt, 2) ;
          
			if((pduhdr->E==0)&&(pduhdr->FIfirst==1))
			{
				if(pdu_left->SN5_Left != -1 && (pdu_left->SN5_Left+1)%32==pduhdr->SN)
				{
					FSM_PKT *new_skb;
					length = pdu_left->sduLeft->len + tbuff->pkt->len;
					new_skb = fsm_pkt_create(length);

					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,tbuff->pkt->len),tbuff->pkt->data,tbuff->pkt->len);
					fsm_mem_cpy(fsm_skb_push(new_skb,pdu_left->sduLeft->len),pdu_left->sduLeft->data,pdu_left->sduLeft->len);
					clear_left_pdu(pdu_left);
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
					}
					if(pduhdr->FIsecond==0)
					{
						skb = new_skb;
						//fsm_printf(5,"%s\n",skb->data);
						fsm_pkt_send(skb,STRM_TO_PDCP);
						if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
						{
							skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
						}
						fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
						fsm_pkt_send(skb,STRM_TO_PDCP);
						//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
						SV(countSendToUpper)++;
						fsm_printf("[RLC][rlc_UM5_reassemble_deliver] <--send to PDCP\n"); 
					}
					else
					{
						pdu_left->SN5_Left = pduhdr->SN;
						pdu_left->sduLeft = new_skb;
					}
				}
				else
				{
					SV(countDropPacket)++;
					clear_left_pdu(pdu_left);
					if(tbuff->pkt != NULL)
					{
						fsm_pkt_destroy(tbuff->pkt);
						tbuff->pkt = NULL;
					}
				}
			}
			else if((pduhdr->E==0)&&(pduhdr->FIfirst==0))
			{

				clear_left_pdu(pdu_left);

				if(pduhdr->FIsecond==0)
				{

					skb = tbuff->pkt;
					//fsm_printf(5,"%s\n",skb->data);
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
						skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_UM5_reassemble_deliver] <--send to PDCP\n");
					list_del(&tbuff->list);
                    fsm_mem_free(tbuff);
					tbuff=NULL;
				}

				else
				{
					pdu_left->SN5_Left = pduhdr->SN;
					pdu_left->sduLeft=tbuff->pkt;
					list_del(&tbuff->list);
					fsm_mem_free(tbuff);
					tbuff=NULL;
				}
			}
			else if(pduhdr->E==1)
			{
				struct _pduhdrExt *pPduhdrExt;
				pPduhdrExt = (struct _pduhdrExt *)fsm_mem_alloc(sizeof(struct _pduhdrExt));
				count = get_UmPduhdExt(1,tbuff,0,pPduhdrExt);  
                
				if(pduhdr->FIfirst==1)
				{
					if(pdu_left->SN5_Left != -1 && (pdu_left->SN5_Left+1)%512==pduhdr->SN)
					{
						FSM_PKT *new_skb;
						length = pdu_left->sduLeft->len + pPduhdrExt->LI[0];
						new_skb = fsm_pkt_create(length);

						fsm_skb_reserve(new_skb,length);
						fsm_mem_cpy(fsm_skb_push(new_skb,pPduhdrExt->LI[0]),tbuff->pkt->data,pPduhdrExt->LI[0]);
						fsm_skb_pull(tbuff->pkt,pPduhdrExt->LI[0] ) ;
						fsm_mem_cpy(fsm_skb_push(new_skb,pdu_left->sduLeft->len),pdu_left->sduLeft->data,pdu_left->sduLeft->len);

						skb = new_skb;
						//fsm_printf(5,"%s\n",skb->data);
						fsm_pkt_send(skb,STRM_TO_PDCP);
						if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
						{
							skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
						}
						fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
						fsm_pkt_send(skb,STRM_TO_PDCP);
						//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
						SV(countSendToUpper)++;
						fsm_printf("[RLC][rlc_UM5_reassemble_deliver] <--send to PDCP\n");
                      
						clear_left_pdu(pdu_left);
                        
					}
					else
					{
						SV(countDropPacket)++;
						clear_left_pdu(pdu_left);
						fsm_skb_pull(tbuff->pkt,pPduhdrExt->LI[0] ) ;
					}
				}
				else
				{
					clear_left_pdu(pdu_left);

					length =pPduhdrExt->LI[0];
					FSM_PKT *new_skb =fsm_pkt_create(length);

					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length ) ;
					skb = new_skb;
					//fsm_printf(5,"%s\n",skb->data);
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
			    		skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_UM5_reassemble_deliver] <--send to PDCP\n");
				}
                
				int j;
				for(j=1; j<count;j++)
				{
					length = pPduhdrExt->LI[j];
					FSM_PKT *new_skb = fsm_pkt_create(length);

					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length ) ;
					skb = new_skb;
					//fsm_printf(5,"%s\n",skb->data);
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
					{
			    		skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
					}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_UM5_reassemble_deliver] <--send to PDCP\n");
                    
				}
               
				if(pduhdr->FIsecond==0)
				{
					length = tbuff->pkt->len;
					FSM_PKT *new_skb = fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,length) ;
					skb = new_skb ;
					//fsm_printf(5,"%s\n",skb->data);
					if( fsm_skb_headroom(skb) < sizeof(URLC_IciMsg) )
			    	{
			    		skb = fsm_skb_realloc_headeroom(skb,sizeof(URLC_IciMsg));
			    	}
					fsm_mem_cpy(skb->head,icimsg,sizeof(URLC_IciMsg));
					fsm_pkt_send(skb,STRM_TO_PDCP);
					//fsm_pkt_send(skb, STRM_TO_IPADP);	//for test 20150828
					SV(countSendToUpper)++;
					fsm_printf("[RLC][rlc_UM5_reassemble_deliver] <--send to PDCP\n");                  

					list_del(&tbuff->list);
					fsm_pkt_destroy(tbuff->pkt);
					tbuff->pkt = NULL;
					fsm_mem_free(tbuff);
					tbuff = NULL;
				}
				else
				{
					length = tbuff->pkt->len;
					FSM_PKT *new_skb = fsm_pkt_create(length);
					fsm_skb_reserve(new_skb,length);
					fsm_mem_cpy(fsm_skb_push(new_skb,length),tbuff->pkt->data,length);
					fsm_skb_pull(tbuff->pkt,tbuff->pkt->len) ;
                    
					pdu_left->SN5_Left = pduhdr->SN;
					pdu_left->sduLeft = new_skb;
					list_del(&tbuff->list);
					fsm_pkt_destroy(tbuff->pkt);	
					tbuff->pkt = NULL;
					fsm_mem_free(tbuff);
					tbuff = NULL;
				}
				if(pPduhdrExt != NULL)
				{
					fsm_mem_free(pPduhdrExt);
					pPduhdrExt = NULL;
				}
			}
			if(tbuff != NULL)
			{
				list_del(&tbuff->list);
				fsm_mem_free(tbuff);
				tbuff = NULL;
			}
			if(icimsg != NULL)
			{
				fsm_mem_free(icimsg);
				icimsg=NULL;
			}
			umIns->umRecvWindow10[i]=NULL;
		}//end if
	}//end for
    
}

