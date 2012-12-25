KERVER = $(shell uname -r)
KERNEL ?= /lib/modules/$(KERVER)/build
RUNPATH = $(shell pwd)

MAJOR = `cat /proc/devices|grep "GSFS"|cut -f1 -d" "`

#ati_home=/opt/AMDAPP
ati_home=/home/Programs/GPU/ati-stream-sdk-v2.2-lnx64
add_exten = -Wfloat-equal -Wpointer-arith -g3 
#PATH := ${PATH}:$(ati_home)/bin/x86_64
#LD_LIBRARY_PATH := $(ati_home)/lib/x86_64:${LD_LIBRARY_PATH}


ifndef $n
	n=512
endif

obj-m := GSFS.o 

GSFS-objs :=	lru.o		super.o		inode.o		file.o\
		pagecache.o	reg_inode.o	users.o		cdev.o\
		events.o	hash.o		avl_tree.o	cipher.o	crust.o\
		skein512.o	rsa.o 		aes.o		gcm.o
all:		
	make -i um
	make -i rem
	@echo -e "\n**** All\n"
	make -C $(KERNEL) M=$(RUNPATH) modules
	make ins

mknod:
	@mknod gsfs_cdev c $(MAJOR) 0
	@chmod a+rw gsfs_cdev

gsfs_user_module: gsfs_user_module.o
	make mknod
	./gsfs_user_module.o `pwd`/gsfs_cdev | tee gum_output &

gsfs_user_module.o:  gsfs_user_module.obj
	#@rm gsfs_user_module.o
	gcc gsfs_user_module.obj -o gsfs_user_module.o -lpthread -lOpenCL -L$(ati_home)/lib/x86_64

gsfs_user_module.obj:  gsfs_user_module.c gsfs_user_module.h
	#@rm gsfs_user_module.obj 
	gcc  -std=gnu99 gsfs_user_module.c  -c -o gsfs_user_module.obj  -I $(ati_home)/include 

gup:
	@make all -i -f gup_Makefile -e p1=$(p1) p2="$(p2)" p3="$(p3)"

call:
	touch *.c *.h
	make all

clean:
	@echo -e "\n**** Clean\n"
	@echo "">/var/log/messages
	rm *.o
	rm -f *tmp* -r
	rm -f *~
	rm -f *.cmd

view:  
	@echo -e "\n**** View\n"
	@cat /var/log/messages

ins:
	@echo -e "\n**** Insert\n"
	@insmod GSFS.ko 

rem:
	@echo -e "\n**** Remove\n"	
	@rmmod GSFS.ko

mount:	c1
	mount -o loop=/dev/loop0 fs /media/fs1 -t GSFS
	make gsfs_user_module 

mount1:
	mount /dev/sda9 /media/fs1 -t GSFS
	make gsfs_user_module 

mountc: c1
	mount -o loop=/dev/loop0,create fs /media/fs1 -t GSFS
	make gsfs_user_module 

mount1c:
	mount /dev/sda9 -o create /media/fs1 -t GSFS
	make gsfs_user_module 
um:
	umount /media/fs1
	rm gsfs_cdev
	make rem

c1:	fs
	gcc 1.c -o 1.o -std=c99
	./1.o>fs

c2:
	gcc 2.c -o 2.o -std=c99
	./2.o
nl2:		
	echo `nl *.h *.c |tail -n 1|cut -f1`  "-" `nl G*.c|tail -n 1|cut -f1`|bc
nl:		
	echo `nl *.h *.c |tail -n 1|cut -f1`  "-" `nl  iml*.c ?.c G*.c skein*.c aes.c rsa.c |tail -n 1|cut -f1`|bc

hd:
	hexdump -C fs -n $n -s `echo "4096*$(b)"|bc`
edit:
	kate gsfs.h super.c inode.c reg_inode.c file.c pagecache.c gcm.c hash.c events.c cdev.c message.h users.c gsfs*.c gsfs_user_module.h cipher.c lru.c crust.c rsa.c avl_tree.c Makefile gup_Makefile gum_output me_test.c&

mountgc:
	mount -o loop=/dev/loop0,create gs /media/fs1 -t GSFS
mountg:
	mount -o loop=/dev/loop0 gs /media/fs1 -t GSFS

me:
	mount /dev/sda9 /media/fs2
	make -i all
	mount -oloop=/dev/loop0,create /media/fs2/1 /media/fs1 -t GSFS
	make mknod
	make -i gup -e p1=12 p2=.
	mkdir /media/fs1/1
	make -i gup -e p1=3 p2=/media/fs1/1
	#gcc me_test.c -o me_test.o -std=c99 -D_GNU_SOURCE;./me_test.o /media/fs1/1 res_gsfs_kernel

m1:
	make -i all mountc
	mkdir /media/fs1/1
	make -i gup -e p1=12 p2=.
	make -i gup -e p1=3 p2=/media/fs1/1
	#dd if=gs of=/media/fs1/1/2 bs=4096 count=1
	#make um

m11:
	umount /media/fs1
	umount /media/fs2
	make -i all
	mount /dev/sda9 /media/fs2
	mount -o loop=/dev/loop0 /media/fs2/1 /media/fs1 -t GSFS
	make gsfs_user_module 
	make -i gup -e p1=2 p2=.
	#mkdir /media/fs1/1
	#make -i gup -e p1=2 p2=.
	#dd of=13 if=/media/fs1/1/2 bs=4096 count=1

m2:
	make -i all mountc
	mkdir /media/fs1/1
	make -i gup -e p1=12 p2=.
	make -i gup -e p1=3 p2=/media/fs1/1
	#dd if=gs of=/media/fs1/1/2 bs=40960 count=1
	#dd if=3.pdf of=/media/fs1/1/3 bs=4096000 count=5
	

m21:
	make -i all mount
	make -i gup -e p1=2 p2=.
	dd of=13 if=/media/fs1/1/2 bs=40960 count=1

m211:
	mkdir /media/fs1/1/11
	mkdir /media/fs1/1/12
	mkdir /media/fs1/1/11/111
	mkdir /media/fs1/1/11/112

m22:
	mkdir /media/fs1/1/11/111/1111
	mkdir /media/fs1/1/11/111/1111a
	mkdir /media/fs1/1/11/111/1111b
	mkdir /media/fs1/1/11/111/1111/2a
	mkdir /media/fs1/1/11/111/1111/2b

m23:
	mkdir /media/fs1/1/11/111/1111/2a/2aa
	mkdir /media/fs1/1/11/111/1111/2a/2ab

m3:	
	make -i gup -e p1=4 p2="/media/fs1/1/12" p3="0 1"
	mkdir /media/fs1/1/12/121
	mkdir /media/fs1/1/12/121/121a
	mkdir /media/fs1/1/12/121/121b
	mkdir /media/fs1/1/12/122
	mkdir /media/fs1/1/12/123
	make -i gup -e p1=4 p2="/media/fs1/1/12" p3="1001 1"
m35:
	make -i gup -e p1=4 p2="/media/fs1/1/12/123" p3="1002 0"

m4:
	mkdir /media/fs1/1/12/124
	mkdir /media/fs1/1/12/125
	mkdir /media/fs1/1/12/126
	make -i gup -e p1=3 p2=/media/fs1/1/12/123
m45:
	mkdir /media/fs1/1/12/123/123a
	mkdir /media/fs1/1/12/123/123b

r1:
	make -i gup -e p1=5 p2=/media/fs1/1/12/123 p3="1002"

r2:
	make -i gup -e p1=5 p2=/media/fs1/1/12 p3="1001"

au1:
	make -i gup -e p1=4 p2="/media/fs1/1/12" p3="1001 0"

l0:
	make -i gup -e p1=2 p2=.

c0:
	make -i gup -e p1=12 p2=.

ext3test_mount:
	mount /dev/sda9 /media/fs2
	mount -o loop=/dev/loop0 /media/fs2/1 /media/fs1

ext3test_umount:
	umount /media/fs1
	umount /media/fs2

tar:
	tar -cf gsfs.tar gsfs.h super.c inode.c reg_inode.c file.c pagecache.c gcm.c hash.c events.c cdev.c message.h users.c gsfs*.c gsfs_user_module.h cipher.c lru.c crust.c rsa.c avl_tree.c Makefile gup_Makefile ?.c me_test.c
