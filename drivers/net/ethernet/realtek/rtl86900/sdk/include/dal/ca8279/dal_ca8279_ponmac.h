/*
 * Copyright (C) 2018 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: $
 * $Date: $
 *
 * Purpose : Definition of PON MAC API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) PON mac
 */


#ifndef __DAL_CA8279_PONMAC_H__
#define __DAL_CA8279_PONMAC_H__


/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/ponmac.h>

/*
 * Symbol Definition
 */
/*
 * Data Declaration
 */


/*
 * Function Declaration
 */

/* Function Name:
 *      dal_ca8279_ponmac_losState_get
 * Description:
 *      Get the current optical lost of signal (LOS) state
 * Input:
 *      None
 * Output:
 *      pEnable  - the current optical lost of signal state
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 */
extern int32
dal_ca8279_ponmac_losState_get(rtk_enable_t *pState);

#endif /* __DAL_CA8279_PONMAC_H__ */
