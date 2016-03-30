#include <linux/hrtimer.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/time.h>
#include <linux/random.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <net/sock.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#include "../fsm/fsmcore.h"

#ifdef		SIOCDEVPRIVATE
#define		IOCKERNEL			SIOCDEVPRIVATE
#define 	IOCUSER				SIOCDEVPRIVATE + 1	
#endif		

		
/* Macro memory allocation and free.*/
#define		FSM_MEM_ALLOC(x)	kmalloc(x, GFP_ATOMIC)			
#define		FSM_MEM_FREE		kfree	
#define		FSM_MEM_CPY			memcpy
#define		FSM_MEM_SET			memset
#define		FSM_MEM_CMP			memcmp


#define MAX_INTF		32
#define	MAX_FSM			6
#define	IFNAMSIZ		16

#define	CORE_CLOSED		0
#define CORE_OPENED		1

#define FOREVER			0xffffffff
#define EVMASK			68130518

/* tm_state value definition */
#define FSM_TM_STOP		0
#define FSM_TM_RUN		1
#define FSM_TM_EXPIRE	2

/** Macros **/
#define INTF			fsm_core.__intf
#define TMBLOCK_PTR     &fsm_core.__tmCtrlBlock
#define FSM_SV_PTR		fsm_core.fsm_sv_ptr
#define	FSM				fsm_core.fsm
#define MAIN_PORT		fsm_core.main_port
#define EVHANDLE(x)		(evHandle)(x)
#define LATE_TIME(t,late)	(ktime_sub(late, t).tv64 > 0)   // true if late > t

/* Macros for kernel print.*/
#define		VPRINTK				vprintk			

/* Macros used for get net device name. */
#define 	DEV_GET_BY_NAME(name)	dev_get_by_name(dev_net(fsm_core.dev), name) 

/* Macros for random bytes.*/
#define		GET_RANDOM_BYTES		get_random_bytes

/* Endianness */																
#define		HTONS				htons							
#define		NTOHS				ntohs
#define		HTONL				htonl
#define		NTOHL				ntohl

/* Macros used in operation of skbuff.*/
#define		ALLOC_SKB			dev_alloc_skb					
#define		KFREE_SKB			kfree_skb
#define		SKB_PUT				skb_put
#define		SKB_PUSH			skb_push
#define		SKB_PULL			skb_pull
#define		SKB_RESERVE			skb_reserve
#define		SKB_TAILROOM		skb_tailroom
#define		SKB_HEADROOM		skb_headroom
#define		SKB_COPY			skb_copy
#define		SKB_REALLOC_HEADROOM 	skb_realloc_headroom
#define		SKB_SET_OWNER_W		 	skb_set_owner_w

/* Macros functions of send and receive.*/ 
#define		NETIF_RX			netif_rx			
#define		DEV_QUEUE_XMIT		dev_queue_xmit

/* type used to manage multiply FSM */

typedef struct{
	char name[128];
	int  id;
	void (*fsm_main)(void);
	void* fsm_sv_ptr;
	int _fsm_current_block;
} FSM_SPEC;


typedef struct{
	int	valid;
	u16	proto;
	NETDEV	*dev;
	void	(*send)(FSM_PKT* pkptr, NETDEV* dev, u16 proto);
} __Intf;

typedef struct __tmEvent{
	struct __tmEvent*	pNext;
	u32			evmask;
	ktime_t		expire;
	unsigned int delay;	
	u32			code;
	u32			type;
	FSM_PKT*	pkptr;
	int			dst_id;
	int			src_id;
	void*		buffer;
} __tmEvent;

typedef struct __tmBlock{
	int					tm_state;
	struct 	hrtimer 	timer;
	struct __tmEvent* 	evQ;
}__tmBlock;

typedef struct {
	int			current_fsm;
	int*			current_state_ptr;
	int			state;
	FSM_PKT*		pkptr;
	int			evtype;
	u32			code;
	int			src;
	void* 			fsm_sv_ptr;
	void			(*fsm_drive)(void);
	__Intf			__intf[MAX_INTF];
	__tmBlock		__tmCtrlBlock;
	int			main_port;    //id of the FSM which is the main port. 
	FSM_SPEC*		fsm[MAX_FSM];
	void*			dev;
	void*			buffer;		 //for ioctrl and message event
	u32			ioctrl_cmd;	 //for ioctrl event
	spinlock_t 		lock;		//lock the core (FSM).
	struct sk_buff_head*	tx_queue;
	struct sk_buff_head*	rx_queue;
	struct tasklet_struct*	tx_tsklt;
	struct tasklet_struct* 	rx_tsklt;
	struct tasklet_struct*  do_ioctl_tsklt;
	struct tasklet_struct*  exp_ev_tsklt;
	spinlock_t		tx_queue_lock;		//lock the pending tx queue
	spinlock_t		rx_queue_lock;		//lock the pending rx queue
	struct list_head	do_ioctl_list;	
	struct list_head        exp_ev_list;
	spinlock_t		ioctl_list_lock;	//lock the pending ioctl event queue
	spinlock_t		expev_list_lock;	//lock the pending expired event queue
	spinlock_t		tmev_list_lock;		//lock the timing event queue
} FSM_CORE;

struct evioctl{
	NETDEV * dev;
	struct kifreq ifr;
	struct list_head list_node;
};

struct tmev{
	__tmEvent* pE;
	struct list_head list_node;
};
	


/* define a static fsm core */
static FSM_CORE fsm_core;

/* ---------------------internal functions declaration---------------------------*/
static void		__fsm_ev_flush(void);
static void		__fsm_skb_send_null(FSM_PKT* pkptr, NETDEV* dev, u16 protocol);
static void		__fsm_skb_pending_netrx(FSM_PKT* pkptr, NETDEV* dev, u16 protocol);
static void		__fsm_skb_pending_xmit(FSM_PKT* pkptr, NETDEV* dev, u16 protocol);
static void 		__fsm_skb_netrx(unsigned long data);
static void 		__fsm_skb_xmit(unsigned long data);
static void		__fsm_core_self_drive(u32 type, u32 code, FSM_PKT* pkptr, int src_id, int dst_id, void* buffer, u32 ioctrl_cmd);
static void		__fsm_tm_init(void);
static void		__fsm_tm_flush(void);
static void		__fsm_tm_stop(void);
static void		__fsm_tm_ev_insert(__tmEvent *pE);
static enum hrtimer_restart __fsm_tm_expire(struct hrtimer *p_timer);
static void		__fsm_tm_restart(void);
static evHandle		__fsm_tm_ev_add(unsigned int delay, u32 type, u32 code, FSM_PKT* pkptr, int src, int dst, void* buff);
static int		__fsm_tm_ev_rmv(__tmEvent* pE);
static void		__fsm_do_ioctl(unsigned long data);

static void 		__fsm_pending_expev(unsigned long data);

/* ---------------------utilities for state machine-----------------------------*/

/* Return information of event.	*/
u32 fsm_ev_type()
{
	return fsm_core.evtype;
}

u32 fsm_ev_code()
{
	return fsm_core.code;
}

int fsm_ev_src()
{
	return fsm_core.src;
}

u32 fsm_ev_ioctrl_cmd()
{
	return fsm_core.ioctrl_cmd;
}

/* FSM information get */
void* fsm_sv_ptr_get()
{
	return fsm_core.fsm_sv_ptr;
}

int* fsm_current_state_ptr_get()
{
	return fsm_core.current_state_ptr;
}

void* fsm_dev_get()
{
	return fsm_core.dev;
}

int fsm_get_id_by_name(const char* name)
{
	int i;
	for(i=0; i< MAX_FSM; ++i)
		{
		if(strcmp(FSM[i]->name, name) == 0)
			{
			return FSM[i]->id;
			}
		}
	return -1;
}

void fsm_get_name_by_id(int id, char* name)
{
	int i;
	for(i=0; i< MAX_FSM; ++i)
		{
		if(FSM[i]->id == id)
			{
			strcpy(name, FSM[i]->name);
			break;
			}
		}
	*name ='\0';
}

void* fsm_intf_addr_get(int strm_id)
{
	if(INTF[strm_id].valid == DEV_INTF_REQ)
		return INTF[strm_id].dev->dev_addr;
	else
		return NULL;
}
	
u8* fsm_self_addr_get()
{
	NETDEV* dev;
	dev = (NETDEV*)fsm_core.dev; 
	return (u8*)dev->dev_addr;
}



void* fsm_data_get()
{
	void* buffer;
	buffer = fsm_core.buffer;
	fsm_core.buffer = NULL;
	return buffer;
}

void fsm_data_destroy(void* data_ptr)
{
	fsm_mem_free(data_ptr);
}


/* Packet operation */

FSM_PKT* fsm_pkt_get()
{
	FSM_PKT* ptr;
	ptr = fsm_core.pkptr;
	fsm_core.pkptr = NULL;
	return ptr;
}

void fsm_pkt_send(FSM_PKT* pkptr, int oinf)
{
	if(oinf < MAX_INTF && INTF[oinf].valid)
	{
		if(INTF[oinf].valid == DEV_INTF_REQ)
			{
			INTF[oinf].send(pkptr, INTF[oinf].dev, INTF[oinf].proto);
			}
		else if(INTF[oinf].valid == DEV_INTF_IND)
			{
			INTF[oinf].send(pkptr, fsm_core.dev, INTF[oinf].proto);
			}
	}
	else
	{
		fsm_pkt_destroy(pkptr);
	}
}

FSM_PKT* fsm_pkt_create(unsigned int size)
{
	FSM_PKT* pkptr;
	pkptr = (FSM_PKT*) ALLOC_SKB(size);
	fsm_mem_set(pkptr->head, 0, pkptr->end - pkptr->head);
	return pkptr;
}


FSM_PKT* fsm_skb_realloc_headeroom(FSM_PKT* pkptr, unsigned int head_len)
{
	FSM_PKT* pkptr_new;
	pkptr_new = SKB_REALLOC_HEADROOM(pkptr, head_len);
	if (unlikely(pkptr_new == NULL))
        {
        KFREE_SKB(pkptr);
        return NULL;
       	}
    if(pkptr->sk)
        {
        SKB_SET_OWNER_W(pkptr_new, pkptr->sk); 
	}
	KFREE_SKB(pkptr);
	return pkptr_new;
}

void fsm_pkt_destroy(FSM_PKT* pkptr)
{
	KFREE_SKB(pkptr);
}

FSM_PKT* fsm_pkt_duplicate(FSM_PKT* pkptr)
{
	return SKB_COPY(pkptr, GFP_ATOMIC);
}

u8* fsm_skb_put(FSM_PKT* pkptr, int len)
{	
	return (u8*)SKB_PUT(pkptr, len);
}

u8* fsm_skb_push(FSM_PKT* pkptr, int len)
{
	return SKB_PUSH(pkptr, len);
}

/*u8* fsm_skb_pull(FSM_PKT* pkptr, int len)
{
	return SKB_PULL(pkptr, len);
}*/
u8* fsm_skb_pull(FSM_PKT* pkptr, int len)
{
	//return SKB_PULL(pkptr, len);
	pkptr->data+=len;
	pkptr->len-=len;
	return pkptr->data;
}
void fsm_skb_reserve(FSM_PKT* pkptr , int len)
{
	SKB_RESERVE(pkptr, len);
}

int fsm_skb_tailroom(FSM_PKT* pkptr)
{
	return SKB_TAILROOM(pkptr);
}

int fsm_skb_headroom(FSM_PKT* pkptr)
{
	return SKB_HEADROOM(pkptr);
}

/* Memory operation */
void* fsm_mem_alloc(size_t size)
{
	void* ptr;
	ptr = FSM_MEM_ALLOC(size);
	return ptr;
}

void fsm_mem_free(void* ptr)
{
	FSM_MEM_FREE(ptr);
}

void* fsm_mem_cpy(void* dst_ptr, const void* src_ptr, size_t size)
{
	return FSM_MEM_CPY(dst_ptr,src_ptr,size);
}

void* fsm_mem_set(void* dst_ptr, const int val, size_t size)
{
	return FSM_MEM_SET(dst_ptr,val,size);
}

int fsm_mem_cmp(const void* dst_ptr, const void* src_ptr, size_t size)
{
	return FSM_MEM_CMP(dst_ptr,src_ptr,size);
}


/* Print */
size_t fsm_printf(const char* fmt,...)
{
	//return;
	int print_len;
	va_list args;
	va_start(args,fmt);
	print_len = VPRINTK(fmt,args);
	va_end(args);
	return print_len;
	return 0;
}

size_t fsm_octets_print(void* data_ptr, size_t num)
{
	u8* ptr;
	int i;
	ptr =(u8*)data_ptr;
	fsm_printf("=====================================================\n");
	for(i = 1; i <= num; ++ i)
	{
		fsm_printf("%x", (*ptr >> 4));
		fsm_printf("%x", (*ptr & 0x0f));
		fsm_printf(" ");
		++ ptr;
		if(i >= 16 && (i % 16 == 0))
		{
			fsm_printf("\n");
		}
	}
	fsm_printf("\n");
	fsm_printf("=====================================================\n");
	return num;
}

/* Endianness */
u16 fsm_htons(u16 val)
{
	return HTONS(val);
}

u32 fsm_htonl(u32 val)
{
	return HTONL(val);
}

u16 fsm_ntohs(u16 val)
{
	return NTOHS(val);
}

u32 fsm_ntohl(u32 val)
{
	return NTOHL(val);
}

/*  Event scheduling operation */
evHandle fsm_schedule_self(unsigned int delay, u32 code)
{
	return 	__fsm_tm_ev_add(delay, FSM_EV_TYPE_SELF, code, NULL, fsm_core.current_fsm, fsm_core.current_fsm, NULL);
}

evHandle fsm_schedule_exfsm(unsigned int delay, u32 code, int dst_id)
{
	return 	__fsm_tm_ev_add(delay, FSM_EV_TYPE_EXFSM, code, NULL, fsm_core.current_fsm, dst_id, NULL);
}

evHandle fsm_post_msg(u32 code , void* msg_ptr, int dst_id, size_t size)
{
	void* buffer;
	buffer = fsm_mem_alloc(size);
	fsm_mem_cpy(buffer, msg_ptr, size);
	return	__fsm_tm_ev_add(0, FSM_EV_TYPE_MSG, code, NULL, fsm_core.current_fsm, dst_id, buffer);
}

int fsm_schedule_cancel(evHandle eh)
{
	return __fsm_tm_ev_rmv((__tmEvent*)eh);
}

/* I/O control */
/*int fsm_ioctrl(u32 STRM, unsigned char cmd, void* buffer, unsigned short size)
{
	NETDEV*	 dev;
	struct kifreq*  ifr_ptr;
	int result;
	dev = fsm_core.__intf[STRM].dev;
	ifr_ptr = (struct kifreq *)fsm_mem_alloc(sizeof(struct kifreq));
	ifr_ptr->cmd = cmd;
	ifr_ptr->size = size;
	ifr_ptr->buffer = buffer;
	result = dev->netdev_ops->ndo_do_ioctl(dev, (struct ifreq*)ifr_ptr, IOCKERNEL);
	fsm_mem_free(ifr_ptr);
	return result;
}*/

int fsm_do_ioctrl(int strm_id, u32 cmd, void* data_ptr, size_t size)
{
	NETDEV*	 dev;
	struct evioctl* ev_ptr;
	dev = fsm_core.__intf[strm_id].dev;
	ev_ptr = (struct evioctl*)fsm_mem_alloc(sizeof(struct evioctl));
	ev_ptr->dev =dev;
	ev_ptr->ifr.cmd = cmd;
	ev_ptr->ifr.size = size;
	if(data_ptr != NULL && size > 0)
		{
		ev_ptr->ifr.buffer = fsm_mem_alloc(size);
		fsm_mem_cpy(ev_ptr->ifr.buffer, data_ptr, size);
		}
	else
		{
		ev_ptr->ifr.buffer = NULL;
		}
	spin_lock(&fsm_core.ioctl_list_lock);
	list_add_tail(&ev_ptr->list_node, &fsm_core.do_ioctl_list);
	spin_unlock(&fsm_core.ioctl_list_lock);
	tasklet_schedule(fsm_core.do_ioctl_tsklt);
	return 1;
}


int fsm_ioctrl_arrival(struct ifreq *ifr, int cmd)
{ 
	struct kifreq* kifr_ptr;	
	struct ifreq* uifr_ptr; 
	struct kifreq kifr;
	void* buffer = NULL;
	unsigned long flags2;
	if(cmd == IOCKERNEL)	
		{
//		fsm_printf("fsm_ioctl_arrival\n");
//		return 1;		
		kifr_ptr = (struct kifreq*)ifr;
//		if(kifr_ptr->buffer)
//			{
//			buffer = fsm_mem_alloc(kifr_ptr->size);
//			fsm_mem_cpy(buffer, kifr_ptr->buffer, kifr_ptr->size);
//			}
		spin_lock_irqsave(&fsm_core.lock, flags2);
		__fsm_core_self_drive(FSM_EV_TYPE_CORE,FSM_EV_IOCTRL,NULL, KERNEL_MODULE,MAIN_PORT,kifr_ptr->buffer,kifr_ptr->cmd);
		spin_unlock_irqrestore(&fsm_core.lock, flags2);
		}	
	else if(cmd == IOCUSER)	
		{		
		uifr_ptr = ifr;		
		copy_from_user(&kifr, uifr_ptr->ifr_ifru.ifru_data, sizeof(struct kifreq)); 	
		kifr_ptr = &kifr;		
		if(kifr_ptr->buffer)		
			{			
			buffer = fsm_mem_alloc(kifr_ptr->size); 		
			copy_from_user(buffer, kifr_ptr->buffer, kifr_ptr->size);	
			spin_lock_irqsave(&fsm_core.lock, flags2);
			__fsm_core_self_drive(FSM_EV_TYPE_CORE,FSM_EV_IOCTRL,NULL, USER_SPACE, MAIN_PORT,buffer,kifr_ptr->cmd);
			spin_unlock_irqrestore(&fsm_core.lock, flags2);
			copy_to_user(kifr_ptr->buffer, buffer, kifr_ptr->size); 		
//			fsm_mem_free(buffer);		
			}		
		else		
			{
			spin_lock_irqsave(&fsm_core.lock, flags2);			
			__fsm_core_self_drive(FSM_EV_TYPE_CORE,FSM_EV_IOCTRL,NULL, USER_SPACE, MAIN_PORT,NULL,kifr_ptr->cmd);
			spin_unlock_irqrestore(&fsm_core.lock, flags2);		
			}		
		copy_to_user(uifr_ptr->ifr_ifru.ifru_data, kifr_ptr, sizeof(struct kifreq));
		}
	
	return 1;
}



/* Time get */
u32 fsm_get_curtime()
{
	ktime_t	ktime;
	struct timeval cur_time;
	u32 ctm;
	ktime = ktime_get();
	cur_time = ktime_to_timeval(ktime);
	ctm = cur_time.tv_sec * 1000000 + cur_time.tv_usec;
	ctm = ctm/10;
	return ctm;
}


/* Random bytes get*/
void fsm_get_random_bytes(void *buf, int nbytes)
{
	GET_RANDOM_BYTES(buf, nbytes);
}



/* FSM core operations */

void fsm_core_create(void* dev)
{
	int i;
	fsm_core.current_fsm = -1;
	fsm_core.current_state_ptr = NULL;
	fsm_core.state = CORE_CLOSED;
	fsm_core.pkptr = 0;
	fsm_core.evtype = 0;
	fsm_core.code = 0;
	fsm_core.src = 0;
	fsm_core.dev = dev;
	FSM_SV_PTR = NULL;
	fsm_core.fsm_drive = NULL;
	MAIN_PORT = -1;
	for(i = 0; i < MAX_INTF; i++)
	{
		INTF[i].valid = 0;
		INTF[i].proto = 0;
		INTF[i].dev	= 0;
		INTF[i].send = __fsm_skb_send_null;
	}
	for(i = 0; i < MAX_FSM; ++i)
	{
		FSM[i] = NULL;
	}
	fsm_core.buffer = NULL;
	fsm_core.ioctrl_cmd = 0;
	spin_lock_init(&fsm_core.lock);
	fsm_core.tx_queue = (struct sk_buff_head*)fsm_mem_alloc(sizeof(struct sk_buff_head));
	fsm_core.rx_queue = (struct sk_buff_head*)fsm_mem_alloc(sizeof(struct sk_buff_head));
	fsm_core.tx_tsklt = (struct tasklet_struct*)fsm_mem_alloc(sizeof(struct tasklet_struct));
	fsm_core.rx_tsklt = (struct tasklet_struct*)fsm_mem_alloc(sizeof(struct tasklet_struct));
	fsm_core.do_ioctl_tsklt = (struct tasklet_struct*)fsm_mem_alloc(sizeof(struct tasklet_struct));
	fsm_core.exp_ev_tsklt = (struct tasklet_struct*)fsm_mem_alloc(sizeof(struct tasklet_struct));
	return; 
}

		
void fsm_core_destroy()
{
	int i;
	if(fsm_core.state != CORE_CLOSED)
		fsm_core_close();
	for(i = 0; i < MAX_FSM; i++)
	{
		fsm_unregister_destroy(i);
	}
	fsm_mem_free(fsm_core.tx_queue);
	fsm_mem_free(fsm_core.rx_queue);
	fsm_mem_free(fsm_core.tx_tsklt);
	fsm_mem_free(fsm_core.rx_tsklt);
	fsm_mem_free(fsm_core.do_ioctl_tsklt);
	fsm_mem_free(fsm_core.exp_ev_tsklt);
	return;
}

//proto is the protocol of upperlayer of the data stream 
//dev name is the destination of the data stream
void fsm_core_intf_set(int index, u32 type, u16 proto, const char* dev_name)
{
	NETDEV *dev = NULL;
	switch(type)
	{
	case DEV_INTF_NULL:
		INTF[index].valid = DEV_INTF_NULL;
		INTF[index].send = __fsm_skb_send_null;
		break;
	case DEV_INTF_IND:
		if(dev_name)
			dev = DEV_GET_BY_NAME(dev_name);
		INTF[index].valid = DEV_INTF_IND;
		INTF[index].proto = htons(proto);
		INTF[index].dev = dev;
		INTF[index].send = __fsm_skb_pending_netrx;
		break;
	case DEV_INTF_REQ:
		if(dev_name)
			dev = DEV_GET_BY_NAME(dev_name);
		INTF[index].valid = DEV_INTF_REQ;
		INTF[index].proto = htons(proto);
		INTF[index].dev = dev;
		INTF[index].send = __fsm_skb_pending_xmit;
		break;
	}
	if(proto == 0 && dev_name == 0)
		{
		INTF[index].valid = DEV_INTF_NULL;
		INTF[index].send = __fsm_skb_send_null;
		}
	return;
}


int fsm_core_open()
{
	unsigned long flags;
	if(fsm_core.state == CORE_CLOSED)
	{
	fsm_printf("core is opening.\n");
	spin_lock_irqsave(&fsm_core.lock, flags);
	//Initialize queues
	skb_queue_head_init(fsm_core.tx_queue);
	skb_queue_head_init(fsm_core.rx_queue);
	spin_lock_init(&fsm_core.tx_queue_lock);
	spin_lock_init(&fsm_core.rx_queue_lock);
	//Initialize tasklets
	tasklet_init(fsm_core.tx_tsklt, __fsm_skb_xmit, 0);
	tasklet_init(fsm_core.rx_tsklt, __fsm_skb_netrx, 0);
	spin_lock_init(&fsm_core.ioctl_list_lock);
	INIT_LIST_HEAD(&fsm_core.do_ioctl_list);
	tasklet_init(fsm_core.do_ioctl_tsklt, __fsm_do_ioctl, 0);
	spin_lock_init(&fsm_core.expev_list_lock);
	INIT_LIST_HEAD(&fsm_core.exp_ev_list);
	tasklet_init(fsm_core.exp_ev_tsklt, __fsm_pending_expev, 0);
	spin_lock_init(&fsm_core.tmev_list_lock);
	__fsm_tm_init();
	fsm_core.state = CORE_OPENED;
	__fsm_core_self_drive(FSM_EV_TYPE_CORE,FSM_EV_CORE_OPEN,NULL,KERNEL_MODULE,MAIN_PORT,NULL,INVALIDE);
	spin_unlock_irqrestore(&fsm_core.lock, flags);
	fsm_printf("core has opened.\n");
	}
	return FSM_EXEC_SUCC;
}

int fsm_core_close()
{
	struct evioctl *ev_ptr, *tmp_ptr;
	struct tmev *tmev_ptr, *tmev_tmp_ptr;
	int i;
	unsigned long flags, flags2;
	if(fsm_core.state == CORE_CLOSED)
		return FSM_EXEC_SUCC;
	spin_lock_irqsave(&fsm_core.lock, flags2);
	fsm_printf("core is closing.\n");
	__fsm_core_self_drive(FSM_EV_TYPE_CORE,FSM_EV_CORE_CLOSE,NULL,KERNEL_MODULE,MAIN_PORT,NULL,INVALIDE);
	__fsm_tm_stop();
	__fsm_tm_flush();
	//Cancel tasklets
	tasklet_kill(fsm_core.tx_tsklt);
	tasklet_kill(fsm_core.rx_tsklt);
	tasklet_kill(fsm_core.do_ioctl_tsklt);
	tasklet_kill(fsm_core.exp_ev_tsklt);	
	//Flush queues;
	while(!skb_queue_empty(fsm_core.tx_queue))
	{
	fsm_pkt_destroy(skb_dequeue(fsm_core.tx_queue));
	}
	while(!skb_queue_empty(fsm_core.rx_queue))
	{
	fsm_pkt_destroy(skb_dequeue(fsm_core.rx_queue));
	}
	if(!list_empty(&fsm_core.do_ioctl_list))
	{
		list_for_each_entry_safe(ev_ptr, tmp_ptr, &fsm_core.do_ioctl_list, list_node)
		{
			spin_lock_irqsave(&fsm_core.ioctl_list_lock, flags);
			list_del(&ev_ptr->list_node);
			spin_unlock_irqrestore(&fsm_core.ioctl_list_lock, flags);
			fsm_mem_free(ev_ptr->ifr.buffer);
			fsm_mem_free(ev_ptr);
			if(list_empty(&fsm_core.do_ioctl_list))
				break;
		}
	}
	if(!list_empty(&fsm_core.exp_ev_list))
	{
		list_for_each_entry_safe(tmev_ptr, tmev_tmp_ptr, &fsm_core.exp_ev_list, list_node)
		{
			spin_lock_irqsave(&fsm_core.expev_list_lock, flags);
			list_del(&tmev_ptr->list_node);
			spin_unlock_irqrestore(&fsm_core.expev_list_lock, flags);
			fsm_mem_free(tmev_ptr->pE);
			fsm_mem_free(tmev_ptr);
			if(list_empty(&fsm_core.exp_ev_list))
				break;
		}
	}	
	fsm_core.state = CORE_CLOSED;
	for(i = 0; i < MAX_INTF; i++)
	{
		if(INTF[i].dev)
		{
			dev_put(INTF[i].dev);
		}	
	}
	for(i = 0; i < MAX_INTF; i++)
	{
		INTF[i].valid = 0;
		INTF[i].proto = 0;
		INTF[i].dev	= 0;
		INTF[i].send = __fsm_skb_send_null;
	}
	fsm_printf("core has closed.\n");
	spin_unlock_irqrestore(&fsm_core.lock , flags2);
	return FSM_EXEC_SUCC;
}



int fsm_core_pkt_drive(FSM_PKT* pkptr, u32 type)
{ 
	unsigned long flags2;
	if(type == RXTX_PKT_IND)
	{
		spin_lock_irqsave(&fsm_core.lock, flags2);
		__fsm_core_self_drive(FSM_EV_TYPE_PKT_IND,INVALIDE,pkptr,KERNEL_MODULE,MAIN_PORT,NULL,INVALIDE);
		spin_unlock_irqrestore(&fsm_core.lock ,flags2);
	}
	if(type == RXTX_PKT_REQ)
	{
/*		if(in_irq())
		{
		fsm_printf("in irq\n");
		}
		if(irqs_disabled())
		{
		fsm_printf("irq is disabled!\n");
		}
		fsm_printf("send packet\n");*/
		spin_lock_irqsave(&fsm_core.lock, flags2);
		__fsm_core_self_drive(FSM_EV_TYPE_PKT_REQ,INVALIDE,pkptr,KERNEL_MODULE,MAIN_PORT,NULL,INVALIDE);
		spin_unlock_irqrestore(&fsm_core.lock , flags2);

	}
	return FSM_EXEC_SUCC;
}

/* FSM operations */
int fsm_create_register(const char* name, void (*fsm_main), void* fsm_sv_ptr)
{
	int i;
	for(i=0; i < MAX_FSM; ++i)
		{
		if(FSM[i] == NULL)
			{
			FSM[i] = (FSM_SPEC*)fsm_mem_alloc(sizeof(FSM_SPEC));
			FSM[i]->fsm_main = fsm_main;
			FSM[i]->id = i;
			strcpy(FSM[i]->name, name);
			FSM[i]->fsm_sv_ptr = fsm_sv_ptr;
			FSM[i]->_fsm_current_block = 0;
			return i;
			}
		}
	return -1;
}

void fsm_unregister_destroy(int id)
{
	if(FSM[id]!=NULL)
	{
		fsm_mem_free(FSM[id]);
		FSM[id] = NULL;
	}
}

void fsm_mainport_bind(int id)
{
	fsm_core.main_port = id;
}


/*---------------------------------------------------------------------------------------------*/
/*
int fsm_param_exchange(unsigned int fsm_id, unsigned char code,void* buffer,u16 size)
{
	FSM_INFO* fsm_info_ptr;
	void*  param_ptr;
	fsm_info_ptr = __fsm_info_get_by_id(fsm_id);
	fsm_data.fsm_sv_ptr = fsm_info_ptr->fsm_sv_ptr;
	if(fsm_info_ptr->fsm_exparam == NULL)
		{
		return -1;
		}
	param_ptr = fsm_info_ptr->fsm_exparam(code,NULL,buffer,size);
	fsm_info_ptr = __fsm_info_get_by_id(fsm_data.my_id);
	fsm_data.fsm_sv_ptr = fsm_info_ptr->fsm_sv_ptr;
	return 1;
}

int fsm_func_call(unsigned int fsm_id, unsigned char code, void* param, u16 size, void* retv, u32 retv_size)
{
	FSM_INFO* fsm_info_ptr;
	fsm_info_ptr = __fsm_info_get_by_id(fsm_id);
	fsm_data.fsm_sv_ptr = fsm_info_ptr->fsm_sv_ptr;
	if(fsm_info_ptr->fsm_exparam == NULL)
		{
		return -1;
		}
	fsm_info_ptr->fsm_exfunc(code,param,size,retv,retv_size);
	fsm_info_ptr = __fsm_info_get_by_id(fsm_data.my_id);
	fsm_data.fsm_sv_ptr = fsm_info_ptr->fsm_sv_ptr;
	return 1;
}

*/








/* ------------------internal functions-------------------------------*/
static void __fsm_ev_flush()
{
	
	if(fsm_core.pkptr)
		{
		fsm_pkt_destroy(fsm_core.pkptr);
		}
//	if(fsm_core.buffer && !(fsm_ev_type()==FSM_EV_TYPE_CORE && fsm_ev_code()==FSM_EV_IOCTRL && fsm_ev_src()==USER_SPACE))
//		{
//		fsm_mem_free(fsm_core.buffer);
//		}
	if(fsm_core.buffer)
		{
		fsm_data_destroy(fsm_core.buffer);
		}
	fsm_core.current_fsm = -1;
	fsm_core.current_state_ptr = NULL;
	fsm_core.pkptr = NULL;
	fsm_core.evtype = INVALIDE;
	fsm_core.code = INVALIDE;
	fsm_core.src = INVALIDE;
	FSM_SV_PTR = NULL;
	fsm_core.fsm_drive = NULL;
	fsm_core.buffer = NULL;
	fsm_core.ioctrl_cmd = INVALIDE;
	return;
}

static void __fsm_skb_pending_netrx(FSM_PKT* pkptr, NETDEV* dev, u16 protocol)
{
	SKBUFF* skb;
	unsigned long flags;
	skb = (SKBUFF*)pkptr;
	skb->protocol = protocol;
	skb->dev = dev;
	if(skb->protocol == htons(ETH_P_IP))
	{
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	}
	spin_lock_irqsave(&fsm_core.rx_queue_lock, flags);
	skb_queue_tail(fsm_core.rx_queue, pkptr);
	fsm_printf("[PDCP]__fsm_skb_pending_netrx:\n");
	fsm_octets_print(pkptr->data, 84);
	spin_unlock_irqrestore(&fsm_core.rx_queue_lock, flags);
	tasklet_schedule(fsm_core.rx_tsklt);
//	fsm_pkt_destroy(pkptr);
	return;
}

static void __fsm_skb_pending_xmit(FSM_PKT* pkptr, NETDEV* dev, u16 protocol)
{
	SKBUFF* skb;
	unsigned long flags;
	skb = (SKBUFF*)pkptr;
	skb->dev = dev;
	skb->protocol = protocol;
	spin_lock_irqsave(&fsm_core.tx_queue_lock, flags);
	skb_queue_tail(fsm_core.tx_queue, pkptr);
	spin_unlock_irqrestore(&fsm_core.tx_queue_lock, flags);
	tasklet_schedule(fsm_core.tx_tsklt);
//	fsm_pkt_destroy(pkptr);
	return;
}

static void __fsm_skb_send_null(FSM_PKT* pkptr, NETDEV* dev, u16 protocol)
{
	fsm_pkt_destroy(pkptr);
}

static void __fsm_skb_netrx(unsigned long data)
{
	FSM_PKT* pkptr;
	unsigned long flags;


	while(1)
	{
	spin_lock_irqsave(&fsm_core.rx_queue_lock, flags);
	if(skb_queue_empty(fsm_core.rx_queue))
		{
		
		spin_unlock_irqrestore(&fsm_core.rx_queue_lock, flags);
		break;
		}
	else
		{
		pkptr = skb_dequeue(fsm_core.rx_queue);
		spin_unlock_irqrestore(&fsm_core.rx_queue_lock, flags);
		NETIF_RX(pkptr);
		}
	}
//	fsm_printf("__fsm_skb_netrx() packet receive\n");
	return;
}

static void __fsm_skb_xmit(unsigned long data)
{
	FSM_PKT* pkptr;
	unsigned long flags;
	while(1)
	{
	spin_lock_irqsave(&fsm_core.tx_queue_lock, flags);
	if(skb_queue_empty(fsm_core.tx_queue))
		{
		spin_unlock_irqrestore(&fsm_core.tx_queue_lock, flags);
		break;
		}
	else
		{
		pkptr = skb_dequeue(fsm_core.tx_queue);
		spin_unlock_irqrestore(&fsm_core.tx_queue_lock, flags);
//		fsm_octets_print(pkptr->data, pkptr->len);
		DEV_QUEUE_XMIT(pkptr);
		}
	}
//	fsm_printf("__fsm_skb_xmit() packet send\n");
	return;
}

static void __fsm_pending_expev(unsigned long data)
{
	struct tmev* tmev_ptr;
	__tmEvent* pE;
	unsigned long flags, flags2;
	while(!list_empty(&fsm_core.exp_ev_list))
	{
		tmev_ptr = list_first_entry(&fsm_core.exp_ev_list, struct tmev, list_node);
		pE = tmev_ptr->pE;
		if(spin_is_locked(&fsm_core.lock))
		{
			fsm_printf("dead lock-_fsm_pending_expev fsm_core.lock\n");
		//	return;
		}
		spin_lock_irqsave(&fsm_core.lock, flags2);
		__fsm_core_self_drive(pE->type,pE->code,pE->pkptr,pE->src_id,pE->dst_id,pE->buffer,INVALIDE); 
		spin_unlock_irqrestore(&fsm_core.lock, flags2);
		if(spin_is_locked(&fsm_core.expev_list_lock))
		{
			fsm_printf("dead lock-_fsm_pending_expev fsm_core.expev_list_lock\n");
		//	return;
		} 
		spin_lock_irqsave(&fsm_core.expev_list_lock, flags);
		list_del(&tmev_ptr->list_node);
		spin_unlock_irqrestore(&fsm_core.expev_list_lock, flags);
		fsm_mem_free(pE);
		fsm_mem_free(tmev_ptr);
	}
}

static void __fsm_core_self_drive(u32 evtype, u32 code, FSM_PKT* pkptr, int src_id, int dst_id, void* buffer, u32 ioctrl_cmd)		
{
	fsm_core.current_fsm = dst_id;
	fsm_core.current_state_ptr = &FSM[dst_id]->_fsm_current_block;
	fsm_core.evtype = evtype;
	fsm_core.code = code;
	fsm_core.pkptr = pkptr;
	fsm_core.src = src_id;
	FSM_SV_PTR = FSM[dst_id]->fsm_sv_ptr;
	fsm_core.buffer = buffer;
	fsm_core.ioctrl_cmd = ioctrl_cmd;
	fsm_core.fsm_drive = FSM[dst_id]->fsm_main;
	fsm_core.fsm_drive();
	__fsm_ev_flush();
}

static void __fsm_tm_init()
{
	__tmBlock* pB;
	pB = TMBLOCK_PTR;
	pB->tm_state = FSM_TM_STOP;
	pB->evQ = 0;
	hrtimer_init(&pB->timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS); 
}
static void __fsm_tm_stop()
{
	__tmBlock* pB;
	pB = TMBLOCK_PTR;
	pB->tm_state = FSM_TM_STOP;
	hrtimer_try_to_cancel(&pB->timer);
}

static void __fsm_tm_flush()
{
	__tmEvent* pE;
	__tmBlock* pB;
	pB = TMBLOCK_PTR;
	unsigned long flags;
	spin_lock_irqsave(&fsm_core.tmev_list_lock, flags);
	while(pB->evQ)
	{
		pE = pB->evQ;
		pB->evQ = pE->pNext;
		fsm_mem_free(pE);
	}
	spin_unlock_irqrestore(&fsm_core.tmev_list_lock, flags);
}

static enum hrtimer_restart __fsm_tm_expire(struct hrtimer *timer)
{
	ktime_t cur_time; 
	__tmEvent* pE;
	__tmBlock* pB;
	struct tmev* tmev_ptr;
	unsigned long flags;
//	fsm_printf("fsm_tm_expire\n");

	pB = TMBLOCK_PTR;
	cur_time = ktime_get();
	pB->tm_state = FSM_TM_EXPIRE;
	if(spin_is_locked(&fsm_core.tmev_list_lock))	
	{
		fsm_printf("dead lock\n");
		return HRTIMER_NORESTART;
	}	
	/* No matter whether core is busy or idle, the expired timing event is pended to prevent the possible nested interruption. */
//	if(spin_is_locked(&fsm_core.lock))
//	{
		spin_lock_irqsave(&fsm_core.tmev_list_lock, flags);
		while(pB->evQ && !LATE_TIME(cur_time, pB->evQ->expire))
		{
			pE = pB->evQ;
			pB->evQ = pE->pNext;
			tmev_ptr = (struct tmev*)fsm_mem_alloc(sizeof(struct tmev));
			tmev_ptr->pE = pE;
			list_add_tail(&tmev_ptr->list_node, &fsm_core.exp_ev_list);
		}
		spin_unlock_irqrestore(&fsm_core.tmev_list_lock, flags);		
		tasklet_schedule(fsm_core.exp_ev_tsklt);
//	}
	/* Core is idle. */
/*	else
	{
		
		while(pB->evQ && !LATE_TIME(cur_time, pB->evQ->expire))
		{
			pE = pB->evQ;
			pB->evQ = pE->pNext;
			spin_lock_bh(&fsm_core.lock);
			__fsm_core_self_drive(pE->type,pE->code,pE->pkptr,pE->src_id,pE->dst_id,pE->buffer,INVALIDE);  
//		fsm_printf("fsm core completes self drive.\n"); 
			fsm_mem_free(pE);
			spin_lock_bh(&fsm_core.lock);
		}
//	fsm_printf("ready to unlock\n");
		spin_unlock_bh(&fsm_core.lock);
//	fsm_printf("unlock_bh\n");
	}
*/
	if(pB->evQ)
        {	
		hrtimer_set_expires(&pB->timer, pB->evQ->expire);
		pB->tm_state = FSM_TM_RUN;	
		return HRTIMER_RESTART;
	}
	else
	{
		pB->tm_state = FSM_TM_STOP;
		return HRTIMER_NORESTART;	
	}
	
}

static void __fsm_tm_ev_insert(__tmEvent *pE)		// insert pE in evQ in increasing order for .->expire
{
	__tmEvent* p0;
	__tmEvent* p1;
	__tmBlock* pB;
	unsigned long flags;
	pB = TMBLOCK_PTR;
	spin_lock_irqsave(&fsm_core.tmev_list_lock, flags);
	if(!pB->evQ || LATE_TIME(pE->expire,pB->evQ->expire))
	{
		pE->pNext = pB->evQ;
		pB->evQ = pE;
		if(pB->tm_state != FSM_TM_EXPIRE)
		{
			__fsm_tm_restart();
		}
		spin_unlock_irqrestore(&fsm_core.tmev_list_lock, flags);
		return;
	}
	p0 = pB->evQ;
	while(p0 && (LATE_TIME(p0->expire,pE->expire)||ktime_equal(p0->expire, pE->expire)))
	{
		p1 = p0;
		p0 = p0->pNext;
	}
	pE->pNext = p0;
	p1->pNext = pE;
	spin_unlock_irqrestore(&fsm_core.tmev_list_lock, flags);	
}

static evHandle __fsm_tm_ev_add(unsigned int delay, u32 type, u32 code, FSM_PKT* pkptr, int src, int dst, void* buffer)		// delay should be in 10us base
{
	__tmEvent* pE;
	pE = (__tmEvent*)fsm_mem_alloc(sizeof(__tmEvent));
	if(pE)
	{
		pE->evmask = EVMASK;
		pE->pNext = 0;
		pE->type = type;
		pE->code = code;
		pE->delay = delay;
		pE->pkptr = pkptr;
		pE->src_id =src;
		pE->dst_id = dst;
		pE->buffer = buffer;
		if(delay == 0)
			pE->expire = ktime_add_us(ktime_get(), 10);
		else
			pE->expire = ktime_add_us(ktime_get(), 10 * delay);
		__fsm_tm_ev_insert(pE);
		return EVHANDLE(pE);
	}
	return 0;
}

static int __fsm_tm_ev_rmv(__tmEvent* pE)
{
	__tmEvent* p0;
	__tmBlock* pB;
	pB = TMBLOCK_PTR;
	unsigned long flags;
	if(pE->evmask != EVMASK)
		return FSM_EXEC_FAIL;
	if(!pB->evQ)
		return FSM_EXEC_FAIL;
	spin_lock_irqsave(&fsm_core.tmev_list_lock, flags);
	if(EVHANDLE(pE) == EVHANDLE(pB->evQ))
	{
		pB->evQ = pB->evQ->pNext;
		if(pB->tm_state != FSM_TM_EXPIRE)
		{
			__fsm_tm_restart();
		}
		fsm_mem_free(pE);
		if(!pB->evQ)
		{
			__fsm_tm_stop();
		}
		spin_unlock_irqrestore(&fsm_core.tmev_list_lock, flags);
		return FSM_EXEC_SUCC;
	}
	p0= pB->evQ;
	while(p0->pNext && (EVHANDLE(pE) != EVHANDLE(p0->pNext)))
		p0 = p0->pNext;
	if(p0->pNext)
	{
		p0->pNext = pE->pNext;
		fsm_mem_free(pE);
		spin_unlock_irqrestore(&fsm_core.tmev_list_lock, flags);
		return FSM_EXEC_SUCC;
	}
	spin_unlock_irqrestore(&fsm_core.tmev_list_lock, flags);
	return FSM_EXEC_FAIL;
}

static void __fsm_tm_restart()
{
	__tmBlock* pB;
	pB = TMBLOCK_PTR;
	if(!(pB->evQ))
	{
		pB->tm_state = FSM_TM_STOP;
		return;
	}
	if(pB->tm_state == FSM_TM_STOP)
	{	
		pB->timer.function = __fsm_tm_expire;
		hrtimer_start(&pB->timer, pB->evQ->expire, HRTIMER_MODE_ABS);
		pB->tm_state = FSM_TM_RUN;
	}
	else if(pB->tm_state == FSM_TM_RUN)
	{	
		hrtimer_cancel(&pB->timer);
		pB->timer.function = __fsm_tm_expire;
		hrtimer_start(&pB->timer, pB->evQ->expire, HRTIMER_MODE_ABS);
	}
}


static void __fsm_do_ioctl(unsigned long data)
{
	struct evioctl* ev_ptr;
	unsigned long flags;
	while(!list_empty(&fsm_core.do_ioctl_list))
	{
		ev_ptr = list_first_entry(&fsm_core.do_ioctl_list, struct evioctl, list_node);
		ev_ptr->dev->netdev_ops->ndo_do_ioctl(ev_ptr->dev, (struct ifreq*)&ev_ptr->ifr, IOCKERNEL);
		spin_lock_irqsave(&fsm_core.ioctl_list_lock, flags);
		list_del(&ev_ptr->list_node);
		spin_unlock_irqrestore(&fsm_core.ioctl_list_lock, flags);
		//fsm_mem_free(ev_ptr->ifr.buffer);
		fsm_mem_free(ev_ptr);
	}
}


