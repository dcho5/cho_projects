#include "paging.h"
#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "syscalls.h"

// VIDEO memory value found in lib.c
#define VIDEO 0xB8000

/*
 * paging_init
 *   DESCRIPTION: initialize paging
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes PD & PT and turns on paging
 */
void
paging_init()
{
  int i;
  // initialize PD & PT
  for (i=0; i<NUM_ENTRIES; i++) {
    page_directory[i].bits = 0;
    page_directory[i].read_and_write = 1;
    page_directory[i].supervisor = 1;
    page_table[i].bits = 0;
    page_table[i].read_and_write = 1;
    page_table[i].supervisor = 1;
  }

  // page_table points to video memory
  page_table[VIDEO>>12].bits = VIDEO;
  page_table[VIDEO>>12].present = 1;
  page_table[VIDEO>>12].read_and_write = 1;
  // page_table[VIDEO>>12].global = 1;

  // first pde should be a pointer to the page table
	page_directory[0].bits = (uint32_t)page_table;
  page_directory[0].present = 1;
  page_directory[0].read_and_write = 1;
  // page_directory[0].global = 1;

  // second pde pointing to address 4MB
	page_directory[1].bits = 0x400000;
  page_directory[1].present = 1;
  page_directory[1].read_and_write = 1;
  page_directory[1].supervisor = 1;
  page_directory[1].page_size = 1; // enable 4MB size page
  // page_directory[1].global = 1;

/*
  ***Each operation involves using EAX as a buffer***

  SET CR3 AS PAGE_DIRECTORY
  "movl $page_directory, %eax;" "andl $0xFFFFFFE7, %eax;" "movl %eax, %cr3;"

  SET BIT 4 OF CR4 TO 1 TO ENABLE PAGE SIZE EXTENSION
  "movl %cr4, %eax;" "andl $0x00000010, %eax;" "movl %eax, %cr4;"

  SET BIT 5 OF CR4 TO 0 TO DISABLE PHYSICAL ADDRESS SIZE EXTENSION
  "movl %cr4, %eax;" "andl $0xFFFFFFDF, %eax;" "movl %eax, %cr4;"

  SET BIT 31 OF CR0 TO 1 TO ENABLE PAGING
  "movl %cr0, %eax;" "orl $0x80000000, %eax;" "movl %eax, %cr0;"
*/
  asm(
    "movl $page_directory, %eax;"
	  // "andl $0xFFFFFFE7, %eax;"
    "movl %eax, %cr3;"
    "movl %cr4, %eax;"
    "orl $0x00000010, %eax;"
    "movl %eax, %cr4;"
    "movl %cr4, %eax;"
    "andl $0xFFFFFFDF, %eax;"
    "movl %eax, %cr4;"
    "movl %cr0, %eax;"
    "orl $0x80000000, %eax;"
    "movl %eax, %cr0;"
  );
}

/*
 * map_page
 *   DESCRIPTION: map virtual address to physical address for 4MB page
 *   INPUTS: process_num -- current process number
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes PD & PT and turns on paging
 */
void
map_page(int process_num)
{
    // index is 32 becuase 128 MB page directory / 4 MB pages
    //page_directory[32].pb_address = 8 * MB + 4 * MB * process_num;
    page_directory[32].bits = 8 * MB + 4 * MB * process_num;
    page_directory[32].page_size = 1; // b/c 4MB pages
    page_directory[32].read_and_write = 1;
    page_directory[32].present = 1;
    page_directory[32].supervisor = 1;

    // Flush TLB - OSDEV
    asm volatile (
      "movl	%cr3, %eax;"
      "movl	%eax, %cr3;"
    );
}

/*
 * map_page_vidmap
 *   DESCRIPTION: map virtual address to physical address for 4KB page
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes PD & PT and turns on paging
 */
void
map_page_vidmap()
{
// initialize 4KB page
  page_table[0].bits = VIDEO;
  page_table[0].supervisor = 1;
  page_table[0].read_and_write = 1;
  page_table[0].present = 1;

  // index is 33 because 132 MB page directory / 4 MB pages
  page_directory[33].bits = (uint32_t) page_table; // points to page_table
  page_directory[33].supervisor = 1;
  page_directory[33].read_and_write = 1;
  page_directory[33].present = 1;

  // Flush TLB - OSDEV
  asm volatile (
    "movl    %cr3, %eax;"
    "movl    %eax, %cr3;"
  );
}
