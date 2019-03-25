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
 * Purpose : Main function of OMCI Application
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI main APIs
 */

#include "app_basic.h"
#include "omci_task.h"


static OMCI_STATUS_INFO_ts stateInfo;


static void defaultOmciStatusSet(void)
{

	memset(&stateInfo,0,sizeof(OMCI_STATUS_INFO_ts));
	gUsrLogLevel = OMCI_LOG_LEVEL_ERR;
	gDrvLogLevel = OMCI_LOG_LEVEL_ERR;
#ifdef OMCI_X86
	stateInfo.devMode  = OMCI_DEV_MODE_BRIDGE;
#else
	stateInfo.devMode  = OMCI_DEV_MODE_HYBRID;
#endif
	stateInfo.dmMode  = FALSE;
	stateInfo.receiveState = 1;
	stateInfo.sn[0] = 'R';
	stateInfo.sn[1] = 'T';
	stateInfo.sn[2] = 'K';
	stateInfo.sn[3] = 'G';
	stateInfo.sn[4] = 0x11;
	stateInfo.sn[5] = 0x11;
	stateInfo.sn[6] = 0x11;
	stateInfo.sn[7] = 0x11;
	stateInfo.gLoggingActMask = DEFAULT_LOGGING_ACTMASK;
	stateInfo.logFileMode = OMCI_LOGFILE_MODE_DISABLE;
    stateInfo.veipSlotId = 0xFF;
    stateInfo.voiceVendor = VOICE_VENDOR_NONE;
    stateInfo.txc_cardhld_fe_slot_type_id = TXC_CARDHLD_FE_SLOT_TYPE_ID;
}

static void userOmciStatusSet(int cnt, char **str)
{
	int cmd=1;
	unsigned long tSn;
	while(cmd < cnt)
	{
		if(!strcmp("-s",str[cmd]))
		{
			cmd++;
			stateInfo.sn[0] = str[cmd][0];
			stateInfo.sn[1] = str[cmd][1];
			stateInfo.sn[2] = str[cmd][2];
			stateInfo.sn[3] = str[cmd][3];
			tSn = strtoul(&str[cmd][4],NULL,16);
			stateInfo.sn[4] = (tSn >> 24) & 0xff;
			stateInfo.sn[5] = (tSn >> 16) & 0xff;
			stateInfo.sn[6] = (tSn >> 8)  & 0xff;
			stateInfo.sn[7] = tSn & 0xff;
			cmd++;
		}
		else if(!strcmp("-d",str[cmd]))
		{
			cmd ++;
			if(!strcmp("err",str[cmd]))
			{
				gUsrLogLevel = OMCI_LOG_LEVEL_ERR;
				gDrvLogLevel = OMCI_LOG_LEVEL_ERR;
			}
			else if(!strcmp("warn",str[cmd]))
			{
				gUsrLogLevel = OMCI_LOG_LEVEL_WARN;
				gDrvLogLevel = OMCI_LOG_LEVEL_WARN;
			}
			else if(!strcmp("dbg",str[cmd]))
			{
				gUsrLogLevel = OMCI_LOG_LEVEL_DBG;
				gDrvLogLevel = OMCI_LOG_LEVEL_DBG;
			}
			else if(!strcmp("off",str[cmd]))
			{
				gUsrLogLevel = OMCI_LOG_LEVEL_OFF;
				gDrvLogLevel = OMCI_LOG_LEVEL_OFF;
			}
			else if(!strcmp("info",str[cmd]))
			{
				gUsrLogLevel = OMCI_LOG_LEVEL_INFO;
				gDrvLogLevel = OMCI_LOG_LEVEL_INFO;
			}
			else 
			{
				gUsrLogLevel = OMCI_LOG_LEVEL_DBG;
				gDrvLogLevel = OMCI_LOG_LEVEL_DBG;
			}
			cmd ++;
		}else
		if(!strcmp("-t",str[cmd]))
		{
			cmd++;
			if(!strcmp("bridge",str[cmd]))
			{
				stateInfo.devMode = OMCI_DEV_MODE_BRIDGE;
			}else
			if(!strcmp("router",str[cmd]))
			{
				stateInfo.devMode = OMCI_DEV_MODE_ROUTER;
			}else
			if(!strcmp("hybrid",str[cmd]))
			{
				stateInfo.devMode = OMCI_DEV_MODE_HYBRID;
			}
			cmd++;
		}else
		if(!strcmp("-cb",str[cmd]))
		{
			cmd++;
			stateInfo.customize.bridgeDP = (unsigned int)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}else
		if(!strcmp("-cr",str[cmd]))
		{
			cmd++;
			stateInfo.customize.routeDP = (unsigned int)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}else
		if(!strcmp("-cmc",str[cmd]))
		{
			cmd++;
			stateInfo.customize.multicast = (unsigned int)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}else
		if(!strcmp("-cme",str[cmd]))
		{
			cmd++;
			stateInfo.customize.me = (unsigned int)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}else if(!strcmp("-p",str[cmd]))
		{
			unsigned int i;

			cmd++;
			for(i = 0; i < 10 && i < strlen(str[cmd]); i++)
				stateInfo.gponPwd[i] = str[cmd][i];

			cmd++;
		}else if(!strcmp("-l",str[cmd]))
		{
			unsigned int i;

			cmd++;
			for(i = 0; i < 24; i++)
			{
				if(i >= strlen(str[cmd]))
					break;
				stateInfo.loidCfg.loid[i] = str[cmd][i];

			}
			cmd++;
		}else if(!strcmp("-w",str[cmd]))
		{
			unsigned int i;

			cmd++;
			for(i = 0; i < 12; i++)
			{
				if(i >= strlen(str[cmd]))
					break;
				stateInfo.loidCfg.loidPwd[i] = str[cmd][i];
			}
			cmd++;
		}else if(!strcmp("-m",str[cmd]))
		{
			cmd++;
			if(!strcmp("enable_wq",str[cmd]))
			{
				stateInfo.dmMode = 1;
			}
                   else if (!strcmp("enable_bc_mc",str[cmd]))
                   {
				stateInfo.dmMode = 2;
                   }
                   else if (!strcmp("enable_wq_bc_mc",str[cmd]))
                   {
				stateInfo.dmMode = 3;
                   }
                   else
			{
				stateInfo.dmMode = 0;
			}
			cmd++;
		}else if(!strcmp("-f",str[cmd]))
		{
			cmd++;
			if(!strcmp("raw",str[cmd]))
			{
				stateInfo.logFileMode = OMCI_LOGFILE_MODE_RAW;
			}else
			if(!strcmp("parsed",str[cmd]))
			{
				stateInfo.logFileMode = OMCI_LOGFILE_MODE_PARSED;
			}else
			if(!strcmp("both",str[cmd]))
			{
				stateInfo.logFileMode = OMCI_LOGFILE_MODE_RAW + OMCI_LOGFILE_MODE_PARSED;
			}else
			if(!strcmp("time",str[cmd]))
			{
				stateInfo.logFileMode = OMCI_LOGFILE_MODE_RAW + OMCI_LOGFILE_MODE_PARSED + OMCI_LOGFILE_MODE_WITH_TIMESTAMP;
			}else
			if (!strcmp("console",str[cmd]))
            {
                stateInfo.logFileMode = OMCI_LOGFILE_MODE_CONSOLE;
            }
            else
			{
				stateInfo.logFileMode = OMCI_LOGFILE_MODE_DISABLE;
			}
			cmd++;
			stateInfo.gLoggingActMask = (unsigned int)(strtoul(str[cmd], NULL, 10));
            if (0 == stateInfo.gLoggingActMask)
                stateInfo.gLoggingActMask = DEFAULT_LOGGING_ACTMASK;
			cmd++;
		}
        else if(!strcmp("-iot_vt", str[cmd]))
		{
			cmd++;
			stateInfo.iotVlanCfg.vlan_cfg_type = (unsigned int)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}
        else if(!strcmp("-iot_vm", str[cmd]))
		{
			cmd++;
			stateInfo.iotVlanCfg.vlan_cfg_manual_mode = (unsigned int)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}
        else if(!strcmp("-iot_vid", str[cmd]))
		{
			cmd++;
			stateInfo.iotVlanCfg.vlan_cfg_manual_vid = (unsigned int)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}
        else if(!strcmp("-iot_pri", str[cmd]))
		{
			cmd++;
			stateInfo.iotVlanCfg.vlan_cfg_manual_pri = (unsigned int)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}
        else if(!strcmp("-slot_veip", str[cmd]))
		{
			cmd++;
			stateInfo.veipSlotId = (unsigned char)(strtoul(str[cmd], NULL, 10));
			cmd++;
		}
        else if(!strcmp("-voice_vendor", str[cmd]))
		{
			cmd++;
			stateInfo.voiceVendor = (unsigned int)(strtoul(str[cmd], NULL, 10));
            if (VOICE_VENDOR_END <= stateInfo.voiceVendor)
                stateInfo.voiceVendor = VOICE_VENDOR_NONE;
			cmd++;
		}
        else
		{
			cmd++;
		}
	}
}


int main(int argc, char* argv[])
{
    GOS_ERROR_CODE ret;

    ret = OMCI_AppInit(OMCI_APPL, "OMCI");
    GOS_ASSERT(ret == GOS_OK);

	/*default setting*/
	defaultOmciStatusSet();

	/*user setting*/
	userOmciStatusSet(argc,argv);

	/*assign info to omci*/
	OMCI_AppInfoSet(&stateInfo);
	/*start omci process*/
    OMCI_AppStart((OMCI_APPL_INIT_PTR)OMCI_Init, (OMCI_APPL_MSG_HANDLER_PTR)OMCI_HandleMsg, (OMCI_APPL_DEINIT_PTR)OMCI_DeInit);

    return 0;
}
