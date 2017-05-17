#include "syscall.h"


#define MB_128 0x08000000
#define MB_132 0x08400000
#define _136MB 0x8800000
int pid = -1;
int pid_status[];
uint8_t exe_ret = -1;
typedef int32_t (*f_ptr)();   // function pointer

//file operation
int32_t* rtc_op[4] = { (int32_t*)rtc_read, (int32_t*)rtc_write, (int32_t*)rtc_open, (int32_t*)rtc_close };
int32_t* terminal_op[4] = { (int32_t*)terminal_read, (int32_t*)terminal_write, (int32_t*)terminal_open, (int32_t*)terminal_close };
int32_t* dir_op[4] = { (int32_t*)dir_read, (int32_t*)dir_write, (int32_t*)dir_open, (int32_t*)dir_close };
int32_t* file_op[4] = { (int32_t*)file_read, (int32_t*)file_write, (int32_t*)file_open, (int32_t*)file_close };

/*
*  system_execute:
*      DESCRIPTION:	   the system call attemps to load and execute a new program, handling off the processor
*					   to the new program until it terminates.
*      INPUT:          command
*      OUTPUT:         -1    if command cannot be executed or
*					   0-255 if the program executes a halt
*/
int32_t system_execute(const uint8_t* command)
{
	cli();
	uint32_t esp;
	uint32_t ebp;
	uint8_t args[BUFFER_SIZE];
	uint8_t file[BUFFER_SIZE];
	dentry_t dir_entry;
	uint32_t elf;
	uint8_t eip_buf[4];
	int len_args = 0;
	int len_file = 0;
	int idx = 0;
	int i;
	int eip = 0;

	//init args and file array
	for (i = 0; i < BUFFER_SIZE; i++) {
		args[i] = '\0';
		file[i] = '\0';
	}
	if (command == NULL || command[0] == '\0') {
		printBuf((uint8_t*)"Empty filename!!\n");
		return -1;
	} 

	//store the esp and ebp
	asm volatile (
		"movl %%esp, %0		\n\
         movl %%ebp, %1"
		:"=r"(esp), "=r"(ebp)
		:
		: "memory"
		);

	if (strlen((int8_t*)command) > BUFFER_SIZE) {  //buffer_size is 128
		printBuf((uint8_t*)"Command too long!!\n");
		return -1;
	}
	//parse file and args of command-------------------------------------------------------
	while (command[idx] == ' ') { //parse command to check where the first char of file start
		idx++;
	}
	while (command[idx] != ' ' && command[idx] != '\0' && command[idx] != '\n') { //parse the file until reaching a space
		file[len_file] = command[idx];
		len_file++;
		idx++;
	}
	if (len_file > buf_len) {
		printBuf((uint8_t*)"Filename too long!!\n");
		return -1;
	}
	while (command[idx] == ' ') { //parse command to check where the first char of args start
		idx++;
	}
	while (command[idx] != ' ' && command[idx] != '\0' && command[idx] != '\n') { //parse the args until reaching a space or new line
		args[len_args] = command[idx];
		len_args++;
		idx++;
	}
	if (len_args > 35) {
		printBuf((uint8_t*)"Argument too long!!\n");
		return -1;
	}
	//-----------------------------------------------------------------------------------------------------
	//check file validity
	int8_t flag = read_dentry_by_name(file, &dir_entry);
	if (flag == -1) {
		printBuf((uint8_t*)"No such file!!\n");
		return -1;
	}
	//read the elf into elf buffer
	flag = read_data(dir_entry.inodes, 0, (void*)&elf, 4);
	if (flag == -1) {
		printBuf((uint8_t*)"Read elf data failed!!\n");
		return -1;
	}
	if (elf != EXE_MAGIC_NUM) {
		printBuf((uint8_t*)"Non executable!!\n");
		return -1; 
	}		
	//check if we reach the max shells
	if (pid >= SHELL_MAX_INDEX) {
		printBuf((uint8_t*)"Hit the maximum shell!!\n");
		return -1;
	}
	//-----------------------------------------------------------------------------------------------------
	//store the old pid and find a new pid
	/*int temp_pid = terminal_pid[display_index];
	for (i = 0; i <= SHELL_MAX_INDEX; i++) { 
		if (pid_status[i] == 0) {
			pid_status[i] = 1;
			pid = i;
			break;
		}
	}
	terminal_pid[display_index] = pid;*/

	int temp_pid;
	if (init_flag == 1) {
		temp_pid = terminal_pid[cur_index];
		int i;
		for (i = 0; i < 6; i++) {
			if (pid_status[i] == 0) {
				pid_status[i] = 1;
				pid = i;
				break;
			}
		}
		terminal_pid[cur_index] = pid;
	}
	else {
		temp_pid = terminal_pid[display_index];
		int i;
		for (i = 0; i < 6; i++) {
			if (pid_status[i] == 0) {
				pid_status[i] = 1;
				pid = i;
				break;
			}
		}
		terminal_pid[display_index] = pid;
	}
	init_flag = 0;

	// Set up paging
	map_user_prog(pid);

	//-----------------------------------------------------------------------------------------------------
	//load file into memory
	file_loader(dir_entry,eip_buf);  // defined in file_system_driver
	eip |= eip_buf[0];
	eip |= eip_buf[1] << (8); //shift 8 bits
	eip |= eip_buf[2] << (16); //shift 16 bits
	eip |= eip_buf[3] << (24);//shift 24 bits
	//-----------------------------------------------------------------------------------------------------
	//set up the current pcb
	pcb_t * pcb = Find_PCB(pid); 

	//set up the pcb entry
	pcb->cur_pid = pid;

	pcb->esp = esp;
	pcb->ebp = ebp;
	//store the prev_pid's esp0 and ss0
	pcb->esp0 = tss.esp0;
	pcb->ss0 = tss.ss0;

	//store prev_pid's pid
	pcb->prev_pid = temp_pid;

	pcb->file_array[0].f_op = terminal_op;
	pcb->file_array[1].f_op = terminal_op;
	pcb->file_array[0].inode = 0;
	pcb->file_array[1].inode = 0;
	pcb->file_array[0].f_position = 0;
	pcb->file_array[1].f_position = 0;
	pcb->file_array[0].flags = 1;
	pcb->file_array[1].flags = 1;
	
	for (i = 0; i < 32; i++) {
		pcb->command_arg[i] = args[i];
	}
	pcb->command_arg_size = len_args;

	//store the arg into pcb (for the system_get_args syscall)
	
	//-----------------------------------------------------------------------------------------------------
	//context switch
	asm volatile (
				"leal halt_ret, %%eax		\n\
				 movl %%eax, %0"
				:"=m"(pcb->return_addr)
				:
				: "eax", "memory"
				);
	
	//set the ss0 and esp0
	tss.ss0 = KERNEL_DS;
	tss.esp0 = EIGHT_MB - EIGHT_KB*pid;
	
	/* Set up the iret context. 
	 * interrupt will be enable after iret by orl EFALGE value 
	 */
	
	asm volatile ("	cli					\n\
				  movw %0, %%ax			\n\
				  movw %%ax, %%ds 		\n\
				  pushl %0				\n\
				  pushl %1 				\n\
				  pushfl				\n\
				  popl %%eax 			\n\
				  orl %2, %%eax 		\n\
				  pushl %%eax			\n\
				  pushl %3 				\n\
				  pushl %4 				\n\
				  "
				:
				: "i"(USER_DS), "r"(USER_ESP), "r"(EFLAGE), "i"(USER_CS), "r"(eip)
				: "eax", "memory"
				);
	asm volatile("iret"); 

	asm volatile("halt_ret:"); //label for the halt

	return exe_ret;
}

/*
*  system_halt:
*      DESCRIPTION:		halt the task	
*      INPUT:          status
*      OUTPUT:         return 0 on success
*					   
*/
int32_t system_halt(uint8_t status) 
{
	pcb_t* pcb = Find_PCB(terminal_pid[cur_index]);			// find the current pcb 
															//reset paging
	tss.esp0 = pcb->esp0;
	tss.ss0 = pcb->ss0;
	pid = pcb->cur_pid;

	int i;
	for (i = fd_min; i < MAX_FILE_NUM; i++)
		pcb->file_array[i].flags = 0;	// reset flag to 0
    
	pid_status[pid] = 0;				// exit shell
	pid = pcb->prev_pid;
	terminal_pid[cur_index] = pid;
	
	if (pid == -1) {
		system_execute((uint8_t *) "shell");	
	}

	exe_ret = status;

	map_user_prog(pid);


	//jump to parent process
	
	asm volatile (
				"movl %0, %%esp			\n\
        		movl %1, %%ebp			\n\
        		jmp *%2"
		:
	: "r"(pcb->esp), "r"(pcb->ebp), "r" (pcb->return_addr)
		);

	//jump to prev_pid process
	/*
	asm volatile("movl %0, %%esp"::"r"(pcb->esp));
	asm volatile("movl %0, %%ebp"::"r"(pcb->ebp));
	asm volatile("jmp *%0"::"r"(pcb->return_addr));*/

	return 0;
}

/*
*  int32_t read(int32_t fd, const unit8_t * buf, int32_t nbytes:
*      DESCRIPTION:		call the rtc/filesystem/termminal read function
*      INPUT:          fd, buffer, and bytes read
*      OUTPUT:         return -1 on failure, and resturn corressponding functor
*					  
*/

int32_t read(int32_t fd, uint8_t * buf, int32_t nbytes) 
{
	sti();
  	pcb_t* pcb = Find_PCB(pid);
  	if (fd > fd_max || fd < 0 || fd==1 || buf == NULL || pcb->file_array[fd].flags == 0)  //check if it is a valid fd, and it cannot do stdout when read
    	return -1;

    f_ptr func = (void*)(pcb->file_array[fd].f_op[0]);			// the corresponding read function
    return ((func)(fd,buf,nbytes));
}

/*
*   int32_t write(int32_t fd, const uint8_t * buf, int32_t nbytes)
*   	DESCRIPTIONL	call the rtc/filesystem/terminal write 
*   	INPUT:          fd, buffer, and bytes read
*		OUTPUT:         return -1 on failure, and resturn corressponding functor
*/

int32_t write(int32_t fd, const uint8_t * buf, int32_t nbytes) 
{
	pcb_t* pcb = Find_PCB(pid);
  	uint8_t checkFlag = pcb->file_array[fd].flags;
  	if (fd > fd_max || fd < 0 || fd == 0 || buf == NULL || checkFlag == 0)  //check if it is a valid fd, and it cannot do stdin when write
    	return -1;
  
    f_ptr func = (void*)pcb->file_array[fd].f_op[1];
    return (func)(fd,buf,nbytes);
}

/*
*   int32_t open(const uint8_t * filename)
*   	DESCRIPTIONL	call the rtc/filesystem/terminal open 
*   	INPUT:          filename
*		OUTPUT:         return -1 on failure, and resturn corressponding functor
*/

int32_t open(const uint8_t * filename) 
{
	
	if(filename == (uint8_t*)NOT_VALID)
    	return -1;
 
  	uint32_t fd;
  	dentry_t dentry;
  	fd = fd_alloc();
  	pcb_t* pcb = Find_PCB(pid);							// find the current pcb and check if the filename is valid or not
  	if(read_dentry_by_name(filename,&dentry) == -1)
    	return -1;
 
    if(dentry.filetype == 0)							// RTC type file, setup file array and call the corresponding function
    {													
      pcb->file_array[fd].f_op = rtc_op;
      pcb->file_array[fd].inode = 0;
      pcb->file_array[fd].f_position = 0;
      pcb->file_array[fd].flags = 1;
	  f_ptr func = (void*)(pcb->file_array[fd].f_op[2]);
      func();
      return fd;
    }
    else if(dentry.filetype == 1)						// directory type file, setup file array and call the corresponding function
    {
      pcb->file_array[fd].f_op = dir_op;
      pcb->file_array[fd].inode = 0;
      pcb->file_array[fd].f_position = 0;
      pcb->file_array[fd].flags = 1;
      f_ptr func = (void*)(pcb->file_array[fd].f_op[2]);
      func();
      return fd;
    }
    else if(dentry.filetype == 2)						// regular type file, setup file array and call the corresponding funciton
    {
       pcb->file_array[fd].f_op = file_op;
       pcb->file_array[fd].inode = dentry.inodes;
       pcb->file_array[fd].f_position = 0;
       pcb->file_array[fd].flags = 1;
       f_ptr func = (void*)(pcb->file_array[fd].f_op[2]);
       func();
       return fd;
    }
    else {
      return -1;
    }
}

/*
*   int32_t close(int32_t fd)
*   	DESCRIPTIONL	call the rtc/filesystem/terminal close 
*   	INPUT:          fd
*		OUTPUT:         0
*/

int32_t close(int32_t fd) {

	pcb_t* pcb = Find_PCB(pid);
	uint8_t checkFlag = pcb->file_array[fd].flags;

	if (fd < fd_min || fd >fd_max || checkFlag==0)		//check fd
		return -1;
	f_ptr func = (void*)pcb->file_array[fd].f_op[3]; //reset file array
	func(fd);
	pcb->file_array[fd].f_op = NULL;
    pcb->file_array[fd].f_position = 0;
    pcb->file_array[fd].flags = 0;
    return 0;	
}

/*
*   int32_t getargs(uint8_t * buf, int32_t nbytes)
*   	DESCRIPTION:	get the argument from command line and put the nbytes of arguments into buf
*   	INPUT:          buf, nbytes
*		OUTPUT:         0
*/
int32_t getargs(uint8_t * buf, int32_t nbytes) 
{
	pcb_t* pcb = Find_PCB(pid);
  	if(buf==NULL||pcb->command_arg_size>nbytes)
    	return -1;
  	strcpy((int8_t*)buf,(int8_t*)pcb->command_arg);
  	return 0;
}

/*
*   int32_t vidmap(uint8_t ** screen_start)
*   	DESCRIPTION: 	map the video memory to certain virtual address  
*   	INPUT: 			where the screen starts
*		OUTPUT: 		0  - success
*					    -1 - fail
*/

int32_t vidmap(uint8_t ** screen_start) 
{

	if (screen_start == NULL || screen_start <= (uint8_t**) INVALID_ADDR)
		return -1;
	map_video_mem((uint32_t)_256MB, (uint32_t)VIDEO+4096*2*cur_index);
	*screen_start = (uint8_t*)_256MB;

	return 0;

}


int32_t set_handler(int32_t signum, void * handler_address) {
	return -1;
}

int32_t sigreturn(void) {
	return -1;
}

/**************** Helper Function ************************/
pcb_t* Find_PCB(int pid)
{
  return (pcb_t*)(EIGHT_MB - EIGHT_KB*(pid + 1));
}

int32_t fd_alloc() {

  int i;
  for (i = fd_min; i < MAX_FILE_NUM; i++) {
    if ((Find_PCB(pid))->file_array[i].flags == 0)
      return i; //return valid fd
  }
  return -1;  //no valid fd
}


