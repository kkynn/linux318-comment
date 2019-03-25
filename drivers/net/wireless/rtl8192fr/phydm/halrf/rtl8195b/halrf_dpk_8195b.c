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

static boolean txgap_done[3] = {false, false, false};
static boolean overflowflag = false;
#define dpk_forcein_sram4 1
#define txgap_ref_index 0x0
#define txgapK_number 0x7

/*---------------------------Define Local Constant---------------------------*/
void do_dpk_8195b(
	void *dm_void,
	u8 delta_thermal_index,
	u8 thermal_value,
	u8 threshold)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	/*boolean		is_recovery = (boolean) delta_thermal_index;*/
	phy_dp_calibrate_8195b(dm, true);
}
boolean
_dpk_check_nctl_done_8195b(
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
			RF_DBG(dm, DBG_RF_DPK, "0x1f7c[31:24] = %x\n",
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
			RF_DBG(dm, DBG_RF_DPK,
			       "[DPK]S%d IQK_CMD =%x timeout!!!\n", path,
			       IQK_CMD);
			break;
		}
	}
	//odm_write_1byte(dm, 0x1b10, 0x0);
	odm_write_1byte(dm, 0x1f7f, 0x0);

	if (fail == false) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK]S%d 1b08[26]=0x0 PASS !!!\n", path);
	} else {
		RF_DBG(dm, DBG_RF_DPK, "[DPK]S%d 1b08[26]=0x1 FAIL !!!\n", path);
	}
	return fail;
}

void _dpk_backup_mac_bb_8195b(
	struct dm_struct *dm,
	u32 *MAC_backup,
	u32 *BB_backup,
	u32 *backup_mac_reg,
	u32 *backup_bb_reg,
	u8 num_backup_bb_reg)
{
	u32 i;

	for (i = 0; i < DPK_MAC_REG_NUM_8195B; i++)
		MAC_backup[i] = odm_read_4byte(dm, backup_mac_reg[i]);

	for (i = 0; i < num_backup_bb_reg; i++)
		BB_backup[i] = odm_read_4byte(dm, backup_bb_reg[i]);
	/*	RF_DBG(dm, DBG_RF_DPK, ("[IQK]BackupMacBB Success!!!!\n")); */
}

void _dpk_backup_rf_8195b(
	struct dm_struct *dm,
	u32 RF_backup[][SS_8195B],
	u32 *backup_rf_reg)
{
	u32 i, j;

	for (i = 0; i < DPK_RF_REG_NUM_8195B; i++)
		for (j = 0; j < SS_8195B; j++) /*j= path*/
			RF_backup[i][j] = odm_get_rf_reg(dm, (u1Byte)j, backup_rf_reg[i], RFREGOFFSETMASK);
	/*	RF_DBG(dm, DBG_RF_DPK, ("[IQK]BackupRF Success!!!!\n")); */
}

void _dpk_afe_setting_8195b(
	struct dm_struct *dm,
	boolean do_iqk)
{
	if (do_iqk) {
		RF_DBG(dm, DBG_RF_DPK, "[IQK]AFE setting for IQK mode!!!!\n");
		/*AFE setting*/
		odm_write_4byte(dm, 0xc60, 0x70000000);
		odm_write_4byte(dm, 0xc60, 0x700F0040);
#if 0
		odm_write_4byte(dm, 0x808, 0x2D028200);
		odm_write_4byte(dm, 0x810, 0x20101063);
		odm_write_4byte(dm, 0x90c, 0x0B00C000);
		odm_write_4byte(dm, 0x9a4, 0x00000080);
#endif
		// for page 1b can read/write
		odm_write_4byte(dm, 0x1c44, 0xa34300F3);
		odm_write_4byte(dm, 0xc94, 0x0100010d);
		odm_set_bb_reg(dm, 0x1904, BIT(17), 0x1);
	} else {
		RF_DBG(dm, DBG_RF_DPK,
		       "[IQK]AFE setting for Normal mode!!!!\n");
		odm_write_4byte(dm, 0xc60, 0x70000000);
		odm_write_4byte(dm, 0xc60, 0x70070040);
		odm_write_4byte(dm, 0xc94, 0x01000101);
#if 0
		odm_write_4byte(dm, 0x808, 0x34028211);
		odm_write_4byte(dm, 0x810, 0x21104285);
		odm_write_4byte(dm, 0x90c, 0x13000400);
		odm_write_4byte(dm, 0x9a4, 0x80000088);
		odm_write_4byte(dm, 0xc94, 0x01000101);
		odm_write_4byte(dm, 0x1904, 0x00238000);
#endif
	}
	/*0x9a4[31]=0: Select da clock*/
//	odm_set_bb_reg(dm, 0x9a4, BIT(31), 0x0);
}

void _dpk_restore_mac_bb_8195b(
	struct dm_struct *dm,
	u32 *MAC_backup,
	u32 *BB_backup,
	u32 *backup_mac_reg,
	u32 *backup_bb_reg,
	u8 num_backup_bb_reg)
{
	u32 i;

	for (i = 0; i < MAC_REG_NUM_8195B; i++)
		odm_write_4byte(dm, backup_mac_reg[i], MAC_backup[i]);
	for (i = 0; i < num_backup_bb_reg; i++)
		odm_write_4byte(dm, backup_bb_reg[i], BB_backup[i]);

	/*	RF_DBG(dm, DBG_RF_DPK, ("[IQK]RestoreMacBB Success!!!!\n")); */
}

void _dpk_restore_rf_8195b(
	struct dm_struct *dm,
	u32 *backup_rf_reg,
	u32 RF_backup[][SS_8195B])
{
	u32 i;

	odm_set_rf_reg(dm, RF_PATH_A, 0xee, RFREGOFFSETMASK, 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, 0xde, RFREGOFFSETMASK, 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, 0x8f, BIT(14) | BIT(13), 0x0);
	for (i = 0; i < DPK_RF_REG_NUM_8195B ; i++)
		odm_set_rf_reg(dm, RF_PATH_A, backup_rf_reg[i], RFREGOFFSETMASK, RF_backup[i][RF_PATH_A]);
}

void _dpk_backup_iqk_8195b(
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
		iqk_info->lok_idac[0][path] = odm_get_rf_reg(dm, (enum rf_path)path, 0x58, RFREGOFFSETMASK);
		break;
	case 2: /*TXIQK backup*/
	case 3: /*RXIQK backup*/
		phydm_get_iqk_cfir(dm, (step - 2), path, false);
		break;
	}
}

void _dpk_reload_iqk_setting_8195b(
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
			odm_set_bb_reg(dm, 0x1b00, MASKDWORD, 0xf8000008 | path << 1);
			odm_set_bb_reg(dm, 0x1b20, 0x0f000000, 0x7);
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
_dpk_reload_iqk_8195b(
	struct dm_struct *dm,
	boolean reset)
{
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
				_dpk_reload_iqk_setting_8195b(dm, i, 2);
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK]reload IQK result before!!!!\n");
				iqk_info->is_reload = true;
			}
		}
	}

	return iqk_info->is_reload;
}

void _dpk_rfe_setting_8195b(
	struct dm_struct *dm,
	boolean ext_pa_on)
{
	if (ext_pa_on) {
		/*RFE setting*/
		odm_write_4byte(dm, 0xcb0, 0x77777777);
		odm_write_4byte(dm, 0xcb4, 0x00007777);
		odm_write_4byte(dm, 0xcbc, 0x0000083B);
		/*odm_write_4byte(dm, 0x1990, 0x00000c30);*/
		RF_DBG(dm, DBG_RF_DPK, "[DPK]external PA on!!!!\n");
	} else {
		/*RFE setting*/
		odm_write_4byte(dm, 0xcb0, 0x77171117);
		odm_write_4byte(dm, 0xcb4, 0x00001177);
		odm_write_4byte(dm, 0xcbc, 0x00000404);
		/*odm_write_4byte(dm, 0x1990, 0x00000c30);*/
		RF_DBG(dm, DBG_RF_DPK, "[DPK]external PA off!!!!\n");
	}
}
void _dpk_rfsetting_8195b(
	struct dm_struct *dm)
{
	struct dm_iqk_info	*iqk_info = &dm->IQK_info;
	u32 rf_reg00=0x0l;
	u8 path = 0x0;;
	u8 tx_agc_init_value = 0x1c; /* DPK TXAGC value*/

	if (*dm->band_type == ODM_BAND_5G) {
		tx_agc_init_value = 0x19;
		rf_reg00 = 0x50000 + tx_agc_init_value; /* set TXAGC value*/
		odm_set_rf_reg(dm, RF_PATH_A, 0x8F, RFREGOFFSETMASK, 0xA8040);
		//odm_set_rf_reg(dm, RF_PATH_A, 0x63, RFREGOFFSETMASK, 0x0126A);
		odm_set_rf_reg(dm, RF_PATH_A, 0x63, BIT(15) | BIT(14) | BIT(13) | BIT(12), 0x1);
		//odm_set_rf_reg(dm, RF_PATH_A, 0x8c, RFREGOFFSETMASK, 0x0108C);
		odm_set_rf_reg(dm, RF_PATH_A, 0x8c, BIT(8) | BIT(7) | BIT(6), 0x2);
		odm_set_rf_reg(dm, RF_PATH_A, 0x00, RFREGOFFSETMASK, rf_reg00);

		RF_DBG(dm, DBG_RF_DPK, "\n [DPK] This is 5G !!!!\n");

	} else {
		tx_agc_init_value = 0x1c;
		rf_reg00 = 0x50000 + tx_agc_init_value; /* set TXAGC value*/
		odm_set_rf_reg(dm, RF_PATH_A, 0x8F, RFREGOFFSETMASK, 0xA8040);
		odm_set_rf_reg(dm, RF_PATH_A, 0x63, BIT(15) | BIT(14) | BIT(13) | BIT(12), 0x3);
		odm_set_rf_reg(dm, RF_PATH_A, 0x8c, BIT(8) | BIT(7) | BIT(6), 0x6);
		//odm_set_rf_reg(dm, RF_PATH_A, 0x57, RFREGOFFSETMASK, 0x9B202);
		odm_set_rf_reg(dm, RF_PATH_A, 0x57, BIT(2) | BIT(1), 0x1);
		odm_set_rf_reg(dm, RF_PATH_A, 0x57, BIT(6) | BIT(5) | BIT(4), 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, 0x00, RFREGOFFSETMASK, rf_reg00);

		RF_DBG(dm, DBG_RF_DPK, "\n [DPK] This is 2G !!!!\n");
	}

}

void _dpk_configure_macbb_8195b(
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
	/*	RF_DBG(dm, DBG_RF_DPK, ("[IQK]Set MACBB setting for IQK!!!!\n"));*/
}

void _dpk_toggle_rxagc_8195b(
	struct dm_struct *dm,
	boolean reset

	)
{
	/*toggle RXAGC  workaround method*/
	u32 tmp1;

	tmp1 = odm_read_4byte(dm, 0xc50);
	odm_set_bb_reg(dm, 0xc50, BIT(3) | BIT(2) | BIT(1) | BIT(0), 0x0);
	ODM_delay_ms(2);
	odm_set_bb_reg(dm, 0xc50, BIT(3) | BIT(2) | BIT(1) | BIT(0), 0x2);
	ODM_delay_ms(2);
	odm_set_bb_reg(dm, 0xc50, BIT(3) | BIT(2) | BIT(1) | BIT(0), 0x0);
	odm_write_4byte(dm, 0xc50, tmp1);
}

void _dpk_set_gain_scaling_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u32 tmp1, tmp2, tmp3, tmp4, reg_1bfc;
	u32 lut_i = 0x0, lut_q = 0x0, lut_pw = 0x0, lut_pw_avg = 0x0;
	u16 gain_scaling = 0x0;

	tmp1 = odm_read_4byte(dm, 0x1b00);
	tmp2 = odm_read_4byte(dm, 0x1b08);
	tmp3 = odm_read_4byte(dm, 0x1bd4);
	tmp4 = odm_read_4byte(dm, 0x1bdc);

	odm_write_4byte(dm, 0x1b00, 0xf8000008);
	odm_write_4byte(dm, 0x1b08, 0x00000080);
	odm_write_4byte(dm, 0x1bd4, 0x00040001);
	odm_write_4byte(dm, 0x1bdc, 0xc0000081);

	reg_1bfc = odm_read_4byte(dm, 0x1bfc);
	lut_i = (reg_1bfc & 0x003ff800) >> 11;
	lut_q = (reg_1bfc & 0x00007ff);

	if ((lut_i & 0x400) == 0x400)
		lut_i = 0x800 - lut_i;
	if ((lut_q & 0x400) == 0x400)
		lut_q = 0x800 - lut_q;

	lut_pw = lut_i * lut_i + lut_q * lut_q;
	lut_pw_avg = (u32)(lut_i + lut_q) >> 1;
	gain_scaling = (u16)(0x800000 / lut_pw_avg);

	odm_set_bb_reg(dm, 0x1b98, 0x0000ffff, gain_scaling);
	odm_set_bb_reg(dm, 0x1b98, 0xffff0000, gain_scaling);

	odm_write_4byte(dm, 0x1b00, tmp1);
	odm_write_4byte(dm, 0x1b08, tmp2);
	odm_write_4byte(dm, 0x1bd4, tmp3);
	odm_write_4byte(dm, 0x1bdc, tmp4);

	RF_DBG(dm, DBG_RF_DPK,
	       "[IQK] reg_1bfc = 0x%x, lut_pw = 0x%x, lut_i = 0x%x, lut_q = 0x%x, lut_pw_avg = 0x%x, gain_scaling = 0x%x, 0x1b98 =0x%x!!!\n",
	       reg_1bfc, lut_pw, lut_i, lut_q, lut_pw_avg, gain_scaling,
	       odm_read_4byte(dm, 0x1b98));
	return;
}

void _dpk_set_dpk_pa_scan_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u32 tmp1, tmp2, reg_1bfc;
	u32 pa_scan_i = 0x0, pa_scan_q = 0x0, pa_scan_pw = 0x0;
	u32 gainloss_back = 0x0;
	/*	boolean pa_scan_search_fail = false;*/
	tmp1 = odm_read_1byte(dm, 0x1bcf);
	tmp2 = odm_read_4byte(dm, 0x1bd4);
	odm_write_1byte(dm, 0x1bcf, 0x11);
	odm_write_4byte(dm, 0x1bd4, 0x00060000);
	reg_1bfc = odm_read_4byte(dm, 0x1bfc);
	odm_write_1byte(dm, 0x1bcf, 0x15); //return to pa-scan
	pa_scan_i = (reg_1bfc & 0xffff0000) >> 16;
	pa_scan_q = (reg_1bfc & 0x0000ffff);

	if ((pa_scan_i & 0x8000) == 0x8000)
		pa_scan_i = 0x10000 - pa_scan_i;
	if ((pa_scan_q & 0x8000) == 0x8000)
		pa_scan_q = 0x10000 - pa_scan_q;

	pa_scan_pw = pa_scan_i * pa_scan_i + pa_scan_q * pa_scan_q;

	/*estimated pa_scan_pw*/
	/*0dB => (512^2) * 10^(0/10) = 262144*/
	/*1dB => (512^2) * 10^(1/10) = 330019*/
	/*2dB => (512^2) * 10^(2/10) = 415470*/
	/*3dB => (512^2) * 10^(3/10) = 523046*/
	/*4dB => (512^2) * 10^(4/10) = 658475*/
	/*5dB => (512^2) * 10^(5/10) = 828972*/
	/*6dB => (512^2) * 10^(6/10) = 1043614*/
	/*7dB => (512^2) * 10^(7/10) = 1313832*/
	/*8dB => (512^2) * 10^(0/10) = 1654016*/
	/*9dB => (512^2) * 10^(1/10) = 2082283*/
	/*10dB => (512^2) * 10^(2/10) = 2621440*/
	/*11dB => (512^2) * 10^(3/10) = 3300197*/
	/*12dB => (512^2) * 10^(4/10) = 4154702*/
	/*13dB => (512^2) * 10^(5/10) = 5230460*/
	/*14dB => (512^2) * 10^(6/10) = 6584759*/
	/*15dB => (512^2) * 10^(7/10) = 8289721*/
	if (pa_scan_pw >= 0x7e7db9)
		pa_scan_pw = 0x0f;
	else if (pa_scan_pw >= 0x6479b7)
		pa_scan_pw = 0x0e;
	else if (pa_scan_pw >= 0x4fcf7c)
		pa_scan_pw = 0x0d;
	else if (pa_scan_pw >= 0x3f654e)
		pa_scan_pw = 0x0c;
	else if (pa_scan_pw >= 0x325b65)
		pa_scan_pw = 0x0b;
	else if (pa_scan_pw >= 0x280000)
		pa_scan_pw = 0x0a;
	else if (pa_scan_pw >= 0x1fc5eb)
		pa_scan_pw = 0x09;
	else if (pa_scan_pw >= 0x193d00)
		pa_scan_pw = 0x8;
	else if (pa_scan_pw >= 0x140c28)
		pa_scan_pw = 0x7;
	else if (pa_scan_pw >= 0xefc9e)
		pa_scan_pw = 0x6;
	else if (pa_scan_pw >= 0xca62c)
		pa_scan_pw = 0x5;
	else if (pa_scan_pw >= 0xa0c2b)
		pa_scan_pw = 0x4;
	else if (pa_scan_pw >= 0x7fb26)
		pa_scan_pw = 0x3;
	else if (pa_scan_pw >= 0x656ee)
		pa_scan_pw = 0x2;
	else if (pa_scan_pw >= 0x50923)
		pa_scan_pw = 0x1;
	else /*262144 >= pa_scan_pw*/
		pa_scan_pw = 0x0;

	odm_write_4byte(dm, 0x1bd4, 0x00060001);
	gainloss_back = (odm_read_4byte(dm, 0x1bfc) & 0x0000000f);

	if (gainloss_back <= 0xa)
		gainloss_back = 0xa - gainloss_back;

	if (gainloss_back > pa_scan_pw + 0x8)
		odm_set_rf_reg(dm, path, 0x8f, BIT(14) | BIT(13), 0x3);
	else if ((pa_scan_pw + 0x8 - gainloss_back) >= 0x6)
		odm_set_rf_reg(dm, path, 0x8f, BIT(14) | BIT(13), 0x00);
	else
		odm_set_rf_reg(dm, path, 0x8f, BIT(14) | BIT(13), 0x01);

	odm_write_4byte(dm, 0x1bcc, tmp1);
	odm_write_4byte(dm, 0x1bd4, tmp2);
	return;
}

void _dpk_disable_bb_dynamic_pwr_threshold_8195b(
	struct dm_struct *dm,
	boolean flag)
{
	if (flag == true) /*disable BB dynamic pwr threshold hold*/
		odm_set_bb_reg(dm, 0x1c74, BIT(31) | BIT(30) | BIT(29) | BIT(28), 0x0);
	else
		odm_set_bb_reg(dm, 0x1c74, BIT(31) | BIT(30) | BIT(29) | BIT(28), 0x2);
	RF_DBG(dm, DBG_RF_DPK, "[DPK]\nset 0x1c74 = 0x%x\n",
	       odm_read_4byte(dm, 0x1c74));
}

void _dpk_set_dpk_sram_to_10_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u32 tmp1, tmp2;
	u8 i;

	tmp1 = odm_read_4byte(dm, 0x1b00);
	tmp2 = odm_read_4byte(dm, 0x1b08);

	for (i = 0; i < 64; i++)
		odm_write_4byte(dm, 0x1bdc, 0xd0000001 + (i * 2) + 1);

	for (i = 0; i < 64; i++)
		odm_write_4byte(dm, 0x1bdc, 0x90000080 + (i * 2) + 1);

	odm_write_4byte(dm, 0x1bdc, 0x0);
	odm_write_4byte(dm, 0x1b00, tmp1);
	odm_write_4byte(dm, 0x1b08, tmp2);
}

void _dpk_set_bbtxagc_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u8 hw_rate, tmp;

	for (hw_rate = 0; hw_rate < 0x53; hw_rate++) {
		phydm_write_txagc_1byte_8195b(dm, 0x30, (enum rf_path)0x0, hw_rate);
		tmp = config_phydm_read_txagc_8195b(dm, (enum rf_path)0x0, hw_rate);
	}
}

u8 _dpk_get_txagcindpk_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u32 tmp1, tmp2, tmp3;
	u8 tmp4;

	tmp1 = odm_read_4byte(dm, 0x1bcc);
	odm_set_bb_reg(dm, 0x1bcc, BIT(26), 0x01);
	tmp2 = odm_read_4byte(dm, 0x1bd4);
	odm_set_bb_reg(dm, 0x1bd4, BIT(20) | BIT(19) | BIT(18) | BIT(17) | BIT(16), 0x0a);
	tmp3 = odm_read_4byte(dm, 0x1bfc);
	tmp4 = ((u8)odm_get_bb_reg(dm, 0x1bfc, MASKDWORD)) >> 2;
	odm_write_4byte(dm, 0x1bcc, tmp1);
	odm_write_4byte(dm, 0x1bd4, tmp2);
	return tmp4;
}

void _dpk_PAS_GL_read_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u8 i;
	u32 tmp;
	RF_DBG(dm, DBG_RF_DPK, "\n");
	RF_DBG(dm, DBG_RF_DPK, "_dpk_PAS_GL_read_8195b\n");
	odm_set_bb_reg(dm, 0x1bcc,  BIT(26), 0x0);
	for (i = 0; i < 8; i++) {
		odm_write_4byte(dm, 0x1b90, 0x0105e038 + i);
		odm_write_4byte(dm, 0x1bd4, 0x00060000);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
		odm_write_4byte(dm, 0x1bd4, 0x00070000);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
		odm_write_4byte(dm, 0x1bd4, 0x00080000);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
		odm_write_4byte(dm, 0x1bd4, 0x00090000);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
	}
	RF_DBG(dm, DBG_RF_DPK, "\n");
	RF_DBG(dm, DBG_RF_DPK, "\n");
}

void _dpk_PAS_DPK_read_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u8 i;
	u32 tmp;

	RF_DBG(dm, DBG_RF_DPK, "\n");
	RF_DBG(dm, DBG_RF_DPK, "_dpk_PAS_DPK_read_8195b\n");
	//odm_set_bb_reg(dm, 0x1bcc,  BIT(26), 0x0);
#if 1
	for (i = 0; i < 8; i++) {
		odm_write_4byte(dm, 0x1b90, 0x0109e038 + i);
		odm_write_4byte(dm, 0x1bd4, 0x00060000);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
		odm_write_4byte(dm, 0x1bd4, 0x00070000);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
		odm_write_4byte(dm, 0x1bd4, 0x00080000);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
		odm_write_4byte(dm, 0x1bd4, 0x00090000);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
	}
	//odm_write_4byte(dm, 0x1b90, 0x0001e018);
#else

#endif
	RF_DBG(dm, DBG_RF_DPK, "\n");
	RF_DBG(dm, DBG_RF_DPK, "\n");
}
void _dpk_readsram_8195b(
	struct dm_struct *dm,
	u8 path)
{
	/* dbg message*/
	u8 i;

	odm_write_4byte(dm, 0x1b00, 0x00000008);
	odm_write_4byte(dm, 0x1b08, 0x00000080);
	odm_write_4byte(dm, 0x1bd4, 0x00040001);

	RF_DBG(dm, DBG_RF_DPK, "\n");
	RF_DBG(dm, DBG_RF_DPK, "_dpk_readsram_8195b\n");

	for (i = 0; i < 64; i++){
		odm_write_4byte(dm, 0x1bdc, 0xc0000081 + i * 2);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
	}
	odm_write_4byte(dm, 0x1bd4, 0x00050001);

	for (i = 0; i < 64; i++){
		odm_write_4byte(dm, 0x1bdc, 0xc0000081 + i * 2);
		RF_DBG(dm, DBG_RF_DPK, "0x%x\n", odm_read_4byte(dm, 0x1bfc));
		}
	ODM_delay_ms(200);
	//odm_write_4byte(dm, 0x1bd4, 0xA0001);
	odm_write_4byte(dm, 0x1bdc, 0x00000000);

	RF_DBG(dm, DBG_RF_DPK, "\n");
	RF_DBG(dm, DBG_RF_DPK, "\n");
}

void _dpk_restore_8195b(
	struct dm_struct *dm,
	u8 path)
{
#if 0
	odm_write_4byte(dm, 0xc60, 0x700B8040);
	odm_write_4byte(dm, 0xc60, 0x700B8040);
	odm_write_4byte(dm, 0xc60, 0x70146040);
	odm_write_4byte(dm, 0xc60, 0x70246040);
	odm_write_4byte(dm, 0xc60, 0x70346040);
	odm_write_4byte(dm, 0xc60, 0x70446040);
	odm_write_4byte(dm, 0xc60, 0x705B2040);
	odm_write_4byte(dm, 0xc60, 0x70646040);
	odm_write_4byte(dm, 0xc60, 0x707B8040);
	odm_write_4byte(dm, 0xc60, 0x708B8040);
	odm_write_4byte(dm, 0xc60, 0x709B8040);
	odm_write_4byte(dm, 0xc60, 0x70aB8040);
	odm_write_4byte(dm, 0xc60, 0x70bB6040);
	odm_write_4byte(dm, 0xc60, 0x70c06040);
	odm_write_4byte(dm, 0xc60, 0x70d06040);
	odm_write_4byte(dm, 0xc60, 0x70eF6040);
	odm_write_4byte(dm, 0xc60, 0x70f06040);

	odm_write_4byte(dm, 0xc58, 0xd8020402);
	odm_write_4byte(dm, 0xc5c, 0xde000120);
	odm_write_4byte(dm, 0xc6c, 0x0000122a);

	odm_write_4byte(dm, 0x808, 0x24028211);
	odm_write_4byte(dm, 0x90c, 0x13000000);
	odm_write_4byte(dm, 0x9a4, 0x80000088);
	odm_write_4byte(dm, 0xc94, 0x01000101);
	odm_write_4byte(dm, 0x1904, 0x00238000);
	odm_write_4byte(dm, 0x1904, 0x00228000);
	odm_write_4byte(dm, 0xC00, 0x00000007);
#endif
}

void _dpk_clear_sram_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u8 i;

	/*clear sram even*/
	for (i = 0; i < 0x40; i++)
		odm_write_4byte(dm, 0x1bdc, 0xd0000000 + ((i * 2) + 1));
	/*clear sram odd*/
	for (i = 0; i < 0x40; i++)
		odm_write_4byte(dm, 0x1bdc, 0x90000080 + ((i * 2) + 1));

	odm_write_4byte(dm, 0x1bdc, 0x0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK]==========write pwsf and clear sram/n");
}

void _dpk_setting_8195b(
	struct dm_struct *dm,
	u8 path)
{
#if 0
	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK]==========Start the DPD setting Initilaize/n");
	/*AFE setting*/
	odm_write_4byte(dm, 0xc60, 0x50000000);
	odm_write_4byte(dm, 0xc60, 0x700F0040);
	odm_write_4byte(dm, 0xc5c, 0xd1000120);
	odm_write_4byte(dm, 0xc58, 0xd8000402);
	odm_write_4byte(dm, 0xc6c, 0x00000a15);
	odm_write_4byte(dm, 0xc00, 0x00000004);
	/*_iqk_bb_reset_8195b(dm);*/
	odm_write_4byte(dm, 0xe5c, 0xD1000120);
	odm_write_4byte(dm, 0xc6c, 0x00000A15);
	odm_write_4byte(dm, 0xe6c, 0x00000A15);
	odm_write_4byte(dm, 0x808, 0x2D028200);
	/*odm_write_4byte(dm, 0x810, 0x211042A5);*/
	odm_write_4byte(dm, 0x8f4, 0x00d80fb1);
	odm_write_4byte(dm, 0x90c, 0x0B00C000);
	odm_write_4byte(dm, 0x9a4, 0x00000080);
	odm_write_4byte(dm, 0xc94, 0x01000101);
	odm_write_4byte(dm, 0xe94, 0x01000101);
	odm_write_4byte(dm, 0xe5c, 0xD1000120);
	odm_write_4byte(dm, 0xc6c, 0x00000A15);
	odm_write_4byte(dm, 0xe6c, 0x00000A15);
	odm_write_4byte(dm, 0x1904, 0x00020000);
	/*path A*/
	/*RF*/
	odm_set_rf_reg(dm, RF_PATH_A, 0xEF, RFREGOFFSETMASK, 0x80000);
	odm_set_rf_reg(dm, RF_PATH_A, 0x33, RFREGOFFSETMASK, 0x00024);
	odm_set_rf_reg(dm, RF_PATH_A, 0x3E, RFREGOFFSETMASK, 0x0003F);
	odm_set_rf_reg(dm, RF_PATH_A, 0x3F, RFREGOFFSETMASK, 0xCBFCE);
	odm_set_rf_reg(dm, RF_PATH_A, 0xEF, RFREGOFFSETMASK, 0x00000);
	/*AGC boundary selection*/
	odm_write_4byte(dm, 0x1bbc, 0x0001abf6);
	odm_write_4byte(dm, 0x1b90, 0x0001e018);
	odm_write_4byte(dm, 0x1bb8, 0x000fffff);
	odm_write_4byte(dm, 0x1bc8, 0x000c55aa);
	/*odm_write_4byte(dm, 0x1bcc, 0x11978200);*/
	odm_write_4byte(dm, 0x1bcc, 0x11978800);
#endif
}

void _dpk_dynamic_bias_8195b(
	struct dm_struct *dm,
	u8 path,
	u8 dynamicbias)
{
	u32 tmp;

	tmp = odm_get_rf_reg(dm, RF_PATH_A, 0xdf, RFREGOFFSETMASK);
	tmp = tmp | BIT(8);
	odm_set_rf_reg(dm, RF_PATH_A, 0xdf, RFREGOFFSETMASK, tmp);
	if ((*dm->band_type == ODM_BAND_5G) && (*dm->band_width == 1))
		odm_set_rf_reg(dm, path, 0x61, BIT(7) | BIT(6) | BIT(5) | BIT(4), dynamicbias);
	if ((*dm->band_type == ODM_BAND_5G) && (*dm->band_width == 2))
		odm_set_rf_reg(dm, path, 0x61, BIT(7) | BIT(6) | BIT(5) | BIT(4), dynamicbias);
	tmp = tmp & (~BIT(8));
	odm_set_rf_reg(dm, RF_PATH_A, 0xdf, RFREGOFFSETMASK, tmp);
	RF_DBG(dm, DBG_RF_DPK, "[DPK]Set DynamicBias 0xdf=0x%x, 0x61=0x%x\n",
	       odm_get_rf_reg(dm, RF_PATH_A, 0xdf, RFREGOFFSETMASK),
	       odm_get_rf_reg(dm, RF_PATH_A, 0x61, RFREGOFFSETMASK));
}

void _dpk_boundary_selection_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u8 tmp_pad, compared_pad, compared_txbb;
	u32 rf_backup_reg00;
	u8 i = 0;
	u8 j = 1;
	u32 boundaryselect = 0;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_DPK, "[DPK] Start the DPD boundary selection\n");
	rf_backup_reg00 = odm_get_rf_reg(dm, (enum rf_path)path, 0x00, RFREGOFFSETMASK);
	tmp_pad = 0;
	compared_pad = 0;

#if 0
	odm_write_4byte(dm, 0x1bb8, 0x000fffff);
	for (i = 0x1f; i > 0x0; i--) { /*i=tx index*/
		odm_set_rf_reg(dm, RF_PATH_A, 0x00, RFREGOFFSETMASK, 0x50000 + i);
		compared_pad = (u8)((0xe0 & odm_get_rf_reg(dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK)) >> 5);
		compared_txbb = (u8)((0x1f & odm_get_rf_reg(dm, (enum rf_path)path, 0x56, RFREGOFFSETMASK)));
		if (i == 0x1f) {
			/*boundaryselect = compared_txbb;*/
			boundaryselect = 0x1f;
			tmp_pad = compared_pad;
		}
		if (compared_pad < tmp_pad) {
			boundaryselect = boundaryselect + (i << (j * 5));
			tmp_pad = compared_pad;
			j++;
		}

		if (j >= 4)
			break;
	}
	odm_write_4byte(dm, 0x1bbc, boundaryselect);
#else
	odm_write_4byte(dm, 0x1bb8, 0x000fffff);
	odm_write_4byte(dm, 0x1bbc, 0x00009DBF);
#endif
	odm_set_rf_reg(dm, RF_PATH_A, 0x00, RFREGOFFSETMASK, rf_backup_reg00);
	RF_DBG(dm, DBG_RF_DPK, "[DPK]DPD boundary selection 0x1bbc = %x\n",
	       boundaryselect);

}

u8 _dpk_get_tx_agc_8195b(
	struct dm_struct *dm,
	u8 path)
{
	u8 tx_agc_init_value = 0x1a; /* DPK TXAGC value*/
	u32 rf_reg00 = 0x0;
	u8 gainloss = 0x1;
	u8 best_tx_agc, txagcindpk = 0x0;
	u8 tmp;
	boolean fail = true;
	u32 IQK_CMD = 0x00001118;

	odm_write_4byte(dm, 0x1b00, 0x00000008);
	odm_write_1byte(dm, 0x1b92, 0x05);

	if (*dm->band_type == ODM_BAND_5G)
		tx_agc_init_value = 0x15;
	else
		tx_agc_init_value = 0x1a;

	odm_set_rf_reg(dm, RF_PATH_A, 0x00, RFREGOFFSETMASK, 0x50000 + tx_agc_init_value);

	_dpk_set_bbtxagc_8195b(dm, RF_PATH_A);
	//txagcindpk = _dpk_get_txagcindpk_8195b(dm, RF_PATH_A);

	ODM_delay_ms(1);
	odm_write_4byte(dm, 0x1b00, IQK_CMD);
	odm_write_4byte(dm, 0x1b00, IQK_CMD + 1);

	fail = _dpk_check_nctl_done_8195b(dm, path, IQK_CMD);
	if (fail == false){
		odm_write_4byte(dm, 0x1b00, 0x00000008);
		odm_write_4byte(dm, 0x1bd4, 0x00060000);
		tmp = (u8)odm_read_4byte(dm, 0x1bfc);
		best_tx_agc = tx_agc_init_value - (0xa - tmp);
	}
	else
		best_tx_agc = 0x19; /*wait for check*/

	//restore for IQK
	odm_set_bb_reg(dm, 0x1bcc, BIT(16), 0x0);

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] _dpk_get_tx_agc_8195b, tmp = %x, best_tx_agc = %x\n", tmp,
	       best_tx_agc);

	return best_tx_agc;
}

boolean
_dpk_enable_dpk_8195b(
	struct dm_struct *dm,
	u8 path,
	u8 best_tx_agc)
{
	u32 rf_reg00 = 0x0;
	boolean fail = true;
	u32 IQK_CMD = 0x00001218;
	u8 offset;

	if (*dm->band_type == ODM_BAND_5G)
		rf_reg00 = 0x50000 + best_tx_agc; /* set TXAGC value*/
	else
		rf_reg00 = 0x50000 + best_tx_agc; /* set TXAGC value*/
	odm_set_rf_reg(dm, RF_PATH_A, 0x00, RFREGOFFSETMASK, rf_reg00);
	//_dpk_set_dpk_pa_scan_8195b(dm, RF_PATH_A);

	ODM_delay_ms(1);

	odm_write_4byte(dm, 0x1b00, 0x00000008);
	odm_write_1byte(dm, 0x1b92, 0x05);

	if (best_tx_agc >= 0x19)
		offset = best_tx_agc - 0x19;
	else
		offset = 0x20 - (0x19 - best_tx_agc);
	odm_set_bb_reg(dm, 0x1bd0, BIT(12) | BIT(11) | BIT(10) | BIT(9) | BIT(8), offset);
	odm_write_4byte(dm, 0x1b00, IQK_CMD);
	odm_write_4byte(dm, 0x1b00, IQK_CMD + 1);
	if( _dpk_check_nctl_done_8195b(dm, path, IQK_CMD))
		RF_DBG(dm, DBG_RF_DPK, "[DPK]In DPD Process(3), DPK process is Fail\n");

	//read DPK fail report
	odm_set_bb_reg(dm, 0x1bcc, BIT(26), 0x0);
	odm_write_1byte(dm, 0x1bd6, 0xa);
	fail = (boolean) (BIT(0) & odm_read_4byte(dm, 0x1bfc));
	//restore for IQK
	odm_set_bb_reg(dm, 0x1bcc, BIT(16), 0x0);

	return fail;
}

void _dpk_init_8195b(
	struct dm_struct *dm)
{
RF_DBG(dm, DBG_RF_DPK, "[DPK]==========_dpk_init_8195b !!!!!==========\n");

/*IQK INIT*/
	odm_write_4byte(dm, 0x1c44, 0xa34300F3);
//	odm_write_4byte(dm, 0x0c94, 0x01000101);
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
	odm_write_4byte(dm, 0x1b1c, 0xA21FFC32);
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
	// pwsf boundary
	odm_write_4byte(dm, 0x1bb8, 0x000fffff);
	// LUT SRAM block selection
	odm_write_4byte(dm, 0x1bbc, 0x00009DBF);
	// tx_amp
	odm_write_4byte(dm, 0x1b98, 0x41382e21);
	odm_write_4byte(dm, 0x1b9c, 0x5b554f48);
	odm_write_4byte(dm, 0x1ba0, 0x6f6b6661);
	odm_write_4byte(dm, 0x1ba4, 0x817d7874);
	odm_write_4byte(dm, 0x1ba8, 0x908c8884);
	odm_write_4byte(dm, 0x1bac, 0x9d9a9793);
	odm_write_4byte(dm, 0x1bb0, 0xaaa7a4a1);
	odm_write_4byte(dm, 0x1bb4, 0xb6b3b0ad);
	// tx_inverse
	odm_write_4byte(dm, 0x1b40, 0x02ce03e9);
	odm_write_4byte(dm, 0x1b44, 0x01fd0249);
	odm_write_4byte(dm, 0x1b48, 0x01a101c9);
	odm_write_4byte(dm, 0x1b4c, 0x016a0181);
	odm_write_4byte(dm, 0x1b50, 0x01430155);
	odm_write_4byte(dm, 0x1b54, 0x01270135);
	odm_write_4byte(dm, 0x1b58, 0x0112011c);
	odm_write_4byte(dm, 0x1b5c, 0x01000108);
	odm_write_4byte(dm, 0x1b60, 0x00f100f8);
	odm_write_4byte(dm, 0x1b64, 0x00e500eb);
	odm_write_4byte(dm, 0x1b68, 0x00db00e0);
	odm_write_4byte(dm, 0x1b6c, 0x00d100d5);
	odm_write_4byte(dm, 0x1b70, 0x00c900cd);
	odm_write_4byte(dm, 0x1b74, 0x00c200c5);
	odm_write_4byte(dm, 0x1b78, 0x00bb00be);
	odm_write_4byte(dm, 0x1b7c, 0x00b500b8);

	// write pwsf
	//S3
	odm_write_4byte(dm, 0x1bdc, 0x40caffe1);
	odm_write_4byte(dm, 0x1bdc, 0x4080a1e3);
	odm_write_4byte(dm, 0x1bdc, 0x405165e5);
	odm_write_4byte(dm, 0x1bdc, 0x403340e7);
	odm_write_4byte(dm, 0x1bdc, 0x402028e9);
	odm_write_4byte(dm, 0x1bdc, 0x401419eb);
	odm_write_4byte(dm, 0x1bdc, 0x400d10ed);
	odm_write_4byte(dm, 0x1bdc, 0x40080aef);

	odm_write_4byte(dm, 0x1bdc, 0x400506f1);
	odm_write_4byte(dm, 0x1bdc, 0x400304f3);
	odm_write_4byte(dm, 0x1bdc, 0x400203f5);
	odm_write_4byte(dm, 0x1bdc, 0x400102f7);
	odm_write_4byte(dm, 0x1bdc, 0x400101f9);
	odm_write_4byte(dm, 0x1bdc, 0x400101fb);
	odm_write_4byte(dm, 0x1bdc, 0x400101fd);
	odm_write_4byte(dm, 0x1bdc, 0x400101ff);
	//S0
	odm_write_4byte(dm, 0x1bdc, 0x40caff81);
	odm_write_4byte(dm, 0x1bdc, 0x4080a183);
	odm_write_4byte(dm, 0x1bdc, 0x40516585);
	odm_write_4byte(dm, 0x1bdc, 0x40334087);
	odm_write_4byte(dm, 0x1bdc, 0x40202889);
	odm_write_4byte(dm, 0x1bdc, 0x4014198b);
	odm_write_4byte(dm, 0x1bdc, 0x400d108d);
	odm_write_4byte(dm, 0x1bdc, 0x40080a8f);

	odm_write_4byte(dm, 0x1bdc, 0x40050691);
	odm_write_4byte(dm, 0x1bdc, 0x40030493);
	odm_write_4byte(dm, 0x1bdc, 0x40020395);
	odm_write_4byte(dm, 0x1bdc, 0x40010297);
	odm_write_4byte(dm, 0x1bdc, 0x40010199);
	odm_write_4byte(dm, 0x1bdc, 0x4001019b);
	odm_write_4byte(dm, 0x1bdc, 0x4001019d);
	odm_write_4byte(dm, 0x1bdc, 0x4001019f);
	odm_write_4byte(dm, 0x1bdc, 0x00000000);
}

boolean
_dpk_enable_dpd_8195b(
	struct dm_struct *dm,
	u8 path,
	u8 best_tx_agc)
{
	boolean fail = true;
	u8 offset = 0x0;
	u32 IQK_CMD = 0x00001318;
	u8 external_pswf_gain;
	boolean gain_scaling_enable = false;

	odm_write_4byte(dm, 0x1b38, 0x20000000);
	odm_write_4byte(dm, 0x1b3c, 0x20000000);

	ODM_delay_ms(1);
	odm_write_4byte(dm, 0x1b00, IQK_CMD);
	odm_write_4byte(dm, 0x1b00, IQK_CMD + 1);
	fail = _dpk_check_nctl_done_8195b(dm, path, IQK_CMD);
	if (fail)
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK]In DPD Process(4), DPD process is Fail\n");
	odm_write_4byte(dm, 0x1b98, 0x4C104C10);
	odm_write_4byte(dm, 0x1b90, 0x0001e018);
	odm_write_4byte(dm, 0x1bd4, 0xA0001);


	if (!fail) {
		odm_write_1byte(dm, 0x1bcf, 0x19);
		odm_write_4byte(dm, 0x1bdc, 0x0);
		external_pswf_gain = 0x2;
		best_tx_agc = best_tx_agc + external_pswf_gain;

		if (best_tx_agc >= 0x19)
			offset = best_tx_agc - 0x19;
		else
			offset = 0x20 - (0x19 - best_tx_agc);
		odm_set_bb_reg(dm, 0x1bd0, BIT(12) | BIT(11) | BIT(10) | BIT(9) | BIT(8), offset);
		if (gain_scaling_enable == true)
			_dpk_set_gain_scaling_8195b(dm, RF_PATH_A);
		else
			odm_write_4byte(dm, 0x1b98, 0x4c004c00);

		odm_set_bb_reg(dm, 0x1b20, 0x0f000000, 0x5); //enable DPK
	}

	RF_DBG(dm, DBG_RF_DPK, "[DPK]_dpk_enable_dpd_8195b => 0x1bd0 = %x\n",
	       odm_read_4byte(dm, 0x1bd0));
	RF_DBG(dm, DBG_RF_DPK, "[DPK]_dpk_enable_dpd_8195b => 0x1b98 = %x\n",
	       odm_read_4byte(dm, 0x1b98));
	return fail;
}

void _phy_dpd_calibrate_8195b(
	struct dm_struct *dm,
	boolean reset)
{
	u32 MAC_backup[DPK_MAC_REG_NUM_8195B], BB_backup[DPK_BB_REG_NUM_8195B], RF_backup[DPK_RF_REG_NUM_8195B][1];
	u32 backup_mac_reg[DPK_MAC_REG_NUM_8195B] = {0x520, 0x550, 0x1518};
	u32 backup_bb_reg[DPK_BB_REG_NUM_8195B] = {0x808, 0x90c, 0xc00, 0xcb0, 0xcb4, 0xcbc,
											   0x1990, 0x9a4, 0xa04, 0xc58, 0xc5c, 0xe58,
											   0xe5c, 0xc6c, 0xe6c, 0x90c, 0xc94, 0xe94,
											   0x1904, 0xcb0, 0xcb4, 0xcbc, 0xc00,0x1b2c,
											   0x1b38, 0x1b3c};
	u32 backup_rf_reg[DPK_RF_REG_NUM_8195B] = {/*0x57, 0x63, 0x8c,*/ 0xdf, 0xde, 0x8f, 0x0, 0x1};
	boolean is_mp = false;
	u8 i;
	u8 best_tx_agc = 0x1c;
	boolean dpk_is_fail;



	struct dm_iqk_info *iqk_info = &dm->IQK_info;


	RF_DBG(dm, DBG_RF_DPK, "[DPK]==========DPK strat!!!!!==========\n");
	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK]band_type = %s, band_width = %d, ExtPA2G = %d, ext_pa_5g = %d\n",
	       (*dm->band_type == ODM_BAND_5G) ? "5G" : "2G", *dm->band_width,
	       dm->ext_pa, dm->ext_pa_5g);
	_dpk_backup_mac_bb_8195b(dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg, DPK_BB_REG_NUM_8195B);
	_dpk_backup_rf_8195b(dm, RF_backup, backup_rf_reg);

	/*PDK Init Register setting*/
	_dpk_configure_macbb_8195b(dm);
	_dpk_afe_setting_8195b(dm, true);
	_dpk_rfe_setting_8195b(dm, false);
	_dpk_init_8195b(dm);
	_dpk_clear_sram_8195b(dm, RF_PATH_A);
	_dpk_boundary_selection_8195b(dm, RF_PATH_A);
	_dpk_rfsetting_8195b(dm);
	/* Get the best TXAGC*/
	//Step 1
	best_tx_agc = _dpk_get_tx_agc_8195b(dm, RF_PATH_A);
	ODM_delay_ms(2);
	_dpk_PAS_GL_read_8195b(dm, RF_PATH_A);//PA SCAN

	//Step 2
	dpk_is_fail = _dpk_enable_dpk_8195b(dm, RF_PATH_A, best_tx_agc);
	ODM_delay_ms(2);
	_dpk_PAS_DPK_read_8195b(dm, RF_PATH_A);//PA SCAN

	//Step 3
	_dpk_enable_dpd_8195b(dm, RF_PATH_A, best_tx_agc);
	ODM_delay_ms(2);
	_dpk_readsram_8195b(dm, RF_PATH_A);

	_dpk_restore_mac_bb_8195b(dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg, DPK_BB_REG_NUM_8195B);
	_dpk_afe_setting_8195b(dm, false);
	_dpk_restore_rf_8195b(dm, backup_rf_reg, RF_backup);

	// back to normal
	odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
	odm_set_bb_reg(dm, 0x1b0c, BIT(13) | BIT(12), 0x0);
	odm_set_bb_reg(dm, 0x1bcc, 0x1f, 0x0);
	/* restore IQK */
	//_dpk_reload_iqk_setting_8195b(dm, 0, 2);

	/* enable dpk*/
	if (dpk_is_fail == FALSE)
		odm_set_bb_reg(dm, 0x1b20, BIT(25), 0x0);
	else
		odm_set_bb_reg(dm, 0x1b20, BIT(25), 0x1);

}

void phy_dp_calibrate_8195b(
	void *dm_void,
	boolean clear)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *p_rf = &(dm->rf_table);
#if 0
#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN))
	if (odm_check_power_status(dm) == false)
		return;
#endif

#if (MP_DRIVER)
	if ((dm->mp_mode != NULL) && (p_rf->is_con_tx != NULL) && (p_rf->is_single_tone != NULL) && (p_rf->is_carrier_suppresion != NULL))
		if (*(dm->mp_mode) && ((*(p_rf->is_con_tx) || *(p_rf->is_single_tone) || *(p_rf->is_carrier_suppresion))))
			return;
#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	if (!(p_rf->rf_supportability & HAL_RF_DPK))
		return;
#endif

#if DISABLE_BB_RF
	return;
#endif

	RF_DBG(dm, DBG_RF_DPK, "[DPK] In PHY, dm->dpk_en == %x\n",
	       p_rf->dpk_en);

	/*if dpk is not enable*/
	if (p_rf->dpk_en == 0x0)
		return;
#endif

	/*start*/
	if (!dm->rf_calibrate_info.is_iqk_in_progress) {
		odm_acquire_spin_lock(dm, RT_IQK_SPINLOCK);
		dm->rf_calibrate_info.is_iqk_in_progress = true;
		odm_release_spin_lock(dm, RT_IQK_SPINLOCK);
		if (*dm->mp_mode)
			dm->rf_calibrate_info.iqk_start_time = odm_get_current_time(dm);

		if (*dm->mp_mode) {
			_phy_dpd_calibrate_8195b(dm, clear);
			//_dpk_toggle_rxagc_8195b(dm, clear);
		}
		if (*dm->mp_mode) {
			dm->rf_calibrate_info.iqk_progressing_time = odm_get_progressing_time(dm, dm->rf_calibrate_info.iqk_start_time);
			RF_DBG(dm, DBG_RF_DPK,
			       "[DPK]DPK progressing_time = %lld ms\n",
			       dm->rf_calibrate_info.iqk_progressing_time);
		}
		odm_acquire_spin_lock(dm, RT_IQK_SPINLOCK);
		dm->rf_calibrate_info.is_iqk_in_progress = false;
		odm_release_spin_lock(dm, RT_IQK_SPINLOCK);
	}
}

void dpk_temperature_compensate_8195b(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	struct _hal_rf_ *p_rf = &(dm->rf_table);
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
	struct _ADAPTER *adapter = dm->adapter;
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	u8 pgthermal = hal_data->eeprom_thermal_meter;
#else
	struct rtl8192cd_priv *priv = dm->priv;
	u8 pgthermal = (u8)priv->pmib->dot11RFEntry.ther;
#endif
	static u8 dpk_tm_trigger = 0;
	u8 thermal_value = 0, delta_dpk, i = 0;
	u8 thermal_value_avg_count = 0;
	u8 thermal_value_avg_times = 2;
	u32 thermal_value_avg = 0;
	u8 tmp, abs_temperature;

	/*if dpk is not enable*/
	if (p_rf->dpk_en == 0x0)
		return;
	if (!dpk_tm_trigger) {
		odm_set_rf_reg(dm, RF_PATH_A, 0x42, BIT(17) | BIT(16), 0x03);
		dpk_tm_trigger = 1;
	} else {
		/* Initialize */
		dpk_tm_trigger = 0;
		/* calculate average thermal meter */
		thermal_value = (u8)odm_get_rf_reg(dm, RF_PATH_A, 0x42, 0xfc00); /*0x42: RF Reg[15:10] 88E*/
		RF_DBG(dm, DBG_RF_DPK, "[DPK] (3) current Thermal Meter = %d\n",
		       thermal_value);

		dm->rf_calibrate_info.thermal_value_dpk = thermal_value;
		dm->rf_calibrate_info.thermal_value_avg[dm->rf_calibrate_info.thermal_value_avg_index] = thermal_value;
		dm->rf_calibrate_info.thermal_value_avg_index++;
		if (dm->rf_calibrate_info.thermal_value_avg_index == thermal_value_avg_times)
			dm->rf_calibrate_info.thermal_value_avg_index = 0;
		for (i = 0; i < thermal_value_avg_times; i++) {
			if (dm->rf_calibrate_info.thermal_value_avg[i]) {
				thermal_value_avg += dm->rf_calibrate_info.thermal_value_avg[i];
				thermal_value_avg_count++;
			}
		}
		if (thermal_value_avg_count) /*Calculate Average thermal_value after average enough times*/
			thermal_value = (u8)(thermal_value_avg / thermal_value_avg_count);
		/* compensate the DPK */
		delta_dpk = (thermal_value > pgthermal) ? (thermal_value - pgthermal) : (pgthermal - thermal_value);
		tmp = (u8)((dpk_result[0] & 0x00001f00) >> 8);
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK] (5)delta_dpk = %d, eeprom_thermal_meter = %d, tmp=%d\n",
		       delta_dpk, pgthermal, tmp);

		if (thermal_value > pgthermal) {
			abs_temperature = thermal_value - pgthermal;
			if (abs_temperature >= 20)
				tmp = tmp + 4;
			else if (abs_temperature >= 15)
				tmp = tmp + 3;
			else if (abs_temperature >= 10)
				tmp = tmp + 2;
			else if (abs_temperature >= 5)
				tmp = tmp + 1;
		} else { /*low temperature*/
			abs_temperature = pgthermal - thermal_value;
			if (abs_temperature >= 20)
				tmp = tmp - 4;
			else if (abs_temperature >= 15)
				tmp = tmp - 3;
			else if (abs_temperature >= 10)
				tmp = tmp - 2;
			else if (abs_temperature >= 5)
				tmp = tmp - 1;
		}
		odm_set_bb_reg(dm, 0x1bd0, BIT(12) | BIT(11) | BIT(10) | BIT(9) | BIT(8), tmp);
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK] (6)delta_dpk = %d, eeprom_thermal_meter = %d, new tmp=%d, 0x1bd0=0x%x\n",
		       delta_dpk, pgthermal, tmp, odm_read_4byte(dm, 0x1bd0));
	}
#endif
}
#endif
