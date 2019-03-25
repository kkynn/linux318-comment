
/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	HalMacFunc.c
	
Abstract:
	Define MAC function support 
	for Driver
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2015-04-29 Eric            Create.	
--*/

#include "HalPrecomp.h"

#if defined(CONFIG_WLAN_HAL) 
void MACFM_software_init(struct rtl8192cd_priv *priv)
{
    unsigned long ability = 0;
    //priv->pshare->_dmODM.priv = priv;
 
#ifdef CONFIG_WLAN_HAL_8881A
    	if (GET_CHIP_VER(priv) == VERSION_8881A) {		
        	ability = 
                    0;
	}
#endif //CONFIG_WLAN_HAL_8881A

#ifdef CONFIG_WLAN_HAL_8192EE
	if (GET_CHIP_VER(priv) == VERSION_8192E) {
		ability = 
#if CFG_HAL_TX_AMSDU
        MAC_FUN_HW_SUPPORT_TX_AMSDU                 |
#endif
			0;
	}
#endif //CONFIG_WLAN_HAL_8192EE

#ifdef CONFIG_WLAN_HAL_8814AE
	if (GET_CHIP_VER(priv) == VERSION_8814A) {
		ability = 
#if CFG_HAL_HW_TX_SHORTCUT_REUSE_TXDESC
            MAC_FUN_HW_TX_SHORTCUT_REUSE_TXDESC		 	|
#endif            
			MAC_FUN_HW_TX_SHORTCUT_HDR_CONV             |
			MAC_FUN_HW_SUPPORT_EACH_VAP_INT		        |
			MAC_FUN_HW_SUPPORT_RELEASE_ONE_PACKET		|
			MAC_FUN_HW_HW_FILL_MACID			        |
			MAC_FUN_HW_HW_DETEC_POWER_STATE				|
			MAC_FUN_HW_SUPPORT_MULTICAST_BMC_ENHANCE	|
#if CFG_HAL_TX_AMSDU
			MAC_FUN_HW_SUPPORT_TX_AMSDU                 |
#endif
            0;
	}
#endif //CONFIG_WLAN_HAL_8814AE

#ifdef CONFIG_WLAN_HAL_8197F
    if (GET_CHIP_VER(priv) == VERSION_8197F) {
        ability = 
#if CFG_HAL_HW_TX_SHORTCUT_REUSE_TXDESC
            MAC_FUN_HW_TX_SHORTCUT_REUSE_TXDESC		 	|
#endif
			MAC_FUN_HW_TX_SHORTCUT_HDR_CONV             |
			MAC_FUN_HW_HW_SEQ                           |
            MAC_FUN_HW_SUPPORT_EACH_VAP_INT             |
            MAC_FUN_HW_SUPPORT_H2C_PACKET               |
            MAC_FUN_HW_SUPPORT_AXI_EXCEPTION            |            
            MAC_FUN_HW_SUPPORT_AP_OFFLOAD               | //for BIT_CPWM2 INT
            0;
    }

#endif //CONFIG_WLAN_HAL_8197F

#ifdef CONFIG_WLAN_HAL_8192FE
    if (GET_CHIP_VER(priv) == VERSION_8192F) {
        ability = 
#if CFG_HAL_TX_AMSDU
			MAC_FUN_HW_SUPPORT_TX_AMSDU 				|
#endif
            //MAC_FUN_HW_SUPPORT_EACH_VAP_INT             |
            MAC_FUN_HW_HW_DETEC_POWER_STATE             |
            MAC_FUN_HW_HW_FILL_MACID                    |
            MAC_FUN_HW_SUPPORT_AGING_FUNC_OFFLOAD       |        
            MAC_FUN_HW_SUPPORT_AP_OFFLOAD               | //for BIT_CPWM2 INT
            MAC_FUN_HW_SUPPORT_AP_SWPS_OFFLOAD          |
            0;
    }

#endif //CONFIG_WLAN_HAL_8192FE


#ifdef CONFIG_WLAN_HAL_8822BE
if (GET_CHIP_VER(priv) == VERSION_8822B) {
    ability = 
        MAC_FUN_HW_HW_DETEC_POWER_STATE             |
#if CFG_HAL_HW_TX_SHORTCUT_REUSE_TXDESC
        MAC_FUN_HW_TX_SHORTCUT_REUSE_TXDESC         |
#endif      
		MAC_FUN_HW_TX_SHORTCUT_HDR_CONV             |
		MAC_FUN_HW_TX_SHORTCUT_HDR_CONV_LLC         |
		MAC_FUN_HW_HW_SEQ                           |
        MAC_FUN_HW_SUPPORT_EACH_VAP_INT             |
        MAC_FUN_HW_SUPPORT_RELEASE_ONE_PACKET       |
        MAC_FUN_HW_HW_FILL_MACID                    |
        MAC_FUN_HW_HW_DETEC_POWER_STATE             |
        MAC_FUN_HW_SUPPORT_MULTICAST_BMC_ENHANCE    |
#if CFG_HAL_TX_AMSDU
        MAC_FUN_HW_SUPPORT_TX_AMSDU                 |
#endif
        //MAC_FUN_HW_SUPPORT_H2C_PACKET               |
        //MAC_FUN_HW_SUPPORT_AP_OFFLOAD               |
        0;
}

#endif //CONFIG_WLAN_HAL_8822BE

#ifdef CONFIG_WLAN_HAL_8821CE
if (GET_CHIP_VER(priv) == VERSION_8821C) {
    ability = 
        MAC_FUN_HW_HW_DETEC_POWER_STATE             |
#if CFG_HAL_HW_TX_SHORTCUT_REUSE_TXDESC
        MAC_FUN_HW_TX_SHORTCUT_REUSE_TXDESC         |
#endif      
		MAC_FUN_HW_TX_SHORTCUT_HDR_CONV             |
		MAC_FUN_HW_TX_SHORTCUT_HDR_CONV_LLC         |
		MAC_FUN_HW_HW_SEQ                           |
        MAC_FUN_HW_SUPPORT_EACH_VAP_INT             |
        MAC_FUN_HW_SUPPORT_RELEASE_ONE_PACKET       |
        MAC_FUN_HW_HW_FILL_MACID                    |
        MAC_FUN_HW_HW_DETEC_POWER_STATE             |
        MAC_FUN_HW_SUPPORT_MULTICAST_BMC_ENHANCE    |      
#if CFG_HAL_TX_AMSDU
        MAC_FUN_HW_SUPPORT_TX_AMSDU                 |
#endif
        //MAC_FUN_HW_SUPPORT_H2C_PACKET               |
        MAC_FUN_HW_SUPPORT_AP_OFFLOAD               |
        0;
}

#endif //CONFIG_WLAN_HAL_8821CE

    priv->pshare->hal_SupportMACfunction = ability;
	GDEBUG("wifi hal support Mac function = 0x%x\n", ability);
}


void MACHAL_version_init(
IN  HAL_PADAPTER Adapter
)
{
    PHAL_DATA_TYPE              pHalData = _GET_HAL_DATA(Adapter);

    pHalData->MacVersion.is_MAC_v1 = IS_HARDWARE_TYPE_MAC_V1(Adapter);
    pHalData->MacVersion.is_MAC_v2 = IS_HARDWARE_TYPE_MAC_V2(Adapter);        
    pHalData->MacVersion.MACHALSupport = IS_HARDWARE_MACHAL_SUPPORT(Adapter);            
}

#endif // CONFIG_WLAN_HAL



