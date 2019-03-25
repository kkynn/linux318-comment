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
 * Purpose : Definition of OMCI utilities APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI voice utilities APIs
 */

#include "app_basic.h"

GOS_ERROR_CODE omci_voice_service_handler(void  *pObj)
{
    // VoIP config data 9.9.18
    MIB_TABLE_VOIPCONFIGDATA_T                  mibVoipCfgData;
    // SIP user data 9.9.2
    MIB_TABLE_SIPUSERDATA_T                     mibSipUserData;
    // SIP Agent config data 9.9.3
    MIB_TABLE_SIPAGENTCONFIGDATA_T              mibSipAgentCfgData;
    // POTS UNI 9.9.1
    MIB_TABLE_POTSUNI_T                         mibPotsUni;
    // VoIP voice CTP 9.9.4
	MIB_TABLE_VOIPVOICECTP_T                    mibVoIpVoiceCtp, *ptr = NULL;
    // Large string 9.12.5  for common use
	MIB_TABLE_LARGE_STRING_T                    tmpMibLargeString, *pMibLargeString;
	// Network address 9.12.3  for common use
	MIB_TABLE_NETWORK_ADDR_T                    tmpMibNetworkAddr, *pMibNetworkAddr;
    // Network dial plan table 9.9.10
    MIB_TABLE_NETWORKDIALPLANTABLE_T            tmpMibDialPlanTbl, *pMibDialPlanTbl = NULL;
    // VoIP media profile 9.9.5
    MIB_TABLE_VOIPMEDIAPROFILE_T                tmpMibVoIPMediaProfile, *pMibVoIPMediaProfile;
    // RTP profile data 9.9.7
    MIB_TABLE_RTPPROFILEDATA_T                  tmpMibRtpProfileData, *pMibRtpProfileData;
    // Voice service profile 9.9.6
    MIB_TABLE_VOICESERVICEPROFILE_T             tmpMibVoiceServiceProfile, *pMibVoiceServiceProfile;
    // Authentication security method 9.12.4
    MIB_TABLE_AUTH_SEC_METHOD_T                 tmpMibAuthSecMethod, *pMibAuthSecMethod;
    // VoIP application service profile 9.9.8
    MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_T   tmpMibVoipApplicationServiceProfile, *pMibVoipApplicationServiceProfile;
    // VoIP feature access codes 9.9.9
    MIB_TABLE_VOIPFEATUREACCESSCODES_T          tmpMibVoipFeatureAccessCodes, *pMibVoipFeatureAccessCodes;
    // TCP/UDP config data 9.4.3 for common use
    MIB_TABLE_TCP_UDP_CFG_DATA_T                tmpMibTcpUdpCfgData, *pMibTcpUdpCfgData;
    // IP host config data 9.4.1
    MIB_TABLE_IP_HOST_CFG_DATA_T                tmpMibIpHostCfgData, *pMibIpHostCfgData;

	UINT32 i;
	UINT16 chid;
	UINT32 proxy = 0; // Backup proxy is optional in G.988
	struct in_addr addr;
	CHAR *addr_str = NULL;
    UINT32 voip_ctp_cnt = 0, count =0;
    char *lstr_ptr = NULL;
    BOOL do_voice_service = FALSE;
    UINT8 order_size = 12;

    ptr = (MIB_TABLE_VOIPVOICECTP_T  *)pObj;

    mibVoipCfgData.EntityId = 0;
    if (GOS_OK != MIB_Get(MIB_TABLE_VOIPCONFIGDATA_INDEX, &mibVoipCfgData, sizeof(MIB_TABLE_VOIPCONFIGDATA_T)))
        return GOS_ERR_DISABLE;

    ODBG_Y("sip_proto_used=%u\n" , mibVoipCfgData.SignallingProtocolUsed);

    if (0 ==  MIB_GetTableCurEntryCount(MIB_TABLE_POTSUNI_INDEX))
        return GOS_ERR_DISABLE;

    if (0 == (voip_ctp_cnt = MIB_GetTableCurEntryCount(MIB_TABLE_VOIPVOICECTP_INDEX)))
        return GOS_ERR_DISABLE;

    if (GOS_OK != MIB_GetFirst(MIB_TABLE_VOIPVOICECTP_INDEX, &mibVoIpVoiceCtp, sizeof(MIB_TABLE_VOIPVOICECTP_T)))
	    return GOS_ERR_DISABLE;

	while (count < voip_ctp_cnt)
	{
		count++;
        if (ptr && ptr->EntityId != mibVoIpVoiceCtp.EntityId)
            goto next_voip_ctp;

        ODBG_Y("entity_id=%u\n" , mibVoIpVoiceCtp.EntityId);

        mibPotsUni.EntityId = mibVoIpVoiceCtp.PPTPPointer;
        if (GOS_OK != MIB_Get(MIB_TABLE_POTSUNI_INDEX, &mibPotsUni, sizeof(MIB_TABLE_POTSUNI_T)))
            goto next_voip_ctp;

        ODBG_Y("PPTPPointer=%u\n" , mibVoIpVoiceCtp.PPTPPointer);

        omci_get_channel_index_by_pots_uni_me_id(mibPotsUni.EntityId, &chid);
        if (chid > gInfo.devCapabilities.potsPortNum)
        {
            OMCI_PRINT("%s(%d) channel number %u out of range\n" , __FUNCTION__ , __LINE__ , chid);
            goto next_voip_ctp;
        }
	    VOICE_WRAPPER(omci_voice_pots_state_set, chid, mibPotsUni.AdminState);
        ODBG_Y("chid=%u\n" , chid);

        if (VCD_SIG_PROTOCOL_USED_H248 == mibVoipCfgData.SignallingProtocolUsed)
            OMCI_PRINT("TBD\n");

        if ((VCD_SIG_PROTOCOL_USED_SIP == mibVoipCfgData.SignallingProtocolUsed)&&
			(VCD_CFG_METHOD_USED_OMCI== mibVoipCfgData.VOIPConfigurationMethodUsed))
        {
            mibSipUserData.EntityId = mibVoIpVoiceCtp.UserProtocolPointer;
            if (GOS_OK != MIB_Get(MIB_TABLE_SIPUSERDATA_INDEX, &mibSipUserData, sizeof(MIB_TABLE_SIPUSERDATA_T)))
                goto next_voip_ctp;

            ODBG_Y("sip user data found, EntityId=%u\n", mibSipUserData.EntityId);
            ODBG_Y("UserPartAOR=%u\n" , mibSipUserData.UserPartAOR);
            tmpMibLargeString.EntityId = mibSipUserData.UserPartAOR;
            if (FALSE == mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &tmpMibLargeString, &pMibLargeString))
                ODBG_R("Large string entry not found\n");
            else
            {
                ODBG_G("Large string entry found, EntityId=%du\n", pMibLargeString->EntityId);
                ODBG_Y("NumOfParts=%u\n", pMibLargeString->NumOfParts);
                VOICE_WRAPPER(omci_voice_user_part_aor_set, chid, proxy,
                              pMibLargeString->Part1, pMibLargeString->NumOfParts, MIB_TABLE_LARGE_STRING_PART_LEN);
            }

            ODBG_Y("SIPDisplayName=%s\n" , mibSipUserData.SIPDisplayName);
            VOICE_WRAPPER(omci_voice_sip_display_name_set, chid, proxy,
                          mibSipUserData.SIPDisplayName, MIB_TABLE_SIPDISPLAYNAME_LEN);

            ODBG_Y("UsernamePassword=%u\n", mibSipUserData.UsernamePassword);
            ODBG_Y("VoicemailServerSIPURI=%u\n", mibSipUserData.VoicemailServerSIPURI);
            ODBG_Y("VoicemailSubscriptionExpirationTime=%u\n", mibSipUserData.VoicemailSubscriptionExpirationTime);
            ODBG_Y("NetworkDialPlanPointer=%u\n", mibSipUserData.NetworkDialPlanPointer);

            tmpMibDialPlanTbl.EntityId = mibSipUserData.NetworkDialPlanPointer;
            if (mib_FindEntry(MIB_TABLE_NETWORKDIALPLANTABLE_INDEX, &tmpMibDialPlanTbl, &pMibDialPlanTbl))
            {
                ODBG_G("Network Dial Plan entry found\n");
                ODBG_Y("EntityId=%d curDpTableEntryCnt=%u\n" , pMibDialPlanTbl->EntityId, pMibDialPlanTbl->DialPlanNumber);

                dpTableEntry_t              *pDialPlanTblEntry = NULL;

                LIST_FOREACH(pDialPlanTblEntry, &pMibDialPlanTbl->head, entries)
                {
                    VOICE_WRAPPER(omci_voice_dial_plan_set, chid,
                                  pDialPlanTblEntry->tableEntry.dpToken, MIB_TABLE_DIALPLANTABLE_TOKEN_LEN);
                }
            }

            ODBG_Y("ApplicationServicesProfilePointer=%u\n", mibSipUserData.ApplicationServicesProfilePointer);
            ODBG_Y("FeatureCodePointer=%u\n", mibSipUserData.FeatureCodePointer);
            ODBG_Y("PPTPPointer=%u\n",  mibSipUserData.PPTPPointer);
            ODBG_Y("ReleaseTimer=%u\n", mibSipUserData.ReleaseTimer);
            ODBG_Y("ROHTimer=%u\n", mibSipUserData.ROHTimer);

            tmpMibAuthSecMethod.EntityId = mibSipUserData.UsernamePassword;
            ODBG_Y("UsernamePassword=%u\n" , mibSipUserData.UsernamePassword);

            if (FALSE == mib_FindEntry(MIB_TABLE_AUTH_SEC_METHOD_INDEX , &tmpMibAuthSecMethod, &pMibAuthSecMethod))
                ODBG_R("Auth sec method Entry not found %u\n" , tmpMibAuthSecMethod.EntityId);
            else
            {
                ODBG_G("Auth sec method Entry found %u\n" , tmpMibAuthSecMethod.EntityId);
                ODBG_Y("Username=%s Password=%s \n" , pMibAuthSecMethod->Username1, pMibAuthSecMethod->Password);
                ODBG_Y("Username2=%s \n" , pMibAuthSecMethod->Username2);
                VOICE_WRAPPER(omci_voice_user_name_set, chid, proxy,
                              pMibAuthSecMethod->Username1, pMibAuthSecMethod->Username2,MIB_TABLE_AUTH_SEC_METHOD_USERNAME1_LEN);
                VOICE_WRAPPER(omci_voice_password_set, chid, proxy,
                              pMibAuthSecMethod->Password, MIB_TABLE_AUTH_SEC_METHOD_PASSWORD_LEN);
            }

            tmpMibVoipApplicationServiceProfile.EntityId = mibSipUserData.ApplicationServicesProfilePointer;

            if (FALSE == mib_FindEntry(MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_INDEX, &tmpMibVoipApplicationServiceProfile, &pMibVoipApplicationServiceProfile))
                ODBG_R("VoIP application service profile not found EntityId=%u\n", tmpMibVoipApplicationServiceProfile.EntityId);
            else
            {
                ODBG_G("VoIP application service profile found, EntityId=%u\n", tmpMibVoipApplicationServiceProfile.EntityId);
                ODBG_Y("CIDFeatures=%u\n" , pMibVoipApplicationServiceProfile->CIDFeatures);

                ODBG_Y("CallWaitingFeatures=%u\n" , pMibVoipApplicationServiceProfile->CallWaitingFeatures);
                VOICE_WRAPPER(omci_voice_call_wait_feature_set, chid, pMibVoipApplicationServiceProfile->CallWaitingFeatures);

                ODBG_Y("CallProgressOrTransferFeatures=%u\n" , pMibVoipApplicationServiceProfile->CallProgressOrTransferFeatures);
                VOICE_WRAPPER(omci_voice_progress_or_transfer_feature_set, chid, pMibVoipApplicationServiceProfile->CallProgressOrTransferFeatures);

                ODBG_Y("CallPresentationFeatures=%u\n" , pMibVoipApplicationServiceProfile->CallPresentationFeatures);

                ODBG_Y("DirectConnectFeature=%u\n" , pMibVoipApplicationServiceProfile->DirectConnectFeature);
                VOICE_WRAPPER(omci_voice_direct_connect_feature_set, chid, pMibVoipApplicationServiceProfile->DirectConnectFeature);

                ODBG_Y("DirectConnectURIPointer=%u\n" , pMibVoipApplicationServiceProfile->DirectConnectURIPointer);

                tmpMibNetworkAddr.EntityId = pMibVoipApplicationServiceProfile->DirectConnectURIPointer;
                if (FALSE == mib_FindEntry(MIB_TABLE_NETWORK_ADDR_INDEX, &tmpMibNetworkAddr, &pMibNetworkAddr))
                    ODBG_Y("Network address entry not found\n");
                else
                {
                    ODBG_G("Network address entry found, EntityId=%u\n", pMibNetworkAddr->EntityId);
                    ODBG_Y("SecurityPtr=%u\n", pMibNetworkAddr->SecurityPtr);
                    ODBG_Y("AddrPtr=0x%02x\n", pMibNetworkAddr->AddrPtr);

                    tmpMibLargeString.EntityId = pMibNetworkAddr->AddrPtr;
                    if (FALSE == mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &tmpMibLargeString, &pMibLargeString))
                        ODBG_R("Large String Entry not found\n");
                    else
                    {
                        ODBG_G("Large String Entry found, EntityId=%u\n", pMibLargeString->EntityId);
                        ODBG_Y(" NumOfParts=%u\n", pMibLargeString->NumOfParts);
                        for (i = 0, lstr_ptr=pMibLargeString->Part1 ; i<pMibLargeString->NumOfParts ; i++ , lstr_ptr += MIB_TABLE_LARGE_STRING_PART_LEN+1)
                        {
                            ODBG_Y("%s\n" , lstr_ptr);
                            //TBD
                        }
                    }
                }

                ODBG_Y("BridgedLineAgentURIPointer=%u\n" , pMibVoipApplicationServiceProfile->BridgedLineAgentURIPointer);
                tmpMibNetworkAddr.EntityId = pMibVoipApplicationServiceProfile->BridgedLineAgentURIPointer;
                if (FALSE == mib_FindEntry(MIB_TABLE_NETWORK_ADDR_INDEX, &tmpMibNetworkAddr, &pMibNetworkAddr))
                    ODBG_Y("Network address entry not found\n");
                else
                {
                    ODBG_G("Network address entry found,EntityId=%u\n", pMibNetworkAddr->EntityId);
                    ODBG_Y("SecurityPtr=%u\n", pMibNetworkAddr->SecurityPtr);
                    ODBG_Y("AddrPtr=0x%02x\n", pMibNetworkAddr->AddrPtr);

                    tmpMibLargeString.EntityId = pMibNetworkAddr->AddrPtr;
                    if (FALSE == mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &tmpMibLargeString, &pMibLargeString))
                        ODBG_R("Large String Entry not found\n");
                    else
                    {
                        ODBG_G("Large String Entry found, EntityId=%d\n", pMibLargeString->EntityId);
                        ODBG_Y("NumOfParts=%u\n", pMibLargeString->NumOfParts);

                        for (i = 0, lstr_ptr=pMibLargeString->Part1; i < pMibLargeString->NumOfParts; i++, lstr_ptr += MIB_TABLE_LARGE_STRING_PART_LEN+1)
                        {
                            ODBG_Y("%s\n" , lstr_ptr);
                            //TBD
                        }
                    }
                }

                ODBG_Y("ConferenceFactoryURIPointer=%u\n" , pMibVoipApplicationServiceProfile->ConferenceFactoryURIPointer);
                tmpMibNetworkAddr.EntityId = pMibVoipApplicationServiceProfile->ConferenceFactoryURIPointer;
                if (FALSE == mib_FindEntry(MIB_TABLE_NETWORK_ADDR_INDEX, &tmpMibNetworkAddr, &pMibNetworkAddr))
                    ODBG_Y("Network address entry not found\n");
                else
                {
                    ODBG_G("Network address entry found,EntityId=%u\n", pMibNetworkAddr->EntityId);
                    ODBG_Y("SecurityPtr=%u\n", pMibNetworkAddr->SecurityPtr);
                    ODBG_Y("AddrPtr=0x%02x\n", pMibNetworkAddr->AddrPtr);
                    tmpMibLargeString.EntityId = pMibNetworkAddr->AddrPtr;
                    if (FALSE == mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &tmpMibLargeString, &pMibLargeString))
                        ODBG_R("Large String Entry not found\n");
                    else
                    {
                        ODBG_G("Large String Entry found, EntityId=%u\n", pMibLargeString->EntityId);
                        ODBG_Y(" NumOfParts=%u\n", pMibLargeString->NumOfParts);

                        for (i = 0, lstr_ptr=pMibLargeString->Part1; i < pMibLargeString->NumOfParts; i++, lstr_ptr += MIB_TABLE_LARGE_STRING_PART_LEN+1)
                        {
                            ODBG_Y("%s\n" , lstr_ptr);
                            //TBD
                        }
                    }
                }
            }

            tmpMibVoipFeatureAccessCodes.EntityId = mibSipUserData.FeatureCodePointer;
            if (FALSE == mib_FindEntry(MIB_TABLE_VOIPFEATUREACCESSCODES_INDEX, &tmpMibVoipFeatureAccessCodes, &pMibVoipFeatureAccessCodes))
                ODBG_R("VoIP feature access codes not found EntityId=%u\n" , tmpMibVoipFeatureAccessCodes.EntityId);
            else
            {
                ODBG_G("VoIP feaute access codes found, EntityId=%u\n" , tmpMibVoipFeatureAccessCodes.EntityId);
                ODBG_Y("CancelCallWaiting=%s\n", pMibVoipFeatureAccessCodes->CancelCallWaiting);
                ODBG_Y("CancelHold=%s\n", pMibVoipFeatureAccessCodes->CancelHold);
                ODBG_Y("CancelPark=%s\n", pMibVoipFeatureAccessCodes->CancelPark);
                ODBG_Y("CancelIDActivate=%s\n", pMibVoipFeatureAccessCodes->CancelIDActivate);
                ODBG_Y("CancelIDdeactivate=%s\n", pMibVoipFeatureAccessCodes->CancelIDdeactivate);
                ODBG_Y("DoNotDisturbActivate=%s\n", pMibVoipFeatureAccessCodes->DoNotDisturbActivate);
                ODBG_Y("DoNotDisturbDeactivate=%s\n", pMibVoipFeatureAccessCodes->DoNotDisturbDeactivate);
                ODBG_Y("DoNotDisturbPINChange=%s\n", pMibVoipFeatureAccessCodes->DoNotDisturbPINChange);
                ODBG_Y("EmergencyServiceNumber=%s\n", pMibVoipFeatureAccessCodes->EmergencyServiceNumber);
                ODBG_Y("IntercomService=%s\n", pMibVoipFeatureAccessCodes->IntercomService);
            }

            mibSipAgentCfgData.EntityId = mibSipUserData.SIPAgentPointer;
            if (GOS_OK != MIB_Get(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX, &mibSipAgentCfgData, sizeof(MIB_TABLE_SIPAGENTCONFIGDATA_T)))
                goto next_voip_ctp;

            ODBG_Y("SIP agent config data found EntityId=%u\n" , mibSipAgentCfgData.EntityId);

            // ProxyServerAddressPointer
            tmpMibLargeString.EntityId = mibSipAgentCfgData.ProxyServerAddressPointer;
            if (FALSE == mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &tmpMibLargeString, &pMibLargeString))
                ODBG_R("Large String Entry not found\n");
            else
            {
                ODBG_G("Large String Entry found, EntityId=%u\n", pMibLargeString->EntityId);
                ODBG_Y("NumOfParts=%u\n", pMibLargeString->NumOfParts);
                VOICE_WRAPPER(omci_voice_proxy_server_ip_set, chid, proxy,
                              pMibLargeString->Part1, pMibLargeString->NumOfParts, MIB_TABLE_LARGE_STRING_PART_LEN);
            }

            // OutboundProxyAddressPointer
            tmpMibLargeString.EntityId = mibSipAgentCfgData.OutboundProxyAddressPointer;
            if (FALSE == mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &tmpMibLargeString, &pMibLargeString))
                ODBG_R("Large String Entry not found\n");
            else
            {
                ODBG_G("Large String Entry found, EntityId=%u\n", pMibLargeString->EntityId);
                ODBG_Y("NumOfParts=%u\n", pMibLargeString->NumOfParts);
                VOICE_WRAPPER(omci_voice_outbound_proxy_ip_set, chid, proxy,
                              pMibLargeString->Part1, pMibLargeString->NumOfParts, MIB_TABLE_LARGE_STRING_PART_LEN);
            }

            ODBG_Y("PrimarySIPDNS=%u\n", mibSipAgentCfgData.PrimarySIPDNS);
            ODBG_Y("SecondarySIPDNS=%u\n", mibSipAgentCfgData.SecondarySIPDNS);
            ODBG_Y("TCPUDPPointer=%u\n", mibSipAgentCfgData.TCPUDPPointer);

            ODBG_Y("SIPRegExpTime=%u\n", mibSipAgentCfgData.SIPRegExpTime);
            VOICE_WRAPPER(omci_voice_sip_reg_exp_time_set, chid, proxy, mibSipAgentCfgData.SIPRegExpTime);

            ODBG_Y("SIPReregHeadStartTime=%u\n" , mibSipAgentCfgData.SIPReregHeadStartTime);
            VOICE_WRAPPER(omci_voice_reg_head_start_time_set, chid, proxy, mibSipAgentCfgData.SIPReregHeadStartTime);

            // HostPartURI
            ODBG_Y("HostPartURI=%u\n", mibSipAgentCfgData.HostPartURI);
            ODBG_Y("SIPStatus=%u\n", mibSipAgentCfgData.SIPStatus);
            ODBG_Y("SIPRegistrar=0x%02x\n" , mibSipAgentCfgData.SIPRegistrar);

            tmpMibNetworkAddr.EntityId = mibSipAgentCfgData.SIPRegistrar;
            if (FALSE == mib_FindEntry(MIB_TABLE_NETWORK_ADDR_INDEX, &tmpMibNetworkAddr, &pMibNetworkAddr))
                ODBG_Y("Network address entry not found\n");
            else
            {
                ODBG_G("Network address entry found, EntityId=%u\n", pMibNetworkAddr->EntityId);
                ODBG_Y("SecurityPtr=%u\n", pMibNetworkAddr->SecurityPtr);
                ODBG_Y("AddrPtr=0x%02x\n", pMibNetworkAddr->AddrPtr);

                tmpMibLargeString.EntityId = pMibNetworkAddr->AddrPtr;
                if (FALSE == mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &tmpMibLargeString, &pMibLargeString))
                    ODBG_R("Large String Entry not found\n");
                else
                {
                    ODBG_G("Large String Entry found, EntityId=%u\n", pMibLargeString->EntityId);
                    ODBG_Y("NumOfParts=%u\n", pMibLargeString->NumOfParts);

                    for (i = 0 ,lstr_ptr=pMibLargeString->Part1; i < pMibLargeString->NumOfParts; i++, lstr_ptr += MIB_TABLE_LARGE_STRING_PART_LEN+1)
                    {
                        ODBG_Y("%s\n" , lstr_ptr);
                    }
                }
            }

            ODBG_Y("Softswitch=%s\n" , mibSipAgentCfgData.Softswitch);

            tmpMibTcpUdpCfgData.EntityId = mibSipAgentCfgData.TCPUDPPointer;
            ODBG_Y("TCPUDPPointer=%u\n" , mibSipAgentCfgData.TCPUDPPointer);

            if (FALSE == mib_FindEntry(MIB_TABLE_TCP_UDP_CFG_DATA_INDEX , &tmpMibTcpUdpCfgData, &pMibTcpUdpCfgData))
                ODBG_R("TCP/UDP config data not found. EntityId=%u\n", tmpMibTcpUdpCfgData.EntityId);
            else
            {
                ODBG_G("TCP/UDP config data found %u\n" , tmpMibTcpUdpCfgData.EntityId);

                ODBG_Y("PortId=%u\n" , pMibTcpUdpCfgData->PortId);
                VOICE_WRAPPER(omci_voice_proxy_tcp_udp_port_set, chid, proxy, pMibTcpUdpCfgData->PortId);

                ODBG_Y("Protocol=%u\n" , pMibTcpUdpCfgData->Protocol);
                ODBG_Y("TosDiffserv=%u\n" , pMibTcpUdpCfgData->TosDiffserv);
                ODBG_Y("IpHostPointer=%u\n" , pMibTcpUdpCfgData->IpHostPointer);

            tmpMibIpHostCfgData.EntityID = pMibTcpUdpCfgData->IpHostPointer;
            ODBG_Y("IpHostPointer=%u\n" , pMibTcpUdpCfgData->IpHostPointer);

            if (FALSE == mib_FindEntry(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &tmpMibIpHostCfgData, &pMibIpHostCfgData))
                ODBG_R("IP host config data not found %u\n" , tmpMibIpHostCfgData.EntityID);
            else
            {
                ODBG_G("IP host config data found %u\n" , tmpMibIpHostCfgData.EntityID);
                ODBG_Y("EntityID=%u\n" , pMibIpHostCfgData->EntityID);
                ODBG_Y("IpOptions=%u\n" , pMibIpHostCfgData->IpOptions);
                ODBG_Y("MacAddress=%s\n" , pMibIpHostCfgData->MacAddress);
                ODBG_Y("OnuIdentifier=%s\n" , pMibIpHostCfgData->OnuIdentifier);
                ODBG_Y("IpAddress=%u\n" , pMibIpHostCfgData->IpAddress);
                ODBG_Y("Mask=%u\n" , pMibIpHostCfgData->Mask);
                ODBG_Y("Gateway=%u\n" , pMibIpHostCfgData->Gateway);
                ODBG_Y("PrimaryDns=%u\n" , pMibIpHostCfgData->PrimaryDns);
                ODBG_Y("SecondaryDns=%u\n" , pMibIpHostCfgData->SecondaryDns);
                ODBG_Y("CurrentAddress=%u\n" , pMibIpHostCfgData->CurrentAddress);
                ODBG_Y("CurrentMask=%u\n" , pMibIpHostCfgData->CurrentMask);
                ODBG_Y("CurrentGateway=%u\n" , pMibIpHostCfgData->CurrentGateway);
                ODBG_Y("CurrentPrimaryDns=%u\n" , pMibIpHostCfgData->CurrentPrimaryDns);
                ODBG_Y("CurrentSecondaryDns=%u\n" , pMibIpHostCfgData->CurrentSecondaryDns);
                ODBG_Y("DomainName=%s\n" , pMibIpHostCfgData->DomainName);
                ODBG_Y("HostName=%s\n" , pMibIpHostCfgData->HostName);
                ODBG_Y("RelayAgentOptions=%u\n" , pMibIpHostCfgData->RelayAgentOptions);
            }
			}
            // HostPartURI
            tmpMibLargeString.EntityId = mibSipAgentCfgData.HostPartURI;
            if (FALSE == mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &tmpMibLargeString, &pMibLargeString))
            {
                ODBG_R("Large String Entry not found\n");
                if (pMibIpHostCfgData)
                {
                    ODBG_G("pMibIpHostCfgData is found\n");
                    addr.s_addr = pMibIpHostCfgData->IpAddress;
                    addr_str = inet_ntoa(addr);
                    VOICE_WRAPPER(omci_voice_proxy_domain_name_set, chid, proxy, addr_str, 0, strlen(addr_str));
                }
            }
            else
            {
                ODBG_G("Large String Entry found, EntityId=%u\n", pMibLargeString->EntityId);
                ODBG_Y("NumOfParts=%u\n", pMibLargeString->NumOfParts);
                VOICE_WRAPPER(omci_voice_proxy_domain_name_set, chid, proxy,
                              pMibLargeString->Part1, pMibLargeString->NumOfParts, MIB_TABLE_LARGE_STRING_PART_LEN);

            }

            ODBG_Y("start to voip media profile\n");
            tmpMibVoIPMediaProfile.EntityId = mibVoIpVoiceCtp.VOIPMediaProfilePointer;
            if (FALSE == mib_FindEntry(MIB_TABLE_VOIPMEDIAPROFILE_INDEX , &tmpMibVoIPMediaProfile, &pMibVoIPMediaProfile))
			ODBG_R("VoIP media profile not found %u\n" , tmpMibVoIPMediaProfile.EntityId);
		else{
            ODBG_G("VoIP media profile Entry found %u\n" , tmpMibVoIPMediaProfile.EntityId);
            ODBG_Y("FaxMode=%u\n" , pMibVoIPMediaProfile->FaxMode);
            VOICE_WRAPPER(omci_voice_fax_mode_set, chid, pMibVoIPMediaProfile->FaxMode);
            ODBG_Y("OOBDTMF=%u\n" , pMibVoIPMediaProfile->OOBDTMF);
            VOICE_WRAPPER(omci_voice_oob_dtmf_set, chid, pMibVoIPMediaProfile->OOBDTMF);

            ODBG_Y("CodecSelection1stOrder=%u\n"        , pMibVoIPMediaProfile->CodecSelection1stOrder);
            ODBG_Y("PacketPeriodSelection1stOrder=%u\n" , pMibVoIPMediaProfile->PacketPeriodSelection1stOrder);
            ODBG_Y("SilenceSuppression1stOrder=%u\n"    , pMibVoIPMediaProfile->SilenceSuppression1stOrder);

            ODBG_Y("CodecSelection2ndOrder=%u\n"        , pMibVoIPMediaProfile->CodecSelection2ndOrder);
            ODBG_Y("PacketPeriodSelection2ndOrder=%u\n" , pMibVoIPMediaProfile->PacketPeriodSelection2ndOrder);
            ODBG_Y("SilenceSuppression2ndOrder=%u\n"    , pMibVoIPMediaProfile->SilenceSuppression2ndOrder);

            ODBG_Y("CodecSelection3rdOrder=%u\n"        , pMibVoIPMediaProfile->CodecSelection3rdOrder);
            ODBG_Y("PacketPeriodSelection3rdOrder=%u\n" , pMibVoIPMediaProfile->PacketPeriodSelection3rdOrder);
            ODBG_Y("SilenceSuppression3rdOrder=%u\n"    , pMibVoIPMediaProfile->SilenceSuppression3rdOrder);

            ODBG_Y("CodecSelection4thOrder=%u\n"        , pMibVoIPMediaProfile->CodecSelection4thOrder);
            ODBG_Y("PacketPeriodSelection4thOrder=%u\n" , pMibVoIPMediaProfile->PacketPeriodSelection4thOrder);
            ODBG_Y("SilenceSuppression4thOrder=%u\n"    , pMibVoIPMediaProfile->SilenceSuppression4thOrder);
#if 0
            extern CODEC_PREDED_T codec_preced[];

            codec_preced[chid].CodecSelection1stOrder        = pMibVoIPMediaProfile->CodecSelection1stOrder;
            codec_preced[chid].PacketPeriodSelection1stOrder = pMibVoIPMediaProfile->PacketPeriodSelection1stOrder;
            codec_preced[chid].SilenceSuppression1stOrder    = pMibVoIPMediaProfile->SilenceSuppression1stOrder;

            codec_preced[chid].CodecSelection2ndOrder        = pMibVoIPMediaProfile->CodecSelection2ndOrder;
            codec_preced[chid].PacketPeriodSelection2ndOrder = pMibVoIPMediaProfile->PacketPeriodSelection2ndOrder;
            codec_preced[chid].SilenceSuppression2ndOrder    = pMibVoIPMediaProfile->SilenceSuppression2ndOrder;

            codec_preced[chid].CodecSelection3rdOrder        = pMibVoIPMediaProfile->CodecSelection3rdOrder;
            codec_preced[chid].PacketPeriodSelection3rdOrder = pMibVoIPMediaProfile->PacketPeriodSelection3rdOrder;
            codec_preced[chid].SilenceSuppression3rdOrder    = pMibVoIPMediaProfile->SilenceSuppression3rdOrder;

            codec_preced[chid].CodecSelection4thOrder        = pMibVoIPMediaProfile->CodecSelection4thOrder;
            codec_preced[chid].PacketPeriodSelection4thOrder = pMibVoIPMediaProfile->PacketPeriodSelection4thOrder;
            codec_preced[chid].SilenceSuppression4thOrder    = pMibVoIPMediaProfile->SilenceSuppression4thOrder;

            VOICE_WRAPPER(omci_voice_codec_sel_order_set_all);
#else
            UINT8 *p_addr_base = (UINT8 *)pMibVoIPMediaProfile;
            VOICE_WRAPPER(omci_voice_codec_sel_order_set_all, chid, (p_addr_base + 5), order_size);
#endif

            tmpMibRtpProfileData.EntityId = pMibVoIPMediaProfile->RTPProfilePointer;
            if (FALSE == mib_FindEntry(MIB_TABLE_RTPPROFILEDATA_INDEX , &tmpMibRtpProfileData, &pMibRtpProfileData))
			ODBG_R("RTP profle data not found %u\n" , tmpMibRtpProfileData.EntityId);
			else{
            ODBG_G("RTP profle data Entry found. EntityId=%u\n" , tmpMibRtpProfileData.EntityId);

            ODBG_Y("LocalPortMin=%u\n" , pMibRtpProfileData->LocalPortMin);
            VOICE_WRAPPER(omci_voice_min_local_port_set, chid, pMibRtpProfileData->LocalPortMin);

            ODBG_Y("LocalPortMax=%u\n" , pMibRtpProfileData->LocalPortMax);

            ODBG_Y("DSCPMark=%u\n" , pMibRtpProfileData->DSCPMark);
            VOICE_WRAPPER(omci_voice_dscp_mark_set, pMibRtpProfileData->DSCPMark);

            // RFC4733
            ODBG_Y("PiggybackEvents=%u\n" , pMibRtpProfileData->PiggybackEvents);
            ODBG_Y("ToneEvents=%u\n" , pMibRtpProfileData->ToneEvents);
            ODBG_Y("DTMFEvents=%u\n" , pMibRtpProfileData->DTMFEvents);
            ODBG_Y("CASEvents=%u\n" , pMibRtpProfileData->CASEvents);
			}

            tmpMibVoiceServiceProfile.EntityId = pMibVoIPMediaProfile->VoiceServiceProfilePointer;
            if (FALSE == mib_FindEntry(MIB_TABLE_VOICESERVICEPROFILE_INDEX , &tmpMibVoiceServiceProfile, &pMibVoiceServiceProfile))
				ODBG_R("RTP profle data not found %u\n" , tmpMibVoiceServiceProfile.EntityId);
			else{
            ODBG_G("Voice service profile Entry found, EntityId=%u\n" , tmpMibVoiceServiceProfile.EntityId);

            ODBG_Y("AnnouncementType=%u\n" , pMibVoiceServiceProfile->AnnouncementType);

            ODBG_Y("JitterTarget=%u\n" , pMibVoiceServiceProfile->JitterTarget);
            VOICE_WRAPPER(omci_voice_jitter_set, chid, OMCI_VOIP_JITTER_TARGET,pMibVoiceServiceProfile->JitterTarget);

            ODBG_Y("JitterBufferMax=%u\n" , pMibVoiceServiceProfile->JitterBufferMax);
            VOICE_WRAPPER(omci_voice_jitter_set, chid, OMCI_VOIP_JITTER_MAX,pMibVoiceServiceProfile->JitterBufferMax);

            ODBG_Y("EchoCancelInd=%u\n" , pMibVoiceServiceProfile->EchoCancelInd);
            VOICE_WRAPPER(omci_voice_echo_cancel_ind_set, chid, pMibVoiceServiceProfile->EchoCancelInd);

            ODBG_Y("PSTNProtocolVariant=%u\n" , pMibVoiceServiceProfile->PSTNProtocolVariant);

            ODBG_Y("HookFlashMinimumTime=%u\n" , pMibVoiceServiceProfile->HookFlashMinimumTime);
            VOICE_WRAPPER(omci_voice_hook_flash_time_set, chid, OMCI_VOIP_HOOK_TIME_MIN, pMibVoiceServiceProfile->HookFlashMinimumTime);

            ODBG_Y("HookFlashMinimumTime=%u\n" , pMibVoiceServiceProfile->HookFlashMaximumTime);
            VOICE_WRAPPER(omci_voice_hook_flash_time_set, chid, OMCI_VOIP_HOOK_TIME_MAX, pMibVoiceServiceProfile->HookFlashMaximumTime);
			}
		}
            do_voice_service = TRUE;

        }
    next_voip_ctp:
		if (GOS_OK != (MIB_GetNext(MIB_TABLE_VOIPVOICECTP_INDEX, &mibVoIpVoiceCtp, sizeof(MIB_TABLE_VOIPVOICECTP_T))))
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"get next fail,count=%d",count);
	}
    if (do_voice_service)
    {
        VOICE_WRAPPER(omci_voice_config_save);
        VOICE_WRAPPER(omci_voice_service_restart);
    }
    return GOS_OK;
}

