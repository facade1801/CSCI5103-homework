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
		int  itemcount;			   		   /* NEW: Number of items in the buffer */
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
	// initialize scull buffer device, link the device, let other methods know
	struct scull_buffer *dev;
	dev = container_of(inode->i_cdev, struct scull_buffer, cdev);
	filp->private_data = dev; /* for other methods */

	// Grab the lock, initialize the buffer space (kmalloc 32*20)
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (!dev->buffer) {
		printk("I'm initializing the buffer because there's no buffer here - this message should only be here 1 time.");
		dev->buffer = kmalloc(NITEMS * itemsize, GFP_KERNEL);
		if (!dev->buffer) {
			up(&dev->sem);
			return -ENOMEM;
		}

		// initialize other stuff in dev.
		// 1.buffersize(32*20)	2.end=start+size 	3.pointers=0
		dev->buffersize = NITEMS * itemsize;
		dev->end = dev->buffer + dev->buffersize;
		dev->rp = dev->wp = dev->buffer;		
		// 3d. setup wait queue (new)
		init_waitqueue_head(&(dev->inq));
		init_waitqueue_head(&(dev->outq));
	}

	// dev->rp = dev->wp = dev->buffer;
	printk("A new person has come. buffer=%d, end=%d, buffersize=%d", (int)(dev->buffer), (int)(dev->end), dev->buffersize);


	// Copied from pipe.c. A new reader(writer) begins holding the file.
	/* use f_mode,not  f_flags: it's cleaner (fs/open.c tells why) */
	if (filp->f_mode & FMODE_READ) {
		printk("A new reader is coming\n");
		dev->nreaders++;
	}
	if (filp->f_mode & FMODE_WRITE) {
		printk("A new writer is coming\n");
		dev->nwriters++;
	}
	up(&dev->sem);

	return nonseekable_open(inode, filp);
}

static int scull_b_release(struct inode *inode, struct file *filp)
{
	struct scull_buffer *dev = filp->private_data;

	// fully copied. Nothing needs to change. Hooray!
	// take the sem, check if anybody out there, if no one, free the buffer.
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (filp->f_mode & FMODE_READ) {
		printk("A reader is leaving\n");
		dev->nreaders--;
	}
	if (filp->f_mode & FMODE_WRITE) {
		printk("A writer is leaving\n");
		dev->nwriters--;	
	}

	if (dev->nreaders == 0) {
		wake_up_interruptible(&dev->outq);
	}
	// last writer leaving, send a notice to all readers.
	if (dev->nwriters == 0) {
		wake_up_interruptible(&dev->inq);
	}

	if (dev->nreaders + dev->nwriters == 0) {
		kfree(dev->buffer);
		dev->buffer = NULL; /* the other fields are not checked on open */
		dev->itemcount = 0;
		dev->rp = dev->wp = NULL;
	}
	// up
	up(&dev->sem);

	return 0;
}

/*
 * Data management: read and write
*/
static ssize_t scull_b_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{

	struct scull_buffer *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	// Got the sem -> check if the buffer has item (rp != lp)
	// if the buffer has 0 item
	while (dev->itemcount == 0) {
		// if there are writers, release lock and wait; if no writers, return 0

		up(&dev->sem);
		if (dev->nwriters > 0) {
			wait_event_interruptible(dev->inq, dev->itemcount != 0 || dev->nwriters == 0);
			// when the writer finish writing, this wait will be resumed...
		} else {
			printk("buffer empty & no producer\n");
			return 0;
		}

		//acquire the lock before next loop
		down_interruptible(&dev->sem);
	}

	// Now do the read. now the buffer must have >0 item, and THIS process is holding the sem
	// strcpy(buf, dev->rp);
	copy_to_user(buf, dev->rp, itemsize);
	dev->rp += itemsize;
	dev->itemcount --;
	if (dev->rp == dev->end) {
		dev->rp = dev->buffer;
	}
	// broadcast to all writers
	up(&dev->sem);
	wake_up_interruptible(&dev->outq);
	return itemsize;

} 
static ssize_t scull_b_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_buffer *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	printk("mod value = %li, dev->itemcount = %i\n", ((dev->rp - dev->wp + dev->buffersize) % dev->buffersize), dev->itemcount);
	// Got the sem -> check if the buffer has item (rp != lp)
	// if the buffer is full	
	// while ((dev->rp - dev->wp + dev->buffersize) % dev->buffersize == 32) {
	while ((dev->itemcount == NITEMS)) {
		// if there are readers, release lock and wait; if no readers, return 0
		up(&dev->sem);
		if (dev->nreaders > 0) {
			wait_event_interruptible(dev->outq, dev->itemcount != NITEMS || dev->nreaders == 0);
			// when the reader finish reading, this wait will be resumed...
		} else {
			printk("buffer full & no consumer\n");
			return 0;
		}

		//acquire the lock before next loop
		down_interruptible(&dev->sem);
	}

	// now the buffer must have space(does it?), and THIS process is holding the sem
	// do the write!
	// TODO: it's dangerous because write could cover the read. (Probably solved)
	// strcpy(dev->wp, buf);
	copy_from_user(dev->wp, buf, itemsize);
	printk("the written string is <%s>\n", dev->wp);

	dev->wp += itemsize;
	dev->itemcount ++;
	if (dev->wp == dev->end) {
		dev->wp = dev->buffer;
	}

	printk("pointer becomes <%i>\n", (int)(dev->wp));

	// broadcast to all readers
	up(&dev->sem);
	wake_up_interruptible(&dev->inq);
	return itemsize;
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
	int err;
	int devno = MKDEV(scull_major, 0);
	// set up char_dev: the variable is called (dev->cdev)
	cdev_init(&dev->cdev, &scull_buffer_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_buffer_fops;
	// the 0 means devno(basically nothing I think, doesn't matter here)
	err = cdev_add(&dev->cdev, devno, 1);

	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding scull buffer %d", err, index);
}


int scull_b_init_module(void)
{
	// Module initialization at the time of module loading
	// (Memory allocation & Initializations)

	int result;
	dev_t dev = 0;

	// 1. Alloc memory according to major/minor number: Almost copied from SCULLPIPE
	int choice;
	if (scull_major) {
		choice = 1;
		dev = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(dev, 1, "scullbuffer");
	} else {
		choice = 2;
		result = alloc_chrdev_region(&dev, scull_minor, 1, "scullbuffer");
		scull_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "SCULLPIPE: cannot get major (major = %d)\n", scull_major);
		return result;
	}
	printk("Scull buffer init: result = %d, scullMajor = %d, choice = %d\n", result, scull_major, choice);

	// 2. Alloc THE device
	scull_b_devices = kmalloc(1 * sizeof(struct scull_buffer), GFP_KERNEL);
	if (!scull_b_devices) {
		result = -ENOMEM;
		goto fail;
	}
	memset(scull_b_devices, 0, 1 * sizeof(struct scull_buffer));

	// 3. Initialize THE device. The number is 0, because it's the only device
	// 3a. itemcount
	scull_b_devices[0].itemcount = 0;
	// 3b. Mutex
	init_MUTEX(&scull_b_devices[0].sem);
	// 3c. setup cdev
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
