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
 * Purpose : Definition of OMCI ME table related define
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI ME table related define
 */

#ifndef __MIB_TABLE_DEFS_H__
#define __MIB_TABLE_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "mib.h"

	/*9.1.x*/
#include "mib_Ontg.h"
#include "mib_Ont2g.h"
#include "mib_OntData.h"
#include "mib_SWImage.h"
#include "mib_Cardholder.h"
#include "mib_CircuitPack.h"
#include "mib_OnuPwrShedding.h"
#include "mib_OnuRemoteDbg.h"

	/*9.2.x*/
#include "mib_Anig.h"
#include "mib_Tcont.h"
#include "mib_GemPortCtp.h"
#include "mib_GemIwTp.h"
#include "mib_MultiGemIwTp.h"
#include "mib_GemPortPMHD.h"
#include "mib_GalEthProf.h"
#include "mib_FecPMHD.h"
#include "mib_PriQ.h"
#include "mib_Scheduler.h"
#include "mib_GemTrafficDescriptor.h"
#include "mib_GemPortNetworkCtpPMHD.h"

	/*9.3.x*/
#include "mib_MacBriServProf.h"
#include "mib_MacBriPortCfgData.h"
#include "mib_MacBridgePortPmMonitorHistoryData.h"
#include "mib_MacBridgePortFilterTable.h"
#include "mib_MacBridgePortFilterPreassign.h"
#include "mib_MacBriPortBriTblData.h"
#include "mib_Map8021pServProf.h"
#include "mib_VlanTagFilterData.h"
#include "mib_VlanTagOpCfgData.h"
#include "mib_ExtVlanTagOperCfgData.h"
#include "mib_Dot1RateLimiter.h"
#include "mib_McastOperProf.h"
#include "mib_McastSubConfInfo.h"
#include "mib_McastSubMonitor.h"
#include "mib_EthPmDataUs.h"
#include "mib_EthPmDataDs.h"
#include "mib_EthExtPmData.h"

	/*9.4.x*/
#include "mib_IpHostConfigData.h"
#include "mib_TcpUdpCfgData.h"

	/*9.5.x*/
#include "mib_EthUni.h"
#include "mib_EthPmHistoryData.h"
#include "mib_EthPmData2.h"
#include "mib_EthPmData3.h"
#include "mib_VEIP.h"

	/*9.6.x*/
#include "mib_Pptp80211Uni.h"

	/*9.8.x*/
#include "mib_RTPPseudowireParameters.h"
#include "mib_PseudowireMaintenanceProfile.h"

	/*9.9.x*/
#include "mib_PotsUni.h"
#include "mib_SIPUserData.h"
#include "mib_SIPAgentConfigData.h"
#include "mib_VoIPVoiceCTP.h"
#include "mib_VoIPMediaProfile.h"
#include "mib_VoiceServiceProfile.h"
#include "mib_RTPProfileData.h"
#include "mib_VoIPApplicationServiceProfile.h"
#include "mib_VoIPFeatureAccessCodes.h"
#include "mib_NetworkDialPlanTable.h"
#include "mib_VoIPLineStatus.h"
#include "mib_CallControlPerformanceMonitoringHistoryData.h"
#include "mib_RTPPerformanceMonitoringHistoryData.h"
#include "mib_SIPAgentPerformanceMonitoringHistoryData.h"
#include "mib_SIPCallInitiationPerformanceMonitoringHistoryData.h"
#include "mib_VoIPConfigData.h"
#include "mib_SIPConfigPortal.h"
#include "mib_PhysicalPathTerminationPointISDNUNI.h"
#include "mib_MgcCfgData.h"
#include "mib_MGCConfigPortal.h"

	/*9.12.x*/
#include "mib_Unig.h"
#include "mib_OltG.h"
#include "mib_Network_Addr.h"
#include "mib_Authen_Sec_Method.h"
#include "mib_LargeString.h"
#include "mib_ThresholdData1.h"
#include "mib_ThresholdData2.h"
#include "mib_OctetString.h"
#include "mib_GeneralPurposeBuffer.h"
#include "mib_GenericPortal.h"
#include "mib_TR069ManageServer.h"
#include "mib_Omci.h"

	/*9.12.x*/
#include "mib_PptpVideoUni.h"
#include "mib_PptpVideoAni.h"

    /*9.13.x*/
#include "mib_LctUni.h"

	/*others*/
#include "mib_VoipImage.h"
#include "mib_LoidAuth.h"
#include "mib_OnuLoopDetection.h"
#include "mib_OnuCapability.h"
#include "mib_ExtendedMcastOperProf.h"
#include "mib_ExtendedOnuG.h"
#include "mib_LoopDetect.h"
#include "mib_OltLocationCfgData.h"
#include "mib_ExtendedIpHostCfgData.h"
#include "mib_OntSelfLoopDetect.h"
#include "mib_OntSystemMgmt.h"
#include "mib_PrivateTellionOntStatistics.h"

    /*rtk private for IOT */
#include "mib_PrivateVlanCfg.h"
#include "mib_PrivateTqCfg.h"


	/*9.1.x*/
UINT32 MIB_TABLE_ONTG_INDEX;
UINT32 MIB_TABLE_ONT2G_INDEX;
UINT32 MIB_TABLE_ONTDATA_INDEX;
UINT32 MIB_TABLE_SWIMAGE_INDEX;
UINT32 MIB_TABLE_CARDHOLDER_INDEX;
UINT32 MIB_TABLE_CIRCUITPACK_INDEX;
UINT32 MIB_TABLE_ONU_PWR_SHEDDING_INDEX;
UINT32 MIB_TABLE_ONU_REMOTE_DBG_INDEX;

	/*9.2.x*/
UINT32 MIB_TABLE_ANIG_INDEX;
UINT32 MIB_TABLE_TCONT_INDEX;
UINT32 MIB_TABLE_GEMPORTCTP_INDEX;
UINT32 MIB_TABLE_GEMIWTP_INDEX;
UINT32 MIB_TABLE_MULTIGEMIWTP_INDEX;
UINT32 MIB_TABLE_GEM_PORT_PMHD_INDEX;
UINT32 MIB_TABLE_GALETHPROF_INDEX;
UINT32 MIB_TABLE_FEC_PMHD_INDEX;
UINT32 MIB_TABLE_PRIQ_INDEX;
UINT32 MIB_TABLE_SCHEDULER_INDEX;
UINT32 MIB_TABLE_TRAFFICDESCRIPTOR_INDEX;
UINT32 MIB_TABLE_GPNC_PMHD_INDEX;

	/*9.3.x*/
UINT32 MIB_TABLE_MACBRISERVPROF_INDEX;
UINT32 MIB_TABLE_MACBRIPORTCFGDATA_INDEX;
UINT32 MIB_TABLE_MACBRIDGEPORTPMHD_INDEX;
UINT32 MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX;
UINT32 MIB_TABLE_MACBRIDGEPORTFILTERPREASSIGN_INDEX;
UINT32 MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX;
UINT32 MIB_TABLE_MAP8021PSERVPROF_INDEX;
UINT32 MIB_TABLE_VLANTAGFILTERDATA_INDEX;
UINT32 MIB_TABLE_VLANTAGOPCFGDATA_INDEX;
UINT32 MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX;
UINT32 MIB_TABLE_DOT1_RATE_LIMITER_INDEX;
UINT32 MIB_TABLE_MCASTOPERPROF_INDEX;
UINT32 MIB_TABLE_MCASTSUBCONFINFO_INDEX;
UINT32 MIB_TABLE_MCASTSUBMONITOR_INDEX;
UINT32 MIB_TABLE_ETHPMDATAUS_INDEX;
UINT32 MIB_TABLE_ETHPMDATADS_INDEX;
UINT32 MIB_TABLE_ETHEXTPMDATA_INDEX;

	/*9.4.x*/
UINT32 MIB_TABLE_IP_HOST_CFG_DATA_INDEX;
UINT32 MIB_TABLE_TCP_UDP_CFG_DATA_INDEX;

	/*9.5.x*/
UINT32 MIB_TABLE_ETHUNI_INDEX;
UINT32 MIB_TABLE_ETHPMHISTORYDATA_INDEX;
UINT32 MIB_TABLE_ETHPMDATA2_INDEX;
UINT32 MIB_TABLE_ETHPMDATA3_INDEX;
UINT32 MIB_TABLE_VEIP_INDEX;

	/*9.6.x*/
UINT32 MIB_TABLE_PPTP_80211_UNI_INDEX;

	/*9.8.x*/
UINT32 MIB_TABLE_RTPPSEUDOWIREPARAMETERS_INDEX;
UINT32 MIB_TABLE_PSEUDOWIREMAINTENANCEPROFILE_INDEX;

	/*9.9.x*/
UINT32 MIB_TABLE_POTSUNI_INDEX;
UINT32 MIB_TABLE_SIPUSERDATA_INDEX;
UINT32 MIB_TABLE_SIPAGENTCONFIGDATA_INDEX;
UINT32 MIB_TABLE_VOIPVOICECTP_INDEX;
UINT32 MIB_TABLE_VOIPMEDIAPROFILE_INDEX;
UINT32 MIB_TABLE_VOICESERVICEPROFILE_INDEX;
UINT32 MIB_TABLE_RTPPROFILEDATA_INDEX;
UINT32 MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_INDEX;
UINT32 MIB_TABLE_VOIPFEATUREACCESSCODES_INDEX;
UINT32 MIB_TABLE_NETWORKDIALPLANTABLE_INDEX;
UINT32 MIB_TABLE_VOIPLINESTATUS_INDEX;
UINT32 MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INDEX;
UINT32 MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INDEX;
UINT32 MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INDEX;
UINT32 MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INDEX;
UINT32 MIB_TABLE_VOIPCONFIGDATA_INDEX;
UINT32 MIB_TABLE_SIPCONFIGPORTAL_INDEX;
UINT32 MIB_TABLE_PHYSICALPATHTERMINATIONPOINTISDNUNI_INDEX;
UINT32 MIB_TABLE_MGCCFGDATA_INDEX;
UINT32 MIB_TABLE_MGCCONFIGPORTAL_INDEX;

	/*9.12.x*/
UINT32 MIB_TABLE_UNIG_INDEX;
UINT32 MIB_TABLE_OLTG_INDEX;
UINT32 MIB_TABLE_NETWORK_ADDR_INDEX;
UINT32 MIB_TABLE_AUTH_SEC_METHOD_INDEX;
UINT32 MIB_TABLE_LARGE_STRING_INDEX;
UINT32 MIB_TABLE_THRESHOLDDATA1_INDEX;
UINT32 MIB_TABLE_THRESHOLDDATA2_INDEX;
UINT32 MIB_TABLE_OCTET_STRING_INDEX;
UINT32 MIB_TABLE_GENERAL_PURPOSE_BUFFER_INDEX;
UINT32 MIB_TABLE_GENERIC_STATUS_PORTAL_INDEX;
UINT32 MIB_TABLE_TR069MANAGESERVER_INDEX;
UINT32 MIB_TABLE_OMCI_INDEX;

	/*9.13.x*/
UINT32 MIB_TABLE_PPTP_VIDEO_UNI_INDEX;
UINT32 MIB_TABLE_PPTP_VIDEO_ANI_INDEX;
UINT32 MIB_TABLE_PPTP_LCT_UNI_INDEX;

	/*others*/
UINT32 MIB_TABLE_VOIP_IMAGE_INDEX;
UINT32 MIB_TABLE_LOIDAUTH_INDEX;
UINT32 MIB_TABLE_ONULOOPDETECTION_INDEX;
UINT32 MIB_TABLE_ONUCAPABILITY_INDEX;
UINT32 MIB_TABLE_EXTMCASTOPERPROF_INDEX;
UINT32 MIB_TABLE_EXTENDED_ONU_G_INDEX;
UINT32 MIB_TABLE_LOOP_DETECT_INDEX;
UINT32 MIB_TABLE_OLT_LOCATION_CFG_DATA_INDEX;
UINT32 MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_INDEX;
UINT32 MIB_TABLE_ONT_SELFLOOPDETECT_INDEX;
UINT32 MIB_TABLE_ONT_SYSTEM_MANAGEMENT;
UINT32 MIB_TABLE_PRIVATE_TELLION_ONT_STATISTICS_INDEX;

	/*rtk private for IOT */
UINT32 MIB_TABLE_PRIVATE_VLANCFG_INDEX;
UINT32 MIB_TABLE_PRIVATE_TQCFG_INDEX;

UINT32 MIB_TABLE_ME242_INDEX;
UINT32 MIB_TABLE_ME243_INDEX;
UINT32 MIB_TABLE_ME373_INDEX;
UINT32 MIB_TABLE_ME350_INDEX;
UINT32 MIB_TABLE_ME370_INDEX;

UINT32 MIB_TABLE_LAST_FAKE_INDEX;


#ifdef __cplusplus
}
#endif

#endif
