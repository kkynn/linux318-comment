#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#define RTK_SYS_R32		0x01
#define RTK_SYS_W32		0x02
#define RTK_SYS_R16		0x03
#define RTK_SYS_W16		0x04
#define RTK_SYS_R8		0x05
#define RTK_SYS_W8		0x06
#define RTK_SYS_DMEM	0x07

#define RTK_MEM_NAME "rtmem"

/*
 * For SYS_R32,SYS_R16,SYS_R8, arg is 2-words(1 input, 1 ouput)
 *   <address> [buffer]
 * For SYS_W32,SYS_W16,SYS_W8, arg is 2-words
 *   <address> <buffer>
 * For SYS_DMEM arg is 3-words
 *   <address> <length> <pointer to buffer>
 */
static long rtk_mem_ioctl(struct file *file, unsigned int req,	unsigned long arg)
{
	void *pAddr;
	u32 __user *p = (u32 __user *)arg;
	u32 buffer[2];
	u32 addr;
	
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	
	get_user(addr, (u32 __user *)p);
	
	switch(req) {
	case RTK_SYS_R32:
		//printk("R32:<%x>=%x\n",addr,*((volatile u32 *)addr));
		put_user(*((volatile u32 *)addr), (u32 __user *)&p[1]);
		break;
	case RTK_SYS_W32:
		//printk("W32:%x<-%x\n",addr,*((u32 __user *)&p[1]));
		get_user(*((volatile u32 *)addr), (u32 __user *)&p[1]);
		break;
	case RTK_SYS_R16:
		//printk("R16:<%x>=%x\n",addr,*((volatile u16 *)addr));
		put_user(*((volatile u16 *)addr), (u16 __user *)&p[1]);
		break;
	case RTK_SYS_W16:	
		//printk("W16:%x<-%x\n",addr,*((u16 __user *)&p[1]));
		get_user(*((volatile u16 *)addr), (u16 __user *)&p[1]);
		break;
	case RTK_SYS_R8:
		//printk("R8:<%x>=%x\n",addr,*((volatile u8 *)addr));
		put_user(*((volatile u8 *)addr), (u8 __user *)&p[1]);
		break;
	case RTK_SYS_W8:
		//printk("W16:%x<-%x\n",addr,*((u8 __user *)&p[1]));
		get_user(*((volatile u8 *)addr), (u8 __user *)&p[1]);
		break;
	case RTK_SYS_DMEM:
		do {
			u32 len;
			get_user(len, (u32 __user *)&p[1]);
			copy_to_user(p[2], addr, len);
		} while (0);
		break;
	default:
		;
	}
	return 0;
}

static struct file_operations mem_fops =
{
	owner:		THIS_MODULE,
	unlocked_ioctl:	rtk_mem_ioctl,
};

static struct miscdevice rtk_mem_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = RTK_MEM_NAME,
	.fops  = &mem_fops
};

static int __init rtk_mem_init(void)
{
	int ret;
	
	ret = misc_register(&rtk_mem_misc_device);
	if (ret) {
		printk("%s: init failed, %d", RTK_MEM_NAME,ret);
		return ret;
	}
	
	return 0;
}

static void __exit rtk_mem_exit(void)
{
	misc_deregister(&rtk_mem_misc_device);	
}

module_init(rtk_mem_init);
module_exit(rtk_mem_exit);

MODULE_DESCRIPTION("Realtek System Memory Module");
MODULE_AUTHOR("Andrew Chang <yachang@realtek.com>");
MODULE_LICENSE("GPL");