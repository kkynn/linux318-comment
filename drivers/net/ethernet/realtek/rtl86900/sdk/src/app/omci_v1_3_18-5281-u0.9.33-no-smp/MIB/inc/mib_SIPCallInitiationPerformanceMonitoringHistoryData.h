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
 */


#ifndef __MIB_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TABLE_H__
#define __MIB_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TABLE_H__

/* Table SIPCallInitiationPerformanceMonitoringHistoryData attribute for STRING type define each entry length */

/* Table SIPCallInitiationPerformanceMonitoringHistoryData attribute index */
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM (8)
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX ((MIB_ATTR_INDEX)1)
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX ((MIB_ATTR_INDEX)2)
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX ((MIB_ATTR_INDEX)3)
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX ((MIB_ATTR_INDEX)4)
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX ((MIB_ATTR_INDEX)5)
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX ((MIB_ATTR_INDEX)6)
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX ((MIB_ATTR_INDEX)7)
#define MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX ((MIB_ATTR_INDEX)8)

/* Table SIPCallInitiationPerformanceMonitoringHistoryData attribute len, only string attrubutes have length definition */
typedef struct {
	UINT16   EntityId;
	UINT8    IntervalEndTime;
	UINT16   ThresholdData12Id;
	UINT32   FailedToConnectCounter;
	UINT32   FailedToValidateCounter;
	UINT32   TimeoutCounter;
	UINT32   FailureReceivedCounter;
	UINT32   FailedToAuthenticateCounter;
} __attribute__((aligned)) MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_T;

#endif /* __MIB_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TABLE_H__ */
