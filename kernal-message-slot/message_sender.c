#include <fcntl.h>  
#include <unistd.h> 
#include <sys/ioctl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "message_slot.h"

int main(int argc, char** argv)
{
	int file_desc;

	if(argc != 4)
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

	if(ioctl( file_desc, MSG_SLOT_CHANNEL, atoi(argv[2])) != SUCCESS)
	{
		perror("Setting channel id failed");
		exit(1);
	}
	
	if(write(file_desc, argv[3], strlen(argv[3])) != strlen(argv[3]))
	{
		perror("Writing to device failed.");
		exit(1);
	}

	close(file_desc);
	
	exit(SUCCESS);
}
