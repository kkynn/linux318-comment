
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/netfilter/nf_traffic_detail_process.h>

#define TF_NETLINK_USER 30

struct sock *tf_nl_sk = NULL;
int pid = 0;

int tf_nl_send_msg(char *msg, char *msg2);

#if 0
static void dump_buff(char *user_data, int len)
{
    char *it;
    char *tail = user_data + len;

    printk("----%s point:%p, len:%d------------------:\n", __FUNCTION__,
           user_data, len);
    for (it = user_data; it != tail; ++it) {
        char c = *(char *) it;
        printk(" 0x%2x ", c);
    }
    printk("\n");
    printk("---------------------------------------------------\n");
}
#else
#define dump_buff(x, y)
#endif
static void tf_nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;

    char *msg = "Response from kernel";

    //   DEBUG_PRINT(KERN_INFO "Entering: %s\n", __FUNCTION__);

    nlh = (struct nlmsghdr *) skb->data;
    //  DEBUG_PRINT(KERN_INFO "Netlink received msg payload:%s, pid:%d\n", (char *)nlmsg_data(nlh), nlh->nlmsg_pid);
    pid = nlh->nlmsg_pid;       /*pid of sending process */

    tf_nl_send_msg(msg, NULL);
}

int tf_nl_send_msg(char *msg, char *msg2)
{
    struct sk_buff *skb_out;
    int res = -1;
    int msg_size;
    tf_msg_header_T msgh = { {'\0'} };
    struct nlmsghdr *nlh;
    char *data_p;

    if (pid == 0) {
        printk(KERN_ERR "Missing user space application pid(%d)\n", pid);
        return -1;
    }

    DBG_DYN_PRINT("sizeof(tf_msg_header_T)=%d, strlen(msg)=%d\n",
                  sizeof(tf_msg_header_T), strlen(msg));

    msg_size = sizeof(tf_msg_header_T) + strlen(msg) + 1;
    msgh.osgi_datalen = msg_size;
    if (msg2 != NULL) {
        strncpy(msgh.bundlename, msg2, sizeof(msgh.bundlename));
    } else {
        strncpy(msgh.bundlename, "KERNEL_NF_TRAFFIC_DETAIL_NETLINK",
                sizeof(msgh.bundlename));
    }

    //DEBUG_PRINT(" msgh.bundlename :%s , len:%d\n", msgh.bundlename,  strlen(msgh.bundlename));
//      DEBUG_PRINT("msg:\n%s\n", msg);

    skb_out = nlmsg_new(msg_size, GFP_ATOMIC);

    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return -1;
    }
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;  /* not in mcast group */

    data_p = nlmsg_data(nlh);
    memcpy(data_p, &msgh, sizeof(tf_msg_header_T));

    data_p = data_p + sizeof(tf_msg_header_T);
    memcpy(data_p, msg, strlen(msg));
    *(data_p + strlen(msg)) = '\0';

    //debug
    dump_buff(nlmsg_data(nlh), msg_size);


    res = netlink_unicast(tf_nl_sk, skb_out, pid, MSG_DONTWAIT);
    DBG_DYN_PRINT("  Send mesg to user app(%d): res(%d)\n", pid, res);
    if (res < 0) {
        printk(KERN_INFO "Error while sending msg  to user. error(%d)\n",
               res);
    }

    return res;
}

struct netlink_kernel_cfg cfg = {
    .input = tf_nl_recv_msg,
};

int tf_nl_init(void)
{
    //This is for 3.6 kernels and above.

    tf_nl_sk = netlink_kernel_create(&init_net, TF_NETLINK_USER, &cfg);

    if (!tf_nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -1;
    }
//     printk("%s\n", __FUNCTION__);
    return 0;
}

void tf_nl_exit(void)
{
    netlink_kernel_release(tf_nl_sk);
}
