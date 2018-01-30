/*
 * init.c: 进行一些初始化
 */ 

#include "s3c24xx.h"
 
void disable_watch_dog(void);
void clock_init(void);
void memsetup(void);
void copy_steppingstone_to_sdram(void);
void clean_bss(void);

/*
 * 关闭WATCHDOG，否则CPU会不断重启
 */
void disable_watch_dog(void)
{
    WTCON = 0;  // 关闭WATCHDOG很简单，往这个寄存器写0即可
}

#define FCLK        200000000
#define HCLK        100000000
#define PCLK        50000000
#define S3C2410_MPLL_200MHZ     ((0x5c<<12)|(0x04<<4)|(0x00))
#define S3C2440_MPLL_200MHZ     ((0x5c<<12)|(0x01<<4)|(0x02))
#define S3C2440_MPLL_400MHZ     ((0x5c<<12)|(0x01<<4)|(0x01))
#define S3C2440_UPLL_48MHZ      ((0x38<<12)|(0x02<<4)|(0x02))
#define S3C2440_UPLL_96MHZ      ((0x38<<12)|(0x02<<4)|(0x01))
/*
 * 对于MPLLCON寄存器，[19:12]为MDIV，[9:4]为PDIV，[1:0]为SDIV
 * 有如下计算公式：
 *  S3C2410: MPLL(FCLK) = (m * Fin)/(p * 2^s)
 *  S3C2410: MPLL(FCLK) = (2 * m * Fin)/(p * 2^s)
 *  其中: m = MDIV + 8, p = PDIV + 2, s = SDIV
 * 对于本开发板，Fin = 12MHz
 * 设置CLKDIVN，令分频比为：FCLK:HCLK:PCLK=1:4:8，
 * FCLK=400MHz,HCLK=100MHz,PCLK=50MHz
 */
void clock_init(void)
{
    // LOCKTIME = 0x00ffffff;   // 使用默认值即可
    //CLKDIVN  = 0x03;            // FCLK:HCLK:PCLK=1:2:4, HDIVN=1,PDIVN=1
	CLKDIVN  = 0x05;            // FCLK:HCLK:PCLK=1:4:8
    /* 如果HDIVN非0，CPU的总线模式应该从“fast bus mode”变为“asynchronous bus mode” */
	__asm__ volatile (
		"mrc    p15, 0, r1, c1, c0, 0\n"        /* 读出控制寄存器 */ 
		"orr    r1, r1, #0xc0000000\n"          /* 设置为“asynchronous bus mode” */
		"mcr    p15, 0, r1, c1, c0, 0\n"        /* 写入控制寄存器 */
		:::"r1"
    );
	/*
	当你同时设置 MPLL 和 UPLL 的值时，你必须首先设置 UPLL 值再设置 MPLL 值。（大约需要 7 个 NOP 的间隔）
	*/
	//UPLLCON = S3C2440_UPLL_48MHZ;
	
	MPLLCON = S3C2440_MPLL_400MHZ;  /* 现在，FCLK=400MHz,HCLK=100MHz,PCLK=50MHz */    
}
/*
 * 启动ICACHE
 */
void enable_ICACNE(void)
{
    __asm__ volatile (
		"mrc    p15, 0, r0, c1, c0, 0\n"		/* 读出控制寄存器 */ 
		"orr    r0, r0, #(1<<12)\n"
		"mcr    p15, 0, r0, c1, c0, 0\n"	/* 写入控制寄存器 */
		:::"r0"
    );
}
/*
 * 设置存储控制器以使用SDRAM
 */
void memsetup(void)
{
    volatile unsigned long *p = (volatile unsigned long *)MEM_CTL_BASE;

    /* 这个函数之所以这样赋值，而不是像前面的实验(比如mmu实验)那样将配置值
     * 写在数组中，是因为要生成”位置无关的代码”，使得这个函数可以在被复制到
     * SDRAM之前就可以在steppingstone中运行
     */
    /* 存储控制器13个寄存器的值 */
    p[0] = 0x22011110;     //BWSCON
    p[1] = 0x00000700;     //BANKCON0
    p[2] = 0x00000700;     //BANKCON1
    p[3] = 0x00000700;     //BANKCON2
    p[4] = 0x00000700;     //BANKCON3  
    p[5] = 0x00000700;     //BANKCON4
    p[6] = 0x00000700;     //BANKCON5
    p[7] = 0x00018005;     //BANKCON6
    p[8] = 0x00018005;     //BANKCON7
    
                                    /* REFRESH,
                                     * HCLK=12MHz:  0x008C07A3,
                                     * HCLK=100MHz: 0x008C04F4
                                     */ 
    p[9]  = 0x008C04F4;
    p[10] = 0x000000B1;     //BANKSIZE
    p[11] = 0x00000030;     //MRSRB6
    p[12] = 0x00000030;     //MRSRB7
}

void copy_steppingstone_to_sdram(void)
{
    unsigned int *pdwSrc  = (unsigned int *)0;
    unsigned int *pdwDest = (unsigned int *)0x30000000;
    
    while (pdwSrc < (unsigned int *)4096)
    {
        *pdwDest = *pdwSrc;
        pdwDest++;
        pdwSrc++;
    }
}

void clean_bss(void)
{
    extern int __bss_start, __bss_end;
    int *p = &__bss_start;
    
    for (; p < &__bss_end; p++)
        *p = 0;
}

int is_boot_from_nor_flash(void) {
	volatile int *p = (volatile int *)0;
	int val;

	val = *p;
	*p = 0x12345678;
	if (*p == 0x12345678) {
		/* 写成功, 是nand启动 */
		*p = val;
		return 0;
	} else {
		/* NOR不能像内存一样写 */
		return 1;
	}
}

int copy_code_to_sdram(unsigned char *buf,unsigned int *addr , unsigned int len)
{
    extern void nand_read_ll(unsigned char *buf,unsigned int addr , unsigned int len);
	int i = 0;

	/* 如果是NOR启动 */
	if (is_boot_from_nor_flash()) {
		while (i < len) {
			buf[i] = addr[i];
			i++;
		}
	} else {
		//nand_init();
		nand_read_ll(buf, (unsigned int)addr, len);
	}
    return 0;
}

/*
 * 设置页表
 */
void create_page_table(void)
{

/* 
 * 用于段描述符的一些宏定义
 */ 
#define MMU_FULL_ACCESS     (3 << 10)   /* 访问权限 */
#define MMU_DOMAIN          (0 << 5)    /* 属于哪个域 */
#define MMU_SPECIAL         (1 << 4)    /* 必须是1 */
#define MMU_CACHEABLE       (1 << 3)    /* cacheable */
#define MMU_BUFFERABLE      (1 << 2)    /* bufferable */
#define MMU_SECTION         (2)         /* 表示这是段描述符 */
#define MMU_SECDESC         (MMU_FULL_ACCESS | MMU_DOMAIN | MMU_SPECIAL | \
                             MMU_SECTION)
#define MMU_SECDESC_WB      (MMU_FULL_ACCESS | MMU_DOMAIN | MMU_SPECIAL | \
                             MMU_CACHEABLE | MMU_BUFFERABLE | MMU_SECTION)
#define MMU_SECTION_SIZE    0x00100000
#define MUM_TLB_BASE_ADDR	0x30000000
#define MUM_SECTION_PADDR_BASE_MASK	(0xfff00000)
#define PHYSICAL_MEM_ADDR	0x30000000
#define VIRTUAL_MEM_ADDR	0x30000000
#define MEM_MAP_SIZE		0x4000000
#define PHYSICAL_IO_ADDR	0x48000000
#define VIRTUAL_IO_ADDR		0x48000000
#define IO_MAP_SIZE			0x18000000

    unsigned long virtuladdr, physicaladdr;
    volatile unsigned long *mmu_tlb_base = (volatile unsigned long *)MUM_TLB_BASE_ADDR;
    
    /*
     * Steppingstone的起始物理地址为0，第一部分程序的起始运行地址也是0，
     * 为了在开启MMU后仍能运行第一部分的程序，
     * 将0～1M的虚拟地址映射到同样的物理地址
     */
    virtuladdr = 0;
    physicaladdr = 0;
    mmu_tlb_base[virtuladdr >> 20] = (physicaladdr & MUM_SECTION_PADDR_BASE_MASK) | \
                                            MMU_SECDESC_WB;

    /*
     * 0x48000000是特殊寄存器的起始物理地址，
     * 将虚拟地址0x48000000～0x5FFFFFFF映射到物理地址0x48000000～0x5FFFFFFF上，
     */
    virtuladdr = VIRTUAL_IO_ADDR;
    physicaladdr = PHYSICAL_IO_ADDR;
	while (virtuladdr < VIRTUAL_IO_ADDR+IO_MAP_SIZE)
    {
        mmu_tlb_base[virtuladdr >> 20] = (physicaladdr & MUM_SECTION_PADDR_BASE_MASK) | \
                                            MMU_SECDESC;
        virtuladdr += MMU_SECTION_SIZE;
        physicaladdr += MMU_SECTION_SIZE;
    }
	
    /*
     * SDRAM的物理地址范围是0x30000000～0x33FFFFFF，
     * 将虚拟地址0x30000000～0x33FFFFFF映射到物理地址0x30000000～0x33FFFFFF上，
     * 总共64M，涉及64个段描述符
     */
    virtuladdr = VIRTUAL_MEM_ADDR;
    physicaladdr = PHYSICAL_MEM_ADDR;
    while (virtuladdr < VIRTUAL_MEM_ADDR+MEM_MAP_SIZE)
    {
        mmu_tlb_base[virtuladdr >> 20] = (physicaladdr & MUM_SECTION_PADDR_BASE_MASK) | \
                                                MMU_SECDESC_WB;
        virtuladdr += MMU_SECTION_SIZE;
        physicaladdr += MMU_SECTION_SIZE;
    }
}

/*
 * 启动MMU
 */
void mmu_init(void)
{
    unsigned long ttb = 0x30000000;

	__asm__ (
		"mov    r0, #0\n"
		"mcr    p15, 0, r0, c7, c7, 0\n"    /* 使无效ICaches和DCaches */
		
		"mcr    p15, 0, r0, c7, c10, 4\n"   /* drain write buffer on v4 */
		"mcr    p15, 0, r0, c8, c7, 0\n"    /* 使无效指令、数据TLB */
		
		"mov    r4, %0\n"                   /* r4 = 页表基址 */
		"mcr    p15, 0, r4, c2, c0, 0\n"    /* 设置页表基址寄存器 */
		
		"mvn    r0, #0\n"                   
		"mcr    p15, 0, r0, c3, c0, 0\n"    /* 域访问控制寄存器设为0xFFFFFFFF，
											 * 不进行权限检查 
											 */    
		/* 
		 * 对于控制寄存器，先读出其值，在这基础上修改感兴趣的位，
		 * 然后再写入
		 */
		"mrc    p15, 0, r0, c1, c0, 0\n"    /* 读出控制寄存器的值 */
		
		/* 控制寄存器的低16位含义为：.RVI ..RS B... .CAM
		 * R : 表示换出Cache中的条目时使用的算法，
		 *     0 = Random replacement；1 = Round robin replacement
		 * V : 表示异常向量表所在的位置，
		 *     0 = Low addresses = 0x00000000；1 = High addresses = 0xFFFF0000
		 * I : 0 = 关闭ICaches；1 = 开启ICaches
		 * R、S : 用来与页表中的描述符一起确定内存的访问权限
		 * B : 0 = CPU为小字节序；1 = CPU为大字节序
		 * C : 0 = 关闭DCaches；1 = 开启DCaches
		 * A : 0 = 数据访问时不进行地址对齐检查；1 = 数据访问时进行地址对齐检查
		 * M : 0 = 关闭MMU；1 = 开启MMU
		 */
		
		/*  
		 * 先清除不需要的位，往下若需要则重新设置它们    
		 */
											/* .RVI ..RS B... .CAM */ 
		"bic    r0, r0, #0x3000\n"          /* ..11 .... .... .... 清除V、I位 */
		"bic    r0, r0, #0x0300\n"          /* .... ..11 .... .... 清除R、S位 */
		"bic    r0, r0, #0x0087\n"          /* .... .... 1... .111 清除B/C/A/M */

		/*
		 * 设置需要的位
		 */
		"orr    r0, r0, #0x0002\n"          /* .... .... .... ..1. 开启对齐检查 */
		"orr    r0, r0, #0x0004\n"          /* .... .... .... .1.. 开启DCaches */
		"orr    r0, r0, #0x1000\n"          /* ...1 .... .... .... 开启ICaches */
		"orr    r0, r0, #0x0001\n"          /* .... .... .... ...1 使能MMU */
		
		"mcr    p15, 0, r0, c1, c0, 0\n"    /* 将修改的值写入控制寄存器 */
		: /* 无输出 */
		: "r" (ttb)
		: "r0","r4"
	);
}
