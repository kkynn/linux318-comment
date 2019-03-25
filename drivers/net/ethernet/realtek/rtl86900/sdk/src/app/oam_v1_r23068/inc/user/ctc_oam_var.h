/*
 * Copyright (C) 2012 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 39101 $
 * $Date: 2013-05-03 17:35:27 +0800 (週五, 03 五月 2013) $
 *
 * Purpose : 
 *
 * Feature : 
 *
 */

#ifndef __CTC_OAM_VAR_H__
#define __CTC_OAM_VAR_H__

/*
 * Include Files
 */

/* 
 * Symbol Definition 
 */
/* CTC reply definintions */
#define CTC_OAM_VAR_RESP_SETOK                      0x80
#define CTC_OAM_VAR_RESP_VARBADPARAM                0x86
#define CTC_OAM_VAR_RESP_VARNORESOURCE              0x87
/* Variable definitions */
/* 0xC7/0x0004 - ONU capabilities-1 */
#define CTC_OAM_VAR_ONUCAPABILITES1_PORTMAP_LEN     8
/* 0x07/0x0025 - aPhyAdminState */
#define CTC_OAM_VAR_APHYADMINSTATE_LEN              4
#define CTC_OAM_VAR_APHYADMINSTATE_DISABLE          0x00000001UL
#define CTC_OAM_VAR_APHYADMINSTATE_ENABLE           0x00000002UL
/* 0x07/0x004F - aAutoNegAdminState */
#define CTC_OAM_VAR_AAUTONEGADMINSTATE_LEN          4
#define CTC_OAM_VAR_AAUTONEGADMINSTATE_DISABLE      0x00000001UL
#define CTC_OAM_VAR_AAUTONEGADMINSTATE_ENABLE       0x00000002UL
/* 0x07/0x0139 - aFECAbility */
#define CTC_OAM_VAR_AFECABILITY_LEN                 4
#define CTC_OAM_VAR_AFECABILITY_UNKNOWN             0x00000001UL
#define CTC_OAM_VAR_AFECABILITY_SUPPORTED           0x00000002UL
#define CTC_OAM_VAR_AFECABILITY_NOTSUPPORT          0x00000003UL
/* 0x07/0x013A - aFECmode */
#define CTC_OAM_VAR_AFECMODE_LEN                    4
#define CTC_OAM_VAR_AFECMODE_UNKNOWN                0x00000001UL
#define CTC_OAM_VAR_AFECMODE_ENABLED                0x00000002UL
#define CTC_OAM_VAR_AFECMODE_DISABLED               0x00000003UL
/* 0x09/0x0005 acPhyAdminControl */
#define CTC_OAM_VAR_ACPHYADMINCONTROL_LEN           4
#define CTC_OAM_VAR_ACPHYADMINCONTROL_DEACTIVE      0x00000001UL
#define CTC_OAM_VAR_ACPHYADMINCONTROL_ACTIVE        0x00000002UL
/* 0x09/0x000B - acAutoNegRestartAutoConfig */
#define CTC_OAM_VAR_ACAUTONEGRESTARTAUTOCONFIG_LEN  0
/* 0x09/0x000C - acAutoNegAdminControl */
#define CTC_OAM_VAR_ACAUTONEGADMINCONTROL_LEN       4
#define CTC_OAM_VAR_ACAUTONEGADMINCONTROL_DEACTIVE  0x00000001UL
#define CTC_OAM_VAR_ACAUTONEGADMINCONTROL_ACTIVE    0x00000002UL
/* 0xC7/0x0008 - HoldoverConfig */
#define CTC_OAM_VAR_HOLDOVERCONFIG_LEN              8
#define CTC_OAM_VAR_HOLDOVERCONFIG_DEACTIVE         0x00000001UL
#define CTC_OAM_VAR_HOLDOVERCONFIG_ACTIVE           0x00000002UL
/* 0x/C7/0x000A - MxUMngSNMPParameter */
#define CTC_OAM_VAR_MXUMNGSNMPPARAMETER_IPV4_LEN			105
#define CTC_OAM_VAR_MXUMNGSNMPPARAMETER_IPV6_LEN			117
/* 0xC7/0x000B - Active PON_IFAdminstate */
#define CTC_OAM_VAR_ACTIVEPONIFADMINSTATE_LEN       1
#define CTC_OAM_VAR_ACTIVEPONIFADMINSTATE_PONNUM    0x00
/* 0xC7/0x000C - ONU capabilities-3 */
#define CTC_OAM_VAR_ONUCAPABILITES_LEN              3
/* 0xC7/0x000D - ONU power saving capabilities */
#define CTC_OAM_VAR_ONUPOWERSAVINGCAPABILITES_LEN   2
#define CTC_OAM_VAR_ONUPOWERSAVINGCAPABILITES_TX_ONLY   1
/* 0xC7/0x000E - ONU power saving config */
#define CTC_OAM_VAR_ONUPOWERSAVINGCONFIG_LEN        7
#define CTC_OAM_VAR_ONUPOWERSAVINGCONFIG_UNSUPPORT  0xFF
/* 0xC7/0x000E - ONU Protection Parameters */
#define CTC_OAM_VAR_ONUPROTECTIONPARAMETERS_LEN     4
#define CTC_OAM_VAR_ONUPROTECTIONPARAMETERS_LOSOPT  2
#define CTC_OAM_VAR_ONUPROTECTIONPARAMETERS_LOSMAC  50
/* 0xC7/0x0011 - EthLinkState */
#define CTC_OAM_VAR_ETHLINKSTATE_LEN                1
#define CTC_OAM_VAR_ETHLINKSTATE_DOWN               0x00
#define CTC_OAM_VAR_ETHLINKSTATE_UP                 0x01
/* 0xC7/0x0012 - EthPortPause */
#define CTC_OAM_VAR_ETHPORTPAUSE_LEN                1
#define CTC_OAM_VAR_ETHPORTPAUSE_DISABLE            0x00
#define CTC_OAM_VAR_ETHPORTPAUSE_ENABLE             0x01
/* 0xC7/0x0013 - ethPortUs Policing */
#define CTC_OAM_VAR_ETHERPORTUSPOLICING_ENABLE_LEN  10
#define CTC_OAM_VAR_ETHERPORTUSPOLICING_DISABLE_LEN 1
#define CTC_OAM_VAR_ETHERPORTUSPOLICING_DISABLE     0
#define CTC_OAM_VAR_ETHERPORTUSPOLICING_ENABLE      1
/* 0xC7/0x0016 - EthPortDs RateLimiting */
#define CTC_OAM_VAR_ETHERPORTDSRATELIMITING_ENABLE_LEN      7
#define CTC_OAM_VAR_ETHERPORTDSRATELIMITING_DISABLE_LEN     1
#define CTC_OAM_VAR_ETHERPORTDSRATELIMITING_DISABLE         0
#define CTC_OAM_VAR_ETHERPORTDSRATELIMITING_ENABLE          1
/* 0xC7/0x0017 - PortLoopDetect */
#define CTC_OAM_VAR_PORTLOOPDETECT_LEN              4
#define CTC_OAM_VAR_PORTLOOPDETECT_DEACTIVE         0x00000001UL
#define CTC_OAM_VAR_PORTLOOPDETECT_ACTIVE           0x00000002UL
/* 0xC7/0x0018 - PortDisableLooped */
#define CTC_OAM_VAR_PORTDISABLELOOPED_LEN           4
#define CTC_OAM_VAR_PORTDISABLELOOPED_DISABLE       0x00000000UL
#define CTC_OAM_VAR_PORTDISABLELOOPED_ENABLE        0x00000001UL
/* 0xC7/0x0019 - PortLoopParameterConfig */
#define CTC_OAM_VAR_PORTLOOPPARAMETERCONFIG_MIN_LEN     8
#define CTC_OAM_VAR_PORTLOOPPARAMETERCONFIG_MAX_VLAN_NUM 16
/* 0xC7/0x00A4 - PortMACAgingTime */
#define CTC_OAM_VAR_PORTMACAGINGTIME_LEN            4
#define CTC_OAM_VAR_PORTMACAGINGTIME_DEACTIVE       0x00000000UL
/* 0xC7/0x0021 - VLAN */
#define CTC_OAM_VAR_VLAN_MODE_LEN                   1
#define CTC_OAM_VAR_VLAN_TAG_MODE_LEN               (CTC_OAM_VAR_VLAN_MODE_LEN + 4)
#define CTC_OAM_VAR_VLAN_TRANSLATION_MODE_MIN       (CTC_OAM_VAR_VLAN_MODE_LEN + 4)
#define CTC_OAM_VAR_VLAN_TRANSLATION_ENTRY_LEN      (8)
#define CTC_OAM_VAR_VLAN_AGGREGATION_MODE_MIN       (CTC_OAM_VAR_VLAN_MODE_LEN + 4 + 2)
#define CTC_OAM_VAR_VLAN_AGGREGATION_ENTRY_LEN      (4)
#define CTC_OAM_VAR_VLAN_TRUNK_MODE_MIN             (CTC_OAM_VAR_VLAN_MODE_LEN + 4)
#define CTC_OAM_VAR_VLAN_TRUNK_ENTRY_LEN            (4)
#define CTC_OAM_VAR_VLAN_MODE_TRANSPARENT           0x00
#define CTC_OAM_VAR_VLAN_MODE_TAG                   0x01
#define CTC_OAM_VAR_VLAN_MODE_TRANSLATION           0x02
#define CTC_OAM_VAR_VLAN_MODE_AGGREGATION           0x03
#define CTC_OAM_VAR_VLAN_MODE_TRUNK                 0x04
/* 0xC7/0x0031 - Classification&Marking */
#define CTC_OAM_VAR_CLASSIFY_ACTION_DEL     		0x00
#define CTC_OAM_VAR_CLASSIFY_ACTION_ADD     		0x01
#define CTC_OAM_VAR_CLASSIFY_ACTION_CLR     		0x02
#define CTC_OAM_VAR_CLASSIFY_ACTION_SHOW    		0x03
#define CTC_OAM_VAR_CLASSIFY_MAX_ENTRY      		128

#define CTC_OAM_CLF_EQUAL           				0x1
#define CTC_OAM_CLF_ALWAYS_MATCH    				0x7
/* 0xC7/0x0041 - Add/Del Multicast VLAN */
#define CTC_OAM_VAR_MCAST_VLAN_DEL   			0x0
#define CTC_OAM_VAR_MCAST_VLAN_ADD   			0x1
#define CTC_OAM_VAR_MCAST_VLAN_CLEAR 			0x2
#define CTC_OAM_VAR_MCAST_VLAN_LIST  			0x3
/* 0xC7/0x0042 - CTC MCAST VLAN tag operation definition */
#define CTC_OAM_VAR_MCAST_VLAN_TAG_LEN          1

#define CTC_OAM_VAR_MCAST_VLAN_TAG_NOT_STRIP 	0x0
#define CTC_OAM_VAR_MCAST_VLAN_TAG_STRIP     	0x1
#define CTC_OAM_VAR_MCAST_VLAN_TAG_TRANLATE  	0x2
/* 0xC7/0x0042 - MulticastSwitch */
#define CTC_OAM_VAR_MCAST_SWITCH_LEN            1
/* 0xC7/0x0044 - CTC MCAST entry operation definition */
#define CTC_OAM_VAR_MCAST_ENTRY_DEL   			0x0
#define CTC_OAM_VAR_MCAST_ENTRY_ADD   			0x1
#define CTC_OAM_VAR_MCAST_ENTRY_CLEAR 			0x2
#define CTC_OAM_VAR_MCAST_ENTRY_LIST  			0x3

#define MAX_GROUP_NUM_PER_PORT 64

/* 0xC7/0x0045 - Group Num Max */
#define CTC_OAM_VAR_MCAST_GRP_NUM_MAX_LEN		1
/* 0xC7/0x0046 - aFastLeaveAbility */
#define CTC_OAM_VAR_AFASTLEAVEABILITY_HDR_LEN       4
#define CTC_OAM_VAR_AFASTLEAVEABILITY_ENUM_LEN      4
#define CTC_OAM_VAR_AFASTLEAVEABILITY_SNOOPING_NONFASTLEAVE     0x00000000UL
#define CTC_OAM_VAR_AFASTLEAVEABILITY_SNOOPING_FASTLEAVE        0x00000001UL
#define CTC_OAM_VAR_AFASTLEAVEABILITY_IGMP_NONFASTLEAVE         0x00000010UL
#define CTC_OAM_VAR_AFASTLEAVEABILITY_IGMP_FASTLEAVE            0x00000011UL
#define CTC_OAM_VAR_AFASTLEAVEABILITY_MLD_NONFASTLEAVE          0x00000002UL
#define CTC_OAM_VAR_AFASTLEAVEABILITY_MLD_FASTLEAVE             0x00000003UL
/* 0xC7/0x0047 - aFastLeaveAdminState */
#define CTC_OAM_VAR_AFASTLEAVEADMINSTATE_LEN        4
#define CTC_OAM_VAR_AFASTLEAVEADMINSTATE_DISABLED   0x00000001UL
#define CTC_OAM_VAR_AFASTLEAVEADMINSTATE_ENABLED    0x00000002UL
/* 0xC7/0x0081 - AlarmAdminState */
#define CTC_OAM_VAR_ALARMADMINSTATE_LEN             6
#define CTC_OAM_VAR_ALARMADMINSTATE_DISABLE         0x00000001UL 
#define CTC_OAM_VAR_ALARMADMINSTATE_ENABLE          0x00000002UL
/* 0xC7/0x0081 - AlarmThreshold */
#define CTC_OAM_VAR_ALARMTHRESHOLD_LEN              10
#define CTC_OAM_VAR_ALARMTHRESHOLD_GET_LEN          2
/* 0xC7/0x00A1 - ONUTxPowerSupplyControl */
#define CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_LEN     12
#define CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_ACTION_REENABLE 	0x0
#define CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_ACTION_SHUTDOWN 	0xFFFF
#define CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_OPTICALID_MAIN  	0x0
#define CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_OPTICALID_BACKUP  	0x1
#define CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_OPTICALID_BOTH  	0x2
/* 0xC7/0x00B1 - performanceMonitoringStatus */
#define CTC_OAM_VAR_PERFORMANCEMONITORINGSTATUS_LEN         6
#define CTC_OAM_VAR_PERFORMANCEMONITORINGSTATUS_DISABLED    0x00000001UL
#define CTC_OAM_VAR_PERFORMANCEMONITORINGSTATUS_ENABLED     0x00000002UL
/* 0xC7/0x00B2 - performanceMonitoringDataCurrent */
#define CTC_OAM_VAR_PERFORMANCEMONITORINGDATACURRENT_LEN        296 /* 128 + 128 + 40 */
/* 0xC7/0x006B - POTSStatus */
#define CTC_OAM_VAR_POTSSTATUS_LEN					12
#define CTC_OAM_VAR_POTSSTATUS_PORT_REGISTER		0
#define CTC_OAM_VAR_POTSSTATUS_PORT_IDLE			1
#define CTC_OAM_VAR_POTSSTATUS_PORT_OFFHOOK			2
#define CTC_OAM_VAR_POTSSTATUS_PORT_DIAL			3
#define CTC_OAM_VAR_POTSSTATUS_PORT_RING			4
#define CTC_OAM_VAR_POTSSTATUS_PORT_RINGBACK		5
#define CTC_OAM_VAR_POTSSTATUS_PORT_CONNECTING		6
#define CTC_OAM_VAR_POTSSTATUS_PORT_CONNECTTED		7
#define CTC_OAM_VAR_POTSSTATUS_PORT_RELEASE			8
#define CTC_OAM_VAR_POTSSTATUS_PORT_REGISTERFAIL	9
#define CTC_OAM_VAR_POTSSTATUS_PORT_NOTAUTH			10
#define CTC_OAM_VAR_POTSSTATUS_SERVICE_ENDLOCAL		0
#define CTC_OAM_VAR_POTSSTATUS_SERVICE_ENDREMOTE	1
#define CTC_OAM_VAR_POTSSTATUS_SERVICE_ENDAUTO		2
#define CTC_OAM_VAR_POTSSTATUS_SERVICE_NORMAL		3

/* 0xC9/0x0002 - Sleep Control */
#define CTC_OAM_VAR_SLEEPCONTROL_DURATION_LEN         10
#define CTC_OAM_VAR_SLEEPCONTROL_DURATION_MAX  0x3B9ACA0  /* vlaue is 1 second (unit of TQ, 1TQ = 16ns) */
#define CTC_OAM_VAR_SLEEPCONTROL_DURATION_SLEEP_DISABLE		0x00
#define CTC_OAM_VAR_SLEEPCONTROL_DURATION_SLEEP_ENABLE		0x01
#define CTC_OAM_VAR_SLEEPCONTROL_DURATION_SLEEP_SET			0x02	
#define CTC_OAM_VAR_SLEEPCONTROL_DURATION_SLEEP_MODE_NONE	0x00
#define CTC_OAM_VAR_SLEEPCONTROL_DURATION_SLEEP_MODE_TX		0x01  /* Tx sleep mode only */
#define CTC_OAM_VAR_SLEEPCONTROL_DURATION_SLEEP_MODE_TRX	0x02  /* Tx and Rx sleep mode */

/*
 * Macro Definition
 */

/*  
 * Function Declaration  
 */
extern int ctc_oam_varCb_dummy_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x07/0x0025 - aPhyAdminState */
extern int ctc_oam_varCb_aPhyAdminState_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x07/0x004F - aAutoNegAdminState */
extern int ctc_oam_varCb_aAutoNegAdminState_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x07/0x0052 - aAutoNegLocalTechnologyAbility */
extern int ctc_oam_varCb_aAutoNegLocalTechnologyAbility_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x07/0x0053 - aAutoNegAdvertisedTechnologyAbility */
extern int ctc_oam_varCb_aAutoNegAdvertisedTechnologyAbility_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x07/0x0139 - aFECAbility */
extern int ctc_oam_varCb_aFECAbility_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x07/0x013A - aFECmode */
extern int ctc_oam_varCb_aFecMode_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x07/0x013A - aFECmode */
extern int ctc_oam_varCb_aFecMode_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x09/0x0005 - acPhyAdminControl */
extern int ctc_oam_varCb_acPhyAdminControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x09/0x000B - acAutoNegRestartAutoConfig */
int ctc_oam_varCb_acAutoNegRestartAutoConfig_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0x09/0x000C - acAutoNegAdminControl */
extern int ctc_oam_varCb_acAutoNegAdminControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0xC7/0x0001 - ONU SN */
extern int ctc_oam_varCb_onuSn_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0002 - FirmwareVer */
extern int ctc_oam_varCb_firmwareVer_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0003 - Chipset ID */
extern int ctc_oam_varCb_chipsetId_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0004 - ONU capabilities-1 */
extern int ctc_oam_varCb_onuCapabilities1_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0005 - OpticalTransceiverDiagnosis */
extern int ctc_oam_varCb_opticalTransceiverDiagnosis_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0006 - Service SLA */
extern int ctc_oam_varCb_serviceSla_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0006 - Service SLA */
extern int ctc_oam_varCb_serviceSla_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0007 - ONU capabilities-2 */
extern int ctc_oam_varCb_onuCapabilities2_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0008 - HoldoverConfig */
extern int ctc_oam_varCb_holdoverConfig_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0008 - HoldoverConfig */
extern int ctc_oam_varCb_holdoverConfig_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0009 - MxUMngGlobalParameter */
extern int ctc_oam_varCb_mxUMngGlobalParameter_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0009 - MxUMngGlobalParameter */
extern int ctc_oam_varCb_mxUMngGlobalParameter_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000A - MxUMngSNMPParameter */
extern int ctc_oam_varCb_mxUMngSNMPParameter_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000A - MxUMngSNMPParameter */
extern int ctc_oam_varCb_mxUMngSNMPParameter_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000B - Active PON_IFAdminstate */
extern int ctc_oam_varCb_activePonIfAdminstate_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000B - Active PON_IFAdminstate */
extern int ctc_oam_varCb_activePonIfAdminstate_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000C - ONU capabilities-3 */
extern int ctc_oam_varCb_onuCapabilities3_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000D - ONU power saving capabilities */
int ctc_oam_varCb_onuPowerSavingCapabilities_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000E - ONU power saving config */
int ctc_oam_varCb_onuPowerSavingConfig_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000E - ONU power saving config */
int ctc_oam_varCb_onuPowerSavingConfig_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000F - ONU Protection Parameters */
int ctc_oam_varCb_onuProtectionParameters_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0011 - EthLinkState */
int ctc_oam_varCb_ethLinkState_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x000F - ONU Protection Parameters */
int ctc_oam_varCb_onuProtectionParameters_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0012 - EthPortPause */
extern int ctc_oam_varCb_ethPortPause_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0012 - ethPort Pause */
extern int ctc_oam_varCb_ethPortPause_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0013 - ethPortUs Policing */
extern int ctc_oam_varCb_ethPortUsPolicing_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0013 - ethPortUs Policing */
extern int ctc_oam_varCb_ethPortUsPolicing_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0014 - voip port */
int ctc_oam_varCb_voipPort_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0014 - voip port */
int ctc_oam_varCb_voipPort_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */


/* 0xC7/0x0016 - EthPortDs RateLimiting */
int ctc_oam_varCb_ethPortDsRateLimiting_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0016 - EthPortDs RateLimiting */
extern int ctc_oam_varCb_ethPortDsRateLimiting_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0xC7/0x0017 - PortLoopDetect */
extern int ctc_oam_varCb_portLoopDetect_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
                                          
/* 0xC7/0x0018 - PortDisableLooped */
extern int ctc_oam_varCb_portDisableLooped_set(
  unsigned char llidIdx, /* LLID index of the incoming operation */
  unsigned char op, /* Operation to be taken */
  ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
  ctc_varDescriptor_t varDesc,
  ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
										* handler won't need to allocate anything
										*/

/* 0xC7/0x0019 - PortLoopParameterConfig */
extern int ctc_oam_varCb_portLoopParameterConfig_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0xC7/0x00A4 - PortMACAgingTime */
extern int ctc_oam_varCb_portMacAgingTime_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x00A4 - PortMACAgingTime */
extern int ctc_oam_varCb_portMacAgingTime_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0xC7/0x0021 - VLAN */
extern int ctc_oam_varCb_vlan_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0021 - VLAN */
extern int ctc_oam_varCb_vlan_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0031 - Classification&Marking */
extern int ctc_oam_varCb_classificationMarking_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0031 - Classification&Marking */
extern int ctc_oam_varCb_classificationMarking_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                         */

/* 0xC7/0x0041 - Add/Del Multicast VLAN */
extern int ctc_oam_varCb_addDelMulticastVlan_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0041 - Add/Del Multicast VLAN */
extern int ctc_oam_varCb_addDelMulticastVlan_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0042 - MulticastTagOper */
extern int ctc_oam_varCb_multicastTagOper_get(
  unsigned char llidIdx, /* LLID index of the incoming operation */
  unsigned char op, /* Operation to be taken */
  ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
  ctc_varDescriptor_t varDesc,
  ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
										* caller will release it
										*/

/* 0xC7/0x0042 - MulticastTagOper */
extern int ctc_oam_varCb_multicastTagOper_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0043 - MulticastSwitch */
extern int ctc_oam_varCb_multicastSwitch_get(
  unsigned char llidIdx, /* LLID index of the incoming operation */
  unsigned char op, /* Operation to be taken */
  ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
  ctc_varDescriptor_t varDesc,
  ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
										* caller will release it
										*/

/* 0xC7/0x0043 - MulticastSwitch */
extern int ctc_oam_varCb_multicastSwitch_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0044 - MulticastSwitch */
extern int ctc_oam_varCb_multicastControl_get(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	unsigned char op, /* Operation to be taken */
	ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
	ctc_varDescriptor_t varDesc,
	ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
										  * caller will release it
										  */

/* 0xC7/0x0044 - MulticastSwitch */
extern int ctc_oam_varCb_multicastControl_set(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	unsigned char op, /* Operation to be taken */
	ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
	ctc_varDescriptor_t varDesc,
	ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
										* caller will release it
										*/

/* 0xC7/0x0045 - Group Num Max */
extern int ctc_oam_varCb_groupNumMax_get(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	unsigned char op, /* Operation to be taken */
	ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
	ctc_varDescriptor_t varDesc,
	ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
										  * caller will release it
										  */

/* 0xC7/0x0045 - Group Num Max */
extern int ctc_oam_varCb_groupNumMax_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */
/* 0xC7/0x0046 - aFastLeaveAbility */
extern int ctc_oam_varCb_aFastLeaveAbility_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x0047 - aFastLeaveAdminState */
extern int ctc_oam_varCb_aFastLeaveAdminState_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

/* 0xC7/0x006B - POTSStatus */
int ctc_oam_varCb_potsStatus_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler allocate data field in container, 
                                          * caller will release it
                                          */

/* 0xC7/0x0081 - AlarmAdminState */
extern int ctc_oam_varCb_alarmAdminState_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
/* 0xC7/0x0082 - AlarmThreshold */
extern int ctc_oam_varCb_alarmThreshold_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler should allocate resource, 
                                          * caller will release it
                                          */

extern int ctc_oam_varCb_alarmThreshold_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0xC7/0x00A1 - ONUTxPowerSupplyControl */
extern int ctc_oam_varCb_txPowerSupplyControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0xC7/0x00B1 - performanceMonitoringStatus */
extern int ctc_oam_varCb_performanceMonitoringStatus_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* handler allocate data field in container, 
                                          * caller will release it
                                          */

/* 0xC7/0x00B1 - performanceMonitoringStatus */
extern int ctc_oam_varCb_performanceMonitoringStatus_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0xC7/0x00B2 - performanceMonitoringDataCurrent */
extern int ctc_oam_varCb_performanceMonitoringDataCurrent_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler allocate data field in container, 
                                          * caller will release it
                                          */

/* 0xC7/0x00B2 - performanceMonitoringDataCurrent */
extern int ctc_oam_varCb_performanceMonitoringDataCurrent_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

/* 0xC7/0x00B3 - performanceMonitoringDataHistory */
extern int ctc_oam_varCb_performanceMonitoringDataHistory_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);    /* handler allocate data field in container, 
                                          * caller will release it
                                          */

/* 0xC7/0x801f - configVlanOfTr069Wan */
extern int ctc_oam_varCb_configVlanOfTr069Wan_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* handler allocate data field in container, 
                                          * caller will release it
                                          */
										 
/* 0xC7/0x801f - configVlanOfTr069Wan */
extern int ctc_oam_varCb_configVlanOfTr069Wan_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */										  

/* 0xC9/0x0001 - Reset ONU */
extern int ctc_oam_varCb_resetOnu_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

extern int ctc_oam_varCb_sleepControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
                                          
/* 0xC9/0x0048 - acFastLeaveAdminControl */
extern int ctc_oam_varCb_acFastLeaveAdminControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */

extern int ctc_oam_churningKey_set(
    unsigned char llidIdx,
    unsigned char keyIdx,
    unsigned char key[]);

extern int ctc_oam_dbaConfig_get(
    ctc_dbaThreshold_t *pDbaThreshold);

extern int ctc_oam_dbaConfig_set(
    ctc_dbaThreshold_t *pDbaSetResp);

#endif /* __CTC_OAM_VAR_H__ */

