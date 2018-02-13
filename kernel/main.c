#include <stdio.h>
#include <s3c24xx.h>
#include "serial.h"
#include "lcdlib.h"
#include "lcddrv.h"
#include "framebuffer.h"
int main() {
	Port_Init();
	uart0_init();							// 波特率115200，8N1(8个数据位，无校验位，1个停止位)
	
	
	Lcd_Port_Init();						// 设置LCD引脚
	Tft_Lcd_Init(MODE_TFT_16BIT_480272);	// 初始化LCD控制器
	Lcd_PowerEnable(0, 1);					// 设置LCD_PWREN有效，它用于打开LCD的电源
	Lcd_EnvidOnOff(1);						// 使能LCD控制器输出信号
	
	ClearScr(0x00);						// 清屏
	lcd_set_text_color(0xffffff);
	lcd_set_background_color(0x00);
	{
		printf("\n\n************************************************\n");
		printf("正在初始化UART...\n");
		printf("正在初始化GPIOs...\n");
		printf("正在初始化LCD controller...\n");
		
		printf("正在初始化mmu...\n");
		mmu_init();
		
		printf("正在初始化内存分配器...\n");
		init_page_map();
		kmalloc_init();
		
		printf("正在初始化nand flash驱动...\n");
		nand_init();
		
		printf("正在初始化定时器驱动...\n");
		timer_init();
		
		printf("正在初始化yaffs文件系统...\n");
		init_yaffs_fs();
		printf("正在开启中断...\n");
		enable_irq();
		printf("正在初始化usb slave...\n");
		usb_init_slave();
		task_init();
		//cmd_loop();
		while (1);
	}
	Lcd_EnvidOnOff(0);
	return 0;
}
