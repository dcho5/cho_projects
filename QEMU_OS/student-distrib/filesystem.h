/*
*   declares filesystem functions
*
*/


#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "types.h"
#include "lib.h"

#define block_size 4096
#define filename_size 32

typedef struct dblock_t {
	uint8_t data[block_size];
} dblock_t;

typedef struct dentry_t {
    uint8_t filename[32];
    uint32_t type;
    uint32_t inode;
    uint8_t reserved[24];
} dentry_t;

typedef struct inode_t {
    uint32_t length;
    uint32_t data[1023];
} inode_t;

typedef struct bootblock_t {
    uint32_t dentry_num;
    uint32_t inode_num;
    uint32_t dblock_num;
    uint8_t reserved[52];
    dentry_t dentry_data[63];
} bootblock_t;



//gets starting address of file system and intializes
void filesys_init();

//opens a file, returns 0(pass) or -1(fail)
int32_t file_open(const uint8_t* filename);

//closes a file, returns 0(pass) or -1(fail)
int32_t file_close(int32_t fd);

//writes to a file, returns 0(pass) or -1(fail)
//currently writing is not enabled so this will always return -1(fail)
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

//reads nbytes from a file into the buf and returns the number of bytes read
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
//had to change up the header for reading a file, will adapt it to be useable form the system call read
//int32_t file_read(int8_t * fname, uint32_t offset, uint8_t * buf, uint32_t length);



//opens a direcory, returns 0(pass) or -1(fail)
int32_t dir_open(const uint8_t* filename);

//closes a directory, returns 0(pass) or -1(fail)
int32_t dir_close(int32_t fd);

//writes to a directory, returns 0(pass) or -1(fail)
//currently writing is not enabled so this will always return -1(fail)
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

//reads nbytes from a directory into the buf and returns 0(pass) or -1(fail)
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);



//given a file name, searches the directory and returns dentry of the file if it exists
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);

//given an index, searches through the dentries and returns dentry of that index
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

//given an inode, reads length amount of bytes into buf from the start of the datablocks + offset
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

#endif 
