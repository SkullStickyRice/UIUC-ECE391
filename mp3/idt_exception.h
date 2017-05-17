#ifndef _EXCEPTION_H
#define _EXCEPTION_H

/*
*	Exceptions
*
*/


/*Vector No. 0x00*/
extern void divide_by_zero_error(void);
/*Vector No. 0x01*/
extern void reversed(void);
/*Vector No. 0x02*/
extern void non_maskable_interrupt(void);
/*Vector No. 0x03*/
extern void breakpoint(void);
/*Vector No. 0x04*/
extern void overflow(void);
/*Vector No. 0x05*/
extern void bound_range_exceeded(void);
/*Vector No. 0x06*/
extern void invalid_opcode(void);
/*Vector No. 0x07*/
extern void device_not_available(void);
/*Vector No. 0x08*/
extern void double_fault(void);
/*Vector No. 0x09*/
extern void coprocessor_segment_overrun(void);
/*Vector No. 0x0A*/
extern void invalid_tss(void);
/*Vector No. 0x0B*/
extern void segment_not_present(void);
/*Vector No. 0x0C*/
extern void stack_segment_fault(void);
/*Vector No. 0x0D*/
extern void general_protection_fault(void);
/*Vector No. 0x0E*/
extern void page_fault(void);
/*Vector No. 0x10*/
extern void x87fpu_floating_point_error(void);
/*Vector No. 0x11*/
extern void alignment_check(void);
/*Vector No. 0x12*/
extern void machine_check(void);
/*Vector No. 0x13*/
extern void simd_floating_point_exception(void);

#endif

