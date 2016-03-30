#include "../fsm/fsmdec.h"

#include <stdbool.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <stddef.h>
#include "rrc_config.h"

#ifndef _MACSCHEDULER_H
#define _MACSCHEDULER_H


/*逻辑信道待发数据情况 */
/*
 typedef  struct  RLCBufferRequestParameters  //RLC层buffer状态报告链表
{
   	  u16 rnti;
               u8  lcid;
 	  u32 txQueueSize;
 	  u16 txQueueHeader;
 	  u32 retxQueueSize;
 	  u16 retxQueueHeader;
 	  u16 statusPduSize;
 	  u16 statusPduHeader;
   struct RLCBufferRequestParameters *next;
}__attribute__((packed))RlcBufferRequest;
*/
#define TTI 1	//表示1ms,在函数FlushBj()中使用
#define MAC_BSR_CTL_LENGTH 4 //(byte),加上了MAC头和控制单元
#define LCG 4	//全局变量
#define BSRTABLEINDEX 64 //BSR表格的索引
#define BANDWIDTH 20//单位MHZ,
#define RB_NUMBER 100//RB的数量


#define SCHEDULE_ADVANCE 4//调度提前量，至少4个TTI
#define UL_SUBFRAME2 2//上行子帧号2
#define UL_SUBFRAME7 7//上行子帧号7

/*****************以下是lhl修改20140410********************/
#define C_RNTI_LCID 27
#define TRUNCATED_BSR_LCID 28
#define SHORT_BSR_LCID 29
#define LONG_BSR_LCID 30
#define MAX_BJ 256000/*20140512modified by lhl,256*1000是最大的pbr*bsd值*/

#define OFDM 7
#define SUB_CARRIER 12
/*****************lhl修改完成20140410********************/


typedef struct UL_resource
{
	bool resource_flag; // "ture" means available,"false" means unvailable 表示该资源是否可用,0表示不可用，1表示可用
	u8 m_rbstart;
	u8 m_rbLen;
	u32  m_tbsize;
	bool  ul_delay;
}UL_resource;

typedef struct RLC_Buffer_Request//lhl 更改
{
	u16 rnti;
	u8 lcid;
	u32 txQueueSize;
	u16 txQueueHeader;
	u32 retxQueueSize;
	u16 retxQueueHeader;
	u16 statusPduSize;
	u16 statusPduHeader;
}__attribute__((packed))RLC_Request;



/*typedef struct RLCBufferRequestParameters_array //lhl 更改，RLC层buffer状态报告数组20140526
{
    unsigned short rnti;
	unsigned char lcid;
	unsigned int txQueueSize;
	unsigned short txQueueHeader;
	unsigned int retxQueueSize;
	unsigned short retxQueueHeader;
	unsigned short statusPduSize;
	unsigned short statusPduHeader;
}__attribute__((packed))RLC_Request;*/

typedef struct RLCBufferRequestParameters //lhl 更改，RLC层buffer状态报告链表
{
	u16 rnti;
	u8 lcid;
	u32 txQueueSize;
	u16 txQueueHeader;
	u32 retxQueueSize;
	u16 retxQueueHeader;
	u16 statusPduSize;
	u16 statusPduHeader;
	struct list_head list;
}__attribute__((packed))RlcBufferRequest;


typedef struct Mac_Buffer_Status  //MAC层保存更新的RLC状态报告
{
	u8 lcid;
	RLC_Request *RlcRequestparams;
	struct list_head list;
}__attribute__((packed))MacBufferStatus;


typedef  struct Logical_Channel_Config_Info  //lhl 更改，逻辑信道属性结构
{
	u8 lcid;
	long priority;
	long prioritizedBitRateKbps;
	u32 bucketSizeDurationMs;
	u32 logicalChannelGroup;
	struct list_head list;
}__attribute__((packed))LogicalChannelConfigInfo;

/*struct LogicalChannelConfig//lhl 20140527更改，RRC层关于逻辑信道的结构
{
	bool haveUl_SpecificParameters;
	struct Ul_SpecificParameters ul_SpecificParameters;
}__attribute__((packed));

typedef struct MAC_LogicalChannelConfig_IoctrlMsg//lhl 2014027更改，RRC层关于逻辑信道的结构
{
    int logicalChannelIdentity;
    struct LogicalChannelConfig logicalChannelConfig;
}__attribute__((packed))MAC_LogicalChannelConfig_IoctrlMsg;*/

typedef  struct  Logical_Channel_Bj  //lhl 更改，逻辑信道的BJ结构
{
	u8 lcid;
	u32 lcbj;	//逻辑信道的BJ值
	struct list_head list;
}__attribute__((packed))LogicalChannelBj;

/*typedef struct ULgrant //lhl 更改,RAR ULgrant结构
{
	u8	m_mcs;                      //4bit  调制编码格式
	u8   m_hoppingflag;              //1bit  跳频标志
	u16  rb_assignment;              //10bit 资源分配RIV
	u8   m_tpc;                     //3bits 相对功率
	u8   m_cqiRequest;               //1bit  cqi请求
	u8   m_ulDelay;                  //1bit  UL延迟
}__attribute__((packed))RAR_ULgrant;*/



/*typedef struct Ul_DCI  //lhl 更改，常规ULgrant结构
{
	u8   m_format;                     //1bit 区分format0和1A
	u8   m_hopping;                    //1bit  跳频标志
	u16  RIV;                          //13bit:when band==20MHZ
	u8   m_ndi;                        //1bit 新数据标志
	u8   m_mcs;                        //5bit 调制编码格式和RV信息
	u8   m_tpc;                        //2bit
	u8   m_cqiRequest;                 //1bit cqi请求标识
	u8   Cyclic_shift;                 //3bit  数据解调导频的循环移位
	u8   m_dai;                        //2bit 下行分配数目的标识 用于上下行时间配比：#1—6
	u8   padding;                      //2bit 填充比特 使其长度与format 1A相同
}__attribute__((packed))Regular_ULgrant;*/


typedef struct Ul_DCI
{
    u32	m_format  	:1;                 //1bit 区分format0和1A
    u32	m_hopping 	:1;                 //1bit  跳频标志
    u32	RIV       	:13;                //13bit:when band==20MHZ type2
    u32	m_mcs		:5;                 //5bit 调制编码格式和RV信息
    u32	m_ndi		:1;                 //1bit 新数据标志
    u32	m_tpc		:2;                 //2bit
    u32 Cyclic_shift:3;                 //3bit 数据解调导频的循环移位
    u32 m_dai		:2;                 //2bit 下行分配数目的标识 when上下行时间配比：#1-6
    u32 m_cqiRequest:1;                 //1bit cqi请求标识
    u32 padding		:2;                 //2bit 填充比特 使其长度与format 1A相同(协议要求)
    u32 emptybits	:1;					//由系统带宽和32bit决定。  
}__attribute__((packed))Regular_ULgrant;// structure of UL DCI format0

typedef struct ULgrant_rar//lhl 更改,RAR ULgrant结构20141111
{
    u32	m_hoppingflag	:1;            	//1bit  跳频标志
	u32	rb_assignment	:10;         	//10bit 资源分配RIV
	u32	m_mcs			:4;          	//4bit  调制编码格式0-15
  	u32	m_tpc			:3;           	//3bits 相对功率
	u32	m_ulDelay		:1;           	//1bit  UL延迟
    u32	m_cqiRequest	:1;				//1bit  cqi请求
	u32	emptybits		:12;			
}__attribute__((packed))RAR_ULgrant;//structure  ULgrant

typedef struct UEPHY_TO_MAC_ULSCHEDULE
{
	u16	m_rnti;
	u16 frameNo;
	u16  subframeNo ; 
	Regular_ULgrant  ulgrant;
}__attribute__((packed))UEPHY_TO_MAC_ULgrant;//LHLmodified 20141105



typedef struct Mac_Buffer_Status_BSR_Info  //hl 更改，UE,调度器产生的BSR 待复用装配
{
	u16 m_rnti;
	u32 m_lcgnum;//标志有多少个LCG有数据要发送
	u32 m_TotalLcgData; //所有逻辑信道要请求的总大小
	u32 m_bufferStatus[LCG];
}__attribute__((packed))MacBufferStatus_BSR_Info;



/*****************************以下是lhl修改20140410******************************/
static unsigned int BufferSizeLevelBsrTable[BSRTABLEINDEX] ={
  0, 10, 12, 14, 17, 19, 22, 26, 31, 36, 42, 49, 57, 67, 78, 91,
  107, 125, 146, 171, 200, 234, 274, 321, 376, 440, 515, 603,
  706, 826, 967, 1132, 1326, 1552, 1817, 2127, 2490, 2915, 3413,
  3995, 4677, 5476, 6411, 7505, 8787, 10287, 12043, 14099, 16507,
  19325, 22624, 26487, 31009, 36304, 42502, 49759, 58255,
  68201, 79846, 93749, 109439, 128125, 150000, 150000
};

static unsigned int Ul_McsToItbsize[29] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  19, 20, 21, 22, 23, 24, 25, 26
};

static int TransportBlockSizeTable [110][27] = {

  /* NPRB 001*/ { 16, 24, 32, 40, 56, 72, 88, 104, 120, 136, 144, 176, 208, 224, 256, 280, 328, 336, 376, 408, 440, 488, 520, 552, 584, 616, 712},
  /* NPRB 002*/ { 32, 56, 72, 104, 120, 144, 176, 224, 256, 296, 328, 376, 440, 488, 552, 600, 632, 696, 776, 840, 904, 1000, 1064, 1128, 1192, 1256, 1480},
  /* NPRB 003*/ { 56, 88, 144, 176, 208, 224, 256, 328, 392, 456, 504, 584, 680, 744, 840, 904, 968, 1064, 1160, 1288, 1384, 1480, 1608, 1736, 1800, 1864, 2216},
  /* NPRB 004*/ { 88, 144, 176, 208, 256, 328, 392, 472, 536, 616, 680, 776, 904, 1000, 1128, 1224, 1288, 1416, 1544, 1736, 1864, 1992, 2152, 2280, 2408, 2536, 2984},
  /* NPRB 005*/ { 120, 176, 208, 256, 328, 424, 504, 584, 680, 776, 872, 1000, 1128, 1256, 1416, 1544, 1608, 1800, 1992, 2152, 2344, 2472, 2664, 2856, 2984, 3112, 3752},
  /* NPRB 006*/ { 152, 208, 256, 328, 408, 504, 600, 712, 808, 936, 1032, 1192, 1352, 1544, 1736, 1800, 1928, 2152, 2344, 2600, 2792, 2984, 3240, 3496, 3624, 3752, 4392},
  /* NPRB 007*/ { 176, 224, 296, 392, 488, 600, 712, 840, 968, 1096, 1224, 1384, 1608, 1800, 1992, 2152, 2280, 2536, 2792, 2984, 3240, 3496, 3752, 4008, 4264, 4392, 5160},
  /* NPRB 008*/ { 208, 256, 328, 440, 552, 680, 808, 968, 1096, 1256, 1384, 1608, 1800, 2024, 2280, 2472, 2600, 2856, 3112, 3496, 3752, 4008, 4264, 4584, 4968, 5160, 5992},
  /* NPRB 009*/ { 224, 328, 376, 504, 632, 776, 936, 1096, 1256, 1416, 1544, 1800, 2024, 2280, 2600, 2728, 2984, 3240, 3624, 3880, 4136, 4584, 4776, 5160, 5544, 5736, 6712},
  /* NPRB 010*/ { 256, 344, 424, 568, 696, 872, 1032, 1224, 1384, 1544, 1736, 2024, 2280, 2536, 2856, 3112, 3240, 3624, 4008, 4264, 4584, 4968, 5352, 5736, 5992, 6200, 7480},
  /* NPRB 011*/ { 288, 376, 472, 616, 776, 968, 1128, 1320, 1544, 1736, 1928, 2216, 2472, 2856, 3112, 3368, 3624, 4008, 4392, 4776, 5160, 5544, 5992, 6200, 6712, 6968, 8248},
  /* NPRB 012*/ { 328, 424, 520, 680, 840, 1032, 1224, 1480, 1672, 1864, 2088, 2408, 2728, 3112, 3496, 3624, 3880, 4392, 4776, 5160, 5544, 5992, 6456, 6968, 7224, 7480, 8760},
  /* NPRB 013*/ { 344, 456, 568, 744, 904, 1128, 1352, 1608, 1800, 2024, 2280, 2600, 2984, 3368, 3752, 4008, 4264, 4776, 5160, 5544, 5992, 6456, 6968, 7480, 7992, 8248, 9528},
  /* NPRB 014*/ { 376, 488, 616, 808, 1000, 1224, 1480, 1672, 1928, 2216, 2472, 2792, 3240, 3624, 4008, 4264, 4584, 5160, 5544, 5992, 6456, 6968, 7480, 7992, 8504, 8760, 10296},
  /* NPRB 015*/ { 392, 520, 648, 872, 1064, 1320, 1544, 1800, 2088, 2344, 2664, 2984, 3368, 3880, 4264, 4584, 4968, 5352, 5992, 6456, 6968, 7480, 7992, 8504, 9144, 9528, 11064},
  /* NPRB 016*/ { 424, 568, 696, 904, 1128, 1384, 1672, 1928, 2216, 2536, 2792, 3240, 3624, 4136, 4584, 4968, 5160, 5736, 6200, 6968, 7480, 7992, 8504, 9144, 9912, 10296, 11832},
  /* NPRB 017*/ { 456, 600, 744, 968, 1192, 1480, 1736, 2088, 2344, 2664, 2984, 3496, 3880, 4392, 4968, 5160, 5544, 6200, 6712, 7224, 7992, 8504, 9144, 9912, 10296, 10680, 12576},
  /* NPRB 018*/ { 488, 632, 776, 1032, 1288, 1544, 1864, 2216, 2536, 2856, 3112, 3624, 4136, 4584, 5160, 5544, 5992, 6456, 7224, 7736, 8248, 9144, 9528, 10296, 11064, 11448, 13536},
  /* NPRB 019*/ { 504, 680, 840, 1096, 1352, 1672, 1992, 2344, 2664, 2984, 3368, 3880, 4392, 4968, 5544, 5736, 6200, 6712, 7480, 8248, 8760, 9528, 10296, 11064, 11448, 12216, 14112},
  /* NPRB 020*/ { 536, 712, 872, 1160, 1416, 1736, 2088, 2472, 2792, 3112, 3496, 4008, 4584, 5160, 5736, 6200, 6456, 7224, 7992, 8504, 9144, 9912, 10680, 11448, 12216, 12576, 14688},
  /* NPRB 021*/ { 568, 744, 936, 1224, 1480, 1864, 2216, 2536, 2984, 3368, 3752, 4264, 4776, 5352, 5992, 6456, 6712, 7480, 8248, 9144, 9912, 10680, 11448, 12216, 12960, 13536, 15264},
  /* NPRB 022*/ { 600, 776, 968, 1256, 1544, 1928, 2280, 2664, 3112, 3496, 3880, 4392, 4968, 5736, 6200, 6712, 7224, 7992, 8760, 9528, 10296, 11064, 11832, 12576, 13536, 14112, 16416},
  /* NPRB 023*/ { 616, 808, 1000, 1320, 1608, 2024, 2408, 2792, 3240, 3624, 4008, 4584, 5352, 5992, 6456, 6968, 7480, 8248, 9144, 9912, 10680, 11448, 12576, 12960, 14112, 14688, 16992},
  /* NPRB 024*/ { 648, 872, 1064, 1384, 1736, 2088, 2472, 2984, 3368, 3752, 4264, 4776, 5544, 6200, 6968, 7224, 7736, 8760, 9528, 10296, 11064, 12216, 12960, 13536, 14688, 15264, 17568},
  /* NPRB 025*/ { 680, 904, 1096, 1416, 1800, 2216, 2600, 3112, 3496, 4008, 4392, 4968, 5736, 6456, 7224, 7736, 7992, 9144, 9912, 10680, 11448, 12576, 13536, 14112, 15264, 15840, 18336},
  /* NPRB 026*/ { 712, 936, 1160, 1480, 1864, 2280, 2728, 3240, 3624, 4136, 4584, 5352, 5992, 6712, 7480, 7992, 8504, 9528, 10296, 11064, 12216, 12960, 14112, 14688, 15840, 16416, 19080},
  /* NPRB 027*/ { 744, 968, 1192, 1544, 1928, 2344, 2792, 3368, 3752, 4264, 4776, 5544, 6200, 6968, 7736, 8248, 8760, 9912, 10680, 11448, 12576, 13536, 14688, 15264, 16416, 16992, 19848},
  /* NPRB 028*/ { 776, 1000, 1256, 1608, 1992, 2472, 2984, 3368, 3880, 4392, 4968, 5736, 6456, 7224, 7992, 8504, 9144, 10296, 11064, 12216, 12960, 14112, 15264, 15840, 16992, 17568, 20616},
  /* NPRB 029*/ { 776, 1032, 1288, 1672, 2088, 2536, 2984, 3496, 4008, 4584, 5160, 5992, 6712, 7480, 8248, 8760, 9528, 10296, 11448, 12576, 13536, 14688, 15840, 16416, 17568, 18336, 21384},
  /* NPRB 030*/ { 808, 1064, 1320, 1736, 2152, 2664, 3112, 3624, 4264, 4776, 5352, 5992, 6712, 7736, 8504, 9144, 9912, 10680, 11832, 12960, 14112, 15264, 16416, 16992, 18336, 19080, 22152},
  /* NPRB 031*/ { 840, 1128, 1384, 1800, 2216, 2728, 3240, 3752, 4392, 4968, 5544, 6200, 6968, 7992, 8760, 9528, 9912, 11064, 12216, 13536, 14688, 15840, 16992, 17568, 19080, 19848, 22920},
  /* NPRB 032*/ { 872, 1160, 1416, 1864, 2280, 2792, 3368, 3880, 4584, 5160, 5736, 6456, 7224, 8248, 9144, 9912, 10296, 11448, 12576, 13536, 14688, 15840, 16992, 18336, 19848, 20616, 23688},
  /* NPRB 033*/ { 904, 1192, 1480, 1928, 2344, 2856, 3496, 4008, 4584, 5160, 5736, 6712, 7480, 8504, 9528, 10296, 10680, 11832, 12960, 14112, 15264, 16416, 17568, 19080, 19848, 20616, 24496},
  /* NPRB 034*/ { 936, 1224, 1544, 1992, 2408, 2984, 3496, 4136, 4776, 5352, 5992, 6968, 7736, 8760, 9912, 10296, 11064, 12216, 13536, 14688, 15840, 16992, 18336, 19848, 20616, 21384, 25456},
  /* NPRB 035*/ { 968, 1256, 1544, 2024, 2472, 3112, 3624, 4264, 4968, 5544, 6200, 6968, 7992, 9144, 9912, 10680, 11448, 12576, 14112, 15264, 16416, 17568, 19080, 19848, 21384, 22152, 25456},
  /* NPRB 036*/ { 1000, 1288, 1608, 2088, 2600, 3112, 3752, 4392, 4968, 5736, 6200, 7224, 8248, 9144, 10296, 11064, 11832, 12960, 14112, 15264, 16992, 18336, 19080, 20616, 22152, 22920, 26416},
  /* NPRB 037*/ { 1032, 1352, 1672, 2152, 2664, 3240, 3880, 4584, 5160, 5736, 6456, 7480, 8504, 9528, 10680, 11448, 12216, 13536, 14688, 15840, 16992, 18336, 19848, 21384, 22920, 23688, 27376},
  /* NPRB 038*/ { 1032, 1384, 1672, 2216, 2728, 3368, 4008, 4584, 5352, 5992, 6712, 7736, 8760, 9912, 11064, 11832, 12216, 13536, 15264, 16416, 17568, 19080, 20616, 22152, 22920, 24496, 28336},
  /* NPRB 039*/ { 1064, 1416, 1736, 2280, 2792, 3496, 4136, 4776, 5544, 6200, 6712, 7736, 8760, 9912, 11064, 11832, 12576, 14112, 15264, 16992, 18336, 19848, 21384, 22152, 23688, 24496, 29296},
  /* NPRB 040*/ { 1096, 1416, 1800, 2344, 2856, 3496, 4136, 4968, 5544, 6200, 6968, 7992, 9144, 10296, 11448, 12216, 12960, 14688, 15840, 16992, 18336, 19848, 21384, 22920, 24496, 25456, 29296},
  /* NPRB 041*/ { 1128, 1480, 1800, 2408, 2984, 3624, 4264, 4968, 5736, 6456, 7224, 8248, 9528, 10680, 11832, 12576, 13536, 14688, 16416, 17568, 19080, 20616, 22152, 23688, 25456, 26416, 30576},
  /* NPRB 042*/ { 1160, 1544, 1864, 2472, 2984, 3752, 4392, 5160, 5992, 6712, 7480, 8504, 9528, 10680, 12216, 12960, 13536, 15264, 16416, 18336, 19848, 21384, 22920, 24496, 25456, 26416, 30576},
  /* NPRB 043*/ { 1192, 1544, 1928, 2536, 3112, 3752, 4584, 5352, 5992, 6712, 7480, 8760, 9912, 11064, 12216, 12960, 14112, 15264, 16992, 18336, 19848, 21384, 22920, 24496, 26416, 27376, 31704},
  /* NPRB 044*/ { 1224, 1608, 1992, 2536, 3112, 3880, 4584, 5352, 6200, 6968, 7736, 8760, 9912, 11448, 12576, 13536, 14112, 15840, 17568, 19080, 20616, 22152, 23688, 25456, 26416, 28336, 32856},
  /* NPRB 045*/ { 1256, 1608, 2024, 2600, 3240, 4008, 4776, 5544, 6200, 6968, 7992, 9144, 10296, 11448, 12960, 13536, 14688, 16416, 17568, 19080, 20616, 22920, 24496, 25456, 27376, 28336, 32856},
  /* NPRB 046*/ { 1256, 1672, 2088, 2664, 3240, 4008, 4776, 5736, 6456, 7224, 7992, 9144, 10680, 11832, 12960, 14112, 14688, 16416, 18336, 19848, 21384, 22920, 24496, 26416, 28336, 29296, 34008},
  /* NPRB 047*/ { 1288, 1736, 2088, 2728, 3368, 4136, 4968, 5736, 6456, 7480, 8248, 9528, 10680, 12216, 13536, 14688, 15264, 16992, 18336, 20616, 22152, 23688, 25456, 27376, 28336, 29296, 35160},
  /* NPRB 048*/ { 1320, 1736, 2152, 2792, 3496, 4264, 4968, 5992, 6712, 7480, 8504, 9528, 11064, 12216, 13536, 14688, 15840, 17568, 19080, 20616, 22152, 24496, 25456, 27376, 29296, 30576, 35160},
  /* NPRB 049*/ { 1352, 1800, 2216, 2856, 3496, 4392, 5160, 5992, 6968, 7736, 8504, 9912, 11064, 12576, 14112, 15264, 15840, 17568, 19080, 21384, 22920, 24496, 26416, 28336, 29296, 31704, 36696},
  /* NPRB 050*/ { 1384, 1800, 2216, 2856, 3624, 4392, 5160, 6200, 6968, 7992, 8760, 9912, 11448, 12960, 14112, 15264, 16416, 18336, 19848, 21384, 22920, 25456, 27376, 28336, 30576, 31704, 36696},
  /* NPRB 051*/ { 1416, 1864, 2280, 2984, 3624, 4584, 5352, 6200, 7224, 7992, 9144, 10296, 11832, 12960, 14688, 15840, 16416, 18336, 19848, 22152, 23688, 25456, 27376, 29296, 31704, 32856, 37888},
  /* NPRB 052*/ { 1416, 1864, 2344, 2984, 3752, 4584, 5352, 6456, 7224, 8248, 9144, 10680, 11832, 13536, 14688, 15840, 16992, 19080, 20616, 22152, 24496, 26416, 28336, 29296, 31704, 32856, 37888},
  /* NPRB 053*/ { 1480, 1928, 2344, 3112, 3752, 4776, 5544, 6456, 7480, 8248, 9144, 10680, 12216, 13536, 15264, 16416, 16992, 19080, 21384, 22920, 24496, 26416, 28336, 30576, 32856, 34008, 39232},
  /* NPRB 054*/ { 1480, 1992, 2408, 3112, 3880, 4776, 5736, 6712, 7480, 8504, 9528, 11064, 12216, 14112, 15264, 16416, 17568, 19848, 21384, 22920, 25456, 27376, 29296, 30576, 32856, 34008, 40576},
  /* NPRB 055*/ { 1544, 1992, 2472, 3240, 4008, 4776, 5736, 6712, 7736, 8760, 9528, 11064, 12576, 14112, 15840, 16992, 17568, 19848, 22152, 23688, 25456, 27376, 29296, 31704, 34008, 35160, 40576},
  /* NPRB 056*/ { 1544, 2024, 2536, 3240, 4008, 4968, 5992, 6712, 7736, 8760, 9912, 11448, 12576, 14688, 15840, 16992, 18336, 20616, 22152, 24496, 26416, 28336, 30576, 31704, 34008, 35160, 40576},
  /* NPRB 057*/ { 1608, 2088, 2536, 3368, 4136, 4968, 5992, 6968, 7992, 9144, 9912, 11448, 12960, 14688, 16416, 17568, 18336, 20616, 22920, 24496, 26416, 28336, 30576, 32856, 35160, 36696, 42368},
  /* NPRB 058*/ { 1608, 2088, 2600, 3368, 4136, 5160, 5992, 6968, 7992, 9144, 10296, 11832, 12960, 14688, 16416, 17568, 19080, 20616, 22920, 25456, 27376, 29296, 31704, 32856, 35160, 36696, 42368},
  /* NPRB 059*/ { 1608, 2152, 2664, 3496, 4264, 5160, 6200, 7224, 8248, 9144, 10296, 11832, 13536, 15264, 16992, 18336, 19080, 21384, 23688, 25456, 27376, 29296, 31704, 34008, 36696, 37888, 43816},
  /* NPRB 060*/ { 1672, 2152, 2664, 3496, 4264, 5352, 6200, 7224, 8504, 9528, 10680, 12216, 13536, 15264, 16992, 18336, 19848, 21384, 23688, 25456, 28336, 30576, 32856, 34008, 36696, 37888, 43816},
  /* NPRB 061*/ { 1672, 2216, 2728, 3624, 4392, 5352, 6456, 7480, 8504, 9528, 10680, 12216, 14112, 15840, 17568, 18336, 19848, 22152, 24496, 26416, 28336, 30576, 32856, 35160, 36696, 39232, 45352},
  /* NPRB 062*/ { 1736, 2280, 2792, 3624, 4392, 5544, 6456, 7480, 8760, 9912, 11064, 12576, 14112, 15840, 17568, 19080, 19848, 22152, 24496, 26416, 29296, 31704, 34008, 35160, 37888, 39232, 45352},
  /* NPRB 063*/ { 1736, 2280, 2856, 3624, 4584, 5544, 6456, 7736, 8760, 9912, 11064, 12576, 14112, 16416, 18336, 19080, 20616, 22920, 24496, 27376, 29296, 31704, 34008, 36696, 37888, 40576, 46888},
  /* NPRB 064*/ { 1800, 2344, 2856, 3752, 4584, 5736, 6712, 7736, 9144, 10296, 11448, 12960, 14688, 16416, 18336, 19848, 20616, 22920, 25456, 27376, 29296, 31704, 34008, 36696, 39232, 40576, 46888},
  /* NPRB 065*/ { 1800, 2344, 2856, 3752, 4584, 5736, 6712, 7992, 9144, 10296, 11448, 12960, 14688, 16992, 18336, 19848, 21384, 23688, 25456, 28336, 30576, 32856, 35160, 37888, 39232, 40576, 48936},
  /* NPRB 066*/ { 1800, 2408, 2984, 3880, 4776, 5736, 6968, 7992, 9144, 10296, 11448, 13536, 15264, 16992, 19080, 20616, 21384, 23688, 26416, 28336, 30576, 32856, 35160, 37888, 40576, 42368, 48936},
  /* NPRB 067*/ { 1864, 2472, 2984, 3880, 4776, 5992, 6968, 8248, 9528, 10680, 11832, 13536, 15264, 16992, 19080, 20616, 22152, 24496, 26416, 29296, 31704, 34008, 36696, 37888, 40576, 42368, 48936},
  /* NPRB 068*/ { 1864, 2472, 3112, 4008, 4968, 5992, 6968, 8248, 9528, 10680, 11832, 13536, 15264, 17568, 19848, 20616, 22152, 24496, 27376, 29296, 31704, 34008, 36696, 39232, 42368, 43816, 51024},
  /* NPRB 069*/ { 1928, 2536, 3112, 4008, 4968, 5992, 7224, 8504, 9528, 11064, 12216, 14112, 15840, 17568, 19848, 21384, 22152, 24496, 27376, 29296, 31704, 35160, 36696, 39232, 42368, 43816, 51024},
  /* NPRB 070*/ { 1928, 2536, 3112, 4136, 4968, 6200, 7224, 8504, 9912, 11064, 12216, 14112, 15840, 18336, 19848, 21384, 22920, 25456, 27376, 30576, 32856, 35160, 37888, 40576, 42368, 43816, 52752},
  /* NPRB 071*/ { 1992, 2600, 3240, 4136, 5160, 6200, 7480, 8760, 9912, 11064, 12576, 14112, 16416, 18336, 20616, 22152, 22920, 25456, 28336, 30576, 32856, 35160, 37888, 40576, 43816, 45352, 52752},
  /* NPRB 072*/ { 1992, 2600, 3240, 4264, 5160, 6200, 7480, 8760, 9912, 11448, 12576, 14688, 16416, 18336, 20616, 22152, 23688, 26416, 28336, 30576, 34008, 36696, 39232, 40576, 43816, 45352, 52752},
  /* NPRB 073*/ { 2024, 2664, 3240, 4264, 5160, 6456, 7736, 8760, 10296, 11448, 12960, 14688, 16416, 19080, 20616, 22152, 23688, 26416, 29296, 31704, 34008, 36696, 39232, 42368, 45352, 46888, 55056},
  /* NPRB 074*/ { 2088, 2728, 3368, 4392, 5352, 6456, 7736, 9144, 10296, 11832, 12960, 14688, 16992, 19080, 21384, 22920, 24496, 26416, 29296, 31704, 34008, 36696, 40576, 42368, 45352, 46888, 55056},
  /* NPRB 075*/ { 2088, 2728, 3368, 4392, 5352, 6712, 7736, 9144, 10680, 11832, 12960, 15264, 16992, 19080, 21384, 22920, 24496, 27376, 29296, 32856, 35160, 37888, 40576, 43816, 45352, 46888, 55056},
  /* NPRB 076*/ { 2088, 2792, 3368, 4392, 5544, 6712, 7992, 9144, 10680, 11832, 13536, 15264, 17568, 19848, 22152, 23688, 24496, 27376, 30576, 32856, 35160, 37888, 40576, 43816, 46888, 48936, 55056},
  /* NPRB 077*/ { 2152, 2792, 3496, 4584, 5544, 6712, 7992, 9528, 10680, 12216, 13536, 15840, 17568, 19848, 22152, 23688, 25456, 27376, 30576, 32856, 35160, 39232, 42368, 43816, 46888, 48936, 57336},
  /* NPRB 078*/ { 2152, 2856, 3496, 4584, 5544, 6968, 8248, 9528, 11064, 12216, 13536, 15840, 17568, 19848, 22152, 23688, 25456, 28336, 30576, 34008, 36696, 39232, 42368, 45352, 46888, 48936, 57336},
  /* NPRB 079*/ { 2216, 2856, 3496, 4584, 5736, 6968, 8248, 9528, 11064, 12576, 14112, 15840, 18336, 20616, 22920, 24496, 25456, 28336, 31704, 34008, 36696, 39232, 42368, 45352, 48936, 51024, 57336},
  /* NPRB 080*/ { 2216, 2856, 3624, 4776, 5736, 6968, 8248, 9912, 11064, 12576, 14112, 16416, 18336, 20616, 22920, 24496, 26416, 29296, 31704, 34008, 36696, 40576, 43816, 45352, 48936, 51024, 59256},
  /* NPRB 081*/ { 2280, 2984, 3624, 4776, 5736, 7224, 8504, 9912, 11448, 12960, 14112, 16416, 18336, 20616, 22920, 24496, 26416, 29296, 31704, 35160, 37888, 40576, 43816, 46888, 48936, 51024, 59256},
  /* NPRB 082*/ { 2280, 2984, 3624, 4776, 5992, 7224, 8504, 9912, 11448, 12960, 14688, 16416, 19080, 21384, 23688, 25456, 26416, 29296, 32856, 35160, 37888, 40576, 43816, 46888, 51024, 52752, 59256},
  /* NPRB 083*/ { 2280, 2984, 3752, 4776, 5992, 7224, 8760, 10296, 11448, 12960, 14688, 16992, 19080, 21384, 23688, 25456, 27376, 30576, 32856, 35160, 39232, 42368, 45352, 46888, 51024, 52752, 61664},
  /* NPRB 084*/ { 2344, 3112, 3752, 4968, 5992, 7480, 8760, 10296, 11832, 13536, 14688, 16992, 19080, 21384, 24496, 25456, 27376, 30576, 32856, 36696, 39232, 42368, 45352, 48936, 51024, 52752, 61664},
  /* NPRB 085*/ { 2344, 3112, 3880, 4968, 5992, 7480, 8760, 10296, 11832, 13536, 14688, 16992, 19080, 22152, 24496, 26416, 27376, 30576, 34008, 36696, 39232, 42368, 45352, 48936, 52752, 55056, 61664},
  /* NPRB 086*/ { 2408, 3112, 3880, 4968, 6200, 7480, 9144, 10680, 12216, 13536, 15264, 17568, 19848, 22152, 24496, 26416, 28336, 30576, 34008, 36696, 40576, 43816, 46888, 48936, 52752, 55056, 63776},
  /* NPRB 087*/ { 2408, 3240, 3880, 5160, 6200, 7736, 9144, 10680, 12216, 13536, 15264, 17568, 19848, 22152, 25456, 26416, 28336, 31704, 34008, 37888, 40576, 43816, 46888, 51024, 52752, 55056, 63776},
  /* NPRB 088*/ { 2472, 3240, 4008, 5160, 6200, 7736, 9144, 10680, 12216, 14112, 15264, 17568, 19848, 22920, 25456, 27376, 28336, 31704, 35160, 37888, 40576, 43816, 46888, 51024, 52752, 55056, 63776},
  /* NPRB 089*/ { 2472, 3240, 4008, 5160, 6456, 7736, 9144, 11064, 12576, 14112, 15840, 18336, 20616, 22920, 25456, 27376, 29296, 31704, 35160, 37888, 42368, 45352, 48936, 51024, 55056, 57336, 66592},
  /* NPRB 090*/ { 2536, 3240, 4008, 5352, 6456, 7992, 9528, 11064, 12576, 14112, 15840, 18336, 20616, 22920, 25456, 27376, 29296, 32856, 35160, 39232, 42368, 45352, 48936, 51024, 55056, 57336, 66592},
  /* NPRB 091*/ { 2536, 3368, 4136, 5352, 6456, 7992, 9528, 11064, 12576, 14112, 15840, 18336, 20616, 23688, 26416, 28336, 29296, 32856, 36696, 39232, 42368, 45352, 48936, 52752, 55056, 57336, 66592},
  /* NPRB 092*/ { 2536, 3368, 4136, 5352, 6456, 7992, 9528, 11448, 12960, 14688, 16416, 18336, 21384, 23688, 26416, 28336, 30576, 32856, 36696, 39232, 42368, 46888, 48936, 52752, 57336, 59256, 68808},
  /* NPRB 093*/ { 2600, 3368, 4136, 5352, 6712, 8248, 9528, 11448, 12960, 14688, 16416, 19080, 21384, 23688, 26416, 28336, 30576, 34008, 36696, 40576, 43816, 46888, 51024, 52752, 57336, 59256, 68808},
  /* NPRB 094*/ { 2600, 3496, 4264, 5544, 6712, 8248, 9912, 11448, 12960, 14688, 16416, 19080, 21384, 24496, 27376, 29296, 30576, 34008, 37888, 40576, 43816, 46888, 51024, 55056, 57336, 59256, 68808},
  /* NPRB 095*/ { 2664, 3496, 4264, 5544, 6712, 8248, 9912, 11448, 13536, 15264, 16992, 19080, 21384, 24496, 27376, 29296, 30576, 34008, 37888, 40576, 43816, 46888, 51024, 55056, 57336, 61664, 71112},
  /* NPRB 096*/ { 2664, 3496, 4264, 5544, 6968, 8504, 9912, 11832, 13536, 15264, 16992, 19080, 22152, 24496, 27376, 29296, 31704, 35160, 37888, 40576, 45352, 48936, 51024, 55056, 59256, 61664, 71112},
  /* NPRB 097*/ { 2728, 3496, 4392, 5736, 6968, 8504, 10296, 11832, 13536, 15264, 16992, 19848, 22152, 25456, 28336, 29296, 31704, 35160, 37888, 42368, 45352, 48936, 52752, 55056, 59256, 61664, 71112},
  /* NPRB 098*/ { 2728, 3624, 4392, 5736, 6968, 8760, 10296, 11832, 13536, 15264, 16992, 19848, 22152, 25456, 28336, 30576, 31704, 35160, 39232, 42368, 45352, 48936, 52752, 57336, 59256, 61664, 73712},
  /* NPRB 099*/ { 2728, 3624, 4392, 5736, 6968, 8760, 10296, 12216, 14112, 15840, 17568, 19848, 22920, 25456, 28336, 30576, 31704, 35160, 39232, 42368, 46888, 48936, 52752, 57336, 61664, 63776, 73712},
  /* NPRB 100*/ { 2792, 3624, 4584, 5736, 7224, 8760, 10296, 12216, 14112, 15840, 17568, 19848, 22920, 25456, 28336, 30576, 32856, 36696, 39232, 43816, 46888, 51024, 55056, 57336, 61664, 63776, 75376},
  /* NPRB 101*/ { 2792, 3752, 4584, 5992, 7224, 8760, 10680, 12216, 14112, 15840, 17568, 20616, 22920, 26416, 29296, 30576, 32856, 36696, 40576, 43816, 46888, 51024, 55056, 57336, 61664, 63776, 75376},
  /* NPRB 102*/ { 2856, 3752, 4584, 5992, 7224, 9144, 10680, 12576, 14112, 16416, 18336, 20616, 23688, 26416, 29296, 31704, 32856, 36696, 40576, 43816, 46888, 51024, 55056, 59256, 61664, 63776, 75376},
  /* NPRB 103*/ { 2856, 3752, 4584, 5992, 7480, 9144, 10680, 12576, 14688, 16416, 18336, 20616, 23688, 26416, 29296, 31704, 34008, 36696, 40576, 43816, 48936, 51024, 55056, 59256, 63776, 66592, 75376},
  /* NPRB 104*/ { 2856, 3752, 4584, 5992, 7480, 9144, 10680, 12576, 14688, 16416, 18336, 21384, 23688, 26416, 29296, 31704, 34008, 37888, 40576, 45352, 48936, 52752, 57336, 59256, 63776, 66592, 75376},
  /* NPRB 105*/ { 2984, 3880, 4776, 6200, 7480, 9144, 11064, 12960, 14688, 16416, 18336, 21384, 23688, 27376, 30576, 31704, 34008, 37888, 42368, 45352, 48936, 52752, 57336, 59256, 63776, 66592, 75376},
  /* NPRB 106*/ { 2984, 3880, 4776, 6200, 7480, 9528, 11064, 12960, 14688, 16992, 18336, 21384, 24496, 27376, 30576, 32856, 34008, 37888, 42368, 45352, 48936, 52752, 57336, 61664, 63776, 66592, 75376},
  /* NPRB 107*/ { 2984, 3880, 4776, 6200, 7736, 9528, 11064, 12960, 15264, 16992, 19080, 21384, 24496, 27376, 30576, 32856, 35160, 39232, 42368, 46888, 48936, 52752, 57336, 61664, 66592, 68808, 75376},
  /* NPRB 108*/ { 2984, 4008, 4776, 6200, 7736, 9528, 11448, 12960, 15264, 16992, 19080, 22152, 24496, 27376, 30576, 32856, 35160, 39232, 42368, 46888, 51024, 55056, 59256, 61664, 66592, 68808, 75376},
  /* NPRB 109*/ { 2984, 4008, 4968, 6456, 7736, 9528, 11448, 13536, 15264, 16992, 19080, 22152, 24496, 28336, 31704, 34008, 35160, 39232, 43816, 46888, 51024, 55056, 59256, 61664, 66592, 68808, 75376},
  /* NPRB 110*/ { 3112, 4008, 4968, 6456, 7992, 9528, 11448, 13536, 15264, 17568, 19080, 22152, 25456, 28336, 31704, 34008, 35160, 39232, 43816, 46888, 51024, 55056, 59256, 63776, 66592, 71112, 75376}

};
/*********************************lhl修改完成20140410*****************************/

/*******************lhl改动*******************/
/********调用：初始化时调用****/
void Init_Uemac_Scheduler(void);//UE 调度器初始化
/******************调用：RLC IOCTL到来时调用****************/
void DoRefreshRLCBuffserRequest (RlcBufferRequest* params);//更新RLC请求
RLC_Request* Rlc_MacRequest_copy(RlcBufferRequest *temp);//复制节点rlcBufferRequest
LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_copy(MAC_LogicalChannelConfig_IoctrlMsg *temp);//复制逻辑信道属性节点
RlcBufferRequest* Mac_RlcRequest_copy(RLC_Request *temp);//复制RlcBufferRequest节点
RLC_Request* Mac_MacRequest_copy(RLC_Request *temp);//复制RlcBufferRequest节点
MacBufferStatus_BSR_Info* MacBufferStatus_BSR_Info_copy(void);//复制BSR
/******************调用：每个TTL 中断IOCTL到来时调用****************/
void FlushBj(void);//每个TTI更新逻辑信道的BJ
/******************调用：收到ULGRANT时调用（由receive_ulgrant 物理适配层的IOCTL提供，指示上行信息的DCI）****************/
//u32 DoReceiveULgrant_Tbsize(u32 receive_ulgrant);
//LHL modified 20141105
u32 DoReceiveULgrant_Tbsize(Regular_ULgrant receive_ulgrant);
/******************调用：收到RARULgrant时调用（由receive_rar_ulgrant解复用模块提供）****************/
u32 DoReceiveRARULgrant_Tbsize(RAR_ULgrant *receive_rar_ulgrant);//由RAR_ULGrant获取TBSize(基站分配总TBsize)
/********调用：调用DoReceiveRARULgrant_Tbsize或者DoReceiveULgrant_Tbsize后再调用***********/
u32 GetTBsize_Allocation(u32 tbsize);//返回tbsize,单位byte,考虑了发送BSR的资源
u32 BufferSize2BsrSize(u32 buffersize);//由Buffer大小转换为申请BSR大小的index,错误返回-1
LogicalChannelConfigInfo* LogicalChannel_ConfigInfo_Rank_Priority(void);//根据逻辑信道优先级对逻辑信道排序
MacBufferStatus* PrioritySort(void);//根据逻辑信道优先级顺序，对MacBufferStatus链表按优先级排序
void DoProduceBsr_LCGZeroToData(void);//由LCGZeroToData条件，生成BSR
void DoProduceBsr_PeriodicBSRTimer(void);//由周期定时器超时 && m_freshUlBsr==true,生成BSR
void DoProduceBsr_RetxBSRTimer(void);//由重传定时器超时生成BSR
void RefreshBufferStatusForBSR(void);//由MAC层保存的RLC请求结构，更新LCG的待发数据
void Free_Bsr_Info(void);//复用模块使用BSR信息后调用该函数，释放BSR资源
/********调用：在调用GetTBsize_Allocation函数后调用****/
void DoResourceAllocation(u32 tb_size);//资源分配
/********调用：在RRC IOCTL到来时调用，或者多逻辑信道默认配置时调用****/
void Init_LogicalChannel_ConfigInfo(MAC_LogicalChannelConfig_IoctrlMsg *lc_info);//更新逻辑信道属性结构和BJ结构,RRC逻辑信道配置IOCTL触发时调用
/******输入msg为GetTBsize_Allocation返回的资源大小，输出:*num是是要发送的IOCTL的buffer长度，void *是RLC 报告的头指针**********/
/********调用：在ResourceAllocation_Algorithm中已经封装调用，若无特殊情况不用其他模块显示调用****************/
void * ResourceAllocation_Algorithm(u32 msg,u32 *num);//UE侧资源分配算法：令牌桶,RlcBufferRequest MACReportToRlc
void Empty_MACBuffer_Rlc(void);//清空MAC层的RLC请求Buffer
void Free_Uemac_Scheduler_Resource(void);//释放UE MAC SCHEDULER资源
void Delete_LogicalChannel_ConfigInfo(u16 lcid_delete);
u32 GetRbsize(u32 channel_bandwidth);//有信道带宽获取RB数量
/*****RLC请求IOCL到来是调用，输入为RLC请求数组的头指针，以及RLC请求数组的长度，输出为链表形式*************/
RlcBufferRequest * RlcRequest_arraytolist(RLC_Request *rlc_request_array,u32 num);//modified 20140526,琑LC请求由数组转换为链表
void Zero_LogicalChannel_Bj_Reset();//将BJ置为0，重配置使用
/*******输入RlcBufferRequest为RLC report链表，输出：*num是要发送的IOCTL的buffer长度（即RLC 报告数组+长度的总长），返回RLC 报告的头指针***************/
void * RlcRequest_listtoarray( RlcBufferRequest *rlc_request,u32 *num);//modified 20140526,RLC请求由链表转换为数组
/*********lhl modified 20140526***********
**int My_Pow(int num,int n);//计算num的n次幂，其中n为整数
**int My_Floor(int x);//下取整
**int My_Ceil(int x);//上取整
**int My_Log2(int rb);//输入为RB数量
**********lhl modified 20140526**********/


u32 My_Pow(u32 num,u32 n);//计算num的n次幂，其中n为整数
//static int My_Floor(double x);//下取整
//static int My_Ceil(double x);//上取整,modified by lhl ,20140514
u32 My_Log2(u32 rb);//输入为RB数量
/*******************lhl改动结束*******************/

/******LHL modified 20140928***********/
void ResourceAllocation_foreach_logicalchannel(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 statuslength,u32 retxlength,u32 txlength,u32 *res_length);
void ResourceAllocation_logicalchannel_firststatusbuffer(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 statuslength,u32 retxlength,u32 txlength,u32 *res_length);
void ResourceAllocation_logicalchannel_firstretxbuffer(RlcBufferRequest *mac_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 retxlength,u32 txlength,u32 *res_length);
void leftresource_lessthan_retxbuffer_eachlogicalchannel(RlcBufferRequest *mac_rlc_report,RlcBufferRequest *temp_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 retxlength,u32 txlength,u32 *res_length);
void leftresource_morethan_retxbuffer_eachlogicalchannel(RlcBufferRequest *mac_rlc_report,RlcBufferRequest *temp_rlc_report,MacBufferStatus *temp_mac_buffer, LogicalChannelBj *temp_logicalchannel_bj,u32 retxlength,u32 txlength,u32 *res_length);

#endif

