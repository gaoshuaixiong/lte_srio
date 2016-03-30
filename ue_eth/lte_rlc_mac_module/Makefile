TARGET = lte_rlc_mac_module
OBJS =  fsmcore.o rlc_mac.o	rlc_macfsm.o mac_pkthandler.o mac_scheduler.o Rlcprimitives.o segment.o test.o arq.o

EXTRA_CFLAGS += -w -g #avoid printing warning information
CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)

obj-m      := $(TARGET).o

$(TARGET)-objs := $(OBJS)

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c
