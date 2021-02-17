// keyboard.c - defines protocols for keyboard interrupts
#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "types.h"
#include "syscalls.h"

// //terminals array stores all info for every terminal
// term_t terminals[NUM_TERMS];
// //stores current terminal
// int cur_term = 0;

//command array of [#number of cmds][128]
//pos 0,x always holds current kbd buffer
unsigned char cmd_buf[MAX_CMDS][KBD_BUF_LENGTH];
//holds index to current position in the cmd array
int cur_cmd_idx = 0;
//holds index for each cmd in history
int buf_idxs[MAX_CMDS];
//holds number of old cmds in history
int old_cmd_num = 0;

///***LINE LENGTH IS 80 CHARS/LINE***

//intitialize most of key_arr to the printable kbd characters
unsigned char key_arr[256] = {
   0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
   0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,
   0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', SINGLE_QUOTE_THING, '`',
   0, BACKSLASH , 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'
 };

//array to store the shifted varients of some characters
// eg '5' -> '%' when shift is pressed
unsigned char shift_arr[KBD_BUF_LENGTH];

//array to store state of every possible key press
//0x00 means its not pressed and 0x01 means it is currently pressed
unsigned char keys_pressed[KBD_BUF_LENGTH];

//store the state of the caps lock since it toggles every time its pressed
uint8_t caps_lock = 0;

//store video memory info
int screen_x = 0;
int screen_y = 0;
char* video_mem = (char *)VIDEO;

//track when enter is pressed
int enter_pressed = 0;

//allow writing to display or not
int display_typing = 0;

/*
 * keyboard_init
 *   DESCRIPTION: initialize necessary variables for keyboard functionality
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: initialize global variables
 */
void
keyboard_init()
{
  int i,j;
  // fill rest of key_arr with zeros
  for (i=54; i<256; i++){
    key_arr[i] = 0;
  }
  //set the space character in key_arr
  key_arr[SPACE_PRESS] = ' ';
  key_arr[ENTER_PRESS] = '\n';

  //fill shift arry and keys_pressed and buffer with zeros
  for(i = 0; i < KBD_BUF_LENGTH; i++){
    shift_arr[i] = 0;
    keys_pressed[i] = 0;
    for(j = 0; j < MAX_CMDS; j++){
      cmd_buf[j][i] = 0;
    }
  }
  //set shift array positions 1-9 and then 0
  shift_arr['1'] = '!';
  shift_arr['2'] = '@';
  shift_arr['3'] = '#';
  shift_arr['4'] = '$';
  shift_arr['5'] = '%';
  shift_arr['6'] = '^';
  shift_arr['7'] = '&';
  shift_arr['8'] = '*';
  shift_arr['9'] = '(';
  shift_arr['0'] = ')';
  //set shift array for - and =
  shift_arr['-'] = '_';
  shift_arr['='] = '+';
  //set shift array for [ and ] and backslash
  shift_arr['['] = '{';
  shift_arr[']'] = '}';
  shift_arr[BACKSLASH] = '|';
  //set shift array for ; and ' and `
  shift_arr[';'] = ':';
  shift_arr[SINGLE_QUOTE_THING] = '"';
  shift_arr['`'] = '~';
  //set shift array for , and . and /
  shift_arr[','] = '<';
  shift_arr['.'] = '>';
  shift_arr['/'] = '?';

  //init terminals
  init_terminals();

  //disable cursor
  disable_cursor();

  //enable irq for KBD
  enable_irq(KBD_IRQ);
}

/*
 * disable_cursor
 *   DESCRIPTION: disables cursor (from OSDEV)
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sends data to screen
 */
void
disable_cursor()
{
  //send data to screen to disalbe it
  outb(0x0A, 0x3D4);
	outb(0x20, 0x3D5);
}

/*
 * enable_cursor
 *   DESCRIPTION: enables cursor (from OSDEV)
 *   INPUTS: cursor_start and end determine the size of the cursor
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sends data to screen
 */
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x0A, 0x3D4);
	outb((inb(0x3D5) & 0xC0) | cursor_start, 0x3D5);

	outb(0x3D4, 0x0B);
	outb((inb(0x3D5) & 0xE0) | cursor_end, 0x3D5);
}

/*
 * update_cursor
 *   DESCRIPTION: updates the position of the cursor (from OSDEV)
 *   INPUTS: x - x position of the cursor
 *           y - y position of the cursor
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sends data to screen
 */
void update_cursor(int x, int y)
{
	uint16_t pos = y * NUM_COLS + x;

	outb(0x0F, 0x3D4);
	outb( (uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb( (uint8_t) ((pos >> 8) & 0xFF), 0x3D5);
}


/*
 * keyboard_IH
 *   DESCRIPTION: interrupt handler for keyboard press
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: send interrupt to print character on monitor
 */
void
keyboard_IH()
{
  int i;
  int num_spaces;
  // receive key input
  unsigned char key_idx = inb(KBD_PRT);
  //set current state of key press. Use bitmask 0x7F to convert releases into presses
  keys_pressed[key_idx & 0x7F] ^= 0x01;

  //******check keys that do special actions******

  switch (key_idx){
    //check if the key is caps lock
    case CAPS_LOCK_PRESS:
      //if so toggle caps lock and return immediately
      caps_lock ^= 0x01;
      //no need to print the buffer so just return
      //send eoi and return
      send_eoi(KBD_IRQ);
      return;
    //check if the key is backspace
    case BACKSPACE_PRESS:
      //ensure the position is greater or equal to 0
      if(buf_idxs[cur_cmd_idx] > 0){
        //set previous char to null and decrement x position
        cmd_buf[cur_cmd_idx][--buf_idxs[cur_cmd_idx]] = 0x00;
        //delete the char from the screen
        terminal_delc();
      }
      //send eoi and return
      send_eoi(KBD_IRQ);
      return;
    //check if tab is pressed
    case TAB_PRESS:
      //send eoi and return if display writing is disabled currently
      if(!display_typing){
        send_eoi(KBD_IRQ);
        return;
      }
      //get number of spaces to add
      num_spaces = fmin(4, 127 - buf_idxs[cur_cmd_idx]);
      //tab does 4 spaces so loop up to 4 times
      for(i = 0; i < num_spaces; i++){
        //set the ith position after the current to space
        cmd_buf[cur_cmd_idx][buf_idxs[cur_cmd_idx]] = ' ';
        //print char
        terminal_putc(' ');
        //increment position
        buf_idxs[cur_cmd_idx]++;
      }
      //send eoi and return
      send_eoi(KBD_IRQ);
      return;
    case ENTER_PRESS:
      //update enter pressed
      enter_pressed = 1;
      //print newline
      terminal_putc('\n');
      //send eoi and return
      send_eoi(KBD_IRQ);
      return;
    //check if key is up arrow
    case UP_PRESS:
      //send eoi and return
      switch_cmd_buffer(1);
      send_eoi(KBD_IRQ);
      return;
    //check if key is down arrow
    case DOWN_PRESS:
      switch_cmd_buffer(-1);
      //send eoi and return
      send_eoi(KBD_IRQ);
      return;
    default:
      break;
  }

  //******check keys that can be pressed with other keys:*******

  //check if CTRL is pressed
  if(keys_pressed[CTRL_PRESS]){
    //CTRL + L clears screen and the buffer if terminal read is not "in progress"
    if(keys_pressed[L_PRESS]){
      //clear video mem and reset position
      clear_and_reset();
      //clear display buffer if terminal read is not "in progress"
      printf("391OS> ");
      if(enter_pressed){
        clear_kbd_buf();
      }
      else{
        //print message
        //printf("[mp3_group epic terminal]$ ");
        //print the buffer back
        printf("%s", cmd_buf[0]);
        //set the cursor
        update_cursor(screen_x, screen_y);
      }
      //send eoi and return
      send_eoi(KBD_IRQ);
      return;
    }
    //CTRL + G prints out the display buffer
    //this messes with alignment so ENTER + CTRL + L should be done afterwards
    else if(keys_pressed[G_PRESS]){
      //print display buf
      print_kbd_buf();
      //send eoi and return
      send_eoi(KBD_IRQ);
      return;
    }
    else if(keys_pressed[ALT_PRESS]){
      // //check if F4 pressed
      if(keys_pressed[F4_PRESS]) bruh();
      //printf("got herfdsafdsae");
    }
    else if(keys_pressed[C_PRESS]){
      //send eoi
      send_eoi(KBD_IRQ);
      //halt the program
      sys_call_halt(0);
      return;
    }
    //else do nothing
  }
  else if(keys_pressed[ALT_PRESS]){
    //check if F1 1 F12 pressed to switch terminal
     check_fns();
  }

  //******do this for every other key:*******

  //get char from array
  unsigned char key = key_arr[key_idx];
  //if key is not a printable character return immediately
  if(key == 0){
    send_eoi(KBD_IRQ);
    return;
  }
  //check if key is a character in range a-z
  if(key >= 0x61 && key <= 0x7A){
    //find if key needs to be capitalized
    if((keys_pressed[L_SHIFT_PRESS] || keys_pressed[R_SHIFT_PRESS]) ^ caps_lock){
      //capitalize character using bitmask 0x5F
      key &= 0x5F;
    }
  }
  //check if any shift is pressed
  else if(keys_pressed[L_SHIFT_PRESS] || keys_pressed[R_SHIFT_PRESS]){
      //set key to the shifted version
      key = shift_arr[key];
  }

  //print to display buffer if position is less or equal to 126
  //since position 127 is reserved for newline
  if(buf_idxs[cur_cmd_idx] <= 126 && key != 0 && display_typing){
    //print to the display buffer
    cmd_buf[cur_cmd_idx][buf_idxs[cur_cmd_idx]] = key;
    //inc x position
    buf_idxs[cur_cmd_idx]++;
    //print char
    terminal_putc(key);
    //set the cursor
    update_cursor(screen_x, screen_y);
  }

  //send eoi
  send_eoi(KBD_IRQ);
}

/* clear_and_reset;
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory and sets line feed to 0,0 */
void clear_and_reset(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
    screen_x = 0;
    screen_y = 0;
    update_cursor(screen_x, screen_y);
}

/*
 * switch_cmd_buffer
 *   DESCRIPTION: switches up or down to an old or new cmd buffer
 *   INPUTS: n -- number of positions to move
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the current cmd buffer
 */
void switch_cmd_buffer(int n){
  int i;
  //clear the chars on the screen (visually only)
  for(i = 0; i < buf_idxs[cur_cmd_idx]; i++){
    terminal_delc();
  }
  //calculate the new current index
  cur_cmd_idx = fmax(0, cur_cmd_idx + n);
  cur_cmd_idx = fmin(old_cmd_num, cur_cmd_idx);

  //reprint the buffer
  for(i = 0; i < buf_idxs[cur_cmd_idx]; i++){
    terminal_putc(cmd_buf[cur_cmd_idx][i]);
  }
  //update the cursor
  update_cursor(screen_x, screen_y);
}

/*
 * shift_old_cmds
 *   DESCRIPTION: shifts all the cmds up 1 position
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the current cmd buffer
 */
void shift_old_cmds(){
  int i,j;
  //shift cmd farther
  for(i = MAX_CMDS - 1; i >= 1; i--){
    for(j = 0; j < KBD_BUF_LENGTH; j++){
      cmd_buf[i][j] = cmd_buf[i - 1][j];
      buf_idxs[i] = buf_idxs[i - 1];
    }
  }
}

/*
 * print_buffer
 *   DESCRIPTION: prints the display buffer to the screen and the current
 *                position within the buffer. Prints the kbd buffer
 *                within the 2 '*' characters. Any newlines are
 *                printed as '@'.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: writes to video memory, does NOT interface with lib.c
 *                 so doing lib.c functions will likely print in a
 *                 completely different area
 */
void print_kbd_buf(){
  int i;
  //print newline
  terminal_putc('\n');
  //print starting char
  terminal_putc('*');
  //loop and putc
  for(i = 0; i < buf_idxs[cur_cmd_idx] + 1; i++){
    //break if null hit
    if(cmd_buf[cur_cmd_idx][i] == 0) break;
    //print special char @ if newline in buffer
    if(cmd_buf[cur_cmd_idx][i] == '\n'){
      terminal_putc('@');
    }
    else{
      //print char
      terminal_putc(cmd_buf[cur_cmd_idx][i]);
    }
  }
  //print ending char
  terminal_putc('*');
  //print index of buffer
  printf("\nPosition: %d", buf_idxs[cur_cmd_idx]);
}

/*
 * clear_kbd_buf
 *   DESCRIPTION: clears the display buffer
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: clears the display buffer
 */
void clear_kbd_buf(){
  int i;
  //loop and set the buffer to null
  for(i = 0; i < KBD_BUF_LENGTH; i++){
    cmd_buf[cur_cmd_idx][i] = 0;
  }
  //set x position to 0
  buf_idxs[cur_cmd_idx] = 0;
}

/*
 * terminal_putc
 *   DESCRIPTION: puts a char to the terminal and interfaces with the buffer position
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: writes to video memory
 */
void terminal_putc(uint8_t c){
  //check if newline
  if(c == '\n' || c == '\r') {
      screen_y++;
      screen_x = 0;
  }
  //otherwise print it
  else {
      *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
      *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
      screen_x++;
      //if line reaches the end of the screen
      if(screen_x >= NUM_COLS){
        screen_x = 0;
        screen_y++;
      }
  }
  //need scrolling
  if(screen_y >= NUM_ROWS){
    scroll_terminal(1);
  }
}

/*
 * scroll_terminal
 *   DESCRIPTION: scrolls the terminal upwards n positions, resets x and y positional data
 *   INPUTS: n - number of times to scroll the screen
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: writes to video memory
 */
void scroll_terminal(int n){
  int x;
  int y;
  //reset x and set the new y position
  screen_y -= n;
  screen_x = 0;
  //move every row up n positions
  for(y = n; y < NUM_ROWS; y++){
    for(x = 0; x < NUM_COLS; x++){
      *(uint8_t *)(video_mem + ((NUM_COLS * (y - n) + x) << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1));
      *(uint8_t *)(video_mem + ((NUM_COLS * (y - n) + x) << 1) + 1) = ATTRIB;
    }
  }
  //clear all rows after and including screen_y
  for(y = screen_y; y < NUM_ROWS; y++){
    for(x = 0; x < NUM_COLS; x++){
        *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1)) = 0x00;
        *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1) + 1) = ATTRIB;
    }
  }
}

/*
 * terminal_delc
 *   DESCRIPTION: does what backspace does
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: writes to video memory
 */
void terminal_delc(){
  //decrement screen_x
  screen_x--;
  //reset x and y if backspace goes past edge
  if(screen_x < 0){
    screen_x = 79;
    screen_y = fmax(--screen_y, 0);
  }
  //remove char from screen
  *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = 0x00;
  *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;

  //update cursor
  update_cursor(screen_x, screen_y);
}

/*
 * terminal_open
 *   DESCRIPTION: clears screen and opens terminal, turns on screen printing
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: clears video memory and kbd buffer
 */
int32_t terminal_open(const uint8_t* filename){
  //clear screen
  clear_and_reset();
  //clear kbd buffer
  clear_kbd_buf();
  //turn on display printing
  //display_typing = 1;
  //printf("Enter something: ");
  return(0);
}

/*
 * terminal_close
 *   DESCRIPTION: clears screen and opens terminal, turns on screen printing
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: clears video memory and kbd buffer
 */
int32_t terminal_close(int32_t fd){
  //clear screen
  clear_and_reset();
  //clear kbd buffer
  clear_kbd_buf();
  //turn off display printing
  display_typing = 0;
  return(0);
}


/*
 * terminal_write
 *   DESCRIPTION: writes n bytes into the display buffer from a given buffer
 *   INPUTS: fd - 1
 *           buf - buf to write from
 *           nbytes - number of bytes to write
 *   OUTPUTS:
 *   RETURN VALUE: 0 upon successful write, -1 if nothing written or if bad inputs,
 *                 -1 if nbytes is larger than KBD_BUF_LENGTH
 *   SIDE EFFECTS: writes to kbd buffer
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
  int i;
  unsigned char c;
  unsigned char * buf2 = ((unsigned char*)buf);
  //return -1 if nbytes is out of the range [0, KBD_BUF_LENGTH] or if buf is null
  //if(nbytes <= 0 || nbytes > KBD_BUF_LENGTH || buf2 == NULL) return -1;
  if(nbytes <= 0 || buf2 == NULL) return -1;
  //clear buffer
  clear_kbd_buf();
  //loop nbytes number
  for(i = 0; i < nbytes; i++){
    c = buf2[i];
    //print char
    if(c != 0x00){
      terminal_putc(c);
    }
  }
  //set newline at the end of the buffer
  //cmd_buf[cur_cmd_idx][fmin(buf_idxs[cur_cmd_idx], 127)] = '\n';
  //return 0 if write is completely successful
  return nbytes;
}

/*
 * terminal_read
 *   DESCRIPTION: reads input of terminal until newline pressed and writes this to buf
 *   INPUTS: fd - 1
 *           buf - buffer to write to
 *           nbytes - number of bytes to read
 *   OUTPUTS: writes nbytes to buffer
 *   RETURN VALUE: -1 if inputs are bad, 0 if successful
 *   SIDE EFFECTS: none
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
  unsigned char c;
  int i,j;
  unsigned char* buf2 = ((unsigned char*)buf);
  //check inputs
  //return -1 if nbytes is out of the range [0, KBD_BUF_LENGTH]
  //if(buf == NULL || nbytes <= 0 || nbytes > KBD_BUF_LENGTH) return -1;
  if(buf == NULL || nbytes <= 0) return -1;
  //sti();
  //test_interrupts();
  //enable cursor
  enable_cursor(0, 0);
  //enable typing
  display_typing = 1;
  //start with enter pressed being 0 so CTRL + L doesnt mess with this
  enter_pressed = 0;
  //clear buffer
  clear_kbd_buf();
  //set the cursor
  update_cursor(screen_x, screen_y);
  //loop until enter pressed or number bytes written goes out of bounds
  while(1){
    //if enter gets pressed
    if(enter_pressed == 1){
      //copy the kbd buffer into the given buffer
      for(i = 0; i < fmin(KBD_BUF_LENGTH, nbytes); i++){
        c = cmd_buf[cur_cmd_idx][i];
        //copy current position
        buf2[i] = c;
        //stop if null or if end of bytes number is hit
        if(i == 127 || i == nbytes - 1 || c == '\0'){
          //set last byte to newline
          buf2[i] = '\n';
          break;
        }
      }

      //***** STUFF THAT HAPPENS AFTER BUF IS COPEID TO USER*******

      //do this if not null command
      if(cmd_buf[cur_cmd_idx][0] != '\0' && cmd_buf[cur_cmd_idx][1] != '\0'){
        //increment the old cmd counter
        old_cmd_num = fmin(MAX_CMDS - 1, old_cmd_num + 1);
        //do this if a past cmd was entered
        if(cur_cmd_idx != 0){
          //loop through all 128 chars
          for(j = 0; j < KBD_BUF_LENGTH; j++){
            //set each spot at 0 to the cmd that was used in order to copy it to history
            cmd_buf[0][j] = cmd_buf[cur_cmd_idx][j];
            buf_idxs[0] = buf_idxs[cur_cmd_idx];
          }
          //reset the buffer array position back to 0
          cur_cmd_idx = 0;
        }
        //shift historical cmds upwards
        shift_old_cmds();
      }
      //disable typing
      display_typing = 0;
      //disable cursor
      disable_cursor();
      //return number of bytes read
      return(i + 1);
    }
  }
  //disable typing
  display_typing = 0;
  //disable cursor
  disable_cursor();
  return 0;
}

/*
 * check_fns
 *   DESCRIPTION: checks if fn key was pressed and switches terminal
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void check_fns()
{
  //local var for current terminal
  term_t *current_term, *new_term;
  int i,j,k;

  //printf("got here\n");

  for(i = 0; i < NUM_TERMS + 1; i++){
    //check if none of the keys were pressed
    if(i == NUM_TERMS) return;
    //find if any f key is pressed (not f10 or f12)
    if(keys_pressed[F1_PRESS + i]) break;
  }

  //get ptrs to current and new terminal structs
  current_term = &(terminals[cur_term]);
  new_term = &(terminals[i]);
  //check if they are equal; do nothing if so
  if(current_term == new_term) return;

  //update cur_term
  cur_term = i;
  set_pcb();

  //*******SAVE CURRENT VARIABLES FOR CURRENT TERMINAL*********

  //save current terminal's local vars to current terminal struct
  current_term->term_cur_cmd_idx = cur_cmd_idx;
  current_term->term_old_cmd_num = old_cmd_num;
  current_term->term_screen_x = screen_x;
  current_term->term_screen_y = screen_y;
  current_term->term_enter_pressed = enter_pressed;
  current_term->term_display_typing = display_typing;
  //save current terminal's old cmds
  for(j = 0; j < MAX_CMDS; j++){
    for(k = 0; k < KBD_BUF_LENGTH; k++){
      current_term->term_cmd_buf[j][k] = cmd_buf[j][k];
    }
  }
  //save current buf idxs
  for(i = 0; i < MAX_CMDS; i++){
    current_term->term_buf_idxs[i] = buf_idxs[i];
  }
  //save current video memory
  for(j = 0; j < NUM_COLS; j++){
    for(k = 0; k < NUM_ROWS; k++){
      current_term->term_screen[j][k] = *(uint8_t *)(video_mem + ((NUM_COLS * k + j) << 1));
    }
  }

  //*******LOAD NEW VARIABLES FOR NEW TERMINAL*********

  //load new terminal's local vars
  cur_cmd_idx = new_term->term_cur_cmd_idx;
  old_cmd_num = new_term->term_old_cmd_num;
  screen_x = new_term->term_screen_x;
  screen_y = new_term->term_screen_y;
  enter_pressed = new_term->term_enter_pressed;
  display_typing = new_term->term_display_typing;
  //load new terminal's old cmds
  for(j = 0; j < MAX_CMDS; j++){
    for(k = 0; k < KBD_BUF_LENGTH; k++){
      cmd_buf[j][k] = new_term->term_cmd_buf[j][k];
    }
  }
  //load new buf idx's
  for(i = 0; i < MAX_CMDS; i++){
    buf_idxs[i] = new_term->term_buf_idxs[i];
  }
  //load new video memory
  for(j = 0; j < NUM_COLS; j++){
    for(k = 0; k < NUM_ROWS; k++){
      *(uint8_t *)(video_mem + ((NUM_COLS * k + j) << 1)) = new_term->term_screen[j][k];
      *(uint8_t *)(video_mem + ((NUM_COLS * k + j) << 1) + 1) = ATTRIB;
    }
  }

  //update le cursor
  update_cursor(screen_x, screen_y);

  //if the new terminal has not had a shell, execute a new one
  if(!(new_term->term_has_shell)){
    //update shell tracker
    new_term->term_has_shell = 1;
    //send eoi
    send_eoi(KBD_IRQ);
    //execute new shell
    sys_call_execute((uint8_t*)"shell");
  }
  else{
    //switch_term_back();
    //save ebp/esp for current terminal
    asm volatile("mov %%esp, %%ecx \n\
                  mov %%ebp, %%ebx \n\
                  "
                : "=c" (current_term->term_stack_ptr), "=b" (current_term->term_base_ptr));

    //load ebp/esp for new terminal
    asm volatile("mov %%ecx, %%esp \n\
                  mov %%ebx, %%ebp \n\
                  "
                : "=c" (new_term->term_stack_ptr), "=b" (new_term->term_base_ptr));

    //asm volatile("jmp context_switch");
  }
  //send eoi
  send_eoi(KBD_IRQ);
}

/*
 * init_terminals
 *   DESCRIPTION: initializes terminals
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_terminals()
{
  int i,j,k;
  //loop through all terminals
  for(i = 0; i < NUM_TERMS; i++){
    //init all vars that need to be zero
    terminals[i].term_cur_cmd_idx = 0;
    terminals[i].term_old_cmd_num = 0;
    terminals[i].term_screen_x = 0;
    terminals[i].term_screen_y = 0;
    terminals[i].term_enter_pressed = 0;
    terminals[i].term_display_typing = 0;
    terminals[i].term_has_shell = 0;

    //clear screen array for terminal
    for(j = 0; j < NUM_COLS; j++){
      for(k = 0; k < NUM_ROWS; k++){
        terminals[i].term_screen[j][k] = 0;
      }
    }

    //clear cmd buf for terminal
    for(j = 0; j < MAX_CMDS; j++){
      for(k = 0; k < KBD_BUF_LENGTH; k++){
        terminals[i].term_cmd_buf[j][k] = 0;
      }
    }

    // //clear pid arr
    // for(j = 0; j < NUM_TERMS; j++){
    //   for(k = 0; k < MAX_PROCESS_NUM; k++){
    //     terminals[i].term_pid_arr[j][k] = 0;
    //   }
    // }

    //init cur_term
    cur_term = 0;
  }
  //set the first terminal to have a shell opened
  terminals[0].term_has_shell = 1;
}

// /*
//  * get_cur_term
//  *   DESCRIPTION: gets the current terminal num
//  *   INPUTS: none
//  *   OUTPUTS: current terminal num
//  *   RETURN VALUE: current terminal num
//  *   SIDE EFFECTS: none
//  */
// int get_cur_term()
// {
//   return(cur_term);
// }

/*
 * bruh
 *   DESCRIPTION: [REDACTED]
 *   INPUTS: [REDACTED]
 *   OUTPUTS: [REDACTED]
 *   RETURN VALUE: [REDACTED]
 *   SIDE EFFECTS: [REDACTED]
 */
void bruh(){
  int i, k;
  for(i = 0; i < 20000; i++){
    for(k = 0; k < 3; k++){
      printf("LOL GET REKT!!!!");
    }
    printf("LOL GET REKT!!!");
  }
  for(i = 0; i < 20000; i++){
    test_interrupts();
  }
  clear_and_reset();
  printf("YOURE STUPID EXCEPTION");
  while(1);
}
