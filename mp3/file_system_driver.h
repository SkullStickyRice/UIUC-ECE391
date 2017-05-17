#ifndef FILE_SYSTEM_DRIVER_H
#define FILE_SYSTEM_DRIVER_H

#include "types.h"
#include "lib.h"
#include "syscall.h"

#define entry_numbers 63
#define block_numbers 1023
#define bitmask 0xFFFFFFFF
#define FNAME_SIZE 32
#define RESERVED_ENTRY_SIZE 24 
#define RESERVED_BLOCK_SIZE 52 

typedef struct{
    int8_t filename[FNAME_SIZE];  
    uint8_t filetype;
    uint32_t inodes;
    uint8_t reserved[RESERVED_ENTRY_SIZE];
}dentry_t;

typedef struct{
    uint32_t data_length;
    uint32_t data_block[block_numbers];
}inode_t;

typedef struct{
    uint32_t dir_entries;
    uint32_t inodes_number;
    uint32_t data_block_num;
    uint8_t reserved[RESERVED_BLOCK_SIZE];
    dentry_t d[entry_numbers];
}boot_block_t;

int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
int file_open(int32_t fd, int8_t* buf, int32_t nbytes);
int file_close(int32_t fd, int8_t* buf, int32_t nbytes);
int file_write(int32_t fd, int8_t* buf, int32_t nbytes);

extern int32_t file_read(int32_t fd, uint8_t* buf, int32_t nbytes);   // new file read

int dir_open(int32_t fd, int8_t* buf, int32_t nbytes);
int dir_close(int32_t fd, int8_t* buf, int32_t nbytes);
int dir_write(int32_t fd, int8_t* buf, int32_t nbytes);
extern int32_t dir_read(int32_t fd, int8_t* buf, int32_t nbytes);
extern void test_fs();
extern void fs_initialize(uint32_t start_address);
//extern void test_dir_read();
extern int32_t read_file_in_dir(uint8_t* buf);
extern void read_test_text();
//extern void test_file_read();
//void test_ctl3(uint32_t dir_num);
uint32_t store_inodes(uint32_t num);
extern void file_loader(dentry_t dir_entry, uint8_t* eip_buf);

#endif
