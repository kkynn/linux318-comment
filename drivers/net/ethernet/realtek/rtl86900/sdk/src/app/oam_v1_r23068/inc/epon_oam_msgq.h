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
 * $Revision: 40647 $
 * $Date: 2013-07-01 15:36:16 +0800 (週一, 01 七月 2013) $
 *
 * Purpose : Define the EPON OAM protcol stack's message queue structures
 *
 * Feature : 
 *
 */

#ifndef __EPON_OAM_MSGQ_H__
#define __EPON_OAM_MSGQ_H__

/* 
 * Symbol Definition 
 */
/* Maximum size of the event data length */
//cxy 2015-1-20: loopdetect cli set exceed 128
#define EPON_OAM_EVENTDATA_MAX              256
/* EPON OAM events */
#define EPON_OAM_EVENT_DISCOVERY_COMPLETE   0x0001
#define EPON_OAM_EVENT_KEEPALIVE_TIMEOUT    0x0002
#define EPON_OAM_EVENT_KEEPALIVE_RESET      0x0003
#define EPON_OAM_EVENT_LOS                  0x0004
#define EPON_OAM_EVENT_CLI                  0x0005
/* below two macro defined for holdover */
#define EPON_OAM_EVENT_HOLDOVER_TIMEOUT		0x0006
#define EPON_OAM_EVENT_LOS_RECOVER			0x0007
#define EPON_OAM_EVENT_DOWNLOAD_ACK_RESEND_TIMER  0x0008
#define EPON_OAM_EVENT_DOWNLOAD_ACK_RESEND_CLEAR  0x0009
#define EPON_OAM_EVENT_TX_POWER_ENABLE_TIMER  	  0x000a
#define EPON_OAM_EVENT_TX_POWER_ENABLE_CLEAR  	  0x000b
#define EPON_OAM_EVENT_LOOP_DETECT_ENABLE	0x000c

#define IGMP_CONTROL_TYPE 1
#define IGMP_MCVLAN_ADD_TYPE 2
#define IGMP_MCVLAN_DEL_TYPE 3
#define IGMP_DEBUG_TYPE 4
#define IGMP_MCTAGOPER_TYPE 5
#define IGMP_TRANSLATION_ADD_TYPE 6

/* CLI types */
typedef enum oam_cliType_e {
    EPON_OAM_CLI_DBG_SET = 1,
    EPON_OAM_CLI_DBG_GET,
    EPON_OAM_CLI_CFGOAM_SET,
    EPON_OAM_CLI_CFGMAC_SET,
    EPON_OAM_CLI_CFGAUTOREG_SET,
    EPON_OAM_CLI_CFGHOLDOVER_SET,
    EPON_OAM_CLI_CFGEVENT_SET,
    EPON_OAM_CLI_CFGKEEPALIVE_SET,
    EPON_OAM_CLI_CFG_GET,
    EPON_OAM_CLI_CALLBACK_SET,
    EPON_OAM_CLI_CALLBACK_GET,
    EPON_OAM_CLI_COUNTER_GET,
    EPON_OAM_CLI_COUNTER_CLEAR,
    EPON_OAM_CLI_DBGEXT_SET,
    EPON_OAM_CLI_DBGEXT_GET,
    EPON_OAM_CLI_FAILOVER_SET,
    EPON_OAM_CLI_FAILOVER_GET,
    EPON_OAM_CLI_OAMINFOOUI_SET,
    EPON_OAM_CLI_OAMINFO_GET,
    EPON_OAM_CLI_OAMSTATE_GET,
    EPON_OAM_CLI_REG_TRIGGER,
    CTC_OAM_CLI_LOID_SET,
    CTC_OAM_CLI_LOID_GET,
    CTC_OAM_CLI_VAR_GET,
    CTC_OAM_CLI_VAR_SET,
    CTC_OAM_CLI_LOOPDETECT_SET,
    CTC_OAM_CLI_LOOPDETECT_GET,
    CTC_OAM_CLI_LANPORTCHANGE_GET,
    CTC_OAM_CLI_ALARM_GET,
    CTC_OAM_CLI_SILENT_SET,
    CTC_OAM_CLI_SILENT_GET,
    CTC_OAM_CLI_AUTHSUCCTIME_GET,
    CTC_OAM_CLI_AUTH_GET,
    CTC_OAM_CLI_REGISTERNUM_GET,
    CTC_OAM_CLI_REGISTERNUM_RESET,
    CTC_OAM_CLI_SILENTPONLEDMODE_SET,
    CTC_OAM_CLI_VLANSTATUS_SET,
    CTC_OAM_CLI_VLAN_GET,
    CTC_OAM_CLI_VLAN_SET,
    EPON_OAM_IGMP_SET,
    EPON_OAM_IGMP_GET,
    EPON_OAM_CLI_OAMINFO_VENDORID_SET,
    EPON_OAM_PORT_MAP_SET,
    EPON_OAM_PORT_MAP_GET,
    EPON_OAM_CLI_UPTIME_GET,
    EPON_OAM_CLI_END,
} oam_cliType_t;

typedef struct msgbuf{
	long mtype;
	unsigned char buf[1024];
}msgbuf_t;

typedef struct oam_msgqData_s {
	long reqid;			// Request id
    unsigned char llidIdx;
    unsigned short dataSize;
    unsigned char data[EPON_OAM_EVENTDATA_MAX];
} oam_msgqData_t;

typedef struct oam_msgqEventData_s {
    long mtype;
    oam_msgqData_t msgqData; 
} oam_msgqEventData_t;

/* Structure for CLI usage */
typedef struct oam_cliDbg_s {
    unsigned int flag;
} oam_cliDbg_t;

typedef struct oam_cliCallback_s {
    unsigned char oui[3];
    unsigned char state;
} oam_cliCallback_t;

typedef struct oam_cliLlidIdx_s {
    unsigned char llidIdx;
} oam_cliLlidIdx_t;

typedef struct oam_cliEnable_s {
    unsigned char llidIdx;
    unsigned char enable;
} oam_cliEnable_t;

typedef struct oam_cliMac_s {
    unsigned char llidIdx;
    unsigned char mac[6];
} oam_cliMac_t;

typedef struct oam_cliFailover_s {
    unsigned short granularity;
    unsigned short backoff;
} oam_cliFailover_t;

typedef struct oam_cliAutoReg_s {
    unsigned char llidIdx;
    unsigned char autoRegEnable;
    unsigned short autoRegTime;
} oam_cliAutoReg_t;

typedef struct oam_cliHoldover_s {
    unsigned char llidIdx;
    unsigned char holdoverEnable;
    unsigned short holdoverTime;
} oam_cliHoldover_t;

typedef struct oam_cliEvent_s {
    unsigned char llidIdx;
    unsigned short eventRepCnt;
    unsigned short eventRepIntvl;
} oam_cliEvent_t;

typedef struct oam_cliCtcLoid_s {
    unsigned char llidIdx;
    unsigned char loid[24/* CTC_ORGSPEC_ONUAUTH_LOID_LEN */];
    unsigned char password[12/* CTC_ORGSPEC_ONUAUTH_PASSWORD_LEN */];
} oam_cliCtcLoid_t;

typedef struct oam_cliCtcLoopdetect_s {
	unsigned char loopdetect_enable;
    unsigned short ether_type;
	unsigned int loopdetect_time;
	unsigned int loop_recover_time;
	short vid[100];
} oam_cliCtcLoopdetect_t;

typedef struct oam_cliCtcVar_s {
    unsigned char llidIdx;
    unsigned char varBranch;
    unsigned short varLeaf;
} oam_cliCtcVar_t;

typedef struct oam_cliIgmp_s {
    unsigned char controlType;
	unsigned short mcVlan;
	unsigned char type;
	unsigned int val;
	unsigned int port;
} oam_cliIgmp_t;

typedef struct oam_cliPortMap_s {
	unsigned int portNum;
	unsigned int portMap[4];
} oam_cliPortMap_t;

typedef struct oam_cli_vlan_s {
    unsigned int pri;
    unsigned int vid;
} oam_cli_vlan_t;

typedef struct oam_cli_vlanTransCfg_s {
    oam_cli_vlan_t cvlan;
    oam_cli_vlan_t svlan;
} oam_cli_vlanTransCfg_t;

typedef struct oam_cliVlanCfg_s {
	unsigned int port;
    unsigned char vlanMode;
    union {
        oam_cli_vlan_t tagCfg;
        oam_cli_vlanTransCfg_t transCfg;
    } cfg;
} oam_cliVlanCfg_t;

typedef struct oam_cliRegisterNum_s {
	unsigned char llidIdx;
	unsigned int registerNumToFile;
}oam_cliRegisterNum_t;

typedef struct oam_cliUptime_s {
	unsigned char llidIdx;
	unsigned int uptimeToFile;
}oam_cliUptime_t;

typedef struct oam_cliKeepalive_s {
    unsigned char llidIdx;
    unsigned short keepaliveTime;
} oam_cliKeepalive_t;

typedef struct oam_cli_s {
    oam_cliType_t cliType;
    union {
        oam_cliLlidIdx_t cliLlidIdx;
        oam_cliEnable_t cliEnable;
        oam_cliCallback_t cliCallback;
        oam_cliDbg_t cliDbg;
        oam_cliMac_t cliMac;
        oam_cliFailover_t cliFailover;
        oam_cliAutoReg_t cliAutoReg;
        oam_cliHoldover_t cliHoldover;
        oam_cliEvent_t cliEvent;
        oam_cliCtcLoid_t cliCtcLoid;
        oam_cliCtcVar_t cliCtcVar;
		oam_cliIgmp_t cliIgmp;
		oam_cliCtcLoopdetect_t cliCtcLoopdetect;
	oam_cliPortMap_t cliPortMap;
		unsigned int cliAlarmShowDisable;
		oam_cliRegisterNum_t cliRegisterNumToFile;
		oam_cliVlanCfg_t cliVlanCfg;
		oam_cliUptime_t cliUptimeToFile;
		oam_cliKeepalive_t cliKeepalive;
    } u;
} oam_cli_t;


/*
 * Function Declaration
 */
extern int epon_oam_event_send(
    unsigned char llidIdx,
    unsigned int eventId);

#endif /* __EPON_OAM_MSGQ_H__ */

