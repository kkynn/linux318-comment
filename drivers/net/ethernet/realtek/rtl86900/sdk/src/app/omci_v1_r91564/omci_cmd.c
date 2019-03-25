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
 * Purpose : Definition of OMCI CLI APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI CLI APIs
 */

#include "app_basic.h"
#include "omci_task.h"


#ifndef OMCI_X86
#if (CONFIG_GPON_VERSION >1)
#include <module/gpon/gpon.h>
#else
#include <rtk/gpon.h>
#endif
#endif

static void omci_cmd_set_help(void)
{
    printf("\nUsage: omcicli set [cmd]\n\n");
    printf("  sn        : set serial number\n");
    printf("    {vendorId, serialNumber}\n");
    printf("  log [UsrLogLvl] [DrvLogLvl]     : set runtime log level\n");
    printf("    {off | err | warn | info | dbg}\n");
    printf("  logfile   : log omci msg to file (defautl at /tmp)\n");
    printf("    {mode (mask) 0:Off | 1:Raw | 2:Parsed | 4:with TimeStamp}\n");
    printf("    [actMask 0:Default | value[, fileName]]\n");
    printf("        -list of omci action masks-\n");
    printf("        [%-22s0x%x]\n", "create", (1 << OMCI_MSG_TYPE_CREATE));
    printf("        [%-22s0x%x]\n", "delete", (1 << OMCI_MSG_TYPE_DELETE));
    printf("        [%-22s0x%x]\n", "set", (1 << OMCI_MSG_TYPE_SET));
    printf("        [%-22s0x%x]\n", "get", (1 << OMCI_MSG_TYPE_GET));
    printf("        [%-22s0x%x]\n", "get all alarms", (1 << OMCI_MSG_TYPE_GET_ALL_ALARMS));
    printf("        [%-22s0x%x]\n", "get all alarms next", (1 << OMCI_MSG_TYPE_GET_ALL_ALARMS_NEXT));
    printf("        [%-22s0x%x]\n", "mib upload", (1 << OMCI_MSG_TYPE_MIB_UPLOAD));
    printf("        [%-22s0x%x]\n", "mib upload next", (1 << OMCI_MSG_TYPE_MIB_UPLOAD_NEXT));
    printf("        [%-22s0x%x]\n", "mib reset", (1 << OMCI_MSG_TYPE_MIB_RESET));
    printf("        [%-22s0x%x]\n", "alarm", (1 << OMCI_MSG_TYPE_ALARM));
    printf("        [%-22s0x%x]\n", "attr value change", (1 << OMCI_MSG_TYPE_ATTR_VALUE_CHANGE));
    printf("        [%-22s0x%x]\n", "test", (1 << OMCI_MSG_TYPE_TEST));
    printf("        [%-22s0x%x]\n", "start sw download", (1 << OMCI_MSG_TYPE_START_SW_DOWNLOAD));
    printf("        [%-22s0x%x]\n", "download section", (1 << OMCI_MSG_TYPE_DOWNLOAD_SECTION));
    printf("        [%-22s0x%x]\n", "end sw download", (1 << OMCI_MSG_TYPE_END_SW_DOWNLOAD));
    printf("        [%-22s0x%x]\n", "activate sw", (1 << OMCI_MSG_TYPE_ACTIVATE_SW));
    printf("        [%-22s0x%x]\n", "commit sw", (1 << OMCI_MSG_TYPE_COMMIT_SW));
    printf("        [%-22s0x%x]\n", "synchronize time", (1 << OMCI_MSG_TYPE_SYNCHRONIZE_TIME));
    printf("        [%-22s0x%x]\n", "reboot", (1 << OMCI_MSG_TYPE_REBOOT));
    printf("        [%-22s0x%x]\n", "get next", (1 << OMCI_MSG_TYPE_GET_NEXT));
    printf("        [%-22s0x%x]\n", "test result", (1 << OMCI_MSG_TYPE_TEST_RESULT));
    printf("        [%-22s0x%x]\n", "get current data", (1 << OMCI_MSG_TYPE_GET_CURRENT_DATA));
    printf("        [%-22s0x%x]\n", "set table", (1 << OMCI_MSG_TYPE_SET_TABLE));
    printf("  devmode   : set omci device mode\n");
    printf("    {router | bridge | hybrid}\n");
    printf("  dmmode    : set dual mgmt mode\n");
    printf("    {off | on_wq | on_bc_mc | on_wq_bc_mc}\n");
    printf("  loid      : set loid and password\n");
    printf("    {loid [, passwd]}\n");
    printf("  resetlauth : Reset loid auth number\n");
    printf("  dhcp       : set dhcp info\n");
    printf("    {IPHost Id, IpV4, Mask}\n");
    printf("  fakeok    : forcenop/enable/disable fake ok\n");
    printf("    {off (default) | on | nop}\n");
    printf("  pm        : set pm running state\n");
    printf("    [stop | start (default)]\n");
    printf("  tm        : set tm running state\n");
    printf("    [stop | start (default)]\n");
    printf("  iotvlancfg    : set IOT vlan setting\n");
    printf("    {type 0:Auto detect | 1:Manual}\n");
    printf("    [mode 0:Transparent | 1:Tagging | 2:Remote Access control | 3:Special]\n");
    printf("    [vid]\n");
    printf("        -vlan range-\n");
    printf("        [%-22s]\n", "0-4095");
    printf("        [%-22s]\n", "0xFFFF is unspecific");
    printf("    [pri]\n");
    printf("        -priority range-\n");
    printf("        [%-22s]\n", "0-7");
    printf("        [%-22s]\n", "0xFF is unspecific");
    printf("  cflag     : set customized flag \n");
    printf("    {type, value (Dec)}\n");
    printf("        -list of cflag type-\n");
    printf("        [%-22s]\n", "bdp");
    printf("        [%-22s]\n", "rdp");
    printf("        [%-22s]\n", "multicast");
    printf("        [%-22s]\n", "me");
    printf("\n");
}

static void omci_cmd_get_help(void)
{
    printf("\nUsage: omcicli get [cmd]\n\n");
    printf("  sn        : get serial number\n");
    printf("  log       : get runtime log level\n");
    printf("  logfile   : get omci msg log mode/action mask\n");
    printf("  tables    : get all registered MIB tables\n");
    printf("  devmode   : get omci device mode\n");
    printf("  dmmode    : get dual mgmt mode\n");
    printf("  loid      : get loid and password\n");
    printf("  loidauth  : get loid auth status\n");
    printf("  cflag     : get customized flag\n");
    printf("  authuptime    : get auth uptime\n");
    printf("\n");
}

static GOS_ERROR_CODE omci_cmd_set_sn(char *vendorId, unsigned long serialNumber)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.sn[0] = vendorId[0];
    msg.sn[1] = vendorId[1];
    msg.sn[2] = vendorId[2];
    msg.sn[3] = vendorId[3];
    msg.sn[4] = (serialNumber >> 24) & 0xff;
    msg.sn[5] = (serialNumber >> 16) & 0xff;
    msg.sn[6] = (serialNumber >> 8) & 0xff;
    msg.sn[7] = serialNumber & 0xff;
    msg.cmd = PON_OMCI_CMD_SN_SET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_sn(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_SN_GET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE
omci_cmd_set_logLevel(
    char* usrLogLvl,
    char* drvLogLvl)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    if (0 == strcmp(usrLogLvl, "off"))
    {
        msg.usrLogLvl = OMCI_LOG_LEVEL_OFF;
    }
    else if (0 == strcmp(usrLogLvl, "err"))
    {
        msg.usrLogLvl = OMCI_LOG_LEVEL_ERR;
    }
    else if (0 == strcmp(usrLogLvl, "warn"))
    {
        msg.usrLogLvl = OMCI_LOG_LEVEL_WARN;
    }
    else if (0 == strcmp(usrLogLvl, "info"))
    {
        msg.usrLogLvl = OMCI_LOG_LEVEL_INFO;
    }
    else if (0 == strcmp(usrLogLvl, "dbg"))
    {
        msg.usrLogLvl = OMCI_LOG_LEVEL_DBG;
    }
    else
    {
        omci_cmd_set_help();
        return GOS_OK;
    }

    if (0 == strcmp(drvLogLvl, "off"))
    {
        msg.drvLogLvl = OMCI_LOG_LEVEL_OFF;
    }
    else if (0 == strcmp(drvLogLvl, "err"))
    {
        msg.drvLogLvl = OMCI_LOG_LEVEL_ERR;
    }
    else if (0 == strcmp(drvLogLvl, "warn"))
    {
        msg.drvLogLvl = OMCI_LOG_LEVEL_WARN;
    }
    else if (0 == strcmp(drvLogLvl, "info"))
    {
        msg.drvLogLvl = OMCI_LOG_LEVEL_INFO;
    }
    else if (0 == strcmp(drvLogLvl, "dbg"))
    {
        msg.drvLogLvl = OMCI_LOG_LEVEL_DBG;
    }
    else
    {
        omci_cmd_set_help();

        return GOS_OK;
    }

    msg.cmd = PON_OMCI_CMD_LOG_SET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_logLevel(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_LOG_GET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_logfile(int argc, char *argv[])
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_LOGFILE_SET;
    msg.state = strtol(argv[1], NULL, 0);
    msg.level = 0;
    if (msg.state < 0 || msg.state > 8)
    {
        printf("mode only support 0 ~ 8\n");

        return GOS_OK;
    }
    if (argc > 2)
    {
        msg.level = strtol(argv[2], NULL, 16);
    }
    if (msg.level > 0xffffffff)
    {
        printf("level only support 0 ~ 0xffffffff\n");

        return GOS_OK;
    }

    if (8 == msg.state && argc > 3)
    {
        printf("mode=8 only support log output to console \n");
        return GOS_OK;
    }

    if (argc > 3)
    {
        if(strlen(argv[3]) >= sizeof(msg.filename))
        {
            return GOS_ERR_OVERFLOW;
        }
        strcpy(msg.filename, argv[3]);
    }

    if (8 == msg.state)
    {
        snprintf(msg.filename, sizeof(msg.filename), "stdout");
    }

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_logfile(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_LOGFILE_GET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_tables(void)
{
    PON_OMCI_CMD_T msg;

    msg.cmd = PON_OMCI_CMD_TABLE_GET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_devmode(char *devMode)
{
    PON_OMCI_CMD_T msg;

    if(strlen(devMode) >= sizeof(msg.filename))
    {
        return GOS_ERR_OVERFLOW;
    }

    memset(&msg,0,sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DEVMODE_SET;
    strcpy(msg.filename, devMode);

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_devmode(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg,0,sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DEVMODE_GET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_dmmode(char *state)
{
    PON_OMCI_CMD_T  tOmciCmd;

    memset(&tOmciCmd, 0, sizeof(PON_OMCI_CMD_T));

    // file content
    if (0 == strcmp(state, "off"))
    {
        tOmciCmd.state = 0;
    }
    else if (0 == strcmp(state, "on_wq"))
    {
        tOmciCmd.state = 1;
    }
    else if (0 == strcmp(state, "on_bc_mc"))
    {
        tOmciCmd.state = 2;
    }
    else if (0 == strcmp(state, "on_wq_bc_mc"))
    {
        tOmciCmd.state = 3;
    }
    else
    {
        omci_cmd_set_help();

        return GOS_OK;
    }
    tOmciCmd.cmd = PON_OMCI_CMD_DUAL_MGMT_MODE_SET;

    // send command
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &tOmciCmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_dmmode(void)
{
    PON_OMCI_CMD_T  tOmciCmd;

    memset(&tOmciCmd, 0, sizeof(PON_OMCI_CMD_T));

    // file content
    tOmciCmd.cmd = PON_OMCI_CMD_DUAL_MGMT_MODE_GET;

    // send command
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &tOmciCmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_loid(int argc, char **arg)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_LOID_SET;

    if(strlen(arg[1]) >= sizeof(msg.filename))
    {
        return GOS_ERR_OVERFLOW;
    }
    strcpy(msg.filename, arg[1]); /*LOID*/


    if (argc > 2) /*Password can be empty*/
    {
        if(strlen(arg[2]) >= sizeof(msg.value))
        {
            return GOS_ERR_OVERFLOW;
        }
        strcpy(msg.value, arg[2]);  /*Password*/
    }
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_loid(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_LOID_GET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_loidAuthStatus(void)
{
    PON_OMCI_CMD_T  msg;
    UINT32          ret;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_LOIDAUTH_GET_RSP;

    ret = omci_SendCmdAndGet(&msg);
    if (GOS_OK != ret)
        return ret;

    printf("Auth Status : %d\n", msg.state);

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_LOIDAUTH_NUM_GET_RSP;

    ret = omci_SendCmdAndGet(&msg);
    if (GOS_OK != ret)
        return ret;

    printf("Auth Num : %d\n", msg.state);
    printf("Auth Success Num : %d\n", msg.level);

    return GOS_OK;
}


static GOS_ERROR_CODE omci_cmd_set_loidAuthNumReset(void)
{
    PON_OMCI_CMD_T  msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_LOIDAUTH_NUM_RESET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_dhcp ( int argc, char** argv )
{
    PON_OMCI_CMD_T msg;
    if_info_t dhcpInfo;

    if (argc < 4)
    {
        return GOS_ERR_INVALID_INPUT;
    }

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));
    memset(&dhcpInfo, 0, sizeof(if_info_t));

    msg.cmd = PON_OMCI_CMD_IPHOST_DHCP_SET;
    //
    // Enable DHCP
    //
    dhcpInfo.if_is_DHCP_B = TRUE;
    //
    // IPHost's Entity Id .
    //
    dhcpInfo.if_id = strtol(argv[1], NULL, 0);
    //
    // IP for DHCP
    //
    dhcpInfo.ip_addr.ipv4_addr = strtoul(argv[2], NULL, 16);
    dhcpInfo.mask_addr.ipv4_mask_addr = strtoul(argv[3], NULL, 16);

    memcpy(msg.value,&dhcpInfo, sizeof(if_info_t));

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get_cflag(void)
{
    PON_OMCI_CMD_T  msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_CFLAG_GET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;

}

static GOS_ERROR_CODE omci_cmd_set_fake_ok(char *state)
{
    PON_OMCI_CMD_T  tOmciCmd;

    memset(&tOmciCmd, 0, sizeof(PON_OMCI_CMD_T));

    // file content
    if (0 == strcmp(state, "off"))
    {
        tOmciCmd.state = 0;
    }
    else if (0 == strcmp(state, "on"))
    {
        tOmciCmd.state = 1;
    }
    else if (0 == strcmp(state, "nop"))
    {
        tOmciCmd.state = 2;
    }
    else
    {
        omci_cmd_set_help();

        return GOS_OK;
    }
    tOmciCmd.cmd = PON_OMCI_CMD_FAKE_OK_SET;

    // send command
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &tOmciCmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_pm_running(char *state)
{
    PON_OMCI_CMD_T  tOmciCmd;

    memset(&tOmciCmd, 0, sizeof(PON_OMCI_CMD_T));

    // file content
    if (0 == strcmp(state, "stop"))
    {
        tOmciCmd.state = FALSE;
    }
    else if (0 == strcmp(state, "start"))
    {
        tOmciCmd.state = TRUE;
    }
    else
    {
        omci_cmd_set_help();

        return GOS_OK;
    }
    tOmciCmd.cmd = PON_OMCI_CMD_PM_SET;

    // send command
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &tOmciCmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_tm_running(char *state)
{
    PON_OMCI_CMD_T  tOmciCmd;

    memset(&tOmciCmd, 0, sizeof(PON_OMCI_CMD_T));

    // file content
    if (0 == strcmp(state, "stop"))
    {
        tOmciCmd.state = FALSE;
    }
    else if (0 == strcmp(state, "start"))
    {
        tOmciCmd.state = TRUE;
    }
    else
    {
        omci_cmd_set_help();

        return GOS_OK;
    }
    tOmciCmd.cmd = PON_OMCI_CMD_TM_SET;

    // send command
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &tOmciCmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_iot_vlan_cfg(int argc, char **arg)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_IOT_VLAN_CFG_SET;
    msg.type = strtol(arg[1], NULL, 10);
    if (PRIVATE_VLANCFG_TYPE_AUTO_DETECTION == msg.type)
    {
        msg.mode = 0xFF;
        msg.vid = 0xFFFF;
        msg.pri = 0xFF;
    }
    else if (PRIVATE_VLANCFG_TYPE_AUTO_DETECTION < msg.type && 4 < argc)
    {
        msg.mode = strtol(arg[2], NULL, 10);
        if (PRIVATE_VLANCFG_MANUAL_MODE_TAGGING == msg.mode)
        {
            msg.vid = strtol(arg[3], NULL, 10);
            if (0xFFF < msg.vid)
                goto help;

            msg.pri = strtol(arg[4], NULL, 10);
            if (0x7 < msg.pri)
                goto help;
        }
    }
    else
    {
        goto help;
    }

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));
    return GOS_OK;

help:
    omci_cmd_set_help();
    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set_customized_flag(int argc, char **arg)
{
    PON_OMCI_CMD_T msg;

    if(strlen(arg[1]) >= sizeof(msg.value))
    {
        return GOS_ERR_OVERFLOW;
    }

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_CFLAG_SET;

    /* cflag type */
    strcpy(msg.value, arg[1]);

    /* cflag value */
    msg.level = strtol(arg[2], NULL, 10);

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;

}

static GOS_ERROR_CODE omci_cmd_get_pon_duration_time(void)
{
    PON_OMCI_CMD_T  msg;
    UINT32          ret;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DURATION_GET_RSP;

    ret = omci_SendCmdAndGet(&msg);
    if (GOS_OK != ret)
        return ret;

    printf("PON duration time : %f seconds\n", msg.difftime);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_set(int argc, char *argv[])
{
    if (argc <= 0)
    {
        omci_cmd_set_help();

        return GOS_OK;
    }

    if (0 == strcmp(argv[0], "sn") && argc > 2)
    {
        omci_cmd_set_sn(argv[1], strtoul(argv[2], NULL, 16));
    }
    else if (0 == strcmp(argv[0], "log") && argc > 2)
    {
        omci_cmd_set_logLevel(argv[1], argv[2]);
    }
    else if (0 == strcmp(argv[0], "logfile") && argc > 1)
    {
        omci_cmd_set_logfile(argc, argv);
    }
    else if (0 == strcmp(argv[0], "devmode") && argc > 1)
    {
        omci_cmd_set_devmode(argv[1]);
    }
    else if (0 == strcmp(argv[0], "dmmode") && argc > 1)
    {
        omci_cmd_set_dmmode(argv[1]);
    }
    else if (0 == strcmp(argv[0], "loid") && argc > 1)
    {
        omci_cmd_set_loid(argc, argv);
    }
    else if (0 == strcmp(argv[0], "dhcp") && argc > 1)
    {
        omci_cmd_set_dhcp (argc, argv);
    }
    else if (0 == strcmp(argv[0], "fakeok") && argc > 1)
    {
        omci_cmd_set_fake_ok(argv[1]);
    }
    else if (0 == strcmp(argv[0], "pm") && argc > 1)
    {
        omci_cmd_set_pm_running(argv[1]);
    }
    else if (0 == strcmp(argv[0], "tm") && argc > 1)
    {
        omci_cmd_set_tm_running(argv[1]);
    }
    else if (0 == strcmp(argv[0], "iotvlancfg") && argc > 1)
    {
        omci_cmd_set_iot_vlan_cfg(argc, argv);
    }
    else if (0 == strcmp(argv[0], "cflag") && argc > 2)
    {
        omci_cmd_set_customized_flag(argc, argv);
    }
    else if (0 == strcmp(argv[0], "resetlauth"))
    {
        omci_cmd_set_loidAuthNumReset();
    }
    else
    {
        omci_cmd_set_help();
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_get(int argc, char *argv[])
{
    if (argc <= 0)
    {
        omci_cmd_get_help();

        return GOS_OK;
    }

    if (0 == strcmp(argv[0], "sn"))
    {
        return omci_cmd_get_sn();
    }
    else if (0 == strcmp(argv[0], "log"))
    {
        return omci_cmd_get_logLevel();
    }
    else if (0 == strcmp(argv[0], "logfile"))
    {
        return omci_cmd_get_logfile();
    }
    else if (0 == strcmp(argv[0], "tables"))
    {
        return omci_cmd_get_tables();
    }
    else if (0 == strcmp(argv[0], "devmode"))
    {
        return omci_cmd_get_devmode();
    }
    else if (0 == strcmp(argv[0], "dmmode"))
    {
        return omci_cmd_get_dmmode();
    }
    else if (0 == strcmp(argv[0], "loid"))
    {
        return omci_cmd_get_loid();
    }
    else if (0 == strcmp(argv[0], "loidauth"))
    {
        return omci_cmd_get_loidAuthStatus();
    }
    else if (0 == strcmp(argv[0], "cflag"))
    {
        return omci_cmd_get_cflag();
    }
    else if (0 == strcmp(argv[0], "authuptime"))
    {
        return omci_cmd_get_pon_duration_time();
    }
    else
    {
        omci_cmd_get_help();
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_mibCreate(int argc, char *argv[])
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    if (strlen(argv[3]) >= sizeof(msg.value))
        return GOS_ERR_OVERFLOW;

    msg.cmd = PON_OMCI_CMD_MIB_CREATE;
    msg.tableId = strtol(argv[1], NULL, 0);
    msg.entityId = strtol(argv[2], NULL, 0);
    memcpy(msg.value, argv[3], strlen(argv[3]));

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_mibDelete(int argc, char *argv[])
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_MIB_DELETE;
    msg.tableId = strtol(argv[1], NULL, 0);
    msg.entityId = strtol(argv[2], NULL, 0);

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_mibSet(int argc, char *argv[])
{
    PON_OMCI_CMD_T msg;

    if(strlen(argv[3]) >= sizeof(msg.filename))
    {
        return GOS_ERR_OVERFLOW;
    }

    if(strlen(argv[4]) >= sizeof(msg.value))
    {
        return GOS_ERR_OVERFLOW;
    }

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_MIB_SET;
    msg.tableId = strtol(argv[1], NULL, 0);
    msg.entityId = strtol(argv[2], NULL, 0);
    strcpy(msg.filename, argv[3]);
    strcpy(msg.value, argv[4]);

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_mibGet(int argc, char *argv[])
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_MIB_GET;
    if (NULL == argv[1])
        return GOS_ERR_PARAM;
    else if (0 == strcmp(argv[1], "all"))
    {
        msg.tableId = -1;
    }
    else
    {
        msg.tableId = 0;

        if(strlen(argv[1]) >= sizeof(msg.filename))
        {
            return GOS_ERR_OVERFLOW;
        }
        strcpy(msg.filename, argv[1]);

        if (argc <= 2 || NULL == argv[2])
            msg.state = -1;
        else
            msg.state = strtol(argv[2], NULL, 0);
    }

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_mibGetCurrent(int argc, char *argv[])
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_MIB_GET_CURRENT;
    if (NULL == argv[1])
        return GOS_ERR_PARAM;

    if(strlen(argv[1]) >= sizeof(msg.filename))
    {
        return GOS_ERR_OVERFLOW;
    }

    strcpy(msg.filename, argv[1]);

    if (argc <= 2 || NULL == argv[2])
        msg.state = -1;
    else
        msg.state = strtol(argv[2], NULL, 0);

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_alarmGet(int argc, char *argv[])
{
    PON_OMCI_CMD_T tOmciCmd;

    memset(&tOmciCmd, 0, sizeof(PON_OMCI_CMD_T));

    // file content
    tOmciCmd.cmd = PON_OMCI_CMD_MIB_ALARM_GET;
    if (argc > 1 && NULL != argv[1])
        tOmciCmd.tableId = strtoul(argv[1], NULL, 0);
    else
        tOmciCmd.tableId = INT_MAX;
    if (argc > 2 && NULL != argv[2])
        tOmciCmd.entityId = strtoul(argv[2], NULL, 0);
    else
        tOmciCmd.entityId = USHRT_MAX;

    // send command
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &tOmciCmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_mibReset(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_MIB_RESET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_mibGetAttr(int argc, char *argv[])
{
    PON_OMCI_CMD_T  msg;
    UINT32          ret;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_MIB_ATTR_GET_RSP;
    //class id
    msg.tableId = strtol(argv[1], NULL, 0);
    //entity id
    msg.entityId = strtol(argv[2], NULL, 0);
    //attribute id
    msg.state = strtol(argv[3], NULL, 0);

    if (GOS_OK != (ret = omci_SendCmdAndGet(&msg)))
        return ret;

    printf("class id:\t%d\n", msg.tableId);
    printf("entity id:\t%u\n", msg.entityId);
    printf("attribute[%d]:\t%s\n", msg.state, msg.value);

    return GOS_OK;
}

static void omci_cmd_mib_help(void)
{
    printf("\nUsage: omcicli mib [cmd]\n\n");
    printf("  create    : create MIB ME\n");
    printf("    {classId, entityId, \"value of all SBC attributes\"}\n");
    printf("  delete    : delete MIB ME\n");
    printf("    {classId, entityId}\n");
    printf("  set       : set MIB ME attribute\n");
    printf("    {classId, entityId, attrName, attrValue}\n");
    printf("  get       : get all MIB or any single ME class\n");
    printf("    [all | classId | tableName[, entityId]]\n");
    printf("  getcurr   : get PM MIB current accumulations\n");
    printf("    {classId | tableName[, entityId]}\n");
    printf("  getalm    : get all alarm or any single ME class\n");
    printf("    [classId[, entityId]]\n");
    printf("  getattr   : get specefic attribute for any single ME class\n");
    printf("    [classId, entityId, attributeId]\n");
    printf("  reset     : trigger MIB reset\n");
    printf("\n");
}

static GOS_ERROR_CODE omci_cmd_mib(int argc, char *argv[])
{
    if (argc <= 0)
    {
        omci_cmd_mib_help();

        return GOS_OK;
    }

    if (0 == strcmp(argv[0], "create") && argc > 3)
    {
        return omci_cmd_mibCreate(argc, argv);
    }
    else if (0 == strcmp(argv[0], "delete") && argc > 2)
    {
        return omci_cmd_mibDelete(argc, argv);
    }
    else if (0 == strcmp(argv[0], "set") && argc > 4)
    {
        return omci_cmd_mibSet(argc, argv);
    }
    else if (0 == strcmp(argv[0], "get") && argc > 1)
    {
        return omci_cmd_mibGet(argc, argv);
    }
    else if (0 == strcmp(argv[0], "getcurr") && argc > 1)
    {
        return omci_cmd_mibGetCurrent(argc, argv);
    }
    else if (0 == strcmp(argv[0], "getalm"))
    {
        return omci_cmd_alarmGet(argc, argv);
    }
    else if (0 == strcmp(argv[0], "getattr") && argc > 1)
    {
        return omci_cmd_mibGetAttr(argc, argv);
    }
    else if (0 == strcmp(argv[0], "reset"))
    {
        return omci_cmd_mibReset();
    }
    else
    {
        omci_cmd_mib_help();
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_dumpAvlTree(char *str)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DUMP_AVL_TREE;
    if (NULL == str)
        msg.state = -1;
    else
        msg.state = atoi(str);

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_dumpQueueMap(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DUMP_QUEUE_MAP;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_dumpTreeConn(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DUMP_TREE_CONN;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_dumpSrvFlow(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DUMP_SRV_FLOW;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_dumpTasks(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DUMP_TASK;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;

}

static void omci_cmd_dump_help(void)
{
    printf("\nUsage: omcicli dump [cmd]\n\n");
    printf("  avltree   : dump MAC bridge AVL tree\n");
    printf("    [avlkeyid]\n");
    printf("        -list of avl tree key id-\n");
    printf("        [%-22s%d]\n", "PPTP Ethernet UNI", AVL_KEY_PPTPUNI);
    printf("        [%-22s%d]\n", "VEIP", AVL_KEY_VEIP);
    printf("        [%-22s%d]\n", "Ext. VLAN OP UNI", AVL_KEY_EXTVLAN_UNI);
    printf("        [%-22s%d]\n", "VLAN OP UNI", AVL_KEY_VLANTAGOPCFG_UNI);
    printf("        [%-22s%d]\n", "VLAN Filter UNI", AVL_KEY_VLANTAGFILTER_UNI);
    printf("        [%-22s%d]\n", "MAC bridge port UNI", AVL_KEY_MACBRIPORT_UNI);
    printf("        [%-22s%d]\n", "MAC bridge service", AVL_KEY_MACBRISERVPROF);
    printf("        [%-22s%d]\n", "MAC bridge port ANI", AVL_KEY_MACBRIPORT_ANI);
    printf("        [%-22s%d]\n", "VLAN Filter ANI", AVL_KEY_VLANTAGFILTER_ANI);
    printf("        [%-22s%d]\n", "VLAN OP ANI", AVL_KEY_VLANTAGOPCFG_ANI);
    printf("        [%-22s%d]\n", "Ext. VLAN OP ANI", AVL_KEY_EXTVLAN_ANI);
    printf("        [%-22s%d]\n", "802.1P mapper", AVL_KEY_MAP8021PSERVPROF);
    printf("        [%-22s%d]\n", "GEM IWTP", AVL_KEY_GEMIWTP);
    printf("        [%-22s%d]\n", "Mcast GEM IWTP", AVL_KEY_MULTIGEMIWTP);
    printf("        [%-22s%d]\n", "GEM Port", AVL_KEY_GEMPORTCTP);
    printf("  qmap      : dump tcont queue mapping\n");
    printf("  conn      : dump data path connections\n");
    printf("  srvflow   : dump data path service flow\n");
    printf("  tasks     : dump tasks\n");
    printf("\n");
}

static GOS_ERROR_CODE omci_cmd_dump(int argc, char *argv[])
{
    if (argc <= 0)
    {
        omci_cmd_dump_help();

        return GOS_OK;
    }

    if (0 == strcmp(argv[0], "avltree"))
    {
        return omci_cmd_dumpAvlTree(argv[1]);
    }
    else if (0 == strcmp(argv[0], "qmap"))
    {
        return omci_cmd_dumpQueueMap();
    }
    else if (0 == strcmp(argv[0], "conn"))
    {
        return omci_cmd_dumpTreeConn();
    }
    else if (0 == strcmp(argv[0], "srvflow"))
    {
        return omci_cmd_dumpSrvFlow();
    }
    else if (0 == strcmp(argv[0], "tasks"))
    {
        return omci_cmd_dumpTasks();
    }
    else
    {
        omci_cmd_dump_help();
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_load_pkt(char *msgOmci)
{
    rtk_gpon_omci_msg_t msg;
    char *endPtr, *curPtr;
    int i;

    memset(&msg, 0, sizeof(rtk_gpon_omci_msg_t));

    curPtr = msgOmci;
    for (i = 0; i < RTK_GPON_OMCI_MSG_LEN; i++)
    {
        msg.msg[i] = strtol(curPtr, &endPtr, 16);
        curPtr = endPtr;
    }

    return OMCI_SendMsg(OMCI_APPL, OMCI_RX_OMCI_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(rtk_gpon_omci_msg_t));
}

static GOS_ERROR_CODE omci_cmd_load_file(char *filename, int start, int end)
{
    FILE *pFile;
    char buf[256];
    char *ptr;
    unsigned int sec, milliSec = 0;
    unsigned int lastMilliSec = 0;
    unsigned int delay;
    unsigned int msgType;

    pFile = fopen(filename, "r");
    if (pFile)
    {
        int cur = 0;
        while (NULL != fgets(buf, sizeof(buf), pFile))
        {
            cur++;
            if ((0 == start && 0 == end) || (cur >= start && cur <= end))
            {
                /*Handle time stamp*/
                if(!strncmp(buf, "[", 1))
                {
                    ptr = strtok(buf,"[.");
                    sec = atoi(ptr);
                    ptr = strtok(NULL,".]");
                    milliSec = atoi(ptr);
                    milliSec += sec * 1000;
                    ptr = strtok(NULL,".]");
                    if(lastMilliSec == 0)
                    {
                        lastMilliSec = milliSec;
                    }
                    delay = (milliSec - lastMilliSec) * 1000;
                } else {
                    /*delay 0.5sec*/
                    delay = 500000;
                    ptr = buf;
                }

                /*only replay AR pkt and download section*/
                sscanf(ptr, "%*s %*s %x", &msgType);
                if((msgType & OMCI_MSG_TYPE_AR_MASK) || ((msgType & OMCI_MSG_TYPE_MT_MASK) == OMCI_MSG_TYPE_DOWNLOAD_SECTION))
                {
                    lastMilliSec = milliSec;
                    usleep(delay);
                    omci_cmd_load_pkt(ptr);
                }
            }
            else
            {
                continue;
            }
        }
        fclose(pFile);
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_simAlarm(int argc, char *argv[])
{
    PON_OMCI_CMD_T tOmciCmd;

    memset(&tOmciCmd, 0, sizeof(PON_OMCI_CMD_T));

    // fill content
    tOmciCmd.cmd = PON_OMCI_CMD_SIM_ALARM;
    tOmciCmd.tableId = strtoul(argv[1], NULL, 0);
    tOmciCmd.level = strtoul(argv[2], NULL, 0);
    tOmciCmd.state = strtoul(argv[3], NULL, 0);
    if (argc > 4 && NULL != argv[4])
        tOmciCmd.entityId = strtoul(argv[4], NULL, 0);
    else
        tOmciCmd.entityId = USHRT_MAX;

    // send command
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &tOmciCmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_simAVC(int argc, char *argv[])
{
    PON_OMCI_CMD_T tOmciCmd;

    memset(&tOmciCmd, 0, sizeof(PON_OMCI_CMD_T));

    // fill content
    tOmciCmd.cmd = PON_OMCI_CMD_SIM_AVC;
    tOmciCmd.tableId = strtoul(argv[1], NULL, 0);
    tOmciCmd.entityId = strtoul(argv[2], NULL, 0);
    tOmciCmd.type = strtoul(argv[3], NULL, 0);

    // send command
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &tOmciCmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_detect_iot_vlan(void)
{
    PON_OMCI_CMD_T cmd;
    memset(&cmd, 0, sizeof(PON_OMCI_CMD_T));

    cmd.cmd = PON_OMCI_CMD_DETECT_IOT_VLAN;
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &cmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_generate_dot(void)
{
    PON_OMCI_CMD_T cmd;
    memset(&cmd, 0, sizeof(PON_OMCI_CMD_T));

    cmd.cmd = PON_OMCI_CMD_GEN_DOT;
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &cmd, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;

}

static GOS_ERROR_CODE omci_cmd_show_reg_module(void)
{
    PON_OMCI_CMD_T cmd;
    memset(&cmd, 0, sizeof(PON_OMCI_CMD_T));

    cmd.cmd = PON_OMCI_CMD_SHOW_REG_MOD;
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &cmd, sizeof(PON_OMCI_CMD_T));
    return GOS_OK;
}

static GOS_ERROR_CODE omci_cmd_show_reg_api(void)
{
    PON_OMCI_CMD_T cmd;
    memset(&cmd, 0, sizeof(PON_OMCI_CMD_T));

    cmd.cmd = PON_OMCI_CMD_SHOW_REG_API;
    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &cmd, sizeof(PON_OMCI_CMD_T));
    return GOS_OK;
}

static void omci_cmd_debug_help(void)
{
    printf("\nUsage: omcicli debug [cmd]\n\n");
    printf("  loadfile      : replay raw omci file\n");
    printf("    {file[, start, end]}\n");
    printf("  loadpkt       : replay raw omci packet\n");
    printf("    {data}\n");
    printf("  simalm        : simulate an alarm\n");
    printf("    {type, number, status[, detail]}\n");
    printf("  simavc        : simulate an AVC\n");
    printf("    {classId, entityId, attrIndex (1 ~ 16)}\n");
    printf("  detectiotvlan     : detect IOT vlan info\n");
    printf("  gendot            : generate a dot file in /tmp/omci_dot_file\n");
    printf("  showregmod        : show register feature module\n");
    printf("  showregapi        : show register feature api\n");
    printf("\n");
}

static GOS_ERROR_CODE omci_cmd_debug(int argc, char *argv[])
{
    if (argc <= 0)
    {
        omci_cmd_debug_help();

        return GOS_OK;
    }

    if (0 == strcmp(argv[0], "loadfile") && argc > 1)
    {
        if (argc <= 2)
            return omci_cmd_load_file(argv[1], 0, 0);
        else if(argc > 3)
        {
            if (GOS_OK == omci_util_is_digit(argv[2]) && GOS_OK == omci_util_is_digit(argv[3]))
            {
                if (atoi(argv[3]) >= atoi(argv[2]))
                    return omci_cmd_load_file(argv[1], atoi(argv[2]), atoi(argv[3]));
                else
                    printf("end index is less than start index !\n");
            }
            else
                printf("start index or end index is not digit start=%s, end=%s !\n", argv[2], argv[3]);
        }
    }
    else if (0 == strcmp(argv[0], "loadpkt") && argc > 1)
    {
        return omci_cmd_load_pkt(argv[1]);
    }
    else if (0 == strcmp(argv[0], "simalm") && argc > 3)
    {
        return omci_cmd_simAlarm(argc, argv);
    }
    else if (0 == strcmp(argv[0], "simavc") && argc > 3)
    {
        return omci_cmd_simAVC(argc, argv);
    }
    else if (0 == strcmp(argv[0], "detectiotvlan"))
    {
        return omci_cmd_detect_iot_vlan();
    }
    else if (0 == strcmp(argv[0], "gendot"))
    {
        return omci_cmd_generate_dot();
    }
    else if (0 == strcmp(argv[0], "showregmod"))
    {
        return omci_cmd_show_reg_module();
    }
    else if (0 == strcmp(argv[0], "showregapi"))
    {
        return omci_cmd_show_reg_api();
    }
    else
    {
        omci_cmd_debug_help();
    }

    return GOS_OK;
}

static void omci_cmd_help(void)
{
    printf("\nUsage: omcicli [cmd]\n\n");
    printf("  set       : set series cmd\n");
    printf("  get       : get series cmd\n");
    printf("  mib       : mib series cmd\n");
    printf("  dump      : dump series cmd\n");
    printf("  debug     : debug series cmd\n");
    printf("\n");
}

#ifdef CONFIG_RELEASE_VERSION
static GOS_ERROR_CODE omci_cmd_get_drv_version(void)
{
    PON_OMCI_CMD_T msg;

    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));

    msg.cmd = PON_OMCI_CMD_DRV_VERSION_GET;

    OMCI_SendMsg(OMCI_APPL, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}
#endif

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        omci_cmd_help();

        return GOS_OK;
    }

    if (0 == strcmp(argv[1], "set"))
    {
        omci_cmd_set(argc-2, &argv[2]);
    }
    else if (0 == strcmp(argv[1], "get"))
    {
        omci_cmd_get(argc-2, &argv[2]);
    }
    else if (0 == strcmp(argv[1], "mib"))
    {
        omci_cmd_mib(argc-2, &argv[2]);
    }
    else if (0 == strcmp(argv[1], "dump"))
    {
        omci_cmd_dump(argc-2, &argv[2]);
    }
    else if (0 == strcmp(argv[1], "debug"))
    {
        omci_cmd_debug(argc-2, &argv[2]);
    }
    else if (0 == strcmp(argv[1], "-v"))
    {
        #ifdef CONFIG_RELEASE_VERSION
            printf("OMCI version     : %s\n", CONFIG_RELEASE_VERSION);
            omci_cmd_get_drv_version();
        #endif
        #ifdef CONFIG_RELEASE_SUBVERSION
            printf("OMCI SVN revision: %s\n", CONFIG_RELEASE_SUBVERSION);
            printf("OMCI Build Time  : %s %s\n", __DATE__, __TIME__);
        #endif
        #if (!defined(CONFIG_RELEASE_VERSION) && !defined(CONFIG_RELEASE_SUBVERSION))
            printf("OMCI Build Time  : %s %s\n", __DATE__, __TIME__);
        #endif
    }
    else
    {
        omci_cmd_help();
    }

    return 0;
}
