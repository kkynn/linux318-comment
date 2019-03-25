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
 * $Revision: 51694 $
 * $Date: 2014-09-24 20:58:33 +0800 (Wed, 24 Sep 2014) $
 *
 * Purpose : Definition of TIME API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) IEEE 1588
 *
 */


/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/port.h>
#include <dal/apollomp/dal_apollomp.h>
#include <rtk/time.h>
#include <dal/apollomp/dal_apollomp_time.h>


/*
 * Data Declaration
 */
 static uint32    time_init = {INIT_NOT_COMPLETED};
 static void timeA_sub_timeB(rtk_time_timeStamp_t timeA, rtk_time_timeStamp_t timeB, rtk_time_ptpRefSign_t *pSign, rtk_time_timeStamp_t *pTimeC);
 static int32 calc_t_rcv_i(rtk_time_timeStamp_t timeStamp, rtk_time_timeStamp_t *pT_rcv_i);

/*
 * Macro Declaration
 */

/*
 * Function Declaration
 */

/* Module Name : TIME */

/* Function Name:
 *      dal_apollomp_time_transparentPort_set
 * Description:
 *      Set transparent ports to the specified device.
 * Input:
 *      port   - ports
 *      enable - enable status
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
int32
dal_apollomp_time_portTransparentEnable_set(rtk_port_t port, rtk_enable_t enable)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_INPUT);
    RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_INPUT);

    val = (uint32)enable;
    if ((ret = reg_array_field_write(APOLLOMP_PTP_TRANSPARENT_CFGr, port, REG_ARRAY_INDEX_NONE, APOLLOMP_TRANSPARENT_PORTf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_portTransparentEnable_set */

/* Function Name:
 *      dal_apollomp_time_transparentPort_get
 * Description:
 *      Get transparent ports to the specified device.
 * Input:
 *      port - ports
 * Output:
 *      pEnable - enable status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - Pointer enable point to NULL.
 * Note:
 *      None
 */
int32
dal_apollomp_time_portTransparentEnable_get(rtk_port_t port, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(APOLLOMP_PTP_TRANSPARENT_CFGr, port, REG_ARRAY_INDEX_NONE, APOLLOMP_TRANSPARENT_PORTf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    *pEnable = (rtk_enable_t)val;

    return RT_ERR_OK;

} /* end of dal_apollomp_time_portTransparentEnable_get */

/* Function Name:
 *      dal_apollomp_time_init
 * Description:
 *      Initialize Time module.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Must initialize Time module before calling any Time APIs.
 */
int32
dal_apollomp_time_init(void)
{
    int32   ret;
    uint32  port;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    time_init = INIT_COMPLETED;

    /* disable PTP */
    HAL_SCAN_ALL_PORT(port)
    {
        if ((ret = dal_apollomp_time_portPtpEnable_set(port, DISABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
            time_init = INIT_NOT_COMPLETED;
            return ret;
        }
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_init */

/* Function Name:
 *      dal_apollomp_time_portPtpEnable_get
 * Description:
 *      Get PTP status of the specified port.
 * Input:
 *      port    - port id
 * Output:
 *      pEnable - status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_INPUT        - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_apollomp_time_portPtpEnable_get(rtk_port_t port, rtk_enable_t *pEnable)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(APOLLOMP_PTP_P_ENr, port, REG_ARRAY_INDEX_NONE, APOLLOMP_PTP_ENf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    *pEnable = (rtk_enable_t)val;

    return RT_ERR_OK;
} /* end of dal_apollomp_time_portPtpEnable_get */

/* Function Name:
 *      dal_apollomp_time_portPtpEnable_set
 * Description:
 *      Set PTP status of the specified port.
 * Input:
 *      port   - port id
 *      enable - status
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_PORT_ID      - Invalid port id.
 *      RT_ERR_INPUT        - Invalid input parameters.
  * Note:
 *      None
 */
int32
dal_apollomp_time_portPtpEnable_set(rtk_port_t port, rtk_enable_t enable)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_INPUT);
    RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_INPUT);

    val = (uint32)enable;
    if ((ret = reg_array_field_write(APOLLOMP_PTP_P_ENr, port, REG_ARRAY_INDEX_NONE, APOLLOMP_PTP_ENf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_portPtpEnable_set */

/* Function Name:
 *      dal_apollomp_time_curTime_get
 * Description:
 *      Get the current time.
 * Input:
 *      None
 * Output:
 *      pTimeStamp - pointer buffer of the current time
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - Pointer pTimeStamp point to NULL.
 * Note:
 *      None
 */
int32
dal_apollomp_time_curTime_get(rtk_time_timeStamp_t *pTimeStamp)
{
    int32   ret;
    uint32  tmp[2];

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pTimeStamp), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(APOLLOMP_PTP_TIME_SECr, APOLLOMP_SEC_31_0f, &tmp[0])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }

    if ((ret = reg_field_read(APOLLOMP_PTP_TIME_SECr, APOLLOMP_SEC_47_32f, &tmp[1])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }
    pTimeStamp->sec = tmp[1];
    pTimeStamp->sec = pTimeStamp->sec << 32;
    pTimeStamp->sec |= tmp[0];

    if ((ret = reg_field_read(APOLLOMP_PTP_TIME_NSECr, APOLLOMP_NSEC_UNITf, &tmp[0])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }
    pTimeStamp->nsec = tmp[0] << 3;

    return RT_ERR_OK;
} /* end of dal_apollomp_time_curTime_get */

/* Function Name:
 *      dal_apollomp_time_curTime_latch
 * Description:
 *      Latch the current time.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
int32
dal_apollomp_time_curTime_latch(void)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    val = TRUE;
    if ((ret = reg_field_write(APOLLOMP_PTP_TIME_CTRLr, APOLLOMP_PTP_TIME_LATCHf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_curTime_latch */


/* Function Name:
 *      dal_apollomp_time_refTime_get
 * Description:
 *      Get the reference time.
 * Input:
 *      None
 * Output:
 *      pSign      - pointer buffer of sign
 *      pTimeStamp - pointer buffer of the reference time
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - Pointer pTimeStamp/pSign point to NULL.
 * Note:
 *      None
 */
int32
dal_apollomp_time_refTime_get(uint32 *pSign, rtk_time_timeStamp_t *pTimeStamp)
{
    int32   ret;
    uint32  tmp[2];

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pSign), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pTimeStamp), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(APOLLOMP_PTP_TIME_OFFSET_SECr, APOLLOMP_SEC_31_0f, &tmp[0])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }

    if ((ret = reg_field_read(APOLLOMP_PTP_TIME_OFFSET_SECr, APOLLOMP_SEC_47_32f, &tmp[1])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }
    pTimeStamp->sec = tmp[1] & 0x0000ffff;
    pTimeStamp->sec = (uint64)pTimeStamp->sec << 32;
    pTimeStamp->sec |= tmp[0];

    if ((ret = reg_field_read(APOLLOMP_PTP_TIME_OFFSET_8NSECr, APOLLOMP_NSEC_UNITf, &tmp[0])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }
    pTimeStamp->nsec = tmp[0];

    if(pTimeStamp->sec & 0x800000000000LL)
    {
        *pSign = PTP_REF_SIGN_NEGATIVE;
        if(pTimeStamp->nsec == 0)
            pTimeStamp->sec = (~(pTimeStamp->sec -1))& 0x0000ffffffffffffLL;
        else
        {
            pTimeStamp->sec = (~(pTimeStamp->sec -1) -1)  & 0x0000ffffffffffffLL;
            pTimeStamp->nsec = (1000000000/8 - pTimeStamp->nsec) << 3;
        }
    }
    else
    {
        *pSign = PTP_REF_SIGN_POSTIVE;
        pTimeStamp->nsec = tmp[0] << 3;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_refTime_get */

/* Function Name:
 *      dal_apollomp_time_refTime_set
 * Description:
 *      Set the reference time.
 * Input:
 *      sign      - significant
 *      timeStamp - reference timestamp value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      sign=0 for positive adjustment, sign=1 for negative adjustment.
 */
int32
dal_apollomp_time_refTime_set(uint32 sign, rtk_time_timeStamp_t timeStamp)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((PTP_REF_SIGN_END <= sign), RT_ERR_INPUT);
    RT_PARAM_CHK((APOLLOMP_TIME_REFTIME_SEC_MAX < timeStamp.sec), RT_ERR_INPUT);
    RT_PARAM_CHK((1000000000 <= timeStamp.nsec), RT_ERR_INPUT);

    if(sign == PTP_REF_SIGN_POSTIVE)
    {
        val = (uint32)(timeStamp.sec & 0xFFFFFFFF);
    }
    else
    {
        if(timeStamp.nsec != 0)
            val = (uint32)((~(timeStamp.sec + 1)+1) & 0xFFFFFFFFUL);
        else
            val = (uint32)((~timeStamp.sec +1) & 0xFFFFFFFFUL);
    }
    if ((ret = reg_field_write(APOLLOMP_PTP_TIME_OFFSET_SECr, APOLLOMP_SEC_31_0f, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    if(sign == PTP_REF_SIGN_POSTIVE)
    {
        val = (uint32)((timeStamp.sec >> 32) & 0xFFFF);
    }
    else
    {
        if(timeStamp.nsec != 0)
            val = (uint32)(((~(timeStamp.sec + 1)+1) >> 32) & 0xFFFF);
        else
            val = (uint32)(((~timeStamp.sec +1) >> 32) & 0xFFFF);
    }
    if ((ret = reg_field_write(APOLLOMP_PTP_TIME_OFFSET_SECr, APOLLOMP_SEC_47_32f, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    if(sign == PTP_REF_SIGN_POSTIVE)
    {
        val = (uint32)((timeStamp.nsec >> 3) & APOLLOMP_TIME_REFTIME_NSEC_MAX) ;
    }
    else
    {
        if(timeStamp.nsec != 0)
            val = (uint32)(((1000000000 - timeStamp.nsec) >> 3) & APOLLOMP_TIME_REFTIME_NSEC_MAX);
        else
            val = 0;
    }
    if ((ret = reg_field_write(APOLLOMP_PTP_TIME_OFFSET_8NSECr, APOLLOMP_NSEC_UNITf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    val = TRUE;
    if ((ret = reg_field_write(APOLLOMP_PTP_TIME_CTRLr, APOLLOMP_CMDf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_refTime_set */

/* Function Name:
 *      dal_apollomp_time_frequency_set
 * Description:
 *      Set frequency of PTP system time.
 * Input:
 *      freq - reference timestamp value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_apollomp_time_frequency_set(uint32 freq)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((APOLLOMP_TIME_FREQUENCY_MAX < freq), RT_ERR_INPUT);

    val = freq;
    if ((ret = reg_field_write(APOLLOMP_PTP_TIME_FREQr, APOLLOMP_FREQf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_frequency_set */

/* Function Name:
 *      dal_apollomp_time_frequency_get
 * Description:
 *      Set frequency of PTP system time.
 * Input:
 *      freq - reference timestamp value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_apollomp_time_frequency_get(uint32 *freq)
{
    int32   ret;
    uint32  val;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == freq), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(APOLLOMP_PTP_TIME_FREQr, APOLLOMP_FREQf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }
    *freq = val;

    return RT_ERR_OK;
} /* end of dal_apollomp_time_frequency_get */

/* Function Name:
 *      dal_apollomp_time_ptpIgrMsgAction_set
 * Description:
 *      Set ingress action configuration for PTP message.
 * Input:
 *      type          - PTP message type
 *      igr_action    - ingress action.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 *      RT_ERR_NOT_ALLOWED 		  - Invalid action.
 *      RT_ERR_INPUT 			      - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_apollomp_time_ptpIgrMsgAction_set(rtk_time_ptpMsgType_t type, rtk_time_ptpIgrMsg_action_t igr_action)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((PTP_MSG_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((PTP_IGR_ACTION_END <= igr_action), RT_ERR_INPUT);

    val = (uint32)igr_action;
    if ((ret = reg_array_field_write(APOLLOMP_PTP_IGR_MSG_ACTr, REG_ARRAY_INDEX_NONE, type, APOLLOMP_ACTf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_ptpIgrMsgAction_set */

/* Function Name:
 *      dal_apollomp_time_ptpIgrMsgAction_get
 * Description:
 *      Get ingress action configuration for PTP message.
 * Input:
 *      type          - PTP message type
 * Output:
 *      igr_action    - ingress action.
 * Return:
 *      RT_ERR_OK            - OK
 *      RT_ERR_FAILED        - Failed
 *      RT_ERR_NULL_POINTER  - Pointer igr_action point to NULL.
 * Note:
 *      None
 */
int32
dal_apollomp_time_ptpIgrMsgAction_get(rtk_time_ptpMsgType_t type, rtk_time_ptpIgrMsg_action_t *igr_action)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((PTP_MSG_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == igr_action), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(APOLLOMP_PTP_IGR_MSG_ACTr, REG_ARRAY_INDEX_NONE, type, APOLLOMP_ACTf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    *igr_action = (rtk_time_ptpIgrMsg_action_t)val;

    return RT_ERR_OK;
} /* end of dal_apollomp_time_ptpIgrMsgAction_get */

/* Function Name:
 *      dal_apollomp_time_ptpEgrMsgAction_set
 * Description:
 *      Set egress action configuration for PTP message.
 * Input:
 *      type          - PTP message type
 *      egr_action    - egress action.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 *      RT_ERR_NOT_ALLOWED 		  - Invalid action.
 *      RT_ERR_INPUT 			      - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_apollomp_time_ptpEgrMsgAction_set(rtk_time_ptpMsgType_t type, rtk_time_ptpEgrMsg_action_t egr_action)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((PTP_MSG_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((PTP_EGR_ACTION_END <= egr_action), RT_ERR_INPUT);

    val = (uint32)egr_action;
    if ((ret = reg_array_field_write(APOLLOMP_PTP_EGR_MSG_ACTr, REG_ARRAY_INDEX_NONE, type, APOLLOMP_ACTf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_ptpEgrMsgAction_set */

/* Function Name:
 *      dal_apollomp_time_ptpEgrMsgAction_get
 * Description:
 *      Get egress action configuration for PTP message.
 * Input:
 *      type          - PTP message type
 * Output:
 *      egr_action    - egress action.
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_NULL_POINTER - Pointer egr_action point to NULL.
 * Note:
 *      None
 */
int32
dal_apollomp_time_ptpEgrMsgAction_get(rtk_time_ptpMsgType_t type, rtk_time_ptpEgrMsg_action_t *egr_action)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((PTP_MSG_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == egr_action), RT_ERR_NULL_POINTER);

    if ((ret = reg_array_field_read(APOLLOMP_PTP_EGR_MSG_ACTr, REG_ARRAY_INDEX_NONE, type, APOLLOMP_ACTf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }
    *egr_action = (rtk_time_ptpEgrMsg_action_t)val;

    return RT_ERR_OK;
} /* end of dal_apollomp_time_ptpEgrMsgAction_get */

/* Function Name:
 *      dal_apollomp_time_meanPathDelay_set
 * Description:
 *      Set dal_apollomp_time_meanPathDelay_set of PTP system time.
 * Input:
 *      delay - mean path delay value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
int32
dal_apollomp_time_meanPathDelay_set(uint32 delay)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((APOLLOMP_TIME_MEANPATHDELAY_MAX < delay), RT_ERR_INPUT);

    val = delay;
    if ((ret = reg_field_write(APOLLOMP_PTP_MEANPATH_DELAYr, APOLLOMP_DELAYf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_meanPathDelay_set */

/* Function Name:
 *      dal_apollomp_time_meanPathDelay_get
 * Description:
 *      Get dal_apollomp_time_meanPathDelay_get of PTP system time.
 * Input:
 *      delay    - mean path delay.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_NULL_POINTER - - Pointer delay point to NULL.
 * Note:
 *      None
 */
int32
dal_apollomp_time_meanPathDelay_get(uint32 *delay)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == delay), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(APOLLOMP_PTP_MEANPATH_DELAYr, APOLLOMP_DELAYf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }
    *delay = val;

    return RT_ERR_OK;
} /* end of dal_apollomp_time_meanPathDelay_get */

/* Function Name:
 *      dal_apollomp_time_rxTime_set
 * Description:
 *      Configure user RX timestamp.
 * Input:
 *      timeStamp - RX timestamp value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 * Note:
 *
 */
int32
dal_apollomp_time_rxTime_set(rtk_time_timeStamp_t timeStamp)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((APOLLOMP_TIME_RXTIME_SEC_MAX <= timeStamp.sec), RT_ERR_INPUT);
    RT_PARAM_CHK((APOLLOMP_TIME_RXTIME_NSEC_MAX <= timeStamp.nsec), RT_ERR_INPUT);

    val = (uint32)(timeStamp.sec & 0x7);
    if ((ret = reg_field_write(APOLLOMP_PTP_RX_TIMEr, APOLLOMP_SEC_2_0f, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    val = timeStamp.nsec >> 3;
    if ((ret = reg_field_write(APOLLOMP_PTP_RX_TIMEr, APOLLOMP_NSEC_UNITf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PTP), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_rxTime_set */

/* Function Name:
 *      dal_apollomp_time_rxTime_get
 * Description:
 *      Get RX timestamp.
 * Input:
 *      None
 * Output:
 *      pTimeStamp - pointer buffer of the RX time
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 *      RT_ERR_NULL_POINTER     - Pointer pTimeStamp point to NULL.
 * Note:
 *
 */
int32
dal_apollomp_time_rxTime_get(rtk_time_timeStamp_t *pTimeStamp)
{
    int32   ret;
    uint32  val;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pTimeStamp), RT_ERR_NULL_POINTER);

    if ((ret = reg_field_read(APOLLOMP_PTP_RX_TIMEr, APOLLOMP_SEC_2_0f, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }
    pTimeStamp->sec = val;

    if ((ret = reg_field_read(APOLLOMP_PTP_RX_TIMEr, APOLLOMP_NSEC_UNITf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL | MOD_PTP), "");
        return ret;
    }
    pTimeStamp->nsec = val << 3;

    return RT_ERR_OK;
} /* end of dal_apollomp_time_rxTime_get */


/* Function Name:
 *      timeA_sub_timeB
 * Description:
 *      timeStamp sub operation
 * Input:
 *      timeA - time A
 *      timeB - time B
 * Output:
 *      pSign  - the sign of result
 *      pTimeC - the timeStamp of result
 * Return:
 *      none
 * Note:
 *
 */
static void
timeA_sub_timeB(rtk_time_timeStamp_t timeA, rtk_time_timeStamp_t timeB, rtk_time_ptpRefSign_t *pSign, rtk_time_timeStamp_t *pTimeC)
{
    if((timeA.sec >= timeB.sec) && (timeA.nsec >= timeB.nsec))
    {
        *pSign = PTP_REF_SIGN_POSTIVE;
        pTimeC->sec = timeA.sec - timeB.sec;
        pTimeC->nsec = timeA.nsec - timeB.nsec;
    }
    else
    if((timeA.sec > timeB.sec) && (timeA.nsec < timeB.nsec))
    {
        *pSign = PTP_REF_SIGN_POSTIVE;
        pTimeC->sec = timeA.sec - 1 - timeB.sec;
        pTimeC->nsec = timeA.nsec + (10^9) - timeB.nsec;
    }
    else
    if((timeA.sec < timeB.sec) && (timeA.nsec > timeB.nsec))
    {
        *pSign = PTP_REF_SIGN_NEGATIVE;
        pTimeC->sec = timeB.sec - 1 - timeA.sec;
        pTimeC->nsec = timeB.nsec + (10^9) - timeA.nsec;
    }
    else
    {
        *pSign = PTP_REF_SIGN_NEGATIVE;
        pTimeC->sec = timeB.sec - timeA.sec;
        pTimeC->nsec = timeB.nsec - timeA.nsec;
    }

}

/* Function Name:
 *      calc_t_rcv_i
 * Description:
 *      calc Trecvi by Tstamp.
 * Input:
 *      timeStamp - PON TOD timestamp value
 * Output:
 *      pT_rcv_i  - the pointer of Trecvi
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 * Note:
 *
 */
static int32
calc_t_rcv_i(rtk_time_timeStamp_t timeStamp, rtk_time_timeStamp_t *pT_rcv_i)
{
    uint32  val, multi, intra, eqd, delta_i;
    rtk_time_timeStamp_t delta_time;
    rtk_time_ptpRefSign_t sign;
    int32 ret;

    /*calc delta-i: get eqd and multiple n1490/(n1310+1490)*/
    if((ret = reg_read(APOLLOMP_GPON_GTC_US_EQDr,&val))!=RT_ERR_OK)
	{
		RT_LOG(LOG_DEBUG, (MOD_DAL | MOD_PTP), "return failed ret value = %x",ret);
		return RT_ERR_FAILED;
	}
    if((ret = reg_field_get(APOLLOMP_GPON_GTC_US_EQDr,APOLLOMP_EQD1_MULTFRAMEf,&multi,&val))!=RT_ERR_OK)
	{
		RT_LOG(LOG_DEBUG, (MOD_DAL | MOD_PTP), "return failed ret value = %x",ret);
		return RT_ERR_FAILED;
	}
	if((ret = reg_field_get(APOLLOMP_GPON_GTC_US_EQDr,APOLLOMP_EQD1_INFRAMEf,&intra,&val))!=RT_ERR_OK)
	{
		RT_LOG(LOG_DEBUG, (MOD_DAL | MOD_PTP), "return failed ret value = %x",ret);
		return RT_ERR_FAILED;
	}

    eqd = multi*(19440*8) + intra;
    /*n1490/(n1310+1490)=0.500085*/
    delta_i = eqd*500085/1000000;

    /*bits to ns*/
    delta_time.sec = (delta_i*125*1000/(19880*8))/(10^9);
    delta_time.nsec = (delta_i*125*1000/(19880*8))%(10^9);

    /*t_rcv_i = t_stamp - delta_i*/
    timeA_sub_timeB(timeStamp, delta_time, &sign, pT_rcv_i);
    if(sign == PTP_REF_SIGN_NEGATIVE)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_apollomp_time_ponTodTime_set
 * Description:
 *      Configure PON TOD timestamp.
 * Input:
 *      timeStamp - PON TOD timestamp value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 * Note:
 *
 */
int32
dal_apollomp_time_ponTodTime_set(rtk_pon_tod_t ponTod)
{
    int32   ret;
    uint32  cur_superframe;
    rtk_time_timeStamp_t cur_time, t_rcv_i, adj_time, olt_time, offset_time;
    rtk_time_ptpRefSign_t sign;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((APOLLOMP_TIME_PONTOD_SEC_MAX < ponTod.timeStamp.sec), RT_ERR_INPUT);
    RT_PARAM_CHK((APOLLOMP_TIME_PONTOD_NSEC_MAX < ponTod.timeStamp.nsec), RT_ERR_INPUT);

    /*get gpon current time*/
    if((ret = dal_apollomp_time_curTime_get(&cur_time))!=RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL | MOD_PTP), "return failed ret value = %x",ret);
        return ret;
    }

    /*get gpon superframe counter*/
    if((ret = reg_field_read(APOLLOMP_GPON_GTC_DS_SUPERFRAME_CNTr,APOLLOMP_SF_CNTRf,&cur_superframe))!=RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL | MOD_PTP), "return failed ret value = %x",ret);
        return ret;
    }

    /* calc Trecvi from Tstamp*/
    if((ret = calc_t_rcv_i(ponTod.timeStamp, &t_rcv_i))!=RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL | MOD_PTP), "return failed ret value = %x",ret);
        return ret;
    }

    /*olt_time = t_rcv_i - (startPoint-cur_superframe)*125*000 */
    adj_time.sec = (ponTod.startPoint.superFrame - cur_superframe)/8000;
    adj_time.nsec = ((ponTod.startPoint.superFrame - cur_superframe)%8000)*125*1000;
    timeA_sub_timeB(t_rcv_i, adj_time, &sign, &olt_time);
    if(sign == PTP_REF_SIGN_NEGATIVE)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL | MOD_PTP), "");
        return RT_ERR_FAILED;
    }

    /*offset = delta(olt_time, cur_time)*/
    timeA_sub_timeB(olt_time, cur_time, &sign, &offset_time);

    /*set offset*/
    if((ret = dal_apollomp_time_refTime_set(sign, offset_time))!=RT_ERR_OK)
    {
        RT_LOG(LOG_DEBUG, (MOD_DAL | MOD_PTP), "return failed ret value = %x",ret);
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_apollomp_time_ponTodTime_set */


/* Function Name:
 *      dal_apollomp_time_ponTodTime_get
 * Description:
 *      Get PON TOD timestamp.
 * Input:
 *      None
 * Output:
 *      pTimeStamp - pointer buffer of the PON TOD time
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 *      RT_ERR_NULL_POINTER     - Pointer pTimeStamp point to NULL.
 * Note:
 *
 */
int32
dal_apollomp_time_ponTodTime_get(rtk_pon_tod_t *pPonTod)
{

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pPonTod), RT_ERR_NULL_POINTER);

    memset(pPonTod, 0, sizeof(rtk_pon_tod_t));

    return RT_ERR_OK;
} /* end of dal_apollomp_time_ponTodTime_get */


/* Function Name:
 *      dal_apollomp_time_todEnable_get
 * Description:
 *      Get pon tod status.
 * Input:
 *      None.
 * Output:
 *      pEnable - status
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT        - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_apollomp_time_todEnable_get(rtk_enable_t *pEnable)
{

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    *pEnable = ENABLED;

    return RT_ERR_OK;
} /* end of dal_apollomp_time_todEnable_get */

/* Function Name:
 *      dal_apollomp_time_todEnable_set
 * Description:
 *      Set pon tod status.
 * Input:
 *      enable - status
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_INPUT        - Invalid input parameters.
  * Note:
 *      None
 */
int32
dal_apollomp_time_todEnable_set(rtk_enable_t enable)
{

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PTP),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(time_init);

    /* parameter check */
    RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_INPUT);

    return RT_ERR_OK;
} /* end of dal_apollomp_time_todEnable_set */
