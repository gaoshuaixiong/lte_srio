/******************************************************************************
 ** Filename:dys_dist_mac_main.h - 
 ** Copyright: uestc.
 ** Descriptions:
 ** v1.0, 2010/07/07, maojianbing written
 ** ----------------------------------------------------------
 ** modification history:
 ** Modified by: 
 ** Modified date:
 ** Descriptions:
 ** 
 ******************************************************************************/

#ifndef _DYS_DIST_MAC_MODULE_MAIN_H
#define _DYS_DIST_MAC_MODULE_MAIN_H

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include "ipadpfsm.h"


/* define fsm interface index */

typedef struct ipadp_priv 
{  
    	struct net_device_stats stats;
		ipadp_sv* ipadp_sv_ptr;
} ipadp_priv;


int fsm_dev_create(void);
void fsm_dev_destroy(void);
void fsm_dev_init(struct net_device *dev);
int fsm_dev_open(struct net_device *dev);
int fsm_dev_stop(struct net_device *dev);
int fsm_dev_set_config(struct net_device *dev, struct ifmap *p_ifmap); 
int fsm_dev_recv(struct sk_buff *skb ,struct net_device *dev1, struct packet_type *pktype, struct net_device *dev2);    
int fsm_dev_tx(struct sk_buff *skb, struct net_device *dev);
int fsm_dev_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
struct net_device_stats* fsm_dev_get_stats(struct net_device *dev);
   
void fsm_dev_tx_timeout(struct net_device *p_net_device);    
void fsm_dev_init(struct net_device *dev); 

void write_to_file(char *buf); 
unsigned int get_buffer_len(char *buf);


#endif  //end of the "#ifndef _DYS_DIST_MAC_MODULE_MAIN_H"






