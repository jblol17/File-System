#ifndef FS3_DRIVER_INCLUDED
#define FS3_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_driver.h
//  Description    : This is the header file for the standardized IO functions
//                   for used to access the FS3 storage system.
//
//  Author         : JOHN B MORILLO
//  Last Modified  : 11-17-21
//

// Include files
#include <stdint.h>
#include <fs3_controller.h>

// Defines
#define FS3_MAX_TOTAL_FILES 1024 // Maximum number of files ever
#define FS3_MAX_PATH_LENGTH 128 // Maximum length of filename length


//struct for the file descriptor and the file status 
typedef struct file_struct
{
	/* data */
	int file_descriptor; // an integer for the file descriptor
	char *file_path;
	int status; //an integer that will be 0 (closed) or 1 (open) for each individual file descriptor 
	int length; //length of the file
	int position; // position of the file
	FS3TrackIndex track; // What track the file is in
	FS3SectorIndex sector; // What sector the file is in
}*file_s;

//
// Interface functions

int32_t fs3_mount_disk(void);
	// FS3 interface, mount/initialize filesystem

int32_t fs3_unmount_disk(void);
	// FS3 interface, unmount the disk, close all files

int16_t fs3_open(char *path);
	// This function opens a file and returns a file handle

int16_t fs3_close(int16_t fd);
	// This function closes a file

int32_t fs3_read(int16_t fd, void *buf, int32_t count);
	// Reads "count" bytes from the file handle "fh" into the buffer  "buf"

int32_t fs3_write(int16_t fd, void *buf, int32_t count);
	// Writes "count" bytes to the file handle "fh" from the buffer  "buf"

int32_t fs3_seek(int16_t fd, uint32_t loc);
	// Seek to specific point in the file

file_s create_file (int file_descriptor, char *file_path, int status, int length, int position, FS3TrackIndex track, FS3SectorIndex sector);
	// Create the file Pepega

file_s find_file (int16_t fd, char* path, file_s* file_array);
	// Find the requested file from the array Pepega

FS3CmdBlk construct_fs3_cmdblock(uint8_t op, uint16_t sec, 
uint32_t trk, uint8_t ret);
// create an FS3 array opcode from the variable fields

void deconstruct_fs3_cmdblock(FS3CmdBlk cmdblock, uint8_t *op, 
uint16_t *sec, uint32_t *trk, uint8_t *ret);
// extract register state from bus values
#endif
