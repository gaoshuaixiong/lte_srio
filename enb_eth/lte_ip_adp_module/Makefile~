TARGET = lte_ipadp_module
OBJS =  fsmcore.o ipadpfsm.o ipadp_module.o

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
