#include "common.h"
#define IS_FIFO_READY (0x1<<7)
#define WAV_FILE_SIZE 882046
short big2small(short val)
{
   short ret;
   
   ret = ((val&0xff)<<8) |(val>>8);

   return ret;
}

static int mp3_play()
{
	short *pData = (short *)0x30200000;
	short *pData_end = pData + WAV_FILE_SIZE/2;
	int send_cnt = 0;

	printf("\n========== play ==========\n");
	//nand_read((unsigned char *)pData, 0x60000, 0x200000);
	printf("the head of wav : 0x%x\r\n", *pData);
	short conv = big2small(*pData);
	printf("after convert value is : 0x%x\r\n", conv);
	pData += 0x2e;//real data offset
	printf("after of offset of wav : 0x%x\r\n", *pData);
	while(1) {
			while (IISCON & IS_FIFO_READY);

			//printf("send the data 0x%x\r\n", (int)*pData);
			IISFIFO = *pData;//big2small(*pData);
			send_cnt = (IISFCON>>6)&0x3f;
 //           printf("send data count : %d\r\n", send_cnt);
  //          printf("read data count : %d\r\n", IISFCON&0x3f);
			if (pData == pData_end) {
					return 0;
			}
			pData++;
	}

	return 0;

}
void mplay(){
	init_IIS_port();
	init_gpio_L3_port();
	init_wm8976();
	init_IIS_bus();
	//Init8976();
	//init_dma();
	IISCON |= IISCON_INTERFACE_ENABLE;
	mp3_play();
}