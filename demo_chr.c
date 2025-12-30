#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#define DEVICE_NAME "hello_char"
static int major;

static ssize_t hello_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    pr_info("hello_char: read called\n");
    return 0;
}

static struct file_operations fops ={
    .read = hello_read,
};

static int __init hello_init(void)
{
    pr_info("Hello from kernel module! \n");
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("Failed to register char device\n");
        return major;
    }
    pr_info("Hello char major number: %d\n", major);
	return 0;
}

static void __exit hello_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("Bye from kernel module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("puck");
MODULE_DESCRIPTION("Stage3: Char Device with file_operations");
