CPPFLAGS   	:= -I$(INCLUDEDIR)
SRC			:= $(wildcard *.c)
OBJ_O		:= $(patsubst %.c, %.o, $(SRC))
SRC			:= $(wildcard *.S)
OBJ_S		:= $(patsubst %.S, %.o, $(SRC))
.PHONY:all clean distclean

../lib/libc.a: $(OBJ_O) $(OBJ_S)
	@echo AR	ALL OBJS
	@$(AR) -r -o ../lib/$@ $^ >/dev/null

$(OBJ_O): %.o:%.c
	@echo CC	$^
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@
	
$(OBJ_S): %.o:%.S
	@echo CC	$^
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

clean:
	@echo RM	$(OBJ_O) $(OBJ_S)
	@-$(RM) -f $(OBJ_O) $(OBJ_S)  >/dev/null

distclean:clean
	@echo RM	libc.a
	@-rm -f ../lib/libc.a >/dev/null