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
#include <common/rt_type.h>
#include <hal/chipdef/chip.h>
#include <rtk/switch.h>
#include <rtk/init.h>
#include <rtk/default.h>
#include <dal/dal_mgmt.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->switch_init)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->switch_init();
    RTK_API_UNLOCK();
    return ret;
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->switch_phyPortId_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->switch_phyPortId_get(portName, pPortId);
    RTK_API_UNLOCK();
    return ret;
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->switch_version_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->switch_version_get(pChipId, pRev, pSubtype);
    RTK_API_UNLOCK();
    return ret;
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->switch_maxPktLenByPort_get)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->switch_maxPktLenByPort_get(port, pLen);
    RTK_API_UNLOCK();
    return ret;
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
    int32   ret;

    /* function body */
    if (NULL == RT_MAPPER->switch_maxPktLenByPort_set)
        return RT_ERR_DRIVER_NOT_FOUND;
    RTK_API_LOCK();
    ret = RT_MAPPER->switch_maxPktLenByPort_set(port, len);
    RTK_API_UNLOCK();
    return ret;
}   /* end of rt_switch_maxPktLenByPort_set */
