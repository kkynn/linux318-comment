/*
 * Include Files
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "epon_oam_config.h"
#include "epon_oam_dbg.h"
#include "epon_oam_err.h"

#include "ctc_oam_alarmtbl.h"
#include "ctc_oam.h"
#include "drv_convert.h"

#include <rtk/port.h>
#include <rtk/stat.h>
#include <rtk/ponmac.h>
#include <common/util/rt_util.h>
#include <common/rt_error.h>
#include <hal/common/halctrl.h>

/* FIXME: for counter (e.g. DownstreamDropEventsAlarm), Alarm Info size is 8 bytes according to ctc 3.0 spec */
#define CTC_3_ALARM_COUNTER_SIZE
#ifdef CTC_3_ALARM_COUNTER_SIZE
#define COUNTER_SIZE 8
#else
#define COUNTER_SIZE 4
#endif

/*default value when threshold not set */
#define POWER_HIGH_DEFAULT_VALUE 0xFFFFFFFF 
#define BIAS_HIGH_DEFAULT_VALUE 0xFFFFFFFF
#define VOLTAGE_HIGH_DEFAULT_VALUE 0xFFFFFFFF
#define TEMP_HIGH_DEFAULT_VALUE 0x7FFFFFFF
#define COUNTER_DEFAULT_VALUE 0xFFFFFFFF 

int alarmNumber = 0;
ctc_eventAlarmCode_t  * alarmDb = NULL;
extern int LAN_PORT_NUM;

/* increased by one after sending a event notification packet */
static unsigned short sequenceNum = EVENT_NOTIFICATION_SEQUENCE_START_NUM;

static unsigned ctc_oui[3] = {0x11,0x11,0x11};

static ctc_oam_alarm_t ctc_alarmTable[] = {
    /* { Alarm Target, Alarm ID, Alarm Info Length, Threshold, ClearThreshold, handler} */
    /* ONU */
	{ CTC_VAR_REQBRANCH_INSTANT_ONU,   CTC_OAM_ALARMCODE_POWER,                   4,   0,   0, alarm_dummy_handler},
	{ CTC_VAR_REQBRANCH_INSTANT_ONU,   CTC_OAM_ALARMCODE_BATTERYMISSING,          0,   0,   0, alarm_dummy_handler},
	{ CTC_VAR_REQBRANCH_INSTANT_ONU,   CTC_OAM_ALARMCODE_BATTERYFAILURE,          0,   0,   0, alarm_dummy_handler},
    { CTC_VAR_REQBRANCH_INSTANT_ONU,   CTC_OAM_ALARMCODE_BATTERYVOLTLOW,          4,   0,   0, alarm_dummy_handler},
    { CTC_VAR_REQBRANCH_INSTANT_ONU,   CTC_OAM_ALARMCODE_ONUSELFTESTFAILURE,      4,   0,   0, alarm_dummy_handler},
    /* PON IF */
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_RXPOWERHIGH,             4,   POWER_HIGH_DEFAULT_VALUE,   POWER_HIGH_DEFAULT_VALUE, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_RXPOWERLOW,              4,   0,   0, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TXPOWERHIGH,             4,   POWER_HIGH_DEFAULT_VALUE,   POWER_HIGH_DEFAULT_VALUE, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TXPOWERLOW,              4,   0,   0, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TXBIASHIGH,              4,   BIAS_HIGH_DEFAULT_VALUE,    BIAS_HIGH_DEFAULT_VALUE, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TXBIASLOW,               4,   0,   0, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_VCCHIGH,                 4,   VOLTAGE_HIGH_DEFAULT_VALUE, VOLTAGE_HIGH_DEFAULT_VALUE, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_VCCLOW,                  4,   0,   0, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TEMPHIGH,                4,   0x7fff,    0x7fff, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TEMPLOW,                 4,   0,   0, alarm_get_pon_transceiver},

	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_RXPOWERHIGHWARNING,      4,   POWER_HIGH_DEFAULT_VALUE,   POWER_HIGH_DEFAULT_VALUE, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_RXPOWERLOWWARNING,       4,   0,   0, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TXPOWERHIGHWARNING,      4,   POWER_HIGH_DEFAULT_VALUE,   POWER_HIGH_DEFAULT_VALUE, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TXPOWERLOWWARNING,       4,   0,   0, alarm_get_pon_transceiver},    
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TXBIASHIGHWARNING,       4,   BIAS_HIGH_DEFAULT_VALUE,    BIAS_HIGH_DEFAULT_VALUE, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TXBIASLOWWARNING,        4,   0,   0, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_VCCHIGHWARNING,          4,   VOLTAGE_HIGH_DEFAULT_VALUE, VOLTAGE_HIGH_DEFAULT_VALUE, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_VCCLOWWARNING,           4,   0,   0, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TEMPHIGHWARNING,         4,   0x7fff,    0x7fff, alarm_get_pon_transceiver},
    { CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_TEMPLOWWARNING,          4,   0,   0, alarm_get_pon_transceiver},

	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSDROPEVENTS,            COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USDROPEVENTS,            COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSCRCERRORFRAMES,        COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USCRCERRORFRAMES,        COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSUNDERSIZEFRAMES,       COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USUNDERSIZEFRAMES,       COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSOVERSIZEFRAMES,        COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USOVERSIZEFRAMES,        COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},	
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSFRAGMENTS,        	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USFRAGMENTS,             COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSJABBERS,               COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USJABBERS,               COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSDISCARDFRAMES,         COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USDISCARDFRAMES,         COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSERRORFRAMES,           COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_error_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USERRORFRAMES,           COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_error_stats},

	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSDROPEVENTSWARNING,     COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USDROPEVENTSWARNING,     COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSCRCERRORFRAMESWARNING, COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USCRCERRORFRAMESWARNING, COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSUNDERSIZEFRAMESWARNING,COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USUNDERSIZEFRAMESWARNING,COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSOVERSIZEFRAMESWARNING, COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USOVERSIZEFRAMESWARNING, COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSFRAGMENTSWARNING,      COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USFRAGMENTSWARNING,      COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSJABBERSWARNING,        COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USJABBERSWARNING,        COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSDISCARDFRAMESWARNING,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_rx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USDISCARDFRAMESWARNING,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_tx_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_DSERRORFRAMESWARNING,    COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_error_stats},
	{ CTC_VAR_REQBRANCH_INSTANT_PONIF, CTC_OAM_ALARMCODE_USERRORFRAMESWARNING,    COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_pon_port_error_stats},
	
    /* Port */
	{ CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_ETHPORTAUTONEGFAILURE,   0,   0,   0, alarm_get_port_autoneg_status},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_ETHPORTLOS,              0,   0,   0, alarm_get_port_status},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_ETHPORTFAILURE,          0,   0,   0, alarm_dummy_handler},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_ETHPORTLOOKBACK,  		  0,   0,   0, alarm_get_port_loop_status},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_ETHPORTCONGESTION,       0,   0,   0, alarm_dummy_handler},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMCRCERRORFRAMES,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},    
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMDROPEVENTS,  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMUNDERSIZEFRAMES, COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMOVERSIZEFRAMES,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMFRAGMENTS,  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMJABBERS,  	 	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMDISCARDS,  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMERRORS,  	  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_error_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMCRCERRORFRAMES,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMDROPEVENTS,    COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMUNDERSIZEFRAMES, COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMOVERSIZEFRAMES,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMFRAGMENTS,  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMJABBERS,  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMDISCARDS,  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMERRORS,  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_error_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_STATUSChANGETIMES,  	  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_status_change_times},
    
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMCRCERRORFRAMESWARNING,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMDROPEVENTSWARNING,  	 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMUNDERSIZEFRAMESWARNING, COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMOVERSIZEFRAMESWARNING,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMFRAGMENTSWARNING,  	 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMJABBERSWARNING,  		 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMDISCARDSWARNING,  		 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_rx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_UPSTREAMERRORSWARNING,  		 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_error_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMCRCERRORFRAMESWARNING,COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMDROPEVENTSWARNING,  	 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMUNDERSIZEFRAMESWARNING, COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMOVERSIZEFRAMESWARNING,  COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMFRAGMENTSWARNING,  	 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMJABBERSWARNING,  	 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMDISCARDSWARNING,  	 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_tx_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_DOWNSTREAMERRORSWARNING,  		 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_error_stats},
    { CTC_VAR_REQBRANCH_INSTANT_PORT,  CTC_OAM_ALARMCODE_STATUSChANGETIMESWARNING,  	 COUNTER_SIZE,   COUNTER_DEFAULT_VALUE,   COUNTER_DEFAULT_VALUE, alarm_get_port_status_change_times},
};

/*alarm check function start*/
/*covert value of power Watt to value of dbm*/
static float power_to_dbm(unsigned int threshold)
{
    float tmp;

    tmp = __log10(((float)(threshold)*1/10000))*10;
    
    return tmp;
}

static void alarm_pon_power_check(ctc_eventAlarmCode_t  * alarm, int * state, rtk_transceiver_data_t * pData)
{
    float tmp, threshold, clearThreshold;
    int upperBound;
	unsigned int res;
    
    tmp = power_to_dbm(((pData->buf[0] << 8) | pData->buf[1]));
    
    threshold = power_to_dbm(alarm->threshold);
    clearThreshold = power_to_dbm(alarm->clearThreshold);
    
    if((alarm->alarmId == CTC_OAM_ALARMCODE_RXPOWERHIGH) ||
       (alarm->alarmId == CTC_OAM_ALARMCODE_RXPOWERHIGHWARNING) ||
       (alarm->alarmId == CTC_OAM_ALARMCODE_TXPOWERHIGH) ||
       (alarm->alarmId == CTC_OAM_ALARMCODE_TXPOWERHIGHWARNING) )
    {
        upperBound = 1;
    }
    else
    {
        upperBound = 0;
    }
	
    if(alarm->alarmState == CTC_OAM_ALARM_NORMAL)
    {
        if((upperBound && tmp > threshold) ||
           (!upperBound && tmp < threshold) )
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] alarmId[%d] current[%f] threshold[%f]\n",
                           __func__,__LINE__,alarm->alarmId,tmp,threshold);
            *state = CTC_OAM_ALARM_REPORT;
			res = ((pData->buf[0] << 8) | pData->buf[1]);
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
         }
    }
    else
    {
        if((upperBound && tmp < clearThreshold) ||
           (!upperBound && tmp > clearThreshold))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] alarmId[%d] current[%f] clearThreshold[%f]\n",
                           __func__,__LINE__,alarm->alarmId,tmp,clearThreshold);
            *state = CTC_OAM_ALARM_NORMAL;
			res = ((pData->buf[0] << 8) | pData->buf[1]);
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
        }
        else
        {
            *state = CTC_OAM_ALARM_REPORT;
        }
    }
}

static void alarm_pon_bias_check(ctc_eventAlarmCode_t  * alarm, int * state, rtk_transceiver_data_t * pData)
{
    double tmp, threshold, clearThreshold;
    int upperBound;
    unsigned int res;
	
    tmp = (double)((pData->buf[0] << 8) | pData->buf[1])*2/1000;
    
    threshold = alarm->threshold*2/1000;
    clearThreshold = alarm->clearThreshold*2/1000;
    
    if((alarm->alarmId == CTC_OAM_ALARMCODE_TXBIASHIGH) ||
       (alarm->alarmId == CTC_OAM_ALARMCODE_TXBIASHIGHWARNING) )
    {
        upperBound = 1;
    }
    else
    {
        upperBound = 0;
    }
	
    if(alarm->alarmState == CTC_OAM_ALARM_NORMAL)
    {
        if((upperBound && tmp > threshold) ||
           (!upperBound && tmp < threshold) )
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] alarmId[%d] current[%f] threshold[%f]\n",
                           __func__,__LINE__,alarm->alarmId,tmp,threshold);
            *state = CTC_OAM_ALARM_REPORT;
			res = ((pData->buf[0] << 8) | pData->buf[1]);
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
         }
    }
    else
    {
        if((upperBound && tmp < clearThreshold) ||
           (!upperBound && tmp > clearThreshold))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] alarmId[%d] current[%f] clearThreshold[%f]\n",
                           __func__,__LINE__,alarm->alarmId,tmp,clearThreshold);
            *state = CTC_OAM_ALARM_NORMAL;
			res = ((pData->buf[0] << 8) | pData->buf[1]);
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
        }
        else
        {
            *state = CTC_OAM_ALARM_REPORT;
        }
    }
}

static void alarm_pon_voltage_check(ctc_eventAlarmCode_t  * alarm, int * state, rtk_transceiver_data_t * pData)
{
    double tmp, threshold, clearThreshold;
    int upperBound;
    unsigned int res;
	
    tmp = (double)((pData->buf[0] << 8) | pData->buf[1])*1/10000;
    
    threshold = alarm->threshold*1/10000;
    clearThreshold = alarm->clearThreshold*1/10000;
    
    if((alarm->alarmId == CTC_OAM_ALARMCODE_VCCHIGH) ||
       (alarm->alarmId == CTC_OAM_ALARMCODE_VCCHIGHWARNING) )
    {
        upperBound = 1;
    }
    else
    {
        upperBound = 0;
    }
    
    if(alarm->alarmState == CTC_OAM_ALARM_NORMAL)
    {
        if((upperBound && tmp > threshold) ||
           (!upperBound && tmp < threshold) )
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] alarmId[%d] current[%f] threshold[%f]\n",
                           __func__,__LINE__,alarm->alarmId,tmp,threshold);
            *state = CTC_OAM_ALARM_REPORT;
			res = ((pData->buf[0] << 8) | pData->buf[1]);
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
         }
    }
    else
    {
        if((upperBound && tmp < clearThreshold) ||
           (!upperBound && tmp > clearThreshold))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] alarmId[%d] current[%f] clearThreshold[%f]\n",
                           __func__,__LINE__,alarm->alarmId,tmp,clearThreshold);
            *state = CTC_OAM_ALARM_NORMAL;
			res = ((pData->buf[0] << 8) | pData->buf[1]);
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
        }
        else
        {
            *state = CTC_OAM_ALARM_REPORT;
        }
    }
}

static double get_temperature(unsigned char * buf)
{
	double tmp;
	if (128 >= buf[0]) //MSB: pSrcData->buf[0]; LSB: pSrcData->buf[1]
	{
		tmp = (-1)*((~(buf[0]))+1)+((double)(buf[1])*1/256);
	}else{
		tmp = (char)buf[0]+((double)(buf[1])*1/256);
	}
	return tmp;
}

static void alarm_pon_temperature_check(ctc_eventAlarmCode_t * alarm,int * state,rtk_transceiver_data_t * pData)
{
    double tmp, threshold, clearThreshold;
    int upperBound;
	unsigned int res;
	short tmpThreshold, tmpClearThreshold;
	
	tmp = get_temperature(pData->buf);

	tmpThreshold = alarm->threshold;
    threshold = get_temperature((unsigned char *)&tmpThreshold);
	tmpClearThreshold = alarm->clearThreshold;
    clearThreshold = get_temperature((unsigned char *)&tmpClearThreshold);
    
    if((alarm->alarmId == CTC_OAM_ALARMCODE_TEMPHIGH) ||
       (alarm->alarmId == CTC_OAM_ALARMCODE_TEMPHIGHWARNING) )
    {
        upperBound = 1;
    }
    else
    {
        upperBound = 0;
    }
    
    if(alarm->alarmState == CTC_OAM_ALARM_NORMAL)
    {
        if((upperBound && tmp > threshold) ||
           (!upperBound && tmp < threshold) )
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] alarmId[%d] current[%f] threshold[%f]\n",
                           __func__,__LINE__,alarm->alarmId,tmp,threshold);
            *state = CTC_OAM_ALARM_REPORT;
			res = ((pData->buf[0] << 8) | pData->buf[1]);
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
         }
    }
    else
    {
        if((upperBound && tmp < clearThreshold) ||
           (!upperBound && tmp > clearThreshold))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] alarmId[%d] current[%f] clearThreshold[%f]\n",
                           __func__,__LINE__,alarm->alarmId,tmp,clearThreshold);
            *state = CTC_OAM_ALARM_NORMAL;
			res = ((pData->buf[0] << 8) | pData->buf[1]);
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
        }
        else
        {
            *state = CTC_OAM_ALARM_REPORT;
        }
    }
}

int alarm_get_pon_transceiver(ctc_eventAlarmCode_t  * alarm, int * state)
{
    int32 ret;
    rtk_transceiver_parameter_type_t type;
    rtk_transceiver_data_t dataCfg;
    
    *state = CTC_OAM_ALARM_NORMAL;

    switch(alarm->alarmId)
    {
        case CTC_OAM_ALARMCODE_RXPOWERHIGH:
        case CTC_OAM_ALARMCODE_RXPOWERLOW:
        case CTC_OAM_ALARMCODE_RXPOWERHIGHWARNING:
        case CTC_OAM_ALARMCODE_RXPOWERLOWWARNING:
            type = RTK_TRANSCEIVER_PARA_TYPE_RX_POWER;
            break;
        case CTC_OAM_ALARMCODE_TXPOWERHIGH:
        case CTC_OAM_ALARMCODE_TXPOWERLOW:
        case CTC_OAM_ALARMCODE_TXPOWERHIGHWARNING:
        case CTC_OAM_ALARMCODE_TXPOWERLOWWARNING:
            type = RTK_TRANSCEIVER_PARA_TYPE_TX_POWER;
            break;
		case CTC_OAM_ALARMCODE_TXBIASHIGH:
		case CTC_OAM_ALARMCODE_TXBIASLOW:
		case CTC_OAM_ALARMCODE_TXBIASHIGHWARNING:
		case CTC_OAM_ALARMCODE_TXBIASLOWWARNING:
			type = RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT;
			break;
		case CTC_OAM_ALARMCODE_VCCHIGH:
		case CTC_OAM_ALARMCODE_VCCLOW:
		case CTC_OAM_ALARMCODE_VCCHIGHWARNING:
		case CTC_OAM_ALARMCODE_VCCLOWWARNING:
			type = RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE;
			break;
		case CTC_OAM_ALARMCODE_TEMPHIGH:
		case CTC_OAM_ALARMCODE_TEMPLOW:
		case CTC_OAM_ALARMCODE_TEMPHIGHWARNING:
		case CTC_OAM_ALARMCODE_TEMPLOWWARNING:
			type = RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE;
			break;
        default:
            return RT_ERR_FAILED;
    }

    ret = rtk_ponmac_transceiver_get(type, &dataCfg);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    if((type == RTK_TRANSCEIVER_PARA_TYPE_RX_POWER) ||
       (type == RTK_TRANSCEIVER_PARA_TYPE_TX_POWER))
    {
        alarm_pon_power_check(alarm, state, &dataCfg);
    }
	else if(type == RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT)
	{
		alarm_pon_bias_check(alarm, state, &dataCfg);
	}
	else if(type == RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE)
	{
		alarm_pon_voltage_check(alarm, state, &dataCfg);
	}
	else if(type == RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE)
	{
		alarm_pon_temperature_check(alarm, state, &dataCfg);
	}
    return RT_ERR_OK;
}

static void alarm_counter_check(ctc_eventAlarmCode_t  * alarm, int * state, uint64 cntr)
{
	unsigned int res;
	
	/*TEST code must remove*/
	//cntr = 100;
				
	if(alarm->alarmState == CTC_OAM_ALARM_NORMAL)
    {
    	if(cntr > alarm->threshold)
    	{
        	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] pon port alarmId[0x%x] cnt:%25llu\n",
                            __func__,__LINE__,alarm->alarmId,cntr);
            *state = CTC_OAM_ALARM_REPORT;
		#ifdef CTC_3_ALARM_COUNTER_SIZE
			CTC_BUF_ENCODE64(alarm->alarmInfo, &cntr);
		#else
			res = cntr;
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
		#endif
    	}
    }
	else
	{
		if(cntr < alarm->clearThreshold)
    	{
        	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                           "%s[%d] pon port alarmId[0x%x] cnt:%25llu\n",
                           __func__,__LINE__,alarm->alarmId,cntr);
            *state = CTC_OAM_ALARM_NORMAL;
		#ifdef CTC_3_ALARM_COUNTER_SIZE
			CTC_BUF_ENCODE64(alarm->alarmInfo, &cntr);
		#else
			res = cntr;
			CTC_BUF_ENCODE32(alarm->alarmInfo, &res);
		#endif
		}
		else
		{
				*state = CTC_OAM_ALARM_REPORT;
		}
	}
}

/* FIXME: for pon port, get total number of all error stats */
int alarm_get_pon_port_error_stats(ctc_eventAlarmCode_t  * alarm, int * state)
{
	uint64 cntr,sum = 0;
	rtk_port_t port;

	port = HAL_GET_PON_PORT();
	if(alarm->alarmId == CTC_OAM_ALARMCODE_DSERRORFRAMES ||
		alarm->alarmId == CTC_OAM_ALARMCODE_DSERRORFRAMESWARNING)
	{
		if (rtk_stat_port_get(port, ETHER_STATS_CRC_ALIGN_ERRORS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_RX_UNDER_SIZE_PKTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_RX_OVERSIZE_PKTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_FRAGMENTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_JABBERS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
	}
	else
	{
		if (rtk_stat_port_get(port, ETHER_STATS_TX_UNDER_SIZE_PKTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_TX_OVERSIZE_PKTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
	}

	alarm_counter_check(alarm,state,sum);
    return RT_ERR_OK;
}

/* FIXME: get stats according to 9602 and 9602C spec. other chips may support different ASIC counters */
int alarm_get_pon_port_rx_stats(ctc_eventAlarmCode_t  * alarm, int * state)
{
	rtk_stat_port_type_t type = MIB_PORT_CNTR_END;
	rtk_port_t port;
	uint64 cntr;
	uint32 uiLPort;
	unsigned int res;
	
	*state = CTC_OAM_ALARM_NORMAL;

	switch(alarm->alarmId)
	{
		case CTC_OAM_ALARMCODE_DSDROPEVENTS:
		case CTC_OAM_ALARMCODE_DSDROPEVENTSWARNING:
			type = ETHER_STATS_DROP_EVENTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DSCRCERRORFRAMES:
		case CTC_OAM_ALARMCODE_DSCRCERRORFRAMESWARNING:
			type = ETHER_STATS_CRC_ALIGN_ERRORS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DSUNDERSIZEFRAMES:
		case CTC_OAM_ALARMCODE_DSUNDERSIZEFRAMESWARNING:
			type = ETHER_STATS_RX_UNDER_SIZE_PKTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DSOVERSIZEFRAMES:
		case CTC_OAM_ALARMCODE_DSOVERSIZEFRAMESWARNING:
			type = ETHER_STATS_RX_OVERSIZE_PKTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DSFRAGMENTS:
		case CTC_OAM_ALARMCODE_DSFRAGMENTSWARNING:
			type = ETHER_STATS_FRAGMENTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DSJABBERS:
		case CTC_OAM_ALARMCODE_DSJABBERSWARNING:
			type = ETHER_STATS_JABBERS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DSDISCARDFRAMES:
		case CTC_OAM_ALARMCODE_DSDISCARDFRAMESWARNING:
			type = DOT1D_TP_PORT_IN_DISCARDS_INDEX;
			break;
		default:
			return RT_ERR_FAILED;
	}

	if(type == MIB_PORT_CNTR_END)
		return RT_ERR_FAILED;
	
    port = HAL_GET_PON_PORT();
	
	if (rtk_stat_port_get(port, type, &cntr) == RT_ERR_OK)
    {
    	alarm_counter_check(alarm,state,cntr);
        return RT_ERR_OK;
	}

    return RT_ERR_FAILED;
}

/* FIXME: get stats according to 9602 and 9602C spec. other chips may support different ASIC counters */
int alarm_get_pon_port_tx_stats(ctc_eventAlarmCode_t  * alarm, int * state)
{
	rtk_stat_port_type_t type = MIB_PORT_CNTR_END;
	rtk_port_t port;
	uint64 cntr;
	uint32 uiLPort;
	unsigned int res;
	
	*state = CTC_OAM_ALARM_NORMAL;

	switch(alarm->alarmId)
	{
		case CTC_OAM_ALARMCODE_USDROPEVENTS:
		case CTC_OAM_ALARMCODE_USDROPEVENTSWARNING:
			/* ASIC not support */
			break;
		case CTC_OAM_ALARMCODE_USCRCERRORFRAMES:
		case CTC_OAM_ALARMCODE_USCRCERRORFRAMESWARNING:
			/* ASIC not support */
			break;
		case CTC_OAM_ALARMCODE_USUNDERSIZEFRAMES:
		case CTC_OAM_ALARMCODE_USUNDERSIZEFRAMESWARNING:
			type = ETHER_STATS_TX_UNDER_SIZE_PKTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_USOVERSIZEFRAMES:
		case CTC_OAM_ALARMCODE_USOVERSIZEFRAMESWARNING:
			type = ETHER_STATS_TX_OVERSIZE_PKTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_USFRAGMENTS:
		case CTC_OAM_ALARMCODE_USFRAGMENTSWARNING:
			/* ASIC not support */
			break;
		case CTC_OAM_ALARMCODE_USJABBERS:
		case CTC_OAM_ALARMCODE_USJABBERSWARNING:
			/* ASIC not support */
			break;
		case CTC_OAM_ALARMCODE_USDISCARDFRAMES:
		case CTC_OAM_ALARMCODE_USDISCARDFRAMESWARNING:
			type = IF_OUT_DISCARDS_INDEX;
			break;
		default:
			return RT_ERR_FAILED;
	}

	if(type == MIB_PORT_CNTR_END)
		return RT_ERR_FAILED;
	
	port = HAL_GET_PON_PORT();
	
	if (rtk_stat_port_get(port, type, &cntr) == RT_ERR_OK)
    {
    	alarm_counter_check(alarm,state,cntr);
        return RT_ERR_OK;
	}
	return RT_ERR_FAILED;
}

int alarm_get_port_status(ctc_eventAlarmCode_t  * alarm, int * state)
{
    int32 ret;
    rtk_port_linkStatus_t linkStatus;
    uint32 uiLPort;
    uint32 uiPPort;
    
    *state = CTC_OAM_ALARM_NORMAL;

    uiLPort = (alarm->alarmInstance & 0xFF) -1;
    uiPPort = PortLogic2PhyID(uiLPort);
    
    ret = rtk_port_link_get(uiPPort, &linkStatus);
    if (RT_ERR_OK != ret)
	{
	    return RT_ERR_FAILED;
    }
    
    if(linkStatus == 0)
    {
        *state = CTC_OAM_ALARM_REPORT;
    }

    return RT_ERR_OK;
}

/* FIXME: for lan port, get total number of all error stats */
int alarm_get_port_error_stats(ctc_eventAlarmCode_t  * alarm, int * state)
{
	uint64 cntr,sum = 0;
	rtk_port_t port;
	uint32 uiLPort;
	
	uiLPort = (alarm->alarmInstance & 0xFF) -1;
    port = PortLogic2PhyID(uiLPort);
	
	if(alarm->alarmId == CTC_OAM_ALARMCODE_UPSTREAMERRORS ||
		alarm->alarmId == CTC_OAM_ALARMCODE_UPSTREAMERRORSWARNING)
	{
		if (rtk_stat_port_get(port, ETHER_STATS_CRC_ALIGN_ERRORS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_RX_UNDER_SIZE_PKTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_RX_OVERSIZE_PKTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_FRAGMENTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_JABBERS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
	}
	else
	{
		if (rtk_stat_port_get(port, ETHER_STATS_TX_UNDER_SIZE_PKTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
		if (rtk_stat_port_get(port, ETHER_STATS_TX_OVERSIZE_PKTS_INDEX, &cntr) == RT_ERR_OK)
	    {
	    	sum += cntr;
		}
	}

	alarm_counter_check(alarm,state,sum);
    return RT_ERR_OK;
}

/* FIXME: get stats according to 9602 and 9602C spec. other chips may support different ASIC counters */
int alarm_get_port_rx_stats(ctc_eventAlarmCode_t  * alarm, int * state)
{
	rtk_stat_port_type_t type;
	rtk_port_t port;
	uint64 cntr;
	uint32 uiLPort;
	unsigned int res;
	
	*state = CTC_OAM_ALARM_NORMAL;

	switch(alarm->alarmId)
	{
		case CTC_OAM_ALARMCODE_UPSTREAMDROPEVENTS:
		case CTC_OAM_ALARMCODE_UPSTREAMDROPEVENTSWARNING:
			type = ETHER_STATS_DROP_EVENTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_UPSTREAMCRCERRORFRAMES:
		case CTC_OAM_ALARMCODE_UPSTREAMCRCERRORFRAMESWARNING:
			type = ETHER_STATS_CRC_ALIGN_ERRORS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_UPSTREAMUNDERSIZEFRAMES:
		case CTC_OAM_ALARMCODE_UPSTREAMUNDERSIZEFRAMESWARNING:
			type = ETHER_STATS_RX_UNDER_SIZE_PKTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_UPSTREAMOVERSIZEFRAMES:
		case CTC_OAM_ALARMCODE_UPSTREAMOVERSIZEFRAMESWARNING:
			type = ETHER_STATS_RX_OVERSIZE_PKTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_UPSTREAMFRAGMENTS:
		case CTC_OAM_ALARMCODE_UPSTREAMFRAGMENTSWARNING:
			type = ETHER_STATS_FRAGMENTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_UPSTREAMJABBERS:
		case CTC_OAM_ALARMCODE_UPSTREAMJABBERSWARNING:
			type = ETHER_STATS_JABBERS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_UPSTREAMDISCARDS:
		case CTC_OAM_ALARMCODE_UPSTREAMDISCARDSWARNING:
			//ASIC not suport
			return RT_ERR_FAILED;
		default:
			return RT_ERR_FAILED;
	}
	
	uiLPort = (alarm->alarmInstance & 0xFF) -1;
    port = PortLogic2PhyID(uiLPort);
	
	if (rtk_stat_port_get(port, type, &cntr) == RT_ERR_OK)
    {
    	alarm_counter_check(alarm,state,cntr);
        return RT_ERR_OK;
	}

    return RT_ERR_FAILED;
}

/* FIXME: get stats according to 9602 and 9602C spec. other chips may support different ASIC counters */
int alarm_get_port_tx_stats(ctc_eventAlarmCode_t  * alarm, int * state)
{
	rtk_stat_port_type_t type;
	rtk_port_t port;
	uint64 cntr;
	uint32 uiLPort;
	unsigned int res;
	
	*state = CTC_OAM_ALARM_NORMAL;

	switch(alarm->alarmId)
	{
		case CTC_OAM_ALARMCODE_DOWNSTREAMDROPEVENTS:
		case CTC_OAM_ALARMCODE_DOWNSTREAMDROPEVENTSWARNING:
			//ASIC not suport
			return RT_ERR_FAILED;
		case CTC_OAM_ALARMCODE_DOWNSTREAMCRCERRORFRAMES:
		case CTC_OAM_ALARMCODE_DOWNSTREAMCRCERRORFRAMESWARNING:
			//ASIC not suport
			return RT_ERR_FAILED;
		case CTC_OAM_ALARMCODE_DOWNSTREAMUNDERSIZEFRAMES:
		case CTC_OAM_ALARMCODE_DOWNSTREAMUNDERSIZEFRAMESWARNING:
			type = ETHER_STATS_TX_UNDER_SIZE_PKTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DOWNSTREAMOVERSIZEFRAMES:
		case CTC_OAM_ALARMCODE_DOWNSTREAMOVERSIZEFRAMESWARNING:
			type = ETHER_STATS_TX_OVERSIZE_PKTS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DOWNSTREAMFRAGMENTS:
		case CTC_OAM_ALARMCODE_DOWNSTREAMFRAGMENTSWARNING:
			//ASIC not suport
			return RT_ERR_FAILED;
		case CTC_OAM_ALARMCODE_DOWNSTREAMJABBERS:
		case CTC_OAM_ALARMCODE_DOWNSTREAMJABBERSWARNING:
			//ASIC not suport
			return RT_ERR_FAILED;
		case CTC_OAM_ALARMCODE_DOWNSTREAMDISCARDS:
		case CTC_OAM_ALARMCODE_DOWNSTREAMDISCARDSWARNING:
			type = IF_OUT_DISCARDS_INDEX;
			break;
		case CTC_OAM_ALARMCODE_DOWNSTREAMERRORS:
		case CTC_OAM_ALARMCODE_DOWNSTREAMERRORSWARNING:
			//ASIC not suport
			return RT_ERR_FAILED;
		default:
			return RT_ERR_FAILED;
	}
	
	uiLPort = (alarm->alarmInstance & 0xFF) -1;
    port = PortLogic2PhyID(uiLPort);
	
	if (rtk_stat_port_get(port, type, &cntr) == RT_ERR_OK)
    {
       	alarm_counter_check(alarm,state,cntr);
        return RT_ERR_OK;
	}

    return RT_ERR_FAILED;
}

int port_link_status_change_times_get(rtk_port_t portindex)
{
	FILE* fp;
	char linebuf[512];
	int portidx, linkchanges;
	if((fp=fopen("/proc/rtl8686gmac/dev_port_mapping","r"))!=NULL)
	{
		while(fgets(linebuf,sizeof(linebuf),fp)!=NULL)
		{
			if(sscanf(linebuf,"Port%d => %*s , %*s , %*s , changes:%d\n", &portidx, &linkchanges)==2)
			{
				if(portidx==portindex)
				{
					//printf("port:%d changes:%d",portidx, linkchanges);
					fclose(fp);
					return linkchanges;
				}
			}
		}
		fclose(fp);
	}
	return RT_ERR_FAILED;
}

int alarm_get_port_status_change_times(ctc_eventAlarmCode_t  * alarm, int * state)
{
	uint32 uiLPort;
	unsigned int cntr;
	rtk_port_t port;
	uint64 res;

	*state = CTC_OAM_ALARM_NORMAL;
	
	uiLPort = (alarm->alarmInstance & 0xFF) -1;
	port = PortLogic2PhyID(uiLPort);
	cntr = port_link_status_change_times_get(port);
	if(cntr!=RT_ERR_FAILED)
	{
		if(alarm->alarmState == CTC_OAM_ALARM_NORMAL)
    	{
    		if(cntr > alarm->threshold)
    		{
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                               "%s[%d] alarmId[0x%x] port[%d] cnt:%d\n",
                               __func__,__LINE__,alarm->alarmId,port,cntr);
                *state = CTC_OAM_ALARM_REPORT;
			#ifdef CTC_3_ALARM_COUNTER_SIZE
				res = cntr;
				CTC_BUF_ENCODE64(alarm->alarmInfo, &res);
			#else
				CTC_BUF_ENCODE32(alarm->alarmInfo, &cntr);
			#endif
    		}
    	}
		else
		{
			if(cntr < alarm->clearThreshold)
    		{
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                               "%s[%d] alarmId[0x%x] port[%d] cnt:%d\n",
                               __func__,__LINE__,alarm->alarmId,port,cntr);
                *state = CTC_OAM_ALARM_NORMAL;
			#ifdef CTC_3_ALARM_COUNTER_SIZE
				res = cntr;
				CTC_BUF_ENCODE64(alarm->alarmInfo, &res);
			#else
				CTC_BUF_ENCODE32(alarm->alarmInfo, &cntr);
			#endif
			}
			else
			{
				*state = CTC_OAM_ALARM_REPORT;
			}
		}
        return RT_ERR_OK;
	}
	return RT_ERR_FAILED;
}

/* diag port get phy-reg port 0 page 0 register 0 
   bit 5 of result: 1 success, 0 fail */
#define AUTONEG_SUCESS 0x20
int alarm_get_port_autoneg_status(ctc_eventAlarmCode_t  * alarm, int * state)
{
	int32 ret;
	uint32 uiLPort;
	uint32 uiPPort;
	uint32 data;

	*state = CTC_OAM_ALARM_NORMAL;
	
	uiLPort = (alarm->alarmInstance & 0xFF) -1;
	uiPPort = PortLogic2PhyID(uiLPort);
	
#if (defined (FPGA_DEFINED)) && (!defined(CONFIG_SDK_RTL9601B))
	{
    	uint16 inputData;
    	uint16 outputData;

        /*change page*/
        inputData = 0xC000 + (port<<5) + (31);

        io_mii_phy_reg_write(8,1,0);
        io_mii_phy_reg_write(8,0,inputData);

        inputData = 0x8000 + (uiPPort<<5) + 0;
        io_mii_phy_reg_write(8,0,inputData);
        io_mii_phy_reg_read(8,2,&outputData);
		data = outputData;
	}
#else
    ret = rtk_port_phyReg_get(uiPPort, 0, 0, &data);
	if (RT_ERR_OK != ret)
	{
	    return RT_ERR_FAILED;
    }
#endif

	if((data & AUTONEG_SUCESS) == 0)
		*state = CTC_OAM_ALARM_REPORT;
	
	return RT_ERR_OK;
}

int alarm_get_port_loop_status(ctc_eventAlarmCode_t  * alarm, int * value)
{
	uint32 uiLPort;
	int status;

	uiLPort = (alarm->alarmInstance & 0xFF) -1;
	status = get_port_loopdetect_status(uiLPort);
	if(status==1)
		*value = CTC_OAM_ALARM_REPORT;
	else if(status==0)
		*value = CTC_OAM_ALARM_NORMAL;
	else//remain old status
		*value = alarm->alarmState;
	return RT_ERR_OK;
}

int alarm_dummy_handler(ctc_eventAlarmCode_t  * alarm, int * value)
{
    *value = CTC_OAM_ALARM_NORMAL;
    return RT_ERR_OK;
}
/*alarm check function end*/

static void alarm_get_func(unsigned short alarmId, ctc_oam_alarm_t ** alarm)
{
    int i;
    int typeNum;
    typeNum = sizeof(ctc_alarmTable)/sizeof(ctc_alarmTable[0]);

    *alarm = NULL;

    for( i = 0; i < typeNum; i++ )
    {
        if(ctc_alarmTable[i].alarmId == alarmId)
		{
		    *alarm = &ctc_alarmTable[i];
            break;
        }
    }
}

ctc_eventAlarmCode_t * ctc_oam_alarm_get_entry(unsigned short alarmId, unsigned int alarmInstance)
{
    int i;
        
    for(i = 0; i < alarmNumber; i++)
    {
        if(alarmDb[i].alarmId == alarmId)
        {
            if((alarmDb[i].alarmTarget == CTC_VAR_REQBRANCH_INSTANT_PORT) &&
                ((alarmDb[i].alarmInstance & 0xFFFF) != (alarmInstance & 0xFFFF)))
            {
                continue;
            }
            
            return &alarmDb[i];           
        }
    } 
}

void ctc_oam_alarm_event_notification_gen(
    ctc_eventOam_t *eventStatus,
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short *pReplyLen)  /* Reply size used by this handler*/ 
{
    unsigned char eventLen;

    eventLen = CTC_EVENT_NOTIFICATION_HDRLEN + CTC_EVENT_NOTIFICATION_CONTENT_MIN_LEN;
    eventLen += eventStatus->alarmInfoLen;

    /*generate org specific event notification header*/
    pReplyBuf[0] = CTC_EXTOAM_OPCODE_NOTIFICATION;
    pReplyBuf[1] = eventLen;
    pReplyBuf[2] = ctc_oui[0];
    pReplyBuf[3] = ctc_oui[1];
    pReplyBuf[4] = ctc_oui[2];

    pReplyBuf += CTC_EVENT_NOTIFICATION_HDRLEN;

    /* 2 bytes ObjectType */
    CTC_BUF_ENCODE16(pReplyBuf, &eventStatus->objectType);
    pReplyBuf += 2;
    /* 4 bytes ObjectInstance */
    CTC_BUF_ENCODE32(pReplyBuf, &eventStatus->objectInstance);
    pReplyBuf += 4;
    /* 2 bytes AlarmId */
    CTC_BUF_ENCODE32(pReplyBuf, &eventStatus->alarmId);
    pReplyBuf += 2;
    /* 2 bytes Time Stamp set to 0x0000 */
    pReplyBuf[0] = 0x00;
    pReplyBuf[1] = 0x00;
    pReplyBuf += 2;
    /* 1 bytes AlarmState */
    pReplyBuf[0] = eventStatus->alarmState;
    pReplyBuf += 1;

    /* 4 bytes or 8 bytes AlarmInfo*/
    if(eventStatus->alarmInfoLen == 4)
    {
        CTC_BUF_ENCODE32(pReplyBuf, eventStatus->alarmInfo);
        pReplyBuf += 4;
    }
	else if(eventStatus->alarmInfoLen == 8)
	{
		CTC_BUF_ENCODE64(pReplyBuf, eventStatus->alarmInfo);
        pReplyBuf += 8;
	}
    *pReplyLen = eventLen;
}

int ctc_oam_event_notification_gen(
    unsigned char llidIdx,
    unsigned char **ppReplyOamPdu,
    unsigned short *pReplyLen,
    ctc_eventOam_t *eventStatus,
    unsigned short count)     
{
    int ret;
    unsigned short bufLen, replyLen, remainLen, genLen;
    unsigned char *pReplyPtr;
    int i;
    
    ret = epon_oam_maxOamPduSize_get(llidIdx, &bufLen);
    if(EPON_OAM_ERR_OK != ret)
    {
        /* Could not decide the OAMPDU size */
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,
            "[OAM:%s:%d] can't get OAMPDU max size\n", __FILE__, __LINE__);
        return RT_ERR_INPUT;
    }

    pReplyPtr = (unsigned char *)malloc(sizeof(char) * bufLen);
    if(NULL == pReplyPtr)
    {
        /* Could not malloc the OAMPDU size of memory*/
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,
            "[OAM:%s:%d] can't allocate memory for event notify OAMPDU\n", __FILE__, __LINE__);
        return RT_ERR_INPUT;
    }

    memset(pReplyPtr, 0, (sizeof(char) * bufLen));

    *ppReplyOamPdu = pReplyPtr;
    replyLen = 0;
    remainLen = bufLen;

    /*2 bytes Sequence Number*/
    CTC_BUF_ENCODE16(pReplyPtr, &sequenceNum);
    CTC_BUF_ADD(pReplyPtr, remainLen, 2);
    replyLen += 2;

    for(i = 0; i < count; i++)
    { 
        ctc_oam_alarm_event_notification_gen(&eventStatus[i],pReplyPtr,&genLen);
		pReplyPtr += genLen;
		replyLen += genLen;
        
        remainLen -= genLen;
        if(remainLen < CTC_EVENT_NOTIFICATION_LEN)
        {
            break;
        }
    }
    
    *pReplyLen = replyLen;
    sequenceNum++;
    return RT_ERR_OK;
}

int ctc_oam_event_notification_process(
    unsigned char llidIdx,
    unsigned char  code,
    ctc_eventOam_t *eventStatus,
    unsigned short count)  
{
    int ret;
    unsigned char *pReplyOamPdu;
    unsigned short replyLen;

    pReplyOamPdu = NULL;
    replyLen = 0;

    ret = ctc_oam_event_notification_gen(llidIdx, &pReplyOamPdu, &replyLen, eventStatus, count);

    if(replyLen > 0)
    {
        ret = epon_oam_reply_send(llidIdx, code, pReplyOamPdu, replyLen);
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
            "[OAM:%s:%d] epon_oam_reply_send %d\n", __FILE__, __LINE__, code);

    }
}

void ctc_oam_alarm_check()
{
    int i;
    int ret;
    uint32 value;
    ctc_oam_alarm_t * alarm;
    ctc_eventOam_t  eventStatus[CTC_EVENT_COUNT_MAX] = {{0}};
    unsigned short count = 0;

    for(i = 0; i < alarmNumber; i++)
    {
        if(alarmDb[i].alarmEnable)
        {
            alarm_get_func(alarmDb[i].alarmId, &alarm);
            if(alarm != NULL)
            {
                ret = alarm->handler(&alarmDb[i], &value);
               	if(ret != RT_ERR_OK)
					continue;
				
                if(alarmDb[i].alarmState != value)
                {             
                    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                                   "event notification alarmId[%x] %s\n", alarmDb[i].alarmId,
                                   value == CTC_OAM_ALARM_NORMAL?"clear":"report");

                    alarmDb[i].alarmState = value;

                    eventStatus[count].objectType = alarmDb[i].alarmTarget;
                    eventStatus[count].objectInstance = alarmDb[i].alarmInstance;
                    eventStatus[count].alarmId = alarmDb[i].alarmId;
                    if(alarmDb[i].alarmState == CTC_OAM_ALARM_REPORT)
                        eventStatus[count].alarmState = EVENT_NOTIFICATION_ALARM_STATE_REPORT;
                    else
                        eventStatus[count].alarmState = EVENT_NOTIFICATION_ALARM_STATE_CLEAR;

                    if(alarmDb[i].alarmInfoLen > 0)
                    {
                        eventStatus[count].alarmInfoLen = alarmDb[i].alarmInfoLen;
                        memcpy(eventStatus[count].alarmInfo, alarmDb[i].alarmInfo, CTC_EVENT_OAM_ALARMINFO_MAX);
                    }
                    count++;

                    if(count >= CTC_EVENT_COUNT_MAX)
                        break;
                }
                               
            }
        }
    }

    if(count > 0)
    {
        unsigned char llidIdx = 0;
        ctc_oam_event_notification_process(llidIdx, EPON_OAMPDU_CODE_EVENT, eventStatus, count);
    }
}

int ctc_oam_alarm_state_set(unsigned short alarmId, unsigned int alarmInstance, int enable)
{
    ctc_eventAlarmCode_t * alarmEntry;
    
    alarmEntry = ctc_oam_alarm_get_entry(alarmId, alarmInstance);
    if(alarmEntry != NULL)
    {
        alarmEntry->alarmEnable = enable;
        alarmEntry->alarmInstance = alarmInstance;
        //alarmEntry->alarmState = CTC_OAM_ALARM_NORMAL;
    }
    return RT_ERR_OK;
}

int ctc_oam_alarm_threshold_set(unsigned short alarmId, unsigned int alarmInstance, 
                                         unsigned int threshold,unsigned int clearThreshold)
{
    ctc_eventAlarmCode_t * alarmEntry;
    
    alarmEntry = ctc_oam_alarm_get_entry(alarmId, alarmInstance);
    if(alarmEntry != NULL)
    {
        alarmEntry->threshold = threshold;
        alarmEntry->clearThreshold = clearThreshold;
    }
    return RT_ERR_OK;
}

int ctc_oam_alarm_threshold_get(unsigned short alarmId, unsigned int alarmInstance, 
                                         unsigned int * threshold,unsigned int * clearThreshold)
{
    ctc_eventAlarmCode_t * alarmEntry;
    
    alarmEntry = ctc_oam_alarm_get_entry(alarmId, alarmInstance);
    if(alarmEntry != NULL)
    {
        *threshold = alarmEntry->threshold;
        *clearThreshold = alarmEntry->clearThreshold;		
    }
	else
	{ 	/* set zero if not support*/
		*threshold = 0;
		*clearThreshold = 0;
	}
    return RT_ERR_OK;
}

static int ctc_oam_get_alarm_size()
{
    int i,number;
	int portEntryNum = 0;

    number = sizeof(ctc_alarmTable)/sizeof(ctc_alarmTable[0]);
    for(i = 0;i < number;i++)
    {
        if(ctc_alarmTable[i].alarmTarget == CTC_VAR_REQBRANCH_INSTANT_PORT)
            portEntryNum++;
    }
    return (number + portEntryNum * ( LAN_PORT_NUM - 1));
}

void ctc_oam_alarm_init()
{
    int i,j=0;
    int alarmNum;
	int typeNum;
	unsigned int port;
    
	alarmNum = ctc_oam_get_alarm_size();     
    typeNum = sizeof(ctc_alarmTable)/sizeof(ctc_alarmTable[0]);
 
    alarmDb = malloc(alarmNum * sizeof(ctc_eventAlarmCode_t));
    memset(alarmDb, 0, (alarmNum * sizeof(ctc_eventAlarmCode_t)));

    for(i = 0; i < typeNum; i++ )
    {
        if(ctc_alarmTable[i].alarmTarget == CTC_VAR_REQBRANCH_INSTANT_PORT)
		{               
            FOR_EACH_LAN_PORT(port)
            {
                alarmDb[j].alarmTarget = ctc_alarmTable[i].alarmTarget;
                alarmDb[j].alarmId = ctc_alarmTable[i].alarmId;
                alarmDb[j].alarmInfoLen =ctc_alarmTable[i].alarmInfoLen;
                alarmDb[j].alarmEnable = DISABLED;
                alarmDb[j].alarmInstance = port+1;
                alarmDb[j].alarmState = CTC_OAM_ALARM_NORMAL;
                alarmDb[j].threshold = ctc_alarmTable[i].threshold;
                alarmDb[j].clearThreshold = ctc_alarmTable[i].clearThreshold;

#ifdef YUEME_CUSTOMIZED_CHANGE
				/* set loopback alarm enable by default */
				if(ctc_alarmTable[i].alarmId == CTC_OAM_ALARMCODE_ETHPORTLOOKBACK)
					alarmDb[j].alarmEnable = 1;
#endif
				
                j++;
            }
		}
        else
        {
            alarmDb[j].alarmTarget = ctc_alarmTable[i].alarmTarget;
            alarmDb[j].alarmId = ctc_alarmTable[i].alarmId;
            alarmDb[j].alarmInfoLen =ctc_alarmTable[i].alarmInfoLen;
            alarmDb[j].alarmEnable = DISABLED;
            alarmDb[j].alarmInstance = 0;
            alarmDb[j].alarmState = CTC_OAM_ALARM_NORMAL;
            alarmDb[j].threshold = ctc_alarmTable[i].threshold;
            alarmDb[j].clearThreshold = ctc_alarmTable[i].clearThreshold;
            j++;
        }   
    }

	alarmNumber = j;
}


void ctc_oam_alarm_show(int showDisable)
{
    int i;
    char * state[2] = {"normal","alarm"};
    char * target[3] = {"ONU", "PONIF", "PORT"};
	char * tmp;
	
    printf("\n----------------------------ALARM SETTING-----------------------------\n");
    printf("target  type  infoLen enable  state  objInstance  threshold   clearThrd\n");
    for(i = 0; i < alarmNumber; i++)
    {
    	if(showDisable == 0 && alarmDb[i].alarmEnable == 0)
			continue;
		
		if(alarmDb[i].alarmTarget == CTC_VAR_REQBRANCH_INSTANT_ONU)
			tmp = target[0];
		else if(alarmDb[i].alarmTarget == CTC_VAR_REQBRANCH_INSTANT_PONIF)
			tmp = target[1];
		else
			tmp = target[2];
		
		printf("%5s   0x%-4x  %3d   %3d    %6s   0x%-8x  0x%-8x  0x%-8x\n",
               tmp, alarmDb[i].alarmId, alarmDb[i].alarmInfoLen,
               alarmDb[i].alarmEnable, state[alarmDb[i].alarmState], alarmDb[i].alarmInstance, 
               alarmDb[i].threshold, alarmDb[i].clearThreshold);      
    }
}


