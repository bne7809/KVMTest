#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bne7809");
MODULE_DESCRIPTION("Hello world kernel module");

static int __init hello_init(void)
{
	pr_info("hello: loaded into kernel\n");
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello: unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);
