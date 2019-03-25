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
 * Purpose : Definition of ME handler: Traffic descriptor (280)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Traffic descriptor (280)
 */

#include "app_basic.h"
#include "feature_mgmt.h"


MIB_TABLE_INFO_T gMibTDtableInfo;
MIB_ATTR_INFO_T  gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ATTR_NUM];
MIB_TABLE_TRAFFICDESCRIPTOR_T gMibTDdefRow;
MIB_TABLE_OPER_T gMibTDoper;


GOS_ERROR_CODE TrafficDescriptorDrvCfg(void             *pOldRow,
                                        void            *pNewRow,
                                        MIB_OPERA_TYPE  operationType,
                                        MIB_ATTRS_SET   attrSet,
                                        UINT32          pri)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_TRAFFICDESCRIPTOR_T   *pNewTD;
    OMCI_GEM_FLOW_ts                flowCfg;
    MIB_TABLE_GEMPORTCTP_T          mibGPNC;
    MIB_TABLE_MACBRIPORTCFGDATA_T   mibMBPCD;
    MIB_TABLE_MAP8021PSERVPROF_T    mib1pMapper;
    UINT16                          *pIwtpPtrOffset;
    UINT8                           i;

    if (MIB_GET == operationType)
        return GOS_OK;

    pNewTD = (MIB_TABLE_TRAFFICDESCRIPTOR_T *)pNewRow;

    if (GOS_OK != omci_is_traffic_descriptor_supported(pNewTD))
        return GOS_ERR_NOTSUPPORT;

    // search gem port who is related to this td
    ret = MIB_GetFirst(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(mibGPNC));
    while (GOS_OK == ret)
    {
        flowCfg.dir = 0;

        if (mibGPNC.UsTraffDescPtr == pNewTD->EntityId)
            flowCfg.dir += PON_GEMPORT_DIRECTION_US;

        if (mibGPNC.DsTraffDescPtr == pNewTD->EntityId)
            flowCfg.dir += PON_GEMPORT_DIRECTION_DS;

        if (flowCfg.dir)
        {
            flowCfg.portId = mibGPNC.PortID;

            omci_apply_traffic_descriptor_to_gem_flow(flowCfg,
                (MIB_DEL == operationType) ? NULL : pNewTD);
        }

        ret = MIB_GetNext(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(mibGPNC));
    }

    // search mac bridge port who is related to this td
    ret = MIB_GetFirst(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(mibMBPCD));
    while (GOS_OK == ret)
    {
        if (MBPCD_TP_TYPE_PPTP_ETH_UNI == mibMBPCD.TPType)
        {
            if (MBPCD_TP_TYPE_PPTP_ETH_UNI == mibMBPCD.TPType)
            {
                if (pNewTD->EntityId == mibMBPCD.InboundTD)
                {
                    omci_apply_traffic_descriptor_to_uni_port(mibMBPCD.TPPointer,
                                                              OMCI_UNI_RATE_DIRECTION_INGRESS,
                                                              (MIB_DEL == operationType ? NULL : pNewTD));
                }
                if (pNewTD->EntityId == mibMBPCD.OutboundTD)
                {
                    omci_apply_traffic_descriptor_to_uni_port(mibMBPCD.TPPointer,
                                                              OMCI_UNI_RATE_DIRECTION_EGRESS,
                                                              (MIB_DEL == operationType ? NULL: pNewTD));
                }
            }
        }
        else if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER != mibMBPCD.TPType &&
                MBPCD_TP_TYPE_GEM_IWTP != mibMBPCD.TPType &&
                MBPCD_TP_TYPE_MCAST_GEM_IWTP != mibMBPCD.TPType)
        {
            // only support rate limiting by gem flows
            goto get_next;
        }
        flowCfg.dir = 0;

        if (mibMBPCD.OutboundTD == pNewTD->EntityId)
        {
            // multicast gem port is used for inbound/ds direction only
            if (MBPCD_TP_TYPE_MCAST_GEM_IWTP != mibMBPCD.TPType)
                flowCfg.dir += PON_GEMPORT_DIRECTION_US;
        }

        if (mibMBPCD.InboundTD == pNewTD->EntityId)
            flowCfg.dir += PON_GEMPORT_DIRECTION_DS;

        if (flowCfg.dir)
        {
            if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == mibMBPCD.TPType)
            {
                mib1pMapper.EntityID = mibMBPCD.TPPointer;

                if (GOS_OK != MIB_Get(MIB_TABLE_MAP8021PSERVPROF_INDEX, &mib1pMapper, sizeof(mib1pMapper)))
                    goto get_next;

                pIwtpPtrOffset = &mib1pMapper.IwTpPtrPbit0;

                for (i = 0; i < 8; i++)
                {
                    mibGPNC.EntityID = *(pIwtpPtrOffset + i);

                    if (GOS_OK != MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(mibGPNC)))
                        continue;

                    flowCfg.portId = mibGPNC.PortID;

                    omci_apply_traffic_descriptor_to_gem_flow(flowCfg,
                        (MIB_DEL == operationType) ? NULL : pNewTD);
                }
            }
            else
            {
                mibGPNC.EntityID = mibMBPCD.TPPointer;

                if (GOS_OK != MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(mibGPNC)))
                    goto get_next;

                flowCfg.portId = mibGPNC.PortID;

                omci_apply_traffic_descriptor_to_gem_flow(flowCfg,
                    (MIB_DEL == operationType) ? NULL : pNewTD);
            }
        }

get_next:
        ret = MIB_GetNext(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(mibMBPCD));
    }

    return ret;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibTDtableInfo.Name = "TrafficDescriptor";
    gMibTDtableInfo.ShortName = "TD";
    gMibTDtableInfo.Desc = "Traffic Descriptor";
    gMibTDtableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_TRAFFIC_DESCRIPTOR);
    gMibTDtableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibTDtableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibTDtableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibTDtableInfo.pAttributes = &(gMibTDattrInfo[0]);

	gMibTDtableInfo.attrNum = MIB_TABLE_TRAFFICDESCRIPTOR_ATTR_NUM;
	gMibTDtableInfo.entrySize = sizeof(MIB_TABLE_TRAFFICDESCRIPTOR_T);
	gMibTDtableInfo.pDefaultRow = &gMibTDdefRow;

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CIR";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PIR";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CBS";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PBS";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ColourMode";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IngressColourMarking";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EgressColourMarking";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MeterType";

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Committed information rate";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Peak information rate";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Committed burst size";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Peak burst size";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Colour mode";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Ingress colour marking";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Egress colour marking";
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Meter type";

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CIR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PIR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_CBS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_PBS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_COLOUR_MODE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_INGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_EGRESS_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibTDattrInfo[MIB_TABLE_TRAFFICDESCRIPTOR_METER_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibTDdefRow.EntityId), 0x00, sizeof(gMibTDdefRow.EntityId));
    memset(&(gMibTDdefRow.CIR), 0x00, sizeof(gMibTDdefRow.CIR));
    memset(&(gMibTDdefRow.PIR), 0x00, sizeof(gMibTDdefRow.PIR));
    memset(&(gMibTDdefRow.CBS), 0x00, sizeof(gMibTDdefRow.CBS));
    memset(&(gMibTDdefRow.PBS), 0x00, sizeof(gMibTDdefRow.PBS));
    gMibTDdefRow.ColourMode = TD_COLOUR_MODE_COLOUR_BLIND;
    gMibTDdefRow.IngressColourMarking = TD_INGRESS_COLOUR_NO_MARKING;
    gMibTDdefRow.EgressColourMarking = TD_EGRESS_COLOUR_NO_MARKING;
    gMibTDdefRow.MeterType = TD_METER_TYPE_NOT_SPECIFIED;

    memset(&gMibTDoper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibTDoper.meOperDrvCfg = TrafficDescriptorDrvCfg;
    gMibTDoper.meOperConnCheck = NULL;
    gMibTDoper.meOperDump= omci_mib_oper_dump_default_handler;
	gMibTDoper.meOperConnCfg   = NULL;

	MIB_TABLE_TRAFFICDESCRIPTOR_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibTDtableInfo, &gMibTDoper);

    return GOS_OK;
}
