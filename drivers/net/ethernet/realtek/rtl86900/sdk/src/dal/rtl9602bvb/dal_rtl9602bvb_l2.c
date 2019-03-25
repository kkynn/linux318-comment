/*
 * Copyright (C) 2014 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision:  $
 * $Date: $
 *
 * Purpose : Definition of L2 API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) Mac address flush
 *           (2) Address learning limit
 *           (3) Parameter for L2 lookup and learning engine
 *           (4) Unicast address
 *           (5) L2 multicast
 *           (6) IP multicast
 *           (7) Multicast forwarding table
 *           (8) CPU mac
 *           (9) Port move
 *           (10) Parameter for lookup miss
 *           (11) Parameter for MISC
 *
 */



/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/port.h>
#include <dal/rtl9602bvb/dal_rtl9602bvb.h>
#include <dal/rtl9602bvb/dal_rtl9602bvb_l2.h>
#include <osal/time.h>

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */

static uint32    l2_init = INIT_NOT_COMPLETED;

/*
 * Macro Declaration
 */


/*
 * Function Declaration
 */
/* Function Name:
 *      rtl9602bvb_raw_l2_lookUpTb_set
 * Description:
 *      Set filtering database entry
 * Input:
 *      pL2Table 	- L2 table entry writing to 1K+64 filtering database
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER
 *      RT_ERR_OUT_OF_RANGE
 * Note:
 *      None
 */
int32 rtl9602bvb_raw_l2_lookUpTb_set(rtl9602bvb_lut_table_t *pL2Table)
{
   int32 ret = RT_ERR_FAILED;
   uint32 state;

    RT_PARAM_CHK(NULL == pL2Table, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(pL2Table->table_type > RTL9602BVB_RAW_LUT_ENTRY_TYPE_END, RT_ERR_OUT_OF_RANGE);

    if ((ret = reg_field_read(RTL9602BVB_LUT_CFGr, RTL9602BVB_BCAM_DISf, &state)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if(0 == state)
        RT_PARAM_CHK(pL2Table->address > (RTL9602BVB_LUT_4WAY_NO + RTL9602BVB_LUT_CAM_NO), RT_ERR_INPUT);
    else
        RT_PARAM_CHK(pL2Table->address > RTL9602BVB_LUT_4WAY_NO, RT_ERR_INPUT);

#ifndef CONFIG_SDK_ASICDRV_TEST
    RT_PARAM_CHK(RTL9602BVB_RAW_LUT_READ_METHOD_ADDRESS == pL2Table->method, RT_ERR_INPUT);
#endif
    /*commna part check*/
    RT_PARAM_CHK(pL2Table->l3lookup > 1, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->nosalearn > 1, RT_ERR_OUT_OF_RANGE);

    /*L2 check*/
    RT_PARAM_CHK(pL2Table->cvid_fid > RTK_VLAN_ID_MAX, RT_ERR_OUT_OF_RANGE);

    /*l2 uc check*/
    RT_PARAM_CHK(pL2Table->fid > HAL_VLAN_FID_MAX(), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->ctag_if > 1, RT_ERR_OUT_OF_RANGE);
	RT_PARAM_CHK(!HAL_IS_PORT_EXIST(pL2Table->spa), RT_ERR_PORT_ID);
    RT_PARAM_CHK(pL2Table->age > RTL9602BVB_RAW_LUT_AGEMAX, RT_ERR_OUT_OF_RANGE);
	RT_PARAM_CHK(pL2Table->auth > 1, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->sa_block > 1, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->da_block > 1, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->arp_used > 1, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->ext_spa > RTL9602BVB_RAW_LUT_EXT_SPAMAX, RT_ERR_OUT_OF_RANGE);

    /*----L3----*/
    RT_PARAM_CHK(pL2Table->ip6 > 1, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->wan_sa > 1, RT_ERR_OUT_OF_RANGE);

    /* ---L3 MC ROUTE---*/
    RT_PARAM_CHK(pL2Table->l3_idx > RTL9602BVB_RAW_LUT_L3IDXAMAX, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->sip_idx > RTL9602BVB_RAW_LUT_SIPIDXMAX, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->sip_filter > 1, RT_ERR_OUT_OF_RANGE);

    /* ---L3v6 MC---*/
    RT_PARAM_CHK(pL2Table->l3mcr_idx > RTL9602BVB_RAW_LUT_L3MCRIDXMAX, RT_ERR_OUT_OF_RANGE);

    /*(L2 MC)(L3 MC ROUTE)(L3 IP6MC)*/
    RT_PARAM_CHK(pL2Table->mbr > RTL9602BVB_PORTMASK, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pL2Table->ext_mbr > RTL9602BVB_RAW_LUT_EXTMBR, RT_ERR_OUT_OF_RANGE);

    /*static entry will assign age to none-ZERO*/
    if(pL2Table->nosalearn && pL2Table->table_type == RTL9602BVB_RAW_LUT_ENTRY_TYPE_L2UC)
        pL2Table->age = 1;

    /*write to l2 table*/
    if ((ret = table_write(RTL9602BVB_L2_UCt, pL2Table->address, (uint32 *)pL2Table)) != RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return RT_ERR_FAILED;
    }
    return RT_ERR_OK;
}

/* Function Name:
 *      rtl9602bvb_raw_l2_lookUpTb_get
 * Description:
 *      Get filtering database entry
 * Input:
 *      method      -  Lut access method
 * Output:
 *      pL2Table 	- L2 table entry writing to 1K+64 filtering database
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT
 *      RT_ERR_NULL_POINTER
 * Note:
 *      None
 */
int32 rtl9602bvb_raw_l2_lookUpTb_get(rtl9602bvb_lut_table_t *pL2Table)
{
    int32 ret = RT_ERR_FAILED;
    uint32 state;

    RT_PARAM_CHK(pL2Table == NULL, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(pL2Table->method > RTL9602BVB_RAW_LUT_READ_METHOD_END, RT_ERR_INPUT);

    if ((ret = reg_field_read(RTL9602BVB_LUT_CFGr, RTL9602BVB_BCAM_DISf, &state)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if(0 == state)
        RT_PARAM_CHK(pL2Table->address > (RTL9602BVB_LUT_4WAY_NO + RTL9602BVB_LUT_CAM_NO), RT_ERR_INPUT);
    else
        RT_PARAM_CHK(pL2Table->address > RTL9602BVB_LUT_4WAY_NO, RT_ERR_INPUT);


    if ((ret = table_read(RTL9602BVB_L2_UCt, pL2Table->address, (uint32 *)pL2Table)) != RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_L2), "return failed ret value = %x",ret);
        return RT_ERR_FAILED;
    }

	return RT_ERR_OK;
}

/* Function Name:
 *      rtl9602bvb_raw_l2_flushEn_set
 * Description:
 *      Set per port force flush setting
 * Input:
 *      port   		- port id
 *      enable 		- enable status
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_PORT_ID
 * Note:
 *      None
 */
int32 rtl9602bvb_raw_l2_flushEn_set(rtk_port_t port, rtk_enable_t enabled)
{
    int32 ret;
    rtl9602bvb_raw_l2_flushStatus_t status;
    uint32 cnt=0;

    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
	RT_PARAM_CHK((RTK_ENABLE_END <= enabled), RT_ERR_INPUT);

 	if ((ret = reg_array_field_write(RTL9602BVB_L2_TBL_FLUSH_ENr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &enabled)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    /* wait the flush status for non-busy */
    if ((ret = reg_field_read(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_FLUSH_STATUSf ,&status)) != RT_ERR_OK )
    {
            RT_ERR(ret, (MOD_L2|MOD_DAL), "");
            return ret;
    }
    while (status != RTL9602BVB_RAW_FLUSH_STATUS_NONBUSY)
    {
        if(cnt++ > 0xFFFF)
            return RT_ERR_NOT_FINISH;

        osal_time_mdelay(10);

        if ((ret = reg_field_read(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_FLUSH_STATUSf ,&status)) != RT_ERR_OK )
        {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
        }
    }

    return RT_ERR_OK;
}
/* Function Name:
 *      rtl9602bvb_raw_l2_flushEn_get
 * Description:
 *      Get per port force flush status
 * Input:
 *      port   		    - port id
 * Output:
 *      pEnable 		- enable status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_PORT_ID
 *      RT_ERR_NULL_POINTER
 * Note:
 *      None
 */
int32 rtl9602bvb_raw_l2_flushEn_get(rtk_port_t port, rtk_enable_t *pEnabled)
{
    int32 ret;

    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK(NULL == pEnabled, RT_ERR_NULL_POINTER);

	if ((ret = reg_array_field_read(RTL9602BVB_L2_TBL_FLUSH_ENr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, pEnabled)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}


/* Function Name:
 *      rtl9602bvb_raw_l2_flushCtrl_set
 * Description:
 *      L2 table flush control configuration.
 * Input:
 *      pCtrl 	- The flush control configuration
 * Output:
 *      None
 * Return:
 *      RT_ERR_FAILED               - Failed
 *      RT_ERR_OK                     - OK
 *      RT_ERR_OUT_OF_RANGE  - input parameter out of range
 *      RT_ERR_NULL_POINTER    - input parameter is null pointer
 *      RT_ERR_VLAN_VID           - invalid vid
 *      RT_ERR_L2_FID                - invalid fid
 *
 * Note:
 *      None
 */
int32 rtl9602bvb_raw_l2_flushCtrl_set(rtl9602bvb_raw_flush_ctrl_t *pCtrl)
{
    int32 ret = RT_ERR_FAILED;
    uint32 staticFlag;
    uint32 dynamicFlag;

    RT_PARAM_CHK(pCtrl == NULL, RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(pCtrl->flushMode >= RTL9602BVB_RAW_FLUSH_MODE_END, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pCtrl->flushType >= RTL9602BVB_RAW_FLUSH_TYPE_END, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(pCtrl->vid > RTK_VLAN_ID_MAX , RT_ERR_VLAN_VID);
    RT_PARAM_CHK(pCtrl->fid > HAL_VLAN_FID_MAX() , RT_ERR_L2_FID);

    if ((ret = reg_field_write(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_MODEf, &pCtrl->flushMode)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    switch(pCtrl->flushType)
    {
        case RTL9602BVB_RAW_FLUSH_TYPE_STATIC:
            staticFlag = 1;
            dynamicFlag = 0;
            break;
        case RTL9602BVB_RAW_FLUSH_TYPE_DYNAMIC:
            staticFlag = 0;
            dynamicFlag = 1;
            break;
        case RTL9602BVB_RAW_FLUSH_TYPE_BOTH:
            staticFlag = 1;
            dynamicFlag = 1;
            break;
        default:
            return RT_ERR_FAILED;
            break;

    }

    if ((ret = reg_field_write(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_STATICf, &staticFlag)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    if ((ret = reg_field_write(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_DYNAMICf, &dynamicFlag)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    if ((ret = reg_field_write(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_VIDf, &pCtrl->vid)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }


    if ((ret = reg_field_write(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_FIDf, &pCtrl->fid)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      rtl9602bvb_raw_l2_flushCtrl_get
 * Description:
 *      L2 table flush control configuration.
 * Input:
 *     None
 * Output:
 *     pCtrl 	- The flush control configuration
 * Return:
 *      RT_ERR_FAILED               - Failed
 *      RT_ERR_OK                     - OK
 *      RT_ERR_NULL_POINTER    - input parameter is null pointer
 * Note:
 *      None
 */
int32 rtl9602bvb_raw_l2_flushCtrl_get(rtl9602bvb_raw_flush_ctrl_t *pCtrl)

{
    int32 ret = RT_ERR_FAILED;
    uint32 staticFlag;
    uint32 dynamicFlag;

    RT_PARAM_CHK(pCtrl == NULL, RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_MODEf, &pCtrl->flushMode)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    if ((ret = reg_field_read(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_STATICf, &staticFlag)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_read(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_DYNAMICf, &dynamicFlag)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

	if (staticFlag && dynamicFlag)
		pCtrl->flushType = RTL9602BVB_RAW_FLUSH_TYPE_BOTH;
	else if (staticFlag)
		pCtrl->flushType = RTL9602BVB_RAW_FLUSH_TYPE_STATIC;
	else if (dynamicFlag)
		pCtrl->flushType = RTL9602BVB_RAW_FLUSH_TYPE_DYNAMIC;
    else
        pCtrl->flushType = RTL9602BVB_RAW_FLUSH_TYPE_END;

    if ((ret = reg_field_read(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_VIDf, &pCtrl->vid)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    if ((ret = reg_field_read(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_LUT_FLUSH_FIDf, &pCtrl->fid)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    if ((ret = reg_field_read(RTL9602BVB_L2_TBL_FLUSH_CTRLr, RTL9602BVB_FLUSH_STATUSf ,&pCtrl->flushStatus)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_rawtoL2uc(rtk_l2_ucastAddr_t *pL2Data, rtl9602bvb_lut_table_t *pLut_entry)
{
    pL2Data->vid            = pLut_entry->cvid_fid;
    pL2Data->mac.octet[0]   = pLut_entry->mac.octet[0];
    pL2Data->mac.octet[1]   = pLut_entry->mac.octet[1];
    pL2Data->mac.octet[2]   = pLut_entry->mac.octet[2];
    pL2Data->mac.octet[3]   = pLut_entry->mac.octet[3];
    pL2Data->mac.octet[4]   = pLut_entry->mac.octet[4];
    pL2Data->mac.octet[5]   = pLut_entry->mac.octet[5];
    pL2Data->fid            = pLut_entry->fid;
    pL2Data->port           = pLut_entry->spa;
    pL2Data->age            = pLut_entry->age;
    pL2Data->auth           = pLut_entry->auth;
    pL2Data->index          = pLut_entry->address;
    pL2Data->ext_port       = pLut_entry->ext_spa;

    if(ENABLED == pLut_entry->sa_block)
        pL2Data->flags |= RTK_L2_UCAST_FLAG_SA_BLOCK;

    if(ENABLED == pLut_entry->da_block)
        pL2Data->flags |= RTK_L2_UCAST_FLAG_DA_BLOCK;

    if(ENABLED == pLut_entry->nosalearn)
        pL2Data->flags |= RTK_L2_UCAST_FLAG_STATIC;

    if(ENABLED == pLut_entry->arp_used)
        pL2Data->flags |= RTK_L2_UCAST_FLAG_ARP_USED;

    if(ENABLED == pLut_entry->ivl_svl)
        pL2Data->flags |= RTK_L2_UCAST_FLAG_IVL;

	if(ENABLED == pLut_entry->ctag_if)
		pL2Data->flags |= RTK_L2_UCAST_FLAG_CTAG_IF;

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_l2uctoRaw(rtl9602bvb_lut_table_t *pLut_entry, rtk_l2_ucastAddr_t *pL2Data)
{
    pLut_entry->cvid_fid    = pL2Data->vid;
    pLut_entry->mac.octet[0]= pL2Data->mac.octet[0];
    pLut_entry->mac.octet[1]= pL2Data->mac.octet[1];
    pLut_entry->mac.octet[2]= pL2Data->mac.octet[2];
    pLut_entry->mac.octet[3]= pL2Data->mac.octet[3];
    pLut_entry->mac.octet[4]= pL2Data->mac.octet[4];
    pLut_entry->mac.octet[5]= pL2Data->mac.octet[5];
    pLut_entry->fid         = pL2Data->fid;
    pLut_entry->spa         = pL2Data->port;
    pLut_entry->age         = pL2Data->age;
    pLut_entry->auth        = pL2Data->auth;
    pLut_entry->address     = pL2Data->index;
    pLut_entry->ext_spa     = pL2Data->ext_port;

    if (pL2Data->flags & RTK_L2_UCAST_FLAG_SA_BLOCK)
        pLut_entry->sa_block = ENABLED;

    if (pL2Data->flags & RTK_L2_UCAST_FLAG_DA_BLOCK)
        pLut_entry->da_block = ENABLED;

    if (pL2Data->flags & RTK_L2_UCAST_FLAG_STATIC)
        pLut_entry->nosalearn = ENABLED;

    if (pL2Data->flags & RTK_L2_UCAST_FLAG_ARP_USED)
        pLut_entry->arp_used = ENABLED;

    if (pL2Data->flags & RTK_L2_UCAST_FLAG_IVL)
        pLut_entry->ivl_svl = ENABLED;

    if (pL2Data->flags & RTK_L2_UCAST_FLAG_CTAG_IF)
        pLut_entry->ctag_if = ENABLED;

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_rawtoL2mc(rtk_l2_mcastAddr_t *pL2Data, rtl9602bvb_lut_table_t *pLut_entry)
{
    pL2Data->mac.octet[0]   = pLut_entry->mac.octet[0];
    pL2Data->mac.octet[1]   = pLut_entry->mac.octet[1];
    pL2Data->mac.octet[2]   = pLut_entry->mac.octet[2];
    pL2Data->mac.octet[3]   = pLut_entry->mac.octet[3];
    pL2Data->mac.octet[4]   = pLut_entry->mac.octet[4];
    pL2Data->mac.octet[5]   = pLut_entry->mac.octet[5];
    pL2Data->index          = pLut_entry->address;
    pL2Data->portmask.bits[0] = pLut_entry->mbr;
    pL2Data->ext_portmask.bits[0] = pLut_entry->ext_mbr;

    if(ENABLED == pLut_entry->ivl_svl)
    {
        pL2Data->flags |= RTK_L2_MCAST_FLAG_IVL;
        pL2Data->vid   = pLut_entry->cvid_fid;
    }
    else
        pL2Data->fid   = pLut_entry->cvid_fid;

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_l2mctoRaw(rtl9602bvb_lut_table_t *pLut_entry, rtk_l2_mcastAddr_t *pL2Data)
{
    pLut_entry->mac.octet[0] = pL2Data->mac.octet[0];
    pLut_entry->mac.octet[1] = pL2Data->mac.octet[1];
    pLut_entry->mac.octet[2] = pL2Data->mac.octet[2];
    pLut_entry->mac.octet[3] = pL2Data->mac.octet[3];
    pLut_entry->mac.octet[4] = pL2Data->mac.octet[4];
    pLut_entry->mac.octet[5] = pL2Data->mac.octet[5];
    pLut_entry->address      = pL2Data->index;
    pLut_entry->mbr          = pL2Data->portmask.bits[0];
    pLut_entry->ext_mbr      = pL2Data->ext_portmask.bits[0];

    if (pL2Data->flags & RTK_L2_MCAST_FLAG_IVL)
    {
        pLut_entry->cvid_fid = pL2Data->vid;
        pLut_entry->ivl_svl  = ENABLED;
    }
    else
        pLut_entry->cvid_fid = pL2Data->fid;

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_rawtoIpv6mc(rtk_l2_ipMcastAddr_t *pL2Data, rtl9602bvb_lut_table_t *pLut_entry)
{
    pL2Data->dip6 = pLut_entry->dip6;
    pL2Data->flags |= RTK_L2_IPMCAST_FLAG_IPV6;

    if(pLut_entry->nosalearn)
        pL2Data->flags |= RTK_L2_IPMCAST_FLAG_STATIC;

    if(pLut_entry->wan_sa)
        pL2Data->flags |= RTK_L2_IPMCAST_FLAG_L3MC_ROUTE_EN;

    pL2Data->l3_mcr_index         = pLut_entry->l3mcr_idx;
    pL2Data->portmask.bits[0]     = pLut_entry->mbr;
    pL2Data->ext_portmask.bits[0] = pLut_entry->ext_mbr;
    pL2Data->index                = pLut_entry->address;

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_Ipv6mctoRaw(rtl9602bvb_lut_table_t *pLut_entry, rtk_l2_ipMcastAddr_t *pL2Data)
{
    pLut_entry->dip6 = pL2Data->dip6;
    pLut_entry->ip6 = ENABLED;

    if(pL2Data->flags & RTK_L2_IPMCAST_FLAG_STATIC)
        pLut_entry->nosalearn = ENABLED;

    if (pL2Data->flags & RTK_L2_IPMCAST_FLAG_L3MC_ROUTE_EN)
        pLut_entry->wan_sa = ENABLED;

    pLut_entry->l3mcr_idx = pL2Data->l3_mcr_index;
    pLut_entry->mbr      = pL2Data->portmask.bits[0];
    pLut_entry->ext_mbr   = pL2Data->ext_portmask.bits[0];
    pLut_entry->address  = pL2Data->index;

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_rawtoIpmc(rtk_l2_ipMcastAddr_t *pL2Data, rtl9602bvb_lut_table_t *pLut_entry)
{
    if(pLut_entry->ip6)
    {
        return _dal_rtl9602bvb_rawtoIpv6mc(pL2Data, pLut_entry);
    }

    pL2Data->dip            = (pLut_entry->gip | 0xE0000000);
    pL2Data->l3_trans_index = pLut_entry->l3_idx;
    pL2Data->sip_index = pLut_entry->sip_idx;

    pL2Data->portmask.bits[0]     = pLut_entry->mbr;
    pL2Data->ext_portmask.bits[0] = pLut_entry->ext_mbr;
    pL2Data->index                = pLut_entry->address;

    if(pLut_entry->wan_sa)
        pL2Data->flags |= RTK_L2_IPMCAST_FLAG_L3MC_ROUTE_EN;
    if(pLut_entry->sip_filter)
        pL2Data->flags |= RTK_L2_IPMCAST_FLAG_SIP_FILTER;
    if(pLut_entry->nosalearn)
        pL2Data->flags |= RTK_L2_IPMCAST_FLAG_STATIC;
    if(pLut_entry->ivl_svl)
    {
        pL2Data->vid = pLut_entry->cvid_fid;
        pL2Data->flags |= RTK_L2_IPMCAST_FLAG_IVL;
    }
    else
    {
        pL2Data->fid = pLut_entry->cvid_fid;
    }

    if(pLut_entry->nosalearn)
        pL2Data->flags |= RTK_L2_IPMCAST_FLAG_STATIC;

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_IpmctoRaw(rtl9602bvb_lut_table_t *pLut_entry, rtk_l2_ipMcastAddr_t *pL2Data)
{
    if(pL2Data->flags & RTK_L2_IPMCAST_FLAG_IPV6)
    {
        return _dal_rtl9602bvb_Ipv6mctoRaw(pLut_entry, pL2Data);
    }

    pLut_entry->gip = pL2Data->dip;
    pLut_entry->l3_idx = pL2Data->l3_trans_index;

    pLut_entry->mbr      = pL2Data->portmask.bits[0];
    pLut_entry->ext_mbr  = pL2Data->ext_portmask.bits[0];
    pLut_entry->address  = pL2Data->index;

    if (pL2Data->flags & RTK_L2_IPMCAST_FLAG_L3MC_ROUTE_EN)
        pLut_entry->wan_sa = ENABLED;
    if(pL2Data->flags & RTK_L2_IPMCAST_FLAG_SIP_FILTER)
    {
        pLut_entry->sip_filter = ENABLED;
        pLut_entry->sip_idx = pL2Data->sip_index;
    }
    else
    {
        /* Force sip_idx to 0 when sip_filter disabled */
        pLut_entry->sip_filter = DISABLED;
        pLut_entry->sip_idx = 0;
    }
    if(pL2Data->flags & RTK_L2_IPMCAST_FLAG_STATIC)
        pLut_entry->nosalearn = ENABLED;

    if (pL2Data->flags & RTK_L2_IPMCAST_FLAG_IVL)
    {
        pLut_entry->cvid_fid = pL2Data->vid;
        pLut_entry->ivl_svl  = ENABLED;
    }
    else
    {
        pLut_entry->cvid_fid = pL2Data->fid;
    }

    return RT_ERR_OK;
}

static int32
_dal_rtl9602bvb_lutReadMethodtoRaw(rtl9602bvb_raw_l2_readMethod_t *raw, rtk_l2_readMethod_t cfg)
{
   	switch(cfg){
	case LUT_READ_METHOD_MAC:
		*raw = RTL9602BVB_RAW_LUT_READ_METHOD_MAC;
	break;
	case LUT_READ_METHOD_ADDRESS:
		*raw = RTL9602BVB_RAW_LUT_READ_METHOD_ADDRESS;
	break;
	case LUT_READ_METHOD_NEXT_ADDRESS:
		*raw = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_ADDRESS;
	break;
	case LUT_READ_METHOD_NEXT_L2UC:
		*raw = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L2UC;
	break;
	case LUT_READ_METHOD_NEXT_L2MC:
		*raw = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L2MC;
	break;
	case LUT_READ_METHOD_NEXT_L3MC:
		*raw = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L3MC;
	break;
	case LUT_READ_METHOD_NEXT_L2L3MC:
		*raw = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L2L3MC;
	break;
	case LUT_READ_METHOD_NEXT_L2UCSPA:
		*raw = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L2UCSPA;
	break;
	default:
	break;
	}
    return RT_ERR_OK;
}

/* Function Name:
 *		rtl9602bvb_l2_unknReservedIpv4McAction_set
 * Description:
 *		Set unknown Reserved IPv4 multicast address packet action
 * Input:
 *		act	- rtl9602bvb_l2_resAct_t type data which has following configurations
 *          RTL9602BVB_L2_RESACT_NORMAL: treat as normal unknown multicast packet
 *          RTL9602BVB_L2_RESACT_FLOODING: always flooding
 *          RTL9602BVB_L2_RESACT_TRAP: trap to CPU
 * Output:
 *		Nnoe.
 * Return:
 *		RT_ERR_OK
 *		RT_ERR_FAILED
 *		RT_ERR_INPUT
 * Note:
 *		None
 */
int32 rtl9602bvb_l2_unknReservedIpv4McAction_set(rtl9602bvb_l2_resAct_t act)
{
	int32 ret;

	RT_PARAM_CHK((RTL9602BVB_L2_RESACT_END <= act), RT_ERR_INPUT);

	if ((ret = reg_field_write(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_RSV_IP4_ACTf, &act)) != RT_ERR_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L2), "");
		return ret;
	}

	return RT_ERR_OK;
}

/* Function Name:
 *		rtl9602bvb_l2_unknReservedIpv4McAction_get
 * Description:
 *		get unknown Reserved IPv4 multicast address packet action
 * Input:
 *		None.
 * Output:
 *		pAct - rtl9602bvb_l2_resAct_t type data which has following configurations
 *          RTL9602BVB_L2_RESACT_NORMAL: treat as normal unknown multicast packet
 *          RTL9602BVB_L2_RESACT_FLOODING: always flooding
 *          RTL9602BVB_L2_RESACT_TRAP: trap to CPU
 * Return:
 *		RT_ERR_OK
 *		RT_ERR_FAILED
 *		RT_ERR_NULL_POINTER
 * Note:
 *		None
 */
int32 rtl9602bvb_l2_unknReservedIpv4McAction_get(rtl9602bvb_l2_resAct_t *pAct)
{
	int32 ret;

	RT_PARAM_CHK((NULL == pAct), RT_ERR_NULL_POINTER);

	if ((ret = reg_field_read(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_RSV_IP4_ACTf, pAct)) != RT_ERR_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L2), "");
		return ret;
	}

	return RT_ERR_OK;
}

/* Function Name:
 *		rtl9602bvb_l2_unknReservedIpv6McAction_set
 * Description:
 *		Set unknown Reserved IPv6 multicast address packet action
 * Input:
 *		act	- rtl9602bvb_l2_resAct_t type data which has following configurations
 *          RTL9602BVB_L2_RESACT_NORMAL: treat as normal unknown multicast packet
 *          RTL9602BVB_L2_RESACT_FLOODING: always flooding
 *          RTL9602BVB_L2_RESACT_TRAP: trap to CPU
 * Output:
 *		Nnoe.
 * Return:
 *		RT_ERR_OK
 *		RT_ERR_FAILED
 *		RT_ERR_INPUT
 * Note:
 *		None
 */
int32 rtl9602bvb_l2_unknReservedIpv6McAction_set(rtl9602bvb_l2_resAct_t act)
{
	int32 ret;

	RT_PARAM_CHK((RTL9602BVB_L2_RESACT_END <= act), RT_ERR_INPUT);

	if ((ret = reg_field_write(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_RSV_IP6_ACTf, &act)) != RT_ERR_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L2), "");
		return ret;
	}

	return RT_ERR_OK;
}

/* Function Name:
 *		rtl9602bvb_l2_unknReservedIpv6McAction_get
 * Description:
 *		get unknown Reserved IPv6 multicast address packet action
 * Input:
 *		None.
 * Output:
 *		pAct - rtl9602bvb_l2_resAct_t type data which has following configurations
 *          RTL9602BVB_L2_RESACT_NORMAL: treat as normal unknown multicast packet
 *          RTL9602BVB_L2_RESACT_FLOODING: always flooding
 *          RTL9602BVB_L2_RESACT_TRAP: trap to CPU
 * Return:
 *		RT_ERR_OK
 *		RT_ERR_FAILED
 *		RT_ERR_NULL_POINTER
 * Note:
 *		None
 */
int32 rtl9602bvb_l2_unknReservedIpv6McAction_get(rtl9602bvb_l2_resAct_t *pAct)
{
	int32 ret;

	RT_PARAM_CHK((NULL == pAct), RT_ERR_NULL_POINTER);

	if ((ret = reg_field_read(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_RSV_IP6_ACTf, pAct)) != RT_ERR_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L2), "");
		return ret;
	}

	return RT_ERR_OK;
}

/* Function Name:
 *		rtl9602bvb_l2_unknMcIcmpv6_set
 * Description:
 *		Set unknown multicast ICMPv6 packet trap action
 * Input:
 *      enable - configure value
 * Output:
 *		Nnoe.
 * Return:
 *		RT_ERR_OK
 *		RT_ERR_FAILED
 *		RT_ERR_INPUT
 * Note:
 *		None
 */
int32 rtl9602bvb_l2_unknMcIcmpv6_set(rtk_enable_t enable)
{
	int32 ret;

	RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_INPUT);

	if ((ret = reg_field_write(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_MC_ICMP6_TRAPf, &enable)) != RT_ERR_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L2), "");
		return ret;
	}

	return RT_ERR_OK;
}

/* Function Name:
 *		rtl9602bvb_l2_unknMcIcmpv6_get
 * Description:
 *		Get unknown multicast ICMPv6 packet trap action
 * Input:
 *		Nnoe.
 * Output:
 *      pEnable - configure value
 * Return:
 *		RT_ERR_OK
 *		RT_ERR_FAILED
 *		RT_ERR_INPUT
 * Note:
 *		None
 */
int32 rtl9602bvb_l2_unknMcIcmpv6_get(rtk_enable_t *pEnable)
{
	int32 ret;

	RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

	if ((ret = reg_field_read(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_MC_ICMP6_TRAPf, pEnable)) != RT_ERR_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L2), "");
		return ret;
	}

	return RT_ERR_OK;
}

/* Function Name:
 *		rtl9602bvb_l2_unknMcDhcpv6_set
 * Description:
 *		Set unknown multicast DHCPv6 packet trap action
 * Input:
 *      enable - configure value
 * Output:
 *		Nnoe.
 * Return:
 *		RT_ERR_OK
 *		RT_ERR_FAILED
 *		RT_ERR_INPUT
 * Note:
 *		None
 */
int32 rtl9602bvb_l2_unknMcDhcpv6_set(rtk_enable_t enable)
{
	int32 ret;

	RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_INPUT);

	if ((ret = reg_field_write(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_MC_DHCP6_TRAPf, &enable)) != RT_ERR_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L2), "");
		return ret;
	}

	return RT_ERR_OK;
}

/* Function Name:
 *		rtl9602bvb_l2_unknMcDhcpv6_get
 * Description:
 *		Get unknown multicast DHCPv6 packet trap action
 * Input:
 *		Nnoe.
 * Output:
 *      pEnable - configure value
 * Return:
 *		RT_ERR_OK
 *		RT_ERR_FAILED
 *		RT_ERR_INPUT
 * Note:
 *		None
 */
int32 rtl9602bvb_l2_unknMcDhcpv6_get(rtk_enable_t *pEnable)
{
	int32 ret;

	RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

	if ((ret = reg_field_read(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_MC_DHCP6_TRAPf, pEnable)) != RT_ERR_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L2), "");
		return ret;
	}

	return RT_ERR_OK;
}

/* Module Name    : L2     */
/* Sub-module Name: Global */

/* Function Name:
 *      dal_rtl9602bvb_l2_init
 * Description:
 *      Initialize l2 module of the specified device.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Must initialize l2 module before calling any l2 APIs.
 */
int32
dal_rtl9602bvb_l2_init(void)
{
    int32   ret;
    rtk_port_t port;
    rtk_portmask_t all_portmask;
    rtk_portmask_t all_extPortmask;
    uint32 value;
    rtl9602bvb_raw_flush_ctrl_t rtl9602bvb_cfg;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    l2_init = INIT_COMPLETED;

    if((ret = dal_rtl9602bvb_l2_flushLinkDownPortAddrEnable_set(ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    if((ret = dal_rtl9602bvb_l2_table_clear()) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    HAL_SCAN_ALL_PORT(port)
    {
        if((ret = dal_rtl9602bvb_l2_portLimitLearningCnt_set(port, HAL_L2_LEARN_LIMIT_CNT_MAX())) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            l2_init = INIT_NOT_COMPLETED;
            return ret;
        }

        if((ret = dal_rtl9602bvb_l2_portLimitLearningCntAction_set(port, LIMIT_LEARN_CNT_ACTION_FORWARD)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            l2_init = INIT_NOT_COMPLETED;
            return ret;
        }

        if((ret = dal_rtl9602bvb_l2_portAgingEnable_set(port, ENABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            l2_init = INIT_NOT_COMPLETED;
            return ret;
        }

        if((ret = dal_rtl9602bvb_l2_newMacOp_set(port, HARDWARE_LEARNING, ACTION_FORWARD)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            l2_init = INIT_NOT_COMPLETED;
            return ret;
        }

        if((ret = dal_rtl9602bvb_l2_illegalPortMoveAction_set(port, ACTION_FORWARD)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            l2_init = INIT_NOT_COMPLETED;
            return ret;
        }

        if((ret = dal_rtl9602bvb_l2_portLookupMissAction_set(port, DLF_TYPE_IPMC, ACTION_FORWARD)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            l2_init = INIT_NOT_COMPLETED;
            return ret;
        }

        if((ret = dal_rtl9602bvb_l2_portLookupMissAction_set(port, DLF_TYPE_MCAST, ACTION_FORWARD)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            l2_init = INIT_NOT_COMPLETED;
            return ret;
        }

        if((ret = dal_rtl9602bvb_l2_portLookupMissAction_set(port, DLF_TYPE_UCAST, ACTION_FORWARD)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            l2_init = INIT_NOT_COMPLETED;
            return ret;
        }
    }

    if((ret = dal_rtl9602bvb_l2_limitLearningCnt_set(HAL_L2_LEARN_LIMIT_CNT_MAX())) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    if((ret = dal_rtl9602bvb_l2_aging_set(RTK_L2_DEFAULT_AGING_TIME)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    HAL_GET_ALL_PORTMASK(all_portmask);
    if((ret = dal_rtl9602bvb_l2_lookupMissFloodPortMask_set(DLF_TYPE_IPMC, &all_portmask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    if((ret = dal_rtl9602bvb_l2_lookupMissFloodPortMask_set(DLF_TYPE_BCAST, &all_portmask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    if((ret = dal_rtl9602bvb_l2_lookupMissFloodPortMask_set(DLF_TYPE_UCAST, &all_portmask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    if((ret = dal_rtl9602bvb_l2_ipmcMode_set(LOOKUP_ON_MAC_AND_VID_FID)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    value = 0; /* <M> MDY REG field */
    if ((ret = reg_field_write(RTL9602BVB_UNKN_MC_CFGr, RTL9602BVB_UNKN_RSV_IP4_ACTf, &value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    if((ret = dal_rtl9602bvb_l2_srcPortEgrFilterMask_set(&all_portmask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    HAL_GET_ALL_EXT_PORTMASK(all_extPortmask);
    RTK_PORTMASK_PORT_CLEAR(all_extPortmask, HAL_GET_EXT_CPU_PORT());
    if((ret = dal_rtl9602bvb_l2_extPortEgrFilterMask_set(&all_extPortmask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    /* Set flush parameter as dynamic only */
    if((ret = rtl9602bvb_raw_l2_flushCtrl_get(&rtl9602bvb_cfg)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    rtl9602bvb_cfg.flushType = RTL9602BVB_RAW_FLUSH_TYPE_DYNAMIC;

    if((ret = rtl9602bvb_raw_l2_flushCtrl_set(&rtl9602bvb_cfg)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }

    HAL_GET_ALL_PORTMASK(all_portmask);
    if((ret = dal_rtl9602bvb_l2_limitLearningPortMask_set(all_portmask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        l2_init = INIT_NOT_COMPLETED;
        return ret;
    }
    
    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_init */

/* Module Name    : L2                */
/* Sub-module Name: Mac address flush */

/* Function Name:
 *      dal_rtl9602bvb_l2_flushLinkDownPortAddrEnable_get
 * Description:
 *      Get HW flush linkdown port mac configuration.
 * Input:
 *      None
 * Output:
 *      pEnable - pointer buffer of state of HW clear linkdown port mac
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) Make sure chip have supported the function before using the API.
 *      (2) The API is apply to whole system.
 *      (3) The status of flush linkdown port address is as following:
 *          - DISABLED
 *          - ENABLED
 */
int32
dal_rtl9602bvb_l2_flushLinkDownPortAddrEnable_get(rtk_enable_t *pEnable)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(RTL9602BVB_LUT_CFGr, RTL9602BVB_LINKDOWN_AGEOUTf, pEnable)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_flushLinkDownPortAddrEnable_get */


/* Function Name:
 *      dal_rtl9602bvb_l2_flushLinkDownPortAddrEnable_set
 * Description:
 *      Set HW flush linkdown port mac configuration.
 * Input:
 *      enable - configure value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT   - invalid input parameter
 * Note:
 *      (1) Make sure chip have supported the function before using the API.
 *      (2) The API is apply to whole system.
 *      (3) The status of flush linkdown port address is as following:
 *          - DISABLED
 *          - ENABLED
 */
int32
dal_rtl9602bvb_l2_flushLinkDownPortAddrEnable_set(rtk_enable_t enable)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_INPUT);

    if ((ret = reg_field_write(RTL9602BVB_LUT_CFGr, RTL9602BVB_LINKDOWN_AGEOUTf, &enable)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_flushLinkDownPortAddrEnable_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_ucastAddr_flush
 * Description:
 *      Flush unicast address
 * Input:
 *      pConfig - flush config
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_ucastAddr_flush(rtk_l2_flushCfg_t *pConfig)
{
    int32   ret;
    int32   back_ret;
    rtl9602bvb_raw_flush_ctrl_t flush_ctrl;
    rtk_port_t port;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pConfig), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pConfig->flushByVid >= RTK_ENABLE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((pConfig->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK((pConfig->flushByPort >= RTK_ENABLE_END), RT_ERR_INPUT);
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(pConfig->port), RT_ERR_INPUT);
    RT_PARAM_CHK((pConfig->flushByFid >= RTK_ENABLE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((pConfig->fid > HAL_VLAN_FID_MAX()), RT_ERR_L2_FID);
    RT_PARAM_CHK((pConfig->flushStaticAddr >= RTK_ENABLE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((pConfig->flushDynamicAddr >= RTK_ENABLE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((pConfig->flushAddrOnAllPorts >= RTK_ENABLE_END), RT_ERR_INPUT);

    /* Error Configuration Check: can't support both VID and FID flush */
    if( (ENABLED == pConfig->flushByVid) && (ENABLED == pConfig->flushByFid) )
        return RT_ERR_CHIP_NOT_SUPPORTED;

    /* Error Configuration Check: must specify port(s). */
    if( (DISABLED == pConfig->flushByPort) && (DISABLED == pConfig->flushAddrOnAllPorts) )
        return RT_ERR_INPUT;

    /* Set Configuration */
    osal_memset(&flush_ctrl, 0x00, sizeof(rtl9602bvb_raw_flush_ctrl_t));
    if(ENABLED == pConfig->flushByVid)
    {
        flush_ctrl.flushMode = RTL9602BVB_RAW_FLUSH_MODE_VID;
        flush_ctrl.vid = pConfig->vid;
    }
    else if(ENABLED == pConfig->flushByFid)
    {
        flush_ctrl.flushMode = RTL9602BVB_RAW_FLUSH_MODE_FID;
        flush_ctrl.fid = pConfig->fid;
    }
    else if(ENABLED == pConfig->flushByPort)
    {
        flush_ctrl.flushMode = RTL9602BVB_RAW_FLUSH_MODE_PORT;
    }

    if( (ENABLED == pConfig->flushStaticAddr) && (ENABLED == pConfig->flushDynamicAddr) )
        flush_ctrl.flushType = RTL9602BVB_RAW_FLUSH_TYPE_BOTH;
    else if(ENABLED == pConfig->flushStaticAddr)
        flush_ctrl.flushType = RTL9602BVB_RAW_FLUSH_TYPE_STATIC;
    else if(ENABLED == pConfig->flushDynamicAddr)
        flush_ctrl.flushType = RTL9602BVB_RAW_FLUSH_TYPE_DYNAMIC;
    else
        return RT_ERR_INPUT;

    if((ret = rtl9602bvb_raw_l2_flushCtrl_set(&flush_ctrl)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    /* Start Flushing */
    if(ENABLED == pConfig->flushByPort)
    {
        if((ret = rtl9602bvb_raw_l2_flushEn_set(pConfig->port, ENABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        }
    }
    else if(ENABLED == pConfig->flushAddrOnAllPorts)
    {
        HAL_SCAN_ALL_PORT(port)
        {
            if((ret = rtl9602bvb_raw_l2_flushEn_set(port, ENABLED)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
                break;
            }
        }
    }

    flush_ctrl.flushType = RTL9602BVB_RAW_FLUSH_TYPE_DYNAMIC;
    if((back_ret = rtl9602bvb_raw_l2_flushCtrl_set(&flush_ctrl)) != RT_ERR_OK)
    {
        RT_ERR(back_ret, (MOD_L2 | MOD_DAL), "");
        return back_ret;
    }

    return ret;
} /* end of dal_rtl9602bvb_l2_ucastAddr_flush */

/* Function Name:
 *      dal_rtl9602bvb_l2_table_clear
 * Description:
 *      Clear entire L2 table.
 *      All the entries (static and dynamic) (L2 and L3) will be deleted.
 * Input:
 *      None.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_table_clear(void)
{
    int32   ret;
    uint32  index = 0;
    rtl9602bvb_lut_table_t lut_entry;

    do
    {
        osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
        lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_ADDRESS;
        lut_entry.address = index;

        if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return ret;
        }

        if(0 == lut_entry.lookup_hit)
        {
            /* table empty */
            break;
        }

        if(lut_entry.address < index)
        {
            /* all table clear and hit new-learn entry */
            break;
        }

        lut_entry.valid      = DISABLED;

        /* Change to write method */
        lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_MAC;
        if ((ret = rtl9602bvb_raw_l2_lookUpTb_set(&lut_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return ret;
        }

        /* Assign new index */
        index = lut_entry.address + 1;
    }
    while(1);

    return RT_ERR_OK;
}   /* end of dal_rtl9602bvb_l2_table_clear */

/* Module Name    : L2                     */
/* Sub-module Name: Address learning limit */

/* Function Name:
 *      dal_rtl9602bvb_l2_learningCnt_get
 * Description:
 *      Get the total mac learning counts of the whole specified device.
 * Input:
 *      None.
 * Output:
 *      pMacCnt - pointer buffer of mac learning counts of the port
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The mac learning counts only calculate dynamic mac numbers.
 */
int32
dal_rtl9602bvb_l2_learningCnt_get(uint32 *pMacCnt)
{
    int32 ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMacCnt), RT_ERR_NULL_POINTER);

    /* get value from CHIP*/
    if((ret = reg_field_read(RTL9602BVB_L2_SYS_LRN_CNTr, RTL9602BVB_SYS_L2_LRN_CNTf, pMacCnt)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_learningCnt_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_limitLearningCnt_get
 * Description:
 *      Get the maximum mac learning counts of the specified device.
 * Input:
 *      None
 * Output:
 *      pMacCnt - pointer buffer of maximum mac learning counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *      address, not apply to static mac address.
 */
int32
dal_rtl9602bvb_l2_limitLearningCnt_get(uint32 *pMacCnt)
{
    int32 ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMacCnt), RT_ERR_NULL_POINTER);

    /* get value from CHIP*/
    if((ret = reg_field_read(RTL9602BVB_LUT_SYS_LRN_LIMITNOr, RTL9602BVB_SYS_LRN_LIMITNOf, pMacCnt)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_limitLearningCnt_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_limitLearningCnt_set
 * Description:
 *      Set the maximum mac learning counts of the specified device.
 * Input:
 *      macCnt - maximum mac learning counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_LIMITED_L2ENTRY_NUM - invalid limited L2 entry number
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *      address, not apply to static mac address.
 *      (2) Set the macCnt to 0 mean disable learning in the system.
 */
int32
dal_rtl9602bvb_l2_limitLearningCnt_set(uint32 macCnt)
{
    int32 ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(macCnt > HAL_L2_LEARN_LIMIT_CNT_MAX(), RT_ERR_LIMITED_L2ENTRY_NUM);

    /* programming value into CHIP*/
    if((ret = reg_field_write(RTL9602BVB_LUT_SYS_LRN_LIMITNOr, RTL9602BVB_SYS_LRN_LIMITNOf, &macCnt)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_limitLearningCnt_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_limitLearningCntAction_get
 * Description:
 *      Get the action when over learning maximum mac counts of the specified device.
 * Input:
 *      None
 * Output:
 *      pLearningAction - pointer buffer of action when over learning maximum mac counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The action symbol as following
 *      - LIMIT_LEARN_CNT_ACTION_DROP
 *      - LIMIT_LEARN_CNT_ACTION_FORWARD
 *      - LIMIT_LEARN_CNT_ACTION_TO_CPU
 *      - LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU
 */
int32
dal_rtl9602bvb_l2_limitLearningCntAction_get(rtk_l2_limitLearnCntAction_t *pLearningAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pLearningAction), RT_ERR_NULL_POINTER);

    /* get value from CHIP*/
    if((ret = reg_field_read(RTL9602BVB_LUT_SYS_LRN_OVER_CTRLr, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* translate register value to action */
    switch (act)
    {
        case 0:
            *pLearningAction = LIMIT_LEARN_CNT_ACTION_FORWARD;
            break;
        case 1:
            *pLearningAction = LIMIT_LEARN_CNT_ACTION_DROP;
            break;
        case 2:
            *pLearningAction = LIMIT_LEARN_CNT_ACTION_TO_CPU;
            break;
        case 3:
            *pLearningAction = LIMIT_LEARN_CNT_ACTION_COPY_CPU;
            break;
        default:
            return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_limitLearningCntAction_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_limitLearningCntAction_set
 * Description:
 *      Set the action when over learning maximum mac counts of the specified device.
 * Input:
 *      learningAction - action when over learning maximum mac counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      (1) The action symbol as following
 *      - LIMIT_LEARN_CNT_ACTION_DROP
 *      - LIMIT_LEARN_CNT_ACTION_FORWARD
 *      - LIMIT_LEARN_CNT_ACTION_TO_CPU
 *      - LIMIT_LEARN_CNT_ACTION_COPY_TO_CPU
 */
int32
dal_rtl9602bvb_l2_limitLearningCntAction_set(rtk_l2_limitLearnCntAction_t learningAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(learningAction >= LIMIT_LEARN_CNT_ACTION_END, RT_ERR_INPUT);

    switch (learningAction)
    {
        case LIMIT_LEARN_CNT_ACTION_DROP:
            act = 1;
            break;
        case LIMIT_LEARN_CNT_ACTION_FORWARD:
            act = 0;
            break;
        case LIMIT_LEARN_CNT_ACTION_TO_CPU:
            act = 2;
            break;
        case LIMIT_LEARN_CNT_ACTION_COPY_CPU:
            act = 3;
            break;
        default:
            return RT_ERR_INPUT;
    }

    /* programming value to CHIP*/
    if((ret = reg_field_write(RTL9602BVB_LUT_SYS_LRN_OVER_CTRLr, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_limitLearningCntAction_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_limitLearningEntryAction_get
 * Description:
 *      Get the action when over learning the same hash result entry.
 * Input:
 *      None
 * Output:
 *      pLearningAction - pointer buffer of action when over learning the same hash result entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The action symbol as following
 *          - LIMIT_LEARN_ENTRY_ACTION_FORWARD
 *          - LIMIT_LEARN_ENTRY_ACTION_TO_CPU
 */
int32
dal_rtl9602bvb_l2_limitLearningEntryAction_get(rtk_l2_limitLearnEntryAction_t *pLearningAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pLearningAction), RT_ERR_NULL_POINTER);

    /* get value from CHIP*/
    if((ret = reg_field_read(RTL9602BVB_LUT_CFGr, RTL9602BVB_LUT_ENTRY_FULL_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    /* translate register value to action */
    switch (act)
    {
        case 0:
            *pLearningAction = LIMIT_LEARN_ENTRY_ACTION_FORWARD;
            break;
        case 1:
            *pLearningAction = LIMIT_LEARN_ENTRY_ACTION_TO_CPU;
            break;
        default:
            return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_limitLearningEntryAction_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_limitLearningEntryAction_set
 * Description:
 *      Set the action when over learning the same hash result entry.
 * Input:
 *      learningAction - action when over learning the same hash result entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      (1) The action symbol as following
 *          - LIMIT_LEARN_ENTRY_ACTION_FORWARD
 *          - LIMIT_LEARN_ENTRY_ACTION_TO_CPU
 */
int32
dal_rtl9602bvb_l2_limitLearningEntryAction_set(rtk_l2_limitLearnEntryAction_t learningAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(learningAction >= LIMIT_LEARN_ENTRY_ACTION_END, RT_ERR_INPUT);

    switch (learningAction)
    {
        case LIMIT_LEARN_ENTRY_ACTION_FORWARD:
            act = 0;
            break;
        case LIMIT_LEARN_ENTRY_ACTION_TO_CPU:
            act = 1;
            break;
        default:
            return RT_ERR_INPUT;
    }

    /* programming value to CHIP*/
    if((ret = reg_field_write(RTL9602BVB_LUT_CFGr, RTL9602BVB_LUT_ENTRY_FULL_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_limitLearningEntryAction_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_limitLearningPortMask_get
 * Description:
 *      Get the port mask that indicate which port(s) are counted into
 *      system learning count
 * Input:
 *      None
 * Output:
 *      pPortmask - configure the port mask which counted into system counter
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *          address, not apply to static mac address.
 */
int32
dal_rtl9602bvb_l2_limitLearningPortMask_get(rtk_portmask_t *pPortmask)
{
    int32   ret;
    uint32  port;
    uint32  value;
    rtk_portmask_t mask;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pPortmask), RT_ERR_NULL_POINTER);

    RTK_PORTMASK_RESET(mask);
    HAL_SCAN_ALL_PORT(port)
    {
        if((ret = reg_array_field_read(RTL9602BVB_LUT_SYS_LRN_LIMITr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return ret;
        }
        if(value)
        {
            RTK_PORTMASK_PORT_SET(mask, port);
        }
    }
    *pPortmask = mask;

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_rtl9602bvb_l2_limitLearningPortMask_set
 * Description:
 *      Set the port mask that indicate which port(s) are counted into
 *      system learning count
 * Input:
 *      portmask - configure the port mask which counted into system counter
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_MASK
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *          address, not apply to static mac address.
 */
int32
dal_rtl9602bvb_l2_limitLearningPortMask_set(rtk_portmask_t portmask)
{
    int32   ret;
    uint32  port;
    uint32  value;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((!HAL_IS_PORTMASK_VALID(portmask)), RT_ERR_NULL_POINTER);

    HAL_SCAN_ALL_PORT(port)
    {
        if(RTK_PORTMASK_IS_PORT_SET(portmask, port))
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        if((ret = reg_array_field_write(RTL9602BVB_LUT_SYS_LRN_LIMITr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return ret;
        }
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_rtl9602bvb_l2_portLimitLearningOverStatus_get
 * Description:
 *      Get the port learning over status
 * Input:
 *      port        - Port ID
 * Output:
 *      pStatus     - 1: learning over, 0: not learning over
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *
 */
int32
dal_rtl9602bvb_l2_portLimitLearningOverStatus_get(rtk_port_t port, uint32 *pStatus)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pStatus), RT_ERR_NULL_POINTER);

    if((ret = reg_array_field_read(RTL9602BVB_L2_LRN_OVER_STSr, (uint32)port, REG_ARRAY_INDEX_NONE, RTL9602BVB_LRN_OVER_INDf, pStatus)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_rtl9602bvb_l2_portLimitLearningOverStatus_clear
 * Description:
 *      Clear the port learning over status
 * Input:
 *      port        - Port ID
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 * Note:
 *
 */
int32
dal_rtl9602bvb_l2_portLimitLearningOverStatus_clear(rtk_port_t port)
{
    int32   ret;
    uint32 regData = 1;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);

    if((ret = reg_array_field_write(RTL9602BVB_L2_LRN_OVER_STSr, (uint32)port, REG_ARRAY_INDEX_NONE, RTL9602BVB_LRN_OVER_INDf, &regData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_rtl9602bvb_l2_portLearningCnt_get
 * Description:
 *      Get the mac learning counts of the port.
 * Input:
 *      port     - port id
 * Output:
 *      pMacCnt  - pointer buffer of mac learning counts of the port
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The mac learning counts only calculate dynamic mac numbers.
 */
int32
dal_rtl9602bvb_l2_portLearningCnt_get(rtk_port_t port, uint32 *pMacCnt)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pMacCnt), RT_ERR_NULL_POINTER);

	if((ret = reg_array_field_read(RTL9602BVB_L2_LRN_CNTr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_L2_LRN_CNTf, pMacCnt)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_portLearningCnt_get */



/* Function Name:
 *      dal_rtl9602bvb_l2_portLimitLearningCnt_get
 * Description:
 *      Get the maximum mac learning counts of the port.
 * Input:
 *      port     - port id
 * Output:
 *      pMacCnt - pointer buffer of maximum mac learning counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *          address, not apply to static mac address.
 */
int32
dal_rtl9602bvb_l2_portLimitLearningCnt_get(rtk_port_t port, uint32 *pMacCnt)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pMacCnt), RT_ERR_NULL_POINTER);

	if (( ret = reg_array_field_read(RTL9602BVB_LUT_LRN_LIMITNOr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_NUMf, pMacCnt)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_portLimitLearningCnt_get */


/* Function Name:
 *      dal_rtl9602bvb_l2_portLimitLearningCnt_set
 * Description:
 *      Set the maximum mac learning counts of the port.
 * Input:
 *      port    - port id
 *      macCnt  - maximum mac learning counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID             - invalid port id
 *      RT_ERR_LIMITED_L2ENTRY_NUM - invalid limited L2 entry number
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *          address, not apply to static mac address.
 *      (2) Set the macCnt to 0 mean disable learning in the port.
 */
int32
dal_rtl9602bvb_l2_portLimitLearningCnt_set(rtk_port_t port, uint32 macCnt)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((HAL_L2_LEARN_LIMIT_CNT_MAX() < macCnt), RT_ERR_INPUT);

    if ((ret = reg_array_field_write(RTL9602BVB_LUT_LRN_LIMITNOr, port,REG_ARRAY_INDEX_NONE, RTL9602BVB_NUMf, &macCnt)) != RT_ERR_OK )
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_portLimitLearningCnt_set */


/* Function Name:
 *      dal_rtl9602bvb_l2_portLimitLearningCntAction_get
 * Description:
 *      Get the action when over learning maximum mac counts of the port.
 * Input:
 *      port    - port id
 * Output:
 *      pLearningAction - pointer buffer of action when over learning maximum mac counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      The action symbol as following
 *      - LIMIT_LEARN_CNT_ACTION_DROP
 *      - LIMIT_LEARN_CNT_ACTION_FORWARD
 *      - LIMIT_LEARN_CNT_ACTION_TO_CPU
 *      - LIMIT_LEARN_CNT_ACTION_COPY_CPU
 */
int32
dal_rtl9602bvb_l2_portLimitLearningCntAction_get(rtk_port_t port, rtk_l2_limitLearnCntAction_t *pLearningAction)
{
    int32   ret;
    uint32 act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pLearningAction), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(RTL9602BVB_LUT_LEARN_OVER_CTRLr, port,REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    switch(act)
    {
        case 0:
            *pLearningAction = LIMIT_LEARN_CNT_ACTION_FORWARD;
            break;
        case 1:
            *pLearningAction = LIMIT_LEARN_CNT_ACTION_DROP;
            break;
        case 2:
            *pLearningAction = LIMIT_LEARN_CNT_ACTION_TO_CPU;
            break;
        case 3:
            *pLearningAction = LIMIT_LEARN_CNT_ACTION_COPY_CPU;
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_portLimitLearningCntAction_get */


/* Function Name:
 *      dal_rtl9602bvb_l2_portLimitLearningCntAction_set
 * Description:
 *      Set the action when over learning maximum mac counts of the port.
 * Input:
 *      port   - port id
 *      learningAction - action when over learning maximum mac counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - invalid port id
 * Note:
 *      The action symbol as following
 *      - LIMIT_LEARN_CNT_ACTION_DROP
 *      - LIMIT_LEARN_CNT_ACTION_FORWARD
 *      - LIMIT_LEARN_CNT_ACTION_TO_CPU
 *      - LIMIT_LEARN_CNT_ACTION_COPY_CPU
 */
int32
dal_rtl9602bvb_l2_portLimitLearningCntAction_set(rtk_port_t port, rtk_l2_limitLearnCntAction_t learningAction)
{

    int32   ret;
    uint32 act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((LIMIT_LEARN_CNT_ACTION_END <= learningAction), RT_ERR_INPUT);

    switch(learningAction)
    {
        case LIMIT_LEARN_CNT_ACTION_FORWARD:
            act = 0;
            break;
        case LIMIT_LEARN_CNT_ACTION_DROP:
            act = 1;
            break;
        case LIMIT_LEARN_CNT_ACTION_TO_CPU:
            act = 2;
            break;
        case LIMIT_LEARN_CNT_ACTION_COPY_CPU:
            act = 3;
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    if ((ret = reg_array_field_write(RTL9602BVB_LUT_LEARN_OVER_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act))!=RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_portLimitLearningCntAction_set */



/* Module Name    : L2                                          */
/* Sub-module Name: Parameter for L2 lookup and learning engine */

/* Function Name:
 *      dal_rtl9602bvb_l2_aging_get
 * Description:
 *      Get the dynamic address aging time.
 * Input:
 *      None
 * Output:
 *      pAgingTime - pointer buffer of aging time
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Get aging_time as 0 mean disable aging mechanism. (0.1sec)
 */
int32
dal_rtl9602bvb_l2_aging_get(uint32 *pAgingTime)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pAgingTime), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(RTL9602BVB_LUT_CFGr, RTL9602BVB_AGE_SPDf, pAgingTime)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_aging_get */


/* Function Name:
 *      dal_rtl9602bvb_l2_aging_set
 * Description:
 *      Set the dynamic address aging time.
 * Input:
 *      agingTime - aging time
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT   - invalid input parameter
 * Note:
 *      (1) RTL8329/RTL8389 aging time is not configurable.
 *      (2) apply aging_time as 0 mean disable aging mechanism.
 */
int32
dal_rtl9602bvb_l2_aging_set(uint32 agingTime)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((HAL_L2_AGING_TIME_MAX() < agingTime), RT_ERR_INPUT);

    if ((ret = reg_field_write(RTL9602BVB_LUT_CFGr, RTL9602BVB_AGE_SPDf, &agingTime)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_aging_set */


/* Function Name:
 *      dal_rtl9602bvb_l2_portAgingEnable_get
 * Description:
 *      Get the dynamic address aging out setting of the specified port.
 * Input:
 *      port    - port id
 * Output:
 *      pEnable - pointer to enable status of Age
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_portAgingEnable_get(rtk_port_t port, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  value;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(RTL9602BVB_LUT_AGEOUT_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_AGEOUT_OUTf, &value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    if (value == 1)
        *pEnable = ENABLED;
    else
        *pEnable = DISABLED;

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_portAgingEnable_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_portAgingEnable_set
 * Description:
 *      Set the dynamic address aging out configuration of the specified port
 * Input:
 *      port    - port id
 *      enable  - enable status of Age
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID  - invalid port id
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_portAgingEnable_set(rtk_port_t port, rtk_enable_t enable)
{
    int32   ret;
    uint32  value;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_INPUT);

    if (enable == ENABLED)
        value = 1;
    else
        value = 0;

    if ((ret = reg_array_field_write(RTL9602BVB_LUT_AGEOUT_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_AGEOUT_OUTf, &value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_portAgingEnable_set */

/* Module Name    : L2      */
/* Sub-module Name: Parameter for lookup miss */
/* Function Name:
 *      dal_rtl9602bvb_l2_lookupMissAction_get
 * Description:
 *      Get forwarding action when destination address lookup miss.
 * Input:
 *      type    - type of lookup miss
 * Output:
 *      pAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_INPUT        - invalid type of lookup miss
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_BCAST
 *      - DLF_TYPE_MCAST
 *
 *      Forwarding action is as following:
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_FLOOD_IN_VLAN
 *      - ACTION_FLOOD_IN_ALL_PORT  (only for DLF_TYPE_MCAST)
 *      - ACTION_FLOOD_IN_ROUTER_PORTS (only for DLF_TYPE_IPMC)
 */
int32
dal_rtl9602bvb_l2_lookupMissAction_get(rtk_l2_lookupMissType_t type, rtk_action_t *pAction)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((DLF_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((DLF_TYPE_BCAST == type), RT_ERR_CHIP_NOT_SUPPORTED);
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    if((ret = dal_rtl9602bvb_l2_portLookupMissAction_get(0, type, pAction)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_lookupMissAction_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_lookupMissAction_set
 * Description:
 *      Set forwarding action when destination address lookup miss.
 * Input:
 *      type   - type of lookup miss
 *      action - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_INPUT      - invalid type of lookup miss
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_BCAST
 *      - DLF_TYPE_MCAST
 *
 *      Forwarding action is as following:
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_FLOOD_IN_VLAN
 *      - ACTION_FLOOD_IN_ALL_PORT  (only for DLF_TYPE_MCAST)
 *      - ACTION_FLOOD_IN_ROUTER_PORTS (only for DLF_TYPE_IPMC)
 */
int32
dal_rtl9602bvb_l2_lookupMissAction_set(rtk_l2_lookupMissType_t type, rtk_action_t action)
{
    int32   ret;
    rtk_action_t raw_act;
    rtk_port_t port;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((DLF_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((DLF_TYPE_BCAST == type), RT_ERR_CHIP_NOT_SUPPORTED);
    RT_PARAM_CHK((ACTION_END <= action), RT_ERR_INPUT);

    switch (action)
    {
        case ACTION_FORWARD:
        case ACTION_DROP:
        case ACTION_TRAP2CPU:
            raw_act = action;
            break;
        case ACTION_DROP_EXCLUDE_RMA:
            if(DLF_TYPE_MCAST != type)
                return RT_ERR_CHIP_NOT_SUPPORTED;
            else
                raw_act = action;
            break;
        default:
            return RT_ERR_FAILED;
    }

    switch(type)
    {
        case DLF_TYPE_IPMC:
            HAL_SCAN_ALL_PORT(port)
            {
                if ((ret = reg_array_field_read(RTL9602BVB_UNKN_IP4_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &raw_act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }
            }
            break;
        case DLF_TYPE_IP6MC:
            HAL_SCAN_ALL_PORT(port)
            {
                if ((ret=reg_array_field_write(RTL9602BVB_UNKN_IP6_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &raw_act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }
            }
            break;
        case DLF_TYPE_UCAST:
            HAL_SCAN_ALL_PORT(port)
            {
                if ((ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_UC_DA_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &raw_act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }
            }
            break;
        case DLF_TYPE_MCAST:
            HAL_SCAN_ALL_PORT(port)
            {
                if ((ret = reg_array_field_write(RTL9602BVB_UNKN_L2_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &raw_act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }
            }
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_lookupMissAction_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_portLookupMissAction_get
 * Description:
 *      Get forwarding action of specified port when destination address lookup miss.
 * Input:
 *      port    - port id
 *      type    - type of lookup miss
 * Output:
 *      pAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_INPUT            - invalid type of lookup miss
 *      RT_ERR_NULL_POINTER     - input parameter may be null pointer
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_MCAST
 *      - DLF_TYPE_IP6MC
 *
 *      Forwarding action is as following:
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_FORWARD
 *      - ACTION_DROP_EXCLUDE_RMA (Only for DLF_TYPE_MCAST)
 */
int32
dal_rtl9602bvb_l2_portLookupMissAction_get(rtk_port_t port, rtk_l2_lookupMissType_t type, rtk_action_t *pAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    switch(type)
    {
        case DLF_TYPE_IPMC:
            if ((ret = reg_array_field_read(RTL9602BVB_UNKN_IP4_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }

            switch (act)
            {
                case 0:
                    *pAction = ACTION_FORWARD;
                    break;
                case 1:
                    *pAction = ACTION_DROP;
                    break;
                case 2:
                    *pAction = ACTION_TRAP2CPU;
                    break;
                default:
                    return RT_ERR_NOT_ALLOWED;
            }
            break;
        case DLF_TYPE_IP6MC:
            if (( ret = reg_array_field_read(RTL9602BVB_UNKN_IP6_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }

            switch (act)
            {
                case 0:
                    *pAction = ACTION_FORWARD;
                    break;
                case 1:
                    *pAction = ACTION_DROP;
                    break;
                case 2:
                    *pAction = ACTION_TRAP2CPU;
                    break;
                default:
                    return RT_ERR_NOT_ALLOWED;
            }
            break;
        case DLF_TYPE_UCAST:
            if ((ret = reg_array_field_read(RTL9602BVB_LUT_UNKN_UC_DA_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }

            switch (act)
            {
                case 0:
                    *pAction = ACTION_FORWARD;
                    break;
                case 1:
                    *pAction = ACTION_DROP;
                    break;
                case 2:
                    *pAction = ACTION_TRAP2CPU;
                    break;
                default:
                    return RT_ERR_CHIP_NOT_SUPPORTED;
            }
            break;
        case DLF_TYPE_MCAST:
            if ((ret = reg_array_field_read(RTL9602BVB_UNKN_L2_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            switch (act)
            {
                case 0:
                    *pAction = ACTION_FORWARD;
                    break;
                case 1:
                    *pAction = ACTION_DROP;
                    break;
                case 2:
                    *pAction = ACTION_TRAP2CPU;
                    break;
                case 3:
                    *pAction = ACTION_DROP_EXCLUDE_RMA;
                     break;
                default:
                    return RT_ERR_NOT_ALLOWED;
            }
            break;
        default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
            break;
    }

    return RT_ERR_OK;

} /* end of dal_rtl9602bvb_l2_portLookupMissAction_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_portLookupMissAction_set
 * Description:
 *      Set forwarding action of specified port when destination address lookup miss.
 * Input:
 *      port    - port id
 *      type    - type of lookup miss
 *      action  - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_INPUT            - invalid type of lookup miss
 *      RT_ERR_PORT_ID          - invalid port id
 *      RT_ERR_FWD_ACTION       - invalid forwarding action
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_MCAST
 *      - DLF_TYPE_IP6MC
 *
 *      Forwarding action is as following:
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_FORWARD
 *      - ACTION_DROP_EXCLUDE_RMA (Only for DLF_TYPE_MCAST)
 */
int32
dal_rtl9602bvb_l2_portLookupMissAction_set(rtk_port_t port, rtk_l2_lookupMissType_t type, rtk_action_t action)
{
    int32   ret;
    uint32 act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((type >= DLF_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((action >= ACTION_END), RT_ERR_INPUT);

    switch(type)
    {
        case DLF_TYPE_IPMC:
            switch (action)
            {
                case ACTION_FORWARD:
                    act = 0;
                    break;
                case ACTION_DROP:
                    act = 1;
                    break;
                case ACTION_TRAP2CPU:
                    act = 2;
                    break;
                default:
                    return RT_ERR_CHIP_NOT_SUPPORTED;
            }

            if ((ret = reg_array_field_write(RTL9602BVB_UNKN_IP4_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        case DLF_TYPE_IP6MC:
            switch (action)
            {
                case ACTION_FORWARD:
                    act = 0;
                    break;
                case ACTION_DROP:
                    act = 1;
                    break;
                case ACTION_TRAP2CPU:
                    act = 2;
                    break;
                default:
                    return RT_ERR_CHIP_NOT_SUPPORTED;

            }

            if ((ret=reg_array_field_write(RTL9602BVB_UNKN_IP6_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        case DLF_TYPE_UCAST:
            switch(action)
            {
                case ACTION_FORWARD:
                    act = 0;
                    break;
                case ACTION_DROP:
                    act = 1;
                    break;
                case ACTION_TRAP2CPU:
                    act = 2;
                    break;
                default:
                    return RT_ERR_CHIP_NOT_SUPPORTED;
            }

            if ((ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_UC_DA_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        case DLF_TYPE_MCAST:
            switch (action)
            {
                case ACTION_FORWARD:
                    act = 0;
                    break;
                case ACTION_DROP:
                    act = 1;
                    break;
                case ACTION_TRAP2CPU:
                    act = 2;
                    break;
                case ACTION_DROP_EXCLUDE_RMA:
                    act = 3;
                    break;
                default:
                    return RT_ERR_CHIP_NOT_SUPPORTED;
            }

            if ((ret = reg_array_field_write(RTL9602BVB_UNKN_L2_MCr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
            break;
    }

    return RT_ERR_OK;

} /* end of dal_rtl9602bvb_l2_portLookupMissAction_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_lookupMissFloodPortMask_get
 * Description:
 *      Get flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      type   - type of lookup miss
 * Output:
 *      pFlood_portmask - flooding port mask configuration when unicast/multicast lookup missed.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      DLF_TYPE_IPMC, DLF_TYPE_IP6MC & DLF_TYPE_MCAST shares the same configuration.
 */
int32
dal_rtl9602bvb_l2_lookupMissFloodPortMask_get(rtk_l2_lookupMissType_t type, rtk_portmask_t *pFlood_portmask)
{
    int32   ret;
    rtk_port_t port;
    uint32 act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((DLF_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pFlood_portmask), RT_ERR_NULL_POINTER);

    /* Clear Portmask */
    RTK_PORTMASK_RESET((*pFlood_portmask));

    switch(type)
    {
        case DLF_TYPE_IPMC:
        case DLF_TYPE_IP6MC:
        case DLF_TYPE_MCAST:
            HAL_SCAN_ALL_PORT(port)
            {
                if ((ret = reg_array_field_read(RTL9602BVB_LUT_UNKN_MC_FLOODr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }

                if(1 == act)
                    RTK_PORTMASK_PORT_SET((*pFlood_portmask), port);

                switch(act)
                {
                    case 0:
                        break;
                    case 1:
                        RTK_PORTMASK_PORT_SET((*pFlood_portmask), port);
                        break;
                    default:
                        return RT_ERR_CHIP_NOT_SUPPORTED;
                }
            }
            break;
        case DLF_TYPE_UCAST:
            HAL_SCAN_ALL_PORT(port)
            {
                if ((ret = reg_array_field_read(RTL9602BVB_LUT_UNKN_UC_FLOODr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }

                switch(act)
                {
                    case 0:
                        break;
                    case 1:
                        RTK_PORTMASK_PORT_SET((*pFlood_portmask), port);
                        break;
                    default:
                        return RT_ERR_CHIP_NOT_SUPPORTED;
                }
            }
            break;
        case DLF_TYPE_BCAST:
            HAL_SCAN_ALL_PORT(port)
            {
                if (( ret = reg_array_field_read(RTL9602BVB_LUT_BC_FLOODr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }

                switch(act)
                {
                    case 0:
                        break;
                    case 1:
                        RTK_PORTMASK_PORT_SET((*pFlood_portmask), port);
                        break;
                    default:
                        return RT_ERR_CHIP_NOT_SUPPORTED;
                }
            }
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_lookupMissFloodPortMask_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_lookupMissFloodPortMask_set
 * Description:
 *      Set flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      type            - type of lookup miss
 *      pFlood_portmask - flooding port mask configuration when unicast/multicast lookup missed.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      DLF_TYPE_IPMC, DLF_TYPE_IP6MC & DLF_TYPE_MCAST shares the same configuration.
 */
int32
dal_rtl9602bvb_l2_lookupMissFloodPortMask_set(rtk_l2_lookupMissType_t type, rtk_portmask_t *pFlood_portmask)
{
    int32   ret;
    rtk_port_t port;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((DLF_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pFlood_portmask), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HAL_IS_PORTMASK_VALID((*pFlood_portmask)), RT_ERR_PORT_MASK);

    switch(type)
    {
        case DLF_TYPE_IPMC:
        case DLF_TYPE_IP6MC:
        case DLF_TYPE_MCAST:
            HAL_SCAN_ALL_PORT(port)
            {
                act = (RTK_PORTMASK_IS_PORT_SET((*pFlood_portmask), port)) ? 1 : 0;

                if ((ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_MC_FLOODr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }
            }
            break;
        case DLF_TYPE_UCAST:
            HAL_SCAN_ALL_PORT(port)
            {
                act = (RTK_PORTMASK_IS_PORT_SET((*pFlood_portmask), port)) ? 1 : 0;

                if (( ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_UC_FLOODr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }
            }
            break;
        case DLF_TYPE_BCAST:
            HAL_SCAN_ALL_PORT(port)
            {
                act = (RTK_PORTMASK_IS_PORT_SET((*pFlood_portmask), port)) ? 1 : 0;

                if ((ret = reg_array_field_write(RTL9602BVB_LUT_BC_FLOODr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                    return ret;
                }
            }
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_lookupMissFloodPortMask_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_lookupMissFloodPortMask_add
 * Description:
 *      Add one port member to flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      type        - type of lookup miss
 *      flood_port  - port id that is going to be added in flooding port mask.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      DLF_TYPE_IPMC & DLF_TYPE_MCAST shares the same configuration.
 */
int32
dal_rtl9602bvb_l2_lookupMissFloodPortMask_add(rtk_l2_lookupMissType_t type, rtk_port_t flood_port)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((DLF_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(flood_port), RT_ERR_PORT_ID);

    switch (type)
    {
        case DLF_TYPE_IPMC:
        case DLF_TYPE_IP6MC:
        case DLF_TYPE_MCAST:
            act = 1;
            if ((ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_MC_FLOODr, flood_port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        case DLF_TYPE_UCAST:
            act = 1;
            if (( ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_UC_FLOODr, flood_port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        case DLF_TYPE_BCAST:
            act = 1;
            if ((ret = reg_array_field_write(RTL9602BVB_LUT_BC_FLOODr, flood_port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_lookupMissFloodPortMask_add */

/* Function Name:
 *      dal_rtl9602bvb_l2_lookupMissFloodPortMask_del
 * Description:
 *      Del one port member in flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      type        - type of lookup miss
 *      flood_port  - port id that is going to be added in flooding port mask.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      DLF_TYPE_IPMC & DLF_TYPE_MCAST shares the same configuration..
 */
int32
dal_rtl9602bvb_l2_lookupMissFloodPortMask_del(rtk_l2_lookupMissType_t type, rtk_port_t flood_port)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((DLF_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(flood_port), RT_ERR_PORT_ID);


    switch (type)
    {
        case DLF_TYPE_IPMC:
        case DLF_TYPE_IP6MC:
        case DLF_TYPE_MCAST:
            act = 0;
            if ((ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_MC_FLOODr, flood_port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        case DLF_TYPE_UCAST:
            act = 0;
            if (( ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_UC_FLOODr, flood_port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        case DLF_TYPE_BCAST:
            act = 0;
            if ((ret = reg_array_field_write(RTL9602BVB_LUT_BC_FLOODr, flood_port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &act)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2|MOD_DAL), "");
                return ret;
            }
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_lookupMissFloodPortMask_del */

/* Module Name    : L2      */
/* Sub-module Name: Unicast */
/* Function Name:
 *      dal_rtl9602bvb_l2_newMacOp_get
 * Description:
 *      Get learning mode and forwarding action of new learned address on specified port.
 * Input:
 *      port       - port id
 * Output:
 *      pLrnMode   - pointer to learning mode
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 *
 *      Learning mode is as following
 *      - HARDWARE_LEARNING
 *      - SOFTWARE_LEARNING
 *      - NOT_LEARNING
 */
int32
dal_rtl9602bvb_l2_newMacOp_get(
    rtk_port_t              port,
    rtk_l2_newMacLrnMode_t  *pLrnMode,
    rtk_action_t            *pFwdAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pLrnMode), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pFwdAction), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(RTL9602BVB_LUT_UNKN_SA_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    switch (act)
    {
        case 0:
            *pFwdAction = ACTION_FORWARD;
            break;
        case 1:
            *pFwdAction = ACTION_DROP;
            break;
        case 2:
            *pFwdAction = ACTION_TRAP2CPU;
            break;
        case 3:
            *pFwdAction = ACTION_COPY2CPU;
            break;
        default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
    }

    *pLrnMode = HARDWARE_LEARNING;
    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_newMacOp_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_newMacOp_set
 * Description:
 *      Set learning mode and forwarding action of new learned address on specified port.
 * Input:
 *      port      - port id
 *      lrnMode   - learning mode
 *      fwdAction - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_INPUT      - invalid input parameter
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 *
 *      Learning mode is as following
 *      - HARDWARE_LEARNING
 *      - SOFTWARE_LEARNING
 *      - NOT_LEARNING
 */
int32
dal_rtl9602bvb_l2_newMacOp_set(
    rtk_port_t              port,
    rtk_l2_newMacLrnMode_t  lrnMode,
    rtk_action_t            fwdAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((LEARNING_MODE_END <= lrnMode), RT_ERR_INPUT);
    RT_PARAM_CHK((HARDWARE_LEARNING != lrnMode), RT_ERR_INPUT);
    RT_PARAM_CHK((ACTION_END <= fwdAction), RT_ERR_INPUT);

    switch (fwdAction)
    {
        case ACTION_FORWARD:
            act = 0;
            break;
        case ACTION_DROP:
            act = 1;
            break;
        case ACTION_TRAP2CPU:
            act = 2;
            break;
        case ACTION_COPY2CPU:
            act = 3;
            break;
        default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
    }

    if ((ret = reg_array_field_write(RTL9602BVB_LUT_UNKN_SA_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_newMacOp_set */


/* Module Name    : L2              */
/* Sub-module Name: Get next entry */

/* Function Name:
 *      dal_rtl9602bvb_l2_nextValidAddr_get
 * Description:
 *      Get next valid L2 unicast address entry from the specified device.
 * Input:
 *      pScanIdx      - currently scan index of l2 table to get next.
 *      include_static - the get type, include static mac or not.
 * Output:
 *      pL2UcastData   - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 multicast and ip multicast entry and
 *          reply next valid L2 unicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScanIdx is the input and also is the output argument.
 */
int32
dal_rtl9602bvb_l2_nextValidAddr_get(
    int32               *pScanIdx,
    rtk_l2_ucastAddr_t  *pL2UcastData)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pScanIdx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pL2UcastData), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((*pScanIdx >= HAL_L2_LEARN_LIMIT_CNT_MAX()), RT_ERR_L2_INDEXTABLE_INDEX);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L2UC;
    lut_entry.address = *pScanIdx;
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_L2_ENTRY_NOTFOUND;

    osal_memset(pL2UcastData, 0x00, sizeof(rtk_l2_ucastAddr_t));
    if ((ret = _dal_rtl9602bvb_rawtoL2uc(pL2UcastData, &lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    *pScanIdx = lut_entry.address;
    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_nextValidAddr_get */


/* Function Name:
 *      dal_rtl9602bvb_l2_nextValidAddrOnPort_get
 * Description:
 *      Get next valid L2 unicast address entry from specify port.
 * Input:
 *      pScanIdx      - currently scan index of l2 table to get next.
 * Output:
 *      pL2UcastData  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 multicast and ip multicast entry and
 *          reply next valid L2 unicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScanIdx is the input and also is the output argument.
 */
int32
dal_rtl9602bvb_l2_nextValidAddrOnPort_get(
    rtk_port_t          port,
    int32               *pScanIdx,
    rtk_l2_ucastAddr_t  *pL2UcastData)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pScanIdx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pL2UcastData), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((*pScanIdx >= HAL_L2_LEARN_LIMIT_CNT_MAX()), RT_ERR_L2_INDEXTABLE_INDEX);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L2UCSPA;
    lut_entry.address = *pScanIdx;
    lut_entry.spa = (uint32)port;
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_L2_ENTRY_NOTFOUND;

    osal_memset(pL2UcastData, 0x00, sizeof(rtk_l2_ucastAddr_t));
    if ((ret = _dal_rtl9602bvb_rawtoL2uc(pL2UcastData, &lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    *pScanIdx = lut_entry.address;
    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_nextValidAddrOnPort_get */


/* Function Name:
 *      dal_rtl9602bvb_l2_nextValidMcastAddr_get
 * Description:
 *      Get next valid L2 multicast address entry from the specified device.
 * Input:
 *      pScanIdx - currently scan index of l2 table to get next.
 * Output:
 *      pL2McastData  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 unicast and ip multicast entry and
 *          reply next valid L2 multicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScan_idx is the input and also is the output argument.
 */
int32
dal_rtl9602bvb_l2_nextValidMcastAddr_get(
    int32               *pScanIdx,
    rtk_l2_mcastAddr_t  *pL2McastData)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pScanIdx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pL2McastData), RT_ERR_NULL_POINTER);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L2MC;
    lut_entry.address = *pScanIdx;
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_L2_ENTRY_NOTFOUND;

    osal_memset(pL2McastData, 0x00, sizeof(rtk_l2_mcastAddr_t));
    if ((ret = _dal_rtl9602bvb_rawtoL2mc(pL2McastData, &lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    *pScanIdx = lut_entry.address;

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_nextValidMcastAddr_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_nextValidIpMcastAddr_get
 * Description:
 *      Get next valid L2 ip multicast address entry from the specified device.
 * Input:
 *      pScanIdx - currently scan index of l2 table to get next.
 * Output:
 *      pIpMcastData  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 unicast and multicast entry and
 *          reply next valid L2 ip multicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScan_idx is the input and also is the output argument.
 */
int32
dal_rtl9602bvb_l2_nextValidIpMcastAddr_get(
    int32                   *pScanIdx,
    rtk_l2_ipMcastAddr_t    *pIpMcastData)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pScanIdx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pIpMcastData), RT_ERR_NULL_POINTER);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_L3MC;
    lut_entry.address = *pScanIdx;
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_L2_ENTRY_NOTFOUND;

    osal_memset(pIpMcastData, 0x00, sizeof(rtk_l2_ipMcastAddr_t));
    if ((ret = _dal_rtl9602bvb_rawtoIpmc(pIpMcastData, &lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    *pScanIdx = lut_entry.address;

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_nextValidIpMcastAddr_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_nextValidEntry_get
 * Description:
 *      Get LUT next valid entry.
 * Input:
 *      pScanIdx - Index field in the structure.
 * Output:
 *      pL2Entry - entry content
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_L2_EMPTY_ENTRY   - Empty LUT entry.
 *      RT_ERR_INPUT            - Invalid input parameters.
 * Note:
 *      This API is used to get next valid LUT entry.
 */
int32
dal_rtl9602bvb_l2_nextValidEntry_get(
        int32                   *pScanIdx,
        rtk_l2_addr_table_t     *pL2Entry)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pScanIdx), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pL2Entry), RT_ERR_NULL_POINTER);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
#if 0
    /* Get method should not be the MAC
     * To use LUT_READ_METHOD_MAC, it should use dal_rtl9602bvb_l2_addr_get
     */
    if(pL2Entry->method != LUT_READ_METHOD_MAC)
    {
        _dal_rtl9602bvb_lutReadMethodtoRaw(&lut_entry.method, pL2Entry->method);
    }
    else
    {
        lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_ADDRESS;
    }
#else
    lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_NEXT_ADDRESS;
#endif
    lut_entry.address = *pScanIdx;
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

	if(0 == lut_entry.lookup_hit)
	{
        return RT_ERR_L2_ENTRY_NOTFOUND;
	}

    osal_memset(pL2Entry, 0x00, sizeof(rtk_l2_addr_table_t));
    if(lut_entry.l3lookup == 0)
    {
        if((lut_entry.mac.octet[0] & 0x01) == 0)
        {
            pL2Entry->entryType = RTK_LUT_L2UC;
            if ((ret = _dal_rtl9602bvb_rawtoL2uc(&(pL2Entry->entry.l2UcEntry), &lut_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
                return ret;
            }
        }
        else
        {
            pL2Entry->entryType = RTK_LUT_L2MC;
            if ((ret = _dal_rtl9602bvb_rawtoL2mc(&(pL2Entry->entry.l2McEntry), &lut_entry)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
                return ret;
            }
        }
    }
    else
    {
        if(lut_entry.ip6)
        {
            pL2Entry->entryType = RTK_LUT_L3V6MC;
        }
        else
        {
            pL2Entry->entryType = RTK_LUT_L3MC;
        }
        if ((ret = _dal_rtl9602bvb_rawtoIpmc(&(pL2Entry->entry.ipmcEntry), &lut_entry)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return ret;
        }
    }

    *pScanIdx = lut_entry.address;
    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_nextValidEntry_get */



/* Module Name    : L2              */
/* Sub-module Name: Unicast address */

/* Function Name:
 *      dal_rtl9602bvb_l2_addr_add
 * Description:
 *      Add L2 entry to ASIC.
 * Input:
 *      pL2_addr - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize L2 entry before add it.
 */
int32
dal_rtl9602bvb_l2_addr_add(rtk_l2_ucastAddr_t *pL2Addr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pL2Addr), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pL2Addr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(((pL2Addr->mac.octet[0] & 0x01) != 0), RT_ERR_MAC);
    RT_PARAM_CHK((pL2Addr->fid > HAL_VLAN_FID_MAX()), RT_ERR_L2_FID);
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(pL2Addr->port), RT_ERR_PORT_ID);
    RT_PARAM_CHK(!HAL_IS_EXT_PORT(pL2Addr->ext_port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((pL2Addr->flags > RTK_L2_UCAST_FLAG_ALL), RT_ERR_INPUT);
    RT_PARAM_CHK((pL2Addr->age > HAL_L2_ENTRY_AGING_MAX()), RT_ERR_INPUT);
    RT_PARAM_CHK((pL2Addr->auth >= RTK_ENABLE_END), RT_ERR_INPUT);

    osal_memset(&lut_entry, 0x00, sizeof(rtl9602bvb_lut_table_t));

    if ((ret = _dal_rtl9602bvb_l2uctoRaw(&lut_entry, pL2Addr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    lut_entry.valid      = ENABLED;
    lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L2UC;
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_set(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_FAILED;

    pL2Addr->index = lut_entry.address;
    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_addr_add */

/* Function Name:
 *      dal_rtl9602bvb_l2_addr_del
 * Description:
 *      Delete a L2 unicast address entry.
 * Input:
 *      pL2Addr  - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      If the mac has existed in the LUT, it will be deleted. Otherwise, it will return RT_ERR_L2_ENTRY_NOTFOUND.
 */
int32
dal_rtl9602bvb_l2_addr_del(rtk_l2_ucastAddr_t *pL2Addr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pL2Addr), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pL2Addr->flags > RTK_L2_UCAST_FLAG_ALL), RT_ERR_INPUT);
    RT_PARAM_CHK(((pL2Addr->mac.octet[0] & 0x01) != 0), RT_ERR_MAC);
    if(pL2Addr->flags & RTK_L2_UCAST_FLAG_IVL)
        RT_PARAM_CHK((pL2Addr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    else
        RT_PARAM_CHK((pL2Addr->fid > HAL_VLAN_FID_MAX()), RT_ERR_L2_FID);

    osal_memset(&lut_entry, 0x00, sizeof(rtl9602bvb_lut_table_t));

    if ((ret = _dal_rtl9602bvb_l2uctoRaw(&lut_entry, pL2Addr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L2UC;
    lut_entry.valid      = DISABLED;
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_set(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_addr_del */


/* Function Name:
 *      dal_rtl9602bvb_l2_addr_get
 * Description:
 *      Get L2 entry based on specified vid and MAC address
 * Input:
 *      None
 * Output:
 *      pL2Addr - pointer to L2 entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vlan id
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      If the unicast mac address existed in LUT, it will return the port and fid where
 *      the mac is learned. Otherwise, it will return a RT_ERR_L2_ENTRY_NOTFOUND error.
 */
int32
dal_rtl9602bvb_l2_addr_get(rtk_l2_ucastAddr_t *pL2Addr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pL2Addr), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pL2Addr->flags > RTK_L2_UCAST_FLAG_ALL), RT_ERR_INPUT);
    RT_PARAM_CHK(((pL2Addr->mac.octet[0] & 0x01) != 0), RT_ERR_MAC);
    if(pL2Addr->flags & RTK_L2_UCAST_FLAG_IVL)
        RT_PARAM_CHK((pL2Addr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    else
        RT_PARAM_CHK((pL2Addr->fid > HAL_VLAN_FID_MAX()), RT_ERR_L2_FID);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    if ((ret = _dal_rtl9602bvb_l2uctoRaw(&lut_entry, pL2Addr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_MAC;
    lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L2UC;
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_L2_ENTRY_NOTFOUND;

    osal_memset(pL2Addr, 0x00, sizeof(rtk_l2_ucastAddr_t));
    if ((ret = _dal_rtl9602bvb_rawtoL2uc(pL2Addr, &lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_addr_get */


/* Function Name:
 *      dal_rtl9602bvb_l2_addr_delAll
 * Description:
 *      Delete all L2 unicast address entry.
 * Input:
 *      includeStatic - include static mac or not?
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_addr_delAll(uint32 includeStatic)
{
    int32   ret;
    rtl9602bvb_raw_flush_ctrl_t flushCtrl, originalCtrl;
    rtk_port_t port;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((TRUE < includeStatic), RT_ERR_INPUT);

    /* Get Current Flush Control */
    if((ret = rtl9602bvb_raw_l2_flushCtrl_get(&originalCtrl)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    /* Set Flush Control to BOTH */
    osal_memset(&flushCtrl, 0x00, sizeof(rtl9602bvb_raw_flush_ctrl_t));
    if (ENABLED == includeStatic)
        flushCtrl.flushType = RTL9602BVB_RAW_FLUSH_TYPE_BOTH;
    else
        flushCtrl.flushType = RTL9602BVB_RAW_FLUSH_TYPE_DYNAMIC;

    flushCtrl.flushMode = RTL9602BVB_RAW_FLUSH_MODE_PORT;
    if((ret = rtl9602bvb_raw_l2_flushCtrl_set(&flushCtrl)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    /* Start Flushing */
    HAL_SCAN_ALL_PORT(port)
    {
        if((ret = rtl9602bvb_raw_l2_flushEn_set(port, ENABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return ret;
        }
    }

    /* Recover to original Flush Control */
    if((ret = rtl9602bvb_raw_l2_flushCtrl_set(&originalCtrl)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_addr_delAll */

/* Module Name    : L2           */
/* Sub-module Name: l2 multicast */

/* Function Name:
 *      dal_rtl9602bvb_l2_mcastAddr_add
 * Description:
 *      Add L2 multicast entry to ASIC.
 * Input:
 *      pMcastAddr - L2 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize L2 multicast entry before add it.
 */
int32
dal_rtl9602bvb_l2_mcastAddr_add(rtk_l2_mcastAddr_t *pMcastAddr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMcastAddr), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pMcastAddr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    RT_PARAM_CHK(((pMcastAddr->mac.octet[0] & 0x01) != 1), RT_ERR_MAC);
    RT_PARAM_CHK((pMcastAddr->fid > HAL_VLAN_FID_MAX()), RT_ERR_L2_FID);
    RT_PARAM_CHK(!HAL_IS_PORTMASK_VALID(pMcastAddr->portmask), RT_ERR_PORT_MASK);
    RT_PARAM_CHK(!HAL_IS_EXTPORTMASK_VALID(pMcastAddr->ext_portmask), RT_ERR_PORT_MASK);
    RT_PARAM_CHK((pMcastAddr->flags > RTK_L2_MCAST_FLAG_ALL), RT_ERR_INPUT);

    if( (0 == RTK_PORTMASK_GET_PORT_COUNT(pMcastAddr->ext_portmask)) && (RTK_PORTMASK_IS_PORT_SET(pMcastAddr->portmask, HAL_GET_CPU_PORT())) )
    {
        /* CPU port is set to member port mask bu no extnsion port set */
        /* Set ext port 0 at extension port mask */
        RTK_PORTMASK_PORT_SET(pMcastAddr->ext_portmask, HAL_GET_MIN_EXT_PORT());
    }

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    if((ret = _dal_rtl9602bvb_l2mctoRaw(&lut_entry, pMcastAddr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L2MC;
    lut_entry.nosalearn  = ENABLED;
    lut_entry.valid      = ENABLED;
    if((ret = rtl9602bvb_raw_l2_lookUpTb_set(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_FAILED;

    pMcastAddr->index = lut_entry.address;
    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_mcastAddr_add */

/* Function Name:
 *      dal_rtl9602bvb_l2_mcastAddr_del
 * Description:
 *      Delete a L2 multicast address entry.
 * Input:
 *      pMac - multicast mac address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_mcastAddr_del(rtk_l2_mcastAddr_t *pMcastAddr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMcastAddr), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(((pMcastAddr->mac.octet[0] & 0x01) != 1), RT_ERR_MAC);
    RT_PARAM_CHK((pMcastAddr->flags > RTK_L2_MCAST_FLAG_ALL), RT_ERR_INPUT);
    if(pMcastAddr->flags & RTK_L2_MCAST_FLAG_IVL)
        RT_PARAM_CHK((pMcastAddr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    else
        RT_PARAM_CHK((pMcastAddr->fid > HAL_VLAN_FID_MAX()), RT_ERR_L2_FID);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    if((ret = _dal_rtl9602bvb_l2mctoRaw(&lut_entry, pMcastAddr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L2MC;
    lut_entry.valid      = DISABLED;
    if((ret = rtl9602bvb_raw_l2_lookUpTb_set(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_mcastAddr_del */

/* Function Name:
 *      dal_rtl9602bvb_l2_mcastAddr_get
 * Description:
 *      Update content of L2 multicast entry.
 * Input:
 *      pMcastAddr - L2 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_mcastAddr_get(rtk_l2_mcastAddr_t *pMcastAddr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMcastAddr), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(((pMcastAddr->mac.octet[0] & 0x01) != 1), RT_ERR_MAC);
    RT_PARAM_CHK((pMcastAddr->flags > RTK_L2_MCAST_FLAG_ALL), RT_ERR_INPUT);
    if(pMcastAddr->flags & RTK_L2_MCAST_FLAG_IVL)
        RT_PARAM_CHK((pMcastAddr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    else
        RT_PARAM_CHK((pMcastAddr->fid > HAL_VLAN_FID_MAX()), RT_ERR_L2_FID);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    if ((ret = _dal_rtl9602bvb_l2mctoRaw(&lut_entry, pMcastAddr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L2MC;
    lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_MAC;

    if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_L2_ENTRY_NOTFOUND;

    osal_memset(pMcastAddr, 0x00, sizeof(rtk_l2_mcastAddr_t));
    if ((ret = _dal_rtl9602bvb_rawtoL2mc(pMcastAddr, &lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_mcastAddr_get */

/* Module Name    : L2        */
/* Sub-module Name: Port move */
/* Function Name:
 *      dal_rtl9602bvb_l2_illegalPortMoveAction_get
 * Description:
 *      Get forwarding action when illegal port moving happen on specified port.
 * Input:
 *      port       - port id
 * Output:
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
int32
dal_rtl9602bvb_l2_illegalPortMoveAction_get(
    rtk_port_t          port,
    rtk_action_t        *pFwdAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pFwdAction), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(RTL9602BVB_LUT_UNMATCHED_SA_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    switch (act)
    {
        case 0:
            *pFwdAction = ACTION_FORWARD;
            break;
        case 1:
            *pFwdAction = ACTION_DROP;
            break;
        case 2:
            *pFwdAction = ACTION_TRAP2CPU;
            break;
        case 3:
            *pFwdAction = ACTION_COPY2CPU;
            break;
        default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_illegalPortMoveAction_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_illegalPortMoveAction_set
 * Description:
 *      Set forwarding action when illegal port moving happen on specified port.
 * Input:
 *      port      - port id
 *      fwdAction - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
int32
dal_rtl9602bvb_l2_illegalPortMoveAction_set(
    rtk_port_t          port,
    rtk_action_t        fwdAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((ACTION_END <= fwdAction), RT_ERR_INPUT);

    switch (fwdAction)
    {
        case ACTION_FORWARD:
            act = 0;
            break;
        case ACTION_DROP:
            act = 1;
            break;
        case ACTION_TRAP2CPU:
            act = 2;
            break;
        case ACTION_COPY2CPU:
            act = 3;
            break;
        default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
    }

    if ((ret = reg_array_field_write(RTL9602BVB_LUT_UNMATCHED_SA_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ACTf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_illegalPortMoveAction_set */


/* Module Name    : L2           */
/* Sub-module Name: IP multicast */


/* Function Name:
 *      dal_rtl9602bvb_l2_ipmcMode_get
 * Description:
 *      Get lookup mode of layer2 ip multicast switching.
 * Input:
 *      None
 * Output:
 *      pMode - pointer to lookup mode of layer2 ip multicast switching
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Lookup mode of layer2 ip multicast switching is as following
 *      - LOOKUP_ON_DIP_AND_SIP
 *      - LOOKUP_ON_MAC_AND_VID_FID
 *      - LOOKUP_ON_DPI_AND_VID
 */
int32
dal_rtl9602bvb_l2_ipmcMode_get(rtk_l2_ipmcMode_t *pMode)
{
    int32   ret;
    uint32  type;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMode), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(RTL9602BVB_LUT_CFGr, RTL9602BVB_LUT_IPMC_HASHf, &type)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    switch(type)
    {
        case 0:
            *pMode = LOOKUP_ON_MAC_AND_VID_FID;
            break;

        case 1:
            *pMode = LOOKUP_ON_DIP_AND_VID_FID;
            break;

        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_ipmcMode_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_ipmcMode_set
 * Description:
 *      Set lookup mode of layer2 ip multicast switching.
 * Input:
 *      mode - lookup mode of layer2 ip multicast switching
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      Lookup mode of layer2 ip multicast switching is as following
 *      - LOOKUP_ON_DIP_AND_SIP
 *      - LOOKUP_ON_MAC_AND_VID_FID
 *      - LOOKUP_ON_DPI_AND_VID
 */
int32
dal_rtl9602bvb_l2_ipmcMode_set(rtk_l2_ipmcMode_t mode)
{
    int32   ret;
    uint32  type;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((IPMC_MODE_END <= mode), RT_ERR_INPUT);

    switch(mode)
    {
        case LOOKUP_ON_MAC_AND_VID_FID:
            type = 0;
            break;

        case LOOKUP_ON_DIP_AND_VID_FID:
            type = 1;
            break;

        default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
            break;
    }

    if ((ret = reg_field_write(RTL9602BVB_LUT_CFGr, RTL9602BVB_LUT_IPMC_HASHf, &type)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_ipmcMode_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_ipv6mcMode_get
 * Description:
 *      Get lookup mode of layer2 ipv6 multicast switching.
 * Input:
 *      None
 * Output:
 *      pMode - pointer to lookup mode of layer2 ipv6 multicast switching
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Lookup mode of layer2 ipv6 multicast switching is as following
 *      - LOOKUP_ON_MAC_AND_VID_FID
 *      - LOOKUP_ON_DPI
 */
int32
dal_rtl9602bvb_l2_ipv6mcMode_get(rtk_l2_ipmcMode_t *pMode)
{
    int32   ret;
    uint32  type;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMode), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(RTL9602BVB_LUT_CFGr, RTL9602BVB_LUT_IP6MC_HASHf, &type)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    switch(type)
    {
        case 0:
            *pMode = LOOKUP_ON_MAC_AND_VID_FID;
            break;

        case 1:
            *pMode = LOOKUP_ON_DIP;
            break;

        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_ipv6mcMode_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_ipv6mcMode_set
 * Description:
 *      Set lookup mode of layer2 ipv6 multicast switching.
 * Input:
 *      mode - lookup mode of layer2 ipv6 multicast switching
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      Lookup mode of layer2 ipv6 multicast switching is as following
 *      - LOOKUP_ON_MAC_AND_VID_FID
 *      - LOOKUP_ON_DPI
 */
int32
dal_rtl9602bvb_l2_ipv6mcMode_set(rtk_l2_ipmcMode_t mode)
{
    int32   ret;
    uint32  type;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((IPMC_MODE_END <= mode), RT_ERR_INPUT);

    switch(mode)
    {
        case LOOKUP_ON_MAC_AND_VID_FID:
            type = 0;
            break;

        case LOOKUP_ON_DIP:
            type = 1;
            break;

        default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
            break;
    }

    if ((ret = reg_field_write(RTL9602BVB_LUT_CFGr, RTL9602BVB_LUT_IP6MC_HASHf, &type)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_ipmcMode_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_ipmcSipFilter_set
 * Description:
 *      Set an entry to IPMC SIP filter Table by index
 * Input:
 *      index       - table index
 *      sip         - SIP to be filtered
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 *      
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_ipmcSipFilter_set(uint32 index, ipaddr_t sip)
{
    int ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(index > RTL9602BVB_RAW_LUT_SIPIDXMAX, RT_ERR_OUT_OF_RANGE);

    if ((ret = reg_array_field_write(RTL9602BVB_IGMP_SIP_FILTER_TBLr, REG_ARRAY_INDEX_NONE, index, RTL9602BVB_SIPf, (uint32 *)&sip)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_rtl9602bvb_l2_ipmcSipFilter_get
 * Description:
 *      Get an entry from IPMC SIP filter Table by index
 * Input:
 *      index       - table index
 * Output:
 *      pSip        - SIP buffer to store the retrieved data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 *      
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_ipmcSipFilter_get(uint32 index, ipaddr_t *pSip)
{
    int ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(index > RTL9602BVB_RAW_LUT_SIPIDXMAX, RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(NULL == pSip, RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(RTL9602BVB_IGMP_SIP_FILTER_TBLr, REG_ARRAY_INDEX_NONE, index, RTL9602BVB_SIPf, (uint32 *)pSip)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_rtl9602bvb_l2_portIpmcAction_get
 * Description:
 *      Get the Action of IPMC packet per ingress port.
 * Input:
 *      port        - Ingress port number
 * Output:
 *      pAction     - IPMC packet action (ACTION_FORWARD or ACTION_DROP)
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_portIpmcAction_get(rtk_port_t port, rtk_action_t *pAction)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2), "port=%d",port);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pAction), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(RTL9602BVB_IGMP_P_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ALLOW_MC_DATAf, (uint32 *)&act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    switch(act)
	{
	 	case 1:
	 		*pAction = ACTION_FORWARD;
			break;
	 	case 0:
	 		*pAction = ACTION_DROP;
			break;
		default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
	}

    return RT_ERR_OK;
}   /* end of dal_rtl9602bvb_l2_portIpmcAction_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_portIpmcAction_set
 * Description:
 *      Set the Action of IPMC packet per ingress port.
 * Input:
 *      port        - Ingress port number
 *      action      - IPMC packet action (ACTION_FORWARD or ACTION_DROP)
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_INPUT      - Invalid input parameter
 * Note:
 *      None
 */
int32
dal_rtl9602bvb_l2_portIpmcAction_set(rtk_port_t port, rtk_action_t action)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2), "port=%d,action=%d",port, action);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK(( (ACTION_FORWARD != action) && (ACTION_DROP != action) ), RT_ERR_INPUT);

    switch(action)
	{
	 	case ACTION_FORWARD:
	 		act = 1;
			break;
	 	case ACTION_DROP:
	 		act = 0;
			break;
		default:
            return RT_ERR_CHIP_NOT_SUPPORTED;
	}

    if ((ret = reg_array_field_write(RTL9602BVB_IGMP_P_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ALLOW_MC_DATAf, (uint32 *)&act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}   /* end of dal_rtl9602bvb_l2_portIpmcAction_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_ipMcastAddr_add
 * Description:
 *      Add IP multicast entry to ASIC.
 * Input:
 *      pIpmcastAddr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT      - The module is not initial
 *      RT_ERR_IPV4_ADDRESS  - Invalid IPv4 address
 *      RT_ERR_VLAN_VID      - invalid vlan id
 *      RT_ERR_NULL_POINTER  - input parameter may be null pointer
 *      RT_ERR_INPUT         - invalid input parameter
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
int32
dal_rtl9602bvb_l2_ipMcastAddr_add(rtk_l2_ipMcastAddr_t *pIpmcastAddr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pIpmcastAddr), RT_ERR_NULL_POINTER);
    if(!(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_IPV6))
    {
        RT_PARAM_CHK(( (pIpmcastAddr->dip > 0xEFFFFFFF) || (pIpmcastAddr->dip < 0xE0000000) ), RT_ERR_INPUT);
    }
    RT_PARAM_CHK(!HAL_IS_PORTMASK_VALID(pIpmcastAddr->portmask), RT_ERR_PORT_MASK);
    RT_PARAM_CHK(!HAL_IS_EXTPORTMASK_VALID(pIpmcastAddr->ext_portmask), RT_ERR_PORT_MASK);
    RT_PARAM_CHK((pIpmcastAddr->flags > RTK_L2_IPMCAST_FLAG_ALL), RT_ERR_INPUT);
    if( (0 == RTK_PORTMASK_GET_PORT_COUNT(pIpmcastAddr->ext_portmask)) && (RTK_PORTMASK_IS_PORT_SET(pIpmcastAddr->portmask, HAL_GET_CPU_PORT())) )
    {
        /* CPU port is set to member port mask bu no extnsion port set */
        /* Set ext port 0 at extension port mask */
        RTK_PORTMASK_PORT_SET(pIpmcastAddr->ext_portmask, HAL_GET_MIN_EXT_PORT());
    }

    if(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_IVL)
        RT_PARAM_CHK((pIpmcastAddr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    else
        RT_PARAM_CHK((pIpmcastAddr->fid > RTL9602BVB_RAW_LUT_FIDMAX), RT_ERR_L2_FID);

    if(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_SIP_FILTER)
        RT_PARAM_CHK((pIpmcastAddr->sip_index > RTL9602BVB_RAW_LUT_SIPIDXMAX), RT_ERR_L2_SIP_INDEX);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    if ((ret = _dal_rtl9602bvb_IpmctoRaw(&lut_entry, pIpmcastAddr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if((0 == lut_entry.ip6))
    {
        lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L3MC_ROUTE;
    }
    else
    {
        lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L3IP6MC;
    }
    lut_entry.l3lookup   = ENABLED;
    lut_entry.valid      = ENABLED;

    if ((ret = rtl9602bvb_raw_l2_lookUpTb_set(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_FAILED;

    pIpmcastAddr->index = lut_entry.address;
    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_ipMcastAddr_add */

/* Function Name:
 *      dal_rtl9602bvb_l2_ipMcastAddr_del
 * Description:
 *      Delete a L2 ip multicast address entry from the specified device.
 * Input:
 *      pIpmcastAddr  - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      (1) In vlan unaware mode (SVL), the vid will be ignore, suggest to
 *          input vid=0 in vlan unaware mode.
 *      (2) In vlan aware mode (IVL), the vid will be care.
 */
int32
dal_rtl9602bvb_l2_ipMcastAddr_del(rtk_l2_ipMcastAddr_t *pIpmcastAddr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pIpmcastAddr), RT_ERR_NULL_POINTER);
    if(!(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_IPV6))
    {
        RT_PARAM_CHK(( (pIpmcastAddr->dip > 0xEFFFFFFF) || (pIpmcastAddr->dip < 0xE0000000) ), RT_ERR_INPUT);
    }
    RT_PARAM_CHK((pIpmcastAddr->flags > RTK_L2_IPMCAST_FLAG_ALL), RT_ERR_PORT_MASK);

    if(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_IVL)
        RT_PARAM_CHK((pIpmcastAddr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    else
        RT_PARAM_CHK((pIpmcastAddr->fid > RTL9602BVB_RAW_LUT_FIDMAX), RT_ERR_L2_FID);

    if(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_SIP_FILTER)
        RT_PARAM_CHK((pIpmcastAddr->sip_index > RTL9602BVB_RAW_LUT_SIPIDXMAX), RT_ERR_L2_SIP_INDEX);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    if ((ret = _dal_rtl9602bvb_IpmctoRaw(&lut_entry, pIpmcastAddr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if((0 == lut_entry.ip6))
    {
        lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L3MC_ROUTE;
    }
    else
    {
        lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L3IP6MC;
    }
    lut_entry.l3lookup = ENABLED;
    lut_entry.valid = DISABLED;
    
    if ((ret = rtl9602bvb_raw_l2_lookUpTb_set(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_ipMcastAddr_del */

/* Function Name:
 *      dal_rtl9602bvb_l2_ipMcastAddr_get
 * Description:
 *      Get IP multicast entry on specified dip and sip.
 * Input:
 *      pIpmcastAddr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV4_ADDRESS - Invalid IPv4 address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
int32
dal_rtl9602bvb_l2_ipMcastAddr_get(rtk_l2_ipMcastAddr_t *pIpmcastAddr)
{
    int32   ret;
    rtl9602bvb_lut_table_t lut_entry;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pIpmcastAddr), RT_ERR_NULL_POINTER);
    if(!(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_IPV6))
    {
        RT_PARAM_CHK(( (pIpmcastAddr->dip > 0xEFFFFFFF) || (pIpmcastAddr->dip < 0xE0000000) ), RT_ERR_INPUT);
    }
    RT_PARAM_CHK((pIpmcastAddr->flags > RTK_L2_IPMCAST_FLAG_ALL), RT_ERR_PORT_MASK);

    if(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_IVL)
        RT_PARAM_CHK((pIpmcastAddr->vid > RTK_VLAN_ID_MAX), RT_ERR_VLAN_VID);
    else
        RT_PARAM_CHK((pIpmcastAddr->fid > RTL9602BVB_RAW_LUT_FIDMAX), RT_ERR_L2_FID);

    if(pIpmcastAddr->flags & RTK_L2_IPMCAST_FLAG_SIP_FILTER)
        RT_PARAM_CHK((pIpmcastAddr->sip_index > RTL9602BVB_RAW_LUT_SIPIDXMAX), RT_ERR_L2_SIP_INDEX);

    osal_memset(&lut_entry, 0x0, sizeof(rtl9602bvb_lut_table_t));
    if ((ret = _dal_rtl9602bvb_IpmctoRaw(&lut_entry, pIpmcastAddr)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if((0 == lut_entry.ip6))
    {
        lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L3MC_ROUTE;
    }
    else
    {
        lut_entry.table_type = RTL9602BVB_RAW_LUT_ENTRY_TYPE_L3IP6MC;
    }
    lut_entry.method = RTL9602BVB_RAW_LUT_READ_METHOD_MAC;
    lut_entry.l3lookup = ENABLED;

    if ((ret = rtl9602bvb_raw_l2_lookUpTb_get(&lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    if(0 == lut_entry.lookup_hit)
        return RT_ERR_L2_ENTRY_NOTFOUND;

    osal_memset(pIpmcastAddr, 0x00, sizeof(rtk_l2_ipMcastAddr_t));
    if ((ret = _dal_rtl9602bvb_rawtoIpmc(pIpmcastAddr, &lut_entry)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_ipMcastAddr_get */


/* Module Name    : L2                 */
/* Sub-module Name: Parameter for MISC */


/* Function Name:
 *      dal_rtl9602bvb_l2_srcPortEgrFilterMask_get
 * Description:
 *      Get source port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      None
 * Output:
 *      pFilter_portmask - source port egress filtering configuration when packet egress.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected.
 *      Get permittion status for frames if its source port is equal to destination port.
 */
int32
dal_rtl9602bvb_l2_srcPortEgrFilterMask_get(rtk_portmask_t *pFilter_portmask)
{
    int32   ret;
    rtk_port_t port;
    uint32 permit;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pFilter_portmask), RT_ERR_NULL_POINTER);

    RTK_PORTMASK_RESET((*pFilter_portmask));

    HAL_SCAN_ALL_PORT(port)
    {
        if ((ret = reg_array_field_read(RTL9602BVB_L2_SRC_PORT_PERMITr, (uint32)port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &permit)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2|MOD_DAL), "");
            return ret;
        }

        if(DISABLED == permit)
            RTK_PORTMASK_PORT_SET((*pFilter_portmask), port);
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_srcPortEgrFilterMask_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_srcPortEgrFilterMask_set
 * Description:
 *      Set source port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      pFilter_portmask - source port egress filtering configuration when packet egress.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected
 */
int32
dal_rtl9602bvb_l2_srcPortEgrFilterMask_set(rtk_portmask_t *pFilter_portmask)
{
    int32   ret;
    rtk_port_t port;
    uint32 permit;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pFilter_portmask), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HAL_IS_PORTMASKPRT_VALID(pFilter_portmask), RT_ERR_INPUT);

    HAL_SCAN_ALL_PORT(port)
    {
        if(RTK_PORTMASK_IS_PORT_SET((*pFilter_portmask), port))
            permit = DISABLED;
        else
            permit = ENABLED;

        if ((ret = reg_array_field_write(RTL9602BVB_L2_SRC_PORT_PERMITr, (uint32)port, REG_ARRAY_INDEX_NONE, RTL9602BVB_ENf, &permit)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2|MOD_DAL), "");
            return ret;
        }
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_srcPortEgrFilterMask_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_extPortEgrFilterMask_get
 * Description:
 *      Get extension port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      None
 * Output:
 *      pExt_portmask - extension port egress filtering configuration when packet egress.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected.
 *      Get permittion status for frames if its source port is equal to destination port.
 */
int32
dal_rtl9602bvb_l2_extPortEgrFilterMask_get(rtk_portmask_t *pExt_portmask)
{
    int32   ret;
    rtk_port_t port;
    uint32 permit;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pExt_portmask), RT_ERR_NULL_POINTER);

    RTK_PORTMASK_RESET((*pExt_portmask));

    HAL_SCAN_ALL_EXT_PORT(port)
    {
        if ((ret = reg_array_field_read(RTL9602BVB_L2_SRC_EXT_PERMITr, REG_ARRAY_INDEX_NONE, port, RTL9602BVB_ENf, &permit)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2|MOD_DAL), "");
            return ret;
        }

        if(DISABLED == permit)
            RTK_PORTMASK_PORT_SET((*pExt_portmask), port);
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_extPortEgrFilterMask_get */

/* Function Name:
 *      dal_rtl9602bvb_l2_extPortEgrFilterMask_set
 * Description:
 *      Set extension port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      pExt_portmask - extension port egress filtering configuration when packet egress.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected
 */
int32
dal_rtl9602bvb_l2_extPortEgrFilterMask_set(rtk_portmask_t *pExt_portmask)
{
    int32   ret;
    rtk_port_t port;
    uint32 permit;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pExt_portmask), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HAL_IS_EXTPORTMASK_VALID((*pExt_portmask)), RT_ERR_PORT_MASK);

    HAL_SCAN_ALL_EXT_PORT(port)
    {
        if(RTK_PORTMASK_IS_PORT_SET((*pExt_portmask), port))
            permit = DISABLED;
        else
            permit = ENABLED;

        if ((ret = reg_array_field_write(RTL9602BVB_L2_SRC_EXT_PERMITr, REG_ARRAY_INDEX_NONE, (uint32)port, RTL9602BVB_ENf, &permit)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_L2|MOD_DAL), "");
            return ret;
        }
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_extPortEgrFilterMask_set */

/* Function Name:
 *      dal_rtl9602bvb_l2_camState_set
 * Description:
 *      Set LUT cam state 
 * Input:
 *      camState - enable or disable cam state 
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      
 */
int32
dal_rtl9602bvb_l2_camState_set(rtk_enable_t camState)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((camState >= RTK_ENABLE_END), RT_ERR_INPUT);

    if (camState == ENABLED)
        act = 0;
    else
        act = 1;

    if ((ret = reg_field_write(RTL9602BVB_LUT_CFGr, RTL9602BVB_BCAM_DISf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_camState_set */


/* Function Name:
 *      dal_rtl9602bvb_l2_camState_get
 * Description:
 *      Get LUT cam state 
 * Input:
 *      pCamState - status of cam state 
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *       
 */
int32
dal_rtl9602bvb_l2_camState_get(rtk_enable_t *pCamState)
{
    int32   ret;
    uint32  act;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L2),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(l2_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pCamState), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(RTL9602BVB_LUT_CFGr, RTL9602BVB_BCAM_DISf, &act)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_L2), "");
        return ret;
    }

    if (1 == act)
        *pCamState = DISABLED;
    else
        *pCamState = ENABLED;

    return RT_ERR_OK;
} /* end of dal_rtl9602bvb_l2_camState_get */

