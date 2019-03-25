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

/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/ponmac.h>
#include <dal/ca8279/dal_ca8279.h>
#include <dal/ca8279/dal_ca8279_ponmac.h>

#include "cortina-api/include/onu.h"
#include <aal/include/aal_pon.h>
#include <soc/cortina/ca8279_pon_registers.h>
/*
 * Symbol Definition
 */

/*
 * Macro Declaration
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
int32
dal_ca8279_ponmac_losState_get(rtk_enable_t *pState)
{
    ca_status_t  ret = CA_E_OK;
	GLOBAL_OPT_MODULE_STATUS_t opt_stas;

	//opt_stas.wrd = AAL_GLB_REG_READ(GLOBAL_OPT_MODULE_STATUS);

    if((ret = ca_onu_losState_get(0, &opt_stas)) == CA_E_OK)
    {
        if((1 == opt_stas.bf.opt_rx_los) || (1 == opt_stas.bf.opt_mod_abs))
            *pState = ENABLED;
        else
            *pState = DISABLED;
    }

    return ret;
}   /* end of dal_ca8279_ponmac_losState_get */

