/*
 * Copyright (C) 2010 Realtek Semiconductor Corp. 
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated, 
 * modified or distributed under the authorized license from Realtek. 
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER 
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED. 
 *
 * $Revision: 10214 $
 * $Date: 2010-06-12 15:30:09 +0800 (Sat, 12 Jun 2010) $
 *
 * Purpose : System Notifier Function
 *
 * Feature : System Notifier Function
 *
 */

#ifndef __COMMON_SYS_NOTIFIER_H__
#define __COMMON_SYS_NOTIFIER_H__

/*
 * Include Files
 */

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */ 
typedef int32 (*sys_notifier_event_cb_t)(char *pData);

/* This is called when observer register. The purpose is to sync current status to observer.*/
typedef int32 (*sys_notifier_event_sync_t)(sys_notifier_event_cb_t eventCallback); 

typedef enum sys_notifier_subject_type_e
{
    SYS_NOTIFIER_SUBJECT_PORT,
    SYS_NOTIFIER_SUBJECT_TRUNK,
    SYS_NOTIFIER_SUBJECT_TRUNK_ACTIVE_MEMBER,
    SYS_NOTIFIER_SUBJECT_VLAN,    
    SYS_NOTIFIER_SUBJECT_END,
}sys_notifier_subject_type_t;

typedef enum sys_notifier_priority_e
{
    SYS_NOTIFIER_PRI_LOW,
    SYS_NOTIFIER_PRI_MEDIUM,
    SYS_NOTIFIER_PRI_HIGH,
    SYS_NOTIFIER_PRI_END,
}sys_notifier_priority_t;

typedef struct sys_notifier_event_handler_list_s
{
    sys_notifier_priority_t priority;
    sys_notifier_event_cb_t eventCallback;
    struct sys_notifier_event_handler_list_s *next;
} sys_notifier_event_handler_list_t;

/*
 * Macro Definition
 */

/*
 * Function Declaration
 */
/* Function Name:
 *      sys_notifier_subject_register
 * Description:
 *      Register Sync event for subject. Subject doesn't need call this API 
 *       if subject doesn't need sync event when observer register.
 * Input:
 *      type          - subject type of event createtor
 *      eventCallback - event callback function when event created. 
 * Output:
 *      None
 * Return:
 *      SYS_ERR_OK
 *      SYS_ERR_FAILED
 *      SYS_ERR_NULL_POINTER
 * Note:
 *      None
 */
extern int32 sys_notifier_subject_register(sys_notifier_subject_type_t type, sys_notifier_event_sync_t syncEvent);

/* Function Name:
 *      _sys_notifier_event_dispatcher
 * Description:
 *      Dispatch notify event to all registered module
 * Input:
 *      type  - subject type of event createtor
 *      pData - pointer of data struct
 * Output:
 *      None
 * Return:
 *      SYS_ERR_OK
 *      SYS_ERR_NULL_POINTER
 * Note:
 *      None
 */
extern int32 sys_notifier_event_dispatcher(sys_notifier_subject_type_t type, char *pData);

/* Function Name:
 *      sys_notifier_observer_register
 * Description:
 *      Register callback function for event notification 
 *      Entry Insert Policy: 
 *                           Base on priority (High > Medium > Low) fisrt, then last in last out 
 *                           at the same priority.
 * Input:
 *      type          - subject type of event createtor
 *      eventCallback - event callback function when event created. 
 *      priority      - priority of observer
 * Output:
 *      None
 * Return:
 *      SYS_ERR_OK
 *      SYS_ERR_FAILED
 *      SYS_ERR_NULL_POINTER
 * Note:
 *      None
 */
extern int32 sys_notifier_observer_register(sys_notifier_subject_type_t type, sys_notifier_event_cb_t eventCallback, sys_notifier_priority_t priority);

/* Function Name:
 *      sys_notifier_observer_unregister
 * Description:
 *      Unregister callback function for event notification 
 * Input:
 *      type          - subject type of event createtor
 *      eventCallback - event callback function when event created. 
 * Output:
 *      None
 * Return:
 *      SYS_ERR_OK
 *      SYS_ERR_FAILED
 *      SYS_ERR_NULL_POINTER
 * Note:
 *      None
 */
extern int32 sys_notifier_observer_unregister(sys_notifier_subject_type_t type, sys_notifier_event_cb_t eventCallback);

#endif /* __COMMON_SYS_NOTIFIER_H__ */

