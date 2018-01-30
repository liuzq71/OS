#include <stdio.h>
#include <syscall.h>
SYSCALL_DEFINE1(print,char *,str){
	dbgprintf("syscall:print\n");
	dbgprintf("arg0=%0X\n",str);
	if(str)
		printf(str);
}