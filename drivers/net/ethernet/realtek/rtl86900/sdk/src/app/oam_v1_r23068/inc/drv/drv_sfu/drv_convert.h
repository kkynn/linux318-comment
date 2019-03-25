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

#ifndef __RTK_DRV_CONVERT_H__
#define __RTK_DRV_CONVERT_H__

#include <common/rt_type.h>
#include <rtk/switch.h>
#include <sys_portmask.h>

extern int maxLanPortNum;

#define MAX_PORT_NUM	4
//#define LOGIC_PON_PORT 	4
#define PHYSICAL_PON_PORT	4
#define LOGIC_PON_PORT	RTK_PORT_PON
#define LOGIC_CPU_PORT  RTK_PORT_CPU

#define INVALID_PORT    0xFFFFFFFF		

#define MIN_VLAN_ID		1
#define MAX_VLAN_ID		4094

#define MAC_ADDR_LEN	6
#define RTL_HEADER_OFF	(2 * MAC_ADDR_LEN)

#define VALID_VLAN_ID(x) ((((x) >= MIN_VLAN_ID) && ((x) <= MAX_VLAN_ID))? 1 : 0)

#define IS_MULTICAST(x) (x[0]&0x1)

#define BYTE_SET_BIT(_bit, _byte) do{ \
    (_byte) |= (uint8)((uint8)1 << (uint8)(_bit)); \
}while(0)

#define VID_LIST_SETBIT(_vid, _pucBuf) do{ \
    BYTE_SET_BIT((uint32)(_vid) % 8, ((uint8 *)(_pucBuf))[(_vid) / 8]); \
}while(0)

#define BITMAP_REMOVE(dstArray, srcArray, length) \
do { \
    uint32  array_index;\
    for (array_index = 0; array_index < length; array_index++) \
    { \
        dstArray[array_index] &= ~srcArray[array_index]; \
    } \
} while(0)

#define RTK_PORTMASK_REMOVE(dstPortmask, srcPortmask) \
    BITMAP_REMOVE((dstPortmask).bits, (srcPortmask).bits, RTK_TOTAL_NUM_OF_WORD_FOR_1BIT_PORT_LIST)

#define TEST_BIT_8(_bit, _byte) (((uint8)(_byte) & ((uint8)1 << (uint8)(_bit))) != 0)

#define TEST_VID_LIST(_vid, _pucBuf) (TEST_BIT_8((_vid) % 8, ((uint8 *)(_pucBuf))[(_vid) / 8]))

#define ForEachListVlan(_vid, _vidListBuf)                     \
    for((_vid=1); (_vid)<=4094; (_vid)++)                       \
        if(TEST_VID_LIST((_vid), (_vidListBuf)))

extern int32 IsValidLgcPort(uint32 ucVirtualPort);

extern int32 IsValidLgcLanPort(uint32 ucVirtualPort);

extern uint32 PortLogic2PhyID(uint32 ucVirtualPort);

extern int32 PhyMaskNotNull(rtk_portmask_t stPhyMask);

extern unsigned int get_virtual_lan_port(unsigned short lport);
extern unsigned int get_logical_lan_port(unsigned int vport);
extern void set_port_remapping_table(int * table, int num);
extern void show_port_mapping_table();

#endif /* __RTK_DRV_CONVERT_H__ */
