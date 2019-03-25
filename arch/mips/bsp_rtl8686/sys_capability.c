#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

extern struct proc_dir_entry *realtek_proc;

static int sys_capability_proc_read(struct seq_file *s, void *v)
{
	seq_printf(s, "%s\n", CONFIG_SYSTEM_CAPABILITY);
	return 0;
}

static int sys_capability_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, sys_capability_proc_read, NULL));
}


static const struct file_operations fops_sys_capability_stats = {
	.open           = sys_capability_single_open,
	.read           = seq_read,
        .llseek         = seq_lseek,
	.release        = single_release,
};

static int __init sys_capability_init(void) {
	struct proc_dir_entry *pe;
	
	pe = proc_create("sys_capability", S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH, realtek_proc,
                                                             &fops_sys_capability_stats);

	if (!pe) {
		return -EINVAL;
	}
	
	return 0;
}

static void __exit sys_capability_exit(void) {
	return;
}

late_initcall_sync(sys_capability_init);
module_exit(sys_capability_exit); 

MODULE_LICENSE("GPL"); 
