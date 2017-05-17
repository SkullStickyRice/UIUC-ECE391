#include "terminal.h"
#include "interrupt_handlers.h"
#include "lib.h"
#include "scheduling.h"

#define NUM_COLS 80
#define NUM_ROWS 25
#define TSIZE   80*25
#define SCREEN_SIZE 4096
#define ATTRIB 0x7			//refer to lib.c
#define VIDEO 0xB8000
#define VGA_BASE1 0x3D4		//VGA reg
#define VGA_BASE2 0x3D5
#define KB_IRQ   1
#define BIT_MASK 0xFF		//used in update the location of cursor
#define CUR_L 0x0F
#define CUR_H 0x0E
#define END_OF_BUF '\0'
#define KEY_LIMIT 127


static char* video_mem = (char *)VIDEO; 
static int keycount = 0;      // count the key entered into buffer
uint8_t key_buf[BUFFER_SIZE][3];
int32_t can_read;
static uint8_t temp;
static uint8_t pre_ATTRIB;
static int32_t s;


/*
*   Function: screen_switch(unit32_t target_screen)
*   Description: 
*   inputs: target_sceen
*   outputs: none
*   effects: 
*/
void screen_switch(uint32_t target_screen)
{
	cli();
	//calculating the high byte and display to the screen
	uint8_t high_byte = (SCREEN_SIZE*target_screen >> 8) & DISP_OFF;
	outb(HB_OFF, VGA_BASE1);
	outb(high_byte, VGA_BASE2);
	//calculating the low byte and display to the screen
	uint8_t low_byte = (SCREEN_SIZE*target_screen) & DISP_OFF;
	outb(LB_OFF, VGA_BASE1);
	outb(low_byte, VGA_BASE2);

	*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + (s << 1)) = temp;
	*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + (s << 1)+1) = pre_ATTRIB;
	display_index = target_screen;

	//call paging and set up new pages for video mem
	vid_new(VIDEO_MEM + 2 * SCREEN_SIZE * display_index, display_index);
	update_cursor(y_screen[display_index],x_screen[display_index]);
	sti();
	//send_eoi(1);
}
/*
* terminal_read
*   DESCRIPTION: read count bytes from keyboard
*   INPUTS: int32_t fd, uint8_t* buf, int32_t nbytes
*   OUTPUTS: none
*   RETURN VALUE: int32_t count
*   SIDE EFFECTS: fill the buf with content in out_buf
*/
int32_t terminal_read(int32_t fd, uint8_t* buf, int32_t nbytes) 
{
	can_read = 0;
	while (!can_read || display_index != cur_index);
	can_read = 0;

	uint32_t i, count;
	if (nbytes <= 0)		//when nothing to be read
		return -1;
	count = 0;
	for (i = 0; i < nbytes && i<KEY_LIMIT; i++) {	//fill the buf 
		buf[i] = key_buf[i][display_index];
		if(buf[i]=='\n')
			break;
		count++;
	}
	count++;
	clearBuffer();
	return count;
}

/*
* terminal_write
*   DESCRIPTION: write data to the terminal
*   INPUTS: int32_t fd, uint8_t* buf, int32_t nbytes
*   OUTPUTS: none
*   RETURN VALUE: int32_t count
*   SIDE EFFECTS: print the keyboard command to the terminal
*/
int32_t terminal_write(int32_t fd, const uint8_t* buf, int32_t nbytes) 
{
	if ((buf == NULL) || (nbytes < 0))	//if the buffer given to write is NULL or the end is reached, return -1
		return -1;

	uint32_t i;
	for (i = 0; i < nbytes; i++)
		printC(buf[i]);					// print data stored in user buffer to screen
	return nbytes;
}

/*
* terminal_open
*   DESCRIPTION: init the devices, enable irq, clear buffer
*   INPUTS: uint8_t * filename    
*   OUTPUTS: none
*   RETURN VALUE: 0
*   SIDE EFFECTS: none
*/
int32_t terminal_open() 
{
	return 0; 	
}

/*
* terminal_close
*   DESCRIPTION: close terminal
*   INPUTS: fd (file descriptor number)
*   OUTPUTS: none
*   RETURN VALUE: 0
*   SIDE EFFECTS: none
*/
int32_t terminal_close(int32_t fd) 
{
	return -1;		
}

/*
* keybrd_init
*   DESCRIPTION: clear the screen and buffer
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: none
*/
void keybrd_init()
{
	int i, j;
	display_index = 0;
	for (i = 0; i < BUFFER_SIZE; i++) {
		for (j = 0; j < 3; j++) {
			//out_buf[i] = 0;			
			key_buf[i][j] = 0;
		}
	}
	can_read = 0;
	return;
}

/* ------------------Helper function for interreupt_handler for Keyboard ------------------------------------ */
/*
*   void printkbd(uint8_t keystroke)
*   DESCRIPTION: print a character on screen with limit 128
*   INPUTS: keystroke: key command to print on the terminal
*   OUTPUTS: 
*   RETURN VALUE: void
*	SIDE EFFECTS : none
*/
void
printkbd(uint8_t keystroke)  //type is 1: running (0 means displaying)
{
	if (keystroke == '\n' || keystroke == '\r') { //newline char encountered
		if(key_buf[0][display_index] != 0) {
			keycount++;
			key_buf[keycount-1][display_index] = keystroke;
		}
		x_screen[display_index] = 0;
		y_screen[display_index] ++;
		if (y_screen[display_index] == NUM_ROWS)   //when y reaches the bottom of the screen
			handle_scrolling(1);
		update_cursor(y_screen[display_index], 0); //change the cursor location accordingly
		return;
	}
	if((keycount+1)>=BUFFER_SIZE-1)
		return;
	keycount++;
	key_buf[keycount-1][display_index] = keystroke;
	//refer to lib.c
	*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + ((NUM_COLS*y_screen[display_index] + x_screen[display_index]) << 1)) = keystroke;
	*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + ((NUM_COLS*y_screen[display_index] + x_screen[display_index]) << 1) + 1) = ATTRIB;
	x_screen[display_index] ++;
	if (x_screen[display_index] == NUM_COLS)	//when x reaches the rightmost end of screen
		handle_wrap_around(1);
	update_cursor(y_screen[display_index], x_screen[display_index]);
}


/*
* handle_scrolling()
*   DESCRIPTION: enable the screen to be scrolled down when cursor reaches the bottom
*   INPUTS : none
*   OUTPUTS : none
*	RETURN VALUE : none
*	SIDE EFFECTS : none
*/
void
handle_scrolling(int type)
{
	if (type == 0) {
		int32_t i;
		for (i = 0; i < (NUM_ROWS - 1) * NUM_COLS; i++) {		//shift video mem 1 row upwards
			*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*cur_index + (i << 1)) = *(uint8_t *)(video_mem + 2 * SCREEN_SIZE*cur_index + ((i + NUM_COLS) << 1));
			*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*cur_index + (i << 1) + 1) = ATTRIB;
		}
		for (i = (NUM_ROWS - 1) * NUM_COLS; i < TSIZE; i++) {	//empty the last row with all " " (space)
			*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*cur_index + (i << 1)) = ' ';
			*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*cur_index + (i << 1) + 1) = ATTRIB;
		}
		y_screen[cur_index]--;
		return;
	}
	else {
		int32_t i;
		for (i = 0; i < (NUM_ROWS - 1) * NUM_COLS; i++) {		//shift video mem 1 row upwards
			*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + (i << 1)) = *(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + ((i + NUM_COLS) << 1));
			*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + (i << 1) + 1) = ATTRIB;
		}
		for (i = (NUM_ROWS - 1) * NUM_COLS; i < TSIZE; i++) {	//empty the last row with all " " (space)
			*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + (i << 1)) = ' ';
			*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + (i << 1) + 1) = ATTRIB;
		}
		y_screen[display_index]--;
		return;
	}
	
}

/*
* update_cursor(int row, int col)
*   DESCRIPTION: update the cursor to its destination location
*   INPUTS : row, col for the destination 
*   OUTPUTS : none
*	RETURN VALUE : none
*	SIDE EFFECTS : change the location of the cursor "_"
*/
void
update_cursor(int row, int col)
{
	unsigned short position = (NUM_COLS * row) + col;
	unsigned char input;
	input = position & BIT_MASK;
	outb(CUR_L, VGA_BASE1);	// connect cursor vga register
	outb(input, VGA_BASE2);

	input = (position>>8) & BIT_MASK;
	outb(CUR_H, VGA_BASE1);  
	outb(input, VGA_BASE2);
}

/*
* print_backspace()
*   DESCRIPTION: change the position of cursor and print ' ' if user types backspace
*   INPUTS : none
*   OUTPUTS : none
*	RETURN VALUE : none
*	SIDE EFFECTS : print ' ' to the screen
*/
void
print_backspace()
{
	if(keycount==0)
		return;
	if(key_buf[0][display_index] == '\0')
		return;
	keycount--;
	key_buf[keycount][display_index] = 0;
	
	if (x_screen[display_index] == 0) {	//case when x is at the leftmost of the terminal
		y_screen[display_index]--;
		x_screen[display_index] = NUM_COLS - 1;
	}
	else
		x_screen[display_index] --;		//update x_screen to be 1 less the original value
	//refer to lib.c
	*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + ((NUM_COLS*y_screen[display_index] + x_screen[display_index]) << 1)) = ' ';
	*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + ((NUM_COLS*y_screen[display_index] + x_screen[display_index]) << 1) + 1) = ATTRIB;
	update_cursor(y_screen[display_index], x_screen[display_index]);			//update the cursor to the previous position
}

/*
* handle_wrap_around()
*   DESCRIPTION: change the position of cursor when x reaches the right most screen
*   INPUTS : none
*   OUTPUTS : none
*	RETURN VALUE : none
*	SIDE EFFECTS : none
*/
void
handle_wrap_around(int type)	//when the cursor reaches the end of terminal
{
	if (type == 0) {
		y_screen[cur_index]++;
		if (y_screen[cur_index] == NUM_ROWS)		//call handle scrolling when cursor reaches the bottom chck
			handle_scrolling(type);
		x_screen[cur_index] %= NUM_COLS;
		y_screen[cur_index] = (y_screen[cur_index] + (x_screen[cur_index] / NUM_COLS)) % NUM_ROWS;
		return;
	}
	else {
		y_screen[display_index]++;
		if (y_screen[display_index] == NUM_ROWS)		//call handle scrolling when cursor reaches the bottom chck
			handle_scrolling(type);
		x_screen[display_index] %= NUM_COLS;
		y_screen[display_index] = (y_screen[display_index] + (x_screen[display_index] / NUM_COLS)) % NUM_ROWS;
		return;
	}
	return;
}

/*
* clearScreen()
*   DESCRIPTION: clear the terminal and set x_screen y_screen to (0,0)
*   INPUTS : none
*   OUTPUTS : none
*	RETURN VALUE : none
*	SIDE EFFECTS : none
*/
void
clearScreen()
{
	int32_t i;
	for (i = 0; i < TSIZE; i++) { //refer to lib.c
		*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + (i << 1)) = ' ';
		*(uint8_t *)(video_mem + 2 * SCREEN_SIZE*display_index + (i << 1) + 1) = ATTRIB;
	}
	clearBuffer();
	keycount=0;
	x_screen[display_index] = 0;
	y_screen[display_index] = 0;
	return;
}

void clearBuffer()
{
	int32_t i;
	for(i=0; i<keycount; i++)
		key_buf[i][display_index] = 0;
	keycount = 0;
}

void
printC(uint8_t keystroke)
{
	if (keystroke == '\n' || keystroke == '\r') { //newline char encountered
		x_screen[cur_index] = 0;
		y_screen[cur_index]++;
		if (y_screen[cur_index] == NUM_ROWS)   //when y reaches the bottom of the screen
			handle_scrolling(0);
		update_cursor(y_screen[cur_index], 0); //change the cursor location accordingly
	}
	//refer to lib.c
	else {
		*(uint8_t *)(video_mem + 2 *SCREEN_SIZE*cur_index + ((NUM_COLS*y_screen[cur_index] + x_screen[cur_index]) << 1)) = keystroke;
		*(uint8_t *)(video_mem + 2 *SCREEN_SIZE*cur_index + ((NUM_COLS*y_screen[cur_index] + x_screen[cur_index]) << 1) + 1) = ATTRIB;
		x_screen[cur_index]++;
		if (x_screen[cur_index] == NUM_COLS)	//when x reaches the rightmost end of screen
			handle_wrap_around(0);
		update_cursor(y_screen[cur_index], x_screen[cur_index]);
	}
}

void printBuf(uint8_t* buf)
{
	int count = 0;
	register uint32_t index = 0;
	while (buf[index] != '\0') {
		if(++count > 32) break; //break when the buf is overflow
		printC(buf[index]);
		index++;
	}
}






