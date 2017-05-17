/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "idt_exception.h"
#include "syscall_linkage.h"
#include "paging.h"
#include "interrupt_linkage.h"
#include "interrupt_handlers.h"
#include "rtc.h"
#include "terminal.h"
#include "file_system_driver.h"


#include "syscall.h"
#include "scheduling.h"


/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

#define KBD_PORT 0x21	
#define RTC_PORT 0x28
#define SYS_CALL_VEC 0x80
#define RTC_IRQ  8 
#define KB_IRQ   1

#define PIT_ENTRY 0x20
/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;
	uint32_t file_system_start; //FS

	/* Clear the screen. */
	clear();

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;

		file_system_start = mod->mod_start; //FS


		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000 -4;
		ltr(KERNEL_TSS);
	}

	/* 0x00-0x19 corresponds to the exception in the IDT */	
	int i;
	/*IDT descriptor format:
	 *15 14 13 12 ----- 8
	 * P   DPL  0 D 1 1 0
	*/
	for (i=0; i<0x20; i++)		
	{
		idt[i].present = 1;
		idt[i].dpl = 0;			//kernel mode	
		idt[i].size = 1;
		idt[i].reserved0 = 0;	
		idt[i].reserved1 = 1;	
		idt[i].reserved2 = 1;
		idt[i].reserved3 = 1;	//Size of gate: 1 = 32 bits; 0 = 16 bits
		idt[i].reserved4 = 0;	
		idt[i].seg_selector = KERNEL_CS; //Segment Selector for dest. code segment
	}
	/* 0x20-0xFF corresponds to the interupts in the IDT */
	for (i = 0x20; i< NUM_VEC; i++)
	{
		idt[i].present = 1;
		idt[i].dpl = 0;			//kernel mode	
		idt[i].size = 1;
		idt[i].reserved0 = 0;		
		idt[i].reserved1 = 1;
		idt[i].reserved2 = 1;
		idt[i].reserved3 = 0;	//Size of gate: 1 = 32 bits; 0 = 16 bits	
		idt[i].reserved4 = 0;
		idt[i].seg_selector = KERNEL_CS;	//Segment Selector for dest. code segment
	}

		/* Set the exception handler in the IDT 0x00-0x13 (first 19 are defined in the exception_handler)*/
		SET_IDT_ENTRY(idt[0x00], divide_by_zero_error);
		SET_IDT_ENTRY(idt[0x01], reversed);
		SET_IDT_ENTRY(idt[0x02], non_maskable_interrupt);
		SET_IDT_ENTRY(idt[0x03], breakpoint);
		SET_IDT_ENTRY(idt[0x04], overflow);
		SET_IDT_ENTRY(idt[0x05], bound_range_exceeded);
		SET_IDT_ENTRY(idt[0x06], invalid_opcode);
		SET_IDT_ENTRY(idt[0x07], device_not_available);
		SET_IDT_ENTRY(idt[0x08], double_fault);
		SET_IDT_ENTRY(idt[0x09], coprocessor_segment_overrun);
		SET_IDT_ENTRY(idt[0x0A], invalid_tss);
		SET_IDT_ENTRY(idt[0x0B], segment_not_present);
		SET_IDT_ENTRY(idt[0x0C], stack_segment_fault);
		SET_IDT_ENTRY(idt[0x0D], general_protection_fault);
		SET_IDT_ENTRY(idt[0x0E], page_fault);
		SET_IDT_ENTRY(idt[0x10], x87fpu_floating_point_error);
		SET_IDT_ENTRY(idt[0x11], alignment_check);
		SET_IDT_ENTRY(idt[0x12], machine_check);
		SET_IDT_ENTRY(idt[0x13], simd_floating_point_exception);
	
		/* Set the interrupt for keyboard in the IDT 0x21 */
		idt[KBD_PORT].seg_selector = KERNEL_CS;
		idt[KBD_PORT].reserved0 = 0;
		idt[KBD_PORT].reserved1 = 1;
		idt[KBD_PORT].reserved2 = 1;		
		idt[KBD_PORT].reserved3 = 0;		
		idt[KBD_PORT].reserved4 = 0;
		idt[KBD_PORT].present = 1;
		idt[KBD_PORT].dpl = 0;
		idt[KBD_PORT].size = 1;
		SET_IDT_ENTRY(idt[KBD_PORT], keyboard_linkage);	
		
		/* Set the interrupt for RTC in the IDT 0x28 */
		idt[RTC_PORT].present = 1;
		idt[RTC_PORT].dpl = 0;
		idt[RTC_PORT].size = 1;
		idt[RTC_PORT].reserved0 = 0;
		idt[RTC_PORT].reserved1 = 1;
		idt[RTC_PORT].reserved2 = 1;
		idt[RTC_PORT].reserved3 = 0;
		idt[RTC_PORT].reserved4 = 0;
		idt[RTC_PORT].seg_selector = KERNEL_CS;
		SET_IDT_ENTRY(idt[RTC_PORT], rtc_linkage);

		/* Set the interrupt for system call in the IDT 0x80 */
		{
		idt[SYS_CALL_VEC].seg_selector = KERNEL_CS;
		idt[SYS_CALL_VEC].present = 1;
		idt[SYS_CALL_VEC].size = 1;
		idt[SYS_CALL_VEC].dpl = 3;
		idt[SYS_CALL_VEC].reserved0 = 0;
		idt[SYS_CALL_VEC].reserved1 = 1;
		idt[SYS_CALL_VEC].reserved2 = 1;
		idt[SYS_CALL_VEC].reserved3 = 0;
		idt[SYS_CALL_VEC].reserved4 = 0;
		SET_IDT_ENTRY(idt[SYS_CALL_VEC], syscall_linkage);
		}

		idt[PIT_ENTRY].seg_selector = KERNEL_CS;
		idt[PIT_ENTRY].reserved4 = 0;
		idt[PIT_ENTRY].reserved3 = 0;
		idt[PIT_ENTRY].reserved2 = 1;
		idt[PIT_ENTRY].reserved1 = 1;
		idt[PIT_ENTRY].size = 1;
		idt[PIT_ENTRY].reserved0 = 0;
		idt[PIT_ENTRY].dpl = 3;
		idt[PIT_ENTRY].present = 1;
		SET_IDT_ENTRY(idt[PIT_ENTRY], pit_linkage);
	
		i8259_init();	 //init the PIC
		
		init_page();	// init paging
		sche_init();
		PIT_init();
		rtc_init();      //call init rtc in rtc.c file
		//terminal_open(); //call terminal open in terminal.c file 
		clear();
		keybrd_init();

		fs_initialize(file_system_start); //FS

		for (i = 0; i < 6; i++) {
			pid_status[i] = 0;
		}
		pid = -1;

		enable_irq(KB_IRQ);
		//enable_irq(KB_IRQ);	//enable keyboard (based on IDT master PIC)
		enable_irq(RTC_IRQ);    //enable RTC	  (based on IDT master PIC)
		//enable_irq(0);
		enable_irq(0);			//enable PIT
	

		sti();
		clear();
		/* Initialize devices, memory, filesystem, enable device interrupts on the
		 * PIC, any other initialization stuff... */

		/* Enable interrupts */
		/* Do not enable the following until after you have set up your
		 * IDT correctly otherwise QEMU will triple fault and simple close
		 * without showing you any output */
		/*printf("Enabling Interrupts\n");
		  sti();*/

		/* Execute the first program (`shell') ... */
		system_execute((uint8_t *) "shell");
		while (1) {

		}
		/* Spin (nicely, so we don't chew up cycles) */
		asm volatile(".1: hlt; jmp .1;");
}



