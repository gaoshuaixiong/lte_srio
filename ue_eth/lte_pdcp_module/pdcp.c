/******************************************************************************
 ** Filename:dys_dist_mac_main.c - 
 ** Copyright: uestc.
 ** Descriptions: dist MAC
 ** v1.0, 2010/07/07, maojianbing written
 ** ----------------------------------------------------------
 ** modification history:
 ** Modified by:summon 
 ** Modified date:2010.0715
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

MODULE_DESCRIPTION("lte pdcp kernel module");
MODULE_AUTHOR("");
MODULE_LICENSE("GPL");

#include "../fsm/fsmdec.h"
#include "../lte_system.h"
#include "../debug.h"


/*------------------------------------Configure your module here-----------------------------*/
#include "pdcpfsm.h"

#define SV_TYPE_NAME		pdcp_sv
#define MY_PROTOCOL		DEV_PROTO_PDCP
#define MTU			1500
#define PDCP_ADDR		{0x00,0x11,0x1A,0x1A,0x8A,0x42}			// Six bytes

#define NETIF_F_NO_CSUM   4   //need to add definition in ubtuntu 12.04, 20150805
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


/* this is used for register the special packet type that processed by DYS */
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





	


/******************************************************************************
 ** Function name:DyS_dist_mac_net_device_stc_init
 ** Descriptions:initiate the field of struct net_device for register use in dys_dist_mac_module
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
    	dev->features |= NETIF_F_NO_CSUM;
    	priv = (fsm_priv*)netdev_priv(dev);
   	fsm_mem_set(priv,0,sizeof(fsm_priv));
	fsm_core_create(dev);
	priv->sv_ptr = fsm_mem_alloc(sizeof(SV_TYPE_NAME));
	fsm_mem_set(priv->sv_ptr, 0, sizeof(SV_TYPE_NAME));
/*-----------------------------------FSM creation---------------------------*/
	fsm_id = fsm_create_register("lte_pdcp_fsm", pdcp_main, priv->sv_ptr);
	fsm_mainport_bind(fsm_id);
/*--------------------------------------------------------------------------*/
    return;
}
                 
                 
/******************************************************************************
 ** Function name:dys_dist_mac_net_device_init
 ** Descriptions:initiate a net_device for dist_mac_layer        
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
   	fsm_priv *priv;
	const char  *name = "LTE_PDCP%d";
	char dev_addr[6] = PDCP_ADDR;
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
 	return 0;       
}

/******************************************************************************
 ** Function name:dys_dist_mac_exit_module
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
 ** Function name:DyS_dist_mac_net_device_open
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
	//fsm_core_intf_set(STRM_TO_IP, DEV_INTF_IND, DEV_PROTO_IP, NULL);
	fsm_core_intf_set(STRM_TO_IPADP, DEV_INTF_IND, DEV_PROTO_IPADP, "LTE_IPADP0");
	fsm_core_intf_set(STRM_TO_RRC, DEV_INTF_IND, DEV_PROTO_RRC, "LTE_RRC0");
	//fsm_core_intf_set(STRM_TO_RLC, DEV_INTF_REQ, DEV_PROTO_PDCP, "LTE_RLC0");
	fsm_core_intf_set(STRM_TO_RLCMAC, DEV_INTF_REQ, DEV_PROTO_PDCP, "LTE_RLC_MAC0"); 	//add in 20150804
    	dev_add_pack(&pk_t); 
	netif_start_queue(dev);   
    	fsm_core_open();	 
    	return 0;    
}


/******************************************************************************
 ** Function name:DyS_dist_mac_net_device_stop
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
 ** Function name:DyS_dist_mac_net_device_recv
 ** Descriptions:receive the sk_buff submitted by the Dist_MAC Module and the Cent_MAC Module
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
 ** Function name:DyS_dist_mac_net_device_set_config
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
 ** Function name:DyS_dist_mac_net_device_tx
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
 ** Function name:DyS_dist_mac_net_device_ioctl
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
 ** Function name:DyS_dist_mac_net_device_get_stats
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
 ** Function name:DyS_dist_mac_net_device_tx_timeout
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














