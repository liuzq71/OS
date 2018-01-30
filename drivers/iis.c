#include "s3c24xx.h"
#include "common.h"

void init_IIS_bus()
{
	IISCON = 0;
	IISCON |= //IISCON_INTERFACE_ENABLE
			 IISCON_PRESCALER_ENABLE//IIS 预分频器使能
			 //|IISCON_TX_DMA
			 |IISCON_RX_IDLE//接收通道空闲命令:初始化为空闲
	//TODO:貌似有问题 // |IISCON_ENABLE_IIS;
			|IISCON_INTERFACE_DISABLE;//IIS 接口使能
	IISMOD = 0;
	//fs:采样频率
	IISMOD =IISMOD_SCLK_32FS//串行位时钟频率选择:32fs
			|IISMOD_MCLK_384FS//主时钟频率选择:384fs
			|IISMOD_SERIAL_BIT_PER_CH_16//串行数据每通道:16 位  
			|IISMOD_TXMOD//接收模式: MSB（左）对齐格式  
			|IISMOD_MSB_FORMAT;//串行接口格式 
	
	IISFCON = 0;
	IISFCON =  IISFCON_TX_ENABLE;//发送 FIFO:enable
		  //  |IISFCON_RX_FIFO_DMA
		   // |IISFCON_TX_FIFO_DMA; 
	IISPSR = (2<<5)|2;
	//预分频器 A:3,预分频器 B:3
}

void init_dma()
{

    //init DMA2
    DISRCC2 = (0<<1) + (0<<0);//the source is in the AHB, increment
    DIDST2 = (int)IISFIFO;//IISFIFO
    DIDSTC2 = (1<<1) + (1<<0);//the destination is in the APB, fixed
    //Handshake[31], Sync PCLK[30], CURR_TC Interrupt enable[29],Unit Transfer[28],Single Service Mode[27]
    //I2SSDO[26:24], DMA Source[23], Auto Reload[22], Half-Word[21:20], TC[19:0]
    DCON2 = (1<<31)+(0<<30)+(1<<29)+(0<<28)+(0<<27)+(0<<24)+(1<<23)+(1<<22)+(1<<20);
    //No-Stop[2], DMA channel is turned on[1], No-Trigger in S/W request mode[0]
    DMASKTRIG2 = (0<<2)+(1<<1)+(0<<0);
    INTMSK &= ~(1 << 19);
}

void init_gpio_L3_port()
{
	GPBCON |= GPB2_OUT|GPB3_OUT|GPB4_OUT; 
		//GPBUP != (GPB2_PULL_UP&GPB4_PULL_UP);
}

void init_IIS_port()
{
    /*[0-1] [2-3] [4-5] [6-7] [8-9]*/
	//*** PORT E GROUP
	//Ports  : GPE15  GPE14 GPE13   GPE12   GPE11   GPE10   GPE9    GPE8     GPE7  GPE6  GPE5   GPE4
	//Signal : IICSDA IICSCL SPICLK SPIMOSI SPIMISO SDDATA3 SDDATA2 SDDATA1 SDDATA0 SDCMD SDCLK I2SDO
	//Binary :  10     10  ,  10      10  ,  10      10   ,  10      10   ,   10    10  , 10     10  ,
	//-------------------------------------------------------------------------------------------------------
	//Ports  :  GPE3   GPE2  GPE1    GPE0
	//Signal :  I2SDI  CDCLK I2SSCKL I2SLRCK
	//Binary :  10     10  , 10      10
	//rGPECON |= 0x2aa;
	//rGPEUP  |= 0x1f;     // The pull up function is disabled GPE[15:0]
	GPECON |= GPE0_I2SLRCK|GPE1_I2SSCKL|GPE2_CDCLK|GPE3_I2SDI|GPE4_I2SDO;
	GPEUP |= GPE0_NOT_PULL_UP|GPE1_NOT_PULL_UP|GPE2_NOT_PULL_UP|GPE3_NOT_PULL_UP|GPE4_NOT_PULL_UP;
}
