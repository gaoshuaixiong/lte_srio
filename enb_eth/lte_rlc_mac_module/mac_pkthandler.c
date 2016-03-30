#include <linux/if_ether.h>
/****lhl改动20140410****/
#include <linux/list.h>
#include <linux/kernel.h>
#include <stddef.h>
/****lhl改动结束20140410****/ 
#include "rlc_macfsm.h"
#include "macpkfmt.h"
#include "../lte_system.h"

#define MAX_LENGTH 256
#define REDUNDANCE 10
#define RESERVE_LENGTH 50
#define CRNTI_TYPE 2
#define T_C_RNTI 1
#define DL_SCHEDULE_NUM 5

//./sriotest -msg1 LTE_SRIO0//
//FOR TEST!//
u16 getRA_rnti(void * ue_infor){
	struct S_RAinfo ra_info;
	u16 ra_rnti;
	u32 infor_len=sizeof(u32),len;

	len=sizeof(struct S_RAinfo);
	fsm_mem_cpy(&ra_info,(S_RAinfo*)(ue_infor+infor_len),len);	//for test
	ra_rnti=ra_info.ra_rnti;

	//fsm_printf("[HEXI]GET RA_RNTI:%d\n",ra_rnti);
	return ra_rnti;
}
//TEST END!//


//void complexRarPdu(struct S_RAinfo m_rar_list){
void complexRarPdu(void * ue_infor){

	FIN(complexRarPdu());
	u32 from_len=0,len=0,head_len=0,ue_numb=0,infor_len=sizeof(u32),ttl_len=0;
	char bi=0;
	u32 ul_grant;
	//char typ=1;	//delete typ in 20160122
	u32 i;
	
	//struct S_RAinfo m_rar_list=(struct S_RAinfo)fsm_mem_alloc(sizeof(struct S_RAinfo));
	MAC_RAR_subhead_withbi *m_bi_subhead=(MAC_RAR_subhead_withbi*)fsm_mem_alloc(sizeof(MAC_RAR_subhead_withbi));
	MAC_RAR_sdu *m_rar_sdu=(MAC_RAR_sdu*)fsm_mem_alloc(sizeof(MAC_RAR_sdu));
	MAC_RAR_subhead *m_rar_subhead=(MAC_RAR_subhead*)fsm_mem_alloc(sizeof(MAC_RAR_subhead));
	DATA_RAR_PDU *m_rar_infor=(DATA_RAR_PDU*)fsm_mem_alloc(sizeof(DATA_RAR_PDU));
	MACtoPHYadapter_IciMsg *m_phy_ici=(MACtoPHYadapter_IciMsg*)fsm_mem_alloc(sizeof(MACtoPHYadapter_IciMsg));
	struct S_RAinfo m_rar_list;
	FSM_PKT *newskb;
	
	fsm_mem_set(m_bi_subhead,0,sizeof(MAC_RAR_subhead_withbi));
	fsm_mem_set(m_rar_sdu,0,sizeof(MAC_RAR_sdu));
	fsm_mem_set(m_rar_subhead,0,sizeof(MAC_RAR_subhead));
	fsm_mem_set(m_rar_infor,0,sizeof(DATA_RAR_PDU));
	fsm_mem_set(m_phy_ici,0,sizeof(m_phy_ici));
	SV_PTR_GET(rlc_mac_sv);
	newskb=fsm_pkt_create(MAX_LENGTH+REDUNDANCE);
	fsm_skb_reserve(newskb,RESERVE_LENGTH);

	//Do_Data_of_MacRarPdu_ulgrant(m_rar_infor,&m_rar_list);
	fsm_mem_cpy(&ue_numb,(char*)ue_infor,sizeof(u32));
	fsm_printf("[RLC_MAC][complexRarPdu]ue_numb:%d\n", ue_numb);
	//fsm_printf("[HEXI]RAR NUMBER:%d\n",ue_numb);
	/*fsm_mem_cpy(&m_rar_list,(struct S_RAinfo*)(ue_infor+infor_len),len);	//for test
	fsm_printf("[HEXI]RA INFO RNTI:%d\n",m_rar_list.ra_rnti);*/
	while(ue_numb>0){
		len=sizeof(struct S_RAinfo);
		fsm_mem_cpy(&m_rar_list,(S_RAinfo*)(ue_infor+infor_len),len);	//for test
		infor_len=infor_len+len;
		
		fsm_printf("[HEXI]Message%d index1 = %u,%u,%u,%u,%u\n",m_rar_list.index,m_rar_list.ra_rnti,m_rar_list.total_num,m_rar_list.cqi,m_rar_list.rapid);

		Do_Data_of_MacRarPdu_ulgrant(m_rar_infor,&m_rar_list,ue_numb);	//for test
		
		if(m_rar_infor->m_bi!=0){
			bi=m_rar_infor->m_bi;
		}
		
		fsm_mem_set(m_rar_subhead,0,sizeof(MAC_RAR_subhead));
		if(newskb->data == newskb->tail)
		{
			//fsm_printf("[HEXI] The first RAR\n");
			m_rar_subhead->m_e_t_rapid = m_rar_subhead->m_e_t_rapid&127;
		}
		else{
			m_rar_subhead->m_e_t_rapid = m_rar_subhead->m_e_t_rapid|128;
		}
		
		fsm_printf("[HEXI]rapid in rar:%d\n",m_rar_infor->m_rapid);
		m_rar_subhead->m_e_t_rapid=m_rar_subhead->m_e_t_rapid+m_rar_infor->m_rapid;
		//m_rar_subhead->m_e_t_rapid=25;	//for test
		
		m_rar_subhead->m_e_t_rapid=m_rar_subhead->m_e_t_rapid|64;
		//m_rar_subhead->m_e_t_rapid=m_rar_subhead->m_e_t_rapid|128;
		len=sizeof(MAC_RAR_subhead);
		fsm_mem_cpy(fsm_skb_push(newskb,len),m_rar_subhead,len);
		from_len=from_len+len;	//因为data指针向前移动了len
		//len=sizeof(DATA_RAR_PDU);
		fsm_mem_cpy(&ul_grant,&(m_rar_infor->s_ulgrant),sizeof(u32));
		m_rar_sdu->m_r_ta=m_rar_infor->m_ta>>4;
		m_rar_sdu->m_ta_ulgrant=(m_rar_infor->m_ta&15)<<4;
		m_rar_sdu->m_ta_ulgrant=m_rar_sdu->m_ta_ulgrant+(ul_grant&0xf0000000);
		m_rar_sdu->m_ulgrant=(ul_grant&0x0ffff000);
		m_rar_sdu->m_tcrnti=m_rar_infor->m_tcrnti;//for test
		fsm_printf("[HEXI]m_rar_infor->m_tcrnti = %d\n",m_rar_infor->m_tcrnti);
		//m_rar_sdu->m_tcrnti=61;	//for test
		len=sizeof(MAC_RAR_sdu);
		fsm_mem_cpy(fsm_skb_put(newskb,len),newskb->data+from_len,ttl_len);	//for test
		fsm_mem_cpy(newskb->data+from_len,m_rar_sdu,len);	//for test
		ttl_len=ttl_len+len;
		/*if(newskb->tail-newskb->data!=len){	//如果是第一次，from_len不自增，也就是from_len指示的是已经填好的最后一个sdu的开始位置
			from_len=from_len+len;
		}*///20150124gai
		//m_rar_subhead->m_e_t_rapid=m_rar_infor->m_rapid;
		//m_rar_subhead->m_e_t_rapid=m_rar_subhead->m_e_t_rapid|64;
		ue_numb--;
	}
	if(bi!=0){
	
		//fsm_printf("[HEXI]:THERE IS A BI SUBHEAD!\n");
	
		len=sizeof(MAC_RAR_subhead_withbi);
		m_bi_subhead->m_e_t_r_r_bi=bi;
		
		if(newskb->tail!=newskb->data){
			m_bi_subhead->m_e_t_r_r_bi=m_bi_subhead->m_e_t_r_r_bi|128;
		}
		
		/*if(newskb->data==newskb->tail){
			fsm_skb_put(newskb,len);
		}*/
		fsm_mem_cpy(fsm_skb_push(newskb,len),m_bi_subhead,len);
	}
	//len=sizeof(char);
	//fsm_mem_cpy(fsm_skb_push(newskb,len),&typ,len);	//delete typ in 20160122
	setMACtoPHY_IciMsg(m_phy_ici,0,getRA_rnti(ue_infor));	//FOR TEST

	fsm_printf("[HEXI]RNTI IN ICI TO SRIO:%d\n",m_phy_ici->rnti);	

	len=sizeof(MACtoPHYadapter_IciMsg);
	fsm_mem_cpy(newskb->head,m_phy_ici,len);

	//fsm_mem_cpy(m_phy_ici,newskb->head,sizeof(MACtoPHYadapter_IciMsg));
	//fsm_printf("[srio][packet_send_to_eth][--]rnti = %u.\n", m_phy_ici->rnti);
	fsm_printf("[HEXI]RAR SK_BUFF DATA: len = %d\n",newskb->len);
	/*for(i=0;i<3;i++){
		fsm_printf("%c ",*((char*)newskb->data+i));
	}
	fsm_printf("\n");*/
	//fsm_octets_print(newskb->data,newskb->len);

	fsm_printf("[HEXI]RAPID BEFORE SENDING:%c\n",*((char*)newskb->data+1)+1);

	fsm_pkt_send(newskb,STRM_TO_SRIO);
	
	fsm_mem_free(m_bi_subhead);
	fsm_mem_free(m_rar_sdu);
	fsm_mem_free(m_rar_subhead);
	fsm_mem_free(m_rar_infor);
	FOUT;
}



void hasContentionResolutionSend(MAC_CE_content_resolution_identity *m_content_resolution){
	FIN(hasContentionResolutionSend());
	u32 len=sizeof(temp_contention_resolution);
	SV_PTR_GET(rlc_mac_sv);

	fsm_printf("[HEXI]CONTENTION RESOLUTION PART1: %d, PART2: %d\n", SV(m_temp_cr)->m_part_1, SV(m_temp_cr)->m_part_2);
	fsm_mem_cpy(m_content_resolution,SV(m_temp_cr),len);
	
	u32 i;
	fsm_printf("[HEXI]SKB->DATA BEFORE SEND:\n");
	for(i=0;i<sizeof(MAC_CE_content_resolution_identity);i++){
		fsm_printf("%c ",*((char*)m_content_resolution+i)+65);	
	}
	fsm_printf("\n");
	
	FOUT;
}

u32 hasTimeAdvanceSend(u32 rnti){
	FIN(hasTimeAdvanceSend());
	u32 pos;
	//fsm_printf("[HEXI]hasTimeAdvanceSend RNTI:%d\n",rnti);
	SV_PTR_GET(rlc_mac_sv);
	for(pos=0;pos<DL_SCHEDULE_NUM;pos++){

		//fsm_printf("[HEXI]RNTI IN UE_TA:%d ",SV(ue_ta[pos]).rnti);

		if(SV(ue_ta[pos]).rnti==rnti&&SV(ue_ta[pos]).update_flag){
			
			//fsm_printf("[HEXI]THERE IS TA CE:%d\n",SV(ue_ta[pos]).ta);

			SV(ue_ta[pos]).update_flag=0;
			FRET(SV(ue_ta[pos]).ta);
		}
	}
	FRET(0);
}

void setMACtoPHY_IciMsg(MACtoPHYadapter_IciMsg *ici_msg,u32 tcid,u16 rnti){	//20140430改 填写MACtoRLC_IciMsg
	FIN(setMACtoRLC_IciMsg());
	SV_PTR_GET(rlc_mac_sv);
	u32 pos;
	ici_msg->tcid=tcid;
	ici_msg->rnti=rnti;
	for(pos=0;pos<DL_SCHEDULE_NUM;pos++){
		if(SV(schedule_scheme[pos]).m_rnti==rnti){
			ici_msg->subframeNo=SV(schedule_scheme[pos]).subframeN;
			ici_msg->frameNo=SV(schedule_scheme[pos]).sfn;
			break;
		}
	}
	//更多成员的填充待定
	FOUT;
}

u32 getTBsize(u32 rnti){
	FIN(getTBsize());
	u32 tb_size=0;
	u32 pos;

	SV_PTR_GET(rlc_mac_sv);
	for(pos=0;pos<DL_SCHEDULE_NUM;pos++)
		if(SV(schedule_scheme[pos]).m_rnti==rnti){
			tb_size=SV(schedule_scheme[pos]).m_data_size;
			SV(schedule_scheme[pos]).m_data_size=0;
			break;
	}

	FRET(tb_size);
}

u32 compare(u32 first,u32 second){
	u32 ret=first<second?0:1;
	return ret;
}



void complexEnbMacPdu(FSM_PKT *skb){
	FIN(complexEnbMacPdu());
	static u32 cnt=0;
	/*if(cnt)	//for test
		FOUT;//for test

	fsm_printf("[HEXI]COMPLEX COUNT:%d\n",cnt);
	cnt++;	//for test*/

	//u32 continue_flag=1;
	u32 lcid,timing,f_domain,rnti;
	u32 ce_ta=0;
	u32 part1;
	short part2;
	u32 numb=0,from_len=0,to_len=0,head_len;
	bool cri_flag=false,ta_flag=false,continue_flag=false;
	FSM_PKT *newskb;
	
	/*MAC_SDU_subhead_15bit *m_15bit_subhead=(MAC_SDU_subhead_15bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_15bit));
	MAC_SDU_subhead_last *m_fixed_subhead=(MAC_SDU_subhead_last*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_last));
	MAC_SDU_subhead_7bit *m_7bit_subhead=(MAC_SDU_subhead_7bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));
	MAC_SH_padding *m_padding_subhead=(MAC_SH_padding*)fsm_mem_alloc(sizeof(MAC_SH_padding));
	MAC_CE_content_resolution_identity *m_content_resolution=(MAC_CE_content_resolution_identity*)fsm_mem_alloc(sizeof(MAC_CE_content_resolution_identity));
	MAC_CE_time_advance *m_time_advance=(MAC_CE_time_advance*)fsm_mem_alloc(sizeof(MAC_CE_time_advance));
	RLCtoMAC_IciMsg *rlc_ici=(RLCtoMAC_IciMsg*)fsm_mem_alloc(sizeof(RLCtoMAC_IciMsg));
	MACtoPHYadapter_IciMsg *ici_msg=(MACtoPHYadapter_IciMsg*)fsm_mem_alloc(sizeof(MACtoPHYadapter_IciMsg));
	//u32 len=sizeof(_attribute_((packed))ICI_to_PHYadapter);
	u32 len=0,typ_rnti;
	short ici_rnti;
	char m_type=2;
	u32 tb_size=0;//20141011加


	

	SV_PTR_GET(rlc_mac_sv);
	fsm_mem_set(m_15bit_subhead,0,sizeof(MAC_SDU_subhead_15bit));
	fsm_mem_set(m_fixed_subhead,0,sizeof(MAC_SDU_subhead_last));
	fsm_mem_set(m_7bit_subhead,0,sizeof(MAC_SDU_subhead_7bit));
	fsm_mem_set(m_padding_subhead,0,sizeof(MAC_SH_padding));
	fsm_mem_set(m_content_resolution,0,sizeof(MAC_CE_content_resolution_identity));
	fsm_mem_set(m_time_advance,0,sizeof(MAC_CE_time_advance));
	fsm_mem_set(rlc_ici,0,sizeof(RLCtoMAC_IciMsg));
	fsm_mem_set(ici_msg,0,sizeof(MACtoPHYadapter_IciMsg));*/



	MAC_SDU_subhead_15bit m_15bit_subhead;
	MAC_SDU_subhead_last m_fixed_subhead;
	MAC_SDU_subhead_7bit m_7bit_subhead;
	MAC_SH_padding m_padding_subhead;
	MAC_CE_content_resolution_identity m_content_resolution;
	MAC_CE_time_advance m_time_advance;
	RLCtoMAC_IciMsg rlc_ici;
	MACtoPHYadapter_IciMsg ici_msg;
	//u32 len=sizeof(_attribute_((packed))ICI_to_PHYadapter);
	u32 len=0,typ_rnti;
	short ici_rnti;
	//char m_type=2;	//delete m_type in 20160122
	u32 tb_size=0;//20141011加


	

	SV_PTR_GET(rlc_mac_sv);
	fsm_mem_set(&m_15bit_subhead,0,sizeof(MAC_SDU_subhead_15bit));
	fsm_mem_set(&m_fixed_subhead,0,sizeof(MAC_SDU_subhead_last));
	fsm_mem_set(&m_7bit_subhead,0,sizeof(MAC_SDU_subhead_7bit));
	fsm_mem_set(&m_padding_subhead,0,sizeof(MAC_SH_padding));
	fsm_mem_set(&m_content_resolution,0,sizeof(MAC_CE_content_resolution_identity));
	fsm_mem_set(&m_time_advance,0,sizeof(MAC_CE_time_advance));
	fsm_mem_set(&rlc_ici,0,sizeof(RLCtoMAC_IciMsg));
	fsm_mem_set(&ici_msg,0,sizeof(MACtoPHYadapter_IciMsg));


	//fsm_mem_cpy(fsm_skb_push(skb,len),&m_type,len);
	//ta_flag=hasTimeAdvanceSend(m_ue_infor);

	/*if(ta_flag){
		time_advance->m_r_r_ta=m_ue_infor->ta;
		len=sizeof(MAC_CE_time_advance);
		fsm_mem_cpy(fsm_skb_push(skb,len),time_advance,len);
	}*/
	/*if(cri_flag){
		len=sizeof(MAC_CE_content_resolution_identity);
		fsm_mem_cpy(m_content_resolution,SV(m_temp_cr),len);
		fsm_mem_cpy(fsm_skb_push(skb,len),m_content_resolution,len);
	}*/
	u32 i;
	fsm_printf("[HEXI]SKB->DATA AFTER PUSH:\n");
	for(i=0;i<12;i++){
		fsm_printf("%c ",*(skb->data+i));	
	}
	fsm_printf("\n");

	SV(total_pktnum_downlink)=SV(total_pktnum_downlink)+1;	//for test


	len=sizeof(RLCtoMAC_IciMsg);
	if(!compare(skb->data-skb->head,len)){
		SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
		fsm_pkt_destroy(skb);
		FOUT;
	}
	fsm_mem_cpy(&rlc_ici,skb->head,len);
	head_len=rlc_ici.len;
	
	if(!compare(skb->data-skb->head-len,sizeof(MAC_SDU_subhead_15bit))){
		SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
		fsm_pkt_destroy(skb);
		FOUT;
	}
	fsm_mem_cpy(&m_15bit_subhead,skb->head+len,sizeof(MAC_SDU_subhead_15bit));
	fsm_printf("[HEXI]LENGTH OF HEAD IN ICI FROM RLC AND L DOMAIN IN 15 BIT SUBHEAD:%d,%d\n",head_len,(m_15bit_subhead.m_l)>>7);
	fsm_octets_print(&(m_15bit_subhead.m_f_l),2);
	
	ici_rnti=rlc_ici.rnti;
	typ_rnti=Do_judge_rnti(ici_rnti);	//funtion is qufen type

	fsm_printf("[RLC_MAC][complexEnbMacPdu]ici_rnti: %d\n", ici_rnti);

	//if(SV(cnt_test)>1)
	//typ_rnti=C_RNTI;	//for test
	//SV(cnt_test)++;

	tb_size=getTBsize(ici_rnti);
	//tb_size=65536; //for test
	tb_size=1530;//for test 20150105
	
	if(head_len+(skb->tail-skb->data)>tb_size){
		
		SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
		fsm_pkt_destroy(skb);

		FOUT;
	}
	
	if(len>0){
		continue_flag=true;
	}
	fsm_printf("[HEXI]TB_SIZE AND PBCH IN THE ICI:%d,%d\n",tb_size,rlc_ici.rnti);//for test
	if(typ_rnti==CRNTI_TYPE){
		//fsm_printf("[HEXI]COMPLEX C_RNTI!\n");

		fsm_printf("[HEXI]rlc_ici->pbCh=%d\n",rlc_ici.pbCh);

		if(rlc_ici.pbCh==PCH){
		setMACtoPHY_IciMsg(&ici_msg,PCCH,ici_rnti);
		len=sizeof(MACtoPHYadapter_IciMsg);
		if(len>tb_size){
		
			SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
			fsm_pkt_destroy(skb);
			FOUT;
		}
		
		if(!compare(skb->data-skb->head,len)){
			SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
			fsm_pkt_destroy(skb);
			FOUT;
		}

		fsm_mem_cpy(skb->head,&ici_msg,len);	//将MAC TO RLC的ICI信息放置到SK_buff中
		fsm_pkt_send(skb,STRM_TO_SRIO);
		
		//printk("[RLC_MAC]countSendToLower:%d\n", ++SV(newcountsendtoLower));
		FOUT;
		}
		else{
			if(rlc_ici.pbCh==BCH){
				setMACtoPHY_IciMsg(&ici_msg,BCCH,ici_rnti);
				len=sizeof(MACtoPHYadapter_IciMsg);
				if(len>tb_size){
				
					SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
					fsm_pkt_destroy(skb);
				
					FOUT;
				}
			
				if(!compare(skb->data-skb->head,len)){
					SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(skb->head,&ici_msg,len);	//将MAC TO RLC的ICI信息放置到SK_buff中
				fsm_pkt_send(skb,STRM_TO_SRIO);
				
				//printk("[RLC_MAC]countSendToLower:%d\n", ++SV(newcountsendtoLower));
				FOUT;
			}
		}

		tb_size=tb_size-(skb->tail-skb->data-head_len);	//get rid of the length of data
		fsm_printf("[HEXI]TB_SIZE AFTER GETTING RID OF THE SDUS:%d\n",tb_size);//for test

		ce_ta=hasTimeAdvanceSend(ici_rnti);	//for test
		if(ce_ta){
			m_time_advance.m_r_r_ta=ce_ta;
			len=sizeof(MAC_CE_time_advance);
		
			//fsm_printf("[HEXI]CE_TA!\n");

			if(tb_size>=len+sizeof(MAC_SH_padding)){
				//fsm_printf("[HEXI]TA TB_SIZE!\n");

				if(!compare(skb->data-skb->head,len)){
					SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(fsm_skb_push(skb,len),&m_time_advance,len);
				tb_size=tb_size-len;
			}
			else
				ce_ta=0;
		}
	}
	else{
		if(typ_rnti==T_C_RNTI){
			//fsm_printf("[HEXI]COMPLEX T_C_RNTI!\n");
			hasContentionResolutionSend(&m_content_resolution);
			len=sizeof(MAC_CE_content_resolution_identity);
			if(tb_size>=len+sizeof(MAC_SH_padding)){
				cri_flag=true;
				
				fsm_printf("[HEXI]CONTENT IN CONTENTOIN RESOLUTION CE AND HEAD LENGTH:%d,%d,%d\n",m_content_resolution.m_resolusion_identity_1,m_content_resolution.m_resolusion_identity_2,(skb->data-skb->head));
				
				
				if(!compare(skb->data-skb->head,len)){
					SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(fsm_skb_push(skb,len),&m_content_resolution,len);
				tb_size=tb_size-len;

			}
			else{
				cri_flag=false;	//有这种情况?
				
				fsm_pkt_destroy(skb);

				SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;


				FOUT;
			}
		}
		else{
			//fsm_printf("[HEXI]COMPLEX T_C_RNTI 's else\n");

			SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;	//FOR TEST


			fsm_pkt_destroy(skb);

			FOUT;
		}
	}

	len=sizeof(RLCtoMAC_IciMsg);

	fsm_printf("[HEXI]SKB->DATA BEFORE PUSH:\n");
	for(i=0;i<3;i++){
		fsm_printf("%c ",*(skb->head+len+i)+65);	
	}
	fsm_printf("\n");

	
	if(!compare(skb->data-skb->head-len,head_len)){
		SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
		fsm_pkt_destroy(skb);
		FOUT;
	}



	fsm_mem_cpy(fsm_skb_push(skb,head_len),skb->head+len,head_len);

	fsm_printf("[HEXI]SKB->DATA AFTER PUSH:\n");
	for(i=0;i<head_len;i++){
		fsm_printf("%c ",*(skb->data+i)+65);	
	}
	fsm_printf("\n");
	/*for(i=head_len;i<head_len+12;i++){
		fsm_printf("%c ",*(skb->data+i));
	}
	fsm_printf("\n");*/

	if(ce_ta){
		//fsm_printf("[HEXI]TA CONTROL ELEMENT!\n");//for test

		m_fixed_subhead.m_lcid_e_r_r=TIMING_ADVANCE_LCID;
		if(skb->tail!=skb->head||continue_flag){
			m_fixed_subhead.m_lcid_e_r_r=m_fixed_subhead.m_lcid_e_r_r|32;
		}
		else{
			m_fixed_subhead.m_lcid_e_r_r=m_fixed_subhead.m_lcid_e_r_r&31;
		}
		len=sizeof(MAC_SDU_subhead_last);

		if(!compare(skb->data-skb->head-len,len)){
			SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
			fsm_pkt_destroy(skb);
			FOUT;
		}

		fsm_mem_cpy(fsm_skb_push(skb,len),&m_fixed_subhead,len);
		continue_flag=true;
	}
	if(cri_flag){
		
		convert_TcRntiToCRnti(ici_rnti);
		
		m_fixed_subhead.m_lcid_e_r_r=CONTENTION_RESOLUTION_LCID;
		if(skb->tail!=skb->head||continue_flag){
			m_fixed_subhead.m_lcid_e_r_r=m_fixed_subhead.m_lcid_e_r_r|32;
		}
		else{
			m_fixed_subhead.m_lcid_e_r_r=m_fixed_subhead.m_lcid_e_r_r&31;
		}
		len=sizeof(MAC_SDU_subhead_last);

		if(!compare(skb->data-skb->head-len,len)){
			SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
			fsm_pkt_destroy(skb);
			FOUT;
		}

		fsm_mem_cpy(fsm_skb_push(skb,len),&m_fixed_subhead,len);
		continue_flag=true;

		//SV(ue_num)++;		//CRA success, increment the ue_num, 20151228;
		printk("[RLC_MAC], currently ue nember is %d\n", SV(ue_num));
	}
	len=sizeof(char);

	if(!compare(skb->data-skb->head-len,head_len)){
		SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
		fsm_pkt_destroy(skb);
		FOUT;
	}


	//fsm_mem_cpy(fsm_skb_push(skb,len),&m_type,len); //delete m_type in 20160122
	fsm_printf("[HEXI]Type in mac is %c\n", *((char*)skb->data)+65);
	setMACtoPHY_IciMsg(&ici_msg,0,ici_rnti);
	len=sizeof(MACtoPHYadapter_IciMsg);

	if(!compare(skb->data-skb->head,len)){
		SV(lost_pktnum_downlink)=SV(lost_pktnum_downlink)+1;
		fsm_pkt_destroy(skb);
		FOUT;
	}

	fsm_mem_cpy(skb->head,&ici_msg,len);

	//static u32 cnt=0;

	//if(!cnt)	//for test
	
	fsm_printf("[HEXI]SKB->DATA BEFORE SEND : data len = %d\n",skb->len);
	//fsm_octets_print(skb->data,skb->len);
	/*for(i=0;i<10;i++){
		fsm_printf("%c ",*(skb->data+i)+65);	
	}
	fsm_printf("\n");*/
	
	fsm_pkt_send(skb,STRM_TO_SRIO);//for test
	//printk("[RLC_MAC]countSendToLower:%d\n", ++SV(newcountsendtoLower));
	//fsm_printf("[HEXI]COMPLEX COUNT:%d\n",cnt);
	//cnt++;	//for test


	FOUT;
}

void setMACtoRLC_IciMsg(MACtoRLC_IciMsg *ici_msg,u32 lcid,u32 rnti){	//20140430改 填写MACtoRLC_IciMsg
	FIN(setMACtoRLC_IciMsg());

	ici_msg->lcid=lcid;
	ici_msg->rnti=rnti;	
	FOUT;
}

void dealCcchSdu(FSM_PKT *skb,u32 offset){
	FIN(dealCcchSdu());

	u32 len=0,from_len=0;
	char part_8bit;
	u32 l_domain;

	u32 cntt1,cntt2;

	RRCConnectionRequest *m_rrc_request=(RRCConnectionRequest*)fsm_mem_alloc(sizeof(RRCConnectionRequest));
	MAC_SDU_subhead_15bit *m_15bit_subhead=(MAC_SDU_subhead_15bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_15bit));	//20140430改
	MAC_SDU_subhead_7bit *m_7bit_subhead=(MAC_SDU_subhead_7bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));	//20140430改
	SV_PTR_GET(rlc_mac_sv);

	fsm_mem_set(m_rrc_request,0,sizeof(RRCConnectionRequest));
	fsm_mem_set(m_15bit_subhead,0,sizeof(MAC_SDU_subhead_15bit));
	fsm_mem_set(m_7bit_subhead,0,sizeof(MAC_SDU_subhead_7bit));
	if(SV(m_temp_cr)==NULL){
		SV(m_temp_cr)=(temp_contention_resolution *)fsm_mem_alloc(sizeof(temp_contention_resolution));	//20140922改
	}

	/*len=sizeof(RRCConnectionRequest);
	fsm_mem_cpy(m_rrc_req,skb->data+offset,len);
	len=sizeof(m_rrc_req->ue_Identity);
	fsm_mem_cpy(SV(m_temp_cr),&(m_rrc_req->ue_Identity),len);
	from_len=from_len+len;
	len=sizeof(char);
	fsm_mem_cpy(&part_8bit,&(m_rrc_req->establishmentCause),len);
	fsm_mem_cpy(SV(m_temp_cr)+from_len,&part_8bit,len);*/
	len=sizeof(MAC_SDU_subhead_7bit);

	fsm_mem_cpy(m_7bit_subhead,skb->data+offset,len);
	l_domain=m_7bit_subhead->m_f_l>>7;
	if(l_domain==1)
		len=sizeof(MAC_SDU_subhead_15bit);
	
	fsm_printf("[HEXI]THE OFFSET AND THE LEN:%d,%d\n",offset,len);	
	//offset=offset+len;

	len=sizeof(RRCConnectionRequest);
	//offset=offset+sizeof(u32); //ignore the type domain
	fsm_mem_cpy(m_rrc_request,skb->data+offset,len);
	len=sizeof(SV(m_temp_cr)->m_part_1);

	fsm_printf("[HEXI]UE ID AND ESTABLUSHMENT CAUSE AND THE OFFSET:%d,%d,%d\n",(m_rrc_request->ue_Identity.s_TMSI.mmec+m_rrc_request->ue_Identity.s_TMSI.m_TMSI),m_rrc_request->establishmentCause,offset);
	//fsm_printf("[HEXI]UE ID AND ESTABLUSHMENT CAUSE AND THE OFFSET:%d,%d,%d\n",m_rrc_request->ue_Identity.s_TMSI.m_TMSI,m_rrc_request->establishmentCause,offset);

	fsm_mem_cpy(&(SV(m_temp_cr)->m_part_1),&(m_rrc_request->ue_Identity),len+1);
	
	fsm_printf("[HEXI]SV(m_temp_cr)->m_part_1=%d\n",SV(m_temp_cr)->m_part_1);
	
	fsm_mem_set(&(SV(m_temp_cr)->m_part_2),0,sizeof(u16));
	fsm_mem_cpy(&(SV(m_temp_cr)->m_part_2),&(m_rrc_request->ue_Identity),1);
	
	fsm_printf("[HEXI]SV(m_temp_cr)->m_part_2=%d\n",SV(m_temp_cr)->m_part_2);
	
	SV(m_temp_cr)->m_part_2=SV(m_temp_cr)->m_part_2+(m_rrc_request->establishmentCause&0x000f)<<8;
	
	fsm_printf("[HEXI]SV(m_temp_cr)->m_part_2=%d\n",SV(m_temp_cr)->m_part_2);
	

	fsm_mem_free(m_rrc_request);
	fsm_mem_free(m_15bit_subhead);
	fsm_mem_free(m_7bit_subhead);
	FOUT;
}


u32 lteMacGetUlHeadLength(FSM_PKT *skb){
	FIN(lteMacGetUlHeadLength());

	u32 len=0,head_len=0;
	u32 continue_flag=1;
	u32 m_lcid=-1,f_domain=-1;
	MAC_SDU_subhead_15bit *m_15bit_subhead=(MAC_SDU_subhead_15bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_15bit));	//20140430改
	MAC_SDU_subhead_last *m_fixed_subhead=(MAC_SDU_subhead_last*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_last));	//20140430改
	MAC_SDU_subhead_7bit *m_7bit_subhead=(MAC_SDU_subhead_7bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));	//20140430改
	MAC_SH_padding *m_padding_subhead=(MAC_SH_padding*)fsm_mem_alloc(sizeof(MAC_SH_padding));	//20140430改
	MAC_CE_power_headroom *m_power_headroom=(MAC_CE_power_headroom*)fsm_mem_alloc(sizeof(MAC_CE_power_headroom));	//20140430改
	MAC_CE_Crnti *m_c_rnti=(MAC_CE_Crnti*)fsm_mem_alloc(sizeof(MAC_CE_Crnti));	//20140430改
	MACtoRLC_IciMsg *m_rlc_type2_icimsg=(MACtoRLC_IciMsg*)fsm_mem_alloc(sizeof(MACtoRLC_IciMsg));		//20140430改	//20140430改
	
	MAC_CE_shortBSR *m_short_bsr=(MAC_CE_shortBSR*)fsm_mem_alloc(sizeof(MAC_CE_shortBSR));
	MAC_CE_longBSR *m_long_bsr=(MAC_CE_longBSR*)fsm_mem_alloc(sizeof(MAC_CE_longBSR));
	
	fsm_mem_set(m_15bit_subhead,0,sizeof(MAC_SDU_subhead_15bit));
	fsm_mem_set(m_fixed_subhead,0,sizeof(MAC_SDU_subhead_last));
	fsm_mem_set(m_7bit_subhead,0,sizeof(MAC_SDU_subhead_7bit));
	fsm_mem_set(m_padding_subhead,0,sizeof(MAC_SH_padding));
	fsm_mem_set(m_power_headroom,0,sizeof(MAC_CE_power_headroom));
	fsm_mem_set(m_c_rnti,0,sizeof(MAC_SDU_subhead_15bit));
	fsm_mem_set(m_rlc_type2_icimsg,0,sizeof(MACtoRLC_IciMsg));
	fsm_mem_set(m_short_bsr,0,sizeof(MAC_CE_shortBSR));
	fsm_mem_set(m_long_bsr,0,sizeof(MAC_CE_longBSR));

	len=sizeof(MAC_SDU_subhead_last);
	fsm_mem_cpy(m_fixed_subhead,skb->data,len);
	//continue_flag=m_fixed_subhead->m_lcid_e_r_r>>5;
	/*if(continue_flag==1){
		len=sizeof(MAC_SDU_subhead_last);
		memcpy(m_fixed_subhead,skb->data+head_len,len);
		head_len=head_len+len;
		continue_flag=m_fixed_subhead->m_lcid_e_r_r>>5;
	}*/
	while(continue_flag==1){
		len=sizeof(MAC_SDU_subhead_last);
		fsm_mem_cpy(m_fixed_subhead,skb->data+head_len,len);
		m_lcid=m_fixed_subhead->m_lcid_e_r_r&31;
		continue_flag=m_fixed_subhead->m_lcid_e_r_r>>5;

		fsm_printf("[HEXI]HEAD LEN LCID:%d\n",m_lcid);
		if(m_lcid>=26&&m_lcid<32){
			head_len=head_len+len;
		}
		else{
			len=sizeof(MAC_SDU_subhead_7bit);
			fsm_mem_cpy(m_7bit_subhead,skb->data+head_len,len);
			f_domain=m_7bit_subhead->m_f_l>>7;
			if(f_domain==1){
				len=sizeof(MAC_SDU_subhead_15bit);
				//fsm_mem_cpy(m_15bit_subhead,skb->data+head_len,len);
				head_len=head_len+len;
				fsm_printf("[HEXI]15BIT HEAD LEN :%d\n",head_len);
			}
			else{
				head_len=head_len+len;
				fsm_printf("[HEXI]7BIT HEAD LEN :%d\n",head_len);
			}	//else
		}		//else
	}			//while



	fsm_mem_free(m_15bit_subhead);
	fsm_mem_free(m_7bit_subhead);
	fsm_mem_free(m_fixed_subhead);
	fsm_mem_free(m_padding_subhead);
	fsm_mem_free(m_power_headroom);
	fsm_mem_free(m_c_rnti);
	fsm_mem_free(m_rlc_type2_icimsg);
	fsm_mem_free(m_short_bsr);
	fsm_mem_free(m_long_bsr);
	FRET(head_len);
}



void decomplexEndMacPdu(FSM_PKT *skb){
	FIN(decomplexEndMacPdu());

	u32 continue_flag=1,padding_flag;
	u32 lcid,f_domain,typ_rnti;
	u16 c_rnti=0,ici_rnti;

	u32 temp,buff_size=0;
	u32 numb=0,from_len=0,to_len=0,head_len,sdu_len=0,ue_numb=0;
	FSM_PKT *newskb;
	/*MAC_SDU_subhead_15bit *m_15bit_subhead=(MAC_SDU_subhead_15bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_15bit));	//20140430改
	MAC_SDU_subhead_last *m_fixed_subhead=(MAC_SDU_subhead_last*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_last));	//20140430改
	MAC_SDU_subhead_7bit *m_7bit_subhead=(MAC_SDU_subhead_7bit*)fsm_mem_alloc(sizeof(MAC_SDU_subhead_7bit));	//20140430改
	MAC_SH_padding *m_padding_subhead=(MAC_SH_padding*)fsm_mem_alloc(sizeof(MAC_SH_padding));	//20140430改
	MAC_CE_power_headroom *m_power_headroom=(MAC_CE_power_headroom*)fsm_mem_alloc(sizeof(MAC_CE_power_headroom));	//20140430改
	MAC_CE_Crnti *m_c_rnti=(MAC_CE_Crnti*)fsm_mem_alloc(sizeof(MAC_CE_Crnti));	//20140430改
	MACtoRLC_IciMsg *m_rlc_type2_icimsg=(MACtoRLC_IciMsg*)fsm_mem_alloc(sizeof(MACtoRLC_IciMsg));		//20140430改	//20140430改
	
	PHYadaptertoMAC_IciMsg *phy_ici=(PHYadaptertoMAC_IciMsg*)fsm_mem_alloc(sizeof(PHYadaptertoMAC_IciMsg));
	MAC_CE_shortBSR *m_short_bsr=(MAC_CE_shortBSR*)fsm_mem_alloc(sizeof(MAC_CE_shortBSR));
	MAC_CE_longBSR *m_long_bsr=(MAC_CE_longBSR*)fsm_mem_alloc(sizeof(MAC_CE_longBSR));
	//struct S_UEinfo *m_ue_infor=(struct S_UEinfo*)fsm_mem_alloc(sizeof(struct S_UEinfo));
	PHYtoMAC_Info *m_ue_infor_from_phy=(PHYtoMAC_Info*)fsm_mem_alloc(sizeof(PHYtoMAC_Info));
	//u32 len=sizeof(_attribute_((packed))ICI_to_PHYadapter);
	u32 len=0;
	u32 i;

	fsm_mem_set(m_15bit_subhead,0,sizeof(MAC_SDU_subhead_15bit));	//20140430改
	fsm_mem_set(m_fixed_subhead,0,sizeof(MAC_SDU_subhead_last));	//20140430改
	fsm_mem_set(m_7bit_subhead,0,sizeof(MAC_SDU_subhead_7bit));	//20140430改
	fsm_mem_set(m_power_headroom,0,sizeof(MAC_CE_power_headroom));	//20140430改
	fsm_mem_set(m_c_rnti,0,sizeof(MAC_CE_Crnti));	//20140430改
	fsm_mem_set(m_rlc_type2_icimsg,0,sizeof(MACtoRLC_IciMsg));	//20140430改
	fsm_mem_set(m_padding_subhead,0,sizeof(MAC_SH_padding));	//20140430改*/
	
	MAC_SDU_subhead_15bit m_15bit_subhead;	//20140430改
	MAC_SDU_subhead_last m_fixed_subhead;	//20140430改
	MAC_SDU_subhead_7bit m_7bit_subhead;	//20140430改
	MAC_SH_padding m_padding_subhead;	//20140430改
	MAC_CE_power_headroom m_power_headroom;	//20140430改
	MAC_CE_Crnti m_c_rnti;	//20140430改
	MACtoRLC_IciMsg m_rlc_type2_icimsg;		//20140430改	//20140430改
	
	PHYadaptertoMAC_IciMsg phy_ici;
	MAC_CE_shortBSR m_short_bsr;
	MAC_CE_longBSR m_long_bsr;
	//struct S_UEinfo *m_ue_infor=(struct S_UEinfo*)fsm_mem_alloc(sizeof(struct S_UEinfo));
	PHYtoMAC_Info m_ue_infor_from_phy;
	//u32 len=sizeof(_attribute_((packed))ICI_to_PHYadapter);
	u32 len=0;
	u32 i;

	fsm_mem_set(&m_15bit_subhead,0,sizeof(MAC_SDU_subhead_15bit));	//20140430改
	fsm_mem_set(&m_fixed_subhead,0,sizeof(MAC_SDU_subhead_last));	//20140430改
	fsm_mem_set(&m_7bit_subhead,0,sizeof(MAC_SDU_subhead_7bit));	//20140430改
	fsm_mem_set(&m_power_headroom,0,sizeof(MAC_CE_power_headroom));	//20140430改
	fsm_mem_set(&m_c_rnti,0,sizeof(MAC_CE_Crnti));	//20140430改
	fsm_mem_set(&m_rlc_type2_icimsg,0,sizeof(MACtoRLC_IciMsg));	//20140430改
	fsm_mem_set(&m_padding_subhead,0,sizeof(MAC_SH_padding));
	
	
	
	
	SV_PTR_GET(rlc_mac_sv);
	head_len=lteMacGetUlHeadLength(skb);	//已解SDU的位置
	fsm_printf("[HEXI]UL HEAD LEN:%d\n",head_len);

	fsm_printf("[HEXI] SKB->DATA 7byte\n");
	//fsm_octets_print(skb->data, 7);

	fsm_printf("SKB->DATA\n");
	for(i=0;i<5;i++){
		fsm_printf("%c ",*(skb->data+i)+65);
	}
	fsm_printf("\n");
	//memcpy(m_type,skb->data,len);
	from_len=len;	//(头部)已被解复用的长度
	len=sizeof(PHYadaptertoMAC_IciMsg);

	if(!compare(skb->data-skb->head,len)){
		SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
		fsm_pkt_destroy(skb);
		FOUT;
	}

	fsm_mem_cpy(&phy_ici,skb->head,len);
	ici_rnti=phy_ici.rnti;
	//m_ue_infor_from_phy=phy_ici->ue_info;
	fsm_mem_cpy(&m_ue_infor_from_phy,&(phy_ici.ue_info),sizeof(PHYtoMAC_Info));
	
	fsm_printf("[HEXI]ici_rnti=%d\n",ici_rnti);
	
	typ_rnti=Do_judge_rnti(ici_rnti);//funtion is to qufen type
	//typ_rnti=C_RNTI;//测试用的	//FOR TEST

	SV(total_pktnum_uplink)=SV(total_pktnum_uplink)+1;


	if(typ_rnti==CRNTI_TYPE){
		fsm_printf("[HEXI]DECOMPLEX C_RNTI\n");
		while(continue_flag==1){
			len=sizeof(MAC_SH_padding);		//获取LCID

			if(!compare(skb->tail-skb->data-from_len,len)){
				SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
				fsm_pkt_destroy(skb);
				FOUT;
			}

			fsm_mem_cpy(&m_padding_subhead,skb->data+from_len,len);
			//from_len=from_len+len;

			continue_flag=m_padding_subhead.m_lcid_e_r_r>>5;
			lcid=(m_padding_subhead.m_lcid_e_r_r)&31;

			fsm_printf("[DECOMPLEX]LCID OF CONTROL ELEMENT:%d\n",lcid);	//FOR TEST
			if(lcid!=31){
			//if(lcid>10&&lcid<31){
				numb++;
				fsm_printf("[DECOMPLEX] lcid = %d\n",lcid);
				switch(lcid){
				case PHR_LCID:	//20140428 改
                
					break;

				case C_RNTI_LCID:
					len=sizeof(MAC_CE_Crnti);

					if(!compare(skb->tail-skb->data-head_len,len)){
						SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
						fsm_pkt_destroy(skb);
						FOUT;
					}

					fsm_mem_cpy(&m_c_rnti,skb->data+head_len,len);
					
					fsm_printf("[MAC DECOMPLEX]HEAD_LEN IN C_RNTI:%d\n",head_len);
					
					head_len=head_len+len;
					//处理C_RNTI
					SV(RAR_receive_crnti)=1;
					c_rnti=m_c_rnti.m_crnti;
			//		c_rnti = 62;  //for test 20141217
					fsm_printf("[DECOMPLEX] run to C_RNTI_LCID\n");
					Do_judge_rnti(c_rnti);

					phy_ici.rnti=c_rnti;

					break;
				case SHORT_BSR:
					len=sizeof(MAC_CE_shortBSR);

					if(!compare(skb->tail-skb->data-head_len,len)){
						SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
						fsm_pkt_destroy(skb);
						FOUT;
					}


					fsm_mem_cpy(&m_short_bsr,skb->data+head_len,len);
					head_len=head_len+len;
					//处理短BSR
					buff_size=(m_short_bsr.m_lcgid_buffersize)&63;
					printk("[RLC_MAC][decomplexEndMacPdu]short bsr buff_size: %d\n", buff_size);
					
					Add_sueinfo_Node(&m_ue_infor_from_phy,buff_size,0);//for test

					break;
				case LONG_BSR:
					len=sizeof(MAC_CE_shortBSR);

					if(!compare(skb->tail-skb->data-head_len,len)){
						SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
						fsm_pkt_destroy(skb);
						FOUT;
					}


					fsm_mem_cpy(&m_long_bsr,skb->data+head_len,len);
					head_len=head_len+len;
					//处理长BSR
					buff_size=m_long_bsr.m_buffersize1;
					buff_size+=(m_long_bsr.m_buffersize2<<8);
					printk("[RLC_MAC][decomplexEndMacPdu]long bsr buff_size: %d\n", buff_size);
	
					fsm_printf("[HEXI]RNTI IN DECOMPLEX:%d\n",ici_rnti);

					Add_sueinfo_Node(&m_ue_infor_from_phy,buff_size,1);//for test
					

					break;
				case PADDING_LCID:
					//from_len=from_len+len;
					break;
				default:
					continue_flag=0;
					from_len=from_len-sizeof(MAC_SH_padding);//after switch from_len will be added which shouldn't be added
					break;
					
				}	//switch
				len=sizeof(MAC_SH_padding);
				from_len=from_len+len;
			}		//if
		}			//while
		fsm_printf("[DECOMPLEX]CONTROL ELEMENTS:%d\n",from_len);			
		fsm_printf("[DECOMPLEX]lcid=%d, c_rnti = %d\n",lcid, c_rnti);			
		

		if(lcid>=0&&lcid<11){
			continue_flag=1;
			while(continue_flag==1){
				if(lcid==0&&!c_rnti){
					////
					//以IOctl的方式告诉RRC层该rnti
					fsm_do_ioctrl(STRM_TO_RRC, IOCCMD_MACtoRRC_RNTI_indicate, &ici_rnti, sizeof(ici_rnti));
					//fsm_printf("[DECOMPLEX] IOCCMD_MACtoRRC_RNTI_indicate, rnti = %d\n ", ici_rnti);			
					////

					dealCcchSdu(skb,head_len);
				}

				len=sizeof(MAC_SDU_subhead_7bit);

				if(!compare(skb->tail-skb->data-from_len,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}


				fsm_mem_cpy(&m_7bit_subhead,skb->data+from_len,len);
				continue_flag=m_7bit_subhead.m_lcid_e_r_r>>5;
				lcid=(m_7bit_subhead.m_lcid_e_r_r)&31;
				f_domain=m_7bit_subhead.m_f_l>>7;
				if(f_domain==1){	//15bit子头		20140430改
					len=sizeof(MAC_SDU_subhead_15bit);

					if(!compare(skb->tail-skb->data-from_len,len)){
						SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
						fsm_pkt_destroy(skb);
						FOUT;
					}


					fsm_mem_cpy(&m_15bit_subhead,skb->data+from_len,len);
					
					setMACtoRLC_IciMsg(&m_rlc_type2_icimsg,lcid,phy_ici.rnti);	//设置ICI消息	

					from_len=from_len+len;

					len=(m_15bit_subhead.m_f_l)&127;
					len=len<<8;
					len=len+m_15bit_subhead.m_l;
					newskb=fsm_pkt_create(len+REDUNDANCE);
					//fsm_skb_put(newskb,len+1);
					fsm_skb_put(newskb,len);  //for test 20150126??????????

					if(!compare(skb->tail-skb->data-head_len,len)||!compare(newskb->tail-newskb->data,len)){
						SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
						fsm_pkt_destroy(skb);
						FOUT;
					}
					

					fsm_mem_cpy(newskb->data,skb->data+head_len,len);	//for test
					//for test//
					/*fsm_printf("[HEXI]RNTI IN PHY_ICI=%d,RNTI TO RLC=%d\n.DATA IN SK_BUFF!\n",phy_ici->rnti,m_rlc_type2_icimsg->rnti);
					for(i=0;i<14;i++){
						fsm_printf("%c ",*(newskb->data+i));
					}					
					fsm_printf("\n");*/
					//end//


					head_len=head_len+len;
					len=sizeof(MACtoRLC_IciMsg);

					if(!compare(newskb->data-newskb->head,len)){
						SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
						fsm_pkt_destroy(skb);
						FOUT;
					}

					fsm_mem_cpy(newskb->head,&m_rlc_type2_icimsg,len);	//这个动作对？

					//fsm_pkt_send(newskb,STRM_TO_RLC);
					
					rlc_pkt_receive_from_mac(newskb);  //add in 20150804
					//fsm_pkt_send(newskb,STRM_TO_IPADP);

					fsm_printf("[HEXI]send to rlc data4: len = %d\n",newskb->len);
				//fsm_octets_print(newskb->data,newskb->len);
					
				}
				else{		//7bit子头		//20140430改
					setMACtoRLC_IciMsg(&m_rlc_type2_icimsg,lcid,phy_ici.rnti);
					from_len=from_len+len;

					len=(m_7bit_subhead.m_f_l)&127;
					newskb=fsm_pkt_create(len+REDUNDANCE);
					//fsm_skb_put(newskb,len+1);
					fsm_skb_put(newskb,len);  //for test 20150126 ????????

					if(!compare(skb->tail-skb->data-head_len,len)||!compare(newskb->tail-newskb->data,len)){
						SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
						fsm_pkt_destroy(skb);
						FOUT;
					}


					fsm_mem_cpy(newskb->data,skb->data+head_len,len);	//for test
					//for test//
					//fsm_printf("[HEXI]RNTI IN PHY_ICI=%d,RNTI TO RLC=%d\n.DATA IN SK_BUFF!\n",phy_ici->rnti,m_rlc_type2_icimsg->rnti);
					/*for(i=0;i<14;i++){
						fsm_printf("%c ",*(newskb->data+i));
					}					
					fsm_printf("\n");*/
					//end//

					head_len=head_len+len;
					len=sizeof(MACtoRLC_IciMsg);

					if(!compare(newskb->data-newskb->head,len)){
						SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
						fsm_pkt_destroy(skb);
						FOUT;
					}

					fsm_mem_cpy(newskb->head,&m_rlc_type2_icimsg,len);

					//fsm_pkt_send(newskb,STRM_TO_RLC);
					rlc_pkt_receive_from_mac(newskb);  //add in 20150804
					

					//fsm_pkt_send(newskb,STRM_TO_IPADP);

					fsm_printf("[HEXI]send to rlc data3: len = %d\n",newskb->len);
					//fsm_octets_print(newskb->data,newskb->len);
				}
			}
		}
		
		/*
		 *for test
		 */
		else{
			SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
		}
	
	}

	else if(typ_rnti==T_C_RNTI){
		fsm_printf("[HEXI]DECOMPLEX T_C_RNTI\n");
		len=sizeof(MAC_SH_padding);		//获取LCID

		if(!compare(skb->tail-skb->data-from_len,len)){
			SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
			fsm_pkt_destroy(skb);
			FOUT;
		}

		fsm_mem_cpy(&m_padding_subhead,skb->data+from_len,len);
		//from_len=from_len+len;
		lcid=(m_padding_subhead.m_lcid_e_r_r)&31;
		
		fsm_printf("[HEXI]T_C_RNTI CONTROL LCID:%d\n",lcid);
		//continue_flag=(m_padding_subhead->m_lcid_e_r_r)>>5;
		continue_flag=1;
		
		/*20150120gai*/
		while(continue_flag){
			len=sizeof(MAC_SH_padding);		//获取LCID

			if(!compare(skb->tail-skb->data-from_len,len)){
				SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
				fsm_pkt_destroy(skb);
				FOUT;
			}

			fsm_mem_cpy(&m_padding_subhead,skb->data+from_len,len);
			//from_len=from_len+len;

			continue_flag=m_padding_subhead.m_lcid_e_r_r>>5;
			lcid=(m_padding_subhead.m_lcid_e_r_r)&31;

			//fsm_printf("[DECOMPLEX]LCID OF CONTROL ELEMENT IN T_C_RNTI:%d\n",lcid);	//FOR TEST
			/*20150120gai*/
		
		
		if(lcid!=0){
			switch(lcid){
			case PHR_LCID:	//20140428 改
        
				break;

			case SHORT_BSR:
				len=sizeof(MAC_CE_shortBSR);

				if(!compare(skb->tail-skb->data-head_len,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(&m_short_bsr,skb->data+head_len,len);
				head_len=head_len+len;
				//处理短BSR
				buff_size=(m_short_bsr.m_lcgid_buffersize)&63;
				
				Add_sueinfo_Node(&m_ue_infor_from_phy,buff_size,0);//for test

				break;
			case LONG_BSR:
				len=sizeof(MAC_CE_longBSR);

				if(!compare(skb->tail-skb->data-head_len,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(&m_long_bsr,skb->data+head_len,len);
				head_len=head_len+len;
				//处理长BSR
				buff_size=m_long_bsr.m_buffersize1;
				buff_size+=(m_long_bsr.m_buffersize2<<8);
				
				fsm_printf("[HEXI]RNTI IN DECOMPLEX AND IN INFORMATION FROM SRIO:%d,%d\n",ici_rnti,m_ue_infor_from_phy.rnti);

				Add_sueinfo_Node(&m_ue_infor_from_phy,buff_size,1);//for test
				

				break;
			case PADDING_LCID:
				//from_len=from_len+len;
				break;
				
				
			case C_RNTI_LCID:
				len=sizeof(MAC_CE_Crnti);

				if(!compare(skb->tail-skb->data-head_len,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(&m_c_rnti,skb->data+head_len,len);
				
				fsm_printf("[MAC DECOMPLEX]HEAD_LEN IN C_RNTI:%d\n",head_len);
				
				head_len=head_len+len;
				//处理C_RNTI
				SV(RAR_receive_crnti)=1;
				c_rnti=m_c_rnti.m_crnti;
		//		c_rnti = 62;  //for test 20141217
				//fsm_printf("[DECOMPLEX] run to C_RNTI_LCID IN T_C_RNTI\n");
				Do_judge_rnti(c_rnti);

				phy_ici.rnti=c_rnti;

				break;
				
				
			default:
				continue_flag=0;
				from_len=from_len-sizeof(MAC_SH_padding);//after switch from_len will be added which shouldn't be added
				break;
				
			}	//switch
			len=sizeof(MAC_SH_padding);
			from_len=from_len+len;
		}		//if
		else
			continue_flag=0;
		
		}//while
		
		//fsm_printf("[DECOMPLEX]T_C_RNTI CONTROL ELEMENTS:%d\n",from_len);		


		len=sizeof(MAC_SH_padding);		//获取LCID

		if(!compare(skb->tail-skb->data-from_len,len)){
			SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
			fsm_pkt_destroy(skb);
			FOUT;
		}

		fsm_mem_cpy(&m_padding_subhead,skb->data+from_len,len);
		//from_len=from_len+len;
		lcid=(m_padding_subhead.m_lcid_e_r_r)&31;
		
		fsm_printf("[HEXI]T_C_RNTI DECOMPLEX LCID:%d\n",lcid);
		//continue_flag=(m_padding_subhead->m_lcid_e_r_r)>>5;
		continue_flag=1;


		/*if(lcid==C_RNTI_LCID){
			len=sizeof(MAC_CE_Crnti);
			fsm_mem_cpy(m_c_rnti,skb->data+head_len,len);
			head_len=head_len+len;
			//处理C_RNTI
			SV(RAR_receive_crnti)=1;
			c_rnti=m_c_rnti->m_crnti;
			Do_judge_rnti(c_rnti);
		}
		else if(lcid==0){*/ //20150120gai
		if(lcid==0){
			/////
				//以IOctl的方式告诉RRC层该rnti
					fsm_do_ioctrl(STRM_TO_RRC, IOCCMD_MACtoRRC_RNTI_indicate, &ici_rnti, sizeof(ici_rnti));
					//fsm_printf("[DECOMPLEX] IOCCMD_MACtoRRC_RNTI_indicate, rnti = %d\n ", ici_rnti);			
			/////

			
			dealCcchSdu(skb,head_len);
		}
		//else{ 
		while(continue_flag){
			len=sizeof(MAC_SDU_subhead_7bit);

			if(!compare(skb->tail-skb->data-from_len,len)){
				SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
				fsm_pkt_destroy(skb);
				FOUT;
			}

			fsm_mem_cpy(&m_7bit_subhead,skb->data+from_len,len);
			f_domain=m_7bit_subhead.m_f_l>>7;
			continue_flag=(m_7bit_subhead.m_lcid_e_r_r)>>5;
			if(f_domain==1){
				
				fsm_printf("[HEXI]DECOMPLEX 15 BIT SUBHEAD!\n");
			
				len=sizeof(MAC_SDU_subhead_15bit);

				if(!compare(newskb->tail-newskb->data-from_len,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(&m_15bit_subhead,skb->data+from_len,len);
				from_len=from_len+len;
				len=(m_15bit_subhead.m_f_l&127);
				len=len<<8;
				len=len+m_15bit_subhead.m_l;
				newskb=fsm_pkt_create(len+REDUNDANCE);
				//fsm_skb_put(newskb,len+1);
				
				fsm_printf("[HEXI]len domain: len = %d\n",len);
				
				fsm_skb_put(newskb,len);  //for test 20150126??????????

				if(!compare(skb->tail-skb->data-head_len,len)||!compare(newskb->tail-newskb->data,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(newskb->data,skb->data+head_len,len);	//for test
				head_len=head_len+len;
				setMACtoRLC_IciMsg(&m_rlc_type2_icimsg,lcid,phy_ici.rnti);
				len=sizeof(MACtoRLC_IciMsg);

				if(!compare(newskb->data-newskb->head,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(newskb->head,&m_rlc_type2_icimsg,len);

				//fsm_pkt_send(newskb,STRM_TO_RLC);		//20140428 改

				rlc_pkt_receive_from_mac(newskb);  //add in 20150804
				//fsm_pkt_send(newskb,STRM_TO_IPADP);

				fsm_printf("[HEXI]send to rlc data1: len = %d\n",newskb->len);
				//fsm_octets_print(newskb->data,newskb->len);
			}
			else{
			
				fsm_printf("[HEXI]DECOMPLEX 7 BIT SUBHEAD!\n");
			
				from_len=from_len+len;
				//len=(m_7bit_subhead->m_f_l<<1)>>1;
				len=(m_7bit_subhead.m_f_l&127);
				newskb=fsm_pkt_create(len+10);

				fsm_printf("[HEXI]len domain: len = %d\n",len);


				fsm_skb_put(newskb,len);

				if(!compare(skb->tail-skb->data-head_len,len)||!compare(newskb->tail-newskb->data,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}


				fsm_mem_cpy(newskb->data,skb->data+head_len,len);	//for test
				head_len=head_len+len;
				setMACtoRLC_IciMsg(&m_rlc_type2_icimsg,lcid,phy_ici.rnti);
				len=sizeof(MACtoRLC_IciMsg);

				if(!compare(newskb->data-newskb->head,len)){
					SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
					fsm_pkt_destroy(skb);
					FOUT;
				}

				fsm_mem_cpy(newskb->head,&m_rlc_type2_icimsg,len);
				//fsm_pkt_send(newskb,STRM_TO_RLC);		//20140428 改
				
				rlc_pkt_receive_from_mac(newskb);  //add in 20150804
				//fsm_pkt_send(newskb,STRM_TO_IPADP);

				fsm_printf("[HEXI]send to rlc data2: len = %d\n",newskb->len);
				//fsm_octets_print(newskb->data,newskb->len);
				//fsm_octets_print(newskb->data,(newskb->tail-newskb->data));
			}
		}
		
		/*
		 *	for test
		 */
		 if(lcid<0||lcid>31){
		 	SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;
		 	
		 }
		//}
	}

	else{
		//fsm_printf("[HEXI]DECOMPLEX ILLEGAL RNTI!\n");		
		SV(lost_pktnum_uplink)=SV(lost_pktnum_uplink)+1;

		fsm_printf("[HEXI]RNTI IS NOT RIGHT!\n");
		
		fsm_pkt_destroy(skb);
		FOUT;
	}

	fsm_pkt_destroy(skb);
	FOUT;
}


/************** HeXi end ******************/


