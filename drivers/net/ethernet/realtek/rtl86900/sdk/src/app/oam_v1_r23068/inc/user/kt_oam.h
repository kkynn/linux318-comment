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
 * $Revision: 39101 $
 * $Date: 2013-05-03 17:35:27 +0800 (週五, 03 五月 2013) $
 *
 * Purpose : Define the KT related extended OAM
 *
 * Feature : Provide KT related extended OAM parsing and handling
 *
 */

#ifndef __KT_OAM_H__
#define __KT_OAM_H__

/*
 * Include Files
 */

/* 
 * Symbol Definition 
 */
/* KT extended OAM configuration */

#define KT_OAM_OUI                         { 0xaa, 0xaa, 0xaa }

/*
 * Macro Definition
 */

/*  
 * Function Declaration  
 */
extern int kt_oam_init(void);
extern int kt_oam_db_init(unsigned char llidIdx);

#endif /* __KT_OAM_H__ */


