/* String matching match for iptables
 *
 * (C) 2005 Pablo Neira Ayuso <pablo@eurodev.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_string.h>
#include <linux/textsearch.h>
#include <net/tcp.h>

MODULE_AUTHOR("Pablo Neira Ayuso <pablo@eurodev.net>");
MODULE_DESCRIPTION("Xtables: string-based matching");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_string");
MODULE_ALIAS("ip6t_string");

/* QL 20110629: url filter fail in some special circumstance, we should match more key words. */
#define http_get_len	4
#define http_host_len	6
#define http_refer_len	9
const char * method[] = {
	"GET ",
	"HEAD ",
	"POST ",
	"PUT ",
	"OPTIONS ",
	"DELETE ",
	"TRACE ",
	"CONNECT ",
	0
};

/* Linear string search based on memcmp() */
static char *search_linear (char *needle, char *haystack, int needle_len, int haystack_len)
{
	char *k = haystack + (haystack_len-needle_len);
	char *t = haystack;

	while ( t <= k ) {
		if (memcmp(t, needle, needle_len) == 0)
			return t;
		t++;
	}

	return NULL;
}

/* Backward Linear string search based on memcmp() */
static char *backward_search_linear (char *needle, char *haystack, int needle_len, int haystack_len)
{
	char *t = haystack + haystack_len -needle_len -5;

	if (memcmp(t, needle, needle_len) == 0)
		return t;

	return NULL;
}

#define http_start_len 4
#define http_end_len 9
#define TCP_HEADER_LENTH 20

static int match_HTTP_GET(char **haystack,int *hlen, char *needle, int nlen)
{
    char http_start[]="GET ";
    //char http_end[]="\r\nAccept:";
    /*QL 20110629 start*/
	char http_refer[]="Referer: ";
	char http_host[]="Host: ";
	char *pchkStart=NULL;//point to the check start place
	int chkLen;//check len
	int idx;
	char *pUri;
	int uriLen;
	/*QL 20110629 end */
    char *ptend;

    //alex
    int tcp_datalen;
	char *ptstart;
	int datalen;

    tcp_datalen = (((struct tcphdr*)(*haystack))->doff)*4;
    //point to the HTTP Header
    ptstart=(*haystack)+tcp_datalen;
    datalen=*hlen-tcp_datalen;

	/*QL 20110629 start*/
#if 0
    // Check if the packet is HTTT GET packet.
    if(memcmp(ptstart,http_start,http_start_len)!= 0) {
    	//printk("match_HTTP: Not Find the HTTP GET\n");
    	return 0;
    }


    //Check if the packet match the URL string of the rules
    //if((ptend=search_linear(http_end,*haystack,http_end_len,datalen))== NULL){
    if((ptend=search_linear(needle,ptstart,nlen,datalen))== NULL){
    	//printk("match_HTTP: URL string not match rule\n");
    	return 0;
    }
#else
	for (idx=0; method[idx]; idx++)
	{
		if (memcmp(ptstart, method[idx], strlen(method[idx]))==0) {
			pchkStart = ptstart + strlen(method[idx]);
			break;
		}
	}
	if (!pchkStart)//didn't find method, pass it.
		return 0;

	for (chkLen=0; pchkStart[chkLen]; chkLen++)
	{
		if ((pchkStart[chkLen] == ' ') ||
			((pchkStart[chkLen]=='\r') && (pchkStart[chkLen+1]=='\n')))
			break;
	}
	if ((1==chkLen) && (pchkStart[0]==0x2f))
	{//first request packet, uri is null, then just check host string only.
		if ((pchkStart=strstr(ptstart, http_host)) != NULL) {
			pchkStart = pchkStart + http_host_len;
			for (chkLen=0; pchkStart[chkLen]; chkLen++)
			{
				if ((pchkStart[chkLen] == '\r') && (pchkStart[chkLen+1]=='\n'))
					break;
			}
			if ((ptend=search_linear(needle, pchkStart, nlen, chkLen)) == NULL)
				return 0;
		}
	}
	else
	{//first check referer string, if start with "http://", then just only check it, else check host; if referer string not exists, then check uri.
		pUri = pchkStart;
		uriLen = chkLen;

		if ((pchkStart=strstr(ptstart, http_start)) != NULL) {
			pchkStart=pchkStart + http_get_len;
			for(chkLen=0; pchkStart[chkLen]; chkLen++)
			{
				if ((pchkStart[chkLen] == ' ') ||
					((pchkStart[chkLen]=='\r') && (pchkStart[chkLen+1]=='\n')))
					break;
			}
			if ((ptend=search_linear(needle, pchkStart, nlen, chkLen)) != NULL)
				return 1;
		}
		if ((pchkStart=strstr(ptstart, http_refer)) != NULL) {
			pchkStart = pchkStart + http_refer_len;
			if (memcmp(pchkStart, "http://", strlen("http://")) == 0) {
				for (chkLen=0; pchkStart[chkLen]; chkLen++)
				{
					if ((pchkStart[chkLen] == '\r') && (pchkStart[chkLen+1]=='\n'))
						break;
				}
				if ((ptend=search_linear(needle, pchkStart, nlen, chkLen)) == NULL)
					return 0;
				return 1;
			}
		}

		//go to check host string, if not find, go to check uri.
		if ((pchkStart=strstr(ptstart, http_host)) != NULL) {
			pchkStart = pchkStart + http_host_len;
			for (chkLen=0; pchkStart[chkLen]; chkLen++)
			{
				if ((pchkStart[chkLen] == '\r') && (pchkStart[chkLen+1]=='\n'))
					break;
			}
			if ((ptend=search_linear(needle, pchkStart, nlen, chkLen)) == NULL)
				return 0;
		} else {//only in http 1.0
			if ((ptend=search_linear(needle, pUri, nlen, uriLen)) == NULL)
				return 0;
		}
	}
#endif
	/*QL 20110629 end */

    return 1;

}

static int match_HTTP_GET_URL_ALLOW(char **haystack,int *hlen, char *needle, int nlen)
{
    char http_start[]="GET ";
    //char http_end[]="\r\nAccept:";
    char *ptend;
	/*QL 20110629 start*/
 	char http_refer[]="Referer: ";
	char http_host[]="Host: ";
	char *pchkStart=NULL;//point to the check start place
	int chkLen;//check len
	int idx;
	char *pUri;
	int uriLen;
	/*QL 20110629 end */

    //point to the HTTP Header
    //char *ptstart=(*haystack)+TCP_HEADER_LENTH;
    //int datalen=*hlen-TCP_HEADER_LENTH;
    char *ptstart;
    int datalen;
    //alex
    int tcp_datalen;
    tcp_datalen = (((struct tcphdr*)(*haystack))->doff)*4;
    //point to the HTTP Header
    ptstart=(*haystack)+tcp_datalen;
    datalen=*hlen-tcp_datalen;

	/*QL 20110629 start*/
#if 0
    // Check if the packet is HTTT GET packet.
    if(memcmp(ptstart,http_start,http_start_len)!= 0) {
    	//printk("match_HTTP: Not Find the HTTP GET\n");
    	return 2;
    }


    //Check if the packet match the URL string of the rules
    //if((ptend=search_linear(http_end,*haystack,http_end_len,datalen))== NULL){
    if((ptend=search_linear(needle,ptstart,nlen,datalen))== NULL){
    	//printk("match_HTTP: URL string not match rule\n");
    	return 0;
    }
    //printk("match_HTTP: URL string  match rule\n");
#else
	for (idx=0; method[idx]; idx++)
	{
		if (memcmp(ptstart, method[idx], strlen(method[idx]))==0) {
			pchkStart = ptstart + strlen(method[idx]);
			break;
		}
	}
	if (!pchkStart)//didn't find method
		return 2;

	for (chkLen=0; pchkStart[chkLen]; chkLen++)
	{
		if ((pchkStart[chkLen] == ' ') ||
			((pchkStart[chkLen]=='\r') && (pchkStart[chkLen+1]=='\n')))
			break;
	}

	if ((1==chkLen) && (pchkStart[0]==0x2f))
	{//first request packet, uri is null, then just check host string only.
		if ((pchkStart=strstr(ptstart, http_host)) != NULL) {
			pchkStart = pchkStart + http_host_len;
			for (chkLen=0; pchkStart[chkLen]; chkLen++)
			{
				if ((pchkStart[chkLen] == '\r') && (pchkStart[chkLen+1]=='\n'))
					break;
			}
			if ((ptend=search_linear(needle, pchkStart, nlen, chkLen)) == NULL)
				return 0;
		}
	}
	else
	{//first check referer string, if start with "http://", then just only check it, else check host; if referer string not exists, then check uri.
		pUri = pchkStart;
		uriLen = chkLen;
		if ((pchkStart=strstr(ptstart, http_start)) != NULL) {
			pchkStart=pchkStart + http_get_len;
			for(chkLen=0; pchkStart[chkLen]; chkLen++)
			{
				if ((pchkStart[chkLen] == ' ') ||
					((pchkStart[chkLen]=='\r') && (pchkStart[chkLen+1]=='\n')))
					break;
			}
			if ((ptend=search_linear(needle, pchkStart, nlen, chkLen)) != NULL)
				return 1;
		}
		if ((pchkStart=strstr(ptstart, http_refer)) != NULL) {
			pchkStart = pchkStart + http_refer_len;
			if (memcmp(pchkStart, "http://", strlen("http://")) == 0) {
				for (chkLen=0; pchkStart[chkLen]; chkLen++)
				{
					if ((pchkStart[chkLen] == '\r') && (pchkStart[chkLen+1]=='\n'))
						break;
				}
				if ((ptend=search_linear(needle, pchkStart, nlen, chkLen)) == NULL)
					return 0;
				return 1;
			}
		}

		//go to check host string, if not find, go to check uri.
		if ((pchkStart=strstr(ptstart, http_host)) != NULL) {
			pchkStart = pchkStart + http_host_len;
			for (chkLen=0; pchkStart[chkLen]; chkLen++)
			{
				if ((pchkStart[chkLen] == '\r') && (pchkStart[chkLen+1]=='\n'))
					break;
			}
			if ((ptend=search_linear(needle, pchkStart, nlen, chkLen)) == NULL)
				return 0;
		} else {//only in http 1.0
			if ((ptend=search_linear(needle, pUri, nlen, uriLen)) == NULL)
				return 0;
		}
	}
#endif
	/*QL 20110629 end */

    return 1;

}


#define UDP_HEADER_LENTH 8
static int match_DNS_QUERY(char **haystack,int *hlen, char *needle, int nlen)
{
	char *ptend;

	//point to the DNS Header
	char *ptstart=(*haystack)+UDP_HEADER_LENTH;
	int datalen=*hlen-UDP_HEADER_LENTH;

	//Check if the packet match the URL string of the rules
	//if((ptend=search_linear(needle,ptstart,nlen,datalen))== NULL){
	//if((ptend=search_linear(cmpStr,ptstart,strlen(cmpStr),datalen))== NULL){
	if((ptend=backward_search_linear(needle,ptstart,nlen,datalen))== NULL){
		//printk("match_DNS_QUERY: Domain name string Not match rule\n");
		return 0;
	}

	//printk("match_DNS_QUERY: Domain name string match rule\n");
	return 1;

}


static bool
string_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_string_info *conf = par->matchinfo;
	struct ts_state state;
	bool invert;

	const struct iphdr *ip = ip_hdr(skb);
	struct ipv6hdr *ipv6h = ipv6_hdr(skb);
    u8 nexthdr;
    int tcphoff;
	int hlen, nlen;
	char *needle, *haystack;

	invert = conf->u.v1.flags & XT_STRING_FLAG_INVERT;

    if (skb->protocol == htons(ETH_P_IP))
    {
    	if (!ip)//process ip packet only
    		return 0;
    	nlen = conf->patlen;
    	hlen = ntohs(ip->tot_len) - (ip->ihl*4);
    	if (nlen > hlen)
    		return 0;

        nexthdr = ip->protocol;
    	needle = (char *)&conf->pattern;
    	haystack = (char *)ip + (ip->ihl*4);
    }
    else if (skb->protocol == htons(ETH_P_IPV6))
    {
        __be16 frag_off;
        
        if (!ipv6h)
            return 0;
        
        nexthdr = ipv6h->nexthdr;
        tcphoff = ipv6_skip_exthdr(skb, sizeof(*ipv6h), &nexthdr, &frag_off);
        if (tcphoff < 0)
        {
            //printk("unknown packet, type=%d\n", nexthdr);
            return 0;
        }
        
        //get length
        nlen = conf->patlen;
        hlen = ntohs(ipv6h->payload_len) - tcphoff + sizeof(*ipv6h);
        if (nlen > hlen)
        {
            //printk("small packet found!\n");
            return 0;
        }
        //printk("rule type %s proto=%d needle %s nlen %d\nhaystack:%s", (conf->flagStr==IPT_DOMAIN_STRING)?"DOMAINBLK":"UNK", nexthdr, 
        //      needle, nlen, haystack);
        
        needle = (char *)&conf->pattern;
        haystack = (char *)ipv6h + tcphoff;
    }
    else
        return 0;

	switch (conf->str_flag) {
	case XT_STRING_FLAG_STRING:
	
	return (skb_find_text((struct sk_buff *)skb, conf->from_offset,
			     conf->to_offset, conf->config, &state)
			     != UINT_MAX) ^ invert;
				 
	case XT_STRING_FLAG_URL:
		if (IPPROTO_TCP==nexthdr) {
			return match_HTTP_GET(&haystack, &hlen, needle, nlen) ^ invert;
		}
		break;
	case XT_STRING_FLAG_DOMAIN:
		if (IPPROTO_UDP==nexthdr) {
			return match_DNS_QUERY(&haystack,&hlen,needle,nlen) ^ invert;
		}
		break;
	case XT_STRING_FLAG_URLALW:
		if (IPPROTO_TCP==nexthdr) {
			int ret;
			ret = match_HTTP_GET_URL_ALLOW(&haystack, &hlen, needle, nlen);
			switch (ret) {
			case 0:
				if (!strncmp(needle, "&endofurl&", 10))
					return 1;
				break;
			case 1:
				return ret;
			case 2:
				if (!strncmp(needle, "&endofurl&", 10))
					return 0;
				break;
			}			
		}
		break;
	}
	return false;
}

#define STRING_TEXT_PRIV(m) ((struct xt_string_info *)(m))

static int string_mt_check(const struct xt_mtchk_param *par)
{
	struct xt_string_info *conf = par->matchinfo;
	struct ts_config *ts_conf;
	int flags = TS_AUTOLOAD;

	/* Damn, can't handle this case properly with iptables... */
	if (conf->from_offset > conf->to_offset)
		return -EINVAL;
	if (conf->algo[XT_STRING_MAX_ALGO_NAME_SIZE - 1] != '\0')
		return -EINVAL;
	if (conf->patlen > XT_STRING_MAX_PATTERN_SIZE)
		return -EINVAL;
	if (conf->u.v1.flags &
	    ~(XT_STRING_FLAG_IGNORECASE | XT_STRING_FLAG_INVERT))
		return -EINVAL;
	if ((conf->str_flag<XT_STRING_FLAG_STRING) || \
	    (conf->str_flag>XT_STRING_FLAG_URLALW) || \
		(conf->str_flag==XT_STRING_FLAG_HEX_STRING))
		return -EINVAL;
	if (conf->u.v1.flags & XT_STRING_FLAG_IGNORECASE)
		flags |= TS_IGNORECASE;
	ts_conf = textsearch_prepare(conf->algo, conf->pattern, conf->patlen,
				     GFP_KERNEL, flags);
	if (IS_ERR(ts_conf))
		return PTR_ERR(ts_conf);

	conf->config = ts_conf;
	return 0;
}

static void string_mt_destroy(const struct xt_mtdtor_param *par)
{
	textsearch_destroy(STRING_TEXT_PRIV(par->matchinfo)->config);
}

static struct xt_match xt_string_mt_reg __read_mostly = {
	.name       = "string",
	.revision   = 1,
	.family     = NFPROTO_UNSPEC,
	.checkentry = string_mt_check,
	.match      = string_mt,
	.destroy    = string_mt_destroy,
	.matchsize  = sizeof(struct xt_string_info),
	.me         = THIS_MODULE,
};

static int __init string_mt_init(void)
{
	return xt_register_match(&xt_string_mt_reg);
}

static void __exit string_mt_exit(void)
{
	xt_unregister_match(&xt_string_mt_reg);
}

module_init(string_mt_init);
module_exit(string_mt_exit);
