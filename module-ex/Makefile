obj-m = sc.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.ko *.mod *.mod.* .*.cmd *.o *.symvers *.order
