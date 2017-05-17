#include "lib.h"
#include "idt_exception.h"

/*
divide_by_zero_eror(void)
Input: void
Return Value: none
Function: print info for divide error exception
*/
void divide_by_zero_error(void)
{
	clear();
	printf("Divide by Zero Error!\n");
	while (1);
}

/*
reversed(void)
Input: void
Output: none
Function: print info for reserved exception
*/
void reversed(void)
{
	clear();
	printf("Reversed Exception! (for Intel only)\n");
	while (1);
}

/*
non_maskable_interrupt(void)
Input: void
Output: none
Function: print info for nmi exception
*/
void non_maskable_interrupt(void)
{
	clear();
	printf("Non-Makable Interrupt Exception!\n");
	while (1);
}

/*
breakpoint(void)
Input: void
Output: none
Function: print info for breakpoint exception
*/
void breakpoint(void)
{
	clear();
	printf("Breakpoint Exception!\n");
	while (1);
}

/*
overflow(void)
Input: void
Output: none
Function: print info for overflow exception
*/
void overflow(void)
{
	clear();
	printf("Overflow Exception!\n");
	while (1);
}
/*
bound_range_exceeded(void)
Input: void
Output: none
Function: print info for nmi exception
*/
void bound_range_exceeded(void)
{
	clear();
	printf("Bound Range Exceeded Exception!\n");
	while (1);
}

/*
invalid_opcode(void)
Input: void
Output: none
Function: print info for invalid_opcode exception
*/
void invalid_opcode(void)
{
	clear();
	printf("Invalid Opcode Exception!\n");
	while (1);
}

/*
device_not+acailabe(void)
Input: void
Output: none
Function: print info for not availabe device exception
*/
void device_not_available(void)
{
	clear();
	printf("Deviece is Not Availabe!\n");
	while (1);
}

/*
double_fault(void)
Input: void
Output: none
Function: print info for double fault exception
*/
void double_fault(void)
{
	clear();
	printf("Double Fault Exception!\n");
	while (1);
}

/*
coprocessor_segment_overrun(void)
Input: void
Output: none
Function: print info for coprocessor segment overrun exception
*/

void coprocessor_segment_overrun(void)
{
	clear();
	printf("Coprocessor segment Overrun!\n");
	while (1);
}

/*
invalid_tss(void)
Input: void
Output: none
Function: print info for invalid tss exception
*/
void invalid_tss(void)
{
	clear();
	printf("Invalid TSS!\n");
	while (1);
}

/*
segment_not_present(void)
Input: void
Output: none
Function: print info for segment to present exception
*/
void segment_not_present(void)
{
	clear();
	printf("Segment Not Present!\n");
	while (1);
}

/*
stack_segment_fault(void)
Input: void
Output: none
Function: print info for stack segment fault exception
*/

void stack_segment_fault(void)
{
	clear();
	printf("Stack Segment Fault!\n");
	while (1);
}

/*
general_protection_fault(void)
Input: void
Output: none
Function: print info for general protection fault exception
*/
void general_protection_fault(void)
{
	clear();
	printf("General Protection Fault!\n");
	while (1);
}

/*
page_fault(void)
Input: void
Output: none
Function: print info for page fault exception
*/
void page_fault(void)
{
	clear();
	printf("Page Fault!\n");
	while (1);
}

/*
x87fpu_floating_point_error(void)
Input: void
Output: none
Function: print info for x87 floating point error exception
*/
void x87fpu_floating_point_error(void)
{
	clear();
	printf("x87 Floating Point Error!\n");
	while (1);
}

/*
alignment_checek(void)
Input: void
Output: none
Function: print info for alignment check exception
*/
void alignment_check(void)
{
	clear();
	printf("Alignemnt Check Exception!\n");
	while (1);
}

/*
nmachine_check(void)
Input: void
Output: none
Function: print info for machine check exception
*/
void machine_check(void)
{
	clear();
	printf("Machine Check Exception\n");
	while (1);
}

/*
simd_floating_point_exception(void)
Input: void
Output: none
Function: print info for simd floating point exception
*/
void simd_floating_point_exception(void)
{
	clear();
	printf("SIMD Floating Point Exception!\n");
	while (1);
}

