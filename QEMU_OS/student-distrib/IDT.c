
#include "IDT.h"
#include "inthandler.h"

/*
 * populate_IDT
 *   DESCRIPTION: Initializes the IDT table by populating it
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes IDT table
 */
void populate_IDT ()
{
    int i;
    // populate IDT table
    for(i = 0; i < NUM_VEC; i++){
        // default values for exceptions [19:0]
        idt[i].seg_selector = KERNEL_CS;
        idt[i].reserved4 = 0;
        idt[i].reserved3 = 1;
        // reserved3 bit is 1 for INT and SYSCALL
        if (i>=0x20) idt[i].reserved3 = 0;
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1;
        idt[i].size = 1;
        idt[i].reserved0 = 0;
        idt[i].dpl = 0;
        // dpl is 3 for sys_call (user-level)
        if (i==0x80) idt[i].dpl = 3;
        idt[i].present = 1;

        SET_IDT_ENTRY(idt[i], undefined_interrupt);
    }

    //set the exceptions
    SET_IDT_ENTRY(idt[0], DE_excpt);
    SET_IDT_ENTRY(idt[1], DB_excpt);
    SET_IDT_ENTRY(idt[2], NMI_excpt);
    SET_IDT_ENTRY(idt[3], BP_excpt);
    SET_IDT_ENTRY(idt[4], OF_excpt);
    SET_IDT_ENTRY(idt[5], BR_excpt);
    SET_IDT_ENTRY(idt[6], UD_excpt);
    SET_IDT_ENTRY(idt[7], NM_excpt);
    SET_IDT_ENTRY(idt[8], DF_excpt);
    SET_IDT_ENTRY(idt[9], CS_excpt);
    SET_IDT_ENTRY(idt[10], TS_excpt);
    SET_IDT_ENTRY(idt[11], NP_excpt);
    SET_IDT_ENTRY(idt[12], SS_excpt);
    SET_IDT_ENTRY(idt[13], GP_excpt);
    SET_IDT_ENTRY(idt[14], PF_excpt);
    //skip 15
    SET_IDT_ENTRY(idt[16], MF_excpt);
    SET_IDT_ENTRY(idt[17], AC_excpt);
    SET_IDT_ENTRY(idt[18], MC_excpt);
    SET_IDT_ENTRY(idt[19], XF_excpt);

    //set the kbd with vector 0x21
    SET_IDT_ENTRY(idt[0x21], kbd_interrupt);
    //set the RTC with vector 0x20
    SET_IDT_ENTRY(idt[0x28], rtc_interrupt);
    //set SYSCALL with vector 0x80
    SET_IDT_ENTRY(idt[0x80], sys_call);

    //load the IDTR
    lidt(idt_desc_ptr);
}
