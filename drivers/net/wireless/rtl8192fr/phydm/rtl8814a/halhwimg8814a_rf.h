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
#ifndef __INC_MP_RF_HW_IMG_8814A_H
#define __INC_MP_RF_HW_IMG_8814A_H


/******************************************************************************
*                           radioa.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_radioa( /* tc: Test Chip, mp: mp Chip*/
				    struct dm_struct *dm);
u32 odm_get_version_mp_8814a_radioa(void);

/******************************************************************************
*                           radiob.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_radiob( /* tc: Test Chip, mp: mp Chip*/
				    struct dm_struct *dm);
u32 odm_get_version_mp_8814a_radiob(void);

/******************************************************************************
*                           radioc.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_radioc( /* tc: Test Chip, mp: mp Chip*/
				    struct dm_struct *dm);
u32 odm_get_version_mp_8814a_radioc(void);

/******************************************************************************
*                           radiod.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_radiod( /* tc: Test Chip, mp: mp Chip*/
				    struct dm_struct *dm);
u32 odm_get_version_mp_8814a_radiod(void);

/******************************************************************************
*                           txpowertrack.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack( /* tc: Test Chip, mp: mp Chip*/
					  struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack(void);

/******************************************************************************
*                           txpowertrack_type0.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type0( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type0(void);

/******************************************************************************
*                           txpowertrack_type1.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type1( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type1(void);

/******************************************************************************
*                           txpowertrack_type10.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type10(
						 /* tc: Test Chip, mp: mp Chip*/
						 struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type10(void);

/******************************************************************************
*                           txpowertrack_type2.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type2( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type2(void);

/******************************************************************************
*                           txpowertrack_type3.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type3( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type3(void);

/******************************************************************************
*                           txpowertrack_type4.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type4( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type4(void);

/******************************************************************************
*                           txpowertrack_type5.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type5( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type5(void);

/******************************************************************************
*                           txpowertrack_type6.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type6( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type6(void);

/******************************************************************************
*                           txpowertrack_type7.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type7( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type7(void);

/******************************************************************************
*                           txpowertrack_type8.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type8( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type8(void);

/******************************************************************************
*                           txpowertrack_type9.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertrack_type9( /* tc: Test Chip, mp: mp Chip*/
						struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertrack_type9(void);

/******************************************************************************
*                           txpowertssi.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpowertssi( /* tc: Test Chip, mp: mp Chip*/
					 struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpowertssi(void);

/******************************************************************************
*                           txpwr_lmt.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt( /* tc: Test Chip, mp: mp Chip*/
				       struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt(void);

/******************************************************************************
*                           txpwr_lmt_type0.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type0( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type0(void);

/******************************************************************************
*                           txpwr_lmt_type1.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type1( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type1(void);

/******************************************************************************
*                           txpwr_lmt_type10.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type10( /* tc: Test Chip, mp: mp Chip*/
					      struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type10(void);

/******************************************************************************
*                           txpwr_lmt_type2.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type2( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type2(void);

/******************************************************************************
*                           txpwr_lmt_type3.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type3( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type3(void);

/******************************************************************************
*                           txpwr_lmt_type4.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type4( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type4(void);

/******************************************************************************
*                           txpwr_lmt_type5.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type5( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type5(void);

/******************************************************************************
*                           txpwr_lmt_type6.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type6( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type6(void);

/******************************************************************************
*                           txpwr_lmt_type7.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type7( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type7(void);

/******************************************************************************
*                           txpwr_lmt_type8.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type8( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type8(void);

/******************************************************************************
*                           txpwr_lmt_type9.TXT
******************************************************************************/

void
odm_read_and_config_mp_8814a_txpwr_lmt_type9( /* tc: Test Chip, mp: mp Chip*/
					     struct dm_struct *dm);
u32	odm_get_version_mp_8814a_txpwr_lmt_type9(void);

#endif
#endif /* end of HWIMG_SUPPORT*/

