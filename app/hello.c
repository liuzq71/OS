#include <syscall.h>
#include <sys/syscall.h>
int main(){
	syscall(__NR_write,1,"Hello World!",sizeof("Hello World!"));
}