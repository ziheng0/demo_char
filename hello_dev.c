#include <linux/module.h>
#include <linux/platform_device.h>

/*
 * This file only describes the existence of the device.
 * It does NOT contain any driver or char device logic.
 */

// static struct resource led_resource[] = {
//     [0] = {
//         .start = 2,
//         .end   = 2,
//         .flags = IORESOURCE_MEM,
//     },
// };

static void hello_release(struct device * dev)
{
    pr_info("hello_device: hello_release\n");
}

/* Define a platform device */
static struct platform_device hello_pdev = {
    .name = "hello_led",   /* Must match platform_driver name */
    .id   = -1,
    //.num_resources      = ARRAY_SIZE(hello_resource),
    //.resource           = hello_resource,
    .dev                = { 
    	                .release = hello_release, 
	},
};

static int __init hello_device_init(void)
{
    pr_info("hello_device: register platform device\n");

    /* Register platform device to the kernel */
    return platform_device_register(&hello_pdev);
}

static void __exit hello_device_exit(void)
{
    pr_info("hello_device: unregister platform device\n");

    /* Unregister platform device */
    platform_device_unregister(&hello_pdev);
}

module_init(hello_device_init);
module_exit(hello_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("puck");
MODULE_DESCRIPTION("Hello platform device");
