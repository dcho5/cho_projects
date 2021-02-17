#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#include "keyboard.h"
// rtc.c - defines protocols for rtc interrupts

// TURN OFF/ON (0/1) VIRTUALIZATION
#define VIRTUALIZE            0

// RTC interrupt flag/counter (0/1)
int32_t int_occurred;

// 1 or MAX_FREQ / FREQ (0 or 1)
int32_t x; // V

/*
 * rtc_init
 *   DESCRIPTION: initialize necessary variables for rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: initialize global variables
 */
void
rtc_init()
{
  // reset interrupt flag
  int_occurred = 0;

  x = 1;

  if (VIRTUALIZE) set_freq(MAX_FREQ); // V

  // from OSDEV
  disable_irq(RTC_IRQ);
  outb(0x8B, 0x70);
  char prev = inb(0x71);
  outb(0x8B, 0x70);
  outb(prev|0x40, 0x71);
  enable_irq(RTC_IRQ);
}

/*
 * rtc_IH
 *   DESCRIPTION: interrupt handler for rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: send interrupt for rtc
 */
void
rtc_IH()
{
  send_eoi(RTC_IRQ);
  if (VIRTUALIZE) int_occurred++; // V
  else int_occurred = 1;

  // from OSDEV
  outb(0x0C, 0x70);
  inb(0x71);
}

/*
 * open
 *   DESCRIPTION: set the RTC interrupt rate at 2Hz
 *   INPUTS: unused
 *   OUTPUTS: none
 *   RETURN VALUE: 0 always?
 *   SIDE EFFECTS: sets rate to 2
 */
int32_t
open (const uint8_t* filename)
{
  // reset the frequency to 2Hz
  if (VIRTUALIZE) x = MAX_FREQ / 2; // V
  else set_freq(2);
  int_occurred = 0;
  return 0;
}

/*
 * read
 *   DESCRIPTION:
 *   INPUTS: fd - unused
             buf - unused
             nbytes - unused
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:
 */
int32_t
read (int32_t fd, void* buf, int32_t nbytes)
{
  // only return once the RTC interrupt occurs
  while (int_occurred != x) {}
  // reset flag
  int_occurred = 0;
  return 0;
}

/*
 * write
 *   DESCRIPTION: set the rate of periodic interrupts
 *   INPUTS: fd - unused
             buf - pointer to interrupt rate
             nbytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: 4 if successful, -1 if fail
 *   SIDE EFFECTS: sets rate to nbytes
 */
int32_t
write (int32_t fd, const void* buf, int32_t nbytes)
{

  // nbytes must be 4
  if (nbytes != 4) return -1;

  // buffer for frquency
  int32_t freq = *((int32_t*)buf);
  // if freq is 0 or >1024 it's invalid
  if (freq == 0 || freq > 1024) return -1;
  // check if power of two (stack overflow)
  if (freq & (freq-1)) return -1;

  // set rate and return # of bytes written (4)
  freq = *((int32_t*)buf);
  if (VIRTUALIZE) x = MAX_FREQ / freq; // V
  else set_freq(freq);
  return 4;
}

/*
 * close
 *   DESCRIPTION: loses the specified file desriptor and makes it available
                  for return from later calls to open
 *   INPUTS: fd - unused
 *   OUTPUTS: none
 *   RETURN VALUE: 0 always
 *   SIDE EFFECTS:
 */
int32_t
close (int32_t fd)
{
  return 0;
}

/*
 * set_rate
 *   DESCRIPTION: code to actually set the RTC refresh rate
 *   INPUTS: frequency - desired frequency in Hz
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets the RTC interrupt rate
 */
void
set_freq (int32_t freq) {
  // frequency = 32768 >> (rate-1);
  int32_t rate = 3;
  while (freq != 32768 >> (rate-1) && rate < 15) rate++;

  // from OSDEV
  rate &= 0x0F;			// rate must be above 2 and not over 15
  disable_irq(RTC_IRQ);
  outb(0x8A, 0x70);		// set index to register A, disable NMI
  char prev = inb(0x71);	// get initial value of register A
  outb(0x8A, 0x70);		// reset index to A
  outb((prev & 0xF0) | rate, 0x71); //write only our rate to A. Note, rate is the bottom 4 bits.
  enable_irq(RTC_IRQ);
}

/*
 * TEST FUNCTION FOR RTC CHECKPOINT 2
 * INPUTS: writing - 0 to test open() and 1 to test write()
           freq - desired frequency if writing
 */
void rtc_test (char writing, int32_t freq)
{
  const uint8_t* filename; // dummy for filename input
  int32_t fd = 0; // dummy for fd input
  int32_t* buf = &freq;
  int32_t nbytes = 4;

  open(filename);
  if (writing){
    if (write(fd, buf, nbytes) == -1) {
      printf("INVALID FREQUENCY");
      return;
    }
  }

  while (1) {
    read(fd, buf, nbytes);
    printf("%x", VIRTUALIZE);
  }
}

/*
 * TEST FUNCTION FOR RTC CHECKPOINT 2
 */
void rtc_test_cycle ()
{
  int32_t i = 0; // counter
  int32_t fd = 0; // dummy for fd input
  int32_t freq = 2; // initial freq
  int32_t* buf = &freq;
  int32_t nbytes = 4;

  while (1) {
    if (i%20 == 0 && freq != 1024) {
      write(fd, buf, nbytes);
      freq = freq << 1;
    }
    read(fd, buf, nbytes);
    printf("%x", VIRTUALIZE);
    i++;
  }
}
