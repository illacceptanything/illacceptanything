# Makefile for hello world kernel module
obj-m := hello.o	# Module Name is hello.c

KDIR  := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) Module.markers modules.order
