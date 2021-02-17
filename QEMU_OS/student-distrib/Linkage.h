#ifndef _LINKAGE_H
#define _LINKAGE_H

    // //the address of the bottom of the 4MB page already holding the image
    // #define JMP_ESP  0x83FFFF0

    void kbd_interrupt();
    // pointer to rtc interrupt
    void rtc_interrupt();
    // pointer for syscalls
    void sys_call();
    // pointer for undefined interrupt
    void undef_interrupt();
    // jump to user level
    uint32_t jump_to_user(uint32_t start_point);


#endif //_LINKAGE_H
