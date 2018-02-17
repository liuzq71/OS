CPPFLAGS   	:= -I$(INCLUDEDIR)
SRC			:= $(wildcard *.c)
OBJ_O		:= $(patsubst %.c, %.o, $(SRC))
ELF			:= $(patsubst %.o, %.elf, $(OBJ_O))
.PHONY:all clean

all:$(ELF)

$(OBJ_O): %.o:%.c
	@echo CC	$^
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@
$(ELF): %.elf:%.o
	@echo LD	$^
	@$(LD) -o $@ $^ ../kernel/syscall.o >/dev/null
clean:
	@echo RM	ALL OBJS
	@-$(RM) -f *.o  >/dev/null
	@-$(RM) -f *.elf  >/dev/null