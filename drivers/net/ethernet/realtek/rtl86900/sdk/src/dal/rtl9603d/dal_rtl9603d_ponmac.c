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
#include <rtk/gpio.h>
#include <rtk/pbo.h>
#include <dal/rtl9603d/dal_rtl9603d.h>
#include <dal/rtl9603d/dal_rtl9603d_ponmac.h>
#include <dal/rtl9603d/dal_rtl9603d_gpon.h>
#include <dal/rtl9603d/dal_rtl9603d_pbo.h>
#include <dal/rtl9603d/dal_rtl9603d_flowctrl.h>
#include <dal/rtl9603d/dal_rtl9603d_switch.h>
#include <dal/rtl9603d/dal_rtl9603d_gpio.h>
#include <osal/time.h>

/*
 * Symbol Definition
 */

static uint32    ponmac_init = INIT_NOT_COMPLETED;
static rtk_ponmac_mode_t ponMode = PONMAC_MODE_GPON;

/*
 * Macro Declaration
 */


static int32 _ponmac_physicalQueueId_get(uint32 schedulerId, uint32 logicalQId, uint32 *pPhysicalQid)
{

    *pPhysicalQid = RTL9603D_TCONT_QUEUE_MAX *(schedulerId) + logicalQId;

#if 0
    if(PONMAC_MODE_GPON == ponMode)
        *pPhysicalQid = RTL9603D_TCONT_QUEUE_MAX *(schedulerId) + logicalQId;
    else
        *pPhysicalQid = RTL9603D_MAX_EPON_MAX_QUEUE_LLID *(schedulerId) + logicalQId;
#endif
    return RT_ERR_OK;
}


/* Function Name:
 *      rtl9603d_ponMacQueueDrainOut_set
 * Description:
 *      Set PON mac per queue draint out state
 * Input:
 *      qid         - Specify the draint out Queue id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
int32 rtl9603d_ponMacQueueDrainOutState_set(uint32 qid)
{
    int32 ret;
    uint32  data;
    uint32  tmpData;
    uint32  busy;
    uint32  i;


    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);

    data = 0;

    /* queue drain out */
    tmpData = 1;
    if ((ret = reg_field_set(RTL9603D_DRN_CMDr, RTL9603D_CFG_DRN_QUEUE_MODEf, &tmpData,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_set(RTL9603D_DRN_CMDr, RTL9603D_CFG_DRN_IDXf, &qid,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    tmpData = 1;
    if ((ret = reg_field_set(RTL9603D_DRN_CMDr, RTL9603D_DRN_PSf, &tmpData,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /* drain out */
    if ((ret = reg_write(RTL9603D_DRN_CMDr,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /* check busy bit */
    i = 0;
    do
    {
        if ((ret = reg_field_read(RTL9603D_DRN_CMDr, RTL9603D_DRN_FLGf, &busy)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }

        i++;
    } while (busy && (i < 10000));

    if(i >= 10000)
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}



/* Function Name:
 *      _rtl9603d_ponMacQueueDrainOutState_get
 * Description:
 *      Get PON mac queue draint out state
 * Input:
 *      None
 * Output:
 *      pState - pon queue draint out status
 * Return:
 *      RT_ERR_OK
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacQueueDrainOutState_get(rtl9603d_ponmac_draintOutState_t *pState)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((pState  == NULL), RT_ERR_NULL_POINTER);

    /* set to register */
    if ((ret = reg_field_read(RTL9603D_DRN_CMDr, RTL9603D_DRN_FLGf, pState)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}


/* Function Name:
 *      _rtl9603d_ponMacTcontDrainOutState_set
 * Description:
 *      Set PON mac per T-cont draint out state
 * Input:
 *      tcont       - Specify the draint out T-cont id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_GPON_EXCEED_MAX_TCONT
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacTcontDrainOutState_set(uint32 tcont)
{
    int32 ret;
    uint32 data;
    uint32 tmpData;
    uint32 i;
    uint32 busy;

    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_GPON_TCONT() <= tcont), RT_ERR_GPON_EXCEED_MAX_TCONT);

    data = 0;

    /* set to register */
    tmpData = 0;
    if ((ret = reg_field_set(RTL9603D_DRN_CMDr, RTL9603D_CFG_DRN_QUEUE_MODEf, &tmpData,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_set(RTL9603D_DRN_CMDr, RTL9603D_CFG_DRN_IDXf, &tcont,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    tmpData = 1;
    if ((ret = reg_field_set(RTL9603D_DRN_CMDr, RTL9603D_DRN_PSf, &tmpData,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /* set to register */
    if ((ret = reg_write(RTL9603D_DRN_CMDr,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /* check busy bit */
    i = 0;
    do
    {
        if ((ret = reg_field_read(RTL9603D_DRN_CMDr, RTL9603D_DRN_FLGf, &busy)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }

        i++;
    } while (busy && (i < 200000));
    
    
    if(i >= 200000)
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}



/* Function Name:
 *      _rtl9603d_ponMacTcontDrainOutState_get
 * Description:
 *      Get PON mac T-cont draint out state
 * Input:
 *      None
 * Output:
 *      pState - T-cont draint out status
 * Return:
 *      RT_ERR_OK
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacTcontDrainOutState_get(rtl9603d_ponmac_draintOutState_t *pState)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((pState  == NULL), RT_ERR_NULL_POINTER);


    /* set to register */
    if ((ret = reg_field_read(RTL9603D_DRN_CMDr, RTL9603D_DRN_FLGf, pState)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}



/* Function Name:
 *      _rtl9603d_ponMacSidToQueueMap_set
 * Description:
 *      Set PON mac session id to queue maping
 * Input:
 *      sid         - Specify the session id
 *      qid         - Specify the mapping queue id id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacSidToQueueMap_set(uint32 sid, uint32 qid)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);
    RT_PARAM_CHK((HAL_CLASSIFY_SID_NUM() <= sid),RT_ERR_INPUT);

    /* set to register */
    if ((ret = reg_array_field_write(RTL9603D_PON_SID2QIDr,REG_ARRAY_INDEX_NONE, sid, RTL9603D_CFG_SID2QIDf, &qid)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
#if 0
    /*for epon mode must mapping sid 64~127 to queue 0~63*/
    if(PONMAC_MODE_EPON == ponMode)
    {
        if(sid < 64)
        {
            sid = sid + 64;
            if ((ret = reg_array_field_write(RTL9603D_PON_SID2QIDr,REG_ARRAY_INDEX_NONE, sid, RTL9603D_CFG_SID2QIDf, &qid)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        }
    }
#endif
    
    return RT_ERR_OK;
}

/* Function Name:
 *      _rtl9603d_ponMacSidToQueueMap_get
 * Description:
 *      Get PON mac session id to queue maping
 * Input:
 *      tcont       - Specify the draint out T-cont id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacSidToQueueMap_get(uint32 sid, uint32 *pQid)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((HAL_CLASSIFY_SID_NUM() <= sid),RT_ERR_INPUT);
    RT_PARAM_CHK((pQid  == NULL), RT_ERR_NULL_POINTER);

    /* set to register */
    if ((ret = reg_array_field_read(RTL9603D_PON_SID2QIDr,REG_ARRAY_INDEX_NONE, sid, RTL9603D_CFG_SID2QIDf, pQid)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}



/* Function Name:
 *      _rtl9603d_ponMacCirRate_set
 * Description:
 *      Per queue set PON mac CIR rate
 * Input:
 *      qid         - - Specify the queue id
 *      rate        - Specify the CIR rate unit 64kbps
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacCirRate_set(uint32 qid, uint32 rate)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);
    RT_PARAM_CHK((HAL_PONMAC_PIR_CIR_RATE_MAX() < rate),RT_ERR_INPUT);

    /* set to register */
    if ((ret = reg_array_field_write(RTL9603D_PON_QID_CIR_RATEr,REG_ARRAY_INDEX_NONE, qid, RTL9603D_RATEf, &rate)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}




/* Function Name:
 *      _rtl9603d_ponMacCirRate_get
 * Description:
 *      Per queue set PON mac CIR rate
 * Input:
 *      qid         - - Specify the queue id
 * Output:
 *      pRate       - Specify the CIR rate unit 64kbps
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_NULL_POINTER
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacCirRate_get(uint32 qid, uint32 *pRate)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);
    RT_PARAM_CHK((pRate  == NULL), RT_ERR_NULL_POINTER);

    /* set to register */
    if ((ret = reg_array_field_read(RTL9603D_PON_QID_CIR_RATEr,REG_ARRAY_INDEX_NONE, qid, RTL9603D_RATEf, pRate)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}



/* Function Name:
 *      _rtl9603d_ponMacPirRate_set
 * Description:
 *      Per queue set PON mac PIR rate
 * Input:
 *      qid         - Specify the queue id
 *      rate        - Specify the CIR rate unit 64kbps
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacPirRate_set(uint32 qid, uint32 rate)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);
    RT_PARAM_CHK((HAL_PONMAC_PIR_CIR_RATE_MAX() < rate),RT_ERR_INPUT);

    /* set to register */
    if ((ret = reg_array_field_write(RTL9603D_PON_QID_PIR_RATEr,REG_ARRAY_INDEX_NONE, qid, RTL9603D_RATEf, &rate)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}




/* Function Name:
 *      _rtl9603d_ponMacPirRate_get
 * Description:
 *      Per queue set PON mac PIR rate
 * Input:
 *      qid         - - Specify the queue id
 * Output:
 *      pRate       - Specify the PIR rate unit 64kbps
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_NULL_POINTER
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacPirRate_get(uint32 qid, uint32 *pRate)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);
    RT_PARAM_CHK((pRate  == NULL), RT_ERR_NULL_POINTER);

    /* set to register */
    if ((ret = reg_array_field_read(RTL9603D_PON_QID_PIR_RATEr,REG_ARRAY_INDEX_NONE, qid, RTL9603D_RATEf, pRate)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}





/* Function Name:
 *      _rtl9603d_ponMacScheQmap_set
 * Description:
 *      Per T-CONT/LLID set schedule queue map
 * Input:
 *      tcont       - Specify T-CONT or LLID
 *      map         - Specify schedule queue mapping mask
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacScheQmap_set(uint32 tcont, uint32 map)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK(((HAL_MAX_NUM_OF_GPON_TCONT()) <= tcont), RT_ERR_GPON_EXCEED_MAX_TCONT);

    /* set to register */
    if ((ret = reg_array_field_write(RTL9603D_PON_SCH_QMAPr,REG_ARRAY_INDEX_NONE, tcont, RTL9603D_MAPPING_TBLf, &map)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}



/* Function Name:
 *      _rtl9603d_ponMacScheQmap_get
 * Description:
 *      Per T-CONT/LLID get schedule queue map
 * Input:
 *      tcont       - Specify T-CONT or LLID
 * Output:
 *      pMap        - Specify schedule queue mapping mask
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacScheQmap_get(uint32 tcont, uint32 *pMap)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_GPON_TCONT() <= tcont), RT_ERR_GPON_EXCEED_MAX_TCONT);

    /* set to register */
    if ((ret = reg_array_field_read(RTL9603D_PON_SCH_QMAPr,REG_ARRAY_INDEX_NONE, tcont, RTL9603D_MAPPING_TBLf, pMap)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}




/* Function Name:
 *      _rtl9603d_ponMac_queueType_set
 * Description:
 *      Set queue set queue type for PON MAC
 * Input:
 *      qid     - The queue ID wanted to set (0~127)
 *      type    - queue type  0: strict 1:WFQ
  * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMac_queueType_set(uint32 qid, rtk_qos_queue_type_t type)
{
    int32 ret;
    uint32 wData;

    /* Error Checking */
    RT_PARAM_CHK((QUEUE_TYPE_END <= type), RT_ERR_INPUT);
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);

    if(STRICT_PRIORITY==type)
        wData = 0;
    else
        wData = 1;

    /* set to register */
    if ((ret = reg_array_field_write(RTL9603D_PON_WFQ_TYPEr,REG_ARRAY_INDEX_NONE, qid, RTL9603D_QUEUE_TYPEf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}


/* Function Name:
 *      _rtl9603d_ponMac_queueType_get
 * Description:
 *      Set queue get queue type for PON MAC
 * Input:
 *      qid     - The queue ID wanted to set (0~127)
 *      type    - queue type  0: strict 1:WFQ
  * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_NULL_POINTER
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMac_queueType_get(uint32 qid, rtk_qos_queue_type_t *pType)
{
    int32 ret;
    uint32 rData;
    /* Error Checking */
    RT_PARAM_CHK((pType  == NULL), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);

    /* set to register */
    if ((ret = reg_array_field_read(RTL9603D_PON_WFQ_TYPEr,REG_ARRAY_INDEX_NONE, qid, RTL9603D_QUEUE_TYPEf, &rData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    if(0==rData)
        *pType= STRICT_PRIORITY;
    else
        *pType= WFQ_WRR_PRIORITY;

    return RT_ERR_OK;
}






/* Function Name:
 *      _rtl9603d_ponMac_wfqWeight_set
 * Description:
 *      Set queue set WFQ queue weight for PON MAC
 * Input:
 *      qid     - The queue ID wanted to set (0~127)
 *      weight  - WFQ weight
  * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMac_wfqWeight_set(uint32 qid, uint32 weight)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((1024 <= weight), RT_ERR_INPUT);

    RT_PARAM_CHK((0 == weight), RT_ERR_INPUT);

    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);

    /* set to register */
    if ((ret = reg_array_field_write(RTL9603D_PON_WFQ_WEIGHTr,REG_ARRAY_INDEX_NONE, qid, RTL9603D_WEIGHT_PONf, &weight)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}


/* Function Name:
 *      _rtl9603d_ponMac_wfqWeight_get
 * Description:
 *      Set queue set WFQ queue weight for PON MAC
 * Input:
 *      qid     - The queue ID wanted to set (0~127)
  * Output:
 *      pWeight     - WFQ weight
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_NULL_POINTER
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMac_wfqWeight_get(uint32 qid, uint32 *pWeight)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((pWeight  == NULL), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((HAL_MAX_NUM_OF_PON_QUEUE() <= qid), RT_ERR_QUEUE_ID);

    /* set to register */
    if ((ret = reg_array_field_read(RTL9603D_PON_WFQ_WEIGHTr,REG_ARRAY_INDEX_NONE, qid, RTL9603D_WEIGHT_PONf, pWeight)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}






/* Function Name:
 *      _rtl9603d_ponMacTcontEnable_set
 * Description:
 *      Per T-CONT set schedule enable state
 * Input:
 *      tcont       - Specify T-CONT
 *      enable      - Specify T-CONT schedule enable state
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacTcontEnable_set(uint32 tcont, rtk_enable_t enable)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK(((HAL_MAX_NUM_OF_GPON_TCONT()) <= tcont), RT_ERR_GPON_EXCEED_MAX_TCONT);
    RT_PARAM_CHK((RTK_ENABLE_END <= enable ), RT_ERR_INPUT);

    /* set to register */
    if ((ret = reg_array_field_write(RTL9603D_PON_TCONT_ENr,REG_ARRAY_INDEX_NONE, tcont, RTL9603D_TCONT_ENf, &enable)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}



/* Function Name:
 *      _rtl9603d_ponMacTcontEnable_get
 * Description:
 *      Per T-CONT get schedule enable state
 * Input:
 *      tcont       - Specify T-CONT
 * Output:
 *      pEnable         - T-CONT schedule enable state
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_QUEUE_ID
 * Note:
 *      None
 */
static int32 _rtl9603d_ponMacTcontEnable_get(uint32 tcont, rtk_enable_t *pEnable)
{
    int32 ret;

    /* Error Checking */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_GPON_TCONT() <= tcont), RT_ERR_GPON_EXCEED_MAX_TCONT);
    RT_PARAM_CHK((pEnable  == NULL), RT_ERR_NULL_POINTER);

    /* set to register */
    if ((ret = reg_array_field_read(RTL9603D_PON_TCONT_ENr,REG_ARRAY_INDEX_NONE, tcont, RTL9603D_TCONT_ENf, pEnable)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}



static int32 _ponmac_schedulerQueue_get(uint32 physicalQid, uint32 *pSchedulerId, uint32 *pLogicalQId)
{
    if(PONMAC_MODE_GPON == ponMode)
    {
        *pSchedulerId = physicalQid/HAL_PONMAC_TCONT_QUEUE_MAX();
        *pLogicalQId  = physicalQid%HAL_PONMAC_TCONT_QUEUE_MAX();
    }
    else
    {
        *pSchedulerId = physicalQid/HAL_PONMAC_TCONT_QUEUE_MAX();
        *pLogicalQId  = physicalQid%HAL_PONMAC_TCONT_QUEUE_MAX();
    }

    if(physicalQid == (HAL_MAX_NUM_OF_PON_QUEUE() - 1))
    {
        *pSchedulerId = HAL_MAX_NUM_OF_GPON_TCONT() - 1;
        *pLogicalQId = 0;
        return RT_ERR_OK;
    }

    return RT_ERR_OK;
}

static int32 _rtl9603d_ponMacGponModeV1_set(void)
{
    int32 ret;
    uint32 data;
    rtk_ponmac_queue_t logicalQueue;
    rtk_ponmac_queueCfg_t queueCfg;
    uint32 flowId;
#if !defined(FPGA_DEFINED)
    rtk_pbo_initParam_t initParam;
#endif
    uint32 cnt=0;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    /* mapping all flow to t-cont 15 queue 6, and set SID invalid */
    logicalQueue.schedulerId = 15;
    logicalQueue.queueId     = 6;
    for(flowId=0 ; flowId<HAL_CLASSIFY_SID_NUM()-1 ; flowId++)
    {
        if((ret= dal_rtl9603d_ponmac_flow2Queue_set(flowId, &logicalQueue)) != RT_ERR_OK)
        {
            ponmac_init = INIT_NOT_COMPLETED;
            return ret;
        }

        data = 0x0;
        if ((ret = reg_array_field_write(RTL9603D_PON_SIDVALIDr, REG_ARRAY_INDEX_NONE, flowId, RTL9603D_CFG_SID2VALIDf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
    }

    /* omci flow to queue */
    logicalQueue.schedulerId = RTL9603D_GPON_OMCI_TCONT_ID;
    logicalQueue.queueId = RTL9603D_GPON_OMCI_QUEUE_ID;
    if((ret= dal_rtl9603d_ponmac_flow2Queue_set(RTL9603D_GPON_OMCI_FLOW_ID, &logicalQueue)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* set omci sid valid */
    data = 0x1;
    if ((ret = reg_array_field_write(RTL9603D_PON_SIDVALIDr, REG_ARRAY_INDEX_NONE, RTL9603D_GPON_OMCI_FLOW_ID, RTL9603D_CFG_SID2VALIDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* set CFG_OMCI_SID */
    data = RTL9603D_GPON_OMCI_FLOW_ID;
    if ((ret = reg_field_write(RTL9603D_PON_OMCI_CFGr, RTL9603D_CFG_OMCI_SIDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* add omci tcont queue */
    queueCfg.cir = 0;
    queueCfg.pir = HAL_PONMAC_PIR_CIR_RATE_MAX();
    queueCfg.type = STRICT_PRIORITY;
    queueCfg.weight = 1;
    queueCfg.egrssDrop = DISABLED;
    if((ret= dal_rtl9603d_ponmac_queue_add(&logicalQueue, &queueCfg)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if !defined(FPGA_DEFINED)
    initParam.mode = PBO_MODE_GPON;
    initParam.usPageSize = PBO_PAGE_SIZE_128;
    initParam.dsPageSize = PBO_PAGE_SIZE_128;

#if 0 /*TBD*/
    if ((ret = dal_rtl9603d_pbo_init(initParam)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    /* set pbo ds sid2q OMCI to high queue */
    if ((ret = rtl9603d_raw_pbo_dsSidToQueueMap_set(RTL9603D_GPON_OMCI_FLOW_ID, RTL9603D_PBO_DSQUEUE_TYPE_HIGH)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
#endif
    
#endif
    /* serdes patch by Chinhao */
    /* set the serdes mode to off */
    data = 0x1f;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if !defined(FPGA_DEFINED)
    /* turn on 125M clock first to prevent other circuit has no clock */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_00r, RTL9603D_CFG_FRC_125M_ENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif

    /*#WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0*/
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }               
    /*#WSDS_DIG_03[6:4]: CFG_TXDIS_SEL_DLY<2:0>=0x2*/
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_03r, RTL9603D_CFG_TXDIS_SEL_DLYf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
      #SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1*/
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 //TBD RTL9603D
    /*#SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0xF
      #SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0*/
    data = 0xF;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x7
      #SDS_ANA_COM_REG25[1]: REG_LC_BYPASS_SF=0x0*/
    data = 0x7;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_LC_BYPASS_SFf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
      #SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xC
      #SDS_ANA_COM_REG21[6]:REG_CMU_BIG_KVCO_RX=0x0
      #SDS_ANA_COM_REG21[4:2]:REG_CMU_LPF_RS_RX=0x3*/
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0xC;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x3;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif
    
    /*#SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x0*/
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2*/
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 //TBD RTL9603D
    /*#SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
      #SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x4
      #SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x4*/
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2=0x4
      #SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2=0x4*/
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif

    /*#SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
      #SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1*/
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG13[7:5]: REG_TX_AMP=0x2*/
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG13r, RTL9603D_REG_TX_AMPf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
      #SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1*/
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* set the serdes mode to GPON mode */
    data = 0x8;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* no force sds */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_ONf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* reset serdes including digital and analog, and GPON MAC */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_CMD_SDS_RST_PSf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);

    /* force sds for not reset GPON MAC when SD down */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_VALf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_ONf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* endof serdes patch by Chinhao */   

    /* change mode to GPON, must reset switch */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_SW_RSTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);
    /* reset fifo TX/RX after reset switch */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_1Dr, RTL9603D_CFG_SFT_RSTB_INF_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_1Dr, RTL9603D_CFG_SFT_RSTB_INF_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_1Dr, RTL9603D_CFG_SFT_RSTB_INF_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_1Dr, RTL9603D_CFG_SFT_RSTB_INF_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* BEN on */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_18r, RTL9603D_BEN_OEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* pon port allow undersize packet */
    data = 1;
    if ((ret = reg_array_field_write(RTL9603D_P_MISCr, HAL_GET_PON_PORT(), REG_ARRAY_INDEX_NONE, RTL9603D_RX_SPCf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* set ben force mode to 0 */
    data = 0;
    if ((ret = reg_field_write(RTL9603D_FORCE_BENr, RTL9603D_BEN_FORCE_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* set max packet len to 2031 (2047-4(ctag)-4(stag)-8(pppoe)=2031) */
    if((ret = dal_rtl9603d_ponmac_maxPktLen_set(2031)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0
    /*flow control threshold and set buffer mode*/
    if((ret = rtl9603d_raw_flowctrl_patch(FLOWCTRL_PATCH_35M_GPON)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#endif

#if !defined(FPGA_DEFINED)
    /* polling until FEP_V2ANALOG =1 for turn off 125M clock */
    do
    {
        osal_time_udelay(200);
        if ((ret = reg_field_read(RTL9603D_FIB_EXT_REG21r, RTL9603D_FEP_V2ANALOGf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        cnt++;
    } while ((data == 0) && (cnt < 1000));
    if(cnt < 1000)
    {
        /* turn off 125M clock for power saving*/
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_WSDS_DIG_00r, RTL9603D_CFG_FRC_125M_ENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
    }
    else
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "polling FEP_V2ANALOG fail!");
    }
#endif

    /* set PON_INBW_HBOUND(0xfe0000 default) PON_INBW_LBOUND(0xfda000) for DS PBO accumulation */
    data = 0xfda000 ;
    if ((ret = reg_field_write(RTL9603D_PON_INBW_LBOUNDr, RTL9603D_LBOUNDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

static int32 _rtl9603d_ponMacGponModeV2_set(void)
{
    int32 ret;
    uint32 data;
    rtk_ponmac_queue_t logicalQueue;
    rtk_ponmac_queueCfg_t queueCfg;
    uint32 flowId;
#if !defined(FPGA_DEFINED)
    rtk_pbo_initParam_t initParam;
#endif
    uint32 cnt=0;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    /* mapping all flow to t-cont 15 queue 7, and set SID invalid */
    logicalQueue.schedulerId = 15;
    logicalQueue.queueId     = 6;
    for(flowId=0 ; flowId<HAL_CLASSIFY_SID_NUM()-1 ; flowId++)
    {
        if((ret= dal_rtl9603d_ponmac_flow2Queue_set(flowId, &logicalQueue)) != RT_ERR_OK)
        {
            ponmac_init = INIT_NOT_COMPLETED;
            return ret;
        }

        data = 0x0;
        if ((ret = reg_array_field_write(RTL9603D_PON_SIDVALIDr, REG_ARRAY_INDEX_NONE, flowId, RTL9603D_CFG_SID2VALIDf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
    }

    /* omci flow to queue */
    logicalQueue.schedulerId = RTL9603D_GPON_OMCI_TCONT_ID;
    logicalQueue.queueId = RTL9603D_GPON_OMCI_QUEUE_ID;
    if((ret= dal_rtl9603d_ponmac_flow2Queue_set(RTL9603D_GPON_OMCI_FLOW_ID, &logicalQueue)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* set omci sid valid */
    data = 0x1;
    if ((ret = reg_array_field_write(RTL9603D_PON_SIDVALIDr, REG_ARRAY_INDEX_NONE, RTL9603D_GPON_OMCI_FLOW_ID, RTL9603D_CFG_SID2VALIDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* set CFG_OMCI_SID */
    data = RTL9603D_GPON_OMCI_FLOW_ID;
    if ((ret = reg_field_write(RTL9603D_PON_OMCI_CFGr, RTL9603D_CFG_OMCI_SIDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* add omci tcont queue */
    queueCfg.cir = 0;
    queueCfg.pir = HAL_PONMAC_PIR_CIR_RATE_MAX();
    queueCfg.type = STRICT_PRIORITY;
    queueCfg.weight = 1;
    queueCfg.egrssDrop = DISABLED;
    if((ret= dal_rtl9603d_ponmac_queue_add(&logicalQueue, &queueCfg)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if !defined(FPGA_DEFINED)
    initParam.mode = PBO_MODE_GPON;
    initParam.usPageSize = PBO_PAGE_SIZE_128;
    initParam.dsPageSize = PBO_PAGE_SIZE_128;
#if 0 /*TBD*/
    if ((ret = dal_rtl9603d_pbo_init(initParam)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    /* set pbo ds sid2q OMCI to high queue */
    if ((ret = rtl9603d_raw_pbo_dsSidToQueueMap_set(RTL9603D_GPON_OMCI_FLOW_ID, RTL9603D_PBO_DSQUEUE_TYPE_HIGH)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
#endif

#endif
    /* serdes patch by Chinhao, GPON_Init_Patch_20171218 */
    /* set the serdes mode to off */
    data = 0x1f;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#if !defined(FPGA_DEFINED)
    /* turn on 125M clock first to prevent other circuit has no clock */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_00r, RTL9603D_CFG_FRC_125M_ENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif
    /*#WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0*/
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }               
    /*#WSDS_DIG_03[6:4]: CFG_TXDIS_SEL_DLY<2:0>=0x2*/
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_03r, RTL9603D_CFG_TXDIS_SEL_DLYf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
      #SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1*/
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 //TBD RTL9603D
    /*#SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0xF
      #SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0*/
    data = 0xF;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x7
      #SDS_ANA_COM_REG25[1]: REG_LC_BYPASS_SF=0x0*/
    data = 0x7;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_LC_BYPASS_SFf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
      #SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xC
      #SDS_ANA_COM_REG21[6]:REG_CMU_BIG_KVCO_RX=0x0
      #SDS_ANA_COM_REG21[4:2]:REG_CMU_LPF_RS_RX=0x3*/
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0xC;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x3;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif
    
    /*#SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x1*/
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2*/
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 //TBD RTL9603D
    /*#SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
      #SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x0
      #SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x6*/
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x6;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2=0x1
      #SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2=0x1*/
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif

    /*#SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
      #SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1*/
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG13[7:5]: REG_TX_AMP=0x2*/
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG13r, RTL9603D_REG_TX_AMPf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*#SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
      #SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1*/
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#if 0 //TBD RTL9603D
    /*#SDS_ANA_COM_REG00[1]: REG_CDR_KD=0*/
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG00r, RTL9603D_REG_CDR_KDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif
    /* set the serdes mode to GPON mode */
    data = 0x8;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* no force sds */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_ONf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* reset serdes including digital and analog, and GPON MAC */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_CMD_SDS_RST_PSf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);

    /* force sds for not reset GPON MAC when SD down */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_VALf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_ONf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* endof serdes patch by Chinhao */   

    /* change mode to GPON, must reset switch */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_SW_RSTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);
    /* reset fifo TX/RX after reset switch */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_1Dr, RTL9603D_CFG_SFT_RSTB_INF_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_1Dr, RTL9603D_CFG_SFT_RSTB_INF_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_1Dr, RTL9603D_CFG_SFT_RSTB_INF_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_1Dr, RTL9603D_CFG_SFT_RSTB_INF_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* BEN on */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_18r, RTL9603D_BEN_OEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* pon port allow undersize packet */
    data = 1;
    if ((ret = reg_array_field_write(RTL9603D_P_MISCr, HAL_GET_PON_PORT(), REG_ARRAY_INDEX_NONE, RTL9603D_RX_SPCf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /* set ben force mode to 0 */
    data = 0;
    if ((ret = reg_field_write(RTL9603D_FORCE_BENr, RTL9603D_BEN_FORCE_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* set max packet len to 2031 (2047-4(ctag)-4(stag)-8(pppoe)=2031) */
    if((ret = dal_rtl9603d_ponmac_maxPktLen_set(2031)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 /*TBD*/
    /*flow control threshold and set buffer mode*/
    if((ret = rtl9603d_raw_flowctrl_patch(FLOWCTRL_PATCH_35M_GPON)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#endif

#if !defined(FPGA_DEFINED)
    /* polling until FEP_V2ANALOG =1 for turn off 125M clock */
    do
    {
        osal_time_udelay(200);
        if ((ret = reg_field_read(RTL9603D_FIB_EXT_REG21r, RTL9603D_FEP_V2ANALOGf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        cnt++;
    } while ((data == 0) && (cnt < 1000));
    if(cnt < 1000)
    {
        /* turn off 125M clock for power saving*/
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_WSDS_DIG_00r, RTL9603D_CFG_FRC_125M_ENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
    }
    else
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "polling FEP_V2ANALOG fail!");
    }
#endif

    /* set PON_INBW_HBOUND(0xfe0000 default) PON_INBW_LBOUND(0xfda000) for DS PBO accumulation */
    data = 0xfda000 ;
    if ((ret = reg_field_write(RTL9603D_PON_INBW_LBOUNDr, RTL9603D_LBOUNDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

static int32 _rtl9603d_ponMacEponModeV1_set(void)
{
    int32 ret;
    uint32 data;
    uint32 flowId,tcont;
#if !defined(FPGA_DEFINED)
    rtk_pbo_initParam_t initParam;
#endif

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

#if !defined(FPGA_DEFINED)
    initParam.mode = PBO_MODE_EPON;
    initParam.usPageSize = PBO_PAGE_SIZE_128;
    initParam.dsPageSize = PBO_PAGE_SIZE_128;
#if 0 /*TBD*/
    if ((ret = dal_rtl9603d_pbo_init(initParam)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    /* set pbo ds sid2q MPCP/OAM to high queue for LLID 0~7 */
    /*mapping mpcp oam packet ot pbo high queue*/
    {
        uint32 streamId;
        for(streamId=0x10 ; streamId<=0x1b ; streamId++)
        {
            if ((ret = rtl9603d_raw_pbo_dsSidToQueueMap_set(streamId,RTL9603D_PBO_DSQUEUE_TYPE_HIGH)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        }
    }            
#endif

#endif


    /* set the serdes mode to off */
    data = 0x1f;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*
    #FIB_REG0[11]: FP_CFG_FIB_PDOWN=0x0
    reg set 0x40c00 0x1140
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_FIB_REG0r, RTL9603D_FP_CFG_FIB_PDOWNf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }            
   /*
    #SDS_REG2[9:8]: SP_SDS_FRC_AN<1:0>=0x1 //NWAY disable
    reg set 0x40808 0x71e0
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_REG2r, RTL9603D_SP_SDS_FRC_ANf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }              
    /*
    #WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0
    reg set 0x40038 0x900
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }                

    /*
    #SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
    #SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1
    reg set 0x405c4 0x2a89
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 //TBD RTL9603D    
    /*            
    #SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0x8
    #SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0
    reg set 0x405e0 0x43C0
    */
    data = 0x8;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }            
    /*            
    #SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x1
    reg set 0x405e4 0x2F42
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    /*            
    #SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
    #SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xF
    #SDS_ANA_COM_REG21[6]: REG_CMU_BIG_KVCO_RX=0x1
    #SDS_ANA_COM_REG21[4:2]: REG_CMU_LPF_RS_RX=0x4
    reg set 0x405d4 0xFDF0
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    data = 0xf;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    /*                
    #SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
    #SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x4
    #SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x4
    reg set 0x40588 0x321D
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }  
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }  
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }  
#endif    
    /*            
    #SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x0
    reg set 0x405a4 0x1800
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }  

    /*SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2*/
    /*reg set 0x40598 0x8002*/
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 //TBD RTL9603D
    /*
    #SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2 =0x4
    #SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2 =0x4
    reg set 0x405a0 0x4845
    */
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }                
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }                
#endif

    /*
    #SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
    #SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1
    reg set 0x405b0 0x11  
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    
    /*
    #SDS_ANA_COM_REG13[7:5]: REG_TX_AMP=0x2
    reg set 0x405b4 0x9150
    */
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG13r, RTL9603D_REG_TX_AMPf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /*
    #SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
    #SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1
    reg set 0x405b8 0x2107
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /*
    #PON MODE CFG
    reg set 0x270 0xC
    */
    /* set the serdes mode to EPON mode */
    data = 0xc;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*
    #SDS_ANA_MISC_REG02[13]: FRC_BER_NOTIFY_VAL=0x1
    #SDS_ANA_MISC_REG02[12]: FRC_BER_NOTIFY_ON=0x1
    reg set 0x40508 0x3000
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_VALf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_ONf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }



    

    /* change mode to EPON, must reset switch */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_SW_RSTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);


    /* reset serdes including digital and analog */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_00r, RTL9603D_CFG_SFT_RSTBf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_00r, RTL9603D_CFG_SFT_RSTBf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);

    /* BEN on */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_18r, RTL9603D_BEN_OEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* SDS_REG1.SP_SDS_FRC_RX = 0 */
    data = 0;
    if ((ret = reg_field_write(RTL9603D_SDS_REG1r, RTL9603D_SP_SDS_FRC_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }


    /*SDS_REG2  SDS_REG2.SP_FRC_IPG = 1 for 129 byte issue*/
    data = 1;
    if ((ret = reg_field_write(RTL9603D_SDS_REG2r,RTL9603D_SP_FRC_IPGf,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*PCS serdes patch*/
    data = 3;
    if ((ret = reg_field_write(RTL9603D_SDS_EXT_REG12r, RTL9603D_SEP_CFG_IPG_CNTf, (uint32 *)&data)) != RT_ERR_OK)
    {                                                             
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    /*EPON ds fec*/
    data = 0x43;
    if ((ret = reg_field_write(RTL9603D_SDS_EXT_REG6r, RTL9603D_SEP_CFG_FEC_MK_OPTf, (uint32 *)&data)) != RT_ERR_OK)
    {                                                             
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    /*FEC must check this setting*/
    data = 0x4e6a;
    if ((ret = reg_write(RTL9603D_SDS_EXT_REG13r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*FEC must check this setting*/
    data = 0x1562;
    if ((ret = reg_write(RTL9603D_SDS_EXT_REG15r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*FEC must check this setting*/
    data = 0xbd2a;
    if ((ret = reg_write(RTL9603D_SDS_EXT_REG16r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    data = 0x59;
    if ((ret = reg_write(RTL9603D_SDS_REG7r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* set ben force mode to 0 */
    data = 0;
    if ((ret = reg_field_write(RTL9603D_FORCE_BENr, RTL9603D_BEN_FORCE_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* force sds for not reset EPON MAC when SD down */
    data = 0xc;
    if ((ret = reg_write(RTL9603D_WSDS_DIG_01r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* set max packet len to 2031 (2047-4(ctag)-4(stag)-8(pppoe)=2031) */
    if((ret = dal_rtl9603d_ponmac_maxPktLen_set(2031)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }


    for(tcont = 0; tcont <HAL_MAX_NUM_OF_GPON_TCONT()-1; tcont++ )
    {
        /*disable T-cont schedule */
        if ((ret = _rtl9603d_ponMacTcontEnable_set(tcont, DISABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
        /*clear T-cont schedule mask*/
        if ((ret = _rtl9603d_ponMacScheQmap_set(tcont, 0)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }

    /*for epon default flow to queue is one to one mapping*/
    for(flowId=0 ; flowId<HAL_CLASSIFY_SID_NUM()-1 ; flowId++)
    {
    
        if((ret= _rtl9603d_ponMacSidToQueueMap_set(flowId, flowId)) != RT_ERR_OK)
        {
            ponmac_init = INIT_NOT_COMPLETED;
            return ret;
        }
    }
#if 0 /*TBD*/
    /*flow control threshold and set buffer mode (EPON/GPON are the same)*/
    if((ret = rtl9603d_raw_flowctrl_patch(FLOWCTRL_PATCH_35M_GPON)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#endif
    /* set PON_INBW_HBOUND(0xfe0000 default) PON_INBW_LBOUND(0xfda000) for DS PBO accumulation */
    data = 0xfda000 ;
    if ((ret = reg_field_write(RTL9603D_PON_INBW_LBOUNDr, RTL9603D_LBOUNDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

static int32 _rtl9603d_ponMacEponModeV2_set(void)
{
    int32 ret;
    uint32 data;
    uint32 flowId,tcont;
#if !defined(FPGA_DEFINED)
    rtk_pbo_initParam_t initParam;
#endif

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

#if !defined(FPGA_DEFINED)
    initParam.mode = PBO_MODE_EPON;
    initParam.usPageSize = PBO_PAGE_SIZE_128;
    initParam.dsPageSize = PBO_PAGE_SIZE_128;
#if 0 /*TBD*/
    if ((ret = dal_rtl9603d_pbo_init(initParam)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    /* set pbo ds sid2q MPCP/OAM to high queue for LLID 0~7 */
    /*mapping mpcp oam packet ot pbo high queue*/
    {
        uint32 streamId;
        for(streamId=0x10 ; streamId<=0x1b ; streamId++)
        {
            if ((ret = rtl9603d_raw_pbo_dsSidToQueueMap_set(streamId,RTL9603D_PBO_DSQUEUE_TYPE_HIGH)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        }
    }
#endif    
#endif

    /* Based on FIB1G_Init_Patch_20171218 */
    /* set the serdes mode to off */
    data = 0x1f;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*
    #FIB_REG0[11]: FP_CFG_FIB_PDOWN=0x0
    reg set 0x40c00 0x1140
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_FIB_REG0r, RTL9603D_FP_CFG_FIB_PDOWNf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }            
   /*
    #SDS_REG2[9:8]: SP_SDS_FRC_AN<1:0>=0x1 //NWAY disable
    reg set 0x40808 0x71e0
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_REG2r, RTL9603D_SP_SDS_FRC_ANf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }              
    /*
    #WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0
    reg set 0x40038 0x900
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }                

    /*
    #SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
    #SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1
    reg set 0x405c4 0x2a89
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 //TBD RTL9603D    
    /*            
    #SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0x8
    #SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0
    reg set 0x405e0 0x43C0
    */
    data = 0x8;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }            
    /*            
    #SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x1
    reg set 0x405e4 0x2F42
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    /*            
    #SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
    #SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xF
    #SDS_ANA_COM_REG21[6]: REG_CMU_BIG_KVCO_RX=0x1
    #SDS_ANA_COM_REG21[4:2]: REG_CMU_LPF_RS_RX=0x4
    reg set 0x405d4 0xFDF0
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    data = 0xf;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    data = 0x4;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }             
    /*                
    #SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
    #SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x0
    #SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x6
    reg set 0x40588 0x321D
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }  
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }  
    data = 0x6;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }  
#endif

    /*            
    #SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x1
    reg set 0x405a4 0x3800
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }  

    /*SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2*/
    /*reg set 0x40598 0x8002*/
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#if 0 //TBD RTL9603D
    /*
    #SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2 =0x1
    #SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2 =0x1
    reg set 0x405a0 0x4845
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }                
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }                
#endif

    /*
    #SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
    #SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1
    reg set 0x405b0 0x11  
    */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    
    /*
    #SDS_ANA_COM_REG13[7:5]: REG_TX_AMP=0x2
    reg set 0x405b4 0x9150
    */
    data = 0x2;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG13r, RTL9603D_REG_TX_AMPf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /*
    #SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
    #SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1
    reg set 0x405b8 0x2107
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#if 0 //TBD RTL9603D
    /*
    #SDS_ANA_COM_REG00[1]: REG_CDR_KD=0
    reg set 0x40580 0x80
    */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG00r, RTL9603D_REG_CDR_KDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif

    /*
    #PON MODE CFG
    reg set 0x270 0xC
    */
    /* set the serdes mode to EPON mode */
    data = 0xc;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* change mode to EPON, must reset switch */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_SW_RSTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);


    /* reset serdes including digital and analog */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_00r, RTL9603D_CFG_SFT_RSTBf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_00r, RTL9603D_CFG_SFT_RSTBf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);

    /* BEN on */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_18r, RTL9603D_BEN_OEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* SDS_REG1.SP_SDS_FRC_RX = 0 */
    data = 0;
    if ((ret = reg_field_write(RTL9603D_SDS_REG1r, RTL9603D_SP_SDS_FRC_RXf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }


    /*SDS_REG2  SDS_REG2.SP_FRC_IPG = 1 for 129 byte issue*/
    data = 1;
    if ((ret = reg_field_write(RTL9603D_SDS_REG2r,RTL9603D_SP_FRC_IPGf,&data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*PCS serdes patch*/
    data = 3;
    if ((ret = reg_field_write(RTL9603D_SDS_EXT_REG12r, RTL9603D_SEP_CFG_IPG_CNTf, (uint32 *)&data)) != RT_ERR_OK)
    {                                                             
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    /*EPON ds fec*/
    data = 0x43;
    if ((ret = reg_field_write(RTL9603D_SDS_EXT_REG6r, RTL9603D_SEP_CFG_FEC_MK_OPTf, (uint32 *)&data)) != RT_ERR_OK)
    {                                                             
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }


    /*FEC must check this setting*/
    data = 0x4e6a;
    if ((ret = reg_write(RTL9603D_SDS_EXT_REG13r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*FEC must check this setting*/
    data = 0x1562;
    if ((ret = reg_write(RTL9603D_SDS_EXT_REG15r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    /*FEC must check this setting*/
    data = 0xbd2a;
    if ((ret = reg_write(RTL9603D_SDS_EXT_REG16r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    data = 0x59;
    if ((ret = reg_write(RTL9603D_SDS_REG7r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* set ben force mode to 0 */
    data = 0;
    if ((ret = reg_field_write(RTL9603D_FORCE_BENr, RTL9603D_BEN_FORCE_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* force sds for not reset EPON MAC when SD down */
    data = 0xc;
    if ((ret = reg_write(RTL9603D_WSDS_DIG_01r, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* set max packet len to 2031 (2047-4(ctag)-4(stag)-8(pppoe)=2031) */
    if((ret = dal_rtl9603d_ponmac_maxPktLen_set(2031)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }


    for(tcont = 0; tcont <HAL_MAX_NUM_OF_GPON_TCONT()-1; tcont++ )
    {
        /*disable T-cont schedule */
        if ((ret = _rtl9603d_ponMacTcontEnable_set(tcont, DISABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
        /*clear T-cont schedule mask*/
        if ((ret = _rtl9603d_ponMacScheQmap_set(tcont, 0)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }

    /*for epon default flow to queue is one to one mapping*/
    for(flowId=0 ; flowId<HAL_CLASSIFY_SID_NUM()-1 ; flowId++)
    {
    
        if((ret= _rtl9603d_ponMacSidToQueueMap_set(flowId, flowId)) != RT_ERR_OK)
        {
            ponmac_init = INIT_NOT_COMPLETED;
            return ret;
        }
    }
#if 0 /*TBD*/
    /*flow control threshold and set buffer mode (EPON/GPON are the same)*/
    if((ret = rtl9603d_raw_flowctrl_patch(FLOWCTRL_PATCH_35M_GPON)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#endif

    /* set PON_INBW_HBOUND(0xfe0000 default) PON_INBW_LBOUND(0xfda000) for DS PBO accumulation */
    data = 0xfda000 ;
    if ((ret = reg_field_write(RTL9603D_PON_INBW_LBOUNDr, RTL9603D_LBOUNDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

static int32 _rtl9603d_ponMacFiberModeV1_set(rtk_ponmac_mode_t mode)
{
    int32 ret;
    uint32 data;
    rtk_ponmac_queueCfg_t   queueCfg;
    rtk_ponmac_queue_t logicalQueue;
    uint32 queueId;
    uint32 flowId;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    RT_PARAM_CHK((PONMAC_MODE_END <= mode), RT_ERR_INPUT);

    /*mapping queue 0~7 to schedule id 0 if not GPON/EPON */
    memset(&queueCfg, 0, sizeof(rtk_ponmac_queueCfg_t));

    logicalQueue.schedulerId = 0 ;

    queueCfg.cir       = 0x0;
    queueCfg.pir       = HAL_PONMAC_PIR_CIR_RATE_MAX();
    queueCfg.type      = STRICT_PRIORITY;
    queueCfg.egrssDrop = DISABLED;
    ponmac_init = INIT_COMPLETED;
    for(queueId = 0 ; queueId <= RTK_DOT1P_PRIORITY_MAX ; queueId++ )
    {
        logicalQueue.queueId = queueId;

        if((ret= dal_rtl9603d_ponmac_queue_add(&logicalQueue, &queueCfg)) != RT_ERR_OK)
        {
            ponmac_init = INIT_NOT_COMPLETED;
            return ret;
        }
    }
    logicalQueue.schedulerId = 0;
    logicalQueue.queueId     = 0;

    for(flowId=0 ; flowId<HAL_CLASSIFY_SID_NUM()-1 ; flowId++)
    {
        if((ret= dal_rtl9603d_ponmac_flow2Queue_set(flowId, &logicalQueue)) != RT_ERR_OK)
        {
            ponmac_init = INIT_NOT_COMPLETED;
            return ret;
        }
    }

    /* =========== Scheduling config =========== */
    /* Config queue mapping for fiber */
    /* add queue 0 to scheduler 0*/
    data = 1;
    if ((ret = _rtl9603d_ponMacScheQmap_set(0, data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    /* enable scheduler 0*/
    if ((ret = _rtl9603d_ponMacTcontEnable_set(0, ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /* =========== Fiber Config =========== */
#ifdef CONFIG_FIBER_DETECT_OOBS
    /* SD from OOBS */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_OOBS_ENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_1P25_REG38r, RTL9603D_REG_RX_SEL_SDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#else
    /* CONFIG_FIBER_DETECT_SD */
    /* SD from signal detect */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_OOBS_ENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_1P25_REG38r, RTL9603D_REG_RX_SEL_SDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#ifdef CONFIG_FIBER_MODULE_LOSS
    /* SD is lOSS */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#else
    /* SD is signal detect */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif
#endif
    /* Set the serdes mode to disable mode first */
    data = 0x1f;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* Disable silent start */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_FIB_EXT_REG19r, RTL9603D_FEP_CFG_TX_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* Config serdes mode according to actual settings */
    switch(mode)
    {
    case PONMAC_MODE_FIBER_1G:
    case PONMAC_MODE_FIBER_FORCE_1G:
        /* Based on FIB1G_Init_Patch_20170815 */
        /* SDS_ANA_MISC_REG01[3]:FRC_125M_VALUE=0
         * SDS_ANA_MISC_REG01[2]:FRC_125M_ON=1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG01r, RTL9603D_FRC_125M_VALUEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG01r, RTL9603D_FRC_125M_ONf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
         * SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0x8
         * SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0
         */
        data = 0x8;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x2 */
        data = 0x2;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
         * SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xF
         * SDS_ANA_COM_REG21[6]:REG_CMU_BIG_KVCO_RX=0x1
         * SDS_ANA_COM_REG21[4:2]:REG_CMU_LPF_RS_RX=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0xf;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif
        /* SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x0
         * SDS_ANA_COM_REG09[10]: REG_RX_SD_POR_SEL=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2 */
        data = 0x2;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
         * SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x4
         * SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2=0x4
         * SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2=0x4
         */
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif
        /* SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
         * SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
         * SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

        /* Set the serdes mode to fiber 1G mode */
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        if(PONMAC_MODE_FIBER_FORCE_1G == mode)
        {
            /* Disable AN */
            data = 0x0140;
        }
        else
        {
            data = 0x1140;
        }
        if ((ret = reg_write(RTL9603D_FIB_REG0r, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        break;
    case PONMAC_MODE_FIBER_100M:
    case PONMAC_MODE_FIBER_FORCE_100M:
        /* Based on FIB100_Init_Patch_20170815 */
        /* SDS_ANA_MISC_REG01[3]:FRC_125M_VALUE=1
         * SDS_ANA_MISC_REG01[2]:FRC_125M_ON=1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG01r, RTL9603D_FRC_125M_VALUEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG01r, RTL9603D_FRC_125M_ONf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
         * SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0x8
         * SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0
         */
        data = 0x8;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x1 */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

        /* SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
         * SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xF
         * SDS_ANA_COM_REG21[6]:REG_CMU_BIG_KVCO_RX=0x1
         * SDS_ANA_COM_REG21[4:2]:REG_CMU_LPF_RS_RX=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0xf;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif

        /* SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x0
         * SDS_ANA_COM_REG09[10]: REG_RX_SD_POR_SEL=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2 */
        data = 0x2;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
         * SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x4
         * SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2=0x4
         * SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2=0x4
         */
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif
        /* SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
         * SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
         * SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* Set the serdes mode to fiber 100M mode */
        data = 0x5;
        if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x2100;
        if ((ret = reg_write(RTL9603D_FIB_REG0r, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        break;
    case PONMAC_MODE_FIBER_AUTO:
        /* Based on Fiber_auto_Init_Patch_20170815 */
        /* WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
         * SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0x8
         * SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0
         */
        data = 0x8;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x1 */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
         * SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xF
         * SDS_ANA_COM_REG21[6]:REG_CMU_BIG_KVCO_RX=0x1
         * SDS_ANA_COM_REG21[4:2]:REG_CMU_LPF_RS_RX=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0xf;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif
        /* SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x0
         * SDS_ANA_COM_REG09[10]: REG_RX_SD_POR_SEL=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2 */
        data = 0x2;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
         * SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x4
         * SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2=0x4
         * SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2=0x4
         */
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif

        /* SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
         * SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
         * SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

        /* Set the serdes mode to fiber auto mode */
        data = 0x7;
        if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1140;
        if ((ret = reg_write(RTL9603D_FIB_REG0r, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        break;
    default:
        break;
    }
    /* SDS_ANA_MISC_REG02[13]: FRC_BER_NOTIFY_VAL=0x1
     * SDS_ANA_MISC_REG02[12]: FRC_BER_NOTIFY_ON=0x1
     */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_VALf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG02r, RTL9603D_FRC_BER_NOTIFY_ONf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* change mode to fiber, must reset switch */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_SW_RSTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);

    return RT_ERR_OK;
}

static int32 _rtl9603d_ponMacFiberModeV2_set(rtk_ponmac_mode_t mode)
{
    int32 ret;
    uint32 data;
    rtk_ponmac_queueCfg_t   queueCfg;
    rtk_ponmac_queue_t logicalQueue;
    uint32 queueId;
    uint32 flowId;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    RT_PARAM_CHK((PONMAC_MODE_END <= mode), RT_ERR_INPUT);

    /*mapping queue 0~7 to schedule id 0 if not GPON/EPON */
    memset(&queueCfg, 0, sizeof(rtk_ponmac_queueCfg_t));

    logicalQueue.schedulerId = 0 ;

    queueCfg.cir       = 0x0;
    queueCfg.pir       = HAL_PONMAC_PIR_CIR_RATE_MAX();
    queueCfg.type      = STRICT_PRIORITY;
    queueCfg.egrssDrop = DISABLED;
    ponmac_init = INIT_COMPLETED;
    for(queueId = 0 ; queueId <= RTK_DOT1P_PRIORITY_MAX ; queueId++ )
    {
        logicalQueue.queueId = queueId;

        if((ret= dal_rtl9603d_ponmac_queue_add(&logicalQueue, &queueCfg)) != RT_ERR_OK)
        {
            ponmac_init = INIT_NOT_COMPLETED;
            return ret;
        }
    }
    logicalQueue.schedulerId = 0;
    logicalQueue.queueId     = 0;

    for(flowId=0 ; flowId<HAL_CLASSIFY_SID_NUM()-1 ; flowId++)
    {
        if((ret= dal_rtl9603d_ponmac_flow2Queue_set(flowId, &logicalQueue)) != RT_ERR_OK)
        {
            ponmac_init = INIT_NOT_COMPLETED;
            return ret;
        }
    }

    /* =========== Scheduling config =========== */
    /* Config queue mapping for fiber */
    /* add queue 0 to scheduler 0*/
    data = 1;
    if ((ret = _rtl9603d_ponMacScheQmap_set(0, data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    /* enable scheduler 0*/
    if ((ret = _rtl9603d_ponMacTcontEnable_set(0, ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /* =========== Fiber Config =========== */
#ifdef CONFIG_FIBER_DETECT_OOBS
    /* SD from OOBS */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_OOBS_ENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_1P25_REG38r, RTL9603D_REG_RX_SEL_SDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#else
    /* CONFIG_FIBER_DETECT_SD */
    /* SD from signal detect */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_OOBS_ENf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_1P25_REG38r, RTL9603D_REG_RX_SEL_SDf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

#ifdef CONFIG_FIBER_MODULE_LOSS
    /* SD is lOSS */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#else
    /* SD is signal detect */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif
#endif
    /* Set the serdes mode to disable mode first */
    data = 0x1f;
    if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* Disable silent start */
    data = 0x0;
    if ((ret = reg_field_write(RTL9603D_FIB_EXT_REG19r, RTL9603D_FEP_CFG_TX_MODEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }

    /* Config serdes mode according to actual settings */
    switch(mode)
    {
    case PONMAC_MODE_FIBER_1G:
    case PONMAC_MODE_FIBER_FORCE_1G:
        /* Based on FIB1G_Init_Patch_20171218 */
        /* SDS_ANA_MISC_REG01[3]:FRC_125M_VALUE=0
         * SDS_ANA_MISC_REG01[2]:FRC_125M_ON=1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG01r, RTL9603D_FRC_125M_VALUEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG01r, RTL9603D_FRC_125M_ONf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
         * SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0x8
         * SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0
         */
        data = 0x8;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x2 */
        data = 0x2;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
         * SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xF
         * SDS_ANA_COM_REG21[6]:REG_CMU_BIG_KVCO_RX=0x1
         * SDS_ANA_COM_REG21[4:2]:REG_CMU_LPF_RS_RX=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0xf;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif
        /* SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x1
         * SDS_ANA_COM_REG09[10]: REG_RX_SD_POR_SEL=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2 */
        data = 0x2;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
         * SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x0
         * SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x6
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x6;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2=0x1
         * SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif
        /* SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
         * SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
         * SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

        /* SDS_ANA_1P25_REG38[12]: REG_RX_SEL_SD=0x1 */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_1P25_REG38r, RTL9603D_REG_RX_SEL_SDf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG00[1]: REG_CDR_KD=0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG00r, RTL9603D_REG_CDR_KDf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif

        /* Set the serdes mode to fiber 1G mode */
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        if(PONMAC_MODE_FIBER_FORCE_1G == mode)
        {
            /* Disable AN */
            data = 0x0140;
        }
        else
        {
            data = 0x1140;
        }
        if ((ret = reg_write(RTL9603D_FIB_REG0r, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        break;
    case PONMAC_MODE_FIBER_100M:
    case PONMAC_MODE_FIBER_FORCE_100M:
        /* Based on FIB100_Init_Patch_20171218 */
        /* SDS_ANA_MISC_REG01[3]:FRC_125M_VALUE=1
         * SDS_ANA_MISC_REG01[2]:FRC_125M_ON=1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG01r, RTL9603D_FRC_125M_VALUEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_MISC_REG01r, RTL9603D_FRC_125M_ONf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
         * SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0x8
         * SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0
         */
        data = 0x8;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x1 */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
         * SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xF
         * SDS_ANA_COM_REG21[6]:REG_CMU_BIG_KVCO_RX=0x1
         * SDS_ANA_COM_REG21[4:2]:REG_CMU_LPF_RS_RX=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0xf;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#endif
        /* SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x1
         * SDS_ANA_COM_REG09[10]: REG_RX_SD_POR_SEL=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2 */
        data = 0x2;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
         * SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x0
         * SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x6
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x6;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2=0x1
         * SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif
        /* SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
         * SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
         * SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_1P25_REG38[12]: REG_RX_SEL_SD=0x1 */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_1P25_REG38r, RTL9603D_REG_RX_SEL_SDf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#if 0 //TBD RTL9603D        
        /* SDS_ANA_COM_REG00[1]: REG_CDR_KD=0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG00r, RTL9603D_REG_CDR_KDf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif
        /* Set the serdes mode to fiber 100M mode */
        data = 0x5;
        if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x2100;
        if ((ret = reg_write(RTL9603D_FIB_REG0r, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        break;
    case PONMAC_MODE_FIBER_AUTO:
        /* Based on Fiber_auto_Init_Patch_20170815 */
        /* WSDS_DIG_02[10]: REG_EN_PDOWN_BEN<0>=0x0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_WSDS_DIG_02r, RTL9603D_REG_EN_PDOWN_BENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG17[4]: REG_BEN_SEL_CML=0x0
         * SDS_ANA_COM_REG17[0]: REG_BEN_TTL_OUT=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG24[14:11]: REG_CMU_CP_I_TX=0x8
         * SDS_ANA_COM_REG24[3:1]: REG_CMU_LPF_CP_TX=0x0
         */
        data = 0x8;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_CP_I_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG24r, RTL9603D_REG_CMU_LPF_CP_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG25[15:13]: REG_CMU_LPF_RS_TX=0x1 */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG25r, RTL9603D_REG_CMU_LPF_RS_TXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG21[15]: REG_CMU_CCO_SEL_RX=0x1
         * SDS_ANA_COM_REG21[14:11]: REG_CMU_CP_I_RX=0xF
         * SDS_ANA_COM_REG21[6]:REG_CMU_BIG_KVCO_RX=0x1
         * SDS_ANA_COM_REG21[4:2]:REG_CMU_LPF_RS_RX=0x4
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CCO_SEL_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0xf;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_CP_I_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_BIG_KVCO_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x4;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG21r, RTL9603D_REG_CMU_LPF_RS_RXf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#endif
        /* SDS_ANA_COM_REG09[13]: REG_RX_SEL_CDR_AFEN=0x1
         * SDS_ANA_COM_REG09[10]: REG_RX_SD_POR_SEL=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SEL_CDR_AFENf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG09r, RTL9603D_REG_RX_SD_POR_SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG06[7:0]: REG_RX_FILT_CONFIG=0x2 */
        data = 0x2;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG06r, RTL9603D_REG_RX_FILT_CONFIGf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG02[15:13]: REG_CDR_KI=0x1
         * SDS_ANA_COM_REG02[12:10]: REG_CDR_KP1=0x0
         * SDS_ANA_COM_REG02[9:7]: REG_CDR_KP2=0x6
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KIf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP1f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x6;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG02r, RTL9603D_REG_CDR_KP2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG08[14:12]: REG_RX_KP1_2=0x1
         * SDS_ANA_COM_REG08[11:9]: REG_RX_KP2_2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP1_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG08r, RTL9603D_REG_RX_KP2_2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif

        /* SDS_ANA_COM_REG12[7:4]: REG_RX_EQ2SEL=0x1 
         * SDS_ANA_COM_REG12[3:0]: REG_RX_EQ2SEL2=0x1
         */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SELf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG12r, RTL9603D_REG_RX_EQ2SEL2f, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_COM_REG14[11:9]: REG_TX_EMP=0x0
         * SDS_ANA_COM_REG14[8]: REG_TX_EN_EMPHASIS=0x1
         */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EMPf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG14r, RTL9603D_REG_TX_EN_EMPHASf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        /* SDS_ANA_1P25_REG38[12]: REG_RX_SEL_SD=0x1 */
        data = 0x1;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_1P25_REG38r, RTL9603D_REG_RX_SEL_SDf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }

#if 0 //TBD RTL9603D
        /* SDS_ANA_COM_REG00[1]: REG_CDR_KD=0 */
        data = 0x0;
        if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG00r, RTL9603D_REG_CDR_KDf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
#endif

        /* Set the serdes mode to fiber auto mode */
        data = 0x7;
        if ((ret = reg_field_write(RTL9603D_SDS_CFGr, RTL9603D_CFG_SDS_MODEf, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        data = 0x1140;
        if ((ret = reg_write(RTL9603D_FIB_REG0r, &data)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
            return ret;
        }
        break;
    default:
        break;
    }

    /* change mode to fiber, must reset switch */
    data = 0x1;
    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_SW_RSTf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
    osal_time_mdelay(10);

    return RT_ERR_OK;
}

/* Module Name    : PON Mac                                  */
/* Sub-module Name: flow/t-cont/queue mapping */


/* Function Name:
 *      dal_rtl9603d_ponmac_init
 * Description:
 *      Configure PON MAC initial settings
 * Input:
 *      None.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_QUEUE_NUM - Invalid queue number
 * Note:
 */
int32
dal_rtl9603d_ponmac_init(void)
{
    int32  ret;
    uint32 tcont,wData;
    uint32 physicalQid;
    rtk_swPbo_initParam_t initParam;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

#ifdef FPGA_DEFINED
    /*fpga do not have RTL9603D_WSDS_ANA_24*/
#else /*FPGA_DEFINED*/

#ifdef CONFIG_BEN_DIFFERENTIAL
    wData = 0;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    wData = 1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_SEL_CMLf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
#else /*CONFIG_BEN_DIFFERENTIAL*/
    wData = 1;
    if ((ret = reg_field_write(RTL9603D_SDS_ANA_COM_REG17r, RTL9603D_REG_BEN_TTL_OUTf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
#endif /*CONFIG_BEN_DIFFERENTIAL*/
#endif /*FPGA_DEFINED*/

    /*init PON BW_THRESHOLD*/
    wData = 5;
    if ((ret = reg_field_write(RTL9603D_PON_BW_THRESr, RTL9603D_CFG_BW_LAST_THRESf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*init PON REQ_BW_THRESHOLD*/
    wData = 5;
    if ((ret = reg_field_write(RTL9603D_PON_BW_THRESr, RTL9603D_CFG_BW_RUNT_THRESf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*drant out all queue when pon mac init*/
    if(INIT_COMPLETED == ponmac_init)
    {
        uint32 schedulerId;

        for(schedulerId = 0 ; schedulerId < HAL_MAX_NUM_OF_GPON_TCONT() ; schedulerId++ )
        {
            if((ret= _rtl9603d_ponMacTcontDrainOutState_set(schedulerId)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        }
    }

    for(tcont = 0; tcont <HAL_MAX_NUM_OF_GPON_TCONT()-1; tcont++ )
    {
        /*disable T-cont schedule */
        if ((ret = _rtl9603d_ponMacTcontEnable_set(tcont, DISABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
        /*clear T-cont schedule mask*/
        if ((ret = _rtl9603d_ponMacScheQmap_set(tcont, 0)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }

    wData = 1;
    if ((ret = reg_field_write(RTL9603D_PON_SCH_CTRLr, RTL9603D_PON_GEN_PIR_DROPf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*set all queue to strict, disable CIR/PIR and disable egress drop*/
    for(physicalQid=0 ; physicalQid<HAL_MAX_NUM_OF_PON_QUEUE(); physicalQid++)
    {
        /*set queue schedule type*/
        if ((ret = _rtl9603d_ponMac_queueType_set(physicalQid, STRICT_PRIORITY)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }

        /*set PIR CIR*/
        if ((ret = _rtl9603d_ponMacCirRate_set(physicalQid, 0x0)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }

        if ((ret = _rtl9603d_ponMacPirRate_set(physicalQid, HAL_PONMAC_PIR_CIR_RATE_MAX())) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }

        /*set default weight to 1*/
        if ((ret = _rtl9603d_ponMac_wfqWeight_set(physicalQid, 0x1)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }

    /* set OMCI MPCP priority to 7 */
    wData = 7;
    if ((ret = reg_field_write(RTL9603D_PON_TRAP_CFGr, RTL9603D_OMCI_MPCP_PRIORITYf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

#if !defined(FPGA_DEFINED)
    /* Initialize switch PBO */
    initParam.pageSize = PBO_PAGE_SIZE_128;
    initParam.pageCount = PBO_PAGE_COUNT_4096;
    if((ret = dal_rtl9603d_swPbo_init(initParam)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
#endif

    wData = 1;
    if ((ret = reg_field_write(RTL9603D_DYNGASP_CTRLr, RTL9603D_DYNGASP_CMP_INVf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    ponmac_init = INIT_COMPLETED;

    return RT_ERR_OK;
} /* end of dal_rtl9603d_ponmac_init */


/* Function Name:
 *      dal_rtl9603d_ponmac_queue_add
 * Description:
 *      Add queue to given scheduler id and apply queue setting
 * Input:
 *      pQueue         - queue id and scheduler id for ths queue.
 *      pQueueCfg     - queue configuration
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                               - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_NULL_POINTER                     - Pointer pQueueList/pQueue point to NULL.
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_rtl9603d_ponmac_queue_add(rtk_ponmac_queue_t *pQueue, rtk_ponmac_queueCfg_t *pQueueCfg)
{
    int32   ret;
    uint32  qMask;
    uint32  physicalQid;
    uint32  sid;
    rtk_ponmac_queue_t tmpQ;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pQueue), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pQueueCfg), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pQueue->schedulerId >= HAL_MAX_NUM_OF_GPON_TCONT()), RT_ERR_INPUT);
    RT_PARAM_CHK((pQueue->queueId >= RTL9603D_TCONT_QUEUE_MAX), RT_ERR_INPUT);

    RT_PARAM_CHK((pQueueCfg->type >= QUEUE_TYPE_END), RT_ERR_INPUT);
    RT_PARAM_CHK((pQueueCfg->egrssDrop >= RTK_ENABLE_END), RT_ERR_INPUT);

    /* tcont 15 queue 7 can not be added, because omci use queue 127 */
    if(pQueue->schedulerId == 15 && pQueue->queueId == 7)
    {
        return RT_ERR_INPUT;
    }

    /* add queue to t-cont schedule mask*/
    if ((ret = _rtl9603d_ponMacScheQmap_get(pQueue->schedulerId, &qMask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if(qMask == 0)
    {/*first queue add to this t-cont enable t-cont schedule*/
        if ((ret = _rtl9603d_ponMacTcontEnable_set(pQueue->schedulerId, ENABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }

    /*set Queue id and flow id mapping */
    _ponmac_physicalQueueId_get(pQueue->schedulerId, pQueue->queueId, &physicalQid);

    /* drain out queue before add to tcont */
    if ((ret = rtl9603d_ponMacQueueDrainOutState_set(physicalQid)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*add queue to qMask*/
    qMask = qMask | (1<<pQueue->queueId);
    if ((ret = _rtl9603d_ponMacScheQmap_set(pQueue->schedulerId, qMask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    /*set queue schedule type*/
    if(pQueueCfg->type == STRICT_PRIORITY)
    {
        if ((ret = _rtl9603d_ponMac_queueType_set(physicalQid, STRICT_PRIORITY)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
        /* for safe, set weight to 1 when strict */
        if ((ret = _rtl9603d_ponMac_wfqWeight_set(physicalQid, 1)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }
    else
    {
        /*set wfq weight*/
        if ((ret = _rtl9603d_ponMac_wfqWeight_set(physicalQid, pQueueCfg->weight)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
        if ((ret = _rtl9603d_ponMac_queueType_set(physicalQid, WFQ_WRR_PRIORITY)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }


    /*set PIR CIR*/
    if ((ret = _rtl9603d_ponMacCirRate_set(physicalQid, pQueueCfg->cir)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if ((ret = _rtl9603d_ponMacPirRate_set(physicalQid, pQueueCfg->pir)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    
    
    /* change sid to valid after queue add */
    for(sid=0; sid<HAL_CLASSIFY_SID_NUM(); sid++)
    {
        if((ret = dal_rtl9603d_ponmac_flow2Queue_get(sid, &tmpQ))==RT_ERR_OK)
        {
            if((tmpQ.schedulerId == pQueue->schedulerId) && (tmpQ.queueId == pQueue->queueId))
            {
                
                if((ret = dal_rtl9603d_ponmac_sidValid_set(sid, 1))!=RT_ERR_OK)
                    osal_printf("set to sidvalid fail! ret=0x%x\n",ret);
            }
        }
        else
            osal_printf("get to sid2queue fail! ret=0x%x\n",ret);
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_ponmac_queue_add */



/* Function Name:
 *      dal_rtl9603d_ponmac_queue_get
 * Description:
 *      get queue setting
 * Input:
 *      pQueue         - queue id and scheduler id for ths queue.
 * Output:
 *      pQueueCfg     - queue configuration
 * Return:
 *      RT_ERR_OK                               - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_NULL_POINTER                     - Pointer pQueueList/pQueue point to NULL.
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_rtl9603d_ponmac_queue_get(rtk_ponmac_queue_t *pQueue, rtk_ponmac_queueCfg_t *pQueueCfg)
{
    int32   ret;
    rtk_enable_t   enable;
    uint32  qMask;
    uint32  physicalQid;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pQueue), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pQueueCfg), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pQueue->schedulerId >= HAL_MAX_NUM_OF_GPON_TCONT()), RT_ERR_INPUT);
    RT_PARAM_CHK((pQueue->queueId >= RTL9603D_TCONT_QUEUE_MAX), RT_ERR_INPUT);

    if ((ret = _rtl9603d_ponMacTcontEnable_get(pQueue->schedulerId, &enable)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if(DISABLED==enable)
        return RT_ERR_ENTRY_NOTFOUND;

    if ((ret = _rtl9603d_ponMacScheQmap_get(pQueue->schedulerId, &qMask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if(!(qMask & 1<<(pQueue->queueId)))
        return RT_ERR_ENTRY_NOTFOUND;

    _ponmac_physicalQueueId_get(pQueue->schedulerId, pQueue->queueId, &physicalQid);

    if ((ret = _rtl9603d_ponMac_queueType_get(physicalQid, &(pQueueCfg->type))) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*set wfq weight*/
    if ((ret = _rtl9603d_ponMac_wfqWeight_get(physicalQid, &(pQueueCfg->weight))) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*set PIR CIR*/
    if ((ret = _rtl9603d_ponMacCirRate_get(physicalQid, &(pQueueCfg->cir))) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if ((ret = _rtl9603d_ponMacPirRate_get(physicalQid, &(pQueueCfg->pir))) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }


    /*get egress drop*/
    pQueueCfg->egrssDrop = DISABLED;


    return RT_ERR_OK;
} /* end of dal_rtl9603d_ponmac_queue_get */


/* Function Name:
 *      dal_rtl9603d_ponmac_queue_del
 * Description:
 *      delete queue from given scheduler id
 * Input:
 *      pQueue         - queue id and scheduler id for ths queue.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                               - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_NULL_POINTER                     - Pointer pQueue point to NULL.
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_rtl9603d_ponmac_queue_del(rtk_ponmac_queue_t *pQueue)
{
    int32   ret;
    uint32  qMask;
    rtk_enable_t   enable;
    uint32  queueId;
    uint32 sid;
    rtk_ponmac_queue_t tmpQ;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);
    
    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pQueue), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pQueue->schedulerId >= HAL_MAX_NUM_OF_GPON_TCONT()), RT_ERR_INPUT);
    RT_PARAM_CHK((pQueue->queueId >= RTL9603D_TCONT_QUEUE_MAX), RT_ERR_INPUT);

    if ((ret = _rtl9603d_ponMacTcontEnable_get(pQueue->schedulerId, &enable)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if(DISABLED==enable)
        return RT_ERR_ENTRY_NOTFOUND;

    if ((ret = _rtl9603d_ponMacScheQmap_get(pQueue->schedulerId, &qMask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if(!(qMask & 1<<(pQueue->queueId)))
        return RT_ERR_ENTRY_NOTFOUND;

    /* change sid to invalid before drain out */
    for(sid=0; sid<HAL_CLASSIFY_SID_NUM(); sid++)
    {
        if((ret = dal_rtl9603d_ponmac_flow2Queue_get(sid, &tmpQ))==RT_ERR_OK)
        {
            if((tmpQ.schedulerId == pQueue->schedulerId) && (tmpQ.queueId == pQueue->queueId))
            {
                if((ret = dal_rtl9603d_ponmac_sidValid_set(sid, 0))!=RT_ERR_OK)
                    osal_printf("set to sidvalid fail! ret=0x%x\n",ret);
            }
        }
        else
            osal_printf("get to sid2queue fail! ret=0x%x\n",ret);
    }

    /*drant out queue*/
    _ponmac_physicalQueueId_get(pQueue->schedulerId, pQueue->queueId, &queueId);
    if ((ret = rtl9603d_ponMacQueueDrainOutState_set(queueId)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    /*remove queue from qMask*/
    qMask = qMask & (~(1<<pQueue->queueId));
    if ((ret = _rtl9603d_ponMacScheQmap_set(pQueue->schedulerId, qMask)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if(0 == qMask)
    {/*no queue map to this t-cont disable schedule for this t-cont*/
        if ((ret = _rtl9603d_ponMacTcontEnable_set(pQueue->schedulerId, DISABLED)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_ponmac_queue_del */



/* Function Name:
 *      dal_rtl9603d_ponmac_flow2Queue_set
 * Description:
 *      mapping flow to given queue
 * Input:
 *      flow          - flow id.
 *      pQueue        - queue id.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                               - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_NULL_POINTER                     - Pointer pQueue point to NULL.
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_rtl9603d_ponmac_flow2Queue_set(uint32  flow, rtk_ponmac_queue_t *pQueue)
{
    int32   ret;
    uint32  physicalQid;


    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((HAL_CLASSIFY_SID_NUM() <= flow), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pQueue), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pQueue->schedulerId >= HAL_MAX_NUM_OF_GPON_TCONT()), RT_ERR_INPUT);
    RT_PARAM_CHK((pQueue->queueId >= RTL9603D_TCONT_QUEUE_MAX), RT_ERR_INPUT);

    if(flow == RTL9603D_GPON_OMCI_FLOW_ID)
    {
#if defined(FPGA_DEFINED)
        _ponmac_physicalQueueId_get(pQueue->schedulerId, pQueue->queueId, &physicalQid);
#else
        physicalQid = 127;
#endif
    }
    else
    {
        _ponmac_physicalQueueId_get(pQueue->schedulerId, pQueue->queueId, &physicalQid);
    }

    if(physicalQid == 127 && flow != RTL9603D_GPON_OMCI_FLOW_ID)
    {
        return RT_ERR_INPUT;
    }

    if ((ret = _rtl9603d_ponMacSidToQueueMap_set(flow, physicalQid)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_ponmac_flow2Queue_set */


/* Function Name:
 *      dal_rtl9603d_ponmac_flow2Queue_get
 * Description:
 *      get queue id for this flow
 * Input:
 *      flow         - flow id.
 * Output:
 *      pQueue       - queue id.
 * Return:
 *      RT_ERR_OK                               - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_NULL_POINTER                     - Pointer pQueue point to NULL.
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_rtl9603d_ponmac_flow2Queue_get(uint32  flow, rtk_ponmac_queue_t *pQueue)
{
    int32   ret;
    uint32  physicalQid;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((HAL_CLASSIFY_SID_NUM() <= flow), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pQueue), RT_ERR_NULL_POINTER);

    if ((ret = _rtl9603d_ponMacSidToQueueMap_get(flow, &physicalQid)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if ((ret = _ponmac_schedulerQueue_get(physicalQid, &(pQueue->schedulerId), &(pQueue->queueId))) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    if(flow == RTL9603D_GPON_OMCI_FLOW_ID)
    {
#if defined(FPGA_DEFINED)
#else
        pQueue->schedulerId = RTL9603D_GPON_OMCI_TCONT_ID;
        pQueue->queueId     = RTL9603D_GPON_OMCI_QUEUE_ID;
#endif
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_ponmac_flow2Queue_get */

/* Function Name:
 *      dal_rtl9603d_ponmac_maxPktLen_set
 * Description:
 *      set pon port max packet length
 * Input:
 *      length         - max accept packet length
 *    state          - enable or disable
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                                   - OK
 *      RT_ERR_FAILED                           - Failed
 * Note:
 *      None
 */
int32 dal_rtl9603d_ponmac_maxPktLen_set(uint32 length)
{
    uint32 data;
    int32  ret;

    data = length;
    if ((ret = reg_array_field_write(RTL9603D_ACCEPT_MAX_LEN_CTRLr, HAL_GET_PON_PORT(), REG_ARRAY_INDEX_NONE, RTL9603D_ACCEPT_MAX_LENTHf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

#ifdef CONFIG_TX_POWER_TURN_ON_FEATURE
static int32 _ponmac_txPower_enable(rtk_enable_t enable)
{
    int32  ret;

    /* set TX power GPIO pin to output 0 */
    if ((ret = dal_rtl9603d_gpio_mode_set(CONFIG_TX_POWER_GPO_PIN, GPIO_OUTPUT)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    if ((ret = dal_rtl9603d_gpio_state_set(CONFIG_TX_POWER_GPO_PIN, ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    if(enable == ENABLED)
    {
        if ((ret = dal_rtl9603d_gpio_databit_set(CONFIG_TX_POWER_GPO_PIN,0)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }
    else
    {
        if ((ret = dal_rtl9603d_gpio_databit_set(CONFIG_TX_POWER_GPO_PIN,1)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }
    return RT_ERR_OK;

}
#endif

/* Function Name:
 *      dal_rtl9603d_ponmac_mode_set
 * Description:
 *      set pon mac mode
 * Input:
 *      mode         - pon mode, epon or gpon
 *    state          - enable or disable
 * Output:
 *      pQueue       - queue id.
 * Return:
 *      RT_ERR_OK                                   - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_NULL_POINTER                     - Pointer pQueue point to NULL.
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_rtl9603d_ponmac_mode_set(rtk_ponmac_mode_t mode)
{
    int32 ret;
    uint32 chip, rev, subtype;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    RT_PARAM_CHK((PONMAC_MODE_END <= mode), RT_ERR_INPUT);

    if((ret = dal_rtl9603d_switch_version_get(&chip, &rev, &subtype)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    switch(mode)
    {
    case PONMAC_MODE_GPON:
        switch(rev)
        {
        case CHIP_REV_ID_A:
            if ((ret = _rtl9603d_ponMacGponModeV1_set()) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
            break;
        case CHIP_REV_ID_B:
        default:
            if ((ret = _rtl9603d_ponMacGponModeV2_set()) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
            break;
        }
        break;

    case PONMAC_MODE_EPON:
        switch(rev)
        {
        case CHIP_REV_ID_A:
            if ((ret = _rtl9603d_ponMacEponModeV1_set()) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
            break;
        case CHIP_REV_ID_B:
        default:
            if ((ret = _rtl9603d_ponMacEponModeV2_set()) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
            break;
        }
        break;

    case PONMAC_MODE_FIBER_1G:
    case PONMAC_MODE_FIBER_100M:
    case PONMAC_MODE_FIBER_AUTO:
    case PONMAC_MODE_FIBER_FORCE_1G:
    case PONMAC_MODE_FIBER_FORCE_100M:
        switch(rev)
        {
        case CHIP_REV_ID_A:
            if ((ret = _rtl9603d_ponMacFiberModeV1_set(mode)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
            break;
        case CHIP_REV_ID_B:
        default:
            if ((ret = _rtl9603d_ponMacFiberModeV2_set(mode)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
            break;
        }
        break;

    default:
        break;
    }

#ifdef CONFIG_TX_POWER_TURN_ON_FEATURE
    /* turn on tx power */
    if((ret =_ponmac_txPower_enable(ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC | MOD_DAL), "");
        return ret;
    }
#endif

    ponMode = mode;
    return RT_ERR_OK;
}


/* Function Name:
 *      dal_rtl9603d_ponmac_mode_get
 * Description:
 *      set pon mac mode
 * Input:
 *      mode         - pon mode, epon or gpon
 * Output:
 *      *pState       - mode state
 * Return:
 *      RT_ERR_OK                                   - OK
 *      RT_ERR_FAILED                           - Failed
 *      RT_ERR_NULL_POINTER                     - Pointer pQueue point to NULL.
 *      RT_ERR_INPUT                            - Invalid input parameters.
 * Note:
 *      None
 */
int32
dal_rtl9603d_ponmac_mode_get(rtk_ponmac_mode_t *pMode)
{
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    RT_PARAM_CHK((NULL == pMode), RT_ERR_NULL_POINTER);

    *pMode=ponMode;

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_rtl9603d_ponmac_queueDrainOut_set
 * Description:
 *      Set pon queue drain out.
 * Input:
 *      pQueue - Specified which PON queue will be drain out
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_queueDrainOut_set(rtk_ponmac_queue_t *pQueue)
{
    uint32 queueId;
    int32  ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "");

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pQueue), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((pQueue->schedulerId >= HAL_MAX_NUM_OF_GPON_TCONT()), RT_ERR_INPUT);
    RT_PARAM_CHK((pQueue->queueId > RTL9603D_TCONT_QUEUE_MAX), RT_ERR_INPUT);

    /* function body */
    if(pQueue->queueId < RTL9603D_TCONT_QUEUE_MAX)
    {
        _ponmac_physicalQueueId_get(pQueue->schedulerId, pQueue->queueId, &queueId);
        if ((ret = rtl9603d_ponMacQueueDrainOutState_set(queueId)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }
    else
    {
        if ((ret = _rtl9603d_ponMacTcontDrainOutState_set(pQueue->schedulerId)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
            return ret;
        }
    }

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_queueDrainOut_set */

/* Function Name:
 *      dal_rtl9603d_ponmac_opticalPolarity_get
 * Description:
 *      Get the current optical output polarity
 * Input:
 *      None
 * Output:
 *      pPolarity  - the current output polarity
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 */
int32
dal_rtl9603d_ponmac_opticalPolarity_get(rtk_ponmac_polarity_t *pPolarity)
{
    int32  ret;
    uint32 val;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "");

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pPolarity), RT_ERR_NULL_POINTER);

    /* function body */
    if ((ret = reg_field_read(RTL9603D_WSDS_DIG_12r,RTL9603D_CFG_FRC_BEN_INVf,&val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if(0 == val)
    {
        *pPolarity = PONMAC_POLARITY_HIGH;
    }
    else
    {
        *pPolarity = PONMAC_POLARITY_LOW;
    }

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_opticalPolarity_get */

/* Function Name:
 *      dal_rtl9603d_ponmac_opticalPolarity_set
 * Description:
 *      Set optical output polarity
 * Input:
 *      polarity - the optical output polarity
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_opticalPolarity_set(rtk_ponmac_polarity_t polarity)
{
    int32  ret;
    uint32 val;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "");

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((PONMAC_POLARITY_END <= polarity), RT_ERR_INPUT);

    /* function body */
    if(PONMAC_POLARITY_HIGH == polarity)
    {
        val = 0;
    }
    else
    {
        val = 1;
    }

    if ((ret = reg_field_write(RTL9603D_WSDS_DIG_12r,RTL9603D_CFG_FRC_BEN_INVf,&val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_opticalPolarity_set */

/* Function Name:
 *      dal_rtl9603d_ponmac_losState_get
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
dal_rtl9603d_ponmac_losState_get(rtk_enable_t *pState)
{
    uint32  tmpVal;
    int32   ret;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "");

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pState), RT_ERR_NULL_POINTER);

    /* function body */
    if ((ret = reg_field_read(RTL9603D_SDS_EXT_REG29r, RTL9603D_SEP_SIGNOK_REALf, &tmpVal)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    if(1 == tmpVal)
        *pState = DISABLED;
    else
        *pState = ENABLED;

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_losState_get */

/* Function Name:
 *      dal_rtl9603d_ponmac_serdesCdr_reset
 * Description:
 *      Serdes CDR reset
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_serdesCdr_reset(void)
{
    uint32 rdata, wdata;
    int32 ret;

    if ((ret = reg_read(RTL9603D_SDS_ANA_COM_REG09r, &rdata)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    wdata = (rdata&(~0x400))|((!((rdata&0x400)>>10))<<10);
    if ((ret = reg_write(RTL9603D_SDS_ANA_COM_REG09r, &wdata)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    osal_time_usleep(10000);

    wdata = rdata;
    if ((ret = reg_write(RTL9603D_SDS_ANA_COM_REG09r, &wdata)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_serdesCdr_reset */

/* Function Name:
 *      dal_rtl9603d_ponmac_linkState_get
 * Description:
 *      check SD and Sync state of GPON/EPON
 * Input:
 *      mode - GPON or EPON mode
 * Output:
 *      pSd     - pointer of signal detect
 *      pSync   - pointer of sync state
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_linkState_get(rtk_ponmac_mode_t mode, uint32 *pSd, uint32 *pSync)
{
    uint32 val, val1;
    uint32 data;
    int32 ret;

    switch(mode)
    {
        case PONMAC_MODE_GPON:
            if ((ret = reg_read(RTL9603D_GPON_GTC_DS_INTR_STSr,&data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_EPON|MOD_DAL), "");
                return ret;
            }
            /* if read data == 0xca0eca0f, then reset cdr, and read data again */
            if(data == 0xca0eca0f) 
            {
                val = 0;
                if ((ret = reg_field_write(RTL9603D_SDS_REG0r,RTL9603D_SP_SDS_EN_RXf,&val)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_EPON|MOD_DAL), "");
                    return ret;
                }
                val = 1;
                if ((ret = reg_field_write(RTL9603D_SDS_REG0r,RTL9603D_SP_SDS_EN_RXf,&val)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_EPON|MOD_DAL), "");
                    return ret;
                }
                osal_time_usleep(10*1000);
            
                if ((ret = reg_read(RTL9603D_GPON_GTC_DS_INTR_STSr,&data)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_EPON|MOD_DAL), "");
                    return ret;
                }
            }
            if ((ret = reg_field_get(RTL9603D_GPON_GTC_DS_INTR_STSr,RTL9603D_LOSf,&val,&data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_EPON|MOD_DAL), "");
                return ret;
            }
            *pSd = !val;
            if ((ret = reg_field_get(RTL9603D_GPON_GTC_DS_INTR_STSr,RTL9603D_LOFf,&val,&data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_EPON|MOD_DAL), "");
                return ret;
            }
            if ((ret = reg_field_get(RTL9603D_GPON_GTC_DS_INTR_STSr,RTL9603D_LOMf,&val1,&data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_EPON|MOD_DAL), "");
                return ret;
            }
            *pSync = !(val|val1);
            break;

        case PONMAC_MODE_EPON:
            if ((ret = reg_field_read(RTL9603D_SDS_EXT_REG29r, RTL9603D_SEP_SIGNOK_REALf, pSd)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }

            if ((ret = reg_field_read(RTL9603D_SDS_EXT_REG29r, RTL9603D_SEP_LINKOK_REALf, &val)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
            *pSync = val&0x1;
            break;

        default:
            return RT_ERR_INPUT;
            break;
    }
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_linkState_get */

/* Function Name:
 *      dal_rtl9603d_ponmac_sidValid_get
 * Description:
 *      Get sid valid
 * Input:
 *      sid     - sid
 * Output:
 *      pValid  - the current sid valid
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 */
int32
dal_rtl9603d_ponmac_sidValid_get(uint32 sid, rtk_enable_t *pValid)
{
    int32  ret;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "");

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_GPON_FLOW() <= sid), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pValid), RT_ERR_NULL_POINTER);

    /* function body */
    if ((ret = reg_array_field_read(RTL9603D_PON_SIDVALIDr,REG_ARRAY_INDEX_NONE, sid, RTL9603D_CFG_SID2VALIDf, pValid)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_sidValid_get */

/* Function Name:
 *      dal_rtl9603d_ponmac_sidValid_set
 * Description:
 *      Set sid valit
 * Input:
 *      sid     - sid
 *      valid   - valid or not
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_sidValid_set(uint32 sid, rtk_enable_t valid)
{
    int32  ret;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "");

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((HAL_MAX_NUM_OF_GPON_FLOW() <= sid), RT_ERR_INPUT);
    RT_PARAM_CHK((RTK_ENABLE_END <= valid), RT_ERR_INPUT);

    /* function body */

    if ((ret = reg_array_field_write(RTL9603D_PON_SIDVALIDr,REG_ARRAY_INDEX_NONE, sid, RTL9603D_CFG_SID2VALIDf, &valid)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    
    
#if 0
    /*for epon mode must mapping sid 64~127 to 0~63*/
    if(PONMAC_MODE_EPON == ponMode)
    {
        if(sid<64)
        {
            sid = sid + 64;
            if ((ret = reg_array_field_write(RTL9603D_PON_SIDVALIDr,REG_ARRAY_INDEX_NONE, sid, RTL9603D_CFG_SID2VALIDf, &valid)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        }
    }    
#endif
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_sidValid_set */


/* Function Name:
 *      dal_rtl9603d_ponmac_schedulingType_get
 * Description:
 *      Get the scheduling types and weights of queues on specific port in egress scheduling.
 * Input:
 *      None
 * Output:
 *      pQueueType  - the WFQ schedule type (DAL_RTL9603D_QOS_WFQ or DAL_RTL9603D_QOS_WRR)
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 */
int32
dal_rtl9603d_ponmac_schedulingType_get(rtk_qos_scheduling_type_t *pQueueType)
{
    uint32  val;
    int32 ret;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "");

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pQueueType), RT_ERR_NULL_POINTER);

    /* function body */

    if ((ret = reg_field_read(RTL9603D_PON_SCH_CTRLr, RTL9603D_PON_WFQ_MODEf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PONMAC), "");
        return ret;
    }
    if(0==val)
        *pQueueType=RTK_QOS_WFQ;
    else
        *pQueueType=RTK_QOS_WRR;



    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_schedulingType_get */


/* Function Name:
 *      dal_rtl9603d_ponmac_schedulingType_set
 * Description:
 *      Get the scheduling types and weights of queues on specific port in egress scheduling.
 * Input:
 *      None
 * Output:
 *      queueType  - the WFQ schedule type (DAL_RTL9603D_QOS_WFQ or DAL_RTL9603D_QOS_WRR)
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 */
int32
dal_rtl9603d_ponmac_schedulingType_set(rtk_qos_scheduling_type_t queueType)
{
    uint32  val;
    int32 ret;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "queueType=%d",queueType);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((SCHEDULING_TYPE_END <=queueType), RT_ERR_INPUT);

    /* function body */
    if(RTK_QOS_WFQ==queueType)
        val=0;
    else
        val=1;
        
    if ((ret = reg_field_write(RTL9603D_PON_SCH_CTRLr, RTL9603D_PON_WFQ_MODEf, &val)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PONMAC), "");
        return ret;
    }        
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_schedulingType_set */


/* Function Name:
 *      dal_rtl9603d_ponmac_egrBandwidthCtrlRate_get
 * Description:
 *      Get the egress bandwidth control rate.
 * Input:
 *      None
 * Output:
 *      pRate - egress bandwidth control rate
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - Invalid port id
 *      RT_ERR_NULL_POINTER - NULL pointer
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in Apollo is 8Kbps.
 */
int32
dal_rtl9603d_ponmac_egrBandwidthCtrlRate_get(uint32 *pRate)
{
    uint32 data;
    int32 ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "");

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pRate), RT_ERR_NULL_POINTER);

    /* function body */
    if ((ret = reg_field_read(RTL9603D_PON_EGR_RATEr, RTL9603D_RATEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    *pRate = (data << 3);
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_egrBandwidthCtrlRate_get */

/* Function Name:
 *      dal_rtl9603d_ponmac_egrBandwidthCtrlRate_set
 * Description:
 *      Set the pon port egress bandwidth control rate.
 * Input:
 *      rate - egress bandwidth control rate
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - Invalid port id
 *      RT_ERR_RATE    - Invalid input rate
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in rtl9603d is 8Kbps.
 */
int32
dal_rtl9603d_ponmac_egrBandwidthCtrlRate_set(uint32 rate)
{
    uint32 data;
    int32 ret;
    
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "rate=%d",rate);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((RTL9603D_PON_EGRESS_RATE_MAX < rate), RT_ERR_INPUT);

    /* function body */
    data = (rate >> 3);
    if ((ret = reg_field_write(RTL9603D_PON_EGR_RATEr, RTL9603D_RATEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_egrBandwidthCtrlRate_set */


/* Function Name:
 *      dal_rtl9603d_ponmac_egrBandwidthCtrlRate_get
 * Description:
 *      Set the pon port egress bandwidth control rate.
 * Input:
 *      scheduleId - T-cont is or LLID index
 * Output:
 *      pRate - egress bandwidth control rate
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - Invalid port id
 *      RT_ERR_RATE    - Invalid input rate
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in Apollo is 8Kbps.
 */
int32
dal_rtl9603d_ponmac_egrScheduleIdRate_get(uint32 scheduleId,uint32 *pRate)
{
    int32 ret;
    uint32 data;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "scheduleId=%d",scheduleId);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((RTL9603D_GPON_TCONT_MAX <=scheduleId), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pRate), RT_ERR_NULL_POINTER);

    /* function body */
    if ((ret = reg_array_field_read(RTL9603D_PON_SCH_RATEr,REG_ARRAY_INDEX_NONE, scheduleId, RTL9603D_RATEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    *pRate = (data << 3);
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_egrScheduleIdRate_get */


/* Function Name:
 *      dal_rtl9603d_ponmac_egrBandwidthCtrlRate_set
 * Description:
 *      Set the pon port egress bandwidth control rate.
 * Input:
 *      scheduleId - T-cont is or LLID index
 *      rate - egress bandwidth control rate
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - Invalid port id
 *      RT_ERR_RATE    - Invalid input rate
 * Note:
 *      (1) The actual rate is "rate * chip granularity".
 *      (2) The unit of granularity in Apollo is 8Kbps.
 */
int32
dal_rtl9603d_ponmac_egrScheduleIdRate_set(uint32 scheduleId,uint32 rate)
{
    int32 ret;
    uint32 data;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_PONMAC), "scheduleId=%d,rate=%d",scheduleId, rate);

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((RTL9603D_GPON_TCONT_MAX <=scheduleId), RT_ERR_INPUT);
    RT_PARAM_CHK((RTL9603D_PON_PIR_CIR_RATE_MAX < rate), RT_ERR_INPUT);

    data = (rate >> 3);
    /* function body */
    if ((ret = reg_array_field_write(RTL9603D_PON_SCH_RATEr,REG_ARRAY_INDEX_NONE, scheduleId, RTL9603D_RATEf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_egrScheduleIdRate_set */


/* Function Name:
 *      dal_rtl9603d_ponmac_egrBandwidthCtrlIncludeIfg_get
 * Description:
 *      get the pon port egress bandwidth control IFG state.
 * Input:
 *      None
 * Output:
 *      pIfgInclude - pointer of state of IFG 
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_egrBandwidthCtrlIncludeIfg_get(rtk_enable_t *pIfgInclude)
{
    int32 ret;
    uint32 data;

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pIfgInclude), RT_ERR_NULL_POINTER);

    /* function body */
    if ((ret = reg_field_read(RTL9603D_US_SCH_EGR_IFGr, RTL9603D_PON_EGR_IFGf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    if(0 == data)
        *pIfgInclude = DISABLED;
    else
        *pIfgInclude = ENABLED;

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_egrBandwidthCtrlIncludeIfg_get */


/* Function Name:
 *      dal_rtl9603d_ponmac_egrBandwidthCtrlIncludeIfg_set
 * Description:
 *      set the pon port egress bandwidth control IFG state.
 * Input:
 *      ifgInclude - state of IFG 
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_egrBandwidthCtrlIncludeIfg_set(rtk_enable_t ifgInclude)
{
    int32 ret;
    uint32 data;

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((RTK_ENABLE_END <= ifgInclude), RT_ERR_INPUT);

    if(ENABLED==ifgInclude)
        data = 1;
    else
        data = 0;

    /* function body */
    if ((ret = reg_field_write(RTL9603D_US_SCH_EGR_IFGr, RTL9603D_PON_EGR_IFGf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_egrBandwidthCtrlIncludeIfg_set */


/* Function Name:
 *      dal_rtl9603d_ponmac_egrScheduleIdIncludeIfg_get
 * Description:
 *      get the tcont egress bandwidth control IFG state.
 * Input:
 *      None
 * Output:
 *      pIfgInclude - pointer of state of IFG 
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_egrScheduleIdIncludeIfg_get(rtk_enable_t *pIfgInclude)
{
    int32 ret;
    uint32 data;

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pIfgInclude), RT_ERR_NULL_POINTER);

    /* function body */
    if ((ret = reg_field_read(RTL9603D_PON_SCH_CTRLr, RTL9603D_PON_WFQ_IFGf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    
    if(0 == data)
        *pIfgInclude = DISABLED;
    else
        *pIfgInclude = ENABLED;

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_egrScheduleIdIncludeIfg_get */


/* Function Name:
 *      dal_rtl9603d_ponmac_egrScheduleIdIncludeIfg_set
 * Description:
 *      set the tcont egress bandwidth control IFG state.
 * Input:
 *      ifgInclude - state of IFG 
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_egrScheduleIdIncludeIfg_set(rtk_enable_t ifgInclude)
{
    int32 ret;
    uint32 data;

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((RTK_ENABLE_END <= ifgInclude), RT_ERR_INPUT);

    if(ENABLED==ifgInclude)
        data = 1;
    else
        data = 0;

    /* function body */
    if ((ret = reg_field_write(RTL9603D_PON_SCH_CTRLr, RTL9603D_PON_WFQ_IFGf, &data)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
        return ret;
    }
    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_egrScheduleIdIncludeIfg_set */

/* Function Name:
 *      dal_rtl9603d_ponmac_aggTcontState_get
 * Description:
 *      get the aggregate tcont state.
 * Input:
 *      aggIndex  - aggregate tcont index
 * Output:
 *      pState - pointer of state of aggregate tcont 
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_aggTcontState_get(rtk_ponmac_aggregate_tcont_t aggIndex, rtk_enable_t *pState)
{
    int32 ret;
    uint32 data;

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pState), RT_ERR_NULL_POINTER);

    /* function body */
    *pState = DISABLED;

    switch(aggIndex)
    {
        case PONMAC_AGG_TCONT0_3:
            if ((ret = reg_field_read(RTL9603D_PON_SCH_ASIC_OPTr, RTL9603D_TCON00_32Qf, &data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        break;
        case PONMAC_AGG_TCONT4_7:
            if ((ret = reg_field_read(RTL9603D_PON_SCH_ASIC_OPTr, RTL9603D_TCON04_32Qf, &data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        break;
        case PONMAC_AGG_TCONT8_11:
            if ((ret = reg_field_read(RTL9603D_PON_SCH_ASIC_OPTr, RTL9603D_TCON08_32Qf, &data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        break;
        case PONMAC_AGG_TCONT12_15:
            if ((ret = reg_field_read(RTL9603D_PON_SCH_ASIC_OPTr, RTL9603D_TCON12_32Qf, &data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        break;
        default:
            return RT_ERR_OUT_OF_RANGE;
    }
    if(data == 1)
    {
        *pState = ENABLED;
    }
    else
    {
        *pState = DISABLED;
    }

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_aggTcontState_get */


/* Function Name:
 *      dal_rtl9603d_ponmac_aggTcontState_set
 * Description:
 *      set the aggregate tcont state.
 * Input:
 *      aggIndex - aggregate tcont index
 *      state - state of aggregate tcont
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 */
int32
dal_rtl9603d_ponmac_aggTcontState_set(rtk_ponmac_aggregate_tcont_t aggIndex, rtk_enable_t state)
{
    int32 ret;
    uint32 data;

    /* check Init status */
    RT_INIT_CHK(ponmac_init);

    /* parameter check */
    RT_PARAM_CHK((RTK_ENABLE_END <= state), RT_ERR_INPUT);

    if(ENABLED==state)
        data = 1;
    else
        data = 0;

    /* function body */
    switch(aggIndex)
    {
        case PONMAC_AGG_TCONT0_3:
            if ((ret = reg_field_write(RTL9603D_PON_SCH_ASIC_OPTr, RTL9603D_TCON00_32Qf, &data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        break;
        case PONMAC_AGG_TCONT4_7:
            if ((ret = reg_field_write(RTL9603D_PON_SCH_ASIC_OPTr, RTL9603D_TCON04_32Qf, &data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        break;
        case PONMAC_AGG_TCONT8_11:
            if ((ret = reg_field_write(RTL9603D_PON_SCH_ASIC_OPTr, RTL9603D_TCON08_32Qf, &data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        break;
        case PONMAC_AGG_TCONT12_15:
            if ((ret = reg_field_write(RTL9603D_PON_SCH_ASIC_OPTr, RTL9603D_TCON12_32Qf, &data)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_PONMAC|MOD_DAL), "");
                return ret;
            }
        break;
        default:
            return RT_ERR_OUT_OF_RANGE;
    }

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_ponmac_aggTcontState_set */
