

#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<asm/uaccess.h>
#define BUFFER_SIZE 1024

static char device_buffer[BUFFER_SIZE];

ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_to_user function. source is device_buffer (the buffer defined at the start of the code) and destination is the userspace 		buffer *buffer */
	return 0;
}



ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_from_user function. destination is device_buffer (the buffer defined at the start of the code) and source is the userspace 		buffer *buffer */
	return length;
}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	return 0;
}


int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	return 0;
}

struct file_operations simple_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	/* add the function pointers to point to the corresponding file operations. look at the file fs.h in the linux souce code*/
};

static int simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	printk(KERN_ALERT "insdie %s function\n", __FUNCTION__);	
	/* register the device */
	register_chrdev(261, simple_character_device, *fops);
	return 0;
}

static int simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	pritnk(KERN_ALERT "inside %s function\n", __FUNCTION);
	/* unregister  the device using the register_chrdev() function. */
	unregister_chrdev(261, simple_character_device);
	return 0;
}

/* add module_init and module_exit to point to the corresponding init and exit function*/
module_init(simple_char_driver_init); // Executes when module starts
module_exit(simple_char_driver_exit); // Executes when module stops
