#include "paging.h"

/* PG - Paging flag, bit 31 of CR0
* PSE- Page size extension, bit 4 of CR4
* PS - Page size, bit 7 of page directory enteries for 4-KBytes (0-4KB, 1-4MB)
*/
#define PD_MASK  		0xFFFFF000       // Page directory addr mask 
#define PT_MASK  		0xFFFFF000		 // Page Table addr mask 
#define RW_PRESENT  	0x00000003     
#define RW_NOT_PRESENT  0x00000002
#define BASE            0x0 
#define PAGE_4MB_ENABLE 0x80             // bit 7 in PDE, set to 1 for 4M_Byte page 
#define PAGE_4MB_ADDR   0x400000
#define CR4_BIT4        0x10
#define VIDEO_MEM       0xB8000          // refer: lib.c 
#define VIDEO_IDX       0xB8 
#define CR0_BIT31       0x80000000
#define START_ADD       0x00000000
#define USER            0x04
#define USER_PAGE 		32
#define FOUR_MB 		0x0400000 
#define ADDR_8MB 		0x0800000

#define USER_RW 0x07
#define FOUR_MB_PRESENT_USER 0x87
#define pe_size 1024	//entry size for both page directory or page table
#define RW 0x00000002	//not present
#define pm_size 4096	//memory size for pages in page table --> 4kB
#define FOUR_MB_PRESENT 0x83

static unsigned int cr0, cr3, cr4;


/* init_page()
* 		DESCRIPTION: The function initialize paging, by setting page directory and page table. 
*		INPUT:       None
*		OUTPUT:      None
*/
void init_page()
{
	unsigned int start_addr = START_ADD;     // starting from 0x0
	int i;
	/* initial page table */
	for(i = 0; i<PTE_num; i++)
	{
		page_table[i] = start_addr | RW_NOT_PRESENT;
		start_addr += PTE_size;
	}
	page_table[VIDEO_IDX] = VIDEO_MEM | RW_PRESENT;    // set video memory in page table entry 

	/* initial page directory */
	for(i = 0; i<PDE_num; i++)
		page_dir[i] = RW_NOT_PRESENT | BASE;

	/* initial user page table */
	for(i = 0; i<PTE_num; i++)
		user_page_table[i] = RW_NOT_PRESENT | BASE;

	// for CP1, we only have one page directory and one page table 
	page_dir[0] = (unsigned int)page_table&PD_MASK; // set bit 31-12 in page dir as page table base addr 
	page_dir[0] |= RW_PRESENT;        // set R/W & present bit in dir[0]

									  // a single 4M_Byte page shoule be refered directly from page directory 
	page_dir[1] |= PAGE_4MB_ENABLE | RW_PRESENT | PAGE_4MB_ADDR;  // 4MB page starting from 0x400000

    // set page size extension in bit 4 of cr4
	asm volatile ("mov %%cr4,%0" : "=r"(cr4));   // native_read_cr4
	cr4 |= CR4_BIT4;							 // set bit 4
	asm volatile ("mov %0, %%cr4":: "r"(cr4));   // write to cr4

	enable_paging();

	return;

}

/*	enable_paging()
*		DESCRIPTION: this function is inline assembly code. It set the Paging Flag (bit 31) in cr0, and set cr3 as a pointer
*					 to the starting of page directory.
*		INPUT:       none
*		OUTPUT:      none
*/
void enable_paging()
{
	asm volatile("lea page_dir,%%eax;"     
		"mov %%eax,%%cr3;"                  // cr3 points to the starting address of page directory
		:"=r"(cr3) 
		:"r"(cr3));
	asm volatile("mov %%cr0,%0;" : "=r"(cr0));
	cr0 |= CR0_BIT31;						// set paging flag in cr0
	asm volatile("mov %0, %%cr0"::"r"(cr0));
}

/*
*   void map_user_prog
*		DESCRIPTION: map the user program into corresponding virtual memory and flush tlb
*		INPUT:       pid
*		OUTPUT:      none
*/
void map_user_prog(uint8_t pid) {

	uint32_t addr = FOUR_MB * pid + ADDR_8MB;
	page_dir[USER_PAGE] = addr|USER|RW_PRESENT|PAGE_4MB_ENABLE; 	//map user level

	flush_tlb();
}

/* 
 * flush_tlb()
 *		DESCRIPTION: flush the TLB by reload page directory base address into cr3
 *		INPUT:       none
 *		OUTPUT:      none
 */ 

void flush_tlb()
{
	asm volatile("movl %%cr3, %%eax;"
		"movl %%eax, %%cr3;"
		:::"memory", "cc");
}

/* 
 * map_video_mem()
 *		DESCRIPTION: Maps the 4KB memory at the given virtual address to the first page of the user page table
 *		INPUT:       virtual Address, physical Address
 *		OUTPUT:      none
 */ 

void map_video_mem(uint32_t virtualAddr, uint32_t physicalAddr)
{
	
    uint32_t PDE_index = virtualAddr>>22;	//get top 10 bits of pde index
    page_dir[PDE_index] = (uint32_t)user_page_table | USER | RW_PRESENT;
    user_page_table[0] = physicalAddr | USER | RW_PRESENT; // attributes: user, read/write, present
    flush_tlb();

}
void vid_new(uint32_t addr, int display_index)
{
	page_table[VIDEO_IDX + display_index * 2] = addr | USER_RW; 	//set to user level
	flush_tlb();
}
