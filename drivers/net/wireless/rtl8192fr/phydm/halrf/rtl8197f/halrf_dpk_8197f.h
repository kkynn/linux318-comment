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

#ifndef __HALRF_DPK_8197F_H__
#define __HALRF_DPK_8197F_H__

/*--------------------------Define Parameters-------------------------------*/
#define DPK_RF_PATH_NUM_8197F 2
#define DPK_GROUP_NUM_8197F 3
#define DPK_MAC_REG_NUM_8197F 5
#define DPK_BB_REG_NUM_8197F 25
#define DPK_RF_REG_NUM_8197F 3
#define DPK_PAS_DBG_8197F 0
#define DPK_SRAM_IQ_DBG_8197F 0
#define DPK_SRAM_read_DBG_8197F 0
#define DPK_SRAM_write_DBG_8197F 0
#define DPK_DO_PATH_A 1
#define DPK_DO_PATH_B 1
#define RF_T_METER_8197F RF_0x42
#define DPK_THRESHOLD_8197F 6


VOID phy_dpk_track_8197f(
	struct dm_struct *dm);

VOID phy_dpkoff_8197f(
	struct dm_struct *dm);

VOID phy_dpkon_8197f(
	struct dm_struct *dm);

void phy_path_a_dpk_init_8197f(
	struct dm_struct *dm);

void phy_path_b_dpk_init_8197f(
	struct dm_struct *dm);

u8 phy_dpk_channel_transfer_8197f(
	struct dm_struct *dm);

u8 phy_lut_sram_read_8197f(
	struct dm_struct *dm,
	u8 k);

VOID phy_lut_sram_write_8197f(
	struct dm_struct *dm);

VOID phy_path_a_dpk_enable_8197f(
	struct dm_struct *dm);

VOID phy_path_b_dpk_enable_8197f(
	struct dm_struct *dm);

VOID phy_dpk_enable_disable_8197f(
	struct dm_struct *dm);

VOID dpk_sram_read_8197f(
	void *dm_void);

VOID dpk_reload_8197f(
	void *dm_void);

VOID do_dpk_8197f(
	void *dm_void);


#endif /* #ifndef __HAL_PHY_RF_8197F_H__*/

