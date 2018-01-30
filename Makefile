CROSS_COMPILE=arm-none-linux-gnueabi-
CC			=$(CROSS_COMPILE)gcc
LD			=$(CROSS_COMPILE)ld
OBJCOPY		=$(CROSS_COMPILE)objcopy
OBJDUMP		=$(CROSS_COMPILE)objdump
AR			=$(CROSS_COMPILE)ar
RM			=rm
#PWDPATH		=$(shell pwd)
#windowsd下使用git shell，此方法将头文件目录传给gcc时目录有问题
PWDPATH		=C:/Users/ZhUyU/Desktop/mOS
#WFLAGS		:= -Wall
WFLAGS		:= -w
CFLAGS 		:= -std=gnu99 $(WFLAGS) -O2 -fno-builtin \
			-march=armv4t -mtune=arm920t -nostdlib -nostdinc -msoft-float -fsigned-char
INCLUDEDIR 	:= $(PWDPATH)/include -iquote$(PWDPATH)/fs/yaffs2/direct -iquote$(PWDPATH)/fs/yaffs2/direct/port
CPPFLAGS   	:= -I$(INCLUDEDIR)
LIBDIR		:= $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`)
FSDIR		:= fs/yaffs2/direct
FSTOOLSDIR	:= fs/yaffs2/utils
LDFLAGS		:= -L $(LIBDIR) -lgcc
objs		:= init/init_builtin.o
objs		+= drivers/drivers_builtin.o drivers/usbslave/usbslave_builtin.o
objs		+= fs/fs_builtin.o
objs		+= kernel/kernel_builtin.o
objs		+= mm/mm_builtin.o
objs		+= $(FSDIR)/yaffs2.o
objs		+= lib/libc.a
libobjs		+= $(LIBDIR)/libgcc.a			
			
			
export CC LD OBJCOPY OBJDUMP AR CFLAGS PWDPATH INCLUDEDIR

.PHONY:all compile libc download yaffs2 clean distclean
#如果compile没有生成新的obj,bootloader.bin 也不会更新
all:compile bootloader.bin 

bootloader.bin:$(objs)
	@echo LD	bootloader_elf
	@$(LD) $(LDFLAGS) -Tbootloader.lds -o bootloader.elf $(objs) $(libobjs)  >/dev/null
	@echo OBJCOPY	$@
	@$(OBJCOPY) -O binary -S bootloader.elf $@  >/dev/null

disasm:bootloader.bin
	@echo OBJDUMP	bootloader.dis
	@$(OBJDUMP) -D -m arm bootloader.elf > bootloader.dis

compile:
	@make -C lib
	@make yaffs2.o -C $(FSDIR)
	@make -C init
	@make -C drivers
	@make -C mm
	@make -C fs
	@make -C kernel
	@make -C drivers/usbslave

download:bootloader.bin
	dnw bootloader.bin

clean:
	@echo RM	bootloader_elf bootloader.dis *.o
	@-rm -f bootloader.elf bootloader.dis *.o >/dev/null
	@-make clean -C lib
	@-make clean -C init
	@-make clean -C drivers
	@-make clean -C mm
	@-make clean -C fs
	@-make clean -C kernel
	@-make clean -C drivers/usbslave
	
distclean:
	@-make clean -C init
	@-make clean -C drivers
	@-make clean -C mm
	@-make clean -C fs
	@-make clean -C kernel
	@-make clean -C drivers/usbslave
	@-make distclean -C lib
	@-make distclean -C $(FSDIR)
	@-make distclean -C $(FSTOOLSDIR)
	@echo RM	bootloader_elf bootloader.dis *.o
	@-rm -f bootloader.elf bootloader.dis *.o >/dev/null
	@echo RM	bootloader.bin
	@-rm -f bootloader.bin >/dev/null