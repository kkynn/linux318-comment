
/* This is RTL9602BVB driver files */
/*
 * ## Please DO NOT edit this file!! ##
 * This file is auto-generated from the register source files.
 * Any modifications to this file will be LOST when it is re-generated.
 *
 * ----------------------------------------------------------------
 * Copyright(c) Realtek Semiconductor Corporation, 2009
 * All rights reserved.
 *
 * $Revision: 41741 $
 * $Date: 2013-08-01 21:25:55 +0800 (星期四, 01 八月 2013) $
 *
 * Purpose : The RTL9602BVB chip driver
 *
 * Feature : RTL9602BVB chip driver
 *
 */

/*
 * Include Files
 */
#include <common/rt_autoconf.h>
#include <hal/chipdef/allreg.h>
#include <hal/chipdef/allmem.h>
#include <hal/chipdef/rtl9602bvb/rtk_rtl9602bvb_reg_struct.h>
#include <hal/chipdef/rtl9602bvb/rtk_rtl9602bvb_table_struct.h>
#include <hal/chipdef/chip.h>
#include <hal/chipdef/driver.h>
#include <hal/mac/drv.h>


/* RTL9602BVB mac driver service APIs */
rt_macdrv_t rtl9602bvb_macdrv =
{
    rtl9602bvb_init,          /* fMdrv_init        */
    rtl9602bvb_miim_read,     /* fMdrv_miim_read   */
    rtl9602bvb_miim_write,    /* fMdrv_miim_write  */
    rtl9602bvb_table_read,    /* fMdrv_table_read  */
    rtl9602bvb_table_write,   /* fMdrv_table_write */
    rtl9602bvb_table_clear,   /* fMdrv_table_clear */
}; /* end of rtl9602bvb_macdrv */


/* Definition RTL9601B Rev. A major driver */
rt_driver_t rtl9602bvb_a_driver =
{
    /* type            */ RT_DRIVER_RTL9602BVB,
    /* RTL chip ID     */ RTL9602BVB_CHIP_ID,
    /* RTL revision ID */ CHIP_REV_ID_A,
    /* register list   */ rtk_rtl9602bvb_reg_list,
    /* table list      */ rtk_rtl9602bvb_table_list,
    /* mac driver      */ &rtl9602bvb_macdrv,
    /* reg index max   */ RTL9602BVB_REG_LIST_END,
    /* regField index max */ RTL9602BVB_REGFIELD_LIST_END,
    /* table index max */ RTL9602BVB_TABLE_LIST_END,
}; /* end of rtl9602bvb_a_driver */
