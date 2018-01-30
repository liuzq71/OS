#include "common.h"
#include "s3c24xx.h"

#define L3M (1<<2)
#define L3D (1<<2)
#define L3C (1<<2)
void udelay(int time) {
    while(time--);
}

static void wm8976_write_reg(unsigned char reg, unsigned int data)
{
	//时序控制，wm8976手册p16
    int i;
    unsigned short val = (reg << 9) | (data & 0x1ff);

    GPBDAT |= GPB_X_WRITE(2);
    GPBDAT |= GPB_X_WRITE(3);
    GPBDAT |= GPB_X_WRITE(4);

    for (i = 0; i < 16; i++){
        if (val & (1<<15))
        {
            GPBDAT &= GPX_X_CLEAR(4);
            GPBDAT |= GPB_X_WRITE(3);
            udelay(10);
            GPBDAT |= GPB_X_WRITE(4);
            //udelay(10);
        }
        else
        {
            GPBDAT &= GPX_X_CLEAR(4);
            GPBDAT &= GPX_X_CLEAR(3);
            udelay(10);
            GPBDAT |= GPB_X_WRITE(4);
            //udelay(10);
        }

        val = val << 1;
    }

    GPBDAT &= GPX_X_CLEAR(2);
    udelay(10);
    GPBDAT |= GPB_X_WRITE(2);
    GPBDAT |= GPB_X_WRITE(3);
    GPBDAT |= GPB_X_WRITE(4);
}

void init_wm8976(void)
{
    //uda1341_volume = 57;
    //uda1341_boost = 0;

    /* software reset */
    wm8976_write_reg(0, 0);

    /* OUT2的左/右声道打开
     * 左/右通道输出混音打开
     * 左/右DAC打开
     */
    wm8976_write_reg(0x3, 0x6f);

    wm8976_write_reg(0x1, 0x1f);//biasen,BUFIOEN.VMIDSEL=11b  
    wm8976_write_reg(0x2, 0x185);//ROUT1EN LOUT1EN, inpu PGA enable ,ADC enable

    wm8976_write_reg(0x6, 0x0);//SYSCLK=MCLK  
    wm8976_write_reg(0x4, 0x10);//16bit 		
    wm8976_write_reg(0x2B,0x10);//BTL OUTPUT  
    wm8976_write_reg(0x9, 0x50);//Jack detect enable  
    wm8976_write_reg(0xD, 0x21);//Jack detect  
    wm8976_write_reg(0x7, 0x01);//Jack detect 
    wm8976_write_reg(52, (1<<8)|57);
    wm8976_write_reg(53, (1<<8)|57);
}

void WriteL3Data(unsigned char regaddr, unsigned char data);

void Init8976(void)
{
	//----------------------------------------
	//         Port B Group
	//Ports:	GPB2	GPB3	GPB4
	//Signal:	L3MODE	L3DATA	L3CLOCK
	//Setting:	OUTPUT	OUTPUT	OUTPUT
	//		[9:8]	[7:6]	[5:4]
	//Binary:	01	01	01
	//----------------------------------------
	//GPBUP = GPBUP | (0x7<<2);//disable GPB[4:2] pull up function
	//GPBCON = GPBCON & ~(0x3f<<4)|(0x15<<4);
	//GPBDAT = GPBDAT & ~(L3M|L3D|L3C) | (L3M|L3C);//L3Mode=H, L3Clock=H
	WriteL3Data((0x3<<1)+1,0xef);//RMIXEN,LMIXEN,DACENR,DACENL    	
	WriteL3Data((0x1<<1)+0,0x1f);//biasen,BUFIOEN.VMIDSEL=11b
	WriteL3Data((0x2<<1)+1,0x80);//ROUT1EN LOUT1EN
	WriteL3Data((0x6<<1)+0,0x0);//SYSCLK=MCLK
	WriteL3Data((0x4<<1)+0,0x10);//16bit    	 
	WriteL3Data((0x2B<<1)+0,0x10);//BTL OUTPUT
	WriteL3Data((0x9<<1)+0,0x50);//Jack detect enable
	WriteL3Data((0xD<<1)+0,0x21);//Jack detect
	WriteL3Data((0x7<<1)+0,0x01);//Jack detect
}

void WriteL3Data(unsigned char regaddr, unsigned char data)
{
	int i, j;
	//start condition: L3M = High L3C = High
	GPBDAT |= L3M;
	GPBDAT |= L3C;
	GPBDAT |= L3M;
	GPBDAT |= L3C;
	for(i=0; i<100; i++);	//delay
	//control register address
	for(j=0; j<8; j++)
	{
		if(regaddr & 0x80)
		{
			GPBDAT &= ~L3C;
			GPBDAT |= L3D;
			for(i=0; i<10; i++);	//delay
			GPBDAT |= L3C;
			for(i=0; i<10; i++);	//delay
		}
		else
		{
			GPBDAT &= ~L3C;
			GPBDAT &= ~L3D;
			for(i=0; i<10; i++);	//delay
			GPBDAT |= L3C;
			for(i=0; i<10; i++);	//delay
		}
		regaddr = regaddr << 1;
	}
	//control register data bits
	for(j=0; j<8; j++)
	{
		if(data & 0x80)
		{
			GPBDAT &= ~L3C;
			GPBDAT |= L3D;
			for(i=0; i<10; i++);	//delay
			GPBDAT |= L3C;
			for(i=0; i<10; i++);	//delay
		}
		else
		{
			GPBDAT &= ~L3C;
			GPBDAT &= ~L3D;
			for(i=0; i<10; i++);	//delay
			GPBDAT |= L3C;
			for(i=0; i<10; i++);	//delay
		}
		data = data << 1;
	}
	GPBDAT &= ~L3M;
	for(i=0; i<1000; i++);
	GPBDAT |= L3M;
	GPBDAT |= L3C;
}
