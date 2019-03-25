/*	
 *	re_privskb.h
*/

#ifndef _RE_PRIVSKB_RG_H_
#define _RE_PRIVSKB_RG_H_
#include <linux/skbuff.h>

#define MBUF_LEN_RG        (13312+2)		//IXIA max packet size.

#define CROSS_LAN_MBUF_LEN_RG		(MBUF_LEN_RG+16)
//used to make the start of shared_info is cache line aligment.
#define SHARED_INFO_PAD		SMP_CACHE_BYTES


#define MAX_ETH_SKB_NUM_RG 128//(RE8670_RX_RING_SIZE) + (2573)


#define ETH_PRIV_SKB_PROC	1


void init_priv_eth_skb_buf_rg(void);
struct sk_buff *dev_alloc_skb_priv_eth_rg(unsigned int size);

#endif /*_RE_PRIVSKB_H_*/
