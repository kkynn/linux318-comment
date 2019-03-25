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
 * Purpose : CTC proprietary behavior HGU wrapper APIs
 *
 * Feature : Provide the wapper layer for CTC HGU application
 *
 */

#ifdef CONFIG_HGU_APP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include "voip_flash.h"



#include "ctc_wrapper.h"
#include <hal/chipdef/chip.h>
#include <hal/common/halctrl.h>
#include <dal/apollomp/dal_apollomp_switch.h>
/* RomerDriver related include */
#include "rtk_rg_struct.h"
#include "rtk_rg_define.h"

/* EPON OAM include */
#include "epon_oam_config.h"
#include "epon_oam_err.h"
#include "epon_oam_db.h"
#include "epon_oam_dbg.h"
#include "epon_oam_rx.h"

/* User specific include */
#include "ctc_oam.h"
#include "ctc_oam_var.h"

/* include CTC wrapper */
#include "ctc_hgu.h"
#include <sys_def.h>

#include "ctc_vlan.h"

#include <common/debug/rt_log.h>
#include <osal/lib.h>
#include <ioal/mem32.h>
#include <hal/chipdef/chip.h>
#include <hal/common/halctrl.h>
#include <rtk/oam.h>

voipCfgParam_t *g_pFlashCfgVoIP = NULL;
static voip_state_share_t *g_pLineStateVoIP = NULL;

static int sem_id = -1;
static int shm_id = -1;
//static int g_nShareSize = 0;

void voip_OAM_state_share_stop(void);
int voip_oam_state_init_variables();



int voip_oam_state_init_variables()
{
	if(g_pLineStateVoIP!=NULL)
		return;

	key_t key = ftok(VOIP_PATHNAME, VOIP_LINE_STATE_VER);
	if (key == -1)
	{
		fprintf(stderr, "ftok failed: %s\n", strerror(errno));
		return -1;
	}
	
	fprintf(stderr, "voip OAM line state share size = %d\n", sizeof(voip_state_share_t));	
	
	// get share memory ID (line_state)
	shm_id = shmget(key, sizeof(voip_flash_share_t), 0666 | IPC_CREAT);

	if (shm_id == -1)
	{
		fprintf(stderr, " get share memory ID shmget failed: %s\n", strerror(errno));
		goto voip_oam_state_init_failed;
	}    

    // attach share memory (line_state)
	if ((g_pLineStateVoIP = shmat(shm_id, NULL, 0)) == (void *)-1)
	{
		fprintf(stderr, "attach share memory shmat failed: %s\n", strerror(errno));
		goto voip_oam_state_init_failed;
	}


	return 0;
voip_oam_state_init_failed:
	fprintf(stderr, "voip_line_state_init_failed \n");
	voip_OAM_state_share_stop();
	return -1;	
}

void voip_OAM_state_share_stop(void)
{
	
	if (g_pLineStateVoIP != NULL)
	{
		if (shmdt(g_pLineStateVoIP) == -1)
			fprintf(stderr, "shmdt failed: %s\n", strerror(errno));
	}

	if (shm_id != -1)
	{
		if (shmctl(shm_id, IPC_RMID, 0) == -1)
			fprintf(stderr, "shmctl failed: %s\n", strerror(errno));
	}


}


/*
ctc_hgu_voip_IADPortStauts

return value:
0. registering;
1. idle; 
2. off-hook; 
3. dialing;
4. ringing; 
5. ringback tone
6. connecting; 
7. connected;
8 releasing connection; 
9. register failed; 
10. disabled;
*/

int ctc_hgu_voip_IADPortStauts(unsigned int port)
{
	int PortStatus=0;
	
	if (g_pLineStateVoIP)
	{
	
		if(g_pLineStateVoIP->VOIPLineStatus[port].voipVoiceServerStatus==REGISTER_ONGOING)
			return 0;

		if(g_pLineStateVoIP->HookState[port]==0)
			return 1;	//idle

		if((g_pLineStateVoIP->HookState[port]==1) && (g_pLineStateVoIP->VOIPLineStatus[port].voip_Port_status==PORT_IDEL))
			return 2;

		switch(g_pLineStateVoIP->VOIPLineStatus[port].voip_Port_status){
			case PORT_DIALING:
				PortStatus=3;
				break;
			case PORT_RINGING:
				PortStatus=4;
				break;
			case PORT_RINGBACKTONE:
				PortStatus=5;
				break;
			case PORT_CONNECTING:
				PortStatus=6;
				break;
			case PORT_CONNECTED:
				PortStatus=7;
				break;
			case PORT_RELEASE_CONNECTED:
				PortStatus=8;
				break;
			case PORT_DISABLE:
				PortStatus=9;
				break;
		}

	}
	return PortStatus;
}



/*
ctc_hgu_voip_iadPortServiceState()

0x00000000¡GendLocal user disable
0x00000001¡GendRemote  
0x00000002¡GendAuto MGC Fail
0x00000003¡Gnormal 

*/

int ctc_hgu_voip_iadPortServiceState(unsigned int port)
{

	int server_status=0;
	if (g_pLineStateVoIP)
	{
		switch(g_pLineStateVoIP->VOIPLineStatus[port].voipVoiceServerStatus){

			case REGISTERED:
				server_status=3;
				break;
			case FAILED_REGISTRATION_SERVER_FAIL_CODE:
				server_status=0;
				break;

			default:
				server_status=3;
		}

	}

	return server_status;
}


/*
function ctc_hgu_voip_iadPortCodecMode_get()
return value
0x00000000¡GG.711 A
0x00000001¡GG.729
0x00000002¡GG.711U
0x00000003¡GG.723
0x00000004¡GG.726
0x00000005¡GT.38


*/

int ctc_hgu_voip_iadPortCodecMode_get(unsigned int port)
{
	int codecnum=0;
	if (g_pLineStateVoIP)
	{

		switch(g_pLineStateVoIP->VOIPLineStatus[port].voipCodecUsed)//use port 0
		{

		case OMCI_VOIP_CODEC_USED_PCMA:
			codecnum=0;
			break;
		case OMCI_VOIP_CODEC_USED_PCMU:
			codecnum=2;
			break;
		case OMCI_VOIP_CODEC_USED_ITUT_G723:
			codecnum=3;
			break;
		case OMCI_VOIP_CODEC_USED_ITUT_G729:
			codecnum=1;
			break;	
		case OMCI_VOIP_CODEC_USED_ITUT_G726:
			codecnum=4;
			break;
		case OMCI_VOIP_CODEC_USED_T38:
			codecnum=5;
		default:
			codecnum=0;
			//printf("\n\n other codec case dispaly PCMU ");
			break;

		}
	}
	return codecnum;
}

#endif

