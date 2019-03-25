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
#include "re_privskb.h"
#include <bspchip.h>
#include "re8686_rtl9607c.h"

#undef __IRAM_NIC
#define __IRAM_NIC __attribute__ ((__section__(".iram-fwd")))

typedef struct rx_extInfo {
	struct rx_info rxInfo;
	unsigned ring_num;
}rx_extInfo_t;

static inline void kick_tx(unsigned int gmac, int ring_num);
void send_local_out_jumbo_frame0( void );
void send_local_out_jumbo_frame1( void );
void init_gdna(void);

struct tx_info gTxInfo;
rx_extInfo_t gRxInfo;
static int isMacChange = 0;
static int stopRx = 0;
static unsigned int stopTx = 2;
static int isRxRingRoute = 0;
static unsigned int ring1Route = 0;
static int gTxRing=0;
static int loopbacktest = 0;
static int isLsoPkt=0;
static int isTxJumbo = 0;
static int localoutTxJumboNum = 0;
static int isPktDbg=0;
static int isRxCrcDbg=0;
static struct sk_buff *jumboFrame = NULL;
static unsigned short jumboLength = 0;
static unsigned int localoutjumboLength = 0;
static int lgMtu = 1500;
static int isRxEor = 0;
static int isPktWithRtkCpuTag = 0;
static int isModuleDbgOn = 0;
static int ModuleDbgLevel = 7;

#define JUMBO_SKB_BUF_SIZE	(13312+2)
#define SKB_BUF_SIZE 1600
#define RX_OFFSET 2

#define TXINFO_SET(sel,field,value) (gTxInfo.sel.bit.field = value)

#define DEV2CP(dev)  (((struct re_dev_private*)dev->priv)->pCp)
#define DEVPRIV(dev)  ((struct re_dev_private*)dev->priv)
#define NEXT_RX(N,RING_SIZE)		(((N) + 1) & (RING_SIZE - 1))
#define NEXT_TX(N,RING_SIZE)		(((N) + 1) & (RING_SIZE - 1))

static int tx_dump_flag = 0;

#define MOD_DBG_PRINT(level, fmt, args...)	do{if(isModuleDbgOn && ModuleDbgLevel>=level){printk(" Module Debug> LEVEL=%d %s %d, "fmt"\n", level, __func__, __LINE__, args);}}while(0)

#define TX_HQBUFFS_AVAIL(CP,ring_num)					\
		(((CP)->tx_Mhqtail[ring_num] - (CP)->tx_Mhqhead[ring_num] + re8670_tx_ring_size[(CP)->gmac][ring_num] - 1)&(re8670_tx_ring_size[(CP)->gmac][ring_num] - 1))		


#define multi_ring_set()

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
#define set_rring_route(idx) RLE0787_W32(idx, RRING_ROUTING1, 0x65432100);
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
};

#define RX_ALL(idx)	\
	(IMR_RX_OK | IMR_RER_RUNT | IMR_RER_OVF \
	|((rx_multiring_bitmap[idx] & 1) ? IMR_RDU : 0)\
	|(rx_multiring_bitmap[idx] >> 1) << 11)

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

struct ring_info {
	struct sk_buff		*skb;
	dma_addr_t			mapping;
	unsigned			frag;
};


/*================================================
			GMAC used Global Variable
================================================*/
static unsigned int gmac_enabled[MAX_GMAC_NUM] = {ON, ON, ON};
static unsigned int gmac_eth_base[MAX_GMAC_NUM] = {GMAC0_ETHBASE, GMAC1_ETHBASE, GMAC2_ETHBASE};
static unsigned int gmac_cpu_port[MAX_GMAC_NUM] = {CPU_PORT0, CPU_PORT1, CPU_PORT2};

static unsigned int re8670_rx_ring_size[MAX_GMAC_NUM][MAX_RXRING_NUM] = 
{
	{GMAC0_RX1_SIZE, GMAC0_RX2_SIZE, GMAC0_RX3_SIZE, GMAC0_RX4_SIZE, GMAC0_RX5_SIZE, GMAC0_RX6_SIZE},
	{GMAC1_RX1_SIZE, GMAC1_RX2_SIZE, GMAC1_RX3_SIZE, GMAC1_RX4_SIZE, GMAC1_RX5_SIZE, GMAC1_RX6_SIZE},
	{GMAC2_RX1_SIZE, GMAC2_RX2_SIZE, GMAC2_RX3_SIZE, GMAC2_RX4_SIZE, GMAC2_RX5_SIZE, GMAC2_RX6_SIZE}
};

static unsigned int re8670_tx_ring_size[MAX_GMAC_NUM][MAX_TXRING_NUM] = 
{
	{GMAC0_TX1_SIZE, GMAC0_TX2_SIZE, GMAC0_TX3_SIZE, GMAC0_TX4_SIZE, GMAC0_TX5_SIZE},
	{GMAC1_TX1_SIZE, GMAC1_TX2_SIZE, GMAC1_TX3_SIZE, GMAC1_TX4_SIZE, GMAC1_TX5_SIZE},
	{GMAC2_TX1_SIZE, GMAC2_TX2_SIZE, GMAC2_TX3_SIZE, GMAC2_TX4_SIZE, GMAC2_TX5_SIZE}
};

static unsigned int rx_multiring_bitmap[MAX_GMAC_NUM] = 
{
	GMAC0_RX_MULTIRING_BITMAP,
	GMAC1_RX_MULTIRING_BITMAP,
	GMAC2_RX_MULTIRING_BITMAP
};

static unsigned int tx_multiring_bitmap[MAX_GMAC_NUM] = 
{
	GMAC0_TX_MULTIRING_BITMAP,
	GMAC1_TX_MULTIRING_BITMAP,
	GMAC2_TX_MULTIRING_BITMAP
};

static unsigned int rx_only_ring1[MAX_GMAC_NUM] = 
{
	GMAC0_RX_ONLY_RING1,
	GMAC1_RX_ONLY_RING1,
	GMAC2_RX_ONLY_RING1
};

static unsigned int rx_not_only_ring1[MAX_GMAC_NUM] = 
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

__IRAM_NIC
struct sk_buff *re8670_getAlloc(unsigned int size)
{	
	struct sk_buff *skb=NULL;

	skb = dev_alloc_skb(size);
	
	return (skb);
}

static inline void unmask_rx_int(unsigned int gmac){
	RLE0787_R16(gmac, IMR)|=(u16)(RX_ALL(gmac));//we still open imr when rx_work==0 for a quickly schedule
	UNMASK_IMR0_RXALL(gmac);
}


static inline void retriveRxInfo(DMA_RX_DESC *desc, struct rx_info *pRxInfo){
	pRxInfo->opts1.dw = desc->opts1;
	pRxInfo->addr = desc->addr;
	pRxInfo->opts2.dw = desc->opts2;
	pRxInfo->opts3.dw = desc->opts3;
}


static inline void updateGmacFlowControl(unsigned int gmac,unsigned rx_tail,int ring_num){
	unsigned int new_cpu_desc_num;		

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
	printk("loopbacktest: %d\n", loopbacktest);
	printk("isLsoPkt: %d, %d\n", isLsoPkt, txInfo.opts4.bit.lgsen);
	printk("isTxJumbo: %d\n", isTxJumbo);
	printk("lgMtu: %d, %d \n", lgMtu, txInfo.opts4.bit.lgmtu);
	printk("isRxEor: %u\n", isRxEor);
	printk("isModuleDbgOn: %d\n",isModuleDbgOn);
	printk("ModuleDbgLevel: %d\n",ModuleDbgLevel);
	printk("localoutjumboLength: %d\n",localoutjumboLength);
	printk("localoutTxJumboNum: %d\n",localoutTxJumboNum);
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
	unsigned int value, i, j;
	
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
					for(j=0 ; j<MAX_GMAC_NUM ; j++)
						kick_tx(j, i);
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
		} else 
		if (!strcmp(fieldptr, "loopbacktest")) {
			loopbacktest = value;
		} else
		if (!strcmp(fieldptr, "isLsoPkt")) 
		{
			TXINFO_SET(opts4, lgsen, value);
			if (!value) {
				TXINFO_SET(opts4, lgmtu, 0);
			} else {
				TXINFO_SET(opts4, lgmtu, lgMtu);
			}
			isLsoPkt = value;
		} else if (!strcmp(fieldptr, "isTxJumbo")) {
			isTxJumbo = value;
		} else 
		if (!strcmp(fieldptr, "localTxJumbo0")) {
			localoutTxJumboNum = value;
			send_local_out_jumbo_frame0();
		} else
		if (!strcmp(fieldptr, "localTxJumbo1")) {
			localoutTxJumboNum = value;
			send_local_out_jumbo_frame1();
		} else
		if (!strcmp(fieldptr, "localTxJumboLen")) {
			localoutjumboLength = value;
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
		if(!strcmp(fieldptr,"isModuleDbgOn"))
		{
			isModuleDbgOn = value;
		}else
		if(!strcmp(fieldptr,"ModuleDbgLevel"))
		{
			ModuleDbgLevel = value;
		}
		else
		if(!strcmp(fieldptr,"init_gdna"))
		{
			init_gdna();
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
	printk("loopbacktest: %d \n", loopbacktest);
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

static inline void kick_tx(unsigned int gmac, int ring_num){	
	ring_num = idx_sw2hw(ring_num);
	
	switch(ring_num) {
		case 0:
		case 1:
		case 2:
		case 3:
			RLE0787_W32(gmac, IO_CMD, iocmd_reg[gmac] | (1 << ring_num));
			break;
		case 4:
			RLE0787_W32(gmac, IO_CMD1, iocmd1_reg[gmac] | TX_POLL5);
			break;
		default:
			printk("%s %d: wrong ring num %d\n", __func__, __LINE__, ring_num);
			break;
	}
}

__IRAM_NIC void re8670_tx (struct re_private *cp,int ring_num)
{
	unsigned int gmac = cp->gmac;
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
		
		tx_tail = NEXT_TX(tx_tail,re8670_tx_ring_size[gmac][ring_num]);

	}
	cp->tx_Mhqtail[ring_num]=tx_tail;
}

int re8686_test_with_txInfo(unsigned int gmac, struct sk_buff *skb, struct tx_info txInfoInput, int ring_num)
{
	struct net_device *dev = skb->dev;
	struct re_private *cp = DEV2CP(dev);
	unsigned entry;
	u32 eor;
    u32 i;
    unsigned char *ptr;
	unsigned long flags;

	cp->cp_stats.tx_sw_num++;

	if(!(cp->dev->flags & IFF_UP)) {
		dev_kfree_skb_any(skb);
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;	
		return (-1);
	}

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

	eor = (entry == (re8670_tx_ring_size[gmac][ring_num] - 1)) ? RingEnd : 0;
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
		txInfoInput.opts1.dw |= (eor | len | DescOwn);
	
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
		entry = NEXT_TX(entry,re8670_tx_ring_size[gmac][ring_num]);
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

	if(isPktDbg){
		printk(" kick_tx gmac=%d ring_num=%d \n", gmac, ring_num);
	}
	kick_tx(gmac, ring_num);

	dev->trans_start = jiffies;
	
	return (0);
}



int re8686_tx_with_Info_skb(unsigned int gmac, struct sk_buff *skb)
{
    MOD_DBG_PRINT(0, "%s", "transmit packet~~~");

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

	if(loopbacktest) {
		if(gTxInfo.opts1.bit.fs==0 && gTxInfo.opts1.bit.ls==1) {
			if(!(RLE0787_R32(gmac, TCR)&(0x3<<8)))
			{
				RLE0787_W32(gmac, TCR, RLE0787_R32(gmac, TCR)|(0x3<<8));
				printk(" last desc loopback mode T to R !!!\n");
				mdelay(3000);
			}
			else
			{
				printk(" last desc is already loopback mode T to R...\n");
			}
		}
	}

    //return re8686_test_with_txInfo_and_mask(skb,&gTxInfo, 0, &mask);
    return (re8686_test_with_txInfo(gmac,skb,gTxInfo,gTxRing));
}




__IRAM_NIC
static void re8670_test_rx (struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	int rx_work;
	unsigned long flags;  
	int ring_num=0;
	unsigned rx_Mtail;
	unsigned int max_ring=1;
    u32 i, j;
    unsigned char *ptr;

	MOD_DBG_PRINT(0, "%s", "Received packet !!");
	
	for(j=0 ; j<MAX_RXRING_NUM ; j++)
		MOD_DBG_PRINT(0, "re8670_rx_ring_size[%d][%d]=%d", gmac, j, re8670_rx_ring_size[gmac][j]);

	spin_lock_irqsave(&cp->rx_lock,flags);

	/*start to rx*/
	if(!(cp->dev->flags & IFF_UP)){
		spin_unlock_irqrestore (&cp->rx_lock, flags);   
		return;
	}

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

			rx_work = re8670_rx_ring_size[gmac][ring_num];
			MOD_DBG_PRINT(0, "ring_num=%d rx_work=%d", ring_num, rx_work);
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

				if(loopbacktest) {
					if(gTxInfo.opts1.bit.fs==0 && gTxInfo.opts1.bit.ls==1) {
						if(len > loopbacktest) {					
							if((RLE0787_R32(gmac, TCR)&(0x3<<8)))
							{
								RLE0787_W32(gmac, TCR, RLE0787_R32(gmac, TCR)&~(0x3<<8));
								printk(" last desc leave loopback mode T to R !!!\n");
							}
							else
							{
								printk(" last desc is already leave loopback mode T to R...\n");
							}
							TXINFO_SET(opts1, fs, 1);
							TXINFO_SET(opts1, ls, 1);
						}
					}
				}

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
					MOD_DBG_PRINT(3, "%s", "rx_next");
					goto rx_next;
				}

				if(rxInfo.opts1.bit.fs==1)
				{
					skb_reserve(skb, RX_OFFSET); // HW DMA start at 4N+2 only in FS.
				}
				MOD_DBG_PRINT(0, "len=%d", len);
				if((gTxInfo.opts1.bit.crc ==1) && rxInfo.opts1.bit.fs && rxInfo.opts1.bit.ls)
				{
					len-=4;	//minus CRC 4 bytes here
					MOD_DBG_PRINT(0, "len=%d", len);
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
				new_skb->dev = cp->dev;

				//dma_cache_wback_inv((unsigned long)new_skb->data,buflen);
				dma_cache_wback_inv((unsigned long)new_skb->data,SKB_BUF_SIZE);

				// Software flow control for Jumbo Frame RX issue. 
				{
					u16 cdo;
					int drop_ring_size=TH_ON_VAL>>1;
					if(ring_num==0)
						cdo=RLE0787_R16(gmac, RxCDO);
					else
						cdo=RLE0787_R16(gmac, RxCDO2+ADDR_OFFSET*(ring_num-1));

					if(cdo+drop_ring_size<re8670_rx_ring_size[gmac][ring_num])
					{
						if((cdo<=rx_Mtail)&&(cdo+drop_ring_size>rx_Mtail))
						{
							printk("NIC Full tail=%d cdo=%d\n",rx_Mtail,cdo);
							dev_kfree_skb_any(skb);
							MOD_DBG_PRINT(3, "%s", "rx_next");
							goto rx_next;
						}
					}
					else
					{
						if((cdo<=(rx_Mtail+re8670_rx_ring_size[gmac][ring_num]))&&(cdo+drop_ring_size>(rx_Mtail+re8670_rx_ring_size[gmac][ring_num])))
						{
							printk("NIC Full2 tail=%d cdo=%d\n",rx_Mtail,cdo);
							dev_kfree_skb_any(skb);
							MOD_DBG_PRINT(3, "%s", "rx_next");
							goto rx_next;				
						}
					}			
				}
				/*fragment packet and LSO test */
				if((isLsoPkt || isTxJumbo) && rxInfo.opts1.bit.fs && !rxInfo.opts1.bit.ls)
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
						MOD_DBG_PRINT(3, "%s", "rx_next");
						goto rx_next;				
	                }

					//memcpy(jumboFrame,skb,sizeof(struct sk_buff));
					jumboFrame->dev = skb->dev;
	                skb_put(jumboFrame, len);
					memcpy(jumboFrame->data,skb->data,len);				
					jumboLength = len;
					MOD_DBG_PRINT(3, "%s", "rx_next");
					goto rx_next;
				}else
				if((isLsoPkt || isTxJumbo) && !rxInfo.opts1.bit.fs && !rxInfo.opts1.bit.ls)
				{
	                if(!jumboFrame) {
	                    printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
						dev_kfree_skb_any(skb);
						MOD_DBG_PRINT(3, "%s", "rx_next");
						goto rx_next;				
	                }

	                skb_put(jumboFrame, len);
					memcpy(jumboFrame->data+jumboLength,skb->data,len);				
					jumboLength += len;
					MOD_DBG_PRINT(3, "%s", "rx_next");
					goto rx_next;
				}else
				if((isLsoPkt || isTxJumbo) && !rxInfo.opts1.bit.fs && rxInfo.opts1.bit.ls)
				{
	                if(!jumboFrame) {
	                    printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
						dev_kfree_skb_any(skb);
						MOD_DBG_PRINT(3, "%s", "rx_next");
						goto rx_next;				
	                }

	                skb_put(jumboFrame, len);
					memcpy(jumboFrame->data+jumboLength,skb->data,len);				
					jumboLength += len;
					jumboLength -=4;	//minus CRC 4 bytes here
					jumboFrame->len = jumboLength;
					//printk("skb %x, frame len %d\n",jumboFrame,jumboLength);
					re8686_tx_with_Info_skb(gmac, jumboFrame);
					
				}else{
					if (stopTx < 1 || stopTx == 2) {
						re8686_tx_with_Info_skb(gmac, skb);
						if (!stopTx) {
							stopTx = 1;
						}
					} else {
						printk("stopTx: %u\n", stopTx);
					}
				}
				
		rx_next:
				desc->opts1 = (DescOwn | cp->rx_buf_sz) | ((rx_Mtail == (re8670_rx_ring_size[gmac][ring_num] - 1))?RingEnd:0);
				updateGmacFlowControl(gmac,rx_Mtail,ring_num);
				rx_Mtail = NEXT_RX(rx_Mtail,re8670_rx_ring_size[gmac][ring_num]);

			}
			cp->rx_Mtail[ring_num] = rx_Mtail;

			if(rx_work <= 0){
				tasklet_hi_schedule(&cp->rx_tasklets);
			}
		}
	}
	unmask_rx_int(gmac);	
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

void init_gdna(void)
{
	printk("%s:%d start..\n", __FILE__, __LINE__);
	//0xb8148000
	//BTG init
			/*0xb8018000 <= 0x00000000
			0xb8018000 <= 0x10000000
			0xb8018000 <= 0x90000000
			0xb8018004 <= 0x07300000
			0xb8018008 <= 0x07300000
			0xb8018008 <= 0x00000000*/
	(*(volatile u32*)0xb8144000) = (u32)0x00000000;
	(*(volatile u32*)0xb8144000) = (u32)0x10000000;
	(*(volatile u32*)0xb8144000) = (u32)0x90000000;
	(*(volatile u32*)0xb8144004) = (u32)0x07300000;
	(*(volatile u32*)0xb8144008) = (u32)0x07300000;
	(*(volatile u32*)0xb8144008) = (u32)0x00000000;
	
	//BTG write:
			/*0xb8018100 <= 0x00000000
			0xb8018100 <= 0x00100010
			0xb8018104 <= 0x00000000
			0xb8018108 <= 0x000061a8
			0xb801810c <= 0x0000c350
			0xb8018110 <= 0x00f00000
			0xb8018114 <= 0x00000fff
			0xb8018118 <= 0x00000010
			0xb8018130 <= 0xa5a55a5a
			0xb8018134 <= 0xffff0000
			0xb8018138 <= 0x0000ffff
			0xb801813c <= 0xff00ff00
			0xb8018140 <= 0x00ff00ff
			0xb8018144 <= 0x5a5aa5a5
			0xb8018148 <= 0x01234567
			0xb801814c <= 0x89abcdef
			0xb8018150 <= 0xaaaa5555
			0xb8018154 <= 0x5555aaaa
			0xb8018158 <= 0xa5a55a5a
			0xb801815c <= 0xffff0000
			0xb8018160 <= 0x0000ffff
			0xb8018164 <= 0xff00ff00
			0xb8018168 <= 0x00ff00ff
			0xb801816c <= 0x5a5aa5a5
			0xb8018100 <= 0x80100010*/ //此步驟完成後即開始 write
	(*(volatile u32*)0xb8144100) = (u32)0x00000000;
	(*(volatile u32*)0xb8144100) = (u32)0x00100010;
	(*(volatile u32*)0xb8144104) = (u32)0x00000000;
	(*(volatile u32*)0xb8144108) = (u32)0x00000096;
	(*(volatile u32*)0xb814410c) = (u32)0x00000032;
	(*(volatile u32*)0xb8144110) = (u32)0x00f00000;
	(*(volatile u32*)0xb8144114) = (u32)0x000fffff;
	(*(volatile u32*)0xb8144118) = (u32)0x00000000;
	(*(volatile u32*)0xb8144130) = (u32)0xa5a55a5a;
	(*(volatile u32*)0xb8144134) = (u32)0xffff0000;
	(*(volatile u32*)0xb8144138) = (u32)0x0000ffff;
	(*(volatile u32*)0xb814413c) = (u32)0xff00ff00;
	(*(volatile u32*)0xb8144140) = (u32)0x00ff00ff;
	(*(volatile u32*)0xb8144144) = (u32)0x5a5aa5a5;
	(*(volatile u32*)0xb8144148) = (u32)0x01234567;
	(*(volatile u32*)0xb814414c) = (u32)0x89abcdef;
	(*(volatile u32*)0xb8144150) = (u32)0xaaaa5555;
	(*(volatile u32*)0xb8144154) = (u32)0x5555aaaa;
	(*(volatile u32*)0xb8144158) = (u32)0xa5a55a5a;
	(*(volatile u32*)0xb814415c) = (u32)0xffff0000;
	(*(volatile u32*)0xb8144160) = (u32)0x0000ffff;
	(*(volatile u32*)0xb8144164) = (u32)0xff00ff00;
	(*(volatile u32*)0xb8144168) = (u32)0x00ff00ff;
	(*(volatile u32*)0xb814416c) = (u32)0x5a5aa5a5;
	(*(volatile u32*)0xb8144100) = (u32)0x80100010;
	 
	//BTG read:
			/*0xb8018200 <= 0x00000000
			0xb8018200 <= 0x00100010
			0xb8018204 <= 0x00000000
			0xb8018208 <= 0x000061a8
			0xb801820c <= 0x0000c350
			0xb8018210 <= 0x00f10000
			0xb8018214 <= 0x00000fff
			0xb8018218 <= 0x00000010
			0xb8018230 <= 0xa5a55a5a
			0xb8018234 <= 0xffff0000
			0xb8018238 <= 0x0000ffff
			0xb801823c <= 0xff00ff00
			0xb8018240 <= 0x00ff00ff
			0xb8018244 <= 0x5a5aa5a5
			0xb8018248 <= 0x01234567
			0xb801824c <= 0x89abcdef
			0xb8018250 <= 0xaaaa5555
			0xb8018254 <= 0x5555aaaa
			0xb8018258 <= 0xa5a55a5a
			0xb801825c <= 0xffff0000
			0xb8018260 <= 0x0000ffff
			0xb8018264 <= 0xff00ff00
			0xb8018268 <= 0x00ff00ff
			0xb801826c <= 0x5a5aa5a5
			0xb8018200 <= 0x80100010*/ //此步驟完成後即開始 read
	(*(volatile u32*)0xb8144200) = (u32)0x00000000;
	(*(volatile u32*)0xb8144200) = (u32)0x00100010;
	(*(volatile u32*)0xb8144204) = (u32)0x00000000;
	(*(volatile u32*)0xb8144208) = (u32)0x000061a8;
	(*(volatile u32*)0xb814420c) = (u32)0x0000c350;
	(*(volatile u32*)0xb8144210) = (u32)0x00f10000;
	(*(volatile u32*)0xb8144214) = (u32)0x00000fff;
	(*(volatile u32*)0xb8144218) = (u32)0x00000010;
	(*(volatile u32*)0xb8144230) = (u32)0xa5a55a5a;
	(*(volatile u32*)0xb8144234) = (u32)0xffff0000;
	(*(volatile u32*)0xb8144238) = (u32)0x0000ffff;
	(*(volatile u32*)0xb814423c) = (u32)0xff00ff00;
	(*(volatile u32*)0xb8144240) = (u32)0x00ff00ff;
	(*(volatile u32*)0xb8144244) = (u32)0x5a5aa5a5;
	(*(volatile u32*)0xb8144248) = (u32)0x01234567;
	(*(volatile u32*)0xb814424c) = (u32)0x89abcdef;
	(*(volatile u32*)0xb8144250) = (u32)0xaaaa5555;
	(*(volatile u32*)0xb8144254) = (u32)0x5555aaaa;
	(*(volatile u32*)0xb8144258) = (u32)0xa5a55a5a;
	(*(volatile u32*)0xb814425c) = (u32)0xffff0000;
	(*(volatile u32*)0xb8144260) = (u32)0x0000ffff;
	(*(volatile u32*)0xb8144264) = (u32)0xff00ff00;
	(*(volatile u32*)0xb8144268) = (u32)0x00ff00ff;
	(*(volatile u32*)0xb814426c) = (u32)0x5a5aa5a5;
	(*(volatile u32*)0xb8144200) = (u32)0x80100010;
	printk("%s:%d end..\n", __FILE__, __LINE__);
}

spinlock_t		test_lock0;
spinlock_t		test_lock1;
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
	cp->rx_tasklets.func = (void (*)(unsigned long))re8670_test_rx;
	printk("%s: devname %s, task %x\n",__FUNCTION__,dev->name, cp->rx_tasklets.func);

	dev = dev_get_by_name(&init_net,"eth1");
	if(dev==NULL)
	{
		printk("can't find eth1 device\n");
	}
	
	cp = DEV2CP(dev);
	cp->rx_tasklets.func = (void (*)(unsigned long))re8670_test_rx;
	printk("%s: devname %s, task %x\n",__FUNCTION__,dev->name, cp->rx_tasklets.func);

	re8686_test_proc();
	/*set rx ring, all mapping to ring 0*/
	multi_ring_set();
	spin_lock_init (&test_lock0);
	spin_lock_init (&test_lock1);
	
	return (0);
}

static void __exit re8686_test_exist(void)
{
	printk("exist re8686_test module\n");
}

static char localoutjumboFrameBuf[9000] = {0};

void send_local0(void)
{
	struct sk_buff *localoutjumboFrame = NULL;
	u32 len = localoutjumboLength;
	struct net_device *dev;	
	unsigned long flags;

	//spin_lock_irqsave(&test_lock, flags);
	dev = dev_get_by_name(&init_net,"eth0");
	if(dev==NULL)
	{
		printk("can't find eth0 device\n");
		return;
	}
	struct re_private *cp = DEV2CP(dev);	
	localoutjumboFrame = dev_alloc_skb(JUMBO_SKB_BUF_SIZE);
    if(!localoutjumboFrame) {
		//spin_unlock_irqrestore(&test_lock, flags);
        printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
		return;			
    }
	localoutjumboFrame->dev = cp->dev;
    skb_put(localoutjumboFrame, len);
	memcpy(localoutjumboFrame->data, &localoutjumboFrameBuf[0], len);	
	re8686_tx_with_Info_skb(cp->gmac, localoutjumboFrame);
	if(localoutjumboFrame) {
        dev_kfree_skb_any(localoutjumboFrame);
	}
	//spin_unlock_irqrestore(&test_lock, flags);
	return;
}

void send_local1(void)
{
	struct sk_buff *localoutjumboFrame = NULL;
	u32 len = localoutjumboLength;
	struct net_device *dev;	
	unsigned long flags;

	//spin_lock_irqsave(&test_lock, flags);
	dev = dev_get_by_name(&init_net,"eth1");
	if(dev==NULL)
	{
		printk("can't find eth1 device\n");
		return;
	}
	struct re_private *cp = DEV2CP(dev);	
	localoutjumboFrame = dev_alloc_skb(JUMBO_SKB_BUF_SIZE);
    if(!localoutjumboFrame) {
		//spin_unlock_irqrestore(&test_lock, flags);
        printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
		return;			
    }
	localoutjumboFrame->dev = cp->dev;
    skb_put(localoutjumboFrame, len);
	memcpy(localoutjumboFrame->data, &localoutjumboFrameBuf[0], len);	
	re8686_tx_with_Info_skb(cp->gmac, localoutjumboFrame);
	if(localoutjumboFrame) {
        dev_kfree_skb_any(localoutjumboFrame);
	}
	//spin_unlock_irqrestore(&test_lock, flags);
	return;
}


void send_local_out_jumbo_frame0( void )
{
	unsigned long flags;
	int i;

	if(localoutjumboFrameBuf[1] == 0x00) 
	{
		localoutjumboFrameBuf[0] = 0x00;localoutjumboFrameBuf[1] = 0xaa;localoutjumboFrameBuf[2] = 0xaa;localoutjumboFrameBuf[3] = 0xaa;
		localoutjumboFrameBuf[4] = 0xaa;localoutjumboFrameBuf[5] = 0xaa;
		localoutjumboFrameBuf[6] = 0x00;localoutjumboFrameBuf[7] = 0xbb;localoutjumboFrameBuf[8] = 0xbb;localoutjumboFrameBuf[9] = 0xbb;
		localoutjumboFrameBuf[10] = 0xbb;localoutjumboFrameBuf[11] = 0xbb;
	}
	
	for(i=0 ; i<localoutTxJumboNum ; i++)
	{
		spin_lock_irqsave(&test_lock0, flags);
		send_local0();
		spin_unlock_irqrestore(&test_lock0, flags);
	}
}

void send_local_out_jumbo_frame1( void )
{
	unsigned long flags;
	int i;

	if(localoutjumboFrameBuf[1] == 0x00) 
	{
		localoutjumboFrameBuf[0] = 0x00;localoutjumboFrameBuf[1] = 0xaa;localoutjumboFrameBuf[2] = 0xaa;localoutjumboFrameBuf[3] = 0xaa;
		localoutjumboFrameBuf[4] = 0xaa;localoutjumboFrameBuf[5] = 0xaa;
		localoutjumboFrameBuf[6] = 0x00;localoutjumboFrameBuf[7] = 0xbb;localoutjumboFrameBuf[8] = 0xbb;localoutjumboFrameBuf[9] = 0xbb;
		localoutjumboFrameBuf[10] = 0xbb;localoutjumboFrameBuf[11] = 0xbb;
	}
	
	for(i=0 ; i<localoutTxJumboNum ; i++)
	{
		spin_lock_irqsave(&test_lock1, flags);
		send_local1();
		spin_unlock_irqrestore(&test_lock1, flags);
	}
}


module_init(re8686_test_init);
module_exit(re8686_test_exist);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sam Hsu <sam_hsu@realtek.com>");

