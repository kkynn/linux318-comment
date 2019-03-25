/*
 * FILE NAME rtl_signalCapture.c
 *
 * BRIEF MODULE DESCRIPTION
 *  Capture fatal signal to User Process.
 *
 *  Author: Martin_ZHU@realsil.com.cn
 *
 * Copyright 2017 Realsil Semiconductor Corp.
 *
 */



#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/reboot.h>
#include <linux/kmod.h>
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <linux/mtd/mtd.h>
#include <rtk/gpio.h>
#include <rtk/irq.h>
#include <common/error.h>
#include <asm/fatalSignal.h>


unsigned int 	signal_num_flag=0;
unsigned int	exit_app_num = 0;
fatalSignal_t	exitProcessArray[MAX_EXIT_PROCESS];

static int signal_onoff_read_proc(struct seq_file *seq, void *v)
{
	int len = 0;
	len = seq_printf(seq,"0x%08x\n",signal_num_flag);

	return len;
}

static int signal_onoff_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;

	if (buffer && !copy_from_user(tmpBuf, buffer, len))
	{
		signal_num_flag=simple_strtoul(tmpBuf, NULL, 16);
		printk("Capture signal: 0x%08x\n", signal_num_flag);
		return count;
	}
	return -EFAULT;
}


static int  signalonoff_open(struct inode *inode, struct file *file)
{
        return single_open(file, signal_onoff_read_proc, inode->i_private);
}

static const struct file_operations signalonoff_fops = {
        .owner          = THIS_MODULE,
        .open           = signalonoff_open,
        .read           = seq_read,
        .write          = signal_onoff_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int fatalSignal_read_proc(struct seq_file *seq, void *v)
{
	int i;

	for(i=0; i<exit_app_num; i++)
	{
		if( strlen(exitProcessArray[i].pName) )
		{
			seq_printf(seq, "%s %d\n", exitProcessArray[i].pName, exitProcessArray[i].signal_no);
		}
	}

	return 0;
}

static int fatalSignal_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	memset(exitProcessArray, 0, sizeof(fatalSignal_t)*MAX_EXIT_PROCESS);
	exit_app_num = 0;

	return count;
}


static int  fatalSignal_open(struct inode *inode, struct file *file)
{
        return single_open(file, fatalSignal_read_proc, inode->i_private);
}

static const struct file_operations fatalSignal_fops = {
        .owner          = THIS_MODULE,
        .open           =fatalSignal_open,
        .read           = seq_read,
        .write          = fatalSignal_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int __init rtl_signalCapture_init(void)
{
	int ret = 0;
	
	printk("Init Realtek SignalCapture Driver. \n");

	proc_create_data("signal_onoff", 0, NULL,&signalonoff_fops, NULL);

	proc_create_data("fatalSignal", 0, NULL,&fatalSignal_fops, NULL);

	memset(exitProcessArray, 0, sizeof(fatalSignal_t)*MAX_EXIT_PROCESS);

	return ret;
}

static void __exit rtl_signalCapture_exit(void)
{
	printk("Unload Realtek SignalCapture Driver. \n");
}

//2017-03-09:this moudule must init later than irq related struct
module_init(rtl_signalCapture_init);
//module_init(rtl_gpio_init);
module_exit(rtl_signalCapture_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for Capture Fatal Signal");

