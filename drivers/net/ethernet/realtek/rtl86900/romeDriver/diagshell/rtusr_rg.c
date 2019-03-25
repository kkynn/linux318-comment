/*
 * Include Files
 */
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <math.h>
#include <osal/lib.h>
#include <rtk/rtusr/include/rtusr_util.h>

#include <rtdrv_rg_netfilter.h>


#include <common/error.h>	//redefine in rtk_rg_liteRomeDriver.h as CONFIG_APOLLO_MP
#include <common/rt_type.h>	//redefine in rtk_rg_liteRomeDriver.h as CONFIG_APOLLO_MP
#include <rtk/port.h>

#ifdef CONFIG_APOLLO_RLE0371
#include <hal/chipdef/apollo/apollo_def.h>
#else
#include <hal/chipdef/apollomp/apollomp_def.h>
#endif

#include <rtk_rg_liteRomeDriver.h>

//#include <rtk_rg_apolloPro_asicDriver.h>

/*
 * Function Declaration
 */
#if defined(CONFIG_RG_FLOW_BASED_PLATFORM)
#if defined(CONFIG_RG_RTL9607C_SERIES)
/* Module Name    : LiteRomeDriver   */
/* Sub-module Name: System configuration */

/* Function Name:
 *	rtk_rg_asic_fb_init
 * Description:
 *      Init pro asic
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 * Note:
 *      None
 */
//rtk_rg_err_code_t call_rtk_rg_asic_fb_init(void)
rtk_rg_err_code_t
rtk_rg_asic_fb_init(void)
{
	rtdrv_rg_asic_fb_init_t cfg;
	/* function body */
	//printf("[rtusr_rg.c,%s,%d]\n", __FUNCTION__, __LINE__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FB_INIT, &cfg, rtdrv_rg_asic_fb_init_t, 1);
	//SETSOCKOPT(RTDRV_RG_ASIC_FB_INIT, NULL, NULL, 1);
	//printf("[rtusr_rg.c,%s,%d]\n", __FUNCTION__, __LINE__); //user space

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_fb_init */

/* Function Name:
 *      rtdrv_rg_asic_dump_table_idx
 * Description:
 *      dump valid entry of flow table
 * Input:
 *      idx - index of netif table
 * Output:
 *      
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32 
rtdrv_rg_asic_dump_flow_table_idx(uint32 idx)
{
    rtdrv_rg_asic_dump_flow_table_idx_t cfg;

    /* function body */
    osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_ASIC_DUMP_FLOW_TABLE_IDX, &cfg, rtdrv_rg_asic_dump_flow_table_idx_t, 1);

    return RT_ERR_OK;
}   /* end of rtdrv_rg_asic_dump_flow_table_idx */


/* Function Name:
 *      rtdrv_rg_asic_dump_table_all
 * Description:
 *      dump valid entry of flow table
 * Input:
 *      
 * Output:
 *      
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32 
rtdrv_rg_asic_dump_flow_table_all(void)
{
    //rtdrv_rg_asic_dump_flow_table_all_t cfg;

    /* function body */
    //GETSOCKOPT(RTDRV_RG_ASIC_DUMP_FLOW_TABLE_ALL, &cfg, rtdrv_rg_asic_dump_flow_table_all_t, 1);
    GETSOCKOPT(RTDRV_RG_ASIC_DUMP_FLOW_TABLE_ALL, NULL, NULL, 1);

    return RT_ERR_OK;
}   /* end of rtdrv_rg_asic_dump_flow_table_all */

/* Function Name:
 *      rtdrv_rg_asic_dump_netif_table_idx
 * Description:
 *      dump valid entry of flow table
 * Input:
 *      idx - index of netif table
 * Output:
 *      
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32 
rtdrv_rg_asic_dump_netif_table_idx(uint32 idx)
{
    rtdrv_rg_asic_dump_netif_table_idx_t cfg;

    /* function body */
    osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_ASIC_DUMP_NETIF_TABLE_IDX, &cfg, rtdrv_rg_asic_dump_netif_table_idx_t, 1);

    return RT_ERR_OK;
}   /* end of rtdrv_rg_asic_dump_netif_table_idx */


/* Function Name:
 *      rtdrv_rg_asic_dump_netif_table_all
 * Description:
 *      dump valid entry of flow table
 * Input:
 *      
 * Output:
 *      
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32 
rtdrv_rg_asic_dump_netif_table_all(void)
{
    //rtdrv_rg_asic_dump_netif_table_all_t cfg;

    /* function body */
    //GETSOCKOPT(RTDRV_RG_ASIC_DUMP_NETIF_TABLE_ALL, &cfg, rtdrv_rg_asic_dump_neitf_table_all_t, 1);
    GETSOCKOPT(RTDRV_RG_ASIC_DUMP_NETIF_TABLE_ALL, NULL, NULL, 1);

    return RT_ERR_OK;
}   /* end of rtdrv_rg_asic_dump_netif_table_all */




/* Function Name:
 *      rtk_rg_asic_netifMib_get
 * Description:
 *      Get netif table entry
 * Input:
 *      idx - index of netif table
 * Output:
 *      *entry -point of netif entry result
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t 
rtk_rg_asic_netifMib_get(uint32 idx, rtk_rg_asic_netifMib_entry_t *pNetifMibEntry)
{
	rtdrv_rg_asic_netifMib_get_t cfg;
	
	/* parameter check */
	//printf("%s\n", __FUNCTION__); //user space
	RT_PARAM_CHK((NULL == pNetifMibEntry), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pNetifMibEntry, pNetifMibEntry, sizeof(rtk_rg_asic_netifMib_entry_t));
	GETSOCKOPT(RTDRV_RG_ASIC_NETIFMIB_GET, &cfg, rtdrv_rg_asic_netifMib_get_t, 1);
	osal_memcpy(pNetifMibEntry, &cfg.pNetifMibEntry, sizeof(rtk_rg_asic_netifMib_entry_t));
	return RT_ERR_OK;
}

/* Function Name:
 *	rtk_rg_asic_netifTable_set 
 * Description:
 *      Set netif table entry
 * Input:
 *      idx - index of netif table
 *      *entry -point of netif entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t 
rtk_rg_asic_netifTable_add(uint32 idx, rtk_rg_asic_netif_entry_t *pNetifEntry)
{
	rtdrv_rg_asic_netifTable_add_t cfg;

	/* parameter check */
	//printf(" [%s,%s,%d\n", __file__, __FUNCTION__, __LINE__); //user space
	RT_PARAM_CHK((NULL == pNetifEntry), RT_ERR_NULL_POINTER);
	//printf(" [%s,%s,%d\n", __file__, __FUNCTION__, __LINE__); //user space

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pNetifEntry, pNetifEntry, sizeof(rtk_rg_asic_netif_entry_t));
	SETSOCKOPT(RTDRV_RG_ASIC_NETIFTABLE_ADD, &cfg, rtdrv_rg_asic_netifTable_add_t, 1);
	//osal_memcpy(pNetifEntry, &cfg.pNetifEntry, sizeof(rtk_rg_asic_netif_entry_t)); no need

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_netifTable_add */


/* Function Name:
 *      rtk_rg_asic_netifTable_get
 * Description:
 *      Get netif table entry
 * Input:
 *      idx - index of netif table
 * Output:
 *      *entry -point of netif entry result
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t 
rtk_rg_asic_netifTable_get(uint32 idx, rtk_rg_asic_netif_entry_t *pNetifEntry)
{
	rtdrv_rg_asic_netifTable_get_t cfg;

	/* parameter check */
	//printf("%s\n", __FUNCTION__); //user space
	RT_PARAM_CHK((NULL == pNetifEntry), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pNetifEntry, pNetifEntry, sizeof(rtk_rg_asic_netif_entry_t));
	GETSOCKOPT(RTDRV_RG_ASIC_NETIFTABLE_GET, &cfg, rtdrv_rg_asic_netifTable_get_t, 1);
	osal_memcpy(pNetifEntry, &cfg.pNetifEntry, sizeof(rtk_rg_asic_netif_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_netifTable_get */


/* Function Name:
 *      rtk_rg_asic_netifTable_del
 * Description:
 *      Get netif table entry
 * Input:
 *      idx - index of netif table
 * Output:
 *      *entry -point of netif entry result
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t 
rtk_rg_asic_netifTable_del(uint32 idx)
{
	rtdrv_rg_asic_netifTable_del_t cfg;

	/* function body */
	//printf("%s\n", __FUNCTION__); //user space
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	SETSOCKOPT(RTDRV_RG_ASIC_NETIFTABLE_DEL, &cfg, rtdrv_rg_asic_netifTable_del_t, 1);

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_netifTable_del */


/* Function Name:
 *      rtk_rg_asic_flowPath_del
 * Description:
 *      Del flow table entry
 * Input:
 *      idx - index of flow table
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath_del(uint32 idx)
{
	rtdrv_rg_asic_flowPath_del_t cfg;

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH_DEL, &cfg, rtdrv_rg_asic_flowPath_del_t, 1);

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath_del */


/* Function Name:
 *      rtk_rg_asic_flowPath1_add
 * Description:
 *      Add flow table path1 entry
 * Input:
 *      *idx - index of flow table
 *      *pP1Data - point of path1 entry
 *      igrSVID
 *      igrCVID
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath1_add(uint32 *idx, rtk_rg_asic_path1_entry_t *pP1Data, uint16 igrSVID, uint16 igrCVID)
{
	rtdrv_rg_asic_flowPath1_add_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == idx), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pP1Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, idx, sizeof(uint32));
	osal_memcpy(&cfg.pP1Data, pP1Data, sizeof(rtk_rg_asic_path1_entry_t));
	osal_memcpy(&cfg.igrSVID, &igrSVID, sizeof(uint16));
	osal_memcpy(&cfg.igrCVID, &igrCVID, sizeof(uint16));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH1_ADD, &cfg, rtdrv_rg_asic_flowPath1_add_t, 1);
	osal_memcpy(idx, &cfg.idx, sizeof(uint32));
	osal_memcpy(pP1Data, &cfg.pP1Data, sizeof(rtk_rg_asic_path1_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath1_add */


/* Function Name:
 *      rtk_rg_asic_flowPath1_set
 * Description:
 *      Set flow table path1 entry
 * Input:
 *      idx - index of flow table
 *      *pP1Data - point of path1 entry
 * Output:
 *	None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath1_set(uint32 idx, rtk_rg_asic_path1_entry_t *pP1Data)
{
	rtdrv_rg_asic_flowPath1_set_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP1Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP1Data, pP1Data, sizeof(rtk_rg_asic_path1_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH1_SET, &cfg, rtdrv_rg_asic_flowPath1_set_t, 1);
	osal_memcpy(pP1Data, &cfg.pP1Data, sizeof(rtk_rg_asic_path1_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath1_set */


/* Function Name:
 *      rtk_rg_asic_flowPath1_get
 * Description:
 *      Get flow table path1 entry
 * Input:
 *      idx - index of flow table
 * Output:
 *      *pP1Data - point of path1 entry
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath1_get(uint32 idx, rtk_rg_asic_path1_entry_t *pP1Data)
{
	rtdrv_rg_asic_flowPath1_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP1Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP1Data, pP1Data, sizeof(rtk_rg_asic_path1_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	GETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH1_GET, &cfg, rtdrv_rg_asic_flowPath1_get_t, 1);
	osal_memcpy(pP1Data, &cfg.pP1Data, sizeof(rtk_rg_asic_path1_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath1_get */


/* Function Name:
 *      rtk_rg_asic_flowPath2_add
 * Description:
 *      Add flow table path2 entry
 * Input:
 *      *idx - index of flow table
 *      *pP2Data - point of path2 entry
 *      igrSVID
 *      igrCVID
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath2_add(uint32 *idx, rtk_rg_asic_path2_entry_t *pP2Data, uint16 igrSVID, uint16 igrCVID)
{
	rtdrv_rg_asic_flowPath2_add_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == idx), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pP2Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, idx, sizeof(uint32));
	osal_memcpy(&cfg.pP2Data, pP2Data, sizeof(rtk_rg_asic_path2_entry_t));
	osal_memcpy(&cfg.igrSVID, &igrSVID, sizeof(uint16));
	osal_memcpy(&cfg.igrCVID, &igrCVID, sizeof(uint16));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH2_ADD, &cfg, rtdrv_rg_asic_flowPath2_add_t, 1);
	osal_memcpy(idx, &cfg.idx, sizeof(uint32));
	osal_memcpy(pP2Data, &cfg.pP2Data, sizeof(rtk_rg_asic_path2_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath2_add */


/* Function Name:
 *      rtk_rg_asic_flowPath2_set
 * Description:
 *      Set flow table path2 entry
 * Input:
 *      idx - index of flow table
 *      *pP2Data - point of path2 entry
 * Output:
 *	None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath2_set(uint32 idx, rtk_rg_asic_path2_entry_t *pP2Data)
{
	rtdrv_rg_asic_flowPath2_set_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP2Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP2Data, pP2Data, sizeof(rtk_rg_asic_path2_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH2_SET, &cfg, rtdrv_rg_asic_flowPath2_set_t, 1);
	osal_memcpy(pP2Data, &cfg.pP2Data, sizeof(rtk_rg_asic_path2_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath2_set */


/* Function Name:
 *      rtk_rg_asic_flowPath2_get
 * Description:
 *      get flow table path2 entry
 * Input:
 *      idx - index of flow table
 * Output:
 *      *pP2Data - point of path2 entry
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath2_get(uint32 idx, rtk_rg_asic_path2_entry_t *pP2Data)
{
	rtdrv_rg_asic_flowPath2_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP2Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP2Data, pP2Data, sizeof(rtk_rg_asic_path2_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	GETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH2_GET, &cfg, rtdrv_rg_asic_flowPath2_get_t, 1);
	osal_memcpy(pP2Data, &cfg.pP2Data, sizeof(rtk_rg_asic_path2_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath2_get */


/* Function Name:
 *      rtk_rg_asic_flowPath3_add
 * Description:
 *      Add flow table path3 entry
 * Input:
 *      *idx - index of flow table
 *      *pP3Data - point of path3 entry
 *      igrSVID
 *      igrCVID
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath3_add(uint32 *idx, rtk_rg_asic_path3_entry_t *pP3Data, uint16 igrSVID, uint16 igrCVID)
{
	rtdrv_rg_asic_flowPath3_add_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == idx), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pP3Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, idx, sizeof(uint32));
	osal_memcpy(&cfg.pP3Data, pP3Data, sizeof(rtk_rg_asic_path3_entry_t));
	osal_memcpy(&cfg.igrSVID, &igrSVID, sizeof(uint16));
	osal_memcpy(&cfg.igrCVID, &igrCVID, sizeof(uint16));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH3_ADD, &cfg, rtdrv_rg_asic_flowPath3_add_t, 1);
	osal_memcpy(idx, &cfg.idx, sizeof(uint32));
	osal_memcpy(pP3Data, &cfg.pP3Data, sizeof(rtk_rg_asic_path3_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath3_add */


/* Function Name:
 *      rtk_rg_asic_flowPath3DAHash_add
 * Description:
 *      Add flow table path3 entry
 * Input:
 *      *idx - index of flow table
 *      *pP3Data - point of path3 entry
 *      igrSVID
 *      igrCVID
 *      lutDaIdx
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath3DAHash_add(uint32 *idx, rtk_rg_asic_path3_entry_t * pP3Data, uint16 igrSVID, uint16 igrCVID, uint16 lutDaIdx)
{
	rtdrv_rg_asic_flowPath3DAHash_add_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == idx), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pP3Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, idx, sizeof(uint32));
	osal_memcpy(&cfg.pP3Data, pP3Data, sizeof(rtk_rg_asic_path3_entry_t));
	osal_memcpy(&cfg.igrSVID, &igrSVID, sizeof(uint16));
	osal_memcpy(&cfg.igrCVID, &igrCVID, sizeof(uint16));
	osal_memcpy(&cfg.lutDaIdx, &lutDaIdx, sizeof(uint16));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH3DAHASH_ADD, &cfg, rtdrv_rg_asic_flowPath3DAHash_add_t, 1);
	osal_memcpy(idx, &cfg.idx, sizeof(uint32));
	osal_memcpy(pP3Data, &cfg.pP3Data, sizeof(rtk_rg_asic_path3_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath3DAHash_add */


/* Function Name:
 *      rtk_rg_asic_flowPath3_set
 * Description:
 *      Set flow table path3 entry
 * Input:
 *      idx - index of flow table
 *      *pP3Data - point of path3 entry
 * Output:
 *	None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath3_set(uint32 idx, rtk_rg_asic_path3_entry_t *pP3Data)
{
	rtdrv_rg_asic_flowPath3_set_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP3Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP3Data, pP3Data, sizeof(rtk_rg_asic_path3_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	GETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH3_SET, &cfg, rtdrv_rg_asic_flowPath3_set_t, 1);
	osal_memcpy(pP3Data, &cfg.pP3Data, sizeof(rtk_rg_asic_path3_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath3_set */


/* Function Name:
 *      rtk_rg_asic_flowPath3_get
 * Description:
 *      Get flow table path3 entry
 * Input:
 *      idx - index of flow table
 * Output:
 *      *pP3Data - point of path3 entry
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath3_get(uint32 idx, rtk_rg_asic_path3_entry_t *pP3Data)
{
	rtdrv_rg_asic_flowPath3_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP3Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP3Data, pP3Data, sizeof(rtk_rg_asic_path3_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	GETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH3_GET, &cfg, rtdrv_rg_asic_flowPath3_get_t, 1);
	osal_memcpy(pP3Data, &cfg.pP3Data, sizeof(rtk_rg_asic_path3_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath3_get */


/* Function Name:
 *      rtk_rg_asic_flowPath4_add
 * Description:
 *      Add flow table path4 entry
 * Input:
 *      *idx - index of flow table
 *      *pP4Data - point of path4 entry
 *      igrSVID
 *      igrCVID
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath4_add(uint32 *idx, rtk_rg_asic_path4_entry_t *pP4Data, uint16 igrSVID, uint16 igrCVID)
{
	rtdrv_rg_asic_flowPath4_add_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == idx), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pP4Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, idx, sizeof(uint32));
	osal_memcpy(&cfg.pP4Data, pP4Data, sizeof(rtk_rg_asic_path4_entry_t));
	osal_memcpy(&cfg.igrSVID, &igrSVID, sizeof(uint16));
	osal_memcpy(&cfg.igrCVID, &igrCVID, sizeof(uint16));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH4_ADD, &cfg, rtdrv_rg_asic_flowPath4_add_t, 1);
	osal_memcpy(idx, &cfg.idx, sizeof(uint32));
	osal_memcpy(pP4Data, &cfg.pP4Data, sizeof(rtk_rg_asic_path4_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath4_add */


/* Function Name:
 *      rtk_rg_asic_flowPath4DAHash_add
 * Description:
 *      Add flow table path4 entry
 * Input:
 *      *idx - index of flow table
 *      *pP3Data - point of path3 entry
 *      igrSVID
 *      igrCVID
 *      lutDaIdx
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath4DAHash_add(uint32 *idx, rtk_rg_asic_path4_entry_t * pP4Data, uint16 igrSVID, uint16 igrCVID, uint16 lutDaIdx)
{
	rtdrv_rg_asic_flowPath4DAHash_add_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == idx), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pP4Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, idx, sizeof(uint32));
	osal_memcpy(&cfg.pP4Data, pP4Data, sizeof(rtk_rg_asic_path4_entry_t));
	osal_memcpy(&cfg.igrSVID, &igrSVID, sizeof(uint16));
	osal_memcpy(&cfg.igrCVID, &igrCVID, sizeof(uint16));
	osal_memcpy(&cfg.lutDaIdx, &lutDaIdx, sizeof(uint16));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH4DAHASH_ADD, &cfg, rtdrv_rg_asic_flowPath4DAHash_add_t, 1);
	osal_memcpy(idx, &cfg.idx, sizeof(uint32));
	osal_memcpy(pP4Data, &cfg.pP4Data, sizeof(rtk_rg_asic_path4_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath4DAHash_add */


/* Function Name:
 *      rtk_rg_asic_flowPath4_set
 * Description:
 *      Set flow table path4 entry
 * Input:
 *      idx - index of flow table
 *      *pP4Data - point of path4 entry
 * Output:
 *	None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath4_set(uint32 idx, rtk_rg_asic_path4_entry_t *pP4Data)
{
	rtdrv_rg_asic_flowPath4_set_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP4Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP4Data, pP4Data, sizeof(rtk_rg_asic_path4_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH4_SET, &cfg, rtdrv_rg_asic_flowPath4_set_t, 1);
	osal_memcpy(pP4Data, &cfg.pP4Data, sizeof(rtk_rg_asic_path4_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath4_set */


/* Function Name:
 *      rtk_rg_asic_flowPath4_get
 * Description:
 *      Get flow table path4 entry
 * Input:
 *      idx - index of flow table
 * Output:
 *      *pP4Data - point of path4 entry
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath4_get(uint32 idx, rtk_rg_asic_path4_entry_t *pP4Data)
{
	rtdrv_rg_asic_flowPath4_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP4Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP4Data, pP4Data, sizeof(rtk_rg_asic_path4_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	GETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH4_GET, &cfg, rtdrv_rg_asic_flowPath4_get_t, 1);
	osal_memcpy(pP4Data, &cfg.pP4Data, sizeof(rtk_rg_asic_path4_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath4_get */


/* Function Name:
 *      rtk_rg_asic_flowPath5_add
 * Description:
 *      Add flow table path5 entry
 * Input:
 *      *idx - index of flow table
 *      *pP5Data - point of path5 entry
 *      igrSVID
 *      igrCVID
 * Return:
 *	None
 * Output:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath5_add(uint32 *idx, rtk_rg_asic_path5_entry_t *pP5Data, uint16 igrSVID, uint16 igrCVID)
{
	rtdrv_rg_asic_flowPath5_add_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == idx), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pP5Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, idx, sizeof(uint32));
	osal_memcpy(&cfg.pP5Data, pP5Data, sizeof(rtk_rg_asic_path5_entry_t));
	osal_memcpy(&cfg.igrSVID, &igrSVID, sizeof(uint16));
	osal_memcpy(&cfg.igrCVID, &igrCVID, sizeof(uint16));
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH5_ADD, &cfg, rtdrv_rg_asic_flowPath5_add_t, 1);
	osal_memcpy(idx, &cfg.idx, sizeof(uint32));
	osal_memcpy(pP5Data, &cfg.pP5Data, sizeof(rtk_rg_asic_path5_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath5_add */


/* Function Name:
 *      rtk_rg_asic_flowPath5_set
 * Description:
 *      Set flow table path5 entry
 * Input:
 *      idx - index of flow table
 *      *pP5Data - point of path5 entry
 * Return:
 *	None
 * Output:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath5_set(uint32 idx, rtk_rg_asic_path5_entry_t *pP5Data)
{
	rtdrv_rg_asic_flowPath5_set_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP5Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP5Data, pP5Data, sizeof(rtk_rg_asic_path5_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH5_SET, &cfg, rtdrv_rg_asic_flowPath5_set_t, 1);
	osal_memcpy(pP5Data, &cfg.pP5Data, sizeof(rtk_rg_asic_path5_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath5_set */


/* Function Name:
 *      rtk_rg_asic_flowPath5_get
 * Description:
 *      Get flow table path5 entry
 * Input:
 *      idx - index of flow table
 * Return:
 *	*pP5Data - point of path5 entry
 * Output:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath5_get(uint32 idx, rtk_rg_asic_path5_entry_t *pP5Data)
{
	rtdrv_rg_asic_flowPath5_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP5Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP5Data, pP5Data, sizeof(rtk_rg_asic_path5_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	GETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH5_GET, &cfg, rtdrv_rg_asic_flowPath5_get_t, 1);
	osal_memcpy(pP5Data, &cfg.pP5Data, sizeof(rtk_rg_asic_path5_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath5_get */


/* Function Name:
 *      rtk_rg_asic_flowPath6_add
 * Description:
 *      Add flow table path6 entry
 * Input:
 *      *idx - index of flow table
 *      *pP6Data - point of path6 entry
 *      igrSVID
 *      igrCVID
 * Return:
 *	None
 * Output:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath6_add(uint32 *idx, rtk_rg_asic_path6_entry_t *pP6Data, uint16 igrSVID, uint16 igrCVID)
{
	rtdrv_rg_asic_flowPath6_add_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == idx), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pP6Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, idx, sizeof(uint32));
	osal_memcpy(&cfg.pP6Data, pP6Data, sizeof(rtk_rg_asic_path6_entry_t));
	osal_memcpy(&cfg.igrSVID, &igrSVID, sizeof(uint16));
	osal_memcpy(&cfg.igrCVID, &igrCVID, sizeof(uint16));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH6_ADD, &cfg, rtdrv_rg_asic_flowPath6_add_t, 1);
	osal_memcpy(idx, &cfg.idx, sizeof(uint32));
	osal_memcpy(pP6Data, &cfg.pP6Data, sizeof(rtk_rg_asic_path6_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath6_add */


/* Function Name:
 *      rtk_rg_asic_flowPath6_set
 * Description:
 *      Set flow table path6 entry
 * Input:
 *      idx - index of flow table
 *      *pP6Data - point of path6 entry
 * Return:
 *	None
 * Output:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath6_set(uint32 idx, rtk_rg_asic_path6_entry_t *pP6Data)
{
	rtdrv_rg_asic_flowPath6_set_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP6Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP6Data, pP6Data, sizeof(rtk_rg_asic_path6_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	SETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH6_SET, &cfg, rtdrv_rg_asic_flowPath6_set_t, 1);
	osal_memcpy(pP6Data, &cfg.pP6Data, sizeof(rtk_rg_asic_path6_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath6_set */


/* Function Name:
 *      rtk_rg_asic_flowPath6_get
 * Description:
 *      Get flow table path6 entry
 * Input:
 *      idx - index of flow table
 * Return:
 *      *pP6Data - point of path6 entry
 * Output:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 *      RT_ERR_RG_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_flowPath6_get(uint32 idx, rtk_rg_asic_path6_entry_t *pP6Data)
{
	rtdrv_rg_asic_flowPath6_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pP6Data), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.idx, &idx, sizeof(uint32));
	osal_memcpy(&cfg.pP6Data, pP6Data, sizeof(rtk_rg_asic_path6_entry_t));
	//printf("%s\n", __FUNCTION__); //user space
	GETSOCKOPT(RTDRV_RG_ASIC_FLOWPATH6_GET, &cfg, rtdrv_rg_asic_flowPath6_get_t, 1);
	osal_memcpy(pP6Data, &cfg.pP6Data, sizeof(rtk_rg_asic_path6_entry_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_asic_flowPath6_get */

#endif	// end CONFIG_RG_RTL9607C_SERIES

/* Function Name:
 *      rtk_rg_asic_globalState_set
 * Description:
 *      Set asic global state
 * Input:
 *      stateType - one of the global control register 
 *      state - enabled or disabled
 * Return:
 *      None
 * Output:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_globalState_set(rtk_rg_asic_globalStateType_t stateType, rtk_enable_t state)
{
    rtdrv_rg_asic_globalState_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.stateType, &stateType, sizeof(rtk_rg_asic_globalStateType_t));
    osal_memcpy(&cfg.state, &state, sizeof(rtk_enable_t));
    SETSOCKOPT(RTDRV_RG_ASIC_GLOBALSTATE_SET, &cfg, rtdrv_rg_asic_globalState_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_asic_globalState_set */

/* Function Name:
 *      rtk_rg_asic_globalState_get
 * Description:
 *      Set asic global state
 * Input:
 *      stateType - one of the global control register 
 * Return:
 *      *pState - control register state
 * Output:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_FAILED
 *      RT_ERR_RG_NOT_INIT     - The module is not initial
 * Note:
 *      None
 */
rtk_rg_err_code_t
rtk_rg_asic_globalState_get(rtk_rg_asic_globalStateType_t stateType, rtk_enable_t *pState)
{
    rtdrv_rg_asic_globalState_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pState), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.stateType, &stateType, sizeof(rtk_rg_asic_globalStateType_t));
    osal_memcpy(&cfg.pState, pState, sizeof(rtk_enable_t));
    GETSOCKOPT(RTDRV_RG_ASIC_GLOBALSTATE_GET, &cfg, rtdrv_rg_asic_globalState_get_t, 1);
    osal_memcpy(pState, &cfg.pState, sizeof(rtk_enable_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_asic_globalState_get */

#endif	// end CONFIG_RG_FLOW_BASED_PLATFORM

/* Function Name:
 *      rtk_rg_driverVersion_get
 * Description:
 *      Get the RG rome driver version number.
 * Input:
 * Output:
 *      version_string - [in]<tab>The pointer of version structure.<nl>[out]<tab>The pointer to the stucture with version string.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the buffer parameter may be NULL
 * Note:
 *      version_string.version_string - the char string of version, at most 20 characters.
 */
int32 
rtk_rg_driverVersion_get(rtk_rg_VersionString_t *version_string)
{
	rtdrv_rg_driverVersion_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == version_string), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.version_string, version_string, sizeof(rtk_rg_VersionString_t));
	GETSOCKOPT(RTDRV_RG_DRIVERVERSION_GET, &cfg, rtdrv_rg_driverVersion_get_t, 1);
	osal_memcpy(version_string, &cfg.version_string, sizeof(rtk_rg_VersionString_t));

	return RT_ERR_OK;
}   /* end of rtk_rg_driverVersion_get */

/* Function Name:
 *      rtk_rg_initParam_get
 * Description:
 *      Get the initialized call-back functions.
 * Input:
 * Output:
 *      init_param - [in]<tab>The instance of rtk_rg_initParams_t.<nl>[out]<tab>The instance of rtk_rg_initParams_t with saved call-back function pointers and global settings.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the buffer parameter may be NULL 
 *      RT_ERR_RG_INITPM_UNINIT - init parameter structure are all NULL pointers
 * Note: 
 *      igmpSnoopingEnable - control IGMP snooping should on or off.<nl>
 *      macBasedTagDecision - control tag decision based on MAC should on or off.<nl>
 *      wanPortGponMode - indicate if the switch configure as GPON mode or not.<nl>
 *      init_param.arpAddByHwCallBack - this call-back function pointer should be called when ARP entry added.<nl>
 *      init_param.arpDelByHwCallBack - this call-back function pointer should be called when ARP entry deleted.<nl>
 *      init_param.macAddByHwCallBack - this call-back function pointer should be called when MAC entry added.<nl>
 *      init_param.macDelByHwCallBack - this call-back function pointer should be called when MAC entry deleted.<nl>
 *      init_param.routingAddByHwCallBack - this call-back function pointer should be called when Routing entry added.<nl>
 *      init_param.routingDelByHwCallBack - this call-back function pointer should be called when Routing entry deleted.<nl>
 *      init_param.naptAddByHwCallBack - this call-back function pointer should be called when NAPT entry added.<nl>
 *      init_param.naptDelByHwCallBack - this call-back function pointer should be called when NAPT entry deleted.
 *      init_param.bindingAddByHwCallBack - this call-back function pointer should be called when Binding entry added.<nl>
 *      init_param.bindingDelByHwCallBack - this call-back function pointer should be called when Binding entry added.
 */
int32 
rtk_rg_initParam_get(rtk_rg_initParams_t *init_param)
{
    rtdrv_rg_initParam_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == init_param), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.init_param, init_param, sizeof(rtk_rg_initParams_t));
    GETSOCKOPT(RTDRV_RG_INITPARAM_GET, &cfg, rtdrv_rg_initParam_get_t, 1);
    osal_memcpy(init_param, &cfg.init_param, sizeof(rtk_rg_initParams_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_initParam_get */

/* Function Name:
 *      rtk_rg_initParam_set
 * Description:
 *      Set the initialized call-back functions, and reset all Global variables.
 * Input:
 * Output:
 *      init_param  - [in]<tab>The instance of rtk_rg_initParams_t with saved call-back function pointers.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - The RG module is failed to initialize
 *      RT_ERR_RG_VLAN_SET_FAIL - Set up default CPU VLAN or LAN VLAN failed
 * Note:
 *      This function should be called before any other RG APIs.
 */
int32 
rtk_rg_initParam_set(rtk_rg_initParams_t *init_param)
{
	rtdrv_rg_initParam_set_t cfg;

	/* parameter check */
	//This API allow NULL parameter
	/* function body */
	if(init_param==NULL){
		osal_memset(&cfg.init_param,0xff,sizeof(rtk_rg_initParams_t));//translate parameter NULL to 0xffff....ffff
		GETSOCKOPT(RTDRV_RG_INITPARAM_SET, &cfg, rtdrv_rg_initParam_set_t, 1);
	}else{
		osal_memcpy(&cfg.init_param, init_param, sizeof(rtk_rg_initParams_t));
		GETSOCKOPT(RTDRV_RG_INITPARAM_SET, &cfg, rtdrv_rg_initParam_set_t, 1);
		osal_memcpy(init_param, &cfg.init_param, sizeof(rtk_rg_initParams_t));
	}
  
	return RT_ERR_OK;
}	/* end of rtk_rg_initParam_set */


//LAN Interface/Static Route/IPv4 DHCP Server

/* Sub-module Name: Lan Interface */

/* Function Name:
 *      rtk_rg_lanInterface_add
 * Description:
 *      Create one new LAN interface, add related entries into HW tables and Global variables.
 * Input:
 * Output:
 *      lan_info  - [in]<tab>LAN interface configuration structure.
 *      intf_idx - [out]<tab>Return the index of new created LAN interface.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameter may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_NOT_INIT - the RG module didn't init
 *      RT_ERR_RG_ENTRY_FULL - the interface number is beyond predefined limitation
 *      RT_ERR_RG_MODIFY_LAN_AT_WAN_EXIST - the LAN interface can not add after WAN interface exist
 *      RT_ERR_RG_PORT_USED - the interface has port overlap with other interface
 *      RT_ERR_RG_ARP_FULL - ARP table is not available for this new interface
 *      RT_ERR_RG_INTF_GET_FAIL
 *      RT_ERR_RG_VLAN_SET_FAIL
 *      RT_ERR_RG_INTF_SET_FAIL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 * Note:
 *      lan_info.gmac - the gateway MAC address of this LAN interface.<nl>
 *      lan_info.ip_addr - the IP address of this LAN interface.<nl>
 *      lan_info.ip_network_mask - the mask decides how many hosts in this LAN.<nl>
 *      lan_info.port_mask - which ports belong to this LAN.<nl>
 *      lan_info.untag_mask - which ports in this LAN should be egress untag.<nl>
 *      lan_info.extport_mask - which extension ports belong to this LAN.<nl>
 *      lan_info.intf_vlan_id - the default VLAN ID of this LAN.<nl>
 *      lan_info.vlan_based_pri - indicate what VLAN priority this WAN interface should carry.<nl> 
 *      lan_info.vlan_based_pri_enable - indicate VLAN-based priority enable or not.<nl> 
 *      lan_info.mtu - the maximum transmission unit of this LAN interface.<nl>
 *      lan_info.isIVL - how to learning layer2 record, IVL or SVL.<nl>
 *      lan_info.replace_subnet - replace exist LAN's IP or create new IP subnet.<nl> 
 *      This function should be called after rtk_rg_initParam_set and before any WAN interface creation.
 */
int32 
rtk_rg_lanInterface_add(rtk_rg_lanIntfConf_t *lan_info,int *intf_idx)
{
    rtdrv_rg_lanInterface_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == lan_info), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == intf_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.lan_info, lan_info, sizeof(rtk_rg_lanIntfConf_t));
    osal_memcpy(&cfg.intf_idx, intf_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_LANINTERFACE_ADD, &cfg, rtdrv_rg_lanInterface_add_t, 1);
    osal_memcpy(lan_info, &cfg.lan_info, sizeof(rtk_rg_lanIntfConf_t));
    osal_memcpy(intf_idx, &cfg.intf_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_lanInterface_add */

/* Function Name:
 *      rtk_rg_dhcpServerStaticAlloc_add
 * Description:
 *      Add DHCP static allocated IP address for indicated MAC address.
 * Input:
 * Output:
 *      ipaddr - [in]<tab>What IP address DHCP server should allocate with the macaddr.
 *      macaddr - [in]<tab>DHCP server use this to decide which IP address should allocate.
 *      static_idx - [out]<tab>Return the index of the IP-MAC pair.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_ENTRY_FULL - the static allocated is not enough for adding a new one
 * Note:
 *      macaddr.octet - divide MAC address into 6 octet array.
 */
//extern int32 rtk_rg_dhcpServerStaticAlloc_add(ipaddr_t ipaddr, rtk_mac_t *macaddr,int *static_idx);

/* Function Name:
 *      rtk_rg_dhcpServerStaticAlloc_del
 * Description:
 *      Delete the static IP address assignment.
 * Input:
 * Output:
 *      static_idx - [in]<tab>The index of static allocated address to be deleted.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information
 * Note:
 *      None
 */
//extern int32 rtk_rg_dhcpServerStaticAlloc_del(int static_idx);

/* Function Name:
 *      rtk_rg_dhcpServerStaticAlloc_find
 * Description:
 *      Get the IP address and MAC address according to the input index.
 * Input:
 * Output:
 *      ipaddr  - [out]<tab>DHCP server should give this address with macaddr.
 *	  macaddr - [out]<tab>DHCP server use this to decide which IP address should give.
 *      idx - [in]<tab>the index of static allocated address to find.<nl>[out]<tab>return the actually index of the record.(See Note below)
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_STATIC_NOT_FOUND - there is no valid record
 * Note:
 *      If the input idx record is not valid, it will auto incress to next index, until return a valid record or meet the end.
 */
//extern int32 rtk_rg_dhcpServerStaticAlloc_find(ipaddr_t *ipaddr, rtk_mac_t *macaddr, int *idx);

/* Sub-module Name: Wan Interface */

/* Function Name:
 *      rtk_rg_wanInterface_add
 * Description:
 *      Add WAN interface.
 * Input:
 * Output:
 *      wanintf - [in]<tab>The configuration structure of WAN interface.
 *      wan_intf_idx - [out]<tab>The created new WAN interface index.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_NOT_INIT - the RG module didn't init
 *      RT_ERR_RG_ENTRY_FULL - the table is full
 *      RT_ERR_RG_DEF_ROUTE_EXIST - the default internet WAN is exist
 *	  RT_ERR_RG_LAN_NOT_EXIST - WAN interface should be add after LAN interface created
 *      RT_ERR_RG_PON_INVALID - for chip A1, the PON port can not be set as WAN port
 * 	  RT_ERR_RG_CHIP_NOT_SUPPORT - the function is not support for the chip
 *      RT_ERR_RG_UNBIND_BDWAN_SHOULD_EQUAL_LAN_VLAN - unbind bridge WAN can't assign VLAN id different than LAN interface
 *      RT_ERR_RG_BIND_WITH_UNBIND_WAN - the global switch macBasedTagDecision didn't turn on
 *      RT_ERR_RG_PORT_BIND_GET_FAIL
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_INTF_GET_FAIL
 *      RT_ERR_RG_INTF_SET_FAIL
 *      RT_ERR_RG_VLAN_GET_FAIL
 *      RT_ERR_RG_VLAN_SET_FAIL
 *	  RT_ERR_RG_PPPOE_SET_FAIL
 *      RT_ERR_RG_NXP_SET_FAIL
 *      RT_ERR_RG_WANTYPE_SET_FAIL
 *      RT_ERR_RG_PORT_BIND_SET_FAIL
 * Note:
 *      wanintf.wan_type - this WAN interface type, can be Bridge mode, DHCP mode, PPPoE mode, etc.<nl>
 *      wanintf.gmac - the gateway MAC address of this WAN interface.<nl>
 *      wanintf.wan_port_idx - indicate which port this WAN interface belong to.<nl>
 *      wanintf.port_binding_mask - indicate which ports and extension ports should be bound to this WAN interface.<nl>
 *      wanintf.egress_vlan_tag_on - indicate the egress packet should carry VLAN CTAG or not.<nl>
 *      wanintf.egress_vlan_id - indicate what VLAN ID this WAN interface should carry.<nl>
 *      wanintf.vlan_based_pri_enable - indicate VLAN-based priority enable or not.<nl> 
 *      wanintf.vlan_based_pri - indicate what VLAN priority this WAN interface should carry.<nl>  
 *      wanintf.none_internet - indicate this WAN is internet WAN or not.<nl>
 *      wanintf.wlan0_dev_binding_mask - indicate which wlan0 devices should be bound to this WAN interface.<nl>
 *      When adding WAN interface except Bridge mode, this function should be called before<nl>
 *      each sub function like rtk_rg_staticInfo_set, rtk_rg_dhcpClientInfo_set, rtk_rg_pppoeClientInfoBeforeDial_set, and rtk_rg_pppoeClientInfoAfterDial_set.
 */
int32 
rtk_rg_wanInterface_add(rtk_rg_wanIntfConf_t *wanintf, int *wan_intf_idx)
{
    rtdrv_rg_wanInterface_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == wanintf), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == wan_intf_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wanintf, wanintf, sizeof(rtk_rg_wanIntfConf_t));
    osal_memcpy(&cfg.wan_intf_idx, wan_intf_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_WANINTERFACE_ADD, &cfg, rtdrv_rg_wanInterface_add_t, 1);
    osal_memcpy(wanintf, &cfg.wanintf, sizeof(rtk_rg_wanIntfConf_t));
    osal_memcpy(wan_intf_idx, &cfg.wan_intf_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_wanInterface_add */

/* Function Name:
 *      rtk_rg_staticInfo_set
 * Description:
 *      Set static mode information to the indexed WAN interface.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for static mode.
 *      static_info - [in]<tab>The configuration related to static mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *	  RT_ERR_RG_ENTRY_NOT_EXIST - the wan interface was not added before
 *	  RT_ERR_RG_ARP_FULL - ARP table is not available for this new interface
 *      RT_ERR_RG_ARP_NOT_FOUND - the remote gateway is not found or time-out
 *	  RT_ERR_RG_GW_MAC_NOT_SET - gateway mac should be set for Lite RomeDriver
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_EXTIP_GET_FAIL
 *      RT_ERR_RG_EXTIP_SET_FAIL
 * Note:
 *      static_info.napt_enable - indicate this WAN interface should turn on napt or not.<nl>
 *      static_info.ip_addr - the IP address of this WAN interface.<nl>
 *      static_info.ip_network_mask - the mask decides how many hosts in this WAN.<nl>
 *      static_info.default_gateway_on - indicate this WAN interface is default internet interface or not.<nl>
 *      static_info.gateway_ip_addr - indicate which gateway this WAN interface is heading to.<nl>
 *      static_info.mtu - the maximum transmission unit of this LAN interface.<nl>
 *      static_info.gw_mac_auto_learn - switch to turn on auto-ARP request for gateway MAC.<nl>
 *      static_info.gateway_mac_addr - pointer to gateway's mac if the auto_learn switch is off.<nl>
 *      The interface index should point to a valid WAN interface with matched type.
 */
int32 
rtk_rg_staticInfo_set(int wan_intf_idx, rtk_rg_ipStaticInfo_t *static_info)
{
    rtdrv_rg_staticInfo_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == static_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.static_info, static_info, sizeof(rtk_rg_ipStaticInfo_t));
    GETSOCKOPT(RTDRV_RG_STATICINFO_SET, &cfg, rtdrv_rg_staticInfo_set_t, 1);
    osal_memcpy(static_info, &cfg.static_info, sizeof(rtk_rg_ipStaticInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_staticInfo_set */

/* Function Name:
 *      rtk_rg_dsliteInfo_set
 * Description:
 *      Set dslite mode information to the indexed WAN interface.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for static mode.
 *      dslite_info - [in]<tab>The configuration related to dslite mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *	  RT_ERR_RG_ENTRY_NOT_EXIST - the wan interface was not added before
 *	  RT_ERR_RG_ARP_FULL - ARP table is not available for this new interface
 *      RT_ERR_RG_ARP_NOT_FOUND - the remote gateway is not found or time-out
 *	  RT_ERR_RG_GW_MAC_NOT_SET - gateway mac should be set for Lite RomeDriver
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_EXTIP_GET_FAIL
 *      RT_ERR_RG_EXTIP_SET_FAIL
 * Note:
 *      dslite_info.napt_enable - indicate this WAN interface should turn on napt or not.<nl>
 *      dslite_info.ip_addr - the IP address of this WAN interface.<nl>
 *      dslite_info.ip_network_mask - the mask decides how many hosts in this WAN.<nl>
 *      dslite_info.default_gateway_on - indicate this WAN interface is default internet interface or not.<nl>
 *      dslite_info.gateway_ip_addr - indicate which gateway this WAN interface is heading to.<nl>
 *      dslite_info.mtu - the maximum transmission unit of this LAN interface.<nl>
 *      dslite_info.gw_mac_auto_learn - switch to turn on auto-ARP request for gateway MAC.<nl>
 *      dslite_info.gateway_mac_addr - pointer to gateway's mac if the auto_learn switch is off.<nl>
 *      The interface index should point to a valid WAN interface with matched type.
 */
int32
rtk_rg_dsliteInfo_set(int wan_intf_idx, rtk_rg_ipDslitStaticInfo_t *dslite_info)
{
    rtdrv_rg_dsliteInfo_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == dslite_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.dslite_info, dslite_info, sizeof(rtk_rg_ipDslitStaticInfo_t));
    GETSOCKOPT(RTDRV_RG_DSLITEINFO_SET, &cfg, rtdrv_rg_dsliteInfo_set_t, 1);
    osal_memcpy(dslite_info, &cfg.dslite_info, sizeof(rtk_rg_ipDslitStaticInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_dsliteInfo_set */


/* Function Name:
 *      rtk_rg_dhcpRequest_set
 * Description:
 *      Set DHCP request for the indexed WAN interface.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for DHCP mode.
 * Return:
 *      RT_ERR_RG_OK
 * Note:
 *      None
 */
int32
rtk_rg_dhcpRequest_set(int wan_intf_idx)
{
    rtdrv_rg_dhcpRequest_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_DHCPREQUEST_SET, &cfg, rtdrv_rg_dhcpRequest_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_dhcpRequest_set */

/* Function Name:
 *      rtk_rg_dhcpClientInfo_set
 * Description:
 *      Set DHCP mode information to the indexed WAN interface.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for DHCP mode.
 *      dhcpClient_info - [in]<tab>The configuration related to the interface.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *	  RT_ERR_RG_ENTRY_NOT_EXIST - the wan interface was not added before
 *	  RT_ERR_RG_ARP_FULL - ARP table is not available for this new interface
 *      RT_ERR_RG_ARP_NOT_FOUND - the remote gateway is not found or time-out
 *	  RT_ERR_RG_GW_MAC_NOT_SET - gateway mac should be set for Lite RomeDriver
 *      RT_ERR_RG_INTF_GET_FAIL
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_EXTIP_GET_FAIL
 *      RT_ERR_RG_EXTIP_SET_FAIL
 * Note:
 *      dhcpClient_info.hw_info - the static_info sturcture of this DHCP mode WAN interface.<nl>
 *      dhcpClient_info.status - the status of DHCP mode WAN interface as client, leased or released.<nl>
 *      The client_info contains the Hardware information which contains static mode settings, and the interface index should point to a WAN interface with matched type.
 */
int32 
rtk_rg_dhcpClientInfo_set(int wan_intf_idx, rtk_rg_ipDhcpClientInfo_t *dhcpClient_info)
{
    rtdrv_rg_dhcpClientInfo_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == dhcpClient_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.dhcpClient_info, dhcpClient_info, sizeof(rtk_rg_ipDhcpClientInfo_t));
    GETSOCKOPT(RTDRV_RG_DHCPCLIENTINFO_SET, &cfg, rtdrv_rg_dhcpClientInfo_set_t, 1);
    osal_memcpy(dhcpClient_info, &cfg.dhcpClient_info, sizeof(rtk_rg_ipDhcpClientInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_dhcpClientInfo_set */

/* Function Name:
 *      rtk_rg_pppoeClientInfoBeforeDial_set
 * Description:
 *      Set PPPoE mode information to the indexed WAN interface before dial. 
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for PPPoE mode.
 *      app_info - [in]<tab>The configuration related to PPPoE mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      app_info.username - saved user account information for PPPoE server.<nl>
 *      app_info.password - saved account password for PPPoE server.<nl>
 *      app_info.auth_type - indicate the PPPoE server use PAP or CHAP.<nl>
 *      app_info.pppoe_proxy_enable - indicate the proxy of PPPoE server enable or not.<nl>
 *      app_info.max_pppoe_proxy_num - how many PPPoE proxy at most.<nl>
 *      app_info.auto_reconnect - indicate that we reconnect automatically no matter what.<nl>
 *      app_info.dial_on_demond - indicate that we connect only on demand.<nl>
 *      app_info.idle_timeout_secs - indicate how long we should turn off connection after idle.<nl>
 *      app_info.stauts - indicate the status of connection.<nl>
 *      app_info.dialOnDemondCallBack - this function would be called when the interface has traffic flow.<nl>
 *      app_info.idleTimeOutCallBack - this function would be called when the interface didn't have traffic for indicated timeout period.<nl>
 *      The required account information is kept and this function should be called before rtk_rg_pppoeClientInfoAfterDial_set.<nl>
 *      The interface index should point to a WAN interface with matched type.
 */
int32 
rtk_rg_pppoeClientInfoBeforeDial_set(int wan_intf_idx, rtk_rg_pppoeClientInfoBeforeDial_t *app_info)
{
    rtdrv_rg_pppoeClientInfoBeforeDial_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == app_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.app_info, app_info, sizeof(rtk_rg_pppoeClientInfoBeforeDial_t));
    GETSOCKOPT(RTDRV_RG_PPPOECLIENTINFOBEFOREDIAL_SET, &cfg, rtdrv_rg_pppoeClientInfoBeforeDial_set_t, 1);
    osal_memcpy(app_info, &cfg.app_info, sizeof(rtk_rg_pppoeClientInfoBeforeDial_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_pppoeClientInfoBeforeDial_set */

/* Function Name:
 *      rtk_rg_pppoeClientInfoAfterDial_set
 * Description:
 *      Set PPPoE mode information to the indexed WAN interface after dial. 
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for PPPoE mode.
 *      clientPppoe_info - [in]<tab>The configuration related to PPPoE mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *	  RT_ERR_RG_ENTRY_NOT_EXIST - the wan interface was not added before
 *	  RT_ERR_RG_ARP_FULL - ARP table is not available for this new interface
 *      RT_ERR_RG_ARP_NOT_FOUND - the remote gateway is not found or time-out
 *	  RT_ERR_RG_GW_MAC_NOT_SET - gateway mac should be set for Lite RomeDriver
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_EXTIP_GET_FAIL
 *      RT_ERR_RG_EXTIP_SET_FAIL
 * Note:
 *      clientPppoe_info.hw_info - the static_info sturcture of this PPPoE mode WAN interface.<nl>
 *      clientPppoe_info.sessionId - stored PPPoE session ID currently used.<nl>
 *      The client_info contains the Hardware information which contains static mode settings, and the interface index should point to a WAN interface with matched type.
 */
int32 
rtk_rg_pppoeClientInfoAfterDial_set(int wan_intf_idx, rtk_rg_pppoeClientInfoAfterDial_t *clientPppoe_info)
{
    rtdrv_rg_pppoeClientInfoAfterDial_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == clientPppoe_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.clientPppoe_info, clientPppoe_info, sizeof(rtk_rg_pppoeClientInfoAfterDial_t));
    GETSOCKOPT(RTDRV_RG_PPPOECLIENTINFOAFTERDIAL_SET, &cfg, rtdrv_rg_pppoeClientInfoAfterDial_set_t, 1);
    osal_memcpy(clientPppoe_info, &cfg.clientPppoe_info, sizeof(rtk_rg_pppoeClientInfoAfterDial_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_pppoeClientInfoAfterDial_set */

/* Function Name:
 *      rtk_rg_pptpClientInfoBeforeDial_set
 * Description:
 *      Set PPTP mode information to the indexed WAN interface before dial.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for PPTP mode.
 *      app_info - [in]<tab>The configuration related to PPTP mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 */
int32 
rtk_rg_pptpClientInfoBeforeDial_set(int wan_intf_idx, rtk_rg_pptpClientInfoBeforeDial_t *app_info)
{
    rtdrv_rg_pptpClientInfoBeforeDial_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == app_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.app_info, app_info, sizeof(rtk_rg_pptpClientInfoBeforeDial_t));
    GETSOCKOPT(RTDRV_RG_PPTPCLIENTINFOBEFOREDIAL_SET, &cfg, rtdrv_rg_pptpClientInfoBeforeDial_set_t, 1);
    osal_memcpy(app_info, &cfg.app_info, sizeof(rtk_rg_pptpClientInfoBeforeDial_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_pptpClientInfoBeforeDial_set */

/* Function Name:
 *      rtk_rg_pptpClientInfoAfterDial_set
 * Description:
 *      Set PPTP mode information to the indexed WAN interface after dial.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for PPTP mode.
 *      clientPptp_info - [in]<tab>The configuration related to PPTP mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *        RT_ERR_RG_ENTRY_NOT_EXIST - the wan interface was not added before
 *        RT_ERR_RG_ARP_FULL - ARP table is not available for this new interface
 *      RT_ERR_RG_ARP_NOT_FOUND - the remote gateway is not found or time-out
 *        RT_ERR_RG_GW_MAC_NOT_SET - gateway mac should be set for Lite RomeDriver
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_EXTIP_GET_FAIL
 *      RT_ERR_RG_EXTIP_SET_FAIL
 * Note:
 */
int32 
rtk_rg_pptpClientInfoAfterDial_set(int wan_intf_idx, rtk_rg_pptpClientInfoAfterDial_t *clientPptp_info)
{
    rtdrv_rg_pptpClientInfoAfterDial_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == clientPptp_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.clientPptp_info, clientPptp_info, sizeof(rtk_rg_pptpClientInfoAfterDial_t));
    GETSOCKOPT(RTDRV_RG_PPTPCLIENTINFOAFTERDIAL_SET, &cfg, rtdrv_rg_pptpClientInfoAfterDial_set_t, 1);
    osal_memcpy(clientPptp_info, &cfg.clientPptp_info, sizeof(rtk_rg_pptpClientInfoAfterDial_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_pptpClientInfoAfterDial_set */

/* Function Name:
 *      rtk_rg_l2tpClientInfoBeforeDial_set
 * Description:
 *      Set L2TP mode information to the indexed WAN interface before dial.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for L2TP mode.
 *      app_info - [in]<tab>The configuration related to L2TP mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 */
int32 
rtk_rg_l2tpClientInfoBeforeDial_set(int wan_intf_idx, rtk_rg_l2tpClientInfoBeforeDial_t *app_info)
{
    rtdrv_rg_l2tpClientInfoBeforeDial_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == app_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.app_info, app_info, sizeof(rtk_rg_l2tpClientInfoBeforeDial_t));
    GETSOCKOPT(RTDRV_RG_L2TPCLIENTINFOBEFOREDIAL_SET, &cfg, rtdrv_rg_l2tpClientInfoBeforeDial_set_t, 1);
    osal_memcpy(app_info, &cfg.app_info, sizeof(rtk_rg_l2tpClientInfoBeforeDial_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_l2tpClientInfoBeforeDial_set */

/* Function Name:
 *      rtk_rg_l2tpClientInfoAfterDial_set
 * Description:
 *      Set L2TP mode information to the indexed WAN interface after dial.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for L2TP mode.
 *      clientPptp_info - [in]<tab>The configuration related to L2TP mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *        RT_ERR_RG_ENTRY_NOT_EXIST - the wan interface was not added before
 *        RT_ERR_RG_ARP_FULL - ARP table is not available for this new interface
 *      RT_ERR_RG_ARP_NOT_FOUND - the remote gateway is not found or time-out
 *        RT_ERR_RG_GW_MAC_NOT_SET - gateway mac should be set for Lite RomeDriver
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_EXTIP_GET_FAIL
 *      RT_ERR_RG_EXTIP_SET_FAIL
 * Note:
 */
int32 
rtk_rg_l2tpClientInfoAfterDial_set(int wan_intf_idx, rtk_rg_l2tpClientInfoAfterDial_t *clientL2tp_info)
{
    rtdrv_rg_l2tpClientInfoAfterDial_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == clientL2tp_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.clientL2tp_info, clientL2tp_info, sizeof(rtk_rg_l2tpClientInfoAfterDial_t));
    GETSOCKOPT(RTDRV_RG_L2TPCLIENTINFOAFTERDIAL_SET, &cfg, rtdrv_rg_l2tpClientInfoAfterDial_set_t, 1);
    osal_memcpy(clientL2tp_info, &cfg.clientL2tp_info, sizeof(rtk_rg_l2tpClientInfoAfterDial_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_l2tpClientInfoAfterDial_set */

/* Function Name:
 *      rtk_rg_pppoeDsliteInfoBeforeDial_set
 * Description:
 *      Set Dslite with PPPoE mode information to the indexed WAN interface before dial.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for Dslite with PPPoE mode.
 *      app_info - [in]<tab>The configuration related to Dslite with PPPoE mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 */
int32 
rtk_rg_pppoeDsliteInfoBeforeDial_set(int wan_intf_idx, rtk_rg_pppoeClientInfoBeforeDial_t *app_info)
{
    rtdrv_rg_pppoeDsliteInfoBeforeDial_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == app_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.app_info, app_info, sizeof(rtk_rg_pppoeClientInfoBeforeDial_t));
    GETSOCKOPT(RTDRV_RG_PPPOEDSLITEINFOBEFOREDIAL_SET, &cfg, rtdrv_rg_pppoeDsliteInfoBeforeDial_set_t, 1);
    osal_memcpy(app_info, &cfg.app_info, sizeof(rtk_rg_pppoeClientInfoBeforeDial_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_pppoeDsliteInfoBeforeDial_set */

/* Function Name:
 *      rtk_rg_pppoeDsliteInfoAfterDial_set
 * Description:
 *      Set Dslite with PPPoE mode information to the indexed WAN interface after dial.
 * Input:
 * Output:
 *      wan_intf_idx - [in]<tab>The interface index of previous setup for Dslite with PPPoE mode.
 *      pppoeDslite_info - [in]<tab>The configuration related to Dslite with PPPoE mode.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *	  RT_ERR_RG_ENTRY_NOT_EXIST - the wan interface was not added before
 *	  RT_ERR_RG_ARP_FULL - ARP table is not available for this new interface
 *      RT_ERR_RG_ARP_NOT_FOUND - the remote gateway is not found or time-out
 *	  RT_ERR_RG_GW_MAC_NOT_SET - gateway mac should be set for Lite RomeDriver
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_EXTIP_GET_FAIL
 *      RT_ERR_RG_EXTIP_SET_FAIL
 * Note:
 */
int32 
rtk_rg_pppoeDsliteInfoAfterDial_set(int wan_intf_idx, rtk_rg_pppoeDsliteInfoAfterDial_t *pppoeDslite_info)
{
    rtdrv_rg_pppoeDsliteInfoAfterDial_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pppoeDslite_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.pppoeDslite_info, pppoeDslite_info, sizeof(rtk_rg_pppoeDsliteInfoAfterDial_t));
    GETSOCKOPT(RTDRV_RG_PPPOEDSLITEINFOAFTERDIAL_SET, &cfg, rtdrv_rg_pppoeDsliteInfoAfterDial_set_t, 1);
    osal_memcpy(pppoeDslite_info, &cfg.pppoeDslite_info, sizeof(rtk_rg_pppoeDsliteInfoAfterDial_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_pppoeDsliteInfoAfterDial_set */


/* Sub-module Name: Interface Utility */

/* Function Name:
 *      rtk_rg_interface_del
 * Description:
 *      Delete the indicated interface, may be LAN or WAN interface.
 * Input:
 * Output:
 *      lan_or_wan_intf_idx - [in]<tab>The index of interface.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *	  RT_ERR_RG_MODIFY_LAN_AT_WAN_EXIST - LAN interface should not be deleted when WAN interface existed
 *      RT_ERR_RG_INTF_GET_FAIL
 *      RT_ERR_RG_ROUTE_GET_FAIL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_NXP_GET_FAIL
 *      RT_ERR_RG_PPPOE_SET_FAIL
 *      RT_ERR_RG_NXP_SET_FAIL
 *      RT_ERR_RG_ROUTE_SET_FAIL
 *      RT_ERR_RG_EXTIP_GET_FAIL
 *      RT_ERR_RG_EXTIP_SET_FAIL
 *      RT_ERR_RG_WANTYPE_GET_FAIL
 *      RT_ERR_RG_WANTYPE_SET_FAIL
 *      RT_ERR_RG_INTF_SET_FAIL
 *      RT_ERR_RG_PORT_BIND_GET_FAIL
 *      RT_ERR_RG_PORT_BIND_SET_FAIL
 * Note:
 *      Before deleting any interface, it should be created first.
 */
int32 
rtk_rg_interface_del(int lan_or_wan_intf_idx)
{
    rtdrv_rg_interface_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.lan_or_wan_intf_idx, &lan_or_wan_intf_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_INTERFACE_DEL, &cfg, rtdrv_rg_interface_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_interface_del */

/* Function Name:
 *      rtk_rg_intfInfo_find
 * Description:
 *      Return the information structure of interface, eithor LAN or WAN.
 * Input:
 * Output:
 *      intf_info - [in]<tab>An empty buffer for storing the structure.<nl>[out]<tab>Returned valid interface information structure, eithor LAN or WAN.
 *      valid_lan_or_wan_intf_idx - [in]<tab>The index of interface to find.<nl>[out]<tab>Return the index of the valid record.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_ENTRY_NOT_EXIST
 * Note:
 *      intf_info.intf_name - the name of found interface.<nl>
 *      intf_info.is_wan - indicate this interface is WAN interface or LAN interface.<nl>
 *      intf_info.lan_intf - if is_wan is 0, this structure contain all information about the LAN interface.<nl>
 *      intf_info.wan_intf - if is_wan is 1, this structure contain all information about the WAN interface.<nl>
 *      If the input idx interface is not exist, it will auto incress to next index, until return a valid one or meet end.<nl>
 *      Besides, if the input idx is -1, intf_info.lan_intf.ip_addr or intf_info.lan_intf.ipv6_addr is given, <nl>
 *      the matching interface information and it's index will be return. Bridge WAN interface could not be found <nl>
 *      through this mode.
 */
int32 
rtk_rg_intfInfo_find(rtk_rg_intfInfo_t *intf_info, int *valid_lan_or_wan_intf_idx)
{
    rtdrv_rg_intfInfo_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == intf_info), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_lan_or_wan_intf_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.intf_info, intf_info, sizeof(rtk_rg_intfInfo_t));
    osal_memcpy(&cfg.valid_lan_or_wan_intf_idx, valid_lan_or_wan_intf_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_INTFINFO_FIND, &cfg, rtdrv_rg_intfInfo_find_t, 1);
    osal_memcpy(intf_info, &cfg.intf_info, sizeof(rtk_rg_intfInfo_t));
    osal_memcpy(valid_lan_or_wan_intf_idx, &cfg.valid_lan_or_wan_intf_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_intfInfo_find */

/* Function Name:
 *      rtk_rg_svlanTpid_set
 * Description:
 *      Set SVLAN TPID to system.
 * Input:
 * Output:
 *      svlan_tag_id - [in]<tab>The assigned TPID.
 * Return:
 *      RT_ERR_RG_OK
 * Note:
 *      None
 */
 int32
rtk_rg_svlanTpid_set(uint32 svlan_tag_id)
{
    rtdrv_rg_svlanTpid_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.svlan_tag_id, &svlan_tag_id, sizeof(uint32));
    SETSOCKOPT(RTDRV_RG_SVLANTPID_SET, &cfg, rtdrv_rg_svlanTpid_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_svlanTpid_set */

/* Function Name:
 *      rtk_rg_svlanTpid_get
 * Description:
 *      Get the SVLAN TPID setting in system.
 * Input:
 * Output:
 *      pSvlanTagId - [in]<tab>The system TPID.
 * Return:
 *      RT_ERR_RG_OK
 * Note:
 *      None
 */
int32
rtk_rg_svlanTpid_get(uint32 *pSvlanTagId)
{
    rtdrv_rg_svlanTpid_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pSvlanTagId), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pSvlanTagId, pSvlanTagId, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_SVLANTPID_GET, &cfg, rtdrv_rg_svlanTpid_get_t, 1);
    osal_memcpy(pSvlanTagId, &cfg.pSvlanTagId, sizeof(uint32));

    return RT_ERR_OK;
}   /* end of rtk_rg_svlanTpid_get */

/* Function Name:
 *      rtk_rg_svlanServicePort_set
 * Description:
 *      Set each port SVLAN enabled/disabled.
 * Input:
 * Output:
 *      port - [in]<tab>The assigned port.
 *        enable - [in]<tab>Enable/disable the assigned port SVLAN.
 * Return:
 *      RT_ERR_RG_OK
 * Note:
 *      None
 */
int32
rtk_rg_svlanServicePort_set(rtk_port_t port, rtk_enable_t enable)
{
    rtdrv_rg_svlanServicePort_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_port_t));
    osal_memcpy(&cfg.enable, &enable, sizeof(rtk_enable_t));
    SETSOCKOPT(RTDRV_RG_SVLANSERVICEPORT_SET, &cfg, rtdrv_rg_svlanServicePort_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_svlanServicePort_set */

/* Function Name:
 *      rtk_rg_svlanServicePort_get
 * Description:
 *      Get each port enabled SVLAN or not.
 * Input:
 * Output:
 *      port - [in]<tab>The assigned port.
 *        pEnable - [out]<tab>The assigned port enable/disable SVLAN.
 * Return:
 *      RT_ERR_RG_OK
 * Note:
 *      None
 */
int32
rtk_rg_svlanServicePort_get(rtk_port_t port, rtk_enable_t *pEnable)
{
    rtdrv_rg_svlanServicePort_get_t cfg;
	
    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_port_t));
    osal_memcpy(&cfg.pEnable, pEnable, sizeof(rtk_enable_t));
    GETSOCKOPT(RTDRV_RG_SVLANSERVICEPORT_GET, &cfg, rtdrv_rg_svlanServicePort_get_t, 1);
    osal_memcpy(pEnable, &cfg.pEnable, sizeof(rtk_enable_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_svlanServicePort_get */





/* Sub-module Name: Customer VLAN Functions */
/* Function Name:
 *      rtk_rg_cvlan_add
 * Description:
 *      Add customer VLAN setting.
 * Input:
 * Output:
 *      cvlan_info - [in]<tab>The VLAN configuration.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_NOT_INIT
 *      RT_ERR_RG_VLAN_USED_BY_INTERFACE - the prefered VLAN ID had been used as interface VLAN ID
 *      RT_ERR_RG_VLAN_USED_BY_VLANBINDING - the prefered VLAN ID had been used as VLAN-binding ID
 *      RT_ERR_RG_CVLAN_CREATED - the VLAN ID had been created as customer VLAN before
 *      RT_ERR_RG_CVLAN_RESERVED - the VLAN ID is reserved for system use
 *      RT_ERR_RG_VLAN_SET_FAIL
 * Note:
 *      cvlan_info.vlanId - which VLAN identifier need to create, between 0 and 4095.<nl>
 *      cvlan_info.isIVL - how to learning layer2 record, by SVL or by IVL setting.<nl>
 *      cvlan_info.memberPortMask - which port contained in this VLAN identifier.<nl>
 *      cvlan_info.untagPortMask - which port contained in this VLAN identifier should be untag.<nl>
 */
int32
rtk_rg_cvlan_add(rtk_rg_cvlan_info_t *cvlan_info)
{
    rtdrv_rg_cvlan_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == cvlan_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.cvlan_info, cvlan_info, sizeof(rtk_rg_cvlan_info_t));
    GETSOCKOPT(RTDRV_RG_CVLAN_ADD, &cfg, rtdrv_rg_cvlan_add_t, 1);
    osal_memcpy(cvlan_info, &cfg.cvlan_info, sizeof(rtk_rg_cvlan_info_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_cvlan_add */

/* Function Name:
 *      rtk_rg_cvlan_del
 * Description:
 *      Delete customer VLAN setting.
 * Input:
 * Output:
 *      cvlan_id - [in]<tab>The VLAN identifier needed to be deleted.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_VLAN_NOT_CREATED_BY_CVLAN - the deleting VLAN ID was not added as customer VLAN ID
 * Note:
 *      Here can not delete VLAN identifier used for interface or VLAN binding. Only the VLAN identifier created by rtk_rg_1qVlan_add can be deleted this way.
 */
int32
rtk_rg_cvlan_del(int cvlan_id)
{
    rtdrv_rg_cvlan_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.cvlan_id, &cvlan_id, sizeof(int));
    SETSOCKOPT(RTDRV_RG_CVLAN_DEL, &cfg, rtdrv_rg_cvlan_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_cvlan_del */

/* Function Name:
 *      rtk_rg_cvlan_get
 * Description:
 *      Get customer VLAN setting.
 * Input:
 * Output:
 *      cvlan_info - [in]<tab>The VLAN configuration.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_NOT_INIT
 * Note:
 *      cvlan_info.vlanId - which VLAN identifier need to retrive, between 0 and 4095.<nl>
 *      cvlan_info.isIVL - how to learning layer2 record, by SVL or by IVL setting.<nl>
 *      cvlan_info.memberPortMask - which port contained in this VLAN identifier.<nl>
 *      cvlan_info.untagPortMask - which port contained in this VLAN identifier should be untag.<nl>
 *      cvlan_info.vlan_based_pri_enable - indicate VLAN-based priority enable or not.<nl> 
 *      cvlan_info.vlan_based_pri - indicate what VLAN priority this VLAN should carry.<nl>
 */
int32
rtk_rg_cvlan_get(rtk_rg_cvlan_info_t *cvlan_info)
{
    rtdrv_rg_cvlan_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == cvlan_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.cvlan_info, cvlan_info, sizeof(rtk_rg_cvlan_info_t));
    GETSOCKOPT(RTDRV_RG_CVLAN_GET, &cfg, rtdrv_rg_cvlan_get_t, 1);
    osal_memcpy(cvlan_info, &cfg.cvlan_info, sizeof(rtk_rg_cvlan_info_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_cvlan_get */


/* Sub-module Name: VLAN Binding */
/* Function Name:
 *      rtk_rg_vlanBinding_add
 * Description:
 *      Add Port-VLAN binding rule.
 * Input:
 * Output:
 *      vlan_binding_info - [in]<tab>The VLAN binding configuration about port index, interface, and VLAN ID
 *      vlan_binding_idx - [out]<tab>The index of added VLAN binding rule.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_NOT_INIT - the RG module is not init
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_VLAN_USED_BY_INTERFACE	- the VLAN id can't used by interface
 *      RT_ERR_RG_VLAN_USED_BY_1QVLAN - the VLAN id can't used by 1QVLAN api
 *      RT_ERR_RG_BIND_WITH_UNBIND_WAN - the global switch macBasedTagDecision didn't turn on
 * Note:
 *      vlan_binding_idx.port_idx - each VLAN binding rule can only assign one single port or extension port (not CPU port).<nl>
 *      vlan_binding_idx.ingress_vid - the VLAN ID used to compare for binding.<nl>
 *      vlan_binding_idx.wan_intf_idx - which WAN interface the matched packet should go.<nl>
 *      The VLAN-binding rule can add only after the binding WAN interface is created at first.
 */
int32 
rtk_rg_vlanBinding_add(rtk_rg_vlanBinding_t *vlan_binding_info, int *vlan_binding_idx)
{
    rtdrv_rg_vlanBinding_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == vlan_binding_info), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == vlan_binding_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.vlan_binding_info, vlan_binding_info, sizeof(rtk_rg_vlanBinding_t));
    osal_memcpy(&cfg.vlan_binding_idx, vlan_binding_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_VLANBINDING_ADD, &cfg, rtdrv_rg_vlanBinding_add_t, 1);
    osal_memcpy(vlan_binding_info, &cfg.vlan_binding_info, sizeof(rtk_rg_vlanBinding_t));
    osal_memcpy(vlan_binding_idx, &cfg.vlan_binding_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_vlanBinding_add */

/* Function Name:
 *      rtk_rg_vlanBinding_del
 * Description:
 *      Delete Port-VLAN binding rule.
 * Input:
 * Output:
 *      vlan_binding_idx - [in]<tab>The index of VLAN binding rule.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_VLAN_BIND_UNINIT - there is no vlan-binding rule added before
 * Note:
 *      None
 */
int32 
rtk_rg_vlanBinding_del(int vlan_binding_idx)
{
    rtdrv_rg_vlanBinding_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.vlan_binding_idx, &vlan_binding_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_VLANBINDING_DEL, &cfg, rtdrv_rg_vlanBinding_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_vlanBinding_del */

/* Function Name:
 *      rtk_rg_vlanBinding_find
 * Description:
 *      Find Port-VLAN binding rule if any.
 * Input:
 * Output:
 *      vlan_binding_info - [in]<tab>The binding configuration of port and VLAN ID.
 *      valid_idx - [in]<tab>The index of the VLAN binding entry.<nl>[out]<tab>The index of first valid VLAN binding entry.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_VLAN_BIND_UNINIT - there is no vlan-binding rule added before
 * Note:
 *      If the input parameter do not point to a valid record, it will continue to look for next valid one until end, and return the valid index by the passed input pointer.
 */
int32 
rtk_rg_vlanBinding_find(rtk_rg_vlanBinding_t *vlan_binding_info, int *valid_idx)
{
    rtdrv_rg_vlanBinding_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == vlan_binding_info), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.vlan_binding_info, vlan_binding_info, sizeof(rtk_rg_vlanBinding_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_VLANBINDING_FIND, &cfg, rtdrv_rg_vlanBinding_find_t, 1);
    osal_memcpy(vlan_binding_info, &cfg.vlan_binding_info, sizeof(rtk_rg_vlanBinding_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_vlanBinding_find */


/* Sub-module Name: ALG configuration */

int rtk_rg_algApps_set(rtk_rg_alg_type_t alg_app);
int rtk_rg_algApps_get(rtk_rg_alg_type_t* alg_app);


/* Sub-module Name: DMZ configuration */

int rtk_rg_dmzHost_set(int wan_intf_idx, rtk_rg_dmzInfo_t *dmz_info);
int rtk_rg_dmzHost_get(int wan_intf_idx, rtk_rg_dmzInfo_t *dmz_info);


/* Sub-module Name: VirtualServer configuration */

/* Function Name:
 *      rtk_rg_virtualServer_add
 * Description:
 *      Add virtual server connection rule.
 * Input:
 * Output:
 *      *virtual_server - [in]<tab>the svirtual server data structure.
 *      *virtual_server_idx - [in]<tab>the index of virtual server table.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INITPM_UNINIT - the virtual server table uninitialized.
 *		RT_ERR_RG_ENTRY_FULL - the virtual server table is full.
 * Note:
 *     	virtual_server.is_tcp - Layer 4 protocol is TCP or not.<nl>	
 *		virtual_server.wan_intf_idx - Wan interface index of server.<nl>	
 *		virtual_server.gateway_port_start - Gateway external port mapping start number.<nl>	
 *		virtual_server.local_ip - The translating local IP address.<nl>	
 *		virtual_server.local_port_start - The translating internal port mapping start number.<nl>	
 *		virtual_server.mappingRangeCnt - The port mapping range count.<nl>
 *		virtual_server.valid - This entry is valid or not.<nl>
 */
int32 
rtk_rg_virtualServer_add(rtk_rg_virtualServer_t *virtual_server, int *virtual_server_idx)
{
    rtdrv_rg_virtualServer_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == virtual_server), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == virtual_server_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.virtual_server, virtual_server, sizeof(rtk_rg_virtualServer_t));
    osal_memcpy(&cfg.virtual_server_idx, virtual_server_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_VIRTUALSERVER_ADD, &cfg, rtdrv_rg_virtualServer_add_t, 1);
    osal_memcpy(virtual_server, &cfg.virtual_server, sizeof(rtk_rg_virtualServer_t));
    osal_memcpy(virtual_server_idx, &cfg.virtual_server_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_virtualServer_add */

/* Function Name:
 *      rtk_rg_virtualServer_del
 * Description:
 *      Delete one server port connection rule.
 * Input:
 * Output:
 *      virtual_server_idx - [in]<tab>the index of virtual server entry for deleting.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM - illegal server port rule index.
 * Note:
 *      None.
 */
int32 
rtk_rg_virtualServer_del(int virtual_server_idx)
{
    rtdrv_rg_virtualServer_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.virtual_server_idx, &virtual_server_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_VIRTUALSERVER_DEL, &cfg, rtdrv_rg_virtualServer_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_virtualServer_del */

/* Function Name:
 *      rtk_rg_virtualServer_find
 * Description:
 *      Find entire virtual server table from valid_idx till valid one.
 * Input:
 * Output:
 *      *virtual_server - [in]<tab>An empty buffer for storing the virtual server entry data structure.<nl>[out]<tab>The data structure of found virtual server entry.
 *      *valid_idx - [in]<tab>The index which find from.<nl>[out]<tab>The existing entry index.
 * Return:
 *      RT_ERR_RG_OK
 * 	 	RT_ERR_RG_INITPM_UNINIT - the virtual server table uninitialized.
 * 	 	RT_ERR_RG_SVRPORT_SW_ENTRY_NOT_FOUND - can't find entry in virtual server table.
 * Note:
 *     	virtual_server.is_tcp - Layer 4 protocol is TCP or not.<nl>	
 *		virtual_server.wan_intf_idx - Wan interface index of server.<nl>	
 *		virtual_server.gateway_port_start - Gateway external port mapping start number.<nl>	
 *		virtual_server.local_ip - The translating local IP address.<nl>	
 *		virtual_server.local_port_start - The translating internal port mapping start number.<nl>	
 *		virtual_server.mappingRangeCnt - The port mapping range count.<nl>
 *		virtual_server.valid - This entry is valid or not.<nl>
 */
int32 
rtk_rg_virtualServer_find(rtk_rg_virtualServer_t *virtual_server, int *valid_idx)
{
    rtdrv_rg_virtualServer_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == virtual_server), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.virtual_server, virtual_server, sizeof(rtk_rg_virtualServer_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_VIRTUALSERVER_FIND, &cfg, rtdrv_rg_virtualServer_find_t, 1);
    osal_memcpy(virtual_server, &cfg.virtual_server, sizeof(rtk_rg_virtualServer_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_virtualServer_find */



/* Sub-module Name: ACL Filter */


/* Function Name:
 *      rtk_rg_aclFilterAndQos_add
 * Description:
 *      Add acl rule.
 * Input:
 * Output:
 *	acl_filter - [in]<tab>assign the patterns which need to be filtered, and assign the related action(include drop, permit, Qos, and Trap to CPU).
 *      acl_filter_idx  - [out]<tab>the index of the added acl rule.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)  
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_ACL_CF_FIELD_CONFLICT - acl_filter assigned some conflict patterns
 *      RT_ERR_RG_ACL_CF_FLOW_DIRECTION_ERROR - the assigned ingress interface and egress interface are not upstream or downstream
 *      RT_ERR_RG_ACL_ENTRY_FULL - the hardware ACL asic entry is full
 *      RT_ERR_RG_ACL_ENTRY_ACCESS_FAILED - access the hardware ACL asic entry failed
 *      RT_ERR_RG_ACL_IPTABLE_FULL - the hardware asic ACL IP_RANGE_TABLE entry is full
 *      RT_ERR_RG_ACL_IPTABLE_ACCESS_FAILED - access the hardware asic ACL IP_RANGE_TABLE entry failed
 *      RT_ERR_RG_ACL_PORTTABLE_FULL - the hardware asic ACL PORT_RANGE_TABLE entry is full
 *      RT_ERR_RG_ACL_PORTTABLE_ACCESS_FAILED - access the hardware ACL asic PORT_RANGE_TABLE entry failed
 *      RT_ERR_RG_CF_ENTRY_FULL - the hardware Classification asic entry is full
 *      RT_ERR_RG_CF_ENTRY_ACCESS_FAILED - access the hardware Classification asic entry failed
 *      RT_ERR_RG_CF_IPTABLE_FULL - the hardware Classification asic IP_RANGE_TABLE entry is full
 *      RT_ERR_RG_CF_IPTABLE_ACCESS_FAILED - access the hardware Classification asic IP_RANGE_TABLE entry failed
 *      RT_ERR_RG_CF_PORTTABLE_FULL - the hardware Classification PORT_RANGE_TABLE asic entry is full
 *      RT_ERR_RG_CF_PORTTABLE_ACCESS_FAILED - access the hardware Classification asic PORT_RANGE_TABLE entry failed
 *      RT_ERR_RG_CF_DSCPTABLE_FULL - the hardware Classification DSCP_REMARKING_TABLE asic entry is full
 *      RT_ERR_RG_CF_DSCPTABLE_ACCESS_FAILED - access the hardware Classification asic DSCP_REMARKING_TABLE entry failed
 *      RT_ERR_RG_ACL_SW_ENTRY_FULL - the software ACL entry is full
 *      RT_ERR_RG_ACL_SW_ENTRY_ACCESS_FAILED - access the software ACL entry failed
 * Note:
 *      acl_filter.filter_fields - use to enable the filtered patterns. Each pattern bit should be "or" together.<nl>
 *      acl_filter.ingress_port_mask - assign the packet ingress physical port pattern(valid when INGRESS_PORT_BIT is enable)<nl>
 *      acl_filter.ingress_dscp - assign the packet ingress dscp(valid when INGRESS_DSCP_BIT is enable)<nl>
 *      acl_filter.ingress_intf_idx - assign the packet ingress interface index(valid when INGRESS_INTF_BIT is enable)<nl>
 *      acl_filter.egress_intf_idx - assign the packet egress interface index(valid when EGRESS_INTF_BIT is enable)<nl>
 *      acl_filter.ingress_ethertype - assign the packet ingress ethertype pattern(valid when INGRESS_ETHERTYPE_BIT is enable)<nl>
 *      acl_filter.ingress_ctag_vid - assign the packet ingress vlan id pattern(valid when INGRESS_CTAG_VID_BIT is enable)<nl>
 *      acl_filter.ingress_ctag_pri - assign the packet ingress vlan priority pattern(valid when INGRESS_CTAG_PRI_BIT is enable)<nl>
 *      acl_filter.ingress_smac - assign the packet ingress source mac pattern(valid when INGRESS_SMAC_BIT is enable)<nl>
 *      acl_filter.ingress_dmac - assign the packet ingress destination mac pattern(valid when INGRESS_DMAC_BIT is enable)<nl>
 *      acl_filter.ingress_src_ipv4_addr_start - assign the packet ingress source ipv4 lower bound(valid when INGRESS_IPV4_SIP_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_src_ipv4_addr_end - assign the packet ingress source ipv4 upper bound(valid when INGRESS_IPV4_SIP_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_dest_ipv4_addr_start - assign the packet ingress destination ipv4 lower bound(valid when INGRESS_IPV4_DIP_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_dest_ipv4_addr_end - assign the packet ingress destination ipv4 upper bound(valid when INGRESS_IPV4_DIP_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_src_ipv6_addr_start - assign the packet ingress source ipv6 lower bound(valid when INGRESS_IPV6_SIP_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_src_ipv6_addr_end - assign the packet ingress source ipv6 upper bound(valid when INGRESS_IPV6_SIP_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_dest_ipv6_addr_start - assign the packet ingress destination ipv6 lower bound(valid when INGRESS_IPV6_DIP_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_dest_ipv6_addr_end - assign the packet ingress destination ipv6 upper bound(valid when INGRESS_IPV6_DIP_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_src_l4_port_start - assign the packet ingress layer4 source port lower bound(valid when INGRESS_L4_SPORT_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_src_l4_port_end - assign the packet ingress layer4 source port upper bound(valid when INGRESS_L4_SPORT_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_dest_l4_port_start - assign the packet ingress layer4 destination port lower bound(valid when INGRESS_L4_DPORT_RANGE_BIT is enable)<nl>
 *      acl_filter.ingress_dest_l4_port_end - assign the packet ingress layer4 destination port upper bound(valid when INGRESS_L4_DPORT_RANGE_BIT is enable)<nl>
 *      acl_filter.egress_src_ipv4_addr_start - assign the packet egress source ipv4 lower bound(valid when EGRESS_IPV4_SIP_RANGE_BIT is enable)<nl>
 *      acl_filter.egress_src_ipv4_addr_end - assign the packet egress source ipv4 upper bound(valid when EGRESS_IPV4_SIP_RANGE_BIT is enable)<nl>
 *      acl_filter.egress_dest_ipv4_addr_start - assign the packet egress destination ipv4 lower bound(valid when EGRESS_IPV4_DIP_RANGE_BIT is enable)<nl>
 *      acl_filter.egress_dest_ipv4_addr_end - assign the packet egress destination ipv4 pattern(upper bound)(valid when EGRESS_IPV4_DIP_RANGE_BIT is enable)<nl>
 *      acl_filter.egress_src_l4_port_start - assign the packet egress layer4 source port lower bound(valid when EGRESS_L4_SPORT_RANGE_BIT is enable)<nl>
 *      acl_filter.egress_src_l4_port_end - assign the packet egress layer4 source port upper bound(valid when EGRESS_L4_SPORT_RANGE_BIT is enable)<nl>
 *      acl_filter.egress_dest_l4_port_start - assign the packet egress layer4 destination port lower bound(valid when EGRESS_L4_DPORT_RANGE_BIT is enable)<nl>
 *      acl_filter.egress_dest_l4_port_end - assign the packet egress layer4 destination port upper bound(valid when EGRESS_L4_DPORT_RANGE_BIT is enable)<nl>
 *      acl_filter.action_type - assign the action to the packets which satisfy the assgned patterns<nl>
 *      acl_filter.qos_actions - assign the qos action. Each action bit should be "or" together (triggered while action_type==ACL_ACTION_TYPE_QOS)<nl>
 *      acl_filter.action_dot1p_remarking_pri - assign the vlan priority value for remarking(valid when while ACL_ACTION_1P_REMARKING_BIT is enable)<nl>
 *      acl_filter.action_ip_precedence_remarking_pri - assign the ip precedence value for remarking(valid when  ACL_ACTION_IP_PRECEDENCE_REMARKING_BIT is enable)<nl>
 *      acl_filter.action_dscp_remarking_pri - assign the dscp value for remarking(valid when  ACL_ACTION_DSCP_REMARKING_BIT is enable)<nl>
 *      acl_filter.action_queue_id - assign the qid(valid when  ACL_ACTION_QUEUE_ID_BIT is enable)<nl>
 *      acl_filter.action_share_meter - assign the sharemeter(valid when  ACL_ACTION_SHARE_METER_BIT is enable)<nl>
 *      acl_filter.action_policy_route_wan - assign the wan interface index(valid when  typs is ACL_ACTION_TYPE_POLICY_ROUTE)<nl>
 */

int32
rtk_rg_aclFilterAndQos_add(rtk_rg_aclFilterAndQos_t *acl_filter, int *acl_filter_idx)
{
    rtdrv_rg_aclFilterAndQos_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == acl_filter), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == acl_filter_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.acl_filter, acl_filter, sizeof(rtk_rg_aclFilterAndQos_t));
    osal_memcpy(&cfg.acl_filter_idx, acl_filter_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_ACLFILTERANDQOS_ADD, &cfg, rtdrv_rg_aclFilterAndQos_add_t, 1);
    osal_memcpy(acl_filter, &cfg.acl_filter, sizeof(rtk_rg_aclFilterAndQos_t));
    osal_memcpy(acl_filter_idx, &cfg.acl_filter_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_aclFilterAndQos_add */

/* Function Name:
 *      rtk_rg_aclFilterAndQos_del
 * Description:
 *      Delete acl rule.
 * Input:
 * Output:
 *	    acl_filter_idx - [in]<tab>the index of the acl rule which need to be delete
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_ACL_ENTRY_ACCESS_FAILED - access the hardware ACL asic entry failed
 *      RT_ERR_RG_ACL_IPTABLE_ACCESS_FAILED - access the hardware asic ACL IP_RANGE_TABLE entry failed
 *      RT_ERR_RG_ACL_PORTTABLE_ACCESS_FAILED - access the hardware ACL asic PORT_RANGE_TABLE entry failed
 *      RT_ERR_RG_CF_ENTRY_ACCESS_FAILED - access the hardware Classification asic entry failed
 *      RT_ERR_RG_CF_IPTABLE_ACCESS_FAILED - access the hardware Classification asic IP_RANGE_TABLE entry failed
 *      RT_ERR_RG_CF_PORTTABLE_ACCESS_FAILED - access the hardware Classification asic PORT_RANGE_TABLE entry failed
 *      RT_ERR_RG_CF_DSCPTABLE_ACCESS_FAILED - access the hardware Classification asic DSCP_REMARKING_TABLE entry failed
 *      RT_ERR_RG_ACL_SW_ENTRY_ACCESS_FAILED - access the software ACL entry failed
 * Note:
 *      None
 */
int32
rtk_rg_aclFilterAndQos_del(int acl_filter_idx)
{
    rtdrv_rg_aclFilterAndQos_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.acl_filter_idx, &acl_filter_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_ACLFILTERANDQOS_DEL, &cfg, rtdrv_rg_aclFilterAndQos_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_aclFilterAndQos_del */

/* Function Name:
 *      rtk_rg_aclFilterAndQos_find
 * Description:
 *      fine acl rule.
 * Input:
 *	  valid_idx - the index of the acl rule which start to search
 * Output:
 *      acl_filter - [out]<tab>the acl rule which be found
 *      valid_idx - [in]<tab>the index of the acl rule which start to search.<nl>[out]<tab>the index of found acl rule
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 *      RT_ERR_RG_ACL_SW_ENTRY_ACCESS_FAILED - access the software ACL entry failed
 *      RT_ERR_RG_ACL_SW_ENTRY_NOT_FOUND - can not find the assigned ACL entry
 * Note:
 *      this API search the software acl entry start from acl_filter_idx, and find the first exist acl entry. If all entry after the acl_filter_idx are empty, it will return RT_ERR_RG_ACL_SW_ENTRY_NOT_FOUND
 */
int32
rtk_rg_aclFilterAndQos_find(rtk_rg_aclFilterAndQos_t *acl_filter, int *valid_idx)
{
    rtdrv_rg_aclFilterAndQos_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == acl_filter), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.acl_filter, acl_filter, sizeof(rtk_rg_aclFilterAndQos_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_ACLFILTERANDQOS_FIND, &cfg, rtdrv_rg_aclFilterAndQos_find_t, 1);
    osal_memcpy(acl_filter, &cfg.acl_filter, sizeof(rtk_rg_aclFilterAndQos_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_aclFilterAndQos_find */

//MAC Filter

/* Sub-module Name: Mac Filter */


/* Function Name:
 *      rtk_rg_macFilter_add
 * Description:
 *      add mac filter rule
 * Input:
 * Output:
 *        macFilterEntry.mac - [in]<tab>the mac address which need to be add to mac filter rule.
 *        macFilterEntry.direct - [in]<tab>the mac address which shouuld be filter in SMAC, DMAC, or BOTH.
 *      mac_filter_idx - [out]<tab>the index of added mac filter rule.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32
rtk_rg_macFilter_add(rtk_rg_macFilterEntry_t *macFilterEntry,int *mac_filter_idx)
{
    rtdrv_rg_macFilter_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == macFilterEntry), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == mac_filter_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.macFilterEntry, macFilterEntry, sizeof(rtk_rg_macFilterEntry_t));
    osal_memcpy(&cfg.mac_filter_idx, mac_filter_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_MACFILTER_ADD, &cfg, rtdrv_rg_macFilter_add_t, 1);
    osal_memcpy(macFilterEntry, &cfg.macFilterEntry, sizeof(rtk_rg_macFilterEntry_t));
    osal_memcpy(mac_filter_idx, &cfg.mac_filter_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_macFilter_add */


/* Function Name:
 *      rtk_rg_macFilter_del
 * Description:
 *      delete mac filter rule
 * Input:
 * Output:
 *          mac_filter_idx - [in]<tab>the index of the mac filter rule which need to be deleted.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32
rtk_rg_macFilter_del(int mac_filter_idx)
{
    rtdrv_rg_macFilter_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.mac_filter_idx, &mac_filter_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_MACFILTER_DEL, &cfg, rtdrv_rg_macFilter_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_macFilter_del */


/* Function Name:
 *      rtk_rg_macFilter_find
 * Description:
 *      find mac filter rule
 * Input:
 * Output:
 *      macFilterEntry.mac - [out]<tab>the mac address which be found.
 *        macFilterEntry.direct - [out]<tab>filter for SMAC, DMAC, or BOTH.
 *      valid_idx - [in]<tab>the index of mac filter rule which start to search.<nl>[out]<tab>the index of found mac filter rule.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32
rtk_rg_macFilter_find(rtk_rg_macFilterEntry_t *macFilterEntry, int *valid_idx)
{
    rtdrv_rg_macFilter_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == macFilterEntry), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.macFilterEntry, macFilterEntry, sizeof(rtk_rg_macFilterEntry_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_MACFILTER_FIND, &cfg, rtdrv_rg_macFilter_find_t, 1);
    osal_memcpy(macFilterEntry, &cfg.macFilterEntry, sizeof(rtk_rg_macFilterEntry_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_macFilter_find */



/* Sub-module Name: URL Filter */

/* Function Name:
 *      rtk_rg_urlFilterString_add
 * Description:
 *      add url filter rule
 * Input:
 * Output:
 *	    filter - [in]<tab>the url rule which need to be added to url filter rule.
 *      url_idx - [out]<tab>the index of added url filter rule.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      filter.url_filter_string - the string which need to be filtered in url string.(ex: in url "http://www.sample.com/explain", "www.sample.com" is the url string)<nl>
 *      filter.path_filter_string - the string which need to be filtered in url path string.(ex: in url "http://www.sample.com/explain", "/explain" is the url path string)<nl>
 *      filter.path_exactly_match - the urlFilter will execute even the path_filter_string is part of url path string while path_exactly_match is 0. Else, the path_filter_string must exactly match the url path string to trigger urlFilter execution.<nl>
 *      filter.wan_intf - the index of the wan interface which should limited by this urlFilter.
 */
int32 
rtk_rg_urlFilterString_add(rtk_rg_urlFilterString_t *filter,int *url_idx)
{
    rtdrv_rg_urlFilterString_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == filter), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == url_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.filter, filter, sizeof(rtk_rg_urlFilterString_t));
    osal_memcpy(&cfg.url_idx, url_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_URLFILTERSTRING_ADD, &cfg, rtdrv_rg_urlFilterString_add_t, 1);
    osal_memcpy(filter, &cfg.filter, sizeof(rtk_rg_urlFilterString_t));
    osal_memcpy(url_idx, &cfg.url_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_urlFilterString_add */

/* Function Name:
 *      rtk_rg_urlFilterString_del
 * Description:
 *      delete url filter rule
 * Input:
 * Output:
 *	    url_idx - [in]<tab>the index of the url filter rule which need to be deleted.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32 
rtk_rg_urlFilterString_del(int url_idx)
{
    rtdrv_rg_urlFilterString_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.url_idx, &url_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_URLFILTERSTRING_DEL, &cfg, rtdrv_rg_urlFilterString_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_urlFilterString_del */

/* Function Name:
 *      rtk_rg_urlFilterString_find
 * Description:
 *      find url filter rule
 * Input:
 * Output:
 *      filter - [out]<tab>the url filter rule which be found.
 *      valid_idx - [in]<tab>the index fo url filter rule which start to search.<nl>[out]<tab>the index of found url filter rule.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      filter.url_filter_string - the string which need to be filtered in url string.(ex: in url "http://www.sample.com/explain", "www.sample.com" is the url string)<nl>
 *      filter.path_filter_string - the string which need to be filtered in url path string.(ex: in url "http://www.sample.com/explain", "/explain" is the url path string)<nl>
 *      filter.path_exactly_match - the urlFilter will execute even the path_filter_string is part of url path string while path_exactly_match is 0. Else, the path_filter_string must exactly match the url path string to trigger urlFilter execution.<nl>
 *      filter.wan_intf - the index of the wan interface which should be limited by this urlFilter.
 */
int32 
rtk_rg_urlFilterString_find(rtk_rg_urlFilterString_t *filter, int *valid_idx)
{
    rtdrv_rg_urlFilterString_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == filter), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.filter, filter, sizeof(rtk_rg_urlFilterString_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_URLFILTERSTRING_FIND, &cfg, rtdrv_rg_urlFilterString_find_t, 1);
    osal_memcpy(filter, &cfg.filter, sizeof(rtk_rg_urlFilterString_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_urlFilterString_find */


/* Sub-module Name: UPnP configuration */

/* Function Name:
 *      rtk_rg_upnpConnection_add
 * Description:
 *      Add UPNP connection rule.
 * Input:
 * Output:
 *      rtk_rg_upnpConnection_t - [in]<tab>the UPNP connection data structure.
 *      upnp_idx - [in]<tab>the index of UPNP table.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_INITPM_UNINIT - the UPNP table uninitialized.
 *		RT_ERR_RG_ENTRY_FULL - the UPNP mapping table is full.
 * Note:
 *		upnp.is_tcp - Layer 4 protocol is TCP or not.<nl>
 *		upnp.valid - The UNPN mapping is valid or not.<nl>
 *		upnp.wan_intf_idx - Wan interface index.<nl>
 *		upnp.gateway_port - Gateway external port number.<nl>
 *		upnp.local_ip - Internal ip address.<nl>
 *		upnp.local_port - Internal port number.<nl>
 *		upnp.limit_remote_ip - The Restricted remote IP address.<nl>
 *		upnp.limit_remote_port - The Restricted remote port number.<nl>
 *		upnp.remote_ip - Remote IP address.<nl
 *		upnp.emote_port - Remote port number.<nl>
 *		upnp.type - One shot or persist.<nl>
 *		upnp.timeout - timeout value for auto-delete. Set 0 to disable auto-delete.
 */
int32 
rtk_rg_upnpConnection_add(rtk_rg_upnpConnection_t *upnp, int *upnp_idx)
{
    rtdrv_rg_upnpConnection_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == upnp), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == upnp_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.upnp, upnp, sizeof(rtk_rg_upnpConnection_t));
    osal_memcpy(&cfg.upnp_idx, upnp_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_UPNPCONNECTION_ADD, &cfg, rtdrv_rg_upnpConnection_add_t, 1);
    osal_memcpy(upnp, &cfg.upnp, sizeof(rtk_rg_upnpConnection_t));
    osal_memcpy(upnp_idx, &cfg.upnp_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_upnpConnection_add */

/* Function Name:
 *      rtk_rg_upnpConnection_del
 * Description:
 *      Delete UPNP connection rule.
 * Input:
 * Output:
 *      upnp_idx - [in]<tab>the index of UPNP table entry to be deleted.
 *      None.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM - illegal UPNP rule index.
 * Note:
 *      None.
 */
int32 
rtk_rg_upnpConnection_del(int upnp_idx)
{
    rtdrv_rg_upnpConnection_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.upnp_idx, &upnp_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_UPNPCONNECTION_DEL, &cfg, rtdrv_rg_upnpConnection_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_upnpConnection_del */

/* Function Name:
 *      rtk_rg_upnpConnection_find
 * Description:
 *      Find entire UPNP mapping table from upnp_idx till valid one.
 * Input:
 * Output:
 *      *upnp - [in]<tab>An empty buffer for storing the UPNP mapping entry data structure.<nl>[out]<tab>The data structure of found UPNP mapping entry.
 *      *valid_idx - [in]<tab>The index which find from.<nl>[out]<tab>The existing entry index.
 * Return:
 *      RT_ERR_RG_OK
 * 	 	RT_ERR_RG_INITPM_UNINIT - the UPNP table uninitialized.
 *      RT_ERR_RG_INVALID_PARAM - illegal UPNP rule index.
 * 	 	RT_ERR_RG_UPNP_SW_ENTRY_NOT_FOUND - can't find entry in UPNP mapping table.
 * Note:
 *		upnp.is_tcp - Layer 4 protocol is TCP or not.<nl>
 *		upnp.valid - The UNPN mapping is valid or not.<nl>
 *		upnp.wan_intf_idx - Wan interface index.<nl>
 *		upnp.gateway_port - Gateway external port number.<nl>
 *		upnp.local_ip - Internal ip address.<nl>
 *		upnp.local_port - Internal port number.<nl>
 *		upnp.limit_remote_ip - The Restricted remote IP address.<nl>
 *		upnp.limit_remote_port - The Restricted remote port number.<nl>
 *		upnp.remote_ip - Remote IP address.<nl
 *		upnp.emote_port - Remote port number.<nl>
 *		upnp.type - One shot or persist.<nl>
 *		upnp.timeout - timeout value for auto-delete. Set 0 to disable auto-delete.
 *		The condition fields will be ignore while it was set to zero.
 */
int32 
rtk_rg_upnpConnection_find(rtk_rg_upnpConnection_t *upnp, int *valid_idx)
{
    rtdrv_rg_upnpConnection_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == upnp), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.upnp, upnp, sizeof(rtk_rg_upnpConnection_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_UPNPCONNECTION_FIND, &cfg, rtdrv_rg_upnpConnection_find_t, 1);
    osal_memcpy(upnp, &cfg.upnp, sizeof(rtk_rg_upnpConnection_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_upnpConnection_find */


// The following function is used for Lite-RomeDriver architecture only.

/* Sub-module Name: NAPT configuration */

/* Function Name:
 *      rtk_rg_naptConnection_add
 * Description:
 *      Add NAPT connection flow.
 * Input:
 * Output:
 *      *naptFlow - [in]<tab>the NAPT connection flow data structure.
 *      *flow_idx - [out]<tab>the NAPT flow index.
 * Return:
 *      RT_ERR_RG_OK
 *		RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INVALID_PARAM - illegal NAPT connection flow index.
 *		RT_ERR_RG_EXTIP_GET_FAIL - illegal wan_intf_idx.
 *		RT_ERR_RG_NAPT_FLOW_DUPLICATE - duplicate NAPT flow.
 *		RT_ERR_RG_NAPTR_OVERFLOW - NAPTR table collision.
 *		RT_ERR_RG_NAPT_OVERFLOW	- NAPT table collision.
 *		RT_ERR_RG_NAPTR_SET_FAIL -  write to NAPTR table failed.
 *		RT_ERR_RG_NAPT_SET_FAIL - write to NAPT table failed.
 * Note:
 *		naptFlow.is_tcp - Layer 4 protocol is TCP or not.<nl>
 *		naptFlow.local_ip - Internal IP address.<nl>
 *		naptFlow.remote_ip - Remote IP address.<nl>
 *		naptFlow.wan_intf_idx - Wan interface index.<nl>
 *		naptFlow.local_port - Internal port number.<nl>
 *		naptFlow.remote_port - Remote port number.<nl>
 *		naptFlow.external_port - Gateway external port number.<nl>
 *		naptFlow.pri_valid - NAPT priority remarking enable.<nl>
 *		naptFlow.priority - NAPT priority remarking value.<nl>
 */
int32 
rtk_rg_naptConnection_add(rtk_rg_naptEntry_t *naptFlow, int *flow_idx)
{
    rtdrv_rg_naptConnection_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == naptFlow), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == flow_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.naptFlow, naptFlow, sizeof(rtk_rg_naptEntry_t));
    osal_memcpy(&cfg.flow_idx, flow_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_NAPTCONNECTION_ADD, &cfg, rtdrv_rg_naptConnection_add_t, 1);
    osal_memcpy(naptFlow, &cfg.naptFlow, sizeof(rtk_rg_naptEntry_t));
    osal_memcpy(flow_idx, &cfg.flow_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_naptConnection_add */

/* Function Name:
 *      rtk_rg_naptConnection_del
 * Description:
 *      Delete NAPT connection flow.
 * Input:
 * Output:
 *      flow_idx - [in]<tab>The index of NAPT connection flow table.
 *      None.
 * Return:
 *      RT_ERR_RG_OK
 *		RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INVALID_PARAM - illegal NAPT connection flow index.
 *		RT_ERR_RG_NAPT_SW_ENTRY_NOT_FOUND - the NAPT entry can not be found.
 *		RT_ERR_RG_NAPTR_SET_FAIL -  write to NAPTR table failed.
 *		RT_ERR_RG_NAPT_SET_FAIL - write to NAPT table failed.
 * Note:
 *		None.
 */
int32 
rtk_rg_naptConnection_del(int flow_idx)
{
    rtdrv_rg_naptConnection_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.flow_idx, &flow_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_NAPTCONNECTION_DEL, &cfg, rtdrv_rg_naptConnection_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_naptConnection_del */

/* Function Name:
 *      rtk_rg_naptConnection_find
 * Description:
 *      Find entire NAPT table from index valid_idx till valid one.
 * Input:
 * Output:
 *      *naptInfo - [in]<tab>An empty buffer for storing the NAPT entry data structure.<nl>[out]<tab>The data structure of found NAPT entry.
 *      *valid_idx - [in]<tab>The index which find from.<nl>[out]<tab>The existing entry index.
 * Return:
 *      RT_ERR_RG_OK
 *		RT_ERR_RG_NOT_INIT - system is not initiated. 
 *		RT_ERR_RG_INVALID_PARAM - illegal NAPT connection flow index.
 *		RT_ERR_RG_NAPT_SW_ENTRY_NOT_FOUND - the NAPT entry can not be found.
 * Note:
 *		naptInfo.is_tcp - Layer 4 protocol is TCP or not.<nl>
 *		naptInfo.local_ip - Internal IP address.<nl>
 *		naptInfo.remote_ip - Remote IP address.<nl>
 *		naptInfo.wan_intf_idx - Wan interface index.<nl>
 *		naptInfo.local_port - Internal port number.<nl>
 *		naptInfo.remote_port - Remote port number.<nl>
 *		naptInfo.external_port - Gateway external port number.<nl>
 *		naptInfo.pri_valid - NAPT priority remarking enable.<nl>
 *		naptInfo.priority - NAPT priority remarking value.<nl>
 *		naptInfo.idleSecs - NAPT flow idle seconds.<nl>
 *		naptInfo.state - NAPT connection state, 0:INVALID 1:SYN_RECV 2:SYN_ACK_RECV 3:CONNECTED 4:FIN_RECV 5:RST_RECV<nl>
 */
int32 
rtk_rg_naptConnection_find(rtk_rg_naptInfo_t *naptInfo,int *valid_idx)
{
    rtdrv_rg_naptConnection_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == naptInfo), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.naptInfo, naptInfo, sizeof(rtk_rg_naptInfo_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_NAPTCONNECTION_FIND, &cfg, rtdrv_rg_naptConnection_find_t, 1);
    osal_memcpy(naptInfo, &cfg.naptInfo, sizeof(rtk_rg_naptInfo_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_naptConnection_find */


/* Sub-module Name: Multicast Flow */

/* Function Name:
 *      rtk_rg_multicastFlow_add
 * Description:
 *      Add a Multicast flow entry into ASIC.
 * Input: 
  * Output: 
 *      *mcFlow - [in]<tab>Multicast flow entry content structure.
 *      *flow_idx - [out]<tab>Returned Multicast flow index.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - System is not initiated.  
 *      RT_ERR_RG_INVALID_PARAM - The params is not accpeted.
 *      RT_ERR_RG_ENTRY_FULL - The MC entry is full. 
 *      RT_ERR_FAILED - Failed
 *      RT_ERR_NOT_INIT      - The module is not initial
 *      RT_ERR_IPV4_ADDRESS  - Invalid IPv4 address
 *      RT_ERR_VLAN_VID      - invalid vlan id
 *      RT_ERR_NULL_POINTER  - input parameter may be null pointer
 *      RT_ERR_INPUT         - invalid input parameter
 *
 * Note:
 *      mcFlow.multicast_ipv4_addr - The destination IP address of packet, must be class D address.<nl>
 *      mcFlow.port_mask - This MC packet will forward to those ports<nl>
 *      mcFlow.ext_port_mask - This MC packet will forward to those extension ports.
 */
int32 
rtk_rg_multicastFlow_add(rtk_rg_multicastFlow_t *mcFlow, int *flow_idx)
{
    rtdrv_rg_multicastFlow_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == mcFlow), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == flow_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.mcFlow, mcFlow, sizeof(rtk_rg_multicastFlow_t));
    osal_memcpy(&cfg.flow_idx, flow_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_MULTICASTFLOW_ADD, &cfg, rtdrv_rg_multicastFlow_add_t, 1);
    osal_memcpy(mcFlow, &cfg.mcFlow, sizeof(rtk_rg_multicastFlow_t));
    osal_memcpy(flow_idx, &cfg.flow_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_multicastFlow_add */


 /* Function Name:
 *      rtk_rg_multicastFlow_del
 * Description:
 *      Delete an Multicast flow ASIC entry.
 * Input:
 * Output:
 *      flow_idx - [in]<tab>The Multicast flow entry index for deleting. 
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE - the index out of range.
 *      RT_ERR_RG_ENTRY_NOT_EXIST - the index is not exist.
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_L2_EMPTY_ENTRY   - Empty LUT entry.
 *      RT_ERR_INPUT            - Invalid input parameters.
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid) 
 * Note:
 *      None
 */
int32 
rtk_rg_multicastFlow_del(int flow_idx)
{
    rtdrv_rg_multicastFlow_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.flow_idx, &flow_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_MULTICASTFLOW_DEL, &cfg, rtdrv_rg_multicastFlow_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_multicastFlow_del */

/* Function Name:
 *		rtk_rg_l2MultiCastFlow_add
 * Description:
 *      Add an L2 Multicast flow ASIC entry. 
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - System is not initiated.  
 *      RT_ERR_RG_INVALID_PARAM - The params is not accpeted.
 *      RT_ERR_RG_ENTRY_FULL - The MC entry is full.
 *      RT_ERR_F: Is a directoryAILE
 * Note:
 *      l2McFlow.mac - The destination Multicast MAC address of packet, it must be 01-00-5E-xx-xx-xx.<nl>
 *      l2McFlow.port_mask - This MC packet will forward to those MAC ports.<nl>
 *      l2McFlow.isIVL - isIVL=1:IVL, isIVL=0:SVL
 *		l2McFlow.vlanID - vlan ID, only use for isIVL=1.
 */
int32
rtk_rg_l2MultiCastFlow_add(rtk_rg_l2MulticastFlow_t *l2McFlow,int *flow_idx)
{
	rtdrv_rg_l2MulticastFlow_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == l2McFlow), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == flow_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.l2McFlow, l2McFlow, sizeof(rtk_rg_l2MulticastFlow_t));
    osal_memcpy(&cfg.flow_idx, flow_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_L2MULTICASTFLOW_ADD, &cfg, rtdrv_rg_l2MulticastFlow_add_t, 1);
    osal_memcpy(l2McFlow, &cfg.l2McFlow, sizeof(rtk_rg_l2MulticastFlow_t));
    osal_memcpy(flow_idx, &cfg.flow_idx, sizeof(int));

    return RT_ERR_OK;

}/* end of rtk_rg_l2MultiCastFlow_add */


/* Function Name:
 *      rtk_rg_multicastFlow_find
 * Description:
 *      Find an exist Multicast flow ASIC entry.
 * Input:
 * Output: 
 *      *mcFlow - [in]<tab>An empty buffer for storing the structure.<nl>[out]<tab>An exist entry structure which index is valid_idx.
 *      *valid_idx - [in]<tab>The index which find from.<nl>[out]<tab>The exist entry index.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE - the index out of range.
 *      RT_ERR_RG_NO_MORE_ENTRY_FOUND - no more exist entry is found.
 *      RT_ERR_FAILED - Failed
 *      RT_ERR_L2_EMPTY_ENTRY - Empty LUT entry.
 *      RT_ERR_INPUT  - Invalid input parameters. 
 * Note:
 *      None 
 */
int32 
rtk_rg_multicastFlow_find(rtk_rg_multicastFlow_t *mcFlow, int *valid_idx)
{
    rtdrv_rg_multicastFlow_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == mcFlow), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.mcFlow, mcFlow, sizeof(rtk_rg_multicastFlow_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_MULTICASTFLOW_FIND, &cfg, rtdrv_rg_multicastFlow_find_t, 1);
    osal_memcpy(mcFlow, &cfg.mcFlow, sizeof(rtk_rg_multicastFlow_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_multicastFlow_find */

/* Sub-module Name: Layer2 configuration */

/* Function Name:
 *      rtk_rg_macEntry_add
 * Description:
 *      Add a MAC Entry into ASIC
 * Input:
 * Output:
 *      *macEntry - [in]<tab>MAC entry content structure.
 *      *entry_idx - [out]<tab>this MAC entry index.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 * Note:
 *      macEntry.mac - The MAC address.<nl>
 *      macEntry.isIVL - The MAC is for IVL or SVL.<nl>
 *      macEntry.fid - The fid, only used in SVL.<nl>
 *      macEntry.vlan_id - The VLAN id.<nl>
 *      macEntry.port_idx - The port index,0~5 for phy. port, 6 for CPU port, 7~11 for ext. port.<nl>
 *      macEntry.arp_used - The entry is used for NAT/NAPT.<nl>
 *      macEntry.static_entry - The MAC is static or not.<nl>
 *      macEntry.auth - The 802.1x authorized state.<nl>
 */
int32 
rtk_rg_macEntry_add(rtk_rg_macEntry_t *macEntry, int *entry_idx)
{
    rtdrv_rg_macEntry_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == macEntry), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == entry_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.macEntry, macEntry, sizeof(rtk_rg_macEntry_t));
    osal_memcpy(&cfg.entry_idx, entry_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_MACENTRY_ADD, &cfg, rtdrv_rg_macEntry_add_t, 1);
    osal_memcpy(macEntry, &cfg.macEntry, sizeof(rtk_rg_macEntry_t));
    osal_memcpy(entry_idx, &cfg.entry_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_macEntry_add */


/* Function Name:
 *      rtk_rg_macEntry_del
 * Description:
 *      Delete an ASIC MAC Entry.
 * Input:
 * Output:
 *      entry_idx - [in]<tab>The MAC entry index for deleting. 
 * Return:
 *      RT_ERR_RG_OK - Success 
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE - the index out of range.
 *      RT_ERR_RG_ENTRY_NOT_EXIST - the index is not exist.
 * Note:
 *      None
 */
int32 
rtk_rg_macEntry_del(int entry_idx)
{
    rtdrv_rg_macEntry_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.entry_idx, &entry_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_MACENTRY_DEL, &cfg, rtdrv_rg_macEntry_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_macEntry_del */


/* Function Name:
 *      rtk_rg_macEntry_find
 * Description:
 *      Find an exist ASIC MAC Entry.
 * Input:
 * Output: 
 *      *macEntry - [in]<tab>An empty buffer for storing the MAC entry data structure.<nl>[out]<tab>The data structure of found MAC entry.
 *      *valid_idx - [in]<tab>The index which find from.<nl>[out]<tab>The existing entry index.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE - the index out of range.
 *      RT_ERR_RG_NO_MORE_ENTRY_FOUND - no more exist entry is found.
 *      RT_ERR_RG_NULL_POINTER - Input parameter is NULL pointer.
 * Note:
 *      macEntry.mac - The MAC address.<nl>
 *      macEntry.isIVL - The MAC is for IVL or SVL.<nl>
 *      macEntry.fid - The fid, only used in SVL.<nl>
 *      macEntry.vlan_id - The VLAN id.<nl>
 *      macEntry.port_idx - The port index,0~5 for phy. port, 6 for CPU port, 7~11 for ext. port.<nl>
 *      macEntry.arp_used - The entry is used for NAT/NAPT.<nl>
 *      macEntry.static_entry - The MAC is static or not.<nl>
 *      macEntry.auth - The 802.1x authorized state.<nl>
 */
int32 
rtk_rg_macEntry_find(rtk_rg_macEntry_t *macEntry,int *valid_idx)
{
    rtdrv_rg_macEntry_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == macEntry), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.macEntry, macEntry, sizeof(rtk_rg_macEntry_t));
    osal_memcpy(&cfg.valid_idx, valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_MACENTRY_FIND, &cfg, rtdrv_rg_macEntry_find_t, 1);
    osal_memcpy(macEntry, &cfg.macEntry, sizeof(rtk_rg_macEntry_t));
    osal_memcpy(valid_idx, &cfg.valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_macEntry_find */


/* Function Name:
 *      rtk_rg_arpEntry_add
 * Description:
 *      Add an ARP Entry into ASIC
 * Input:
 * Output:
 *      arpEntry - [in]<tab>Fill rtk_rg_arpEntry_t each fields for adding.
 *      arp_entry_idx - [out]<tab>Return ARP entry index.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
 *      RT_ERR_RG_L2_ENTRY_NOT_FOUND - L2 entry not found.
 * Note:
 *      arpEntry.macEntryIdx - this MAC entry index of this ARP entry.<nl>
 *      arpEntry.ipv4Addr - the IPv4 IP address of this ARP entry.<nl>
 *      arpEntry.static_entry - this entry is static ARP, and it will never age out.<nl>
 *      If the arpEntry.static_entry is set. The MAC entry must set static, too.
 */
int32 
rtk_rg_arpEntry_add(rtk_rg_arpEntry_t *arpEntry, int *arp_entry_idx)
{
    rtdrv_rg_arpEntry_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == arpEntry), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == arp_entry_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.arpEntry, arpEntry, sizeof(rtk_rg_arpEntry_t));
    osal_memcpy(&cfg.arp_entry_idx, arp_entry_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_ARPENTRY_ADD, &cfg, rtdrv_rg_arpEntry_add_t, 1);
    osal_memcpy(arpEntry, &cfg.arpEntry, sizeof(rtk_rg_arpEntry_t));
    osal_memcpy(arp_entry_idx, &cfg.arp_entry_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_arpEntry_add */


/* Function Name:
 *      rtk_rg_arpEntry_del
 * Description:
 *      Delete an ASIC ARP Entry.
 * Input:
 * Output:
 *      arp_entry_idx - [out]<tab>The ARP entry index for deleting. 
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE - the index out of range.
 *      RT_ERR_RG_ENTRY_NOT_EXIST - the index is not exist.
 * Note:
 *      None
 */
int32 
rtk_rg_arpEntry_del(int arp_entry_idx)
{
    rtdrv_rg_arpEntry_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.arp_entry_idx, &arp_entry_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_ARPENTRY_DEL, &cfg, rtdrv_rg_arpEntry_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_arpEntry_del */


/* Function Name:
 *      rtk_rg_arpEntry_find
 * Description:
 *      Find the entire ASIC ARP table from index arp_valid_idx till valid one.
 * Input:
 * Output: 
 *      *arpInfo - [in]<tab>An empty buffer for storing the ARP entry data structure.<nl>[out]<tab>The data structure of found ARP entry.
 *      *arp_valid_idx - [in]<tab>The index which find from.<nl>[out]<tab>The existing entry index.
 * Return:
 *      RT_ERR_RG_OK - Success 
 *      RT_ERR_RG_NOT_INIT - system is not initiated.
 *      RT_ERR_RG_NULL_POINTER - input buffer pointer is NULL.
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE - the index out of range
 *      RT_ERR_RG_NO_MORE_ENTRY_FOUND - no more exist entry is found. 
 * Note:
 *      arpInfo.arpEntry - The ARP entry data structure.<nl>
 *      arpInfo.valid - The ARP entry is valid or not.<nl>
 *      arpEntry.idleSecs - The ARP entry idle time in seconds.<nl>
 *      arpEntry.lanNetInfo - The net info of the ARP entry.<nl>
 */
int32 
rtk_rg_arpEntry_find(rtk_rg_arpInfo_t *arpInfo,int *arp_valid_idx)
{
    rtdrv_rg_arpEntry_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == arpInfo), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == arp_valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.arpInfo, arpInfo, sizeof(rtk_rg_arpInfo_t));
    osal_memcpy(&cfg.arp_valid_idx, arp_valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_ARPENTRY_FIND, &cfg, rtdrv_rg_arpEntry_find_t, 1);
    osal_memcpy(arpInfo, &cfg.arpInfo, sizeof(rtk_rg_arpInfo_t));
    osal_memcpy(arp_valid_idx, &cfg.arp_valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_arpEntry_find */

/* Sub-module Name: ALG(Application Level Gateway) configuration */

/* Function Name:
 *      rtk_rg_algAppsServerInLanIpAddr_add
 * Description:
 *      Add ServerInLan ALG function IP mapping.
 * Input:
 * Output:
 *      srvIpMapping - [in]<tab>The mapping of bitmask of ALG ServerInLan functions and server IP address.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_ALG_SRV_IN_LAN_EXIST - the server ip address had been assigned
 * Note:
 *      srvIpMapping.algType - indicate which ALG service should assign to the serverAddress.<nl>
 *      srvIpMapping.serverAddress - indicate the server IP address.<nl>
 *      Before call rtk_rg_algApps_set to setup Server In Lan service, this IP mapping should be enter at first, otherwise rtk_rg_algApps_set will return failure.
 */
int32
rtk_rg_algServerInLanAppsIpAddr_add(rtk_rg_alg_serverIpMapping_t *srvIpMapping)
{
    rtdrv_rg_algServerInLanAppsIpAddr_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == srvIpMapping), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.srvIpMapping, srvIpMapping, sizeof(rtk_rg_alg_serverIpMapping_t));
    GETSOCKOPT(RTDRV_RG_ALGSERVERINLANAPPSIPADDR_ADD, &cfg, rtdrv_rg_algServerInLanAppsIpAddr_add_t, 1);
    osal_memcpy(srvIpMapping, &cfg.srvIpMapping, sizeof(rtk_rg_alg_serverIpMapping_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_algServerInLanAppsIpAddr_add */

/* Function Name:
 *      rtk_rg_algAppsServerInLanIpAddr_del
 * Description:
 *      Delete ServerInLan ALG function IP mapping.
 * Input:
 * Output:
 *       delServerMapping - [in]<tab>Delete the server IP address mapping.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      None
 */
int32
rtk_rg_algServerInLanAppsIpAddr_del(rtk_rg_alg_type_t delServerMapping)
{
    rtdrv_rg_algServerInLanAppsIpAddr_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.delServerMapping, &delServerMapping, sizeof(rtk_rg_alg_type_t));
    SETSOCKOPT(RTDRV_RG_ALGSERVERINLANAPPSIPADDR_DEL, &cfg, rtdrv_rg_algServerInLanAppsIpAddr_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_algServerInLanAppsIpAddr_del */


/* Function Name:
 *      rtk_rg_algApps_set
 * Description:
 *      Set ALG functions by bitmask.
 * Input:
 * Output:
 *      alg_app - [in]<tab>The bitmask setting for all ALG functions.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_ALG_SRV_IN_LAN_NO_IP - before turn on Server In Lan services, the server ip has to be added by rtk_rg_algServerInLanAppsIpAddr_add
 * Note:
 *      Althrough the bitmask list all ALG functions here, the implemented functions depend on romeDriver's version. Please refer user document.
 */
int32
rtk_rg_algApps_set(rtk_rg_alg_type_t alg_app)
{
    rtdrv_rg_algApps_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.alg_app, &alg_app, sizeof(rtk_rg_alg_type_t));
    SETSOCKOPT(RTDRV_RG_ALGAPPS_SET, &cfg, rtdrv_rg_algApps_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_algApps_set */


/* Function Name:
 *      rtk_rg_algApps_get
 * Description:
 *      Get ALG functions by bitmask.
 * Input:
 * Output:
 *       alg_app - [out]<tab>Return the bitmask setting for ALG functions.
 * Return:
 *      RT_ERR_RG_OK
 * Note:
 *      Althrough the bitmask list all ALG functions here, the implemented functions depend on romeDriver's version. Please refer user document.
 */
int32
rtk_rg_algApps_get(rtk_rg_alg_type_t *alg_app)
{
    rtdrv_rg_algApps_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == alg_app), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.alg_app, alg_app, sizeof(rtk_rg_alg_type_t));
    GETSOCKOPT(RTDRV_RG_ALGAPPS_GET, &cfg, rtdrv_rg_algApps_get_t, 1);
    osal_memcpy(alg_app, &cfg.alg_app, sizeof(rtk_rg_alg_type_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_algApps_get */

int32
rtk_rg_dmzHost_set(int wan_intf_idx, rtk_rg_dmzInfo_t *dmz_info)
{
    rtdrv_rg_dmzHost_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == dmz_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.dmz_info, dmz_info, sizeof(rtk_rg_dmzInfo_t));
    GETSOCKOPT(RTDRV_RG_DMZHOST_SET, &cfg, rtdrv_rg_dmzHost_set_t, 1);
    osal_memcpy(dmz_info, &cfg.dmz_info, sizeof(rtk_rg_dmzInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_dmzHost_set */
int32
rtk_rg_dmzHost_get(int wan_intf_idx, rtk_rg_dmzInfo_t *dmz_info)
{
    rtdrv_rg_dmzHost_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == dmz_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wan_intf_idx, &wan_intf_idx, sizeof(int));
    osal_memcpy(&cfg.dmz_info, dmz_info, sizeof(rtk_rg_dmzInfo_t));
    GETSOCKOPT(RTDRV_RG_DMZHOST_GET, &cfg, rtdrv_rg_dmzHost_get_t, 1);
    osal_memcpy(dmz_info, &cfg.dmz_info, sizeof(rtk_rg_dmzInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_dmzHost_get */

//QoS


//IGMP/MLD snooping

//IPv6
/* Function Name:
 *      rtk_rg_neighborEntry_add
 * Description:
 *      Add an Neighbor Entry into ASIC
 * Input:
 * Output:
 *      neighborEntry - [in]<tab>Fill rtk_rg_neighborEntry_t each fields for adding.
 *      neighbor_idx - [out]<tab>Return Neighbor entry index.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
 *      RT_ERR_RG_L2_ENTRY_NOT_FOUND - L2 entry not found.
 *      RT_ERR_RG_NEIGHBOR_FULL - neighbor table is full
 * Note:
 *      neighborEntry.l2Idx - this MAC entry index of this Neighbor entry.<nl>
 *      neighborEntry.matchRouteIdx - the routing entry idx that match the ip address.<nl>
 *      neighborEntry.interfaceId - the 64 bits interface identifier of this IPv6 address.<nl>
 *      neighborEntry.staticEntry - this entry is static ARP, and it will never age out.<nl>
 *      neighborEntry.valid - this entry is valid.<nl>
 *      If the neighborEntry.static_entry is set. The MAC entry must set static, too.
 */
int32
rtk_rg_neighborEntry_add(rtk_rg_neighborEntry_t *neighborEntry,int *neighbor_idx)
{
    rtdrv_rg_neighborEntry_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == neighborEntry), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == neighbor_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.neighborEntry, neighborEntry, sizeof(rtk_rg_neighborEntry_t));
    osal_memcpy(&cfg.neighbor_idx, neighbor_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_NEIGHBORENTRY_ADD, &cfg, rtdrv_rg_neighborEntry_add_t, 1);
    osal_memcpy(neighborEntry, &cfg.neighborEntry, sizeof(rtk_rg_neighborEntry_t));
    osal_memcpy(neighbor_idx, &cfg.neighbor_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_neighborEntry_add */

/* Function Name:
 *      rtk_rg_neighborEntry_del
 * Description:
 *      Delete an ASIC Neighbor Entry.
 * Input:
 * Output:
 *      neighbor_idx - [out]<tab>The Neighbor entry index for deleting. 
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NOT_INIT - system is not initiated. 
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE - the index out of range.
 *      RT_ERR_RG_ENTRY_NOT_EXIST - the index is not exist.
 * Note:
 *      None
 */
int32
rtk_rg_neighborEntry_del(int neighbor_idx)
{
    rtdrv_rg_neighborEntry_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.neighbor_idx, &neighbor_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_NEIGHBORENTRY_DEL, &cfg, rtdrv_rg_neighborEntry_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_neighborEntry_del */

/* Function Name:
 *      rtk_rg_neighborEntry_find
 * Description:
 *      Find the entire ASIC Neighbor table from index neighbor_valid_idx till valid one.
 * Input:
 * Output: 
 *      *neighborInfo - [in]<tab>An empty buffer for storing the Neighbor entry data structure.<nl>[out]<tab>The data structure of found Neighbor entry.
 *      *neighbor_valid_idx - [in]<tab>The index which find from.<nl>[out]<tab>The existing entry index.
 * Return:
 *      RT_ERR_RG_OK - Success 
 *      RT_ERR_RG_NOT_INIT - system is not initiated.
 *      RT_ERR_RG_NULL_POINTER - input buffer pointer is NULL.
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE - the index out of range
 *      RT_ERR_RG_NO_MORE_ENTRY_FOUND - no more exist entry is found. 
 *      RT_ERR_RG_NEIGHBOR_NOT_FOUND - the indicated neighbor ifid is not found.
 * Note:
 *      neighborInfo.arpEntry - The Neighbor entry data structure.<nl>
 *      neighborInfo.idleSecs - The Neighbor entry idle time in seconds.<nl>
 */
int32
rtk_rg_neighborEntry_find(rtk_rg_neighborInfo_t *neighborInfo,int *neighbor_valid_idx)
{
    rtdrv_rg_neighborEntry_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == neighborInfo), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == neighbor_valid_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.neighborInfo, neighborInfo, sizeof(rtk_rg_neighborInfo_t));
    osal_memcpy(&cfg.neighbor_valid_idx, neighbor_valid_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_NEIGHBORENTRY_FIND, &cfg, rtdrv_rg_neighborEntry_find_t, 1);
    osal_memcpy(neighborInfo, &cfg.neighborInfo, sizeof(rtk_rg_neighborInfo_t));
    osal_memcpy(neighbor_valid_idx, &cfg.neighbor_valid_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_neighborEntry_find */

/* Function Name:
 *      rtk_rg_softwareIdleTime_set
 * Description:
 *      setup software idle time.
 * Input:
 * Output:
 *	    type - [in]<tab>the type of idle time which be setup.
 *	    idleTime - [in]<tab>the value of idle time for selected type.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32 
rtk_rg_softwareIdleTime_set(rtk_rg_idle_time_type_t idleTimeType, int idleTime)
{
    rtdrv_rg_softwareIdleTime_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.idleTimeType, &idleTimeType, sizeof(rtk_rg_idle_time_type_t));
    osal_memcpy(&cfg.idleTime, &idleTime, sizeof(int));
    SETSOCKOPT(RTDRV_RG_SOFTWAREIDLETIME_SET, &cfg, rtdrv_rg_softwareIdleTime_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_softwareIdleTime_set */


/* Function Name:
 *      rtk_rg_softwareIdleTime_get
 * Description:
 *      get filter mode of MAC address.
 * Input:
 * Output:
 *	    type - [in]<tab>the type of idle time which be setup.
 *	    pIdleTime - [out]<tab>the value of idle time for selected type.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_NULL_POINTER - the input parameters may be NULL
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32 
rtk_rg_softwareIdleTime_get(rtk_rg_idle_time_type_t idleTimeType, int *pIdleTime)
{
    rtdrv_rg_softwareIdleTime_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pIdleTime), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.idleTimeType, &idleTimeType, sizeof(rtk_rg_idle_time_type_t));
    osal_memcpy(&cfg.pIdleTime, pIdleTime, sizeof(int));
    GETSOCKOPT(RTDRV_RG_SOFTWAREIDLETIME_GET, &cfg, rtdrv_rg_softwareIdleTime_get_t, 1);
    osal_memcpy(pIdleTime, &cfg.pIdleTime, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_softwareIdleTime_get */

/* Sub-module Name: Source Address Learning Limit and Action Functions */
/* Function Name:
 *      rtk_rg_accessWanLimit_set
 * Description:
 *      Set MAC access WAN limit, type, and action when using software learning.
 * Input:
 * Output:
 *      access_wan_info - [in]<tab>The information entered.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_ACCESSWAN_WLAN_NOT_ZERO
 *      RT_ERR_RG_ACCESSWAN_WLAN_CONFLICT
 * Note:
 *      access_wan_info.type - the limitation type.<nl>
 *      access_wan_info.data - the limitation data.<nl>
 *      access_wan_info.learningLimitNumber - the maximum number can access WAN.<nl>
 *      access_wan_info.action - what to do if a packet is exceed the learning limit.<nl>
 *      access_wan_info.wlan0_dev_mask - which wlan0 device should be limit.<nl>
 */
int32
rtk_rg_accessWanLimit_set(rtk_rg_accessWanLimitData_t access_wan_info)
{
    rtdrv_rg_accessWanLimit_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.access_wan_info, &access_wan_info, sizeof(rtk_rg_accessWanLimitData_t));
    SETSOCKOPT(RTDRV_RG_ACCESSWANLIMIT_SET, &cfg, rtdrv_rg_accessWanLimit_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_accessWanLimit_set */

/* Function Name:
 *      rtk_rg_accessWanLimit_get
 * Description:
 *      Get MAC access WAN limit, type, and action when using software learning.
 * Input:
 *      access_wan_info - [in]<tab>The information entered.
 * Output:
 *      access_wan_info - [out]<tab>The information contained.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      access_wan_info.learningCount - the number which can access WAN.<nl>
 *      When call this API, you should pass access_wan_info with type and data settle down.<nl>
 *      Port_mask don't need assign data, but category need to assign corresponding value.<nl>
 *      If port_limit_category is selected as type, the portmask used in category mode will be return in data.<nl>
 *      The corresponding type's limit, action, and count will be returned by pointer.<nl>
 */
int32
rtk_rg_accessWanLimit_get(rtk_rg_accessWanLimitData_t *access_wan_info)
{
    rtdrv_rg_accessWanLimit_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == access_wan_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.access_wan_info, access_wan_info, sizeof(rtk_rg_accessWanLimitData_t));
    GETSOCKOPT(RTDRV_RG_ACCESSWANLIMIT_GET, &cfg, rtdrv_rg_accessWanLimit_get_t, 1);
    osal_memcpy(access_wan_info, &cfg.access_wan_info, sizeof(rtk_rg_accessWanLimitData_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_accessWanLimit_get */

/* Function Name:
 *      rtk_rg_accessWanLimitCategory_set
 * Description:
 *      Set MAC to category when using software learning.
 * Input:
 * Output:
 *      macCategory_info - [in]<tab>The information entered.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      macCategory_info.category - the limitation category.<nl>
 *      macCategory_info.mac - the limitation MAC address.<nl>
 */
int32
rtk_rg_accessWanLimitCategory_set(rtk_rg_accessWanLimitCategory_t macCategory_info)
{
    rtdrv_rg_accessWanLimitCategory_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.macCategory_info, &macCategory_info, sizeof(rtk_rg_accessWanLimitCategory_t));
    SETSOCKOPT(RTDRV_RG_ACCESSWANLIMITCATEGORY_SET, &cfg, rtdrv_rg_accessWanLimitCategory_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_accessWanLimitCategory_set */

/* Function Name:
 *      rtk_rg_accessWanLimitCategory_get
 * Description:
 *      Get category of MAC address which using software learning.
 * Input:
 *      macCategory_info - [in]<tab>The information entered.
 * Output:
 *      macCategory_info - [out]<tab>The information contained.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      macCategory_info.category - the limitation category.<nl>
 *      macCategory_info.mac - the limitation MAC address.<nl>
 *      When call this API, you should pass macCategory_info with MAC address settle down.<nl>
 
 *      The corresponding category of MAC will be returned by pointer.<nl>
 */
int32
rtk_rg_accessWanLimitCategory_get(rtk_rg_accessWanLimitCategory_t *macCategory_info)
{
    rtdrv_rg_accessWanLimitCategory_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == macCategory_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.macCategory_info, macCategory_info, sizeof(rtk_rg_accessWanLimitCategory_t));
    GETSOCKOPT(RTDRV_RG_ACCESSWANLIMITCATEGORY_GET, &cfg, rtdrv_rg_accessWanLimitCategory_get_t, 1);
    osal_memcpy(macCategory_info, &cfg.macCategory_info, sizeof(rtk_rg_accessWanLimitCategory_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_accessWanLimitCategory_get */

/* Function Name:
 *      rtk_rg_softwareSourceAddrLearningLimit_set
 * Description:
 *      Set source address learning limit and action when using software learning.
 * Input:
 * Output:
 *      sa_learnLimit_info - [in]<tab>The information entered for the dedicated port.
 *      port_num - [in]<tab>The port number to set source address learning information.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      sa_learnLimit_info.learningLimitNumber - the maximum number can be learned in the port.<nl>
 *      sa_learnLimit_info.action - what to do if a packet is exceed the learning limit.<nl>
 */
int32
rtk_rg_softwareSourceAddrLearningLimit_set(rtk_rg_saLearningLimitInfo_t sa_learnLimit_info, rtk_rg_port_idx_t port_idx)
{
    rtdrv_rg_softwareSourceAddrLearningLimit_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.sa_learnLimit_info, &sa_learnLimit_info, sizeof(rtk_rg_saLearningLimitInfo_t));
    osal_memcpy(&cfg.port_idx, &port_idx, sizeof(rtk_rg_port_idx_t));
    SETSOCKOPT(RTDRV_RG_SOFTWARESOURCEADDRLEARNINGLIMIT_SET, &cfg, rtdrv_rg_softwareSourceAddrLearningLimit_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_softwareSourceAddrLearningLimit_set */

/* Function Name:
 *      rtk_rg_softwareSourceAddrLearningLimit_get
 * Description:
 *      Get source address learning limit and action when using software learning by port number.
 * Input:
 * Output:
 *      sa_learnLimit_info - [out]<tab>The information contained of the dedicated port.
 *      port_num - [in]<tab>The port number to get source address learning information.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      None
 */
int32
rtk_rg_softwareSourceAddrLearningLimit_get(rtk_rg_saLearningLimitInfo_t *sa_learnLimit_info, rtk_rg_port_idx_t port_idx)
{
    rtdrv_rg_softwareSourceAddrLearningLimit_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == sa_learnLimit_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.sa_learnLimit_info, sa_learnLimit_info, sizeof(rtk_rg_saLearningLimitInfo_t));
    osal_memcpy(&cfg.port_idx, &port_idx, sizeof(rtk_rg_port_idx_t));
    GETSOCKOPT(RTDRV_RG_SOFTWARESOURCEADDRLEARNINGLIMIT_GET, &cfg, rtdrv_rg_softwareSourceAddrLearningLimit_get_t, 1);
    osal_memcpy(sa_learnLimit_info, &cfg.sa_learnLimit_info, sizeof(rtk_rg_saLearningLimitInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_softwareSourceAddrLearningLimit_get */

/* Function Name:
 *      rtk_rg_wlanSoftwareSourceAddrLearningLimit_set
 * Description:
 *      Set wlan source address learning limit and action when using software learning.
 * Input:
 * Output:
 *      sa_learnLimit_info - [in]<tab>The information entered for the dedicated device.
 *      wlan_idx - [in]<tab>Wlan index
 *      dev_idx - [in]<tab>The device number to set source address learning information.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_ENTRY_NOT_EXIST
 *      RT_ERR_RG_ACCESSWAN_WLAN_CONFLICT
 * Note:
 *      None
 */
int32
rtk_rg_wlanSoftwareSourceAddrLearningLimit_set(rtk_rg_saLearningLimitInfo_t sa_learnLimit_info, int wlan_idx, int dev_idx)
{
    rtdrv_rg_wlanSoftwareSourceAddrLearningLimit_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.sa_learnLimit_info, &sa_learnLimit_info, sizeof(rtk_rg_saLearningLimitInfo_t));
    osal_memcpy(&cfg.wlan_idx, &wlan_idx, sizeof(int));
    osal_memcpy(&cfg.dev_idx, &dev_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_WLANSOFTWARESOURCEADDRLEARNINGLIMIT_SET, &cfg, rtdrv_rg_wlanSoftwareSourceAddrLearningLimit_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_wlanSoftwareSourceAddrLearningLimit_set */

/* Function Name:
 *      rtk_rg_softwareSourceAddrLearningLimit_get
 * Description:
 *      Get source address learning limit and action when using software learning by wlan device number.
 * Input:
 * Output:
 *      sa_learnLimit_info - [out]<tab>The information contained of the dedicated device.
 *      wlan_idx - [in]<tab>Wlan index
 *      dev_idx - [in]<tab>The device number to set source address learning information.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_ENTRY_NOT_EXIST
 *      RT_ERR_NULL_POINTER
 * Note:
 *      None
 */
int32
rtk_rg_wlanSoftwareSourceAddrLearningLimit_get(rtk_rg_saLearningLimitInfo_t *sa_learnLimit_info, int wlan_idx, int dev_idx)
{
    rtdrv_rg_wlanSoftwareSourceAddrLearningLimit_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == sa_learnLimit_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.sa_learnLimit_info, sa_learnLimit_info, sizeof(rtk_rg_saLearningLimitInfo_t));
    osal_memcpy(&cfg.wlan_idx, &wlan_idx, sizeof(int));
    osal_memcpy(&cfg.dev_idx, &dev_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_WLANSOFTWARESOURCEADDRLEARNINGLIMIT_GET, &cfg, rtdrv_rg_wlanSoftwareSourceAddrLearningLimit_get_t, 1);
    osal_memcpy(sa_learnLimit_info, &cfg.sa_learnLimit_info, sizeof(rtk_rg_saLearningLimitInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_wlanSoftwareSourceAddrLearningLimit_get */


/* Sub-module Name: DoS Functions */
/* Function Name:
 *      rtk_rg_dosPortMaskEnable_set
 * Description:
 *      Enable/Disable denial of service security port.
 * Input:
 * Output:
 *      dos_port_mask - [in]<tab>Security MAC port mask.
 * Return:
 *      RT_ERR_RG_OK - Success
 * Note:
 *      None
 */
int32
rtk_rg_dosPortMaskEnable_set(rtk_rg_mac_portmask_t dos_port_mask)
{
    rtdrv_rg_dosPortMaskEnable_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.dos_port_mask, &dos_port_mask, sizeof(rtk_rg_mac_portmask_t));
    SETSOCKOPT(RTDRV_RG_DOSPORTMASKENABLE_SET, &cfg, rtdrv_rg_dosPortMaskEnable_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_dosPortMaskEnable_set */

/* Function Name:
 *      rtk_rg_dosPortMaskEnable_get
 * Description:
 *      Get denial of service port security port state.
 * Input:
 * Output:
 *      *dos_port_mask - [out]<tab>Security MAC port mask.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NULL_POINTER - input port mask pointer is NULL.
 * Note:
 *      None
 */
int32
rtk_rg_dosPortMaskEnable_get(rtk_rg_mac_portmask_t *dos_port_mask)
{
    rtdrv_rg_dosPortMaskEnable_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == dos_port_mask), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.dos_port_mask, dos_port_mask, sizeof(rtk_rg_mac_portmask_t));
    GETSOCKOPT(RTDRV_RG_DOSPORTMASKENABLE_GET, &cfg, rtdrv_rg_dosPortMaskEnable_get_t, 1);
    osal_memcpy(dos_port_mask, &cfg.dos_port_mask, sizeof(rtk_rg_mac_portmask_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_dosPortMaskEnable_get */

/* Function Name:
 *      rtk_rg_dosType_set
 * Description:
 *      Set denial of service type function.
 * Input:
 * Output:
 *      dos_type - [in]<tab>Port security type.
 *      dos_enabled - [in]<tab>Port security function enabled/disabled.
 *      dos_action - [in]<tab>Port security action.
 * Return:
 *      RT_ERR_RG_OK - Success
 *              RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
 * Note:
 *      dos_type.RTK_RG_DAEQSA_DENY - packets while SMAC is the same as DMAC.
 *      dos_type.RTK_RG_LAND_DENY - packets while SIP is the same as DIP(support IPv4 only).
 *      dos_type.RTK_RG_BLAT_DENY - packets while the TCP/UDP SPORT is the same as DPORT destination TCP/UDP port.
 *      dos_type.RTK_RG_SYNFIN_DENY - TCP packets while SYN and FIN bits are set.
 *      dos_type.RTK_RG_XMA_DENY - TCP packets while sequence number is zero and FIN,URG,PSH bits are set.
 *      dos_type.RTK_RG_NULLSCAN_DENY - TCP packets while sequence number is zero and all contorl bits are zeros.
 *      dos_type.RTK_RG_SYN_SPORTL1024_DENY - TCP SYN packets with source port less than 1024.
 *      dos_type.RTK_RG_TCPHDR_MIN_CHECK - the length of a TCP header carried in an unfragmented IP(IPv4 and IPv6) datagram or the first fragment of a fragmented IP(IPv4) d
atagram is less than MIN_TCP_Header_Size(20 bytes).
 *      dos_type.RTK_RG_TCP_FRAG_OFF_MIN_CHECK - the Frangment_Offset=1 in anyfragment of a fragmented IP datagram carrying part of TCP data.
 *      dos_type.RTK_RG_ICMP_FRAG_PKTS_DENY - ICMPv4/ICMPv6 data unit carried in a fragmented IP datagram.
 *      dos_type.RTK_RG_POD_DENY - IP packet size > 65535 bytes, ((IP offset *8) + (IP length) <A1>V (IPIHL*4))>65535.
 *      dos_type.RTK_RG_UDPDOMB_DENY - UDP length > IP payload length.
 *      dos_type.RTK_RG_SYNWITHDATA_DENY - 1. IP length > IP header + TCP header length while SYN flag is set 1. 2. IP More Fragment and Offset > 0 while SYN is set to 1.
 *      dos_action.RTK_RG_DOS_ACTION_DROP - Drop packet while hit DoS criteria.
 *      dos_action.RTK_RG_DOS_ACTION_TRAP - Trap packet to CPU while hit DoS criteria.
 */
int32
rtk_rg_dosType_set(rtk_rg_dos_type_t dos_type,int dos_enabled,rtk_rg_dos_action_t dos_action)
{
    rtdrv_rg_dosType_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.dos_type, &dos_type, sizeof(rtk_rg_dos_type_t));
    osal_memcpy(&cfg.dos_enabled, &dos_enabled, sizeof(int));
    osal_memcpy(&cfg.dos_action, &dos_action, sizeof(rtk_rg_dos_action_t));
    SETSOCKOPT(RTDRV_RG_DOSTYPE_SET, &cfg, rtdrv_rg_dosType_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_dosType_set */

/* Function Name:
 *      rtk_rg_dosType_get
 * Description:
 *      Get denial of service type function.
 * Input:
 * Output:
 *      dos_type - [in]<tab>Port security type.
 *      *dos_enabled - [out]<tab>Port security function enabled/disabled.
 *      *dos_action - [out]<tab>Port security action.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NULL_POINTER - input port mask pointer is NULL.
 * Note:
 *      dos_type.RTK_RG_DAEQSA_DENY - packets while SMAC is the same as DMAC.
 *      dos_type.RTK_RG_LAND_DENY - packets while SIP is the same as DIP(support IPv4 only).
 *      dos_type.RTK_RG_BLAT_DENY - packets while the TCP/UDP SPORT is the same as DPORT destination TCP/UDP port.
 *      dos_type.RTK_RG_SYNFIN_DENY - TCP packets while SYN and FIN bits are set.
 *      dos_type.RTK_RG_XMA_DENY - TCP packets while sequence number is zero and FIN,URG,PSH bits are set.
 *      dos_type.RTK_RG_NULLSCAN_DENY - TCP packets while sequence number is zero and all contorl bits are zeros.
 *      dos_type.RTK_RG_SYN_SPORTL1024_DENY - TCP SYN packets with source port less than 1024.
 *      dos_type.RTK_RG_TCPHDR_MIN_CHECK - the length of a TCP header carried in an unfragmented IP(IPv4 and IPv6) datagram or the first fragment of a fragmented IP(IPv4) d
atagram is less than MIN_TCP_Header_Size(20 bytes).
 *      dos_type.RTK_RG_TCP_FRAG_OFF_MIN_CHECK - the Frangment_Offset=1 in anyfragment of a fragmented IP datagram carrying part of TCP data.
 *      dos_type.RTK_RG_ICMP_FRAG_PKTS_DENY - ICMPv4/ICMPv6 data unit carried in a fragmented IP datagram.
 *      dos_type.RTK_RG_POD_DENY - IP packet size > 65535 bytes, ((IP offset *8) + (IP length) <A1>V (IPIHL*4))>65535.
 *      dos_type.RTK_RG_UDPDOMB_DENY - UDP length > IP payload length.
 *      dos_type.RTK_RG_SYNWITHDATA_DENY - 1. IP length > IP header + TCP header length while SYN flag is set 1. 2. IP More Fragment and Offset > 0 while SYN is set to 1.
 *      dos_action.RTK_RG_DOS_ACTION_DROP - Drop packet while hit DoS criteria.
 *      dos_action.RTK_RG_DOS_ACTION_TRAP - Trap packet to CPU while hit DoS criteria.
 */
int32
rtk_rg_dosType_get(rtk_rg_dos_type_t dos_type,int *dos_enabled,rtk_rg_dos_action_t *dos_action)
{
    rtdrv_rg_dosType_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == dos_enabled), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == dos_action), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.dos_type, &dos_type, sizeof(rtk_rg_dos_type_t));
    osal_memcpy(&cfg.dos_enabled, dos_enabled, sizeof(int));
    osal_memcpy(&cfg.dos_action, dos_action, sizeof(rtk_rg_dos_action_t));
    GETSOCKOPT(RTDRV_RG_DOSTYPE_GET, &cfg, rtdrv_rg_dosType_get_t, 1);
    osal_memcpy(dos_enabled, &cfg.dos_enabled, sizeof(int));
    osal_memcpy(dos_action, &cfg.dos_action, sizeof(rtk_rg_dos_action_t));
	
	return RT_ERR_OK;
}	/* end of rtk_rg_dosType_get */

/* Function Name:
 *      rtk_rg_dosFloodType_set
 * Description:
 *      Set denial of service flooding attack protection function.
 * Input:
 * Output:
 *      dos_type - [in]<tab>Port security type.
 *      dos_enabled - [in]<tab>Port security function enabled/disabled.
 *      dos_action - [in]<tab>Port security action.
 *              dos_threshold - [in]<tab>System-based SYN/FIN/ICMP flood threshold.
 * Return:
 *      RT_ERR_RG_OK - Success
 *              RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
 * Note:
 *      dos_type.RTK_RG_SYNFLOOD_DENY - Receiving TCP SYN packet number is over threshold.
 *      dos_type.RTK_RG_FINFLOOD_DENY - Receiving TCP FIN packet number is over threshold.
 *      dos_type.RTK_RG_ICMPFLOOD_DENY - Receiving ICMP packet number is over threshold.
 *      dos_action.RTK_RG_DOS_ACTION_DROP - Drop packet while hit DoS criteria.
 *      dos_action.RTK_RG_DOS_ACTION_TRAP - Trap packet to CPU while hit DoS criteria.
 *              dos_threshold - Allowable SYN/FIN/ICMP packet frame rate in 1k packets/sec.
 */
int32
rtk_rg_dosFloodType_set(rtk_rg_dos_type_t dos_type,int dos_enabled,rtk_rg_dos_action_t dos_action,int dos_threshold)
{
    rtdrv_rg_dosFloodType_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.dos_type, &dos_type, sizeof(rtk_rg_dos_type_t));
    osal_memcpy(&cfg.dos_enabled, &dos_enabled, sizeof(int));
    osal_memcpy(&cfg.dos_action, &dos_action, sizeof(rtk_rg_dos_action_t));
    osal_memcpy(&cfg.dos_threshold, &dos_threshold, sizeof(int));
    SETSOCKOPT(RTDRV_RG_DOSFLOODTYPE_SET, &cfg, rtdrv_rg_dosFloodType_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_dosFloodType_set */


/* Function Name:
 *      rtk_rg_dosFloodType_get
 * Description:
 *      Get denial of service port security setting.
 * Input:
 * Output:
 *      dos_type - [in]<tab>Port security type.
 *      *dos_enabled - [out]<tab>Port security function enabled/disabled.
 *      *dos_action - [out]<tab>Port security action.
 *              *dos_threshold - [out]<tab>System-based SYN/FIN/ICMP flood threshold.
 * Return:
 *      RT_ERR_RG_OK - Success
 *              RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
 *      RT_ERR_RG_NULL_POINTER - input port mask pointer is NULL.
 * Note:
 *      dos_type.RTK_RG_SYNFLOOD_DENY - Receiving TCP SYN packet number is over threshold.
 *      dos_type.RTK_RG_FINFLOOD_DENY - Receiving TCP FIN packet number is over threshold.
 *      dos_type.RTK_RG_ICMPFLOOD_DENY - Receiving ICMP packet number is over threshold.
 *      dos_action.RTK_RG_DOS_ACTION_DROP - Drop packet while hit DoS criteria.
 *      dos_action.RTK_RG_DOS_ACTION_TRAP - Trap packet to CPU while hit DoS criteria.
 *              dos_threshold - Allowable SYN/FIN/ICMP packet frame rate in 1k packets/sec.
 */
int32
rtk_rg_dosFloodType_get(rtk_rg_dos_type_t dos_type,int *dos_enabled,rtk_rg_dos_action_t *dos_action,int *dos_threshold)
{
    rtdrv_rg_dosFloodType_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == dos_enabled), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == dos_action), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == dos_threshold), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.dos_type, &dos_type, sizeof(rtk_rg_dos_type_t));
    osal_memcpy(&cfg.dos_enabled, dos_enabled, sizeof(int));
    osal_memcpy(&cfg.dos_action, dos_action, sizeof(rtk_rg_dos_action_t));
    osal_memcpy(&cfg.dos_threshold, dos_threshold, sizeof(int));
    GETSOCKOPT(RTDRV_RG_DOSFLOODTYPE_GET, &cfg, rtdrv_rg_dosFloodType_get_t, 1);
    osal_memcpy(dos_enabled, &cfg.dos_enabled, sizeof(int));
    osal_memcpy(dos_action, &cfg.dos_action, sizeof(rtk_rg_dos_action_t));
    osal_memcpy(dos_threshold, &cfg.dos_threshold, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_dosFloodType_get */


/* Function Name:
 *      rtk_rg_portMirror_set
 * Description:
 *      Enable portMirror for monitor Tx/Rx packet.
 * Input:
 * Output:
 *      rtk_rg_portMirrorInfo_t.monitorPort - [in]<tab>assign monitor port
 *      rtk_rg_portMirrorInfo_t.enabledPortMask - [in]<tab>enabled/disabled each port for mirror or not
 *      rtk_rg_portMirrorInfo_t.direct - [in]<tab>assign each port mirror tx/rx packet
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
 *      RT_ERR_RG_NULL_POINTER - input port mask pointer is NULL.
 * Note:
 *      None
 */
int32
rtk_rg_portMirror_set(rtk_rg_portMirrorInfo_t portMirrorInfo)
{
    rtdrv_rg_portMirror_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.portMirrorInfo, &portMirrorInfo, sizeof(rtk_rg_portMirrorInfo_t));
    SETSOCKOPT(RTDRV_RG_PORTMIRROR_SET, &cfg, rtdrv_rg_portMirror_set_t, 1);
    return RT_ERR_OK;
}   /* end of rtk_rg_portMirror_set */



/* Function Name:
 *      rtk_rg_portMirror_get
 * Description:
 *      Get portMirror for monitor Tx/Rx packet information.
 * Input:
 * Output:
 *      rtk_rg_portMirrorInfo_t.monitorPort - [out]<tab>assigned monitor port
 *      rtk_rg_portMirrorInfo_t.enabledPortMask - [out]<tab>each port for mirror enabled/disabled information
 *      rtk_rg_portMirrorInfo_t.direct - [out]<tab>port mirror tx/rx packet information.
 * Return:
 *      RT_ERR_RG_OK - Success
 *      RT_ERR_RG_NULL_POINTER - input port mask pointer is NULL.
 * Note:
 *      None
 */
int32
rtk_rg_portMirror_get(rtk_rg_portMirrorInfo_t *portMirrorInfo)
{
    rtdrv_rg_portMirror_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == portMirrorInfo), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.portMirrorInfo, portMirrorInfo, sizeof(rtk_rg_portMirrorInfo_t));
    GETSOCKOPT(RTDRV_RG_PORTMIRROR_GET, &cfg, rtdrv_rg_portMirror_get_t, 1);
    osal_memcpy(portMirrorInfo, &cfg.portMirrorInfo, sizeof(rtk_rg_portMirrorInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_portMirror_get */

/* Function Name:
 *      rtk_rg_portMirror_clear
 * Description:
 *      clear port mirror setting
 * Input:
 * Output:
 * Return:
 *      RT_ERR_RG_OK - Success
 * Note:
 *      None
 */
int32
rtk_rg_portMirror_clear(void)
{
    //rtdrv_rg_portMirror_clear_t cfg;

    /* function body */
    //SETSOCKOPT(RTDRV_RG_PORTMIRROR_CLEAR, &cfg, rtdrv_rg_portMirror_clear_t, 1);
    SETSOCKOPT(RTDRV_RG_PORTMIRROR_CLEAR, NULL, NULL, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_portMirror_clear */

/* Function Name:
 *      rtk_rg_portEgrBandwidthCtrlRate_set
 * Description:
 *      Set egress rate limitation for physical port.
 * Input:
 * Output:
 *      port - [in]<tab>assigned rate limit port
 *      rate - [in]<tab>assigned rate, unit Kbps
 * Return:
 *      RT_ERR_RG_OK - Success
 * Note:
 *      None
 */
int32
rtk_rg_portEgrBandwidthCtrlRate_set(rtk_rg_mac_port_idx_t port, uint32 rate)
{
    rtdrv_rg_portEgrBandwidthCtrlRate_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
    osal_memcpy(&cfg.rate, &rate, sizeof(uint32));
    SETSOCKOPT(RTDRV_RG_PORTEGRBANDWIDTHCTRLRATE_SET, &cfg, rtdrv_rg_portEgrBandwidthCtrlRate_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_portEgrBandwidthCtrlRate_set */

/* Function Name:
 *      rtk_rg_portIgrBandwidthCtrlRate_set
 * Description:
 *      Set ingress rate limitation for physical port.
 * Input:
 * Output:
 *      port - [in]<tab>assigned rate limit port
 *      rate - [in]<tab>assigned rate, unit Kbps
 * Return:
 *      RT_ERR_RG_OK - Success
 * Note:
 *      None
 */
int32
rtk_rg_portIgrBandwidthCtrlRate_set(rtk_rg_mac_port_idx_t port, uint32 rate)
{
    rtdrv_rg_portIgrBandwidthCtrlRate_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
    osal_memcpy(&cfg.rate, &rate, sizeof(uint32));
    SETSOCKOPT(RTDRV_RG_PORTIGRBANDWIDTHCTRLRATE_SET, &cfg, rtdrv_rg_portIgrBandwidthCtrlRate_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_portIgrBandwidthCtrlRate_set */

/* Function Name:
 *              rtk_rg_portEgrBandwidthCtrlRate_get
 * Description:
 *              Get egress rate limitation of physical port.
 * Input:
 * Output:
 *              port - [in]<tab>assigned rate limit port
 *              *rate - [out]<tab>assigned rate, unit Kbps
 * Return:
 *              RT_ERR_RG_OK - Success
 *              RT_ERR_RG_NULL_POINTER - input rate pointer is NULL.
 * Note:
 *              None
 */
int32
rtk_rg_portEgrBandwidthCtrlRate_get(rtk_rg_mac_port_idx_t port, uint32 *rate)
{
    rtdrv_rg_portEgrBandwidthCtrlRate_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == rate), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
    osal_memcpy(&cfg.rate, rate, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_PORTEGRBANDWIDTHCTRLRATE_GET, &cfg, rtdrv_rg_portEgrBandwidthCtrlRate_get_t, 1);
    osal_memcpy(rate, &cfg.rate, sizeof(uint32));

    return RT_ERR_OK;
}   /* end of rtk_rg_portEgrBandwidthCtrlRate_get */

/* Function Name:
 *              rtk_rg_portIgrBandwidthCtrlRate_get
 * Description:
 *              Get ingress rate limitation of physical port.
 * Input:
 * Output:
 *              port - [in]<tab>assigned rate limit port
 *              *rate - [out]<tab>assigned rate, unit Kbps
 * Return:
 *              RT_ERR_RG_OK - Success
 *              RT_ERR_RG_NULL_POINTER - input rate pointer is NULL.
 * Note:
 *              None
 */
int32
rtk_rg_portIgrBandwidthCtrlRate_get(rtk_rg_mac_port_idx_t port, uint32 *rate)
{
    rtdrv_rg_portIgrBandwidthCtrlRate_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == rate), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
    osal_memcpy(&cfg.rate, rate, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_PORTIGRBANDWIDTHCTRLRATE_GET, &cfg, rtdrv_rg_portIgrBandwidthCtrlRate_get_t, 1);
    osal_memcpy(rate, &cfg.rate, sizeof(uint32));

    return RT_ERR_OK;
}   /* end of rtk_rg_portIgrBandwidthCtrlRate_get */

/* Function Name:
 *       rtk_rg_phyPortForceAbility_set
 * Description:
 *       Force Set physical port status & ability.
 * Input:
 * Output:
 *       port - [in]<tab>assigned physical port
 *       ability.force_disable_phy - [in]<tab> ENABLED:force link-down phyPort
 *       ability.valid - [in]<tab> Enable/Disable Force assigned phyPort ability
 *       ability.speed - [in]<tab> Assign linkup speed
 *       ability.duplex - [in]<tab> Assign half duplex or full duplex
 *       ability.flowCtrl - [in]<tab> enable/disable flow control
 * Return:
 *       RT_ERR_RG_OK - Success
 * Note:
 *       None
 */
int32
rtk_rg_phyPortForceAbility_set(rtk_rg_mac_port_idx_t port, rtk_rg_phyPortAbilityInfo_t ability)
{
    rtdrv_rg_phyPortForceAbility_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
    osal_memcpy(&cfg.ability, &ability, sizeof(rtk_rg_phyPortAbilityInfo_t));
    SETSOCKOPT(RTDRV_RG_PHYPORTFORCEABILITY_SET, &cfg, rtdrv_rg_phyPortForceAbility_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_phyPortForceAbility_set */

 /* Function Name:
  *      rtk_rg_phyPortForceAbility_get
  * Description:
  *      Get physical port status & ability.
  * Input:
  * Output:
  *      port - [in]<tab>assigned physical port
  *      ability.force_disable_phy - [out]<tab> ENABLED:force link-down phyPort
  *      ability.valid - [out]<tab> Enabled/Disabled Force assigned phyPort ability
  *      ability.speed - [out]<tab> Assigned linkup speed
  *      ability.duplex - [out]<tab> Assigned half duplex or full duplex
  *      ability.flowCtrl - [out]<tab> enabled/disabled flow control
  * Return:
  *      RT_ERR_RG_OK - Success
  *      RT_ERR_RG_NULL_POINTER - input ability pointer is NULL.
  * Note:
  *      None
  */
int32
rtk_rg_phyPortForceAbility_get(rtk_rg_mac_port_idx_t port, rtk_rg_phyPortAbilityInfo_t *ability)
{
    rtdrv_rg_phyPortForceAbility_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == ability), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
    osal_memcpy(&cfg.ability, ability, sizeof(rtk_rg_phyPortAbilityInfo_t));
    GETSOCKOPT(RTDRV_RG_PHYPORTFORCEABILITY_GET, &cfg, rtdrv_rg_phyPortForceAbility_get_t, 1);
    osal_memcpy(ability, &cfg.ability, sizeof(rtk_rg_phyPortAbilityInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_phyPortForceAbility_get */

/* Function Name:
 *              rtk_rg_cpuPortForceTrafficCtrl_set
 * Description:
 *              Set cpu port flow control status.
 * Input:
 * Output:
 *              tx_fc_state - [in]<tab>enable/disable tx flowcontrol enable.
 *              rx_fc_state - [in]<tab>enable/disable rx flowcontrol enable.
 * Return:
 *              RT_ERR_RG_OK - Success
 * Note:
 *              None
 */

int32
rtk_rg_cpuPortForceTrafficCtrl_set(rtk_rg_enable_t tx_fc_state, rtk_rg_enable_t rx_fc_state)
{
    rtdrv_rg_cpuPortForceTrafficCtrl_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.tx_fc_state, &tx_fc_state, sizeof(rtk_rg_enable_t));
    osal_memcpy(&cfg.rx_fc_state, &rx_fc_state, sizeof(rtk_rg_enable_t));
    SETSOCKOPT(RTDRV_RG_CPUPORTFORCETRAFFICCTRL_SET, &cfg, rtdrv_rg_cpuPortForceTrafficCtrl_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_cpuPortForceTrafficCtrl_set */

/* Function Name:
 *              rtk_rg_cpuPortForceTrafficCtrl_get
 * Description:
 *              Get cpu port flow control status.
 * Input:
 * Output:
 *              pTx_fc_state - [in]<tab>tx flowcontrol enable or not.
 *              pRx_fc_state - [in]<tab>rx flowcontrol enable or not.
 * Return:
 *              RT_ERR_RG_OK - Success
 *              RT_ERR_RG_NULL_POINTER - input ability pointer is NULL.
 * Note:
 *              None
 */
int32
rtk_rg_cpuPortForceTrafficCtrl_get(rtk_rg_enable_t *pTx_fc_state,       rtk_rg_enable_t *pRx_fc_state)
{
    rtdrv_rg_cpuPortForceTrafficCtrl_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pTx_fc_state), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pRx_fc_state), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pTx_fc_state, pTx_fc_state, sizeof(rtk_rg_enable_t));
    osal_memcpy(&cfg.pRx_fc_state, pRx_fc_state, sizeof(rtk_rg_enable_t));
    GETSOCKOPT(RTDRV_RG_CPUPORTFORCETRAFFICCTRL_GET, &cfg, rtdrv_rg_cpuPortForceTrafficCtrl_get_t, 1);
    osal_memcpy(pTx_fc_state, &cfg.pTx_fc_state, sizeof(rtk_rg_enable_t));
    osal_memcpy(pRx_fc_state, &cfg.pRx_fc_state, sizeof(rtk_rg_enable_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_cpuPortForceTrafficCtrl_get */


/* Function Name:
*              rtk_rg_portMibInfo_get
* Description:
*              Get physical port mib data.
* Input:
* Output:
*              port - [in]<tab>assigned physical port
*              *mibInfo - [out]<tab> Enabled/Disabled Force assigned phyPort ability
* Return:
*              RT_ERR_RG_OK - Success
*              RT_ERR_RG_NULL_POINTER - input ability pointer is NULL.
*      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
* Note:
*              mibInfo->ifInOctets - The total number of octets received on the interface<A1>A including framing characters.
*              mibInfo->ifInUcastPkts - The number of packets, delivered by this sub-layer to a higher (sub-)layer, which were not addressed to a multicast or broadcast address at this sub-layer.
*              mibInfo->ifInMulticastPkts - The number of packets<A1>A delivered by this sub-layer to a higher (sub-)layer<A1>A which were addressed to a multicast address at this sub-layer. For a MAC layer protocol<A1>A this includes both Group and Functional addresses.
*              mibInfo->ifInBroadcastPkts - The number of packets<A1>A delivered by this sub-layer to a higher (sub-)layer<A1>A which were addressed to a broadcast address at this sub-layer.
*              mibInfo->ifInDiscards - The number of inbound packets which were chosen to be discarded even though no errors had been detected to prevent their being transmitted. One possible reason for discarding such a packet could be to free up buffer space.
*              mibInfo->ifOutOctets - The total number of octets transmitted out of the interface<A1>A including framing characters.
*              mibInfo->ifOutDiscards - The number of outbound packets which were chosen to be discarded even though no errors had been detected to prevent their being transmitted. One possible reason for discarding such a packet could be to free up buffer space.
*              mibInfo->ifOutUcastPkts - The total number of packets that higher-level protocols requested be transmitted, and which were not addressed to a multicast or broadcast address at this sub-layer, including those that were discarded or not sent.
*              mibInfo->ifOutMulticastPkts - The total number of packets that higher-level protocols requested be transmitted, and which were addressed to a multicast address at this sub-layer, including those that were discarded or not sent.
*              mibInfo->ifOutBrocastPkts - The total number of packets that higher-level protocols requested be transmitted, and which were addressed to a broadcast address at this sub-layer, including those that were discarded or not sent.
*              mibInfo->dot1dBasePortDelayExceededDiscards - The number of transmitted frames that were discarded due to the maximum bridge transit delay being exceeded.
*              mibInfo->dot1dTpPortInDiscards - Count of valid frames received which were discarded (i.e., filtered) by the Forwarding Process.
*              mibInfo->dot1dTpHcPortInDiscards - The total number of Forwarding Database entries<A1>A which have been or would have been learnt<A1>A but have been discarded due to lack of space to store them in the Forwarding Database.
*              mibInfo->dot3InPauseFrames - A count of MAC Control frames received on this interface with an opcode indicating the PAUSE operation.
*              mibInfo->dot3OutPauseFrames - A count of MAC control frames transmitted on this interface with an opcode indicating the PAUSE operation.
*              mibInfo->dot3StatsAligmentErrors - The total number of bits of a received frame is not divisible by eight.
*              mibInfo->dot3StatsFCSErrors - The number of received frame that frame check sequence error.
*              mibInfo->dot3StatsSingleCollisionFrames - A count of frames that are involved in a single collision<A1>A and are subsequently transmitted successfully.
*              mibInfo->dot3StatsMultipleCollisionFrames - A count of frames that are involved in more than one collision and are subsequently transmitted successfully.
*              mibInfo->dot3StatsDeferredTransmissions - A count of frames for which the first transmission attempt on a particular interface is delayed because the medium is busy.
*              mibInfo->dot3StatsLateCollisions - The number of times that a collision is detected on a particular interface later than one slotTime into the transmission of a packet.
*              mibInfo->dot3StatsExcessiveCollisions - A count of frames for which transmission on a particular interface fails due to excessive collisions.
*              mibInfo->dot3StatsFrameTooLongs -  RX_etherStatsOversizePkt -  RX_etherStatsPkts1519toMaxOctets.
*			mibInfo->dot3StatsSymbolErrors - Number of data symbol errors.
* 			 mibInfo->dot3ControlInUnknownOpcodes - A count of MAC Control frames received on this interface that contain an opcode that is not supported by this device.
* 			 mibInfo->etherStatsDropEvents - Number of received packets dropped due to lack of resource.
* 			 mibInfo->etherStatsOctets - ifInOctets(bad+good) + ifOutOctets(bad+good).
* 			 mibInfo->etherStatsBcastPkts - The total number of packets that were directed to the broadcast address. Note that this does not include multicast packets.
* 			 mibInfo->etherStatsMcastPkts - The total number of packets that were directed to a multicast address. Note that this number does not include packets directed to the broadcast address.
* 			 mibInfo->etherStatsUndersizePkts - The total number of packets that were less than 64 octets long (excluding framing bits, but including FCS octets) and were otherwise well formed.
* 			 mibInfo->etherStatsOversizePkts - Number of valid packets whose size are more than 1518 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsFragments - Number of invalid (FCS error / alignment error) packets whose size are less than 64 bytes<A1>A including MAC header and FCS <A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsJabbers - Number of invalid (FCS error / alignment error) packets whose size are more than 1518 bytes<A1>A including MAC header and FCS <A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsCollisions - The best estimate of the total number of collisions on this Ethernet segment.
* 			 mibInfo->etherStatsCRCAlignErrors - The total number of packets that had a length (excluding framing bits, but including FCS octets) of between 64 and 1518 octets, inclusive, but had either a bad Frame Check Sequence (FCS) with an integral number of octets (FCS Error) or a bad FCS with a non-integral number of octets (Alignment Error).
* 			 mibInfo->etherStatsPkts64Octets - Number of all packets whose size are exactly 64 bytes<A1>Aincluding MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsPkts65to127Octets - Number of all packets whose size are between 65 ~ 127 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsPkts128to255Octets - Number of all packets whose size are between 128 ~ 255 bytes<A1>A including MAC header and FCS<A1>A excluding preambl e and SFD.
* 			 mibInfo->etherStatsPkts256to511Octets - Number of all packets whose size are between 256 ~ 511 bytes<A1>A including FCS<A1>A excluding MAC header<A1>A preamble and SFD.
* 			 mibInfo->etherStatsPkts512to1023Octets - Number of all packets whose size are between 512 ~ 1023 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsPkts1024to1518Octets - Number of all packets whose size are between 1024 ~ 1518 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxOctets - ifOutOctets.
* 			 mibInfo->etherStatsTxUndersizePkts - The total number of packets transmitted that were less than 64 octets long (excluding framing bits, but including FCS octets) and were otherwise well formed.
* 			 mibInfo->etherStatsTxOversizePkts - Number of transmitted valid packets whose size are more than 1518 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxPkts64Octets - Number of all transmitted packets whose size are exactly 64 bytes<A1>Aincluding MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxPkts65to127Octets - Number of all transmitted packets whose size are between 65 ~ 127 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxPkts128to255Octets - Number of all transmitted packets whose size are between 128 ~ 255 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxPkts256to511Octets - Number of all transmitted packets whose size are between 256 ~ 511 bytes<A1>A including FCS<A1>A excluding MAC header<A1>A preamble and SFD.
* 			 mibInfo->etherStatsTxPkts512to1023Octets - Number of all transmitted packets whose size are between 512 ~ 1023 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxPkts1024to1518Octets - - Number of all transmitted packets whose size are between 1024 ~ 1518 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxPkts1519toMaxOctets - The total number of packets (including bad packets) transmitted that were between 1519 and Max octets in length inclusive (excluding framing bits but including FCS octets).
* 			 mibInfo->etherStatsTxBcastPkts - The total number of good packets transmitted that were directed to the broadcast address. Note that this does not include multicast packets.
* 			 mibInfo->etherStatsTxMcastPkts - The total number of good packets transmitted that were directed to a multicast address. Note that this number does not include packets directed to the broadcast address.
* 			 mibInfo->etherStatsTxFragments - Number of transmitted invalid (FCS error / alignment error) packets whose size are less than 64 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxJabbers - Number of transmitted invalid (FCS error / alignment error) packets whose size are more than 1518 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsTxCRCAlignErrors - The total number of packets transmitted that had a length (excluding framing bits, but including FCS octets) of between 64 and 1518 octets, inclusive, but had either a bad Frame Check Sequence (FCS) with an integral number of octets (FCS Error) or a bad FCS with a non-integral number of octets (Alignment Error).
* 			 mibInfo->etherStatsRxUndersizePkts - The total number of packets received that were less than 64 octets long (excluding framing bits, but including FCS octets) and were otherwise well formed.
* 			 mibInfo->etherStatsRxUndersizeDropPkts - The total number of packets received that were less than 64 octets long (excluding framing bits, but including FCS octets) and were otherwise well formed.
* 			 mibInfo->etherStatsRxOversizePkts - Number of received valid packets whose size are more than 1518 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsRxPkts64Octets - Number of all received packets whose size are exactly 64 bytes<A1>Aincluding MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsRxPkts65to127Octets - Number of all received packets whose size are between 65 ~ 127 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsRxPkts128to255Octets - Number of all received packets whose size are between 128 ~ 255 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsRxPkts256to511Octets - Number of all received packets whose size are between 256 ~ 511 bytes<A1>A including FCS<A1>A excluding MAC header<A1>A preamble and SFD.
* 			 mibInfo->etherStatsRxPkts512to1023Octets - Number of all received packets whose size are between 512 ~ 1023 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsRxPkts1024to1518Octets - Number of all received packets whose size are between 1024 ~ 1518 bytes<A1>A including MAC header and FCS<A1>A excluding preamble and SFD.
* 			 mibInfo->etherStatsRxPkts1519toMaxOctets - The total number of packets (including bad packets) received that were between 1519 and Max octets in length inclusive (excluding framing bits but including FCS octets).
* 			 mibInfo->inOampduPkts - Number of received OAMPDUs.
* 			 mibInfo->outOampduPkts - A count of OAMPDUs transmitted on this interface.
*/
 int32
 rtk_rg_portMibInfo_get(rtk_rg_mac_port_idx_t port, rtk_rg_port_mib_info_t *mibInfo)
 {
	 rtdrv_rg_portMibInfo_get_t cfg;
 
	 /* parameter check */
	 RT_PARAM_CHK((NULL == mibInfo), RT_ERR_NULL_POINTER);
 
	 /* function body */
	 osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
	 osal_memcpy(&cfg.mibInfo, mibInfo, sizeof(rtk_rg_port_mib_info_t));
	 GETSOCKOPT(RTDRV_RG_PORTMIBINFO_GET, &cfg, rtdrv_rg_portMibInfo_get_t, 1);
	 osal_memcpy(mibInfo, &cfg.mibInfo, sizeof(rtk_rg_port_mib_info_t));
 
	 return RT_ERR_OK;
 }	 /* end of rtk_rg_portMibInfo_get */
 
/* Function Name:
* 			 rtk_rg_portMibInfo_clear
* Description:
* 			 Clear physical port mib data.
* Input:
* Output:
* 			 port - [in]<tab>assigned physical port
* Return:
* 			 RT_ERR_RG_OK - Success
* 	 RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
* Note:
* 			 None
*/
int32
rtk_rg_portMibInfo_clear(rtk_rg_mac_port_idx_t port)
{
	 rtdrv_rg_portMibInfo_clear_t cfg;
 
	 /* function body */
	 osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
	 SETSOCKOPT(RTDRV_RG_PORTMIBINFO_CLEAR, &cfg, rtdrv_rg_portMibInfo_clear_t, 1);
 
	 return RT_ERR_OK;
}	 /* end of rtk_rg_portMibInfo_clear */

/* Function Name:
 *		rtk_rg_portIsolation_set
 * Description:
 *		Set Port isolation portmask.
 * Input:
 *		isolationSetting.port - [in]<tab>assigned port index
 *		isolationSetting.portmask - [in]<tab>assigned isolation port mask
 * Output:
 * Return:
 *		RT_ERR_RG_OK - Success 
 *      	RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
 * Note:
 *		None
 */ 
int32
rtk_rg_portIsolation_set(rtk_rg_port_isolation_t isolationSetting)
{
    rtdrv_rg_portIsolation_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.isolationSetting, &isolationSetting, sizeof(rtk_rg_port_isolation_t));
    SETSOCKOPT(RTDRV_RG_PORTISOLATION_SET, &cfg, rtdrv_rg_portIsolation_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_portIsolation_set */

/* Function Name:
 *		rtk_rg_portIsolation_get
 * Description:
 *		Get Port isolation portmask.
 * Input:
 *		isolationSetting.port - [in]<tab>assigned port index
 * Output: 
 *		isolationSetting.portmask - [in]<tab>assigned isolation port mask for the port
 * Return:
 *		RT_ERR_RG_OK - Success 
 * 	 	RT_ERR_RG_NULL_POINTER - parameter is null. <nl>
 *      	RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information.
 * Note:
 *		None
 */ 
int32
rtk_rg_portIsolation_get(rtk_rg_port_isolation_t *isolationSetting)
{
    rtdrv_rg_portIsolation_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == isolationSetting), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.isolationSetting, isolationSetting, sizeof(rtk_rg_port_isolation_t));
    GETSOCKOPT(RTDRV_RG_PORTISOLATION_GET, &cfg, rtdrv_rg_portIsolation_get_t, 1);
    osal_memcpy(isolationSetting, &cfg.isolationSetting, sizeof(rtk_rg_port_isolation_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_portIsolation_get */

/* Function Name:
* 	 rtk_rg_stormControl_add
* Description:
* 	Add stormControl.
* Input:
* Output:
* 	 stormInfo.valid - [in]<tab>enable stormCtrol
* 	 astormInfo.port  - [in]<tab> assigned port
* 	 stormInfo.stormType - [in]<tab> assigned stormType
* 	 stormInfo.meterIdx - [in]<tab> assigned rate by shareMeter
* Return:
* 	 RT_ERR_RG_OK - Success<nl>
* 	 RT_ERR_RG_NULL_POINTER - parameter is null. <nl>
* 	 RT_ERR_RG_INVALID_PARAM - parameters value is out of range.<nl>
* 	 RT_ERR_RG_STORMCONTROL_ENTRY_HAS_BEEN_SET - same stormInfo has been set before .<nl>
* 	 RT_ERR_RG_STORMCONTROL_TYPE_FULL - add stormType full(ASIC support at most 4 different types)<nl>
* Note:
* 			  None
*/
int32
rtk_rg_stormControl_add(rtk_rg_stormControlInfo_t *stormInfo,int *stormInfo_idx)
{
	 rtdrv_rg_stormControl_add_t cfg;
 
	 /* parameter check */
	 RT_PARAM_CHK((NULL == stormInfo), RT_ERR_NULL_POINTER);
	 RT_PARAM_CHK((NULL == stormInfo_idx), RT_ERR_NULL_POINTER);
 
	 /* function body */
	 osal_memcpy(&cfg.stormInfo, stormInfo, sizeof(rtk_rg_stormControlInfo_t));
	 osal_memcpy(&cfg.stormInfo_idx, stormInfo_idx, sizeof(int));
	 GETSOCKOPT(RTDRV_RG_STORMCONTROL_ADD, &cfg, rtdrv_rg_stormControl_add_t, 1);
	 osal_memcpy(stormInfo, &cfg.stormInfo, sizeof(rtk_rg_stormControlInfo_t));
	 osal_memcpy(stormInfo_idx, &cfg.stormInfo_idx, sizeof(int));
 
	 return RT_ERR_OK;
}	 /* end of rtk_rg_stormControl_add */

/* Function Name:
 *      rtk_rg_stormControl_del
 * Description:
 *     Delete stormControl.
 * Input:
 * Output:
 *      stormInfo_idx - [out]<tab>The index of stormInfo which should be delete.
 * Return:
 *      RT_ERR_RG_OK - Success<nl>
 *      RT_ERR_RG_INVALID_PARAM - parameters value is out of range.<nl>
 * Note:
 *               None
 */
int32
rtk_rg_stormControl_del(int stormInfo_idx)
{
    rtdrv_rg_stormControl_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.stormInfo_idx, &stormInfo_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_STORMCONTROL_DEL, &cfg, rtdrv_rg_stormControl_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_stormControl_del */

/* Function Name:
 *      rtk_rg_stormControl_find
 * Description:
 *     Find stormInfo index.
 * Input:
 * Output:
 *      astormInfo.port  - [in]<tab> assigned compare port
 *      stormInfo.stormType - [in]<tab> assigned compare stormType
 *      stormInfo.valid - [out]<tab>enable stormCtrol
 *      stormInfo.meterIdx - [out]<tab> assigned shareMeter index
 * Return:
 *      RT_ERR_RG_OK - Success<nl>
 *      RT_ERR_RG_NULL_POINTER - parameter is null. <nl>
 *      RT_ERR_RG_INVALID_PARAM - parameters value is out of range. <nl>
 *      RT_ERR_RG_STORMCONTROL_ENTRY_NOT_FOUND - assigned stormInfo not found<nl>
 * Note:
 *               None
 */
int32
rtk_rg_stormControl_find(rtk_rg_stormControlInfo_t *stormInfo,int *stormInfo_idx)
{
    rtdrv_rg_stormControl_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == stormInfo), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == stormInfo_idx), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.stormInfo, stormInfo, sizeof(rtk_rg_stormControlInfo_t));
    osal_memcpy(&cfg.stormInfo_idx, stormInfo_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_STORMCONTROL_FIND, &cfg, rtdrv_rg_stormControl_find_t, 1);
    osal_memcpy(stormInfo, &cfg.stormInfo, sizeof(rtk_rg_stormControlInfo_t));
    osal_memcpy(stormInfo_idx, &cfg.stormInfo_idx, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_stormControl_find */

/* Function Name:
 *      rtk_rg_shareMeter_set
 * Description:
 *     Set shareMeter rate.
 * Input:
 * Output:
 *      index - [in]<tab>shared meter index
 *      rate  - [in]<tab> rate of share meter
 *      ifgInclude - [in]<tab> include IFG or not, ENABLE:include DISABLE:exclude
 * Return:
 *      RT_ERR_RG_OK - Success<nl>
 * Note:
 *      The API can set shared meter rate and ifg include for each meter.
 *      The rate unit is 1 kbps and the range is from 8k to 1048568k.
 *      The granularity of rate is 8 kbps. The ifg_include parameter is used
 *      for rate calculation with/without inter-frame-gap and preamble.
 */
int32
rtk_rg_shareMeter_set(uint32 index, uint32 rate, rtk_rg_enable_t ifgInclude)
{
    rtdrv_rg_shareMeter_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(uint32));
    osal_memcpy(&cfg.rate, &rate, sizeof(uint32));
    osal_memcpy(&cfg.ifgInclude, &ifgInclude, sizeof(rtk_rg_enable_t));
    SETSOCKOPT(RTDRV_RG_SHAREMETER_SET, &cfg, rtdrv_rg_shareMeter_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_shareMeter_set */

/* Function Name:
 *      rtk_rg_shareMeter_get
 * Description:
 *     Set shareMeter rate.
 * Input:
 * Output:
 *      index - [in]<tab>shared meter index
 *      rate  - [out]<tab> pointer of rate of share meter
 *      ifgInclude - [out]<tab>include IFG or not, ENABLE:include DISABLE:exclude
 * Return:
 *      RT_ERR_RG_OK - Success<nl>
 * Note:
 *              The API can get shared meter rate and ifg include for each meter.
 *              The rate unit is 1 kbps and the granularity of rate is 8 kbps.
 *              The ifg_include parameter is used for rate calculation with/without inter-frame-gap and preamble
 */
int32
rtk_rg_shareMeter_get(uint32 index, uint32 *pRate , rtk_rg_enable_t *pIfgInclude)
{
    rtdrv_rg_shareMeter_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRate), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pIfgInclude), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(uint32));
    osal_memcpy(&cfg.pRate, pRate, sizeof(uint32));
    osal_memcpy(&cfg.pIfgInclude, pIfgInclude, sizeof(rtk_rg_enable_t));
    GETSOCKOPT(RTDRV_RG_SHAREMETER_GET, &cfg, rtdrv_rg_shareMeter_get_t, 1);
    osal_memcpy(pRate, &cfg.pRate, sizeof(uint32));
    osal_memcpy(pIfgInclude, &cfg.pIfgInclude, sizeof(rtk_rg_enable_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_shareMeter_get */

/* Function Name:
 *      rtk_rg_shareMeterMode_set
 * Description:
 *     Set shareMeter mode.
 * Input:
 * Output: 
 *      index - [in]<tab>shared meter index
 *      meterMode  - [in]<tab> meter mode
 * Return:
 *      RT_ERR_RG_OK - Success<nl>
 * Note:
 *              The API can set shared meter mode (bps or pps) for each meter.
 */
int32 
rtk_rg_shareMeterMode_set(uint32 index,rtk_rate_metet_mode_t meterMode)
{
    rtdrv_rg_shareMeterMode_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(uint32));
    osal_memcpy(&cfg.meterMode, &meterMode, sizeof(rtk_rate_metet_mode_t));
    SETSOCKOPT(RTDRV_RG_SHAREMETERMODE_SET, &cfg, rtdrv_rg_shareMeterMode_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_shareMeterMode_set */

 /* Function Name:
 *      rtk_rg_shareMeterMode_get
 * Description:
 *     Get shareMeter mode. 
 * Input:
 * Output: 
 *      index - [in]<tab>shared meter index
 *      meterMode  - [Out]<tab> meter mode
 * Return:
 *      RT_ERR_RG_OK - Success<nl>
 * Note:
 *              The API can get shared meter mode (bps or pps) for each meter.
 */
int32
rtk_rg_shareMeterMode_get(uint32 index,rtk_rate_metet_mode_t *pMeterMode)
{
    rtdrv_rg_shareMeterMode_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pMeterMode), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(uint32));
    osal_memcpy(&cfg.pMeterMode, pMeterMode, sizeof(rtk_rate_metet_mode_t));
    GETSOCKOPT(RTDRV_RG_SHAREMETERMODE_GET, &cfg, rtdrv_rg_shareMeterMode_get_t, 1);
    osal_memcpy(pMeterMode, &cfg.pMeterMode, sizeof(rtk_rate_metet_mode_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_shareMeterMode_get */


/* Function Name:
 *      rtk_rg_qosStrictPriorityOrWeightFairQueue_set
 * Description:
 *      Set the scheduling types and weights of queues on specific port in egress scheduling.
 * Input:
 * Output:
 *      port_idx - [in]<tab>The port index
 *      q_weight - [in]<tab>The array of weights for WRR/WFQ queue (valid:1~128, 0 for STRICT_PRIORITY queue)
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      The types of queue are: WFQ_WRR_PRIORITY or STRICT_PRIORITY.
 *      If the weight is 0 then the type is STRICT_PRIORITY, else the type is WFQ_WRR_PRIORITY.
 */
extern int32  
rtk_rg_qosStrictPriorityOrWeightFairQueue_set(rtk_rg_mac_port_idx_t port_idx,rtk_rg_qos_queue_weights_t q_weight)
{
	rtdrv_rg_qosStrictPriorityOrWeightFairQueue_set_t cfg;

	/* function body */
	osal_memcpy(&cfg.port_idx, &port_idx, sizeof(rtk_rg_mac_port_idx_t));
	osal_memcpy(&cfg.q_weight, &q_weight, sizeof(rtk_rg_qos_queue_weights_t));
	SETSOCKOPT(RTDRV_RG_QOSSTRICTPRIORITYORWEIGHTFAIRQUEUE_SET, &cfg, rtdrv_rg_qosStrictPriorityOrWeightFairQueue_set_t, 1);

	return RT_ERR_OK;
}	/* end of rtk_rg_qosStrictPriorityOrWeightFairQueue_set */

/* Function Name:
 *      rtk_rg_qosStrictPriorityOrWeightFairQueue_get
 * Description:
 *      Get the scheduling types and weights of queues on specific port in egress scheduling.
 * Input:
 * Output:
 *      port_idx - [in]<tab>The port index.
 *      pQ_weight - [in]<tab>Pointer to the array of weights for WRR/WFQ queue (valid:1~128, 0 for STRICT_PRIORITY queue)
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      The types of queue are: WFQ_WRR_PRIORITY or STRICT_PRIORITY.
 *      If the weight is 0 then the type is STRICT_PRIORITY, else the type is WFQ_WRR_PRIORITY.
 */
extern int32
rtk_rg_qosStrictPriorityOrWeightFairQueue_get(rtk_rg_mac_port_idx_t port_idx,rtk_rg_qos_queue_weights_t *pQ_weight)
{
	rtdrv_rg_qosStrictPriorityOrWeightFairQueue_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pQ_weight), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.port_idx, &port_idx, sizeof(rtk_rg_mac_port_idx_t));
	osal_memcpy(&cfg.pQ_weight, pQ_weight, sizeof(rtk_rg_qos_queue_weights_t));
	GETSOCKOPT(RTDRV_RG_QOSSTRICTPRIORITYORWEIGHTFAIRQUEUE_GET, &cfg, rtdrv_rg_qosStrictPriorityOrWeightFairQueue_get_t, 1);
	osal_memcpy(pQ_weight, &cfg.pQ_weight, sizeof(rtk_rg_qos_queue_weights_t));

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosInternalPriMapToQueueId_set
 * Description:
 *      Set the entry of internal priority to QID mapping table.
 * Input:
 * Output:
 *      queueId  - [in]<tab>Mapping queue Id.
 * Return:
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_OK
 * Note:
 *      Below is an example of internal priority to QID mapping table.
 *      -
 *      -              
 *      - Priority   0   1   2   3   4   5   6   7
 *      -              ==================== 
 *      - queue     0   0   1   1   2   2   3   3
 *      -
 *      -for table index 0
 *      -    pPri2qid[0] = 0   internal priority 0 map to queue 0       
 *      -    pPri2qid[1] = 0   internal priority 1 map to queue 0 
 *      -    pPri2qid[2] = 1   internal priority 2 map to queue 1 
 *      -    pPri2qid[3] = 1   internal priority 3 map to queue 1 
 *      -    pPri2qid[4] = 2   internal priority 4 map to queue 2 
 *      -    pPri2qid[5] = 2   internal priority 5 map to queue 2 
 *      -    pPri2qid[6] = 3   internal priority 6 map to queue 3  
 *      -    pPri2qid[7] = 3   internal priority 7 map to queue 3 
 */
extern int32
rtk_rg_qosInternalPriMapToQueueId_set(int intPri, int queueId)
{
	rtdrv_rg_qosInternalPriMapToQueueId_set_t cfg;

	/* function body */
	osal_memcpy(&cfg.intPri, &intPri, sizeof(uint32));
	osal_memcpy(&cfg.queueId, &queueId, sizeof(uint32));
	SETSOCKOPT(RTDRV_RG_QOSINTERNALPRIMAPTOQUEUEID_SET, &cfg, rtdrv_rg_qosInternalPriMapToQueueId_set_t, 1);

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosInternalPriMapToQueueId_get
 * Description:
 *      Get the entry of internal priority to QID mapping table.
 * Input:
 * Output:
 *      pQueueId  - [out]<tab>Pointer to mapping queue Id.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *		None
 */
extern int32
rtk_rg_qosInternalPriMapToQueueId_get(int intPri, int *pQueueId)
{
	rtdrv_rg_qosInternalPriMapToQueueId_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pQueueId), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.intPri, &intPri, sizeof(uint32));
	GETSOCKOPT(RTDRV_RG_QOSINTERNALPRIMAPTOQUEUEID_GET, &cfg, rtdrv_rg_qosInternalPriMapToQueueId_get_t, 1);
	osal_memcpy(pQueueId, &cfg.pQueueId, sizeof(uint32));

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosInternalPriDecisionByWeight_set
 * Description:
 *      Set weight of each priority assignment.
 * Input:
 * Output:
 *		weightOfPriSel - [in]<tab>weight of each priority assignment
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      None
 */
extern int32
rtk_rg_qosInternalPriDecisionByWeight_set(rtk_rg_qos_priSelWeight_t weightOfPriSel)
{
	rtdrv_rg_qosInternalPriDecisionByWeight_set_t cfg;

   /* function body */
	osal_memcpy(&cfg.weightOfPriSel, &weightOfPriSel, sizeof(rtk_rg_qos_priSelWeight_t));
	SETSOCKOPT(RTDRV_RG_QOSINTERNALPRIDECISIONBYWEIGHT_SET, &cfg, rtdrv_rg_qosInternalPriDecisionByWeight_set_t, 1);

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosInternalPriDecisionByWeight_get
 * Description:
 *      Get weight of each priority assignment.
 * Input:
 * Output:
 *	  pWeightOfPriSel - [out]<tab>weight of each priority assignment
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      None
 */
extern int32
rtk_rg_qosInternalPriDecisionByWeight_get(rtk_rg_qos_priSelWeight_t *pWeightOfPriSel)
{
    rtdrv_rg_qosInternalPriDecisionByWeight_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pWeightOfPriSel), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pWeightOfPriSel, pWeightOfPriSel, sizeof(rtk_rg_qos_priSelWeight_t));
    GETSOCKOPT(RTDRV_RG_QOSINTERNALPRIDECISIONBYWEIGHT_GET, &cfg, rtdrv_rg_qosInternalPriDecisionByWeight_get_t, 1);
    osal_memcpy(pWeightOfPriSel, &cfg.pWeightOfPriSel, sizeof(rtk_rg_qos_priSelWeight_t));

    return RT_ERR_OK;
} 

/* Function Name:
 *      rtk_rg_qosDscpRemapToInternalPri_set
 * Description:
 *      Set remapped internal priority of DSCP.
 * Input:
 * Output:
 *      dscp  - [in]<tab>DSCP
 *      intPri - [in]<tab>internal priority
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 * Note:
 *      Apollo only support group 0
 */
extern int32
rtk_rg_qosDscpRemapToInternalPri_set(uint32 dscp,uint32 intPri)
{
	rtdrv_rg_qosDscpRemapToInternalPri_set_t cfg;

	/* function body */
	osal_memcpy(&cfg.dscp, &dscp, sizeof(uint32));
	osal_memcpy(&cfg.intPri, &intPri, sizeof(uint32));
	SETSOCKOPT(RTDRV_RG_QOSDSCPREMAPTOINTERNALPRI_SET, &cfg, rtdrv_rg_qosDscpRemapToInternalPri_set_t, 1);

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDscpRemapToInternalPri_get
 * Description:
 *      Get remapped internal priority of DSCP.
 * Input:
 * Output:
 *      dscp  - [in]<tab>DSCP
 *      pIntPri - [out]<tab>internal priority
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      Apollo only support group 0
 */
extern int32
rtk_rg_qosDscpRemapToInternalPri_get(uint32 dscp,uint32 *pIntPri)
{
    rtdrv_rg_qosDscpRemapToInternalPri_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pIntPri), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.dscp, &dscp, sizeof(uint32));
    osal_memcpy(&cfg.pIntPri, pIntPri, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_QOSDSCPREMAPTOINTERNALPRI_GET, &cfg, rtdrv_rg_qosDscpRemapToInternalPri_get_t, 1);
    osal_memcpy(pIntPri, &cfg.pIntPri, sizeof(uint32));

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosPortBasedPriority_set
 * Description:
 *      Set internal priority of one port.
 * Input:
 * Output:
 *      port_idx     - [in]<tab>Port index, including extension port.
 *      intPri  - [in]<tab>Priorities assigment for specific port. 
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM 
 * Note:
 *    None
 */
extern int32
rtk_rg_qosPortBasedPriority_set(rtk_rg_mac_port_idx_t port_idx,uint32 intPri)
{
	rtdrv_rg_qosPortBasedPriority_set_t cfg;

	/* function body */
	osal_memcpy(&cfg.port, &port_idx, sizeof(rtk_rg_mac_port_idx_t));
	osal_memcpy(&cfg.intPri, &intPri, sizeof(uint32));
	SETSOCKOPT(RTDRV_RG_QOSPORTBASEDPRIORITY_SET, &cfg, rtdrv_rg_qosPortBasedPriority_set_t, 1);

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosPortBasedPriority_get
 * Description:
 *      Get internal priority of one port.
 * Input:
 * Output:
 *      port_idx     - [in]<tab>Port index, including extension port.
 *      pIntPri  - [out]<tab>Priorities assigment for specific port. 
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM 
 *		RT_ERR_RG_NULL_POINTER
 * Note:
 *    None
 */
extern int32
rtk_rg_qosPortBasedPriority_get(rtk_rg_mac_port_idx_t port_idx,uint32 *pIntPri)
{
	rtdrv_rg_qosPortBasedPriority_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pIntPri), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.port, &port_idx, sizeof(rtk_rg_mac_port_idx_t));
	osal_memcpy(&cfg.pIntPri, pIntPri, sizeof(uint32));
	GETSOCKOPT(RTDRV_RG_QOSPORTBASEDPRIORITY_GET, &cfg, rtdrv_rg_qosPortBasedPriority_get_t, 1);
	osal_memcpy(pIntPri, &cfg.pIntPri, sizeof(uint32));

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDot1pPriRemapToInternalPri_set
 * Description:
 *      Set remapped internal priority of 802.1p priority.
 * Input:
 * Output:
 *      dot1p  - [in]<tab>802.1p priority
 *      intPri - [in]<tab>internal priority
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 * Note:
 *      Apollo only support group 0
 */
extern int32
rtk_rg_qosDot1pPriRemapToInternalPri_set(uint32 dot1p,uint32 intPri)
{
	rtdrv_rg_qosDot1pPriRemapToInternalPri_set_t cfg;

	/* function body */
	osal_memcpy(&cfg.dot1p, &dot1p, sizeof(uint32));
	osal_memcpy(&cfg.intPri, &intPri, sizeof(uint32));
	SETSOCKOPT(RTDRV_RG_QOSDOT1PPRIREMAPTOINTERNALPRI_SET, &cfg, rtdrv_rg_qosDot1pPriRemapToInternalPri_set_t, 1);

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDot1pPriRemapToInternalPri_get
 * Description:
 *      Get remapped internal priority of 802.1p priority.
 * Input:
 * Output:
 *      dscp  - [in]<tab>802.1p priority
 *      pIntPri - [out]<tab>internal priority
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      Apollo only support group 0
 */
extern int32
rtk_rg_qosDot1pPriRemapToInternalPri_get(uint32 dot1p,uint32 *pIntPri)
{
	rtdrv_rg_qosDot1pPriRemapToInternalPri_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pIntPri), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.dot1p, &dot1p, sizeof(uint32));
	osal_memcpy(&cfg.pIntPri, pIntPri, sizeof(uint32));
	GETSOCKOPT(RTDRV_RG_QOSDOT1PPRIREMAPTOINTERNALPRI_GET, &cfg, rtdrv_rg_qosDot1pPriRemapToInternalPri_get_t, 1);
	osal_memcpy(pIntPri, &cfg.pIntPri, sizeof(uint32));

	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_set
 * Description:
 *      Set remark DSCP enable/disable port.
 * Input:
 * Output:
 *      rmk_port  - [in]<tab>Enable/Dsiable DSCP remarking port
 *      rmk_enable - [in]<tab>Enable/Disable DSCP remark
 *      rmk_src_select  - [in]<tab>DSCP remarking source
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 * Note:
 *		rtk_rg_qos_dscpRmkSrc_t.RTK_RG_DSCP_RMK_SRC_INT_PRI - Using internal priority as DSCP remarking source.
 *		rtk_rg_qos_dscpRmkSrc_t.RTK_RG_DSCP_RMK_SRC_DSCP - Using DSCP as DSCP remarking source.
 */
extern int32
rtk_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_set(rtk_rg_mac_port_idx_t rmk_port,rtk_rg_enable_t rmk_enable, rtk_rg_qos_dscpRmkSrc_t rmk_src_select)
{
   rtdrv_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_set_t cfg;

	/* function body */
	osal_memcpy(&cfg.port, &rmk_port, sizeof(rtk_rg_mac_port_idx_t));
	osal_memcpy(&cfg.is_enabled, &rmk_enable, sizeof(rtk_rg_enable_t));
	osal_memcpy(&cfg.src_sel, &rmk_src_select, sizeof(rtk_rg_qos_dscpRmkSrc_t));
	SETSOCKOPT(RTDRV_RG_QOSDSCPREMARKEGRESSPORTENABLEANDSRCSELECT_SET, &cfg, rtdrv_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_set_t, 1);
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_get
 * Description:
 *      Get remark DSCP enable/disable port and remarking source.
 * Input:
 * Output:
 *      rmk_port  - [in]<tab>DSCP remarking port 
 *      pRmk_enable  - [in]<tab>Pointer to DSCP remarking port enabled/disabled
 *      pRmk_src_select  - [in]<tab>Pointer to DSCP remarking source 
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 *		RT_ERR_RG_NULL_POINTER
 *		RT_ERR_RG_FAILED
 * Note:
 *		rtk_rg_qos_dscpRmkSrc_t.RTK_RG_DSCP_RMK_SRC_INT_PRI - Using internal priority as DSCP remarking source.
 *		rtk_rg_qos_dscpRmkSrc_t.RTK_RG_DSCP_RMK_SRC_DSCP - Using DSCP as DSCP remarking source.
 */
extern int32
rtk_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_get(rtk_rg_mac_port_idx_t rmk_port,rtk_rg_enable_t *pRmk_enable, rtk_rg_qos_dscpRmkSrc_t *pRmk_src_select)
{
	rtdrv_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pRmk_enable), RT_ERR_NULL_POINTER);
	RT_PARAM_CHK((NULL == pRmk_src_select), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.port, &rmk_port, sizeof(rtk_rg_mac_port_idx_t));
	osal_memcpy(&cfg.pIs_enabled, pRmk_enable, sizeof(rtk_rg_enable_t));
	osal_memcpy(&cfg.pSrc_sel, pRmk_src_select, sizeof(uint32));
	GETSOCKOPT(RTDRV_RG_QOSDSCPREMARKEGRESSPORTENABLEANDSRCSELECT_GET, &cfg, rtdrv_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_get_t, 1);
	osal_memcpy(pRmk_enable, &cfg.pIs_enabled, sizeof(rtk_rg_enable_t));
	osal_memcpy(pRmk_src_select, &cfg.pSrc_sel, sizeof(rtk_rg_qos_dscpRmkSrc_t));
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDscpRemarkByInternalPri_set
 * Description:
 *      Set Internal priority/User priority to DSCP remarking mapping.
 * Input:
 * Output:
 *      int_pri  - [in]<tab>Internal/User priority
 *      rmk_dscp - [in]<tab>DSCP remarking value
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 * Note:
 */
extern int32
rtk_rg_qosDscpRemarkByInternalPri_set(int int_pri,int rmk_dscp)
{
	rtdrv_rg_qosDscpRemarkByInternalPri_set_t cfg;

	/* function body */
	osal_memcpy(&cfg.int_pri, &int_pri, sizeof(uint32));
	osal_memcpy(&cfg.rmk_dscp, &rmk_dscp, sizeof(uint32));
	SETSOCKOPT(RTDRV_RG_QOSDSCPREMARKBYINTERNALPRI_SET, &cfg, rtdrv_rg_qosDscpRemarkByInternalPri_set_t, 1);
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDscpRemarkByInternalPri_get
 * Description:
 *      Get Internal priority/User priority to DSCP remarking mapping.
 * Input:
 * Output:
 *      int_pri  - [in]<tab>Internal/User priority
 *      pRmk_dscp - [out]<tab>Pointer to remarking DSCP
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 *		RT_ERR_RG_NULL_POINTER
 * Note:
 */
extern int32
rtk_rg_qosDscpRemarkByInternalPri_get(int int_pri,int *pRmk_dscp)
{
	rtdrv_rg_qosDscpRemarkByInternalPri_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == pRmk_dscp), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.int_pri, &int_pri, sizeof(uint32));
	osal_memcpy(&cfg.pRmk_dscp, pRmk_dscp, sizeof(uint32));
	GETSOCKOPT(RTDRV_RG_QOSDSCPREMARKBYINTERNALPRI_GET, &cfg, rtdrv_rg_qosDscpRemarkByInternalPri_get_t, 1);
	osal_memcpy(pRmk_dscp, &cfg.pRmk_dscp, sizeof(uint32));
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDscpRemarkByDscp_set
 * Description:
 *      Set DSCP to DSCP remarking mapping.
 * Input:
 * Output:
 *      dscp  - [in]<tab>DSCP source
 *      rmk_dscp - [in]<tab>DSCP remarking value
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 * Note:
 */
extern int32
rtk_rg_qosDscpRemarkByDscp_set(int dscp,int rmk_dscp)
{
    rtdrv_rg_qosDscpRemarkByDscp_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.dscp, &dscp, sizeof(uint32));
    osal_memcpy(&cfg.rmk_dscp, &rmk_dscp, sizeof(uint32));
    SETSOCKOPT(RTDRV_RG_QOSDSCPREMARKBYDSCP_SET, &cfg, rtdrv_rg_qosDscpRemarkByDscp_set_t, 1);
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDscpRemarkByDscp_get
 * Description:
 *      Get DSCP to DSCP remarking mapping.
 * Input:
 * Output:
 *      dscp  - [in]<tab>DSCP source
 *      pRmk_dscp - [out]<tab>Pointer to DSCP remarking value
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 *		RT_ERR_RG_NULL_POINTER
 * Note:
 */
extern int32
rtk_rg_qosDscpRemarkByDscp_get(int dscp,int *pRmk_dscp)
{
    rtdrv_rg_qosDscpRemarkByDscp_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRmk_dscp), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.dscp, &dscp, sizeof(uint32));
    osal_memcpy(&cfg.pRmk_dscp, pRmk_dscp, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_QOSDSCPREMARKBYDSCP_GET, &cfg, rtdrv_rg_qosDscpRemarkByDscp_get_t, 1);
    osal_memcpy(pRmk_dscp, &cfg.pRmk_dscp, sizeof(uint32));
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_set
 * Description:
 *      Set remark 802.1p priority port.
 * Input:
 * Output:
 *      rmk_port  - [in]<tab>Enable/Dsiable 802.1p remarking port.
 *      rmk_enable - [in]<tab>Enable/Disable 1p remark.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 * Note:
 */
extern int32
rtk_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_set(rtk_rg_mac_port_idx_t rmk_port, rtk_rg_enable_t rmk_enable)
{
    rtdrv_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.port, &rmk_port, sizeof(rtk_rg_mac_port_idx_t));
    osal_memcpy(&cfg.enable, &rmk_enable, sizeof(rtk_rg_enable_t));
    SETSOCKOPT(RTDRV_RG_QOSDOT1PPRIREMARKBYINTERNALPRIEGRESSPORTENABLE_SET, &cfg, rtdrv_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_set_t, 1);
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_get
 * Description:
 *      Get remark 802.1p priority port.
 * Input:
 * Output:
 *      rmk_port  - [out]<tab>Pointer to 802.1p remarking port.
 *      pRmk_enable - [in]<tab>Enable/Disable 1p remark.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 *		RT_ERR_RG_NULL_POINTER
 * Note:
 */
extern int32
rtk_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_get(rtk_rg_mac_port_idx_t rmk_port, rtk_rg_enable_t *pRmk_enable)
{
    rtdrv_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRmk_enable), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.port, &rmk_port, sizeof(rtk_rg_mac_portmask_t));
    osal_memcpy(&cfg.pEnable, &pRmk_enable, sizeof(rtk_rg_enable_t));
    GETSOCKOPT(RTDRV_RG_QOSDOT1PPRIREMARKBYINTERNALPRIEGRESSPORTENABLE_GET, &cfg, rtdrv_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_get_t, 1);
    osal_memcpy(pRmk_enable, &cfg.pEnable, sizeof(rtdrv_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_get_t));
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDot1pPriRemarkByInternalPri_set
 * Description:
 *      Set Internal priority to 802.1p remarking mapping.
 * Input:
 * Output:
 *      int_pri  - [in]<tab>Internal priority
 *      rmk_dot1p - [in]<tab>802.1p priority remarking value
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 * Note:
 */
extern int32
rtk_rg_qosDot1pPriRemarkByInternalPri_set(int int_pri,int rmk_dot1p)
{
    rtdrv_rg_qosDot1pPriRemarkByInternalPri_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.int_pri, &int_pri, sizeof(uint32));
    osal_memcpy(&cfg.rmk_dot1p, &rmk_dot1p, sizeof(uint32));
    SETSOCKOPT(RTDRV_RG_QOSDOT1PPRIREMARKBYINTERNALPRI_SET, &cfg, rtdrv_rg_qosDot1pPriRemarkByInternalPri_set_t, 1);
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_qosDot1pPriRemarkByInternalPri_get
 * Description:
 *      Get Internal priority to 802.1p remarking mapping.
 * Input:
 * Output:
 *      int_pri  - [in]<tab>Internal priority
 *      pRmk_dot1p - [out]<tab>Pointer to 802.1p priority remarking value
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 *		RT_ERR_RG_NULL_POINTER
 * Note:
 */
extern int32
rtk_rg_qosDot1pPriRemarkByInternalPri_get(int int_pri,int *pRmk_dot1p)
{
    rtdrv_rg_qosDot1pPriRemarkByInternalPri_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRmk_dot1p), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.int_pri, &int_pri, sizeof(uint32));
    osal_memcpy(&cfg.pRmk_dot1p, pRmk_dot1p, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_QOSDOT1PPRIREMARKBYINTERNALPRI_GET, &cfg, rtdrv_rg_qosDot1pPriRemarkByInternalPri_get_t, 1);
    osal_memcpy(pRmk_dot1p, &cfg.pRmk_dot1p, sizeof(uint32));
	return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_portBasedCVlanId_set
 * Description:
 *      Set port vlan ID.
 * Input:
 * Output:
 *      port  - [in]<tab>Port index
 *      pPvid - [in]<tab>Wished port vlan id.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 * Note:
 */
int32 
rtk_rg_portBasedCVlanId_set(rtk_rg_port_idx_t port_idx,int pvid)
{
    rtdrv_rg_portBasedCVlanId_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.port_idx, &port_idx, sizeof(rtk_rg_port_idx_t));
    osal_memcpy(&cfg.pvid, &pvid, sizeof(int));
    SETSOCKOPT(RTDRV_RG_PORTBASEDCVLANID_SET, &cfg, rtdrv_rg_portBasedCVlanId_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_portBasedCVlanId_set */


/* Function Name:
 *      rtk_rg_portBasedCVlanId_get
 * Description:
 *      Get port vlan ID.
 * Input:
 * Output:
 *      port  - [in]<tab>Port index
 *      pPvid - [in]<tab>Pointer to port vlan id.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM  
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 */
int32 
rtk_rg_portBasedCVlanId_get(rtk_rg_port_idx_t port_idx,int *pPvid)
{
    rtdrv_rg_portBasedCVlanId_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pPvid), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.port_idx, &port_idx, sizeof(rtk_rg_port_idx_t));
    osal_memcpy(&cfg.pPvid, pPvid, sizeof(int));
    GETSOCKOPT(RTDRV_RG_PORTBASEDCVLANID_GET, &cfg, rtdrv_rg_portBasedCVlanId_get_t, 1);
    osal_memcpy(pPvid, &cfg.pPvid, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_portBasedCVlanId_get */


/* Function Name:
 *      rtk_rg_portStatus_get
 * Description:
 *      Get port linkup status.
 * Input:
 * Output:
 *      port  - [in]<tab>Port index
 *      portInfo - [out]<tab>port linkup information.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *              RT_ERR_RG_NULL_POINTER
 * Note:
 */
int32
rtk_rg_portStatus_get(rtk_rg_mac_port_idx_t port, rtk_rg_portStatusInfo_t *portInfo)
{
    rtdrv_rg_portStatus_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == portInfo), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.port, &port, sizeof(rtk_rg_mac_port_idx_t));
    osal_memcpy(&cfg.portInfo, portInfo, sizeof(rtk_rg_portStatusInfo_t));
    GETSOCKOPT(RTDRV_RG_PORTSTATUS_GET, &cfg, rtdrv_rg_portStatus_get_t, 1);
    osal_memcpy(portInfo, &cfg.portInfo, sizeof(rtk_rg_portStatusInfo_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_portStatus_get */

/* Function Name:
 *      rtk_rg_wlanDevBasedCVlanId_set
 * Description:
 *      Set wlan devices' vlan ID.
 * Input:
 * Output:
 *      wlan - [in]<tab>Wlan index
 *      dev  - [in]<tab>Dev index
 *      dvid - [in]<tab>Wished dev vlan id.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_ENTRY_NOT_EXIST - VlanID not exist
 * Note:
 */
int32
rtk_rg_wlanDevBasedCVlanId_set(int wlan_idx,int dev_idx,int dvid)
{
    rtdrv_rg_wlanDevBasedCVlanId_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.wlan_idx, &wlan_idx, sizeof(int));
    osal_memcpy(&cfg.dev_idx, &dev_idx, sizeof(int));
    osal_memcpy(&cfg.dvid, &dvid, sizeof(int));
    SETSOCKOPT(RTDRV_RG_WLANDEVBASEDCVLANID_SET, &cfg, rtdrv_rg_wlanDevBasedCVlanId_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_wlanDevBasedCVlanId_set */

/* Function Name:
 *      rtk_rg_wlanDevBasedCVlanId_get
 * Description:
 *      Get wlan devices' vlan ID.
 * Input:
 * Output:
 *      wlan - [in]<tab>Wlan index
 *      dev  - [in]<tab>Dev index
 *      pDvid - [out]<tab>Pointer to dev vlan id.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 */
int32
rtk_rg_wlanDevBasedCVlanId_get(int wlan_idx,int dev_idx,int *pDvid)
{
    rtdrv_rg_wlanDevBasedCVlanId_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pDvid), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.wlan_idx, &wlan_idx, sizeof(int));
    osal_memcpy(&cfg.dev_idx, &dev_idx, sizeof(int));
    osal_memcpy(&cfg.pDvid, pDvid, sizeof(int));
    GETSOCKOPT(RTDRV_RG_WLANDEVBASEDCVLANID_GET, &cfg, rtdrv_rg_wlanDevBasedCVlanId_get_t, 1);
    osal_memcpy(pDvid, &cfg.pDvid, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_wlanDevBasedCVlanId_get */


/* Function Name:
 *      rtk_rg_classifyEntry_add
 * Description:
 *      Add an classification entry to ASIC
 * Input:
 *      classifyFilter     - The classification configuration that this function will
add comparison rule
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                                               - OK
 *      RT_ERR_FAILED                                           - Failed
 *      RT_ERR_NULL_POINTER                             - Pointer classifyFilter point
 to NULL.
 *      RT_ERR_INPUT                                                    - Invalid inpu
t parameters.
 * Note:
 *      None.
 */
int32
rtk_rg_classifyEntry_add(rtk_rg_classifyEntry_t *classifyFilter)
{
    rtdrv_rg_classifyEntry_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == classifyFilter), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.classifyFilter, classifyFilter, sizeof(rtk_rg_classifyEntry_t));
    GETSOCKOPT(RTDRV_RG_CLASSIFYENTRY_ADD, &cfg, rtdrv_rg_classifyEntry_add_t, 1);
    osal_memcpy(classifyFilter, &cfg.classifyFilter, sizeof(rtk_rg_classifyEntry_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_classifyEntry_add */

/* Function Name:
 *      rtk_rg_classifyEntry_find
 * Description:
 *      Get an classification entry from ASIC
 * Input:
 *      index           - The Index of classification entry
 * Output:
 *      classifyFilter     - The classification configuration
 * Return:
 *      RT_ERR_OK                                               - OK
 *      RT_ERR_FAILED                                           - Failed
 *      RT_ERR_NULL_POINTER                                     - Pointer pClassifyCfg
 point to NULL.
 *      RT_ERR_INPUT                                                    - Invalid inpu
t parameters.
 * Note:
 *      None.
 */
int32
rtk_rg_classifyEntry_find(int index, rtk_rg_classifyEntry_t *classifyFilter)
{
    rtdrv_rg_classifyEntry_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == classifyFilter), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    osal_memcpy(&cfg.classifyFilter, classifyFilter, sizeof(rtk_rg_classifyEntry_t));
    GETSOCKOPT(RTDRV_RG_CLASSIFYENTRY_FIND, &cfg, rtdrv_rg_classifyEntry_find_t, 1);
    osal_memcpy(classifyFilter, &cfg.classifyFilter, sizeof(rtk_rg_classifyEntry_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_classifyEntry_find */

/* Function Name:
 *      rtk_rg_classifyEntry_del
 * Description:
 *      Delete an classification configuration from ASIC
 * Input:
 *      index    - index of classification entry.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_ENTRY_INDEX              - Invalid classification index .
 * Note:
 *      None.
 */
int32
rtk_rg_classifyEntry_del(int index)
{
    rtdrv_rg_classifyEntry_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    SETSOCKOPT(RTDRV_RG_CLASSIFYENTRY_DEL, &cfg, rtdrv_rg_classifyEntry_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_classifyEntry_del */

/* Function Name:
 *      rtk_rg_pppoeInterfaceIdleTime_get
 * Description:
 *      Get idle time from pppoe interface
 * Input:
 *      intfIdx    - index of interface entry.
 * Output:
 *      idleSec    - idle time (unit:sec)
 * Return:
 *      RT_ERR_RG_OK               - OK
 *      RT_ERR_RG_INVALID_PARAM                 - Invalid intfIdx index .
 * Note:
 *      None.
 */
int32
rtk_rg_pppoeInterfaceIdleTime_get(int intfIdx,uint32 *idleSec)
{
    rtdrv_rg_pppoeInterfaceIdleTime_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == idleSec), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.intfIdx, &intfIdx, sizeof(int));
    osal_memcpy(&cfg.idleSec, idleSec, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_PPPOEINTERFACEIDLETIME_GET, &cfg, rtdrv_rg_pppoeInterfaceIdleTime_get_t, 1);
    osal_memcpy(idleSec, &cfg.idleSec, sizeof(uint32));

    return RT_ERR_OK;
}   /* end of rtk_rg_pppoeInterfaceIdleTime_get */

/* Function Name:
 *      rtk_rg_gatewayServicePortRegister_add
 * Description:
 *      add a gateway service port, once the packet src/dest port_num hit will be trap to protocal-stack
 * Input:
 *      serviceEntry    - gateway service port information.
 * Output:
 *      index      - the index of gatewayServicePort table
 * Return:
 *      RT_ERR_RG_OK               - OK
 *      RT_ERR_RG_INVALID_PARAM                 - Invalid parameter .
 *      RT_ERR_RG_ENTRY_EXIST               -The Entry has been registered.
 *      RT_ERR_RG_ENTRY_FULL               -The fwdEngine supported gatewayServicePort table is full.
 * Note:
 *      None.
 */
int32
rtk_rg_gatewayServicePortRegister_add(rtk_rg_gatewayServicePortEntry_t *serviceEntry, int *index)
{
    rtdrv_rg_gatewayServicePortRegister_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == serviceEntry), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == index), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.serviceEntry, serviceEntry, sizeof(rtk_rg_gatewayServicePortEntry_t));
    osal_memcpy(&cfg.index, index, sizeof(int));
    GETSOCKOPT(RTDRV_RG_GATEWAYSERVICEPORTREGISTER_ADD, &cfg, rtdrv_rg_gatewayServicePortRegister_add_t, 1);
    osal_memcpy(serviceEntry, &cfg.serviceEntry, sizeof(rtk_rg_gatewayServicePortEntry_t));
    osal_memcpy(index, &cfg.index, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_gatewayServicePortRegister_add */

/* Function Name:
 *      rtk_rg_gatewayServicePortRegister_del
 * Description:
 *      Del a gateway service port which registered in gatewayServicePort table
 * Input:
 *      index    - gatewayServicePort table index.
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK               - OK
 *      RT_ERR_RG_ENTRY_NOT_FOUND               - The entry not exist.
 * Note:
 *      None.
 */
int32
rtk_rg_gatewayServicePortRegister_del(int index)
{
    rtdrv_rg_gatewayServicePortRegister_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    SETSOCKOPT(RTDRV_RG_GATEWAYSERVICEPORTREGISTER_DEL, &cfg, rtdrv_rg_gatewayServicePortRegister_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_gatewayServicePortRegister_del */

/* Function Name:
 *      rtk_rg_gatewayServicePortRegister_find
 * Description:
 *      Get a gateway service port Entry from gatewayServicePort.
 * Input:
 *      index      - the index of gatewayServicePort table
 * Output:
 *      serviceEntry    - gateway service port information.
 * Return:
 *      RT_ERR_RG_OK               - OK
 *      RT_ERR_RG_INVALID_PARAM                 - Invalid parameter .
 *      RT_ERR_RG_ENTRY_NOT_FOUND               - The entry not exist.
 * Note:
 *      None.
 */
int32
rtk_rg_gatewayServicePortRegister_find(rtk_rg_gatewayServicePortEntry_t *serviceEntry, int *index)
{
    rtdrv_rg_gatewayServicePortRegister_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == serviceEntry), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == index), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.serviceEntry, serviceEntry, sizeof(rtk_rg_gatewayServicePortEntry_t));
    osal_memcpy(&cfg.index, index, sizeof(int));
    GETSOCKOPT(RTDRV_RG_GATEWAYSERVICEPORTREGISTER_FIND, &cfg, rtdrv_rg_gatewayServicePortRegister_find_t, 1);
    osal_memcpy(serviceEntry, &cfg.serviceEntry, sizeof(rtk_rg_gatewayServicePortEntry_t));
    osal_memcpy(index, &cfg.index, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_gatewayServicePortRegister_find */


/* Function Name:
 *              rtk_rg_gponDsBcFilterAndRemarking_Enable
 * Description:
 *              enable/disable the GPON downstream Broadcast filter module
 * Input:
 *              enable     - enable/disable the GPON downstream Broadcast filter module
 * Return:
 *              RT_ERR_RG_OK                       - OK
 *              RT_ERR_RG_INVALID_PARAM                 - Invalid parameter .
 * Note:
 *              None.
 */
int32
rtk_rg_gponDsBcFilterAndRemarking_Enable(rtk_rg_enable_t enable)
{
    rtdrv_rg_gponDsBcFilterAndRemarking_Enable_t cfg;

    /* function body */
    osal_memcpy(&cfg.enable, &enable, sizeof(rtk_rg_enable_t));
    SETSOCKOPT(RTDRV_RG_GPONDSBCFILTERANDREMARKING_ENABLE, &cfg, rtdrv_rg_gponDsBcFilterAndRemarking_Enable_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_gponDsBcFilterAndRemarking_Enable */



/* Function Name:
 *      rtk_rg_gponDsBcFilterAndRemarking_add
 * Description:
 *      Add egress remarking rule for GPON downstream Broadcast
 * Input:
 *      index      - the assigned rule index to dsBcFilterAndRemarking table
 *      filterRule    - filter and remarking rule.
 * Return:
 *      RT_ERR_RG_OK               - OK
 *      RT_ERR_RG_INVALID_PARAM                 - Invalid parameter .
 *      RT_ERR_RG_ENTRY_NOT_FOUND               - The entry not exist.
 * Note:
 *      None.
 */
int32
rtk_rg_gponDsBcFilterAndRemarking_add(rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t *filterRule,int *index)
{
    rtdrv_rg_gponDsBcFilterAndRemarking_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == filterRule), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == index), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.filterRule, filterRule, sizeof(rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t));
    osal_memcpy(&cfg.index, index, sizeof(int));
    GETSOCKOPT(RTDRV_RG_GPONDSBCFILTERANDREMARKING_ADD, &cfg, rtdrv_rg_gponDsBcFilterAndRemarking_add_t, 1);
    osal_memcpy(filterRule, &cfg.filterRule, sizeof(rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t));
    osal_memcpy(index, &cfg.index, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_gponDsBcFilterAndRemarking_add */

/* Function Name:
 *      rtk_rg_gponDsBcFilterAndRemarking_del
 * Description:
 *      Delete egress remarking rule form GPON downstream Broadcast
 * Input:
 *      index      - the assigned rule index to dsBcFilterAndRemarking table
 * Return:
 *      RT_ERR_RG_OK               - OK
 *      RT_ERR_RG_INVALID_PARAM                 - Invalid parameter .
 *      RT_ERR_RG_ENTRY_NOT_FOUND               - The entry not exist.
 * Note:
 *      None.
 */
int32
rtk_rg_gponDsBcFilterAndRemarking_del(int index)
{
    rtdrv_rg_gponDsBcFilterAndRemarking_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    SETSOCKOPT(RTDRV_RG_GPONDSBCFILTERANDREMARKING_DEL, &cfg, rtdrv_rg_gponDsBcFilterAndRemarking_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_gponDsBcFilterAndRemarking_del */

/* Function Name:
 *      rtk_rg_gponDsBcFilterAndRemarking_find
 * Description:
 *      Delete egress remarking rule form GPON downstream Broadcast
 * Input:
 *      index      - the assigned rule index to get dsBcFilterAndRemarking table entry
 * Output:
 *      filterRule    - the dsBcFilterAndRemarking table rule entry.
 * Return:
 *      RT_ERR_RG_OK               - OK
 *      RT_ERR_RG_INVALID_PARAM                 - Invalid parameter .
 *      RT_ERR_RG_ENTRY_NOT_FOUND               - The entry not exist.
 * Note:
 *      None.
 */
int32
rtk_rg_gponDsBcFilterAndRemarking_find(int *index,rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t *filterRule)
{
    rtdrv_rg_gponDsBcFilterAndRemarking_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == index), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == filterRule), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.index, index, sizeof(int));
    osal_memcpy(&cfg.filterRule, filterRule, sizeof(rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t));
    GETSOCKOPT(RTDRV_RG_GPONDSBCFILTERANDREMARKING_FIND, &cfg, rtdrv_rg_gponDsBcFilterAndRemarking_find_t, 1);
    osal_memcpy(index, &cfg.index, sizeof(int));
    osal_memcpy(filterRule, &cfg.filterRule, sizeof(rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_gponDsBcFilterAndRemarking_find */


/* Function Name:
 *      rtk_rg_gponDsBcFilterAndRemarking_del_all
 * Description:
 *      reflush egress remarking rules form GPON downstream Broadcast
 * Return:
 *      RT_ERR_RG_OK               - OK
 *      RT_ERR_RG_INVALID_PARAM                 - Invalid parameter .
 *      RT_ERR_RG_ENTRY_NOT_FOUND               - The entry not exist.
 * Note:
 *      None.
 */
int32
rtk_rg_gponDsBcFilterAndRemarking_del_all(void)
{
    rtdrv_rg_gponDsBcFilterAndRemarking_del_all_t cfg;

    /* function body */
    SETSOCKOPT(RTDRV_RG_GPONDSBCFILTERANDREMARKING_DEL_ALL, &cfg, rtdrv_rg_gponDsBcFilterAndRemarking_del_all_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_gponDsBcFilterAndRemarking_del_all */




/* Function Name:
 *      rtk_rg_naptFilterAndQos_add
 * Description:
 *      add napt filter rule for assigning napt-priority
 * Input:
 *        index - the index of the napt filter rule.
 *        napt_filter - the napt filter rule content.
 * Output:
 *        none
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32
rtk_rg_naptFilterAndQos_add(int *index,rtk_rg_naptFilterAndQos_t *napt_filter)
{
    rtdrv_rg_naptFilterAndQos_add_t cfg;
    /* function body */
    osal_memcpy(&cfg.index, index, sizeof(int));
    osal_memcpy(&cfg.napt_filter, napt_filter, sizeof(rtk_rg_naptFilterAndQos_t));
    GETSOCKOPT(RTDRV_RG_NAPTFILTERANDQOS_ADD, &cfg, rtdrv_rg_naptFilterAndQos_add_t, 1);
	osal_memcpy(napt_filter, &cfg.napt_filter, sizeof(rtk_rg_naptFilterAndQos_t));
    osal_memcpy(index, &cfg.index, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_naptFilterAndQos_set */

/* Function Name:
 *      rtk_rg_naptFilterAndQos_del
 * Description:
 *      del napt filter rule.
 * Input:
 *        index - the index of the napt filter rule.
 * Output:
 *        none
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32
rtk_rg_naptFilterAndQos_del(int index)
{
    rtdrv_rg_naptFilterAndQos_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    SETSOCKOPT(RTDRV_RG_NAPTFILTERANDQOS_DEL, &cfg, rtdrv_rg_naptFilterAndQos_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_naptFilterAndQos_del */

/* Function Name:
 *      rtk_rg_naptFilterAndQos_find
 * Description:
 *      get napt filter rule for assigning napt-priority
 * Input:
 *        index - the index of the napt filter rule.
 * Output:
 *        napt_filter - the napt filter rule content.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NOT_INIT - the rg has not been init. (rtk_rg_initParam_set() should be called first)
 *      RT_ERR_RG_INVALID_PARAM - the input parameter contains illegal information or range
 * Note:
 *      None
 */
int32
rtk_rg_naptFilterAndQos_find(int *index,rtk_rg_naptFilterAndQos_t *napt_filter)
{
    rtdrv_rg_naptFilterAndQos_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == napt_filter), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.index, index, sizeof(int));
    osal_memcpy(&cfg.napt_filter, napt_filter, sizeof(rtk_rg_naptFilterAndQos_t));
    GETSOCKOPT(RTDRV_RG_NAPTFILTERANDQOS_FIND, &cfg, rtdrv_rg_naptFilterAndQos_find_t, 1);
    osal_memcpy(napt_filter, &cfg.napt_filter, sizeof(rtk_rg_naptFilterAndQos_t));
	osal_memcpy(index, &cfg.index, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_naptFilterAndQos_get */

int32 rtk_rg_stpBlockingPortmask_set(rtk_rg_portmask_t Mask)
{
    rtdrv_rg_stpBlockingPortmask_set_t cfg;

    /* function body */    
    osal_memcpy(&cfg.Mask, &Mask, sizeof(rtk_rg_portmask_t));
    SETSOCKOPT(RTDRV_RG_STPBLOCKINGPORTMASK_SET, &cfg.Mask, rtdrv_rg_stpBlockingPortmask_set_t, 1);
    osal_memcpy(&Mask, &cfg.Mask, sizeof(rtk_rg_portmask_t));
    return RT_ERR_OK;
}   /* end of rtk_rg_stpBlockingPortmask_set */

int32 rtk_rg_stpBlockingPortmask_get(rtk_rg_portmask_t *pMask)
{
    rtdrv_rg_stpBlockingPortmask_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pMask), RT_ERR_NULL_POINTER);

    /* function body */
    GETSOCKOPT(RTDRV_RG_STPBLOCKINGPORTMASK_GET, &cfg.Mask, rtdrv_rg_stpBlockingPortmask_get_t, 1);
    osal_memcpy(pMask, &cfg.Mask, sizeof(rtk_rg_portmask_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_stpBlockingPortmask_get */



/* Function Name:
 *      rtk_rg_interfaceMibCounter_del
 * Description:
 *      clear per interface mib counter
 * Input:
 *        intf_idx      - interface index
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 *      RT_ERR_RG_FAILED
 * Note:
 *      None.
 */
int32
rtk_rg_interfaceMibCounter_del(int intf_idx)
{
    rtdrv_rg_interfaceMibCounter_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.intf_idx, &intf_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_INTERFACEMIBCOUNTER_DEL, &cfg, rtdrv_rg_interfaceMibCounter_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_interfaceMibCounter_del */

/* Function Name:
 *      rtk_rg_interfaceMibCounter_get
 * Description:
 *      get per interface mib counter
 * Output:
 *      pNetifMib - mib counter
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 *      RT_ERR_RG_FAILED
 * Note:
 *      pNetifMib->netifIdx should point to the target interface index.
 */
int32
rtk_rg_interfaceMibCounter_get(rtk_rg_netifMib_entry_t *pNetifMib)
{
    rtdrv_rg_interfaceMibCounter_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pNetifMib), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pNetifMib, pNetifMib, sizeof(rtk_rg_netifMib_entry_t));
    GETSOCKOPT(RTDRV_RG_INTERFACEMIBCOUNTER_GET, &cfg, rtdrv_rg_interfaceMibCounter_get_t, 1);
    osal_memcpy(pNetifMib, &cfg.pNetifMib, sizeof(rtk_rg_netifMib_entry_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_interfaceMibCounter_get */

/* Function Name:
 *      rtk_rg_redirectHttpAll_set
 * Description:
 *      setup the redirect parameters for all http request
 * Input:
 *	  pRedirectHttpAll - redirect http info
 * Return:
 *      RT_ERR_RG_OK 
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      pRedirectHttpAll->pushweb - http payload.<nl>
 *      pRedirectHttpAll->enable - enable http redirect or not.<nl>
 */
int32
rtk_rg_redirectHttpAll_set(rtk_rg_redirectHttpAll_t *pRedirectHttpAll)
{
    rtdrv_rg_redirectHttpAll_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpAll), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpAll, pRedirectHttpAll, sizeof(rtk_rg_redirectHttpAll_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPALL_SET, &cfg, rtdrv_rg_redirectHttpAll_set_t, 1);
    osal_memcpy(pRedirectHttpAll, &cfg.pRedirectHttpAll, sizeof(rtk_rg_redirectHttpAll_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpAll_set */


/* Function Name:
 *      rtk_rg_redirectHttpAll_get
 * Description:
 *      get current http redirect parameters
 * Output:
 *	  pRedirectHttpAll - redirect http info
 * Return:
 *      RT_ERR_RG_OK 
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      None.
 */
int32
rtk_rg_redirectHttpAll_get(rtk_rg_redirectHttpAll_t *pRedirectHttpAll)
{
    rtdrv_rg_redirectHttpAll_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpAll), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpAll, pRedirectHttpAll, sizeof(rtk_rg_redirectHttpAll_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPALL_GET, &cfg, rtdrv_rg_redirectHttpAll_get_t, 1);
    osal_memcpy(pRedirectHttpAll, &cfg.pRedirectHttpAll, sizeof(rtk_rg_redirectHttpAll_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpAll_get */


/* Function Name:
 *      rtk_rg_redirectHttpURL_add
 * Description:
 *      specify http URL which should be redirected
 * Input:
 *	  pRedirectHttpURL - adding redirect http URL info
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_FAILED
 * Note:
 *      pRedirectHttpURL->url_str - the string which need to be redirect in url string.(ex: in url "http://www.sample.com/explain", "www.sample.com" is the url string)<nl>
 *      pRedirectHttpURL->dst_url_str - the string which redirect to in url string.(ex: in url "http://www.sample.com/explain", "www.sample.com" is the url string)<nl>
 *      pRedirectHttpURL->count - how many times should such URL be redirected.<nl>
 */
int32
rtk_rg_redirectHttpURL_add(rtk_rg_redirectHttpURL_t *pRedirectHttpURL)
{
    rtdrv_rg_redirectHttpURL_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpURL), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpURL, pRedirectHttpURL, sizeof(rtk_rg_redirectHttpURL_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPURL_ADD, &cfg, rtdrv_rg_redirectHttpURL_add_t, 1);
    osal_memcpy(pRedirectHttpURL, &cfg.pRedirectHttpURL, sizeof(rtk_rg_redirectHttpURL_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpURL_add */


/* Function Name:
 *      rtk_rg_redirectHttpURL_del
 * Description:
 *      delete http URL which should be redirected
 * Input:
 *	  pRedirectHttpURL - deleting redirect http URL info
 * Return:
 *      RT_ERR_RG_OK 
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_ENTRY_NOT_EXIST
 * Note:
 *      None.
 */
int32
rtk_rg_redirectHttpURL_del(rtk_rg_redirectHttpURL_t *pRedirectHttpURL)
{
    rtdrv_rg_redirectHttpURL_del_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpURL), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpURL, pRedirectHttpURL, sizeof(rtk_rg_redirectHttpURL_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPURL_DEL, &cfg, rtdrv_rg_redirectHttpURL_del_t, 1);
    osal_memcpy(pRedirectHttpURL, &cfg.pRedirectHttpURL, sizeof(rtk_rg_redirectHttpURL_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpURL_del */


/* Function Name:
 *      rtk_rg_redirectHttpWhiteList_add
 * Description:
 *      specify white list URL and keyword which should not be redirected
 * Input:
 *	  pRedirectHttpWhiteList - adding redirect white list info
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_ENTRY_FULL
 * Note:
 *		pRedirectHttpWhiteList->url_str - the string which need to be redirect in url string.(ex: in url "http://www.sample.com/explain", "www.sample.com" is the url string)<nl>
 *		pRedirectHttpWhiteList->keyword_str - keyword appeared in URL.<nl>
 */
int32
rtk_rg_redirectHttpWhiteList_add(rtk_rg_redirectHttpWhiteList_t *pRedirectHttpWhiteList)
{
    rtdrv_rg_redirectHttpWhiteList_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpWhiteList), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpWhiteList, pRedirectHttpWhiteList, sizeof(rtk_rg_redirectHttpWhiteList_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPWHITELIST_ADD, &cfg, rtdrv_rg_redirectHttpWhiteList_add_t, 1);
    osal_memcpy(pRedirectHttpWhiteList, &cfg.pRedirectHttpWhiteList, sizeof(rtk_rg_redirectHttpWhiteList_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpWhiteList_add */


/* Function Name:
 *      rtk_rg_redirectHttpWhiteList_del
 * Description:
 *      setup the multicast entry for specified index
 * Input:
 *	  pRedirectHttpWhiteList - deleting redirect white list info
 * Return:
 *      RT_ERR_RG_OK 
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_ENTRY_NOT_EXIST
 * Note:
 *      None.
 */
int32
rtk_rg_redirectHttpWhiteList_del(rtk_rg_redirectHttpWhiteList_t *pRedirectHttpWhiteList)
{
    rtdrv_rg_redirectHttpWhiteList_del_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpWhiteList), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpWhiteList, pRedirectHttpWhiteList, sizeof(rtk_rg_redirectHttpWhiteList_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPWHITELIST_DEL, &cfg, rtdrv_rg_redirectHttpWhiteList_del_t, 1);
    osal_memcpy(pRedirectHttpWhiteList, &cfg.pRedirectHttpWhiteList, sizeof(rtk_rg_redirectHttpWhiteList_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpWhiteList_del */

/* Function Name:
 *      rtk_rg_redirectHttpRsp_set
 * Description:
 *      setup the redirect parameters for http response
 * Input:
 *        pRedirectHttpRsp - redirect http info
 * Return:
 *      RT_ERR_RG_OK 
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      pRedirectHttpRsp->url_str - the string which need to be redirect in url string.(ex: in url "http://www.sample.com/explain", "www.sample.com" is the url string)<nl>
 *      pRedirectHttpRsp->statusCode - http status code need to be redirected.<nl>
 *      pRedirectHttpRsp->enable - enable http redirect or not.<nl>
 */
int32
rtk_rg_redirectHttpRsp_set(rtk_rg_redirectHttpRsp_t *pRedirectHttpRsp)
{
    rtdrv_rg_redirectHttpRsp_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpRsp), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpRsp, pRedirectHttpRsp, sizeof(rtk_rg_redirectHttpRsp_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPRSP_SET, &cfg, rtdrv_rg_redirectHttpRsp_set_t, 1);
    osal_memcpy(pRedirectHttpRsp, &cfg.pRedirectHttpRsp, sizeof(rtk_rg_redirectHttpRsp_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpRsp_set */

/* Function Name:
 *      rtk_rg_redirectHttpRsp_get
 * Description:
 *      get current http response redirect parameters
 * Output:
 *        pRedirectHttpRsp - redirect http info
 * Return:
 *      RT_ERR_RG_OK 
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      None.
 */
int32
rtk_rg_redirectHttpRsp_get(rtk_rg_redirectHttpRsp_t *pRedirectHttpRsp)
{
    rtdrv_rg_redirectHttpRsp_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpRsp), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpRsp, pRedirectHttpRsp, sizeof(rtk_rg_redirectHttpRsp_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPRSP_GET, &cfg, rtdrv_rg_redirectHttpRsp_get_t, 1);
    osal_memcpy(pRedirectHttpRsp, &cfg.pRedirectHttpRsp, sizeof(rtk_rg_redirectHttpRsp_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpRsp_get */



/* Function Name:
 *      rtk_rg_svlanTpid2_enable_set
 * Description:
 *      setup the svlanTpid2 enable or disable
 * Input:
 *        enable - enable or disable
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      None.
 */
int32
rtk_rg_svlanTpid2_enable_set(rtk_rg_enable_t enable)
{
    rtdrv_rg_svlanTpid2_enable_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.enable, &enable, sizeof(rtk_rg_enable_t));
    SETSOCKOPT(RTDRV_RG_SVLANTPID2_ENABLE_SET, &cfg, rtdrv_rg_svlanTpid2_enable_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_svlanTpid2_enable_set */

/* Function Name:
 *      rtk_rg_svlanTpid2_enable_get
 * Description:
 *      get the svlanTpid2 enable or disable
 * Output:
 *        pEnable - enable or disable
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      None.
 */
int32
rtk_rg_svlanTpid2_enable_get(rtk_rg_enable_t *pEnable)
{
    rtdrv_rg_svlanTpid2_enable_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pEnable, pEnable, sizeof(rtk_rg_enable_t));
    GETSOCKOPT(RTDRV_RG_SVLANTPID2_ENABLE_GET, &cfg, rtdrv_rg_svlanTpid2_enable_get_t, 1);
    osal_memcpy(pEnable, &cfg.pEnable, sizeof(rtk_rg_enable_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_svlanTpid2_enable_get */

/* Function Name:
 *      rtk_rg_svlanTpid2_set
 * Description:
 *      setup the svlanTpid2 value
 * Input:
 *        svlan_tag_id - the tpid2 value
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      None.
 */
int32
rtk_rg_svlanTpid2_set(uint32 svlan_tag_id)
{
    rtdrv_rg_svlanTpid2_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.svlan_tag_id, &svlan_tag_id, sizeof(uint32));
    SETSOCKOPT(RTDRV_RG_SVLANTPID2_SET, &cfg, rtdrv_rg_svlanTpid2_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_svlanTpid2_set */

/* Function Name:
 *      rtk_rg_svlanTpid2_get
 * Description:
 *      get the svlanTpid2 value
 * Output:
 *        pSvlanTagId - the tpid2 value
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      None.
 */
int32
rtk_rg_svlanTpid2_get(uint32 *pSvlanTagId)
{
    rtdrv_rg_svlanTpid2_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pSvlanTagId), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pSvlanTagId, pSvlanTagId, sizeof(uint32));
    GETSOCKOPT(RTDRV_RG_SVLANTPID2_GET, &cfg, rtdrv_rg_svlanTpid2_get_t, 1);
    osal_memcpy(pSvlanTagId, &cfg.pSvlanTagId, sizeof(uint32));

    return RT_ERR_OK;
}   /* end of rtk_rg_svlanTpid2_get */


/* Function Name:
 *      rtk_rg_hostPoliceControl_set
 * Description:
 *      specify which MAC address should be metering and logging
 * Input:
 *	  pHostPoliceControl - content of how the host be metering or logging.<nl>
 *      pHost_idx - returning the host index in system.<nl>
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 *      RT_ERR_RG_FAILED
 * Note:
 *	  pHostPoliceControl.macAddr - MAC address of host.<nl>
 *	  pHostPoliceControl.ingressLimitCtrl - control ingress limit on or off.<nl>
 *	  pHostPoliceControl.egressLimitCtrl - control egress limit on or off.<nl> 
 *	  pHostPoliceControl.mibCountCtrl - control mib count on or off.<nl> 
 *	  pHostPoliceControl.limitMeterIdx - bandwidth limit meter index.<nl>
 */
int32
rtk_rg_hostPoliceControl_set(rtk_rg_hostPoliceControl_t *pHostPoliceControl, int host_idx)
{
    rtdrv_rg_hostPoliceControl_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pHostPoliceControl), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pHostPoliceControl, pHostPoliceControl, sizeof(rtk_rg_hostPoliceControl_t));
    osal_memcpy(&cfg.host_idx, &host_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_HOSTPOLICECONTROL_SET, &cfg, rtdrv_rg_hostPoliceControl_set_t, 1);
    osal_memcpy(pHostPoliceControl, &cfg.pHostPoliceControl, sizeof(rtk_rg_hostPoliceControl_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_hostPoliceControl_set */


/* Function Name:
 *      rtk_rg_hostPoliceControl_get
 * Description:
 *      get the hostPoliceControl entry
 * Input:
 *	  host_idx - the host index.<nl>
 * Output:
 *	  pHostPoliceControl - returning host police and meter info.<nl>
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 * Note:
 *      None.
 */
int32
rtk_rg_hostPoliceControl_get(rtk_rg_hostPoliceControl_t *pHostPoliceControl, int host_idx)
{
    rtdrv_rg_hostPoliceControl_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pHostPoliceControl), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pHostPoliceControl, pHostPoliceControl, sizeof(rtk_rg_hostPoliceControl_t));
    osal_memcpy(&cfg.host_idx, &host_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_HOSTPOLICECONTROL_GET, &cfg, rtdrv_rg_hostPoliceControl_get_t, 1);
    osal_memcpy(pHostPoliceControl, &cfg.pHostPoliceControl, sizeof(rtk_rg_hostPoliceControl_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_hostPoliceControl_get */


/* Function Name:
 *      rtk_rg_hostPoliceLogging_get
 * Description:
 *      get per host mib count added by hostPoliceControl
 * Input:
 *	  host_idx - the host index.<nl>
 * Output:
 *      pHostMibCnt - host mib counter.<nl>
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 * Note:
 *      None.
 */
int32
rtk_rg_hostPoliceLogging_get(rtk_rg_hostPoliceLogging_t *pHostMibCnt, int host_idx)
{
    rtdrv_rg_hostPoliceLogging_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pHostMibCnt), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pHostMibCnt, pHostMibCnt, sizeof(rtk_rg_hostPoliceLogging_t));
    osal_memcpy(&cfg.host_idx, &host_idx, sizeof(int));
    GETSOCKOPT(RTDRV_RG_HOSTPOLICELOGGING_GET, &cfg, rtdrv_rg_hostPoliceLogging_get_t, 1);
    osal_memcpy(pHostMibCnt, &cfg.pHostMibCnt, sizeof(rtk_rg_hostPoliceLogging_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_hostPoliceLogging_get */


/* Function Name:
 *      rtk_rg_hostPoliceLogging_del
 * Description:
 *      clear per host mib count added by hostPoliceControl
 * Input:
 *	  host_idx - host index.<nl>
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 * Note:
 *      None.
 */
int32
rtk_rg_hostPoliceLogging_del(int host_idx)
{
    rtdrv_rg_hostPoliceLogging_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.host_idx, &host_idx, sizeof(int));
    SETSOCKOPT(RTDRV_RG_HOSTPOLICELOGGING_DEL, &cfg, rtdrv_rg_hostPoliceLogging_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_hostPoliceLogging_del */

/* Function Name:
 *      rtk_rg_redirectHttpCount_set
 * Description:
 *      setup the redirect parameters for http request
 * Input:
 *        pRedirectHttpCount - redirect http info
 * Return:
 *      RT_ERR_RG_OK 
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      pRedirectHttpCount->pushweb - http payload.<nl>
 *      pRedirectHttpCount->enable - enable http redirect or not.<nl>
 *      pRedirectHttpCount->count - redirect times.<nl>
 */
int32
rtk_rg_redirectHttpCount_set(rtk_rg_redirectHttpCount_t *pRedirectHttpCount)
{
    rtdrv_rg_redirectHttpCount_set_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpCount), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpCount, pRedirectHttpCount, sizeof(rtk_rg_redirectHttpCount_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPCOUNT_SET, &cfg, rtdrv_rg_redirectHttpCount_set_t, 1);
    osal_memcpy(pRedirectHttpCount, &cfg.pRedirectHttpCount, sizeof(rtk_rg_redirectHttpCount_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpCount_set */

/* Function Name:
 *      rtk_rg_redirectHttpCount_get
 * Description:
 *      get current http redirect parameters
 * Output:
 *        pRedirectHttpCount - redirect http info
 * Return:
 *      RT_ERR_RG_OK 
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      None.
 */
int32
rtk_rg_redirectHttpCount_get(rtk_rg_redirectHttpCount_t *pRedirectHttpCount)
{
    rtdrv_rg_redirectHttpCount_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pRedirectHttpCount), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pRedirectHttpCount, pRedirectHttpCount, sizeof(rtk_rg_redirectHttpCount_t));
    GETSOCKOPT(RTDRV_RG_REDIRECTHTTPCOUNT_GET, &cfg, rtdrv_rg_redirectHttpCount_get_t, 1);
    osal_memcpy(pRedirectHttpCount, &cfg.pRedirectHttpCount, sizeof(rtk_rg_redirectHttpCount_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_redirectHttpCount_get */

/* Function Name:
 *      rtk_rg_staticRoute_add
 * Description:
 *      create static route
 * Input:
 *      pStaticRoute    - static route information.
 * Output:
 *      index	   - the index of static route list
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_ENTRY_FULL
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_FAILED 
 *      RT_ERR_RG_SR_TO_CPU
 *      RT_ERR_RG_SR_TO_DROP
 *      RT_ERR_RG_SR_MAC_FAILED
 * Note:
 *      None.
 */
int32 
rtk_rg_staticRoute_add(rtk_rg_staticRoute_t *pStaticRoute, int *index)
{
    rtdrv_rg_staticRoute_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pStaticRoute), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == index), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pStaticRoute, pStaticRoute, sizeof(rtk_rg_staticRoute_t));
    osal_memcpy(&cfg.index, index, sizeof(int));
    GETSOCKOPT(RTDRV_RG_STATICROUTE_ADD, &cfg, rtdrv_rg_staticRoute_add_t, 1);
    osal_memcpy(pStaticRoute, &cfg.pStaticRoute, sizeof(rtk_rg_staticRoute_t));
    osal_memcpy(index, &cfg.index, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_staticRoute_add */

/* Function Name:
 *      rtk_rg_staticRoute_del
 * Description:
 *      Delete static route by index
 * Input:
 *      index    - the index of static route list
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_ENTRY_NOT_FOUND
 *      RT_ERR_RG_FAILED 
 * Note:
 *      None.
 */
int32
rtk_rg_staticRoute_del(int index)
{
    rtdrv_rg_staticRoute_del_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    SETSOCKOPT(RTDRV_RG_STATICROUTE_DEL, &cfg, rtdrv_rg_staticRoute_del_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_staticRoute_del */

/* Function Name:
 *      rtk_rg_staticRoute_find
 * Description:
 *      Get a pStaticRoute Entry from index.
 * Input: 		
 *      index	   - the index of static route list
 * Output:
 *      pStaticRoute    - static route information.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_ENTRY_NOT_FOUND
 *      RT_ERR_RG_FAILED 
 * Note:
 *      None.
 */
int32 
rtk_rg_staticRoute_find(rtk_rg_staticRoute_t *pStaticRoute, int *index)
{
    rtdrv_rg_staticRoute_find_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pStaticRoute), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == index), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pStaticRoute, pStaticRoute, sizeof(rtk_rg_staticRoute_t));
    osal_memcpy(&cfg.index, index, sizeof(int));
    GETSOCKOPT(RTDRV_RG_STATICROUTE_FIND, &cfg, rtdrv_rg_staticRoute_find_t, 1);
    osal_memcpy(pStaticRoute, &cfg.pStaticRoute, sizeof(rtk_rg_staticRoute_t));
    osal_memcpy(index, &cfg.index, sizeof(int));

    return RT_ERR_OK;
}   /* end of rtk_rg_staticRoute_find */

/* Function Name:
 *      rtk_rg_aclLogCounterControl_get
 * Description:
 *      Get a log counter's status
 * Input: 		
 *      index		- the index of the log counter
 * Output:
 *		ctrl		- counting type, byte count (1) or packet count (0). couting mode, 32bit mode (0) or 64bit mode (1)
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_ENTRY_NOT_FOUND
 *      RT_ERR_RG_FAILED 
 * Note:
 *      None.
 */
int32 
rtk_rg_aclLogCounterControl_get(int index, int *type, int *mode)
{
    rtdrv_rg_aclLogCounterControl_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == type), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == mode), RT_ERR_NULL_POINTER);
    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    GETSOCKOPT(RTDRV_RG_ACLLOGCOUNTERCONTROL_GET, &cfg, rtdrv_rg_aclLogCounterControl_get_t, 1);
    osal_memcpy(type, &cfg.type, sizeof(int));
    osal_memcpy(mode, &cfg.mode, sizeof(int));


    return RT_ERR_OK;
}   /* end of rtk_rg_aclLogCounterControl_get */

/* Function Name:
 *      rtk_rg_aclLogCounterControl_set
 * Description:
 *      config a log counter by given type and mode
 * Input: 		
 *      index		- the index of log counter
 *		ctrl		- counting type, byte count (1) or packet count (0). couting mode, 32bit mode (0) or 64bit mode (1)
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_ENTRY_NOT_FOUND
 *      RT_ERR_RG_FAILED 
 * Note:
 *      None.
 */
int32 
rtk_rg_aclLogCounterControl_set(int index, int type, int mode)
{
    rtdrv_rg_aclLogCounterControl_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    osal_memcpy(&cfg.type, &type, sizeof(int));
    osal_memcpy(&cfg.mode, &mode, sizeof(int));
	
    SETSOCKOPT(RTDRV_RG_ACLLOGCOUNTERCONTROL_SET, &cfg, rtdrv_rg_aclLogCounterControl_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_aclLogCounterControl_set */

/* Function Name:
 *      rtk_rg_aclLogCounter_get
 * Description:
 *      get acl log counter 
 * Input: 		
 *      index		- the index of log counter
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_FAILED 
 * Note:
 *      None.
 */
int32 
rtk_rg_aclLogCounter_get(int index, uint64 *count)
{
    rtdrv_rg_aclLogCounter_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == count), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    osal_memcpy(&cfg.count, count, sizeof(uint64));
    GETSOCKOPT(RTDRV_RG_ACLLOGCOUNTER_GET, &cfg, rtdrv_rg_aclLogCounter_get_t, 1);
    osal_memcpy(count, &cfg.count, sizeof(uint64));

    return RT_ERR_OK;
}   /* end of rtk_rg_aclLogCounter_get */

/* Function Name:
 *      rtk_rg_aclLogCounter_reset
 * Description:
 *      reset acl log counter
 * Input: 		
 *      index		- the index of log counter
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_FAILED 
 * Note:
 *      None.
 */
int32 
rtk_rg_aclLogCounter_reset(int index)
{
    rtdrv_rg_aclLogCounter_reset_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    SETSOCKOPT(RTDRV_RG_ACLLOGCOUNTER_RESET, &cfg, rtdrv_rg_aclLogCounter_reset_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_aclLogCounter_reset */

/* Function Name:
 *      rtk_rg_groupMacLimit_set
 * Description:
 *      Set group MAC limit when using software learning.
 * Input:
 * Output:
 *      rtk_rg_groupMacLimit_t - [in]<tab>The information entered.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 * Note:
 *      group_mac_info.port_mask - the group of port for limitation.<nl>
 *      group_mac_info.learningCount - the learning count.<nl>
 *      group_mac_info.learningLimitNumber - the maximum number can be learned.<nl>
 *      group_mac_info.wlan0_dev_mask - which wlan0 device should be limit.<nl>
 */
int32
rtk_rg_groupMacLimit_set(rtk_rg_groupMacLimit_t group_mac_info)
{
    rtdrv_rg_groupMacLimit_set_t cfg;

    /* function body */
    osal_memcpy(&cfg.group_mac_info, &group_mac_info, sizeof(rtk_rg_groupMacLimit_t));
    SETSOCKOPT(RTDRV_RG_GROUPMACLIMIT_SET, &cfg, rtdrv_rg_groupMacLimit_set_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_groupMacLimit_set */


/* Function Name:
 *      rtk_rg_groupMacLimit_get
 * Description:
 *      Get group MAC limit when using software learning.
 * Input:
 *      rtk_rg_groupMacLimit_t - [in]<tab>The information entered.
 * Output:
 *      rtk_rg_groupMacLimit_t - [out]<tab>The information contained.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      pGroup_mac_info->learningCount - the learning count.<nl>
 *      The portmask, limit and count will be returned by pointer.<nl>
 */
int32
rtk_rg_groupMacLimit_get(rtk_rg_groupMacLimit_t *pGroup_mac_info)
{
    rtdrv_rg_groupMacLimit_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pGroup_mac_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.pGroup_mac_info, pGroup_mac_info, sizeof(rtk_rg_groupMacLimit_t));
    GETSOCKOPT(RTDRV_RG_GROUPMACLIMIT_GET, &cfg, rtdrv_rg_groupMacLimit_get_t, 1);
    osal_memcpy(pGroup_mac_info, &cfg.pGroup_mac_info, sizeof(rtk_rg_groupMacLimit_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_groupMacLimit_get */

/* Function Name:
 *      rtk_rg_callback_function_ptr_get
 * Description:
 *      Get callback function pointer for user space.
 * Input:
 *      rtk_rg_callbackFunctionPtrGet_t - [in]<tab>The information entered.
 * Output:
 *      rtk_rg_callbackFunctionPtrGet_t - [out]<tab>The information contained.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      callback_function_ptr_get_info->callback_function_idx - the index of callback function
 *      callback_function_ptr_get_info->callback_function_pointer - the function pointer of callback function
 */
int32
rtk_rg_callback_function_ptr_get(rtk_rg_callbackFunctionPtrGet_t *callback_function_ptr_get_info)
{
    rtdrv_rg_callback_function_ptr_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == callback_function_ptr_get_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.callback_func_ptr_info, callback_function_ptr_get_info, sizeof(rtk_rg_callbackFunctionPtrGet_t));
    GETSOCKOPT(RTDRV_RG_CALLBACK_FUNCTION_PTR_GET, &cfg, rtdrv_rg_callback_function_ptr_get_t, 1);
    osal_memcpy(callback_function_ptr_get_info, &cfg.callback_func_ptr_info, sizeof(rtk_rg_callbackFunctionPtrGet_t));

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_callback_function_ptr_get
 * Description:
 *      Get callback function pointer for user space.
 * Input:
 *      rtk_rg_callbackFunctionPtrGet_t - [in]<tab>The information entered.
 * Output:
 *      rtk_rg_callbackFunctionPtrGet_t - [out]<tab>The information contained.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      mac_filter_whitelist_info->mac - the MAC address added to white list
 */
int32
rtk_rg_mac_filter_whitelist_add(rtk_rg_macFilterWhiteList_t *mac_filter_whitelist_info)
{
    rtdrv_rg_mac_filter_whitelist_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == mac_filter_whitelist_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.mac_filter_info, mac_filter_whitelist_info, sizeof(rtk_rg_macFilterWhiteList_t));
    GETSOCKOPT(RTDRV_RG_MACFILTER_WHITE_LIST_ADD, &cfg, rtk_rg_macFilterWhiteList_t, 1);
    osal_memcpy(mac_filter_whitelist_info, &cfg.mac_filter_info, sizeof(rtk_rg_macFilterWhiteList_t));

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rg_callback_function_ptr_get
 * Description:
 *      Get callback function pointer for user space.
 * Input:
 *      rtk_rg_callbackFunctionPtrGet_t - [in]<tab>The information entered.
 * Output:
 *      rtk_rg_callbackFunctionPtrGet_t - [out]<tab>The information contained.
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 * Note:
 *      mac_filter_whitelist_info->mac - the MAC address removed to white list
 *      mac_filter_whitelist_info->del_flag - determine remove ONE or ALL entry from white list
 */
int32
rtk_rg_mac_filter_whitelist_del(rtk_rg_macFilterWhiteList_t *mac_filter_whitelist_info)
{
    rtdrv_rg_mac_filter_whitelist_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == mac_filter_whitelist_info), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.mac_filter_info, mac_filter_whitelist_info, sizeof(rtk_rg_macFilterWhiteList_t));
    GETSOCKOPT(RTDRV_RG_MACFILTER_WHITE_LIST_DEL, &cfg, rtk_rg_macFilterWhiteList_t, 1);
    osal_memcpy(mac_filter_whitelist_info, &cfg.mac_filter_info, sizeof(rtk_rg_macFilterWhiteList_t));

    return RT_ERR_OK;
}


int32 
rtk_rg_igmpMldSnoopingControl_set(rtk_rg_igmpMldSnoopingControl_t *config )
{
    rtdrv_rg_igmp_mld_control_t cfg;

    /* function body */
    osal_memcpy(&cfg.igmpMld_info, config, sizeof(cfg.igmpMld_info));
    SETSOCKOPT(RTDRV_RG_IGMPMLD_CONTROL_SET, &cfg, rtdrv_rg_igmp_mld_control_t, 1);

    return RT_ERR_OK;
}  

int32 
rtk_rg_igmpMldSnoopingControl_get(rtk_rg_igmpMldSnoopingControl_t *config )
{
    rtdrv_rg_igmp_mld_control_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == config), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.igmpMld_info, config, sizeof(cfg.igmpMld_info));
    GETSOCKOPT(RTDRV_RG_IGMPMLD_CONTROL_GET, &cfg, rtdrv_rg_igmp_mld_control_t, 1);
    osal_memcpy(config, &cfg.igmpMld_info, sizeof(cfg.igmpMld_info));

    return RT_ERR_OK;
}  

int32 
rtk_rg_igmpMldSnoopingPortControl_add(rtk_rg_port_idx_t port_idx,rtk_rg_igmpMldSnoopingPortControl_t *config )
{
    rtdrv_rg_igmp_mld_port_control_add_t cfg;

    /* function body */
    osal_memcpy(&cfg.igmpMld_port_info, config, sizeof(cfg.igmpMld_port_info));
	osal_memcpy(&cfg.port_idx, &port_idx, sizeof(cfg.port_idx));
    SETSOCKOPT(RTDRV_RG_IGMPMLD_PORT_CONTROL_ADD, &cfg, rtdrv_rg_igmp_mld_port_control_add_t, 1);

    return RT_ERR_OK;
}  


int32 
rtk_rg_igmpMldSnoopingPortControl_del(rtk_rg_port_idx_t port_idx )
{
    rtdrv_rg_igmp_mld_port_control_del_t cfg;

    /* function body */
	osal_memcpy(&cfg.port_idx, &port_idx, sizeof(cfg.port_idx));
    SETSOCKOPT(RTDRV_RG_IGMPMLD_PORT_CONTROL_DEL, &cfg, rtdrv_rg_igmp_mld_port_control_del_t, 1);

    return RT_ERR_OK;
}  

int32 
rtk_rg_igmpMldSnoopingPortControl_find(rtk_rg_port_idx_t port_idx,rtk_rg_igmpMldSnoopingPortControl_t *config )
{
    rtdrv_rg_igmp_mld_port_control_add_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == config), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.igmpMld_port_info, config, sizeof(cfg.igmpMld_port_info));
	osal_memcpy(&cfg.port_idx, &port_idx, sizeof(cfg.port_idx));
    GETSOCKOPT(RTDRV_RG_IGMPMLD_PORT_CONTROL_FIND, &cfg, rtdrv_rg_igmp_mld_control_t, 1);
    osal_memcpy(config, &cfg.igmpMld_port_info, sizeof(cfg.igmpMld_port_info));

    return RT_ERR_OK;
}  


/* Function Name:
 *      rtk_rg_flowMibCounter_get
 * Description:
 *      get flow group counter 
 * Input:               
 *      index           - the index of flow group counter
 * Output:
 *      pCounter        - packet count and byte count
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_FAILED 
 * Note:
 *      support 32 entries.
 */
int32 
rtk_rg_flowMibCounter_get(int index, rtk_rg_table_flowmib_t *pCounter)
{
    rtdrv_rg_flowMibCounter_get_t cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pCounter), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    osal_memcpy(&cfg.counter, pCounter, sizeof(rtk_rg_table_flowmib_t));
    GETSOCKOPT(RTDRV_RG_FLOWMIBCOUNTER_GET, &cfg, rtdrv_rg_flowMibCounter_get_t, 1);
    osal_memcpy(pCounter, &cfg.counter, sizeof(rtk_rg_table_flowmib_t));

    return RT_ERR_OK;
}   /* end of rtk_rg_flowMibCounter_get */

/* Function Name:
 *      rtk_rg_flowMibCounter_reset
 * Description:
 *      reset flow group counter
 * Input:               
 *      index           - the index of flow group counter
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_FAILED 
 * Note:
 *      support 32 entries.
 */
int32 
rtk_rg_flowMibCounter_reset(int index)
{
    rtdrv_rg_flowMibCounter_reset_t cfg;

    /* function body */
    osal_memcpy(&cfg.index, &index, sizeof(int));
    SETSOCKOPT(RTDRV_RG_FLOWMIBCOUNTER_RESET, &cfg, rtdrv_rg_flowMibCounter_reset_t, 1);

    return RT_ERR_OK;
}   /* end of rtk_rg_flowMibCounter_reset */

/* Function Name:
 *      rtk_rg_funcbasedMeter_set
 * Description:
 *      set funcbased meter
 * Input:
 *      meterConf               - the funcbased meter sonfiguration
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK;
 *       RT_ERR_RG_FAILED
 *       RT_ERR_RG_NULL_POINTER
 *       RT_ERR_RG_INVALID_PARAM
 *       RT_ERR_RG_INDEX_OUT_OF_RANGE
 *       RT_ERR_RG_SHAREMETER_SET_FAILED
 *       RT_ERR_RG_SHAREMETER_GET_FAILED
 *       RT_ERR_RG_SHAREMETER_INVALID_RATE
 *       RT_ERR_RG_SHAREMETER_INVALID_INPUT
 *       RT_ERR_RG_SHAREMETER_INVALID_METER_INDEX
 * Note:
 *      support 32 entries of each funcbased meter type.
 */
int32
rtk_rg_funcbasedMeter_set(rtk_rg_funcbasedMeterConf_t meterConf)
{
	rtdrv_rg_funcbasedMeter_set_t cfg;

	/* function body */
	osal_memcpy(&cfg.meterConf, &meterConf, sizeof(rtk_rg_funcbasedMeterConf_t));
	SETSOCKOPT(RTDRV_RG_FUNCBASEDMETER_SET, &cfg, rtdrv_rg_funcbasedMeter_set_t, 1);

	return RT_ERR_OK;
}	/* end of rtk_rg_funcbasedMeter_set */

/* Function Name:
 *      rtk_rg_funcbasedMeter_get
 * Description:
 *      get funcbased meter
 * Input:
 *      meterConf               - the funcbased meter sonfiguration
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 *      RT_ERR_RG_SHAREMETER_GET_FAILED
 *      RT_ERR_RG_SHAREMETER_INVALID_METER_INDEX
 * Note:
 *      support 32 entries of each funcbased meter type.
 */
int32 
rtk_rg_funcbasedMeter_get(rtk_rg_funcbasedMeterConf_t *meterConf)
{
	rtdrv_rg_funcbasedMeter_get_t cfg;

	/* parameter check */
	RT_PARAM_CHK((NULL == meterConf), RT_ERR_NULL_POINTER);

	/* function body */
	osal_memcpy(&cfg.meterConf, meterConf, sizeof(rtk_rg_funcbasedMeterConf_t));
	GETSOCKOPT(RTDRV_RG_FUNCBASEDMETER_GET, &cfg, rtdrv_rg_funcbasedMeter_get_t, 1);
	osal_memcpy(meterConf, &cfg.meterConf, sizeof(rtk_rg_funcbasedMeterConf_t));

	return RT_ERR_OK;
}	/* end of rtk_rg_funcbasedMeter_get */



/* Function Name:
 *      rtk_rg_funcbasedMeter_set
 * Description:
 *      add high priority entry
 * Input:
 *      hiPriEntry               - the funcbased meter sonfiguration
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK;
 *       RT_ERR_RG_FAILED
 *       RT_ERR_RG_NULL_POINTER
 *       RT_ERR_RG_INVALID_PARAM
 *       RT_ERR_RG_INDEX_OUT_OF_RANGE
 * Note:
 *      support 8 entries of each funcbased meter type.
 */

int32
rtk_rg_flowHiPriEntry_add(rtk_rg_table_highPriPatten_t hiPriEntry,int *entry_idx)
{
	rtdrv_rg_hiPriEntry_add_t cfg;

	/* function body */
	osal_memcpy(&cfg.hiPriEntry, &hiPriEntry, sizeof(rtk_rg_table_highPriPatten_t));
	osal_memcpy(&cfg.index, entry_idx, sizeof(int));
	GETSOCKOPT(RTDRV_RG_FLOWHIGHTPRIENTRY_ADD, &cfg, rtdrv_rg_hiPriEntry_add_t, 1);
	osal_memcpy(entry_idx, &cfg.index, sizeof(int));
	osal_memcpy(&hiPriEntry, &cfg.hiPriEntry, sizeof(rtk_rg_table_highPriPatten_t));

	return RT_ERR_OK;
}	/* end of rtk_rg_flowHiPriEntry_add */

/* Function Name:
 *      rtk_rg_flowHiPriEntry_del
 * Description:
 *      delete High priority entry
 * Input:
 *      hiPriEntry               - the funcbased meter sonfiguration
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_NULL_POINTER
 *      RT_ERR_RG_INVALID_PARAM
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 * Note:
 *      support 8 entries of each funcbased meter type.
 */
int32 
rtk_rg_flowHiPriEntry_del(int entry_idx)
{
	rtdrv_rg_hiPriEntry_del_t cfg;

	/* function body */
	osal_memcpy(&cfg.index, &entry_idx, sizeof(int));
	GETSOCKOPT(RTDRV_RG_FLOWHIGHTPRIENTRY_DEL, &cfg, rtdrv_rg_hiPriEntry_del_t, 1);

	return RT_ERR_OK;
}	/* end of rtk_rg_flowHiPriEntry_get */


