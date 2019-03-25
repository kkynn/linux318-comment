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
 * Purpose : Definition callback API for rtk snooping mode
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1)
 */

#include <igmp_ipc.h>
#include "mcast_mgmt.h"
#include "feature_mgmt.h"

#define TO_LOGICAL_PORT(port)	(1 + port)

#define MCAST_NOP_CHK(portId, retCode)\
do {\
    if ((portId == gInfo.devCapabilities.ponPort) && \
        (OMCI_DEV_MODE_BRIDGE == gInfo.devMode) && \
        (FAL_ERR_NOT_REGISTER != feature_api_is_registered(FEATURE_API_BDP_00000100))) \
        return retCode; \
} while (0)


static void
mcast_rtk_fill_filtering_info(mcast_port_info_t	*pMcastInfo, OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
	switch(pBridgeRule->vlanRule.filterMode)
	{
		case VLAN_OPER_MODE_FORWARD_ALL:
			pMcastInfo->inTag2.tagOp.tagFilter = IGMP_NO_FILTER;
			pMcastInfo->inTag1.tagOp.tagFilter = IGMP_NO_FILTER;
			break;
		case VLAN_OPER_MODE_FORWARD_UNTAG:
			pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_UNTAG;//4
			pMcastInfo->inTag1.tagOp.tagFilter = IGMP_FILTER_UNTAG;
			break;
		case VLAN_OPER_MODE_FORWARD_SINGLETAG:
			pMcastInfo->inTag2.tpid = 0x8100;
			pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_TAG;
			pMcastInfo->inTag1.tagOp.tagFilter = IGMP_FILTER_UNTAG;
			break;
		case VLAN_OPER_MODE_FILTER_INNER_PRI:
			pMcastInfo->inTag2.tci.bit.pri = pBridgeRule->vlanRule.filterRule.filterCTag.pri;
			pMcastInfo->inTag2.tpid = 0x8100;
			pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_PRI;
			pMcastInfo->inTag1.tagOp.tagFilter = IGMP_FILTER_UNTAG;
			break;
		case VLAN_OPER_MODE_FILTER_SINGLETAG:
			pMcastInfo->inTag2.tpid = 0x8100;
			pMcastInfo->inTag2.tci.bit.vid = pBridgeRule->vlanRule.filterRule.filterCTag.vid;
			pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_VID;
			if ((pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_PRI) ||
				(pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_TCI))
			{
				pMcastInfo->inTag2.tci.bit.pri = pBridgeRule->vlanRule.filterRule.filterCTag.pri;
				pMcastInfo->inTag2.tagOp.tagFilter |= IGMP_FILTER_PRI;
			}
			break;
		case VLAN_OPER_MODE_EXTVLAN:
			//filter untag
			if (pBridgeRule->vlanRule.filterRule.filterStagMode & VLAN_FILTER_NO_TAG &&
				pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_NO_TAG)
			{
				pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_UNTAG;
				pMcastInfo->inTag1.tagOp.tagFilter = IGMP_FILTER_UNTAG;

				if (pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_ETHTYPE)
				{
					pMcastInfo->inTag2.tagOp.tagFilter |= IGMP_FILTER_ETYPE;
					switch (pBridgeRule->vlanRule.filterRule.etherType)
					{
						case ETHTYPE_FILTER_IP:
							pMcastInfo->inTag2.etype = 0x0800;
							break;
						case ETHTYPE_FILTER_PPPOE:
							pMcastInfo->inTag2.etype = 0x8863;
							break;
						case ETHTYPE_FILTER_ARP:
							pMcastInfo->inTag2.etype = 0x0806;
							break;
						case ETHTYPE_FILTER_PPPOE_S:
							pMcastInfo->inTag2.etype = 0x8864;
							break;
						case ETHTYPE_FILTER_IPV6:
							pMcastInfo->inTag2.etype = 0x86dd;
							break;
						default:
							break;
					}
				}
			}

			//filter outer tag
			if (!(pBridgeRule->vlanRule.filterRule.filterStagMode & VLAN_FILTER_NO_TAG))
			{
			    //TBD
				pMcastInfo->inTag1.tpid = 0x88a8;

				if (pBridgeRule->vlanRule.filterRule.filterStagMode & VLAN_FILTER_NO_CARE_TAG)
				{
					pMcastInfo->inTag1.tagOp.tagFilter = IGMP_NO_FILTER;
				}
				if (pBridgeRule->vlanRule.filterRule.filterStagMode & VLAN_FILTER_CARE_TAG)
				{
					pMcastInfo->inTag1.tagOp.tagFilter = IGMP_FILTER_TAG;
				}
				if (pBridgeRule->vlanRule.filterRule.filterStagMode & VLAN_FILTER_TCI)
				{
					pMcastInfo->inTag1.tci.bit.vid = pBridgeRule->vlanRule.filterRule.filterSTag.vid;
					pMcastInfo->inTag1.tci.bit.pri = pBridgeRule->vlanRule.filterRule.filterSTag.pri;
					pMcastInfo->inTag1.tagOp.tagFilter = (IGMP_FILTER_VID | IGMP_FILTER_PRI |
                                                            IGMP_FILTER_TPID);
				}
				else
				{
					if (pBridgeRule->vlanRule.filterRule.filterStagMode & VLAN_FILTER_VID)
					{
						pMcastInfo->inTag1.tci.bit.vid = pBridgeRule->vlanRule.filterRule.filterSTag.vid;
						pMcastInfo->inTag1.tagOp.tagFilter = IGMP_FILTER_VID;
					}

					if (pBridgeRule->vlanRule.filterRule.filterStagMode & VLAN_FILTER_PRI)
					{
						pMcastInfo->inTag1.tci.bit.pri = pBridgeRule->vlanRule.filterRule.filterSTag.pri;
						if (IGMP_FILTER_VID & pMcastInfo->inTag1.tagOp.tagFilter)
							pMcastInfo->inTag1.tagOp.tagFilter = (IGMP_FILTER_VID | IGMP_FILTER_PRI);
						else
							pMcastInfo->inTag1.tagOp.tagFilter = IGMP_FILTER_PRI;
					}
				}
			}
			else
			{
				pMcastInfo->inTag1.tagOp.tagFilter = IGMP_FILTER_UNTAG;
			}

			//filter inner tag
			if (!(pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_NO_TAG))
			{
				pMcastInfo->inTag2.tpid = 0x8100;

                if (pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_NO_CARE_TAG)
                {
                    pMcastInfo->inTag2.tagOp.tagFilter = IGMP_NO_FILTER;
                }

				if (pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_CARE_TAG)
				{
					pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_TAG;
				}

				if (pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_TCI)
				{
					pMcastInfo->inTag2.tci.bit.vid = pBridgeRule->vlanRule.filterRule.filterCTag.vid;
					pMcastInfo->inTag2.tci.bit.pri = pBridgeRule->vlanRule.filterRule.filterCTag.pri;
					pMcastInfo->inTag2.tagOp.tagFilter = (IGMP_FILTER_VID | IGMP_FILTER_PRI |
                                                            IGMP_FILTER_TPID);
				}
				else
				{
					if (pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_VID)
					{
						pMcastInfo->inTag2.tci.bit.vid = pBridgeRule->vlanRule.filterRule.filterCTag.vid;
						pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_VID;
					}

					if (pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_PRI)
					{
						pMcastInfo->inTag2.tci.bit.pri = pBridgeRule->vlanRule.filterRule.filterCTag.pri;

						if (IGMP_FILTER_VID & pMcastInfo->inTag2.tagOp.tagFilter)
							pMcastInfo->inTag2.tagOp.tagFilter = (IGMP_FILTER_VID | IGMP_FILTER_PRI);
						else
							pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_PRI;
					}
				}

				if (pBridgeRule->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_ETHTYPE)
				{
					pMcastInfo->inTag2.tagOp.tagFilter |= IGMP_FILTER_ETYPE;

					switch (pBridgeRule->vlanRule.filterRule.etherType)
					{
						case ETHTYPE_FILTER_IP:
							pMcastInfo->inTag2.etype = 0x0800;
							break;
						case ETHTYPE_FILTER_PPPOE:
							pMcastInfo->inTag2.etype = 0x8863;
							break;
						case ETHTYPE_FILTER_ARP:
							pMcastInfo->inTag2.etype = 0x0806;
							break;
						case ETHTYPE_FILTER_PPPOE_S:
							pMcastInfo->inTag2.etype = 0x8864;
							break;
						case ETHTYPE_FILTER_IPV6:
							pMcastInfo->inTag2.etype = 0x86dd;
							break;
						default:
							pMcastInfo->inTag2.etype = 0x8100;
					}
				}
			}
			else
			{
				pMcastInfo->inTag2.tagOp.tagFilter = IGMP_FILTER_UNTAG;
			}
			break;
		default:
			break;
	}

	return;
}

static void
mcast_rtk_fill_treatment_vlan(UINT32				actType,
				              OMCI_VID_ACT_MODE_e	vidAct,
				              OMCI_PRI_ACT_MODE_e   priAct,
				              igmp_tag_t			*pTag,
				              unsigned int			assignVid,
				              unsigned int			assignPri)
{

	switch(vidAct)
	{
		case VID_ACT_ASSIGN:
			pTag->tagOp.tagAct = ((VLAN_ACT_ADD == actType) ? IGMP_ADD_VID : IGMP_MODIFY_VID);
			pTag->tci.bit.vid = assignVid;
			break;
		case VID_ACT_COPY_INNER:
			pTag->tagOp.tagAct = IGMP_COPY_INNER_VID;
			break;
		case VID_ACT_COPY_OUTER:
			pTag->tagOp.tagAct = IGMP_COPY_OUTER_VID;
			break;
		default:
			break;
	}

	switch(priAct)
	{
		case PRI_ACT_ASSIGN:
			pTag->tagOp.tagAct |= ((VLAN_ACT_ADD == actType) ? IGMP_ADD_PRI : IGMP_MODIFY_PRI);
			pTag->tci.bit.pri = assignPri;
			break;
		case PRI_ACT_COPY_INNER:
			pTag->tagOp.tagAct |= IGMP_COPY_INNER_PRI;
			break;
		case PRI_ACT_COPY_OUTER:
			pTag->tagOp.tagAct |= IGMP_COPY_OUTER_PRI;
			break;
		default:
			break;
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() tagAct=%u, vid=%u, pri=%u\n", __FUNCTION__,
		pTag->tagOp.tagAct, pTag->tci.bit.vid, pTag->tci.bit.pri);
	return;
}

static void
mcast_rtk_fill_treatment_info(mcast_port_info_t         *pMcastInfo,
				              MIB_TABLE_MCASTOPERPROF_T	*pMibMop,
				              OMCI_BRIDGE_RULE_ts		*pBridgeRule)
{
	switch (pMibMop->UsIGMPTagControl)
	{
		case CTRL_PASS:
			switch (pBridgeRule->vlanRule.sTagAct.vlanAct)
			{
				case VLAN_ACT_NON:
				case VLAN_ACT_TRANSPARENT:
					pMcastInfo->outTag1.tagOp.tagAct = IGMP_TRANSPARENT;
                    pMcastInfo->outTag1.tpid = pMcastInfo->inTag1.tpid;
					break;
				case VLAN_ACT_ADD:
				case VLAN_ACT_MODIFY:
					mcast_rtk_fill_treatment_vlan(pBridgeRule->vlanRule.sTagAct.vlanAct,
						pBridgeRule->vlanRule.sTagAct.vidAct,
						pBridgeRule->vlanRule.sTagAct.priAct, &(pMcastInfo->outTag1),
						pBridgeRule->vlanRule.sTagAct.assignVlan.vid,
						pBridgeRule->vlanRule.sTagAct.assignVlan.pri);
                    pMcastInfo->outTag1.tpid = 0x88a8;
					break;
				case VLAN_ACT_REMOVE:
					pMcastInfo->outTag1.tagOp.tagAct = IGMP_REMOVE_VLAN;
					break;
				default:
					OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d", __FUNCTION__, __LINE__);
			}

			switch(pBridgeRule->vlanRule.cTagAct.vlanAct)
			{
				case VLAN_ACT_NON:
				case VLAN_ACT_TRANSPARENT:
					pMcastInfo->outTag2.tagOp.tagAct = IGMP_TRANSPARENT;
                    pMcastInfo->outTag2.tpid = pMcastInfo->inTag2.tpid;
					break;
				case VLAN_ACT_ADD:
				case VLAN_ACT_MODIFY:
					mcast_rtk_fill_treatment_vlan(pBridgeRule->vlanRule.cTagAct.vlanAct,
						pBridgeRule->vlanRule.cTagAct.vidAct,
						pBridgeRule->vlanRule.cTagAct.priAct, &(pMcastInfo->outTag2),
						pBridgeRule->vlanRule.cTagAct.assignVlan.vid,
						pBridgeRule->vlanRule.cTagAct.assignVlan.pri);
					pMcastInfo->outTag2.tpid = 0x8100;
					break;
				case VLAN_ACT_REMOVE:
					pMcastInfo->outTag2.tagOp.tagAct = IGMP_REMOVE_VLAN;
					break;
				default:
					OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d", __FUNCTION__, __LINE__);
			}
			break;
		case CTRL_ADD:
			pMcastInfo->outTag2.tagOp.tagAct = (IGMP_ADD_VID | IGMP_ADD_PRI);
			pMcastInfo->outTag2.tci.val = pMibMop->UsIGMPTci;
			pMcastInfo->outTag2.tpid =
                (TPID_88A8 == pBridgeRule->vlanRule.outStyle.tpid ? 0x88A8 : 0x8100);
            if (pMcastInfo->inTag2.tpid != pMcastInfo->outTag2.tpid)
                pMcastInfo->outTag2.tagOp.tagAct |= IGMP_ADD_TPID;
			break;
		case CTRL_REPLACE:
			pMcastInfo->outTag2.tagOp.tagAct =
				((pMcastInfo->inTag2.tagOp.tagFilter &  IGMP_FILTER_UNTAG) ?
				    (IGMP_ADD_VID | IGMP_ADD_PRI) : (IGMP_MODIFY_VID | IGMP_MODIFY_PRI));
			pMcastInfo->outTag2.tci.val = pMibMop->UsIGMPTci;
			pMcastInfo->outTag2.tpid =
                (TPID_88A8 == pBridgeRule->vlanRule.outStyle.tpid ? 0x88A8 : 0x8100);
            if (pMcastInfo->inTag2.tpid != pMcastInfo->outTag2.tpid)
                pMcastInfo->outTag2.tagOp.tagAct |= ((pMcastInfo->inTag2.tagOp.tagFilter &  IGMP_FILTER_UNTAG) ?
				    (IGMP_ADD_TPID) : (IGMP_MODIFY_TPID));
			break;
		case CTRL_REPLACE_VID:
			pMcastInfo->outTag2.tpid =
                (TPID_88A8 == pBridgeRule->vlanRule.outStyle.tpid ? 0x88A8 : 0x8100);
			if (pMcastInfo->inTag2.tagOp.tagFilter & IGMP_FILTER_UNTAG)
			{
				pMcastInfo->outTag2.tagOp.tagAct = (IGMP_ADD_VID | IGMP_ADD_PRI);
				pMcastInfo->outTag2.tci.val = pMibMop->UsIGMPTci;
                if (pMcastInfo->inTag2.tpid != pMcastInfo->outTag2.tpid)
                    pMcastInfo->outTag2.tagOp.tagAct |= IGMP_ADD_TPID;
			}
			else
			{
				pMcastInfo->outTag2.tagOp.tagAct = IGMP_MODIFY_VID;
				pMcastInfo->outTag2.tci.bit.vid = (pMibMop->UsIGMPTci & 0xfff);
                if (pMcastInfo->inTag2.tpid != pMcastInfo->outTag2.tpid)
                    pMcastInfo->outTag2.tagOp.tagAct |= IGMP_MODIFY_TPID;
			}
			break;
		default:
			break;
	}
	return;
}

GOS_ERROR_CODE mcast_rtk_profile_per_port_set(UINT32 op, UINT32 aclType, UINT16 portId, UINT32 aclEntryId)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId <= gInfo.devCapabilities.ponPort)
	{
		mcast_mcProfile_t profileVal;
		mcast_msgType_t cmd;

		memset(&profileVal, 0, sizeof(mcast_mcProfile_t));
		cmd = (op == SET_CTRL_WRITE ? MCAST_MSGTYPE_PROFILE_SET : MCAST_MSGTYPE_PROFILE_DEL);
		profileVal.ipType = MULTICAST_TYPE_IPV4;
		profileVal.aclType = (aclType == MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX ? IGMP_GROUP_ENTRY_DYNAMIC : IGMP_GROUP_ENTRY_STATIC);
		profileVal.port = TO_LOGICAL_PORT(portId);
		profileVal.aclEntryId = aclEntryId;
		feature_api(FEATURE_API_MCAST_RTK_IPC_SET, cmd, &profileVal, sizeof(mcast_mcProfile_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_ERR, " %s() %d, port =%u, aclEntryId=%x, ", __FUNCTION__, __LINE__, portId, aclEntryId);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_clear_all(void)
{
    feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_RESET_ALL, NULL, 0);

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "mcast_reset_all");

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_max_sim_group_set(UINT16 portId, UINT16 maxGroup)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId <= gInfo.devCapabilities.ponPort)
	{
		mcast_portGroupLimit_t portGroupLimitVal;
		memset(&portGroupLimitVal, 0, sizeof(mcast_portGroupLimit_t));

		//TBD: IPv6
		portGroupLimitVal.ipType = MULTICAST_TYPE_IPV4;
		portGroupLimitVal.port = TO_LOGICAL_PORT(portId);
		portGroupLimitVal.maxnum = maxGroup;

		feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_PORT_GROUP_LIMIT_SET, &portGroupLimitVal, sizeof(mcast_portGroupLimit_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, maxgrp=%u", __FUNCTION__, portId, maxGroup);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_immediate_leave_set(UINT16 mopId, UINT16 portId, UINT8 enableB)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId <= gInfo.devCapabilities.ponPort)
	{
		mcast_portFastLeaveMode_t fastLeaveModeVal;
		memset(&fastLeaveModeVal, 0, sizeof(mcast_portFastLeaveMode_t));

		fastLeaveModeVal.port = TO_LOGICAL_PORT(portId);
		fastLeaveModeVal.enable = enableB;
		feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_FASTLEAVE_MODE_SET, &fastLeaveModeVal, sizeof(mcast_portFastLeaveMode_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, port =%u, enable=%u", __FUNCTION__, __LINE__, portId, enableB);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_last_mbr_query_intv_set(UINT16 mopId, UINT32 lastMbrQueryInterval)
{
		mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(mcast_prof_t));
	mcastProfVal.mopId = mopId;
	mcastProfVal.lastMbrQueryIntval = lastMbrQueryInterval;
	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_LASTMBRQUERYINTVAL_SET, &mcastProfVal, sizeof(mcast_prof_t));

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, mopId =%u, lastMbrQueryInterval=%u", __FUNCTION__, __LINE__, mopId, lastMbrQueryInterval);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_acl_set(UINT32 op, UINT32 ipType, UINT32 aclType, UINT16 portId, UINT16 rowKey,
	MIB_TABLE_MCASTOPERPROF_T *pMop, MIB_TABLE_EXTMCASTOPERPROF_T *pExtMop)
{
	struct aclHead *pTmpHead;
	struct extAclHead *pExtTmpHead;

	aclTableEntry_t *ptr = NULL;
	extAclTableEntry_t *ptrExt = NULL;
	UINT8 *pAddrCurr = NULL;
	UINT32 addrRangeVal;

    MCAST_NOP_CHK(portId, GOS_OK);

	if ((0xffff == portId || portId <= gInfo.devCapabilities.ponPort))
	{
		UINT32 id;
		mcast_aclEntry_t aclEntryVal;
		mcast_group_type_t type;
		mcast_msgType_t cmd;
		memset(&aclEntryVal, 0, sizeof(mcast_aclEntry_t));
		/* standard */
		if(pMop)
		{
			aclEntryVal.ipType = (ipType == IP_TYPE_IPV4 ? MULTICAST_TYPE_IPV4 : MULTICAST_TYPE_IPV6);
			cmd = (op == SET_CTRL_WRITE ? MCAST_MSGTYPE_ACLENTRY_SET : MCAST_MSGTYPE_ACLENTRY_DEL);
			type = (aclType == MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX ? IGMP_GROUP_ENTRY_DYNAMIC : IGMP_GROUP_ENTRY_STATIC);
			pTmpHead = (IGMP_GROUP_ENTRY_DYNAMIC == type ? &(pMop->DACLhead) : &(pMop->SACLhead));
			aclEntryVal.aclType = type;
			id = ((pMop->EntityId << 16) | rowKey);
			aclEntryVal.aclEntry.id = id;

			/*  ipType is IPV4:
				(a) only support G.984 without preveiw feature
				(b) support G.988 but the first 10 Bytes are zero and the last 2 Bytes = 0 or 0xFFFF
			*/
			switch (ipType)
			{
				case IP_TYPE_IPV4:
					LIST_FOREACH(ptr, pTmpHead, entries)
					{
						if(ptr->tableEntry.tableCtrl.bit.rowKey == rowKey)
						{
							switch(ptr->tableEntry.tableCtrl.bit.rowPartId)
							{
								case ROW_PART_0:
									aclEntryVal.aclEntry.aniVid = ptr->tableEntry.rowPart.rowPart0.aniVid;
									aclEntryVal.aclEntry.sip.ipv4 = ptr->tableEntry.rowPart.rowPart0.sip;
									aclEntryVal.aclEntry.dipStart.ipv4 = ptr->tableEntry.rowPart.rowPart0.dipStartRange;
									aclEntryVal.aclEntry.dipEnd.ipv4 = ptr->tableEntry.rowPart.rowPart0.dipEndRange;
									aclEntryVal.aclEntry.imputedGrpBW = ptr->tableEntry.rowPart.rowPart0.ImputedGrpBw;
									break;
								case ROW_PART_1:
									aclEntryVal.aclEntry.previewLen = ptr->tableEntry.rowPart.rowPart1.previewLen;
									aclEntryVal.aclEntry.previewRepeatTime= ptr->tableEntry.rowPart.rowPart1.previewRepeatTime;
									aclEntryVal.aclEntry.previewRepeatCnt= ptr->tableEntry.rowPart.rowPart1.previewRepeatCnt;
									aclEntryVal.aclEntry.previewReset = ptr->tableEntry.rowPart.rowPart1.previewResetTime;
									break;
								default:
									break;
							}
						}
					}
					break;
				case IP_TYPE_IPV6:
					// get 3 row part value
					LIST_FOREACH(ptr, pTmpHead, entries)
					{
						if(ptr->tableEntry.tableCtrl.bit.rowKey == rowKey)
						{
							switch(ptr->tableEntry.tableCtrl.bit.rowPartId)
							{
								case ROW_PART_0:
									aclEntryVal.aclEntry.aniVid = ptr->tableEntry.rowPart.rowPart0.aniVid;
									pAddrCurr = aclEntryVal.aclEntry.sip.ipv6;
									pAddrCurr += LEADING_12BYTE_ADDR_LEN;
									memcpy(pAddrCurr, &(ptr->tableEntry.rowPart.rowPart0.sip), sizeof(UINT32));
									pAddrCurr = aclEntryVal.aclEntry.dipStart.ipv6;
									pAddrCurr += LEADING_12BYTE_ADDR_LEN;
									memcpy(pAddrCurr, &(ptr->tableEntry.rowPart.rowPart0.dipStartRange), sizeof(UINT32));
									pAddrCurr = aclEntryVal.aclEntry.dipEnd.ipv6;
									pAddrCurr += LEADING_12BYTE_ADDR_LEN;
									memcpy(pAddrCurr, &(ptr->tableEntry.rowPart.rowPart0.dipEndRange), sizeof(UINT32));
									aclEntryVal.aclEntry.imputedGrpBW = ptr->tableEntry.rowPart.rowPart0.ImputedGrpBw;
									break;
								case ROW_PART_1:
									pAddrCurr = aclEntryVal.aclEntry.sip.ipv6;
									memcpy(pAddrCurr, ptr->tableEntry.rowPart.rowPart1.ipv6Sip, LEADING_12BYTE_ADDR_LEN);
									aclEntryVal.aclEntry.previewLen = ptr->tableEntry.rowPart.rowPart1.previewLen;
									aclEntryVal.aclEntry.previewRepeatTime= ptr->tableEntry.rowPart.rowPart1.previewRepeatTime;
									aclEntryVal.aclEntry.previewRepeatCnt= ptr->tableEntry.rowPart.rowPart1.previewRepeatCnt;
									aclEntryVal.aclEntry.previewReset = ptr->tableEntry.rowPart.rowPart1.previewResetTime;
									break;
								case ROW_PART_2:
									pAddrCurr = aclEntryVal.aclEntry.dipStart.ipv6;
									memcpy(pAddrCurr, ptr->tableEntry.rowPart.rowPart2.ipv6Dip, LEADING_12BYTE_ADDR_LEN);
									pAddrCurr = aclEntryVal.aclEntry.dipEnd.ipv6;
									memcpy(pAddrCurr, ptr->tableEntry.rowPart.rowPart2.ipv6Dip, LEADING_12BYTE_ADDR_LEN);
									break;
								default:
									break;
							}
						}
					}
					break;
				case IP_TYPE_UNKNOWN:
					break;
				default:
					OMCI_PRINT("Not support ");
			}
			if(op == SET_CTRL_WRITE)
			{
				OMCI_LOG(OMCI_LOG_LEVEL_ERR, "ipType=%u", ipType);
                feature_api(FEATURE_API_MCAST_RTK_IPC_SET, cmd, &aclEntryVal, sizeof(mcast_aclEntry_t));

				/* update ACL rule with the related uni port */
				if (0xffff != portId)
				{
					mcast_rtk_profile_per_port_set(op, aclType, portId, id);
				}
			}
			else
			{
				/* update ACL rule with the related uni port */
				if (0xffff != portId)
				{
					mcast_rtk_profile_per_port_set(op, aclType, portId, id);
				}
				feature_api(FEATURE_API_MCAST_RTK_IPC_SET, cmd, &aclEntryVal, sizeof(mcast_aclEntry_t));
			}
			OMCI_LOG(OMCI_LOG_LEVEL_ERR, " %s() %d, port =%u, aclType=%u,	mopId=%u, key=%u",
				__FUNCTION__, __LINE__, portId, aclType, pMop->EntityId, rowKey);
			switch (aclEntryVal.ipType)
			{
				case MULTICAST_TYPE_IPV4:
					OMCI_LOG(OMCI_LOG_LEVEL_DBG, "id=%x, dipStart="IPADDR_PRINT", dipEnd="IPADDR_PRINT"",
						id, IPADDR_PRINT_ARG(aclEntryVal.aclEntry.dipStart.ipv4), IPADDR_PRINT_ARG( aclEntryVal.aclEntry.dipEnd.ipv4));
					break;
				case MULTICAST_TYPE_IPV6:
					OMCI_LOG(OMCI_LOG_LEVEL_DBG, "id=%x, dipStart="IPADDRV6_PRINT", dipEnd="IPADDRV6_PRINT"",
						id, IPADDRV6_PRINT_ARG(aclEntryVal.aclEntry.dipStart.ipv6), IPADDRV6_PRINT_ARG( aclEntryVal.aclEntry.dipEnd.ipv6));
					break;
				default:
					OMCI_PRINT("Not support");
			}
		}
		else if(pExtMop) /* private without source filtering function */
		{
			cmd = (op == SET_CTRL_WRITE ? MCAST_MSGTYPE_ACLENTRY_SET : MCAST_MSGTYPE_ACLENTRY_DEL);
			type = (aclType == MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX ? IGMP_GROUP_ENTRY_DYNAMIC : IGMP_GROUP_ENTRY_STATIC);
			pExtTmpHead = (IGMP_GROUP_ENTRY_DYNAMIC == type ? &(pExtMop->DACLhead) : &(pExtMop->SACLhead));
			aclEntryVal.aclType = type;
			id = ((pExtMop->EntityId << 16) | rowKey);
			aclEntryVal.aclEntry.id = id;

			LIST_FOREACH(ptrExt, pExtTmpHead, entries)
			{
				if(ptrExt->tableEntry.tableCtrl.bit.rowKey == rowKey)
				{
					switch(ptrExt->tableEntry.tableCtrl.bit.rowPartId)
					{
						case ROW_PART_0:
							aclEntryVal.aclEntry.aniVid = ptrExt->tableEntry.row.format0.aniVid;
							aclEntryVal.aclEntry.imputedGrpBW = ptrExt->tableEntry.row.format0.ImputedGrpBw;
							aclEntryVal.aclEntry.previewLen = ptrExt->tableEntry.row.format0.previewLen;
							aclEntryVal.aclEntry.previewRepeatTime= ptrExt->tableEntry.row.format0.previewRepeatTime;
							aclEntryVal.aclEntry.previewRepeatCnt= ptrExt->tableEntry.row.format0.previewRepeatCnt;
							aclEntryVal.aclEntry.previewReset = ptrExt->tableEntry.row.format0.previewResetTime;
							//TBD: vendorSpecificUse
							break;
						case ROW_PART_1:
							pAddrCurr = ptrExt->tableEntry.row.format1.dstIpAddrStartRange;//low address is put msb
							memcpy(&addrRangeVal, pAddrCurr, sizeof(UINT32));
							if(addrRangeVal)
							{
								//ipv6
								aclEntryVal.ipType = MULTICAST_TYPE_IPV6;
								memcpy(aclEntryVal.aclEntry.dipStart.ipv6, ptrExt->tableEntry.row.format1.dstIpAddrStartRange, IPV6_ADDR_LEN);
							}
							else
							{
								//ipv4
								aclEntryVal.ipType = MULTICAST_TYPE_IPV4;
								pAddrCurr = ptrExt->tableEntry.row.format1.dstIpAddrStartRange;
								pAddrCurr += LEADING_12BYTE_ADDR_LEN;
								memcpy(&addrRangeVal, pAddrCurr, sizeof(UINT32));
								aclEntryVal.aclEntry.dipStart.ipv4 = addrRangeVal;
							}
							break;
						case ROW_PART_2:
							pAddrCurr = ptrExt->tableEntry.row.format2.dstIpAddrEndRange;//low address is put msb
							memcpy(&addrRangeVal, pAddrCurr, sizeof(UINT32));
							if(addrRangeVal)
							{
								//ipv6
								aclEntryVal.ipType = MULTICAST_TYPE_IPV6;
								memcpy(aclEntryVal.aclEntry.dipEnd.ipv6, ptrExt->tableEntry.row.format2.dstIpAddrEndRange, IPV6_ADDR_LEN);
							}
							else
							{
								//ipv4
								aclEntryVal.ipType = MULTICAST_TYPE_IPV4;
								pAddrCurr = ptrExt->tableEntry.row.format2.dstIpAddrEndRange;
								pAddrCurr += LEADING_12BYTE_ADDR_LEN;
								memcpy(&addrRangeVal, pAddrCurr, sizeof(UINT32));
								aclEntryVal.aclEntry.dipEnd.ipv4 = addrRangeVal;
							}
							break;
						default:
							break;
					}
				}
			}

			if(op == SET_CTRL_WRITE)
			{
                feature_api(FEATURE_API_MCAST_RTK_IPC_SET, cmd, &aclEntryVal, sizeof(mcast_aclEntry_t));

				/* update ACL rule with the related uni port */
				if (0xffff != portId)
				{
					mcast_rtk_profile_per_port_set(op, aclType, portId, id);
				}
			}
			else
			{
				/* update ACL rule with the related uni port */
				if (0xffff != portId)
				{
					mcast_rtk_profile_per_port_set(op, aclType, portId, id);
				}

				feature_api(FEATURE_API_MCAST_RTK_IPC_SET, cmd, &aclEntryVal, sizeof(mcast_aclEntry_t));
			}
			OMCI_LOG(OMCI_LOG_LEVEL_ERR, " %s() %d, port =%u, aclType=%u,	mopId=%u, key=%u",
				__FUNCTION__, __LINE__, portId, aclType, pExtMop->EntityId, rowKey);
			switch (aclEntryVal.ipType)
			{
				case MULTICAST_TYPE_IPV4:
					OMCI_LOG(OMCI_LOG_LEVEL_DBG, "id=%x, dipStart="IPADDR_PRINT", dipEnd="IPADDR_PRINT"",
						id, IPADDR_PRINT_ARG(aclEntryVal.aclEntry.dipStart.ipv4), IPADDR_PRINT_ARG( aclEntryVal.aclEntry.dipEnd.ipv4));
					break;
				case MULTICAST_TYPE_IPV6:
					OMCI_LOG(OMCI_LOG_LEVEL_DBG, "id=%x, dipStart="IPADDRV6_PRINT", dipEnd="IPADDRV6_PRINT"",
						id, IPADDRV6_PRINT_ARG(aclEntryVal.aclEntry.dipStart.ipv6), IPADDRV6_PRINT_ARG( aclEntryVal.aclEntry.dipEnd.ipv6));
					break;
				default:
					OMCI_PRINT("Not support");
			}
		}
	}
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_un_auth_join_bhvr_set(UINT16 mopId, UINT8 unAuthJoinBhvr)
{
	mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(mcast_prof_t));
	mcastProfVal.mopId = mopId;
	mcastProfVal.unAuthJoinRqtBhvr = unAuthJoinBhvr;
	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_UNAUTHJOINBHVR_SET, &mcastProfVal, sizeof(mcast_prof_t));

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, mopId =%u, unAuthJoinBhvr=%u", __FUNCTION__, __LINE__, mopId, unAuthJoinBhvr);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_prof_del(UINT16 mopId)
{
	mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(mcast_prof_t));
	mcastProfVal.mopId = mopId;
	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_MCASTPROF_DEL, &mcastProfVal, sizeof(mcast_prof_t));

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, mopId =%u, ", __FUNCTION__, __LINE__, mopId);
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rtk_us_igmp_rate_set(UINT16 mopId, UINT16 portId, UINT32 usIgmpRate)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId <= gInfo.devCapabilities.ponPort)
	{
		mcast_igmpRateLimit_t rateLimitVal;
		memset(&rateLimitVal, 0, sizeof(mcast_igmpRateLimit_t));

		rateLimitVal.port = TO_LOGICAL_PORT(portId);
		rateLimitVal.packetRate = usIgmpRate;
		feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_IGMP_RATELIMIT_SET, &rateLimitVal, sizeof(mcast_igmpRateLimit_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, port =%u, usIgmpRate=%u", __FUNCTION__, __LINE__, portId, usIgmpRate);
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rtk_ds_igmp_mcast_tci_set(UINT16 mopId, UINT8 *pDsIgmpMcastTci)
{
	mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(mcast_prof_t));
	mcastProfVal.mopId = mopId;
	memcpy(mcastProfVal.dsIgmpTci, pDsIgmpMcastTci, sizeof(UINT8) * 3);
	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_DSIGMPMCASTTCI_SET, &mcastProfVal, sizeof(mcast_prof_t));

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, mopId =%u, dsIgmpMcastTci=%p", __FUNCTION__, __LINE__, mopId, pDsIgmpMcastTci);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_igmp_fun_set(UINT16 mopId, UINT16 portId, UINT8 mode)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId <= gInfo.devCapabilities.ponPort)
	{
		mcast_portIgmpMode_t igmpModeVal;
		igmp_mode_t val;
		memset(&igmpModeVal, 0, sizeof(mcast_portIgmpMode_t));
		switch(mode)
		{
			case IGMP_SNP:
				val = IGMP_MODE_SNOOPING;
				break;
			case IGMP_SPR:
				val = IGMP_MODE_SPR;
				break;
			case IGMP_PROXY:
				val = IGMP_MODE_PROXY;
				break;
			default:
				val = IGMP_MODE_SNOOPING;
		}
		igmpModeVal.mode = val;
		igmpModeVal.port = TO_LOGICAL_PORT(portId);
        feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_IGMP_MODE_SET, &igmpModeVal, sizeof(mcast_portIgmpMode_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, port =%u, mode=%u", __FUNCTION__, __LINE__, portId, mode);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_igmp_ctrl_pkt_set(UINT16 portId, UINT8 enableB)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId <= gInfo.devCapabilities.ponPort)
	{
		mcast_igmpTrapAct_t igmpTrapVal;
		memset(&igmpTrapVal, 0, sizeof(mcast_igmpTrapAct_t));

		igmpTrapVal.enable = enableB;
		igmpTrapVal.port = TO_LOGICAL_PORT(portId);
		feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MACST_MSGTYPE_IGMPTRAP_SET, &igmpTrapVal, sizeof(mcast_igmpTrapAct_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s()  port =%u, trapEnable=%u", __FUNCTION__, portId, enableB);
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rtk_robustness_set(UINT16 mopId, UINT8 robustness)
{
	mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(mcast_prof_t));
	mcastProfVal.mopId = mopId;
	mcastProfVal.robustness = robustness;
	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_ROBUSTNESS_SET, &mcastProfVal, sizeof(mcast_prof_t));

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() %d, mopId =%u, robustness=%u", __FUNCTION__, __LINE__, mopId, robustness);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_querier_ip_addr_set(UINT16 mopId, void *pQuerierIp, UINT32 size)
{

	mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(mcast_prof_t));
	mcastProfVal.mopId = mopId;
	if(size == sizeof(mcastProfVal.querierIpAddr))
		memcpy(&(mcastProfVal.querierIpAddr), pQuerierIp, size);//mcastProfVal.querierIpAddr = querierIp;
	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_QUERIERIP_SET, &mcastProfVal, sizeof(mcast_prof_t));

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, mopId =%u, querierIp=%p", __FUNCTION__, __LINE__, mopId, pQuerierIp);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_query_intv_set(UINT16 mopId, UINT32 queryInterval)
{
	mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(mcast_prof_t));
	mcastProfVal.mopId = mopId;
	mcastProfVal.queryIntval = queryInterval;
	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_QUERYINTVAL_SET, &mcastProfVal, sizeof(mcast_prof_t));

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, mopId =%u, queryInterval=%u", __FUNCTION__, __LINE__, mopId, queryInterval);
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rtk_query_max_rsp_time_set(UINT16 mopId, UINT32 queryMaxResponseTime)
{

	mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(mcast_prof_t));
	mcastProfVal.mopId = mopId;
	mcastProfVal.queryMaxRspTime = queryMaxResponseTime;
	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_QUERYMAXRSPTIME_SET, &mcastProfVal, sizeof(mcast_prof_t));

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() %d, mopId =%u, queryMaxResponseTime=%u", __FUNCTION__, __LINE__, mopId, queryMaxResponseTime);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_port_flow_set(UINT32                       op,
                                       int							i,
                                       MIB_TABLE_MCASTOPERPROF_T	*pMibMop,
                                       OMCI_BRIDGE_RULE_ts			*pBridgeRule)
{
	mcast_port_info_t portInfoVal;
	mcast_msgType_t cmd;

    MCAST_NOP_CHK(i, GOS_OK);

	memset(&portInfoVal, 0, sizeof(mcast_port_info_t));

    portInfoVal.inTag1.tagOp.tagFilter  = IGMP_NO_FILTER;
    portInfoVal.inTag2.tagOp.tagFilter  = IGMP_NO_FILTER;
    portInfoVal.inTag1.tagOp.tagAct     = IGMP_TRANSPARENT;
    portInfoVal.inTag2.tagOp.tagAct     = IGMP_TRANSPARENT;
    portInfoVal.inTag1.tpid             = 0xFFFF;
    portInfoVal.inTag2.tpid             = 0xFFFF;
    portInfoVal.outTag1.tpid            = 0xFFFF;
    portInfoVal.outTag2.tpid            = 0xFFFF;
    portInfoVal.inTag1.etype            = 0xFFFF;
    portInfoVal.inTag2.etype            = 0xFFFF;
    portInfoVal.outTag1.etype           = 0xFFFF;
    portInfoVal.outTag2.etype           = 0xFFFF;
	portInfoVal.mopId                   = pMibMop->EntityId;
	portInfoVal.vidUni                  = 0xFFFF;
	portInfoVal.usFlowId                = pBridgeRule->usFlowId;
    portInfoVal.uniPort                 = (unsigned int)(TO_LOGICAL_PORT(i));

	mcast_rtk_fill_filtering_info(&portInfoVal, pBridgeRule);
	//  TBD: svlan tpid is 88a8,
	mcast_rtk_fill_treatment_info(&portInfoVal, pMibMop, pBridgeRule);

    cmd = (op == SET_CTRL_WRITE ? MCAST_MSGTYPE_PORT_FLOW_INFO_ADD : MCAST_MSGTYPE_PORT_FLOW_INFO_DEL);

	feature_api(FEATURE_API_MCAST_RTK_IPC_SET, cmd, &portInfoVal, sizeof(mcast_port_info_t));

	OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "port =%u,  usFlowId=%d, mopId=%u ", i, pBridgeRule->usFlowId, pMibMop->EntityId);
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rtk_allowed_preview_set(UINT32 op, UINT16 portId, UINT16 rowKey,
	MIB_TABLE_MCASTSUBCONFINFO_T* pMibMcastSubCfgInfo)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	allowPreviewGrpTblEntry_t *ptr = NULL;
	if (portId <= gInfo.devCapabilities.ponPort)
	{
		mcast_allowed_preview_entry_t allowPreviewVal;
		mcast_msgType_t cmd;
		UINT32 id;
		memset(&allowPreviewVal, 0, sizeof(mcast_allowed_preview_entry_t));
		cmd = (op == SET_CTRL_WRITE ? MCAST_MSGTYPE_ALLOWED_PREVIEW_SET : MCAST_MSGTYPE_ALLOWED_PREVIEW_DEL);
		id = (((TO_LOGICAL_PORT(portId)) << 16) | rowKey);
		allowPreviewVal.id = id;
		LIST_FOREACH(ptr, &pMibMcastSubCfgInfo->allowPreviewGrpHead, entries)
		{
			if(ptr->tableEntry.tableCtrl.bit.rowKey == rowKey)
			{
				switch(ptr->tableEntry.tableCtrl.bit.rowPart)
				{
					case ROW_PART_0:
						memcpy(allowPreviewVal.srcIpAddr, ptr->tableEntry.row.part0.srcIpAddr, MIB_TABLE_PREVIEWGRPIPADDR_LEN);
						allowPreviewVal.aniVid = ptr->tableEntry.row.part0.vidAni;
						allowPreviewVal.uniVid = ptr->tableEntry.row.part0.vidUni;
						break;
					case ROW_PART_1:
						memcpy(allowPreviewVal.dstIpAddr, ptr->tableEntry.row.part1.dstIpAddr, MIB_TABLE_PREVIEWGRPIPADDR_LEN);
						allowPreviewVal.duration = ptr->tableEntry.row.part1.duration;
						allowPreviewVal.timeLeft = ptr->tableEntry.row.part1.timeleft;
						break;
					default:
						break;
				}
			}
		}
		feature_api(FEATURE_API_MCAST_RTK_IPC_SET, cmd, &allowPreviewVal, sizeof(mcast_allowed_preview_entry_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_ERR, "port =%u, rowKey=%u ", portId, rowKey);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_max_bw_set(UINT16 portId, UINT32 bw)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId != gInfo.devCapabilities.ponPort && portId != 0xffff)
	{
		mcast_portBwInfo_t portBwInfoVal;
		memset(&portBwInfoVal, 0, sizeof(mcast_portBwInfo_t));

		portBwInfoVal.port = TO_LOGICAL_PORT(portId);
		portBwInfoVal.maxBw = bw;

		feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_PORT_MAX_MCAST_BW_SET, &portBwInfoVal, sizeof(mcast_portBwInfo_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, maxBw=%u", __FUNCTION__, portId, bw);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_bw_enforce_set(UINT16 portId, UINT8 enforceB)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId != gInfo.devCapabilities.ponPort && portId != 0xffff)
	{
		mcast_portBwInfo_t portBwInfoVal;
		memset(&portBwInfoVal, 0, sizeof(mcast_portBwInfo_t));

		portBwInfoVal.port = TO_LOGICAL_PORT(portId);
		portBwInfoVal.bwEnforceB = enforceB;

		feature_api(FEATURE_API_MCAST_RTK_IPC_SET, MCAST_MSGTYPE_PORT_BW_ENFORCE_SET, &portBwInfoVal, sizeof(mcast_portBwInfo_t));
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, BwEnforceB=%u", __FUNCTION__, portId, enforceB);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_curr_bw_get(UINT16 portId, UINT32 *pCurrBw)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId != gInfo.devCapabilities.ponPort && portId != 0xffff)
	{
		mcast_portStat_t portStatVal;
		memset(&portStatVal, 0, sizeof(mcast_portStat_t));

		portStatVal.port = TO_LOGICAL_PORT(portId);

		feature_api(FEATURE_API_MCAST_RTK_IPC_GET, MCAST_MSGTYPE_PORT_CURR_BW_GET, &portStatVal, sizeof(mcast_portStat_t));

		*pCurrBw = portStatVal.currBw;

		OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, cur bw=%u", __FUNCTION__, portId, portStatVal.currBw);
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, cur bw=%u", __FUNCTION__, portId, *pCurrBw);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_join_count_get(UINT16 portId, UINT32 *pJoinCount)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId != gInfo.devCapabilities.ponPort && portId != 0xffff)
	{
		mcast_portStat_t portStatVal;
		memset(&portStatVal, 0, sizeof(mcast_portStat_t));

		portStatVal.port = TO_LOGICAL_PORT(portId);

		feature_api(FEATURE_API_MCAST_RTK_IPC_GET, MCAST_MSGTYPE_PORT_JOIN_COUNT_GET, &portStatVal, sizeof(mcast_portStat_t));

		*pJoinCount = portStatVal.joinCount;

		OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, join count=%u", __FUNCTION__, portId, portStatVal.joinCount);
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, join count=%u", __FUNCTION__, portId, *pJoinCount);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_exceed_bw_count_get(UINT16 portId, UINT32 *pExcdBwCount)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId != gInfo.devCapabilities.ponPort && portId != 0xffff)
	{
		mcast_portStat_t portStatVal;
		memset(&portStatVal, 0, sizeof(mcast_portStat_t));

		portStatVal.port = TO_LOGICAL_PORT(portId);

		feature_api(FEATURE_API_MCAST_RTK_IPC_GET, MCAST_MSGTYPE_PORT_BW_EXCEED_COUNT_GET, &portStatVal, sizeof(mcast_portStat_t));

		*pExcdBwCount = portStatVal.bwExcdCount;

		OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, join count=%u", __FUNCTION__, portId, portStatVal.bwExcdCount);
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, join count=%u", __FUNCTION__, portId, *pExcdBwCount);
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rtk_active_group_get(MIB_ATTR_INDEX attrIndex, UINT16 portId,
	MIB_TABLE_MCASTSUBMONITOR_T *pMSM, UINT32 *pCurActiveGrpCnt)
{
    MCAST_NOP_CHK(portId, GOS_OK);

	if (portId != gInfo.devCapabilities.ponPort && portId != 0xffff)
	{
		mcast_portStat_t portStatVal;
		void *shm, *p;
		int shmId;
		UINT32 idx;

		memset(&portStatVal, 0, sizeof(mcast_portStat_t));
		portStatVal.port = TO_LOGICAL_PORT(portId);

		if (MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX == attrIndex)
		{
            feature_api(FEATURE_API_MCAST_RTK_IPC_GET, MCAST_MSGTYPE_PORT_IPV4_ACTIVE_GRP_COUNT_GET, &portStatVal, sizeof(mcast_portStat_t));

            *pCurActiveGrpCnt = portStatVal.ipv4ActiveGrpNum;

			/*Get pointer of share memory*/
			feature_api(FEATURE_API_MCAST_RTK_IPC_CREATE, MCAST_MSGTYPE_PORT_IPV4_ACTIVE_GRP_COUNT_GET, &shm, &shmId);
			p = shm;

			//insert to pMSM->IPv4ActiveGroupListTable_head from shared memory of IGMP
			for (idx = 0; idx < portStatVal.ipv4ActiveGrpNum; idx++)
			{
				msm_attr_ipv4_active_group_list_table_entry_t *pNew =
					(msm_attr_ipv4_active_group_list_table_entry_t *)malloc(sizeof(msm_attr_ipv4_active_group_list_table_entry_t));
				if(pNew)
				{
					memcpy(&(pNew->tableEntry), p, sizeof(msm_attr_ipv4_active_group_list_table_t));
					p += sizeof(msm_attr_ipv4_active_group_list_table_t);
					LIST_INSERT_HEAD(&pMSM->IPv4ActiveGroupListTable_head, pNew, entries);
				}
			}

			/*destroy share memory*/
			feature_api(FEATURE_API_MCAST_RTK_IPC_DESTROY, shmId, shm);

			OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, ipv4 active group num=%u", __FUNCTION__, portId, portStatVal.ipv4ActiveGrpNum);
		}
		else if (MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX == attrIndex)
		{
            feature_api(FEATURE_API_MCAST_RTK_IPC_GET, MCAST_MSGTYPE_PORT_IPV6_ACTIVE_GRP_COUNT_GET, &portStatVal, sizeof(mcast_portStat_t));

			*pCurActiveGrpCnt = portStatVal.ipv6ActiveGrpNum;

			//TBD
			//insert to pMSM->IPv6ActiveGroupListTable_head from shared memory of IGMP

			OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, ipv6 active group num=%u", __FUNCTION__, portId, portStatVal.ipv6ActiveGrpNum);
		}
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, " %s() port =%u, join count=%u", __FUNCTION__, portId, *pCurActiveGrpCnt);
	return GOS_OK;
}


omci_mcast_wrapper_t rtk_snp_wrapper =
{
	/* reset */
    .omci_config_init                            = mcast_rtk_clear_all,
    /* set */
    .omci_max_simultaneous_groups_set            = mcast_rtk_max_sim_group_set,
    .omci_immediate_leave_set                    = mcast_rtk_immediate_leave_set,
    .omci_last_member_query_interval_set         = mcast_rtk_last_mbr_query_intv_set,
    .omci_dynamic_acl_table_entry_set            = mcast_rtk_acl_set,
    .omci_unauthorized_join_behaviour_set        = mcast_rtk_un_auth_join_bhvr_set,
    .omci_mop_profile_add                        = NULL,
    .omci_mop_profile_del                        = mcast_rtk_prof_del,
    .omci_mop_profile_per_port_set               = NULL,                             //RG
    .omci_acl_per_port_set                       = mcast_rtk_profile_per_port_set,   //RTK
    .omci_us_igmp_rate_set                       = mcast_rtk_us_igmp_rate_set,
    .omci_ds_igmp_multicast_tci_set              = mcast_rtk_ds_igmp_mcast_tci_set,
    .omci_us_igmp_tag_info_set                   = NULL,
    .omci_igmp_function_set                      = mcast_rtk_igmp_fun_set,
    .omci_ctrl_pkt_behaviour_set                 = mcast_rtk_igmp_ctrl_pkt_set,
    .omci_robustness_set                         = mcast_rtk_robustness_set,
    .omci_querier_ip_addr_set                    = mcast_rtk_querier_ip_addr_set,
    .omci_query_interval_set                     = mcast_rtk_query_intv_set,
    .omci_query_max_response_time_set            = mcast_rtk_query_max_rsp_time_set,
    .omci_connection_rule_set                    = mcast_rtk_port_flow_set,
    .omci_allowed_preview_groups_table_entry_set = mcast_rtk_allowed_preview_set,
    .omci_max_multicast_bandwidth_set            = mcast_rtk_max_bw_set,
    .omci_bandwidth_enforcement_set              = mcast_rtk_bw_enforce_set,
    .omci_igmp_version_set                       = NULL,
    /* get */
    .omci_current_multicast_bandwidth_get        = mcast_rtk_curr_bw_get,
    .omci_join_message_counter_get               = mcast_rtk_join_count_get,
    .omci_bandwidth_exceeded_counter_get         = mcast_rtk_exceed_bw_count_get,
    .omci_active_group_list_table_get            = mcast_rtk_active_group_get,
};
