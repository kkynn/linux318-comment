/*
 * Copyright (C) 2011 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * 
 * 
 * $Revision: 39101 $
 * $Date: 2013-05-03 17:35:27 +0800 (Fri, 03 May 2013) $
 *
 * Purpose : Classification Driver API test
 *
 * Feature : Provide the APIs to access classification fratures
 *
 */

#ifndef __DAL_CF_TEST_CASE_H__
#define __DAL_CF_TEST_CASE_H__

extern int32 dal_raw_cf_ruleAll_test(uint32 caseNo);
extern int32 dal_raw_cf_downStreamAll_test(uint32 caseNo);
extern int32 dal_raw_cf_upStreamAll_test(uint32 caseNo);
extern int32 dal_raw_cf_rule_test(uint32 caseNo);
extern int32 dal_raw_cf_cfg_test(uint32 caseNo);

#endif /* __DAL_CF_TEST_CASE_H__ */
