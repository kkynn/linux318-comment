/*	
 *	re8686_rtl9607c.h
*/
#ifndef _RE8686_RTL9607C_H_
#define _RE8686_RTL9607C_H_
#include <linux/netdevice.h>
#include <uapi/linux/if.h>


/*================================================
				GMAC used Macros
================================================*/
#define ON 1
#define OFF 0
#define ADDR_OFFSET 0x0010

#define CONFIG_RG_JUMBO_FRAME 1
//#define RTL0371
//#define GMAC_FPGA

#ifdef CONFIG_RG_JUMBO_FRAME
#define RE8686_ETH_DATA_LEN 10000
#endif

#define VPORT_CPU_TAG			0
#define VPORT_VLAN_TAG			1
#define ETH_WAN_PORT 			3 // in virtual view
#define SW_PORT_NUM 			11
#define APOLLOPRO_PON_PORT		5
#define APOLLOPRO_SGMII0_PORT	6
#define APOLLOPRO_SGMII1_PORT	7
#define RGMII_PORT 				8
#define CPU_PORT0 				9		// IA
#define CPU_PORT1 				10		// IA
#define CPU_PORT2 				7		// 5281
#define LAN_PORT1				0
#define LAN_PORT2				1
#define LAN_PORT3				2
#define LAN_PORT4				3
#define LAN_PORT5				4
#define LAN_PORT6				8
#if defined(CONFIG_ETHWAN_USE_USB_SGMII)
#define WAN_PORT 				APOLLOPRO_SGMII0_PORT
#elif defined(CONFIG_ETHWAN_USE_PCIE1_SGMII)
#define WAN_PORT 				APOLLOPRO_SGMII1_PORT
#else
#define WAN_PORT 				APOLLOPRO_PON_PORT
#endif

#define MAX_GMAC_NUM		3
#define MAX_LAN_PORT		6
#define MAX_PON_PORT		1
#define IS_CPU_PORT(port)	(port==CPU_PORT0||port==CPU_PORT1||port==CPU_PORT2)
#define IS_LAN_PORT(port)	(port==LAN_PORT1||port==LAN_PORT2||port==LAN_PORT3||port==LAN_PORT4||port==LAN_PORT5||port==LAN_PORT6)
#define IS_PON_PORT(port)	(port==APOLLOPRO_PON_PORT)
#define IS_WAN_PORT(port)	(port==WAN_PORT)

#define GMAC0_ETHBASE	0xB8012000  // memory mapping of GMAC0
#define GMAC1_ETHBASE	0xB8014000	// memory mapping of GMAC1
#define GMAC2_ETHBASE	0xB8016000	// memory mapping of GMAC2

#define MAX_RXRING_NUM 6
#define MAX_TXRING_NUM 5
// plz use the order of 2, set size 0 to no use ring
// GMAC0, Rx ring:

#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
#define GMAC0_RX1_SIZE 4096
#elif defined(CONFIG_YUEME)
#define GMAC0_RX1_SIZE 2048
#else
#define GMAC0_RX1_SIZE 1024
#endif
#define GMAC0_RX2_SIZE 256
#define GMAC0_RX3_SIZE 256
#define GMAC0_RX4_SIZE 64
#define GMAC0_RX5_SIZE 64
#define GMAC0_RX6_SIZE 64

// GMAC0, Tx ring:
#define GMAC0_TX1_SIZE 1024
#define GMAC0_TX2_SIZE 64
#define GMAC0_TX3_SIZE 64
#define GMAC0_TX4_SIZE 64
#define GMAC0_TX5_SIZE 64

// GMAC1, Rx ring:
#ifdef CONFIG_GMAC1_USABLE
#define GMAC1_RX1_SIZE 1024
#define GMAC1_RX2_SIZE 256
#define GMAC1_RX3_SIZE 256
#define GMAC1_RX4_SIZE 64
#define GMAC1_RX5_SIZE 64
#define GMAC1_RX6_SIZE 64
#else 
#define GMAC1_RX1_SIZE 0
#define GMAC1_RX2_SIZE 0
#define GMAC1_RX3_SIZE 0
#define GMAC1_RX4_SIZE 0
#define GMAC1_RX5_SIZE 0
#define GMAC1_RX6_SIZE 0
#endif 

// GMAC1, Tx ring:
#define GMAC1_TX1_SIZE 1024
#define GMAC1_TX2_SIZE 64
#define GMAC1_TX3_SIZE 64
#define GMAC1_TX4_SIZE 64
#define GMAC1_TX5_SIZE 64

// GMAC2, Rx ring:
#ifdef CONFIG_GMAC2_USABLE
#define GMAC2_RX1_SIZE 256
#define GMAC2_RX2_SIZE 256
#define GMAC2_RX3_SIZE 256
#define GMAC2_RX4_SIZE 64
#define GMAC2_RX5_SIZE 64
#define GMAC2_RX6_SIZE 64
#else
#define GMAC2_RX1_SIZE 0
#define GMAC2_RX2_SIZE 0
#define GMAC2_RX3_SIZE 0
#define GMAC2_RX4_SIZE 0
#define GMAC2_RX5_SIZE 0
#define GMAC2_RX6_SIZE 0
#endif 

// GMAC2, Tx ring:
#define GMAC2_TX1_SIZE 256
#define GMAC2_TX2_SIZE 64
#define GMAC2_TX3_SIZE 64
#define GMAC2_TX4_SIZE 64
#define GMAC2_TX5_SIZE 64

// GMAC0 total ring size:
#define GMAC0_RX_SIZE	(GMAC0_RX1_SIZE+GMAC0_RX2_SIZE+GMAC0_RX3_SIZE+GMAC0_RX4_SIZE+GMAC0_RX5_SIZE+GMAC0_RX6_SIZE)
// GMAC1 total ring size:
#define GMAC1_RX_SIZE	(GMAC1_RX1_SIZE+GMAC1_RX2_SIZE+GMAC1_RX3_SIZE+GMAC1_RX4_SIZE+GMAC1_RX5_SIZE+GMAC1_RX6_SIZE)
// GMAC2 total ring size:
#define GMAC2_RX_SIZE	(GMAC2_RX1_SIZE+GMAC2_RX2_SIZE+GMAC2_RX3_SIZE+GMAC2_RX4_SIZE+GMAC2_RX5_SIZE+GMAC2_RX6_SIZE)

#define RE8670_RX_RING_SIZE \
GMAC0_RX_SIZE+   \
GMAC1_RX_SIZE+   \
GMAC2_RX_SIZE


#define RDU6	(1 << 15)
#define RDU5	(1 << 14)
#define RDU4	(1 << 13)
#define RDU3	(1 << 12)
#define RDU2	(1 << 11)
#define RDU		(1 << 5)
#define RX_RDU_CHECK(x) (((x&(RDU2|RDU3|RDU4|RDU5|RDU6))>>11<<1)|(((x&RDU)>>5)&1))

#define GMAC0_RX_MULTIRING_BITMAP	((GMAC0_RX1_SIZE? 1: 0) \
								| (GMAC0_RX2_SIZE? 1<<1 : 0) \
								| (GMAC0_RX3_SIZE? 1<<2 : 0) \
								| (GMAC0_RX4_SIZE? 1<<3 : 0) \
								| (GMAC0_RX5_SIZE? 1<<4 : 0) \
								| (GMAC0_RX6_SIZE? 1<<5 : 0))
#define GMAC1_RX_MULTIRING_BITMAP ((GMAC1_RX1_SIZE? 1: 0) \
								| (GMAC1_RX2_SIZE? 1<<1 : 0) \
								| (GMAC1_RX3_SIZE? 1<<2 : 0) \
								| (GMAC1_RX4_SIZE? 1<<3 : 0) \
								| (GMAC1_RX5_SIZE? 1<<4 : 0) \
								| (GMAC1_RX6_SIZE? 1<<5 : 0))
#define GMAC2_RX_MULTIRING_BITMAP ((GMAC2_RX1_SIZE? 1: 0) \
								| (GMAC2_RX2_SIZE? 1<<1 : 0) \
								| (GMAC2_RX3_SIZE? 1<<2 : 0) \
								| (GMAC2_RX4_SIZE? 1<<3 : 0) \
								| (GMAC2_RX5_SIZE? 1<<4 : 0) \
								| (GMAC2_RX6_SIZE? 1<<5 : 0))
#define GMAC0_TX_MULTIRING_BITMAP ((GMAC0_TX1_SIZE? 1 : 0) \
								| (GMAC0_TX2_SIZE? 1<<1 : 0) \
								| (GMAC0_TX3_SIZE? 1<<2 : 0) \
								| (GMAC0_TX4_SIZE? 1<<3 : 0) \
								| (GMAC0_TX5_SIZE? 1<<4 : 0))
#define GMAC1_TX_MULTIRING_BITMAP ((GMAC1_TX1_SIZE? 1 : 0) \
								| (GMAC1_TX2_SIZE? 1<<1 : 0) \
								| (GMAC1_TX3_SIZE? 1<<2 : 0) \
								| (GMAC1_TX4_SIZE? 1<<3 : 0) \
								| (GMAC1_TX5_SIZE? 1<<4 : 0))
#define GMAC2_TX_MULTIRING_BITMAP ((GMAC2_TX1_SIZE? 1 : 0) \
								| (GMAC2_TX2_SIZE? 1<<1 : 0) \
								| (GMAC2_TX3_SIZE? 1<<2 : 0) \
								| (GMAC2_TX4_SIZE? 1<<3 : 0) \
								| (GMAC2_TX5_SIZE? 1<<4 : 0))

#define GMAC0_RX_ONLY_RING1			((GMAC0_TX_MULTIRING_BITMAP == 1)? 1 : 0)
#define GMAC0_RX_NOT_ONLY_RING1		((GMAC0_TX_MULTIRING_BITMAP == 1)? 0 : 1)
#define GMAC1_RX_ONLY_RING1			((GMAC1_TX_MULTIRING_BITMAP == 1)? 1 : 0)
#define GMAC1_RX_NOT_ONLY_RING1		((GMAC1_TX_MULTIRING_BITMAP == 1)? 0 : 1)
#define GMAC2_RX_ONLY_RING1			((GMAC2_TX_MULTIRING_BITMAP == 1)? 1 : 0)
#define GMAC2_RX_NOT_ONLY_RING1		((GMAC2_TX_MULTIRING_BITMAP == 1)? 0 : 1)

#define CMD_CONFIG	0xd059f130	// 0x4049E130// pkt timer = 15 => 4pkt trigger int
#define CMD1_CONFIG	0x30000000   // desc format ==> apollo type, not support multiple ring

/* Macros for Tx/Rx info set/get */
#define GMAC_RXINFO_OWN(x)                  (((struct rx_info *)x)->opts1.bit.own)
#define GMAC_RXINFO_EOR(x)                  (((struct rx_info *)x)->opts1.bit.eor)
#define GMAC_RXINFO_FS(x)                   (((struct rx_info *)x)->opts1.bit.fs)
#define GMAC_RXINFO_LS(x)                   (((struct rx_info *)x)->opts1.bit.ls)
#define GMAC_RXINFO_CRCERR(x)               (((struct rx_info *)x)->opts1.bit.crcerr)
#define GMAC_RXINFO_IPV4CSF(x)              (((struct rx_info *)x)->opts1.bit.ipv4csf)
#define GMAC_RXINFO_L4CSF(x)                (((struct rx_info *)x)->opts1.bit.l4csf)
#define GMAC_RXINFO_RCDF(x)                 (((struct rx_info *)x)->opts1.bit.rcdf)
#define GMAC_RXINFO_IPFRAG(x)               (((struct rx_info *)x)->opts1.bit.ipfrag)
#define GMAC_RXINFO_PPPOETAG(x)             (((struct rx_info *)x)->opts1.bit.pppoetag)
#define GMAC_RXINFO_RWT(x)                  (((struct rx_info *)x)->opts1.bit.rwt)
#define GMAC_RXINFO_DATA_LENGTH(x)          (((struct rx_info *)x)->opts1.bit.data_length)

#define GMAC_RXINFO_CPUTAG(x)               (((struct rx_info *)x)->opts2.bit.cputag)
#define GMAC_RXINFO_PTP_EXIST(x)            (((struct rx_info *)x)->opts2.bit.ptp_in_cpu_tag_exist)
#define GMAC_RXINFO_SVLAN_EXIST(x)          (((struct rx_info *)x)->opts2.bit.svlan_tag_exist)
#define GMAC_RXINFO_REASON(x)               (((struct rx_info *)x)->opts2.bit.reason)
#define GMAC_RXINFO_CTAGVA(x)               (((struct rx_info *)x)->opts2.bit.ctagva)
#define GMAC_RXINFO_CVLAN_TAG(x)            (((struct rx_info *)x)->opts2.bit.cvlan_tag)

#define GMAC_RXINFO_INTERNAL_PRIORITY(x)    (((struct rx_info *)x)->opts3.bit.internal_priority)
#define GMAC_RXINFO_PON_STREAM_ID(x)        (((struct rx_info *)x)->opts3.bit.pon_sid_or_extspa)
#define GMAC_RXINFO_EXTSPA(x)               (((struct rx_info *)x)->opts3.bit.pon_sid_or_extspa)
#define GMAC_RXINFO_L3ROUTING(x)            (((struct rx_info *)x)->opts3.bit.l3routing)
#define GMAC_RXINFO_ORIGFORMAT(x)           (((struct rx_info *)x)->opts3.bit.origformat)
#define GMAC_RXINFO_SRC_PORT_NUM(x)         (((struct rx_info *)x)->opts3.bit.src_port_num)
#define GMAC_RXINFO_FBI(x)                  (((struct rx_info *)x)->opts3.bit.fbi)
#define GMAC_RXINFO_DST_PORT_MASK(x)        (((struct rx_info *)x)->opts3.bit.fb_hash_or_dst_portmsk)
#define GMAC_RXINFO_FB_HASH(x)              (((struct rx_info *)x)->opts3.bit.fb_hash_or_dst_portmsk)

#define GMAC_RXINFO_PKTTYPE(x)              (0)/* No such field */
#define GMAC_RXINFO_PCTRL(x)                (0)/* No such field */
#define GMAC_RXINFO_EXT_PORT_TTL_1(x)       (0)/* No such field */

#define GMAC_TXINFO_OWN(x)                  (((struct tx_info *)x)->opts1.bit.own)
#define GMAC_TXINFO_EOR(x)                  (((struct tx_info *)x)->opts1.bit.eor)
#define GMAC_TXINFO_FS(x)                   (((struct tx_info *)x)->opts1.bit.fs)
#define GMAC_TXINFO_LS(x)                   (((struct tx_info *)x)->opts1.bit.ls)
#define GMAC_TXINFO_IPCS(x)                 (((struct tx_info *)x)->opts1.bit.ipcs)
#define GMAC_TXINFO_L4CS(x)                 (((struct tx_info *)x)->opts1.bit.l4cs)
#define GMAC_TXINFO_TPID_SEL(x)             (((struct tx_info *)x)->opts1.bit.tpid_sel)
#define GMAC_TXINFO_STAG_AWARE(x)           (((struct tx_info *)x)->opts1.bit.stag_aware)
#define GMAC_TXINFO_CRC(x)                  (((struct tx_info *)x)->opts1.bit.crc)
#define GMAC_TXINFO_DATA_LENGTH(x)          (((struct tx_info *)x)->opts1.bit.data_length)

#define GMAC_TXINFO_CPUTAG(x)               (((struct tx_info *)x)->opts2.bit.cputag)
#define GMAC_TXINFO_TX_SVLAN_ACTION(x)      (((struct tx_info *)x)->opts2.bit.tx_svlan_action)
#define GMAC_TXINFO_TX_CVLAN_ACTION(x)      (((struct tx_info *)x)->opts2.bit.tx_cvlan_action)
#define GMAC_TXINFO_TX_PMASK(x)             (((struct tx_info *)x)->opts2.bit.tx_portmask)
#define GMAC_TXINFO_CVLAN_VIDL(x)           (((struct tx_info *)x)->opts2.bit.cvlan_vidl)
#define GMAC_TXINFO_CVLAN_PRIO(x)           (((struct tx_info *)x)->opts2.bit.cvlan_prio)
#define GMAC_TXINFO_CVLAN_CFI(x)            (((struct tx_info *)x)->opts2.bit.cvlan_cfi)
#define GMAC_TXINFO_CVLAN_VIDH(x)           (((struct tx_info *)x)->opts2.bit.cvlan_vidh)

#define GMAC_TXINFO_ASPRI(x)                (((struct tx_info *)x)->opts3.bit.aspri)
#define GMAC_TXINFO_CPUTAG_PRI(x)           (((struct tx_info *)x)->opts3.bit.cputag_pri)
#define GMAC_TXINFO_KEEP(x)                 (((struct tx_info *)x)->opts3.bit.keep)
#define GMAC_TXINFO_DISLRN(x)               (((struct tx_info *)x)->opts3.bit.dislrn)
#define GMAC_TXINFO_CPUTAG_PSEL(x)          (((struct tx_info *)x)->opts3.bit.cputag_psel)
#define GMAC_TXINFO_L34_KEEP(x)             (((struct tx_info *)x)->opts3.bit.l34_keep)
#define GMAC_TXINFO_EXTSPA(x)               (((struct tx_info *)x)->opts3.bit.extspa)
#define GMAC_TXINFO_TX_PPPOE_ACTION(x)      (((struct tx_info *)x)->opts3.bit.tx_pppoe_action)
#define GMAC_TXINFO_TX_PPPOE_IDX(x)         (((struct tx_info *)x)->opts3.bit.tx_pppoe_idx)
#define GMAC_TXINFO_TX_DST_STREAM_ID(x)     (((struct tx_info *)x)->opts3.bit.tx_dst_stream_id)

#define GMAC_TXINFO_LGSEN(x)                (((struct tx_info *)x)->opts4.bit.lgsen)
#define GMAC_TXINFO_LGMTU(x)                (((struct tx_info *)x)->opts4.bit.lgmtu)
#define GMAC_TXINFO_SVLAN_VIDL(x)           (((struct tx_info *)x)->opts4.bit.svlan_vidl)
#define GMAC_TXINFO_SVLAN_PRIO(x)           (((struct tx_info *)x)->opts4.bit.svlan_prio)
#define GMAC_TXINFO_SVLAN_CFI(x)            (((struct tx_info *)x)->opts4.bit.svlan_cfi)
#define GMAC_TXINFO_SVLAN_VIDH(x)           (((struct tx_info *)x)->opts4.bit.svlan_vidh)

/* Obsolete fields */
#define GMAC_TXINFO_BLU(x)                  /* No such field */
#define GMAC_TXINFO_VSEL(x)                 /* No such field */
#define GMAC_TXINFO_CPUTAG_IPCS(x)          /* No such field */
#define GMAC_TXINFO_CPUTAG_L4CS(x)          /* No such field */
#define GMAC_TXINFO_TX_VLAN_ACTION(x)       /* No such field */
#define GMAC_TXINFO_EFID(x)                 /* No such field */
#define GMAC_TXINFO_ENHANCE_FID(x)          /* No such field */
#define GMAC_TXINFO_VIDL(x)                 /* No such field */
#define GMAC_TXINFO_PRIO(x)                 /* No such field */
#define GMAC_TXINFO_CFI(x)                  /* No such field */
#define GMAC_TXINFO_VIDH(x)                 /* No such field */
#define GMAC_TXINFO_SB(x)                   /* No such field */
#define GMAC_TXINFO_PTP(x)                  /* No such field */

#define RX_ALL(idx)	\
	(RX_OK | RER_RUNT | RER_OVF | SW_INT\
	|((rx_multiring_bitmap[idx] & 1) ? RDU : 0)\
	|(rx_multiring_bitmap[idx] >> 1) << 11)

#ifdef CONFIG_RTL8686_SWITCH
#ifndef CONFIG_RTK_L34_ENABLE
#define RTL8686_Switch_Mode_Normal		0x00
#define RTL8686_Switch_Mode_Trap2Cpu	0x01
static unsigned int	SWITCH_MODE	= RTL8686_Switch_Mode_Normal;
#endif
#endif

//#define KERNEL_SOCKET

#ifdef CONFIG_REALTEK_HW_LSO
#define MODE_PURE_SW 1
#define MODE_HW_LSO 4
#define MODE_HW_LSO_SG 5
#define GMAC_MODE MODE_HW_LSO_SG //Choise Mode Here!!
//#define GMAC_MODE MODE_PURE_SW //Choise Mode Here!!
#define NIC_DESC_ACCELERATE_FOR_SG //tysu: many pages which have the continuious address are using the same descriptor, for accelerating.

#if	GMAC_MODE==MODE_PURE_SW
#undef HW_CHECKSUM_OFFLOAD //tysu: for hardware checksum offload.
#undef LINUX_LSO_ENABLE //tysu: enable new version kernel lso(sw & hw LSO are required)
#undef LINUX_SG_ENABLE //tysu: enable kernel SG.
#undef HW_LSO_ENABLE //tysu: enable hw lso (new or old kernel are required)

#elif	GMAC_MODE==MODE_HW_LSO
#define HW_LSO_ENABLE
#define HW_CHECKSUM_OFFLOAD
#define LINUX_LSO_ENABLE
#undef LINUX_SG_ENABLE


#elif	GMAC_MODE==MODE_HW_LSO_SG
#define HW_LSO_ENABLE
#define HW_CHECKSUM_OFFLOAD
#define LINUX_LSO_ENABLE
#define LINUX_SG_ENABLE
#endif

/* hardware minimum and maximum for a single frame's data payload */
#define CP_MIN_MTU		60	/* TODO: allow lower, but pad */

#ifdef HW_LSO_ENABLE
#define CP_LS_MTU		1500  //for LSO
#endif
#endif

//#define RE8670_MAX_ALLOC_RXSKB_NUM 3072
#if defined(CONFIG_CMCC) || defined(CONFIG_CU)
#if defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)
#define RE8670_MAX_ALLOC_RXSKB_NUM (21400+2048) // for veriwave test
#else
#define RE8670_MAX_ALLOC_RXSKB_NUM (15400+2048)//(5120*MAX_GMAC_NUM)
#endif
#elif defined(CONFIG_YUEME)
#define RE8670_MAX_ALLOC_RXSKB_NUM 11000
#else
#define RE8670_MAX_ALLOC_RXSKB_NUM 7680//(5120*MAX_GMAC_NUM)
#endif

#define RE8670_MAX_ALLOC_MC_NUM (RE8670_MAX_ALLOC_RXSKB_NUM-200)
#define RE8670_MAX_ALLOC_BC_NUM (RE8670_MAX_ALLOC_RXSKB_NUM-100)

#if defined(CONFIG_DUALBAND_CONCURRENT)
#define SKB_BUF_SIZE  1800
#else
#define SKB_BUF_SIZE  1600
#endif

#ifdef CONFIG_RG_JUMBO_FRAME
#define JUMBO_SKB_BUF_SIZE	(13312+2)		//IXIA max packet size.
#endif

#define GMAC_PRIORITY_NORMAL	0
#define GMAC_PRIORITY_MIDDLE	1
#define GMAC_PRIORITY_HIGH		2
#define GMAC_PRIORITY_CRITICAL	7

/*================================================
			GMAC used Enumeration
================================================*/
enum {
	/* NIC register offsets */
	IDR0 = 0,			/* Ethernet ID */
	IDR1 = 0x1,			/* Ethernet ID */
	IDR2 = 0x2,			/* Ethernet ID */
	IDR3 = 0x3,			/* Ethernet ID */
	IDR4 = 0x4,			/* Ethernet ID */
	IDR5 = 0x5,			/* Ethernet ID */
	MAR0 = 0x8,			/* Multicast register */
	MAR1 = 0x9,
	MAR2 = 0xa,
	MAR3 = 0xb,
	MAR4 = 0xc,
	MAR5 = 0xd,
	MAR6 = 0xe,
	MAR7 = 0xf,
	TXOKCNT=0x10,
	RXOKCNT=0x12,
	TXERR = 0x14,
	RXERRR = 0x16,
	MISSPKT = 0x18,
	FAE=0x1A,
	TX1COL = 0x1c,
	TXMCOL=0x1e,
	RXOKPHY=0x20,
	RXOKBRD=0x22,
	RXOKMUL=0x24,
	TXABT=0x26,
	TXUNDRN=0x28,
	RDUMISSPKT=0x2a,
	TRSR=0x34,
	COM_REG=0x38,
	CMD=0x3B,
	IMR=0x3C,
	ISR=0x3E,
	TCR=0x40,
	RCR=0x44,
	CPUtagCR=0x48,
	CONFIG_REG=0x4C,	
	CPUtag1CR=0x50,
	MSR=0x58,
	MIIAR=0x5C,
	SWINT_REG=0x60,
	VLAN_REG=0x64,
	VLAN1_REG=0x68,
	LEDCR=0x70,
	IMR0=0xd0,
	IMR1=0xd4,
	ISR1=0xd8,
	INTR_REG=0xdc,
	TxFDP1=0x1300,
	TxCDO1=0x1304,
    TxFDP2=0x1310, 
	TxCDO2=0x1314, 
	TxFDP3=0x1320, 
	TxCDO3=0x1324, 
	TxFDP4=0x1330, 
	TxCDO4=0x1334,
	TxFDP5=0x1340,
	TxCDO5=0x1344,
	RRING_ROUTING1=0x1370,
	RRING_ROUTING2=0x1374,
	RRING_ROUTING3=0x1378,
	RRING_ROUTING4=0x137c,
	RRING_ROUTING5=0x1380,
	RRING_ROUTING6=0x1384,
	RRING_ROUTING7=0x1388,
	RxFDP=0x13F0,
	RxCDO=0x13F4,	
	RxRingSize=0x13F6,
	SMSA=0x13FC,
	EthrntRxCPU_Des_Num=0x1430,
	EthrntRxCPU_Des_Wrap =	0x1431,
	Rx_Pse_Des_Thres = 	0x1432,	
	RxRingSize_h=0x13F7, 
	EthrntRxCPU_Des_Num_h=0x1433,
	EthrntRxCPU_Des_Wrap_h =0x1433,
	Rx_Pse_Des_Thres_h =0x142c,

	RST = (1<<0),
	RxChkSum = (1<<1),
	RxVLAN_Detag = (1<<2),
	RxJumboSupport = (1<<3),
	IO_CMD = 0x1434,
	IO_CMD1 = 0x1438,  // RLE0371 and other_platform set different value.
	RX_IntNum_Shift = 0x8,             /// ????
	TX_OWN = (1<<5),
	RX_OWN = (1<<4),
	MII_RE = (1<<3),
	MII_TE = (1<<2),
	TX_FNL = (1<<1),
	TX_FNH = (1),
	/*TX_START= MII_RE | MII_TE | TX_FNL | TX_FNH,
	TX_START = 0x8113c,*/
	RXMII = MII_RE,
	TXMII = MII_TE,
	LSO_F = (1<<28),
	EN_1GB = (1<<24),

	Rff_size_sel_2k = (0x2 << 28),
	Int_route_r4r5r6t2t4 = (0x00440540),
	Int_route_enable = (1<<25),
	En_int_split = (0x01 << 24),
		
	RxFDP2=0x1390,
	RxCDO2=0x1394,
	RxRingSize2=0x1396,
	RxRingSize_h2=0x1397,
	EthrntRxCPU_Des_Num2=0x1398,
	EthrntRxCPU_Des_Wrap2 =	0x139c,
	DIAG1_REG=0x1404,
};

enum RE8670_IOCMD_REG
{

	RX_MIT 			= 7,
	RX_TIMER 		= 1,
	RX_FIFO 		= 2,
	TX_FIFO			= 1,
	TX_MIT			= 7,
	TX_POLL4		= (1 << 3),
	TX_POLL3		= (1 << 2),
	TX_POLL2		= (1 << 1),
	TX_POLL			= (1 << 0),
};

enum RE8670_IOCMD1_REG
{
	TX_RR_scheduler	= (1 << 14),
	TX_POLL5		= (1 << 8),
	txq5_h			= (1 << 4),
	txq4_h			= (1 << 3),
	txq3_h			= (1 << 2),
	txq2_h			= (1 << 1),
	txq1_h			= (1 << 0),
};

enum{
	TXD_VLAN_INTACT,
	TXD_VLAN_INSERT,
	TXD_VLAN_REMOVE,
	TXD_VLAN_REMARKING,
};

enum {
	RE8670_RX_STOP=0,
	RE8670_RX_CONTINUE,
	RE8670_RX_STOP_SKBNOFREE,
	RE8670_RX_END
};

/*
*   | callback                      	| priority		| return
*   | re8670_rx_skb                 	| 0			| STOP
*   | fwdEngine_rx_skb      		| 1			| STOP
*   | rtk_gpon_omci_rx_wrapper	| 2			| STOP
*   | re8686_dump_rx			| 6			| IS CONTINUE
*   | re8686_rx_patch			| 7			| IS CONTINUE
*/

typedef enum {
	RE8686_RXPRI_DEFAULT=0,
	RE8686_RXPRI_RG,
	RE8686_RXPRI_L34LITE,
	RE8686_RXPRI_VOIP,
	RE8686_RXPRI_OMCI,
	RE8686_RXPRI_OAM,
	RE8686_RXPRI_MPCP,
	RE8686_RXPRI_MUTICAST,
	RE8686_RXPRI_DUMP,
	RE8686_RXPRI_PATCH,
}RE8686_RX_PRI_T;

enum PHY_REGS{
	FORCE_TX = (1<<7),
	RXFCE = (1<<6),
	TXFCE = (1<<5),
	SP1000 = (1<<4),
	SP10 = (1<<3),
	LINK = (1<<2),
	TXPF = (1<<1),
	RXPF = (1<<0),
};

enum RE8670_STATUS_REGS
{
	/*TX/RX share */
	DescOwn 	= (1 << 31), /* Descriptor is owned by NIC */
	RingEnd 	= (1 << 30), /* End of descriptor ring */
	FirstFrag 	= (1 << 29), /* First segment of a packet */
	LastFrag 	= (1 << 28), /* Final segment of a packet */

	/*Tx descriptor opt1*/
	IPCS		= (1 << 27),
	L4CS		= (1 << 26),
	KEEP		= (1 << 25),
	BLU			= (1 << 24),
	TxCRC		= (1 << 23),
	VSEL		= (1 << 22),
	DisLrn		= (1 << 21),
	CPUTag_ipcs = (1 << 20),
	CPUTag_l4cs	= (1 << 19),

	/*Tx descriptor opt2*/
	CPUTag		= (1 << 31),
	aspri		= (1 << 30),
	CPRI		= (1 << 27),
	TxVLAN_int	= (0 << 25),  //intact
	TxVLAN_ins	= (1 << 25),  //insert
	TxVLAN_rm	= (2 << 25),  //remove
	TxVLAN_re	= (3 << 25),  //remark
	//TxPPPoEAct	= (1 << 23),
	TxPPPoEAct	= 23,
	//TxPPPoEIdx	= (1 << 20),
	TxPPPoEIdx	= 20,
	Efid			= (1 << 19),
	//Enhan_Fid	= (1 << 16),
	Enhan_Fid 	= 16,
	/*Tx descriptor opt3*/
	SrcExtPort	= 29,
	TxDesPortM	= 23,
	TxDesStrID 	= 16,
	TxDesVCM	= 0,
	/*Tx descriptor opt4*/
	/*Rx descriptor  opt1*/
	CRCErr		= (1 << 27),
	IPV4CSF		= (1 << 26),
	L4CSF		= (1 << 25),
	RCDF		= (1 << 24),
	IP_FRAG		= (1 << 23),
	PPPoE_tag	= (1 << 22),
	RWT			= (1 << 21),
	PktType		= (1 << 17),
	RxProtoIP	= 1,
	RxProtoPPTP	= 2,
	RxProtoICMP	= 3,
	RxProtoIGMP	= 4,
	RxProtoTCP	= 5,   
	RxProtoUDP	= 6,
	RxProtoIPv6	= 7,
	RxProtoICMPv6	= 8,
	RxProtoTCPv6	= 9,
	RxProtoUDPv6	= 10,
	L3route		= (1 << 16),
	OrigFormat	= (1 << 15),
	PCTRL		= (1 << 14),
	/*Rx descriptor opt2*/
	PTPinCPU	= (1 << 30),
	SVlanTag		= (1 << 29),
	/*Rx descriptor opt3*/
	SrcPort		= (1 << 27),
	DesPortM	= (1 << 21),
	Reason		= (1 << 13),
	IntPriority	= (1 << 10),
	ExtPortTTL	= (1 << 5),
};

enum RE8670_THRESHOLD_REGS{
	//shlee	THVAL		= 2,
	TH_ON_VAL = 0x10,	//shlee flow control assert threshold: available desc <= 16
	TH_OFF_VAL= 0x30,	//shlee flow control de-assert threshold : available desc>=48
	//	RINGSIZE	= 0x0f,	//shlee 	2,
	LOOPBACK	= (0x3 << 8),
	AcceptErr	= 0x20,	     /* Accept packets with CRC errors */
	AcceptRunt	= 0x10,	     /* Accept runt (<64 bytes) packets */
	AcceptBroadcast	= 0x08,	     /* Accept broadcast packets */
	AcceptMulticast	= 0x04,	     /* Accept multicast packets */
	AcceptMyPhys	= 0x02,	     /* Accept pkts with our MAC as dest */
	AcceptAllPhys	= 0x01,	     /* Accept all pkts w/ physical dest */
	AcceptAll = AcceptBroadcast | AcceptMulticast | AcceptMyPhys |  AcceptAllPhys | AcceptErr | AcceptRunt,
	AcceptNoBroad = AcceptMulticast |AcceptMyPhys |  AcceptAllPhys | AcceptErr | AcceptRunt,
	AcceptNoMulti =  AcceptMyPhys |  AcceptAllPhys | AcceptErr | AcceptRunt,
	NoErrAccept = AcceptBroadcast | AcceptMulticast | AcceptMyPhys,
	NoErrPromiscAccept = AcceptBroadcast | AcceptMulticast | AcceptMyPhys |  AcceptAllPhys,
};

enum RE8670_ISR_REGS{
	SW_INT 		= (1 <<10),
	TDU			= (1 << 9),
	LINK_CHG	= (1 <<	8),
	TER			= (1 << 7),
	TOK			= (1 << 6),
	RER_OVF		= (1 << 4),
	RER_RUNT	= (1 << 2),
	RX_OK		= (1 << 0),
	RDU_ALL 	= (RDU | RDU2 | RDU3 | RDU4 | RDU5 | RDU6)
};

enum RTL8672GMAC_CPUtag_Control
{
	CTEN_RX     = (1<<31),
	CT_TSIZE	= 27,
	CT_DSLRN	= (1 << 24),
	CT_NORMK	= (1 << 23),
	CT_ASPRI	= (1 << 22),
	CT_APPLO_PRO	= (8 << 18),

	CT_RSIZE_H = 25,
	CT_RSIZE_L = 16,
	CTPM_8306   = (0xf0 << 8),
	CTPM_8368   = (0xe0 << 8),
	CTPM_8370   = (0xff << 8),
	CTPM_8307   = (0xff << 8),
	CTPV_8306   = 0x90,
	CTPV_8368   = 0xa0,
	CTPV_8370   = 0x04,
	CTPV_8307	  = 0x04,
};

enum RTL8672GMAC_CPUtag1_Control
{
	CT1_SID     = (64<<8)

};

enum RTL8672GMAC_PG_REG
{
	EN_PGLBK     = (1<<15),
	DATA_SEL     = (1<<14),
	LEN_SEL      = (1<<11),
	NUM_SEL      = (1<<10),
};

typedef enum
{
	FLAG_WRITE		= (1<<31),
	FLAG_READ		= (0<<31),

	MII_PHY_ADDR_SHIFT	= 26, 
	MII_REG_ADDR_SHIFT	= 16,
	MII_DATA_SHIFT		= 0,
}MIIAR_MASK;

enum RTL8686GMAC_DEBUG_LEVEL{
	RTL8686_PRINT_NOTHING = 0,
	RTL8686_SKB_RX = (1<<0),
	RTL8686_SKB_TX = (1<<1),
	RTL8686_RXINFO = (1<<2),
	RTL8686_TXINFO = (1<<3),
	RTL8686_RX_TRACE = (1<<4),
	RTL8686_TX_TRACE = (1<<5),	
	RTL8686_RX_WARN = (1<<6),	
	RTL8686_TX_WARN = (1<<7),	
};

enum RTL8686_IFFLAG{
	RTL8686_ELAN = 0,
	RTL8686_WAN = 1,
	RTL8686_WLAN = 2,
	RTL8686_SVAP = 3,		// Luke: slave VAP interface, should not be sent by bridge
};


/*================================================
			GMAC used Structure
================================================*/
struct rx_info{
	union{
		struct{
			u32 own:1;//31
			u32 eor:1;//30
			u32 fs:1;//29
			u32 ls:1;//28
			u32 crcerr:1;//27
			u32 ipv4csf:1;//26
			u32 l4csf:1;//25
			u32 rcdf:1;//24
			u32 ipfrag:1;//23
			u32 pppoetag:1;//22
			u32 rwt:1;//21
			u32 rsvd1:7;//14~17
#ifdef CONFIG_RG_JUMBO_FRAME
			u32 data_length:14;//0~13
#else
			u32 rsvd2:2;//12~13
			u32 data_length:12;//0~11
#endif
		}bit;
		u32 dw;//double word
	}opts1;
	u32 addr;
	union{
		struct{
			u32 cputag:1;//31
			u32 ptp_in_cpu_tag_exist:1;//30
			u32 svlan_tag_exist:1;//29
			u32 reason:8;//21~28
			u32 rsvd_1:4;//17~20
			u32 ctagva:1;//16
			u32 cvlan_tag:16;//0~15
		}bit;
		u32 dw;//double word
	}opts2;
	union{
		struct{
			u32 internal_priority:3;//29~31
			u32 pon_sid_or_extspa:7;//22~28 or 26~28
			u32 l3routing:1;//21
			u32 origformat:1;//20
			u32 src_port_num:4;//16~19
			u32 fbi:1;//15
			u32 fb_hash_or_dst_portmsk:15;//0~14 or 0~6
		}bit;
		struct{
			u32 internal_priority:3;//29~31
			u32 extspa:3;//26~28
			u32 rsvd_1:4;//22~25
			u32 l3routing:1;//21
			u32 origformat:1;//20
			u32 src_port_num:4;//16~19
			u32 fbi:1;//15
			u32 fb_hash_or_dst_portmsk:15;//0~14 or 0~6
		}bit1;
		u32 dw;//double word
	}opts3;
};

struct tx_info{
	union{
		struct{
			u32 own:1;//31
			u32 eor:1;//30
			u32 fs:1;//29
			u32 ls:1;//28
			u32 ipcs:1;//27
			u32 l4cs:1;//26
			u32 tpid_sel:1;//25
			u32 stag_aware:1;//24		
			u32 crc:1;//23
			u32 rsvd:6;//17~22
			u32 data_length:17;//0~16
		}bit;
		u32 dw;//double word
	}opts1;
	u32 addr;
	union{
		struct{
			u32 cputag:1;//31
			u32 tx_svlan_action:2;//29~30
			u32 tx_cvlan_action:2;//27~28
			u32 tx_portmask:11;//16~26
			u32 cvlan_vidl:8;//8~15
			u32 cvlan_prio:3;//5~7
			u32 cvlan_cfi:1;// 4
			u32 cvlan_vidh:4;//0~3
		}bit;
		u32 dw;//double word
	}opts2;
	union{
		struct{
			u32 rsvd1:4;//28~31
			u32 aspri:1;//27
			u32 cputag_pri:3;//24~26
			u32 keep:1;//23
			u32 rsvd2:1;//22
			u32 dislrn:1;//21
			u32 cputag_psel:1;//20
			u32 rsvd3:2;//18~19
			u32 l34_keep:1;//17
			u32 gmac_id:1;//16  //software used for gmac_tx_idx(0:gmac9, 1:gmac10)
			u32 extspa:3;//13~15
			u32 tx_pppoe_action:2;//11~12
			u32 tx_pppoe_idx:4;//7~10
			u32 tx_dst_stream_id:7;//0~6
		}bit;
		u32 dw;//double word
	}opts3;
	union{
		struct{
			u32 lgsen:1;//31
			u32 lgmtu:11;//20~30
			u32 rsvd:4;//16~19
			u32 svlan_vidl:8;//8~15
			u32 svlan_prio:3;//5~7
			u32 svlan_cfi:1;// 4
			u32 svlan_vidh:4;//0~3
		}bit;
		u32 dw;//double word
	}opts4;
};

typedef struct dma_tx_desc {
	u32		opts1;
	u32		addr;
	u32		opts2;
	u32		opts3; //cputag
	u32		opts4; //lso
}DMA_TX_DESC;

typedef struct dma_rx_desc {
	u32		opts1;
	u32		addr;
	u32		opts2;
	u32		opts3;
}DMA_RX_DESC;

struct cp_extra_stats {
	unsigned long rx_frags;
	unsigned long tx_timeouts;
	//krammer add for rx info
	unsigned int rx_hw_num;
	unsigned int rx_sw_num;
	unsigned int rer_runt;
	unsigned int rer_ovf;
	unsigned int rdu;
	unsigned int frag;
#ifdef CONFIG_RG_JUMBO_FRAME
	unsigned int toobig;
#endif
	unsigned int crcerr;
	unsigned int rcdf;
	unsigned int rx_no_mem;
	//krammer add for tx info
	unsigned int tx_sw_num;
	unsigned int tx_hw_num;
	unsigned int tx_no_desc;
	unsigned int rx_critical_num;
	unsigned int rx_critical_drop_num;
};

struct re_private {
	unsigned		gmac;
	void			*regs;
	struct net_device	*dev;
	spinlock_t		tx_lock;
	spinlock_t		rx_lock;

    DMA_RX_DESC     *rx_Mring[MAX_RXRING_NUM];
    unsigned		rx_Mtail[MAX_RXRING_NUM];
    char*			rxdesc_Mbuf[MAX_RXRING_NUM];

    DMA_TX_DESC		*tx_Mhqring[MAX_TXRING_NUM];
	char*			txdesc_Mbuf[MAX_TXRING_NUM];
	unsigned		tx_Mhqhead[MAX_TXRING_NUM];
	unsigned		tx_Mhqtail[MAX_TXRING_NUM];

	struct ring_info	*tx_skb[MAX_TXRING_NUM];
	struct ring_info	*rx_skb[MAX_RXRING_NUM];

	#ifdef CONFIG_DUALBAND_CONCURRENT
	DMA_RX_DESC     	*default_rx_desc;
	char				*default_rxdesc_Mbuf;
	struct ring_info	*default_rx_skb;
	unsigned 		old_tx_Mhqhead[MAX_TXRING_NUM];
	unsigned 		old_tx_Mhqtail[MAX_TXRING_NUM];
	unsigned		old_rx_Mtail[MAX_RXRING_NUM];
	#endif

	
	unsigned		rx_buf_sz;
	dma_addr_t		ring_dma;
	u32			msg_enable;

	struct cp_extra_stats	cp_stats;
	u32 isr_status;
	u32 isr1_status;

	struct pci_dev		*pdev;
	u32			rx_config;

	struct sk_buff		*frag_skb;
	unsigned		dropping_frag : 1;

	//struct tq_struct	rx_task;
	//struct tq_struct	tx_task;
	struct tasklet_struct rx_tasklets;
	//struct tasklet_struct tx_tasklets;

#if 1	
	struct tasklet_struct tx_tasklets; 
#endif	
	struct net_device* port2dev[SW_PORT_NUM];
	int (*port2rxfunc[SW_PORT_NUM])(struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo);

#ifdef CONFIG_RG_SIMPLE_PROTOCOL_STACK
	struct net_device* multiWanDev[8];		//at most 7 WAN interface(reserved one interface for WAN)
	int wanInterfaceIdx;				//used for multi-WAN in SPS
#endif

#ifdef CONFIG_RG_JUMBO_FRAME
	struct sk_buff *jumboFrame;
	unsigned short jumboLength;
#endif
	unsigned int stag_pid;
	unsigned int stag_pid1;
};

struct rtl8686_dev_table_entry {
	unsigned char 			ifname[IFNAMSIZ];
	unsigned char			ifflag; //0:ELAN, 1:WAN, 2:WLAN
	unsigned short			vid;
	unsigned int			phyPort;
	unsigned char 			isPanelPort;
	struct net_device *dev_instant;
};

/*================================================
			Function Prototype
================================================*/
//must be same with port2rxfunc define in cp
typedef int (*p2rfunc_t)(struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo);
int re8686_register_rxfunc_all_port(p2rfunc_t pfunc);
int re8686_send_with_txInfo(struct sk_buff *skb, struct tx_info* ptxInfo, int ring_num);
int re8686_send_with_txInfo_and_mask(struct sk_buff * skb,struct tx_info * ptxInfo,int ring_num,struct tx_info * ptxInfoMask);
void re8686_reset_rxfunc_to_default(unsigned int gmac);
int re8670_rx_skb (struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo);
int re8686_set_vlan_register(struct tx_info* ptxInfo, unsigned char reg_num, unsigned int value);
int re8686_get_vlan_register(struct tx_info* ptxInfo, unsigned char reg_num, unsigned int *value_p);
int re8686_set_flow_control(unsigned int gmac, unsigned int ring, unsigned char enable);
void re8686_set_pauseBySw(unsigned char enable);
struct sk_buff *re8670_getAlloc(unsigned int size);
int re8670_host_speedup (struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo);

struct re_dev_private {
	struct re_private* pCp;
	struct net_device_stats net_stats;
	u32 txPortMask;
};

#endif /*_RE8686_RTL9607C_H_*/

