/*
* Copyright (C) 2015 Realtek Semiconductor Corp.
* All Rights Reserved.
*
* This program is the proprietary software of Realtek Semiconductor
* Corporation and/or its licensors, and only be used, duplicated,
* modified or distributed under the authorized license from Realtek.
*
* ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
* THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
*
* $Revision: 1 $
* $Date: 2015-09-18 11:22:15 +0800 (Fri, 18 Sep 2015) $
*
* Purpose : 
*
* Feature : 
*
*/

#ifndef __EPON_OAM_IGMP_UTIL_H__
#define __EPON_OAM_IGMP_UTIL_H__

#include <common/rt_type.h>
#include <semaphore.h>
#include "ctc_mc.h"

#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN	6
#endif

#ifndef ETH_FRAME_LEN
#define ETH_FRAME_LEN 1514
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAIL
#define FAIL -1
#endif

//util usage start
#define IGMP_SEM_INIT(sem)    \
	do {\
		sem_init(&sem, 0, 1);\
	} while(0)

#define IGMP_SEM_LOCK(sem)    \
	do {\
		sem_wait(&sem);\
	} while(0)
	
#define IGMP_SEM_UNLOCK(sem) \
	do {\
		sem_post(&sem);\
	} while(0)

#define IGMP_IN_CLASSD(a)        ((((long int) (a)) & 0xf0000000) == 0xe0000000)
#define IGMP_IN_MULTICAST(a)     IGMP_IN_CLASSD(a)

//uitl usage end

//debug usage start
#define MAC_PRINT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_PRINT_ARG(mac) \
    (mac)[0], (mac)[1], (mac)[2], (mac)[3], (mac)[4], (mac)[5]

#define IPADDR_PRINT "%d.%d.%d.%d"
#define IPADDR_PRINT_ARG(ip) \
    ((ip) & 0xFF000000) >> 24, ((ip) & 0xFF0000) >> 16, ((ip) & 0xFF00) >> 8, (ip) & 0xFF

#define IPADDRV6_PRINT "%X:%X:%X:%X:%X:%X:%X:%X"
#define IPADDRV6_PRINT_ARG(ipv6) \
    *((uint16 *)&ipv6[0]), *((uint16 *)&ipv6[2]), *((uint16 *)&ipv6[4]), *((uint16 *)&ipv6[6]), \
    *((uint16 *)&ipv6[8]), *((uint16 *)&ipv6[10]), *((uint16 *)&ipv6[12]), *((uint16 *)&ipv6[14])

//debug usage end
#endif
