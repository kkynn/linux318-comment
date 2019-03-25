#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
#include <net/rtl/rtl_types.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/spinlock_types.h>
#include <linux/seq_file.h>
#include <uapi/linux/if.h>
#include <net/sock.h>
#include <linux/netlink.h>


#define DRIVER_AUTHOR 	"IulianWu"
#define DRIVER_DESC 	"RealTek WLAN Event Counters"

static struct proc_dir_entry *entry = NULL;
static struct proc_dir_entry *parent;
#if defined(CONFIG_LUNA_DUAL_LINUX)	
static struct proc_dir_entry *wlan1;
#endif
struct sock *nl_sk=NULL;					//used for hanlde RTK WLAN driver 


#define PROCFS_WLAN_EVENT_COUNTER			"wlan_event_counter"
#define PROCFS_OSGI_DIR0 					"osgi/wlan0"
#define PROCFS_OSGI_DIR1 					"osgi/wlan1"
#define WLAN_EVENT_DISAPPEAR_TIMEOUT		10 //if counter didn't increase in 10secs, adjust the traffic is disappear.
#define MACADDRLEN	6
#define NETLINK_RTK_WLAN_EVENT 31			//used for hanlde RTK WLAN driver 

static DEFINE_SPINLOCK(wlan_event_unres_lock); //protect the list

typedef struct wlan_event_entry_s
{
	int eventID;
	unsigned char mac[MACADDRLEN];
	char ifname[IFNAMSIZ];
	char reason;
	long timer;
    struct list_head list;
} wlan_event_entry_t;

wlan_event_entry_t wlan_event_entry_list;


void rtk_eventd_netlink_send(int pid, struct sock *nl_sk, int eventID, char *ifname, char *data, int data_len)
{
	unsigned char mac_addr[MACADDRLEN];
	char if_name[IFNAMSIZ];
	char reason;
	wlan_event_entry_t *pWlanEventEntry; 
	wlan_event_entry_t *node = NULL;
	
	memcpy(mac_addr, data, MACADDRLEN);
	memcpy(if_name, data+MACADDRLEN+1, IFNAMSIZ);
	reason = data[MACADDRLEN];
#if __DEBUG	
	printk("[%s:%d] eventID=%d, if_name=%s mac_addr=%02x:%02x:%02x:%02x:%02x:%02x, reason=%d \n", 
		__FUNCTION__, __LINE__,eventID, if_name,
		mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], reason);
#endif

	if(!list_empty(&wlan_event_entry_list.list)){
		list_for_each_entry(node, &(wlan_event_entry_list.list), list)
		{
			if(!memcmp(mac_addr, node->mac, MACADDRLEN) && (eventID == node->eventID)){
#if __DEBUG				
				printk("[%s:%d] Found the same MAC \n",__FUNCTION__, __LINE__);
#endif
				return;
			}
		}
	}
	
	pWlanEventEntry = kmalloc(sizeof(wlan_event_entry_t), GFP_ATOMIC);
	if(!pWlanEventEntry)
		return;
	memset(pWlanEventEntry, 0, sizeof(wlan_event_entry_t));

	pWlanEventEntry->eventID = eventID;
	memcpy(pWlanEventEntry->mac, mac_addr, MACADDRLEN);
	memcpy(pWlanEventEntry->ifname, if_name, IFNAMSIZ);
	pWlanEventEntry->reason = reason;

	spin_lock(&wlan_event_unres_lock);
	list_add_tail(&(pWlanEventEntry->list),&(wlan_event_entry_list.list));
	spin_unlock(&wlan_event_unres_lock);
	
}
EXPORT_SYMBOL(rtk_eventd_netlink_send);

int get_nl_eventd_pid(void)
{
    return 1;
}
EXPORT_SYMBOL(get_nl_eventd_pid);

struct sock *get_nl_eventd_sk(void)
{
    return &nl_sk;
}
EXPORT_SYMBOL(get_nl_eventd_sk);

struct sock *rtk_eventd_netlink_init(void)
{
    return nl_sk;
}
EXPORT_SYMBOL(rtk_eventd_netlink_init);

static struct mfc_cache *wlan_event_seq_idx(loff_t pos)
{
	wlan_event_entry_t *node = NULL;
	list_for_each_entry(node, &(wlan_event_entry_list.list), list)
		if (pos-- == 0)
			return node;
    	return NULL;
}

static void *wlan_event_seq_start(struct seq_file *s, loff_t *pos) 
{ 
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos);		 
#endif
	return *pos ? wlan_event_seq_idx(*pos - 1)	: SEQ_START_TOKEN;
}
static void *wlan_event_seq_next(struct seq_file *s, void *v, loff_t *pos) 
{ 
	++*pos;
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos); 	 
#endif
	return wlan_event_seq_start(s, pos);
} 

static void wlan_event_seq_stop(struct seq_file *s, void *v) 
{ 
#if __DEBUG
	printk("[%s:%d] \n", __FUNCTION__, __LINE__); 	 
#endif	
	wlan_event_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(wlan_event_entry_list.list), list)
	{
        	list_del(&node->list);
        	kfree(node);		
	}
} 

static int wlan_event_seq_show(struct seq_file *seq, void *v) 
{ 
	int n;
	
	if (v == SEQ_START_TOKEN) {
		seq_puts(seq,  "eventID ifname mac reason\n");
	} 
	else {
		struct timespec now;
		long curr_time;		
		wlan_event_entry_t *node = v;
		now = current_kernel_time();
		curr_time = now.tv_sec;	
#if __DEBUG
		printk("[%s:%d] %d %s %02x:%02x:%02x:%02x:%02x:%02x %d\n", __FUNCTION__, __LINE__,
				node->eventID, node->ifname, node->mac[0], node->mac[1], node->mac[2], 
				node->mac[3], node->mac[4], node->mac[5], node->reason);
#endif
		seq_printf(seq, "%d %s %02x:%02x:%02x:%02x:%02x:%02x %d\n",
				node->eventID, node->ifname, node->mac[0], node->mac[1], node->mac[2], 
				node->mac[3], node->mac[4], node->mac[5], node->reason);
		
	}
	return 0;
}

static struct seq_operations wlan_event_counter_ops = { 
	.start			= wlan_event_seq_start,
	.next			= wlan_event_seq_next,
	.stop			= wlan_event_seq_stop,
	.show			= wlan_event_seq_show 
};

static int wlan_event_counter_open(struct inode *inode, struct file *file) 
{ 
	return seq_open(file, &wlan_event_counter_ops);
};

static struct file_operations wlan_event_counter_fops = { 
	.owner 			= THIS_MODULE,
	.open			= wlan_event_counter_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= seq_release 
};


static void wlan_event_entry_list_init(void)
{
    INIT_LIST_HEAD(&wlan_event_entry_list.list);
}


static int __init wlan_event_init_main(void)
{
	parent = proc_mkdir(PROCFS_OSGI_DIR0, NULL);
#if defined(CONFIG_LUNA_DUAL_LINUX)	
	wlan1 = proc_mkdir(PROCFS_OSGI_DIR1, NULL);
#endif
	entry = proc_create_data(PROCFS_WLAN_EVENT_COUNTER, 0444, parent, &wlan_event_counter_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR0, PROCFS_WLAN_EVENT_COUNTER);
	}
	wlan_event_entry_list_init();

	nl_sk = netlink_kernel_create(&init_net, NETLINK_RTK_WLAN_EVENT, NULL);//used for hanlde RTK WLAN driver 
	if(!nl_sk)
		printk("%s:%d] netlink_kernel_create failed !! \n", __FUNCTION__, __LINE__);
	printk("%s: Successfully inserted a hook into kernel\n", __FUNCTION__);
	return 0;
}

static void __exit wlan_event_cleanup_main(void)
{
	wlan_event_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(wlan_event_entry_list.list), list)
	{
        	list_del(&node->list);
		kfree(node);		
	}

	remove_proc_entry(PROCFS_WLAN_EVENT_COUNTER, parent);
	proc_remove(parent);
#if defined(CONFIG_LUNA_DUAL_LINUX)	
	proc_remove(wlan1);
#endif
	printk("Successfully unloaded the hook\n");
}


late_initcall(wlan_event_init_main);
module_exit(wlan_event_cleanup_main);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);


