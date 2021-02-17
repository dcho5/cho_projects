/*
*   This file holds the functions for opening, closing, reading and writing files and directories
*
*/
#include "filesystem.h"
#include "types.h"
#include "syscalls.h"

uint32_t bb_addr;

bootblock_t * boot;

dentry_t dentry0;

// filesys_init
//input: start_addr, the address where the boot block begins
//output: none
//side effects: sets the bb_addr variable
// sets up the boot variable to hold the bootblock structure, for access into all data files
void filesys_init(uint32_t start_addr)
{
    bb_addr = start_addr;
    boot = (bootblock_t *) start_addr;
}

//file_open
//input: filename - the name of the file to open
//output: 0 if success, -1 on failure
//side effects: none
//if the file name exists in our directory return 0, else -1
int32_t file_open(const uint8_t* filename)
{
    return read_dentry_by_name((uint8_t *)filename, &dentry0);
}

//file_close
//input: fd - file descriptor, telling which file to close
//output: 0 on success, -1 on failure
//side effects: none
//nothing really needs to be done for closing a file so its always a success
int32_t file_close(int32_t fd)
{
    return 0;
}

// file_write
//inputs: fd - file descriptor giving info about file to write, buf - the buf we want to write into the file
//inputs: nbytes - the amount of bytes we want to write into the file
//output: -1 because we have not implemented write, always fails
//side effects: none
//this function doesnt do anything but return fail
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;
}

// file_read
//inputs: fname - filename to be read, offset - how far into the start of the file to start reading,
//inputs: buf - the buffer holding the data we want to return, length - the number of bytes we want to read
//outputs: -1 on failure, otherwise return the number of bytes read
//side effects: none
//passes on data into the read_data function to be returned
//int32_t file_read( int8_t * fname, uint32_t offset, uint8_t * buf, uint32_t length)
int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
{
    int32_t copied_bytes = read_data(pcb->file_array[fd].inode, pcb->file_array[fd].file_position, buf, nbytes);
    // if error return -1
    if (copied_bytes < 0) return -1;

    // increment file_position
    pcb->file_array[fd].file_position += copied_bytes;

    return copied_bytes;
}

// dir_open
//inputs: filename - file we are trying to open
//outputs: 0 on success, -1 on failure
//side effects: updates dentry0 with data
//if the filename exists in our directory return 0, else return -1
int32_t dir_open(const uint8_t* filename)
{
	return read_dentry_by_name((uint8_t *)filename, &dentry0);
}

// dir_close
//input: fd - file descriptor, telling which file to close
//output: 0 on success, -1 on failure
//side effects: none
//nothing really needs to be done for closing a file so its always a success
int32_t dir_close(int32_t fd)
{
    return 0;
}

// dir_write
//inputs: fd - file descriptor giving info about file to write, buf - the buf we want to write into the file
//inputs: nbytes - the amount of bytes we want to write into the file
//output: -1 because we have not implemented write, always fails
//side effects: none
//this function doesnt do anything but return fail
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;
}

// dir_read
//inputs: fd - file descriptor(not used rn), buf - a buffer to hold the file name, nbytes - the number of bytes to copy into buf
//outputs: returns number of bytes copied into buf, if 0 we are probably out of index and done with directories
//side effects: chnages buf, pcb->file_array.file_position is constantly getting updated
//reads nbytes of the current directory into buf, every time the function is called it automatically moves onto the next directory
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes)
{
    dentry_t dentry;
    int i;

    if (read_dentry_by_index(pcb->file_array[fd].file_position, &dentry) == -1) //if read_dentry_by_index fails
    {
        return 0; //no bytes were copied into buf
    }
    else
    {
		//clear buffer
        for (i = 0; i < 33; i++)// GIVEN BUFFER IS 33 long(according to the test, so maybe change for CP3) so we need to clear out all 33
		{
			((int8_t*)(buf))[i] = NULL;
		}

        int32_t length = strlen((int8_t*)dentry.filename);
        strncpy((int8_t*)buf, (int8_t*)dentry.filename, nbytes);
        pcb->file_array[fd].file_position++;
        return fmin(length, nbytes);
    }
}

//helper functions needed to execute above commands

//inputs: fname - the name of the file in the directory, dentry - the dentry we are copying the data into
//outputs: returns 0 on success, -1 on fail. given dentry is full of info now
//side effects: changes dentry
//copies data from the named dentry into the given dentry, if the name doesnt exist in the directory return -1
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
{
	int j = 0;
  	while (fname[j] != '\0') {
    	j++;
  	}
	if(j >= 33)
	{
		return -1;
	}
	int i;
	for(i=0; i<63; i++)//iterate through every file
	{
		uint8_t* dentryname = boot->dentry_data[i].filename;
		if(strncmp((int8_t*)fname, (int8_t*)dentryname, 32)==0)//compares fname and dentryname, if same it = 0
		{
			strncpy((int8_t*)(dentry->filename), (int8_t*)(fname), filename_size);   // copy string of file name into dentry
			dentry->type = boot->dentry_data[i].type;				// update file type of the parameter dentry
			dentry->inode = boot->dentry_data[i].inode;			// update inode index of the parameter dentry
			return 0;
		}
	}
	return -1;   // return -1 on failure
}

//inputs: index - the index of the file in the directory, dentry - the dentry we are copying the data into
//outputs: returns 0 on success, -1 on fail. given dentry is full of info now
//side effects: changes dentry
//copies data from the indexed dentry into the given dentry
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
	if(index > 62)//63 is the max number of possible files in our directory, so we index from 0-62
	{
		return -1;
	}
    uint32_t i;
    //Copy over the filename
	for (i = 0; i < filename_size; i ++)
		dentry->filename[i] = boot->dentry_data[index].filename[i];

	//Copy over the other necessary information
	dentry->type = boot->dentry_data[index].type;
	dentry->inode = boot->dentry_data[index].inode;
	return 0;
}

//inputs: inode - the inode of the file were reading, offset - offset from the base address to start reading,
//inputs: buf - the buf holding the data that we copy from the file, length - the amount of byte we want to read
//outputs: returns number of bytes copied into buf, if 0 we are probably out of index and done with directories
//side effects: changes buf
//reads length of the current directory into buf

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
	uint32_t i;
 	int32_t cur_byte = 0;
 	int32_t dblock_offset;
 	int32_t calc_offset; // offset once inside correct data block
 	inode_t *cur_inode;
	dblock_t* cur_dblock;

 	if(inode < 0 || inode >= boot->inode_num)//if inode index is invalid, return without reading any bytes
	{
 		return cur_byte;
 	}


	//get address of where we are reading into buffer from
 	calc_offset = offset % block_size;
 	cur_inode = (inode_t*)((uint8_t*) boot + ((inode+1) * block_size));
	dblock_offset = offset / block_size;
 	cur_dblock = (dblock_t*) ((uint8_t*) boot + (1 + boot->inode_num + cur_inode->data[dblock_offset])*block_size);


    for (i = 0; i < length; i++)//clear buffer that we are writing to
	{
		((int8_t*)(buf))[i] = NULL;
	}


 	for(i = 0; i < length; i++)
	 {
 		if(cur_byte + offset < cur_inode->length)//check if we are over the file length
		 {
 			buf[i] = cur_dblock->data[calc_offset];
 			cur_byte++;
 			calc_offset++;
 			if(calc_offset >= block_size)//this is how we move onto the next block if we are reading between two blocks
			 {
 				dblock_offset++;
 				cur_dblock = (dblock_t*) ((uint8_t*) boot + (1 + boot->inode_num + cur_inode->data[dblock_offset])*block_size);
 				calc_offset = 0;
 			}
 		}
 	}
 return cur_byte;
}
