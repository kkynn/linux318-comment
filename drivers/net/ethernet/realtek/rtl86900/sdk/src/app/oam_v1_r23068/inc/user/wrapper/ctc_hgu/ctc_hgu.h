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
 * Purpose : CTC proprietary behavior HGU wrapper APIs header file
 *
 * Feature : Provide the wapper layer header for CTC HGU application
 *
 */
 
#ifndef __CTC_HGU_H__
#define __CTC_HGU_H__

#ifndef ENABLE
#define ENABLE 1
#endif

#ifndef DISABLE
#define DISABLE 0
#endif

typedef struct ctc_hgu_pm_s {
    rtk_enable_t pmState;
    unsigned int pmPeriod;
    unsigned int pmPeriodCur;
    ctc_pm_statistics_t pmData[CTC_WRAPPER_PMTYPE_NUM];  /* Ping-pong buffer */
    ctc_pm_statistics_t *pmDataCur, *pmDataHis;
} ctc_hgu_pm_t;


enum {
    CTC_HGU_CFUSAGE_VLAN = 0,
	CTC_HGU_CFUSAGE_CLASSIFICATION = 1,
	CTC_HGU_CFUSAGE_MC=2,
    CTC_HGU_CFUSAGE_RESERVED = 0xff,
};


extern void ctc_wrapper_hgu_init(void);

#endif /* __CTC_HGU_H__ */

