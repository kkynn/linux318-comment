/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/
#include "mp_precomp.h"
#include "../phydm_precomp.h"

#define PHYDM_FW_API_ENABLE_8198F 1
#define PHYDM_FW_API_FUNC_ENABLE_8198F 1

#if (RTL8198F_SUPPORT == 1)
#if (PHYDM_FW_API_ENABLE_8198F == 1)
/* ======================================================================== */
/* These following functions can be used for PHY DM only*/

enum channel_width bw_8198f;
u8 central_ch_8198f;
u8 central_ch_8198f_drp;

#if !(DM_ODM_SUPPORT_TYPE == ODM_CE)
u32 cca_ifem_bcut_98f[3][4] = {
	{0x75D97010, 0x75D97010, 0x75D97010, 0x75D97010}, /*Reg82C*/
	{0x79a0ea2a, 0x79a0ea2a, 0x79a0ea2a, 0x79a0ea2a}, /*Reg830*/
	{0x87766441, 0x87746341, 0x87765541, 0x87746341} /*Reg838*/
};
u32 cca_efem_bcut_98f[3][4] = {
	{0x75B76010, 0x75B76010, 0x75B76010, 0x75B75010}, /*Reg82C*/
	{0x79a0ea2a, 0x79a0ea2a, 0x79a0ea2a, 0x79a0ea2a}, /*Reg830*/
	{0x87766451, 0x87766431, 0x87766451, 0x87766431} /*Reg838*/
};
#endif

u32 cca_ifem_ccut_98f[3][4] = {
	{0x75C97010, 0x75C97010, 0x75C97010, 0x75C97010}, /*Reg82C*/
	{0x79a0eaaa, 0x79A0EAAC, 0x79a0eaaa, 0x79a0eaaa}, /*Reg830*/
	{0x87765541, 0x87746341, 0x87765541, 0x87746341} /*Reg838*/
};
u32 cca_efem_ccut_98f[3][4] = {
	{0x75B86010, 0x75B76010, 0x75B86010, 0x75B76010}, /*Reg82C*/
	{0x79A0EAA8, 0x79A0EAAC, 0x79A0EAA8, 0x79a0eaaa}, /*Reg830*/
	{0x87766451, 0x87766431, 0x87766451, 0x87766431} /*Reg838*/
};
u32 cca_ifem_ccut_rfetype_98f[3][4] = {
	{0x75da8010, 0x75da8010, 0x75da8010, 0x75da8010}, /*Reg82C*/
	{0x79a0eaaa, 0x97A0EAAC, 0x79a0eaaa, 0x79a0eaaa}, /*Reg830*/
	{0x87765541, 0x86666341, 0x87765561, 0x86666361} /*Reg838*/
};

__iram_odm_func__
void phydm_igi_toggle_8198f(
	struct dm_struct *dm)
{
	u32 igi = 0x20;

	igi = odm_get_bb_reg(dm, R_0x1d70, 0x7f);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f, (igi - 2)); /*path0*/
	odm_set_bb_reg(dm, R_0x1d70, 0x7f, igi);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f00, (igi - 2)); /*path1*/
	odm_set_bb_reg(dm, R_0x1d70, 0x7f00, igi);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f0000, (igi - 2)); /*path2*/
	odm_set_bb_reg(dm, R_0x1d70, 0x7f0000, igi);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f000000, (igi - 2)); /*path3*/
	odm_set_bb_reg(dm, R_0x1d70, 0x7f000000, igi);
}

__iram_odm_func__
void phydm_8198f_type15_rfe(
	struct dm_struct *dm,
	u8 channel)
{
	if (channel <= 14) {
		/* signal source */
		odm_set_bb_reg(dm, R_0xcb0, 0xffffff, 0x777777);
		odm_set_bb_reg(dm, R_0xeb0, 0xffffff, 0x777777);
		odm_set_bb_reg(dm, R_0xcb4, MASKBYTE1, 0x77);
		odm_set_bb_reg(dm, R_0xeb4, MASKBYTE1, 0x77);
	} else if ((channel > 35) && (channel <= 64)) {
		/* signal source */
		odm_set_bb_reg(dm, R_0xcb0, 0xffffff, 0x777747);
		odm_set_bb_reg(dm, R_0xeb0, 0xffffff, 0x777747);
		odm_set_bb_reg(dm, R_0xcb4, MASKBYTE0, 0x57);
		odm_set_bb_reg(dm, R_0xeb4, MASKBYTE0, 0x57);
	} else if (channel > 64) {
		/* signal source */
		odm_set_bb_reg(dm, R_0xcb0, 0xffffff, 0x777747);
		odm_set_bb_reg(dm, R_0xeb0, 0xffffff, 0x777747);
		odm_set_bb_reg(dm, R_0xcb4, MASKBYTE0, 0x75);
		odm_set_bb_reg(dm, R_0xeb4, MASKBYTE0, 0x75);
	} else
		return;

	/* inverse or not */
	odm_set_bb_reg(dm, R_0xcbc, 0x3f, 0x0);
	odm_set_bb_reg(dm, R_0xcbc, (BIT(11) | BIT(10) | BIT(9) | BIT(8)), 0x0);
	odm_set_bb_reg(dm, R_0xebc, 0x3f, 0x0);
	odm_set_bb_reg(dm, R_0xebc, (BIT(11) | BIT(10) | BIT(9) | BIT(8)), 0x0);

	/* antenna switch table */
	if (channel <= 14) {
		if (dm->rx_ant_status == BB_PATH_AB || dm->tx_ant_status == BB_PATH_AB) {
			/* 2TX or 2RX */
			odm_set_bb_reg(dm, R_0xca0, MASKLWORD, 0xa501);
			odm_set_bb_reg(dm, R_0xea0, MASKLWORD, 0xa501);
		} else if (dm->rx_ant_status == dm->tx_ant_status) {
			/* TXA+RXA or TXB+RXB */
			odm_set_bb_reg(dm, R_0xca0, MASKLWORD, 0xa500);
			odm_set_bb_reg(dm, R_0xea0, MASKLWORD, 0xa500);
		} else {
			/* TXB+RXA or TXA+RXB */
			odm_set_bb_reg(dm, R_0xca0, MASKLWORD, 0xa005);
			odm_set_bb_reg(dm, R_0xea0, MASKLWORD, 0xa005);
		}
	} else if (channel > 35) {
		odm_set_bb_reg(dm, R_0xca0, MASKLWORD, 0xa5a5);
		odm_set_bb_reg(dm, R_0xea0, MASKLWORD, 0xa5a5);
	}
}

__iram_odm_func__
u32 phydm_check_bit_mask_8198f(u32 bit_mask, u32 data_original, u32 data)
{
	u8 bit_shift;
	if (bit_mask != 0xfffff) {
		for (bit_shift = 0; bit_shift <= 19; bit_shift++) {
			if (((bit_mask >> bit_shift) & 0x1) == 1)
				break;
		}
		return ((data_original) & (~bit_mask)) | (data << bit_shift);
	}

	return data;
}

__iram_odm_func__
void phydm_rfe_8198f_setting(
	void *dm_void,
	u8 rfe_num,
	u8 path_mux_sel,
	u8 inv_en,
	u8 source_sel)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "8198F RFE[%d]:{Path=0x%x}{inv_en=%d}{source=0x%x}\n",
		  rfe_num, path_mux_sel, inv_en, source_sel);

	if (rfe_num > 19 || rfe_num < 0) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "[Warning] Wrong RFE num=%d}\n",
			  rfe_num);
		return;
	}

	/*[Path_mux_sel] */ /*ref_num 0~15, 0x1990->0x1c98;ref_num 16~19, 0x1990 -> 0x1c9c*/
	if (rfe_num < 16) {
		if (path_mux_sel == BB_PATH_A)
			odm_set_bb_reg(dm, R_0x1c98, BIT(rfe_num * 2 + 1) | BIT(rfe_num * 2), 0x0);
		else if (path_mux_sel == BB_PATH_B)
			odm_set_bb_reg(dm, R_0x1c98, BIT(rfe_num * 2 + 1) | BIT(rfe_num * 2), 0x1);
		else if (path_mux_sel == BB_PATH_C)
			odm_set_bb_reg(dm, R_0x1c98, BIT(rfe_num * 2 + 1) | BIT(rfe_num * 2), 0x2);
		else /*path D*/
			odm_set_bb_reg(dm, R_0x1c98, BIT(rfe_num * 2 + 1) | BIT(rfe_num * 2), 0x3);
	} else {
		if (path_mux_sel == BB_PATH_A)
			odm_set_bb_reg(dm, R_0x1c9c, BIT((rfe_num - 16) * 2 + 1) | BIT((rfe_num - 16) * 2), 0x0);
		else if (path_mux_sel == BB_PATH_B)
			odm_set_bb_reg(dm, R_0x1c9c, BIT((rfe_num - 16) * 2 + 1) | BIT((rfe_num - 16) * 2), 0x1);
		else if (path_mux_sel == BB_PATH_C)
			odm_set_bb_reg(dm, R_0x1c9c, BIT((rfe_num - 16) * 2 + 1) | BIT((rfe_num - 16) * 2), 0x2);
		else /*path D*/
			odm_set_bb_reg(dm, R_0x1c9c, BIT((rfe_num - 16) * 2 + 1) | BIT((rfe_num - 16) * 2), 0x3);
	}
	/*[Inv_en]*/
	odm_set_bb_reg(dm, R_0x183c, BIT(rfe_num), (u32)inv_en); /*0xcbc -> 0x183c*/
	odm_set_bb_reg(dm, R_0x413c, BIT(rfe_num), (u32)inv_en);
	odm_set_bb_reg(dm, R_0x523c, BIT(rfe_num), (u32)inv_en);
	odm_set_bb_reg(dm, R_0x533c, BIT(rfe_num), (u32)inv_en);

	/*[Output Source Signal Selection]*/
	if (rfe_num <= 7) {
		odm_set_bb_reg(dm, R_0x1840, ((0xf) << (rfe_num * 4)), (u32)source_sel); /*0xcb0 -> 0x1840*/
		odm_set_bb_reg(dm, R_0x4140, ((0xf) << (rfe_num * 4)), (u32)source_sel);
		odm_set_bb_reg(dm, R_0x5240, ((0xf) << (rfe_num * 4)), (u32)source_sel);
		odm_set_bb_reg(dm, R_0x5340, ((0xf) << (rfe_num * 4)), (u32)source_sel);
	} else if (rfe_num > 7 && rfe_num <= 15) {
		odm_set_bb_reg(dm, R_0x1844, ((0xf) << ((rfe_num - 8) * 4)), (u32)source_sel); /*0xcb4 -> 0x1844*/
		odm_set_bb_reg(dm, R_0x4144, ((0xf) << ((rfe_num - 8) * 4)), (u32)source_sel);
		odm_set_bb_reg(dm, R_0x5244, ((0xf) << ((rfe_num - 8) * 4)), (u32)source_sel);
		odm_set_bb_reg(dm, R_0x5344, ((0xf) << ((rfe_num - 8) * 4)), (u32)source_sel);
	} else {
		odm_set_bb_reg(dm, R_0x1848, ((0xf) << ((rfe_num - 16) * 4)), (u32)source_sel); /*0xcb4 -> 0x1848*/
		odm_set_bb_reg(dm, R_0x4148, ((0xf) << ((rfe_num - 16) * 4)), (u32)source_sel);
		odm_set_bb_reg(dm, R_0x5248, ((0xf) << ((rfe_num - 16) * 4)), (u32)source_sel);
		odm_set_bb_reg(dm, R_0x5348, ((0xf) << ((rfe_num - 16) * 4)), (u32)source_sel);
	}
}

__iram_odm_func__
void phydm_rfe_8198f_init(
	struct dm_struct *dm)
{
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "8198F RFE_Init, RFE_type=((%d))\n",
		  dm->rfe_type);

	/* chip top mux */
	/*odm_set_bb_reg(dm, R_0x64, BIT(29) | BIT(28), 0x3); BT control w/o in 98F */
	odm_set_bb_reg(dm, R_0x66, BIT(13) | BIT(12), 0x3);

	/* input or output */
	odm_set_bb_reg(dm, R_0x1c94, 0x3f, 0x32);

	/* from s0 ~ s3 */
	odm_set_bb_reg(dm, R_0x1ca0, MASKDWORD, 0x141);
}

__iram_odm_func__
boolean
phydm_rfe_8198f(
	struct dm_struct *dm,
	u8 channel)
{
	/* default rfe_type*/

	/*signal source*/
	odm_set_bb_reg(dm, R_0x1840, MASKDWORD, 0x77027770); /*path A*/
	odm_set_bb_reg(dm, R_0x4140, MASKDWORD, 0x02777707); /*path B*/
	odm_set_bb_reg(dm, R_0x5240, MASKDWORD, 0x77777702); /*path C*/
	odm_set_bb_reg(dm, R_0x5244, MASKDWORD, 0x02777707); /*path C*/
	odm_set_bb_reg(dm, R_0x5340, MASKDWORD, 0x77777702); /*path D*/
	odm_set_bb_reg(dm, R_0x5344, MASKDWORD, 0x02777707); /*path D*/

	/* path select setting*/
	odm_set_bb_reg(dm, R_0x1c98, MASKDWORD, 0x00fa50e4); /*all paths common setting*/

	return true;
}

__iram_odm_func__
void phydm_ccapar_by_rfe_8198f(
	struct dm_struct *dm)
{
	/*default*/


	/* CCA DC low threshold*/
	odm_set_bb_reg(dm, R_0x1900, 0xf, 0x4); /*RF20 pri20, Nstr 1*/
	odm_set_bb_reg(dm, R_0x1900, 0xf00, 0x4); /*RF40 pri20, Nstr 1*/
	odm_set_bb_reg(dm, R_0x1900, 0xf0000, 0x5); /*RF40 2nd20, Nstr 1*/
	odm_set_bb_reg(dm, R_0x4000, 0xf, 0x5); /*RF20 pri20, Nstr 2*/
	odm_set_bb_reg(dm, R_0x4000, 0xf00, 0x5); /*RF40 pri20, Nstr 2*/
	odm_set_bb_reg(dm, R_0x4000, 0xf0000, 0x5); /*RF40 2nd20, Nstr 2*/
	odm_set_bb_reg(dm, R_0x5000, 0xf, 0x3); /*RF20 pri20, Nstr 3*/
	odm_set_bb_reg(dm, R_0x5000, 0xf00, 0x3); /*RF40 pri20, Nstr 3*/
	odm_set_bb_reg(dm, R_0x5000, 0xf0000, 0x3); /*RF40 2nd20, Nstr 3*/
	odm_set_bb_reg(dm, R_0x5100, 0xf, 0x3); /*RF20 pri20, Nstr 4*/
	odm_set_bb_reg(dm, R_0x5100, 0xf00, 0x3); /*RF40 pri20, Nstr 4*/
	odm_set_bb_reg(dm, R_0x5100, 0xf0000, 0x2); /*RF40 2nd20, Nstr 4*/

	/*DC count max*/
	odm_set_bb_reg(dm, R_0x828, BIT(28) | BIT(27) | BIT(26), 0x4);
	/* CCA DC high threshold*/
	odm_set_bb_reg(dm, R_0x1900, 0xf0, 0x7); /*RF20 pri20, Nstr 1*/
	odm_set_bb_reg(dm, R_0x1900, 0xf000, 0x7); /*RF40 pri20, Nstr 1*/
	odm_set_bb_reg(dm, R_0x1900, 0xf00000, 0x8); /*RF40 2nd20, Nstr 1*/
	odm_set_bb_reg(dm, R_0x4000, 0xf0, 0x7); /*RF20 pri20, Nstr 2*/
	odm_set_bb_reg(dm, R_0x4000, 0xf000, 0x7); /*RF40 pri20, Nstr 2*/
	odm_set_bb_reg(dm, R_0x4000, 0xf00000, 0x8); /*RF40 2nd20, Nstr 2*/
	odm_set_bb_reg(dm, R_0x5000, 0xf0, 0x6); /*RF20 pri20, Nstr 3*/
	odm_set_bb_reg(dm, R_0x5000, 0xf000, 0x6); /*RF40 pri20, Nstr 3*/
	odm_set_bb_reg(dm, R_0x5000, 0xf00000, 0x6); /*RF40 2nd20, Nstr 3*/
	odm_set_bb_reg(dm, R_0x5100, 0xf0, 0x5); /*RF20 pri20, Nstr 4*/
	odm_set_bb_reg(dm, R_0x5100, 0xf000, 0x5); /*RF40 pri20, Nstr 4*/
	odm_set_bb_reg(dm, R_0x5100, 0xf00000, 0x4); /*RF40 2nd20, Nstr 4*/

	odm_set_bb_reg(dm, R_0x874, BIT(30), 0x1); /* BW decision opt*/
	odm_set_bb_reg(dm, R_0x884, BIT(16) | BIT(15) | BIT(14), 0x5);
	odm_set_bb_reg(dm, R_0x884, BIT(19) | BIT(18) | BIT(17), 0x3);

	/* CCA MF 20M threshold */
	odm_set_bb_reg(dm, R_0x1908, 0xf, 0x8); /*RF20 pri20,Nrx 1*/
	odm_set_bb_reg(dm, R_0x4008, 0xf, 0x8); /*RF20 pri20,Nrx 2*/
	odm_set_bb_reg(dm, R_0x5008, 0xf, 0x8); /*RF20 pri20,Nrx 3*/
	odm_set_bb_reg(dm, R_0x5108, 0xf, 0x8); /*RF20 pri20,Nrx 4*/

	/* CCA MF 40M threshold */
	odm_set_bb_reg(dm, R_0x1908, 0xf0, 0x8); /*RF40 pri20,Nrx 1*/
	odm_set_bb_reg(dm, R_0x4008, 0xf0, 0x8); /*RF40 pri20,Nrx 2*/
	odm_set_bb_reg(dm, R_0x5008, 0xf0, 0x8); /*RF40 pri20,Nrx 3*/
	odm_set_bb_reg(dm, R_0x5108, 0xf0, 0x8); /*RF40 pri20,Nrx 4*/

	odm_set_bb_reg(dm, R_0x1908, 0xf00, 0x8); /*RF40 2nd20,Nrx 1*/
	odm_set_bb_reg(dm, R_0x4008, 0xf00, 0x7); /*RF40 2nd20,Nrx 2*/
	odm_set_bb_reg(dm, R_0x5008, 0xf00, 0x7); /*RF40 2nd20,Nrx 3*/
	odm_set_bb_reg(dm, R_0x5108, 0xf00, 0x7); /*RF40 2nd20,Nrx 4*/

	/* force no CCA */
	odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x0); /*dis_pd_flag*/
	odm_set_bb_reg(dm, R_0x1c68, BIT(25), 0x0); /*dis_pd_flag 2nd20*/
	odm_set_bb_reg(dm, R_0x1c68, BIT(26), 0x0); /*dis_pd_flag 2nd40*/
	odm_set_bb_reg(dm, R_0x1c68, BIT(27), 0x0); /*dis_pd_flag 2nd80*/

	/* 20M SBD threshold */
	odm_set_bb_reg(dm, R_0x1918, 0x3f, 0x20); /*20M, Nrx 1*/
	odm_set_bb_reg(dm, R_0x4018, 0x3f, 0x22); /*20M, Nrx 2*/
	odm_set_bb_reg(dm, R_0x5018, 0x3f, 0x24); /*20M, Nrx 3*/
	odm_set_bb_reg(dm, R_0x5118, 0x3f, 0x24); /*20M, Nrx 4*/

	/* 40M SBD threshold */
	odm_set_bb_reg(dm, R_0x1918, 0xfc0, 0x20); /*20M, Nrx 1*/
	odm_set_bb_reg(dm, R_0x4018, 0xfc0, 0x22); /*20M, Nrx 2*/
	odm_set_bb_reg(dm, R_0x5018, 0xfc0, 0x24); /*20M, Nrx 3*/
	odm_set_bb_reg(dm, R_0x5118, 0xfc0, 0x22); /*20M, Nrx 4*/

	/* 20M CCD0 threshold */
	odm_set_bb_reg(dm, R_0x191c, 0x3f, 0x22); /*Nrx 1*/
	odm_set_bb_reg(dm, R_0x401c, 0x3f, 0x22); /*Nrx 2*/
	odm_set_bb_reg(dm, R_0x501c, 0x3f, 0x24); /*Nrx 3*/
	odm_set_bb_reg(dm, R_0x511c, 0x3f, 0x24); /*Nrx 4*/

	/* 40M CCD0 threshold */
	odm_set_bb_reg(dm, R_0x191c, 0xfc0, 0x22); /*Nrx 1*/
	odm_set_bb_reg(dm, R_0x401c, 0xfc0, 0x22); /*Nrx 2*/
	odm_set_bb_reg(dm, R_0x501c, 0xfc0, 0x24); /*Nrx 3*/
	odm_set_bb_reg(dm, R_0x511c, 0xfc0, 0x24); /*Nrx 4*/

	/*Big jump*/
	odm_set_bb_reg(dm, R_0x1928, BIT(20) | BIT(19) | BIT(18), 0x3); /*PathA*/
	odm_set_bb_reg(dm, R_0x4028, BIT(20) | BIT(19) | BIT(18), 0x3); /*PathB*/
	odm_set_bb_reg(dm, R_0x5028, BIT(20) | BIT(19) | BIT(18), 0x3); /*PathC*/
	odm_set_bb_reg(dm, R_0x5128, BIT(20) | BIT(19) | BIT(18), 0x3); /*PathD*/
}

__iram_odm_func__
void phydm_init_hw_info_by_rfe_type_8198f(
	struct dm_struct *dm)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8198F == 1)
	u16 mask_path_a = 0x0303;
	u16 mask_path_b = 0x0c0c;
	u16 mask_path_c = 0x3030;
	u16 mask_path_d = 0xc0c0;

	dm->is_init_hw_info_by_rfe = false;
	/* Default setting */

	dm->is_init_hw_info_by_rfe = true;

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "%s: RFE type (%d), Board type (0x%x), Package type (%d)\n",
		  __func__, dm->rfe_type, dm->board_type, dm->package_type);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "%s: 5G ePA (%d), 5G eLNA (%d), 2G ePA (%d), 2G eLNA (%d)\n",
		  __func__, dm->ext_pa_5g, dm->ext_lna_5g, dm->ext_pa,
		  dm->ext_lna);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "%s: 5G PA type (%d), 5G LNA type (%d), 2G PA type (%d), 2G LNA type (%d)\n",
		  __func__, dm->type_apa, dm->type_alna, dm->type_gpa,
		  dm->type_glna);

#endif /*PHYDM_FW_API_FUNC_ENABLE_8198F == 1*/
}

__iram_odm_func__
s32 phydm_get_condition_number_8198f(
	struct dm_struct *dm)
{
	s32 ret_val;

	/*odm_set_bb_reg(dm, R_0x1988, BIT(22), 0x1);*/
	/*ret_val = (s32)odm_get_bb_reg(dm, R_0xf84, (BIT(17) | BIT(16) | MASKLWORD));*/

	/*return ret_val;*/
}

/* ======================================================================== */

/* ======================================================================== */
/* These following functions can be used by driver*/

__iram_odm_func__
u32 config_phydm_read_rf_reg_8198f(
	struct dm_struct *dm,
	enum rf_path path,
	u32 reg_addr,
	u32 bit_mask)
{
	u32 readback_value, direct_addr;
	u32 offset_read_rf[4] = {0x3c00, 0x4c00, 0x5800, 0x5c00};

	/* Error handling.*/
	if (path > RF_PATH_D) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s: unsupported path (%d)\n",
			  __func__, path);
		return INVALID_RF_DATA;
	}

	/* Calculate offset */
	reg_addr &= 0xff;
	direct_addr = offset_read_rf[path] + (reg_addr << 2);

	/* RF register only has 20bits */
	bit_mask &= RFREGOFFSETMASK;

	/* Read RF register directly */
	readback_value = odm_get_bb_reg(dm, direct_addr, bit_mask);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "%s: RF-%d 0x%x = 0x%x, bit mask = 0x%x\n", __func__, path,
		  reg_addr, readback_value, bit_mask);
	return readback_value;
}

__iram_odm_func__
boolean
config_phydm_write_rf_reg_8198f(
	struct dm_struct *dm,
	enum rf_path path,
	u32 reg_addr,
	u32 bit_mask,
	u32 data)
{
	u32 data_and_addr = 0, data_original = 0;
	u32 offset_write_rf[4] = {0x1808, 0x4108, 0x5208, 0x5308};

	/* Error handling.*/
	if (path > RF_PATH_D) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_write_rf_reg_8198f(): unsupported path (%d)\n",
			  path);
		return false;
	}

	/* Read RF register content first */
	reg_addr &= 0xff;
	bit_mask = bit_mask & RFREGOFFSETMASK;

	if (bit_mask != RFREGOFFSETMASK) {
		data_original = config_phydm_read_rf_reg_8198f(dm, path, reg_addr, RFREGOFFSETMASK);

		/* Error handling. RF is disabled */
		if (config_phydm_read_rf_check_8198f(data_original) == false) {
			PHYDM_DBG(dm, ODM_PHY_CONFIG,
				  "config_phydm_write_rf_reg_8198f(): Write fail, RF is disable\n");
			return false;
		}

		/* check bit mask */
		data = phydm_check_bit_mask_8198f(bit_mask, data_original, data);
	}

	/* Put write addr in [27:20]  and write data in [19:00] */
	data_and_addr = ((reg_addr << 20) | (data & 0x000fffff)) & 0x0fffffff;

	/* Write operation */
	odm_set_bb_reg(dm, offset_write_rf[path], MASKDWORD, data_and_addr);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "config_phydm_write_rf_reg_8198f(): RF-%d 0x%x = 0x%x (original: 0x%x), bit mask = 0x%x\n",
		  path, reg_addr, data, data_original, bit_mask);
	return true;
}

__iram_odm_func__
boolean
config_phydm_write_txagc_ref_8198f(
	struct dm_struct *dm,
	u32 power_index,
	enum rf_path path,
	enum PDM_RATE_TYPE rate_type)
{
	/* 4-path power reference */
	u32 txagc_ofdm_ref[4] = {0x18e8, 0x41e8, 0x52e8, 0x53e8};
	u32 txagc_cck_ref[4] = {0x18a0, 0x41a0, 0x52a0, 0x53a0};

	/* bbrstb TX AGC report - default disable */
	/* Enable for writing the TX AGC table when bb_reset=0 */
	odm_set_bb_reg(dm, R_0x1c90, BIT(15), 0x0);

	/* According the rate to write in the ofdm or the cck */
	if (rate_type == 0) /* CCK reference setting */
		odm_set_bb_reg(dm, txagc_cck_ref[path], 0x007F0000, power_index);
	else if (rate_type == 1) /* OFDM reference setting */
		odm_set_bb_reg(dm, txagc_ofdm_ref[path], 0x0001FC00, power_index);

	/* Input need to be HW rate index, not driver rate index!!!! */
	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_write_txagc_8198f(): disable PHY API for debug!!\n");
		return true;
	}

	/* Error handling */
	if (path > RF_PATH_D) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_write_txagc_8198f(): unsupported path (%d)\n",
			  path);
		return false;
	}

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "config_phydm_write_txagc_8198f(): path-%d rate type %d (0x%x) = 0x%x\n",
		  path, rate_type, txagc_ofdm_ref[path], power_index);
	return true;
}

__iram_odm_func__
boolean
config_phydm_write_txagc_diff_8198f(
	struct dm_struct *dm,
	u32 power_index,
	enum rf_path path,
	u8 hw_rate)
{
	u32 offset_txagc = 0x3a00;
	u8 rate_idx = (hw_rate & 0xfc); /* Extract the 0xfc */

	/* bbrstb TX AGC report - default disable */
	/* Enable for writing the TX AGC table when bb_reset=0 */
	odm_set_bb_reg(dm, R_0x1c90, BIT(15), 0x0);

	/* Input need to be HW rate index, not driver rate index!!!! */
	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_write_txagc_diff_8198f(): disable PHY API for debug!!\n");
		return true;
	}

	/* Error handling */
	if (path > RF_PATH_D || hw_rate > 0x53) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_write_txagc_diff_8198f(): unsupported path (%d)\n",
			  path);
		return false;
	}

	/* According the rate to write in the ofdm or the cck */
	/* driver need to construct a 4-byte power index */
	odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKDWORD, power_index);

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "config_phydm_write_txagc_diff_8198f(): path-%d rate index 0x%x (0x%x) = 0x%x\n",
		  path, hw_rate, (offset_txagc + hw_rate), power_index);
	return true;
}

__iram_odm_func__
s8 config_phydm_read_txagc_diff_8198f(
	struct dm_struct *dm,
	enum rf_path path,
	u8 hw_rate)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8198F == 1)
	s8 read_back_data;

	/* Input need to be HW rate index, not driver rate index!!!! */

	/* Error handling */
	if (path > RF_PATH_D || hw_rate > 0x53) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s: unsupported path (%d)\n",
			  __func__, path);
		return INVALID_TXAGC_DATA;
	}

	/* Disable TX AGC report */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x0); /* need to check */

	/* Set data rate index (bit0~6) and path index (bit7) */
	odm_set_bb_reg(dm, R_0x1c7c, 0x7F000000, (hw_rate | (path << 7)));

	/* Enable TXAGC report */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x1);

	/* Read TX AGC report */
	read_back_data = (u8)odm_get_bb_reg(dm, R_0x2de8, 0xff);

	/* Driver have to disable TXAGC report after reading TXAGC (ref. user guide v11) */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x0);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s: path-%d rate index 0x%x = 0x%x\n",
		  __func__, path, hw_rate, read_back_data);
	return read_back_data;
#else
	return 0;
#endif
}

__iram_odm_func__
u8 config_phydm_read_txagc_8198f(
	struct dm_struct *dm,
	enum rf_path path,
	u8 hw_rate,
	enum PDM_RATE_TYPE rate_type)
{
#if (1) /*PHYDM_FW_API_FUNC_ENABLE_8198F == 1)*/
	s8 read_back_data;
	u8 read_ref_data;
	u8 result_data;
	/* 4-path power reference */
	u32 txagc_ofdm_ref[4] = {0x18e8, 0x41e8, 0x52e8, 0x53e8};
	u32 txagc_cck_ref[4] = {0x18a0, 0x41a0, 0x52a0, 0x53a0};

	/* Input need to be HW rate index, not driver rate index!!!! */

	/* Error handling */
	if (path > RF_PATH_D || hw_rate > 0x53) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s: unsupported path (%d)\n",
			  __func__, path);
		return INVALID_TXAGC_DATA;
	}

	/* Disable TX AGC report */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x0); /* need to check */

	/* Set data rate index (bit0~6) and path index (bit7) */
	odm_set_bb_reg(dm, R_0x1c7c, 0x7F000000, (hw_rate | (path << 7)));

	/* Enable TXAGC report */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x1);

	/* Read power difference report */
	read_back_data = (u8)odm_get_bb_reg(dm, R_0x2de8, 0x000000ff);
	/* Read power reference value report */
	if (rate_type == 0) /* Bit=22:16 */
		read_ref_data = (u8)odm_get_bb_reg(dm, txagc_cck_ref[path], 0x007F0000);
	else /* Bit=16:10 */
		read_ref_data = (u8)odm_get_bb_reg(dm, txagc_ofdm_ref[path], 0x0001FC00);
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "read_txagc_8198f(): diff=%d ref=%d\n",
		  read_back_data, read_ref_data);
	result_data = read_back_data + read_ref_data;
	/* Driver have to disable TXAGC report after reading TXAGC (ref. user guide v11) */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x0);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s: path-%d rate index 0x%x = 0x%x\n",
		  __func__, path, hw_rate, read_back_data);
	return result_data;
#else
	return 0;
#endif
}

__iram_odm_func__
void phydm_dynamic_spur_det_eliminate_8198f(
	struct dm_struct *dm)
{
#if 0

	u32		freq_2g[FREQ_PT_2G_NUM] = {0xFC67, 0xFC27, 0xFFE6, 0xFFA6, 0xFC67, 0xFCE7, 0xFCA7, 0xFC67, 0xFC27, 0xFFE6, 0xFFA6, 0xFF66, 0xFF26, 0xFCE7};
	u32		freq_5g[FREQ_PT_5G_NUM] = {0xFFC0, 0xFFC0, 0xFC81, 0xFC81, 0xFC41, 0xFC40, 0xFF80, 0xFF80, 0xFF40, 0xFD42};
	u32		freq_2g_n1[FREQ_PT_2G_NUM] = {0}, freq_2g_p1[FREQ_PT_2G_NUM] = {0};
	u32		freq_5g_n1[FREQ_PT_5G_NUM] = {0}, freq_5g_p1[FREQ_PT_5G_NUM] = {0};
	u32		freq_pt_2g_final = 0, freq_pt_5g_final = 0, freq_pt_2g_b_final = 0, freq_pt_5g_b_final = 0;
	u32		max_ret_psd_final = 0, max_ret_psd_b_final = 0;
	u32		max_ret_psd_2nd[PSD_SMP_NUM] = {0}, max_ret_psd_b_2nd[PSD_SMP_NUM] = {0};
	u32		psd_set[PSD_VAL_NUM] = {0}, psd_set_B[PSD_VAL_NUM] = {0};
	u32		rank_psd_index_in[PSD_VAL_NUM] = {0}, rank_sample_index_in[PSD_SMP_NUM] = {0};
	u32		rank_psd_index_out[PSD_VAL_NUM] = {0};
	u32		rank_sample_index_out[PSD_SMP_NUM] = {0};
	u32		reg_910_15_12 = 0;
	u8		j = 0, k = 0, threshold_nbi = 0x8D, threshold_csi = 0x8D;
	u8		idx = 0, set_result_nbi = PHYDM_SET_NO_NEED, set_result_csi = PHYDM_SET_NO_NEED;
	boolean	s_dopsd = false, s_donbi_a = false, s_docsi = false, s_donbi_b = false;

	/* Reset NBI/CSI everytime after changing channel/BW/band  */
	odm_set_bb_reg(dm, R_0x880, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x884, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x888, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x88c, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x890, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x894, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x898, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x89c, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x874, BIT(0), 0x0);

	odm_set_bb_reg(dm, R_0x87c, BIT(13), 0x0);
	odm_set_bb_reg(dm, R_0xc20, BIT(28), 0x0);
	odm_set_bb_reg(dm, R_0xe20, BIT(28), 0x0);

	/* 2G Channel Setting > 20M: 5, 6, 7, 8, 13; 40M: 3~11 */
	if ((*dm->channel >= 1) && (*dm->channel <= 14)) {
		if (*dm->band_width == CHANNEL_WIDTH_20) {
			if (*dm->channel >= 5 && *dm->channel <= 8)
				idx = *dm->channel - 5;
			else if (*dm->channel == 13)
				idx = 4;
			else
				idx = 16;
		} else {
			if (*dm->channel >= 3 && *dm->channel <= 11)
				idx = *dm->channel + 2;
			else
				idx = 16;
		}
	} else { /* 5G Channel Setting > 20M: 153, 161; 40M: 54, 118, 151, 159; 80M: 58, 122, 155, 155 */
		switch (*dm->channel) {
		case 153:
			idx = 0;
			break;
		case 161:
			idx = 1;
			break;
		case 54:
			idx = 2;
			break;
		case 118:
			idx = 3;
			break;
		case 151:
			idx = 4;
			break;
		case 159:
			idx = 5;
			break;
		case 58:
			idx = 6;
			break;
		case 122:
			idx = 7;
			break;
		case 155:
			idx = 8;
			break;
		default:
			idx = 16;
			break;
		}
	}

	if (idx <= 16) {
		s_dopsd = true;
	} else {
		PHYDM_DBG(dm, ODM_COMP_API,
			  "[Return Point] Idx Is Exceed, Not Support Dynamic Spur Detection and Eliminator\n");
		return;
	}

	PHYDM_DBG(dm, ODM_COMP_API, "[%s] idx = %d, BW = %d, Channel = %d\n",
		  __func__, idx, *dm->band_width, *dm->channel);

	for (k = 0; k < FREQ_PT_2G_NUM; k++) {
		freq_2g_n1[k] = freq_2g[k] - 1;
		freq_2g_p1[k] = freq_2g[k] + 1;
	}

	for (k = 0; k < FREQ_PT_5G_NUM; k++) {
		freq_5g_n1[k] = freq_5g[k] - 1;
		freq_5g_p1[k] = freq_5g[k] + 1;
	}

	if (!s_dopsd || idx > 13) {
		PHYDM_DBG(dm, ODM_COMP_API,
			  "[Return Point] s_dopsd is flase, Not Support Dynamic Spur Detection and Eliminator\n");
		return;
	}

	for (k = 0; k < PSD_SMP_NUM; k++) {
		if (k == 0) {
			freq_pt_2g_final = freq_2g_n1[idx];
			freq_pt_2g_b_final = freq_2g_n1[idx] | BIT(16);
			if (idx <= 10) {
				freq_pt_5g_final = freq_5g_n1[idx];
				freq_pt_5g_b_final = freq_5g_n1[idx] | BIT(16);
			}
		} else if (k == 1) {
			freq_pt_2g_final = freq_2g[idx];
			freq_pt_2g_b_final = freq_2g[idx] | BIT(16);
			if (idx <= 10) {
				freq_pt_5g_final = freq_5g[idx];
				freq_pt_5g_b_final = freq_5g[idx] | BIT(16);
			}
		} else if (k == 2) {
			freq_pt_2g_final = freq_2g_p1[idx];
			freq_pt_2g_b_final = freq_2g_p1[idx] | BIT(16);
			if (idx <= 10) {
				freq_pt_5g_final = freq_5g_p1[idx];
				freq_pt_5g_b_final = freq_5g_p1[idx] | BIT(16);
			}
		}

		for (j = 0; j < PSD_VAL_NUM; j++) {
			odm_set_bb_reg(dm, R_0xc00, MASKBYTE0, 0x4);/* disable 3-wire, path-A */
			odm_set_bb_reg(dm, R_0xe00, MASKBYTE0, 0x4);/* disable 3-wire, path-B */
			reg_910_15_12 = odm_get_bb_reg(dm, R_0x910, (BIT(15) | BIT(14) | BIT(13) | BIT(12)));

			if (dm->rx_ant_status & BB_PATH_A) {
				odm_set_bb_reg(dm, R_0x808, MASKBYTE0, (((BB_PATH_A) << 4) | BB_PATH_A));/*path-A*/

				if ((*dm->channel >= 1) && (*dm->channel <= 14))
					odm_set_bb_reg(dm, R_0x910, MASKDWORD, BIT(22) | freq_pt_2g_final);/* Start PSD */
				else
					odm_set_bb_reg(dm, R_0x910, MASKDWORD, BIT(22) | freq_pt_5g_final);/* Start PSD */

				ODM_delay_us(500);

				psd_set[j] = odm_get_bb_reg(dm, R_0xf44, MASKLWORD);

				odm_set_bb_reg(dm, R_0x910, BIT(22), 0x0);/* turn off PSD */
			}

			if (dm->rx_ant_status & BB_PATH_B) {
				odm_set_bb_reg(dm, R_0x808, MASKBYTE0, (((BB_PATH_B) << 4) | BB_PATH_B));/*path-B*/

				if ((*dm->channel > 0) && (*dm->channel <= 14))
					odm_set_bb_reg(dm, R_0x910, MASKDWORD, BIT(22) | freq_pt_2g_b_final);/* Start PSD */
				else
					odm_set_bb_reg(dm, R_0x910, MASKDWORD, BIT(22) | freq_pt_5g_b_final);/* Start PSD */

				ODM_delay_us(500);

				psd_set_B[j] = odm_get_bb_reg(dm, R_0xf44, MASKLWORD);

				odm_set_bb_reg(dm, R_0x910, BIT(22), 0x0);/* turn off PSD */
			}

			odm_set_bb_reg(dm, R_0xc00, MASKBYTE0, 0x7);/*eanble 3-wire*/
			odm_set_bb_reg(dm, R_0xe00, MASKBYTE0, 0x7);
			odm_set_bb_reg(dm, R_0x910, (BIT(15) | BIT(14) | BIT(13) | BIT(12)), reg_910_15_12);

			odm_set_bb_reg(dm, R_0x808, MASKBYTE0, (((dm->rx_ant_status) << 4) | dm->rx_ant_status));

			/* Toggle IGI to let RF enter RX mode, because BB doesn't send 3-wire command when RX path is enable */
			phydm_igi_toggle_8198f(dm);
		}
		if (dm->rx_ant_status & BB_PATH_A) {
			phydm_seq_sorting(dm, psd_set, rank_psd_index_in, rank_psd_index_out, PSD_VAL_NUM);
			max_ret_psd_2nd[k] = psd_set[0];
		}
		if (dm->rx_ant_status & BB_PATH_B) {
			phydm_seq_sorting(dm, psd_set_B, rank_psd_index_in, rank_psd_index_out, PSD_VAL_NUM);
			max_ret_psd_b_2nd[k] = psd_set_B[0];
		}
	}

	if (dm->rx_ant_status & BB_PATH_A) {
		phydm_seq_sorting(dm, max_ret_psd_2nd, rank_sample_index_in, rank_sample_index_out, PSD_SMP_NUM);
		max_ret_psd_final = max_ret_psd_2nd[0];

		if (max_ret_psd_final >= threshold_nbi)
			s_donbi_a = true;
		else
			s_donbi_a = false;
	}
	if (dm->rx_ant_status & BB_PATH_B) {
		phydm_seq_sorting(dm, max_ret_psd_b_2nd, rank_sample_index_in, rank_sample_index_out, PSD_SMP_NUM);
		max_ret_psd_b_final = max_ret_psd_b_2nd[0];

		if (max_ret_psd_b_final >= threshold_nbi)
			s_donbi_b = true;
		else
			s_donbi_b = false;
	}

	PHYDM_DBG(dm, ODM_COMP_API,
		  "[%s] max_ret_psd_final = %d, max_ret_psd_b_final = %d\n",
		  __func__, max_ret_psd_final, max_ret_psd_b_final);

	if (max_ret_psd_final >= threshold_csi || max_ret_psd_b_final >= threshold_csi)
		s_docsi = true;
	else
		s_docsi = false;


	/* Reset NBI/CSI everytime after changing channel/BW/band  */
	odm_set_bb_reg(dm, R_0x880, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x884, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x888, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x88c, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x890, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x894, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x898, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x89c, MASKDWORD, 0);
	odm_set_bb_reg(dm, R_0x874, BIT(0), 0x0);

	odm_set_bb_reg(dm, R_0x87c, BIT(13), 0x0);
	odm_set_bb_reg(dm, R_0xc20, BIT(28), 0x0);
	odm_set_bb_reg(dm, R_0xe20, BIT(28), 0x0);

	if (s_donbi_a == true || s_donbi_b == true) {
		if (*dm->band_width == CHANNEL_WIDTH_20) {
			if (*dm->channel == 153)
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 20, 5760, PHYDM_DONT_CARE);
			else if (*dm->channel == 161)
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 20, 5800, PHYDM_DONT_CARE);
			else if (*dm->channel >= 5 && *dm->channel <= 8)
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 20, 2440, PHYDM_DONT_CARE);
			else if (*dm->channel == 13)
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 20, 2480, PHYDM_DONT_CARE);
			else
				set_result_nbi = PHYDM_SET_NO_NEED;
		} else if (*dm->band_width == CHANNEL_WIDTH_40) {
			if (*dm->channel == 54) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 40, 5280, PHYDM_DONT_CARE);
			} else if (*dm->channel == 118) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 40, 5600, PHYDM_DONT_CARE);
			} else if (*dm->channel == 151) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 40, 5760, PHYDM_DONT_CARE);
			} else if (*dm->channel == 159) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 40, 5800, PHYDM_DONT_CARE);
				/* 2.4G */
			} else if ((*dm->channel >= 4) && (*dm->channel <= 6)) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 40, 2440, PHYDM_DONT_CARE);
			} else if (*dm->channel == 11) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 40, 2480, PHYDM_DONT_CARE);
			} else
				set_result_nbi = PHYDM_SET_NO_NEED;
		} else if (*dm->band_width == CHANNEL_WIDTH_80) {
			if (*dm->channel == 58) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 80, 5280, PHYDM_DONT_CARE);
			} else if (*dm->channel == 122) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 80, 5600, PHYDM_DONT_CARE);
			} else if (*dm->channel == 155) {
				set_result_nbi = phydm_nbi_setting(dm, FUNC_ENABLE, *dm->channel, 80, 5760, PHYDM_DONT_CARE);
			} else
				set_result_nbi = PHYDM_SET_NO_NEED;
		} else
			set_result_nbi = PHYDM_SET_NO_NEED;
	}

	if (s_docsi == true) {
		if (*dm->band_width == CHANNEL_WIDTH_20) {
			if (*dm->channel == 153)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 20, 5760, PHYDM_DONT_CARE);
			else if (*dm->channel == 161)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 20, 5800, PHYDM_DONT_CARE);
			else if (*dm->channel >= 5 && *dm->channel <= 8)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 20, 2440, PHYDM_DONT_CARE);
			else if (*dm->channel == 13)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 20, 2480, PHYDM_DONT_CARE);
			else
				set_result_csi = PHYDM_SET_NO_NEED;
		} else if (*dm->band_width == CHANNEL_WIDTH_40) {
			if (*dm->channel == 54)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 40, 5280, PHYDM_DONT_CARE);
			else if (*dm->channel == 118)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 40, 5600, PHYDM_DONT_CARE);
			else if (*dm->channel == 151)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 40, 5760, PHYDM_DONT_CARE);
			else if (*dm->channel == 159)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 40, 5800, PHYDM_DONT_CARE);
			else if ((*dm->channel >= 3) && (*dm->channel <= 10))
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 40, 2440, PHYDM_DONT_CARE);
			else if (*dm->channel == 11)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 40, 2480, PHYDM_DONT_CARE);
			else
				set_result_csi = PHYDM_SET_NO_NEED;
		} else if (*dm->band_width == CHANNEL_WIDTH_80) {
			if (*dm->channel == 58)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 80, 5280, PHYDM_DONT_CARE);
			else if (*dm->channel == 122)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 80, 5600, PHYDM_DONT_CARE);
			else if (*dm->channel == 155)
				set_result_csi = phydm_csi_mask_setting(dm, FUNC_ENABLE, *dm->channel, 80, 5760, PHYDM_DONT_CARE);
			else
				set_result_csi = PHYDM_SET_NO_NEED;
		} else
			set_result_csi = PHYDM_SET_NO_NEED;
	}
#endif
}

__iram_odm_func__
boolean
config_phydm_switch_channel_8198f(
	struct dm_struct *dm,
	u8 central_ch)
{
	struct phydm_dig_struct *dig_tab = &dm->dm_dig_table;
	u32 rf_reg18 = 0, rf_reg_bc = 0xff;
	boolean rf_reg_status = true;
	u8 low_band[15] = {0x7, 0x6, 0x6, 0x5, 0x0, 0x0, 0x7, 0xff, 0x6, 0x5, 0x0, 0x0, 0x7, 0x6, 0x6};
	u8 middle_band[23] = {0x6, 0x5, 0x0, 0x0, 0x7, 0x6, 0x6, 0xff, 0x0, 0x0, 0x7, 0x6, 0x6, 0x5, 0x0, 0xff, 0x7, 0x6, 0x6, 0x5, 0x0, 0x0, 0x7};
	u8 high_band[15] = {0x5, 0x5, 0x0, 0x7, 0x7, 0x6, 0x5, 0xff, 0x0, 0x7, 0x7, 0x6, 0x5, 0x5, 0x0};
	u8 band_index = 0;

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "config_phydm_switch_channel_8198f()====================>\n");

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_channel_8198f(): disable PHY API for debug!!\n");
		return true;
	}

	central_ch_8198f = central_ch;

	/* Errir handling for wrong HW setting due to wrong channel setting */
	if (central_ch_8198f <= 14)
		band_index = 1;
	else
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_channel_8198f(): unsupported band (5G) (%d)\n",
			  central_ch_8198f);

	if (dm->rfe_hwsetting_band != band_index)
		phydm_rfe_8198f(dm, central_ch_8198f);

	/*if (dm->rfe_type == 15)
		phydm_rfe_8198f(dm, central_ch_8198f);*/

	/* RF register setting */
	rf_reg18 = config_phydm_read_rf_reg_8198f(dm, RF_PATH_A, 0x18, RFREGOFFSETMASK);
	rf_reg_status = rf_reg_status & config_phydm_read_rf_check_8198f(rf_reg18);
	rf_reg18 = (rf_reg18 & (~(0xf))); /* 98F only 2.4G -> ch-1~14, 3bit */

	/* Switch band and channel */
	if (central_ch <= 14) {
		/* 2.4G */

		/* 1. RF band and channel*/
		rf_reg18 = (rf_reg18 | central_ch);

		/* 2. AGC table selection */
		odm_set_bb_reg(dm, R_0x18ac, 0x1f0, 0x0);
		dig_tab->agc_table_idx = 0x0;

		/* 3. Set central frequency for clock offset tracking */
		if (central_ch == 13 || central_ch == 14)
			odm_set_bb_reg(dm, R_0xc30, MASK12BITS, 0x969); /*n:41 s:37*/
		else
			odm_set_bb_reg(dm, R_0xc30, MASK12BITS, 0x9aa); /*n:42 s:38*/

		/* CCK TX filter parameters */

		if (central_ch == 14) {
			odm_set_bb_reg(dm, R_0x1a24, MASKDWORD, 0x0000b81c);
			odm_set_bb_reg(dm, R_0x1a28, MASKLWORD, 0x0000);
			odm_set_bb_reg(dm, R_0x1aac, MASKDWORD, 0x00003667);
		} else {
			odm_set_bb_reg(dm, R_0x1a24, MASKDWORD, 0x64b80c1c);
			odm_set_bb_reg(dm, R_0x1a28, MASKLWORD, (0x00008810 & MASKLWORD));
			odm_set_bb_reg(dm, R_0x1aac, MASKDWORD, 0x01235667);
		}

	} else {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_channel_8198f(): Fail to switch channel (ch: %d)\n",
			  central_ch);
		return false;
	}

	/* Modify IGI for MP driver to aviod PCIE interference */
	/*if (*dm->mp_mode && ((dm->rfe_type == 3) || (dm->rfe_type == 5))) {
		if (central_ch == 14)
			odm_write_dig(dm, 0x26);
		else if (central_ch < 14)
			odm_write_dig(dm, 0x20);
		else
			PHYDM_DBG(dm, ODM_PHY_CONFIG, "config_phydm_switch_channel_8198f(): Fail to switch channel (ch: %d)\n", central_ch);
	}*/

	/* Modify the setting of register 0xBC to reduce phase noise */
	if (central_ch <= 14)
		rf_reg_bc = 0x0;
	else
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_channel_8198f(): Fail to switch channel (ch: %d)\n",
			  central_ch);

	if (rf_reg_bc != 0xff)
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xbc, (BIT(17) | BIT(16) | BIT(15)), rf_reg_bc);
	else {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_channel_8198f(): Fail to switch channel (ch: %d, Phase noise)\n",
			  central_ch);
		return false;
	}

	odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREGOFFSETMASK, rf_reg18);

	if (dm->rf_type > RF_1T1R) {
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREGOFFSETMASK, rf_reg18);
		odm_set_rf_reg(dm, RF_PATH_C, RF_0x18, RFREGOFFSETMASK, rf_reg18);
		odm_set_rf_reg(dm, RF_PATH_D, RF_0x18, RFREGOFFSETMASK, rf_reg18);
	}
	if (rf_reg_status == false) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_channel_8198f(): Fail to switch channel (ch: %d), because writing RF register is fail\n",
			  central_ch);
		return false;
	}

	/* Debug for RF resister reading error during synthesizer parameters parsing */
	/*odm_set_rf_reg(dm, RF_PATH_A, RF_0xb0, BIT(10), 0);*/
	/*odm_set_rf_reg(dm, RF_PATH_A, RF_0xb0, BIT(10), 1);*/

	phydm_igi_toggle_8198f(dm);
	/* Dynamic spur detection by PSD and NBI/CSI mask */
	if (*dm->mp_mode)
		phydm_dynamic_spur_det_eliminate_8198f(dm);

	phydm_ccapar_by_rfe_8198f(dm);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "config_phydm_switch_channel_8198f(): Success to switch channel (ch: %d)\n",
		  central_ch);
	return true;
}

__iram_odm_func__
boolean
config_phydm_switch_bandwidth_8198f(
	struct dm_struct *dm,
	u8 primary_ch_idx,
	enum channel_width bandwidth)
{
	u32 rf_reg18, val32;
	boolean rf_reg_status = true;
	u8 rfe_type = dm->rfe_type;

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "config_phydm_switch_bandwidth_8198f()===================>\n");

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_bandwidth_8198f(): disable PHY API for debug!!\n");
		return true;
	}

	/* Error handling */
	if (bandwidth >= CHANNEL_WIDTH_MAX || (bandwidth == CHANNEL_WIDTH_40 && primary_ch_idx > 2) ||
	    (bandwidth == CHANNEL_WIDTH_80 && primary_ch_idx > 4)) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_bandwidth_8198f(): Fail to switch bandwidth (bw: %d, primary ch: %d)\n",
			  bandwidth, primary_ch_idx);
		return false;
	}

	bw_8198f = bandwidth;
	rf_reg18 = config_phydm_read_rf_reg_8198f(dm, RF_PATH_A, 0x18, RFREGOFFSETMASK);
	rf_reg_status = rf_reg_status & config_phydm_read_rf_check_8198f(rf_reg18);

	/* Switch bandwidth */
	switch (bandwidth) {
	case CHANNEL_WIDTH_20: {
		/*val32 = odm_get_bb_reg(dm, R_0x8ac, MASKDWORD);*/
		/*val32 &= 0xFFCFFC00;*/
		/*val32 |= (CHANNEL_WIDTH_20);*/
		/*odm_set_bb_reg(dm, R_0x8ac, MASKDWORD, val32);*/
		odm_set_bb_reg(dm, R_0x9b0, 0x3, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0xc, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0); /*small BW*/
		odm_set_bb_reg(dm, R_0x9b0, 0xf00, primary_ch_idx); /*TX pri ch*/
		odm_set_bb_reg(dm, R_0x9b0, 0xf000, primary_ch_idx); /*RX pri ch*/
		/* DAC clock = 160M clock for BW20 */
		/* ADC clock = 160M clock for BW20 */
		odm_set_bb_reg(dm, R_0x9b4, MASKH3BYTES, 0x9246db);

		/* Gain setting */
		/* !!The value will depend on the contents of AGC table!! */
		/* AGC table change ==> parameter must be changed*/
		odm_set_bb_reg(dm, R_0x82c, 0x3f, 0x19);
		/* [19:14]=22, [25:20]=20, [31:26]=1d */
		odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1d822);
		odm_set_bb_reg(dm, R_0x86c, 0x1, 0x0);
		odm_set_bb_reg(dm, R_0x8a4, 0x3f, 0x18);
		/* RF bandwidth */
		rf_reg18 = (rf_reg18 | BIT(11) | BIT(10));

		break;
	}
	case CHANNEL_WIDTH_40: {
		/* CCK primary channel */
		if (primary_ch_idx == 1)
			odm_set_bb_reg(dm, R_0x1a00, BIT(4), primary_ch_idx);
		else
			odm_set_bb_reg(dm, R_0x1a00, BIT(4), 0);
		/*val32 = odm_get_bb_reg(dm, R_0x8ac, MASKDWORD);*/
		/*val32 &= 0xFF3FF300;*/
		/*val32 |= (((primary_ch_idx & 0xf) << 2) | CHANNEL_WIDTH_40);*/
		/*odm_set_bb_reg(dm, R_0x8ac, MASKDWORD, val32);*/
		odm_set_bb_reg(dm, R_0x9b0, 0x3, 0x1); /*TX_RF_BW*/
		odm_set_bb_reg(dm, R_0x9b0, 0xc, 0x1); /*RX_RF_BW*/
		odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0xf00, primary_ch_idx);
		odm_set_bb_reg(dm, R_0x9b0, 0xf000, primary_ch_idx);
		/* DAC clock = 160M clock for BW40 */
		/* ADC clock = 160M clock for BW40 */
		odm_set_bb_reg(dm, R_0x9b4, MASKH3BYTES, 0x9246db);

		/* Gain setting */
		/* !!The value will depend on the contents of AGC table!! */
		/* AGC table change ==> parameter must be changed*/
		/* [17:12]=19, [11:6]=19 */
		odm_set_bb_reg(dm, R_0x82c, 0x3ffc0, 0x659);
		/* [19:14]=26, [25:20]=24, [31:26]=21 */
		odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x21926);
		odm_set_bb_reg(dm, R_0x86c, 0x1, 0x0);
		odm_set_bb_reg(dm, R_0x8a4, MASK12BITS, 0x71c);
		odm_set_bb_reg(dm, R_0x870, 0x3ffff, 0x21926);

		/* RF bandwidth */
		rf_reg18 = (rf_reg18 & (~(BIT(11) | BIT(10))));
		rf_reg18 = (rf_reg18 | BIT(10));

		break;
	}
	case CHANNEL_WIDTH_80: {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "switch_bandwidth_8198f(): Not sup BW80 (bw: %d, primary ch: %d)\n",
			  bandwidth, primary_ch_idx);
	}
	case CHANNEL_WIDTH_5: {
		/*val32 = odm_get_bb_reg(dm, R_0x8ac, MASKDWORD);*/
		/*val32 &= 0xEFEEFE00;*/
		/*val32 |= ((BIT(6) | CHANNEL_WIDTH_20));*/
		/*odm_set_bb_reg(dm, R_0x8ac, MASKDWORD, val32);*/
		odm_set_bb_reg(dm, R_0x810, MASKDWORD, 0x10b02ab0);
		odm_set_bb_reg(dm, R_0x9b0, 0x00000003, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0x000000c0, 0x1);
		odm_set_bb_reg(dm, R_0x9b0, 0x00000f00, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0x0000f000, 0x0);
		/* DAC clock = 160M clock for BW5 */
		/* ADC clock = 160M clock for BW5 */
		odm_set_bb_reg(dm, R_0x9b4, MASKH3BYTES, 0x9246db);

		/* RF bandwidth */
		rf_reg18 = (rf_reg18 | BIT(11) | BIT(10));

		break;
	}
	case CHANNEL_WIDTH_10: {
		/*val32 = odm_get_bb_reg(dm, R_0x8ac, MASKDWORD);*/
		/*val32 &= 0xEFFEFF00;*/
		/*val32 |= ((BIT(7) | CHANNEL_WIDTH_20));*/
		/*odm_set_bb_reg(dm, R_0x8ac, MASKDWORD, val32);*/
		odm_set_bb_reg(dm, R_0x810, MASKDWORD, 0x10b02ab0);
		odm_set_bb_reg(dm, R_0x9b0, 0x00000003, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0x000000c0, 0x2);
		odm_set_bb_reg(dm, R_0x9b0, 0x00000f00, 0x0);
		odm_set_bb_reg(dm, R_0x9b0, 0x0000f000, 0x0);
		/* DAC clock = 160M clock for BW10 */
		/* ADC clock = 160M clock for BW10 */
		odm_set_bb_reg(dm, R_0x9b4, MASKH3BYTES, 0x9246db);

		/* RF bandwidth */
		rf_reg18 = (rf_reg18 | BIT(11) | BIT(10));

		break;
	}
	default:
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_bandwidth_8198f(): Fail to switch bandwidth (bw: %d, primary ch: %d)\n",
			  bandwidth, primary_ch_idx);
	}

	/* Write RF register */
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREGOFFSETMASK, rf_reg18);

	if (dm->rf_type > RF_1T1R) {
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREGOFFSETMASK, rf_reg18);
		odm_set_rf_reg(dm, RF_PATH_C, RF_0x18, RFREGOFFSETMASK, rf_reg18);
		odm_set_rf_reg(dm, RF_PATH_D, RF_0x18, RFREGOFFSETMASK, rf_reg18);
	}
	if (rf_reg_status == false) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "config_phydm_switch_bandwidth_8198f(): Fail to switch bandwidth (bw: %d, primary ch: %d), because writing RF register is fail\n",
			  bandwidth, primary_ch_idx);
		return false;
	}

	/* Toggle IGI to let RF enter RX mode */
	phydm_igi_toggle_8198f(dm);

	/* Dynamic spur detection by PSD and NBI/CSI mask */
	/*if (*dm->mp_mode)
		phydm_dynamic_spur_det_eliminate_8198f(dm);*/

	/* Modify CCA parameters */
	phydm_ccapar_by_rfe_8198f(dm);

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "config_phydm_switch_bandwidth_8198f(): Success to switch bandwidth (bw: %d, primary ch: %d)\n",
		  bandwidth, primary_ch_idx);
	return true;
}

__iram_odm_func__
boolean
config_phydm_switch_channel_bw_8198f(
	struct dm_struct *dm,
	u8 central_ch,
	u8 primary_ch_idx,
	enum channel_width bandwidth)
{
	/* Switch channel */
	if (config_phydm_switch_channel_8198f(dm, central_ch) == false)
		return false;

	/* Switch bandwidth */
	if (config_phydm_switch_bandwidth_8198f(dm, primary_ch_idx, bandwidth) == false)
		return false;

	return true;
}

__iram_odm_func__
boolean
phydm_config_cck_tx_path_8198f(
	struct dm_struct *dm,
	enum bb_path tx_path)
{
	boolean set_result = PHYDM_SET_FAIL;

	/* Using antenna A for transmit all CCK packet */
	odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x8);
	odm_set_bb_reg(dm, R_0x1e5c, BIT(30), 0x1);

	/* Control CCK TX path by 0xa07[7] */
	odm_set_bb_reg(dm, R_0x1e5c, BIT(30), 0x1);

	/* TX logic map and TX path en for Nsts = 1 */
	if (tx_path == BB_PATH_A) /* 1T, 1ss */
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x8); /* CCK */
	else if (tx_path == BB_PATH_B)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x4); /* CCK */
	else if (tx_path == BB_PATH_C)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x2); /* CCK */
	else if (tx_path == BB_PATH_D)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x1); /* CCK */

	/* TX logic map and TX path en for 2T */
	if (tx_path == BB_PATH_AB)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xc); /* CCK */
	else if (tx_path == BB_PATH_AC)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xa); /* CCK */
	else if (tx_path == BB_PATH_AD)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x9); /* CCK */
	else if (tx_path == BB_PATH_BC)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x6); /* CCK */
	else if (tx_path == BB_PATH_BD)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x5); /* CCK */
	else if (tx_path == BB_PATH_CD)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x3); /* CCK */

	/* TX logic map and TX path en for 3T */
	if (tx_path == BB_PATH_ABC)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xe); /* CCK */
	else if (tx_path == BB_PATH_ABD)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xd); /* CCK */
	else if (tx_path == BB_PATH_ACD)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xb); /* CCK */
	else if (tx_path == BB_PATH_BCD)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x7); /* CCK */

	/* TX logic map and TX path en for 4T */
	if (tx_path == BB_PATH_ABCD)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xf); /* CCK */

	set_result = PHYDM_SET_SUCCESS;

	return set_result;
}

__iram_odm_func__
boolean
phydm_config_cck_rx_path_8198f(
	struct dm_struct *dm,
	enum bb_path rx_path)
{
	boolean set_result = PHYDM_SET_FAIL;

	/*[RX Antenna Setting] ==========================================*/
	odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x0); /*Disable MRC for CCK CCA */
	odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x0); /*Disable MRC for CCK barker */

	/* Setting the 4-path RX MRC enable */
	if (rx_path == BB_PATH_A || rx_path == BB_PATH_B || rx_path == BB_PATH_C || rx_path == BB_PATH_D) {
		odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x0);
		odm_set_bb_reg(dm, R_0x1ac0, 0x80000000, 0x0);
	} else if (rx_path == BB_PATH_AB || rx_path == BB_PATH_AC || rx_path == BB_PATH_AD ||
		   rx_path == BB_PATH_BC || rx_path == BB_PATH_BD || rx_path == BB_PATH_CD) {
		odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x1);
		odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x1);
	} else if (rx_path == BB_PATH_ABC || rx_path == BB_PATH_ABD ||
		   rx_path == BB_PATH_ACD || rx_path == BB_PATH_BCD) {
		odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x2);
		odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x2);
	} else if (rx_path == BB_PATH_ABCD) {
		odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x3);
		odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x3);
	}

	/* Initailize the CCK path mapping */
	odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x0);
	odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0x0);

	/* CCK RX 1~4 path setting*/
	/* The path-X signal in the CCK is from the path-X (or Y) ADC */
	if (rx_path == BB_PATH_A) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x0); /*00*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0x0); /*00*/
	} else if (rx_path == BB_PATH_B) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x5); /*01*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0x5); /*01*/
	} else if (rx_path == BB_PATH_C) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0xa); /*10*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xa); /*10*/
	} else if (rx_path == BB_PATH_D) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0xf); /*11*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xf); /*11*/
	} else if (rx_path == BB_PATH_AB)
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1); /*00,01*/
	else if (rx_path == BB_PATH_AC)
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x2); /*00,10*/
	else if (rx_path == BB_PATH_AD)
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x3); /*00,11*/
	else if (rx_path == BB_PATH_BC)
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x6); /*01,10*/
	else if (rx_path == BB_PATH_BD)
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x7); /*01,11*/
	else if (rx_path == BB_PATH_CD)
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0xb); /*10,11*/
	else if (rx_path == BB_PATH_ABC) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1); /*00,01*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0c000000, 0x2); /*10*/
	} else if (rx_path == BB_PATH_ABD) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1); /*00,01*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0c000000, 0x3); /*11*/
	} else if (rx_path == BB_PATH_ACD) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x2); /*00,10*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0c000000, 0x1); /*11*/
	} else if (rx_path == BB_PATH_BCD) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x6); /*01,10*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0c000000, 0x1); /*11*/
	} else if (rx_path == BB_PATH_ABCD) {
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1); /*00,01*/
		odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xb); /*10,11*/
	}

	set_result = PHYDM_SET_SUCCESS;

	return set_result;
}

__iram_odm_func__
boolean
phydm_config_ofdm_tx_path_8198f(
	struct dm_struct *dm,
	enum bb_path tx_path)
{
	boolean set_result = PHYDM_SET_FAIL;

	/*[TX Antenna Setting] ==========================================*/
	/* TX path HW block enable */
	odm_set_bb_reg(dm, 0x1e28, 0xf, tx_path);

	/* TX logic map and TX path en for Nsts = 1, and OFDM TX path*/
	if (tx_path == BB_PATH_A) { /* 1T, 1ss */
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x1); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x0);
	} else if (tx_path == BB_PATH_B) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x2); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x0);
	} else if (tx_path == BB_PATH_C) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x4); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x0);
	} else if (tx_path == BB_PATH_D) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x8); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x0);
	} else if (tx_path == BB_PATH_AB) { /* --2TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x3); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x0);
	} else if (tx_path == BB_PATH_AC) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x5); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x0);
	} else if (tx_path == BB_PATH_AD) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x9); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x1);
	} else if (tx_path == BB_PATH_BC) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x6); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x0);
	} else if (tx_path == BB_PATH_BD) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xa); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x1);
	} else if (tx_path == BB_PATH_CD) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xc); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x1);
	} else if (tx_path == BB_PATH_ABC) { /* --3TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x7); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x2);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x0);
	} else if (tx_path == BB_PATH_ABD) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xb); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x2);
	} else if (tx_path == BB_PATH_ACD) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xd); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x2);
	} else if (tx_path == BB_PATH_BCD) {
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xe); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x2);
	} else if (tx_path == BB_PATH_ABCD) { /* --4TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xf); /* path_en */
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000003, 0x0); /* logic map */
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000000c, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000030, 0x2);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000000c0, 0x3);
	}

	/* TX logic map and TX path en for Nsts = 2*/
	/* Due to LO is stand-by while 1T at path-b in normal driver, so 0x940 is the same setting btw path-A/B*/
	if (tx_path == BB_PATH_A || tx_path == BB_PATH_B ||
	    tx_path == BB_PATH_C || tx_path == BB_PATH_D) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x0);
	} else if (tx_path == BB_PATH_AB) { /* --2TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x3);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x0);
	} else if (tx_path == BB_PATH_AC) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x5);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x0);
	} else if (tx_path == BB_PATH_AD) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x9);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x1);
	} else if (tx_path == BB_PATH_BC) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x6);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x0);
	} else if (tx_path == BB_PATH_BD) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xa);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x1);
	} else if (tx_path == BB_PATH_CD) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xc);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x1);
	} else if (tx_path == BB_PATH_ABC) { /* --3TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x7);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x2);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x0);
	} else if (tx_path == BB_PATH_ABD) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xb);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x2);
	} else if (tx_path == BB_PATH_ACD) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xd);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x2);
	} else if (tx_path == BB_PATH_BCD) {
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xe);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x2);
	} else if (tx_path == BB_PATH_ABCD) { /* --4TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xf);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000300, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00000c00, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00003000, 0x2);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0000c000, 0x3);
	}

	/* TX logic map and TX path en for Nsts = 3*/
	if (tx_path == BB_PATH_ABC) { /* --3TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0x7);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00030000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000c0000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00300000, 0x2);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00c00000, 0x0);
	} else if (tx_path == BB_PATH_ABD) {
		odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0xb);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00030000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000c0000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00300000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00c00000, 0x2);
	} else if (tx_path == BB_PATH_ACD) {
		odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0xd);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00030000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000c0000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00300000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00c00000, 0x2);
	} else if (tx_path == BB_PATH_BCD) {
		odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0xe);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00030000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000c0000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00300000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00c00000, 0x2);
	} else if (tx_path == BB_PATH_ABCD) { /* --4TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0xf);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00030000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x000c0000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00300000, 0x2);
		odm_set_bb_reg(dm, R_0x1e2c, 0x00c00000, 0x3);
	}

	/* TX logic map and TX path en for Nsts = 4 */
	if (tx_path == BB_PATH_ABCD) { /* --4TX-- */
		odm_set_bb_reg(dm, R_0x820, 0x0000f000, 0xf);
		odm_set_bb_reg(dm, R_0x1e2c, 0x03000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e2c, 0x0c000000, 0x1);
		odm_set_bb_reg(dm, R_0x1e2c, 0x30000000, 0x2);
		odm_set_bb_reg(dm, R_0x1e2c, 0xc0000000, 0x3);
	}
	return set_result;
}

__iram_odm_func__
boolean
phydm_config_ofdm_rx_path_8198f(
	struct dm_struct *dm,
	enum bb_path rx_path)
{
	boolean set_result = PHYDM_SET_SUCCESS;

	/* Setting the number of the antenna in the idle condition*/
	odm_set_bb_reg(dm, R_0x824, MASKBYTE3LOWNIBBLE, rx_path);

	/* Setting the number of the antenna */
	odm_set_bb_reg(dm, R_0x824, 0x000F0000, rx_path);

	/* Setting the RF mode */
	/* RF mode seeting in the RF-0 */
	if (!(rx_path & BB_PATH_A))
		config_phydm_write_rf_reg_8198f(dm, 0, 0, 0xf0000, 0x1);
	if (!(rx_path & BB_PATH_B))
		config_phydm_write_rf_reg_8198f(dm, 1, 0, 0xf0000, 0x1);
	if (!(rx_path & BB_PATH_C))
		config_phydm_write_rf_reg_8198f(dm, 2, 0, 0xf0000, 0x1);
	if (!(rx_path & BB_PATH_D))
		config_phydm_write_rf_reg_8198f(dm, 3, 0, 0xf0000, 0x1);

	if (rx_path == BB_PATH_A || rx_path == BB_PATH_B ||
	    rx_path == BB_PATH_C || rx_path == BB_PATH_D) {
		odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x0);
		odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x0);
	} else if (rx_path == BB_PATH_AB || rx_path == BB_PATH_AC || rx_path == BB_PATH_AD ||
		   rx_path == BB_PATH_BC || rx_path == BB_PATH_BD || rx_path == BB_PATH_CD) {
		odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x1);
		odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x1);
	} else if (rx_path == BB_PATH_ABC || rx_path == BB_PATH_ABD ||
		   rx_path == BB_PATH_ACD || rx_path == BB_PATH_BCD) {
		odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x2);
		odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x2);
	} else if (rx_path == BB_PATH_ABCD) {
		odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x3);
		odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x3);
	}

	return set_result;
}

__iram_odm_func__
boolean
config_phydm_trx_mode_8198f(
	struct dm_struct *dm,
	enum bb_path tx_path,
	enum bb_path rx_path,
	boolean is_tx2_path)
{
	u32 rf_reg33 = 0;
	u16 counter = 0;
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "disable PHY API\n");
		return true;
	}

	if (((tx_path & ~BB_PATH_ABCD) != 0) || ((rx_path & ~BB_PATH_ABCD) != 0)) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Wrong setting: TX:0x%x, RX:0x%x\n", tx_path,
			  rx_path);
		return false;
	}

	/* [mode table] RF mode of path-A and path-B ===========================*/
	/* Cannot shut down path-A, beacause synthesizer will be shut down when path-A is in shut down mode */
	/* 3-wire setting */
	/*0: shutdown, 1: standby, 2: TX, 3: RX */
	/* RF mode setting*/
	if ((tx_path | rx_path) & BB_PATH_A)
		odm_set_bb_reg(dm, 0x1800, MASK20BITS, 0x33312);
	else
		odm_set_bb_reg(dm, 0x1800, MASK20BITS, 0x11111);
	if ((tx_path | rx_path) & BB_PATH_B)
		odm_set_bb_reg(dm, 0x4100, MASK20BITS, 0x33312);
	else
		odm_set_bb_reg(dm, 0x4100, MASK20BITS, 0x11111);
	if ((tx_path | rx_path) & BB_PATH_C)
		odm_set_bb_reg(dm, 0x5200, MASK20BITS, 0x33312);
	else
		odm_set_bb_reg(dm, 0x5200, MASK20BITS, 0x11111);
	if ((tx_path | rx_path) & BB_PATH_D)
		odm_set_bb_reg(dm, 0x5300, MASK20BITS, 0x33312);
	else
		odm_set_bb_reg(dm, 0x5300, MASK20BITS, 0x11111);

	/* CCK TX antenna mapping */
	phydm_config_cck_tx_path_8198f(dm, tx_path);

	/* OFDM TX antenna mapping*/
	phydm_config_ofdm_tx_path_8198f(dm, tx_path);
	odm_set_bb_reg(dm, R_0x1c28, 0xf, tx_path);

	/* CCK RX antenna mapping */
	phydm_config_cck_rx_path_8198f(dm, rx_path);

	/* OFDM RX antenna mapping*/
	phydm_config_ofdm_rx_path_8198f(dm, rx_path);

	if (rx_path == BB_PATH_A || rx_path == BB_PATH_B) {
		/* 1R */
		/* Disable Antenna weighting */
		odm_set_bb_reg(dm, R_0xc44, BIT(17), 0x0); /*AntWgt_en*/
		odm_set_bb_reg(dm, R_0xc54, BIT(20), 0x0); /*htstf ant-wgt enable = 0*/
		odm_set_bb_reg(dm, R_0xc38, BIT(24), 0x0); /*MRC_mode  =  'original ZF eqz'*/
	} else {
		/* 2R 3R 4R */
		/* Enable Antenna weighting */
		odm_set_bb_reg(dm, R_0xc44, BIT(17), 0x1); /*AntWgt_en*/
		odm_set_bb_reg(dm, R_0xc54, BIT(20), 0x1); /*htstf ant-wgt enable = 1*/
		odm_set_bb_reg(dm, R_0xc38, BIT(24), 0x1); /*MRC_mode =  'modified ZF eqz'*/
	}

	/* Update TXRX antenna status for PHYDM */
	dm->tx_ant_status = (tx_path & 0xf);
	dm->rx_ant_status = (rx_path & 0xf);
	/*
	if (*dm->mp_mode || (*dm->antenna_test) || (dm->normal_rx_path)) {
		0xef 0x80000  0x33 0x00001  0x3e 0x00034  0x3f 0x4080e  0xef 0x00000    suggested by Lucas
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xef, RFREGOFFSETMASK, 0x80000);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x33, RFREGOFFSETMASK, 0x00001);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x3e, RFREGOFFSETMASK, 0x00034);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x3f, RFREGOFFSETMASK, 0x4080e);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xef, RFREGOFFSETMASK, 0x00000);
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "MP mode or Antenna test mode!! support path-B TX and RX\n");
	} else {
		0xef 0x80000  0x33 0x00001  0x3e 0x00034  0x3f 0x4080c  0xef 0x00000
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xef, RFREGOFFSETMASK, 0x80000);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x33, RFREGOFFSETMASK, 0x00001);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x3e, RFREGOFFSETMASK, 0x00034);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x3f, RFREGOFFSETMASK, 0x4080c);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xef, RFREGOFFSETMASK, 0x00000);
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Normal mode!! Do not support path-B TX and RX\n");
	}*/

	/*odm_set_rf_reg(dm, RF_PATH_A, RF_0xef, RFREGOFFSETMASK, 0x00000);*/

	/* Toggle igi to let RF enter RX mode, because BB doesn't send 3-wire command when RX path is enable */
	phydm_igi_toggle_8198f(dm);

	/* Modify CCA parameters */
	phydm_ccapar_by_rfe_8198f(dm);

	/* HW Setting depending on RFE type & band */
	phydm_rfe_8198f(dm, central_ch_8198f);

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "Success to set TRx mode setting (TX: 0x%x, RX: 0x%x)\n",
		  tx_path, rx_path);
	return true;
}

__iram_odm_func__
boolean
config_phydm_parameter_init_8198f(
	struct dm_struct *dm,
	enum odm_parameter_init type)
{
	if (type == ODM_PRE_SETTING) {
		odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x0); /* 0x808 -> 0x1c3c, 0 ->29, 1->28 */
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "%s: Pre setting: disable OFDM and CCK block\n",
			  __func__);
	} else if (type == ODM_POST_SETTING) {
		odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x3); /* 0x808 -> 0x1c3c, 0 ->29, 1->28 */
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "%s: Post setting: enable OFDM and CCK block\n",
			  __func__);
#if (PHYDM_FW_API_FUNC_ENABLE_8198F == 1)
	} else if (type == ODM_INIT_FW_SETTING) {
		u8 h2c_content[4] = {0};

		h2c_content[0] = dm->rfe_type;
		h2c_content[1] = dm->rf_type;
		h2c_content[2] = dm->cut_version;
		h2c_content[3] = (dm->tx_ant_status << 4) | dm->rx_ant_status;

		odm_fill_h2c_cmd(dm, PHYDM_H2C_FW_GENERAL_INIT, 4, h2c_content);
#endif
	} else {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s: Wrong type!!\n", __func__);
		return false;
	}

	return true;
}

/* ======================================================================== */
#endif /*PHYDM_FW_API_ENABLE_8198F == 1*/
#endif /* RTL8198F_SUPPORT == 1 */
