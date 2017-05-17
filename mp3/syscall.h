#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "lib.h"
#include "rtc.h"
#include "terminal.h"
#include "file_system_driver.h"
#include "types.h"
#include "paging.h"
#include "x86_desc.h"
#include "interrupt_handlers.h"
#include "paging.h"
#include "scheduling.h"

#define EXE_MAGIC_NUM 0x464c457f
#define FOUR_MB 0x0400000 
#define EIGHT_MB 0x0800000
#define EIGHT_KB 0x2000
#define USER_ESP 0x0083FFFFC // 0x8000000(128MB)+0x400000(4MB)-4, user space stack pointer
#define buf_len 32
#define fd_min 2
#define fd_max 7
#define MAX_FILE_NUM 8
#define SHELL_MAX_INDEX 5
#define EFLAGE 0x200
#define FILE_RTC 0
#define FILE_DIR 1
#define FILE_FILE 2
#define FOUR_KB 0x1000
#define _256MB 0x10000000
#define NOT_VALID 0x8048caf
#define INVALID_ADDR 0x400000
#define VIDEO 0xB8000
int pid;
uint8_t exe_ret;
int pid_status[6];

// file struct
typedef struct file_t {
	int32_t ** f_op;
	int32_t inode;
	uint32_t f_position;
	uint32_t flags;	
} file_t;

// pcb structure
typedef struct pcb {
	int8_t cur_pid;
	file_t file_array[MAX_FILE_NUM];
	int8_t prev_pid;
	uint8_t command_file[buf_len];
	uint8_t command_arg[buf_len];
	int32_t command_arg_size;
	uint32_t esp0;			
	uint8_t ss0;	
	uint32_t esp;			
	uint32_t ebp;		
	uint32_t return_addr;	
	uint32_t sche_esp;
	uint32_t sche_ebp;
} pcb_t;

int32_t system_execute(const uint8_t* command);
int32_t system_halt(uint8_t status);

int32_t read(int32_t fd, uint8_t * buf, int32_t nbytes);
int32_t write(int32_t fd, const uint8_t * buf, int32_t nbytes);
int32_t open(const uint8_t * filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t * buf, int32_t nbytes);
int32_t vidmap(uint8_t ** screen_start);
int32_t set_handler(int32_t signum, void * handler_address);
int32_t sigreturn(void);


int32_t fd_alloc();
pcb_t* Find_PCB(int pid);


#endif


