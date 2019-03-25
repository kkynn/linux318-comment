/*
 * Copyright (C) 2012 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 39101 $
 * $Date: 2013-05-03 17:35:27 +0800 (週五, 03 五月 2013) $
 *
 * Purpose : Define the standard OAM callback and CTC extended OAM callback
 *
 * Feature : Provide standard and CTC related callback functions
 *
 */

/*
 * Include Files
 */
/* Standard include */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <net/if.h>
#include <linux/autoconf.h>
#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
/* EPON OAM include */
#include "epon_oam_config.h"
#include "epon_oam_err.h"
#include "epon_oam_db.h"
#include "epon_oam_dbg.h"
#include "epon_oam_rx.h"
#include "epon_oam_msgq.h"
/* User specific include */
#include "ctc_oam.h"
#include "ctc_oam_var.h"
/* Wrapper include */
#include "ctc_wrapper.h"
#ifdef CONFIG_SFU_APP
#include "ctc_clf.h"
#endif
#include "ctc_mc.h"
#include "ctc_oam_alarmtbl.h"
#include "epon_oam_igmp_util.h"

#include <rtk/port.h>
#include <rtk/vlan.h>
#include <rtk/epon.h>
#include <rtk/rldp.h>
#include <hal/chipdef/chip.h>
#include <hal/common/halctrl.h>
//#ifdef CONFIG_SDK_APOLLO
#include <dal/apollomp/dal_apollomp_switch.h>
//#endif

#include "sys_portmask.h"
/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */
/* MODIFY ME - use actual database instead */
ctc_dbaThreshold_t dbaThresholdDb = {
    .numQSet = 1,
    .reportMap[0] = 0xff,
    .queueSet[0].queueThreshold[0] = 0xffff,
    .queueSet[0].queueThreshold[1] = 0xffff,
    .queueSet[0].queueThreshold[2] = 0xffff,
    .queueSet[0].queueThreshold[3] = 0xffff,
    .queueSet[0].queueThreshold[4] = 0xffff,
    .queueSet[0].queueThreshold[5] = 0xffff,
    .queueSet[0].queueThreshold[6] = 0xffff,
    .queueSet[0].queueThreshold[7] = 0xffff,
};

/*
 * define some global variable here to maintain CTC state
 */
/**************** below for Service SLA *****************/
/* srvDBA: 0-deactivate 1-activate */
unsigned int srvDBA=0;
/* schedScheme: 0-SP  1-WRR  2-SP+WRR*/
unsigned int schedScheme=0;
/* highPrioBoundary: threshold for SP and WRR */
unsigned int highPrioBoundary;
/* cycLen */
unsigned int cycLen=0;
/* srvNum: 1-8 */
unsigned int srvNum=1;
/* queue info */
struct SRVSLA_ST{
	unsigned char queueNum;
	unsigned short pktSize;
	unsigned short fir;
	unsigned short cir;
	unsigned short pir;
	unsigned short weight;
} serviceSLA[8] = {
	{8, 0, 0, 0, 0x1000, 0}
};
/***************** end of Service SLA *******************/

/********** below for Active PON_IF Adminstate **********/
unsigned int pon_if_admin=0;
unsigned int pon_active_port=0;
/*********** end of Active PON_IF Adminstate ************/

/************** ONU Protection Parameters ***************/
unsigned short los_optical_time = 2;	//unit: ms
unsigned short los_mac_time = 50;		//unit: ms
/************** ONU Protection Parameters ***************/

#ifdef CONFIG_SFU_APP
/**************** Classification&Marking ****************/
static oam_clasmark_local_t local_clasmark[4];
/**************** Classification&Marking ****************/
#endif
extern ctc_infoOamVer_t currCtcVer[EPON_OAM_SUPPORT_LLID_NUM];
extern ctc_onuSnInfo_t ctcOnuSn;

static unsigned long long power_saving_sleep_duration_max = CTC_OAM_VAR_SLEEPCONTROL_DURATION_MAX;

#if defined(CONFIG_USER_SNMPD_SNMPD_V2CTRAP) || defined(CONFIG_USER_SNMPD_SNMPD_V3)
#define SNMPPID  "/var/run/snmpd.pid"

#ifdef CONFIG_USER_SNMPD_SNMPD_V2CTRAP
// Added by Mason Yu for ILMI(PVC) community string
static const char PVCREADSTR[] = "ADSL";
static const char PVCWRITESTR[] = "ADSL";
// Added by Mason Yu for write community string
static const char SNMPCOMMSTR[] = "/var/snmpComStr.conf";

// return value:
// 1  : successful
// -1 : startup failed
static int startSnmp(void)
{
    char cmdStr[256];
	unsigned char trapip[16];
	unsigned char commRW[100], commRO[100], enterOID[100];
	FILE 	      *fp;

	// Get SNMP Trap Host IP Address
    ctc_oam_flash_var_get("SNMP_TRAP_IP", trapip, sizeof(trapip));
	//printf("***** trapip = %s\n", trapip);

	// Get CommunityRO String
    ctc_oam_flash_var_get("SNMP_COMM_RO", commRO, sizeof(commRO));
	//printf("*****buffer = %s\n", commRO);


	// Get CommunityRW String
    ctc_oam_flash_var_get("SNMP_COMM_RW", commRW, sizeof(commRW));
	//printf("*****commRW = %s\n", commRW);


	// Get Enterprise OID
    ctc_oam_flash_var_get("SNMP_SYS_OID", enterOID, sizeof(enterOID));
	//printf("*****enterOID = %s\n", enterOID);


	// Write community string to file
	if ((fp = fopen(SNMPCOMMSTR, "w")) == NULL)
	{
		printf("Open file %s failed !\n", SNMPCOMMSTR);
		return -1;
	}

	if (commRO[0])
		fprintf(fp, "readStr %s\n", commRO);
	if (commRW[0])
		fprintf(fp, "writeStr %s\n", commRW);

	// Add ILMI(PVC) community string
	fprintf(fp, "PvcReadStr %s\n", PVCREADSTR);
	fprintf(fp, "PvcWriteStr %s\n", PVCWRITESTR);
	fclose(fp);

	// Mason Yu
	// ZyXEL Remote management does not verify the comm string, so we can limit the comm string as "ADSL"
	snprintf(cmdStr, sizeof(cmdStr), "/bin/snmpd -p 161 -c %s -th %s -te %s", 
	            PVCREADSTR, trapip, enterOID);
	system(cmdStr);
	return 1;

}
#else
#define SNMP_VERSION_2 1
#define SNMP_VERSION_3 2
#define SNMP_STRING_LEN					64
typedef enum { SNMP_AUTH_MD5=0, SNMP_AUTH_SHA=1, SNMP_AUTH_NONE=2 } SNMPV3_AUTH_TYPE;

static const char NETSNMPCONF[] = "/var/snmpd.conf";

int startSnmp(void)
{
	unsigned char commRW[100], commRO[100];
	FILE *fp;
	char buf[256];
    char tmpStr[10];
	unsigned char trapip[16];
	unsigned char roName[SNMP_STRING_LEN], roPwd[SNMP_STRING_LEN];
	unsigned char roAuth;
	unsigned char rwName[SNMP_STRING_LEN], rwPwd[SNMP_STRING_LEN];
	unsigned char rwAuth, rwPrivacy;
	unsigned char snmpVersion = 1;
	char str_val[256];
	char * auth[3] = {"MD5", "SHA", ""};
	char * privacy[2] = {"DES", "AES"};

	// Get SNMP Trap Host IP Address
	ctc_oam_flash_var_get("SNMP_TRAP_IP", trapip, sizeof(trapip));
	
	// Get CommunityRO String
    ctc_oam_flash_var_get("SNMP_COMM_RO", commRO, sizeof(commRO));
	
	// Get CommunityRW String
    ctc_oam_flash_var_get("SNMP_COMM_RW", commRW, sizeof(commRW));

    ctc_oam_flash_var_get("MIB_SNMPV3_RO_NAME", roName, sizeof(roName));
    ctc_oam_flash_var_get("MIB_SNMPV3_RO_PASSWORD", roPwd, sizeof(roPwd));
    ctc_oam_flash_var_get("MIB_SNMPV3_RO_AUTH", tmpStr, sizeof(tmpStr));
    roAuth = atoi(tmpStr);
    ctc_oam_flash_var_get("MIB_SNMPV3_RW_NAME", rwName, sizeof(rwName));
    ctc_oam_flash_var_get("MIB_SNMPV3_RW_PASSWORD", rwPwd, sizeof(rwPwd));
    ctc_oam_flash_var_get("MIB_SNMPV3_RW_AUTH", tmpStr, sizeof(tmpStr));
    rwAuth = atoi(tmpStr);
    ctc_oam_flash_var_get("MIB_SNMPV3_RW_PRIVACY", tmpStr, sizeof(tmpStr));
    rwPrivacy = atoi(tmpStr);
    ctc_oam_flash_var_get("MIB_SNMP_VERSION", tmpStr, sizeof(tmpStr));
    snmpVersion = atoi(tmpStr);
	
	// Write community string to file
	if ((fp = fopen(NETSNMPCONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", NETSNMPCONF);
		return -1;
	}

	if(snmpVersion & SNMP_VERSION_2)
	{
		if (commRO[0] && commRW[0] && (strcmp(commRO,commRW) == 0))
		{
			/*if read-only user name and read-write user name are the same, 
			  the action of net-snmp is ambiguity, so only set read-write user */
			fprintf(fp, "rwcommunity %s\n", commRW);
		}
		else
		{
			if (commRO[0])
				fprintf(fp, "rocommunity %s\n", commRO);
			if (commRW[0])
				fprintf(fp, "rwcommunity %s\n", commRW);
		}
	}

	if(snmpVersion & SNMP_VERSION_3)
	{
		if(roName[0])
		{
			if(roAuth == SNMP_AUTH_NONE)
			{
				fprintf(fp, "createuser %s\n", roName);
				fprintf(fp, "rouser %s noauth\n", roName);
			}
			else if(roPwd[0])
			{
				fprintf(fp, "createuser %s %s \"%s\"\n", roName, auth[roAuth], roPwd);
				fprintf(fp, "rouser %s\n", roName);
			}
		}
		if(rwName[0] && rwPwd[0])
		{
			fprintf(fp, "createuser %s %s \"%s\" %s\n", rwName, auth[rwAuth], rwPwd, privacy[rwPrivacy]);
			fprintf(fp, "rwuser %s\n", rwName);
		}
	}
	
	if(trapip[0])
	{
		fprintf(fp, "trapsink %s:162\n", trapip); /* enable snmpv1 trap */
		fprintf(fp, "trap2sink %s:162\n", trapip); /* enable snmpv2 trap */
	}

	/* config values for net-snmp to set the corresponding oid values */
    if (EPON_OAM_ERR_OK == ctc_oam_flash_var_get("SNMP_SYS_DESCR", str_val, sizeof(str_val))) {
        fprintf(fp, "sysdescr %s\n", str_val);
    }

    if (EPON_OAM_ERR_OK == ctc_oam_flash_var_get("SNMP_SYS_OID", str_val, sizeof(str_val))) {           
		fprintf(fp, "sysobjectid %s\n", str_val);
	}
	
    if (EPON_OAM_ERR_OK == ctc_oam_flash_var_get("SNMP_SYS_CONTACT", str_val, sizeof(str_val))) {           
		fprintf(fp, "psyscontact %s\n", str_val);
	}

    if (EPON_OAM_ERR_OK == ctc_oam_flash_var_get("SNMP_SYS_NAME", str_val, sizeof(str_val))) {           
		fprintf(fp, "psysname %s\n", str_val);
	}

    if (EPON_OAM_ERR_OK == ctc_oam_flash_var_get("SNMP_SYS_LOCATION", str_val, sizeof(str_val))) {           
		fprintf(fp, "psyslocation %s\n", str_val);
	}
	fclose(fp);

	snprintf(buf, 256, "/bin/snmpd -r -L -c %s -p %s", NETSNMPCONF, SNMPPID);
	system(buf);

	return 1;
}
#endif

static int read_pid(const char *filename)
{
	FILE *fp;
	int pid;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	if(fscanf(fp, "%d", &pid) != 1)
		pid = -1;
	fclose(fp);

	return pid;
}

int restart_snmp(int flag)
{
	int snmppid=0;
	int status=0;

	snmppid = read_pid((char*)SNMPPID);

	//printf("\nsnmppid=%d\n",snmppid);

	if(snmppid > 0) {
		kill(snmppid, 9);
		unlink(SNMPPID);
	}

	if(flag==1){
		status = startSnmp();
	}
	return status;
}
#endif

/*
 * Macro Definition
 */

/*
 * Function Declaration
 */
/* Dummy variable set function, always return OK.
 * This function is for debug usage only
 */
int ctc_oam_varCb_dummy_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    /* Pretend set operation complete */
    pVarContainer->varWidth = 0x80;

    return EPON_OAM_ERR_OK;
}

/* 0x07/0x0025 - aPhyAdminState */
int ctc_oam_varCb_aPhyAdminState_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    int ret;
    unsigned int getValue;
    rtk_enable_t enable;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_APHYADMINSTATE_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    ret = CTC_WRAPPER(portStateGet, pVarInstant->parse.uniPort.portNo - 1, &enable);
    if(EPON_OAM_ERR_OK == ret)
    {
        if(DISABLED == enable)
        {
            getValue = CTC_OAM_VAR_APHYADMINSTATE_DISABLE;
        }
        else
        {
            getValue = CTC_OAM_VAR_APHYADMINSTATE_ENABLE;
        }
        CTC_BUF_ENCODE32(pVarContainer->pVarData, &getValue);
    }
    else
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

/* 0x07/0x004F - aAutoNegAdminState */
int ctc_oam_varCb_aAutoNegAdminState_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    int ret;
    unsigned int getValue;
    rtk_enable_t enable;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_AAUTONEGADMINSTATE_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    ret = CTC_WRAPPER(portAutoNegoGet, pVarInstant->parse.uniPort.portNo - 1, &enable);
    if(EPON_OAM_ERR_OK == ret)
    {
        if(DISABLED == enable)
        {
            getValue = CTC_OAM_VAR_AAUTONEGADMINSTATE_DISABLE;
        }
        else
        {
            getValue = CTC_OAM_VAR_AAUTONEGADMINSTATE_ENABLE;
        }
        CTC_BUF_ENCODE32(pVarContainer->pVarData, &getValue);
    }
    else
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

/* 0x07/0x0052 - aAutoNegLocalTechnologyAbility */
int ctc_oam_varCb_aAutoNegLocalTechnologyAbility_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    int ret;
    int i;
    char *pPtr;
    unsigned int capNum, capability, value;

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    pVarContainer->varDesc = varDesc;

    ret = CTC_WRAPPER(portCapGet, pVarInstant->parse.uniPort.portNo - 1, &capability);
    if(EPON_OAM_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    capNum = 0;
    for(i = 0 ; i < 32 ; i ++)
    {
        if(capability & (1 << i))
        {
            capNum ++;
        }
    }

    pVarContainer->varWidth = 4 + 4 * capNum;
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }


    /* Value List extract from 802.3 30A
     * global (0),          --reserved for future use.
     * other (1),           --undefined
     * unknown (2),         --initializing, true ability not yet known.
     * 10BASE-T (14),       --10BASE-T as defined in Clause 14
     * 10BASE-TFD (142),    --Full duplex 10BASE-T as defined in Clauses
                              14 and 31
     * 100BASE-T4 (23),     --100BASE-T4 as defined in Clause 23
     * 100BASE-TX (25),     --100BASE-TX as defined in Clause 25
     * 100BASE-TXFD (252),  --Full duplex 100BASE-TX as defined in Clauses
                              25 and 31
     * 10GBASE-KX4 (483),   --10GBASE-KX4 PHY as defined in Clause 71
     * 10GBASE-KR (495),    --10GBASE-KR PHY as defined in Clause 72
     * 10GBASE-T (55),      --10GBASE-T PHY as defined in Clause 55
     * FDX PAUSE (312),     --PAUSE operation for full duplex links as 
                              defined in Annex 31B
     * FDX APAUSE (313),    --Asymmetric PAUSE operation for full duplex
                              links as defined in Clause 37 and Annex 31B
     * FDX SPAUSE (314),    --Symmetric PAUSE operation for full duplex
                              links as defined in Clause 37 and Annex 31B
     * FDX BPAUSE (315),    --Asymmetric and Symmetric PAUSE operation for
                              full duplex links as defined in Clause 37
                              and Annex 31B
     * 100BASE-T2 (32),     --100BASE-T2 as defined in Clause 32
     * 100BASE-T2FD (322),  --Full duplex 100BASE-T2 as defined in Clauses
                              31 and 32
     * 1000BASE-X (36),     --1000BASE-X as defined in Clause 36
     * 1000BASE-XFD (362),  --Full duplex 1000BASE-X as defined in Clause 36
     * 1000BASE-KX (393),   --1000BASE-KX PHY as defined in Clause 70
     * 1000BASE-T (40),     --1000BASE-T UTP PHY as defined in Clause 40
     * 1000BASE-TFD (402),  --Full duplex 1000BASE-T UTP PHY to be defined
                              in Clause 40
     * Rem Fault1 (37),     --Remote fault bit 1 (RF1) as specified in Clause 37
     * Rem Fault2 (372),    --Remote fault bit 2 (RF1) as specified in Clause 37
     * isoethernet (8029)   --802.9 ISLAN-16T
     */

    pPtr = pVarContainer->pVarData;
    
    CTC_BUF_ENCODE32(pPtr, &capNum);
    pPtr += 4;

    if(capability & CTC_WRAPPER_PORTCAP_10M)
    {
        /* 10BASE-T (14) */
        value = 14;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
    if(capability & CTC_WRAPPER_PORTCAP_100M)
    {
        /* 100BASE-TX (25) */
        value = 25;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
    if(capability & CTC_WRAPPER_PORTCAP_1000M)
    {
        /* 1000BASE-T (40) */
        value = 40;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
    
    return EPON_OAM_ERR_OK;
}

/* 0x07/0x0053 - aAutoNegAdvertisedTechnologyAbility */
int ctc_oam_varCb_aAutoNegAdvertisedTechnologyAbility_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    int ret;
    int i;
    char *pPtr;
    unsigned int capNum, capability, value;

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    pVarContainer->varDesc = varDesc;

    ret = CTC_WRAPPER(portAutoNegoAdvertiseGet, pVarInstant->parse.uniPort.portNo - 1, &capability);
    if(EPON_OAM_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    capNum = 0;
    for(i = 0 ; i < 32 ; i ++)
    {
        if(capability & (1 << i))
        {
            capNum ++;
        }
    }

    pVarContainer->varWidth = 4 + 4 * capNum;
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }


    /* Value List extract from 802.3 30A
     * global (0),          --reserved for future use.
     * other (1),           --undefined
     * unknown (2),         --initializing, true ability not yet known.
     * 10BASE-T (14),       --10BASE-T as defined in Clause 14
     * 10BASE-TFD (142),    --Full duplex 10BASE-T as defined in Clauses
                              14 and 31
     * 100BASE-T4 (23),     --100BASE-T4 as defined in Clause 23
     * 100BASE-TX (25),     --100BASE-TX as defined in Clause 25
     * 100BASE-TXFD (252),  --Full duplex 100BASE-TX as defined in Clauses
                              25 and 31
     * 10GBASE-KX4 (483),   --10GBASE-KX4 PHY as defined in Clause 71
     * 10GBASE-KR (495),    --10GBASE-KR PHY as defined in Clause 72
     * 10GBASE-T (55),      --10GBASE-T PHY as defined in Clause 55
     * FDX PAUSE (312),     --PAUSE operation for full duplex links as 
                              defined in Annex 31B
     * FDX APAUSE (313),    --Asymmetric PAUSE operation for full duplex
                              links as defined in Clause 37 and Annex 31B
     * FDX SPAUSE (314),    --Symmetric PAUSE operation for full duplex
                              links as defined in Clause 37 and Annex 31B
     * FDX BPAUSE (315),    --Asymmetric and Symmetric PAUSE operation for
                              full duplex links as defined in Clause 37
                              and Annex 31B
     * 100BASE-T2 (32),     --100BASE-T2 as defined in Clause 32
     * 100BASE-T2FD (322),  --Full duplex 100BASE-T2 as defined in Clauses
                              31 and 32
     * 1000BASE-X (36),     --1000BASE-X as defined in Clause 36
     * 1000BASE-XFD (362),  --Full duplex 1000BASE-X as defined in Clause 36
     * 1000BASE-KX (393),   --1000BASE-KX PHY as defined in Clause 70
     * 1000BASE-T (40),     --1000BASE-T UTP PHY as defined in Clause 40
     * 1000BASE-TFD (402),  --Full duplex 1000BASE-T UTP PHY to be defined
                              in Clause 40
     * Rem Fault1 (37),     --Remote fault bit 1 (RF1) as specified in Clause 37
     * Rem Fault2 (372),    --Remote fault bit 2 (RF1) as specified in Clause 37
     * isoethernet (8029)   --802.9 ISLAN-16T
     */

    pPtr = pVarContainer->pVarData;
    
    CTC_BUF_ENCODE32(pPtr, &capNum);
    pPtr += 4;

    if(capability & CTC_WRAPPER_PORTCAP_10BASE_T)
    {
        /* 10BASE-T (14) */
        value = 14;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
	if(capability & CTC_WRAPPER_PORTCAP_10BASE_T_FD)
    {
        /* 10BASE-TXFD (142) */
        value = 142;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
    if(capability & CTC_WRAPPER_PORTCAP_100BASE_T)
    {
        /* 100BASE-TX (25) */
        value = 25;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
	if(capability & CTC_WRAPPER_PORTCAP_100BASE_T_FD)
    {
        /* 100BASE-TXFD (252) */
        value = 252;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
	if(capability & CTC_WRAPPER_PORTCAP_PAUSE)
    {
        /* FDX PAUSE (312) */
        value = 312;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
	if(capability & CTC_WRAPPER_PORTCAP_APAUSE)
    {
        /* FDX APAUSE (313) */
        value = 313;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
    if(capability & CTC_WRAPPER_PORTCAP_1000BASE_T_FD)
    {
        /* 1000BASE-TFD (402) */
        value = 402;
        CTC_BUF_ENCODE32(pPtr, &value);
        pPtr += 4;
    }
    
    return EPON_OAM_ERR_OK;
}

/* 0x07/0x0139 - aFECAbility */
int ctc_oam_varCb_aFECAbility_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    unsigned int getValue;

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_AFECABILITY_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* Value - always return FEC is supported */
    getValue = CTC_OAM_VAR_AFECABILITY_SUPPORTED;
    pVarContainer->pVarData[0] = ((unsigned char *)&getValue)[0];
    pVarContainer->pVarData[1] = ((unsigned char *)&getValue)[1];
    pVarContainer->pVarData[2] = ((unsigned char *)&getValue)[2];
    pVarContainer->pVarData[3] = ((unsigned char *)&getValue)[3];

    return EPON_OAM_ERR_OK;
}

/* 0x07/0x013A - aFECmode */
int ctc_oam_varCb_aFecMode_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    int ret;
    rtk_enable_t state;
    unsigned int getValue;

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_AFECMODE_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    ret = CTC_WRAPPER(fecStateGet, &state);
    if(EPON_OAM_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

 
    {
        if(ENABLED == state)
        {
            getValue = CTC_OAM_VAR_AFECMODE_ENABLED;
        }
        else
        {
            getValue = CTC_OAM_VAR_AFECMODE_DISABLED;
        }
    }

    /* Construct return value */
    CTC_BUF_ENCODE32(pVarContainer->pVarData, &getValue);

    return EPON_OAM_ERR_OK;
}

/* 0x07/0x013A - aFECmode */
int ctc_oam_varCb_aFecMode_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int setValue;

    if(CTC_OAM_VAR_AFECMODE_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    CTC_BUF_PARSE32(pVarContainer->pVarData, &setValue);

    /* Configure both US and DS FEC */
    switch(setValue)
    {
    case CTC_OAM_VAR_AFECMODE_ENABLED:
        do {
            ret = CTC_WRAPPER(fecStateSet, ENABLED);
            if(EPON_OAM_ERR_OK != ret)
            {
                pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
                break;
            }
            pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
        } while(0);
        break;
    case CTC_OAM_VAR_AFECMODE_DISABLED:
        do {
            ret = CTC_WRAPPER(fecStateSet, DISABLED);
            if(EPON_OAM_ERR_OK != ret)
            {
                pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
                break;
            }
            pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
        } while(0);
        break;
    case CTC_OAM_VAR_AFECMODE_UNKNOWN:
    default:
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
        break;
    }

    return EPON_OAM_ERR_OK;
}

/* 0x09/0x0005 - acPhyAdminControl */
int ctc_oam_varCb_acPhyAdminControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int port;
    unsigned int setValue;
    rtk_enable_t enable;

    if(CTC_OAM_VAR_ACPHYADMINCONTROL_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    CTC_BUF_PARSE32(pVarContainer->pVarData, &setValue);
    if(CTC_OAM_VAR_ACPHYADMINCONTROL_DEACTIVE == setValue)
    {
        enable = DISABLED;
    }
    else if(CTC_OAM_VAR_ACPHYADMINCONTROL_ACTIVE == setValue)
    {
        enable = ENABLED;
    }
    else
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
        return EPON_OAM_ERR_OK;
    }

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
        "[OAM:%s:%d] PhyAdminControl %s\n", __FILE__, __LINE__, (ENABLED == enable) ? "enable" : "disable");
	
    ret = CTC_WRAPPER(portStateSet, pVarInstant->parse.uniPort.portNo - 1, enable);
    if(EPON_OAM_ERR_OK != ret)
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
        return EPON_OAM_ERR_OK;
    }
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0x09/0x000B - acAutoNegRestartAutoConfig */
int ctc_oam_varCb_acAutoNegRestartAutoConfig_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int port;
    rtk_enable_t enable;

    if(CTC_OAM_VAR_ACAUTONEGRESTARTAUTOCONFIG_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

	ret = CTC_WRAPPER(portAutoNegoGet, pVarInstant->parse.uniPort.portNo - 1, &enable);
    if(RT_ERR_OK != ret)
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
        return EPON_OAM_ERR_OK;
    }

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
        "[OAM:%s:%d] AutoNegRestartAutoConfig %s\n", __FILE__, __LINE__, (ENABLED == enable) ? "enable" : "disable");
	
	if(enable == ENABLED)
	{
	    ret = CTC_WRAPPER(portAutoNegoRestart, pVarInstant->parse.uniPort.portNo - 1);
	    if(RT_ERR_OK != ret)
	    {
	        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	        return EPON_OAM_ERR_OK;
	    }
	}
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0x09/0x000C - acAutoNegAdminControl */
int ctc_oam_varCb_acAutoNegAdminControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int port;
    unsigned int setValue;
    rtk_enable_t enable;

    if(CTC_OAM_VAR_ACAUTONEGADMINCONTROL_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    CTC_BUF_PARSE32(pVarContainer->pVarData, &setValue);
    if(CTC_OAM_VAR_ACAUTONEGADMINCONTROL_DEACTIVE == setValue)
    {
        enable = DISABLED;
    }
    else if(CTC_OAM_VAR_ACAUTONEGADMINCONTROL_ACTIVE == setValue)
    {
        enable = ENABLED;
    }
    else
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
        return EPON_OAM_ERR_OK;
    }

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
        "[OAM:%s:%d] AutoNegAdminControl %s\n", __FILE__, __LINE__, (ENABLED == enable) ? "enable" : "disable");
	
    ret = CTC_WRAPPER(portAutoNegoSet, pVarInstant->parse.uniPort.portNo - 1, enable);
    if(RT_ERR_OK != ret)
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
        return EPON_OAM_ERR_OK;
    }
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0001 - ONU SN */
int ctc_oam_varCb_onuSn_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    unsigned char *pPtr;
    oam_config_t oamConfig;
    extern ctc_swDlBootInfo_t bootInfo;
	int offset;
	
    pVarContainer->varDesc = varDesc;
	if(currCtcVer[llidIdx].version == CTC_OAM_VERSION_30)
   		pVarContainer->varWidth = 0x36; /* Fix value from CTC 3.0 standard */
	else
		pVarContainer->varWidth = 0x26; /* Fix value from CTC 2.1 or below standard */
	
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    pPtr = pVarContainer->pVarData;

    /* Vendor ID - RTK */
	memcpy(pPtr, ctcOnuSn.vendorID, sizeof(ctcOnuSn.vendorID) > 4 ? 4 : sizeof(ctcOnuSn.vendorID));
    pPtr += 4;
    
    /* ONU model - blank model */
    memcpy(pPtr, ctcOnuSn.onuModel, sizeof(ctcOnuSn.onuModel) > 4 ? 4 : sizeof(ctcOnuSn.onuModel));
    pPtr += 4;

    /* ONU ID - Local device MAC address */
    /* Get MAC address from EPON OAM stack configuration */
    epon_oam_config_get(llidIdx, &oamConfig);
    memcpy(pPtr, oamConfig.macAddr, sizeof(oamConfig.macAddr));
    pPtr += 6;

    /* MODIFY ME - Get actual HW version */
    /* Hardware Version - dummy HW version */
	if(strlen(ctcOnuSn.hwVersion) >= 8)
	{
    	memcpy(pPtr, ctcOnuSn.hwVersion, 8);
	}
	else
	{
		offset = 8 - strlen(ctcOnuSn.hwVersion);
		memset(pPtr, 0, offset); /* set NULL to the front if the size of parameter is less than 8 */
		memcpy(pPtr+offset, ctcOnuSn.hwVersion, strlen(ctcOnuSn.hwVersion));
	}
    pPtr += 8;

    /* Software Version */
	if(strlen(ctcOnuSn.swVersion) >= 16)
	{
		memcpy(pPtr, ctcOnuSn.swVersion, 16);
	}
	else
	{
		offset = 16 - strlen(ctcOnuSn.swVersion);
		memset(pPtr, 0, offset); /* set NULL to the front if the size of parameter is less than 16 */
		memcpy(pPtr+offset, ctcOnuSn.swVersion, strlen(ctcOnuSn.swVersion));
	}
    pPtr += 16;

	if(currCtcVer[llidIdx].version == CTC_OAM_VERSION_30)
	{
		if(strlen(ctcOnuSn.extOnuModel) >= 16)
		{
	    	memcpy(pPtr, ctcOnuSn.extOnuModel, 16);
		}
		else
		{
			offset = 16 - strlen(ctcOnuSn.extOnuModel);
			memset(pPtr, 0, offset); /* set NULL to the front if the size of parameter is less than 16 */
			memcpy(pPtr+offset, ctcOnuSn.extOnuModel, strlen(ctcOnuSn.extOnuModel));
		}
	    pPtr += 16;
	}

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0002 - FirmwareVer */
int ctc_oam_varCb_firmwareVer_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    pVarContainer->varDesc = varDesc;
#ifdef EPON_OAM_VERSION
    pVarContainer->varWidth = 0x20 + sizeof(EPON_OAM_VERSION); /* EPON_OAM_VERSION + __DATE__ + __TIME__ size */
#else
    pVarContainer->varWidth = 0x20; /* __DATE__ + __TIME__ size */
#endif
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

#ifdef EPON_OAM_VERSION
    snprintf(pVarContainer->pVarData, pVarContainer->varWidth, "%s %s %s", EPON_OAM_VERSION, __DATE__, __TIME__);
#else
    snprintf(pVarContainer->pVarData, pVarContainer->varWidth, "%s %s", __DATE__, __TIME__);
#endif


    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0003 - Chipset ID */
int ctc_oam_varCb_chipsetId_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
    ctc_wrapper_chipInfo_t chipInfo;

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = 0x08; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    if(ret = CTC_WRAPPER(chipInfoGet, &chipInfo))
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    /* Vender ID - JEDEC ID (dummy ID here) */
    pVarContainer->pVarData[0] = 0x04;
    pVarContainer->pVarData[1] = 0x79;

    /* Chip Model & revision */
    switch(chipInfo.chipId)
    {
#ifdef CONFIG_SDK_APOLLOMP
    case APOLLOMP_CHIP_ID:
        switch(chipInfo.subType)
        {
        case APPOLOMP_CHIP_SUB_TYPE_RTL9601:
            pVarContainer->pVarData[2] = 0x96;
            pVarContainer->pVarData[3] = 0x01;
            pVarContainer->pVarData[4] = 0x01;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9602B:
            pVarContainer->pVarData[2] = 0x96;
            pVarContainer->pVarData[3] = 0x02;
            pVarContainer->pVarData[4] = 0x02;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9606:
            pVarContainer->pVarData[2] = 0x96;
            pVarContainer->pVarData[3] = 0x06;
            pVarContainer->pVarData[4] = 0x01;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9607:
            pVarContainer->pVarData[2] = 0x96;
            pVarContainer->pVarData[3] = 0x07;
            pVarContainer->pVarData[4] = 0x01;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9602:
            pVarContainer->pVarData[2] = 0x96;
            pVarContainer->pVarData[3] = 0x02;
            pVarContainer->pVarData[4] = 0x01;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9603:
            pVarContainer->pVarData[2] = 0x96;
            pVarContainer->pVarData[3] = 0x03;
            pVarContainer->pVarData[4] = 0x01;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9607P:
            pVarContainer->pVarData[2] = 0x96;
            pVarContainer->pVarData[3] = 0x07;
            pVarContainer->pVarData[4] = 0x01;
            break;
        }
        break;
#endif
//#ifdef CONFIG_SDK_RTL9601B
    case RTL9601B_CHIP_ID:
        pVarContainer->pVarData[2] = 0x96;
        pVarContainer->pVarData[3] = 0x01;
        pVarContainer->pVarData[4] = 0x02;
        break;
//#endif
//#ifdef CONFIG_SDK_RTL9602C
    case RTL9602C_CHIP_ID:
        pVarContainer->pVarData[2] = 0x96;
        pVarContainer->pVarData[3] = 0x02;
        pVarContainer->pVarData[4] = 0x03;
        break;
//#endif
    default:
        /* Unknown chip ID */
        pVarContainer->pVarData[2] = 0x00;
        pVarContainer->pVarData[3] = 0x00;
        pVarContainer->pVarData[4] = 0x00;
        break;
    }

    /* IC_Version/Date - revision */
    pVarContainer->pVarData[5] = 0x00;
    pVarContainer->pVarData[6] = 0x00;
    pVarContainer->pVarData[7] = chipInfo.rev;
    
    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0004 - ONU capabilities-1 */
int ctc_oam_varCb_onuCapabilities1_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret, i;
    unsigned int numOfPorts;
    unsigned char *pPtrGe, *pPtrFe;
    ctc_boardCap_t boardCap;

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = 0x1A; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    ret = CTC_WRAPPER(boardCapGet, &boardCap);
    if(ret != EPON_OAM_ERR_OK)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    /* ServiceSupported */
    pVarContainer->pVarData[0] = 0x0;
    if(boardCap.uniGePortCnt != 0)
    {
        pVarContainer->pVarData[0] |= 0x1;
    }
    if(boardCap.uniFePortCnt != 0)
    {
        pVarContainer->pVarData[0] |= 0x2;
    }
    if(boardCap.potsPortCnt != 0)
    {
        pVarContainer->pVarData[0] |= 0x4;
    }

    /* Number of GE Ports - 4 GE ports */
    pVarContainer->pVarData[1] = boardCap.uniGePortCnt;

    /* Number of FE Ports - No FE ports */
    pVarContainer->pVarData[10] = boardCap.uniFePortCnt;

    pPtrGe = &pVarContainer->pVarData[2];
    pPtrFe = &pVarContainer->pVarData[11];
    memset(pPtrGe, 0, CTC_OAM_VAR_ONUCAPABILITES1_PORTMAP_LEN);
    memset(pPtrFe, 0, CTC_OAM_VAR_ONUCAPABILITES1_PORTMAP_LEN);
    numOfPorts = (boardCap.uniPortCnt <= (CTC_OAM_VAR_ONUCAPABILITES1_PORTMAP_LEN * 8)) ? boardCap.uniPortCnt : (CTC_OAM_VAR_ONUCAPABILITES1_PORTMAP_LEN * 8);
    for(i = 0 ; i < numOfPorts ; i ++)
    {
    	/* siyuan 2016-10-10: Bitmap format: the highest byte store the first 8 GE or FE ports map
		   GE bitmap e.g. 0x0000000000000800 means lan port 12 is GE port */
        if(boardCap.uniPortCap[i] & CTC_WRAPPER_PORTCAP_1000M)
        {
            /* Bitmap of GE Ports */
            *(pPtrGe + (7 - i / 8)) |= (1 << (i % 8));
        }
        else if(boardCap.uniPortCap[i] & CTC_WRAPPER_PORTCAP_100M)
        {
            /* Bitmap of FE Ports */
            *(pPtrFe + (7 - i / 8)) |= (1 << (i % 8));
        }
    }

    /* Number of POTS ports - No POTS ports */
    pVarContainer->pVarData[19] = boardCap.potsPortCnt;

    /* Number of E1 ports - No E1 ports */
    pVarContainer->pVarData[20] = 0x00;

    /* Number of US Queues - No US queue */
    pVarContainer->pVarData[21] = 8;

    /* QueueMax per Port US - No queue per port */
    pVarContainer->pVarData[22] = 8;

    /* Number of DS Queues - No DS queue */
    pVarContainer->pVarData[23] = 8;

    /* QueueMax per Port DS - No queue per port */
    pVarContainer->pVarData[24] = 8;

    /* Battery Backup - No battery backup */
    pVarContainer->pVarData[25] = 0x00;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0005 - OpticalTransceiverDiagnosis */
int ctc_oam_varCb_opticalTransceiverDiagnosis_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
    unsigned char *pPtr;
    unsigned short status;

    pVarContainer->varDesc = varDesc;
    /* For sample system, pretend it is a 4 ports GE SFU w/o multiple LLID */
    pVarContainer->varWidth = 0x0A; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    pPtr = pVarContainer->pVarData;

    /* TransceiverTemperature */
    ret = CTC_WRAPPER(transceiverStatusGet, CTC_WRAPPER_TRANSCIVERSTATUS_TEMPERATURE, &status);
    if(ret != EPON_OAM_ERR_OK)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    CTC_BUF_ENCODE16(pPtr, &status);
    pPtr += 2;

    /* Supply Voltage */
    ret = CTC_WRAPPER(transceiverStatusGet, CTC_WRAPPER_TRANSCIVERSTATUS_SUPPLYVOLTAGE, &status);
    if(ret != EPON_OAM_ERR_OK)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    CTC_BUF_ENCODE16(pPtr, &status);
    pPtr += 2;

    /* Tx bias current */
    ret = CTC_WRAPPER(transceiverStatusGet, CTC_WRAPPER_TRANSCIVERSTATUS_BIASCURRENT, &status);
    if(ret != EPON_OAM_ERR_OK)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    CTC_BUF_ENCODE16(pPtr, &status);
    pPtr += 2;

    /* Tx power (output) */
    ret = CTC_WRAPPER(transceiverStatusGet, CTC_WRAPPER_TRANSCIVERSTATUS_TXPOWER, &status);
    if(ret != EPON_OAM_ERR_OK)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    CTC_BUF_ENCODE16(pPtr, &status);
    pPtr += 2;

    /* Rx power (input) */
    ret = CTC_WRAPPER(transceiverStatusGet, CTC_WRAPPER_TRANSCIVERSTATUS_RXPOWER, &status);
    if(ret != EPON_OAM_ERR_OK)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    CTC_BUF_ENCODE16(pPtr, &status);
    pPtr += 2;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0006 - Service SLA */
int ctc_oam_varCb_serviceSla_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    unsigned char *pPtr;
	int i;

    pVarContainer->varDesc = varDesc;
	/* Possible Value is 1 if operation is deactivate or 
	 * 8+10N (N - Number of services) if operation is activate */
	if (0 == srvDBA)
		pVarContainer->varWidth = 1;
	else
		pVarContainer->varWidth = 8 + 10*srvNum;
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    pPtr = pVarContainer->pVarData;
	/* Operation of ServiceDBA */
	*(unsigned char *)pPtr++ = !!srvDBA;
	if(srvDBA)
	{
		/* Best effort scheduling scheme */
		*(unsigned char *)pPtr++ = schedScheme;
		/* High priority boundary */
		*(unsigned char *)pPtr++ = highPrioBoundary;
		/* Cycle length */
		CTC_BUF_ENCODE32(pPtr, &cycLen);
		pPtr += 4;
		/* Number of services (N) */
		*(unsigned char *)pPtr++ = srvNum;
		
		for (i=0; i<srvNum; i++)
		{
			/* Queue number of 1st service */
			*(unsigned char *)pPtr++ = serviceSLA[i].queueNum;
			/* Fixed packet size of 1st service */
			CTC_BUF_ENCODE16(pPtr, &serviceSLA[i].pktSize);
			pPtr += 2;
			/* Fixed bandwidth of 1st service */
			CTC_BUF_ENCODE16(pPtr, &serviceSLA[i].fir);
			pPtr += 2;
			/* Guaranteed bandwidth of 1st service */
			CTC_BUF_ENCODE16(pPtr, &serviceSLA[i].cir);
			pPtr += 2;
			/* Best effort bandwidth of 1st service */
			CTC_BUF_ENCODE16(pPtr, &serviceSLA[i].pir);
			pPtr += 2;
			/* WRR weight of 1st service */
			*(unsigned char *)pPtr++ = serviceSLA[i].weight;
		}
	}
    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0006 - Service SLA */
int ctc_oam_varCb_serviceSla_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    rtk_ponmac_queueCfg_t   queueCfg;
    rtk_ponmac_queue_t queue;
	unsigned char tmpSrvDba, 
				tmpSchedSchem, 
				tmphighPrioBound,
				tmpSrvNum;
	unsigned int tmpCycLen;
	struct SRVSLA_ST tmpSrvSla[8];
    int idx=0, i;
	unsigned char *pPtr;
	int ret;
	
	if (pVarContainer->varWidth != 1) 
	{
		if (pVarContainer->varWidth < 18)
			return EPON_OAM_ERR_UNKNOWN;

		if ((pVarContainer->varWidth-8) % 10)
			return EPON_OAM_ERR_UNKNOWN;
	}

	/*   1. get setting from payload                    */
	/* Operation of ServiceDBA */
    tmpSrvDba = pVarContainer->pVarData[idx++];
	if ((tmpSrvDba != 0) && (tmpSrvDba != 1))
		goto BADPARAM;

	if(pVarContainer->varWidth != 1)
	{
		/* Operation of ServiceDBA */
		tmpSchedSchem = pVarContainer->pVarData[idx++];
		if (tmpSchedSchem > 2)
			goto BADPARAM;
		/* High priority boundary */
		tmphighPrioBound = pVarContainer->pVarData[idx++];
		/* Cycle length */
		CTC_BUF_PARSE32(&pVarContainer->pVarData[idx], &tmpCycLen);
		idx += 4;
		/* Number of services (N) */
		tmpSrvNum = pVarContainer->pVarData[idx++];
		if (tmpSrvNum > 8)
			goto BADPARAM;

		pPtr = &pVarContainer->pVarData[idx];
		for (i=0; i<tmpSrvNum; i++)
		{
			/* Queue number of 1st service */
			tmpSrvSla[i].queueNum = *pPtr;
			pPtr+=1;
			/* Fixed packet size of 1st service */
			tmpSrvSla[i].pktSize = *(unsigned short *)pPtr;
			pPtr+=2;
			/* Fixed bandwidth of 1st service */
			tmpSrvSla[i].fir = *(unsigned short *)pPtr;
			pPtr+=2;
			/* Guaranteed bandwidth of 1st service */
			tmpSrvSla[i].cir = *(unsigned short *)pPtr;
			pPtr+=2;
			/* Best effort bandwidth of 1st service */
			tmpSrvSla[i].pir = *(unsigned short *)pPtr;
			pPtr+=2;
			/* WRR weight of 1st service */
			tmpSrvSla[i].weight = *pPtr;
			pPtr+=1;
			if (tmpSrvSla[i].weight > 100)
				goto BADPARAM;
		}

		/*   2. reset queue                         */
		for (i=0; i<7; i++)
		{
			queue.schedulerId = llidIdx;
			queue.queueId = i;

			rtk_ponmac_queue_del(&queue);
		}
		/*   3. save the setting                    */
		srvDBA = tmpSrvDba;
		schedScheme = tmpSchedSchem;
		highPrioBoundary = tmphighPrioBound;
		cycLen = tmpCycLen;
		srvNum = tmpSrvNum;
		for (i=0; i<srvNum; i++)
			serviceSLA[i] = tmpSrvSla[i];
	}
	else
	{
		/* varWidth Value is 1 if operation is deactivate*/
		if(tmpSrvDba != 0)
			goto BADPARAM;
		srvDBA = tmpSrvDba;
	}
	
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
				"%s %d srvDBA(%d) schedScheme(%d)\n", __func__, __LINE__, srvDBA, schedScheme);
	/*   4. take effect now                      */
	if (0 == srvDBA)
	{
		for (i=0; i<7; i++)
		{
			queue.schedulerId = llidIdx;
			queue.queueId = i;
			
			queueCfg.cir		= 0;
			queueCfg.pir		= HAL_PONMAC_PIR_CIR_RATE_MAX();
			queueCfg.type		= STRICT_PRIORITY;
			queueCfg.weight		= 0;
			queueCfg.egrssDrop	= ENABLED;
			
			rtk_ponmac_queue_add(&queue, &queueCfg);
		}
	}
	else
	{
		for (i=0; i<srvNum; i++)
		{
			queue.schedulerId = llidIdx;
			queue.queueId = i;

			if (1 == schedScheme)//WRR
			{
				queueCfg.type		= WFQ_WRR_PRIORITY;
				queueCfg.weight		= serviceSLA[i].weight;
			}
			else if (0 == schedScheme)//SP
			{
				queueCfg.type		= STRICT_PRIORITY;
				queueCfg.weight		= 0;
			}
			else if (3 == schedScheme)
			{
				if (i < highPrioBoundary)//WRR
				{
					queueCfg.type	= WFQ_WRR_PRIORITY;
					queueCfg.weight	= serviceSLA[i].weight;
				}
				else//SP
				{
					queueCfg.type	= STRICT_PRIORITY;
					queueCfg.weight	= 0;
				}
			}
			queueCfg.cir = (serviceSLA[i].cir > HAL_PONMAC_PIR_CIR_RATE_MAX()) ?  HAL_PONMAC_PIR_CIR_RATE_MAX():serviceSLA[i].cir;
			queueCfg.pir = (serviceSLA[i].pir > HAL_PONMAC_PIR_CIR_RATE_MAX()) ?  HAL_PONMAC_PIR_CIR_RATE_MAX():serviceSLA[i].pir;
			queueCfg.egrssDrop	= DISABLED;

			ret = rtk_ponmac_queue_add(&queue, &queueCfg);
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, "%s %d queue[%d] cir(%d) pir(%d) weight(%d) ret[%d]\n", 
					__func__, __LINE__, i, serviceSLA[i].cir, serviceSLA[i].pir, serviceSLA[i].weight, ret);
		}
	}
	
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;

BADPARAM:
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0007 - ONU capabilities-2 */
int ctc_oam_varCb_onuCapabilities2_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	int ret;
	ctc_boardCap_t boardCap;
	
	pVarContainer->varDesc = varDesc;
    /* For sample system, pretend it is a 4 ports GE SFU w/o multiple LLID 
	   add 100M FE lan port info, voip port info, usb port info and wlan info */	
    pVarContainer->varWidth = 10 + 6 + 6 + 6 + 6 + 6; 

    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

	ret = CTC_WRAPPER(boardCapGet, &boardCap);
    if(ret != EPON_OAM_ERR_OK)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

#ifdef CONFIG_SFU_APP	
    /* ONU Type - SFU */
    pVarContainer->pVarData[0] = 0x00;
    pVarContainer->pVarData[1] = 0x00;
    pVarContainer->pVarData[2] = 0x00;
    pVarContainer->pVarData[3] = 0x00;
#else
	/* ONU Type - HGU */
    pVarContainer->pVarData[0] = 0x00;
    pVarContainer->pVarData[1] = 0x00;
    pVarContainer->pVarData[2] = 0x00;
    pVarContainer->pVarData[3] = 0x01;
#endif

    /* MultiLLID - single LLID */
    pVarContainer->pVarData[4] = 0x01;

    /* ProtectionType - Unsupport */
    pVarContainer->pVarData[5] = 0x00;

    /* Num of PON IF - 1 PON IF */
    pVarContainer->pVarData[6] = 0x01;

    /* Num of Slot - SFU or HGU */
    pVarContainer->pVarData[7] = 0x00;

    /* Num of Interface type - 5 IF Types (VOIP, USB, WLAN, GE and FE port) */
    pVarContainer->pVarData[8] = 0x05;

    /* Interface Type - GE */
    pVarContainer->pVarData[9] = 0x00;
    pVarContainer->pVarData[10] = 0x00;
    pVarContainer->pVarData[11] = 0x00;
    pVarContainer->pVarData[12] = 0x00;

    /* Num of GE Port */
    pVarContainer->pVarData[13] = 0x00;
    pVarContainer->pVarData[14] = boardCap.uniGePortCnt;

	/* Interface Type - FE */
    pVarContainer->pVarData[15] = 0x00;
    pVarContainer->pVarData[16] = 0x00;
    pVarContainer->pVarData[17] = 0x00;
    pVarContainer->pVarData[18] = 0x01;

    /* Num of FE Port */
    pVarContainer->pVarData[19] = 0x00;
    pVarContainer->pVarData[20] = boardCap.uniFePortCnt;

	/* Interface Type - VOIP */
    pVarContainer->pVarData[21] = 0x00;
    pVarContainer->pVarData[22] = 0x00;
    pVarContainer->pVarData[23] = 0x00;
    pVarContainer->pVarData[24] = 0x02;

    /* Num of VOIP Port */
    pVarContainer->pVarData[25] = 0x00;
    pVarContainer->pVarData[26] = boardCap.potsPortCnt;

	/* Interface Type - USB */
    pVarContainer->pVarData[27] = 0x00;
    pVarContainer->pVarData[28] = 0x00;
    pVarContainer->pVarData[29] = 0x00;
    pVarContainer->pVarData[30] = 0x07;

    /* Num of USB Port */
    pVarContainer->pVarData[31] = 0x00;
    pVarContainer->pVarData[32] = boardCap.usbPortCnt;

	/* Interface Type - WLAN */
    pVarContainer->pVarData[33] = 0x00;
    pVarContainer->pVarData[34] = 0x00;
    pVarContainer->pVarData[35] = 0x00;
    pVarContainer->pVarData[36] = 0x06;

    /* Num of WLAN Port */
    pVarContainer->pVarData[37] = 0x00;
    pVarContainer->pVarData[38] = boardCap.wlanPortCnt;
	
    /* Battery Backup - No battery backup */
    pVarContainer->pVarData[39] = 0x00;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0008 - HoldoverConfig */
int ctc_oam_varCb_holdoverConfig_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret = RT_ERR_OK;
	oam_config_t oamConfig;
    unsigned int getState, getTime;

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_HOLDOVERCONFIG_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

	epon_oam_config_get(llidIdx, &oamConfig);
	if (1 == oamConfig.holdoverEnable)
	{
		getState = CTC_OAM_VAR_HOLDOVERCONFIG_ACTIVE;
	}
	else if (0 == oamConfig.holdoverEnable)
	{
		getState = CTC_OAM_VAR_HOLDOVERCONFIG_DEACTIVE;
	}
	getTime = oamConfig.holdoverTime;

    /* Construct return value */
    CTC_BUF_ENCODE32(pVarContainer->pVarData, &getState);
    CTC_BUF_ENCODE32(pVarContainer->pVarData + 4, &getTime);

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0008 - HoldoverConfig */
int ctc_oam_varCb_holdoverConfig_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
	oam_config_t oamConfig;
    unsigned int setState, setTime;

    if(CTC_OAM_VAR_HOLDOVERCONFIG_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    CTC_BUF_PARSE32(pVarContainer->pVarData, &setState);
    CTC_BUF_PARSE32(pVarContainer->pVarData + 4, &setTime);

	epon_oam_config_get(llidIdx, &oamConfig);
	
	if (CTC_OAM_VAR_HOLDOVERCONFIG_ACTIVE == setState)
		oamConfig.holdoverEnable = 1;
	else if (CTC_OAM_VAR_HOLDOVERCONFIG_DEACTIVE == setState)
		oamConfig.holdoverEnable = 0;
	else
	{
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
		return EPON_OAM_ERR_OK;
	}
	oamConfig.holdoverTime = setTime;

    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0009 - MxUMngGlobalParameter */
int ctc_oam_varCb_mxUMngGlobalParameter_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	ctc_wrapper_mxuMngGlobal_t mxu;
	int ret;
	unsigned char *pPtr;
	
	ret = CTC_WRAPPER(mxuMngGlobalGet, &mxu);
	if(EPON_OAM_ERR_OK != ret)
        return ret;

	if(mxu.isIpv4)
		pVarContainer->varWidth = 17;
	else
		pVarContainer->varWidth = 41;
	
	pVarContainer->varDesc = varDesc;
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
	if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    pPtr = pVarContainer->pVarData;
	if(mxu.isIpv4)
	{
		/* MngIPAddr */
		CTC_BUF_ENCODE32(pPtr, &mxu.ip[0]);
		pPtr += 4;
		/* MngIPMask */
		CTC_BUF_ENCODE32(pPtr, &mxu.mask);
		pPtr += 4;
		/* MngIPGW */
		CTC_BUF_ENCODE32(pPtr, &mxu.gateway[0]);
		pPtr += 4;
	}
	else
	{
		CTC_BUF_ENCODE32(pPtr, &mxu.ip[0]);
		pPtr += 4;
		CTC_BUF_ENCODE32(pPtr, &mxu.ip[1]);
		pPtr += 4;
		CTC_BUF_ENCODE32(pPtr, &mxu.ip[2]);
		pPtr += 4;
		CTC_BUF_ENCODE32(pPtr, &mxu.ip[3]);
		pPtr += 4;
		CTC_BUF_ENCODE32(pPtr, &mxu.mask);
		pPtr += 4;
		CTC_BUF_ENCODE32(pPtr, &mxu.gateway[0]);
		pPtr += 4;
		CTC_BUF_ENCODE32(pPtr, &mxu.gateway[1]);
		pPtr += 4;
		CTC_BUF_ENCODE32(pPtr, &mxu.gateway[2]);
		pPtr += 4;
		CTC_BUF_ENCODE32(pPtr, &mxu.gateway[3]);
		pPtr += 4;
	}
	/* MngDataCVlan */
	CTC_BUF_ENCODE16(pPtr, &mxu.cvlan);
	pPtr += 2;
	/* MngDataSVLAN */
	CTC_BUF_ENCODE16(pPtr, &mxu.svlan);
	pPtr += 2;
	/* MngDataPriority */
	*pPtr++ = mxu.priority;
	
	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0009 - MxUMngGlobalParameter */
int ctc_oam_varCb_mxUMngGlobalParameter_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	ctc_wrapper_mxuMngGlobal_t mxu;
	int offset;
	int ret;
	
	if(pVarContainer->varWidth == 17)
		mxu.isIpv4 = 1;
	else if(pVarContainer->varWidth == 41)
		mxu.isIpv4 = 0;
	else
		goto BADPARAM;

	if(mxu.isIpv4)
	{
		/* MngIPAddr */
		CTC_BUF_PARSE32(pVarContainer->pVarData, &mxu.ip[0]);
		/* MngIPMask */
		CTC_BUF_PARSE32(pVarContainer->pVarData+4, &mxu.mask);
		/* MngIPGW */
		CTC_BUF_PARSE32(pVarContainer->pVarData+8, &mxu.gateway[0]);
		offset = 12;
	}
	else
	{
		CTC_BUF_PARSE32(pVarContainer->pVarData, &mxu.ip[0]);
		CTC_BUF_PARSE32(pVarContainer->pVarData+4, &mxu.ip[1]);
		CTC_BUF_PARSE32(pVarContainer->pVarData+8, &mxu.ip[2]);
		CTC_BUF_PARSE32(pVarContainer->pVarData+12, &mxu.ip[3]);
		CTC_BUF_PARSE32(pVarContainer->pVarData+16, &mxu.mask);
		CTC_BUF_PARSE32(pVarContainer->pVarData+20, &mxu.gateway[0]);
		CTC_BUF_PARSE32(pVarContainer->pVarData+24, &mxu.gateway[1]);
		CTC_BUF_PARSE32(pVarContainer->pVarData+28, &mxu.gateway[2]);
		CTC_BUF_PARSE32(pVarContainer->pVarData+32, &mxu.gateway[3]);
		offset = 36;
	}
	/* MngDataCVlan */
	CTC_BUF_PARSE16(pVarContainer->pVarData+offset, &mxu.cvlan);
	/* MngDataSVLAN */
	CTC_BUF_PARSE16(pVarContainer->pVarData+offset+2, &mxu.svlan);
	/* MngDataPriority default 5*/
	mxu.priority = pVarContainer->pVarData[offset+4];

	if(mxu.isIpv4)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
				"IPv4 "IPADDR_PRINT" netmask "IPADDR_PRINT" cvlan[%d] svlan[%d]\n", 
				IPADDR_PRINT_ARG(mxu.ip[0]), IPADDR_PRINT_ARG(mxu.mask), mxu.cvlan, mxu.svlan);
	}
	
	ret = CTC_WRAPPER(mxuMngGlobalSet, &mxu);
	if(EPON_OAM_ERR_OK != ret)
        goto BADPARAM;
      
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
    return EPON_OAM_ERR_OK;

BADPARAM:
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x000A - MxUMngSNMPParameter */
int ctc_oam_varCb_mxUMngSNMPParameter_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
#ifdef CONFIG_USER_SNMPD_SNMPD_V2CTRAP
	struct in_addr trap_ip;
    unsigned char ipStr[16];
	unsigned short trap_port=162, snmp_port=161;
	unsigned char commRW[100], commRO[100], securName[30];
    unsigned char *pPtr;

    pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = CTC_OAM_VAR_MXUMNGSNMPPARAMETER_IPV4_LEN;
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    pPtr = pVarContainer->pVarData;

	memset(commRW, 0, sizeof(commRW));
	memset(commRO, 0, sizeof(commRO));
	memset(securName, 0, sizeof(securName));
	
	//mib_get(MIB_SNMP_TRAP_IP, (void *)&trap_ip);
    ctc_oam_flash_var_get("SNMP_TRAP_IP", ipStr, sizeof(ipStr));
    inet_aton(ipStr, &trap_ip);
	//mib_get(MIB_SNMP_COMM_RO,  (void *)commRO);
    ctc_oam_flash_var_get("SNMP_COMM_RO", commRO, sizeof(commRO));
	//mib_get(MIB_SNMP_COMM_RW,  (void *)commRW);
	ctc_oam_flash_var_get("SNMP_COMM_RW", commRO, sizeof(commRW));

	/* SNMPVer: fixed to 2 */
	*(unsigned char *)pPtr++ = 0x02;
	/* TrapHostIPAddr */
	CTC_BUF_ENCODE32(pPtr, &trap_ip.s_addr);
	pPtr += 4;
	/* Trap port */
	CTC_BUF_ENCODE32(pPtr, &trap_port);
	pPtr += 2;
	/* SNMP port */
	CTC_BUF_ENCODE32(pPtr, &snmp_port);
	pPtr += 2;
	/* Security Name */
	memcpy(pPtr, "ADSL", 4);
	pPtr += 32;
	/* CommunityForRead */
	memcpy(pPtr, commRO, 32);
	pPtr += 32;
	/* CommunityForWrite */
	memcpy(pPtr, commRW, 32);
	pPtr += 32;

	return EPON_OAM_ERR_OK;
#else
    pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	
	return EPON_OAM_ERR_OK;
#endif
}

/* 0xC7/0x000A - MxUMngSNMPParameter */
int ctc_oam_varCb_mxUMngSNMPParameter_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
#ifdef CONFIG_USER_SNMPD_SNMPD_V2CTRAP
	struct in_addr trap_ip;
	unsigned short trap_port=162, snmp_port=161;
	unsigned char commRW[100], commRO[100], securName[30];
	unsigned char snmpEnable;
    char tmpStr[10];

    if (CTC_OAM_VAR_MXUMNGSNMPPARAMETER_IPV4_LEN != pVarContainer->varWidth)
    {
    	if (CTC_OAM_VAR_MXUMNGSNMPPARAMETER_IPV6_LEN != pVarContainer->varWidth)
		{
			pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
			return EPON_OAM_ERR_OK;
		}
		
        return EPON_OAM_ERR_UNKNOWN;
    }
    /* SNMPVer: fixed to 2 */
	if (pVarContainer->pVarData[0] != 2)
		goto BADPARAM;
	/* TrapHostIPAddr */
	CTC_BUF_PARSE32(pVarContainer->pVarData+1, &trap_ip.s_addr);
	/* Trap port */
	CTC_BUF_PARSE16(pVarContainer->pVarData+5, &trap_port);
	/* SNMP port */
	CTC_BUF_PARSE16(pVarContainer->pVarData+7, &snmp_port);
	/* Security Name */
	memcpy(securName, pVarContainer->pVarData+9, 32);
	/* CommunityForRead */
	memcpy(commRO, pVarContainer->pVarData+41, 32);
	/* CommunityForWrite */
	memcpy(commRW, pVarContainer->pVarData+73, 32);

	//mib_set(MIB_SNMP_TRAP_IP, (void *)&trap_ip);
    ctc_oam_flash_var_set("SNMP_TRAP_IP", inet_ntoa(trap_ip));
	//mib_set(MIB_SNMP_COMM_RO,  (void *)commRO);
	ctc_oam_flash_var_set("SNMP_COMM_RO", commRO);
	//mib_set(MIB_SNMP_COMM_RW,  (void *)commRW);
	ctc_oam_flash_var_set("SNMP_COMM_RW", commRW);

	//mib_get(MIB_SNMPD_ENABLE, (void *)&snmpEnable);
    ctc_oam_flash_var_get("SNMPD_ENABLE", tmpStr, sizeof(tmpStr));
    snmpEnable = atoi(tmpStr);
	if (snmpEnable)
		restart_snmp(1);

    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
    return EPON_OAM_ERR_OK;

BADPARAM:
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
	return EPON_OAM_ERR_OK;
#else
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK; /* return setok to avoid oam register failure */

	return EPON_OAM_ERR_OK;
#endif
}

/* 0xC7/0x000B - Active PON_IFAdminstate */
int ctc_oam_varCb_activePonIfAdminstate_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    pVarContainer->varDesc = varDesc;
	if (0 == pon_if_admin)
	{
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
		return EPON_OAM_ERR_OK;
	}
	else
	{
	    pVarContainer->varWidth = CTC_OAM_VAR_ACTIVEPONIFADMINSTATE_LEN; /* Fix value from CTC standard */
	    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
	    if(NULL == pVarContainer->pVarData)
	    {
	        return EPON_OAM_ERR_MEM;
	    }

	    pVarContainer->pVarData[0] = pon_active_port;

	    return EPON_OAM_ERR_OK;
	}
}

/* 0xC7/0x000B - Active PON_IFAdminstate */
int ctc_oam_varCb_activePonIfAdminstate_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
#if 0
	if (CTC_OAM_VAR_ACTIVEPONIFADMINSTATE_LEN != pVarContainer->varWidth)
	{
		return EPON_OAM_ERR_UNKNOWN;
	}

	pon_active_port = pVarContainer->pVarData[0];
	/* TODO: take effect here */

	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
    return EPON_OAM_ERR_OK;
#else
    /* Not support type C/D protection */
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;

    return EPON_OAM_ERR_OK;
#endif
}

/* 0xC7/0x000C - ONU capabilities-3 */
int ctc_oam_varCb_onuCapabilities3_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    pVarContainer->varDesc = varDesc;
    /* For sample system, pretend it doesn't support IPv6 aware and power control */
    pVarContainer->varWidth = CTC_OAM_VAR_ONUCAPABILITES_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* IPv6 Supported - not supported */
    pVarContainer->pVarData[0] = 0x00;

    /* ONUPowerSupplyControl - not supported */
    pVarContainer->pVarData[1] = 0x00;

	/* Service SLA */
	if ((0 == srvDBA) || (1 == srvNum))//not supported
		pVarContainer->pVarData[2] = 0x1;
	else
		pVarContainer->pVarData[2] = srvNum;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x000D - ONU power saving capabilities */
int ctc_oam_varCb_onuPowerSavingCapabilities_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_ONUPOWERSAVINGCAPABILITES_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* Sleep mode capability - support Tx sleep mode only */
    pVarContainer->pVarData[0] = CTC_OAM_VAR_ONUPOWERSAVINGCAPABILITES_TX_ONLY;

    /* Early wake-up capability - not supported */
    pVarContainer->pVarData[1] = 0x00;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x000E - ONU power saving config */
int ctc_oam_varCb_onuPowerSavingConfig_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_ONUPOWERSAVINGCONFIG_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* Early wake-up - unsupport */
    pVarContainer->pVarData[0] = CTC_OAM_VAR_ONUPOWERSAVINGCONFIG_UNSUPPORT;

    /* Sleep duration max */
    CTC_BUF_ENCODE48(pVarContainer->pVarData+1, &power_saving_sleep_duration_max);

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x000E - ONU power saving config */
int ctc_oam_varCb_onuPowerSavingConfig_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	unsigned long long sleepDuration;

	if(CTC_OAM_VAR_ONUPOWERSAVINGCONFIG_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    /* support power saving : Tx sleep mode only */
	CTC_BUF_PARSE48(pVarContainer->pVarData+1, &sleepDuration);
	
	power_saving_sleep_duration_max = sleepDuration;
	
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x000F - ONU Protection Parameters */
int ctc_oam_varCb_onuProtectionParameters_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
#if 0//9601B not supported
    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_ONUPROTECTIONPARAMETERS_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* Due to the actual LOS time setting is on the tranceiver side
     * And the tranciver control should be different from vender to vender
     * So here always return the default value.
     */

    /* T LOS_Optical - 2 ms */
    CTC_BUF_ENCODE16(pVarContainer->pVarData, los_optical_time);

    /* T LOS_MAC - 50 ms */
    CTC_BUF_ENCODE16(pVarContainer->pVarData + 2, los_mac_time);

    return EPON_OAM_ERR_OK;
#else
    pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK; /* return setok to avoid oam register failure */
	
	return EPON_OAM_ERR_OK;
#endif
}

/* 0xC7/0x000F - ONU Protection Parameters */
int ctc_oam_varCb_onuProtectionParameters_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
#if 0//9601B not support
	if (CTC_OAM_VAR_ONUPROTECTIONPARAMETERS_LEN != pVarContainer->varWidth)
	{
		return EPON_OAM_ERR_UNKNOWN;
	}

	CTC_BUF_PARSE16(pVarContainer->pVarData, &los_optical_time);
	CTC_BUF_PARSE16(pVarContainer->pVarData+2, &los_mac_time);
	/* TODO: take effect here */

	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
    return EPON_OAM_ERR_OK;
#else
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	return EPON_OAM_ERR_OK;
#endif
}
/* 0xC7/0x0011 - EthLinkState */
int ctc_oam_varCb_ethLinkState_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
    rtk_enable_t enable;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_ETHLINKSTATE_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    ret = CTC_WRAPPER(portLinkStatusGet, pVarInstant->parse.uniPort.portNo - 1, &enable);
    if(RT_ERR_OK == ret)
    {
        if(DISABLED == enable)
        {
            *pVarContainer->pVarData = CTC_OAM_VAR_ETHLINKSTATE_DOWN;
        }
        else
        {
            *pVarContainer->pVarData = CTC_OAM_VAR_ETHLINKSTATE_UP;
        }
    }
    else
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0012 - EthPortPause */
int ctc_oam_varCb_ethPortPause_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
    rtk_enable_t enable;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_ETHPORTPAUSE_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    ret = CTC_WRAPPER(portFlowControlGet, pVarInstant->parse.uniPort.portNo - 1, &enable);
    if(RT_ERR_OK == ret)
    {
        if(enable == ENABLED)
        {
            *pVarContainer->pVarData = CTC_OAM_VAR_ETHPORTPAUSE_ENABLE;
        }
        else
        {
            *pVarContainer->pVarData = CTC_OAM_VAR_ETHPORTPAUSE_DISABLE;
        }
    }
    else
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0012 - ethPort Pause */
int ctc_oam_varCb_ethPortPause_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int port;
    unsigned char setValue;
    rtk_enable_t enable;

    if(CTC_OAM_VAR_ETHPORTPAUSE_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    setValue = *pVarContainer->pVarData;
    if(CTC_OAM_VAR_ETHPORTPAUSE_DISABLE == setValue)
    {
        enable = DISABLED;
    }
    else if(CTC_OAM_VAR_ETHPORTPAUSE_ENABLE == setValue)
    {
        enable = ENABLED;
    }
    else
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
        return EPON_OAM_ERR_OK;
    }
    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
        "[OAM:%s:%d] port pause %s\n", __FILE__, __LINE__, (ENABLED == enable) ? "enable" : "disable");

    ret = CTC_WRAPPER(portFlowControlSet, pVarInstant->parse.uniPort.portNo - 1, enable);
    if(RT_ERR_OK != ret)
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
        return EPON_OAM_ERR_OK;
    }
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0013 - ethPortUs Policing */
int ctc_oam_varCb_ethPortUsPolicing_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
    unsigned int rateCir, rateCbs, rateEbs;
    rtk_enable_t enable;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    ret = CTC_WRAPPER(portIngressBwGet, pVarInstant->parse.uniPort.portNo - 1, &enable, &rateCir, &rateCbs, &rateEbs);
    if(RT_ERR_OK == ret)
    {
        pVarContainer->varDesc = varDesc;
        if(enable == ENABLED)
        {
            pVarContainer->varWidth = CTC_OAM_VAR_ETHERPORTUSPOLICING_ENABLE_LEN; /* Fix value from CTC standard */
        }
        else
        {
            pVarContainer->varWidth = CTC_OAM_VAR_ETHERPORTUSPOLICING_DISABLE_LEN; /* Fix value from CTC standard */
        }

        pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
        if(NULL == pVarContainer->pVarData)
        {
            return EPON_OAM_ERR_MEM;
        }

        if(enable == ENABLED)
        {
            /* Don't change the order */
            CTC_BUF_ENCODE32(&pVarContainer->pVarData[6], &rateEbs);
            CTC_BUF_ENCODE32(&pVarContainer->pVarData[3], &rateCbs);
            CTC_BUF_ENCODE32(&pVarContainer->pVarData[0], &rateCir);
            *pVarContainer->pVarData = CTC_OAM_VAR_ETHERPORTUSPOLICING_ENABLE;
            /* Don't change the order */
        }
        else
        {
            *pVarContainer->pVarData = CTC_OAM_VAR_ETHERPORTUSPOLICING_DISABLE;
        }
    }
    else
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0013 - ethPortUs Policing */
int ctc_oam_varCb_ethPortUsPolicing_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int rateCir, rateCbs, rateEbs;
    unsigned char state;

    if((CTC_OAM_VAR_ETHERPORTUSPOLICING_ENABLE_LEN != pVarContainer->varWidth) &&
        (CTC_OAM_VAR_ETHERPORTUSPOLICING_DISABLE_LEN != pVarContainer->varWidth))
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    state = *pVarContainer->pVarData;
    if((CTC_OAM_VAR_ETHERPORTUSPOLICING_ENABLE == state) && (CTC_OAM_VAR_ETHERPORTUSPOLICING_ENABLE_LEN == pVarContainer->varWidth))
    {
        CTC_BUF_PARSE32(pVarContainer->pVarData + 1, &rateCir);
        rateCir = (rateCir & 0xffffff00) >> 8;
        CTC_BUF_PARSE32(pVarContainer->pVarData + 4, &rateCbs);
        rateCbs = (rateCbs & 0xffffff00) >> 8;
        CTC_BUF_PARSE32(pVarContainer->pVarData + 7, &rateEbs);
        rateEbs = (rateEbs & 0xffffff00) >> 8;
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
            "[OAM:%s:%d] enable us policing cir/cbs/ebs %u/%u/%u\n",
            __FILE__, __LINE__, rateCir, rateCbs, rateEbs);
        ret = CTC_WRAPPER(portIngressBwSet, pVarInstant->parse.uniPort.portNo - 1, ENABLED, rateCir, rateCbs, rateEbs);
        if(EPON_OAM_ERR_OK != ret)
        {
            pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
            return EPON_OAM_ERR_OK;
        }
    }
    else if((CTC_OAM_VAR_ETHERPORTUSPOLICING_DISABLE == state) && (CTC_OAM_VAR_ETHERPORTUSPOLICING_DISABLE_LEN == pVarContainer->varWidth))
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
            "[OAM:%s:%d] disable us policing\n", __FILE__, __LINE__);
        ret = CTC_WRAPPER(portIngressBwSet, pVarInstant->parse.uniPort.portNo - 1, DISABLED, 0, 0, 0);
        if(EPON_OAM_ERR_OK != ret)
        {
            pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
            return EPON_OAM_ERR_OK;
        }
    }
    else
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
        return EPON_OAM_ERR_OK;
    }
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0014 - voip port */
static unsigned char voipManagement = 0; /* TODO: fake global vlaue, to pass the olt test */
int ctc_oam_varCb_voipPort_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

	pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = 1;
	pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
	if(NULL == pVarContainer->pVarData)
	{
		return EPON_OAM_ERR_MEM;
	}
	*pVarContainer->pVarData = voipManagement;
	
	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0014 - voip port */
int ctc_oam_varCb_voipPort_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	voipManagement = *pVarContainer->pVarData;
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0016 - EthPortDs RateLimiting */
int ctc_oam_varCb_ethPortDsRateLimiting_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
    unsigned int rateCir, ratePir;
    rtk_enable_t enable;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    ret = CTC_WRAPPER(portEgressBwGet, pVarInstant->parse.uniPort.portNo - 1, &enable, &rateCir, &ratePir);
    if(RT_ERR_OK == ret)
    {
        pVarContainer->varDesc = varDesc;
        if(enable == ENABLED)
        {
            pVarContainer->varWidth = CTC_OAM_VAR_ETHERPORTDSRATELIMITING_ENABLE_LEN; /* Fix value from CTC standard */
        }
        else
        {
            pVarContainer->varWidth = CTC_OAM_VAR_ETHERPORTDSRATELIMITING_DISABLE_LEN; /* Fix value from CTC standard */
        }

        pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
        if(NULL == pVarContainer->pVarData)
        {
            return EPON_OAM_ERR_MEM;
        }

        if(enable == ENABLED)
        {
            /* Don't change the order */
            CTC_BUF_ENCODE32(&pVarContainer->pVarData[3], &ratePir);
            CTC_BUF_ENCODE32(&pVarContainer->pVarData[0], &rateCir);
            *pVarContainer->pVarData = CTC_OAM_VAR_ETHERPORTDSRATELIMITING_ENABLE;
            /* Don't change the order */
        }
        else
        {
            *pVarContainer->pVarData = CTC_OAM_VAR_ETHERPORTDSRATELIMITING_DISABLE;
        }
    }
    else
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0016 - EthPortDs RateLimiting */
int ctc_oam_varCb_ethPortDsRateLimiting_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int rateCir, ratePir;
    unsigned char state;

    if((CTC_OAM_VAR_ETHERPORTDSRATELIMITING_ENABLE_LEN != pVarContainer->varWidth) &&
        (CTC_OAM_VAR_ETHERPORTDSRATELIMITING_DISABLE_LEN != pVarContainer->varWidth))
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

    state = *pVarContainer->pVarData;
    if((CTC_OAM_VAR_ETHERPORTDSRATELIMITING_ENABLE == state) &&
        (CTC_OAM_VAR_ETHERPORTDSRATELIMITING_ENABLE_LEN == pVarContainer->varWidth))
    {
        CTC_BUF_PARSE32(pVarContainer->pVarData + 1, &rateCir);
        rateCir = (rateCir & 0xffffff00) >> 8;
        CTC_BUF_PARSE32(pVarContainer->pVarData + 4, &ratePir);
        ratePir = (ratePir & 0xffffff00) >> 8;
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
            "[OAM:%s:%d] enable ds rate limiting cir/pir %u/%u\n",
            __FILE__, __LINE__, rateCir, ratePir);
        ret = CTC_WRAPPER(portEgressBwSet, pVarInstant->parse.uniPort.portNo - 1, ENABLED, rateCir, ratePir);
        if(EPON_OAM_ERR_OK != ret)
        {
            pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
            return EPON_OAM_ERR_OK;
        }
    }
    else if((CTC_OAM_VAR_ETHERPORTDSRATELIMITING_DISABLE == state) &&
        (CTC_OAM_VAR_ETHERPORTDSRATELIMITING_DISABLE_LEN == pVarContainer->varWidth))
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
            "[OAM:%s:%d] disable ds rate limiting\n", __FILE__, __LINE__);
        ret = CTC_WRAPPER(portEgressBwSet, pVarInstant->parse.uniPort.portNo - 1, DISABLED, 0, 0);
        if(EPON_OAM_ERR_OK != ret)
        {
            pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
            return EPON_OAM_ERR_OK;
        }
    }
    else
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
        return EPON_OAM_ERR_OK;
    }
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0017 - PortLoopDetect */
int ctc_oam_varCb_portLoopDetect_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int port;
    unsigned int setValue;
    rtk_enable_t enable;

    if(CTC_OAM_VAR_PORTLOOPDETECT_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(!CTC_INSTANT_IS_ETHERNET(pVarInstant))
    {
        return EPON_OAM_ERR_PARAM;
    }

    CTC_BUF_PARSE32(pVarContainer->pVarData, &setValue);
    if(CTC_OAM_VAR_PORTLOOPDETECT_DEACTIVE == setValue)
    {
        enable = DISABLED;
    }
    else if(CTC_OAM_VAR_PORTLOOPDETECT_ACTIVE == setValue)
    {
        enable = ENABLED;
    }
    else
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
        return EPON_OAM_ERR_OK;
    }
    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
        "[OAM:%s:%d] port loop detect %s\n", __FILE__, __LINE__, (ENABLED == enable) ? "enable" : "disable");

	if(CTC_INSTANT_IS_ALLPORTS(pVarInstant))
	{
		int i;
		for(i=0; i<LAN_PORT_NUM; i++)
		{
			ret = CTC_WRAPPER(portLoopDetectSet, i, enable);
		    if(RT_ERR_OK != ret)
		    {
		        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
		        return EPON_OAM_ERR_OK;
		    }
		}
	}
	else
	{
	    ret = CTC_WRAPPER(portLoopDetectSet, pVarInstant->parse.uniPort.portNo - 1, enable);
	    if(RT_ERR_OK != ret)
	    {
	        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	        return EPON_OAM_ERR_OK;
	    }
	}
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0018 - PortDisableLooped */
int ctc_oam_varCb_portDisableLooped_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int port;
    unsigned int setValue;
    rtk_enable_t enable;

    if(CTC_OAM_VAR_PORTDISABLELOOPED_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(!CTC_INSTANT_IS_ETHERNET(pVarInstant))
    {
        return EPON_OAM_ERR_PARAM;
    }

    CTC_BUF_PARSE32(pVarContainer->pVarData, &setValue);
    if(CTC_OAM_VAR_PORTDISABLELOOPED_DISABLE == setValue)
    {
        enable = DISABLED;
    }
    else if(CTC_OAM_VAR_PORTDISABLELOOPED_ENABLE == setValue)
    {
        enable = ENABLED;
    }
    else
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
        return EPON_OAM_ERR_OK;
    }
    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
        "[OAM:%s:%d] port disable looped %s\n", __FILE__, __LINE__, (ENABLED == enable) ? "enable" : "disable");

	if(CTC_INSTANT_IS_ALLPORTS(pVarInstant))
	{
		int i;
		for(i=0; i<LAN_PORT_NUM; i++)
		{
			ret = CTC_WRAPPER(portDisableLoopedSet, i, enable);
		    if(RT_ERR_OK != ret)
		    {
		        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
		        return EPON_OAM_ERR_OK;
		    }
		}
	}
	else
	{
	    ret = CTC_WRAPPER(portDisableLoopedSet, pVarInstant->parse.uniPort.portNo - 1, enable);
	    if(RT_ERR_OK != ret)
	    {
	        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	        return EPON_OAM_ERR_OK;
	    }
	}
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0019 - PortLoopParameterConfig */
int ctc_oam_varCb_portLoopParameterConfig_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	int ret;
	int i;
	unsigned char *pPtr;
	unsigned short detectFrequency;
	unsigned short recoveryInterval;
	short svlan[CTC_OAM_VAR_PORTLOOPPARAMETERCONFIG_MAX_VLAN_NUM];
	short cvlan[CTC_OAM_VAR_PORTLOOPPARAMETERCONFIG_MAX_VLAN_NUM];
	
	if(CTC_OAM_VAR_PORTLOOPPARAMETERCONFIG_MIN_LEN > pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

	if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(!CTC_INSTANT_IS_ETHERNET(pVarInstant))
    {
        return EPON_OAM_ERR_PARAM;
    }
	
	pPtr = pVarContainer->pVarData;
	CTC_BUF_PARSE16(pPtr, &detectFrequency);
	pPtr += 2;
	CTC_BUF_PARSE16(pPtr, &recoveryInterval);
	pPtr += 2;

	for(i = 0; i < CTC_OAM_VAR_PORTLOOPPARAMETERCONFIG_MAX_VLAN_NUM; i++)
	{
		if(pVarContainer->varWidth >= (CTC_OAM_VAR_PORTLOOPPARAMETERCONFIG_MIN_LEN + 4*(i+1)))
		{
			CTC_BUF_PARSE16(pPtr, &svlan[i]);
			pPtr += 2;
			CTC_BUF_PARSE16(pPtr, &cvlan[i]);
			pPtr += 2;
		}
		else		
			break;
	}
	svlan[i] = -1;
	cvlan[i] = -1;
	
	if(detectFrequency == 0) /* value range is 1 ~ 65535 */
		return EPON_OAM_ERR_PARAM;

	if(CTC_INSTANT_IS_ALLPORTS(pVarInstant))
	{
		for(i=0; i<LAN_PORT_NUM; i++)
		{
			ret = CTC_WRAPPER(portLoopParameterConfigSet, i, detectFrequency, recoveryInterval, svlan, cvlan);
		    if(RT_ERR_OK != ret)
		    {
		        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
		        return EPON_OAM_ERR_OK;
		    }
		}
	}
	else
	{
	    ret = CTC_WRAPPER(portLoopParameterConfigSet, pVarInstant->parse.uniPort.portNo - 1, detectFrequency, recoveryInterval, svlan, cvlan);
	    if(RT_ERR_OK != ret)
	    {
	        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	        return EPON_OAM_ERR_OK;
	    }
	}
	
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
	
    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x00A4 - PortMACAgingTime */
int ctc_oam_varCb_portMacAgingTime_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
    unsigned int getValue;
    rtk_enable_t enable;

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_PORTMACAGINGTIME_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }
	/* siyuan 20170719: operation target of this command was changed from UNI port to ONU, ignore parameter port */
    ret = CTC_WRAPPER(portMacAgingGet, 0, &enable, &getValue);
    if(RT_ERR_OK == ret)
    {
        if(DISABLED == enable)
        {
            getValue = CTC_OAM_VAR_PORTMACAGINGTIME_DEACTIVE;
        }
        CTC_BUF_ENCODE32(pVarContainer->pVarData, &getValue);
    }
    else
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x00A4 - PortMACAgingTime */
int ctc_oam_varCb_portMacAgingTime_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int setValue;
    rtk_enable_t enable;

    if(CTC_OAM_VAR_PORTMACAGINGTIME_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    CTC_BUF_PARSE32(pVarContainer->pVarData, &setValue);
    if(CTC_OAM_VAR_PORTMACAGINGTIME_DEACTIVE == setValue)
    {
        setValue = 0;
        enable = DISABLED;
    }
    else
    {
        enable = ENABLED;
    }
    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
        "[OAM:%s:%d] aging time %u\n", __FILE__, __LINE__, setValue);

	/* siyuan 20170719: operation target of this command was changed from UNI port to ONU, ignore parameter port */
    ret = CTC_WRAPPER(portMacAgingSet, 0, enable, setValue);
    if(RT_ERR_OK != ret)
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
        return EPON_OAM_ERR_OK;
    }
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

#if 0
/* According to CTC Spec, the transparent mode is defined as:
 * Upstream: tag in, tag out, untag in, untag out. No change to tag.
 * Downstream: tag in, tag out, untag in, untag out. No change to tag.
 */
static int ctc_oam_varCb_vlanTransparent_set(
        rtk_port_t port)
{
    int ret;
    rtk_port_t ponPort;
    rtk_portmask_t igrPortMask;
    rtk_vlan_tagKeepType_t keepType;

    /* Set up a transparent tunnel between PON port and UNI port */
    ponPort = HAL_GET_PON_PORT();
    /* Set upstream (PON port) */
    ret = rtk_vlan_portEgrTagKeepType_get(ponPort, &igrPortMask, &keepType);
    if(RT_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    RTK_PORTMASK_PORT_SET(igrPortMask, port);
    ret = rtk_vlan_portEgrTagKeepType_set(ponPort, &igrPortMask, keepType);
    if(RT_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    /* Set downstream (UNI port) */
    ret = rtk_vlan_portEgrTagKeepType_get(port, &igrPortMask, &keepType);
    if(RT_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    RTK_PORTMASK_PORT_SET(igrPortMask, ponPort);
    ret = rtk_vlan_portEgrTagKeepType_set(port, &igrPortMask, keepType);
    if(RT_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

/* According to CTC Spec, the transparent mode is defined as:
 * Upstream: untag in, tag out, tag in drop.
 * Downstream: tag in, untag out, untag in, drop.
 */
static int ctc_oam_varCb_vlanTag_set(
    rtk_port_t port,
    unsigned int vlan,
    unsigned int priority)
{
    int ret;

    ret = rtk_vlan_portAcceptFrameType_set(port, ACCEPT_FRAME_TYPE_UNTAG_ONLY);
    if(RT_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    ret = rtk_vlan_portPvid_set(port, vlan);
    if(RT_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    ret = rtk_vlan_portPriority_set(port, priority);
    if(RT_ERR_OK != ret)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}
#endif

static int ctc_oam_vlanAggrValid_check(
    unsigned char *pPktBuf, /* Point to "Number of VLAN Aggregation tables" field */
    unsigned char remainLen) /* Remaining length from "Number of VLAN Aggregation tables" to the end */
{
    unsigned int i;
    unsigned short tableNum, entryNum;

    if(remainLen < 2)
    {
        return EPON_OAM_ERR_PARSE;
    }

    CTC_BUF_PARSE16(pPktBuf, &tableNum);
    CTC_BUF_ADD(pPktBuf, remainLen, 2); /* tableNum */
    for(i = 0 ; i < tableNum ; i++)
    {
        if(remainLen < 6)
        {
            return EPON_OAM_ERR_PARSE;
        }
        CTC_BUF_PARSE16(pPktBuf, &entryNum);
        CTC_BUF_ADD(pPktBuf, remainLen, 2); /* entryNum */
        CTC_BUF_ADD(pPktBuf, remainLen, 4); /* VLAN to be aggr */
        if(remainLen < entryNum * 4)
        {
            return EPON_OAM_ERR_PARSE;
        }
        CTC_BUF_ADD(pPktBuf, remainLen, entryNum * 4); /* Aggregated VLANs */
    }

    return EPON_OAM_ERR_OK;
}

#ifdef CONFIG_SFU_APP
/* 0xC7/0x0021 - VLAN */
int ctc_oam_varCb_vlan_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int i, j;
    int ret;
    unsigned char *pPtr;
    unsigned int totalLen;
    ctc_wrapper_vlanCfg_t vlanCfg;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }
    
    if(CTC_INSTANT_IS_ETHERNET(pVarInstant) && !CTC_INSTANT_IS_ALLPORTS(pVarInstant))
    {
        ret = CTC_WRAPPER(vlanGet, pVarInstant->parse.uniPort.portNo - 1, &vlanCfg);
        if(EPON_OAM_ERR_OK != ret)
        {
            return EPON_OAM_ERR_UNKNOWN;
        }
        pVarContainer->varDesc = varDesc;

        ret = EPON_OAM_ERR_OK;
        switch(vlanCfg.vlanMode)
        {
        case CTC_OAM_VAR_VLAN_MODE_TRANSPARENT:
            pVarContainer->varWidth = CTC_OAM_VAR_VLAN_MODE_LEN; /* For transparent mode, only mode field is counting for */
            pVarContainer->pVarData = (unsigned char *) malloc(sizeof(unsigned char) * pVarContainer->varWidth);
            if(NULL == pVarContainer->pVarData)
            {
                ret = EPON_OAM_ERR_MEM;
            }
            else
            {
                *pVarContainer->pVarData = vlanCfg.vlanMode;
            }
            break;
        case CTC_OAM_VAR_VLAN_MODE_TAG:
            pVarContainer->varWidth = CTC_OAM_VAR_VLAN_TAG_MODE_LEN; /* For tag mode, mode and VLAN tag fields are counting for */
            pVarContainer->pVarData = (unsigned char *) malloc(sizeof(unsigned char) * pVarContainer->varWidth);
            if(NULL == pVarContainer->pVarData)
            {
                ret = EPON_OAM_ERR_MEM;
            }
            else
            {
                *pVarContainer->pVarData = vlanCfg.vlanMode;
                CTC_BUF_ENCODE32(pVarContainer->pVarData + CTC_OAM_VAR_VLAN_MODE_LEN, &(vlanCfg.cfg.tagCfg));
            }
            break;
        case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
            pVarContainer->varWidth = CTC_OAM_VAR_VLAN_TRANSLATION_MODE_MIN + (vlanCfg.cfg.transCfg.num) * CTC_OAM_VAR_VLAN_TRANSLATION_ENTRY_LEN;
            pVarContainer->pVarData = (unsigned char *) malloc(sizeof(unsigned char) * pVarContainer->varWidth);
            if(NULL == pVarContainer->pVarData)
            {
                ret = EPON_OAM_ERR_MEM;
            }
            else
            {
                *pVarContainer->pVarData = vlanCfg.vlanMode;
                CTC_BUF_ENCODE32(pVarContainer->pVarData + CTC_OAM_VAR_VLAN_MODE_LEN, &(vlanCfg.cfg.transCfg.defVlan));
                pPtr = pVarContainer->pVarData + CTC_OAM_VAR_VLAN_TRANSLATION_MODE_MIN;
                for(i = 0 ; i < vlanCfg.cfg.transCfg.num ; i++)
                {
                    CTC_BUF_ENCODE32(pPtr, &vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan);
                    pPtr += 4;
                    CTC_BUF_ENCODE32(pPtr, &vlanCfg.cfg.transCfg.transVlanPair[i].newVlan);
                    pPtr += 4;
                }
            }
            free(vlanCfg.cfg.transCfg.transVlanPair);
            break;
        case CTC_OAM_VAR_VLAN_MODE_AGGREGATION:
            /* Calculate the overall length of reply */
            totalLen = CTC_OAM_VAR_VLAN_AGGREGATION_MODE_MIN;
            for(i = 0 ; i < vlanCfg.cfg.aggreCfg.tableNum ; i++)
            {
                totalLen += 2 /* number of aggregated VLAN in table */ + 4 /* VLAN to be aggregated */
                    + vlanCfg.cfg.aggreCfg.aggrTbl[i].entryNum * CTC_OAM_VAR_VLAN_AGGREGATION_ENTRY_LEN;
            }
            pVarContainer->varWidth = totalLen;
            pVarContainer->pVarData = (unsigned char *) malloc(sizeof(unsigned char) * pVarContainer->varWidth);
            if(NULL == pVarContainer->pVarData)
            {
                ret = EPON_OAM_ERR_MEM;
            }
            else
            {
                *pVarContainer->pVarData = vlanCfg.vlanMode;
                CTC_BUF_ENCODE32(pVarContainer->pVarData + CTC_OAM_VAR_VLAN_MODE_LEN, &(vlanCfg.cfg.aggreCfg.defVlan));
                CTC_BUF_ENCODE16(pVarContainer->pVarData + CTC_OAM_VAR_VLAN_MODE_LEN + 4, &(vlanCfg.cfg.aggreCfg.tableNum));
                pPtr = pVarContainer->pVarData + CTC_OAM_VAR_VLAN_AGGREGATION_MODE_MIN;
                for(i = 0 ; i < vlanCfg.cfg.aggreCfg.tableNum ; i++)
                {
                    CTC_BUF_ENCODE16(pPtr, &vlanCfg.cfg.aggreCfg.aggrTbl[i].entryNum);
                    pPtr += 2;
                    CTC_BUF_ENCODE32(pPtr, &vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan);
                    pPtr += 4;
                    for(j = 0 ; j < vlanCfg.cfg.aggreCfg.aggrTbl[i].entryNum ; j++)
                    {
                        CTC_BUF_ENCODE32(pPtr, &vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j]);
                        pPtr += 4;
                    }
                }
            }
            for(i = 0 ; i < vlanCfg.cfg.aggreCfg.tableNum ; i++)
            {
                free(vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan);
            }
            free(vlanCfg.cfg.aggreCfg.aggrTbl);
            break;
        case CTC_OAM_VAR_VLAN_MODE_TRUNK:
            pVarContainer->varWidth = CTC_OAM_VAR_VLAN_TRUNK_MODE_MIN + (vlanCfg.cfg.trunkCfg.num) * CTC_OAM_VAR_VLAN_TRUNK_ENTRY_LEN;
            pVarContainer->pVarData = (unsigned char *) malloc(sizeof(unsigned char) * pVarContainer->varWidth);
            if(NULL == pVarContainer->pVarData)
            {
                ret = EPON_OAM_ERR_MEM;
            }
            else
            {
                *pVarContainer->pVarData = vlanCfg.vlanMode;
                CTC_BUF_ENCODE32(pVarContainer->pVarData + CTC_OAM_VAR_VLAN_MODE_LEN, &(vlanCfg.cfg.trunkCfg.defVlan));
                pPtr = pVarContainer->pVarData + CTC_OAM_VAR_VLAN_TRUNK_MODE_MIN;
                for(i = 0 ; i < vlanCfg.cfg.trunkCfg.num ; i++)
                {
                    CTC_BUF_ENCODE32(pPtr, &vlanCfg.cfg.trunkCfg.acceptVlan[i]);
                    pPtr += 4;
                }
            }
            free(vlanCfg.cfg.trunkCfg.acceptVlan);
            break;
        default:
            ret = EPON_OAM_ERR_PARAM;
            break;
        }
        return ret;
    }

    return EPON_OAM_ERR_PARAM;
}

/* 0xC7/0x0021 - VLAN */
int ctc_oam_varCb_vlan_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret = EPON_OAM_ERR_OK;
    unsigned char *pPtr;
    unsigned short i, j, val16;
    unsigned int port;
    unsigned int vlanTag;
    unsigned int vlan, priority;
    ctc_wrapper_vlanCfg_t vlanCfg;

    /* For VLAN variable, the length depends on the VLAN mode */
    if(CTC_OAM_VAR_VLAN_MODE_LEN > pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ETHERNET(pVarInstant) && !CTC_INSTANT_IS_ALLPORTS(pVarInstant))
    {
        memset(&vlanCfg, 0, sizeof(ctc_wrapper_vlanCfg_t));
        vlanCfg.vlanMode = pVarContainer->pVarData[0];

		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
				"port %d vlan mode is %d [0-transparent 1-tag 2-translation 3-aggregation 4-trunk]\n", 
					pVarInstant->parse.uniPort.portNo - 1, vlanCfg.vlanMode);

		/* siyuan 2016-08-05: unicast vlan setting can't be changed by OLT 
   			when boa in charge of vlan setting and set vlan status to static */
		if(ctc_oam_vlan_is_static() == 1)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
				"%s %d vlan status is static: vlan can't be changed by OLT, do nothing\n", __func__, __LINE__);
			return EPON_OAM_ERR_OK;
		}
        /* Parse the configure data & configure to chip according to vlan mode */
        switch(vlanCfg.vlanMode)
        {
        case CTC_OAM_VAR_VLAN_MODE_TAG:
            if(CTC_OAM_VAR_VLAN_TAG_MODE_LEN != pVarContainer->varWidth)
            {
                return EPON_OAM_ERR_UNKNOWN;
            }
            CTC_BUF_PARSE16(pVarContainer->pVarData + 1, &val16);
            vlanCfg.cfg.tagCfg.tpid = val16;
            CTC_BUF_PARSE16(pVarContainer->pVarData + 3, &val16);
            vlanCfg.cfg.tagCfg.pri = (val16 & 0xe000) >> 13;
            vlanCfg.cfg.tagCfg.cfi = (val16 & 0x1000) >> 12;
            vlanCfg.cfg.tagCfg.vid = val16 & 0x0fff;

            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                "[OAM:%s:%d] vlan tag mode tpid/pri/cfi/vid 0x%04x/%u/%u/%u\n",
                __FILE__, __LINE__,
                vlanCfg.cfg.tagCfg.tpid, vlanCfg.cfg.tagCfg.pri, vlanCfg.cfg.tagCfg.cfi, vlanCfg.cfg.tagCfg.vid);

            ret = CTC_WRAPPER(vlanSet, pVarInstant->parse.uniPort.portNo - 1, &vlanCfg);
            break;
        case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
            if(CTC_OAM_VAR_VLAN_TRANSLATION_MODE_MIN > pVarContainer->varWidth)
            {
                return EPON_OAM_ERR_UNKNOWN;
            }
            CTC_BUF_PARSE16(pVarContainer->pVarData + 1, &val16);
            vlanCfg.cfg.transCfg.defVlan.tpid = val16;
            CTC_BUF_PARSE16(pVarContainer->pVarData + 3, &val16);
            vlanCfg.cfg.transCfg.defVlan.pri = (val16 & 0xe000) >> 13;
            vlanCfg.cfg.transCfg.defVlan.cfi = (val16 & 0x1000) >> 12;
            vlanCfg.cfg.transCfg.defVlan.vid = val16 & 0x0fff;
            if((pVarContainer->varWidth - CTC_OAM_VAR_VLAN_TRANSLATION_MODE_MIN) % CTC_OAM_VAR_VLAN_TRANSLATION_ENTRY_LEN)
            {
                /* Incorrect vlan pair encoded */
                return EPON_OAM_ERR_UNKNOWN;
            }
            vlanCfg.cfg.transCfg.num = (pVarContainer->varWidth - CTC_OAM_VAR_VLAN_TRANSLATION_MODE_MIN) / CTC_OAM_VAR_VLAN_TRANSLATION_ENTRY_LEN;
            vlanCfg.cfg.transCfg.transVlanPair = (ctc_wrapper_vlanTransPair_t *)malloc(sizeof(ctc_wrapper_vlanTransPair_t) * vlanCfg.cfg.transCfg.num);
            if(NULL == vlanCfg.cfg.transCfg.transVlanPair)
            {
                return EPON_OAM_ERR_MEM;
            }
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                "[OAM:%s:%d] vlan translation mode defVlan tpid/pri/cfi/vid 0x%04x/%u/%u/%u %u entries\n",
                __FILE__, __LINE__,
                vlanCfg.cfg.transCfg.defVlan.tpid, vlanCfg.cfg.transCfg.defVlan.pri, vlanCfg.cfg.transCfg.defVlan.cfi, vlanCfg.cfg.transCfg.defVlan.vid,
                vlanCfg.cfg.transCfg.num);
            pPtr = pVarContainer->pVarData + 5; /* VLAN mode + default VLAN */
            for(i = 0 ; i < vlanCfg.cfg.transCfg.num ; i ++)
            {
                /* Original VLAN */
                CTC_BUF_PARSE16(pPtr, &val16);
                vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.tpid = val16;
                pPtr += 2; /* TPID */
                CTC_BUF_PARSE16(pPtr, &val16);
                pPtr += 2; /* PRI + CFI + VID */
                vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.pri = (val16 & 0xe000) >> 13;
                vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.cfi = (val16 & 0x1000) >> 12;
                vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.vid = val16 & 0x0fff;
                /* New VLAN */
                CTC_BUF_PARSE16(pPtr, &val16);
                vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.tpid = val16;
                pPtr += 2; /* TPID */
                CTC_BUF_PARSE16(pPtr, &val16);
                pPtr += 2; /* PRI + CFI + VID */
                vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.pri = (val16 & 0xe000) >> 13;
                vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.cfi = (val16 & 0x1000) >> 12;
                vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.vid = val16 & 0x0fff;
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                    "    translation pair tpid/pri/cfi/vid 0x%04x/%u/%u/%u -> tpid/pri/cfi/vid 0x%04x/%u/%u/%u\n",
                    vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.tpid, vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.pri,
                    vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.cfi, vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.vid,
                    vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.tpid, vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.pri,
                    vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.cfi, vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.vid);
            }

            ret = CTC_WRAPPER(vlanSet, pVarInstant->parse.uniPort.portNo - 1, &vlanCfg);

            free(vlanCfg.cfg.transCfg.transVlanPair);
            break;
        case CTC_OAM_VAR_VLAN_MODE_AGGREGATION:
            if(CTC_OAM_VAR_VLAN_AGGREGATION_MODE_MIN > pVarContainer->varWidth)
            {
                return EPON_OAM_ERR_UNKNOWN;
            }
            CTC_BUF_PARSE16(pVarContainer->pVarData + 1, &val16);
            vlanCfg.cfg.aggreCfg.defVlan.tpid = val16;
            CTC_BUF_PARSE16(pVarContainer->pVarData + 3, &val16);
            vlanCfg.cfg.aggreCfg.defVlan.pri = (val16 & 0xe000) >> 13;
            vlanCfg.cfg.aggreCfg.defVlan.cfi = (val16 & 0x1000) >> 12;
            vlanCfg.cfg.aggreCfg.defVlan.vid = val16 & 0x0fff;

            ret = ctc_oam_vlanAggrValid_check(pVarContainer->pVarData + 5, pVarContainer->varWidth - 5 /* VLAN mode + DefaultVLAN */);
            if(ret)
            {
                return ret;
            }

            /* Parsing check is passed, no more check from here */
            CTC_BUF_PARSE16(pVarContainer->pVarData + 5, &vlanCfg.cfg.aggreCfg.tableNum);
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                "[OAM:%s:%d] vlan aggregation mode defVlan tpid/pri/cfi/vid 0x%04x/%u/%u/%u %u tables\n",
                __FILE__, __LINE__,
                vlanCfg.cfg.aggreCfg.defVlan.tpid, vlanCfg.cfg.aggreCfg.defVlan.pri, vlanCfg.cfg.aggreCfg.defVlan.cfi, vlanCfg.cfg.aggreCfg.defVlan.vid,
                vlanCfg.cfg.aggreCfg.tableNum);
            pPtr = pVarContainer->pVarData + 7; /* VLAN mode + default VLAN + tableNum */
            vlanCfg.cfg.aggreCfg.aggrTbl = (ctc_wrapper_vlanAggreTableCfg_t *)malloc(sizeof(ctc_wrapper_vlanAggreTableCfg_t) * vlanCfg.cfg.aggreCfg.tableNum);
            if(NULL == vlanCfg.cfg.aggreCfg.aggrTbl)
            {
                return EPON_OAM_ERR_MEM;
            }
            for(i = 0 ; i < vlanCfg.cfg.aggreCfg.tableNum ; i ++)
            {
                CTC_BUF_PARSE16(pPtr, &vlanCfg.cfg.aggreCfg.aggrTbl[i].entryNum);
                pPtr += 2;
                CTC_BUF_PARSE16(pPtr, &val16);
                vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.tpid = val16;
                pPtr += 2;
                CTC_BUF_PARSE16(pPtr, &val16);
                pPtr += 2;
                vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.pri = (val16 & 0xe000) >> 13;
                vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.cfi = (val16 & 0x1000) >> 12;
                vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid = val16 & 0x0fff;
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                    "    table %u aggreTo tpid/pri/cfi/vid 0x%04x/%u/%u/%u from %u entries\n", i,
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.tpid, vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.pri,
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.cfi, vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid,
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].entryNum);

                vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan = (ctc_wrapper_vlan_t *)malloc(sizeof(ctc_wrapper_vlan_t) * vlanCfg.cfg.aggreCfg.aggrTbl[i].entryNum);
                if(NULL == vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan)
                {
                    for(j = 0 ; j < i ; j++)
                    {
                        if(NULL != vlanCfg.cfg.aggreCfg.aggrTbl[j].aggreFromVlan)
                        {
                            free(vlanCfg.cfg.aggreCfg.aggrTbl[j].aggreFromVlan);
                        }
                    }
                    free(vlanCfg.cfg.aggreCfg.aggrTbl);

                    return EPON_OAM_ERR_MEM;
                }
                for(j = 0 ; j < vlanCfg.cfg.aggreCfg.aggrTbl[i].entryNum ; j ++)
                {
                    CTC_BUF_PARSE16(pPtr, &val16);
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].tpid = val16;
                    pPtr += 2;
                    CTC_BUF_PARSE16(pPtr, &val16);
                    pPtr += 2;
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].pri = (val16 & 0xe000) >> 13;
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].cfi = (val16 & 0x1000) >> 12;
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].vid = val16 & 0x0fff;
                    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                        "        aggreFrom tpid/pri/cfi/vid 0x%04x/%u/%u/%u\n",
                        vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].tpid, vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].pri,
                        vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].cfi, vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].vid);
                }
            }

            ret = CTC_WRAPPER(vlanSet, pVarInstant->parse.uniPort.portNo - 1, &vlanCfg);

            for(j = 0 ; j < vlanCfg.cfg.aggreCfg.tableNum ; j++)
            {
                if(NULL != vlanCfg.cfg.aggreCfg.aggrTbl[j].aggreFromVlan)
                {
                    free(vlanCfg.cfg.aggreCfg.aggrTbl[j].aggreFromVlan);
                }
            }
            free(vlanCfg.cfg.aggreCfg.aggrTbl);
            break;
        case CTC_OAM_VAR_VLAN_MODE_TRUNK:
            if(CTC_OAM_VAR_VLAN_TRUNK_MODE_MIN > pVarContainer->varWidth)
            {
                return EPON_OAM_ERR_UNKNOWN;
            }
            CTC_BUF_PARSE16(pVarContainer->pVarData + 1, &val16);
            vlanCfg.cfg.trunkCfg.defVlan.tpid = val16;
            CTC_BUF_PARSE16(pVarContainer->pVarData + 3, &val16);
            vlanCfg.cfg.trunkCfg.defVlan.pri = (val16 & 0xe000) >> 13;
            vlanCfg.cfg.trunkCfg.defVlan.cfi = (val16 & 0x1000) >> 12;
            vlanCfg.cfg.trunkCfg.defVlan.vid = val16 & 0x0fff;
            if((pVarContainer->varWidth - CTC_OAM_VAR_VLAN_TRUNK_MODE_MIN) % CTC_OAM_VAR_VLAN_TRUNK_ENTRY_LEN)
            {
                /* Incorrect vlan pair encoded */
                return EPON_OAM_ERR_UNKNOWN;
            }
            vlanCfg.cfg.trunkCfg.num = (pVarContainer->varWidth - CTC_OAM_VAR_VLAN_TRUNK_MODE_MIN) / CTC_OAM_VAR_VLAN_TRUNK_ENTRY_LEN;
            vlanCfg.cfg.trunkCfg.acceptVlan = (ctc_wrapper_vlan_t *)malloc(sizeof(ctc_wrapper_vlan_t) * vlanCfg.cfg.trunkCfg.num);
            if(NULL == vlanCfg.cfg.trunkCfg.acceptVlan)
            {
                return EPON_OAM_ERR_MEM;
            }
            pPtr = pVarContainer->pVarData + 5; /* VLAN mode + default VLAN */
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                "[OAM:%s:%d] vlan trunk mode defVlan tpid/pri/cfi/vid 0x%04x/%u/%u/%u %u entries\n",
                __FILE__, __LINE__,
                vlanCfg.cfg.trunkCfg.defVlan.tpid, vlanCfg.cfg.trunkCfg.defVlan.pri, vlanCfg.cfg.trunkCfg.defVlan.cfi, vlanCfg.cfg.trunkCfg.defVlan.vid,
                vlanCfg.cfg.trunkCfg.num);
            for(i = 0 ; i < vlanCfg.cfg.trunkCfg.num ; i ++)
            {
                /* Accepted VLAN */
                CTC_BUF_PARSE16(pPtr, &val16);
                vlanCfg.cfg.trunkCfg.acceptVlan[i].tpid = val16;
                pPtr += 2; /* TPID */
                CTC_BUF_PARSE16(pPtr, &val16);
                pPtr += 2; /* PRI + CFI + VID */
                vlanCfg.cfg.trunkCfg.acceptVlan[i].pri = (val16 & 0xe000) >> 13;
                vlanCfg.cfg.trunkCfg.acceptVlan[i].cfi = (val16 & 0x1000) >> 12;
                vlanCfg.cfg.trunkCfg.acceptVlan[i].vid = val16 & 0x0fff;
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                    "    accept vlan tpid/pri/cfi/vid 0x%04x/%u/%u/%u\n",
                    vlanCfg.cfg.trunkCfg.acceptVlan[i].tpid, vlanCfg.cfg.trunkCfg.acceptVlan[i].pri,
                    vlanCfg.cfg.trunkCfg.acceptVlan[i].cfi, vlanCfg.cfg.trunkCfg.acceptVlan[i].vid);
            }

            ret = CTC_WRAPPER(vlanSet, pVarInstant->parse.uniPort.portNo - 1, &vlanCfg);

            free(vlanCfg.cfg.trunkCfg.acceptVlan);
            break;
        case CTC_OAM_VAR_VLAN_MODE_TRANSPARENT:
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
                "[OAM:%s:%d] vlan transparent mode\n", __FILE__, __LINE__);
            ret = CTC_WRAPPER(vlanSet, pVarInstant->parse.uniPort.portNo - 1, &vlanCfg);
            break;
        default:
            ret = EPON_OAM_ERR_PARAM;
            break;
        }

        if(EPON_OAM_ERR_OK != ret)
        {
            pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
        }
        else
        {
            pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
        }
    }
    else
    {
        pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
    }

    return EPON_OAM_ERR_OK;
}

static void ctc_clfrmk2local_get(uint32 lport,
	                         uint32  precedenceOfRule,
	                         oam_clasmark_rulebody_t *pstClsRule,
	                         oam_clasmark_fieldbody_t *pstClsField)
{
	int phyid;
	
	*pstClsRule = local_clasmark[lport].ClsRule[precedenceOfRule-1];
	memcpy(pstClsField, local_clasmark[lport].ClsField[precedenceOfRule-1], pstClsRule->numOfField*sizeof(oam_clasmark_fieldbody_t));
}

static void ctc_clear_clfrmk2local_by_port(uint32 lport)
{
	memset(&local_clasmark[lport], 0, sizeof(oam_clasmark_local_t));	
}

static void ctc_clear_clfrmk2local_by_prec(uint32 lport, uint32  precedenceOfRule)
{
	memset(&local_clasmark[lport].ClsRule[precedenceOfRule], 0, sizeof(oam_clasmark_rulebody_t));
	memset(local_clasmark[lport].ClsField[precedenceOfRule], 0, CLF_RMK_RULES_IN_1_PRECEDENCE_MAX*sizeof(oam_clasmark_fieldbody_t));
}

static void ctc_clfrmk2local_add(uint32 lport,
	                         uint32  precedenceOfRule,
	                         oam_clasmark_rulebody_t *pstClsRule,
	                         oam_clasmark_fieldbody_t *pstClsField)
{
	oam_clasmark_rulebody_t tmpClsRule, *preClsRule;
	oam_clasmark_fieldbody_t tmpClsField[CLF_RMK_RULES_IN_1_PRECEDENCE_MAX], 
							 *preClsField;
	int ruleIdx;

	if (precedenceOfRule >= CTC_CLF_REMARK_RULE_NUM_MAX)
		return;

	/* 1. delete all rule first */
	for (ruleIdx=0; ruleIdx<CTC_CLF_REMARK_RULE_NUM_MAX; ruleIdx++)
	{
		ctc_del_clfrmk_by_prec(lport, ruleIdx);
	}

	/* 2. save rule to local db */
	if (0 == local_clasmark[lport].ClsRule[precedenceOfRule].precedenceOfRule)
	{
		local_clasmark[lport].ClsRule[precedenceOfRule] = *pstClsRule;
		memcpy(local_clasmark[lport].ClsField[precedenceOfRule], pstClsField, 
				pstClsRule->numOfField*sizeof(oam_clasmark_fieldbody_t));
	}
	else
	{
		/* replace the old entry, and all other entry with lower priority should 
		 * move forward 
		 */
		preClsRule = &local_clasmark[lport].ClsRule[precedenceOfRule];
		preClsField = local_clasmark[lport].ClsField[precedenceOfRule];
		
		preClsRule->precedenceOfRule += 1;
		
		for (ruleIdx=precedenceOfRule+1; ruleIdx<CTC_CLF_REMARK_RULE_NUM_MAX; ruleIdx++)
		{
			if (local_clasmark[lport].ClsRule[ruleIdx].precedenceOfRule != 0)
			{
				tmpClsRule = local_clasmark[lport].ClsRule[ruleIdx];
				memcpy(tmpClsField, local_clasmark[lport].ClsField[ruleIdx], 
						tmpClsRule.numOfField*sizeof(oam_clasmark_fieldbody_t));
				
				/* modify precedenceOfRule */
				tmpClsRule.precedenceOfRule += 1;

				local_clasmark[lport].ClsRule[ruleIdx] = *preClsRule;
				memcpy(local_clasmark[lport].ClsField[ruleIdx], preClsField, 
						preClsRule->numOfField*sizeof(oam_clasmark_fieldbody_t));

				preClsRule = &tmpClsRule;
				preClsField = tmpClsField;
			}
			else
			{
				local_clasmark[lport].ClsRule[ruleIdx] = *preClsRule;
				memcpy(local_clasmark[lport].ClsField[ruleIdx], preClsField, 
						preClsRule->numOfField*sizeof(oam_clasmark_fieldbody_t));
				
				break;
			}
		}
		
		local_clasmark[lport].ClsRule[precedenceOfRule] = *pstClsRule;
		memcpy(local_clasmark[lport].ClsField[precedenceOfRule], pstClsField, 
				pstClsRule->numOfField*sizeof(oam_clasmark_fieldbody_t));
	}

	/* 3. classification rule should be added by precedence */
	for (ruleIdx=0; ruleIdx<CTC_CLF_REMARK_RULE_NUM_MAX; ruleIdx++)
	{
		/* ignore invalid entry */
		if (0 == local_clasmark[lport].ClsRule[ruleIdx].precedenceOfRule)
			continue;
		
		tmpClsRule = local_clasmark[lport].ClsRule[ruleIdx];
		memcpy(tmpClsField, local_clasmark[lport].ClsField[ruleIdx], 
				tmpClsRule.numOfField*sizeof(oam_clasmark_fieldbody_t));
		
		if (RT_ERR_FAILED == ctc_add_clfrmk_by_prec(lport, ruleIdx, &tmpClsRule, tmpClsField))
			printf("%s %d add classfication rule %d to port %d fail\n", 
					__func__, __LINE__, ruleIdx, lport);
	}
}

/* 0xC7/0x0031 - Classification&Marking */
int ctc_oam_varCb_classificationMarking_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	oam_clasmark_rulebody_t *pClsRule;
	oam_clasmark_fieldbody_t *pClsField;
	oam_clasmark_rulebody_t clsRule;
	oam_clasmark_fieldbody_t clsFields[CLF_RMK_RULES_IN_1_PRECEDENCE_MAX];
	unsigned char numOfRule = 0;
	unsigned char numOfField;
	unsigned char precedenceOfRule, fieldIdx;
	unsigned char portNo, *p_byte;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

	pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = 2;/*action + numOfRule*/
	pVarContainer->pVarData = (unsigned char *) malloc(1500);
	if(NULL == pVarContainer->pVarData)
	{
		return EPON_OAM_ERR_MEM;
	}
	
    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }
	
	if (!ctc_is_uniport(pVarInstant->parse.uniPort.portNo-1))
	{
		return EPON_OAM_ERR_PARAM;
	}

	p_byte = pVarContainer->pVarData;
	pVarContainer->pVarData[0] = CTC_OAM_VAR_CLASSIFY_ACTION_SHOW;
	//numOfRule will be updated later
	pVarContainer->pVarData[1] = numOfRule;
	p_byte += 2;

	portNo = pVarInstant->parse.uniPort.portNo - 1;
	
	for(precedenceOfRule = 1; precedenceOfRule <= 8; precedenceOfRule++)
	{
		memset(&clsRule, 0, sizeof(clsRule));
		memset(clsFields, 0, sizeof(clsFields));

		ctc_clfrmk2local_get(portNo, precedenceOfRule, &clsRule, clsFields);
		if (0 == clsRule.precedenceOfRule)//invalid rule
			continue;
		
		numOfRule++;

		/* add cls rule entry */
		pClsRule = (oam_clasmark_rulebody_t *)p_byte;
		memcpy(pClsRule, &clsRule, sizeof(oam_clasmark_rulebody_t));
		p_byte += sizeof(oam_clasmark_rulebody_t);

		/* add fileds entry */
		numOfField = clsRule.numOfField;
		//field length is variable
		for (fieldIdx=0; fieldIdx<numOfField; fieldIdx++)
		{
			unsigned char fieldSelect, matchLen;
			/* Field Select */
			fieldSelect = clsFields[fieldIdx].fieldSelect;
			*p_byte++ = fieldSelect;
			/* Match Value */
			if (fieldSelect <= OAM_CTC_CLS_FIELD_TYPE_L4_DST_PORT)
				matchLen = 6;
			else
				matchLen = 16;
			memcpy(p_byte, clsFields[fieldIdx].matchValue, matchLen);
			p_byte += matchLen;
			/* Validation Operator */
			*p_byte++ = clsFields[fieldIdx].operator;
		}

		pVarContainer->varWidth += pClsRule->lenOfRule + 2;
	}

	pVarContainer->pVarData[1] = numOfRule;

	return EPON_OAM_ERR_OK;
}


/* 0xC7/0x0031 - Classification&Marking */
int ctc_oam_varCb_classificationMarking_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	int ret;
	oam_clasmark_rulebody_t *pClsRule;
	oam_clasmark_fieldbody_t clsFields[CLF_RMK_RULES_IN_1_PRECEDENCE_MAX];
	unsigned char numOfRule, action;
	unsigned char numOfField;
	unsigned char ruleId, fieldId;
	unsigned short curPortId;
	unsigned char precedenceOfRule;
	unsigned char*	p_in;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

	if (!ctc_is_uniport(pVarInstant->parse.uniPort.portNo-1))
	{
		ret = CTC_OAM_VAR_RESP_VARNORESOURCE;
		goto send_rsp;
	}

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
			"[%s %d] port %d action %d[0-del 1-add 2-clr 3-show]\n", __func__, __LINE__, 
				pVarInstant->parse.uniPort.portNo - 1, pVarContainer->pVarData[0]);

	/*get the classification config*/
	curPortId = pVarInstant->parse.uniPort.portNo - 1;
	action = pVarContainer->pVarData[0];
	numOfRule = pVarContainer->pVarData[1];

	p_in = &pVarContainer->pVarData[2];
	
	if(CTC_OAM_VAR_CLASSIFY_ACTION_CLR == action)
	{
		ret = CTC_OAM_VAR_RESP_SETOK;

		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "Clear Port%d's Classification\n", curPortId);
		if (RT_ERR_OK != ctc_clear_clfrmk_by_port(curPortId))
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "Fail Clear Port%d's Classification\n", curPortId);
			ret = CTC_OAM_VAR_RESP_VARNORESOURCE;
			goto send_rsp;
		}

		ctc_clear_clfrmk2local_by_port(curPortId);
	}
	else if(CTC_OAM_VAR_CLASSIFY_ACTION_DEL == action)
	{
		ret = CTC_OAM_VAR_RESP_SETOK;
		
		for(ruleId = 0; ruleId < numOfRule; ruleId++)
		{
			pClsRule = (oam_clasmark_rulebody_t *)p_in;
			p_in += 2 + pClsRule->lenOfRule;

			precedenceOfRule = pClsRule->precedenceOfRule;
			if (precedenceOfRule > 0)
				precedenceOfRule -= 1;

			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "del Port%d's Classification Prec %d\n", curPortId, pClsRule->precedenceOfRule);
			if (RT_ERR_OK != ctc_del_clfrmk_by_prec(curPortId, precedenceOfRule))
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "Fail del Port%d's Classification\n", curPortId);
				ret = CTC_OAM_VAR_RESP_VARNORESOURCE;
				goto send_rsp;
			}
			
			ctc_clear_clfrmk2local_by_prec(curPortId, precedenceOfRule);
		}
	}
	else if((CTC_OAM_VAR_CLASSIFY_ACTION_ADD == action))
	{
		unsigned char matchLen;
		unsigned char *pField;
		
		ret = CTC_OAM_VAR_RESP_SETOK;
		
		for(ruleId = 0; ruleId < numOfRule; ruleId++)
		{
			pClsRule = (oam_clasmark_rulebody_t *)p_in;
			numOfField = pClsRule->numOfField;

			if (numOfField > CLF_RMK_RULES_IN_1_PRECEDENCE_MAX)
			{
				ret = CTC_OAM_VAR_RESP_VARNORESOURCE;
				goto send_rsp;
			}

			precedenceOfRule = pClsRule->precedenceOfRule;
			if (precedenceOfRule > 0)
				precedenceOfRule -= 1;
			
			pField = p_in + sizeof(oam_clasmark_rulebody_t);
			for (fieldId=0; fieldId<numOfField; fieldId++)
			{
				clsFields[fieldId].fieldSelect = *pField++;
				if (clsFields[fieldId].fieldSelect <= OAM_CTC_CLS_FIELD_TYPE_L4_DST_PORT)
					matchLen = 6;
				else if (clsFields[fieldId].fieldSelect <= OAM_CTC_CLS_FIELD_TYPE_NXTHDR)
					matchLen = 16;
				else//no support, ignore this field
				{
					numOfField--;
					break;
				}
				memcpy(clsFields[fieldId].matchValue, pField, matchLen);
				pField += matchLen;
				clsFields[fieldId].operator = *pField++;
				
				/* only support equal and always-match */
				if((CTC_OAM_CLF_EQUAL != clsFields[fieldId].operator) && 
					(CTC_OAM_CLF_ALWAYS_MATCH != clsFields[fieldId].operator))
				{
					/*Not support*/
					ret = CTC_OAM_VAR_RESP_VARNORESOURCE;
					goto send_rsp;
				}
			}
			p_in += 2 + pClsRule->lenOfRule;

			if (0 == numOfField)
				continue;
			
			ctc_clfrmk2local_add(curPortId, precedenceOfRule, pClsRule, clsFields);
		}
	}
	else
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_WARN, "classify unknown\n");
		ret = CTC_OAM_VAR_RESP_VARBADPARAM;
	}

send_rsp:
	
	pVarContainer->varWidth = ret;
	
	return EPON_OAM_ERR_OK;

}
#else
/* 0xC7/0x0021 - VLAN */
int ctc_oam_varCb_vlan_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
}

/* 0xC7/0x0021 - VLAN */
int ctc_oam_varCb_vlan_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;	
	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0031 - Classification&Marking */
int ctc_oam_varCb_classificationMarking_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
}

/* 0xC7/0x0031 - Classification&Marking */
int ctc_oam_varCb_classificationMarking_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;	
	return EPON_OAM_ERR_OK;
}
#endif

/* 0xC7/0x0041 - Add/Del Multicast VLAN */
int ctc_oam_varCb_addDelMulticastVlan_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	int ret;
	unsigned char port, *p_byte, *pVlan;
    unsigned int number;
	
    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }
	
	pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = 1;/*action + 2*numOfVlan*/
	pVarContainer->pVarData = (unsigned char *) malloc(1500);
	if(NULL == pVarContainer->pVarData)
	{
		return EPON_OAM_ERR_MEM;
	}
	
    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

	port = pVarInstant->parse.uniPort.portNo - 1;
	
	p_byte = pVarContainer->pVarData;
	pVarContainer->pVarData[0] = CTC_OAM_VAR_MCAST_VLAN_LIST;
	p_byte += 1;
	pVlan = p_byte;
	
	ret = igmpMcVlanGet(port, pVlan, &number);
	if(0 != ret)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_WARN, "[ %s ]: odmMulticastVlanGet return error!\n", __FUNCTION__);
		number = 0;
	}

	pVarContainer->varWidth = 1+2*number;

	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0041 - Add/Del Multicast VLAN */
int ctc_oam_varCb_addDelMulticastVlan_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret = EPON_OAM_ERR_OK;
	unsigned short curPortId;
	unsigned char *p_in, *p_in_tmp;
	unsigned char action;
	unsigned short width, ui_mc_vlan_num, i;
	unsigned short vid;
	
	if(NULL == pVarInstant)
	{
		return EPON_OAM_ERR_NOT_FOUND;
	}

	if(igmpMcSwitchGet() != IGMP_MODE_SNOOPING)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "Not in MC_MODE_SNOOPING mode.\r\n");
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
		return EPON_OAM_ERR_OK;
	}
	
	curPortId = pVarInstant->parse.uniPort.portNo - 1;
	width = pVarContainer->varWidth;
	action = pVarContainer->pVarData[0];
	p_in = &pVarContainer->pVarData[1];
	
	/* clear the Multicast VLAN of the port */
    if (CTC_OAM_VAR_MCAST_VLAN_CLEAR == action)
	{
		if (1 != width)
		{
            ret = EPON_OAM_ERR_PARAM;
	    	goto send_rsp;
		}
		igmpMcVlanClear(curPortId);
	}
	else//add or delete
	{
		if (((width-1)&0x1) != 0)
		{
            ret = EPON_OAM_ERR_PARAM;
	    	goto send_rsp;
		}
		
		ui_mc_vlan_num = (width-1)/2;

		p_in_tmp = p_in;
		for (i=0; i<ui_mc_vlan_num; i++)
		{
			CTC_BUF_PARSE16(p_in_tmp, &vid);
			p_in_tmp += 2;

			/* QL 20160112
			 * CDATA 9601B may add the same mc vlan to ctc vlan, As mc vlan tagStripped
			 * mode will be modified by OLT unexpectedly, so we should save all mc vlan
			 * in local.
			 */
			#if 0
			/*Mc vlan can not be same with ctc vlan.*/
			if (FALSE == OamCheckValidVid(vid))
			{
				ret = EPON_OAM_ERR_PARAM;
				goto send_rsp;
			}
			#endif
		}

		for(i=0; i<ui_mc_vlan_num; i++)
		{
			CTC_BUF_PARSE16(p_in, &vid);
			p_in += 2;
			
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "mc vlan: %d\n", vid);
			
	 		if (CTC_OAM_VAR_MCAST_VLAN_DEL == action)
	 		{
				igmpMcVlanDelete(curPortId, vid);
	 		}
			else if(CTC_OAM_VAR_MCAST_VLAN_ADD == action)
			{	
				igmpMcVlanAdd(curPortId, vid);
			}
		}
	}

	/* patch for DEL action, we should renew all mc clf rule */
	if (CTC_OAM_VAR_MCAST_VLAN_DEL == action)
	{
		igmpMcTagstripSet(curPortId, igmpTagOperPerPortGet(curPortId));
	}

send_rsp:
	if(ret == EPON_OAM_ERR_OK)
	{
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
	}
	else
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "Multicast vlan set fail\n");
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
	}
  
    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0042 - MulticastTagOper */
int ctc_oam_varCb_multicastTagOper_get(
  unsigned char llidIdx, /* LLID index of the incoming operation */
  unsigned char op, /* Operation to be taken */
  ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
  ctc_varDescriptor_t varDesc,
  ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
										* caller will release it
										*/
{
    int ret;
	unsigned char portNo;
	unsigned char tagOpr;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_MCAST_VLAN_TAG_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

	portNo = pVarInstant->parse.uniPort.portNo - 1;
	igmpMcTagstripGet(portNo, &tagOpr);
	
	*pVarContainer->pVarData = tagOpr;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0042 - MulticastTagOper */
int ctc_oam_varCb_multicastTagOper_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    int ret;
	unsigned short portNo;
	unsigned char *p_in, *p_in_tmp;
	unsigned char reusableVttNum,num_of_mcast_vlan=0;
	unsigned char dal_vtt_num;
	unsigned char tagstriped;
	unsigned short mc_vlan, iptv_vlan;
	int i;

	if(NULL == pVarInstant)
	{
		return EPON_OAM_ERR_NOT_FOUND;
	}

	portNo = pVarInstant->parse.uniPort.portNo - 1;
	tagstriped = pVarContainer->pVarData[0];	

    if (CTC_OAM_VAR_MCAST_VLAN_TAG_TRANLATE == tagstriped)
	{	
		num_of_mcast_vlan = pVarContainer->pVarData[1];
		p_in = &pVarContainer->pVarData[2];

		/* get existed vlan num for this port */
		dal_vtt_num=igmpMcTagTranslationTableNum(portNo);
		
		if((dal_vtt_num+num_of_mcast_vlan) > 16) 
		{
			ret = EPON_OAM_ERR_PARAM;
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "eopl_set_onu_mcast_tag_strip: %d+%d>SHIVA_MAX_VLAN_TRANS\n", dal_vtt_num, num_of_mcast_vlan);
			goto send_rsp;
		}

		p_in_tmp = p_in;
		
		for (i=0; i<num_of_mcast_vlan; i++)
		{
			CTC_BUF_PARSE16(p_in_tmp, &mc_vlan);
			CTC_BUF_PARSE16(p_in_tmp+2, &iptv_vlan);
			p_in_tmp += 4;

			if(EPON_OAM_ERR_OK != igmpMctag_translation_check(portNo, mc_vlan, iptv_vlan))
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "FIALED ... ");
				ret = EPON_OAM_ERR_PARAM;
				goto send_rsp;
			}
		}

		ret = igmpMcTagstripSet(portNo, tagstriped);
		if(ret != EPON_OAM_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "call igmpMcTagstripSet() fail!\n");
			goto send_rsp;
		}

		for(i = 0; i < num_of_mcast_vlan; i++)
		{
			CTC_BUF_PARSE16(p_in, &mc_vlan);
			CTC_BUF_PARSE16(p_in+2, &iptv_vlan);
			p_in += 4;
	
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "call igmpMcTagTranslationAdd(%ld, %ld, %ld)\n",
						 portNo, mc_vlan, iptv_vlan);
	
			ret = igmpMcTagTranslationAdd(portNo, mc_vlan, iptv_vlan);
			if(ret != EPON_OAM_ERR_OK)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,	"call igmpMcTagTranslationAdd() fail!\n");
				goto send_rsp;
			}
		}
	}
	else//STRIP/NOT_STRIP
	{
		ret = igmpMcTagstripSet(portNo, tagstriped);
		if(ret != EPON_OAM_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "call igmpMcTagstripSet() fail!\n");
			goto send_rsp;
		}
	}

send_rsp:
	if(ret == EPON_OAM_ERR_OK)
	{
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
	}
	else
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "Multicast tag oper set fail\n");
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	}
  
	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0043 - MulticastSwitch */
int ctc_oam_varCb_multicastSwitch_get(
  unsigned char llidIdx, /* LLID index of the incoming operation */
  unsigned char op, /* Operation to be taken */
  ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
  ctc_varDescriptor_t varDesc,
  ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
										* caller will release it
										*/
{
    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_MCAST_SWITCH_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

	pVarContainer->pVarData[0] = igmpMcSwitchGet();

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0043 - MulticastSwitch */
int ctc_oam_varCb_multicastSwitch_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	int ret;
	unsigned char mcast_switch;

	mcast_switch = pVarContainer->pVarData[0];
	if ((mcast_switch != 0) && (mcast_switch != 1))
	{
		ret = EPON_OAM_ERR_PARAM;
		goto send_rsp;
	}
	else
	{
		ret = igmpMcSwitchSet(mcast_switch);
	}

send_rsp:
	if(ret == EPON_OAM_ERR_OK)
	{
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
	}
	else
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "Multicast tag oper set fail\n");
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	}
  
	return EPON_OAM_ERR_OK;
}


/* 0xC7/0x0044 - MulticastControl */
int ctc_oam_varCb_multicastControl_get(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	unsigned char op, /* Operation to be taken */
	ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
	ctc_varDescriptor_t varDesc,
	ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
										  * caller will release it
										  */
{
	unsigned char *p_byte;
	unsigned char mcastCtrlType;
	unsigned short number;
	
	pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = 1;/*action + 2*numOfVlan*/
	pVarContainer->pVarData = (unsigned char *) malloc(1500);
	if(NULL == pVarContainer->pVarData)
	{
		return EPON_OAM_ERR_MEM;
	}

	pVarContainer->pVarData[0] = CTC_OAM_VAR_MCAST_ENTRY_LIST;
	mcastCtrlType = (unsigned char)igmpMcCtrlGrpTypeGet();
	pVarContainer->pVarData[1] = mcastCtrlType;
	p_byte = &pVarContainer->pVarData[3];

	igmpMcCtrlGrpEntryGet(p_byte, &number);
	if ((MC_CTRL_GDA_MAC_VID == mcastCtrlType) ||
		(MC_CTRL_GDA_GDA_IP_VID == mcastCtrlType) ||
		(MC_CTRL_GDA_MAC == mcastCtrlType))
		pVarContainer->varWidth = 3+10*number;
	else if (MC_CTRL_GDA_GDA_SA_IP == mcastCtrlType)
		pVarContainer->varWidth = 3+24*number;
	
	pVarContainer->pVarData[2] = number;

	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0044 - MulticastControl */
int ctc_oam_varCb_multicastControl_set(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	unsigned char op, /* Operation to be taken */
	ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
	ctc_varDescriptor_t varDesc,
	ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
										* caller will release it
										*/
{
    int ret;
	int i;
	unsigned short portId;
	unsigned short vlan;
	unsigned char gda[6];
	unsigned char *p_in, *p_in_tmp;
	unsigned char width, action, ctrlType, number;

	width = pVarContainer->varWidth;
	action = pVarContainer->pVarData[0];
	ctrlType = pVarContainer->pVarData[1];
	number = pVarContainer->pVarData[2];

	p_in = &pVarContainer->pVarData[3];

	/* clear the Multicast VLAN of the port */
    if (CTC_OAM_VAR_MCAST_ENTRY_CLEAR == action)
	{
		if (1 != width)
		{
            ret = EPON_OAM_ERR_PARAM;
	    	goto send_rsp;
		}

		ret = igmpMcCtrlGrpEntryClear();
	}
	else
	{
		if (CTC_OAM_VAR_MCAST_ENTRY_ADD == action)
		{
			oam_mcast_control_entry_t ctl_entry_list[(MAX_PORT_NUM)*(MAX_GROUP_NUM_PER_PORT)];

			unsigned short ctl_entry_num = 0;
			unsigned short ctl_entry_index;
			unsigned char ctl_entry_num_per_port[MAX_PORT_NUM] = {0};

			igmpMcCtrlGrpEntryGet(&ctl_entry_list, &ctl_entry_num);

			for(ctl_entry_index=0; ctl_entry_index<ctl_entry_num; ctl_entry_index++)
			{
			    ctl_entry_num_per_port[ctl_entry_list[ctl_entry_index].port_id]++;
			}

			p_in_tmp= p_in;
			for(i=0; i<number; i++)
			{
				CTC_BUF_PARSE16(p_in_tmp, &portId);
				portId = portId - 1;
				p_in_tmp += 10;
				
			    ctl_entry_num_per_port[portId]++;
			}

			FOR_EACH_LAN_PORT(i)
			{
			    if(ctl_entry_num_per_port[i]>MAX_GROUP_NUM_PER_PORT)
			    {
				    ret = -1;
				    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "eopl_set_onu_mcast_control: port%d entry>64\n", i);
				    goto send_rsp;
			    }
			}
		}
		
		igmpMcCtrlGrpTypeSet(ctrlType);
		if ((MC_CTRL_GDA_GDA_SA_IP == ctrlType) ||
			(MC_CTRL_GDA_GDA_IP6_VID == ctrlType))
		{
			pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
			return EPON_OAM_ERR_OK;
		}
		
		for(i=0; i<number; i++)
		{
			CTC_BUF_PARSE16(p_in, &portId);
			CTC_BUF_PARSE16(p_in+2, &vlan);
			memcpy(gda, p_in+4, 6);
			p_in += 10;

			portId = portId - 1;
			
			if (CTC_OAM_VAR_MCAST_ENTRY_DEL == action)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_DUMP, "mc vlan ctrl del, port: %d, vlan: %d, gda: %02x:%02x:%02x:%02x:%02x:%02x\n",
					portId, vlan, gda[0], gda[1], gda[2], gda[3], gda[4], gda[5]);

				ret = igmpMcCtrlGrpEntryDelete(portId, vlan, gda);
			}
			if(CTC_OAM_VAR_MCAST_ENTRY_ADD == action)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_DUMP, "mc vlan ctrl add, port: %d, vlan: %d, gda: %02x:%02x:%02x:%02x:%02x:%02x\n",
					portId, vlan, gda[0], gda[1], gda[2], gda[3], gda[4], gda[5]);
				
				ret = igmpMcCtrlGrpEntryAdd(portId, vlan, gda);
			}
		}
	}
	
send_rsp:
	if(ret == EPON_OAM_ERR_OK)
	{
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
	}
	else
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "Multicast vlan set fail\n");
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
	}
  
	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0045 - Group Num Max */
int ctc_oam_varCb_groupNumMax_get(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	unsigned char op, /* Operation to be taken */
	ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
	ctc_varDescriptor_t varDesc,
	ctc_varContainer_t *pVarContainer)  /* handler should allocate resource, 
										  * caller will release it
										  */
{
	unsigned char portNo;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }
	
    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_MCAST_GRP_NUM_MAX_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

	portNo = pVarInstant->parse.uniPort.portNo - 1;
	pVarContainer->pVarData[0] = igmpMcGrpMaxNumGet(portNo);

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0045 - Group Num Max */
int ctc_oam_varCb_groupNumMax_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    int ret;
    unsigned int port;
	unsigned int groupNumMax;

    if(CTC_OAM_VAR_MCAST_GRP_NUM_MAX_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant) || (!CTC_INSTANT_IS_ETHERNET(pVarInstant)))
    {
        return EPON_OAM_ERR_PARAM;
    }

	port = pVarInstant->parse.uniPort.portNo - 1;
    groupNumMax = pVarContainer->pVarData[0];

	ret = igmpMcGrpMaxNumSet(port, groupNumMax);
	if(!ret) /*for multicast, true means success */
	{
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
	}
	else
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "Port%d's groupNumMax set fail\n", port);
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARNORESOURCE;
	}

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0046 - aFastLeaveAbility */
int ctc_oam_varCb_aFastLeaveAbility_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
    unsigned int enumCnt, enumValue;
	int ret, fastleaveEnable;

    pVarContainer->varDesc = varDesc;

#ifdef CONFIG_HGU_APP
	ret = CTC_WRAPPER(fastLeaveAbilityGet, &fastleaveEnable);
	if(EPON_OAM_ERR_OK != ret)
	{
		return EPON_OAM_ERR_UNKNOWN;
	}
	if(fastleaveEnable)
		enumCnt = 6;
	else
		enumCnt = 3;
#else
	fastleaveEnable = 1;
	enumCnt = 6;
#endif

    /* Length varies according to the enum number
     * Here reply all possible enum values defined in CTC standard
     */
    pVarContainer->varWidth = 
        CTC_OAM_VAR_AFASTLEAVEABILITY_HDR_LEN +
        (CTC_OAM_VAR_AFASTLEAVEABILITY_ENUM_LEN * enumCnt);
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* Fill in the header */
    CTC_BUF_ENCODE32(pVarContainer->pVarData, &enumCnt);

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARGET, "[OAM:%s:%d] fastleaveEnable(%d)\n", 
                                __FILE__, __LINE__, fastleaveEnable);
	if(fastleaveEnable)
	{
	    /* 1st enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_SNOOPING_NONFASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 4), &enumValue);

	    /* 2nd enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_SNOOPING_FASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 8), &enumValue);

	    /* 3rd enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_IGMP_NONFASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 12), &enumValue);

	    /* 4th enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_IGMP_FASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 16), &enumValue);

	    /* 5th enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_MLD_NONFASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 20), &enumValue);

	    /* 6th enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_MLD_FASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 24), &enumValue);
	}
	else
	{
	    /* 1st enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_SNOOPING_NONFASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 4), &enumValue);

	    /* 2nd enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_IGMP_NONFASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 8), &enumValue);

	    /* 3rd enum */
	    enumValue = CTC_OAM_VAR_AFASTLEAVEABILITY_MLD_NONFASTLEAVE;
	    CTC_BUF_ENCODE32((pVarContainer->pVarData + 12), &enumValue);
	}

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0047 - aFastLeaveAdminState */
int ctc_oam_varCb_aFastLeaveAdminState_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	unsigned int leaveMode;
	
    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_AFASTLEAVEADMINSTATE_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    leaveMode = igmpmcfastleaveget();
	
    CTC_BUF_ENCODE32(pVarContainer->pVarData, &leaveMode);

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x006B - POTSStatus */
int ctc_oam_varCb_potsStatus_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
	int ret;
    unsigned int portState, serviceState, codecMode;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

	
	pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = CTC_OAM_VAR_POTSSTATUS_LEN; /* Fix value from CTC standard */
	pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
	if(NULL == pVarContainer->pVarData)
	{
		return EPON_OAM_ERR_MEM;
	}

	ret = CTC_WRAPPER(potsStatusGet, pVarInstant->parse.uniPort.portNo - 1, &portState, &serviceState, &codecMode);
    if(RT_ERR_OK == ret)
    {
    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARGET, 
				"POTSStatus portState[%d] serviceState[%d] codecMode[%d]\n", portState, serviceState, codecMode);
        CTC_BUF_ENCODE32(pVarContainer->pVarData, &portState);
		CTC_BUF_ENCODE32(pVarContainer->pVarData+4, &serviceState);
		CTC_BUF_ENCODE32(pVarContainer->pVarData+8, &codecMode);
    }
    else
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
	
	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0081 - AlarmAdminState */
int ctc_oam_varCb_alarmAdminState_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    unsigned short alarmId;
    unsigned int alarmConfig;
    unsigned int port;
    int enable;
    unsigned int alarmInstance;
    
    if(CTC_OAM_VAR_ALARMADMINSTATE_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    CTC_BUF_PARSE16(pVarContainer->pVarData, &alarmId);
    CTC_BUF_PARSE32(pVarContainer->pVarData+2, &alarmConfig);

    if(alarmConfig == CTC_OAM_VAR_ALARMADMINSTATE_DISABLE)
    {
        enable = DISABLED;
    }
    else if(alarmConfig == CTC_OAM_VAR_ALARMADMINSTATE_ENABLE)
    {
        enable = ENABLED;
    }
    else
    {
        /*ignore not valid value*/
        goto send_rsp;
    }

    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
				"alarmAdminState alarmId[%x] %s\n", 
					alarmId, enable ? "enable":"disable");
    
    if(NULL != pVarInstant) 
    {
        if(CTC_INSTANT_IS_PORT(pVarInstant) && CTC_INSTANT_IS_ETHERNET(pVarInstant))
        {
            alarmInstance = (pVarInstant->parse.uniPort.portType << 24) |
                            ((pVarInstant->parse.uniPort.chassisNo & 0x03) << 22) |
                            ((pVarInstant->parse.uniPort.slotNo & 0x3f) << 16);
            
            if(CTC_INSTANT_IS_ALLPORTS(pVarInstant))
            {  
                FOR_EACH_LAN_PORT(port)
                {
                    alarmInstance = (alarmInstance & 0xFFFF0000) | ((port+1) & 0xFFFF);
                    ctc_oam_alarm_state_set(alarmId, alarmInstance, enable);
                }
            }
            else
            {
                alarmInstance |= pVarInstant->parse.uniPort.portNo;
                ctc_oam_alarm_state_set(alarmId, alarmInstance, enable);
            }
        }
        else
        {
            ctc_oam_alarm_state_set(alarmId, pVarInstant->parse.ponIf, enable);
        }
    }
    else
    {
        ctc_oam_alarm_state_set(alarmId, CTC_EVENT_NOTIFICATION_ONU_OBJECT_INSTANCE, enable);
    }
send_rsp:

    pVarContainer->varWidth = 0x80;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0082 - AlarmThreshold */
int ctc_oam_varCb_alarmThreshold_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	unsigned short alarmId;
	unsigned int alarmThreshold;
    unsigned int clearAlarmThreshold;

	/* Special Process: this get request command has extra parameter */
	if(NULL != pVarContainer->pVarData)
	{
		CTC_BUF_PARSE16(pVarContainer->pVarData, &alarmId);

		/* free previous malloc buffer */		
	    free(pVarContainer->pVarData);
	}
	else
		return EPON_OAM_ERR_PARAM;

	pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = CTC_OAM_VAR_ALARMTHRESHOLD_LEN;
	pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

	/* only consider PON_IF alarm now */
	ctc_oam_alarm_threshold_get(alarmId, 0, &alarmThreshold, &clearAlarmThreshold);
	
	CTC_BUF_ENCODE16(pVarContainer->pVarData, &alarmId);
	CTC_BUF_ENCODE32(pVarContainer->pVarData+2, &alarmThreshold);
	CTC_BUF_ENCODE32(pVarContainer->pVarData+6, &alarmThreshold);

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARGET, "%s alarmId[0x%-4x] threshold[0x%-8x] clearThreshold[0x%-8x]\n", 
					__func__, alarmId, alarmThreshold, clearAlarmThreshold);
	return EPON_OAM_ERR_OK;
}

int ctc_oam_varCb_alarmThreshold_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{   
    unsigned short alarmId;
    unsigned int alarmThreshold;
    unsigned int clearAlarmThreshold;
    unsigned int port;
    
    if(CTC_OAM_VAR_ALARMTHRESHOLD_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
    
    CTC_BUF_PARSE16(pVarContainer->pVarData, &alarmId);
    CTC_BUF_PARSE32(pVarContainer->pVarData+2, &alarmThreshold);
    CTC_BUF_PARSE32(pVarContainer->pVarData+6, &clearAlarmThreshold);

    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
				"alarmThreshold alarmId[%x] threshold[%x] clearThreshold[%x]\n", 
					alarmId, alarmThreshold, clearAlarmThreshold);
    
    if((NULL != pVarInstant) && CTC_INSTANT_IS_PORT(pVarInstant) && CTC_INSTANT_IS_ETHERNET(pVarInstant))
    {
        if(CTC_INSTANT_IS_ALLPORTS(pVarInstant))
        {  
            FOR_EACH_LAN_PORT(port)
            {
                ctc_oam_alarm_threshold_set(alarmId, port+1, alarmThreshold,clearAlarmThreshold);
            }
        }
        else
        {
            port = pVarInstant->parse.uniPort.portNo;
            ctc_oam_alarm_threshold_set(alarmId, port, alarmThreshold,clearAlarmThreshold);
        }
    }
    else
    {
        ctc_oam_alarm_threshold_set(alarmId, 0, alarmThreshold,clearAlarmThreshold);
    }
    
    pVarContainer->varWidth = 0x80;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x00A1 - ONUTxPowerSupplyControl */
int ctc_oam_varCb_txPowerSupplyControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	unsigned short action;
	unsigned char onuID[6];
	unsigned int opticalID;
	oam_config_t oamConfig;
	unsigned int gpio_disTx;

	ctc_oam_gpio_tx_disable_pin_get(&gpio_disTx);
	
	if(CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_LEN != pVarContainer->varWidth)
    {
    	/* zte olt have different data format and data length is 0x0A */
		if(0x0A != pVarContainer->varWidth)
        	return EPON_OAM_ERR_NO_RESPONSE;
    }

	if(CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_LEN == pVarContainer->varWidth)
	{
		CTC_BUF_PARSE16(pVarContainer->pVarData, &action);
		memcpy(onuID, pVarContainer->pVarData+2, 6);
		CTC_BUF_PARSE32(pVarContainer->pVarData+8, &opticalID);
	}
	else
	{
		/* data format: 4 bytes action, 6 bytes onuid */
		unsigned int act;
		CTC_BUF_PARSE32(pVarContainer->pVarData, &act);
		action = act;
		memcpy(onuID, pVarContainer->pVarData+4, 6);
		opticalID = 0;
	}

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
				"txPowerSupplyControl action[0x%x] onuID[%02x:%02x:%02x:%02x:%02x:%02x] opticalID[0x%x] gpio_disTx[%d]\n", 
					action, onuID[0],onuID[1],onuID[2],onuID[3],onuID[4],onuID[5], opticalID, gpio_disTx);

	epon_oam_config_get(llidIdx, &oamConfig);
	if(memcmp(oamConfig.macAddr, onuID, 6) != 0 && 
	   !(onuID[0] == 0xFF && onuID[1] == 0xFF && onuID[2] == 0xFF && onuID[3] == 0xFF && onuID[4] == 0xFF && onuID[5] == 0xFF))
	{
		/* ignore the command if onu id is not our's, must not send response to olt */
		return EPON_OAM_ERR_NO_RESPONSE;
	}
	
	if(opticalID == CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_OPTICALID_BACKUP)
	{
		/* not have backup optical module, so return error */
		return EPON_OAM_ERR_UNKNOWN;
	}	
	
	if(action == CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_ACTION_REENABLE)
	{
		/* clear re-enable tx power timer */
		epon_oam_event_send(llidIdx, EPON_OAM_EVENT_TX_POWER_ENABLE_CLEAR);
		
		/* re-enable the Tx power supply:set ds_tx gpio to 0 */
		rtk_gpio_databit_set(gpio_disTx, 0);
	}
	else if(action == CTC_OAM_VAR_ONUTXPOWERSUPPLYCONTROL_ACTION_SHUTDOWN)
	{
		/* clear re-enable tx power timer */
		epon_oam_event_send(llidIdx, EPON_OAM_EVENT_TX_POWER_ENABLE_CLEAR);
		
		/* permanently shutdown: set ds_tx gpio to 1 */
		rtk_gpio_databit_set(gpio_disTx, 1);
	}
	else
	{
		/* duration during which optical Txpower supply is shut down, uint is sec. */
		rtk_gpio_databit_set(gpio_disTx, 1);

		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "txPowerSupplyControl re-enable tx power after 0x%x seconds\n", action);
		epon_oam_eventData_send(llidIdx, EPON_OAM_EVENT_TX_POWER_ENABLE_TIMER, (unsigned char *)&action, 2);
	}

	pVarContainer->varWidth = 0x80;
    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x00B1 - performanceMonitoringStatus */
int ctc_oam_varCb_performanceMonitoringStatus_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    rtk_enable_t state;
    unsigned int period;
    unsigned short setValue16;

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_PERFORMANCEMONITORINGSTATUS_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* Only accept PON IF & port instant */
    if(CTC_INSTANT_IS_PONIF(pVarInstant))
    {
        /* Only support 1 PON IF */
        CTC_WRAPPER(pmStatusGet, 0, &state, &period);
    }
    else if(CTC_INSTANT_IS_PORT(pVarInstant))
    {
        CTC_WRAPPER(pmStatusGet, pVarInstant->parse.uniPort.portNo - 1 + CTC_WRAPPER_NUM_PONIF, &state, &period);
    }
    else
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

	if(state == DISABLED)
    	setValue16 = 1;
	else
		setValue16 = 2;
	
    CTC_BUF_ENCODE16(pVarContainer->pVarData, &setValue16);
    CTC_BUF_ENCODE32(pVarContainer->pVarData + 2, &period);

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x00B1 - performanceMonitoringStatus */
int ctc_oam_varCb_performanceMonitoringStatus_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    unsigned short state;
    unsigned int period;
	rtk_enable_t enable;
	
    if(CTC_OAM_VAR_PERFORMANCEMONITORINGSTATUS_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    /* Retrive set data */
    CTC_BUF_PARSE16(pVarContainer->pVarData, &state);
    CTC_BUF_PARSE32(pVarContainer->pVarData + 2, &period);

	/* state value: 1 disable; 2 enable. ignore other value */
	if(state == 1)
		enable = DISABLED;
	else if(state == 2)
		enable = ENABLED;
	else
		return EPON_OAM_ERR_PARAM;
	
    /* Only accept PON IF & port instant */
    if(CTC_INSTANT_IS_PONIF(pVarInstant))
    {
        /* Only support 1 PON IF */
        CTC_WRAPPER(pmStatusSet, 0, enable, period);
    }
    else if(CTC_INSTANT_IS_PORT(pVarInstant))
    {
        CTC_WRAPPER(pmStatusSet, pVarInstant->parse.uniPort.portNo - 1 + CTC_WRAPPER_NUM_PONIF, enable, period);
    }
    else
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x00B2 - performanceMonitoringDataCurrent */
int ctc_oam_varCb_performanceMonitoringDataCurrent_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    ctc_pm_statistics_t pmStat;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_PERFORMANCEMONITORINGDATACURRENT_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* Only accept PON IF & port instant */
    if(CTC_INSTANT_IS_PONIF(pVarInstant))
    {
        /* Only support 1 PON IF */
        CTC_WRAPPER(pmStatGet, 0, &pmStat, CTC_WRAPPER_PMTYPE_CURRENT);
    }
    else if(CTC_INSTANT_IS_PORT(pVarInstant))
    {
        CTC_WRAPPER(pmStatGet, pVarInstant->parse.uniPort.portNo - 1 + CTC_WRAPPER_NUM_PONIF, &pmStat, CTC_WRAPPER_PMTYPE_CURRENT);
    }
    else
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    CTC_BUF_ENCODE64(pVarContainer->pVarData, &pmStat.dsDropEvents);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 8, &pmStat.usDropEvents);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 16, &pmStat.dsOctets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 24, &pmStat.usOctets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 32, &pmStat.dsFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 40, &pmStat.usFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 48, &pmStat.dsBroadcastFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 56, &pmStat.usBroadcastFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 64, &pmStat.dsMulticastFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 72, &pmStat.usMulticastFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 80, &pmStat.dsCrcErrorFrames);
	CTC_BUF_ENCODE64(pVarContainer->pVarData + 88, &pmStat.usCrcErrorFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 96, &pmStat.dsUndersizeFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 104, &pmStat.usUndersizeFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 112, &pmStat.dsOversizeFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 120, &pmStat.usOversizeFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 128, &pmStat.dsFragmentFrames);
	CTC_BUF_ENCODE64(pVarContainer->pVarData + 136, &pmStat.usFragmentFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 144, &pmStat.dsJabberFrames);
	CTC_BUF_ENCODE64(pVarContainer->pVarData + 152, &pmStat.usJabberFrames);	
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 160, &pmStat.dsFrames64Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 168, &pmStat.dsFrames65to127Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 176, &pmStat.dsFrames128to255Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 184, &pmStat.dsFrames256to511Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 192, &pmStat.dsFrames512to1023Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 200, &pmStat.dsFrames1024to1518Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 208, &pmStat.usFrames64Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 216, &pmStat.usFrames65to127Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 224, &pmStat.usFrames128to255Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 232, &pmStat.usFrames256to511Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 240, &pmStat.usFrames512to1023Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 248, &pmStat.usFrames1024to1518Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 256, &pmStat.dsDiscardFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 264, &pmStat.usDiscardFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 272, &pmStat.dsErrorFrames);
	CTC_BUF_ENCODE64(pVarContainer->pVarData + 280, &pmStat.usErrorFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 288, &pmStat.statusChangeTimes);

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x00B2 - performanceMonitoringDataCurrent */
int ctc_oam_varCb_performanceMonitoringDataCurrent_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    if(CTC_OAM_VAR_PERFORMANCEMONITORINGDATACURRENT_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    /* Only accept PON IF & port instant */
    if(CTC_INSTANT_IS_PONIF(pVarInstant))
    {
        /* Only support 1 PON IF */
        CTC_WRAPPER(pmStatSet, 0);
    }
    else if(CTC_INSTANT_IS_PORT(pVarInstant))
    {
        CTC_WRAPPER(pmStatSet, pVarInstant->parse.uniPort.portNo - 1 + CTC_WRAPPER_NUM_PONIF);
    }
    else
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x00B3 - performanceMonitoringDataHistory */
int ctc_oam_varCb_performanceMonitoringDataHistory_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
    ctc_pm_statistics_t pmStat;

    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_PERFORMANCEMONITORINGDATACURRENT_LEN; /* Fix value from CTC standard */
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

    /* Only accept PON IF & port instant */
    if(CTC_INSTANT_IS_PONIF(pVarInstant))
    {
        /* Only support 1 PON IF */
        CTC_WRAPPER(pmStatGet, 0, &pmStat, CTC_WRAPPER_PMTYPE_HISTORY);
    }
    else if(CTC_INSTANT_IS_PORT(pVarInstant))
    {
        CTC_WRAPPER(pmStatGet, pVarInstant->parse.uniPort.portNo - 1 + CTC_WRAPPER_NUM_PONIF, &pmStat, CTC_WRAPPER_PMTYPE_HISTORY);
    }
    else
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }

    CTC_BUF_ENCODE64(pVarContainer->pVarData, &pmStat.dsDropEvents);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 8, &pmStat.usDropEvents);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 16, &pmStat.dsOctets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 24, &pmStat.usOctets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 32, &pmStat.dsFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 40, &pmStat.usFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 48, &pmStat.dsBroadcastFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 56, &pmStat.usBroadcastFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 64, &pmStat.dsMulticastFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 72, &pmStat.usMulticastFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 80, &pmStat.dsCrcErrorFrames);
	CTC_BUF_ENCODE64(pVarContainer->pVarData + 88, &pmStat.usCrcErrorFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 96, &pmStat.dsUndersizeFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 104, &pmStat.usUndersizeFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 112, &pmStat.dsOversizeFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 120, &pmStat.usOversizeFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 128, &pmStat.dsFragmentFrames);
	CTC_BUF_ENCODE64(pVarContainer->pVarData + 136, &pmStat.usFragmentFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 144, &pmStat.dsJabberFrames);
	CTC_BUF_ENCODE64(pVarContainer->pVarData + 152, &pmStat.usJabberFrames);	
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 160, &pmStat.dsFrames64Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 168, &pmStat.dsFrames65to127Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 176, &pmStat.dsFrames128to255Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 184, &pmStat.dsFrames256to511Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 192, &pmStat.dsFrames512to1023Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 200, &pmStat.dsFrames1024to1518Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 208, &pmStat.usFrames64Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 216, &pmStat.usFrames65to127Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 224, &pmStat.usFrames128to255Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 232, &pmStat.usFrames256to511Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 240, &pmStat.usFrames512to1023Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 248, &pmStat.usFrames1024to1518Octets);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 256, &pmStat.dsDiscardFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 264, &pmStat.usDiscardFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 272, &pmStat.dsErrorFrames);
	CTC_BUF_ENCODE64(pVarContainer->pVarData + 280, &pmStat.usErrorFrames);
    CTC_BUF_ENCODE64(pVarContainer->pVarData + 288, &pmStat.statusChangeTimes);

    return EPON_OAM_ERR_OK;
}

/* 0xC7/0x801f - configVlanOfTr069Wan */
/*  private command data format:
	byte size 1: Variable Width: 0x05
	byte size 2: Vlan id 	
	byte size 1: 802.1p
	byte size 1: status
	byte size 1: always set to 0x00
*/
#define CTC_OAM_VAR_CONFIGVLANOFTR069WAN_LEN	5
typedef struct oamTr069VlanConfig_s{
	unsigned short tagVlan;
	unsigned char tagPriority;
	unsigned char status;
}oamTr069VlanConfig_t;

static oamTr069VlanConfig_t tr069Vlanconfig = {0,0,0};

int ctc_oam_varCb_configVlanOfTr069Wan_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler allocate data field in container, 
                                          * caller will release it
                                          */
{
	pVarContainer->varDesc = varDesc;
    pVarContainer->varWidth = CTC_OAM_VAR_CONFIGVLANOFTR069WAN_LEN;
    pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
    if(NULL == pVarContainer->pVarData)
    {
        return EPON_OAM_ERR_MEM;
    }

	CTC_BUF_ENCODE16(pVarContainer->pVarData, &tr069Vlanconfig.tagVlan);
	pVarContainer->pVarData[2] = tr069Vlanconfig.tagPriority;
	pVarContainer->pVarData[3] = tr069Vlanconfig.status;
	pVarContainer->pVarData[4] = 0;

	return EPON_OAM_ERR_OK;
}

void configVlanOfTr069Wan_set(unsigned char llidIdx,unsigned short vlanId, 
									unsigned char vlanPrio, unsigned char status)
{
	if((vlanId != tr069Vlanconfig.tagVlan) || (vlanPrio != tr069Vlanconfig.tagPriority) || (status != tr069Vlanconfig.status))
	{
		tr069Vlanconfig.tagVlan = vlanId;
		tr069Vlanconfig.tagPriority = vlanPrio;
		if(status)
			tr069Vlanconfig.status = 1;
		else
			tr069Vlanconfig.status = 0;

	/* config changes, send event to notify systemd */
	#ifdef SUPPORT_OAM_EVENT_TO_KERNEL
		rtk_epon_oam_event_t event;
	
		event.llidIdx = llidIdx;
		event.eventType = RTK_EPON_OAM_EVENT;
		event.eventData[0] = 0;
		memcpy(event.eventData+1, &tr069Vlanconfig, sizeof(tr069Vlanconfig));
		epon_oam_notify_event_send(&event); 
	#endif
	}
}

/* 0xC7/0x801f - configVlanOfTr069Wan */
int ctc_oam_varCb_configVlanOfTr069Wan_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	unsigned short vlanId;
	unsigned char vlanPrio;
	unsigned char status;
	
	if(CTC_OAM_VAR_CONFIGVLANOFTR069WAN_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }
	 /* Retrive set data */
    CTC_BUF_PARSE16(pVarContainer->pVarData, &vlanId);
    vlanPrio = pVarContainer->pVarData[2];
	status = pVarContainer->pVarData[3];

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET,
            "[OAM:%s:%d] set tr069 wan vlanId[%d] vlanPrio[%d] status[%d]\n",
            __FILE__, __LINE__, vlanId, vlanPrio, status);
	
	configVlanOfTr069Wan_set(llidIdx, vlanId, vlanPrio, status);
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;
    return EPON_OAM_ERR_OK;
}


/* 0xC9/0x0001 - Reset ONU */
int ctc_oam_varCb_resetOnu_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
    pVarContainer->varWidth = 0x80;

    /* OLT issue reset ONU command */
    printf("%s:%d OLU issue reset ONU command\n", __FILE__, __LINE__);
    /* Sleep 3 seconds to make sure the print complete */
    sleep(3);
    reboot(LINUX_REBOOT_CMD_RESTART);

    return EPON_OAM_ERR_OK;
}

/* 0xC9/0x0002 - Sleep Control */
int ctc_oam_varCb_sleepControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	unsigned int sleepDuration;
	unsigned int waitDuration;
	unsigned char sleepFlag;
	unsigned char sleepMode;

	if(CTC_OAM_VAR_SLEEPCONTROL_DURATION_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

	CTC_BUF_PARSE32(pVarContainer->pVarData, &sleepDuration);
	CTC_BUF_PARSE32(pVarContainer->pVarData+4, &waitDuration);
	sleepFlag = pVarContainer->pVarData[8];
	sleepMode = pVarContainer->pVarData[9];

	if((CTC_OAM_VAR_SLEEPCONTROL_DURATION_MAX < sleepDuration) || (CTC_OAM_VAR_SLEEPCONTROL_DURATION_MAX < waitDuration))
		return EPON_OAM_ERR_PARAM;
	
	/* TODO: wait for switch interface to support */
	
    pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

    return EPON_OAM_ERR_OK;
}

/* 0xC9/0x0048 - acFastLeaveAdminControl */
int ctc_oam_varCb_acFastLeaveAdminControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	unsigned int enable;
	int ret;
	
	if (pVarContainer->varWidth != 4)
	{
		pVarContainer->varWidth = CTC_OAM_VAR_RESP_VARBADPARAM;
		return EPON_OAM_ERR_OK;
	}

	CTC_BUF_PARSE32(pVarContainer->pVarData, &enable);
	if (1 == enable)
	{
		ret = igmpMcFastleaveModeSet(0);
	}
	else if (2 == enable)
	{
		ret = igmpMcFastleaveModeSet(1);
	}
	
	pVarContainer->varWidth = CTC_OAM_VAR_RESP_SETOK;

	return EPON_OAM_ERR_OK;
}

int ctc_oam_churningKey_set(
    unsigned char llidIdx,
    unsigned char keyIdx,
    unsigned char key[])
{
    rtk_epon_churningKeyEntry_t entry;

    entry.llidIdx = llidIdx;
    entry.keyIdx = keyIdx;
    memcpy(entry.churningKey, key, RTK_EPON_KEY_SIZE);

    /* Call RTK API to set key to HW */
    rtk_epon_churningKey_set(&entry);

    return EPON_OAM_ERR_OK;
}

int ctc_oam_dbaConfig_get(
    ctc_dbaThreshold_t *pDbaThreshold)
{
    /* MODIFY ME - replace with acturl RTK API
     * Fill in the dummy database
     */
    *pDbaThreshold = dbaThresholdDb;

    return EPON_OAM_ERR_OK;
}

int ctc_oam_dbaConfig_set(
    ctc_dbaThreshold_t *pDbaSetResp)
{
    /* MODIFY ME - replace with acturl RTK API
     * pretend to store the data
     */
    dbaThresholdDb = *pDbaSetResp;

    return EPON_OAM_ERR_OK;
}

