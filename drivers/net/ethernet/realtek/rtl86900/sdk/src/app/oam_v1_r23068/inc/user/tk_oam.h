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
 * Purpose : Define the TK related extended OAM
 *
 * Feature : Provide TK related extended OAM parsing and handling
 *
 */

#ifndef __TK_OAM_H__
#define __TK_OAM_H__

/*
 * Include Files
 */

/* 
 * Symbol Definition 
 */
/* CTC extended OAM configuration */

#define TK_OAM_OUI                         { 0x00, 0x0d, 0xb6 }

#if 0
/* CTC Extended Discovery FSM States */
#define CTC_OAM_FSM_STATE_WAIT_REMOTE       1
#define CTC_OAM_FSM_STATE_WAIT_REMOTE_OK    2
#define CTC_OAM_FSM_STATE_COMPLETE          3
#endif

/* Performance monitor thread checking interval in seconds */
#define TK_OAM_KEEPALIVE_INTERVAL           (1)

/*
 * Macro Definition
 */

/*  
 * Function Declaration  
 */
extern int tk_oam_init(void);
extern int tk_oam_db_init(unsigned char llidIdx);

#endif /* __TK_OAM_H__ */


