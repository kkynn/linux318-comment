

#define MAXNET_FDS_ERR          (-1)
#define MAXNET_FDS_TRANS        (3)

/* 64 Bytes */
typedef struct dpi_appinfo_
{
    unsigned char       valid_mac;
    unsigned char       unused;
    unsigned char       user_mac[6];
    
    /* app id */
    unsigned int        related_app_id;
    unsigned int        cache_app_id;
    unsigned int        dev_id;
    unsigned int        fds_pkt_id;

    /* pad */
    unsigned char       pad[40];
} dpi_appinfo_t;

union dpi_inet_addr {
	__u32           all[4];
	__be32          ip;
	__be32          ip6[4];
	struct in_addr  in;
	struct in6_addr in6;
};
 
 union dpi_conntrack_man_proto {
	/* Add other protocols here. */
	__be16 all;

	struct {
	__be16 port;
	} tcp;
	struct {
	__be16 port;
	} udp;
	struct {
	__be16 id;
	} icmp;
	struct {
	__be16 port;
	} dccp;
	struct {
	__be16 port;
	} sctp;
	struct {
	__be16 key;     /* GRE key is 32bit, PPtP only uses 16bit */
	} gre;
};

struct dpi_nat_range 
{
	unsigned int                     flags;
	union dpi_inet_addr              min_addr;
	union dpi_inet_addr              max_addr;
	union dpi_conntrack_man_proto    min_proto;
	union dpi_conntrack_man_proto    max_proto;
};

typedef struct dpi_gwinfo_
{
    unsigned int                layer7_id;
	struct dpi_nat_range         nat_range;
} dpi_gwinfo_t;

typedef dpi_gwinfo_t         CtSgwGwInfo;

typedef int (*ctSgw_appGetInfoProc)(void *app_ctx, void *app_info);

int ctSgw_appRegisterGetInfo(ctSgw_appGetInfoProc funcs);
void ctSgw_appUnRegisterGetInfo(void);
int ctSgw_appGetInfo(void *app_ctx, void *app_info);

typedef int (*ctSgw_appPreProc)(unsigned char *layer2data, CtSgwTupleInfo *tupleinfo,
        void *p_app_ctx, CtSgwGwInfo *gwinfo, void *data);

typedef struct CtSgwPreProcInfo_
{
    ctSgw_appPreProc    hook;
    int                 priority;
    void                *priv_data;
} CtSgwPreProcInfo;

int ctSgw_appRegisterPreProc(CtSgwPreProcInfo *info);
void ctSgw_appUnRegisterPreProc(CtSgwPreProcInfo *info);

typedef int (*ctSgw_appForwardProc)(unsigned char *layer2data, CtSgwTupleInfo *tupleinfo,
        void *p_app_ctx, CtSgwGwInfo *gwinfoi, void *data);

typedef struct CtSgwForwardProcInfo_
{
    ctSgw_appForwardProc    hook;
    int                     priority;
    void                    *priv_data;
} CtSgwForwardProcInfo;

int ctSgw_appRegisterForwardProc(CtSgwForwardProcInfo *info);
void ctSgw_appUnRegisterForwardProc(CtSgwForwardProcInfo *info);

typedef struct CtSgwDPIStatistic_
{
	//create/destroy context;
	unsigned long create_context;
	unsigned long create_ctx_err;
	unsigned long destroy_context;
	//register/ungister
	unsigned long prerte_reghook;
	unsigned long forward_reghook;
	//pre-route 
	unsigned long prerte_cnt;
	unsigned long prerte_noctx;
	unsigned long prerte_dnat;
	unsigned long prerte_indir;
	unsigned long prerte_outdir;
	unsigned long prerte_conti;
	unsigned long prerte_trap;
	unsigned long prerte_drop;
	unsigned long prerte_scan;
	unsigned long prerte_new;
	unsigned long prerte_tcp;
	unsigned long prerte_udp;
	unsigned long prerte_otherpro;
	unsigned long prerte_errret;
	//forward 
	unsigned long forward_cnt;
	unsigned long forward_noctx;
	unsigned long forward_indir;
	unsigned long forward_outdir;
	unsigned long forward_conti;
	unsigned long forward_fin;
	unsigned long forward_drop;
	unsigned long forward_scan;
	unsigned long forward_new;
	unsigned long forward_tcp;
	unsigned long forward_udp;
	unsigned long forward_otherpro;
	unsigned long forward_errret;
	unsigned long forward_qos;
}CtSgwDPIStatistic;



