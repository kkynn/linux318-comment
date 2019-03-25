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
 * Purpose : Definition of ME handler: ANI-G (263)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: ANI-G (263)
 */

#include "app_basic.h"
#include "omci_task.h"
#ifndef OMCI_X86
#include "rtk/gpon.h"
#include "rtk/ponmac.h"
#endif

MIB_TABLE_INFO_T gMibAnigTableInfo;
MIB_ATTR_INFO_T  gMibAnigAttrInfo[MIB_TABLE_ANIG_ATTR_NUM];
MIB_TABLE_ANIG_T gMibAnigDefRow;
MIB_TABLE_OPER_T gMibAnigOper;


static GOS_ERROR_CODE anig_sr_indication_updater(MIB_TABLE_ANIG_T   *pMibAniG)
{
    GOS_ERROR_CODE  ret = GOS_OK;
    unsigned int    usDBRuStatus;

    if (!pMibAniG)
        return GOS_ERR_PARAM;

    ret = omci_wrapper_getUsDBRuStatus(&usDBRuStatus);
    if (GOS_OK != ret)
        return GOS_FAIL;

    pMibAniG->SRInd = usDBRuStatus;

    return ret;
}

static GOS_ERROR_CODE AnigSignalParaOper(OMCI_SIG_TYPE type,
								unsigned int threshold)
{
	OMCI_SIGNAL_PARA_ts signalPara;

	memset(&signalPara, 0, sizeof(OMCI_SIGNAL_PARA_ts));

	signalPara.signal_type = type;
	signalPara.signal_threshold = threshold;

	return omci_wrapper_setSignalParameter(&signalPara);
}

GOS_ERROR_CODE AnigDrvCfg(void              *pOldRow,
                            void            *pNewRow,
                            MIB_OPERA_TYPE  operationType,
                            MIB_ATTRS_SET   attrSet,
                            UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_ANIG_INDEX;
	MIB_TABLE_ANIG_T    mibAniG, *pMibAniG;

    // read out the instanceID
	pMibAniG = (MIB_TABLE_ANIG_T*)pNewRow;
   	mibAniG.EntityID = pMibAniG->EntityID;

    switch (operationType)
    {
        case MIB_ADD:
		{
			double	value;
            if(GOS_OK != MIB_Get(tableIndex, &mibAniG, sizeof(mibAniG)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibAniG.EntityID);

                return GOS_FAIL;
            }

			if (GOS_OK == anig_generic_transceiver_para_updater(
						&value, OMCI_TRANSCEIVER_STATUS_TYPE_RX_POWER))
				// unit: dBm, 0.002 dB granularity
				mibAniG.OpticalSignalLevel = value * 500;

			if (GOS_OK == anig_generic_transceiver_para_updater(
						&value, OMCI_TRANSCEIVER_STATUS_TYPE_TX_POWER))
				// unit: dBm, 0.002 dB granularity
				mibAniG.TranOpticLevel = value * 500;

			/*Update to MIB*/
			MIB_Set(tableIndex, &mibAniG, sizeof(mibAniG));

			ret = AnigSignalParaOper(SIG_TYPE_SF, (unsigned int)mibAniG.SFThreshold);
			ret = AnigSignalParaOper(SIG_TYPE_SD, (unsigned int)mibAniG.SDThreshold);

            ret = mib_alarm_table_add(tableIndex, pNewRow);

            break;
        }
        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ANIG_ARC_INDEX))
            {
                ret = omci_arc_timer_processor(tableIndex,
                    pOldRow, pNewRow, MIB_TABLE_ANIG_ARC_INDEX, MIB_TABLE_ANIG_ARCINTERVAL_INDEX);
            }

			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ANIG_GEMBLKLEN_INDEX))
			{
				ret = omci_wrapper_setGemBlkLen(pMibAniG->GemBlkLen);

			}

			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ANIG_SFTHRESHOLD_INDEX))
			{
				ret = AnigSignalParaOper(SIG_TYPE_SF, (unsigned int)pMibAniG->SFThreshold);
			}

			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ANIG_SDTHRESHOLD_INDEX))
			{
				ret = AnigSignalParaOper(SIG_TYPE_SD, (unsigned int)pMibAniG->SDThreshold);
			}

            break;
		case MIB_GET:
            if(GOS_OK != MIB_Get(tableIndex, &mibAniG, sizeof(mibAniG)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibAniG.EntityID);

                return GOS_FAIL;
            }


			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ANIG_GEMBLKLEN_INDEX))
			{
				UINT16 gemBlkLen;
				omci_wrapper_getGemBlkLen(&gemBlkLen);
				mibAniG.GemBlkLen = gemBlkLen;

			}

			if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_ANIG_SRIND_INDEX))
			{
				anig_sr_indication_updater(&mibAniG);
			}

			if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX))
			{
				double  value;
				if (GOS_OK == anig_generic_transceiver_para_updater(
                            &value, OMCI_TRANSCEIVER_STATUS_TYPE_RX_POWER))
                // unit: dBm, 0.002 dB granularity
                mibAniG.OpticalSignalLevel = value * 500;
			}

			if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX))
			{
				double  value;
				if (GOS_OK == anig_generic_transceiver_para_updater(
                            &value, OMCI_TRANSCEIVER_STATUS_TYPE_TX_POWER))
                // unit: dBm, 0.002 dB granularity
                mibAniG.TranOpticLevel = value * 500;
			}
			/*Update to MIB*/
			MIB_Set(tableIndex, &mibAniG, sizeof(mibAniG));
			break;
        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE anig_alarm_handler(MIB_TABLE_INDEX        tableIndex,
                                            omci_alm_data_t     alarmData,
                                            omci_me_instance_t  *pInstanceID,
                                            BOOL                *pIsUpdated)
{
    mib_alarm_table_t   alarmTable;
    BOOL                isSuppressed;
    MIB_ENTRY_T         *pEntry;
    MIB_TABLE_ANIG_T    *pMibAniG;
    omci_me_instance_t  cpInstanceID;
    INT16               slotId = -1, portId = -1;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    // portId should be the pon port number (if more than one, 0-based)
    portId = (alarmData.almDetail & 0xFF) + 1;
    slotId = TXC_CARDHLD_PON_SLOT_TYPE_ID;

    *pInstanceID = (slotId << 8) | portId;

    if (GOS_OK != mib_alarm_table_get(tableIndex, *pInstanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), *pInstanceID);

        return GOS_FAIL;
    }

    // update alarm status if it has being changed
    mib_alarm_table_update(&alarmTable, &alarmData, pIsUpdated);

    if (*pIsUpdated)
    {
        if (GOS_OK != mib_alarm_table_set(tableIndex, *pInstanceID, &alarmTable))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set alarm table fail: %s, 0x%x",
                MIB_GetTableName(tableIndex), *pInstanceID);

            return GOS_FAIL;
        }

        // correct the circuit pack instanceID to the slot it belongs to
        cpInstanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | slotId;

        // check if notifications are suppressed by parent's admin state
        omci_is_notify_suppressed_by_circuitpack(cpInstanceID, &isSuppressed);

        if (isSuppressed)
            *pIsUpdated = FALSE;
        else
        {
            // find the entry data by using the instanceID
            pEntry = mib_FindEntryByInstanceID(tableIndex, *pInstanceID);
            if (!pEntry)
                return GOS_FAIL;
            pMibAniG = pEntry->pData;
            if (!pMibAniG)
                return GOS_FAIL;

            // check if notifications are suppressed by ARC
            if (OMCI_ME_ATTR_ARC_ENABLED == pMibAniG->ARC)
                *pIsUpdated = FALSE;
        }
    }

    return GOS_OK;
}

/* BEGIN: Modified by huangyl, 2019/1/16; 解决对接华为OLT，OMCC版本小于0x86时，上报 BOSA 参数失败的问题 */
#define HUAWEI_VENDOR  0x48575443
static GOS_ERROR_CODE anig_test_handler(void    *pData)
{
    GOS_ERROR_CODE              ret = GOS_OK;
    omci_msg_norm_baseline_t    *pOmciMsg;
    MIB_TABLE_INDEX             tableIndex = MIB_TABLE_ANIG_INDEX;
    MIB_TABLE_ANIG_T            mibAniG;
    UINT16                      tempBuf;
    double                      value;

    // make sure the data is available
    if (!pData)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Test data unavailable to proceed: %s",
            MIB_GetTableName(tableIndex));

        return GOS_ERR_PARAM;
    }
    pOmciMsg = (omci_msg_norm_baseline_t *)pData;

    // fill in header
    pOmciMsg->db     = 0;
    pOmciMsg->ar     = 0;
    pOmciMsg->ak     = 0;
    pOmciMsg->type   = OMCI_MSG_TYPE_TEST_RESULT;

    MIB_TABLE_ONT2G_T   mibOnu2G;
    mibOnu2G.EntityID = TXC_ONU2G_INSTANCE_ID;
    // read out the entry data
    MIB_Get(MIB_TABLE_ONT2G_INDEX, &mibOnu2G, sizeof(mibOnu2G));


    MIB_TABLE_OLTG_T mibOltG;
    mibOltG.EntityId = 0;
    MIB_Get(MIB_TABLE_OLTG_INDEX, &mibOltG, sizeof(mibOltG));
    
    // check if the we support the test type
    //if (ANIG_TEST_TYPE_SELF_TEST != pOmciMsg->content[0] )
    //    goto out;

    memset(pOmciMsg->content, 0, OMCI_MSG_BASELINE_CONTENTS_LEN);

    // read out the entry data
    mibAniG.EntityID = pOmciMsg->meId.meInstance;
    ret = MIB_Get(tableIndex, &mibAniG, sizeof(mibAniG));

    // update all supported parameters
    if (GOS_OK == anig_generic_transceiver_para_updater(
                    &value, OMCI_TRANSCEIVER_STATUS_TYPE_RX_POWER))
    {
        // unit: dBm, 0.002 dB granularity
        mibAniG.OpticalSignalLevel = value * 500;

        // unit: dBu (dBm = 10*log(mW), dBu = 10*log(mw*10^3) = dBm+30), 0.002 dB resolution
        tempBuf = (value + 30) * 500;    
        
        if( mibOnu2G.OMCCVer < ONU2G_OMCC_VERSION_0x86 && mibOltG.OltVendorId == HUAWEI_VENDOR)
        {
            //a solution for huawei olt issue when omcc  < 134
            if(tempBuf > 65000)
                tempBuf = 15620;
            else if((tempBuf > 0)&&(tempBuf <= 500)) //-30 ~  -29
                tempBuf = 15000 + 500 - tempBuf;
            else if((tempBuf > 500)&&(tempBuf <= 1000)) //-29~  -28
                tempBuf = 14500 + 1000 - tempBuf;
            else if((tempBuf > 1000)&&(tempBuf <= 1500)) //-28  -27
                tempBuf = 14000 + 1500 - tempBuf;
            else if((tempBuf > 1500)&&(tempBuf <= 2000)) //-27 -26
                tempBuf = 13500 + 2000 - tempBuf;
            else if((tempBuf > 2000)&&(tempBuf <= 2500)) //-26 - 25
                tempBuf = 13000 + 2500 - tempBuf;
            else if((tempBuf > 2500)&&(tempBuf <= 3000)) //-25- 24
                tempBuf = 12500 + 3000 - tempBuf;
            else if((tempBuf > 3000)&&(tempBuf <= 3500)) //-24- 23
                tempBuf = 12000 + 3500 - tempBuf;
            else if((tempBuf > 3500)&&(tempBuf <= 4000)) //-23- 22
                tempBuf = 11500 + 4000 - tempBuf;
            else if((tempBuf > 4000)&&(tempBuf <= 4500)) //-22  -21
                tempBuf = 11000 + 4500 - tempBuf;
            else if((tempBuf > 4500)&&(tempBuf <= 5000)) //-21 -20
                tempBuf = 10500 + 5000 - tempBuf;
            else if((tempBuf > 5000)&&(tempBuf <= 5500)) //-20- 19
                tempBuf = 10000 + 5500 - tempBuf;
            else if((tempBuf > 5500)&&(tempBuf <= 6000)) //-19- 18
                tempBuf = 9500 + 6000 - tempBuf;
            else if((tempBuf > 6000)&&(tempBuf <= 6500)) //-18 ~ 17
                tempBuf = 9000 + 6500 - tempBuf;
            else if((tempBuf > 6500)&&(tempBuf <= 7000)) //-17- 16
                tempBuf = 8500 + 7000 - tempBuf;
            else if((tempBuf > 7000)&&(tempBuf <= 7500)) //-16 ~1 5
                tempBuf = 8000 + 7500 - tempBuf;
            else if((tempBuf > 7500)&&(tempBuf <= 8000)) //-15  -14
                tempBuf = 7500 + 8000 - tempBuf;
            else if((tempBuf > 8000)&&(tempBuf <= 8500)) //-14 -13
                tempBuf = 7000 + 8500 - tempBuf;
            else if((tempBuf > 8500)&&(tempBuf <= 9000)) //-13- 12
                tempBuf = 6500 + 9000 - tempBuf;
            else if((tempBuf > 9000)&&(tempBuf <= 9500)) //-12- 11
                tempBuf = 6000 + 9500 - tempBuf;
            else if((tempBuf > 9500)&&(tempBuf <= 10000)) //-11 ~ 10
                tempBuf = 5500 + 10000 - tempBuf;
            else if((tempBuf > 10000)&&(tempBuf <= 10500)) //-10- 9
                tempBuf = 5000 + 10500 - tempBuf;
            else if((tempBuf > 10500)&&(tempBuf <= 11000)) //-9 ~ 8
                tempBuf = 4500 + 11000 - tempBuf;
            else if((tempBuf > 11000)&&(tempBuf <= 11500)) //-8 ~ 7
                tempBuf = 4000 + 11500 - tempBuf;
        }
        
        memcpy(&pOmciMsg->content[4], &tempBuf, sizeof(UINT16));
        pOmciMsg->content[3] = ANIG_TEST_SELF_TEST_RESULT_RX_OPTICAL_POWER;
    }
    if (GOS_OK == anig_generic_transceiver_para_updater(
                    &value, OMCI_TRANSCEIVER_STATUS_TYPE_TX_POWER))
    {
        // unit: dBm, 0.002 dB granularity
        mibAniG.TranOpticLevel = value * 500;

        // unit: dBu (dBm = 10*log(mW), dBu = 10*log(mw*10^3) = dBm+30), 0.002 dB resolution
        tempBuf = (value + 30) * 500;

        if( mibOnu2G.OMCCVer < ONU2G_OMCC_VERSION_0x86 && mibOltG.OltVendorId == HUAWEI_VENDOR )
        {
            //a solution for huawei olt issue when omcc  < 134
            if(tempBuf >= 15000) //>=0
                tempBuf -= 15000;
            else
                tempBuf = 15000; //=0
        }
        
        memcpy(&pOmciMsg->content[7], &tempBuf, sizeof(UINT16));
        pOmciMsg->content[6] = ANIG_TEST_SELF_TEST_RESULT_MEAN_OPTICAL_POWER;
    }
    if (GOS_OK == anig_generic_transceiver_para_updater(
                    &value, OMCI_TRANSCEIVER_STATUS_TYPE_VOLTAGE))
    {

        if( mibOnu2G.OMCCVer < ONU2G_OMCC_VERSION_0x86 && mibOltG.OltVendorId == HUAWEI_VENDOR)
        {
            tempBuf = value/10; //pooson
        }
        else
        {
            // unit: V, 20 mV resolution
            tempBuf = value / 200;     
        }

        memcpy(&pOmciMsg->content[1], &tempBuf, sizeof(UINT16));
        pOmciMsg->content[0] = ANIG_TEST_SELF_TEST_RESULT_POWER_FEED_VOLTAGE;
    }
    if (GOS_OK == anig_generic_transceiver_para_updater(
                    &value, OMCI_TRANSCEIVER_STATUS_TYPE_BIAS_CURRENT))
    { 
        if( mibOnu2G.OMCCVer < ONU2G_OMCC_VERSION_0x86 && mibOltG.OltVendorId == HUAWEI_VENDOR )
        {
            tempBuf = (value * 2) /1000.0;//pooson
        }
        else
        {
            // unit: mA, 2 uA resolution
            tempBuf = value;           
        }
 
        memcpy(&pOmciMsg->content[10], &tempBuf, sizeof(UINT16));
        pOmciMsg->content[9] = ANIG_TEST_SELF_TEST_RESULT_LASER_BIAS_CURRENT;
    }
    if (GOS_OK == anig_generic_transceiver_para_updater(
                    &value, OMCI_TRANSCEIVER_STATUS_TYPE_TEMPERATURE))
    { 
        if( mibOnu2G.OMCCVer < ONU2G_OMCC_VERSION_0x86 && mibOltG.OltVendorId == HUAWEI_VENDOR )
        {
            tempBuf = value;//pooson 
            pOmciMsg->content[13] = (tempBuf & 0xFF00) >> 8;      
            pOmciMsg->content[14] = tempBuf & 0x00FF;
        }
        else
        {
            // unit: degrees (C), 1/256 resolution
            tempBuf = value * 256;
            memcpy(&pOmciMsg->content[13], &tempBuf, sizeof(UINT16));
        }
        
        pOmciMsg->content[12] = ANIG_TEST_SELF_TEST_RESULT_TEMPERATURE;
    }

    // save to database if necessary
    if (GOS_OK == ret)
    {
        // invoke AVC callback and update attribute value
        MIB_Set(tableIndex, &mibAniG, sizeof(mibAniG));
    }

//out:
    // send back the response
    ret = OMCI_SendMsg(OMCI_APPL,
                        OMCI_TX_OMCI_MSG,
                        OMCI_MSG_PRI_NORMAL,
                        pOmciMsg,
                        sizeof(omci_msg_norm_baseline_t));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Unable to send out test result: %s",
            MIB_GetTableName(tableIndex));
    }

    // free allocated memory before leaving
    free(pData);

    return ret;
}
/* END:   Modified by huangyl, 2019/1/16 */

static GOS_ERROR_CODE anig_avc_cb(MIB_TABLE_INDEX   tableIndex,
                                    void            *pOldRow,
                                    void            *pNewRow,
                                    MIB_ATTRS_SET   *pAttrsSet,
                                    MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    MIB_ATTRS_SET       avcAttrSet;
    BOOL                isSuppressed;
    omci_me_instance_t  instanceID;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pNewRow, sizeof(omci_me_instance_t));

    // correct the instanceID to the slot it belongs to
    instanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | (instanceID >> 8);

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(instanceID, &isSuppressed);

    if (isSuppressed)
        MIB_ClearAttrSet(&avcAttrSet);
    else
    {
        avcAttrSet = *pAttrsSet;

        for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
                i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
        {
            if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
                continue;

            MIB_GetAttrSize(tableIndex, attrIndex);
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibAnigTableInfo.Name = "Anig";
    gMibAnigTableInfo.ShortName = "ANIG";
    gMibAnigTableInfo.Desc = "Ani-g";
    gMibAnigTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ANI_G);
    gMibAnigTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibAnigTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibAnigTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_TEST);
    gMibAnigTableInfo.pAttributes = &(gMibAnigAttrInfo[0]);
	gMibAnigTableInfo.attrNum = MIB_TABLE_ANIG_ATTR_NUM;
	gMibAnigTableInfo.entrySize = sizeof(MIB_TABLE_ANIG_T);
	gMibAnigTableInfo.pDefaultRow = &gMibAnigDefRow;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SRInd";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NumOfTcont";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].Name = "GemBlkLen";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PbDbaRpt";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OnuDbaRpt";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SFThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SDThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARC";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARCInterval";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OpticalSignalLevel";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LowOpThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UppOpThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OntRspTime";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].Name = "TranOpticLevel";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].Name = "LowTranPowThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].Name = "UppTranPowThreshold";

	gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "SR Indication";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Total T-CONT Number";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "GEM Block Length";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Piggyback DBA Reporting";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Whole ONU DBA Reporting";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "SF threshold";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "SD threshold";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Alarm Report Control";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ARC Interval";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Optical Signal Level";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Lower Optical Threshold";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upper Optical Threshold";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ONT Response time";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].Desc = "Transmit Optic Level";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].Desc = "Lower Transmit Power Threshold";
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].Desc = "Upper Transmit Power Threshold";

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].DataType =MIB_ATTR_TYPE_UINT8;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].Len = 2;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SRIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_NUMOFTCONT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_GEMBLKLEN_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_PBDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONUDBARPT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SFTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_SDTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_OPTICALSIGNALLEVEL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPOPTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_ONTRSPTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_TRANOPTICLEVEL_INDEX- MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOWTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_UPPTRANPOWTHRESHOLD_INDEX- MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
	gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].Name = "LowDflRxThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].Desc = "Lower Default Rx Threshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].Name = "UppDflRxThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].Desc = "Upper Default Rx Threshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_RX_THR_IDX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].Name = "LowDflTxThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].Desc = "Lower Default Tx Threshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_LOW_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].Name = "UppDflTxThreshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].Desc = "Upper Default Tx Threshold";
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibAnigAttrInfo[MIB_TABLE_ANIG_UPP_DFL_TX_THR_IDX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;


    memset(&(gMibAnigDefRow.EntityID), 0x00, sizeof(gMibAnigDefRow.EntityID));
    memset(&(gMibAnigDefRow.SRInd), 0x00, sizeof(gMibAnigDefRow.SRInd));
    memset(&(gMibAnigDefRow.NumOfTcont), 0x00, sizeof(gMibAnigDefRow.NumOfTcont));
    gMibAnigDefRow.GemBlkLen = 48;
    memset(&(gMibAnigDefRow.PbDbaRpt), 0x00, sizeof(gMibAnigDefRow.PbDbaRpt));
    memset(&(gMibAnigDefRow.OnuDbaRpt), 0x00, sizeof(gMibAnigDefRow.OnuDbaRpt));
    gMibAnigDefRow.SFThreshold = 5;
    gMibAnigDefRow.SDThreshold = 9;
    memset(&(gMibAnigDefRow.ARC), 0x00, sizeof(gMibAnigDefRow.ARC));
    memset(&(gMibAnigDefRow.ARCInterval), 0x00, sizeof(gMibAnigDefRow.ARCInterval));
	memset(&(gMibAnigDefRow.OpticalSignalLevel), 0x00, sizeof(gMibAnigDefRow.OpticalSignalLevel));
    gMibAnigDefRow.LowOpThreshold = 0xFF;
    gMibAnigDefRow.UppOpThreshold = 0xFF;
	memset(&(gMibAnigDefRow.OntRspTime), 0x00, sizeof(gMibAnigDefRow.OntRspTime));
	memset(&(gMibAnigDefRow.TranOpticLevel), 0x00, sizeof(gMibAnigDefRow.TranOpticLevel));
	gMibAnigDefRow.LowTranPowThreshold = 0x81;
	gMibAnigDefRow.UppTranPowThreshold = 0x81;

    gMibAnigDefRow.LowDflRxThreshold = 0x38; //-28
    gMibAnigDefRow.UppDflRxThreshold = 0x10; //-8
    gMibAnigDefRow.LowDflTxThreshold = 0xF0; //-8
    gMibAnigDefRow.UppDflTxThreshold = 0x06; //+3

    memset(&gMibAnigOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibAnigOper.meOperDrvCfg = AnigDrvCfg;
	gMibAnigOper.meOperConnCheck = NULL;
	gMibAnigOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibAnigOper.meOperConnCfg = NULL;
    gMibAnigOper.meOperAlarmHandler = anig_alarm_handler;
    gMibAnigOper.meOperTestHandler = anig_test_handler;

	MIB_TABLE_ANIG_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibAnigTableInfo,&gMibAnigOper);
    MIB_RegisterCallback(tableId, NULL, anig_avc_cb);

    return GOS_OK;
}
