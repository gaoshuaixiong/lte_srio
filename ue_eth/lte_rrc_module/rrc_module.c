/******************************************************************************
 ** Filename:rrc.c - 
 ** Copyright: uestc.
 ** Descriptions: LTE RRC Module
 ** v1.0, 2014/02/24, zhenglingjie written
 ** ----------------------------------------------------------
 ** modification history:
 ** Modified by:summon 
 ** Modified date:2014/02/24
 ** Descriptions:
 ** 
 ******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/netdevice.h>

#include <linux/device.h>
#include <linux/time.h>
#include <net/sock.h>
#include <linux/if_ether.h>

#include <linux/netlink.h>



MODULE_DESCRIPTION("lterrc kernel module");
MODULE_AUTHOR("");
MODULE_LICENSE("GPL");

#include "../fsm/fsmdec.h"
#include "../lte_system.h"
#include "../debug.h"


/*------------------------------------Configure your module here-----------------------------*/
#include "rrcfsm.h"
#include "rrc_module.h"
#include "rrc_utils.h"


#define SV_TYPE_NAME		rrc_sv
#define MY_PROTOCOL			DEV_PROTO_RRC

#ifdef  VIRTUAL_TEST
#define MTU		1500
#else
#define MTU 	65535
#endif

#define RRC_ADDR			{0x00,0x11,0x1A,0x1A,0x8A,0x42}

#define NETIF_F_NO_CSUM   4   //add in 20150805, this difinition not exist in ubuntu 12.04
/*-----------------------------------------------------------------------------------------*/


/*-----------------------------functions declaration------------------------*/
int __fsm_dev_create(void);
void __fsm_dev_destroy(void);
void __fsm_dev_init(struct net_device *dev);
int __fsm_dev_open(struct net_device *dev);
int __fsm_dev_stop(struct net_device *dev);
int __fsm_dev_set_config(struct net_device *dev, struct ifmap *p_ifmap); 
int __fsm_dev_recv(struct sk_buff *skb ,struct net_device *dev1, struct packet_type *pktype, struct net_device *dev2);    
int __fsm_dev_tx(struct sk_buff *skb, struct net_device *dev);
int __fsm_dev_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
struct net_device_stats* __fsm_dev_get_stats(struct net_device *dev);
void __fsm_dev_tx_timeout(struct net_device *p_net_device);    
void __fsm_dev_init(struct net_device *dev); 



/*---------------------------------------------------------------------------*/


/* this is used for register the special packet type that processed by RRC */
struct packet_type pk_t = {
	.type = __constant_htons(MY_PROTOCOL),
	.func = __fsm_dev_recv,
};

static const struct net_device_ops fsm_netdev_ops = {
	.ndo_open = __fsm_dev_open,
	.ndo_stop = __fsm_dev_stop,
	.ndo_start_xmit = __fsm_dev_tx,
	.ndo_get_stats = __fsm_dev_get_stats,
	.ndo_do_ioctl = __fsm_dev_ioctl,
	.ndo_set_config = __fsm_dev_set_config,
	.ndo_tx_timeout = __fsm_dev_tx_timeout,
};

typedef struct fsm_priv 
{  
	struct net_device_stats stats;
	SV_TYPE_NAME* sv_ptr;
} fsm_priv;



/*---------------------------------- netlinlk -----------------------------------------*/
#define NETLINK_TEST 21
//u32 pid;    //process of user space
//static struct sock *netlinkfd = NULL;

void send_to_user(char *info, int size_info){
	// int size;
	// struct sk_buff *skb;
	// unsigned char *old_tail;
	// struct nlmsghdr *nlh; //nl header
	// int retval;

	//     size = NLMSG_SPACE(size_info); //报文大小
	//     skb = alloc_skb(size, GFP_ATOMIC); //分配一个新的套接字缓存,使用GFP_ATOMIC标志进程不>会被置为睡眠
	//     //初始化一个netlink消息首部
	// nlh = nlmsg_put(skb, 0, 0, 0, NLMSG_SPACE(size_info)-sizeof(struct nlmsghdr), 0); 
	//     old_tail = skb->tail;

	//     memcpy(NLMSG_DATA(nlh), info, strlen(info)); //填充数据区
	// nlh->nlmsg_len = skb->tail - old_tail; //设置消息长度

	//     //设置控制字段
	// NETLINK_CB(skb).pid = 0;
	// NETLINK_CB(skb).dst_group = 0;

	//     //fsm_printf("[kernel space] msg sended to user space is skb->data:%s\n", (char *)NLMSG_DATA((struct nlmsghdr *)skb->data));
	//     //发送数据
	// retval = netlink_unicast(netlinkfd, skb, pid, MSG_DONTWAIT);
	//fsm_printf("[kernel space] netlink_unicast return: %d\n", retval);


}

void rcv_msg_from_us(struct sk_buff *__skb){
	// struct sk_buff *skb;
	// struct nlmsghdr *nlh = NULL;

	// char *data = "This is a test message from kernel";

	// skb = skb_get(__skb);
	//     //获取用户程序pid
	//     if(skb->len >= sizeof(struct nlmsghdr)){
	//     nlh = (struct nlmsghdr *)skb->data;
	//     if((nlh->nlmsg_len >= sizeof(struct nlmsghdr)) && (__skb->len >= nlh->nlmsg_len)){
	//         pid = nlh->nlmsg_pid;
	//         fsm_printf("[kernel space] data receive from user are:%s\n", (char *)NLMSG_DATA(nlh));
	//         fsm_printf("[kernel space] user_pid:%d\n", pid);
	//         send_to_user(data, strlen(data)+1);
	//     }
	// }else{
	//     //fsm_printf("[kernel space] data receive from user are:%s\n",(char *)NLMSG_DATA(nlmsg_hdr(__skb)));
	// }
}




/******************************************************************************
 ** Function name:dev_init
 ** Descriptions:initiate the field of struct net_device for register use in lte_rrc_module
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void dev_init(struct net_device *dev)
{
	fsm_priv* priv;
	int fsm_id;
	ASSERT(dev);
	dev->tx_queue_len = 128;   	/* the queue length */   
	dev->mtu = MTU;		/* now we do not fragmentate the packet */
	dev->header_ops = NULL;   	/* cancle eth_header function for not building ethnet mac header */ 
	dev->addr_len = 0;
	dev->hard_header_len = 0;
	dev->netdev_ops = &fsm_netdev_ops;
	dev->watchdog_timeo = 1000;  				/* the time unit is jiffy */
	dev->flags = IFF_BROADCAST | IFF_MULTICAST | IFF_NOARP;
	//dev->features |= NETIF_F_NO_CSUM;

	priv = (fsm_priv*)netdev_priv(dev);
	fsm_mem_set(priv,0,sizeof(fsm_priv));
	fsm_core_create(dev);

	priv->sv_ptr = fsm_mem_alloc(sizeof(SV_TYPE_NAME));
	fsm_mem_set(priv->sv_ptr, 0, sizeof(SV_TYPE_NAME));
	/*-----------------------------------FSM creation---------------------------*/
	fsm_id = fsm_create_register("lte_rrc_fsm", rrc_main, priv->sv_ptr);
	fsm_mainport_bind(fsm_id);
	/*--------------------------------------------------------------------------*/

	return;
}


/******************************************************************************
 ** Function name:__fsm_dev_create
 ** Descriptions:initiate a net_device for lte_rrc_module        
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
int __fsm_dev_create(void)
{   
	//printk(KERN_EMERG"rrc device creating.\n");

	fsm_priv *priv;
	const char  *name = "LTE_RRC%d";
	char dev_addr[6] = RRC_ADDR;
	NETDEV *dev = NULL;
	dev = alloc_netdev(sizeof(fsm_priv), name, dev_init);
	if (dev == NULL)   /* allocation error */
	{
		return -2;
	}

	memcpy(dev->dev_addr, &dev_addr, 6);       
	/* ---------ADD some private data into the net_device struct */
	priv = netdev_priv(dev);
	if (priv == NULL)
	{                
		return -2;                   
	}

	/* register the net device */    
	if (register_netdev(dev))    /* register error */
	{
		free_netdev(dev);   /* release the struct net_device */
		dev = NULL;               
		return -1;
	}

	//initial netlink
	//    netlinkfd = netlink_kernel_create(&init_net, NETLINK_TEST, 0, rcv_msg_from_us, NULL, THIS_MODULE);
	//    if(!netlinkfd){
	//        printk(KERN_ERR "can not create a netlink socket\n");
	//        return -1;
	//    }

	return 0;       
}

/******************************************************************************
 ** Function name:__fsm_dev_destroy
 ** Descriptions:the exit of the module
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void __fsm_dev_destroy(void)
{
	//netlink_kernel_release(netlinkfd);  //release netlink
	//	if(netlinkfd != 0)
	//	{
	//		netlink_kernel_release(netlinkfd);  //release netlink
	//	//	sock_release(netlinkfd->sk_socket);
	//	}

	NETDEV* dev;
	fsm_priv* priv;
	dev = (NETDEV*)fsm_dev_get();
	fsm_core_destroy();
	priv = netdev_priv(dev);
	fsm_mem_free(priv->sv_ptr);
	unregister_netdev(dev);
	free_netdev(dev);
}   

/******************************************************************************
 ** Function name:__fsm_dev_open
 ** Descriptions:called when we use ifconfig up
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
int __fsm_dev_open(struct net_device *dev)
{
	/*-----------------now we need to register the special protocol_type */	
	//mdelay(3000);

	//	fsm_core_intf_set(STRM_TO_IP, DEV_INTF_IND, DEV_PROTO_IP, NULL);	
	fsm_core_intf_set(STRM_TO_PDCP, DEV_INTF_REQ, DEV_PROTO_RRC, "LTE_PDCP0");
	fsm_core_intf_set(STRM_TO_RLCMAC, DEV_INTF_REQ, DEV_PROTO_RRC, "LTE_RLC_MAC0");
	fsm_core_intf_set(STRM_TO_RLCMAC, DEV_INTF_REQ, DEV_PROTO_RRC, "LTE_RLC_MAC0");

	dev_add_pack(&pk_t); 
	netif_start_queue(dev);  
	fsm_core_open();	 
	return 0;    
}


/******************************************************************************
 ** Function name:__fsm_dev_stop
 ** Descriptions:called when we use ifconfig down
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
int __fsm_dev_stop(struct net_device *dev)
{  

	/*-----------------now we need to register the special protocol_type */
	fsm_core_close();
	dev_remove_pack(&pk_t);
	netif_stop_queue(dev);
	return 0;    
}


/******************************************************************************
 ** Function name:__fsm_dev_recv
 ** Descriptions:receive the sk_buff submitted by the other modules
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- ------system--------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
int __fsm_dev_recv(struct sk_buff *skb ,struct net_device *dev1, struct packet_type *pktype, struct net_device *dev2)
{  
	fsm_core_pkt_drive(skb, RXTX_PKT_IND);
	return 0;            
}


/******************************************************************************
 ** Function name:__fsm_dev_set_config
 ** Descriptions:
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
int __fsm_dev_set_config(struct net_device *dev, struct ifmap *p_ifmap)
{
	return 0;
}


/******************************************************************************
 ** Function name:__fsm_dev_tx
 ** Descriptions:transmit sk_buff handed down by the up-layer
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
int __fsm_dev_tx(struct sk_buff *skb, struct net_device *dev)
{ 
	fsm_core_pkt_drive(skb, RXTX_PKT_REQ);	
	return 0;    
}	


/******************************************************************************
 ** Function name:__fsm_dev_ioctl
 ** Descriptions:
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
int __fsm_dev_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	fsm_ioctrl_arrival(ifr,cmd);
	return 0;
}


/******************************************************************************
 ** Function name:net_device_stats
 ** Descriptions:
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
struct net_device_stats* __fsm_dev_get_stats(struct net_device *dev)
{
	fsm_priv* priv;
	priv = (fsm_priv*)netdev_priv(dev);
	return &(priv->stats);            
}

/******************************************************************************
 ** Function name:__fsm_dev_tx_timeout
 ** Descriptions:tx timeout call_back function
 ** Input: 
 ** Output: 
 ** Returns: 
 ** Created by: 
 ** Created Date:2010/07/07 
 **-------------------------------------- --------------------
 ** Modified by:
 ** Modified Date: 
 ******************************************************************************/
void __fsm_dev_tx_timeout(struct net_device *dev)
{  
	return;   
}

module_init(__fsm_dev_create);
module_exit(__fsm_dev_destroy);














