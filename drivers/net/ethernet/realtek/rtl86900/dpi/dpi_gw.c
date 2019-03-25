#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <net/icmp.h>
#include <net/ip.h>     /*  for local_port_range[] */
#include <net/tcp.h>        /*  struct or_callable used in sock_rcv_skb */
#include <asm/ioctls.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/netlink.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/ipv6.h>
// romeDriver header files
#if defined(CONFIG_RTK_L34_ENABLE)
#include <rtk_rg_struct.h>
#include <rtk_rg_internal.h>
#endif
// DPI header files
#include "dpi_ctx.h"
#include "dpi_gw.h"
// definition
#define DPI_MAX_HOOKS   8
#define MAX_NETDEV		32
// for debug
#define LOG_DEBUG   //printk
//#define LOCAL_DEBUG 1
// enumeration
enum {
    DPI_PRE_ROUTING,
    DPI_FORWARD,
};
// local structure
typedef struct netfilter_priv_info_
{
    int                 index;
    int                 hooknum;
} netfilter_priv_info_t;

typedef struct netdev_info_
{
	int ifindex;
	char name[IFNAMSIZ];
}netdev_info_t;

// global variable
static int  g_forward_count = 0;

static ctSgw_appGetInfoProc g_getinfo_func = NULL;
static ctSgw_appPreProc     g_prerouting_func[DPI_MAX_HOOKS];
static ctSgw_appForwardProc g_forward_func[DPI_MAX_HOOKS];
static ctSgw_appProcAppId   g_forward_appid = NULL;
static ctSgw_appCtxCreate   g_create_ctx = NULL;
static ctSgw_appCtxDestroy  g_destroy_ctx = NULL;
static netdev_info_t g_netdev_inf[MAX_NETDEV];
static CtSgwDPIStatistic g_dpi_stat;
static int qos_pri[10] = {0,6,5,4,3,2,1,0,0,0};
static unsigned short dev_eth_ifindex = 0;

void show_netdev_info(void)
{
	int ifidx = 0;
	for(ifidx = 0 ; ifidx < MAX_NETDEV; ifidx++)
	{
		if(g_netdev_inf[ifidx].ifindex == 0)
			break;
		LOG_DEBUG("%d Name:%s, ifindex:%d \n", ifidx, g_netdev_inf[ifidx].name, g_netdev_inf[ifidx].ifindex);
	}
}

void renew_netdev_info(void)
{
	struct net_device *dev;
	int ifidx = 0;
	memset(g_netdev_inf, 0x0 , sizeof(netdev_info_t) * MAX_NETDEV);
	
	read_lock(&dev_base_lock);
	dev = first_net_device(&init_net);
	while (dev) 
	{
		if((strncmp(dev->name, "nas0_" ,5) == 0) || (strlen(dev->name) == 4 && strncmp(dev->name, "eth0",4) == 0))
		{
			g_netdev_inf[ifidx].ifindex = dev->ifindex;
			strcpy(g_netdev_inf[ifidx].name, dev->name);
			if(strncmp(dev->name, "eth0",4) == 0)
				dev_eth_ifindex = g_netdev_inf[ifidx].ifindex;
			LOG_DEBUG("name:%s,ifindex:%d,idx:%d %p \n" , g_netdev_inf[ifidx].name,g_netdev_inf[ifidx].ifindex,ifidx, &g_netdev_inf[ifidx]);
			ifidx++;
		}
		else if(strlen(dev->name) == 4 && strncmp(dev->name, "eth0",4) == 0)
		{
			g_netdev_inf[ifidx].ifindex = dev->ifindex;
			strcpy(g_netdev_inf[ifidx].name, dev->name);	
			LOG_DEBUG("name:%s,ifindex:%d,idx:%d %p \n" , g_netdev_inf[ifidx].name,g_netdev_inf[ifidx].ifindex,ifidx, &g_netdev_inf[ifidx]);
			ifidx++;			
		}
		dev = next_net_device(dev);
	};
	read_unlock(&dev_base_lock);
}

int find_netdev_ifindex(char *name)
{
	int ifidx = 0;
	for(ifidx = 0 ; ifidx < MAX_NETDEV; ifidx++)
	{
		if(g_netdev_inf[ifidx].ifindex == 0)
			break;
		if(strcmp(name, g_netdev_inf[ifidx].name) == 0)
			return g_netdev_inf[ifidx].ifindex;
	}
	return -1;
}

#if defined(CONFIG_RTK_L34_ENABLE)
int DPI_naptPreRouteCallBack(void *data, rtk_rg_naptDirection_t direct)
{
    int i = 0;
    int ret = MAXNET_DPI_CON;
	rtk_rg_pktHdr_t *pPktHdr=(rtk_rg_pktHdr_t *)data;
	CtSgwTupleInfo tuple_info;
	unsigned char *layer2_data = NULL;
	dpi_gwinfo_t gwinfo;
	char outif[32]={0x0};
	int out_ifindex = 0;

	CtSgwDpiCtx *ctx = NULL;
	if(pPktHdr->naptOutboundIndx == -1)
		return RG_FWDENGINE_PREROUTECB_CONTINUE;
	g_dpi_stat.prerte_cnt++;
	ctx = (CtSgwDpiCtx *)(rg_db.naptOut[pPktHdr->naptOutboundIndx].pContext);
	if(ctx == NULL || ctx->dpi_context == NULL) 
	{
		g_dpi_stat.prerte_noctx++;
		return RG_FWDENGINE_PREROUTECB_CONTINUE;
	}
	if(g_dpi_stat.prerte_reghook <= 0)
		return RG_FWDENGINE_PREROUTECB_CONTINUE;
		
	LOG_DEBUG("PreRoute ctx:%p,layer7_id:%u,dpi_context:%p\n", ctx, ctx->layer7_id,ctx->dpi_context);
	skb_reset_mac_header(pPktHdr->skb);
	layer2_data = (uint8_t *) skb_mac_header(pPktHdr->skb);

	tuple_info.sipv4 = pPktHdr->ipv4Sip;
	tuple_info.dipv4 = pPktHdr->ipv4Dip;

	if((pPktHdr->tagif&TCP_TAGIF)||(pPktHdr->tagif&UDP_TAGIF))
	{
		tuple_info.sport = pPktHdr->sport;
		tuple_info.dport = pPktHdr->dport;
		if(pPktHdr->tagif&TCP_TAGIF)
			g_dpi_stat.prerte_tcp++;
		else
			g_dpi_stat.prerte_udp++;
	}
	else 
	{
		tuple_info.sport = tuple_info.dport = 0;
		g_dpi_stat.prerte_otherpro++;
	}

	tuple_info.direct = ((direct==NAPT_DIRECTION_OUTBOUND)?NAPT_DIRECTION_OUTBOUND:NAPT_DIRECTION_INBOUND);
	if(tuple_info.direct) g_dpi_stat.prerte_indir++;
	else g_dpi_stat.prerte_outdir++;
	if(pPktHdr->skb->dev == NULL)
		tuple_info.in_iif = dev_eth_ifindex;
	else tuple_info.in_iif = pPktHdr->skb->dev->ifindex;

	if(pPktHdr->netifIdx == 0) snprintf(outif, IFNAMSIZ, "eth0");
	else snprintf(outif, IFNAMSIZ, "nas0_%d", (pPktHdr->netifIdx-1));

	out_ifindex = find_netdev_ifindex(outif);
	if(unlikely(out_ifindex == -1)) //ifindex not found, should be first packet come in, so renew all netdev info.
	{
		renew_netdev_info();
		tuple_info.out_iif = find_netdev_ifindex(outif);
	}
	else
		tuple_info.out_iif = out_ifindex;

	if(rg_db.naptOut[pPktHdr->naptOutboundIndx].state==TCP_CONNECTED || 
		rg_db.naptOut[pPktHdr->naptOutboundIndx].state==UDP_CONNECTED)
		tuple_info.new_conntrack = 0;
	else 
	{
		tuple_info.new_conntrack = 1;
		g_dpi_stat.prerte_new++;
	}
	tuple_info.proto = pPktHdr->ipProtocol;
	
	LOG_DEBUG("PRE sipv4:%d.%d.%d.%d,dipv4:%d.%d.%d.%d,sport:%d,dport:%d,direct:%d,in_iif:%d,out_iif:%d,proto:%d\n",
	(tuple_info.sipv4& 0xff000000)>>24,(tuple_info.sipv4& 0xff0000)>>16, (tuple_info.sipv4&0xff00)>>8, (tuple_info.sipv4&0xff),
	(tuple_info.dipv4& 0xff000000)>>24,(tuple_info.dipv4& 0xff0000)>>16, (tuple_info.dipv4&0xff00)>>8, (tuple_info.dipv4&0xff),
	tuple_info.sport, tuple_info.dport,tuple_info.direct, tuple_info.in_iif,tuple_info.out_iif,tuple_info.proto);
	
	memset(&gwinfo, 0, sizeof(gwinfo));
	
	for (i = 0; i < DPI_MAX_HOOKS; i++) 
	{
		if (g_prerouting_func[i] != NULL) 
		{
			ret = (g_prerouting_func[i])(layer2_data, &tuple_info, ctx->dpi_context, &gwinfo, NULL);
			g_dpi_stat.prerte_scan++;
			if (ret == MAXNET_FDS_TRANS) {
				/* dnat */
				//nf_nat_setup_info(ct, &(gwinfo.nat_range), NF_NAT_MANIP_DST);
				pPktHdr->ipv4Dip = gwinfo.nat_range.min_addr.ip;
				if(pPktHdr->ipProtocol == RG_IP_PROTO_TCP)
					pPktHdr->dport = gwinfo.nat_range.min_proto.tcp.port;
				else if(pPktHdr->ipProtocol == RG_IP_PROTO_UDP)
					pPktHdr->dport = gwinfo.nat_range.min_proto.udp.port;
				else pPktHdr->dport = gwinfo.nat_range.min_proto.all;
			}
		}
	}
	if(ret == MAXNET_DPI_CON)
	{
		g_dpi_stat.prerte_conti++;
		return RG_FWDENGINE_PREROUTECB_CONTINUE;
	}
	else if(ret == MAXNET_FDS_TRANS)
	{
		g_dpi_stat.prerte_trap++;
		return RG_FWDENGINE_PREROUTECB_TRAP;
	}
	else if(ret == MAXNET_DPI_DNY)
	{
		g_dpi_stat.prerte_drop++;
		return RG_FWDENGINE_PREROUTECB_DROP;
	}
	else
	{
		LOG_DEBUG("(%d,%s)fatal error !! ignore this flow(%d)\n",__LINE__,__func__,ret);
		g_dpi_stat.prerte_errret++;
		return RG_FWDENGINE_PREROUTECB_CONTINUE;
	}
}

int DPI_naptForwardCallBack(void *data, rtk_rg_naptDirection_t direct)
{
    int i = 0;
    int fincnt = 0;
    int ret = MAXNET_DPI_CON;
	rtk_rg_pktHdr_t *pPktHdr=(rtk_rg_pktHdr_t *)data;
	CtSgwTupleInfo tuple_info;
	unsigned char *layer2_data = NULL;
	dpi_gwinfo_t gwinfo;
	unsigned int layer7_id = 0;
	char outif[32]={0x0}; 
	int out_ifindex = 0;
	//CtSgwDpiCtx *ctx = *((void **)(rg_db.naptOut[pPktHdr->naptOutboundIndx].pContext));
	CtSgwDpiCtx *ctx = NULL;
	if(pPktHdr->naptOutboundIndx == -1)
		return RG_FWDENGINE_FORWARDCB_FINISH_DPI;
	g_dpi_stat.forward_cnt++;
	ctx = (CtSgwDpiCtx *)(rg_db.naptOut[pPktHdr->naptOutboundIndx].pContext);
	if(ctx == NULL || ctx->dpi_context == NULL) 
	{
		g_dpi_stat.forward_noctx++;
		return RG_FWDENGINE_FORWARDCB_FINISH_DPI;
	}
	if(g_forward_appid == NULL && g_dpi_stat.forward_reghook <= 0)
		return RG_FWDENGINE_FORWARDCB_FINISH_DPI;
	
	LOG_DEBUG("Forward ctx:%p, layer7_id:%u, dpi_context:%p\n", ctx, ctx->layer7_id,ctx->dpi_context);
	skb_reset_mac_header(pPktHdr->skb);
	layer2_data = (uint8_t *) skb_mac_header(pPktHdr->skb);
	memset(&tuple_info, 0x0, sizeof(CtSgwTupleInfo));
	tuple_info.sipv4 = pPktHdr->ipv4Sip;
	tuple_info.dipv4 = pPktHdr->ipv4Dip;
	if((pPktHdr->tagif&TCP_TAGIF)||(pPktHdr->tagif&UDP_TAGIF))
	{
		tuple_info.sport = pPktHdr->sport;
		tuple_info.dport = pPktHdr->dport;
		if(pPktHdr->tagif&TCP_TAGIF)
			g_dpi_stat.forward_tcp++;
		else
			g_dpi_stat.forward_udp++;
	}
	else 
	{
		tuple_info.sport = tuple_info.dport = 0;
		g_dpi_stat.forward_otherpro++;
	}
	tuple_info.direct = ((direct==NAPT_DIRECTION_OUTBOUND)?NAPT_DIRECTION_OUTBOUND:NAPT_DIRECTION_INBOUND);
	if(tuple_info.direct) g_dpi_stat.forward_indir++;
	else g_dpi_stat.forward_outdir++;
	layer2_data = (uint8_t *) skb_mac_header(pPktHdr->skb);
	if(pPktHdr->skb->dev == NULL) tuple_info.in_iif = dev_eth_ifindex;
	else tuple_info.in_iif = pPktHdr->skb->dev->ifindex;
	if(pPktHdr->netifIdx == 0) snprintf(outif, IFNAMSIZ, "eth0");
	else snprintf(outif, IFNAMSIZ, "nas0_%d", (pPktHdr->netifIdx-1));
	out_ifindex = find_netdev_ifindex(outif);
	if(unlikely(out_ifindex == -1)) //ifindex not found, should be first packet come in, so renew all netdev info.
	{
		renew_netdev_info();
		tuple_info.out_iif = find_netdev_ifindex(outif);
	}
	else
		tuple_info.out_iif = out_ifindex;

	if(pPktHdr->naptOutboundIndx != -1 &&
		(rg_db.naptOut[pPktHdr->naptOutboundIndx].state==TCP_CONNECTED || 
		rg_db.naptOut[pPktHdr->naptOutboundIndx].state==UDP_CONNECTED)
		)
		tuple_info.new_conntrack = 0;
	else 
	{	
		tuple_info.new_conntrack = 1;
		g_dpi_stat.forward_new++;
	}
	tuple_info.proto = pPktHdr->ipProtocol;
	
	memset(&gwinfo, 0, sizeof(gwinfo));
	
	LOG_DEBUG("FWD sipv4:%d.%d.%d.%d,dipv4:%d.%d.%d.%d,sport:%d,dport:%d,direct:%d,in_iif:%d,out_iif:%d,proto:%d\n",
	(tuple_info.sipv4& 0xff000000)>>24,(tuple_info.sipv4& 0xff0000)>>16, (tuple_info.sipv4&0xff00)>>8, (tuple_info.sipv4&0xff),
	(tuple_info.dipv4& 0xff000000)>>24,(tuple_info.dipv4& 0xff0000)>>16, (tuple_info.dipv4&0xff00)>>8, (tuple_info.dipv4&0xff),
	tuple_info.sport, tuple_info.dport,tuple_info.direct, tuple_info.in_iif,tuple_info.out_iif,tuple_info.proto);
	
	if(g_forward_appid)
	{
		ret = (g_forward_appid)(layer2_data, &tuple_info, ctx->dpi_context, &layer7_id);
		g_dpi_stat.forward_scan++;
		if (layer7_id != 0) 
		{
			ctx->layer7_id = layer7_id;
		}

		if (ret == MAXNET_DPI_FIN) 
		{
			fincnt++;
        }
	}
	
	for (i = 0; i < DPI_MAX_HOOKS; i++) 
	{
		if (g_forward_func[i] != NULL) 
		{
			ret = (g_forward_func[i])(layer2_data, &tuple_info, ctx->dpi_context, &gwinfo, NULL);
			g_dpi_stat.forward_scan++;
			if (gwinfo.layer7_id != 0) 
			{
				ctx->layer7_id = gwinfo.layer7_id;
			}

			if (ret == MAXNET_DPI_FIN) 
			{
                    fincnt++;
            }
        }
	}
    if (fincnt == g_forward_count) {
        /* TODO: all hook finish, help yourself */
		if((ret&0xf) == MAXNET_DPI_PQOS)
		{
			unsigned int qos = ret >> 8;
			if(qos <= 9)
			{
				rg_db.naptOut[pPktHdr->naptOutboundIndx].rtk_naptOut.priValid=1;
				rg_db.naptOut[pPktHdr->naptOutboundIndx].rtk_naptOut.priValue=qos_pri[qos];
				rg_db.naptIn[pPktHdr->naptrInboundIndx].rtk_naptIn.priValid=1;
				rg_db.naptIn[pPktHdr->naptrInboundIndx].rtk_naptIn.priId=qos_pri[qos];
			}
			g_dpi_stat.forward_qos++;
		}
		
		g_dpi_stat.forward_fin++;
		return RG_FWDENGINE_FORWARDCB_FINISH_DPI;
    }
	if(ret == MAXNET_DPI_CON)
	{
		g_dpi_stat.forward_conti++;
		return RG_FWDENGINE_FORWARDCB_CONTINUE_DPI;
	}
	else if(ret == MAXNET_DPI_FIN)
	{
		g_dpi_stat.forward_fin++;
		return RG_FWDENGINE_FORWARDCB_FINISH_DPI;
	}
	else if(ret == MAXNET_DPI_DNY)
	{
		g_dpi_stat.forward_drop++;
		return RG_FWDENGINE_FORWARDCB_DROP;
	}
	else
	{
		printk("(%d,%s)fatal error !! ignore this flow(%d)\n",__LINE__,__func__,ret);
		g_dpi_stat.forward_errret++;
		return RG_FWDENGINE_FORWARDCB_FINISH_DPI;
	}
}
#endif

static int register_prerouting_hook(ctSgw_appPreProc hook, int priority, void **priv_data)
{
    int i = 0;
    netfilter_priv_info_t **info = NULL;
    
    if (hook == NULL) {
        return -1;
    }

    for (i = 0; i < DPI_MAX_HOOKS; i++) {
        if (g_prerouting_func[i] == NULL) {
            break;
        }
    }

    if (i == DPI_MAX_HOOKS) {
        LOG_DEBUG("%s: prerouting has max hooks\n", __func__);
        return -1;
    }

    if (!priv_data) {
        LOG_DEBUG("%s: priv data not init\n", __func__);
        return -1;
    }

    info = (netfilter_priv_info_t **)priv_data;
    *info = (netfilter_priv_info_t *)kmalloc(sizeof(netfilter_priv_info_t), GFP_ATOMIC);
    if (*info == NULL) {
        LOG_DEBUG("%s: malloc netfilter priv info failed\n", __func__);
        return -1;
    }

    memset(*info, 0, sizeof(netfilter_priv_info_t));
    (*info)->index = i + 1;
    (*info)->hooknum = DPI_PRE_ROUTING;
	g_prerouting_func[i] = hook;
	g_dpi_stat.prerte_reghook++;
    return 0;
}

static int register_forward_hook(ctSgw_appForwardProc hook, int priority, void **priv_data)
{
    int i = 0;
    netfilter_priv_info_t **info = NULL;

    for (i = 0; i < DPI_MAX_HOOKS; i++) {
        if (g_forward_func[i] == NULL) {
            break;
        }
    }

    if (i == DPI_MAX_HOOKS) {
        LOG_DEBUG("%s: forward has max hooks\n", __func__);
        return -1;
    }

    if (!priv_data) {
        LOG_DEBUG("%s: priv data not init\n", __func__);
        return -1;
    }

    info = (netfilter_priv_info_t **)priv_data;
    *info = (netfilter_priv_info_t *)kmalloc(sizeof(netfilter_priv_info_t), GFP_ATOMIC);
    if (*info == NULL) {
        LOG_DEBUG("%s: malloc netfilter priv info failed\n", __func__);
        return -1;
    }

    memset(*info, 0, sizeof(netfilter_priv_info_t));
    (*info)->index = i + 1;
    (*info)->hooknum = DPI_FORWARD;
	g_forward_func[i] = hook;
    g_forward_count++;
	g_dpi_stat.forward_reghook++;
    return 0;
}

static void unregister_netfilter_hook(void **priv_data)
{
    netfilter_priv_info_t *info = NULL;
    
    if (!priv_data) {
        LOG_DEBUG("%s: priv data not init\n", __func__);
        return;
    }

    info = *((netfilter_priv_info_t **)priv_data);
    if (info == NULL) {
        LOG_DEBUG("%s: info is null\n", __func__);
        return;
    }

    if (info->index < 1 || info->index > DPI_MAX_HOOKS) {
        LOG_DEBUG("%s: info index %d\n", __func__, info->index);
        kfree(info);
        *priv_data = NULL;
        return;
    }
    
    if (info->hooknum == DPI_PRE_ROUTING) {
        g_prerouting_func[info->index - 1] = NULL;
    }
    else if (info->hooknum == DPI_FORWARD) {
        g_forward_count--;
        g_forward_func[info->index - 1] = NULL;
    }

    kfree(info);

    *priv_data = NULL;
    return;
}

int ctSgw_appRegisterGetInfo(ctSgw_appGetInfoProc funcs)
{
    if (funcs == NULL) {
        return -1;
    }

    g_getinfo_func = funcs;
    return 0;
}

void ctSgw_appUnRegisterGetInfo(void)
{
    g_getinfo_func = NULL;
    return;
}

int ctSgw_appGetInfo(void *app_ctx, void *app_info)
{
    if (g_getinfo_func == NULL) {
        return -1;
    }

    return g_getinfo_func(app_ctx, app_info);
}

int ctSgw_appRegisterPreProc(CtSgwPreProcInfo *info)
{
    if (info == NULL) {
        return -1;
    }

    return register_prerouting_hook(info->hook, info->priority, &(info->priv_data));
}

void ctSgw_appUnRegisterPreProc(CtSgwPreProcInfo *info)
{
    if (info != NULL) {
        unregister_netfilter_hook(&(info->priv_data));
    }
    return;
}

int ctSgw_appRegisterForwardProc(CtSgwForwardProcInfo *info)
{
    if (info == NULL) {
        return -1;
    }

    return register_forward_hook(info->hook, info->priority, &(info->priv_data));
}

void ctSgw_appUnRegisterForwardProc(CtSgwForwardProcInfo *info)
{
    if (info != NULL) {
        unregister_netfilter_hook(&(info->priv_data));
    }
    return;
}

#if defined(CONFIG_RTK_L34_ENABLE)
int DPI_naptInfoAddCallBack(rtk_rg_naptInfo_t* naptInfo)
{
	#ifdef LOCAL_DEBUG
	static unsigned long ctx_cnt = 0;
	// kmem_cache_alloc?
	CtSgwDpiCtx *create_ctx = (CtSgwDpiCtx *)kmalloc(sizeof(CtSgwDpiCtx), GFP_KERNEL);
	if(create_ctx == NULL)
	{
		printk("Fatal Error : No Memory !!\n");
	}
	*naptInfo->pContext = create_ctx;
	create_ctx->layer7_id = ctx_cnt++;
	create_ctx->dpi_context = (void*)create_ctx;
	g_dpi_stat.create_context++;
	LOG_DEBUG("LOCAL Create Context(%p):layer7_id:%u,dpi_ctx:%p\n",create_ctx, create_ctx->layer7_id, create_ctx->dpi_context );
	return 0;
	#else
	if (g_create_ctx) 
	{
		int ret = 0;
		CtSgwDpiCtx *create_ctx =  (CtSgwDpiCtx *)kmalloc(sizeof(CtSgwDpiCtx), GFP_KERNEL);
		memset(create_ctx, 0x0, sizeof(CtSgwDpiCtx));
		create_ctx->dpi_context = NULL;
		ret = (g_create_ctx)(&(create_ctx->dpi_context), 0);
		g_dpi_stat.create_context++;
		create_ctx->layer7_id = 0;
        *naptInfo->pContext = create_ctx;
		LOG_DEBUG("DPI Create Context(%p):layer7_id:%u,dpi_ctx:%X\n",create_ctx, create_ctx->layer7_id, create_ctx->dpi_context );
		if (unlikely(ret != 0)) {
			LOG_DEBUG("%s, app_ctx_create failed on %d\n", __func__, ret);
		}
    }
	#endif
	return 0;
}

int DPI_naptInfoDeleteCallBack(rtk_rg_naptInfo_t* naptInfo)
{
	#ifdef LOCAL_DEBUG
	CtSgwDpiCtx *destroy_ctx = (CtSgwDpiCtx*)(*(naptInfo->pContext));
	*(naptInfo->pContext) = NULL;
	LOG_DEBUG("LOCAL Delete Context(%p):layer7_id:%u,dpi_ctx:%p\n", destroy_ctx, destroy_ctx->layer7_id,destroy_ctx->dpi_context);
	if(destroy_ctx) kfree(destroy_ctx);
	g_dpi_stat.destroy_context++;
	#else
	if (g_create_ctx != NULL) 
	{
		CtSgwDpiCtx *destroy_ctx = (CtSgwDpiCtx*)(*(naptInfo->pContext));
		if(destroy_ctx)
		{
			LOG_DEBUG("DPI Delete Context(%p):layer7_id:%u,dpi_ctx:%p\n", destroy_ctx, destroy_ctx->layer7_id,destroy_ctx->dpi_context);
        	    	(g_destroy_ctx)(&(destroy_ctx->dpi_context));
			*(naptInfo->pContext) = NULL;
			if(destroy_ctx) kfree(destroy_ctx);
			g_dpi_stat.destroy_context++;
		}
    }
	#endif
	return 0;
}
#endif

int ctSgw_appRegisterFunc(ctSgw_dpiFuncs *funcs)
{
    if (funcs == NULL)
        return -1;

    g_create_ctx = funcs->ctSgw_appCtxCreateHook;
    g_destroy_ctx = funcs->ctSgw_appCtxDestroyHook; 
	g_forward_appid = funcs->ctSgw_appProcAppIdHook;
	
    return 0;
}

void ctSgw_appUnRegisterFunc(void)
{
    g_create_ctx = NULL;
    g_destroy_ctx = NULL;
	g_forward_appid = NULL;
}

static void (*nf_conntrack_dpi_show)(struct seq_file *, void *) = NULL;
void nf_conntrack_procfs_register(void (*hook)(void *))
{
    nf_conntrack_dpi_show = hook;
}

static int dpi_stat_read(struct seq_file *seq, void *v)
{
	seq_printf(seq,"----------------------------------------\n");
	seq_printf(seq,"Context\n");
	seq_printf(seq,"Create:%08lu\n",g_dpi_stat.create_context);
	seq_printf(seq,"Error:%08lu\n",g_dpi_stat.create_ctx_err);
	seq_printf(seq,"Destroy:%08lu\n",g_dpi_stat.destroy_context);
	seq_printf(seq,"PreRoute Register Hook %08lu\n", g_dpi_stat.prerte_reghook);
	seq_printf(seq,"Forword Register Hook %08lu\n", g_dpi_stat.forward_reghook);
	seq_printf(seq,"----------------------------------------\n");
	seq_printf(seq,"Pre-Route\n");
	seq_printf(seq,"CNT:%08lu\n",g_dpi_stat.prerte_cnt);
	seq_printf(seq,"NOCTX:%08lu\n",g_dpi_stat.prerte_noctx);
	seq_printf(seq,"DNAT:%08lu\n",g_dpi_stat.prerte_dnat);
	seq_printf(seq,"InDir:%08lu\n",g_dpi_stat.prerte_indir);
	seq_printf(seq,"OutDir:%08lu\n",g_dpi_stat.prerte_outdir);
	seq_printf(seq,"Continue:%08lu\n",g_dpi_stat.prerte_conti);
	seq_printf(seq,"Trap:%08lu\n",g_dpi_stat.prerte_trap);
	seq_printf(seq,"Drop:%08lu\n",g_dpi_stat.prerte_drop);
	seq_printf(seq,"Scan:%08lu\n",g_dpi_stat.prerte_scan);
	seq_printf(seq,"New:%08lu\n",g_dpi_stat.prerte_new);
	seq_printf(seq,"TCP:%08lu\n",g_dpi_stat.prerte_tcp);
	seq_printf(seq,"UDP:%08lu\n",g_dpi_stat.prerte_udp);
	seq_printf(seq,"OtherPro:%08lu\n",g_dpi_stat.prerte_otherpro);
	seq_printf(seq,"ErrorRet:%08lu\n",g_dpi_stat.prerte_errret);
	seq_printf(seq,"----------------------------------------\n");
	seq_printf(seq,"Forward\n");
	seq_printf(seq,"CNT:%08lu\n",g_dpi_stat.forward_cnt);
	seq_printf(seq,"NOCTX:%08lu\n",g_dpi_stat.forward_noctx);
	seq_printf(seq,"InDor:%08lu\n",g_dpi_stat.forward_indir);
	seq_printf(seq,"OutDir:%08lu\n",g_dpi_stat.forward_outdir);
	seq_printf(seq,"Continue:%08lu\n",g_dpi_stat.forward_conti);
	seq_printf(seq,"Finish:%08lu\n",g_dpi_stat.forward_fin);
	seq_printf(seq,"Drop:%08lu\n",g_dpi_stat.forward_drop);
	seq_printf(seq,"QoS:%08lu\n",g_dpi_stat.forward_qos);
	seq_printf(seq,"Scan:%08lu\n",g_dpi_stat.forward_scan);
	seq_printf(seq,"New:%08lu\n",g_dpi_stat.forward_new);
	seq_printf(seq,"TCP:%08lu\n",g_dpi_stat.forward_tcp);
	seq_printf(seq,"UDP:%08lu\n",g_dpi_stat.forward_udp);
	seq_printf(seq,"OtherPro:%08lu\n",g_dpi_stat.forward_otherpro);
	seq_printf(seq,"ErrorRet:%08lu\n",g_dpi_stat.forward_errret);
	seq_printf(seq,"----------------------------------------\n");
/*	
	if (nf_conntrack_dpi_show) 
	{
		int i;
		for(i = 0 ; i < MAX_NAPT_OUT_SW_TABLE_SIZE; i++)
		{
			if(rg_db.naptOut[i].rtk_naptOut.valid)
			{
				CtSgwDpiCtx *ctx = NULL;
				ctx = (CtSgwDpiCtx *)(rg_db.naptOut[i].pContext);
				if(ctx) (*nf_conntrack_dpi_show)(seq, ctx->dpi_context);
			}
		}
    }
*/
	seq_printf(seq,"----------------------------------------\n");
	return 0;
}

static int dpi_stat_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char flag;
	if (buffer && !copy_from_user(&flag, buffer, sizeof(flag))) 
	{
		switch(flag) 
		{
			case '0': // reset dpi statistic
			memset(&g_dpi_stat, 0x0 , sizeof(CtSgwDPIStatistic));
			break;
			default:
			return -EFAULT;
		}
		return count;
	}
	return -EFAULT;
}

static int dpi_stat_open(struct inode *inode, struct file *file)
{
        return single_open(file, dpi_stat_read, inode->i_private);
}
static const struct file_operations dpi_stat_fops = {
        .owner          = THIS_MODULE,
        .open           = dpi_stat_open,
        .read           = seq_read,
        .write          = dpi_stat_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int __init moduledpi_init(void)
{
	proc_create_data("dpi", 0644, NULL,&dpi_stat_fops, NULL);
	memset(&g_dpi_stat, 0x0 , sizeof(CtSgwDPIStatistic));
    LOG_DEBUG("Yueme DPI interface Loaded\n");
    return 0;
}

static void __exit moduledpi_exit(void)
{
    synchronize_net();
    
    LOG_DEBUG("Yueme DPI interface Unloaded\n");
    return;
}

EXPORT_SYMBOL(ctSgw_appRegisterGetInfo);
EXPORT_SYMBOL(ctSgw_appUnRegisterGetInfo);
EXPORT_SYMBOL(ctSgw_appGetInfo);
EXPORT_SYMBOL(ctSgw_appRegisterPreProc);
EXPORT_SYMBOL(ctSgw_appUnRegisterPreProc);
EXPORT_SYMBOL(ctSgw_appRegisterForwardProc);
EXPORT_SYMBOL(ctSgw_appUnRegisterForwardProc);
EXPORT_SYMBOL(ctSgw_appRegisterFunc);
EXPORT_SYMBOL(ctSgw_appUnRegisterFunc);
EXPORT_SYMBOL(nf_conntrack_procfs_register);
EXPORT_SYMBOL(DPI_naptInfoAddCallBack);
EXPORT_SYMBOL(DPI_naptInfoDeleteCallBack);
EXPORT_SYMBOL(DPI_naptPreRouteCallBack);
EXPORT_SYMBOL(DPI_naptForwardCallBack);

module_init(moduledpi_init);
module_exit(moduledpi_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("maxnet");
