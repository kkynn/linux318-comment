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
 * $Revision: 46644 $
 * $Date: 2014-02-26 18:16:35 +0800 (?Ÿæ?ä¸? 26 äºŒæ? 2014) $
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
#include <ioal/mem32.h>
#include <rtk/switch.h>
#include <rtk/acl.h>
#include <rtk/qos.h>
#include <rtk/rate.h>
#include <rtk/l2.h>

#include <dal/rtl9603d/dal_rtl9603d.h>
#include <dal/rtl9603d/dal_rtl9603d_switch.h>
#include <dal/rtl9603d/dal_rtl9603d_flowctrl.h>
#include <dal/rtl9603d/dal_rtl9603d_rate.h>
#include <dal/rtl9603d/dal_rtl9603d_tool.h>
#include <hal/common/miim.h>
#include <osal/time.h>
#include <hal/mac/mac_probe.h>

/*
 * Symbol Definition
 */

// TBD - 20180129
#if 1//!defined(CONFIG_SDK_KERNEL_LINUX)
#define dal_rtl9603d_tool_get_patch_info(x, y) (0)
#endif

typedef struct dal_rtl9603d_phy_data_s
{
    uint16  phy;
    uint16  addr;
    uint16  data;
} dal_rtl9603d_phy_data_t;

/*
 * Data Declaration
 */
static uint32    switch_init = INIT_NOT_COMPLETED;
static dal_rtl9603d_phy_data_t  phyPatchArray[] = {
    /*TBD*/
    /* Channel Estimation ver. 20170803 */

    /* Green Table initial part ver. 20170810 */

    /* RTCT Open/Mismatch threshold ver. 20170810 */
    {0x0004,0xa438,0xC4E7}
};

/*
 * Function Declaration
 */

int32 dal_rtl9603d_switch_phyPatch(void);

int32 _rtl9603d_switch_macAddr_set(rtk_mac_t *pMacAddr);
int32 _rtl9603d_switch_macAddr_get(rtk_mac_t *pMacAddr);

#if defined(CONFIG_SDK_KERNEL_LINUX)
static int32
_dal_rtl9603d_switch_ipEnable_set(void)
{
#if defined(FPGA_DEFINED)
#else
    int32  ret;
    uint32 value;

    /* Enable all necessary IPs in SoC */
    /* Enable PONPBO IP */
    if ((ret = ioal_socMem32_read(0xB800063C, &value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PBO), "");
        return ret;
    }
    value |= (1 << 5);
    if ((ret = ioal_socMem32_write(0xB800063C, value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PBO), "");
        return ret;
    }

    /* Initialize switch PBO */
    /* Enable PONPBO IP */
    if ((ret = ioal_socMem32_read(0xB800063C, &value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PBO), "");
        return ret;
    }
    value |= (1 << 25);
    if ((ret = ioal_socMem32_write(0xB800063C, value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_PBO), "");
        return ret;
    }
#endif
    return RT_ERR_OK;
}
#endif

static int32
_dal_rtl9603d_switch_ponAsicPhyPortId_get(rtk_switch_port_name_t portName, int32 *pPortId)
{
    switch(portName)
    {
#if defined(CONFIG_SDK_KERNEL_LINUX)
    case RTK_PORT_UTP0:
    case RTK_PORT_UTP1:
    case RTK_PORT_UTP2:
    case RTK_PORT_UTP3:
    case RTK_PORT_UTP4:
    {
        int ret;
        uint32 i, valid, portCnt = portName - RTK_PORT_UTP0;
        uint32 capability = 0;

        // TBD - 20180129
        #if 0
        if((ret = dal_rtl9603d_tool_get_capability(&capability)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        #endif

        /* Count how many valid port before portName */
        for(i = 0, valid = 0 ; i < 5 ; i ++)
        {
            if(!(capability & (1<<i)))
            {
                valid ++;
            }
            if(valid == (portCnt + 1))
            {
                *pPortId = i;
                return RT_ERR_OK;
            }
        }
        return RT_ERR_INPUT;
    }
#else
    case RTK_PORT_UTP0:
        *pPortId = 0;
        break;
    case RTK_PORT_UTP1:
        *pPortId = 1;
        break;
    case RTK_PORT_UTP2:
        *pPortId = 2;
        break;
    case RTK_PORT_UTP3:
        *pPortId = 3;
        break;
#endif
    case RTK_PORT_PON:
        *pPortId = 4;
        break;
    case RTK_PORT_CPU:
        *pPortId = 5;
        break;
    default:
        return RT_ERR_INPUT;
    }
        
    return RT_ERR_OK;
}

static int32
_dal_rtl9603d_switch_ponAsicLogicalPortName_get(int32 portId, rtk_switch_port_name_t *pPortName)
{

    switch(portId)
    {
#if defined(CONFIG_SDK_KERNEL_LINUX)
    case 0:
    case 1:
    case 2:
    case 3:
    {
        int ret;
        uint32 i, valid;
        uint32 capability = 0;

        // TBD - 20180129
        #if 0
        if((ret = dal_rtl9603d_tool_get_capability(&capability)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        #endif

        /* Find nth valid physical port */
        if((capability & (1<<portId)))
        {
            return RT_ERR_INPUT;
        }
        for(i = 0, valid = 0 ; i < portId ; i++)
        {
            if(!(capability & (1<<i)))
            {
                valid ++;
            }
        }
        *pPortName = valid + RTK_PORT_UTP0;
        break;
    }
#else
    case 0:
    case 1:
    case 2:
    case 3:
        *pPortName = RTK_PORT_UTP0 + portId;
        break;
#endif
    case 4:
        *pPortName = RTK_PORT_PON;
        break;
    case 5:
        *pPortName = RTK_PORT_CPU;
        break;
    default:
        return RT_ERR_INPUT;
    }
        
    return RT_ERR_OK;
}


static int32
_dal_rtl9603d_switch_tickToken_init(void)
{
    int ret;
    uint32 wData, switchTick, switchToken;
    uint32 chip, rev, subtype;

    /*meter pon-tick-token configuration*/
    wData = 0x6e;
    if ((ret = reg_field_write(RTL9603D_PON_TB_CTRLr, RTL9603D_TICK_PERIODf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
    wData = 0x95;
    if ((ret = reg_field_write(RTL9603D_PON_TB_CTRLr, RTL9603D_TKNf, &wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    if((ret = dal_rtl9603d_switch_version_get(&chip, &rev, &subtype)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    /*meter switch-tick-token configuration*/
    switchTick = 0x2b;
    switchToken = 0xbd;
    if ((ret = reg_field_write(RTL9603D_METER_TB_CTRLr, RTL9603D_TICK_PERIODf, &switchTick)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_write(RTL9603D_METER_TB_CTRLr, RTL9603D_TKNf, &switchToken)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}



static int32
_dal_rtl9603d_switch_green_enable(void)
{
    return RT_ERR_OK;
}

static int32
_dal_rtl9603d_switch_eee_enable(void)
{
    return RT_ERR_OK;
}

static int32
_dal_rtl9603d_switch_powerSaving_init(void)
{
    int ret;

    /* === Green enable === */
    if ((ret = _dal_rtl9603d_switch_green_enable()) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    /* === EEE enable === */
    if ((ret = _dal_rtl9603d_switch_eee_enable()) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}

static int32
_dal_rtl9603d_switch_phyPower_set(rtk_enable_t enable)
{
    int ret;
    uint32 wData;
    uint32 rData;
    uint32 i;

    for(i = 0 ; i <= 5 ; i++)
    {
        if(!HAL_IS_PORT_EXIST(i))
        {
            continue;
        }

        /* Port i */
        wData = 0x20a400 | (i << 16);
        if ((ret = reg_write(RTL9603D_GPHY_IND_CMDr, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
            return ret;
        }
        osal_time_mdelay(10);
        if ((ret = reg_read(RTL9603D_GPHY_IND_RDr, &rData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
            return ret;
        }

        if(enable)
        {
            wData = rData & 0xf7ff;
        }
        else
        {
            wData = rData | 0x0800;
        }
        if ((ret = reg_write(RTL9603D_GPHY_IND_WDr, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
            return ret;
        }
        wData = 0x60a400 | (i << 16);
        if ((ret = reg_write(RTL9603D_GPHY_IND_CMDr, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
            return ret;
        }
        osal_time_mdelay(10);
    }

    return RT_ERR_OK;
}

int32
dal_rtl9603d_switch_phyPatch(void)
{
    int i;
    int ret;
    dal_rtl9603d_phy_data_t *patchArray;
    uint32 patchSize;
    uint32 chip, rev, subtype;

    if((ret = dal_rtl9603d_switch_version_get(&chip, &rev, &subtype)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    patchArray = phyPatchArray;
    patchSize = sizeof(phyPatchArray)/sizeof(dal_rtl9603d_phy_data_t);

    /* Select phy patch */
    if(CHIP_REV_ID_A >= rev)
    {
        patchArray = phyPatchArray;
        patchSize = sizeof(phyPatchArray)/sizeof(dal_rtl9603d_phy_data_t);
    }
    else
    {
        patchArray = phyPatchArray;
        patchSize = sizeof(phyPatchArray)/sizeof(dal_rtl9603d_phy_data_t);
    }

    /*start patch phy*/
    for(i=0 ; i<patchSize ; i++)
    {
        if((ret=rtl9603d_ocpInterPhy_write(patchArray[i].phy,
                              patchArray[i].addr,
                              patchArray[i].data))!=RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
    }

    return RT_ERR_OK;
}

#define CAL_LONG_START_ENTRY 14
#define CAL_SHORT_START_ENTRY 19
#define CAL_NUM_PHY_PORT 5
#define CAL_PATCH_TIMEOUT 10000

static dal_rtl9603d_phy_data_t  en70PatchArray[] = {
    /* En70 ver. 20171113 */
    {0x0, 0xa436, 0x83ed},
    {0x0, 0xa438, 0xaf83},
    {0x0, 0xa438, 0xf9af},
    {0x0, 0xa438, 0x8402},
    {0x0, 0xa438, 0xaf84},
    {0x0, 0xa438, 0x02af},
    {0x0, 0xa438, 0x8402},
    {0x0, 0xa438, 0x020c},
    {0x0, 0xa438, 0x3f02},
    {0x0, 0xa438, 0x8402},
    {0x0, 0xa438, 0xaf0c},
    {0x0, 0xa438, 0x39f8},
    {0x0, 0xa438, 0xf9fb},
    {0x0, 0xa438, 0xef79},
    {0x0, 0xa438, 0xbf84},
    {0x0, 0xa438, 0x5602},
    {0x0, 0xa438, 0x4997},
    {0x0, 0xa438, 0x0d11},
    {0x0, 0xa438, 0xf62f},
    {0x0, 0xa438, 0xef31},
    {0x0, 0xa438, 0xe087},
    {0x0, 0xa438, 0x770d},
    {0x0, 0xa438, 0x01f6},
    {0x0, 0xa438, 0x271b},
    {0x0, 0xa438, 0x03aa},
    {0x0, 0xa438, 0x1ae0},
    {0x0, 0xa438, 0x877c},
    {0x0, 0xa438, 0xe187},
    {0x0, 0xa438, 0x7dbf},
    {0x0, 0xa438, 0x8459},
    {0x0, 0xa438, 0x0249},
    {0x0, 0xa438, 0x53e0},
    {0x0, 0xa438, 0x877e},
    {0x0, 0xa438, 0xe187},
    {0x0, 0xa438, 0x7fbf},
    {0x0, 0xa438, 0x845c},
    {0x0, 0xa438, 0x0249},
    {0x0, 0xa438, 0x53ae},
    {0x0, 0xa438, 0x18e0},
    {0x0, 0xa438, 0x8778},
    {0x0, 0xa438, 0xe187},
    {0x0, 0xa438, 0x79bf},
    {0x0, 0xa438, 0x8459},
    {0x0, 0xa438, 0x0249},
    {0x0, 0xa438, 0x53e0},
    {0x0, 0xa438, 0x877a},
    {0x0, 0xa438, 0xe187},
    {0x0, 0xa438, 0x7bbf},
    {0x0, 0xa438, 0x845c},
    {0x0, 0xa438, 0x0249},
    {0x0, 0xa438, 0x53ef},
    {0x0, 0xa438, 0x97ff},
    {0x0, 0xa438, 0xfdfc},
    {0x0, 0xa438, 0x0470},
    {0x0, 0xa438, 0xa880},
    {0x0, 0xa438, 0xf0bc},
    {0x0, 0xa438, 0xdcf0},
    {0x0, 0xa438, 0xbcde},
    {0x0, 0xa436, 0xb818},
    {0x0, 0xa438, 0x0C36},
    {0x0, 0xa436, 0xb81a},
    {0x0, 0xa438, 0x3531},
    {0x0, 0xa436, 0xb81c},
    {0x0, 0xa438, 0x3531},
    {0x0, 0xa436, 0xb81e},
    {0x0, 0xa438, 0x000F},
    {0x0, 0xa436, 0xb832},
    {0x0, 0xa438, 0x0001}
};

static int32
_dal_rtl9603d_switch_phyEn70Cal(void)
{
    int32 ret;
    uint32 i, j, port, cnt, patchSize, calValue;
    uint16 value;
    dal_rtl9603d_phy_data_t *patchArray;

    RT_LOG(LOG_DEBUG, (MOD_SWITCH|MOD_DAL), "switch en70 cal start\n");
    for(i = 0;i < CAL_NUM_PHY_PORT;i ++)
    {
        port = CAL_NUM_PHY_PORT - i - 1;

        /* Patch start */
        if((ret = rtl9603d_ocpInterPhy_read(port, 0xb820, &value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value |= 0x0010;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xb820, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }

        cnt = CAL_PATCH_TIMEOUT;
        while(cnt > 0)
        {
            if((ret = rtl9603d_ocpInterPhy_read(port, 0xb800, &value)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
                return ret;
            }
            if((value & 0x40) == 0x40)
            {
                break;
            }
            osal_time_mdelay(1);
            cnt --;
        }

        if((value & 0x40) != 0x40)
        {
            RT_ERR(RT_ERR_TIMEOUT, (MOD_SWITCH|MOD_DAL), "");
            return RT_ERR_TIMEOUT;
        }
        /* Ready for patch */

        value = 0x8028;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xA436, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value = 0x3700;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xA438, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }

        if((ret = rtl9603d_ocpInterPhy_read(port, 0xb82e, &value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value |= 0x0001;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xb82e, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }

        /* Patch code execution */
        patchArray = en70PatchArray;
        patchSize = sizeof(en70PatchArray)/sizeof(dal_rtl9603d_phy_data_t);
        for(j=0 ; j<patchSize ; j++)
        {
            if((ret=rtl9603d_ocpInterPhy_write(port,
                                  patchArray[j].addr,
                                  patchArray[j].data))!=RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
                return ret;
            }
        }

        /* Length threshold */
        value = 0x8777;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa436, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value = 0x4600;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa438, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }

        /* Long length */
        if(dal_rtl9603d_tool_get_patch_info(i + CAL_LONG_START_ENTRY, &calValue) != 0)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return RT_ERR_FAILED;
        }
        value = 0x8778;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa436, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value = calValue;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa438, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        
        /* Short length */
        if(dal_rtl9603d_tool_get_patch_info(i + CAL_SHORT_START_ENTRY, &calValue) != 0)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return RT_ERR_FAILED;
        }
        value = 0x877A;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa436, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value = calValue;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa438, calValue)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value = 0x877C;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa436, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa438, calValue)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value = 0x877E;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa436, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xa438, calValue)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }

        if((ret = rtl9603d_ocpInterPhy_read(port, 0xb82e, &value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        value &= 0xfffe;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xb82e, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }

        /* Patch end */
        value = 0;
        if((ret = rtl9603d_ocpInterPhy_write(port, 0xb820, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
    }

    return RT_ERR_OK;
}


static int32
_dal_rtl9603d_switch_mdiAmpVoltage(void)
{
    int32 ret, port;
    uint32 wData;

    for(port = 0 ; port < CAL_NUM_PHY_PORT ; port ++)
    {
        wData = 0x2AA00;
        if ((ret = reg_array_write(RTL9603D_PHY_GDAC_IB_10Mr, REG_ARRAY_INDEX_NONE, port, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        wData = 0x2AA00;
        if ((ret = reg_array_write(RTL9603D_PHY_GDAC_IB_100Mr, REG_ARRAY_INDEX_NONE, port, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        wData = 0x2AA00;
        if ((ret = reg_array_write(RTL9603D_PHY_GDAC_IB_1000Mr, REG_ARRAY_INDEX_NONE, port, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
    }
    
    return RT_ERR_OK;
}

static int32
_dal_rtl9603d_switch_phyCal(void)
{
    uint32 i;
    uint32 value, va, vb, vc, vd;
    int32 ret;

    RT_LOG(LOG_DEBUG, (MOD_SWITCH|MOD_DAL), "switch cal start\n");
    for(i = 0;i < CAL_NUM_PHY_PORT;i ++)
    {
        if(dal_rtl9603d_tool_get_patch_info(i + CAL_LONG_START_ENTRY, &value) != 0)
    if(1)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return RT_ERR_FAILED;
        }

        va = value & 0xf;
        vb = (value & 0xf0) >> 4;
        vc = (value & 0xf00) >> 8;
        vd = (value & 0xf000) >> 12;
        va = (va + 7) > 0xf ? 0xf : (va + 7);
        vb = (vb + 7) > 0xf ? 0xf : (vb + 7);
        vc = (vc + 7) > 0xf ? 0xf : (vc + 7);
        vd = (vd + 7) > 0xf ? 0xf : (vd + 7);
        value = va | (vb << 4) | (vc << 8) | (vd << 12);

        RT_LOG(LOG_DEBUG, (MOD_SWITCH|MOD_DAL), "write phy=%d, val=0x%x\n", CAL_NUM_PHY_PORT - i - 1, value);
        if((ret = rtl9603d_ocpInterPhy_write(CAL_NUM_PHY_PORT - i - 1, 0xbcdc, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if((ret = rtl9603d_ocpInterPhy_write(CAL_NUM_PHY_PORT - i - 1, 0xbcde, value)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
    }

    return RT_ERR_OK;
}

#define PATCH_NUM_ENTRY 37
#define PATCH_START_ENTRY 38
#define PATCH_CMD_END_ENTRY 251 
static int32
_dal_rtl9603d_switch_default_patch(void)
{
    uint32 v1, v2, v3, v4, ptn1_no, ptn2_no, i, j;
    int32 ret;

    /* check need patch or not */
    if ((ret = reg_array_field_read(RTL9603D_EFUSE_BOND_RSLTr, 0, REG_ARRAY_INDEX_NONE, RTL9603D_EF_BOND_RSLTf, &v1)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
    if((v1 & 0x1) == 0)
        return RT_ERR_OK;

    if((ret = dal_rtl9603d_tool_get_patch_info(PATCH_NUM_ENTRY, &ptn1_no))!=0)
        return RT_ERR_FAILED;
    osal_printf("switch default start\n");
    ptn1_no=ptn1_no&0xff;
    for(i=0; i<ptn1_no; i++)
    {
        if(dal_rtl9603d_tool_get_patch_info(PATCH_START_ENTRY+i*3, &v1)!=0)
            return RT_ERR_FAILED;
        if(dal_rtl9603d_tool_get_patch_info(PATCH_START_ENTRY+i*3+1, &v2)!=0)
            return RT_ERR_FAILED;
        if(dal_rtl9603d_tool_get_patch_info(PATCH_START_ENTRY+i*3+2, &v3)!=0)
            return RT_ERR_FAILED;
        if(v1==0 && v2==0 && v3==0)
            return RT_ERR_OK;
        if(v1==0xffff && v2==0xffff && v3==0xffff)
            continue;
        HAL_SCAN_ALL_PORT(j)
        {
            if((v1>>j)&0x1)
            {
                osal_printf("write phy=%d, reg=0x%x, val=0x%x\n", j, v2, v3);
                if((ret = rtl9603d_ocpInterPhy_write(j, v2, v3)) != RT_ERR_OK)
                {
                    RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
                    return ret;
                }
                
            }
        }
    }

    ptn2_no=(PATCH_CMD_END_ENTRY-PATCH_START_ENTRY+1-3*ptn1_no)/4;
    for(i=0; i<ptn2_no; i++)
    {
        if(dal_rtl9603d_tool_get_patch_info(PATCH_START_ENTRY+ptn1_no*3+i*4, &v1)!=0)
            return RT_ERR_FAILED;
        if(dal_rtl9603d_tool_get_patch_info(PATCH_START_ENTRY+ptn1_no*3+i*4+1, &v2)!=0)
            return RT_ERR_FAILED;
        if(dal_rtl9603d_tool_get_patch_info(PATCH_START_ENTRY+ptn1_no*3+i*4+2, &v3)!=0)
            return RT_ERR_FAILED;
        if(dal_rtl9603d_tool_get_patch_info(PATCH_START_ENTRY+ptn1_no*3+i*4+3, &v4)!=0)
            return RT_ERR_FAILED;
        if(v1==0 && v2==0 && v3==0 && v4==0)
            return RT_ERR_OK;
        if(v1==0xffff && v2==0xffff && v3==0xffff && v4==0xffff)
            continue;
        WRITE_MEM32((v1<<16)|v2, (v3<<16)|v4);
        osal_printf("write mem=0x%x, val=0x%x\n", (v1<<16)|v2, (v3<<16)|v4);
    }

    return RT_ERR_OK;
}

/* Module Name    : Switch     */
/* Sub-module Name: Switch parameter settings */

/* Function Name:
 *      dal_rtl9603d_switch_init
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
dal_rtl9603d_switch_init(void)
{
    int32 ret;
    uint32 wData;
    rtk_port_t port;
    uint32 chip, rev, subtype;
    
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    switch_init = INIT_COMPLETED;

#if defined(CONFIG_SDK_KERNEL_LINUX)
    /* Init all necessary IPs before all initializations */
    _dal_rtl9603d_switch_ipEnable_set();
#endif


#if 0    /*TBD*/ 
    /*flow control threshold and set buffer mode*/
    if((ret = rtl9603d_raw_flowctrl_patch(FLOWCTRL_PATCH_DEFAULT)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#endif 

    /*meter tick-token configuration*/
    if((ret = _dal_rtl9603d_switch_tickToken_init())!=RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    /* Make sure all the ports are link down before any PHY related operation */
    if ((ret = _dal_rtl9603d_switch_phyPower_set(DISABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    /*set switch ready, phy patch done*/
    wData = 1;
    if ((ret = reg_field_write(RTL9603D_WRAP_GPHY_MISCr,RTL9603D_PATCH_PHY_DONEf,&wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    if((ret = dal_rtl9603d_switch_version_get(&chip, &rev, &subtype)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#if defined(FPGA_DEFINED)

#else
#if defined(RTL_CYGWIN_EMULATE)

#else
    /*analog patch*/
    /*phy patch*/
    if((ret=dal_rtl9603d_switch_phyPatch())!=RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "phy patch fail\n");
        return ret;
    }

    if(rev > CHIP_REV_ID_A)
    {
        /* phy en70 calibration patch */
        if((ret=_dal_rtl9603d_switch_phyEn70Cal())!=RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "phy en70 calibration fail, ret=0x%x\n",ret);
            return ret;
        }
        if((ret=_dal_rtl9603d_switch_mdiAmpVoltage())!=RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "phy mdi amp voltage fail, ret=0x%x\n",ret);
            return ret;
        }
    }
    else
    {
        /* phy calibration patch */
        if((ret=_dal_rtl9603d_switch_phyCal())!=RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "phy calibration fail, ret=0x%x\n",ret);
            return ret;
        }
    }

    /* default patch */
    if((ret=_dal_rtl9603d_switch_default_patch())!=RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "default patch fail, ret=0x%x\n",ret);
        return ret;
    }

    /* power saving feature */
    if((ret=_dal_rtl9603d_switch_powerSaving_init())!=RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#endif
#if 0 // TBD RTL9603D_DIVSYSCLK_TX_CTRLr REG Remove
    if(rev > CHIP_REV_ID_A)
    {
        /* Switch patch */
        wData = 1;
        if ((ret = reg_array_field_write(RTL9603D_DIVSYSCLK_TX_CTRLr, 0, REG_ARRAY_INDEX_NONE, RTL9603D_DLY_TXCOMf, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if ((ret = reg_array_field_write(RTL9603D_DIVSYSCLK_TX_CTRLr, 1, REG_ARRAY_INDEX_NONE, RTL9603D_DLY_TXCOMf, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if ((ret = reg_array_field_write(RTL9603D_DIVSYSCLK_TX_CTRLr, 2, REG_ARRAY_INDEX_NONE, RTL9603D_DLY_TXCOMf, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if ((ret = reg_array_field_write(RTL9603D_DIVSYSCLK_TX_CTRLr, 3, REG_ARRAY_INDEX_NONE, RTL9603D_DLY_TXCOMf, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if ((ret = reg_array_field_write(RTL9603D_DIVSYSCLK_TX_CTRLr, 4, REG_ARRAY_INDEX_NONE, RTL9603D_DLY_TXCOMf, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if ((ret = reg_array_field_write(RTL9603D_DIVSYSCLK_TX_CTRLr, 8, REG_ARRAY_INDEX_NONE, RTL9603D_DLY_TXCOMf, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if ((ret = reg_array_field_write(RTL9603D_DIVSYSCLK_TX_CTRLr, 9, REG_ARRAY_INDEX_NONE, RTL9603D_DLY_TXCOMf, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
        if ((ret = reg_array_field_write(RTL9603D_DIVSYSCLK_TX_CTRLr, 10, REG_ARRAY_INDEX_NONE, RTL9603D_DLY_TXCOMf, &wData)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
    }
#endif
#if defined(CONFIG_SWITCH_INIT_LINKDOWN)
    /* Turn off phy power*/
    if ((ret = _dal_rtl9603d_switch_phyPower_set(DISABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#else
    /*turn on phy power*/
    if ((ret = _dal_rtl9603d_switch_phyPower_set(ENABLED)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }
#endif

#endif
    wData = 1;
    if ((ret = reg_field_write(RTL9603D_CFG_UNHIOLr, RTL9603D_IPG_COMPENSATIONf, (uint32 *)&wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    wData = 1;
    if ((ret = reg_field_write(RTL9603D_SDS_REG7r, RTL9603D_SP_CFG_NEG_CLKWR_A2Df, (uint32 *)&wData)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }


    /*force CPU port accept small packet and enable padding*/
    HAL_SCAN_ALL_PORT(port)
    {    
        if(HAL_IS_CPU_PORT(port))
        {
            wData = 1;
            if ((ret = reg_array_field_write(RTL9603D_P_MISCr, port, REG_ARRAY_INDEX_NONE, RTL9603D_RX_SPCf, &wData)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
                return ret;
            }    

            wData = 1;
            if ((ret = reg_array_field_write(RTL9603D_P_MISCr, port, REG_ARRAY_INDEX_NONE, RTL9603D_PADDING_ENf, &wData)) != RT_ERR_OK)
            {
                RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
                return ret;
            }    
        }
    }        

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_init */

/* Module Name    : Switch     */
/* Sub-module Name: Switch parameter settings */


/* Function Name:
 *      dal_rtl9603d_switch_phyPortId_get
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
dal_rtl9603d_switch_phyPortId_get(rtk_switch_port_name_t portName, int32 *pPortId)
{
    dal_rtl9603d_switch_asic_type_t asicType;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* parameter check */
    RT_PARAM_CHK((RTK_PORT_NAME_END <= portName), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pPortId), RT_ERR_NULL_POINTER);

    /*get chip info to check port name mapping*/
    asicType = RTL9603D_ASIC_PON;

    switch(asicType)
    {
        case RTL9603D_ASIC_PON:
        default:
            return _dal_rtl9603d_switch_ponAsicPhyPortId_get(portName,pPortId);
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_phyPortId_get */



/* Function Name:
 *      dal_rtl9603d_switch_logicalPort_get
 * Description:
 *      Get logical port name from physical port id
 * Input:
 *      portId  - physical port id
 * Output:
 *      pPortName - pointer to logical port name
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 */
int32
dal_rtl9603d_switch_logicalPort_get(int32 portId, rtk_switch_port_name_t *pPortName)
{
    dal_rtl9603d_switch_asic_type_t asicType;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* parameter check */
    RT_PARAM_CHK((NULL == pPortName), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(portId), RT_ERR_PORT_ID);

    /*get chip info to check port name mapping*/
    asicType = RTL9603D_ASIC_PON;

    switch(asicType)
    {
        case RTL9603D_ASIC_PON:
        default:
            return _dal_rtl9603d_switch_ponAsicLogicalPortName_get(portId,pPortName);
            break;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_logicalPort_get */



/* Function Name:
 *      dal_rtl9603d_switch_port2PortMask_set
 * Description:
 *      Set port id to the portlist
 * Input:
 *      pPortMask    - port mask
 *      portName     - logical port name
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Call RTK API the port mask must set by this API
 */
int32
dal_rtl9603d_switch_port2PortMask_set(rtk_portmask_t *pPortMask, rtk_switch_port_name_t portName)
{
    int32 portId;
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* parameter check */
    RT_PARAM_CHK((NULL == pPortMask), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((RTK_PORT_NAME_END <= portName), RT_ERR_INPUT);

    if((ret=dal_rtl9603d_switch_phyPortId_get(portName,&portId))!=RT_ERR_OK)
        return ret;

    RTK_PORTMASK_PORT_SET(*pPortMask,portId);

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_port2PortMask_set */



/* Function Name:
 *      dal_rtl9603d_switch_port2PortMask_clear
 * Description:
 *      Set port id to the portlist
 * Input:
 *      pPortMask    - port mask
 *      portName     - logical port name
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Call RTK API the port mask must set by this API
 */
int32
dal_rtl9603d_switch_port2PortMask_clear(rtk_portmask_t *pPortMask, rtk_switch_port_name_t portName)
{
    int32   ret;
    int32 portId;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* parameter check */
    RT_PARAM_CHK((NULL == pPortMask), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((RTK_PORT_NAME_END <= portName), RT_ERR_INPUT);

    if((ret=dal_rtl9603d_switch_phyPortId_get(portName,&portId))!=RT_ERR_OK)
        return ret;

    RTK_PORTMASK_PORT_CLEAR(*pPortMask, portId);

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_port2PortMask_clear */



/* Function Name:
 *      dal_rtl9603d_switch_portIdInMask_check
 * Description:
 *      Check if given port is in port list
 * Input:
 *      pPortMask    - port mask
 *      portName     - logical port name
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 */
int32
dal_rtl9603d_switch_portIdInMask_check(rtk_portmask_t *pPortMask, rtk_switch_port_name_t portName)
{
    int32   ret;
    int32 portId;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* parameter check */
    RT_PARAM_CHK((NULL == pPortMask), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((RTK_PORT_NAME_END <= portName), RT_ERR_INPUT);

    if((ret=dal_rtl9603d_switch_phyPortId_get(portName,&portId))!=RT_ERR_OK)
        return ret;

    if(RTK_PORTMASK_IS_PORT_SET(*pPortMask,portId))
        return RT_ERR_OK;
    else
        return RT_ERR_FAILED;

} /* end of dal_rtl9603d_switch_portIdInMask_check */

#if 0
/* Function Name:
  *      dal_rtl9603d_switch_maxPktLenLinkSpeed_get
  * Description:
  *      Get the max packet length setting of the specific speed type
  * Input:
  *      speed - speed type
  * Output:
  *      pLen  - pointer to the max packet length
  * Return:
  *      RT_ERR_OK
  *      RT_ERR_FAILED
  *      RT_ERR_NULL_POINTER - input parameter may be null pointer
  *      RT_ERR_INPUT        - invalid enum speed type
  * Note:
  *      Max packet length setting speed type
  *      - MAXPKTLEN_LINK_SPEED_FE
  *      - MAXPKTLEN_LINK_SPEED_GE
  */
int32
dal_rtl9603d_switch_maxPktLenLinkSpeed_get(rtk_switch_maxPktLen_linkSpeed_t speed, uint32 *pLen)
{
    int32   ret;
    uint32 regAddr;
    uint32 fieldIdx;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    /* parameter check */
    RT_PARAM_CHK((MAXPKTLEN_LINK_SPEED_END <= speed), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pLen), RT_ERR_NULL_POINTER);

    if(speed == MAXPKTLEN_LINK_SPEED_FE)
    {
        regAddr =  RTL9603D_MAX_LENGTH_CFG0r;
        fieldIdx = RTL9603D_ACCEPT_MAX_LENTH_CFG0f;
    }
    else
    {
        regAddr =  RTL9603D_MAX_LENGTH_CFG1r;
        fieldIdx = RTL9603D_ACCEPT_MAX_LENTH_CFG1f;
    }

    if ((ret = reg_field_read(regAddr, fieldIdx, pLen)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }


    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_maxPktLenLinkSpeed_get */

/* Function Name:
  *      dal_rtl9603d_switch_maxPktLenLinkSpeed_set
  * Description:
  *      Set the max packet length of the specific speed type
  * Input:
  *      speed - speed type
  *      len   - max packet length
  * Output:
  *      None
  * Return:
  *      RT_ERR_OK
  *      RT_ERR_FAILED
  *      RT_ERR_INPUT   - invalid enum speed type
  * Note:
  *      Max packet length setting speed type
  *      - MAXPKTLEN_LINK_SPEED_FE
  *      - MAXPKTLEN_LINK_SPEED_GE
  */
int32
dal_rtl9603d_switch_maxPktLenLinkSpeed_set(rtk_switch_maxPktLen_linkSpeed_t speed, uint32 len)
{
    rtk_port_t  port, max_port;
    int32   ret;
    uint32 regAddr;
    uint32 fieldIdx;
    uint32 portField;
    uint32 index;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    /* parameter check */
    RT_PARAM_CHK((MAXPKTLEN_LINK_SPEED_END <= speed), RT_ERR_INPUT);
    RT_PARAM_CHK((RTL9603D_PACEKT_LENGTH_MAX < len), RT_ERR_INPUT);

    if(speed == MAXPKTLEN_LINK_SPEED_FE)
    {
        regAddr =  RTL9603D_MAX_LENGTH_CFG0r;
        fieldIdx = RTL9603D_ACCEPT_MAX_LENTH_CFG0f;
        portField = RTL9603D_MAX_LENGTH_10_100f;
        index = 0;
    }
    else
    {
        regAddr   = RTL9603D_MAX_LENGTH_CFG1r;
        fieldIdx  = RTL9603D_ACCEPT_MAX_LENTH_CFG1f;
        portField = RTL9603D_MAX_LENGTH_GIGAf;
        index = 1;
    }

    if ((ret = reg_field_write(regAddr, fieldIdx, &len)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }

    /*set all port index to currect length index*/
    max_port = HAL_GET_MAX_PORT();
    for (port = 0; port <= max_port; port++)
    {
        if (!HAL_IS_PORT_EXIST(port))
        {
            continue;
        }

        if ((ret = reg_array_field_write(RTL9603D_ACCEPT_MAX_LEN_CTRLr, port, REG_ARRAY_INDEX_NONE, portField, &index)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
            return ret;
        }
    }
    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_maxPktLenLinkSpeed_set */
#endif

/* Module Name    : Switch     */
/* Sub-module Name: Management address and vlan configuration */
/* Function Name:
 *      _rtl9603d_switch_macAddr_set
 * Description:
 *      Set switch mac address configurations
 * Input:
 *      pMacAddr - Switch mac address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                   - Success
 *      RT_ERR_SMI                  - SMI access error
 *      RT_ERR_INPUT                - Invalid input parameter
 * Note:
 *      None
 */
int32 _rtl9603d_switch_macAddr_set(rtk_mac_t *pMacAddr)
{
    int32 ret;
    uint32 i, tmp[6];

    if(pMacAddr == NULL)
        return RT_ERR_NULL_POINTER;

    for (i=0;i<ETHER_ADDR_LEN;i++)
        tmp[i] = pMacAddr->octet[i];


    if ((ret = reg_field_write(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC5f, &tmp[0])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_write(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC4f, &tmp[1])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_write(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC3f, &tmp[2])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_write(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC2f, &tmp[3])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
     if ((ret = reg_field_write(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC1f, &tmp[4])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
     if ((ret = reg_field_write(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC0f, &tmp[5])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
}/*end of _rtl9603d_switch_macAddr_set*/

/* Function Name:
 *      _rtl9603d_switch_macAddr_get
 * Description:
 *      Get switch mac address configurations
 * Input:
 *      pMacAddr - Switch mac address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK                   - Success
 *      RT_ERR_SMI                  - SMI access error
 *      RT_ERR_INPUT                - Invalid input parameter
 * Note:
 *      None
 */
int32 _rtl9603d_switch_macAddr_get(rtk_mac_t *pMacAddr)
{
    int32 ret;
    uint32 i, tmp[6];

    if ((ret = reg_field_read(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC5f, &tmp[0])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_read(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC4f, &tmp[1])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_read(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC3f, &tmp[2])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
    if ((ret = reg_field_read(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC2f, &tmp[3])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
     if ((ret = reg_field_read(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC1f, &tmp[4])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }
     if ((ret = reg_field_read(RTL9603D_SWITCH_MACr, RTL9603D_SWITCH_MAC0f, &tmp[5])) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }

    for (i=0;i<ETHER_ADDR_LEN;i++)
        pMacAddr->octet[i] = tmp[i];

    return RT_ERR_OK;
}/*end of _rtl9603d_switch_macAddr_get*/



/* Function Name:
 *      dal_rtl9603d_switch_mgmtMacAddr_get
 * Description:
 *      Get MAC address of switch.
 * Input:
 *      None
 * Output:
 *      pMac - pointer to MAC address
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_rtl9603d_switch_mgmtMacAddr_get(rtk_mac_t *pMac)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMac), RT_ERR_NULL_POINTER);

    if ((ret = _rtl9603d_switch_macAddr_get(pMac)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_mgmtMacAddr_get */

/* Function Name:
 *      dal_rtl9603d_switch_mgmtMacAddr_set
 * Description:
 *      Set MAC address of switch.
 * Input:
 *      pMac - MAC address
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_rtl9603d_switch_mgmtMacAddr_set(rtk_mac_t *pMac)
{

    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pMac), RT_ERR_NULL_POINTER);

    if((pMac->octet[0] & BITMASK_1B) == 1)
        return RT_ERR_INPUT;

    if ((ret = _rtl9603d_switch_macAddr_set(pMac)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_mgmtMacAddr_set */


/* Function Name:
 *      dal_rtl9603d_switch_chip_reset
 * Description:
 *      Reset switch chip
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
int32
dal_rtl9603d_switch_chip_reset(void){

    int32   ret;
    uint32 resetValue = 1;
    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr,RTL9603D_CMD_CHIP_RST_PSf,&resetValue)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_SWITCH | MOD_DAL), "");
        return ret;
    }

    return RT_ERR_OK;

}


/* Function Name:
 *      dal_rtl9603d_switch_version_get
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
dal_rtl9603d_switch_version_get(uint32 *pChipId, uint32 *pRev, uint32 *pSubtype)
{
    int32  ret;
    static uint8 is_init = 0;
    static uint32 chipId;
    static uint32 chipRev;
    static uint32 chipSubtype;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* parameter check */
    RT_PARAM_CHK((NULL == pChipId), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pRev), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pSubtype), RT_ERR_NULL_POINTER);

    if(0 == is_init)
    {
        if (drv_swcore_cid_get(&chipId, &chipRev) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }

    // TBD - 20180129
    #if 0//defined(CONFIG_SDK_KERNEL_LINUX)
        if ((ret = dal_rtl9603d_tool_get_chipSubType(&chipSubtype)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_SWITCH|MOD_DAL), "");
            return ret;
        }
    #endif
        is_init = 1;
    }

    /* function body */
    *pChipId = chipId;
    *pRev = chipRev;
    *pSubtype = chipSubtype;

    return RT_ERR_OK;
}   /* end of dal_rtl9603d_switch_version_get */

/* Function Name:
  *      dal_rtl9603d_switch_maxPktLenByPort_get
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
dal_rtl9603d_switch_maxPktLenByPort_get(rtk_port_t port, uint32 *pLen)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pLen), RT_ERR_NULL_POINTER);

    /* function body */
    if((ret = reg_array_field_read(RTL9603D_ACCEPT_MAX_LEN_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9603D_ACCEPT_MAX_LENTHf, pLen)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_SWITCH), "");
        return ret;
    }

    return RT_ERR_OK;
} /*end of dal_rtl9603d_switch_maxPktLenByPort_get*/

/* Function Name:
  *      dal_rtl9603d_switch_maxPktLenByPort_set
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
dal_rtl9603d_switch_maxPktLenByPort_set(rtk_port_t port, uint32 len)
{
    int32   ret;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((RTL9603D_PACEKT_LENGTH_MAX < len), RT_ERR_INPUT);

    /* function body */
    if((ret = reg_array_field_write(RTL9603D_ACCEPT_MAX_LEN_CTRLr, port, REG_ARRAY_INDEX_NONE, RTL9603D_ACCEPT_MAX_LENTHf, &len)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_SWITCH), "");
        return ret;
    }

    return RT_ERR_OK;
}/*end of dal_rtl9603d_switch_maxPktLenByPort_set*/

/* Function Name:
  *      dal_rtl9603d_switch_changeDuplex_get
  * Description:
  *      Get change duplex function state
  * Input:
  *      enable   - change duplex state
  * Output:
  *      None
  * Return:
  *      RT_ERR_OK
  *      RT_ERR_FAILED
  *      RT_ERR_INPUT   - invalid enum speed type
  * Note:
  */
int32
dal_rtl9603d_switch_changeDuplex_get(rtk_enable_t *pState)
{
    int     ret;
    uint32  value;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    /* parameter check */
    RT_PARAM_CHK((NULL == pState), RT_ERR_NULL_POINTER);

    if((ret = reg_field_read(RTL9603D_CHANGE_DUPLEX_CTRLr, RTL9603D_CFG_CHG_DUP_ENf, &value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_SWITCH), "");
        return ret;
    }
    *pState = value;

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_changeDuplex_get */

/* Function Name:
  *      dal_rtl9603d_switch_changeDuplex_set
  * Description:
  *      Set change duplex function state
  * Input:
  *      enable   - change duplex state
  * Output:
  *      None
  * Return:
  *      RT_ERR_OK
  *      RT_ERR_FAILED
  *      RT_ERR_INPUT   - invalid enum speed type
  * Note:
  * This function only apply to local N-way enable but link
  * partner in force mode. In that way, the local link status
  * will be 100Mb/half duplex. This function will change
  * local link status to 100Mb/full duplex under specific
  * condition.
  */
int32
dal_rtl9603d_switch_changeDuplex_set(rtk_enable_t state)
{
    int     ret;
    uint32  value;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_SWITCH),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(switch_init);

    /* parameter check */
    RT_PARAM_CHK((state >= RTK_ENABLE_END), RT_ERR_NULL_POINTER);

    value = (ENABLED == state) ? 1 : 0;
    if((ret = reg_field_write(RTL9603D_CHANGE_DUPLEX_CTRLr, RTL9603D_CFG_CHG_DUP_ENf, &value)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_SWITCH), "");
        return ret;
    }

    return RT_ERR_OK;
} /* end of dal_rtl9603d_switch_changeDuplex_set */

/* Function Name:
  *      dal_rtl9603d_switch_system_init
  * Description:
  *      Set system application initial
  * Input:
  *      mode   - initial mode
  * Output:
  *      None
  * Return:
  *      RT_ERR_OK
  *      RT_ERR_FAILED
  * Note:
  *     This function is used for SFU application system
  */
int32
dal_rtl9603d_switch_system_init(rtk_switch_system_mode_t *pMode)
{
#if 0
    int32   ret,qid;
#endif
    if(pMode->initDefault)
    {
#if 0
        /*set CPU port APR for Q0~Q6 rate control with meter 7, exclude OMCI/OAM QID 7 usage*/
        for(qid =0 ; qid <= 6; qid++)
        {
            if ((ret = dal_rtl9603d_rate_egrQueueBwCtrlEnable_set(9,qid,ENABLED)) != RT_ERR_OK)
            {
                return ret;
            }
            /*point to real meter index of CPU port*/
            if ((ret = dal_rtl9603d_rate_egrQueueBwCtrlMeterIdx_set(9,qid,7)) != RT_ERR_OK)
            {
                return ret;
            }
#endif
        }

#if 0
        /*set meter 31(port 9 offset index 7) to to 5000 PPS*/
        if ((ret = dal_rtl9603d_rate_shareMeterMode_set(31, METER_MODE_PACKET_RATE)) != RT_ERR_OK)
        {
            return ret;
        }

        if ((ret = dal_rtl9603d_rate_shareMeter_set(31, 5000, ENABLED)) != RT_ERR_OK)
        {
            return ret;
        }
        /*set CPU egress port for drop tail only*/
        for(qid = 0 ; qid <= 7; qid++)
        {
            if ((ret = rtl9603d_raw_flowctrl_egressDropEnable_set(9,qid ,ENABLED)) != RT_ERR_OK)
            {
                return ret;
            }
        }
    }
#endif
    
    return RT_ERR_OK;
}/* end of dal_rtl9603d_switch_system_init */
