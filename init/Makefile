CPPFLAGS   	:= -I$(INCLUDEDIR)
SRC			:= $(wildcard *.c)
OBJ_O		:= $(patsubst %.c, %.o, $(SRC))
SRC			:= $(wildcard *.S)
OBJ_S		:= $(patsubst %.S, %.o, $(SRC))
.PHONY:all clean

all:init_builtin.o

init_builtin.o:$(OBJ_O) $(OBJ_S)
	@echo LD	ALL OBJS
	@$(LD) -r -o $@ head.o init.o nand_ll.o >/dev/null

$(OBJ_O): %.o:%.c
	@echo CC	$^
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@
	
$(OBJ_S): %.o:%.S
	@echo CC	$^
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@
	
clean:
	@echo RM	ALL OBJS
	@-$(RM) -f *.o  >/dev/null