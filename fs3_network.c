////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_netowork.c
//  Description    : This is the network implementation for the FS3 system.

//
//  Author         : John B Morillo
//  Last Modified  : 6-1-22
//

// Includes
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>

// Project Includes
#include <fs3_network.h>

#include <fs3_driver.h>
#include <fs3_controller.h>
#include <fs3_cache.h>


//
//  Global data
unsigned char     *fs3_network_address = FS3_DEFAULT_IP; // Address of FS3 server
unsigned short     fs3_network_port = FS3_DEFAULT_PORT;       // Port of FS3 serve

struct sockaddr_in caddr;
int socket_fd;

//char temp_buf[FS3_SECTOR_SIZE + sizeof(FS3CmdBlk)];


//
// Network functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : network_fs3_syscall
// Description  : Perform a system call over the network
//
// Inputs       : cmd - the command block to send
//                ret - the returned command block
//                buf - the buffer to place received data in
// Outputs      : 0 if successful, -1 if failure

int network_fs3_syscall(FS3CmdBlk cmd, FS3CmdBlk *ret, void *buf)
{
    // checking for the operation of the commandblock we give

    uint8_t op; 
    deconstruct_fs3_cmdblock(cmd, &op, NULL, NULL,NULL);

    if (op == FS3_OP_MOUNT)
    {
        /* code */
        // Return successfully
        socket_fd = socket(AF_INET, SOCK_STREAM, 0); // creating a socket_fd for the network system to use...
        // Think of this as a file descriptor
        if (socket_fd == -1) // error making the socket_fd
        {
            /* code */
            printf("Error making the socket.");
            return(-1);
        }
        
        // setting struct values
        caddr.sin_family = AF_INET;
        caddr.sin_port = htons(fs3_network_port);
        
        // connecting the socket_fd to the network 
        if (connect(socket_fd, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1)
        {
            /* code */
            printf("Error connecting the socket.");
            close(socket_fd);
            return(-1);
        }
    }
    

    // making the command block we give into one the network can understand
    FS3CmdBlk net_cmd = htonll64(cmd);

    // error raised if the commandblock was sent incorrectly to the network 
    if (write(socket_fd, &net_cmd, sizeof(net_cmd)) != sizeof(net_cmd))
    {
        /* code */
        printf("Error passing command block.");
        close(socket_fd);
        return(-1);
    }

    // if the buf is not null and we give a write operation, then do a write through the network
    if (buf != NULL && op == FS3_OP_WRSECT) // If the buf is not empty, then we can read or write over the network 
    {
        if (write(socket_fd, buf, FS3_SECTOR_SIZE) != FS3_SECTOR_SIZE)
        {
            /* code */
            printf("Error writing.");
            close(socket_fd);
            return(-1);
        }
    }
    // Return commanc block
    FS3CmdBlk ret_cmd; 

    // returns the network command block
    if (read(socket_fd, &ret_cmd,sizeof(FS3CmdBlk)) != sizeof(FS3CmdBlk))
    {
        /* code */
        printf("Error reading return command block.");
        close(socket_fd);
        return(-1);
    }

    if (buf != NULL  && op == FS3_OP_RDSECT)
    {
        /* code */
         if (read(socket_fd, buf, FS3_SECTOR_SIZE) != FS3_SECTOR_SIZE)
        {
            /* code */
            printf("Error reading.");
            close(socket_fd);
            return(-1);
        }
    }
    
    // if it's not null, then change it into a readable command block for the host
    if (ret != NULL)
    {
        /* code */
        *ret =  ntohll64(ret_cmd);
    }

    if (op == FS3_OP_UMOUNT)
    {
        /* code */
        close(socket_fd);
    }
    
    
    //socket_fd = -1;
    return (0);
}