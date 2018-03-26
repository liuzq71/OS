CROSS_COMPILE	= arm-none-linux-gnueabi-
AS				= $(CROSS_COMPILE)as
LD				= $(CROSS_COMPILE)ld
CC				= $(CROSS_COMPILE)gcc
CPP				= $(CC) -E
AR				= $(CROSS_COMPILE)ar
NM				= $(CROSS_COMPILE)nm
STRIP			= $(CROSS_COMPILE)strip
OBJCOPY			= $(CROSS_COMPILE)objcopy
OBJDUMP			= $(CROSS_COMPILE)objdump
RM				=rm

PWDPATH			= $(subst /c,c:,$(shell pwd))
#WFLAGS			:= -Wall
WFLAGS			:= -w
CFLAGS 			:= -std=gnu99 $(WFLAGS) -O2 -fno-builtin \
				-march=armv4t -mtune=arm920t -nostdlib -nostdinc -msoft-float -fsigned-char
INCLUDEDIR 		:= $(PWDPATH)/include -iquote$(PWDPATH)/fs/yaffs2/direct -iquote$(PWDPATH)/fs/yaffs2/direct/port
CPPFLAGS   		:= -I$(INCLUDEDIR)
FSDIR			:= fs/yaffs2/direct
FSTOOLSDIR		:= fs/yaffs2/utils
LDFLAGS			:= -L$(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc
objs			:= init/init_builtin.o
objs			+= drivers/drivers_builtin.o drivers/usbslave/usbslave_builtin.o
objs			+= fs/fs_builtin.o
objs			+= kernel/kernel_builtin.o
objs			+= mm/mm_builtin.o
objs			+= $(FSDIR)/yaffs2.o
objs			+= lib/libc.a

			
export CC LD OBJCOPY OBJDUMP AR CFLAGS PWDPATH INCLUDEDIR

.PHONY:all compile libc download yaffs2 clean distclean app
#如果compile没有生成新的obj,bootloader.bin 也不会更新
all:compile bootloader.bin 

bootloader.bin:$(objs)
	@echo LD	bootloader_elf
	@$(LD) -Tbootloader.lds -o bootloader.elf $(objs) $(LDFLAGS)>/dev/null
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

app:
	@make -C app

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
	@-make clean -C app

distclean:
	@-make clean -C init
	@-make clean -C drivers
	@-make clean -C mm
	@-make clean -C fs
	@-make clean -C kernel
	@-make clean -C drivers/usbslave
	@-make clean -C app
	@-make distclean -C lib
	@-make distclean -C $(FSDIR)
	@-make distclean -C $(FSTOOLSDIR)
	@echo RM	bootloader_elf bootloader.dis *.o
	@-rm -f bootloader.elf bootloader.dis *.o >/dev/null
	@echo RM	bootloader.bin
	@-rm -f bootloader.bin >/dev/null