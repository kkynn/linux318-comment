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

#ifndef __HALRF_DPK_8195B_H__
#define __HALRF_DPK_8195B_H__

#if (RTL8195B_SUPPORT == 1)
/*============================================================*/
/*Definition */
/*============================================================*/

/*--------------------------Define Parameters-------------------------------*/
//#define		MAC_REG_NUM_8195B 3
//#define		BB_REG_NUM_8195B 10
//#define		RF_REG_NUM_8195B 5
#define DPK_BB_REG_NUM_8195B 26
#define DPK_RF_REG_NUM_8195B 5
#define DPK_MAC_REG_NUM_8195B 3


/*---------------------------End Define Parameters-------------------------------*/

VOID phy_dp_calibrate_8195b(
	void *dm_void,
	boolean clear);

#else /* (RTL8195B_SUPPORT == 0)*/

#define phy_dp_calibrate_8821c(_pdm_void, clear)

#endif /* RTL8195B_SUPPORT */

#endif /*#ifndef __HALRF_IQK_8195B_H__*/
