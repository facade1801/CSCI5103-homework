/*
 * main.c -- the bare scull char module
 *
 * This code is based on the scullpipe code from LDD book.
 *         Anand Tripathi November 2020
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/sched.h>

#include <linux/configfs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <linux/uaccess.h>	/* copy_*_user */
#include <linux/sched/signal.h>
#include "scullbuffer.h"	/* local definitions */


struct scull_buffer {
		wait_queue_head_t inq, outq;       /* read and write queues */
		char *buffer, *end;                /* begin of buf, end of buf */
		int buffersize;                    /* used in pointer arithmetic */
		char *rp, *wp;                     /* where to read, where to write */
		int  itemcount;			   		   /* Number of items in the buffer */
		int nreaders, nwriters;            /* number of openings for r/w */
		struct semaphore sem;              /* mutual exclusion semaphore */
		struct cdev cdev;                  /* Char device structure */
};

/* parameters */
static int scull_b_nr_devs = SCULL_B_NR_DEVS;	/* number of buffer devices */
dev_t scull_b_devno;							/* Our first device number */

// THE device ptr
static struct scull_buffer *scull_b_devices;

// Our parameters which can be set at load time.
#define init_MUTEX(_m) sema_init(_m, 1);

int scull_major =   SCULL_MAJOR;
int scull_minor =   0;
int NITEMS		=	20;
int itemsize	=	SCULL_B_ITEM_SIZE;

module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_b_nr_devs, int, 0);
module_param(NITEMS, int, 0);

MODULE_AUTHOR("Student CSCI 5103-Fall 2020 - adding code to the framework by Anand Tripathi");
MODULE_LICENSE("Dual BSD/GPL");

/*
 * Open and close
 */
static int scull_b_open(struct inode *inode, struct file *filp)
{
	struct scull_buffer *dev;
		
	// IMPLEMENT THIS FUNCTION 

	return nonseekable_open(inode, filp);
}

static int scull_b_release(struct inode *inode, struct file *filp)
{
	struct scull_buffer *dev ;

	// IMPLEMENT THIS FUNCTION 
	//

}

/*
 * Data management: read and write
*/
static ssize_t scull_b_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{

	struct scull_buffer *dev ;

	// IMPLEMENT THIS FUNCTION
	//
} 
static ssize_t scull_b_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_buffer *dev; 

	// IMPLEMENT THIS FUNCTION
	//
}

/*
 * The file operations for the buffer device
 * (some are overlayed with bare scull)
 */
struct file_operations scull_buffer_fops = {
	.owner =	THIS_MODULE,
	.llseek =	no_llseek,
	.read =		scull_b_read,
	.write =	scull_b_write,
	.open =		scull_b_open,
	.release =	scull_b_release,
};

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void scull_b_cleanup_module(void)
{
	// cleanup!
	dev_t dev = MKDEV(scull_major, scull_minor);
	if (scull_b_devices) {
		cdev_del(&scull_b_devices[0].cdev);
		kfree(scull_b_devices);
	}
	unregister_chrdev_region(dev, 1); // just one device
}

/*
 * Set up a cdev entry.
 */
static void scull_b_setup_cdev(struct scull_buffer *dev, int index)
{
	// IMPLEMENT THIS FUNCTION
	//
}


int scull_b_init_module(void)
{
	// Module initialization at the time of module loading
	// (Memory allocation & Initializations)

	int result;
	dev_t dev = 0;

	// 1. Alloc memory according to major/minor number: Almost copied from SCULLPIPE
	if (scull_major) {
		dev = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(dev, scull_minor, "scullbuffer");
	} else {
		result = alloc_chrdev_region(&dev, scull_minor, 1, "scullbuffer");
		scull_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "SCULLPIPE: cannot get major (major = %d)\n", scull_major);
		return result;
	}

	// 2. Alloc THE device
	scull_b_devices = kmalloc(1 * sizeof(struct scull_buffer), GFP_KERNEL);
	if (!scull_b_devices) {
		result = -ENOMEM;
		goto fail;
	}
	memset(scull_b_devices, 0, 1 * sizeof(struct scull_buffer));

	// 3. Initialize THE device. The number is 0, because it's the only device
	scull_b_devices[0].buffersize = NITEMS;
	init_MUTEX(&scull_b_devices[0].sem);
	scull_b_setup_cdev(&scull_b_devices[0], 0);

	// should be everything for now!
	return 0;

	// TODO - Make it more graceful :)
	fail:
	scull_b_cleanup_module();
	return result;
}

module_init(scull_b_init_module);
module_exit(scull_b_cleanup_module);
