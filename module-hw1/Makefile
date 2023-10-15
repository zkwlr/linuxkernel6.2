obj-m = hw1.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.ko *.mod *.mod.* .*.cmd *.o *.symvers *.order
