/*
 * Copyright (C) 2014 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : Definition of rtk/rg or customized snooping mode and API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1)
 */

#include "mcast_mgmt.h"
#include "mcast_rtk_wrapper.h"
#include "mcast_rg_wrapper.h"


const static omci_mcast_wrapper_info_t snp_supported_mode[SNP_MODE_END] =
{
    {SNP_MODE_RTK, &rtk_snp_wrapper},
    {SNP_MODE_RG,  &rg_snp_wrapper},
    /* Customers could use own handlers */
};

omci_mcast_wrapper_t* omci_mcast_wrapper_find(omci_mcast_snp_mode_t snp_mode)
{
	unsigned int  mode_index;

	for (mode_index = 0; mode_index < SNP_MODE_END; mode_index++)
    {
        if (snp_supported_mode[mode_index].mode_id == snp_mode)
        {
            return snp_supported_mode[mode_index].pWrapper;
        }
    }
	return 0;
}
