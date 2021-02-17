// keyboard.h - declares protocols for keyboard interrupts

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "syscalls.h"

//definitions for press signals of various commonly used keys
#define CAPS_LOCK_PRESS         0x3A
#define L_SHIFT_PRESS           0x2A
#define R_SHIFT_PRESS           0x36
#define CTRL_PRESS              0x1D
#define G_PRESS                 0x22
#define L_PRESS                 0x26
#define ALT_PRESS               0x38
#define SPACE_PRESS             0x39
#define ENTER_PRESS             0x1C
#define BACKSPACE_PRESS         0x0E
#define TAB_PRESS               0x0F
#define DEL_PRESS               0x53
#define F1_PRESS                0x3B
#define F2_PRESS                0x3C
#define F3_PRESS                0x3D
#define F4_PRESS                0x3E
#define F5_PRESS                0x3F
#define F6_PRESS                0x40
#define F7_PRESS                0x41
#define F8_PRESS                0x42
#define F9_PRESS                0x43
#define F10_PRESS               0x44
#define C_PRESS                 0x2E
#define UP_PRESS                0x48
#define DOWN_PRESS              0x50

//kbd buffer length
#define KBD_BUF_LENGTH          128
//max number of cmds
#define MAX_CMDS                50

//definitions for unwriteable chars
#define BACKSLASH               0x5C
#define SINGLE_QUOTE_THING      0x27
#define BACKSPACE               0x08

//kbd irq number
#define KBD_IRQ                 0x01
// keyboard port number for inb and outb
#define KBD_PRT                 0x60

//video memory definitions
#define VIDEO                   0xB8000
#define NUM_COLS                80
#define NUM_ROWS                25
#define ATTRIB                  0x2 // COLOR - LIB.C

//number of terminals
#define NUM_TERMS               10

//number of processses
#define MAX_PROCESS_NUM         24

//terminal struct
typedef struct term_t {
    //holds stack ptr
    uint32_t term_stack_ptr;
    //holds base ptr
    uint32_t term_base_ptr;
    //holds old cmds for terminal
    unsigned char term_cmd_buf[MAX_CMDS][KBD_BUF_LENGTH];
    //holds index to current position in the cmd array
    int term_cur_cmd_idx;
    //holds index for each cmd in history
    int term_buf_idxs[MAX_CMDS];
    //holds number of old cmds in history
    int term_old_cmd_num;
    //holds screen position of cursor
    int term_screen_x;
    int term_screen_y;
    //holds screen for terminal
    unsigned char term_screen[NUM_COLS][NUM_ROWS];
    //store if enter is pressed
    int term_enter_pressed;
    //store if typing is allowed
    int term_display_typing;
    //stores if terminal has had a shell opened
    int term_has_shell;
} term_t;

//terminals array stores all info for every terminal
term_t terminals[NUM_TERMS];
//stores current terminal
int cur_term;

// initialize necessary variables for keyboard functionality
void keyboard_init();
//disables the cursor when typing is off
void disable_cursor();
//enables the cursor with the given size
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
//updates the position of the cursor
void update_cursor(int x, int y);
// interrupt handler for keyboard press
void keyboard_IH();
//clears screen and sets line feed to 0,0
void clear_and_reset(void);
//switches the current kbd buffer between current and old ones
void switch_cmd_buffer();
//shifts old cmds
void shift_old_cmds();
//function to print the buffer
void print_kbd_buf();
//clears the display buffer
void clear_kbd_buf();
//open terminal
int32_t terminal_open(const uint8_t* filename);
//close terminal
int32_t terminal_close(int32_t fd);
//write char to video mem
void terminal_putc(uint8_t c);
//scrolls the terminal upwards
void scroll_terminal(int n);
//backspace function
void terminal_delc();
//copies terminal buffer to video memory
void show_screen();
//terminal_write
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
//terminal_read
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
//checks if fn keys were pressed and switches terminals
void check_fns();
//initializes terminals
void init_terminals();
// //gets current terminal number
// int get_cur_term();
//does something probably idk
void bruh();


#endif //_KEYBOARD_H
