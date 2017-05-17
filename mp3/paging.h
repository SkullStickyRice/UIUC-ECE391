#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

#define PDE_num  1024
#define PTE_num  1024
#define PDE_size  PDE_num*4		// 4B for each pde entry 
#define PTE_size  PTE_num*4		// 4B for each pte entry 

uint32_t page_dir[PDE_num] __attribute__((aligned(PDE_size)));

/*********************************************************
*        Page-Directory Entry (4KB Page Table)
*	31-------------------12 11---9-8-7-6-5-4-3-2-1-0
*	  Page-Tabel Base Addr   Avail G P 0 A P P U R P
*									 S 	   C W / /
*										   D T S W
**********************************************************
*             Page-Directory Entry (4MB Page)
*	31-------------22 21------13 12 11--9 8-7-6-5-4-3-2-1-0
*	  Page Base Addr   Reserved  P  Avail G P D A P P U R P
*								 A	        S 	  C W / /
*								 T		          D T S W
***********************************************************/

uint32_t page_table[PTE_num] __attribute__((aligned(PTE_size)));

/****************************************************
*				Page-Table Entry (4kB Page)
*	31-------------------12 11---9-8-7-6-5-4-3-2-1-0
*	     Page Base Addr      Avail G P D A P P U R P
*									 A 	   C W / /
*									 T	   D T S W
****************************************************/

uint32_t user_page_table[PTE_num] __attribute__((aligned(PTE_size)));

extern void init_page();
extern void enable_paging();
extern void map_user_prog(uint8_t pid);
extern void flush_tlb();
extern void map_video_mem(uint32_t virtualAddr, uint32_t physicalAddr);

extern void vid_new(uint32_t addr, int display_index);


#endif

