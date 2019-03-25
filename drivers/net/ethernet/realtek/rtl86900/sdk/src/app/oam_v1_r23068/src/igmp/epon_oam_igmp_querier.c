#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <linux/sockios.h>

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>

#include <sys_portview.h>
#include <sys_portmask.h>

#include "epon_oam_err.h"
#include "epon_oam_igmp_db.h"
#include "epon_oam_igmp_util.h"
#include <epon_oam_igmp_querier.h>
#include "ctc_mc.h"

/*
* Symbol Definition
*/
#define IGMP_QUERY_PDU_LEN  46 /* MAC(12 + 4+2) + IP(20) + IGMP(8) */
#define IGMP_QUERY_V3_PDU_LEN  50 /* MAC(12 +4+2) + IP(24) + IGMP(8) */

#define MLD_QUERY_V1_PDU_LEN  90
#define MLD_QUERY_V2_PDU_LEN  94

typedef enum { IP_ADDR, DST_IP_ADDR, SUBNET_MASK, DEFAULT_GATEWAY, HW_ADDR } ADDR_T;

igmp_querier_db_t querier_db[MULTICAST_TYPE_END];
uint32            sys_ip;
uint32            sys_ip6[4];
sys_mac_t         sys_mac; /* system MAC address */

static int isAddrLarger(uint32 *ip1, uint32 *ip2)
{
	if (ip1[0] > ip2[0])
		return 1;
	else if (ip1[0] < ip2[0])
		return 0;

	if (ip1[1] > ip2[1])
		return 1;
	else if (ip1[1] < ip2[1])
		return 0;
	
	if (ip1[2] > ip2[2])
		return 1;
	else if (ip1[2] < ip2[2])
		return 0;
	
	if (ip1[3] > ip2[3])
		return 1;
	else if (ip1[3] < ip2[3])
		return 0;
}

static int isZeroAddr(uint32 *ipAddr)
{
	if ((0 == ipAddr[0]) && (0 == ipAddr[1]) && (0 == ipAddr[2]) && (0 == ipAddr[3]))
		return 1;
	return 0;
}

static int isAddrEqual(uint32 *ip1, uint32 *ip2)
{
	if ((ip1[0] == ip2[0]) && (ip1[1] == ip2[1]) && (ip1[2] == ip2[2]) && (ip1[3] == ip2[3]))
		return 1;
	return 0;
}

static int do_ioctl(unsigned int cmd, struct ifreq *ifr)
{
	int skfd, ret;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (-1);
	}

	ret = ioctl(skfd, cmd, ifr);
	close(skfd);
	return ret;
}

/*
 * Get Interface Addr (MAC, IP, Mask)
 */
static int getInAddr(char *interface, ADDR_T type, void *pAddr)
{
	struct ifreq ifr;
	int found=0;
	struct sockaddr_in *addr;

	strcpy(ifr.ifr_name, interface);
	if (do_ioctl(SIOCGIFFLAGS, &ifr) < 0)
		return (0);

	if (type == HW_ADDR) {
		if (do_ioctl(SIOCGIFHWADDR, &ifr) >= 0) {
			memcpy(pAddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));
			found = 1;
		}
	}
	else if (type == IP_ADDR) {
		if (do_ioctl(SIOCGIFADDR, &ifr) == 0) {
			addr = ((struct sockaddr_in *)&ifr.ifr_addr);
			*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
			found = 1;
		}
	}
	else if (type == SUBNET_MASK) {
		if (do_ioctl(SIOCGIFNETMASK, &ifr) >= 0) {
			addr = ((struct sockaddr_in *)&ifr.ifr_addr);
			*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
			found = 1;
		}
	}
	return found;
}

int32 sys_ipAddr_get(char *devName, uint32 *pIp)
{
    SYS_PARAM_CHK(((NULL == devName) || (NULL == pIp)), FALSE);
	
    getInAddr(devName, IP_ADDR, pIp);
	
	return TRUE;
}

#ifdef CONFIG_IPV6
int getifip6(char *ifname, unsigned int addr_scope, uint8 *pAddr)
{
	FILE 			*fp;
	uint8           ip6_addr[16];
	unsigned int	ifindex = 0;
	unsigned int	prefixlen, scope, flags;
	unsigned char	scope_value;
	char			devname[IFNAMSIZ];
	char 			buf[1024];
	int				k = 0;

	memset(pAddr, 0, sizeof(struct in6_addr));
	
	/* Get link local addresses from /proc/net/if_inet6 */
	fp = fopen("/proc/net/if_inet6", "r");
	if (!fp)
		return 0;

	scope_value = addr_scope;
	if (addr_scope == IPV6_ADDR_UNICAST)
		scope_value = IPV6_ADDR_ANY;
	
	/* Format "fe80000000000000029027fffe24bbab 02 0a 20 80     eth0" */
	while (fgets(buf, sizeof(buf), fp))
	{
		//printf("buf= %s\n", buf);
		if (21 != sscanf( buf,
			"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %x %x %x %x %8s",
			&ip6_addr[ 0], &ip6_addr[ 1], &ip6_addr[ 2], &ip6_addr[ 3],
			&ip6_addr[ 4], &ip6_addr[ 5], &ip6_addr[ 6], &ip6_addr[ 7],
			&ip6_addr[ 8], &ip6_addr[ 9], &ip6_addr[10], &ip6_addr[11],
			&ip6_addr[12], &ip6_addr[13], &ip6_addr[14], &ip6_addr[15],
			&ifindex, &prefixlen, &scope, &flags, devname))
		{
			printf("/proc/net/if_inet6 has a broken line, ignoring");
			continue;
		}

		if (!strcmp(ifname, devname) && (addr_scope == IPV6_ADDR_ANY || scope_value == scope))
		{
			memcpy(pAddr, ip6_addr, 16);
			break;
		}
	}

	fclose(fp);
	return k;
}
#endif

int32 nic_pkt_alloc(int32 size, packet_info_t **ppPacket)
{
    SYS_PARAM_CHK((NULL == ppPacket), SYS_ERR_NULL_POINTER);
    packet_info_t *pkt = osal_alloc(sizeof(packet_info_t));
    if (NULL == pkt)
    {
        SYS_PRINTF("\nmalloc packet_info_t fail!\n");
        return SYS_ERR_FAILED;
    }
    osal_memset(pkt,0,sizeof(packet_info_t));
    
    pkt->data = osal_alloc(size);
    if (NULL == pkt->data)
    {
        SYS_PRINTF("\nmalloc pkt->data fail!\n");
        osal_free(pkt);
        return SYS_ERR_FAILED;
    }
    osal_memset(pkt->data,0,size);
    *ppPacket = pkt;
    return SYS_ERR_OK;
}

int32 nic_pkt_free(packet_info_t *pPacket)
{
    SYS_PARAM_CHK((NULL == pPacket), SYS_ERR_NULL_POINTER);
    if (NULL != pPacket->data)
    {
        osal_free(pPacket->data);
    }

    osal_free(pPacket);
    
    return SYS_ERR_OK;
}

uint16 sys_checksum_get(uint16 *addr, int32 len, uint16* pChksum)
{
    /* Compute Internet Checksum for "count" bytes
     *         beginning at location "addr".
     */
    register int32 sum = 0;
    uint16 *source = (uint16 *) addr;

    while (len > 1)  {
        /*  This is the inner loop */
        sum += *source++;
        len -= 2;
    }

    /*  Add left-over byte, if any */
    if (len > 0) {
        /* Make sure that the left-over byte is added correctly both
         * with little and big endian hosts */
        uint16 tmp = 0;
        *(uint8 *) (&tmp) = * (uint8 *) source;
        sum += tmp;
    }
    /*  Fold 32-bit sum to 16 bits */
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    *pChksum = ~sum;
    
    return SYS_ERR_OK;
}

int32 mcast_querier_array_search(uint16 vid, uint32 ipType, igmp_querier_entry_t **entry)
{
	igmp_querier_entry_t *sortedArray = NULL;
	int querier_num;
    int i;

	*entry = NULL;
	
	sortedArray = querier_db[ipType].querier_entry;
	querier_num = querier_db[ipType].entry_num;

	for (i=0; i<querier_num; i++)
	{
		if (vid == sortedArray[i].vid)
		{
			*entry = &sortedArray[i];
			return TRUE;
		}
	}

	return TRUE;
}

int32 mcast_querier_array_ins(igmp_querier_entry_t *entry, uint32 ipType)
{
	igmp_querier_entry_t *sortedArray = NULL;
	int querier_num;

	if (entry == NULL)
		return SYS_ERR_FAILED;

	sortedArray = querier_db[ipType].querier_entry;
	querier_num = querier_db[ipType].entry_num;
	
	if (querier_num >= MAX_ROUTER_VLAN)
		return SYS_ERR_FAILED;
	
	sortedArray[querier_num] = *entry;
	querier_db[ipType].entry_num++;
	
	return SYS_ERR_OK;
}

int32 mcast_querier_array_remove(uint16 vid, uint32 ipType)
{
	igmp_querier_entry_t *sortedArray = NULL;
	int querier_num;
	int i, j;

	sortedArray = querier_db[ipType].querier_entry;
	querier_num = querier_db[ipType].entry_num;

	for (i=0; i<querier_num; i++)
	{
		if (vid == sortedArray[i].vid)
		{
			for (j=i; j<querier_num-1; j++)
				sortedArray[j] = sortedArray[j+1];
			
			memset(&sortedArray[j], 0, sizeof(igmp_querier_entry_t));
			querier_db[ipType].entry_num--;

			break;
		}
	}

	return TRUE;
}

int32 mcast_querier_db_init(void)
{
	multicast_ipType_t ipType;
	struct sockaddr hwaddr;
	
    for(ipType = MULTICAST_TYPE_IPV4 ; ipType < MULTICAST_TYPE_END; ipType++)
    {
        osal_memset(&querier_db[ipType], 0, sizeof(querier_db[ipType]));
    }

	getInAddr("br0", HW_ADDR, &hwaddr);
	memcpy(&sys_mac, hwaddr.sa_data, 6);
	getInAddr("br0", IP_ADDR, &sys_ip);
#ifdef CONFIG_IPV6
	getifip6("br0", IPV6_ADDR_LINKLOCAL, (uint8 *)sys_ip6);
#endif

	return TRUE;
}

int32 mcast_querier_db_add(uint16 vid, uint32 ipType)
{
	igmp_querier_entry_t *entry = NULL;
	igmp_querier_entry_t newEntry;

	mcast_querier_array_search(vid, ipType, &entry);
	if (entry)
	{
		/* Do nothing */
	}
	else  /* not found */
	{
		memset(&newEntry, 0, sizeof(igmp_querier_entry_t));
		newEntry.vid = vid;
		newEntry.ipType = ipType;
		return mcast_querier_array_ins(&newEntry, ipType);
	}

	return TRUE;
}

int32 mcast_querier_db_get(uint16 vid, uint32 ipType, igmp_querier_entry_t **ppEntry)
{
	SYS_PARAM_CHK(NULL == ppEntry, FALSE);

	mcast_querier_array_search(vid, ipType, ppEntry);

	return TRUE;
}

int32 mcast_querier_db_del(uint16 vid, uint32 ipType)
{
	return mcast_querier_array_remove(vid, ipType);
}

void mcast_code_convert(uint32 code,  uint8 *pResult)
{
    uint8 tmp;
    
    if (NULL == pResult )
        return ;
    
    if(code < 128)    
    {           
        *pResult = code;   
    }    
    else   
    {        
        for(tmp =15; tmp>7; tmp--)            
            if((code) & (0x1<<tmp))               
                break;        
        *pResult = (((code)>>(tmp-4)) & 0xf) /*bit0-bit3*/                                    
            |(((tmp-4-3)<<4) & 0x70 ) /*bit4-bit6*/                                    
            | 0x80;    /*bit 7*/   
     }
}

void mcast_igmp_build_general_query (packet_info_t *pkt, sys_vid_t vid, uint16 pktLen, uint8 version, uint32 queryEnable, uint32 gdaddr, uint8 query_type)
{
	uint8 igmpv3hd[IGMPV3_HEAD_LEN];  /*s_rqv 1byte, qqic 1byte, src_num 16*/
	uint8 qqic;

	uint8 dst_mac[6]    = {0x01, 0x00, 0x5e, 0x00, 0x00, 0x01};
	uint16 ctag         = htons(0x8100);
	uint16 cvid;
	uint16 type         = htons(0x0800);

	uint8 lv            = htons(0x45);
	uint8 tos           = htons(0x00);
	uint16 id           = htons(0x0000);
	uint16 frag_off     = htons(0x0000);
	uint8 ttl           = htons(0x01);
	uint8 protocol      = htons(2);     /* 2: IPPROTO_IGMP */
	uint16 csum         = htons(0x00);

	uint32 daddr        = htonl(0xE0000001L);

	uint8 itype         = htons(IGMP_TYPE_MEMBERSHIP_QUERY);
	//   uint8  code = (uint8)htons(igmp_stats.responseTime * 10);
	uint32 gda           = htonl(0x00000000L);
	ipv4hdr_t *iph;
	igmp_hdr_t *igmph;
	uint32 saddr;
	uint16 tot_len;
	uint8 code;
	uint16 posite, igmpHdrCsm, ipHdrCsm;

	igmp_group_entry_t *group;
	igmp_config_t *igmpconfig;

	uint8  options[4] = {0x94, 0x04, 0x00, 0x00};

	uint32 ipType = MULTICAST_TYPE_IPV4;

	igmpconfig = getIgmpConfig();

	if (IGMP_GENERAL_QUERY == query_type)
	{
		daddr =  htonl(0xE0000001L);
		gda    = htonl(0x00000000L);
	}
	else if(IGMP_SPECIFIC_GROUP_QUERY == query_type)
	{
	    daddr = htonl(gdaddr);
	    gda    = htonl(gdaddr);
	    dst_mac[3] = (gdaddr >> 16) & 0x7f;
	    dst_mac[4] = (gdaddr >> 8) & 0xff;
	    dst_mac[5] =  gdaddr & 0xff;

		group = mcast_search_group_entry(ipType, vid, &gdaddr);
		if (!group)
			return;
	}

	cvid = (uint16)htons(vid);
	
	if(version == IGMP_QUERY_V3)
	{
	    mcast_code_convert(igmpconfig->responseTime*10, &code);
	    
	    code = htons(code);
	    //should equel 1, then the VLC test can pass.
	    //code = htons(1);    
	    tot_len =  htons ((sizeof(ipv4hdr_t)) + sizeof(igmp_hdr_t)) + 4/*iphdr option*/;
	}
	else
	{
	    code = igmpconfig->responseTime*10;

	    tot_len =  htons ((sizeof(ipv4hdr_t)) + sizeof(igmp_hdr_t) - sizeof(igmph->v3));    
	} 

	if(ENABLED == queryEnable)
	{
		getInAddr("br0", IP_ADDR, &sys_ip);
	    saddr = htonl(sys_ip);
	}
	else
	    saddr = htonl(0x00);

	posite = 0;
	
	/* ethernet II header */
	IGMP_FILL_QUERY(pkt, &dst_mac, posite, 6);
	posite += 6;
	IGMP_FILL_QUERY(pkt, &sys_mac, posite, 6);
	posite += 6;
	if (vid > 0)
	{
		IGMP_FILL_QUERY(pkt, &ctag, posite, 2);
		posite += 2;
		IGMP_FILL_QUERY(pkt, &cvid, posite, 2);
		posite += 2;
	}
	IGMP_FILL_QUERY(pkt, &type, posite, 2);
	posite += 2;

	/* ip header */
	iph = (ipv4hdr_t *)(&pkt->data[18]);

	if(version == IGMP_QUERY_V2)
	{
	    IGMP_FILL_QUERY(pkt, &lv, posite, 1);
		posite += 1;
	}
	else
	{
	    lv = htons(0x46);
	    IGMP_FILL_QUERY(pkt, &lv, posite, 1);
		posite += 1;
	}

	IGMP_FILL_QUERY(pkt, &tos, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &tot_len, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &id, posite, 2);
	posite += 2;

	if(version == IGMP_QUERY_V2)
	{
	    IGMP_FILL_QUERY(pkt, &frag_off, posite, 2);
		posite += 2;
	}
	else
	{
	    frag_off = htons(0x4000);
	    IGMP_FILL_QUERY(pkt, &frag_off, posite, 2);
		posite += 2;
	}

	IGMP_FILL_QUERY(pkt, &ttl, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &protocol, posite, 1);
	posite += 1;
	ipHdrCsm = posite;
	IGMP_FILL_QUERY(pkt, &csum, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &saddr, posite, 4);
	posite += 4;
	IGMP_FILL_QUERY(pkt, &daddr, posite, 4);
	posite += 4;

	if (IGMP_QUERY_V2  == version)
	{
	    sys_checksum_get((uint16 *)iph, sizeof(ipv4hdr_t), &csum);
	}
	else
	{
		/* Add 4 byte options */
		IGMP_FILL_QUERY(pkt, options, posite, 4);
		posite += 4;
		sys_checksum_get((uint16 *)iph, sizeof(ipv4hdr_t)+4, &csum);
	}

	csum = htons(csum);
	IGMP_FILL_QUERY(pkt, &csum, ipHdrCsm, 2);

	/* igmp header */
	csum = htons(0x0000);
	igmph = (igmp_hdr_t *)(&pkt->data[posite]);

	IGMP_FILL_QUERY(pkt, &itype, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &code, posite, 1);
	posite += 1;
	igmpHdrCsm = posite;
	IGMP_FILL_QUERY(pkt, &csum, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &gda, posite, 4);
	posite += 4;

	if(IGMP_QUERY_V3 == version)
	{
		if (IGMP_GENERAL_QUERY == query_type)
		{
			mcast_code_convert(igmpconfig->queryIntv, &qqic);
			igmpv3hd[0] = htons(igmpconfig->robustness);
		}
		else
		{
			mcast_code_convert(igmpconfig->lastMemberQueryIntv, &qqic);
			igmpv3hd[0] = htons(igmpconfig->lastMemberQueryCnt);
		}

		igmpv3hd[1] = htons(qqic);
		igmpv3hd[2] = htons(0x0);
		igmpv3hd[3] = htons(0x0);
		
	    IGMP_FILL_QUERY(pkt, igmpv3hd, posite, 4);
		posite += 4;
	}

	/* calculate cksum */
	if(IGMP_QUERY_V3 == version)
	    tot_len = sizeof(igmp_hdr_t) ;
	else
	    tot_len = sizeof(igmp_hdr_t)  - sizeof(igmph->v3);

	sys_checksum_get((uint16 *)igmph, tot_len, &csum);
	csum = htons(csum);

	IGMP_FILL_QUERY(pkt, &csum, igmpHdrCsm, 2);

	pkt->forced = 1;
	pkt->length = pktLen;
}

void mcast_igmp_build_gs_query(packet_info_t *pkt, sys_vid_t vid, uint16 pktLen, uint32 gdaddr, uint16 version)
{
	uint8 dst_mac[6]    = {0x01, 0x00, 0x5e, 0x00, 0x00, 0x01}; /* all hosts */
	uint16 ctag        = htons(0x8100);
	uint16 cvid;
	uint16 type         = htons(0x0800);

	uint8 lv            = htons(0x46);
	uint8 tos           = htons(0x00);
	uint16 tot_len ;

	uint16 id           = htons(0x0000);
	uint16 frag_off     = htons(0x0000);
	uint8 ttl           = htons(0x01);
	uint8 protocol      = htons(2);     /* 2: IPPROTO_IGMP */
	uint16 csum         = htons(0x0000);

	uint32 daddr        = htonl(gdaddr);

	uint8 itype         = htons(IGMP_TYPE_MEMBERSHIP_QUERY);

	uint8 code ; 

	uint32 gda          = htonl(gdaddr);
	uint8  options[4] = {0x94, 0x04, 0x00, 0x00};
	uint8 igmpv3hd[IGMPV3_HEAD_LEN];  /*s_rqv 1byte, qqic 1byte, src_num 16*/
	uint8 qqic;

	ipv4hdr_t *iph;
	igmp_hdr_t *igmph;
	uint32 saddr;

	uint32 ipType = MULTICAST_TYPE_IPV4;
	igmp_group_entry_t *group;
	igmp_config_t *igmpconfig;

	uint16 posite, ipHdrCsm, igmphdrCsm;

	group = mcast_search_group_entry(ipType, vid, &gdaddr);
	if (!group)
		return;

	igmpconfig = getIgmpConfig();
	
	cvid = (uint16)htons(vid);
	code   = (uint8)htons(igmpconfig->lastMemberQueryIntv * 10);

	if (vid > 0)
		tot_len = (uint16)htons(pktLen-18);  /*18 is the l2 header*/
	else
		tot_len = (uint16)htons(pktLen-14);  /*no vlan tag*/

	getInAddr("br0", IP_ADDR, &sys_ip);
	saddr = htonl(sys_ip);

	dst_mac[3] = (gdaddr >> 16) & 0x7f;
	dst_mac[4] = (gdaddr >> 8) & 0xff;
	dst_mac[5] =  gdaddr & 0xff;

	posite = 0;
	
	/* ethernet II header */
	IGMP_FILL_QUERY(pkt, &dst_mac, posite, 6);
	posite += 6;
	IGMP_FILL_QUERY(pkt, &sys_mac, posite, 6);
	posite += 6;
	if (vid > 0)
	{
		IGMP_FILL_QUERY(pkt, &ctag, posite, 2);
		posite += 2;
		IGMP_FILL_QUERY(pkt, &cvid, posite, 2);
		posite += 2;
	}
	IGMP_FILL_QUERY(pkt, &type, posite, 2);
	posite += 2;

	/* ip header */
	iph = (ipv4hdr_t *)(&pkt->data[18]);
	IGMP_FILL_QUERY(pkt, &lv, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &tos, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &tot_len, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &id, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &frag_off, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &ttl, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &protocol, posite, 1);
	posite += 1;
	ipHdrCsm = posite;
	IGMP_FILL_QUERY(pkt, &csum, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &saddr, posite, 4);
	posite += 4;
	IGMP_FILL_QUERY(pkt, &daddr, posite, 4);
	posite += 4;

	/* calculate ip header cksum */
	if (IGMP_QUERY_V2  == version)
	{
	    sys_checksum_get((uint16 *)iph, sizeof(ipv4hdr_t), &csum);
	}
	else
	{
		/* Add 4 byte options */
		IGMP_FILL_QUERY(pkt, options, posite, 4);
		posite += 4;
		sys_checksum_get((uint16 *)iph, sizeof(ipv4hdr_t)+4, &csum);
	}
	
	csum = htons(csum);
	IGMP_FILL_QUERY(pkt, &csum, ipHdrCsm, 2);

	/* igmp header */
	igmph = (igmp_hdr_t *)(&pkt->data[posite]);
	csum = 0;

	IGMP_FILL_QUERY(pkt, &itype, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &code, posite, 1);
	posite += 1;
	igmphdrCsm = posite;
	IGMP_FILL_QUERY(pkt, &csum, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &gda, posite, 4);
	posite += 4;

	if(IGMP_QUERY_V3 == version)
	{
	    mcast_code_convert(igmpconfig->lastMemberQueryIntv, &qqic);
	    igmpv3hd[0] = htons(igmpconfig->lastMemberQueryCnt);
	    
	    igmpv3hd[1] = htons(qqic);
	    igmpv3hd[2] = htons(0x0);
	    igmpv3hd[3] = htons(0x0);

	    IGMP_FILL_QUERY(pkt, igmpv3hd, posite, 4);
		posite + 4;

	    /* calculate cksum */
	    sys_checksum_get((uint16 *)igmph, sizeof(igmp_hdr_t), &csum);
	}
	else
	{
	    sys_checksum_get((uint16 *)igmph, sizeof(igmp_hdr_t)-sizeof(igmph->v3), &csum);
	}

	csum = htons(csum);

	IGMP_FILL_QUERY(pkt, &csum, igmphdrCsm, 2);

	pkt->forced = 1;
	pkt->length = pktLen;
}

void mcast_igmp_build_gss_query(packet_info_t * pkt, sys_vid_t vid, uint32 gdaddr, uint32 *pSip, uint16 numSrc, uint16 pktlen)
{
	uint8 dst_mac[6] ; /*specific group address  hosts */
	uint16 ctag        = htons(0x8100);
	uint16 cvid;
	uint16 type         = htons(0x0800);
	uint8 lv            = htons(0x45);
	uint8 tos           = htons(0x00);
	uint16 tot_len      = htons (pktlen - 14);
	uint16 id           = htons(0x0000);
	uint16 frag_off     = htons(0x0000);
	uint8 ttl           = htons(0x01);
	uint8 protocol      = htons(2);     /* 2: IPPROTO_IGMP */
	uint16 csum         = htons(0x0000);
	uint32 daddr        = htonl(gdaddr);
	uint8 itype         = htons(IGMP_TYPE_MEMBERSHIP_QUERY);
	uint32 gda          = htonl(gdaddr);
	uint8 rsv               = htons(0);
	uint8 qqic               = htons(60);
	uint16 srcNum         = htons(numSrc);
	ipv4hdr_t *iph;
	igmp_hdr_t *igmph;
	uint32 saddr;
	uint32 sip;
	uint16 i;
	uint8 code;
	uint32 tmp=0;

	uint32 ipType = MULTICAST_TYPE_IPV4;
	igmp_group_entry_t *group;
	igmp_config_t *igmpconfig;

	uint16 posite, ipHdrCsm, igmphdrCsm, igmphdrOffset;

	group = mcast_search_group_entry(ipType, vid, &gdaddr);
	if (!group)
		return;

	igmpconfig = getIgmpConfig();
	
	osal_memset(dst_mac, 0, sizeof(dst_mac));

	dst_mac[0] = 0x01;
	dst_mac[1] = 0x00;
	dst_mac[2] = 0x5e;
	dst_mac[3] = (gdaddr >>16 & 0x07f);
	dst_mac[4] = (gdaddr >>8 & 0x0ff);
	dst_mac[5] = (gdaddr & 0x0ff);

	getInAddr("br0", IP_ADDR, &sys_ip);
	saddr = htonl(sys_ip);

	cvid = (uint16)htons(vid);

	mcast_code_convert(igmpconfig->lastMemberQueryIntv, &code);
	code = htons(code);

	posite = 0;
	
	/* ethernet II header */
	IGMP_FILL_QUERY(pkt, &dst_mac, posite, 6);
	posite += 6;
	IGMP_FILL_QUERY(pkt, &sys_mac, posite, 6);
	posite += 6;
	if (vid > 0)
	{
		IGMP_FILL_QUERY(pkt, &ctag, posite, 2);
		posite += 2;
		IGMP_FILL_QUERY(pkt, &cvid, posite, 2);
		posite += 2;
	}
	IGMP_FILL_QUERY(pkt, &type, posite, 2);
	posite += 2;

	/* ip header */
	if (vid > 0)
		tot_len      = htons (pktlen - 18);
	else
		tot_len      = htons (pktlen - 14);
	iph = (ipv4hdr_t *)(&pkt->data[18]);
	IGMP_FILL_QUERY(pkt, &lv, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &tos, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &tot_len, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &id, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &frag_off, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &ttl, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &protocol, posite, 1);
	posite += 1;
	ipHdrCsm = posite;
	IGMP_FILL_QUERY(pkt, &csum, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &saddr, posite, 4);
	posite += 4;
	IGMP_FILL_QUERY(pkt, &daddr, posite, 4);
	posite += 4;

	/* calculate ip header cksum */
	sys_checksum_get((uint16*)iph, sizeof(ipv4hdr_t), &csum);
	csum = htons(csum);
	IGMP_FILL_QUERY(pkt, &csum, ipHdrCsm, 2);

	/* igmp header */
	igmphdrOffset = posite;
	igmph = (igmp_hdr_t *)(&pkt->data[igmphdrOffset]);
	csum = 0;

	IGMP_FILL_QUERY(pkt, &itype, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &code, posite, 1);
	posite += 1;
	igmphdrCsm = posite;
	IGMP_FILL_QUERY(pkt, &csum, posite, 2);
	posite += 2;
	IGMP_FILL_QUERY(pkt, &gda, posite, 4);
	posite += 4;

	IGMP_FILL_QUERY(pkt, &rsv, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &qqic, posite, 1);
	posite += 1;
	IGMP_FILL_QUERY(pkt, &srcNum, posite, 2);
	posite += 2;

	for(i = 0; i < numSrc; i++)
	{
	    sip = ntohl(*(pSip + i));
	    IGMP_FILL_QUERY(pkt, &sip, posite, 4);
		posite += 4;
	}

	/* calculate cksum */
	sys_checksum_get((uint16*)igmph, pktlen - igmphdrOffset, &csum);
	csum = htons(csum);

	IGMP_FILL_QUERY(pkt, &csum, igmphdrCsm, 2);

	pkt->forced = 1;
	pkt->length = pktlen;
}

void mcast_igmp_send_general_query(igmp_querier_entry_t* qryPtr, uint8 igmp_query_version)
{
	packet_info_t *pkt;
	int32 queryPduLen;
	igmp_mc_vlan_entry_t *pVlanEntry;
	sys_logic_portmask_t txPmsk;

	if (!isEnableIgmpSnooping())
	    return;

	if(igmp_query_version == IGMP_QUERY_V3)
	    queryPduLen =  IGMP_QUERY_PDU_LEN + IGMPV3_HEAD_LEN + IGMPV3_OPTIONS_LEN ;
	else
	    queryPduLen =  IGMP_QUERY_PDU_LEN;

	if (qryPtr->vid > 0)
	{
		pVlanEntry = mcast_mcVlan_entry_get(qryPtr->vid);
		txPmsk = pVlanEntry->portmask;
	}
	else
	{
		/* remove vlan tag */
		queryPduLen -= 4;
		LOGIC_PORTMASK_SET_ALL(txPmsk);
	}

	if ((!IS_LOGIC_PORTMASK_CLEAR(txPmsk)) && (ENABLED == qryPtr->enabled))
	{
		if (SYS_ERR_OK == nic_pkt_alloc(queryPduLen, &pkt))
		{
			mcast_igmp_build_general_query(pkt, qryPtr->vid, queryPduLen, igmp_query_version, ENABLED, 0, IGMP_GENERAL_QUERY);

			SYS_DBG("Send General Query in VLAN-%d\n", qryPtr->vid);

			if (SYS_ERR_OK == mcast_snooping_tx(pkt, qryPtr->vid, pkt->length, txPmsk))
			{
			    if(IGMP_QUERY_V3 == igmp_query_version)
			        igmp_stats.v3.g_queryV3_xmit++;      /* General query */
			    else
			        igmp_stats.g_query_xmit++;      /* General query */
			}
			nic_pkt_free(pkt);
		}
		else
		{
		    SYS_DBG("igmp_snooping_send_general_query: dev_alloc_skb() failed.\n");
		    return;
		}
	}
}

void mcast_igmp_send_grp_specific_query(uint16 vid, uint32 dip, sys_logic_port_t port)
{
	int32   ret;
	packet_info_t *pkt;
	sys_logic_portmask_t txPmsk;
	uint16 pktlen = 0;
	igmp_querier_entry_t *pEntry = NULL;
	igmp_group_entry_t *pGroup;
	sys_logic_portmask_t mbr;

	uint32 ipType = MULTICAST_TYPE_IPV4;

	if (!isEnableIgmpSnooping())
	    return;

	LOGIC_PORTMASK_CLEAR_ALL(txPmsk);
	LOGIC_PORTMASK_SET_PORT(txPmsk, port);

	mcast_querier_db_get(vid, ipType, &pEntry);
	if (NULL == pEntry)
	    return;

	if (IGMP_QUERY_V2 == pEntry->version)
	    pktlen = IGMP_QUERY_PDU_LEN + IGMPV3_OPTIONS_LEN;
	else
	    pktlen = IGMP_QUERY_V3_PDU_LEN + IGMPV3_OPTIONS_LEN;

	pGroup = mcast_search_group_entry(ipType, vid, &dip);
	if (!pGroup)
		return;

	mcast_getGroupFwdPortMask(pGroup, &mbr);
	if (!IS_LOGIC_PORTMASK_PORTSET(mbr, port))
	    return;

	if (0 == vid)
	{
		/* remove vlan tag */
		pktlen -= 4;
	}

	if (SYS_ERR_OK == nic_pkt_alloc(pktlen, &pkt))
	{
	    mcast_igmp_build_gs_query(pkt, vid, pktlen, dip, pEntry->version);

	    ret = mcast_snooping_tx(pkt, vid, pktlen, txPmsk);
	    if (ret)
	        SYS_DBG("mcast_snooping_tx() failed!  ret:%d\n", ret);
	    else
	    {
	        if (IGMP_QUERY_V3 == pEntry->version)
	            igmp_stats.v3.gs_queryV3_xmit++;
	        else
	            igmp_stats.gs_query_xmit++;
	        SYS_DBG("Send Group Specific Query ("IPADDR_PRINT") in VLAN-%d .\n", IPADDR_PRINT_ARG(dip), vid);
	    }
		nic_pkt_free(pkt);
	}
	else
	{
	    SYS_DBG("igmp_snooping_send_general_query: dev_alloc_skb() failed.\n");
	    return;
	}
}

void mcast_igmp_send_grp_src_specific_query(uint16 vid, uint32 dip, uint32 *sip, uint16 numSrc, sys_logic_port_t port)
{
    int ret;
    packet_info_t *pkt;
    sys_logic_portmask_t txPmsk;
    uint16 pktLen = 0;
    int i;

    igmp_querier_entry_t *pEntry = NULL;
    igmp_group_entry_t *pGroup = NULL;

    uint32 ipType = MULTICAST_TYPE_IPV4;

    if (!isEnableIgmpSnooping())
        return;
        
    LOGIC_PORTMASK_CLEAR_ALL(txPmsk);

    mcast_querier_db_get(vid, ipType, &pEntry);
    if ((NULL == pEntry) || (IGMP_QUERY_V3 != pEntry->version))
        return;

	pGroup = mcast_search_group_entry(ipType, vid, &dip);
	if (!pGroup)
		return;

	mcast_getGroupFwdPortMask(pGroup, &txPmsk);
    if (IS_LOGIC_PORTMASK_CLEAR(txPmsk))
        return;
    
    pktLen =  IGMP_QUERY_PDU_LEN + 1 + 1  + 2 + numSrc * 4; /*rserved + SQRV+QQIC+sourceNum + sourceList*/
	if (0 == vid)
	{
		/* remove vlan tag */
		pktLen -= 4;
	}

    if(SYS_ERR_OK == nic_pkt_alloc(pktLen, &pkt))
    {
        mcast_igmp_build_gss_query(pkt, vid, dip, sip, numSrc, pktLen);

        ret = mcast_snooping_tx(pkt, vid, pktLen, txPmsk);
        if (ret)
            SYS_DBG("mcast_snooping_tx() failed!  ret:%d\n", ret);
        else
        {
            igmp_stats.v3.gss_queryV3_xmit++;
            SYS_DBG("Send Group Specific Source Specific Query ("IPADDR_PRINT") in VLAN-%d. \n", IPADDR_PRINT_ARG(dip), vid);
        }

		nic_pkt_free(pkt);
    }
    else
    {
        SYS_DBG("mcast_igmp_send_grp_src_specific_query: dev_alloc_skb() failed.\n");
        return;
    }
}

#ifdef SUPPORT_MLD_SNOOPING
void mcast_mld_build_general_query (packet_info_t *pkt, sys_vid_t vid, uint16 pktLen, uint8 version)
{
    uint8 dst_mac[6]    = {0x33, 0x33, 0x00, 0x00, 0x00, 0x01};
    uint16 ctag         = htons(0x8100);
    uint16 cvid;
    uint16 type         = htons(0x86dd);
    uint16 vtf           =  htons(0x6000);
    uint16 flowlabel   = htons(0x0000);
    uint16 tot_len;
    uint8  ipv6_nextHrd = htons(0x00);
    uint8  hopLimit           = htons(0x01);
    uint8  saddr[SYS_IPV6_NUM_LEN];
    uint8  daddr[SYS_IPV6_NUM_LEN] = {0xff,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    uint8  hop_nextHrd =  htons(0x3a);
    uint8  hop_len         =    htons(0x0);
    uint8 routerAlert[4]    =  {0x05, 0x02, 0x00, 0x00};
    uint16 padn = htons(0x0100);
    uint8  reportType = htons(0x82);
    uint8  code = htons(0x0);
    uint16 mldHdrCsm = 0;
    uint16 max_response;
    uint16 rev = htons(0x0000);
    uint8  sflag_rob = 0x02;
    uint8  qqi = 0x14;
    uint16 numSrc = htons(0x0000);

    igmp_config_t *igmpconfig;
    uint8 bufcsm[32+4+4+28]; /*dip +sip, len ,nex_head,mld*/
    uint16 offset = 0, sipOffset, mldCsmOffset;
    uint16 mld_offset = 0;

    igmpconfig = getIgmpConfig();

    if (MLD_QUERY_V1 == version)
    {
        tot_len = 32;	//hop by hop header 8 + icmpv6 header 24
    }
    else
    {
        tot_len = 36;
    }

    tot_len = htons(tot_len);
    cvid = (uint16)htons(vid);
    max_response = igmpconfig->responseTime * 1000;
    max_response = htons(max_response);
    osal_memset(bufcsm,0,sizeof(bufcsm));

    /* ethernet II header */
    IGMP_FILL_QUERY(pkt, &dst_mac, offset, 6);
    offset += 6;
    IGMP_FILL_QUERY(pkt, &sys_mac, offset, 6);
    offset += 6;
	if (vid > 0)
	{
	    IGMP_FILL_QUERY(pkt, &ctag, offset, 2);
	    offset += 2;
	    IGMP_FILL_QUERY(pkt, &cvid, offset, 2);
	    offset += 2;
	}
    IGMP_FILL_QUERY(pkt, &type, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &vtf, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &flowlabel, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &tot_len, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &ipv6_nextHrd, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &hopLimit, offset, 1);
    offset += 1;

    osal_memset(saddr,0,sizeof(saddr));
	getifip6("br0", IPV6_ADDR_LINKLOCAL, (uint8 *)sys_ip6);
    osal_memcpy(saddr, sys_ip6, IPV6_ADDR_LEN);
	sipOffset = offset;
    IGMP_FILL_QUERY(pkt, saddr, offset, 16);
    offset += 16;
    IGMP_FILL_QUERY(pkt, daddr, offset, 16);
    offset += 16;
    IGMP_FILL_QUERY(pkt, &hop_nextHrd, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &hop_len, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, routerAlert, offset, 4);
    offset += 4;
    IGMP_FILL_QUERY(pkt, &padn, offset, 2);
    offset += 2;
    mld_offset = offset;
    IGMP_FILL_QUERY(pkt, &reportType, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &code, offset, 1);
    offset += 1;
	mldCsmOffset = offset;
    IGMP_FILL_QUERY(pkt, &mldHdrCsm, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &max_response, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &rev, offset, 2);
    offset += 2;

	/* for general query, mcast address should be 0 */
    osal_memset(saddr,0,sizeof(saddr));
    IGMP_FILL_QUERY(pkt, saddr, offset, 16);
    offset += 16;

    osal_memcpy(bufcsm, &pkt->data[sipOffset], 32);
    if (MLD_QUERY_V2 == version)
    {
        sflag_rob = igmpconfig->robustness & 0x7;
        mcast_code_convert(igmpconfig->queryIntv, &qqi);
        
        IGMP_FILL_QUERY(pkt, &sflag_rob, offset, 1);
        offset += 1;
        IGMP_FILL_QUERY(pkt, &qqi, offset, 1);
        offset += 1;
        IGMP_FILL_QUERY(pkt, &numSrc, offset, 2);
        offset += 2;

        bufcsm[35] = 28;
        bufcsm[39] = 0x3a;
        osal_memcpy(&bufcsm[40], &pkt->data[mld_offset], 28);
        sys_checksum_get((uint16 *)bufcsm, sizeof(bufcsm), &mldHdrCsm);
    }
    else
    {
        bufcsm[35] = 24;
        bufcsm[39] = 0x3a;
        osal_memcpy(&bufcsm[40], &pkt->data[mld_offset], 24);
        sys_checksum_get((uint16 *)bufcsm,sizeof(bufcsm)-4,&mldHdrCsm);
    }
	mldHdrCsm = htons(mldHdrCsm);
	IGMP_FILL_QUERY(pkt, &mldHdrCsm, mldCsmOffset, 2);

	pkt->forced = 1;
    pkt->length = pktLen;
}

void mcast_mld_build_gs_query(packet_info_t *pkt, sys_vid_t vid, uint16 pktLen, uint8 *pIpv6Addr, uint16 version)
{
    uint8 dst_mac[6]    = {0x33, 0x33, 0x00, 0x00, 0x00, 0x00};
    uint16 ctag         = htons(0x8100);
    uint16 cvid;
    uint16 type         = htons(0x86dd);
    uint16 vtf           =  htons(0x6000);
    uint16 flowlabel   = htons(0x0000);
    uint16 tot_len;
    uint8  ipv6_nextHrd = htons(0x00);
    uint8  hopLimit           = htons(0x01);
    uint8  saddr[SYS_IPV6_NUM_LEN];
    uint8  daddr[SYS_IPV6_NUM_LEN];
    uint8  hop_nextHrd =  htons(0x3a);
    uint8  hop_len         =    htons(0x0);
    uint8 routerAlert[4]    =  {0x05, 0x02, 0x00, 0x00};
    uint16 padn = htons(0x0100);
    uint8  reportType = htons(0x82);
    uint8  code = htons(0x0);
    uint16 mldHdrCsm = 0;
    uint16 max_response;
    uint16 rev = htons(0x0000);
    uint8  sflag_rob = 0x02;
    uint8  qqi = 0x14;
    uint16 numSrc = htons(0x0000);

	igmp_config_t *igmpconfig;
    uint8 bufcsm[32+4+4+28]; /*dip +sip, len, nex_head, mld*/
    uint16 offset = 0, sipOffset, mldCsmOffset;
    uint16 mld_offset = 0;

    igmpconfig = getIgmpConfig();

    if (MLD_QUERY_V1 == version)
    {
        tot_len = 32;
    }
    else
    {
        tot_len = 36;
    }

    tot_len = htons(tot_len);
    cvid = (uint16)htons(vid);
    max_response = igmpconfig->responseTime * 1000;
    max_response = htons(max_response);
    osal_memset(bufcsm,0, sizeof(bufcsm));

    osal_memcpy(dst_mac + 2, pIpv6Addr+12, 4);
    osal_memcpy(daddr, pIpv6Addr, IPV6_ADDR_LEN);

    /* ethernet II header */
    IGMP_FILL_QUERY(pkt, &dst_mac, offset, 6);
    offset += 6;
    IGMP_FILL_QUERY(pkt, &sys_mac, offset, 6);
    offset += 6;
	if (vid > 0)
	{
		IGMP_FILL_QUERY(pkt, &ctag, offset, 2);
		offset += 2;
		IGMP_FILL_QUERY(pkt, &cvid, offset, 2);
		offset += 2;
	}
    IGMP_FILL_QUERY(pkt, &type, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &vtf, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &flowlabel, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &tot_len, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &ipv6_nextHrd, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &hopLimit, offset, 1);
    offset += 1;

    osal_memset(saddr,0,sizeof(saddr));
	getifip6("br0", IPV6_ADDR_LINKLOCAL, (uint8 *)sys_ip6);
	osal_memcpy(saddr, sys_ip6, IPV6_ADDR_LEN);
	sipOffset = offset;
    IGMP_FILL_QUERY(pkt, saddr, offset, 16);
    offset += 16;
    IGMP_FILL_QUERY(pkt, daddr, offset, 16);
    offset += 16;
    IGMP_FILL_QUERY(pkt, &hop_nextHrd, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &hop_len, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, routerAlert, offset, 4);
    offset += 4;
    IGMP_FILL_QUERY(pkt, &padn, offset, 2);
    offset += 2;
    mld_offset = offset;
    IGMP_FILL_QUERY(pkt, &reportType, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &code, offset, 1);
    offset += 1;
	mldCsmOffset = offset;
    IGMP_FILL_QUERY(pkt, &mldHdrCsm, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &max_response, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &rev, offset, 2);
    offset += 2;

    IGMP_FILL_QUERY(pkt, daddr, offset, 16);
    offset += 16;

    osal_memcpy(bufcsm, &pkt->data[sipOffset], 32);
    if (MLD_QUERY_V2 == version)
    {
        sflag_rob = (igmpconfig->lastMemberQueryCnt) & 0x7;
        mcast_code_convert(igmpconfig->queryIntv, &qqi);
        
        IGMP_FILL_QUERY(pkt, &sflag_rob, offset, 1);
        offset += 1;
        IGMP_FILL_QUERY(pkt, &qqi, offset, 1);
        offset += 1;
        IGMP_FILL_QUERY(pkt, &numSrc, offset, 2);
        offset += 2;

        bufcsm[35] = 28;
        bufcsm[39] = 0x3a;
        osal_memcpy(&bufcsm[40], &pkt->data[mld_offset], 28);
        sys_checksum_get((uint16 *)bufcsm,sizeof(bufcsm),&mldHdrCsm);
    }
    else
    {
        bufcsm[35] = 24;
        bufcsm[39] = 0x3a;
        osal_memcpy(&bufcsm[40], &pkt->data[mld_offset], 24);
        sys_checksum_get((uint16 *)bufcsm,sizeof(bufcsm)-4,&mldHdrCsm);
    }
	mldHdrCsm = htons(mldHdrCsm);
	IGMP_FILL_QUERY(pkt, &mldHdrCsm, mldCsmOffset, 2);

	pkt->forced = 1;
    pkt->length = pktLen;
}

void mcast_mld_build_gss_query(packet_info_t *pkt, sys_vid_t vid, uint16 pktLen, uint8 *pIpv6Addr, uint32 *pSip, uint16 numSrc)
{
    uint8 dst_mac[6]    = {0x33, 0x33, 0x00, 0x00, 0x00, 0x00};
    uint16 ctag         = htons(0x8100);
    uint16 cvid;
    uint16 type         = htons(0x86dd);
    uint16 vtf           =  htons(0x6000);
    uint16 flowlabel   = htons(0x0000);
    uint16 tot_len;
    uint8  ipv6_nextHrd = htons(0x00);
    uint8  hopLimit           = htons(0x01);
    uint8  saddr[SYS_IPV6_NUM_LEN];
    uint8  daddr[SYS_IPV6_NUM_LEN];
    uint8  hop_nextHrd =  htons(0x3a);
    uint8  hop_len         =    htons(0x0);
    uint8 routerAlert[4]    =  {0x05, 0x02, 0x00, 0x00};
    uint16 padn = htons(0x0100);
    uint8  reportType = htons(0x82);
    uint8  code = htons(0x0);
	uint16 srcNum = htons(numSrc);
    uint16 mldHdrCsm = 0;
    uint16 max_response;
    uint16 rev = htons(0x0000);
    uint8  sflag_rob = 0x02;
    uint8  qqi = 0x14;
	uint16 i;

	igmp_config_t *igmpconfig;
    uint8 bufcsm[1500]; /*dip +sip, len, nex_head, mld*/
    uint16 offset = 0, sipOffset, mldCsmOffset;
    uint16 mld_offset = 0;
	uint16 mldhdrLen;

    igmpconfig = getIgmpConfig();

    tot_len = htons(36 + 16*numSrc);
    cvid = (uint16)htons(vid);
    max_response = igmpconfig->responseTime * 1000;
    max_response = htons(max_response);
    osal_memset(bufcsm,0, sizeof(bufcsm));

    osal_memcpy(dst_mac + 2, pIpv6Addr+12, 4);
    osal_memcpy(daddr, pIpv6Addr, IPV6_ADDR_LEN);

    /* ethernet II header */
    IGMP_FILL_QUERY(pkt, &dst_mac, offset, 6);
    offset += 6;
    IGMP_FILL_QUERY(pkt, &sys_mac, offset, 6);
    offset += 6;
	if (vid > 0)
	{
		IGMP_FILL_QUERY(pkt, &ctag, offset, 2);
		offset += 2;
		IGMP_FILL_QUERY(pkt, &cvid, offset, 2);
		offset += 2;
	}
    IGMP_FILL_QUERY(pkt, &type, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &vtf, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &flowlabel, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &tot_len, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &ipv6_nextHrd, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &hopLimit, offset, 1);
    offset += 1;

    osal_memset(saddr,0,sizeof(saddr));
	getifip6("br0", IPV6_ADDR_LINKLOCAL, (uint8 *)sys_ip6);
	osal_memcpy(saddr, sys_ip6, IPV6_ADDR_LEN);
	sipOffset = offset;
    IGMP_FILL_QUERY(pkt, saddr, offset, 16);
    offset += 16;
    IGMP_FILL_QUERY(pkt, daddr, offset, 16);
    offset += 16;
    IGMP_FILL_QUERY(pkt, &hop_nextHrd, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &hop_len, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, routerAlert, offset, 4);
    offset += 4;
    IGMP_FILL_QUERY(pkt, &padn, offset, 2);
    offset += 2;
    mld_offset = offset;
    IGMP_FILL_QUERY(pkt, &reportType, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &code, offset, 1);
    offset += 1;
	mldCsmOffset = offset;
    IGMP_FILL_QUERY(pkt, &mldHdrCsm, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &max_response, offset, 2);
    offset += 2;
    IGMP_FILL_QUERY(pkt, &rev, offset, 2);
    offset += 2;

    IGMP_FILL_QUERY(pkt, daddr, offset, 16);
    offset += 16;

    osal_memcpy(bufcsm, &pkt->data[sipOffset], 32);
	
    sflag_rob = (igmpconfig->lastMemberQueryCnt) & 0x7;
    mcast_code_convert(igmpconfig->queryIntv, &qqi);
    
    IGMP_FILL_QUERY(pkt, &sflag_rob, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &qqi, offset, 1);
    offset += 1;
    IGMP_FILL_QUERY(pkt, &srcNum, offset, 2);
    offset += 2;
	for (i=0; i<numSrc; i++)
	{
		IGMP_FILL_QUERY(pkt, pSip, offset, 16);
		offset += 16;
		pSip += 4;
	}

	mldhdrLen = 28 + 16*numSrc;
	memcpy(&bufcsm[32], (uint8 *)&mldhdrLen, 4);
    bufcsm[39] = 0x3a;
    osal_memcpy(&bufcsm[40], &pkt->data[mld_offset], mldhdrLen);
    sys_checksum_get((uint16 *)bufcsm, 40+mldhdrLen, &mldHdrCsm);
	mldHdrCsm = htons(mldHdrCsm);
	IGMP_FILL_QUERY(pkt, &mldHdrCsm, mldCsmOffset, 2);

	pkt->forced = 1;
    pkt->length = pktLen;
}

void mcast_mld_send_general_query(igmp_querier_entry_t* qryPtr, uint8 query_version)
{
    packet_info_t *pkt;
	igmp_mc_vlan_entry_t *pVlanEntry;
    sys_logic_portmask_t txPmsk;
    int32 queryPduLen;

    uint32 ipType = MULTICAST_TYPE_IPV6;

    if (!isEnableIgmpSnooping())
        return;

    if(query_version == MLD_QUERY_V1)
        queryPduLen =  MLD_QUERY_V1_PDU_LEN;
    else
        queryPduLen =  MLD_QUERY_V2_PDU_LEN;

	if (qryPtr->vid > 0)
	{
		pVlanEntry = mcast_mcVlan_entry_get(qryPtr->vid);
		txPmsk = pVlanEntry->portmask;
	}
	else
	{
		/* remove vlan tag */
		queryPduLen -= 4;
		LOGIC_PORTMASK_SET_ALL(txPmsk);
	}
    
    if ((!IS_LOGIC_PORTMASK_CLEAR(txPmsk)) && (ENABLED == qryPtr->enabled))
    {
        if (SYS_ERR_OK == nic_pkt_alloc(queryPduLen, &pkt))
        {
            mcast_mld_build_general_query(pkt, qryPtr->vid, queryPduLen, query_version);

            SYS_DBG("Send MLD General Query in VLAN-%d\n", qryPtr->vid);

			if (SYS_ERR_OK == mcast_snooping_tx(pkt, qryPtr->vid, pkt->length, txPmsk))
			{
			    if(MLD_QUERY_V2 == query_version)
			        igmp_stats.v3.g_queryV3_xmit++;      /* General query */
			    else
			        igmp_stats.g_query_xmit++;      /* General query */
			}
			
			nic_pkt_free(pkt);
        }
        else
        {
            SYS_DBG("mcast_mld_send_general_query: dev_alloc_skb() failed.\n");
            return;
        }
    }
}

void mcast_mld_send_grp_specific_query(igmp_group_entry_t *pGroup, sys_logic_port_t port)
{
    int32   ret;
    packet_info_t *pkt;
    sys_logic_portmask_t txPmsk;
    uint16 pktlen = 0;
    igmp_querier_entry_t *pEntry = NULL;

    uint32 ipType = MULTICAST_TYPE_IPV4;

    if (!isEnableIgmpSnooping())
        return;
    
    LOGIC_PORTMASK_CLEAR_ALL(txPmsk);

    mcast_querier_db_get(pGroup->vid, ipType, &pEntry);
    if (NULL == pEntry)
        return;

    if (MLD_QUERY_V1 == pEntry->version)
        pktlen = MLD_QUERY_V1_PDU_LEN;
    else
        pktlen = MLD_QUERY_V2_PDU_LEN;
    
	mcast_getGroupFwdPortMask(pGroup, &txPmsk);
    if (IS_LOGIC_PORTMASK_CLEAR(txPmsk))
        return;

	if (0 == pGroup->vid)
	{
		/* remove vlan tag */
		pktlen -= 4;
	}
    
    if (SYS_ERR_OK == nic_pkt_alloc(pktlen, &pkt))
    {
        mcast_mld_build_gs_query(pkt, pGroup->vid, pktlen, pGroup->groupAddr, pEntry->version);

        ret = mcast_snooping_tx(pkt, pGroup->vid, pktlen, txPmsk);
        
        if (ret)
            SYS_DBG("mcast_snooping_tx() failed!  ret:%d\n", ret);
        else
        {
            igmp_stats.gs_query_xmit++;
            SYS_DBG("Send Group Specific MLD Query ("IPADDR_PRINT") in VLAN-%d .\n", IPADDR_PRINT_ARG(pGroup->groupAddr[0]), pGroup->vid);
        }

        nic_pkt_free(pkt);
    }
    else
    {
        SYS_DBG("igmp_snooping_send_general_query: dev_alloc_skb() failed.\n");
        return;
    }
}

void mcast_mld_send_grp_src_specific_query(igmp_group_entry_t *pGroup, uint32 *sip, uint16 numSrc, sys_logic_port_t port)
{
	int32   ret;
	packet_info_t *pkt;
	sys_logic_portmask_t txPmsk;
	uint16 pktlen = 0;
	igmp_querier_entry_t *pEntry = NULL;

	uint32 ipType = MULTICAST_TYPE_IPV4;

	if (!isEnableIgmpSnooping())
	    return;

	LOGIC_PORTMASK_CLEAR_ALL(txPmsk);

	mcast_querier_db_get(pGroup->vid, ipType, &pEntry);
	if ((NULL == pEntry) || (MLD_QUERY_V2 != pEntry->version))
	    return;

	pktlen = MLD_QUERY_V2_PDU_LEN + 16*numSrc;

	mcast_getGroupFwdPortMask(pGroup, &txPmsk);
	if (IS_LOGIC_PORTMASK_CLEAR(txPmsk))
	    return;

	if (0 == pGroup->vid)
	{
		/* remove vlan tag */
		pktlen -= 4;
	}

	if (SYS_ERR_OK == nic_pkt_alloc(pktlen, &pkt))
	{
        mcast_mld_build_gss_query(pkt, pGroup->vid, pktlen, pGroup->groupAddr, sip, numSrc);

        ret = mcast_snooping_tx(pkt, pGroup->vid, pktlen, txPmsk);
        
        if (ret)
            SYS_DBG("mcast_snooping_tx() failed!  ret:%d\n", ret);
        else
        {
            igmp_stats.v3.gss_queryV3_xmit++;
            SYS_DBG("Send Group Specific MLD Query ("IPADDR_PRINT") in VLAN-%d .\n", IPADDR_PRINT_ARG(pGroup->groupAddr[0]), pGroup->vid);
        }

        nic_pkt_free(pkt);
    }
    else
    {
        SYS_DBG("igmp_snooping_send_general_query: dev_alloc_skb() failed.\n");
        return;
    }
}

int32 mcast_mld_querier_check(uint16 vid, uint32 *sip, uint8 qver)
{
	igmp_querier_entry_t *pEntry;
	uint32 ipType = MULTICAST_TYPE_IPV6;
	igmp_mc_vlan_entry_t *pMcastVlan = NULL;
	igmp_config_t *igmpconfig;

	SYS_PARAM_CHK(!VALID_VLAN_ID(vid), SYS_ERR_VLAN_ID);

	pMcastVlan = mcast_mcVlan_entry_get(vid);
	if ((vid > 0) && (0 == pMcastVlan->vid))
	{
		return SYS_ERR_FAILED;
	}

	mcast_querier_db_get(vid, ipType, &pEntry);
	if (pEntry && pEntry->enabled)
	{
		igmpconfig = getIgmpConfig();
		
		if (isAddrLarger(pEntry->ip, sip) || 
			(isZeroAddr(pEntry->ip) && isAddrLarger(sys_ip6, sip)))
		{
		    memcpy(pEntry->ip, sip, 16);
		    pEntry->timer = igmpconfig->otherQuerierPresentInterval;
		    pEntry->status = IGMP_NON_QUERIER;
		    pEntry->version = qver;
		}
		else if ((isAddrLarger(pEntry->ip, sys_ip6) && isAddrLarger(sip, sys_ip6)) ||
			(isZeroAddr(pEntry->ip) && isAddrLarger(sip, sys_ip6)))
		{
		    memcpy(pEntry->ip, sys_ip6, 16);
		    pEntry->status = IGMP_QUERIER;
		}
		else if (isAddrEqual(pEntry->ip, sip))
		{
		    pEntry->timer = igmpconfig->otherQuerierPresentInterval;
		    pEntry->status = IGMP_NON_QUERIER;
		    pEntry->version = qver;
		}
	}
	else if (pEntry == NULL)
	{
		SYS_DBG("%s():%d  Warring! Querier for VLAN-%d doesn't exist!\n", __FUNCTION__, __LINE__, vid);
	}

	return SYS_ERR_OK;
}
#endif

void mcast_send_general_query( igmp_querier_entry_t *qryPtr, uint8 version)
{
	if (NULL == qryPtr)
		return;

	if (MULTICAST_TYPE_IPV4 == qryPtr->ipType)
		mcast_igmp_send_general_query(qryPtr, version);
#ifdef SUPPORT_MLD_SNOOPING
	else
		mcast_mld_send_general_query(qryPtr, version);
#endif
}

void mcast_send_gs_query(igmp_group_entry_t *pGroup, sys_logic_port_t lport)
{
	igmp_querier_entry_t *pQueryEntry;
	
	if (NULL == pGroup)
		return;

	mcast_querier_db_get(pGroup->vid, pGroup->ipVersion, &pQueryEntry);
	if ((NULL == pQueryEntry) || 
		(DISABLED == pQueryEntry->enabled) || 
		(IGMP_QUERIER != pQueryEntry->status))
		return;
	
	if (MULTICAST_TYPE_IPV4 == pGroup->ipVersion)
		mcast_igmp_send_grp_specific_query(pGroup->vid, pGroup->groupAddr[0], lport);
#ifdef SUPPORT_MLD_SNOOPING
	else
	    mcast_mld_send_grp_specific_query(pGroup, lport);
#endif
}

void mcast_send_gss_query(igmp_group_entry_t *pGroup, uint32 *sip, uint16 numSrc, sys_logic_port_t lport)
{
	igmp_querier_entry_t *pQueryEntry;
	
	if (NULL == pGroup)
		return;

	mcast_querier_db_get(pGroup->vid, pGroup->ipVersion, &pQueryEntry);
	if ((NULL == pQueryEntry) || 
		(DISABLED == pQueryEntry->enabled) || 
		(IGMP_QUERIER != pQueryEntry->status))
		return;
	
	if (MULTICAST_TYPE_IPV4 == pGroup->ipVersion)
		mcast_igmp_send_grp_src_specific_query(pGroup->vid, pGroup->groupAddr[0], sip, numSrc, lport);
#ifdef SUPPORT_MLD_SNOOPING
	else
		  mcast_mld_send_grp_src_specific_query(pGroup, sip, numSrc, lport);
#endif
}

int32 mcast_igmp_querier_check(uint16 vid, uint32 sip, uint8 qver)
{
	igmp_querier_entry_t *pEntry;

	uint32 ipType = MULTICAST_TYPE_IPV4;
	igmp_mc_vlan_entry_t *pMcastVlan = NULL;
	igmp_config_t *igmpconfig;

	SYS_PARAM_CHK(!VALID_VLAN_ID(vid), SYS_ERR_VLAN_ID);

	pMcastVlan = mcast_mcVlan_entry_get(vid);
	if ((vid > 0) && (0 == pMcastVlan->vid))
	{
		return SYS_ERR_FAILED;
	}

	mcast_querier_db_get(vid, ipType, &pEntry);
	if (pEntry && pEntry->enabled)
	{
		igmpconfig = getIgmpConfig();
		
		if (sip < pEntry->ip[0] || (pEntry->ip[0] == 0 && sip < sys_ip))
		{
		    pEntry->ip[0] = sip;
		    pEntry->timer = igmpconfig->otherQuerierPresentInterval;
		    pEntry->status = IGMP_NON_QUERIER;
		    pEntry->version = qver;
		}
		else if ((sys_ip <= pEntry->ip[0] && sys_ip < sip) || (pEntry->ip[0] == 0 && sys_ip < sip))
		{
		    pEntry->ip[0] = sys_ip;
		    pEntry->status = IGMP_QUERIER;
		}
		else if (pEntry->ip[0] == sip)
		{
		    pEntry->timer = igmpconfig->otherQuerierPresentInterval;
		    pEntry->status = IGMP_NON_QUERIER;
		    pEntry->version = qver;
		}
	}
	else if (pEntry == NULL)
	{
		SYS_DBG("%s():%d  Warring! Querier for VLAN-%d doesn't exist!\n", __FUNCTION__, __LINE__, vid);
	}

	return SYS_ERR_OK;
}

void mcast_igmp_querier_timer(void)
{
	uint16                  i, time;
	igmp_querier_entry_t    *sortedArray ;
	static uint32           cnt;

	multicast_ipType_t ipType;
	igmp_config_t *igmpconfig;

	igmpconfig = getIgmpConfig();
	
	for (ipType = MULTICAST_TYPE_IPV4; ipType < MULTICAST_TYPE_END; ipType++)
	{
		sortedArray = querier_db[ipType].querier_entry;

		for (i = 0; i < querier_db[ipType].entry_num; i++)
		{
		    if (sortedArray[i].enabled)
		    {
				if (sortedArray[i].startupQueryCnt <= igmpconfig->robustness)
				{
					/* Tx querier pkt */
					mcast_send_general_query(&sortedArray[i], sortedArray[i].version);
					sortedArray[i].startupQueryCnt++;
				}

				time = sortedArray[i].timer;
				if (time != 0)
				{
					if (time > PASS_SECONDS)
					{
						if (time > igmpconfig->otherQuerierPresentInterval) /* If user change the time period, update it */
							sortedArray[i].timer = igmpconfig->otherQuerierPresentInterval;

						sortedArray[i].timer -= PASS_SECONDS;
					}
					else
					{
					    /* Tx querier pkt */
					    mcast_send_general_query(&sortedArray[i], sortedArray[i].version);
					    
					    /* If non-Querier switch doesn't receive any query packet before a 
					    igmp_stats.otherQuerierPresentInterval time, should clear ip */
					    if (MULTICAST_TYPE_IPV4 == ipType)
							sortedArray[i].ip[0] = sys_ip;
#ifdef SUPPORT_MLD_SNOOPING
						else
							memcpy(sortedArray[i].ip, sys_ip6, 16);
#endif
					    sortedArray[i].timer = igmpconfig->queryIntv;
					    sortedArray[i].status = IGMP_QUERIER;
					    osal_time_usleep(10 * 1000); /* sleep for 0.01 sec */
					}
				}
		    }
		}
	}
}

void mcast_igmp_querier_show()
{
	uint16  i;
	igmp_querier_entry_t    *sortedArray ;

	printf("*************igmp querier table*************\n");
	printf("Idx   Enable   Vid   Time IgmpVer  Status\n");

	sortedArray = querier_db[MULTICAST_TYPE_IPV4].querier_entry;

	for (i = 0; i < querier_db[MULTICAST_TYPE_IPV4].entry_num; i++)
	{	
		printf("%-3d  %-7s  %-4d   %-4d    %d   %-11s \n", i,  
			   	sortedArray[i].enabled ? "Enable" : "Disable",sortedArray[i].vid,			
				sortedArray[i].timer, sortedArray[i].version,
				(sortedArray[i].status == IGMP_QUERIER)?"Querier":"Not Querier");		
	}
}
