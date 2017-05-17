#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "lib.h"
#include "i8259.h"

#define BUFFER_SIZE 128
#define TERMINAL_NUM 3
#define DISP_OFF 0xFF
#define HB_OFF 0x0C
#define LB_OFF 0x0D

/* Terminal handler */
extern int32_t terminal_read(int32_t fd, uint8_t* buf, int32_t nbytes);
extern int32_t terminal_write(int32_t fd, const uint8_t* buf, int32_t nbytes);
extern int32_t terminal_open();
extern int32_t terminal_close(int32_t fd);
extern void keybrd_init();
extern int32_t can_read;

//int can_read;
uint8_t key_buf[BUFFER_SIZE][TERMINAL_NUM];

int x_screen[TERMINAL_NUM]; //the x coordinate in the screen (as defined in lib.c)
int y_screen[TERMINAL_NUM]; //the y coordinate in the screen (as defined in lib.c)


/* KBD handler helper functions */
extern void screen_switch(uint32_t target_screen);
void printkbd(uint8_t c);
void handle_scrolling(int type);
void update_cursor(int row, int col);
void print_backspace();
void handle_wrap_around(int type);
void clearScreen();
void clearBuffer();
//void printStr(uint8_t* strBuf);
void printC(uint8_t keystroke);
void printBuf(uint8_t* buf);
//void printBufFile(uint32_t keystroke);
#endif


