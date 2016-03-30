TARGET = lte_virtual_srio_module
OBJS =  fsmcore.o sriofsm.o virtual_srio.o


EXTRA_CFLAGS += -w #avoid printing warning information
CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
#This is the kernel of Fedora18.
 
#KDIR = /home/fedora/QorIQ-SDK-V1.3.2-20130325-yocto/amc718/kernel
#This is the kernel of AMC719


PWD = $(shell pwd)

obj-m      := $(TARGET).o

$(TARGET)-objs := $(OBJS)

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c
