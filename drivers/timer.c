#include <s3c24xx.h>
#include "interrupt.h"

void timer_init(void){
	// INTMS_clr(INT_TIMER4);
	// //定时器配制寄存器 0 （TCFG0）
	// TCFG0|=0xFF00;//定时器 2，3 和 4 的预分频值 
	// //定时器控制寄存器 1 （TCON）
	// TCON&=(~(7<<20));//清空20~21位
}
static volatile int delay_start = 0;
void delay_irq_hander();

void delay_u(int delay_time){
	INTMS_clr(INT_TIMER4);
	if(delay_time>65535)
		delay_time=65535;
	delay_start=1;
	
	
	//定时器配制寄存器 0 （TCFG0）
	TCFG0|=(24<<8);//定时器 2，3 和 4 的预分频值 
	//定时器控制寄存器 1 （TCON）
	TCON&=(~(7<<20));//清空20~21位
	TCON|=(0<<22);//定时器 4单稳态
	TCON|=(1<<21);//定时器 4手动更新 TCNTB4
	//TCONB4:定时器 4  计数缓冲寄存器
	TCNTB4=delay_time;

	TCON|=(1<<20);//启动
	TCON&=~(1<<21);//定时器 4 取消手动更新
	
	
	set_irq_handler(INT_TIMER4,delay_irq_hander);
	
	//TODO:1多进程中断嵌套,2总开关不能随便用
	INTMS_set(INT_TIMER4);
	while(delay_start!=0);
	INTMS_clr(INT_TIMER4);
}

void delay_irq_hander(){
	delay_start=0;
	INTMS_clr(INT_TIMER4);
	//TCON&=(~(1<<20));//定时器关闭
}