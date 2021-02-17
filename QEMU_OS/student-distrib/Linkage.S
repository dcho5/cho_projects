
.text
# assembly linkage for interrupts

# pointer to kbd interrupt
.globl kbd_interrupt
# pointer to rtc interrupt
.globl rtc_interrupt
# pointer for syscalls
.globl sys_call
# pointer for undefined interrupt
.globl undef_interrupt
# pointer for jump to user function
.globl jump_to_user
# pointer to end of execute

.globl end_of_execute

.globl context_switch

.globl sys_call_success_RET

return_save:    .long 0x00

esp_save:       .long 0x00
ebp_save:       .long 0x00
eax_save:       .long 0x00

# undef_interrupt
# Description: jumps to handler for undefined interrupts
# Inputs   : none
# Outputs  : none
# Registers: none
undef_interrupt:
    cli                         # turn interrupts off
    pushal                      # save all registers
    pushfl                      # save flag reg
    call    undefined_interrupt # call interrupt handler for undefined interrupts
    jmp     ret_from_intr       # jump to the interrupt return

# kbd_interrupt
# Description: jumps to handler for kbd interrupts
# Inputs   : none
# Outputs  : none
# Registers: none
kbd_interrupt:
    cli                         # turn interrupts off
    pushal                      # save all registers
    pushfl                      # save flag reg
    call    keyboard_IH         # call interrupt handler for kbd
    jmp     ret_from_intr       # jump to the interrupt return

# kbd_interrupt
# Description: jumps to handler for rtc interrupts
# Inputs   : none
# Outputs  : none
# Registers: none
rtc_interrupt:
    cli                         # turn interrupts off
    pushal                      # save all registers
    pushfl                      # save flag reg
    call    rtc_IH              # call interrupt handler for rtc
    jmp     ret_from_intr       # jump to the interrupt return

ret_from_intr:
    popfl                       # restore flag register
    popal                       # restore registers
    sti                         # turn interrupts back on
    IRET                        # return from interrupt


# sys_call
# Description:
# sys call cmds go from 1-10 inclusive, but the jump table goes from 0-9 inclusive
# meaning each system call is 1 position off from the mp3 document
# This was done for simplicity
# Inputs   : args are in eax, ebx, ecx, edx
# Outputs  : 0 upon success and -1 if failure
# Registers: all are saved and restored except eax which has the return value
sys_call:
    sti                         # turn interrupts on
    pushal                      # save all registers
    pushfl                      # save flag reg   

    cmpl	$10, %eax						
    ja 		sys_call_error_RET          # jump to return if 10 < cmd number
    cmpl    $0x0, %eax          
    jle     sys_call_error_RET          # jump to return if cmd number <= 0 

    addl    $-1, %eax                   # make the cmd in range 0-9 inclusive
    sal     $2, %eax                    # multiply eax by 4 since each address is 4 bytes apart
    addl    $sys_call_jump_table, %eax  # add the jump table address to eax
    movl    0(%eax), %eax               # move address of system call function into eax

    pushl   %edx                        # push 3rd arg
    pushl   %ecx                        # push 2nd arg
    pushl   %ebx                        # push first arg
    call    *%eax                       # call the jmp table function
    addl    $12, %esp                   # clean up stack

    cmpl    $-1, %eax                  
    je      sys_call_error_RET          # return error if eax is -1
    # otherwise return eax

sys_call_success_RET:
    movl    %eax, return_save   # save eax
    popfl                       # restore flag register
    popal                       # restore registers
    movl    return_save, %eax   # put save back into eax
    # sti                         # turn interrupts back on
    IRET                        # return from interrupt

sys_call_error_RET:
    popfl                       # restore flag register
    popal                       # restore registers
    movl    $-1, %eax           # return -1 for error
    # sti                         # turn interrupts back on
    IRET                        # return from interrupt

# jump table for system calls
sys_call_jump_table:
.long   sys_call_halt
.long   sys_call_execute
.long   sys_call_read
.long   sys_call_write
.long   sys_call_open
.long   sys_call_close
.long   sys_call_getargs
.long   sys_call_vidmap
.long   sys_call_set_handler
.long   sys_call_sigreturn

# jump_to_user
# Description: Jumps to ring 3 by setting up the stack and doing an IRET
# STACK before IRET:
#           EIP
#           CS
#           EFLAGS
#           ESP
#           SS
# source: http://www.jamesmolloy.co.uk/tutorial_html/10.-User%20Mode.html
# Inputs   : 1 argument which is the entry point from bytes 24-27 of the exe just loaded
# Outputs  : if its being called, it jumps to custom stack. If it returns, eax is the output
# Registers: eax, ebx
jump_to_user:
    cli                          # turn interrupts off for critical section
    # c calling convention:
    pushl 	%ebp
	movl 	%esp, %ebp

    # *******save current ebp and esp*******

    # pushl   $0x00                   # push pid offset of 0
    call    get_cur_pcb
    # addl    $4, %esp                # clean up stack
    movl    %esp, 0(%eax)           # move esp into cur->stack_ptr
    movl    %ebp, 4(%eax)           # move ebp into cur->base_ptr
    # movl    %esp, esp_save
    # movl    %ebp, ebp_save

    # *******do fake IRET stuff*******

    # 0x002B is USER_DS
    movl    $0x002B, %eax       # move SS to eax
    movw    %ax, %fs             # load segment selectors
    movw    %ax, %es
    movw    %ax, %ds
    movw    %ax, %gs 

    movl   8(%esp), %ebx         # move arg into ebx
    movl    %esp, %eax           # move the stack pointer into eax
    # 0x002B is USER_DS
    pushl   $0x002B               # push user SS to stack
    # 0x83FFFF0 is the address of the bottom of the 4MB page already holding the image
    pushl   $0x83FFFF0             # push the value that ESP needs to take
    pushfl                       # push flags to stack

    # need to STI atomically. Done by pushing modified eflags to stack and
    # then doing IRET.
    popl    %eax                 # pop flags into eax
    orl     $0x200, %eax         # enable interrupts
    pushl   %eax                 # push modified eflags back onto stack

    # 0x0023 is USER_CS
    pushl   $0x0023               # push user CS to stack
    pushl   %ebx                 # push the arg to the stack

    IRET                         # run iret on custom stack

end_of_execute:
    # *******restore epb and esp from jump_to_user*******

    movl    %eax, eax_save      # save eax without using stack
    # pushl   $1                  # push pid offset of 1
    call    get_old_pcb
    # addl    $4, %esp            # clean up stack
    movl    0(%eax), %esp       # move cur->stack_ptr into esp
    movl    4(%eax), %ebp       # move cur->base_ptr into ebp
    movl    eax_save, %eax      # restore eax

    # movl    esp_save, %esp
    # movl    ebp_save, %ebp

    # *******return from jump_to_user*******
    leave
    ret

context_switch:
    # *******restore epb and esp from jump_to_user*******

    movl    %eax, eax_save      # save eax without using stack
    # pushl   $1                  # push pid offset of 1
    call    get_cur_pcb
    # addl    $4, %esp            # clean up stack
    movl    0(%eax), %esp       # move cur->stack_ptr into esp
    movl    4(%eax), %ebp       # move cur->base_ptr into ebp
    movl    eax_save, %eax      # restore eax

    # movl    esp_save, %esp
    # movl    ebp_save, %ebp

    # *******return from jump_to_user*******
    IRET
