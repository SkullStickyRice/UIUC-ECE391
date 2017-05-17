/* i8259.c - Functions to interact with the 8259 interrupt controller
* vim:ts=4 noexpandtab
*/

#include "i8259.h"
#include "lib.h"

//static unsigned int cached_irq_mask = 0xffff;


/* Interrupt masks to determine which interrupts
* are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */



/* Initialize the 8259 PIC */
//Input: none
//Output: none
void
i8259_init(void)
{
	master_mask = MASTER_INIT_MASK;
	slave_mask = SLAVE_INIT_MASK;

	outb(0xff, Master_Data);  /* mask all of 8259A-1 */
	outb(0xff, Slave_Data);  /* mask all of 8259A-2 */

	outb(ICW1, MASTER_8259_PORT);     /* ICW1: select 8259A-1 init */
	outb(MASTER_8259_PORT, Master_Data); /* ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27 */
	outb(ICW3_MASTER, Master_Data);     /* 8259A-1 (the master) has a slave on IR2 */

	outb(ICW4, Master_Data);     /* master expects normal EOI */
	outb(ICW1, SLAVE_8259_PORT);     /* ICW1: select 8259A-2 init */
	outb(ICW2_SLAVE, Slave_Data); /* ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2f */
	outb(ICW3_SLAVE, Slave_Data);     /* 8259A-2 is a slave on master's IR2 */
	outb(ICW4, Slave_Data);     /* (slave's support for AEOI in flat mode is to be investigated) */

	outb(master_mask, Master_Data);
	outb(slave_mask, Slave_Data);  /* restore slave IRQ mask */
}

/* Enable (unmask) the specified IRQ */
//Input: irq_num
//Output: none
void
enable_irq(uint32_t irq_num)
{
	//int base_mask = 0x0B;
	unsigned int mask = 0x1;

	if (irq_num & 8)
	{
		// master_mask &= base_mask;
		// outb(master_mask,MASTER_8259_PORT+1);
		mask <<= (irq_num & 7);
		mask = ~mask;
		outb((mask & inb(Slave_Data)), Slave_Data);
	}
	else
	{
		mask <<= irq_num;
		mask = ~mask;
		outb((mask & inb(Master_Data)), Master_Data);
	}

}

/* Disable (mask) the specified IRQ */
//Input: irq_num
//Output: none
void
disable_irq(uint32_t irq_num)
{

	unsigned int mask = 0x1;

	if (irq_num & 8)
	{
		mask <<= (irq_num & 7);
		outb((mask | inb(Slave_Data)), Slave_Data);
	}
	else
	{
		outb((mask | inb(Master_Data)), Master_Data);
	}
}

/* Send end-of-interrupt signal for the specified IRQ */
//Input: irq_num
//Output: none
void
send_eoi(uint32_t irq_num)
{

	if (irq_num & 8)
	{
		outb(EOI | (irq_num & 7), SLAVE_8259_PORT); /* 'Specific EOI' to slave */
		outb(EOI + 2, MASTER_8259_PORT);            /* 'Specific EOI' to master-IRQ2 */
	}
	else
	{
		outb(EOI | irq_num, MASTER_8259_PORT);    /* 'Specific EOI' to master */
	}
	return;
}

