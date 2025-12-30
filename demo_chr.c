#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h> 
#include <linux/uaccess.h>   // copy_from_user

#define DEVICE_NAME "hello_char"
#define CLASS_NAME "hello_class"

#define BUF_SIZE 128
static char kernel_buf[BUF_SIZE];

static int major;
static struct class *hello_class = NULL;
static struct device *hello_device = NULL;
static dev_t dev_num;
static struct cdev hello_cdev;  

static int hello_open(struct inode *inode, struct file *file)
{
    pr_info("hello_char: device opened\n");
    return 0;
}

static ssize_t hello_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    size_t len = count;

    if (len > BUF_SIZE - 1)
    len = BUF_SIZE - 1;

    if (copy_from_user(kernel_buf, buf, len)) {
        pr_err("hello_char: copy_from_user failed\n");
        return -EFAULT;
    }

    kernel_buf[len] = '\0'; 
    pr_info("hello_char: write received: %s\n", kernel_buf);

    return count; 
}

static int hello_release(struct inode *inode, struct file *file)
{
    pr_info("hello_char: device closed\n");
    return 0;
}


static ssize_t hello_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    pr_info("hello_char: read called\n");
    return 0;
}

static struct file_operations fops ={
    .owner   = THIS_MODULE,
    .open    = hello_open,
    .read    = hello_read,
    .write   = hello_write,
    .release = hello_release,
};

static int __init hello_init(void)
{
    int retval;
    pr_info("Hello from kernel module! \n");

    //1- Dynamically allocate major device number
    retval = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (retval < 0) {
        pr_err("Failed to allocate char device region\n");
        return retval;
    }

    major = MAJOR(dev_num);
    pr_info("Hello char major number: %d\n", major);

    //2- Initialize and add cdev structure
    cdev_init(&hello_cdev, &fops);    
    hello_cdev.owner = THIS_MODULE;
    
    retval = cdev_add(&hello_cdev, dev_num, 1);
    if (retval < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return retval;
    }

    //3- Create class
    hello_class = class_create(THIS_MODULE ,CLASS_NAME);
    if (IS_ERR(hello_class)) {
        unregister_chrdev_region(dev_num, 1);
        pr_err("Failed to create device class\n");
        return PTR_ERR(hello_class);
    }

    //4- Create device node
    hello_device = device_create(hello_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(hello_device)) {
        class_destroy(hello_class);
        unregister_chrdev_region(dev_num, 1);
        pr_err("Failed to create device\n");
        return PTR_ERR(hello_device);
    }

    pr_info("Hello char device created successfully\n");
    pr_info("Device node will be: /dev/%s\n", DEVICE_NAME);

	return 0;
}

static void __exit hello_exit(void)
{
    // Clean the device -> class -> device_num
    if (hello_device) {
        device_destroy(hello_class, dev_num);
        pr_info("device_destroy!\n");
    }
    
    if (hello_class) {
        class_destroy(hello_class);
        pr_info("class_destroy!\n");
    }
    unregister_chrdev_region(dev_num, 1);
    pr_info("unregister_chrdev_region!\n");

    pr_info("Bye from kernel module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("puck");
MODULE_DESCRIPTION("Stage5: Improve file_operations");
