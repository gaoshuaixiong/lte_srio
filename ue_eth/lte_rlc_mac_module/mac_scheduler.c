#include <linux/if_ether.h>
/****lhl改动20140410****/
#include <linux/list.h>
#include <linux/kernel.h>
#include <stddef.h>
/****lhl改动结束20140410****/
#include "rlc_macfsm.h"
#include "macpkfmt.h"
#include "rrc_config.h"

/************************UL schedule LiuHanLi**************************/

/*
******************************
->Change Data and time: 
->Function:Initialization parameters
->Change:
->Details:函数完成各个全局变量的初始化
->input:rlc_mac_sv 中部分全局变量
->output:
->Special:
*******************************
*/
void Init_Uemac_Scheduler(void)
{
	FIN(Init_Uemac_Scheduler(void));
	SV_PTR_GET(rlc_mac_sv);
	SV(LCGZeroToData)=false;
	SV(m_freshUlBsr)=false;
	//SV(ALLOCATION_RESOURCE_BSR_INFO)=false;
	SV(DATA_WAIT_ALLOCATION)=false;

	SV(bsr)=(MacBufferStatus_BSR_Info *)fsm_mem_alloc(sizeof(MacBufferStatus_BSR_Info));
	fsm_mem_set(SV(bsr),0,sizeof(MacBufferStatus_BSR_Info));
	SV(Periodicbsr)=NULL;
	SV(Regularbsr)=NULL;
	SV(MacBuffer_RLC)=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
	SV(LogicalChannel_Bj)=(LogicalChannelBj *)fsm_mem_alloc(sizeof(LogicalChannelBj));
	SV(LogicalChannel_Config)=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo));
	INIT_LIST_HEAD(&(SV(MacBuffer_RLC)->list));
	INIT_LIST_HEAD(&(SV(LogicalChannel_Bj)->list));
	INIT_LIST_HEAD(&(SV(LogicalChannel_Config)->list));
	FOUT;
}


/*
******************************
->Change Data and time: 20140526
->Function:the array of rlc data request is converted to list
->Change:
->Details:将RLC的数组请求转换为链表请求，为了与之前的设计保持一致
->Input:RLC_Request *rlc_request_array:RLC的IOCTL请求,int num:IOCTL中的长度字段，表示有num个信道
->Output:RlcBufferRequest 链表
->Special:
*******************************
*/
RlcBufferRequest * RlcRequest_arraytolist(RLC_Request *rlc_request_array,u32 num)//num是请求的长度
{
	u32 i;
	RlcBufferRequest *rlcrequest;
	RlcBufferRequest *rlcrequest_temp;
	LogicalChannelConfigInfo *temp_lcc_info;
	u8 lcid;
	u32 flag;

	FIN(RlcRequest_arraytolist(RLC_Request *rlc_request_array,u32 num));
	SV_PTR_GET(rlc_mac_sv);
	rlcrequest=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	/*while(rlcrequest==0)//申请内存失败，再次申请，直到申请成功
	{
		rlcrequest=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	}*/
	INIT_LIST_HEAD(&(rlcrequest->list));
	for(i=0;i<num;i++)
	{
		lcid=rlc_request_array[i].lcid;
		temp_lcc_info=NULL;
		flag=0;
		list_for_each_entry(temp_lcc_info, &(SV(LogicalChannel_Config)->list),list)//遍历
		{
			if(lcid==temp_lcc_info->lcid)
			{
				if(temp_lcc_info->logicalChannelGroup>=0 && temp_lcc_info->logicalChannelGroup<4)
				{
					flag=1;
					break;
				}
				else
				{
					fsm_printf("[UEMAC][RLCreq_tolist]:lc_group error ,maybe it's rrc's mistake\n");
					continue;
				}
			}
		}
		if(flag==1)//只有存在该request的逻辑信道配置时才添加到MAC 更新链表中，否则忽视
		{		
			rlcrequest_temp=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
			/*while(rlcrequest_temp==0)//申请内存失败，再次申请，直到申请成功
			{
				rlcrequest_temp=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
			}*/
			INIT_LIST_HEAD(&(rlcrequest_temp->list));
			rlcrequest_temp->rnti=rlc_request_array[i].rnti;
			rlcrequest_temp->lcid=rlc_request_array[i].lcid;
			rlcrequest_temp->txQueueSize=rlc_request_array[i].txQueueSize;
			rlcrequest_temp->txQueueHeader=rlc_request_array[i].txQueueHeader;
			rlcrequest_temp->retxQueueSize=rlc_request_array[i].retxQueueSize;
			rlcrequest_temp->retxQueueHeader=rlc_request_array[i].retxQueueHeader;
			rlcrequest_temp->statusPduSize=rlc_request_array[i].statusPduSize;
			rlcrequest_temp->statusPduHeader=rlc_request_array[i].statusPduHeader;
			list_add_tail(&(rlcrequest_temp->list), &(rlcrequest->list));
		}
		else
			fsm_printf("[UEMAC][RLCreq_tolist]:rlc request and lc_config error\n");
	}
	SV(Rlc_Request_Num)=num;
	FRET(rlcrequest);
}

/*******20140526 将RLC的链表请求转换为数组请求，为了与之前的设计保持一致********/
/*RLC_Request * RlcRequest_listtoarray( RlcBufferRequest *rlc_request,int *num)//num是report的大小
{
	struct list_head *pos,*p;
	RlcBufferRequest *temp;
	int i;
	RLC_Request *rlc_report;
	FIN(RlcRequest_listtoarray( RlcBufferRequest *rlc_request,int *num));
	SV_PTR_GET(rlc_mac_sv);
	rlc_report=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request)*SV(Rlc_Request_Num));//动态申请report的大小
	while(rlc_report==0)//申请内存失败，再次申请直到成功
	{
		rlc_report=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request)*SV(Rlc_Request_Num));
	}
	pos=NULL,p=NULL,temp=NULL;
	i=0;
	list_for_each_safe(pos,p,&(rlc_request->list))//遍历
	{
		temp=list_entry(pos,RlcBufferRequest,list);
		rlc_report[i].rnti=temp->rnti;
		rlc_report[i].lcid=temp->lcid;
		rlc_report[i].txQueueSize=temp->txQueueSize;
		rlc_report[i].txQueueHeader=temp->txQueueHeader;
		rlc_report[i].retxQueueSize=temp->retxQueueSize;
		rlc_report[i].retxQueueHeader=temp->retxQueueHeader;
		rlc_report[i].statusPduSize=temp->statusPduSize;
		rlc_report[i].statusPduHeader=temp->statusPduHeader;
		i++;
	}

	*num=i;//返回report的大小(数组大小)
	pos=NULL,p=NULL,temp=NULL;
	list_for_each_safe(pos,p,&(rlc_request->list))//遍历
	{
		temp=list_entry(pos,RlcBufferRequest,list);
		list_del_init(&(temp->list));
		fsm_mem_free(temp);//释放资源
	}
	FRET(rlc_report);
}*/


/*******20140529 将RLC的链表请求转换为数组请求，为了与之前的设计保持一致********/
/*******输入RlcBufferRequest为RLC report链表，输出：*num是要发送的IOCTL的buffer长度（即RLC 报告数组+长度的总长），返回RLC 报告的头指针***************/
/*
******************************
->Change Data and time: 20140529
->Function:the list of rlc response is converted to array ,in order to send IOCTL
->Change:
->Details:将RLC的response 由链表的形式转换为数组，最终转换为IOCTL的数据结构形式。
->Input:RlcBufferRequest为RLC report链表
->Output:*num:是要发送的IOCTL的buffer长度（即RLC 报告数组+长度的总长）， void *: 返回RLC 报告的头指针
->Special:
*******************************
*/
 void * RlcRequest_listtoarray( RlcBufferRequest *rlc_request,u32 *num)//num是report的大小
{
	struct list_head *pos,*p;
	RlcBufferRequest *temp;
	u32 i;
	void *rlc_report;
	u32 *temp_num;
	RLC_Request *rlc_report_temp;

	FIN(RlcRequest_listtoarray( RlcBufferRequest *rlc_request,int *num));
	SV_PTR_GET(rlc_mac_sv);
	rlc_report=(void *)fsm_mem_alloc(sizeof(RLC_Request)*SV(Rlc_Request_Num)+sizeof(u32));//动态申请report的大小
	/*while(rlc_report==0)//申请内存失败，再次申请直到成功
	{
		rlc_report=(void *)fsm_mem_alloc(sizeof(RLC_Request)*SV(Rlc_Request_Num)+sizeof(int));
	}*/
	pos=NULL,p=NULL,temp=NULL;
	i=0;

	rlc_report_temp=(RLC_Request *)(((u32 *)rlc_report)+1);//指针指向数组的首地址，+1是放个数的地址
	list_for_each_safe(pos,p,&(rlc_request->list))//遍历
	{
		temp=list_entry(pos,RlcBufferRequest,list);
		rlc_report_temp[i].rnti=temp->rnti;
		rlc_report_temp[i].lcid=temp->lcid;
		rlc_report_temp[i].txQueueSize=temp->txQueueSize;
		rlc_report_temp[i].txQueueHeader=temp->txQueueHeader;
		rlc_report_temp[i].retxQueueSize=temp->retxQueueSize;
		rlc_report_temp[i].retxQueueHeader=temp->retxQueueHeader;
		rlc_report_temp[i].statusPduSize=temp->statusPduSize;
		rlc_report_temp[i].statusPduHeader=temp->statusPduHeader;
		i++;
	}
	temp_num=(u32 *)rlc_report;
	*temp_num=i;//report的大小(数组大小)
	*num=sizeof(RLC_Request)*i+sizeof(u32);//返回RLC report的总大小
	pos=NULL,p=NULL,temp=NULL;
	list_for_each_safe(pos,p,&(rlc_request->list))//遍历
	{
		temp=list_entry(pos,RlcBufferRequest,list);
		list_del_init(&(temp->list));
		fsm_mem_free(temp);//释放资源
	}
	fsm_mem_free(rlc_request);
	FRET(rlc_report);
}




 /*
******************************
->Change Data and time: 
->Function:update RLC buffer  request
->Change:
->Details:由RLC层的buffer请求，更新MAC层buffer状态变量
->Input:RlcBufferRequest* params : RLC request 链表
->Output:SV(MacBuffer_RLC) :MAC缓冲区链表
->Special:
*******************************
*/
void DoRefreshRLCBuffserRequest (RlcBufferRequest* params)	//已测试
{
	struct list_head *pos,*p;
	u8 temp_lcid,temp_mac_bufferstatus_lcid;
	FIN(DoRefreshRLCBuffserRequest(RlcBufferRequest* params));
	
	SV_PTR_GET(rlc_mac_sv);
	RlcBufferRequest *RLChead=params,*temp=NULL;	
	MacBufferStatus *temp_mac_bufferstatus=NULL;
	MacBufferStatus *temp_mac_findno=NULL;	
	bool flag=false;

	//int length_rlc_request=0;
	RefreshBufferStatusForBSR();
	if (SV(bsr)->m_lcgnum==0)
		SV(LCGZeroToData)=true;
	SV(DATA_WAIT_ALLOCATION)=true;
	list_for_each_safe(pos,p,&(RLChead->list))//遍历
	{
		temp=list_entry(pos,RlcBufferRequest,list);
		temp_lcid=temp->lcid;
		//length_rlc_request++;
		temp_mac_bufferstatus=NULL;
		list_for_each_entry(temp_mac_bufferstatus, &(SV(MacBuffer_RLC)->list),list)//遍历
		{
			temp_mac_bufferstatus_lcid=temp_mac_bufferstatus->lcid;
			if(temp_lcid==temp_mac_bufferstatus_lcid)
			{
				temp_mac_bufferstatus->RlcRequestparams=Rlc_MacRequest_copy(temp);
				flag=true;
				break;
			}
		}
		if(flag==false)//查找失败
		{
			temp_mac_findno=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
			while(temp_mac_findno==0)//申请内存失败，再次申请直到成功
			{
				temp_mac_findno=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
			}
			INIT_LIST_HEAD(&(temp_mac_findno->list));
			temp_mac_findno->lcid=temp_lcid;
			temp_mac_findno->RlcRequestparams=Rlc_MacRequest_copy(temp);
			list_add_tail(&(temp_mac_findno->list), &(SV(MacBuffer_RLC)->list));
		}
		flag=false;
		SV(m_freshUlBsr)=true;
	}
	temp=NULL;
	list_for_each_safe(pos,p,&(RLChead->list))//遍历
	{
		temp=list_entry(pos,RlcBufferRequest,list);
		list_del_init(&(temp->list));
		fsm_mem_free(temp);
	}
	fsm_mem_free(RLChead);
	FOUT;
}


 /*
******************************
->Change Data and time: 
->Function:copy the node of RlcBufferRequest to RLC_Request
->Change:
->Details:复制RlcBufferRequest节点，使返回节点可插入MacBufferStatus中，解决next为NULL的问题
->Input:RlcBufferRequest* temp : RLC request 链表
->Output:RLC_Request
->Special:
*******************************
*/
RLC_Request* Rlc_MacRequest_copy(RlcBufferRequest *temp)
{
	FIN(Rlc_MacRequest_copy(RlcBufferRequest *temp));
	RLC_Request *copy=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
	/*while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
	}*/
	copy->lcid=temp->lcid;
	copy->retxQueueHeader=temp->retxQueueHeader;
	copy->retxQueueSize=temp->retxQueueSize;
	copy->rnti=temp->rnti;
	copy->statusPduHeader=temp->statusPduHeader;
	copy->statusPduSize=temp->statusPduSize;
	copy->txQueueHeader=temp->txQueueHeader;
	copy->txQueueSize=temp->txQueueSize;
	FRET(copy);
}
 /*
******************************
->Change Data and time: 
->Function:copy the node of MAC_LogicalChannelConfig_IoctrlMsg to LogicalChannelConfigInfo
->Change:
->Details:复制MAC_LogicalChannelConfig_IoctrlMsg节点，到LogicalChannelConfigInfo中，使MAC维护逻辑信道属性的链表
->Input:MAC_LogicalChannelConfig_IoctrlMsg* temp : RRC对MAC的逻辑信道配置IOCTL结构
->Output:LogicalChannelConfigInfo:MAC维护的逻辑信道链表
->Special:
*******************************
*/
LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_copy(MAC_LogicalChannelConfig_IoctrlMsg *temp)
{
	FIN(LogicalChannel_ConfigInfo_copy(MAC_LogicalChannelConfig_IoctrlMsg *temp));
	LogicalChannelConfigInfo *copy=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo));
	/*while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo));
	}*/
	INIT_LIST_HEAD(&(copy->list));
	copy->lcid=temp->logicalChannelIdentity;
	copy->priority=(temp->logicalChannelConfig).ul_SpecificParameters.priority;
	copy->prioritizedBitRateKbps=(temp->logicalChannelConfig).ul_SpecificParameters.prioritisedBitRate;
	copy->bucketSizeDurationMs=(temp->logicalChannelConfig).ul_SpecificParameters.bucketSizeDuration;
	copy->logicalChannelGroup=(temp->logicalChannelConfig).ul_SpecificParameters.logicalChannelGroup;
	FRET(copy);
}
 /*
******************************
->Change Data and time: 
->Function:copy the node of RLC_Request to RlcBufferRequest
->Change:
->Details:复制RLC_Request节点，到RlcBufferRequest中，使MAC维护RLC request链表
->Input:RlcBufferRequest* temp : RLC对MAC的request  IOCTL结构中的部分结构
->Output:RlcBufferRequest:MAC维护的RLC request链表
->Special:
*******************************
*/
RlcBufferRequest* Mac_RlcRequest_copy(RLC_Request *temp)//复制RlcBufferRequest节点
{
	FIN(Mac_RlcRequest_copy(RLC_Request *temp));
	RlcBufferRequest *copy=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	/*while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	}*/
	INIT_LIST_HEAD(&(copy->list));
	copy->lcid=temp->lcid;
	copy->retxQueueHeader=temp->retxQueueHeader;
	copy->retxQueueSize=temp->retxQueueSize;
	copy->rnti=temp->rnti;
	copy->statusPduHeader=temp->statusPduHeader;
	copy->statusPduSize=temp->statusPduSize;
	copy->txQueueHeader=temp->txQueueHeader;
	copy->txQueueSize=temp->txQueueSize;
	FRET(copy);
}
 /*
******************************
->Change Data and time: 
->Function:copy the node of RLC_Request 
->Change:
->Details:复制RLC_Request节点
->Input:RLC_Request* temp 
->Output:RLC_Request *
->Special:
*******************************
*/
RLC_Request* Mac_MacRequest_copy(RLC_Request *temp)//复制RlcBufferRequest节点
{
	FIN(Mac_MacRequest_copy(RLC_Request *temp));
	RLC_Request *copy=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
	/*while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
	}*/
	copy->lcid=temp->lcid;
	copy->retxQueueHeader=temp->retxQueueHeader;
	copy->retxQueueSize=temp->retxQueueSize;
	copy->rnti=temp->rnti;
	copy->statusPduHeader=temp->statusPduHeader;
	copy->statusPduSize=temp->statusPduSize;
	copy->txQueueHeader=temp->txQueueHeader;
	copy->txQueueSize=temp->txQueueSize;
	FRET(copy);
}
 /*
******************************
->Change Data and time: 
->Function:copy BSR
->Change:
->Details:复制bsr
->Input:SV(bsr) :全局的bsr结构
->Output:MacBufferStatus_BSR_Info *
->Special:
*******************************
*/
MacBufferStatus_BSR_Info* MacBufferStatus_BSR_Info_copy(void)//复制BSR
{
	FIN(MacBufferStatus_BSR_Info_copy(void));
	SV_PTR_GET(rlc_mac_sv);
	u32 j=0;
	MacBufferStatus_BSR_Info* copy=(MacBufferStatus_BSR_Info*)fsm_mem_alloc(sizeof(MacBufferStatus_BSR_Info));
	/*while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(MacBufferStatus_BSR_Info*)fsm_mem_alloc(sizeof(MacBufferStatus_BSR_Info));
	}*/
	copy->m_lcgnum=SV(bsr)->m_lcgnum;
	copy->m_rnti=SV(bsr)->m_rnti;
	copy->m_TotalLcgData=SV(bsr)->m_TotalLcgData;
	for(j=0;j<LCG;j++)
		copy->m_bufferStatus[j]=SV(bsr)->m_bufferStatus[j];
	FRET(copy);
}


 /*
******************************
->Change Data and time: 
->Function:
->Change:20140526modified 更改了无穷大的情况,无穷大的逻辑信道lcbj设置为MAX_BJ/8+1
->Details:每个TTI更新逻辑信道的Bj值
->Input:SV(LogicalChannel_Bj)，(SV(LogicalChannel_Config)
->Output:SV(LogicalChannel_Config)中的lcbj
->Special:无穷大的逻辑信道lcbj设置为MAX_BJ/8+1:这是协议中没有明确说明的，讨论的方案
*******************************
*/
void FlushBj(void)
{
	LogicalChannelConfigInfo *temp_Logical_Channel_Config;
	u8 temp_Bj_lcid,temp_lcc_lcid,BucketSize;
	FIN(FlushBj(void));
	SV_PTR_GET(rlc_mac_sv);
	LogicalChannelBj *head_Bj=SV(LogicalChannel_Bj),*temp=NULL;	
	bool flag_tail=false;

	list_for_each_entry(temp, &head_Bj->list,list)//遍历
	{
		temp_Bj_lcid=temp->lcid;
		flag_tail=false;
		temp_Logical_Channel_Config=NULL;
		list_for_each_entry(temp_Logical_Channel_Config, &(SV(LogicalChannel_Config)->list),list)//遍历
		{
			temp_lcc_lcid=temp_Logical_Channel_Config->lcid;
			if(temp_lcc_lcid==temp_Bj_lcid)
			{
				if((temp_Logical_Channel_Config->prioritizedBitRateKbps)==infinity)//无穷大的情况
				{
					temp->lcbj=MAX_BJ/8+1;//处理为最大值加1
					flag_tail=true;
				}
				else
				{
					BucketSize=(temp_Logical_Channel_Config->prioritizedBitRateKbps)*(temp_Logical_Channel_Config->bucketSizeDurationMs)/8;
					if(temp->lcbj < BucketSize)
						temp->lcbj=temp->lcbj+(temp_Logical_Channel_Config->prioritizedBitRateKbps)*TTI/8;//除以8是转换为byte
					else
						temp->lcbj=BucketSize;
					flag_tail=true;
				}
				break;
			}
		}
		if(flag_tail==false)
			//fsm_printf("[UEMAC][FlushBj]FlushBj ERROR");//LogicalChannelConfig和LogicalChannelConfigBj不匹配
		flag_tail=false;
	}
	FOUT;
}


/***************由ULGrant获取TBSize(基站分配总TBsize)****************/

 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由ULGrant获取TBSize(基站分配总TBsize)
->Input: receive_ulgrant:由物理适配层的IOCTL 结构而来
->Output:返回当前分配到的资源总大小
->Special:
*******************************
*/
/*int DoReceiveULgrant_Tbsize(int receive_ulgrant)
{
	unsigned short rb_number;
	FIN(DoReceiveULgrant_Tbsize(int receive_ulgrant));
	unsigned short RIV=(receive_ulgrant>>16)&(int)(My_Pow(2,13)-1);
	unsigned  short mcs=(receive_ulgrant>>12)&31;//20140725
	unsigned short tmp=0;
	unsigned short RB_length=0;
	
	rb_number=GetRbsize(BANDWIDTH);
	//tmp=My_Floor(RIV/rb_number)+RIV%rb_number;
	tmp=RIV/rb_number+RIV%rb_number;
	if(tmp<rb_number)
		RB_length=RIV/rb_number+1;
	else
		//RB_length=rb_number-My_Floor(RIV/rb_number)+1;
		RB_length=rb_number-RIV/rb_number+1;//20140514modified by lhl 为了不使用float
	//????????????????上行的发送子帧和帧，由于 delay没有确定，帧号子帧号也不能确定
	FRET(OFDM*SUB_CARRIER*RB_length*mcs);
}*/

 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由ULGrant获取TBSize(基站分配总TBsize)
->Input: receive_ulgrant:由物理适配层的IOCTL 结构而来
->Output:返回当前分配到的资源总大小
->Special:
*******************************
*/
/*u32 DoReceiveULgrant_Tbsize(u32 receive_ulgrant)
{
    FIN(DoReceiveULgrant_Tbsize(u32 receive_ulgrant));
    u16 RIV=(receive_ulgrant>>16)&(u32)(My_Pow(2,13)-1);
    u16 mcs=(receive_ulgrant>>12)&31;//20140725
    u16 tmp=0;
    u16 RB_length=0;
    u16 rb_number;

    rb_number=GetRbsize(BANDWIDTH);
    //tmp=My_Floor(RIV/rb_number)+RIV%rb_number;
    tmp=RIV/rb_number+RIV%rb_number;
    if(tmp<rb_number)
        RB_length=RIV/rb_number+1;
    else
        //RB_length=rb_number-My_Floor(RIV/rb_number)+1;
        RB_length=rb_number-RIV/rb_number+1;//20140514modified by lhl 为了不使用float
    //????????????????上行的发送子帧和帧，由于 delay没有确定，帧号子帧号也不能确定
    FRET(TransportBlockSizeTable[RB_length-1][mcs]);
}*/

u32 DoReceiveULgrant_Tbsize(Regular_ULgrant receive_ulgrant)
{
    FIN(DoReceiveULgrant_Tbsize(u32 receive_ulgrant));
    u16 RIV=receive_ulgrant.RIV;//(receive_ulgrant>>16)&(u32)(My_Pow(2,13)-1);
    u16 mcs=receive_ulgrant.m_mcs;//20140725
    u16 temp_subframe;
    u16 temp_frame; 
    u16 tmp=0;
    u16 RB_length=0;
    u16 rb_number;
	unsigned int itbs;
    SV_PTR_GET(rlc_mac_sv);
    rb_number=GetRbsize(BANDWIDTH);
    //tmp=My_Floor(RIV/rb_number)+RIV%rb_number;
    tmp=RIV/rb_number+RIV%rb_number;
    if(tmp<rb_number)
        RB_length=RIV/rb_number+1;
    else
        //RB_length=rb_number-My_Floor(RIV/rb_number)+1;
        RB_length=rb_number-RIV/rb_number+1;//20140514modified by lhl 为了不使用float
    //????????????????上行的发送子帧和帧，由于 delay没有确定，帧号子帧号也不能确定
	temp_subframe=SV(recv_frame).subframeNo;//获取接收ULGRNT时的帧号子帧号
	temp_frame=SV(recv_frame).frameNo;
	SV(recv_frame).subframeNo=0;//清空
	SV(recv_frame).frameNo=0;
	if( (SCHEDULE_ADVANCE+temp_subframe)<=UL_SUBFRAME2)//加上提前量也小于2
	{
		SV(ue_schedule_frame.frameNo)=temp_frame;
		SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME2;
	}
	else if( (SCHEDULE_ADVANCE+temp_subframe)>UL_SUBFRAME2 && (SCHEDULE_ADVANCE+temp_subframe)<=UL_SUBFRAME7)
	//且加上提前量也大于2，小于等于7
	{
		SV(ue_schedule_frame.frameNo)=temp_frame;
		SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME7;
	}
	else if( (SCHEDULE_ADVANCE+temp_subframe)>UL_SUBFRAME7 && (SCHEDULE_ADVANCE+temp_subframe)<=9)
	//加上提前子帧后，需要调度下一个上行2子帧
	{
		SV(ue_schedule_frame.frameNo)=temp_frame+1;
		SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME2;
	}
	else if( (SCHEDULE_ADVANCE+temp_subframe)>9 &&  (SCHEDULE_ADVANCE+temp_subframe)%10<=2)
	//加上提前子帧后，需要调度下一个上行2子帧
	{
		SV(ue_schedule_frame.frameNo)=temp_frame+1;
		SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME2;
	}
	else if((SCHEDULE_ADVANCE+temp_subframe)>9 &&  (SCHEDULE_ADVANCE+temp_subframe)%10>2 && (SCHEDULE_ADVANCE+temp_subframe)<=7)
	//加上提前子帧后，需要调度下一个上行7子帧
	{
		SV(ue_schedule_frame.frameNo)=temp_frame+1;
		SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME7;
	}
	else//这种情况不存在
	{
		//fsm_printf("[UE MAC][DoReceiveULgrant_Tbsize]error\n");
		FRET(0);
	}
	itbs = Ul_McsToItbsize[mcs];
	FRET(TransportBlockSizeTable[RB_length-1][itbs]);
    //FRET(TransportBlockSizeTable[RB_length-1][mcs]);
}

/***********由RAR_ULGrant获取TBSize(基站分配总TBsize,单位bit)*************/
/***********modified 20140526 为了不使用float
unsigned int DoReceiveRARULgrant_Tbsize(int receive_rar_ulgrant)
{
    FIN(DoReceiveRARULgrant_Tbsize(int receive_rar_ulgrant));
    SV_PTR_GET(rlc_mac_sv);
    unsigned char mcs=(receive_rar_ulgrant>>16)&15;
    bool hopping=(receive_rar_ulgrant>>15)&1;
    unsigned short rb_assignment=(receive_rar_ulgrant>>5)&1023;
    bool ulDelay=receive_rar_ulgrant&1;
    unsigned short positon=0;
    unsigned char ulhopping_num=0;
    unsigned short RAR_RIV=0;
    unsigned short temp=0,temp_move=0;
    unsigned short tmp=0;
    unsigned short RB_length=0;
    unsigned short rb_number;
    rb_number=GetRbsize(BANDWIDTH);
    if(rb_number<=44)
    {
        positon=My_Ceil(My_Log2(rb_number));
        RAR_RIV=rb_assignment&(int)(My_Pow(2,positon)-1);
    }
    else
    {
        if((hopping==true)&&(rb_number>49))
            ulhopping_num=2;
        else if((hopping==true)&&(rb_number<=49))
            ulhopping_num=1;
        if(hopping==false)
            ulhopping_num=0;
        positon=My_Ceil(My_Log2(rb_number))-10;
        temp=(rb_assignment>>(10-ulhopping_num))&(int)(My_Pow(2,ulhopping_num)-1);
        temp_move=rb_assignment&(int)(My_Pow(2,10-ulhopping_num)-1);
        RAR_RIV=temp_move | (temp<<(10-ulhopping_num+positon));
    }
    tmp=My_Floor(RAR_RIV/rb_number)+RAR_RIV%rb_number;
    if(tmp<rb_number)
        RB_length=My_Floor(RAR_RIV/rb_number)+1;
    else
        RB_length=rb_number-My_Floor(RAR_RIV/rb_number)+1;
    if(ulDelay==true)//?????????????????????????????,delay的操作还待确定
    {
    }
    FRET(TransportBlockSizeTable[RB_length-1][mcs]);
}
**********/


 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由RAR_ULGrant获取TBSize(基站分配总TBsize,单位bit)
->Input: receive_rar_ulgrant:由解复用而来
->Output:返回当前分配到的资源总大小
->Special:
*******************************
*/
/***********由RAR_ULGrant获取TBSize(基站分配总TBsize,单位bit)*************/
/*u32 DoReceiveRARULgrant_Tbsize(u32 receive_rar_ulgrant)
{
    FIN(DoReceiveRARULgrant_Tbsize(u32 receive_rar_ulgrant));
    SV_PTR_GET(rlc_mac_sv);
    u8 mcs=(receive_rar_ulgrant>>17)&15;//20140725
    bool hopping=(receive_rar_ulgrant>>31)&1;
    u16 rb_assignment=(receive_rar_ulgrant>>21)&1023;//20140725
    bool ulDelay=(receive_rar_ulgrant>>13)&1;
    u16 positon=0;
    u32 ulhopping_num=0;
    u16 RAR_RIV=0;
    u16 temp=0,temp_move=0;
    u16 tmp=0;
    u16 RB_length=0;
    u16 rb_number;
    u16 temp_log;

    rb_number=GetRbsize(BANDWIDTH);
    if(rb_number<=44)
    {
        //positon=My_Ceil(My_Log2(rb_number));//20140514modified by lhl 为了不使用float
        temp_log=My_Log2(rb_number);
	 positon=temp_log/1000+1;
        RAR_RIV=rb_assignment&(u32)(My_Pow(2,positon)-1);
    }
    else
    {
        if((hopping==true)&&(rb_number>49))
            ulhopping_num=2;
        else if((hopping==true)&&(rb_number<=49))
            ulhopping_num=1;
        if(hopping==false)
            ulhopping_num=0;
        positon=((My_Log2(rb_number))/1000+1)-10;
        temp=(rb_assignment>>(10-ulhopping_num))&(u32)(My_Pow(2,ulhopping_num)-1);
        temp_move=rb_assignment&(u32)(My_Pow(2,10-ulhopping_num)-1);
        RAR_RIV=temp_move | (temp<<(10-ulhopping_num+positon));
    }
    //tmp=My_Floor(RAR_RIV/rb_number)+RAR_RIV%rb_number;
    tmp=RAR_RIV/rb_number+RAR_RIV%rb_number;//20140514modified by lhl 为了不使用float
    if(tmp<rb_number)
        //RB_length=My_Floor(RAR_RIV/rb_number)+1;//20140514modified by lhl 为了不使用float
        RB_length=RAR_RIV/rb_number+1;
    else
        RB_length=rb_number-RAR_RIV/rb_number+1;
    if(ulDelay==true)//?????????????????????????????,delay的操作还待确定
    {
    }
    FRET(TransportBlockSizeTable[RB_length-1][mcs]);
}*/


 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由RAR_ULGrant获取TBSize(基站分配总TBsize,单位bit)
->Input: receive_rar_ulgrant:由解复用而来
->Output:返回当前分配到的资源总大小
->Special:
*******************************
*/
u32 DoReceiveRARULgrant_Tbsize(RAR_ULgrant *receive_rar_ulgrant)
{
	u16 rb_number;
	u16 temp_log;

	FIN(DoReceiveRARULgrant_Tbsize(int receive_rar_ulgrant));
	SV_PTR_GET(rlc_mac_sv);
	u8 mcs=receive_rar_ulgrant->m_mcs;//20140725
	bool hopping=receive_rar_ulgrant->m_hoppingflag;
	u16 rb_assignment=receive_rar_ulgrant->rb_assignment;//20140725
	bool ulDelay=receive_rar_ulgrant->m_ulDelay;
	u16 positon=0;
	u32 ulhopping_num=0;
	u16 RAR_RIV=0;
	u16 temp=0,temp_move=0;
	u16 tmp=0;
	u16 RB_length=0;
	u16 temp_subframe;
    	u16 temp_frame; 
		
	rb_number=GetRbsize(BANDWIDTH);
	if(rb_number<=44)
	{
		//positon=My_Ceil(My_Log2(rb_number));//20140514modified by lhl 为了不使用float
		temp_log=My_Log2(rb_number);
		positon=temp_log/1000+1;
		RAR_RIV=rb_assignment&(int)(My_Pow(2,positon)-1);
	}
	else
	{
		if((hopping==true)&&(rb_number>49))
			ulhopping_num=2;
		else if((hopping==true)&&(rb_number<=49))
			ulhopping_num=1;
		if(hopping==false)
			ulhopping_num=0;
		positon=((My_Log2(rb_number))/1000+1)-10;
		temp=(rb_assignment>>(10-ulhopping_num))&(int)(My_Pow(2,ulhopping_num)-1);
		temp_move=rb_assignment&(int)(My_Pow(2,10-ulhopping_num)-1);
		RAR_RIV=temp_move | (temp<<(10-ulhopping_num+positon));//20141112 LHL
		//RAR_RIV=temp_move | (temp<<(10-ulhopping_num-positon));
	}
	//tmp=My_Floor(RAR_RIV/rb_number)+RAR_RIV%rb_number;
	tmp=RAR_RIV/rb_number+RAR_RIV%rb_number;//20140514modified by lhl 为了不使用float
	if(tmp<rb_number)
		//RB_length=My_Floor(RAR_RIV/rb_number)+1;//20140514modified by lhl 为了不使用float
		RB_length=RAR_RIV/rb_number+1;//rb_length就是LCRB
	else
		RB_length=rb_number-RAR_RIV/rb_number+1;
	//temp_subframe=SV(sys_frame.subframeNo);
	//temp_frame=SV(sys_frame.frameNo);

	temp_subframe=SV(recv_frame).subframeNo;//获取接收ULGRNT时的帧号子帧号
	temp_frame=SV(recv_frame).frameNo;
	SV(recv_frame).subframeNo=0;//清空
	SV(recv_frame).frameNo=0;
	if(ulDelay == true)//?????????????????????????????,delay的操作还待确定
	{
		if(temp_subframe>=0 && temp_subframe<UL_SUBFRAME2)//子帧在0-2之间
		{
			SV(ue_schedule_frame.frameNo)=temp_frame;
			SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME7;
		}
		else if(temp_subframe>=UL_SUBFRAME2 && temp_subframe<UL_SUBFRAME7)//子帧在2到7之间
		{
			SV(ue_schedule_frame.frameNo)=temp_frame+1;
			SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME2;
		}
		else if(temp_subframe>=UL_SUBFRAME7 && temp_subframe<=9)//子帧在7到9之间
		{
			SV(ue_schedule_frame.frameNo)=temp_frame+1;
			SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME7;
		}
	}
	else
	{
		if(temp_subframe>=0 && temp_subframe<UL_SUBFRAME2)//子帧在0-2之间
		{
			SV(ue_schedule_frame.frameNo)=temp_frame;
			SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME2;
		}
		else if(temp_subframe>=UL_SUBFRAME2 && temp_subframe<UL_SUBFRAME7)//子帧在2到7之间
		{
			SV(ue_schedule_frame.frameNo)=temp_frame;
			SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME7;
		}
		else if(temp_subframe>=UL_SUBFRAME7 && temp_subframe<=9)//子帧在7到9之间
		{
			SV(ue_schedule_frame.frameNo)=temp_frame+1;
			SV(ue_schedule_frame.subframeNo)=UL_SUBFRAME2;
		}
	}
	FRET(SUB_CARRIER*OFDM*mcs*RB_length);
}


 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:返回tbsize,单位byte,考虑了发送BSR的资源
->Input: 当前分配到的资源总大小
->Output:返回当前可使用的资源大小
->Special:
*******************************
*/
u32 GetTBsize_Allocation(u32 tbsize)
{
	FIN(GetTBsize_Allocation(u32 tbsize));
	SV_PTR_GET(rlc_mac_sv);
	RefreshBufferStatusForBSR();
	//SV(ALLOCATION_RESOURCE_BSR_INFO)=false;
	if((tbsize/8) < (SV(bsr)->m_TotalLcgData)) //可能需要发送BSR
	{
		//SV(ALLOCATION_RESOURCE_BSR_INFO)=true;
		FRET(((tbsize/8)-MAC_BSR_CTL_LENGTH));
	}
	else
	{
		//SV(ALLOCATION_RESOURCE_BSR_INFO)=false; //不需要发送BSR
		SV(m_freshUlBsr)=false;
		FRET((tbsize)/8);
	}
}


 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由MAC层保存的RLC请求结构，更新LCG的待发数据
->Input: (SV(MacBuffer_RLC)
->Output:SV(bsr)
->Special:
*******************************
*/
void RefreshBufferStatusForBSR(void)
{
	LogicalChannelConfigInfo *temp_lcc_info=NULL;
	MacBufferStatus *temp=NULL;
	struct list_head *pos1=NULL,*p1=NULL;
	struct list_head *pos=NULL,*p=NULL;
	u32 queue[LCG]={0,0,0,0};//LCG的数据
	u32 logicalchannelgroup;
	u32 i;
	bool flag_find=false;

	FIN(RefreshBufferStatusForBSR(void));
	SV_PTR_GET(rlc_mac_sv);
/*    if (SV(C_RNTI) == 0)//用于判断该UE是否处于连接状态
    {
      printk(KERN_INFO"MAC not initialized, BSR deferred\n");
      FOUT;
    }*/
	if(list_empty(&(SV(MacBuffer_RLC)->list)))//MAC缓冲区链表长度为0
	{
		SV(bsr)->m_lcgnum=0;
		//printk(KERN_INFO"No BSR report to transmit\n");
		FOUT;
	}	
	SV(bsr)->m_rnti=SV(C_RNTI);//状态变量
	SV(bsr)->m_lcgnum=0;
	SV(bsr)->m_TotalLcgData=0;		
	//list_for_each_entry(temp, &(SV(MacBuffer_RLC)->list),list)//遍历
	list_for_each_safe(pos1,p1,(&(SV(MacBuffer_RLC)->list)))
	{
		temp=list_entry(pos1,MacBufferStatus,list);
		temp_lcc_info=NULL;
		pos=NULL;p=NULL;
		//fsm_printf("[UEMAC][refreshBSR]:temp:lcid:%d\n",temp->lcid);
		list_for_each_safe(pos,p,(&(SV(LogicalChannel_Config)->list)))
		//  list_for_each_entry(temp_lcc_info, &(SV(LogicalChannel_Config)->list),list)//遍历
		{
			temp_lcc_info=list_entry(pos,LogicalChannelConfigInfo,list);
			//fsm_printf("MAC: temp_lcc_info:lcid:%d\n",temp_lcc_info->lcid);
			//fsm_printf("MAC: temp_lcc_info:logicalChannelGroup:%d\n",temp_lcc_info->logicalChannelGroup);
			if(temp_lcc_info->logicalChannelGroup>=0 && temp_lcc_info->logicalChannelGroup<4)
			{
				if(temp->lcid==temp_lcc_info->lcid)
				{
					logicalchannelgroup=temp_lcc_info->logicalChannelGroup;
					queue[logicalchannelgroup] =queue[logicalchannelgroup] + temp->RlcRequestparams->retxQueueHeader + temp->RlcRequestparams->retxQueueSize + temp->RlcRequestparams->statusPduHeader \
					+ temp->RlcRequestparams->statusPduSize + temp->RlcRequestparams->txQueueHeader + temp->RlcRequestparams->txQueueSize;
					flag_find=true;
					break;
				}
			}
			else
			{
				fsm_printf("[UEMAC][refreshBSR]:lc_group error\n");
				break;
			}
		}
		
		if(flag_find==false)
			//fsm_printf("[UEMAC][refreshBSR]LogicalChannel_Config and  MacBuffer_RLC find ERROR\n");//LogicalChannelConfig和lMacBuffer_RLC不匹配
		flag_find=false;
	}
	for(i=0;i<LCG;i++)
	{
		SV(bsr)->m_TotalLcgData=SV(bsr)->m_TotalLcgData+queue[i];
		SV(bsr)->m_bufferStatus[i]= BufferSize2BsrSize(queue[i]);//返回索引
		if((SV(bsr)->m_bufferStatus[i])!=0)
			SV(bsr)->m_lcgnum++;
	}
}



 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由Buffer大小转换为申请BSR大小的index
->Input: buffersize:需要申请的BSR大小
->Output:BufferSizeLevelBsrTable表中对应的index
->Special:
*******************************
*/
u32 BufferSize2BsrSize(u32 buffersize)
{
	FIN(BufferSize2BsrSize(u32 buffersize));
	u32 i=0;

	for(i=0;i<BSRTABLEINDEX-1;i++)
	{
		if (BufferSizeLevelBsrTable[i]==buffersize)
			FRET(i);
		if(BufferSizeLevelBsrTable[i]< buffersize && BufferSizeLevelBsrTable[i+1]>= buffersize)
			FRET(i+1);
		if(i== BSRTABLEINDEX-2)
			FRET(BSRTABLEINDEX-1);
	}
	FRET(-1);
}

 /*
******************************
->Change Data and time: 
->Function:
->Change:20140526modified ，无穷大逻辑系电脑属性的不特殊处理
->Details:根据逻辑信道优先级对逻辑信道排序
->Input: SV(LogicalChannel_Config)逻辑信道属性链表
->Output:LogicalChannelConfigInfo 优先级排序的逻辑信道
->Special:
*******************************
*/
LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_Rank_Priority()
{
	FIN(LogicalChannel_ConfigInfo_Rank_Priority());
	SV_PTR_GET(rlc_mac_sv);
	LogicalChannelConfigInfo *lcc_info_priority=NULL,*temp=NULL,*temp_logicalChannel=NULL;

	lcc_info_priority=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo ));
	/*while(lcc_info_priority==0)//申请内存失败，再次申请直到成功
	{
		lcc_info_priority=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo ));
	}*/
	INIT_LIST_HEAD(&(lcc_info_priority->list));
	LogicalChannelConfigInfo *next_node=NULL;
	LogicalChannelConfigInfo *temp_next=NULL;
	bool flag_add=false;
	list_for_each_entry(temp, &(SV(LogicalChannel_Config)->list),list)//遍历
	{
		next_node=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo ));
		/*while(next_node==0)//申请内存失败，再次申请直到成功
		{
			next_node=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo ));
		}*/
		next_node->lcid=temp->lcid;
		next_node->priority=temp->priority;
		next_node->prioritizedBitRateKbps=temp->prioritizedBitRateKbps;
		next_node->bucketSizeDurationMs=temp->bucketSizeDurationMs;
		next_node->logicalChannelGroup=temp->logicalChannelGroup;
		INIT_LIST_HEAD(&(next_node->list));
		/* if(next_node->prioritizedBitRateKbps==-1)//分配资源时优先考虑
			list_add(&(next_node->list), &(lcc_info_priority->list));//将无穷大优先级，放在第一个链表节点，优先分配资源
		else
		{*/
			if (list_empty(&(lcc_info_priority->list)))
				list_add_tail(&(next_node->list), &(lcc_info_priority->list));
			else
			{
				flag_add=false;
				list_for_each_entry(temp_next, &(lcc_info_priority->list),list)//遍历
				{
					//  if(temp_next->prioritizedBitRateKbps==-1)
						//  continue;
					if ((next_node->priority) <= (temp_next->priority))
					{
						list_add_tail(&(next_node->list), &(temp_next->list));
						flag_add=true;
						break;
					}
				}
				if(flag_add==false)
					list_add_tail(&(next_node->list), &(lcc_info_priority->list));
			}

	}
	FRET(lcc_info_priority);
}


 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:根据逻辑信道优先级顺序，对MacBufferStatus链表按优先级排序
->Input: SV(LogicalChannel_Config)逻辑信道属性链表，SV(MacBuffer_RLC):MAC保存的RLC请求
->Output:MacBufferStatus *:对SV(MacBuffer_RLC)排序后的链表
->Special:
*******************************
*/
MacBufferStatus* PrioritySort(void)
{
	u8 temp_Bj_lcid;
	MacBufferStatus *temp_node;
	struct list_head *pos,*p;

	FIN(PrioritySort(void));
	SV_PTR_GET(rlc_mac_sv);
	LogicalChannelConfigInfo *Priority_logicalChannel=NULL,*temp_logicalChannel=NULL;
	MacBufferStatus *PriorityMacBufferStatus=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus)),*temp_mbs=NULL;

	/*while(PriorityMacBufferStatus==0)//申请内存失败，再次申请直到成功
	{
		PriorityMacBufferStatus=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
	}*/
	
	INIT_LIST_HEAD(&(PriorityMacBufferStatus->list));	
	Priority_logicalChannel=LogicalChannel_ConfigInfo_Rank_Priority();
	list_for_each_entry(temp_logicalChannel, &(Priority_logicalChannel->list),list)//遍历
	{
		temp_Bj_lcid=temp_logicalChannel->lcid;
		list_for_each_entry(temp_node, (&(SV(MacBuffer_RLC)->list)),list)//遍历
		{
			if(temp_Bj_lcid==temp_node->lcid)
			{
				temp_mbs=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
			/*while(temp_mbs==0)//申请内存失败，再次申请直到成功
			{
				temp_mbs=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
				}*/
				INIT_LIST_HEAD(&(temp_mbs->list));
				temp_mbs->lcid=temp_node->lcid;
				temp_mbs->RlcRequestparams=Mac_MacRequest_copy(temp_node->RlcRequestparams);
				list_add_tail(&(temp_mbs->list), &(PriorityMacBufferStatus->list));
				break;
			}
		}
	}
	temp_logicalChannel=NULL;
	list_for_each_safe(pos,p,&(Priority_logicalChannel->list))//遍历
	{
		temp_logicalChannel=list_entry(pos,LogicalChannelConfigInfo,list);
		list_del_init(&(temp_logicalChannel->list));//删除节点
		fsm_mem_free(temp_logicalChannel);//释放资源
	}
	fsm_mem_free(Priority_logicalChannel);
	FRET(PriorityMacBufferStatus);
}


 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由LCGZeroToData条件，生成BSR
->Input:
->Output: SV(Regularbsr) :常规BSR，SV(MAC_CE_Tags):填充相应标识
->Special:
*******************************
*/
void DoProduceBsr_LCGZeroToData(void)
{
	FIN(DoProduceBsr_LCGZeroToData(void));
	SV_PTR_GET(rlc_mac_sv);
	if(SV(LCGZeroToData) == true)
	{
		RefreshBufferStatusForBSR();
		if (SV(bsr)!=NULL)
		{
			//fsm_printf("[UEMAC][BSR]have BSR\n");
			if(SV(Regularbsr)!=NULL)
			{
				fsm_mem_free(SV(Regularbsr));//释放Regularbsr的内存
				SV(Regularbsr)=NULL;
			}
			SV(Regularbsr)=MacBufferStatus_BSR_Info_copy();//该函数会再次申请内存
			if((SV(Regularbsr)->m_lcgnum) >1)
			{
				SV(MAC_CE_Tags)[regular_longBSR_CE].addinPDU_flag=true;
				SV(MAC_CE_Tags)[regular_longBSR_CE].LCID=LONG_BSR_LCID;
				SV(MAC_CE_Tags)[regular_longBSR_CE].MAC_CE_ptr=(void *)SV(Regularbsr);
				//fsm_printf("[UEMAC][BSR]MAC BSR long:%d\n",SV(MAC_CE_Tags)[regular_shortBSR_CE].LCID);
			}
			else if((SV(Regularbsr)->m_lcgnum) ==1)
			{
				SV(MAC_CE_Tags)[regular_shortBSR_CE].addinPDU_flag=true;
				SV(MAC_CE_Tags)[regular_shortBSR_CE].LCID=SHORT_BSR_LCID;
				SV(MAC_CE_Tags)[regular_shortBSR_CE].MAC_CE_ptr=(void *)SV(Regularbsr);
				//fsm_printf("[UEMAC][BSR]MAC BSR short:%d\n",SV(MAC_CE_Tags)[regular_shortBSR_CE].LCID);
			}
		}
	}
	SV(LCGZeroToData)=false;	
	FOUT;
}

 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由周期定时器超时,生成BSR
->Input:
->Output: SV(Periodicbsr):周期BSR，SV(MAC_CE_Tags):填充相应标识
->Special:
*******************************
*/
void DoProduceBsr_PeriodicBSRTimer(void)
{
	FIN(DoProduceBsr_PeriodicBSRTimer(void));
	SV_PTR_GET(rlc_mac_sv);
	if(SV(m_freshUlBsr) ==true)// 琼姐的周期BSR定时中断超时，NS3中周期为1ms
	{
		RefreshBufferStatusForBSR ();
		if (SV(bsr)!=NULL)
		{
			if(SV(Periodicbsr)==NULL)
				SV(Periodicbsr)=MacBufferStatus_BSR_Info_copy();
			else
			{
				fsm_mem_free(SV(Periodicbsr));//释放Regularbsr的内存
				SV(Periodicbsr)=MacBufferStatus_BSR_Info_copy();//该函数会再次申请内存
			}
			// if(SV(ALLOCATION_RESOURCE_BSR_INFO)==true)
			// {
			if((SV(Periodicbsr)->m_lcgnum) >1)
			{
				SV(MAC_CE_Tags)[period_longBSR_CE].addinPDU_flag=true;
				SV(MAC_CE_Tags)[period_longBSR_CE].LCID=LONG_BSR_LCID;
				SV(MAC_CE_Tags)[period_longBSR_CE].MAC_CE_ptr=(void *)SV(Periodicbsr);
			}
			else
			{
				SV(MAC_CE_Tags)[period_shortBSR_CE].addinPDU_flag=true;
				SV(MAC_CE_Tags)[period_shortBSR_CE].LCID=SHORT_BSR_LCID;
				SV(MAC_CE_Tags)[period_shortBSR_CE].MAC_CE_ptr=(void *)SV(Periodicbsr);
			}
		}
		SV(m_freshUlBsr)=false;
	}
	FOUT;
}

 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由重传定时器超时生成BSR
->Input:
->Output: SV(Regularbsr):常规BSR，SV(MAC_CE_Tags):填充相应标识
->Special:
*******************************
*/
void DoProduceBsr_RetxBSRTimer(void)
{
	FIN(DoProduceBsr_RetxBSRTimer(void));
	SV_PTR_GET(rlc_mac_sv);
	if(list_empty((&(SV(MacBuffer_RLC)->list))))//没有BSR要发，可能在这个TTI 已经发了BSR
	{
		FOUT;
	}
	RefreshBufferStatusForBSR ();
	if (SV(bsr)!=NULL)
	{
		if(SV(Regularbsr)==NULL)//第一次生成Regularbsr
			SV(Regularbsr)=MacBufferStatus_BSR_Info_copy();
		else
		{
			fsm_mem_free(SV(Regularbsr));//释放Regularbsr的内存
			SV(Regularbsr)=MacBufferStatus_BSR_Info_copy();//该函数会再次申请内存
		}
		if((SV(Regularbsr)->m_lcgnum) >1)
		{
			SV(MAC_CE_Tags)[regular_longBSR_CE].addinPDU_flag=true;
			SV(MAC_CE_Tags)[regular_longBSR_CE].LCID=LONG_BSR_LCID;
			SV(MAC_CE_Tags)[regular_longBSR_CE].MAC_CE_ptr=(void *)SV(Regularbsr);
		}
		else
		{
			SV(MAC_CE_Tags)[regular_shortBSR_CE].addinPDU_flag=true;
			SV(MAC_CE_Tags)[regular_shortBSR_CE].LCID=SHORT_BSR_LCID;
			SV(MAC_CE_Tags)[regular_shortBSR_CE].MAC_CE_ptr=(void *)SV(Regularbsr);
		}
	}
	FOUT;
}


 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:复用模块使用BSR信息后调用该函数，释放BSR资源
->Input:
->Output: 
->Special:
*******************************
*/
void Free_Bsr_Info(void)
{
	FIN(Free_Bsr_Info(void));
	SV_PTR_GET(rlc_mac_sv);
	if(SV(Regularbsr)!=NULL)
	{
		fsm_mem_free(SV(Regularbsr));
		SV(Regularbsr)=NULL;
	}
	if(SV(Periodicbsr)!=NULL)
	{
		fsm_mem_free(SV(Periodicbsr));
		SV(Periodicbsr)=NULL;
	}
	fsm_mem_set(SV(bsr),0,sizeof(MacBufferStatus_BSR_Info));
	FOUT;
}


 /*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:资源分配
->Input:tb_size:可用的资源总大小
->Output: RLC report
->Special:
*******************************
*/
void DoResourceAllocation(u32 tb_size)//资源分配
{
	u32 num;
	u32 *rlc_report_num=(u32 *)fsm_mem_alloc(sizeof(u32));	
	void *UEmac_Rlc_Report=NULL;
	RLC_Request *rlc_temp=NULL;

	FIN(DoResourceAllocation(u32 tb_size));
	SV_PTR_GET(rlc_mac_sv);
	//RLC_Request *UEmac_Rlc_Report=NULL;//*temp_rlc=NULL;//UE MAC层回馈给RLC的分配结果报告
	//SetFrameNo();???????????????????由于现在对delay没有确定，因此先保留接口	
	UEmac_Rlc_Report=ResourceAllocation_Algorithm(tb_size,rlc_report_num);
	SV(DATA_WAIT_ALLOCATION)=false;
	num=(*rlc_report_num-sizeof(u32))/sizeof(RLC_Request);
	rlc_temp=(RLC_Request *)((u32 *)UEmac_Rlc_Report+1);
	// printk(KERN_INFO"report_num:%d",*((int *)UEmac_Rlc_Report));

	/*int j=0;//test  code
	for(j=0;j<num;j++)//test code
	{
		printk(KERN_INFO"UEMAC rlc_Report_lcid:%d",rlc_temp[j].lcid);
		printk(KERN_INFO" UEMAC rlc_Report_txQueueSize:%d\n",rlc_temp[j].txQueueSize);
		printk(KERN_INFO" rlc_Report_txQueueHeader:%d\n",rlc_temp[j].txQueueHeader);
		printk(KERN_INFO" rlc_Report_retxQueueSize:%d\n",rlc_temp[j].retxQueueSize);
		printk(KERN_INFO" rlc_Report_retxQueueHeader:%d\n",rlc_temp[j].retxQueueHeader);
		printk(KERN_INFO" rlc_Report_statusPduSize:%d\n",rlc_temp[j].statusPduSize);
		printk(KERN_INFO" rlc_Report_statusPduHeader:%d\n",rlc_temp[j].statusPduHeader);
	}*/
	//fsm_printf("MAC :%d\n",sizeof(RLC_Request));
	//fsm_do_ioctrl(STRM_TO_RLC,IOCCMD_MACtoRLC_datasend_Allow,UEmac_Rlc_Report,*rlc_report_num);//调用状态机的函数，反馈MAC分配结果给RLC
	fsm_printf("[UEMAC][SCHEDULER]SEND DATA ALLOW IOCTL\n");
	
	PMRLC_TRANSOP_IND ((void *)UEmac_Rlc_Report); 	//modify in 20150804
	//fsm_mem_free(UEmac_Rlc_Report);//释放report 内存
	fsm_mem_free(rlc_report_num);
	FOUT;

}



/*
******************************
->Change Data and time: 
->Function:
->Change:LHL modified 20150717 重配置调用，将BJ置为0
->Details:将逻辑信道的lcbj置为0,用于重配置
->Input:SV(LogicalChannel_Bj)
->Output: SV(LogicalChannel_Bj)中lcbj为0
->Special:
*******************************
*/
void Zero_LogicalChannel_Bj_Reset()
{
	LogicalChannelBj *temp;
	struct list_head *pos=NULL,*p=NULL;	

	FIN(Zero_LogicalChannel_Bj_Reset());
	SV_PTR_GET(rlc_mac_sv);
	list_for_each_safe(pos,p,(&(SV(LogicalChannel_Bj)->list)))//遍历
	{
		temp=list_entry(pos,LogicalChannelBj,list);
		if(temp->lcbj!=(MAX_BJ/8+1))//不是无穷属性的逻辑信道
			temp->lcbj=0;//BJ置为0			
	}
	FOUT;
}


/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:更新逻辑信道属性结构和BJ结构,RRC逻辑信道配置IOCTL触发时调用
->Input:RRC的IOCTL逻辑信道配置数据
->Output: SV(LogicalChannel_Bj)
->Special:
*******************************
*/
void Init_LogicalChannel_ConfigInfo(MAC_LogicalChannelConfig_IoctrlMsg *lc_info)
{
	u8 temp_lc_config_lcid;
	FIN(Init_LogicalChannel_ConfigInfo(MAC_LogicalChannelConfig_IoctrlMsg *lc_info));
	SV_PTR_GET(rlc_mac_sv);
	bool flag=false;
	struct list_head *pos=NULL,*p=NULL;
	LogicalChannelConfigInfo *temp_lc_config=NULL;
	LogicalChannelConfigInfo *temp_add=NULL;
	LogicalChannelBj *temp_bj=NULL;
	u8 temp_lcid=lc_info->logicalChannelIdentity;
	
	list_for_each_safe(pos,p,(&(SV(LogicalChannel_Config)->list)))//遍历
	{
		temp_lc_config=list_entry(pos,LogicalChannelConfigInfo,list);
		temp_lc_config_lcid=temp_lc_config->lcid;
		if(temp_lcid==temp_lc_config_lcid)
		{
			if((lc_info->logicalChannelConfig).haveUl_SpecificParameters==true)
			{
				temp_lc_config->priority=(lc_info->logicalChannelConfig).ul_SpecificParameters.priority;
				temp_lc_config->prioritizedBitRateKbps=(lc_info->logicalChannelConfig).ul_SpecificParameters.prioritisedBitRate;
				temp_lc_config->bucketSizeDurationMs=(lc_info->logicalChannelConfig).ul_SpecificParameters.bucketSizeDuration;
				temp_lc_config->logicalChannelGroup=(lc_info->logicalChannelConfig).ul_SpecificParameters.logicalChannelGroup;
				flag=true;
				break;
			}
		}
	}
	if(flag==false)
	{
		temp_add=LogicalChannel_ConfigInfo_copy(lc_info);
		list_add_tail(&(temp_add->list), (&(SV(LogicalChannel_Config)->list)));
		temp_bj=(LogicalChannelBj *)fsm_mem_alloc(sizeof(LogicalChannelBj));
		/*while(temp_bj==0)//申请内存失败，再次申请直到成功
		{
			temp_bj=(LogicalChannelBj *)fsm_mem_alloc(sizeof(LogicalChannelBj));
		}*/
		//fsm_mem_set(temp_bj,0,sizeof(LogicalChannelBj));
		INIT_LIST_HEAD(&(temp_bj->list));
		temp_bj->lcid=lc_info->logicalChannelIdentity;
		if((temp_add->prioritizedBitRateKbps)==infinity)
			temp_bj->lcbj=MAX_BJ/8+1;
		else
			temp_bj->lcbj=0;
		list_add_tail(&(temp_bj->list), (&(SV(LogicalChannel_Bj)->list)));
	}
	// fsm_mem_free(lc_info);//modified 20140526 ,根据IOCTL 的实现，不释放IOCTL的数据
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:剩余资源大于重传buffer时的分配方案
->Input:temp_mac_buffer当前要分配资源的逻辑信道的RLC请求，
->Input:temp_logicalchannel_bj:当前要处理的逻辑信道的属性
->Input:int retxlength:该逻辑信道请求中重传PDU长度
->Input:int txlength该逻辑信道请求中传输PDU长度
->Output:mac_rlc_report:反馈给RLC的report指针,
->Output:temp_rlc_report:RLC REPORT的一个节点
->Output:*res_length剩余资源长度
->Special:
*******************************
*/
void leftresource_morethan_retxbuffer_eachlogicalchannel(RlcBufferRequest *mac_rlc_report,RlcBufferRequest *temp_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 retxlength,u32 txlength,u32 *res_length)
{
	FIN(leftresource_morethan_retxbuffer_eachlogicalchannel());
	temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
	temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
	*res_length=*res_length-retxlength;
	temp_logicalchannel_bj->lcbj-=retxlength;
	//temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
	// temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
	if(*res_length>=txlength)
	{
		temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
		temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
		temp_logicalchannel_bj->lcbj-=txlength;
		// temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
		// temp_mac_buffer->RlcRequestparams->txQueueSize=0;
		list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
		*res_length-=txlength;
		//  break;
	}
	else
	{
		if(*res_length>=(temp_mac_buffer->RlcRequestparams->txQueueHeader))//大于头部
		{
			temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
			*res_length=*res_length-(temp_mac_buffer->RlcRequestparams->txQueueHeader);
			temp_rlc_report->txQueueSize=*res_length;
			temp_logicalchannel_bj->lcbj-=(temp_mac_buffer->RlcRequestparams->txQueueHeader+temp_rlc_report->txQueueSize);
			// temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
			// temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
			list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
			*res_length=0;
			// break;
		}
		else if(*res_length < (temp_mac_buffer->RlcRequestparams->txQueueHeader))//小于头部
		{
			
			if(*res_length>= (temp_mac_buffer->RlcRequestparams->txQueueSize))//小于头，大于数据
			{
				temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
				temp_rlc_report->txQueueHeader=*res_length-(temp_mac_buffer->RlcRequestparams->txQueueSize);

				temp_logicalchannel_bj->lcbj-=*res_length;
				//  temp_mac_buffer->RlcRequestparams->txQueueHeader-=temp_rlc_report->txQueueHeader;
				// temp_mac_buffer->RlcRequestparams->txQueueSize=0;
				*res_length=0;
				list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
				//  break;
			}
			else//小于头，小于数据
			{
				temp_rlc_report->txQueueSize=*res_length;
				temp_rlc_report->txQueueHeader=0;
				temp_logicalchannel_bj->lcbj-=*res_length;
				//temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
				*res_length=0;
				list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
				// break;
			}
		}
	}
	FOUT;
}



/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:剩余资源小于重传buffer时的分配方案
->Input:temp_mac_buffer当前要分配资源的逻辑信道的RLC请求，
->Input:temp_logicalchannel_bj:当前要处理的逻辑信道的属性
->Input:int statuslength:该逻辑信道请求中状态PDU长度
->Input:int retxlength:该逻辑信道请求中重传PDU长度
->Input:int txlength该逻辑信道请求中传输PDU长度
->Output:mac_rlc_report:反馈给RLC的report指针
->Output:temp_rlc_report:RLC REPORT的一个节点 
->Output:*res_length剩余资源长度
->Special:
*******************************
*/
void leftresource_lessthan_retxbuffer_eachlogicalchannel(RlcBufferRequest *mac_rlc_report,RlcBufferRequest *temp_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 retxlength,u32 txlength,u32 *res_length)
{
	FIN(leftresource_lessthan_retxbuffer_eachlogicalchannel());
	if(*res_length>=(temp_mac_buffer->RlcRequestparams->retxQueueHeader))//大于头部
	{
		temp_logicalchannel_bj->lcbj-=*res_length;
		temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
		*res_length=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
		temp_rlc_report->retxQueueSize=*res_length;
		temp_rlc_report->txQueueHeader=0;
		temp_rlc_report->txQueueSize=0;
		// temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
		// temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
		list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
		*res_length=0;
		// break;
	}
	else if(*res_length < (temp_mac_buffer->RlcRequestparams-> retxQueueHeader))//小于重传头部
	{
		if(*res_length>= (temp_mac_buffer->RlcRequestparams->retxQueueSize))//小于头，大于数据
		{
			temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
			temp_rlc_report->retxQueueHeader=*res_length-(temp_mac_buffer->RlcRequestparams->retxQueueSize);

			temp_logicalchannel_bj->lcbj-=*res_length;
			// temp_mac_buffer->RlcRequestparams->retxQueueHeader-=temp_rlc_report->retxQueueHeader;
			//temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
			temp_rlc_report->txQueueHeader=0;
			temp_rlc_report->txQueueSize=0;
			*res_length=0;
			list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
			// break;
		}
		else//小于头，小于数据
		{
			temp_rlc_report->retxQueueSize=*res_length;
			temp_rlc_report->retxQueueHeader=0;
			temp_rlc_report->txQueueHeader=0;
			temp_rlc_report->txQueueSize=0;
			temp_logicalchannel_bj->lcbj-=*res_length;
			// temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
			*res_length=0;
			list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
			//break;
		}
	}
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:剩余资源小于状态PDU时，分配重传PDU
->Input:temp_mac_buffer当前要分配资源的逻辑信道的RLC请求，
->Input:temp_logicalchannel_bj:当前要处理的逻辑信道的属性
->Input:int statuslength:该逻辑信道请求中状态PDU长度
->Input:int retxlength:该逻辑信道请求中重传PDU长度
->Input:int txlength该逻辑信道请求中传输PDU长度
->Output:mac_rlc_report:反馈给RLC的report指针
->Output:*res_length剩余资源长度
->Special:
*******************************
*/
void ResourceAllocation_logicalchannel_firstretxbuffer(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 retxlength,u32 txlength,u32 *res_length)
{
	FIN(ResourceAllocation_logicalchannel_firstretxbuffer);
	RlcBufferRequest *temp_rlc_report;
	
	temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	fsm_mem_set(temp_rlc_report,0,sizeof(RlcBufferRequest));
	INIT_LIST_HEAD(&(temp_rlc_report->list));        
	temp_rlc_report->lcid=temp_mac_buffer->RlcRequestparams->lcid;
	temp_rlc_report->rnti=temp_mac_buffer->RlcRequestparams->rnti;
	temp_rlc_report->statusPduHeader=0;
	temp_rlc_report->statusPduSize=0;//不分状态PDU
	if(*res_length >= retxlength)
	{
		leftresource_morethan_retxbuffer_eachlogicalchannel(mac_rlc_report,temp_rlc_report,temp_mac_buffer, temp_logicalchannel_bj,retxlength,txlength, res_length);
	}
	else if(*res_length < retxlength)
	{
		leftresource_lessthan_retxbuffer_eachlogicalchannel(mac_rlc_report,temp_rlc_report,temp_mac_buffer, temp_logicalchannel_bj,retxlength, txlength,res_length);
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:对于状态PDU buffer的分配
->Input:temp_mac_buffer当前要分配资源的逻辑信道的RLC请求，
->Input:temp_logicalchannel_bj:当前要处理的逻辑信道的属性
->Input:int statuslength:该逻辑信道请求中状态PDU长度

->Input:int retxlength:该逻辑信道请求中重传PDU长度
->Input:int txlength该逻辑信道请求中传输PDU长度
->Output:mac_rlc_report:反馈给RLC的report指针
->Output:*res_length剩余资源长度
->Special:
*******************************
*/
void ResourceAllocation_logicalchannel_firststatusbuffer(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 statuslength,u32 retxlength,u32 txlength,u32 *res_length)
{
	FIN(ResourceAllocation_logicalchannel_firststatusbuffer());
	RlcBufferRequest *temp_rlc_report;
	
	temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	fsm_mem_set(temp_rlc_report,0,sizeof(RlcBufferRequest));
	INIT_LIST_HEAD(&(temp_rlc_report->list));
	temp_rlc_report->lcid=temp_mac_buffer->RlcRequestparams->lcid;
	temp_rlc_report->rnti=temp_mac_buffer->RlcRequestparams->rnti;
	temp_rlc_report->statusPduHeader=temp_mac_buffer->RlcRequestparams->statusPduHeader;
	temp_rlc_report->statusPduSize=temp_mac_buffer->RlcRequestparams->statusPduSize;
	*res_length=*res_length-statuslength;        
	if(*res_length >= retxlength)
	{
		leftresource_morethan_retxbuffer_eachlogicalchannel(mac_rlc_report,temp_rlc_report,temp_mac_buffer, temp_logicalchannel_bj,retxlength,txlength, res_length);
	}
	else if(*res_length < retxlength)
	{
		leftresource_lessthan_retxbuffer_eachlogicalchannel(mac_rlc_report,temp_rlc_report,temp_mac_buffer, temp_logicalchannel_bj,retxlength, txlength,res_length);
	}
	FOUT;
}
/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:每个逻辑信道的资源分配
->Input:temp_mac_buffer当前要分配资源的逻辑信道的RLC请求，
->Input:temp_logicalchannel_bj:当前要处理的逻辑信道的属性
->Input:int statuslength:该逻辑信道请求中状态PDU长度
->Input:int retxlength:该逻辑信道请求中重传PDU长度
->Input:int txlength该逻辑信道请求中传输PDU长度
->Output:mac_rlc_report:反馈给RLC的report指针，
->Special:
*******************************
*/
void ResourceAllocation_foreach_logicalchannel(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 statuslength,u32 retxlength,u32 txlength,u32 *res_length)
{
	FIN(ResourceAllocation_foreach_logicalchannel());
	RlcBufferRequest *temp_rlc_report=NULL;
	
	SV_PTR_GET(rlc_mac_sv);	
	if(*res_length>=(statuslength+retxlength+txlength))//TBSize大于总需求
	{
		temp_rlc_report=Mac_RlcRequest_copy(temp_mac_buffer->RlcRequestparams);
		list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
		*res_length=*res_length-(statuslength+retxlength+txlength);
		temp_logicalchannel_bj->lcbj-=(statuslength+retxlength+txlength);	
		// break;
	}
	else if(*res_length>=statuslength)//先分状态PDU ，TBSize小于总需求，但大于状态PDU
	{
		ResourceAllocation_logicalchannel_firststatusbuffer(mac_rlc_report,temp_mac_buffer,temp_logicalchannel_bj,statuslength,retxlength,txlength,res_length);
	}
	else if(*res_length <statuslength)//小于状态PDU ，TBSize小于总需求，小于状态PDU
	{
		ResourceAllocation_logicalchannel_firstretxbuffer(mac_rlc_report,temp_mac_buffer,temp_logicalchannel_bj,retxlength,txlength,res_length);
	}
	FOUT;
}


/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:UE侧资源分配算法：令牌桶
->Input:msg资源大小，SV(LogicalChannel_Bj)，
->Output:int *num:指出report数组大小，void *:RLC report指针
->Special:
*******************************
*/

void * ResourceAllocation_Algorithm(u32 resource_len,u32 *num)//num输出，指出report数组大小
{
	void *report_array;
	//int *report_num;
	MacBufferStatus *lcbj_lessthan_zero;
	MacBufferStatus *temp_lessthan_zero;
	u8 temp_buffer_lcid;
	MacBufferStatus *priority_mac_buffer,*temp_mac_buffer,*temp_free;
	RlcBufferRequest *mac_rlc_report,*temp_rlc_report;
	LogicalChannelBj *temp_logicalchannel_bj;
	u32 *res_length;
	u32 statuslength,retxlength,txlength;
	struct list_head *pos,*p;

	FIN(ResourceAllocation_Algorithm(int resource_len));
	SV_PTR_GET(rlc_mac_sv);
	priority_mac_buffer=NULL;
	temp_mac_buffer=NULL;
	temp_free=NULL;
	temp_rlc_report=NULL;

	mac_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	lcbj_lessthan_zero =(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
	INIT_LIST_HEAD(&(lcbj_lessthan_zero->list));
	/*while(mac_rlc_report==0)//申请内存失败，再次申请直到成功
	{
		mac_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	}*/
	INIT_LIST_HEAD(&(mac_rlc_report->list));
	temp_logicalchannel_bj=NULL;
	res_length=&resource_len;
	statuslength=0,retxlength=0,txlength=0;
	pos=NULL,p=NULL;
	priority_mac_buffer=PrioritySort();
	list_for_each_entry(temp_mac_buffer, &(priority_mac_buffer->list),list)//遍历
	{
		temp_buffer_lcid=temp_mac_buffer->lcid;
		if(*res_length<=0)//当前资源已经分配完
			break;
		temp_logicalchannel_bj=NULL;
		list_for_each_entry(temp_logicalchannel_bj, (&(SV(LogicalChannel_Bj)->list)),list)//遍历
		{
			if(temp_buffer_lcid==(temp_logicalchannel_bj->lcid))
			{
				// printk(KERN_INFO"lcid:%d  lcbj:%d\n",temp_buffer_lcid,temp_logicalchannel_bj->lcbj);
				statuslength=(temp_mac_buffer->RlcRequestparams->statusPduSize)+(temp_mac_buffer->RlcRequestparams->statusPduHeader);
				retxlength=(temp_mac_buffer->RlcRequestparams->retxQueueSize)+(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
				txlength=(temp_mac_buffer->RlcRequestparams->txQueueSize)+(temp_mac_buffer->RlcRequestparams->txQueueHeader);
				if((temp_logicalchannel_bj->lcbj)==(MAX_BJ/8+1) &&(statuslength>0 || retxlength>0 ||txlength>0))//把当前所有的剩余资源分给改逻辑信道
				{
					if(*res_length>=(statuslength+retxlength+txlength))//TBSize大于总需求
					{
						temp_rlc_report=Mac_RlcRequest_copy(temp_mac_buffer->RlcRequestparams);
						temp_rlc_report->statusPduSize=temp_rlc_report->statusPduSize+*res_length-(statuslength+retxlength+txlength);//把多的资源给statusPduSize
						list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
						*res_length=0;//剩余资源为0
						//temp_logicalchannel_bj->lcbj-=(statuslength+retxlength+txlength);
						break;
					}
					else if(*res_length>=statuslength)//先分状态PDU
					{
						ResourceAllocation_logicalchannel_firststatusbuffer(mac_rlc_report,temp_mac_buffer,temp_logicalchannel_bj,statuslength,retxlength,txlength,res_length);
						break;
					}
					else if(*res_length <statuslength)//小于状态PDU
					{
						ResourceAllocation_logicalchannel_firstretxbuffer(mac_rlc_report,temp_mac_buffer,temp_logicalchannel_bj,retxlength,txlength,res_length);
						break;
					}
				}
				else if((temp_logicalchannel_bj->lcbj)>=0)
				{
					ResourceAllocation_foreach_logicalchannel(mac_rlc_report,temp_mac_buffer, temp_logicalchannel_bj,statuslength,retxlength,txlength,res_length);
					break;
				}
				else if((temp_logicalchannel_bj->lcbj)<0)
				{
					temp_lessthan_zero=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
					fsm_mem_cpy(temp_lessthan_zero,temp_mac_buffer,sizeof(MacBufferStatus));
					list_add_tail(&(temp_lessthan_zero->list), &(lcbj_lessthan_zero->list));
				}			
				break;
			}
		}
	}
	if(*res_length>0)
	{
		temp_mac_buffer=NULL;
		list_for_each_entry(temp_mac_buffer, &(lcbj_lessthan_zero->list),list)//遍历
		{	
			if(statuslength>0 || retxlength>0 ||txlength>0)
			{
				ResourceAllocation_foreach_logicalchannel(mac_rlc_report,temp_mac_buffer, temp_logicalchannel_bj,statuslength,retxlength, txlength,res_length);
			}
		}
	}
	report_array=RlcRequest_listtoarray(mac_rlc_report,num);    
	pos=NULL,p=NULL;
	list_for_each_safe(pos,p,&(priority_mac_buffer->list))//遍历
	{
		temp_free=list_entry(pos,MacBufferStatus,list);
		fsm_mem_free(temp_free->RlcRequestparams);
		list_del_init(&(temp_free->list));
		fsm_mem_free(temp_free);
	}
	fsm_mem_free(priority_mac_buffer);
	pos=NULL,p=NULL;
	list_for_each_safe(pos,p,&(lcbj_lessthan_zero->list))//遍历
	{
		temp_free=list_entry(pos,MacBufferStatus,list);
		fsm_mem_free(temp_free->RlcRequestparams);
		list_del_init(&(temp_free->list));
		fsm_mem_free(temp_free);
	}
	fsm_mem_free(lcbj_lessthan_zero);
	Empty_MACBuffer_Rlc();//分配资源后，清空MacBufferStatus
	FRET(report_array);
}



/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:删除lcid对应的逻辑信道属性,也删除该逻辑信道的BJ
->Input:lcid_delete逻辑信道id，SV(LogicalChannel_Config)，SV(LogicalChannel_Bj)
->Output:int *num:指出report数组大小，void *:RLC report指针
->Special:
*******************************
*/
void Delete_LogicalChannel_ConfigInfo(u16 lcid_delete)
{
	FIN(Delete_LogicalChannel_ConfigInfo(u16 lcid_delete));
	SV_PTR_GET(rlc_mac_sv);
	LogicalChannelConfigInfo *temp_free=NULL;
	LogicalChannelBj *temp=NULL;
	struct list_head *pos=NULL,*p=NULL;

	list_for_each_safe(pos,p,(&(SV(LogicalChannel_Config)->list)))
	{
		temp_free=list_entry(pos,LogicalChannelConfigInfo,list);
		if(temp_free->lcid==lcid_delete)
		{
			list_del_init(&(temp_free->list));//delete a node of LogicalChannel_Config
			fsm_mem_free(temp_free);
		}
	}
	pos=NULL,p=NULL;
	list_for_each_safe(pos,p,(&(SV(LogicalChannel_Bj)->list)))
	{
		temp=list_entry(pos,LogicalChannelBj,list);
		if(temp->lcid==lcid_delete)
		{
			list_del_init(&(temp->list));//delete a node of LogicalChannel_Bj
			fsm_mem_free(temp);
		}
	}
	FOUT;
}

/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:清空MAC层的RLC请求Buffer
->Input:SV(MacBuffer_RLC)
->Output:
->Special:
*******************************
*/
void Empty_MACBuffer_Rlc(void)
{
	FIN(Empty_MACBuffer_Rlc(void));
	SV_PTR_GET(rlc_mac_sv);
	struct list_head *pos=NULL,*p=NULL;
	MacBufferStatus *temp_free=NULL;

	list_for_each_safe(pos,p,(&(SV(MacBuffer_RLC)->list)))//遍历
	{
		temp_free=list_entry(pos,MacBufferStatus,list);
		fsm_mem_free(temp_free->RlcRequestparams);
		list_del_init(&(temp_free->list));
		fsm_mem_free(temp_free);
	}	
	FOUT;
}


/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:释放UE MAC SCHEDULER资源
->Input:
->Output:
->Special:
*******************************
*/
void Free_Uemac_Scheduler_Resource(void)
{
	FIN(Free_Uemac_Scheduler_Resource(void));
	struct list_head *pos,*p;
	MacBufferStatus *temp_rlc_free=NULL;
	LogicalChannelBj *temp_bj_free=NULL;
	LogicalChannelConfigInfo *temp_lcginfo_free=NULL;//逻辑信道属性

	SV_PTR_GET(rlc_mac_sv);
	fsm_mem_free(SV(bsr));
	//fsm_mem_free(Frame_No_Allocation);
	if(SV(Regularbsr)!=NULL)
	{
		fsm_mem_free(SV(Regularbsr));
		SV(Regularbsr)=NULL;
	}
	if(SV(Periodicbsr)!=NULL)
	{
		fsm_mem_free(SV(Periodicbsr));
		SV(Periodicbsr)=NULL;
	}
	list_for_each_safe(pos,p,(&(SV(MacBuffer_RLC)->list)))//遍历
	{
		temp_rlc_free=list_entry(pos,MacBufferStatus,list);
		fsm_mem_free(temp_rlc_free->RlcRequestparams);
		list_del_init(&(temp_rlc_free->list));
		fsm_mem_free(temp_rlc_free);
	}
	pos=NULL;
	p=NULL;
	list_for_each_safe(pos,p,(&(SV(LogicalChannel_Bj)->list)))//遍历
	{
		temp_bj_free=list_entry(pos,LogicalChannelBj,list);
		list_del_init(&(temp_bj_free->list));
		fsm_mem_free(temp_bj_free);
	}
	pos=NULL;
	p=NULL;
	list_for_each_safe(pos,p,(&(SV(LogicalChannel_Config)->list)))//遍历
	{
		temp_lcginfo_free=list_entry(pos,LogicalChannelConfigInfo,list);
		list_del_init(&(temp_lcginfo_free->list));
		fsm_mem_free(temp_lcginfo_free);
	}
	fsm_mem_free(SV(MacBuffer_RLC));
	fsm_mem_free(SV(LogicalChannel_Bj));
	fsm_mem_free(SV(LogicalChannel_Config));
	
	FOUT;
}



/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:由带宽获取RB大小
->Input:channel_bandwidth系统带宽
->Output:
->Special:
*******************************
*/
u32 GetRbsize(u32  channel_bandwidth)
{
	int rbsize;
	FIN(GetRbsize(u32 channel_bandwidth));
	rbsize=0;

	/*if(channel_bandwidth==1.4)//modified by lhl 为了不使用float
		rbsize=6;*/
	//else
	// {
	switch(channel_bandwidth)
	{
		case 3   : rbsize=15;break;
		case 5   : rbsize=25;break;
		case 10  : rbsize=50;break;
		case 15  : rbsize=75;break;
		case 20  : rbsize=100;break;
		default  : fsm_printf("[UEMAC][getrbsize]信道带宽取值不在定义的6个值中！\n");break;
	}
	//}
	FRET(rbsize);
}
/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:计算
->Input:num底数，n指数
->Output:num的n次幂
->Special:
*******************************
*/
u32 My_Pow(u32 num,u32 n)//计算num的n次幂，其中n为整数
{
	int i;
	FIN(My_Pow(u32 num,u32 n));
	int powint=1;	

	for(i=1;i<=n;i++)
		powint*=num;
	FRET(powint);
}
/*****lhl modified 20140526****
int My_Floor(int x)
{
    FIN(My_Floor(int x));
    int ret=0;
  //  ret=(int)(x+0.000001);
    FRET(ret);
}
int My_Ceil(int x)
{
    FIN(My_Ceil(int x));
    int ret=0;
       //ret=(int)(x-0.000001+1);
    FRET(ret);
}
****lhl modified 20140526*****/

/*
******************************
->Change Data and time: 
->Function:
->Change:
->Details:计算log2(x)
->Input:rb
->Output:log2(rb)
->Special:
*******************************
*/
u32 My_Log2(u32 rb)
{
	u32 ret;
	FIN(My_Log2(u32 rb));

	//double ret;
	ret=0;
	switch(rb)//20140514modified by lhl 为了不使用float
	{
		case 6   : ret=4392 ;break;
		case 15  : ret=6907;break;
		case 25  : ret=8344;break;
		case 75  : ret=11477;break;
		case 100 : ret=12302;break;
		default  :fsm_printf("[UEMAC][my_log2]RB number error ");break;
	}
	FRET(ret);
}//计算以2为底的对数
/************************UL schedule LiuHanLi结束**************************/
