/*******************************************************************
author:HuangQiong   Date:20140521 v0
description: 该文件的函数主要是现实上行调度队列的管理
              （队列的实现使用链表的方式，linux中的list）
note: 使用了两种方式实现
      第一种方式是编写函数的形式，但只是针对一种队列。
	  第二种方式是使用宏定义的方式，可以适用于不同的队列，
	   但是要求队列元素的结构具有一致性。
*******************************************************************/
#include <linux/if_ether.h>
#include <linux/list.h>
#include <linux/kernel.h>

#include "queue_manager.h"


/******************第一种方式**函数************************/

/****************************************************************************
funcName: 初始化 直接使用list.h中使用的，需要定义一个表头，比如struct list_head *listhead
function: INIT_LIST_HEAD(struct list_head *list)
parameter: list_head结构的指针，即定义的表头
return: non
*****************************************************************************/

/****************************************************************************
funcName: insert_ULschedQue(struct list_head *listhead,struct S_UEinfo * newue)
function: 将一个用户（需要进行上行调度的）从队列尾部加入到队列中
parameter: listptr表示指向该队列表头，newue则是该用户的相应信息
return: 0表示成功添加，非零表示出错
*****************************************************************************/
int insert_ULschedQue(struct list_head *listhead,struct S_UEinfo * newue)
{   
	int flag =0 ;
	P_ULsched_QueueEntry tmp = (P_ULsched_QueueEntry)fsm_mem_alloc(sizeof(S_ULsched_QueueEntry));
	if(tmp == NULL)
	{   
		fsm_printf("alloc memory error \n");
		flag = 1; //内存分配出错 
	}
	tmp->info = *newue; //复制UE的信息
	list_add_tail(&(tmp->list_h),listhead);
	return flag;
}

/*******************************************************************
funcName: find_ULschedQue(struct list_head *listhead,u16 ue_rnti)
function: 在队列中查找某个UE，如果找到则返回一个指针指向该UE，如果没有找到则返回NULL
parameter: listptr表示指向该队列的指针，ue_rnti是UE的RNTI,可能是C_RNTI,也可能是T_C_RNTI
return: 被取到的用户的信息
*******************************************************************/
struct S_UEinfo* find_ULschedQue(struct list_head *listhead,u16 ue_rnti)
{
	P_ULsched_QueueEntry tmp = NULL;
	struct S_UEinfo* get_ue_info = NULL;
	struct list_head *pos = NULL;
	struct list_head *p = NULL;
	int flag = 0 ;
	list_for_each_safe(pos,p,listhead)//遍历 //隐藏对空队列的判断 不需要再增加测试语句   
	{        
		tmp = list_entry(pos,S_ULsched_QueueEntry,list_h);//取每个元素	
		if(tmp->info.rnti == ue_rnti) 
		{
			get_ue_info	= &(tmp->info);	
			break;
		}
	}
	return get_ue_info;
}


/*******************************************************************
funcName: take_first_ULschedQue(struct list_head *listhead)
function: 从队列头部取出一个用户，并将用户从该队列删除。
parameter: listptr表示指向该队列的指针
return: 被取到的用户的信息
*******************************************************************/
struct S_UEinfo take_first_ULschedQue(struct list_head *listhead)
{
	P_ULsched_QueueEntry tmp = NULL;
	struct S_UEinfo tmp_info = {0};//初始化为0
	if(list_empty_careful(listhead))//检查队列为空
	{
		fsm_printf("The queue is empty,no element can be taken. \n");
		return tmp_info;//返回默认值为0 
	}
	tmp = list_first_entry(listhead,S_ULsched_QueueEntry,list_h);//第三个参数是第二个参数（即整个数据结构）中的list_head变量的变量名 第一参数是表头
	tmp_info = tmp->info;
	list_del_init(&tmp->list_h);//从表中删除该节点 
	return tmp_info;
}

/*******************************************************************
funcName: clean_ULschedQue(struct list_head *listhead)
function: 清空队列，释放内存
parameter: 表头
return: 0表示成功清空，非零表示出错
*******************************************************************/
int clean_ULschedQue(struct list_head *listhead)
{
	P_ULsched_QueueEntry tmp = NULL;
	struct list_head *pos = NULL;
	struct list_head *p = NULL;
	int flag = 0 ;
	list_for_each_safe(pos,p,listhead)//遍历 //隐藏对空队列的判断 不需要再增加测试语句   
	{        
		tmp = list_entry(pos,S_ULsched_QueueEntry,list_h);//取每个元素		
		list_del_init(&tmp->list_h);		
		if(tmp != NULL)		   
			fsm_mem_free(tmp); //因为在insert时进行了malloc分配内存，所以该处需要释放内存		
		else 		
		{   
			fsm_printf("free memory error \n");
			flag = 1; //出错，修改标志 //此处的处理还待考虑
		}
		fsm_printf("cleaning \n");//testing code 
	}
	if(list_empty_careful(listhead) != 1)//检查队列不为空，则是出错了
		flag = 2;
	return flag;
}

/*******************************************************************
funcName: print_ULschedQue(struct list_head *listhead)
function: 遍历队列并打印出数据信息
parameter: 表头
return: 0表示成功打印，非零表示出错
*******************************************************************/
int print_ULschedQue(struct list_head *listhead)
{
	P_ULsched_QueueEntry tmp = NULL;
	struct list_head *pos = NULL;
	struct list_head *p = NULL;
	list_for_each_safe(pos,p,listhead)//遍历    
	{        
		tmp = list_entry(pos,struct S_ULsched_QueueEntry,list_h);		
		fsm_printf("RNTI:%d,CQI:%d\n",tmp->info.rnti,tmp->info.cqi);    
	}
	fsm_printf("--------------------\n");
	return 0;
}
/***********
*下面几个函数是管理RA_Sched_Queue的，只是变量的类型不同
************/
int insert_RAschedQue(struct list_head *listhead,struct S_RAinfo * newue)
{   
	int flag =0 ;
	P_RAsched_QueueEntry tmp = (P_RAsched_QueueEntry)fsm_mem_alloc(sizeof(S_RAsched_QueueEntry));
	if(tmp == NULL)
	{   
		fsm_printf("alloc memory error \n");
		flag = 1; //内存分配出错 
	}
	tmp->info = *newue; //复制UE的信息
	list_add_tail(&(tmp->list_h),listhead);
	return flag;
}

struct S_RAinfo* find_RAschedQue(struct list_head *listhead,u16 ue_rnti)
{
	P_RAsched_QueueEntry tmp = NULL;
	struct S_RAinfo* get_ue_info = NULL;
	struct list_head *pos = NULL;
	struct list_head *p = NULL;
	int flag = 0 ;
	list_for_each_safe(pos,p,listhead)//遍历 //隐藏对空队列的判断 不需要再增加测试语句   
	{        
		tmp = list_entry(pos,S_RAsched_QueueEntry,list_h);//取每个元素	
		if(tmp->info.ra_rnti == ue_rnti) 
		{
			get_ue_info	= &(tmp->info);	
			break;
		}
	}
	return get_ue_info;
}

struct S_RAinfo take_first_RAschedQue(struct list_head *listhead)
{
	P_RAsched_QueueEntry tmp = NULL;
	struct S_RAinfo tmp_info = {0};//初始化为0
	if(list_empty_careful(listhead))//检查队列为空
	{
		fsm_printf("The queue is empty,no element can be taken. \n");
		return tmp_info;//返回默认值为0 
	}
	tmp = list_first_entry(listhead,S_RAsched_QueueEntry,list_h);//第三个参数是第二个参数（即整个数据结构）中的list_head变量的变量名 第一参数是表头
	tmp_info = tmp->info;
	list_del_init(&tmp->list_h);//从表中删除该节点 
	return tmp_info;
}

int clean_RAschedQue(struct list_head *listhead)
{
	P_RAsched_QueueEntry tmp = NULL;
	struct list_head *pos = NULL;
	struct list_head *p = NULL;
	int flag = 0 ;
	list_for_each_safe(pos,p,listhead)//遍历 //隐藏对空队列的判断 不需要再增加测试语句   
	{        
		tmp = list_entry(pos,S_RAsched_QueueEntry,list_h);//取每个元素		
		list_del_init(&tmp->list_h);		
		if(tmp != NULL)		   
			fsm_mem_free(tmp); //因为在insert时进行了malloc分配内存，所以该处需要释放内存		
		else 		
		{   
			fsm_printf("free memory error \n");
			flag = 1; //出错，修改标志 //此处的处理还待考虑
		}
		fsm_printf("cleaning \n");//testing code 
	}
	if(list_empty_careful(listhead) != 1)//检查队列不为空，则是出错了
		flag = 2;
	return flag;
}

int print_RAschedQue(struct list_head *listhead)
{
	P_RAsched_QueueEntry tmp = NULL;
	struct list_head *pos = NULL;
	struct list_head *p = NULL;
	list_for_each_safe(pos,p,listhead)//遍历    
	{        
		tmp = list_entry(pos,struct S_RAsched_QueueEntry,list_h);		
		fsm_printf("RA-RNTI:%d\n",tmp->info.ra_rnti);    
	}
	fsm_printf("--------------------\n");
	return 0;
}

/***************第二种方式***宏定义********************************/
/*****************************************************************
description: 可以针对不同队列，进行"从尾部加入元素","从头部取走第一个元素"，
				"清空队列"，"打印队列中现有的元素"
note: 使用该宏定义的前提是，参数的数据结构格式一定，
	  如下所示，必须具有info和lish_h这两个成员变量，即变量名。
	  此外，无法返回执行结果的标志
typedef struct myqueue
{ 
	struct datatype info ; 
	struct list_head lish_h; 
}myqueue; 

******************************************************************/


#define insert_queue(listhead,type,info_ptr) {	type* tmp = (type*)malloc(sizeof(type));\
	tmp->info = *info_ptr;\
	list_add_tail(&(tmp->list_h),listhead);}

/*********************************************************************************
funcName: take_first_entry(listhead,type,info_ptr)
function: 从队列头部取出一个用户，并将用户从该队列删除。
parameter: listhead是队列头，type是队列元素的数据类型，info_ptr是指向该元素（数据结构）的数据成员变量指针
return: 被取到的元素的数据部分 
note:不需要写分号
例如 typedef struct myqueue
    { struct datatype info ; 
	  struct list_head lish_h;} myqueue; 
 用法
     struct datatype myinfo;
     take_first_entry(listhead,myqueue,&myinfo)

**********************************************************************************/
#define take_first_entry(listhead,type,info_ptr) {  type* tmp = NULL;\
	tmp = list_first_entry(listhead,type,list_h);\
	list_del_init(&tmp->list_h); \
	*info_ptr=tmp->info;} 


#define clean_queue(listhead,type) { type* tmp = NULL;\
	struct list_head *pos = NULL;\
	struct list_head *p = NULL;\
	list_for_each_safe(pos,p,listhead) { \		tmp = list_entry(pos,type,list_h);\		list_del_init(&tmp->list_h);\		if(tmp != NULL)\		   free(tmp);} }


#define print_queue(listhead,type) {  type* tmp = NULL;\
	struct list_head *pos = NULL;\
	struct list_head *p = NULL;\
	list_for_each_safe(pos,p,listhead){ tmp = list_entry(pos,struct S_ULsched_QueueEntry,list_h); \		fsm_printf("RNTI:%d,CQI:%d\n",tmp->info.rnti,tmp->info.cqi);} fsm_printf("--------------------\n");}

/*******************************************************************/

