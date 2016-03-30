#include "../fsm/fsmdec.h"

#include <stdbool.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <stddef.h>

#ifndef _MACSCHEDULER_H
#define _MACSCHEDULER_H
#define LCG 4
typedef struct Mac_Buffer_Status_BSR_Info  //hl 更改，UE,调度器产生的BSR 待复用装配
{
	unsigned short m_rnti;
	unsigned int m_lcgnum;//标志有多少个LCG有数据要发送
	unsigned int m_TotalLcgData; //所有逻辑信道要请求的总大小
	unsigned int m_bufferStatus[LCG];
}__attribute__((packed))MacBufferStatus_BSR_Info;

struct Ul_SpecificParameters{
	int priority;        //INTEGER (1..16)
	enum PrioritisedBitRate{
		kBps0=0, kBps8=8, kBps16=16, kBps32=32, kBps64=64, kBps128=128,
		kBps256=256, infinity=-1//..........spare1~8未表示
	}prioritisedBitRate;
	enum BucketSizeDuration{
		bucketSizeDuration_ms50=50, bucketSizeDuration_ms100=100, bucketSizeDuration_ms150=150, bucketSizeDuration_ms300=300, 
		bucketSizeDuration_ms500=500, bucketSizeDuration_ms1000=1000//..........spare1~2未表示
	}bucketSizeDuration;
	int logicalChannelGroup;	//INTEGER (0..3) 
};

typedef struct LogicalChannelConfig//lhl 更改，RRC层关于逻辑信道的结构
{
	bool haveUl_SpecificParameters;
	struct Ul_SpecificParameters ul_SpecificParameters;
}LogicalChannelConfig;


typedef struct MAC_LogicalChannelConfig_IoctrlMsg{
	u16 rnti;
	int logicalChannelIdentity;
	struct LogicalChannelConfig logicalChannelConfig;
}MAC_LogicalChannelConfig_IoctrlMsg;


/*****************lhl注释完成，以下是lhl修改********************/
#define TTI 1	//表示1ms,在函数FlushBj()中使用

typedef struct RLC_Buffer_Request//lhl 更改
{
	//u16 frame;   //eNB侧添加成员
	//u16 subframe;
	unsigned short rnti;
	unsigned char lcid;
	unsigned int txQueueSize;
	unsigned short txQueueHeader;
	unsigned int retxQueueSize;
	unsigned short retxQueueHeader;
	unsigned short statusPduSize;
unsigned short statusPduHeader;
}__attribute__((packed))RLC_Request,RLC_Report;


typedef struct RLCBufferRequestParameters //lhl 更改，RLC层buffer状态报告链表
{
    unsigned short rnti;
	unsigned char lcid;
	unsigned int txQueueSize;
	unsigned short txQueueHeader;
	unsigned int retxQueueSize;
	unsigned short retxQueueHeader;
	unsigned short statusPduSize;
	unsigned short statusPduHeader;
    struct list_head list;
}__attribute__((packed))RlcBufferRequest;


typedef struct Mac_Buffer_Status  //MAC层保存更新的RLC状态报告
{
	int lcid;
	RLC_Request *RlcRequestparams;
	struct list_head list;
}__attribute__((packed))MacBufferStatus;


typedef  struct Logical_Channel_Config_Info  //lhl 更改，逻辑信道属性结构
{
	int lcid;
	unsigned short rnti;
	int priority;
    int prioritizedBitRateKbps;
	int bucketSizeDurationMs;
	int logicalChannelGroup;
	struct list_head list;
}__attribute__((packed))LogicalChannelConfigInfo;



typedef  struct  Logical_Channel_Bj  //lhl 更改，逻辑信道的BJ结构
{
	int lcid;
	unsigned short rnti;
    int lcbj;	//逻辑信道的BJ值
	struct list_head list;
}__attribute__((packed))LogicalChannelBj;



#define MAX_BJ 256000/*20140512modified by lhl,256*1000是最大的pbr*bsd值*/




/********调用：初始化时调用****/
void Init_Enbmac_Scheduler(void);//UE 调度器初始化
void * DoRefreshRLCBuffserRequest (RlcBufferRequest* params, struct list_head *enb_buffer_rlc);//更新RLC请求
RLC_Request* Rlc_MacRequest_copy(RlcBufferRequest *temp);//复制节点rlcBufferRequest
LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_copy(MAC_LogicalChannelConfig_IoctrlMsg *temp);//复制逻辑信道属性节点
//LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_copy(LogicalChannelConfigInfo *temp);//复制逻辑信道属性节点
RlcBufferRequest* Mac_RlcRequest_copy(RLC_Request *temp);//复制RlcBufferRequest节点
RLC_Request* Mac_MacRequest_copy(RLC_Request *temp);//复制RlcBufferRequest节点

/******************调用：每个TTL 中断IOCTL到来时调用****************/
void FlushBj(struct list_head *enb_logicalChannel_bj, struct list_head *enb_buffer_rlc);//每个TTI更新逻辑信道的BJ

LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_Rank_Priority(struct list_head * enb_logicalChannel_config);//根据逻辑信道优先级对逻辑信道排序
MacBufferStatus* PrioritySort( struct list_head * enb_logicalChannel_config,struct list_head *enb_buffer_rlc);//根据逻辑信道优先级顺序，对MacBufferStatus链表按优先级排序



RLC_Request * DoResourceAllocation(int tb_size,struct list_head *enb_buffer_rlc, struct list_head * enb_logicalChannel_config, struct list_head *enb_logicalChannel_bj,int index,int *rlc_report_num);//资源分配
/********调用：在RRC IOCTL到来时调用，或者多逻辑信道默认配置时调用****/
void Init_LogicalChannel_ConfigInfo(MAC_LogicalChannelConfig_IoctrlMsg *lc_info);//更新逻辑信道属性结构和BJ结构,RRC逻辑信道配置IOCTL触发时调用

/******输入msg为GetTBsize_Allocation返回的资源大小，输出:*num是是要发送的IOCTL的buffer长度，void *是RLC 报告的头指针**********/
/********调用：在ResourceAllocation_Algorithm中已经封装调用，若无特殊情况不用其他模块显示调用****************/
RLC_Request * ResourceAllocation_Algorithm(int msg,struct list_head *enb_buffer_rlc,struct list_head * enb_logicalChannel_config, struct list_head *enb_logicalChannel_bj,int *num,int index);//UE侧资源分配算法：令牌桶,RlcBufferRequest MACReportToRlc
void Empty_MACBuffer_Rlc(struct list_head *enb_buffer_rlc);//清空MAC层的RLC请求Buffer
void Free_Enbmac_Scheduler_Resource(void);//释放UE MAC SCHEDULER资源
void Delete_LogicalChannel_ConfigInfo(int unsigned short rnti,unsigned char lcid_delete);
/*****RLC请求IOCL到来是调用，输入为RLC请求数组的头指针，以及RLC请求数组的长度，输出为链表形式*************/
RlcBufferRequest * RlcRequest_arraytolist(RLC_Request *rlc_request_array,int num,int uesernum);
void Empty_Rlcrequestnum(void);
/*******输入RlcBufferRequest为RLC report链表，输出：*num是要发送的IOCTL的buffer长度返回RLC 报告的头指针***************/
RLC_Request * RlcRequest_listtoarray( RlcBufferRequest *rlc_request,int reqnum ,int *num);
struct  list_head * Get_LogicalChannelInfo_From_Enbbuffer(struct list_head *enb_buffer_rlc);
struct  list_head * Get_LogicalChannelBj_From_Enbbuffer(struct list_head *enb_buffer_rlc);
unsigned short Getrnti_From_Enbbuffer(struct list_head *enb_buffer_rlc);//获取RNTI，modified by lhl 20140715 
 struct list_head * Get_LogicalChannelInfo_From_Rnti(unsigned short m_rnti);//由RNTI 获 取逻辑信道属性，modified by lhl 20140715 
struct list_head * Get_LogicalChannelBj_From_Rnti(unsigned short m_rnti);//由RNTI 获 取逻辑信BJ,modified by lhl 20140715 
 struct list_head * Get_Enbbuffer_From_Rnti(unsigned short m_rnti);
void Empty_LogicalChannelInfo(struct list_head *m_lc_info);//释放逻辑信道内存，lhl 20140715
void Empty_LogicalChannelBj(struct list_head *m_lcbj_info);//释放逻辑BJ 内存，响应RRC释放请求lhl 20140715

void ResourceAllocation_foreach_logicalchannel(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int statuslength,int retxlength,int txlength,int *res_length);
void ResourceAllocation_logicalchannel_firststatusbuffer(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int statuslength,int retxlength,int txlength,int *res_length);
void ResourceAllocation_logicalchannel_firstretxbuffer(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int retxlength,int txlength,int *res_length);
void leftresource_lessthan_retxbuffer_eachlogicalchannel(RlcBufferRequest *mac_rlc_report,RlcBufferRequest *temp_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int retxlength,int txlength,int *res_length);
void leftresource_morethan_retxbuffer_eachlogicalchannel(RlcBufferRequest *mac_rlc_report,RlcBufferRequest *temp_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,int retxlength,int txlength,int *res_length);
#endif

