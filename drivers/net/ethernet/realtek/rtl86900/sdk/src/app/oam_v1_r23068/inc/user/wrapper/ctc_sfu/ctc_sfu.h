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
 * Purpose : CTC proprietary behavior SFU wrapper APIs header file
 *
 * Feature : Provide the wapper layer header for CTC SFU application
 *
 */
 
#ifndef __CTC_SFU_H__
#define __CTC_SFU_H__

#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_RTL9602C_SERIES)
/* 9602c and 9607c not support keeptag and c2s function, so must use classification rule and 4k svlan table to implement vlan attribute */
#define CONFIG_VLAN_USE_CLASSF
#endif

enum {
    CTC_SFU_CFTYPE_ACLHIT = 0,
    CTC_SFU_CFTYPE_ETHERTYPE,
    CTC_SFU_CFTYPE_MULTICAST, /* used for multicast vlan mode (tag, untag, translation) */
    CTC_SFU_CFTYPE_GENERIC,
    CTC_SFU_CFTYPE_VLAN_PERMIT,
    CTC_SFU_CFTYPE_RESERVED = 0xff /* This should be the last one */
};

enum {
    CTC_SFU_CFUSAGE_VLAN = 0,
	CTC_SFU_CFUSAGE_CLASSIFICATION = 1,
	CTC_SFU_CFUSAGE_MC=2,
	CTC_SFU_CFUSAGE_VLAN_PERMIT=3,
    CTC_SFU_CFUSAGE_RESERVED = 0xff,
};

typedef struct ctc_sfu_pm_s {
    rtk_enable_t pmState;
    unsigned int pmPeriod;
    unsigned int pmPeriodCur;
    ctc_pm_statistics_t pmData[CTC_WRAPPER_PMTYPE_NUM];  /* Ping-pong buffer */
    ctc_pm_statistics_t *pmDataCur, *pmDataHis;
} ctc_sfu_pm_t;

#define CTC_SFU_CF_ALLPORTS     0x7f

extern void ctc_wrapper_sfu_init(void);
extern int32 is9601B(void);
extern int32 ctc_is_uniport(uint32 lport);
extern int is_vid_used_by_mxuMng(uint32 uiVid);

#endif /* __CTC_SFU_H__ */

