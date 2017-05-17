#include "scheduling.h"


volatile uint8_t curr_term = 0;
uint8_t next_term = 0;

int terminal_pid[];
int next_index;
pcb_t * cur_pcb; // current pcb
int init_flag;
int cur_index;
uint32_t esp, ebp;

/*
*   Function: PIT_init()
*   Description: initializes PIT
*   inputs: none
*   outputs: none
*   effects: 
*   background:
*       http://wiki.osdev.org/Programmable_Interval_Timer
*       http://www.osdever.net/bkerndev/Docs/pit.htm
*/
void PIT_init(void) 
{
	int divisor = dividor_num;   // 1193180/100 , 100 is for accurate and easy timekeeping
	outb(PIT_SQUARE_WAVE_MODE, PIT_COMMAND_REG);  // set our command byte 0x36
	outb(divisor & DIVISOR_MASK,PIT_CHANNEL_0);    // set low byte of divisor
	outb(divisor >> 8,PIT_CHANNEL_0);  // set high byte of fivisor

	return;
}

/*
*   Function: sche_init()
*   Description: initializes scheduling pid for each terminal, initializes flags for terminal checking
*   inputs: none
*   outputs: none
*   effects: 
*/
void sche_init()
{
	int i;
	// initializes terminal pid
	for (i = 0; i < 3; i++) {
		terminal_pid[i] = -1;
	}
	// initializes checking flags
	cur_index = -1;
	next_index = 0;
	init_flag = 1;
}

/*
*   Function: scheduling(void)
*   Description: scheduling process
*   inputs: none
*   outputs: none
*   effects: 
*/
void scheduling(void)
{
	// circulating active terminal index
	next_index = (cur_index + 1) % 3;

	// set up new video memory pages for every active terminal index
	vid_new(VIDEO_MEM + 2 * SCREEN_SIZE * next_index, next_index);

	//moves esp and ebp 
	move_registers_out();

	// calculate pcb address based on pid
	calculate_pcbaddr();

	//save old esp and ebp
	cur_pcb->sche_esp = esp;
	cur_pcb->sche_ebp = ebp;

	terminal_set();

	//change ss0, esp0 and restore esp, ebp
	tss.ss0 = KERNEL_DS;
	tss.esp0 = EIGHT_MB - EIGHT_KB*(cur_pcb->cur_pid) - 4;
	esp = cur_pcb->sche_esp;
	ebp = cur_pcb->sche_ebp;

	// end of interrupt, move esp and ebp
	send_eoi(0);
	move_registers_in();
	return;
}

/*
*   Function: calculate_pcbaddr()
*   Description: conditionally calculate pcb address based on pid
*   inputs: none
*   outputs: none
*   effects: 
*/
void calculate_pcbaddr()
{	// if there is no running process, calculate base pcb addr
    if (terminal_pid[next_index] == -1 && pid == -1) {
		cur_pcb = (pcb_t *)(EIGHT_MB - EIGHT_KB*(next_index + 1));
	}
	// otherwise, calculate pcb addr based on pid
	else {
		cur_pcb = (pcb_t *)(EIGHT_MB - EIGHT_KB*(terminal_pid[cur_index] + 1));
	}
    return;
}

/*
*   Function: terminal_set()
*   Description: check terminal status and map video mem
*   inputs: none
*   outputs: none
*   effects: 
*/
void terminal_set()
{
    // check if the terminal ran before, if so, execute shell, set initialized flag
	if (terminal_pid[next_index] == -1) {
		init_flag = 1;
		cur_index = next_index;

		//send eoi when done
		send_eoi(0);
		system_execute((uint8_t *) "shell");
	}
	// otherwise, calculate current pcb addr, change terminal index and use pid to map new addr to user space
	else {
		cur_pcb = (pcb_t *)(EIGHT_MB - EIGHT_KB*(terminal_pid[next_index] + 1));
		cur_index = next_index;

		map_user_prog(cur_pcb->cur_pid);
	}
	return;
}

/*
*   Function: move_registers_out
*   Description: move esp and ebp
*   inputs: none
*   outputs: none
*   effects: 
*/
void move_registers_out()
{
	asm volatile(
	    "movl %%esp, %0 	\n \
	    movl %%ebp, %1"
		:"=r"(esp), "=r"(ebp)
		:
		: "memory"
		);
	return;
}

/*
*   Function: move_registers_in
*   Description: move esp and ebp
*   inputs: none
*   outputs: none
*   effects: 
*/
void move_registers_in()
{
	asm volatile(
		"movl %0, %%esp  \n 	\
		 movl %1, %%ebp"
		 :
		 : "r"(esp), "r"(ebp)
		 : "memory"
		 );
	return;
}
