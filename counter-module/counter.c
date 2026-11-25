#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bne7809");
MODULE_DESCRIPTION("/proc/hello read counter");

static atomic_t counter = ATOMIC_INIT(0);
static struct proc_dir_entry *entry;

static int hello_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", atomic_inc_return(&counter));
	return 0;
}

static int hello_open(struct inode *inode, struct file *file)
{
	return single_open(file, hello_show, NULL);
}

static ssize_t hello_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
	char  kbuf[16];
	int val;

	if (len >= sizeof(kbuf))
		return -EINVAL;
	if (copy_from_user(kbuf, buf, len))
		return -EFAULT;
	kbuf[len] = '\0';
	if (kstrtoint(kbuf, 10, &val))
		return -EINVAL;
	atomic_set(&counter, val);
	pr_info("hello: counter set to %d\n", val);
	return len;
}

static const struct proc_ops hello_ops = {
	.proc_open = hello_open,
	.proc_read = seq_read,
	.proc_write = hello_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

static int __init counter_init(void)
{
	entry = proc_create("hello", 0644, NULL, &hello_ops);
	if (!entry)
		return -ENOMEM;
	pr_info("hello: /proc/hello created\n");
	return 0;
}

static void __exit counter_exit(void)
{
	proc_remove(entry);
	pr_info("hello: /proc/hello removed\n");
}

module_init(counter_init);
module_exit(counter_exit);
