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

#ifndef __HALRF_8198F_H__
#define __HALRF_8198F_H__

#define AVG_THERMAL_NUM_8198F 4
#define RF_T_METER_8198F 0x42

void configure_txpower_track_8198f(
	struct _TXPWRTRACK_CFG *p_config);

void odm_tx_pwr_track_set_pwr8198f(
	void *p_dm_void,
	enum pwrtrack_method method,
	u8 rf_path,
	u8 channel_mapped_index);

void get_delta_swing_table_8198f(
	void *p_dm_void,
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
	u8 **temperature_up_a,
	u8 **temperature_down_a,
	u8 **temperature_up_b,
	u8 **temperature_down_b,
	u8 **temperature_up_cck_a,
	u8 **temperature_down_cck_a,
	u8 **temperature_up_cck_b,
	u8 **temperature_down_cck_b
#else
	u8 **temperature_up_a,
	u8 **temperature_down_a,
	u8 **temperature_up_b,
	u8 **temperature_down_b
#endif
	);

void phy_lc_calibrate_8198f(
	void *p_dm_void);

void halrf_rf_lna_setting_8198f(
	struct PHY_DM_STRUCT *p_dm,
	enum phydm_lna_set type);

void phy_set_rf_path_switch_8198f(
	struct dm_struct *dm,
	boolean is_main);

boolean phy_query_rf_path_switch_8198f(
	struct dm_struct *dm);

#endif /* #ifndef __HALRF_8198F_H__ */
