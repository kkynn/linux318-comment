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

#include <stdio.h>

#include "ctc_wrapper.h"

ctc_wrapper_func_t wrapper_func;

void ctc_wrapper_init(void)
{
    memset(&wrapper_func, 0, sizeof(ctc_wrapper_func_t));
#ifdef CONFIG_SFU_APP
    ctc_wrapper_sfu_init();
#else
    ctc_wrapper_hgu_init();
#endif
}

