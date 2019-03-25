#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <uapi/linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/types.h>
#include <linux/inet.h>
#include <net/rtl/rtl_types.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/spinlock_types.h>

//#define __DEBUG 1

#define DRIVER_AUTHOR 	"IulianWu"
#define DRIVER_DESC 	"Traffic Monitor"

struct sk_buff *sock_buff;
static struct nf_hook_ops nfho;
static struct proc_dir_entry *proc_dir = NULL;

#define PROCFS_TRAFFIC_MONITOR_ADD		"traffic_monitor_add"
#define PROCFS_TRAFFIC_MONITOR_MOD		"traffic_monitor_mod"
#define PROCFS_TRAFFIC_MONITOR_DEL		"traffic_monitor_del"
#define PROCFS_TRAFFIC_MONITOR_COUNTER	"traffic_monitor_counter"
#define PROCFS_OSGI_DIR 			"osgi"
#define TRAFFIC_MONITOR_DISAPPEAR_TIMEOUT	10 //if counter didn't increase in 10secs, adjust the traffic is disappear.

static DEFINE_SPINLOCK(traffic_monitor_unres_lock); //protect the list 

typedef struct traffic_montior_key_entry_s
{
	int bundleID;
	__be32 daddr;
	int netmask;
	char url[128];
} traffic_montior_key_entry_t;

typedef struct traffic_montior_entry_s
{
	traffic_montior_key_entry_t key;
	int counter;
	long timer;
	bool detect_flag;
	bool dispear_flag;
    struct list_head list;
} traffic_montior_entry_t;

traffic_montior_entry_t traffic_montior_entry_list;

static bool ipAddress_match(__be32 addr, __be32 net, uint8 bits) {
	if (bits == 0) {
		return true;
	}
#if __DEBUG		
	printk("Compare the ip: %pI4 , net:%pI4 , netmask:%d\n", &addr, &net, bits );
#endif
	return !((addr ^ net) & htonl(0xFFFFFFFFu << (32 - bits)));
}


/* Older kernel has different funcion definition. */
static unsigned int hook_func(
		const struct nf_hook_ops *ops,
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	char *ifname = (char *)(in->name);
	traffic_montior_entry_t *node = NULL;
	struct ethhdr *mac_header = (struct ethhdr *)skb_mac_header(skb);
	struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);
	sock_buff = skb;
	struct timespec now;

	if (!sock_buff) {
		return NF_ACCEPT;
	}

	//check the entry is empty or not
	if(!list_empty(&traffic_montior_entry_list.list)){ 
		list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		{
			if (ipAddress_match(ip_header->daddr, node->key.daddr, node->key.netmask )) {
				if(node->counter == 0) {
					node->detect_flag = TRUE;
				}
				node->counter++;
				now = current_kernel_time();
				node->timer = now.tv_sec;
#if __DEBUG			
				printk("======Found the monitor destination ipaddress in dev: %s=====\n", ifname);
				printk("src_mac: %pM ", mac_header->h_source);
				printk("dst_mac: %pM ", mac_header->h_dest);
				printk("src_ip: %pI4 ", &ip_header->saddr);
				printk("dst_ip: %pI4 ", &ip_header->daddr);
				printk("timer: %ld ", 	now.tv_sec);
				printk("\n");
#endif			
			}

		}	
	}
#if __DEBUG		
	else {	
		printk("====== list is empty! \n");
	}
#endif	
	return NF_ACCEPT;
}

static int proc_traffic_monitor_add_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[512] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	int bundle_id, netmask;
	traffic_montior_entry_t *pTrafficMonitorEntry;
	traffic_montior_entry_t *node = NULL; 
	
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		pTrafficMonitorEntry = kmalloc(sizeof(traffic_montior_entry_t), GFP_ATOMIC);
		if (!pTrafficMonitorEntry)
			return -1;
		memset(pTrafficMonitorEntry, 0, sizeof(traffic_montior_entry_t));

		strptr = tmpbuf;
		/* Bundle ID */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) return -EFAULT;
		bundle_id = simple_strtol(tokptr, NULL, 0);
		pTrafficMonitorEntry->key.bundleID = bundle_id;

		/* IP address */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) return -EFAULT;
		pTrafficMonitorEntry->key.daddr= in_aton(tokptr);

		/* netmask */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) return -EFAULT;
		netmask = simple_strtol(tokptr, NULL, 0);
		pTrafficMonitorEntry->key.netmask = netmask;

		/* url */
		tokptr = strsep(&strptr, "\n");
		if (!tokptr) {
			strcpy(pTrafficMonitorEntry->key.url, "na");
		} else {
			strcpy(pTrafficMonitorEntry->key.url, tokptr);
		}
#if __DEBUG			
		printk("The [%s:%d] url of  Bundle ID=%d is pTrafficMonitorEntry->key.url=%s addr=%pI4 \n", 
				__FUNCTION__, __LINE__, bundle_id,  pTrafficMonitorEntry->key.url,
				&pTrafficMonitorEntry->key.daddr );
#endif		
        //Found the entry exist or not
		list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		{
			if((pTrafficMonitorEntry->key.bundleID == node->key.bundleID) &&
				strcmp(pTrafficMonitorEntry->key.url, "na") &&
				!strcmp(pTrafficMonitorEntry->key.url, node->key.url)) 
			{
#if __DEBUG	
				printk("The Entry already exist of Bundle ID=%d url=%s\n", bundle_id, pTrafficMonitorEntry->key.url);
#endif			
				return count;
			}
				
			if(!memcmp (&pTrafficMonitorEntry->key, &node->key, sizeof(node->key))){
#if __DEBUG	
				printk("The Entry already exist of Bundle ID=%d url=%s addr=%pI4\n", 
				bundle_id, pTrafficMonitorEntry->key.url, &pTrafficMonitorEntry->key.daddr);
#endif
				return count;
			}
		}
		spin_lock(&traffic_monitor_unres_lock);
		list_add_tail(&(pTrafficMonitorEntry->list),&(traffic_montior_entry_list.list));
		spin_unlock(&traffic_monitor_unres_lock);
	}
	return count;
}

static int proc_traffic_monitor_mod_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[512] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	int bundle_id, netmask;
	traffic_montior_entry_t *pTrafficMonitorEntry;
	traffic_montior_entry_t *node = NULL; 
	
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		pTrafficMonitorEntry = kmalloc(sizeof(traffic_montior_entry_t), GFP_ATOMIC);
		if (!pTrafficMonitorEntry)
			return -1;
		memset(pTrafficMonitorEntry, 0, sizeof(traffic_montior_entry_t));

		strptr = tmpbuf;
		/* Bundle ID */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) return -EFAULT;
		bundle_id = simple_strtol(tokptr, NULL, 0);
		pTrafficMonitorEntry->key.bundleID = bundle_id;

		/* IP address */
		tokptr = strsep(&strptr, " ");	
		if (!tokptr) return -EFAULT;
		pTrafficMonitorEntry->key.daddr= in_aton(tokptr);

		/* netmask */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) return -EFAULT;
		netmask = simple_strtol(tokptr, NULL, 0);
		pTrafficMonitorEntry->key.netmask = netmask;

		/* url */
		tokptr = strsep(&strptr, "\n");
		if (!tokptr)
			strcpy(pTrafficMonitorEntry->key.url, "");
		else
			strcpy(pTrafficMonitorEntry->key.url, tokptr);
			
        //Found the entry exist or not
		list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		{	
			if(pTrafficMonitorEntry->key.bundleID == node->key.bundleID ){
				if( strcmp(pTrafficMonitorEntry->key.url, "") &&
					!strcmp(pTrafficMonitorEntry->key.url, node->key.url))
				{

				node->key.daddr = pTrafficMonitorEntry->key.daddr;
				node->key.netmask = pTrafficMonitorEntry->key.netmask;			
#if __DEBUG	
				printk("Modify Bundle ID=%d ip_addr=%pI4 netmask=%d url=%s\n", bundle_id, &pTrafficMonitorEntry->key.daddr, pTrafficMonitorEntry->key.netmask, pTrafficMonitorEntry->key.url);
#endif
				return count;
				}
			}
		}
	}
	return count;
}

static int proc_traffic_monitor_read(struct seq_file *seq, void *v)
{
	traffic_montior_entry_t *node = NULL; 

	printk("SYNOPSIS:\n");
	printk("         echo \"[bundleID] [IP] [netmask] [url]\" > /proc/osgi/%s\n", PROCFS_TRAFFIC_MONITOR_ADD);
	printk("example  echo \"1 8.8.8.8 24\" > /proc/osgi/%s\n", PROCFS_TRAFFIC_MONITOR_ADD);
	printk("\n");
	printk("DESCRIPTION:\n");
	printk("         Add/Modify the bundle ID and IP address to traffic monitor\n");
		
	list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
	{
 		printk("Bundle ID:%d daddr = %pI4/%d url:%s counter:%d timer:%d detect_flag:%d dispear_flag:%d \n",
				node->key.bundleID, &(node->key.daddr), node->key.netmask, node->key.url, node->counter, node->timer, node->detect_flag, node->dispear_flag); 
	}
	return 0;
}

static int proc_traffic_monitor_del(struct seq_file *seq, void *v)
{
	traffic_montior_entry_t *node = NULL;

	printk("SYNOPSIS:\n");
	printk("         echo \"[bundleID]\" > /proc/osgi/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);
	printk("example  echo \"1\" > /proc/osgi/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);
	printk("         echo \"[bundleID] [IP] [netmask] | [url]\" > /proc/osgi/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);
	printk("example  echo \"1 8.8.8.8 24 \" > /proc/osgi/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);	
	printk("example  echo \"1 www.google.com \" > /proc/osgi/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);	
	printk("\n");
	printk("DESCRIPTION:\n");
	printk("         1.Delete the all IP address belong to the bundle ID from traffic monitor\n");
	printk("         2.Delete the bundle ID and IP address from traffic monitor\n");
	list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
	{
		printk("Bundle ID:%d daddr = %pI4 netmask=%d url:%s\n", node->key.bundleID, &(node->key.daddr), node->key.netmask,  node->key.url);
	} 
	return 0;
}


static int proc_traffic_monitor_add_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_traffic_monitor_read, inode->i_private);
}

static int proc_traffic_monitor_mod_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_traffic_monitor_read, inode->i_private);
}

static int proc_traffic_monitor_del_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_traffic_monitor_del, inode->i_private);
}


static int proc_traffic_monitor_del_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[512] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	__be32 ip_addr;
	int bundle_id, netmask;
	traffic_montior_entry_t *node = NULL, *tmp = NULL;
	char url[128]={0};
	
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {	
		strptr = tmpbuf;
		/* Bundle ID */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) return -EFAULT;
		bundle_id = simple_strtol(tokptr, NULL, 0);
		
		/* IP address */
		tokptr = strsep(&strptr, " ");
		if (!tokptr){
			ip_addr=0;
		}
		else {
			ip_addr=in_aton(tokptr);
		}

		/* netmask */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {
			netmask=0;
		} else {
			netmask = simple_strtol(tokptr, NULL, 0);
		}
		
		/* url */
		tokptr = strsep(&strptr, "\n");
		if (!tokptr) {
			strcpy(url, "");
		} else {
			strcpy(url, tokptr);
		}

		list_for_each_entry_safe(node, tmp, &(traffic_montior_entry_list.list), list)
		{
			if (node->key.bundleID == bundle_id) {			
				if (ip_addr == 0 && !strcmp(url, "")) { //Delete all list in bundle		
#if __DEBUG					
					printk("Delete Bundle ID =  %d \n", bundle_id);
#endif
					list_del(&node->list);
	        		kfree(node);
				}
				else {				
					if(strcmp(url, "")){ //if url exist, check url only
						if(!strcmp(url, node->key.url)) {
#if __DEBUG							
							printk("Delete Bundle ID=%d url=%s\n", bundle_id, url);
#endif
							list_del(&node->list);
		        			kfree(node);
						}
					}
					else if ((ip_addr == node->key.daddr) && (netmask == node->key.netmask)){ //check ip address					
#if __DEBUG							
						printk("Delete Bundle ID=%d ip_addr=%pI4 netmask=%d\n", bundle_id, &ip_addr, netmask);
#endif
						list_del(&node->list);
	        			kfree(node);
					}
				}
			}
		}
		
	}
	return count;
}

static struct mfc_cache *traffic_monitor_seq_idx(loff_t pos)
{
	traffic_montior_entry_t *node = NULL;
	list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		if (pos-- == 0)
                	return node;
	return NULL;
}

static void *traffic_monitor_seq_start(struct seq_file *s, loff_t *pos) 
{ 
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos);		 
#endif
	return *pos ? traffic_monitor_seq_idx(*pos - 1)	: SEQ_START_TOKEN;
}
static void *traffic_monitor_seq_next(struct seq_file *s, void *v, loff_t *pos) 
{ 
	++*pos;
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos); 	 
#endif
	return traffic_monitor_seq_start(s, pos);
} 

static void traffic_monitor_seq_stop(struct seq_file *s, void *v) 
{ 
#if __DEBUG
	printk("[%s:%d] \n", __FUNCTION__, __LINE__); 	 
#endif	
} 

static int traffic_monitor_seq_show(struct seq_file *seq, void *v) 
{ 
	int n;
	
	if (v == SEQ_START_TOKEN) {
		seq_puts(seq,  "id IP netmask detect dispear url\n");
	} 
	else {
		struct timespec now;
		long curr_time;		
		traffic_montior_entry_t *node = v;
		now = current_kernel_time();
		curr_time = now.tv_sec;
		
		/* Traffic dispear , reset the counter*/
		if ( node->counter !=0 && ((curr_time - node->timer) > TRAFFIC_MONITOR_DISAPPEAR_TIMEOUT) ) {
			node->counter = 0;
			node->timer = 0;
			node->dispear_flag = TRUE; 
		}

		seq_printf(seq, "%d %pI4 %d %d %d %s\n",
				node->key.bundleID, &(node->key.daddr), node->key.netmask, node->detect_flag, node->dispear_flag, node->key.url);
		
		/* This two flag used for sending OSGI event ,it should be clean after read */
		if (node->detect_flag) node->detect_flag = FALSE;
		if (node->dispear_flag) node->dispear_flag = FALSE;		
		
	}
	return 0;
}

static struct seq_operations traffic_monitor_counter_ops = { 
	.start			= traffic_monitor_seq_start,
	.next			= traffic_monitor_seq_next,
	.stop			= traffic_monitor_seq_stop,
	.show			= traffic_monitor_seq_show 
};

static int traffic_monitor_counter_open(struct inode *inode, struct file *file) 
{ 
	return seq_open(file, &traffic_monitor_counter_ops);
};

static struct file_operations traffic_monitor_counter_fops = { 
	.owner 			= THIS_MODULE,
	.open			= traffic_monitor_counter_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= seq_release 
};

static const struct file_operations traffic_monitor_add_fops = {
    .owner          = THIS_MODULE,
    .open           = proc_traffic_monitor_add_open,
    .read           = seq_read,
    .write          = proc_traffic_monitor_add_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static const struct file_operations traffic_monitor_mod_fops = {
    .owner          = THIS_MODULE,
    .open           = proc_traffic_monitor_mod_open,
    .read           = seq_read,
    .write          = proc_traffic_monitor_mod_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};


static const struct file_operations traffic_monitor_del_fops = {
    .owner          = THIS_MODULE,
    .open           = proc_traffic_monitor_del_open,
    .read           = seq_read,
    .write          = proc_traffic_monitor_del_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};


static void traffic_montior_entry_list_init(void)
{
    traffic_montior_entry_list.key.daddr	= -1;
    traffic_montior_entry_list.key.netmask = -1;
	memset(traffic_montior_entry_list.key.url, 0, sizeof(traffic_montior_entry_list.key.url));
    traffic_montior_entry_list.counter = -1;
    INIT_LIST_HEAD(&traffic_montior_entry_list.list);
}


static int __init init_main(void)
{
	struct proc_dir_entry *entry = NULL;
	
    proc_dir = proc_mkdir(PROCFS_OSGI_DIR, NULL);
	if ( proc_dir == NULL){
		printk("%s: Register to /proc/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_ADD, 0644, proc_dir, &traffic_monitor_add_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR, PROCFS_TRAFFIC_MONITOR_ADD);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_MOD, 0644, proc_dir, &traffic_monitor_mod_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR, PROCFS_TRAFFIC_MONITOR_MOD);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_DEL, 0644, proc_dir, &traffic_monitor_del_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR, PROCFS_TRAFFIC_MONITOR_DEL);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_COUNTER, 0444, proc_dir, &traffic_monitor_counter_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR, PROCFS_TRAFFIC_MONITOR_COUNTER);
	}


	traffic_montior_entry_list_init();
	nfho.hook = 	hook_func;
	//nfho.hooknum = 	NF_INET_PRE_ROUTING;
	nfho.hooknum = 	NF_INET_FORWARD;
	nfho.pf = 		NFPROTO_IPV4;
	nfho.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&nfho);
	
	printk("%s: Successfully inserted a hook into kernel\n", __FUNCTION__);

	return 0;
}

static void __exit cleanup_main(void)
{
	traffic_montior_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(traffic_montior_entry_list.list), list)
	{
        list_del(&node->list);
        kfree(node);		
	}

    remove_proc_entry(PROCFS_TRAFFIC_MONITOR_ADD, proc_dir);
	remove_proc_entry(PROCFS_TRAFFIC_MONITOR_MOD, proc_dir);
	remove_proc_entry(PROCFS_TRAFFIC_MONITOR_DEL, proc_dir);
	remove_proc_entry(PROCFS_TRAFFIC_MONITOR_COUNTER, proc_dir);
	remove_proc_entry(PROCFS_OSGI_DIR, NULL);
	
	nf_unregister_hook(&nfho);
	
	printk("Successfully unloaded the hook\n");
}

module_init(init_main);
module_exit(cleanup_main);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

