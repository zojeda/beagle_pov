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
	find /media/sdcard/linux-$(shell uname -r)/ -name "*.h" -o -name "*.c" > cscope.file
	find ./ -name "*.h" >> cscope.file
	find ./ -name "*.c" >> cscope.file
	cscope -i cscope.file
ctags:
	find /usr/src/linux-headers-$(shell uname -r)/ -name "*.h" -o -name "*.c" > ctags.file
	find ./ -name "*.h" >> ctags.file
	find ./ -name "*.c" >> ctags.file
	ctags -L ctags.file

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


