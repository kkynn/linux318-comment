#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mtd/mtd.h>
#include <linux/netdevice.h>

extern struct proc_dir_entry *realtek_proc;

void dump_stats(struct net_device_stats *stats)
{
	printk("rx_packets=%d\n",stats->rx_packets);
	printk("tx_packets=%d\n",stats->tx_packets);
	printk("rx_bytes=%d\n",stats->rx_bytes);
	printk("tx_bytes=%d\n",stats->tx_bytes);
	printk("rx_errors=%d\n",stats->rx_errors);
	printk("tx_errors=%d\n",stats->tx_errors);

/*	unsigned long	tx_packets;
	unsigned long	rx_bytes;
	unsigned long	tx_bytes;
	unsigned long	rx_errors;
	unsigned long	tx_errors;
	unsigned long	rx_dropped;
	unsigned long	tx_dropped;
	unsigned long	multicast;
	unsigned long	collisions;
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;
	unsigned long	rx_crc_errors;
	unsigned long	rx_frame_errors;
	unsigned long	rx_fifo_errors;
	unsigned long	rx_missed_errors;
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
*/

}
static int netdev_reset_proc_read(struct seq_file *s, void *v)
{
	return 0;
}

static int netdev_reset_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, netdev_reset_proc_read, NULL));
}

static int netdev_reset_proc_write( struct file *file, const char __user *buffer, 
						size_t count, loff_t *off ) {
	struct net_device *dev;
	struct net_device_stats *stats;

	char buf[128];

	if (copy_from_user(buf, buffer, count)) {
		return -EFAULT;
	}

	buf[count-1] = '\0';

	if (strcmp(buf, "all")==0){
		printk("clear all netdev stat counter\n");
		for_each_netdev(&init_net, dev)
		{
			//stats = (dev->get_stats ? dev->get_stats(dev): NULL);
			//stats = (dev->get_stats ? dev->get_stats(dev): &dev->stats);
			if ((dev->flags & IFF_POINTOPOINT) && (dev->flags & IFF_NOARP))
			{
				extern void clear_ppp_stats(struct net_device *dev);
				clear_ppp_stats(dev);
			}
			else
			{
				stats = (dev->netdev_ops->ndo_get_stats ? dev->netdev_ops->ndo_get_stats(dev):&dev->stats);
				if(stats) {
					memset(stats, 0, sizeof(struct net_device_stats));
				}
			}
		}
	} else {
		for_each_netdev(&init_net, dev)
		{
			if (strcmp(dev->name, buf)==0){
				printk("clear %s netdev stat counter\n", buf);
				//stats = (dev->get_stats ? dev->get_stats(dev): NULL);
				//stats = (dev->get_stats ? dev->get_stats(dev): &dev->stats);
				//stats = &dev->netdev_ops->ndo_get_stats;
				stats = (dev->netdev_ops->ndo_get_stats ? dev->netdev_ops->ndo_get_stats(dev):&dev->stats);
				if(stats) {
					memset(stats, 0, sizeof(struct net_device_stats));
				}
				break;
			}
		}
	}

	return count;
}


static const struct file_operations fops_reset_netdev_stats = {
	.open           = netdev_reset_single_open,
	.write          = netdev_reset_proc_write,
	.read           = seq_read,
    .llseek         = seq_lseek,
	.release        = single_release,
};

static int __init netdev_reset_init(void) {
	struct proc_dir_entry *pe;
	
	pe = proc_create_data("netdev_reset", S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH, realtek_proc,
                                                             &fops_reset_netdev_stats, NULL);

	if (!pe) {
		return -EINVAL;
	}
	
	return 0;
}

static void __exit netdev_reset_exit(void) {
	return;
}

late_initcall_sync(netdev_reset_init);
module_exit(netdev_reset_exit); 

MODULE_DESCRIPTION("netdev_reset_stat"); 
MODULE_AUTHOR("Frank Liao<frankliao@realtek.com>"); 
MODULE_LICENSE("GPL"); 
