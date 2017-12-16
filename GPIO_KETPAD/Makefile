#obj-m := matrix_keypad.o
obj-m += matrix-keymap.o
#obj-m += gpiokeypad.o
obj-m += gpiokeypad_LCD.o
obj-m += i2cLCD_drv.o

KDIR =  /home/elinux/linux-4.4.96

PWD := $(shell pwd)

default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

#make ARCH=arm CROSS_COMPILE=arm-linux-
