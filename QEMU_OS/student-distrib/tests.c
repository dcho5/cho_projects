#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "keyboard.h"
#include "filesystem.h"
#include "syscalls.h"

#include "rtc.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}


/* Tries to dereference a page on the edge, should not be able to
* expected output page fault
*	no inputs
* no side effects
* coverage- paging init and IDT
*/
void paging_test(){
	TEST_HEADER;

	int* ptr = (int*)(0xb7FFF);
	int testVar;
	testVar = *(ptr);
}
/* Tries to derefernce a pointer that points to a memory address in video memory
*	should output Pass
*	No inputs
*	no side effects
*	coverage- paging
*/
int paging_test2(){
	TEST_HEADER;

	int* ptr = (int*)(0xb9000);
	// int* ptr = (int*)(0x800000);
	int testVar;
	testVar = *(ptr);
	return PASS;
}

void exception_test_1(){
	asm volatile("int $0");
}
void exception_test_2(){
	asm volatile("int $2");
}
void exception_test_3(){
	asm volatile("int $4");
}
void exception_test_4(){
	asm volatile("int $10");
}
void exception_test_5(){
	int n = 3;
	int d = 0;
	printf("%x", n/d);
}

void sys_call_test(){
	asm volatile("int $0x80");
}

// add more tests here

/* Checkpoint 2 tests */

//read test with full size buffer
void terminal_driver_read_test1(){
	int i;
	unsigned char buf[128];
	//set entire buffer to 0
	for(i = 0; i < 128; i++){
		buf[i] = 0;
	}
	//open terminal
	terminal_open(0);
	//attempt to read once
	terminal_read(1, buf, 128);
	//put newline before continuing
	terminal_putc('\n');

	for(i = 0; i < 128; i++){
		if(buf[i] == 0x00) break;
		if(buf[i] == '\n'){
			terminal_putc('@');
		}
		else{
			terminal_putc(buf[i]);
		}
	}
}


//read test with small buffer (size = 50)
int terminal_driver_read_test2(){
	TEST_HEADER;

	int i;
	int char_count = 0;
	unsigned char buf[50];
	//set entire buffer to 0
	for(i = 0; i < 50; i++){
		buf[i] = 0;
	}
	//open terminal
	terminal_open(0);
	//attempt to read once
	terminal_read(1, buf, 50);
	//put newline before continuing
	terminal_putc('\n');

	for(i = 0; i < 50; i++){
		if(buf[i] == 0x00) break;
		if(buf[i] == '\n'){
			terminal_putc('@');
		}
		else{
			terminal_putc(buf[i]);
		}
		char_count++;
	}
	//printf("\n%d", char_count);
	if(char_count <= 50) return PASS;
	return FAIL;
}

/*
*
*
*
* tests the dir_read function which uses the read_dentry_by_index function
*/

void print_dir() {
	TEST_HEADER;

	int32_t ret;
	int32_t i;
	int32_t filename_len = 32;
	int32_t maxfiles = 63;
	char buf[filename_len+1];
	for(i = 0; i < maxfiles; i++) {
		ret = dir_read(i, buf, filename_len);
		if(ret == 0)
		{
			return;
		}
		printf(buf);
		printf("\n");
	}
}

/*
*
*	no inputs
* no side effects
* can print the contents of any of the files and gives the amount of bytes
* tests the read_data and read_dentry_by_name functions
*/
void print_exefile(){
	TEST_HEADER;

	uint8_t buf[6000]; //big buffer so we dont leave off any parts of the file, and if buffer isnt big enough, make it bigger.
 	uint32_t i;
 	uint32_t length;
	int8_t* targetFile = "ls";

 	length = file_read((int32_t)targetFile,buf,6000);
 	printf("File Size: %d bytes\n", length);
  	for(i=0; i< length; i++)
	{
		if(buf[i]!= NULL)
	  	{
  			putc(buf[i]);
	  	}

 	}
}

/*
*
*	no inputs
* no side effects
* can print the contents of any of the files and gives the amount of bytes
* tests the read_data and read_dentry_by_name functions
*/
void print_smalltxtfile(){
	TEST_HEADER;

	uint8_t buf[6000]; //big buffer so we dont leave off any parts of the file, and if buffer isnt big enough, make it bigger.
 	uint32_t i;
 	uint32_t length;
	int8_t* targetFile = "frame0.txt";
 	length = file_read((int32_t)targetFile,buf,6000);
 	printf("File Size: %d bytes\n", length);
  	for(i=0; i< length; i++)
	{
		if(buf[i]!= NULL)
	  	{
  			putc(buf[i]);
	  	}

 	}
}

void print_largetxtfile(){
	TEST_HEADER;

	uint8_t buf[6000]; //big buffer so we dont leave off any parts of the file, and if buffer isnt big enough, make it bigger.
 	uint32_t i;
 	uint32_t length;
	int8_t* targetFile = "verylargetextwithverylongname.tx";
 	length = file_read((int32_t)targetFile,buf,6000);
 	printf("File Size: %d bytes\n", length);
  	for(i=0; i< length; i++)
	{
		if(buf[i]!= NULL)
	  	{
  			putc(buf[i]);
	  	}

 	}
}

//read test with full size buffer
void terminal_driver_write_test1(){
	int i;
	unsigned char buf[128] = {'C', 'a', 'l', 'e', 'b', ' ', 'C', 'o', 'l', 'e'};
	//set entire buffer to 0
	for(i = 10; i < 128; i++){
		buf[i] = 0;
	}
	//open terminal
	terminal_open(0);
	//print message
  	printf("[mp3_group TERMINAL INPUT]$ ");
	//write to kbd buffer
	terminal_write(1, buf, 128);
}

//scroll test for 1 long print chain with size 1 scrolls
void scroll_test1(){
	terminal_open(0);
	int i;
	for(i = 0; i < 100; i++){
		printf("i == %d\n", i);
	}
}

//scroll test for multiple prints and multiple scroll sizes
void scroll_test2(){
	terminal_open(0);
	int i;
	for(i = 0; i < 20; i++){
		printf("i == %d\n", i);
	}
	scroll_terminal(5);
	printf("END1\n", i);

	for(i = 0; i < 5; i++){
		printf("j == %d\n", i);
	}
	scroll_terminal(3);
	printf("END2\n", i);
}

void terminal_test_big_input(){
	int i;
	int32_t ret;
	unsigned char buf[1024];
	//set entire buffer to 0
	for(i = 0; i < 1024; i++){
		buf[i] = 0;
	}
	//open terminal
	terminal_open(0);
	//loop inputs
	while(1){
		printf("\n[mp3_group epic terminal]$ ");
		ret = terminal_read(1, buf, 1024);
		printf("You wrote: ");
		terminal_write(1, buf, 1024);
		printf("%d bytes were written\n", ret);
		if (ret > 0 && '\n' == buf[ret - 1]){
			printf("ret correct\n");
		}
	}
}

void terminal_test_small_input(){
	int i;
	int32_t ret;
	unsigned char buf[20];
	//set entire buffer to 0
	for(i = 0; i < 20; i++){
		buf[i] = 0;
	}
	//open terminal
	terminal_open(0);
	//loop inputs
	while(1){
		printf("\n[mp3_group epic terminal]$ ");
		ret = terminal_read(1, buf, 20);
		printf("You wrote: ");
		terminal_write(1, buf, 20);
		printf("%d bytes were written\n", ret);
		if (ret > 0 && '\n' == buf[ret - 1]){
			printf("ret correct\n");
		}
	}
}

void rtc_open() {
	rtc_test(0,0);
}

void rtc_m2() {
	rtc_test(1,-2);
}

void rtc_2() {
	rtc_test(1,2);
}

void rtc_1024() {
	rtc_test(1,1024);
}

void rtc_2048() {
	rtc_test(1,2048);
}

void rtc_511() {
	rtc_test(1,511);
}

/* Checkpoint 3 tests */

void shell(){
	// if( == -1){
	// 	printf("returned -1");
	// }
	sys_call_execute((uint8_t*)"shell");
}

void testprint(){
	sys_call_execute((uint8_t*)"testprint");
}



/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// clear_and_reset();

	/* TERMINAL TESTS */
	// terminal_test_small_input();
	// terminal_test_big_input();
	// terminal_driver_read_test1();
	// printf(terminal_driver_read_test2());
	// TEST_OUTPUT("terminal_driver_read_test2", terminal_driver_read_test2());
	// scroll_test1();
	// scroll_test2();

	/* FILE SYSTEM TESTS */
	// print_dir();
	// print_smalltxtfile();
	// print_largetxtfile();
	// print_exefile();

	/* RTC TESTS */
	// rtc_open();
	// rtc_m2();
	// rtc_2();
	// rtc_1024();
	// rtc_2048();
	// rtc_511();
	// rtc_test_cycle();
	// Test with whatever you'd like (open[0]/write[1], frequency if writing)
	// rtc_test(1,256);

	/*Checkpoint 3 tests */
	shell();
	// testprint();
	//sys_call_execute((uint8_t*)"ls");
}
