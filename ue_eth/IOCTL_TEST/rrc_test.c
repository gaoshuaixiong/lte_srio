#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "./rrc_test.h" 

#define MAX_IOCTL_DATA_LEN 1024

struct Ioctl_Msg_Rrc_Enb {
	unsigned short crnti;
	unsigned int data_len;
	char data[MAX_IOCTL_DATA_LEN]; 
};


//3.用户空间发起连接重配置
#define IOCCMD_USTORRC_RRC_CONN_RECONFIG 	0x69
//rrctest -reconfig LTE_RRC_ENB0 12345 hello_world
#define IOCCMD_USTORRC_RRC_CONN_RELEASE 0x68
//用户空间发起寻呼
#define IOCCMD_USTORRC_PAGING 0x12
#define IOCCMD_MACTORRC_REPORT_CRNTI  0x31


//rrctest -a LTE_RRC_ENB0
//rrctest -b LTE_RRC_ENB0
//rrctest -i LTE_RRC_ENB0 xxxxx

int main(int argc, char *argv[])
{
	int sock;
	struct ifreq ifr;
	struct kifreq kifr;
	float interval;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	kifr.data_ptr = NULL;
	if(argc > 2)
	{

		if(strcmp(argv[1], "-a") == 0)
		{
			if(argc == 3)
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				start_cell_access(&kifr);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}	
		}
		else if(strcmp(argv[1], "-b") == 0)
		{	
			if(argc == 3)
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				rrc_conn_req(&kifr);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}
		}
		else if(strcmp(argv[1], "-reconfig") == 0) 
		{
			unsigned short crnti=0;
			char data[100];
			if(argc == 4)
			{
				crnti = atoi(argv[3]);
				strcpy(data, " ");
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				start_rrc_reconfig(&kifr, crnti, data);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);

			}
		}
		else if(strcmp(argv[1], "-release") == 0) 
		{
			unsigned short crnti=0;
			char data[100];
			if(argc == 4)
			{
				crnti = atoi(argv[3]);
				strcpy(data, " ");
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				start_rrc_release(&kifr, crnti, data);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);

			}
		}
		else if(strcmp(argv[1], "-paging") == 0)
		{
			unsigned short crnti=0;
			char data[100];
			if(argc == 3)
			{
				crnti = 0;
				strcpy(data, " ");

				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				start_rrc_paging(&kifr, data);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}
		}
		else if(strcmp(argv[1], "-build") == 0)
		{
			unsigned short crnti=0;
			char data[100];
			if(argc == 4)
			{
				crnti = atoi(argv[3]);
				strcpy(data, " ");

				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				//	start_rrc_paging(&kifr, data);
				test_build_uefsm(&kifr, crnti, data);
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


void start_cell_access(struct kifreq* ifr_ptr)
{
	ifr_ptr->cmd = IOCCMD_USTORRC_CELL_ACCESS;
	ifr_ptr->size = 0;
	ifr_ptr->data_ptr = NULL;
}



void rrc_conn_req(struct kifreq* ifr_ptr)
{
	ifr_ptr->cmd = IOCCMD_USTORRC_RRC_CONN_REQ;
	ifr_ptr->size = 0;
	ifr_ptr->data_ptr = NULL;
}


void start_rrc_reconfig(struct kifreq* ifr_ptr, unsigned short crnti, const char *data)
{	
	struct Ioctl_Msg_Rrc_Enb *msg;
	msg = (struct Ioctl_Msg_Rrc_Enb *) malloc(sizeof(struct Ioctl_Msg_Rrc_Enb));
	msg->crnti = crnti;
	msg->data_len =  strlen(data)+1;
	memcpy(msg->data, data, msg->data_len);

	printf("send msg->data:%s\n", msg->data);

	ifr_ptr->cmd = IOCCMD_USTORRC_RRC_CONN_RECONFIG;
	ifr_ptr->size = sizeof(struct Ioctl_Msg_Rrc_Enb);
	ifr_ptr->data_ptr = msg;
}
void start_rrc_release(struct kifreq* ifr_ptr, unsigned short crnti, const char *data)
{	
	struct Ioctl_Msg_Rrc_Enb *msg;
	msg = (struct Ioctl_Msg_Rrc_Enb *) malloc(sizeof(struct Ioctl_Msg_Rrc_Enb));
	msg->crnti = crnti;
	msg->data_len =  strlen(data)+1;
	memcpy(msg->data, data, msg->data_len);

	printf("send msg->data:%s\n", msg->data);

	ifr_ptr->cmd = IOCCMD_USTORRC_RRC_CONN_RELEASE;
	ifr_ptr->size = sizeof(struct Ioctl_Msg_Rrc_Enb);
	ifr_ptr->data_ptr = msg;
}

void start_rrc_paging(struct kifreq* ifr_ptr, const char *data)
{
	struct Ioctl_Msg_Rrc_Enb *msg;
	msg = (struct Ioctl_Msg_Rrc_Enb *) malloc(sizeof(struct Ioctl_Msg_Rrc_Enb));
	//	msg->crnti = crnti;
	msg->data_len =  strlen(data)+1;
	memcpy(msg->data, data, msg->data_len);

	printf("send msg->data:%s\n", msg->data);

	ifr_ptr->cmd = IOCCMD_USTORRC_PAGING;
	ifr_ptr->size = sizeof(struct Ioctl_Msg_Rrc_Enb);
	ifr_ptr->data_ptr = msg;
}
void test_build_uefsm(struct kifreq* ifr_ptr, unsigned short crnti, const char *data)
{	
	struct Ioctl_Msg_Rrc_Enb *msg;
	msg = (struct Ioctl_Msg_Rrc_Enb *) malloc(sizeof(struct Ioctl_Msg_Rrc_Enb));
	msg->crnti = crnti;
	msg->data_len =  strlen(data)+1;
	memcpy(msg->data, data, msg->data_len);

	printf("send msg->data:%s\n", msg->data);

	ifr_ptr->cmd = IOCCMD_MACTORRC_REPORT_CRNTI;
	ifr_ptr->size = sizeof(struct Ioctl_Msg_Rrc_Enb);
	ifr_ptr->data_ptr = msg;
}

void usage(void)
{
	const char* info_ptr = "sriotest -s LTE_SRIO0   \
							\nsriotest -r LTE_SRIO0 \
							\nsriotest -i LTE_SRIO0 xxxxx (seconds)\n";
	printf(info_ptr);
}

