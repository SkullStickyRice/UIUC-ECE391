#ifndef _INTERRUPT_HANDLERS_H
#define _INTERRUPT_HANDLERS_H

#include "i8259.h"
#include "lib.h"
#include "rtc.h"
#include "types.h"
#include "terminal.h"
#include "file_system_driver.h"
#include "scheduling.h"

#define K_NUM     				104
#define SPECIAL_CHAR            32
#define LOWCASE_ALP				26
#define UPCASE_ALP				26
#define NUMBER					10
#define FUNC_KEY      			10

#define CAPS_LOCK_PRESSED 		0x3A
#define CAPS_LOCK_RELEASED		0xBA

#define LEFT_SHIFT_PRESSED 		0x2A
#define LEFT_SHIFT_RELEASED		0xAA

#define RIGHT_SHIFT_PRESSED     0x36
#define RIGHT_SHIFT_RELEASED    0xB6

#define LEFT_CTRL_PRESSED       0x1D   // RIGHT_CTRL use the same scancode 
#define LEFT_CTRL_RELEASED      0x9D  

#define BACKSPACE_PRESSED       0x0E
#define BACKSPACE_RELEASED      0x8E

#define ENTER_PRESSED           0x1C
#define ENTER_RELEASED 			0x9C

#define TAB_PRESSED             0x0F
#define SPACE_PRESSED           0x39

#define ALT_PRESSED       		0x38
#define ALT_RELEASED            0xB8

#define ESC_PRESSED             0x01
#define ESC_RELEASED            0x81

#define ARROW_UP				0x48
#define ARROW_DOWN				0x50	
#define ARROW_LEFT				0x4B
#define ARROW_RIGHT				0x4D 		

#define TAB_SIZE                4
#define TAB_FORMAT              {'    '}

#define F1						0x3B
#define F2						0x3C
#define F3 						0x3D	


extern uint32_t display_index;



extern const unsigned char KEYBOARD_CHAR_TABLE[K_NUM];
extern void keyboard_handler(void);
extern void rtc_handler(void);
extern void pit_handler(void);

#endif







