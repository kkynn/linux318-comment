#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <bspchip.h>
#include <bspgpio.h>
#include <rtl_usb_phy.h>

extern struct proc_dir_entry *realtek_proc;

static void show_usage(void){
	//printk("echo [command] [arg...] > /proc/realtek/rtk_usb_phy\n");
	//printk("command : \n");
	//printk("	help : show controlling commands\n");
	printk("	w [reg] [value] : write [value] to [reg] of port1 (value should be 8 bits, reg should be in [E0-E7], [F0-F6])\n");
	//printk("cat /proc/realtek/rtk_usb_phy\n");
	//printk("	dump all usb phy's registers to console\n");
}

static ssize_t usb_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off) {
	char buf[32];
	int len;
	u8 reg;
	u16 val;
	struct rtk_usb_phy *phy = PDE_DATA(file_inode(file));
	
	len = min(sizeof(buf), count);
	if (copy_from_user(buf, userbuf, len))
		return -E2BIG;
		
	if (strncmp(buf, "help", 4) == 0) {
		show_usage();
	} else if (strncmp(buf, "w ", 2) == 0) {
		if(2==sscanf(buf, "w %hhx %hx", &reg, &val)){
			//printk("EHCI: write %hhx to phy port %d, phy(%hhx)\n", val, PHY_PORT, reg);
			//ehci_phy_write(PHY_PORT, reg, val);
			if (phy->phy_write(phy, phy->port, reg, val))
				return -EINVAL;
		}
		else{
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "r ", 2) == 0) {
		if(1==sscanf(buf, "r %hhx", &reg)){
			if (phy->phy_read(phy, phy->port, reg, &val))
				return -EINVAL;
			
			printk("%02x = %02x\n", reg, val);
		}
	} else {
		goto ERROR_PARA;
	}
	return count;
ERROR_PARA:
	printk("error parameter...\n");
	show_usage();
	return -EPERM;
}


#define ITEM_PER_LINE 4
static int rtk_usb_phy_show(struct seq_file *s, void *v) {
	struct rtk_usb_phy *phy = s->private;
	int reg = -1, n = 0;
	u16 data;

	while (0==phy->phy_get_next(phy, phy->port, &reg)) {
		n++;
		if (0==phy->phy_read(phy, phy->port, reg, &data))
			seq_printf(s, "%02x: %4x   ", reg, data);
		else
			seq_printf(s, "%02x:  err   ", reg);
		
		if ((n%ITEM_PER_LINE)==0)
			seq_printf(s, "\n");
	}
	seq_printf(s, "\n");
	return 0;
}

static int rtk_usb_phy_open(struct inode *inode, struct file *file) {	
	return(single_open(file, rtk_usb_phy_show, PDE_DATA(inode)));
}


static const struct file_operations fops_rtk_usb_phy = {
	.open  		= rtk_usb_phy_open,
	.write 		= usb_proc_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int rtk_usb_phy_register(struct rtk_usb_phy *phy) {
	
	struct proc_dir_entry *e;
	
	if (phy->phy_init)
		phy->phy_init(phy);
		
	e = proc_create_data(phy->name, S_IRUGO | S_IWUSR, realtek_proc, &fops_rtk_usb_phy, phy);
	if (!e) {
		printk("Failed to register USB_PHY %s\n", phy->name);
		return -EINVAL;
}
	return 0;		
}
EXPORT_SYMBOL(rtk_usb_phy_register);

MODULE_DESCRIPTION("Realtek USB proc module");
MODULE_AUTHOR("Scott Wu <scott27585206@realtek.com>");
MODULE_LICENSE("GPL");
