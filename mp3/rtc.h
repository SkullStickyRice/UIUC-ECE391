#ifndef _RTC_H
#define _RTC_H

#include "types.h"

#define DV_RS 0x2F
#define Control_A 0x8A
#define Control_B 0x8B
#define RTC_port 0x70
#define CMOS_port 0x71 
#define UIP_mask 0x80
#define RTC_IRQ 8
#define Control_B_mask 0x4F
#define Control_C 0x0C
#define RS_mask 0xF0

int rtc_flag;

// initiliaze rtc
void rtc_init(void);
int rtc_open(int32_t fd, int8_t* buf, int32_t nbytes);
int rtc_close(int32_t fd, int8_t* buf, int32_t nbytes);
int rtc_read(int32_t fd, int8_t* buf, int32_t nbytes);
int rtc_write(int32_t fd, void* buf, int32_t nbytes);

#endif

