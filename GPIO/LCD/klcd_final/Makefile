#
# 	A Makefile to cross-compile a kernel module
#
#	(from a Ubuntu PC to a Beaglebone Black ARM target board)
#

obj-m	:= klcd.o

KDIR =/home/elinux/linux-4.4.96
#KDIR =/home/shrikant/embedded_and_linux/porting/builds/linux-4.4.94

PWD := $(shell pwd)

default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	@rm -rf a.out
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

#make ARCH=arm CROSS_COMPILE=arm-linux-

