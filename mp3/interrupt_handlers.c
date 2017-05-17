#include "interrupt_handlers.h"

#define KEYBOARD_PORT  0x60
#define UPPER_CASE_OFFSET  52
#define CHAR_TABLE_CORRECTION  2



#define VGA_BASE1 0x3D4			// addr of VGA index register
#define VGA_BASE2 0x3D5

#define SCREEN_SIZE 4096
#define VIDEO_MEM 0xB8000
uint32_t display_index = 0;

//static char* video_mem = (char *)VIDEO_MEM;
const unsigned char KEYBOARD_CHAR_TABLE[K_NUM] = {
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', ' ', ' ',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', ' ', ' ',
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', ' ','\\',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', ' ', ' ', 
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', ' ', ' ', 
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', ' ', '|', 
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?'
};

static uint8_t capsFlag = 0;
static uint8_t lshiftFlag = 0;
static uint8_t rshiftFlag = 0;
static uint8_t lctrlFlag = 0;
static uint8_t altFlag = 0;
static int k_offset = 0;
//testFS
static uint32_t rtcPrintFlag = 0;


/*
 *  keyboard_handler:
 *      DESCRIPTION:	This function serves keyboard interrupt, scancode will be
 *                      sent to port 0x60 and echo out to the screen, special cases will be handled
 *      INPUT:          none
 *      OUTPUT:         none
 */
void keyboard_handler(void)
{
	int index;
	unsigned char scancode;
	scancode = inb(KEYBOARD_PORT);   // obtain scan code from port 0x60
	uint8_t input;
	switch(scancode)
	{
		case CAPS_LOCK_PRESSED:
			if(!capsFlag)
				capsFlag= 1;
			else
				capsFlag = 0;
			goto END_INTERRUPT;
		case LEFT_SHIFT_PRESSED:
			k_offset = UPPER_CASE_OFFSET;   
			lshiftFlag = 1;
			goto END_INTERRUPT;
		case LEFT_SHIFT_RELEASED:
			k_offset = 0;
			lshiftFlag = 0;
			goto END_INTERRUPT;
		case RIGHT_SHIFT_PRESSED:
			k_offset = UPPER_CASE_OFFSET;
			rshiftFlag = 1;
			goto END_INTERRUPT;
		case RIGHT_SHIFT_RELEASED:
			k_offset = 0;
			rshiftFlag = 0;
			goto END_INTERRUPT;
		case LEFT_CTRL_PRESSED:
			lctrlFlag = 1;
			goto END_INTERRUPT;
		case LEFT_CTRL_RELEASED:
			lctrlFlag = 0;
			goto END_INTERRUPT;
		case ALT_PRESSED:
			altFlag = 1;
			goto END_INTERRUPT;
		case ALT_RELEASED:
			altFlag = 0;
			goto END_INTERRUPT;
		default:
			break;
	}

	if (altFlag)
	{
		switch (scancode)
		{
		case F1:
			// function launch_terminal0
			send_eoi(1);
			screen_switch(0);
			break;
		case F2:
			// function launch_terminal1
			send_eoi(1);
			screen_switch(1);
			break;
		case F3:
			// function launch_terminal2
			send_eoi(1);
			screen_switch(2);
			break;
		default: break;

		}
		send_eoi(1);
		sti();
		return;
	}

	/* Space press */
	if(scancode==SPACE_PRESSED)
	{
		input = ' ';
		printkbd(input);
		send_eoi(1);
		return;
	}
	 /* Tab press */
	if(scancode==TAB_PRESSED)
	{
		uint32_t i;
		for(i=0; i<4; i++)
			printkbd(' ');
		send_eoi(1);
		return;
	}

	/* press BackSpace, delete the most recent input */
	if(scancode==BACKSPACE_PRESSED)
	{
		print_backspace();
		send_eoi(1);
		return;
	}

	/* press Enter */
	if(scancode==ENTER_PRESSED)
	{
		input = '\n';
		printkbd(input);
		can_read = 1;      //used in terminal_read
		send_eoi(1);
		return;
	}
	

	/* deal with CapsLock & Shift & Ctrl+L */	
	index = scancode - CHAR_TABLE_CORRECTION;  // get keyboard table index 
	if(index<K_NUM)
	{
		input = KEYBOARD_CHAR_TABLE[index];
		if(k_offset!=0)		// shift pressed
			index += k_offset;
		if(capsFlag)		// caps lock pressed 
		{
			if(input>=0x61&&input<=0x7A)
			{
				if(k_offset==0) // only capsLock pressed 
					index += UPPER_CASE_OFFSET;  
				if(lshiftFlag||rshiftFlag) // capsLock pressed & shift pressed 
					index -= k_offset;
			}
			
		}
		input = KEYBOARD_CHAR_TABLE[index];
		// ctrl+l, clean the screen 
		if(lctrlFlag && (input=='l'||input=='L'))
		{
			clearScreen();
			update_cursor(0,0);
			send_eoi(1);
			return;
		}
	
		printkbd(input);
	}
	END_INTERRUPT:
		send_eoi(1);
		return;
}

//rtc_handler 
//handle rtc interrupts
//Input: none
//Output: none
void rtc_handler(void)
{
	outb(Control_C, RTC_port);
	inb(CMOS_port);                //dump the data
	send_eoi(RTC_IRQ);
	rtc_flag = 0;
	if (rtcPrintFlag == 1)			//when the ctrl 4 has been pressed
		printC('1');
}

void pit_handler(void)
{
	scheduling();
}


