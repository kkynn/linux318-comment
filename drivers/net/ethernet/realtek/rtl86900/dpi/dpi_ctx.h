
#define MAXNET_GROUPID(id) ((((id) & 0xFF000000) >> 24) & 0x007F)
#define MAXNET_APPID(id)   ((id) & 0x0000FFFF)

#define MAXNET_DPI_ERR     (-1)
#define MAXNET_DPI_CON     (0)
#define MAXNET_DPI_FIN     (1)
#define MAXNET_DPI_DNY     (2)  /* drop this packet */
#define MAXNET_DPI_PQOS    (4)

typedef struct dpi_ctx_
{
	unsigned int layer7_id;
	void *dpi_context;
}dpi_ctx_t;

typedef struct tupleinfo_ {
    unsigned char direct;
    unsigned char proto;
    unsigned char new_conntrack;
    unsigned int sipv4;
    unsigned int dipv4;
    unsigned int sipv6[4];
    unsigned int dipv6[4];
    unsigned short sport;
    unsigned short dport;
    unsigned int in_iif;
    unsigned int out_iif;
} tupleinfo_t;

typedef dpi_ctx_t	CtSgwDpiCtx;
typedef tupleinfo_t       CtSgwTupleInfo;
typedef  int  (*ctSgw_appCtxCreate)(void** p_app_ctx, unsigned char flag);
typedef  void (*ctSgw_appCtxDestroy)(void** p_app_ctx);
typedef  int (*ctSgw_appProcAppId)(unsigned char *layer2data, CtSgwTupleInfo *tupleinfo, void *p_app_ctx, unsigned int *layer7_id);

typedef struct ctSgw_dpiFuncs_ {
    ctSgw_appCtxCreate  ctSgw_appCtxCreateHook;
    ctSgw_appCtxDestroy ctSgw_appCtxDestroyHook;
    ctSgw_appProcAppId  ctSgw_appProcAppIdHook;
} ctSgw_dpiFuncs;
 
int ctSgw_appRegisterFunc(ctSgw_dpiFuncs  *funcs);
void ctSgw_appUnRegisterFunc(void);

