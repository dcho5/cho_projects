/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 *   https://hkn.illinois.edu/wiki/pic_interactions
 * ^ website talks about how to initialize and send EOI
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */



/*
 * i8259_init
 *   DESCRIPTION: Initializes the PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sends data to PIC
 */
void i8259_init(void) {
    //write first control word
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);
    //write 2nd control word
    outb(ICW2_MASTER , MASTER_DATA);
    outb(ICW2_SLAVE, SLAVE_DATA);
    //write 3rd control word
    outb(ICW3_MASTER, MASTER_DATA);
    outb(ICW3_SLAVE, SLAVE_DATA);
    //write last control word
    outb(ICW4, MASTER_DATA);
    outb(ICW4, SLAVE_DATA);
    //mask everything
    outb(master_mask, MASTER_DATA);
    outb(slave_mask, SLAVE_DATA);

    //enable slave which is on irq line 2
    enable_irq(2);

}

/*
 * enable_irq
 *   DESCRIPTION: enables the given irq line
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sends data to PIC
 */
void enable_irq(uint32_t irq_num) {
    if(irq_num >= 0 && irq_num < 8)//then we have the master PIC
    {
        //calcualte new mask
        master_mask &= ~(0x01 << irq_num);
        //send data to PIC
        outb(master_mask, MASTER_DATA);
    }
    if(irq_num > 7 && irq_num < 16) //we have a slave port irq
    {
        //calculate new mask
        slave_mask &= ~(0x01 << (irq_num - 8));
        //send data to PIC
        outb(slave_mask, SLAVE_DATA);
    }
}

/*
 * disable_irq
 *   DESCRIPTION: disable the given irq line
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sends data to PIC
 */
void disable_irq(uint32_t irq_num) {
    if(irq_num >= 0 && irq_num < 8)//then we have the master PIC
    {
        //calculate new mask
        master_mask |= (0x01 << irq_num);
        //send data to PIC
        outb(master_mask, MASTER_DATA);
    }
    if(irq_num > 7 && irq_num < 16) //we have a slave port irq
    {
        //calcualte new mask
        slave_mask |= (0x01 << (irq_num - 8));
        //send data to PIC
        outb(slave_mask, SLAVE_DATA);
    }
}

/*
 * send_eoi
 *   DESCRIPTION: sends end of interrupt signal
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sends data to PIC
 */
void send_eoi(uint32_t irq_num) {
    if(irq_num >= 0 && irq_num < 8)//then we have the master PIC
    {
        outb(irq_num | EOI, MASTER_8259_PORT);//send the EOI ord with the irq through the command port
    }
    else if(irq_num > 7 && irq_num < 16) //we have a slave port irq
    {
        outb((irq_num - 8) | EOI, SLAVE_8259_PORT);//send EOI signal from slave
        outb((0x02 | EOI), MASTER_8259_PORT); //aso need to send eoi through master
    }
}
