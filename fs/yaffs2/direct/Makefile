CPPFLAGS   	:= -I$(INCLUDEDIR)
#CFLAGS+=    -fstack-check -O0

#CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations
#CFLAGS+=   -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline


DIRECTTESTOBJS	= port/yaffs_osglue.o yaffs_ecc.o yaffsfs.o yaffs_guts.o \
		 port/yaffs_nandif.o \
		 yaffs_tagscompat.o yaffs_hweight.o yaffs_packedtags2.o  yaffs_nand.o \
		 yaffs_checkptrw.o  yaffs_qsort.o yaffs_error.o yaffs_summary.o yaffs_verify.o \
		 yaffs_nameval.o yaffs_attribs.o yaffs_allocator.o yaffs_tagsmarshall.o \
		 yaffs_yaffs2.o yaffs_yaffs1.o yaffs_bitmap.o
		 
COPYFILES	= yaffs_ecc.h           yaffs_packedtags1.h   yaffs_trace.h \
		yaffs_allocator.c  yaffs_getblockinfo.h  yaffs_packedtags2.c   yaffs_verify.c \
		yaffs_allocator.h  yaffs_guts.c          yaffs_packedtags2.h   yaffs_verify.h \
		yaffs_attribs.h    yaffs_guts.h          yaffs_summary.c       yaffs_yaffs1.c \
		yaffs_bitmap.c     yaffs_nameval.c       yaffs_summary.h       yaffs_yaffs1.h \
		yaffs_bitmap.h     yaffs_nameval.h       yaffs_tagscompat.c    yaffs_yaffs2.c \
		yaffs_checkptrw.c  yaffs_nand.c          yaffs_tagscompat.h    yaffs_yaffs2.h \
		yaffs_checkptrw.h  yaffs_nand.h          yaffs_tagsmarshall.c \
		yaffs_ecc.c        yaffs_packedtags1.c   yaffs_tagsmarshall.h \

ALLOBJS		= $(DIRECTTESTOBJS)

.PHONY:all clean distclean

all:yaffs2.o

%.o: %.c
	@echo CC	$^
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

yaffs2.o: $(COPYFILES) $(DIRECTTESTOBJS)
#-r 表示可重定向，可以合并.o文件,简化上层makefile的链接
	@echo LD	ALL OBJS
	@$(LD) -r -o $@ $(DIRECTTESTOBJS) >/dev/null
	
$(COPYFILES):
	@echo CP	$@
	@cp ../$@ ./ >/dev/null

clean:
	@echo RM	ALL OBJS
	@-rm -f *.o port/*.o >/dev/null
	
distclean:clean
	@echo RM	ALL LINKS
	@-rm -f $(COPYFILES) >/dev/null
