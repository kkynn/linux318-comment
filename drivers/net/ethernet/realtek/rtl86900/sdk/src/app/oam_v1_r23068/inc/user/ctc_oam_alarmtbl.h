#ifndef __CTC_OAM_ALARMTBL_H__
#define __CTC_OAM_ALARMTBL_H__

#include "ctc_oam.h"

#define CTC_OAM_ALARM_NORMAL 0
#define CTC_OAM_ALARM_REPORT 1

#define CTC_EVENT_NOTIFICATION_HDRLEN 5  /* 1 byte EventType, 1 byte EventLength, 3 bytes OUI*/
/*2 byte ObjectType, 4 bytes ObjectInstance, 2 bytes AlarmId, 2 bytes TimeStamp, 1 byte AlarmState*/
#define CTC_EVENT_NOTIFICATION_CONTENT_MIN_LEN 11
#define CTC_EVENT_NOTIFICATION_LEN (CTC_EVENT_NOTIFICATION_HDRLEN + CTC_EVENT_NOTIFICATION_CONTENT_MIN_LEN + CTC_EVENT_OAM_ALARMINFO_MAX)
#define CTC_EVENT_NOTIFICATION_ONU_OBJECT_INSTANCE 0xFFFFFFFF

#define EVENT_NOTIFICATION_SEQUENCE_START_NUM 0x0123  /*magic num*/
#define EVENT_NOTIFICATION_ALARM_STATE_REPORT 0
#define EVENT_NOTIFICATION_ALARM_STATE_CLEAR  1

typedef struct ctc_oam_alarm_s{
    unsigned short alarmTarget;
    unsigned short alarmId;
    unsigned char  alarmInfoLen;
	unsigned int   threshold;
    unsigned int   clearThreshold;
    int (*handler)(
        ctc_eventAlarmCode_t  * alarm,
        int * value);
} ctc_oam_alarm_t; 

int ctc_oam_alarm_state_set(unsigned short alarmId, unsigned int alarmInstance, int enable);
int ctc_oam_alarm_threshold_set(unsigned short alarmId, unsigned int alarmInstance, 
                                unsigned int threshold,unsigned int clearThreshold);
int ctc_oam_alarm_threshold_get(unsigned short alarmId, unsigned int alarmInstance, 
                                unsigned int * threshold,unsigned int * clearThreshold);

ctc_eventAlarmCode_t * ctc_oam_alarm_get_entry(unsigned short alarmId, unsigned int alarmInstance);

void ctc_oam_alarm_init();
void ctc_oam_alarm_show(int showDisable);

int alarm_get_pon_transceiver(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_pon_port_rx_stats(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_pon_port_tx_stats(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_pon_port_error_stats(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_port_status(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_port_rx_stats(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_port_tx_stats(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_port_error_stats(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_port_status_change_times(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_port_autoneg_status(ctc_eventAlarmCode_t  * alarm, int * state);
int alarm_get_port_loop_status(ctc_eventAlarmCode_t  * alarm, int * value);
int alarm_dummy_handler(ctc_eventAlarmCode_t  * alarm, int * value);

#endif
