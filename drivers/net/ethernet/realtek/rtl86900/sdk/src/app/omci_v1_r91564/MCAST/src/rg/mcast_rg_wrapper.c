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
 * Purpose : Definition callback API for rg snooping mode
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1)
 */

/****************************************************************/
/* Include Files                                                */
/****************************************************************/
#ifndef OMCI_X86
#include "rtk/gponv2.h"
#include <rtk/rtusr/include/rtusr_util.h>
#include <rtdrv/rtdrv_netfilter.h>
#endif
#include "mcast_mgmt.h"
#include "feature_mgmt.h"

#include "omci_driver_api.h"

#define MCAST_RG_LOG_LVL OMCI_LOG_LEVEL_ERR


#define IPV4_ENTRY_COUNT_PER_GET    (5)

GOS_ERROR_CODE mcast_rg_clear_all(void)
{
#ifndef OMCI_X86
    feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_RESET_ALL, NULL, 0);
#endif
	OMCI_LOG(MCAST_RG_LOG_LVL, " %s(), cmd=%u", __FUNCTION__, OMCI_DM_MCAST_CMD_RESET_ALL);
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rg_igmp_port_enable_set(UINT16 portId, UINT8 enableB)
{
#ifndef OMCI_X86
	omci_dm_mcast_port_Info_t portInfo;

	memset(&portInfo, 0, sizeof(omci_dm_mcast_port_Info_t));
	portInfo.portId = portId;
	portInfo.SnoopingEnable = enableB;

	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_SNOOP_ENABLE_SET, &portInfo, sizeof(omci_dm_mcast_port_Info_t));

#endif
	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, portId=%u, SnoopingEnable=%u",
	    __FUNCTION__, OMCI_DM_MCAST_CMD_SNOOP_ENABLE_SET, portInfo.portId, portInfo.SnoopingEnable);

    return GOS_OK;
}


GOS_ERROR_CODE mcast_rg_prof_add(UINT16 mopId)
{
#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId= mopId;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_PROFILE_ADD, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));
#endif
	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_PROFILE_ADD,mcastProfVal.profileId);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_prof_del(UINT16 mopId)
{
#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId = mopId;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_PROFILE_DEL, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));
#endif
	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_PROFILE_DEL,mcastProfVal.profileId);

	return GOS_OK;
}


GOS_ERROR_CODE mcast_rg_igmp_function_set(UINT16 mopId, UINT16 portId, UINT8 func)
{
#ifndef OMCI_X86
    omci_dm_mcast_igmpMode_t val;
	omci_dm_mcast_prof_t mcastProfVal;
	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));

    switch(func)
    {
    	case IGMP_SNP:
    		val = MCAST_IGMP_MODE_SNOOPING;
    		break;
    	case IGMP_SPR:
    		val = MCAST_IGMP_MODE_SPR;
    		break;
    	case IGMP_PROXY:
    		val = MCAST_IGMP_MODE_PROXY;
    		break;
    	default:
    		val = MCAST_IGMP_MODE_SNOOPING;
    }
	mcastProfVal.profileId = mopId;
	mcastProfVal.IGMPFun   = val;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_SNOOP_ENABLE_SET, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));
#endif
	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u, IGMPFun=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_SNOOP_ENABLE_SET, mcastProfVal.profileId, mcastProfVal.IGMPFun);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_igmp_version_set(UINT16 mopId, UINT16 portId, UINT8 ver)
{

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s() , cmd=%u, profileId=%u, version=%u", 
        __FUNCTION__, OMCI_DM_MCAST_CMD_IGMP_VERSION_SET,  mopId, ver);

    return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_immediate_leave_set(UINT16 mopId, UINT16 portId, UINT8 enableB)
{
#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId      = mopId;
	mcastProfVal.ImmediateLeave = enableB;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_FAST_LEAVE_SET, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));

#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u, ImmediateLeave=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_FAST_LEAVE_SET, mcastProfVal.profileId, mcastProfVal.ImmediateLeave);

	return GOS_OK;
}


GOS_ERROR_CODE mcast_rg_us_igmp_rate_set(UINT16 mopId, UINT16 portId, UINT32 usIgmpRate)
{

#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId  = mopId;
	mcastProfVal.UsIGMPRate = usIgmpRate;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_US_IGMP_RATE_SET, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));

#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u, UsIGMPRate=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_US_IGMP_RATE_SET, mcastProfVal.profileId, mcastProfVal.UsIGMPRate);
    
	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_max_sim_group_set(UINT16 portId, UINT16 maxGroup)
{
#ifndef OMCI_X86
	omci_dm_mcast_port_Info_t portInfo;

	memset(&portInfo, 0, sizeof(omci_dm_mcast_port_Info_t));
	portInfo.portId = portId;
	portInfo.portMaxGrpNum = maxGroup;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_MAX_GRP_NUM_SET, &portInfo, sizeof(omci_dm_mcast_port_Info_t));
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(),cmd=%u, portId=%u, maxGrpNum=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_MAX_GRP_NUM_SET, portInfo.portId, portInfo.portMaxGrpNum);
    
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rg_robustness_set(UINT16 mopId, UINT8 robustness)
{
#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId = mopId;
	mcastProfVal.robustness = robustness;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_ROBUSTNESS_SET, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u, robustness=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_ROBUSTNESS_SET, mcastProfVal.profileId, mcastProfVal.robustness);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_query_intv_set(UINT16 mopId, UINT32 queryInterval)
{
#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId = mopId;
	mcastProfVal.queryIntval = queryInterval;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_QUERY_INTERVAL_SET, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u, queryIntval=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_QUERY_INTERVAL_SET, mcastProfVal.profileId, mcastProfVal.queryIntval);
    
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rg_query_max_rsp_time_set(UINT16 mopId, UINT32 queryMaxResponseTime)
{
#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId = mopId;
	mcastProfVal.queryMaxRspTime = queryMaxResponseTime;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_QUERY_MAX_RESPONSE_TIME_SET, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));
#endif
	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u, queryMaxRspTime=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_QUERY_MAX_RESPONSE_TIME_SET, mcastProfVal.profileId, mcastProfVal.queryMaxRspTime);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_last_mbr_query_intv_set(UINT16 mopId, UINT32 lastMbrQueryInterval)
{
#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId = mopId;
	mcastProfVal.lastMbrQueryIntval = lastMbrQueryInterval;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_LAST_MBR_QUERY_INTERVAL_SET, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u, lastMbrQueryIntval=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_LAST_MBR_QUERY_INTERVAL_SET, mcastProfVal.profileId, mcastProfVal.lastMbrQueryIntval);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_acl_set(UINT32 op, UINT32 ipType, UINT32 aclType, UINT16 portId, UINT16 rowKey,
	MIB_TABLE_MCASTOPERPROF_T *pMop, MIB_TABLE_EXTMCASTOPERPROF_T *pExtMop)
{
    struct aclHead *pTmpHead;
    struct extAclHead *pExtTmpHead;
    aclTableEntry_t *ptr = NULL;
    extAclTableEntry_t *ptrExt = NULL;
    UINT8 *pAddrCurr = NULL;
    UINT32 addrRangeVal;
    omci_dm_mcast_acl_entry_t mcastAclEntry;
    omci_dm_mcast_cmd_t mcastCmd;

	memset(&mcastAclEntry, 0, sizeof(omci_dm_mcast_acl_entry_t));
    
	if(pMop)
	{
#ifndef OMCI_X86
        mcastCmd = (op == SET_CTRL_WRITE ? OMCI_DM_MCAST_CMD_DYNAMIC_ACL_SET : OMCI_DM_MCAST_CMD_DYNAMIC_ACL_DEL);
        mcastAclEntry.profileId = pMop->EntityId;
        mcastAclEntry.aclTableEntryId = rowKey;
   
        if(aclType == MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX)
        {
            pTmpHead = &(pMop->DACLhead);
            LIST_FOREACH(ptr, pTmpHead, entries)
            {
                if(ptr->tableEntry.tableCtrl.bit.rowKey == rowKey)
                {
                    switch(ptr->tableEntry.tableCtrl.bit.rowPartId)
                    {
                        case ROW_PART_0:
                            mcastAclEntry.aniVid        =  ptr->tableEntry.rowPart.rowPart0.aniVid;
                            mcastAclEntry.imputedGrpBw  =  ptr->tableEntry.rowPart.rowPart0.ImputedGrpBw;
                            if(IP_TYPE_IPV4 == ipType)
                            {
                                mcastAclEntry.sip.ipv4      =  ptr->tableEntry.rowPart.rowPart0.sip;
                                mcastAclEntry.dipStart.ipv4 =  ptr->tableEntry.rowPart.rowPart0.dipStartRange;
                                mcastAclEntry.dipEnd.ipv4   =  ptr->tableEntry.rowPart.rowPart0.dipEndRange;
                            }
                            else    /*IPv6*/
                            {
								pAddrCurr = mcastAclEntry.sip.ipv6;
								pAddrCurr += LEADING_12BYTE_ADDR_LEN;
								memcpy(pAddrCurr, &(ptr->tableEntry.rowPart.rowPart0.sip), sizeof(UINT32));
								pAddrCurr = mcastAclEntry.dipStart.ipv6;
								pAddrCurr += LEADING_12BYTE_ADDR_LEN;
								memcpy(pAddrCurr, &(ptr->tableEntry.rowPart.rowPart0.dipStartRange), sizeof(UINT32));
								pAddrCurr = mcastAclEntry.dipEnd.ipv6;
								pAddrCurr += LEADING_12BYTE_ADDR_LEN;
								memcpy(pAddrCurr, &(ptr->tableEntry.rowPart.rowPart0.dipEndRange), sizeof(UINT32));
                            }                              
                            break;
                        case ROW_PART_1:
                            mcastAclEntry.previewLen           =  ptr->tableEntry.rowPart.rowPart1.previewLen;
                            mcastAclEntry.previewRepeatTime    =  ptr->tableEntry.rowPart.rowPart1.previewRepeatTime;
                            mcastAclEntry.previewRepeatCnt     =  ptr->tableEntry.rowPart.rowPart1.previewRepeatCnt;
                            mcastAclEntry.previewResetTime     =  ptr->tableEntry.rowPart.rowPart1.previewResetTime;
                            if(IP_TYPE_IPV6 == ipType)
                            {
                                memcpy(mcastAclEntry.sip.ipv6, ptr->tableEntry.rowPart.rowPart1.ipv6Sip, LEADING_12BYTE_ADDR_LEN);
                            }
                            break;
                        case ROW_PART_2:
                            if(IP_TYPE_IPV6 == ipType)
                            {
                                memcpy(mcastAclEntry.dipStart.ipv6, ptr->tableEntry.rowPart.rowPart1.ipv6Sip, LEADING_12BYTE_ADDR_LEN);
                                memcpy(mcastAclEntry.dipEnd.ipv6, ptr->tableEntry.rowPart.rowPart1.ipv6Sip, LEADING_12BYTE_ADDR_LEN);
                            }
                            break;
                        default:
                            break;
                    }
                    }
                }


		}
		else
		{
			//TBD static ACL rule
			pTmpHead = &(pMop->SACLhead);

		}
	    feature_api(
            FEATURE_API_MCAST_RG_IGMP_SET, 
            mcastCmd, 
            &mcastAclEntry, 
            sizeof(omci_dm_mcast_acl_entry_t));
        
		OMCI_LOG(MCAST_RG_LOG_LVL, " %s() %d, port=%u, aclType=%u,	mopId=%u, key=%u",
			__FUNCTION__, __LINE__, portId, aclType, pMop->EntityId, rowKey);
		switch (ipType)
		{
			case IP_TYPE_IPV4:
				OMCI_LOG(MCAST_RG_LOG_LVL, "id=%x, dipStart="IPADDR_PRINT", dipEnd="IPADDR_PRINT"",
					mcastAclEntry.aclTableEntryId,
					IPADDR_PRINT_ARG(mcastAclEntry.dipStart.ipv4),
					IPADDR_PRINT_ARG(mcastAclEntry.dipEnd.ipv4));
				break;
			case IP_TYPE_IPV6:
				OMCI_LOG(MCAST_RG_LOG_LVL, "id=%x, dipStart="IPADDRV6_PRINT", dipEnd="IPADDRV6_PRINT"",
					mcastAclEntry.aclTableEntryId,
					IPADDRV6_PRINT_ARG(mcastAclEntry.dipStart.ipv6),
					IPADDRV6_PRINT_ARG(mcastAclEntry.dipEnd.ipv6));
				break;
			default:
				OMCI_PRINT("Not support");
	    }
#endif
	}


	else if(pExtMop)
	{
#ifndef OMCI_X86
        mcastCmd = (op == SET_CTRL_WRITE ? OMCI_DM_MCAST_CMD_DYNAMIC_ACL_SET : OMCI_DM_MCAST_CMD_DYNAMIC_ACL_DEL);
        mcastAclEntry.profileId = pExtMop->EntityId;
        mcastAclEntry.aclTableEntryId = rowKey;

		if(aclType == MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX)
		{
			pExtTmpHead = &(pExtMop->DACLhead);
			LIST_FOREACH(ptrExt, pExtTmpHead, entries)
			{
				if(ptrExt->tableEntry.tableCtrl.bit.rowKey == rowKey)
				{
					switch(ptrExt->tableEntry.tableCtrl.bit.rowPartId)
					{
						case ROW_PART_0:
							mcastAclEntry.aniVid            =  ptrExt->tableEntry.row.format0.aniVid;
							mcastAclEntry.imputedGrpBw      =  ptrExt->tableEntry.row.format0.ImputedGrpBw;
							mcastAclEntry.previewLen        =  ptrExt->tableEntry.row.format0.previewLen;
							mcastAclEntry.previewRepeatTime =  ptrExt->tableEntry.row.format0.previewRepeatTime;
							mcastAclEntry.previewRepeatCnt  =  ptrExt->tableEntry.row.format0.previewRepeatCnt;
							mcastAclEntry.previewResetTime  =  ptrExt->tableEntry.row.format0.previewResetTime;
							//TBD: vendorSpecificUse
							break;
						case ROW_PART_1:
							pAddrCurr = ptrExt->tableEntry.row.format1.dstIpAddrStartRange;//low address is put msb
							memcpy(&addrRangeVal, pAddrCurr, sizeof(UINT32));
							if(addrRangeVal)
							{
								//ipv6
								memcpy(mcastAclEntry.dipStart.ipv6,
									ptrExt->tableEntry.row.format1.dstIpAddrStartRange, IPV6_ADDR_LEN);
							}
							else
							{
								//ipv4
								pAddrCurr = ptrExt->tableEntry.row.format1.dstIpAddrStartRange;
								pAddrCurr += LEADING_12BYTE_ADDR_LEN;
								memcpy(&mcastAclEntry.dipStart.ipv4, pAddrCurr, sizeof(UINT32));
							}
							break;
						case ROW_PART_2:
							pAddrCurr = ptrExt->tableEntry.row.format2.dstIpAddrEndRange;//low address is put msb
							memcpy(&addrRangeVal, pAddrCurr, sizeof(UINT32));
							if(addrRangeVal)
							{
								//ipv6
								memcpy(mcastAclEntry.dipEnd.ipv6,
									ptrExt->tableEntry.row.format2.dstIpAddrEndRange, IPV6_ADDR_LEN);
							}
							else
							{
								//ipv4
								pAddrCurr = ptrExt->tableEntry.row.format2.dstIpAddrEndRange;
								pAddrCurr += LEADING_12BYTE_ADDR_LEN;
                                memcpy(&mcastAclEntry.dipEnd.ipv4, pAddrCurr, sizeof(UINT32));
							}
							break;
						default:
							break;
					}
				}
			}
		}
		else
		{
			//TBD static ACL rule
			pExtTmpHead = &(pExtMop->SACLhead);
		}
	    feature_api(
            FEATURE_API_MCAST_RG_IGMP_SET, 
            mcastCmd, 
            &mcastAclEntry, 
            sizeof(omci_dm_mcast_acl_entry_t));

        OMCI_LOG(MCAST_RG_LOG_LVL, " %s() %d, port =%u, aclType=%u,	mopId=%u, key=%u",
			__FUNCTION__, __LINE__, portId, aclType, pExtMop->EntityId, rowKey);
#endif
		}

	return GOS_OK;
}



GOS_ERROR_CODE mcast_rg_un_auth_join_bhvr_set(UINT16 mopId, UINT8 unAuthJoinBhvr)
{
#ifndef OMCI_X86
	omci_dm_mcast_prof_t mcastProfVal;

	memset(&mcastProfVal, 0, sizeof(omci_dm_mcast_prof_t));
	mcastProfVal.profileId = mopId;
	mcastProfVal.unAuthJoinRqtBhvr = unAuthJoinBhvr;
	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_UNAUTHORIZED_JOIN_BEHAVIOR_SET, &mcastProfVal, sizeof(omci_dm_mcast_prof_t));
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, profileId=%u, unAuthJoinRqtBhvr=%u", 
	    __FUNCTION__, OMCI_DM_MCAST_CMD_UNAUTHORIZED_JOIN_BEHAVIOR_SET, mcastProfVal.profileId, mcastProfVal.unAuthJoinRqtBhvr);

	return GOS_OK;
}







GOS_ERROR_CODE
mcast_rg_mcast_profile_per_port_set(UINT32 op, UINT16 mopId, UINT16 portId)
{


#ifndef OMCI_X86
    omci_dm_mcast_cmd_t mcastCmd;
	omci_dm_mcast_port_Info_t portInfo;
	memset(&portInfo, 0, sizeof(omci_dm_mcast_port_Info_t));

    portInfo.portId = portId;
    portInfo.profileId = mopId;
    mcastCmd = op == SET_CTRL_WRITE ? OMCI_DM_MCAST_CMD_PROFILE_PER_PORT_ADD : OMCI_DM_MCAST_CMD_PROFILE_PER_PORT_DEL;

    feature_api(FEATURE_API_MCAST_RG_IGMP_SET, mcastCmd, &portInfo, sizeof(omci_dm_mcast_port_Info_t));
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s() , cmd=%u, profileId =%u, portId=%u", 
	    __FUNCTION__, mcastCmd, portInfo.profileId, portInfo.portId);

    return GOS_OK;
}

#if 0
GOS_ERROR_CODE mcast_rg_us_igmp_tag_info_set(MIB_TABLE_MCASTOPERPROF_T *pMop)
{
	#ifndef OMCI_X86
		rtk_gpon_rgIgmp_cfg_msg_t cfgMsg;
		memset(&cfgMsg, 0, sizeof(rtk_gpon_rgIgmp_cfg_msg_t));

		cfgMsg.opt_id = GPON_RGIGMP_IOCTL_US_TAG_SET;
		cfgMsg.igmp_cfg.profile_id = pMop->EntityId;

		switch(pMop->UsIGMPTagControl)
		{
			case CTRL_PASS:
				cfgMsg.igmp_cfg.us_igmp_tag_ctrl = GPON_RGIGMP_TAG_CTRL_PASS;
				break;
			case CTRL_ADD:
				cfgMsg.igmp_cfg.us_igmp_tag_ctrl = GPON_RGIGMP_TAG_CTRL_ADD_VLAN;
				break;
			case CTRL_REPLACE:
				cfgMsg.igmp_cfg.us_igmp_tag_ctrl = GPON_RGIGMP_TAG_CTRL_REPLACE_VLAN;
				break;
			case CTRL_REPLACE_VID:
				cfgMsg.igmp_cfg.us_igmp_tag_ctrl = GPON_RGIGMP_TAG_CTRL_REPLACE_VID;
				break;
			default:
				OMCI_PRINT("Not support");
		}
		cfgMsg.igmp_cfg.us_igmp_tci = pMop->UsIGMPTci;
		SETSOCKOPT(RTDRV_GPON_RGIGMP_CFGMSG_SET, &cfgMsg, rtk_gpon_rgIgmp_cfg_msg_t, 1);
	#endif
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d, mopId =%u", __FUNCTION__, __LINE__, pMop->EntityId);
	return GOS_OK;
}
#endif
GOS_ERROR_CODE mcast_rg_ds_igmp_mcast_tag_info_set(UINT16 mopId, UINT8 *pDsIgmpMcastTci)
{
	#ifndef OMCI_X86
		rtk_gpon_rgIgmp_cfg_msg_t cfgMsg;
		memset(&cfgMsg, 0, sizeof(rtk_gpon_rgIgmp_cfg_msg_t));

		cfgMsg.opt_id = GPON_RGIGMP_IOCTL_DS_TAG_SET;
		cfgMsg.igmp_cfg.profile_id = mopId;

		switch(pDsIgmpMcastTci[0])
		{
			case 0:
				cfgMsg.igmp_cfg.ds_igmp_mcast_tag_ctrl = GPON_RGIGMP_TAG_CTRL_PASS;
				break;
			case 1:
				cfgMsg.igmp_cfg.ds_igmp_mcast_tag_ctrl = GPON_RGIGMP_TAG_CTRL_STRIP_OUT_VLAN;
				break;
			case 2:
			case 5: //TBD svc
				cfgMsg.igmp_cfg.ds_igmp_mcast_tag_ctrl = GPON_RGIGMP_TAG_CTRL_ADD_VLAN;
				break;
			case 3:
			case 6: //TBD svc
				cfgMsg.igmp_cfg.ds_igmp_mcast_tag_ctrl = GPON_RGIGMP_TAG_CTRL_REPLACE_VLAN;
				break;
			case 4:
			case 7: //TBD svc
				cfgMsg.igmp_cfg.ds_igmp_mcast_tag_ctrl = GPON_RGIGMP_TAG_CTRL_REPLACE_VID;
				break;
			default:
				OMCI_PRINT("Not support");
		}
		cfgMsg.igmp_cfg.ds_igmp_mcast_tci = pDsIgmpMcastTci[1] << 8 | pDsIgmpMcastTci[2];
		SETSOCKOPT(RTDRV_GPON_RGIGMP_CFGMSG_SET, &cfgMsg, rtk_gpon_rgIgmp_cfg_msg_t, 1);
	#endif
	OMCI_LOG(MCAST_RG_LOG_LVL, "%s() %d, mopId =%u", __FUNCTION__, __LINE__, mopId);
	return GOS_OK;
}


GOS_ERROR_CODE mcast_rg_max_bw_set(UINT16 portId, UINT32 bw)
{
#ifndef OMCI_X86
	omci_dm_mcast_port_Info_t portInfo;
	memset(&portInfo, 0, sizeof(omci_dm_mcast_port_Info_t));

    portInfo.portId = portId;
    portInfo.portMaxBw = bw;

    feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_MAX_BW_SET, &portInfo, sizeof(omci_dm_mcast_port_Info_t));
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, port =%u, maxBw=%u", 
         __FUNCTION__, OMCI_DM_MCAST_CMD_MAX_BW_SET, portInfo.portId, portInfo.portMaxBw);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_bw_enforce_set(UINT16 portId, UINT8 enforceB)
{
#ifndef OMCI_X86
	omci_dm_mcast_port_Info_t portInfo;
	memset(&portInfo, 0, sizeof(omci_dm_mcast_port_Info_t));

	portInfo.portId = portId;
	portInfo.bwEnforceB = enforceB;

	feature_api(FEATURE_API_MCAST_RG_IGMP_SET, OMCI_DM_MCAST_CMD_BW_ENFORCE_SET, &portInfo, sizeof(omci_dm_mcast_port_Info_t));
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, port =%u, bwEnforceB=%u", 
         __FUNCTION__, OMCI_DM_MCAST_CMD_BW_ENFORCE_SET, portInfo.portId, portInfo.bwEnforceB);

	return GOS_OK;
}





GOS_ERROR_CODE mcast_rg_curr_bw_get(UINT16 portId, UINT32 *pCurrBw)
{

#ifndef OMCI_X86
	omci_dm_mcast_port_Info_t portInfo;
	memset(&portInfo, 0, sizeof(omci_dm_mcast_port_Info_t));

	portInfo.portId = portId;

	feature_api(FEATURE_API_MCAST_RG_IGMP_GET, OMCI_DM_MCAST_CMD_CURRENT_BW_GET, &portInfo, sizeof(omci_dm_mcast_port_Info_t));

	*pCurrBw = portInfo.currBw;

#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, port =%u, cur bw=%u", 
         __FUNCTION__, OMCI_DM_MCAST_CMD_CURRENT_BW_GET, portInfo.portId, *pCurrBw);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_join_count_get(UINT16 portId, UINT32 *pJoinCount)
{

#ifndef OMCI_X86
	omci_dm_mcast_port_Info_t portInfo;
	memset(&portInfo, 0, sizeof(omci_dm_mcast_port_Info_t));

	portInfo.portId = portId;

	feature_api(FEATURE_API_MCAST_RG_IGMP_GET, OMCI_DM_MCAST_CMD_JOIN_MSG_COUNT_GET, &portInfo, sizeof(omci_dm_mcast_port_Info_t));

	*pJoinCount = portInfo.joinCount;
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, port =%u, pJoinCount=%u", 
         __FUNCTION__, OMCI_DM_MCAST_CMD_JOIN_MSG_COUNT_GET, portInfo.portId, *pJoinCount);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_exceed_bw_count_get(UINT16 portId, UINT32 *pExcdBwCount)
{
#ifndef OMCI_X86
	omci_dm_mcast_port_Info_t portInfo;
	memset(&portInfo, 0, sizeof(omci_dm_mcast_port_Info_t));

	portInfo.portId = portId;

	feature_api(FEATURE_API_MCAST_RG_IGMP_GET, OMCI_DM_MCAST_CMD_BW_EXCEEDED_COUNT_GET, &portInfo, sizeof(omci_dm_mcast_port_Info_t));

	*pExcdBwCount = portInfo.bwExcdCount;
#endif

	OMCI_LOG(MCAST_RG_LOG_LVL, "%s(), cmd=%u, port =%u, exceed bw count=%u", 
         __FUNCTION__, OMCI_DM_MCAST_CMD_BW_EXCEEDED_COUNT_GET, portInfo.portId, *pExcdBwCount);

	return GOS_OK;
}

GOS_ERROR_CODE mcast_rg_active_group_get(MIB_ATTR_INDEX attrIndex, UINT16 portId,
	MIB_TABLE_MCASTSUBMONITOR_T *pMSM, UINT32 *pCurActiveGrpCnt)
{
    UINT32 ret,idx;
    INT32 curEntryCnt = 0;
    msm_attr_ipv4_active_group_list_table_t activeGroupTbl;
    omci_dm_mcast_ipv4_active_group_table_t ipv4GroupTbl;
    omci_dm_mcast_ipv4_active_group_entry_t ipv4GroupEntry[IPV4_ENTRY_COUNT_PER_GET];

	if (MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX == attrIndex)
	{
	    /*Start to get acitve count and let driver lock database*/
        ipv4GroupTbl.cmd = MCAST_GET_GROUP_TBL_CMD_START; 
		ret = feature_api(FEATURE_API_MCAST_RG_IGMP_GET, OMCI_DM_MCAST_CMD_IPV4_ACTIVE_GROUP_GET, 
                 &ipv4GroupTbl, sizeof(omci_dm_mcast_ipv4_active_group_table_t));

        if(FAL_OK != ret)
        {
            *pCurActiveGrpCnt = 0;
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, " %s()@%d ERR ret=%u", __FUNCTION__, __LINE__, ret);
            return GOS_OK;
        }       
        curEntryCnt = ipv4GroupTbl.activeGrpCnt;

        while(curEntryCnt > 0)
        {            
            ipv4GroupTbl.cmd = MCAST_GET_GROUP_TBL_CMD_GET; //get cmd

            if(curEntryCnt >= IPV4_ENTRY_COUNT_PER_GET)
            {               
                ipv4GroupTbl.entryCnt = IPV4_ENTRY_COUNT_PER_GET;
            }
            else
            {
                ipv4GroupTbl.entryCnt = curEntryCnt;

            }
            ipv4GroupTbl.pEntry = (omci_dm_mcast_ipv4_active_group_entry_t *)&ipv4GroupEntry[0];

            ret = feature_api(FEATURE_API_MCAST_RG_IGMP_GET, OMCI_DM_MCAST_CMD_IPV4_ACTIVE_GROUP_GET, 
                &ipv4GroupTbl, sizeof(omci_dm_mcast_ipv4_active_group_table_t));
            if(FAL_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, " %s()@%d ERR ret=%u", __FUNCTION__, __LINE__, ret);
            } 

            for (idx = 0; idx < ipv4GroupTbl.entryCnt; idx++)
            {
                memset(&activeGroupTbl, 0x0, sizeof(msm_attr_ipv4_active_group_list_table_t));
                msm_attr_ipv4_active_group_list_table_entry_t *pNew =
                    (msm_attr_ipv4_active_group_list_table_entry_t *)malloc(sizeof(msm_attr_ipv4_active_group_list_table_entry_t));
                if(pNew)
                {
                    activeGroupTbl.VlanID               = ipv4GroupTbl.pEntry->vid;
                    activeGroupTbl.ClientIpAddr         = ipv4GroupTbl.pEntry->clientIp;
                    activeGroupTbl.ElapseTime           = ipv4GroupTbl.pEntry->elapseTime;
                    activeGroupTbl.McastDstIpAddr       = ipv4GroupTbl.pEntry->dip;
                    activeGroupTbl.BeActualBandwidthEst = ipv4GroupTbl.pEntry->beActualBandwidthEst;
                    activeGroupTbl.SrcIpAddr            = ipv4GroupTbl.pEntry->sip;
                    memcpy(&(pNew->tableEntry), &activeGroupTbl, sizeof(msm_attr_ipv4_active_group_list_table_t));
                    LIST_INSERT_HEAD(&pMSM->IPv4ActiveGroupListTable_head, pNew, entries);
                }
                ipv4GroupTbl.pEntry++; 

            }
            curEntryCnt -= ipv4GroupTbl.entryCnt;

        }
        
         *pCurActiveGrpCnt = ipv4GroupTbl.activeGrpCnt;
        
    }
    else if(MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX == attrIndex)
    {

    }


	OMCI_LOG(MCAST_RG_LOG_LVL, " %s() port =%u, join count=%u", __FUNCTION__, portId, *pCurActiveGrpCnt);
	return GOS_OK;
}


omci_mcast_wrapper_t rg_snp_wrapper =
{
	/* Common */
    .omci_config_init                            = mcast_rg_clear_all,
    .omci_ctrl_pkt_behaviour_set                 = mcast_rg_igmp_port_enable_set,
    .omci_mop_profile_add                        = mcast_rg_prof_add,
    .omci_mop_profile_del                        = mcast_rg_prof_del,


    /* ME-309 */
    .omci_igmp_version_set                       = mcast_rg_igmp_version_set,
    .omci_igmp_function_set                      = mcast_rg_igmp_function_set,
    .omci_immediate_leave_set                    = mcast_rg_immediate_leave_set,
    .omci_max_simultaneous_groups_set            = mcast_rg_max_sim_group_set,
    .omci_us_igmp_tag_info_set                   = NULL, 
    .omci_us_igmp_rate_set                       = mcast_rg_us_igmp_rate_set,
    .omci_robustness_set                         = mcast_rg_robustness_set,
    .omci_querier_ip_addr_set                    = NULL,
    .omci_query_interval_set                     = mcast_rg_query_intv_set,
    .omci_query_max_response_time_set            = mcast_rg_query_max_rsp_time_set,
    .omci_last_member_query_interval_set         = mcast_rg_last_mbr_query_intv_set,
    .omci_unauthorized_join_behaviour_set        = mcast_rg_un_auth_join_bhvr_set,
    .omci_ds_igmp_multicast_tci_set              = NULL,
    
    .omci_dynamic_acl_table_entry_set            = mcast_rg_acl_set,


    .omci_mop_profile_per_port_set               = mcast_rg_mcast_profile_per_port_set,  
    .omci_acl_per_port_set                       = NULL,                                 


    /* ME-310 */
    .omci_connection_rule_set                    = NULL,                                 
    .omci_allowed_preview_groups_table_entry_set = NULL,
    .omci_max_multicast_bandwidth_set            = mcast_rg_max_bw_set,
    .omci_bandwidth_enforcement_set              = mcast_rg_bw_enforce_set,

    /* ME-311 */
    .omci_current_multicast_bandwidth_get        = mcast_rg_curr_bw_get,
    .omci_join_message_counter_get               = mcast_rg_join_count_get,
    .omci_bandwidth_exceeded_counter_get         = mcast_rg_exceed_bw_count_get,
    .omci_active_group_list_table_get            = mcast_rg_active_group_get,
};
