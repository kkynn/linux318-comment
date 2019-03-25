#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <net/dst.h>
#include <net/flow.h>
#include <net/ipv6.h>
#include <net/route.h>
#include <net/tcp.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv6/ip6_tables.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_tcpudp.h>
#include <linux/netfilter/xt_tcpmac.h>

MODULE_DESCRIPTION("Xtables: TCP Terminate MAC adjustment");
MODULE_ALIAS("ipt_TCPMAC");
MODULE_ALIAS("ip6t_TCPMAC");


static inline unsigned int
optlen(const u_int8_t *opt, unsigned int offset)
{
	/* Beware zero-length options: make finite progress */
	if (opt[offset] <= TCPOPT_NOP || opt[offset+1] == 0)
		return 1;
	else
		return opt[offset+1];
}

static int
tcpmac_mangle_packet(struct sk_buff *skb,
		     const struct xt_tcpmac_info *info,
		     unsigned int tcphoff)
{
	struct tcphdr *tcph;
	unsigned int tcplen, i, opttotallen=0;
	__be16 oldval;
	u8 newmacinfo[14];
	u8 *opt;

	if (!skb_make_writable(skb, skb->len))
		return -1;

	tcplen = skb->len - tcphoff;
	tcph = (struct tcphdr *)(skb_network_header(skb) + tcphoff);

	memcpy(newmacinfo, info->macinfo, 14);

	opt = (u_int8_t *)tcph;
	for (i = sizeof(struct tcphdr); i < tcph->doff*4; i += optlen(opt, i)) {
		if (opt[i] == TCPOPT_TERMAC&& tcph->doff*4 - i >= TCPOLEN_TERMAC&&
		    opt[i+1] == TCPOLEN_TERMAC) {
			u8 oldmacinfo[14];

			memcpy(oldmacinfo, &(opt[i+2]), 14);

			memcpy(&(opt[i+2]), newmacinfo, 14);

			inet_proto_csum_replace4(&tcph->check, skb,
							 *(__be32 *)&(oldmacinfo[0]), *(__be32 *)&(newmacinfo[0]),
							 0);
			inet_proto_csum_replace4(&tcph->check, skb,
							 *(__be32 *)&(oldmacinfo[4]), *(__be32 *)&(newmacinfo[4]),
							 0);
			inet_proto_csum_replace4(&tcph->check, skb,
							 *(__be32 *)&(oldmacinfo[8]), *(__be32 *)&(newmacinfo[8]),
							 0);
			inet_proto_csum_replace2(&tcph->check, skb,
							 *(__be16 *)&(oldmacinfo[12]), *(__be32 *)&(newmacinfo[12]),
							 0);

			return 0;
		}
	}

	opttotallen = i - sizeof(struct tcphdr);
	/*
	 * tcp Option not found ?! add it..
	 */
	if (skb_tailroom(skb) < TCPOLEN_TERMAC) {
		if (pskb_expand_head(skb, 0,
				     TCPOLEN_TERMAC- skb_tailroom(skb),
				     GFP_ATOMIC))
			return -1;
		tcph = (struct tcphdr *)(skb_network_header(skb) + tcphoff);
	}

	skb_put(skb, TCPOLEN_TERMAC);

	opt = (u_int8_t *)tcph + sizeof(struct tcphdr)+opttotallen;

	inet_proto_csum_replace2(&tcph->check, skb,
				 htons(tcplen), htons(tcplen + TCPOLEN_TERMAC), 1);
	opt[0] = TCPOPT_TERMAC;
	opt[1] = TCPOLEN_TERMAC;
	memcpy(&(opt[2]), newmacinfo, 14);

	inet_proto_csum_replace4(&tcph->check, skb, 0, *(__be32 *)&(opt[0]), 0);
	inet_proto_csum_replace4(&tcph->check, skb, 0, *(__be32 *)&(opt[4]), 0);
	inet_proto_csum_replace4(&tcph->check, skb, 0, *(__be32 *)&(opt[8]), 0);
	inet_proto_csum_replace4(&tcph->check, skb, 0, *(__be32 *)&(opt[12]), 0);

	oldval = ((__be16 *)tcph)[6];
	tcph->doff += TCPOLEN_TERMAC/4;
	inet_proto_csum_replace2(&tcph->check, skb,
				 oldval, ((__be16 *)tcph)[6], 0);
	return TCPOLEN_TERMAC;
}

static unsigned int
tcpmac_tg4(struct sk_buff *skb, const struct xt_action_param *par)
{
	struct iphdr *iph = ip_hdr(skb);
	__be16 newlen;
	int ret;
	ret = tcpmac_mangle_packet(skb, par->targinfo, iph->ihl * 4);
	if (ret < 0)
		return NF_DROP;
	if (ret > 0) {
		iph = ip_hdr(skb);
		newlen = htons(ntohs(iph->tot_len) + ret);
		csum_replace2(&iph->check, iph->tot_len, newlen);
		iph->tot_len = newlen;
	}
	return XT_CONTINUE;
}

#if defined(CONFIG_IP6_NF_IPTABLES) || defined(CONFIG_IP6_NF_IPTABLES_MODULE)
static unsigned int
tcpmac_tg6(struct sk_buff *skb, const struct xt_action_param *par)
{
	struct ipv6hdr *ipv6h = ipv6_hdr(skb);
	u8 nexthdr;
	int tcphoff;
	int ret;
	__be16 frag_off;

	nexthdr = ipv6h->nexthdr;
	tcphoff = ipv6_skip_exthdr(skb, sizeof(*ipv6h), &nexthdr, &frag_off);
	if (tcphoff < 0)
		return NF_DROP;
	ret = tcpmac_mangle_packet(skb, par->targinfo, tcphoff);
	if (ret < 0)
		return NF_DROP;
	if (ret > 0) {
		ipv6h = ipv6_hdr(skb);
		ipv6h->payload_len = htons(ntohs(ipv6h->payload_len) + ret);
	}
	return XT_CONTINUE;
}
#endif

#define TH_SYN 0x02

/* Must specify -p tcp --syn */
static inline bool find_syn_match(const struct xt_entry_match *m)
{
	const struct xt_tcp *tcpinfo = (const struct xt_tcp *)m->data;
	if (strcmp(m->u.kernel.match->name, "tcp") == 0 &&
	    tcpinfo->flg_cmp & TH_SYN &&
	    !(tcpinfo->invflags & XT_TCP_INV_FLAGS))
		return true;
	return false;
}


static int tcpmac_tg4_check(const struct xt_tgchk_param *par)
{
	const struct xt_tcpmss_info *info = par->targinfo;
	const struct ipt_entry *e = par->entryinfo;
	const struct xt_entry_match *ematch;

	xt_ematch_foreach(ematch, e)
		if (find_syn_match(ematch))
			return 0;
	pr_info("Only works on TCP SYN packets\n");
	return -EINVAL;
}

#if defined(CONFIG_IP6_NF_IPTABLES) || defined(CONFIG_IP6_NF_IPTABLES_MODULE)
static int tcpmac_tg6_check(const struct xt_tgchk_param *par)
{
	const struct xt_tcpmss_info *info = par->targinfo;
	const struct ip6t_entry *e = par->entryinfo;
	const struct xt_entry_match *ematch;
	
	xt_ematch_foreach(ematch, e)
		if (find_syn_match(ematch))
			return 0;
	pr_info("Only works on TCP SYN packets\n");
	return -EINVAL;
}
#endif

static struct xt_target tcpmac_tg_reg[] __read_mostly = {
	{
		.family		= NFPROTO_IPV4,
		.name		= "TCPTERMAC",
		.checkentry	= tcpmac_tg4_check,
		.target		= tcpmac_tg4,
		.targetsize	= sizeof(struct xt_tcpmac_info),
		.proto		= IPPROTO_TCP,
		.me		= THIS_MODULE,
	},
#if defined(CONFIG_IP6_NF_IPTABLES) || defined(CONFIG_IP6_NF_IPTABLES_MODULE)
	{
		.family		= NFPROTO_IPV6,
		.name		= "TCPTERMAC",
		.checkentry	= tcpmac_tg6_check,
		.target		= tcpmac_tg6,
		.targetsize	= sizeof(struct xt_tcpmac_info),
		.proto		= IPPROTO_TCP,
		.me		= THIS_MODULE,
	},
#endif
};

static int __init tcpmac_tg_init(void)
{
	return xt_register_targets(tcpmac_tg_reg, ARRAY_SIZE(tcpmac_tg_reg));
}

static void __exit tcpmac_tg_exit(void)
{
	xt_unregister_targets(tcpmac_tg_reg, ARRAY_SIZE(tcpmac_tg_reg));
}

module_init(tcpmac_tg_init);
module_exit(tcpmac_tg_exit);
