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
#include <hal/common/halctrl.h>
#include <osal/print.h>
#include <rtk/l2.h>
#include <rtk/trap.h>

#include "drv_convert.h"
#include "drv_mac.h"
#include "drv_vlan.h"
#include "ctc_mc.h"


static vlan_fid_t *m_astVlanFid;

static mac_mcast_t _mac_mcast_entry_dump[MAX_MULTICAST_ENTRY];

static uint32 ulMacMcastCount = 0;

static uint32 ulMacMacstInited = 0;

static uint32 drv_mac_compare(mac_address_t address_1, mac_address_t address_2)
{
    uint32 i;

    for(i = 0; i< MAC_ADDR_LEN; i++)
    {
        if(address_1[i] != address_2[i])
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

static int32 drv_mac_mcast_add(mac_mcast_t *pstMacMcast)
{
    uint32 i;
    mac_address_t pMacAddr;
    
    memset(pMacAddr, 0, sizeof(mac_address_t));

    if(ulMacMcastCount >= MAX_MULTICAST_ENTRY)
    {
        return RT_ERR_FAILED;
    }
    for(i = 0; i < MAX_MULTICAST_ENTRY; i++)
    {
        if(0 == drv_mac_compare(_mac_mcast_entry_dump[i].mac_addr, pMacAddr))
        {
            _mac_mcast_entry_dump[i].tdVid = pstMacMcast->tdVid;
            memcpy(_mac_mcast_entry_dump[i].mac_addr, pstMacMcast->mac_addr, sizeof(mac_address_t));
            memcpy(&(_mac_mcast_entry_dump[i].port_mask), &(pstMacMcast->port_mask), sizeof(rtk_portmask_t));
            ulMacMcastCount++;
            return RT_ERR_OK;
        }
    }
	
    return RT_ERR_FAILED;
}

static int32 drv_mac_mcast_delete(mac_mcast_t *pstMacMcast)
{
    uint32 i;
    
    for(i = 0; i < MAX_MULTICAST_ENTRY; i++)
    {
        if(pstMacMcast->tdVid == _mac_mcast_entry_dump[i].tdVid)
        {
            if(!drv_mac_compare(pstMacMcast->mac_addr, _mac_mcast_entry_dump[i].mac_addr))
            {
                memset(&_mac_mcast_entry_dump[i], 0, sizeof(_mac_mcast_entry_dump[i]));
                ulMacMcastCount--;
                
                return RT_ERR_OK;
            }            
        }
    }
    
    return RT_ERR_FAILED;
}

static int32 drv_mac_mcast_set(mac_mcast_t *pstMacMcast)
{
    uint32 i;
    
    for(i = 0; i < MAX_MULTICAST_ENTRY; i++)
    {
        if(pstMacMcast->tdVid == _mac_mcast_entry_dump[i].tdVid)
        {
            if(!drv_mac_compare(pstMacMcast->mac_addr, _mac_mcast_entry_dump[i].mac_addr))
            {
                memcpy(&(_mac_mcast_entry_dump[i].port_mask), &(pstMacMcast->port_mask), sizeof(rtk_portmask_t));
                return RT_ERR_OK;
            }            
        }
    }

    return RT_ERR_FAILED;
}

static int32 drv_mac_mcast_get_by_vid(uint16 tdVid, mac_mcast_t *pstMacMcast)
{
    uint32 i;

    if (NULL == pstMacMcast)
    {
        return RT_ERR_FAILED;
    }
    
    for(i = 0; i < MAX_MULTICAST_ENTRY; i++)
    {
        if(tdVid == _mac_mcast_entry_dump[i].tdVid)
        {
            memcpy(pstMacMcast, &_mac_mcast_entry_dump[i], sizeof(mac_mcast_t));
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

	SYS_DBG("%s vid[%d] fid[%d] uiIndex[%d], memberPort[0x%x]\n", __func__, uiVid, uiFid, uiIndex, stPhyMask.bits[0]);
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

	return RT_ERR_OK;
}

int32 drv_mac_mcast_get_by_mac_and_vid(mac_address_t tdMac, uint16 tdVid, mac_mcast_t *pstMacMcast)
{
    uint32 i;

    if ((NULL == tdMac) || (NULL == pstMacMcast))
    {
        return RT_ERR_FAILED;
    }
    
    for(i = 0; i < MAX_MULTICAST_ENTRY; i++)
    {
        
        if(!drv_mac_compare(tdMac, _mac_mcast_entry_dump[i].mac_addr) && tdVid == _mac_mcast_entry_dump[i].tdVid)
        {
            memcpy(pstMacMcast, &_mac_mcast_entry_dump[i], sizeof(mac_mcast_t));
            return RT_ERR_OK;
        }            
    }
    return RT_ERR_FAILED;
}

int32 drv_mac_mcast_set_by_mac_and_fid(mac_mcast_t stMacMcast)
{
    int32 bSamePort;
    uint32 uiFid;
    int32 ret;
    rtk_portmask_t phy_mask;
	rtk_portmask_t phy_mask_tmp;
    mac_mcast_t stMcastMacEntryTmp;
    int32 rtkApiRet;
	rtk_l2_mcastAddr_t stMcastAddr;

	memcpy(&phy_mask, &(stMacMcast.port_mask), sizeof(rtk_portmask_t));
    
    /*Check same multicast mac entry.*/
    ret = drv_mac_mcast_get_by_mac_and_vid(stMacMcast.mac_addr, stMacMcast.tdVid, &stMcastMacEntryTmp);
    
    if (RT_ERR_OK == ret)
    {
		if(!RTK_PORTMASK_COMPARE(stMacMcast.port_mask, stMcastMacEntryTmp.port_mask))
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

		memcpy(&phy_mask_tmp, &(stMcastMacEntryTmp.port_mask), sizeof(rtk_portmask_t));
        /*Add new ports to existed port mask.*/
		RTK_PORTMASK_OR(phy_mask, phy_mask_tmp);
    }

    drv_valid_fid_get(stMacMcast.tdVid, &uiFid);
    if (HAL_VLAN_FID_MAX() < uiFid)
    {
        return RT_ERR_FAILED;
    }
    
    memset(&stMcastAddr, 0, sizeof(stMcastAddr));
    memcpy(stMcastAddr.mac.octet, stMacMcast.mac_addr, sizeof(mac_address_t));
	stMcastAddr.fid = uiFid;

	if(stMacMcast.fidMode == VLAN_FID_IVL)
	{
		stMcastAddr.flags |= RTK_L2_MCAST_FLAG_IVL;
		stMcastAddr.vid = stMacMcast.tdVid;
	}
	
    memcpy(&stMcastAddr.portmask, &phy_mask, sizeof(rtk_portmask_t));
    rtkApiRet = rtk_l2_mcastAddr_add(&stMcastAddr);
    if (RT_ERR_OK != rtkApiRet)
    {
        return RT_ERR_FAILED;
    }

    if(!ulMacMacstInited)
    {        
        memset(_mac_mcast_entry_dump, 0, sizeof(_mac_mcast_entry_dump));
        ulMacMacstInited = 1;        
    }
    
    /*Renew the existed multicast mac entry.*/
    if (RT_ERR_OK == ret)
    {
        drv_mac_mcast_set(&stMacMcast);
    }
    /*Add a new multicast mac entry.*/
    else
    {
        drv_mac_mcast_add(&stMacMcast);
    }
    
    return RT_ERR_OK;
}

int32 drv_mc_mac_delete_by_mac_and_fid(mac_mcast_t stMacMcast)
{
    int32 bSamePort;
	uint32 uiFid;
    int32 ret;
    int32 rtkApiRet;
    rtk_portmask_t phy_mask;
	rtk_portmask_t phy_mask_tmp;
    mac_mcast_t stMcastMacEntryTmp;
	rtk_l2_mcastAddr_t stMcastAddr;

    if(!VALID_VLAN_ID(stMacMcast.tdVid))
    {
        return RT_ERR_INPUT;
    }

    if(!IS_MULTICAST(stMacMcast.mac_addr))
    {
        /*unicast*/
        return RT_ERR_INPUT;
    }

	memset(&stMcastAddr, 0, sizeof(stMcastAddr));
    memcpy(stMcastAddr.mac.octet, stMacMcast.mac_addr, sizeof(rtk_mac_t));
    
    if(!ulMacMacstInited)
    {        
        memset(_mac_mcast_entry_dump, 0, sizeof(_mac_mcast_entry_dump));
        ulMacMacstInited = 1;        
    }
    
     ret = drv_mac_mcast_get_by_mac_and_vid(stMacMcast.mac_addr, stMacMcast.tdVid, &stMcastMacEntryTmp);

    /*Find the entry.*/
    if (RT_ERR_OK == ret)
    {                
		if(!RTK_PORTMASK_COMPARE(stMacMcast.port_mask, stMcastMacEntryTmp.port_mask))
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

		stMcastAddr.fid = uiFid;
		if(stMacMcast.fidMode == VLAN_FID_IVL)
		{
			stMcastAddr.flags |= RTK_L2_MCAST_FLAG_IVL;
			stMcastAddr.vid = stMacMcast.tdVid;
		}
		
        /*Delete it*/
        if (TRUE == bSamePort)
        {        					
            if(RT_ERR_OK != rtk_l2_mcastAddr_del(&stMcastAddr))
            {
                return RT_ERR_FAILED;
            }

            drv_mac_mcast_delete(&stMcastMacEntryTmp);
            
            return RT_ERR_OK;
        }
        /*Renew the existed entry.*/
        else
        {
			memcpy(&phy_mask, &(stMacMcast.port_mask), sizeof(rtk_portmask_t));
			memcpy(&phy_mask_tmp, &(stMcastMacEntryTmp.port_mask), sizeof(rtk_portmask_t));
            /*Delete ports from existed port mask.*/
			RTK_PORTMASK_REMOVE(phy_mask_tmp, phy_mask);

            memcpy(&stMcastAddr.portmask, &phy_mask_tmp, sizeof(rtk_portmask_t));
            rtkApiRet = rtk_l2_mcastAddr_add(&stMcastAddr);
            if (RT_ERR_OK != rtkApiRet)
            {
                return RT_ERR_FAILED;
            }

			RTK_PORTMASK_REMOVE(stMcastMacEntryTmp.port_mask, stMacMcast.port_mask);
            drv_mac_mcast_set(&stMcastMacEntryTmp);
        }
    }
    
    return RT_ERR_OK;
}

int32 drv_ipMcastMode_set(uint32 mode)
{
	int32 ret;
	ret = rtk_l2_ipmcMode_set(mode);
	SYS_PRINTF("%s ret %d, set mode %d\n", __func__, ret, mode);
	return ret;
}

int32 drv_ipMcastEntry_add(ip_mcast_t stIpMcast)
{
	rtk_l2_ipMcastAddr_t ipmcAddr;
	uint32 uiLPort, uiPPort;
	rtk_portmask_t isInMask, isExMask, maskI, maskE, retMask;
	rtk_l2_ipmcMode_t l2mode;
	mac_address_t pMacAddr;
	
	RTK_PORTMASK_RESET(isInMask);
    RTK_PORTMASK_RESET(isExMask);
	RTK_PORTMASK_RESET(retMask);

    memset(&ipmcAddr, 0x00, sizeof(rtk_l2_ipMcastAddr_t));
    ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_STATIC;
    ipmcAddr.dip = (ipaddr_t)(stIpMcast.dip);
    ipmcAddr.sip = (ipaddr_t)(stIpMcast.sip);
	ipmcAddr.vid = stIpMcast.vid;
	memcpy(&(ipmcAddr.portmask), &(stIpMcast.port_mask), sizeof(rtk_portmask_t));

	if(RT_ERR_OK != rtk_l2_ipmcMode_get(&l2mode))
	{
		SYS_PRINTF("rtk_l2_ipmcMode_get failed\n");
	}

	if(l2mode == LOOKUP_ON_MAC_AND_VID_FID)
	{
		SYS_PRINTF("ip mcast mode is mac_fid, not work\n");
	}
	
	/*9601b not support LOOKUP_ON_DIP_AND_SIP mode*/
	if(0 == stIpMcast.sip && l2mode != LOOKUP_ON_DIP_AND_VID)
		ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_DIP_ONLY; 
			
	if(RT_ERR_OK != rtk_l2_ipMcastAddr_add(&ipmcAddr))
	{
		SYS_PRINTF("rtk_l2_ipMcastAddr_add failed\n");
		return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}

int32 drv_ipMcastEntry_del(ip_mcast_t stIpMcast)
{
	rtk_l2_ipMcastAddr_t ipmcAddr;
	rtk_l2_ipmcMode_t l2mode;
	mac_address_t pMacAddr;
	
    memset(&ipmcAddr, 0x00, sizeof(rtk_l2_ipMcastAddr_t));
	
	if(RT_ERR_OK != rtk_l2_ipmcMode_get(&l2mode))
	{
		SYS_PRINTF("rtk_l2_ipmcMode_get failed\n");
	}

    ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_STATIC;
	if(0 == stIpMcast.sip && l2mode != LOOKUP_ON_DIP_AND_VID)
		ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_DIP_ONLY;
	
    ipmcAddr.dip = (ipaddr_t)(stIpMcast.dip);
	if (l2mode != LOOKUP_ON_DIP_AND_VID)
    	ipmcAddr.sip = (ipaddr_t)(stIpMcast.sip);
	
	ipmcAddr.vid = stIpMcast.vid;
	memcpy(&(ipmcAddr.portmask), &(stIpMcast.port_mask), sizeof(rtk_portmask_t));
	
	if(RT_ERR_OK != rtk_l2_ipMcastAddr_del(&ipmcAddr))
	{
		SYS_PRINTF("rtk_l2_ipMcastAddr_del failed\n");
		return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}

int32 drv_ipMcastEntry_get(ip_mcast_t *pStIpMcast)
{
	rtk_l2_ipMcastAddr_t ipmcAddr;
	rtk_l2_ipmcMode_t l2mode;
	mac_address_t pMacAddr;
	rtk_portmask_t etherPm;

    osal_memset(&ipmcAddr, 0x00, sizeof(rtk_l2_ipMcastAddr_t));
	
	if(RT_ERR_OK != rtk_l2_ipmcMode_get(&l2mode))
	{
		SYS_PRINTF("rtk_l2_ipmcMode_get failed\n");
	}

    ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_STATIC;
	
	if(0 == pStIpMcast->sip && l2mode != LOOKUP_ON_DIP_AND_VID)
		ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_DIP_ONLY;

    ipmcAddr.dip = (ipaddr_t)(pStIpMcast->dip);
	if(l2mode != LOOKUP_ON_DIP_AND_VID)
    	ipmcAddr.sip = (ipaddr_t)(pStIpMcast->sip);
	ipmcAddr.vid = pStIpMcast->vid;

	
	if(RT_ERR_OK != rtk_l2_ipMcastAddr_get(&ipmcAddr))
	{
	    drv_ipMcastEntry_del(*pStIpMcast);
	    SYS_PRINTF("%s() %d drv_ipMcastEntry_del failed\n", __FUNCTION__, __LINE__);
	    return RT_ERR_FAILED;
	}
	memcpy(&(pStIpMcast->port_mask), &(ipmcAddr.portmask), sizeof(rtk_portmask_t)); 
	
	return RT_ERR_OK;
}

/* TODO: support ipv6 mcast for 9602c sfu */
int32 drv_ipv6McastEntry_add(ip_mcast_t stIpMcast)
{
	rtk_l2_ipMcastAddr_t ipmcAddr;
	
	osal_memset(&ipmcAddr, 0x00, sizeof(rtk_l2_ipMcastAddr_t));
    ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_STATIC;
    ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_IPV6;

	memcpy(ipmcAddr.dip6.ipv6_addr,(unsigned char *)stIpMcast.dipv6,IPV6_ADDR_LEN);
	memcpy(&(ipmcAddr.portmask), &(stIpMcast.port_mask), sizeof(rtk_portmask_t));
	
	if(RT_ERR_OK != rtk_l2_ipMcastAddr_add(&ipmcAddr))
	{
		SYS_PRINTF("rtk_l2_ipMcastAddr_add failed\n");
		return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}

int32 drv_ipv6McastEntry_del(ip_mcast_t stIpMcast)
{
	rtk_l2_ipMcastAddr_t ipmcAddr;

    osal_memset(&ipmcAddr, 0x00, sizeof(rtk_l2_ipMcastAddr_t));
    ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_STATIC;
    ipmcAddr.flags |= RTK_L2_IPMCAST_FLAG_IPV6;

	memcpy(ipmcAddr.dip6.ipv6_addr,(unsigned char *)stIpMcast.dipv6,IPV6_ADDR_LEN);

	if(RT_ERR_OK != rtk_l2_ipMcastAddr_del(&ipmcAddr))
	{
		SYS_PRINTF("rtk_l2_ipMcastAddr_del failed\n");
		return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}
