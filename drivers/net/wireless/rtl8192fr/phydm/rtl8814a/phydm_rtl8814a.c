/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

/* ************************************************************
 * include files
 * ************************************************************ */

#include "mp_precomp.h"
#include "../phydm_precomp.h"

#if (RTL8814A_SUPPORT == 1)

u8 phydm_spur_nbi_setting_8814a(struct dm_struct *dm)
{
	u8 set_result = 0;

	/*dm->channel means central frequency, so we can use 20M as input*/
	if (dm->rfe_type == 0 || dm->rfe_type == 1 || dm->rfe_type == 6 || dm->rfe_type == 7) {
		/*channel asked by RF Jeff*/
		if (*dm->channel == 14)
			set_result = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 40, 2480, PHYDM_DONT_CARE);
		else if (*dm->channel >= 4 && *dm->channel <= 8)
			set_result = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 40, 2440, PHYDM_DONT_CARE);
		else
			set_result = phydm_nbi_setting(dm, FUNC_DISABLE, *dm->channel, 40, 2440, PHYDM_DONT_CARE);
	}
	PHYDM_DBG(dm, ODM_COMP_API, "%s, set_result = 0x%x, channel = %d\n",
		  __func__, set_result, *dm->channel);
	dm->nbi_set_result = set_result;
	return set_result;
}

void phydm_dynamic_nbi_switch_8814a(struct dm_struct *dm)
{
	/*if rssi < 15%, disable nbi notch filter, if rssi > 20%, enable nbi notch filter*/
	/*add by YuChen 20160218*/
	if (dm->rfe_type == 0 || dm->rfe_type == 1 || dm->rfe_type == 6 || dm->rfe_type == 7) {
		if (dm->nbi_set_result == PHYDM_SET_SUCCESS) {
			if (dm->rssi_min <= 15)
				odm_set_bb_reg(dm, R_0x87c, BIT(13), 0x0);
			else if (dm->rssi_min >= 20)
				odm_set_bb_reg(dm, R_0x87c, BIT(13), 0x1);
		}
		PHYDM_DBG(dm, ODM_COMP_API, "%s\n", __func__);
	}
}

void phydm_hwsetting_8814a(struct dm_struct *dm)
{
	phydm_dynamic_nbi_switch_8814a(dm);
}

#endif /* RTL8814A_SUPPORT == 1 */
