/*
 * Copyright (C) 2011 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 *
 *
 * Purpose : Limits file size for tmpfs
 *           Kernel doesn't only aim at file size in tmpfs.
 * Feature :
 *
 */


/*
  * Include Files
  */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <asm/mipsregs.h>

#include <bspchip.h>
#include <bspchip_9607c.h>

unsigned long tmpfs_file_size_limit = (256 << 20); //256MB, by default.

static int tmp_file_size_show(struct seq_file *seq, void *v)
{
    seq_printf(seq, " tmpfs's file size limit : %lu\n", tmpfs_file_size_limit );
    return 0;
}

static ssize_t tmpfs_file_size_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{

    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;
    int input_val = 0;
    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        input_val = simple_strtoul(tmpBuf, NULL, 0);
       if(input_val >= (1<<20)){
           tmpfs_file_size_limit = input_val;
       }else{
          printk("tmp file size should > 1MB\n");
		   tmpfs_file_size_limit = (1<<20);
       }
        return size;
    }
    return -EFAULT;
}


struct proc_dir_entry *yueme_proc_dir = NULL;

static int tmpfs_file_size_open(struct inode *inode, struct file *file)
{
    return single_open(file, tmp_file_size_show, inode->i_private);
}
static const struct file_operations bsp_luna_test_fops = {
    .owner      = THIS_MODULE,
    .open       = tmpfs_file_size_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = tmpfs_file_size_write,
};


int __init luna_yueme_init(void)
{

    yueme_proc_dir = proc_mkdir("luna_yueme", NULL);
    if (yueme_proc_dir == NULL) {
        printk("create /proc/luna_yueme failed!\n");
        return 1;
    }

    if(! proc_create("tmpfs_file_size", 0444, yueme_proc_dir, &bsp_luna_test_fops)){
      printk("create proc luna_yueme/tmpfs_file_size failed!\n");
      return 1;
    }

    return 0;
}

void __exit luna_yueme_exit(void)
{
     remove_proc_entry("tmpfs_file_size", yueme_proc_dir);
     proc_remove(yueme_proc_dir);
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek TMPFS Limit : ");
module_init(luna_yueme_init);
module_exit(luna_yueme_exit);
