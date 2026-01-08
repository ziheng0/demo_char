#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h> 
#include <linux/uaccess.h>   // copy_from_user
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/io.h>   // ioremap, iounmap

volatile unsigned long *gpiocon = NULL;
volatile unsigned long *gpiodat = NULL;

static unsigned int led_gpio[] = {
    NULL,
    0x01000000+0x1000*25, //PIN1,GPIO1,GPIO_25
    0x01000000+0x1000*10, //PIN2,GPIO2,GPIO_10
    0x01000000+0x1000*42, //PIN3,GPIO3,GPIO_42
    0x01000000+0x1000*11, //PIN4,GPIO4,GPIO_11
    0x01000000+0x1000*24, //PIN5,GPIO5,GPIO_24
};

#define DEVICE_NAME "led"
#define CLASS_NAME "hello_class"

#define BUF_SIZE 128
static char kernel_buf[BUF_SIZE];

static int major;
static struct class *hello_class = NULL;
static struct device *hello_device = NULL;
static dev_t dev_num;
static struct cdev hello_cdev; 

static int led_pin;

static int hello_open(struct inode *inode, struct file *file)
{
    int base = led_gpio[led_pin];
    gpiocon = (volatile unsigned long *)ioremap(base, 16);
    
    //pr_info("[1] gpiocon:%x , gpiocon:%x \n", &gpiocon, &gpiodat)

    //gpiocon = ioremap(base, 16);
    if (!gpiocon)
        return -ENOMEM;

    gpiodat = gpiocon + 1;
    //pr_info("[2] gpiocon:%x , gpiocon:%x \n", &gpiocon, &gpiodat)

    *gpiocon &= ~(0x3FF);  // 11 1111 1111 -> bit[9:0] = 00 0000 0000
    *gpiocon |= (0 << 0);  
    *gpiocon |= (0 << 2);
    *gpiocon |= (3 << 6); // 0011 -> 1100 0000 -> bit[7:6] = 11
    *gpiocon |= (1 << 9); // 1 0000 0000 -> bit[9] = 1 

    pr_info("hello_open ...\n");

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
    
    /* Remove newline characters introduced by echo */
    if (kernel_buf[len - 1] == '\n')
        kernel_buf[len - 1] = '\0';

    pr_info("hello_char: write received: %s\n", kernel_buf);

    /* Simulate LED control */
    if (strcmp(kernel_buf, "on") == 0) {
        *gpiodat &= ~(0x3<< 0); 
        *gpiodat |= (0x2 << 0); 
        pr_info("hello_char: LED ON\n");
    } else if (strcmp(kernel_buf, "off") == 0) {
        *gpiodat &= ~(0x3<< 0);
        pr_info("hello_char: LED OFF\n");
    } else {
        pr_info("hello_char: Unknown command\n");
    }

    return count; 
}

static int hello_release(struct inode *inode, struct file *file)
{
    if (gpiocon)
        iounmap(gpiocon);
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

static int hello_probe(struct platform_device *pdev)
{
    int retval;
    struct resource		*res;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	led_pin = res->start;
    pr_info("hello_probe and led_pin %d...\n", led_pin);

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



static int hello_remove(struct platform_device *pdev)
{
    pr_info("hello_remove ... \n");

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

    return 0;
}

struct platform_driver hello_drv = {
	.probe		= hello_probe,
	.remove		= hello_remove,
	.driver		= {
		.name	= "hello_led",
	}
};

static int __init hello_init(void)
{
    pr_info("hello_init ....\n");
    platform_driver_register(&hello_drv);

	return 0;
}

static void __exit hello_exit(void)
{
    platform_driver_unregister(&hello_drv);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("puck");
MODULE_DESCRIPTION("Hello platform driver");
