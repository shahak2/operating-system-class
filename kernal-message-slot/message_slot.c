#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "message_slot.h"

MODULE_LICENSE("GPL");

//================== HELPER FUNCTIONS ==========================

priv_data* init_priv_data(int minor, long channel_id) 
//A constructor function for priv_data. 
//Returns a private_data pointer, or NULL upon failure.
{
	priv_data *p = kmalloc(sizeof(priv_data), GFP_KERNEL);
	
	if(p == NULL)
		return NULL;
	
	p->minor = minor;
	p-> channel_id = channel_id;
	
	return p;
}

channel* init_channel(long id) 
//A constructor function for channel. 
//Returns a new channel pointer, or NULL upon failure.
{
	channel *c;

	c = (channel*) kmalloc(sizeof(channel), GFP_KERNEL);
	
	if(c == NULL)
		return NULL;
	
	c->id = id;
	c->message = NULL;
	c->length = 0;
	c->next = NULL;
	
	return c;
}

channel* get_channel(message_slot m, long channel_id)
// Input: Receives a message slot and a channel id.
// Functionality: Searches for the id and returns a pointer to the designated channel.
//Returns NULL upon failure.
{
	channel *c;
	
	if(m.head == NULL)
		return NULL;
	
	c = m.head;
	
	while(c != NULL)
	{
		if(c->id == channel_id)
			return c;
		c = c->next;
	}	
	return NULL;
}

int append_channel(message_slot *m, long channel_id)
// Input:a message slot and a new channel id.
// Functionality: Adds the channel to the end of the list. 
//Returns FAILED, upon failure.
{
	channel *new_channel, *curr, *prev;	
	
	new_channel = init_channel(channel_id);
	if(new_channel == NULL)
		return FAILED;
	
	curr = m->head;
	
	if(curr == NULL) // The channel list is empty.
	{
		m->head = new_channel;
	}
	else // Append to the end of the list.
	{
		while(curr != NULL)
		{
			prev = curr;
			curr = curr->next;
		}
		prev->next = new_channel;
	}		
	
	return SUCCESS;
}

//================== DEVICE FUNCTIONS ===========================

// The actual data structure!
static message_slot message_slots[MAX_MINOR_VALUE];  	

static int device_open(struct inode* inode, struct file*  file)
{
	int minor;
	priv_data *p;
	
	minor = iminor(inode);
	p = init_priv_data(minor, ID_NOT_SET);
	
	if(p == NULL)
		{
			printk("Memory allocation failed\n");
			return -EINVAL;
		}
		
	file->private_data = (void*) p;
	
	return SUCCESS;
}

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset )
{
	int i, message_length, minor;
	long channel_id;
	priv_data *p;
	channel *c;
	char* read_message;
	
	p = (priv_data*) file->private_data;
	if(p == NULL)
		return -EINVAL;
	
	minor = p->minor;
	channel_id = p->channel_id;
	c = get_channel(message_slots[minor], channel_id);
	if(c == NULL)
		return -EINVAL;
	
	read_message = c->message;
	message_length = c->length;
	
	if(channel_id == ID_NOT_SET || buffer == NULL)
		return -EINVAL;
	if(length < message_length )
		return -ENOSPC;
	if(read_message == NULL)
		return -EWOULDBLOCK;
	
	for(i = 0; i < message_length ; i++)
	{
		if(put_user(read_message[i], &buffer[i]) != SUCCESS)
			return -EINVAL;
	}
	
	return i;
}

static ssize_t device_write( struct file* file, const char __user* buffer, size_t length,  loff_t* offset)
{
	int i;
	char *new_message;
	priv_data *p;
	channel *c;
	
	p = (priv_data*) file->private_data;
	
	if(p == NULL)
		return -EINVAL;
	
	c = get_channel(message_slots[p->minor], p->channel_id);
	
	if(p->channel_id == ID_NOT_SET || c == NULL || buffer == NULL)
		return -EINVAL;
	if(length > BUF_LEN || length <= 0)
		return -EMSGSIZE;

	new_message = (char*)kmalloc(sizeof(char) * length, GFP_KERNEL);
	if(new_message == NULL)
	{
		printk("Memory allocation failed\n");
		return -EINVAL;
	}
	
	for( i = 0; i < length; i++ )
	{
		if(get_user(new_message[i], &buffer[i]) != SUCCESS)
			return -EINVAL;
	}
	
	c->message = new_message;
	c->length = length; 
	
	return i;
}

static long device_ioctl(struct file* file, unsigned int   ioctl_command_id, unsigned long  channel_id )
{
	channel *c;
	priv_data* p;
	
	p = (priv_data*) file->private_data;
	
	printk("IOCTL-ING-----------\n");
	
	if(p == NULL || ioctl_command_id != MSG_SLOT_CHANNEL || channel_id == ID_NOT_SET)
		return -EINVAL;
	
	c = get_channel(message_slots[p->minor], channel_id);
	
	// If the channel_id has no existing channel, 
	// we append a channel to the message slot.
	// Otherwise, it is sufficient to change the file channel_id to the new id.
	if(c == NULL) 
		if(append_channel(&message_slots[p->minor], channel_id) == FAILED)
		{
			printk("Memory allocation failed\n");
			return -EINVAL;
		}
	
	p->channel_id = channel_id; //updating the channel id.
	
	return SUCCESS;
}

//==================== DEVICE SETUP =============================

struct file_operations Fops =
{
	.owner	  = THIS_MODULE, 
	.read           = device_read,
	.write          = device_write,
	.open           = device_open,
	.unlocked_ioctl = device_ioctl,
};

// Initialize the module
static int __init message_slot_init(void)
{
	int i, rc;
	
	rc = -1;
	
	rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

	if( rc < 0 ) // failed loading model.
	{
		printk( KERN_ERR "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM );
		return rc;
	}
	
	// Initiating all message slots to not being used.
	for(i = 0; i < MAX_MINOR_VALUE; i++) 
	{
		message_slots[i].head = NULL;
	}
	
	return SUCCESS;
}

// Unregistering the model and freeing alocated memory.
static void __exit message_slot_exit(void)
{
	int i;
	channel* curr, *next;

	// For each minor(slot) we free each of its channels.
	for(i = 0; i < MAX_MINOR_VALUE; i++) 
	{
		curr = message_slots[i].head;
		while(curr != NULL)
		{
			next = curr;
			if(curr->message != NULL)
				kfree(curr->message);
			curr = curr->next;			
			kfree(curr);
		}
	}	
	unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(message_slot_init);
module_exit(message_slot_exit);


