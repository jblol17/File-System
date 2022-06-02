////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the storage system.
//
//   Author        : *** JOHN B MORILLO ***
//   Last Modified : *** 11-19-21 ***
//


// Includes
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Project Includes
#include <fs3_driver.h>
#include <fs3_controller.h>
#include <fs3_cache.h>
#include <fs3_network.h>

//
// Defines
#define SECTOR_INDEX_NUMBER(x) ((int)(x/FS3_SECTOR_SIZE))

int status = 0; //This is the checker to see if the system is mounted. 
//It is unmounted by default (0) and mounted (1) when the mount call is called 
// It is set to (0) when unmount is called. 

char temp_buf[FS3_SECTOR_SIZE * 350]; // a character array for a temp buf that's the size of a full sector! Should it be the size of the max we'll have per file which is 10k? It's set to 1k...? No


static FS3TrackIndex trk_index = 0; // The index of the track, as in, what track we are in at the moment
// The index starts at 0 and gets updated depending on if we go over the amount of sectors in each track. 
static FS3SectorIndex sec_index = 0; // The index of the sector, as in, what sector we in at the moment. 
// The index starts at sector 0 (We start here) and moves as the sectors get full.

int open_count = 0; // This keeps track of how many times open was called. It will be (0) by default and (1) when open is called.
// Every time open is called, this will increase by 1 and the file handle that open creates will also increase by 1 so that there are no equal file handles. 

int array_size = 18;
file_s file_array[18] = {NULL};
int sectors_being_used[FS3_SECTOR_SIZE] = {0}; // need to keep track of the sector(s) being used by each file handle or file :(



//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function 	: construct_fs3_cmdblock
// Description 	: Constructs the command block to send instructions throught fs3_syscall
// Inputs 		: 8b integer containing instruction, 16b integer containing sector, 32b integer containing track, 8b integer return
// Outputs 		: The constructed Command Block

FS3CmdBlk construct_fs3_cmdblock (uint8_t op, uint16_t sec, uint32_t trk, uint8_t ret){ 
	FS3CmdBlk cmdblock = 0; //64 bits of 0
	// trk = 0; // setting this and ret to 0 because I was getting weird numbers when constructing the command block but I think it was something else instead of the construction
	// ret = 0; // This might have to go on the next assignment, as there will probably be more than 1 track used
	cmdblock = cmdblock | op; //The following lines are oring the needed bits into the command block and shifting the command block itself the needed bits for the rest of the sector and track & such
	cmdblock = (cmdblock << 16) | sec; // shifting the cmndblock over the amount of bits for sector to fit at the end, and oring that
	cmdblock = (cmdblock << 32) | trk; // shifting the cmdblock over the amount of bits for track to fit at the end, and oring that
	cmdblock = (cmdblock << 1) | ret; // shifting cmdlock over 1 bit for return, and oring it in 
	cmdblock =  (cmdblock << 11); // shifting the rest of the cmdblock over to where it should end up with the unused space included.

	return(cmdblock); //returns the constructed command block
}

////////////////////////////////////////////////////////////////////////////////
//
// Function 	: deconstruct_fs3_cmdblock
// Description 	: Deconstructs the command block constructed above
// Inputs 		: Newly constructed Command Block
// Outputs 		: An integer that says if the command is valid to send throught fs3_syscall. 
// 				  0 if passed, -1 if failure 

// deconstructs the command block to change the necessary parts of it through the network. 
void deconstruct_fs3_cmdblock(FS3CmdBlk cmdblock, uint8_t *op, uint16_t *sec, uint32_t *trk, uint8_t *ret){

	cmdblock =  (cmdblock >> 11); // shifting the rest of the cmdblock over to where it should end up with the unused space included.

	if (ret != NULL)
	{
		/* code */
		*ret = (uint8_t) (cmdblock & 1); // shifting cmdlock over 1 bit for return, and oring it in 
	}
	if (trk != NULL)
	{
		/* code */
		*trk = (uint32_t) (cmdblock >> 1); // shifting the cmdblock over the amount of bits for track to fit at the end, and oring that
	}
	if (sec != NULL)
	{
		/* code */
		*sec = (uint16_t) (cmdblock >> 33); // shifting the cmndblock over the amount of bits for sector to fit at the end, and oring that
	}
	if (op != NULL)
	{
		/* code */
		*op = (uint8_t) (cmdblock >> 49); //The following lines are oring the needed bits into the command block and shifting the command block itself the needed bits for the rest of the sector and track & such
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_mount_disk
// Description  : FS3 interface, mount/initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_mount_disk(void) {
	if (status != 1) // if the status is not mounted (1), then...
	{ 
		//call controller MOUNT & initialize variables that might be needed
		FS3CmdBlk cmdblock = construct_fs3_cmdblock(FS3_OP_MOUNT,0,0,0); // construct the command block to call the MOUNT function
		network_fs3_syscall(cmdblock, NULL, NULL); // call the command block with the mount function and a void buffer
		//deconstruct_fs3_cmdblock(cmdblock, FS3_OP_MOUNT, 0,0,0);
		// for (int i = 3; i < array_size; i++)
		// {
		// 	//file_s file = NULL;
		// 	file_array[i] = NULL;
		// }
		
		status = 1; //set status of system to mounted (1)
		return(0); //Return sucess
	}
	return(-1);// else return failure cuz then the system is mounted and there's no need to mount it again
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_unmount_disk
// Description  : FS3 interface, unmount the disk, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_unmount_disk(void) {
	if (status != 0) // if the status is not unmounted, then do the following...
	{ 
		// call controller UNMOUNT & discard/uninitialized what's being done
		FS3CmdBlk cmdblock = construct_fs3_cmdblock(FS3_OP_UMOUNT,0,0,0); // construct the command block to call the UNMOUNT function
		network_fs3_syscall(cmdblock, NULL, NULL); // system call with the unmount command block and null buffer (I think there should be something there tho)
		for (int i = 0; i < array_size; i++) // free the allocated memory for each file_s struct in the array
		{
			free(file_array[i]);
		}
		status = 0; //Set the status of the system to unmounted
		return(0); // return sucess
	}
	return(-1); // else return failure cuz that means the system is unmounted
}

////////////////////////////////////////////////////////////////////////////////
//
// Function		: create_file
// Description	: allocates space for a file of type file_s
//
// Inputs		: the number of the file descriptor, file status, length, position
// Outputs		: the file that was just made of type file_s

file_s create_file (int file_descriptor, char *file_path, int status, int length, int position, FS3TrackIndex track, FS3SectorIndex sector){
	file_s file = (file_s) malloc(sizeof(struct file_struct)); // allocates memory that is the size of the struct
	if (file != NULL) // if the file was allocated properly, AKA is not null, AKA malloc worked correctly
	{
		file->file_descriptor = file_descriptor; // points the file descriptor pointer to its value
		file->file_path = file_path;
		file->status = status; // points the status pointer to its value
		file->length = length; // points the length pointer to length value
		file->position = position; // points the position pointer to position value
		file->track = track; // points to the track where the file is 
		file->sector = sector; // points to the sector where the file is
	}	
	return(file); // return the completed file 
}

////////////////////////////////////////////////////////////////////////////////
//
// Function		: find_file
// Description	: loops through the array of made files
//
// Inputs		: fd and the whole array
// Outputs		: the file requested

file_s find_file (int16_t fd, char* path, file_s* file_array){
	for (int i = 3; i < open_count + 3; i++) // to loop through the array itself
	{
		if (file_array[i]->file_descriptor == fd ) // if the element in the array's file descriptor is equal to the requested fd, then return that particular file
		{
			return(file_array[i]); // I need to return a file_s type and the array is of file_s type, maybe I need to return the file descriptor but that doesn't make sense
		}
		if (path != NULL && strcmp(file_array[i]->file_path, path) == 0)
		{
			return(file_array[i]);
		}
	}
	return(NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t fs3_open(char *path) { 
	//Check if file is already open

	file_s file = find_file(0, path, file_array);
	
	if (file == NULL)
	{
		file = create_file(3 + open_count, path, 0, 0, 0, 0 , 0); // Create a file_s type called file with default parameters fd 3, status 0 (closed), length 0, position 0, track index 0 + which track we at, sector index 0 plus where we are in the track
		file_array[3 + open_count] = file; // put this file in the array, increasing by 1 every time open is called & this way the file descriptor is different and the file is in a different part of the array so it shouldn't overwrite
		open_count += 1; // Increase opencount by 1 & it should not be greater than 10 as we have a max of 10 files 
	}
	if (file->status == 1)
	{
		return(-1);
	}
	file->status =1;
	return(file->file_descriptor); // return the file descriptor
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t fs3_close(int16_t fd) {
	file_s file_in_use = find_file(fd, NULL, file_array); // this will find the file with the fd required within the array of files

	if (file_in_use != NULL && file_in_use->status != 0) // If the requested fd is the file descriptor and the requested file status is open...
	{
		file_in_use->status = 0; // set this status so closed (0)
		return(0); // return 0 
	}
	else //otherwise, fail and return (-1)
	{
		return(-1);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_read
// Description  : Reads "count" bytes from the file handle "fh" into the 
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t fs3_read(int16_t fd, void *buf, int32_t count) {

	file_s file_in_use = find_file(fd, NULL, file_array);
	//if the fd's track and sec are in the cache, return that instead of below

	if (file_in_use != NULL && file_in_use->status == 1 && file_in_use->file_descriptor == fd)
	{
		// keep track of variables we will need to write the way I want to, over sectors in different tracks
		int starting_sector = 0 + (file_in_use->position / FS3_SECTOR_SIZE); //math for the first sector of the file
		int bytes_read = file_in_use->position + count > file_in_use->length ? file_in_use->length - file_in_use->position : count; 
		int sec_to_read = (file_in_use->position % FS3_SECTOR_SIZE + bytes_read + (FS3_SECTOR_SIZE -1))/FS3_SECTOR_SIZE; // math for amount of sectors to read
		
		network_fs3_syscall(construct_fs3_cmdblock(FS3_OP_TSEEK, starting_sector, file_in_use->file_descriptor, 0), NULL, NULL); //seek to the track and sector we need to write to

		// I'm doing a big read into the temp buf and spacing it over sectors, then copying that big temp buf into buf, with the cache doing its thing instead if what I need is in the cache
		for (int i = 0; i < sec_to_read; i++)
		{			
			char* cache = fs3_get_cache(file_in_use->file_descriptor, starting_sector + i);
			if ( cache != NULL)
			{
				memcpy(temp_buf+ (FS3_SECTOR_SIZE * i), cache, FS3_SECTOR_SIZE); 
			}

			else // so if the cache is empty
			{
				FS3CmdBlk cmdblock = construct_fs3_cmdblock(FS3_OP_RDSECT, starting_sector + i, file_in_use->file_descriptor, 0); //I'm thinking return should be something else...? Track COULD be hard coded to 0 I think.
				//We read in the sector we're currently at & track we're at
				network_fs3_syscall(cmdblock, NULL, temp_buf +(FS3_SECTOR_SIZE * i)); // systcall with the constructed command block and the temporary buffer & an offset of size sector
				fs3_put_cache(file_in_use->file_descriptor, starting_sector + i, temp_buf +(FS3_SECTOR_SIZE * i));	
			}			
		}
		memcpy(buf,(temp_buf + (file_in_use->position % FS3_SECTOR_SIZE)),bytes_read); // copy the tempbuf + our position, however big count is, into buf
		return (bytes_read); // returns the amount of bytes read
	}
	else
	{
		return(-1); // fail
	}	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_write
// Description  : Writes "count" bytes to the file handle "fh" from the 
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t fs3_write(int16_t fd, void *buf, int32_t count) { 

	// add the write amount (count) to position and length. 
	file_s file_in_use = find_file(fd, NULL, file_array);

		///////////////////////////////////// Gonna write the files accross different tracks instead of accross different sectors
		// write into the cache whatever we do whether it's already been there
		// Same as the read, having general values needed stored
		if (file_in_use != NULL && file_in_use->status == 1 && file_in_use->file_descriptor == fd)
		{
			int startin_sec = 0 + (file_in_use->position / FS3_SECTOR_SIZE); // math for the first sector to write
			int sectors_to_write = (file_in_use->position % FS3_SECTOR_SIZE + count + (FS3_SECTOR_SIZE -1))/FS3_SECTOR_SIZE; // math for amount of sectors to write
			
			
			network_fs3_syscall(construct_fs3_cmdblock(FS3_OP_TSEEK, 0, file_in_use->file_descriptor,0), NULL, NULL); // track seek to the track that has the same index as the file descriptor
			//that each file will have its own track and the first 10 sectors to do whatever they want

			// a big read, with the cache stuff as previously mentioned in read
			for (int i = 0; i < sectors_to_write; i++)
			{
				char* cache = fs3_get_cache(file_in_use->file_descriptor, startin_sec + i);
				if ( cache != NULL)
				{
					memcpy(temp_buf+ (FS3_SECTOR_SIZE * i), cache, FS3_SECTOR_SIZE); 
				}
				else // so if the cache is emptu
				{
					FS3CmdBlk cmdblock1 = construct_fs3_cmdblock(FS3_OP_RDSECT, startin_sec + i, file_in_use->file_descriptor, 0); //I'm thinking return should be something else...? Track COULD be hard coded to 0 I think.
					//We read in the sector we're currently at & track we're at
					network_fs3_syscall(cmdblock1, NULL, temp_buf +(FS3_SECTOR_SIZE * i)); // systcall with the constructed command block and the temporary buffer & an offset of size sector	
				}	
			}

			memcpy(temp_buf + (file_in_use->position % FS3_SECTOR_SIZE), buf, count);

			// A big write where, since we're writing to the disk and cache anyway, we just put what we wrote in the cache
			for (int i = 0; i < sectors_to_write; i++)
			{
				FS3CmdBlk cmdblock2 = construct_fs3_cmdblock(FS3_OP_WRSECT, startin_sec + i, file_in_use->file_descriptor, 0);
				network_fs3_syscall(cmdblock2, NULL, temp_buf + (FS3_SECTOR_SIZE * i)); 
				fs3_put_cache(file_in_use->file_descriptor, startin_sec+ i, temp_buf +(FS3_SECTOR_SIZE * i));
			}

			if (count + file_in_use->position > file_in_use->length ) //if position + count is more than the length of the file, increase the length
			{
				file_in_use->length = file_in_use->position + count;
			}

			file_in_use->position += count;
			return(count);
		}
		return(-1);
	}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_seek
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_seek(int16_t fd, uint32_t loc) { // change position to what location we want

	file_s file_in_use = find_file(fd, NULL, file_array); // finds the required file within the array

	if ((file_in_use != NULL) && (file_in_use->status == 1) && (0 <= loc) && (loc <= file_in_use->length)) // if the requested file is open, location is greater than 0, location is less than the length of the file, and the file's fd matches
	{
		file_in_use->position = loc; // changes the file's position to the location that we want
		return(0); //sucess
	}
	else
	{
		return(-1); //fail
	}
} 
// POGGERS, this took quite a while and lots of rethinking my approach over 11-18-21 and 11-20-21 which was kind of a pain but it was cool.
// GDB helped out a lot. 