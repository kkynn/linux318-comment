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
 * $Revision: 9344 $
 * $Date: 2010-05-03 21:45:31 +0800 (Mon, 03 May 2010) $
 *
 * Purpose : Initialize user/logic/physic port mapping and provide translation funtions
 *
 * Feature : Initialize user/logic/physic port mapping and provide translation funtions
 *
 */

#ifndef __COMMON_SYS_PORTVIEW_H__
#define __COMMON_SYS_PORTVIEW_H__

/*
 * Include Files
 */
#include <sys_def.h>
#include <sys_bitmap.h>


#define RTK_PORT_MASK_WORD_MAX SYS_BITS_TO_LONGS(SYS_LOGIC_PORT_NUM)

/*
 * Data Type Declaration
 */

typedef uint32 sys_user_port_t;
typedef uint32 sys_logic_port_t;

typedef struct sys_logic_portmask_s
{
    uint32 bits[RTK_PORT_MASK_WORD_MAX];
} sys_logic_portmask_t;


extern uint32 sys_util_bitmaskIsClear_ret(uint32* addr, uint32 startport, uint32 length);
extern uint32 sys_util_bitmaskIsAllSet_ret(uint32* addr, uint32 startport, uint32 length);
extern uint32 sys_util_bitmaskIsEqual_ret(uint32* addr1, uint32* addr2, uint32 bits);

#endif /* __COMMON_SYS_PORTVIEW_H__ */

