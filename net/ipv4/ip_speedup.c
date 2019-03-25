#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <net/snmp.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/route.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/raw.h>
#include <net/checksum.h>
#include <linux/netfilter_ipv4.h>
#include <net/xfrm.h>
#include <linux/mroute.h>
#include <linux/netlink.h>
#include <linux/ip.h>

#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "ip_speedup.h"
#if defined(CONFIG_RTL8686)
#include  "../../arch/rlx/bsp_rtl8686/bspchip.h"
#elif defined(CONFIG_RTL9607C)
#include  "../../arch/rlx/bsp_rtl9607c/bspchip.h"
#include  "../../arch/rlx/bsp_rtl9607c/bspchip_9607c.h"
#endif
#define ip_inet_ntoa(x) _ip_inet_ntoa(x)
static void host_timeout(unsigned long data);
static int dbgflag = 0;

static inline char *_ui8tod( unsigned char n, char *p )
{
	if( n > 99 ) *p++ = (n/100) + '0';
	if( n >  9 ) *p++ = ((n/10)%10) + '0';
	*p++ = (n%10) + '0';
	return p;
}
static char *_ip_inet_ntoa(unsigned int ina)
{
	static char buf[4*sizeof "123"];
	char *p = buf;
	unsigned char *ucp = (unsigned char *)&ina;

	p = _ui8tod( ucp[0] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[1] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[2] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[3] & 0xFF, p);
	*p++ = '\0';

	return (buf);
}


#define TRACK_NUM 16

//__DRAM unsigned int HostSpeedUP=0;
volatile unsigned int HostSpeedUP=0;

struct timer_list ht_timer;
int HOST_TIMEOUT = 10; /*10 sec*/
int HTTP_PORT = 80;


#define SPEEDUP_CONN_NUM		50
#define SPEEDUP_HASH_SIZE 		64	//must be power of 2

struct _speedup_stream_track {
	u32 dstip;		/* server Addr */
	u32 srcip;		/* client Addr */
	u16 dport;		/* server Port */
	u16 sport;		/* client port */
	u32 totalbytes;	/* file segment size */
	u16 id;			/* ACK ID */
	u16 cnt;		/* rx pkt count */
	u32 seqno;		/* next sequence no. */
	u32 dupSegCnt;
	unsigned int rcvtime;	/* unit:us*/
	struct sk_buff *skb;		/* skb queued for quickly ack */
	//struct _speedup_stream_track *next;
	struct list_head node;
	/* add to active stream list */
	struct list_head activeNode;
};
static struct _speedup_stream_track sst[SPEEDUP_CONN_NUM];
static struct list_head sstFreeHeadList;
static struct list_head sstAciveHeadList;

struct _host_track_st {		
	u32 dstip;	/* local ip(client) */
	u32 srcip;	/* remote ip(server) */
	u16 dport;	
	u16 sport;
	u16 ref_cnt;	//support more than 1 url download test.
	u8  active;
	u8  bypassFastAck;	//default 0, if set to 1, bypass ip_speedup module and deliver to socket.
	//struct _speedup_stream_track *listHead;
	struct list_head hash[SPEEDUP_HASH_SIZE];
};
static struct _host_track_st host_track[10];

/* support up to 10 conntrack for speed test */
enum SPEEDUP_PROC_RT {
	SPEEDUP_STREAM_ADD,
	SPEEDUP_STREAM_DEL,
	SPEEDUP_STREAM_GET,
};

#define SAMPLE_NUM 60
static u8 sampletime=0;
static unsigned int hostspeeduprate[SAMPLE_NUM];//byte/s
static unsigned int bypassFastAckFlag=0;

unsigned short FAST_ACK_WIN_SIZE=0xff00;
static volatile unsigned int dataAckCnt=2;
static volatile unsigned int fastAckTimer = 15;	//unit:us

#ifdef CONFIG_RTL9607C
#define SPEEDTEST_TIMER_IDX	7
#else
#define SPEEDTEST_TIMER_IDX	3
#endif

#define REAL_SPEEDTEST_TIME	(REG32(BSP_TC0CNT + (SPEEDTEST_TIMER_IDX << 4)))

/* below variable for debug only */
#ifdef IP_SPEEDUP_DEBUG
static unsigned int duplicatePktCnt=0;
static unsigned int inOrderPktCnt=0;
static unsigned int outOrderPktCnt=0;
static unsigned int revisePktCnt=0;
static unsigned int delayAckPktCnt=0;
static unsigned int longDelayAckPktCnt=0;
unsigned int g_vec_nr=10;
static char * softirq_new_name[11] = {
	"HI", "TIMER", "NET_TX", "NET_RX", "BLOCK", "BLOCK_IOPOLL",
	"TASKLET", "SCHED", "HRTIMER", "RCU", "UNKNOWN"
};
static unsigned int softirq_delay_stats[11];
#endif

static unsigned int fastAckCalibrationStartTime=0;
static unsigned int fastAckCalibrationRxBytes=0;

static unsigned int speedtestSampleStartTime=0;
static unsigned int speedtestSampleRxBytes=0;

extern void __ip_select_ident(struct iphdr *iph, int segs);
static inline void host_track_on(u32 dstip, u32 srcip, u16 dport, u16 sport);
static void setup_Timer_for_speedup(void);

extern void memDump (void *start, u32 size, char * strHeader);
//#define HOSTCHECKDBG
#ifdef HOSTCHECKDBG
#define HOSTPRINTK printk
#else
#define HOSTPRINTK(f,a...) do {} while (0)
#endif

inline static u32
speedup_hash_entry(u32 sip, u32 sport, u32 dip, u32 dport)
{
	register __u16 hash = 0;
	
	hash = ((sip>>16)^sip);
	hash ^= ((dip>>16)^dip);
	hash ^= sport;
	hash ^= dport;
	
	return (SPEEDUP_HASH_SIZE-1) & (hash ^ (hash >> 12));
}

static void resetSstTbl(void)
{
	int i;
	unsigned long flags;

	local_irq_save(flags);

	memset(sst, 0, sizeof(sst));

	INIT_LIST_HEAD(&sstFreeHeadList);
	INIT_LIST_HEAD(&sstAciveHeadList);
	for (i=0; i<SPEEDUP_CONN_NUM; i++)
	{
		INIT_LIST_HEAD(&sst[i].node);
		INIT_LIST_HEAD(&sst[i].activeNode);
		list_add_tail(&sst[i].node, &sstFreeHeadList);
	}

	local_irq_restore(flags);
}

static void  retSSTEntryToFreeList(struct _speedup_stream_track * pEntry)
{
	list_add_tail(&pEntry->node, &sstFreeHeadList);
}

static struct _speedup_stream_track *getFreeSSTEntry(void)
{
	struct _speedup_stream_track *pEntry=NULL;

	if (list_empty(&sstFreeHeadList))
		return NULL;
	
	pEntry = list_first_entry(&sstFreeHeadList, struct _speedup_stream_track, node);
	list_del_init(&pEntry->node);
	
	return (pEntry);
}

static void delHostTrackEntry(struct _host_track_st *ht)
{
	struct _speedup_stream_track *pEntry, *pNextEntry;
	int i;

	for (i=0; i<SPEEDUP_HASH_SIZE; i++)
	{
		list_for_each_entry_safe(pEntry, pNextEntry, &ht->hash[i], node)
		{
			list_del(&pEntry->activeNode);
			list_del(&pEntry->node);
			retSSTEntryToFreeList(pEntry);
		}
	}

	memset(ht, 0, sizeof(struct _host_track_st));
}

static void _host_track_init(void)
{
	int i, j;
	
	memset(&host_track, 0, sizeof(host_track));
	for (i=0; i<10; i++)
	{
		for (j=0; j<SPEEDUP_HASH_SIZE; j++)
			INIT_LIST_HEAD(&host_track[i].hash[j]);
	}
	resetSstTbl();

	init_timer(&ht_timer);
	ht_timer.data = (unsigned long)host_track;
	ht_timer.function = host_timeout;
}

void fast_ack_calibration_init(void)
{
	fastAckCalibrationStartTime = 0;
	fastAckCalibrationRxBytes = 0;
}

void fast_ack_calibration_enter(int pktLen)
{
	//unsigned long flags;

	//local_irq_save(flags);
	if (0 == fastAckCalibrationStartTime)
		fastAckCalibrationStartTime = jiffies;
	
	fastAckCalibrationRxBytes += pktLen;
	if ((jiffies - fastAckCalibrationStartTime) >= 50)	//500ms
	{
		/*
		 * 700Mbps == 91750400Bps             half:45875200
		 * 600Mbps == 78643200Bps             half:39321600
		 * 500Mbps == 65536000Bps             half:32768000
		 * 300Mbps == 39321600Bps             half:19660800
		 */
		if (fastAckCalibrationRxBytes >= 39321600) //600Mbps
		{
			//FAST_ACK_WIN_SIZE = 0x4000;
			dataAckCnt = 8;
			fastAckTimer = 40;
		}
		else if (fastAckCalibrationRxBytes <= 19660800) //300Mbps
		{
			dataAckCnt = 2;
			fastAckTimer = 15;
			//FAST_ACK_WIN_SIZE = 0x0800;
		}
	
		fastAckCalibrationRxBytes = 0;
		fastAckCalibrationStartTime = jiffies;
	}
	//local_irq_restore(flags);
}

void speedtest_sample_init(void)
{
	speedtestSampleStartTime = 0;
	speedtestSampleRxBytes = 0;

	bypassFastAckFlag = 0;

	memset(hostspeeduprate, 0, sizeof(hostspeeduprate));
	sampletime = 0;

#ifdef IP_SPEEDUP_DEBUG
	inOrderPktCnt = 0;
	outOrderPktCnt = 0;
	duplicatePktCnt = 0;
	revisePktCnt = 0;
	delayAckPktCnt = 0;
	longDelayAckPktCnt = 0;
	g_vec_nr = 10;
	memset(softirq_delay_stats, 0, sizeof(softirq_delay_stats));
#endif
}

void speedtest_sample_enter(int pktLen)
{
	unsigned int time = REAL_SPEEDTEST_TIME;
	unsigned int diff;
	unsigned int rate;

	speedtestSampleRxBytes += pktLen;
	
	if (0 == speedtestSampleStartTime)
	{
		speedtestSampleStartTime = time;
		return;
	}
	
	if (time >= speedtestSampleStartTime)
		diff = time - speedtestSampleStartTime;
	else
		diff = 0xFFFFFFF - speedtestSampleStartTime + time;
	if (diff >= 1000000)	//1s
	{
		if (sampletime < SAMPLE_NUM)
		{
			rate = speedtestSampleRxBytes>>10;
			if ((rate > 100) && (diff < 1500000))//discard any sample long than 1.5s
			{
				if (0 == sampletime)
				{
					hostspeeduprate[sampletime] = rate;
					//if rx rate lower than 40Mbps, go normal path
					if (rate <= 5120)
						bypassFastAckFlag = 1;
				}
				else
					hostspeeduprate[sampletime] = hostspeeduprate[sampletime-1] + rate;
				
				sampletime++;
			}
			speedtestSampleRxBytes = 0;
		}
		
		speedtestSampleStartTime = time;
	}
}

static void _speedup_stream_track_record(u32 dstip, u32 srcip, u16 dport, u16 sport)
{
	struct _speedup_stream_track *pEntry, *pNewEntry;
	struct iphdr iph;
	unsigned int hashKey;
	int i;

	/* get host track entry */
	for (i=0; i<10; i++)
	{
		if (0 == host_track[i].active)
			break;
		
		if (((host_track[i].dstip == 0) || (host_track[i].dstip == srcip)) &&
			((host_track[i].dport == 0) || (host_track[i].dport == sport)) &&
			(host_track[i].srcip == dstip) &&
			((host_track[i].sport == 0) || (host_track[i].sport == dport)))
		{//rule alerasy existed.
			break;
		}
	}

	if ((i >= 10) || (0 == host_track[i].active))
	{
		printk("%s can not find speedup stream track entry (%x/%d->%x/%d)\n", __func__, srcip, sport, dstip, dport);

		if (i >= 10)
			return;
		else
			host_track_on(0, dstip, 0, dport);//new entry will be save to host_track[i]
	}

	hashKey = speedup_hash_entry(srcip, sport, dstip, dport);
	list_for_each_entry(pEntry, &host_track[i].hash[hashKey], node)
	{
		if (((0 == pEntry->dstip) || (pEntry->dstip == dstip)) &&
			((0 == pEntry->srcip) || (pEntry->srcip == srcip)) &&
			((0 == pEntry->dport) || (pEntry->dport == dport)) &&
			((0 == pEntry->sport) || (pEntry->sport == sport)))
		{
			printk("%s speedup stream already exist!\n", __func__);
			return;
		}
	}

	pNewEntry = getFreeSSTEntry();
	if (NULL == pNewEntry)
	{
		printk("%s can not get free sst entry!\n", __func__);
		return;
	}
	pNewEntry->dstip = dstip;
	pNewEntry->srcip = srcip;
	pNewEntry->dport = dport;
	pNewEntry->sport = sport;
	pNewEntry->totalbytes = 0;
	pNewEntry->seqno = 0;
	pNewEntry->skb = NULL;
	iph.saddr = srcip;
	iph.daddr = dstip;
	iph.protocol = IPPROTO_TCP;
	__ip_select_ident(&iph, 1);
	pNewEntry->id = iph.id;
	pNewEntry->cnt = 0;

	list_add_tail(&pNewEntry->node, &host_track[i].hash[hashKey]);
	list_add_tail(&pNewEntry->activeNode, &sstAciveHeadList);
	//printk("%s add stream src %x/%d dst %x/%d\n", __func__, srcip, sport, dstip, dport);
}

static void _speedup_stream_track_del(u32 dstip, u32 srcip, u16 dport, u16 sport)
{
	struct _speedup_stream_track *pEntry, *pNextEntry;
	unsigned int hashKey;
	int i;

	/* get host track entry */
	for (i=0; i<10; i++)
	{
		if (0 == host_track[i].active)
			break;

		if (((host_track[i].dstip == 0) || (host_track[i].dstip == srcip)) &&
			((host_track[i].dport == 0) || (host_track[i].dport == sport)) &&
			(host_track[i].srcip == dstip) &&
			((host_track[i].sport == 0) || (host_track[i].sport == dport)))
		{//rule alerasy existed.
			break;
		}
	}

	if ((i >= 10) || (0 == host_track[i].active))
	{
		printk("%s can not find speedup stream track entry (%x/%d->%x/%d)\n", __func__, srcip, sport, dstip, dport);
		return;
	}

	hashKey = speedup_hash_entry(srcip, sport, dstip, dport);
	list_for_each_entry_safe(pEntry, pNextEntry, &host_track[i].hash[hashKey], node)
	{
		if (((0 == pEntry->dstip) || (pEntry->dstip == dstip)) &&
			((0 == pEntry->srcip) || (pEntry->srcip == srcip)) &&
			((0 == pEntry->dport) || (pEntry->dport == dport)) &&
			((0 == pEntry->sport) || (pEntry->sport == sport)))
		{
			list_del_init(&pEntry->node);
			list_del_init(&pEntry->activeNode);
			retSSTEntryToFreeList(pEntry);
			break;
		}
	}
	//printk("%s del stream src %x/%d dst %x/%d\n", __func__, srcip, sport, dstip, dport);
}

int HostSpeedUP_Stat_get(u32 dstip, u32 srcip, u16 dport, u16 sport)
{
	struct _speedup_stream_track *pEntry;
	unsigned int hashKey;
	u32 rcvBytes;
	int i;

	/* get host track entry */
	for (i=0; i<10; i++)
	{
		if (0 == host_track[i].active)
			break;

		if (((host_track[i].dstip == 0) || (host_track[i].dstip == srcip)) &&
			((host_track[i].dport == 0) || (host_track[i].dport == sport)) &&
			(host_track[i].srcip == dstip) &&
			((host_track[i].sport == 0) || (host_track[i].sport == dport)))
		{//rule alerasy existed.
			break;
		}
	}

	if ((i >= 10) || (0 == host_track[i].active))
	{
		printk("%s can not find speedup stream track entry (%x/%d->%x/%d)\n", __func__, srcip, sport, dstip, dport);
		return 0;
	}

	hashKey = speedup_hash_entry(srcip, sport, dstip, dport);
	list_for_each_entry(pEntry, &host_track[i].hash[hashKey], node)
	{
		if (((0 == pEntry->dstip) || (pEntry->dstip == dstip)) &&
			((0 == pEntry->srcip) || (pEntry->srcip == srcip)) &&
			((0 == pEntry->dport) || (pEntry->dport == dport)) &&
			((0 == pEntry->sport) || (pEntry->sport == sport)))
		{
			rcvBytes = pEntry->totalbytes;
			pEntry->totalbytes = 0;
			return (rcvBytes);
		}
	}
	
	return 0;
}

#if 0
void print_debug_info(u32 dstip, u32 srcip, u16 dport, u16 sport)
{
	struct _speedup_stream_track *pEntry, *pNextEntry;
	unsigned int hashKey;
	int i, j;
	int streamNum=0;

	hashKey = speedup_hash_entry(dstip, dport, srcip, sport);
	printk("[unknown %d] %x/%d -> %x/%d\n", hashKey, srcip, sport, dstip, dport);
	printk("	stream table list:\n");
	for (i=0; i<10; i++)
	{
		for (j=0; j<SPEEDUP_HASH_SIZE; j++)
		{
			list_for_each_entry_safe(pEntry, pNextEntry, &host_track[i].hash[j], node)
			{
				if (((0 == pEntry->dstip) || (pEntry->dstip == srcip)) &&
					((0 == pEntry->srcip) || (pEntry->srcip == dstip)) &&
					((0 == pEntry->dport) || (pEntry->dport == sport)) &&
					((0 == pEntry->sport) || (pEntry->sport == dport)))
				{
					printk("	match in hash %d, calc hash is %d\n", j, hashKey);
					return;
				}
				streamNum++;
				printk("	[%d]stream %d src %x/%d dst %x/%d\n", j, streamNum, pEntry->srcip, pEntry->sport,
							pEntry->dstip, pEntry->dport);
			}
		}
	}
}
#endif

unsigned int get_speedtest_time(void)
{
	return REAL_SPEEDTEST_TIME;
}

unsigned int get_time_duration(unsigned int startTime)
{
	unsigned int currtime;
	unsigned int duration;	//unit us;

	currtime = REAL_SPEEDTEST_TIME;
	if (currtime >= startTime)
		duration = currtime - startTime;
	else
		duration = 0xFFFFFFF - startTime + currtime;

	return duration;
}

/*
 * RETURN VALUE: 1 means we should send ack now; 0 means don't need to send ack
 */
static inline int should_ack_now(unsigned int rcvtime)
{
	unsigned int currtime;
	unsigned int duration;	//unit us;

	currtime = REAL_SPEEDTEST_TIME;
	if (currtime >= rcvtime)
		duration = currtime - rcvtime;
	else
		duration = 0xFFFFFFF - rcvtime + currtime;

#ifdef IP_SPEEDUP_DEBUG
	if (duration >= 100)
	{
		longDelayAckPktCnt++;
		softirq_delay_stats[g_vec_nr]++;
		//printk("start %u  end %u\n", rcvtime, currtime);
	}
#endif
	if (duration >= fastAckTimer)//ACK NOW
		return 1;
	
	return 0;
}

#ifdef IP_SPEEDUP_DEBUG
unsigned int get_softirq_delay_stats(int vec_nr)
{
	return (softirq_delay_stats[vec_nr]);
}
#endif

/* 20180103: sometimes http server will wait for ack to send the next segment, if speedup module 
 * didn't send ack because dataAckCnt is not reached, so the interval between the two segment will 
 * be too large, 
 * there is no timer for such case, so I think ack should be triggered when NIC Rx handler processed
 * all packet in rx ring and rx ring is empty.
 * We have tested this patch, and found that there are too many ack sent and throughput is poor. is it
 * an ugly idea???
 * 20180104: I have add a real count for timer check.
 *           modify return value type from void to int, 1 means some segment is not acked, 0 means no 
 *           segment wait for ack.
 */
int send_delay_ack_timer(void)
{
	struct _speedup_stream_track *pEntry;
	int unAckSegment=0;
	int ackedNum=0;

	/* FIXME: we must waiting for all thread up, 2s is enough??? 
	 *  20180105: I think maybe waiting for at least 5 stream established is a better idea.
	 */
	//if (sampletime <= 2)
	//	return 0;
#ifdef CONFIG_SMP
	/* TODO: eth0 interrupt is exec in cpu 1 for 9607C, so speedup should only be implemented in cpu 1,
	 * eth0 interrupt is default mapping to CPU 0, which is defined in bsp_gic_intr_map array of irq.c
	 * while romedriver will remapping it to CPU 1 in rtl86900/romeDriver/rtk_rg_apollo_liteRomeDriver.c
	 * such as: irq_set_affinity(52, &cpumask); //GMAC9
	 * 			cpu_mask = 0x2 -> CPU1
	 * if mapping relationship is changed, below patch should also be modified, remember!!!
	 */
#if CONFIG_NR_CPUS == 4
	if (1 != smp_processor_id())
		return 0;
#else
	if (0 != smp_processor_id())
		return 0;
#endif
#endif
XMIT_ACK:
	list_for_each_entry(pEntry, &sstAciveHeadList, activeNode)
	{
		if (NULL == pEntry->skb)//no ack should be sent
			continue;

		if ((pEntry->cnt < dataAckCnt) && !should_ack_now(pEntry->rcvtime))
		{
			unAckSegment = 1;
			continue;
		}
		
#ifdef IP_SPEEDUP_DEBUG
		delayAckPktCnt++;
#endif
		/* TODO send ack immediately */
		re8670_speedup_ack_send(pEntry->skb, pEntry->seqno, pEntry->id);
		pEntry->id++;
		pEntry->skb = NULL;
		pEntry->cnt = 0;
		
		ackedNum++;
	}
	if ((ackedNum >= 5) && unAckSegment)
	{
		ackedNum = 0;
		unAckSegment = 0;
		
		goto XMIT_ACK;
	}
	return (unAckSegment);
}

/* shortcut for speedtest 
 * PARAMETER
 *   dstip -  client Addr
 *   srcip -  server Addr
 * RETURN VALUE
 *   1 -  success;  0 -  fail
 */
int speedtest_stream_shortcut_check(void *pHostTrack, 
										u32 dstip, u32 srcip, u16 dport, u16 sport, 
										u32 *seqNo, u16 *id, u32 dataLen, 
										struct sk_buff *skb)
{
	struct _host_track_st *ht;
	struct _speedup_stream_track *pEntry;
	unsigned int hashKey;

	ht = (struct _host_track_st *)pHostTrack;
	hashKey = speedup_hash_entry(dstip, dport, srcip, sport);
	list_for_each_entry(pEntry, &ht->hash[hashKey], node)
	{
		if (((0 == pEntry->dstip) || (pEntry->dstip == srcip)) &&
			((0 == pEntry->srcip) || (pEntry->srcip == dstip)) &&
			((0 == pEntry->dport) || (pEntry->dport == sport)) &&
			((0 == pEntry->sport) || (pEntry->sport == dport)))
		{
			/*
			 * Rules for Generating ACK
			 * 1. When one end sends a data segment to the other end, it must include an ACK.  That gives the next sequence number it expects to receive. (Piggyback)
			 * 2. The receiver needs to delay sending (until another segment arrives or 500ms) an ACK segment if there is only one outstanding in-order segment. 
			 *    It prevents ACK segments from creating extra traffic.
			 * 3. There should not be more than 2 in-order unacknowledged segments at any time. It prevent the unnecessary retransmission
			 * 4. When a segment arrives with an out-of-order sequence number that is higher than expected, the receiver immediately sends an ACK segment 
			 *    announcing the sequence number of the next expected segment. (for fast retransmission)
			 * 5. When a missing segment arrives, the receiver sends an ACK segment to announce the next sequence number expected.
			 * 6. If a duplicate segment arrives, the receiver immediately sends an ACK.
			 */
			if (0 == pEntry->seqno)
			{
				*seqNo += dataLen;
				pEntry->seqno = *seqNo;
				pEntry->totalbytes += dataLen;
				pEntry->cnt = 0;
				if (pEntry->skb != NULL)
				{
					__kfree_skb(pEntry->skb);
					pEntry->skb = NULL;
				}
				
				return SPEEDUP_RET_MATCH_ACK;
			}

			if (*seqNo < pEntry->seqno)		//missing segment or duplicate segment arrives, ack immediately.
			{
#ifdef IP_SPEEDUP_DEBUG
				duplicatePktCnt++;
#endif
				
				*id = pEntry->id++;
				/* QL if contineously send unseen segment ack, we should reassign seqno */
				pEntry->dupSegCnt++;
				if (pEntry->dupSegCnt >= 5)
				{
#ifdef IP_SPEEDUP_DEBUG
					revisePktCnt++;
#endif
					pEntry->dupSegCnt = 0;
					*seqNo += dataLen;
					pEntry->seqno = *seqNo;
					pEntry->cnt = 0;
					if (pEntry->skb != NULL)
					{
						__kfree_skb(pEntry->skb);
						pEntry->skb = NULL;
					}
				}
				else
				{
					/* LOG: ack the lost segment only, is it better to ack the newest segment? 
					 * 20180104 It seems ack the newest segment get the better result.
					 */
					#if 0
					*seqNo += dataLen;

					if (pEntry->seqno <= *seqNo)
					{
						pEntry->seqno = *seqNo;
						pEntry->cnt = 0;
						if (pEntry->skb != NULL)
						{
							__kfree_skb(pEntry->skb);
							pEntry->skb = NULL;
						}
					}
					#else
					*seqNo = pEntry->seqno;
					pEntry->cnt = 0;
					if (pEntry->skb != NULL)
					{
						__kfree_skb(pEntry->skb);
						pEntry->skb = NULL;
					}
					#endif
				}

				return SPEEDUP_RET_MATCH_ACK;
			}
			
#ifdef IP_SPEEDUP_DEBUG
			if (*seqNo == pEntry->seqno)
				inOrderPktCnt++;
			else
				outOrderPktCnt++;
#endif
			
			pEntry->dupSegCnt = 0;
			
			*seqNo += dataLen;
			pEntry->seqno = *seqNo;
			pEntry->totalbytes += dataLen;
			pEntry->cnt++;
			
			/* because we have no delay timer for ack, so speedup will send ack for the last data packet for delay requirement. */
			if (pEntry->cnt >= dataAckCnt)	//ack immediately
			{
				#if 0
				pEntry->cnt = 0;
				*id = pEntry->id++;
				if (pEntry->skb != NULL)
				{
					__kfree_skb(pEntry->skb);
					pEntry->skb = NULL;
				}
				
				return SPEEDUP_RET_MATCH_ACK;
				#else
				send_delay_ack_timer();
				return SPEEDUP_RET_MATCH_NO_ACK;
				#endif
			}
			
			if (NULL == pEntry->skb)
			{
				pEntry->rcvtime = REAL_SPEEDTEST_TIME;
				
				pEntry->skb = skb;
				return SPEEDUP_RET_MATCH_QUEUED;
			}
			
			return SPEEDUP_RET_MATCH_NO_ACK;
		}
	}

	return SPEEDUP_RET_UNMATCH;
}

static void host_timeout(unsigned long data) {
	struct _host_track_st *ht = (struct _host_track_st *)data;
	int i;

	if (!list_empty(&sstAciveHeadList))
	{
		mod_timer(&ht_timer, jiffies+(HOST_TIMEOUT*HZ));
		return;
	}
	
	for (i=0; i<10; i++)
	{
		ht[i].active = 0;
	}
	resetSstTbl();
	
	printk("host_track timeout!\n");

};

static inline void host_track_on(u32 dstip, u32 srcip, u16 dport, u16 sport)
{
	int first_on=1;
	int i, j;

	if (unlikely(!HostSpeedUP))
	{
		printk("host speedup is off!\n");
		return;
	}
	
	//printk("host_track_on(%s:%d,%s:%d)\n",ip_inet_ntoa(dstip),dport,ip_inet_ntoa(srcip),sport);
	for (i=0; i<10; i++)
	{
		if (host_track[i].active)
		{
			/* host tracking is already enabled */
			first_on = 0;

			if ((host_track[i].dstip == dstip) &&
				(host_track[i].dport == dport) &&
				(host_track[i].srcip == srcip) &&
				(host_track[i].sport == sport))
			{//rule alerasy existed.
				host_track[i].ref_cnt++;
				break;
			}
		}
		else
		{
			//no matched rule found, add a new rule.
			host_track[i].dstip = dstip;
			host_track[i].srcip = srcip;
			host_track[i].dport = dport;
			host_track[i].sport = sport;
			host_track[i].ref_cnt = 1;
			for (j=0; j<SPEEDUP_HASH_SIZE; j++)
				INIT_LIST_HEAD(&host_track[i].hash[j]);
			host_track[i].active = 1;
			host_track[i].bypassFastAck = 0;
			break;
		}
	}

	if (first_on)
	{
	    speedtest_sample_init();
		fast_ack_calibration_init();
	}
	
	mod_timer(&ht_timer, jiffies+(HOST_TIMEOUT*HZ));
}

static inline void host_track_off(u32 dstip, u32 srcip, u16 dport, u16 sport)
{
	int i;
	
	printk("host_track_off enter\n");
	
#ifdef CONFIG_SMP
	/* TODO: eth0 interrupt is exec in cpu 1 for 9607C, so speedup should only be implemented in cpu 1,
	 * eth0 interrupt is default mapping to CPU 0, which is defined in bsp_gic_intr_map array of irq.c
	 * while romedriver will remapping it to CPU 1 in rtl86900/romeDriver/rtk_rg_apollo_liteRomeDriver.c
	 * such as: irq_set_affinity(52, &cpumask); //GMAC9
	 *			cpu_mask = 0x2 -> CPU1
	 * if mapping relationship is changed, below patch should also be modified, remember!!!
	 */
#if CONFIG_NR_CPUS == 4
	if (1 != smp_processor_id())
	{
		printk("!!!!host_track_off not on cpu1, return\n");
		return 0;
	}
#else
	if (0 != smp_processor_id())
	{
		printk("!!!!host_track_off not on cpu0, return\n");
		return 0;
	}
#endif
#endif
	if ((0 == dstip) && (0 == srcip) && (0 == sport) && (0 == dport))
	{
		for (i=0; i<10; i++)
		{
			host_track[i].active = 0;
		}
		resetSstTbl();

		del_timer(&ht_timer);
	}
	else
	{
		for (i=0; i<10; i++)
		{
			if (host_track[i].active)
			{
				if ((host_track[i].dstip == dstip) &&
					(host_track[i].dport == dport) &&
					(host_track[i].srcip == srcip) &&
					(host_track[i].sport == sport))
				{//rule found
					host_track[i].ref_cnt--;
					if (0 == host_track[i].ref_cnt)
					{//delete it
						delHostTrackEntry(&host_track[i]);
						
						/* FIXME: is it a good idea to move following rule ahead? */
						if ((9 != i) && (0 != host_track[i+1].active))
						{
							for (; i<10; i++)
							{
								if (i < 9)
								{
									memcpy(&host_track[i], &host_track[i+1], sizeof(struct _host_track_st));
									if (0 == host_track[i+1].active)
										break;
								}
								else
								{
									memset(&host_track[i], 0, sizeof(struct _host_track_st));
								}
							}
						}
					}
					break;
				}
			}
			else
				break;
		}
	}
}

static inline void host_track_set(u32 dstip, u32 srcip, u16 dport, u16 sport, int bypass)
{
	int first_on=1;
	int i, j;

	if (unlikely(!HostSpeedUP))
	{
		printk("host speedup is off!\n");
		return;
	}
	
	//printk("host_track_on(%s:%d,%s:%d)\n",ip_inet_ntoa(dstip),dport,ip_inet_ntoa(srcip),sport);
	for (i=0; i<10; i++)
	{
		if (host_track[i].active)
		{
			if ((host_track[i].dstip == dstip) &&
				(host_track[i].dport == dport) &&
				(host_track[i].srcip == srcip) &&
				(host_track[i].sport == sport))
			{//rule alerasy existed.
				host_track[i].bypassFastAck = bypass;
				break;
			}
		}
		else
			break;
	}
}

static inline int host_match(u32 dstip, u32 srcip, u16 dport, u16 sport)
{
	int i;
	int r=0;

	for (i=0; i<10; i++)
	{
		if (0 == host_track[i].active)
			break;
		
		if (((host_track[i].dstip == 0) || (host_track[i].dstip == dstip)) && 
			((host_track[i].srcip == srcip)) && 
			((host_track[i].dport==0)||(host_track[i].dport == dport)) && 
			((host_track[i].sport==0)||(host_track[i].sport == sport)))
		{
			r = 1;
			break;
		}
	}
	
	if (r){
		mod_timer(&ht_timer, jiffies+(HOST_TIMEOUT*HZ));
	}
	return r;
}

static inline int checkSambaIPandPort(u32 ip, u16 port)
{
	int r = 0;
	r = ( HostSpeedUP && 
		( ip == 0xc0a80101 ) && 
		( (port == 137) || (port == 138) || (port == 139) || (port == 445) ) );
	
	if (r){
		mod_timer(&ht_timer, jiffies+(HOST_TIMEOUT*HZ));
	}	
	return r;
}

int isHostSpeedUpEnable(void)
{
    return HostSpeedUP;
}

void * shouldSpeedUp(u32 dir, u32 dstip, u32 srcip, u16 dport, u16 sport)
{
	int ret = 0;
	int i;
	static int sp_count = 0;
	
    if (0 == dir)
    {//local in
    	for (i=0; i<10; i++)
		{
			if (0 == host_track[i].active)
				break;
			
	        if (((host_track[i].dstip == 0) || (host_track[i].dstip == dstip)) && 
	            ((host_track[i].srcip == srcip)) && 
	            ((host_track[i].dport==0)||(host_track[i].dport == dport)) && 
	            ((host_track[i].sport==0)||(host_track[i].sport == sport)))
        	{
	            ret = 1;
				break;
        	}
		}
    }
    else
    {//local out
    	for (i=0; i<10; i++)
		{
			if (0 == host_track[i].active)
				break;
			
	        if (((host_track[i].dstip == 0) || (host_track[i].dstip == srcip)) && 
	            ((host_track[i].srcip == dstip)) && 
	            ((host_track[i].dport==0)||(host_track[i].dport == sport)) && 
	            ((host_track[i].sport==0)||(host_track[i].sport == dport)))
        	{
	            ret = 1;
				break;
        	}
		}
    }
	
	if (ret)
	{
		if (sp_count++ >= 100)
		{
			sp_count = 0;
			mod_timer(&ht_timer, jiffies+(HOST_TIMEOUT*HZ));
		}

		return (void *)&host_track[i];
	}

    return (NULL);
}

int shouldBypassSpeedUP(void *pHostTrack)
{
 	struct _host_track_st *ht = (struct _host_track_st *)pHostTrack;

	return ht->bypassFastAck || bypassFastAckFlag;
}

#if 1
/*
direction:
1, Up stream
2, Down stream
*/
int isHostCheck(struct sk_buff *skb, int hook, int direction) {
	struct iphdr *iph = ip_hdr(skb);	
	//const struct iphdr *iph = ((struct sk_buff *)skb)->nh.iph;
	struct tcphdr *th;
	
	if (unlikely(!HostSpeedUP))
		return 0;

	if (unlikely( iph->frag_off & 0x3fff )) /* Ignore fragment */		 
	{
		HOSTPRINTK(KERN_ERR "[%s][%d]\n", __FUNCTION__, __LINE__);
		return 0;		
	}
	
	//if( IPPROTO_TCP != iph->protocol )
	//	return 0;
		
	th=(struct tcphdr *) ((void *) iph + iph->ihl*4);
	if (th->syn)
		return 0;

//printk("%s(%d):hook%d, s=%x:%d d=%x:%d dir=%d\n",__func__,__LINE__,hook,iph->saddr,th->source,iph->daddr,th->dest,direction);	
	switch (hook) {
	case NF_INET_LOCAL_IN:
		if (host_match(iph->daddr,iph->saddr,th->dest,th->source)) {
			HOSTPRINTK(KERN_ERR "[%s][%d] DATA0 %x:%d<-%x:%d\n", __FUNCTION__, __LINE__,iph->daddr,th->dest,iph->saddr,th->source);
			return 1;
		}else if ( checkSambaIPandPort(iph->daddr ,th->dest) ){
			if ( dbgflag )
				HOSTPRINTK(KERN_ERR "[%s][%d] DATA0 %x:%d<-%x:%d\n", __FUNCTION__, __LINE__,iph->daddr,th->dest,iph->saddr,th->source);
			return 1;
		}
		break;
	case NF_INET_LOCAL_OUT:
//printk("%s(%d):hook%d, s=%x:%d d=%x:%d\n",__func__,__LINE__,hook,iph->saddr,th->source,iph->daddr,th->dest);
		if (host_match(iph->saddr,iph->daddr,th->source,th->dest)) {			
			HOSTPRINTK(KERN_ERR "[%s][%d] DATA1 %x:%d->%x:%d\n", __FUNCTION__, __LINE__,iph->saddr,th->source,iph->daddr,th->dest);
			return 1;
		}else if ( checkSambaIPandPort(iph->saddr ,th->source) ){
			HOSTPRINTK(KERN_ERR "[%s][%d] DATA0 %x:%d<-%x:%d\n", __FUNCTION__, __LINE__,iph->daddr,th->dest,iph->saddr,th->source);
			return 1;
		}
		break;
	case NF_INET_PRE_ROUTING:
		if (host_match(iph->daddr,iph->saddr,th->dest,th->source)) {
			HOSTPRINTK(KERN_ERR "[%s][%d] DATA2 %x:%d<-%x:%d\n", __FUNCTION__, __LINE__,iph->daddr,th->dest,iph->saddr,th->source);
			return 1;
		}else if ( checkSambaIPandPort(iph->daddr ,th->dest) ){
			if ( dbgflag )		
				HOSTPRINTK(KERN_ERR "[%s][%d] DATA0 %x:%d<-%x:%d\n", __FUNCTION__, __LINE__,iph->daddr,th->dest,iph->saddr,th->source);
			return 1;
		}
		break;
	case NF_INET_POST_ROUTING:
		if (skb->dev) // only care about local out 
			return 0;
//printk("%s(%d):hook%d, s=%x:%d d=%x:%d\n",__func__,__LINE__,hook,iph->saddr,th->source,iph->daddr,th->dest);
		if (host_match(iph->saddr,iph->daddr,th->source,th->dest)) {
			HOSTPRINTK(KERN_ERR "[%s][%d] DATA3 %x:%d->%x:%d\n", __FUNCTION__, __LINE__,iph->saddr,th->source,iph->daddr,th->dest);
			return 1;
		}else if ( checkSambaIPandPort(iph->saddr ,th->source) ){
			if ( dbgflag )
				HOSTPRINTK(KERN_ERR "[%s][%d] DATA0 %x:%d<-%x:%d\n", __FUNCTION__, __LINE__,iph->daddr,th->dest,iph->saddr,th->source);
			return 1;
		}
		break;
	default:
		return 0;
	}
	return 0;
}
#else
int isHostCheck(struct sk_buff *skb, int hook, int direction)
{
	return 0;
}
#endif

int HostSpeedUP_read_proc(struct seq_file *seq, void *v)
{
	seq_printf(seq, "HostSpeedUP=%d\n", HostSpeedUP);
	return 0;
}

static int proc_HostSpeedUP_open(struct inode *inode, struct file *file)
{
	return single_open(file, HostSpeedUP_read_proc, inode->i_private);
}

int HostSpeedUP_write_proc(struct file *file, const char __user *buffer, size_t len, loff_t *ppos)
{	
	unsigned char tmpBuf[16] = {0};
	int count = (len > 15) ? 15 : len;

	if (buffer && !copy_from_user(tmpBuf, buffer, count))
	{
		HostSpeedUP=simple_strtoul(tmpBuf, NULL, 16);
		switch(HostSpeedUP) 
		{
			case 0:
				printk("Turn off Host Speed UP HostSpeedUP:%d\n",HostSpeedUP);
				break;
			case 1:
				printk("Turn on Host Speed UP HostSpeedUP:%d\n",HostSpeedUP);
				setup_Timer_for_speedup();
				break;
		}
		return count;
	}	
	return -EFAULT;
}

void HostSpeedUP_USAGE(void)
{
	printk("\nexample: local-ip and remote-ip is necessary! remote/local port is optional\n");
	printk("\n1.match 4 tuples \n");	
	printk("  echo \"add rip 192.168.1.40 rport 5000 lip 192.168.1.1 lport 5001\" > proc-path\n");
	printk("\n2.match 2 tuples \n");	
	printk("  echo \"add rip 192.168.1.40 lip 192.168.1.1\" > proc-path\n");

}
void dump_host_track(void)
{
	int i, idx=0;
	
	printk("------- host_track --------\n");
	for (i=0; i<10; i++)
	{
		if (host_track[i].active)
		{
			printk("%d:\n", idx++);
			printk("\tactive=%d\n", host_track[i].active);
			printk("\tdstip=%s\n", ip_inet_ntoa(host_track[i].dstip));
			printk("\tdport=%d\n", host_track[i].dport);
			printk("\tsrcip=%s\n", ip_inet_ntoa(host_track[i].srcip));
			printk("\tsport=%d\n", host_track[i].sport);
			printk("\ref=%d\n", host_track[i].ref_cnt);
		}
	}
	if (0 == idx)
		printk("host track is not active!\n");
}

int HostSpeedUP_Info_read_proc(struct seq_file *seq, void *v)
{
	unsigned int maxRate=0, rate, expectMinRate=0, averageRate=0;
	unsigned int i, idx=0;
	unsigned int tmp;
	int delta=0;

	if (sampletime > 0)
		averageRate = hostspeeduprate[sampletime]/sampletime;
	
	seq_printf(seq,"valid sample num:%d\n",sampletime);
	for(i=0; i<sampletime; i++)
	{
		tmp = hostspeeduprate[i];
		
		if (i >= 3)
		{
			rate = hostspeeduprate[i] - hostspeeduprate[i-1];
			if (rate < expectMinRate)
			{
				delta += expectMinRate - rate;
			}
			else if (rate > maxRate)
			{
				if (delta > 0)
				{
					if (delta > (rate - maxRate))
						delta -= (rate - maxRate);
					else
						delta = 0;
				}
				else
				{
					maxRate = rate;
					expectMinRate = (maxRate * 7) >> 3; //maxRate*7/8
				}
			}
		}
		
		tmp += delta;

		if (averageRate < 70400)//550Mbps
			seq_printf(seq, "%u ", tmp);
		else
			seq_printf(seq, "%u ", tmp + tmp/10);
	}
	seq_printf(seq,"\n");

	seq_printf(seq, "real sample:\n");
	for(i=0; i<sampletime; i++)
	{
		seq_printf(seq, "%u ", hostspeeduprate[i]);
	}
	seq_printf(seq,"\n");
	
	for (i=0; i<10; i++)
	{
		if (host_track[i].active)
		{
			seq_printf(seq, "%d:\n", idx++);
			seq_printf(seq, "\tdstip=%s\n", ip_inet_ntoa(host_track[i].dstip));
			seq_printf(seq, "\tdport=%d\n", host_track[i].dport);
			seq_printf(seq, "\tsrcip=%s\n", ip_inet_ntoa(host_track[i].srcip));
			seq_printf(seq, "\tsport=%d\n", host_track[i].sport);
			seq_printf(seq, "\tref=%d\n", host_track[i].ref_cnt);
			seq_printf(seq, "\tbypass=%d\n", (host_track[i].bypassFastAck | bypassFastAckFlag));
		}
	}
	if (0 == idx)
		seq_printf(seq, "host track is not active\n");
#ifdef IP_SPEEDUP_DEBUG
	seq_printf(seq, "inOrderPktCnt %d outOrderPktCnt %d duplicatePktCnt %d revisePktCnt %d delayAckPktCnt %d longDelayAckPktCnt %d\n", 
			inOrderPktCnt, outOrderPktCnt, duplicatePktCnt, revisePktCnt, delayAckPktCnt, longDelayAckPktCnt);
	for (i=0; i<11; i++)
	{
		if (softirq_delay_stats[i] != 0)
			seq_printf(seq, "softirq %d %s long delay %d times\n", i, softirq_new_name[i], softirq_delay_stats[i]);
	}
#endif
	return 0;
}

static int proc_HostSpeedUP_Info_open(struct inode *inode, struct file *file)
{
	return single_open(file, HostSpeedUP_Info_read_proc, inode->i_private);
}


int HostSpeedUP_Info_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{	
	char	tmpbuf[512]={0};
	u32 dip=0, sip=0;
	u16 dport=0, sport=0, key=0, bypass=0;
	int action;	//0-add 1-del 2-set

	if (buffer && !copy_from_user(tmpbuf, buffer, count))
	{
		char *strptr,*split_str;

		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		split_str=strsep(&strptr," ");
		/*parse command*/
		if ((strcasecmp(split_str, "add") == 0) || (strcasecmp(split_str, "set") == 0))
		{
			if (strcasecmp(split_str, "add") == 0)
				action = 0;
			else
				action = 2;
			
			while(1){
				split_str=strsep(&strptr," ");
				if(strcasecmp(split_str, "lip") == 0)
				{
					char *ip_token,*split_ip_token,j;
					split_str=strsep(&strptr," ");
					ip_token=split_str;
//printk("%s-%d dip=%s\n",__func__,__LINE__,ip_token);
					for(j=0;j<4;j++)
					{
						split_ip_token=strsep(&ip_token,".");
						dip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
						key=1;
						if(ip_token==NULL) break;
					}
				}
				else if(strcasecmp(split_str, "lport") == 0)
				{
					split_str=strsep(&strptr," ");
					dport=simple_strtol(split_str, NULL, 0);
					key=1;
//printk("%s-%d dport=%d\n",__func__,__LINE__,dport);

				}
				else if(strcasecmp(split_str, "rip") == 0)
				{
					char *ip_token,*split_ip_token,j;
					split_str=strsep(&strptr," ");
					ip_token=split_str;
//printk("%s-%d sip=%s\n",__func__,__LINE__,ip_token);
					
					for(j=0;j<4;j++)
					{
						split_ip_token=strsep(&ip_token,".");
						sip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
						key=1;
						if(ip_token==NULL) break;
					}				
				}
				else if(strcasecmp(split_str, "rport") == 0)
				{
					split_str=strsep(&strptr," ");
					sport=simple_strtol(split_str, NULL, 0);
					key=1;					
//printk("%s-%d sport=%d\n",__func__,__LINE__,sport);

				}
				else if (strcasecmp(split_str, "bypass") == 0)
				{
					split_str=strsep(&strptr," ");
					bypass = simple_strtol(split_str, NULL, 0);
				}
				if (strptr==NULL) break;
			}
		}
		else if(strcasecmp(split_str, "del") == 0)
		{
			action = 1;
			
			while(1){
				split_str=strsep(&strptr," ");
				if(strncasecmp(split_str, "info", 4) == 0)
				{
					host_track_off(0, 0, 0, 0);
					dump_host_track();
					goto err_skip_track;
				}
				else if(strcasecmp(split_str, "lip") == 0)
				{
					char *ip_token,*split_ip_token,j;
					split_str=strsep(&strptr," ");
					ip_token=split_str;
					for(j=0;j<4;j++)
					{
						split_ip_token=strsep(&ip_token,".");
						dip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
						key=1;
						if(ip_token==NULL) break;
					}
				}
				else if(strcasecmp(split_str, "lport") == 0)
				{
					split_str=strsep(&strptr," ");
					dport=simple_strtol(split_str, NULL, 0);
					key=1;
				}
				else if(strcasecmp(split_str, "rip") == 0)
				{
					char *ip_token,*split_ip_token,j;
					split_str=strsep(&strptr," ");
					ip_token=split_str;
					
					for(j=0;j<4;j++)
					{
						split_ip_token=strsep(&ip_token,".");
						sip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
						key=1;
						if(ip_token==NULL) break;
					}				
				}
				else if(strcasecmp(split_str, "rport") == 0)
				{
					split_str=strsep(&strptr," ");
					sport=simple_strtol(split_str, NULL, 0);
					key=1;
				}
				if (strptr==NULL) break;
			}
		}
		else if(strcasecmp(split_str, "dbg") == 0)
		{
			split_str=strsep(&strptr," ");
			dbgflag = simple_strtol(split_str, NULL, 0);
			printk("dbgflag %d\n", dbgflag);
			goto err_skip_track;
		}else{
			printk("error command!\n");
		}

	}
	if(!key){
		printk("you must set one rule at less\n");
		HostSpeedUP_USAGE();		
		goto err_skip_track;
	}
	if(!sip /*|| !dip*/){
		printk("you must set rip(Remote IP) or lip (Local IP)(ex: A.B.C.D=192.168.1.1)\n");
		HostSpeedUP_USAGE();		
		goto err_skip_track;

	}
	if (0 == action)//add
		host_track_on(dip, sip, dport, sport);
	else if (2 == action)//set
		host_track_set(dip, sip, dport, sport, bypass);
	else
		host_track_off(dip, sip, dport, sport);
	//dump_host_track();

	//printk("%s parse command done!\n", __func__);
	err_skip_track:
	
	return count;
}

#if 0
static void dump_host_stream(void)
{
	struct _speedup_stream_track *pEntry, *pNextEntry;
	int i, j;

	for (i=0; i<10; i++)
	{
		if (0 == host_track[i].active)
			break;

		for (j=0; j<SPEEDUP_HASH_SIZE; j++)
		{
			list_for_each_entry_safe(pEntry, pNextEntry, &host_track[i].hash[j], node)
			{
				printk("stream %d hash %d total size %d \n", i, j, pEntry->totalbytes);
				printk("	dstip=%s\n", ip_inet_ntoa(pEntry->dstip));
				printk("	dport=%d\n", pEntry->dport);
				printk("	srcip=%s\n", ip_inet_ntoa(pEntry->srcip));
				printk("	sport=%d\n", pEntry->sport);
				printk("	id=%d\n", pEntry->id);
				printk("	seq=%x\n", pEntry->seqno);
			}
		}
	}
}
#endif

static int HostSpeedUP_Stream_read_proc(struct seq_file *seq, void *v)
{
	struct _speedup_stream_track *pEntry;
	int i, j;

	for (i=0; i<10; i++)
	{
		if (0 == host_track[i].active)
			break;

		for (j=0; j<SPEEDUP_HASH_SIZE; j++)
		{
			list_for_each_entry(pEntry, &host_track[i].hash[j], node)
			{
				seq_printf(seq, "stream %d hash %d total size %d \n", i, j, pEntry->totalbytes);
				seq_printf(seq, "	dstip=%s\n", ip_inet_ntoa(pEntry->dstip));
				seq_printf(seq, "	dport=%d\n", pEntry->dport);
				seq_printf(seq, "	srcip=%s\n", ip_inet_ntoa(pEntry->srcip));
				seq_printf(seq, "	sport=%d\n", pEntry->sport);
				seq_printf(seq, "	id=%d\n", pEntry->id);
				seq_printf(seq, "	seq=%x\n", pEntry->seqno);
			}
		}
	}

	return 0;
}

static int proc_HostSpeedUP_Stream_open(struct inode *inode, struct file *file)
{
	return single_open(file, HostSpeedUP_Stream_read_proc, inode->i_private);
}

/* echo get/set sip %%s sport %%d dip %%s dport %%s > HostSpeedUP-STREAM */
int HostSpeedUP_Stream_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char tmpbuf[512]={0};
	u32 dip=0, sip=0;
	u16 dport=0, sport=0, key=0;
	int action=0;

	if (buffer && !copy_from_user(tmpbuf, buffer, count))
	{
		char *strptr,*split_str;

		//printk("\n[%s cpu %d] %s\n", __func__, smp_processor_id(), tmpbuf);
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		split_str=strsep(&strptr," ");
		/*parse command*/
		if (strcasecmp(split_str, "add") == 0)
		{
			action = SPEEDUP_STREAM_ADD;
		}
		else if (strcasecmp(split_str, "del") == 0)
		{
			action = SPEEDUP_STREAM_DEL;
		}
		else
		{
			printk("%s error command!\n", __func__);
			goto err_skip_track;
		}
		
		while(1)
		{
			split_str=strsep(&strptr," ");
			if(strcasecmp(split_str, "sip") == 0)
			{
				char *ip_token,*split_ip_token,j;
				split_str=strsep(&strptr," ");
				ip_token=split_str;
				//printk("%s-%d sip=%s\n",__func__,__LINE__,ip_token);
				for(j=0;j<4;j++)
				{
					split_ip_token=strsep(&ip_token,".");
					sip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
					key=1;
					if(ip_token==NULL) break;
				}
			}
			else if(strcasecmp(split_str, "sport") == 0)
			{
				split_str=strsep(&strptr," ");
				sport=simple_strtol(split_str, NULL, 0);
				key=1;
				//printk("%s-%d sport=%d\n",__func__,__LINE__,sport);
			}
			else if(strcasecmp(split_str, "dip") == 0)
			{
				char *ip_token,*split_ip_token,j;
				split_str=strsep(&strptr," ");
				ip_token=split_str;
				//printk("%s-%d dip=%s\n",__func__,__LINE__,ip_token);
				
				for(j=0;j<4;j++)
				{
					split_ip_token=strsep(&ip_token,".");
					dip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
					key=1;
					if(ip_token==NULL) break;
				}
			}
			else if(strcasecmp(split_str, "dport") == 0)
			{
				split_str=strsep(&strptr," ");
				dport=simple_strtol(split_str, NULL, 0);
				key=1;					
				//printk("%s-%d dport=%d\n",__func__,__LINE__,dport);
			}
			if (strptr==NULL) break;
		}
	}
	
	if(!key){
		printk("you must set one rule at less\n");
		printk("e.g\n");
		printk("echo set sip 192.168.2.1 sport 2000 dip 10.10.10.10 dport 80 total 2000000 > HostSpeedUP-STREAM\n");		
		goto err_skip_track;
	}

#if 0
	printk("------- host_stream_track --------\n");
	printk("dstip=%s\n", ip_inet_ntoa(dip));
	printk("dport=%d\n", dport);
	printk("srcip=%s\n", ip_inet_ntoa(sip));
	printk("sport=%d\n", sport);
#endif
	if (SPEEDUP_STREAM_ADD == action)
		_speedup_stream_track_record(dip, sip, dport, sport);
	else if (SPEEDUP_STREAM_DEL == action)
		_speedup_stream_track_del(dip, sip, dport, sport);

	//printk("%s parse command done!\n", __func__);
	//dump_host_stream();
err_skip_track:
	
	return count;
}

static const struct file_operations hostspeedup_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_HostSpeedUP_open,
        .read           = seq_read,
        .write          = HostSpeedUP_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static const struct file_operations hostspeedup_info_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_HostSpeedUP_Info_open,
        .read           = seq_read,
        .write          = HostSpeedUP_Info_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static const struct file_operations hostspeedup_stream_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_HostSpeedUP_Stream_open,
        .read           = seq_read,
        .write          = HostSpeedUP_Stream_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static void setup_Timer_for_speedup(void)
{
	int offset;

	printk("====   SPEEDTEST TIMER %d   =====\n", SPEEDTEST_TIMER_IDX);
	
	offset = SPEEDTEST_TIMER_IDX * 0x10;
	/* disable timer */
	REG32(BSP_TC0CTL + offset) = 0x0; /* disable timer before setting CDBR */
	/* initialize timer registers */
	REG32(BSP_TC0CTL + offset) |= (200) << 0;	//1us
	REG32(BSP_TC0DATA + offset) = 0xFFFFFFF;
	/* enable timer */
	REG32(BSP_TC0CTL + offset) |= BSP_TCEN | BSP_TCMODE_TIMER;
	REG32(BSP_TC0INT + offset) &= ~BSP_TCIE;

	printk("--------------------------------\n");
	printk("BSP_CTRLR(%08x): %x\n", BSP_TC0CTL + offset, REG32(BSP_TC0CTL + offset));
	printk("BSP_DATAR(%08x): %x\n", BSP_TC0DATA + offset, REG32(BSP_TC0DATA + offset));
	printk("BSP_INTRR(%08x): %x\n", BSP_TC0INT + offset, REG32(BSP_TC0INT + offset));
	printk("--------------------------------\n");
}

int HostSpeedUP_proc_init(void)
{
	struct proc_dir_entry *entry=NULL;

    _host_track_init();
	setup_Timer_for_speedup();

	entry = proc_create_data("HostSpeedUP", 0644, NULL, &hostspeedup_fops,NULL);
	if (entry == NULL)
	{
		printk("%s-%d create proc HostSpeedUP fail!\n", __func__, __LINE__);	
		remove_proc_entry("HostSpeedUP", NULL);
		return -ENOMEM;
	}
	
	entry = proc_create_data("HostSpeedUP-Info", 0644, NULL, &hostspeedup_info_fops,NULL);
	if (entry == NULL)
	{
		printk("%s-%d create proc HostSpeedUP-Info fail!\n", __func__, __LINE__);		
		remove_proc_entry("HostSpeedUP-Info", NULL);
		return -ENOMEM;
	}

	entry = proc_create_data("HostSpeedUP-STREAM", 0644, NULL, &hostspeedup_stream_fops,NULL);
	if (entry == NULL)
	{
		printk("%s-%d create proc HostSpeedUP-STREAM fail!\n", __func__, __LINE__);		
		remove_proc_entry("HostSpeedUP-STREAM", NULL);
		return -ENOMEM;
	}

	return 0;
}

int HostSpeedUP_proc_exit(void)
{

	return 0;
}






