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

#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <rtk/vlan.h>
#include <rtk/svlan.h>
#include <rtk/qos.h>
#include <rtk/switch.h>
#include <osal/print.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#include <hal/mac/reg.h>

#include <sys_def.h>
#include "drv_convert.h"
#include "drv_vlan.h"
#include "drv_acl.h"
#include <sys_portmask.h>


static ctc_vlan_cfg_t m_astUniVlanMode[MAX_PORT_NUM + 1];

/* use different fidmode based on mcControlType
   MC_CTRL_GDA_MAC : VLAN_FID_SVL
   other types: VLAN_FID_IVL */
int32 drv_vlan_set_fidMode(uint32 ulVlanId, int fidMode)
{
	if (RT_ERR_OK != rtk_vlan_fidMode_set(ulVlanId, fidMode))
    {
        return RT_ERR_FAILED;
    }
	
    return RT_ERR_OK;
}

int32 drv_vlan_entry_create(uint32 ulVlanId, int fidMode)
{
	int32 ret;

    if((0 != ulVlanId) && (!VALID_VLAN_ID(ulVlanId)))
    {
        return RT_ERR_INPUT;
    }

	/*create vlan*/
	ret = rtk_vlan_create(ulVlanId);
	if (RT_ERR_VLAN_EXIST == ret)
    {
        return RT_ERR_VLAN_EXIST;
    }
	else if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	ret = rtk_vlan_fidMode_set(ulVlanId, fidMode);
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	
    return RT_ERR_OK;
}

int32 drv_vlan_entry_delete(uint32 ulVlanId)
{
	int32 ret;

    if(!VALID_VLAN_ID(ulVlanId))
    {
        return RT_ERR_INPUT;
    }

	/*delete vlan*/
	ret = rtk_vlan_destroy(ulVlanId);
	if (RT_ERR_VLAN_ENTRY_NOT_FOUND == ret)
    {
        return RT_ERR_VLAN_ENTRY_NOT_FOUND;
    }
	else if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
	{
		return RT_ERR_FAILED;
	}   
    
    return RT_ERR_OK;
}

int32 drv_vlan_check_member_empty(uint32 ulVlanId, int32 *isNotEmpty)
{
	int32 ret;
    rtk_portmask_t stPhyMaskTmp;
	rtk_portmask_t stPhyMaskUntagTmp;

    if ((0 != ulVlanId) && (!VALID_VLAN_ID(ulVlanId)))
    {
        return RT_ERR_INPUT;
    }

    RTK_PORTMASK_RESET(stPhyMaskTmp);
    RTK_PORTMASK_RESET(stPhyMaskUntagTmp);

	ret = rtk_vlan_port_get(ulVlanId, &stPhyMaskTmp, &stPhyMaskUntagTmp);
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	if(stPhyMaskTmp.bits[0] == 0)
	{
		*isNotEmpty = FALSE;
	}
	else
	{
		*isNotEmpty = TRUE;
	}
	
    return RT_ERR_OK;
}

int32 drv_vlan_member_add(uint32 ulVlanId, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag)
{
	int32 ret;
    rtk_portmask_t stPhyMaskTmp;
	rtk_portmask_t stPhyMaskUntagTmp;

    if ((0 != ulVlanId) && (!VALID_VLAN_ID(ulVlanId)))
    {
        return RT_ERR_INPUT;
    }

    RTK_PORTMASK_RESET(stPhyMaskTmp);
    RTK_PORTMASK_RESET(stPhyMaskUntagTmp);
    
	ret = rtk_vlan_port_get(ulVlanId, &stPhyMaskTmp, &stPhyMaskUntagTmp);
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    RTK_PORTMASK_OR(stPhyMask, stPhyMaskTmp);
    RTK_PORTMASK_OR(stPhyMaskUntag, stPhyMaskUntagTmp);

	ret = rtk_vlan_port_set(ulVlanId, &stPhyMask, &stPhyMaskUntag);
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 drv_vlan_member_remove(uint32 ulVlanId, rtk_portmask_t stPhyMask)
{
	int32 ret;
    rtk_portmask_t stPhyMaskTmp;
	rtk_portmask_t stPhyMaskUntagTmp;

    if ((0 != ulVlanId) && (!VALID_VLAN_ID(ulVlanId)))
    {
        return RT_ERR_INPUT;
    }

    RTK_PORTMASK_RESET(stPhyMaskTmp);
    RTK_PORTMASK_RESET(stPhyMaskUntagTmp);

	ret = rtk_vlan_port_get(ulVlanId, &stPhyMaskTmp, &stPhyMaskUntagTmp);
	/*because svlan and cvlan share same entry in vlan table,
	we assume the specified vlan entry is deleted successfully if return value is failed*/
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_OK;
    }

    RTK_PORTMASK_REMOVE(stPhyMaskTmp, stPhyMask);
    RTK_PORTMASK_REMOVE(stPhyMaskUntagTmp, stPhyMask);

	ret = rtk_vlan_port_set(ulVlanId, &stPhyMaskTmp, &stPhyMaskUntagTmp);
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	
    return RT_ERR_OK;
}

int32 drv_svlan_member_add(uint32 ulSvlanId, uint32 ulSvlanPri, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag, uint32 ulSvlanFid)
{
	int32 ret;
    rtk_portmask_t stPortmask, stUntagPortmask;

    if ((0 != ulSvlanId) && (!VALID_VLAN_ID(ulSvlanId)))
    {
        return RT_ERR_INPUT;
    }

    ret = rtk_svlan_memberPort_get(ulSvlanId, &stPortmask, &stUntagPortmask);

	if(RT_ERR_SVLAN_NOT_EXIST == ret || RT_ERR_SVLAN_ENTRY_NOT_FOUND == ret 
		|| RT_ERR_VLAN_ENTRY_NOT_FOUND == ret)
	{
		/*try create svlan if not exist*/
		ret = rtk_svlan_create(ulSvlanId);
		if(RT_ERR_OK != ret)
    	{
        	return RT_ERR_FAILED;
    	}
		RTK_PORTMASK_RESET(stPortmask);
		RTK_PORTMASK_RESET(stUntagPortmask);
	}
    else if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    stPortmask.bits[0] |= stPhyMask.bits[0];
    stUntagPortmask.bits[0] |= stPhyMaskUntag.bits[0];
    ret = rtk_svlan_memberPort_set(ulSvlanId, &stPortmask, &stUntagPortmask);
    if(RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*below these functions might fail due to chip not support, just ignore it*/
    rtk_svlan_priority_set(ulSvlanId, ulSvlanPri);
    rtk_svlan_fid_set(ulSvlanId, ulSvlanFid);
	drv_fid_set_by_vid(ulSvlanId, ulSvlanFid, stPortmask, stUntagPortmask);
    return RT_ERR_OK;
}

int32 drv_svlan_member_remove(uint32 ulSvlanId, rtk_portmask_t stPhyMask)
{
	int32 ret;    
    rtk_portmask_t stPortmask, stUntagPortmask;
	uint32 uiFid;
	
    if ((0 != ulSvlanId) && (!VALID_VLAN_ID(ulSvlanId)))
    {
        return RT_ERR_INPUT;
    }   

   	ret = rtk_svlan_memberPort_get(ulSvlanId, &stPortmask, &stUntagPortmask);
	/*because svlan and cvlan share same entry in vlan table,
	we assume the specified vlan entry is deleted successfully if return value is faile*/
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_OK;
    }

    stPortmask.bits[0] &= ~(stPhyMask.bits[0]);
	/* siyuan 20170424: for 9607c vlan transparent mode, must also remove untag member port */
	stUntagPortmask.bits[0] &= ~(stPhyMask.bits[0]); 

	ret = rtk_svlan_memberPort_set(ulSvlanId, &stPortmask, &stUntagPortmask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	if((0 == stPortmask.bits[0]) && (0 != ulSvlanId))
	{
		/*delete svlan*/
		ret = rtk_svlan_destroy(ulSvlanId);
		if(RT_ERR_OK != ret)
        {
        	return RT_ERR_FAILED;
        }
	}
	drv_valid_fid_get(ulSvlanId, &uiFid);
	drv_fid_set_by_vid(ulSvlanId, uiFid, stPortmask, stUntagPortmask);
    return RT_ERR_OK;
}

int32 drv_svlan_port_svid_set(uint32 uiLPort, uint32 ulSvlanId)
{
	int32 ret;
    uint32 uiPPort;

    if ((0 != ulSvlanId) && (!VALID_VLAN_ID(ulSvlanId)))
    {
        return RT_ERR_INPUT;
    }

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	
    ret = rtk_svlan_portSvid_set(uiPPort, ulSvlanId);
    if(RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 drv_mc_translation_vlan_member_add(uint32 usrvlan, uint32 mvlan, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag, 
													rtk_portmask_t stPhyMasksvlanUntag, int fidMode)
{
	int32 ret;
	uint32 uiFid;
	ctc_vlan_cfg_t stCtcVlanMode;
	rtk_portmask_t stPhyMaskTmp;
	rtk_portmask_t stPhyMaskUntagTmp;
	rtk_fid_t fidTmp = 0;

	if ((0 != usrvlan) && (!VALID_VLAN_ID(usrvlan)))
	{
		return RT_ERR_INPUT;
	}
	if ((0 != mvlan) && (!VALID_VLAN_ID(mvlan)))
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

	/*create cvlan*/
	ret = drv_vlan_entry_create(usrvlan, fidMode);
	if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
	{
	    return RT_ERR_FAILED;
	}

	fidTmp = uiFid;

	ret = rtk_vlan_port_set(usrvlan, &stPhyMask, &stPhyMaskUntag);
	if (RT_ERR_OK != ret)
	{
	    return RT_ERR_FAILED;
	}
	
	rtk_vlan_fid_set(usrvlan, fidTmp); /* not check return value */

	/*set svlan entry*/
	drv_valid_fid_get(mvlan, &fidTmp);
	ret = drv_svlan_member_add(mvlan, 0, stPhyMask, stPhyMasksvlanUntag, fidTmp);
	if(RT_ERR_OK != ret)
	{
	    return RT_ERR_FAILED;
	}

	/*port members exclude ctc transparent ports ,uplink and cpu ports*/
	(void)drv_fid_set_by_vid(usrvlan, uiFid, stPhyMaskTmp, stPhyMaskUntagTmp);
	return RT_ERR_OK;
}

int32 drv_mc_translation_vlan_member_remove(uint32 usrvlan,uint32 mvlan, rtk_portmask_t stPhyMask)
{
	int32 ret;
    uint32 uiFid;
    ctc_vlan_cfg_t stCtcVlanMode;
	rtk_portmask_t stPhyMaskTmp, stPhyMaskRmv;
	rtk_portmask_t stPhyMaskUntagTmp;
	int32 bFind;

    if ((0 != usrvlan) && (!VALID_VLAN_ID(usrvlan)))
    {
        return RT_ERR_INPUT;
    }
	
	if ((0 != mvlan) && (!VALID_VLAN_ID(mvlan)))
	{
	   return RT_ERR_INPUT;
	}
    (void)drv_valid_fid_get(usrvlan, &uiFid);
    if (HAL_VLAN_FID_MAX() < uiFid)
    {
        return RT_ERR_FAILED;
    }

	RTK_PORTMASK_RESET(stPhyMaskTmp);
    RTK_PORTMASK_RESET(stPhyMaskUntagTmp);
	RTK_PORTMASK_RESET(stPhyMaskRmv);

    (void)drv_mc_vlan_mem_get(usrvlan, &stPhyMaskTmp, &stPhyMaskUntagTmp);

	RTK_PORTMASK_REMOVE(stPhyMaskTmp, stPhyMask);
	RTK_PORTMASK_REMOVE(stPhyMaskUntagTmp, stPhyMask);
	memcpy(&stPhyMaskRmv, &stPhyMask, sizeof(rtk_portmask_t));

    if (TRUE == PhyMaskNotNull(stPhyMaskTmp))
    {
		RTK_PORTMASK_PORT_SET(stPhyMaskTmp, HAL_GET_PON_PORT());
    }
	else
	{
		RTK_PORTMASK_PORT_CLEAR(stPhyMaskTmp, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskRmv, HAL_GET_PON_PORT());
	}

	/*set cvlan entry*/
	ret = drv_vlan_member_remove(usrvlan, stPhyMaskRmv);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	ret = drv_vlan_check_member_empty(usrvlan, &bFind);
	if ((RT_ERR_OK == ret) && (FALSE == bFind))
	{
		drv_flush_mc_vlan_mac_by_vid(usrvlan);
		ret = drv_vlan_entry_delete(usrvlan);
		if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
		{
			return RT_ERR_FAILED;
		}
	}

    /*set svlan entry*/
    ret = drv_svlan_member_remove(mvlan, stPhyMaskRmv);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*port members exclude ctc transparent ports ,uplink and cpu ports*/
    (void)drv_fid_set_by_vid(usrvlan, uiFid, stPhyMaskTmp, stPhyMaskUntagTmp);
    
    return RT_ERR_OK;
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
    ctc_vlan_cfg_t stCtcVlanMode;
    rtk_portmask_t stPhyMaskTmp;
	rtk_portmask_t stPhyMaskUntagTmp;
    rtk_fid_t fidTmp = 0;

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

    (void)drv_mc_vlan_mem_get(ulVlanId, &stPhyMaskTmp, &stPhyMaskUntagTmp);
	
    RTK_PORTMASK_OR(stPhyMaskTmp, stPhyMask);
	RTK_PORTMASK_OR(stPhyMaskUntagTmp, stPhyMaskUntag);

    RTK_PORTMASK_OR(stPhyMask, stPhyMaskTmp);
    RTK_PORTMASK_OR(stPhyMaskUntag, stPhyMaskUntagTmp);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT()); /*add pon port to vlan member*/
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
    RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
	
	/*create cvlan*/
	ret = drv_vlan_entry_create(ulVlanId, fidMode);
	if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
    {
        return RT_ERR_FAILED;
    }
	
    fidTmp = uiFid;

	ret = rtk_vlan_port_set(ulVlanId, &stPhyMask, &stPhyMaskUntag);
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	rtk_vlan_fid_set(ulVlanId, fidTmp); /* not check return value */

    /*set svlan entry*/
	ret = drv_svlan_member_add(ulVlanId, 0, stPhyMask, stPhyMaskUntag, uiFid);
    if(RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*port members exclude ctc transparent ports ,uplink and cpu ports*/
    (void)drv_fid_set_by_vid(ulVlanId, uiFid, stPhyMaskTmp, stPhyMaskUntagTmp);
	
    return RT_ERR_OK;
}

int32 drv_mc_vlan_member_remove(uint32 ulVlanId, rtk_portmask_t stPhyMask)
{
	int32 ret;
	uint32 uiFid;
	ctc_vlan_cfg_t stCtcVlanMode;
	rtk_portmask_t stPhyMaskTmp, stPhyMaskRmv;
	rtk_portmask_t stPhyMaskUntagTmp;
	rtk_svlan_memberCfg_t stSvlan_cfg;
	int32 bFind;

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
	memcpy(&stPhyMaskRmv, &stPhyMask, sizeof(rtk_portmask_t));

	if (TRUE == PhyMaskNotNull(stPhyMaskTmp))
	{
		/* this vlan is used by other port */
		RTK_PORTMASK_PORT_SET(stPhyMaskTmp, HAL_GET_PON_PORT());
	}
	else
	{
		/* this vlan is useless */
		RTK_PORTMASK_PORT_CLEAR(stPhyMaskTmp, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskRmv, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskRmv, HAL_GET_CPU_PORT());
	}

	/*set svlan entry */
    ret = drv_svlan_member_remove(ulVlanId, stPhyMaskRmv);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/*set cvlan entry*/
	ret = drv_vlan_member_remove(ulVlanId, stPhyMaskRmv);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	ret = drv_vlan_check_member_empty(ulVlanId, &bFind);
	if ((RT_ERR_OK == ret) && (FALSE == bFind))
	{
		drv_flush_mc_vlan_mac_by_vid(ulVlanId);
		ret = drv_vlan_entry_delete(ulVlanId);
		if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
		{
			return RT_ERR_FAILED;
		}
	}

	/*port members exclude ctc transparent ports ,uplink and cpu ports*/
	(void)drv_fid_set_by_vid(ulVlanId, uiFid, stPhyMaskTmp, stPhyMaskUntagTmp);

	return RT_ERR_OK;
}

int32 drv_vlan_port_pvid_set(uint32 ulLgcPortNumber, uint32 ulPvid, uint32 priority)
{
	int32 ret;
	uint32 uiPPort;

    if (7 < priority)
    {
        return RT_ERR_INPUT;
    }
	
    if ((0 != ulPvid) && (!VALID_VLAN_ID(ulPvid)))
    {
        return RT_ERR_INPUT;
    }
    if(!IsValidLgcPort(ulLgcPortNumber))
    {
        return RT_ERR_INPUT;
    }

	uiPPort = PortLogic2PhyID(ulLgcPortNumber);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;

    ret = rtk_vlan_portPvid_set(uiPPort, ulPvid);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	ret = rtk_qos_portPri_set(uiPPort, priority);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
    
    return RT_ERR_OK;
}

int32 drv_vlan_port_ingressfilter_set(uint32 uiLPort, int32 bEnable)
{
    uint32 uiPPort;
    rtk_enable_t tdEnable;
    
    if((!IsValidLgcPort(uiLPort)) ||
        ((TRUE != bEnable) && (FALSE != bEnable)))
    {
        return RT_ERR_INPUT;
    }

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;

    if (TRUE == bEnable)
    {
        tdEnable = ENABLED;
    }
    else
    {
        tdEnable = DISABLED;
    }
    
    if (RT_ERR_OK != rtk_vlan_portIgrFilterEnable_set(uiPPort, tdEnable))
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 drv_vlan_egress_keeptag_ingress_enable_set(uint32 uiEgLPort, uint32 uiIngLPort)
{
	int32 ret;
	uint32 uiEgPPort;
	uint32 uiIngPPort;
	rtk_portmask_t stPPortMask; 
	rtk_vlan_tagKeepType_t pType = TAG_KEEP_TYPE_CONTENT;
	
	if ((!IsValidLgcPort(uiEgLPort)) || (!IsValidLgcPort(uiIngLPort)))
	{
		return RT_ERR_INPUT;
	}
	
	uiEgPPort = PortLogic2PhyID(uiEgLPort);
	if (INVALID_PORT == uiEgPPort)
		return RT_ERR_FAILED;
	uiIngPPort = PortLogic2PhyID(uiIngLPort);
	if (INVALID_PORT == uiIngPPort)
		return RT_ERR_FAILED;
	rtk_switch_portMask_Clear(&stPPortMask);

	ret = rtk_vlan_portEgrTagKeepType_get(uiEgPPort, &stPPortMask, &pType);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	stPPortMask.bits[0] |= 1U << uiIngPPort;
	
	ret = rtk_vlan_portEgrTagKeepType_set(uiEgPPort, &stPPortMask, TAG_KEEP_TYPE_CONTENT);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}

int32 drv_vlan_egress_keeptag_ingress_disable_set(uint32 uiEgLPort, uint32 uiIngLPort)
{
	int32 ret;
    uint32 uiEgPPort;
    uint32 uiIngPPort;
    rtk_portmask_t stPPortMask;
	rtk_vlan_tagKeepType_t pType = TAG_KEEP_TYPE_CONTENT;
    
    if ((!IsValidLgcPort(uiEgLPort)) || (!IsValidLgcPort(uiIngLPort)))
    {
        return RT_ERR_INPUT;
    }
    
	uiEgPPort = PortLogic2PhyID(uiEgLPort);
	if (INVALID_PORT == uiEgPPort)
		return RT_ERR_FAILED;
	uiIngPPort = PortLogic2PhyID(uiIngLPort);
	if (INVALID_PORT == uiIngPPort)
		return RT_ERR_FAILED;

	rtk_switch_portMask_Clear(&stPPortMask);

	ret = rtk_vlan_portEgrTagKeepType_get(uiEgPPort, &stPPortMask, &pType);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	stPPortMask.bits[0] &= ~(1U << uiIngPPort);
	
	ret = rtk_vlan_portEgrTagKeepType_set(uiEgPPort, &stPPortMask, TAG_KEEP_TYPE_CONTENT);
	if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 drv_port_c2s_entry_add(uint32 uiLPort, uint32 ulCvid, uint32 ulSvid)
{
	int32 ret;
    uint32 uiPPort;

    if (((0 != ulSvid) && (!VALID_VLAN_ID(ulSvid))) ||
        (!VALID_VLAN_ID(ulCvid)) ||
        (!IsValidLgcPort(uiLPort)))
    {
        return RT_ERR_INPUT;
    }

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
    ret = rtk_svlan_c2s_add(ulCvid, uiPPort, ulSvid);
    if(RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    } 

    return RT_ERR_OK;
}

int32 drv_port_c2s_entry_delete(uint32 uiLPort, uint32 ulCvid, uint32 ulSvid)
{
	int32 ret;
    uint32 uiPPort;

    if ((!VALID_VLAN_ID(ulCvid)) || (!IsValidLgcPort(uiLPort)))
    {
        return RT_ERR_INPUT;
    }

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
    ret = rtk_svlan_c2s_del(ulCvid, uiPPort, ulSvid);
    if(RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    } 

    return RT_ERR_OK;
}

int32 drv_port_sp2c_entry_add(uint32 uiLPort, uint32 ulSvid, uint32 ulCvid)
{
	int32 ret;
    uint32 uiPPort;

    if (((0 != ulSvid) && (!VALID_VLAN_ID(ulSvid))) ||
        (!VALID_VLAN_ID(ulCvid)) ||
        (!IsValidLgcPort(uiLPort)))
    {
        return RT_ERR_INPUT;
    }

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
    ret = rtk_svlan_sp2c_add(ulSvid, uiPPort, ulCvid);
    if(RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    } 

    return RT_ERR_OK;
}

int32 drv_port_sp2c_entry_delete(uint32 uiLPort, uint32 ulSvid)
{
	int32 ret;
    uint32 uiPPort;

    if ((0 != ulSvid) && (!VALID_VLAN_ID(ulSvid)))
    {
        return RT_ERR_INPUT;
    }

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
    ret = rtk_svlan_sp2c_del(ulSvid, uiPPort);
    if(RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
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
	rtk_enable_t enable;
	uint32 lport, pport;

	/* no need to call rtk_switch_init which will cause FE port link up slowly */
    //rtk_switch_init();
                        
    rtk_svlan_init();
	
    rtk_vlan_init();
    
    /*Change TPID from 0x88a8 to 0x8100*/
    rtk_svlan_tpidEntry_set(0, 0x8100);

    /*set uplink Port as service port*/
    rtk_svlan_servicePort_set(HAL_GET_PON_PORT(), ENABLED);

	/* disable all lan port as service port */
	FOR_EACH_LAN_PORT(lport)
	{
		pport = PortLogic2PhyID(lport);
		if (pport != INVALID_PORT) {
			rtk_svlan_servicePort_set(pport, DISABLED);
		}
	}
	/* disable cpu port svlan */
	rtk_svlan_servicePort_set(HAL_GET_CPU_PORT(), DISABLED);
	
	/*set VID 0 and VID 4095 action*/
	rtk_vlan_reservedVidAction_set(RESVID_ACTION_TAG, RESVID_ACTION_TAG);
	
    /*create cvlan CVID 0*/
	rtk_vlan_create(0);
            
	/*create svlan SVID 0*/
	rtk_svlan_create(0);

    /*Unmatch packet assign to SVID 0*/
    rtk_svlan_unmatchAction_set(UNMATCH_ASSIGN, 0);
		
	if((TRUE == is9607C()) || (TRUE == is9602C()))
	{
		/*Untag packet assign to PVID: hardware not support assign svlan action */
	    rtk_svlan_untagAction_set(SVLAN_ACTION_PSVID, 0);
	}
	else
	{
		/*Untag packet assign to SVID 0*/
	    rtk_svlan_untagAction_set(UNTAG_ASSIGN, 0);
	}

	return RT_ERR_OK;
}

