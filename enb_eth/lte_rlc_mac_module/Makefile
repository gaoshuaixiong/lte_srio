TARGET = lte_rlc_mac_module
OBJS =  fsmcore.o rlc_mac.o rlc_macfsm.o queue_manager.o mac_pkthandler.o mac_scheduler.o enb_macscheduler.o Rlcprimitives.o segment.o test.o arq.o

KBUILD_EXTRA_SYMBOLS = /home/xirujun/Desktop/lte_merge_code/liantiao_enb_20151228_merge/lte_virtual_srio_module/Module.symvers
EXTRA_CFLAGS += -w -g#avoid printing warning information
CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)

obj-m      := $(TARGET).o

$(TARGET)-objs := $(OBJS)

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c
