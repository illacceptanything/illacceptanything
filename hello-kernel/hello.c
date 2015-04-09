/*
 * hello.c - The simplest Linux kernel module.
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/init.h>    /* Needed for __init and __exit */

static int __init init_module(void)
{
        pr_debug("Hello from the Kernel!\n");

        // A non 0 return means init_module failed; module can't be loaded.
        return 0;
}

static void __exit cleanup_module(void)
{
        pr_debug("Goodbye, I'm out for dinner.\n");
}

MODULE_LICENSE("GPL");

module_init(init_module);
moudle_exit(cleanup_module);
