#include <sys/types.h>
#include <s3c24xx.h>
#include <stdio.h>
#include <syscall.h>
#include "interrupt.h"
#include <sched.h>
#include <fcntl.h>
#include <trace.h>


PCB task[NR_TASK];								// 进程队列(PCB数组)，最多支持62个进程
PCB *__current_task = NULL;						// 声明当前执行进程指针
long runningCount = 0;							// 正在运行进程个数
extern void __switch_to(struct cpu_context_save *prev, struct cpu_context_save *next);
// 引入外部声明上下文切换函数
void switch_to(PCB *pcur, PCB *pnext) {
	__switch_to(&(pcur->context), &(pnext->context));
}
int cmd_loop();
extern void *kmalloc(unsigned int size);
extern void kfree(void *addr);
static void task0();
static void task1();
/*****************************************************************************
* 进程队列初始化函数
*****************************************************************************/
void sched_init(void) {
	PCB *p = &task[0];					// 0号进程为内核进程
	int i;
	/* 循环为每个进程PCB初始化 */
	for (i = 0; i < NR_TASK; i++, p++) {
		p->pid = -1;					// pid = -1，表示未分配pid
		p->state = TASK_UNALLOCATE;		// 设置初始进程状态为未分配状态
		p->count = 0;					// 设置时间片计数为0，表示没有时间片
		p->priority = 0;				// 初始进程优先级为0
	}
	CURRENT_TASK() = NULL;					// 当前运行进程为0号进程
	printf("kernel:sched_init, all task init OK\n");
}
void schedule(void) {
	/*
	* max用来保存当前进程队列里最高优先级进程count
	* p_tsk保存当前进程PID副本
	*/
	long max = -1;						// max初始值为-1，后面做判断
	long i = 0,  next = 0;				// next保存最高优先级PID
	PCB *p_tsk = NULL;					// 临时进程结构体指针

	// 如果只有一个进程且为0号进程,则赋予其时间片,返回
	if (runningCount == 1 && CURRENT_TASK()->pid == 0) {
		CURRENT_TASK()->count = CURRENT_TASK()->priority;
		return ;
	}
	// 进程调度循环

	trace(KERN_DEBUG, "kernel:schedule\n");
	while (1) {
		/*
		*  循环找出进程队列里，就绪状态最高优先级进程，也就是count值最大进程，
		*  count越大说明其被执行时间越短，CPU需求越高，
		*  同时保存其PID（进程队列数组下标）到next里
		*  0号进程不参与比较
		*/
		for (i = 1; i < NR_TASK; i++) {
			if ( (task[i].state == TASK_RUNNING) && (max < (long)task[i].count) ) {
				max = (long)task[i].count;
				next = i;
			}
		}

		// 如果max为非0，跳出循环，说明选出了调度进程
		// 如果max为0，说明count值最大进程count为0，说明全部进程分配时间片已执行完，
		// 需要重新分配，执行break后面for语句
		// 如果max为-1说明没有就绪状态进程可被调度，退出循环，继续执行0进程

		if (max) break;		// max = 0时，选出新进程，跳出循环
		// max = 0，即进程队列中count值最大为0，全部进程时间片用尽，需要重新分配
		for (i = 1; i < NR_TASK; i++) {
			if ( task[i].state == TASK_RUNNING ) {
				// 时间片数为其默认优先级
				task[i].count = task[i].priority;
			}
		}
	}
	// 当前进程为选出进程，说明当前进程优先级还是最高，返回继续执行
	if (CURRENT_TASK() == &task[next])
		return;
	// 无效PID
	if (task[next].pid < 0)
		return;
	// 保存当前进程副本到p_tsk，将选出进程设置为当前运行进程
	p_tsk = CURRENT_TASK();
	CURRENT_TASK() = &task[next];
	trace(KERN_DEBUG, "__switch_to\n");
	trace(KERN_DEBUG, "\nold task id =%d", p_tsk->pid);
	trace(KERN_DEBUG, "\nnew task id =%d", next);
	trace(KERN_DEBUG, "\n");
	// 调用上下文切换函数
	switch_to(p_tsk, &task[next]);
}
/*****************************************************************************
* 定时器处理函数
* 主要用于进程时间片处理和睡眠时间处理，每次定时器中断产生后，调用该函数，对进程时间片进行
* 递减操作，如果时间片用完，则进行调度，如果用户进程主动进入睡眠状态，则
* 该函数在睡眠时间到达后，将其唤醒
*****************************************************************************/
void do_timer(void) {
	int i = 0;
	// 没有当前进程，说明进程还未创建，返回
	if (!CURRENT_TASK()) {
		trace(KERN_WARNING, "kernel:leaving do_timer,hasn't init task\n");
		return;
	}
	// 递减睡眠进程，睡眠时间到了，将其状态改为就绪态
	for (i = 1; i < NR_TASK; i++) {
		if (task[i].state == TASK_SLEEPING) {			// 检查其睡眠时间
			if (!(--task[i].timer)) {
				task[i].state = TASK_RUNNING;		// 如果睡眠时间为0，唤醒它
				runningCount++;
			}
		}
	}
	// 对当前执行进程时间片递减，每10ms递减一次
	if (CURRENT_TASK()->count) {
		CURRENT_TASK()->count--;
	}
	// 如果当前进程时间片已经用完，或当前进程状态为非就绪态，则尝试调度新进程
	if ((CURRENT_TASK()->state != TASK_RUNNING) || CURRENT_TASK()->count <= 0 ) {
		// 保障内核空间执行进程不会被抢占打断
		//if(is_in_user_space())
		schedule();
	}
}
/*****************************************************************************
* 杀死进程函数
* 参数：int pid：0：随机杀死一个进程，pid不为0：杀死指定进程id号为pid进程
*****************************************************************************/
void kthread_kill(int pid) {
	int i;
	if (pid == 0)
		return;
	trace(KERN_DEBUG, "kernel:kill_task\r\n");
	for (i = 1; i < NR_TASK; i++) {
		if (task[i].state != TASK_UNALLOCATE) {
			if (pid == task[i].pid) {
				task[i].pid = -1;
				task[i].state = TASK_UNALLOCATE;
				task[i].count = 0;
				task[i].priority = 0;
				runningCount--;
				break;
			}
		}
	}
	// 杀死进程后，重新调度
	schedule();
}
void debug_task() {
	printf("debug\n");
	int *ad = 0x33d00000 - 14 * 4;
	for (int i = 0; i < 14; i++) {
		printf("%X\n", ad[i]);
	}
	printf("\n");
	struct cpu_context_save *context = &(CURRENT_TASK()->context);
	trace(KERN_DEBUG, "cpsr:%X,sp:%X,pc:%X\n", context->cpsr, context->sp, context->pc);
}
void task_start(__u32 sp, __u32 pc) {
	trace(KERN_DEBUG, "sp:%X,pc:%X\n", sp, pc);
	trace(KERN_DEBUG, "task_start\n");
	asm(
	    "mov sp,%0\n"
	    "mov pc,%1\n"
	    :
	    :"r"(sp), "r"(pc)
	);
}
void task_timer_init() {
	int delay_time = 10000;
	//定时器配制寄存器 0 （TCFG0）
	TCFG0 |= (24); //定时器 0，1 的预分频值
	//定时器控制寄存器 1 （TCON）
	TCON &= (~(15 << 8)); //清空8~11位
	TCON |= (1 << 11); //定时器 1间隙模式（自动重载）
	TCON |= (1 << 9); //定时器 1手动更新 TCNTB1
	//TCONB1:定时器 1  计数缓冲寄存器
	TCNTB1 = delay_time;

	TCON |= (1 << 8); //启动
	TCON &= ~(1 << 9); //定时器 1 取消手动更新
}
void task_init() {
	sched_init();
	void *sp = kmalloc(512);
	if (sp) {
		kthread_create(task0, sp, 5);
		if (task[0].state != TASK_RUNNING) {
			trace(KERN_CRIT, "task init failed!");
			while (1);
		} else {
			CURRENT_TASK() = &task[0];
		}
	}
	sp = kmalloc(2048);
	if (sp)
		kthread_create(cmd_loop, sp, 5);
	task_timer_init();
	set_irq_handler(INT_TIMER1, do_timer);
	INTMS_set(INT_TIMER1);
	task_start(CURRENT_TASK()->context.sp, CURRENT_TASK()->context.pc);
}
static void task0() {
	trace(KERN_DEBUG, "task0:init\n");
	while (1);
}

__u32 get_cpsr() {
	int ret;
	asm (
	    "mrs %0,cpsr"
	    :"=r"(ret)
	);
	return ret;
}
void kthread_exit() {
	if (CURRENT_TASK()) {
		CURRENT_TASK()->state = TASK_UNALLOCATE;
		kfree(CURRENT_TASK()->stack);
	}
}
/*******************************************************************************
* 创建新进程函数
**********************************************************************************/
int kthread_create(unsigned long pc, void *data, long priority) {
	unsigned long i, pid = -1;

	printf("kernel:kthread_create\n");
	/*
	* 检查用户程序是否符合程序调用规格，所有用户程序第一条指令为：ldr	r0, [sp]
	* 其对应机器码为:0xe59d0000，如果加载的是非法程序，则出错退出
	*/
	// 为新进程挑选可用pid
	for (i = 0; i < NR_TASK; i++) {
		if ((task[i].state == TASK_UNALLOCATE) ) {
			pid = i;
			break;
		}
	}
	// 如果没有可用Pid，出错，退出
	if (pid == -1) {
		trace(KERN_CRIT, "task has to max number!\r\n");
		return -1;
	}
	void *sp = kmalloc(512);
	if (!sp) {
		trace(KERN_CRIT, "memory allocation failed for the new task!");
		return -1;
	}
	// 进入进程创建，此过程中不能被中断打断
	//OS_ENTER_CRITICAL();
	// -----------------以上为程序执行时所带参数处理-----------------------------------
	runningCount++;
	task[pid].pid = pid;					// 新进程PID
	task[pid].state = TASK_RUNNING;			// 新进程执行状态
	task[pid].count = 5;					// 新进程时间片
	task[pid].priority = priority;			// 新进程优先级
	task[pid].context.cpsr = get_cpsr();
	task[pid].context.sp = sp; // SP栈指针
	task[pid].stack = sp;
	task[pid].context.lr = kthread_exit;	// LR返回地址
	task[pid].context.pc = pc;				// PC
	task[pid].pwd = get_root_inode();				// PWD
	// 打开中断
	//OS_EXIT_CRITICAL();
	return pid;
}
