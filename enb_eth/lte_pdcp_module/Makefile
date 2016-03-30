TARGET = lte_pdcp_module
OBJS =  fsmcore.o pdcpfsm.o pdcp.o

EXTRA_CFLAGS += -w
CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)

obj-m      := $(TARGET).o

$(TARGET)-objs := $(OBJS)

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c
