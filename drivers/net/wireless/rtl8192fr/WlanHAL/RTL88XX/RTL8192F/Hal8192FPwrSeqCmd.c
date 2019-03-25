/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	Hal8192FAPwrSeqCmd.c
	
Abstract:
	This file includes all kinds of Power Action event for RTL8192F and 
	corresponding hardware configurtions which are released from HW SD.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2017-09-22 scko            Create.
	
--*/

#if !defined(__ECOS) && !defined(CPTCFG_CFG80211_MODULE)
#include "HalPrecomp.h"
#else
#include "../../HalPrecomp.h"
#endif
/*
 *	drivers should parse below arrays and do the corresponding actions
 */

/*	Power on  Array	*/
WLAN_PWR_CFG rtl8192F_power_on_flow[RTL8192F_TRANS_CARDEMU_TO_ACT_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	RTL8192F_TRANS_CARDEMU_TO_ACT
	RTL8192F_TRANS_END
};

/*	Radio off Array	*/
WLAN_PWR_CFG rtl8192F_radio_off_flow[RTL8192F_TRANS_ACT_TO_CARDEMU_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	RTL8192F_TRANS_ACT_TO_CARDEMU
	RTL8192F_TRANS_END
};

/*	Card Disable Array	*/
WLAN_PWR_CFG rtl8192F_card_disable_flow[RTL8192F_TRANS_ACT_TO_CARDEMU_STEPS+RTL8192F_TRANS_CARDEMU_TO_CARDDIS_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	RTL8192F_TRANS_ACT_TO_CARDEMU
	RTL8192F_TRANS_CARDEMU_TO_CARDDIS
	RTL8192F_TRANS_END
};

/*	Card Enable Array	*/
WLAN_PWR_CFG rtl8192F_card_enable_flow[RTL8192F_TRANS_CARDDIS_TO_CARDEMU_STEPS+RTL8192F_TRANS_CARDEMU_TO_ACT_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	RTL8192F_TRANS_CARDDIS_TO_CARDEMU
	RTL8192F_TRANS_CARDEMU_TO_ACT
	RTL8192F_TRANS_END
};

/*	Suspend Array	*/
WLAN_PWR_CFG rtl8192F_suspend_flow[RTL8192F_TRANS_ACT_TO_CARDEMU_STEPS+RTL8192F_TRANS_CARDEMU_TO_SUS_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	RTL8192F_TRANS_ACT_TO_CARDEMU
	RTL8192F_TRANS_CARDEMU_TO_SUS
	RTL8192F_TRANS_END
};

/*	Resume Array		*/
WLAN_PWR_CFG rtl8192F_resume_flow[RTL8192F_TRANS_SUS_TO_CARDEMU_STEPS+RTL8192F_TRANS_CARDEMU_TO_ACT_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	RTL8192F_TRANS_SUS_TO_CARDEMU
	RTL8192F_TRANS_CARDEMU_TO_ACT
	RTL8192F_TRANS_END
};



/*	HWPDN Array		*/
WLAN_PWR_CFG rtl8192F_hwpdn_flow[RTL8192F_TRANS_ACT_TO_CARDEMU_STEPS+RTL8192F_TRANS_CARDEMU_TO_PDN_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	RTL8192F_TRANS_ACT_TO_CARDEMU
	RTL8192F_TRANS_CARDEMU_TO_PDN	
	RTL8192F_TRANS_END
};

/*	Enter LPS 	*/
WLAN_PWR_CFG rtl8192F_enter_lps_flow[RTL8192F_TRANS_ACT_TO_LPS_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	//FW behavior
	RTL8192F_TRANS_ACT_TO_LPS	
	RTL8192F_TRANS_END
};

/*	Leave LPS 	*/
WLAN_PWR_CFG rtl8192F_leave_lps_flow[RTL8192F_TRANS_LPS_TO_ACT_STEPS+RTL8192F_TRANS_END_STEPS]=
{
	//FW behavior
	RTL8192F_TRANS_LPS_TO_ACT
	RTL8192F_TRANS_END
};

