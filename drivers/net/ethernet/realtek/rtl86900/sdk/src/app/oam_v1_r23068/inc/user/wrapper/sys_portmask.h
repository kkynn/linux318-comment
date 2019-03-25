/*
 * Copyright (C) 2010 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 9753 $
 * $Date: 2010-05-21 14:57:08 +0800 (Fri, 21 May 2010) $
 *
 * Purpose : Define software system utility
 *
 * Feature : System utility definition
 *
 */

#ifndef __COMMON_SYS_PORTMASK_H__
#define __COMMON_SYS_PORTMASK_H__

/*
 * Include Files
 */
#include <sys_bitmap.h>

extern int LAN_PORT_NUM;
/*
 * Macro Definition
 */


/**********************************************************
 *
 *                           Logical Port
 *
 **********************************************************/

#define LOGIC_PORTMASK_SET_PORT(portmask, port) \
do{ \
    SYS_BITMAP_SET_BIT((portmask).bits, port); \
} while(0)

#define LOGIC_PORTMASK_CLEAR_PORT(portmask, port) \
do{ \
    SYS_BITMAP_CLEAR_BIT((portmask).bits, port); \
} while(0)

#define LOGIC_PORTMASK_SET_ALL(lPortmask) \
do{ \
    SYS_BITMAP_SET_ALL((lPortmask).bits, SYS_LOGIC_PORT_NUM); \
} while(0)

#define LOGIC_PORTMASK_CLEAR_ALL(lPortmask) \
do{ \
    SYS_BITMAP_CLEAR_ALL((lPortmask).bits, SYS_LOGIC_PORT_NUM); \
} while(0)

#define LOGIC_PORTMASK_IS_CLEAR(portmask, result) \
    SYS_BITMAP_IS_CLEAR((portmask).bits, SYS_LOGIC_PORT_NUM, result)

#define LOGIC_PORTMASK_COPY(dst, src) \
    SYS_BITMAP_COPY((dst).bits, (src).bits, SYS_LOGIC_PORT_NUM)

#define LOGIC_PORTMASK_COMPLEMENT(dst, src) \
    SYS_BITMAP_COMPLEMENT((dst).bits, (src).bits, SYS_LOGIC_PORT_NUM)

#define LOGIC_PORTMASK_AND(dst, portmask1, portmask2) \
    SYS_BITMAP_AND((dst).bits, (portmask1).bits, (portmask2).bits, SYS_LOGIC_PORT_NUM)

#define LOGIC_PORTMASK_ANDNOT(dst, portmask1, portmask2) \
    SYS_BITMAP_ANDNOT((dst).bits, (portmask1).bits, (portmask2).bits, SYS_LOGIC_PORT_NUM)

#define LOGIC_PORTMASK_OR(dst, portmask1, portmask2) \
    SYS_BITMAP_OR((dst).bits, (portmask1).bits, (portmask2).bits, SYS_LOGIC_PORT_NUM)

#define LOGIC_PORTMASK_XOR(dst, portmask1, portmask2) \
    SYS_BITMAP_XOR((dst).bits, (portmask1).bits, (portmask2).bits, SYS_LOGIC_PORT_NUM)

#define FOR_EACH_LAN_PORT(port)  for((port) = 0; (port) < LAN_PORT_NUM; (port++))

#define IS_LOGIC_PORTMASK_PORTSET(portmask, port) \
    (((port) <= LOGIC_PORT_END) ? SYS_BITMAP_IS_BITSET((portmask).bits, port) : 0)
        
#define IS_LOGIC_PORTMASK_PORTCLEAR(portmask, port) \
    (((port) <= LOGIC_PORT_END) ? SYS_BITMAP_IS_BITCLEAR((portmask).bits, port) : 0)

#define IS_LOGIC_PORTMASK_CLEAR(portmask)  \
    (sys_util_bitmaskIsClear_ret(portmask.bits, LOGIC_PORT_START, SYS_LOGIC_PORT_NUM))
        
#define IS_LOGIC_PORTMASK_ALLSET(portmask)  \
    (sys_util_bitmaskIsAllSet_ret(portmask.bits, LOGIC_PORT_START, SYS_LOGIC_PORT_NUM))

#define IS_LOGIC_PORTMASK_EQUAL(portmask1, portmask2)  \
    (sys_util_bitmaskIsEqual_ret(portmask1.bits, portmask2.bits, SYS_LOGIC_PORT_NUM))

#endif /* __COMMON_SYS_PORTMASK_H__ */

