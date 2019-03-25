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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <common/rt_type.h>

#include <ctc_mc.h>
#include <sys_portview.h>
#include <sys_portmask.h>


uint32 sys_util_bitmaskIsClear_ret(uint32* addr, uint32 startport, uint32 length) 
{ 
    uint32 i, tail; 
    
    tail = startport + length -1;
    if(tail > LOGIC_PORT_END-1)
    {
        tail = LOGIC_PORT_END-1;
    }
    
    for(i = startport; i <= tail; i++)
    { 
        if (SYS_BITMAP_IS_BITSET(addr, i)) 
        {
            return 0;
        }
    } 
    return 1;
}

uint32 sys_util_bitmaskIsAllSet_ret(uint32* addr, uint32 startport, uint32 length)
{ 
    uint32 i, tail;
    
    tail = startport + length -1;
    if(tail > LOGIC_PORT_END-1)
    {
        tail = LOGIC_PORT_END-1;
    }
     
    for(i = startport; i <= tail; i ++) 
    { 
        if (SYS_BITMAP_IS_BITCLEAR(addr, i)) 
        {
            return 0;
        }
    } 
    return 1;
}

uint32 sys_util_bitmaskIsEqual_ret(uint32* addr1, uint32* addr2, uint32 bits)
{ 
    uint32 i, n = SYS_BITS_TO_LONGS(bits); 

    for(i = 0; i < n; i ++)
    { 
        if (addr1[i] != addr2[i])
        {
            return 0;
        }
    } 
    return 1;
}

