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
#include <unistd.h>

#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <osal/print.h>
#include <hal/common/halctrl.h>

#include "drv_convert.h"
#include "drv_mac.h"
#include "drv_vlan.h"
#include "ctc_mc.h"

/* RomeDriver related struct define */
#include "rtk_rg_struct.h"
#include "rtk_rg_define.h"
#include "epon_oam_igmp_db.h"

static vlan_fid_t *m_astVlanFid;

static uint32 ulMacMacstInited = 0;
static uint32 ulIpMacstInited = 0;

static ip_multicast_db_t ipMcastEntryDB[MAX_MULTICAST_ENTRY];
static mac_mcast_db_t macMcastEntryDB[MAX_MULTICAST_ENTRY];

uint32 rg_portmask_get_by_rtk_portmask(rtk_portmask_t rtkPortmask)
{
	rtk_rg_portmask_t rgPortmask;
	rgPortmask.portmask = rtkPortmask.bits[0];
	return rgPortmask.portmask;
}

uint32 rtk_portmask_get_by_rg_portmask(rtk_rg_portmask_t rgPortmask)
{
	rtk_portmask_t rtkPortmask;
	rtkPortmask.bits[0] = rgPortmask.portmask;
	return rtkPortmask.bits[0];
}

static uint32 drv_mac_compare(mac_address_t address_1, rtk_mac_t address_2)
{
    uint32 i;

    for(i = 0; i< MAC_ADDR_LEN; i++)
    {
        if(address_1[i] != address_2.octet[i])
        {
            break;
        }
    }

    if(MAC_ADDR_LEN == i)
    {
        return 0;
    }

    return 1;
}

static int32 drv_mac_mcast_add(rtk_rg_l2MulticastFlow_t *pl2McFlow, int flow_index)
{
	memcpy(&macMcastEntryDB[flow_index].l2McFlow, pl2McFlow, sizeof(rtk_rg_l2MulticastFlow_t));
	macMcastEntryDB[flow_index].valid = 1;

	return RT_ERR_OK;;
}

static int drv_mac_mcast_delete(int flow_index)
{	
	memset(&macMcastEntryDB[flow_index], 0, sizeof(mac_mcast_db_t));
    return RT_ERR_OK;
}

static int32 drv_ip_mcast_index_get(rtk_rg_multicastFlow_t *p_mcFlow)
{
	int i;
	for (i=0; i<MAX_MULTICAST_ENTRY; i++)
	{
		if( 0 == memcmp(p_mcFlow, &ipMcastEntryDB[i].ipMcastEntry, sizeof(rtk_rg_multicastFlow_t)) )
			return i;	
	}
	return RT_ERR_FAILED;
}

static int32 drv_ip_mcast_add(rtk_rg_multicastFlow_t *p_mcFlow, int flow_index)
{
 	memcpy(&ipMcastEntryDB[flow_index].ipMcastEntry, p_mcFlow, sizeof(rtk_rg_multicastFlow_t));
	ipMcastEntryDB[flow_index].valid = 1;
	return RT_ERR_OK;
}

static int drv_ip_mcast_delete(int flow_index)
{	
	memset(&ipMcastEntryDB[flow_index],0x00, sizeof(ip_multicast_db_t));
}

static int32 drv_mac_mcast_get_by_vid(uint16 tdVid, mac_mcast_t *pl2McFlow)
{
    uint32 i;

    if (NULL == pl2McFlow)
    {
        return RT_ERR_FAILED;
    }
    
    for(i = 0; i < MAX_MULTICAST_ENTRY; i++)
    {
        if(tdVid == macMcastEntryDB[i].l2McFlow.vlanID)
        {
            memcpy(pl2McFlow, &macMcastEntryDB[i], sizeof(rtk_rg_l2MulticastFlow_t));
            return RT_ERR_OK;         
        }
    }

    return RT_ERR_FAILED;
}

static int32 drv_fid_get_by_vid(uint32 uiVid, uint32 *puiFid)
{
	uint32 i;

	if (!VALID_VLAN_ID(uiVid) || (NULL == puiFid)) 
	{
		return RT_ERR_INPUT;
	}

	*puiFid = FID_INVALID_ID;

	for (i = 0; i < FID_MAX_VLAN_NUM; i++)
	{
		if (TRUE == m_astVlanFid[i].bValid)
		{
			if (uiVid == m_astVlanFid[i].uiVid)
			{
				*puiFid = m_astVlanFid[i].uiFid;
				break;
			}
		}
	}

	return RT_ERR_OK;
}


int drv_mac_init(void)
{
    uint32 numFid = FID_MAX_VLAN_NUM;

    m_astVlanFid = (vlan_fid_t *)malloc(sizeof(vlan_fid_t) * numFid);
    if(NULL == m_astVlanFid)
        return RT_ERR_NULL_POINTER;

    memset(m_astVlanFid, 0, sizeof(vlan_fid_t) * numFid);

	memset(ipMcastEntryDB, 0x00, sizeof(ip_multicast_db_t) * MAX_MULTICAST_ENTRY);
	memset(macMcastEntryDB, 0x00, sizeof(mac_mcast_db_t) * MAX_MULTICAST_ENTRY);
	return RT_ERR_OK;
}

void drv_flush_mc_vlan_mac_by_vid(int32 vid)
{
	
}

int32 drv_fid_set_by_vid(uint32 uiVid, uint32 uiFid, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag)
{
	uint32 i;
	uint32 uiIndex = FID_INVALID_ID;

	if (!VALID_VLAN_ID(uiVid) || (HAL_VLAN_FID_MAX() < uiFid)) 
	{
		return RT_ERR_INPUT;
	}

	for (i = 0; i < FID_MAX_VLAN_NUM; i++)
	{
		if (FALSE == m_astVlanFid[i].bValid)
		{
			if (FID_INVALID_ID == uiIndex)
			{
				uiIndex = i;
			}
		}
		else
		{
			if (uiVid == m_astVlanFid[i].uiVid)
			{
				uiIndex = i;
				break;
			}
		}
	}

	if (FID_INVALID_ID != uiIndex)
	{
		if (TRUE == PhyMaskNotNull(stPhyMask))
		{
			m_astVlanFid[uiIndex].bValid = TRUE;
			m_astVlanFid[uiIndex].uiVid  = uiVid;
			m_astVlanFid[uiIndex].uiFid  = uiFid;
			memcpy(&(m_astVlanFid[uiIndex].stPhyMask), &stPhyMask, sizeof(rtk_portmask_t));
			memcpy(&(m_astVlanFid[uiIndex].stPhyMaskUntag), &stPhyMaskUntag, sizeof(rtk_portmask_t));
		}
		else
		{
			memset(&m_astVlanFid[uiIndex], 0, sizeof(m_astVlanFid[uiIndex]));
		}
	}

	if (FID_INVALID_ID == uiIndex)
	{
		return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}

int32 drv_mc_vlan_mem_get(uint32 uiVid, rtk_portmask_t *pstPhyMask, rtk_portmask_t *pstPhyMaskUntag)
{
	uint32 i;
	uint32 uiIndex = FID_INVALID_ID;

	if (!VALID_VLAN_ID(uiVid) || (NULL == pstPhyMask) || (NULL == pstPhyMaskUntag)) 
	{
		return RT_ERR_INPUT;
	}

	memset(pstPhyMask, 0, sizeof(rtk_portmask_t));
	memset(pstPhyMaskUntag, 0, sizeof(rtk_portmask_t));

	for (i = 0; i < FID_MAX_VLAN_NUM; i++)
	{
		if (TRUE == m_astVlanFid[i].bValid)
		{
			if (uiVid == m_astVlanFid[i].uiVid)
			{
				uiIndex = i;
				break;
			}
		}
	}

	if (FID_INVALID_ID != uiIndex)
	{
		memcpy(pstPhyMask, &(m_astVlanFid[uiIndex].stPhyMask), sizeof(rtk_portmask_t));
		memcpy(pstPhyMaskUntag, &(m_astVlanFid[uiIndex].stPhyMaskUntag), sizeof(rtk_portmask_t));
	}

	return RT_ERR_OK;
}

int32 drv_valid_fid_get(uint32 uiVid, uint32 *puiFid)
{
	uint32 i;
	uint32 uiFidIdx;

	if (!VALID_VLAN_ID(uiVid) || (NULL == puiFid))  
	{
		return RT_ERR_INPUT;
	}

	if(TRUE == is9601B() || TRUE == is9607C() || TRUE == is9602C())
	{
		*puiFid = 0;
		return RT_ERR_OK;
	}
	
	drv_fid_get_by_vid(uiVid, puiFid);
	if (*puiFid <= HAL_VLAN_FID_MAX())
	{
		return RT_ERR_OK;
	}

	/* find out an unused fid for the new vlan */
	for (uiFidIdx = 0; uiFidIdx <= HAL_VLAN_FID_MAX(); uiFidIdx++)
	{
		for (i = 0; i < FID_MAX_VLAN_NUM; i++)
		{
			if (TRUE == m_astVlanFid[i].bValid)
			{
				if (uiFidIdx == m_astVlanFid[i].uiFid)
				{
					break;
				}
			}
		}

		if (i >= FID_MAX_VLAN_NUM)
		{
			*puiFid = uiFidIdx;
			break;
		}
	}

	/* if there is no unused fid, just set fid to 0 */
	if(*puiFid > HAL_VLAN_FID_MAX())
		*puiFid = 0;
	
	return RT_ERR_OK;
}

int32 drv_mac_mcast_get_by_mac_and_vid(mac_address_t tdMac, uint16 tdVid, rtk_rg_l2MulticastFlow_t *pl2McFlow)
{
    uint32 i;

    if ((NULL == tdMac) || (NULL == pl2McFlow))
    {
        return RT_ERR_FAILED;
    }
    
    for(i = 0; i < MAX_MULTICAST_ENTRY; i++)
    {  
        if( (macMcastEntryDB[i].valid) && (!drv_mac_compare(tdMac, macMcastEntryDB[i].l2McFlow.mac) ) && (tdVid == macMcastEntryDB[i].l2McFlow.vlanID) )
        {
            memcpy(pl2McFlow, &macMcastEntryDB[i].l2McFlow, sizeof(rtk_rg_l2MulticastFlow_t));
            return i;
        }            
    }
    return RT_ERR_FAILED;
}

/* Martin ZHU note: apollo rg support DMAC+VID and DMAC+FID */
int32 drv_mac_mcast_set_by_mac_and_fid(mac_mcast_t stMacMcast)
{
	rtk_rg_l2MulticastFlow_t l2McFlow, l2McFlowTmp;
	int flow_idx;
	int32 bSamePort;
    uint32 uiFid;
    int32 ret;
    rtk_portmask_t phy_mask;
	rtk_portmask_t phy_mask_tmp;
	
	drv_valid_fid_get(stMacMcast.tdVid, &uiFid);
    if (HAL_VLAN_FID_MAX() < uiFid)
    {
        return RT_ERR_FAILED;
    }

	if(!ulMacMacstInited)
    {        
        memset(macMcastEntryDB, 0, sizeof(macMcastEntryDB));
        ulMacMacstInited = 1;        
    }
		
	memcpy(&phy_mask, &(stMacMcast.port_mask), sizeof(rtk_portmask_t));

	/*Check same multicast mac entry.*/
	if(stMacMcast.fidMode == VLAN_FID_IVL)
    	flow_idx = drv_mac_mcast_get_by_mac_and_vid(stMacMcast.mac_addr, stMacMcast.tdVid, &l2McFlowTmp);
    else
		flow_idx = drv_mac_mcast_get_by_mac_and_vid(stMacMcast.mac_addr, 0, &l2McFlowTmp);
	
    if (RT_ERR_FAILED != flow_idx)
    {//delete first
    	phy_mask_tmp.bits[0] = rtk_portmask_get_by_rg_portmask(l2McFlowTmp.port_mask); 
		if(!RTK_PORTMASK_COMPARE(stMacMcast.port_mask, phy_mask_tmp))
		{
			bSamePort = TRUE;
		}
		else
		{
			bSamePort = FALSE;
		}
        /*Try add the same one.*/
        if (TRUE == bSamePort)
        {
            return RT_ERR_OK;
        }

		/*Add new ports to existed port mask.*/
		RTK_PORTMASK_OR(phy_mask, phy_mask_tmp);
	}
	
	memset(&l2McFlow, 0x00, sizeof(rtk_rg_l2MulticastFlow_t));
	memcpy(l2McFlow.mac.octet, stMacMcast.mac_addr, sizeof(mac_address_t));

	if(stMacMcast.fidMode == VLAN_FID_IVL)
	{
		l2McFlow.isIVL = 1;;
		l2McFlow.vlanID = stMacMcast.tdVid;
	}
	l2McFlow.port_mask.portmask = rg_portmask_get_by_rtk_portmask(phy_mask);
	
	ret = rtk_rg_l2MultiCastFlow_add(&l2McFlow, &flow_idx);
    if (RT_ERR_RG_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	/* add to macMcastEntryDB */
	drv_mac_mcast_add(&l2McFlow, flow_idx);
    
    return RT_ERR_OK;
}

int32 drv_mc_mac_delete_by_mac_and_fid(mac_mcast_t stMacMcast)
{
	rtk_rg_l2MulticastFlow_t l2McFlow, l2McFlowTmp;
	int flow_idx;
	int32 bSamePort;
	uint32 uiFid;
    int32 ret;
    int32 rtkApiRet;
    rtk_portmask_t phy_mask;
	rtk_portmask_t phy_mask_tmp;
	
    if(!VALID_VLAN_ID(stMacMcast.tdVid))
    {
        return RT_ERR_INPUT;
    }

    if(!IS_MULTICAST(stMacMcast.mac_addr))
    {
        /*unicast*/
        return RT_ERR_INPUT;
    }
  
    if(!ulMacMacstInited)
    {        
        memset(macMcastEntryDB, 0, sizeof(macMcastEntryDB));
        ulMacMacstInited = 1;        
    }
	
   	if(stMacMcast.fidMode == VLAN_FID_IVL)
    	flow_idx = drv_mac_mcast_get_by_mac_and_vid(stMacMcast.mac_addr, stMacMcast.tdVid, &l2McFlowTmp);
    else
		flow_idx = drv_mac_mcast_get_by_mac_and_vid(stMacMcast.mac_addr, 0, &l2McFlowTmp);

    /*Find the entry.*/
    if (RT_ERR_FAILED != flow_idx)
    {
    	phy_mask_tmp.bits[0] = rtk_portmask_get_by_rg_portmask(l2McFlowTmp.port_mask);
		if(!RTK_PORTMASK_COMPARE(stMacMcast.port_mask, phy_mask_tmp))
		{
			bSamePort = TRUE;
		}
		else
		{
			bSamePort = FALSE;
		}

        drv_fid_get_by_vid(stMacMcast.tdVid, &uiFid);
        if (HAL_VLAN_FID_MAX() < uiFid)
        {
            return RT_ERR_FAILED;
        }
		
        /*Delete it*/
		rtkApiRet = rtk_rg_multicastFlow_del(flow_idx);
        if (RT_ERR_RG_OK != rtkApiRet)
        {
           return RT_ERR_FAILED;
        }

		/* delete from macMcastEntryDB first */
		drv_mac_mcast_delete(flow_idx);
		
        if (FALSE == bSamePort)
        {
			memcpy(&phy_mask, &(stMacMcast.port_mask), sizeof(rtk_portmask_t));
			/*Delete ports from existed port mask.*/
			RTK_PORTMASK_REMOVE(phy_mask_tmp, phy_mask);

			memset(&l2McFlow, 0x00, sizeof(rtk_rg_l2MulticastFlow_t ));
			memcpy(l2McFlow.mac.octet, stMacMcast.mac_addr, sizeof(mac_address_t));
			if(stMacMcast.fidMode == VLAN_FID_IVL)
			{
				l2McFlow.isIVL = 1;;
				l2McFlow.vlanID = stMacMcast.tdVid;
			}
			l2McFlow.port_mask.portmask = rg_portmask_get_by_rtk_portmask(phy_mask_tmp);

			rtkApiRet = rtk_rg_l2MultiCastFlow_add(&l2McFlow, &flow_idx);
            if (RT_ERR_RG_OK != rtkApiRet)
            {
                return RT_ERR_FAILED;
            }

			/* add to macMcastEntryDB again */
			drv_mac_mcast_add(&l2McFlow, flow_idx);
        }
    }
   
    return RT_ERR_OK;
}

int32 drv_ipMcastMode_set(uint32 mode)
{
	int32 ret = RT_ERR_RG_FAILED;
	int i;
	rtk_rg_initParams_t init_param;
		
	if (LOOKUP_ON_MAC_AND_VID_FID == mode)
	{
		/* delete all IP MC entry */
		for( i = 0; i<MAX_MULTICAST_ENTRY; i++)
		{
			if(ipMcastEntryDB[i].valid){
				if(RT_ERR_RG_OK == rtk_rg_multicastFlow_del(i))
				{/* clear ipMcastEntryDB[i] */
					drv_ip_mcast_delete(i);
				}
			}
		}
		/* echo 1 > /proc/rg/ivlMulticastSupport */
		system("/bin/echo 1 > /proc/rg/ivlMulticastSupport");
	}
	else if (LOOKUP_ON_DIP_AND_VID == mode)
	{// set LUT_IPMC_HASH = 1, LUT_IPMC_LOOKUP_OP=1 and tell rg
		//delete all IP MC entry
		for( i = 0; i<MAX_MULTICAST_ENTRY; i++)
		{
			if(macMcastEntryDB[i].valid){
				if( RT_ERR_RG_OK==rtk_rg_multicastFlow_del(i) )
				{			
					/* clear ipMcastEntryDB[i] */
					drv_mac_mcast_delete(i);
				}
			}
		}
		
		/* echo 0 > /proc/rg/ivlMulticastSupport */
		system("/bin/echo 0 > /proc/rg/ivlMulticastSupport");
	}
	
	if (RT_ERR_RG_OK == ret){	
		SYS_PRINTF("%s ret %d, set mode %d\n", __func__, ret, mode);
		return RT_ERR_OK;
	}	

	return RT_ERR_FAILED;	
}


int32 drv_ipMcastEntry_get(ip_mcast_t *pStIpMcast)
{
	int32 flow_idx = -1, i;
	
	for (i=0; i<MAX_MULTICAST_ENTRY; i++)
	{
		if((pStIpMcast->isIPv6 == 0) && (pStIpMcast->dip == ipMcastEntryDB[i].ipMcastEntry.multicast_ipv4_addr)
			&& (pStIpMcast->vid == ipMcastEntryDB[i].ipMcastEntry.vlanID) )
		{
			flow_idx = i;
			break;
		}
		else if((pStIpMcast->isIPv6 == 1) && (pStIpMcast->vid == ipMcastEntryDB[i].ipMcastEntry.vlanID) 
				&& (pStIpMcast->dipv6[0] == ipMcastEntryDB[i].ipMcastEntry.multicast_ipv6_addr[0])
				&& (pStIpMcast->dipv6[1] == ipMcastEntryDB[i].ipMcastEntry.multicast_ipv6_addr[1])
				&& (pStIpMcast->dipv6[2] == ipMcastEntryDB[i].ipMcastEntry.multicast_ipv6_addr[2])
				&& (pStIpMcast->dipv6[3] == ipMcastEntryDB[i].ipMcastEntry.multicast_ipv6_addr[3]))
		{
			flow_idx = i;
			break;
		}
	}	

	if ( -1 == flow_idx ){
		SYS_PRINTF("drv_ipMcastEntry_get failed: no this ip MC entry\n");
		return RT_ERR_FAILED;
	}
	
	pStIpMcast->port_mask.bits[0] = rtk_portmask_get_by_rg_portmask(ipMcastEntryDB[flow_idx].ipMcastEntry.port_mask);
	return flow_idx;
}

/* For this case, rg will change GIP+VID as GIP only */
int32 drv_ipMcastEntry_add(ip_mcast_t stIpMcast)
{
    	
	rtk_rg_multicastFlow_t mcFlow;
	int flow_idx;
	rtk_l2_ipmcMode_t l2mode;
	
	if(RT_ERR_OK != rtk_rg_l2_ipmcMode_get(&l2mode))
	{
		SYS_PRINTF("rtk_l2_ipmcMode_get failed\n");
	}

	if(l2mode == LOOKUP_ON_MAC_AND_VID_FID)
	{
		SYS_PRINTF("set ip mcast mode not mac_fid, not work\n");
	}

	if(!ulIpMacstInited)
    {        
        memset(ipMcastEntryDB, 0, sizeof(ipMcastEntryDB));
        ulIpMacstInited = 1;        
    }

    memset(&mcFlow, 0x00, sizeof(rtk_rg_multicastFlow_t));
    mcFlow.multicast_ipv4_addr = (ipaddr_t)(stIpMcast.dip);

	/* because ASIC use GIP only replace GIP+VID */
	mcFlow.isIVL = 0;
	mcFlow.isIPv6 = 0;

	/* because ASIC use GIP only replace GIP+VID */
	if (mcFlow.isIVL == 0)
	{
		stIpMcast.vid = 0;
	}
	mcFlow.vlanID = stIpMcast.vid;
	
	mcFlow.port_mask.portmask = rg_portmask_get_by_rtk_portmask(stIpMcast.port_mask);
	
	/* add old portmask */
	flow_idx = drv_ipMcastEntry_get(&stIpMcast);
	mcFlow.port_mask.portmask |= rg_portmask_get_by_rtk_portmask(stIpMcast.port_mask);

	if(RT_ERR_RG_OK != rtk_rg_multicastFlow_add(&mcFlow, &flow_idx))
	{
		SYS_PRINTF("rtk_rg_multicastFlow_add failed\n");
		return RT_ERR_FAILED;
	}

	/* keep in multicast lut DB for delete */
	drv_ip_mcast_add(&mcFlow, flow_idx);
	return RT_ERR_OK;
}

/* For this case, rg will change GIP+VID as GIP only */
int32 drv_ipMcastEntry_del(ip_mcast_t stIpMcast)
{
	rtk_rg_multicastFlow_t mcFlow;
	rtk_portmask_t tmpPortmask;
	int flow_idx = -1, i, ret;
	
	if(!ulIpMacstInited)
    {        
        memset(ipMcastEntryDB, 0, sizeof(ipMcastEntryDB));
        ulIpMacstInited = 1;        
    }
	
	/* because ASIC use GIP only replace GIP+VID */
	stIpMcast.vid = 0;

	memcpy(&tmpPortmask, &(stIpMcast.port_mask), sizeof(rtk_portmask_t));
	flow_idx = drv_ipMcastEntry_get(&stIpMcast);
	if ( RT_ERR_FAILED == flow_idx ){
		SYS_PRINTF("%s[%d]: no this ip MC entry\n",__func__,__LINE__);
	}

	//delete this entry
	if(RT_ERR_RG_OK != rtk_rg_multicastFlow_del(flow_idx))
	{				
		SYS_PRINTF("%s[%d]:	%s failed\n",__func__,__LINE__,__func__);
		return RT_ERR_FAILED;
	}

	/* clear ipMcastEntryDB[flow_idx] */
	drv_ip_mcast_delete(flow_idx);
	
	RTK_PORTMASK_REMOVE(stIpMcast.port_mask, tmpPortmask);
	ret = PhyMaskNotNull(stIpMcast.port_mask);
	if (ret)
	{//renew port mask

		memset(&mcFlow, 0x00, sizeof(rtk_rg_multicastFlow_t));
   		mcFlow.multicast_ipv4_addr = (ipaddr_t)(stIpMcast.dip);
	
		mcFlow.isIVL = 0;
		mcFlow.isIPv6 = 0;
	
		/* because ASIC use GIP only replace GIP+VID */
		if (mcFlow.isIVL == 0)
		{
			stIpMcast.vid = 0;
		}
		mcFlow.vlanID = stIpMcast.vid;
		
		
		mcFlow.port_mask.portmask = rg_portmask_get_by_rtk_portmask(stIpMcast.port_mask);
		if(RT_ERR_RG_OK != rtk_rg_multicastFlow_add(&mcFlow, &flow_idx))
		{				
			SYS_PRINTF("%s[%d]:	%s failed\n",__func__,__LINE__,__func__);
			return RT_ERR_FAILED;
		}
		/* keep in multicast lut DB for delete */
		drv_ip_mcast_add(&mcFlow, flow_idx);
	}
	return RT_ERR_OK;
}

int32 drv_ipv6McastEntry_add(ip_mcast_t stIpMcast)
{
	rtk_rg_multicastFlow_t mcFlow;
	int flow_idx;
	rtk_l2_ipmcMode_t l2mode;
	
	if(!ulIpMacstInited)
    {        
        memset(ipMcastEntryDB, 0, sizeof(ipMcastEntryDB));
        ulIpMacstInited = 1;        
    }

    memset(&mcFlow, 0x00, sizeof(rtk_rg_multicastFlow_t));
	
	mcFlow.isIPv6 = 1;
    mcFlow.multicast_ipv6_addr[0] = (ipaddr_t)(stIpMcast.dipv6[0]);
	mcFlow.multicast_ipv6_addr[1] = (ipaddr_t)(stIpMcast.dipv6[1]);
	mcFlow.multicast_ipv6_addr[2] = (ipaddr_t)(stIpMcast.dipv6[2]);
	mcFlow.multicast_ipv6_addr[3] = (ipaddr_t)(stIpMcast.dipv6[3]);
	
	/* because ASIC use GIP only replace GIP+VID */
	mcFlow.isIVL = 0;
	
	mcFlow.port_mask.portmask = rg_portmask_get_by_rtk_portmask(stIpMcast.port_mask);
	
	/* add old portmask */
	flow_idx = drv_ipMcastEntry_get(&stIpMcast);
	mcFlow.port_mask.portmask |= rg_portmask_get_by_rtk_portmask(stIpMcast.port_mask);

	if(RT_ERR_RG_OK != rtk_rg_multicastFlow_add(&mcFlow, &flow_idx))
	{
		SYS_PRINTF("rtk_rg_multicastFlow_add failed\n");
		return RT_ERR_FAILED;
	}
	SYS_PRINTF("rtk_rg_multicastFlow_add flowIdx[%d]\n", flow_idx);
	
	/* keep in multicast lut DB for delete */
	drv_ip_mcast_add(&mcFlow, flow_idx);
	return RT_ERR_OK;
}

int32 drv_ipv6McastEntry_del(ip_mcast_t stIpMcast)
{
	rtk_rg_multicastFlow_t mcFlow;
	rtk_portmask_t tmpPortmask;
	int flow_idx = -1, i, ret;
	
	if(!ulIpMacstInited)
    {        
        memset(ipMcastEntryDB, 0, sizeof(ipMcastEntryDB));
        ulIpMacstInited = 1;        
    }
	
	/* because ASIC use GIP only replace GIP+VID */
	stIpMcast.vid = 0;

	memcpy(&tmpPortmask, &(stIpMcast.port_mask), sizeof(rtk_portmask_t));
	flow_idx = drv_ipMcastEntry_get(&stIpMcast);
	if ( RT_ERR_FAILED == flow_idx ){
		SYS_PRINTF("%s[%d]: no this ip MC entry\n",__func__,__LINE__);
		return RT_ERR_FAILED;
	}

	//delete this entry
	if(RT_ERR_RG_OK != rtk_rg_multicastFlow_del(flow_idx))
	{				
		SYS_PRINTF("%s[%d]:	%s failed\n",__func__,__LINE__,__func__);
		return RT_ERR_FAILED;
	}
	SYS_PRINTF("rtk_rg_multicastFlow_del flowIdx[%d]\n", flow_idx);
	
	/* clear ipMcastEntryDB[flow_idx] */
	drv_ip_mcast_delete(flow_idx);
	
	RTK_PORTMASK_REMOVE(stIpMcast.port_mask, tmpPortmask);
	ret = PhyMaskNotNull(stIpMcast.port_mask);
	if (ret)
	{//renew port mask

		memset(&mcFlow, 0x00, sizeof(rtk_rg_multicastFlow_t));
	
		mcFlow.isIPv6 = 1;
		mcFlow.multicast_ipv6_addr[0] = (ipaddr_t)(stIpMcast.dipv6[0]);
		mcFlow.multicast_ipv6_addr[1] = (ipaddr_t)(stIpMcast.dipv6[1]);
		mcFlow.multicast_ipv6_addr[2] = (ipaddr_t)(stIpMcast.dipv6[2]);
		mcFlow.multicast_ipv6_addr[3] = (ipaddr_t)(stIpMcast.dipv6[3]);
		
		/* because ASIC use GIP only replace GIP+VID */
		mcFlow.isIVL = 0;		
		
		mcFlow.port_mask.portmask = rg_portmask_get_by_rtk_portmask(stIpMcast.port_mask);
		if(RT_ERR_RG_OK != rtk_rg_multicastFlow_add(&mcFlow, &flow_idx))
		{				
			SYS_PRINTF("%s[%d]:	%s failed\n",__func__,__LINE__,__func__);
			return RT_ERR_FAILED;
		}
		/* keep in multicast lut DB for delete */
		drv_ip_mcast_add(&mcFlow, flow_idx);
	}
	return RT_ERR_OK;
}

