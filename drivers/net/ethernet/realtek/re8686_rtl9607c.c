/* re8670.c: A Linux Ethernet driver for the RealTek 8670 chips. */
/*
   Copyright 2001,2002 Jeff Garzik <jgarzik@mandrakesoft.com>

   Copyright (C) 2001, 2002 David S. Miller (davem@redhat.com) [tg3.c]
   Copyright (C) 2000, 2001 David S. Miller (davem@redhat.com) [sungem.c]
   Copyright 2001 Manfred Spraul				    [natsemi.c]
   Copyright 1999-2001 by Donald Becker.			    [natsemi.c]
   Written 1997-2001 by Donald Becker.			    [8139too.c]
   Copyright 1998-2001 by Jes Sorensen, <jes@trained-monkey.org>. [acenic.c]

   This software may be used and distributed according to the terms of
   the GNU General Public License (GPL), incorporated herein by reference.
   Drivers based on or derived from this code fall under the GPL and must
   retain the authorship, copyright and license notice.  This file is not
   a complete program and may only be used when the entire operating
   system is licensed under the GPL.

   See the file COPYING in this distribution for more information.

   TODO, in rough priority order:
 * dev->tx_timeout
 * LinkChg interrupt
 * Support forcing media type with a module parameter,
 like dl2k.c/sundance.c
 * Implement PCI suspend/resume
 * Constants (module parms?) for Rx work limit
 * support 64-bit PCI DMA
 * Complete reset on PciErr
 * Consider Rx interrupt mitigation using TimerIntr
 * Implement 8139C+ statistics dump; maybe not...
 h/w stats can be reset only by software reset
 * Tx checksumming
 * Handle netif_rx return value
 * ETHTOOL_GREGS, ETHTOOL_[GS]WOL,
 * Investigate using skb->priority with h/w VLAN priority
 * Investigate using High Priority Tx Queue with skb->priority
 * Adjust Rx FIFO threshold and Max Rx DMA burst on Rx FIFO error
 * Adjust Tx FIFO threshold and Max Tx DMA burst on Tx FIFO error
 * Implement Tx software interrupt mitigation via
 Tx descriptor bit
 * The real minimum of CP_MIN_MTU is 4 bytes.  However,
 for this to be supported, one must(?) turn on packet padding.

*/

#define DRV_NAME		"8686"
#define DRV_VERSION		"0.0.1"
#define DRV_RELDATE		"Aug 17, 2016"
#ifdef CONFIG_DUALBAND_CONCURRENT
#define DRV_DUALBAND	"Dual Band Enable"
#else
#define DRV_DUALBAND	"Dual Band Disable"
#endif


#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/in.h>
#include <linux/if_vlan.h>
#include <linux/crc32.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <net/xfrm.h>
#include <linux/proc_fs.h>

#ifdef CONFIG_RTL8672	//shlee 2.6
//#ifndef CONFIG_APOLLO_ROMEDRIVER
#include <bspchip.h>
//#endif
#include <linux/version.h>
#endif
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB
#include "re_privskb.h"
#include "re_privskb_rg.h"
#endif
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
#include "re_privskb_adv.h"
#include "brg_shortcut.h"
#endif

#include "ethctl_implement.h"
#include "re8686_rtl9607c.h"

#undef __IRAM_NIC
#define __IRAM_NIC __attribute__ ((__section__(".iram-fwd")))

#ifdef CONFIG_RTK_HOST_SPEEDUP
#include "../../net/ipv4/ip_speedup.h"
#include <net/tcp.h>
#endif

#define SMBSHORTCUT
#ifdef SMBSHORTCUT
unsigned int smbserverip = 0xc0a80101;
unsigned char smbscflag = 0x0;
#endif

#ifdef CONFIG_RTL8686_SWITCH
#include "rtl86900/sdk/include/dal/apollo/raw/apollo_raw_port.h"
#include <rtk/l2.h>
#include <dal/apollomp/dal_apollomp_l2.h>
#include <rtk/switch.h>
#include <rtk/acl.h>
#include <rtk/trap.h>
#include <rtk/gponv2.h> 
#include <common/type.h>
#include <rtk/intr.h>
#include <module/intr_bcaster/intr_bcaster.h>
#endif

#ifdef CONFIG_RG_DEBUG
#include <rtk_rg_profile.h>
#else
#define MIPS_PERF_NIC_RX 0
inline void mips_perf_measure_entrance(int index)
{
}

inline void mips_perf_measure_exit(int index)
{
}
#endif

#if defined(CONFIG_DUALBAND_CONCURRENT) && defined(CONFIG_VIRTUAL_WLAN_DRIVER)	
extern int vwlan_open(struct net_device *dev);
extern int vwlan_close(struct net_device *dev);
extern int vwlan_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
extern int vwlan_set_hwaddr(struct net_device *dev, void *addr);
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
extern int rtk_rg_fc_egress_flowLearning(struct sk_buff *skb, struct net_device *dev, int portMask);
#endif

#if defined(CONFIG_APOLLO_ROMEDRIVER) || defined(CONFIG_RG_SIMPLE_PROTOCOL_STACK)
int re8670_start_xmit_check(struct sk_buff *skb, struct net_device *dev);
#elif defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int re8670_start_xmit_flowCache(struct sk_buff *skb, struct net_device *dev);
#endif
/*static*/ int re8670_start_xmit (struct sk_buff *skb, struct net_device *dev);

/* Jonah + for FASTROUTE */
struct net_device *eth_net_dev[MAX_GMAC_NUM];
struct tasklet_struct *eth_rx_tasklets[MAX_GMAC_NUM]={0};

#define WITH_NAPI		""

/* These identify the driver base version and may not be removed. */
static char version[] = KERN_INFO DRV_NAME " Ethernet driver v" DRV_VERSION " (" DRV_RELDATE ")["DRV_DUALBAND"]" WITH_NAPI "\n";

MODULE_AUTHOR("Sam Hsu <sam_hsu@realtek.com>");
MODULE_DESCRIPTION("RealTek RTL-8686 series 10/100/1000 Ethernet driver");
MODULE_LICENSE("GPL");

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
   The RTL chips use a 64 element hash table based on the Ethernet CRC.  */
static int multicast_filter_limit = 32;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)	//shlee 2.6
module_param(multicast_filter_limit, int, S_IRUGO|S_IWUSR);
#else
MODULE_PARM (multicast_filter_limit, "i");
#endif
MODULE_PARM_DESC (multicast_filter_limit, "8686 maximum number of filtered multicast addresses");

#define PFX			DRV_NAME ": "
#define CP_DEF_MSG_ENABLE	(NETIF_MSG_DRV		| \
		NETIF_MSG_PROBE 	| \
		NETIF_MSG_LINK)
#define CP_REGS_SIZE		(0xff + 1)

#define DESC_ALIGN		0x100
#define UNCACHE_MASK		0xa0000000
/*add 1 desc for dummy desc : we will allocate one more desc even if the RING_SIZE is 0, 
this can let the ring which we disable has a dummy desc in its FDP, RD says our NIC need this...*/
#define RE8670_RXRING_BYTES(RING_SIZE)	( (sizeof(struct dma_rx_desc) * (RING_SIZE+1)) + DESC_ALIGN)
#define RE8670_TXRING_BYTES(RING_SIZE)	( (sizeof(struct dma_tx_desc) * (RING_SIZE+1)) + DESC_ALIGN)

#define NEXT_TX(N,RING_SIZE)		(((N) + 1) & (RING_SIZE - 1))
#define NEXT_RX(N,RING_SIZE)		(((N) + 1) & (RING_SIZE - 1))

#define NOT_IN_BITMAP(map, num) (!((map)&(1<<(num))))

static inline int idx_sw2hw(int ring_num) {
	return (MAX_TXRING_NUM-1)-ring_num;
}

#define RX_MRING_INT_SPLIT
//#undef RX_MRING_INT_SPLIT

#define TX_HQBUFFS_AVAIL(CP,ring_num)					\
		(((CP)->tx_Mhqtail[ring_num] - (CP)->tx_Mhqhead[ring_num] + re8670_tx_ring_size[(CP)->gmac][ring_num] - 1)&(re8670_tx_ring_size[(CP)->gmac][ring_num] - 1))		

//#define PKT_BUF_SZ		1536	/* Size of each temporary Rx buffer.*/
#ifndef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
#define RX_OFFSET		2	//move to re_privskb_adv.h
#endif

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024, 7==end of packet. */
/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT		(3*HZ)
/* hardware minimum and maximum for a single frame's data payload */
#define CP_MIN_MTU		60	/* TODO: allow lower, but pad */
#define CP_MAX_MTU		4096

#define TOTAL_RTL8686_DEV_NUM	((sizeof(rtl8686_dev_table))/(sizeof(struct rtl8686_dev_table_entry)))

#define RLE0787_W32(idx, reg, value)		(*(volatile u32*)(gmac_eth_base[idx]+reg)) = (u32)value
#define RLE0787_W16(idx, reg, value)		(*(volatile u16*)(gmac_eth_base[idx]+reg)) = (u16)value
#define RLE0787_W8(idx, reg, value)			(*(volatile u8*)(gmac_eth_base[idx]+reg)) = (u8)value
#define RLE0787_R32(idx, reg)				(*(volatile u32*)(gmac_eth_base[idx]+reg))
#define RLE0787_R16(idx, reg)				(*(volatile u16*)(gmac_eth_base[idx]+reg))
#define RLE0787_R8(idx, reg)				(*(volatile u8*)(gmac_eth_base[idx]+reg))


#ifdef RX_MRING_INT_SPLIT/*plz add into both define and not define area*/
#define en_rx_mring_int_split(idx) RLE0787_W32(idx, CONFIG_REG, (RLE0787_R32(idx, CONFIG_REG) | En_int_split))
#ifdef CONFIG_DUALBAND_CONCURRENT
//#define set_rring_route() RTL_W32(RRING_ROUTING1, 0x65442100);
#define MASTER_GMAC_PRI_TO_RING ((0x33222111&(~(0xf<<(CONFIG_DEFAULT_TO_SLAVE_GMAC_PRI<<2))))|(4<<(CONFIG_DEFAULT_TO_SLAVE_GMAC_PRI<<2)))
#define SLAVE_GMAC_PRI_TO_RING (0x66555444)
#define set_rring_route(idx) {RLE0787_W32(idx, RRING_ROUTING1, MASTER_GMAC_PRI_TO_RING);RLE0787_W32(idx, RRING_ROUTING2, MASTER_GMAC_PRI_TO_RING);RLE0787_W32(idx, RRING_ROUTING3, SLAVE_GMAC_PRI_TO_RING);RLE0787_W32(idx, RRING_ROUTING4, MASTER_GMAC_PRI_TO_RING);RLE0787_W32(idx, RRING_ROUTING5, MASTER_GMAC_PRI_TO_RING);RLE0787_W32(idx, RRING_ROUTING6, MASTER_GMAC_PRI_TO_RING);}
#else
#define set_rring_route(idx) {RLE0787_W32(idx, RRING_ROUTING1, 0x65432111);RLE0787_W32(idx, RRING_ROUTING2, 0x65432111);RLE0787_W32(idx, RRING_ROUTING3, 0x65432111);RLE0787_W32(idx, RRING_ROUTING4, 0x65432111);RLE0787_W32(idx, RRING_ROUTING5, 0x65432111);RLE0787_W32(idx, RRING_ROUTING6, 0x65432111);RLE0787_W32(idx, RRING_ROUTING7, 0x65432111);}
#endif

#define retrive_isr1_status(idx) RLE0787_R32(idx, ISR1)
#define assigne_cpisr_status(x, y) (x) = (y)
#define assigne_cpisr1_status(x, y) (x) = (y)
#define gather_rx_isr(x,y) (x) = (x)|(!!(y & 0x3f))
#define CLEAR_ISR1(idx, x) RLE0787_W32(idx, ISR1, (x))
#define MASK_IMR0_RXALL(idx) RLE0787_R32(idx, IMR0)&=~(rx_multiring_bitmap[idx])
#define UNMASK_IMR0_RXALL(idx) RLE0787_R32(idx, IMR0)|=(rx_multiring_bitmap[idx])
#else
#define en_rx_mring_int_split(idx)
#define set_rring_route(idx)
#define retrive_isr1_status(idx) 0
#define assigne_cpisr_status(x, y)
#define assigne_cpisr1_status(x, y)
#define gather_rx_isr(x, y)
#define CLEAR_ISR1(idx, x) 
#define MASK_IMR0_RXALL(idx)
#define UNMASK_IMR0_RXALL(idx)
#endif

struct ring_info {
	struct sk_buff		*skb;
	dma_addr_t		mapping;
	unsigned		frag;
};

static struct re_private re_private_data[MAX_GMAC_NUM];
int dev_num=0;

#ifndef CONFIG_RG_SIMPLE_PROTOCOL_STACK
struct net_device *nicRootDev;
struct re_private *nicRootDevCp;
#endif

#define DEV2CP(dev)  (((struct re_dev_private*)dev->priv)->pCp)
#define DEVPRIV(dev)  ((struct re_dev_private*)dev->priv)
#define VTAG2DESC(d) (((((d) & 0x00ff)<<8) | (((d) & 0xff00)>>8)) & 0x0000ffff)
/*warning! +1 for smux.................................................*/
#define VTAG2VLANTCI(v) (( (((v) & 0xff00)>>8) | (((v) & 0x00ff)<<8) ) + 1) 

static void __re8670_set_rx_mode (struct net_device *dev);
static void re8670_tx (struct re_private *cp,int ring_num);

static void re8670_clean_rings (struct re_private *cp);
static void re8670_tx_timeout (struct net_device *dev);
static int change_dev_port_mapping(int port_num ,char* name);
static int change_tx_always_device(char* name);

/*================================================
			GMAC used Global Variable
================================================*/
static const unsigned int gmac_enabled[MAX_GMAC_NUM] = {ON, 
	#ifdef CONFIG_GMAC1_USABLE	
	ON, 
	#else
	OFF,
	#endif 
	#ifdef CONFIG_GMAC2_USABLE
	ON
	#else
	OFF
	#endif
	};
static const unsigned int gmac_eth_base[MAX_GMAC_NUM] = {GMAC0_ETHBASE, GMAC1_ETHBASE, GMAC2_ETHBASE};
static const unsigned int gmac_cpu_port[MAX_GMAC_NUM] = {CPU_PORT0, CPU_PORT1, CPU_PORT2};

static const unsigned int re8670_rx_ring_size[MAX_GMAC_NUM][MAX_RXRING_NUM] = 
{
	{GMAC0_RX1_SIZE, GMAC0_RX2_SIZE, GMAC0_RX3_SIZE, GMAC0_RX4_SIZE, GMAC0_RX5_SIZE, GMAC0_RX6_SIZE},
	{GMAC1_RX1_SIZE, GMAC1_RX2_SIZE, GMAC1_RX3_SIZE, GMAC1_RX4_SIZE, GMAC1_RX5_SIZE, GMAC1_RX6_SIZE},
	{GMAC2_RX1_SIZE, GMAC2_RX2_SIZE, GMAC2_RX3_SIZE, GMAC2_RX4_SIZE, GMAC2_RX5_SIZE, GMAC2_RX6_SIZE}
};

static const unsigned int re8670_tx_ring_size[MAX_GMAC_NUM][MAX_TXRING_NUM] = 
{
	{GMAC0_TX1_SIZE, GMAC0_TX2_SIZE, GMAC0_TX3_SIZE, GMAC0_TX4_SIZE, GMAC0_TX5_SIZE},
	{GMAC1_TX1_SIZE, GMAC1_TX2_SIZE, GMAC1_TX3_SIZE, GMAC1_TX4_SIZE, GMAC1_TX5_SIZE},
	{GMAC2_TX1_SIZE, GMAC2_TX2_SIZE, GMAC2_TX3_SIZE, GMAC2_TX4_SIZE, GMAC2_TX5_SIZE}
};

static unsigned char re8670_rx_flow_control_status[MAX_GMAC_NUM][MAX_RXRING_NUM] = 
{
	{ON, ON, ON, ON, ON, ON},
	{ON, ON, ON, ON, ON, ON},
	{ON, ON, ON, ON, ON, ON}
};

static const unsigned int rx_multiring_bitmap[MAX_GMAC_NUM] = 
{
	GMAC0_RX_MULTIRING_BITMAP,
	GMAC1_RX_MULTIRING_BITMAP,
	GMAC2_RX_MULTIRING_BITMAP
};

static const unsigned int tx_multiring_bitmap[MAX_GMAC_NUM] = 
{
	GMAC0_TX_MULTIRING_BITMAP,
	GMAC1_TX_MULTIRING_BITMAP,
	GMAC2_TX_MULTIRING_BITMAP
};

static const unsigned int rx_only_ring1[MAX_GMAC_NUM] = 
{
	GMAC0_RX_ONLY_RING1,
	GMAC1_RX_ONLY_RING1,
	GMAC2_RX_ONLY_RING1
};

static const unsigned int rx_not_only_ring1[MAX_GMAC_NUM] = 
{
	GMAC0_RX_NOT_ONLY_RING1,
	GMAC1_RX_NOT_ONLY_RING1,
	GMAC2_RX_NOT_ONLY_RING1
};

//int re8670_rxskb_num=0;

static unsigned int iocmd_reg[MAX_GMAC_NUM] = {CMD_CONFIG, CMD_CONFIG, CMD_CONFIG}; // =0x4009113d;	//shlee 8672
#ifdef CONFIG_DUALBAND_CONCURRENT
static unsigned int iocmd1_reg[MAX_GMAC_NUM] = 
{
	CMD1_CONFIG | (GMAC0_RX_NOT_ONLY_RING1<<25) | TX_RR_scheduler | 0x3f<<16  | txq1_h,
	CMD1_CONFIG | (GMAC1_RX_NOT_ONLY_RING1<<25) | TX_RR_scheduler | 0x3f<<16  | txq1_h,
	CMD1_CONFIG | (GMAC2_RX_NOT_ONLY_RING1<<25) | TX_RR_scheduler | 0x3f<<16  | txq1_h
}; // set RR scheduler and tx ring 1 high queue
#else
static unsigned int iocmd1_reg[MAX_GMAC_NUM] = 
{
	CMD1_CONFIG | (GMAC0_RX_NOT_ONLY_RING1<<25) | GMAC0_RX_MULTIRING_BITMAP << 16 | txq1_h,
	CMD1_CONFIG | (GMAC1_RX_NOT_ONLY_RING1<<25) | GMAC1_RX_MULTIRING_BITMAP << 16 | txq1_h,
	CMD1_CONFIG | (GMAC2_RX_NOT_ONLY_RING1<<25) | GMAC2_RX_MULTIRING_BITMAP << 16 | txq1_h
}; // set TX ring 1 to high queue
#endif

static unsigned int tx_ring_show_bitmap[MAX_GMAC_NUM] =
{
	((1<<MAX_TXRING_NUM)-1),
	((1<<MAX_TXRING_NUM)-1),
	((1<<MAX_TXRING_NUM)-1)
};
static unsigned int rx_ring_show_bitmap[MAX_GMAC_NUM] =
{
	((1<<MAX_RXRING_NUM)-1),
	((1<<MAX_RXRING_NUM)-1),
	((1<<MAX_RXRING_NUM)-1)
};

extern atomic_t re8670_rxskb_num;

unsigned int txAlwaysGmac = 0;

unsigned int txJumboFrameEnabled[MAX_GMAC_NUM] = {1, 1, 1};

/*__DRAM*/ unsigned int debug_enable[MAX_GMAC_NUM]={0};
unsigned int padding_enable[MAX_GMAC_NUM]={0, 0, 0};

#ifdef CONFIG_RG_SIMPLE_PROTOCOL_STACK
static struct rtl8686_dev_table_entry rtl8686_dev_table[] = {
	//ifname, ifflag, vid, phyPort, dev_instant
	{"eth0",		RTL8686_ELAN, 0, CPU_PORT0, 0, NULL}, // root dev eth0 must be first
	{"eth1",		RTL8686_ELAN, 0, CPU_PORT1, 0, NULL}, // dev eth1 will be second	
	{"eth2",		RTL8686_ELAN, 0, CPU_PORT2, 0, NULL}, // dev eth2 will be third	
	{"eth0.2",		RTL8686_ELAN, 0, LAN_PORT1, 1, NULL},
	{"eth0.3",		RTL8686_ELAN, 0, LAN_PORT2, 1, NULL},
	{"eth0.4",		RTL8686_ELAN, 0, LAN_PORT3, 1, NULL},
	{"eth0.5",		RTL8686_ELAN, 0, LAN_PORT4, 1, NULL},
	{"eth0.6",		RTL8686_ELAN, 0, LAN_PORT5, 1, NULL},
	{"eth0.7",		RTL8686_ELAN, 0, LAN_PORT6, 1, NULL},
	{"eth",			RTL8686_WAN, 0, WAN_PORT, 0, NULL},	// 1
	{"eth", 		RTL8686_WAN, 0, WAN_PORT, 0, NULL},	// 2
	{"eth", 		RTL8686_WAN, 0, WAN_PORT, 0, NULL},	// 3
	{"eth", 		RTL8686_WAN, 0, WAN_PORT, 0, NULL},	// 4
//	{"eth", 		RTL8686_WAN, 0, WAN_PORT, 0, NULL},	// 5
//	{"eth", 		RTL8686_WAN, 0, WAN_PORT, 0, NULL},	// 6
//	{"eth", 		RTL8686_WAN, 0, WAN_PORT, 0, NULL},	// 7
	{"wan", 		RTL8686_WAN, 0, WAN_PORT, 0, NULL},	// represent as WAN interface for port mapping 
};
#else
/*Krammer: this table will re-map after port_relate_setting if is 0371*/
#ifdef CONFIG_DUALBAND_CONCURRENT
static int wlan1_dev_idx=7;
static int vwlan_dev_idx=8;
static unsigned char src_mac[ETH_ALEN];
#endif

static struct rtl8686_dev_table_entry rtl8686_dev_table[] = {	
	//ifname, ifflag, vid, phyPort, dev_instant
	{"eth0",		RTL8686_ELAN, 0, CPU_PORT0, 0, NULL}, // root dev eth0 must be first
	{"eth1",		RTL8686_ELAN, 0, CPU_PORT1, 0, NULL}, // dev eth1 will be second	
	{"eth2",		RTL8686_ELAN, 0, CPU_PORT2, 0, NULL}, // dev eth2 will be third	
	{"eth0.2",		RTL8686_ELAN, 0, LAN_PORT1, 1, NULL},
	{"eth0.3",		RTL8686_ELAN, 0, LAN_PORT2, 1, NULL},
	{"eth0.4",		RTL8686_ELAN, 0, LAN_PORT3, 1, NULL},
	{"eth0.5",		RTL8686_ELAN, 0, LAN_PORT4, 1, NULL},
	{"eth0.6",		RTL8686_ELAN, 0, LAN_PORT5, 1, NULL},
	{"eth0.7",		RTL8686_ELAN, 0, LAN_PORT6, 1, NULL},
	{"nas",			RTL8686_WAN, 0, WAN_PORT, 0, NULL},
	{"pon",			RTL8686_WAN, 0, WAN_PORT, 0, NULL},
#ifdef CONFIG_DUALBAND_CONCURRENT
	{"wlan1",		RTL8686_WLAN, 0, CPU_PORT0, 0, NULL}, // must modify wlan1_dev_idx
	{"vwlan",		RTL8686_WLAN, 0, CPU_PORT0, 0, NULL}, // must modify vwlan_dev_idx
#ifdef CONFIG_RTL_VAP_SUPPORT
	{"wlan1-vap0",	RTL8686_SVAP, 0, CPU_PORT0, 0, NULL},
	{"wlan1-vap1",	RTL8686_SVAP, 0, CPU_PORT0, 0, NULL},
	{"wlan1-vap2",	RTL8686_SVAP, 0, CPU_PORT0, 0, NULL},
	{"wlan1-vap3",	RTL8686_SVAP, 0, CPU_PORT0, 0, NULL},
#endif
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
	{"wlan1-vxd",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
#endif
#ifdef CONFIG_RTL_WDS_SUPPORT
	{"wlan1-wds0",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
	{"wlan1-wds1",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
	{"wlan1-wds2",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
	{"wlan1-wds3",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
	{"wlan1-wds4",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
	{"wlan1-wds5",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
	{"wlan1-wds6",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
	{"wlan1-wds7",	RTL8686_WLAN, 0, CPU_PORT0, 0, NULL},
#endif
#endif
};
#endif

#ifdef CONFIG_RTL8686_SWITCH
struct netif_carrier_dev_mapping
{
	unsigned char 	ifname[IFNAMSIZ];
	struct net_device *phy_dev;
	unsigned char status; // 0 : disable , 1 : enable
	int linkchangetimes;
};

// global variable for link change interrupt device mapping

struct netif_carrier_dev_mapping LCDev_mapping[SW_PORT_NUM];

unsigned char pauseBySwRingBitmap[MAX_GMAC_NUM] = {0,0,0};
unsigned char pauseBySwGmacBitmap = 0;
unsigned char pauseBySwEnable = OFF;
struct timer_list software_interrupt_timer;

static void gmacintr_notifier_link_change(intrBcasterMsg_t	*pMsgData)
{
#if 0
    printk("intrType	= %d\n", pMsgData->intrType);
    printk("intrSubType	= %u\n", pMsgData->intrSubType);
    printk("intrBitMask	= %u\n", pMsgData->intrBitMask);
    printk("intrStatus	= %d\n", pMsgData->intrStatus);
#endif

	if(!IS_LAN_PORT(pMsgData->intrBitMask))
		return;

	if(pMsgData->intrType == MSG_TYPE_LINK_CHANGE) // Link Change Interrupt
	{
		if(pMsgData->intrBitMask < SW_PORT_NUM && pMsgData->intrBitMask >= 0)
		{
			if(pMsgData->intrSubType == 1 && LCDev_mapping[pMsgData->intrBitMask].status == 0) // Link Up
			{
				if (!netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_on(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 1;
			}
			else if (pMsgData->intrSubType == 2 && LCDev_mapping[pMsgData->intrBitMask].status == 1) //Link Down 
			{
				if (netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_off(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 0;
			}
			else printk("something error ?!\n");
			
		}
	}
}
// Handle Link Change Interrupt for Netlink 
static intrBcasterNotifier_t GMAClinkChangeNotifier = {
    .notifyType = MSG_TYPE_LINK_CHANGE,
    .notifierCb = gmacintr_notifier_link_change,
};

#if defined(CONFIG_ETHWAN_USE_USB_SGMII) || defined(CONFIG_ETHWAN_USE_PCIE1_SGMII)
static void gmacintr_notifier_ethwan_link_change(intrBcasterMsg_t	*pMsgData)
{
#if 0
    printk("intrType	= %d\n", pMsgData->intrType);
    printk("intrSubType	= %u\n", pMsgData->intrSubType);
    printk("intrBitMask	= %u\n", pMsgData->intrBitMask);
    printk("intrStatus	= %d\n", pMsgData->intrStatus);
#endif

	if(!IS_WAN_PORT(pMsgData->intrBitMask))
		return;

	if(pMsgData->intrType == MSG_TYPE_LINK_CHANGE) // Link Change Interrupt
	{
		if(pMsgData->intrBitMask < SW_PORT_NUM && pMsgData->intrBitMask >= 0)
		{
			if(pMsgData->intrSubType == 1 && LCDev_mapping[pMsgData->intrBitMask].status == 0) // Link Up
			{
				if (!netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_on(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 1;
			}
			else if (pMsgData->intrSubType == 2 && LCDev_mapping[pMsgData->intrBitMask].status == 1) //Link Down 
			{
				if (netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_off(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 0;
			}
			else printk("something error ?!\n");
			
		}
	}
}
// Handle Link Change Interrupt for Netlink 
static intrBcasterNotifier_t GMACethwanStateChangeNotifier = {
    .notifyType = MSG_TYPE_LINK_CHANGE,
    .notifierCb = gmacintr_notifier_ethwan_link_change,
};
#else
static void gmacintr_notifier_onu_state_change(intrBcasterMsg_t	*pMsgData)
{
#if 0
    printk("intrType	= %d\n", pMsgData->intrType);
    printk("intrSubType	= %u\n", pMsgData->intrSubType);
    printk("intrBitMask	= %u\n", pMsgData->intrBitMask);
    printk("intrStatus	= %d\n", pMsgData->intrStatus);
#endif

	if(!IS_PON_PORT(pMsgData->intrBitMask))
		return;
	
	if(pMsgData->intrType == MSG_TYPE_ONU_STATE) // ONU state Change event
	{
		if(pMsgData->intrBitMask < SW_PORT_NUM && pMsgData->intrBitMask >= 0)
		{
			if(pMsgData->intrSubType == GPON_STATE_O5 && LCDev_mapping[pMsgData->intrBitMask].status == 0) // Link Up
			{
				if (!netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_on(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 1;
			}
			else if (pMsgData->intrSubType != GPON_STATE_O5 && LCDev_mapping[pMsgData->intrBitMask].status == 1) //Link Down 
			{
				if (netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_off(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 0;
			}			
		}
	}
}
// Handle Link Change Interrupt for Netlink 
static intrBcasterNotifier_t GMAConuStateChangeNotifier = {
    .notifyType = MSG_TYPE_ONU_STATE,
    .notifierCb = gmacintr_notifier_onu_state_change,
};
#endif
#endif
static int re8686_set_mac_addr(struct net_device *dev, void *addr_p)
{
	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;
	struct sockaddr *addr = addr_p;

	if (netif_running(dev))
        return -EBUSY;

    if (!is_valid_ether_addr(addr->sa_data))
    	return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
	RLE0787_W32(gmac, IDR0 , (dev->dev_addr[0] << 24) | (dev->dev_addr[1] << 16) | 
			(dev->dev_addr[2] << 8) | (dev->dev_addr[3] << 0));
	RLE0787_W32(gmac, IDR4 , (dev->dev_addr[4] << 24) | (dev->dev_addr[5] << 16));

    return 0;
}

int change_rx_sideband_setup(unsigned int gmac, int enabled)
{
	if(enabled)
	{
		RLE0787_W32(gmac, CONFIG_REG, RLE0787_R32(gmac, CONFIG_REG)|(1<<22)|(1<<23));
	}
	else
	{
		RLE0787_W32(gmac, CONFIG_REG, RLE0787_R32(gmac, CONFIG_REG)&~((1<<22)|(1<<23)));
	}
}

int change_tx_jumbo_setup(unsigned int gmac, int enabled)
{
	if(enabled)
	{
		RLE0787_W32(gmac, TCR, RLE0787_R32(gmac, TCR)|(1<<15));
		RLE0787_W32(gmac, IO_CMD, RLE0787_R32(gmac, IO_CMD)|(1<<28));
		RLE0787_W32(gmac, CONFIG_REG, RLE0787_R32(gmac, CONFIG_REG)|(1<<16));
	}
	else
	{
		RLE0787_W32(gmac, TCR, RLE0787_R32(gmac, TCR)&~(1<<15));
		RLE0787_W32(gmac, IO_CMD, RLE0787_R32(gmac, IO_CMD)&~(1<<28));
		RLE0787_W32(gmac, CONFIG_REG, RLE0787_R32(gmac, CONFIG_REG)&~(1<<16));
	}
}

static int re8686_set_mtu(struct net_device *dev, int new_mtu)
{
	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;
	
	if (new_mtu < 68 || new_mtu > RE8686_ETH_DATA_LEN)
	{
		return -EINVAL;
	}
	dev->mtu = new_mtu;
	txJumboFrameEnabled[gmac] = (new_mtu > SKB_BUF_SIZE) ? 1 : 0;
	change_tx_jumbo_setup(gmac, (new_mtu > SKB_BUF_SIZE) ? 1 : 0);
	
	return 0;
}

static inline u16 read_isr_status(struct re_private *cp){
	unsigned int gmac = cp->gmac;
	u16 isr_status = (RLE0787_R16(gmac, ISR) & RX_ALL(gmac));

	/*rx mring split int*/
	assigne_cpisr_status(cp->isr_status, isr_status);
	assigne_cpisr1_status(cp->isr1_status, (retrive_isr1_status(gmac) & rx_multiring_bitmap[gmac]));
	gather_rx_isr(isr_status, cp->isr1_status);
	return isr_status;
}

static inline void mask_rx_int(unsigned int gmac){
	RLE0787_R16(gmac, IMR)&=(u16)(~RX_ALL(gmac));
	MASK_IMR0_RXALL(gmac);
}

static inline void unmask_rx_int(unsigned int gmac){
	RLE0787_R16(gmac, IMR)|=(u16)(RX_ALL(gmac));//we still open imr when rx_work==0 for a quickly schedule
	UNMASK_IMR0_RXALL(gmac);
}

static inline void clear_isr(struct re_private *cp, u16 status){
	unsigned int gmac = cp->gmac;
	
	RLE0787_W16(gmac, ISR, status);
	CLEAR_ISR1(gmac, cp->isr1_status);	
}

void memDump (void *start, u32 size, char * strHeader)
{
	int row, column, index, index2, max;
//	uint32 buffer[5];
	u8 *buf, *line, ascii[17];
	char empty = '.';

	if(!start ||(size==0))
		return;
	line = (u8*)start;

	/*
	16 bytes per line
	*/
	if (strHeader)
		printk("%s", strHeader);
	column = size % 16;
	row = (size / 16) + 1;
	for (index = 0; index < row; index++, line += 16) 
	{
		buf = line;

		memset (ascii, 0, 17);

		max = (index == row - 1) ? column : 16;
		if ( max==0 ) break; /* If we need not dump this line, break it. */

		printk("\n%08x ", (u32) line);
		
		//Hex
		for (index2 = 0; index2 < max; index2++)
		{
			if (index2 == 8)
			printk("  ");
			printk("%02x ", (u8) buf[index2]);
			ascii[index2] = (((u8) buf[index2] < 32) || ((u8) buf[index2] >= 128)) ? empty : buf[index2];
		}

		if (max != 16)
		{
			if (max < 8)
				printk("  ");
			for (index2 = 16 - max; index2 > 0; index2--)
				printk("   ");
		}

		//ASCII
		printk("  %s", ascii);
	}
	printk("\n");
	return;
}


#undef ETH_DBG
#define ETH_DBG
#ifdef ETH_DBG
static void skb_debug(const struct sk_buff *skb, int enable, int flag)
{
#define NUM2PRINT 1518
	if (unlikely(enable & flag)) {
		if (flag == RTL8686_SKB_RX)
			printk("\nI: ");
		else
			printk("\nO: ");
		printk("eth len = %d eth name %s", skb->len,skb->dev?skb->dev->name:"");
		memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "");
		if(skb->len > NUM2PRINT){
			printk("........");
		}
		printk("\n");
	}
}
//int32 dump_hs (void);
//int32 dump_lut_table (void);

void rxinfo_debug(unsigned int gmac, struct rx_info *pRxInfo, int ring_num)
{
	if (unlikely(debug_enable[gmac] & (RTL8686_RXINFO))) {
		printk("rxInfo[ring%d]:\n", ring_num);		
		printk("opts1\t= 0x%08x Own=%d EOR=%d FS=%d LS=%d CRCErr=%d IP4CSErr=%d L4CSErr=%d RCDF=%d IPFrag=%d PPPoETag=%d RWT=%d Len=%d\n", pRxInfo->opts1.dw,
			pRxInfo->opts1.bit.own,
			pRxInfo->opts1.bit.eor,
			pRxInfo->opts1.bit.fs,
			pRxInfo->opts1.bit.ls,
			pRxInfo->opts1.bit.crcerr,
			pRxInfo->opts1.bit.ipv4csf,
			pRxInfo->opts1.bit.l4csf,
			pRxInfo->opts1.bit.rcdf,
			pRxInfo->opts1.bit.ipfrag,			
			pRxInfo->opts1.bit.pppoetag,
			pRxInfo->opts1.bit.rwt,
			pRxInfo->opts1.bit.data_length
		);
		printk("addr\t= 0x%08x\n", pRxInfo->addr);
		printk("opts2\t= 0x%08x CpuTag=%d PTPCpuTagEst=%d SVLAN_Est=%d Reason=%d CtagVa=%d CvlanTag=%d\n", pRxInfo->opts2.dw,
			pRxInfo->opts2.bit.cputag,	
			pRxInfo->opts2.bit.ptp_in_cpu_tag_exist,
			pRxInfo->opts2.bit.svlan_tag_exist,
			pRxInfo->opts2.bit.reason,
			pRxInfo->opts2.bit.ctagva,
			pRxInfo->opts2.bit.cvlan_tag
		);		
		printk("opts3\t= 0x%08x IntPri=%d PONSID_or_EXTSPA=%d L3Rout=%d ORG=%d SrcPortNum=%d FBI=%d FBHSH_or_DstPortmsk=0x%x\n", pRxInfo->opts3.dw,
			pRxInfo->opts3.bit.internal_priority,	
			pRxInfo->opts3.bit.pon_sid_or_extspa,	
			pRxInfo->opts3.bit.l3routing,	
			pRxInfo->opts3.bit.origformat,	
			pRxInfo->opts3.bit.src_port_num,
			pRxInfo->opts3.bit.fbi,
			pRxInfo->opts3.bit.fb_hash_or_dst_portmsk
		);
	}
}

void txinfo_debug(unsigned int gmac, struct tx_info *pTxInfo)
{

	if (unlikely(debug_enable[gmac] & (RTL8686_TXINFO))) {
		printk("txInfo:\n");
		printk("opts1\t= 0x%08x Own=%d EOR=%d FS=%d LS=%d IPCS=%d L4CS=%d TPID_Sel=%d STAG_Aware=%d CRC=%d Len=%d\n", pTxInfo->opts1.dw,
			pTxInfo->opts1.bit.own,
			pTxInfo->opts1.bit.eor,
			pTxInfo->opts1.bit.fs,
			pTxInfo->opts1.bit.ls,
			pTxInfo->opts1.bit.ipcs,
			pTxInfo->opts1.bit.l4cs,
			pTxInfo->opts1.bit.tpid_sel,
			pTxInfo->opts1.bit.stag_aware,
			pTxInfo->opts1.bit.crc,
			pTxInfo->opts1.bit.data_length
		);
		printk("addr\t= 0x%08x \n", pTxInfo->addr);
		printk("opts2\t= 0x%08x CPUTag=%d SVLANAct=%d CVLANAct=%d TxPmsk=0x%x CVLAN_VID=%d CVLAN_Pri=%d\n", pTxInfo->opts2.dw,
			pTxInfo->opts2.bit.cputag,			
			pTxInfo->opts2.bit.tx_svlan_action,
			pTxInfo->opts2.bit.tx_cvlan_action,
			pTxInfo->opts2.bit.tx_portmask,
			pTxInfo->opts2.bit.cvlan_vidh<<8|pTxInfo->opts2.bit.cvlan_vidl,
			pTxInfo->opts2.bit.cvlan_prio
		);
		printk("opts3\t= 0x%08x AsPri=%d CpuPri=%d Keep=%d DisLrn=%d Psel=%d L34Keep=%d extspa=%d Pppoe=%d SidIdx=%d PON_SID=%d\n", pTxInfo->opts3.dw,
			pTxInfo->opts3.bit.aspri,
			pTxInfo->opts3.bit.cputag_pri,
			pTxInfo->opts3.bit.keep,
			pTxInfo->opts3.bit.dislrn,
			pTxInfo->opts3.bit.cputag_psel,
			pTxInfo->opts3.bit.l34_keep,
			pTxInfo->opts3.bit.extspa,
			pTxInfo->opts3.bit.tx_pppoe_action,
			pTxInfo->opts3.bit.tx_pppoe_idx,
			pTxInfo->opts3.bit.tx_dst_stream_id
		);
		printk("opts4\t= 0x%08x LgsEn=%d LgMtu=%d SVLAN_VID=%d SVLAN_Pri=%d\n", pTxInfo->opts4.dw,
			pTxInfo->opts4.bit.lgsen,
			pTxInfo->opts4.bit.lgmtu,
			pTxInfo->opts4.bit.svlan_vidh<<8|pTxInfo->opts4.bit.svlan_vidl,
			pTxInfo->opts4.bit.svlan_prio
		);	
	}
}

static char mt_watch_tmp[512];

#define RX_TRACE( gmac, comment, arg...) \
do {\
		if(debug_enable[gmac]&RTL8686_RX_TRACE)\
		{\
			u8 mt_trace_head_str[]="[NIC RX]\033[1;36m"; \
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s %s \033[1;30m@%s:%d\033[0m\n",mt_trace_head_str,mt_watch_tmp,__FILE__,__LINE__);\
		}\
} while(0);

#define TX_TRACE( gmac, comment, arg...) \
do {\
		if(debug_enable[gmac]&RTL8686_TX_TRACE)\
		{\
			u8 mt_trace_head_str[]="[NIC TX]\033[1;36m"; \
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s %s \033[1;30m@%s:%d\033[0m\n",mt_trace_head_str,mt_watch_tmp,__FILE__,__LINE__);\
		}\
} while(0);

#define RX_WARN( gmac, comment, arg...) \
do {\
		if(debug_enable[gmac]&RTL8686_RX_WARN)\
		{\
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s\n",mt_watch_tmp);\
		}\
} while(0);

#define TX_WARN( gmac, comment, arg...) \
do {\
		if(debug_enable[gmac]&RTL8686_TX_WARN)\
		{\
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s\n",mt_watch_tmp);\
		}\
} while(0);

#define ETHDBG_PRINT(gmac, flag, fmt, args...)  if(unlikely(debug_enable[gmac] & flag)) printk(fmt, ##args)
#define SKB_DBG(args...) skb_debug(args)
#define RXINFO_DBG(gmac, args...) rxinfo_debug(gmac, args)
#define TXINFO_DBG(gmac, args...) txinfo_debug(gmac, args)
#else
#define RX_TRACE(gmac, args...)
#define TX_TRACE(gmac, args...)
#define ETHDBG_PRINT(gmac, flag, fmt, args...)
#define SKB_DBG(args...) 
#define RXINFO_DBG(gmac, args...)
#define TXINFO_DBG(gmac, args...)
#endif 

__IRAM_NIC
struct sk_buff *re8670_getAlloc(unsigned int size)
{	
	struct sk_buff *skb=NULL;
#ifndef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
	if ( atomic_read(&re8670_rxskb_num) < RE8670_MAX_ALLOC_RXSKB_NUM ) {
		#ifdef CONFIG_RTL865X_ETH_PRIV_SKB  //For RTL8192CE
		skb = dev_alloc_skb_priv_eth(CROSS_LAN_MBUF_LEN);
		#else
		skb = dev_alloc_skb(size);
		#endif

		if (skb!=NULL) {
			atomic_inc(&re8670_rxskb_num);
			skb->src_port=IF_ETH;
		}
	}
	else {
//		printk("%s(%d): limit reached (%d/%d)\n",__FUNCTION__,__LINE__,atomic_read(&re8670_rxskb_num),RE8670_MAX_ALLOC_RXSKB_NUM);
	}
#else
	skb = dev_alloc_skb_priv_eth(CROSS_LAN_MBUF_LEN);
#endif
	
	return skb;
}

__IRAM_NIC
struct sk_buff *re8670_getMcAlloc(unsigned int size)
{	
	struct sk_buff *skb=NULL;
	if ( atomic_read(&re8670_rxskb_num) < RE8670_MAX_ALLOC_MC_NUM ) {
		#ifdef CONFIG_RTL865X_ETH_PRIV_SKB  //For RTL8192CE
		skb = dev_alloc_skb_priv_eth(CROSS_LAN_MBUF_LEN);
		#else
		skb = dev_alloc_skb(size);
		#endif

		if (skb!=NULL) {
			atomic_inc(&re8670_rxskb_num);
			skb->src_port=IF_ETH;
		}
	}
	else {
//		printk("%s(%d): limit MC reached (%d/%d)\n",__FUNCTION__,__LINE__,atomic_read(&re8670_rxskb_num),RE8670_MAX_ALLOC_RXSKB_NUM);
	}
	return skb;
}

__IRAM_NIC
struct sk_buff *re8670_getBcAlloc(unsigned int size)
{	
	struct sk_buff *skb=NULL;
	if ( atomic_read(&re8670_rxskb_num) < RE8670_MAX_ALLOC_BC_NUM ) {
		#ifdef CONFIG_RTL865X_ETH_PRIV_SKB  //For RTL8192CE
		skb = dev_alloc_skb_priv_eth(CROSS_LAN_MBUF_LEN);
		#else
		skb = dev_alloc_skb(size);
		#endif

		if (skb!=NULL) {
			atomic_inc(&re8670_rxskb_num);
			skb->src_port=IF_ETH;
		}
	}
	else {
//		printk("%s(%d): limit BC reached (%d/%d)\n",__FUNCTION__,__LINE__,atomic_read(&re8670_rxskb_num),RE8670_MAX_ALLOC_RXSKB_NUM);
	}
	return skb;
}

__IRAM_NIC
struct sk_buff *re8670_getCriticalAlloc(unsigned int size)
{
	struct sk_buff *skb=NULL;

#ifdef CONFIG_RTL865X_ETH_PRIV_SKB  //For RTL8192CE
	skb = dev_alloc_critical_skb_priv_eth(CROSS_LAN_MBUF_LEN);
#else
	skb = dev_alloc_skb(size);
#endif

	return skb;
}

__IRAM_NIC
struct sk_buff *re8670_getJumboAlloc(unsigned int size)
{
	struct sk_buff *skb=NULL;

		#ifdef CONFIG_RTL865X_ETH_PRIV_SKB  //For RTL8192CE
		skb = dev_alloc_skb_priv_eth_rg(CROSS_LAN_MBUF_LEN_RG);
		#else
		skb = dev_alloc_skb(size);
		#endif

	return skb;
}

static inline void re8670_set_rxbufsize (struct re_private *cp)
{
	unsigned int mtu = cp->dev->mtu;
#if defined(CONFIG_DUALBAND_CONCURRENT)
	cp->dev->mtu = SKB_BUF_SIZE-ETH_HLEN-8;
	mtu = cp->dev->mtu;
#endif
	if (mtu > ETH_DATA_LEN)
		/* MTU + ethernet header + FCS + optional VLAN tag */
		cp->rx_buf_sz = mtu + ETH_HLEN + 8;
	else
		cp->rx_buf_sz = SKB_BUF_SIZE;
}

atomic_t lock_tx_tail = ATOMIC_INIT(0);

int re8686_register_rxfunc_by_port(unsigned int gmac, unsigned int port, p2rfunc_t pfunc){
	if(port >= SW_PORT_NUM || !pfunc)
		return -EINVAL;
	re_private_data[gmac].port2rxfunc[port] = pfunc;
	return 0;
}

int re8686_register_rxfunc_all_port(p2rfunc_t pfunc){
	unsigned int gmac;
	unsigned int i;
	int ret;
	
	if(!pfunc)
		return -EINVAL;

	for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {
		for(i=0;i<SW_PORT_NUM;i++) {
			ret = re8686_register_rxfunc_by_port(gmac, i, pfunc);
			if(ret){
				return ret;
			}
		}
	}

	return 0;
}

void re8686_reset_rxfunc_to_default(unsigned int gmac) {
	unsigned int i;
	for(i=0;i<SW_PORT_NUM;i++){
		re8686_register_rxfunc_by_port(gmac, i, re8670_rx_skb);
	}
}

static inline void _tx_additional_setting(struct sk_buff *skb, struct net_device *dev, struct tx_info *pTxInfo){
	pTxInfo->opts2.bit.cputag = 1;
	
#ifdef CONFIG_RG_SIMPLE_PROTOCOL_STACK
	//the txPmask and VLAN information should be decided in fwdEngine_xmit and recorded in txInfo,
	//so we do not need these check code here
#else
	if(pTxInfo->opts2.bit.tx_portmask == 0){
#ifdef CONFIG_DUALBAND_CONCURRENT
		if(memcmp(skb->dev->name,"wlan1",5)==0 || memcmp(skb->dev->name,"vwlan", 5)==0)
		{
			pTxInfo->opts2.bit.cvlan_vidl=CONFIG_DEFAULT_TO_SLAVE_GMAC_VID&0xff;
			pTxInfo->opts2.bit.cvlan_vidh=CONFIG_DEFAULT_TO_SLAVE_GMAC_VID>>8;
			pTxInfo->opts2.bit.cvlan_prio=CONFIG_DEFAULT_TO_SLAVE_GMAC_PRI;		
			pTxInfo->opts2.bit.tx_portmask=0; //HWLOOKUP (because: HW do not have extension port & CPU port bit)
			//rg_kernel.txDesc.opts2.bit.tx_vlan_action=0;// no-action
			pTxInfo->opts2.bit.tx_cvlan_action=3;// remarking
		}
		else
		{
		pTxInfo->opts2.bit.tx_portmask = DEVPRIV(dev)->txPortMask;
	}
#else
		pTxInfo->opts2.bit.tx_portmask = DEVPRIV(dev)->txPortMask;
#endif
	}
	
	//luke:20130411, patch for protocol stack broadcast hardware lookup packet to LAN will fail 
	//this patch should be remove when fwdEngine Tx module is ready.
	if((pTxInfo->opts2.bit.tx_portmask == 0) && ((skb->data[0]&1)==1))
	{
#ifdef CONFIG_DUALBAND_CONCURRENT
		if(memcmp(skb->dev->name,"wlan1",5)!=0 && memcmp(skb->dev->name,"vwlan", 5)!=0)
		{
#endif
			//luke:20130506, patch for broadcast packet from WAN port will return to WAN port, cause spanning tree protocol treats it as loop
			if(skb->from_dev && (skb->from_dev->priv_flags & (IFF_DOMAIN_ELAN|IFF_DOMAIN_WAN)))
			{
				//printk("skb is from %s %x, did not tx to wan port\n",skb->from_dev->name,DEVPRIV(skb->from_dev)->txPortMask);
			}
			else
			{
				//printk("broadcast!! the dev name is %s, txportmask is %x\n",dev->name,DEVPRIV(dev)->txPortMask);
				//force do DirectTx to all port except CPU port
				pTxInfo->opts2.bit.tx_portmask=0x2f;
			}
			//printk("the txpmsk is %x\n",pTxInfo->opts3.bit.tx_portmask);
#ifdef CONFIG_DUALBAND_CONCURRENT		
		}		
#endif
	}
//20141114LUKE: disable for protocol stack should carry ctag itself.
#if 0	
	/*should we check vlan_tci means vlan 0 or no vlan?*/
	if((skb->dev->priv_flags & IFF_DOMAIN_WAN) && skb->vlan_tci){
		pTxInfo->opts2.bit.tx_vlan_action = TXD_VLAN_REMARKING;
		pTxInfo->opts2.dw |= VTAG2DESC(skb->vlan_tci);
	}
#endif
#endif

	if(skb->from_dev){
		if(skb->from_dev->priv_flags & (IFF_DOMAIN_ELAN|IFF_DOMAIN_WAN)){
			pTxInfo->opts3.bit.dislrn = 1; //disable cpu port learning
		}
	}
#if defined(CONFIG_RTK_L34_ENABLE) && defined(CONFIG_RG_API_RLE0371)
	//tysu patch for 0371
	pTxInfo->opts2.bit.tx_portmask=0;
#endif
	
	//20130724: If NIC TX send by HWLOOKUP, L34 Keep will cause un-except problem.
	if(pTxInfo->opts2.bit.tx_portmask!=0) pTxInfo->opts3.bit.l34_keep = 1;	//ensure switch won't modify or filter packet
	//20141104LUKE: when L34Keep is on, Keep is also needed for gpon.
	if(pTxInfo->opts3.bit.l34_keep)pTxInfo->opts3.bit.keep = 1;

	//20131028: If ipv4 packet length <=60, then recaculate data_length in txInfo. Otherwise GMAC will cause L4chksum error
	//20131029: OMCI packet may cause data_length too big, so check streamID to filter them!!
	if(skb->len==60
#ifdef CONFIG_GPON_FEATURE
		&& pTxInfo->opts3.bit.tx_dst_stream_id!=0x40
#endif
		)
	{
		u16 off=12;
		u32 xlen=0;
	
		if((*(u16*)(skb->data+off))==htons(0x8100))//CTAG
	        off+=4;
		if(((*(u16*)(skb->data+off))==htons(0x8863))||((*(u16*)(skb->data+off))==htons(0x8864)))//PPPoE
			off+=8;
		if(((*(u16*)(skb->data+off))==htons(0x0800))||((*(u16*)(skb->data+off))==htons(0x0021)))//IPv4 or IPv4 with PPPoE
		{
			xlen=(off+2+((skb->data[off+4]<<8)|skb->data[off+5]))&0x1ffff;	//l3offset + IPv4 total length
			if(xlen<=60)
				pTxInfo->opts1.bit.data_length=xlen;
		}
			
	}
	#if 0
	if( !padding_enable && pTxInfo->opts1.bit.cputag_psel != 1)
		pTxInfo->opts3.bit.tx_dst_stream_id = 0x40;
	#endif
}

//#define RE8686_VERIFY
#ifdef RE8686_VERIFY
#include "./re8686_verify.c"
#else
#define INVERIFYMODE 0
#endif

#if 0
/*__IRAM_NIC*/ void nic_tx2(struct sk_buff *skb)
{
	skb->users.counter=1;
	re8670_start_xmit(skb,eth_net_dev[0]);
}
#endif

__IRAM_NIC
#if !defined(CONFIG_RG_WMUX_SUPPORT) && !defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
static
#endif
struct net_device* decideRxDevice(struct re_private *cp, struct rx_info *pRxInfo){
	unsigned int num = (pRxInfo->opts3.bit.src_port_num >= SW_PORT_NUM) ? 
		(0) : pRxInfo->opts3.bit.src_port_num ;
#ifdef CONFIG_RG_SIMPLE_PROTOCOL_STACK
	//printk("the num is %d, the src_port_num is %d, port2dev->name is %s\n",num,pRxInfo->opts3.bit.src_port_num,
	//cp->port2dev[num]->name);
	if((cp->port2dev[num]->priv_flags & IFF_DOMAIN_WAN)>0)
	{
		//printk("in %s, the wan interface %s\n",__FUNCTION__,cp->multiWanDev[cp->wanInterfaceIdx]->name);
		return cp->multiWanDev[cp->wanInterfaceIdx];
	}
#endif

#ifdef CONFIG_DUALBAND_CONCURRENT
	if(pRxInfo->opts3.bit.dst_port_mask==0x10) //from EXT2
		return rtl8686_dev_table[wlan1_dev_idx].dev_instant;
#endif
	return cp->port2dev[num];
}

static inline void updateRxStatic(struct re_private *cp, struct sk_buff *skb){
	skb->dev->last_rx = jiffies;
	cp->cp_stats.rx_sw_num++;
	DEVPRIV(skb->dev)->net_stats.rx_packets++;
	DEVPRIV(skb->dev)->net_stats.rx_bytes += skb->len;
}

#ifdef CONFIG_DUALBAND_CONCURRENT
void str2mac(unsigned char *mac_string, unsigned char *MacEntry)
{
	int i,j=0,k=0;
	memset(MacEntry,0,ETH_ALEN);
	for(i=0;i<strlen(mac_string);i++)
	{			
		if(mac_string[i]==':') 
		{
			j=0;
			continue;
		}
		else if((mac_string[i]>='A')&&(mac_string[i]<='F'))
			MacEntry[k]+=(mac_string[i]-'A'+10);
		else if((mac_string[i]>='a')&&(mac_string[i]<='f'))
			MacEntry[k]+=(mac_string[i]-'a'+10);
		else if((mac_string[i]>='0')&&(mac_string[i]<='9'))
			MacEntry[k]+=(mac_string[i]-'0');
		else 
			printk("str2mac MAC string parsing error!");
		if(j==0) MacEntry[k]<<=4;
		if(j==1) k++;
		j++;
	}
}
#endif

__IRAM_NIC
int re8670_rx_skb (struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo)
{
	unsigned int gmac = cp->gmac;
	struct net_device *net_device_record = NULL;
#ifdef CONFIG_DUALBAND_CONCURRENT
	int i;
#endif
	skb->dev = decideRxDevice(cp, pRxInfo);
	net_device_record = skb->dev;
#ifdef CONFIG_DUALBAND_CONCURRENT
	if(!memcmp(src_mac, skb->data + ETH_ALEN, ETH_ALEN))
		skb->dev = rtl8686_dev_table[vwlan_dev_idx].dev_instant;
#endif
	skb->vlan_tci = (pRxInfo->opts2.bit.ctagva) ? VTAG2VLANTCI(pRxInfo->opts2.bit.cvlan_tag) : 0;
	ETHDBG_PRINT(gmac, RTL8686_SKB_RX, "This packet comes from %s(vlan %u)\n", skb->dev->name, skb->vlan_tci);
#ifdef CONFIG_DUALBAND_CONCURRENT
	if(strcmp(skb->dev->name, "vwlan") == 0 && (*(u16*)&skb->data[12])==0x8100)
        {
                for(i=12;i>=4;i-=4)
                {
                        *(u32*)&skb->data[i]=*(u32*)&skb->data[i-4];
                }
                skb->len-=4;
                skb->data+=4;
        }
#endif

	skb->from_dev=skb->dev;

	/* switch_port is patched for iptables and ebtables rule matching */
	skb->switch_port = NULL;
	skb->mark = (skb->vlan_tci & 0xFFF);
	//printk("%s %d switch_port: %s vlan_tci=0x%x mark=0x%x\n", 
	//		__func__, __LINE__, skb->switch_port, skb->vlan_tci, skb->mark);

	skb->protocol = eth_type_trans (skb, skb->dev);
	
	//do we need any wan dev rx hacking here?(before pass to netif_rx)
	
	updateRxStatic(cp, skb);
	SKB_DBG(skb, debug_enable[gmac], RTL8686_SKB_RX);
	if (netif_rx(skb) == NET_RX_DROP)
		DEVPRIV(net_device_record)->net_stats.rx_dropped++;

	return RE8670_RX_STOP_SKBNOFREE;
}

#ifdef SMBSHORTCUT
int checkLocalIPPort(u32 srcip, u16 dport){
//      printk("src 0x%x, dport %d\n", srcip, dport);
        if ( srcip == smbserverip && ( dport == 20 || dport == 21 || dport == 137 || dport == 138 || dport == 139 || dport == 445))
                return 1;
        else
                return 0;
}

int checkSMBIPPort(struct sk_buff *skb, int hook){
        struct iphdr *iph = ip_hdr(skb);
        //const struct iphdr *iph = ((struct sk_buff *)skb)->nh.iph;
        struct tcphdr *th;

        if ( !smbscflag )
                return 0;

        if (unlikely( iph->frag_off & 0x3fff )) /* Ignore fragment */
        {
                //printk(KERN_ERR "[%s][%d]\n", __FUNCTION__, __LINE__);
                return 0;
        }

        th=(struct tcphdr *) ((void *) iph + iph->ihl*4);
        if (th->syn)
                return 0;

        switch (hook) {
                case NF_INET_LOCAL_IN:
                case NF_INET_PRE_ROUTING:
                        if ( checkLocalIPPort(iph->daddr, th->dest) ){
                                //printk("Incoming serverip = %d.%d.%d.%d\n",  (iph->daddr & 0xff000000) >> 24,         (iph->daddr & 0x00ff0000) >> 16, (iph->daddr & 0x0000ff00) >> 8,        (iph->daddr & 0xff));
                                return 1;
                        }
                        break;
                case NF_INET_LOCAL_OUT:
                case NF_INET_POST_ROUTING:
                        if ( checkLocalIPPort(iph->saddr, th->source) ){
                                //printk("Outgoing serverip = %d.%d.%d.%d\n", (iph->saddr & 0xff000000) >> 24,  (iph->saddr & 0x00ff0000) >> 16, (iph->saddr & 0x0000ff00) >> 8,        (iph->saddr & 0xff));
                                return 1;
                        }
                        break;
                default:
                        return 0;
        }
        return 0;
}

static void smbSetServerIP(u32 sip){
        smbserverip = sip;
}
static void smbSetflag(u8 flag){
        smbscflag = flag;
}
#endif


#ifdef CONFIG_RTK_HOST_SPEEDUP
int re8670_start_xmit_txInfo (struct sk_buff *skb, struct net_device *dev, struct tx_info* ptxInfo, struct tx_info* ptxInfoMask);
__IRAM_NIC
int re8670_host_speedup (struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo)
{
    struct tcphdr *tcph;
#ifdef SMBSHORTCUT	
	struct udphdr *udph;
#endif	
	struct iphdr *iph;
    unsigned char *pData = skb->data;
	int isPPPoE=0;
	unsigned char *pL2Hdr, *pPPPoEHdr, *pL3Hdr;
	unsigned int rcvBytes;
	unsigned int pktlen=0, l3Len, l4Len;
	unsigned int seqNo, ackNo;
	unsigned short ip_id=0;
	void *pHostTrack;
	int ret;

    if (isHostSpeedUpEnable())
    {
        pL2Hdr = pData;

        pData += 12;
        if (*(unsigned short *)pData == 0x8100)
            pData += 4;

        if (((*(unsigned short *)pData == 0x8864) && (*(unsigned short *)(pData+8) == 0x0021)) ||
            (*(unsigned short *)pData == 0x0800))
        {
             if (*(unsigned short *)pData == 0x8864)
			{
				isPPPoE = 1;
				pPPPoEHdr = pData + 2;
			}
			
            if (*(unsigned short *)pData == 0x0800)
                pL3Hdr = pData + 2;
            else
                pL3Hdr = pData + 10;
			
			iph = (struct iphdr *)pL3Hdr;

            if (iph->protocol != IPPROTO_TCP)
                return RE8670_RX_CONTINUE;

            tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);

            if ((pHostTrack = shouldSpeedUp(0, iph->daddr, iph->saddr, tcph->dest, tcph->source)) != NULL)
            {
				if ((tcph->syn) || (tcph->fin))
					return re8670_rx_skb(cp, skb, pRxInfo);
            	/* calc rx rate */
				speedtest_sample_enter(skb->len);

				if (shouldBypassSpeedUP(pHostTrack))
					return re8670_rx_skb(cp, skb, pRxInfo);
				
				fast_ack_calibration_enter(skb->len);
				
				l3Len = (iph->ihl<<2);
				l4Len = (tcph->doff<<2);
				pktlen = (pL3Hdr - pL2Hdr) + l3Len + l4Len;
				rcvBytes = iph->tot_len - l3Len - l4Len;
				
				if ((0 == rcvBytes) && tcph->ack)
					return re8670_rx_skb(cp, skb, pRxInfo);
				
				seqNo = tcph->seq;
				ackNo = tcph->ack_seq;
				
            	ret = speedtest_stream_shortcut_check(pHostTrack, iph->daddr, iph->saddr, tcph->dest, tcph->source, 
													&seqNo, &ip_id, rcvBytes, skb);
				if (SPEEDUP_RET_MATCH_ACK == ret)
        		{
        			/* ACK right now! */
					unsigned int i;
					unsigned int tmp;
					skb->dev = decideRxDevice(cp, pRxInfo);
					updateRxStatic(cp, skb);

					/* modify mac header */
					for (i=0; i<3; i++)
					{
						tmp = *((unsigned short *)pL2Hdr + i);
						*((unsigned short *)pL2Hdr +i) = *((unsigned short *)pL2Hdr + 3 + i);
						*((unsigned short *)pL2Hdr + 3 + i) = tmp;
					}

					/* modify pppoe header */
					if (isPPPoE)
					{
						/* modify pppoe length field */
						*(unsigned short *)(pPPPoEHdr + 4) = l3Len + l4Len + 2;
					}

					/*modify IP Header */
					iph->tot_len = l3Len + l4Len;
					iph->frag_off = htons(IP_DF);
					iph->ttl = 64;
					tmp = iph->saddr;
					iph->saddr = iph->daddr;
					iph->daddr = tmp;
					iph->id = ip_id;
					ip_send_check(iph);

					/*modify tcp header */
					tmp = tcph->source;
					tcph->source = tcph->dest;
					tcph->dest = tmp;
					tcph->seq = ackNo;
					tcph->ack_seq = seqNo;
					*((unsigned char *)tcph + 13) = 0;
					tcph->ack = 1;
					tcph->window = FAST_ACK_WIN_SIZE;
					tcph->check = 0;
					skb->csum = 0;
					/* modify timestamp */
					if (l4Len > 20)
					{
						char *ptr = (unsigned char *)(tcph + 1);
						int length = l4Len - sizeof(struct tcphdr);
						while (length > 0) {
							int opcode = *ptr++;
							int opsize;
					
							switch (opcode) {
							case 0:	/* EOL */
								length = 0;
								break;
							case 1:	/* NOP Ref: RFC 793 section 3.1 */
								length--;
								break;
							default:
								opsize = *ptr++;
								if (opsize < 2) /* "silly options" */
								{
									length = 0;
									break;
								}
								if (opsize > length)
								{
									length = 0; /* don't parse partial options */
									break;
								}
								if (8 == opcode)	/* timestamps */
								{
									if (opsize == 10)
									{
										for (i=0; i<4; i++)
										{
											tmp = ptr[i];
											ptr[i] = ptr[4+i];
											ptr[4+i] = tmp;
										}
									}
								}
								ptr += opsize-2;
								length -= opsize;
							}
						}
					}
					
					tcph->check = tcp_v4_check(l4Len, iph->saddr, iph->daddr,
											 csum_partial(tcph,
												      tcph->doff << 2,
												      skb->csum));
					skb->len = pktlen;

					re8670_start_xmit(skb, skb->dev);
					
        			return RE8670_RX_STOP_SKBNOFREE;
        		}
				else if (SPEEDUP_RET_MATCH_NO_ACK == ret)
				{
					skb->dev = decideRxDevice(cp, pRxInfo);
					updateRxStatic(cp, skb);
					return RE8670_RX_STOP;
				}
				else if (SPEEDUP_RET_MATCH_QUEUED == ret)
				{
					skb->dev = decideRxDevice(cp, pRxInfo);
					updateRxStatic(cp, skb);
					return RE8670_RX_STOP_SKBNOFREE;
				}
				else
					return re8670_rx_skb(cp, skb, pRxInfo);
            }
        }
    }
#ifdef SMBSHORTCUT
        if ( smbscflag  && (0x3f&(1 << pRxInfo->opts3.bit.src_port_num)) ){
                pData += 12;
                if (*(unsigned short *)pData == 0x8100)
                        pData += 4;

                if (((*(unsigned short *)pData == 0x8864) && (*(unsigned short *)(pData+8) == 0x0021)) ||
                        (*(unsigned short *)pData == 0x0800))
                {
                        if (*(unsigned short *)pData == 0x0800)
                                iph = (struct iphdr *)(pData + 2);
                        else
                                iph = (struct iphdr *)(pData + 10);

                        if (iph->protocol == IPPROTO_TCP){
                                tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);
                                if ( checkLocalIPPort(iph->daddr, tcph->dest) ){
                                        return re8670_rx_skb(cp, skb, pRxInfo);
                                }
                        }else if (iph->protocol == IPPROTO_UDP){
                                udph = (struct udphdr*)((__u32 *)iph + iph->ihl);
                                if ( checkLocalIPPort(iph->daddr, udph->dest) ){
                                        return re8670_rx_skb(cp, skb, pRxInfo);
                                }
                        }
                }
        }
#endif

    return RE8670_RX_CONTINUE;
}

int re8670_speedup_ack_send(struct sk_buff *skb, unsigned int seqNo, unsigned short ip_id)
{
	struct tcphdr *tcph;
	struct iphdr *iph;
	unsigned char *pData = skb->data;
	int isPPPoE=0;
	unsigned char *pL2Hdr, *pPPPoEHdr, *pL3Hdr;
	unsigned int pktlen=0, l3Len, l4Len;
	unsigned int ackNo;
	unsigned int i;
	unsigned int tmp;

	pL2Hdr = pData;

	pData += 12;
	if (*(unsigned short *)pData == 0x8100)
		pData += 4;

	if (*(unsigned short *)pData == 0x8864)
	{
		isPPPoE = 1;
		pPPPoEHdr = pData + 2;
	}

	if (*(unsigned short *)pData == 0x0800)
		pL3Hdr = pData + 2;
	else
		pL3Hdr = pData + 10;

	iph = (struct iphdr *)pL3Hdr;

	tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);

	l3Len = (iph->ihl<<2);
	l4Len = (tcph->doff<<2);
	pktlen = (pL3Hdr - pL2Hdr) + l3Len + l4Len;

	ackNo = tcph->ack_seq;

	/* modify mac header */
	for (i=0; i<3; i++)
	{
		tmp = *((unsigned short *)pL2Hdr + i);
		*((unsigned short *)pL2Hdr +i) = *((unsigned short *)pL2Hdr + 3 + i);
		*((unsigned short *)pL2Hdr + 3 + i) = tmp;
	}

	/* modify pppoe header */
	if (isPPPoE)
	{
		/* modify pppoe length field */
		*(unsigned short *)(pPPPoEHdr + 4) = l3Len + l4Len + 2;
	}

	/*modify IP Header */
	iph->tot_len = l3Len + l4Len;
	iph->frag_off = htons(IP_DF);
	iph->ttl = 64;
	tmp = iph->saddr;
	iph->saddr = iph->daddr;
	iph->daddr = tmp;
	iph->id = ip_id;
	ip_send_check(iph);

	/*modify tcp header */
	tmp = tcph->source;
	tcph->source = tcph->dest;
	tcph->dest = tmp;
	tcph->seq = ackNo;
	tcph->ack_seq = seqNo;
	*((unsigned char *)tcph + 13) = 0;
	tcph->ack = 1;
	tcph->window = FAST_ACK_WIN_SIZE;
	tcph->check = 0;
	skb->csum = 0;
	/* modify timestamp */
	if (l4Len > 20)
	{
		char *ptr = (unsigned char *)(tcph + 1);
		int length = l4Len - sizeof(struct tcphdr);
		while (length > 0) {
			int opcode = *ptr++;
			int opsize;

			switch (opcode) {
			case 0:	/* EOL */
				length = 0;
				break;
			case 1:	/* NOP Ref: RFC 793 section 3.1 */
				length--;
				break;
			default:
				opsize = *ptr++;
				if (opsize < 2) /* "silly options" */
				{
					length = 0;
					break;
				}
				if (opsize > length)
				{
					length = 0; /* don't parse partial options */
					break;
				}
				if (8 == opcode)	/* timestamps */
				{
					if (opsize == 10)
					{
						for (i=0; i<4; i++)
						{
							tmp = ptr[i];
							ptr[i] = ptr[4+i];
							ptr[4+i] = tmp;
						}
					}
				}
				ptr += opsize-2;
				length -= opsize;
			}
		}
	}

	tcph->check = tcp_v4_check(l4Len, iph->saddr, iph->daddr,
							 csum_partial(tcph,
								      tcph->doff << 2,
								      skb->csum));
	skb->len = pktlen;

	re8670_start_xmit(skb, skb->dev);

	return RE8670_RX_STOP_SKBNOFREE;
}
#endif


__IRAM_NIC
static void re8670_rx_software (struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo)
{
	unsigned int gmac = cp->gmac;
#if 1
	unsigned int num = (pRxInfo->opts3.bit.src_port_num >= SW_PORT_NUM) ? 
		(0) : pRxInfo->opts3.bit.src_port_num ;
	RX_TRACE(gmac, "port2rxfunc=0x%p\n", cp->port2rxfunc[num]);
	cp->port2rxfunc[num](cp, skb, pRxInfo);
#else
	re8670_rx_skb(cp, skb, pRxInfo);
#endif
}


static 
__IRAM_NIC
unsigned int re8670_rx_csum_ok (struct rx_info *rxInfo)
{
#if 0
	unsigned int protocol = rxInfo->opts1.bit.pkttype;

	switch(protocol){
		case RxProtoTCP:
		case RxProtoUDP:
		case RxProtoICMP:
		case RxProtoIGMP:/*we check both l4cs and ipv4cs when protocol is ipv4 l4*/
			if(likely((!rxInfo->opts1.bit.l4csf) && (!rxInfo->opts1.bit.ipv4csf))){
				return 1;
			}
			break;
		case RxProtoTCPv6:
		case RxProtoUDPv6:
		case RxProtoICMPv6:/*when protocol is ipv6, we only check l4cs*/
			if(likely(!rxInfo->opts1.bit.l4csf)){
				return 1;
			}
			break;
		default:/*no guarantee when the protocol is only ipv4*/
			break;
	}
#endif
	return 0;
}

unsigned char eth_close[MAX_GMAC_NUM] = {1,1,1};

// Kaohj -- for polling mode
int eth_poll=0; // poll mode flag
void eth_poll_schedule(void)
{
	tasklet_hi_schedule(eth_rx_tasklets);
}

static inline void retriveRxInfo(DMA_RX_DESC *desc, struct rx_info *pRxInfo){
	pRxInfo->opts1.dw = desc->opts1;
	pRxInfo->addr= desc->addr;
	pRxInfo->opts2.dw = desc->opts2;
	pRxInfo->opts3.dw = desc->opts3;
}

static inline void updateGmacFlowControl(unsigned int gmac,unsigned rx_tail,int ring_num){
	unsigned int new_cpu_desc_num;		

	if(re8670_rx_flow_control_status[gmac][ring_num] == OFF)
		return;
	
	if(ring_num==0)
	{    
		new_cpu_desc_num = RLE0787_R32(gmac, EthrntRxCPU_Des_Num);
		new_cpu_desc_num &= 0x00FFFF0F; // clear
		new_cpu_desc_num |= (((rx_tail&0xFF)<<24)|(((rx_tail>>8)&0xF)<<4)); // update
		RLE0787_R32(gmac, EthrntRxCPU_Des_Num) = new_cpu_desc_num;
	}
	else
	{
		new_cpu_desc_num = RLE0787_R32(gmac, EthrntRxCPU_Des_Num2+ADDR_OFFSET*(ring_num-1));
		new_cpu_desc_num &= 0xfffff000; // clear
		new_cpu_desc_num |= (((rx_tail&0xFF)|(((rx_tail>>8)&0xF)<<4))); // update
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+ADDR_OFFSET*(ring_num-1), new_cpu_desc_num);
	}
}

void vlan_detag(unsigned int gmac, int onoff){
	switch(onoff){
		case ON:
			RLE0787_W8(gmac, CMD, RLE0787_R8(gmac, CMD)|RxVLAN_Detag);
			break;
		case OFF:
			RLE0787_W8(gmac, CMD, RLE0787_R8(gmac, CMD)&~RxVLAN_Detag);
			break;
		default:
			printk("%s %d: wrong arg %d\n", __func__, __LINE__, onoff);
			break;
	}
}

void SetTxRingPrioInRR(unsigned int gmac)
{
	iocmd1_reg[gmac] |= TX_RR_scheduler;
	RLE0787_W32(gmac, IO_CMD1, iocmd1_reg[gmac]);
}

static inline unsigned char getRxRingBitMap(struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	unsigned char result = (unsigned char)(((cp->isr1_status)|(RX_RDU_CHECK(cp->isr_status))|((cp->isr_status&SW_INT)?pauseBySwRingBitmap[gmac]:0)) & rx_multiring_bitmap[gmac]);
	//RX_TRACE("cp->isr1_status=%x cp->isr_status=%x RX_RDU_CHECK(cp->isr_status)=%x RX_MULTIRING_BITMAP=%x result=%x",cp->isr1_status, cp->isr_status, RX_RDU_CHECK(cp->isr_status), RX_MULTIRING_BITMAP,result);	
	cp->isr_status=0;	
	cp->isr1_status=0;
	return result;
}

void re8686_set_pauseBySw(unsigned char enable)
{
	if(enable)
		pauseBySwEnable = ON;
	else
		pauseBySwEnable = OFF;
	return;
}
void setSoftwareInterrupt(void)
{
#if defined (CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	struct softnet_data *sd;
	unsigned long flag;
	unsigned int qlen;

	sd = &per_cpu(softnet_data, smp_processor_id());
	local_irq_save(flag);
	spin_lock(&sd->input_pkt_queue.lock);

	qlen = skb_queue_len(&sd->input_pkt_queue);
#endif
	//linux input queue is empty and skb is enough=>trigger NIC to receive packet 
	if ((atomic_read(&re8670_rxskb_num) < RE8670_MAX_ALLOC_RXSKB_NUM)
#if defined (CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)		
		&&(qlen <= netdev_max_backlog)
#endif		
		)
	{
		//Gmac 0
		if(pauseBySwGmacBitmap & (1<<0))
			RLE0787_W32(0, SWINT_REG, 1<<24);
		//Gmac 1
		if(pauseBySwGmacBitmap & (1<<1))
			RLE0787_W32(1, SWINT_REG, 1<<24);
	}	
	else
	{
		mod_timer(&software_interrupt_timer, jiffies+1);
	}
#if defined (CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)		
	spin_unlock(&sd->input_pkt_queue.lock);
	local_irq_restore(flag);
#endif	
	return ;
}
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
/*
 * RETURN VAL: 	1 fast forwarding successfully
 *                    	0 should go normal path
 */
static int re8670_ff_enter(struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo)
{
	extern void reinit_skbhdr(struct sk_buff *skb, 
						void (*prealloc_cb)(struct sk_buff *, unsigned));
	extern int rteFastForwarding(struct sk_buff *skb);

	skb->dev = decideRxDevice(cp, pRxInfo);
	if ( !ether_addr_equal(skb->data, skb->dev->dev_addr))
	{
		int dir=DIR_LAN;
		
		if (skb->dev->priv_flags & IFF_DOMAIN_WAN) // wan
			dir = DIR_WAN;
		
		if (brgFastForwarding((struct sk_buff *)skb, dir))
		{
			return 1;
		}
	}

	skb_reset_mac_header(skb);
	skb->protocol = ((unsigned short *)(skb->data))[6];
		
	skb_pull(skb, ETH_HLEN);
	skb->dst = NULL;
	
	if (NET_RX_SUCCESS == rteFastForwarding(skb))
	{
		return 1;
	}
	skb_push(skb, ETH_HLEN);

	reinit_skbhdr(skb, rtl865x_free_eth_priv_buf);
	
	return 0;
}
#endif //end of CONFIG_RTL865X_ETH_PRIV_SKB_ADV


#if defined (CONFIG_RTK_VOIP_QOS)
extern int ( *check_voip_channel_loading )( void );
#endif

/*
	we can use this map to decide what sequence we want, that means, queue's priority.
	also, we can use this to decrease some iterations when we split rx interrupt. 
	if u only open rx ring1 and ring6, u can set pri_map = {0,5,1,2,3,4}, and 
	rx_ring_bitmap_current will only has these two bits as well, so we don't need to run "for"
	6 times.
	*/
#ifdef RX_MRING_INT_SPLIT
#ifdef CONFIG_DUALBAND_CONCURRENT
	unsigned char pri_map[MAX_GMAC_NUM][MAX_RXRING_NUM] = {{2,1,0,0,0,0}, {2,1,0,0,0,0}, {2,1,0,0,0,0}};
	unsigned char higherq[MAX_GMAC_NUM][MAX_RXRING_NUM] = {{0x06,0x04,0x0,0x0,0x0,0x0}, {0x06,0x04,0x0,0x0,0x0,0x0}, {0x06,0x04,0x0,0x0,0x0,0x0}};
#else
	unsigned char pri_map[MAX_GMAC_NUM][MAX_RXRING_NUM] = {{5,4,3,2,1,0}, {5,4,3,2,1,0}, {5,4,3,2,1,0}};
	unsigned char higherq[MAX_GMAC_NUM][MAX_RXRING_NUM] = {{0x3e,0x3c,0x38,0x30,0x20,0x00}, {0x3e,0x3c,0x38,0x30,0x20,0x00}, {0x3e,0x3c,0x38,0x30,0x20,0x00}};
#endif
#endif
static int loop_wdt=0;
__IRAM_NIC
static void re8670_rx (struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	int rx_work;
	unsigned long flags;  
	int ring_num=0;
	unsigned rx_Mtail;    
	unsigned long expiry = jiffies + (2 * HZ);

#if defined (CONFIG_RTK_VOIP_QOS)
	unsigned long start_time = jiffies;
	int pkt_rcv_cnt = 0;
#endif
#ifdef CONFIG_RTK_HOST_SPEEDUP
	unsigned int unAckSegmentExist = 0;
#endif

#ifdef RX_MRING_INT_SPLIT
	static unsigned char rx_ring_backup = 0;
	unsigned char rx_ring_bitmap_current=getRxRingBitMap(cp);
	int i = 0, max_rx_ring_num;
#endif
	
	spin_lock_irqsave(&cp->rx_lock,flags); 

	//protect eth rx while reboot
	if(unlikely(eth_close[gmac] == 1)){
		unmask_rx_int(gmac);
		spin_unlock_irqrestore (&cp->rx_lock, flags);   
		return;
	}
	
#ifdef RX_MRING_INT_SPLIT
	rx_ring_bitmap_current|=rx_ring_backup;
	rx_ring_backup=0;
	max_rx_ring_num = rx_not_only_ring1[gmac]?MAX_RXRING_NUM:1;
	for(i=0;rx_ring_bitmap_current && i<max_rx_ring_num;i++){
		if(rx_not_only_ring1[gmac]) {
			ring_num = pri_map[gmac][i];
			if(NOT_IN_BITMAP(rx_ring_bitmap_current, ring_num)){
				continue;
			}
			rx_ring_bitmap_current&=~(1<<ring_num);
		}
#endif		
		rx_Mtail = cp->rx_Mtail[ring_num];   

		// Kaohj
		if (unlikely(eth_poll)) // in poll mode
			rx_work = 2; // rx rate is rx_work * 100 pps (timer tick is 10ms)
		else
			rx_work = re8670_rx_ring_size[gmac][ring_num];
		#if defined(CONFIG_RTK_VOIP_QOS)
		start_time = jiffies;
		pkt_rcv_cnt = 0;
		#endif
		
		while (rx_work--)
		{
			u32 len;
			struct sk_buff *skb, *new_skb;
#ifdef CONFIG_RG_JUMBO_FRAME
			struct sk_buff *orig_skb;
#endif
			DMA_RX_DESC *desc;
			unsigned buflen;
			struct rx_info rxInfo;
			loop_wdt++;	
			if (loop_wdt > 1024) {
				#ifdef CONFIG_LUNA_WDT_KTHREAD
				extern void luna_watchdog_kick(void);
				luna_watchdog_kick();
				loop_wdt = 0;
				#endif 
			}
			if (unlikely(time_is_before_jiffies(expiry))) {
				printk_once("nic-rx: rx time exceed %lu/%lu\n",expiry, jiffies);
				break;
			}

			if(pauseBySwEnable)
			{
#if defined (CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)			
				struct softnet_data *sd;
				unsigned long flag;
				unsigned int qlen;
				//The packet will be dropped by Linux PS when it is sent to PS and PS input queue is full, so pause receiving packet to SW.
			
				sd = &per_cpu(softnet_data, smp_processor_id());
				local_irq_save(flag);
				spin_lock(&sd->input_pkt_queue.lock);

				qlen = skb_queue_len(&sd->input_pkt_queue);
				
				if (qlen > netdev_max_backlog)
				{
					pauseBySwRingBitmap[gmac] |= (1<<ring_num);
					pauseBySwGmacBitmap |= (1<<gmac);
					if(!timer_pending(&software_interrupt_timer))
						mod_timer(&software_interrupt_timer, jiffies+1);
					spin_unlock(&sd->input_pkt_queue.lock);
					local_irq_restore(flag);
					break;
				}
				spin_unlock(&sd->input_pkt_queue.lock);
				local_irq_restore(flag);
#endif			
				pauseBySwRingBitmap[gmac] &= ~(1<<ring_num);
				pauseBySwGmacBitmap &= ~(1<<gmac);
			}
			
#if defined (CONFIG_RTK_VOIP_QOS)
			if( (pkt_rcv_cnt++ > 100 || (jiffies - start_time) >= 1)&& check_voip_channel_loading && check_voip_channel_loading() > 0)
			{
				break; 
			}
#endif

#ifdef RX_MRING_INT_SPLIT
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if (!isHostSpeedUpEnable())
			{
#endif
				/* if higher queue has packet, quit and restart */
				if(rx_not_only_ring1[gmac]) {
					if (higherq[gmac][ring_num] && (retrive_isr1_status(gmac)&higherq[gmac][ring_num])) {
						rx_ring_backup=(1<<ring_num)|rx_ring_bitmap_current; /* restore */
						i = MAX_RXRING_NUM;
						break;				
					}
				}
#ifdef CONFIG_RTK_HOST_SPEEDUP
			}
#endif
#endif 
			desc = &cp->rx_Mring[ring_num][rx_Mtail];	
			retriveRxInfo(desc, &rxInfo);	

			if (rxInfo.opts1.bit.own)
			{				
				break;
			}

			mips_perf_measure_entrance(MIPS_PERF_NIC_RX);

            cp->cp_stats.rx_hw_num++;
            skb = cp->rx_skb[ring_num][rx_Mtail].skb;
            if (unlikely(!skb))
            	BUG();

			RXINFO_DBG(gmac, &rxInfo, ring_num);
			
			len = rxInfo.opts1.bit.data_length & 0x0fff;		//minus CRC 4 bytes later
	
			if (unlikely(rxInfo.opts1.bit.rcdf)){		//DMA error
#ifdef CONFIG_RG_JUMBO_FRAME
				if(re_private_data[gmac].jumboLength>0)		//flush jumbo skb
				{
					if(re_private_data[gmac].jumboFrame) 
						dev_kfree_skb_any(re_private_data[gmac].jumboFrame);
					re_private_data[gmac].jumboLength=0;
					re_private_data[gmac].jumboFrame=NULL;
				}
#endif
				cp->cp_stats.rcdf++;
				goto rx_next;
			}
			if (unlikely(rxInfo.opts1.bit.crcerr)){		//CRC error
#ifdef CONFIG_RG_JUMBO_FRAME
				if(re_private_data[gmac].jumboLength>0)		//flush jumbo skb
				{
					if(re_private_data[gmac].jumboFrame) 
						dev_kfree_skb_any(re_private_data[gmac].jumboFrame);
					re_private_data[gmac].jumboLength=0;
					re_private_data[gmac].jumboFrame=NULL;
				}
#endif
				cp->cp_stats.crcerr++;
				goto rx_next;
			}

			buflen = cp->rx_buf_sz + RX_OFFSET;
#ifdef CONFIG_RG_JUMBO_FRAME
			if(rxInfo.opts1.bit.fs==1)	//FS=1
			{
				if(rxInfo.opts1.bit.ls==1) ////FS=1, LS=1
				{
					orig_skb=NULL;
					goto rx_to_software;
				}
				else //FS=1, LS=0
				{				
					orig_skb=skb;
					//printk("FS=1, LS=0, first frag skb is %d\n",len);
					//memDump(skb->data+RX_OFFSET,64,"skb_start_64");
					//memDump(skb->data+RX_OFFSET+len-16,16,"skb_last_16");

					if(re_private_data[gmac].jumboLength>0)		//flush jumbo skb
					{
						if(re_private_data[gmac].jumboFrame) 
							dev_kfree_skb_any(re_private_data[gmac].jumboFrame);
						re_private_data[gmac].jumboLength=0;
						re_private_data[gmac].jumboFrame=NULL;
					}
					
					//Init global variables, copy the skb into jumboFrame and free the frag one
					re_private_data[gmac].jumboFrame = dev_alloc_skb(JUMBO_SKB_BUF_SIZE);
					if (unlikely(!re_private_data[gmac].jumboFrame)) {
						printk("there is no memory for jumbo frame\n");
						cp->cp_stats.rx_no_mem++;
						dma_cache_wback_inv((unsigned long)skb->data,SKB_BUF_SIZE);
						re_private_data[gmac].jumboLength=0;
						goto rx_next;
					}
					re_private_data[gmac].jumboLength=len;
					
					//Copy skb into jumboFrame
					memcpy(re_private_data[gmac].jumboFrame->data+RX_OFFSET,skb->data+RX_OFFSET,len);
				}

			}
			else 	//FS=0
			{
				orig_skb=skb; //keep
				//if the length is zero, JumboFrame is not allocated...
				if(unlikely(re_private_data[gmac].jumboLength==0))
				{
					printk("the first skb is not entered or error...\n");
					cp->cp_stats.frag++;
				}
				else
				{
					if(unlikely(re_private_data[gmac].jumboLength+len>=JUMBO_SKB_BUF_SIZE))
					{
						if(re_private_data[gmac].jumboFrame) 
							dev_kfree_skb_any(re_private_data[gmac].jumboFrame);
						re_private_data[gmac].jumboLength=0;
						re_private_data[gmac].jumboFrame=NULL;
						cp->cp_stats.toobig++;
					}
					else
					{
						//Copy into jumboFrame, free the skb						
						memcpy(re_private_data[gmac].jumboFrame->data+RX_OFFSET+re_private_data[gmac].jumboLength,skb->data,len);
						re_private_data[gmac].jumboLength+=len;
						
						if(rxInfo.opts1.bit.ls==1)		//FS=0, LS=1
						{
							//printk("FS=0, LS=1, last frag skb is %d\n",len);
							//memDump(skb->data,64,"skb_start_64");
							//memDump(skb->data+len-16,16,"skb_last_16");
							//dev_kfree_skb_any(skb);							
							skb=re_private_data[gmac].jumboFrame;
							len=re_private_data[gmac].jumboLength;							
							rxInfo.opts1.bit.data_length=re_private_data[gmac].jumboLength & 0x3fff;		//at most 14 bits
							re_private_data[gmac].jumboLength=0;
							re_private_data[gmac].jumboFrame=NULL;
							rxInfo.opts1.bit.fs=1;
							dma_cache_wback_inv((unsigned long)skb->data,len);
							goto rx_to_software;
						}
						else
						{
							//printk("FS=0, LS=0 frag skb is %d\n",len);
							//memDump(skb->data,64,"skb_start_64");
							//memDump(skb->data+len-16,16,"skb_last_16");							
						}
						dma_cache_wback_inv((unsigned long)skb->data,SKB_BUF_SIZE);
					}
				}
			}
			goto rx_next;
rx_to_software:
#else
			if (unlikely((rxInfo.opts1.dw & (FirstFrag | LastFrag)) != (FirstFrag | LastFrag))) {
				cp->cp_stats.frag++;
				goto rx_next;
			}
#endif

#ifdef CONFIG_RG_JUMBO_FRAME
			if(orig_skb==NULL) {
				if(rxInfo.opts3.bit.internal_priority!=GMAC_PRIORITY_CRITICAL) {
					new_skb=re8670_getAlloc(SKB_BUF_SIZE);
				} else {
					re_private_data[gmac].cp_stats.rx_critical_num++;
					new_skb=re8670_getCriticalAlloc(SKB_BUF_SIZE);
				}
			}
			else
				new_skb=orig_skb;
#else				
			if(rxInfo.opts3.bit.internal_priority!=GMAC_PRIORITY_CRITICAL) {
				new_skb=re8670_getAlloc(SKB_BUF_SIZE);
			} else {
				re_private_data[gmac].cp_stats.rx_critical_num++;
				new_skb=re8670_getCriticalAlloc(SKB_BUF_SIZE);
				if (!new_skb) {
					re_private_data[gmac].cp_stats.rx_critical_drop_num++;
				}
			}
#endif
			if (unlikely(!new_skb)) {
				cp->cp_stats.rx_no_mem++;
				//printk("nic rx new_skb alloc failed no mem %d\n",atomic_read(&re8670_rxskb_num));
				//dma_cache_wback_inv((unsigned long)skb->data,buflen);
				dma_cache_wback_inv((unsigned long)skb->data,SKB_BUF_SIZE);
				//goto rx_next;
				//The packet will be dropped when run out of skb, so pause receiving packet to SW.
				if(pauseBySwEnable)
				{		
					pauseBySwRingBitmap[gmac] |= (1<<ring_num);
					pauseBySwGmacBitmap |= (1<<gmac);
					if(!timer_pending(&software_interrupt_timer))
						mod_timer(&software_interrupt_timer, jiffies+1);
					break;
				}
				else
					goto rx_next;
			}

			/* Handle checksum offloading for incoming packets. */
			if (re8670_rx_csum_ok(&rxInfo))
				skb->ip_summed = CHECKSUM_UNNECESSARY;
			else
				skb->ip_summed = CHECKSUM_NONE;

			if(rxInfo.opts1.bit.fs==1)
			{
				skb_reserve(skb, RX_OFFSET); // HW DMA start at 4N+2 only in FS.
			}
			len-=4;	//minus CRC 4 bytes here			
			skb_put(skb, len);
			
			SKB_DBG(skb, debug_enable[gmac], RTL8686_SKB_RX);
			RX_TRACE(gmac, "SKB[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d\n",(u32)skb&0xffff
			,skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5]
			,skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11]			
			,(skb->data[12]<<8)|skb->data[13],len);

			cp->rx_Mring[ring_num][rx_Mtail].addr = CPHYSADDR(new_skb->data);
			cp->rx_skb[ring_num][rx_Mtail].skb = new_skb;
			
			//dma_cache_inv((unsigned long)new_skb->data,(u32)new_skb->end-(u32)new_skb->data);
			dma_cache_inv((unsigned long)skb->data,skb->len);

			/*fastpath enter here.*/
#ifndef CONFIG_RTK_L34_ENABLE
	#ifdef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
			if (re8670_ff_enter(cp, skb, &rxInfo))
				goto rx_next;
			//printk("go normal path\n");
	#endif
#endif//end of CONFIG_RTK_L34_ENABLE
			re8670_rx_software(cp, skb, &rxInfo);
rx_next:
			desc->opts1 = (DescOwn | cp->rx_buf_sz) | ((rx_Mtail == (re8670_rx_ring_size[gmac][ring_num] - 1))?RingEnd:0);
			updateGmacFlowControl(gmac, rx_Mtail, ring_num);
			rx_Mtail = NEXT_RX(rx_Mtail, re8670_rx_ring_size[gmac][ring_num]);
			mips_perf_measure_exit(MIPS_PERF_NIC_RX);

		}
		cp->rx_Mtail[ring_num] = rx_Mtail;

#ifdef CONFIG_RTK_HOST_SPEEDUP
		unAckSegmentExist = 0;
		if (isHostSpeedUpEnable())
		{
			/* if unacked segment exist, we should always enable Rx tasklet to waiting for new segment or send ack
			 * when timer expired.
			 */
			unAckSegmentExist = send_delay_ack_timer();
		}
		if ((rx_work <= 0) || unAckSegmentExist)
			tasklet_hi_schedule(&cp->rx_tasklets);
#else
		if(rx_work <= 0){
			tasklet_hi_schedule(&cp->rx_tasklets);
		}
#endif
#ifdef RX_MRING_INT_SPLIT
	}
#endif
	unmask_rx_int(gmac);
	spin_unlock_irqrestore (&cp->rx_lock, flags); 
}

__IRAM_NIC
static irqreturn_t re8670_interrupt(int irq, void * dev_instance, struct pt_regs *regs)
{
	struct net_device *dev = dev_instance;
	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;
	u16 status = read_isr_status(cp);
	
	if (!status)  
	{
		//printk("%s: no status indicated in interrupt, weird!\n", __func__);	//shlee 2.6
		return IRQ_RETVAL(IRQ_NONE);
	}

	if (status & RX_ALL(gmac)) {

		if(status & RER_RUNT){
			cp->cp_stats.rer_runt++;
		}
		if(status & RER_OVF){
			cp->cp_stats.rer_ovf++;
		}
		if(status & RDU_ALL){
			mask_rx_int(gmac);
			cp->cp_stats.rdu++;
			tasklet_hi_schedule(&cp->rx_tasklets);
		}
		if(status & (RX_OK | SW_INT)){
			mask_rx_int(gmac);
			tasklet_hi_schedule(&cp->rx_tasklets);
		}
	}

#ifdef KERNEL_SOCKET
	if(status & TOK)
	{
//		printk("TX OK...recycle\n");
		tasklet_hi_schedule(&cp->tx_tasklets);
	}	
#endif	
	clear_isr(cp, status);

	return IRQ_RETVAL(IRQ_HANDLED);
}

static inline void updateTxStatic(struct sk_buff *skb){
	DEVPRIV(skb->dev)->net_stats.tx_packets++;
	DEVPRIV(skb->dev)->net_stats.tx_bytes += skb->len;
}

#ifdef KERNEL_SOCKET
atomic_t re8670_tx_lock=ATOMIC_INIT(0);
#include <linux/inet.h>

#if 1
#define TCP_SOCKET
#else
#define UDP_SOCKET
#endif

void kernel_socket(struct work_struct *data);


extern atomic_t zero_copy_alloc;
DECLARE_WAIT_QUEUE_HEAD(wqh);

void kernel_socket(struct work_struct *data2)
{
	struct socket *clientsocket=NULL;
	struct sockaddr_in to;
	struct msghdr msg;
	struct iovec iov;
	u8 *data;
	int data_len;
	mm_segment_t oldfs;
	int len;
	long file_size=100*1024*1024;
//	long file_size=512*1024;

	long start_t,end_t;
	

	/* socket to send data */
#ifdef TCP_SOCKET
	int i,error;
	if (sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &clientsocket) < 0) {
		printk( KERN_ERR "server: Error creating tcp clientsocket.n" );
		return;
	}
#elif defined(UDP_SOCKET)
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &clientsocket) < 0) {
		printk( KERN_ERR "server: Error creating udp clientsocket.n" );
		return;
	}
#endif	

	/* generate answer message */
	memset(&to,0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = in_aton("192.168.150.117");  
	to.sin_port = htons((unsigned short)1234);

#ifdef TCP_SOCKET

	error = clientsocket->ops->connect(clientsocket,(struct sockaddr*)&to,sizeof(to),0);
	if(error<0)
	{
		printk("Error connecting client socket to server: %i.\n",error);
		return;
	}

#endif

	memset(&msg,0,sizeof(msg));
	msg.msg_name = &to;
	msg.msg_namelen = sizeof(to);
	
	/* send the message */
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = MSG_NOSIGNAL;//0/*MSG_DONTWAIT*/;


	oldfs = get_fs();

	start_t=jiffies;

#ifdef TCP_SOCKET
	data=(u8*)kmalloc(65535,GFP_KERNEL);
	if(data==NULL) printk("alloc failed\n");
	for(i=0;i<65500;i+=2)
	{
		data[i]=(i>>8)&0xff;
		data[i+1]=(i&0xff);
	}
		
#endif

	while(1)
	{
#ifdef TCP_SOCKET	
#else
		if(atomic_read(&zero_copy_alloc)>262144)
		{
			//wait_event(wqh,(volatile int *)zero_copy_alloc<262144);
			DEFINE_WAIT(__wait);
			for(;;)
			{
				prepare_to_wait(&wqh,&__wait,TASK_INTERRUPTIBLE);
				if(atomic_read(&zero_copy_alloc)<262144)
					break;
				//printk("sleep zero_copy_alloc=%d\n",atomic_read(&zero_copy_alloc));
				//schedule_timeout(500);
				schedule();
			}
//			printk("wake\n");
			finish_wait(&wqh,&__wait);
		}

		data=kmalloc(65535,GFP_KERNEL); //tysu: don't free this memory manually, this memory is free in skbuff.c:skb_release_data()		
		if(data==NULL) printk("alloc failed\n");
//		else printk("kmalloc %x here zero_copy_alloc=%x\n",(u32)data,zero_copy_alloc);	
#endif		

		

#ifdef TCP_SOCKET 
		//data_len=65160; // 65160 is multiple of 1448 (mss) , 1448+12(tcp opt)+40(ip+tcp)+14(eth)=1514
		data_len=64240; // 64240 is multiple of 1460 (mss) , 1460+40(ip+tcp)+14(eth)=1514 (disable tcp timestamp)
#else
		data_len=65400;
#endif
		
		/* send the message */
		iov.iov_base = data;
		iov.iov_len  = data_len;
		
		/* adjust memory boundaries */	
		set_fs(KERNEL_DS);
		
		len = sock_sendmsg(clientsocket, &msg, data_len);
		
		if(len<0) { printk("error at %d\n",len);break;}
		else
		{
//			printk("send len=%d\n",len);

		}
		
		file_size-=len;
		set_fs(oldfs);
		
		if(file_size<=0) break;
	}
	
	end_t=jiffies;


#ifdef TCP_SOCKET
	kfree(data);
#endif
	

	/* free the initial skb */
	printk("finished!! diff_jiffies=%ld\n",end_t-start_t);

	//close
	if (clientsocket)
		sock_release(clientsocket);

}

#endif

void nic_tx_ring_dump(unsigned int gmac);
void nic_rx_ring_dump(unsigned int gmac);

__IRAM_NIC void re8670_tx (struct re_private *cp,int ring_num)
{
	unsigned int gmac = cp->gmac;
	unsigned tx_tail= cp->tx_Mhqtail[ring_num];
	u32 status;
	struct sk_buff *skb;

	if(unlikely(eth_close[gmac]))
		return;
	
#ifdef KERNEL_SOCKET
	if(atomic_read(&re8670_tx_lock)==1) return;
	atomic_set(&re8670_tx_lock,1);
#endif	

	while (!((status = (cp->tx_Mhqring[ring_num][tx_tail].opts1))& DescOwn)) {
		if (tx_tail == cp->tx_Mhqhead[ring_num])
			break;

		skb = cp->tx_skb[ring_num][tx_tail].skb;

#ifdef CONFIG_REALTEK_HW_LSO
		//printk("skb=%x tx_tail=%d\n",(u32)skb,tx_tail);
		while((u32)skb==0xffffffff) //skb is last_frag in mfrag_skb.
		{
			cp->tx_skb[ring_num][tx_tail].skb=NULL;
			tx_tail = NEXT_TX(tx_tail,re8670_tx_ring_size[gmac][ring_num]); //tysu: this skb is many frags skb, just free once.
			skb = cp->tx_skb[ring_num][tx_tail].skb;
			//printk("skb=%x tx_tail=%d\n",(u32)skb,tx_tail);			
			status = cp->tx_Mhqring[ring_num][tx_tail].opts1;			
			if(status & DescOwn) break;
		}

		if(status & DescOwn) break;
		if(skb==NULL) break;
#else
		if (unlikely(!skb))   
			break;
#endif
		updateTxStatic(skb);	

		dev_kfree_skb_any(skb); 
		cp->tx_skb[ring_num][tx_tail].skb = NULL;		
		
		tx_tail = NEXT_TX(tx_tail,re8670_tx_ring_size[gmac][ring_num]);

#ifdef KERNEL_SOCKET		
		wake_up(&wqh);
#endif

	}
	cp->tx_Mhqtail[ring_num]=tx_tail;

	if (netif_queue_stopped(cp->dev) && (TX_HQBUFFS_AVAIL(cp,ring_num) > (MAX_SKB_FRAGS + 1)))
		netif_wake_queue(cp->dev);

#ifdef KERNEL_SOCKET
	atomic_set(&re8670_tx_lock,0);
#endif	

}

#ifdef KERNEL_SOCKET
__IRAM_NIC void re8670_tx_all (struct re_private *cp)
{
	re8670_tx(cp,0);
}
#endif

__IRAM_NIC void checkTXDesc(int ring_num, unsigned int gmac)
{
	struct re_private *cp = DEV2CP(eth_net_dev[gmac]);
	if (likely(!atomic_read(&lock_tx_tail)) && cp!=NULL) {
		atomic_set(&lock_tx_tail, 1);
        re8670_tx(cp,ring_num);
		atomic_set(&lock_tx_tail, 0);
	}
}


void re8670_mFrag_xmit (struct sk_buff *skb, struct re_private *cp, unsigned *entry,int ring_num)
{
	unsigned int gmac = cp->gmac;
	u32 eor;
	DMA_TX_DESC *txd;
	u32 first_len, first_mapping;
	int frag, first_entry = *entry;
	u32 firstFragAddr;
	u32 temp_opt1;

	/* We must give this initial chunk to the device last.
	 * Otherwise we could race with the device.
	 */
	first_len = skb->len - skb->data_len;
	/*first_mapping = pci_map_single(cp->pdev, skb->data,*
				       first_len, PCI_DMA_TODEddVICE);*/
	firstFragAddr = (u32)skb->data;
	//first_mapping = (u32)skb->data|UNCACHE_MASK; //tysu:disable uncache access
	first_mapping = (u32)skb->data;
#ifdef CONFIG_REALTEK_HW_LSO			
	// tysu: skb and desc is one-by-one mapping in old nic driver, but in new nic driver, many descs are using the same skb.
	// we need a flag(skb=0xffffffff) to check the same skb which in different descs must free or not.(when skb=0xffffffff, don't free,else must free)
	cp->tx_skb[ring_num][*entry].skb = (struct sk_buff *)0xffffffff;     
#else
	cp->tx_skb[ring_num][*entry].skb = skb;
#endif
	cp->tx_skb[ring_num][*entry].mapping = (dma_addr_t)first_mapping;
	cp->tx_skb[ring_num][*entry].frag = 1;
	*entry = NEXT_TX(*entry, re8670_tx_ring_size[gmac][ring_num]);

	for (frag = 0; frag < skb_shinfo(skb)->nr_frags; frag++) {
		skb_frag_t *this_frag = &skb_shinfo(skb)->frags[frag];
		u32 len, mapping;			
		u32 ctrl;
					 
#if defined(LINUX_SG_ENABLE) && defined(NIC_DESC_ACCELERATE_FOR_SG)
		u32 first_data_mapping;
		u32 next_mapping;
		skb_frag_t *next_frag = &skb_shinfo(skb)->frags[frag+1];
		if((frag+1) == skb_shinfo(skb)->nr_frags) next_frag=NULL;

		len=0;
		mapping = (u32)page_address(skb_frag_page(this_frag))+(u32)this_frag->page_offset;
		first_data_mapping=mapping;

		while(next_frag)
		{

			next_mapping=(u32)page_address(skb_frag_page(next_frag))+(u32)next_frag->page_offset;
			if(next_mapping==this_frag->size+mapping)
			{
				len += this_frag->size;
				this_frag=next_frag;
				mapping=next_mapping;
				++frag;					
				next_frag=&skb_shinfo(skb)->frags[frag+1];
				if((frag+1) == skb_shinfo(skb)->nr_frags) next_frag=NULL;
			}
			else //the data is not continuously
			{					
				break;
			}
		}
		len+=this_frag->size;
		//first_data_mapping |= UNCACHE_MASK; //tysu: disable uncache
		mapping=first_data_mapping;
#else
		len = this_frag->size;
		//mapping = (u32)page_address(this_frag->page)+(u32)this_frag->page_offset|UNCACHE_MASK;
		mapping = (u32)page_address(this_frag->page.p)+(u32)this_frag->page_offset; //tysu: disable uncache
#endif

		eor = (*entry == (re8670_tx_ring_size[gmac][ring_num] - 1)) ? RingEnd : 0;
		ctrl = eor | len | DescOwn | TxCRC;
		if (frag == skb_shinfo(skb)->nr_frags - 1)
			ctrl |= LastFrag;

		txd = &cp->tx_Mhqring[ring_num][*entry];
		
		//CP_VLAN_TX_TAG(txd, vlan_tag);

		txd->addr = (mapping);
		//msg_queue_print("i=%x %x s=%x %x l=%d\n",*(u8*)(txd->addr+0x12),*(u8*)(txd->addr+0x13),*(u8*)(txd->addr+0x32),*(u8*)(txd->addr+0x33),len);
		//wmb();
		
#ifdef HW_CHECKSUM_OFFLOAD
		ctrl|= ( IPCS | L4CS);	
#endif

#ifdef HW_LSO_ENABLE
	if(skb->len-14>CP_LS_MTU)
	{
		txd->opts4 = 0x80000000 | (CP_LS_MTU<<20); //real MTU
	}
	else
		txd->opts4=0;
#endif
		
		dma_cache_wback_inv((unsigned long)mapping, len);

#ifdef TX_DESC_UNCACHE
		txd->opts1=ctrl;
#else
		txd->opts1=0; //make sure own not set
		dma_cache_wback_inv((u32)&txd->opts1,sizeof(DMA_TX_DESC));
		*(u32 *)((u32)&txd->opts1|0xa0000000) = ctrl; // tysu: own must write at last.
#endif
		
		//wmb();
		//cp->tx_skb[*entry].skb = skb;
		if(frag == skb_shinfo(skb)->nr_frags-1)
			cp->tx_skb[ring_num][*entry].skb = skb;
		else
			cp->tx_skb[ring_num][*entry].skb = (struct sk_buff *)0xffffffff;  // tysu: make sure the same skb is not free many times.

		//msg_queue_print("i=%x %x s=%x %x l=%d\n",*(u8*)(txd->addr+0x12),*(u8*)(txd->addr+0x13),*(u8*)(txd->addr+0x32),*(u8*)(txd->addr+0x33),len);
	
#ifdef LINUX_SG_ENABLE				
		cp->tx_skb[ring_num][*entry].mapping = first_data_mapping;
#else
		cp->tx_skb[ring_num][*entry].mapping = mapping;
#endif
		cp->tx_skb[ring_num][*entry].frag = frag + 2;
		*entry = NEXT_TX(*entry, re8670_tx_ring_size[gmac][ring_num]);
	}

	txd = &cp->tx_Mhqring[ring_num][first_entry];
//	CP_VLAN_TX_TAG(txd, vlan_tag);
	
	txd->addr = (first_mapping);
	//wmb();
	eor = (first_entry == (re8670_tx_ring_size[gmac][ring_num] - 1)) ? RingEnd : 0;
#ifdef HW_CHECKSUM_OFFLOAD
	temp_opt1 = (first_len | FirstFrag | DescOwn|TxCRC|eor|IPCS | L4CS);
#else
	temp_opt1 = (first_len | FirstFrag | DescOwn|TxCRC|eor);
#endif
#ifdef TEST_HW_VLAN_TAG_OFFLOAD
#ifdef RLE0371
	txd->opts2 = 0x02000700; // vid=7, VIDL=8bits,PRI=3bits,CFI=1bit,VIDH=4bits
#else
	txd->opts2 = 0x10700; // vid=7, VIDL=8bits,PRI=3bits,CFI=1bit,VIDH=4bits
#endif		

#endif
#ifdef HW_LSO_ENABLE
	if(skb->len-14>CP_LS_MTU)
	{
		txd->opts4 = 0x80000000 | (CP_LS_MTU<<20); //real MTU
	}
	else
		txd->opts4=0;
#endif
	dma_cache_wback_inv((unsigned long)firstFragAddr, first_len);
#ifdef TX_DESC_UNCACHE
	txd->opts1=temp_opt1;
#else
	txd->opts1=0;
	dma_cache_wback_inv((unsigned long)&txd->opts1, sizeof(DMA_TX_DESC));
	*(u32 *)((u32)&txd->opts1|0xa0000000) =temp_opt1; //tysu: own must write at last.		
#endif		

//		memDump(&txd->opts1,16,"desc");
//	printk("first len=%d first_entry=0x%x\n",first_len,first_entry);
//		memDump(firstFragAddr,128,"first frag data");

	//wmb();
}

#ifdef CONFIG_PORT_MIRROR
void nic_tx_mirror(struct sk_buff *skb)
{
	struct net_device *dev = skb->dev;
	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;
	
	re8670_start_xmit(skb, eth_net_dev[gmac]);
}
#endif

static 
__IRAM_NIC
void	tx_additional_setting(struct sk_buff *skb, struct net_device *dev, struct tx_info *pTxInfo)
{
#ifndef RE8686_VERIFY
	_tx_additional_setting(skb, dev, pTxInfo);
#else
	tx_additional_setting_verify_version(skb, dev, pTxInfo);
#endif
}

static inline void kick_tx(unsigned int gmac, int ring_num){	
	ring_num = idx_sw2hw(ring_num);
	
	switch(ring_num) {
		case 0:
		case 1:
		case 2:
		case 3:
			RLE0787_W32(gmac, IO_CMD, RLE0787_R32(gmac, IO_CMD) | (1 << ring_num));
			break;
		case 4:
			RLE0787_W32(gmac, IO_CMD1, RLE0787_R32(gmac, IO_CMD1) | TX_POLL5);
			break;
		default:
			printk("%s %d: wrong ring num %d\n", __func__, __LINE__, ring_num);
			break;
	}
}

static inline void apply_to_txdesc(DMA_TX_DESC  *txd, struct tx_info *pTxInfo){
	txd->addr = pTxInfo->addr;
	txd->opts2 = pTxInfo->opts2.dw;
	txd->opts3 = pTxInfo->opts3.dw;
	txd->opts4 = pTxInfo->opts4.dw;
	//must be last write....
	wmb();
	txd->opts1 = pTxInfo->opts1.dw;
}

__IRAM_NIC
void do_txInfoMask(struct tx_info* pTxInfo, struct tx_info* ptx, struct tx_info* ptxMask){
	if(ptxMask && ptx){
		if(ptxMask->opts1.dw)
		{
			pTxInfo->opts1.dw &= (~ptxMask->opts1.dw);
			pTxInfo->opts1.dw |= (ptx->opts1.dw & ptxMask->opts1.dw);
		}
		if(ptxMask->opts2.dw)
		{
			pTxInfo->opts2.dw &= (~ptxMask->opts2.dw);
			pTxInfo->opts2.dw |= (ptx->opts2.dw & ptxMask->opts2.dw);
		}
		if(ptxMask->opts3.dw)
		{
			pTxInfo->opts3.dw &= (~ptxMask->opts3.dw);
			pTxInfo->opts3.dw |= (ptx->opts3.dw & ptxMask->opts3.dw);
		}
		if(ptxMask->opts4.dw)
		{
			pTxInfo->opts4.dw &= (~ptxMask->opts4.dw);
			pTxInfo->opts4.dw |= (ptx->opts4.dw & ptxMask->opts4.dw);
		}
	}
}

int re8686_send_with_txInfo_and_mask(struct sk_buff *skb, struct tx_info* ptxInfo, int ring_num, struct tx_info* ptxInfoMask)
{
	//struct net_device *dev = eth_net_dev[txAlwaysGmac];
	//struct net_device *dev = eth_net_dev[(smp_processor_id()<2)?1:0];
	struct net_device *dev = eth_net_dev[ptxInfo->opts3.bit.gmac_id];

	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;
	unsigned entry;
	u32 eor;
	unsigned long flags;
	struct tx_info local_txInfo={{{0}},0,{{0}},{{0}}};

	spin_lock_irqsave(&cp->tx_lock, flags);
	skb->dev = dev;
	//save ptxInfo, now we only need to save opts1 and opts2
	if(ptxInfo!=NULL)memcpy(&local_txInfo, ptxInfo, sizeof(struct tx_info));
	
	ETHDBG_PRINT( gmac, RTL8686_SKB_TX, "tx %s\n", dev->name );
	SKB_DBG(skb, debug_enable[gmac], RTL8686_SKB_TX);
	cp->cp_stats.tx_sw_num++;
	checkTXDesc(ring_num, gmac);

	if(unlikely(eth_close[gmac] == 1)) {
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		spin_unlock_irqrestore(&cp->tx_lock, flags);
		return -1;
	}

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= (skb_shinfo(skb)->nr_frags + 1)))
	{
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		cp->cp_stats.tx_no_desc++;
		spin_unlock_irqrestore(&cp->tx_lock, flags);
		return -1;
			}
			
	entry = cp->tx_Mhqhead[ring_num];    

	eor = (entry == (re8670_tx_ring_size[gmac][ring_num] - 1)) ? RingEnd : 0;
	if (skb_shinfo(skb)->nr_frags == 0) {
		u32 len;
		DMA_TX_DESC  *txd;
		txd = &cp->tx_Mhqring[ring_num][entry];    

		len = skb->len;
		if(len > 0)		
		// Kaohj --- invalidate DCache before NIC DMA
			dma_cache_wback_inv((unsigned long)skb->data, len);

		//default setting, always need this
		local_txInfo.addr = (u32)(skb->data);

		//[step1] opts1 clear & set init value
		local_txInfo.opts1.dw &= ~(RingEnd|0x1ffff|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);
		local_txInfo.opts1.dw |= (eor|len|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);

		//[step2] opts1 might be changed by some additional setting or parameter ptxInfo
		//plz put tx additional setting into this function
		tx_additional_setting(skb, dev, &local_txInfo);
		do_txInfoMask(&local_txInfo, ptxInfo, ptxInfoMask);

		//[step3] remove IPCS&L4CS in opts1 condictionally 
		if(txJumboFrameEnabled[gmac])
		{
			local_txInfo.opts1.dw &= ~((skb->len > SKB_BUF_SIZE)?(IPCS|L4CS):0);
		}

		TXINFO_DBG(gmac, &local_txInfo);
		
		//apply to txdesc
		apply_to_txdesc(txd, &local_txInfo);
		cp->tx_skb[ring_num][entry].skb = skb;
		cp->tx_skb[ring_num][entry].mapping = (dma_addr_t)(skb->data);
		cp->tx_skb[ring_num][entry].frag = 0;

		TX_TRACE(gmac, "FROM_FWD[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d VlanAct=%d Vlan=%d Pri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x PON_SID=%d\n",
		(u32)skb&0xffff,
		skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],
		skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11],
		(skb->data[12]<<8)|skb->data[13],skb->len,
		local_txInfo.opts2.bit.tx_cvlan_action,
		local_txInfo.opts2.bit.cvlan_vidh<<8|local_txInfo.opts2.bit.cvlan_vidl,
		local_txInfo.opts2.bit.cvlan_prio,
		local_txInfo.opts3.bit.extspa,
		local_txInfo.opts2.bit.tx_portmask,
		local_txInfo.opts3.bit.l34_keep,
		local_txInfo.opts3.bit.tx_dst_stream_id);

		entry = NEXT_TX(entry, re8670_tx_ring_size[gmac][ring_num]);
	} else {
#ifdef CONFIG_REALTEK_HW_LSO	
		re8670_mFrag_xmit(skb, cp, &entry, ring_num);
#else
		printk("%s %d: not support frag xmit now\n", __func__, __LINE__);
		dev_kfree_skb_any(skb);
#endif
		}
	cp->tx_Mhqhead[ring_num] = entry;

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= 1))
		netif_stop_queue(dev);

	//for memory controller's write buffer
	write_buffer_flush();

	spin_unlock_irqrestore(&cp->tx_lock, flags);
	wmb();
	cp->cp_stats.tx_hw_num++;

	kick_tx(gmac, ring_num);
	
	dev->trans_start = jiffies;
	return 0;
}

__IRAM_NIC
int re8686_send_with_txInfo(struct sk_buff *skb, struct tx_info* ptxInfo, int ring_num) 
{
	//struct net_device *dev = eth_net_dev[txAlwaysGmac];
	//struct net_device *dev = eth_net_dev[(smp_processor_id()<2)?1:0];
	struct net_device *dev = eth_net_dev[ptxInfo->opts3.bit.gmac_id];

	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;
	unsigned entry;
	u32 eor;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&cp->tx_lock, flags);
	skb->dev = dev;
	
	ETHDBG_PRINT( gmac, RTL8686_SKB_TX, "tx %s\n", dev->name );
	SKB_DBG(skb, debug_enable[gmac], RTL8686_SKB_TX);
	cp->cp_stats.tx_sw_num++;
	checkTXDesc(ring_num, gmac);

	if(unlikely(eth_close[gmac] == 1)) {
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		spin_unlock_irqrestore(&cp->tx_lock, flags);
		return -1;
	}

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= (skb_shinfo(skb)->nr_frags + 1)))
	{
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		cp->cp_stats.tx_no_desc++;
		spin_unlock_irqrestore(&cp->tx_lock, flags);
		return -1;
	}

	entry = cp->tx_Mhqhead[ring_num];	 

	eor = (entry == (re8670_tx_ring_size[gmac][ring_num] - 1)) ? RingEnd : 0;
	if (skb_shinfo(skb)->nr_frags == 0) {
		u32 len;
		DMA_TX_DESC  *txd;
		txd = &cp->tx_Mhqring[ring_num][entry];    

		len = skb->len;
		if(len > 0) 	
		// Kaohj --- invalidate DCache before NIC DMA
			dma_cache_wback_inv((unsigned long)skb->data, len);

		//default setting, always need this
		ptxInfo->addr = (u32)(skb->data);

		//[step1] opts1 clear & set init value
		ptxInfo->opts1.dw &= ~(RingEnd|0x1ffff|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);
		ptxInfo->opts1.dw |= (eor|len|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);

		//[step2] remove IPCS&L4CS in opts1 condictionally 
		if(txJumboFrameEnabled[gmac])
		{
			ptxInfo->opts1.dw &= ~((skb->len > SKB_BUF_SIZE)?(IPCS|L4CS):0);
		}
		
		ptxInfo->opts2.bit.cputag = 1;

		TXINFO_DBG(gmac, ptxInfo);
		
		//apply to txdesc
		apply_to_txdesc(txd, ptxInfo);
		cp->tx_skb[ring_num][entry].skb = skb;
		cp->tx_skb[ring_num][entry].mapping = (dma_addr_t)(skb->data);
		cp->tx_skb[ring_num][entry].frag = 0;

		TX_TRACE(gmac, "FROM_FWD[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d VlanAct=%d Vlan=%d Pri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x PON_SID=%d\n",
		(u32)skb&0xffff,
		skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],
		skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11],
		(skb->data[12]<<8)|skb->data[13],skb->len,
		ptxInfo->opts2.bit.tx_cvlan_action,
		ptxInfo->opts2.bit.cvlan_vidh<<8|ptxInfo->opts2.bit.cvlan_vidl,
		ptxInfo->opts2.bit.cvlan_prio,
		ptxInfo->opts3.bit.extspa,
		ptxInfo->opts2.bit.tx_portmask,
		ptxInfo->opts3.bit.l34_keep,
		ptxInfo->opts3.bit.tx_dst_stream_id);

		entry = NEXT_TX(entry, re8670_tx_ring_size[gmac][ring_num]);
	} else {
#ifdef CONFIG_REALTEK_HW_LSO	
		re8670_mFrag_xmit(skb, cp, &entry, ring_num);
#else
		printk("%s %d: not support frag xmit now\n", __func__, __LINE__);
		dev_kfree_skb_any(skb);
#endif
	}
	cp->tx_Mhqhead[ring_num] = entry;

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= 1)) {
		netif_stop_queue(dev);
		ret = -2;
	}

	//for memory controller's write buffer
	write_buffer_flush();

	spin_unlock_irqrestore(&cp->tx_lock, flags);
	wmb();
	cp->cp_stats.tx_hw_num++;

	kick_tx(gmac, ring_num);

	dev->trans_start = jiffies;
	return ret;
}

/*
*	return 0 -> set OK, return -1 -> set FAIL
*	ptxInfo: pointer of tx_info(ptxInfo->opts3.bit.gmac_id would be used)
*	reg_num: 0 to set VLAN_REG, 1 to set VLAN1_REG
*	value: the value to set VLAN_REG/ VLAN1_REG
*/
__IRAM_NIC
int re8686_set_vlan_register(struct tx_info* ptxInfo, unsigned char reg_num, unsigned int value) 
{
	struct net_device *dev = eth_net_dev[ptxInfo->opts3.bit.gmac_id];
	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;
	
	if(!value)
	{
		printk(" %s %d: wrong VLAN register value=%d\n", __func__, __LINE__, value);
		return -1;
	}

	switch(reg_num)
	{
		case 0:
			re_private_data[gmac].stag_pid = value;
			RLE0787_W32(gmac, VLAN_REG, value);
			break;
		case 1:
			re_private_data[gmac].stag_pid1 = value;
			RLE0787_W32(gmac, VLAN1_REG, value);
			break;
		default:
			printk(" %s %d: wrong reg_num=%d\n", __func__, __LINE__, reg_num);
			return -1;
	}

	return 0;
}

/*
*	return 0 -> get OK, return -1 -> get FAIL
*	ptxInfo: pointer of tx_info(ptxInfo->opts3.bit.gmac_id would be used)
*	reg_num: 0 to get VLAN_REG, 1 to get VLAN1_REG
*	value_p: the pointer to get content of VLAN_REG/ VLAN1_REG
*/
__IRAM_NIC
int re8686_get_vlan_register(struct tx_info* ptxInfo, unsigned char reg_num, unsigned int *value_p) 
{
	struct net_device *dev = eth_net_dev[ptxInfo->opts3.bit.gmac_id];
	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;

	if(value_p==NULL)
	{
		printk(" %s %d: wrong VLAN register value pointer=NULL\n", __func__, __LINE__);
		return -1;
	}

	switch(reg_num)
	{
		case 0:
			*value_p = RLE0787_R32(gmac, VLAN_REG);
			break;
		case 1:
			*value_p = RLE0787_R32(gmac, VLAN1_REG);
			break;
		default:
			printk(" %s %d: wrong reg_num=%d\n", __func__, __LINE__, reg_num);
			return -1;
	}

	return 0;
}

#if 0//def CONFIG_APOLLO_ROMEDRIVER
int re8670_start_xmit_common(struct sk_buff *skb, struct net_device *dev, struct tx_info *ptx, struct tx_info *ptxMask);

__IRAM_NIC int rtk_rg_fwdEngine_xmit (struct sk_buff *skb, void *void_ptx, void *void_ptxMask) // (void *) using (rtk_rg_txdesc_t *) casting in romeDriver, using (struct tx_info*) casting in re8686.c.
{
	struct tx_info *ptx=(struct tx_info *)void_ptx;
	struct tx_info *ptxMask=(struct tx_info *)void_ptxMask;			
	skb->cb[0]=1;
	skb->dev = rtl8686_dev_table[0].dev_instant;
	re8670_start_xmit_common(skb,skb->dev,ptx,ptxMask);	
	return 0;
}

__IRAM_NIC int re8670_start_xmit (struct sk_buff *skb, struct net_device *dev)	//shlee temp, fix this later
{
	skb->cb[0]=0;
	re8670_start_xmit_common(skb,skb->dev,NULL,NULL);
	return 0;
}

__IRAM_NIC int re8670_start_xmit_common(struct sk_buff *skb, struct net_device *dev, struct tx_info *ptx, struct tx_info *ptxMask)
#else
#if defined(CONFIG_RTK_L34_ENABLE) && defined(CONFIG_APOLLO_GPON_FPGATEST) 
extern int _rtk_rg_virtualMAC_with_PON_get(void);
#endif
__IRAM_NIC int re8670_start_xmit_txInfo (struct sk_buff *skb, struct net_device *dev, struct tx_info* ptxInfo, struct tx_info* ptxInfoMask)	//luke
#endif
{
	struct re_private *cp = &re_private_data[txAlwaysGmac];
	unsigned int gmac = cp->gmac;
	unsigned entry;
	u32 eor;
	unsigned long flags;
	struct tx_info txInfo;
	int ring_num=0;

	memset(&txInfo, 0, sizeof(struct tx_info));
	ETHDBG_PRINT( gmac, RTL8686_SKB_TX, "tx %s\n", dev->name );
	
	spin_lock_irqsave(&cp->tx_lock, flags);
	SKB_DBG(skb, debug_enable[gmac], RTL8686_SKB_TX);
	cp->cp_stats.tx_sw_num++;
	checkTXDesc(ring_num, gmac);

	if(unlikely(eth_close[gmac] == 1 || INVERIFYMODE)) {
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;			
		spin_unlock_irqrestore(&cp->tx_lock, flags);
		return 0;
	}

	if(skb->len > SKB_BUF_SIZE) // drop jumbo frame
	{
		if(!txJumboFrameEnabled[gmac])
		{
			//memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "TX SKB LEN > 1522");
			dev_kfree_skb_any(skb);
	        DEVPRIV(skb->dev)->net_stats.tx_dropped++;
			spin_unlock_irqrestore(&cp->tx_lock, flags);
	        return 0;
		}
	}

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= (skb_shinfo(skb)->nr_frags + 1)))
	{
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		cp->cp_stats.tx_no_desc++;		
		spin_unlock_irqrestore(&cp->tx_lock, flags);
		return 0;
	}	

	entry = cp->tx_Mhqhead[ring_num];
	eor = (entry == (re8670_tx_ring_size[gmac][ring_num] - 1)) ? RingEnd : 0;
	if (skb_shinfo(skb)->nr_frags == 0) {

		DMA_TX_DESC  *txd = &cp->tx_Mhqring[ring_num][entry];
		u32 len;

		len = skb->len;	
		// Kaohj --- invalidate DCache before NIC DMA
		dma_cache_wback_inv((unsigned long)skb->data, len);

		//default setting, always need this
		txInfo.addr = CPHYSADDR(skb->data);
		
		//[step1] opts1 set init value
		txInfo.opts1.dw = (eor|len|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);

#ifdef HW_LSO_ENABLE
		txInfo.opts4.dw=0; //set lso=0
#endif	

		//[step2] opts1 might be changed by some additional setting or parameter ptxInfo
#if 0//def CONFIG_APOLLO_ROMEDRIVER
		if(skb->cb[0]) //from RomeDriver
			_rtk_rg_fwdEngineTxDescSetting((void*)&txInfo,(void*)ptx,(void*)ptxMask);
#endif
		//plz put tx additional setting into this function
		tx_additional_setting(skb, dev, &txInfo);		
		do_txInfoMask(&txInfo, ptxInfo, ptxInfoMask);

		//[step3] remove IPCS&L4CS in opts1 condictionally 
		if(txJumboFrameEnabled[gmac])
		{
			txInfo.opts1.dw &= ~((skb->len > SKB_BUF_SIZE)?(IPCS|L4CS):0);
		}

#if defined(CONFIG_RTK_L34_ENABLE) && defined(CONFIG_APOLLO_GPON_FPGATEST) 
		//20150703LUKE: filter packet from protocol stack send to virtualmac mapping portmask!
		txInfo.opts3.bit.tx_portmask&=~(_rtk_rg_virtualMAC_with_PON_get());
#endif

		TXINFO_DBG(gmac, &txInfo);
		//apply to txdesc
		apply_to_txdesc(txd, &txInfo);
		cp->tx_skb[ring_num][entry].skb = skb;
		cp->tx_skb[ring_num][entry].mapping = CPHYSADDR(skb->data);
		cp->tx_skb[ring_num][entry].frag = 0;
		entry = NEXT_TX(entry, re8670_tx_ring_size[gmac][ring_num]);
	} else {
#ifdef CONFIG_REALTEK_HW_LSO	
		re8670_mFrag_xmit(skb, cp, &entry,ring_num);
#else
		printk("%s %d: not support frag xmit now\n", __func__, __LINE__);
		dev_kfree_skb_any(skb);
#endif		
	}
	cp->tx_Mhqhead[ring_num] = entry;

#if 0//krammer close this
	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= 1))
		netif_stop_queue(dev);
#endif

	//for memory controller's write buffer
	write_buffer_flush();

	spin_unlock_irqrestore(&cp->tx_lock, flags);
	wmb();
	cp->cp_stats.tx_hw_num++;
	
	kick_tx(gmac, ring_num);

	TX_TRACE(gmac, "FROM_PS[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d Vlan=%d Pri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x PON_SID=%d\n",
		(u32)skb&0xffff,
		skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],
		skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11],
		(skb->data[12]<<8)|skb->data[13],skb->len,
		txInfo.opts2.bit.cvlan_vidh<<8|txInfo.opts2.bit.cvlan_vidl,
		txInfo.opts2.bit.cvlan_prio,
		txInfo.opts3.bit.extspa,
		txInfo.opts2.bit.tx_portmask,
		txInfo.opts3.bit.l34_keep,
		txInfo.opts3.bit.tx_dst_stream_id);
	
	dev->trans_start = jiffies;
	return 0;
}

__IRAM_NIC int re8670_start_xmit (struct sk_buff *skb, struct net_device *dev)	//shlee temp, fix this later
{
	return re8670_start_xmit_txInfo(skb,dev,NULL,NULL);
}

/* Set or clear the multicast filter for this adaptor.
   This routine is not state sensitive and need not be SMP locked. */
static void __re8670_set_rx_mode (struct net_device *dev)
{
	struct re_private *cp = DEV2CP(dev);
	unsigned int gmac = cp->gmac;
	int rx_mode;

	rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys | AcceptAllPhys;

	/* We can safely update without stopping the chip. */
	// Kao, 2004/01/07
	RLE0787_W32(gmac, MAR0, 0xFFFFFFFF);
	RLE0787_W32(gmac, MAR4, 0xFFFFFFFF);
	RLE0787_W32(gmac, RCR, rx_mode);
}

static void re8670_set_rx_mode (struct net_device *dev)
{
	unsigned long flags;
	struct re_private *cp = DEV2CP(dev);
	
	spin_lock_irqsave (&cp->rx_lock, flags);
	__re8670_set_rx_mode(dev);
	spin_unlock_irqrestore (&cp->rx_lock, flags);
}

static struct net_device_stats *re8670_get_stats(struct net_device *dev)
{
	return &(DEVPRIV(dev)->net_stats);
}

static void re8670_init_trx_cdo(struct re_private *cp)
{
	int j;
	for(j=0;j<MAX_TXRING_NUM;j++){
		cp->tx_Mhqhead[j] = cp->tx_Mhqtail[j] = 0;
	}	

	for(j=0;j<MAX_RXRING_NUM;j++){
		cp->rx_Mtail[j] = 0;		
	}	
}

static void re8670_stop_hw (struct re_private *cp)
{
	unsigned int gmac=cp->gmac;
	int j;
	RLE0787_W32(gmac, IO_CMD, 0); /* timer  rx int 1 packets*/
	RLE0787_W32(gmac, IO_CMD1, 0); //czyao
	RLE0787_W16(gmac, IMR, 0);
	RLE0787_W32(gmac, IMR0, 0);
	RLE0787_W16(gmac, ISR, 0xffff);
	RLE0787_W32(gmac, ISR1, 0xffffffff);
	//synchronize_irq();
	synchronize_irq(cp->dev->irq);/*linux-2.6.19*/
	udelay(10);

	for(j=0;j<MAX_RXRING_NUM;j++)
		cp->rx_Mtail[j] = 0;		
	for(j=0;j<MAX_TXRING_NUM;j++)
		cp->tx_Mhqhead[j] = cp->tx_Mhqtail[j] = 0;
}

static unsigned int re8670_get_irq_number (struct re_private *cp)
{
	unsigned int gmac=cp->gmac;
	unsigned int irq_number;
	
	switch(gmac) {
		case 0:
			irq_number = BSP_INT0_GMAC0_IRQ;
			break;
		case 1:
			irq_number = BSP_INT0_GMAC1_IRQ;
			break;
		case 2:
			irq_number = BSP_INT0_GMAC2_IRQ;
			break;
		default:
			irq_number = 0;
	}
	return irq_number;
}

static void re8670_ip_enable_all(void)
{
	REG32(BSP_IP_SEL) |= BSP_EN_GMAC;
	#ifdef CONFIG_GMAC1_USABLE
	REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC1;
	#endif
	#ifdef CONFIG_GMAC2_USABLE
	REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC2;
	#endif
}

static void re8670_reset_hw (struct re_private *cp)
	{
	unsigned int gmac = cp->gmac;
	
	/* After apollo use this for totaly gmac reset
	, in old method, mring can't receive packet at first time packet coming */
	disable_irq(cp->dev->irq);
	switch(gmac) {
		case 0:
			REG32(BSP_IP_SEL) &= ~BSP_EN_GMAC;
			mdelay(10);
	REG32(BSP_IP_SEL) |= BSP_EN_GMAC;
			break;
		case 1:
			REG32(NEW_BSP_IP_SEL) &= ~BSP_EN_GMAC1;
			mdelay(10);
	REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC1;
			break;
		case 2:
			REG32(NEW_BSP_IP_SEL) &= ~BSP_EN_GMAC2;
			mdelay(10);
	REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC2;
			break;
	}
	enable_irq(cp->dev->irq);
}

static inline void re8670_start_hw (struct re_private *cp)
{
	unsigned int gmac=cp->gmac;

	RLE0787_W32(gmac, IO_CMD1, iocmd1_reg[gmac]);
	RLE0787_W32(gmac, IO_CMD, iocmd_reg[gmac]); /* timer  rx int 1 packets*/
	//printk("Start NIC in Pkt Processor disabled mode.. IO_CMD %x\n", RLE0787_R32(gmac, IO_CMD));	
}

int re8686_set_flow_control(unsigned int gmac, unsigned int ring, unsigned char enable)
{
	u32 reg32_val, ring_size, ring0_size_msk;
	u16 desc_l, desc_h;
	u16 reg16_val;
	#ifndef CONFIG_GMAC1_USABLE
	if (gmac==1)
		goto error;
	#endif 
	#ifndef CONFIG_GMAC2_USABLE
	if (gmac==2)
		goto error;
	#endif 
	
	if(enable)
	{
		ring_size = (re8670_rx_ring_size[gmac][ring]);
	}
	else
	{
		ring_size = ((re8670_rx_ring_size[gmac][ring])<<1);
	}
	
	if(ring==0)
	{
		ring0_size_msk = ring_size-1;
		desc_l = ring0_size_msk&0xff;
		desc_h = (ring0_size_msk&0xf00)>>8;
		reg32_val = (desc_l<<24)|((desc_h)<<4)|(RLE0787_R32(gmac, EthrntRxCPU_Des_Num)&0x00ffff0f);
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num, reg32_val);
		reg32_val = (desc_l<<8)|desc_h|(RLE0787_R32(gmac, RxCDO)&0xffff00f0);
		RLE0787_W32(gmac, RxCDO, reg32_val);		
	}
	else
	{
		reg32_val = ((ring_size&0xfff)-1)|(RLE0787_R32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(ring-1)))&0xfffff000);
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(ring-1)), reg32_val);
		reg16_val = ((ring_size&0xfff)-1);
		RLE0787_W16(gmac, RxRingSize2+(ADDR_OFFSET*(ring-1)), reg16_val);
	}
	
	re8670_rx_flow_control_status[gmac][ring] = enable?ON:OFF;
	return 0;
error:
	printk("%s(%d): ignored gmac=%d\n",__func__,__LINE__,gmac);
	return -1;
}

static void multi_rtx_ring_init(struct re_private *cp)
{
	unsigned int gmac=cp->gmac;
	u32 reg32_val, ring0_size_msk;
	u16 desc_l, desc_h;
	int i;
	
	for(i=0;i<MAX_TXRING_NUM;i++)
	{
		#ifdef CONFIG_DUALBAND_CONCURRENT
		if(!cp->tx_Mhqring[i]){
			continue;
		}
		#endif
		RLE0787_W32(gmac, TxFDP1+(ADDR_OFFSET*i), CPHYSADDR(cp->tx_Mhqring[idx_sw2hw(i)]));
		RLE0787_W16(gmac, TxCDO1+(ADDR_OFFSET*i), 0);
	}
	for(i=0;i<MAX_RXRING_NUM;i++)
	{	
		#ifdef CONFIG_DUALBAND_CONCURRENT
		if(!cp->rx_Mring[i]){
			continue;
		}
		#endif
		/*we set flow control even if we don't enable this queue.........
		this is because we want to prevent triggering flow control of the queue we disable.....*/
		if(i==0)
		{
			ring0_size_msk = (re8670_rx_ring_size[gmac][i])-1;
			desc_l = ring0_size_msk&0xff;
			desc_h = (ring0_size_msk&0xf00)>>8;
			RLE0787_W32(gmac, RxFDP, CPHYSADDR(cp->rx_Mring[0]));
			reg32_val = (desc_l<<24)|((TH_ON_VAL&0xff)<<16)|((TH_OFF_VAL&0xff)<<8)|((desc_h)<<4)|((TH_ON_VAL&0xf00)>>8);
			RLE0787_W32(gmac, EthrntRxCPU_Des_Num, reg32_val);
			reg32_val = (desc_l<<8)|desc_h;
			RLE0787_W32(gmac, RxCDO, reg32_val);
			reg32_val = ((TH_OFF_VAL&0xf00)<<16)|(RLE0787_R32(gmac, Rx_Pse_Des_Thres_h)&0xf0ffffff);
			RLE0787_W32(gmac, Rx_Pse_Des_Thres_h, reg32_val);
		}
		else
		{
			RLE0787_W32(gmac, RxFDP2+(ADDR_OFFSET*(i-1)), CPHYSADDR(cp->rx_Mring[i]));
			RLE0787_W32(gmac, EthrntRxCPU_Des_Wrap2+(ADDR_OFFSET*(i-1)), 0);  //initialize the register content
			RLE0787_W32(gmac, EthrntRxCPU_Des_Wrap2+(ADDR_OFFSET*(i-1)), ((TH_ON_VAL&0xfff)<<16)|(TH_OFF_VAL&0xfff));
			RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(i-1)), (re8670_rx_ring_size[gmac][i]&0xfff)-1);
			RLE0787_W16(gmac, RxRingSize2+(ADDR_OFFSET*(i-1)), (re8670_rx_ring_size[gmac][i]&0xfff)-1);
		}
	}
	// set queue 456 to default setting , otherwise the eth0 can't rx packet (due to gmac flow control)
#ifdef CONFIG_DUALBAND_CONCURRENT
	if(RLE0787_R16(gmac, RxRingSize2+(ADDR_OFFSET*(2))) == 0)
	{
		// queue 4
		RLE0787_W32(gmac, RxFDP2+(ADDR_OFFSET*(2)), CPHYSADDR(cp->default_rx_desc));
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(2)), 1);
		RLE0787_W16(gmac, RxRingSize2+(ADDR_OFFSET*(2)), 1);
	}
	if(RLE0787_R16(gmac, RxRingSize2+(ADDR_OFFSET*(3))) == 0)
	{
		// queue 5
		RLE0787_W32(gmac, RxFDP2+(ADDR_OFFSET*(3)), CPHYSADDR(cp->default_rx_desc));
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(3)), 1);
		RLE0787_W16(gmac, RxRingSize2+(ADDR_OFFSET*(3)), 1);
	}
	if(RLE0787_R16(gmac, RxRingSize2+(ADDR_OFFSET*(4))) == 0)
	{
		// queue 6
		RLE0787_W32(gmac, RxFDP2+(ADDR_OFFSET*(4)), CPHYSADDR(cp->default_rx_desc));
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(4)), 1);
		RLE0787_W16(gmac, RxRingSize2+(ADDR_OFFSET*(4)), 1);
	}
#endif

	re8686_set_flow_control(gmac, 0, re8670_rx_flow_control_status[gmac][0]);
	re8686_set_flow_control(gmac, 1, re8670_rx_flow_control_status[gmac][1]);
	re8686_set_flow_control(gmac, 2, re8670_rx_flow_control_status[gmac][2]);
	re8686_set_flow_control(gmac, 3, re8670_rx_flow_control_status[gmac][3]);
	re8686_set_flow_control(gmac, 4, re8670_rx_flow_control_status[gmac][4]);
	re8686_set_flow_control(gmac, 5, re8670_rx_flow_control_status[gmac][5]);

}

static void re8670_init_hw (struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	struct net_device *dev = cp->dev;
	u8 status;
	// Kao
	u32 *hwaddr, cputag_info;
	//unsigned int value;

#ifndef CONFIG_DUALBAND_CONCURRENT
	// dual band do not reset hw ip
	re8670_reset_hw(cp);
#endif
	RLE0787_W8(gmac, CMD, RxChkSum|RxJumboSupport);	 /* checksum */		//shlee 8672
//#ifdef CONFIG_ETHWAN
//	vlan_detag(ON);//krammer: default de-tag from skb into desc
//#endif
	vlan_detag(gmac, OFF);
	RLE0787_W16(gmac, ISR, 0xffff);/*clear all interrupt*/
	RLE0787_W32(gmac, ISR1, 0xffffffff);/*clear all interrupt*/
#ifdef KERNEL_SOCKET
	RLE0787_W16(gmac, IMR,RX_ALL(gmac) | TOK); 
#else
	RLE0787_W16(gmac, IMR,RX_ALL(gmac)); 
#endif
	UNMASK_IMR0_RXALL(gmac);
        
	// Kao
	//20170502: disable gmac padding by default
	RLE0787_W32(gmac, TCR,(u32)(0x0C01));	
	RLE0787_W32(gmac, CPUtagCR,(u32)(0x0000));  /* Turn off CPU tag function */
	//cpu tag function
	cputag_info = (CTEN_RX | 2<<CT_RSIZE_L | 2<<CT_TSIZE | CT_APPLO_PRO | CTPM_8370 | CTPV_8370);
	RLE0787_W32(gmac, CPUtagCR,cputag_info); /* Turn on the cpu tag adding function */  //czyao 8672c
	RLE0787_W32(gmac, CPUtag1CR, (CT1_SID));	

	multi_rtx_ring_init(cp);

	status = RLE0787_R8(gmac, MSR);
	status = status | (TXFCE|FORCE_TX);	// enable tx flowctrl
	status = status | RXFCE;
	RLE0787_W8(gmac, MSR, status);	
	// Kao, set hw ID for physical match
	hwaddr = (u32 *)cp->dev->dev_addr;
	RLE0787_W32(gmac, IDR0, *hwaddr);	
	hwaddr = (u32 *)(cp->dev->dev_addr+4);
	RLE0787_W32(gmac, IDR4, *hwaddr);	
	
	RLE0787_W32(gmac, CONFIG_REG, Rff_size_sel_2k);	
	en_rx_mring_int_split(gmac);
	change_rx_sideband_setup(gmac, ON);
	set_rring_route(gmac);
	#ifdef CONFIG_DUALBAND_CONCURRENT
	RLE0787_W32(gmac, INTR_REG, Int_route_r4r5r6t2t4);
	RLE0787_R32(gmac, CONFIG_REG) |= Int_route_enable;
	#endif

	re8670_start_hw(cp);
	__re8670_set_rx_mode(dev);
}

static int re8670_refill_rx (struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	unsigned int i, j;	
#ifdef CONFIG_DUALBAND_CONCURRENT
	DMA_RX_DESC *tmp;
#endif

	for(j=0;j<MAX_RXRING_NUM;j++)
	{
		for (i = 0; i < re8670_rx_ring_size[gmac][j]; i++) {
			struct sk_buff *skb;
			skb = re8670_getAlloc(SKB_BUF_SIZE);
			if (!skb)
				goto err_out;
			// Kaohj --- invalidate DCache for uncachable usage
			//ql_xu
			dma_cache_wback_inv((unsigned long)skb->data, SKB_BUF_SIZE);
			skb->dev = cp->dev;
			cp->rx_skb[j][i].skb = skb;
			cp->rx_skb[j][i].frag = 0;
			if ((u32)skb->data &0x3)
				printk(KERN_DEBUG "skb->data unaligment %8x\n",(u32)skb->data);

			cp->rx_Mring[j][i].addr = (u32)skb->data|UNCACHE_MASK;      

			if (i == (re8670_rx_ring_size[gmac][j] - 1))          
				cp->rx_Mring[j][i].opts1 = (DescOwn | RingEnd | SKB_BUF_SIZE);
			else
				cp->rx_Mring[j][i].opts1 =(DescOwn | SKB_BUF_SIZE);
			cp->rx_Mring[j][i].opts2 = 0;

		}    
	}
#ifdef CONFIG_DUALBAND_CONCURRENT
	tmp = cp->default_rx_desc;
	tmp->opts1 = (0 << 31) | RingEnd| cp->rx_buf_sz; // set own to cpu , can't rx
#endif
	return 0;
err_out:
	re8670_clean_rings(cp);
	return -ENOMEM;
}

static void re8670_tx_timeout (struct net_device *dev)
{
	//unsigned long flags;
	struct re_private *cp = DEV2CP(dev);
#if 0//krammer test
	unsigned tx_head;
	unsigned tx_tail;
	
	cp->cp_stats.tx_timeouts++;

	spin_lock_irqsave(&cp->lock, flags);
	tx_head = cp->tx_hqhead;
	tx_tail = cp->tx_hqtail;
	while (tx_tail != tx_head) {
		struct sk_buff *skb;
		u32 status;
		rmb();
		status = (cp->tx_hqring[tx_tail].opts1);
		if (status & DescOwn)
			break;
		skb = cp->tx_skb[tx_tail].skb;
		if (!skb)
			BUG();
		DEVPRIV(dev)->net_stats.tx_packets++;	

		dev_kfree_skb(skb);
		cp->tx_skb[tx_tail].skb = NULL;
		tx_tail = NEXT_TX(tx_tail);
	}

	cp->tx_hqtail = tx_tail;

	spin_unlock_irqrestore(&cp->lock, flags);
#endif
	if (netif_queue_stopped(cp->dev))
		netif_wake_queue(cp->dev);

}

static int re8670_init_rings (struct re_private *cp)
{
	int j;
	for(j=0;j<MAX_TXRING_NUM;j++){
		cp->tx_Mhqhead[j] = cp->tx_Mhqtail[j] = 0;
	}	

	for(j=0;j<MAX_RXRING_NUM;j++){
		cp->rx_Mtail[j] = 0;		
	}	
	return re8670_refill_rx (cp);
}

static int re8670_alloc_rings (struct re_private *cp)
{
	unsigned int gmac;
	void*	pBuf;
	int j;
	
	gmac = cp->gmac;
	for(j=0;j<MAX_RXRING_NUM;j++)
	{    
		#ifdef CONFIG_DUALBAND_CONCURRENT
		if(!re8670_rx_ring_size[gmac][j]) {
			continue;
		}
		#endif		
		pBuf = kzalloc(RE8670_RXRING_BYTES(re8670_rx_ring_size[gmac][j]), GFP_KERNEL);		
		if (!pBuf)
			goto ErrMem;
		dma_cache_wback_inv((unsigned long)pBuf, RE8670_RXRING_BYTES(re8670_rx_ring_size[gmac][j]));
		cp->rxdesc_Mbuf[j] = pBuf;
		pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ;
		cp->rx_Mring[j] = (DMA_RX_DESC*)((u32)(pBuf) | UNCACHE_MASK);
		if(re8670_rx_ring_size[gmac][j]) {
			pBuf=(struct ring_info*)kzalloc(sizeof(struct ring_info)*re8670_rx_ring_size[gmac][j],GFP_KERNEL);			
			if (!pBuf)
				goto ErrMem;
			cp->rx_skb[j]= pBuf;
		}
	}

#ifdef CONFIG_DUALBAND_CONCURRENT
	// setting default rx desc
	// alloc 1 desc only
	pBuf = kzalloc(RE8670_RXRING_BYTES(1), GFP_KERNEL);
	if(!pBuf)
			goto ErrMem;
	dma_cache_wback_inv((unsigned long)pBuf, RE8670_RXRING_BYTES(1));
	cp->default_rxdesc_Mbuf = pBuf;
	pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ;
	cp->default_rx_desc = (DMA_RX_DESC*)((u32)(pBuf) | UNCACHE_MASK);
	pBuf=(struct ring_info*)kzalloc(sizeof(struct ring_info),GFP_KERNEL);
	if (!pBuf)
		goto ErrMem;
	cp->default_rx_skb = pBuf;
#endif

	for(j=0;j<MAX_TXRING_NUM;j++)
	{
		if(!re8670_tx_ring_size[gmac][j])
			continue;
		pBuf = kzalloc(RE8670_TXRING_BYTES(re8670_tx_ring_size[gmac][j]), GFP_KERNEL);		
		if (!pBuf)
			goto ErrMem;
		dma_cache_wback_inv((unsigned long)pBuf, RE8670_TXRING_BYTES(re8670_tx_ring_size[gmac][j]));
		cp->txdesc_Mbuf[j] = pBuf;
		pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ;
		cp->tx_Mhqring[j] = (DMA_TX_DESC*)((u32)(pBuf) | UNCACHE_MASK);

		pBuf =(struct ring_info*)kzalloc(sizeof(struct ring_info)*re8670_tx_ring_size[gmac][j],GFP_KERNEL);		
		if (!pBuf)
			goto ErrMem;
		cp->tx_skb[j] = pBuf;
	}

	return re8670_init_rings(cp);

ErrMem:

	for(j=0;j<MAX_RXRING_NUM;j++)
	{
		if (cp->rx_skb[j])    
			kfree(cp->rx_skb[j]);
	}        

	for(j=0;j<MAX_TXRING_NUM;j++)
	{        
		if (cp->tx_skb[j])    
			kfree(cp->tx_skb[j]);	
	}
	
#ifdef CONFIG_DUALBAND_CONCURRENT
	if(cp->default_rx_skb)
		kfree(cp->default_rx_skb);
	if(cp->default_rxdesc_Mbuf)
		kfree(cp->default_rxdesc_Mbuf);
#endif

	return -ENOMEM;

}

static void re8670_clean_rings (struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	unsigned i,j;
	
	for (j = 0; j < MAX_RXRING_NUM; j++) {
		if(cp->rx_skb[j]){
			for (i = 0; i < re8670_rx_ring_size[gmac][j]; i++) {
				if (cp->rx_skb[j][i].skb) {
					dev_kfree_skb(cp->rx_skb[j][i].skb);
				}
			}
			memset(cp->rx_skb[j], 0, sizeof(struct ring_info) * re8670_rx_ring_size[gmac][j]);		
		}
	}
	for (j = 0; j < MAX_TXRING_NUM; j++) {
		if(cp->tx_skb[j]){
			for (i = 0; i < re8670_tx_ring_size[gmac][j]; i++) {
				if (cp->tx_skb[j][i].skb) {
					struct sk_buff *skb = cp->tx_skb[j][i].skb;
					DEVPRIV(skb->dev)->net_stats.tx_dropped++;
					dev_kfree_skb(skb);
				}
			}
			memset(cp->tx_skb[j], 0, sizeof(struct ring_info) * re8670_tx_ring_size[gmac][j]);	
		}
	}
}

static void re8670_free_rings (struct re_private *cp)
{
	int j;
	re8670_clean_rings(cp);
	/*pci_free_consistent(cp->pdev, CP_RING_BYTES, cp->rx_ring, cp->ring_dma);*/

	for(j=0;j<MAX_RXRING_NUM;j++)
	{
		if (cp->rxdesc_Mbuf[j]) {
			kfree(cp->rxdesc_Mbuf[j]);
			cp->rxdesc_Mbuf[j] = NULL;
		}
		
		cp->rx_Mring[j] = NULL;   

		if (cp->rx_skb[j]) {   			
			kfree(cp->rx_skb[j]);
			cp->rx_skb[j] = NULL;
		}
		cp->rx_skb[j]=NULL;
	}     

	for(j=0;j<MAX_TXRING_NUM;j++)        
	{
		if (cp->txdesc_Mbuf[j]) {
			kfree(cp->txdesc_Mbuf[j]);
			cp->txdesc_Mbuf[j] = NULL;
		}
		
		cp->tx_Mhqring[j] = NULL;

		if (cp->tx_skb[j])   {			
			kfree(cp->tx_skb[j]);
			cp->tx_skb[j] = NULL;
		}
		cp->tx_skb[j]=NULL;	
	}    

#ifdef CONFIG_DUALBAND_CONCURRENT
	if(cp->default_rx_skb)
		kfree(cp->default_rx_skb);
	cp->default_rx_skb = NULL;
	if(cp->default_rxdesc_Mbuf)
		kfree(cp->default_rxdesc_Mbuf);
#endif
}

static int re8670_open (struct net_device *dev)
{
	struct re_private *cp = DEV2CP(dev);
	unsigned long rxflags; 
	unsigned long txflags; 
	unsigned int gmac;
	int rc=0;

#if defined(CONFIG_DUALBAND_CONCURRENT) && defined(CONFIG_VIRTUAL_WLAN_DRIVER)	
	if(!memcmp(dev->name, "wlan1", 5)){
		vwlan_open(dev);
		//printk("%s %d\n",__FUNCTION__, __LINE__);
	}
#endif

	if (netif_msg_ifup(cp))
		printk(KERN_DEBUG "%s: enabling interface\n", dev->name);

	if(dev_num == 0) {
		printk("%s %d\n", __func__, __LINE__);
		for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
		{
			if(gmac_enabled[gmac]==OFF)
				continue;

			re8670_set_rxbufsize(&re_private_data[gmac]);	/* set new rx buf size */
			re8670_init_hw(&re_private_data[gmac]);
			re8670_init_trx_cdo(&re_private_data[gmac]);
			//rc = request_irq(dev->irq, (irq_handler_t)re8670_interrupt, IRQF_DISABLED, dev->name, dev);
			rc = request_irq(eth_net_dev[gmac]->irq, (irq_handler_t)re8670_interrupt, IRQF_DISABLED, eth_net_dev[gmac]->name, eth_net_dev[gmac]);
			if (rc)
				goto err_out_hw;
			if(re_private_data[gmac].stag_pid)
				RLE0787_R32(gmac, VLAN_REG) = re_private_data[gmac].stag_pid;
			if(re_private_data[gmac].stag_pid1)
				RLE0787_R32(gmac, VLAN1_REG) = re_private_data[gmac].stag_pid1;	
		}
	}

	dev_num++;
	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		if(gmac_enabled[gmac]==OFF)
			continue;
		eth_close[gmac]=0;
	}
	netif_start_queue(dev);
	return 0;

err_out_hw:
	#ifndef CONFIG_DUALBAND_CONCURRENT
	re8670_stop_hw(cp);
	#endif
	return rc;
}

static int re8670_close (struct net_device *dev)
{
	struct re_private *cp = DEV2CP(dev);
	unsigned long rxflags; 
	unsigned long txflags;
	unsigned int gmac;

	dev_num--;	
#if defined(CONFIG_DUALBAND_CONCURRENT) && defined(CONFIG_VIRTUAL_WLAN_DRIVER)	
	if(!memcmp(dev->name, "wlan1", 5)){
		vwlan_close(dev);
		//printk("%s %d\n",__FUNCTION__, __LINE__);
	}
#endif
	if(dev_num == 0){
		netif_stop_queue(dev);
		for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
		{
			if(gmac_enabled[gmac]==OFF)
				continue;

			eth_close[gmac]=1;
			#ifndef CONFIG_DUALBAND_CONCURRENT
			re8670_stop_hw(&re_private_data[gmac]);
			#endif
			
			free_irq(eth_net_dev[gmac]->irq, eth_net_dev[gmac]);
			printk("%s %d\n", __func__, __LINE__);
			re_private_data[gmac].stag_pid = RLE0787_R32(gmac, VLAN_REG);
			re_private_data[gmac].stag_pid1 = RLE0787_R32(gmac, VLAN1_REG);
		}
	}

	if (netif_msg_ifdown(cp))
		printk(KERN_DEBUG "%s: disabling interface\n", dev->name);

	return 0;
}

int re8670_ioctl (struct net_device *dev, struct ifreq *rq, int cmd)
{
	int rc = 0;

	if (!netif_running(dev) && cmd!=SIOCETHTEST)
		return -EINVAL;

	switch (cmd) {
		case SIOCETHTEST:
			eth_ctl((struct eth_arg *)rq->ifr_data);
			break;
		default:
			rc = -EOPNOTSUPP;
			break;
	}

	return rc;
}

static int dbg_level_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;	
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	if (buf && !copy_from_user(tmpBuf, buf, len))
	{
		debug_enable[gmac]=simple_strtoul(tmpBuf, NULL, 16);
		printk("write debug_enable to 0x%08x\n", debug_enable[gmac]);
		return count;
	}
	return -EFAULT;
}

static int memrw_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	return 0;
}

static int memrw_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 		tmpbuf[64];
	unsigned int	*mem_addr, mem_data, mem_len;
	char		*strptr, *cmd_addr;
	char		*tokptr;

	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
		{
			goto errout;
		}
		printk("cmd %s\n", cmd_addr);
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}

		if (!memcmp(cmd_addr, "r", 1))
		{
			mem_addr=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_len=simple_strtol(tokptr, NULL, 0);
			memDump(mem_addr, mem_len, "");
		}
		else if (!memcmp(cmd_addr, "w", 1))
		{
			mem_addr=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_data=simple_strtol(tokptr, NULL, 0);
			WRITE_MEM32(mem_addr, mem_data);
			printk("Write memory 0x%p dat 0x%x: 0x%x\n", mem_addr, mem_data, READ_MEM32(mem_addr));
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("Memory operation only support \"r\" and \"w\" as the first parameter\n");
		printk("Read format:	\"r mem_addr length\"\n");
		printk("Write format:	\"w mem_addr mem_data\"\n");
	}
	return count;
}
#ifdef CONFIG_RTL8686_SWITCH
#ifndef CONFIG_RTK_L34_ENABLE
static int switch_mode_read(struct seq_file *seq, void *v)
{	  
	  switch(SWITCH_MODE)
	  {
		case RTL8686_Switch_Mode_Trap2Cpu:
			printk("Asic switch mode : trap2cpu\n");
			break;
		case RTL8686_Switch_Mode_Normal:
			printk("Asic switch mode : normal\n");
			break;
		default:
			printk("Asic switch mode : Unknown\n");
	  }      
      return 0;
}
static int switch_control_set_mode(int mode)
{
	if( mode!=RTL8686_Switch_Mode_Trap2Cpu
		&& mode!=RTL8686_Switch_Mode_Normal)
		return -1;	
	SWITCH_MODE = mode;
	return 0;
}
#define SWITCH_PORT_NUM 5
static int switch_normal(void)
{
	int ret = RT_ERR_FAILED;
	rtk_acl_ingress_entry_t aclRule;
	memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t)); 
    aclRule.index = 0;	
	if((ret = rtk_acl_igrRuleEntry_del(aclRule.index))!= RT_ERR_OK)
	{
		printk("%s-%d error rtk_acl_igrRuleEntry_del index %d\n",__func__,__LINE__,aclRule.index);
		return ret; 	
	}
	return 0;
}
static int switch_trap2cpu(void)
{
	int ret = RT_ERR_FAILED;
	rtk_acl_ingress_entry_t aclRule;	
	unsigned int port;
	memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t)); 
	//default set to acl index 0, if set trap2cpu
    aclRule.index = 0;	
    aclRule.templateIdx = 0;
	//aclRule.activePorts.bits[0] = 1 << 0; /* port 0*/
   	for(port=0;port < SWITCH_PORT_NUM ; port ++)
		aclRule.activePorts.bits[0] |= (1 << port);
	aclRule.valid = ENABLED;
	aclRule.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
	aclRule.act.forwardAct.act = ACL_IGR_FORWARD_TRAP_ACT;
	if((ret = rtk_acl_igrRuleEntry_add(&aclRule))!= RT_ERR_OK) 
	{
		printk("%s-%d error rtk_acl_igrRuleEntry_add index %d\n",__func__,__LINE__,aclRule.index);
		return ret; 
	}
   	for(port=0;port < SWITCH_PORT_NUM ; port ++)
   	{ 
		rtk_acl_igrState_set(port,ENABLED);
	}
	return 0;
}
static int switch_mode_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 	tmpbuf[512];
	char		*strptr;	
	int retval = -1;
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;	
	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		if(strlen(strptr)==0)
		{
			goto errout;
		}
		if(strncmp(strptr, "trap2cpu",8) == 0)
		{
			retval = switch_control_set_mode(RTL8686_Switch_Mode_Trap2Cpu);
			vlan_detag(gmac, ON);
			retval = switch_trap2cpu();
#ifdef CONFIG_RTL_MULTI_LAN_DEV			
			change_dev_port_mapping(LAN_PORT1, "eth0.2");
			change_dev_port_mapping(LAN_PORT2, "eth0.3");
			change_dev_port_mapping(LAN_PORT3, "eth0.4");
			change_dev_port_mapping(LAN_PORT4, "eth0.5");
			change_dev_port_mapping(LAN_PORT5, "eth0.6");
			change_dev_port_mapping(LAN_PORT6, "eth0.7");
			change_dev_port_mapping(WAN_PORT,"nas0");						
#endif						
		}
		else if(strncmp(strptr, "normal",6) == 0)
		{
			retval = switch_control_set_mode(RTL8686_Switch_Mode_Normal);
			vlan_detag(gmac, OFF);
			retval = switch_normal();
#ifdef CONFIG_RTL_MULTI_LAN_DEV			
			change_dev_port_mapping(LAN_PORT1, "eth0.2");
			change_dev_port_mapping(LAN_PORT2, "eth0.3");
			change_dev_port_mapping(LAN_PORT3, "eth0.4");
			change_dev_port_mapping(LAN_PORT4, "eth0.5");
			change_dev_port_mapping(LAN_PORT5, "eth0.6");
			change_dev_port_mapping(LAN_PORT6, "eth0.7");
			change_dev_port_mapping(WAN_PORT,"nas0");						
#endif	
		}
		else
		{
			goto errout;
		}
		if(retval==0)
			printk("write success ! \n");
		else
			printk("error : change mode fail ! \n");
	}
	else
	{
errout:
		printk("error input  (trap/normal)\n");
	}
	return count;
}
#endif //CONFIG_RTK_L34_ENABLE

static u32 rtl_ethtool_get_port_link(struct net_device *dev)
{
	//unsigned int i;
	u32 txportmask = DEVPRIV(dev)->txPortMask;
	rtk_port_linkStatus_t LinkStatus;
	int portnum;

	if(txportmask == 0){//eth0, always link, have problem?
		return 1;
	}
#if 0
	for(i=0;i<SW_PORT_NUM;i++){
		if(txportmask & (1<<i)){
			//return get_port_link(i);//first match bit
			return 1;
		}
	}
#endif
	//krammer change to default link up
	//return 0;//default link down
	for(portnum = 0; portnum < SW_PORT_NUM; ++portnum)
	{
		if(txportmask & (1 << portnum))
			break;
	}
		
	rtk_port_link_get(portnum, &LinkStatus);
#if 0
	if (0 == LinkStatus)
		printk("port %d is down.\n", portnum);
	else if (1 == LinkStatus)
		printk("port %d is up.\n", portnum);
	else
		printk("port %d is chaos\n", portnum);
#endif
	return LinkStatus;//default link down
}



static int rtl_ethtool_get_settings(struct net_device *dev, struct ethtool_cmd *ecmd)
{
#if 0
	unsigned int  portnum;
	unsigned int  tmp_spd;

    apollo_raw_port_ability_t Ability;

	u32 txportmask = DEVPRIV(dev)->txPortMask;

	if(0 == txportmask)
		return 0;

	//we only treat the first match bit;
	for(portnum = 0; portnum < SW_PORT_NUM; ++portnum)
	{
		if(txportmask & (1 << portnum))
			break;
	}

	if(portnum > SW_PORT_NUM)
		return 0;

	apollo_raw_port_ability_get(portnum, &Ability);

	ecmd->supported = SUPPORTED_10baseT_Half 		
							|	SUPPORTED_10baseT_Full 	
							|	SUPPORTED_100baseT_Half 	
							|	SUPPORTED_100baseT_Full
							|	SUPPORTED_1000baseT_Full
							|	SUPPORTED_Autoneg 	
							|	SUPPORTED_Pause
							|	((PON_PORT == portnum) ? SUPPORTED_FIBRE : SUPPORTED_TP );
		                   
	ecmd->advertising = ADVERTISED_10baseT_Half 
							|	ADVERTISED_10baseT_Full
							|	ADVERTISED_100baseT_Full
							|	ADVERTISED_100baseT_Half
							|	ADVERTISED_Autoneg
							|	ADVERTISED_Pause
							|	((PON_PORT == portnum) ? SUPPORTED_FIBRE : SUPPORTED_TP );

	if(PON_PORT == portnum)
		ecmd->port = PORT_FIBRE;
	else
		ecmd->port = PORT_TP;

	ecmd->autoneg = (Ability.nwayAbility) ? AUTONEG_ENABLE : AUTONEG_DISABLE;

	ecmd->duplex = Ability.duplex ? DUPLEX_FULL : DUPLEX_HALF;

	ecmd->transceiver = XCVR_INTERNAL;

	ecmd->phy_address = portnum;

	if(PON_PORT == portnum)
		tmp_spd = Ability.linkFib1g ? SPEED_1000 : SPEED_100;
	else
		tmp_spd = (PORT_SPEED_100M == Ability.speed) ? SPEED_100 : 
						((PORT_SPEED_10M == Ability.speed) ? SPEED_10 : SPEED_1000);

	//if the port is not linked, we set the speed as "UNKOWN!".
	if(rtl_ethtool_get_port_link(dev))
		ecmd->speed = tmp_spd;
	else
		ecmd->speed = 0;
#endif
	return 0;	
}


#else
static u32 rtl_ethtool_get_port_link(struct net_device *dev)
{
	return 1;//always link, have problem?
}
#endif

static const struct ethtool_ops rtl_ethtool_ops = {
	.get_link 			= rtl_ethtool_get_port_link,
#ifdef CONFIG_RTL8686_SWITCH
	.get_settings 		= rtl_ethtool_get_settings,
#endif
};

void rtl_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &rtl_ethtool_ops;
}

static int misc_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	printk("iocmd_reg 0x%x\n", iocmd_reg[gmac]);
	printk("iocmd1_reg 0x%x\n", iocmd1_reg[gmac]);
	printk("TX jumbo frame: %s\n", txJumboFrameEnabled[gmac]?"Enabled":"Disabled");
	return 0;
}
static int misc_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	char	tmpbuf[64];
	char	*strptr, *var;
	char	*tokptr;
	int i;
	unsigned int value;
	#define VAR_NUM 8
	char var_name[VAR_NUM][64]={"iocmd_reg", "iocmd1_reg", "txJumbo", ""};
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	unsigned int* pVar[VAR_NUM] = {&iocmd_reg[gmac], &iocmd1_reg[gmac], &txJumboFrameEnabled[gmac], NULL};
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		var = strsep(&strptr," ");
		if (var==NULL)
		{
			goto errout;
		}
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		
		for(i=0;strlen(var_name[i])&&pVar[i];i++){
			if(!memcmp(var, var_name[i], strlen(var_name[i]))){
				if(!strcmp(var, "txJumbo")){
					value = simple_strtol(tokptr, NULL, 0);
					*pVar[i] = value;
					change_tx_jumbo_setup(gmac, value);
				}else{
					value = simple_strtol(tokptr, NULL, 0);
					*pVar[i] = value;
				}
				break;
			}
		}

		if(!pVar[i]){
			goto errout;
		}

	}
	else
	{
errout:
		printk("iocmd_reg 0x%x\n", iocmd_reg[gmac]);
		printk("iocmd1_reg 0x%x\n", iocmd1_reg[gmac]);
		printk("TX jumbo frame: %s\n", txJumboFrameEnabled[gmac]?"Enabled":"Disabled");
	}
	return count;
}

static int dev_port_mapping_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	unsigned int i;
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	printk("WAN PORT %d, CPU PORT %d\n", WAN_PORT, gmac_cpu_port[gmac]);
	printk("DEV ability: ");
	for(i=0;i<totalDev;i++){
		if(rtl8686_dev_table[i].dev_instant)
			printk("%s ", rtl8686_dev_table[i].dev_instant->name);
	}
	printk("\n");

	printk("rx: phyPort -> dev[the packet rx from phyPort will send to kernel using dev]\n");
	for(i=0;i<SW_PORT_NUM;i++){
		if(re_private_data[gmac].port2dev[i])
			printk("port%d -> %s\n", i, re_private_data[gmac].port2dev[i]->name);
	}

	printk("tx:dev -> txPortMask[when tx from dev, we will use this txPortMask]\n");
	for(i=0;i<totalDev;i++){
		if(rtl8686_dev_table[i].dev_instant)
			printk("%s -> 0x%x\n",
				rtl8686_dev_table[i].dev_instant->name, DEVPRIV(rtl8686_dev_table[i].dev_instant)->txPortMask);
	}

	printk("TX always using: %s\n", (re_private_data[txAlwaysGmac].dev)->name);
#ifdef CONFIG_RTL8686_SWITCH
	printk("netdev carrier mapping\n");
	for(i=0;i<SW_PORT_NUM;i++)
	{
		printk("Port%d => ifname:%s , dev:%s(0x%p) , status:%d \n",
			i , LCDev_mapping[i].ifname , LCDev_mapping[i].phy_dev->name, LCDev_mapping[i].phy_dev, LCDev_mapping[i].status);
	}
#endif
	return 0;
}

static int dev_port_mapping_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 		tmpbuf[64];
	char		*strptr;
	unsigned int port_num;//, i;
	char		*tokptr;
	unsigned long buf_size;
	
	buf_size = min(count, (sizeof(tmpbuf)-1));
	if (buf && !copy_from_user(tmpbuf, buf, buf_size)) {
	
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		port_num = simple_strtol(tokptr, NULL, 0);
		if(strcmp(tokptr, "txAlways")) {
			printk("port %d ", port_num);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			if(tokptr[strlen(tokptr) - 1] == 0x0a)
				tokptr[strlen(tokptr) - 1] = 0x00;
			printk("assign to %s\n", tokptr);
			change_dev_port_mapping(port_num, tokptr);	
		} else {
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			if(tokptr[strlen(tokptr) - 1] == 0x0a)
				tokptr[strlen(tokptr) - 1] = 0x00;			
			if(!change_tx_always_device(tokptr))
				printk("TX always using %s \n", tokptr);
			else
				goto errout;
		}
	}
	else
	{
errout:
		printk("num_of_port dev_name\n");
		printk("txAlways eth0/eth1/../ethN \n");
		return -EFAULT;
	}
		
	return count;
}

static int change_dev_tx_port_mask(int port_num, char* name, int index)
{
	if(strcmp(name, "eth0")){
		DEVPRIV(rtl8686_dev_table[index].dev_instant)->txPortMask = 1 << port_num;
		printk("%s -> 0x%x\n", 
			rtl8686_dev_table[index].dev_instant->name, DEVPRIV(rtl8686_dev_table[index].dev_instant)->txPortMask);
	}
	return 0;
}

/*ccwei: add dev port mapping api*/
static int change_dev_port_mapping(int port_num, char* name)
{
	unsigned int gmac;
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	char* dev_name;
	struct net_device *dev = NULL;
	int i;
	
	for(i=0;i<totalDev;i++){

		if(i <= MAX_GMAC_NUM && gmac_enabled[i] == OFF)
		{
			continue;
		}
		
		if(rtl8686_dev_table[i].dev_instant)
		{
			dev_name = rtl8686_dev_table[i].dev_instant->name;
			dev = rtl8686_dev_table[i].dev_instant;
		}
		else
		{
			printk("no dev_instant, strange.......\n");
			dev_name = rtl8686_dev_table[i].ifname;
		}		
		if(!strcmp(dev_name, name)){
			for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {	
				
				if(gmac_enabled[gmac]==OFF)
					continue;	
				
				re_private_data[gmac].port2dev[port_num] = rtl8686_dev_table[i].dev_instant;
			}
			change_dev_tx_port_mask(port_num, name, i);
			break;
		}
	}
	
	if(i == totalDev){
		printk("can't find dev %s\n", name);
		return -1;
	}
	
#ifdef CONFIG_RTL8686_SWITCH 
	if(port_num < SW_PORT_NUM && port_num >= 0 && dev )
	{
		LCDev_mapping[port_num].phy_dev = dev;
		strcpy(LCDev_mapping[port_num].ifname,dev->name);
	}
#endif
	
	return 0;
}

static int change_tx_always_device(char* name)
{
	int ret = -1;
	int i;

	for(i=0 ; i<MAX_GMAC_NUM ; i++)
	{
		if(!strcmp((re_private_data[i].dev)->name, name)) {
			txAlwaysGmac = i;
			ret = 0;
			break;
		}
	}
	
	return ret;
}

#if defined(CONFIG_APOLLO_ROMEDRIVER)
extern int fwdEngine_rx_skb(struct re_private *cp, struct sk_buff *skb,struct rx_info *pRxInfo);
#endif
static int port_to_rxfunc_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	unsigned int totalPortTable = SW_PORT_NUM;
	unsigned int i;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	//default set to eth0
	for(i=0;i<totalPortTable;i++){
		printk("port%d -> 0x%p\n", i, re_private_data[gmac].port2rxfunc[i]);
	}
	printk("================[port2rxfunc func pointer table]================\n");
#if defined(CONFIG_APOLLO_ROMEDRIVER)
	printk("fwdEngine_rx_skb: 0x%p\n", fwdEngine_rx_skb);
#endif
	printk("re8670_rx_skb: 0x%p\n", re8670_rx_skb);
	printk("================================================================\n");
	return 0;
}

static int port_to_rxfunc_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 	tmpbuf[512];
	char		*strptr;	
	int retval = -1, i;
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
	return 0;
}

	gmac = data->gmac;	
	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		if( strlen(strptr)==0 )
		{
			goto errout;
		}
#if defined(CONFIG_APOLLO_ROMEDRIVER)
		if(strncmp(strptr, "force2rg", 8) == 0)
		{
			re8686_register_rxfunc_all_port(&fwdEngine_rx_skb);
			printk("force NIC Rx hook to RG only!\n");
		}
		else 
#endif
		if(strncmp(strptr, "force2nf", 8) == 0)
		{
			re8686_register_rxfunc_all_port(&re8670_rx_skb);
			printk("force NIC Rx hook to PS only!\n");
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
#if defined(CONFIG_APOLLO_ROMEDRIVER)
		printk("error input (force2rg/force2nf)\n");
#else
		printk("error input (force2nf)\n");
#endif
	}
	return count;
}

int dbg_level_read(struct file *filp, char *buf, size_t count, loff_t *offp ) 
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	printk("[debug_enable = 0x%08x]\n", debug_enable[gmac]);
	printk("RTL8686_PRINT_NOTHING\t0x%08x\n", RTL8686_PRINT_NOTHING);
	printk("RTL8686_SKB_RX\t\t0x%08x\n", RTL8686_SKB_RX);
	printk("RTL8686_SKB_TX\t\t0x%08x\n", RTL8686_SKB_TX);
	printk("RTL8686_RXINFO\t\t0x%08x\n", RTL8686_RXINFO);
	printk("RTL8686_TXINFO\t\t0x%08x\n", RTL8686_TXINFO);
	printk("RTL8686_RX_TRACE\t\t0x%08x\n", RTL8686_RX_TRACE);
	printk("RTL8686_TX_TRACE\t\t0x%08x\n", RTL8686_TX_TRACE);
	printk("RTL8686_RX_WARN\t\t0x%08x\n", RTL8686_RX_WARN);
	printk("RTL8686_TX_WARN\t\t0x%08x\n", RTL8686_TX_WARN);
	
	return 0;
}

static int hw_reg_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	int len = 0, i;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	printk("ETHBASE		=0x%08x\n", gmac_eth_base[gmac]);
	printk("IDR		=%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\n", 
		RLE0787_R8(gmac, IDR0), RLE0787_R8(gmac, IDR1), RLE0787_R8(gmac, IDR2), RLE0787_R8(gmac, IDR3), RLE0787_R8(gmac, IDR4), RLE0787_R8(gmac, IDR5));
	printk("MAR		=%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\n", 
		RLE0787_R8(gmac, MAR0), RLE0787_R8(gmac, MAR1), RLE0787_R8(gmac, MAR2), RLE0787_R8(gmac, MAR3), 
		RLE0787_R8(gmac, MAR4), RLE0787_R8(gmac, MAR5), RLE0787_R8(gmac, MAR6), RLE0787_R8(gmac, MAR7));
	printk("TXOKCNT		=0x%04x		RXOKCNT		=0x%04x\n", RLE0787_R16(gmac, TXOKCNT), RLE0787_R16(gmac, RXOKCNT));
	printk("TXERR		=0x%04x		RXERRR		=0x%04x\n", RLE0787_R16(gmac, TXERR), RLE0787_R16(gmac, RXERRR));
	printk("MISSPKT		=0x%04x		FAE		=0x%04x\n", RLE0787_R16(gmac, MISSPKT), RLE0787_R16(gmac, FAE));
	printk("TX1COL		=0x%04x		TXMCOL		=0x%04x\n", RLE0787_R16(gmac, TX1COL), RLE0787_R16(gmac, TXMCOL));
	printk("RXOKPHY		=0x%04x		RXOKBRD		=0x%04x\n", RLE0787_R16(gmac, RXOKPHY), RLE0787_R16(gmac, RXOKBRD));
	printk("RXOKMUL		=0x%04x		TXABT		=0x%04x\n", RLE0787_R16(gmac, RXOKMUL), RLE0787_R16(gmac, TXABT));
	printk("TXUNDRN		=0x%04x		RDUMISSPKT	=0x%04x\n", RLE0787_R16(gmac, TXUNDRN), RLE0787_R16(gmac, RDUMISSPKT));
	printk("TRSR		=0x%08x\n", RLE0787_R32(gmac, TRSR));
	printk("CMD		=0x%02x		IMR		=0x%04x\n", RLE0787_R8(gmac, CMD), RLE0787_R16(gmac, IMR));
	printk("ISR		=0x%04x		TCR		=0x%08x\n", RLE0787_R16(gmac, ISR), RLE0787_R32(gmac, TCR));
	printk("ISR1		=0x%08x	IMR0		=0x%08x\n", RLE0787_R32(gmac, ISR1), RLE0787_R32(gmac, IMR0));
	printk("RCR		=0x%08x	CPUtagCR	=0x%08x\n", RLE0787_R32(gmac, RCR), RLE0787_R32(gmac, CPUtagCR));
	printk("CONFIG_REG	=0x%08x	CPUtag1CR	=0x%08x\n", RLE0787_R32(gmac, CONFIG_REG), RLE0787_R32(gmac, CPUtag1CR));
	printk("MSR		=0x%08x	VLAN_REG	=0x%08x\n", RLE0787_R32(gmac, MSR), RLE0787_R32(gmac, VLAN_REG));
	printk("VLAN1_REG	=0x%08x		LEDCR		=0x%08x\n", RLE0787_R32(gmac, VLAN1_REG), RLE0787_R32(gmac, LEDCR));

	for(i=0;i<MAX_TXRING_NUM;i++)
	{
		printk("TxFDP%d		=0x%08x	TxCDO%d		=0x%04x\n",i,RLE0787_R32(gmac, TxFDP1+(ADDR_OFFSET*i)),i,RLE0787_R16(gmac, TxCDO1+(ADDR_OFFSET*i)));
	}
	for(i=0;i<MAX_RXRING_NUM;i++){
		if(i==0)
		{
			printk("RxRingSize%d	=0x%04x\n", i, RLE0787_R16(gmac, RxRingSize));
			printk("RxFDP%d		=0x%08x	RxCDO%d		=0x%04x\n",i,RLE0787_R32(gmac, RxFDP), i, RLE0787_R16(gmac, RxCDO));
			printk("EthrntRxCPU_Des_Num	=0x%02x	EthrntRxCPU_Des_Wrap	=0x%02x\n", 
				RLE0787_R8(gmac, EthrntRxCPU_Des_Num), RLE0787_R8(gmac, EthrntRxCPU_Des_Wrap));
			printk("Rx_Pse_Des_Thres	=0x%02x	EthrntRxCPU_Des_Num_h	=0x%02x\n", 
				RLE0787_R8(gmac, Rx_Pse_Des_Thres), RLE0787_R8(gmac, EthrntRxCPU_Des_Num_h));
			printk("Rx_Pse_Des_Thres_h	=0x%02x\n", RLE0787_R8(gmac, Rx_Pse_Des_Thres_h));
		}
		else
		{
			printk("RxRingSize%d	=0x%04x\n", i, RLE0787_R16(gmac, RxRingSize2+(ADDR_OFFSET*(i-1))));
			printk("RxFDP%d		=0x%08x	RxCDO%d		=0x%04x\n",i,RLE0787_R32(gmac, RxFDP2+(ADDR_OFFSET*(i-1))),i,RLE0787_R16(gmac, RxCDO2+(ADDR_OFFSET*(i-1))));
			printk("RxCPU_Des_Num%d	=0x%08x	RxCPU_Des_Thres%d=0x%08x\n", 
				i, RLE0787_R32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(i-1))), 
				i, RLE0787_R32(gmac, EthrntRxCPU_Des_Wrap2+(ADDR_OFFSET*(i-1))));
		}
	}
	printk("RRING_ROUTING1	=0x%08x	RRING_ROUTING2	=0x%08x\n", RLE0787_R32(gmac, RRING_ROUTING1), RLE0787_R32(gmac, RRING_ROUTING2));
	printk("RRING_ROUTING3	=0x%08x	RRING_ROUTING4	=0x%08x\n", RLE0787_R32(gmac, RRING_ROUTING3), RLE0787_R32(gmac, RRING_ROUTING4));
	printk("RRING_ROUTING5	=0x%08x	RRING_ROUTING6	=0x%08x\n", RLE0787_R32(gmac, RRING_ROUTING5), RLE0787_R32(gmac, RRING_ROUTING6));
	printk("RRING_ROUTING7	=0x%08x\n", RLE0787_R32(gmac, RRING_ROUTING7));
	printk("IO_CMD		=0x%08x	IO_CMD1		=0x%08x\n", RLE0787_R32(gmac, IO_CMD), RLE0787_R32(gmac, IO_CMD1));
	printk("DIAG1_REG       =0x%08x\n",RLE0787_R32(gmac, DIAG1_REG));

	return len;
}

static int sw_cnt_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	int len = 0;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	printk("rx_hw_num	   :%14d		rx_sw_num	   :%14d\n", 
		re_private_data[gmac].cp_stats.rx_hw_num, re_private_data[gmac].cp_stats.rx_sw_num);
	printk("rer_runt	   :%14d		rer_ovf 	   :%14d\n", 
		re_private_data[gmac].cp_stats.rer_runt, re_private_data[gmac].cp_stats.rer_ovf);
	printk("rdu 		   :%14d		frag		   :%14d\n", 
		re_private_data[gmac].cp_stats.rdu, re_private_data[gmac].cp_stats.frag);
#ifdef CONFIG_RG_JUMBO_FRAME
	printk("toobig		   :%14d\n", re_private_data[gmac].cp_stats.toobig);
#endif
	printk("crcerr		   :%14d		rcdf		   :%14d\n", 
		re_private_data[gmac].cp_stats.crcerr, re_private_data[gmac].cp_stats.rcdf);
	printk("rx_no_mem	   :%14d		tx_sw_num	   :%14d\n", 
		re_private_data[gmac].cp_stats.rx_no_mem, re_private_data[gmac].cp_stats.tx_sw_num);
	printk("tx_hw_num	   :%14d		tx_no_desc	   :%14d\n", 
		re_private_data[gmac].cp_stats.tx_hw_num, re_private_data[gmac].cp_stats.tx_no_desc);
	printk("rx_cri_num	   :%14d		rx_cri_no_desc :%14d\n", 
		re_private_data[gmac].cp_stats.rx_critical_num, re_private_data[gmac].cp_stats.rx_critical_drop_num);
#if defined( CONFIG_RTL8686NIC) || defined( CONFIG_RTL8686NIC_MODULE)
	printk("rxskb_num	   :%14d\n", re8670_rxskb_num.counter);
#endif

	return len;
}

static int sw_cnt_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 	tmpbuf[512];
	char		*strptr;	
	int retval = -1, i;
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;	
	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		if( strlen(strptr)==0 )
		{
			goto errout;
		}
		if(strncmp(strptr, "all", 8) == 0 || strncmp(strptr, "1", 1) == 0)
		{
			re_private_data[gmac].cp_stats.rx_hw_num=0;
			re_private_data[gmac].cp_stats.rx_sw_num=0;
			re_private_data[gmac].cp_stats.rer_runt=0;
			re_private_data[gmac].cp_stats.rer_ovf=0;
			re_private_data[gmac].cp_stats.rdu=0;
			re_private_data[gmac].cp_stats.frag=0;
			re_private_data[gmac].cp_stats.toobig=0;
			re_private_data[gmac].cp_stats.crcerr=0;
			re_private_data[gmac].cp_stats.rcdf=0;
			re_private_data[gmac].cp_stats.rx_no_mem=0;
			re_private_data[gmac].cp_stats.tx_sw_num=0;
			re_private_data[gmac].cp_stats.tx_hw_num=0;
			re_private_data[gmac].cp_stats.tx_no_desc=0;
			printk("all software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_hw_num", 9) == 0 || strncmp(strptr, "2", 1) == 0)
		{
			re_private_data[gmac].cp_stats.rx_hw_num=0;
			printk("rx_hw_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_sw_num", 9) == 0 || strncmp(strptr, "3", 1) == 0)
		{
			re_private_data[gmac].cp_stats.rx_sw_num=0;
			printk("rx_sw_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "rer_runt", 8) == 0 || strncmp(strptr, "4", 1) == 0)
		{
			re_private_data[gmac].cp_stats.rer_runt=0;
			printk("rer_runt software counter cleared !\n");
		}
		else if(strncmp(strptr, "rer_ovf", 7) == 0 || strncmp(strptr, "5", 1) == 0)
		{
			re_private_data[gmac].cp_stats.rer_ovf=0;
			printk("rer_ovf software counter cleared !\n");
		}
		else if(strncmp(strptr, "rdu", 3) == 0 || strncmp(strptr, "6", 1) == 0)
		{
			re_private_data[gmac].cp_stats.rdu=0;
			printk("rdu software counter cleared !\n");
		}
		else if(strncmp(strptr, "frag", 4) == 0 || strncmp(strptr, "7", 1) == 0)
		{
			re_private_data[gmac].cp_stats.frag=0;
			printk("frag software counter cleared !\n");
		}
		else if(strncmp(strptr, "toobig", 6) == 0 || strncmp(strptr, "8", 1) == 0)
		{
			re_private_data[gmac].cp_stats.toobig=0;
			printk("toobig software counter cleared !\n");
		}
		else if(strncmp(strptr, "crcerr", 6) == 0 || strncmp(strptr, "9", 1) == 0)
		{
			re_private_data[gmac].cp_stats.crcerr=0;
			printk("crcerr software counter cleared !\n");
		}
		else if(strncmp(strptr, "rcdf", 4) == 0 || strncmp(strptr, "10", 2) == 0)
		{
			re_private_data[gmac].cp_stats.rcdf=0;
			printk("rcdf software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_no_mem", 9) == 0 || strncmp(strptr, "11", 2) == 0)
		{
			re_private_data[gmac].cp_stats.rx_no_mem=0;
			printk("rx_no_mem software counter cleared !\n");
		}
		else if(strncmp(strptr, "tx_sw_num", 9) == 0 || strncmp(strptr, "12", 2) == 0)
		{
			re_private_data[gmac].cp_stats.tx_sw_num=0;
			printk("tx_sw_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "tx_hw_num", 9) == 0 || strncmp(strptr, "13", 2) == 0)
		{
			re_private_data[gmac].cp_stats.tx_hw_num=0;
			printk("tx_hw_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "tx_no_desc", 10) == 0 || strncmp(strptr, "14", 2) == 0)
		{
			re_private_data[gmac].cp_stats.tx_no_desc=0;
			printk("tx_no_desc software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_critical_num", 10) == 0 || strncmp(strptr, "15", 2) == 0)
		{
			re_private_data[gmac].cp_stats.rx_critical_num=0;
			printk("rx_critical_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_cri_no_desc", 10) == 0 || strncmp(strptr, "16", 2) == 0)
		{
			re_private_data[gmac].cp_stats.rx_critical_drop_num=0;
			printk("tx_cri_no_desc software counter cleared !\n");
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("error input: \n");
		printk("1/ all - clear all\n");
		printk("2/ rx_hw_num - clear rx_hw_num\n");
		printk("3/ rx_sw_num - clear rx_sw_num\n");
		printk("4/ rer_runt - clear rer_runt\n");
		printk("5/ rer_ovf - clear rer_ovf\n");
		printk("6/ rdu - clear rdu\n");
		printk("7/ frag - clear frag\n");
		printk("8/ toobig - clear toobig\n");
		printk("9/ crcerr - clear crcerr\n");
		printk("10/ rcdf - clear rcdf\n");
		printk("11/ rx_no_mem - clear rx_no_mem\n");
		printk("12/ tx_sw_num - clear tx_sw_num\n");
		printk("13/ tx_hw_num - clear tx_hw_num\n");
		printk("14/ tx_no_desc - clear tx_no_desc\n");
	}
	return count;
}

static int rx_ring_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	unsigned int	ring_num, fc_enabled;
	static struct re_private *data;
	unsigned int gmac;
	char 	tmpbuf[64];
	char	*strptr;
	char	*tokptr;
	int 	i=0;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count-1] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		if(!strcmp(tokptr, "free"))
		{
			printk("\r\nFree 20 Rx skb to GMAC ring 0 !! \n\n",0);
			if(data->rx_Mring[0] == 0 || data->rx_skb[0] == 0)
			{
				printk("no rx_ring info!\n");
			}

			for(i=0;i<20;i++)
			{
				printk("[idx%3d]:desc[0x%p] \n", data->rx_Mtail[0], &data->rx_Mring[0][data->rx_Mtail[0]]);
				data->rx_Mring[0][data->rx_Mtail[0]].opts1 = (DescOwn | data->rx_buf_sz) | ((data->rx_Mtail[0] == (re8670_rx_ring_size[gmac][0] - 1))?RingEnd:0);
				updateGmacFlowControl(gmac, data->rx_Mtail[0], 0);
				data->rx_Mtail[0] = NEXT_RX(data->rx_Mtail[0],re8670_rx_ring_size[gmac][0]);
			}
		}
		else if(!strcmp(tokptr, "fc"))
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			ring_num=simple_strtol(tokptr, NULL, 0);
			if(ring_num < 0 || ring_num>=MAX_RXRING_NUM)
			{
				goto errout;	
			}
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;	
			}
			fc_enabled=simple_strtol(tokptr, NULL, 0);
			if(fc_enabled != 0 && fc_enabled != 1)
			{
				goto errout;
			}
			re8686_set_flow_control(gmac, ring_num, fc_enabled);
			printk("\r\nGMAC%d ring%d flow control is %s \n", gmac, ring_num, fc_enabled?"Enabled":"Disabled");
			goto show_fc_result;
		}
		else if(!strcmp(tokptr, "?") || !strcmp(tokptr, "help"))
		{
			goto errout;
		}
		else 
		{
			rx_ring_show_bitmap[gmac] = simple_strtol(tokptr, NULL, 0);
			printk("\r\nrx_ring_show_bitmap 0x%08x \n", rx_ring_show_bitmap[gmac]);
		}
	}
	else
	{
errout:
		printk("\r\nRx ring operation only support \"fc\", \"?\" or \"help\" as the first parameter\n");
		printk("Ring flow control:	\"fc ring_num(0~%d) switch(0/1)\"\n", (MAX_RXRING_NUM-1));
		printk("rx_ring bitmap: 0x%08x\n", rx_ring_show_bitmap[gmac]);
show_fc_result:
		printk("flow control of GMAC%d of each Rx ring:\n", gmac);
		for(i=0;i<MAX_RXRING_NUM;i++)
		{
			printk("	Rx ring %d is %s\n", i, (re8670_rx_flow_control_status[gmac][i]==ON)?"ON":"OFF");
		}
	}
	return count;
}

static int rx_ring_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	int len = 0;
	int i = 0,j = 0;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	for(j=0;j<MAX_RXRING_NUM;j++)
	{
		if(rx_ring_show_bitmap[gmac]&(1<<j))
		{
			printk("==================rx_ring [%d] info==================\n\n",j);
			if(data->rx_Mring[j] == 0 || data->rx_skb[j] == 0){
				printk("no rx_ring info!\n");
				continue;
			}

			for(i=0;i<re8670_rx_ring_size[gmac][j];i++){
				printk("[idx%3d]:desc[0x%p]->skb[0x%p]->buf[0x%08x]:%s", 
					i, &data->rx_Mring[j][i], data->rx_skb[j][i].skb, 
					data->rx_Mring[j][i].addr, 
					(data->rx_Mring[j][i].opts1 & DescOwn)? "NIC" : "CPU");
				if(i == data->rx_Mtail[j]){
					printk("<=rx_tail");
				}
				printk("\n");
			}
		}
	}
	return len;
}

static int tx_ring_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	char 	tmpbuf[64];
	char	*strptr;
	char	*tokptr;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		tx_ring_show_bitmap[gmac] = simple_strtol(tokptr, NULL, 0);
		printk("tx_ring_show_bitmap 0x%08x\n", tx_ring_show_bitmap[gmac]);		
	}
	else
	{
errout:
		printk("tx_ring_show_bitmap: 0x%08x\n", tx_ring_show_bitmap[gmac]);
	}
	return 0;
}
static int tx_ring_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	int len = 0;
	int i = 0,j = 0;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	for(j=0;j<MAX_TXRING_NUM;j++)
	{
		if(tx_ring_show_bitmap[gmac]&(1<<j))
		{
			printk("=============FDP%d:tx_ring [%d] info==================\n\n",idx_sw2hw(j)+1,j);
			if(data->tx_Mhqring[j] == 0 || data->tx_skb[j] == 0){
				printk("no tx_ring info\n");
				continue;
			}

			for(i=0;i<re8670_tx_ring_size[gmac][j];i++) {
				printk("[idx%3d]:desc[0x%p]->skb[0x%p]->buf[0x%08x]:%s", 
					i, &data->tx_Mhqring[j][i], data->tx_skb[j][i].skb, 
					data->tx_Mhqring[j][i].addr, 
					(data->tx_Mhqring[j][i].opts1 & DescOwn)? "NIC" : "CPU");
				if(i == data->tx_Mhqtail[j]) {
					printk("<=tx_hqtail");
				}
				if(i == data->tx_Mhqhead[j]) {
					printk("<=tx_hqhead");
				}
				printk("\n");
			}
		}
	}

	return len;
}


#ifdef KERNEL_SOCKET	
struct workqueue_struct *wq=NULL;
struct wq_wrapper{
    struct work_struct worker;
};
struct wq_wrapper wq_data;
#endif

void nic_tx_ring_dump(unsigned int gmac)
{
	struct re_private *cp;
	DMA_TX_DESC *txd;
	int i;
	int ring_index=0;
	
	cp=&re_private_data[gmac];	
	for(i=0;i<re8670_tx_ring_size[gmac][ring_index];i++)
	{
		txd = (DMA_TX_DESC *)((u32)&cp->tx_Mhqring[ring_index][i]|0xa0000000);
		
		printk("%08x[%03d] %08x %08x %08x OWN=%d E=%d F=%d L=%d LEN=%05d LSO=%d MTU=%d\n",(u32)&txd->opts1,i,txd->opts1,txd->addr,txd->opts2
			,(txd->opts1&0x80000000)?1:0
			,(txd->opts1&0x40000000)?1:0
			,(txd->opts1&0x20000000)?1:0
			,(txd->opts1&0x10000000)?1:0
			,(txd->opts1&0x1ffff)			
			,(txd->opts4&0x80000000)?1:0
			,(txd->opts4&0x7ff00000)>>20
		);
	}
	printk("\n");
	
#ifdef KERNEL_SOCKET	

	// free old workqueue
	if (wq) {
		flush_workqueue(wq);
		destroy_workqueue(wq);
		wq=NULL;
	}

	INIT_WORK(&wq_data.worker, kernel_socket);
#ifdef TCP_SOCKET	
	wq = create_singlethread_workqueue("tcp_kernel_socket wq"); 
#elif defined(UDP_SOCKET)
	wq = create_singlethread_workqueue("udp_kernel_socket wq"); 
#else
	#error "not config"
#endif
	if (!wq){
		printk("ERR AT:%s %d\n",__func__,__LINE__);			
		return;
	}
	queue_work(wq, &wq_data.worker);	
	printk("%s %d\n",__func__,__LINE__);
#endif
	
}

static int nic_tx_ring_dump_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	nic_tx_ring_dump(gmac);
	return 0;
}

static int nic_tx_ring_dump_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{	
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	nic_tx_ring_dump(gmac);
	return 0;
}


void nic_rx_ring_dump(unsigned int gmac)
{
	struct re_private *cp;
	DMA_RX_DESC *rxd;
	int i;
	int ring_index=0;

	cp=&re_private_data[gmac];
	for(i=0;i<re8670_rx_ring_size[gmac][ring_index];i++)
	{
		rxd = (DMA_RX_DESC *)((u32)&cp->rx_Mring[ring_index][i]|0xa0000000);
		printk("%08x[%03d] %08x %08x %08x %08x OWN=%d E=%d LEN=%05d\n",(u32)&rxd->opts1,i,rxd->opts1,rxd->addr,rxd->opts2,rxd->opts3
			,(rxd->opts1&0x80000000)?1:0
			,(rxd->opts1&0x40000000)?1:0
			,(rxd->opts1&0xfff)
		);
	}
	printk("\n");
}

static int nic_rx_ring_dump_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	nic_rx_ring_dump(gmac);
	return 0;
}

static int nic_rx_ring_dump_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	nic_rx_ring_dump(gmac);
	return 0;
}

static int padding_enable_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	//printk("[padding_enable = %d]\n", padding_enable);
	switch (padding_enable[gmac]) {
	case 0:
		printk("0, (padding is disable)\n");
		break;
	case 1:
		printk("1, (padding is enable)\n");
		break;
	default:
		printk("Error, (padding_enable is invalid)\n");
	}
	return 0;
}

void gmac_padding_enable(unsigned int gmac, rtk_enable_t enable)
{
	if (gmac>2)
		goto error;
	#ifndef CONFIG_GMAC1_USABLE
	if (gmac==1)
		goto error;
	#endif
	#ifndef CONFIG_GMAC2_USABLE
	if (gmac==2)
		goto error;
	#endif
	if(enable==ENABLED)
	{
		padding_enable[gmac]=1;
		RLE0787_W32(gmac, TCR, RLE0787_R32(gmac, TCR) & (~0x1));
		printk("Enable gmac[%d] padding\n", gmac);
	}
	else
	{
		padding_enable[gmac]=0;
		RLE0787_W32(gmac, TCR, RLE0787_R32(gmac, TCR) | 0x1);
		printk("Disable gmac[%d] padding\n", gmac);
	}
	
	return;
error:
	printk("%s: ignored gmac %d\n",__func__,gmac);
}

static int padding_enable_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	unsigned char tmpBuf[16] = {0};
	int i=0;	
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	if (buf && count==2 && !copy_from_user(tmpBuf, buf, count))
	{
		tmpBuf[count] = '\0';
		i=simple_strtoul(tmpBuf, NULL, 10);
		switch (i) {
		case 0:
			gmac_padding_enable(gmac, DISABLED);
			break;
		case 1:
			gmac_padding_enable(gmac, ENABLED);
			break;
		default:
			printk("Unknown Setting\nDisable: echo 0 > /proc/rtl8686gmac/padding_enable\nEnable: echo 1 > /proc/rtl8686gmac/padding_enable\n");
			break;
		}
	}

	return count;
}

static int dbg_level_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}

static const struct file_operations dbglv_fops = {
        .owner          = THIS_MODULE,
        .open           = dbg_level_open,
        .read           = dbg_level_read,
        .write          = dbg_level_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int hwreg_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations hwreg_fops = {
        .owner          = THIS_MODULE,
        .open           = hwreg_open,
        .read           = hw_reg_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int sw_cnt_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations swcnt_fops = {
        .owner          = THIS_MODULE,
        .open           = sw_cnt_open,
        .read           = sw_cnt_read,
        .write           = sw_cnt_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int rxring_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations rxring_fops = {
        .owner          = THIS_MODULE,
        .open           = rxring_open,
        .read           = rx_ring_read,
        .llseek         = seq_lseek,
		.write          = rx_ring_write,
        .release        = single_release,
};

static int txring_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations txring_fops = {
        .owner          = THIS_MODULE,
        .open           = txring_open,
        .read           = tx_ring_read,
        .llseek         = seq_lseek,
		.write          = tx_ring_write,
        .release        = single_release,
};

static int memrw_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations mem_fops = {
        .owner          = THIS_MODULE,
        .open           = memrw_open,
        .read           = memrw_read,
        .write          = memrw_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int dev_port_mapping_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations devport_fops = {
        .owner          = THIS_MODULE,
        .open           = dev_port_mapping_open,
        .read           = dev_port_mapping_read,
        .write          = dev_port_mapping_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int misc_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations misc_fops = {
        .owner          = THIS_MODULE,
        .open           = misc_open,
        .read           = misc_read,
        .write          = misc_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int port_to_rxfunc_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations port2rx_fops = {
        .owner          = THIS_MODULE,
        .open           = port_to_rxfunc_open,
        .read           = port_to_rxfunc_read,
        .write          = port_to_rxfunc_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

#ifdef CONFIG_RTL8686_SWITCH
#ifndef CONFIG_RTK_L34_ENABLE
static int swmode_open(struct inode *inode, struct file *file)
{
        return single_open(file, switch_mode_read, inode->i_private);
}
static const struct file_operations swmode_fops = {
        .owner          = THIS_MODULE,
        .open           = swmode_open,
        .read           = seq_read,
        .write          = switch_mode_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#endif // CONFIG_RTK_L34_ENABLE
#endif // CONFIG_RTL8686_SWITCH

#ifdef RE8686_VERIFY
static int verify_open(struct inode *inode, struct file *file)
{
        return single_open(file, verify_read, inode->i_private);
}
static const struct file_operations verify_fops = {
        .owner          = THIS_MODULE,
        .open           = verify_open,
        .read           = seq_read,
        .write          = verify_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#endif //RE8686_VERIFY

static int txdetail_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations txdetail_fops = {
        .owner          = THIS_MODULE,
        .open           = txdetail_open,
        .read           = nic_tx_ring_dump_read,
        .write          = nic_tx_ring_dump_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int rxdetail_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations rxdetail_fops = {
        .owner          = THIS_MODULE,
        .open           = rxdetail_open,
        .read           = nic_rx_ring_dump_read,
        .write          = nic_rx_ring_dump_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int padding_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations padding_fops = {
        .owner          = THIS_MODULE,
        .open           = padding_open,
        .read           = padding_enable_read,
        .write          = padding_enable_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

#define RTL8686_GMAC0_PROC_DIR_NAME "rtl8686gmac"
#define RTL8686_GMAC1_PROC_DIR_NAME "rtl8686gmac1"
#define RTL8686_GMAC2_PROC_DIR_NAME "rtl8686gmac2"
unsigned char *rtl8686_proc_dir_name[MAX_GMAC_NUM] =
{
	RTL8686_GMAC0_PROC_DIR_NAME,
	RTL8686_GMAC1_PROC_DIR_NAME,
	RTL8686_GMAC2_PROC_DIR_NAME
};
struct proc_dir_entry *rtl8686_proc_dir[MAX_GMAC_NUM]={0};

struct gmac_proc_entry{
	struct proc_dir_entry* p_dir_entry;
	char* entry_name;
	struct file_operations *fops;
};

static void rtl8686_proc_debug_init(void){
	int i;

	for(i=0;i<MAX_GMAC_NUM;i++) {

		if(gmac_enabled[i]==OFF)
			continue;
		
		if(rtl8686_proc_dir[i]==NULL)
			rtl8686_proc_dir[i] = proc_mkdir(rtl8686_proc_dir_name[i], NULL);
		
		if(rtl8686_proc_dir[i])
		{
			proc_create_data("dbg_level", 0, rtl8686_proc_dir[i], &dbglv_fops, &re_private_data[i]);
			proc_create_data("hw_reg", 0, rtl8686_proc_dir[i], &hwreg_fops, &re_private_data[i]);
			proc_create_data("sw_cnt", 0, rtl8686_proc_dir[i], &swcnt_fops, &re_private_data[i]);
			proc_create_data("rx_ring", 0, rtl8686_proc_dir[i], &rxring_fops, &re_private_data[i]);
			proc_create_data("tx_ring", 0, rtl8686_proc_dir[i], &txring_fops, &re_private_data[i]);
			proc_create_data("mem", 0, rtl8686_proc_dir[i], &mem_fops, &re_private_data[i]);
			proc_create_data("dev_port_mapping", 0, rtl8686_proc_dir[i], &devport_fops, &re_private_data[i]);
			proc_create_data("misc", 0, rtl8686_proc_dir[i], &misc_fops, &re_private_data[i]);
			proc_create_data("port2rxfunc", 0, rtl8686_proc_dir[i], &port2rx_fops, &re_private_data[i]);
			#ifdef CONFIG_RTL8686_SWITCH
			#ifndef CONFIG_RTK_L34_ENABLE
			proc_create_data("switch_mode", 0, rtl8686_proc_dir[i], &swmode_fops, &re_private_data[i]);
			#endif
			#endif
			#ifdef RE8686_VERIFY
			proc_create_data("verify", 0, rtl8686_proc_dir[i], &verify_fops, &re_private_data[i]);
			#endif
			proc_create_data("rx_ring_detail", 0, rtl8686_proc_dir[i], &rxdetail_fops, &re_private_data[i]);
			proc_create_data("tx_ring_detail", 0, rtl8686_proc_dir[i], &txdetail_fops, &re_private_data[i]);
			proc_create_data("padding_enable", 0, rtl8686_proc_dir[i], &padding_fops, &re_private_data[i]);
		}
	}
}

void port_relate_setting(void) {
	unsigned int gmac;	
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	int i, j;

	for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) { 
		// Rx function
		re8686_reset_rxfunc_to_default(gmac);
		// port2dev: default set to eth0, eth1 and eth3 respectly
		for(j=0;j<SW_PORT_NUM;j++) {
			re_private_data[gmac].port2dev[j] = eth_net_dev[gmac];
		}
	}
	
	for(i=0;i<totalDev;i++) {
		if(rtl8686_dev_table[i].dev_instant==NULL)
			continue;
		DEVPRIV(rtl8686_dev_table[i].dev_instant)->txPortMask = 
			IS_CPU_PORT(rtl8686_dev_table[i].phyPort) ? 0 : (1<<rtl8686_dev_table[i].phyPort);
	}
}

#if defined(CONFIG_APOLLO_ROMEDRIVER) //&& defined(CONFIG_GPON_FEATURE)
extern int rtk_rg_fwdEngine_xmit (struct sk_buff *skb, struct net_device *dev);
#else 
#if defined(CONFIG_RG_SIMPLE_PROTOCOL_STACK)
extern int rtk_rg_fwdEngine_xmit (struct sk_buff *skb, struct net_device *dev);
#endif
#endif

#if defined(CONFIG_APOLLO_ROMEDRIVER) || defined(CONFIG_RG_SIMPLE_PROTOCOL_STACK)
__IRAM_NIC int re8670_start_xmit_check(struct sk_buff *skb, struct net_device *dev)
{
        // direct tx
        if(memcmp(skb->dev->name,"wlan1",5)==0 || memcmp(skb->dev->name,"vwlan", 5)==0)
        {
                return re8670_start_xmit_txInfo(skb,dev,NULL,NULL);
        }
        else
        {       // send to fwdEngine
			struct tcphdr *tcph;
			unsigned char *pData = skb->data;
			struct iphdr *iph;			
			
#ifdef CONFIG_RTK_HOST_SPEEDUP
            if (isHostSpeedUpEnable())
            {
                pData += 12;
                if (*(unsigned short *)pData == 0x8100)
                        pData += 4;
                if (((*(unsigned short *)pData == 0x8864) && (*(unsigned short *)(pData+8) == 0x0021)) ||
                        (*(unsigned short *)pData == 0x0800))
                {
                    if (*(unsigned short *)pData == 0x0800)
                        iph = (struct iphdr *)(pData + 2);
                    else
                        iph = (struct iphdr *)(pData + 10);

                    if (IPPROTO_TCP == iph->protocol)
                    {
                        tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);

                        if (shouldSpeedUp(1, iph->daddr, iph->saddr, tcph->dest, tcph->source))
                        {
							return re8670_start_xmit_txInfo(skb, dev, NULL, NULL);
                        }
                    }
                }
            }
#endif

#ifdef	SMBSHORTCUT
			if ( smbscflag ){
				pData += 12;
				if (*(unsigned short *)pData == 0x8100)
						pData += 4;
				if (((*(unsigned short *)pData == 0x8864) && (*(unsigned short *)(pData+8) == 0x0021)) ||
						(*(unsigned short *)pData == 0x0800))
				{
					
					if (*(unsigned short *)pData == 0x0800)
						iph = (struct iphdr *)(pData + 2);
					else
						iph = (struct iphdr *)(pData + 10);		
					tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);				
					if (checkLocalIPPort(iph->saddr, tcph->source))
					{
							return re8670_start_xmit_txInfo(skb,dev,NULL,NULL);
					}					
				}
			}
#endif					
                return rtk_rg_fwdEngine_xmit (skb,dev);
        }
}
#elif defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int re8670_start_xmit_flowCache(struct sk_buff *skb, struct net_device *dev)
{
	return rtk_rg_fc_egress_flowLearning(skb, dev, DEVPRIV(dev)->txPortMask);
}
#endif


struct net_device_ops rtl_netdevops = {
    .ndo_open       = re8670_open,
    .ndo_stop       = re8670_close,
#if defined(CONFIG_RG_SIMPLE_PROTOCOL_STACK) ||defined(CONFIG_APOLLO_ROMEDRIVER)
    .ndo_start_xmit = re8670_start_xmit_check,
#elif defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	.ndo_start_xmit = re8670_start_xmit_flowCache,
#else
	.ndo_start_xmit = re8670_start_xmit,
#endif
    .ndo_do_ioctl   = re8670_ioctl,
    .ndo_tx_timeout = re8670_tx_timeout,
	.ndo_set_rx_mode = re8670_set_rx_mode,
	.ndo_set_mac_address = re8686_set_mac_addr,
	.ndo_get_stats = re8670_get_stats,
	.ndo_change_mtu = re8686_set_mtu,
};
#if defined(CONFIG_DUALBAND_CONCURRENT) && defined(CONFIG_VIRTUAL_WLAN_DRIVER)
struct net_device_ops rtl_wlan1_netdevops = {
    .ndo_open       = re8670_open,
    .ndo_stop       = re8670_close,
#if defined(CONFIG_RG_SIMPLE_PROTOCOL_STACK) ||defined(CONFIG_APOLLO_ROMEDRIVER)
    .ndo_start_xmit = re8670_start_xmit_check,
#else
	.ndo_start_xmit = re8670_start_xmit,
#endif
    .ndo_do_ioctl   = vwlan_ioctl,
    .ndo_tx_timeout = re8670_tx_timeout,
	.ndo_set_rx_mode = re8670_set_rx_mode,
	.ndo_set_mac_address = vwlan_set_hwaddr,
	.ndo_get_stats = re8670_get_stats,
	.ndo_change_mtu = re8686_set_mtu,
};
#endif

int __init re8670_probe (void)
{	
	unsigned int current_gmac=0;
	unsigned int gmac;	
	unsigned int ring_index;
	unsigned int i, j;
	struct net_device *dev;
	int rc;

#ifdef CONFIG_RG_SIMPLE_PROTOCOL_STACK 
	unsigned wanCount=0;
#endif
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
    extern int drv_nic_rxhook_init(void);
#if defined(CONFIG_DUALBAND_CONCURRENT) && defined(CONFIG_VIRTUAL_WLAN_DRIVER)
	extern int vwlan_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
#endif

#ifdef MODULE
	printk("%s", version);
#endif
#ifndef MODULE
	static int version_printed;
	if (version_printed++ == 0)
		printk("%s", version);
#endif

	// All IPs disabled by bootloader for power saving
	// need to enable these IP first before access it
	re8670_ip_enable_all();

	//sw stuff
#if defined(CONFIG_RTL865X_ETH_PRIV_SKB) || defined(CONFIG_RTL865X_ETH_PRIV_SKB_ADV)
	init_priv_eth_skb_buf();
	init_priv_eth_skb_buf_rg();
#endif

	for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {

		if(gmac_enabled[gmac]==OFF)
			continue;
		
		printk("GMAC%d Tx Ring Size:\n", gmac);
		for(ring_index=0;ring_index<MAX_TXRING_NUM;ring_index++) {
			printk("[#%d]:%d ", ring_index, re8670_tx_ring_size[gmac][ring_index]);
		}
		printk("\nGMAC%d Rx Ring Size:\n", gmac);
		for(ring_index=0;ring_index<MAX_RXRING_NUM;ring_index++) {
			printk("[#%d]:%d ", ring_index, re8670_rx_ring_size[gmac][ring_index]);
		}

		//allocate and register root dev first
		dev = alloc_etherdev(sizeof(struct re_dev_private));
		if (!dev) {
			printk("%s %d alloc_etherdev fail!\n", __func__, __LINE__);
			continue;
		}
		
#ifdef CONFIG_RTL8686_SWITCH 
		memset(LCDev_mapping, 0, SW_PORT_NUM * totalDev);
#endif
		//reset re_private_data
		memset(&re_private_data[gmac], 0, sizeof(struct re_private));
		re_private_data[gmac].gmac = gmac;
		re_private_data[gmac].dev = dev;
		spin_lock_init (&(re_private_data[gmac].tx_lock));
		spin_lock_init (&(re_private_data[gmac].rx_lock));
		re_private_data[gmac].regs = gmac_eth_base[gmac];
		memset(&re_private_data[gmac].rx_tasklets, 0, sizeof(struct tasklet_struct));
		re_private_data[gmac].rx_tasklets.func=(void (*)(unsigned long))re8670_rx;	
		re_private_data[gmac].rx_tasklets.data=(unsigned long)&re_private_data[gmac];

#ifdef KERNEL_SOCKET
		memset(&re_private_data[gmac].tx_tasklets, 0, sizeof(struct tasklet_struct));
		re_private_data[gmac].tx_tasklets.func=(void (*)(unsigned long))re8670_tx_all;
		re_private_data[gmac].tx_tasklets.data=(unsigned long)&re_private_data[gmac];	
#endif	
		eth_rx_tasklets[gmac] = &(re_private_data[gmac].rx_tasklets);
		eth_net_dev[gmac] = dev;
		
		//stop hw
		re8670_stop_hw(&re_private_data[gmac]);		
		re8670_reset_hw(&re_private_data[gmac]);
		
		dev->base_addr = (unsigned long) gmac_eth_base[gmac];
		/* read MAC address from EEPROM */
		for (i = 0; i < 3; i++)
			((u16 *) (dev->dev_addr))[i] = i;

		dev->netdev_ops = &rtl_netdevops;
		dev->watchdog_timeo = TX_TIMEOUT;
#ifdef CONFIG_REALTEK_HW_LSO
#ifdef HW_CHECKSUM_OFFLOAD
		dev->features |= NETIF_F_HW_CSUM;
#endif
#ifdef LINUX_SG_ENABLE
		dev->features |= NETIF_F_SG;
#endif
#ifdef LINUX_LSO_ENABLE
#ifdef HW_LSO_ENABLE
		dev->features |= NETIF_F_GSO | NETIF_F_TSO| NETIF_F_UFO; //: test
#else
	//	dev->features |= NETIF_F_SG |  NETIF_F_GSO; //: this configuration will crash in skb_segment
		dev->features |= NETIF_F_GSO; //:test
#endif
		netif_set_gso_max_size(dev, 65535); 	
#endif
#endif
		dev->irq = re8670_get_irq_number(&re_private_data[gmac]);	// internal phy
		//priv data setting
		dev->priv_flags = IFF_DOMAIN_ELAN;
		DEV2CP(dev) = &re_private_data[gmac];
		rtl8686_dev_table[gmac].dev_instant = dev;
		sprintf(dev->name, rtl8686_dev_table[gmac].ifname);
		rtl_set_ethtool_ops(dev);
		rc = register_netdev(dev);
		if (rc) {
			printk("%s %d rc = %d\n", __func__, __LINE__, rc);
			continue;
		}
		
		printk (KERN_INFO "%s: %s at 0x%lx, "
				"%02x:%02x:%02x:%02x:%02x:%02x, "
				"IRQ %d\n",
				dev->name,
				"RTL-8686",
				dev->base_addr,
				dev->dev_addr[0], dev->dev_addr[1],
				dev->dev_addr[2], dev->dev_addr[3],
				dev->dev_addr[4], dev->dev_addr[5],
				dev->irq);
		
		printk("RTL8686 GMAC Probing..\n");
		
#ifdef CONFIG_DUALBAND_CONCURRENT
		str2mac(CONFIG_DEFAULT_SLAVE_IPC_MAC_ADDRESS, src_mac);
#endif
		
		rc = re8670_alloc_rings(&re_private_data[gmac]);
		if (rc)
			return rc;
		change_tx_jumbo_setup(gmac, txJumboFrameEnabled[gmac]);

		//20170502: disable gmac padding by default
		gmac_padding_enable(gmac, DISABLED);
	}

	// allocate and register other device
	for(j=3;j<totalDev;j++) {
		dev = alloc_etherdev(sizeof(struct re_dev_private));
		if (!dev) {
			printk("%s %d alloc_etherdev fail!\n", __func__, __LINE__);
			goto err_out_iomap;
		}
		/* read MAC address from EEPROM */
		for (i = 0; i < 3; i++)
			((u16 *) (dev->dev_addr))[i] = i;

#if defined(CONFIG_DUALBAND_CONCURRENT) && defined(CONFIG_VIRTUAL_WLAN_DRIVER)
		if(!memcmp(rtl8686_dev_table[j].ifname,"wlan1", 5)) {
			dev->netdev_ops = &rtl_wlan1_netdevops;
		}
		else
			dev->netdev_ops = &rtl_netdevops;
#else
		dev->netdev_ops = &rtl_netdevops;
#endif
		dev->watchdog_timeo = TX_TIMEOUT;
		// priv data setting
		switch(rtl8686_dev_table[j].ifflag)
		{
			case RTL8686_ELAN:
				dev->priv_flags = IFF_DOMAIN_ELAN;
				break;
			case RTL8686_WAN:
				dev->priv_flags = IFF_DOMAIN_WAN;
				break;
			case RTL8686_WLAN:
				dev->priv_flags = IFF_DOMAIN_WLAN;
				break;
			case RTL8686_SVAP:		//for slave VAP interfaces, bridge should not send query for each, just need one for slave's br0
				dev->priv_flags = IFF_DOMAIN_WLAN | IFF_DOMAIN_WAN;
				break;
			default:
				printk("Error! Should not go here!\n");
		}
#ifdef CONFIG_RG_SIMPLE_PROTOCOL_STACK 
		if(rtl8686_dev_table[j].ifflag == RTL8686_WAN)	
			re_private_data[0].multiWanDev[wanCount++]=dev;
#endif

		/* 
Assign eth0~ethX for eth0.2~eth0.X evenly
		   to make sure eth0~ethX devices will be open*/
		if(rtl8686_dev_table[j].isPanelPort)
		{
			if(MAX_GMAC_NUM>1 && current_gmac<MAX_GMAC_NUM && gmac_enabled[current_gmac]==ON)
			{
				DEV2CP(dev) =  &re_private_data[current_gmac];
				dev->base_addr = (unsigned long) gmac_eth_base[current_gmac];
				dev->irq = re8670_get_irq_number(&re_private_data[current_gmac]); // internal phy		
			}
			else
			{
				DEV2CP(dev) = &re_private_data[0];
				dev->base_addr = (unsigned long) gmac_eth_base[0];
				dev->irq = re8670_get_irq_number(&re_private_data[0]); // internal phy	
			}
			current_gmac++;
		}
		else
		{
			DEV2CP(dev) = &re_private_data[0];
			dev->base_addr = (unsigned long) gmac_eth_base[0];
			dev->irq = re8670_get_irq_number(&re_private_data[0]); // internal phy	
		}
		rtl8686_dev_table[j].dev_instant = dev;
		memcpy(dev->name, rtl8686_dev_table[j].ifname, strlen(rtl8686_dev_table[j].ifname));
		
		rtl_set_ethtool_ops(dev);
		rc = register_netdev(dev);
		if (rc) {
			printk("%s %d rc = %d\n", __func__, __LINE__, rc);
			goto err_out_iomap;
		}
#ifdef CONFIG_RTL8686_SWITCH 
		if(j < (MAX_GMAC_NUM+MAX_LAN_PORT+MAX_PON_PORT) && j >= MAX_GMAC_NUM )
		{
			// copy interface name
			memcpy(&LCDev_mapping[rtl8686_dev_table[j].phyPort].ifname,dev->name , strlen(dev->name));
			// assign net_dev
			LCDev_mapping[rtl8686_dev_table[j].phyPort].phy_dev = dev;
			/* sfu requires port link change notifier */
			if ( rtl8686_dev_table[j].isPanelPort )
				netif_carrier_off(LCDev_mapping[rtl8686_dev_table[j].phyPort].phy_dev);
		}
#endif
		printk (KERN_INFO "%s: %s at 0x%lx, "
				"%02x:%02x:%02x:%02x:%02x:%02x, "
				"IRQ %d\n",
				dev->name,
				"RTL-8686",
				dev->base_addr,
				dev->dev_addr[0], dev->dev_addr[1],
				dev->dev_addr[2], dev->dev_addr[3],
				dev->dev_addr[4], dev->dev_addr[5],
				dev->irq);
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		if(!strncmp(dev->name, "nas", 3) && dev->priv_flags == IFF_DOMAIN_WAN)
		{
			netif_carrier_off(dev);
			printk(">>>> Set %s carrier off !!! \n",dev->name);
		}
#endif
	}

	rtl8686_proc_debug_init();
	port_relate_setting();
#ifdef CONFIG_RTL8686_SWITCH
                //init this for gpon driver
        drv_nic_rxhook_init();
#endif

#if 0//def CONFIG_APOLLO_ROMEDRIVER
	{
		rtk_rg_fwdEngine_initParams_t param;
		memset(&param,0,sizeof(rtk_rg_fwdEngine_initParams_t));
		param.lan_dev = rtl8686_dev_table[0].dev_instant;
		param.wan_dev = rtl8686_dev_table[6].dev_instant;
		rtk_rg_fwdEngineInit(&param);
	}
#endif

	nicRootDev=rtl8686_dev_table[0].dev_instant;
	nicRootDevCp=DEV2CP(nicRootDev);
#ifdef CONFIG_RTL8686_SWITCH 
	// port 5 is rgmii port
#ifdef CONFIG_RGMII_RESET_PROCESS
	strcpy(LCDev_mapping[RGMII_PORT].ifname, "eth0");
        LCDev_mapping[RGMII_PORT].phy_dev = eth_net_dev[0];
#else
	strcpy(LCDev_mapping[RGMII_PORT].ifname, "nas0");
	LCDev_mapping[RGMII_PORT].phy_dev = LCDev_mapping[WAN_PORT].phy_dev;
#endif
	/* sfu requires port link change notifier */
	if(intr_bcaster_notifier_cb_register(&GMAClinkChangeNotifier) != 0)
	{
		printk("Interrupt Broadcaster for Link Change Error !! \n");
	}
#if defined(CONFIG_ETHWAN_USE_USB_SGMII) || defined(CONFIG_ETHWAN_USE_PCIE1_SGMII)
	if(intr_bcaster_notifier_cb_register(&GMACethwanStateChangeNotifier) != 0)
	{
		printk("Interrupt Broadcaster for ether WAN Link Change Error !! \n");
	}
#else
	if(intr_bcaster_notifier_cb_register(&GMAConuStateChangeNotifier) != 0)
	{
		printk("Interrupt Broadcaster for onu state Change Error !! \n");
	}
#endif
#endif

#ifdef CONFIG_RTL_MULTI_LAN_DEV
	// Setup default port mapping
	change_dev_port_mapping(LAN_PORT1, "eth0.2");
	change_dev_port_mapping(LAN_PORT2, "eth0.3");
	change_dev_port_mapping(LAN_PORT3, "eth0.4");
	change_dev_port_mapping(LAN_PORT4, "eth0.5");
	change_dev_port_mapping(LAN_PORT5, "eth0.6");
	change_dev_port_mapping(LAN_PORT6, "eth0.7");
#endif

	if(timer_pending(&software_interrupt_timer))
		del_timer(&software_interrupt_timer);	
	init_timer(&software_interrupt_timer);
	software_interrupt_timer.function = setSoftwareInterrupt;
	software_interrupt_timer.data = (unsigned long)NULL;
	
	return 0;

err_out_iomap:
	printk("%s %d: here is a error when probe! \n", __func__, __LINE__);
	return -1 ;
}

void __exit re8670_exit (void)
{
	unsigned i;
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
    extern int drv_nic_rxhook_exit(void);
	unsigned int gmac;

	//desc and hw setting & proc		
	if(dev_num){			
		for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {
			if(gmac_enabled[gmac]==OFF)
				continue;
		
			re8670_close(eth_net_dev[gmac]);
			// release allocated memory after moudle unload 
			re8670_free_rings(&re_private_data[gmac]);
		}			
	}
	
	for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {
		if(gmac_enabled[gmac]==OFF)
			continue;
		
		proc_remove(rtl8686_proc_dir[gmac]);
	}
	
	//dev
	for(i=0; i < totalDev; i++){

		if(gmac_enabled[i]==OFF)
			continue;
		
		if(rtl8686_dev_table[i].dev_instant){
			unregister_netdev(rtl8686_dev_table[i].dev_instant);
			free_netdev(rtl8686_dev_table[i].dev_instant);
		}
	}
	if(timer_pending(&software_interrupt_timer))
		del_timer(&software_interrupt_timer);
#ifdef CONFIG_RTL8686_SWITCH
	//for gpon driver
    drv_nic_rxhook_exit();
#endif
}

module_init(re8670_probe);
module_exit(re8670_exit);
#ifdef CONFIG_WIRELESS_LAN_MODULE
int (*wirelessnet_hook)(void) = NULL;
EXPORT_SYMBOL(wirelessnet_hook);
#endif
EXPORT_SYMBOL(re8686_send_with_txInfo_and_mask);
EXPORT_SYMBOL(re8686_set_flow_control);
EXPORT_SYMBOL(re8686_set_pauseBySw);
EXPORT_SYMBOL(re8686_set_vlan_register);
EXPORT_SYMBOL(re8686_get_vlan_register);

#ifdef SMBSHORTCUT
static int smbsc_read_proc(struct seq_file *seq, void *v)
{
	seq_printf(seq, "SMB serverip = %d.%d.%d.%d\n", 
		(smbserverip & 0xff000000) >> 24, 
		(smbserverip & 0x00ff0000) >> 16, 
		(smbserverip & 0x0000ff00) >> 8, 
		(smbserverip & 0xff));
	if ( smbscflag )
		seq_printf(seq, "SMB shortcut enable\n");
	else
		seq_printf(seq, "SMB shortcut disable\n");
	return 0;
}

static int proc_smbsc_open(struct inode *inode, struct file *file)
{
	return single_open(file, smbsc_read_proc, inode->i_private);
}

//# echo "sip 192.168.1.2" > /proc/smbshortcut
//# echo "set 1" > /proc/smbshortcut
int smbsc_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char tmpbuf[512]={0};
	u32 sip=0;
	unsigned char flag = 0;

	if (buffer && !copy_from_user(tmpbuf, buffer, count))
	{
		char *strptr,*split_str;

		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		split_str=strsep(&strptr," ");
		/*parse command*/

		//printk("split_str [%s]\n", split_str);
		while(1)
		{
			//split_str=strsep(&strptr," ");
			//printk("split_str [%s]\n", split_str);
			if(strcasecmp(split_str, "sip") == 0)
			{
				char *ip_token,*split_ip_token,j;
				split_str=strsep(&strptr," ");
				ip_token=split_str;
				//printk("%s-%d sip=%s\n",__func__,__LINE__,ip_token);
				for(j=0;j<4;j++)
				{
					split_ip_token=strsep(&ip_token,".");
					//printk("ip_token [%s], split_ip_token [%s]\n", ip_token, split_ip_token);
					sip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
					if(ip_token==NULL) {
						smbSetServerIP(sip);
						break;
					}
				}
			}else if(strcasecmp(split_str, "set") == 0)
			{
				split_str=strsep(&strptr," ");
				flag=simple_strtol(split_str, NULL, 0);
				smbSetflag(flag);
				//printk("%s-%d sport=%d\n",__func__,__LINE__,sport);
			}else
				split_str=strsep(&strptr," ");
			if (strptr==NULL) break;
		}
		//printk("sip 0x%x, flag 0x%x\n", sip, flag);

	}
	
	return count;
}

static const struct file_operations smbsc_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_smbsc_open,
        .read           = seq_read,
        .write          = smbsc_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

int __init SMBShortCut_proc_init(void)
{
	struct proc_dir_entry *entry=NULL;

	entry = proc_create_data("smbshortcut", 0644, NULL, &smbsc_fops,NULL);
	if (entry == NULL)
	{
		printk("create proc smbsc fail!\n");		
		remove_proc_entry("smbsc", NULL);
		return -ENOMEM;
	}

	return 0;
}

int SMBShortCut_proc_exit(void)
{

	return 0;
}
module_init(SMBShortCut_proc_init);
#endif
