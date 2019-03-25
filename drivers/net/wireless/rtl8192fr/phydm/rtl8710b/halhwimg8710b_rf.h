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

/*Image2HeaderVersion: 3.5.2*/
#if (RTL8710B_SUPPORT == 1)
#ifndef __INC_MP_RF_HW_IMG_8710B_H
#define __INC_MP_RF_HW_IMG_8710B_H


/******************************************************************************
*                           radioa.TXT
******************************************************************************/

void
odm_read_and_config_mp_8710b_radioa(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8710b_radioa(void);

/******************************************************************************
*                           txpowertrack_qfn48m_smic.TXT
******************************************************************************/

void
odm_read_and_config_mp_8710b_txpowertrack_qfn48m_smic(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8710b_txpowertrack_qfn48m_smic(void);

/******************************************************************************
*                           txpowertrack_qfn48m_umc.TXT
******************************************************************************/

void
odm_read_and_config_mp_8710b_txpowertrack_qfn48m_umc(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8710b_txpowertrack_qfn48m_umc(void);

/******************************************************************************
*                           txpwr_lmt.TXT
******************************************************************************/

void
odm_read_and_config_mp_8710b_txpwr_lmt(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8710b_txpwr_lmt(void);

/******************************************************************************
*                           txxtaltrack.TXT
******************************************************************************/

void
odm_read_and_config_mp_8710b_txxtaltrack(/* tc: Test Chip, mp: mp Chip*/
	struct	dm_struct *dm
);
u32	odm_get_version_mp_8710b_txxtaltrack(void);

#endif
#endif /* end of HWIMG_SUPPORT*/

