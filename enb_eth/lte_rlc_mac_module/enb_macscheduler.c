#include <linux/if_ether.h>
/****lhl改动20140410****/
#include <linux/list.h>
#include <linux/kernel.h>
#include <stddef.h>
/****lhl改动结束20140410****/
#include "rlc_macfsm.h"
//#include "macpkfmt.h"
//#include "rrc_config.h"




/************************UL schedule LiuHanLi**************************/
/************************初始化MAC各个全局变量参数***********************/
void Init_Enbmac_Scheduler(void)
{
       int i;
	FIN(Init_Enbmac_Scheduler(void));
	SV_PTR_GET(rlc_mac_sv);
	//SV(LCGZeroToData)=false;
	//SV(DATA_WAIT_ALLOCATION)=false;	
	for(i=0;i<USERNUMMAX;i++)
	{
		SV(Enb_MacBuffer_RLC[i])=(struct list_head *)fsm_mem_alloc(sizeof(struct list_head));
		SV(Enb_LogicalChannel_Bj[i])=(struct list_head *)fsm_mem_alloc(sizeof(struct list_head));
		SV(Enb_LogicalChannel_Config[i])=(struct list_head *)fsm_mem_alloc(sizeof(struct list_head));
		INIT_LIST_HEAD(SV(Enb_MacBuffer_RLC[i]));
		INIT_LIST_HEAD(SV(Enb_LogicalChannel_Bj[i]));
		INIT_LIST_HEAD(SV(Enb_LogicalChannel_Config[i]));
	}
	FOUT;
}

/*******20140526 将RLC的数组请求转换为链表请求，为了与之前的设计保持一致,针对一个用户********/
/*****rlc_request_array为每个用户的请求头指针，num是该用户的数组节点个数******/
/*****uesernum:该rlc请求中对应的第uesernum个用户,从0开始*****/
RlcBufferRequest * RlcRequest_arraytolist(RLC_Request *rlc_request_array,int num,int uesernum)//num是请求的长度
{
	int i;
	RlcBufferRequest *rlcrequest;
	RlcBufferRequest *rlcrequest_temp;
	FIN(RlcRequest_arraytolist(RLC_Request *rlc_request_array,int num,int uesernum));
	SV_PTR_GET(rlc_mac_sv);
	rlcrequest=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	while(rlcrequest==0)//申请内存失败，再次申请，直到申请成功
	{
		rlcrequest=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	}
	INIT_LIST_HEAD(&(rlcrequest->list));
	for(i=0;i<num;i++)
	{
		rlcrequest_temp=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
		while(rlcrequest_temp==0)//申请内存失败，再次申请，直到申请成功
		{
			rlcrequest_temp=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
		}
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
	SV(Rlc_Request_Num[uesernum])=num;//当前申请资源的第index个用户的数组节点长，即该用户有数据要发的逻辑信道数
	FRET(rlcrequest);
}


/*******20140529 将RLC的链表请求转换为数组请求，为了与之前的设计保持一致********/
/*******输入RlcBufferRequest为RLC report链表，输出：*num是要发送的IOCTL的buffer长度，返回RLC 报告的头指针***************/
/*******reqnum表示该用户对应的Rlc_Request_Num*******/
RLC_Request * RlcRequest_listtoarray( RlcBufferRequest *rlc_request,int reqnum ,int *num)//num是report的总数
{
	struct list_head *pos,*p;
	RlcBufferRequest *temp;
	int i;
	RLC_Request *rlc_report_temp;
	FIN( RlcRequest_listtoarray( RlcBufferRequest *rlc_request,int reqnum ,int *num));
	SV_PTR_GET(rlc_mac_sv);
	rlc_report_temp=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request)*reqnum);//动态申请report的总个数
	while(rlc_report_temp==0)//申请内存失败，再次申请直到成功
	{
		rlc_report_temp=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request)*reqnum);
	}
	pos=NULL,p=NULL,temp=NULL;
	i=0;
	//rlc_report_temp=(RLC_Request *)((int *)rlc_report);//指针指向数组的首地址
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
    *num=i;//返回RLC report的总个数大小
	pos=NULL,p=NULL,temp=NULL;
	list_for_each_safe(pos,p,&(rlc_request->list))//遍历
	{
        	temp=list_entry(pos,RlcBufferRequest,list);
        	list_del_init(&(temp->list));
        	fsm_mem_free(temp);//释放资源
  	}
	FRET(rlc_report_temp);
}



/***************由RLC层的buffer请求，更新MAC层buffer状态变量****************/
/*********输入RlcBufferRequest* params 为一个用户的RLC请求链表头指针************/
/***list_head  *enb_buffer_rlc为某个用户的BUFFER首地址，如enb_buffer_rlc=SV(Enb_MacBuffer_RLC[index])**/
void * DoRefreshRLCBuffserRequest (RlcBufferRequest* params,struct list_head *enb_buffer_rlc)	
{
	RlcBufferRequest *RLChead=params,*temp=NULL;
	struct list_head *pos,*p;
	MacBufferStatus *temp_mac_bufferstatus=NULL;
	MacBufferStatus *temp_mac_findno=NULL;
	unsigned char temp_lcid,temp_mac_bufferstatus_lcid;
    FIN(DoRefreshRLCBuffserRequest(RlcBufferRequest* params));
    SV_PTR_GET(rlc_mac_sv);	
	bool flag=false;
	struct list_head *MacBuffer_RLC=enb_buffer_rlc;//头指针		
    //SV(DATA_WAIT_ALLOCATION)=true;//有数据待分配
	//fsm_printf("[MAC] received rnti = %d\n", params->rnti);
     list_for_each_safe(pos,p,&(RLChead->list))//遍历
    {
        temp=list_entry(pos,RlcBufferRequest,list);
        temp_lcid=temp->lcid;
        //length_rlc_request++;
        temp_mac_bufferstatus=NULL;
        list_for_each_entry(temp_mac_bufferstatus, MacBuffer_RLC,list)//遍历，查找MAC是否已经保存逻辑信道的数据        
        {
            temp_mac_bufferstatus_lcid=temp_mac_bufferstatus->lcid;
            if(temp_lcid==temp_mac_bufferstatus_lcid)
            {
                temp_mac_bufferstatus->RlcRequestparams=Rlc_MacRequest_copy(temp);
				//fsm_printf("[MAC] temp_mac_bufferstatus->RlcRequestparams->rnti = %d\n", temp_mac_bufferstatus->RlcRequestparams->rnti);
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
			//fsm_printf("[MAC] temp_mac_findno->RlcRequestparams->rnti = %d\n", temp_mac_findno->RlcRequestparams->rnti);
            list_add_tail(&(temp_mac_findno->list), MacBuffer_RLC);
        }
        flag=false;
    }
    temp=NULL;
    list_for_each_safe(pos,p,&(RLChead->list))//遍历
    {
        temp=list_entry(pos,RlcBufferRequest,list);
        list_del_init(&(temp->list));
        fsm_mem_free(temp);//释放资源
    }
    FOUT;
}

/****复制RlcBufferRequest节点，使返回节点可插入MacBufferStatus中，解决next为NULL的问题******/
RLC_Request* Rlc_MacRequest_copy(RlcBufferRequest *temp)
{
    FIN(Rlc_MacRequest_copy(RlcBufferRequest *temp));
	RLC_Request *copy=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
	while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
	}
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

LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_copy(MAC_LogicalChannelConfig_IoctrlMsg *temp)
{
    FIN(LogicalChannel_ConfigInfo_copy(MAC_LogicalChannelConfig_IoctrlMsg *temp));
	LogicalChannelConfigInfo *copy=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo));
	while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo));
	}
	INIT_LIST_HEAD(&(copy->list));
	copy->lcid=temp->logicalChannelIdentity;
	copy->rnti=temp->rnti;
	copy->priority=(temp->logicalChannelConfig).ul_SpecificParameters.priority;
    copy->prioritizedBitRateKbps=(temp->logicalChannelConfig).ul_SpecificParameters.prioritisedBitRate;
    copy->bucketSizeDurationMs=(temp->logicalChannelConfig).ul_SpecificParameters.bucketSizeDuration;
    copy->logicalChannelGroup=(temp->logicalChannelConfig).ul_SpecificParameters.logicalChannelGroup;
	FRET(copy);
}

RlcBufferRequest* Mac_RlcRequest_copy(RLC_Request *temp)//复制RlcBufferRequest节点
{
    FIN(Mac_RlcRequest_copy(RLC_Request *temp));
	RlcBufferRequest *copy=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	}
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

RLC_Request* Mac_MacRequest_copy(RLC_Request *temp)//复制RlcBufferRequest节点
{
    FIN(Mac_MacRequest_copy(RLC_Request *temp));
	RLC_Request *copy=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
	while(copy==0)//申请内存失败，再次申请直到成功
	{
		copy=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
	}
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



/********************每个TTI更新逻辑信道的Bj值20140526modified 更改了无穷大的情况**********************/
/********输入enb_logicalChannel_bj 为Enb_LogicalChannel_Bj[i]头指针(对应一个用户)，需要针对每个用户输入*******/
/******** 输入list_head *enb_logicalChannel_config 为Enb_LogicalChannel_Config[i]头指针，与BJ是一一对应的***********/
void FlushBj(struct  list_head *enb_logicalChannel_bj, struct list_head *enb_logicalChannel_config)
{
	 struct list_head  *head_Bj=enb_logicalChannel_bj;
	 LogicalChannelBj  *temp=NULL;
	LogicalChannelConfigInfo *temp_Logical_Channel_Config;
	int i;
	int temp_Bj_lcid,temp_lcc_lcid,BucketSize;
    	FIN(FlushBj(struct  list_head *enb_logicalChannel_bj, struct list_head *enb_logicalChannel_config));
   	 SV_PTR_GET(rlc_mac_sv);	
	bool flag_tail=false;
	list_for_each_entry(temp, head_Bj,list)//遍历当前用户的BJ链表
	{
		temp_Bj_lcid=temp->lcid;
		flag_tail=false;
		temp_Logical_Channel_Config=NULL;
		list_for_each_entry(temp_Logical_Channel_Config, enb_logicalChannel_config,list)//遍历该用户的逻辑信道属性，找到对应的LCID配置
		 {
		  	if(temp->rnti==temp_Logical_Channel_Config->rnti)//确保是针对该用户的
		  	{
		  		temp_lcc_lcid=temp_Logical_Channel_Config->lcid;
			        if(temp_lcc_lcid==temp_Bj_lcid)
			        {
			            if((temp_Logical_Channel_Config->prioritizedBitRateKbps)==infinity)//无穷大的情况
			            {
			                temp->lcbj=MAX_BJ/8+1;//处理为最大值加1
			                flag_tail=true;
			            }
			            else//不是无穷大时的更新
			            {
				            BucketSize=(temp_Logical_Channel_Config->prioritizedBitRateKbps)*(temp_Logical_Channel_Config->bucketSizeDurationMs)/8;
			                    if(temp->lcbj < BucketSize)
			                       temp->lcbj=temp->lcbj+(temp_Logical_Channel_Config->prioritizedBitRateKbps)*TTI/8;//除以8是转换为byte
			                    else
			                        temp->lcbj=BucketSize;
			                    flag_tail=true;
						break;
					}		                	
			        }

		  	}
			else//逻辑信道与用户的BJ不符合
			{
				 printk(KERN_INFO"Bj and logical channel ERROR");
				 FOUT;
			}
		}
		if(flag_tail==false)//没有找到想匹配的逻辑信道
            		printk(KERN_INFO"FlushBj ERROR");//LogicalChannelConfig和LogicalChannelConfigBj不匹配
	}
	FOUT;
}

/**************(对每个用户)*根据逻辑信道优先级对逻辑信道排序***************/
/******输入为Enb_LogicalChannel_Config[i],需要针对每个用户做输入********/
LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_Rank_Priority(struct list_head * enb_logicalChannel_config)
{

    FIN(LogicalChannel_ConfigInfo_Rank_Priority(struct list_head * enb_logicalChannel_config));
    SV_PTR_GET(rlc_mac_sv);
	LogicalChannelConfigInfo *lcc_info_priority=NULL,*temp=NULL,*temp_logicalChannel=NULL;
	lcc_info_priority=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo ));
	while(lcc_info_priority==0)//申请内存失败，再次申请直到成功
	{
		lcc_info_priority=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo ));
	}
	INIT_LIST_HEAD(&(lcc_info_priority->list));
	LogicalChannelConfigInfo *next_node=NULL;
	LogicalChannelConfigInfo *temp_next=NULL;
	bool flag_add=false;
	list_for_each_entry(temp, enb_logicalChannel_config,list)//遍历
	{
	    next_node=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo ));
	    while(next_node==0)//申请内存失败，再次申请直到成功
	    {
		 next_node=(LogicalChannelConfigInfo *)fsm_mem_alloc(sizeof(LogicalChannelConfigInfo ));
	    }
		//fsm_printf("[mac] before sort lcid : %d\n", temp->lcid);
	    next_node->lcid=temp->lcid;
	 	next_node->rnti=temp->rnti;
        next_node->priority=temp->priority;
        next_node->prioritizedBitRateKbps=temp->prioritizedBitRateKbps;
        next_node->bucketSizeDurationMs=temp->bucketSizeDurationMs;
        next_node->logicalChannelGroup=temp->logicalChannelGroup;
        INIT_LIST_HEAD(&(next_node->list));
        if (list_empty(&(lcc_info_priority->list)))
            list_add_tail(&(next_node->list), &(lcc_info_priority->list));
        else
        {
            flag_add=false;
            list_for_each_entry(temp_next, &(lcc_info_priority->list),list)//遍历
            {
                if ((next_node->priority) <= (temp_next->priority))
                {
                    list_add_tail(&(next_node->list), &(temp_next->list));//把next_node加到temp_next的前面
                    flag_add=true;
                    break;
                }
            }
            if(flag_add==false)
                list_add_tail(&(next_node->list), &(lcc_info_priority->list));
        }
    }
	FRET(lcc_info_priority);//返回具有优先级排队的LogicalChannelConfigInfo头指针
}

/******根据逻辑信道优先级顺序，对MacBufferStatus链表按优先级排序********/
/******输入为Enb_LogicalChannel_Config[i],需要针对每个用户做输入********/
/******输入为enb_buffer_rlc[i],需要针对每个用户做输入********/
MacBufferStatus* PrioritySort(struct  list_head * enb_logicalChannel_config,struct list_head *enb_buffer_rlc)
{

	MacBufferStatus *PriorityMacBufferStatus,*temp_mbs=NULL,*temp_node;
	LogicalChannelConfigInfo *Priority_logicalChannel=NULL,*temp_logicalChannel=NULL;
	int flag;//标志逻辑信道信息与BUFFER是否匹配
	struct list_head *pos,*p;
    FIN(PrioritySort(struct  list_head * enb_logicalChannel_config,struct list_head *enb_buffer_rlc));
    SV_PTR_GET(rlc_mac_sv);
	if (list_empty(enb_buffer_rlc))
	{
		printk(KERN_INFO"PrioritySort:input enb_mac buffer error ");
		FRET(NULL);
	}
	else
	{
		PriorityMacBufferStatus=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
		//while(PriorityMacBufferStatus==0)//申请内存失败，再次申请直到成功
		//{
			//PriorityMacBufferStatus=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
		 //}
		unsigned char temp_Bj_lcid;
	    INIT_LIST_HEAD(&(PriorityMacBufferStatus->list));
		Priority_logicalChannel=LogicalChannel_ConfigInfo_Rank_Priority(enb_logicalChannel_config);
		flag=false;
		list_for_each_entry(temp_logicalChannel, &(Priority_logicalChannel->list),list)//遍历
		{
			
			temp_Bj_lcid=temp_logicalChannel->lcid;
			list_for_each_entry(temp_node, enb_buffer_rlc,list)//遍历
			{
				//fsm_printf("[mac] temp_Bj_lcid = %d, temp_node->lcid = %d\n", temp_Bj_lcid, temp_node->lcid);
			    if(temp_Bj_lcid==temp_node->lcid)
			    {
			        temp_mbs=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
				while(temp_mbs==0)//申请内存失败，再次申请直到成功
				{
					temp_mbs=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
				 }
	                INIT_LIST_HEAD(&(temp_mbs->list));
			        temp_mbs->lcid=temp_node->lcid;
			        temp_mbs->RlcRequestparams=Mac_MacRequest_copy(temp_node->RlcRequestparams);
			        list_add_tail(&(temp_mbs->list), &(PriorityMacBufferStatus->list));
				flag=true;
			        break;
			    }
			}
		}
	}
	if(flag==false)
	{
		printk(KERN_INFO"the function of PrioritySort() error :channel and buffer don't match\n");		
	}
	temp_logicalChannel=NULL;
	pos=NULL,p=NULL;
	list_for_each_safe(pos,p,&(Priority_logicalChannel->list))//遍历
    	{
	        temp_logicalChannel=list_entry(pos,LogicalChannelConfigInfo,list);
	        list_del_init(&(temp_logicalChannel->list));//删除节点
	        fsm_mem_free(temp_logicalChannel);//释放资源
   	 }
	FRET(PriorityMacBufferStatus);
}

unsigned short Getrnti_From_Enbbuffer(struct list_head *enb_buffer_rlc)
{
	struct list_head *pos=NULL,*p=NULL;
	MacBufferStatus *temp_rlc;
	unsigned short rnti;
	FIN(Getrnti_From_Enbbuffer(struct list_head *enb_buffer_rlc));
	SV_PTR_GET(rlc_mac_sv);
	 list_for_each_safe(pos,p,enb_buffer_rlc)
	 {
	 	temp_rlc=list_entry(pos,MacBufferStatus,list);
		rnti=temp_rlc->RlcRequestparams->rnti;
		break;
	 }
	 FRET(rnti);
}
 struct list_head * Get_LogicalChannelInfo_From_Rnti(unsigned short m_rnti)
 {
 	int i;
       struct list_head *pos=NULL,*p=NULL;
	LogicalChannelConfigInfo *temp_lc_config;
	FIN(Get_LogicalChannelInfo_From_Rnti(unsigned short m_rnti));
	SV_PTR_GET(rlc_mac_sv);
	for(i=0;i<USERNUMMAX;i++)//遍历用户
	{
		pos=NULL,p=NULL;
		 list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Config[i]))
		{
			temp_lc_config=list_entry(pos,LogicalChannelConfigInfo,list);
			if (m_rnti==temp_lc_config->rnti)//查找具有相同RNTI的用户
			{
				FRET(SV(Enb_LogicalChannel_Config[i]));//返回该用户逻辑信道头指针
			}
			else
				break;
		 }
	}
	printk(KERN_INFO"Get_LogicalChannelInfo_From_Rnti:error\n");
	FRET(NULL);
 }
 
 struct list_head * Get_Enbbuffer_From_Rnti(unsigned short m_rnti)
 {
 	int i;
       struct list_head *pos=NULL,*p=NULL;
	MacBufferStatus *temp;
	FIN(Get_Enbbuffer_From_Rnti(unsigned short m_rnti));
	SV_PTR_GET(rlc_mac_sv);
	for(i=0;i<USERNUMMAX;i++)//遍历用户
	{
		pos=NULL,p=NULL;
		 list_for_each_safe(pos,p,SV(Enb_MacBuffer_RLC[i]))
		{
			temp=list_entry(pos,MacBufferStatus,list);
			if (m_rnti==temp->RlcRequestparams->rnti)//查找具有相同RNTI的用户
			{
				FRET(SV(Enb_MacBuffer_RLC[i]));//返回该用户逻辑信道头指针
			}
			else
				break;
		 }
	}
	printk(KERN_INFO"Get_Enbbuffer_From_Rnti:error\n");
	FRET(NULL);
 }


/************ 功能:由ENB某个用户的请求，找到对应用户的逻辑信道属性*************/
/*********输入为某个用户的数据发送请求*******/
/****输出为该用户对应的逻辑信道属性链表头指针******/
 struct list_head * Get_LogicalChannelInfo_From_Enbbuffer(struct list_head *enb_buffer_rlc)
{
	int i;
	int rnti_temp;
	struct list_head *pos=NULL,*p=NULL;
	LogicalChannelConfigInfo *temp_lc_config;
	MacBufferStatus *temp_rlc;
	FIN(Get_LogicalChannelInfo_From_Enbbuffer(struct list_head *enb_buffer_rlc));
	SV_PTR_GET(rlc_mac_sv);
	 list_for_each_safe(pos,p,enb_buffer_rlc)
	 {
	 	temp_rlc=list_entry(pos,MacBufferStatus,list);
		rnti_temp=temp_rlc->RlcRequestparams->rnti;
		break;
	 } 	
	if(list_empty(enb_buffer_rlc))//输入用户的buffer链表为空
	{
		printk(KERN_INFO"Get_LogicalChannelInfo_From_Enbbuffer:input enb_mac buffer error ");
		FRET(NULL);
	}
	else
	{
		for(i=0;i<USERNUMMAX;i++)//遍历用户
		{
			pos=NULL,p=NULL;
			 list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Config[i]))
			{
				temp_lc_config=list_entry(pos,LogicalChannelConfigInfo,list);
				if (rnti_temp==temp_lc_config->rnti)//查找具有相同RNTI的用户
				{
					FRET(SV(Enb_LogicalChannel_Config[i]));//返回该用户逻辑信道头指针
				}
				else
					break;
			 }
		}
	}
	printk(KERN_INFO"logical channel and user rnti don't match");
	FRET(NULL);//返回空指针
	
}

 struct list_head * Get_LogicalChannelBj_From_Rnti(unsigned short m_rnti)
 {
 	int i;	
	struct list_head *pos=NULL,*p=NULL;
	LogicalChannelBj *temp_bj;
	MacBufferStatus *temp_rlc;
	FIN(Get_LogicalChannelBj_From_Rnti(unsigned short m_rnti));
	SV_PTR_GET(rlc_mac_sv);
	for(i=0;i<USERNUMMAX;i++)//遍历用户
	{
		pos=NULL,p=NULL;
		 list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Bj[i]))
		{
			temp_bj=list_entry(pos,LogicalChannelBj,list);
			if(m_rnti==temp_bj->rnti)//查找具有相同RNTI的用户
			{
				FRET(SV(Enb_LogicalChannel_Bj[i]));//返回该用户BJ 头指针
			}
			else
				break;
		 }
	}
	printk(KERN_INFO"Get_LogicalChannelBj_From_Rnti:error\n");
	FRET(NULL);//返回空指针

 }

/************ 功能:由ENB某个用户的请求，找到对应用户的逻辑信道的BJ*************/
/*********输入为某个用户的数据发送请求*******/
/****输出为该用户对应的逻辑信道BJ 链表头指针******/
 struct list_head * Get_LogicalChannelBj_From_Enbbuffer(struct list_head *enb_buffer_rlc)
{
	int i;
	int rnti_temp;
	struct list_head *pos=NULL,*p=NULL;
	LogicalChannelBj *temp_bj;
	MacBufferStatus *temp_rlc;
	FIN(Get_LogicalChannelBj_From_Enbbuffer(struct list_head *enb_logicalchannel_bj));
	SV_PTR_GET(rlc_mac_sv);
	 list_for_each_safe(pos,p,enb_buffer_rlc)
	 {
	 	temp_rlc=list_entry(pos,MacBufferStatus,list);
		rnti_temp=temp_rlc->RlcRequestparams->rnti;
		break;
	 }
	if(list_empty(enb_buffer_rlc))//输入用户的buffer链表为空
	{
		printk(KERN_INFO"Get_LogicalChannelInfo_From_Enbbuffer:input enb_mac buffer error ");
		FRET(NULL);
	}
	else
	{
		for(i=0;i<USERNUMMAX;i++)//遍历用户
		{
			pos=NULL,p=NULL;
			 list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Bj[i]))
			{
				temp_bj=list_entry(pos,LogicalChannelBj,list);
				if(rnti_temp==temp_bj->rnti)//查找具有相同RNTI的用户
				{
					FRET(SV(Enb_LogicalChannel_Bj[i]));//返回该用户BJ 头指针
				}
				else
					break;
			 }
		}
	}
	printk(KERN_INFO"logical channel BJ and user rnti don't match");
	FRET(NULL);//返回空指针	
}

void Init_LogicalChannel_ConfigInfo(MAC_LogicalChannelConfig_IoctrlMsg *lc_info)
{
    struct list_head *pos=NULL,*p=NULL;
    LogicalChannelConfigInfo *temp_lc_config=NULL;
    LogicalChannelConfigInfo *temp_add=NULL;
    LogicalChannelBj *temp_bj=NULL;
    FIN(Init_LogicalChannel_ConfigInfo(MAC_LogicalChannelConfig_IoctrlMsg *lc_info));
    SV_PTR_GET(rlc_mac_sv);
    bool flag=false;    
    int temp_lcid=lc_info->logicalChannelIdentity;
    int temp_rnti=lc_info->rnti;
    int temp_lc_config_lcid;
	int i=0;
	for(i=0;i<USERNUMMAX;i++)
	{	
		 if(list_empty( SV(Enb_LogicalChannel_Config[i])))//链表为空
		 {
		 	  temp_add=LogicalChannel_ConfigInfo_copy(lc_info);
		        list_add_tail(&(temp_add->list), SV(Enb_LogicalChannel_Config[i]));//添加该用户的逻辑信道配置
		        temp_bj=(LogicalChannelBj *)fsm_mem_alloc(sizeof(LogicalChannelBj));//同时添加逻辑信道BJ
		        while(temp_bj==0)//申请内存失败，再次申请直到成功
		        {
		            temp_bj=(LogicalChannelBj *)fsm_mem_alloc(sizeof(LogicalChannelBj));
		        }		       
		        INIT_LIST_HEAD(&(temp_bj->list));
		        temp_bj->lcid=lc_info->logicalChannelIdentity;
			temp_bj->rnti=lc_info->rnti;
		        if((temp_add->prioritizedBitRateKbps)==infinity)
//无穷的处理为最大
		            temp_bj->lcbj=MAX_BJ/8+1;
		        else
		            temp_bj->lcbj=0;//其他情况初始化为0
		        list_add_tail(&(temp_bj->list), SV(Enb_LogicalChannel_Bj[i]));//因为BJ 和逻辑信道是一一对应的
			//break;
			FOUT;
		 }
		 else//链表非空
		 {
		 	 list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Config[i]))//遍历
			    {
			    	 temp_lc_config=list_entry(pos,LogicalChannelConfigInfo,list);
			    	if(temp_lc_config->rnti==temp_rnti)//若是针对该用户的配置
			    	{
			    		pos=NULL,p=NULL;
					flag=false;
			    		 list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Config[i]))//重新遍历，寻找是否已经配置了该逻辑信道，如是则更新，否则添加节点
			    		 {
			    		 	temp_lc_config=list_entry(pos,LogicalChannelConfigInfo,list);
			    		 	temp_lc_config_lcid=temp_lc_config->lcid;
					        if(temp_lcid==temp_lc_config_lcid)//已经配置了该信道，则更新信息
					        {
					            if((lc_info->logicalChannelConfig).haveUl_SpecificParameters==true)
					            {
					                temp_lc_config->priority=(lc_info->logicalChannelConfig).ul_SpecificParameters.priority;
					                temp_lc_config->prioritizedBitRateKbps=(lc_info->logicalChannelConfig).ul_SpecificParameters.prioritisedBitRate;
					                temp_lc_config->bucketSizeDurationMs=(lc_info->logicalChannelConfig).ul_SpecificParameters.bucketSizeDuration;
					                temp_lc_config->logicalChannelGroup=(lc_info->logicalChannelConfig).ul_SpecificParameters.logicalChannelGroup;
					                flag=true;
					               // break;
					               FOUT;
					            }
					        }
			    		 }
	  				   if(flag==false)//如果没有配置该信道的信息，则添加信息
	  				    {
	  				        temp_add=LogicalChannel_ConfigInfo_copy(lc_info);
	  				        list_add_tail(&(temp_add->list), SV(Enb_LogicalChannel_Config[i]));
	  				        temp_bj=(LogicalChannelBj *)fsm_mem_alloc(sizeof(LogicalChannelBj));
	  				        while(temp_bj==0)//申请内存失败，再次申请直到成功
	  				        {
	  				            temp_bj=(LogicalChannelBj *)fsm_mem_alloc(sizeof(LogicalChannelBj));
	  				        }
	  				        INIT_LIST_HEAD(&(temp_bj->list));
	  				        temp_bj->lcid=lc_info->logicalChannelIdentity;
						temp_bj->rnti=lc_info->rnti;
	  				        if((temp_add->prioritizedBitRateKbps)==infinity)
	  				            temp_bj->lcbj=MAX_BJ/8+1;
	  				        else
	  				            temp_bj->lcbj=0;
	  				        list_add_tail(&(temp_bj->list), SV(Enb_LogicalChannel_Bj[i]));
						//break;
						FOUT;
	  				    }
				        
			    	}
				else
					break;//不是针对该用户的配置，跳到下一个用户
		 	 }
			//break;
		 }
	}
    FOUT;
}


/*
******************************
->Change Data and time: 20141008
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
void leftresource_morethan_retxbuffer_eachlogicalchannel(RlcBufferRequest *mac_rlc_report,RlcBufferRequest *temp_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int retxlength,int txlength,int *res_length)
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
->Change Data and time: 20141008
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
void leftresource_lessthan_retxbuffer_eachlogicalchannel(RlcBufferRequest *mac_rlc_report,RlcBufferRequest *temp_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int retxlength,int txlength,int *res_length)
{
	FIN(leftresource_lessthan_retxbuffer_eachlogicalchannel());
	if(*res_length>=(temp_mac_buffer->RlcRequestparams->retxQueueHeader))//大于头部
	{
		temp_logicalchannel_bj->lcbj-=*res_length;
		temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
		*res_length=*res_length-(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
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
->Change Data and time: 20141008
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
void ResourceAllocation_logicalchannel_firstretxbuffer(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int retxlength,int txlength,int *res_length)
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
->Change Data and time: 20141008
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
void ResourceAllocation_logicalchannel_firststatusbuffer(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int statuslength,int retxlength,int txlength,int *res_length)
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
->Change Data and time: 20141008
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
void ResourceAllocation_foreach_logicalchannel(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int statuslength,int retxlength,int txlength,int *res_length)
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
->Change Data and time: 20141008
->Function:
->Change:为了减少函数复杂度，使用多个子函数
->Details:UE侧资源分配算法：令牌桶
->Input:msg资源大小，SV(LogicalChannel_Bj)，
->Output:int *num:指出report数组大小，void *:RLC report指针
->Special:
*******************************
*/

//void * ResourceAllocation_Algorithm(int resource_len,int *num)//num输出，指出report数组大小
RLC_Request * ResourceAllocation_Algorithm(int resource_len,struct list_head *enb_buffer_rlc,struct list_head * enb_logicalChannel_config,struct  list_head *enb_logicalChannel_bj,int *num,int buffer_len)//num输出，指出report数组大?{
{
	
	//void *report_array;
	RLC_Request *report_array;
	//int *report_num;
	MacBufferStatus *lcbj_lessthan_zero;
	MacBufferStatus *temp_lessthan_zero;
	int temp_buffer_lcid;
	MacBufferStatus *priority_mac_buffer,*temp_mac_buffer,*temp_free;
	RlcBufferRequest *mac_rlc_report,*temp_rlc_report;
	LogicalChannelBj *temp_logicalchannel_bj;    
	int *res_length;
	int statuslength,retxlength,txlength;
	struct list_head *pos,*p;
	FIN(ResourceAllocation_Algorithm(int resource_len));
	SV_PTR_GET(rlc_mac_sv);
	priority_mac_buffer=NULL;
	temp_mac_buffer=NULL;
	temp_free=NULL;
	mac_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
	temp_rlc_report=NULL;
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
	//priority_mac_buffer=PrioritySort();
	priority_mac_buffer=PrioritySort(enb_logicalChannel_config,enb_buffer_rlc);

	list_for_each_entry(temp_mac_buffer, &(priority_mac_buffer->list),list)//遍历
	{
		temp_buffer_lcid=temp_mac_buffer->lcid;
		if(*res_length<=0)//当前资源已经分配完
			break;
		temp_logicalchannel_bj=NULL;

		list_for_each_entry(temp_logicalchannel_bj,enb_logicalChannel_bj,list)//遍历
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
					fsm_printf("lcbj>=0\n");
					ResourceAllocation_foreach_logicalchannel(mac_rlc_report,temp_mac_buffer, temp_logicalchannel_bj,statuslength,retxlength,txlength,res_length);
					break;
				}
				else if((temp_logicalchannel_bj->lcbj)<0)
				{	
					fsm_printf("lcbj<0\n");
					temp_lessthan_zero=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
					INIT_LIST_HEAD(&(temp_lessthan_zero->list));				
					
					fsm_mem_cpy(temp_lessthan_zero,temp_mac_buffer,sizeof(MacBufferStatus));
					temp_lessthan_zero->RlcRequestparams=(RLC_Request *)fsm_mem_alloc(sizeof(RLC_Request));
					fsm_mem_cpy(temp_lessthan_zero->RlcRequestparams,temp_mac_buffer->RlcRequestparams,sizeof(RLC_Request));
					list_add_tail(&(temp_lessthan_zero->list), &(lcbj_lessthan_zero->list));
					break;
				}			
				
			}
		}
	}

	if(*res_length>0)
	{
		temp_mac_buffer=NULL;
		//fsm_printf("*res_length>0\n");
		list_for_each_entry(temp_mac_buffer, &(lcbj_lessthan_zero->list),list)//遍历
		{	
			if(statuslength>0 || retxlength>0 ||txlength>0)
			{
				//fsm_printf("*res_length>0 inner\n");
				ResourceAllocation_foreach_logicalchannel(mac_rlc_report,temp_mac_buffer, temp_logicalchannel_bj,statuslength,retxlength, txlength,res_length);
			}
		}
	}
	//report_array=RlcRequest_listtoarray(mac_rlc_report,num); 

	//fsm_printf("mac_rlc_report:txQueueHeader:%d,mac_rlc_report->txQueueSize:%d\n",mac_rlc_report->txQueueHeader,mac_rlc_report->txQueueSize);
	report_array=RlcRequest_listtoarray(mac_rlc_report,buffer_len,num);

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
		temp_free=NULL;
		temp_free=list_entry(pos,MacBufferStatus,list);
		fsm_mem_free(temp_free->RlcRequestparams);
		list_del_init(&(temp_free->list));
		if(temp_free !=NULL)
			fsm_mem_free(temp_free);
	}
	fsm_mem_free(lcbj_lessthan_zero);
	//Empty_MACBuffer_Rlc();//分配资源后，清空MacBufferStatus
	
	Empty_MACBuffer_Rlc(enb_buffer_rlc);
	
	FRET(report_array);
}






/***********************资源分配**************************/
/***list_head  *enb_buffer_rlc为某个用户的BUFFER首地址，如enb_buffer_rlc=SV(Enb_MacBuffer_RLC[index])*********/
/***************tb_size为每个用户 的资源***********************/
/*****enb_logicalChannel_config为某个用户的逻辑信道配置，调用之前需要根据	RNTI找出与用户匹配的逻辑信道配置******/
/******buffer_len:是int Rlc_Request_Num[USERNUMMAX]中取值，对应enb_buffer_rlc的第几个用户的链表长度******/
/*******输出参数：rlc_report_num是当前用户的report链表长度，由其他函数调度后需要释放内存******/
/*****输出参数:RLC_Request *是当前用户report的数组首地址，由其他函数调用后需要释放内存****/
/*****该函数只能得到一个用户的report首地址和大小，如果要发送所有用户最终的
report ，需要将所有用户的report组装起来和rlc_report_num加起来放在最终report的最前面****/
RLC_Request * DoResourceAllocation(int tb_size,struct list_head *enb_buffer_rlc,struct  list_head * enb_logicalChannel_config,struct  list_head *enb_logicalChannel_bj,int buffer_len,int *rlc_report_num)//资源分配
{
	//int num;
	RLC_Request *rlc_temp=NULL;
	RLC_Request *UEmac_Rlc_Report=NULL;
    FIN( DoResourceAllocation(int tb_size,struct list_head *enb_buffer_rlc,struct  list_head * enb_logicalChannel_config,struct  list_head *enb_logicalChannel_bj,int buffer_len,int *rlc_report_num));
    SV_PTR_GET(rlc_mac_sv);	
	UEmac_Rlc_Report=ResourceAllocation_Algorithm(tb_size,enb_buffer_rlc,enb_logicalChannel_config,enb_logicalChannel_bj,rlc_report_num,buffer_len);
	//SV(DATA_WAIT_ALLOCATION)=false;  
	FRET(UEmac_Rlc_Report);
}
/*******ENB侧资源分配算法：令牌桶,RlcBufferRequest MACReportToRlc****************/
/***list_head  *enb_buffer_rlc为某个用户的BUFFER首地址，如enb_buffer_rlc=SV(Enb_MacBuffer_RLC[index])*********/
/*RLC_Request * ResourceAllocation_Algorithm(int resource_len,struct list_head *enb_buffer_rlc,struct list_head * enb_logicalChannel_config,struct  list_head *enb_logicalChannel_bj,int *num,int buffer_len)//num输出，指出report数组大小
{
    RLC_Request *report_array;
    //int *report_num;
    MacBufferStatus *lcbj_lessthan_zero;
    MacBufferStatus *temp_lessthan_zero;
    FIN(ResourceAllocation_Algorithm(int msg,struct list_head *enb_buffer_rlc,struct list_head * enb_logicalChannel_config,struct  list_head *enb_logicalChannel_bj,int *num,int index));
    SV_PTR_GET(rlc_mac_sv);
    MacBufferStatus *priority_mac_buffer=NULL,*temp_mac_buffer=NULL,*temp_free=NULL;
    RlcBufferRequest *mac_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest)),*temp_rlc_report=NULL;
    lcbj_lessthan_zero=(MacBufferStatus *)fsm_mem_alloc(sizeof(MacBufferStatus));
    INIT_LIST_HEAD(&(lcbj_lessthan_zero->list));
    while(mac_rlc_report==0)//申请内存失败，再次申请直到成功
    {
    	mac_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
    }
    INIT_LIST_HEAD(&(mac_rlc_report->list));
    LogicalChannelBj *temp_logicalchannel_bj=NULL;
    int temp_buffer_lcid;
    int *res_length=resource_len;
    int statuslength=0,retxlength=0,txlength=0;
    struct list_head *pos=NULL,*p=NULL;
    priority_mac_buffer=PrioritySort(enb_logicalChannel_config,enb_buffer_rlc);
    list_for_each_entry(temp_mac_buffer, &(priority_mac_buffer->list),list)//遍历
    {
        temp_buffer_lcid=temp_mac_buffer->lcid;
	if(res_length<=0)//当前资源已经分配完
		break;
		temp_logicalchannel_bj=NULL;
        list_for_each_entry(temp_logicalchannel_bj, enb_logicalChannel_bj,list)//遍历
        {
            if(temp_buffer_lcid==(temp_logicalchannel_bj->lcid))
            {
               // printk(KERN_INFO"lcid:%d  lcbj:%d\n",temp_buffer_lcid,temp_logicalchannel_bj->lcbj);

            	if((temp_logicalchannel_bj->lcbj)==(MAX_BJ/8+1))//把当前所有的剩余资源分给改逻辑信道
            	{
            	    //printk(KERN_INFO"res_length:%d\n",res_length);

                    statuslength=(temp_mac_buffer->RlcRequestparams->statusPduSize)+(temp_mac_buffer->RlcRequestparams->statusPduHeader);
                    retxlength=(temp_mac_buffer->RlcRequestparams->retxQueueSize)+(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
                    txlength=(temp_mac_buffer->RlcRequestparams->txQueueSize)+(temp_mac_buffer->RlcRequestparams->txQueueHeader);
                    if(statuslength>0 || retxlength>0 ||txlength>0)
                    {
                        if(res_length>=(statuslength+retxlength+txlength))//TBSize大于总需求
                        {
                            temp_rlc_report=Mac_RlcRequest_copy(temp_mac_buffer->RlcRequestparams);
				temp_rlc_report->statusPduSize=temp_rlc_report->statusPduSize+res_length-(statuslength+retxlength+txlength);//把多的资源给statusPduSize
                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                            res_length=0;//剩余资源为0
                            //temp_logicalchannel_bj->lcbj-=(statuslength+retxlength+txlength);
                            break;
                        }
                        else if(res_length>=statuslength)//先分状态PDU
                        {
                            temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
				while(temp_rlc_report==0)//申请内存失败，再次申请直到成功
				{
					temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
				}
                            INIT_LIST_HEAD(&(temp_rlc_report->list));
                            temp_rlc_report->lcid=temp_mac_buffer->RlcRequestparams->lcid;
                            temp_rlc_report->rnti=temp_mac_buffer->RlcRequestparams->rnti;
                            temp_rlc_report->statusPduHeader=temp_mac_buffer->RlcRequestparams->statusPduHeader;
                            temp_rlc_report->statusPduSize=temp_mac_buffer->RlcRequestparams->statusPduSize;
                            res_length=res_length-statuslength;
                            //temp_logicalchannel_bj->lcbj-=statuslength;
                            temp_mac_buffer->RlcRequestparams->statusPduHeader=0;
                            temp_mac_buffer->RlcRequestparams->statusPduSize=0;
                            if(res_length >= retxlength)
                            {
                                temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
                                temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
                                res_length=res_length-retxlength;
                                //temp_logicalchannel_bj->lcbj-=retxlength;
                                temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
                                temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
                                if(res_length>=(temp_mac_buffer->RlcRequestparams->txQueueHeader))//大于头部
                                {
                                    temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
                                    res_length=res_length-(temp_mac_buffer->RlcRequestparams->txQueueHeader);
                                    temp_rlc_report->txQueueSize=res_length;
                                   // temp_logicalchannel_bj->lcbj-=(temp_mac_buffer->RlcRequestparams->txQueueHeader+temp_rlc_report->txQueueSize);
                                    temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
                                    temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
                                    list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                    res_length=0;
                                    break;
                                }
                                else if(res_length < (temp_mac_buffer->RlcRequestparams->txQueueHeader))//小于头部
                                {
                                    if(res_length>= (temp_mac_buffer->RlcRequestparams->txQueueSize))//小于头，大于数据
                                    {
                                        temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
                                        temp_rlc_report->txQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->txQueueSize);

                                       // temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->txQueueHeader-=temp_rlc_report->txQueueHeader;
                                        temp_mac_buffer->RlcRequestparams->txQueueSize=0;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                    else//小于头，小于数据
                                    {
                                        temp_rlc_report->txQueueSize=res_length;
                                        temp_rlc_report->txQueueHeader=0;
                                        //temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }

                                }
                            }
                            else if(res_length < retxlength)
                            {
                                if(res_length>=(temp_mac_buffer->RlcRequestparams->retxQueueHeader))//大于头部
                                {
                                   // temp_logicalchannel_bj->lcbj-=res_length;
                                    temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
                                    res_length=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
                                    temp_rlc_report->retxQueueSize=res_length;
                                    temp_rlc_report->txQueueHeader=0;
                                    temp_rlc_report->txQueueSize=0;
                                    temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
                                    temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
                                    list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                    res_length=0;
                                    break;
                                }
                                else if(res_length < (temp_mac_buffer->RlcRequestparams-> retxQueueHeader))//小于重传头部
                                {
                                    if(res_length>= (temp_mac_buffer->RlcRequestparams->retxQueueSize))//小于头，大于数据
                                    {
                                        temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
                                        temp_rlc_report->retxQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueSize);

                                        //temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->retxQueueHeader-=temp_rlc_report->retxQueueHeader;
                                        temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_rlc_report->txQueueSize=0;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                    else//小于头，小于数据
                                    {
                                        temp_rlc_report->retxQueueSize=res_length;
                                        temp_rlc_report->retxQueueHeader=0;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_rlc_report->txQueueSize=0;
                                        //temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                }
                            }
                        }
                        else if(res_length <statuslength)//小于状态PDU
                        {
                            temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
				while(temp_rlc_report==0)//申请内存失败，再次申请直到成功
				{
					temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
				}
                            INIT_LIST_HEAD(&(temp_rlc_report->list));
                            temp_rlc_report->lcid=temp_mac_buffer->RlcRequestparams->lcid;
                            temp_rlc_report->rnti=temp_mac_buffer->RlcRequestparams->rnti;
                            temp_rlc_report->statusPduHeader=0;
                            temp_rlc_report->statusPduSize=0;
                            if(res_length >= retxlength)
                            {
                                temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
                                temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
                                res_length=res_length-retxlength;
                                //temp_logicalchannel_bj->lcbj-=retxlength;
                                temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
                                temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
                                if(res_length>=txlength)
                                {
                                    temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
                                    temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
                                   // temp_logicalchannel_bj->lcbj-=txlength;
                                    temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
                                    temp_mac_buffer->RlcRequestparams->txQueueSize=0;
                                    list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                    res_length-=txlength;
                                    break;
                                }
                                else
                                {
                                    if(res_length>=(temp_mac_buffer->RlcRequestparams->txQueueHeader))//大于头部
                                    {
                                        temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
                                        res_length=res_length-(temp_mac_buffer->RlcRequestparams->txQueueHeader);
                                        temp_rlc_report->txQueueSize=res_length;
                                        //temp_logicalchannel_bj->lcbj-=(temp_mac_buffer->RlcRequestparams->txQueueHeader+temp_rlc_report->txQueueSize);
                                        temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
                                        temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        res_length=0;
                                        break;
                                    }
                                    else if(res_length < (temp_mac_buffer->RlcRequestparams->txQueueHeader))//小于头部
                                    {
                                        if(res_length>= (temp_mac_buffer->RlcRequestparams->txQueueSize))//小于头，大于数据
                                        {
                                            temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
                                            temp_rlc_report->txQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->txQueueSize);

                                            //temp_logicalchannel_bj->lcbj-=res_length;
                                            temp_mac_buffer->RlcRequestparams->txQueueHeader-=temp_rlc_report->txQueueHeader;
                                            temp_mac_buffer->RlcRequestparams->txQueueSize=0;
                                            res_length=0;
                                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                            break;
                                        }
                                        else//小于头，小于数据
                                        {
                                            temp_rlc_report->txQueueSize=res_length;
                                            temp_rlc_report->txQueueHeader=0;
                                            //temp_logicalchannel_bj->lcbj-=res_length;
                                            temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
                                            res_length=0;
                                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                            break;
                                        }
                                    }
                                }
                            }
                            else if(res_length < retxlength)
                            {
                                if(res_length>=(temp_mac_buffer->RlcRequestparams->retxQueueHeader))//大于头部
                                {
                                    //temp_logicalchannel_bj->lcbj-=res_length;
                                    temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
                                    res_length=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
                                    temp_rlc_report->retxQueueSize=res_length;
                                    temp_rlc_report->txQueueHeader=0;
                                    temp_rlc_report->txQueueSize=0;
                                    temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
                                    temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
                                    list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                    res_length=0;
                                    break;
                                }
                                else if(res_length < (temp_mac_buffer->RlcRequestparams-> retxQueueHeader))//小于重传头部
                                {
                                    if(res_length>= (temp_mac_buffer->RlcRequestparams->retxQueueSize))//小于头，大于数据
                                    {
                                        temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
                                        temp_rlc_report->retxQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueSize);

                                        //temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->retxQueueHeader-=temp_rlc_report->retxQueueHeader;
                                        temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_rlc_report->txQueueSize=0;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                    else//小于头，小于数据
                                    {
                                        temp_rlc_report->retxQueueSize=res_length;
                                        temp_rlc_report->retxQueueHeader=0;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_rlc_report->txQueueSize=0;
                                        //temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
               else if((temp_logicalchannel_bj->lcbj)>=0)
                {
                    statuslength=(temp_mac_buffer->RlcRequestparams->statusPduSize)+(temp_mac_buffer->RlcRequestparams->statusPduHeader);
                    retxlength=(temp_mac_buffer->RlcRequestparams->retxQueueSize)+(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
                    txlength=(temp_mac_buffer->RlcRequestparams->txQueueSize)+(temp_mac_buffer->RlcRequestparams->txQueueHeader);
                    if(statuslength>0 || retxlength>0 ||txlength>0)
                    {
                        if(res_length>=(statuslength+retxlength+txlength))//TBSize大于总需求
                        {
                            temp_rlc_report=Mac_RlcRequest_copy(temp_mac_buffer->RlcRequestparams);
                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                            res_length=res_length-(statuslength+retxlength+txlength);
                            temp_logicalchannel_bj->lcbj-=(statuslength+retxlength+txlength);
                            break;
                        }
                        else if(res_length>=statuslength)//先分状态PDU
                        {
                            temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
				while(temp_rlc_report==0)//申请内存失败，再次申请直到成功
				{
					temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
				}
                            INIT_LIST_HEAD(&(temp_rlc_report->list));
                            temp_rlc_report->lcid=temp_mac_buffer->RlcRequestparams->lcid;
                            temp_rlc_report->rnti=temp_mac_buffer->RlcRequestparams->rnti;
                            temp_rlc_report->statusPduHeader=temp_mac_buffer->RlcRequestparams->statusPduHeader;
                            temp_rlc_report->statusPduSize=temp_mac_buffer->RlcRequestparams->statusPduSize;
                            res_length=res_length-statuslength;
                            temp_logicalchannel_bj->lcbj-=statuslength;
                            temp_mac_buffer->RlcRequestparams->statusPduHeader=0;
                            temp_mac_buffer->RlcRequestparams->statusPduSize=0;
                            if(res_length >= retxlength)
                            {
                                temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
                                temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
                                res_length=res_length-retxlength;
                                temp_logicalchannel_bj->lcbj-=retxlength;
                                temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
                                temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
                                if(res_length>=(temp_mac_buffer->RlcRequestparams->txQueueHeader))//大于头部
                                {
                                    temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
                                    res_length=res_length-(temp_mac_buffer->RlcRequestparams->txQueueHeader);
                                    temp_rlc_report->txQueueSize=res_length;
                                    temp_logicalchannel_bj->lcbj-=(temp_mac_buffer->RlcRequestparams->txQueueHeader+temp_rlc_report->txQueueSize);
                                    temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
                                    temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
                                    list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                    res_length=0;
                                    break;
                                }
                                else if(res_length < (temp_mac_buffer->RlcRequestparams->txQueueHeader))//小于头部
                                {
                                    if(res_length>= (temp_mac_buffer->RlcRequestparams->txQueueSize))//小于头，大于数据
                                    {
                                        temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
                                        temp_rlc_report->txQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->txQueueSize);

                                        temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->txQueueHeader-=temp_rlc_report->txQueueHeader;
                                        temp_mac_buffer->RlcRequestparams->txQueueSize=0;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                    else//小于头，小于数据
                                    {
                                        temp_rlc_report->txQueueSize=res_length;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }

                                }
                            }
                            else if(res_length < retxlength)
                            {
                                if(res_length>=(temp_mac_buffer->RlcRequestparams->retxQueueHeader))//大于头部
                                {
                                    temp_logicalchannel_bj->lcbj-=res_length;
                                    temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
                                    res_length=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
                                    temp_rlc_report->retxQueueSize=res_length;
                                    temp_rlc_report->txQueueHeader=0;
                                    temp_rlc_report->txQueueSize=0;
                                    temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
                                    temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
                                    list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                    res_length=0;
                                    break;
                                }
                                else if(res_length < (temp_mac_buffer->RlcRequestparams-> retxQueueHeader))//小于重传头部
                                {
                                    if(res_length>= (temp_mac_buffer->RlcRequestparams->retxQueueSize))//小于头，大于数据
                                    {
                                        temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
                                        temp_rlc_report->retxQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueSize);

                                        temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->retxQueueHeader-=temp_rlc_report->retxQueueHeader;
                                        temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_rlc_report->txQueueSize=0;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                    else//小于头，小于数据
                                    {
                                        temp_rlc_report->retxQueueSize=res_length;
                                        temp_rlc_report->retxQueueHeader=0;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_rlc_report->txQueueSize=0;
                                        temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                }
                            }
                        }
                        else if(res_length <statuslength)//小于状态PDU
                        {
                            temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
				while(temp_rlc_report==0)//申请内存失败，再次申请直到成功
				{
					temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
				}
                            INIT_LIST_HEAD(&(temp_rlc_report->list));
                            temp_rlc_report->lcid=temp_mac_buffer->RlcRequestparams->lcid;
                            temp_rlc_report->rnti=temp_mac_buffer->RlcRequestparams->rnti;
                            temp_rlc_report->statusPduHeader=0;
                            temp_rlc_report->statusPduSize=0;
                            if(res_length >= retxlength)
                            {
                                temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
                                temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
                                res_length=res_length-retxlength;
                                temp_logicalchannel_bj->lcbj-=retxlength;
                                temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
                                temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
                                if(res_length>=txlength)
                                {
                                    temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
                                    temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
                                    temp_logicalchannel_bj->lcbj-=txlength;
                                    temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
                                    temp_mac_buffer->RlcRequestparams->txQueueSize=0;
                                    list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                    res_length-=txlength;
                                    break;
                                }
                                else
                                {
                                    if(res_length>=(temp_mac_buffer->RlcRequestparams->txQueueHeader))//大于头部
                                    {
                                        temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
                                        res_length=res_length-(temp_mac_buffer->RlcRequestparams->txQueueHeader);
                                        temp_rlc_report->txQueueSize=res_length;
                                        temp_logicalchannel_bj->lcbj-=(temp_mac_buffer->RlcRequestparams->txQueueHeader+temp_rlc_report->txQueueSize);
                                        temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
                                        temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        res_length=0;
                                        break;
                                    }
                                    else if(res_length < (temp_mac_buffer->RlcRequestparams->txQueueHeader))//小于头部
                                    {
                                        if(res_length>= (temp_mac_buffer->RlcRequestparams->txQueueSize))//小于头，大于数据
                                        {
                                            temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
                                            temp_rlc_report->txQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->txQueueSize);

                                            temp_logicalchannel_bj->lcbj-=res_length;
                                            temp_mac_buffer->RlcRequestparams->txQueueHeader-=temp_rlc_report->txQueueHeader;
                                            temp_mac_buffer->RlcRequestparams->txQueueSize=0;
                                            res_length=0;
                                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                            break;
                                        }
                                        else//小于头，小于数据
                                        {
                                            temp_rlc_report->txQueueSize=res_length;
                                            temp_rlc_report->txQueueHeader=0;
                                            temp_logicalchannel_bj->lcbj-=res_length;
                                            temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
                                            res_length=0;
                                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                            break;
                                        }
                                    }
                                }
                            }
                            else if(res_length < retxlength)
                            {
                                if(res_length>=(temp_mac_buffer->RlcRequestparams->retxQueueHeader))//大于头部
                                {
                                    temp_logicalchannel_bj->lcbj-=res_length;
                                    temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
                                    res_length=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
                                    temp_rlc_report->retxQueueSize=res_length;
                                    temp_rlc_report->txQueueHeader=0;
                                    temp_rlc_report->txQueueSize=0;
                                    temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
                                    temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
                                    list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                    res_length=0;
                                    break;
                                }
                                else if(res_length < (temp_mac_buffer->RlcRequestparams-> retxQueueHeader))//小于重传头部
                                {
                                    if(res_length>= (temp_mac_buffer->RlcRequestparams->retxQueueSize))//小于头，大于数据
                                    {
                                        temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
                                        temp_rlc_report->retxQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueSize);

                                        temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->retxQueueHeader-=temp_rlc_report->retxQueueHeader;
                                        temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_rlc_report->txQueueSize=0;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                    else//小于头，小于数据
                                    {
                                        temp_rlc_report->retxQueueSize=res_length;
                                        temp_rlc_report->retxQueueHeader=0;
                                        temp_rlc_report->txQueueHeader=0;
                                        temp_rlc_report->txQueueSize=0;
                                        temp_logicalchannel_bj->lcbj-=res_length;
                                        temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
                                        res_length=0;
                                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
                                        break;
                                    }
                                }
                            }
                        }
                    }
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
   if(res_length>0)
   {
    	temp_mac_buffer=NULL;
	list_for_each_entry(temp_mac_buffer, &(lcbj_lessthan_zero->list),list)//遍历
	{
	        statuslength=(temp_mac_buffer->RlcRequestparams->statusPduSize)+(temp_mac_buffer->RlcRequestparams->statusPduHeader);
	        retxlength=(temp_mac_buffer->RlcRequestparams->retxQueueSize)+(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
	        txlength=(temp_mac_buffer->RlcRequestparams->txQueueSize)+(temp_mac_buffer->RlcRequestparams->txQueueHeader);
	        if(statuslength>0 || retxlength>0 ||txlength>0)
	        {
	            if(res_length>=(statuslength+retxlength+txlength))//TBSize大于总需求
	            {
	                temp_rlc_report=Mac_RlcRequest_copy(temp_mac_buffer->RlcRequestparams);
	                list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                res_length=res_length-(statuslength+retxlength+txlength);
	                temp_logicalchannel_bj->lcbj-=(statuslength+retxlength+txlength);
	                break;
	            }
	            else if(res_length>=statuslength)//先分状态PDU
	            {
	                temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
			while(temp_rlc_report==0)//申请内存失败，再次申请直到成功
			{
				temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
			}
	                INIT_LIST_HEAD(&(temp_rlc_report->list));
	                temp_rlc_report->lcid=temp_mac_buffer->RlcRequestparams->lcid;
	                temp_rlc_report->rnti=temp_mac_buffer->RlcRequestparams->rnti;
	                temp_rlc_report->statusPduHeader=temp_mac_buffer->RlcRequestparams->statusPduHeader;
	                temp_rlc_report->statusPduSize=temp_mac_buffer->RlcRequestparams->statusPduSize;
	                res_length=res_length-statuslength;
	                temp_logicalchannel_bj->lcbj-=statuslength;
	                temp_mac_buffer->RlcRequestparams->statusPduHeader=0;
	                temp_mac_buffer->RlcRequestparams->statusPduSize=0;
	                if(res_length >= retxlength)
	                {
	                    temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
	                    temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
	                    res_length=res_length-retxlength;
	                    temp_logicalchannel_bj->lcbj-=retxlength;
	                    temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
	                    temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
	                    if(res_length>=(temp_mac_buffer->RlcRequestparams->txQueueHeader))//大于头部
	                    {
	                        temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
	                        res_length=res_length-(temp_mac_buffer->RlcRequestparams->txQueueHeader);
	                        temp_rlc_report->txQueueSize=res_length;
	                        temp_logicalchannel_bj->lcbj-=(temp_mac_buffer->RlcRequestparams->txQueueHeader+temp_rlc_report->txQueueSize);
	                        temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
	                        temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
	                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                        res_length=0;
	                        break;
	                    }
	                    else if(res_length < (temp_mac_buffer->RlcRequestparams->txQueueHeader))//小于头部
	                    {
	                        if(res_length>= (temp_mac_buffer->RlcRequestparams->txQueueSize))//小于头，大于数据
	                        {
	                            temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
	                            temp_rlc_report->txQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->txQueueSize);

	                            temp_logicalchannel_bj->lcbj-=res_length;
	                            temp_mac_buffer->RlcRequestparams->txQueueHeader-=temp_rlc_report->txQueueHeader;
	                            temp_mac_buffer->RlcRequestparams->txQueueSize=0;
	                            res_length=0;
	                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                            break;
	                        }
	                        else//小于头，小于数据
	                        {
	                            temp_rlc_report->txQueueSize=res_length;
	                            temp_rlc_report->txQueueHeader=0;
	                            temp_logicalchannel_bj->lcbj-=res_length;
	                            temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
	                            res_length=0;
	                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                            break;
	                        }

	                    }
	                }
	                else if(res_length < retxlength)
	                {
	                    if(res_length>=(temp_mac_buffer->RlcRequestparams->retxQueueHeader))//大于头部
	                    {
	                        temp_logicalchannel_bj->lcbj-=res_length;
	                        temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
	                        res_length=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
	                        temp_rlc_report->retxQueueSize=res_length;
	                        temp_rlc_report->txQueueHeader=0;
	                        temp_rlc_report->txQueueSize=0;
	                        temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
	                        temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
	                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                        res_length=0;
	                        break;
	                    }
	                    else if(res_length < (temp_mac_buffer->RlcRequestparams-> retxQueueHeader))//小于重传头部
	                    {
	                        if(res_length>= (temp_mac_buffer->RlcRequestparams->retxQueueSize))//小于头，大于数据
	                        {
	                            temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
	                            temp_rlc_report->retxQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueSize);

	                            temp_logicalchannel_bj->lcbj-=res_length;
	                            temp_mac_buffer->RlcRequestparams->retxQueueHeader-=temp_rlc_report->retxQueueHeader;
	                            temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
	                            temp_rlc_report->txQueueHeader=0;
	                            temp_rlc_report->txQueueSize=0;
	                            res_length=0;
	                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                            break;
	                        }
	                        else//小于头，小于数据
	                        {
	                            temp_rlc_report->retxQueueSize=res_length;
	                            temp_rlc_report->retxQueueHeader=0;
	                            temp_rlc_report->txQueueHeader=0;
	                            temp_rlc_report->txQueueSize=0;
	                            temp_logicalchannel_bj->lcbj-=res_length;
	                            temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
	                            res_length=0;
	                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                            break;
	                        }
	                    }
	                }
	            }
	            else if(res_length <statuslength)//小于状态PDU
	            {
	                temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
		while(temp_rlc_report==0)//申请内存失败，再次申请直到成功
		{
			temp_rlc_report=(RlcBufferRequest *)fsm_mem_alloc(sizeof(RlcBufferRequest));
		}
	                INIT_LIST_HEAD(&(temp_rlc_report->list));
	                temp_rlc_report->lcid=temp_mac_buffer->RlcRequestparams->lcid;
	                temp_rlc_report->rnti=temp_mac_buffer->RlcRequestparams->rnti;
	                temp_rlc_report->statusPduHeader=0;
	                temp_rlc_report->statusPduSize=0;
	                if(res_length >= retxlength)
	                {
	                    temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
	                    temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
	                    res_length=res_length-retxlength;
	                    temp_logicalchannel_bj->lcbj-=retxlength;
	                    temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
	                    temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
	                    if(res_length>=txlength)
	                    {
	                        temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
	                        temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
	                        temp_logicalchannel_bj->lcbj-=txlength;
	                        temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
	                        temp_mac_buffer->RlcRequestparams->txQueueSize=0;
	                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                        res_length-=txlength;
	                        break;
	                    }
	                    else
	                    {
	                        if(res_length>=(temp_mac_buffer->RlcRequestparams->txQueueHeader))//大于头部
	                        {
	                            temp_rlc_report->txQueueHeader=temp_mac_buffer->RlcRequestparams->txQueueHeader;
	                            res_length=res_length-(temp_mac_buffer->RlcRequestparams->txQueueHeader);
	                            temp_rlc_report->txQueueSize=res_length;
	                            temp_logicalchannel_bj->lcbj-=(temp_mac_buffer->RlcRequestparams->txQueueHeader+temp_rlc_report->txQueueSize);
	                            temp_mac_buffer->RlcRequestparams->txQueueHeader=0;
	                            temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
	                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                            res_length=0;
	                            break;
	                        }
	                        else if(res_length < (temp_mac_buffer->RlcRequestparams->txQueueHeader))//小于头部
	                        {
	                            if(res_length>= (temp_mac_buffer->RlcRequestparams->txQueueSize))//小于头，大于数据
	                            {
	                                temp_rlc_report->txQueueSize=temp_mac_buffer->RlcRequestparams->txQueueSize;
	                                temp_rlc_report->txQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->txQueueSize);

	                                temp_logicalchannel_bj->lcbj-=res_length;
	                                temp_mac_buffer->RlcRequestparams->txQueueHeader-=temp_rlc_report->txQueueHeader;
	                                temp_mac_buffer->RlcRequestparams->txQueueSize=0;
	                                res_length=0;
	                                list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                                break;
	                            }
	                            else//小于头，小于数据
	                            {
	                                temp_rlc_report->txQueueSize=res_length;
	                                temp_rlc_report->txQueueHeader=0;
	                                temp_logicalchannel_bj->lcbj-=res_length;
	                                temp_mac_buffer->RlcRequestparams->txQueueSize-=res_length;
	                                res_length=0;
	                                list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                                break;
	                            }
	                        }
	                    }
	                }
	                else if(res_length < retxlength)
	                {
	                    if(res_length>=(temp_mac_buffer->RlcRequestparams->retxQueueHeader))//大于头部
	                    {
	                        temp_logicalchannel_bj->lcbj-=res_length;
	                        temp_rlc_report->retxQueueHeader=temp_mac_buffer->RlcRequestparams->retxQueueHeader;
	                        res_length=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueHeader);
	                        temp_rlc_report->retxQueueSize=res_length;
	                        temp_rlc_report->txQueueHeader=0;
	                        temp_rlc_report->txQueueSize=0;
	                        temp_mac_buffer->RlcRequestparams->retxQueueHeader=0;
	                        temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
	                        list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                        res_length=0;
	                        break;
	                    }
	                    else if(res_length < (temp_mac_buffer->RlcRequestparams-> retxQueueHeader))//小于重传头部
	                    {
	                        if(res_length>= (temp_mac_buffer->RlcRequestparams->retxQueueSize))//小于头，大于数据
	                        {
	                            temp_rlc_report->retxQueueSize=temp_mac_buffer->RlcRequestparams->retxQueueSize;
	                            temp_rlc_report->retxQueueHeader=res_length-(temp_mac_buffer->RlcRequestparams->retxQueueSize);

	                            temp_logicalchannel_bj->lcbj-=res_length;
	                            temp_mac_buffer->RlcRequestparams->retxQueueHeader-=temp_rlc_report->retxQueueHeader;
	                            temp_mac_buffer->RlcRequestparams->retxQueueSize=0;
	                            temp_rlc_report->txQueueHeader=0;
	                            temp_rlc_report->txQueueSize=0;
	                            res_length=0;
	                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                            break;
	                        }
	                        else//小于头，小于数据
	                        {
	                            temp_rlc_report->retxQueueSize=res_length;
	                            temp_rlc_report->retxQueueHeader=0;
	                            temp_rlc_report->txQueueHeader=0;
	                            temp_rlc_report->txQueueSize=0;
	                            temp_logicalchannel_bj->lcbj-=res_length;
	                            temp_mac_buffer->RlcRequestparams->retxQueueSize-=res_length;
	                            res_length=0;
	                            list_add_tail(&(temp_rlc_report->list), &(mac_rlc_report->list));
	                            break;
	                        }
	                    }
	                }
	            }
	        }
	    }	
	
    }	
    report_array=RlcRequest_listtoarray(mac_rlc_report,buffer_len,num);//report_array是一个用户的report首地址，num是该用户的report 数组的个数
    //num=report_num;
    pos=NULL,p=NULL;
    list_for_each_safe(pos,p,&(priority_mac_buffer->list))//遍历
    {
        temp_free=list_entry(pos,MacBufferStatus,list);
	fsm_mem_free(temp_free->RlcRequestparams);
        list_del_init(&(temp_free->list));
        fsm_mem_free(temp_free);
    }
     pos=NULL,p=NULL;
    list_for_each_safe(pos,p,&(lcbj_lessthan_zero->list))//遍历
    {
        temp_free=list_entry(pos,MacBufferStatus,list);
	fsm_mem_free(temp_free->RlcRequestparams);
        list_del_init(&(temp_free->list));
        fsm_mem_free(temp_free);
    }
    Empty_MACBuffer_Rlc(enb_buffer_rlc);//分配资源后，清空MacBufferStatus
    FRET(report_array);
}
*/

/*****************删除lcid对应的逻辑信道属性,也删除该逻辑信道的BJ***************/
/*******输入为一个用户的rnti，和要删除的lcid*****/
void Delete_LogicalChannel_ConfigInfo(int unsigned short rnti,unsigned char lcid_delete)
{
    int i;
    LogicalChannelConfigInfo *temp_free=NULL;
    LogicalChannelBj *temp=NULL;
    struct list_head *pos=NULL,*p=NULL;
    FIN(Delete_LogicalChannel_ConfigInfo(int unsigned short rnti,unsigned char lcid_delete));
    SV_PTR_GET(rlc_mac_sv);
    for(i=0;i<USERNUMMAX;i++)
    {
    	pos=NULL,p=NULL;
    	  list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Config[i]))//查找RNTI对应的用户
    	  {
    	  	temp_free=list_entry(pos,LogicalChannelConfigInfo,list);
    	  	if(rnti==temp_free->rnti)//找到该用户
    	  	{
    	  		pos=NULL,p=NULL;
    	  		 list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Config[i]))
		    	 {
		    	  	temp_free=list_entry(pos,LogicalChannelConfigInfo,list);
				if(temp_free->lcid==lcid_delete)//找到要删除的逻辑信道
			        {
			            list_del_init(&(temp_free->list));//delete a node of LogicalChannel_Config
			            fsm_mem_free(temp_free);
			        }
    	  		 }
			pos=NULL,p=NULL;
		    	list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Bj[i]))//BJ与Enb_LogicalChannel_Config是一一对应的
		    	{
			        temp=list_entry(pos,LogicalChannelBj,list);
			        if(temp->lcid==lcid_delete)//找到对应的逻辑信道
			        {
			            list_del_init(&(temp->list));//delete a node of LogicalChannel_Bj
			            fsm_mem_free(temp);
			        }
		    	}
    	  	}
		else
			break;//RNTI不对应，重新查找用户
    	  }
    }
    FOUT;
}
/********清空该TTL所有用户的RLC请求链表的长度，为下次请求清空资源**********/
/********需要在黄琼的总体(所有用户)调度的函数中调用一次即可**********/
void Empty_Rlcrequestnum(void)
{
	FIN(Empty_Rlcrequestnum());
    	SV_PTR_GET(rlc_mac_sv);
	fsm_mem_set(SV(Rlc_Request_Num),0,sizeof(int)*USERNUMMAX);
	FOUT;
}

/************清空MAC层的RLC请求Buffer********************/
/***输入为一个用户的buffer头指针***/
void Empty_MACBuffer_Rlc(struct list_head *enb_buffer_rlc)
{
    FIN(Empty_MACBuffer_Rlc(struct list_head *enb_buffer_rlc));
    SV_PTR_GET(rlc_mac_sv);
    struct list_head *pos=NULL,*p=NULL;
    MacBufferStatus *temp_free=NULL;
    list_for_each_safe(pos,p,enb_buffer_rlc)//遍历
    {
        temp_free=list_entry(pos,MacBufferStatus,list);
	fsm_mem_free(temp_free->RlcRequestparams);
        list_del_init(&(temp_free->list));
        fsm_mem_free(temp_free);
    }
    FOUT;
}
void Empty_LogicalChannelInfo(struct list_head *m_lc_info)
{
    struct list_head *pos=NULL,*p=NULL;
    LogicalChannelConfigInfo *temp_free=NULL;
    FIN(Empty_LogicalChannelInfo(struct list_head *m_lc_info));
    SV_PTR_GET(rlc_mac_sv);    
    list_for_each_safe(pos,p,m_lc_info)//遍历
    {
        temp_free=list_entry(pos,LogicalChannelConfigInfo,list);
        list_del_init(&(temp_free->list));
        fsm_mem_free(temp_free);
    }
    FOUT;
}

void Empty_LogicalChannelBj(struct list_head *m_lcbj_info)
{
    struct list_head *pos=NULL,*p=NULL;
    LogicalChannelBj *temp_free=NULL;
    FIN(Empty_LogicalChannelBj(struct list_head *m_lcbj_info));
    SV_PTR_GET(rlc_mac_sv);    
    list_for_each_safe(pos,p,m_lcbj_info)//遍历
    {
        temp_free=list_entry(pos,LogicalChannelBj,list);
        list_del_init(&(temp_free->list));
        fsm_mem_free(temp_free);
    }
    FOUT;
}

/***********释放UE MAC SCHEDULER资源************/
void Free_Enbmac_Scheduler_Resource(void)
{
    struct list_head *pos,*p;
    MacBufferStatus *temp_rlc_free=NULL;
    LogicalChannelBj *temp_bj_free=NULL;
    LogicalChannelConfigInfo *temp_lcginfo_free=NULL;//逻辑信道属 性
    int i;
    FIN(Free_Enbmac_Scheduler_Resource(void));
    SV_PTR_GET(rlc_mac_sv);  
    for(i=0;i<USERNUMMAX;i++)
    {
	    list_for_each_safe(pos,p,SV(Enb_MacBuffer_RLC[i]))//遍历
	    {
	        temp_rlc_free=list_entry(pos,MacBufferStatus,list);
		 fsm_mem_free(temp_rlc_free->RlcRequestparams);
	        list_del_init(&(temp_rlc_free->list));
	        fsm_mem_free(temp_rlc_free);
	    }
	   
	    pos=NULL;
	    p=NULL;
	    list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Bj[i]))//遍历
	    {
	        temp_bj_free=list_entry(pos,LogicalChannelBj,list);
	        list_del_init(&(temp_bj_free->list));
	        fsm_mem_free(temp_bj_free);
	    }
		
	    pos=NULL;
	    p=NULL;
	    list_for_each_safe(pos,p,SV(Enb_LogicalChannel_Config[i]))//遍历
	    {
	        temp_lcginfo_free=list_entry(pos,LogicalChannelConfigInfo,list);
	        list_del_init(&(temp_lcginfo_free->list));
	        fsm_mem_free(temp_lcginfo_free);
	    }
		
	fsm_mem_free(SV(Enb_MacBuffer_RLC[i]));
	fsm_mem_free(SV(Enb_LogicalChannel_Bj[i]));
	fsm_mem_free(SV(Enb_LogicalChannel_Config[i]));
    }
    FOUT;
}

  












