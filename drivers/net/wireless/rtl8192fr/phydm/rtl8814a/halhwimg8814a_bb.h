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

/*Image2HeaderVersion: R3 1.1.1*/
#if (RTL8814A_SUPPORT == 1)
#ifndef __INC_MP_BB_HW_IMG_8814A_H
#define __INC_MP_BB_HW_IMG_8814A_H


/******************************************************************************
*                           agc_tab.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_agc_tab( /* tc: Test Chip, mp: mp Chip*/
				     struct dm_struct *dm);
u32 odm_get_version_mp_8814a_agc_tab(void);

/******************************************************************************
*                           phy_reg.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg( /* tc: Test Chip, mp: mp Chip*/
				     struct dm_struct *dm);
u32 odm_get_version_mp_8814a_phy_reg(void);

/******************************************************************************
*                           phy_reg_mp.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_mp( /* tc: Test Chip, mp: mp Chip*/
					struct dm_struct *dm);
u32 odm_get_version_mp_8814a_phy_reg_mp(void);

/******************************************************************************
*                           phy_reg_pg.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg( /* tc: Test Chip, mp: mp Chip*/
					struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg(void);

/******************************************************************************
*                           phy_reg_pg_type0.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type0( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type0(void);

/******************************************************************************
*                           phy_reg_pg_type1.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type1( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type1(void);

/******************************************************************************
*                           phy_reg_pg_type10.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type10( /* tc: Test Chip, mp: mp Chip*/
					       struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type10(void);

/******************************************************************************
*                           phy_reg_pg_type2.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type2( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type2(void);

/******************************************************************************
*                           phy_reg_pg_type3.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type3( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type3(void);

/******************************************************************************
*                           phy_reg_pg_type4.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type4( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type4(void);

/******************************************************************************
*                           phy_reg_pg_type5.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type5( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type5(void);

/******************************************************************************
*                           phy_reg_pg_type6.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type6( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type6(void);

/******************************************************************************
*                           phy_reg_pg_type7.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type7( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type7(void);

/******************************************************************************
*                           phy_reg_pg_type8.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type8( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type8(void);

/******************************************************************************
*                           phy_reg_pg_type9.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_phy_reg_pg_type9( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_phy_reg_pg_type9(void);

#endif
#endif /* end of HWIMG_SUPPORT*/

