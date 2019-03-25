/*
 * Copyright (c) 2013 Nicira, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/in.h>
#include <linux/if_arp.h>
#include <linux/mroute.h>
#include <linux/init.h>
#include <linux/in6.h>
#include <linux/inetdevice.h>
#include <linux/netfilter_ipv4.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>

#include <net/ip.h>
#include <net/icmp.h>
#include <net/protocol.h>
#include <net/ip_tunnels.h>
#include <net/arp.h>
#include <net/checksum.h>
#include <net/dsfield.h>
#include <net/inet_ecn.h>
#include <net/xfrm.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>
#include <net/rtnetlink.h>

#if 0//defined(CONFIG_RTL867X_IPTABLES_FAST_PATH)
static unsigned short iphdr_id=0;

#if 0//move ipip_sanity_check to fp_common.c
/*
 * Description: pskb is a pointer of struct sk_buff, pskb->dst point to the new rtable, while dst point to the old rtable.
 */
int ipip_sanity_check(void *pskb, void *dst/* struct dst_entry * */)
{
	struct sk_buff *skb = (struct sk_buff *)pskb;
	struct dst_entry *rt = (struct dst_entry *)dst;
	struct net_device *tdev;			/* Device to other host */
	struct iphdr  *old_iph = (struct iphdr *)skb->transport_header;
	struct iphdr  *tiph = ip_hdr(skb);
	int    mtu;

	tdev = skb_dst(skb)->dev;

	if (tdev == skb->dev) {
		goto tx_error;
	}

	if (tiph->frag_off)
		mtu = dst_mtu(skb_dst(skb)) - sizeof(struct iphdr);
	else
		mtu = rt ? dst_mtu(rt) : skb->dev->mtu;

	if (mtu < 68) {
		goto tx_error;
	}
	if (rt)
		rt->ops->update_pmtu(rt, mtu);

	if ((old_iph->frag_off&htons(IP_DF)) && mtu < ntohs(old_iph->tot_len)) {
		goto tx_error;
	}

	dst_release(rt);
	
	return 1;

tx_error:
	printk("%s error.\n", __func__);
	dst_release(skb_dst(skb));
	skb_dst_set(skb, (struct dst_entry *)dst);
	
	return 0;
}
#endif

int ipip_up_fastpath(struct sk_buff *skb)
{
	struct ip_tunnel *tunnel = netdev_priv(skb->dev);
	struct net_device_stats *stats = &tunnel->dev->stats;
	struct iphdr  *tiph = &tunnel->parms.iph;
	u8	   tos = tunnel->parms.iph.tos;
	__be16 df = tiph->frag_off;
	struct rtable *rt;				/* Route to the other host */
	struct net_device *tdev;			/* Device to other host */
	struct iphdr  *old_iph = ip_hdr(skb);
	struct iphdr  *iph; 		/* Our new IP header */
	unsigned int max_headroom;		/* The extra header space needed */
	//__be32 dst = tiph->daddr;
	int    mtu;

	//printk("%s enter.\n", __func__);

	if (tos&1)
		tos = old_iph->tos;

	/*
	 * QL, tunnel dest address must be assigned through ip tunnel cmd, so don't need to check dst here
	 */
#if 0
	if (!dst) {
		/* NBMA tunnel */
		if ((rt = skb->rtable) == NULL) {
			stats->tx_fifo_errors++;
			goto tx_error;
		}
		if ((dst = rt->rt_gateway) == 0)
			goto tx_error_icmp;
	}
#endif

	df |= (old_iph->frag_off&htons(IP_DF));

	if (tunnel->err_count > 0) {
		if (time_before(jiffies,
				tunnel->err_time + IPTUNNEL_ERR_TIMEO)) {
			tunnel->err_count--;
			dst_link_failure(skb);
		} else
			tunnel->err_count = 0;
	}

	/*
	 * Okay, now see if we can stuff it in the buffer as-is.
	 */
	//max_headroom = (LL_RESERVED_SPACE(tdev)+sizeof(struct iphdr));
	max_headroom = (14 + sizeof(struct iphdr));
	
	if (skb_headroom(skb) < max_headroom || skb_shared(skb) ||
		(skb_cloned(skb) && !skb_clone_writable(skb, 0))) {
		struct sk_buff *new_skb = skb_realloc_headroom(skb, max_headroom);
		if (!new_skb) {
			stats->tx_dropped++;
			dev_kfree_skb(skb);
			return 0;
		}
		dev_kfree_skb(skb);
		skb = new_skb;
		old_iph = ip_hdr(skb);
	}

	skb->transport_header = skb->network_header;
	skb_push(skb, sizeof(struct iphdr));
	skb_reset_network_header(skb);
	memset(&(IPCB(skb)->opt), 0, sizeof(IPCB(skb)->opt));
	IPCB(skb)->flags &= ~(IPSKB_XFRM_TUNNEL_SIZE | IPSKB_XFRM_TRANSFORMED |
				  IPSKB_REROUTED);

	/*
	 *	Push down and install the IPIP header.
	 */

	iph 			=	ip_hdr(skb);
	iph->version	=	4;
	iph->ihl		=	sizeof(struct iphdr)>>2;
	iph->id			=	++iphdr_id;
	iph->frag_off	=	df;
	iph->protocol	=	IPPROTO_IPIP;
	iph->tos		=	INET_ECN_encapsulate(tos, old_iph->tos);
	iph->tot_len	=	htons(skb->len);
	iph->daddr		=	tiph->daddr;
	iph->saddr		=	tiph->saddr;

	if ((iph->ttl = tiph->ttl) == 0)
		iph->ttl	=	old_iph->ttl;

	ip_send_check(iph);
	
	if (FastPath_Enter(skb) != 1) {
		struct flowi fl = { .oif = tunnel->parms.link,
					.nl_u = { .ip4_u =
						  { .daddr = tiph->daddr,
						.saddr = tiph->saddr,
						.tos = RT_TOS(tos) } },
					.proto = IPPROTO_IPIP };
		if (ip_route_output_key(dev_net(skb->dev), &rt, &fl)) {
			stats->tx_carrier_errors++;
			goto tx_error_icmp;
		}
	
		tdev = rt->u.dst.dev;

		if (tdev == skb->dev) {
			ip_rt_put(rt);
			stats->collisions++;
			goto tx_error;
		}

		if (tiph->frag_off)
			mtu = dst_mtu(&rt->u.dst) - sizeof(struct iphdr);
		else
			mtu = skb_dst(skb) ? dst_mtu(skb_dst(skb)) : skb->dev->mtu;

		if (mtu < 68) {
			stats->collisions++;
			ip_rt_put(rt);
			goto tx_error;
		}
		if (skb_dst(skb))
			skb_dst(skb)->ops->update_pmtu(skb_dst(skb), mtu);

		if ((old_iph->frag_off&htons(IP_DF)) && mtu < ntohs(old_iph->tot_len)) {
			skb_pull(skb, sizeof(struct iphdr));
			skb_reset_network_header(skb);
			icmp_send(skb, ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED, htonl(mtu));
			ip_rt_put(rt);
			goto tx_error;
		}

		dst_release(skb_dst(skb));
		skb_dst_set(skb, &rt->dst)
	
		IPTUNNEL_XMIT();
		iphdr_id = iph->id;
	}
	//else
	//	printk("%s go fastpath.\n", __func__);

	return 0;

tx_error_icmp:
	dst_link_failure(skb);
tx_error:
	printk("%s error.\n", __func__);
	stats->tx_errors++;
	dev_kfree_skb(skb);
	return 0;
}

int ipip_down_fastpath(struct sk_buff *skb)
{
	struct net *net = dev_net(skb->dev);
	struct ip_tunnel_net *itn = net_generic(net, ipip_net_id);
	struct ip_tunnel *tunnel;
	const struct iphdr *iph = ip_hdr(skb);
	struct net_device *dev;
	unsigned int iph_len;


	read_lock(&ipip_lock);
	if ((tunnel = ip_tunnel_lookup(itn, skb->dev->ifindex, TUNNEL_NO_KEY,
			     iph->saddr, iph->daddr, 0)) != NULL) {
		if (!xfrm4_policy_check(NULL, XFRM_POLICY_IN, skb)) {
			read_unlock(&ipip_lock);
			kfree_skb(skb);
			return 0;
		}

		secpath_reset(skb);

		iph_len = iph->ihl<<2;
		skb_pull(skb, iph_len);
		skb_reset_network_header(skb);
		skb_set_mac_header(skb, -ETH_HLEN);

		dev = skb->dev;//backup original dev
		skb->dev = tunnel->dev;

		ipip_ecn_decapsulate(iph, skb);
		
		if (FastPath_Enter(skb) != 1) {
			skb_push(skb, iph_len);
			skb_reset_network_header(skb);
			skb_set_mac_header(skb, -ETH_HLEN);
			
			skb->dev = dev;
			
			goto FAIL;
		}

		tunnel->dev->stats.rx_packets++;
		tunnel->dev->stats.rx_bytes += skb->len;
		
		read_unlock(&ipip_lock);
		return 1;
	}

FAIL:
	read_unlock(&ipip_lock);

	return -1;
}

#endif //end of CONFIG_RTL867X_IPTABLES_FAST_PATH

int iptunnel_xmit(struct sock *sk, struct rtable *rt, struct sk_buff *skb,
		  __be32 src, __be32 dst, __u8 proto,
		  __u8 tos, __u8 ttl, __be16 df, bool xnet)
{
	int pkt_len = skb->len;
	struct iphdr *iph;
	int err;

	skb_scrub_packet(skb, xnet);

	skb_clear_hash(skb);
	skb_dst_set(skb, &rt->dst);
	memset(IPCB(skb), 0, sizeof(*IPCB(skb)));

	/* Push down and install the IP header. */
	skb_push(skb, sizeof(struct iphdr));
	skb_reset_network_header(skb);

	iph = ip_hdr(skb);

	iph->version	=	4;
	iph->ihl	=	sizeof(struct iphdr) >> 2;
	iph->frag_off	=	df;
	iph->protocol	=	proto;
	iph->tos	=	tos;
	iph->daddr	=	dst;
	iph->saddr	=	src;
	iph->ttl	=	ttl;
	__ip_select_ident(iph, skb_shinfo(skb)->gso_segs ?: 1);

#if 0//defined(CONFIG_RTL867X_IPTABLES_FAST_PATH)
	iphdr_id = iph->id;
#endif

	err = ip_local_out_sk(sk, skb);
	if (unlikely(net_xmit_eval(err)))
		pkt_len = 0;
	return pkt_len;
}
EXPORT_SYMBOL_GPL(iptunnel_xmit);

int iptunnel_pull_header(struct sk_buff *skb, int hdr_len, __be16 inner_proto)
{
	if (unlikely(!pskb_may_pull(skb, hdr_len)))
		return -ENOMEM;

	skb_pull_rcsum(skb, hdr_len);

	if (inner_proto == htons(ETH_P_TEB)) {
		struct ethhdr *eh;

		if (unlikely(!pskb_may_pull(skb, ETH_HLEN)))
			return -ENOMEM;

		eh = (struct ethhdr *)skb->data;
		if (likely(ntohs(eh->h_proto) >= ETH_P_802_3_MIN))
			skb->protocol = eh->h_proto;
		else
			skb->protocol = htons(ETH_P_802_2);

	} else {
		skb->protocol = inner_proto;
	}

	nf_reset(skb);
	secpath_reset(skb);
	skb_clear_hash_if_not_l4(skb);
	skb_dst_drop(skb);
	skb->vlan_tci = 0;
	skb_set_queue_mapping(skb, 0);
	skb->pkt_type = PACKET_HOST;
	return 0;
}
EXPORT_SYMBOL_GPL(iptunnel_pull_header);

struct sk_buff *iptunnel_handle_offloads(struct sk_buff *skb,
					 bool csum_help,
					 int gso_type_mask)
{
	int err;

	if (likely(!skb->encapsulation)) {
		skb_reset_inner_headers(skb);
		skb->encapsulation = 1;
	}

	if (skb_is_gso(skb)) {
		err = skb_unclone(skb, GFP_ATOMIC);
		if (unlikely(err))
			goto error;
		skb_shinfo(skb)->gso_type |= gso_type_mask;
		return skb;
	}

	/* If packet is not gso and we are resolving any partial checksum,
	 * clear encapsulation flag. This allows setting CHECKSUM_PARTIAL
	 * on the outer header without confusing devices that implement
	 * NETIF_F_IP_CSUM with encapsulation.
	 */
	if (csum_help)
		skb->encapsulation = 0;

	if (skb->ip_summed == CHECKSUM_PARTIAL && csum_help) {
		err = skb_checksum_help(skb);
		if (unlikely(err))
			goto error;
	} else if (skb->ip_summed != CHECKSUM_PARTIAL)
		skb->ip_summed = CHECKSUM_NONE;

	return skb;
error:
	kfree_skb(skb);
	return ERR_PTR(err);
}
EXPORT_SYMBOL_GPL(iptunnel_handle_offloads);

/* Often modified stats are per cpu, other are shared (netdev->stats) */
struct rtnl_link_stats64 *ip_tunnel_get_stats64(struct net_device *dev,
						struct rtnl_link_stats64 *tot)
{
	int i;

	for_each_possible_cpu(i) {
		const struct pcpu_sw_netstats *tstats =
						   per_cpu_ptr(dev->tstats, i);
		u64 rx_packets, rx_bytes, tx_packets, tx_bytes;
		unsigned int start;

		do {
			start = u64_stats_fetch_begin_irq(&tstats->syncp);
			rx_packets = tstats->rx_packets;
			tx_packets = tstats->tx_packets;
			rx_bytes = tstats->rx_bytes;
			tx_bytes = tstats->tx_bytes;
		} while (u64_stats_fetch_retry_irq(&tstats->syncp, start));

		tot->rx_packets += rx_packets;
		tot->tx_packets += tx_packets;
		tot->rx_bytes   += rx_bytes;
		tot->tx_bytes   += tx_bytes;
	}

	tot->multicast = dev->stats.multicast;

	tot->rx_crc_errors = dev->stats.rx_crc_errors;
	tot->rx_fifo_errors = dev->stats.rx_fifo_errors;
	tot->rx_length_errors = dev->stats.rx_length_errors;
	tot->rx_frame_errors = dev->stats.rx_frame_errors;
	tot->rx_errors = dev->stats.rx_errors;

	tot->tx_fifo_errors = dev->stats.tx_fifo_errors;
	tot->tx_carrier_errors = dev->stats.tx_carrier_errors;
	tot->tx_dropped = dev->stats.tx_dropped;
	tot->tx_aborted_errors = dev->stats.tx_aborted_errors;
	tot->tx_errors = dev->stats.tx_errors;

	tot->collisions  = dev->stats.collisions;

	return tot;
}
EXPORT_SYMBOL_GPL(ip_tunnel_get_stats64);
