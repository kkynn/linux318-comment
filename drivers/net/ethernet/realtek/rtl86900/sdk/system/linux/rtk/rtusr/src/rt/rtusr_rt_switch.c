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
 * Purpose : Definition of Switch Global API
 *
 * Feature : The file have include the following module and sub-modules
 *           (1) Switch parameter settings
 *           (2) Management address and vlan configuration.
 *
 */

/*
 * Include Files
 */
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <osal/lib.h>
#include <rtk/rtusr/include/rtusr_util.h>
#include <rtdrv/rtdrv_netfilter.h>
#include <common/rt_type.h>
#include <hal/chipdef/chip.h>
#include <rtk/switch.h>

/*
 * Function Declaration
 */

/* Module Name    : Switch     */
/* Sub-module Name: Switch parameter settings */

/* Function Name:
 *      rt_switch_init
 * Description:
 *      Initialize switch module of the specified device.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Module must be initialized before using all of APIs in this module
 */
int32
rt_switch_init(void)
{
    rtdrv_rt_switchCfg_t rt_switch_cfg;

    /* function body */
    SETSOCKOPT(RTDRV_RT_SWITCH_INIT, &rt_switch_cfg, rtdrv_rt_switchCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_switch_init */

/* Function Name:
 *      rt_switch_phyPortId_get
 * Description:
 *      Get physical port id from logical port name
 * Input:
 *      portName - logical port name
 * Output:
 *      pPortId  - pointer to the physical port id
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Call RTK API the port ID must get from this API
 */
int32
rt_switch_phyPortId_get(rtk_switch_port_name_t portName, int32 *pPortId)
{
    rtdrv_rt_switchCfg_t rt_switch_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pPortId), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&rt_switch_cfg.portName, &portName, sizeof(rtk_switch_port_name_t));
    GETSOCKOPT(RTDRV_RT_SWITCH_PHYPORTID_GET, &rt_switch_cfg, rtdrv_rt_switchCfg_t, 1);
    osal_memcpy(pPortId, &rt_switch_cfg.portId, sizeof(int32));

    return RT_ERR_OK;
}   /* end of rt_switch_phyPortId_get */

/* Function Name:
 *      rt_switch_version_get
 * Description:
 *      Get chip version
 * Input:
 *      pChipId    - chip id
 *      pRev       - revision id
 *      pSubtype   - sub type
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 */
int32
rt_switch_version_get(uint32 *pChipId, uint32 *pRev, uint32 *pSubtype)
{
    rtdrv_rt_switchCfg_t rt_switch_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pChipId), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pRev), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pSubtype), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&rt_switch_cfg.subtype, pSubtype, sizeof(uint32));
    GETSOCKOPT(RTDRV_RT_SWITCH_VERSION_GET, &rt_switch_cfg, rtdrv_rt_switchCfg_t, 1);
    osal_memcpy(pChipId, &rt_switch_cfg.chipId, sizeof(uint32));
    osal_memcpy(pRev, &rt_switch_cfg.rev, sizeof(uint32));
    osal_memcpy(pSubtype, &rt_switch_cfg.subtype, sizeof(uint32));

    return RT_ERR_OK;
}   /* end of rt_switch_version_get */

/* Function Name:
  *      rt_switch_maxPktLenByPort_get
  * Description:
  *      Get the max packet length setting of specific port
  * Input:
  *      port - speed type
  * Output:
  *      pLen - pointer to the max packet length
  * Return:
  *      RT_ERR_OK
  *      RT_ERR_FAILED
  *      RT_ERR_NULL_POINTER - input parameter may be null pointer
  *      RT_ERR_INPUT        - invalid enum speed type
  * Note:
  */
int32
rt_switch_maxPktLenByPort_get(rtk_port_t port, uint32 *pLen)
{
    rtdrv_rt_switchCfg_t rt_switch_cfg;

    /* parameter check */
    RT_PARAM_CHK((NULL == pLen), RT_ERR_NULL_POINTER);

    /* function body */
    osal_memcpy(&rt_switch_cfg.port, &port, sizeof(rtk_port_t));
    GETSOCKOPT(RTDRV_RT_SWITCH_MAXPKTLENBYPORT_GET, &rt_switch_cfg, rtdrv_rt_switchCfg_t, 1);
    osal_memcpy(pLen, &rt_switch_cfg.len, sizeof(uint32));

    return RT_ERR_OK;
}   /* end of rt_switch_maxPktLenByPort_get */

/* Function Name:
  *      rt_switch_maxPktLenByPort_set
  * Description:
  *      Set the max packet length of specific port
  * Input:
  *      port  - port
  *      len   - max packet length
  * Output:
  *      None
  * Return:
  *      RT_ERR_OK
  *      RT_ERR_FAILED
  *      RT_ERR_INPUT   - invalid enum speed type
  * Note:
  */
int32
rt_switch_maxPktLenByPort_set(rtk_port_t port, uint32 len)
{
    rtdrv_rt_switchCfg_t rt_switch_cfg;

    /* function body */
    osal_memcpy(&rt_switch_cfg.port, &port, sizeof(rtk_port_t));
    osal_memcpy(&rt_switch_cfg.len, &len, sizeof(uint32));
    SETSOCKOPT(RTDRV_RT_SWITCH_MAXPKTLENBYPORT_SET, &rt_switch_cfg, rtdrv_rt_switchCfg_t, 1);

    return RT_ERR_OK;
}   /* end of rt_switch_maxPktLenByPort_set */

