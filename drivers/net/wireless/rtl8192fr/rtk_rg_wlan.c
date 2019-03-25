#include "rtk_rg_wlan.h"

#if defined(CONFIG_RG_WLAN_HWNAT_ACCELERATION) && !defined(CONFIG_ARCH_LUNA_SLAVE)
#if !defined(CONFIG_ARCH_CORTINA_G3) && !defined(CONFIG_ARCH_CORTINA_G3HGU)
// wlan0
struct net_device *wlan_root_netdev=NULL;
struct net_device *wlan_vap_netdev[RTL8192CD_NUM_VWLAN]={0};
struct net_device *wlan_wds_netdev[8]={0};
struct net_device *wlan_vxd_netdev=NULL;
// wlan1
struct net_device *wlan1_root_netdev=NULL;
struct net_device *wlan1_vap_netdev[RTL8192CD_NUM_VWLAN]={0};
struct net_device *wlan1_wds_netdev[8]={0};
struct net_device *wlan1_vxd_netdev=NULL;

/* Function Name:
 *      rtk_rg_wlan_netDevice_set
 * Description:
 *      Set address of net device of wlan
 *      Refer to: drivers/net/ethernet/realtek/rtl86900/romeDriver/rtk_rg_apollo_liteRomeDriver.c
 * Input:
 *      wlanDevIdx  - wlan device index(refer to rtk_rg_mbssidDev_t)
 *      pDev        - address of net device of wlan
 * Output:
 *      None
 * Return:
 *      RT_ERR_RG_OK
 *      RT_ERR_RG_INDEX_OUT_OF_RANGE
 *      RT_ERR_RG_NULL_POINTER
 */
int rtk_rg_wlan_netDevice_set(rtk_rg_mbssidDev_t wlanDevIdx, struct net_device *pDev)
{
    if(!(RG_RET_MBSSID_MASTER_ROOT_INTF<=wlanDevIdx && wlanDevIdx<WLAN_DEVICE_NUM*2))
        return -1;
    if(pDev==NULL)
        return -2;

    switch(wlanDevIdx)
    {
        // wlan0
        case RG_RET_MBSSID_MASTER_ROOT_INTF:
            wlan_root_netdev = pDev;
            //printk("Set net_device of wlan0 root");
            break;
        case RG_RET_MBSSID_MASTER_VAP0_INTF:
        case RG_RET_MBSSID_MASTER_VAP1_INTF:
        case RG_RET_MBSSID_MASTER_VAP2_INTF:
        case RG_RET_MBSSID_MASTER_VAP3_INTF:
#if defined(CONFIG_WLAN_MBSSID_NUM) && (CONFIG_WLAN_MBSSID_NUM==7)
        case RG_RET_MBSSID_MASTER_VAP4_INTF:
        case RG_RET_MBSSID_MASTER_VAP5_INTF:
        case RG_RET_MBSSID_MASTER_VAP6_INTF:
#endif
            wlan_vap_netdev[wlanDevIdx-RG_RET_MBSSID_MASTER_VAP0_INTF] = pDev;
            //printk("Set net_device of wlan0 vap%d", wlanDevIdx-RG_RET_MBSSID_MASTER_VAP0_INTF);
            break;
        case RG_RET_MBSSID_MASTER_WDS0_INTF:
        case RG_RET_MBSSID_MASTER_WDS1_INTF:
        case RG_RET_MBSSID_MASTER_WDS2_INTF:
        case RG_RET_MBSSID_MASTER_WDS3_INTF:
        case RG_RET_MBSSID_MASTER_WDS4_INTF:
        case RG_RET_MBSSID_MASTER_WDS5_INTF:
        case RG_RET_MBSSID_MASTER_WDS6_INTF:
        case RG_RET_MBSSID_MASTER_WDS7_INTF:
            wlan_wds_netdev[wlanDevIdx-RG_RET_MBSSID_MASTER_WDS0_INTF] = pDev;
            //printk("Set net_device of wlan0 wds%d", wlanDevIdx-RG_RET_MBSSID_MASTER_WDS0_INTF);
            break;
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
        case RG_RET_MBSSID_MASTER_CLIENT_INTF:
            wlan_vxd_netdev = pDev;
            //printk("Set net_device of wlan0 vxd");
            break;
#endif
        // wlan1
        case RG_RET_MBSSID_SLAVE_ROOT_INTF:
            wlan1_root_netdev = pDev;
            //printk("Set net_device of wlan1 root");
            break;
        case RG_RET_MBSSID_SLAVE_VAP0_INTF:
        case RG_RET_MBSSID_SLAVE_VAP1_INTF:
        case RG_RET_MBSSID_SLAVE_VAP2_INTF:
        case RG_RET_MBSSID_SLAVE_VAP3_INTF:
#if defined(CONFIG_WLAN_MBSSID_NUM) && (CONFIG_WLAN_MBSSID_NUM==7)
        case RG_RET_MBSSID_SLAVE_VAP4_INTF:
        case RG_RET_MBSSID_SLAVE_VAP5_INTF:
        case RG_RET_MBSSID_SLAVE_VAP6_INTF:
#endif
            wlan1_vap_netdev[wlanDevIdx-RG_RET_MBSSID_SLAVE_VAP0_INTF] = pDev;
            //printk("Set net_device of wlan1 vap%d", wlanDevIdx-RG_RET_MBSSID_SLAVE_VAP0_INTF);
            break;
        case RG_RET_MBSSID_SLAVE_WDS0_INTF:
        case RG_RET_MBSSID_SLAVE_WDS1_INTF:
        case RG_RET_MBSSID_SLAVE_WDS2_INTF:
        case RG_RET_MBSSID_SLAVE_WDS3_INTF:
        case RG_RET_MBSSID_SLAVE_WDS4_INTF:
        case RG_RET_MBSSID_SLAVE_WDS5_INTF:
        case RG_RET_MBSSID_SLAVE_WDS6_INTF:
        case RG_RET_MBSSID_SLAVE_WDS7_INTF:
            wlan1_wds_netdev[wlanDevIdx-RG_RET_MBSSID_SLAVE_WDS0_INTF] = pDev;
            //printk("Set net_device of wlan1 wds%d", wlanDevIdx-RG_RET_MBSSID_SLAVE_WDS0_INTF);
            break;
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
        case RG_RET_MBSSID_SLAVE_CLIENT_INTF:
            wlan1_vxd_netdev = pDev;
            //printk("Set net_device of wlan1 vxd");
            break;
#endif
        default:
            break;
    }

    return 0;
}

int rtk_rg_wlan_netDevice_show(void)
{
    int i;

    printk("wlan_root_netdev\t\t= %p \n", wlan_root_netdev);
    for(i=0; i<RTL8192CD_NUM_VWLAN; i++)
        printk("wlan_vap_netdev[%d]\t= %p \n", i, wlan_vap_netdev[i]);
    for(i=0; i<8; i++)
        printk("wlan_wds_netdev[%d]\t= %p \n", i, wlan_wds_netdev[i]);
    printk("wlan_vxd_netdev\t\t= %p \n", wlan_vxd_netdev);

    printk("wlan1_root_netdev\t= %p \n", wlan1_root_netdev);
    for(i=0; i<RTL8192CD_NUM_VWLAN; i++)
        printk("wlan1_vap_netdev[%d]\t= %p \n", i, wlan1_vap_netdev[i]);
    for(i=0; i<8; i++)
        printk("wlan1_wds_netdev[%d]\t= %p \n", i, wlan1_wds_netdev[i]);
    printk("wlan1_vxd_netdev\t\t= %p \n", wlan1_vxd_netdev);

    return 0;
}

#endif // end of !defined(CONFIG_ARCH_CORTINA_G3) && !defined(CONFIG_ARCH_CORTINA_G3HGU)
#endif  // end of defined(CONFIG_RG_WLAN_HWNAT_ACCELERATION) && !defined(CONFIG_ARCH_LUNA_SLAVE)

