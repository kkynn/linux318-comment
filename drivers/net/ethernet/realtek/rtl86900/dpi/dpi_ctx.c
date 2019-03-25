#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <net/netlabel.h>
#include <rtk_rg_struct.h>
#include "dpi_ctx.h"

#define LOG_DEBUG printk

static ctSgw_appCtxCreate   g_create_func = NULL;
static ctSgw_appCtxDestroy  g_destroy_func = NULL;
static ctSgw_appProcAppId   g_forward_func = NULL;


int DPI_naptInfoAddCallBack(rtk_rg_naptInfo_t* naptInfo)
{
//	LOG_DEBUG("come napt add\n");
	if (g_create_func) 
	{
		int ret = 0;
		CtSgwDpiCtx *create_ctx = (CtSgwDpiCtx*)(*(naptInfo->pContext));
            ret = (g_create_func)(&(create_ctx->dpi_context), 0);
            create_ctx->layer7_id = 0;
            if (unlikely(ret != 0)) {
                LOG_DEBUG("%s, app_ctx_create failed on %d\n", __func__, ret);
            }
        }
	return 0;
}

int DPI_naptInfoDeleteCallBack(rtk_rg_naptInfo_t* naptInfo)
{
//	LOG_DEBUG("come napt del\n");
	if (g_destroy_func != NULL) 
	{
		CtSgwDpiCtx *destroy_ctx = (CtSgwDpiCtx*)(*(naptInfo->pContext));
            (g_destroy_func)(&(destroy_ctx->dpi_context));
        }
	return 0;
}

int ctSgw_appRegisterFunc(ctSgw_dpiFuncs *funcs)
{
    if (funcs == NULL)
        return -1;

    g_create_func = funcs->ctSgw_appCtxCreateHook;
    g_destroy_func = funcs->ctSgw_appCtxDestroyHook; 
    g_forward_func = funcs->ctSgw_appProcAppIdHook;
    return 0;
}

void ctSgw_appUnRegisterFunc(void)
{
    g_create_func = NULL;
    g_destroy_func = NULL;
    g_forward_func = NULL;
}
 
EXPORT_SYMBOL(ctSgw_appRegisterFunc);
EXPORT_SYMBOL(ctSgw_appUnRegisterFunc);

static int __init moduledpi_init(void)
{
    LOG_DEBUG("MAXNET DPI interface Loaded\n");
    return 0;
}

static void __exit moduledpi_exit(void)
{
    synchronize_net();
    
    LOG_DEBUG("MAXNET DPI interface Unloaded\n");
    return;
}

module_init(moduledpi_init);
module_exit(moduledpi_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("maxnet");

