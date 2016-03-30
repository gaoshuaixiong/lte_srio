#include <linux/netdevice.h>
#include <linux/skbuff.h>

#ifndef _DEF_FSM_CORE
#define _DEF_FSM_CORE

#define		RXTX_PKT_REQ			0
#define		RXTX_PKT_IND			1

/* BYTE ORDER */
#define _LITTLE_ENDIAN              0
#define _BIG_ENDIAN                 1
#define BYTE_ORDER                 _LITTLE_ENDIAN

/* Basic definition in kernel.*/
#define		SKBUFF				struct sk_buff					
#define		NETDEV				struct net_device

/* Event Types */
#define FSM_EV_TYPE_PKT_REQ		1	/* Packet request from upper layer.	*/
#define FSM_EV_TYPE_PKT_IND		2	/* Packet received from lower layer.	*/ 
#define FSM_EV_TYPE_SELF		3	/* Self schedule event.			*/
#define FSM_EV_TYPE_EXFSM		4	/* Event schedule for extern FSM (in same module).*/
#define FSM_EV_TYPE_CORE 		5	/* Event from fsm core. */
#define FSM_EV_TYPE_MSG			6	/* Message from other fsm in this module. */


/* Event code from core */
#define FSM_EV_CORE_OPEN		0		
#define FSM_EV_CORE_CLOSE		1
#define FSM_EV_IOCTRL			2

#define INVALIDE			0		


/* Excute Result */	
#define FSM_EXEC_SUCC			0
#define FSM_EXEC_FAIL			-1

/* Interface Types */
#define	DEV_INTF_NULL			0
#define	DEV_INTF_REQ			1
#define	DEV_INTF_IND			2

/** Types Definition **/
#define evHandle 			int*				
#define FSM_PKT				SKBUFF


/** Macro **/
#define KERNEL_MODULE			0	 		
#define USER_SPACE			1

struct kifreq {	
	u32 	cmd;			/* IOCTL command.*/				
	size_t	size;			/* Size of IOCTL data. */ 	
	void* 	buffer;				/* Pointer to the IOCTL data. */
	};

/* Return information of event.	*/
extern u32 fsm_ev_type(void);
extern u32 fsm_ev_code(void);
extern int fsm_ev_src(void);
extern u32 fsm_ev_ioctrl_cmd(void);

/* FSM information get */
extern void * fsm_sv_ptr_get(void);
extern int* fsm_current_state_ptr_get(void);
extern void* fsm_dev_get(void);
extern int fsm_get_id_by_name(const char* name);
extern void fsm_get_name_by_id(int id, char* name);
extern void* fsm_intf_addr_get(int strm_id);
extern u8* fsm_self_addr_get(void);

extern void* fsm_data_get(void);
extern void fsm_data_destroy(void* data_ptr);

/* Packet operation */
extern FSM_PKT* fsm_pkt_get(void);
extern void fsm_pkt_send(FSM_PKT* pkptr, int oinf);
extern FSM_PKT* fsm_pkt_create(unsigned int size);
extern FSM_PKT* fsm_skb_realloc_headeroom(FSM_PKT* pkptr, unsigned int head_len);
extern void fsm_pkt_destroy(FSM_PKT* pkptr);
extern FSM_PKT* fsm_pkt_duplicate(FSM_PKT* pkptr);
extern u8* fsm_skb_put(FSM_PKT* pkptr, int len);
extern u8* fsm_skb_push(FSM_PKT* pkptr, int len);
extern u8* fsm_skb_pull(FSM_PKT* pkptr, int len);
extern void fsm_skb_reserve(FSM_PKT* pkptr, int len);
extern int fsm_skb_tailroom(FSM_PKT* pkptr);
extern int fsm_skb_headroom(FSM_PKT* pkptr);

/* Utilities for operation of memory.*/
extern void* fsm_mem_alloc(size_t size);
extern void fsm_mem_free(void* ptr);
extern void* fsm_mem_cpy(void* dst_ptr, const void* src_ptr, size_t size);
extern void* fsm_mem_set(void* dst_ptr, const int val, size_t size);
extern int   fsm_mem_cmp(const void* dst_ptr, const void* src_ptr, size_t size);

/* Utilities for print.	*/
extern size_t fsm_printf(const char* fmt,...);
extern size_t fsm_octets_print(void* data_ptr, unsigned int num);


/* Endianness */
extern u16 fsm_htons(u16 val);								
extern u32 fsm_htonl(u32 val);
extern u16 fsm_ntohs(u16 val);
extern u32 fsm_ntohl(u32 val);

/* Utilities for schedule self interruption.*/
extern evHandle fsm_schedule_self(unsigned int delay, u32 code);
extern evHandle fsm_schedule_exfsm(unsigned int delay, u32 code, int dst_id);
extern evHandle fsm_post_msg(u32 code,void * msg_ptr, int dst_id, size_t size);
extern int fsm_schedule_cancel(evHandle eh);

/* I/O control */
extern int fsm_do_ioctrl(int strm_id, u32 cmd, void* data_ptr, size_t size);
extern int fsm_ioctrl_arrival(struct ifreq *ifr, int cmd);

/* Time get */
extern u32 fsm_get_curtime(void);

/* Random bytes get */
extern void fsm_get_random_bytes(void *buf, int nbytes); 

/*FSM core control */
extern void fsm_core_create(void *dev);
extern void fsm_core_destroy(void);
extern void fsm_core_intf_set(int index, u32 type, u16 proto, const char* dev_name);
extern int fsm_core_open(void);
extern int fsm_core_close(void);
extern int fsm_core_pkt_drive(FSM_PKT* pkptr, u32 type);

/* FSM operations */
extern int fsm_create_register(const char* name, void (*fsm_main), void* fsm_sv_ptr);
extern void fsm_unregister_destroy(int id);
extern void fsm_mainport_bind(int id);

extern int fsm_param_exchange(int fsm_id, u32 code,void* data_ptr, size_t size);
extern int fsm_func_call(int fsm_id, u32 code, void* param, size_t size, void* retv, size_t retv_size);

#endif
