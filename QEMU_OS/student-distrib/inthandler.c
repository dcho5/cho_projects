#include "inthandler.h"
#include "lib.h"

/*
 * undef_interrupt
 *   DESCRIPTION: handler for undefined interrupt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints message
 */
void undefined_interrupt(){
    clear();
    printf("\nAn undefined interrupt happened\n");
    // while(1);
}

/*
 * system call
 *   DESCRIPTION: temp handler for system calls
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints message
 */
void system_call(){
    clear();
    printf("\nA system call happened\n");
}
