#define USERIOCCMD	SIOCDEVPRIVATE + 1

/* -----------------------
IOCTL event code (defined and used by rrc's rrc_ioctl_handler())
----------------------- */
//ST_IDLE_CELL_SEL
#define IOCCMD_USTORRC_CELL_ACCESS 	0x01       //ioctl from user space
//ST_IDLE_NORMALLY
#define IOCCMD_USTORRC_RRC_CONN_REQ 	0x02       //ioctl from user space


struct kifreq
{
	unsigned int cmd;
	size_t size;
	void* data_ptr;
};

void begin_recv_sysinfo(struct kifreq* ifr_ptr);	
void rrc_conn_req(struct kifreq* ifr_ptr);
void start_rrc_release(struct kifreq* ifr_ptr, unsigned short crnti, const char *data);
void start_rrc_paging(struct kifreq* ifr_ptr, const char *data);
void test_build_uefsm(struct kifreq* ifr_ptr, unsigned short crnti, const char *data);


//void periodical_send_interval(struct kifreq* ifr_ptr, float interval);
void usage(void);		
