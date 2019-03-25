/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation.
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

#include "mp_precomp.h"
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
#if RT_PLATFORM == PLATFORM_MACOSX
#include "phydm_precomp.h"
#else
#include "../phydm_precomp.h"
#endif
#else
#include "../../phydm_precomp.h"
#endif

#if (RTL8195B_SUPPORT == 1)
/*---------------------------Define Local Constant---------------------------*/


static boolean overflowflag = false;
#define dpk_forcein_sram4 1
#define txgap_ref_index 0x0

/*---------------------------Define Local Constant---------------------------*/
void do_txgapk_8195b(
	void *dm_void,
	u8 delta_thermal_index,
	u8 thermal_value,
	u8 threshold)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	/*boolean		is_recovery = (boolean) delta_thermal_index;*/
	phy_txgap_calibrate_8195b(dm, true);
}

void _txgapk_backup_8195b(
	struct dm_struct *dm,
	u32 *backup_txgap,
	u32 *backup_txgap_reg,
	u8 txgapk_reg_num)
{
	u32 i;

	for (i = 0; i < txgapk_reg_num; i++)
		backup_txgap[i] = odm_read_4byte(dm, backup_txgap_reg[i]);
}

void _txgapk_restore_8195b(
	struct dm_struct *dm,
	u32 *backup_txgap,
	u32 *backup_txgap_reg,
	u8 txgapk_reg_num)
{
	u32 i;

	for (i = 0; i < txgapk_reg_num; i++)
		odm_write_4byte(dm, backup_txgap_reg[i], backup_txgap[i]);
}

void _txgapk_setting_8195b(
	struct dm_struct *dm,
	u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	/*RF*/
	odm_set_rf_reg(dm, RF_PATH_A, 0xEF, RFREGOFFSETMASK, 0x80000);
	odm_set_rf_reg(dm, RF_PATH_A, 0x33, RFREGOFFSETMASK, 0x00024);
	odm_set_rf_reg(dm, RF_PATH_A, 0x3E, RFREGOFFSETMASK, 0x0003F);
	odm_set_rf_reg(dm, RF_PATH_A, 0x3F, RFREGOFFSETMASK, 0xCBFCE);
	odm_set_rf_reg(dm, RF_PATH_A, 0xEF, RFREGOFFSETMASK, 0x00000);
	switch (*dm->band_type) {
	case ODM_BAND_2_4G:
		break;
	case ODM_BAND_5G:
		odm_set_rf_reg(dm, RF_PATH_A, 0x8f, RFREGOFFSETMASK, 0xA9C00);
		odm_set_rf_reg(dm, RF_PATH_A, 0xdf, RFREGOFFSETMASK, 0x00809);
		odm_set_rf_reg(dm, RF_PATH_A, 0x00, RFREGOFFSETMASK, 0x4001c);
		odm_write_4byte(dm, 0x1bcc, 0x00000009); /*try the iqk swing*/
		odm_write_4byte(dm, 0x1b20, 0x00040008);
		odm_write_4byte(dm, 0x1b24, 0x00040848);
		odm_set_bb_reg(dm, 0x1b2c, 0xfff, 0x010);
		odm_set_bb_reg(dm, 0x1b2c, 0xfff0000, 0x010);
		odm_write_4byte(dm, 0x1b14, 0x00001000);
		odm_write_4byte(dm, 0x1b1c, 0x82193d31);
		odm_write_1byte(dm, 0x1b22, 0x04); /*sync with RF0x33[3:0]*/
		odm_write_1byte(dm, 0x1b26, 0x04); /*sync with RF0x33[3:0]*/
		odm_write_1byte(dm, 0x1b2c, 0x03);
		odm_set_bb_reg(dm, 0x1b2c, 0xfff, 0x010);
		odm_set_bb_reg(dm, 0x1b2c, 0xfff, 0x010);
		odm_write_4byte(dm, 0x1b38, 0x20000000);
		odm_write_4byte(dm, 0x1b3c, 0x20000000);
		odm_write_4byte(dm, 0xc00, 0x4);
		RF_DBG(dm, DBG_RF_IQK,
		       "[TXCAPK](1) txgap calibration setting!!!\n");
		break;
	}
}

u32 _txgapk_one_shot_8195b(
	struct dm_struct *dm,
	u8 path,
	u32 reg0x56)
{
	boolean txgapK_notready = true;
	u8 delay_count = 0x0;
	u32 txgapK_tmp1 = 0x1, txgapK_tmp2 = 0x2;
	u8 offset;
	u32 reg_1bb8;
	u32 rx_dsp_power;

	reg_1bb8 = odm_read_4byte(dm, 0x1bb8);
	/*clear the flag*/
	odm_write_1byte(dm, 0x1bd6, 0x0b);
	odm_set_bb_reg(dm, 0x1bfc, BIT(1), 0x0);
	txgapK_notready = true;
	delay_count = 0x0;
	/* get tx gain*/
	odm_write_1byte(dm, 0x1b2b, 0x00);
	odm_write_1byte(dm, 0x1bb8, 0x00);
	odm_set_rf_reg(dm, path, 0xdf, RFREGOFFSETMASK, 0x00802);
	odm_set_rf_reg(dm, path, 0x8f, RFREGOFFSETMASK, 0xa9c00);
	odm_set_rf_reg(dm, path, 0x56, RFREGOFFSETMASK, reg0x56);
	odm_write_4byte(dm, 0x1bb8, 0x00100000);
	/*ODM_sleep_us(10);*/
	ODM_delay_us(10);
	/* one-shot-1*/
	odm_write_4byte(dm, 0x1b34, 0x1);
	odm_write_4byte(dm, 0x1b34, 0x0);
#if 1
	while (txgapK_notready) {
		odm_write_1byte(dm, 0x1bd6, 0x0b);
		if ((boolean)odm_get_bb_reg(dm, 0x1bfc, BIT(1)))
			txgapK_notready = false;
		else
			txgapK_notready = true;

		if (txgapK_notready) {
			ODM_delay_us(100);
			delay_count++;
		}

		if (delay_count >= 20) {
			RF_DBG(dm, DBG_RF_IQK,
			       "[TXGAPK] (3)txgapktimeout,delay_count=0x%x !!!\n",
			       delay_count);
			txgapK_notready = false;
			break;
		}
	}
#else
	ODM_sleep_ms(1);
	if ((boolean)odm_get_bb_reg(dm, 0x1bfc, BIT(1)))
		txgapK_notready = false;
	else
		txgapK_notready = true;

#endif

	if (!txgapK_notready) {
		odm_write_4byte(dm, 0x1bd6, 0x5);
		txgapK_tmp1 = odm_read_4byte(dm, 0x1bfc) >> 27;
		odm_write_4byte(dm, 0x1bd6, 0xe);
		txgapK_tmp2 = odm_read_4byte(dm, 0x1bfc);
		if (txgapK_tmp1 == 0)
			offset = 0x0;
		else if (txgapK_tmp1 < 2)
			offset = 0x1;
		else if (txgapK_tmp1 < 4)
			offset = 0x2;
		else
			offset = 0x3;

		if (txgapK_tmp1 == 0x0)
			rx_dsp_power = txgapK_tmp2;
		else {
			txgapK_tmp1 = txgapK_tmp1 << (32 - offset);
			txgapK_tmp2 = txgapK_tmp2 >> offset;
			rx_dsp_power = txgapK_tmp1 + txgapK_tmp2;
			overflowflag = true;
		}
	}
	odm_write_4byte(dm, 0x1bb8, reg_1bb8);

	return rx_dsp_power;
}

void _phy_txgapk_calibrate_8195b(
	void *dm_void,
	u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u1Byte rf_path, rf0_idx, rf0_idx_current, rf0_idx_next, i, delta_gain_retry = 3;
	s1Byte delta_gain_gap_pre, delta_gain_gap[1][11];
	u4Byte rf56_current, rf56_next, psd_value_current, psd_value_next;
	u4Byte psd_gap, rf56_current_temp[1][11];
	s32 rf33[1][11];

	memset(rf33, 0x0, sizeof(rf33));

	_txgapk_setting_8195b(dm, RF_PATH_A);

	for (rf_path = RF_PATH_A; rf_path <= RF_PATH_A; rf_path++) {
		if (rf_path == RF_PATH_A)
			odm_set_bb_reg(dm, 0xc00, 0xff, 0x4); /*disable 3-wire*/
		ODM_delay_us(100);
		for (rf0_idx = 1; rf0_idx <= 10; rf0_idx++) {
			rf0_idx_current = 3 * (rf0_idx - 1) + 1;
			odm_set_rf_reg(dm, rf_path, 0x0, 0xff, rf0_idx_current);
			ODM_delay_us(100);
			rf56_current_temp[rf_path][rf0_idx] = odm_get_rf_reg(dm, rf_path, 0x56, 0xfff);
			rf56_current = rf56_current_temp[rf_path][rf0_idx];
			rf0_idx_next = 3 * rf0_idx + 1;
			odm_set_rf_reg(dm, rf_path, 0x0, 0xff, rf0_idx_next);
			ODM_delay_us(100);
			rf56_next = odm_get_rf_reg(dm, rf_path, 0x56, 0xfff);
			RF_DBG(dm, DBG_RF_IQK,
			       "[TGGC] rf56_current[%d][%d]=0x%x rf56_next[%d][%d]=0x%x\n",
			       rf_path, rf0_idx, rf56_current, rf_path, rf0_idx,
			       rf56_next);
			if ((rf56_current >> 5) == (rf56_next >> 5)) {
				delta_gain_gap[rf_path][rf0_idx] = 0;
				RF_DBG(dm, DBG_RF_IQK,
				       "[TGGC] rf56_current[11:5] == rf56_next[%d][%d][11:5]=0x%x delta_gain_gap[%d][%d]=%d\n",
				       rf_path, rf0_idx, (rf56_next >> 5),
				       rf_path, rf0_idx,
				       delta_gain_gap[rf_path][rf0_idx]);
			} else {
				RF_DBG(dm, DBG_RF_IQK,
				       "[TGGC] rf56_current[%d][%d][11:5]=0x%x != rf56_next[%d][%d][11:5]=0x%x\n",
				       rf_path, rf0_idx, (rf56_current >> 5),
				       rf_path, rf0_idx, (rf56_next >> 5));
				for (i = 0; i < delta_gain_retry; i++) {
					psd_value_current = _txgapk_one_shot_8195b(dm, rf_path, rf56_current);
					psd_value_next = _txgapk_one_shot_8195b(dm, rf_path, rf56_next - 2);
					psd_gap = psd_value_next / (psd_value_current / 1000);
#if 0
						if (psd_gap > 1413)
							delta_gain_gap[rf_path][rf0_idx] = 1;
						else if (psd_gap > 1122)
							delta_gain_gap[rf_path][rf0_idx] = 0;
						else
							delta_gain_gap[rf_path][rf0_idx] = -1;
#endif
					if (psd_gap > 1445)
						delta_gain_gap[rf_path][rf0_idx] = 1;
					else if (psd_gap > 1096)
						delta_gain_gap[rf_path][rf0_idx] = 0;
					else
						delta_gain_gap[rf_path][rf0_idx] = -1;
					if (i == 0)
						delta_gain_gap_pre = delta_gain_gap[rf_path][rf0_idx];
					RF_DBG(dm, DBG_RF_IQK,
					       "[TGGC] psd_value_current=0x%x psd_value_next=0x%x psd_value_next/psd_value_current=%d delta_gain_gap[%d][%d]=%d\n",
					       psd_value_current, psd_value_next, psd_gap, rf_path, rf0_idx, delta_gain_gap[rf_path][rf0_idx]);
					if (!(i == 0 && delta_gain_gap[rf_path][rf0_idx] == 0))
						if (delta_gain_gap_pre != delta_gain_gap[rf_path][rf0_idx]) {
							delta_gain_gap[rf_path][rf0_idx] = 0;
							RF_DBG(dm, DBG_RF_IQK,
							       "[TGGC] delta_gain_gap_pre(%d) != delta_gain_gap[%d][%d](%d) time=%d\n",
							       delta_gain_gap_pre, rf_path, rf0_idx, delta_gain_gap[rf_path][rf0_idx], i);
						} else {
							RF_DBG(dm, DBG_RF_IQK,
							       "[TGGC] delta_gain_gap_pre(%d) == delta_gain_gap[%d][%d](%d) time=%d\n",
							       delta_gain_gap_pre, rf_path, rf0_idx, delta_gain_gap[rf_path][rf0_idx], i);
						}
				}
			}
		}

		if (rf_path == RF_PATH_A)
			odm_set_bb_reg(dm, 0xc00, 0xff, 0x7); /*enable 3-wire*/
		ODM_delay_us(100);
	}

	for (rf_path = RF_PATH_A; rf_path <= RF_PATH_A; rf_path++) {
		odm_set_rf_reg(dm, rf_path, 0xef, bRFRegOffsetMask, 0x00100);
		for (rf0_idx = 1; rf0_idx <= 10; rf0_idx++) {
			rf33[rf_path][rf0_idx] = rf33[rf_path][rf0_idx] + (rf56_current_temp[rf_path][rf0_idx] & 0x1f);
			for (i = rf0_idx; i <= 10; i++)
				rf33[rf_path][rf0_idx] = rf33[rf_path][rf0_idx] + delta_gain_gap[rf_path][i];
			if (rf33[rf_path][rf0_idx] >= 0x1d)
				rf33[rf_path][rf0_idx] = 0x1d;
			else if (rf33[rf_path][rf0_idx] <= 0x2)
				rf33[rf_path][rf0_idx] = 0x2;
			rf33[rf_path][rf0_idx] = rf33[rf_path][rf0_idx] + ((rf0_idx - 1) * 0x4000) + (rf56_current_temp[rf_path][rf0_idx] & 0xfffe0);

			RF_DBG(dm, DBG_RF_IQK,
			       "[TGGC] rf56[%d][%d]=0x%05x rf33[%d][%d]=0x%05x\n",
			       rf_path, rf0_idx,
			       rf56_current_temp[rf_path][rf0_idx], rf_path,
			       rf0_idx, rf33[rf_path][rf0_idx]);

			odm_set_rf_reg(dm, rf_path, 0x33, bRFRegOffsetMask, rf33[rf_path][rf0_idx]);
		}
		odm_set_rf_reg(dm, rf_path, 0xef, bRFRegOffsetMask, 0x00000);
	}
}

void phy_txgap_calibrate_8195b(
	void *dm_void,
	boolean clear)
{
	u32 MAC_backup[MAC_REG_NUM_8195B], BB_backup[BB_REG_NUM_8195B], RF_backup[RF_REG_NUM_8195B][1];
	u32 backup_mac_reg[MAC_REG_NUM_8195B] = {0x520, 0x550, 0x1518};
	u32 backup_bb_reg[BB_REG_NUM_8195B] = {0x808, 0x90c, 0xc00, 0xcb0, 0xcb4, 0xcbc, 0x1990, 0x9a4, 0xa04};
	u32 backup_rf_reg[RF_REG_NUM_8195B] = {0xdf, 0xde, 0x8f, 0x0, 0x1};

	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	struct _ADAPTER *p_adapter = dm->adapter;

#if (MP_DRIVER == 1)
#ifdef CONFIG_MP_INCLUDED
	PMPT_CONTEXT p_mpt_ctx = &(p_adapter->mppriv.MptCtx);
#endif
#endif

	struct _hal_rf_ *p_rf = &(dm->rf_table);

#if (DM_ODM_SUPPORT_TYPE == ODM_IOT)
	if (!(p_rf->rf_supportability & HAL_RF_IQK))
		return;
#endif

#if MP_DRIVER == 1
#ifdef CONFIG_MP_INCLUDED
	if (p_mpt_ctx->bSingleTone || p_mpt_ctx->bCarrierSuppression)
		return;
#endif
#endif

	if ((dm->rf_table.rf_supportability & HAL_RF_TXGAPK))
		if ((iqk_info->lok_fail[RF_PATH_A] == 0) &
		    (iqk_info->iqk_fail_report[0][RF_PATH_A][TXIQK] == 0) &
		    (iqk_info->iqk_fail_report[0][RF_PATH_A][RXIQK] == 0))
			_phy_txgapk_calibrate_8195b(dm, RF_PATH_A);
	return;
}

#endif
