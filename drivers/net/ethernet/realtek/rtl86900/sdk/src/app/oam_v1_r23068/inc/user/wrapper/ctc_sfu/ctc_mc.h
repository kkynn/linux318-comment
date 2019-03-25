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

#ifndef __IGSP_TYPE_H__
#define __IGSP_TYPE_H__
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common/rt_type.h>
#include <sys_def.h>

#include "drv_clf.h"
#include <sys_portview.h>

/*
 * Data Type Declaration
 */
typedef rtk_mac_t sys_mac_t;

typedef uint32  sys_ipv4_addr_t;

#define     IPV6_ADDR_LEN   16

#define     SYS_IPV6_NUM_LEN IPV6_ADDR_LEN
typedef struct sys_ipv6_addr_s
{
    uint8   ipv6_addr[IPV6_ADDR_LEN];
} sys_ipv6_addr_t;

#define   VLAN_BUF_LEN   512

#ifndef MAX_GROUP_NUM_PER_PORT
#define MAX_GROUP_NUM_PER_PORT 64
#endif
#define CTC_START_PORT_NUM 1 /* ctc oam spec: portId start from 1 */

typedef uint8 vid_list_t[VLAN_BUF_LEN];
typedef vid_list_t   sys_vlanmask_t;

typedef int32 sys_pri_t;
typedef rtk_enable_t sys_enable_t;
typedef uint16  sys_vid_t;

typedef enum tagerr_code_e
{
    SYS_ERR_OK = 0,
    SYS_ERR_FAILED,
    SYS_ERR_NULL_POINTER,
    SYS_ERR_VLAN_ID,
    SYS_ERR_PORT_ID,
    SYS_ERR_L2_MAC_IS_EXIST,
    SYS_ERR_L2_MAC_FULL,
    SYS_ERR_MCAST_IPTYPE,
    SYS_ERR_INPUT,
    SYS_ERR_IGMP_GROUP_ENTRY_NOT_EXIST,
    SYS_ERR_IGMP_QUERIER_CHANGE,
    SYS_ERR_IGMP_PROFILE_ID,
    SYS_ERR_IGMP_PROFILE_NOT_EXIST, 
    SYS_ERR_MCAST_DATABASE_FULL,
    SYS_ERR_MLD_PROFILE_NOT_EXIST,
    SYS_ERR_IPV4_ADDR,
    SYS_ERR_MCAST_GROUP_TYPE,
    SYS_ERR_IGMP_REMOVE_PORT_OK
} err_code_e;
//#define SYS_ERR_IGMP_REMOVE_PORT_OK SYS_ERR_OK
/*
 * Symbol
 */
extern unsigned int debug_enable;
#define SYS_CPRINTF(fmt, args...) \
	do \
	{ \
		if(debug_enable) \
		{ \
			FILE *con_fp = fopen("/dev/console", "w"); \
            if (con_fp) \
            { \
                fprintf(con_fp, fmt, ##args); \
                fclose(con_fp); \
            } \
		} \
	}while (0)


//#define IGMP_TEST_MODE  /*test related code*/
#define SYS_PRINTF SYS_CPRINTF

#define SYS_DBG(fmt, arg...)\
do {\
    {\
      SYS_PRINTF(fmt,##arg);\
    }\
} while (0)

#define SYS_PARAM_CHK(expr, errCode)\
do {\
    if ((int32)(expr)) {\
        SYS_PRINTF("\n SYS_PARAM_CHK %s %d\n",__FUNCTION__,__LINE__);\
        return errCode; \
    }\
} while (0)

#define SYS_ERR_CHK(op)\
do {\
    if ((op) != SYS_ERR_OK) {\
        SYS_PRINTF("\n SYS_ERR_CHK %s %d\n",__FUNCTION__,__LINE__);\
        return SYS_ERR_FAILED;\
    }\
} while(0)

#define SYS_ERR_XLATE_CHK(op)\
    do {\
        if ((op) != SYS_ERR_OK) {\
            SYS_PRINTF("\n SYS_ERR_XLATE_CHK %s %d\n",__FUNCTION__,__LINE__);\
            return SYS_ERR_FAILED;\
        }\
    } while(0)


#define SYS_ERR_CONTINUE(ret) if(ret) continue

#define SYS_TRACE() SYS_PRINTF("\n%s,%d\n",__FUNCTION__,__LINE__)

#define VLANMASK_IS_VIDSET(vlanlist,vid) TEST_VID_LIST(vid,vlanlist)
#define VLANMASK_SET_VID(vlanlist,vid)   VID_LIST_SETBIT(vid,vlanlist)
#define VLANMASK_CLEAR_ALL(vlanlist)     memset(vlanlist,0,sizeof(vlanlist))
#define VLANMASK_SET_ALL(vlanMsk)        memset(vlanMsk,0xff,sizeof(vlanMsk))

typedef enum igmp_mode_e {
	IGMP_MODE_SNOOPING = 0,
	IGMP_MODE_CTC,
	IGMP_MODE_END
} igmp_mode_t;

struct lw_l2_ctl_s
{
    unsigned short    sll_port;
    unsigned short    sll_vlanid; 
};

typedef struct
{
	rtk_portmask_t portmask;
}vlan_tab_t;

typedef enum mcvlan_tag_oper_mode_e {
	TAG_OPER_MODE_TRANSPARENT=0,
	TAG_OPER_MODE_STRIP,
	TAG_OPER_MODE_TRANSLATION
} mcvlan_tag_oper_mode_t;

typedef struct igmp_vlan_translation_entry_s {
	unsigned int enable;
	unsigned int portId;
	unsigned int mcVid;
	unsigned int userVid;
} igmp_vlan_translation_entry_t;

typedef struct oam_mcast_vlan_translation_entry{
	unsigned short   mcast_vlan;
	unsigned short   iptv_vlan;
}__attribute__((packed)) oam_mcast_vlan_translation_entry_t;

typedef struct ctc_igmp_control_s {
	unsigned int mcRecvEn;
	unsigned int mcMode;
	unsigned int leaveMode;
	unsigned int ctcControlType;
} ctc_igmp_control_t;

typedef enum mc_control_s
{
	MC_CTRL_GDA_MAC			= 0x00,
	MC_CTRL_GDA_MAC_VID		= 0x01,
	MC_CTRL_GDA_GDA_SA_IP	= 0x02,
	MC_CTRL_GDA_GDA_IP_VID	= 0x03,
	MC_CTRL_GDA_GDA_IP6_VID = 0x04,
	MC_CTRL_END/* End of multicast controls */
} mc_control_e;

typedef enum igmp_leave_mode_s {
	IGMP_LEAVE_MODE_NON_FAST_LEAVE = 1,
	IGMP_LEAVE_MODE_FAST
} igmp_leave_mode_t;

typedef struct igmp_mc_vlan_entry_s
{
	sys_logic_portmask_t portmask;
	uint32  vid;
}igmp_mc_vlan_entry_t;

#endif

