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

#ifndef __SYS_DEF_H__
#define __SYS_DEF_H__

#include "drv_convert.h"

#define LOGIC_PORT_START               0
#define ONU_STYLE                      1

#define LOGIC_PORT_END                 MAX_PORT_NUM

#define SYS_LOGIC_PORT_NUM             (MAX_PORT_NUM)


#define SYS_MCAST_MAX_DB_NUM		512
#define SYS_MCAST_MAX_GROUP_NUM      SYS_MCAST_MAX_DB_NUM
#define SYS_IGMP_PORT_LIMIT_ENTRY_NUM 64
#define SYS_VLAN_STATIC_NUM           512

#define SYS_IGMP_MAX_GROUP_NUM        256
#define SYS_MLD_MAX_GROUP_NUM        256


//add for MLD
#define SYS_MLD_MAX_GROUP_NUM        256
#define SYS_MLD_PORT_LIMIT_ENTRY_NUM 256


#define SYS_IGMP_MIN_PROFILE_ID 1
#define SYS_IGMP_PROFILE_SIZE   100

#define SYS_CPU_PORT_PRIORITY_IGMP    6
#define SYS_CPU_PORT_NO               LOGIC_CPU_PORT
#define SYS_THREAD_PRI_IGMP_TIMER     1


#endif
