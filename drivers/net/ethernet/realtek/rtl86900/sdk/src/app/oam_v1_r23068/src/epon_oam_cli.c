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
 * $Revision: 40647 $
 * $Date: 2013-07-01 15:36:16 +0800 (?±ä?, 01 ä¸ƒæ? 2013) $
 *
 * Purpose : Main function of the EPON OAM protocol stack user application
 *           It create two additional threads for packet Rx and state control
 *
 * Feature : Start point of the EPON OAM protocol stack. Use individual threads
 *           for packet Rx and state control
 *
 */

/*
 * Include Files
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "epon_oam_config.h"
#include "epon_oam_err.h"
#include "epon_oam_msgq.h"
#include "epon_oam_cli.h"

/* 
 * Symbol Definition 
 */
#define EPON_OAM_CLI_BRANCH     0x01
#define EPON_OAM_CLI_LEAF       0x02
#define EPON_OAM_CLI_END        0x03

typedef struct oam_cli_tree_s {
    unsigned char cmd[16];
    unsigned char type;
    union {
        struct oam_cli_tree_s *pSubTree;
        int (*cli_proc)(int argc, char *argv[], oam_cli_t *pCli);
    } u;
} oam_cli_tree_t;


/*  
 * Data Declaration  
 */
static int msgQId;
static oam_cli_tree_t cliRootClear[] = {
    {
        "counter",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_counter_clear }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};

static oam_cli_tree_t cliRootGetCtc[] = {
    {
        "loid",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcLoid_get }
    },
    {
        "var",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcVar_get }
    },
    {
        "alarm",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcAlarm_get }
    },
    {
        "loopdetect",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcLoopdetect_get }
    },
    {
        "portstatuschanges",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_lanportchange_get }
    },
    {
        "silent",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcSilent_get }
    },
    {
        "succAuthTime",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcAuthSuccTime_get }
    },
    {
        "auth",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcAuth_get }
    },
    {
        "registernum",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcRegisterNum_get }
    },
    {
        "vlan",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcVlan_get }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};

static oam_cli_tree_t cliRootGet[] = {
    {
        "config",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_config_get }
    },
    {
        "callback",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_callback_get }
    },
    {
        "ctc",
        EPON_OAM_CLI_BRANCH,
        { cliRootGetCtc }
    },
    {
        "counter",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_counter_get }
    },
    {
        "dbgext",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_dbgExt_get }
    },
    {
        "dbgflag",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_dbgFlag_get }
    },
    {
        "failover",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_failover_get }
    },
    {
        "oaminfo",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_oamInfo_get }
    },
    {
        "oamstate",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_oamState_get }
    },
    {
        "version",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_version_get }
    },
    {
    	"igmp",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_igmp_get }
    }
	,
    {
        "portmap",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_portMap_get }
    },
    {
        "uptime",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_uptime_get }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};

static oam_cli_tree_t cliRootSetConfig[] = {
    {
        "autoreg",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_configAutoReg_set }
    },
    {
        "event",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_configEvent_set }
    },
    {
        "holdover",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_configHoldover_set }
    },
    {
        "mac",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_configMac_set }
    },
    {
        "oam",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_configOam_set }
    },
    {
        "keepalive",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_configKeepalive_set }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};

static oam_cli_tree_t cliRootSetOamInfo[] = {
    {
        "venderoui",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_oamInfoVenderOui_set }
    },
    {
        "venderid",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_oamInfoVenderId_set }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};

static oam_cli_tree_t cliRootSetCtc[] = {
    {
        "loid",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcLoid_set }
    },
    {
        "silent",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcSilent_set }
    },
    {
        "loopdetect",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcLoopdetect_set }
    },
    {
        "registernumReset",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcRegisterNum_reset }
    },
    {
    	"ponLedInSilent",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcSilentPonLedMode_set }
    },
    {
    	"statusVlan",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcVlanStatus_set }
    },
    {
    	"vlan",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_ctcVlan_set }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};

static oam_cli_tree_t cliRootSet[] = {
    {
        "config",
        EPON_OAM_CLI_BRANCH,
        { cliRootSetConfig }
    },
    {
        "callback",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_callback_set }
    },
    {
        "ctc",
        EPON_OAM_CLI_BRANCH,
        { cliRootSetCtc }
    },
    {
        "dbgext",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_dbgExt_set }
    },
    {
        "dbgflag",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_dbgFlag_set }
    },
    {
        "failover",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_failover_set }
    },
    {
        "oaminfo",
        EPON_OAM_CLI_BRANCH,
        { cliRootSetOamInfo }
    },
    {
        "igmp",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_igmp_set }
    },
    {
        "portmap",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_portMap_set }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};

static oam_cli_tree_t cliRootTrigger[] = {
    {
        "register",
        EPON_OAM_CLI_LEAF,
        { .cli_proc = epon_oam_cli_register_trigger }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};

static oam_cli_tree_t cliRoot[] = {
    {
        "clear",
        EPON_OAM_CLI_BRANCH,
        { cliRootClear }
    },
    {
        "get",
        EPON_OAM_CLI_BRANCH,
        { cliRootGet }
    },
    {
        "set",
        EPON_OAM_CLI_BRANCH,
        { cliRootSet }
    },
    {
        "trigger",
        EPON_OAM_CLI_BRANCH,
        { cliRootTrigger }
    },
    {
        "",
        EPON_OAM_CLI_END,
        { NULL }
    }
};


/* 
 * Macro Definition 
 */

/*  
 * Function Declaration  
 */

/* ------------------------------------------------------------------------- */
/* Internal APIs */
static int epon_oam_cli_configAutoReg_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
    
    if(argc >= 3)
    {
        pCli->cliType = EPON_OAM_CLI_CFGAUTOREG_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliAutoReg.llidIdx = llidIdx;

        if(strcmp(argv[2], "enable") == 0)
        {
            if(argc == 4)
            {
                pCli->u.cliAutoReg.autoRegEnable = 1;
                pCli->u.cliAutoReg.autoRegTime = atoi(argv[3]);
            }
            else
            {
                printf("[enable <autoreg-interval>|disable]\n");
            }
        }
        else if(strcmp(argv[2], "disable") == 0)
        {
            if(argc == 3)
            {
                pCli->u.cliAutoReg.autoRegEnable = 0;
            }
            else
            {
                printf("[enable <autoreg-interval>|disable]\n");
            }
        }
        else
        {
            printf("%s <llidIdx> [enable <autoreg-interval>|disable]\n", argv[0]);
            return -1;
        }
        return 0;
    }
    else
    {
        printf("%s <llidIdx> [enable <autoreg-interval>|disable]\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_configEvent_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
    
    if(argc == 4)
    {
        pCli->cliType = EPON_OAM_CLI_CFGEVENT_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliEvent.llidIdx = llidIdx;
        pCli->u.cliEvent.eventRepCnt = atoi(argv[2]);
        pCli->u.cliEvent.eventRepIntvl = atoi(argv[3]);

        return 0;
    }
    else
    {
        printf("%s <llidIdx> <repeat-cnt> <repeat-interval>\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_configHoldover_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
    
    if(argc == 4)
    {
        pCli->cliType = EPON_OAM_CLI_CFGHOLDOVER_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }

        if(strcmp(argv[2], "enable") == 0)
        {
            pCli->u.cliHoldover.holdoverEnable = 1;
        }
        else if(strcmp(argv[2], "disable") == 0)
        {
            pCli->u.cliHoldover.holdoverEnable = 0;
        }

        pCli->u.cliHoldover.llidIdx = llidIdx;
        pCli->u.cliHoldover.holdoverTime = atoi(argv[3]);

        return 0;
    }
    else
    {
        printf("%s <llidIdx> [enable|disable] <holdover-time>\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_configMac_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    int i;
    char *endPtr, *pch;
    unsigned int llidIdx;
    
    if(argc == 3)
    {
        pCli->cliType = EPON_OAM_CLI_CFGMAC_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliMac.llidIdx = llidIdx;

        /* Replace ':' with ' ' */
        pch = endPtr = argv[2];
        pch = strchr(pch, ':');
        while(pch != NULL)
        {
            *pch = ' ';
            pch = strchr(pch, ':');
        }

        for(i = 0;i < 6;i++)
        {
            pCli->u.cliMac.mac[i] =  strtol(endPtr, &endPtr, 16); 
        }

        return 0;
    }
    else
    {
        printf("%s <llidIdx> <mac-addr>\n", argv[0]);
        printf("Ex: %s 3 00:11:22:33:44:55\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_configOam_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
    
    if(argc == 3)
    {
        pCli->cliType = EPON_OAM_CLI_CFGOAM_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliEnable.llidIdx = llidIdx;

        if(strcmp(argv[2], "enable") == 0)
        {
            pCli->u.cliEnable.enable = 1;
        }
        else if(strcmp(argv[2], "disable") == 0)
        {
            pCli->u.cliEnable.enable = 0;
        }
        else
        {
            printf("[enable | disable]\n");
            return -1;
        }

        return 0;
    }
    else
    {
        printf("%s <llidIdx> [enable | disable]\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_configKeepalive_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
    unsigned short temp;
	
    if(argc == 3)
    {
        pCli->cliType = EPON_OAM_CLI_CFGKEEPALIVE_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliKeepalive.llidIdx = llidIdx;

		temp = atoi(argv[2]);
		if(temp < 5000)
		{
			printf("<keepalive-time>: min[5000]\n");
			return -1;
		}
        pCli->u.cliKeepalive.keepaliveTime = temp;

        return 0;
    }
    else
    {
        printf("%s <llidIdx> <keepalive-time>\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_config_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_CFG_GET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliLlidIdx.llidIdx = llidIdx;

        return 0;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_counter_clear(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_COUNTER_CLEAR;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliLlidIdx.llidIdx = llidIdx;

        return 0;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}


static int epon_oam_cli_callback_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int tmp;
    if(argc == 3)
    {
        pCli->cliType = EPON_OAM_CLI_CALLBACK_SET;
        tmp = strtol(argv[1], NULL, 16);
        pCli->u.cliCallback.oui[0] = ((unsigned char *)(&tmp))[1];
        pCli->u.cliCallback.oui[1] = ((unsigned char *)(&tmp))[2];
        pCli->u.cliCallback.oui[2] = ((unsigned char *)(&tmp))[3];

        if(strcmp(argv[2], "enable") == 0)
        {
            pCli->u.cliCallback.state = 1;
        }
        else if(strcmp(argv[2], "disable") == 0)
        {
            pCli->u.cliCallback.state = 0;
        }
        else
        {
            printf("[enable | disable]\n");
            return -1;
        }

        return 0;
    }
    else
    {
        printf("%s <oui> [enable | disable]\n", argv[0]);
        return -1;
    }
}


static int epon_oam_cli_callback_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int tmp;
    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_CALLBACK_GET;
        tmp = strtol(argv[1], NULL, 16);
        pCli->u.cliCallback.oui[0] = ((unsigned char *)(&tmp))[1];
        pCli->u.cliCallback.oui[1] = ((unsigned char *)(&tmp))[2];
        pCli->u.cliCallback.oui[2] = ((unsigned char *)(&tmp))[3];

        return 0;
    }
    else
    {
        printf("%s <oui>\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_counter_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_COUNTER_GET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliLlidIdx.llidIdx = llidIdx;

        return 0;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_dbgFlag_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_DBG_SET;
        pCli->u.cliDbg.flag = strtoul(argv[1], NULL, 16);

        return 0;
    }
    else
    {
        printf("%s <hex-value>\n", argv[0]);
        printf("EPON_OAM_DBGFLAG_NONE       (0x00000000UL)\n");
        printf("EPON_OAM_DBGFLAG_INFO       (0x00000001UL)\n");
        printf("EPON_OAM_DBGFLAG_WARN       (0x00000002UL)\n");
        printf("EPON_OAM_DBGFLAG_ERROR      (0x00000004UL)\n");
        printf("EPON_OAM_DBGFLAG_DUMP       (0x00000008UL)\n");
        printf("EPON_OAM_DBGFLAG_CTC_INFO   (0x00000010UL)\n");
        printf("EPON_OAM_DBGFLAG_CTC_WARN   (0x00000020UL)\n");
        printf("EPON_OAM_DBGFLAG_CTC_ERROR  (0x00000040UL)\n");
        printf("EPON_OAM_DBGFLAG_CTC_DUMP   (0x00000080UL)\n");
        printf("EPON_OAM_DBGFLAG_CTC_VARSET (0x00000100UL)\n");
        printf("EPON_OAM_DBGFLAG_CTC_VARGET (0x00000200UL)\n");
        printf("EPON_OAM_DBGFLAG_ORGRX      (0x04000000UL)\n");
        printf("EPON_OAM_DBGFLAG_ORGTX      (0x08000000UL)\n");
        printf("EPON_OAM_DBGFLAG_TXOAM      (0x10000000UL)\n");
        printf("EPON_OAM_DBGFLAG_RXOAM      (0x20000000UL)\n");
        printf("EPON_OAM_DBGFLAG_DEBUG      (0x80000000UL)\n");
        printf("EPON_OAM_DBGFLAG_ALL        (0xFFFFFFFFUL)\n");
        return -1;
    }
}

static int epon_oam_cli_dbgFlag_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    pCli->cliType = EPON_OAM_CLI_DBG_GET;

    return 0;
}

static int epon_oam_cli_dbgExt_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_DBGEXT_SET;
        pCli->u.cliDbg.flag = strtol(argv[1], NULL, 16);

        return 0;
    }
    else
    {
        printf("%s <hex-value>\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_dbgExt_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    pCli->cliType = EPON_OAM_CLI_DBGEXT_GET;

    return 0;
}

static int epon_oam_cli_failover_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    int value;
    if(argc == 3)
    {
        pCli->cliType = EPON_OAM_CLI_FAILOVER_SET;
        value = atoi(argv[1]);
        if(value < 0)
        {
            return -1;
        }
        pCli->u.cliFailover.granularity = value;

        value = atoi(argv[2]);
        if(value < 0)
        {
            return -1;
        }
        pCli->u.cliFailover.backoff = value;

        return 0;
    }
    else
    {
        printf("%s <granularity-ms> <backoff-ms>\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_failover_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    pCli->cliType = EPON_OAM_CLI_FAILOVER_GET;

    return 0;
}

static int epon_oam_cli_oamInfoVenderOui_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    int i;
    char *endPtr, *pch;
    unsigned int llidIdx;
    
    if(argc == 3)
    {
        pCli->cliType = EPON_OAM_CLI_OAMINFOOUI_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliMac.llidIdx = llidIdx;

        /* Replace ':' with ' ' */
        pch = endPtr = argv[2];
        pch = strchr(pch, ':');
        while(pch != NULL)
        {
            *pch = ' ';
            pch = strchr(pch, ':');
        }

        for(i = 0;i < 3;i++)
        {
            pCli->u.cliMac.mac[i] =  strtol(endPtr, &endPtr, 16); 
        }

        return 0;
    }
    else
    {
        printf("%s <llidIdx> <vender-oui>\n", argv[0]);
        printf("Ex: %s 3 00:11:22\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_oamInfoVenderId_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    int i;
    char *endPtr, *pch;
    unsigned int llidIdx;
    
    if(argc == 3)
    {
        pCli->cliType = EPON_OAM_CLI_OAMINFO_VENDORID_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliMac.llidIdx = llidIdx;

        /* Replace ':' with ' ' */
        pch = endPtr = argv[2];
        pch = strchr(pch, ':');
        while(pch != NULL)
        {
            *pch = ' ';
            pch = strchr(pch, ':');
        }

        for(i = 0;i < 4;i++)
        {
            pCli->u.cliMac.mac[i] =  strtol(endPtr, &endPtr, 16); 
        }

        return 0;
    }
    else
    {
        printf("%s <llidIdx> <vender-id>\n", argv[0]);
        printf("Ex: %s 1 00:11:22:33\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_oamInfo_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_OAMINFO_GET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliLlidIdx.llidIdx = llidIdx;

        return 0;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}
    
static int epon_oam_cli_oamState_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_OAMSTATE_GET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliLlidIdx.llidIdx = llidIdx;

        return 0;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }
}

static int epon_oam_cli_version_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

#ifdef EPON_OAM_VERSION
    printf("EPON OAM Version: %s\n", EPON_OAM_VERSION);
#else
    printf("EPON OAM Version: Unknown version\n");
#endif
    printf("Build Time: %s %s\n", __DATE__, __TIME__);
	return -1;
}

static int epon_oam_cli_igmp_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
   
    pCli->cliType = EPON_OAM_IGMP_GET;  

    return 0;
}

static int epon_oam_cli_igmp_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int val;

	memset(pCli, 0, sizeof(oam_cli_t));
	pCli->cliType = EPON_OAM_IGMP_SET;

	if(argc == 3)
    {
		if(strcmp(argv[1], "controltype") == 0)
		{
			pCli->u.cliIgmp.type = IGMP_CONTROL_TYPE;
			val = atoi(argv[2]);
			if(val < 0 || val > 2)
			{
				printf("<controltype>: %u~%u\n", 0, 2);
	            return -1;
			}
			pCli->u.cliIgmp.controlType = val;
			return 0;
		}
		else if(strcmp(argv[1], "debug") == 0)
		{
			pCli->u.cliIgmp.type = IGMP_DEBUG_TYPE;
			val = atoi(argv[2]);
			if(val < 0 || val > 1)
			{
				printf("<debug>: %u~%u\n", 0, 1);
	            return -1;
			}
			pCli->u.cliIgmp.val = val;
			return 0;
		}
	}
	else if(argc == 4)
	{
		if(strcmp(argv[1], "mctag") == 0)
		{
			val = atoi(argv[2]);
			if(val < 0 || val > 3)
			{
				printf("<mctag> <port>: %u~%u\n", 0,3);
	            return -1;
			}
			pCli->u.cliIgmp.port = val;
			
			val = atoi(argv[3]);
			if(val < 0 || val > 2)
			{
				printf("<mctag> <val>: %u~%u\n", 0,2);
	            return -1;
			}
			pCli->u.cliIgmp.val = val;
			pCli->u.cliIgmp.type = IGMP_MCTAGOPER_TYPE;
			return 0;
		}
	}
	else if(argc == 5)
	{	
		if(strcmp(argv[1], "mcvlan") == 0)
		{
			val = atoi(argv[2]);
			if(val < 0 || val > 3)
			{
				printf("<mcvlan> <port>: %u~%u\n", 0, 3);
	            return -1;
			}
			pCli->u.cliIgmp.port= val;

			val = atoi(argv[3]);
			if(val <= 0 || val > 4095)
			{
				printf("<mcvlan> <vid>: %u~%u\n", 1, 4095);
	            return -1;
			}
			pCli->u.cliIgmp.mcVlan= val;

			val = atoi(argv[4]);
			if(val == 0)
				pCli->u.cliIgmp.type = IGMP_MCVLAN_DEL_TYPE;
			else
				pCli->u.cliIgmp.type = IGMP_MCVLAN_ADD_TYPE;
			return 0;
		}
		else if(strcmp(argv[1], "translation") == 0)
		{
			val = atoi(argv[2]);
			if(val < 0 || val > 3)
			{
				printf("<mctag> <port>: %u~%u\n", 0,3);
	            return -1;
			}
			pCli->u.cliIgmp.port = val;

			val = atoi(argv[3]);
			if(val <= 0 || val > 4095)
			{
				printf("<translation> <mcvlan>: %u~%u\n", 1, 4095);
	            return -1;
			}
			pCli->u.cliIgmp.mcVlan= val;

			val = atoi(argv[4]);
			if(val <= 0 || val > 4095)
			{
				printf("<translation> <uservlan>: %u~%u\n", 1, 4095);
	            return -1;
			}
			pCli->u.cliIgmp.val = val;
			
			pCli->u.cliIgmp.type = IGMP_TRANSLATION_ADD_TYPE;			
			return 0;
		}
	}
	printf("%-4s <controltype> <val> (0:MAC 1:MAC+VID 2:IP+VID)\n"
		   "     <mcvlan> <port> <vid> <type> (0:delete 1:add)\n"
		   "     <mctag> <port> <val> (0:transparent 1:strip 2:translation)\n"
		   "     <translation> <port> <mcvlan> <uservlan> (is add command)\n"
		   "     <debug> <val> (0:disable 1:enable)\n", argv[0]);
    return -1;
}

static int epon_oam_cli_portMap_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	int num;
	int i;
	
	if(argc >= 3 && argc <= 6)
	{
		num = atoi(argv[1]);
		if(num > 4 || num <= 0)
		{
			printf("%s <lan port num>: 1~4 \n", argv[0]);
		       return -1;
		}

		pCli->u.cliPortMap.portNum =  num;
		for(i = 0; i < num; i++)
		{
			pCli->u.cliPortMap.portMap[i] = atoi(argv[2+i]);
	 	}
		
	        pCli->cliType = EPON_OAM_PORT_MAP_SET;
	        return 0;
	}
	 else
	{
	        printf("%s <lan port num> <port 1 map> <port 2 map> <port 3 map> <port 4 map> \n", argv[0]);
	        return -1;
	}
}

static int epon_oam_cli_portMap_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	 pCli->cliType = EPON_OAM_PORT_MAP_GET;
	 return 0;
}

static int epon_oam_cli_uptime_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	unsigned int tmp;
	
	memset(pCli, 0, sizeof(oam_cli_t));
	pCli->cliType = EPON_OAM_CLI_UPTIME_GET;

	if (argc == 2)
    {       
        pCli->u.cliUptimeToFile.uptimeToFile= 0;

		tmp = atoi(argv[1]);
		if(tmp >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
		pCli->u.cliUptimeToFile.llidIdx = tmp;
    }
	else if (argc == 3)
	{
		tmp = atoi(argv[1]);
		if(tmp >= 2)
		{
			tmp = 1;
		}
		
		pCli->u.cliUptimeToFile.uptimeToFile= tmp;

		tmp = atoi(argv[2]);
		if(tmp >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
		pCli->u.cliUptimeToFile.llidIdx = tmp;
	}
    else
    {
        printf("%s [1:saveToFile] <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_register_trigger(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

    if(argc == 2)
    {
        pCli->cliType = EPON_OAM_CLI_REG_TRIGGER;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliLlidIdx.llidIdx = llidIdx;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
/* CTC APIs */
static int epon_oam_cli_ctcLoid_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

    if(argc == 2)
    {
        memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_LOID_GET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliCtcLoid.llidIdx = llidIdx;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_ctcLoid_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;

	if(argc == 2)
	{
		memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_LOID_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliCtcLoid.llidIdx = llidIdx;
	}
    else if(argc == 3)
    {
        memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_LOID_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliCtcLoid.llidIdx = llidIdx;
        strcpy(pCli->u.cliCtcLoid.loid, argv[2]);
    }
    else if(argc == 4)
    {
        memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_LOID_SET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliCtcLoid.llidIdx = llidIdx;
        strcpy(pCli->u.cliCtcLoid.loid, argv[2]);
        strcpy(pCli->u.cliCtcLoid.password, argv[3]);
    }
    else
    {
        printf("%s <llidIdx> <loid> {<password>}\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_ctcSilent_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{   
    memset(pCli, 0, sizeof(oam_cli_t));
    pCli->cliType = CTC_OAM_CLI_SILENT_GET;

    return 0;
}

static int epon_oam_cli_ctcSilent_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	unsigned int val;
	
	if(argc == 2)
    {
    	memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_SILENT_SET;
        val = atoi(argv[1]);
		if(val < 0 || val > 1)
		{
			printf("<val>: %u~%u\n", 0, 1);
	        return -1;
		}
		if(val == 0)
			pCli->u.cliEnable.enable = 0;
		else
			pCli->u.cliEnable.enable = 1;
		return 0;
	}
	else
    {
        printf("%s val (0:disable, 1:enable)\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_ctcAuthSuccTime_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
    
    if (argc == 2)
    {
        memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_AUTHSUCCTIME_GET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliLlidIdx.llidIdx == llidIdx;
        
        return 0;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_ctcAuth_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
    
    if (argc == 2)
    {
        memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_AUTH_GET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        pCli->u.cliLlidIdx.llidIdx == llidIdx;
        
        return 0;
    }
    else
    {
        printf("%s <llidIdx>\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_ctcRegisterNum_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{  	
	memset(pCli, 0, sizeof(oam_cli_t));
    pCli->cliType = CTC_OAM_CLI_REGISTERNUM_GET;

	if (argc == 1)
    {       
        pCli->u.cliRegisterNumToFile.registerNumToFile= 0;
		pCli->u.cliRegisterNumToFile.llidIdx = 0;
    }
	else if (argc == 2)
	{
		unsigned int tmp;
		tmp = atoi(argv[1]);
		if(tmp >= 2)
		{
			tmp = 1;
		}
		
		pCli->u.cliRegisterNumToFile.registerNumToFile = tmp;	
		pCli->u.cliRegisterNumToFile.llidIdx = 0;
	}
	else if (argc == 3)
	{
		unsigned int tmp;
		tmp = atoi(argv[1]);
		if(tmp >= 2)
		{
			tmp = 1;
		}
		
		pCli->u.cliRegisterNumToFile.registerNumToFile = tmp;

		tmp = atoi(argv[2]);
		if(tmp >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
		pCli->u.cliRegisterNumToFile.llidIdx = tmp;
	}
    else
    {
        printf("%s [1:saveToFile] [llidIdx]\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_ctcRegisterNum_reset(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	memset(pCli, 0, sizeof(oam_cli_t));
	pCli->cliType = CTC_OAM_CLI_REGISTERNUM_RESET;
	
	if(argc == 1)
    {   	
		pCli->u.cliRegisterNumToFile.llidIdx = 0;
	}
	else if(argc == 2)
	{
		unsigned int tmp;
		tmp = atoi(argv[1]);
		if(tmp >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
		pCli->u.cliRegisterNumToFile.llidIdx = tmp;
	}
	else
    {
        printf("%s [llidIdx]\n", argv[0]);
        return -1;
    }
	
	return 0;
}

static int epon_oam_cli_ctcSilentPonLedMode_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	if(argc == 2)
    {
    	unsigned int val;
		
    	memset(pCli, 0, sizeof(oam_cli_t));	
    	pCli->cliType = CTC_OAM_CLI_SILENTPONLEDMODE_SET;
		val = atoi(argv[1]);
		if(val < 0 || val > 1)
		{
			printf("<val>: %u~%u\n", 0, 1);
	        return -1;
		}
		if(val == 0)
			pCli->u.cliEnable.enable = 0;
		else
			pCli->u.cliEnable.enable = 1;
		return 0;
	}
	else
    {
        printf("%s ponLedMode(0:down, 1: blink)\n", argv[0]);
        return -1;
    }
	
	return 0;
}

static int epon_oam_cli_ctcVlanStatus_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	if(argc == 2)
    {
    	unsigned int val;
		
    	memset(pCli, 0, sizeof(oam_cli_t));	
    	pCli->cliType = CTC_OAM_CLI_VLANSTATUS_SET;
		val = atoi(argv[1]);
		if(val < 0 || val > 1)
		{
			printf("<status val>: %u~%u\n", 0, 1);
	        return -1;
		}
		pCli->u.cliEnable.enable = val;		
		return 0;
	}
	else
    {
        printf("%s status(0:can be changed by olt 1:static)\n", argv[0]);
        return -1;
    }
	
	return 0;
}

static int epon_oam_cli_ctcVlan_get(
	int argc,
    char *argv[],
    oam_cli_t *pCli)
{   
    memset(pCli, 0, sizeof(oam_cli_t));
    pCli->cliType = CTC_OAM_CLI_VLAN_GET;

    return 0;
}

static int epon_oam_cli_ctcVlan_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	if(argc >= 3)
    {
    	unsigned int val;
		
    	memset(pCli, 0, sizeof(oam_cli_t));	
    	pCli->cliType = CTC_OAM_CLI_VLAN_SET;
		val = atoi(argv[1]);
		if(val < 0 || val > 4)
		{
			printf("<port val>: %u~%u\n", 0, 4);
	        return -1;
		}
		pCli->u.cliVlanCfg.port = val;

		val = atoi(argv[2]);
		if(val < 0 || val > 2)
		{
			printf("<mode val>: %u~%u\n", 0, 2);
	        return -1;
		}
		pCli->u.cliVlanCfg.vlanMode = val;

		if(argc == 4)
		{
			if(pCli->u.cliVlanCfg.vlanMode == 1)
			{
				sscanf(argv[3],"%d/%d", &pCli->u.cliVlanCfg.cfg.tagCfg.vid, &pCli->u.cliVlanCfg.cfg.tagCfg.pri);
			}
			else if(pCli->u.cliVlanCfg.vlanMode == 2)
			{
				sscanf(argv[3],"%d/%d:%d/%d", &pCli->u.cliVlanCfg.cfg.transCfg.svlan.vid, &pCli->u.cliVlanCfg.cfg.transCfg.svlan.pri,
											  &pCli->u.cliVlanCfg.cfg.transCfg.cvlan.vid, &pCli->u.cliVlanCfg.cfg.transCfg.cvlan.pri);
			}
			else
				return -1;
		}
		return 0;
	}
	else
    {
        printf("%s port mode(0:transparent 1:tag 2:translation) [vlan]\n", argv[0]);
		printf("\ttransparent:don't input vlan\n");
		printf("\ttag vlan(vid/pri)\n");
		printf("\ttranslation svlan:cvlan(vid/pri:vid/pri)\n");
        return -1;
    }
	
	return 0;
}

static int epon_oam_cli_ctcVar_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
    unsigned int llidIdx;
    unsigned int varBranch, varLeaf;

    if(argc == 4)
    {
        memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_VAR_GET;
        llidIdx = atoi(argv[1]);
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            printf("<llidIdx>: %u~%u\n", 0, EPON_OAM_SUPPORT_LLID_NUM - 1);
            return -1;
        }
        varBranch = strtoul(argv[2], NULL, 16);
        varLeaf = strtoul(argv[3], NULL, 16);
        pCli->u.cliCtcVar.varBranch = varBranch;
        pCli->u.cliCtcVar.varLeaf = varLeaf;
    }
    else
    {
        printf("%s <llidIdx> <varBranch> <varLeaf>\n", argv[0]);
        return -1;
    }

    return 0;
}

static int epon_oam_cli_ctcAlarm_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{   
    memset(pCli, 0, sizeof(oam_cli_t));
    pCli->cliType = CTC_OAM_CLI_ALARM_GET;

	if(argc == 2)
	{
		pCli->u.cliAlarmShowDisable = atoi(argv[1]);
	}
    return 0;
}

static int epon_oam_cli_ctcLoopdetect_set(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	int val;
	if(argc==5 || argc==6)
	{
		memset(pCli, 0, sizeof(oam_cli_t));
        pCli->cliType = CTC_OAM_CLI_LOOPDETECT_SET;
		val = atoi(argv[1]);
		if(val!=0 && val!=1)
		{
			printf("please input 0:disable 1:enable\n");
			return -1;
		}
		pCli->u.cliCtcLoopdetect.loopdetect_enable = val;
		val=strtol(argv[2],NULL,NULL);
		pCli->u.cliCtcLoopdetect.ether_type = val;
		val=strtol(argv[3],NULL,NULL);
		pCli->u.cliCtcLoopdetect.loopdetect_time = val;
		val=strtol(argv[4],NULL,NULL);
		pCli->u.cliCtcLoopdetect.loop_recover_time = val;
		if(argc==6)
		{
			int i;
			char vlanstr[512];
			char* pstr,*pvlanstr;
			strcpy(vlanstr, argv[5]);
			pvlanstr = vlanstr;
			for(i=0;i<100;i++)
			{
				if((pstr = strsep(&pvlanstr, ","))!=NULL)
				{
					pCli->u.cliCtcLoopdetect.vid[i] = strtol(pstr, NULL, NULL);
				}
				else
				{
					pCli->u.cliCtcLoopdetect.vid[i] = -1;
					break;
				}
			}
		}
		else
		{
			pCli->u.cliCtcLoopdetect.vid[0] = 0;
			pCli->u.cliCtcLoopdetect.vid[1] = -1;
		}
	}
	else
	{
		printf("%s <enable> <etherType> <detectTime> <recoverTime> [vid]\n", argv[0]);
		printf("\t time unit is second, vid:x,y\n", argv[0]);
        return -1;
	}
	return 0;
}

static int epon_oam_cli_ctcLoopdetect_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	memset(pCli, 0, sizeof(oam_cli_t));
    pCli->cliType = CTC_OAM_CLI_LOOPDETECT_GET;

    return 0;
}

static int epon_oam_cli_lanportchange_get(
    int argc,
    char *argv[],
    oam_cli_t *pCli)
{
	memset(pCli, 0, sizeof(oam_cli_t));
    pCli->cliType = CTC_OAM_CLI_LANPORTCHANGE_GET;

    return 0;
}

int epon_oam_event_send(
    unsigned char llidIdx,
    unsigned int eventId)
{
    int ret;
    oam_msgqEventData_t event;

    event.mtype = eventId;
    event.msgqData.llidIdx = llidIdx;
    event.msgqData.dataSize = 0;
    ret = msgsnd(msgQId, (void *)&event, 0, IPC_NOWAIT);
    if(-1 == ret)
    {
        printf("[OAM:%s:%d] msgsnd failed %d\n", __FILE__, __LINE__, errno);
        return EPON_OAM_ERR_MSGQ;
    }

    return EPON_OAM_ERR_OK;
}

static int epon_oam_eventData_send(
    unsigned char llidIdx,
    unsigned int eventId,
    unsigned char *data,
    unsigned short dataLen)
{
    int ret;
    oam_msgqEventData_t eventData;

    if(dataLen > EPON_OAM_EVENTDATA_MAX)
    {
        return EPON_OAM_ERR_PARAM;
    }

    eventData.mtype = eventId;
	eventData.msgqData.reqid = getpid();
    eventData.msgqData.llidIdx = llidIdx;
    eventData.msgqData.dataSize = dataLen;
    memcpy(eventData.msgqData.data, data, dataLen);
    ret = msgsnd(msgQId, (void *)&eventData, sizeof(eventData.msgqData), IPC_NOWAIT);
    if(-1 == ret)
    {
        printf("[OAM:%s:%d] msgsnd failed %d\n", __FILE__, __LINE__, errno);
        return EPON_OAM_ERR_MSGQ;
    }
	
	if((((oam_cli_t*)data)->cliType == CTC_OAM_CLI_AUTH_GET) || (((oam_cli_t*)data)->cliType == CTC_OAM_CLI_AUTHSUCCTIME_GET))
	{
		msgbuf_t msg;
		memset(&msg, 0, sizeof(msg));
		ret = msgrcv(msgQId, (void*)&msg, sizeof(msg.buf), eventData.msgqData.reqid, 0);
		msg.buf[sizeof(msg.buf)-1]='\0';
		printf("%s\n",msg.buf);
	}
	
    return EPON_OAM_ERR_OK;
}

int epon_oam_cli_parse(
	int argc,
	char *argv[],
	oam_cli_tree_t *pCliTree,
	oam_cli_t *pCli)
{
    int inputLen, cmdLen;

    if(0 == argc)
    {
        /* No any argument for parsing */
        printf("available cmds:\n");
        while(pCliTree->type != EPON_OAM_CLI_END)
        {
            printf("%s\n", pCliTree->cmd);
            pCliTree += 1;
        }

        return -1;
    }

    if(NULL == pCliTree)
    {
        printf("incorrect command tree\n");
        return -2;
    }

    inputLen = strlen(argv[0]);
    while(pCliTree->type != EPON_OAM_CLI_END)
    {
        cmdLen = strlen(pCliTree->cmd);
        if(strncmp(argv[0], pCliTree->cmd, inputLen > cmdLen ? cmdLen : inputLen) == 0)
        {
            /* Search subtree or execute the command */
            if(pCliTree->type == EPON_OAM_CLI_BRANCH)
            {
                return epon_oam_cli_parse(argc - 1, &argv[1], pCliTree->u.pSubTree, pCli);
            }
            else if(pCliTree->type == EPON_OAM_CLI_LEAF)
            {
                if(NULL != pCliTree->u.cli_proc)
                {
                    return pCliTree->u.cli_proc(argc, argv, pCli);
                }
                else
                {
                    printf("incorrect command callback\n");
                    return -3;
                }
            }
            else
            {
                printf("incorrect command type\n");
                return -3;
            }
        }
        pCliTree = pCliTree + 1;
    }

    printf("incorrect command\n");
    return -4;
}

int
main(
	int argc,
	char *argv[])
{
    int ret;
    int permits;
    key_t msgQKey = 1568;
    oam_cli_t cli;

    /* Get message queue to send event to state keeper */
    /* S_IRUSR | S_IWUSR | State keeper can read/write message
     * S_IRGRP | S_IWGRP | All others can read/write message
     * S_IROTH | S_IWOTH   All others can read/write message
     */
    permits = 00666;
    permits |= IPC_CREAT;
    msgQId = msgget(msgQKey, permits);
    if(-1 == msgQId)
    {
        printf("[OAM:%s:%d] msgq create failed %d\n", __FILE__, __LINE__, errno);
        return -1;
    }

    memset(&cli, 0x0, sizeof(cli));
    ret = epon_oam_cli_parse(argc - 1, &argv[1], cliRoot, &cli);

    if(0 == ret)
    {
        epon_oam_eventData_send(0,
            EPON_OAM_EVENT_CLI,
            (unsigned char *)&cli,
            sizeof(cli));
    }

    return 0;
}

