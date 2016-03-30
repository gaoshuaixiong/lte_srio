#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "./uemac_ver1.h" 
#define PRINTRLC	0x0a
//./uemac -rareq 4 LTE_MAC0  //第3个参数是rapid 例如4 
//uemac -ulschedreq LTE_MAC0
//uemac -dlschedreq LTE_MAC0
//uemac -reset LTE_MAC0
//uemac -test LTE_MAC0
//./uemac -rlc_send_req LTE_RLC0 //让RLC发送datareq给MAC层 
//./uemac -rrc_lc_config 2 1 LTE_MAC0 //配置逻辑信道 


/************分割线***************/
//./uemac -rar_failed i LTE_MAC0,i表示前i次失败，第i+1次成功，i<MAX_TIMES
//./uemac -contention_failed i LTE_MAC0,i表示前i次失败，第i+1次成功，i<MAX_TIMES


// ./uemac -pkt_num LTE_MAC0
int main(int argc, char *argv[])
{
	int sock;
	struct ifreq ifr;
	struct kifreq kifr;
	float interval;
	sock = socket(AF_INET, SOCK_DGRAM, 0); //创建一个套接口，类型是SOCK_DGRAM(TCP), 协议类型是0即IP
	kifr.data_ptr = NULL;
	if(argc > 2)
	{	
		if(strcmp(argv[1], "-reset") == 0)
		{	
			if(argc == 3)
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				send_reset(&kifr);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}
		}
		else if(strcmp(argv[1], "-pkt_num") == 0)
		{
			if(argc == 3)
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				get_mac_pkt_num(&kifr);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}
		}
		else if(strcmp(argv[1], "-p") == 0)
		{
			if(argc == 3)
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				get_pkt_num(&kifr);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}
		}
		else if(strcmp(argv[1], "-rar_failed") == 0)
		{
			if(argc == 4) 
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[3]);
				rar_failed(&kifr,argv[2]);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}
		}
		else if(strcmp(argv[1], "-contention_failed") == 0)
		{
			if(argc == 4) 
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[3]);
				contention_failed(&kifr,argv[2]);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}
		}
		if(kifr.data_ptr != NULL)
			free(kifr.data_ptr);
	}
	else
		usage();
	return 1;
}

void get_pkt_num(struct kifreq* ifr_ptr)
{
	ifr_ptr->cmd = PRINTRLC;
	ifr_ptr->size = 0;
	ifr_ptr->data_ptr = NULL;
}
void get_mac_pkt_num(struct kifreq* ifr_ptr)
{
	ifr_ptr->cmd = _IOCTLtoUEMAC_PKT_NUM;
	ifr_ptr->size = 0;
	ifr_ptr->data_ptr = NULL;
}
void contention_failed(struct kifreq * ifr_ptr,char *failed_times)
{
	int *times=(int *)malloc(sizeof(int));
	*times=*failed_times-48;
	ifr_ptr->cmd = _IOCTLtoUEMAC_Contention_Failed ;
	ifr_ptr->size = sizeof(int);
	ifr_ptr->data_ptr = (void *)times;
}

void rar_failed(struct kifreq * ifr_ptr,char *failed_times)
{
	int *times=(int *)malloc(sizeof(int));
	*times=*failed_times-48;
	ifr_ptr->cmd = _IOCTLtoUEMAC_RAR_Failed ;
	ifr_ptr->size = sizeof(int);
	ifr_ptr->data_ptr = (void *)times;
}



void send_reset(struct kifreq* ifr_ptr)
{
	ifr_ptr->cmd = _RRCtoMAC_reset ;
	ifr_ptr->size = 0;
	ifr_ptr->data_ptr = NULL;
}


void usage(void)
{
	const char* info_ptr = "./uemac -rar_failed i LTE_MAC0   \
			  	\n./uemac -contention_failed i LTE_MAC0 \n";
	//printf(info_ptr);
}


	
	
