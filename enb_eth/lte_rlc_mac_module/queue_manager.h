#include "../fsm/fsmdec.h"

#include <linux/if_ether.h>
#include <linux/list.h>
#include <linux/kernel.h>

#ifndef _QUEUE_MANAGER_H
#define _QUEUE_MANAGER_H

#define UE_RA_MAXNUM 10  //一个时刻接入的UE的最大数目
typedef struct S_UEinfo //每个UE对应的基本信息，用于调度用的，比如RNTI，CQI等
{
    u16    rnti;             //
    //u16    pgtype;           //0,1,2
    u16    sfn;              //系统帧号，0—1023
    u16    subframeN;        //子帧号，0——9
    u16    crc;
    u16    harqindex;
	u8    harq_result;//20141008
    u16    sr;
    u16    cqi;
    u16    pmi;
    u16    ta;	 
    u16    datasizeof_bsr;
}__attribute__((packed))S_UEinfo;





#define RA_UE_NUMBER_MAX 10
typedef struct S_RAinfo //当有UE接入eNB时，物理层会检测到RNTI和RAPID上报 按这样的格式
{
   u16    total_num;//当前随机接入检测到的总用户数
   u16    index;//该用户在总用户数中的下标，从0开始取值
    u16    ra_rnti;             //
   // u16    pgtype;           //0,1,2。0:新建用户 1:删除用户 2:已有用户更新信息
    u16    sfn; //             //系统帧号，0—1023
    u16    subframeN;//        //子帧号，0——9
    u16    crc;
    u16    harqindex;
	u8 harq_result;
    u16    sr;
    u16    cqi;//
    u16    pmi;
    u16    ta;//
    u8    rapid;//
}__attribute__((packed))S_RAinfo;
/*typedef struct S_RAinfo //当有UE接入eNB时，物理层会检测到RNTI和RAPID上报 按这样的格式
{
   u16    total_num;//当前随机接入检测到的总用户数
   u16    index;//该用户在总用户数中的下标，从0开始取值
    u16    ra_rnti;             //
   // u16    pgtype;           //0,1,2。0:新建用户 1:删除用户 2:已有用户更新信息
    u16    sfn; //             //系统帧号，0—1023
    u16    subframeN;//        //子帧号，0——9
    u16    crc;
    u16    harqindex;
	u8 harq_result;
    u16    sr;
    u16    cqi;//
    u16    pmi;
    u16    ta;//
    u16    rapid;//
}__attribute__((packed))S_RAinfo;*/

typedef struct S_ULsched_QueueEntry //队列里的元素
{
	struct S_UEinfo info;
    struct list_head list_h;
}S_ULsched_QueueEntry;
typedef S_ULsched_QueueEntry* P_ULsched_QueueEntry;


typedef struct S_RAsched_QueueEntry
{
	struct S_RAinfo info;
    struct list_head list_h;
}S_RAsched_QueueEntry;
typedef S_RAsched_QueueEntry* P_RAsched_QueueEntry;

//UL调度队列的管理函数 声明
int insert_ULschedQue(struct list_head *listhead,struct S_UEinfo * newue);//从尾部插入一个
struct S_UEinfo* find_ULschedQue(struct list_head *listhead,u16 ue_rnti);//查找
struct S_UEinfo take_first_ULschedQue(struct list_head *listhead);//从头部取出一个
int clean_ULschedQue(struct list_head *listhead);
int print_ULschedQue(struct list_head *listhead);
//RA调度队列的管理函数 声明
int insert_RAschedQue(struct list_head *listhead,struct S_RAinfo * newue);
struct S_RAinfo* find_RAschedQue(struct list_head *listhead,u16 ue_rnti);
struct S_RAinfo take_first_RAschedQue(struct list_head *listhead);
int clean_RAschedQue(struct list_head *listhead);
int print_RAschedQue(struct list_head *listhead);


#endif


