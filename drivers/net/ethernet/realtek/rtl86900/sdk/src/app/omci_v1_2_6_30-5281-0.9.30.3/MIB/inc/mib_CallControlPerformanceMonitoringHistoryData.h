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
 * Purpose : Definition of ME attribute: Call control PMHD (140)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME attribute: Call control PMHD (140)
 */


#ifndef __MIB_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_TABLE_H__
#define __MIB_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Table CallControlPerformanceMonitoringHistoryData attribute for STRING type define each entry length */

/* Table CallControlPerformanceMonitoringHistoryData attribute index */
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM (8)
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX ((MIB_ATTR_INDEX)1)
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX ((MIB_ATTR_INDEX)2)
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX ((MIB_ATTR_INDEX)3)
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX ((MIB_ATTR_INDEX)4)
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX ((MIB_ATTR_INDEX)5)
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX ((MIB_ATTR_INDEX)6)
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX ((MIB_ATTR_INDEX)7)
#define MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX ((MIB_ATTR_INDEX)8)

/* Table CallControlPerformanceMonitoringHistoryData attribute len, only string attrubutes have length definition */
typedef struct {
	UINT16   EntityId;
	UINT8    IntervalEndTime;
	UINT16   ThresholdData12Id;
	UINT32   CallSetupFailures;
	UINT32   CallSetupTimer;
	UINT32   CallTerminateFailures;
	UINT32   AnalogPortReleases;
	UINT32   AnalogPortOffhookTimer;
} __attribute__((aligned)) MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_T;

#ifdef __cplusplus
}
#endif

#endif /* __MIB_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_TABLE_H__ */
