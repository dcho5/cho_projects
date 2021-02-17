// rtc.h - declares protocols for rtc interrupts
#ifndef _RTC_H
#define _RTC_H

#define RTC_IRQ               0x08
#define MAX_FREQ              1024

#include "types.h"

// initialize necessary variables for rtc functionality
void rtc_init();
// interrupt handler for rtc
void rtc_IH();

// reset the frequency to 2Hz
int32_t open (const uint8_t* filename);
// notification that interrupt happened
int32_t read (int32_t fd, void* buf, int32_t nbytes);
// change refresh rate
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
// loses the specified file desriptor and makes it available for return
// from later calls to open
int32_t close (int32_t fd);

// helper function to set RTC interrupt rate
void set_freq (int32_t freq);

// rtc_test function for CP2
void rtc_test (char writing, int32_t freq);
// rtc_test_cycle function for CP2
void rtc_test_cycle ();

#endif //_RTC_H
