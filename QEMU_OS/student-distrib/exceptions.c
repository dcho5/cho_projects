#include "exceptions.h"
#include "types.h"
#include "lib.h"
#include "syscalls.h"

//holds the names of the exceptions in order of number
char exception_names[20][35] = {
    "Divide Error Exception\0",
    "Debug Exception\0",
    "NMI Interrupt\0",
    "Breakpoint Exception\0",
    "Overflow Exception\0",
    "BOUND Range Exceeded Exception\0",
    "Invalid Opcode Exception\0",
    "Device Not Available Exception\0",
    "Double Fault Exception\0",
    "Coprocessor Segment Overrun\0",
    "Invalid TSS Exception\0",
    "Segment Not Present\0",
    "Stack Fault Exception\0",
    "General Protection Exception\0",
    "Page-Fault Exception\0",
    "System Call\0",
    "x87 FPU Floating-Point Error\0",
    "Alignment Check Exception\0",
    "Machine-Check Exception\0",
    "SIMD Floating-Point Exception\0",
};

//macro that defines a function for each exception
//which prints the name and loops infinitely
#define HANDLE_EXCPT(excpt_name, vector)            \
    void (excpt_name)(){                            \
                                            \
        printf("\n");                               \
        printf(exception_names[vector]);            \
        while(1);                                   \
    }

//exceptions for all the various exceptions
HANDLE_EXCPT(DE_excpt, 0x00);
HANDLE_EXCPT(DB_excpt, 0x01);
HANDLE_EXCPT(NMI_excpt, 0x02);
HANDLE_EXCPT(BP_excpt, 0x03);
HANDLE_EXCPT(OF_excpt, 0x04);
HANDLE_EXCPT(BR_excpt, 0x05);
HANDLE_EXCPT(UD_excpt, 0x06);
HANDLE_EXCPT(NM_excpt, 0x07);
HANDLE_EXCPT(DF_excpt, 0x08);
HANDLE_EXCPT(CS_excpt, 0x09);
HANDLE_EXCPT(TS_excpt, 0x0A);
HANDLE_EXCPT(NP_excpt, 0x0B);
HANDLE_EXCPT(SS_excpt, 0x0C);
HANDLE_EXCPT(GP_excpt, 0x0D);
//HANDLE_EXCPT(PF_excpt, 0x0E);
//15 skipped
HANDLE_EXCPT(MF_excpt, 0x10);
HANDLE_EXCPT(AC_excpt, 0x11);
HANDLE_EXCPT(MC_excpt, 0x12);
HANDLE_EXCPT(XF_excpt, 0x13);

void PF_excpt(){
    // uint32_t EIP = 0;
    // uint32_t CR2 = 0;
    /* asm volatile("mov %%eip, %%ecx \n\
                    mov %%cr2, %%ebx \n\
                "
                : "=c" (EIP), "=b" (CR2)); */
    //sys_call_execute((uint8_t*)"shell");
    printf(exception_names[0x0E]);
    printf("\n");
    // printf("EIP: %d\n", EIP);
    // printf("CR2: %d\n", CR2);

    while(1);
}


//clear();
