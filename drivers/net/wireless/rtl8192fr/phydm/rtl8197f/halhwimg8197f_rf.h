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

/*Image2HeaderVersion: R3 1.0*/
#if (RTL8197F_SUPPORT == 1)
#ifndef __INC_MP_RF_HW_IMG_8197F_H
#define __INC_MP_RF_HW_IMG_8197F_H


/******************************************************************************
*                           radioa.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_radioa(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_radioa(void);

/******************************************************************************
*                           radiob.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_radiob(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_radiob(void);

/******************************************************************************
*                           txpowertrack.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack(void);

/******************************************************************************
*                           txpowertrack_type0.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack_type0(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack_type0(void);

/******************************************************************************
*                           txpowertrack_type1.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack_type1(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack_type1(void);

/******************************************************************************
*                           txpowertrack_type2.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack_type2(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack_type2(void);

/******************************************************************************
*                           txpowertrack_type3.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack_type3(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack_type3(void);

/******************************************************************************
*                           txpowertrack_type4.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack_type4(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack_type4(void);

/******************************************************************************
*                           txpowertrack_type5.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack_type5(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack_type5(void);

/******************************************************************************
*                           txpowertrack_type6.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack_type6(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack_type6(void);

/******************************************************************************
*                           txpowertrack_type7.TXT
******************************************************************************/

void
odm_read_and_config_mp_8197f_txpowertrack_type7(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8197f_txpowertrack_type7(void);

#endif
#endif /* end of HWIMG_SUPPORT*/

