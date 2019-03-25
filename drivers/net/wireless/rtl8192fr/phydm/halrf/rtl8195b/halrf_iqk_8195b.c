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

/*---------------------------Define Local Constant---------------------------*/
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
void do_iqk_8195b(
	void *dm_void,
	u8 delta_thermal_index,
	u8 thermal_value,
	u8 threshold)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	dm->rf_calibrate_info.thermal_value_iqk = thermal_value;
	halrf_segment_iqk_trigger(dm, true, iqk_info->segment_iqk);
}
#else
/*Originally p_config->do_iqk is hooked phy_iq_calibrate_8195b, but do_iqk_8195b and phy_iq_calibrate_8195b have different arguments*/
void do_iqk_8195b(
	void *dm_void,
	u8 delta_thermal_index,
	u8 thermal_value,
	u8 threshold)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	/*boolean		is_recovery = (boolean) delta_thermal_index;*/
	halrf_segment_iqk_trigger(dm, true, iqk_info->segment_iqk);
}
#endif

boolean
_iqk_check_nctl_done_8195b(
	struct dm_struct *dm,
	u8 path,
	u32 IQK_CMD)
{
	/*this function is only used after the version of nctl8.0*/
	boolean notready = true;
	boolean fail = true;
	u32 delay_count = 0x0;

	while (notready) {
		if (odm_read_1byte(dm, 0x1f7f) == 0x55){
			RF_DBG(dm, DBG_RF_IQK, "0x1f7c[31:24] = %x\n",
			       odm_read_4byte(dm, 0x1f7c));
			notready = false;
		} else
			notready = true;
		if (notready) {
			/*ODM_sleep_ms(1);*/
			ODM_delay_ms(1);
			delay_count++;
		} else {
			fail = (boolean)odm_get_bb_reg(dm, 0x1b08, BIT(26));
			break;
		}
		if (delay_count >= 50) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d IQK timeout!!!\n",
			       path);
			break;
		}
	}
	//odm_write_1byte(dm, 0x1b10, 0x0);
	odm_write_1byte(dm, 0x1f7f, 0x0);

	if (fail == false) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d IQK PASS!!!\n", path);
	} else {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d IQK Fail!!!\n", path);
	}
	return fail;
}

void _phydm_get_read_counter_8195b(struct dm_struct *dm)
{
	u32 counter = 0x0;
#if 0
	while (1) {
		if ((odm_get_rf_reg(dm, RF_PATH_A, 0x8, RFREGOFFSETMASK) == 0xabcde) || counter > 300)
			break;
		counter++;
		ODM_delay_ms(1);
	};
	odm_set_rf_reg(dm, RF_PATH_A, 0x8, RFREGOFFSETMASK, 0x0);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]counter = %d\n", counter);
#endif
}

void _iqk_fail_count_8195b(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i;

	dm->n_iqk_cnt++;
	//if (odm_get_rf_reg(dm, RF_PATH_A, 0x1bf0, BIT(16)) == 1)
	if (odm_get_bb_reg(dm, 0x1bf0, BIT(16)) == 1)
		iqk_info->is_reload = true;
	else
		iqk_info->is_reload = false;

	if (!iqk_info->is_reload) {
		for (i = 0; i < 8; i++) {
			if (odm_get_bb_reg(dm, 0x1bf0, BIT(i)) == 1)
				dm->n_iqk_fail_cnt++;
		}
	}
	RF_DBG(dm, DBG_RF_IQK, "[IQK]All/Fail = %d %d, 0x1be8=0x%x, 0x1bf0 =0x%x\n", dm->n_iqk_cnt,
	       dm->n_iqk_fail_cnt, odm_read_4byte(dm, 0x1be8), odm_read_4byte(dm, 0x1bf0));
}

void _iqk_fill_iqk_report_8195b(
	void *dm_void,
	u8 channel)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 tmp1 = 0x0, tmp2 = 0x0, tmp3 = 0x0;
	u8 i;

	for (i = 0; i < SS_8195B; i++) {
		tmp1 = tmp1 + ((iqk_info->iqk_fail_report[channel][i][TX_IQK] & 0x1) << i);
		tmp2 = tmp2 + ((iqk_info->iqk_fail_report[channel][i][RX_IQK] & 0x1) << (i + 4));
		tmp3 = tmp3 + ((iqk_info->rxiqk_fail_code[channel][i] & 0x3) << (i * 2 + 8));
	}
	odm_write_4byte(dm, 0x1b00, 0x00000008);
	odm_set_bb_reg(dm, 0x1bf0, 0x00ffffff, tmp1 | tmp2 | tmp3);

	for (i = 0; i < SS_8195B; i++)
		odm_write_4byte(dm, 0x1be8 + (i * 4), (iqk_info->rxiqk_agc[channel][(i * 2) + 1] << 16) | iqk_info->rxiqk_agc[channel][i * 2]);
}

void _iqk_iqk_fail_report_8195b(
	struct dm_struct *dm)
{
	u32 tmp1bf0 = 0x0;
	u8 i;

	tmp1bf0 = odm_read_4byte(dm, 0x1bf0);

	for (i = 0; i < 4; i++) {
		if (tmp1bf0 & (0x1 << i))
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			RF_DBG(dm, DBG_RF_IQK, "[IQK] please check S%d TXIQK\n",
			       i);
#else
			panic_printk("[IQK] please check S%d TXIQK\n", i);
#endif
		if (tmp1bf0 & (0x1 << (i + 12)))
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			RF_DBG(dm, DBG_RF_IQK, "[IQK] please check S%d RXIQK\n",
			       i);
#else
			panic_printk("[IQK] please check S%d RXIQK\n", i);
#endif
	}
}

void _iqk_backup_mac_bb_8195b(
	struct dm_struct *dm,
	u32 *MAC_backup,
	u32 *BB_backup,
	u32 *backup_mac_reg,
	u32 *backup_bb_reg,
	u8 num_backup_bb_reg)
{
	u32 i;

	for (i = 0; i < IQK_MAC_REG_NUM_8195B; i++)
		MAC_backup[i] = odm_read_4byte(dm, backup_mac_reg[i]);

	for (i = 0; i < num_backup_bb_reg; i++)
		BB_backup[i] = odm_read_4byte(dm, backup_bb_reg[i]);
	/*	RF_DBG(dm, DBG_RF_IQK, ("[IQK]BackupMacBB Success!!!!\n")); */
}

void _iqk_backup_rf_8195b(
	struct dm_struct *dm,
	u32 RF_backup[][SS_8195B],
	u32 *backup_rf_reg)
{
	u32 i, j;

	for (i = 0; i < IQK_RF_REG_NUM_8195B; i++)
		for (j = 0; j < SS_8195B; j++)
			RF_backup[i][j] = odm_get_rf_reg(dm, (u1Byte)j, backup_rf_reg[i], RFREGOFFSETMASK);
	/*	RF_DBG(dm, DBG_RF_IQK, ("[IQK]BackupRF Success!!!!\n")); */
}

void _iqk_agc_bnd_int_8195b(
	struct dm_struct *dm)
{
	RF_DBG(dm, DBG_RF_IQK, "[IQK]init. rx agc bnd\n");
	/*initialize RX AGC bnd, it must do after bbreset*/
	odm_write_4byte(dm, 0x1b00, 0x00000008);
	odm_write_4byte(dm, 0x1b00, 0x00A70008);
	odm_write_4byte(dm, 0x1b00, 0x00150008);
	odm_write_4byte(dm, 0x1b00, 0x00000008);
}

void _iqk_bb_reset_8195b(
	struct dm_struct *dm)
{
#if 0
	boolean cca_ing = false;
	u32 count = 0;

	odm_set_rf_reg(dm, RF_PATH_A, 0x0, RFREGOFFSETMASK, 0x10000);
	odm_set_bb_reg(dm, 0x8f8, BIT(27) | BIT26 | BIT25 | BIT24 | BIT23 | BIT22 | BIT21 | BIT20, 0x0);

	while (1) {
		odm_write_4byte(dm, 0x8fc, 0x0);
		odm_set_bb_reg(dm, 0x198c, 0x7, 0x7);
		cca_ing = (boolean)odm_get_bb_reg(dm, 0xfa0, BIT(3));

		if (count > 30)
			cca_ing = false;

		if (cca_ing) {
			ODM_sleep_ms(1);
			count++;
		} else {
			odm_write_1byte(dm, 0x808, 0x0); /*RX ant off*/
			odm_set_bb_reg(dm, 0xa04, BIT(27) | BIT26 | BIT25 | BIT24, 0x0); /*CCK RX path off*/

			/*BBreset*/
			odm_set_bb_reg(dm, 0x0, BIT(16), 0x0);
			odm_set_bb_reg(dm, 0x0, BIT(16), 0x1);

			if (odm_get_bb_reg(dm, 0x660, BIT(16)))
				odm_write_4byte(dm, 0x6b4, 0x89000006);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]BBreset!!!!\n");
			break;
		}
	}
#endif
}

void _iqk_afe_setting_8195b(
	struct dm_struct *dm,
	boolean do_iqk)
{
	if (do_iqk) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]AFE setting for IQK mode!!!!\n");
		/*AFE setting*/
		odm_write_4byte(dm, 0xc60, 0x70000000);
		odm_write_4byte(dm, 0xc60, 0x700F005A);
		// for page 1b can read/write
		odm_write_4byte(dm, 0x1c44, 0xa34300F3);
		odm_write_4byte(dm, 0xc94, 0x01000101);
	} else {
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]AFE setting for Normal mode!!!!\n");
		odm_write_4byte(dm, 0xc60, 0x70000000);
		odm_write_4byte(dm, 0xc60, 0x70070040);
	}
	/*0x9a4[31]=0: Select da clock*/
	//odm_set_bb_reg(dm, 0x9a4, BIT(31), 0x0);
}

void _iqk_restore_mac_bb_8195b(
	struct dm_struct *dm,
	u32 *MAC_backup,
	u32 *BB_backup,
	u32 *backup_mac_reg,
	u32 *backup_bb_reg,
	u8 num_backup_bb_reg)
{
	u32 i;

	for (i = 0; i < IQK_MAC_REG_NUM_8195B; i++)
		odm_write_4byte(dm, backup_mac_reg[i], MAC_backup[i]);
	for (i = 0; i < num_backup_bb_reg; i++)
		odm_write_4byte(dm, backup_bb_reg[i], BB_backup[i]);
}

void _iqk_restore_rf_8195b(
	struct dm_struct *dm,
	u32 *backup_rf_reg,
	u32 RF_backup[][SS_8195B])
{
	u32 i;

	odm_set_rf_reg(dm, RF_PATH_A, 0xef, RFREGOFFSETMASK, 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, 0xee, RFREGOFFSETMASK, 0x0);
	//odm_set_rf_reg(dm, RF_PATH_A, 0xdf, RFREGOFFSETMASK, 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, 0xde, RFREGOFFSETMASK, 0x0);
	//odm_set_rf_reg(dm, RF_PATH_A, 0xdf, RFREGOFFSETMASK, RF_backup[0][RF_PATH_A] & (~BIT(4)));
	//odm_set_rf_reg(dm, RF_PATH_A, 0xde, RFREGOFFSETMASK, RF_backup[1][RF_PATH_A] & (~BIT(4)));

	for (i = 0; i < IQK_RF_REG_NUM_8195B ; i++)
		odm_set_rf_reg(dm, RF_PATH_A, backup_rf_reg[i], RFREGOFFSETMASK, RF_backup[i][RF_PATH_A]);

	//odm_set_rf_reg(dm, RF_PATH_A, 0x1, RFREGOFFSETMASK, (RF_backup[4][RF_PATH_A] & (~BIT(0))));

	/*RF_DBG(dm, DBG_RF_IQK, ("[IQK]RestoreRF Success!!!!\n")); */
}

void _iqk_backup_iqk_8195b(
	struct dm_struct *dm,
	u8 step,
	u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i, j, k;
	/*u16		iqk_apply[2] = {0xc94, 0xe94};*/

	switch (step) {
	case 0:
		iqk_info->iqk_channel[1] = iqk_info->iqk_channel[0];
		for (i = 0; i < SS_8195B; i++) {
			iqk_info->lok_idac[1][i] = iqk_info->lok_idac[0][i];
			iqk_info->rxiqk_agc[1][i] = iqk_info->rxiqk_agc[0][i];
			iqk_info->bypass_iqk[1][i] = iqk_info->bypass_iqk[0][i];
			iqk_info->rxiqk_fail_code[1][i] = iqk_info->rxiqk_fail_code[0][i];
			for (j = 0; j < 2; j++) {
				iqk_info->iqk_fail_report[1][i][j] = iqk_info->iqk_fail_report[0][i][j];
				for (k = 0; k < 8; k++) {
					iqk_info->iqk_cfir_real[1][i][j][k] = iqk_info->iqk_cfir_real[0][i][j][k];
					iqk_info->iqk_cfir_imag[1][i][j][k] = iqk_info->iqk_cfir_imag[0][i][j][k];
				}
			}
		}
		for (i = 0; i < 4; i++) {
			iqk_info->rxiqk_fail_code[0][i] = 0x0;
			iqk_info->rxiqk_agc[0][i] = 0x0;
			for (j = 0; j < 2; j++) {
				iqk_info->iqk_fail_report[0][i][j] = true;
				iqk_info->gs_retry_count[0][i][j] = 0x0;
			}
			for (j = 0; j < 3; j++)
				iqk_info->retry_count[0][i][j] = 0x0;
		}
		/*backup channel*/
		iqk_info->iqk_channel[0] = iqk_info->rf_reg18;
		break;
	case 1: /*LOK backup*/
		iqk_info->lok_idac[0][path] = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x58, RFREGOFFSETMASK);
		break;
	case 2: /*TXIQK backup*/
	case 3: /*RXIQK backup*/
		phydm_get_iqk_cfir(dm, (step - 2), path, false);
		break;
	}
}

void _iqk_reload_iqk_setting_8195b(
	struct dm_struct *dm,
	u8 channel,
	u8 reload_idx /*1: reload TX, 2: reload TX, RX*/
	)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i, path, idx;
	u16 iqk_apply[2] = {0xc94, 0xe94};

	for (path = 0; path < SS_8195B; path++) {
#if 0
		if (reload_idx == 2) {
			odm_set_rf_reg(dm, (enum rf_path)path, 0xdf, BIT(4), 0x1);
			odm_set_rf_reg(dm, (enum rf_path)path, 0x58, RFREGOFFSETMASK, iqk_info->LOK_IDAC[channel][path]);
		}
#endif
		for (idx = 0; idx < reload_idx; idx++) {
			odm_set_bb_reg(dm, 0x1b00, MASKDWORD, 0x00000008 | path << 1);
			odm_set_bb_reg(dm, 0x1b20, 0x07000000, 0x7);
			odm_set_bb_reg(dm, 0x1b38, MASKDWORD, 0x20000000);
			odm_set_bb_reg(dm, 0x1b3c, MASKDWORD, 0x20000000);
			odm_set_bb_reg(dm, 0x1bcc, MASKDWORD, 0x00000000);
			if (idx == 0)
				odm_set_bb_reg(dm, 0x1b0c, BIT(13) | BIT(12), 0x3);
			else
				odm_set_bb_reg(dm, 0x1b0c, BIT(13) | BIT(12), 0x1);
			odm_set_bb_reg(dm, 0x1bd4, BIT(20) | BIT(19) | BIT(18) | BIT(17) | BIT(16), 0x10);
			for (i = 0; i < 8; i++) {
				odm_write_4byte(dm, 0x1bd8, ((0xc0000000 >> idx) + 0x3) + (i * 4) + (iqk_info->iqk_cfir_real[channel][path][idx][i] << 9));
				odm_write_4byte(dm, 0x1bd8, ((0xc0000000 >> idx) + 0x1) + (i * 4) + (iqk_info->iqk_cfir_imag[channel][path][idx][i] << 9));
			}
			if (idx == 0)
				odm_set_bb_reg(dm, iqk_apply[path], BIT(0), ~(iqk_info->iqk_fail_report[channel][path][idx]));
			else
				odm_set_bb_reg(dm, iqk_apply[path], BIT(10), ~(iqk_info->iqk_fail_report[channel][path][idx]));
		}
		odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, 0x0);
		odm_set_bb_reg(dm, 0x1b0c, BIT(13) | BIT(12), 0x0);
	}
}

boolean
_iqk_reload_iqk_8195b(
	struct dm_struct *dm,
	boolean reset)
{
#if 1
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i;

	iqk_info->is_reload = false;
	odm_set_bb_reg(dm, 0x1bf0, BIT(16), 0x0); /*clear the reload flag*/

	if (reset) {
		for (i = 0; i < SS_8195B; i++)
			iqk_info->iqk_channel[i] = 0x0;
	} else {
		iqk_info->rf_reg18 = odm_get_rf_reg(dm, RF_PATH_A, 0x18, RFREGOFFSETMASK);

		for (i = 0; i < SS_8195B; i++) {
			if (iqk_info->rf_reg18 == iqk_info->iqk_channel[i]) {
				_iqk_reload_iqk_setting_8195b(dm, i, 2);
				_iqk_fill_iqk_report_8195b(dm, i);
				RF_DBG(dm, DBG_RF_IQK,
				       "[IQK]reload IQK result before!!!!\n");
				odm_set_bb_reg(dm, 0x1bf0, BIT(16), 0x1);
				iqk_info->is_reload = true;
			}
		}
	}
#endif
	return false;
}

void _iqk_rfe_setting_8195b(
	struct dm_struct *dm,
	boolean ext_pa_on)
{
	if (ext_pa_on) {
		/*RFE setting*/
		odm_write_4byte(dm, 0xcb0, 0x77777777);
		odm_write_4byte(dm, 0xcb4, 0x00007777);
		odm_write_4byte(dm, 0xcbc, 0x0000083B);
		/*odm_write_4byte(dm, 0x1990, 0x00000c30);*/
		RF_DBG(dm, DBG_RF_IQK, "[IQK]external PA on!!!!\n");
	} else {
		/*RFE setting*/
		odm_write_4byte(dm, 0xcb0, 0x77171117);
		odm_write_4byte(dm, 0xcb4, 0x00001177);
		odm_write_4byte(dm, 0xcbc, 0x00000404);
		/*odm_write_4byte(dm, 0x1990, 0x00000c30);*/
		RF_DBG(dm, DBG_RF_IQK, "[IQK]external PA off!!!!\n");
	}
}

void _iqk_rfsetting_8195b(
	struct dm_struct *dm)
{
#if 1
	struct dm_iqk_info	*iqk_info = &dm->IQK_info;

	u8 path;
	u32 tmp;

	odm_write_4byte(dm, 0x1b00, 0x00000008);
	odm_write_4byte(dm, 0x1bb8, 0x00000000);

	for (path = 0; path < SS_8195B; path++) {
		/*0xdf:B11 = 1,B4 = 0, B1 = 1*/
		//tmp = odm_get_rf_reg(dm, (enum rf_path)path, 0xdf, RFREGOFFSETMASK);
		//tmp = (tmp & (~BIT(4))) | BIT(1) | BIT(11);
		//odm_set_rf_reg(dm, (enum rf_path)path, 0xdf, RFREGOFFSETMASK, tmp);
		/*WLAN_AG*/
		/*TX IQK	 mode init*/
		//odm_set_rf_reg(dm, (enum rf_path)path, 0xef, RFREGOFFSETMASK, 0x80000);
		//odm_set_rf_reg(dm, (enum rf_path)path, 0x33, RFREGOFFSETMASK, 0x00024);
		//odm_set_rf_reg(dm, (enum rf_path)path, 0x3e, RFREGOFFSETMASK, 0x0003f);
		/*odm_set_rf_reg(dm, (enum rf_path)path, 0x3f, RFREGOFFSETMASK, 0x60fde);*/
		//odm_set_rf_reg(dm, (enum rf_path)path, 0x3f, RFREGOFFSETMASK, 0xe0fde);
		//odm_set_rf_reg(dm, (enum rf_path)path, 0xef, RFREGOFFSETMASK, 0x00000);
		if (*dm->band_type == ODM_BAND_5G) {
			odm_set_rf_reg(dm, (enum rf_path)path, 0xef, BIT(19), 0x1);
			odm_set_rf_reg(dm, (enum rf_path)path, 0x33, RFREGOFFSETMASK, 0x00027);
			odm_set_rf_reg(dm, (enum rf_path)path, 0x3e, RFREGOFFSETMASK, 0x017b8);
			odm_set_rf_reg(dm, (enum rf_path)path, 0x3f, RFREGOFFSETMASK, 0xfeaab);
			odm_set_rf_reg(dm, (enum rf_path)path, 0xef, BIT(19), 0x0);
		} else {
			odm_set_rf_reg(dm, (enum rf_path)path, 0xef, BIT(19), 0x1);
			odm_set_rf_reg(dm, (enum rf_path)path, 0x33, RFREGOFFSETMASK, 0x00027);
			odm_set_rf_reg(dm, (enum rf_path)path, 0x3e, RFREGOFFSETMASK, 0x017f8);
			odm_set_rf_reg(dm, (enum rf_path)path, 0x3f, RFREGOFFSETMASK, 0xfeaab);
			odm_set_rf_reg(dm, (enum rf_path)path, 0xef, BIT(19), 0x0);
		}
	}
#endif
}

void _iqk_init_8195b(
	struct dm_struct *dm)
{
RF_DBG(dm, DBG_RF_IQK, "[IQK]==========%s !!!!!==========\n", __func__);
#if 0
	odm_write_4byte(dm, 0x1c44,0xa34300F3);
	odm_write_4byte(dm, 0x0c94,0x01000101);
	odm_write_4byte(dm, 0x1b04,0xE24629D2);
	odm_write_4byte(dm, 0x1b08,0x00000080);
	odm_write_4byte(dm, 0x1b0c,0x00000000);
	odm_write_4byte(dm, 0x1b10,0x00010C00);
	odm_write_4byte(dm, 0x1b14,0x00000000);
	odm_write_4byte(dm, 0x1b18,0x00292903);
	odm_write_4byte(dm, 0x1b1c,0xA2193C32);
	odm_write_4byte(dm, 0x1b20,0x03040008);
	odm_write_4byte(dm, 0x1b24,0x00060008);
	odm_write_4byte(dm, 0x1b28,0x80060300);
	odm_write_4byte(dm, 0x1b2c,0x00180018);
	odm_write_4byte(dm, 0x1b30,0x20000000);
	odm_write_4byte(dm, 0x1b34,0x00000800);
	odm_write_4byte(dm, 0x1b3c,0x20000000);
	odm_write_4byte(dm, 0x1bc0,0x01000000);
	odm_write_4byte(dm, 0x1bcc,0x00000000);
#else
/*IQK INIT*/
	odm_write_4byte(dm, 0x1c44, 0xa34300F3);
	odm_write_4byte(dm, 0x0c94, 0x01000101);
	odm_write_4byte(dm, 0x1b00, 0x00000008);
	odm_write_4byte(dm, 0x1b00, 0x00A70008);
	odm_write_4byte(dm, 0x1b00, 0x00150008);
	odm_write_4byte(dm, 0x1b00, 0x00000008);
	odm_write_4byte(dm, 0x1b04, 0xE24629D2);
	odm_write_4byte(dm, 0x1b08, 0x00000080);
	odm_write_4byte(dm, 0x1b0c, 0x00000000);
	odm_write_4byte(dm, 0x1b10, 0x00010C00);
	odm_write_4byte(dm, 0x1b14, 0x00000000);
	odm_write_4byte(dm, 0x1b18, 0x00292903);
	odm_write_4byte(dm, 0x1b1c, 0xA2193C32);
	odm_write_4byte(dm, 0x1b20, 0x03040008);
	odm_write_4byte(dm, 0x1b24, 0x00060008);
	odm_write_4byte(dm, 0x1b28, 0x80060300);
	odm_write_4byte(dm, 0x1b2c, 0x00180018);
	odm_write_4byte(dm, 0x1b30, 0x20000000);
	odm_write_4byte(dm, 0x1b34, 0x00000800);
	odm_write_4byte(dm, 0x1b38, 0x20000000);
	odm_write_4byte(dm, 0x1b3c, 0x20000000);
	odm_write_4byte(dm, 0x1bc0, 0x01000000);
	//odm_write_4byte(dm, 0x1bcc, 0x00000000);
/*DPK INIT*/
	//DPK phy setting
	odm_write_4byte(dm, 0x1b90, 0x0105f038);
	odm_write_4byte(dm, 0x1b94, 0xf76d9f84);
	odm_write_4byte(dm, 0x1bc8, 0x000c44aa);
	odm_write_4byte(dm, 0x1bcc, 0x11160200);
#endif
}

void _iqk_configure_macbb_8195b(
	struct dm_struct *dm)
{
	/*MACBB register setting*/
	odm_write_1byte(dm, 0x522, 0x7f);
	odm_set_bb_reg(dm, 0x1518, BIT(16), 0x1);
	odm_set_bb_reg(dm, 0x550, BIT(11) | BIT(3), 0x0);
	odm_set_bb_reg(dm, 0x90c, BIT(15), 0x1); /*0x90c[15]=1: dac_buf reset selection*/

	odm_set_bb_reg(dm, 0xc94, BIT(0), 0x1);
	odm_set_bb_reg(dm, 0xc94, (BIT(11) | BIT(10)), 0x1);
	/* 3-wire off*/
	odm_write_4byte(dm, 0xc00, 0x00000004);
	/*disable PMAC*/
	odm_set_bb_reg(dm, 0xb00, BIT(8), 0x0);
	/*	RF_DBG(dm, DBG_RF_IQK, ("[IQK]Set MACBB setting for IQK!!!!\n"));*/
}

void _iqk_lok_setting_8195b(
	struct dm_struct *dm,
	u8 path,
	u8 uPADindex)
{
	u32 LOK0x56_2G = 0xee3;
	u32 LOK0x56_5G = 0xeec;

	u32 LOK0x33 = 0;
	u32 tmp = 0;

	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	LOK0x33 = uPADindex;

	odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
	odm_set_mac_reg(dm, 0x1bb8, BIT(20), 0x0);
	odm_set_mac_reg(dm, 0x1b0c, BIT(13) | BIT(12), 0x0);
	odm_set_mac_reg(dm, 0x1b2c, 0x00000fff, 0x038);
	odm_write_1byte(dm, 0x1b23, 0x0);
	odm_set_rf_reg(dm, path, 0xde, BIT(16), 0x1);
	odm_set_rf_reg(dm, path, 0xee, BIT(18), 0x1);

	switch (*dm->band_type) {
	case ODM_BAND_2_4G:
		odm_write_1byte(dm, 0x1bcc, 0x12);
		odm_set_rf_reg(dm, path, 0x57, 0x00007000, 0x0);// TAG
		odm_set_rf_reg(dm, path, 0x57, 0x00000700, 0x0);// TXA
		LOK0x56_2G = LOK0x56_2G & (0xfff1f | ((u32)uPADindex << 5));
		odm_set_rf_reg(dm, path, 0x56, 0x00fff, LOK0x56_2G);
		odm_set_rf_reg(dm, path, 0x8f, 0x00600, 0xafc00);
		odm_set_rf_reg(dm, path, 0x33, BIT(3) | BIT(2) | BIT(1) | BIT(0), (~BIT(3)) & LOK0x33);
		break;
	case ODM_BAND_5G:
		odm_write_1byte(dm, 0x1bcc, 0x9);
		odm_set_rf_reg(dm, path, 0x57, 0x07000, 0x0);// TAG
		odm_set_rf_reg(dm, path, 0x57, 0x00700, 0x0); //TXA
		LOK0x56_5G = LOK0x56_5G & (0xfff1f | ((u32)uPADindex << 5));
		odm_set_rf_reg(dm, path, 0x56, 0x00fff, LOK0x56_5G);
		odm_set_rf_reg(dm, path, 0x8f, 0x00600, 0x3);
		odm_set_rf_reg(dm, path, 0x33, BIT(3) | BIT(2) | BIT(1) | BIT(0), BIT(3) | LOK0x33);
		break;
	}
}

void _iqk_txk_setting_8195b(
	struct dm_struct *dm,
	u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
	odm_set_mac_reg(dm, 0x1bb8, BIT(20), 0x0);
	odm_set_mac_reg(dm, 0x1b0c, BIT(13) | BIT(12), 0x0);//disable dbg
	odm_write_1byte(dm, 0x1bcc, 0x12);
	odm_set_mac_reg(dm, 0x1b2c, 0x00000fff, 0x038);
	//odm_write_4byte(dm, 0x1b20, 0x00040008);
	odm_set_rf_reg(dm, path, 0xde, BIT(16), 0x1);
	odm_set_rf_reg(dm, path, 0xee, BIT(18), 0x0);
	//odm_set_rf_reg(dm, path, 0x00, RFREGOFFSETMASK, 0x40018);
	switch (*dm->band_type) {
	case ODM_BAND_2_4G:
		odm_set_rf_reg(dm, path, 0x57, 0x07000, 0x0);// TAG
		odm_set_rf_reg(dm, path, 0x57, 0x00700, 0x0);// TXA
		odm_set_rf_reg(dm, path, 0x56, 0x00fff, 0xee3);
		odm_set_rf_reg(dm, path, 0x8f, 0x00600, 0x3);
		break;
	case ODM_BAND_5G:
		odm_set_rf_reg(dm, path, 0x57, 0x07000, 0x0);// TAG
		odm_set_rf_reg(dm, path, 0x57, 0x00700, 0x0); //TXA
		odm_set_rf_reg(dm, path, 0x56, 0x00fff, 0xeec);
		odm_set_rf_reg(dm, path, 0x8f, 0x00600, 0x3);
		break;
	}
}

void _iqk_rxk1setting_8195b(
	struct dm_struct *dm,
	u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
	odm_set_mac_reg(dm, 0x1b2c, 0x00000fff, 0x038);
	odm_set_mac_reg(dm, 0x1b2c, 0x0fff0000, 0x038);
	odm_write_4byte(dm, 0x1b20, 0x00040008);
	odm_set_rf_reg(dm, path, 0xde, BIT(16), 0x1);
	odm_set_rf_reg(dm, path, 0xee, BIT(18), 0x0);

	switch (*dm->band_type) {
	case ODM_BAND_2_4G:
		odm_write_1byte(dm, 0x1bcc, 0x1b);
		odm_write_4byte(dm, 0x1b20, 0x00060008);
		odm_write_4byte(dm, 0x1b24, 0x00070848);
		odm_set_rf_reg(dm, path, 0x56, 0x00fff, 0x020);
		odm_set_rf_reg(dm, path, 0x8f, 0x00600, 0x3);
		break;
	case ODM_BAND_5G:
		odm_write_1byte(dm, 0x1bcc, 0x12);
		odm_write_4byte(dm, 0x1b20, 0x00060008);
		odm_write_4byte(dm, 0x1b24, 0x00071448);
		odm_set_rf_reg(dm, path, 0x56, 0x00fff, 0x040);
		odm_set_rf_reg(dm, path, 0x8f, 0x00600, 0x3);
		break;
	}
	/*RF_DBG(dm, DBG_RF_IQK, ("[IQK]Set RXK setting!!!!\n"));*/
}

//static u8 btg_lna[5] = {0x0, 0x4, 0x8, 0xc, 0xf};
static u8 wlg_lna[5] = {0x0, 0x1, 0x2, 0x3, 0x5};
static u8 wla_lna[5] = {0x0, 0x1, 0x3, 0x4, 0x5};

void _iqk_rxk2setting_8195b(
	struct dm_struct *dm,
	u8 path,
	boolean is_gs)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
	odm_set_mac_reg(dm, 0x1b2c, 0x00000fff, 0x038);
	odm_set_mac_reg(dm, 0x1b2c, 0x0fff0000, 0x038);
	odm_set_rf_reg(dm, path, 0xde, BIT(16), 0x1);
	odm_set_rf_reg(dm, path, 0xee, BIT(18), 0x0);
	odm_set_mac_reg(dm, 0x1b18, BIT(1), 0x1);
	switch (*dm->band_type) {
	case ODM_BAND_2_4G:
		if (is_gs) {
			iqk_info->tmp1bcc = 0x12;
			iqk_info->lna_idx = 2;
		}
		odm_write_1byte(dm, 0x1bcc, iqk_info->tmp1bcc);
		odm_write_4byte(dm, 0x1b20, 0x00060008);
		odm_write_4byte(dm, 0x1b24, 0x00070848);
		odm_set_rf_reg(dm, path, 0x56, 0x00fff, 0x020);
		odm_set_rf_reg(dm, path, 0x8f, 0x00600, 0x3);
		break;
	case ODM_BAND_5G:
		if (is_gs) {
			iqk_info->tmp1bcc = 0x1b;
			iqk_info->lna_idx = 0;
		}
		odm_write_1byte(dm, 0x1bcc, iqk_info->tmp1bcc);
		odm_write_4byte(dm, 0x1b20, 0x00060008);
		odm_write_4byte(dm, 0x1b24, 0x00071448);
		odm_set_rf_reg(dm, path, 0x56, 0x00fff, 0x040);
		odm_set_rf_reg(dm, path, 0x8f, 0x00600, 0x3);
		break;
	}
}

boolean
_iqk_check_cal_8195b(
	struct dm_struct *dm,
	u32 IQK_CMD)
{
	boolean notready = true, fail = true;
	u32 delay_count = 0x0;

	while (notready) {
		if (odm_read_4byte(dm, 0x1b00) == (IQK_CMD & 0xffffff0f)) {
			fail = (boolean)odm_get_bb_reg(dm, 0x1b08, BIT(26));
			notready = false;
		} else {
			ODM_delay_ms(1);
			delay_count++;
		}
		if (delay_count >= 50) {
			fail = true;
			RF_DBG(dm, DBG_RF_IQK, "[IQK]IQK timeout!!!\n");
			break;
		}
	}
	return fail;
}

boolean
_iqk_rx_iqk_gain_search_fail_8195b(
	struct dm_struct *dm,
	u8 path,
	u8 step)
{
	struct dm_iqk_info	*iqk_info = &dm->IQK_info;
	boolean		fail = true;
	u32	IQK_CMD = 0x0, rf_reg0, tmp, rxbb;
	u8	IQMUX[4] = {0x9, 0x12, 0x1b, 0x24}, *plna;
	u8	idx;
	/*u8	lna_setting[5];*/

	if (*dm->band_type == ODM_BAND_2_4G)
		plna = wlg_lna;
	else
		plna = wla_lna;


	if (step == RXIQK1) {
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]============ RXIQK RXIQK1 GainSearch ============\n");
		IQK_CMD = 0x00000208 | (1 << (path + 4));
		odm_write_4byte(dm, 0x1b00, IQK_CMD);
		odm_write_4byte(dm, 0x1b00, IQK_CMD + 0x1);
		ODM_delay_ms(GS_delay_8195B);
		fail = _iqk_check_nctl_done_8195b(dm, path, IQK_CMD);
	} else if (step == RXIQK2) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]============ RXIQK RXIQK2 GainSearch ============\n");
		for (idx = 0; idx < 4; idx++) {
			if (iqk_info->tmp1bcc == IQMUX[idx])
				break;
		}
		//odm_set_mac_reg(dm, 0x1c38, BIT(0), 0x1);// 3-wire, control by iqk
		odm_write_4byte(dm, 0x1b00, 0xf8000008 | path << 1);
		odm_write_4byte(dm, 0x1bcc, iqk_info->tmp1bcc);
		IQK_CMD = 0x00000308 | (1 << (path + 4));
		odm_write_4byte(dm, 0x1b00, IQK_CMD);
		odm_write_4byte(dm, 0x1b00, IQK_CMD + 0x1);
		ODM_delay_ms(GS_delay_8195B);
		fail = _iqk_check_nctl_done_8195b(dm, path, IQK_CMD);

		rf_reg0 = odm_get_rf_reg(dm, (enum rf_path)path, 0x0, RFREGOFFSETMASK);
		odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
		tmp = (rf_reg0 & 0x1fe0) >> 5;
		iqk_info->lna_idx = tmp >> 5;
		rxbb = tmp & 0x1f;

		odm_set_mac_reg(dm, 0x1b18, BIT(1), 0x0); //disable rxgac=0 ; enable rxagc=1
		//odm_set_mac_reg(dm, 0x1c38, BIT(0), 0x0); // release the 3-wire control by iqk

		if (rxbb == 0x1) {
			if (iqk_info->lna_idx != 0x0)
				iqk_info->lna_idx--;
			else if (idx != 3)
				idx++;
			else
				iqk_info->isbnd = true;
			fail = true;
		} else if (rxbb >= 0xa) {
			if (idx != 0)
				idx--;
			else if (iqk_info->lna_idx != 0x4)
				iqk_info->lna_idx++;
			else
				iqk_info->isbnd = true;
			fail = true;
		} else
			fail = false;

		if (iqk_info->isbnd == true)
			fail = false;

		iqk_info->tmp1bcc = IQMUX[idx];

		if (fail) {
			odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
			odm_write_4byte(dm, 0x1b24, (odm_read_4byte(dm, 0x1b24) & 0xffffe3ff) | (iqk_info->lna_idx << 10));
		}
	}
	return fail;
}
void _iqk_rxk2setting_by_toneindex_8195b(
	struct dm_struct *dm,
	u8 path,
	boolean is_gs,
	u8 toneindex)
{
#if 0
	struct dm_iqk_info	*iqk_info = &dm->IQK_info;
	u8 tmplna, tmp1bcc;

	odm_write_4byte(dm, 0x1b00, 0xf8000008 | path << 1);
	switch (*dm->band_type) {
	case ODM_BAND_2_4G:
		iqk_info->tmp1bcc = 0x12;
		iqk_info->lna_idx = 2;
		odm_write_1byte(dm, 0x1bcc, iqk_info->tmp1bcc);
		odm_write_1byte(dm, 0x1b2b, 0x00);
		odm_write_4byte(dm, 0x1b20, 0x01450008);
		odm_write_4byte(dm, 0x1b24, (0x01460048 | (wlg_lna[iqk_info->lna_idx] << 10)));
		odm_set_rf_reg(dm, path, 0x56, RFREGOFFSETMASK, 0x510f3);
		odm_set_rf_reg(dm, path, 0x8f, RFREGOFFSETMASK, 0xa9c00);
		break;
	case ODM_BAND_5G:
		iqk_info->tmp1bcc = 0x09;
		iqk_info->lna_idx = 2;
		odm_write_1byte(dm, 0x1bcc, iqk_info->tmp1bcc);
		odm_write_1byte(dm, 0x1b2b, 0x00);
		odm_write_4byte(dm, 0x1b20, 0x00450008);
		odm_write_4byte(dm, 0x1b24, (0x01460048 | (wla_lna[iqk_info->lna_idx] << 10)));
		odm_set_rf_reg(dm, path, 0x56, RFREGOFFSETMASK, 0x51060);
		odm_set_rf_reg(dm, path, 0x8f, RFREGOFFSETMASK, 0xa9c00);
		break;
	}
	odm_write_4byte(dm, 0x1b20, (odm_read_4byte(dm, 0x1b20) && 0x000fffff) | toneindex<<20);
	odm_write_4byte(dm, 0x1b24, (odm_read_4byte(dm, 0x1b24) && 0x000fffff) | toneindex<<20);
#endif
}

boolean
_iqk_rx_iqk_gain_search_fail_by_toneindex_8195b(
	struct dm_struct *dm,
	u8 path,
	u8 step,
	u8 tone_index)
{
#if 0
	struct dm_iqk_info	*iqk_info = &dm->IQK_info;
	boolean		fail = true;
	u32	IQK_CMD;

	_iqk_rxk2setting_by_toneindex_8195b(dm, path, RXIQK1, tone_index);
	odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);

	IQK_CMD = 0x00000208 | (1 << (path + 4));
	odm_write_4byte(dm, 0x1b00, IQK_CMD);
	odm_write_4byte(dm, 0x1b00, IQK_CMD + 0x1);
	ODM_delay_ms(GS_delay_8195B);
	fail = _iqk_check_cal_8195b(dm, IQK_CMD);
	return fail;
#endif
	return false;
}

boolean
_lok_one_shot_8195b(
	void *dm_void,
	u8 path,
	u8 uPADindex

	)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 delay_count = 0;
	boolean LOK_notready = false;
	u32 static RF0x08 = 0x80200, LOK_temp3 = 0;
	u32 IQK_CMD = 0x0;

	/*u8		LOKreg[] = {0x58, 0x78};*/

	RF_DBG(dm, DBG_RF_IQK, "[IQK]==========  LOK ==========\n");
	odm_write_1byte(dm, 0x1b10, 0x0);
	IQK_CMD = 0x00000008 | (1 << (4 + path));
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]LOK_Trigger = 0x%x\n", IQK_CMD);
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]uPADindex = 0x%x\n", uPADindex);


	if (uPADindex >= 0x4){
		//_iqk_set_gnt_wl_gnt_bt(dm, true);
		odm_write_4byte(dm, 0x1b00, IQK_CMD);
		odm_write_4byte(dm, 0x1b00, IQK_CMD + 1);
		/*LOK: CMD ID = 0	{0xf8000018, 0xf8000028}*/
		/*LOK: CMD ID = 0	{0xf8000019, 0xf8000029}*/
		ODM_delay_ms(50);
		//delay_ms(50);
		delay_count = 0;
		LOK_notready = true;
		while (LOK_notready) {
			if (odm_read_1byte(dm, 0x1f7f) == 0x55){
				LOK_notready = false;
				ODM_delay_ms(1);
				RF0x08 = odm_get_rf_reg(dm, (enum rf_path)path, 0x8, RFREGOFFSETMASK);
				break;
			}

			LOK_notready = true;
			if (LOK_notready) {
				ODM_delay_ms(1);
				delay_count++;
			}
			if (delay_count >= 50) {
				RF_DBG(dm, DBG_RF_IQK,
				       "[IQK]S%d LOK timeout!!!\n", path);
			break;
			}
		}
	} else {
		ODM_delay_ms(1);
		odm_set_rf_reg(dm, path, 0x08, RFREGOFFSETMASK, RF0x08);
		RF_DBG(dm, DBG_RF_IQK, "In LOK 0, RF08 = %x\n",
		       odm_get_rf_reg(dm, (enum rf_path)path, 0x8, RFREGOFFSETMASK));
	}
	//odm_set_rf_reg(dm, path, 0xee, BIT(18), 0x0);
	odm_write_1byte(dm, 0x1f7f, 0x0);

	if (LOK_notready)
		RF_DBG(dm, DBG_RF_IQK, "[IQK]==>S%d LOK Fail!!!\n", path);
	else {
		//RF_DBG(dm, DBG_RF_IQK, "In LOK, 0x1f7c[31:24] = %x\n",
		//       odm_read_4byte(dm, 0x1f7c));
		//RF_DBG(dm, DBG_RF_IQK, "In LOK, 0x1b10[7:0] = %x\n",
		//       odm_read_4byte(dm, 0x1b10));
		//RF_DBG(dm, DBG_RF_IQK, "In LOK, RF08 = %x, RF00=%x\n",
		//       odm_get_rf_reg(dm, (enum rf_path)path, 0x8,
		//       RFREGOFFSETMASK),
		//       odm_get_rf_reg(dm, (enum rf_path)path, 0x0,
		//       RFREGOFFSETMASK));
		//RF_DBG(dm, DBG_RF_IQK, "In LOK, RF58 = %x\n",
		//       odm_get_rf_reg(dm, (enum rf_path)path, 0x58,
		//       RFREGOFFSETMASK));
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d ==> delay_count = 0x%x\n",
		//       path, delay_count);
	}
	iqk_info->lok_fail[path] = LOK_notready;
	//odm_set_mac_reg(dm, 0x1f7c, 0xff000000, 0x00);

	return LOK_notready;
}

boolean
_iqk_one_shot_8195b(
	void *dm_void,
	u8 path,
	u8 idx)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 delay_count = 0;
	boolean fail = true;
	u32 IQK_CMD = 0x0;
	u16 iqk_apply[2] = {0xc94, 0xe94};

	if (idx == TX_IQK)
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]============ WBTXIQK ============\n");
	else if (idx == RXIQK1)
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]============ WBRXIQK STEP1============\n");
	else
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]============ WBRXIQK STEP2============\n");

	odm_write_1byte(dm, 0x1b10, 0x0);
	if (idx == TXIQK) {
		IQK_CMD = 0x00000008 | ((*dm->band_width + 4) << 8) | (1 << (path + 4));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]TXK_Trigger = 0x%x\n", IQK_CMD);
		/*{0xf8000418, 0xf800042a} ==> 20 WBTXK (CMD = 4)*/
		/*{0xf8000518, 0xf800052a} ==> 40 WBTXK (CMD = 5)*/
		/*{0xf8000618, 0xf800062a} ==> 80 WBTXK (CMD = 6)*/
	} else if (idx == RXIQK1) {
		if (*dm->band_width == 2)
			IQK_CMD = 0x00000808 | (1 << (path + 4));
		else
			IQK_CMD = 0x00000708 | (1 << (path + 4));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]RXK1_Trigger = 0x%x\n", IQK_CMD);
		/*{0xf8000718, 0xf800072a} ==> 20 WBTXK (CMD = 7)*/
		/*{0xf8000718, 0xf800072a} ==> 40 WBTXK (CMD = 7)*/
		/*{0xf8000818, 0xf800082a} ==> 80 WBTXK (CMD = 8)*/
	} else if (idx == RXIQK2) {
		IQK_CMD = 0x00000008 | ((*dm->band_width + 9) << 8) | (1 << (path + 4));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]RXK2_Trigger = 0x%x\n", IQK_CMD);
		/*{0xf8000918, 0xf800092a} ==> 20 WBRXK (CMD = 9)*/
		/*{0xf8000a18, 0xf8000a2a} ==> 40 WBRXK (CMD = 10)*/
		/*{0xf8000b18, 0xf8000b2a} ==> 80 WBRXK (CMD = 11)*/
	}

	odm_write_4byte(dm, 0x1b00, IQK_CMD);
	odm_write_4byte(dm, 0x1b00, IQK_CMD + 0x1);

	ODM_delay_ms(2);

	fail = _iqk_check_nctl_done_8195b(dm, path, IQK_CMD);
	ODM_delay_ms(10);

	if (dm->debug_components & DBG_RF_IQK) {
		odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]S%d ==> 0x1b00 = 0x%x, 0x1b08 = 0x%x\n", path,
		       odm_read_4byte(dm, 0x1b00), odm_read_4byte(dm, 0x1b08));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d ==> delay_count = 0x%x\n",
		       path, delay_count);
		if (idx != TXIQK)
			RF_DBG(dm, DBG_RF_IQK,
			       "[IQK]S%d ==> RF0x0 = 0x%x, RF0x%x = 0x%x\n",
			       path,
			       odm_get_rf_reg(dm, path, 0x0, RFREGOFFSETMASK),
			       0x56,
			       odm_get_rf_reg(dm, path, 0x56, RFREGOFFSETMASK));
	}
	odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
	if (idx == TXIQK) {
		if (fail)
			odm_set_bb_reg(dm, iqk_apply[path], BIT(0), 0x0);
		else
			_iqk_backup_iqk_8195b(dm, 0x2, path);
	}

	if (idx == RXIQK2) {
		iqk_info->rxiqk_agc[0][path] =
			(u16)(((odm_get_rf_reg(dm, (enum rf_path)path, 0x0, RFREGOFFSETMASK) >> 5) & 0xff) |
			      (iqk_info->tmp1bcc << 8));

		odm_write_4byte(dm, 0x1b38, 0x20000000);
		if (fail)
			odm_set_bb_reg(dm, iqk_apply[path], (BIT(11) | BIT(10)), 0x0);
		else
			_iqk_backup_iqk_8195b(dm, 0x3, path);
	}

	if (idx == TXIQK)
		iqk_info->iqk_fail_report[0][path][TXIQK] = fail;
	else
		iqk_info->iqk_fail_report[0][path][RXIQK] = fail;


	odm_write_4byte(dm, 0x1b38, 0x20000000);
	return fail;
}

boolean
_iqk_rxiqkbystep_8195b(
	void *dm_void,
	u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean KFAIL = true, gonext;
	u8 i;
	u32 tmp;

#if 1
	switch (iqk_info->rxiqk_step) {
	case 1: /*gain search_RXK1*/
		_iqk_rxk1setting_8195b(dm, path);
		gonext = false;
		while (1) {
			KFAIL = _iqk_rx_iqk_gain_search_fail_8195b(dm, path, RXIQK1);
			if (KFAIL && iqk_info->gs_retry_count[0][path][0] < 2)
				iqk_info->gs_retry_count[0][path][0]++;
			else if (KFAIL) {
				iqk_info->rxiqk_fail_code[0][path] = 0;
				iqk_info->rxiqk_step = 5;
				gonext = true;
			} else {
				iqk_info->rxiqk_step++;
				gonext = true;
			}
			if (gonext)
				break;
		}
		//halrf_iqk_xym_read(dm, 0x0, 0x2);
		break;
	case 2: /*gain search_RXK2*/
		_iqk_rxk2setting_8195b(dm, path, true);
		iqk_info->isbnd = false;
		while (1) {
			KFAIL = _iqk_rx_iqk_gain_search_fail_8195b(dm, path, RXIQK2);
			if (KFAIL && iqk_info->gs_retry_count[0][path][1] < rxiqk_gs_limit)
				iqk_info->gs_retry_count[0][path][1]++;
			else {
				iqk_info->rxiqk_step++;
				break;
			}
		}
		//halrf_iqk_xym_read(dm, 0x0, 0x3);
		break;
	case 3: /*RXK1*/
		_iqk_rxk1setting_8195b(dm, path);
		gonext = false;
		while (1) {
			KFAIL = _iqk_one_shot_8195b(dm, path, RXIQK1);
			if (KFAIL && iqk_info->retry_count[0][path][RXIQK1] < 2)
				iqk_info->retry_count[0][path][RXIQK1]++;
			else if (KFAIL) {
				iqk_info->rxiqk_fail_code[0][path] = 1;
				iqk_info->rxiqk_step = 5;
				gonext = true;
			} else {
				iqk_info->rxiqk_step++;
				gonext = true;
			}
			if (gonext)
				break;
		}
		//halrf_iqk_xym_read(dm, 0x0, 0x4);
		break;
	case 4: /*RXK2*/
		_iqk_rxk2setting_8195b(dm, path, false);
		gonext = false;
		while (1) {
			KFAIL = _iqk_one_shot_8195b(dm, path, RXIQK2);
			if (KFAIL && iqk_info->retry_count[0][path][RXIQK2] < 2)
				iqk_info->retry_count[0][path][RXIQK2]++;
			else if (KFAIL) {
				iqk_info->rxiqk_fail_code[0][path] = 2;
				iqk_info->rxiqk_step = 5;
				gonext = true;
			} else {
				iqk_info->rxiqk_step++;
				gonext = true;
			}
			if (gonext)
				break;
		}
		//halrf_iqk_xym_read(dm, 0x0, 0x0);
		break;
	}
	return KFAIL;
#endif
}

void _iqk_iqk_by_path_8195b(
	void *dm_void,
	boolean segment_iqk)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean KFAIL = true;
	u8 i, kcount_limit;

	/*	RF_DBG(dm, DBG_RF_IQK, ("[IQK]iqk_step = 0x%x\n", dm->rf_calibrate_info.iqk_step)); */
	if (*dm->band_width == 2)
		kcount_limit = kcount_limit_80m;
	else
		kcount_limit = kcount_limit_others;

	while (1) {
		switch (dm->rf_calibrate_info.iqk_step) {
		case 1: /*S0 LOK*/
			for (i = 7; i >= 0; i--) { /* the LOK Cal in the each PAD stage*/
				_iqk_lok_setting_8195b(dm, RF_PATH_A, i);
				_lok_one_shot_8195b(dm, RF_PATH_A, i);
				if (i==0)
					break;
			}
			dm->rf_calibrate_info.iqk_step++;
			break;
		case 2:
			/*S0 TXIQK*/
			_iqk_txk_setting_8195b(dm, RF_PATH_A);
			KFAIL = _iqk_one_shot_8195b(dm, RF_PATH_A, TXIQK);
			iqk_info->kcount++;
			RF_DBG(dm, DBG_RF_IQK, "[IQK]KFail = 0x%x\n", KFAIL);
			if (KFAIL && iqk_info->retry_count[0][RF_PATH_A][TXIQK] < 3)
				iqk_info->retry_count[0][RF_PATH_A][TXIQK]++;
			else
				dm->rf_calibrate_info.iqk_step++;
			break;

		case 3: /*S0 RXIQK*/
			while (1) {
				KFAIL = _iqk_rxiqkbystep_8195b(dm, RF_PATH_A);
				RF_DBG(dm, DBG_RF_IQK,
				       "[IQK]S0RXK KFail = 0x%x\n", KFAIL);
				if (iqk_info->rxiqk_step == 5) {
					dm->rf_calibrate_info.iqk_step++;
					iqk_info->rxiqk_step = 1;
					if (KFAIL) {
						RF_DBG(dm, DBG_RF_IQK,
						       "[IQK]S0RXK fail code: %d!!!\n", iqk_info->rxiqk_fail_code[0][RF_PATH_A]);
					}
					break;
				}
			}
			iqk_info->kcount++;
			break;
		}

		if (dm->rf_calibrate_info.iqk_step == 4) {
			odm_set_mac_reg(dm, 0x1b20, BIT(26), 0x1);// enable iqk
			RF_DBG(dm, DBG_RF_IQK,
			       "[IQK]==========LOK summary ==========\n");
			RF_DBG(dm, DBG_RF_IQK, "[IQK]PathA_LOK_notready = %d\n",
			       iqk_info->lok_fail[RF_PATH_A]);
			RF_DBG(dm, DBG_RF_IQK,
			       "[IQK]==========IQK summary ==========\n");
			RF_DBG(dm, DBG_RF_IQK, "[IQK]PathA_TXIQK_fail = %d\n",
			       iqk_info->iqk_fail_report[0][RF_PATH_A][TXIQK]);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]PathA_RXIQK_fail = %d\n",
			       iqk_info->iqk_fail_report[0][RF_PATH_A][RXIQK]);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]PathA_TXIQK_retry = %d\n",
			       iqk_info->retry_count[0][RF_PATH_A][TXIQK]);
			RF_DBG(dm, DBG_RF_IQK,
			       "[IQK]PathA_RXK1_retry = %d, PathA_RXK2_retry = %d\n",
			       iqk_info->retry_count[0][RF_PATH_A][RXIQK1],
			       iqk_info->retry_count[0][RF_PATH_A][RXIQK2]);
			RF_DBG(dm, DBG_RF_IQK,
			       "[IQK]PathA_GS1_retry = %d, PathA_GS2_retry = %d\n",
			       iqk_info->gs_retry_count[0][RF_PATH_A][0],
			       iqk_info->gs_retry_count[0][RF_PATH_A][1]);

			for (i = 0; i < SS_8195B; i++) {
				odm_write_4byte(dm, 0x1b00, 0x00000008 | i << 1);
				//odm_write_4byte(dm, 0x1b2c, 0x7);
				odm_write_1byte(dm, 0x1b24, 0x7);
				odm_write_1byte(dm, 0x1bcc, 0x0);
				odm_write_4byte(dm, 0x1b38, 0x20000000);
			}
			break;
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]segmentIQK = %d, Kcount = %d\n",
		       segment_iqk, iqk_info->kcount);
		if (segment_iqk == true && iqk_info->kcount == kcount_limit)
			break;
	}
}

void _iqk_start_iqk_8195b(
	struct dm_struct *dm,
	boolean segment_iqk)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 tmp;

	//odm_write_4byte(dm, 0x1b00, 0x00000008);
	//odm_write_4byte(dm, 0x1bb8, 0x00000000);
	/*GNT_WL = 1*/
	//tmp = odm_get_rf_reg(dm, RF_PATH_A, 0x1, RFREGOFFSETMASK);
	//tmp = ((tmp & (~BIT(3))) & (~BIT(5)))  | BIT(0) | BIT(2);
	//odm_set_rf_reg(dm, RF_PATH_A, 0x1, RFREGOFFSETMASK, tmp);
	//RF_DBG(dm, DBG_RF_IQK,
	//			"[IQK]==> RF0x1 = 0x%x\n", odm_get_rf_reg(dm, RF_PATH_A, 0x1, RFREGOFFSETMASK));
	_iqk_iqk_by_path_8195b(dm, segment_iqk);
	//odm_write_4byte(dm, 0x10c0, 0x5555aaaa);
}

void _iq_calibrate_8195b_init(
	struct dm_struct *dm)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i, j, k, m;
	static boolean firstrun = true;

	RF_DBG(dm, DBG_RF_IQK, "%s \n", __func__);
	if (firstrun) {
		firstrun = false;
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]=====>PHY_IQCalibrate_8195B_Init\n");

		for (i = 0; i < SS_8195B; i++) {
			for (j = 0; j < 2; j++) {
				iqk_info->lok_fail[i] = true;
				iqk_info->iqk_fail[j][i] = true;
				iqk_info->iqc_matrix[j][i] = 0x20000000;
			}
		}

		for (i = 0; i < 2; i++) {
			iqk_info->iqk_channel[i] = 0x0;

			for (j = 0; j < SS_8195B; j++) {
				iqk_info->lok_idac[i][j] = 0x0;
				iqk_info->rxiqk_agc[i][j] = 0x0;
				iqk_info->bypass_iqk[i][j] = 0x0;

				for (k = 0; k < 2; k++) {
					iqk_info->iqk_fail_report[i][j][k] = true;
					for (m = 0; m < 8; m++) {
						iqk_info->iqk_cfir_real[i][j][k][m] = 0x0;
						iqk_info->iqk_cfir_imag[i][j][k][m] = 0x0;
					}
				}

				for (k = 0; k < 3; k++)
					iqk_info->retry_count[i][j][k] = 0x0;
			}
		}
	}
}

u32 _iqk_tximr_selfcheck_8195b(
	void *dm_void,
	u8 tone_index,
	u8 path)
{
#if 0
	u32 tx_ini_power_H[2], tx_ini_power_L[2];
	u32 tmp1, tmp2, tmp3, tmp4, tmp5;
	u32 IQK_CMD;
	u32 tximr = 0x0;
	u8 i;

	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	/*backup*/
	odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
	odm_write_4byte(dm, 0x1bc8, 0x80000000);
	odm_write_4byte(dm, 0x8f8, 0x41400080);
	tmp1 = odm_read_4byte(dm, 0x1b0c);
	tmp2 = odm_read_4byte(dm, 0x1b14);
	tmp3 = odm_read_4byte(dm, 0x1b1c);
	tmp4 = odm_read_4byte(dm, 0x1b20);
	tmp5 = odm_read_4byte(dm, 0x1b24);
	/*setup*/
	odm_write_4byte(dm, 0x1b0c, 0x00003000);
	odm_write_4byte(dm, 0x1b1c, 0xA2193C32);
	odm_write_1byte(dm, 0x1b15, 0x00);
	odm_write_4byte(dm, 0x1b20, (u32)(tone_index << 20 | 0x00040008));
	odm_write_4byte(dm, 0x1b24, (u32)(tone_index << 20 | 0x00060008));
	odm_write_4byte(dm, 0x1b2c, 0x07);
	odm_write_4byte(dm, 0x1b38, 0x20000000);
	odm_write_4byte(dm, 0x1b3c, 0x20000000);
	/* ======derive pwr1========*/
	for (i = 0; i < 2; i++) {
		if (i == 0)
			odm_write_4byte(dm, 0x1bcc, 0x0f);
		else
			odm_write_4byte(dm, 0x1bcc, 0x09);
		/* One Shot*/
		IQK_CMD = 0x00000800;
		odm_write_4byte(dm, 0x1b34, IQK_CMD + 1);
		odm_write_4byte(dm, 0x1b34, IQK_CMD);
		ODM_delay_ms(1);
		odm_write_4byte(dm, 0x1bd4, 0x00040001);
		tx_ini_power_H[i] = odm_read_4byte(dm, 0x1bfc);
		odm_write_4byte(dm, 0x1bd4, 0x000C0001);
		tx_ini_power_L[i] = odm_read_4byte(dm, 0x1bfc);
	}
	/*restore*/
	odm_write_4byte(dm, 0x1b0c, tmp1);
	odm_write_4byte(dm, 0x1b14, tmp2);
	odm_write_4byte(dm, 0x1b1c, tmp3);
	odm_write_4byte(dm, 0x1b20, tmp4);
	odm_write_4byte(dm, 0x1b24, tmp5);

	if (tx_ini_power_H[1] == tx_ini_power_H[0])
		tximr = (3 * (halrf_psd_log2base(tx_ini_power_L[0] << 2) - halrf_psd_log2base(tx_ini_power_L[1]))) / 100;
	else
		tximr = 0;
	return tximr;
#endif
return 0;
}

u32 _iqk_rximr_selfcheck_8195b(
	void *dm_void,
	u8 tone_index,
	u8 path,
	u32 tmp1b38)
{
#if 0
	u32 rx_ini_power_H[2], rx_ini_power_L[2]; /*[0]: psd tone; [1]: image tone*/
	u32 tmp1, tmp2, tmp3, tmp4, tmp5;
	u32 IQK_CMD, tmp1bcc;
	u8 i, num_k1, rximr_step;
	u32 rximr = 0x0;
	boolean KFAIL = true;

	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	/*backup*/
	odm_write_4byte(dm, 0x1b00, 0x00000008 | path << 1);
	tmp1 = odm_read_4byte(dm, 0x1b0c);
	tmp2 = odm_read_4byte(dm, 0x1b14);
	tmp3 = odm_read_4byte(dm, 0x1b1c);
	tmp4 = odm_read_4byte(dm, 0x1b20);
	tmp5 = odm_read_4byte(dm, 0x1b24);

	tmp1bcc = (odm_read_4byte(dm, 0x1be8) & 0x0000ff00) >> 8;
	odm_write_4byte(dm, 0x1b0c, 0x00001000);
	odm_write_1byte(dm, 0x1b15, 0x00);
	odm_write_4byte(dm, 0x1b1c, 0x82193d31);
	odm_write_4byte(dm, 0x1b20, (u32)(tone_index << 20 | 0x00040008));
	odm_write_4byte(dm, 0x1b24, (u32)(tone_index << 20 | 0x00060048));
	odm_write_4byte(dm, 0x1b2c, 0x07);
	//odm_write_4byte(dm, 0x1b38, iqk_info->rxk1_tmp1b38[path][(tone_index&0xff0)>>4]);
	odm_write_4byte(dm, 0x1b38, tmp1b38);

	odm_write_4byte(dm, 0x1b3c, 0x20000000);
	odm_write_4byte(dm, 0x1bcc, tmp1bcc);
	for (i = 0; i < 2; i++) {
		if (i == 0)
			odm_write_4byte(dm, 0x1b1c, 0x82193d31);
		else
			odm_write_4byte(dm, 0x1b1c, 0xA2193d31);
		IQK_CMD = 0x00000800;
		odm_write_4byte(dm, 0x1b34, IQK_CMD + 1);
		odm_write_4byte(dm, 0x1b34, IQK_CMD);
		ODM_delay_us(2000);
		odm_write_4byte(dm, 0x1bd6, 0xb);
		/*if ((boolean)odm_get_bb_reg(dm, 0x1bfc, BIT(1))){*/
		if (1) {
			odm_write_4byte(dm, 0x1bd6, 0x5);
			rx_ini_power_H[i] = odm_read_4byte(dm, 0x1bfc);
			odm_write_4byte(dm, 0x1bd6, 0xe);
			rx_ini_power_L[i] = odm_read_4byte(dm, 0x1bfc);
		} else {
			rx_ini_power_H[i] = 0x0;
			rx_ini_power_L[i] = 0x0;
		}
	}
	/*restore*/
	odm_write_4byte(dm, 0x1b0c, tmp1);
	odm_write_4byte(dm, 0x1b14, tmp2);
	odm_write_4byte(dm, 0x1b1c, tmp3);
	odm_write_4byte(dm, 0x1b20, tmp4);
	odm_write_4byte(dm, 0x1b24, tmp5);

	for (i = 0; i < 2; i++)
		rx_ini_power_H[i] = (rx_ini_power_H[i] & 0xf8000000) >> 27;

	if (rx_ini_power_H[0] != rx_ini_power_H[1])
		switch (rx_ini_power_H[0]) {
		case 1:
			rx_ini_power_L[0] = (u32)((rx_ini_power_L[0] >> 1) | 0x80000000);
			rx_ini_power_L[1] = (u32)rx_ini_power_L[1] >> 1;
			break;
		case 2:
			rx_ini_power_L[0] = (u32)((rx_ini_power_L[0] >> 2) | 0x80000000);
			rx_ini_power_L[1] = (u32)rx_ini_power_L[1] >> 2;
			break;
		case 3:
			rx_ini_power_L[0] = (u32)((rx_ini_power_L[0] >> 2) | 0xc0000000);
			rx_ini_power_L[1] = (u32)rx_ini_power_L[1] >> 2;
			break;
		case 4:
			rx_ini_power_L[0] = (u32)((rx_ini_power_L[0] >> 3) | 0x80000000);
			rx_ini_power_L[1] = (u32)rx_ini_power_L[1] >> 3;
			break;
		case 5:
			rx_ini_power_L[0] = (u32)((rx_ini_power_L[0] >> 3) | 0xa0000000);
			rx_ini_power_L[1] = (u32)rx_ini_power_L[1] >> 3;
			break;
		case 6:
			rx_ini_power_L[0] = (u32)((rx_ini_power_L[0] >> 3) | 0xc0000000);
			rx_ini_power_L[1] = (u32)rx_ini_power_L[1] >> 3;
			break;
		case 7:
			rx_ini_power_L[0] = (u32)((rx_ini_power_L[0] >> 3) | 0xe0000000);
			rx_ini_power_L[1] = (u32)rx_ini_power_L[1] >> 3;
			break;
		default:
			break;
		}
	rximr = (u32)(3 * ((halrf_psd_log2base(rx_ini_power_L[0] / 100) - halrf_psd_log2base(rx_ini_power_L[1] / 100))) / 100);
	/*
		RF_DBG(dm, DBG_RF_IQK, ("%-20s: 0x%x, 0x%x, 0x%x, 0x%x,0x%x, tone_index=%x, rximr= %d\n",
		(path == 0) ? "PATH A RXIMR ": "PATH B RXIMR",
		rx_ini_power_H[0], rx_ini_power_L[0], rx_ini_power_H[1], rx_ini_power_L[1], tmp1bcc, tone_index, rximr));
		return rximr;
#endif
return 0;
}

void _iqk_start_imr_test_8195b(
	void *dm_void,
	u8 path)
{
#if 0
	u8 imr_limit, i, tone_index;
	u32 tmp;
	boolean KFAIL;
	u32 rxk1_tmp1b38[2][14];
	u32 imr_result[2];

	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	/*TX IMR*/
	if (*dm->band_width == 2)
		imr_limit = 0xe;
	else if (*dm->band_width == 1)
		imr_limit = 0x7;
	else
		imr_limit = 0x3;

	_iqk_txk_setting_8195b(dm, RF_PATH_A);
	KFAIL = _iqk_one_shot_8195b(dm, RF_PATH_A, TXIQK);
	for (i = 0x0; i <= imr_limit; i++) {
		tone_index = (u8)(0x08 | i << 4);
		imr_result[RF_PATH_A] = _iqk_tximr_selfcheck_8195b(dm, tone_index, RF_PATH_A);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]toneindex = %x, TXIMR = %d\n",
		       tone_index, imr_result[RF_PATH_A]);
	}
	RF_DBG(dm, DBG_RF_IQK, "\n");
	/*RX IMR*/
	/*get the rxk1 tone index 0x1b38 setting*/
	_iqk_rxk1setting_8195b(dm, path);
	tmp = odm_read_4byte(dm, 0x1b1c);
	for (path = 0; path < SS_8195B; path++) {
		for (i = 0; i <= imr_limit; i++) {
			tone_index = (u8)(0x08 | i << 4);
			KFAIL = _iqk_rx_iqk_gain_search_fail_by_toneindex_8195b(dm, path, RXIQK1, tone_index);
			if (!KFAIL) {
				odm_write_4byte(dm, 0x1b1c, 0xa2193c32);
				odm_write_4byte(dm, 0x1b14, 0xe5);
				odm_write_4byte(dm, 0x1b14, 0x0);
				rxk1_tmp1b38[path][i] = odm_read_4byte(dm, 0x1b38);
			} else
				rxk1_tmp1b38[path][i] = 0x0;
		}
	}
	_iqk_rxk2setting_8195b(dm, path, true);
	for (path = 0; path < SS_8195B; path++) {
		for (i = 0x0; i <= imr_limit; i++) {
			tone_index = (u8)(0x08 | i << 4);
			imr_result[RF_PATH_A] = _iqk_rximr_selfcheck_8195b(dm, tone_index, RF_PATH_A, rxk1_tmp1b38[path][i]);
			RF_DBG(dm, DBG_RF_IQK,
			       "[IQK]toneindex = %x, RXIMR = %d\n", tone_index,
			       imr_result[RF_PATH_A]);
		}
	}
	odm_write_4byte(dm, 0x1b1c, tmp);
	odm_write_4byte(dm, 0x1b38, 0x20000000);
#endif
}

void _phy_iq_calibrate_8195b(
	struct dm_struct *dm,
	boolean reset,
	boolean segment_iqk,
	boolean do_imr_test)
{
	u32 MAC_backup[IQK_MAC_REG_NUM_8195B], BB_backup[IQK_BB_REG_NUM_8195B], RF_backup[IQK_RF_REG_NUM_8195B][1];
	u32 backup_mac_reg[IQK_MAC_REG_NUM_8195B] = {0x520, 0x550, 0x1518};
	u32 backup_bb_reg[IQK_BB_REG_NUM_8195B] = {0x808, 0x90c, 0xc00, 0xcb0, 0xcb4, 0xcbc, 0x1990, 0x9a4, 0xa04, 0x1c44, 0xc94};
	u32 backup_rf_reg[IQK_RF_REG_NUM_8195B] = {0xdf, 0xde, 0x8f, 0x0, 0x1, 0xef};
	boolean is_mp = false;

	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	if (*dm->mp_mode)
		is_mp = true;
	else if (dm->is_linked)
		segment_iqk = false;
	//iqk_info->is_BTG = (boolean)odm_get_bb_reg(dm, 0xcb8, BIT(16));

	if (!is_mp)
		if (_iqk_reload_iqk_8195b(dm, reset))
			return;

	if (!do_imr_test) {
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]==========IQK strat!!!!!==========\n");
	}
	RF_DBG(dm, DBG_RF_IQK,
	       "[IQK]band_type = %s, band_width = %d, ExtPA2G = %d, ext_pa_5g = %d\n",
	       (*dm->band_type == ODM_BAND_5G) ? "5G" : "2G", *dm->band_width,
	       dm->ext_pa, dm->ext_pa_5g);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Interface = %d, cut_version = %x\n",
	       dm->support_interface, dm->cut_version);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]dm->channel = %d, dm->band_width = %x\n",
	       *dm->channel, *dm->band_width);

	//iqk_info->tmp_GNTWL = _iqk_indirect_read_reg(dm, 0x38);
	iqk_info->iqk_times++;
	iqk_info->kcount = 0;
	dm->rf_calibrate_info.iqk_total_progressing_time = 0;
	dm->rf_calibrate_info.iqk_step = 1;
	iqk_info->rxiqk_step = 1;
	iqk_info->is_reload = false;

	_iqk_backup_iqk_8195b(dm, 0x0, 0x0);
	_iqk_backup_mac_bb_8195b(dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg, IQK_BB_REG_NUM_8195B);
	_iqk_backup_rf_8195b(dm, RF_backup, backup_rf_reg);

	while (1) {
		if (!is_mp)
			dm->rf_calibrate_info.iqk_start_time = odm_get_current_time(dm);
		_iqk_init_8195b(dm);
		_iqk_configure_macbb_8195b(dm);
		_iqk_afe_setting_8195b(dm, true);
		_iqk_agc_bnd_int_8195b(dm);
		_iqk_rfe_setting_8195b(dm, false);
		_iqk_rfsetting_8195b(dm);
		_iqk_start_iqk_8195b(dm, segment_iqk);
		//_phy_dpd_calibrate_8195b(dm, true);
		_iqk_afe_setting_8195b(dm, false);
		_iqk_restore_mac_bb_8195b(dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg, IQK_BB_REG_NUM_8195B);
		_iqk_restore_rf_8195b(dm, backup_rf_reg, RF_backup);

		if (!is_mp) {
			dm->rf_calibrate_info.iqk_progressing_time = odm_get_progressing_time(dm, dm->rf_calibrate_info.iqk_start_time);
			dm->rf_calibrate_info.iqk_total_progressing_time += odm_get_progressing_time(dm, dm->rf_calibrate_info.iqk_start_time);
			RF_DBG(dm, DBG_RF_IQK,
			       "[IQK]IQK progressing_time = %lld ms\n",
			       dm->rf_calibrate_info.iqk_progressing_time);
		}

		if (dm->rf_calibrate_info.iqk_step == 4)
			break;

		iqk_info->kcount = 0;
		RF_DBG(dm, DBG_RF_IQK, "[IQK]delay 50ms!!!\n");
		ODM_delay_ms(50);
	}

	if (!do_imr_test) {
		if (segment_iqk)
			_iqk_reload_iqk_setting_8195b(dm, 0x0, 0x1);
		_iqk_fill_iqk_report_8195b(dm, 0);
		if (!is_mp)
			RF_DBG(dm, DBG_RF_IQK,
			       "[IQK]Total IQK progressing_time = %lld ms\n",
			       dm->rf_calibrate_info.iqk_total_progressing_time)
			       ;
		RF_DBG(dm, DBG_RF_IQK,
		       "[IQK]==========IQK end!!!!!==========\n");
	}
}

void _phy_iq_calibrate_by_fw_8195b(
	void *dm_void,
	u8 clear,
	u8 segment_iqk)
{
	/*
	struct dm_struct		*dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info	*iqk_info = &dm->IQK_info;
	enum hal_status		status = HAL_STATUS_FAILURE;

	if (*(dm->mp_mode))
		clear = 0x1;
	else if (dm->is_linked)
		segment_iqk = 0x1;

	status = odm_iq_calibrate_by_fw(dm, clear, segment_iqk);

	if (status == HAL_STATUS_SUCCESS)
		RF_DBG(dm, DBG_RF_IQK, "[IQK]FWIQK  OK!!!\n");
	else
		RF_DBG(dm, DBG_RF_IQK, "[IQK]FWIQK fail!!!\n");
*/
}

/*********debug message start**************/

void _phy_iqk_XYM_Read_Using_0x1b38_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u32 tmp = 0x0;
	u32 tmp2;
	u8 i;

	tmp = odm_read_4byte(dm, 0x1b1c);
	odm_write_4byte(dm, 0x1b1c, 0xA2193C32);
	RF_DBG(dm, DBG_RF_IQK, "\n");
	for (i = 0; i < 0xa; i++) {
		odm_write_4byte(dm, 0x1b14, 0xe6 + i);
		odm_write_4byte(dm, 0x1b14, 0x0);
		tmp2 = odm_read_4byte(dm, 0x1b38);
		RF_DBG(dm, DBG_RF_IQK, "%x\n", tmp2);
	}
	odm_write_4byte(dm, 0x1b1c, tmp);
	RF_DBG(dm, DBG_RF_IQK, "\n");
	odm_write_4byte(dm, 0x1b38, 0x20000000);
}

void _phy_iqk_debug_inner_lpbk_psd_8195b(
	struct dm_struct *dm,
	u8 path)
{
	s16 tx_x;
	s16 tx_y;
	u32 temp = 0x0;
	u32 psd_pwr = 0x0;
	u32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10;

	tmp1 = odm_read_4byte(dm, 0x1b20);
	tmp2 = odm_read_4byte(dm, 0x1b24);
	tmp3 = odm_read_4byte(dm, 0x1b15);
	tmp4 = odm_read_4byte(dm, 0x1b18);
	tmp5 = odm_read_4byte(dm, 0x1b1c);
	tmp6 = odm_read_4byte(dm, 0x1b28);
	tmp7 = odm_read_4byte(dm, 0x1b90);
	tmp8 = odm_read_4byte(dm, 0x1bcc);
	tmp9 = odm_read_4byte(dm, 0x1b2c);
	tmp10 = odm_read_4byte(dm, 0x1b30);
	odm_write_4byte(dm, 0x1b20, 0x00040008);
	odm_write_4byte(dm, 0x1b24, 0x00060008);

	odm_write_1byte(dm, 0x1b15, 0x00);
	odm_write_4byte(dm, 0x1b18, 0x00010101);
	odm_write_4byte(dm, 0x1b1c, 0x02effcb2);
	odm_write_4byte(dm, 0x1b28, 0x00060c00);
	odm_write_4byte(dm, 0x1b90, 0x00080003);
	odm_write_4byte(dm, 0x1bcc, 0x00000009);
	odm_write_4byte(dm, 0x1b2c, 0x20000003);
	odm_write_4byte(dm, 0x1b30, 0x20000000);
	RF_DBG(dm, DBG_RF_IQK, "\n");
	for (tx_x = 507; tx_x <= 532; tx_x++) {
		for (tx_y = 0; tx_y <= 10 + 20; tx_y++) {
			if (tx_y < 0)
				temp = (tx_x << 20) | (tx_y + 2048) << 8;
			else
				temp = (tx_x << 20) | (tx_y << 8);
			odm_write_4byte(dm, 0x1b38, temp);
			odm_write_4byte(dm, 0x1b3c, 0x20000000);
			odm_write_4byte(dm, 0x1b34, 0x00000801);
			odm_write_4byte(dm, 0x1b34, 0x00000800);
			ODM_delay_ms(2);
			/*PSD_bef_K*/
			odm_write_4byte(dm, 0x1bd4, 0x000c0001);
			psd_pwr = odm_read_4byte(dm, 0x1bfc);
			RF_DBG(dm, DBG_RF_IQK, "%d ", psd_pwr);
		}
	}
	RF_DBG(dm, DBG_RF_IQK, "\n");
	odm_write_4byte(dm, 0x1b20, tmp1);
	odm_write_4byte(dm, 0x1b24, tmp2);
	odm_write_4byte(dm, 0x1b15, tmp3);
	odm_write_4byte(dm, 0x1b18, tmp4);
	odm_write_4byte(dm, 0x1b1c, tmp5);
	odm_write_4byte(dm, 0x1b28, tmp6);
	odm_write_4byte(dm, 0x1b90, tmp7);
	odm_write_4byte(dm, 0x1bcc, tmp8);
	odm_write_4byte(dm, 0x1b2c, tmp9);
	odm_write_4byte(dm, 0x1b30, tmp10);
	odm_write_4byte(dm, 0x1b38, 0x20000000);
}

void _iqk_readsram_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u32 tmp1bd4, tmp1bd8, tmp;
	u8 i;

	tmp1bd4 = odm_read_4byte(dm, 0x1bd4);
	tmp1bd8 = odm_read_4byte(dm, 0x1bd8);
	odm_write_4byte(dm, 0x1bd4, 0x00010001);
	for (i = 0; i < 0x80; i++) {
		odm_write_4byte(dm, 0x1bd8, 0xa0000101 + (u32)(i << 1));
		tmp = (u32)odm_read_4byte(dm, 0x1bfc) & 0x3ff;
		if (i < 0x40)
			RF_DBG(dm, DBG_RF_IQK, "adc_i[%d] = %x\n", i, tmp);
		else
			RF_DBG(dm, DBG_RF_IQK, "adc_q[%d] = %x\n", i, tmp);
	}
}

void do_imr_test_8195b(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_IQK,
	       "[IQK]  ************IMR Test *****************\n");
	_phy_iq_calibrate_8195b(dm, false, false, true);
	RF_DBG(dm, DBG_RF_IQK,
	       "[IQK]  **********End IMR Test *******************\n");
}

/*********debug message end***************/

/*IQK version:0x23, NCTL:0x8*/
/*1. modify the iqk counters for coex.*/

void phy_iq_calibrate_8195b(
	void *dm_void,
	boolean clear,
	boolean segment_iqk)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &(dm->rf_table);

	//if (*(dm->mp_mode))
	//	halrf_iqk_hwtx_check(dm, true);

	RF_DBG(dm, DBG_RF_IQK, "[IQK]  %s \n", __func__);
	_iq_calibrate_8195b_init(dm);
	//_phy_iq_calibrate_8195b(dm, clear, segment_iqk, false);
	_phy_iq_calibrate_8195b(dm, clear, false, false);
	_iqk_fail_count_8195b(dm);

//	if (*(dm->mp_mode))
//		halrf_iqk_hwtx_check(dm, false);
#if (dm_SUPPORT_TYPE & ODM_IOT)
	_iqk_iqk_fail_report_8195b(dm);
#endif
	//halrf_iqk_dbg(dm);
	_phy_dpd_calibrate_8195b(dm, true);

}
#endif
