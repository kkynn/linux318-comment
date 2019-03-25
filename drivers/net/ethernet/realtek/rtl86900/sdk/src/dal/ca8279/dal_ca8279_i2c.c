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
 * $Revision: 60215 $
 * $Date: 2017-03-23 16:58:52 +0800 (Thur, 23 Mar 2017) $
 *
 * Purpose : Definition of I2C API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) I2C control
 *           (2) I2C read/write
 */



/*
 * Include Files
 */

#include <common/rt_type.h>
#include <dal/ca8279/dal_ca8279.h>
#include <dal/ca8279/dal_ca8279_i2c.h>
#include <rtk/i2c.h>

#include <ca-plat-1.0/peri/peri.h>

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */
static uint32 i2c_init = INIT_NOT_COMPLETED;

static rtk_enable_t GBL_I2C_ENABLE = DISABLED;

/*
 * Macro Declaration
 */


/*
 * Function Declaration
 */
/* Function Name:
 *      dal_ca8279_i2c_init
 * Description:
 *      Initialize i2c interface.
 * Input:
 *      i2cPort    - I2C port interface
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Must initialize before calling any other APIs.
 */
int32
dal_ca8279_i2c_init (
    rtk_i2c_port_t i2cPort )
{
    int32 ret=RT_ERR_OK;

    /* Init status */
    i2c_init = INIT_COMPLETED;

    if((ret = dal_ca8279_i2c_enable_set(0,ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_HWMISC), "");
        i2c_init = INIT_NOT_COMPLETED;
        return ret;
    }

#if !defined(CONFIG_BOARD_REALTEK_CA8277_REV_C_CTC)
    if((ret = dal_ca8279_i2c_write(0, 0xffff, 0xe0, 0x04)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_HWMISC), "");
        i2c_init = INIT_NOT_COMPLETED;
        return ret;
    }
#endif

    return RT_ERR_OK;
}   /* end of dal_ca8279_i2c_init */

/* Function Name:
 *      dal_ca8279_i2c_enable_get
 * Description:
 *      Get I2C interface state.
 * Input:
 *      i2cPort   - I2C port interface
 *      pEnable   - the pointer of enable/disable state
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER
 * Note:
 *
 */
int32
dal_ca8279_i2c_enable_get (
    rtk_i2c_port_t i2cPort,
    rtk_enable_t* pEnable )
{
    /* check Init status */
    RT_INIT_CHK(i2c_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pEnable), RT_ERR_NULL_POINTER);

    /* function body */

    *pEnable = GBL_I2C_ENABLE;

    return RT_ERR_OK;
}   /* end of dal_ca8279_i2c_enable_get */

/* Function Name:
 *      dal_ca8279_i2c_enable_set
 * Description:
 *      Enable/Disable I2C interface.
 * Input:
 *      i2cPort    - I2C port interface
 *      enable     - enable/disable state
 * Output:
 *      None
 * Return:
*      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT
 * Note:
 *
 */
int32
dal_ca8279_i2c_enable_set (
    rtk_i2c_port_t i2cPort,
    rtk_enable_t enable )
{
    ca_uint32_t ret = CA_E_OK;
    ca_boolean ca_enable;

    RT_DBG(LOG_DEBUG, (MOD_DAL | MOD_HWMISC), "enable=%d", enable);

    /* check Init status */
    RT_INIT_CHK(i2c_init);

    /* parameter check */
    RT_PARAM_CHK((RTK_ENABLE_END <= enable), RT_ERR_INPUT);

    /* function body */
    if(enable == ENABLED)
        ca_enable = 1;
    else
        ca_enable = 0;

    if((ret = cap_i2c_enable_set(0,ca_enable,100)) != CA_E_OK)
    {
        RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
        return RT_ERR_FAILED;
    }

    GBL_I2C_ENABLE = enable;

    return RT_ERR_OK;
}   /* end of dal_ca8279_i2c_enable_set */

/* Function Name:
 *      dal_ca8279_i2c_write
 * Description:
 *      I2c write data.
 * Input:
 *      i2cPort   - I2C port interface
 *      devID     - the device ID
 *      regAddr   - register address
 *      data      - data value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT
 * Note:
 *
 */
int32
dal_ca8279_i2c_write (
    rtk_i2c_port_t i2cPort,
    uint32 devID,
    uint32 regAddr,
    uint32 data )
{
    ca_uint32_t ret = CA_E_OK;
    ca_uint8_t slave_addr;
    ca_uint8_t slave_offset;
    ca_uint8_t tx_data;

    RT_DBG(LOG_DEBUG, (MOD_DAL | MOD_HWMISC), "devID=%d,regAddr=%d data=0x%x", devID, regAddr ,data);

    /* check Init status */
    RT_INIT_CHK(i2c_init);

    if(GBL_I2C_ENABLE == DISABLED)
    {
        RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
        return RT_ERR_NOT_ALLOWED;
    }

    /* function body */
    if((ret = cap_i2c_enable_set(0,1,100)) != CA_E_OK)
    {
        RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
        return RT_ERR_FAILED;
    }
#if !defined(CONFIG_BOARD_REALTEK_CA8277_REV_C_CTC)
    if(devID == 0xffff)
    {
        slave_addr = regAddr;
        tx_data = data;

        if((ret = cap_i2c_mut_write(0,slave_addr,tx_data)) != CA_E_OK)
        {
            if(ret == CA_E_BUSY)
            {
                cap_i2c_enable_set(0,1,100);
            }
            RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
            return RT_ERR_CHIP_NOT_FOUND;
        }
    }
    else
#endif
    {
        /* CA API require the 8-bits address directly, so shift 1 bit left here */
        slave_addr = (devID << 1);
        slave_offset = regAddr;
        tx_data = data;

        if((ret = cap_i2c_write(0,slave_addr,slave_offset,1,&tx_data)) != CA_E_OK)
        {
            if(ret == CA_E_BUSY)
            {
                cap_i2c_enable_set(0,1,100);
            }
            RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
            return RT_ERR_CHIP_NOT_FOUND;
        }
    }
    return RT_ERR_OK;
}   /* end of dal_ca8279_i2c_write */

/* Function Name:
 *      dal_ca8279_i2c_read
 * Description:
 *      I2c read data.
 * Input:
 *      i2cPort   - I2C port interface
 *      devID     - the device ID
 *      regAddr   - register address
 *      pData     - the pointer of returned data
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT
 *      RT_ERR_NULL_POINTER
 * Note:
 *
 */
int32
dal_ca8279_i2c_read (
    rtk_i2c_port_t i2cPort,
    uint32 devID,
    uint32 regAddr,
    uint32* pData )
{
    ca_uint32_t ret = CA_E_OK;
    ca_uint8_t slave_addr;
    ca_uint8_t slave_offset;
    ca_uint8_t rx_data;

    RT_DBG(LOG_DEBUG, (MOD_DAL | MOD_HWMISC), "devID=%d,regAddr=%d", devID, regAddr);

    /* check Init status */
    RT_INIT_CHK(i2c_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pData), RT_ERR_NULL_POINTER);

    if(GBL_I2C_ENABLE == DISABLED)
    {
        RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
        return RT_ERR_NOT_ALLOWED;
    }

    /* function body */
    if((ret = cap_i2c_enable_set(0,1,100)) != CA_E_OK)
    {
        RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
        return RT_ERR_FAILED;
    }
#if !defined(CONFIG_BOARD_REALTEK_CA8277_REV_C_CTC)
    if(devID == 0xffff)
    {
        slave_addr = regAddr;

        if((ret = cap_i2c_mut_read(0,slave_addr,&rx_data)) != CA_E_OK)
        {
            if(ret == CA_E_BUSY)
            {
                cap_i2c_enable_set(0,1,100);
            }
            RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
            return RT_ERR_CHIP_NOT_FOUND;
        }

        *pData = rx_data;
    }
    else
#endif
    {
        /* CA API require the 8-bits address directly, so shift 1 bit left here */
        slave_addr = (devID << 1);
        slave_offset = regAddr; 

        if((ret = cap_i2c_read(0,slave_addr,slave_offset,1,&rx_data)) != CA_E_OK)
        {
            if(ret == CA_E_BUSY)
            {
                cap_i2c_enable_set(0,1,100);
            }

            RT_ERR(ret, (MOD_HWMISC | MOD_DAL), "");
            return RT_ERR_CHIP_NOT_FOUND;
        }

        *pData = rx_data;
    }

    return RT_ERR_OK;
}   /* end of dal_ca8279_i2c_read */
