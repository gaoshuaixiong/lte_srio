#define USERIOCCMD	SIOCDEVPRIVATE + 1

#define SUSPEND	 	0x01
#define BUILD 		0x02
#define RECONFIG		0x03
#define DEACT		0x04
#define RESUME		0x05

#define SENDRRC		0x07
#define SENDMAC		0x08
#define	SENDPDCP	0x09
#define PRINTRLC	0x0a
//#define PSEND_STOP 	0x02
//#define PSEND_INTERVAL	0x03

struct kifreq
{
	unsigned int cmd;
	//int cmd;
	//unsigned short	size;
	size_t size;
	void* data_ptr;
};

struct config
{
	unsigned int mode;
	unsigned char rbid;
	//unsigned char*cmd;
};

void periodical_set(struct kifreq* ifr_ptr,unsigned char rbid);
//void periodical_build(struct kifreq* ifr_ptr,struct config*conf);
void periodical_config(struct kifreq* ifr_ptr, struct config*conf, char*cmd);
//void periodical_deact(struct kifreq* ifr_ptr,unsigned char rbid );
//void periodical_resume(struct kifreq* ifr_ptr, unsigned char rbid);
//void periodical_suspend(struct kifreq* ifr_ptr, unsigned char rbid);

void periodical_send_from_rrc(struct kifreq* ifr_ptr);
void periodical_send_from_mac(struct kifreq* ifr_ptr,struct config *tr);
void periodical_send_from_pdcp(struct kifreq* ifr_ptr, unsigned char rbid);

//void periodical_send_interval(struct kifreq* ifr_ptr, float interval);
void usage(void);		
