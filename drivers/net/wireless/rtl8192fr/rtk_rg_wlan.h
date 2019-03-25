#ifndef _RTK_RG_WLAN_H_
#define _RTK_RG_WLAN_H_

#ifdef __KERNEL__
#include <linux/netdevice.h>
#endif

#include "8192cd_cfg.h"

#if defined(CONFIG_RG_WLAN_HWNAT_ACCELERATION) && !defined(CONFIG_ARCH_LUNA_SLAVE)

// Refer to: drivers/net/ethernet/realtek/rtl86900/romeDriver/rtk_rg_define.h
#if defined(CONFIG_WLAN_MBSSID_NUM) && (CONFIG_WLAN_MBSSID_NUM==7)  // vap num is 7
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
#define WLAN_DEVICE_NUM     17  //root(1)+vap(7)+wds(8)+vxd(1)
#else
#define WLAN_DEVICE_NUM     16  //root(1)+vap(7)+wds(8)
#endif
#else   //1  vap num is 4
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
#define WLAN_DEVICE_NUM     14  //root(1)+vap(4)+wds(8)+vxd(1)
#else
#define WLAN_DEVICE_NUM     13  //root(1)+vap(4)+wds(8)
#endif
#endif  // end defined(CONFIG_WLAN_MBSSID_NUM) && (CONFIG_WLAN_MBSSID_NUM==7)

// Refer to: drivers/net/ethernet/realtek/rtl86900/romeDriver/rtk_rg_struct.h
typedef enum rtk_rg_mbssidDev_e
{
    RG_RET_MBSSID_NOT_FOUND         = -1,
    RG_RET_MBSSID_MASTER_ROOT_INTF  =  0,
    RG_RET_MBSSID_MASTER_VAP0_INTF,
    RG_RET_MBSSID_MASTER_VAP1_INTF,
    RG_RET_MBSSID_MASTER_VAP2_INTF,
    RG_RET_MBSSID_MASTER_VAP3_INTF,
#if defined(CONFIG_WLAN_MBSSID_NUM) && (CONFIG_WLAN_MBSSID_NUM==7)
    RG_RET_MBSSID_MASTER_VAP4_INTF,
    RG_RET_MBSSID_MASTER_VAP5_INTF,
    RG_RET_MBSSID_MASTER_VAP6_INTF,
#endif
    RG_RET_MBSSID_MASTER_WDS0_INTF,
    RG_RET_MBSSID_MASTER_WDS1_INTF,
    RG_RET_MBSSID_MASTER_WDS2_INTF,
    RG_RET_MBSSID_MASTER_WDS3_INTF,
    RG_RET_MBSSID_MASTER_WDS4_INTF,
    RG_RET_MBSSID_MASTER_WDS5_INTF,
    RG_RET_MBSSID_MASTER_WDS6_INTF,
    RG_RET_MBSSID_MASTER_WDS7_INTF,
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
    RG_RET_MBSSID_MASTER_CLIENT_INTF,
#endif
    RG_RET_MBSSID_SLAVE_ROOT_INTF   = WLAN_DEVICE_NUM,
    RG_RET_MBSSID_SLAVE_VAP0_INTF,
    RG_RET_MBSSID_SLAVE_VAP1_INTF,
    RG_RET_MBSSID_SLAVE_VAP2_INTF,
    RG_RET_MBSSID_SLAVE_VAP3_INTF,
#if defined(CONFIG_WLAN_MBSSID_NUM) && (CONFIG_WLAN_MBSSID_NUM==7)
    RG_RET_MBSSID_SLAVE_VAP4_INTF,
    RG_RET_MBSSID_SLAVE_VAP5_INTF,
    RG_RET_MBSSID_SLAVE_VAP6_INTF,
#endif
    RG_RET_MBSSID_SLAVE_WDS0_INTF,
    RG_RET_MBSSID_SLAVE_WDS1_INTF,
    RG_RET_MBSSID_SLAVE_WDS2_INTF,
    RG_RET_MBSSID_SLAVE_WDS3_INTF,
    RG_RET_MBSSID_SLAVE_WDS4_INTF,
    RG_RET_MBSSID_SLAVE_WDS5_INTF,
    RG_RET_MBSSID_SLAVE_WDS6_INTF,
    RG_RET_MBSSID_SLAVE_WDS7_INTF,
#ifdef CONFIG_RTL_REPEATER_MODE_SUPPORT
    RG_RET_MBSSID_SLAVE_CLIENT_INTF,
#endif
    RG_RET_MBSSID_FLOOD_ALL_INTF    = 100,
}rtk_rg_mbssidDev_t;


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
extern int rtk_rg_wlan_netDevice_set(rtk_rg_mbssidDev_t wlanDevIdx, struct net_device *pDev);
extern int rtk_rg_wlan_netDevice_show(void);
#else // !defined(CONFIG_ARCH_CORTINA_G3) && !defined(CONFIG_ARCH_CORTINA_G3HGU)
int rtk_rg_wlan_netDevice_set(rtk_rg_mbssidDev_t wlanDevIdx, struct net_device *pDev);
int rtk_rg_wlan_netDevice_show(void);
#endif // end of defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)

#endif // end of defined(CONFIG_RG_WLAN_HWNAT_ACCELERATION) && !defined(CONFIG_ARCH_LUNA_SLAVE)

#endif //end of #ifndef _RTK_RG_WLAN_H_
