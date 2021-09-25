#ifndef MESSAGE_H
#define MESSAGE_H

#include <linux/ioctl.h>

#define MAJOR_NUM 240
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "message_slot"
#define BUF_LEN 128
#define SUCCESS 0
#define ID_NOT_SET 0
#define MAX_MINOR_VALUE 257
#define FAILED -1

//================== PRIVATE DATA ==========================
/* 
	Each file(device) will point to a private_data variable containing
	its minor value and its channel_id. This way it will be able to access 
	the channel's message.
*/
typedef struct priv_data
{
	int minor;
	long channel_id;
} priv_data;

//================== MESSAGE SLOT ==========================
/*
	The driver would be represented as an array of 257 possibe message_slots.
	Channel 0 would always be empty.
	Each of those slots may contain one or more channels.
	Thus, each message_slot contains a list of its channels.
 */
 
typedef struct channel
{
	long id;
	char* message;
	int length;
	struct channel *next;
} channel;

typedef struct message_slot
{
	channel *head;
} message_slot;

#endif
