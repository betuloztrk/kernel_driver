#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/init.h>


#define WR_VALUE _IOW('a','a',int*)
#define RD_VALUE _IOR('a','b',int*)
#define TIMEOUT 5000

dev_t dev = 0;
int value = 0;

static struct class *dev_class;
static struct cdev device_cdev;
static struct timer_list device_timer;
static unsigned int count = 0;


//static bool can_write = true;
//static bool can_read  = false;

static int      __init ozturk_init(void);
static void     __exit ozturk_exit(void);

static int device_open(struct inode *i, struct file *f);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *f, char *buf, size_t len, loff_t *off);
static ssize_t device_write(struct file *f, const char *buf, size_t len, loff_t *off);
static long  device_ioctl(struct file *f, unsigned int cmd, unsigned long arg);
static unsigned int device_poll(struct file *f, poll_table *wait);


typedef enum{
	DEVICE_NONE,
	DEVICE_READ,
	DEVICE_WRITE
}device_status_t;

device_status_t device_status = DEVICE_NONE;

spinlock_t device_spinlock;
//wait_queue_head_t wait_queue_device_data;
DECLARE_WAIT_QUEUE_HEAD(wait_queue_device_data);

device_status_t get_device_status (void)
{
	device_status_t valu = DEVICE_NONE;
	spin_lock(&device_spinlock);
	valu = device_status;
	spin_unlock(&device_spinlock);
	return valu;

}

 void set_device_status (device_status_t valu)
{
	spin_lock(&device_spinlock);
	device_status = valu;
	spin_unlock(&device_spinlock);
}

static struct file_operations fops =
{
  .open = device_open,
  .release = device_release,
  .read = device_read,
  .write = device_write,
  .unlocked_ioctl = device_ioctl,
  .poll = device_poll,
  .owner = THIS_MODULE

};

void timer_callback(struct timer_list *data)
{
	device_status_t status = get_device_status();

	pr_info("Timer Callback function Called %d %d \n",count++, device_status);

    	if((status == DEVICE_READ) || (status == DEVICE_NONE))
    	{
    		set_device_status(DEVICE_WRITE);
    	}
    	else if(status == DEVICE_WRITE)
    	{
    		set_device_status(DEVICE_READ);
    	}

		wake_up(&wait_queue_device_data);
		mod_timer(&device_timer, jiffies + msecs_to_jiffies(TIMEOUT));

}

static int __init ozturk_init(void)
{

        if((alloc_chrdev_region(&dev, 0, 1, "ozturk")) <0)
        {
                pr_err("Cannot allocate major number for device\n");
                return -1;
        }
        pr_info("Major = %d Minor = %d\n",MAJOR(dev), MINOR(dev));

        cdev_init(&device_cdev,&fops);
        device_cdev.owner = THIS_MODULE;
        device_cdev.ops = &fops;

        if((cdev_add(&device_cdev,dev,1)) < 0)
        {
                 pr_err("Cannot add the device to the system\n");
                 goto r_class;
        }

        if((dev_class = class_create(THIS_MODULE,"ozturk")) == NULL)
        {
            pr_err("Cannot create the struct class for device\n");
            goto r_class;
        }

        if((device_create(dev_class,NULL,dev,NULL,"ozturk")) == NULL)
        {
            pr_err("Cannot create the Device\n");
            goto r_device;
        }

        //init_waitqueue_head(&wait_queue_device_data);


        return 0;

r_device:
     class_destroy(dev_class);
r_class:
     unregister_chrdev_region(dev,1);
     return -1;
}


static int device_open(struct inode *i, struct file *f)
{

	pr_info("Device opened \n ");
	device_status = DEVICE_NONE;
	spin_lock_init(&device_spinlock);
	timer_setup(&device_timer, timer_callback, 0);
	mod_timer(&device_timer, jiffies + msecs_to_jiffies(TIMEOUT));

	return 0;
}

static ssize_t device_read(struct file *f, char *buf, size_t len, loff_t *off)
{
	pr_info("Read function\n ");
	if(copy_to_user(buf,&value,len))
			{
				//pr_info("data read : err!\n");
			}
			pr_info("read ready\n");

	return 0;
}
static ssize_t device_write(struct file *f, const char *buf, size_t len, loff_t *off)
{
	pr_info("Write function\n");
	if (copy_from_user(&value,buf,len))
			{
				//pr_info("data write : err!\n");
			}
			pr_info("Value = %d\n",value);
    return len;
}

static unsigned int device_poll(struct file *f,poll_table *wait)
{
  //__poll_t mask = 0;
  unsigned int mask = 0;
  device_status_t status = DEVICE_NONE;

  pr_info("Poll function\n");

  poll_wait(f,&wait_queue_device_data, wait);
  status = get_device_status();
  {
	  if(status == DEVICE_READ)
	  {
		  mask |= ( POLLIN | POLLRDNORM );
	  }
	  else if(status == DEVICE_WRITE)
	  {
		  mask |= ( POLLOUT | POLLWRNORM );
	  }
  }
  return mask;

}

static long device_ioctl(struct file *f,unsigned int cmd,unsigned long arg)
{
	switch(cmd)
	{
	case WR_VALUE :
		if (copy_from_user(&value,(int*) arg, sizeof(value)))
		{
			pr_info("data write : err!\n");
		}
		pr_info("Value = %d\n",value);
		break;
	case RD_VALUE:
		if(copy_to_user((int*)arg, &value,sizeof(value)))
		{
			pr_info("data read : err!\n");
		}
		break;
	default:
		pr_info("default\n");
		break;
	}
	return 0;
}

static  int  device_release ( struct inode *i, struct file *f)
{
	pr_info("Device closed \n");
    del_timer(&device_timer);

    return 0;
}

static void __exit ozturk_exit(void)
{
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(dev, 1);
        pr_info("Kernel Module Removed Successfully...\n");
}

module_init(ozturk_init);
module_exit(ozturk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Betul Ozturk");
MODULE_DESCRIPTION("file operations with chracter device");
MODULE_VERSION("1.2");
