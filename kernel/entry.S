.global vector_swi
vector_swi:	
	@TODO:现在应用与内核共用一个栈,如果不共用需要添加切换代码
	stmdb   sp!,    { r8-r12,lr }   @ 保存使用到的寄存器
	ldr r8,	= sys_call_table
	ldr r9, [ r8, r7, LSL#2 ]
	ldr lr, = swi_return			@ 设置调用函数后的返回地址
	stmdb   sp!,    { r4-r6 }
    mov pc, r9						@ 调用syscall函数
swi_return:
	ldmia   sp!,    { r4-r6 }
    ldmia   sp!,    { r8-r12,pc }^  @ 中断返回, ^表示将spsr的值复制到cpsr

.equ NR_syscalls,0  
#define CALL(x) .equ NR_syscalls,NR_syscalls+1  
#include "calls.S"  
#undef CALL  
#define CALL(x) .long x 
sys_call_table: 
#include "calls.S" 