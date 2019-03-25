///--#include <linux/config.h>
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
#include "re_privskb.h"
#include <bspchip.h>

#if defined(CONFIG_RTL9607C)
#include "re8686_rtl9607c.h"
#else
#if defined(CONFIG_RTL9600_SERIES)
#include "re8686.h"
#endif
#if defined(CONFIG_RTL9601B_SERIES)
#include "re8686_rtl9601b.h"
#endif
#if defined(CONFIG_RTL9602C_SERIES)
#include "re8686_rtl9602c.h"
#endif
#if defined(CONFIG_RTL9607B_SERIES)
#include "re8686_rtl9607b.h"
#endif
#endif

#undef __IRAM_NIC
#define __IRAM_NIC __attribute__ ((__section__(".iram-fwd")))

typedef struct rx_extInfo {
	struct rx_info rxInfo;
	unsigned ring_num;
}rx_extInfo_t;

static inline void kick_tx(int ring_num);

struct tx_info gTxInfo;
rx_extInfo_t gRxInfo;
static int isMacChange = 0;
static int stopRx = 0;
static unsigned int stopTx = 2;
static int isRxRingRoute = 0;
static unsigned int ring1Route = 0;
static int gTxRing=0;
static int isLsoPkt=0;
static int isPktDbg=0;
static int isRxCrcDbg=0;
static struct sk_buff *jumboFrame = NULL;
static unsigned short jumboLength = 0;
static int lgMtu = 1500;
static int isRxEor = 0;
static int isPktWithRtkCpuTag = 0;
static int isSamDbgOn = 0;
static int SamDbgLevel = 7;

#define JUMBO_SKB_BUF_SIZE	(13312+2)
#define SKB_BUF_SIZE 1600
#define RX_OFFSET 2

#define TXINFO_SET(sel,field,value) (gTxInfo.sel.bit.field = value)

#define DEV2CP(dev)  (((struct re_dev_private*)dev->priv)->pCp)
#define DEVPRIV(dev)  ((struct re_dev_private*)dev->priv)
#define NEXT_RX(N,RING_SIZE)		(((N) + 1) & (RING_SIZE - 1))
#define NEXT_TX(N,RING_SIZE)		(((N) + 1) & (RING_SIZE - 1))

unsigned int iocmd_reg=CMD_CONFIG;
unsigned int iocmd1_reg=CMD1_CONFIG | (RX_NOT_ONLY_RING1<<25);   


static const int RE8670_RX_MRING_SIZE[MAX_RXRING_NUM] = {
RE8670_RX_RING1_SIZE,
RE8670_RX_RING2_SIZE,
RE8670_RX_RING3_SIZE,
RE8670_RX_RING4_SIZE,
RE8670_RX_RING5_SIZE,
RE8670_RX_RING6_SIZE
};

static const int RE8670_TX_MRING_SIZE[MAX_TXRING_NUM] = {
RE8670_TX_RING1_SIZE,
RE8670_TX_RING2_SIZE,
RE8670_TX_RING3_SIZE,
RE8670_TX_RING4_SIZE,
RE8670_TX_RING5_SIZE
};

static int tx_dump_flag = 0;

#define SAMDBG_PRINT(level, fmt, args...)	do{if(isSamDbgOn && SamDbgLevel>=level){printk(" SAMDBG> LEVEL=%d %s %d, "fmt"\n", level, __func__, __LINE__, args);}}while(0)

#define TX_HQBUFFS_AVAIL(CP,ring_num)					\
		(((CP)->tx_Mhqtail[ring_num] - (CP)->tx_Mhqhead[ring_num] + RE8670_TX_MRING_SIZE[ring_num] - 1)&(RE8670_TX_MRING_SIZE[ring_num] - 1))		


#define multi_ring_set()  \
if(isRxRingRoute){ \
	RTL_W32(IMR0,0x3f); \
	RTL_W32(RRING_ROUTING1, ring1Route); \
}else \
{ \	
	RTL_W32(IMR0,0x1); \
	RTL_W32(RRING_ROUTING1, ring1Route); \
}

enum RE8670_STATUS_REGS
{
	/*TX/RX share */
	DescOwn		 = (1 << 31), /* Descriptor is owned by NIC */
	RingEnd		 = (1 << 30), /* End of descriptor ring */
	FirstFrag	 = (1 << 29), /* First segment of a packet */
	LastFrag	 = (1 << 28), /* Final segment of a packet */

	/*Tx descriptor opt1*/
	IPCS		 = (1 << 27),
	L4CS		 = (1 << 26),
	TPID_SEL     = (1 << 25),
	STAG_AWARE   = (1 << 24),
	TxCRC		 = (1 << 23),

	/*Tx descriptor opt2*/
	TxCPUTag	 = (1 << 31),
	TxSVLAN_int	 = (0 << 27),  //intact
	TxSVLAN_ins	 = (1 << 27),  //insert
	TxSVLAN_rm	 = (2 << 27),  //remove
	TxSVLAN_re	 = (3 << 27),  //remark
	TxCVLAN_int	 = (0 << 25),  //intact
	TxCVLAN_ins	 = (1 << 25),  //insert
	TxCVLAN_rm	 = (2 << 25),  //remove
	TxCVLAN_re	 = (3 << 25),  //remark

	/*Tx descriptor opt3*/
	aspri		 = (1 << 27),
	PRI		     = 24,
	KEEP	     = (1 << 23),
	DisLrn       = (1 << 21),
	PSEL         = (1 << 20),
	l34_keep     = (1 << 17),
	SrcExtPort	 = 13,
	TxPPPoEAct	 = 11,
	TxPPPoEIdx   = 7,
	
	/*Tx descriptor opt4*/
	LGSEN        = (1 << 31),
	LGMTU        = 20,
	
	/*Rx descriptor  opt1*/
	CRCErr	     = (1 << 27),
	IPV4CSF		 = (1 << 26),
	L4CSF		 = (1 << 25),
	RCDF		 = (1 << 24),
	IP_FRAG		 = (1 << 23),
	PPPoE_tag	 = (1 << 22),
	RWT			 = (1 << 21),
	DataLen		 = 0,

	/*Rx descriptor opt2*/
	RxCPUTag     = (1 << 31),
	PTPinCPU	 = (1 << 30),
	SVlanTag	 = (1 << 29),
	Reason       = 21,
	CTAVA        = (1 << 16),
	CVLAN_VIDL   = 8,
	CVLAN_Prio   = 5,
	CVLAN_CFI    = 4,
	CVLAN_VIDH   = 0,
	/*Rx descriptor opt3*/
	IntPriority	 = 29,
	PON_SID      = 22,
	ExtSPA       = 26,
	L3route		 = (1 << 21),
	OrigFormat	 = (1 << 20),
	SrcPort		 = 16,
	FBI          = (1 << 15),
	FBI_hash_idx = 0,
	DesPortM	 = 0
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
	NoErrPromiscAccept = AcceptBroadcast | AcceptMulticast | AcceptMyPhys |  AcceptAllPhys
};

enum RE8670_IMR_REGS
{
	IMR_RDU6msk      = (1 << 31),
	IMR_RDU5msk      = (1 << 30),
	IMR_RDU4msk      = (1 << 29),
	IMR_RDU3msk      = (1 << 28),
	IMR_RDU2msk      = (1 << 27),
	IMR_SWIntmsk     = (1 << 26),
	IMR_TDUmsk       = (1 << 25),
	IMR_LinkChgmsk   = (1 << 24),
	IMR_TERmsk       = (1 << 23),
	IMR_TOKmsk       = (1 << 22),	
	IMR_RDUmsk       = (1 << 21),
	IMR_RER_OVFmsk   = (1 << 20),
	IMR_RER_RUNTmsk  = (1 << 18),
	IMR_CNT_WRAPmsk  = (1 << 17),
	IMR_ROKmsk       = (1 << 16),
	IMR_RDU6    	 = (1 << 15),
	IMR_RDU5    	 = (1 << 14),
	IMR_RDU4    	 = (1 << 13),
	IMR_RDU3    	 = (1 << 12),
	IMR_RDU2    	 = (1 << 11),
	IMR_SWInt   	 = (1 << 10),
	IMR_TDU     	 = (1 << 9),
	IMR_LinkChg 	 = (1 << 8),
	IMR_TER     	 = (1 << 7),
	IMR_TOK     	 = (1 << 6),	
	IMR_RDU     	 = (1 << 5),
	IMR_RER_OVF 	 = (1 << 4),
	IMR_RER_RUNT	 = (1 << 2),
	IMR_CNT_WRAP	 = (1 << 1),
	IMR_RX_OK     	 = (1 << 0),	
	IMR_RDU_ALL      = (IMR_RDU | IMR_RDU2 | IMR_RDU3 | IMR_RDU4 | IMR_RDU5 | IMR_RDU6),
	IMR_RX_ALL       = (IMR_RX_OK | IMR_RER_RUNT | IMR_RER_OVF |
					((RX_MULTIRING_BITMAP & 1) ? IMR_RDU : 0) |
					(RX_MULTIRING_BITMAP >> 1) << 11)
};

enum RE8670_ISR1_REGS{
	ISR1_TDU5     	 = (1 << 28),
	ISR1_TDU4     	 = (1 << 27),
	ISR1_TDU3      	 = (1 << 26),
	ISR1_TDU2    	 = (1 << 25),
	ISR1_TDU1    	 = (1 << 24),
	ISR1_TOK5		 = (1 << 20),
	ISR1_TOK4		 = (1 << 19),
	ISR1_TOK3		 = (1 << 18),
	ISR1_TOK2		 = (1 << 17),
	ISR1_TOK1		 = (1 << 16),
	ISR1_RX_OK6	 	 = (1 << 5),
	ISR1_RX_OK5	 	 = (1 << 4),
	ISR1_RX_OK4	 	 = (1 << 3),
	ISR1_RX_OK3	 	 = (1 << 2),
	ISR1_RX_OK2	 	 = (1 << 1),
	ISR1_RX_OK1	   	 = (1 << 0)
};

enum RTL8672GMAC_CPUtag_Control
{
	CTEN_RX      = (1<<31),
	CT_TSIZE	 = 27,
	CT_DSLRN	 = (1 << 24),
	CT_NORMK	 = (1 << 23),
	CT_ASPRI	 = (1 << 22),
	CT_0787      = (8 << 18),
	CT_RSIZE_H   = 25,
	CT_RSIZE_L   = 16,
	CTPM_8306    = (0xf0 << 8),
	CTPM_8368    = (0xe0 << 8),
	CTPM_8370    = (0xff << 8),
	CTPM_8307    = (0xff << 8),
	CTPV_8306    = 0x90,
	CTPV_8368    = 0xa0,
	CTPV_8370    = 0x04,
	CTPV_8307	 = 0x04
};

enum RTL8672GMAC_PG_REG
{
	EN_PGLBK     = (1<<15),
	DATA_SEL     = (1<<14),
	LEN_SEL      = (1<<11),
	NUM_SEL      = (1<<10)
};

struct ring_info {
	struct sk_buff		*skb;
	dma_addr_t			mapping;
	unsigned			frag;
};


__IRAM_NIC
struct sk_buff *re8670_getAlloc(unsigned int size)
{	
	struct sk_buff *skb=NULL;

#ifdef CONFIG_RTL865X_ETH_PRIV_SKB  //For RTL8192CE
	skb = dev_alloc_skb_priv_eth(CROSS_LAN_MBUF_LEN);
#else
	skb = dev_alloc_skb(size);
#endif
	
	return (skb);
}

static inline void unmask_rx_int(void){
	RTL_R16(IMR)|=(u16)(IMR_RX_ALL);//we still open imr when rx_work==0 for a quickly schedule	
	multi_ring_set();
}


static inline void retriveRxInfo(DMA_RX_DESC *desc, struct rx_info *pRxInfo){
	pRxInfo->opts1.dw = desc->opts1;
	pRxInfo->addr = desc->addr;
	pRxInfo->opts2.dw = desc->opts2;
	pRxInfo->opts3.dw = desc->opts3;
}


static inline void updateGmacFlowControl(unsigned rx_tail,int ring_num){
	unsigned int new_cpu_desc_num;		

	if(ring_num==0)
	{    
		new_cpu_desc_num = RTL_R32(EthrntRxCPU_Des_Num);
		new_cpu_desc_num &= 0x00FFFF0F;//clear
		new_cpu_desc_num |= (((rx_tail&0xFF)<<24)|(((rx_tail>>8)&0xF)<<4));//update
		RTL_R32(EthrntRxCPU_Des_Num) = new_cpu_desc_num;
	}
	else
	{
		new_cpu_desc_num = RTL_R32(EthrntRxCPU_Des_Num2+ADDR_OFFSET*(ring_num-1));
		new_cpu_desc_num &= 0xfffff000;//clear
		new_cpu_desc_num |= (((rx_tail&0xFF)|(((rx_tail>>8)&0xF)<<4)));//update
		RTL_W32(EthrntRxCPU_Des_Num2+ADDR_OFFSET*(ring_num-1), new_cpu_desc_num);
	}
}


void tx_dump(struct tx_info txInfo)
{
	/*opt1*/
	printk("own: %d\n",txInfo.opts1.bit.own); // 31
	printk("eor: %d\n",txInfo.opts1.bit.eor); // 30
	printk("fs: %d\n",txInfo.opts1.bit.fs); // 29
	printk("ls: %d\n",txInfo.opts1.bit.ls); // 28
	printk("ipcs: %d\n",txInfo.opts1.bit.ipcs); // 27
	printk("l4cs: %d\n",txInfo.opts1.bit.l4cs); // 26
	printk("tpid_sel: %d\n",txInfo.opts1.bit.tpid_sel); // 25
	printk("stag_aware: %d\n",txInfo.opts1.bit.stag_aware); // 24
	printk("crc: %d\n",txInfo.opts1.bit.crc); // 23
	printk("data_length: %d\n",txInfo.opts1.bit.data_length); // 0~16	
	printk("addr:%x\n",txInfo.addr); // 0~31

	/*opt2*/
	printk("cputag: %d\n",txInfo.opts2.bit.cputag); // 31	
	printk("tx_svlan_action: %d\n",txInfo.opts2.bit.tx_svlan_action); // 29~30
	printk("tx_cvlan_action: %d\n",txInfo.opts2.bit.tx_cvlan_action); // 27~28
	printk("tx_portmask: 0x%x\n",txInfo.opts2.bit.tx_portmask); // 16~26
	printk("cvlan_vidl: %d\n",txInfo.opts2.bit.cvlan_vidl); // 12~15
	printk("cvlan_prio: %d\n",txInfo.opts2.bit.cvlan_prio); // 8~11
	printk("cvlan_vidh: %d\n",txInfo.opts2.bit.cvlan_vidh); // 0~3

	/*opt3*/
	printk("aspri: %d\n",txInfo.opts3.bit.aspri); // 27	
	printk("cputag_pri: %d\n",txInfo.opts3.bit.cputag_pri); // 24~26
	printk("keep: %d\n",txInfo.opts3.bit.keep); // 23
	printk("dislrn: %d\n",txInfo.opts3.bit.dislrn); // 21
	printk("cputag_psel: %d\n",txInfo.opts3.bit.cputag_psel); // 20	
	printk("l34_keep: %d\n",txInfo.opts3.bit.l34_keep); // 17
	printk("extspa: %d\n",txInfo.opts3.bit.extspa); // 13~15
	printk("tx_pppoe_action: %d\n",txInfo.opts3.bit.tx_pppoe_action); // 11~12
	printk("tx_pppoe_idx: %d\n",txInfo.opts3.bit.tx_pppoe_idx);	 // 7~10
	printk("tx_dst_stream_id: %d\n",txInfo.opts3.bit.tx_dst_stream_id); // 0~6

	/*opt4*/
	printk("lgsen: %d\n",txInfo.opts4.bit.lgsen); // 31	
	printk("lgmtu: %d\n",txInfo.opts4.bit.lgmtu); // 20~30
	printk("svlan_vidl: %d\n",txInfo.opts4.bit.svlan_vidl); // 12~15
	printk("svlan_prio: %d\n",txInfo.opts4.bit.svlan_prio);	// 8~11
	printk("svlan_vidh: %d\n",txInfo.opts4.bit.svlan_vidh); // 0~3

	/*private*/
	printk("isMacChange: %d\n",isMacChange);
	printk("tx_dump_flag: %d\n",tx_dump_flag);
	printk("stopRx: %d\n",stopRx);
	printk("stopTx: %d \n", stopTx);
	printk("isRxRingRoute: %d\n",isRxRingRoute);
	printk("ring1Route: 0x%x\n",ring1Route);
	printk("gTxRing: %d\n",gTxRing);
	printk("isPktDbg: %d\n",isPktDbg);
	printk("isRxCrcDbg: %d\n", isRxCrcDbg);
	printk("isLsoPkt: %d, %d\n", isLsoPkt, txInfo.opts4.bit.lgsen);
	printk("lgMtu: %d, %d \n", lgMtu, txInfo.opts4.bit.lgmtu);
	printk("isRxEor: %u\n", isRxEor);
	printk("isSamDbgOn: %d\n",isSamDbgOn);
	printk("SamDbgLevel: %d\n",SamDbgLevel);
	
}


int tx_dump_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	tx_dump(gTxInfo);
	return (0);
}

int tx_dump_write(struct file *file, const char *buff, unsigned long len, void *data)
{
	char 	tmpbuf[64];
	char	*strptr;
	char	*fieldptr,*valueptr;
	unsigned int value, i;
	
	if (buff && !copy_from_user(tmpbuf, buff, len)) {
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		fieldptr = strsep(&strptr," ");
		if(fieldptr == NULL)
		{
			goto errout;
		}

		valueptr = strsep(&strptr," ");
		if(valueptr == NULL)
		{
			goto errout;
		}
		value = simple_strtol(valueptr, NULL, 0);

		printk("set field: %s, value %d\n",fieldptr,value);
		
		if(!strcmp(fieldptr,"fs"))
		{
			TXINFO_SET(opts1,fs,value);
		}else
		if(!strcmp(fieldptr,"ls"))
		{
			TXINFO_SET(opts1,ls,value);
		}else
		if(!strcmp(fieldptr,"tx_portmask"))
		{
			TXINFO_SET(opts2,tx_portmask,value);
		}else
		if(!strcmp(fieldptr,"ipcs"))
		{
			TXINFO_SET(opts1,ipcs,value);
		}else
		if(!strcmp(fieldptr,"l4cs"))
		{
			TXINFO_SET(opts1,l4cs,value);
		}else
		if(!strcmp(fieldptr,"tpid_sel"))
		{
			TXINFO_SET(opts1,tpid_sel,value);
		}else
		if(!strcmp(fieldptr,"stag_aware"))
		{
			TXINFO_SET(opts1,stag_aware,value);
		}else
		if(!strcmp(fieldptr,"keep"))
		{
			TXINFO_SET(opts3,keep,value);
		}else
		if(!strcmp(fieldptr,"crc"))
		{
			TXINFO_SET(opts1,crc,value);
		}else
		if(!strcmp(fieldptr,"dislrn"))
		{
			TXINFO_SET(opts3,dislrn,value);
		}else
		if(!strcmp(fieldptr,"cputag_psel"))
		{
			TXINFO_SET(opts3,cputag_psel,value);
		}else
		if(!strcmp(fieldptr,"data_length"))
		{
			TXINFO_SET(opts1,data_length,value);
		}else
		if(!strcmp(fieldptr,"cputag"))
		{
			TXINFO_SET(opts2,cputag,value);
		}else
		if(!strcmp(fieldptr,"aspri"))
		{
			TXINFO_SET(opts3,aspri,value);
		}else
		if(!strcmp(fieldptr,"cputag_pri"))
		{
			TXINFO_SET(opts3,cputag_pri,value);
		}else
		if(!strcmp(fieldptr,"tx_cvlan_action"))
		{
			TXINFO_SET(opts2,tx_cvlan_action,value);
		}else
		if(!strcmp(fieldptr,"tx_svlan_action"))
		{
			TXINFO_SET(opts2,tx_svlan_action,value);
		}else
		if(!strcmp(fieldptr,"tx_pppoe_idx"))
		{
			TXINFO_SET(opts3,tx_pppoe_idx,value);
		}else
		if(!strcmp(fieldptr,"tx_pppoe_action"))
		{
			TXINFO_SET(opts3,tx_pppoe_action,value);
		}else
		if(!strcmp(fieldptr,"vidl"))
		{
			TXINFO_SET(opts2,cvlan_vidl,value);
		}else
		if(!strcmp(fieldptr,"prio"))
		{
			TXINFO_SET(opts2,cvlan_prio,value);
		}else
		if(!strcmp(fieldptr,"vidh"))
		{
			TXINFO_SET(opts2,cvlan_vidh,value);
		}else
		if(!strcmp(fieldptr,"svidl"))
		{
			TXINFO_SET(opts4,svlan_vidl,value);
		}else
		if(!strcmp(fieldptr,"sprio"))
		{
			TXINFO_SET(opts4,svlan_prio,value);
		}else
		if(!strcmp(fieldptr,"svidh"))
		{
			TXINFO_SET(opts4,svlan_vidh,value);
		}else
		if(!strcmp(fieldptr,"tx_dst_stream_id"))
		{
			TXINFO_SET(opts3,tx_dst_stream_id,value);
		}else
		if(!strcmp(fieldptr,"ext_portmask"))
		{
			TXINFO_SET(opts3,tx_dst_stream_id,value);
		}else
		if(!strcmp(fieldptr,"l34_keep"))
		{
			TXINFO_SET(opts3,l34_keep,value);
		}else
		if(!strcmp(fieldptr,"isMacChange"))
		{
			isMacChange = value;
		}else
		if(!strcmp(fieldptr,"tx_dump_flag"))
		{
			tx_dump_flag = value;
		}else
		if(!strcmp(fieldptr,"stopRx"))
		{
			stopRx = value;
			if(!value)
			{
				printk(" Kick all rings pkts! \n");
				for(i=0 ; i<MAX_TXRING_NUM ; i++)
					kick_tx(i);
			}
		}else
		if (!strcmp(fieldptr, "stopTx")) {
			stopTx = value;
		}else
		if(!strcmp(fieldptr,"isRxRingRoute"))
		{
			isRxRingRoute = value;
			multi_ring_set();
		}else
		if(!strcmp(fieldptr,"ring1Route"))
		{
			value = simple_strtol(valueptr, NULL, 16);
			ring1Route = value;
		}else
		if(!strcmp(fieldptr,"gTxRing"))
		{
			gTxRing = value;
		}else
		if (!strcmp(fieldptr, "isLsoPkt")) 
		{
			TXINFO_SET(opts4, lgsen, value);
			if (!value) {
				TXINFO_SET(opts4, lgmtu, 0);
			} else {
				TXINFO_SET(opts4, lgmtu, lgMtu);
			}
			isLsoPkt = value;
		} else
		if(!strcmp(fieldptr,"isLsoPkt"))
		{
			isLsoPkt = value;
		}else
		if(!strcmp(fieldptr,"isPktDbg"))
		{
			isPktDbg = value;
		}else
		if(!strcmp(fieldptr,"isRxCrcDbg"))
		{
			isRxCrcDbg = value;
		}else
		if (!strcmp(fieldptr, "lgMtu")) 
		{
			lgMtu = value;
			if (isLsoPkt) {
				TXINFO_SET(opts4, lgmtu, lgMtu);
			}
		}else
		if (!strcmp(fieldptr, "isRxEor")) 
		{
			isRxEor = value;
		}else
		if(!strcmp(fieldptr,"isSamDbgOn"))
		{
			isSamDbgOn = value;
		}else
		if(!strcmp(fieldptr,"SamDbgLevel"))
		{
			SamDbgLevel = value;
		}
	}
	else
	{
errout:
		printk("tx_dump_writee error\n");
	}

	return len;
}


void rxInfo_dump(rx_extInfo_t rf)
{
	struct rx_info rxInfo = rf.rxInfo;

	/*opt1*/
	printk("\nown: %d\n",rxInfo.opts1.bit.own); // 31
	printk("eor: %d\n",rxInfo.opts1.bit.eor); // 30
	printk("fs: %d\n",rxInfo.opts1.bit.fs); // 29
	printk("ls: %d\n",rxInfo.opts1.bit.ls); // 28
	printk("crcerr: %d\n",rxInfo.opts1.bit.crcerr); // 27
	printk("ipv4csf: %d\n",rxInfo.opts1.bit.ipv4csf); // 26
	printk("l4csf: %d\n",rxInfo.opts1.bit.l4csf); // 25
	printk("rcdf: %d\n",rxInfo.opts1.bit.rcdf); // 24
	printk("ipfrag: %d\n",rxInfo.opts1.bit.ipfrag); // 23
	printk("pppoetag: %d\n",rxInfo.opts1.bit.pppoetag);	// 22
	printk("rwt: %d\n",rxInfo.opts1.bit.rwt); // 21
	printk("data_length: %d\n",rxInfo.opts1.bit.data_length); // 0~11
	printk("addr: %x\n",rxInfo.addr); // 0~31
	
	/*opt2*/
	printk("cputag: %d\n",rxInfo.opts2.bit.cputag);	// 31
	printk("ptp_in_cpu_tag_exist: %d\n",rxInfo.opts2.bit.ptp_in_cpu_tag_exist); // 30
	printk("svlan_tag_exist: %d\n",rxInfo.opts2.bit.svlan_tag_exist); // 29
	printk("reason: %d\n",rxInfo.opts2.bit.reason); // 21~28
	printk("ctagva: %d\n",rxInfo.opts2.bit.ctagva); // 16	
	printk("cvlan_tag: 0x%02X \n",
		((rxInfo.opts2.bit.cvlan_tag << 8) | (rxInfo.opts2.bit.cvlan_tag >> 8)) & 0x0000FFFF); // 0~15
	
	/*opt3*/	
	printk("internal_priority: %d\n",rxInfo.opts3.bit.internal_priority); // 29~31
	if(rxInfo.opts3.bit.src_port_num == 5)
		printk("pon_sid: %d\n",rxInfo.opts3.bit.pon_sid_or_extspa); // 26~28
	else
		printk("extspa: %d\n",(rxInfo.opts3.bit.pon_sid_or_extspa>>4)&0x7); // 22~28
	printk("l3routing: %d\n",rxInfo.opts3.bit.l3routing); // 21
	printk("origformat: %d\n",rxInfo.opts3.bit.origformat); // 20
	printk("src_port_num: %d\n",rxInfo.opts3.bit.src_port_num);	// 16~19
	if(rxInfo.opts3.bit.src_port_num == 9 || rxInfo.opts3.bit.src_port_num == 10 || rxInfo.opts3.bit.src_port_num == 7)
		printk("dst_portmsk: 0x%x\n",rxInfo.opts3.bit.fb_hash_or_dst_portmsk&0x7f); // 0~6
	else{
		printk("fbi: %d\n",rxInfo.opts3.bit.fbi); // 15
		printk("fb_hash_idx: %d\n",rxInfo.opts3.bit.fb_hash_or_dst_portmsk); // 0~14
	}
	/*extend information*/
	printk("ring_num: %d\n",rf.ring_num);
	printk("isPktWithRtkCpuTag: %d \n", isPktWithRtkCpuTag);
}

int rx_dump_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	rxInfo_dump(gRxInfo);
	return (0);
}

int rx_dump_write(struct file *file, const char *buff, unsigned long len, void *data)
{
	return (0);
}

int save_rx_info(struct rx_info rxInfo,unsigned ring_num)
{
	memcpy(&gRxInfo.rxInfo,&rxInfo,sizeof(struct rx_info));
	gRxInfo.ring_num = ring_num;
	return (0);
}

static inline void apply_to_txdesc(DMA_TX_DESC *pDmaTxDesc, struct tx_info *pTxInfo){
	pDmaTxDesc->addr = pTxInfo->addr;
	pDmaTxDesc->opts2 = pTxInfo->opts2.dw;
	pDmaTxDesc->opts3 = pTxInfo->opts3.dw;
	pDmaTxDesc->opts4 = pTxInfo->opts4.dw;
	//must be last write....
	wmb();
	pDmaTxDesc->opts1 = pTxInfo->opts1.dw;

	if(isPktDbg)
	{
		printk("0~3:   0x%08x\n", pDmaTxDesc->opts1);
		printk(">>>>>  own=%d eor=%d fs=%d ls=%d ipcs=%d l4cs=%d tpid_sel=%d stag_aware=%d crc=%d data_length=%d\n"
			, pTxInfo->opts1.bit.own, pTxInfo->opts1.bit.eor, pTxInfo->opts1.bit.fs, pTxInfo->opts1.bit.ls, pTxInfo->opts1.bit.ipcs
			, pTxInfo->opts1.bit.l4cs, pTxInfo->opts1.bit.tpid_sel, pTxInfo->opts1.bit.stag_aware, pTxInfo->opts1.bit.crc, pTxInfo->opts1.bit.data_length);
		printk("4~7:   0x%08x\n", pDmaTxDesc->addr);		
		printk("8~11:  0x%08x\n", pDmaTxDesc->opts2);		
		printk(">>>>>  cpu_tag=%d tx_svlan_action=%d tx_cvlan_action=%d tx_portmask=0x%x cvlan_vidl=%d cvlan_prio=%d cvlan_cfi=%d cvlan_vidh=%d\n"
			, pTxInfo->opts2.bit.cputag, pTxInfo->opts2.bit.tx_svlan_action, pTxInfo->opts2.bit.tx_cvlan_action, pTxInfo->opts2.bit.tx_portmask, pTxInfo->opts2.bit.cvlan_vidl
			, pTxInfo->opts2.bit.cvlan_prio, pTxInfo->opts2.bit.cvlan_cfi, pTxInfo->opts2.bit.cvlan_vidh);
		printk("12~15: 0x%08x\n", pDmaTxDesc->opts3);	
		printk(">>>>>  aspri=%d cputag_pri=%d keep=%d dislrn=%d cputag_psel=%d l34_keep=%d extspa=%d tx_pppoe_action=%d tx_pppoe_idx=%d tx_dst_stream_id=%d\n"
			, pTxInfo->opts3.bit.aspri, pTxInfo->opts3.bit.cputag_pri, pTxInfo->opts3.bit.keep, pTxInfo->opts3.bit.dislrn, pTxInfo->opts3.bit.cputag_psel
			, pTxInfo->opts3.bit.l34_keep, pTxInfo->opts3.bit.extspa, pTxInfo->opts3.bit.tx_pppoe_action, pTxInfo->opts3.bit.tx_pppoe_idx, pTxInfo->opts3.bit.tx_dst_stream_id);
		printk("16~19: 0x%08x", pDmaTxDesc->opts4);	
		printk(">>>>>  lgsen=%d lgmtu=%d svlan_vidl=%d svlan_prio=%d svlan_cfi=%d svlan_vidh=%d\n"
			, pTxInfo->opts4.bit.lgsen, pTxInfo->opts4.bit.lgmtu, pTxInfo->opts4.bit.svlan_vidl, pTxInfo->opts4.bit.svlan_prio, pTxInfo->opts4.bit.svlan_cfi
			, pTxInfo->opts4.bit.svlan_vidh);
	}
}

static inline int idx_sw2hw(int ring_num) {
	return (MAX_TXRING_NUM-1)-ring_num;
}

static inline void kick_tx(int ring_num){

	ring_num = idx_sw2hw(ring_num);
	
	switch(ring_num){
		case 0:
		case 1:
		case 2:
		case 3:
			RTL_W32(IO_CMD,iocmd_reg |(1 << ring_num));
			break;
		case 4:
			RTL_W32(IO_CMD1,iocmd1_reg | TX_POLL5);
			break;
		default:
			printk("%s %d: wrong ring num %d\n", __func__, __LINE__, ring_num);
			break;
	}
}

__IRAM_NIC void re8670_tx (struct re_private *cp,int ring_num)
{
	unsigned tx_tail= cp->tx_Mhqtail[ring_num];
	struct sk_buff *skb;

	while (!((cp->tx_Mhqring[ring_num][tx_tail].opts1)& DescOwn)) {
		if (tx_tail == cp->tx_Mhqhead[ring_num]){
			break;
		}

		skb = cp->tx_skb[ring_num][tx_tail].skb;

		if (unlikely(!skb)){   
			break;
		}

		dev_kfree_skb_any(skb); 
		cp->tx_skb[ring_num][tx_tail].skb = NULL;
		
		tx_tail = NEXT_TX(tx_tail,RE8670_TX_MRING_SIZE[ring_num]);

	}
	cp->tx_Mhqtail[ring_num]=tx_tail;
}

int re8686_test_with_txInfo(struct sk_buff *skb, struct tx_info txInfoInput, int ring_num)
{
	struct net_device *dev = skb->dev;
	struct re_private *cp = DEV2CP(dev);
	unsigned entry;
	u32 eor;
    u32 i;
    unsigned char *ptr;
	unsigned long flags;

	cp->cp_stats.tx_sw_num++;

#if 0
	if(!(cp->dev->flags & IFF_UP)) {
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;	
		return (-1);
	}
#endif

    re8670_tx(cp,ring_num);

	spin_lock_irqsave(&cp->tx_lock, flags);

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= (skb_shinfo(skb)->nr_frags + 1)))
	{
		spin_unlock_irqrestore(&cp->tx_lock, flags);
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		cp->cp_stats.tx_no_desc++;
		return (-1);
	}

	entry = cp->tx_Mhqhead[ring_num];

	eor = (entry == (RE8670_TX_MRING_SIZE[ring_num] - 1)) ? RingEnd : 0;
	if (skb_shinfo(skb)->nr_frags == 0) {
		u32 len;
		DMA_TX_DESC  *txd;
		txd = &cp->tx_Mhqring[ring_num][entry];    

		len = skb->len;				
		// Kaohj --- invalidate DCache before NIC DMA
		dma_cache_wback_inv((unsigned long)skb->data, len);
		cp->tx_skb[ring_num][entry].skb = skb;
		cp->tx_skb[ring_num][entry].mapping = (dma_addr_t)(skb->data);
		cp->tx_skb[ring_num][entry].frag = 0;

		//default setting, always need this
		txInfoInput.addr = (u32)(skb->data);
		txInfoInput.opts1.dw |= (eor | len | DescOwn | FirstFrag | LastFrag);
	

		if(tx_dump_flag){
			tx_dump(txInfoInput);
		}

		//apply to txdesc
		apply_to_txdesc(txd, &txInfoInput);

		if(isPktDbg) {
			printk("FROM_FWD[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d VlanAct=%d cVlan=%d cPri=%d sVlan=%d sPri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x %s=%d(0x%x)\n",
				(u32)skb&0xffff,
				skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],
				skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11],
				(skb->data[12]<<8)|skb->data[13],skb->len,
				txInfoInput.opts2.bit.tx_cvlan_action,
				txInfoInput.opts2.bit.cvlan_vidh<<8|txInfoInput.opts2.bit.cvlan_vidl,
				txInfoInput.opts2.bit.cvlan_prio,
				txInfoInput.opts4.bit.svlan_vidh<<8|txInfoInput.opts4.bit.svlan_vidl,
				txInfoInput.opts4.bit.svlan_prio,
				txInfoInput.opts3.bit.extspa,
				txInfoInput.opts2.bit.tx_portmask,
				txInfoInput.opts3.bit.l34_keep,
				txInfoInput.opts2.bit.tx_portmask&(1 << 5) ? "PON_SID":"EXT_PORTMSK",
				txInfoInput.opts3.bit.tx_dst_stream_id,
				txInfoInput.opts3.bit.tx_dst_stream_id
			);
		}
		
        if(isPktDbg){
            printk("TxDesc: 0x%08x 0x%08x 0x%08x 0x%08x buf 0x%08x\n", txd->opts1, txd->opts2, txd->opts3, txd->opts4, txd->addr);
            ptr = (unsigned char *) txd->addr;
            printk("Dump from 0x%08x\n", ptr);
            for(i = 0;i < (txd->opts1 & 0x1ff);i++)
            {
                if(i % 16 == 0)
                {
                    printk("\n");
                }
                printk("%02x ", *ptr);
                ptr ++;
            }			
			printk("\n------\n");
        }
		entry = NEXT_TX(entry,RE8670_TX_MRING_SIZE[ring_num]);
	}
	else 
	{
		printk("%s %d: not support frag xmit now\n", __func__, __LINE__);
		dev_kfree_skb_any(skb);
	}

	cp->tx_Mhqhead[ring_num] = entry;

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= 1)){
		netif_stop_queue(dev);
	}

	write_buffer_flush();

	spin_unlock_irqrestore(&cp->tx_lock, flags);
	wmb();
	cp->cp_stats.tx_hw_num++;

	kick_tx(ring_num);

	dev->trans_start = jiffies;
	
	return (0);
}



int re8686_tx_with_Info_skb(struct sk_buff *skb)
{
    SAMDBG_PRINT(0, "%s", "transmit packet~~~");

	if (!skb) {
		return (-1);
	}
	
	/*set force mac*/
	if(isMacChange){
		skb->data[0] = 0x00;
		skb->data[1] = 0x00;
		skb->data[2] = 0x00;
		skb->data[3] = 0x00;
		skb->data[4] = 0x00;
		skb->data[5] = 0x03;
	}

    //return re8686_test_with_txInfo_and_mask(skb,&gTxInfo, 0, &mask);
    return (re8686_test_with_txInfo(skb,gTxInfo,gTxRing));
}




__IRAM_NIC
static void re8670_test_rx (struct re_private *cp)
{
	int rx_work;
	unsigned long flags;  
	int ring_num=0;
	unsigned rx_Mtail;
	unsigned int max_ring=1;
    u32 i, j;
    unsigned char *ptr;

	SAMDBG_PRINT(0, "%s", "Received packet !!");
	
	for(j=0 ; j<MAX_RXRING_NUM ; j++)
		SAMDBG_PRINT(0, "RE8670_RX_MRING_SIZE[%d]=%d", j, RE8670_RX_MRING_SIZE[j]);

	spin_lock_irqsave(&cp->rx_lock,flags);

	/*start to rx*/

#if 0
	if(!(cp->dev->flags & IFF_UP)){
		spin_unlock_irqrestore (&cp->rx_lock, flags);   
		return;
	}
#endif

	if(isRxRingRoute)
	{
		printk("ISR1: %x\n",cp->isr1_status);
		max_ring = 6;
	}

	for(ring_num=0;ring_num < max_ring ;ring_num++)
	{
		if(isRxRingRoute && !(1 << ring_num & cp->isr1_status) ) {
			continue;
		}

		printk("\n@@@@@@\n");
		printk(
			"%s: start test rx,devname: %s, IFF_UP: %x, ring %d # \n",
			__FUNCTION__,
			cp->dev->name,
			cp->dev->flags & IFF_UP,
			ring_num);
		
		printk("%s: start test rx,devname: %s, IFF_UP: %x, ring %d\n",__FUNCTION__,cp->dev->name,cp->dev->flags & IFF_UP,ring_num);

		if(stopRx)
		{
			rx_Mtail = cp->rx_Mtail[ring_num];   
			printk("%s: current rx_tail",__FUNCTION__,rx_Mtail);
			
		}
		else
		{
			rx_Mtail = cp->rx_Mtail[ring_num];   

			rx_work = RE8670_RX_MRING_SIZE[ring_num];
			SAMDBG_PRINT(0, "ring_num=%d rx_work=%d", ring_num, rx_work);
			while (rx_work--)
			{
				u32 len;
				struct sk_buff *skb, *new_skb;

				DMA_RX_DESC *desc;
				unsigned buflen;
				struct rx_info rxInfo;

				cp->cp_stats.rx_hw_num++;		           	    
				skb = cp->rx_skb[ring_num][rx_Mtail].skb;

				if (unlikely(!skb)) {
					BUG();	   
				}
				desc = &cp->rx_Mring[ring_num][rx_Mtail];	
				retriveRxInfo(desc, &rxInfo);	

				if (rxInfo.opts1.bit.own) {
					break;
				}

				if (rxInfo.opts1.bit.eor) {
					isRxEor = 1;
				}
				
				save_rx_info(rxInfo,ring_num);

				len = rxInfo.opts1.bit.data_length & 0x0fff;		//minus CRC 4 bytes later

				if (unlikely(rxInfo.opts1.bit.rcdf)) {		//DMA error
					cp->cp_stats.rcdf++;
				}
				
				if (unlikely(rxInfo.opts1.bit.crcerr)) {	//CRC error
					cp->cp_stats.crcerr++;
				}

				buflen = cp->rx_buf_sz + RX_OFFSET;

				if (unlikely((rxInfo.opts1.dw & (FirstFrag | LastFrag)) != (FirstFrag | LastFrag))) {
					cp->cp_stats.frag++;
				}

				new_skb=re8670_getAlloc(SKB_BUF_SIZE);
				
				if (unlikely(!new_skb)) {
					cp->cp_stats.rx_no_mem++;
					dma_cache_wback_inv((unsigned long)skb->data,SKB_BUF_SIZE);
					SAMDBG_PRINT(3, "%s", "rx_next");
					goto rx_next;
				}

				if(rxInfo.opts1.bit.fs==1)
				{
					skb_reserve(skb, RX_OFFSET); // HW DMA start at 4N+2 only in FS.
				}

				if((gTxInfo.opts1.bit.crc ==1) && rxInfo.opts1.bit.fs && rxInfo.opts1.bit.ls)
				{
					len-=4;	//minus CRC 4 bytes here			
				}		
				skb_put(skb, len);

				if(isPktDbg) {
					printk("\n@@@ RX DBG @@@\n");
					printk("Rx St: FS %u LS %u\n", rxInfo.opts1.bit.fs, rxInfo.opts1.bit.ls);
					printk("SKB[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%u\n",(u32)skb&0xffff
					,skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5]
					,skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11]			
					,(skb->data[12]<<8)|skb->data[13],len);
				}

				if (skb->data[12] == 0x88 && skb->data[13] == 0x99) {
					isPktWithRtkCpuTag = 1;
				} else {
					isPktWithRtkCpuTag = 0;
				}

		        if(isRxCrcDbg && rxInfo.opts1.bit.crcerr){
					printk("\n@@@ RX CRC DBG @@@\n");
					printk("Rx St: FS %u LS %u\n", rxInfo.opts1.bit.fs, rxInfo.opts1.bit.ls);
					printk("SKB[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%u\n",(u32)skb&0xffff
					,skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5]
					,skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11]			
					,(skb->data[12]<<8)|skb->data[13],len);

		            ptr = (unsigned char *) skb->data;
		            printk("Dump from 0x%08x\n", ptr);
		            for(i = 0;i < (rxInfo.opts1.bit.data_length & 0x1fff);i++)
		            {
		                if(i % 16 == 0)
		                {
		                    printk("\n");
		                }
		                printk("%02x ", *ptr);
		                ptr ++;
		            }
					printk("\n");
		        }

				cp->rx_Mring[ring_num][rx_Mtail].addr = CPHYSADDR(new_skb->data);
				cp->rx_skb[ring_num][rx_Mtail].skb = new_skb;
				new_skb->dev = cp->dev;;

				//dma_cache_wback_inv((unsigned long)new_skb->data,buflen);
				dma_cache_wback_inv((unsigned long)new_skb->data,SKB_BUF_SIZE);

				// Software flow control for Jumbo Frame RX issue. 
				{
					u16 cdo;
					int drop_ring_size=TH_ON_VAL>>1;
					if(ring_num==0)
						cdo=RTL_R16(RxCDO);
					else
						cdo=RTL_R16(RxCDO2+ADDR_OFFSET*(ring_num-1));

					if(cdo+drop_ring_size<RE8670_RX_MRING_SIZE[ring_num])
					{
						if((cdo<=rx_Mtail)&&(cdo+drop_ring_size>rx_Mtail))
						{
							printk("NIC Full tail=%d cdo=%d\n",rx_Mtail,cdo);
							dev_kfree_skb_any(skb);
							SAMDBG_PRINT(3, "%s", "rx_next");
							goto rx_next;
						}
					}
					else
					{
						if((cdo<=(rx_Mtail+RE8670_RX_MRING_SIZE[ring_num]))&&(cdo+drop_ring_size>(rx_Mtail+RE8670_RX_MRING_SIZE[ring_num])))
						{
							printk("NIC Full2 tail=%d cdo=%d\n",rx_Mtail,cdo);
							dev_kfree_skb_any(skb);
							SAMDBG_PRINT(3, "%s", "rx_next");
							goto rx_next;				
						}
					}			
				}
				/*fragment packet and LSO test */
				if(isLsoPkt && rxInfo.opts1.bit.fs && !rxInfo.opts1.bit.ls)
				{
					if(jumboFrame) {
	                    dev_kfree_skb_any(jumboFrame);
	                    jumboFrame = NULL;
	                    jumboLength = 0;
					}

	    		    jumboFrame = dev_alloc_skb(JUMBO_SKB_BUF_SIZE);

	                if(!jumboFrame) {
	                    printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
						dev_kfree_skb_any(skb);
						SAMDBG_PRINT(3, "%s", "rx_next");
						goto rx_next;				
	                }

					//memcpy(jumboFrame,skb,sizeof(struct sk_buff));
					jumboFrame->dev = skb->dev;
	                skb_put(jumboFrame, len);
					memcpy(jumboFrame->data,skb->data,len);				
					jumboLength = len;
					SAMDBG_PRINT(3, "%s", "rx_next");
					goto rx_next;
				}else
				if(isLsoPkt && !rxInfo.opts1.bit.fs && !rxInfo.opts1.bit.ls)
				{
	                if(!jumboFrame) {
	                    printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
						dev_kfree_skb_any(skb);
						SAMDBG_PRINT(3, "%s", "rx_next");
						goto rx_next;				
	                }

	                skb_put(jumboFrame, len);
					memcpy(jumboFrame->data+jumboLength,skb->data,len);				
					jumboLength += len;
					SAMDBG_PRINT(3, "%s", "rx_next");
					goto rx_next;
				}else
				if(isLsoPkt && !rxInfo.opts1.bit.fs && rxInfo.opts1.bit.ls)
				{
	                if(!jumboFrame) {
	                    printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
						dev_kfree_skb_any(skb);
						SAMDBG_PRINT(3, "%s", "rx_next");
						goto rx_next;				
	                }

	                skb_put(jumboFrame, len);
					memcpy(jumboFrame->data+jumboLength,skb->data,len);				
					jumboLength += len;
					jumboLength -=4;	//minus CRC 4 bytes here
					jumboFrame->len = jumboLength;
					//printk("skb %x, frame len %d\n",jumboFrame,jumboLength);
					re8686_tx_with_Info_skb(jumboFrame);
					
				}else{
					if (stopTx < 1 || stopTx == 2) {
						re8686_tx_with_Info_skb(skb);
						if (!stopTx) {
							stopTx = 1;
						}
					} else {
						printk("stopTx: %u\n", stopTx);
					}
				}
				
		rx_next:
				desc->opts1 = (DescOwn | cp->rx_buf_sz) | ((rx_Mtail == (RE8670_RX_MRING_SIZE[ring_num] - 1))?RingEnd:0);
				updateGmacFlowControl(rx_Mtail,ring_num);
				rx_Mtail = NEXT_RX(rx_Mtail,RE8670_RX_MRING_SIZE[ring_num]);

			}
			cp->rx_Mtail[ring_num] = rx_Mtail;

			if(rx_work <= 0){
				tasklet_hi_schedule(&cp->rx_tasklets);
			}
		}
	}
	unmask_rx_int();	
	spin_unlock_irqrestore (&cp->rx_lock, flags); 
}

#define RTL8686_TEST_PROC_DIR_NAME "rtl8686test"
struct proc_dir_entry *rtl8686_test_proc_dir=NULL;

static int rxinfo_open(struct inode *inode, struct file *file)
{
        return single_open(file, rx_dump_read, inode->i_private);
}
static const struct file_operations rxinfo_fops = {
        .owner          = THIS_MODULE,
        .open           = rxinfo_open,
        .read           = seq_read,
        .write          = rx_dump_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int txinfo_open(struct inode *inode, struct file *file)
{
        return single_open(file, tx_dump_read, inode->i_private);
}
static const struct file_operations txinfo_fops = {
        .owner          = THIS_MODULE,
        .open           = txinfo_open,
        .read           = seq_read,
        .write          = tx_dump_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static void re8686_test_proc(void){

	if(rtl8686_test_proc_dir==NULL)
		rtl8686_test_proc_dir = proc_mkdir(RTL8686_TEST_PROC_DIR_NAME,NULL);
	if(rtl8686_test_proc_dir)
	{
		proc_create_data("rxinfo", 0, rtl8686_test_proc_dir,&rxinfo_fops, NULL);
		proc_create_data("txinfo", 0, rtl8686_test_proc_dir,&txinfo_fops, NULL);
	}
}

static int __init re8686_test_init(void)
{
	struct net_device *dev;	
	/*hook re8686_test to interrupt */
	dev = dev_get_by_name(&init_net,"eth0");
	if(dev==NULL)
	{
		printk("can't find eth0 device\n");
	}
	struct re_private *cp = DEV2CP(dev);

	printk(" SAMDBG> %s %d cp=0x%x \n", __func__, __LINE__, cp);
	cp->rx_tasklets.func = (void (*)(unsigned long))re8670_test_rx;
	printk("%s: devname %s, task %x\n",__FUNCTION__,dev->name, cp->rx_tasklets.func);
	
	re8686_test_proc();
	/*set rx ring, all mapping to ring 0*/
	multi_ring_set();
	
	return (0);
}

static void __exit re8686_test_exist(void)
{
	printk("exist re8686_test module\n");
}



module_init(re8686_test_init);
module_exit(re8686_test_exist);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sam Hsu <sam_hsu@realtek.com>");

