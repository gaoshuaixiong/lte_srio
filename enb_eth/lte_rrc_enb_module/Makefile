TARGET = lte_rrc_module
OBJS = fsmcore.o rrcfsm_enb.o rrcfsm_enb_ue.o rrc_enb_module.o rrc_utils.o rrc_conn_reconfig_enb.o rrc_conn_release_enb.o impl_rrc_conn_establish.o rrc_enb_paging_SI.o
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
