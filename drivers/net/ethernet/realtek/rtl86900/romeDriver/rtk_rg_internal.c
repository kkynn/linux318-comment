void *rtk_rg_malloc(int NBYTES);
void rtk_rg_free(void *APTR);

#include <rtk_rg_define.h>
#include <rtk_rg_internal.h>
#include <rtk_rg_igmpsnooping.h>


#include <rtk_rg_liteRomeDriver.h>
#include <rtk_rg_callback.h>
#if defined(CONFIG_APOLLO_MP)
//apollo
#elif defined(CONFIG_XDSL_NEW_HWNAT_DRIVER)
#include <rtk_rg_xdsl_extAPI.h>
#endif

// OS-dependent defination
#ifndef CONFIG_APOLLO_MODEL
#ifdef __linux__
#include <linux/slab.h>
#include <linux/skbuff.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 30)
#include <linux/kmod.h>
#endif

#include <linux/kallsyms.h> //for sprint_symbol
#include <linux/module.h> //for sprint_symbol

#if defined(CONFIG_RG_G3_SERIES)
#include <rtk_rg_g3_internal.h>
#endif

//Support Protocol Stack Rx packet dump.
#if defined(CONFIG_APOLLO_MP)
int DumpProtocolStackRx_debug = 0;
#endif
int DumpProtocolStackTx_debug = 0;

static inline int8 *_ui8tod( uint8 n, int8 *p )
{
	if( n > 99 ) *p++ = (n/100) + '0';
	if( n >  9 ) *p++ = ((n/10)%10) + '0';
	*p++ = (n%10) + '0';
	return p;
}

int8 *_inet_ntoa_r(ipaddr_t ipaddr, int8 *p)
{
	uint8 *ucp = (unsigned char *)&ipaddr;
	assert(p!=NULL);
#ifdef __LITTLE_ENDIAN
	p = _ui8tod( ucp[3] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[2] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[1] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[0] & 0xFF, p);
	*p++ = '\0';
#else
	p = _ui8tod( ucp[0] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[1] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[2] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[3] & 0xFF, p);
	*p++ = '\0';
#endif	
	return (p);
}

int8 *_inet_ntoa(rtk_ip_addr_t ina)
{
	static int8 buf[4*sizeof "123"];
	int8 *p = buf;
	uint8 *ucp = (unsigned char *)&ina;
#ifdef __LITTLE_ENDIAN
	p = _ui8tod( ucp[3] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[2] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[1] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[0] & 0xFF, p);
	*p++ = '\0';
#else
	p = _ui8tod( ucp[0] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[1] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[2] & 0xFF, p);
	*p++ = '.';
	p = _ui8tod( ucp[3] & 0xFF, p);
	*p++ = '\0';
#endif
	return (buf);
}

int8 *diag_util_inet_ntoa(uint32 ina)
{	
	static int8 buf[4*sizeof "123"];    
	sprintf(buf, "%d.%d.%d.%d", ((ina>>24)&0xff), ((ina>>16)&0xff), ((ina>>8)&0xff), (ina&0xff));	
	return (buf);
}


static void *rtk_rg_single_start(struct seq_file *p, loff_t *pos)
{
	return NULL + (*pos == 0);
}

static void *rtk_rg_single_next(struct seq_file *p, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void rtk_rg_single_stop(struct seq_file *p, void *v)
{
}

int rtk_rg_seq_open(struct file *file, const struct seq_operations *op)
{
	struct seq_file *p = file->private_data;

	if (!p) {
			p = kmalloc(sizeof(*p), GFP_ATOMIC);
			if (!p){
				return -ENOMEM;
			}
			file->private_data = p;
	}
	memset(p, 0, sizeof(*p));
	mutex_init(&p->lock);
	p->op = op;
#ifdef CONFIG_USER_NS
	p->user_ns = file->f_cred->user_ns;
#endif

	/*
	 * Wrappers around seq_open(e.g. swaps_open) need to be
	 * aware of this. If they set f_version themselves, they
	 * should call seq_open first and then set f_version.
	 */
	file->f_version = 0;

	/*
	 * seq_files support lseek() and pread().  They do not implement
	 * write() at all, but we clear FMODE_PWRITE here for historical
	 * reasons.
	 *
	 * If a client of seq_files a) implements file.write() and b) wishes to
	 * support pwrite() then that client will need to implement its own
	 * file.open() which calls seq_open() and then sets FMODE_PWRITE.
	 */
	file->f_mode &= ~FMODE_PWRITE;
	return 0;
}


int rtk_rg_single_open(struct file *file, int (*show)(struct seq_file *, void *),void *data)
{
	struct seq_operations *op = kmalloc(sizeof(*op), GFP_ATOMIC);
	int res = -ENOMEM;

	if (op) {
		op->start = rtk_rg_single_start;
		op->next = rtk_rg_single_next;
		op->stop = rtk_rg_single_stop;
		op->show = show;
		res = rtk_rg_seq_open(file, op);
		if (!res)
			((struct seq_file *)file->private_data)->private = data;
		else
			kfree(op);
	}
	return res;
}


int assert_eq(int func_return,int expect_return,const char *func,int line)
{
	if (func_return != expect_return) 
	{
		rtlglue_printf("\033[31;43m%s(%d): func_return=0x%x expect_return=0x%x, fail, so abort!\033[m\n",func, line,func_return,expect_return);
#ifdef CONFIG_RG_DEBUG		
		if(rg_kernel.debug_level&RTK_RG_DEBUG_LEVEL_WARN)
		{
			rtlglue_printf("\033[1;33;41m");
			_rtk_rg_dump_stack();
			rtlglue_printf("\033[0m\n");
		}
#endif		
		
		return func_return; 
	}
	return 0;
}

void *rtk_rg_malloc(int NBYTES) 
{
	if(NBYTES==0) return NULL;
	return (void *)kmalloc(NBYTES,GFP_ATOMIC);
}
void rtk_rg_free(void *APTR) 
{
	kfree(APTR);
}

void _rtk_rg_dev_kfree_skb_any(struct sk_buff *skb)
{	
	if(skb==NULL)//the skb didn't allocate
		return;
	if((skb)&&(rg_db.systemGlobal.fwdStatistic))
	{
		rg_db.systemGlobal.statistic.perPortCnt_skb_free[rg_db.pktHdr->ingressPort]++;
	}
#if defined(CONFIG_RG_G3_SERIES)	
	rg_ca_skb_out(skb);
#endif
	dev_kfree_skb_any(skb);	
}

struct sk_buff *_rtk_rg_getMcAlloc(unsigned int size)
{
	struct sk_buff *skb;
#if defined(CONFIG_RG_G3_SERIES)
	skb=dev_alloc_skb(size);
	rg_ca_skb_in(skb, TRUE);
#else
	skb=re8670_getMcAlloc(size);
#endif
	if((skb)&&(rg_db.systemGlobal.fwdStatistic))
	{
		rg_db.systemGlobal.statistic.perPortCnt_skb_pre_alloc_for_mc_bc[rg_db.pktHdr->ingressPort]++;
	}
	//20160614LUKE: assign device in case needed.
#if !defined(CONFIG_RG_G3_SERIES)
	if(skb)skb->dev=nicRootDev;
#endif
	return skb; 
}

struct sk_buff *_rtk_rg_getBcAlloc(unsigned int size)
{
	struct sk_buff *skb;
#if defined(CONFIG_RG_G3_SERIES)
	skb=dev_alloc_skb(size);
	rg_ca_skb_in(skb, TRUE);
#else
	skb=re8670_getBcAlloc(size);
#endif
	if((skb)&&(rg_db.systemGlobal.fwdStatistic))
	{
		rg_db.systemGlobal.statistic.perPortCnt_skb_pre_alloc_for_mc_bc[rg_db.pktHdr->ingressPort]++;
	}
	//20160614LUKE: assign device in case needed.
#if !defined(CONFIG_RG_G3_SERIES)
	if(skb)skb->dev=nicRootDev;
#endif
	return skb; 
}

struct sk_buff *_rtk_rg_getAlloc(unsigned int size)
{
	struct sk_buff *skb;
#if defined(CONFIG_RG_G3_SERIES)
	skb=dev_alloc_skb(size);
	rg_ca_skb_in(skb, TRUE);
#else
	skb=re8670_getAlloc(size);
#endif
	if((skb)&&(rg_db.systemGlobal.fwdStatistic))
	{
		rg_db.systemGlobal.statistic.perPortCnt_skb_pre_alloc_for_uc[rg_db.pktHdr->ingressPort]++;
	}
	//20160614LUKE: assign device in case needed.
#if !defined(CONFIG_RG_G3_SERIES)
	if(skb)skb->dev=nicRootDev;
#endif
	return skb;	
}

#if defined(CONFIG_RG_SUPPORT_JUMBO_FRAME)
struct sk_buff *_rtk_rg_getJumboAlloc(unsigned int size)
{
	struct sk_buff *skb;
#if defined(CONFIG_RG_G3_SERIES)
	skb=dev_alloc_skb(size);
	rg_ca_skb_in(skb, TRUE);
#else
	skb=re8670_getJumboAlloc(size);
#endif
	if((skb)&&(rg_db.systemGlobal.fwdStatistic))
	{
		rg_db.systemGlobal.statistic.perPortCnt_skb_pre_alloc_for_jumbo[rg_db.pktHdr->ingressPort]++;
	}
	//20160614LUKE: assign device in case needed.
#if !defined(CONFIG_RG_G3_SERIES)
	if(skb)skb->dev=nicRootDev;
#endif
	return skb;	
}
#endif

//deprecated, use this carefully!!
struct sk_buff *_rtk_rg_skb_copy(const struct sk_buff *skb, unsigned gfp_mask)
{
	struct sk_buff *ret_skb;
	ret_skb=skb_copy(skb,(gfp_t)gfp_mask);
#if defined(CONFIG_RG_G3_SERIES)
	rg_ca_skb_in(ret_skb, TRUE);
#endif
	if((ret_skb)&&(rg_db.systemGlobal.fwdStatistic))
	{
		rg_db.systemGlobal.statistic.perPortCnt_skb_alloc[rg_db.pktHdr->ingressPort]++;
	}
	return ret_skb; 
}

//deprecated, use this carefully!!
struct sk_buff *_rtk_rg_skb_clone(struct sk_buff *skb, unsigned gfp_mask)
{
	struct sk_buff *ret_skb;
	ret_skb=skb_clone(skb,(gfp_t)gfp_mask);
	if((ret_skb)&&(rg_db.systemGlobal.fwdStatistic))
	{
		rg_db.systemGlobal.statistic.perPortCnt_skb_alloc[rg_db.pktHdr->ingressPort]++;
	}
	return ret_skb; 
}

//This function is not copy SKB, only be used for TX path.
struct sk_buff *rtk_rg_skbCopyToPreAllocSkb(struct sk_buff *skb)
{
	struct sk_buff *new_skb;
#if RTK_RG_SKB_PREALLOCATE
#if defined(CONFIG_RG_SUPPORT_JUMBO_FRAME)
	if(skb->len > SKB_BUF_SIZE)
	{
		new_skb=_rtk_rg_getJumboAlloc(JUMBO_SKB_BUF_SIZE);
		if(new_skb==NULL) return NULL;
		skb_put(new_skb, (skb->len <= JUMBO_SKB_BUF_SIZE) ? skb->len : JUMBO_SKB_BUF_SIZE);
		memcpy(new_skb->data,skb->data,(skb->len <= JUMBO_SKB_BUF_SIZE) ? skb->len : JUMBO_SKB_BUF_SIZE);
	}
	else
#endif
	{
		if(skb->data[0]&1)
		{
			if(skb->data[0]==0xff)			
				new_skb=_rtk_rg_getBcAlloc(SKB_BUF_SIZE);
			else
				new_skb=_rtk_rg_getMcAlloc(SKB_BUF_SIZE);	
		}	
		else
		{
			new_skb=_rtk_rg_getAlloc(SKB_BUF_SIZE);
		}
		if(new_skb==NULL) return NULL;
		skb_put(new_skb, (skb->len <= SKB_BUF_SIZE) ? skb->len : SKB_BUF_SIZE);
		memcpy(new_skb->data,skb->data,(skb->len <= SKB_BUF_SIZE) ? skb->len : SKB_BUF_SIZE);
	}

#else
	//bcSkb = dev_alloc_skb(skb->len);
	new_skb=_rtk_rg_skb_copy(skb,GFP_ATOMIC);
#endif
	//20160614LUKE: assign device in case needed.
#if !defined(CONFIG_RG_G3_SERIES)
	if(new_skb)new_skb->dev=nicRootDev;
#endif
	return new_skb;
}


#endif


int32 _rtk_rg_platform_function_register_failed(void){
	FIXME("pf.function hasn't registered! (Be careful the bootup WARNING message.)\n");
	RETURN_ERR(RT_ERR_RG_DRIVER_NOT_SUPPORT);
}


int32 _rtk_rg_platform_function_register_check(struct platform *pf){
	int i;
	for(i=0;i < sizeof(struct platform)/sizeof(POINTER_CAST);i++){
		if((POINTER_CAST)(*((POINTER_CAST*)pf+i))==0x0){
			*((POINTER_CAST*)pf+i) = (POINTER_CAST)&_rtk_rg_platform_function_register_failed;
			FIXME("pf.function[%d] hasn't registered!!!",i);
		}
	}
	
	return (RT_ERR_RG_OK);
}

#if 1
//#ifdef CONFIG_APOLLO_RLE0371
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17
#define _RTK_RG_CHM _rtk_rg_standard_chksum

static u16
_rtk_rg_standard_chksum(u8 *dataptr, u16 len)
{
  u32 acc;
  u16 src;
  u8 *octetptr;

  acc = 0;

  octetptr = (u8*)dataptr;
  while (len > 1)
  {

    src = (*octetptr) << 8;
    octetptr++;
    src |= (*octetptr);
    octetptr++;
    acc += src;
    len -= 2;
  }

  if (len > 0)
  {
    src = (*octetptr) << 8;
    acc += src;
  }

  acc = (acc >> 16) + (acc & 0x0000ffffUL);
  while (acc >> 16) 
  {
	acc = (acc & 0xffffUL) + (acc >> 16);
  }

  return (u16)acc;
}

u16 inet_chksum(u8 *dataptr, u16 len)
{
  u32 acc;

  acc = _RTK_RG_CHM(dataptr, len);

  return (u16)~(acc & 0xffff);
}

u16 inet_chksum_pseudo(u8 *tcphdr, u16 tcplen,
       u32 srcip, u32 destip,  u8 proto)
{
	u32 acc;
	u8 swapped;

	acc = 0;
	swapped = 0;
	/* iterate through all pbuf in chain */

	acc += _RTK_RG_CHM(tcphdr, tcplen);

	if (tcplen % 2 != 0) 
	{
		swapped = 1 - swapped;
		acc = ((acc & 0xff) << 8) | ((acc & 0xff00UL) >> 8);
	}

	if (swapped) 
	{
		acc = ((acc & 0xff) << 8) | ((acc & 0xff00UL) >> 8);
	}

	acc += (srcip & 0xffffUL);
	acc += ((srcip >> 16) & 0xffffUL);
	acc += (destip & 0xffffUL);
	acc += ((destip>> 16) & 0xffffUL);	
	acc += (u32)proto;
	acc += (u32)tcplen;
	while (acc >> 16) 
	{
		acc = (acc & 0xffffUL) + (acc >> 16);
	}
	return (u16)~(acc & 0xffffUL);

}

u16 inet_chksum_pseudoV6(u8 *tcphdr, u16 tcplen,
       u8 *srcip, u8 *destip,  u8 proto)
{
	u32 acc;
	u8 swapped, i;
	u16 *ipTmp;

	acc = 0;
	swapped = 0;
	/* iterate through all pbuf in chain */

	acc += _RTK_RG_CHM(tcphdr, tcplen);

	if (tcplen % 2 != 0) 
	{
		swapped = 1 - swapped;
		acc = ((acc & 0xff) << 8) | ((acc & 0xff00UL) >> 8);
	}

	if (swapped) 
	{
		acc = ((acc & 0xff) << 8) | ((acc & 0xff00UL) >> 8);
	}

	ipTmp = (u16 *)srcip;
	for(i=0; i<(IPV6_ADDR_LEN/2); i++)
	{
		acc += ntohs(ipTmp[i]);
	}
	ipTmp = (u16 *)destip;
	for(i=0; i<(IPV6_ADDR_LEN/2); i++)
	{
		acc += ntohs(ipTmp[i]);
	}	
	acc += (u32)proto;
	acc += (u32)tcplen;
	while (acc >> 16) 
	{
		acc = (acc & 0xffffUL) + (acc >> 16);
	}
	return (u16)~(acc & 0xffffUL);

}



//Checksum update functions
uint16 _rtk_rg_fwdengine_L4checksumUpdate(uint8 acktag, uint16 ori_checksum, uint32 ori_ip, uint16 ori_port, uint32 ori_seq, uint32 ori_ack, uint32 new_ip, uint16 new_port, uint32 new_seq, uint32 new_ack)
{
	uint32 tmp_chksum;
	//uint16 ori_ttlProto,new_ttlProto;
	//DEBUG("[L4]ori chksum = 0x%x, acktag is 0x%x, ori_ip is 0x%x, ori_port is 0x%x, ori_seq is 0x%x, ori_ack is 0x%x, new_ip is 0x%x, new_port is 0x%x, new_seq is 0x%x, new_ack is 0x%x", ori_checksum, acktag, ori_ip, ori_port, ori_seq, ori_ack, new_ip, new_port, new_seq, new_ack);

	if(ori_checksum==0x0)
	{
		TRACE("No need to update L4 checksum(0x0)");
		return ori_checksum;
	}
	
	//IP
	if(((ori_ip&0xffff0000)^(new_ip&0xffff0000))!=0)
	{
		//DEBUG("1");
		tmp_chksum = ((~ori_checksum)&0xffff) + (((~ori_ip)&0xffff0000)>>16) + (new_ip>>16);
		//DEBUG("chksum = %x",tmp_chksum);
		tmp_chksum += (((~ori_ip)&0xffff) + (new_ip&0xffff));
		//DEBUG("chksum = %x",tmp_chksum);
	}
	else 
	{
		//DEBUG("2");
		tmp_chksum = ((~ori_checksum)&0xffff) + ((~ori_ip)&0xffff) + (new_ip&0xffff);
		//DEBUG("chksum = %x",tmp_chksum);
	}

	//SEQ
	if(ori_seq!=new_seq)
	{
		if(((ori_seq&0xffff0000)^(new_seq&0xffff0000))!=0)
		{
			//DEBUG("1");
			tmp_chksum += (((~ori_seq)&0xffff0000)>>16) + (new_seq>>16);
			//DEBUG("chksum = %x",tmp_chksum);
			tmp_chksum += (((~ori_seq)&0xffff) + (new_seq&0xffff));
			//DEBUG("chksum = %x",tmp_chksum);
		}
		else 
		{
			//DEBUG("2");
			tmp_chksum += ((~ori_seq)&0xffff) + (new_seq&0xffff);
			//DEBUG("chksum = %x",tmp_chksum);
		}
	}

	//ACK
	if(acktag && ori_ack!=new_ack)
	{
		if(((ori_ack&0xffff0000)^(new_ack&0xffff0000))!=0)
		{
			//DEBUG("1");
			tmp_chksum += (((~ori_ack)&0xffff0000)>>16) + (new_ack>>16);
			//DEBUG("chksum = %x",tmp_chksum);
			tmp_chksum += (((~ori_ack)&0xffff) + (new_ack&0xffff));
			//DEBUG("chksum = %x",tmp_chksum);
		}
		else 
		{
			//DEBUG("2");
			tmp_chksum += ((~ori_ack)&0xffff) + (new_ack&0xffff);
			//DEBUG("chksum = %x",tmp_chksum);
		}
	}
	
	//PORT
	if(ori_port!=new_port)
	{
		//DEBUG("3");
		tmp_chksum += (((~ori_port)&0xffff) + new_port);
		//DEBUG("chksum = %x",tmp_chksum);
	}
	//L4Chksum didn't contain TTL field!!
	while (tmp_chksum >> 16) 
	{
		tmp_chksum = (tmp_chksum & 0xffffUL) + (tmp_chksum >> 16);
	}
	
	return ~(tmp_chksum&0xffff);
}

uint16 _rtk_rg_fwdengine_L4checksumUpdateForMss(uint16 ori_checksum, uint16 ori_mss, uint16 new_mss)
{
	uint32 tmp_chksum=((~ori_checksum)&0xffff);

	if(ori_checksum==0x0)
	{
		TRACE("No need to update L4 checksum(0x0)");
		return ori_checksum;
	}
	
	if(ori_mss!=new_mss)
	{
		tmp_chksum += (((~ori_mss)&0xffff) + new_mss);
		while (tmp_chksum >> 16) 
		{
			tmp_chksum = (tmp_chksum & 0xffffUL) + (tmp_chksum >> 16);
		}
		return ~(tmp_chksum&0xffff);
	}
	return ori_checksum;
	
}

uint16 _rtk_rg_fwdengine_L3checksumUpdate(uint16 ori_checksum, uint32 ori_sip, uint8 ori_ttl, uint8 ori_protocol, uint32 new_sip, uint8 new_ttl)
{
	uint32 tmp_chksum;
	uint16 ori_ttlProto,new_ttlProto;
	//DEBUG("[L3]ori chksum = 0x%x, oriSip is 0x%x, oriTTL is 0x%x, oriProto is 0x%x, newSip is 0x%x, newTTL is 0x%x",ori_checksum,ori_sip,ori_ttl,ori_protocol,new_sip,new_ttl);

	if(((ori_sip&0xffff0000)^(new_sip&0xffff0000))!=0)
	{
		//DEBUG("1");
		tmp_chksum = ((~ori_checksum)&0xffff) + ((~ori_sip)>>16) + (new_sip>>16);
		//DEBUG("chksum = %x",tmp_chksum);
		tmp_chksum += (((~ori_sip)&0xffff) + (new_sip&0xffff));
		//DEBUG("chksum = %x",tmp_chksum);
	}
	else
	{
		//DEBUG("2");
		tmp_chksum = ((~ori_checksum)&0xffff) + ((~ori_sip)&0xffff) + (new_sip&0xffff);
		//DEBUG("chksum = %x",tmp_chksum);
	}

	if(ori_ttl!=new_ttl)
	{
		//DEBUG("3");
		ori_ttlProto=(ori_ttl<<8)+ori_protocol;
		new_ttlProto=(new_ttl<<8)+ori_protocol;
		tmp_chksum += ((~ori_ttlProto)&0xffff);
		tmp_chksum += new_ttlProto;
		//DEBUG("chksum = %x",tmp_chksum);
	}

	while (tmp_chksum >> 16) 
	{
		tmp_chksum = (tmp_chksum & 0xffffUL) + (tmp_chksum >> 16);
	}
	
	return ~(tmp_chksum&0xffff);
}

__IRAM_FWDENG
uint16 _rtk_rg_fwdengine_L3checksumUpdateDSCP(uint16 ori_checksum, uint8 header_length, uint8 ori_DSCP, uint8 new_DSCP)
{
	uint32 tmp_chksum;
	uint16 ori_version_IHL_DSCP,new_version_IHL_DSCP;
	//DEBUG("ori chksum = %x, oriSip is %x, oriTTL is %x, oriProto is %x, newSip is %x, newTTL is %x",ori_checksum,ori_sip,ori_ttl,ori_protocol,new_sip,new_ttl);

	if(ori_DSCP!=new_DSCP)
	{
		//DEBUG("3");
		ori_version_IHL_DSCP=0x4000+(header_length<<6)+ori_DSCP;
		new_version_IHL_DSCP=0x4000+(header_length<<6)+new_DSCP;
		tmp_chksum = ((~ori_checksum)&0xffff) + ((~ori_version_IHL_DSCP)&0xffff) + (new_version_IHL_DSCP&0xffff);
		//DEBUG("chksum = %x",tmp_chksum);
	}
	else
		return ori_checksum;

	while (tmp_chksum >> 16) 
	{
		tmp_chksum = (tmp_chksum & 0xffffUL) + (tmp_chksum >> 16);
	}
	
	return ~(tmp_chksum&0xffff);
}

uint16 _rtk_rg_fwdengine_L3checksumUpdateTotalLen(uint16 ori_checksum, uint16 ori_len, uint16 new_len)
{
	uint32 tmp_chksum;
	//DEBUG("ori chksum = %x, oriLen is %x, newLen is %x",ori_checksum,ori_len,new_len);

	if(ori_len!=new_len)
	{
		//DEBUG("3");
		tmp_chksum = ((~ori_checksum)&0xffff) + ((~ori_len)&0xffff) + (new_len&0xffff);
		//DEBUG("chksum = %x",tmp_chksum);
	}
	else
		return ori_checksum;

	while (tmp_chksum >> 16) 
	{
		tmp_chksum = (tmp_chksum & 0xffffUL) + (tmp_chksum >> 16);
	}
	
	return ~(tmp_chksum&0xffff);
}

#endif


int _rtk_rg_wlanDeviceCount_dec(int wlan_idx, unsigned char *macAddr, int *dev_idx)
{
#ifdef CONFIG_MASTER_WLAN0_ENABLE
	int i,idx;
#endif
	if(wlan_idx==0)
	{
#ifdef CONFIG_MASTER_WLAN0_ENABLE
		for(i=rg_db.wlanMbssidHeadIdx;i<rg_db.wlanMbssidHeadIdx+MAX_WLAN_MBSSID_SW_TABLE_SIZE;i++)
		{
			idx=i%MAX_WLAN_MBSSID_SW_TABLE_SIZE;
			if(memcmp(rg_db.wlanMbssid[idx].mac.octet,macAddr,ETHER_ADDR_LEN)==0) //the MAC is finded in table.
			{
				atomic_dec(&rg_db.systemGlobal.wlan0SourceAddrLearningCount[rg_db.wlanMbssid[idx].wlan_dev_idx]);
				if(dev_idx!=NULL)*dev_idx=rg_db.wlanMbssid[idx].wlan_dev_idx;
				return (RT_ERR_RG_OK);			
			}
		}
#endif
	}
	
	return (RT_ERR_RG_OK);
}

int _rtk_rg_ipv6IFIDCompare(unsigned int prefixLen, unsigned char *ipv6Addr, unsigned char *ipv6AddrCmp)
{
	unsigned int i,j;
	i=prefixLen>>3;
	j=(0x1<<(8-(prefixLen%8)))-1;
	if(memcmp(ipv6Addr+(i>0?i:1),ipv6AddrCmp+(i>0?i:1),IPV6_ADDR_LEN-(i>0?i:1)))return 0;
	if((ipv6Addr[i]&j)!=(ipv6AddrCmp[i]&j))return 0;

	return 1;
}

static char cmd_buff[CB_CMD_BUFF_SIZE];
static char env_PATH[CB_CMD_BUFF_SIZE];
int rtk_rg_callback_pipe_cmd(const char *comment, ...) {

	char * envp[]={	//element size 3
		"HOME=/",
		env_PATH,
		NULL
	};
	char * argv[]={ //element size 4
		"/bin/ash",
		"-c",
		cmd_buff,
		NULL
	};
	int idx, retval;
	va_list argList;
	va_start(argList, comment);
	snprintf( env_PATH, CB_CMD_BUFF_SIZE, "PATH=%s", CONFIG_RG_CALLBACK_ENVIRONMENT_VARIABLE_PATH);
	//sprintf( cmd_buff, comment, ##arg);
	vsprintf(cmd_buff, comment, argList);
	//rtlglue_printf("\033[1;35mcallback_pipe_cmd cmd_buff=[%s]\n\033[0m", cmd_buff);
	if(rg_kernel.debug_level&RTK_RG_DEBUG_LEVEL_CALLBACK){
		rtlglue_printf("[rg callback]CMD:");
		for(idx=0;argv[idx]!=NULL;idx++){rtlglue_printf("\033[1;33m%s \033[0m",argv[idx]);}
		if((retval=call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC))==0){rtlglue_printf("\n");}
		else {rtlglue_printf("\033[1;35m [Exec Failed]\033[0m, ret=%d @%s,line:%d\n",retval,__func__,__LINE__);}
	}else{
		retval=call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
	}
	va_end(argList);
	return retval;
}


//==========================  ==============================
/*move from (xdsl/apollo)_litRomeDriver.c Boyce 2014-10-24*/
//==========================  ==============================
int _rtk_rg_eiplookup(ipaddr_t ip)
{
#if 0 //CONFIG_APOLLO_MODEL
	return 0;
#else
 	int i;

	for(i=0;i<MAX_EXTIP_SW_TABLE_SIZE;i++)
	{
		if(rg_db.extip[i].rtk_extip.valid)
		{		
			if(rg_db.extip[i].rtk_extip.extIpAddr==ip)
			{
				return i; //wan interface
			}
		}
	}
	return -1; 
#endif
}

int _rtk_rg_ipv4MatchBindingDomain(ipaddr_t ip, int bindNextHopIdx)
{
	ipaddr_t domain_ip,domain_mask;
	int netif=rg_db.nexthop[bindNextHopIdx].rtk_nexthop.ifIdx;
	if(!rg_db.systemGlobal.interfaceInfo[netif].p_wanStaticInfo) goto RET_ERR;
	if(rg_db.systemGlobal.interfaceInfo[netif].p_wanStaticInfo->ip_version==IPVER_V6ONLY) goto RET_ERR;
		
	domain_ip=rg_db.systemGlobal.interfaceInfo[netif].p_wanStaticInfo->ip_addr;
	domain_mask=rg_db.systemGlobal.interfaceInfo[netif].p_wanStaticInfo->ip_network_mask;

	if((domain_ip&domain_mask)==(ip&domain_mask)) 
		return TRUE;

RET_ERR:
	return FALSE;
}

__SRAM_FWDENG_SLOWPATH
int32 _rtK_rg_checkCategoryPortmask_spa(rtk_rg_port_idx_t spa)
{		
	if(((0x1<<spa)&rg_db.systemGlobal.lanPortMask.portmask)==0)	//unmatch
		return RT_ERR_RG_ACCESSWAN_NOT_LAN;
	
	return SUCCESS;
}

void _rtk_rg_disablePortmaskPermitedLut(uint32 l2Idx)
{
	if(rg_db.systemGlobal.activeLimitField==RG_ACCESSWAN_LIMIT_BY_SMAC && rg_db.systemGlobal.activeLimitFunction==RG_ACCESSWAN_TYPE_PORTMASK && rg_db.lut[l2Idx].valid && rg_db.lut[l2Idx].permit_for_l34_forward &&
		(rg_db.systemGlobal.accessWanLimitPortMask_member.portmask&(0x1<<(rg_db.lut[l2Idx].rtk_lut.entry.l2UcEntry.port+rg_db.lut[l2Idx].rtk_lut.entry.l2UcEntry.ext_port))
#ifdef CONFIG_MASTER_WLAN0_ENABLE										
		|| ((rg_db.lut[l2Idx].rtk_lut.entry.l2UcEntry.ext_port==RTK_RG_MAC_EXT_PORT0
#ifdef CONFIG_RG_WLAN_HWNAT_ACCELERATION
			|| (rg_db.systemGlobal.enableSlaveSSIDBind && rg_db.lut[l2Idx].rtk_lut.entry.l2UcEntry.ext_port==RTK_RG_MAC_EXT_PORT1)
#endif
		) && rg_db.lut[l2Idx].wlan_device_idx>=0 && (rg_db.systemGlobal.accessWanLimitPortMask_wlan0member&(0x1<<rg_db.lut[l2Idx].wlan_device_idx)))
#endif
		))

	{
		rg_db.lut[l2Idx].permit_for_l34_forward=0;
		atomic_dec(&rg_db.systemGlobal.accessWanLimitPortMaskCount);
	}
}

int32 _rtk_rg_checkPortNotExistByPhy(rtk_rg_port_idx_t port)
{
	return ((0x1<<port)&rg_db.systemGlobal.phyPortStatus)?0:1;
}

void _rtk_rg_cleanPortAndProtocolSettings(rtk_port_t port)
{
	int ret;
	rtk_vlan_protoVlanCfg_t protoVlanCfg;

	//Clean all Group of port-and-protocol based VID
	protoVlanCfg.valid=0;
	protoVlanCfg.vid=rg_db.systemGlobal.initParam.fwdVLAN_CPU;
	protoVlanCfg.pri=0;			//FIXME: should I change this?
	protoVlanCfg.dei=0;
	ret = RTK_VLAN_PORTPROTOVLAN_SET(port,RG_IPV4_GROUPID,&protoVlanCfg);
	assert_ok(ret);

	protoVlanCfg.valid=0;
	protoVlanCfg.vid=rg_db.systemGlobal.initParam.fwdVLAN_CPU;
	protoVlanCfg.pri=0;			//FIXME: should I change this?
	protoVlanCfg.dei=0;
	ret = RTK_VLAN_PORTPROTOVLAN_SET(port,RG_ARP_GROUPID,&protoVlanCfg);
	assert_ok(ret);

	protoVlanCfg.valid=0;
	protoVlanCfg.vid=rg_db.systemGlobal.initParam.fwdVLAN_CPU;
	protoVlanCfg.pri=0;			//FIXME: should I change this?
	protoVlanCfg.dei=0;
	ret = RTK_VLAN_PORTPROTOVLAN_SET(port,RG_IPV6_GROUPID,&protoVlanCfg);
	assert_ok(ret);
}


void _rtk_rg_wanVlanTagged(rtk_rg_pktHdr_t *pPktHdr,int vlan_tag_on)
{
	int VLANId;
	VLANId=pPktHdr->egressVlanID;
	//DEBUG("the VLANId decide in _rtk_rg_interfaceVlanTagged is %d",VLANId);
	if(rg_db.vlan[VLANId].fidMode==VLAN_FID_IVL)
	{
		//tag/untag by VLAN untag setting
		if(rg_db.vlan[VLANId].UntagPortmask.bits[0]&rg_kernel.txDesc.tx_tx_portmask)
		{
			//DEBUG("IVL:untagged!");
			pPktHdr->egressVlanTagif=0;
		}
		else
		{
			//DEBUG("IVL:tagged!");
			pPktHdr->egressVlanTagif=1;
		}
	}
	else
	{
		//2 FIXME:Should we check DMAC2CVID settings here?
		//Vlan tagged or not by interface setting
		pPktHdr->egressVlanTagif=vlan_tag_on;
	}
}

void _rtk_rg_wan_mssCache_reset(int wan_idx)
{
	int i;
	rtk_rg_netif_mssCache_t *pCache;

	if(wan_idx<0 || wan_idx>=MAX_NETIF_SW_TABLE_SIZE)return;

	pCache=&rg_db.msscache[wan_idx];
	bzero(pCache, sizeof(*pCache));

	INIT_LIST_HEAD(&pCache->free);
	for(i=0;i<MAX_NETIF_MSSCACHE_HASH;i++)
		INIT_LIST_HEAD(&pCache->hash[i]);
	for(i=0;i<MAX_NETIF_MSSCACHE_POOL;i++)
	{
		INIT_LIST_HEAD(&pCache->pool[i].msscache_list);
		list_add_tail(&pCache->pool[i].msscache_list, &pCache->free);
	}
	TRACE("[Reset] mssCache for wan[%d]!!",wan_idx);
}

void _rtk_rg_wan_mssCache_add(int wan_idx, ipaddr_t dest, unsigned short advmss)
{
	int hash,cnt=0;
	rtk_rg_netif_mssCache_t *pCache;
	rtk_rg_netif_mssCacheEntry_t *pCur;

	if(wan_idx<0 || wan_idx>=MAX_NETIF_SW_TABLE_SIZE)return;
	if(!(rg_db.systemGlobal.interfaceInfo[wan_idx].valid && rg_db.systemGlobal.interfaceInfo[wan_idx].storedInfo.is_wan))return;
	
	pCache=&rg_db.msscache[wan_idx];
	hash=dest&(MAX_NETIF_MSSCACHE_HASH-1);

	if(!list_empty(&pCache->hash[hash]))
	{
		list_for_each_entry(pCur, &pCache->hash[hash], msscache_list)
		{
			cnt++;
			if(pCur->dest==dest)
			{
				//update mss
				TRACE("[Update] mss(%d) for IP %x!!",advmss,dest);
				pCur->advmss=advmss;
				return;
			}
		}
		if(cnt==MAX_NETIF_MSSCACHE_WAYS)
		{
			//take the first(eldest) one, overwrite it and move to tail.
			pCur=list_first_entry(&pCache->hash[hash], rtk_rg_netif_mssCacheEntry_t, msscache_list);
			pCur->dest=dest;
			pCur->advmss=advmss;
			TRACE("[Overwrite] mss(%d) for IP %x!!",advmss,dest);
			list_move_tail(&pCur->msscache_list, &pCache->hash[hash]);
			return;
		}
	}

	if(list_empty(&pCache->free))return;
	
	pCur=list_first_entry(&pCache->free, rtk_rg_netif_mssCacheEntry_t, msscache_list);
	list_del_init(&pCur->msscache_list);
	
	pCur->dest=dest;
	pCur->advmss=advmss;

	list_add_tail(&pCur->msscache_list, &pCache->hash[hash]);	//add newest to tail
	TRACE("[Insert] mss(%d) for IP %x!!",advmss,dest);
}

unsigned int _rtk_rg_wan_mssCache_search(int wan_idx, ipaddr_t dest)
{
	int hash;
	unsigned short advmss=DEFAULT_MSSCACHE_VALUE;
	rtk_rg_netif_mssCache_t *pCache;
	rtk_rg_netif_mssCacheEntry_t *pCur;
	
	if(wan_idx<0 || wan_idx>=MAX_NETIF_SW_TABLE_SIZE)return advmss;

	pCache=&rg_db.msscache[wan_idx];
	hash=dest&(MAX_NETIF_MSSCACHE_HASH-1);

	if(!list_empty(&pCache->hash[hash]))
	{
		list_for_each_entry(pCur, &pCache->hash[hash], msscache_list)
		{
			if(pCur->dest==dest)
			{
				advmss=pCur->advmss;
				TRACE("[Search] mss(%d) for IP %x!!",advmss,dest);
				break;
			}
		}
	}
	return advmss;
}

#if 0
int _rtk_rg_portBindingLookup(int srcPort, int srcExtPort, int vid, rtk_rg_sipDipClassification_t sipDipClass, rtk_rg_pktHdr_t *pPktHdr)
{
	int i;
	//src port binding
	for(i=0;i<MAX_BIND_SW_TABLE_SIZE;i++)
	{
		if(rg_db.bind[i].valid)			
		{			
			int wanTypeIdx=rg_db.bind[i].rtk_bind.wanTypeIdx;
			int match=0;
			//DEBUG("srcPort is %d, bt[%d].portMask is %x, vid is %d, vidLan is %d",srcPort,i,rg_db.bind[i].rtk_bind.portMask,vid,rg_db.bind[i].rtk_bind.vidLan);
			if(srcPort==RTK_RG_PORT_CPU)
			{
				if(rg_db.bind[i].rtk_bind.extPortMask.bits[0]&(1<<srcExtPort)) 
				{
					if(rg_db.bind[i].rtk_bind.vidLan==vid) match=1;
				}
			}
			else			
			{
				if(rg_db.bind[i].rtk_bind.portMask.bits[0]&(1<<srcPort))
				{
					if(rg_db.bind[i].rtk_bind.vidLan==vid) match=1;
				}
			}

			if(match==1)
			{
				if((sipDipClass==SIP_DIP_CLASS_NAPT) && (rg_db.wantype[wanTypeIdx].rtk_wantype.wanType==L34_WAN_TYPE_L34NAT_ROUTE))
				{
					DEBUG("L4 binding look up success! nhidx = %d",rg_db.wantype[wanTypeIdx].rtk_wantype.nhIdx);
					pPktHdr->bindingDecision=RG_BINDING_LAYER4;
					return rg_db.wantype[wanTypeIdx].rtk_wantype.nhIdx; //nextHopIdx for pPktHdr->bindNextHopIdx		
				}
				else if((sipDipClass==SIP_DIP_CLASS_ROUTING) && (rg_db.wantype[wanTypeIdx].rtk_wantype.wanType==L34_WAN_TYPE_L3_ROUTE))
				{					
					DEBUG("L3 binding look up success! nhidx = %d",rg_db.wantype[wanTypeIdx].rtk_wantype.nhIdx);
					pPktHdr->bindingDecision=RG_BINDING_LAYER3;
					return rg_db.wantype[wanTypeIdx].rtk_wantype.nhIdx; //nextHopIdx for pPktHdr->bindNextHopIdx
				}	
				else
				{
					FIXME("Unmatched binding action!\n");
				}
			}
		}		
	}		
	//DEBUG("binding look up failed!!");
	pPktHdr->bindingDecision=RG_BINDING_FINISHED;
	return FAIL;
}
#endif


int _rtk_rg_broadcastWithDscpRemarkMask_get(unsigned int bcMask,unsigned int *bcWithoutDscpRemarMask,unsigned int *bcWithDscpRemarByInternalpriMask,unsigned int *bcWithDscpRemarByDscpkMask)
{
	
	/*s
	sk into two masks, onr for dscp remarking, one for no need to remarking */
	int i;
	(*bcWithDscpRemarByDscpkMask) = 0x0;
	(*bcWithDscpRemarByInternalpriMask) = 0x0;
	(*bcWithoutDscpRemarMask) = 0x0;
	for(i=0;i<RTK_RG_MAC_PORT_MAX;i++){
		if(RG_INVALID_MAC_PORT(i)) continue;
		if(bcMask&(1<<i)){
			if(rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkEgressPortEnableAndSrcSelect[i]==ENABLED_DSCP_REMARK_AND_SRC_FROM_INTERNALPRI){
				(*bcWithDscpRemarByInternalpriMask) |= (1<<i);
			}else if(rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkEgressPortEnableAndSrcSelect[i]==ENABLED_DSCP_REMARK_AND_SRC_FROM_DSCP){
				(*bcWithDscpRemarByDscpkMask) |= (1<<i);
			}else{
				(*bcWithoutDscpRemarMask) |= (1<<i);
			}
		}
	}
	return SUCCESS;
}


int _rtk_rg_broadcastWithDot1pRemarkMask_get(unsigned int bcMask,unsigned int *bcWithDot1pRemarkMask,unsigned int *bcWithoutDot1pRemarkMask)
{
	/*seperate broadcast/multicast portMask into two masks, onr for dot1p remarking, one for no need to remarking */
	int i;
	(*bcWithDot1pRemarkMask) = 0x0;
	(*bcWithoutDot1pRemarkMask) = 0x0;
	for(i=0;i<RTK_RG_MAC_PORT_MAX;i++){
		if(RG_INVALID_MAC_PORT(i)) continue;
		if(bcMask&(1<<i)){
			if(rg_db.systemGlobal.qosInternalDecision.qosDot1pPriRemarkByInternalPriEgressPortEnable[i]){
				(*bcWithDot1pRemarkMask) |= (1<<i);
			}else{
				(*bcWithoutDot1pRemarkMask) |= (1<<i);
			}
		}
	}
	return SUCCESS;
}



int32 _rtk_rg_igmpReport_portmask_check_and_limit(rtk_rg_pktHdr_t *pPktHdr, uint32* egress_port_mask){

	//patch for user force control IGMP report forward ports.  
	// *egress_port_mask should bring the original egress portmask, and here will return the check result. If result set to 0x0 means no need to send!
	if ( pPktHdr != NULL  && rg_db.systemGlobal.initParam.igmpSnoopingEnable
	&& ((rg_db.systemGlobal.multicastProtocol!=RG_MC_MLD_ONLY &&  (pPktHdr->tagif&IGMP_TAGIF)  &&  (pPktHdr->IGMPType == IGMPV1_REPORT  ||  pPktHdr->IGMPType == IGMPV2_REPORT || pPktHdr->IGMPType == IGMPV3_REPORT))
	||  (rg_db.systemGlobal.multicastProtocol!=RG_MC_IGMP_ONLY && (pPktHdr->tagif&IPV6_MLD_TAGIF) && (pPktHdr->IGMPv6Type==MLDV1_REPORT || pPktHdr->IGMPv6Type==MLDV2_REPORT ))) ) 
	{	
#ifdef CONFIG_DUALBAND_CONCURRENT		
		if(pPktHdr->egressVlanTagif==1 && 
			pPktHdr->egressVlanID==CONFIG_DEFAULT_TO_SLAVE_GMAC_VID &&
			pPktHdr->egressPriority==CONFIG_DEFAULT_TO_SLAVE_GMAC_PRI){
			//check packet to slave wifi
			if((rg_db.systemGlobal.igmpReportPortmask&(1<<RTK_RG_EXT_PORT1))==0x0){
				TRACE("IGMP report forward to slave wifi is limited");
				*egress_port_mask = 0x0;
				return SUCCESS;
			}
		}
		else
#endif
		{
			//check packet to other physical port or master wifi
			*egress_port_mask &= rg_db.systemGlobal.igmpReportPortmask;//modified the portmask by proc/rg/igmp_report_filter_portmask
			TRACE("IGMP report forward port limit to 0x%x",*egress_port_mask);
			return SUCCESS;
		}

	}	

	return SUCCESS;
}

int32 _rtk_rg_igmpLeave_portmask_check_and_limit(rtk_rg_pktHdr_t *pPktHdr, uint32* egress_port_mask){

	//patch for user force control IGMP leave forward ports.  
	// *egress_port_mask should bring the original egress portmask, and here will return the check result. If result set to 0x0 means no need to send!
	if ( pPktHdr != NULL  && rg_db.systemGlobal.initParam.igmpSnoopingEnable 
		&& ((rg_db.systemGlobal.multicastProtocol!=RG_MC_MLD_ONLY &&  (pPktHdr->tagif&IGMP_TAGIF)  &&  (pPktHdr->IGMPType == IGMPV2_LEAVE/*IGMPv2 leave*/ ) ) 
		||  (rg_db.systemGlobal.multicastProtocol!=RG_MC_IGMP_ONLY && (pPktHdr->tagif&IPV6_MLD_TAGIF)  &&  (pPktHdr->IGMPv6Type==MLDV1_DONE) )))
	{	
#ifdef CONFIG_DUALBAND_CONCURRENT		
		if(pPktHdr->egressVlanTagif==1 && 
			pPktHdr->egressVlanID==CONFIG_DEFAULT_TO_SLAVE_GMAC_VID &&
			pPktHdr->egressPriority==CONFIG_DEFAULT_TO_SLAVE_GMAC_PRI){
			//check packet to slave wifi
			if((rg_db.systemGlobal.igmpLeavePortmask&(1<<RTK_RG_EXT_PORT1))==0x0){
				TRACE("IGMP leave forward to slave wifi is limited");
				*egress_port_mask = 0x0;
				return SUCCESS;
			}
		}
		else
#endif
		{
			//check packet to other physical port or master wifi
			*egress_port_mask &= rg_db.systemGlobal.igmpLeavePortmask;//modified the portmask by proc/rg/igmp_leave_filter_portmask
			TRACE("IGMP Leave forward port limit to 0x%x",*egress_port_mask);
			return SUCCESS;
		}

	}	

	return SUCCESS;
}

//MAC Port Mask(rtk_rg_port_idx_t Mask) -> User Port Mask
int32 _rtk_rg_macPortMsk_to_UserPortMsk(uint32 macPortMask ,uint32 *userPortMask)
{
#if defined(CONFIG_RG_FLOW_BASED_PLATFORM)			
	
#if defined(CONFIG_MASTER_WLAN0_ENABLE) && defined(CONFIG_RG_FLOW_NEW_WIFI_MODE)
#if defined(CONFIG_RG_G3_SERIES)
	*userPortMask = macPortMask & ~(RTK_RG_ALL_REAL_MASTER_EXT_PORTMASK|RTK_RG_ALL_REAL_SLAVE_EXT_PORTMASK);
	if(macPortMask & RTK_RG_ALL_REAL_MASTER_EXT_PORTMASK)
	{
		*userPortMask = *userPortMask | RTK_RG_ALL_MASTER_EXT_PORTMASK ;
	}
	if(macPortMask & RTK_RG_ALL_REAL_SLAVE_EXT_PORTMASK)
	{
		*userPortMask = *userPortMask | RTK_RG_ALL_SLAVE_EXT_PORTMASK;
	}
#else	// not CONFIG_RG_G3_SERIES
	if(rg_kernel.disableSlaveWifiRxAcc_and_enableForwardHash)
	{
		*userPortMask = ((1<<RTK_RG_EXT_PORT0)-1)&macPortMask;
		if(macPortMask & ((1<<RTK_RG_EXT_PORT0)|(1<<RTK_RG_EXT_PORT1)|(1<<RTK_RG_EXT_PORT2)|(1<<RTK_RG_EXT_PORT3)|(1<<RTK_RG_EXT_PORT4)))
		{
			*userPortMask = *userPortMask |  (1<<RTK_RG_EXT_PORT0) ;
		}
		if(macPortMask & (1<<RTK_RG_EXT_PORT5))
		{
			*userPortMask = *userPortMask | ( (1<<RTK_RG_EXT_PORT0) | (1<<RTK_RG_EXT_PORT1));
		}
	}
	else
	{
		*userPortMask = ((1<<RTK_RG_EXT_PORT0)-1)&macPortMask;
		if(macPortMask & ((1<<RTK_RG_EXT_PORT0)|(1<<RTK_RG_EXT_PORT1)|(1<<RTK_RG_EXT_PORT2)|(1<<RTK_RG_EXT_PORT3)|(1<<RTK_RG_EXT_PORT4)|(1<<RTK_RG_EXT_PORT5)))
		{
			*userPortMask = *userPortMask | (1<<RTK_RG_EXT_PORT0);
		}
		if(macPortMask & ((1<<RTK_RG_MAC10_EXT_PORT0)|(1<<RTK_RG_MAC10_EXT_PORT1)|(1<<RTK_RG_MAC10_EXT_PORT2)|(1<<RTK_RG_MAC10_EXT_PORT3)|(1<<RTK_RG_MAC10_EXT_PORT4)|(1<<RTK_RG_MAC10_EXT_PORT5)))
		{
			*userPortMask = *userPortMask | (1<<RTK_RG_EXT_PORT1);
		}
	}
#endif	// end CONFIG_RG_G3_SERIES
#else
	*userPortMask=macPortMask;			
#endif	
	
#else
	*userPortMask=macPortMask;
#endif
	return SUCCESS;
}


int32 _rtk_rg_igmpMldQuery_portmask_check_and_limit(rtk_rg_pktHdr_t *pPktHdr, uint32* egress_port_mask ,uint32* wlan0Pmsk){

	//patch for user force control IGMP/MLD query from igmpSnooping forward ports.  
	// *egress_port_mask should bring the original egress portmask, and here will return the check result. If result set to 0x0 means no need to send!
	if ( (pPktHdr != NULL)  && 
		(rg_db.systemGlobal.initParam.igmpSnoopingEnable) && 
		(pPktHdr->ingressLocation==RG_IGR_IGMP_OR_MLD) && //only from igmpSnooping module
		(((pPktHdr->tagif&IGMP_TAGIF)&&(pPktHdr->IGMPType == 0x11/*IGMP query*/)) || ((pPktHdr->tagif&IPV6_MLD_TAGIF)&&(pPktHdr->IGMPv6Type==130/*Multicast Listener Query*/)))
		)
	{	

		extern int32 rtl_getQueryPortMask(uint32 moduleIndex, rtk_rg_pktHdr_t * pPktHdr, uint32 *fwdPortMask, uint32 *fwdMbssidMask);
		uint32 fwdportmask=0x0, fwdmbssidmask=0x0;
		rtl_getQueryPortMask(rg_db.systemGlobal.nicIgmpModuleIndex, pPktHdr, &fwdportmask, &fwdmbssidmask);

		if(egress_port_mask!= NULL)
		{
			uint32 userFwdPortMsk;
			_rtk_rg_macPortMsk_to_UserPortMsk(fwdportmask,&userFwdPortMsk);
			IGMP("IGMP Query egress_port_mask[0x%x] fwdportmask[0x%x] fwdportmask[0x%x], fwdmbssidmask[0x%x]", *egress_port_mask & (userFwdPortMsk), fwdportmask,userFwdPortMsk, fwdmbssidmask);
			*egress_port_mask &= (userFwdPortMsk);
		}
		if(wlan0Pmsk!= NULL)
		{
			*wlan0Pmsk &= fwdmbssidmask;
		}
		
#ifdef CONFIG_DUALBAND_CONCURRENT		
		if(pPktHdr->egressVlanTagif==1 && 
			pPktHdr->egressVlanID==CONFIG_DEFAULT_TO_SLAVE_GMAC_VID &&
			pPktHdr->egressPriority==CONFIG_DEFAULT_TO_SLAVE_GMAC_PRI){
			//check packet to slave wifi
			if((rg_db.systemGlobal.igmpMldQueryPortmask&(1<<RTK_RG_EXT_PORT1))==0x0){
				TRACE("IGMP Query forward to slave wifi is limited");
				if(egress_port_mask!= NULL)
					*egress_port_mask = 0x0;
				if(wlan0Pmsk!= NULL)
					*wlan0Pmsk &= 0x0;
				return SUCCESS;
			}
		}
		else
#endif
		{
			//check packet to other physical port or master wifi
			if(egress_port_mask!= NULL)
			{
				*egress_port_mask &= rg_db.systemGlobal.igmpMldQueryPortmask;//modified the portmask by proc/rg/igmp_report_filter_portmask
				TRACE("IGMP Query forward port limit to 0x%x  igmpMldQueryPortmask[0x%x]",*egress_port_mask,rg_db.systemGlobal.igmpMldQueryPortmask);
			}
			
			return SUCCESS;
		}

	}	

	return SUCCESS;
}


int _rtk_rg_getPortLinkupStatus(void)
{
	// call func from _rtk_rg_initParam_set, all procedure has been protected by spinlock(rgApiLock).
	rtk_rg_portStatusInfo_t portInfo;
	int i = 0;

	if(pf.rtk_rg_portStatus_get != NULL)
	{
		for (i = 0; i< RTK_RG_MAC_PORT_MAX; i++)
		{
			if(RG_INVALID_MAC_PORT(i)) continue;
			if((rg_db.systemGlobal.phyPortStatus & (1<<i)) == 0)	// phisical port is not exist
				continue;		
			pf.rtk_rg_portStatus_get(i, &portInfo);
			rg_db.portLinkupMask |= (portInfo.linkStatus<<i);
		}
		rg_db.portLinkStatusInitDone = TRUE;
	}
	DEBUG("Port linkup status: init done(%s), portmask = 0x%x", rg_db.portLinkStatusInitDone?"O":"X", rg_db.portLinkupMask);
	return RT_ERR_OK;
}


/* Return FAIL: means no needed to do transmition, so skip furthur process*/
int _rtk_rg_egressPortMaskCheck(rtk_rg_pktHdr_t *pPktHdr, unsigned int bcTxMask, unsigned int allDestPortMask)
{
	if (pPktHdr->egressUniPortmask != 0)	// if hit ACL/CF force forward
	{
		unsigned int filterPortMask = 0;
		unsigned int oriUniPortMask = pPktHdr->egressUniPortmask;

		pPktHdr->egressUniPortmask &= ~bcTxMask;
		filterPortMask = pPktHdr->egressUniPortmask & allDestPortMask;
		pPktHdr->egressUniPortmask = oriUniPortMask & (~filterPortMask) & bcTxMask;

		if(pPktHdr->aclDecision.action_uni.uniActionDecision==ACL_UNI_FWD_TO_PORTMASK_ONLY){

			if(!FWD_DECISION_IS_MC(pPktHdr->fwdDecision) )
			{
				pPktHdr->egressUniPortmask &= rg_db.vlan[pPktHdr->internalVlanID].MemberPortmask.bits[0];
				TRACE("Portmask [0x%x] filter by VLAN ID = %d, pmask=0x%x(after filter)",bcTxMask,pPktHdr->internalVlanID,pPktHdr->egressUniPortmask);
			}
		}
		
		if (pPktHdr->egressUniPortmask == 0){		// force forward ports didn't include self, skip transmission because force forward ports will handle Tx by self.
			TRACE("Portmask [0x%x] **STOP** sending packet. Update Egress port mask from 0x%x to 0x%x.", bcTxMask, oriUniPortMask, pPktHdr->egressUniPortmask);
			return FAIL;
		}else
			TRACE("Portmask [0x%x] Update Egress port mask from 0x%x to 0x%x", bcTxMask, oriUniPortMask, pPktHdr->egressUniPortmask);
	} else {	
		// Normal case: SKIP sending if Tx port is not belong to member set
		if(FWD_DECISION_IS_MC(pPktHdr->fwdDecision) ){
			if ((pPktHdr->multicastMacPortMask.portmask & bcTxMask) == 0) {
				TRACE("Portmask [0x%x] **STOP** sending packet.  Multicast, member set = 0x%x", bcTxMask, pPktHdr->multicastMacPortMask.portmask);
				return FAIL;
			}
		}
		else if ( ((rg_db.vlan[pPktHdr->internalVlanID].MemberPortmask.bits[0] & bcTxMask) == 0)) 
		{
			//20161005LUKE: we will not filter packet from protocol stack with txPmsk with vlan filter.
			if(pPktHdr->ingressLocation==RG_IGR_PROTOCOL_STACK && rg_kernel.protocolStackTxPortMask==bcTxMask)return SUCCESS;

			// Not Forcefwd && specific port is not belong to member port.
			TRACE("Portmask [0x%x] **STOP** sending packet. VLAN ID = %d, member set = 0x%x", bcTxMask, pPktHdr->internalVlanID, rg_db.vlan[pPktHdr->internalVlanID].MemberPortmask.bits[0]);
			return FAIL;
		}
	}
	return SUCCESS;
}

int _rtk_rg_egressPacketDoQosRemarkingDecision(rtk_rg_pktHdr_t *pPktHdr, struct sk_buff *skb, struct sk_buff *bcSkb, unsigned int dpMask, unsigned int internalVlanID)
{
	/* MUST decide egress tagif first [pPktHdr->egressVlanTagif] before calling this Qos remarking function */
	unsigned int bcWithDot1pRemarkMask, bcWithoutDot1pRemarkMask;
	unsigned int bcWithoutDscpRemarMask,bcWithDscpRemarByInternalpriMask,bcWithDscpRemarByDscpkMask;
	int dscp_off;
	unsigned char doQosDot1pRemark = FALSE;				// enable 1P remark or not

	if(pPktHdr==NULL){WARNING("pPktHdr NULL Point Error");  return RT_ERR_RG_INVALID_PARAM;}
	pPktHdr->egressDSCPRemarking = DISABLED_DSCP_REMARK;	// enable DSCP remark or not

	// 1.1. Decide: with or without dot1p remarking
	if(pPktHdr!=NULL && pPktHdr->egressVlanTagif){
		_rtk_rg_broadcastWithDot1pRemarkMask_get(dpMask,&bcWithDot1pRemarkMask,&bcWithoutDot1pRemarkMask);
		if(bcWithDot1pRemarkMask != 0x0)
			doQosDot1pRemark = TRUE;
		else if(bcWithoutDot1pRemarkMask != 0x0)
			doQosDot1pRemark = FALSE;
	}else{
		//if pPktHdr is NULL, it doesn't need to do remarking.
		doQosDot1pRemark = FALSE;
	}
	
	// 1.2. Decide: with or without dscp remarking
	_rtk_rg_broadcastWithDscpRemarkMask_get(dpMask,&bcWithoutDscpRemarMask,&bcWithDscpRemarByInternalpriMask,&bcWithDscpRemarByDscpkMask);		
	if (bcWithoutDscpRemarMask != 0x0)
		pPktHdr->egressDSCPRemarking = DISABLED_DSCP_REMARK;
	else if (bcWithDscpRemarByInternalpriMask)
		pPktHdr->egressDSCPRemarking = ENABLED_DSCP_REMARK_AND_SRC_FROM_INTERNALPRI;
	else if (bcWithDscpRemarByDscpkMask)
		pPktHdr->egressDSCPRemarking = ENABLED_DSCP_REMARK_AND_SRC_FROM_DSCP;

	//if (doQosDot1pRemark || doQosDSCPRemark) // mask to always show trace log
	TRACE("Portmask [0x%x] Qos remark: 1P remarking(%s), DSCP remarking(%s)", dpMask, doQosDot1pRemark?"O":"X", pPktHdr->egressDSCPRemarking?"O":"X");
	
	// 2.1. set up dot1p remarking
	if (doQosDot1pRemark)	{
		pPktHdr->egressPriority=rg_db.systemGlobal.qosInternalDecision.qosDot1pPriRemarkByInternalPri[pPktHdr->internalPriority]&0x7;
		TRACE("Dot1P remark by InternalPri[%d] => Dot1P[%d]", pPktHdr->internalPriority, pPktHdr->egressPriority);
	}

	// 2.2. set up dscp remarking
	if(pPktHdr->egressDSCPRemarking && (pPktHdr->tagif&IPV4_TAGIF || pPktHdr->tagif&IPV6_TAGIF)){
		dscp_off = (pPktHdr->pTos)-(skb->data);
		_rtk_rg_dscpRemarkToSkb(pPktHdr->egressDSCPRemarking,pPktHdr,bcSkb,dscp_off);
	}

	//DEBUG("BC Qos remark portmask [0x%x]: [%s] 1P remarking, [%s] DSCP remarking", dpMask, doQosDot1pRemark?"with":"without", doQosDSCPRemark?"with":"without");

	return RT_ERR_RG_OK;
}

int32 _rtk_rg_sendBroadcastToWan(rtk_rg_pktHdr_t *pPktHdr, struct sk_buff *bcSkb, int wanIdx, unsigned int dpMask)
{
	int egressPort;
	uint32 lanInternalVlan = 0;


	//DEBUG("wan port is %d",rg_db.systemGlobal.wanIntfGroup[i].p_intfInfo->storedInfo.wan_intf.wan_intf_conf.wan_port_idx);
	rg_kernel.txDesc.tx_tx_portmask=dpMask;
	
	//decision of VLAN tagging
	lanInternalVlan = pPktHdr->internalVlanID; //back up lan internalVlan to avoid _rtk_rg_interfaceVlanIDPriority() change it to wan interfcae vlan.
	_rtk_rg_interfaceVlanIDPriority(pPktHdr,&rg_db.systemGlobal.interfaceInfo[wanIdx].storedInfo,&rg_kernel.txDesc);
	pPktHdr->internalVlanID = lanInternalVlan;//rowback pPktHdr->internalVlanID to lan interfcae vlan

	_rtk_rg_wanVlanTagged(pPktHdr,rg_db.systemGlobal.interfaceInfo[wanIdx].storedInfo.wan_intf.wan_intf_conf.egress_vlan_tag_on);

	//Qos remarking: Chuck
	egressPort=rg_db.systemGlobal.interfaceInfo[wanIdx].storedInfo.wan_intf.wan_intf_conf.wan_port_idx;
	if(rg_db.systemGlobal.qosInternalDecision.qosDot1pPriRemarkByInternalPriEgressPortEnable[egressPort]==RTK_RG_ENABLED){
		TRACE("QoS dop1p Remarking by port[%d]: internalPri %d => CRI %d",egressPort,pPktHdr->internalPriority,rg_db.systemGlobal.qosInternalDecision.qosDot1pPriRemarkByInternalPri[pPktHdr->internalPriority]);		
		pPktHdr->egressPriority=rg_db.systemGlobal.qosInternalDecision.qosDot1pPriRemarkByInternalPri[pPktHdr->internalPriority];						
	}

	if((pPktHdr->tagif&IPV4_TAGIF || pPktHdr->tagif&IPV6_TAGIF))
		_rtk_rg_qosDscpRemarking(egressPort,pPktHdr,bcSkb);

	_rtk_rg_modifyPacketByACLAction(bcSkb,pPktHdr,rg_db.systemGlobal.interfaceInfo[wanIdx].storedInfo.wan_intf.wan_intf_conf.wan_port_idx);
#if defined(CONFIG_RTL_REPEATER_MODE_SUPPORT) && defined(CONFIG_MASTER_WLAN0_ENABLE)
	if(rg_db.systemGlobal.interfaceInfo[wanIdx].storedInfo.wan_intf.wirelessWan==RG_WWAN_WLAN0_VXD)
	{
		TRACE("Send to Master WIFI vxd intf");
		pPktHdr->egressWlanDevIdx=rg_db.systemGlobal.interfaceInfo[wanIdx].storedInfo.wan_intf.wirelessWan;	//20151210LUKE: keep idx for rate limit
		_rtk_rg_splitJumboSendToMasterWifi(pPktHdr,bcSkb,wlan_vxd_netdev);
	}
	else if(rg_db.systemGlobal.interfaceInfo[wanIdx].storedInfo.wan_intf.wirelessWan==RG_WWAN_WLAN1_VXD)
	{
		TRACE("Send to Slave WIFI vxd intf");
		pPktHdr->egressWlanDevIdx=rg_db.systemGlobal.interfaceInfo[wanIdx].storedInfo.wan_intf.wirelessWan;	//20151210LUKE: keep idx for rate limit
		_rtk_rg_splitJumboSendToMasterWifi(pPktHdr,bcSkb,wlan1_vxd_netdev);
	}
	else
#endif
	{
		//Transfer mirror packet with dedicated VLAN and tagging to each WAN interface
		//TRACE("_rtk_rg_egressPacketSend");
		_rtk_rg_egressPacketSend(bcSkb,pPktHdr);
	}

	return RG_FWDENGINE_RET_CONTINUE;

}


int _rtk_rg_dscpRemarkToSkb(rtk_rg_qosDscpRemarkSrcSelect_t dscpSrc,rtk_rg_pktHdr_t *pPktHdr,struct sk_buff *skb, int dscp_off){
	unsigned char tos;
	if(dscpSrc==ENABLED_DSCP_REMARK_AND_SRC_FROM_INTERNALPRI){
		if(pPktHdr->pTos!=NULL){//packet may not have IP header
			TRACE("DSCP remark by InternalPri[%d] => DSCP[%d]",pPktHdr->internalPriority,rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByInternalPri[pPktHdr->internalPriority]);
			if(pPktHdr->tagif&IPV6_TAGIF)
			{
				//dscp is the MSB 6 bits of traffic class
				tos = rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByInternalPri[pPktHdr->internalPriority]>>0x2;	//dscp MSB 4 bits
				tos |= (*pPktHdr->pTos)&0xf0;		//keep version 4 bits
				*pPktHdr->pTos=tos;
				*(skb->data+dscp_off)=tos;

				tos = (rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByInternalPri[pPktHdr->internalPriority]&0x3)<<0x6;	//dscp LSB 2 bits
				tos |= (*(pPktHdr->pTos+1))&0x3f;		//keep original traffic label LSB 2 bits and flow label MSB 4 bits
				*(skb->data+dscp_off+1)=tos;
			}
			else if(pPktHdr->tagif&IPV4_TAGIF)
			{
				tos = rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByInternalPri[pPktHdr->internalPriority]<<0x2;
				tos |= (*pPktHdr->pTos)&0x3;		//keep 2 bits from LSB
				*(skb->data+dscp_off)=tos; 	//remarking tos of packet
			}

			//record egressDSCP
			pPktHdr->egressDSCP=rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByInternalPri[pPktHdr->internalPriority];
		}

	}else if(dscpSrc==ENABLED_DSCP_REMARK_AND_SRC_FROM_DSCP){
		if(pPktHdr->pTos!=NULL){//packet may not have IP header
			TRACE("DSCP remark by DSCP[%d] => DSCP[%d]",(*(pPktHdr->pTos)>>0x2),rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByDscp[*(pPktHdr->pTos)>>0x2]); 
			if(pPktHdr->tagif&IPV6_TAGIF)
			{
				//dscp is the MSB 6 bits of traffic class
				tos = rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByDscp[*(pPktHdr->pTos)>>0x2]>>0x2;	//dscp MSB 4 bits
				tos |= (*pPktHdr->pTos)&0xf0;		//keep version 4 bits
				*(skb->data+dscp_off)=tos;

				tos = (rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByDscp[*(pPktHdr->pTos)>>0x2]&0x3)<<0x6; //dscp LSB 2 bits
				tos |= (*(pPktHdr->pTos+1))&0x3f;		//keep original traffic label LSB 2 bits and flow label MSB 4 bits
				*(skb->data+dscp_off+1)=tos;
			}
			else if(pPktHdr->tagif&IPV4_TAGIF)
			{
				tos = rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByDscp[*(pPktHdr->pTos)>>0x2]<<0x2;
				tos |= (*pPktHdr->pTos)&0x3;		//keep 2 bits from LSB
				*(skb->data+dscp_off)=tos; 	//remarking tos of packet
			}

			//record egressDSCP
			pPktHdr->egressDSCP=rg_db.systemGlobal.qosInternalDecision.qosDscpRemarkByDscp[*(pPktHdr->pTos)>>0x2];
		}

	}

	return RT_ERR_RG_OK;
}

#if defined(CONFIG_RG_FLOW_BASED_PLATFORM)	
extern rtk_rg_err_code_t _rtk_rg_multicastRecordFlowData(rtk_rg_pktHdr_t *pPktHdr,uint32 dpMask,uint32 allMask);
#endif
int _rtk_rg_egressPacketSend_for_gponDsBcFilterAndRemarking(struct sk_buff *skb, rtk_rg_pktHdr_t *pPktHdr, int toMasterWifiOrCPU)
{
	int i;
	rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t *filterRule;
	unsigned int candidatePortMask;
	unsigned int sendPortMask = 0x0;
	struct sk_buff *cpSkb=NULL;
	struct tx_info tmp_txDesc;

	if(toMasterWifiOrCPU==1)//to wifi
	{
		 candidatePortMask = (1<<RTK_RG_EXT_PORT0);
	}
	else if(toMasterWifiOrCPU==2)//to cpu
	{
		candidatePortMask = RTK_RG_ALL_CPU_PORTMASK;
	}
	else{//to physical ports (not support for slave wifi)
		 candidatePortMask = rg_kernel.txDesc.tx_tx_portmask;
	}
	
	//check rule
	for(i=0;i<MAX_GPON_DS_BC_FILTER_SW_ENTRY_SIZE;i++){

		if(candidatePortMask ==0x0)//no port needs to send
			break;

		//get valid rule
		if(rg_db.systemGlobal.gpon_SW_ds_bc_filter_table_entry[i].valid==RTK_RG_ENABLED)
			filterRule = &rg_db.systemGlobal.gpon_SW_ds_bc_filter_table_entry[i].filterRule;
		else
			continue;

		DEBUG("check rule[%d]",i);

		//pattern check
		if(filterRule->filter_fields & GPON_DS_BC_FILTER_INGRESS_STREAMID_BIT)
		{
			if(filterRule->ingress_stream_id!=pPktHdr->pRxDesc->rx_pon_stream_id){
				DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_INGRESS_STREAMID_BIT unhit",i);
				continue;
			}
		}

		if(filterRule->filter_fields & GPON_DS_BC_FILTER_INGRESS_STAGIf_BIT)
		{
			if(filterRule->ingress_stagIf==1 && (pPktHdr->tagif&SVLAN_TAGIF)==0x0){//must stag
				DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_INGRESS_STAGIf_BIT unhit",i);
				continue;
			}else if(filterRule->ingress_stagIf==0 && (pPktHdr->tagif&SVLAN_TAGIF)){//must un-stag
				DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_INGRESS_STAGIf_BIT unhit",i);
				continue;
			}
		}

		if(filterRule->filter_fields & GPON_DS_BC_FILTER_INGRESS_CTAGIf_BIT)
		{
			if(filterRule->ingress_ctagIf==1 && (pPktHdr->tagif&CVLAN_TAGIF)==0x0){//must ctag
				DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_INGRESS_CTAGIf_BIT unhit",i);
				continue;
			}else if(filterRule->ingress_ctagIf==0 && (pPktHdr->tagif&CVLAN_TAGIF)){//must un-ctag
				DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_INGRESS_CTAGIf_BIT unhit",i);
				continue;
			}

		}

		if(filterRule->filter_fields & GPON_DS_BC_FILTER_INGRESS_SVID_BIT)
		{
			if(filterRule->ingress_stag_svid!=pPktHdr->stagVid){
				DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_INGRESS_SVID_BIT unhit",i);
				continue;
			}
		}

		if(filterRule->filter_fields & GPON_DS_BC_FILTER_INGRESS_CVID_BIT)
		{
			if(filterRule->ingress_ctag_cvid!=pPktHdr->ctagVid){
				DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_INGRESS_SVID_BIT unhit",i);
				continue;
			}
		}
		
		if(filterRule->filter_fields & GPON_DS_BC_FILTER_EGRESS_PORT_BIT){
			if(toMasterWifiOrCPU==0){ //to mac_port 
				if((filterRule->egress_portmask.portmask & rg_kernel.txDesc.tx_tx_portmask)==0x0){
					DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_EGRESS_PORT_BIT unhit",i);
					continue;
				}
			}
			else if(toMasterWifiOrCPU==1){	//to master wifi
				if((filterRule->egress_portmask.portmask & (1<<RTK_RG_EXT_PORT0))==0x0){
					DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_EGRESS_PORT_BIT unhit",i);
					continue;
				}
			}
			else if(toMasterWifiOrCPU==2){ //to PS 
				if((filterRule->egress_portmask.portmask & RTK_RG_ALL_CPU_PORTMASK)==0x0){
					DEBUG("[GPON BC] rule[%d] GPON_DS_BC_FILTER_EGRESS_PORT_BIT unhit",i);
					continue;
				}
			}
		}
		
		DEBUG("[GPON BC] rule[%d] hit!!!",i);
		//rule hit, send to assigned egress port!
		sendPortMask = candidatePortMask & filterRule->egress_portmask.portmask;
		if(sendPortMask){ 
			uint32 ori_egressVlanTagif,ori_egressVlanID,ori_egressPriority,ori_gponDsBcModuleRuleHit;

			if(toMasterWifiOrCPU==0 || toMasterWifiOrCPU==1){//to lan port or to wifi need to copy a new skb to send
				//make another copy for none CF port in this case.
				cpSkb=rtk_rg_skbCopyToPreAllocSkb(skb);
				if(cpSkb==NULL){
					if(cpSkb) _rtk_rg_dev_kfree_skb_any(cpSkb);
					//can not send
#if defined(CONFIG_RG_RTL9600_SERIES)
					if(skb->len>SKB_BUF_SIZE)
						DEBUG("[Drop] GPON BC allocate skb failed!");
					else
#endif
						WARNING("[Drop] GPON BC allocate skb failed!");
					return RG_FWDENGINE_RET_DROP;		
				} 
			}

			//backup tx_desc & pktHdr
			memcpy(&tmp_txDesc,&rg_kernel.txDesc,sizeof(tmp_txDesc));
			//memcpy(&rg_db.systemGlobal.pktHeader_broadcast,pPktHdr,sizeof(rtk_rg_pktHdr_t));
			ori_egressVlanTagif= pPktHdr->egressVlanTagif;
			ori_egressVlanID   = pPktHdr->egressVlanID;
			ori_egressPriority = pPktHdr->egressPriority;
			ori_gponDsBcModuleRuleHit = pPktHdr->gponDsBcModuleRuleHit;

			
			if(filterRule->ctag_action.ctag_decision==RTK_RG_GPON_BC_FORCE_UNATG){
				pPktHdr->egressVlanTagif = 0;
				DEBUG("[GPON BC] rule[%d] Hit! Remove Ctag to portmask 0x%x",i,sendPortMask);
			}else if(filterRule->ctag_action.ctag_decision==RTK_RG_GPON_BC_FORCE_TAGGIN_WITH_CVID){
				pPktHdr->egressVlanTagif = 1;
				pPktHdr->egressVlanID = filterRule->ctag_action.assigned_ctag_cvid;
				DEBUG("[GPON BC] rule[%d] Hit! Add Ctag(%d,not assign) to portmask 0x%x",i,pPktHdr->egressVlanID,sendPortMask);
			}else if(filterRule->ctag_action.ctag_decision==RTK_RG_GPON_BC_FORCE_TAGGIN_WITH_CVID_CPRI){
				pPktHdr->egressVlanTagif = 1;
				pPktHdr->egressVlanID = filterRule->ctag_action.assigned_ctag_cvid;
				pPktHdr->egressPriority = filterRule->ctag_action.assigned_ctag_cpri;			
				DEBUG("[GPON BC] rule[%d] Hit! Add Ctag(%d,%d) to portmask 0x%x",i,pPktHdr->egressVlanID,pPktHdr->egressPriority,sendPortMask);
			}else if(filterRule->ctag_action.ctag_decision==RTK_RG_GPON_BC_FORCE_TRANSPARENT){
				if((pPktHdr->tagif&CVLAN_TAGIF)==0){//transparent as  untag
					pPktHdr->egressVlanTagif = 0;	
					DEBUG("[GPON BC] rule[%d] Hit! Transparent unCtag to portmask 0x%x",i,sendPortMask);
				}else{//transparent as original tag
					pPktHdr->egressVlanTagif = 1;
					pPktHdr->egressVlanID =pPktHdr->ctagVid;
					pPktHdr->egressPriority = pPktHdr->ctagPri;			
					DEBUG("[GPON BC] rule[%d] Hit! Transparent Ctag(%d,%d) to portmask 0x%x",i,pPktHdr->egressVlanID,pPktHdr->egressPriority,sendPortMask);
				}	
			}

			if(toMasterWifiOrCPU==1){
				#ifdef CONFIG_RG_WLAN_HWNAT_ACCELERATION
					rtk_rg_mbssidDev_t intf_idx;
					pPktHdr->gponDsBcModuleRuleHit = 1;
						
					intf_idx=_rtk_master_wlan_mbssid_tx(pPktHdr,cpSkb);

					if(intf_idx==RG_RET_MBSSID_NOT_FOUND)//send failed, free the skb
					{
						TRACE("[Drop] master wifi tx but DMAC is not found in MBSSID table, drop!");
						if(cpSkb) _rtk_rg_dev_kfree_skb_any(cpSkb);
					}
					
					if(intf_idx==RG_RET_MBSSID_FLOOD_ALL_INTF)
					{
						TRACE("Broadcast to master WLAN(flooding)");
					}
					else
					{
						TRACE("Broadcast to master WLAN(intf=%d)",intf_idx);
					}
				#else
					TRACE("No Wifi Acceleration");
				#endif
			}
			else if(toMasterWifiOrCPU==2)//to CPU
			{
				//no need to call _rtk_rg_egressPacketSend
				//and no need to free the skb, because it is not copied.
			}
			else
			{
				rg_kernel.txDesc.tx_tx_portmask= sendPortMask;
				_rtk_rg_egressPacketSend(cpSkb,pPktHdr);
#if defined(CONFIG_RG_FLOW_BASED_PLATFORM)					
				if(FWD_DECISION_IS_MC(pPktHdr->fwdDecision) && rg_db.systemGlobal.hwnat_enable==1)
					_rtk_rg_multicastRecordFlowData(pPktHdr,sendPortMask,pPktHdr->multicastMacPortMask.portmask);					
#endif				
			}
				
			candidatePortMask &= ~(sendPortMask); //remove send ports from candidatePortMask

			if(toMasterWifiOrCPU==0 || toMasterWifiOrCPU==1){//after send packet,  tx_desc & pktHdr need to restore
				//restore tx_desc & pktHdr
				memcpy(&rg_kernel.txDesc,&tmp_txDesc,sizeof(tmp_txDesc));
				pPktHdr->egressVlanTagif = ori_egressVlanTagif;
			   	pPktHdr->egressVlanID = ori_egressVlanID;
			 	pPktHdr->egressPriority = ori_egressPriority;
			 	pPktHdr->gponDsBcModuleRuleHit = ori_gponDsBcModuleRuleHit;				
				//memcpy(pPktHdr,&rg_db.systemGlobal.pktHeader_broadcast,sizeof(rtk_rg_pktHdr_t));
			}
		}

	}


	if(toMasterWifiOrCPU==0 || toMasterWifiOrCPU==1){//to CPU, the SKB is not copied one, so need to free 

		if(candidatePortMask) //some port without hit gponDsBcRules should send as original.
		{
			if(toMasterWifiOrCPU==1){
#ifdef CONFIG_RG_WLAN_HWNAT_ACCELERATION
				rtk_rg_mbssidDev_t intf_idx;
					
				intf_idx=_rtk_master_wlan_mbssid_tx(pPktHdr,skb);

				if(intf_idx==RG_RET_MBSSID_NOT_FOUND)//send failed, free the skb
				{
					TRACE("[Drop] master wifi tx but DMAC is not found in MBSSID table, drop!");
					if(skb) _rtk_rg_dev_kfree_skb_any(skb);
				}
				
				if(intf_idx==RG_RET_MBSSID_FLOOD_ALL_INTF)
				{
					TRACE("Broadcast to master WLAN(flooding)");
				}
				else
				{
					TRACE("Broadcast to master WLAN(intf=%d)",intf_idx);
				}
#else
				TRACE("No Wifi Acceleration");
#endif
			}
			else
			{
				rg_kernel.txDesc.tx_tx_portmask= candidatePortMask;
				_rtk_rg_egressPacketSend(skb,pPktHdr);
#if defined(CONFIG_RG_FLOW_BASED_PLATFORM)				
				if(FWD_DECISION_IS_MC(pPktHdr->fwdDecision) && rg_db.systemGlobal.hwnat_enable==1)
					_rtk_rg_multicastRecordFlowData(pPktHdr,candidatePortMask,pPktHdr->multicastMacPortMask.portmask);				
#endif
			}
		}
		else
		{
			//all candidatePortMask is send, handle(free) the incoming skb as _rtk_rg_egressPacketSend()
			if(skb) _rtk_rg_dev_kfree_skb_any(skb);
		}
	}
	
	//after for loop, the rest ports of candidatePortMask should be discard by no-body send(unhit drop as CF)
	//but return RG_FWDENGINE_RET_CONTINUE to let it goto PS.
	return RG_FWDENGINE_RET_CONTINUE;	

}

int _rtk_rg_BroadcastPacketToLanWithEgressACLModification(int direct, int naptIdx, rtk_rg_pktHdr_t *pPktHdr,struct sk_buff *bcSkb,int l3Modify,int l4Modify, unsigned int bcTxMask, unsigned int allDestPortMask, rtk_rg_port_idx_t egressPort)
{
		/* egressPort==0 means caller is normal case, function will replace it by checking if bcTxMask belongs to CF port 
		     egressPort==-1, means use -1 to do ACL pattern check. It is used in vlan binding case.*/

		struct sk_buff *cpSkb=NULL;
		int aclRet= RG_FWDENGINE_RET_CONTINUE;
		unsigned int allPortmask = RTK_RG_ALL_MAC_PORTMASK;
#if defined(CONFIG_RG_RTL9602C_SERIES) || defined(CONFIG_RG_G3_SERIES)	
		unsigned int cfPortMask = (1<<RTK_RG_PORT_PON);	
#else
		unsigned int cfPortMask = ((1<<RTK_RG_PORT_PON)|(1<<RTK_RG_PORT_RGMII));	
#endif
		unsigned int toCfPortMask = (bcTxMask & cfPortMask);
		unsigned int toOtherPortMask =(bcTxMask & (allPortmask&=(~cfPortMask)) );

		//ACL("mask cfPortMask=0x%x  toCfPortMask=0x%x  toOtherPortMask=0x%x",cfPortMask,toCfPortMask,toOtherPortMask);

		if (egressPort != -1){
			egressPort = (toCfPortMask!= 0x0)?(RTK_RG_PORT_PON):(RTK_RG_PORT_MAX);
		}
		
		// Cheney: abnormal case handler: to prevent use same skb source. (for BC path1)
		if(toCfPortMask!= 0x0 && toOtherPortMask!=0x0)
		{		
			//make a copy for original skb, (and using this copy in this sub-function)		
			cpSkb=rtk_rg_skbCopyToPreAllocSkb(bcSkb);		
			if(cpSkb==NULL){			
				if(cpSkb) _rtk_rg_dev_kfree_skb_any(cpSkb); 
#if defined(CONFIG_RG_RTL9600_SERIES)	
				if(bcSkb->len>SKB_BUF_SIZE)
					DEBUG("[Drop] Fail to get free skb buffer");
				else
#endif		
					WARNING("[Drop] Fail to get free skb buffer");	

				return RG_FWDENGINE_RET_DROP;	
			}	
		}
			
		if(toCfPortMask!=0x0){//send to CF ports only
			rg_kernel.txDesc.tx_tx_portmask=toCfPortMask;

			aclRet = _rtk_rg_modifyPacketByACLAction(bcSkb,pPktHdr,RTK_RG_PORT_PON);
			if(aclRet == RG_FWDENGINE_RET_DROP){
				goto SKIPPACKETSEND;
			}
			
			if (_rtk_rg_egressPortMaskCheck(pPktHdr,bcTxMask, allDestPortMask) == FAIL){
				aclRet = RG_FWDENGINE_RET_DROP;
				goto SKIPPACKETSEND;
			}

			//TRACE("Portmask [0x%x] Send to CF pmsk only",toCfPortMask);
			_rtk_rg_egressPacketSend(bcSkb,pPktHdr);
#if defined(CONFIG_RG_FLOW_BASED_PLATFORM)	
			if(FWD_DECISION_IS_MC(pPktHdr->fwdDecision) && rg_db.systemGlobal.hwnat_enable==1)
				_rtk_rg_multicastRecordFlowData(pPktHdr,toCfPortMask,pPktHdr->multicastMacPortMask.portmask);
#endif

		}
		if(toOtherPortMask!=0x0){//send to none-CF ports only
			if(toCfPortMask!=0x0){// Cheney: abnormal case handler: to prevent use same skb source. (for BC path1)
				// original bcSkb has been transmitted! Use the backup one
				bcSkb = cpSkb;
			}
			rg_kernel.txDesc.tx_tx_portmask=toOtherPortMask;

			aclRet = _rtk_rg_modifyPacketByACLAction(bcSkb,pPktHdr,RTK_RG_PORT_MAX);
			if(aclRet == RG_FWDENGINE_RET_DROP){
				goto SKIPPACKETSEND;
			}
			
			if (_rtk_rg_egressPortMaskCheck(pPktHdr,bcTxMask, allDestPortMask) == FAIL){
				aclRet = RG_FWDENGINE_RET_DROP;
				goto SKIPPACKETSEND;
			}

#if defined(CONFIG_RG_FLOW_BASED_PLATFORM)
			{
				extern rtk_rg_err_code_t _modifyPacketByMulticastDecision(struct sk_buff *skb,rtk_rg_pktHdr_t *pPktHdr,uint32 txPmsk);
				if(_modifyPacketByMulticastDecision(bcSkb,pPktHdr,bcTxMask) == RT_ERR_RG_FAILED)
				{
					aclRet = RG_FWDENGINE_RET_DROP;
					goto SKIPPACKETSEND;
				}
			}
#endif


			//TRACE("Portmask [0x%x] Send to none-CF pmsk only",toOtherPortMask);				
#if defined(CONFIG_RG_FLOW_BASED_PLATFORM) 
			if((rg_db.systemGlobal.gponDsBCModuleEnable && ( (1<<pPktHdr->ingressPort) & (rg_db.systemGlobal.wanPortMask.portmask)) && (pPktHdr->pDmac[0]&0x1 )) )
#else
			if(rg_db.systemGlobal.gponDsBCModuleEnable && (pPktHdr->ingressPort==RTK_RG_PORT_PON) && (((pPktHdr->pDmac[0]&pPktHdr->pDmac[1]&pPktHdr->pDmac[2]&pPktHdr->pDmac[3]&pPktHdr->pDmac[4]&pPktHdr->pDmac[5])==0xff)||(pPktHdr->pDmac[0]==0x01 && pPktHdr->pDmac[1]==0x00 && pPktHdr->pDmac[2]==0x5e)) && (rg_db.systemGlobal.initParam.wanPortGponMode==1))//must be GPON, BC, from PON
#endif
			{
				_rtk_rg_egressPacketSend_for_gponDsBcFilterAndRemarking(bcSkb,pPktHdr,0);
			}
			else
			{
				_rtk_rg_egressPacketSend(bcSkb,pPktHdr);
#if defined(CONFIG_RG_FLOW_BASED_PLATFORM)					
				if(FWD_DECISION_IS_MC(pPktHdr->fwdDecision) && rg_db.systemGlobal.hwnat_enable==1)
					_rtk_rg_multicastRecordFlowData(pPktHdr,toOtherPortMask,pPktHdr->multicastMacPortMask.portmask);				
#endif
			}

		}
SKIPPACKETSEND:
		return aclRet;

		
}

//collect napt info for callback from NAPT and NAPTR table
void _rtk_rg_naptInfoCollectForCallback(int naptOutIdx, rtk_rg_naptInfo_t *naptInfo)
{	
	int naptInIdx=rg_db.naptOut[naptOutIdx].rtk_naptOut.hashIdx;
	
	memset(naptInfo,0,sizeof(rtk_rg_naptInfo_t));	
	naptInfo->naptTuples.is_tcp = rg_db.naptIn[naptInIdx].rtk_naptIn.isTcp;
	naptInfo->naptTuples.local_ip = rg_db.naptIn[naptInIdx].rtk_naptIn.intIp;
	naptInfo->naptTuples.local_port = rg_db.naptIn[naptInIdx].rtk_naptIn.intPort;
#if 0		
	naptInfo->naptTuples.remote_ip = rg_db.naptIn[naptInIdx].remoteIp;
	naptInfo->naptTuples.remote_port = rg_db.naptIn[naptInIdx].remotePort;
#else
	naptInfo->naptTuples.remote_ip = rg_db.naptOut[naptOutIdx].remoteIp;
	naptInfo->naptTuples.remote_port = rg_db.naptOut[naptOutIdx].remotePort;
#endif
	naptInfo->naptTuples.wan_intf_idx = rg_db.nexthop[rg_db.extip[rg_db.naptIn[naptInIdx].rtk_naptIn.extIpIdx].rtk_extip.nhIdx].rtk_nexthop.ifIdx;;
	naptInfo->naptTuples.inbound_pri_valid = rg_db.naptIn[naptInIdx].rtk_naptIn.priValid;
	naptInfo->naptTuples.inbound_priority = rg_db.naptIn[naptInIdx].rtk_naptIn.priId;		
	naptInfo->naptTuples.external_port = rg_db.naptOut[naptOutIdx].extPort;
	naptInfo->naptTuples.outbound_pri_valid = rg_db.naptOut[naptOutIdx].rtk_naptOut.priValid;
	naptInfo->naptTuples.outbound_priority = rg_db.naptOut[naptOutIdx].rtk_naptOut.priValue;
	naptInfo->idleSecs = rg_db.naptOut[naptOutIdx].idleSecs;
	naptInfo->state = rg_db.naptOut[naptOutIdx].state;
	naptInfo->pContext = &rg_db.naptOut[naptOutIdx].pContext;
}


int _rtk_rg_add_l2tp_lcp_reserved_acl_detect(void)
{
	//if exist L2TP Wan, need reserved ACL RTK_RG_ACLANDCF_RESERVED_L2TP_CONTROL_LCP_PACKET_TRAP_AND_ASSIGN_PRIORITY to higher the LCP rx priroity to avoid L2TP disconnection when CPU receiving is busy.
	int i;
	
	for(i=0;i<rg_db.systemGlobal.wanIntfTotalNum;i++){
		if(rg_db.systemGlobal.wanIntfGroup[i].p_wanIntfConf->wan_type==RTK_RG_L2TP){
			DEBUG("Need to add reserved ACL RTK_RG_ACLANDCF_RESERVED_L2TP_CONTROL_LCP_PACKET_TRAP_AND_ASSIGN_PRIORITY");
			return(ENABLE);
		}
	}
	
	DEBUG("NO need to add reserved ACL RTK_RG_ACLANDCF_RESERVED_L2TP_CONTROL_LCP_PACKET_TRAP_AND_ASSIGN_PRIORITY");
	return DISABLE; 
}



int _rtk_rg_add_pppoe_lcp_reserved_acl_detect(void)
{
	//if exist PPPoE Wan, need reserved ACL RTK_RG_ACLANDCF_RESERVED_PPPoE_LCP_PACKET_ASSIGN_PRIORITY to higher the LCP rx priroity to avoid PPPoE disconnection when CPU receiving is busy.
	int i;

#if defined(CONFIG_RG_RTL9607C_SERIES) //9607C testchip need to force trap none-recognized (PPP version=0x1 + Type=0x1 + Code)  PPP packet
	if(rg_db.systemGlobal.internalSupportMask == RTK_RG_INTERNAL_SUPPORT_BIT0)
		return(ENABLE);
#endif
	
	for(i=0;i<rg_db.systemGlobal.wanIntfTotalNum;i++){
		if(rg_db.systemGlobal.wanIntfGroup[i].p_wanIntfConf->wan_type==RTK_RG_PPPoE){
			DEBUG("Need to add reserved ACL RTK_RG_ACLANDCF_RESERVED_PPPoE_LCP_PACKET_ASSIGN_PRIORITY");
			return(ENABLE);
		}
	}
	
	DEBUG("NO need to add reserved ACL RTK_RG_ACLANDCF_RESERVED_PPPoE_LCP_PACKET_ASSIGN_PRIORITY");
	return DISABLE; 
}

#ifdef CONFIG_RG_RTL9602C_SERIES
int _rtk_rg_aclAndCfReservedRuleAdd(rtk_rg_aclAndCf_reserved_type_t rsvType,void * parameter);
int _rtk_rg_aclAndCfReservedRuleDel(rtk_rg_aclAndCf_reserved_type_t rsvType);
//20160418LUKE: if exist DSlite Wan in routing mode, need reserved ACL RTK_RG_ACLANDCF_RESERVED_SYN_PACKET_TRAP to trap TCP SYN packet for MSS clamping.
void _rtk_rg_dslite_routing_reserved_acl_decision(void)
{
	int i;
	for(i=0;i<rg_db.systemGlobal.wanIntfTotalNum;i++){
		if(rg_db.systemGlobal.wanIntfGroup[i].p_wanIntfConf->wan_type==RTK_RG_PPPoE_DSLITE||
			rg_db.systemGlobal.wanIntfGroup[i].p_wanIntfConf->wan_type==RTK_RG_DSLITE){
			if(rg_db.systemGlobal.wanIntfGroup[i].p_intfInfo->p_wanStaticInfo->napt_enable==0){
				DEBUG("Need to add reserved ACL RTK_RG_ACLANDCF_RESERVED_SYN_PACKET_TRAP");
				_rtk_rg_aclAndCfReservedRuleAdd(RTK_RG_ACLANDCF_RESERVED_SYN_PACKET_TRAP, NULL);
				return;
			}
		}
	}
	
	DEBUG("NO need to add reserved ACL RTK_RG_ACLANDCF_RESERVED_SYN_PACKET_TRAP");
	_rtk_rg_aclAndCfReservedRuleDel(RTK_RG_ACLANDCF_RESERVED_SYN_PACKET_TRAP);
	return; 
}
#endif

int32 _rtk_rg_svlMacLookup(rtk_mac_t mac)
{
	int32 l2Idx, search_index;
	uint32 fid=LAN_FID;

	l2Idx = _rtk_rg_hash_mac_fid_efid(mac.octet, fid, 0) << MAX_LUT_HASH_WAY_SHIFT;		//FIXME;current efid is always 0 
	//Check 4-way
	for(search_index=l2Idx; search_index<l2Idx+MAX_LUT_HASH_WAY_SIZE; search_index++)
	{
		if(rg_db.lut[search_index].valid==0) continue;
		if(rg_db.lut[search_index].rtk_lut.entryType==RTK_LUT_L2UC
			&& (rg_db.lut[search_index].rtk_lut.entry.l2UcEntry.flags & RTK_L2_UCAST_FLAG_IVL)==0 //svl
			&& !memcmp(rg_db.lut[search_index].rtk_lut.entry.l2UcEntry.mac.octet, mac.octet, ETHER_ADDR_LEN)
			&& rg_db.lut[search_index].rtk_lut.entry.l2UcEntry.fid==fid)
		{
			return search_index;
		}
	}
	//Check BCAM 
	for(search_index=MAX_LUT_HW_TABLE_SIZE-MAX_LUT_BCAM_TABLE_SIZE; search_index<MAX_LUT_HW_TABLE_SIZE; search_index++)
	{
		if(rg_db.lut[search_index].valid==0) continue;
		if(rg_db.lut[search_index].rtk_lut.entryType==RTK_LUT_L2UC
			&& (rg_db.lut[search_index].rtk_lut.entry.l2UcEntry.flags & RTK_L2_UCAST_FLAG_IVL)==0 //svl
			&& !memcmp(rg_db.lut[search_index].rtk_lut.entry.l2UcEntry.mac.octet, mac.octet, ETHER_ADDR_LEN)
			&& rg_db.lut[search_index].rtk_lut.entry.l2UcEntry.fid==fid)
		{
			return search_index;
		}
	}
	
	return FAIL;
}

#if defined(CONFIG_RG_G3_SERIES)
int32 _rtk_rg_hw_lut_idx_get(uint8 *mac, uint8 isIVL, uint32 vid_fid, uint32 *hwL2Idx)
{
	int32 i=0;
	uint32 l2Idx, ret;
	rtk_l2_addr_table_t l2entry;	

	if(isIVL)
		l2Idx = _rtk_rg_hash_mac_vid_efid(mac, vid_fid, 0) << MAX_LUT_HASH_WAY_SHIFT;
	else
		l2Idx = _rtk_rg_hash_mac_fid_efid(mac, vid_fid, 0) << MAX_LUT_HASH_WAY_SHIFT;

	DEBUG("l2Idx=%d", l2Idx);
	for(i=l2Idx; i<(l2Idx+MAX_LUT_HASH_WAY_SIZE); i++)
	{
		ret=rtk_l2_nextValidEntry_get(&i, &l2entry);
		if(ret==RT_ERR_OK)
		{
			if(!memcmp(l2entry.entry.l2UcEntry.mac.octet, mac, ETHER_ADDR_LEN))
			{
				*hwL2Idx = i;
				return RT_ERR_OK;
			}
		}
	}


	//Search CAM
	for(i=MAX_LUT_HW_TABLE_SIZE-MAX_LUT_BCAM_TABLE_SIZE; i<MAX_LUT_HW_TABLE_SIZE; i++)
	{
		ret=rtk_l2_nextValidEntry_get(&i, &l2entry);
		if(ret==RT_ERR_OK)
		{
			if(!memcmp(l2entry.entry.l2UcEntry.mac.octet, mac, ETHER_ADDR_LEN))
			{
				*hwL2Idx = i;
				return RT_ERR_OK;
			}
		}

	}

	WARNING("Does not find corresponding hw lut entry, mac=%pM isIVL=%d", mac, isIVL);
	return RT_ERR_FAILED;
}
#endif
rtk_rg_err_code_t _rtk_rg_shareMeterCount_get(uint32 index, rtk_rg_meterCount_t **meterCount)
{
	if((index >= PURE_SW_METER_IDX_OFFSET) && (index < (PURE_SW_METER_IDX_OFFSET+PURE_SW_SHAREMETER_TABLE_SIZE)))
	{
		*meterCount = rg_db.systemGlobal.systemMeter.swMeter[index-PURE_SW_METER_IDX_OFFSET].meterCount;
	}
	else if((index >= 0) && (index < (MAX_SHAREMETER_TABLE_SIZE)))
	{
		*meterCount = rg_db.systemGlobal.systemMeter.hwMeter[index].meterCount;
	}
	else
	{
		WARNING("Invalid meter index [%d]", index);
		return RT_ERR_FAILED;
	}
	return RT_ERR_OK;
	
}
#if defined(CONFIG_RG_RTL9607C_SERIES)|| defined(CONFIG_RG_G3_SERIES)

rtk_rg_err_code_t _rtk_rg_funcbasedMeterCount_get(uint32 index, rtk_rg_meter_type_t funcbasedMeterType, rtk_rg_meterCount_t **meterCount)
{
	
	return RT_ERR_OK;
}
#endif
rtk_rg_fwdEngineReturn_t rtk_rg_dropBySwRateLimt_Check(uint32 meterIndex, uint32 byteCount, uint32 packetCount, rtk_rg_meter_type_t funcbasedMeterType)
{
    rtk_rg_enable_t ifgInclude;
    uint32 rate;
	rtk_rate_metet_mode_t meterMode;
	rtk_rg_meterCount_t *meterCount = NULL;
	int32 ret;
#if defined(CONFIG_RG_RTL9607C_SERIES) || defined(CONFIG_RG_G3_SERIES)
	/* care meter mode*/
	if(rg_db.systemGlobal.funbasedMeter_mode == RTK_RG_METERMODE_NOT_INIT)
		return RG_FWDENGINE_RET_CONTINUE;

	else if(rg_db.systemGlobal.funbasedMeter_mode == RTK_RG_METERMODE_SW_INDEX)
	{
		rtk_rg_funcbasedMeterConf_t funcMeter;
							
		funcMeter.type = funcbasedMeterType;
		funcMeter.idx = meterIndex;
		ret = (pf.rtk_rg_funcbasedMeter_get)(&funcMeter);
		if(ret != RT_ERR_OK)
		{
			WARNING("Get meter[%d] rate by rtk_rg_funcbasedMeter_get() failed, ret = 0x%x!", meterIndex, ret);
			return RG_FWDENGINE_RET_CONTINUE;
		}
		
		if(funcMeter.state != RTK_RG_DISABLED)
		{
			rate = funcMeter.rate;
			ifgInclude = funcMeter.ifgInclude;
		}
		else
		{
			return RG_FWDENGINE_RET_CONTINUE; //meter is disabled, forward the packet
		}

		ret = _rtk_rg_funcbasedMeterCount_get(meterIndex, funcbasedMeterType, &meterCount);
		if(ret != RT_ERR_OK)
		{
			WARNING("Get meter[%d] count by _rtk_rg_funcbasedMeterCount_get() failed, ret = 0x%x!", meterIndex, ret);
			return RG_FWDENGINE_RET_CONTINUE;
		}
		
	}
	else//(rg_db.systemGlobal.funbasedMeter_mode == RTK_RG_METERMODE_SW_INDEX)
#endif
	{
		/* not care meter mode */
		ret=(pf.rtk_rg_shareMeter_get)(meterIndex, &rate , &ifgInclude);
		if(ret != RT_ERR_OK)
		{
			WARNING("Get meter[%d] rate by rtk_rg_shareMeter_get() failed, ret = 0x%x!", meterIndex, ret);
			return RG_FWDENGINE_RET_CONTINUE;
		}
		
		ret=(pf.rtk_rg_shareMeterMode_get)(meterIndex, &meterMode);
		if(ret != RT_ERR_OK)
		{
			WARNING("Get meter[%d] rate by rtk_rg_shareMeterMode_get() failed, ret = 0x%x!", meterIndex, ret);
			return RG_FWDENGINE_RET_CONTINUE;
		}

		ret = _rtk_rg_shareMeterCount_get(meterIndex, &meterCount);
		if(ret != RT_ERR_OK)
		{
			WARNING("Get meter[%d] count by _rtk_rg_shareMeterCount_get() failed, ret = 0x%x!", meterIndex, ret);
			return RG_FWDENGINE_RET_CONTINUE;
		}
	}
	
	if(meterCount == NULL)
	{
		TRACE("meter[%d] count is NULL, keep forwarding", meterIndex, ret);
		return RG_FWDENGINE_RET_CONTINUE;
	}
		

	meterCount->byteCount += byteCount;
	meterCount->packetCount += packetCount;
	
	//Check if over rate limit
	if(meterMode == METER_MODE_BIT_RATE)
	{
		if(meterCount->byteCount*8/*bits in time period*/ > (rate*1000/*Kbps*/*RTK_RG_SWRATELIMIT_SECOND/16/*unit:(1/16)sec*/)/*rate limit bits in time period*/)
		{
			TRACE("[Drop] the packet belongs to shareMeter[%d], and over its rate limit", meterIndex);
			return RG_FWDENGINE_RET_DROP;
		}
	}
	else if(meterMode == METER_MODE_PACKET_RATE)
	{
		if(meterCount->packetCount > (rate*RTK_RG_SWRATELIMIT_SECOND/16/*unit:(1/16)sec*/)/*rate limit bits in time period*/)
		{
			TRACE("[Drop] the packet belongs to shareMeter[%d], and over its rate limit", meterIndex);
			return RG_FWDENGINE_RET_DROP;
		}
	}
	else
	{
		WARNING("Check! meter[%d] is not in bit rate mode or packet rate mode");
		return RG_FWDENGINE_RET_CONTINUE;
	}
	TRACE("[Forward] the packet belongs to shareMeter[%d], and NOT over its rate limit");
	return RG_FWDENGINE_RET_CONTINUE;
}

#ifdef TIMER_AGG

void _rtk_rg_del_timer(rtk_rg_timer_t *pRgTimer)
{
	
	rtk_rg_timer_table_t *preTimer,*pTimer,*pMatchTimer;
	preTimer=&rg_kernel.rgSystemTimerTableHead;
	pTimer=preTimer->pNext;
	while(1)
	{
		if(pTimer==NULL) break;
		if(pTimer->pRgTimer==pRgTimer)
		{
			
			if(rg_kernel.debug_level&RTK_RG_DEBUG_LEVEL_TIMER)
			{			
				char fname[KSYM_SYMBOL_LEN];
				sprint_symbol(fname, (POINTER_CAST)pRgTimer->function);
				TIMER("[SysTimer][del_timer][%s]",fname);
			}
			
			pMatchTimer=pTimer;
			preTimer->pNext=pTimer->pNext;
			

			//insert pMatchTimer into head of free list 
			preTimer=&rg_kernel.rgSystemTimerTableFreeHead;
			pTimer=preTimer->pNext;
			
			preTimer->pNext=pMatchTimer;
			pMatchTimer->pNext=pTimer;
			return;
		}				
		preTimer=pTimer;
		pTimer=pTimer->pNext;
		
	}

	

/*
	int i;
	
	for(i=0;i<MAX_RG_TIMER_SIZE;i++)
	{
		if(rg_kernel.rgSystemTimerTable[i].pRgTimer==pRgTimer)
		{
			pRgTimer->expires=jiffies;
			TIMER("[SysTimer][del_timer] idx=%d func=0x%x",i,(u32)pRgTimer->function);
			return;
		}
	}
*/

}

void _rtk_rg_init_timer(rtk_rg_timer_t *pRgTimer)
{
	// do nothing
}

void _rtk_rg_mod_timer(rtk_rg_timer_t *pRgTimer,unsigned long int expires)
{
	
	rtk_rg_timer_table_t *preTimer,*pTimer,*pFreeTimer;
	preTimer=&rg_kernel.rgSystemTimerTableHead;
	pTimer=preTimer->pNext;
	while(1)
	{
		if(pTimer==NULL) break;
		if(pTimer->pRgTimer==pRgTimer)
		{			
			pRgTimer->expires=expires;
			pRgTimer->valid=1;

			if(rg_kernel.debug_level&RTK_RG_DEBUG_LEVEL_TIMER)
			{			
				char fname[KSYM_SYMBOL_LEN];				
				sprint_symbol(fname, (POINTER_CAST)pRgTimer->function);				
				TIMER("[SysTimer][mod_timer][%s] next_action=%ld\n",fname,expires-jiffies);
			}			
			return;
		}				
		preTimer=pTimer;
		pTimer=pTimer->pNext;
		
	}

	//not found in link list. get a free entry
	pFreeTimer=rg_kernel.rgSystemTimerTableFreeHead.pNext;
	if(pFreeTimer==NULL)
	{
		WARNING("Timer entry is not enough!");
		return;
	}
	rg_kernel.rgSystemTimerTableFreeHead.pNext=pFreeTimer->pNext;


	//put this free entry into head of link list	
	preTimer=&rg_kernel.rgSystemTimerTableHead;
	pTimer=preTimer->pNext;
	
	preTimer->pNext=pFreeTimer;
	pFreeTimer->pNext=pTimer;

	if(rg_kernel.debug_level&RTK_RG_DEBUG_LEVEL_TIMER)
	{			
		char fname[KSYM_SYMBOL_LEN];		
		sprint_symbol(fname, (POINTER_CAST)pRgTimer->function);				
		TIMER("[SysTimer][mod_timer][%s][NEW] next_action=%ld\n",fname,expires-jiffies);
	}

	pFreeTimer->pRgTimer=pRgTimer;
	pFreeTimer->pRgTimer->expires=expires;
	pFreeTimer->pRgTimer->valid=1;
	


/*
	int i,freeIdx=-1;
	for(i=0;i<MAX_RG_TIMER_SIZE;i++)
	{		
		if(rg_kernel.rgSystemTimerTable[i].pRgTimer==pRgTimer)
		{
			pRgTimer->expires=expires;
			TIMER("[SysTimer][mod_timer] idx=%d func=0x%x jiffies=0x%lx expires=0x%lx\n",i,(u32)pRgTimer->function,jiffies, expires);
			return;
		}
		else if(rg_kernel.rgSystemTimerTable[i].pRgTimer==NULL)
		{
			freeIdx=i;
		}
		
	}

	//not found, create a new timer entry
	if(freeIdx==-1)
	{
		WARNING("Timer is full!");
	}
	else
	{
		rg_kernel.rgSystemTimerTable[freeIdx].pRgTimer=pRgTimer;
		TIMER("[SysTimer][mod_timer][NEW] idx=%d func=0x%x jiffies=0x%lx expires=0x%lx\n",freeIdx,(u32)pRgTimer->function,jiffies, expires);
		pRgTimer->expires=expires;
	}
*/
	
}

#else

void _rtk_rg_del_timer(struct timer_list *timer)
{
#ifdef __KERNEL__
	if(timer_pending(timer))
		del_timer(timer);
#endif
}

void _rtk_rg_init_timer(struct timer_list *timer)
{
#ifdef __KERNEL__
	init_timer(timer);
#endif
}

void _rtk_rg_mod_timer(struct timer_list *timer, unsigned long expires)
{
#ifdef __KERNEL__
	mod_timer(timer,expires);	
#endif
}


#endif

/* LAN NET INFO declaration string */
const char* rg_lanNet_phone_type[] = {
	"ANDROID",
	"IPHONE",
	"IPAD",
	"WINDOWS PHONE",
	NULL
};

const char* rg_lanNet_computer_type[] = {
	"WINDOWS NT",
	"MACINTOSH",
	"LINUX",
	NULL
};

const char* rg_lanNet_brand[][MAX_LANNET_SUB_BRAND_SIZE] = {
	{"OTHER", NULL},					//RG_BRAND_OTHER
	{"HUAWEI", NULL},					//RG_BRAND_HUAWEI
	{"XIAOMI", NULL},					//RG_BRAND_XIAOMI
	{"MEIZU", NULL},					//RG_BRAND_MEIZU
	{"IPHONE", NULL},					//RG_BRAND_IPHONE
	{"NOKIA", NULL},					//RG_BRAND_NOKIA
	{"SAMSUNG", NULL},					//RG_BRAND_SAMSUNG
	{"SONY", "XPERIA", NULL},			//RG_BRAND_SONY
	{"ERICSSON", NULL},					//RG_BRAND_ERICSSON
	{"MOT", NULL},						//RG_BRAND_MOT
	{"HTC", NULL},						//RG_BRAND_HTC
	{"SGH", NULL},						//RG_BRAND_SGH
	{"LG", NULL},						//RG_BRAND_LG
	{"SHARP", NULL},					//RG_BRAND_SHARP
	{"PHILIPS", NULL},					//RG_BRAND_PHILIPS
	{"PANASONIC", NULL},				//RG_BRAND_PANASONIC
	{"ALCATEL", NULL},					//RG_BRAND_ALCATEL
	{"LENOVO", NULL},					//RG_BARND_LENOVO
	{"OPPO", NULL},						//RG_BARND_OPPO
	{NULL}
};

const char* rg_lanNet_model[][MAX_LANNET_SUB_MODEL_SIZE] = {
	{"OTHER", NULL},					//RG_MODEL_OTHER
	{"HONOR", NULL},					//RG_MODEL_HONOR_NT
	{"P7", NULL},						//RG_MODEL_P7
	{"R7", NULL},						//RG_MODEL_R7
	{"MI 4LTE", NULL},					//RG_MODEL_MI4LTE
	{NULL}
};

const char* rg_lanNet_os[][MAX_LANNET_SUB_OS_SIZE] = {
	{"OTHER", NULL},					//RG_OS_OTHER
	{"WINDOWS NT", NULL},				//RG_OS_WINDOWS_NT
	{"MACINTOSH", "MAC OS X", NULL},	//RG_OS_MACINTOSH
	{"IOS", "IPHONE OS", "IPHONE", NULL},//RG_OS_IPHONE_OS
	{"ANDROID", NULL},					//RG_OS_ANDROID
	{"WINDOWS PHONE", NULL},			//RG_OS_WINDOWS_PHONE
	{"LINUX", NULL},					//RG_OS_LINUX
	{NULL}
};

const char* rg_http_request_cmd[] = {
	"GET",
	"POST",
	"PUT",
	"OPTIONS",
	"HEAD",
	"DELETE",
	"TRACE",
	NULL
};

/* ForcePortal broswer type */
const char* rg_forcePortal_browser_type[] = {
	"FIREFOX", // Firefox
	"SEAMONKEY", // Seamokey
	"CHROME", // Chrome
	"CHROMINUM", // Chrominum
	"SAFARI", // Safari
	"OPERA", // Opera
	"MSIE", // Internet Explorer
	"TRIDENT", // Internet Explorer
	"NEWLIANGXING", // test only
	NULL
};
#else //APOLLO_MODEL CODE
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>

#include <fcntl.h>
#include <unistd.h>

void *rtk_rg_malloc(int NBYTES)
{
	return malloc(NBYTES);
}
void rtk_rg_free(void *APTR){
	free(APTR);
}

#endif


