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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/rt_type.h>
#include <common/rt_error.h>
#include <common/util/rt_util.h>
#include <osal/print.h>
#include <hal/common/halctrl.h>
#include "sys_portmask.h"
#include "drv_convert.h"
#include "rtk_rg_define.h"
#include "epon_oam_igmp.h"

int LAN_PORT_NUM = 4;
/*input logical port ID, return truly mapping physical port ID*/
static int re_map_tbl[]={0,1,2,3};

unsigned int get_virtual_lan_port(unsigned short lport)
{
	int i;
	
	for(i = 0; i < LAN_PORT_NUM; i++)
	{
		if(re_map_tbl[i] == lport)
			return i;
	}
    
    printf("fatal error, unknown logical port %d meets, why???\n", lport);
	return 0;
}

unsigned int get_logical_lan_port(unsigned int vport)
{
	if (!IsValidLgcPort(vport)) 
    {
        return INVALID_PORT;
   	}
	return re_map_tbl[vport];
}

void set_port_remapping_table(int * table, int num)
{
	int i;

    printf("-----------%s-----------\n", __func__);
	for(i = 0; i < LAN_PORT_NUM; i++)
	{
		if(i >= num)
			break;
		re_map_tbl[i] = table[i];
        printf("LAN %d PHY %d\n", i, table[i]);
	}
}

void show_port_mapping_table()
{
	int i;

	printf("Port Mapping Table\n");
	printf("logcial port:   ");
	
	for(i = 0; i < LAN_PORT_NUM; i++)
	{
		printf("%d  ", i);
	}
	printf("\nphysical port:  ");
	for(i = 0; i < LAN_PORT_NUM; i++)
	{
	
		printf("%d  ", re_map_tbl[i]);
	}
	printf("\n");
}

int32 IsValidLgcPort(uint32 ucVirtualPort)
{
    if ((LOGIC_CPU_PORT != ucVirtualPort) && 
        (LOGIC_PON_PORT != ucVirtualPort) &&
        (ucVirtualPort >= LAN_PORT_NUM)) 
    {
        return FALSE;
    }
    else
	{
        return TRUE;
    }
}

int32 IsValidLgcLanPort(uint32 ucVirtualPort)
{
    if (ucVirtualPort >= LAN_PORT_NUM) 
    {
        return FALSE;
    }
    else
	{
        return TRUE;
    }
}

uint32 PortLogic2PhyID(uint32 ucVirtualPort)
{
	uint32 uiPPort, ucLogicPort;
	
	if (!IsValidLgcPort(ucVirtualPort)) 
	{
        return INVALID_PORT;
    }

	if(ucVirtualPort < LAN_PORT_NUM)
	{
		ucLogicPort = get_logical_lan_port(ucVirtualPort);
		if(ucLogicPort == INVALID_PORT)
			return INVALID_PORT;

#if 0   /* Hope Bug is fixed */
		/*siyuan 2016-10-8: no need to get lan port by rtk_switch_phyPortId_get 
		  and rtk_switch_phyPortId_get do lan port remapping in 9602B chip which conflicts with boa port remapping setting */ 
		uiPPort = ucLogicPort;
#else
		if (RT_ERR_OK != rtk_switch_phyPortId_get(ucLogicPort, &uiPPort))
			return INVALID_PORT;
#endif
	}
	else
	{
		ucLogicPort = ucVirtualPort;
		if (RT_ERR_OK != rtk_switch_phyPortId_get(ucLogicPort, &uiPPort))
			return INVALID_PORT;
	}
  
	return uiPPort;
}

int32 PhyMaskNotNull(rtk_portmask_t stPhyMask)
{
    uint32 lport, vport;
	uint32 uiPPort;
    
    FOR_EACH_LAN_PORT(vport)
    {
        lport = get_logical_lan_port(vport);
    	uiPPort = PortLogic2PhyID(lport);
        if(RTK_PORTMASK_IS_PORT_SET(stPhyMask, uiPPort))
		{
            return TRUE;
        }
    }
    return FALSE;
}

