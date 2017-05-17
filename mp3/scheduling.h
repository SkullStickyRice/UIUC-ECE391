#ifndef _SCHEDULING_H
#define _SCHEDULING_H

#include "syscall.h"
#include "i8259.h"
#include "lib.h"
#include "paging.h"
#include "terminal.h"
#include "x86_desc.h"

/* */
#define PIT_COMMAND_REG				0x43
#define PIT_SQUARE_WAVE_MODE 		0x36
#define PIT_CHANNEL_0 				0x40
#define DIVISOR_MASK                0xFF

//#define VIDMEM_136MB    0x8800000
#define CR0_BIT31       0x80000000
#define EIGHT_MB        0x0800000
#define EIGHT_KB        0x2000
#define FOUR_MB 		0x0400000 
#define SCREEN_SIZE 4096
#define VIDEO_MEM 0xB8000
#define _136MB 0x8800000
#define dividor_num 11932
/* Divisors for PIT Frequency setting 
 * HZ = 1193180 / HZ_VALUE (ex: HZ = 1193180 / 20);  
 */	
int terminal_pid[3];
//extern volatile uint8_t curr_term;


int cur_index;
int next_index;
int init_flag;

/* Initialize RTC */
void PIT_init(void);

void scheduling(void);
void sche_init();
void calculate_pcbaddr();
void terminal_set();
void move_registers_out();
void move_registers_in();


#endif
