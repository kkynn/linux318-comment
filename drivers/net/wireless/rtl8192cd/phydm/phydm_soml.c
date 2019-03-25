/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
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
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

/* ************************************************************
 * include files
 * ************************************************************ */

#include "mp_precomp.h"
#include "phydm_precomp.h"


extern u1Byte
phydm_rate_to_num_ss(
	IN OUT	PDM_ODM_T		pDM_Odm,
	IN		u1Byte			DataRate
);

/* ******************************************************
 * when antenna test utility is on or some testing need to disable antenna diversity
 * call this function to disable all ODM related mechanisms which will switch antenna.
 * ****************************************************** */
#if (CONFIG_ADAPTIVE_SOML)
void
phydm_soml_on_off(
	IN		PVOID		pDM_VOID,
	u8		swch
)
{
	PDM_ODM_T				p_dm_odm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);

	if (swch == SOML_ON) {
		
		ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("(( Turn on )) SOML\n"));

		if (p_dm_odm->SupportICType == ODM_RTL8822B) {
			/*ODM_SetBBReg(p_dm_odm, 0x19a8, 0xf0000000, 0xd); */
			phydm_somlrxhp_setting(p_dm_odm, TRUE);
		} else if (p_dm_odm->SupportICType == ODM_RTL8197F) {
			ODM_SetBBReg(p_dm_odm, 0x998, BIT(6), swch);
		}
	} else if (swch == SOML_OFF) {
	
		ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("(( Turn off )) SOML\n"));

		if (p_dm_odm->SupportICType == ODM_RTL8822B) {
			/*ODM_SetBBReg(p_dm_odm, 0x19a8, 0xf0000000, 0x0); */
			phydm_somlrxhp_setting(p_dm_odm, FALSE);
		} else if (p_dm_odm->SupportICType == ODM_RTL8197F) {
			ODM_SetBBReg(p_dm_odm, 0x998, BIT(6), swch);
		}
	}
	p_dm_soml_table->soml_on_off = swch;
}

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
void
phydm_adaptive_soml_callback(
	PRT_TIMER		pTimer
)
{
	PADAPTER		Adapter = (PADAPTER)pTimer->Adapter;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		p_dm_odm = &pHalData->DM_OutSrc;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);

#if DEV_BUS_TYPE == RT_PCI_INTERFACE
#if USE_WORKITEM
	ODM_ScheduleWorkItem(&(p_dm_soml_table->phydm_adaptive_soml_workitem));
#else
	{
		/*dbg_print("phydm_adaptive_soml-phydm_adaptive_soml_callback\n");*/
		phydm_adaptive_soml(p_dm_odm);
	}
#endif
#else
	ODM_ScheduleWorkItem(&(p_dm_soml_table->phydm_adaptive_soml_workitem));
#endif
}

void
phydm_adaptive_soml_workitem_callback(
	IN PVOID            pContext
)
{
	PADAPTER		pAdapter = (PADAPTER)pContext;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

	PDM_ODM_T		*p_dm_odm = &(pHalData->DM_OutSrc);

	/*dbg_print("phydm_adaptive_soml-phydm_adaptive_soml_workitem_callback\n");*/
	phydm_adaptive_soml(p_dm_odm);
}

#else

void
phydm_adaptive_soml_callback(
	IN		PVOID		pDM_VOID
)
{
	PDM_ODM_T				p_dm_odm = (PDM_ODM_T)pDM_VOID;
	//struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);
	
	ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("******SOML_Callback******\n"));
	phydm_adsl(p_dm_odm);

}

#endif

void
phydm_adaptive_soml_timers(
	IN		PVOID		pDM_VOID,
	u8		state
)
{
	PDM_ODM_T				p_dm_odm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);

	if (state == INIT_SOML_TIMMER) {

		ODM_InitializeTimer(p_dm_odm, &(p_dm_soml_table->phydm_adaptive_soml_timer),
			(void *)phydm_adaptive_soml_callback, NULL, "phydm_adaptive_soml_timer");
	} else if (state == CANCEL_SOML_TIMMER) {

		ODM_CancelTimer(p_dm_odm, &(p_dm_soml_table->phydm_adaptive_soml_timer));
	} else if (state == RELEASE_SOML_TIMMER) {

		ODM_ReleaseTimer(p_dm_odm, &(p_dm_soml_table->phydm_adaptive_soml_timer));
	}
}

void
phydm_soml_debug(
	IN		PVOID		pDM_VOID,
	u32		*const dm_value,
	u32		*_used,
	char			*output,
	u32		*_out_len
)
{
	PDM_ODM_T				p_dm_odm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);
	u32 used = *_used;
	u32 out_len = *_out_len;

	if (dm_value[0] == 1) { /*Turn on/off SOML*/
		p_dm_soml_table->soml_select = (u8)dm_value[1];

	} else if (dm_value[0] == 2) { /*training number for SOML*/

		p_dm_soml_table->soml_train_num = (u8)dm_value[1];
		PHYDM_SNPRINTF((output + used, out_len - used, "soml_train_num = ((%d))\n", p_dm_soml_table->soml_train_num));
	} else if (dm_value[0] == 3) { /*training interval for SOML*/

		p_dm_soml_table->soml_intvl = (u8)dm_value[1];
		PHYDM_SNPRINTF((output + used, out_len - used, "soml_intvl = ((%d))\n", p_dm_soml_table->soml_intvl));
	} else if (dm_value[0] == 4) { /*function period for SOML*/

		p_dm_soml_table->soml_period = (u8)dm_value[1];
		PHYDM_SNPRINTF((output + used, out_len - used, "soml_period = ((%d))\n", p_dm_soml_table->soml_period));
	} else if (dm_value[0] == 5) { /*delay_time for SOML*/

		p_dm_soml_table->soml_delay_time= (u8)dm_value[1];
		PHYDM_SNPRINTF((output + used, out_len - used, "soml_delay_time = ((%d))\n", p_dm_soml_table->soml_delay_time));
	} else if (dm_value[0] == 100) { /*show parameters*/

		PHYDM_SNPRINTF((output + used, out_len - used, "soml_train_num = ((%d))\n", p_dm_soml_table->soml_train_num));
		PHYDM_SNPRINTF((output + used, out_len - used, "soml_intvl = ((%d))\n", p_dm_soml_table->soml_intvl));
		PHYDM_SNPRINTF((output + used, out_len - used, "soml_period = ((%d))\n", p_dm_soml_table->soml_period));
		PHYDM_SNPRINTF((output + used, out_len - used, "soml_delay_time = ((%d))\n", p_dm_soml_table->soml_delay_time));
	}
}

VOID
phydm_soml_statistics(
	IN		PVOID			pDM_VOID,
	IN		u1Byte			on_off_state
)
{
	PDM_ODM_T				p_dm_odm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);
	ODM_PHY_DBG_INFO_T		*p_dbg = &(p_dm_odm->PhyDbgInfo);

	u8	i, j;
	u32 	num_pkt_diff, num_error_diff;
	u32	num_bytes_diff, num_rate_diff;
	
	if (p_dm_odm->SupportICType == ODM_RTL8197F) {
		if (on_off_state == SOML_ON) {
			for (i = 0; i < HT_RATE_IDX; i++) {
				num_pkt_diff = p_dbg->num_qry_ht_pkt[i] - p_dm_soml_table->pre_num_ht_pkt[i];
				p_dm_soml_table->num_ht_pkt_on[i] += num_pkt_diff;
				p_dm_soml_table->pre_num_ht_pkt[i] = p_dbg->num_qry_ht_pkt[i];
			}
		} else if (on_off_state == SOML_OFF) {
			for (i = 0; i < HT_RATE_IDX; i++) {
				num_pkt_diff = p_dbg->num_qry_ht_pkt[i] - p_dm_soml_table->pre_num_ht_pkt[i];
				p_dm_soml_table->num_ht_pkt_off[i] += num_pkt_diff;
				p_dm_soml_table->pre_num_ht_pkt[i] = p_dbg->num_qry_ht_pkt[i];
			}
		}
	} else if (p_dm_odm->SupportICType == ODM_RTL8822B) {
		if (on_off_state == SOML_ON) {
			for (j = ODM_RATEVHTSS1MCS0; j <= ODM_RATEVHTSS2MCS9; j++) {
				num_rate_diff = p_dm_soml_table->num_vht_cnt[j - ODM_RATEVHTSS1MCS0] - p_dm_soml_table->pre_num_vht_cnt[j - ODM_RATEVHTSS1MCS0];
				p_dm_soml_table->num_vht_cnt_on[j - ODM_RATEVHTSS1MCS0] += num_rate_diff;
				p_dm_soml_table->pre_num_vht_cnt[j - ODM_RATEVHTSS1MCS0] = p_dm_soml_table->num_vht_cnt[j - ODM_RATEVHTSS1MCS0];
			}
			for (i = 0; i < VHT_RATE_IDX; i++) {
				num_pkt_diff = p_dbg->dNumQryVhtPktBytes[i] - p_dm_soml_table->pre_num_vht_pkt[i];
				p_dm_soml_table->num_vht_pkt_on[i] += num_pkt_diff;
				p_dm_soml_table->pre_num_vht_pkt[i] = p_dbg->dNumQryVhtPktBytes[i];
				
				num_error_diff = p_dbg->dNumQryVhtPktCRC[i] - p_dm_soml_table->pre_num_vht_error[i];
				p_dm_soml_table->num_vht_error_on[i] += num_error_diff;
				p_dm_soml_table->pre_num_vht_error[i] = p_dbg->dNumQryVhtPktCRC[i];
			}
		} else if (on_off_state == SOML_OFF) {
			for (j = ODM_RATEVHTSS1MCS0; j <= ODM_RATEVHTSS2MCS9; j++) {
				num_rate_diff = p_dm_soml_table->num_vht_cnt[j - ODM_RATEVHTSS1MCS0] - p_dm_soml_table->pre_num_vht_cnt[j - ODM_RATEVHTSS1MCS0];
				p_dm_soml_table->num_vht_cnt_off[j - ODM_RATEVHTSS1MCS0] += num_rate_diff;
				p_dm_soml_table->pre_num_vht_cnt[j - ODM_RATEVHTSS1MCS0] = p_dm_soml_table->num_vht_cnt[j - ODM_RATEVHTSS1MCS0];
			}
			for (i = 0; i < VHT_RATE_IDX; i++) {
				num_pkt_diff = p_dbg->dNumQryVhtPktBytes[i] - p_dm_soml_table->pre_num_vht_pkt[i];
				p_dm_soml_table->num_vht_pkt_off[i] += num_pkt_diff;
				p_dm_soml_table->pre_num_vht_pkt[i] = p_dbg->dNumQryVhtPktBytes[i];
				
				num_error_diff = p_dbg->dNumQryVhtPktCRC[i] - p_dm_soml_table->pre_num_vht_error[i];
				p_dm_soml_table->num_vht_error_off[i] += num_error_diff;
				p_dm_soml_table->pre_num_vht_error[i] = p_dbg->dNumQryVhtPktCRC[i];
			}
		}
	}
}

void phydm_rx_rate_for_soml(
	IN		PVOID			pDM_VOID,
	IN		PVOID			p_pkt_info_void)
{
	PDM_ODM_T				dm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_ *dm_soml_table = &dm->dm_soml_table;
	PODM_PACKET_INFO_T 	pktinfo=(PODM_PACKET_INFO_T)p_pkt_info_void;
	u8 data_rate = (pktinfo->DataRate & 0x7f);

	if (data_rate >= ODM_RATEMCS0 && data_rate <= ODM_RATEMCS15)
		dm_soml_table->num_ht_cnt[data_rate - ODM_RATEMCS0]++;
	else if ((data_rate >= ODM_RATEVHTSS1MCS0) && (data_rate <= ODM_RATEVHTSS2MCS9))
		dm_soml_table->num_vht_cnt[data_rate - ODM_RATEVHTSS1MCS0]++;
}

void phydm_soml_reset_rx_rate(
	IN		PVOID			pDM_VOID)
{
	PDM_ODM_T				dm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_ *dm_soml_table = &dm->dm_soml_table;
	u8 order;

	for (order = 0; order < HT_RATE_IDX; order++)
		dm_soml_table->num_ht_cnt[order] = 0;

	for (order = 0; order < VHT_RATE_IDX; order++)
		dm_soml_table->num_vht_cnt[order] = 0;
}

void
phydm_adsl(
	IN		PVOID		pDM_VOID
)
{
	PDM_ODM_T				p_dm_odm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);
	ODM_PHY_DBG_INFO_T		*p_dbg = &(p_dm_odm->PhyDbgInfo);
	
	u8 	next_on_off, i, rate_ss_shift = 0;
	u8	ht_size = HT_RATE_IDX, vht_size = VHT_RATE_IDX;
	u8	rate_num = 1;
	u8	rate_ss_1st_on, rate_ss_2nd_on, rate_ss_3rd_on, rate_ss_1st_off, rate_ss_2nd_off, rate_ss_3rd_off;
	u32	byte_total_on = 0, byte_total_off = 0, error_total_on = 0, error_total_off = 0;
	u32	train_tp_on = 0, train_tp_off = 0;
	u32 	on_1st_cnt = 0, on_2nd_cnt = 0, on_3rd_cnt = 0, off_1st_cnt = 0, off_2nd_cnt = 0, off_3rd_cnt = 0;
	u32 	on_1st_idx = 0, on_2nd_idx = 0, on_3rd_idx = 0, off_1st_idx = 0, off_2nd_idx = 0, off_3rd_idx = 0;
	u32	on_off_diff = 0;
	u32	num_ht_total_cnt_on = 0, num_ht_total_cnt_off = 0, total_ht_rate_on = 0, total_ht_rate_off = 0;
	u32	num_vht_total_cnt_on = 0, num_vht_total_cnt_off = 0, total_vht_rate_on = 0, total_vht_rate_off = 0;
	u32	rate_per_pkt_on = 0, rate_per_pkt_off = 0;
	u32 	ht_reset[HT_RATE_IDX] = {0}, vht_reset[VHT_RATE_IDX] = {0};
	u32	rank_ht_on[HT_RATE_IDX], max_to_min_ht_on_idx[HT_RATE_IDX];
	u32	rank_ht_off[HT_RATE_IDX], max_to_min_ht_off_idx[HT_RATE_IDX];
	u32	size = sizeof(ht_reset[0]);
	u16	vht_phy_rate_table[] = {
		/*20M*/
		6, 13, 19, 26, 39, 52, 58, 65, 78, 90, /*1SS MCS0~9*/
		13, 26, 39, 52, 78, 104, 117, 130, 156, 180 /*2SSMCS0~9*/
	};

	if (p_dm_odm->SupportICType & ODM_IC_4SS)
		rate_num = 4;
	else if (p_dm_odm->SupportICType & ODM_IC_3SS)
		rate_num = 3;
	else if (p_dm_odm->SupportICType & ODM_IC_2SS)
		rate_num = 2;

	if ((p_dm_odm->SupportICType & ODM_ADAPTIVE_SOML_SUPPORT_IC)) {
		if (p_dm_odm->number_active_client == 1) {
			ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("number_active_client == 1, RSSI_Min= %d\n", p_dm_odm->RSSI_Min));

			/*Traning state: 0(alt) 1(ori) 2(alt) 3(ori)============================================================*/
			if (p_dm_soml_table->soml_state_cnt < ((p_dm_soml_table->soml_train_num)<<1)) {
				if (p_dm_soml_table->soml_state_cnt == 0) {
					phydm_soml_reset_rx_rate(p_dm_odm);

					if (p_dm_odm->SupportICType == ODM_RTL8197F) {
						ODM_MoveMemory(p_dm_odm, p_dbg->num_qry_ht_pkt, ht_reset, HT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->num_ht_pkt_on, ht_reset, HT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->num_ht_pkt_off, ht_reset, HT_RATE_IDX*size);
					} else if (p_dm_odm->SupportICType == ODM_RTL8822B) {
						ODM_MoveMemory(p_dm_odm, p_dbg->num_qry_vht_pkt, vht_reset, VHT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dbg->dNumQryVhtPkt, vht_reset, VHT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dbg->dNumQryVhtPktCRC, vht_reset, VHT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dbg->dNumQryVhtPktBytes, vht_reset, VHT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->num_vht_pkt_on, vht_reset, VHT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->num_vht_pkt_off, vht_reset, VHT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->num_vht_error_on, vht_reset, VHT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->num_vht_error_off, vht_reset, VHT_RATE_IDX*size);
					}

					p_dm_soml_table->is_soml_method_enable = 1;
					p_dm_soml_table->soml_state_cnt++;
					next_on_off = (p_dm_soml_table->soml_on_off == SOML_ON) ? SOML_ON : SOML_OFF;
					phydm_soml_on_off(p_dm_odm, next_on_off);
					
					ODM_SetTimer(p_dm_odm, &p_dm_soml_table->phydm_adaptive_soml_timer, p_dm_soml_table->soml_delay_time); //ms
				} else if ((p_dm_soml_table->soml_state_cnt %2) != 0) {
					p_dm_soml_table->soml_state_cnt++;
					ODM_MoveMemory(p_dm_odm, p_dm_soml_table->pre_num_ht_cnt, p_dm_soml_table->num_ht_cnt, HT_RATE_IDX * size);
					ODM_MoveMemory(p_dm_odm, p_dm_soml_table->pre_num_vht_cnt, p_dm_soml_table->num_vht_cnt, VHT_RATE_IDX * size);
					if (p_dm_odm->SupportICType == ODM_RTL8197F) {
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->pre_num_ht_pkt, p_dbg->num_qry_ht_pkt, HT_RATE_IDX*size);
					} else if (p_dm_odm->SupportICType == ODM_RTL8822B) {
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->pre_num_vht_pkt, p_dbg->dNumQryVhtPktBytes, VHT_RATE_IDX*size);
						ODM_MoveMemory(p_dm_odm, p_dm_soml_table->pre_num_vht_error, p_dbg->dNumQryVhtPktCRC, VHT_RATE_IDX*size);
					}
					ODM_SetTimer(p_dm_odm, &p_dm_soml_table->phydm_adaptive_soml_timer, p_dm_soml_table->soml_intvl); //ms
				} else if ((p_dm_soml_table->soml_state_cnt %2) == 0) {
					p_dm_soml_table->soml_state_cnt++;
					phydm_soml_statistics(p_dm_odm, p_dm_soml_table->soml_on_off);
					next_on_off = (p_dm_soml_table->soml_on_off == SOML_ON) ? SOML_OFF : SOML_ON;
					phydm_soml_on_off(p_dm_odm, next_on_off);
					ODM_SetTimer(p_dm_odm, &p_dm_soml_table->phydm_adaptive_soml_timer, p_dm_soml_table->soml_delay_time); //ms
				}
			}
			/*Decision state: 4==============================================================*/
			else {
				p_dm_soml_table->soml_state_cnt = 0;
				phydm_soml_statistics(p_dm_odm, p_dm_soml_table->soml_on_off);

				/* [Search 1st and 2ed rate by counter] */
				if (p_dm_odm->SupportICType == ODM_RTL8197F) {
					phydm_seq_sorting(p_dm_odm, p_dm_soml_table->num_ht_pkt_on, max_to_min_ht_on_idx, rank_ht_on, ht_size);
					phydm_seq_sorting(p_dm_odm, p_dm_soml_table->num_ht_pkt_off, max_to_min_ht_off_idx, rank_ht_off, ht_size);
					on_1st_idx = max_to_min_ht_on_idx[0];
					on_2nd_idx = max_to_min_ht_on_idx[1];
					on_1st_cnt = p_dm_soml_table->num_ht_pkt_on[0];
					on_2nd_cnt = p_dm_soml_table->num_ht_pkt_on[1];
					off_1st_idx = max_to_min_ht_off_idx[0];
					off_2nd_idx = max_to_min_ht_off_idx[1];
					off_1st_cnt = p_dm_soml_table->num_ht_pkt_off[0];
					off_2nd_cnt = p_dm_soml_table->num_ht_pkt_off[1];
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[ On 1st : HT-MCS %d, counter = ((%d)) ]\n", on_1st_idx, on_1st_cnt));
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[ On 2nd: HT-MCS %d, counter = ((%d)) ]\n", on_2nd_idx, on_2nd_cnt));
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[ Off 1st : HT-MCS %d, counter = ((%d)) ]\n", off_1st_idx, off_1st_cnt));
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[ Off 2nd: HT-MCS %d, counter = ((%d)) ]\n", off_2nd_idx, off_2nd_cnt));
				} else if (p_dm_odm->SupportICType == ODM_RTL8822B) {
					for (i = 0; i < rate_num; i++) {
					rate_ss_shift = 10 * i;
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[  num_vht_cnt_on  VHT-%d ss MCS[0:9] = {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d} ]\n",
						(i + 1),
						p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 0], p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 1],
						p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 2], p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 3],
						p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 4], p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 5],
						p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 6], p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 7],
						p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 8], p_dm_soml_table->num_vht_cnt_on[rate_ss_shift + 9]));
					}

					 for (i = 0; i < 2; i++) {
						rate_ss_shift = 10 * i;
						ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("* num_vht_pkt_on VHT-%d ss MCS[0:9] = {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d}\n",
						(i + 1),
						p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 0], p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 1],
						p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 2], p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 3],
						p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 4], p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 5],
						p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 6], p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 7],
						p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 8], p_dm_soml_table->num_vht_pkt_on[rate_ss_shift + 9]));
					}

					 for (i = 0; i < rate_num; i++) {
					rate_ss_shift = 10 * i;
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[  num_vht_cnt_off  VHT-%d ss MCS[0:9] = {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d} ]\n",
						(i + 1),
						p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 0], p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 1],
						p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 2], p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 3],
						p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 4], p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 5],
						p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 6], p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 7],
						p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 8], p_dm_soml_table->num_vht_cnt_off[rate_ss_shift + 9]));
					}
					
					for (i = 0; i < 2; i++) {
						rate_ss_shift = 10 * i;
						ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("* num_vht_pkt_off VHT-%d ss MCS[0:9] = {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d}\n",
						(i + 1),
						p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 0], p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 1],
						p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 2], p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 3],
						p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 4], p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 5],
						p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 6], p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 7],
						p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 8], p_dm_soml_table->num_vht_pkt_off[rate_ss_shift + 9]));
					}
					
					for (i = 0; i < VHT_RATE_IDX; i++) {
						byte_total_on += p_dm_soml_table->num_vht_pkt_on[i];
						byte_total_off += p_dm_soml_table->num_vht_pkt_off[i];
						error_total_on += p_dm_soml_table->num_vht_error_on[i];
						error_total_off += p_dm_soml_table->num_vht_error_off[i];
					}
					train_tp_on = (byte_total_on >> 7) / (p_dm_soml_table->soml_intvl*p_dm_soml_table->soml_train_num>>1); /* <<3(8bit), <<10(10^3,ms), >>20(10^6,M)*/
					train_tp_off = (byte_total_off >> 7) / (p_dm_soml_table->soml_intvl*p_dm_soml_table->soml_train_num>>1);/* <<3(8bit), <<10(10^3,ms), >>20(10^6,M)*/
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[  byte_total_on = %d ; byte_total_off = %d; train_tp_on = %d Mbps; train_tp_off = %d Mbps]\n", byte_total_on, byte_total_off, train_tp_on, train_tp_off));
					for (i = ODM_RATEVHTSS2MCS0; i <= ODM_RATEVHTSS2MCS9; i++) {
						num_vht_total_cnt_on += p_dm_soml_table->num_vht_cnt_on[i - ODM_RATEVHTSS1MCS0];
						num_vht_total_cnt_off += p_dm_soml_table->num_vht_cnt_off[i - ODM_RATEVHTSS1MCS0];
						total_vht_rate_on += (p_dm_soml_table->num_vht_cnt_on[i - ODM_RATEVHTSS1MCS0] * vht_phy_rate_table[i - ODM_RATEVHTSS1MCS0]);
						total_vht_rate_off += (p_dm_soml_table->num_vht_cnt_off[i - ODM_RATEVHTSS1MCS0] * vht_phy_rate_table[i - ODM_RATEVHTSS1MCS0]);
					}
					rate_per_pkt_on = (num_vht_total_cnt_on != 0) ? ((total_vht_rate_on << 3) / num_vht_total_cnt_on) : 0;
					rate_per_pkt_off = (num_vht_total_cnt_off != 0) ? ((total_vht_rate_off << 3) / num_vht_total_cnt_off) : 0;
				}

				/* [Decision] */
				ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[Decisoin state ]\n"));
				ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[  rate_per_pkt_on = %d ; rate_per_pkt_off = %d ]\n", rate_per_pkt_on, rate_per_pkt_off));

				if (rate_per_pkt_on > rate_per_pkt_off) {
					next_on_off = SOML_ON;
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[ rate_per_pkt_on > rate_per_pkt_off ==> SOML_ON ]\n"));
				}
				else if (rate_per_pkt_on < rate_per_pkt_off){
					next_on_off = SOML_OFF;
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[ rate_per_pkt_on < rate_per_pkt_off ==> SOML_OFF ]\n"));
				} else {
					ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[ stay at soml_last_state ]\n"));
					next_on_off = p_dm_soml_table->soml_last_state;
				}
				 
				ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[ Final decisoin ] : "));
				if (next_on_off == SOML_ON)
					p_dm_soml_table->soml_on_cnt++;
				else
					p_dm_soml_table->soml_off_cnt++;
				phydm_soml_on_off(p_dm_odm, next_on_off);
				p_dm_soml_table->soml_last_state = next_on_off;
			}
		} else {
			ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[multi-Client]\n"));
			phydm_adaptive_soml_reset(p_dm_odm);
			phydm_soml_on_off(p_dm_odm, SOML_ON);
		}
	}
}

void
phydm_adaptive_soml_reset(
	IN		PVOID		pDM_VOID
)
{
	PDM_ODM_T				p_dm_odm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);

	p_dm_soml_table->soml_state_cnt = 0;
	p_dm_soml_table->is_soml_method_enable = 0;
	p_dm_soml_table->soml_period = 4;
}

#endif /* end of CONFIG_ADAPTIVE_SOML*/

void
phydm_adaptive_soml_init(
	IN		PVOID		pDM_VOID
)
{
#if (CONFIG_ADAPTIVE_SOML)
	PDM_ODM_T					p_dm_odm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);

	if (!(p_dm_odm->SupportAbility & ODM_BB_ADAPTIVE_SOML)) {
		ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[Return]   Not Support Adaptive SOML\n"));
		return;
	}
	ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("phydm_adaptive_soml_init\n"));

	p_dm_soml_table->soml_state_cnt= 0;
	p_dm_soml_table->soml_delay_time = 40;
	p_dm_soml_table->soml_intvl = 150;
	p_dm_soml_table->soml_train_num = 4;
	p_dm_soml_table->is_soml_method_enable = 0;
	p_dm_soml_table->soml_counter = 0;
	p_dm_soml_table->soml_period =  4;
	p_dm_soml_table->soml_select = 0;
#endif
}

void
phydm_adaptive_soml(
	IN		PVOID		pDM_VOID
)
{
#if (CONFIG_ADAPTIVE_SOML)
	PDM_ODM_T					p_dm_odm = (PDM_ODM_T)pDM_VOID;
	struct _ADAPTIVE_SOML_	*p_dm_soml_table = &(p_dm_odm->dm_soml_table);
	
	if (!(p_dm_odm->SupportAbility & ODM_BB_ADAPTIVE_SOML)) {
		ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[Return!!!]   Not Support Adaptive SOML Function\n"));
		return;
	}

	if (p_dm_soml_table->soml_counter <  p_dm_soml_table->soml_period) {
		p_dm_soml_table->soml_counter++;
		return;
	} else
		p_dm_soml_table->soml_counter = 0;

	if (p_dm_soml_table->soml_select == 0) {
		ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[Adaptive SOML Training !!!]\n"));
	} else if (p_dm_soml_table->soml_select == 1) {
		ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[Turn on SOML !!!] Exit from Adaptive SOML Training\n"));
		phydm_soml_on_off(p_dm_odm, SOML_ON);
		return;
	} else if (p_dm_soml_table->soml_select == 2) {
		ODM_RT_TRACE(p_dm_odm, ODM_COMP_ADAPTIVE_SOML, ODM_DBG_LOUD, ("[Turn off SOML !!!] Exit from Adaptive SOML Training\n"));
		phydm_soml_on_off(p_dm_odm, SOML_OFF);
		return;
	}
	
	phydm_adsl(p_dm_odm);

#endif
}
