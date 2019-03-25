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

/* ************************************************************
 * include files
 * ************************************************************ */

#include "mp_precomp.h"

#include "../phydm_precomp.h"

#if (RTL8188E_SUPPORT == 1)

void odm_dig_lower_bound_88e(struct dm_struct *dm)
{
	struct phydm_dig_struct *dig_t = &dm->dm_dig_table;

	if (dm->ant_div_type == CG_TRX_HW_ANTDIV) {
		dig_t->rx_gain_range_min = (u8)dig_t->ant_div_rssi_max;
		PHYDM_DBG(dm, DBG_ANT_DIV, "%s: dig_t->ant_div_rssi_max=%d\n",
			  __func__, dig_t->ant_div_rssi_max);
	}
	/* If only one Entry connected */
}
#endif
