#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_tuple.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/hashtable.h>
#include <linux/netfilter/nf_traffic_detail_process.h>
#include <linux/spinlock.h>

#ifdef DEBUG
int tf_debug_level = 1;
#endif

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Realtek");
MODULE_DESCRIPTION("TrafficDetailProcessService");
MODULE_VERSION("1.0");

/******************* test data  ******************/
#if 0
int DEFAULT_PORT_NUM = 80;
char remoteAddress[16] = "";

char HOST_MAC[6] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

char HOST_MAC2[6] = { 0x08, 0x00, 0x27, 0x5d, 0x3a, 0x04 };

char methodList[64] = "GET,HEAD,POST,PUT,DELETE,TRACE,CONNECT";

char statusList[64] = "100,101,200,201,202,203,204,205,206";
char HEADLIST_TEST[] = "Content-Type,Connection,Host";
#endif
/***********************************/

#define method_max_len  16      //CONNECT, 7 Bypte
#define status_max_len  3       //"xxx" ,3 Bypte

struct store_list_head_t store_list_head;
static DEFINE_MUTEX(tf_rules_list_mutex);

static void dump_rule(traffice_process_rule * rule);

static inline void tf_rules_lock(void)
{
    mutex_lock(&tf_rules_list_mutex);
}

static inline void tf_rules_unlock(void)
{
    mutex_unlock(&tf_rules_list_mutex);
}

static void tf_rules_add(traffice_process_rule * rule)
{

    tf_rules_lock();
    INIT_LIST_HEAD(&rule->list);
    list_add_tail_rcu(&rule->list, &(store_list_head.list));
    atomic_inc(&(store_list_head.n));
    tf_rules_unlock();

    return;
}

#if 0
static void tf_rules_del(traffice_process_rule * rule)
{
    DEBUG_PRINT("---------------\n");
    dump_rule(rule);
    tf_rules_lock();
    atomic_dec(&(store_list_head.n));
    list_del_rcu(&rule->list);
    tf_rules_unlock();
    synchronize_rcu();
    kfree(rule);
}
#endif
/*
 * 
 *  IP_CT_DIR_ORIGINAL = 0
    IP_CT_DIR_REPLY =1
  */
/* 打印点分制ip地址 */
#define printk_ip(info, be32_addr) \
    printk("%s %d.%d.%d.%d\n", \
    info, \
    ((unsigned char *)&(be32_addr))[0], \
    ((unsigned char *)&(be32_addr))[1], \
    ((unsigned char *)&(be32_addr))[2], \
    ((unsigned char *)&(be32_addr))[3])

#if 0
static void tf_dump_mem(const char *p, int len)
{
    int i;
    if (tf_debug_level < 2) {
        return;
    }
    if (p) {
        printk("data:\n");
        for (i = 0; i < len; i++) {
            char c = *(char *) (p + i);
            printk("%c", c);
        }
        printk("\n");
        printk("---------------------------------------------------\n");
    }
}
#endif
/***********hash table ******************/
int HASH_TBL_MAX_SIZE = 10240;
int hash_tbl_current_size = 0;
static DEFINE_HASHTABLE(tf_htable, 8);
typedef struct tf_data_hash {
    unsigned long key;
    char *osgi_data_p;
    unsigned long time;         //jiffy
    char bundlename[64];
    unsigned long rule_time;    //jiffy
    struct hlist_node node;
} TF_DATA_HASH, *TF_DATA_HASH_P;

extern unsigned long volatile jiffies;
DEFINE_SPINLOCK(tf_hash_lock);

static TF_DATA_HASH_P
tf_hash_find_get(tf_process_task * tf_task, unsigned long rule_t)
{
    unsigned long flags;
    TF_DATA_HASH_P obj = NULL;
    TF_DATA_HASH_P tf_hash_node = NULL;
    unsigned long key = (unsigned long) tf_task->ct;
    spin_lock_irqsave(&tf_hash_lock, flags);
    hash_for_each_possible(tf_htable, obj, node, key) {
        if ((obj->key == (unsigned long) tf_task->ct)
            && (obj->rule_time == rule_t)) {
            hash_del(&obj->node);
			hash_tbl_current_size--;
            //     DBG_DYN_PRINT("[%s]key=%lu => %s\n", __func__, obj->key, obj->bundlename);
            tf_hash_node = obj;
            goto exit_hash;
        }
    }
  exit_hash:
    spin_unlock_irqrestore(&tf_hash_lock, flags);
    return tf_hash_node;
}

static int tf_hash_size_check(void){
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&tf_hash_lock, flags);
	if(hash_tbl_current_size >= HASH_TBL_MAX_SIZE){
		ret = -1;
	}
	spin_unlock_irqrestore(&tf_hash_lock, flags);
	if (ret == -1){
		if(printk_ratelimit())
                printk(KERN_ERR "[tf_report_filter_result] Reach HASH_TBL_MAX_SIZE(%d)!\n", HASH_TBL_MAX_SIZE);
	}
	return ret;
}

static int
tf_hash_add(tf_process_task * tf_task, char *bname,
            unsigned long rule_time)
{
    TF_DATA_HASH_P task_hash_p;
    unsigned long flags;

	int MAX_LOOP = 4096;
	int loop = 0;
    TF_DATA_HASH_P request_data = NULL;

	if(tf_hash_size_check() != 0){
		return -1;
	}
	
	
    task_hash_p = kmalloc(sizeof(TF_DATA_HASH), GFP_ATOMIC);

    DBG_DYN_PRINT("[%s][%d]add tf_task into hashtbl!\n", __FUNCTION__,
                  __LINE__);

    if (task_hash_p == NULL) {
        printk("[%s]kmalloc fail!\n", __FUNCTION__);
        return -1;
    }

    task_hash_p->osgi_data_p = tf_task->osgi_data;
    tf_task->osgi_data = NULL;
    task_hash_p->rule_time = rule_time;
    task_hash_p->key = (long) tf_task->ct;
    task_hash_p->time = jiffies;
    strcpy(task_hash_p->bundlename, bname);
#if 1 // every connection + the same rule, should add 1 queue request data.
	for(loop=0; loop < MAX_LOOP; loop++){
	request_data = NULL;
    request_data = tf_hash_find_get(tf_task, rule_time);
		if(request_data != NULL){
			kfree(request_data->osgi_data_p);
			kfree(request_data);
		}else{
			break;
		}
	}
#endif
    INIT_HLIST_NODE(&task_hash_p->node);
    spin_lock_irqsave(&tf_hash_lock, flags);
	hash_tbl_current_size ++;
    hash_add(tf_htable, &task_hash_p->node, task_hash_p->key);
    spin_unlock_irqrestore(&tf_hash_lock, flags);
    return 0;
}

#if 0
static void tf_hash_del(TF_DATA_HASH_P hash_member)
{
    unsigned long flags;
    spin_lock_irqsave(&tf_hash_lock, flags);
    hash_del(&hash_member->node);
    kfree(hash_member);
	hash_tbl_current_size--;
    spin_unlock_irqrestore(&tf_hash_lock, flags);

}
#endif
static void tf_dump_hash_table(void)
{
    TF_DATA_HASH_P obj = NULL;
    struct hlist_node *tmp;
    int i;
    unsigned long flags;
    printk("%s\n", __FUNCTION__);
    spin_lock_irqsave(&tf_hash_lock, flags);

    hash_for_each_safe(tf_htable, i, tmp, obj, node) {
        printk("%lu: %lu: %s\n", obj->time, obj->key, obj->osgi_data_p);
    }

    spin_unlock_irqrestore(&tf_hash_lock, flags);
}

static void tf_hashtbl_clear(void)
{
    TF_DATA_HASH_P obj = NULL;
    struct hlist_node *tmp;
    int i;
    unsigned long flags;

    spin_lock_irqsave(&tf_hash_lock, flags);

    hash_for_each_safe(tf_htable, i, tmp, obj, node) {

        hash_del(&obj->node);
        if (obj->osgi_data_p) {
            kfree(obj->osgi_data_p);
        }
        kfree(obj);
		hash_tbl_current_size--;
    }
    spin_unlock_irqrestore(&tf_hash_lock, flags);
}



#define HASHTBL_ENTRY_EXPIRES_SEC     240
struct timer_list hashtbl_timeout_timer;
static void tf_hashtbl_maintenance(unsigned long args)
{
    TF_DATA_HASH_P obj = NULL;
    struct hlist_node *tmp;
    int i;
    unsigned long flags;

    spin_lock_irqsave(&tf_hash_lock, flags);

    hash_for_each_safe(tf_htable, i, tmp, obj, node) {
        DBG_DYN_PRINT("%lu: %lu: %s: expires:%lu, jiffies:%lu\n",
                      obj->time, obj->key, obj->osgi_data_p,
                      obj->time + (HASHTBL_ENTRY_EXPIRES_SEC * HZ),
                      jiffies);

        if (time_after(jiffies, obj->time + (HASHTBL_ENTRY_EXPIRES_SEC * HZ))) {        //obj over HASHTBL_ENTRY_EXPIRES_SEC sec without response
            hash_del(&obj->node);
			hash_tbl_current_size--;
            if (obj->osgi_data_p) {
                kfree(obj->osgi_data_p);
                obj->osgi_data_p = NULL;
            }
            kfree(obj);
        }
    }

    spin_unlock_irqrestore(&tf_hash_lock, flags);

    mod_timer(&hashtbl_timeout_timer,
              jiffies + (HASHTBL_ENTRY_EXPIRES_SEC * HZ));
}

static int tf_init_timer(void)
{

    // Timer init
    init_timer(&hashtbl_timeout_timer);
    // 定義 timer 所執行之函式
    hashtbl_timeout_timer.function = &tf_hashtbl_maintenance;
    // 定義 timer 傳入函式之 Data
    hashtbl_timeout_timer.data = (0);
    // 定義 timer Delay 時間

    hashtbl_timeout_timer.expires = jiffies + (HASHTBL_ENTRY_EXPIRES_SEC * HZ); //300 second
    // 啟動 Timer
    add_timer(&hashtbl_timeout_timer);

    return 0;
}

#ifdef __TEST_HASHTBL
void tf_test_tbl(void)
{
    tf_process_task *tf_task;
    tf_task = kmalloc(sizeof(tf_process_task), GFP_KERNEL);
    tf_task->osgi_data = kmalloc(512, GFP_KERNEL);
    strcpy(tf_task->osgi_data, "tf_test_tbl");
    tf_task->ct = 100000000;
    tf_task->osgi_dlen = 512;
    tf_hash_add(tf_task, "alantest", jiffies);

}
#endif

/***********************************************
 *  trafficdetailprocess main
**********************************************/
static int filter_remote_address(tf_process_task * tf_task, __be32 r_addr)
{
    int ret = TF_PROCESS_SKIP;
    if (r_addr == 0) {
        //No Raddress, filter all ip
        return TF_PROCESS_NEED;
    }
    switch (tf_task->direction) {
        case IP_CT_DIR_ORIGINAL:
            if (tf_task->iph->daddr == r_addr) {
                ret = TF_PROCESS_NEED;
            }
            break;
        case IP_CT_DIR_REPLY:
            if (tf_task->iph->saddr == r_addr) {
                ret = TF_PROCESS_NEED;
            }
            break;
    }
//    DEBUG_PRINT("tf_task->iph->saddr = %u, tf_task->iph->daddr=%u , r_addr=%u, direction=%d\n",
//         tf_task->iph->saddr, tf_task->iph->daddr, r_addr, tf_task->direction);
    return ret;
}

/*
 *  Check 1. remotePort, 
 *        2. direction
 * 
 */

static int
filter_port_num(tf_process_task * tf_task, int port_num, int direction)
{
//    if ((tf_task->direction == direction) || direction == CONN_DIR_ALL) {
    /* direction =IP_CT_DIR_REPLY (1), Server ---> Client */
    if ((tf_task->direction == IP_CT_DIR_REPLY)
        && (ntohs(tf_task->tcph->source) == port_num)) {
        return TF_PROCESS_NEED;
    } else if ((tf_task->direction == IP_CT_DIR_ORIGINAL)
               && (ntohs(tf_task->tcph->dest) == port_num)) {
        return TF_PROCESS_NEED;
    }
    //  }
    return TF_PROCESS_SKIP;
}

static int filter_host_mac(tf_process_task * tf_task, char *host_mac)
{
    uint16_t *mac_p = (uint16_t *) host_mac;
#if 0                           // DEBUG
    u8 *d_mac_addr = eth_hdr(tf_task->skb)->h_dest;
    u8 *s_mac_addr = eth_hdr(tf_task->skb)->h_source;
    if (tf_debug_level > 0) {
        DEBUG_PRINT("[%s] %pM , %pM . target=%pM , IP_CT_DIR_REPLY %d, IP_CT_DIR_ORIGINAL %d tf_task->direction %d\n", __func__,
                    s_mac_addr, d_mac_addr, host_mac, IP_CT_DIR_REPLY, IP_CT_DIR_ORIGINAL, tf_task->direction);
    }
#endif
    if (*mac_p == 0 && *(mac_p + 1) == 0 && *(mac_p + 2) == 0) {
        return TF_PROCESS_NEED;
    }
#if 0
    if ((tf_task->direction == IP_CT_DIR_REPLY)
        && (ether_addr_equal(eth_hdr(tf_task->skb)->h_dest, host_mac))) {
        return TF_PROCESS_NEED;
    } else if ((tf_task->direction == IP_CT_DIR_ORIGINAL)
               &&
               (ether_addr_equal
                (eth_hdr(tf_task->skb)->h_source, host_mac))) {
        return TF_PROCESS_NEED;
    }
#else
	if (tf_task->direction == IP_CT_DIR_REPLY){
		return TF_PROCESS_NEED;
	}
	else if ((tf_task->direction == IP_CT_DIR_ORIGINAL)
        && (ether_addr_equal(eth_hdr(tf_task->skb)->h_source, host_mac))) {
        return TF_PROCESS_NEED;
    }
#endif
    return TF_PROCESS_SKIP;
}

static void dump_skb(struct sk_buff *skb, int count)
{
    struct iphdr *iph;          /* IPv4 header */
    struct tcphdr *tcph;        /* TCP header */
    u16 sport, dport;           /* Source and destination ports */
    u32 saddr, daddr;           /* Source and destination addresses */
    int print_count = 0;
    unsigned char *user_data;   /* TCP data begin pointer */
    unsigned char *tail;        /* TCP data end pointer */
    unsigned char *it;          /* TCP data iterator */
    /* Calculate pointers for begin and end of TCP packet data */

    if (tf_debug_level < 3) {
        return;
    }

    iph = ip_hdr(skb);          /* get IP header */

    tcph = tcp_hdr(skb);        /* get TCP header */

    saddr = ntohl(iph->saddr);
    daddr = ntohl(iph->daddr);
    sport = ntohs(tcph->source);
    dport = ntohs(tcph->dest);

    user_data =
        (unsigned char *) ((unsigned char *) tcph + (tcph->doff * 4));
    tail = skb_tail_pointer(skb);

    /* ----- Print all needed information from received TCP packet ------ */
#if 0
    /* Show only HTTP packets */
    if (user_data[0] != 'H' || user_data[1] != 'T' || user_data[2] != 'T'
        || user_data[3] != 'P') {
        return;
    }
#endif
    /* Print packet route */
    printk
        ("[TRAFFIC_DETAIL_PROCESS Enter]print_tcp: %pI4h:%d -> %pI4h:%d\n",
         &saddr, sport, &daddr, dport);

    if (count == 0) {
        count = 2048;
    }
#if 1
    /* Print TCP packet data (payload) */
    printk("----TCP DATA ------------------:\n");
    for (it = user_data; it != tail; ++it) {
        char c = *(char *) it;
        printk("%c", c);
        print_count++;
        if (print_count > count) {      //only dump 32 B data
            break;
        }
    }
    printk("\n");
    printk("---------------------------------------------------\n");
    printk("\n");
#endif
}

static void dump_tf_http_info(tf_process_task * tf_task)
{
#if DEBUG
    //  int i;

    if (tf_debug_level >= 3) {
        if (tf_task->request_h) {
            printk
                ("----------------------- REQUEST Summary ---------------------\n");
            printk("Method: %.*s\n",
                   tf_task->request_h->RequestMethodLen,
                   tf_task->request_h->RequestMethod);
            printk("HTTPVersion: %.*s\n",
                   tf_task->request_h->HTTPVersionLen,
                   tf_task->request_h->HTTPVersion);
            printk
                ("----------------------- REQUEST FINISH-------------------------\n");
            printk("\n");
        }

        if (tf_task->response_h) {
            printk("***************  RESPONSE Summary*****************\n");
            printk("Status: %.*s\n",
                   tf_task->response_h->ResponseStatusLen,
                   tf_task->response_h->ResponseStatus);
            printk("HTTPVersion: %.*s\n",
                   tf_task->response_h->HTTPVersionLen,
                   tf_task->response_h->HTTPVersion);
            printk
                ("***************  RESPONSE FINISH********************************\n");
        }
#if 0
        if (tf_task->osgi_data) {
            printk("osgi_data:\n");
            for (i = 0; i < tf_task->osgi_dlen; i++) {
                char c = *(char *) (tf_task->osgi_data + i);
                printk("%c", c);
            }
            printk("\n");
            printk
                ("---------------------------------------------------\n");
        }
#endif
    }
#endif
}

static void tf_report_msg_2(char *report_data, char *bundlename)
{
    if (report_data && bundlename) {
#if 0
        DBG_DYN_CPRINT("[tf_report_msg_2]bundlename=%s, osgi_data=\n%s\n",
                       bundlename, report_data);
#endif
        tf_nl_send_msg(report_data, bundlename);
    } else {
        DBG_DYN_PRINT
            ("[tf_nl_send_msg]Failed!! bundlename=%s, osgi_data=\n%s\n",
             bundlename, report_data);
    }
}


#define TF_REPORT_BUF_LEN   2048
static void
tf_report_filter_result(tf_process_task * tf_task, unsigned long rule_time)
{
    char *tf_buf = NULL;
    TF_DATA_HASH_P request_data = NULL;
	request_data = tf_hash_find_get(tf_task, rule_time);
	
    if (request_data  != NULL) {


        if ((strlen(request_data->osgi_data_p) +
             strlen(tf_task->osgi_data) >= (TF_REPORT_BUF_LEN - 1))) {
            printk("tf_report_filter_result error! data len(%d) > %d\n",
                   strlen(request_data->osgi_data_p) +
                   strlen(tf_task->osgi_data), TF_REPORT_BUF_LEN);
            return;
        }

        tf_buf = kmalloc(TF_REPORT_BUF_LEN, GFP_ATOMIC);
        if (tf_buf == NULL) {
            if (printk_ratelimit())
                printk(KERN_ERR
                       "[tf_report_filter_result] kmalloc failed!\n");
            goto free_data;
        }
        memset(tf_buf, 0, TF_REPORT_BUF_LEN);

        if (request_data->osgi_data_p) {
            // DBG_DYN_CPRINT("[tf_report_filter_result][REQUEST DATA]\n%s\n", request_data->osgi_data_p);
            strncpy(tf_buf, request_data->osgi_data_p,
                    TF_REPORT_BUF_LEN - 1);
        }

        if (tf_task->osgi_data) {
            //    DBG_DYN_CPRINT("[tf_report_filter_result][Response DATA]\n%s\n", tf_task->osgi_data);
            strncat(tf_buf, tf_task->osgi_data,
                    TF_REPORT_BUF_LEN - 1 -
                    strlen(request_data->osgi_data_p));
        }

        if (strlen(tf_buf) > 0) {
            tf_report_msg_2(tf_buf, request_data->bundlename);
        }

        kfree(tf_buf);
      free_data:
        if (request_data->osgi_data_p) {
            kfree(request_data->osgi_data_p);
            request_data->osgi_data_p = NULL;
        }
        kfree(request_data);
        request_data = NULL;
    }

}

static void
tf_flush_hash_request(tf_process_task * tf_task, unsigned long rule_time)
{
    TF_DATA_HASH_P request_data = NULL;

    while ((request_data = tf_hash_find_get(tf_task, rule_time)) != NULL) {
        if (request_data->osgi_data_p) {
            kfree(request_data->osgi_data_p);
            request_data->osgi_data_p = NULL;

        }
        kfree(request_data);
        request_data = NULL;
    }
}

#define RULE_METHOD_STR_MAX 128
static int
filter_request_method(tf_process_task * tf_task, char *method_str)
{
    char rh_method[method_max_len + 1];
    char *const delim = ",";
    char *token, *cur;
    char tmp_buf[RULE_METHOD_STR_MAX];

    if (method_str[0] == '\0') {
        return TF_PROCESS_NEED;
    }

    strncpy(tmp_buf, method_str, sizeof(tmp_buf));
    tmp_buf[RULE_METHOD_STR_MAX - 1] = '\0';
    cur = tmp_buf;

    memset(rh_method, '\0', sizeof(rh_method));

    if (method_str[0] == '\0') {
        return TF_PROCESS_SKIP;
    }
    if ((tf_task->request_h)
        && (tf_task->request_h->RequestMethodLen > 0)
        && (tf_task->request_h->RequestMethodLen < method_max_len)) {
        memcpy(rh_method, tf_task->request_h->RequestMethod,
               tf_task->request_h->RequestMethodLen);

        while ((token = strsep(&cur, delim)) != NULL) {
//        DEBUG_PRINT("%s\n", strstrip(token));  
            if (strcmp(rh_method, strstrip(token)) == 0) {
                return TF_PROCESS_NEED;
            }
        }
    }
    return TF_PROCESS_SKIP;
}

static int
filter_response_status(tf_process_task * tf_task, char *status_str)
{
    char *const delim = ",";
    char *token, *cur;
    char *p;
    char tmp_buf[128];

    if (status_str[0] == '\0') {
        return TF_PROCESS_NEED;
    }


    strncpy(tmp_buf, status_str, sizeof(tmp_buf));
    cur = tmp_buf;
    if ((tf_task->response_h)
        && (tf_task->response_h->ResponseStatusLen > 0)
        && (tf_task->response_h->ResponseStatusLen <= status_max_len)) {
        strncpy(tf_task->r_status, tf_task->response_h->ResponseStatus,
                tf_task->response_h->ResponseStatusLen);

        while ((token = strsep(&cur, delim)) != NULL) {
            p = strstrip(token);


            if (strcmp(p, tf_task->r_status) == 0) {
                DBG_DYN_PRINT
                    ("[%s]Match. tf_task->r_status:%s, match:%s\n",
                     __func__, tf_task->r_status, p);
                return TF_PROCESS_NEED;
            }
        }
    }
    return TF_PROCESS_SKIP;
}

static int
filter_request_headerlist(tf_process_task * tf_task,
                          traffice_process_rule * tf_rule)
{
    int ret = TF_PROCESS_NEED;
    ret =
        h3_required_header_parse_by_headlist(tf_task,
                                             tf_rule->HTTP_headerlist,
                                             tf_rule->direction);
    return ret;
}

static int
filter_http_hdr(tf_process_task * tf_task, traffice_process_rule * tf_rule)
{
    RequestHeader *request_h = NULL;
    ResponseHeader *response_h = NULL;

    switch (tf_task->direction) {
        case IP_CT_DIR_ORIGINAL:       //Client Request
		if(tf_hash_size_check() != 0){
			return TF_PROCESS_SKIP;
		}
            request_h = h3_request_header_new();
            if (request_h == NULL)
                return TF_HTTP_PARSER_FAIL;

            if (h3_request_header_parse
                (request_h, tf_task->data, tf_task->data_len) != 0) {
                h3_request_header_free(request_h);
                request_h = NULL;
                return TF_HTTP_PARSER_FAIL;
            }
            tf_task->request_h = request_h;

            if (filter_request_method(tf_task, tf_rule->methodList) ==
                TF_PROCESS_SKIP) {
                return TF_PROCESS_SKIP;
            }

            if (filter_request_headerlist(tf_task, tf_rule) ==
                TF_PROCESS_SKIP) {
                return TF_PROCESS_SKIP;
            }

            /** add request data to hashtable and wait reponse status code***/
            if (tf_hash_add(tf_task, tf_rule->bundlename, tf_rule->time) !=
                0) {
                return TF_PROCESS_SKIP;
            }
            break;
        case IP_CT_DIR_REPLY:
            if ((tf_task->data_len > 4) && (memcmp((void *) tf_task->data, "HTTP/", 5) == 0)) { //HTTP HEADER Start from HTTP For Server response  
                response_h = h3_response_header_new();
                if (response_h == NULL)
                    return TF_HTTP_PARSER_FAIL;

                if (h3_response_header_parse
                    (response_h, tf_task->data, tf_task->data_len) != 0) {
                    h3_response_header_free(response_h);
                    response_h = NULL;
                    return TF_HTTP_PARSER_FAIL;
                }
                tf_task->response_h = response_h;
                if (filter_response_status(tf_task, tf_rule->statusList) ==
                    TF_PROCESS_SKIP) {

                    DBG_DYN_PRINT("[%s]StatusCode isn't match or wrong\n",
                                  __func__);
                    tf_flush_hash_request(tf_task, tf_rule->time);
                    return TF_PROCESS_SKIP;
                }
                if (filter_request_headerlist(tf_task, tf_rule) ==
                    TF_PROCESS_SKIP) {
                    DBG_DYN_PRINT
                        ("[%s]filter_request_headerlist failed!\n",
                         __func__);
                    tf_flush_hash_request(tf_task, tf_rule->time);
                    return TF_PROCESS_SKIP;
                }

                tf_report_filter_result(tf_task, tf_rule->time);

            }
            break;
        default:
            break;
    }

    return TF_HTTP_PARSER_OK;
}

static void filter_resource_free(tf_process_task * tf_task)
{
    if (tf_task->request_h) {
        DBG_DYN_PRINT
            ("[TF]---------------------REQUEST   EXIT----------------------------------\n");
        kfree(tf_task->request_h);
        tf_task->request_h = NULL;
    }
    if (tf_task->response_h) {
        DBG_DYN_PRINT
            ("[TF]---------------------RESPONSE  EXIT----------------------------------\n");
        kfree(tf_task->response_h);
        tf_task->response_h =NULL;
    }

    if (tf_task->osgi_data) {
        kfree(tf_task->osgi_data);
        tf_task->osgi_data = NULL;
    }

}

/*
 *   MAIN
 *  
 * 
 * 
 */
static void filter_process(tf_process_task * tf_task)
{
    traffice_process_rule *tf_rule;
    rcu_read_lock();
    list_for_each_entry_rcu(tf_rule, &(store_list_head.list), list) {


        if (filter_port_num
            (tf_task, tf_rule->remotePort,
             tf_rule->direction) == TF_PROCESS_SKIP) {
            continue;
        };

        if (filter_host_mac(tf_task, tf_rule->h_dest) == TF_PROCESS_SKIP) {
            continue;
        }

        if (filter_remote_address(tf_task, tf_rule->remoteAddress) ==
            TF_PROCESS_SKIP) {
            continue;
        };
#if DEBUG
        dump_skb(tf_task->skb, 256);
#endif
        if (filter_http_hdr(tf_task, tf_rule) > TF_PROCESS_SKIP) {
			/*Hit one rule and do something. only suport match the fist rule*/
            dump_tf_http_info(tf_task);
			goto exit_rcu;
        } else {
			
			filter_resource_free(tf_task);
#if 0
            if (tf_task->osgi_data) {
                kfree(tf_task->osgi_data);
                tf_task->osgi_data = NULL;
            }
#endif
        }
    }
exit_rcu:
    rcu_read_unlock();

    return;
}

/****************************************************************************
 * tf_task->data: SKB data
 * tf_task->data_len: SKB data len
 * 
 *****************************************************************************/
static int tf_task_init(struct sk_buff *skb, tf_process_task * tf_task,
                        int flag)
{
    char *tail;
    struct nf_conn *ct;
    enum ip_conntrack_info ctinfo;
    enum ip_conntrack_dir dir;

    tf_task->iph = ip_hdr(skb);

    if ((tf_task->iph->protocol != IPPROTO_TCP)
        && (tf_task->iph->protocol != IPPROTO_IP)) {
        return 0;
    }


    tf_task->tcph = (struct tcphdr *) (skb->data + tf_task->iph->ihl * 4);
    tf_task->data =
        (unsigned char *) ((unsigned char *) tf_task->tcph +
                           (tf_task->tcph->doff * 4));
    tail = skb_tail_pointer(skb);
    tf_task->data_len = tail - tf_task->data;
#if 0
    u16 sport, dport;           /* Source and destination ports */
    u32 saddr, daddr;           /* Source and destination addresses */
    saddr = ntohl(tf_task->iph->saddr);
    daddr = ntohl(tf_task->iph->daddr);
    sport = ntohs(tf_task->tcph->source);
    dport = ntohs(tf_task->tcph->dest);
    if (tf_debug_level > 0) {
        if ((sport == 80) || (dport == 80)) {
            ct = nf_ct_get(skb, &ctinfo);
            if (ct != NULL) {
                dir = CTINFO2DIR(ctinfo);
                tf_task->direction = dir;
                tf_task->ct = (void *) ct;
            } else {
                dir = -1;
            }
            if (flag == 1)
                printk
                    ("LOCAL_IN:%pI4h :%d -> %pI4h :%d ; direction:%d, datalen:%d\n",
                     &saddr, sport, &daddr, dport, dir, tf_task->data_len);
            else if (flag == 2)
                printk
                    ("LOCAL_OUT:%pI4h :%d -> %pI4h :%d ; direction:%d, datalen:%d\n",
                     &saddr, sport, &daddr, dport, dir, tf_task->data_len);
            else if (flag == 3)
                printk
                    ("FORWARD:%pI4h :%d -> %pI4h :%d ; direction:%d, datalen:%d\n",
                     &saddr, sport, &daddr, dport, dir, tf_task->data_len);
            else
                printk
                    ("UNKNOW:%pI4h :%d -> %pI4h :%d ; direction:%d, datalen:%d\n",
                     &saddr, sport, &daddr, dport, dir, tf_task->data_len);
        }
    }
#endif


    if (tf_task->data_len > 0) {        //has application data (with http header)
        tf_task->skb = skb;
        ct = nf_ct_get(skb, &ctinfo);
        if (ct == NULL) {
            DEBUG_PRINT("------------------CT is null---------------\n");
            return 0;
        }
        dir = CTINFO2DIR(ctinfo);
        tf_task->direction = dir;
        tf_task->ct = (void *) ct;


        return 1;
    }
    return 0;
}

/* NF_INET_LOCALIN_ROUTING */
static unsigned int
localin_routing_hook(const struct nf_hook_ops *ops,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn) (struct sk_buff *))
{
    tf_process_task tf_task;
    memset(&tf_task, 0, sizeof(tf_task));
    if (tf_task_init(skb, &tf_task, 1)) {
        filter_process(&tf_task);
        filter_resource_free(&tf_task);
    }
    return NF_ACCEPT;
}

/* NF_INET_LOCALOUT_ROUTING */
static unsigned int
localout_routing_hook(const struct nf_hook_ops *ops,
                      struct sk_buff *skb,
                      const struct net_device *in,
                      const struct net_device *out,
                      int (*okfn) (struct sk_buff *))
{
    tf_process_task tf_task;
    memset(&tf_task, 0, sizeof(tf_task));
    if (tf_task_init(skb, &tf_task, 2)) {
        filter_process(&tf_task);
        filter_resource_free(&tf_task);
    }
    return NF_ACCEPT;
}

/* NF_INET_forward_ROUTING */
static unsigned int
forward_routing_hook(const struct nf_hook_ops *ops,
                     struct sk_buff *skb,
                     const struct net_device *in,
                     const struct net_device *out,
                     int (*okfn) (struct sk_buff *))
{
    tf_process_task tf_task;
    memset(&tf_task, 0, sizeof(tf_task));
    if (tf_task_init(skb, &tf_task, 3)) {
        filter_process(&tf_task);
        filter_resource_free(&tf_task);
    }
    return NF_ACCEPT;
}


struct nf_hook_ops localin_routing_ops = {
    .hook = localin_routing_hook,
    .pf = NFPROTO_IPV4,
    .hooknum = NF_INET_LOCAL_IN,
    .priority = NF_IP_PRI_FIRST
};

struct nf_hook_ops localout_routing_ops = {
    .hook = localout_routing_hook,
    .pf = NFPROTO_IPV4,
    .hooknum = NF_INET_LOCAL_OUT,
    .priority = NF_IP_PRI_LAST - 200
};

struct nf_hook_ops forward_routing_ops = {
    .hook = forward_routing_hook,
    .pf = NFPROTO_IPV4,
    .hooknum = NF_INET_FORWARD,
    .priority = NF_IP_PRI_FIRST
};


/************************************************
 * Rule part
 *************************************************/


#ifdef __TEST_RULES
static void init_test_rule(void)
{
    traffice_process_rule *r;
    DEBUG_PRINT("=== init_test_rule t ====\n");
    r = kmalloc(sizeof(traffice_process_rule), GFP_KERNEL);

    r->direction = CONN_DIR_ALL;
    r->remotePort = 80;
    r->remoteAddress = 0;       //in_aton(remoteAddress);
    memcpy(r->h_dest, HOST_MAC, 6);
    strncpy(r->methodList, methodList, sizeof(r->methodList));
    strncpy(r->statusList, statusList, sizeof(r->statusList));
    strncpy(r->HTTP_headerlist, HEADLIST_TEST, sizeof(r->statusList));
    strcpy(r->bundlename, "TEST1");
	r->index = 0;
    r->time = jiffies;
    tf_rules_add(r);
#if 0
    /* add 2 */
    DEBUG_PRINT("=== init_test_rule t ====\n");
    r = kmalloc(sizeof(traffice_process_rule), GFP_KERNEL);

    r->direction = CONN_DIR_DOWN;
    r->remotePort = 80;
    r->remoteAddress = in_aton(remoteAddress);
    memcpy(r->h_dest, HOST_MAC2, 6);
    strncpy(r->methodList, methodList, sizeof(r->methodList));
    strncpy(r->statusList, statusList, sizeof(r->statusList));
    r->HTTP_headerlist = HEADLIST_TEST;
    strcpy(r->bundlename, "TEST2");
	r->index = 1;
    tf_rules_add(r);
#endif
}

#endif
void tf_rule_reclaim(struct rcu_head *rp)
{
    struct TRAFFCIE_PROCESS_RULE *fp =
        container_of(rp, struct TRAFFCIE_PROCESS_RULE, rcu);
    kfree(fp);
}

static void release_all_rules(void)
{
    traffice_process_rule *tf_rule;
    tf_rules_lock();

    list_for_each_entry_rcu(tf_rule, &store_list_head.list, list) {
        atomic_dec(&(store_list_head.n));
        list_del_rcu(&tf_rule->list);
        call_rcu(&tf_rule->rcu, tf_rule_reclaim);
    }
    tf_rules_unlock();
}


static void dump_rule(traffice_process_rule * rule)
{
#if DEBUG
    char ip_str[16];

    snprintf(ip_str, 16, "%pI4", &rule->remoteAddress);

    DBG_DYN_PRINT
        ("RemoteAddress:%s RemotePort:%d Direction:%d HostMAC:%pM MethodList:%s statuscodeList:%s HeaderList:%s BundleName:%s Index:%d\n",
         ip_str, rule->remotePort, rule->direction, rule->h_dest,
         rule->methodList, rule->statusList, rule->HTTP_headerlist,
         rule->bundlename, rule->index);
#endif
}

static int tf_del_rule_by_index(int i)
{
    traffice_process_rule *tf_rule;
    int cnt = 0;

    tf_rules_lock();
    list_for_each_entry_rcu(tf_rule, &store_list_head.list, list) {
        if (tf_rule->index == i) {
            list_del_rcu(&tf_rule->list);
            atomic_dec(&(store_list_head.n));
            call_rcu(&tf_rule->rcu, tf_rule_reclaim);
            break;
        }
        cnt++;
    }

    tf_rules_unlock();
    return 0;
}

static void tf_del_rule_by_user(int index)
{
//     DBG_DYN_PRINT("index:%d, atomic_read(&store_list_head.n)=%d\n", index,
//                   atomic_read(&store_list_head.n));

    if (index >= TRAFFCIE_PROCESS_RULE_MAX) {
        printk("%s: del all rules!\n", __func__);
        release_all_rules();
        return;
    }

    if (index >= atomic_read(&store_list_head.n)) {
        return;
    }
    tf_del_rule_by_index(index);
}




/*
 * PROC
 */
#define PROCFS_OSGI_DIR         "osgi_traffic_process"
#define PROC_NAME   "traffice_process_rule"

static void fill_mac(char *mac_str, traffice_process_rule * cur_node)
{
    if (strlen(mac_str) < 6) {
        memset(cur_node->h_dest, '\0', ETH_ALEN);
    } else {
        sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &cur_node->h_dest[0], &cur_node->h_dest[1],
               &cur_node->h_dest[2], &cur_node->h_dest[3],
               &cur_node->h_dest[4], &cur_node->h_dest[5]);
    }
}

static void
parser_input(int count, char *field, traffice_process_rule * cur_node)
{
    char fieldvalue[256];
    char tmp_buf[320] = { '\0' };
    char *const delim = ":";
    char *const delim1 = " ";
    char *token;
    char *after;
    char *p;

    strncpy(tmp_buf, field, sizeof(tmp_buf));
    p = tmp_buf;
    token = strsep(&p, delim);
    //      DBG_DYN_PRINT("fieldname: %s\n", token);
    token = strsep(&p, delim1);
    strncpy(fieldvalue, token, sizeof(fieldvalue));
//       DBG_DYN_PRINT("fieldvalue: %s\n", fieldvalue);

    switch (count) {
        case 0:
            cur_node->remoteAddress = in_aton(fieldvalue);
            break;
        case 1:
            cur_node->remotePort = simple_strtoul(fieldvalue, &after, 10);
            break;
        case 2:
            cur_node->direction =
                (uint8_t) simple_strtoul(fieldvalue, &after, 10);
            break;
        case 3:
            fill_mac(fieldvalue, cur_node);
            break;
        case 4:
            strncpy(cur_node->methodList, fieldvalue, 64);
            break;
        case 5:
            strncpy(cur_node->statusList, fieldvalue, HEADER_LIST_MAX_LEN);
            break;
        case 6:
            strncpy(cur_node->HTTP_headerlist, fieldvalue,
                    HEADER_LIST_MAX_LEN);
            break;
        case 7:
            if (fieldvalue[strlen(fieldvalue) - 1] == '\n')
                fieldvalue[strlen(fieldvalue) - 1] = '\0';

            strncpy(cur_node->bundlename, fieldvalue, 64);
            break;
		case 8:
            cur_node->index = simple_strtoul(fieldvalue, &after, 10);
            break;
        default:
            break;
    }
}





static void tf_debug_option(int debug_value)
{
    tf_debug_level = debug_value - 10000;
 //   printk("%s:DEBUG OPTION:tf_debug_level = %d\n", __FUNCTION__, tf_debug_level);

    if (tf_debug_level >= 10000) {
        tf_dump_hash_table();
        tf_debug_level -= 10000;
    }

}



ssize_t
proc_traffic_rule_write(struct file *filp, const char __user * buff,
                        size_t count, loff_t * f_pos)
{
    traffice_process_rule *cur_node = NULL;
    char tmp_buf[512] = { '\0' };
    char *const delim = " ";
    char *token;
    char *p;

    int index;
//      int ret;
    char *after;
    int para_idx = 0;

    DBG_DYN_PRINT("Write function is invoked\n");

    if (copy_from_user(tmp_buf, buff, count)) {
        return -EFAULT;
    }

    p = tmp_buf;

    if ((token = strsep(&p, delim)) != NULL) {

        if (strchr(token, ':') != NULL) {

            cur_node = (traffice_process_rule *)
                kmalloc(sizeof(*cur_node), GFP_KERNEL);
            if (!cur_node)
                return -ENOMEM;

            cur_node->time = jiffies;
            parser_input(para_idx, token, cur_node);
            while (((token = strsep(&p, delim)) != NULL)
                   && (token[0] != '\0')) {

//                DBG_DYN_PRINT("[%s.%d][%s]\n", __FUNCTION__, __LINE__, token);

                para_idx++;
                parser_input(para_idx, token, cur_node);
                //DEBUG_PRINT("%s\n", token);
            }
        } else {
            index = simple_strtoul(token, &after, 10);
//            DBG_DYN_PRINT("index: %d\n", index);
            if (index >= 10000) {
                tf_debug_option(index);
            } else
            {
                tf_del_rule_by_user(index);
            }
        }

    }

    if (cur_node) {
        tf_rules_add(cur_node);
        dump_rule(cur_node);
    }

    return count;
}

static void *traffic_process_seq_start(struct seq_file *s, loff_t * pos)
{
    if (atomic_read(&store_list_head.n) == 0)
        return NULL;

    if (*pos >= atomic_read(&store_list_head.n)) {
        rcu_read_unlock();
        return NULL;
    }

    rcu_read_lock();
    return list_first_entry(&store_list_head.list,
                            struct TRAFFCIE_PROCESS_RULE, list);
}

static void *traffic_process_seq_next(struct seq_file *s, void *v,
                                      loff_t * pos)
{
    traffice_process_rule *tmp = NULL;
    (*pos)++;
    tmp = list_next_entry((traffice_process_rule *) v, list);
    if (&tmp->list == &store_list_head.list) {
        return NULL;
    }

    return tmp;
}

static void traffic_process_seq_stop(struct seq_file *s, void *v)
{

}

static int traffic_process_seq_show(struct seq_file *s, void *v)
{
    traffice_process_rule *rule = (traffice_process_rule *) v;
    char ip_str[16];

    snprintf(ip_str, 16, "%pI4", &rule->remoteAddress);

    //DEBUG_PRINT("mac: %pM\n", rule->h_dest);

    seq_printf(s,
               "RemoteAddress:%s RemotePort:%d Direction:%d HostMAC:%pM MethodList:%s statuscodeList:%s HeaderList:%s BundleName:%s Index:%d\n",
               ip_str, rule->remotePort, rule->direction, rule->h_dest,
               rule->methodList, rule->statusList, rule->HTTP_headerlist,
               rule->bundlename, rule->index);

    return 0;
}

static struct seq_operations traffic_process_detail_ops = {
    .start = traffic_process_seq_start,
    .next = traffic_process_seq_next,
    .stop = traffic_process_seq_stop,
    .show = traffic_process_seq_show
};

static int proc_traffic_rule_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &traffic_process_detail_ops);
};

static const struct file_operations traffic_process_fops = {
    .owner = THIS_MODULE,
    .open = proc_traffic_rule_open,
    .read = seq_read,
    .write = proc_traffic_rule_write,
    .llseek = seq_lseek,
    .release = seq_release,
};

static struct proc_dir_entry *proc_dir = NULL;

static int tf_create_proc(void)
{
    struct proc_dir_entry *entry = NULL;
    proc_dir = proc_mkdir(PROCFS_OSGI_DIR, NULL);
    if (proc_dir == NULL) {
        printk("%s: Register to /proc/%s failed \n", __FUNCTION__,
               PROCFS_OSGI_DIR);
        return 1;
    }

    entry =
        proc_create_data(PROC_NAME, 0644, proc_dir, &traffic_process_fops,
                         NULL);
    if (entry == NULL) {
        printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__,
               PROCFS_OSGI_DIR, PROC_NAME);
        return 1;
    }
    return 0;
}

static void tf_rm_proc(void)
{

    remove_proc_entry(PROC_NAME, proc_dir);
    remove_proc_entry(PROCFS_OSGI_DIR, NULL);

}

static int tf_hook_init(void)
{
    hash_init(tf_htable);
    atomic_set(&store_list_head.n, 0);
    INIT_LIST_HEAD(&store_list_head.list);

    printk("==== Netfilter hook: nf_traffic_detail_process init ====\n");
    nf_register_hook(&localin_routing_ops);
    nf_register_hook(&localout_routing_ops);
    nf_register_hook(&forward_routing_ops);

    tf_nl_init();
#ifdef __TEST_RULES
    init_test_rule();
#endif
    tf_create_proc();

#ifdef __TEST_HASHTBL
    tf_test_tbl();
    tf_dump_hash_table();
#endif
    //Hashtbl maintenance       
    tf_init_timer();


    return 0;
}

static void tf_hook_exit(void)
{

    nf_unregister_hook(&localin_routing_ops);
    nf_unregister_hook(&localout_routing_ops);
    nf_unregister_hook(&forward_routing_ops);
    tf_hashtbl_clear();
    del_timer(&hashtbl_timeout_timer);
    tf_nl_exit();
    tf_rm_proc();
    DEBUG_PRINT("hook_exit()=====================\n");
}

module_init(tf_hook_init);
module_exit(tf_hook_exit);
