/*
 * hello.c - The simplest Linux kernel module.
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */

int init_module(void)
{
        printk(KERN_ALERT "Hello from the Kernel!\n");

        // A non 0 return means init_module failed; module can't be loaded.
        return 0;
}

void cleanup_module(void)
{
        printk(KERN_ALERT "Goodbye, I'm out for dinner.\n");
}

MODULE_LICENSE("GPL");
