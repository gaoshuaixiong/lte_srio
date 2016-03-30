/*******************************Copyright (c)*********************************************
 **                University of Electronic Science and Technology of China
 **                  School of Communication and Information Engineering
 **                          http://www.uestc.edu.cn
 **
 **--------------------------- File Info ---------------------------------------------------
 ** File name:           ipadpfsm.c
 ** Last modified Date:  2014-09-08
 ** Last Version:        2.00
 ** Descriptions:        design the ipadp fsm and correspond function, include the packet 
 **						transmit and receive, and some ioctl.
 **                      Based on Linux 10.04. 
 **------------------------------------------------------------------------------------------
 ** Created by:          Lou lina
 ** Created date:        
 **------------------------------------------------------------------------------------------
 ** Modified by:   		Li xiru
 ** Modified date: 		2014-09-08
 ** Version:    			2.00
 ** Descriptions:  		modified the function: packet_send_to_pdcp() and packet_send_to_upperlayer(), 
 **						added some notes. 
 **
 *******************************************************************************************/

#include <linux/if_ether.h>
#include "ipadpfsm.h"
#include "../lte_system.h"
#include "../pkfmt.h"
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/kernel.h>
//#include <netinet/in.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>
#include <linux/inet.h>

#define ST_INIT 0
#define	ST_IDLE 1
#define ST_SEND	2
#define ST_RECV	3

#define IOCCMD_PSEND_RUN 	0x01
#define IOCCMD_PSEND_STOP 	0x02
#define IOCCMD_PSEND_INTERVAL	0x03
#define IOCCMD_SAY_HELLO 	0x04

static void init_enter(void);
static void ipadp_sv_init(void);	
static void ipadp_close(void);
static void do_ioctl_period(void);
static void packet_send_to_pdcp(void);
static void packet_send_to_upperlayer(void);
static void idle_exit(void);
static void ioctl_handler(void);
static void send_packet_period(void);
static void packet_send_to_upperlayer_telnet(void);

void Ipadp_printf(void);

void ipadp_main(void)
{
	FSM_ENTER(ipadp_main);
	FSM_BLOCK_SWITCH
	{
		FSM_STATE_FORCED(ST_INIT, "INIT", init_enter(), )	//change UNFORCED to FORCED,20150111	
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "INIT -> IDLE");
			/*FSM_COND_TEST_IN("INIT")				
				FSM_TEST_COND(START_WORK)							
				FSM_COND_TEST_OUT("INIT")			
				FSM_TRANSIT_SWITCH			
				{	
					FSM_CASE_TRANSIT(0, ST_IDLE, ipadp_sv_init(), "INIT -> IDLE")						
						FSM_CASE_DEFAULT(ST_INIT, , "INIT->INIT")
				}*/			
		}		
		FSM_STATE_UNFORCED(ST_IDLE, "IDLE",send_packet_period(),idle_exit())		
		{
			FSM_COND_TEST_IN("IDLE")				
				FSM_TEST_COND(IPADP_PK_FROM_LOWER)				
				FSM_TEST_COND(IPADP_PK_FROM_UPPER)
				FSM_TEST_COND(IPADP_CLOSE)			
				FSM_COND_TEST_OUT("IDLE")	
				FSM_TRANSIT_SWITCH			
				{	
					FSM_CASE_TRANSIT(0, ST_RECV, Ipadp_printf(), "IDLE -> RECV")				
						FSM_CASE_TRANSIT(1, ST_SEND, , "IDLE -> SEND")
						FSM_CASE_TRANSIT(2, ST_INIT, , "IDLE -> INIT")				
						FSM_CASE_DEFAULT(ST_IDLE, , "IDLE->IDLE")	//transit to idle state	by default.
				}	
		}
		FSM_STATE_FORCED(ST_RECV, "RECV", packet_send_to_upperlayer(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "RECV -> IDLE");
		}
		FSM_STATE_FORCED(ST_SEND, "SEND", packet_send_to_pdcp(), )
		{
			FSM_TRANSIT_FORCE(ST_IDLE, , "default", "", "SEND -> IDLE");
		}
	}
	FSM_EXIT(0)
}

void Ipadp_printf(void)
{
	FIN(Ipadp_printf());
	fsm_printf("[IPADP]RECIEVE_IPADP_PK_FROM_LOWER\n");
	FOUT;
}
static void init_enter(void)
{
	FIN(init_enter());
	//SV_PTR_GET(ipadp_sv);
	if(IPADP_OPEN)
	{
		//fsm_schedule_self(0, _START_WORK);	//20150111
		ipadp_sv_init();

	}
	FOUT;
}

static void ipadp_sv_init(void)
{
	FIN(ipadp_sv_init());
	SV_PTR_GET(ipadp_sv);
	SV(hello_count) = 0;
	SV(packet_count) = 0;
	SV(interval) = 100000;
	SV(V_Flag) = true;
	//INIT_LIST_HEAD(&SV(pktBuf).list);	//initial the list head,20141202
	//fsm_schedule_self(0, _DO_IOCTL_PERIOD);
	FOUT;
}

static void ipadp_close(void)
{
	//pktbuffer *pktbuf, *tempPktBuf;
	FIN(ipadp_close());
	SV_PTR_GET(ipadp_sv);
	fsm_printf("ipadp has sent %d times hello!.\n", SV(hello_count));
	fsm_printf("ipadp has sent %d packets.\n", SV(packet_count));
	/*if(!list_empty(&SV(pktBuf).list))
	{
		list_for_each_entry_safe(pktbuf, tempPktBuf, &SV(pktBuf).list, list) //20141201
		{
			list_del(&pktbuf->list);
			if(pktbuf->pkt != NULL)
			{
				fsm_pkt_destroy(pktbuf->pkt);
				pktbuf->pkt = NULL;
			}
			fsm_mem_free(pktbuf);
			if(list_empty(&SV(pktBuf).list))
			{
				fsm_printf("[IPADP][rlc_close] the pktBuf has released\n");
				break;
			}
		}
	}*/
	FOUT;
}

static void do_ioctl_period()
{
	const char* data_ptr = "Hello RLC\n";
	FIN(do_ioctl_period());
	SV_PTR_GET(ipadp_sv);
	if(DO_IOCTL_PERIOD)
	{
		fsm_do_ioctrl(STRM_TO_RLC, IOCCMD_SAY_HELLO, (void*)data_ptr, 12);
		fsm_printf("do ioctl period\n");
		fsm_printf(data_ptr);
		SV(psend_handle) = fsm_schedule_self(SV(interval), _DO_IOCTL_PERIOD);
	}
	FOUT;
}

static void idle_exit(void)
{
	FIN(idle_exit());
	if(IPADP_CLOSE)
	{
		ipadp_close();
	}
	else if(IOCTL_ARRIVAL)
	{
		ioctl_handler();
	}

	FOUT;
}

/*******************************************************************************************
 ** Function name: packet_send_to_pdcp()
 ** Descriptions	: receive upper layer(IP) packet, after adding the head and IciMsg,
 **					then send to lower layer(pdcp).
 ** Input: void
 ** Output: data buffer -- packet
 **	return	: NONE.
 ** Created by	: Lou lina
 ** Created Date	: 
 **------------------------------------------------------------------------------------------
 ** Modified by	: Li xiru
 ** Modified Date: 2014-09-08
 **------------------------------------------------------------------------------------------
 *******************************************************************************************/

static void packet_send_to_pdcp(void)
{
	FSM_PKT* pkptr;
	//u8 *p_rbid;
	struct iphdr* iph_ptr;
	struct udphdr* udph_ptr;
	struct tcphdr* tcph_ptr;
	struct icmphdr* icmph_ptr;
	struct lte_test_ipadp_head* ipah_ptr;
	struct UPDCP_IciMsg* Ici_ptr;  //time:20140729
	u8 protocol,drbid, type;
	u16 dport,userid;
	int i=0;  //FOR TEST

	FIN(packet_send_to_pdcp());
	SV_PTR_GET(ipadp_sv);
	//p_rbid = (u8*)fsm_mem_alloc(sizeof(u8));
	pkptr = fsm_pkt_get();
	fsm_printf("[IPADP]\n");
//	fsm_octets_print(pkptr->data , 128);
	if(pkptr == NULL) return;
	if(pkptr->network_header == NULL) return;
	//fsm_printf("	IPADP0 has recv IP packet head: \n");
	iph_ptr = (struct iphdr*)pkptr->network_header;
	protocol = iph_ptr->protocol;

	//get last byte of IP addr,turn it to userid;
	userid =  ((unsigned char*)&iph_ptr->saddr)[3];
	/*u8 *destIP;
	in4_pton((void *)&iph_ptr->daddr, strlen((void *)&iph_ptr->daddr), destIP, '\0', NULL);
	//destIP = iph_ptr->daddr;
	//inet_pton(AF_INET, destIP, (void *)&iph_ptr->daddr);
	printk("the destIP is: %s\n", destIP);
	if(strcmp(destIP, "192.168.1.190") != 0&&strcmp(destIP, "192.168.1.199") != 0)
	{	
		fsm_pkt_destroy(pkptr);
		return;
	}*/
	fsm_printf("		IP tos: %d;IP version:%d;protocol:%d\n",iph_ptr->tos,iph_ptr->version,iph_ptr->protocol);
	//fsm_printf("		IP source addr:%u.%u.%u.%u;IP dest addr:%u.%u.%u.%u;\n",NIPQUAD(iph_ptr->saddr),NIPQUAD(iph_ptr->daddr));
	//extract UDP port,trun it to drb id;
	switch(protocol){
		case 1:
			//ICMP code
			fsm_printf("[IPADP][ICMP]iph_ptr->ihl=%d\n",iph_ptr->ihl);
			fsm_printf("[IPADP][ICMP]\n");  //FOR TEST
	//		fsm_octets_print(pkptr->transport_header, 128);
			//icmph_ptr = (struct icmphdr*)(pkptr->network_header + (iph_ptr->ihl)*4);
			//if(pkptr->transport_header != NULL){	//icmp_header?!
			//	icmph_ptr = (struct icmphdr*)pkptr->transport_header;
			//	type = icmph_ptr->type;
			//	fsm_printf("[IPADP]whethe return\n"); 
			//	if((type != 8) && (type != 0)) return;  //Is the type the ping
				drbid = (u8)(4);	//map the ICMP services' drbid as 10001
				//fsm_mem_cpy(p_rbid,&drbid,sizeof(u8));	//copy the drbid to p_rbid, noted in 141209
				//fsm_printf("		UDP source port:%d,dest port:%d;len:%d\n",fsm_ntohs(udph_ptr->source),fsm_ntohs(udph_ptr->dest),fsm_ntohs(udph_ptr->len));
				//add ipadp head 
				if( fsm_skb_headroom(pkptr) < (sizeof(struct lte_test_ipadp_head))){
					pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_test_ipadp_head));
					if(pkptr == NULL)
						return;
				}
				fsm_skb_push(pkptr, sizeof(struct lte_test_ipadp_head));
				ipah_ptr = (struct lte_test_ipadp_head*)pkptr->data;	//change "data" to "head",20140918
				ipah_ptr->type = fsm_htonl(0);
				ipah_ptr->user_id = userid;
				ipah_ptr->drb_id = drbid;//default:only drb0 in easiest system 
				fsm_printf("		user id =%d,drb id = %d\n",ipah_ptr->user_id,ipah_ptr->drb_id); 
				//warning: rbid output format error, following same
				ipah_ptr->len = fsm_htonl(pkptr->len - sizeof(struct lte_test_ipadp_head));
				//skb_reset_network_header(pkptr);

				//fsm_skb_push(pkptr, sizeof(struct URLC_IciMsg));
				Ici_ptr = (struct UPDCP_IciMsg*)pkptr->head;
				Ici_ptr->pbCh = 0;
				Ici_ptr->rbId = drbid;
				Ici_ptr->rnti = 61;
				fsm_printf("		pbCh = %d, rbId = %d\n",Ici_ptr->pbCh,Ici_ptr->rbId);

				//send packet to PDCP
				//fsm_printf("[IPADP] the pkt len is %d\n",pkptr->len);
				fsm_printf("[IPADP][ICMP]the data is \n");  //FOR TEST
		//		fsm_octets_print(pkptr->data , 128);
				/*for(i=0;i<50;i++)*/
				/*fsm_printf("%c ",*(pkptr->data+i)+65);*/
				/*fsm_printf("\n");*/
				//ioctrl to RRC
				//fsm_do_ioctrl(STRM_TO_RRC, CTRL_RBID_MAP, (void*)p_rbid, sizeof(u8));		noted in 20141209
				if(SV(V_Flag) == true)
				{
					fsm_pkt_send(pkptr,STRM_TO_PDCP);
					++SV(packet_count);
					fsm_printf("[IPADP]IPADP sends IP packet to PDCP.\n");
				}
				else
				{
					fsm_printf("[IPADP] V_Flag == false\n");
				}
				//++SV(packet_count);
				//fsm_printf("		IPADP sends IP packet to PDCP.\n");	 				
			//}
			break;
		case 6:
			//TCP code
			if(pkptr->transport_header != NULL){
				tcph_ptr = (struct tcphdr*)pkptr->transport_header;
				dport = fsm_ntohs(tcph_ptr->dest);
				drbid = (u8)(4);	//map the TCP services' drbid as 10002
				//fsm_printf("		UDP source port:%d,dest port:%d;len:%d\n",fsm_ntohs(udph_ptr->source),fsm_ntohs(udph_ptr->dest),fsm_ntohs(udph_ptr->len));
				//fsm_mem_cpy(p_rbid,&drbid,sizeof(u8));
				//add ipadp head 
				if( fsm_skb_headroom(pkptr) < (sizeof(struct lte_test_ipadp_head))){
					pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_test_ipadp_head));
					if(pkptr == NULL)
						return;
				}
				fsm_skb_push(pkptr, sizeof(struct lte_test_ipadp_head));
				ipah_ptr = (struct lte_test_ipadp_head*)pkptr->data;	//change "data" to "head",20140918
				ipah_ptr->type = fsm_htonl(0);
				ipah_ptr->user_id = userid;
				ipah_ptr->drb_id = drbid;//default:only drb0 in easiest system 
				fsm_printf("		user id =%d,drb id = %d\n",ipah_ptr->user_id,ipah_ptr->drb_id); 
				//warning: rbid output format error, following same
				ipah_ptr->len = fsm_htonl(pkptr->len - sizeof(struct lte_test_ipadp_head));
				//skb_reset_network_header(pkptr);

				//fsm_skb_push(pkptr, sizeof(struct URLC_IciMsg));
				Ici_ptr = (struct UPDCP_IciMsg*)pkptr->head;
				Ici_ptr->pbCh = 0;
				Ici_ptr->rbId = drbid;
				Ici_ptr->rnti = 61;
				fsm_printf("		pbCh = %d, rbId = %d\n",Ici_ptr->pbCh,Ici_ptr->rbId);


				//send packet to PDCP
				//fsm_printf("[IPADP] the pkt len is %d\n",pkptr->len);
				fsm_printf("[IPADP] the data is \n");  //FOR TEST
		//		fsm_octets_print(pkptr->data , pkptr->tail-pkptr->data);
				/*
				for(i=0;i<50;i++)
					fsm_printf("%c ",*(pkptr->data+i)+65);
				fsm_printf("\n");
				*/
				//fsm_do_ioctrl(STRM_TO_RRC, CTRL_RBID_MAP, (void*)p_rbid, sizeof(u8));					
				if(SV(V_Flag) == true)
				{
					fsm_pkt_send(pkptr,STRM_TO_PDCP);
					++SV(packet_count);
					fsm_printf("		IPADP sends IP packet to PDCP.\n");
				}
				else
				{
					fsm_printf("[IPADP] V_Flag == false\n");
				}
				//++SV(packet_count);
				//fsm_printf("		IPADP sends IP packet to PDCP.\n");	 				
			}
			break;
		case 17:
			//UDP code
			if(pkptr->transport_header != NULL){
				udph_ptr = (struct udphdr*)pkptr->transport_header;
				dport = fsm_ntohs(udph_ptr->dest);
				drbid = (u8)(4);	//map the UDP services' drbid as 10003
				//fsm_printf("		UDP source port:%d,dest port:%d;len:%d\n",fsm_ntohs(udph_ptr->source),fsm_ntohs(udph_ptr->dest),fsm_ntohs(udph_ptr->len));
				//fsm_mem_cpy(p_rbid,&drbid,sizeof(u8));
				//add ipadp head 
				if( fsm_skb_headroom(pkptr) < (sizeof(struct lte_test_ipadp_head))){
					pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_test_ipadp_head));
					if(pkptr == NULL)
						return;
				}
				fsm_skb_push(pkptr, sizeof(struct lte_test_ipadp_head));
				ipah_ptr = (struct lte_test_ipadp_head*)pkptr->data;	//change "data" to "head",20140918
				ipah_ptr->type = fsm_htonl(0);
				ipah_ptr->user_id = fsm_htons(userid);
				ipah_ptr->drb_id = fsm_htons(drbid);//default:only drb0 in easiest system 
				fsm_printf("		user id =%d,drb id = %d\n", userid, drbid);	//20150108 
				//warning: rbid output format error, following same
				ipah_ptr->len = fsm_htonl(pkptr->len - sizeof(struct lte_test_ipadp_head));
				//skb_reset_network_header(pkptr);

				//fsm_skb_push(pkptr, sizeof(struct URLC_IciMsg));
				Ici_ptr = (struct UPDCP_IciMsg*)pkptr->head;
				Ici_ptr->pbCh = 0;
				Ici_ptr->rbId = drbid;
				Ici_ptr->rnti = 61;
				fsm_printf("		pbCh = %d, rbId = %d\n",Ici_ptr->pbCh,Ici_ptr->rbId);

				//send packet to PDCP
				//fsm_printf("[IPADP] the pkt len is %d\n",pkptr->len);
				fsm_printf("[IPADP] the data is \n");  //FOR TEST
		//		fsm_octets_print(pkptr->data , pkptr->tail-pkptr->data);
				/*
				for(i=0;i<50;i++)
					fsm_printf("%c ",*(pkptr->data+i)+65);
				fsm_printf("\n");
				*/
				//fsm_do_ioctrl(STRM_TO_RRC, CTRL_RBID_MAP, (void*)p_rbid, sizeof(u8));
				if(SV(V_Flag) == true)
				{
					fsm_pkt_send(pkptr,STRM_TO_PDCP);
					++SV(packet_count);
					fsm_printf("		IPADP sends IP packet to PDCP.\n");
				}
				else
				{
					fsm_printf("[IPADP] V_Flag == false\n");
				}
					 				
			} break;
		default: return;
	}
	/*if(p_rbid != NULL){
	  fsm_mem_free(p_rbid);
	  p_rbid = NULL;
	  }*/	//free the pointer,very important!!!
	FOUT;
}

/*******************************************************************************************
 ** Function name: packet_send_to_upperlayer()
 ** Descriptions	: receive lower layer(PDCP) packet, delete the IciMsg,
 **					then send to upper layer(IP).
 ** Input: void
 ** Output: data buffer -- packet
 **	return	: NONE.
 ** Created by	: Lou lina
 ** Created Date	: 
 **------------------------------------------------------------------------------------------
 ** Modified by	: Li xiru
 ** Modified Date: 2014-09-08
 **------------------------------------------------------------------------------------------
 *******************************************************************************************/
static void packet_send_to_upperlayer(void)
{
	FSM_PKT* pkptr;
	struct lte_test_ipadp_head* sh_ptr;
	struct UPDCP_IciMsg* Ici_ptr;		//time:20140729
	FIN(packet_send_to_upperlayer());
	pkptr = fsm_pkt_get();

	//extract the ICIMsg and printf,20140729
	Ici_ptr = (struct UPDCP_IciMsg*)pkptr->head;
	fsm_printf("[IPADP]		pbCh = %d, rbId = %d\n",Ici_ptr->pbCh,Ici_ptr->rbId);
			//remove the IciMsg,20140729

	sh_ptr = (struct lte_test_ipadp_head*)pkptr->data;
	fsm_printf("[IPADP][packet_send_to_upperlayer]pkptr->data[new version]:\n");
//	fsm_octets_print(pkptr->data, 128);
	//fsm_skb_pull(pkptr,sizeof(struct lte_test_ipadp_head));		//fatal, noted in 20150111
	if(fsm_ntohl(sh_ptr->type) == 0){
			
			fsm_skb_pull(pkptr, sizeof(struct lte_test_ipadp_head));
			fsm_pkt_send(pkptr, STRM_TO_IP);
			fsm_printf("	IPADP recv packet: \n");
			fsm_printf("		userid:%d,drbid:%d\n",fsm_ntohs(sh_ptr->user_id),sh_ptr->drb_id);
			fsm_printf("	IPADP:send packet to IP !\n");
	}	
	/*else if(fsm_ntohl(sh_ptr->type) == 1){
		fsm_printf("		IPADP0 has recv srio packet: \n");
		fsm_printf("			userid:%d,drbid:%d,datalen:%d\n",fsm_ntohs(sh_ptr->user_id),sh_ptr->drb_id,fsm_ntohl(sh_ptr->len));

		fsm_skb_pull(pkptr, sizeof(struct lte_test_ipadp_head));
		fsm_printf("			data content is:%s\n",(char*)pkptr->head);
		fsm_printf("		ipadp0 recv finished!\n");
		fsm_pkt_destroy(pkptr);
	}*/
	else {
		fsm_printf("		IPADP0 has recv packet: \n");
		//fsm_printf((char*)pkptr->data);
		//fsm_printf("\n");
		fsm_pkt_destroy(pkptr);
	}
	FOUT;
}

/*******************************************************************************************
 ** Function name: ioctl_handler()
 ** Descriptions	: according different ioctl command, execute different fsm_schedule_self.
 ** Input: void
 ** Output: 
 **	return	: NONE.
 ** Created by	: Lou lina
 ** Created Date	: 
 **------------------------------------------------------------------------------------------
 ** Modified by	: Li xiru
 ** Modified Date: 2014-09-08
 **------------------------------------------------------------------------------------------
 *******************************************************************************************/

static void ioctl_handler(void)
{
	const char* data_ptr = "Hello IP\n"; 
	unsigned int *interval_ptr;
	char* rec_data_ptr;

	//pktbuffer *pktTxBuf;
	//pktbuffer *pos;
	CRLC_RbidBuild_IOCTRLMsg *rlcioct;
	FIN(ioctl_handler());
	SV_PTR_GET(ipadp_sv);
	//fsm_printf("event ioctl cmd =%d\n",fsm_ev_ioctrl_cmd());
	switch(fsm_ev_ioctrl_cmd())
	{
		case IOCCMD_PSEND_RUN:
			if(SV(psend_handle) == 0)
			{
				SV(psend_handle) = fsm_schedule_self(SV(interval), _SEND_PACKET_PERIOD);
			}
			FOUT;
		case IOCCMD_PSEND_STOP:
			if(SV(psend_handle))
			{
				fsm_schedule_cancel(SV(psend_handle));
				SV(psend_handle)= 0;

			}
			FOUT;
		case IOCCMD_PSEND_INTERVAL:
			interval_ptr = (unsigned int*)fsm_data_get();
			SV(interval) = *interval_ptr;
			fsm_data_destroy(interval_ptr);
			if(SV(psend_handle))
			{
				fsm_schedule_cancel(SV(psend_handle));
				SV(psend_handle) = fsm_schedule_self(SV(interval), _SEND_PACKET_PERIOD);	
			}
			else
			{
				SV(psend_handle) = fsm_schedule_self(SV(interval), _SEND_PACKET_PERIOD);
			}
			FOUT;
		case IOCCMD_SAY_HELLO:
			rec_data_ptr = (char*)fsm_data_get();
			fsm_printf(rec_data_ptr);
			++ SV(hello_count);
			fsm_data_destroy(rec_data_ptr);
			//fsm_do_ioctrl(STRM_TO_MAC, IOCCMD_SAY_HELLO, (void*)data_ptr, 11);	
			FOUT;
			//add CTRL_RBID_BUILD 20141201
		case CTRL_RBID_BUILD:
			if(rlcioct->V_Flag == true)
			{
				SV(V_Flag) = true;
			}
			else
			{
				SV(V_Flag) = false;
			}
			/*list_for_each_entry_safe(pktTxBuf, pos, &SV(pktBuf).list,list)
			{
				//amTxedBuffer = list_entry(pos,struct AmBuffer,list);
				//if( amTxedBuffer->SN >= amIns->vt_a && amTxedBuffer->SN < amIns->vt_s)
				if(rlcioct->V_Flag == true){		//determine the V_Flag, 20141215
					fsm_pkt_send(pktTxBuf->pkt,STRM_TO_PDCP);
					SV(pktBufSize) -= pktTxBuf->pkt->len;
					SV(pktBufNum) -= 1;

					list_del(&pktTxBuf->list);
					if(pktTxBuf != NULL){	
						fsm_mem_free(pktTxBuf);	
						pktTxBuf = NULL;
					}
					//fsm_pkt_send(pkptr,STRM_TO_PDCP);
					++SV(packet_count);
				}
			}*/
			FOUT;
		default:
			fsm_printf("IPadp:Unrecognized I/O control command.\n");
			FOUT;
	}
}

/*******************************************************************************************************************
 *for test : telnet commend to connect LTE_IPADP0 of another host 
 *print some info of IP and TCP protocal .eg:sour addr,dest addr,sour port,dest port.....
 *for using this fun,you need change packet_send_to_upperlayer() to packet_send_to_upperlayer_telnet() in ipadp_main().
 *********************************************************************************************************************/
static void packet_send_to_upperlayer_telnet(void)
{
	FSM_PKT* pkptr;
	struct ethhdr* head_ptr;
	struct iphdr* iph_ptr;
	struct tcphdr* tcph_ptr;
	u8 transprotol=0;
	FIN(packet_send_to_upperlayer_telnet());
	pkptr = fsm_pkt_get();
	if(pkptr->network_header!= NULL){
		iph_ptr = (struct iphdr*)pkptr->network_header;
		transprotol = iph_ptr->protocol;
		fsm_printf("		IPADP0 has recv IP packet head: \n");
		fsm_printf("		IP tos: %d;IP version:%d;protocol:%d\n",iph_ptr->tos,iph_ptr->version,iph_ptr->protocol);
		//fsm_printf("		IP source addr:%u.%u.%u.%u;IP dest addr:%u.%u.%u.%u;\n",NIPQUAD(iph_ptr->saddr),NIPQUAD(iph_ptr->daddr));
	}
	if((pkptr->transport_header!= NULL)&&(transprotol == 6)){		
		tcph_ptr = (struct tcphdr*)pkptr->transport_header;
		fsm_printf("		IPADP0 has recv TCP packet head: \n");
		fsm_printf("		TCP seq: %d;TCP ack_seq:%d;source port:%d,dest port:%d\n",tcph_ptr->seq,tcph_ptr->ack_seq,tcph_ptr->source,tcph_ptr->dest);		
	}
	fsm_pkt_destroy(pkptr);
	FOUT;
}

/*******************************************************************************************************************
 *for test : send a packet to RLC/PDCP 
 *you can use it as transmit fun of idle state in ipadp_main().
 *then you must send ioctl from userspace for active it.
 *********************************************************************************************************************/
static void send_packet_period(void)
{
	FSM_PKT* pkptr;
	struct lte_test_ipadp_head* ipah_ptr;
	char* data = "IPADP says hello world!";
	FIN(send_packet_period());
	SV_PTR_GET(ipadp_sv);
	if(SEND_PACKET_PERIOD)
	{
		pkptr = fsm_pkt_create(256);
		fsm_skb_put(pkptr, 128);
		fsm_mem_cpy(pkptr->data, data, 24);
		if(fsm_skb_headroom(pkptr) < (sizeof(struct lte_test_ipadp_head))){
			pkptr = fsm_skb_realloc_headeroom(pkptr,sizeof(struct lte_test_ipadp_head));
			if(pkptr == NULL)
				return;
		}
		fsm_skb_push(pkptr, sizeof(struct lte_test_ipadp_head));
		ipah_ptr = (struct lte_test_ipadp_head*)pkptr->data;
		ipah_ptr->type = fsm_htonl(1);
		ipah_ptr->len = fsm_htonl(pkptr->len - sizeof(struct lte_test_ipadp_head));
		skb_reset_network_header(pkptr);

		//fsm_printf("set new timer\n");
		SV(psend_handle) = fsm_schedule_self(SV(interval), _SEND_PACKET_PERIOD);
		//fsm_printf("timer event is added\n");
		//fsm_pkt_destroy(pkptr);
		fsm_pkt_send(pkptr,STRM_TO_RLC);//or PDCP
		++SV(packet_count);
		fsm_printf("IPADP sends hello world packet.\n");
	}
	FOUT;
}







