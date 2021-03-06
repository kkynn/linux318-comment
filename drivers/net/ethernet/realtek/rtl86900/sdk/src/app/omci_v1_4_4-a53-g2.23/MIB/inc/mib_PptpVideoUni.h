/*
 * Copyright (C) 2014 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : Definition of ME attribute: PPTP video UNI (82)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME attribute: PPTP video UNI (82)
 */

#ifndef __MIB_PPTP_VIDEO_UNI_H__
#define __MIB_PPTP_VIDEO_UNI_H__

#ifdef __cplusplus
extern "C" {
#endif


#define MIB_TABLE_PPTP_VIDEO_UNI_ATTR_NUM (6)
#define MIB_TABLE_PPTP_VIDEO_UNI_ENTITY_ID_INDEX ((MIB_ATTR_INDEX)1)
#define MIB_TABLE_PPTP_VIDEO_UNI_ADMIN_STATE_INDEX ((MIB_ATTR_INDEX)2)
#define MIB_TABLE_PPTP_VIDEO_UNI_OP_STATE_INDEX ((MIB_ATTR_INDEX)3)
#define MIB_TABLE_PPTP_VIDEO_UNI_ARC_INDEX ((MIB_ATTR_INDEX)4)
#define MIB_TABLE_PPTP_VIDEO_UNI_ARC_INTVL_INDEX ((MIB_ATTR_INDEX)5)
#define MIB_TABLE_PPTP_VIDEO_UNI_PWR_CTRL_INDEX ((MIB_ATTR_INDEX)6)


typedef struct {
	UINT16	EntityId;
	UINT8	AdminState;
	UINT8	OpState;
	UINT8	Arc;
	UINT8	ArcIntvl;
    UINT8   PwrCtrl;
} __attribute__((packed)) MIB_TABLE_PPTP_VIDEO_UNI_T;


#ifdef __cplusplus
}
#endif

#endif
