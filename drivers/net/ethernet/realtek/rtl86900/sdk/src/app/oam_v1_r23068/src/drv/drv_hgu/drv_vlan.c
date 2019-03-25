/*
 * Copyright (C) 2012 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 
 * $Date: 
 *
 * Purpose :            
 *
 * Feature :           
 *
 */

/*
 * Include Files
 */
#ifdef CONFIG_HGU_APP
#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <osal/print.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#include "ctc_mc.h"

#include <sys_def.h>
#include "drv_convert.h"
#include "drv_vlan.h"
#include "drv_mac.h"
#include <sys_portmask.h>

/* RG related include */
#include "rtk_rg_define.h"
#include "rtk_rg_struct.h"

static ctc_vlan_cfg_t m_astUniVlanMode[MAX_PORT_NUM + 1];

/* use different fidmode based on mcControlType
   MC_CTRL_GDA_MAC : VLAN_FID_SVL
   other types: VLAN_FID_IVL */
int32 drv_vlan_set_fidMode(uint32 ulVlanId, int fidMode)
{//change ASIC vlan[ulVlanId] fidMode
	rtk_rg_cvlan_info_t cvlan_info;
	int ret = RT_ERR_FAILED;
	if ((0 != ulVlanId) && (!VALID_VLAN_ID(ulVlanId)))
	{
		return RT_ERR_INPUT;
	}
			
	/*fill cvlan info*/
	memset( &cvlan_info, 0,sizeof(rtk_rg_cvlan_info_t) );
	cvlan_info.vlanId = ulVlanId;
	/* should check if VLAN ID has existed first */
	ret = rtk_rg_cvlan_get(&cvlan_info);
	if(RT_ERR_RG_OK == ret)//this vlan has exist
	{//add member only
		cvlan_info.isIVL = (fidMode == VLAN_FID_IVL)?1:0;
		ret = rtk_rg_cvlan_add(&cvlan_info);
		if( RT_ERR_RG_OK != ret )
		{
			SYS_PRINTF("%s[%d]:add cvlan table error!\n",__func__,__LINE__);
			return RT_ERR_FAILED;
		}
	
		//(void)drv_fid_set_by_vid(ulVlanId, uiFid, stPhyMaskTmp, stPhyMaskUntagTmp);
		return RT_ERR_OK;
	}
	return RT_ERR_FAILED;
}

/*clf rule: only add user vid for hw l2 mcast entry check*/
int32 drv_clf_translation_vlan_member_add(uint32 usrvlan,rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag)
{
	uint32 uiFid;
	rtk_portmask_t stPhyMaskTmp;
	rtk_portmask_t stPhyMaskUntagTmp;
	
	if ((0 != usrvlan) && (!VALID_VLAN_ID(usrvlan)))
	{
		return RT_ERR_INPUT;
	}

	(void)drv_valid_fid_get(usrvlan, &uiFid);
	if (uiFid > HAL_VLAN_FID_MAX())
	{
	    return RT_ERR_FAILED;
	}
		
	RTK_PORTMASK_RESET(stPhyMaskTmp);
	RTK_PORTMASK_RESET(stPhyMaskUntagTmp);

	(void)drv_mc_vlan_mem_get(usrvlan, &stPhyMaskTmp, &stPhyMaskUntagTmp);
	RTK_PORTMASK_OR(stPhyMaskTmp, stPhyMask);
	RTK_PORTMASK_OR(stPhyMaskUntagTmp, stPhyMaskUntag);
	
	RTK_PORTMASK_OR(stPhyMask, stPhyMaskTmp);
	RTK_PORTMASK_OR(stPhyMaskUntag, stPhyMaskUntagTmp);

	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT()); 

	/*port members exclude ctc transparent ports ,uplink and cpu ports*/
	drv_fid_set_by_vid(usrvlan, uiFid, stPhyMaskTmp, stPhyMaskUntagTmp);

	return RT_ERR_OK;
}

/*clf rule: only remove user vid for hw l2 mcast entry check*/
int32 drv_clf_translation_vlan_member_remove(uint32 usrvlan, rtk_portmask_t stPhyMask)
{
	uint32 uiFid;
	rtk_portmask_t stPhyMaskTmp;
	rtk_portmask_t stPhyMaskUntagTmp;
	
	if ((0 != usrvlan) && (!VALID_VLAN_ID(usrvlan)))
	{
		return RT_ERR_INPUT;
	}

	(void)drv_valid_fid_get(usrvlan, &uiFid);
	if (uiFid > HAL_VLAN_FID_MAX())
	{
	    return RT_ERR_FAILED;
	}

	RTK_PORTMASK_RESET(stPhyMaskTmp);
    RTK_PORTMASK_RESET(stPhyMaskUntagTmp);

    (void)drv_mc_vlan_mem_get(usrvlan, &stPhyMaskTmp, &stPhyMaskUntagTmp);

	RTK_PORTMASK_REMOVE(stPhyMaskTmp, stPhyMask);
	RTK_PORTMASK_REMOVE(stPhyMaskUntagTmp, stPhyMask);

    if (TRUE == PhyMaskNotNull(stPhyMaskTmp))
    {
		RTK_PORTMASK_PORT_SET(stPhyMaskTmp, HAL_GET_PON_PORT());
    }
	else
	{
		RTK_PORTMASK_PORT_CLEAR(stPhyMaskTmp, HAL_GET_PON_PORT());
	}

	/*port members exclude ctc transparent ports ,uplink and cpu ports*/
    drv_fid_set_by_vid(usrvlan, uiFid, stPhyMaskTmp, stPhyMaskUntagTmp);

	return RT_ERR_OK;
}

int32 drv_mc_vlan_member_add(uint32 ulVlanId, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag, int fidMode)
{
	int32 ret;
    uint32 uiFid;

    rtk_portmask_t stPhyMaskTmp;
	rtk_portmask_t stPhyMaskUntagTmp;
    rtk_fid_t fidTmp = 0;
	rtk_rg_cvlan_info_t cvlan_info;
	
    if ((0 != ulVlanId) && (!VALID_VLAN_ID(ulVlanId)))
    {
        return RT_ERR_INPUT;
    }

    (void)drv_valid_fid_get(ulVlanId, &uiFid);
    if (HAL_VLAN_FID_MAX() < uiFid)
    {
    	SYS_PRINTF("%s[%d]:vlan[%d] Fid error!\n", __func__, __LINE__, ulVlanId);
        return RT_ERR_FAILED;
    }

    RTK_PORTMASK_RESET(stPhyMaskTmp);
    RTK_PORTMASK_RESET(stPhyMaskUntagTmp);

    (void)drv_mc_vlan_mem_get(ulVlanId, &stPhyMaskTmp, &stPhyMaskUntagTmp);
	
    RTK_PORTMASK_OR(stPhyMaskTmp, stPhyMask);
	RTK_PORTMASK_OR(stPhyMaskUntagTmp, stPhyMaskUntag);	
	
	RTK_PORTMASK_OR(stPhyMask, stPhyMaskTmp);
    RTK_PORTMASK_OR(stPhyMaskUntag, stPhyMaskUntagTmp);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT()); /*add pon port to vlan member*/
	//RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
    
	/*fill cvlan info*/
	memset( &cvlan_info, 0,sizeof(rtk_rg_cvlan_info_t) );
	cvlan_info.vlanId = ulVlanId;
	
	/* should check if VLAN ID has existed first */
	//ret = rtk_rg_cvlan_get(&cvlan_info);
	//if(RT_ERR_RG_OK != ret)//this vlan is not exist
	//{//set fidMode
	cvlan_info.isIVL = (fidMode == VLAN_FID_IVL)?1:0;
	//}
	
	/* not exist, so add cvlan entry */
	cvlan_info.memberPortMask.portmask |= stPhyMask.bits[0];
	cvlan_info.untagPortMask.portmask |= stPhyMaskUntag.bits[0];

	ret = rtk_rg_cvlan_add(&cvlan_info);
	if( RT_ERR_RG_OK != ret )
    {
    	SYS_PRINTF("%s[%d]:add cvlan table error!\n",__func__,__LINE__);
    	return RT_ERR_FAILED;
	}

	(void)drv_fid_set_by_vid(ulVlanId, uiFid, stPhyMaskTmp, stPhyMaskUntagTmp);
	
	return RT_ERR_OK;
}

int32 drv_mc_vlan_member_remove(uint32 ulVlanId, rtk_portmask_t stPhyMask)
{
	int32 ret;
	uint32 uiFid;

	rtk_portmask_t stPhyMaskTmp, stPhyMaskRmv;
	rtk_portmask_t stPhyMaskUntagTmp, stPhyMaskUntagRmv;

	rtk_rg_cvlan_info_t cvlan_info;
	
	if ((0 != ulVlanId) && (!VALID_VLAN_ID(ulVlanId)))
	{
		return RT_ERR_INPUT;
	}

	(void)drv_valid_fid_get(ulVlanId, &uiFid);
	if (HAL_VLAN_FID_MAX() < uiFid)
	{
		return RT_ERR_FAILED;
	}
	
	RTK_PORTMASK_RESET(stPhyMaskTmp);
	RTK_PORTMASK_RESET(stPhyMaskUntagTmp);
	RTK_PORTMASK_RESET(stPhyMaskRmv);

	(void)drv_mc_vlan_mem_get(ulVlanId, &stPhyMaskTmp, &stPhyMaskUntagTmp);
	RTK_PORTMASK_REMOVE(stPhyMaskTmp, stPhyMask);
	RTK_PORTMASK_REMOVE(stPhyMaskUntagTmp, stPhyMask);
	
	memcpy(&stPhyMaskRmv, &stPhyMaskTmp, sizeof(rtk_portmask_t));	
	memcpy(&stPhyMaskUntagRmv, &stPhyMaskUntagTmp, sizeof(rtk_portmask_t));
	
	if (TRUE == PhyMaskNotNull(stPhyMaskRmv))
	{
		/* this vlan is used by other port, so add cpu port and pon port to MBR, pon port to untag MBR */
		RTK_PORTMASK_PORT_SET(stPhyMaskTmp, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskTmp, HAL_GET_CPU_PORT());

		RTK_PORTMASK_PORT_SET(stPhyMaskUntagTmp, HAL_GET_PON_PORT());
	}

	memset( &cvlan_info, 0,sizeof(rtk_rg_cvlan_info_t) );
	cvlan_info.vlanId = ulVlanId;
	
	ret = rtk_rg_cvlan_get(&cvlan_info);
	if(RT_ERR_RG_OK == ret)
	{
		ret = rtk_rg_cvlan_del(cvlan_info.vlanId);// delete
		if ( (ret == RT_ERR_RG_OK) && (TRUE == PhyMaskNotNull(stPhyMaskRmv)) )// not empty, so add again
		{
			cvlan_info.memberPortMask.portmask = stPhyMaskTmp.bits[0];
			cvlan_info.untagPortMask.portmask = stPhyMaskUntagTmp.bits[0];
			rtk_rg_cvlan_add(&cvlan_info);
		}
		
		(void)drv_fid_set_by_vid(ulVlanId, uiFid, stPhyMaskRmv, stPhyMaskUntagRmv);
	}
	return RT_ERR_OK;
}

int32 drv_cfg_port_vlan_ruleid_set(uint32 uiLPortId, ctc_vlan_cfg_t *pstVlanCfg)
{
    if ((!IsValidLgcPort(uiLPortId)) ||
        (NULL == pstVlanCfg))
    {
        return RT_ERR_INPUT;
    }

    memcpy(&m_astUniVlanMode[uiLPortId], pstVlanCfg, sizeof(ctc_vlan_cfg_t));

    return RT_ERR_OK;
}

int32 drv_cfg_port_vlan_ruleid_get(uint32 uiLPortId, ctc_vlan_cfg_t *pstVlanCfg)
{
    if ((!IsValidLgcPort(uiLPortId)) ||
        (NULL == pstVlanCfg))
    {
        return RT_ERR_INPUT;
    }

    memcpy(pstVlanCfg, &m_astUniVlanMode[uiLPortId], sizeof(ctc_vlan_cfg_t));

    return RT_ERR_OK;
}

int32 drv_vlan_init(void)
{
	uint32 lport, pport, svlanTpid;

	/* siyuan 2016-08-24: rtk_rg_switch_init changes 1000F value of port ability to enable,
	so not call rtk_rg_switch_init for 9603 */
	if(FALSE == is9603())	
    	    rtk_rg_switch_init();


	/* ENABLE VLAN function */
	if(FALSE == is9607C())
		rtk_rg_vlan_vlanFunctionEnable_set(ENABLED); /* rg set vlan function disable for 9607c */

    /* ENABLE SVLAN function */
    rtk_rg_svlan_svlanFunctionEnable_set(ENABLED);
    
	/*set uplink Port as service port*/
    rtk_rg_svlanServicePort_set(HAL_GET_PON_PORT(), ENABLED);

	/* disable all lan port as service port */
	FOR_EACH_LAN_PORT(lport)
	{
		pport = PortLogic2PhyID(lport);
		if (pport != INVALID_PORT)
			rtk_rg_svlanServicePort_set(pport, DISABLED);
	}
	
	return RT_ERR_OK;
}
#endif

