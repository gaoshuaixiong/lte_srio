#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "./srio.h" 

//sriotest -s LTE_SRIO0
//sriotest -r LTE_SRIO0
//sriotest -i LTE_SRIO0 xxxxx

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
		
		if(strcmp(argv[1], "-s") == 0)
		{
			if(argc == 3)
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				periodical_send_stop(&kifr);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}	
		}
		if(strcmp(argv[1], "-send_sf") == 0)
		{
			if(argc == 3)
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				send_sf(&kifr);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}	
		}
		else if(strcmp(argv[1], "-r") == 0)
		{	
			if(argc == 3)
			{
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				periodical_send_run(&kifr);
				ifr.ifr_ifru.ifru_data = (void*)&kifr;
				ioctl(sock, USERIOCCMD, &ifr);
			}
		}
		else if(strcmp(argv[1], "-i") == 0) 
		{
			if(argc == 4)
			{
				interval = atof(argv[3]);
				if(interval < 0.00001)
				{
					printf("The minimum send interval is 10 us.\n");
					return -1;
				}
				strcpy(ifr.ifr_ifrn.ifrn_name, argv[2]);
				periodical_send_interval(&kifr, interval);
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
void periodical_send_stop(struct kifreq* ifr_ptr)
{
	ifr_ptr->cmd = PSEND_STOP;
	ifr_ptr->size = 0;
	ifr_ptr->data_ptr = NULL;
}

void send_sf(struct kifreq* ifr_ptr)
{
	ifr_ptr->cmd = SEND_SF;
	ifr_ptr->size = 0;
	ifr_ptr->data_ptr = NULL;
}

void periodical_send_run(struct kifreq* ifr_ptr)
{
	ifr_ptr->cmd = PSEND_RUN;
	ifr_ptr->size = 0;
	ifr_ptr->data_ptr = NULL;
}

void periodical_send_interval(struct kifreq* ifr_ptr, float interval)
{	
	unsigned int *ptr;
	ifr_ptr->cmd = PSEND_INTERVAL;
	ifr_ptr->size = sizeof(unsigned int);
	ptr = (unsigned int*) malloc(sizeof(unsigned int));
	*ptr = (unsigned int)(interval*100000);
	ifr_ptr->data_ptr = ptr;
}

void usage(void)
{
	const char* info_ptr = "sriotest -s LTE_SRIO0   \
			  	\nsriotest -r LTE_SRIO0 \
			  	\nsriotest -send_sf LTE_SRIO0 \
			  	\nsriotest -i LTE_SRIO0 xxxxx (seconds)\n";
	printf("%s", info_ptr);
}
	
	
	
	
