#include "i8259.h"
#include "lib.h"
#include "rtc.h"

//Set all the nesessary bits in control register A and B and write to CMOS port
//Input: none
//Output: none
void rtc_init(void)
{
	outb(Control_A, RTC_port);                  //tell RTC which register we are dealing with
	unsigned char a = inb(CMOS_port);

	outb(Control_B, RTC_port);
	unsigned char b = inb(CMOS_port);

	outb(Control_A, RTC_port);
	outb((UIP_mask & a) | DV_RS, CMOS_port); // set frequence to 2Hz and turn on the oscillator

	outb(Control_B, RTC_port);
	outb((b | Control_B_mask), CMOS_port);  // set DSE, 24 hours, Binary data, Square wave and Periodic interrupt 
	enable_irq(RTC_IRQ);
}
/* rtc_open:
 * 		DESCRIPTION:  enable irq
 *      INPUT:        none
 *      OUTPUT:       0
 */
int rtc_open(int32_t fd, int8_t* buf, int32_t nbytes)
{
	rtc_init();
	return 0;
}
/* rtc_close:
 * 		DESCRIPTION:  disable interrupt
 *      INPUT:        none
 *      OUTPUT:       0
 */
int rtc_close(int32_t fd, int8_t* buf, int32_t nbytes)
{
	disable_irq(RTC_IRQ);
	return 0;
}
/* rtc_read:
 * 		DESCRIPTION:  block until interrupt
 *      INPUT:        none
 *      OUTPUT:       0
 */
int rtc_read(int32_t fd, int8_t* buf, int32_t nbytes)
{
	while (rtc_flag);
	rtc_flag = 1;
	return 0;
}
/* rtc_write:
 * 		DESCRIPTION:  change the clock rate
 *      INPUT:        none
 *      OUTPUT:       0
 */
int rtc_write(int32_t fd, void* buf, int32_t nbytes)
{
	int RS;
	outb(Control_A, RTC_port);
	unsigned char a = inb(CMOS_port);  //Get orginal data from RTC
    int32_t freq =  *((int32_t*)buf);
	switch (freq) {
	case 1024:
		RS = 6;
		break;
	case 512:
		RS = 7;
		break;
	case 256:
		RS = 8;
		break;
	case 128:
		RS = 9;
		break;
	case 64:
		RS = 10;
		break;
	case 32:
		RS = 11;
		break;
	case 16:
		RS = 12;
		break;
	case 8:
		RS = 13;
		break;
	case 4:
		RS = 14;
		break;
	case 2:
		RS = 15;
		break;
	default:
		return -1;
	}
	outb(Control_A, RTC_port);
	outb((a & RS_mask) | RS, CMOS_port); // set frequence to RS Hz
	return 4;
}



