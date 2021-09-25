Operating systems course homework assignment:

1. Introduction
The goal of this assignment was to gain experience with kernel programming and, particularly, a better understanding on the design and implementation of inter-process communication (IPC), kernel modules, and drivers. In this assignment, I implemented a kernel module that provides a new IPC mechanism, called a message slot. A message slot is a character device file through which processes communicate. A message slot device has multiple message channels active concurrently, which can be used by multiple processes. After opening a message slot device file, a process uses ioctl() to specify the id of the message channel it wants to use. It subsequently uses read()/write() to receive/send messages on the channel. In contrast to pipes, a message channel preserves a message until it is overwritten, so the same message can be read multiple times.

2. Message slot kernel module - message_slot.c and mesage_slot.h
The module uses the hard-coded major number 240. The module implements the file operations needed to provide the message slot interface: device_open, device_ioctl, device_read, and device_write.

3. Message sender - message_sender.c
Command line arguments:
•  argv[1]: message slot file path.
•  argv[2]: the target message channel id. Assume a non-negative integer.
•  argv[3]: the message to pass.
Opens the specified message slot device file. Sets the channel id to the id specified on the command line. Writes the specified message to the message slot file. Closes the device and exits the program with exit value 0.

4. Message reader – message_reader.c
Command line arguments:
•  argv[1]: message slot file path.
•  argv[2]: the target message channel id. Assume a non-negative integer.
Opens the specified message slot device file, sets the channel id to the id specified on the command line, reads a message from the message slot file to a buffer, closes the device, prints the message to standard output and exit the program with exit value 0.
