// syscalls.h - declares syscall handler functions

#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "types.h"

#define FILENAME_LEN 32
#define BUFFER_LIM 128
#define MAX_FILE_SIZE 100000

#define PROG_IMG_ADDR 0x08048000

#define KB 1024
#define MB 0x100000

#define MAX_INDEX 7
#define UNUSED 0
#define USED 1


//jump table for file open/close/r/w functions
typedef struct file_jump_table_t {
  //function pointer declarations
  int32_t (*open)(const uint8_t* filename);
  int32_t (*close)(int32_t fd);
  int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
  int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
} file_jump_table_t;

// Appendix A 8.2 File System Abstractions - 4 bytes each
typedef struct fd_t {
  //holds pointer to jump table
  file_jump_table_t* jump_table_ptr;
  //holds inode for file
  uint32_t inode;
  //holds file position
  uint32_t file_position;
  //holds flags
  uint32_t flags;
} fd_t;

//DO NOT EDIT THE ORDER OF THE FIRST 2 OR U WILL MESS UP SOME ASM CODE
typedef struct pcb_t {
  //stores the stack ptr
  uint32_t stack_ptr;
  //stores the base ptr
  uint32_t base_ptr;
  // process id of parent
  uint8_t parent_pid;
  //file array holds files for pcb
  fd_t file_array[8]; // 8.2 - "Each task can have up to 8 open files"
  //stores signal info
  uint32_t signal_info;
  // process id
  uint8_t pid;
  // argument buffer
  uint8_t arguments[BUFFER_LIM];
} pcb_t;


// keeps track of current pcb
pcb_t* pcb;

//get current pcb
pcb_t* get_cur_pcb();
//get old pcb
pcb_t* get_old_pcb();

void set_pcb();

//switch back to old terminal
void switch_term_back();


//sys call functions
int32_t sys_call_halt(uint8_t status);
int32_t sys_call_execute(const uint8_t* command);
int32_t sys_call_read(int32_t fd, void* buf, int32_t nbytes);
int32_t sys_call_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t sys_call_open(const uint8_t* filename);
int32_t sys_call_close(int32_t fd);
int32_t sys_call_getargs(uint8_t* buf, int32_t nbytes);
int32_t sys_call_vidmap(uint8_t** screen_start);
int32_t sys_call_set_handler(int32_t signum, void* handler_address);
int32_t sys_call_sigreturn(void);
int32_t retfail();

#endif //_SYSCALLS_H
