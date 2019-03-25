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
#include <linux/spinlock_types.h>
#include <linux/inetdevice.h>

#define DRIVER_AUTHOR 	"Iulian.Wu@realtek.com"
#define DRIVER_DESC 	"Mac filter event"
#define LOCAL_IN_INTERFACE "br0"

static struct nf_hook_ops nfho;
static struct proc_dir_entry *proc_dir = NULL;
static u32 ip_addr_onu=0;

#define PROCFS_MAC_FILTER_ADD		"mac_filter_add"
#define PROCFS_MAC_FILTER_COUNTER	"mac_filter_counter"
#define PROCFS_OSGI_DIR 			"osgi/mac_filter"

enum PROCFS_MAC_FILTER_OPER {
	PROCFS_MAC_FILTER_ENTRY_ADD = 0,
	PROCFS_MAC_FILTER_ENTRY_DEL,
	PROCFS_MAC_FILTER_ENTRY_DEL_BY_ACL,
};

static DEFINE_SPINLOCK(mac_filter_unres_lock);

typedef struct mac_filter_entry_s
{
	int aclIndex;
	unsigned char srcMac[ETH_ALEN];
	int hitCounter;
    struct list_head list;
} mac_filter_entry_t;

mac_filter_entry_t mac_filter_entry_list;

u32 get_ip_by_eth_name(const char *name)
{
	u32 ret = 0;
	struct net_device *nd = NULL;

	if ( (nd = dev_get_by_name(&init_net, name)) != NULL) {
		ret = inet_select_addr(nd, 0, 0);
		dev_put(nd); /* to release net_device */
	}
	return ret;
}
static bool ipAddress_match(__be32 addr, __be32 net) {
#if __DEBUG
	printk("Compare the ip: %pI4 , net:%pI4 \n", &addr, &net );
#endif
	return !((addr ^ net) & htonl(0xFFFFFFFFF));
}

/* Older kernel has different funcion definition. */
static unsigned int mac_filter_hook_func(
		const struct nf_hook_ops *ops,
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	char *ifname = (char *)(in->name);
	mac_filter_entry_t *node = NULL;
	struct ethhdr *mac_header = (struct ethhdr *)skb_mac_header(skb);
	struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);

	//check the entry is empty or not
	if(!list_empty(&mac_filter_entry_list.list)){ 
		list_for_each_entry(node, &(mac_filter_entry_list.list), list)
		{
			if(!memcmp(mac_header->h_source, node->srcMac, sizeof(node->srcMac))){		
#if __DEBUG	
				printk("======Found the mac filter in dev: %s=====\n", ifname);
				printk("src_mac: %pM ", mac_header->h_source);
				printk("dst_ip: %pI4 ", &ip_header->daddr);
				printk("\n");
#endif
				node->hitCounter++;
				if (ipAddress_match(ip_header->daddr, ip_addr_onu))
					return NF_ACCEPT;
				else
					return NF_DROP;
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


static int proc_mac_filter_del_entry(unsigned char* macAddr, int aclIdx)
{
	mac_filter_entry_t *node = NULL, *tmp = NULL; 
	list_for_each_entry_safe(node, tmp, &(mac_filter_entry_list.list), list)
	{
		if ( aclIdx == 0 ) {	
			if(!memcmp(macAddr, node->srcMac, sizeof(node->srcMac))){
				list_del(&node->list);
				kfree(node);
				return 0;
			}
		}
		else {
			if(aclIdx == node->aclIndex){
				list_del(&node->list);
				kfree(node);
				return 0;
			}			
		}
	}
}


static int proc_mac_filter_add_entry(unsigned char* macAddr, int aclIdx)
{
	mac_filter_entry_t *pMacFilterEntry;
	mac_filter_entry_t *node = NULL; 

	pMacFilterEntry = kmalloc(sizeof(mac_filter_entry_t), GFP_ATOMIC);
	if(!pMacFilterEntry)
		return -1;
	memset(pMacFilterEntry, 0, sizeof(mac_filter_entry_t));

	//Found the entry exist or not
	list_for_each_entry(node, &(mac_filter_entry_list.list), list)
	{
		if(!memcmp(macAddr, node->srcMac, sizeof(node->srcMac))){
			return 0;
		}
	}

	memcpy(pMacFilterEntry->srcMac, macAddr, sizeof(pMacFilterEntry->srcMac));
	pMacFilterEntry->aclIndex = aclIdx;
	spin_lock(&mac_filter_unres_lock);
	list_add_tail(&(pMacFilterEntry->list),&(mac_filter_entry_list.list));
	spin_unlock(&mac_filter_unres_lock);
	return 0;
}

static int proc_mac_filter_add_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[32] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	int i=0, tmp=0, ret=0, mode=0, aclIdx=0;
	unsigned char mac[ETH_ALEN];

	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		strptr = tmpbuf;
		
		/* 0 -> add, 1 ->delete*/
		tokptr = strsep(&strptr, " ");
		if (!tokptr) return -EFAULT;
		mode = simple_strtol(tokptr, NULL, 0);
		
		/* Mac address */
		if ( mode == PROCFS_MAC_FILTER_ENTRY_ADD )
		{
			tokptr = strsep(&strptr, " ");			
			if (!tokptr) return -EFAULT;
			aclIdx = simple_strtol(tokptr, NULL, 0);

			memset(mac, 0 , sizeof(mac));
			for (i=0; i<ETH_ALEN; i++) {
				tokptr = strsep(&strptr, ":");
				if (!tokptr) return -EFAULT;
				sscanf(tokptr, "%x", &tmp);
				mac[i] = tmp;
			}		
		}
		else if (mode == PROCFS_MAC_FILTER_ENTRY_DEL) {
			memset(mac, 0 , sizeof(mac));
			for (i=0; i<ETH_ALEN; i++) {
				tokptr = strsep(&strptr, ":");
				if (!tokptr) return -EFAULT;
				sscanf(tokptr, "%x", &tmp);
				mac[i] = tmp;
			}				
		}
		else if (mode == PROCFS_MAC_FILTER_ENTRY_DEL_BY_ACL)
		{
			tokptr = strsep(&strptr, " ");
			if (!tokptr) return -EFAULT;
			aclIdx = simple_strtol(tokptr, NULL, 0);			
		}
		
		if (mode == PROCFS_MAC_FILTER_ENTRY_ADD) 
			ret = proc_mac_filter_add_entry(mac, aclIdx);
		else if (mode == PROCFS_MAC_FILTER_ENTRY_DEL)
			ret = proc_mac_filter_del_entry(mac, 0);
		else if (mode == PROCFS_MAC_FILTER_ENTRY_DEL_BY_ACL)
			ret = proc_mac_filter_del_entry(mac, aclIdx);
		else
			printk("%s: mode=%d is not support!\n", __FUNCTION__, mode);
			
	}
	//update br0 ip address
	ip_addr_onu = get_ip_by_eth_name(LOCAL_IN_INTERFACE);	
	return count;
}


static int proc_mac_filter_read(struct seq_file *seq, void *v)
{
	mac_filter_entry_t *node = NULL; 

	list_for_each_entry(node, &(mac_filter_entry_list.list), list)
	{
 		printk("aclIndex=%d mac_addr=%pM\n", node->aclIndex, node->srcMac);
	}
	return 0;
}

static int proc_mac_filter_add_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_mac_filter_read, inode->i_private);
}

static struct mfc_cache *mac_filter_seq_idx(loff_t pos)
{
	mac_filter_entry_t *node = NULL;
	list_for_each_entry(node, &(mac_filter_entry_list.list), list) 
	        if (pos-- == 0)
                	return node;
    	return NULL;
}

static void *mac_filter_seq_start(struct seq_file *s, loff_t *pos) 
{ 
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos);		 
#endif
	return *pos ? mac_filter_seq_idx(*pos - 1)	: SEQ_START_TOKEN;
}
static void *mac_filter_seq_next(struct seq_file *s, void *v, loff_t *pos) 
{ 
	++*pos;
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos); 	 
#endif
	return mac_filter_seq_start(s, pos);
} 

static void mac_filter_seq_stop(struct seq_file *s, void *v) 
{ 
#if __DEBUG
	printk("[%s:%d]  \n", __FUNCTION__, __LINE__);
#endif	
} 

static int mac_filter_seq_show(struct seq_file *seq, void *v) 
{ 
	int n;
	
	if (v == SEQ_START_TOKEN) {
				 seq_puts(seq,  "id mac\n");
	} 
	else {
		mac_filter_entry_t *node = v;
		seq_printf(seq, "%pM %d\n", node->srcMac, node->hitCounter);
		node->hitCounter = 0; //read then clear
	}
	
	return 0;
}

static struct seq_operations mac_filter_counter_ops = { 
	.start			= mac_filter_seq_start,
	.next			= mac_filter_seq_next,
	.stop			= mac_filter_seq_stop,
	.show			= mac_filter_seq_show 
};

static int mac_filter_counter_open(struct inode *inode, struct file *file) 
{ 
	return seq_open(file, &mac_filter_counter_ops);
};

static struct file_operations mac_filter_counter_fops = { 
	.owner 			= THIS_MODULE,
	.open			= mac_filter_counter_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= seq_release 
};

static const struct file_operations mac_filter_add_fops = {
    .owner          = THIS_MODULE,
    .open           = proc_mac_filter_add_open,
    .read           = seq_read,
    .write          = proc_mac_filter_add_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static void mac_filter_entry_list_init(void)
{
    INIT_LIST_HEAD(&mac_filter_entry_list.list);
}


static int __init init_main(void)
{
	struct proc_dir_entry *entry = NULL;
	
	proc_dir = proc_mkdir(PROCFS_OSGI_DIR, NULL);
	if ( proc_dir == NULL){
		printk("%s: Register to /proc/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR);
	}

	entry = proc_create_data(PROCFS_MAC_FILTER_ADD, 0644, proc_dir, &mac_filter_add_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR, PROCFS_MAC_FILTER_ADD);
	}

	entry = proc_create_data(PROCFS_MAC_FILTER_COUNTER, 0444, proc_dir, &mac_filter_counter_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR, PROCFS_MAC_FILTER_COUNTER);
	}

	mac_filter_entry_list_init();
	nfho.hook = 	mac_filter_hook_func;
	nfho.hooknum = 	NF_INET_PRE_ROUTING;
	nfho.pf = 	NFPROTO_IPV4;
	nfho.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&nfho);
	printk("%s: Successfully inserted a hook into kernel\n", __FUNCTION__);
	return 0;
}

static void __exit cleanup_main(void)
{
	mac_filter_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(mac_filter_entry_list.list), list)
	{
        list_del(&node->list);
        kfree(node);		
	}

	remove_proc_entry(PROCFS_MAC_FILTER_ADD, proc_dir);
	remove_proc_entry(PROCFS_MAC_FILTER_COUNTER, proc_dir);
	proc_remove(proc_dir);
	
	nf_unregister_hook(&nfho);
	
	printk("Successfully unloaded the hook\n");
}

late_initcall(init_main);

module_exit(cleanup_main);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

