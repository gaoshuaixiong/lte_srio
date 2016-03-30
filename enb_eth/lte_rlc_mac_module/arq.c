#include "arq.h"

/************************************************************************
**Function name: Assemble_ctrlPduPart2()
**Descriptions: Assemble the part2 of ctrlPdu before assembling the part3 of ctrlPdu
**Input: struct Buffer *ctrlBf, SequenceNumber sn,u32 *pduLen,u32 size
**Output: assemble the part2 of ctrlPdu
**Returns:
**Created by: ChenLang
**Created Date: 2015
**---------------------------------------------------------------------
**Modified by:
**Modified Date:
************************************************************************/

void Assemble_ctrlPduPart2 (struct Buffer *ctrlBf, SequenceNumber sn,u32 *pduLen,u32 size)
{
	ctrlPduPart2 *part2 = (ctrlPduPart2*)fsm_mem_alloc(sizeof(ctrlPduPart2));
	memset(part2,0,sizeof(ctrlPduPart2));
	part2->NACK_SN = sn;
	part2->E2 = 1;	
	if (*pduLen + 6 < size)
	{
		part2->E1 =1;
	}
	else if (*pduLen + 6 == size)
	{
		part2->E1 =0;
	}
	fsm_printf("[RLC][Assemble_ctrlPduPart2] pdulen = %d, size = %d, E1 = %d\n",*pduLen,size,part2->E1);
	*(u16 *)part2 = htons(*(u16 *)part2);
	fsm_mem_cpy(ctrlBf->pkt->tail,part2,2);
	fsm_skb_put(ctrlBf->pkt,2);
	*pduLen += 2;
	if ( part2 != NULL )
	{
		fsm_mem_free(part2);
		part2=NULL;
	}
}

/************************************************************************
**Function name: size_of_ctrl_pdu()
**Descriptions: Calculate the size of the ctrl pdu
**Input: struct AM_Instance *amIns
**Output:
**Returns: the size of the ctrl pdu
**Created by: ZhangYi
**Created Date: 2014/10/15
**---------------------------------------------------------------------
**Modified by:ChenLang
**Modified Date:2015/03
************************************************************************/
u32 size_of_ctrl_pdu(struct AM_Instance *amIns)
{
	u32 size = 0;
	int i;
	//¼ÆËã×´Ì¬PDU´óÐ¡
	size = 2;  //Ç°¼¸¸ö¹Ì¶¨Óò´óÐ¡
	if( amIns->vr_r < amIns->vr_h -1 )  //µ±Ç°½ÓÊÕ¶ÓÁÐÖÐµÄPDUÎ´È«²¿³É¹¦½ÓÊÕ
	{
		for( i = amIns->vr_r; i < amIns->vr_ms; i++)
		{
			//sn = i;
			if( amIns->amRecvWindow[i] == NULL )
			{
				size += 2;//Ö±½Ó¼Ó2×Ö½Ú
			}
			else //±íÊ¾¸ÃPDUÒÑ¾­ÊÕµ½²¿·Ö·Ö¶Î
			{
				if( amIns->amRecvWindow[i]->pktstatus != FULL_SDU)
				{
					if( amIns->amRecvWindow[i]->segStart[0] > 0 )
					{
						size += 6;//°üº¬soendºÍsostart¹²6×Ö½Ú
					}
					if( amIns->amRecvWindow[i]->pktstatus ==  LAST_SEGMENT)
					{
						size += (amIns->amRecvWindow[i]->segnum - 1)*6;
					}
					else if( amIns->amRecvWindow[i]->pktstatus ==  ANY_SEGMENT)
					{
						size += amIns->amRecvWindow[i]->segnum*6;
					}
				}
			}
		}
	}
	fsm_printf("[RLC][size_of_ctrl_pdu] size of ctrl pdu is %d\n",size);
	return size;
}

/************************************************************************
**Function name: rlc_ctrl_pkt_trans()
**Descriptions: Assemble ctrl PDU and add to the ctrl buffer
**Input: struct AM_Instance *amIns
**Output: Assemble ctrl PDU and add to the ctrl buffer
**Returns:
**Created by: ChenLang
**Created Date: 2015
**---------------------------------------------------------------------
**Modified by:
**Modified Date:
************************************************************************/


void rlc_ctrl_pkt_trans(struct AM_Instance *amIns)
{
	struct Buffer *ctrlBf;
	SequenceNumber sn;  //¼ÇÂ¼½ÓÊÕÊý×éÖÐ¿ÕÖ¸ÕëËù¶ÔÓ¦µÄSN
	int i;
	int j;
	u16 soStart;
	u16 soEnd;
	u32 size;  //¼ÇÂ¼Òª×é×°µÄ×´Ì¬PDU´óÐ¡
	u32 pduLen;
	ctrlPduPart1 *part1;
	ctrlPduPart2 *part2;
	ctrlPduPart3 *part3; // ¿ØÖÆpdu»®·ÖÎª3¸ö²¿·Ö
	FIN(rlc_ctrl_pkt_trans());
	fsm_printf("[RLC][rlc_ctrl_pkt_trans] entering rlc_ctrl_pkt_trans()\n");
	pduLen = 0;
	size = 0;
	size = size_of_ctrl_pdu(amIns);
	fsm_printf("[RLC][rlc_ctrl_pkt_trans] size = %d\n",size);
	if( size < 2)  //¿ØÖÆPDUÐ¡ÓÚ2£¬¶ªÆú²¢Ö±½Ó·µ»Ø
	{
		fsm_printf("[RLC][rlc_ctrl_pkt_trans] discard the PDU\n");
		FOUT;
	}
	ctrlBf = (struct Buffer *)fsm_mem_alloc(sizeof(struct Buffer));
	memset(ctrlBf,0,sizeof(struct Buffer));
	ctrlBf->pkt = fsm_pkt_create(size);
	part1 = (ctrlPduPart1 *)fsm_mem_alloc(sizeof(ctrlPduPart1));
	memset(part1,0,sizeof(ctrlPduPart1));
	part1->D_C = 0;
	part1->CPT = 0;
	part1->ACK_SN = amIns->vr_ms;	
	fsm_printf("[RLC][rlc_ctrl_pkt_trans] vr_ms = %d\n",amIns->vr_ms);
	fsm_octets_print(part1,2);
	if( size == 2 )  //µ±Ç°½ÓÊÕ¶ÓÁÐÖÐµÄPDU¶¼ÒÑ³É¹¦½ÓÊÕ
	{
		part1->E1 = 0;
		*(u16 *)part1 = htons(*(u16 *)part1);
		fsm_mem_cpy(ctrlBf->pkt->tail,part1,2);
		fsm_skb_put(ctrlBf->pkt,2);
		pduLen += 2;
	}
	else  //ËµÃ÷ÓÐNACK
	{
		fsm_printf("[RLC][rlc_ctrl_pkt_trans] there is nack\n");
		part1->E1 = 1;
		*(u16 *)part1 = htons(*(u16 *)part1);
		fsm_mem_cpy(ctrlBf->pkt->tail,part1,2);
		fsm_skb_put(ctrlBf->pkt,2);
		pduLen += 2;
		if( part1 != NULL )
		{
			fsm_mem_free(part1);
			part1=NULL;
		}
		fsm_printf("[RLC][rlc_ctrl_pkt_trans] vr_r = %d, vr_ms = %d, pdulen = %d\n",amIns->vr_r,amIns->vr_ms,pduLen);
		for( i = amIns->vr_r; i < amIns->vr_ms && pduLen < size; i++ )
		{
			fsm_printf("[RLC][rlc_ctrl_pkt_trans] i = %d\n",i);
			sn = i;
			if(amIns->amRecvWindow[i] == NULL)  //±íÊ¾Õû¸öPDU¶¼Ã»ÊÕµ½
			{
				fsm_printf("[RLC][rlc_ctrl_pkt_trans] amIns->amRecvWindow[i] == NULL\n");
				if( pduLen+2<=size )
				{
					part2= (ctrlPduPart2*)fsm_mem_alloc(sizeof(ctrlPduPart2));
					memset(part2,0,sizeof(ctrlPduPart2));
					part2->NACK_SN = sn;
					if( pduLen+2==size )
					{
						part2->E1=0; //ÌîE1Îª1
					}
					else
					{
						part2->E1=1;
					}
					part2->E2=0;
					*(u16 *)part2 = htons(*(u16 *)part2);
					fsm_mem_cpy(ctrlBf->pkt->tail,part2,2);
					fsm_skb_put(ctrlBf->pkt,2);
					pduLen += 2;
					if( part2 != NULL )
					{
						fsm_mem_free(part2);
						part2=NULL;
					}
				}
			}
			else
			{
				fsm_printf("[RLC][rlc_ctrl_pkt_trans] amIns->amRecvWindow[i] != NULL\n");
				if( amIns->amRecvWindow[i]->pktstatus != FULL_SDU)  //±íÊ¾²¿·Ö·Ö¶ÎÃ»ÓÐÊÕµ½
				{				
					if( amIns->amRecvWindow[i]->segStart[0] > 0 )
					{					
						Assemble_ctrlPduPart2(ctrlBf,sn,&pduLen,size);//ÏÈ×é×°µÚ¶þ²¿·Ö£¬ÔÙ×é×°µÚÈý²¿·Ö
						part3= (ctrlPduPart3*)fsm_mem_alloc(sizeof(ctrlPduPart3));
						memset(part3,0,sizeof(ctrlPduPart3));
						part3->SOstart = 0;
						part3->SOend = amIns->amRecvWindow[i]->segStart[0];
						*(u32 *)part3 = htons(*(u32 *)part3);
						fsm_mem_cpy(ctrlBf->pkt->tail,part3,4);
						fsm_skb_put(ctrlBf->pkt,4);
						pduLen += 4;
						if( part3 != NULL )
						{
							fsm_mem_free(part3);
							part3=NULL;
						}
					}				
					for( j = 0; j < amIns->amRecvWindow[i]->segnum - 1; j++ )
					{
						Assemble_ctrlPduPart2(ctrlBf,sn,&pduLen,size);
						part3= (ctrlPduPart3*)fsm_mem_alloc(sizeof(ctrlPduPart3));
						memset(part3,0,sizeof(ctrlPduPart3));
						part3->SOstart = amIns->amRecvWindow[i]->segEnd[j];
						part3->SOend = amIns->amRecvWindow[i]->segStart[j+1] ;
						*(u32 *)part3 = htons(*(u32 *)part3);
						fsm_mem_cpy(ctrlBf->pkt->tail,part3,4);
						fsm_skb_put(ctrlBf->pkt,4);
						pduLen += 4;
						if( part3 != NULL )
						{
							fsm_mem_free(part3);
							part3=NULL;
						}
					}
					if( amIns->amRecvWindow[i]->pktstatus == ANY_SEGMENT)  //±íÊ¾×îºóÒ»¸ö·Ö¶ÎÃ»ÓÐÊÕµ½
					{			
						Assemble_ctrlPduPart2(ctrlBf,sn,&pduLen,size);
						part3= (ctrlPduPart3*)fsm_mem_alloc(sizeof(ctrlPduPart3));
						memset(part3,0,sizeof(ctrlPduPart3));
						part3->SOstart = amIns->amRecvWindow[i]->segEnd[amIns->amRecvWindow[i]->segnum-1];
						part3->SOend = 0x7fff;
						*(u32 *)part3 = htons(*(u32 *)part3);
						fsm_mem_cpy(ctrlBf->pkt->tail,part3,4);
						fsm_skb_put(ctrlBf->pkt,4);
						pduLen += 4;
						if( part3 != NULL )
						{
							fsm_mem_free(part3);
							part3=NULL;
						}
					}
				}
			}
		}
	}
	amIns->ctrlPduBufferNum++;
	amIns->ctrlPduBufferSize += ctrlBf->pkt->len;
	fsm_printf("[RLC][rlc_ctrl_pkt_trans] ctrlPduBufferNum = %d, ctlPduSize = %d\n",amIns->ctrlPduBufferNum,amIns->ctrlPduBufferSize);
	list_add_tail(&ctrlBf->list,&amIns->ctrlPduBuffer.list);
	fsm_octets_print(ctrlBf->pkt->data,ctrlBf->pkt->len);
	amIns->statFlag = false;
	FOUT;
}


/**********************************************************************************************
**Function name: ack_handler()
**Descriptions: After reveiving a ctrl PDU, handle the corresponding PDUs according to the ackSn in the ctrl PDU
**Input: struct AM_Instance *amIns, SequenceNumber ackSn
**Output: Remove the PDUs whose sn < ackSn in the amTransmittedBuffer and amRetxBuffer
**Returns:
**Created by: ZhangYi
**Created Date: 2014
**----------------------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014/10/13
**********************************************************************************************/
void ack_handler(struct AM_Instance *amIns,SequenceNumber ackSn)   //20141013
{
	struct AmBuffer *amTxedBuffer;  //ÒÑ·¢ËÍ¶ÓÁÐ½Úµã
	//struct AmBuffer *amTxedBf;
	struct AmBuffer *amRetxBf;
	struct AmBuffer * nposBf;
	struct AmBuffer * reposBf;
	int i;
	while( (amIns->vt_a<ackSn||ackSn<amIns->vt_a-AM_WINDOW_SIZE) && (amIns->vt_a<amIns->vt_s||amIns->vt_s<amIns->vt_a-AM_WINDOW_SIZE))
	{
		if( amIns->vt_a == amIns->pollSn )
		{
			//Í£Ö¹²¢¸´Î»ÖØ´«¼ÆÊ±Æ÷
			if(amIns->pollRetxTimer)
			{
				fsm_schedule_cancel(amIns->pollRetxTimer);
				amIns->pollRetxTimer = 0;
				fsm_printf("[RLC][ack_handler] pollRetsTimer cancelled\n");
			}
		}

		//½«vt_a´ÓÒÑ·¢ËÍ¶ÓÁÐÉ¾³ý
		if( !list_empty(&amIns->amTransmittedBuffer.list) )
		{
			list_for_each_entry_safe(amTxedBuffer, nposBf, &amIns->amTransmittedBuffer.list,list)
			{
				if( amTxedBuffer->SN == amIns->vt_a )
				{
					amIns->amTransmittedBufferSize -= amTxedBuffer->pkt->len;
					amIns->amTransmittedBufferNum--;
					if(amTxedBuffer->pkt != NULL)
					{
						fsm_pkt_destroy( amTxedBuffer->pkt );
						amTxedBuffer->pkt = NULL;
					}
					list_del(&amTxedBuffer->list);
					if(amTxedBuffer != NULL)
					{
						fsm_mem_free(amTxedBuffer);
						amTxedBuffer = NULL;
					}
					//printk("[RLC][ack_handler] has remove pdu %d from TxedRuffer\n",amIns->vt_a);
					break;
				}
			}
			//nposBf = NULL;
		}

		//½«vt_a´ÓÖØ´«¶ÓÁÐÉ¾³ý
		if( !list_empty(&amIns->amRetxBuffer.list) )
		{
			list_for_each_entry_safe(amRetxBf, nposBf, &amIns->amRetxBuffer.list,list)
			{
				if( amRetxBf->SN == amIns->vt_a )
				{
					for( i = 0; i <amRetxBf->segnum; i++)
					{
						amIns->amRetxBufferSize -= amRetxBf->segEnd[i] - amRetxBf->segStart[i];
					}
					amIns->amRetxBufferSize -= amRetxBf->headLen;
					amIns->amRetxBufferNum--;
					if(amRetxBf->pkt != NULL)
					{
						fsm_pkt_destroy( amRetxBf->pkt );
						amRetxBf->pkt = NULL;
					}
					list_del(&amRetxBf->list);
					if(amRetxBf != NULL)
					{
						fsm_mem_free(amRetxBf);
						amRetxBf = NULL;
					}
					//printk("[RLC][ack_handler] has remove pdu %d from RetxRuffer\n",amIns->vt_a);
					break;
				}
			}
			//nposBf = NULL;
		}
		amIns->vt_a = (amIns->vt_a + 1)%(AM_WINDOW_SIZE*2);
	}
	amIns->vt_ms = (amIns->vt_a + AM_WINDOW_SIZE)%(AM_WINDOW_SIZE*2);
}

/**********************************************************************************************
**Function name:no_so_handler()
**Descriptions: Handle the corresponding PDU according to the NACK_SN field 
**Input: struct AM_Instance *amIns, ctrlPduPart2 *part2, bool *isPduFound
**Output: Retx or remove the PDU whose sn == part2->NACK_SN
**Returns:
**Created by: ChenLang
**Created Date: 2015
**----------------------------------------------------------------------------------
**Modified by:
**Modified Date:
**********************************************************************************************/

void no_so_handler(struct AM_Instance *amIns, ctrlPduPart2 *part2, bool *isPduFound)
{
	struct AmBuffer *amTxedBuffer;	//ÒÑ·¢ËÍ¶ÓÁÐ½Úµã
	struct AmBuffer *amRetxBf;
	struct AmBuffer * nposBf;
	struct AmBuffer * reposBf;
	CRLC_StatusInd_IoctrlMsg *mioctrl;
	fsm_printf("[RLC][rlc_ctrl_pkt_recv] run to no_so_handler\n");
	if( !list_empty(&amIns->amTransmittedBuffer.list) )
	{
		list_for_each_entry_safe(amTxedBuffer, nposBf, &amIns->amTransmittedBuffer.list,list)
		{
			if( part2->NACK_SN == amTxedBuffer->SN )
			{
				*isPduFound = true;
				amTxedBuffer->segStart[0] = 0;
				amTxedBuffer->segEnd[0] = amTxedBuffer->pkt->len;

				//¸üÐÂÖØ´«¼ÆÊýretxCount
				if( amTxedBuffer->retxCount == -1 )   //±íÊ¾Ê×´ÎÖØ´«
				{
					amTxedBuffer->retxCount = 0;
				}
				else
				{
					amTxedBuffer->retxCount++;
				}
				//Èç¹û´ïµ½×î´óÖØ´«´ÎÊýÔò±¨¸æ¸ø¸ß²
				fsm_printf("[RLC][no_so_handler]find the NACK PDU \n");
				if( amTxedBuffer->retxCount == amIns->maxRetxThreshold )
				{
					mioctrl = (CRLC_StatusInd_IoctrlMsg*)fsm_mem_alloc(sizeof(CRLC_StatusInd_IoctrlMsg));
					mioctrl->eventCode = 0;
					mioctrl->rbIdentity = amIns->rbId;
					//fsm_do_ioctrl(STRM_TO_RRC, CRLC_STATUS_IND, (void*)mioctrl, sizeof(CRLC_StatusInd_IoctrlMsg));
					fsm_mem_free(mioctrl);
					mioctrl = NULL;
					//´ÓÒÑ·¢ËÍ¶ÓÁÐÉ¾³ýPDU
					amIns->amTransmittedBufferSize -= amTxedBuffer->pkt->len;
					amIns->amTransmittedBufferNum--;
					fsm_pkt_destroy( amTxedBuffer->pkt );
					amTxedBuffer->pkt = NULL;
					list_del(&amTxedBuffer->list);
					fsm_mem_free(amTxedBuffer);
					amTxedBuffer = NULL;
					//´ÓÖØ´«¶ÓÁÐÉ¾³ýPDU
					if( !list_empty(&amIns->amRetxBuffer.list) )
					{
						list_for_each_entry_safe(amRetxBf, reposBf, &amIns->amRetxBuffer.list,list)
						{
							if( amRetxBf->SN == part2->NACK_SN )
							{
								amIns->amRetxBufferSize -= amRetxBf->pkt->len;
								amIns->amRetxBufferNum--;
								fsm_pkt_destroy( amRetxBf->pkt );
								amRetxBf->pkt = NULL;
								list_del(&amRetxBf->list);
								fsm_mem_free(amRetxBf);
								amRetxBf = NULL;
								break;
							}
						}
					}
				}
				else
				{
					fsm_printf("[RLC][rlc_ctrl_pkt_recv] run to rlc_retx()\n");
				}
				break;
			}
		}
	}
	FOUT;
}

/**********************************************************************************************
**Function name:so_handler()
**Descriptions: Handle the corresponding PDU segments according to the NACK_SN , SOstart and SOend 
**Input: struct AM_Instance *amIns, ctrlPduPart3 *part3, bool *isPduFound
**Output: Retx or remove the PDUs' segments whose sn == part3->NACK_SN
**Returns:
**Created by: ChenLang
**Created Date: 2015
**----------------------------------------------------------------------------------
**Modified by:
**Modified Date:
**********************************************************************************************/


void so_handler(struct AM_Instance *amIns, ctrlPduPart3 *part3, bool *isPduFound,SequenceNumber nackSn)
{
	struct AmBuffer *amTxedBuffer;	//ÒÑ·¢ËÍ¶ÓÁÐ½Úµã
	struct AmBuffer *amRetxBf;
	struct AmBuffer * nposBf;
	struct AmBuffer * reposBf;
	CRLC_StatusInd_IoctrlMsg *mioctrl;
	int i;

	if( !list_empty(&amIns->amTransmittedBuffer.list) )
	{
		list_for_each_entry_safe(amTxedBuffer, nposBf, &amIns->amTransmittedBuffer.list,list)
		{
			if( nackSn == amTxedBuffer->SN )
			{
				*isPduFound = true;
				fsm_printf("[RLC][so_handler] has found pdu %d from amTxedBuffer\n",nackSn);
				for( i = 0; i < amTxedBuffer->segnum; i++ )
				{
					if(amTxedBuffer->segStart[i]==part3->SOstart&&amTxedBuffer->segEnd[i]==part3->SOend)
					{
						fsm_printf("[RLC][so_handler] has found segum %d from amTxedBuffer\n",amTxedBuffer->segnum);
						break;
					}
				}
				if( i == amTxedBuffer->segnum )
				{
					amTxedBuffer->segStart[amTxedBuffer->segnum] = part3->SOstart;
					if(part3->SOend  == 0x7fff ) //±íÊ¾Îª×îºóÒ»¸ö·Ö¶Î
					{
						amTxedBuffer->segEnd[amTxedBuffer->segnum] = amTxedBuffer->pkt->len;
					}
					else
					{
						amTxedBuffer->segEnd[amTxedBuffer->segnum] = part3->SOend;
					}
					amTxedBuffer->segnum++;
					fsm_printf("[RLC][so_handler]it is the last segnum\n");
				}

				//¸üÐÂÖØ´«¼ÆÊýretxCount
				if( amTxedBuffer->retxCount == -1 )   //±íÊ¾Ê×´ÎÖØ´«
				{
					amTxedBuffer->retxCount = 0;
				}
				else
				{
					amTxedBuffer->retxCount++;
				}
				//Èç¹û´ïµ½×î´óÖØ´«´ÎÊýÔò±¨¸æ¸ø¸ß²ã
				if( amTxedBuffer->retxCount == amIns->maxRetxThreshold )
				{
					fsm_printf("[RLC][so_handler] the number of retx is already the max\n");
					mioctrl = (CRLC_StatusInd_IoctrlMsg*)fsm_mem_alloc(sizeof(CRLC_StatusInd_IoctrlMsg));
					mioctrl->eventCode = 0;
					mioctrl->rbIdentity = amIns->rbId;
					//fsm_do_ioctrl(STRM_TO_RRC, CRLC_STATUS_IND, (void*)mioctrl, sizeof(CRLC_StatusInd_IoctrlMsg));
					fsm_mem_free(mioctrl);
					mioctrl = NULL;
					//´ÓÒÑ·¢ËÍ¶ÓÁÐÉ¾³ýPDU
					amIns->amTransmittedBufferSize -= amTxedBuffer->pkt->len;
					amIns->amTransmittedBufferNum--;
					fsm_pkt_destroy( amTxedBuffer->pkt );
					amTxedBuffer->pkt = NULL;
					list_del(&amTxedBuffer->list);
					fsm_mem_free(amTxedBuffer);
					amTxedBuffer = NULL;
					//´ÓÖØ´«¶ÓÁÐÉ¾³ýPDU
					if( !list_empty(&amIns->amRetxBuffer.list) )
					{
						list_for_each_entry_safe(amRetxBf, reposBf, &amIns->amRetxBuffer.list,list)
						{
							if( amRetxBf->SN == nackSn)
							{
								if( amRetxBf->pktstatus != FULL_SDU )
								{
									for( i = 0; i < amRetxBf->segnum; i++)
									{
										if( amRetxBf->segStart[i] != -1 )
										{
											amIns->amRetxBufferSize -= amRetxBf->segEnd[i] - amRetxBf->segStart[i];
										}
									}
									amIns->amRetxBufferSize -= amRetxBf->headLen;
								}
								else
								{
									amIns->amRetxBufferSize -= amRetxBf->pkt->len;
								}
								amIns->amRetxBufferNum--;
								fsm_pkt_destroy( amRetxBf->pkt );
								amRetxBf->pkt = NULL;
								list_del(&amRetxBf->list);
								fsm_mem_free(amRetxBf);
								amRetxBf= NULL;
								break;
							}
						}
					}
				}
				else
				{
					rlc_retx(amIns,amTxedBuffer);
					fsm_printf("[RLC][rlc_ctrl_pkt_recv] run to rlc_retx()\n");
				}
				break;
			}
		}
	}
	FOUT;
}

/************************************************************************
**Function name: rlc_ctrl_pkt_recv()
**Descriptions: Receive ctrl PDU
**Input: struct AM_Instance *amIns, FSM_PKT * pkt
**Output: handle the corresponding PDUs according to the ackSn and nackSn fields in the ctrl PDU
**Returns:
**Created by: ChenLang
**Created Date: 2015
**---------------------------------------------------------------------
**Modified by:
**Modified Date:
************************************************************************/

void rlc_ctrl_pkt_recv(struct AM_Instance *amIns, FSM_PKT * pkt)
{
	SequenceNumber ackSn;
	SequenceNumber nackSn;
	ctrlPduPart1 *part1;
	ctrlPduPart2 *part2;
	ctrlPduPart3 *part3; // ¿ØÖÆpdu»®·ÖÎª3¸ö²¿·Ö
	//u32 amHdr;
	//u64 am_part3;
	u32 pduLen=0;
	u8 E1=0;
	bool isPduFound = false;

	FIN(rlc_ctrl_pkt_recv());
	fsm_printf("[RLC][rlc_ctrl_pkt_recv] entering rlc_ctrl_pkt_recv() function\n");
	fsm_printf("[RLC][rlc_ctrl_pkt_recv]  (pkt->len is %d)\n", pkt->len);
	
	pduLen = pkt->len;
	part1 = (ctrlPduPart1 *)fsm_mem_alloc(sizeof(ctrlPduPart1));
	memset(part1,0,sizeof(ctrlPduPart1));
	fsm_mem_cpy(part1,pkt->data,2);//´ÓPKTÈ¡Á½¸ö×Ö½Ú
	fsm_skb_pull(pkt,2);
	*(u16 *)part1 = ntohs(*(u16 *)part1);
	pduLen -= 2;
	fsm_octets_print(part1,2);
	fsm_printf("[RLC][rlc_ctrl_pkt_recv] (part1->E1 is %d)\n",part1->E1);	
	fsm_printf("[RLC][rlc_ctrl_pkt_recv] (part1->acksn is %d)\n",part1->ACK_SN);
	ackSn = part1->ACK_SN;
	FOUT;
	ack_handler(amIns,ackSn);//ÏàÓ¦ACK
	E1 = part1->E1;
	
	if ( part1 != NULL ) //ÊÍ·Åpart1
	{
		fsm_mem_free(part1);
		part1=NULL;
	}
	fsm_printf("[RLC][rlc_ctrl_pkt_recv] (E1 is %d)\n",E1);
	fsm_printf("[RLC][rlc_ctrl_pkt_recv] (pdulen is %d)\n",pduLen);
	fsm_printf("[RLC][rlc_ctrl_pkt_recv] (acksn is %d)\n",ackSn);
	//FOUT;
	while( E1 == 1 && pduLen >=0)//±íÊ¾ÓÐNACK£¬PKT³¤¶È´óÓÚ0
	{
		fsm_printf("[RLC][rlc_ctrl_pkt_recv] entering rlc_ctrl_pkt_recv() cycle\n");
		part2 = (ctrlPduPart2 *)fsm_mem_alloc(sizeof(ctrlPduPart2));
		memset(part2,0,sizeof(ctrlPduPart2));
		fsm_mem_cpy(part2,pkt->data,2);//´ÓPKTÈ¡Á½¸ö×Ö½Ú
		*(u16 *)part2 = ntohs(*(u16 *)part2);
		fsm_skb_pull(pkt,2);
		pduLen -= 2;
		nackSn = part2->NACK_SN;
		if( nackSn == amIns->pollSn )
		{
			//Í£Ö¹²¢¸´Î»ÖØ´«¼ÆÊ±Æ÷
			if(amIns->pollRetxTimer)
			{
				fsm_schedule_cancel(amIns->pollRetxTimer);
				amIns->pollRetxTimer = 0;
			}
		}
		if (part2->E2)//±íÊ¾ÓÐSOstartºÍSOend£¬ÎªctrlPduPart3
		{
			part3 = (ctrlPduPart3 *)fsm_mem_alloc(sizeof(ctrlPduPart3));
			memset(part3,0,sizeof(ctrlPduPart3));
			fsm_mem_cpy(part3,pkt->data,4);//´ÓPKTÈ¡4¸ö×Ö½Ú
			*(u32 *)part3 = ntohs(*(u32 *)part3);
			fsm_skb_pull(pkt,4);
			pduLen -= 4;
			so_handler(amIns,part3,&isPduFound,nackSn);
		}
		else  //±íÊ¾Ã»ÓÐSOstartºÍSOend£¬ÊÇAMD PDU
		{
			no_so_handler(amIns,part2,&isPduFound);
		}
		if( !isPduFound )
		{
			fsm_printf("[RLC][rlc_ctrl_pkt_recv] The corresponding PDU of the nackSn is not found\n");
		}
		E1 = part2->E1;
		if ( part2 != NULL )
		{
			fsm_mem_free(part2);
			part2=NULL;
		}
		if ( part3 != NULL )
		{
			fsm_mem_free(part3);
			part3=NULL;
		}
	}
}

/************************************************************************
**Function name: rlc_poll_retx()
**Descriptions: Move the PDU whose sn == vt_s - 1 to retxbuffer
**Input: struct AM_Instance *amIns
**Output: Move the PDU whose sn == vt_s - 1 to retxbuffer
**Returns:
**Created by: ZhangYi
**Created Date: 2014
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/

void rlc_poll_retx(struct AM_Instance *amIns)
{
	struct AmBuffer *amTxedBuffer;
	struct AmBuffer * pos;
	//struct list_head* pos;
	struct AmBuffer *nposBf;
	//struct AmBuffer *amTxedBf;
	struct AmBuffer *amRetxBf;  //±£´æÖØ´«¶ÓÁÐÖÐÒÑÓÐµÄPDU
	CRLC_StatusInd_IoctrlMsg *mioctrl;
	int i;

	FIN(rlc_poll_retx());
	fsm_printf("[RLC][rlc_poll_retx] entering rlc_poll_retx() function\n");
	//ÒÑ·¢ËÍ¶ÓÁÐ²»Îª¿Õ
	if( !list_empty(&amIns->amTransmittedBuffer.list) )
	{
		fsm_printf("[RLC][rlc_poll_retx] rlc_poll_retx() anTransmittedBuffer is not empty\n");

		//for( pos = amIns->amTransmittedBuffer.list.prev; pos != &amIns->amTransmittedBuffer.list; pos = pos->prev )
		list_for_each_entry_safe(amTxedBuffer, pos, &amIns->amTransmittedBuffer.list,list)
		{
			//amTxedBuffer = list_entry(pos,struct AmBuffer,list);
			//if( amTxedBuffer->SN >= amIns->vt_a && amTxedBuffer->SN < amIns->vt_s)
			if( amTxedBuffer->SN == amIns->vt_s - 1 )
			{
				if( amTxedBuffer->retxCount == -1 )
				{
					amTxedBuffer->retxCount = 0;
				}
				else
				{
					amTxedBuffer->retxCount++;
				}
				fsm_printf("[RLC][rlc_poll_retx] the pdu of rlc_poll_retx's SN is %d\n",amTxedBuffer->SN);
				fsm_printf("[RLC][rlc_poll_retx] the retxCount is %d, the maxThreshold is %d\n",amTxedBuffer->retxCount,amIns->maxRetxThreshold);
				//Èç¹û´ïµ½×î´óÖØ´«´ÎÊýÔò±¨¸æ¸ø¸ß²ã
				if( amTxedBuffer->retxCount == amIns->maxRetxThreshold )
				{
					fsm_printf("[RLC][rlc_poll_retx] has reach the maxRetxThreshold\n");
					mioctrl = (CRLC_StatusInd_IoctrlMsg*)fsm_mem_alloc(sizeof(CRLC_StatusInd_IoctrlMsg));
					mioctrl->eventCode = 0;
					mioctrl->rbIdentity = amIns->rbId;
					//fsm_do_ioctrl(STRM_TO_RRC, CRLC_STATUS_IND, (void*)mioctrl, sizeof(CRLC_StatusInd_IoctrlMsg));
					fsm_mem_free(mioctrl);
					mioctrl = NULL;
					//´ÓÖØ´«¶ÓÁÐÉ¾³ýPDU
					if( !list_empty(&amIns->amRetxBuffer.list) )
					{
						list_for_each_entry_safe(amRetxBf, nposBf, &amIns->amRetxBuffer.list,list)
						{
							if( amRetxBf->SN == amTxedBuffer->SN )
							{
								if( amRetxBf->pktstatus != FULL_SDU )
								{
									for( i = 0; i < amRetxBf->segnum; i++)
									{
										if( amRetxBf->segStart[i] != -1 )
										{
											amIns->amRetxBufferSize -= amRetxBf->segEnd[i] - amRetxBf->segStart[i];
										}
									}
									amIns->amRetxBufferSize -= amRetxBf->headLen;
								}
								else
								{
									amIns->amRetxBufferSize -= amRetxBf->pkt->len;
								}
								amIns->amRetxBufferNum--;
								list_del(&amRetxBf->list);
								if(amRetxBf->pkt != NULL)
								{
									fsm_pkt_destroy( amRetxBf->pkt );
									amRetxBf->pkt = NULL;
								}
								if(amRetxBf != NULL)
								{
									fsm_mem_free(amRetxBf);
									amRetxBf = NULL;
								}
								break;
							}
						}
					//nposBf = NULL;
					}

					//´ÓÒÑ·¢ËÍ¶ÓÁÐÉ¾³ýPDU
					//pos = pos->next;
					amIns->amTransmittedBufferSize -= amTxedBuffer->pkt->len;
					amIns->amTransmittedBufferNum--;
					if(amTxedBuffer->pkt != NULL)
					{
						fsm_pkt_destroy( amTxedBuffer->pkt );
						amTxedBuffer->pkt = NULL;
					}
					list_del(&amTxedBuffer->list);
					if(amTxedBuffer != NULL)
					{
						fsm_mem_free(amTxedBuffer);
						amTxedBuffer = NULL;
					}
					fsm_printf("[RLC][rlc_poll_retx] retx amretxbuffersize is %d\n",amIns->amRetxBufferSize);
				}
				else
				{
					fsm_printf("[RLC][rlc_poll_retx] rlc_poll_retx pdu has not reach maxRetxThreshold\n");
					rlc_poll(amIns,amTxedBuffer->pkt);	//ÉèÖÃÂÖÑ¯Î»
					rlc_retx(amIns,amTxedBuffer);
					break;
				}
			}
		}
	}
	else
	{
		fsm_printf("[RLC][rlc_poll_retx] amTransmittedBuffer is empty\n");
	}
	FOUT;
}

/************************************************************************
**Function name: rlc_retx()
**Descriptions: move the PDU which need to be retransimitted to the retx buffer,and duplicate another one to the transmitted buffer,
				if the PDU has been in the retx buffer then remove it
**Input: struct AM_Instance *amIns,struct AmBuffer *amTxedBuffer
**Output: move the PDU which need to be retransimitted to the retx buffer,and duplicate another one to the transmitted buffer,
				if the PDU has been in the retx buffer then remove it
**Returns:
**Created by: ZhangYi
**Created Date: 2014
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/

void rlc_retx(struct AM_Instance *amIns,struct AmBuffer *amTxedBuffer)
{
	struct AmBuffer *amTxedBf;
	struct AmBuffer *amRetxBf;
	struct AmBuffer *nposBf;
	int i;

	FIN(rlc_retx());
	fsm_printf("[RLC][rlc_retx] entering rlc_retx() function\n");
	//Èç¹ûÖØ´«¶ÓÁÐÖÐÒÑÓÐ¶ÔÓ¦µÄPDUÔòÏÈ½«ÆäÉ¾³ý
	if( !list_empty(&amIns->amRetxBuffer.list) )
	{
		list_for_each_entry_safe(amRetxBf, nposBf, &amIns->amRetxBuffer.list,list)
		{
			if( amRetxBf->SN == amTxedBuffer->SN )
			{
				fsm_printf("[RLC][rlc_retx] has found the pdu %d in RetxBuffer\n",amRetxBf->SN);
				if( amRetxBf->pktstatus != FULL_SDU )
				{
					for( i = 0; i < amRetxBf->segnum; i++)
					{
						if( amRetxBf->segStart[i] != -1 )
						{
							amIns->amRetxBufferSize -= amRetxBf->segEnd[i] - amRetxBf->segStart[i];
						}
					}
					amIns->amRetxBufferSize -= amRetxBf->headLen;
				}
				else
				{
					amIns->amRetxBufferSize -= amRetxBf->pkt->len;
				}
				amIns->amRetxBufferNum--;
				if(amRetxBf->pkt != NULL)
				{
					fsm_pkt_destroy( amRetxBf->pkt );
					amRetxBf->pkt = NULL;
				}
				list_del(&amRetxBf->list);
				if(amRetxBf != NULL)
				{
					fsm_mem_free(amRetxBf);
					amRetxBf = NULL;
				}
				break;
			}
		}
		//nposBf = NULL;
	}

	//ÒÆµ½ÖØ´«¶ÓÁÐ
	amIns->amTransmittedBufferSize -= amTxedBuffer->pkt->len;
	amIns->amTransmittedBufferNum--;
	list_del(&amTxedBuffer->list);
	list_add_tail(&amTxedBuffer->list,&amIns->amRetxBuffer.list);
	if( amTxedBuffer->pktstatus != FULL_SDU )
	{
		for( i = 0; i < amTxedBuffer->segnum; i++)
		{
			if( amTxedBuffer->segStart[i] != -1 )
			{
				amIns->amRetxBufferSize += amTxedBuffer->segEnd[i] - amTxedBuffer->segStart[i];
			}
		}
		amIns->amRetxBufferSize += amTxedBuffer->headLen;
	}
	else
	{
		amIns->amRetxBufferSize += amTxedBuffer->pkt->len;
	}
	amIns->amRetxBufferNum++;
	fsm_printf("[RLC][rlc_retx] has move to RetxBuffer,%d\n",amTxedBuffer->SN);
	//ÔÙ¸´ÖÆÒ»·Ýµ½ÒÑ·¢ËÍ¶ÓÁÐ
	amTxedBf = (struct AmBuffer *)fsm_mem_alloc(sizeof(struct AmBuffer));
	//amTxedBf->pkt = fsm_pkt_create(amTxedBuffer->pkt->len+fsm_skb_headroom(amTxedBuffer->pkt));
	amTxedBf->pkt = fsm_pkt_duplicate(amTxedBuffer->pkt);
	//fsm_mem_cpy(amTxedBf->pkt->data,amTxedBuffer->pkt->data,amTxedBuffer->pkt->len);
	//fsm_skb_put(amTxedBf->pkt,amTxedBuffer->pkt->len);
	amTxedBf->datalen = amTxedBuffer->datalen;
	amTxedBf->headLen = amTxedBuffer->headLen;
	amTxedBf->pktstatus = amTxedBuffer->pktstatus;
	amTxedBf->pos = amTxedBuffer->pos;
	amTxedBf->retxCount = amTxedBuffer->retxCount;
	amTxedBf->segnum = amTxedBuffer->segnum;
	for( i = 0;i < amTxedBuffer->segnum; i++)
	{
		amTxedBf->segStart[i] = amTxedBuffer->segStart[i];
		amTxedBf->segEnd[i] = amTxedBuffer->segEnd[i];
	}
	amTxedBf->SN = amTxedBuffer->SN;
	list_add_tail(&amTxedBf->list,&amIns->amTransmittedBuffer.list);
	amIns->amTransmittedBufferSize += amTxedBf->pkt->len;
	amIns->amTransmittedBufferNum++;
	fsm_printf("[RLC][rlc_retx] has copy to amTxedBuffer,%d\n",amTxedBuffer->SN);
	FOUT;
}

/************************************************************************
**Function name: rlc_poll()
**Descriptions: Set polling bit to 1
**Input: struct AM_Instance *amIns, FSM_PKT * pkt
**Output: set the polling bit in the head of the PDU to 1
**Returns:
**Created by: ZhangYi
**Created Date: 2014
**---------------------------------------------------------------------
**Modified by: ZhangYi
**Modified Date: 2014
************************************************************************/

void rlc_poll(struct AM_Instance *amIns, FSM_PKT * pkt)
{
	//u16 amHdr = 0x0000; //±£´æAM PDUÍ·²¿
	RLC_AMPDU_fixhead * amHdr;

	FIN(rlc_poll());
	//SV_PTR_GET(rlc_sv);
	fsm_printf("[RLC][rlc_poll] entring rlc_poll()\n");
	//fsm_mem_cpy(&amHdr,pkt->data,2);//È¡Í·²¿	
	amHdr = (RLC_AMPDU_fixhead *)pkt->data;
	*(u16*)amHdr=htons(*(u16*)amHdr);
	if(amHdr->P!=1)
	{
		amHdr->P = 1;
	}
	*(u16*)amHdr=ntohs(*(u16*)amHdr);
	/*if( (amHdr&0x2000)>>13 == 0 )
	{
		amHdr|=1<<13; //½«pollingÎ»ÖÃ1
	}
	fsm_mem_cpy(pkt->data,&amHdr,2);//·ÅÍ·²¿*/
	amIns->pduWithoutPoll = 0;
	amIns->byteWithoutPoll = 0;
	amIns->pollSn = amIns->vt_s - 1;
	if(amIns->pollRetxTimer)
	{
		fsm_schedule_cancel(amIns->pollRetxTimer);
		amIns->pollRetxTimer = 0;
	}
	amIns->pollRetxTimer = fsm_schedule_self(amIns->pollRetxTimerValue,amIns->pollRetxTimerCode);
	fsm_printf("[RLC][rlc_poll] pollretxTimer start\n");
	FOUT;
}

