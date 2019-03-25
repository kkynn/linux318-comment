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

#ifndef __HALRF_8723B_H__
#define __HALRF_8723B_H__

/*--------------------------Define Parameters-------------------------------*/
#define IQK_DELAY_TIME_8723B 20 /* ms */
#define IQK_DEFERRED_TIME_8723B 4
#define index_mapping_NUM_8723B 15
#define AVG_THERMAL_NUM_8723B 4
#define RF_T_METER_8723B 0x42

void configure_txpower_track_8723b(struct txpwrtrack_cfg *config);

void do_iqk_8723b(void *dm_void, u8 delta_thermal_index, u8 thermal_value,
		  u8 threshold);

void odm_tx_pwr_track_set_pwr_8723b(void *dm_void, enum pwrtrack_method method,
				    u8 rf_path, u8 channel_mapped_index);

/* 1 7.	IQK */

void phy_iq_calibrate_8723b(void *dm_void, boolean is_recovery);

void odm_set_iqc_by_rfpath(struct dm_struct *dm, u32 rf_path);

/*
 * LC calibrate
 */
void phy_lc_calibrate_8723b(void *dm_void);

void _phy_save_adda_registers_8723b(
	struct dm_struct *dm,
	u32 *adda_reg,
	u32 *adda_backup,
	u32 register_num);

void _phy_path_adda_on_8723b(
	struct dm_struct *dm,
	u32 *adda_reg,
	boolean is_path_a_on,
	boolean is2T);

void _phy_mac_setting_calibration_8723b(
	struct dm_struct *dm,
	u32 *mac_reg,
	u32 *mac_backup);

void halrf_rf_lna_setting_8723b(struct dm_struct *dm, enum halrf_lna_set type);

#endif /*#ifndef __HALRF_8723B_H__*/