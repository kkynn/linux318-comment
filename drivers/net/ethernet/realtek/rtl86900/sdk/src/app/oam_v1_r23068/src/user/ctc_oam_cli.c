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
 * Purpose : Provide CTC OAM debug commands
 *
 * Feature : 
 *
 */

/*
 * Include Files
 */
/* Standard include */
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "epon_oam_config.h"
#include "epon_oam_rx.h"
#include "epon_oam_db.h"
#include "epon_oam_msgq.h"
#include "epon_oam_dbg.h"
#include "epon_oam_err.h"

#include "ctc_oam.h"
#include "ctc_oam_var.h"
#include "ctc_wrapper.h"
#include "ctc_mc.h"
#include "ctc_oam_alarmtbl.h"
#ifdef CONFIG_SDK_APOLLOMP
#include <dal/apollomp/dal_apollomp_switch.h>
#endif

#include <hal/chipdef/chip.h>

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */

/*
 * Macro Definition
 */

/*
 * Function Declaration
 */
/* aPhyAdminState */
int ctc_oam_varCli_phyAdminState_get(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	ctc_varDescriptor_t varDesc)
{
	rtk_enable_t enable;
	int ret;
	
    ret = CTC_WRAPPER(portStateGet, 0, &enable);
    if(EPON_OAM_ERR_OK == ret)
    {
        if(DISABLED == enable)
        {
            printf("port 0 disabled!\n");
        }
        else
        {
            printf("port 0 enabled!\n");
        }
    }
	
	return EPON_OAM_ERR_OK;
}

/* aAutoNegAdminState */
int ctc_oam_varCli_autoNegAdminState_get(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	ctc_varDescriptor_t varDesc)
{
	rtk_enable_t enable;
	int ret;
	
    ret = CTC_WRAPPER(portAutoNegoGet, 0, &enable);
    if(EPON_OAM_ERR_OK == ret)
    {
        if(DISABLED == enable)
        {
            printf("port 0 disabled!\n");
        }
        else
        {
            printf("port 0 enabled!\n");
        }
    }
	
	return EPON_OAM_ERR_OK;
}

/* Chipset ID */
int ctc_oam_varCli_chipID_get(
	unsigned char llidIdx, /* LLID index of the incoming operation */
	ctc_varDescriptor_t varDesc)
{
	int ret;
	ctc_wrapper_chipInfo_t chipInfo;

	if(ret = CTC_WRAPPER(chipInfoGet, &chipInfo))
	{
		return EPON_OAM_ERR_UNKNOWN;
	}

	/* Chip Model & revision */
	switch(chipInfo.chipId)
	{
#ifdef CONFIG_SDK_APOLLOMP
	case APOLLOMP_CHIP_ID:
		switch(chipInfo.subType)
		{
		case APPOLOMP_CHIP_SUB_TYPE_RTL9601:
			printf("0000960101");
			break;
		case APPOLOMP_CHIP_SUB_TYPE_RTL9602B:
			printf("0000960202");
			break;
		case APPOLOMP_CHIP_SUB_TYPE_RTL9606:
			printf("0000960601");
			break;
		case APPOLOMP_CHIP_SUB_TYPE_RTL9607:
			printf("0000960701");
			break;
		case APPOLOMP_CHIP_SUB_TYPE_RTL9602:
			printf("0000960201");
			break;
		case APPOLOMP_CHIP_SUB_TYPE_RTL9603:
			printf("0000960301");
			break;
		case APPOLOMP_CHIP_SUB_TYPE_RTL9607P:
			printf("0000960701");
			break;
		}
		break;
#endif
//#ifdef CONFIG_SDK_RTL9601B
	case RTL9601B_CHIP_ID:
		printf("0000960102");
		break;
//#endif
//#ifdef CONFIG_SDK_RTL9602C
	case RTL9602C_CHIP_ID:
		printf("0000960203");
		break;
//#endif
	default:
		/* Unknown chip ID */
		printf("0000000000");
		break;
	}

	/* IC_Version/Date - revision */
	printf("0000%02x\n", chipInfo.rev);
	
	return EPON_OAM_ERR_OK;
}

int ctc_oam_varCli_vlan_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    ctc_varDescriptor_t varDesc)
{
    int i, j;
    int ret, port;
    ctc_wrapper_chipInfo_t chipInfo;
    ctc_wrapper_vlanCfg_t vlanCfg;

    printf("port mode        config\n");
    CTC_WRAPPER(chipInfoGet, &chipInfo);
    for(port = 0 ; port < chipInfo.uniPortCnt ; port ++)
    {
        ret = CTC_WRAPPER(vlanGet, port, &vlanCfg);
        if(EPON_OAM_ERR_OK == ret)
        {
            switch(vlanCfg.vlanMode)
            {
            case CTC_OAM_VAR_VLAN_MODE_TRANSPARENT:
                printf("%4u transparent n/a\n", port);
                break;
            case CTC_OAM_VAR_VLAN_MODE_TAG:
                printf("%4u tag         %04x/%u/%u/%u\n", port,
                    vlanCfg.cfg.tagCfg.tpid, vlanCfg.cfg.tagCfg.pri, vlanCfg.cfg.tagCfg.cfi, vlanCfg.cfg.tagCfg.vid);
                break;
            case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
                printf("%4u translation def %04x/%u/%u/%u\n", port,
                    vlanCfg.cfg.transCfg.defVlan.tpid,
                    vlanCfg.cfg.transCfg.defVlan.pri,
                    vlanCfg.cfg.transCfg.defVlan.cfi,
                    vlanCfg.cfg.transCfg.defVlan.vid);
                for(i = 0 ; i < vlanCfg.cfg.transCfg.num ; i++)
                {
                    printf("                 %04x/%u/%u/%u => %04x/%u/%u/%u\n", 
                    vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.tpid,
                    vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.pri,
                    vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.cfi,
                    vlanCfg.cfg.transCfg.transVlanPair[i].oriVlan.vid,
                    vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.tpid,
                    vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.pri,
                    vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.cfi,
                    vlanCfg.cfg.transCfg.transVlanPair[i].newVlan.vid);
                }
                /* Release vlan config */
                free(vlanCfg.cfg.transCfg.transVlanPair);
                break;
            case CTC_OAM_VAR_VLAN_MODE_AGGREGATION:
                printf("%4u aggregation def %04x/%u/%u/%u\n", port,
                    vlanCfg.cfg.aggreCfg.defVlan.tpid,
                    vlanCfg.cfg.aggreCfg.defVlan.pri,
                    vlanCfg.cfg.aggreCfg.defVlan.cfi,
                    vlanCfg.cfg.aggreCfg.defVlan.vid);
                for(i = 0 ; i < vlanCfg.cfg.aggreCfg.tableNum ; i++)
                {
                    printf("                 %04x/%u/%u/%u <= ", 
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.tpid,
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.pri,
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.cfi,
                    vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid);
                    for(j = 0 ; j < vlanCfg.cfg.aggreCfg.aggrTbl[i].entryNum ; j++)
                    {
                        printf("%04x/%u/%u/%u ", 
                        vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].tpid,
                        vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].pri,
                        vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].cfi,
                        vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].vid);
                    }
                    printf("\n");
                }
                /* Release vlan config */
                for(i = 0 ; i < vlanCfg.cfg.aggreCfg.tableNum ; i++)
                {
                    free(vlanCfg.cfg.aggreCfg.aggrTbl[i].aggreFromVlan);
                }
                free(vlanCfg.cfg.aggreCfg.aggrTbl);
                break;
            case CTC_OAM_VAR_VLAN_MODE_TRUNK:
                printf("%4u trunk       def %04x/%u/%u/%u\n", port,
                    vlanCfg.cfg.trunkCfg.defVlan.tpid,
                    vlanCfg.cfg.trunkCfg.defVlan.pri,
                    vlanCfg.cfg.trunkCfg.defVlan.cfi,
                    vlanCfg.cfg.trunkCfg.defVlan.vid);
                for(i = 0 ; i < vlanCfg.cfg.trunkCfg.num ; i++)
                {
                    printf("                 %04x/%u/%u/%u\n", 
                    vlanCfg.cfg.trunkCfg.acceptVlan[i].tpid,
                    vlanCfg.cfg.trunkCfg.acceptVlan[i].pri,
                    vlanCfg.cfg.trunkCfg.acceptVlan[i].cfi,
                    vlanCfg.cfg.trunkCfg.acceptVlan[i].vid);
                }
                /* Release vlan config */
                free(vlanCfg.cfg.trunkCfg.acceptVlan);
                break;
            default:
                break;
            }
        }
    }

	return EPON_OAM_ERR_OK;
}

int ctc_oam_varCli_multicastSwitch_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    ctc_varDescriptor_t varDesc)
{
	int mcMode;

	mcMode = igmpMcSwitchGet();
	if(mcMode == IGMP_MODE_SNOOPING)
		printf("IGMP mcMode : %s\n", "IGMP snooping");
	else
		printf("IGMP mcMode : %s\n", "CTC mode");
}

/* siyuan 2016-08-05: if vlan is static, vlan setting can't be changed by OLT 
   for the case that boa in charge of vlan setting */
static int vlanIsStatic = 0;
int ctc_oam_vlan_is_static()
{
	return vlanIsStatic;
}

static void ctc_oam_vlan_status_set(int isStatic)
{
	vlanIsStatic = isStatic;
}

void ctc_oam_cli_vlan_set(oam_cli_t *pCli)
{
	ctc_wrapper_vlanCfg_t vlanCfg;
	int ret;
	
	memset(&vlanCfg, 0, sizeof(ctc_wrapper_vlanCfg_t));
    vlanCfg.vlanMode = pCli->u.cliVlanCfg.vlanMode;
	switch(vlanCfg.vlanMode)
    {
    	case CTC_OAM_VAR_VLAN_MODE_TRANSPARENT:
			ret = CTC_WRAPPER(vlanSet, pCli->u.cliVlanCfg.port, &vlanCfg);
			break;
		case CTC_OAM_VAR_VLAN_MODE_TAG:
			vlanCfg.cfg.tagCfg.tpid = 0x8100;
			vlanCfg.cfg.tagCfg.pri = pCli->u.cliVlanCfg.cfg.tagCfg.pri;
            vlanCfg.cfg.tagCfg.vid = pCli->u.cliVlanCfg.cfg.tagCfg.vid;
			ret = CTC_WRAPPER(vlanSet, pCli->u.cliVlanCfg.port, &vlanCfg);
			break;
		case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
			vlanCfg.cfg.transCfg.num = 1;
			vlanCfg.cfg.transCfg.transVlanPair = (ctc_wrapper_vlanTransPair_t *)malloc(sizeof(ctc_wrapper_vlanTransPair_t) * vlanCfg.cfg.transCfg.num);
            if(NULL == vlanCfg.cfg.transCfg.transVlanPair)
            {
                return;
            }
			vlanCfg.cfg.transCfg.transVlanPair[0].newVlan.tpid = 0x8100;
			vlanCfg.cfg.transCfg.transVlanPair[0].newVlan.vid = pCli->u.cliVlanCfg.cfg.transCfg.svlan.vid;
			vlanCfg.cfg.transCfg.transVlanPair[0].newVlan.pri = pCli->u.cliVlanCfg.cfg.transCfg.svlan.pri;
			vlanCfg.cfg.transCfg.transVlanPair[0].oriVlan.tpid = 0x8100;
			vlanCfg.cfg.transCfg.transVlanPair[0].oriVlan.vid = pCli->u.cliVlanCfg.cfg.transCfg.cvlan.vid;
			vlanCfg.cfg.transCfg.transVlanPair[0].oriVlan.pri = pCli->u.cliVlanCfg.cfg.transCfg.cvlan.pri;
			ret = CTC_WRAPPER(vlanSet, pCli->u.cliVlanCfg.port, &vlanCfg);
			break;
		default:
			break;
	}
}

void ctc_oam_cli_proc(
    oam_cli_t *pCli)
{
    int ret;
    ctc_varCb_t varCb;
    ctc_onuAuthLoid_t authLoid;
    ctc_varDescriptor_t varDesc;

    switch(pCli->cliType)
    {
    case CTC_OAM_CLI_LOID_GET:
        ctc_oam_onuAuthLoid_get(pCli->u.cliCtcLoid.llidIdx, &authLoid);
        printf("LOID for LLID %d\n", pCli->u.cliCtcLoid.llidIdx);
        printf("    LOID: %s\n", authLoid.loid);
        printf("password: %s\n", authLoid.password);
        break;
    case CTC_OAM_CLI_LOID_SET:
        strcpy(authLoid.loid, pCli->u.cliCtcLoid.loid);
        strcpy(authLoid.password, pCli->u.cliCtcLoid.password);
        ctc_oam_onuAuthLoid_set(pCli->u.cliCtcLoid.llidIdx, &authLoid);
        break;
    case CTC_OAM_CLI_VAR_GET:
        varDesc.varBranch = pCli->u.cliCtcVar.varBranch;
        varDesc.varLeaf = pCli->u.cliCtcVar.varLeaf;
        ret = ctc_oam_orgSepcVarCb_get(varDesc, &varCb);
        if(EPON_OAM_ERR_OK == ret)
        {
            if(varCb.var_get)
            {
                printf("=====================================================================\n");
                printf("%2u 0x%02x/%04x %s\n\n", pCli->u.cliCtcVar.llidIdx, varDesc.varBranch, varDesc.varLeaf, varCb.varName);
                varCb.var_get(pCli->u.cliCtcVar.llidIdx, varDesc);
                printf("=====================================================================\n");
            }
            else
            {
                printf("var %02x/%04x %s doesn't support var get\n", varDesc.varBranch, varDesc.varLeaf, varCb.varName);
            }
        }
        else
        {
            printf("No such var %02x/%04x\n", varDesc.varBranch, varDesc.varLeaf);
        }
        break;
	case CTC_OAM_CLI_LOOPDETECT_SET:
		ctc_oam_loop_detect_set(pCli->u.cliCtcLoopdetect.loopdetect_enable,pCli->u.cliCtcLoopdetect.ether_type,
			pCli->u.cliCtcLoopdetect.loopdetect_time,pCli->u.cliCtcLoopdetect.loop_recover_time,
			pCli->u.cliCtcLoopdetect.vid);
		break;
	case CTC_OAM_CLI_LOOPDETECT_GET:
		ctc_oam_loopdetect_get();
		break;
    case CTC_OAM_CLI_ALARM_GET:
        ctc_oam_alarm_show(pCli->u.cliAlarmShowDisable);
        break;
	case CTC_OAM_CLI_VLANSTATUS_SET:
		ctc_oam_vlan_status_set(pCli->u.cliEnable.enable);
		break;
	case CTC_OAM_CLI_VLAN_GET:
		printf("vlan status:%s\n", (vlanIsStatic == 0)?"can be changed by olt":"static");
		ctc_oam_varCli_vlan_get(0, varDesc);
		break;
	case CTC_OAM_CLI_VLAN_SET:
		ctc_oam_cli_vlan_set(pCli);
		break;
    default:
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_WARN,
                "[OAM:%s:%d] unsupported CLI type %u\n", __FILE__, __LINE__, pCli->cliType);
        break;
    }
}

