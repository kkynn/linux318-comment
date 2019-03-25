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
 * $Revision: 39101 $
 * $Date: 2013-05-03 17:35:27 +0800 (週五, 03 五月 2013) $
 *
 * Purpose : Define the configuration of EPON OAM protocol stack
 *
 * Feature : 
 *
 */

#ifndef __EPON_OAM_CONFIG_H__
#define __EPON_OAM_CONFIG_H__
/* 
 * Symbol Definition 
 */
/* EPON OAM configurations */
#define EPON_OAM_SUPPORT_LLID_NUM       8
#define EPON_OAM_DEBUG                  1
/* In unit of 1 ms */
#define EPON_OAM_FAILOVER_GRANULARITY   (60)
#define EPON_OAM_FAILOVER_BACKOFF       (2000)

//#define SUPPORT_OAM_EVENT_TO_KERNEL	/* undef by default to avoid compile error if epon.h is old */

//#define YUEME_CUSTOMIZED_CHANGE	/* customized changes for yueme branch, Must disable in main trunk */

//#define SUPPORT_OAM_BYTEORDER /* support little endian when use ARM chip */

#define EPON_OAM_SUPPORT_TK             0
#define EPON_OAM_SUPPORT_KT             0
#define EPON_OAM_SUPPORT_CORTINA		1
#endif /* __EPON_OAM_CONFIG_H__ */

