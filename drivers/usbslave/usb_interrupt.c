#include <common.h>
#if defined(CONFIG_S3C2400)
#include <s3c2400.h>
#elif defined(CONFIG_S3C2410) || defined (CONFIG_S3C2440)
#include <s3c2410.h>
#endif
#include "common_usb.h"

/* Start : add by www.100ask.net */
void (*isr_handle_array[50])(void);
S3C24X0_INTERRUPT * intregs;

extern void IsrUsbd(void);
extern void IsrDma2(void);


extern void Dummy_isr(void);




void USB_ISR_Init(void) {
	intregs = S3C24X0_GetBase_INTERRUPT();
	isr_handle_array[ISR_USBD_OFT] = IsrUsbd;
	isr_handle_array[ISR_DMA2_OFT] = IsrDma2;
	ClearPending(BIT_DMA2);
	ClearPending(BIT_USBD);
}


