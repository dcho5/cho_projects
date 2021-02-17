//syscalls.c - has functions for all the system calls
#include "syscalls.h"
#include "keyboard.h"
#include "filesystem.h"
#include "Linkage.h"
#include "paging.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"

#define MAX_PROCESS_NUM 24 // MAKE SURE TO CHANGE PID_ARR IF MODIFIED

#define DEBUG 0 // debug switch

// "magic number that identifies the file as an executable."
uint8_t exec_check[4] = {0x7F, 0x45, 0x4C, 0x46};

// array of flags to see which processes are running
static uint8_t pid_arr[NUM_TERMS][MAX_PROCESS_NUM] =
{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

// keeps track of active process
static uint8_t pid_active[NUM_TERMS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// keeps track of parent process
static uint8_t pid_old[NUM_TERMS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//terminal jump table
file_jump_table_t term_fn = {terminal_open, terminal_close, terminal_read, terminal_write};
//rtc jump table
file_jump_table_t rtc_fn = {open, close, read, write};
//file jump table
file_jump_table_t file_fn = {file_open, file_close, file_read, file_write};
//directory jump table
file_jump_table_t dir_fn = {dir_open, dir_close, dir_read, dir_write};
//null jump table
file_jump_table_t null_fn = {retfail, retfail, retfail, retfail};

/*
 * sys_call_halt
 *   DESCRIPTION: terminates a process
 *   INPUTS: status - 8-bit argument stored into %BL
 *   RETURN VALUE: 0 on success, -1 on fail
 */
int32_t sys_call_halt(uint8_t status){
  uint32_t bl;
  int i;

  // initialize current PCB
  int pid_cur = pid_active[cur_term];
  int pcb_pos_cur = (cur_term+1) * pid_cur;
  pcb_t* pcb_cur = (pcb_t*) (8*MB - (8*KB * (pcb_pos_cur + 1)));

  // initialize parent PCB
  int pid_par = pcb_cur->parent_pid;
  int pcb_pos_par = (cur_term+1) * pid_par;
  pcb_t* pcb_par = (pcb_t*) (8*MB - (8*KB * (pcb_pos_par + 1)));

  if (DEBUG) printf("HALT\nPid_cur: %d\nPid_par: %d\n", pcb_pos_cur, pcb_pos_par);

  //execute shell if try to halt shell
  if (pid_cur == 0) {
    pid_arr[cur_term][pid_cur] = 0;
    printf("Closing Last Shell...\n");
    return sys_call_execute((uint8_t*)"shell");
  }

  //init the max possible number of files (8) to the default values
  for(i = 2; i < 8; i++){
      if (pcb_cur->file_array[i].flags == 1) sys_call_close(i);
      pcb_cur->file_array[i].jump_table_ptr = &null_fn;
      pcb_cur->file_array[i].flags = 0;
  }

  //map the parent page
  map_page(pcb_pos_par);

  //set esp0 in TSS
  tss.esp0 = 8*MB - 8*KB * (pcb_pos_par) - 4;

  //lower 8 bits of status expanded to 32 bits
  bl = (uint32_t)(status & 0xFF);

  //set old pid
  pid_old[cur_term] = pid_cur;

  // latest process is now parent process
  pid_active[cur_term] = pid_par;

  // keep track of current pcb
  pcb = pcb_par;

  // reset flag for pid that is being closed
  pid_arr[cur_term][pid_cur] = 0;

  if (DEBUG) printf("TSS: %d\n", tss.esp0);

  //set return value and jump to end of execute
  asm volatile("mov %0, %%eax;"
               "jmp end_of_execute;"
               :
               :"r"(bl)
               :"%eax"
              );
  return 0;
}

/*
 * sys_call_execute
 *   DESCRIPTION: attempts to load and exeute a new program, handing off the
                  processor to the new program until it terminates
 *   INPUTS: command - first word is filename, rest of it is handled via getargs
 *   RETURN VALUE: -1 on fail, 256 on exception, 0-255 otherwise (halt)
 */
int32_t sys_call_execute(const uint8_t* command){

/********************************PARSE********************************/
uint32_t ret;
  uint8_t fname[FILENAME_LEN]; // file name string - 32 indices
  //uint8_t shell[FILENAME_LEN] = "shell";
  uint8_t args[BUFFER_LIM]; // argument string - 128 indices
  uint8_t buf[4]; // buf for file_read - read 4 bytes
  int i = 0; // index for loops
  int j = 0; // index for fname and args

  dentry_t dentry; // dentry for read later

  // if command is NULL or has 0 size, fail
  if (!command || command[0] == '\0') return -1;


  // extract filename from command
  while (command[i] == ' ') i++; // strip leading zeroes
  while (command[i] != ' ' && command[i] != '\0'){
    if (j >= FILENAME_LEN) return -1; // if filename is longer than 32, fail
    fname[j] = command[i];
    i++;
    j++;
  }
  fname[j] = '\0'; // end of filename string
  j = 0; // reset j

  // extract args from command
  while (command[i] == ' ') i++; // strip leading zeroes
  while (command[i] != '\0') {
    if (i >= BUFFER_LIM) return -1; // if too many args, fail
    args[j] = command[i];
    i++;
    j++;
  }
  args[j] = '\0'; // end of args string

/***************************EXECUTABLE CHECK**************************/

  // if not read properly, fail
  if (read_dentry_by_name((uint8_t*)fname, &dentry) != 0) return -1;
  if (read_data(dentry.inode, 0, buf, 4) == 0) return -1;

  // check if buf has magic numbers
  for (i=0; i<4; i++) {
    if (buf[i] != exec_check[i]) return -1;
  }

  // Save buf so that EIP spans 4 bytes from 24 to 27
  if (read_data(dentry.inode, 24, buf, 4) == 0) return -1;

/********************************PAGING*******************************/

  // initialize pid_par and pid_cur
  // iterate through pid_arr to check for space
  int pid_par = pid_active[cur_term];
  int pid_cur;
  for (pid_cur = 0; pid_cur < MAX_PROCESS_NUM; pid_cur++) {
    if(pid_arr[cur_term][pid_cur] == 0) {
      pid_arr[cur_term][pid_cur] = 1;
      break;
    }
  }

  // if pid_arr is full, can't execute any more programs
  if (pid_cur == MAX_PROCESS_NUM) {
    printf("Max Process Number Reached!\n");
    return 1;
  }

  // calculate linear position of pcb
  int pcb_pos_cur = (cur_term+1) * pid_cur;

  //map the current process number
  map_page(pcb_pos_cur);

/**************************LOAD USER PROGRAM**************************/

  // The program image itself is linked to execute at 0x08048000
  // Copy entire file to memory starting at virtual address 0x08048000
  if (read_data(dentry.inode, 0, (uint8_t*)PROG_IMG_ADDR, MAX_FILE_SIZE) == 0) return -1;

/******************************CREATE PCB*****************************/

  pcb_t* pcb_cur = (pcb_t*) (8*MB - (8*KB * (pcb_pos_cur + 1)));

  //init the max possible number of files (8) to the default values
  for(i = 0; i < 8; i++){
    //initialize all values of fd to 0
    pcb_cur->file_array[i].jump_table_ptr = &null_fn;
    pcb_cur->file_array[i].inode = 0;
    pcb_cur->file_array[i].file_position = 0;
    pcb_cur->file_array[i].flags = 0;

    //first 2 are occupied by stdin and stdout which use terminal functions
    if(i == 0 || i == 1){
      pcb_cur->file_array[i].jump_table_ptr = &term_fn;
      pcb_cur->file_array[i].flags = 1;
    }
  }

  //init variables in the PCB
  pcb_cur->signal_info = 0;
  pcb_cur->pid = pid_cur;
  pcb_cur->parent_pid = pid_par;

  // copy args to arguments
  i = 0;
  while (args[i] != '\0') {
    pcb_cur->arguments[i] = args[i];
    i++;
  }
  pcb_cur->arguments[i] = '\0';

  // now set pcb to current
  pcb = pcb_cur;
  pid_active[cur_term] = pid_cur;

  // set flag
  pid_arr[cur_term][pid_cur] = 1;

/***************************CONTEXT SWITCH****************************/

  //set ss0 in TSS
  tss.ss0 = KERNEL_DS;
  //set esp0 in TSS
  tss.esp0 = 8*MB - 8*KB * (pcb_pos_cur) - 4;

  if (DEBUG) {
    printf("EXECUTE --- Process #: %d\n", pid_active[cur_term]);
    printf("TSS: %d\n", tss.esp0);
  }

  ret = jump_to_user(*((uint32_t*)buf));
  pid_old[cur_term] = pid_active[cur_term];
  return (ret);
}

/*
 * switch_term_back
 *   DESCRIPTION: switches back to an old terminal. Similar to execute.
 *   INPUTS: none
 *   RETURN VALUE: none
 * SIDE EFFECT: none
 */
void switch_term_back()
{
  // calculate linear position of pcb
  int pcb_pos_cur = (cur_term+1) * pid_active[cur_term];
  // printf("%x\n%x\n", cur_term, pid_active[cur_term]);

  //map the current process number
  map_page(pcb_pos_cur);

  //set ss0 in TSS
  tss.ss0 = KERNEL_DS;
  //set esp0 in TSS
  tss.esp0 = 8*MB - 8*KB * (pcb_pos_cur) - 4;
}

/*
 * get_cur_pcb
 *   DESCRIPTION: gets current pcb
 *   INPUTS: none
 *   RETURN VALUE: pcb pointer to current pcb
 * SIDE EFFECT: none
 */
pcb_t* get_cur_pcb()
{
  int cur_pcb_pos = (cur_term+1) * pid_active[cur_term];
  return((pcb_t*) (8*MB - (8*KB * (cur_pcb_pos + 1))));
}

/*
 * set_pcb
 *   DESCRIPTION: sets pcb
 *   INPUTS: none
 *   RETURN VALUE: pcb pointer to current pcb
 * SIDE EFFECT: none
 */
void set_pcb()
{
  int cur_pcb_pos = (cur_term+1) * pid_active[cur_term];
  pcb = (pcb_t*) (8*MB - (8*KB * (cur_pcb_pos + 1)));
}

/*
 * get_old_pcb
 *   DESCRIPTION: gets the old pcb
 *   INPUTS: none
 *   RETURN VALUE: pcb pointer to old pcb
 * SIDE EFFECT: none
 */
pcb_t* get_old_pcb()
{
  int old_pcb_pos = (cur_term+1) * pid_old[cur_term];
  return((pcb_t*) (8*MB - (8*KB * (old_pcb_pos + 1))));
}



/*
 * sys_call_read
 *   DESCRIPTION: read data from other read() functions
 *   INPUTS: fd - file directory to be checked for validity - 0 if reading from terminal
             buf - data pointer
             nbytes - number of bytes to be read
 *   RETURN VALUE: number of bytes read, -1 on fail
 * SIDE EFFECT: file_position is updated in dir_read and file_read
 */
int32_t sys_call_read(int32_t fd, void* buf, int32_t nbytes){
	if (fd < 0 || fd > MAX_INDEX || fd == 1)//make sure fd index is within array bounds AND CANNOT BE STDOUT(which is fd 1)
  {
    //printf("yee");
		return -1;
  }

	if (pcb->file_array[fd].flags == UNUSED)//if file is not present we cannot read it
  {
    //printf("yo");
		return -1;
  }


int32_t val = pcb->file_array[fd].jump_table_ptr->read(fd, buf, nbytes);//call appropriate read function from jumptable

if(val > nbytes)
{
  printf("WRONG");
}
	return val;//return number of bytes read
}

/*
 * sys_call_write
 *   DESCRIPTION: writes data to the terminal or to a device
 *   INPUTS: fd - file directory to be checked for validity - 0 if writing to terminal
             buf - data pointer
             nbytes - number of bytes to be written
 *   RETURN VALUE: number of bytes written, -1 on fail
 */
int32_t sys_call_write(int32_t fd, const void* buf, int32_t nbytes){
	if (fd < 1 || fd > MAX_INDEX)//make sure fd index is within the array bounds AND CANNOT BE STDIN(which is fd 0)
		return -1;

	if (pcb->file_array[fd].flags == UNUSED)//if the fd index is unused, no file to write with
		return -1;

	int32_t val = pcb->file_array[fd].jump_table_ptr->write(fd, buf, nbytes);//get number of bytes written, otherwise -1 if fail
  return val;
}

/*
 * sys_call_open
 *   DESCRIPTION: provides access to the file system
 *   INPUTS: filename - specifies PDE to be used
 *   RETURN VALUE: 0 on success, -1 on fail
 */
int32_t sys_call_open(const uint8_t* filename){

  int i;
  dentry_t dentry;
  int array_entry = 0;

  if (!filename || filename[0] == '\0') return -1;

	if (read_dentry_by_name(filename, &dentry) == -1)//if filename does not exist in filesystem, fail
	{
		return -1;
  }

  for (i = 2; i <= MAX_INDEX; i++) {//check fd entries 2-7 for empy spots to open a new program at
		if (pcb->file_array[i].flags == UNUSED) {
			pcb->file_array[i].flags = USED;
			pcb->file_array[i].file_position = 0;
      array_entry = i;
      break;
		}
		else if (i == 7 ) {
			return -1; //no free array entries
		}
	}

  switch (dentry.type)//depending on what type of file(RTC, dir, or file), set jumptables
  //dentry type can be 0, 1 or 2
  //0 = RTC, 1 = dir, 2 = file
	{
		case 0:
			if (0 != open(filename))//return fail if unsuccessful open
      {
				return -1;
      }
			pcb->file_array[array_entry].inode = NULL;//inode is null for RTC
			pcb->file_array[array_entry].jump_table_ptr = &rtc_fn;//set rtc jumptable
			break;
		case 1:
			if (0 != dir_open(filename))//return fail if unsuccessful open
      {
				return -1;
      }
			pcb->file_array[array_entry].inode = NULL;//inode is null for dir
			pcb->file_array[array_entry].jump_table_ptr = &dir_fn;//set dir jumptable
			break;
		case 2:
			if (0 != file_open(filename))//return fail if unsuccessful open
      {
				return -1;
      }
			pcb->file_array[array_entry].inode = dentry.inode;//set correct inode for file
			pcb->file_array[array_entry].jump_table_ptr = &file_fn;//set file jumptable
			break;
	}


return array_entry;

}

/*
 * sys_call_close
 *   DESCRIPTION: closes the specified file descriptor and makes it available
                  for return from later calls to open
 *   INPUTS: fd - descriptor for file to be closed
 *   RETURN VALUE: 0 on success, -1 on fail
 */
int32_t sys_call_close(int32_t fd){
	if (fd < 2 || fd > MAX_INDEX)//check if fd index is within file array
		return -1;

	if (pcb->file_array[fd].flags == UNUSED) //return error if were trying to close a file that was never open
		return -1;

	if (0 != pcb->file_array[fd].jump_table_ptr->close(fd))//call close for file using jumptable, all return 0 on success
		return -1;

  //printf("got here");

	pcb->file_array[fd].flags = UNUSED;//close file(mark as unused)



	return 0;//successful close
}

/*
 * sys_call_getargs
 *   DESCRIPTION: reads the program's command line arguments into a user-level
                  buffer
 *   INPUTS: buf - user level buffer
             nbytes - number of bytes to be copied into user space
 *   RETURN VALUE: 0 on success, -1 on fail
 */
int32_t sys_call_getargs(uint8_t* buf, int32_t nbytes){
  // check if buf is NULL or nbytes is non-positive
  if (pcb->arguments[0] == '\0' || nbytes <= 0) return -1;

  // copy PCB arguments into buf
  int i = 0;
  while (pcb->arguments[i] != '\0') {
    buf[i] = pcb->arguments[i];
    i++;
  }
  buf[i] = '\0';
  return 0;
}

/*
 * sys_call_vidmap
 *   DESCRIPTION: maps the text-mode video memory into user space at a pre-set
                  virtual address
 *   INPUTS: screen_start - address to map memory onto
 *   RETURN VALUE: returns address, -1 on fail
 */
int32_t sys_call_vidmap(uint8_t** screen_start){
  uint32_t address; // virtual address that screen_start points to
  // fail cases
  if (screen_start == NULL) return -1; // if screen_start doesn't exist fail
  address = (uint32_t) screen_start;
  if (address < 128*MB || 132*MB < address) return -1; // if out of bounds fail

  // change paging
  map_page_vidmap();

  // set screen_start to be at 136MB
  *screen_start = (uint8_t*) (132*MB);
  return (132*MB);
}

/*
 * sys_call_set_handler
 *   DESCRIPTION: UNUSED, RETURNS -1
 */
int32_t sys_call_set_handler(int32_t signum, void* handler_address){
    return -1;
}

/*
 * sys_call_sigreturn
 *   DESCRIPTION: UNUSED, RETURNS -1
 */
int32_t sys_call_sigreturn(void){
    return -1;
}

/*
 * sys_call_sigreturn
 *   DESCRIPTION: RETURNS -1
 */
int32_t retfail()
{
  return -1;
}
