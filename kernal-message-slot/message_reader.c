#include <fcntl.h>  
#include <unistd.h> 
#include <sys/ioctl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "message_slot.h"

int main(int argc, char** argv)
{
	int file_desc, length;
	char message[BUF_LEN];

	if(argc != 3)
	{
		perror("Invalid number of argumenst");
		exit(1);
	}
	
	file_desc = open(argv[1], O_RDWR);
	
	if(file_desc < 0) 
	{
		perror("Can't open device file");
		exit(1);
	}

	if(ioctl(file_desc, MSG_SLOT_CHANNEL, atoi(argv[2])) != SUCCESS)
	{
		perror("Setting channel id failed");
		exit(1);
	}
	
	length = read(file_desc, message, BUF_LEN);
	if(length <= 0)
	{
		perror("Can't read from device file");
		exit(1);
	}
	
	close(file_desc);
	
	if(write(1, message, length) != length)
	{
		perror("Writing to device failed.");
		exit(1);
	}	
	
	exit(SUCCESS);
}
