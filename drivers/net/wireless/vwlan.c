#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/types.h>

#include <linux/version.h>


#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/mutex.h>


#include <linux/delay.h>

#include <linux/wireless.h>
#include <linux/vmalloc.h>
#include <net/sock.h>



#define DEFAULT_PORT 	2325
#define SIG_PORT 	2313
#define MODULE_NAME 	"vwlan"
#define DRV_VERSION 	"0.1.2"
#define SIOCGIWPRIV 	0x8B0D
#define SIOCSAPPPID     0x8b3e
#define SIOCGIWNAME     0x8B01
#define SIOCGMIIPHY		0x8947
//#ifdef SUPPORT_TX_MCAST2UNI
#define SIOCGIMCAST_ADD			0x8B80
#define SIOCGIMCAST_DEL			0x8B81
//#endif
#ifdef CONFIG_RTL_11R_SUPPORT
#define SIOCSIWRTLSETFTPID	0x8BE7
#endif
#define CMD_TIMEOUT 	5*HZ
#define RCV_TIMEOUT		3*HZ
#define CMD_IOCTL		0
#define CMD_OPEN		1
#define CMD_CLOSE		2
#define CMD_SETHWADD	3
#define CMD_GETSTATE	4
#define CMD_FORCESTOP	5
#define CMD_MASTER_CPU_ONLY 6
#define CMD_CPU_CONCURRENT  7
#define	ETHER_ADDRLEN	6
#define ETHER_HDRLEN 	14

//#define DEBUG_VWLAN
#ifdef DEBUG_VWLAN
#define dbg(fmt, args...) printk(fmt, ##args)
#else
#define dbg(fmt, args...) {}
#endif	

#define INADDR_SEND ((unsigned long int)0x0AFDFD02) //10.253.253.2
#define INADDR_RECV ((unsigned long int)0x0AFDFD01) //10.253.253.1

#define MAXLENGTH 	(64000)	
#define DATALENGTH	(63900)

#define LENGTH		56
#define IFNAMSIZ	16

//static struct socket *sock;
static struct socket *sock_send;
//static struct sockaddr_in addr;
static struct sockaddr_in addr_send;

static struct proc_dir_entry *proc_wlan1;
static struct proc_dir_entry *proc_vwlan;

#ifdef CONFIG_RTL_VAP_SUPPORT
#ifdef CONFIG_E8B
#define VAP_NUM			3
#else
#define VAP_NUM			4
#endif
#endif
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
#endif
#ifdef CONFIG_RTL_WDS_SUPPORT
#define MAX_WDS_NUM 	8
#endif

int vwlan_value=0;
//static DEFINE_MUTEX(pm_mutex);
static DEFINE_MUTEX(ksocket_mutex);

int vwlan_open(struct net_device *dev);
int vwlan_close(struct net_device *dev);
int vwlan_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
static int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
static int ksocket_receive(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
static void signal_socket_start(void);
static void signal_socket_end(void);
int vwlan_general_send_cmd(char *buf);

typedef struct vwlan_packet
{
	char cmd_type;
	struct iwreq wrq;
	int cmd;
	char ret;
	char data[DATALENGTH];
} vwlan_packet_t;

typedef struct vwlan_sig_packet
{
	int cmd;
	char ret;
	char data[sizeof(pid_t)];
} vwlan_sig_packet_t;

typedef struct vwlan_if_packet
{
	char cmd_type;
	char ifname[IFNAMSIZ];
	char hwaddr[ETHER_ADDRLEN*3];
	char ret;
} vwlan_if_packet_t;

#define VWLAN_PLEN	(sizeof(vwlan_packet_t) - DATALENGTH)

//char buffer[MAXLENGTH];

typedef struct vwlan_ioctl_data
{
	pid_t user_pid;
	int timeout;
} vwlan_ioctl_data_t;

struct kthread_t
{
        struct task_struct *thread;
        struct socket *sock;
        struct sockaddr_in addr;
        int running;
};

static struct kthread_t *kthread = NULL;

static int ksocket_create(struct socket **sock)
{
	int err = 0;
	if (  sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, sock) < 0 )
	{
		printk(KERN_INFO MODULE_NAME": Could not create a datagram socket, error = %d\n", -ENXIO);
		return -1;
	}
	return 0;
}

int vwlan_open_send(char *devname)
{
	char buf[sizeof(vwlan_if_packet_t)];
	vwlan_if_packet_t *vp;
	vp = (vwlan_if_packet_t *)buf;

	memset(buf, '\0', sizeof(buf)); 

	vp->cmd_type = CMD_OPEN;
	memcpy(vp->ifname, devname, IFNAMSIZ);
	memset(vp->ifname + 4, '0', 1);

	//ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
	vwlan_general_send_cmd(buf);
	
}

int vwlan_open(struct net_device *dev)
{
	vwlan_open_send(dev->name);
	return 0;
}

int vwlan_close_send(char *devname)
{
	char buf[sizeof(vwlan_if_packet_t)];
	vwlan_if_packet_t *vp;
	vp = (vwlan_if_packet_t *)buf;

	memset(buf, '\0', sizeof(buf)); 

	vp->cmd_type = CMD_CLOSE;
	memcpy(vp->ifname, devname, IFNAMSIZ);
	memset(vp->ifname + 4, '0', 1);

	//ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
	vwlan_general_send_cmd(buf);
}


int vwlan_close(struct net_device *dev)
{
	vwlan_close_send(dev->name);
	return 0;
}

int vwlan_cpu_master_only_send(char *devname)
{
    char buf[sizeof(vwlan_if_packet_t)];
    vwlan_if_packet_t *vp;
    vp = (vwlan_if_packet_t *)buf;

    memset(buf, '\0', sizeof(buf));

    vp->cmd_type = CMD_MASTER_CPU_ONLY;
    memcpy(vp->ifname, devname, IFNAMSIZ);
    memset(vp->ifname + 4, '0', 1);

    ksocket_create(&sock_send);
    ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
    sock_release(sock_send);
}

int vwlan_cpu_concurrent_send(char *devname)
{
    char buf[sizeof(vwlan_if_packet_t)];
    vwlan_if_packet_t *vp;
    vp = (vwlan_if_packet_t *)buf;

    memset(buf, '\0', sizeof(buf));

    vp->cmd_type = CMD_CPU_CONCURRENT;
    memcpy(vp->ifname, devname, IFNAMSIZ);
    memset(vp->ifname + 4, '0', 1);

    ksocket_create(&sock_send);
    ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
    sock_release(sock_send);
}

void force_stop_vwlan_hw(void)
{
	char buf[LENGTH];
	char type = CMD_FORCESTOP;

	memset(buf, '\0', sizeof(buf)); 

	memcpy(buf, &type, sizeof(char));

	ksocket_create(&sock_send);
	ksocket_send(sock_send, &addr_send, buf, LENGTH);
	sock_release(sock_send);
	printk("stop vwlan\n");
}

int  vwlan_set_hwaddr(struct net_device *dev, void *addrs)
{
	char buf[sizeof(vwlan_if_packet_t)];
	unsigned char ethaddr[ETHER_ADDRLEN];
	vwlan_if_packet_t *vp;
	struct sockaddr *saddr = addrs;
	int hwaddr_ind = 1;	

	if (!is_valid_ether_addr(saddr->sa_data))
			return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, saddr->sa_data, ETHER_ADDRLEN);
	memcpy(ethaddr, saddr->sa_data, ETHER_ADDRLEN);
	if(!strcmp(dev->name,"wlan1")){
#ifdef	CONFIG_RTL_VAP_SUPPORT
		hwaddr_ind += VAP_NUM;
#endif

#ifdef CONFIG_RTL_WDS_SUPPORT
		hwaddr_ind += (MAX_WDS_NUM);
#endif
		if((ethaddr[ETHER_ADDRLEN-1]+hwaddr_ind)>0xff){
			if((ethaddr[ETHER_ADDRLEN-2]+1)>0xff){
				ethaddr[ETHER_ADDRLEN-3]+=1;
			}
			ethaddr[ETHER_ADDRLEN-2]+=1;
		}
		ethaddr[ETHER_ADDRLEN-1] += hwaddr_ind;
	}
	//clear buf
	memset(buf, '\0', sizeof(buf)); 
	vp = (vwlan_if_packet_t *) buf;

	vp->cmd_type = CMD_SETHWADD;
	memcpy(vp->ifname, dev->name, IFNAMSIZ);
	memset(vp->ifname + 4, '0', 1);
    dbg("%s %s: Setting MAC address to %pM\n", __FILE__, dev->name, ethaddr);
	sprintf(vp->hwaddr, "%pM", ethaddr);
	
	//ksocket_send(sock_send, &addr_send, buf, sizeof(buf));
	vwlan_general_send_cmd(buf);
	
	return 0;
}

static void poll_func(unsigned long data) {
	vwlan_ioctl_data_t *vwlan_ioctl_d = data;
	printk("%s timeout!\n", __FILE__);
	vwlan_ioctl_d->timeout = 1;
	//kill_pid(find_get_pid(user_pid), SIGTERM, 1);
}

int vwlan_general_send_cmd(char *buf)
{
	
	int size=-1, err;
	int ret;
	struct timer_list tmr_poll;
	int send_len = sizeof(vwlan_if_packet_t);
	vwlan_if_packet_t *vwlan_pkt;
	vwlan_ioctl_data_t vwlan_ioctl_cmd;
	struct socket *sock;
	struct sockaddr_in addr;

	if( (err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock)) < 0)
	{
		printk(KERN_ERR MODULE_NAME": Could not create a datagram socket, error = %d\n", -err);
		kfree(buf);
		return -1;
	}
	
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr      = htonl(INADDR_RECV);
	addr.sin_port = htons(0);

	if ((err = sock->ops->bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr) ) ) < 0 )
	{
		printk(KERN_ERR MODULE_NAME": Could not bind or connect to socket, error = %d\n", -err);
		kfree(buf);
		return -1;
	}
	
	//if (!mutex_trylock(&pm_mutex)){
	//	return -1;
	//}
	
	//change device name to wlan0
	//sprintf(wrq->ifr_name, "wlan0");
	//memset(wrq->ifr_name + 4, '0', 1);
	//dbg("ifname %s\n", wrq->ifr_name);
		


	// cp cmd to buf
	vwlan_pkt = (vwlan_if_packet_t *) buf;
	dbg("cmd: %x\n", vwlan_pkt->cmd_type);

	// send packet with format: iwreq / cmd / iwreq data content

	//ksocket_send(sock_send, &addr_send, buf, MAXLENGTH);
	size = ksocket_send(sock, &addr_send, buf, send_len);
	if(size < 0){
		dbg("%s send failed\n", __FILE__);
		ret = -1;
		goto UN_LOCK;
	}

	//start timer
	//get user pid
    //vwlan_ioctl_cmd.user_pid = current->pid;
    //printk("user pid %d \n", vwlan_ioctl_cmd.user_pid);
	vwlan_ioctl_cmd.timeout = 0;
	init_timer(&tmr_poll);
	//tmr_poll.data = 0;
	tmr_poll.data = &vwlan_ioctl_cmd;
	tmr_poll.function = poll_func;	
	mod_timer(&tmr_poll, jiffies + CMD_TIMEOUT);
recieve_pkt:	

	//clear buf
	memset(buf, 0, sizeof(vwlan_if_packet_t));
	
	// receive packet
	size = ksocket_receive(sock, &addr, buf, sizeof(vwlan_if_packet_t));

	if(size >= 0){
		dbg("recieved\n");
		//del timer if receive
		del_timer(&tmr_poll);

		//check if ioctl failed
		if(vwlan_pkt->ret==0) {
			dbg("%s cmd failed\n", __FILE__);
			ret = -1;
			goto UN_LOCK;
		}

		ret = 0;
		goto UN_LOCK;
	}
	else{
		dbg("error getting datagram, sock_recvmsg error = %d\n", size);
		
		if(vwlan_ioctl_cmd.timeout==1) {
			ret = -1;
			goto UN_LOCK;
		}
		
		if(size==(-EAGAIN)){ //rcv timeout
			del_timer(&tmr_poll);
			ret = -1;
			goto UN_LOCK;
		}
		else// if(size==(-ERESTARTSYS))
			goto recieve_pkt;
	}
UN_LOCK:
	//mutex_unlock(&pm_mutex);
	sock_release(sock);
	return ret;
    
}


int vwlan_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	
	
	int size=-1, err, i;
	char type = CMD_IOCTL;
	int ret;
	struct iw_priv_args priv[128];
	struct iwreq *wrq = (struct iwreq *) ifr;
	struct iwreq twrq;
	struct timer_list tmr_poll;
	int send_len = VWLAN_PLEN;
	vwlan_packet_t *vwlan_pkt;
	vwlan_ioctl_data_t vwlan_ioctl_cmd;
	char *buf;
	struct socket *sock;
	struct sockaddr_in addr;

	if(cmd == SIOCGIMCAST_ADD || cmd == SIOCGIMCAST_DEL){
		return 0;
	}
	buf = kmalloc(MAXLENGTH, GFP_KERNEL);
	if(buf == NULL)
	{
		printk(KERN_ERR MODULE_NAME": memory allocate failed\n");	
		return -1;
	}

	if( (err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock)) < 0)
	{
		printk(KERN_ERR MODULE_NAME": Could not create a datagram socket, error = %d\n", -err);
		kfree(buf);
		return -1;
	}
	
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr      = htonl(INADDR_RECV);
	addr.sin_port = htons(0);

	if ((err = sock->ops->bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr) ) ) < 0 )
	{
		printk(KERN_ERR MODULE_NAME": Could not bind or connect to socket, error = %d\n", -err);
		kfree(buf);
		return -1;
	}
	
	//if (!mutex_trylock(&pm_mutex)){
	//	return -1;
	//}
	
	//change device name to wlan0
	//sprintf(wrq->ifr_name, "wlan0");
	//memset(wrq->ifr_name + 4, '0', 1);
	//dbg("ifname %s\n", wrq->ifr_name);
		
	vwlan_pkt = (vwlan_packet_t *)(buf);
	
	//clear buf
	memset(vwlan_pkt, 0, MAXLENGTH); 

	//cp type to buf
	vwlan_pkt->cmd_type = type;

	// cp iwreq struct to buf
	memcpy(&vwlan_pkt->wrq, wrq, sizeof(struct iwreq)); 
		
	//change device name to wlan0
	memset(vwlan_pkt->wrq.ifr_name + 4, '0', 1);

	// cp cmd to buf
	vwlan_pkt->cmd = cmd;
	dbg("cmd: %x\n", cmd);
	
	switch (cmd){
		case SIOCGIWNAME:
		case SIOCGMIIPHY:
		case SIOCGIWPRIV:
			break;
		default:			
			if(copy_from_user(vwlan_pkt->data, (void *)wrq->u.data.pointer, wrq->u.data.length)){
				ret = -1;
				goto UN_LOCK;
			}
			send_len += wrq->u.data.length;
			break;
	}
		
	// send packet with format: iwreq / cmd / iwreq data content

	//ksocket_send(sock_send, &addr_send, buf, MAXLENGTH);
	size = ksocket_send(sock, &addr_send, buf, send_len);
	if(size < 0){
		dbg("%s send failed\n", __FILE__);
		ret = -1;
		goto UN_LOCK;
	}

	//start timer
	//get user pid
    //vwlan_ioctl_cmd.user_pid = current->pid;
    //printk("user pid %d \n", vwlan_ioctl_cmd.user_pid);
	vwlan_ioctl_cmd.timeout = 0;
	init_timer(&tmr_poll);
	//tmr_poll.data = 0;
	tmr_poll.data = &vwlan_ioctl_cmd;
	tmr_poll.function = poll_func;	
	mod_timer(&tmr_poll, jiffies + CMD_TIMEOUT);
recieve_pkt:	

	//clear buf
	memset(buf, 0, MAXLENGTH);
	
	// receive packet with format: ret / iwreq / iwreq data content
	size = ksocket_receive(sock, &addr, buf, MAXLENGTH);

	if(size >= 0){
		dbg("recieved\n");
		//del timer if receive
		del_timer(&tmr_poll);
		
		if(cmd == SIOCSAPPPID
#ifdef CONFIG_RTL_11R_SUPPORT
		|| cmd == SIOCSIWRTLSETFTPID
#endif
		){
			//signal_socket_end();
			signal_socket_start();
		}

		//check if ioctl failed
		if(vwlan_pkt->ret==0) {
			dbg("%s ioctl failed\n", __FILE__);
			ret = -1;
			goto UN_LOCK;
		}

		//temporarily copy ioctl result
		memcpy(&twrq, &vwlan_pkt->wrq, sizeof(struct iwreq));

		if(vwlan_pkt->cmd != cmd)
			printk(KERN_ERR MODULE_NAME": receive wrong packet!\n");
	
		// cp iwreq data content from recieved packet
		switch (cmd){
			case SIOCGIWPRIV:
				if(copy_to_user((void *)wrq->u.data.pointer, vwlan_pkt->data, sizeof(struct iw_priv_args)*twrq.u.data.length)){
					ret = -1;
					goto UN_LOCK;
				}
				break;
			case SIOCGIWNAME:
				memcpy(wrq->u.name, twrq.u.name, sizeof(twrq.u.name));
				ret = 0;
				goto UN_LOCK;
			default:
				if(copy_to_user((void *)wrq->u.data.pointer, vwlan_pkt->data, twrq.u.data.length)){
					ret = -1;
					goto UN_LOCK;
				}
				break;
		}
		// cp iwreq data length & flags from recieved packet
		//__put_user(twrq.u.data.length, &wrq->u.data.length);
		//__put_user(twrq.u.data.flags, &wrq->u.data.flags);
		wrq->u.data.length = twrq.u.data.length;
		wrq->u.data.flags = twrq.u.data.flags;
		ret = 0;
		goto UN_LOCK;
	}
	else{
		dbg("error getting datagram, sock_recvmsg error = %d\n", size);
		
		if(vwlan_ioctl_cmd.timeout==1) {
			ret = -1;
			goto UN_LOCK;
		}
		
		if(size==(-EAGAIN)){ //rcv timeout
			del_timer(&tmr_poll);
			ret = -1;
			goto UN_LOCK;
		}
		else// if(size==(-ERESTARTSYS))
			goto recieve_pkt;
	}
UN_LOCK:
	//mutex_unlock(&pm_mutex);
	sock_release(sock);
	kfree(buf);
	return ret;
    
}

static int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	if (sock->sk==NULL)
		return 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = addr;
	msg.msg_namelen  = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_sendmsg(sock,&msg,len);
	set_fs(oldfs);

	return size;
}

static int ksocket_receive(struct socket* sock, struct sockaddr_in* addr, unsigned char* buf, int len)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;
	struct sock *sk=sock->sk;
	sk->sk_rcvtimeo = RCV_TIMEOUT;

	if (sock->sk==NULL) return 0;
	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = addr;
	msg.msg_namelen  = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_recvmsg(sock,&msg,len,msg.msg_flags);
	set_fs(oldfs);
	return size;
}

static void process_signal( vwlan_sig_packet_t *vp)
{
	pid_t pid;
	
	if(vp->ret == 2){ 
		memcpy(&pid, vp->data, sizeof(pid_t));
		if(vp->cmd == SIOCSAPPPID){
			dbg("send signal to pid (%d)\n", pid);
			kill_pid(find_get_pid(pid), SIGIO, 1);
		}
#ifdef CONFIG_RTL_11R_SUPPORT
		else if(vp->cmd == SIOCSIWRTLSETFTPID){
			dbg("send signal to ft pid (%d)\n", pid);
			kill_pid(find_get_pid(pid), SIGUSR1, 1);
		}
#endif
	}
	else{
		dbg("do nothing!\n");
	}
}

static void ksocket_start(void)
{
	int size, err;
	char buf[sizeof(vwlan_sig_packet_t)];
	vwlan_sig_packet_t *vp;
	vp = (vwlan_sig_packet_t *) buf;

	mutex_lock(&ksocket_mutex);
	kthread->running = 1;
	current->flags |= PF_NOFREEZE;

	allow_signal(SIGKILL);
	mutex_unlock(&ksocket_mutex);
	
	if ( (err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &kthread->sock)) < 0)

	{
		printk(KERN_INFO MODULE_NAME": Could not create a datagram socket, error = %d\n", -ENXIO);
		goto out;
	}

	memset(&kthread->addr, 0, sizeof(struct sockaddr_in));
	kthread->addr.sin_family      = AF_INET;

	kthread->addr.sin_addr.s_addr     = htonl(INADDR_RECV);
	kthread->addr.sin_port      = htons(SIG_PORT);

	if ( (err = kthread->sock->ops->bind(kthread->sock, (struct sockaddr *)&kthread->addr, sizeof(struct sockaddr) ) ) < 0) 
	{
		printk(KERN_INFO MODULE_NAME": Could not bind or connect to socket, error = %d\n", -err);
		goto close_and_out;
	}

	for (;;)
	{
		memset(&buf, 0, sizeof(vwlan_sig_packet_t));
		size = ksocket_receive(kthread->sock, &kthread->addr, buf, sizeof(vwlan_sig_packet_t));

		if (signal_pending(current))
			break;

		if (size < 0)
		{
			dbg(KERN_INFO MODULE_NAME": error getting datagram, sock_recvmsg error = %d\n", size);
		}
		else 
		{
			dbg(KERN_INFO MODULE_NAME": received %d bytes\n", size);
			process_signal(vp); 
			
		}
	}

close_and_out:
	sock_release(kthread->sock);
	kthread->sock = NULL;

out:
	kthread->thread = NULL;
	kthread->running = 0;
}


static void signal_socket_start(void)
{

	/* start kernel thread */
	if(kthread->thread == NULL){
		kthread->thread = kthread_run((void *)ksocket_start, NULL, MODULE_NAME);
		if (IS_ERR(kthread->thread)) 
		{
			printk(KERN_INFO MODULE_NAME": unable to start kernel thread\n");
			memset(kthread, 0, sizeof(struct kthread_t));
			//kfree(kthread);
			//kthread = NULL;
			//return -ENOMEM;
		}
	}

	//return 0;
}

static void signal_socket_end(void)
{
	int err;

	if(kthread->thread == NULL)
	{
		dbg(KERN_INFO MODULE_NAME": no kernel thread to kill\n");
	}
	else 
	{
	
		mutex_lock(&ksocket_mutex);
		err = kill_pid(find_get_pid(kthread->thread->pid), SIGKILL, 1);
		mutex_unlock(&ksocket_mutex);

		if (err < 0)
			printk(KERN_INFO MODULE_NAME": unknown error %d while trying to terminate kernel thread\n",-err);
		else 
		{
			while (kthread->running == 1)
				msleep(10);
			dbg(KERN_INFO MODULE_NAME": succesfully killed kernel thread!\n");
		}
	}

	if (kthread->sock != NULL) 
	{
		sock_release(kthread->sock);
		kthread->sock = NULL;
	}

}

/*Handle all user/system command from outside */
static int proc_vwlan_write(struct file *file, const char *buffer,
                      unsigned long count, void *data)
{
	//static char write_value='0'; 
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;

    if (count < 2)
		return -EFAULT;
	if (buffer && !copy_from_user(tmpBuf, buffer, len)) 
	{
	
		vwlan_value=simple_strtoul(tmpBuf, NULL, 16);
		switch(vwlan_value)
		{
			case 1:
				force_stop_vwlan_hw();
				break;
			case 2:
				vwlan_open_send("wlan1");
				break;
			case 3:
				vwlan_close_send("wlan1");
				break;
			case 4:
            	printk("vwlan_cpu_master_only\n");
            	vwlan_cpu_master_only_send("wlan1");				
				break;
			case 5:
            	printk("vwlan_cpu_concurrent\n");
            	vwlan_cpu_concurrent_send("wlan1");				
				break;

		}
		return len;
#if 0		
		/*value = 1 is force slave wlan down*/
		if(write_value=='1')
		{  
			force_stop_vwlan_hw();
            return count;
        }
		else if(write_value=='2')
		{
			vwlan_open_send("wlan1");
		}
		else if(write_value=='3')
		{
			vwlan_close_send("wlan1");
		}
        else if(write_value=='4')
        {
            printk("vwlan_cpu_master_only\n");
            vwlan_cpu_master_only_send("wlan1");
        }
        else if(write_value=='5')
        {
            printk("vwlan_cpu_concurrent\n");
            vwlan_cpu_concurrent_send("wlan1");
        }
		/**/
#endif		
	}
    return -EFAULT;
}

int proc_vwlan_read(struct seq_file *seq, void *v)
{
	seq_printf(seq, "vwlan=%d\n", vwlan_value);	  	

  return 0;

}


static int proc_vwlan_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_vwlan_read, inode->i_private);
}


struct file_operations proc_vwlan_fops=
{
	.owner = THIS_MODULE,
	.write = proc_vwlan_write,
    .open  = proc_vwlan_open,
    .read  = seq_read,	
};

static int __init vwlan_init(void)
{
	int err;

	kthread = kmalloc(sizeof(struct kthread_t), GFP_KERNEL);
	memset(kthread, 0, sizeof(struct kthread_t));

#if 0
  	if ( ( (err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_send)) < 0 ))
	{
		printk(KERN_INFO MODULE_NAME": Could not create a datagram socket, error = %d\n", -ENXIO);
		goto out;
	}
#endif

//	memset(&addr, 0, sizeof(struct sockaddr_in));
	memset(&addr_send, 0, sizeof(struct sockaddr_in));
//	addr.sin_family      = AF_INET;
	addr_send.sin_family = AF_INET;

//	addr.sin_addr.s_addr      = htonl(INADDR_RECV);
	addr_send.sin_addr.s_addr = htonl(INADDR_SEND);

//	addr.sin_port      = htons(DEFAULT_PORT);
	addr_send.sin_port = htons(DEFAULT_PORT);

//	if ((err = sock->ops->bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr) ) ) < 0 )
//	{
//		printk(KERN_INFO MODULE_NAME": Could not bind or connect to socket, error = %d\n", -err);
//		sock_release(sock);
//		sock_release(sock_send);
//		goto out;
//	}


	proc_wlan1 = proc_mkdir("wlan1",NULL);
#ifdef CONFIG_RTL_VAP_SUPPORT
	proc_wlan1 = proc_mkdir("wlan1-vap0",NULL);
	proc_wlan1 = proc_mkdir("wlan1-vap1",NULL);
	proc_wlan1 = proc_mkdir("wlan1-vap2",NULL);
	proc_wlan1 = proc_mkdir("wlan1-vap3",NULL);
#endif
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
	proc_wlan1 = proc_mkdir("wlan1-vxd",NULL);
#endif
	proc_vwlan = proc_create_data(MODULE_NAME, 0644, NULL, &proc_vwlan_fops, NULL);
	
out:

	printk(KERN_INFO MODULE_NAME": version %s\n", DRV_VERSION);
	//mutex_init(&pm_mutex);
	return 0;
}

static void __exit vwlan_exit(void)
{
	kfree(kthread);
    kthread = NULL;
	
	remove_proc_entry("wlan1",NULL);
#ifdef CONFIG_RTL_VAP_SUPPORT
	remove_proc_entry("wlan1-vap0",NULL);
	remove_proc_entry("wlan1-vap1",NULL);
	remove_proc_entry("wlan1-vap2",NULL);
	remove_proc_entry("wlan1-vap3",NULL);
#endif
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
	remove_proc_entry("wlan1-vxd",NULL);
#endif
	remove_proc_entry(MODULE_NAME, NULL);

	//sock_release(sock_send);
	
	printk(KERN_INFO MODULE_NAME": module unloaded\n");

}

/* init and cleanup functions */
late_initcall(vwlan_init);
module_exit(vwlan_exit);

/* module information */
MODULE_DESCRIPTION("virtual wlan device driver for ioctl");
MODULE_AUTHOR("Paula Tseng <pntseng@realtek.com>");
MODULE_LICENSE("GPL");



