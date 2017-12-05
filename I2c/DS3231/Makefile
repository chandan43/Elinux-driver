obj-m := ds3231.o

KDIR =  /home/elinux/linux-4.4.96

PWD := $(shell pwd)

default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

#make ARCH=arm CROSS_COMPILE=arm-linux-
