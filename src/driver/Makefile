LNX?=~/linux-$(shell uname -r)
obj-m += beagle_pov.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

load: beagle_pov.ko unload
	insmod ./beagle_pov.ko

unload:
	rmmod ./beagle_pov.ko || true

install:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install

cscope:
	find $(LNX) \
        -path "$(LNX)/arch/*" ! -path "$(LNX)/arch/arm" -prune -o               \
        -path "$(LNX)/include/*" -prune -o \
        -path "$(LNX)/tmp*" -prune -o                                           \
        -path "$(LNX)/Documentation*" -prune -o                                 \
        -path "$(LNX)/scripts*" -prune -o                                       \
        -path "$(LNX)/drivers*" -prune -o                                       \
            -name "*.[chxsS]" -print > cscope.files
	find ./ -name "*.h" >> cscope.files
	find ./ -name "*.c" >> cscope.files
	cscope -i cscope.files

ctags:
	find /usr/src/linux-headers-$(shell uname -r)/ -name "*.h" -o -name "*.c" > ctags.file
	find ./ -name "*.h" >> ctags.file
	find ./ -name "*.c" >> ctags.file
	ctags -L ctags.file

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
