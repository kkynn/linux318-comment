/*
 * Copyright (C) 2015 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: $
 * $Date: $
 *
 * Purpose : CTC proprietary behavior wrapper APIs
 *
 * Feature : Provide the wapper layer for CTC application
 *
 */
 
#ifndef __CTC_WRAPPER_H__
#define __CTC_WRAPPER_H__

/*
 * Include Files
 */
#include <common/rt_type.h>
#include <common/rt_error.h>

/*
 * Macro Definition
 */
typedef enum ctc_wrapper_if_e {
    CTC_WRAPPER_IF_PORT,
    CTC_WRAPPER_IF_LLID,
    CTC_WRAPPER_IF_PONIF,
    CTC_WRAPPER_IF_END
} ctc_wrapper_if_t;

typedef enum ctc_pm_statisticsType_e
{
    CTC_WRAPPER_PMTYPE_CURRENT = 0,
    CTC_WRAPPER_PMTYPE_HISTORY = 1,
    CTC_WRAPPER_PMTYPE_NUM = 2,
    CTC_WRAPPER_PMTYPE_END = CTC_WRAPPER_PMTYPE_NUM,
} ctc_pm_statisticsType_t;

enum {
    CTC_WRAPPER_PORTCAP_10M = 1,
    CTC_WRAPPER_PORTCAP_100M = 2,
    CTC_WRAPPER_PORTCAP_1000M = 4
};

enum {
	CTC_WRAPPER_PORTCAP_10BASE_T = (1<<5),
	CTC_WRAPPER_PORTCAP_10BASE_T_FD = (1<<6),
	CTC_WRAPPER_PORTCAP_100BASE_T = (1<<7),
	CTC_WRAPPER_PORTCAP_100BASE_T_FD = (1<<8),
	CTC_WRAPPER_PORTCAP_1000BASE_T_FD = (1<<9),
	CTC_WRAPPER_PORTCAP_PAUSE = (1<<10),
	CTC_WRAPPER_PORTCAP_APAUSE = (1<<11)
};

enum {
    CTC_WRAPPER_TRANSCIVERSTATUS_TEMPERATURE = 1,
    CTC_WRAPPER_TRANSCIVERSTATUS_SUPPLYVOLTAGE,
    CTC_WRAPPER_TRANSCIVERSTATUS_BIASCURRENT,
    CTC_WRAPPER_TRANSCIVERSTATUS_TXPOWER,
    CTC_WRAPPER_TRANSCIVERSTATUS_RXPOWER
};

#define QUEUE_NUM         8

#define QOS_COS           0
#define QOS_DSCP          1
#define QOS_PORT          2
#define QOS_SVLAN         3


#define BITMAP_REMOVE(dstArray, srcArray, length) \
do { \
    uint32  array_index;\
    for (array_index = 0; array_index < length; array_index++) \
    { \
        dstArray[array_index] &= ~srcArray[array_index]; \
    } \
} while(0)

#define RTK_PORTMASK_REMOVE(dstPortmask, srcPortmask) \
    BITMAP_REMOVE((dstPortmask).bits, (srcPortmask).bits, RTK_TOTAL_NUM_OF_WORD_FOR_1BIT_PORT_LIST)


#define CTC_WRAPPER_NUM_UNIPORT                 16
#define CTC_WRAPPER_NUM_PONIF                   1

#define CTC_WRAPPER(name, args...)              ((wrapper_func.name == NULL) ? EPON_OAM_ERR_NOT_FOUND : wrapper_func.name( args ))
#define CTC_WRAPPER_PARAM_CHECK(expr,errno)     \
{                                               \
    if(expr)                                    \
    {                                           \
        return errno;                           \
    }                                           \
}

/*
 * Symbol Definition
 */
typedef struct ctc_wrapper_chipInfo_s {
    unsigned int chipId;
    unsigned int rev;
    unsigned int subType;
    unsigned int uniPortCnt;
	unsigned int uniGePortCnt;
    unsigned int uniFePortCnt;
    unsigned int voipPortCnt;
	unsigned int usbPortCnt;
	unsigned int wlanPortCnt;
    unsigned int ponPortNo;
    unsigned int cfEntryCnt;
    unsigned int uniPortCap[CTC_WRAPPER_NUM_UNIPORT];
} ctc_wrapper_chipInfo_t;

typedef struct ctc_boardCap_s {
    unsigned int uniPortCnt;
    unsigned int uniGePortCnt;
    unsigned int uniFePortCnt;
    unsigned int potsPortCnt;
	unsigned int usbPortCnt;
	unsigned int wlanPortCnt;
    unsigned int uniPortCap[CTC_WRAPPER_NUM_UNIPORT];
} ctc_boardCap_t;

typedef struct ctc_pm_statistics_s
{
    unsigned long long dsDropEvents;
    unsigned long long usDropEvents;
    unsigned long long dsOctets;
    unsigned long long usOctets;
    unsigned long long dsFrames;
    unsigned long long usFrames;
    unsigned long long dsBroadcastFrames;
    unsigned long long usBroadcastFrames;
    unsigned long long dsMulticastFrames;
    unsigned long long usMulticastFrames;
    unsigned long long dsCrcErrorFrames;
	unsigned long long usCrcErrorFrames;
    unsigned long long dsUndersizeFrames;
    unsigned long long usUndersizeFrames;
    unsigned long long dsOversizeFrames;
    unsigned long long usOversizeFrames;
    unsigned long long dsFragmentFrames;
	unsigned long long usFragmentFrames;
    unsigned long long dsJabberFrames;
	unsigned long long usJabberFrames;
    unsigned long long dsFrames64Octets;
    unsigned long long dsFrames65to127Octets;
    unsigned long long dsFrames128to255Octets;
    unsigned long long dsFrames256to511Octets;
    unsigned long long dsFrames512to1023Octets;
    unsigned long long dsFrames1024to1518Octets;
    unsigned long long usFrames64Octets;
    unsigned long long usFrames65to127Octets;
    unsigned long long usFrames128to255Octets;
    unsigned long long usFrames256to511Octets;
    unsigned long long usFrames512to1023Octets;
    unsigned long long usFrames1024to1518Octets;
    unsigned long long dsDiscardFrames;
    unsigned long long usDiscardFrames;
    unsigned long long dsErrorFrames;
	unsigned long long usErrorFrames;
    unsigned long long statusChangeTimes;
} ctc_pm_statistics_t;

typedef struct ctc_wrapper_vlan_s {
    unsigned int tpid:16;
    unsigned int pri:3;
    unsigned int cfi:1;
    unsigned int vid:12;
} ctc_wrapper_vlan_t;

typedef struct ctc_wrapper_vlanTransPair_s {
    ctc_wrapper_vlan_t oriVlan;
    ctc_wrapper_vlan_t newVlan;
} ctc_wrapper_vlanTransPair_t;

typedef struct ctc_wrapper_vlanTransCfg_s {
    ctc_wrapper_vlan_t defVlan;
    unsigned short num;
    ctc_wrapper_vlanTransPair_t *transVlanPair;
} ctc_wrapper_vlanTransCfg_t;

typedef struct ctc_wrapper_vlanAggreTableCfg_s {
    unsigned short entryNum;
    ctc_wrapper_vlan_t aggreToVlan;
    ctc_wrapper_vlan_t *aggreFromVlan;
} ctc_wrapper_vlanAggreTableCfg_t;

typedef struct ctc_wrapper_vlanAggreCfg_s {
    ctc_wrapper_vlan_t defVlan;
    unsigned short tableNum;
    ctc_wrapper_vlanAggreTableCfg_t *aggrTbl;
} ctc_wrapper_vlanAggreCfg_t;

typedef struct ctc_wrapper_vlanTrunkCfg_s {
    ctc_wrapper_vlan_t defVlan;
    unsigned short num;
    ctc_wrapper_vlan_t *acceptVlan;
} ctc_wrapper_vlanTrunkCfg_t;

typedef struct ctc_wrapper_vlanCfg_s {
    unsigned char vlanMode;
    union {
        ctc_wrapper_vlan_t tagCfg;
        ctc_wrapper_vlanTransCfg_t transCfg;
        ctc_wrapper_vlanAggreCfg_t aggreCfg;
        ctc_wrapper_vlanTrunkCfg_t trunkCfg;
    } cfg;
} ctc_wrapper_vlanCfg_t;

typedef struct ctc_wrapper_mcastVlanCfg_s {
    unsigned char vlanMode;
} ctc_wrapper_mcastVlanCfg_t;

typedef struct ctc_wrapper_ingressBw_s {
    rtk_enable_t enable;
    unsigned int cir;
    unsigned int cbs;
    unsigned int ebs;
} ctc_wrapper_ingressBw_t;

typedef struct ctc_wrapper_egressBw_s {
    rtk_enable_t enable;
    unsigned int cir;
    unsigned int pir;
} ctc_wrapper_egressBw_t;

typedef struct oam_mcast_control_entry {
	unsigned short	port_id;
	unsigned short	vlan_id;
	unsigned char	gda[6];
}__attribute__((packed)) oam_mcast_control_entry_t;

typedef struct ctc_wrapper_mxuMngGlobal_s {
    unsigned int ip[4];
	unsigned int mask;
	unsigned int gateway[4];
	unsigned short cvlan;
	unsigned short svlan;
	unsigned char priority;
	unsigned char isIpv4;
} ctc_wrapper_mxuMngGlobal_t;

typedef struct ctc_wrapper_func_s
{
    int (*portStateSet)(unsigned int, rtk_enable_t);
    int (*portStateGet)(unsigned int, rtk_enable_t *);
    int (*portAutoNegoSet)(unsigned int, rtk_enable_t);
    int (*portAutoNegoGet)(unsigned int, rtk_enable_t *);
    int (*portCapGet)(unsigned int, unsigned int *);
    int (*portLinkStatusGet)(unsigned int, rtk_enable_t *);
    int (*portFlowControlSet)(unsigned int, rtk_enable_t);
    int (*portFlowControlGet)(unsigned int, rtk_enable_t *);
    int (*portIngressBwSet)(unsigned int, rtk_enable_t, unsigned int, unsigned int, unsigned int);
    int (*portIngressBwGet)(unsigned int, rtk_enable_t *, unsigned int *, unsigned int *, unsigned int *);
    int (*portEgressBwSet)(unsigned int, rtk_enable_t, unsigned int, unsigned int);
    int (*portEgressBwGet)(unsigned int, rtk_enable_t *, unsigned int *, unsigned int *);
    int (*portLoopDetectSet)(unsigned int, rtk_enable_t);
    int (*portLoopDetectGet)(unsigned int, rtk_enable_t *);
    int (*portDisableLoopedSet)(unsigned int, rtk_enable_t);
    int (*portDisableLoopedGet)(unsigned int, rtk_enable_t *);
	int (*portLoopParameterConfigSet)(unsigned int, unsigned short, unsigned short, short *, short *);
    int (*portMacAgingSet)(unsigned int, rtk_enable_t, unsigned int);
    int (*portMacAgingGet)(unsigned int, rtk_enable_t *, unsigned int *);
    int (*boardCapGet)(ctc_boardCap_t *);
    int (*transceiverStatusGet)(unsigned int, unsigned short *);
    int (*portAutoNegoAdvertiseGet)(unsigned int, unsigned int *);
    int (*fecStateSet)(rtk_enable_t);
    int (*fecStateGet)(rtk_enable_t *);
    int (*chipInfoGet)(ctc_wrapper_chipInfo_t *);
    int (*vlanSet)(unsigned int, ctc_wrapper_vlanCfg_t *);
    int (*vlanGet)(unsigned int, ctc_wrapper_vlanCfg_t *);
    int (*pmStatusSet)(unsigned int, rtk_enable_t, unsigned int);
    int (*pmStatusGet)(unsigned int, rtk_enable_t *, unsigned int *);
    void (*pmStatUpdate)(unsigned int);
    int (*pmStatSet)(unsigned int);
    int (*pmStatGet)(unsigned int, ctc_pm_statistics_t *, ctc_pm_statisticsType_t);
	int (*ponLoopbackSet)(rtk_enable_t);
	int (*mxuMngGlobalSet)(ctc_wrapper_mxuMngGlobal_t *);
	int (*mxuMngGlobalGet)(ctc_wrapper_mxuMngGlobal_t *);
	int (*portAutoNegoRestart)(unsigned int);
	int (*fastLeaveAbilityGet)(int *);
	int (*potsStatusGet)(unsigned int, unsigned int *, unsigned int *, unsigned int *);
} ctc_wrapper_func_t;

/*
 * Data Declaration
 */
extern ctc_wrapper_func_t wrapper_func;

/*
 * Function Declaration
 */
extern void ctc_wrapper_init(void);

#endif /* __CTC_WRAPPER_H__ */

