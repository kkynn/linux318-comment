/*
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 *
 * *
 * $Revision:  $
 * $Date: 2012-08-07
 *
 * Purpose : GPON MAC register access APIs
 *
 * Feature : Provide the APIs to access GPON MAC
 *
 */

/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/port.h>
#include <dal/ca8279/dal_ca8279.h>
#include <dal/ca8279/dal_ca8279_gpon.h>
#include <osal/time.h>

#include "cortina-api/include/gpon.h"
#include "cortina-api/include/port.h"
#include "aal/include/aal_pon.h"
//#include "aal/include/aal_gpon.h"
#include "scfg.h"

static uint32 gpon_init = {INIT_NOT_COMPLETED};

/* Function Name:
 *      dal_ca8279_gpon_init
 * Description:
 *      gpon register level initial function
 * Input:
 *
 * Output:
 *      
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 dal_ca8279_gpon_init(void)
{
    int32 ret=RT_ERR_OK;
    ca_uint32_t pon_mode;

    pon_mode = aal_pon_mac_mode_get(0);

    if(pon_mode != ONU_PON_MAC_XGPON1 && pon_mode != ONU_PON_MAC_XGSPON)
    {
        RT_ERR(ret, (MOD_DAL|MOD_GPON), "");
        gpon_init = INIT_NOT_COMPLETED;
        return RT_ERR_OK;
    }

    gpon_init = INIT_COMPLETED;
    return RT_ERR_OK;    
}   /* end of dal_ca8279_gpon_init */


/* Function Name:
 *      dal_ca8279_gpon_onuState_set
 * Description:
 *      Set ONU State .
 * Input:
 *      onuState: onu state
 * Output:
 *
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 dal_ca8279_gpon_onuState_set(rtk_gpon_onuState_t  onuState)
{
    int32 ret;
    rtk_port_t pon_port;
    ca_port_id_t port_id;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_GPON), "onuState=%d",onuState);

    /* check Init status */
    RT_INIT_CHK(gpon_init);

    /* parameter check */
    RT_PARAM_CHK((GPON_STATE_END <=onuState), RT_ERR_INPUT);

    /* function body */
    pon_port = HAL_GET_PON_PORT();
    port_id = CA_PORT_ID(CA_PORT_TYPE_EPON,pon_port);
    switch (onuState) 
    {
        case RTK_GPONMAC_INIT_STATE_O1: //treat O1 as active PON-MAC
            ca_gpon_onu_set(0, 0, 1);
            break;
        case RTK_GPONMAC_INIT_STATE_O7: //treat O7 as de-active PON-MAC
            ca_gpon_onu_set(0, 0, 0);
            break;
        default :
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
}   /* end of   dal_rtl9603d_gpon_onuState_set */


/* Function Name:
 *      dal_ca8279_gpon_onuState_get
 * Description:
 *      Get ONU State
 * Input:
 *
 * Output:
 *      pOnuState: point for get onu state
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 * Note:
 */
int32 dal_ca8279_gpon_onuState_get(rtk_gpon_onuState_t  *pOnuState)
{
    int32 ret;
    rtk_port_t pon_port;
    ca_port_id_t port_id;
    ca_gpon_port_onu_states_t caGponState;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_GPON), "");

    /* check Init status */
    RT_INIT_CHK(gpon_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pOnuState), RT_ERR_NULL_POINTER);

    /* function body */
    pon_port = HAL_GET_PON_PORT();
    port_id = CA_PORT_ID(CA_PORT_TYPE_GPON,pon_port);
    if((ret = ca_gpon_port_onu_state_get(0, port_id, &caGponState))!=CA_E_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_GPON), "return failed ret value = %x",ret);
        return RT_ERR_FAILED;
    }
	
	switch(caGponState)
    {
        case CA_NGP2_ONU_ACT_STATE_O1_1:	//fallthrough
        case CA_NGP2_ONU_ACT_STATE_O1_2:
            *pOnuState = GPON_STATE_O1;
            break;
        case CA_NGP2_ONU_ACT_STATE_O2_3:
            *pOnuState = GPON_STATE_O3;
            break;
        case CA_NGP2_ONU_ACT_STATE_O4:
            *pOnuState = GPON_STATE_O4;
            break;
        case CA_NGP2_ONU_ACT_STATE_O5_1:	//fallthrough
        case CA_NGP2_ONU_ACT_STATE_O5_2:
            *pOnuState = GPON_STATE_O5;
            break;
        case CA_NGP2_ONU_ACT_STATE_O6:
            *pOnuState = GPON_STATE_O6;
            break;
        case CA_NGP2_ONU_ACT_STATE_O7:
            *pOnuState = GPON_STATE_O7;
            break;
        default:
            ret = RT_ERR_OUT_OF_RANGE;
            break;
    }

    if(ret!=RT_ERR_OK){
        RT_LOG(LOG_DEBUG, (MOD_DAL|MOD_GPON), "return failed ret value = %x",ret);
        return RT_ERR_FAILED;
    }
	
    return RT_ERR_OK;
}   /* end of   dal_ca8279_gpon_onuState_get */
