#include <stdint.h>
#define USERIOCCMD	SIOCDEVPRIVATE + 1

#define PSEND_RUN 	0x01
#define PSEND_STOP 	0x02
#define PSEND_INTERVAL	0x03
typedef uint32_t 	u32;

struct kifreq
{
	u32 cmd;
	size_t	size;
	void* data_ptr;
};

void periodical_send_stop(struct kifreq* ifr_ptr);	
void periodical_send_run(struct kifreq* ifr_ptr);
void periodical_send_interval(struct kifreq* ifr_ptr, float interval);
void usage(void);		
