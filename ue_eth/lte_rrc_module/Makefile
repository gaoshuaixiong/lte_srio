TARGET = lte_rrc_module
OBJS = fsmcore.o rrc_utils.o rrcfsm.o rrc_module.o rrc_conn_establish.o rrc_conn_reconfig.o rrc_conn_release.o rrc_paging_SI.o

EXTRA_CFLAGS += -w -g
CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)

obj-m      := $(TARGET).o

$(TARGET)-objs := $(OBJS)

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c
