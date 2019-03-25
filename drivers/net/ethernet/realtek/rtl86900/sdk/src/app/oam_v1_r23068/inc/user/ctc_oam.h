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
 * Purpose : Define the CTC related extended OAM
 *
 * Feature : Provide CTC related extended OAM parsing and handling
 *
 */

#ifndef __CTC_OAM_H__
#define __CTC_OAM_H__

/*
 * Include Files
 */
#include "epon_oam_db.h"
#include "epon_oam_rx.h"
#include "epon_oam_config.h"

/* 
 * Symbol Definition 
 */
/* CTC extended OAM configuration */
#define CTC_OAM_SUPPORT_VERSION_01          1
#define CTC_OAM_SUPPORT_VERSION_13          1
#define CTC_OAM_SUPPORT_VERSION_20          1
#define CTC_OAM_SUPPORT_VERSION_21          1
#define CTC_OAM_SUPPORT_VERSION_30          1
#define CTC_OAM_SUPPORT_BENEATH_20          (CTC_OAM_SUPPORT_VERSION_01 | CTC_OAM_SUPPORT_VERSION_13 | CTC_OAM_SUPPORT_VERSION_20)
#define CTC_OAM_SUPPORT_ABOVE_21            (CTC_OAM_SUPPORT_VERSION_21 | CTC_OAM_SUPPORT_VERSION_30)

#define CTC_OAM_VERSION_20                  0x20
#define CTC_OAM_VERSION_30					0x30
#define CTC_OAM_OUI                         { 0x11, 0x11, 0x11 }
/* CTC OAM parsing */
/* Extended opode */
#define CTC_EXTOAM_OPCODE_RESERVED          0x00
#define CTC_EXTOAM_OPCODE_VARREQ            0x01
#define CTC_EXTOAM_OPCODE_VARRESP           0x02
#define CTC_EXTOAM_OPCODE_SETREQ            0x03
#define CTC_EXTOAM_OPCODE_SETRESP           0x04
#define CTC_EXTOAM_OPCODE_ONUAUTH           0x05
#define CTC_EXTOAM_OPCODE_SWDOWNLOAD        0x06
#define CTC_EXTOAM_OPCODE_CHURNING          0x09
#define CTC_EXTOAM_OPCODE_DBA               0x0A
#define CTC_EXTOAM_OPCODE_NOTIFICATION      0xFE
#define CTC_EXTOAM_OPCODE_EVENT             0xFF
/* Variable */
#define CTC_VAR_REQBRANCH_INSTANT20         0x36 /* Instant branch for CTC 2.0- */
#define CTC_VAR_REQBRANCH_INSTANT21         0x37 /* Instant branch for CTC 2.1+ */
#define CTC_VAR_REQBRANCH_END               0x00 /* End of variable operation */
#define CTC_VAR_REQBRANCH_STDATTR           0x07 /* Standard attribute */
#define CTC_VAR_REQBRANCH_STDACT            0x09 /* Standard action */
#define CTC_VAR_REQBRANCH_EXTATTR           0xC7 /* Extended attribute */
#define CTC_VAR_REQBRANCH_EXTACT            0xC9 /* Extended action */
/* Variable Instant available leaf for CTC 2.1+ */
#define CTC_VAR_REQBRANCH_INSTANT_NONE      0x0000 /* Internal use only */
#define CTC_VAR_REQBRANCH_INSTANT_PORT      0x0001 /* Port instant */
#define CTC_VAR_REQBRANCH_INSTANT_LLID      0x0003 /* LLID instant */
#define CTC_VAR_REQBRANCH_INSTANT_PONIF     0x0004 /* PON IF instant */
#define CTC_VAR_REQBRANCH_INSTANT_ONU       0xFFFF /* ONU instant */
/* Variable Instant available port type for CTC 2.1+ */
#define CTC_VAR_INSTANT_PORTTYPE_ETHERNET   0x0001 /* Ethernet port */
#define CTC_VAR_INSTANT_PORTTYPE_VOIP       0x0002 /* VoIP port */
#define CTC_VAR_INSTANT_PORTTYPE_ADSL2PLUS  0x0003 /* ADSL2+ port */
#define CTC_VAR_INSTANT_PORTTYPE_VDSL2      0x0004 /* VDSL2 port */
#define CTC_VAR_INSTANT_PORTTYPE_E1         0x0005 /* E1 port */
#define CTC_VAR_INSTANT_PORTTYPE_PON        0x0080 /* PON port - non CTC, only use to convert CTC 2.0- instant to CTC 2.0+ ones */
#define CTC_VAR_INSTANT_PORTTYPE_OTHERS     0x0081 /* other port type - non CTC, only use to convert CTC 2.0- instant to CTC 2.0+ ones */
/* Variable Instant other define for CTC 2.1+ */
#define CTC_VAR_INSTANT_PORTALL             0xFFFF /* all ports */
/* Variable flags */
#define CTC_VAR_OP_GET                      0x01 /* Support get operation */
#define CTC_VAR_OP_SET                      0x02 /* Support set operation */
#define CTC_VAR_CONTAINER_INDICATOR         0x8000 /* Width field is Indicator */
#define CTC_VAR_TARGET_ONU                  0x01 /* Operation target is ONU */
#define CTC_VAR_TARGET_UNIPORT              0x02 /* Operation target is UNI port */
#define CTC_VAR_TARGET_MULTICAST            0x04 /* Operation target is multicast */
#define CTC_VAR_TARGET_LLID                 0x08 /* Operation target is LLID */
#define CTC_VAR_TARGET_VOIP                 0x10 /* Operation target is VoIP */
#define CTC_VAR_TARGET_ALARM                0x20 /* Operation target is alarm */
#define CTC_VAR_TARGET_PONIF                0x40 /* Operation target is PON IF */
/* ONU Auth */
#define CTC_ORGSPEC_ONUAUTH_CODE_REQ        0x01 /* Auth request */
#define CTC_ORGSPEC_ONUAUTH_CODE_RESP       0x02 /* Auth response */
#define CTC_ORGSPEC_ONUAUTH_CODE_SUCC       0x03 /* Auth success */
#define CTC_ORGSPEC_ONUAUTH_CODE_FAIL       0x04 /* Auth fail */
#define CTC_ORGSPEC_ONUAUTH_LOID            0x01 /* Auth type LOID + password */
#define CTC_ORGSPEC_ONUAUTH_NAK             0x02 /* Auth type NAK */
/* Software download */
#ifdef YUEME_CUSTOMIZED_CHANGE
#define CTC_ORGSPEC_SWDL_STORAGE            "/var/tmp2/ctc_fwdl.tar"
#else
#define CTC_ORGSPEC_SWDL_STORAGE            "/tmp/ctc_fwdl.tar"
#endif
#define CTC_ORGSPEC_SWDL_TYPE_FILE          0x01 /* File operation */
#define CTC_ORGSPEC_SWDL_TYPE_END           0x02 /* End operation */
#define CTC_ORGSPEC_SWDL_TYPE_ACTIVATE      0x03 /* Activate operation */
#define CTC_ORGSPEC_SWDL_TYPE_COMMIT        0x04 /* Commit operation */
#define CTC_ORGSPEC_SWDL_OPCODE_FILEREQ     0x02 /* File write request */
#define CTC_ORGSPEC_SWDL_OPCODE_FILEDATA    0x03 /* File transfer data */
#define CTC_ORGSPEC_SWDL_OPCODE_FILEACK     0x04 /* File transfer ACK */
#define CTC_ORGSPEC_SWDL_OPCODE_ERROR       0x05 /* Error */
#define CTC_ORGSPEC_SWDL_OPCODE_ENDREQ      0x06 /* End download request */
#define CTC_ORGSPEC_SWDL_OPCODE_ENDRESP     0x07 /* End download response */
#define CTC_ORGSPEC_SWDL_OPCODE_ACTREQ      0x08 /* Activate image request */
#define CTC_ORGSPEC_SWDL_OPCODE_ACTRESP     0x09 /* Activate image response */
#define CTC_ORGSPEC_SWDL_OPCODE_COMMITREQ   0x0a /* Commit image request */
#define CTC_ORGSPEC_SWDL_OPCODE_COMMITRESP  0x0b /* Commit image response */
#define CTC_ORGSPEC_SWDL_ERROR_UNSPECIFY    0x00 /* Unspecify error, see errMsg for detail */
#define CTC_ORGSPEC_SWDL_ERROR_MEMORY       0x03 /* Insufficient memory space for storage */
#define CTC_ORGSPEC_SWDL_ERROR_ILLEGALOAM   0x04 /* Receiving illegal oam message */
#define CTC_ORGSPEC_SWDL_ERROR_EXIST        0x06 /* Already exist software image file */
#define CTC_ORGSPEC_SWDL_RPSCODE_COMPLETE   0x00 /* Check pass and program flash complete */
#define CTC_ORGSPEC_SWDL_RPSCODE_PROGRAMING 0x01 /* Check pass and programming flash */
#define CTC_ORGSPEC_SWDL_RPSCODE_CHECKFAIL  0x02 /* Check failed */
#define CTC_ORGSPEC_SWDL_RPSCODE_PARAMETER  0x03 /* OLT command parameter error */
#define CTC_ORGSPEC_SWDL_RPSCODE_NOTSUPPORT 0x04 /* ONU not support this command */
#define CTC_ORGSPEC_SWDL_ACTACK_SUCCESS     0x00 /* Activate success */
#define CTC_ORGSPEC_SWDL_ACTACK_PARAMETER   0x01 /* Activate parameter incorrect */
#define CTC_ORGSPEC_SWDL_ACTACK_NOTSUPPORT  0x02 /* Activate not support */
#define CTC_ORGSPEC_SWDL_ACTACK_FAIL        0x03 /* Activate fail */
#define CTC_ORGSPEC_SWDL_CMTACK_SUCCESS     0x00 /* Commit success */
#define CTC_ORGSPEC_SWDL_CMTACK_PARAMETER   0x01 /* Commit parameter incorrect */
#define CTC_ORGSPEC_SWDL_CMTACK_NOTSUPPORT  0x02 /* Commit not support */
#define CTC_ORGSPEC_SWDL_CMTACK_FAIL        0x03 /* Commit fail */
/* Churning */
#define CTC_ORGSPEC_CHURNING_CODE_MASK      0x03 /* bits [1:0] */
#define CTC_ORGSPEC_CHURNING_NEWKEYREQ      0x00 /* New key request */
#define CTC_ORGSPEC_CHURNING_NEWKEY         0x01 /* New churning key */
/* DBA */
#define CTC_ORGSPEC_DBA_CODE_MASK           0x03 /* bits [1:0] */
#define CTC_ORGSPEC_DBA_GET_REQ             0x00 /* get_DBA_request */
#define CTC_ORGSPEC_DBA_GET_RESP            0x01 /* get_DBA_response */
#define CTC_ORGSPEC_DBA_SET_REQ             0x02 /* set_DBA_request */
#define CTC_ORGSPEC_DBA_SET_RESP            0x03 /* set_DBA_response */
#define CTC_ORGSPEC_DBA_QUEUE_MAX           0x08 /* Maximum 8 queues in the report */
#define CTC_ORGSPEC_DBA_QUEUESET_MAX        0x03 /* Maximum 3 queue sets in the report */
#define CTC_ORGSPEC_DBA_QUEUESET_MIN        0x01 /* Minimum 1 queue sets in the report */
/* Event*/
#define CTC_ORGSPEC_EVENT_STATUS_REQ        0x01 /* EventStatus_Request */
#define CTC_ORGSPEC_EVENT_STATUS_SET        0x02 /* EventStatus_Set */
#define CTC_ORGSPEC_EVENT_STATUS_RESP       0x03 /* EventStatus_Response */
#define CTC_ORGSPEC_EVENT_THRESHOLD_REQ     0x04 /* EventThreshold_Request */
#define CTC_ORGSPEC_EVENT_THRESHOLD_SET     0x05 /* EventThreshold_Set */
#define CTC_ORGSPEC_EVENT_THRESHOLD_RESP    0x06 /* EventThreshold_Response */
#define CTC_ORGSPEC_EVENT_EVENTSTATUS_DISABLE 0x00000000UL
#define CTC_ORGSPEC_EVENT_EVENTSTATUS_ENABLE  0x00000001UL
/* Length definitions */
#define CTC_INFO_OAM_MIN                    7 /* Type(1) + Length(1) + OUI(3) + ExtSupport(1) + Version(1) */
#define CTC_INFO_OAM_VERITEM_LEN            4 /* OUI(3) + Version(1) */
#define CTC_EVENT_OAM_ALARMINFO_MAX         8 /* Defined in CTC standard */
#define CTC_ORGSPEC_HDR_LEN                 4 /* OUI(3) + Ext. Opcode(1) */
#define CTC_ORGSPEC_VARDESC_LEN             3 /* Branch(1) + Leaf(2) */
#define CTC_ORGSPEC_VARINSTANT_LEN20        2 /* Width(1) + Value(1) => CTC 2.0- */
#define CTC_ORGSPEC_VARINSTANT_LEN21        5 /* Width(1) + Value(4) => CTC 2.1+ */
#define CTC_ORGSPEC_VARINSTANT_WIDTH20      1 /* Size defined in CTC 2.0- standard */
#define CTC_ORGSPEC_VARINSTANT_WIDTH21      4 /* Size defined in CTC 2.1+ standard */
#define CTC_ORGSPEC_VARCONTAINER_MIN        4 /* Branch(1) + Leaf(2) + Width(1) */
#define CTC_ORGSPEC_ONUAUTH_HDR_LEN         3 /* Auth code(1) + Length of Auth Data(2) */
#define CTC_ORGSPEC_ONUAUTH_RESP_LOID_LEN   0x25 /* Auth type(1) + LOID(24) + Password(12) */
#define CTC_ORGSPEC_ONUAUTH_RESP_NAK_LEN    2 /* Auth type(1) + Desire Auth Type(1) */
#define CTC_ORGSPEC_ONUAUTH_LOID_LEN        24 /* LOID(24) */
#define CTC_ORGSPEC_ONUAUTH_PASSWORD_LEN    12 /* Password(12) */
#define CTC_ORGSPEC_SWDOWNLOAD_HDR_LEN      7 /* Data type(1) + length(2) + TID(2) + opCode(2) */
#define CTC_ORGSPEC_SWDOWNLOAD_FILEDATA_LEN 2 /* Block #(2) */
#define CTC_ORGSPEC_SWDOWNLOAD_FILEACK_LEN  2 /* Block #(2) */
#define CTC_ORGSPEC_SWDOWNLOAD_ERROR_LEN    3 /* ErrorCode(2) + ErrMsg(0) + ASCII NULL(1) */
#define CTC_ORGSPEC_SWDOWNLOAD_ENDREQ_LEN   4 /* File size(4) */
#define CTC_ORGSPEC_SWDOWNLOAD_ENDRESP_LEN  1 /* RPSCode(2) */
#define CTC_ORGSPEC_SWDOWNLOAD_ACTREQ_LEN   1 /* flag(1) */
#define CTC_ORGSPEC_SWDOWNLOAD_ACTRESP_LEN  1 /* ack(1) */
#define CTC_ORGSPEC_SWDOWNLOAD_CMTREQ_LEN   1 /* flag(1) */
#define CTC_ORGSPEC_SWDOWNLOAD_CMTRESP_LEN  1 /* ack(1) */
#define CTC_ORGSPEC_SWDOWNLOAD_BLOCK_LEN    (1400) /* each file block size */
#define CTC_ORGSPEC_SWDOWNLOAD_FILEBUF_LEN  (4 * 1024 * 1024) /* Pre-allocate file buffer 4MB */
#define CTC_ORGSPEC_CHURNING_HDR_LEN        2 /* Churning code(1) + key index(1) */
#define CTC_ORGSPEC_CHURNING_KEY_LEN        3 /* Churning key */
#define CTC_ORGSPEC_DBA_HDR_LEN             1 /* DBA code(1) */
#define CTC_ORGSPEC_DBA_REPORTHDR_LEN       1 /* Number of Queue Sets(1) */
#define CTC_ORGSPEC_DBA_SETACK_LEN          1 /* Set ACK(1) */
#define CTC_ORGSPEC_DBA_REPORTMAP_LEN       1 /* Report Bitmap(1) */
#define CTC_ORGSPEC_DBA_QUEUETHRESHOLD_LEN  2 /* Queue threshold(2) */
#define CTC_ORGSPEC_EVENT_HDR_LEN           3 /* Subtype(1) + Entry Count(2) */
#define CTC_EXECMD_UV_VAR_LEN               32 /* NV variable length */
#define CTC_EXECMD_UV_VALUE_LEN             32 /* NV variable value length */
#define CTC_EXECMD_UV_CMD_LEN               32 /* NV Command length */
#define CTC_EXECMD_STDOUT_LEN               (CTC_EXECMD_UV_VAR_LEN + CTC_EXECMD_UV_CMD_LEN + 32) /* STDOUT length for busybox shell */

/* CTC Extended Discovery FSM States */
#define CTC_OAM_FSM_STATE_WAIT_REMOTE       1
#define CTC_OAM_FSM_STATE_WAIT_REMOTE_OK    2
#define CTC_OAM_FSM_STATE_COMPLETE          3

/* Event alarm code */
/* ONU */
#define CTC_OAM_ALARMCODE_NONE                      0x0000
#define CTC_OAM_ALARMCODE_EQUIPMENT                 0x0001
#define CTC_OAM_ALARMCODE_POWER                     0x0002
#define CTC_OAM_ALARMCODE_BATTERYMISSING            0x0003
#define CTC_OAM_ALARMCODE_BATTERYFAILURE            0x0004
#define CTC_OAM_ALARMCODE_BATTERYVOLTLOW            0x0005
#define CTC_OAM_ALARMCODE_PHYSICALINTRUSION         0x0006
#define CTC_OAM_ALARMCODE_ONUSELFTESTFAILURE        0x0007
#define CTC_OAM_ALARMCODE_ONUTEMPHIGH               0x0009
#define CTC_OAM_ALARMCODE_ONUTEMPLOW                0x000A
#define CTC_OAM_ALARMCODE_IADCONNECTIONFAILURE      0x000B
#define CTC_OAM_ALARMCODE_PONIFSWITCH               0x000C
#define CTC_OAM_ALARMCODE_SLEEPSTATUSUPDATE         0x000D
/* PON IF */
#define CTC_OAM_ALARMCODE_RXPOWERHIGH               0x0101
#define CTC_OAM_ALARMCODE_RXPOWERLOW                0x0102
#define CTC_OAM_ALARMCODE_TXPOWERHIGH               0x0103
#define CTC_OAM_ALARMCODE_TXPOWERLOW                0x0104
#define CTC_OAM_ALARMCODE_TXBIASHIGH                0x0105
#define CTC_OAM_ALARMCODE_TXBIASLOW                 0x0106
#define CTC_OAM_ALARMCODE_VCCHIGH                   0x0107
#define CTC_OAM_ALARMCODE_VCCLOW                    0x0108
#define CTC_OAM_ALARMCODE_TEMPHIGH                  0x0109
#define CTC_OAM_ALARMCODE_TEMPLOW                   0x010A
#define CTC_OAM_ALARMCODE_RXPOWERHIGHWARNING        0x010B
#define CTC_OAM_ALARMCODE_RXPOWERLOWWARNING         0x010C
#define CTC_OAM_ALARMCODE_TXPOWERHIGHWARNING        0x010D
#define CTC_OAM_ALARMCODE_TXPOWERLOWWARNING         0x010E
#define CTC_OAM_ALARMCODE_TXBIASHIGHWARNING         0x010F
#define CTC_OAM_ALARMCODE_TXBIASLOWWARNING          0x0110
#define CTC_OAM_ALARMCODE_VCCHIGHWARNING            0x0111
#define CTC_OAM_ALARMCODE_VCCLOWWARNING             0x0112
#define CTC_OAM_ALARMCODE_TEMPHIGHWARNING           0x0113
#define CTC_OAM_ALARMCODE_TEMPLOWWARNING            0x0114
#define CTC_OAM_ALARMCODE_DSDROPEVENTS              0x0115
#define CTC_OAM_ALARMCODE_USDROPEVENTS              0x0116
#define CTC_OAM_ALARMCODE_DSCRCERRORFRAMES          0x0117
#define CTC_OAM_ALARMCODE_USCRCERRORFRAMES          0x0118
#define CTC_OAM_ALARMCODE_DSUNDERSIZEFRAMES         0x0119
#define CTC_OAM_ALARMCODE_USUNDERSIZEFRAMES         0x011A
#define CTC_OAM_ALARMCODE_DSOVERSIZEFRAMES          0x011B
#define CTC_OAM_ALARMCODE_USOVERSIZEFRAMES          0x011C
#define CTC_OAM_ALARMCODE_DSFRAGMENTS               0x011D
#define CTC_OAM_ALARMCODE_USFRAGMENTS               0x011E
#define CTC_OAM_ALARMCODE_DSJABBERS                 0x011F
#define CTC_OAM_ALARMCODE_USJABBERS                 0x0120
#define CTC_OAM_ALARMCODE_DSDISCARDFRAMES           0x0121
#define CTC_OAM_ALARMCODE_USDISCARDFRAMES           0x0122
#define CTC_OAM_ALARMCODE_DSERRORFRAMES             0x0123
#define CTC_OAM_ALARMCODE_USERRORFRAMES             0x0124
#define CTC_OAM_ALARMCODE_DSDROPEVENTSWARNING       0x0125
#define CTC_OAM_ALARMCODE_USDROPEVENTSWARNING       0x0126
#define CTC_OAM_ALARMCODE_DSCRCERRORFRAMESWARNING   0x0127
#define CTC_OAM_ALARMCODE_USCRCERRORFRAMESWARNING   0x0128
#define CTC_OAM_ALARMCODE_DSUNDERSIZEFRAMESWARNING  0x0129
#define CTC_OAM_ALARMCODE_USUNDERSIZEFRAMESWARNING  0x012A
#define CTC_OAM_ALARMCODE_DSOVERSIZEFRAMESWARNING   0x012B
#define CTC_OAM_ALARMCODE_USOVERSIZEFRAMESWARNING   0x012C
#define CTC_OAM_ALARMCODE_DSFRAGMENTSWARNING        0x012D
#define CTC_OAM_ALARMCODE_USFRAGMENTSWARNING        0x012E
#define CTC_OAM_ALARMCODE_DSJABBERSWARNING          0x012F
#define CTC_OAM_ALARMCODE_USJABBERSWARNING          0x0130
#define CTC_OAM_ALARMCODE_DSDISCARDFRAMESWARNING    0x0131
#define CTC_OAM_ALARMCODE_USDISCARDFRAMESWARNING    0x0132
#define CTC_OAM_ALARMCODE_DSERRORFRAMESWARNING      0x0133
#define CTC_OAM_ALARMCODE_USERRORFRAMESWARNING      0x0134
/* Port */
#define CTC_OAM_ALARMCODE_ETHPORTAUTONEGFAILURE		0x0301
#define CTC_OAM_ALARMCODE_ETHPORTLOS				0x0302
#define CTC_OAM_ALARMCODE_ETHPORTFAILURE			0x0303
#define CTC_OAM_ALARMCODE_ETHPORTLOOKBACK			0x0304
#define CTC_OAM_ALARMCODE_ETHPORTCONGESTION			0x0305
#define CTC_OAM_ALARMCODE_DOWNSTREAMDROPEVENTS		0x0306
#define CTC_OAM_ALARMCODE_UPSTREAMDROPEVENTS		0x0307
#define CTC_OAM_ALARMCODE_DOWNSTREAMCRCERRORFRAMES	0x0308
#define CTC_OAM_ALARMCODE_UPSTREAMCRCERRORFRAMES	0x0309
#define CTC_OAM_ALARMCODE_DOWNSTREAMUNDERSIZEFRAMES	0x030A
#define CTC_OAM_ALARMCODE_UPSTREAMUNDERSIZEFRAMES	0x030B
#define CTC_OAM_ALARMCODE_DOWNSTREAMOVERSIZEFRAMES	0x030C
#define CTC_OAM_ALARMCODE_UPSTREAMOVERSIZEFRAMES	0x030D
#define CTC_OAM_ALARMCODE_DOWNSTREAMFRAGMENTS		0x030E
#define CTC_OAM_ALARMCODE_UPSTREAMFRAGMENTS			0x030F
#define CTC_OAM_ALARMCODE_DOWNSTREAMJABBERS			0x0310
#define CTC_OAM_ALARMCODE_UPSTREAMJABBERS			0x0311
#define CTC_OAM_ALARMCODE_DOWNSTREAMDISCARDS		0x0312
#define CTC_OAM_ALARMCODE_UPSTREAMDISCARDS			0x0313
#define CTC_OAM_ALARMCODE_DOWNSTREAMERRORS			0x0314
#define CTC_OAM_ALARMCODE_UPSTREAMERRORS			0x0315
#define CTC_OAM_ALARMCODE_STATUSChANGETIMES			0x0316
#define CTC_OAM_ALARMCODE_DOWNSTREAMDROPEVENTSWARNING		0x0317
#define CTC_OAM_ALARMCODE_UPSTREAMDROPEVENTSWARNING			0x0318
#define CTC_OAM_ALARMCODE_DOWNSTREAMCRCERRORFRAMESWARNING	0x0319
#define CTC_OAM_ALARMCODE_UPSTREAMCRCERRORFRAMESWARNING		0x031A
#define CTC_OAM_ALARMCODE_DOWNSTREAMUNDERSIZEFRAMESWARNING	0x031B
#define CTC_OAM_ALARMCODE_UPSTREAMUNDERSIZEFRAMESWARNING	0x031C
#define CTC_OAM_ALARMCODE_DOWNSTREAMOVERSIZEFRAMESWARNING	0x031D
#define CTC_OAM_ALARMCODE_UPSTREAMOVERSIZEFRAMESWARNING		0x031E
#define CTC_OAM_ALARMCODE_DOWNSTREAMFRAGMENTSWARNING		0x031F
#define CTC_OAM_ALARMCODE_UPSTREAMFRAGMENTSWARNING			0x0320
#define CTC_OAM_ALARMCODE_DOWNSTREAMJABBERSWARNING			0x0321
#define CTC_OAM_ALARMCODE_UPSTREAMJABBERSWARNING			0x0322
#define CTC_OAM_ALARMCODE_DOWNSTREAMDISCARDSWARNING			0x0323
#define CTC_OAM_ALARMCODE_UPSTREAMDISCARDSWARNING			0x0324
#define CTC_OAM_ALARMCODE_DOWNSTREAMERRORSWARNING			0x0325
#define CTC_OAM_ALARMCODE_UPSTREAMERRORSWARNING				0x0326
#define CTC_OAM_ALARMCODE_STATUSChANGETIMESWARNING			0x0327
#ifdef CONFIG_USER_RTK_VOIP
#define CTC_OAM_ALARMCODE_POTSPORTFAILURE			0x0401
#endif
#define CTC_OAM_ALARMCODE_E1PORTFAILURE				0x0501
#define CTC_OAM_ALARMCODE_E1TIMINGUNLOCK			0x0502
#define CTC_OAM_ALARMCODE_E1LOS						0x0503

#define CTC_EVENT_COUNT_MAX 64

/* Performance monitor thread checking interval in seconds */
#define CTC_OAM_PM_INTERVAL                         (1)

/* In unit of 1 ms */
#define CTC_OAM_SILENT_MODE_TIME					60000

/*ONU SN info*/
#define CTC_OAM_VENDOR_ID    "PON_VENDOR_ID"
#ifdef YUEME_CUSTOMIZED_CHANGE
#define CTC_OAM_ONU_MODEL   "HW_CWMP_PRODUCTCLASS"
#else
#define CTC_OAM_ONU_MODEL    "EPON_ONU_MODEL"
#endif
#define CTC_OAM_HW_VERSION   "HW_HWVER"
//#define CTC_OAM_SW_VERSION   "EPON_SW_VERSION"    //sw_version should be gotten from uboot env.
#ifdef YUEME_CUSTOMIZED_CHANGE
#define CTC_OAM_EXTONU_MODEL "EPON_ONU_MODEL"
#else
#define CTC_OAM_EXTONU_MODEL "EPON_EXTONU_MODEL"
#endif
#define CTC_OAM_MIB_OUI          "OUI"
#define CTC_OAM_LOID         "LOID"
#define CTC_OAM_LOID_PASSWD  "LOID_PASSWD"
#define CTC_OAM_PORT_REMAPPING  "PORT_REMAPPING"
/*ONU silent mode*/
#define CTC_OAM_SILENT_MODE  "EPON_SILENT_MODE"
#define CTC_OAM_VENDOR_SPECIFIC_INFO	"HW_VENDOR_SPECINFO"

#ifdef CONFIG_RTL_MULTI_LAN_DEV
#ifdef CONFIG_RTL9601B_SERIES
#define ELANVIF_NUM 1//eth lan virtual interface number
#else
#define ELANVIF_NUM CONFIG_LAN_PORT_NUM//eth lan virtual interface number
#endif
#else
#define ELANVIF_NUM 1
#endif

enum {
    CTC_OAM_ONUAUTH_STATE_SUCC = 0,
    CTC_OAM_ONUAUTH_STATE_FAIL,
    CTC_OAM_ONUAUTH_STATE_NOTCOMPLETE,
    CTC_OAM_ONUAUTH_STATE_END
};

enum {
	CTC_OAM_ONUAUTH_AUTH_MAC = 0,
	CTC_OAM_ONUAUTH_AUTH_LOID,
	CTC_OAM_ONUAUTH_AUTH_IGNORE, /* auth is successfull, ignore it */
	CTC_OAM_ONUAUTH_AUTH_END
};

enum {
    CTC_OAM_SWDOWNLOAD_BUF_CLEAR = 0,       /* Buffer empty, no memory allocated */
    CTC_OAM_SWDOWNLOAD_BUF_FILEREQ = 0x01,  /* File request complete, all memory allocated */
    CTC_OAM_SWDOWNLOAD_BUF_FILEDATA = 0x02,
    CTC_OAM_SWDOWNLOAD_BUF_DATAEND = 0x04,
    CTC_OAM_SWDOWNLOAD_BUF_WRITEING = 0x08,
    CTC_OAM_SWDOWNLOAD_BUF_WRITEOK = 0x10,
    CTC_OAM_SWDOWNLOAD_BUF_WRITEFAIL = 0x20,
};

typedef struct ctc_infoOamVer_s {
    unsigned char oui[EPON_OAM_OUI_LENGTH];
    unsigned char version;
} ctc_infoOamVer_t;

typedef struct ctc_infoOamVerRec_s {
    unsigned char oui[EPON_OAM_OUI_LENGTH];
    unsigned char version;
    struct ctc_infoOamVerRec_s *next;
} ctc_infoOamVerRec_t;

typedef struct ctc_infoOam_s {
    unsigned char type;
    unsigned char length;
    unsigned char oui[EPON_OAM_OUI_LENGTH];
    unsigned char extSupport;
    unsigned char version;
    ctc_infoOamVerRec_t *vertionList;
} ctc_infoOam_t;

typedef struct ctc_eventOam_s {
    unsigned short objectType;
    unsigned int   objectInstance;
    unsigned short alarmId;
	unsigned int   eventStatus;
    unsigned char  alarmState;
    unsigned char  alarmInfo[CTC_EVENT_OAM_ALARMINFO_MAX];
    unsigned char  alarmInfoLen;
    struct ctc_eventOam_s *pNext;
} ctc_eventOam_t;

typedef struct ctc_thresholdOam_s {
    unsigned short objectType;
    unsigned int objectInstance;
    unsigned short alarmId;
    unsigned int setThreshold;
	unsigned int clearThreshold;
} ctc_thresholdOam_t;

typedef struct ctc_eventAlarmCode_s {
    unsigned short alarmTarget;
    unsigned short alarmId;
    unsigned char  alarmInfoLen;
	unsigned int   alarmEnable;
    unsigned int   alarmInstance;
    unsigned char  alarmState;
	unsigned int   threshold;      /* generate alarm if match this threshold condition */
    unsigned int   clearThreshold; /* clear alarm if match this threshold condition*/
    unsigned char  alarmInfo[CTC_EVENT_OAM_ALARMINFO_MAX];
} ctc_eventAlarmCode_t;

typedef struct ctc_varDescriptor_s {
    unsigned char varBranch;
    unsigned short varLeaf;
} ctc_varDescriptor_t;

typedef struct ctc_varContainer_s {
    ctc_varDescriptor_t varDesc;
    unsigned short varWidth;    /* Use extra 2 bytes to indicate length or indicator */
    unsigned char *pVarData;
} ctc_varContainer_t;

typedef struct ctc_varInstantUniPort_s {
    unsigned char portType; /* UNI Port type */
    unsigned char chassisNo; /* Chassis number */
    unsigned char slotNo; /* Slot number */
    unsigned short portNo; /* Port number, 1-index */
} ctc_varInstantUniPort_t;

/* Special variable container for object identifier number
 * The size of varData is CTC_ORGSPEC_VARINSTANT_WIDTH21 due to it can backward support
 * for CTC_ORGSPEC_VARINSTANT_WIDTH20
 * variable process callback should check this field to get the version of instant
 */
typedef struct ctc_varInstant_s {
    ctc_varDescriptor_t varDesc;
    unsigned char varWidth; /* Width is fixed at 1/4 for instant in CTC 2.0-/2.1+ */
    unsigned char varData[CTC_ORGSPEC_VARINSTANT_WIDTH21]; /* 4 octect data for object identifier number */
    union {
        ctc_varInstantUniPort_t uniPort;
        unsigned int llid;
        unsigned int ponIf;
    } parse;
} ctc_varInstant_t;

typedef struct ctc_varCb_s {
    oam_varDescriptor_t varDesc;
    unsigned char op;
    unsigned char target;
    /* Reserve the flexibility to use one set/get or two individual callback functions */
    int (*handler_get)(
        unsigned char llidIdx, /* LLID index of the incoming operation */
        unsigned char op, /* Operation to be taken */
        ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
        ctc_varDescriptor_t varDesc,
        ctc_varContainer_t *pVarContainer);  /* handler allocate data field in container, 
                                              * caller will release it
                                              */
    int (*handler_set)(
        unsigned char llidIdx, /* LLID index of the incoming operation */
        unsigned char op, /* Operation to be taken */
        ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
        ctc_varDescriptor_t varDesc,
        ctc_varContainer_t *pVarContainer);  /* caller prepare all the resource needed, 
                                              * handler won't need to allocate anything
                                              */
    /* var info */
    char varName[64];
    int (*var_get)(
        unsigned char llidIdx, /* LLID index of the incoming operation */
        ctc_varDescriptor_t varDesc);
} ctc_varCb_t;

typedef struct ctc_onuAuthLoid_s {
    unsigned char loid[30];
    unsigned char password[30];
} ctc_onuAuthLoid_t;

typedef struct ctc_swDlFileReq_s {
    char *fileName;
    char *mode;
} ctc_swDlFileReq_t;

typedef struct ctc_swDlFileData_s {
    unsigned short block;
    char *data;
} ctc_swDlFileData_t;

typedef struct ctc_swDlFileAck_s {
    unsigned short block;
} ctc_swDlFileAck_t;

typedef struct ctc_swDlError_s {
    unsigned short errCode;
    char *errMsg;
} ctc_swDlError_t;

typedef struct ctc_swDlEndReq_s {
    unsigned int fileSize;
} ctc_swDlEndReq_t;

typedef struct ctc_swDlEndResp_s {
    unsigned char respCode;
} ctc_swDlEndResp_t;

typedef struct ctc_swDlActivateReq_s {
    unsigned char flag;
} ctc_swDlActivateReq_t;

typedef struct ctc_swDlActivateResp_s {
    unsigned char ack;
} ctc_swDlActivateResp_t;

typedef struct ctc_swDlCommitReq_s {
    unsigned char flag;
} ctc_swDlCommitReq_t;

typedef struct ctc_swDlCommitResp_s {
    unsigned char ack;
} ctc_swDlCommitResp_t;

typedef struct ctc_swDownload_s {
    unsigned char dataType;
    unsigned short length;
    unsigned short tid;
    unsigned short opCode;
    union {
        ctc_swDlFileReq_t       fileReq;
        ctc_swDlFileData_t      fileData;
        ctc_swDlFileAck_t       fileAck;
        ctc_swDlError_t         error;
        ctc_swDlEndReq_t        endReq;
        ctc_swDlEndResp_t       endResp;
        ctc_swDlActivateReq_t   activateReq;
        ctc_swDlActivateResp_t  activateResp;
        ctc_swDlCommitReq_t     commitReq;
        ctc_swDlCommitResp_t    commitResp;
    } parse;
} ctc_swDownload_t;

typedef struct ctc_swDlFile_s {
    unsigned int flag;          /* Software download flag */
    unsigned int fileSize;      /* Current file size */
    unsigned int checkSize;     /* Actual file size from OLT */
    unsigned int block;         /* Current number of blocks */
    char *fileName;             /* Current file name */
    char *mode;                 /* Current file mode */
} ctc_swDlFile_t;

typedef struct ctc_swDlBootInfo_s {
    unsigned int active;        /* Active image number */
    unsigned int commit;        /* Commit image number */
    unsigned char version[CTC_EXECMD_UV_VALUE_LEN];     /* Current active image version */
} ctc_swDlBootInfo_t;

typedef struct ctc_churning_s {
    unsigned char churningCode;
    unsigned char keyIdx; /* For churningCode = 0x0, this is in-use key index
                           * For churningCode = 0x1, this is new key index
                           */
    unsigned char churningKey[CTC_ORGSPEC_CHURNING_KEY_LEN]; /* Only available when churning code = 0x1 */
} ctc_churning_t;

typedef struct ctc_dbaQSet_s {
    unsigned short queueThreshold[CTC_ORGSPEC_DBA_QUEUE_MAX];
} ctc_dbaQSet_t;

/* Note: below structure size is 1 + 3 + 3 * 16 = 52 bytes */
typedef struct ctc_dbaThreshold_s {
    unsigned char numQSet; /* Actual number of queue set */
    /* Indicate which queues are valid of specific queue set */
    unsigned char reportMap[CTC_ORGSPEC_DBA_QUEUESET_MAX];
    /* Indicate each queue's threshold of specific queue set
     * Only valid when reportMap's corresponding bit being set
     */
    ctc_dbaQSet_t queueSet[CTC_ORGSPEC_DBA_QUEUESET_MAX];
} ctc_dbaThreshold_t;

typedef struct ctc_onuSnInfo_s {
	unsigned char vendorID[4];
	unsigned char onuModel[4];
	unsigned char hwVersion[8];
	unsigned char swVersion[16];
	unsigned char extOnuModel[16];
}ctc_onuSnInfo_t;

/*
 * Macro Definition
 */
#define CTC_BUF_ADD(pBufPtr, bufLen, size)          {pBufPtr += size; bufLen -= size;}
#ifdef SUPPORT_OAM_BYTEORDER
#define HTONLL(x) ((1==htonl(1)) ? (x) : (((uint64)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32)((x) >> 32)))
#define NTOHLL(x) ((1==ntohl(1)) ? (x) : (((uint64)ntohl((x) & 0xFFFFFFFFUL)) << 32) | ntohl((uint32)((x) >> 32)))

#define CTC_BUF_PARSE16(pBufPtr, pVar)              \
do {                                                \
    (*(unsigned short *)(pVar))= ntohs(*(unsigned short *)(pBufPtr));     \
} while(0);
#define CTC_BUF_PARSE32(pBufPtr, pVar)              \
do {                                                \
    (*(unsigned int *)(pVar))= ntohl(*(unsigned int *)(pBufPtr));         \
} while(0);
#define CTC_BUF_PARSE48(pBufPtr, pVar)              \
do {                                                \
    ((unsigned char *)pVar)[0] = (pBufPtr)[0];      \
    ((unsigned char *)pVar)[1] = (pBufPtr)[1];      \
    ((unsigned char *)pVar)[2] = (pBufPtr)[2];      \
    ((unsigned char *)pVar)[3] = (pBufPtr)[3];      \
    ((unsigned char *)pVar)[4] = (pBufPtr)[4];      \
    ((unsigned char *)pVar)[5] = (pBufPtr)[5];      \
} while(0);
#define CTC_BUF_PARSE64(pBufPtr, pVar)              \
do {                                                \
    (*(uint64 *)(pVar)) = HTONLL(*(uint64 *)(pBufPtr));    \
} while(0);
#define CTC_BUF_ENCODE16(pBufPtr, pVar)             \
do {                                                \
    (*(unsigned short *)(pBufPtr)) = htons(*(unsigned short *)(pVar));    \
} while(0);
#define CTC_BUF_ENCODE32(pBufPtr, pVar)             \
do {                                                \
    (*(unsigned int *)(pBufPtr)) = htonl(*(unsigned int *)(pVar));        \
} while(0);
#define CTC_BUF_ENCODE48(pBufPtr, pVar)             \
do {                                                \
    (pBufPtr)[0] = ((unsigned char *)pVar)[0];      \
    (pBufPtr)[1] = ((unsigned char *)pVar)[1];      \
    (pBufPtr)[2] = ((unsigned char *)pVar)[2];      \
    (pBufPtr)[3] = ((unsigned char *)pVar)[3];      \
    (pBufPtr)[4] = ((unsigned char *)pVar)[4];      \
    (pBufPtr)[5] = ((unsigned char *)pVar)[5];      \
} while(0);
#define CTC_BUF_ENCODE64(pBufPtr, pVar)             \
do {                                                \
    (*(uint64 *)(pBufPtr)) = NTOHLL(*(uint64 *)(pVar));    \
} while(0);
#else
#define CTC_BUF_PARSE16(pBufPtr, pVar)              \
do {                                                \
    ((unsigned char *)pVar)[0] = (pBufPtr)[0];      \
    ((unsigned char *)pVar)[1] = (pBufPtr)[1];      \
} while(0);
#define CTC_BUF_PARSE32(pBufPtr, pVar)              \
do {                                                \
    ((unsigned char *)pVar)[0] = (pBufPtr)[0];      \
    ((unsigned char *)pVar)[1] = (pBufPtr)[1];      \
    ((unsigned char *)pVar)[2] = (pBufPtr)[2];      \
    ((unsigned char *)pVar)[3] = (pBufPtr)[3];      \
} while(0);
#define CTC_BUF_PARSE48(pBufPtr, pVar)              \
do {                                                \
    ((unsigned char *)pVar)[0] = (pBufPtr)[0];      \
    ((unsigned char *)pVar)[1] = (pBufPtr)[1];      \
    ((unsigned char *)pVar)[2] = (pBufPtr)[2];      \
    ((unsigned char *)pVar)[3] = (pBufPtr)[3];      \
    ((unsigned char *)pVar)[4] = (pBufPtr)[4];      \
    ((unsigned char *)pVar)[5] = (pBufPtr)[5];      \
} while(0);
#define CTC_BUF_PARSE64(pBufPtr, pVar)              \
do {                                                \
    ((unsigned char *)pVar)[0] = (pBufPtr)[0];      \
    ((unsigned char *)pVar)[1] = (pBufPtr)[1];      \
    ((unsigned char *)pVar)[2] = (pBufPtr)[2];      \
    ((unsigned char *)pVar)[3] = (pBufPtr)[3];      \
    ((unsigned char *)pVar)[4] = (pBufPtr)[4];      \
    ((unsigned char *)pVar)[5] = (pBufPtr)[5];      \
    ((unsigned char *)pVar)[6] = (pBufPtr)[6];      \
    ((unsigned char *)pVar)[7] = (pBufPtr)[7];      \
} while(0);
#define CTC_BUF_ENCODE16(pBufPtr, pVar)             \
do {                                                \
    (pBufPtr)[0] = ((unsigned char *)pVar)[0];      \
    (pBufPtr)[1] = ((unsigned char *)pVar)[1];      \
} while(0);
#define CTC_BUF_ENCODE32(pBufPtr, pVar)             \
do {                                                \
    (pBufPtr)[0] = ((unsigned char *)pVar)[0];      \
    (pBufPtr)[1] = ((unsigned char *)pVar)[1];      \
    (pBufPtr)[2] = ((unsigned char *)pVar)[2];      \
    (pBufPtr)[3] = ((unsigned char *)pVar)[3];      \
} while(0);
#define CTC_BUF_ENCODE48(pBufPtr, pVar)             \
do {                                                \
    (pBufPtr)[0] = ((unsigned char *)pVar)[0];      \
    (pBufPtr)[1] = ((unsigned char *)pVar)[1];      \
    (pBufPtr)[2] = ((unsigned char *)pVar)[2];      \
    (pBufPtr)[3] = ((unsigned char *)pVar)[3];      \
    (pBufPtr)[4] = ((unsigned char *)pVar)[4];      \
    (pBufPtr)[5] = ((unsigned char *)pVar)[5];      \
} while(0);
#define CTC_BUF_ENCODE64(pBufPtr, pVar)             \
do {                                                \
    (pBufPtr)[0] = ((unsigned char *)pVar)[0];      \
    (pBufPtr)[1] = ((unsigned char *)pVar)[1];      \
    (pBufPtr)[2] = ((unsigned char *)pVar)[2];      \
    (pBufPtr)[3] = ((unsigned char *)pVar)[3];      \
    (pBufPtr)[4] = ((unsigned char *)pVar)[4];      \
    (pBufPtr)[5] = ((unsigned char *)pVar)[5];      \
    (pBufPtr)[6] = ((unsigned char *)pVar)[6];      \
    (pBufPtr)[7] = ((unsigned char *)pVar)[7];      \
} while(0);
#endif
#define CTC_INSTANT_IS_ALLPORTS(pVarInstant)        (pVarInstant->parse.uniPort.portNo == CTC_VAR_INSTANT_PORTALL)
#define CTC_INSTANT_IS_ETHERNET(pVarInstant)        (pVarInstant->parse.uniPort.portType == CTC_VAR_INSTANT_PORTTYPE_ETHERNET)
#define CTC_INSTANT_IS_PORT(pVarInstant)            (pVarInstant->varDesc.varLeaf == CTC_VAR_REQBRANCH_INSTANT_PORT)
#define CTC_INSTANT_IS_LLID(pVarInstant)            (pVarInstant->varDesc.varLeaf == CTC_VAR_REQBRANCH_INSTANT_LLID)
#define CTC_INSTANT_IS_PONIF(pVarInstant)           (((pVarInstant->varWidth == CTC_ORGSPEC_VARINSTANT_WIDTH21) && (pVarInstant->varDesc.varLeaf == CTC_VAR_REQBRANCH_INSTANT_PONIF)) || \
                                                    ((pVarInstant->varWidth == CTC_ORGSPEC_VARINSTANT_WIDTH20) && (pVarInstant->parse.uniPort.portType == CTC_VAR_INSTANT_PORTTYPE_PON)))

/*  
 * Function Declaration  
 */
extern int ctc_oam_init(void);
extern int ctc_oam_db_init(unsigned char llidIdx);
extern void ctc_oam_auth_state_init(unsigned char llidIdx);

#endif /* __CTC_OAM_H__ */


