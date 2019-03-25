
static ctc_varCb_t ctc_stdAttrCb[] = {
    /*  The leaf value should be sorted
     *  { varBranch, varLeaf },
     *  allowed_op,
     *  target,
     *  get_callback, set_callback,
     *  varName
     */
    {
        /* aPhyAdminState */
        { 0x07, 0x0025 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_aPhyAdminState_get, NULL,
        "aPhyAdminState", 
        ctc_oam_varCli_phyAdminState_get
    },
    {
        /* aAutoNegAdminState */
        { 0x07, 0x004F },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_aAutoNegAdminState_get, NULL,
        "aAutoNegAdminState", 
        ctc_oam_varCli_autoNegAdminState_get
    },
    {
        /* aAutoNegLocalTechnologyAbility */
        { 0x07, 0x0052 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_aAutoNegLocalTechnologyAbility_get, NULL,
        "aAutoNegLocalTechnologyAbility"
    },
    {
        /* aAutoNegAdvertisedTechnologyAbility */
        { 0x07, 0x0053 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_aAutoNegAdvertisedTechnologyAbility_get, NULL,
        "aAutoNegAdvertisedTechnologyAbility"
    },
    {
        /* aFECAbility */
        { 0x07, 0x0139 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_aFECAbility_get, NULL,
        "aFECAbility"
    },
    {
        /* aFECmode */
        { 0x07, 0x013A },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_aFecMode_get, ctc_oam_varCb_aFecMode_set,
        "aFECmode"
    },
    {
        /* End indicator - must be the last one */
        { 0x00, 0x0000 },
        0,
        0,        
        NULL, NULL,
        ""
    }
};
    
static ctc_varCb_t ctc_stdActCb[] = {
    /*  The leaf value should be sorted
     *  { varBranch, varLeaf },
     *  allowed_op,
     *  target,
     *  get_callback, set_callback,
     *  varName
     */
    {
        /* acPhyAdminControl */
        { 0x09, 0x0005 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        NULL, ctc_oam_varCb_acPhyAdminControl_set,
        "acPhyAdminControl"
    },
    {
        /* acAutoNegRestartAutoConfig */
        { 0x09, 0x000B },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        NULL, ctc_oam_varCb_acAutoNegRestartAutoConfig_set,
        "acAutoNegRestartAutoConfig"
    },
    {
        /* acAutoNegAdminControl */
        { 0x09, 0x000C },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        NULL, ctc_oam_varCb_acAutoNegAdminControl_set,
        "acAutoNegAdminControl"
    },
    {
        /* End indicator - must be the last one */
        { 0x00, 0x0000 },
        0,
        0,        
        NULL, NULL,
        ""
    }
};

static ctc_varCb_t ctc_extAttrCb[] = {
    /*  The leaf value should be sorted
     *  { varBranch, varLeaf },
     *  allowed_op,
     *  target,
     *  get_callback, set_callback,
     *  varName
     */
    {
        /* ONU SN */
        { 0xC7, 0x0001 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_onuSn_get, NULL,
        "ONU SN"
    },
    {
        /* FirmwareVer */
        { 0xC7, 0x0002 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_firmwareVer_get, NULL,
        "FirmwareVer"
    },
    {
        /* Chipset ID */
        { 0xC7, 0x0003 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_chipsetId_get, NULL,
        "Chipset ID", 
        ctc_oam_varCli_chipID_get
    },
    {
        /* ONU Capabilities-1 */
        { 0xC7, 0x0004 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_onuCapabilities1_get, NULL,
        "ONU Capabilities-1"
    },
    {
        /* OpticalTransceiverDiagnosis */
        { 0xC7, 0x0005 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_opticalTransceiverDiagnosis_get, NULL,
        "OpticalTransceiverDiagnosis"
    },
    {
        /* Sevice SLA */
        { 0xC7, 0x0006 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_serviceSla_get, ctc_oam_varCb_serviceSla_set,
        "Sevice SLA"
    },
    {
        /* ONU Capabilities-2 */
        { 0xC7, 0x0007 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_onuCapabilities2_get, NULL,
        "ONU Capabilities-2"
    },
    {
        /* HoldoverConfig */
        { 0xC7, 0x0008 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_holdoverConfig_get, ctc_oam_varCb_holdoverConfig_set,
        "HoldoverConfig"
    },
    {
        /* MxUMngGlobalParameter */
        { 0xC7, 0x0009 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_mxUMngGlobalParameter_get, ctc_oam_varCb_mxUMngGlobalParameter_set,
        "MxUMngGlobalParameter"
    },
    {
        /* MxUMngSNMPParameter */
        { 0xC7, 0x000A },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_mxUMngSNMPParameter_get, ctc_oam_varCb_mxUMngSNMPParameter_set,
        "MxUMngSNMPParameter"
    },
    {
        /* Active PON_IFAdminstate */
        { 0xC7, 0x000B },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_activePonIfAdminstate_get, ctc_oam_varCb_activePonIfAdminstate_set,
        "Active PON_IFAdminstate"
    },
    {
        /* ONU Capabilities-3 */
        { 0xC7, 0x000C },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_onuCapabilities3_get, NULL,
        "ONU Capabilities-3"
    },
    {
        /* ONU power saving capabilities */
        { 0xC7, 0x000D },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_onuPowerSavingCapabilities_get, NULL,
        "ONU power saving capabilities"
    },
    {
        /* ONU power saving config */
        { 0xC7, 0x000E },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_onuPowerSavingConfig_get, ctc_oam_varCb_onuPowerSavingConfig_set,
        "ONU power saving config"
    },
    {
        /* ONU Protection Parameters */
        { 0xC7, 0x000F },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_onuProtectionParameters_get, ctc_oam_varCb_onuProtectionParameters_set,
        "ONU Protection Parameters"
    },
    {
        /* ethPort State */
        { 0xC7, 0x0011 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_ethLinkState_get, NULL,
        "ethPort State"
    },
    {
        /* ethPort Pause */
        { 0xC7, 0x0012 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_ethPortPause_get, ctc_oam_varCb_ethPortPause_set,
        "ethPort Pause"
    },
    {
        /* ethPortUs Policing */
        { 0xC7, 0x0013 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_ethPortUsPolicing_get, ctc_oam_varCb_ethPortUsPolicing_set,
        "ethPortUs Policing"
    },
    {
        /* voip port */
        { 0xC7, 0x0014 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_voipPort_get, ctc_oam_varCb_voipPort_set,
        "voip port"
    },
    {
        /* EthPortDs RateLimiting */
        { 0xC7, 0x0016 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_ethPortDsRateLimiting_get, ctc_oam_varCb_ethPortDsRateLimiting_set,
        "EthPortDs RateLimiting"
    },
    {
        /* PortLoopDetect */
        { 0xC7, 0x0017 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        NULL, ctc_oam_varCb_portLoopDetect_set,
        "PortLoopDetect"
    },
    {
        /* PortDisableLooped */
        { 0xC7, 0x0018 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        NULL, ctc_oam_varCb_portDisableLooped_set,
        "PortDisableLooped"
    },
    {
        /* PortLoopParameterConfig */
        { 0xC7, 0x0019 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        NULL, ctc_oam_varCb_portLoopParameterConfig_set,
        "PortLoopParameterConfig"
    },
    {
        /* MAC Aging Time */
        { 0xC7, 0x00A4 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_portMacAgingTime_get, ctc_oam_varCb_portMacAgingTime_set,
        "MAC Aging Time"
    },
    {
        /* VLAN */
        { 0xC7, 0x0021 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_vlan_get, ctc_oam_varCb_vlan_set,
        "VLAN",
        ctc_oam_varCli_vlan_get
    },    
    {
        /* Classification&Marking */
        { 0xC7, 0x0031 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT,
        ctc_oam_varCb_classificationMarking_get, ctc_oam_varCb_classificationMarking_set,
        "Classification"
    },
    {
        /* Add/Del Multicast VLAN */
        { 0xC7, 0x0041 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_MULTICAST,
        ctc_oam_varCb_addDelMulticastVlan_get, ctc_oam_varCb_addDelMulticastVlan_set,
        "Add/Del Multicast VLAN"
    },
    {
        /* MulticastTagOper */
        { 0xC7, 0x0042 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_MULTICAST,
        ctc_oam_varCb_multicastTagOper_get, ctc_oam_varCb_multicastTagOper_set,
        "MulticastTagOper"
    },
    {
        /* MulticastSwitch */
        { 0xC7, 0x0043 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_MULTICAST,
        ctc_oam_varCb_multicastSwitch_get, ctc_oam_varCb_multicastSwitch_set,
        "MulticastSwitch",
        ctc_oam_varCli_multicastSwitch_get
    }, 
	{
		/* MulticastControl */
		{ 0xC7, 0x0044},
		CTC_VAR_OP_GET | CTC_VAR_OP_SET,
		CTC_VAR_TARGET_MULTICAST,
		ctc_oam_varCb_multicastControl_get, ctc_oam_varCb_multicastControl_set,
		"MulticastControl"
	},
    {
        /* Group Num Max */
        { 0xC7, 0x0045 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_MULTICAST,
        ctc_oam_varCb_groupNumMax_get, ctc_oam_varCb_groupNumMax_set,
        "Group Num Max"
    },
    {
        /* aFastLeaveAbility */
        { 0xC7, 0x0046 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_MULTICAST,
        ctc_oam_varCb_aFastLeaveAbility_get, NULL,
        "aFastLeaveAbility"
    },
    {
        /* aFastLeaveAdminState */
        { 0xC7, 0x0047 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_MULTICAST,
        ctc_oam_varCb_aFastLeaveAdminState_get, NULL,
        "aFastLeaveAdminState"
    },
    {
        /* POTSStatus */
        { 0xC7, 0x006B },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_UNIPORT | CTC_VAR_TARGET_PONIF,
        ctc_oam_varCb_potsStatus_get, NULL,
        "POTSStatus"
    },
    {
        /* AlarmAdminState */
        { 0xC7, 0x0081 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU | CTC_VAR_TARGET_UNIPORT | CTC_VAR_TARGET_PONIF,
        NULL, ctc_oam_varCb_alarmAdminState_set,
        "AlarmAdminState"
    },
    {
        /* AlarmThreshold */
        { 0xC7, 0x0082 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU | CTC_VAR_TARGET_UNIPORT | CTC_VAR_TARGET_PONIF,
        ctc_oam_varCb_alarmThreshold_get, ctc_oam_varCb_alarmThreshold_set,
        "AlarmThreshold"
    },
    {
        /* ONUTxPowerSupplyControl */
        { 0xC7, 0x00A1 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        NULL, ctc_oam_varCb_txPowerSupplyControl_set,
        "ONUTxPowerSupplyControl"
    },
    {
        /* performanceMonitoringStatus */
        { 0xC7, 0x00B1 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT | CTC_VAR_TARGET_PONIF,
        ctc_oam_varCb_performanceMonitoringStatus_get, ctc_oam_varCb_performanceMonitoringStatus_set,
        "performanceMonitoringStatus"
    },
    {
        /* performanceMonitoringDataCurrent */
        { 0xC7, 0x00B2 },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_UNIPORT | CTC_VAR_TARGET_PONIF,
        ctc_oam_varCb_performanceMonitoringDataCurrent_get, ctc_oam_varCb_performanceMonitoringDataCurrent_set,
        "performanceMonitoringDataCurrent"
    },
    {
        /* performanceMonitoringDataHistory */
        { 0xC7, 0x00B3 },
        CTC_VAR_OP_GET,
        CTC_VAR_TARGET_UNIPORT | CTC_VAR_TARGET_PONIF,
        ctc_oam_varCb_performanceMonitoringDataHistory_get, NULL,
        "performanceMonitoringDataHistory"
    },
	{
		/* configVlanOfTr069Wan Private ctc command*/
    	{ 0xC7, 0x801f },
        CTC_VAR_OP_GET | CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        ctc_oam_varCb_configVlanOfTr069Wan_get, ctc_oam_varCb_configVlanOfTr069Wan_set,
        "configVlanOfTr069Wan"
    },
    {
        /* End indicator - must be the last one */
        { 0x00, 0x0000 },
        0,
        0,        
        NULL, NULL,
        ""
    }
};

static ctc_varCb_t ctc_extActCb[] = {
    /*  The leaf value should be sorted
     *  { varBranch, varLeaf },
     *  allowed_op,
     *  target,
     *  get_callback, set_callback,
     *  varName
     */
    {
        /* Reset ONU */
        { 0xC9, 0x0001 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        NULL, ctc_oam_varCb_resetOnu_set,
        "Reset ONU"
    },
    {
        /* Sleep Control */
        { 0xC9, 0x0002 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        NULL, ctc_oam_varCb_sleepControl_set,
        "Sleep Control"
    },
    {
        /* acFastLeaveAdminControl */
        { 0xC9, 0x0048 },
        CTC_VAR_OP_SET,
        CTC_VAR_TARGET_ONU,
        NULL, ctc_oam_varCb_acFastLeaveAdminControl_set,
        "acFastLeaveAdminControl"
    },
    {
        /* End indicator - must be the last one */
        { 0x00, 0x0000 },
        0,
        0,        
        NULL, NULL,
        ""
    }
};

static ctc_varDescriptor_t ctc_noWidthList[] = {
    /* 
     *  { varBranch, varLeaf },
     */
    /* Reset ONU */
    { 0xC9, 0x0001 },
    /* acAutoNegRestartAutoConfig */
    { 0x09, 0x000B },
    /* End indicator - must be the last one */
    { 0x00, 0x0000 },
};

