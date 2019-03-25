#include <rtk_rg_struct.h>
#include <linux/kmod.h>


static char cmd_buff[CB_CMD_BUFF_SIZE];
static char env_PATH[CB_CMD_BUFF_SIZE];

int _rt_edp_pipe_cmd(const char *comment, ...) 
{
	char * envp[]={	//element size 3
		"HOME=/",
		env_PATH,
		NULL
	};
	char * argv[]={ //element size 4
		"/bin/bash",
		"-c",
		cmd_buff,
		NULL
	};
	int retval;
	va_list argList;
	va_start(argList, comment);
	snprintf( env_PATH, CB_CMD_BUFF_SIZE, "PATH=%s", CONFIG_RG_CALLBACK_ENVIRONMENT_VARIABLE_PATH);
	vsprintf(cmd_buff, comment, argList);
	printk("[CMD][%s]\n",cmd_buff);
	retval=call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);	
	va_end(argList);
	return retval;
}



int rt_edp_api_module_init(void)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_driverVersion_get(rtk_rg_VersionString_t *version_string)
{	
	return 0;
}

rtk_rg_err_code_t rt_edp_initParam_get(rtk_rg_initParams_t *init_param)
{	
	return 0;
}

rtk_rg_err_code_t rt_edp_initParam_set(rtk_rg_initParams_t *init_param)
{
	//down all actived netdev
	_rt_edp_pipe_cmd("	ethdev=$(ifconfig | grep Ethernet | awk '{ print $1 }')\nfor dev in ${ethdev}\n	do\n	  ifconfig $dev down\n	done\n");	


	//remove all netdev from br0
	_rt_edp_pipe_cmd("	ethdev=$(ifconfig -a | grep Ethernet | awk '{ print $1 }')\nfor dev in ${ethdev}\n	do\n	  brctl delif br0 $dev\n	done\n");


	//remove all brX netdev
	_rt_edp_pipe_cmd("	ethdev=$(ifconfig -a | grep Ethernet |  awk '{ print $1 }' | grep br)\nfor dev in ${ethdev}\n	do\n	  brctl delbr $dev\n	done\n");


	//reset iptables/ebtables
	_rt_edp_pipe_cmd("iptables -F");
	_rt_edp_pipe_cmd("iptables -X");
	_rt_edp_pipe_cmd("ebtables -F");
	_rt_edp_pipe_cmd("ebtables -X");
	_rt_edp_pipe_cmd("iptables -P INPUT ACCEPT");
	_rt_edp_pipe_cmd("iptables -P OUTPUT ACCEPT");
	_rt_edp_pipe_cmd("iptables -P FORWARD ACCEPT");
	_rt_edp_pipe_cmd("ebtables -P INPUT ACCEPT");
	_rt_edp_pipe_cmd("ebtables -P OUTPUT ACCEPT");
	_rt_edp_pipe_cmd("ebtables -P FORWARD ACCEPT");

	
	return 0;
}

rtk_rg_err_code_t rt_edp_lanInterface_add(rtk_rg_lanIntfConf_t *lan_info,int *intf_idx)
{
	int i,j;

	// set br0 netdev
	_rt_edp_pipe_cmd("brctl addbr br0");
	_rt_edp_pipe_cmd("ifconfig br0 hw ether %02X:%02X:%02X:%02X:%02X:%02X up %d.%d.%d.%d netmask %d.%d.%d.%d mtu %d add %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x/%d"
		,lan_info->gmac.octet[0],lan_info->gmac.octet[1],lan_info->gmac.octet[2],lan_info->gmac.octet[3],lan_info->gmac.octet[4],lan_info->gmac.octet[5]
		,(u8)(lan_info->ip_addr>>24),(u8)(lan_info->ip_addr>>16),(u8)(lan_info->ip_addr>>8),(u8)(lan_info->ip_addr)
		,(u8)(lan_info->ip_network_mask>>24),(u8)(lan_info->ip_network_mask>>16),(u8)(lan_info->ip_network_mask>>8),(u8)(lan_info->ip_network_mask)
		,lan_info->mtu
		,lan_info->ipv6_addr.ipv6_addr[0],lan_info->ipv6_addr.ipv6_addr[1],lan_info->ipv6_addr.ipv6_addr[2],lan_info->ipv6_addr.ipv6_addr[3]
		,lan_info->ipv6_addr.ipv6_addr[4],lan_info->ipv6_addr.ipv6_addr[5],lan_info->ipv6_addr.ipv6_addr[6],lan_info->ipv6_addr.ipv6_addr[7]
		,lan_info->ipv6_addr.ipv6_addr[8],lan_info->ipv6_addr.ipv6_addr[9],lan_info->ipv6_addr.ipv6_addr[10],lan_info->ipv6_addr.ipv6_addr[11]
		,lan_info->ipv6_addr.ipv6_addr[12],lan_info->ipv6_addr.ipv6_addr[13],lan_info->ipv6_addr.ipv6_addr[14],lan_info->ipv6_addr.ipv6_addr[15]
		,lan_info->ipv6_network_mask_length
		);

	// set br0.XXX netdev
	_rt_edp_pipe_cmd("vconfig add br0 %d",lan_info->intf_vlan_id);
	_rt_edp_pipe_cmd("ifconfig br0.%d up",lan_info->intf_vlan_id);
	_rt_edp_pipe_cmd("ifconfig br0.%d hw ether %02X:%02X:%02X:%02X:%02X:%02X up %d.%d.%d.%d netmask %d.%d.%d.%d mtu %d add %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x/%d",lan_info->intf_vlan_id
		,lan_info->gmac.octet[0],lan_info->gmac.octet[1],lan_info->gmac.octet[2],lan_info->gmac.octet[3],lan_info->gmac.octet[4],lan_info->gmac.octet[5]
		,(u8)(lan_info->ip_addr>>24),(u8)(lan_info->ip_addr>>16),(u8)(lan_info->ip_addr>>8),(u8)(lan_info->ip_addr)
		,(u8)(lan_info->ip_network_mask>>24),(u8)(lan_info->ip_network_mask>>16),(u8)(lan_info->ip_network_mask>>8),(u8)(lan_info->ip_network_mask)
		,lan_info->mtu
		,lan_info->ipv6_addr.ipv6_addr[0],lan_info->ipv6_addr.ipv6_addr[1],lan_info->ipv6_addr.ipv6_addr[2],lan_info->ipv6_addr.ipv6_addr[3]
		,lan_info->ipv6_addr.ipv6_addr[4],lan_info->ipv6_addr.ipv6_addr[5],lan_info->ipv6_addr.ipv6_addr[6],lan_info->ipv6_addr.ipv6_addr[7]
		,lan_info->ipv6_addr.ipv6_addr[8],lan_info->ipv6_addr.ipv6_addr[9],lan_info->ipv6_addr.ipv6_addr[10],lan_info->ipv6_addr.ipv6_addr[11]
		,lan_info->ipv6_addr.ipv6_addr[12],lan_info->ipv6_addr.ipv6_addr[13],lan_info->ipv6_addr.ipv6_addr[14],lan_info->ipv6_addr.ipv6_addr[15]
		,lan_info->ipv6_network_mask_length
		);

	// add lan port to br0
	// FIXME: must add wifi netdev 
	for(i=1,j=2;i<=8;i<<=1,j++)
	{	
		if(lan_info->port_mask.portmask&i) 
		{
			_rt_edp_pipe_cmd("brctl addif br0 eth0.%d",j); 
			_rt_edp_pipe_cmd("ifconfig eth0.%d up",j);	
		}
	}

		
	return 0;
}

rtk_rg_err_code_t rt_edp_wanInterface_add(rtk_rg_wanIntfConf_t *wanintf, int *wan_intf_idx)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_staticInfo_set(int wan_intf_idx, rtk_rg_ipStaticInfo_t *static_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_dsliteInfo_set(int wan_intf_idx, rtk_rg_ipDslitStaticInfo_t *dslite_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_dhcpRequest_set(int wan_intf_idx)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_dhcpClientInfo_set(int wan_intf_idx, rtk_rg_ipDhcpClientInfo_t *dhcpClient_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_pppoeClientInfoBeforeDial_set(int wan_intf_idx, rtk_rg_pppoeClientInfoBeforeDial_t *app_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_pppoeClientInfoAfterDial_set(int wan_intf_idx, rtk_rg_pppoeClientInfoAfterDial_t *clientPppoe_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_pptpClientInfoBeforeDial_set(int wan_intf_idx, rtk_rg_pptpClientInfoBeforeDial_t *app_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_pptpClientInfoAfterDial_set(int wan_intf_idx, rtk_rg_pptpClientInfoAfterDial_t *clientPptp_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_l2tpClientInfoBeforeDial_set(int wan_intf_idx, rtk_rg_l2tpClientInfoBeforeDial_t *app_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_l2tpClientInfoAfterDial_set(int wan_intf_idx, rtk_rg_l2tpClientInfoAfterDial_t *clientL2tp_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_pppoeDsliteInfoBeforeDial_set(int wan_intf_idx, rtk_rg_pppoeClientInfoBeforeDial_t *app_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_pppoeDsliteInfoAfterDial_set(int wan_intf_idx, rtk_rg_pppoeDsliteInfoAfterDial_t *pppoeDslite_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_interface_del(int lan_or_wan_intf_idx)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_intfInfo_find(rtk_rg_intfInfo_t *intf_info, int *valid_lan_or_wan_intf_idx)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_cvlan_add(rtk_rg_cvlan_info_t *cvlan_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_cvlan_del(int cvlan_id)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_cvlan_get(rtk_rg_cvlan_info_t *cvlan_info)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_vlanBinding_add(rtk_rg_vlanBinding_t *vlan_binding_info, int *vlan_binding_idx)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_vlanBinding_del(int vlan_binding_idx)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_vlanBinding_find(rtk_rg_vlanBinding_t *vlan_binding_info, int *valid_idx)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_algServerInLanAppsIpAddr_add(rtk_rg_alg_serverIpMapping_t *srvIpMapping)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_algServerInLanAppsIpAddr_del(rtk_rg_alg_type_t delServerMapping)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_algApps_set(rtk_rg_alg_type_t alg_app)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_algApps_get(rtk_rg_alg_type_t *alg_app)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_dmzHost_set(int wan_intf_idx, rtk_rg_dmzInfo_t *dmz_info)
{

	return 0;
}

rtk_rg_err_code_t rt_edp_dmzHost_get(int wan_intf_idx, rtk_rg_dmzInfo_t *dmz_info)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_virtualServer_add(rtk_rg_virtualServer_t *virtual_server, int *virtual_server_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_virtualServer_del(int virtual_server_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_virtualServer_find(rtk_rg_virtualServer_t *virtual_server, int *valid_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_aclFilterAndQos_add(rtk_rg_aclFilterAndQos_t *acl_filter, int *acl_filter_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_aclFilterAndQos_del(int acl_filter_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_naptFilterAndQos_add(int *index,rtk_rg_naptFilterAndQos_t *napt_filter){
	
	
	return 0;
}
rtk_rg_err_code_t rt_edp_naptFilterAndQos_del(int index){
	
	
	
	return 0;
}
rtk_rg_err_code_t rt_edp_naptFilterAndQos_find(int *index,rtk_rg_naptFilterAndQos_t *napt_filter){
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_aclFilterAndQos_find(rtk_rg_aclFilterAndQos_t *acl_filter, int *valid_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_macFilter_add(rtk_rg_macFilterEntry_t *macFilterEntry,int *mac_filter_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_macFilter_del(int mac_filter_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_macFilter_find(rtk_rg_macFilterEntry_t *macFilterEntry, int *valid_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_urlFilterString_add(rtk_rg_urlFilterString_t *filter,int *url_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_urlFilterString_del(int url_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_urlFilterString_find(rtk_rg_urlFilterString_t *filter, int *valid_idx)
{
	
	
	
	return 0;
}



rtk_rg_err_code_t rt_edp_upnpConnection_add(rtk_rg_upnpConnection_t *upnp, int *upnp_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_upnpConnection_del(int upnp_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_upnpConnection_find(rtk_rg_upnpConnection_t *upnp, int *valid_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_naptConnection_add(rtk_rg_naptEntry_t *naptFlow, int *flow_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_naptConnection_del(int flow_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_naptConnection_find(rtk_rg_naptInfo_t *naptInfo,int *valid_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_multicastFlow_add(rtk_rg_multicastFlow_t *mcFlow, int *flow_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_multicastFlow_del(int flow_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_l2MultiCastFlow_add(rtk_rg_l2MulticastFlow_t *l2McFlow,int *flow_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_multicastFlow_find(rtk_rg_multicastFlow_t *mcFlow, int *valid_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_macEntry_add(rtk_rg_macEntry_t *macEntry, int *entry_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_macEntry_del(int entry_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_macEntry_find(rtk_rg_macEntry_t *macEntry,int *valid_idx)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_arpEntry_add(rtk_rg_arpEntry_t *arpEntry, int *arp_entry_idx)
{
	return 0;
}

rtk_rg_err_code_t rt_edp_arpEntry_del(int arp_entry_idx)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_arpEntry_find(rtk_rg_arpInfo_t *arpInfo,int *arp_valid_idx)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_neighborEntry_add(rtk_rg_neighborEntry_t *neighborEntry,int *neighbor_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_neighborEntry_del(int neighbor_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_neighborEntry_find(rtk_rg_neighborInfo_t *neighborInfo,int *neighbor_valid_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_softwareIdleTime_set(rtk_rg_idle_time_type_t idleTimeType, int idleTime)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_softwareIdleTime_get(rtk_rg_idle_time_type_t idleTimeType, int *pIdleTime)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_accessWanLimit_set(rtk_rg_accessWanLimitData_t access_wan_info)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_accessWanLimit_get(rtk_rg_accessWanLimitData_t *access_wan_info)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_accessWanLimitCategory_set(rtk_rg_accessWanLimitCategory_t macCategory_info)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_accessWanLimitCategory_get(rtk_rg_accessWanLimitCategory_t *macCategory_info)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_softwareSourceAddrLearningLimit_set(rtk_rg_saLearningLimitInfo_t sa_learnLimit_info, rtk_rg_port_idx_t port_idx)
{

	
	return 0;
}

rtk_rg_err_code_t rt_edp_softwareSourceAddrLearningLimit_get(rtk_rg_saLearningLimitInfo_t *sa_learnLimit_info, rtk_rg_port_idx_t port_idx)
{
	
	

	
	return 0;
}

rtk_rg_err_code_t rt_edp_wlanSoftwareSourceAddrLearningLimit_set(rtk_rg_saLearningLimitInfo_t sa_learnLimit_info, int wlan_idx, int dev_idx)
{
		
	return 0;
}

rtk_rg_err_code_t rt_edp_wlanSoftwareSourceAddrLearningLimit_get(rtk_rg_saLearningLimitInfo_t *sa_learnLimit_info, int wlan_idx, int dev_idx)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_dosPortMaskEnable_set(rtk_rg_mac_portmask_t dos_port_mask)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_dosPortMaskEnable_get(rtk_rg_mac_portmask_t *dos_port_mask)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_dosType_set(rtk_rg_dos_type_t dos_type,int dos_enabled,rtk_rg_dos_action_t dos_action)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_dosType_get(rtk_rg_dos_type_t dos_type,int *dos_enabled,rtk_rg_dos_action_t *dos_action)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_dosFloodType_set(rtk_rg_dos_type_t dos_type,int dos_enabled,rtk_rg_dos_action_t dos_action,int dos_threshold)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_dosFloodType_get(rtk_rg_dos_type_t dos_type,int *dos_enabled,rtk_rg_dos_action_t *dos_action,int *dos_threshold)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portMirror_set(rtk_rg_portMirrorInfo_t portMirrorInfo)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portMirror_get(rtk_rg_portMirrorInfo_t *portMirrorInfo)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portMirror_clear(void)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portEgrBandwidthCtrlRate_set(rtk_rg_mac_port_idx_t port, uint32 rate)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portIgrBandwidthCtrlRate_set(rtk_rg_mac_port_idx_t port, uint32 rate)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portEgrBandwidthCtrlRate_get(rtk_rg_mac_port_idx_t port, uint32 *rate)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portIgrBandwidthCtrlRate_get(rtk_rg_mac_port_idx_t port, uint32 *rate)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_phyPortForceAbility_set(rtk_rg_mac_port_idx_t port, rtk_rg_phyPortAbilityInfo_t ability)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_phyPortForceAbility_get(rtk_rg_mac_port_idx_t port, rtk_rg_phyPortAbilityInfo_t *ability)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_cpuPortForceTrafficCtrl_set(rtk_rg_enable_t tx_fc_state,	rtk_rg_enable_t rx_fc_state)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_cpuPortForceTrafficCtrl_get(rtk_rg_enable_t *pTx_fc_state,	rtk_rg_enable_t *pRx_fc_state)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portMibInfo_get(rtk_rg_mac_port_idx_t port, rtk_rg_port_mib_info_t *mibInfo)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portMibInfo_clear(rtk_rg_mac_port_idx_t port)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portIsolation_set(rtk_rg_port_isolation_t isolationSetting)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portIsolation_get(rtk_rg_port_isolation_t *isolationSetting)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_stormControl_add(rtk_rg_stormControlInfo_t *stormInfo,int *stormInfo_idx)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_stormControl_del(int stormInfo_idx)
{
	
	

	return 0;
}

rtk_rg_err_code_t rt_edp_stormControl_find(rtk_rg_stormControlInfo_t *stormInfo,int *stormInfo_idx)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_shareMeter_set(uint32 index, uint32 rate, rtk_rg_enable_t ifgInclude)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_shareMeter_get(uint32 index, uint32 *pRate , rtk_rg_enable_t *pIfgInclude)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_shareMeterMode_set(uint32 index, rtk_rate_metet_mode_t meterMode)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_shareMeterMode_get(uint32 index, rtk_rate_metet_mode_t *pMeterMode)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosStrictPriorityOrWeightFairQueue_set(rtk_rg_mac_port_idx_t port_idx,rtk_rg_qos_queue_weights_t q_weight)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosStrictPriorityOrWeightFairQueue_get(rtk_rg_mac_port_idx_t port_idx,rtk_rg_qos_queue_weights_t *pQ_weight)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosInternalPriMapToQueueId_set(int int_pri, int queue_id)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosInternalPriMapToQueueId_get(int int_pri, int *pQueue_id)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosInternalPriDecisionByWeight_set(rtk_rg_qos_priSelWeight_t weightOfPriSel)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosInternalPriDecisionByWeight_get(rtk_rg_qos_priSelWeight_t *pWeightOfPriSel)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDscpRemapToInternalPri_set(uint32 dscp,uint32 int_pri)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDscpRemapToInternalPri_get(uint32 dscp,uint32 *pInt_pri)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosPortBasedPriority_set(rtk_rg_mac_port_idx_t port_idx,uint32 int_pri)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosPortBasedPriority_get(rtk_rg_mac_port_idx_t port_idx,uint32 *pInt_pri)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDot1pPriRemapToInternalPri_set(uint32 dot1p,uint32 int_pri)
{
	

	return 0;
}

rtk_rg_err_code_t rt_edp_qosDot1pPriRemapToInternalPri_get(uint32 dot1p,uint32 *pInt_pri)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDscpRemarkEgressPortEnableAndSrcSelect_set(rtk_rg_mac_port_idx_t rmk_port,rtk_rg_enable_t rmk_enable, rtk_rg_qos_dscpRmkSrc_t rmk_src_select)
{

	return 0;
}

rtk_rg_err_code_t rt_edp_qosDscpRemarkEgressPortEnableAndSrcSelect_get(rtk_rg_mac_port_idx_t rmk_port,rtk_rg_enable_t *pRmk_enable, rtk_rg_qos_dscpRmkSrc_t *pRmk_src_select)
{
	

	return 0;
}

rtk_rg_err_code_t rt_edp_qosDscpRemarkByInternalPri_set(int int_pri,int rmk_dscp)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDscpRemarkByInternalPri_get(int int_pri,int *pRmk_dscp)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDscpRemarkByDscp_set(int dscp,int rmk_dscp)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDscpRemarkByDscp_get(int dscp,int *pRmk_dscp)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDot1pPriRemarkByInternalPriEgressPortEnable_set(rtk_rg_mac_port_idx_t rmk_port, rtk_rg_enable_t rmk_enable)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDot1pPriRemarkByInternalPriEgressPortEnable_get(rtk_rg_mac_port_idx_t rmk_port, rtk_rg_enable_t *pRmk_enable)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDot1pPriRemarkByInternalPri_set(int int_pri,int rmk_dot1p)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_qosDot1pPriRemarkByInternalPri_get(int int_pri,int *pRmk_dot1p)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portBasedCVlanId_set(rtk_rg_port_idx_t port_idx,int pvid)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portBasedCVlanId_get(rtk_rg_port_idx_t port_idx,int *pPvid)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_wlanDevBasedCVlanId_set(int wlan_idx,int dev_idx,int dvid)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_wlanDevBasedCVlanId_get(int wlan_idx,int dev_idx,int *pDvid)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_portStatus_get(rtk_rg_mac_port_idx_t port, rtk_rg_portStatusInfo_t *portInfo)
{
	
	return 0;
}

#ifdef CONFIG_RG_NAPT_PORT_COLLISION_PREVENTION
rtk_rg_err_code_t rt_edp_naptExtPortGet(int isTcp,uint16 *pPort)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_naptExtPortFree(int isTcp,uint16 port)
{
	
	return 0;
}
#endif

rtk_rg_err_code_t rt_edp_classifyEntry_add(rtk_rg_classifyEntry_t *classifyFilter)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_classifyEntry_find(int index, rtk_rg_classifyEntry_t *classifyFilter)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_classifyEntry_del(int index)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_svlanTpid_set(uint32 svlan_tag_id)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_svlanTpid_get(uint32 *pSvlanTagId)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_svlanServicePort_set(rtk_port_t port, rtk_enable_t enable)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_svlanServicePort_get(rtk_port_t port, rtk_enable_t *pEnable)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_svlanTpid2_enable_set(rtk_rg_enable_t enable)
{
	
	
	return 0;
}
rtk_rg_err_code_t rt_edp_svlanTpid2_enable_get(rtk_rg_enable_t *pEnable)
{
	
	return 0;
}
rtk_rg_err_code_t rt_edp_svlanTpid2_set(uint32 svlan_tag_id)
{
	
	
	return 0;
}
rtk_rg_err_code_t rt_edp_svlanTpid2_get(uint32 *pSvlanTagId)
{
	
	
	return 0;
}



rtk_rg_err_code_t rt_edp_pppoeInterfaceIdleTime_get(int intfIdx,uint32 *idleSec)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_gatewayServicePortRegister_add(rtk_rg_gatewayServicePortEntry_t *serviceEntry, int *index){
	
	
	return 0;

}

rtk_rg_err_code_t rt_edp_gatewayServicePortRegister_del(int index){
	
	
	return 0;

}

rtk_rg_err_code_t rt_edp_gatewayServicePortRegister_find(rtk_rg_gatewayServicePortEntry_t *serviceEntry, int *index){
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_stpBlockingPortmask_set(rtk_rg_portmask_t Mask){
	
	
	return 0;
}
rtk_rg_err_code_t rt_edp_stpBlockingPortmask_get(rtk_rg_portmask_t *pMask){
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_gponDsBcFilterAndRemarking_Enable(rtk_rg_enable_t enable)
{
	
	return 0;
}


rtk_rg_err_code_t rt_edp_gponDsBcFilterAndRemarking_add(rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t *filterRule,int *index)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_gponDsBcFilterAndRemarking_del(int index)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_gponDsBcFilterAndRemarking_del_all(void)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_gponDsBcFilterAndRemarking_find(int *index,rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t *filterRule){
	
	return 0;
}


rtk_rg_err_code_t rt_edp_interfaceMibCounter_del(int intf_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_interfaceMibCounter_get(rtk_rg_netifMib_entry_t *pNetifMib)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpAll_set(rtk_rg_redirectHttpAll_t *pRedirectHttpAll)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpAll_get(rtk_rg_redirectHttpAll_t *pRedirectHttpAll)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpURL_add(rtk_rg_redirectHttpURL_t *pRedirectHttpURL)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpURL_del(rtk_rg_redirectHttpURL_t *pRedirectHttpURL)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpWhiteList_add(rtk_rg_redirectHttpWhiteList_t *pRedirectHttpWhiteList)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpWhiteList_del(rtk_rg_redirectHttpWhiteList_t *pRedirectHttpWhiteList)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpRsp_set(rtk_rg_redirectHttpRsp_t *pRedirectHttpRsp)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpRsp_get(rtk_rg_redirectHttpRsp_t *pRedirectHttpRsp)
{
	
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpCount_set(rtk_rg_redirectHttpCount_t *pRedirectHttpCount)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_redirectHttpCount_get(rtk_rg_redirectHttpCount_t *pRedirectHttpCount)
{
	
	return 0;
}


rtk_rg_err_code_t rt_edp_hostPoliceControl_set(rtk_rg_hostPoliceControl_t *pHostPoliceControl, int host_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_hostPoliceControl_get(rtk_rg_hostPoliceControl_t *pHostPoliceControl, int host_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_hostPoliceLogging_get(rtk_rg_hostPoliceLogging_t *pHostMibCnt, int host_idx)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_hostPoliceLogging_del(int host_idx)
{

	
	return 0;
}

rtk_rg_err_code_t rt_edp_staticRoute_add(rtk_rg_staticRoute_t *pStaticRoute, int *index)
{
	
	return 0;

}

rtk_rg_err_code_t rt_edp_staticRoute_del(int index)
{	

	return 0;

}

rtk_rg_err_code_t rt_edp_staticRoute_find(rtk_rg_staticRoute_t *pStaticRoute, int *index)
{
	
	return 0;
}

rtk_rg_err_code_t rt_edp_aclLogCounterControl_get(int index, int *type,  int *mode)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_aclLogCounterControl_set(int index,  int type,  int mode)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_aclLogCounter_get(int index, uint64 *count)
{
	
	
	return 0;
}

rtk_rg_err_code_t rt_edp_aclLogCounter_reset(int index)
{
	return 0;

}

rtk_rg_err_code_t rt_edp_groupMacLimit_set(rtk_rg_groupMacLimit_t group_mac_info)
{
	return 0;

}

rtk_rg_err_code_t rt_edp_groupMacLimit_get(rtk_rg_groupMacLimit_t *pGroup_mac_info)
{
	return 0;

}

rtk_rg_err_code_t rt_edp_igmpMldSnoopingControl_set(rtk_rg_igmpMldSnoopingControl_t *config )
{
	return 0;

}

rtk_rg_err_code_t rt_edp_igmpMldSnoopingControl_get(rtk_rg_igmpMldSnoopingControl_t *config )
{
	return 0;

}

rtk_rg_err_code_t rt_edp_igmpMldSnoopingPortControl_add(rtk_rg_port_idx_t port_idx,rtk_rg_igmpMldSnoopingPortControl_t *config )
{
	return 0;

}
rtk_rg_err_code_t rt_edp_igmpMldSnoopingPortControl_del(rtk_rg_port_idx_t port_idx)
{
	return 0;

}
rtk_rg_err_code_t rt_edp_igmpMldSnoopingPortControl_find(rtk_rg_port_idx_t port_idx,rtk_rg_igmpMldSnoopingPortControl_t *config )
{
	return 0;

}

rtk_rg_err_code_t rt_edp_flowMibCounter_get(int index, rtk_rg_table_flowmib_t *pCounter)
{
	return 0;

}

rtk_rg_err_code_t rt_edp_flowMibCounter_reset(int index)
{
	return 0;

}

rtk_rg_err_code_t rt_edp_funcbasedMeter_set(rtk_rg_funcbasedMeterConf_t meterConf)
{
	return 0;

}
 
rtk_rg_err_code_t rt_edp_funcbasedMeter_get(rtk_rg_funcbasedMeterConf_t *meterConf)
{
	return 0;

}

rtk_rg_err_code_t rt_edp_flowHiPriEntry_add(rtk_rg_table_highPriPatten_t hiPriEntry,int *entry_idx)
{
	return 0;

}
 
rtk_rg_err_code_t rt_edp_flowHiPriEntry_del(int entry_idx)
{
	return 0;

}


rtk_rg_err_code_t rt_edp_callback_function_ptr_get(rtk_rg_callbackFunctionPtrGet_t *callback_function_ptr_get_info)
{

	return 0;
}

rtk_rg_err_code_t rt_edp_mac_filter_whitelist_add(rtk_rg_macFilterWhiteList_t *mac_filter_whitelist_info)
{

	return 0;
}

rtk_rg_err_code_t rt_edp_mac_filter_whitelist_del(rtk_rg_macFilterWhiteList_t *mac_filter_whitelist_info)
{

	return 0;
}




struct platform pf=
{
	.rtk_rg_api_module_init=rt_edp_api_module_init,
	.rtk_rg_driverVersion_get = rt_edp_driverVersion_get,
	.rtk_rg_initParam_get	=rt_edp_initParam_get,
	.rtk_rg_initParam_set	=rt_edp_initParam_set,
	.rtk_rg_lanInterface_add	=rt_edp_lanInterface_add,
//5
	.rtk_rg_wanInterface_add	=rt_edp_wanInterface_add,
	.rtk_rg_staticInfo_set	=rt_edp_staticInfo_set,
	.rtk_rg_dhcpRequest_set	=rt_edp_dhcpRequest_set,
	.rtk_rg_dhcpClientInfo_set	=rt_edp_dhcpClientInfo_set,
	.rtk_rg_pppoeClientInfoBeforeDial_set	=rt_edp_pppoeClientInfoBeforeDial_set,
//10
	.rtk_rg_pppoeClientInfoAfterDial_set	=rt_edp_pppoeClientInfoAfterDial_set,
	.rtk_rg_interface_del	=rt_edp_interface_del,
	.rtk_rg_intfInfo_find	=rt_edp_intfInfo_find,
	.rtk_rg_cvlan_add	=rt_edp_cvlan_add,
	.rtk_rg_cvlan_del	=rt_edp_cvlan_del,
//15
	.rtk_rg_cvlan_get=rt_edp_cvlan_get,
	.rtk_rg_vlanBinding_add	=rt_edp_vlanBinding_add,
	.rtk_rg_vlanBinding_del	=rt_edp_vlanBinding_del,
	.rtk_rg_vlanBinding_find	=rt_edp_vlanBinding_find,
	.rtk_rg_algServerInLanAppsIpAddr_add	=rt_edp_algServerInLanAppsIpAddr_add,
//20
	.rtk_rg_algServerInLanAppsIpAddr_del	=rt_edp_algServerInLanAppsIpAddr_del,
	.rtk_rg_algApps_set	=rt_edp_algApps_set,
	.rtk_rg_algApps_get	=rt_edp_algApps_get,
	.rtk_rg_dmzHost_set	=rt_edp_dmzHost_set,
	.rtk_rg_dmzHost_get	=rt_edp_dmzHost_get,
//25
	.rtk_rg_virtualServer_add	=rt_edp_virtualServer_add,
	.rtk_rg_virtualServer_del	=rt_edp_virtualServer_del,
	.rtk_rg_virtualServer_find	=rt_edp_virtualServer_find,
	.rtk_rg_aclFilterAndQos_add	=rt_edp_aclFilterAndQos_add,
	.rtk_rg_aclFilterAndQos_del	=rt_edp_aclFilterAndQos_del,
//30
	.rtk_rg_aclFilterAndQos_find	=rt_edp_aclFilterAndQos_find,
	.rtk_rg_macFilter_add	=rt_edp_macFilter_add,
	.rtk_rg_macFilter_del	=rt_edp_macFilter_del,
	.rtk_rg_macFilter_find	=rt_edp_macFilter_find,
	.rtk_rg_urlFilterString_add	=rt_edp_urlFilterString_add,
//35
	.rtk_rg_urlFilterString_del	=rt_edp_urlFilterString_del,
	.rtk_rg_urlFilterString_find	=rt_edp_urlFilterString_find,
	.rtk_rg_upnpConnection_add	=rt_edp_upnpConnection_add,
	.rtk_rg_upnpConnection_del	=rt_edp_upnpConnection_del,
	.rtk_rg_upnpConnection_find	=rt_edp_upnpConnection_find,
	/* martin zhu add */
	.rtk_rg_l2MultiCastFlow_add	=rt_edp_l2MultiCastFlow_add,
//40
	.rtk_rg_naptConnection_add	=rt_edp_naptConnection_add,
	.rtk_rg_naptConnection_del	=rt_edp_naptConnection_del,
	.rtk_rg_naptConnection_find	=rt_edp_naptConnection_find,
	.rtk_rg_multicastFlow_add	=rt_edp_multicastFlow_add,
	.rtk_rg_multicastFlow_del	=rt_edp_multicastFlow_del,
//45
	.rtk_rg_multicastFlow_find	=rt_edp_multicastFlow_find,
	.rtk_rg_macEntry_add	=rt_edp_macEntry_add,
	.rtk_rg_macEntry_del	=rt_edp_macEntry_del,
	.rtk_rg_macEntry_find	=rt_edp_macEntry_find,
	.rtk_rg_arpEntry_add	=rt_edp_arpEntry_add,
//50
	.rtk_rg_arpEntry_del	=rt_edp_arpEntry_del,
	.rtk_rg_arpEntry_find	=rt_edp_arpEntry_find,
	.rtk_rg_neighborEntry_add	=rt_edp_neighborEntry_add,
	.rtk_rg_neighborEntry_del	=rt_edp_neighborEntry_del,
	.rtk_rg_neighborEntry_find	=rt_edp_neighborEntry_find,
//55
	.rtk_rg_accessWanLimit_set	=rt_edp_accessWanLimit_set,
	.rtk_rg_accessWanLimit_get	=rt_edp_accessWanLimit_get,
	.rtk_rg_accessWanLimitCategory_set	=rt_edp_accessWanLimitCategory_set,
	.rtk_rg_accessWanLimitCategory_get	=rt_edp_accessWanLimitCategory_get,
	.rtk_rg_softwareSourceAddrLearningLimit_set	=rt_edp_softwareSourceAddrLearningLimit_set,
//60
	.rtk_rg_softwareSourceAddrLearningLimit_get	=rt_edp_softwareSourceAddrLearningLimit_get,
	.rtk_rg_dosPortMaskEnable_set	=rt_edp_dosPortMaskEnable_set,
	.rtk_rg_dosPortMaskEnable_get	=rt_edp_dosPortMaskEnable_get,
	.rtk_rg_dosType_set	=rt_edp_dosType_set,
	.rtk_rg_dosType_get	=rt_edp_dosType_get,
//65
	.rtk_rg_dosFloodType_set	=rt_edp_dosFloodType_set,
	.rtk_rg_dosFloodType_get	=rt_edp_dosFloodType_get,
	.rtk_rg_portMirror_set	=rt_edp_portMirror_set,
	.rtk_rg_portMirror_get	=rt_edp_portMirror_get,
	.rtk_rg_portMirror_clear	=rt_edp_portMirror_clear,
//70
	.rtk_rg_portEgrBandwidthCtrlRate_set	=rt_edp_portEgrBandwidthCtrlRate_set,
	.rtk_rg_portIgrBandwidthCtrlRate_set	=rt_edp_portIgrBandwidthCtrlRate_set,
	.rtk_rg_portEgrBandwidthCtrlRate_get	=rt_edp_portEgrBandwidthCtrlRate_get,
	.rtk_rg_portIgrBandwidthCtrlRate_get	=rt_edp_portIgrBandwidthCtrlRate_get,
	.rtk_rg_phyPortForceAbility_set	=rt_edp_phyPortForceAbility_set,
//75
	.rtk_rg_phyPortForceAbility_get	=rt_edp_phyPortForceAbility_get,
	.rtk_rg_cpuPortForceTrafficCtrl_set	=rt_edp_cpuPortForceTrafficCtrl_set,
	.rtk_rg_cpuPortForceTrafficCtrl_get	=rt_edp_cpuPortForceTrafficCtrl_get,
	.rtk_rg_portMibInfo_get	=rt_edp_portMibInfo_get,
	.rtk_rg_portMibInfo_clear	=rt_edp_portMibInfo_clear,
//80
	.rtk_rg_stormControl_add	=rt_edp_stormControl_add,
	.rtk_rg_stormControl_del	=rt_edp_stormControl_del,
	.rtk_rg_stormControl_find	=rt_edp_stormControl_find,
	.rtk_rg_shareMeter_set	=rt_edp_shareMeter_set,
	.rtk_rg_shareMeter_get	=rt_edp_shareMeter_get,
//85
	.rtk_rg_shareMeterMode_set =rt_edp_shareMeterMode_set,
	.rtk_rg_shareMeterMode_get =rt_edp_shareMeterMode_get,
	.rtk_rg_qosStrictPriorityOrWeightFairQueue_set	=rt_edp_qosStrictPriorityOrWeightFairQueue_set,
	.rtk_rg_qosStrictPriorityOrWeightFairQueue_get	=rt_edp_qosStrictPriorityOrWeightFairQueue_get,
	.rtk_rg_qosInternalPriMapToQueueId_set	=rt_edp_qosInternalPriMapToQueueId_set,
//90
	.rtk_rg_qosInternalPriMapToQueueId_get	=rt_edp_qosInternalPriMapToQueueId_get,
	.rtk_rg_qosInternalPriDecisionByWeight_set	=rt_edp_qosInternalPriDecisionByWeight_set,
	.rtk_rg_qosInternalPriDecisionByWeight_get	=rt_edp_qosInternalPriDecisionByWeight_get,
	.rtk_rg_qosDscpRemapToInternalPri_set	=rt_edp_qosDscpRemapToInternalPri_set,
	.rtk_rg_qosDscpRemapToInternalPri_get	=rt_edp_qosDscpRemapToInternalPri_get,
//95	
	.rtk_rg_qosPortBasedPriority_set	=rt_edp_qosPortBasedPriority_set,	
	.rtk_rg_qosPortBasedPriority_get	=rt_edp_qosPortBasedPriority_get,
	.rtk_rg_qosDot1pPriRemapToInternalPri_set	=rt_edp_qosDot1pPriRemapToInternalPri_set,
	.rtk_rg_qosDot1pPriRemapToInternalPri_get	=rt_edp_qosDot1pPriRemapToInternalPri_get,
	.rtk_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_set	=rt_edp_qosDscpRemarkEgressPortEnableAndSrcSelect_set,
//100	
	.rtk_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_get	=rt_edp_qosDscpRemarkEgressPortEnableAndSrcSelect_get,		
	.rtk_rg_qosDscpRemarkByInternalPri_set	=rt_edp_qosDscpRemarkByInternalPri_set,
	.rtk_rg_qosDscpRemarkByInternalPri_get	=rt_edp_qosDscpRemarkByInternalPri_get,
	.rtk_rg_qosDscpRemarkByDscp_set	=rt_edp_qosDscpRemarkByDscp_set,
	.rtk_rg_qosDscpRemarkByDscp_get	=rt_edp_qosDscpRemarkByDscp_get,
//105	
	.rtk_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_set	=rt_edp_qosDot1pPriRemarkByInternalPriEgressPortEnable_set,
	.rtk_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_get	=rt_edp_qosDot1pPriRemarkByInternalPriEgressPortEnable_get,
	.rtk_rg_qosDot1pPriRemarkByInternalPri_set	=rt_edp_qosDot1pPriRemarkByInternalPri_set,
	.rtk_rg_qosDot1pPriRemarkByInternalPri_get	=rt_edp_qosDot1pPriRemarkByInternalPri_get,
	.rtk_rg_portBasedCVlanId_set	=rt_edp_portBasedCVlanId_set,
//110	
	.rtk_rg_portBasedCVlanId_get	=rt_edp_portBasedCVlanId_get,	
	.rtk_rg_portStatus_get	=rt_edp_portStatus_get,
#ifdef CONFIG_RG_NAPT_PORT_COLLISION_PREVENTION
	.rtk_rg_naptExtPortGet	=rt_edp_naptExtPortGet,
	.rtk_rg_naptExtPortFree	=rt_edp_naptExtPortFree,
#endif
	.rtk_rg_classifyEntry_add	=rt_edp_classifyEntry_add,
//115	
	.rtk_rg_classifyEntry_find	=rt_edp_classifyEntry_find,	
	.rtk_rg_classifyEntry_del	=rt_edp_classifyEntry_del,
	.rtk_rg_svlanTpid_get=rt_edp_svlanTpid_get,
	.rtk_rg_svlanTpid_set=rt_edp_svlanTpid_set,
	.rtk_rg_svlanServicePort_set=rt_edp_svlanServicePort_set,
//120	
	.rtk_rg_svlanServicePort_get=rt_edp_svlanServicePort_get,	
	.rtk_rg_pppoeInterfaceIdleTime_get=rt_edp_pppoeInterfaceIdleTime_get,
	.rtk_rg_gatewayServicePortRegister_add=rt_edp_gatewayServicePortRegister_add,
	.rtk_rg_gatewayServicePortRegister_del=rt_edp_gatewayServicePortRegister_del,
	.rtk_rg_gatewayServicePortRegister_find=rt_edp_gatewayServicePortRegister_find,
//125	
	.rtk_rg_wlanDevBasedCVlanId_set=rt_edp_wlanDevBasedCVlanId_set,	
	.rtk_rg_wlanDevBasedCVlanId_get=rt_edp_wlanDevBasedCVlanId_get,
	.rtk_rg_wlanSoftwareSourceAddrLearningLimit_set=rt_edp_wlanSoftwareSourceAddrLearningLimit_set,
	.rtk_rg_wlanSoftwareSourceAddrLearningLimit_get=rt_edp_wlanSoftwareSourceAddrLearningLimit_get,
	.rtk_rg_naptFilterAndQos_add=rt_edp_naptFilterAndQos_add,
//130	
	.rtk_rg_naptFilterAndQos_del=rt_edp_naptFilterAndQos_del,	
	.rtk_rg_naptFilterAndQos_find=rt_edp_naptFilterAndQos_find,
	.rtk_rg_pptpClientInfoBeforeDial_set=rt_edp_pptpClientInfoBeforeDial_set,
	.rtk_rg_pptpClientInfoAfterDial_set=rt_edp_pptpClientInfoAfterDial_set,
	.rtk_rg_l2tpClientInfoBeforeDial_set=rt_edp_l2tpClientInfoBeforeDial_set,
//135	
	.rtk_rg_l2tpClientInfoAfterDial_set=rt_edp_l2tpClientInfoAfterDial_set,	
	.rtk_rg_stpBlockingPortmask_set=rt_edp_stpBlockingPortmask_set,
	.rtk_rg_stpBlockingPortmask_get=rt_edp_stpBlockingPortmask_get,
	.rtk_rg_portIsolation_set=rt_edp_portIsolation_set,
	.rtk_rg_portIsolation_get=rt_edp_portIsolation_get,
//140
	.rtk_rg_dsliteInfo_set=rt_edp_dsliteInfo_set,	
	.rtk_rg_pppoeDsliteInfoBeforeDial_set=rt_edp_pppoeDsliteInfoBeforeDial_set,
	.rtk_rg_pppoeDsliteInfoAfterDial_set=rt_edp_pppoeDsliteInfoAfterDial_set,
	.rtk_rg_gponDsBcFilterAndRemarking_add=rt_edp_gponDsBcFilterAndRemarking_add,
	.rtk_rg_gponDsBcFilterAndRemarking_del=rt_edp_gponDsBcFilterAndRemarking_del,
//145	
	.rtk_rg_gponDsBcFilterAndRemarking_find=rt_edp_gponDsBcFilterAndRemarking_find,	
	.rtk_rg_gponDsBcFilterAndRemarking_del_all=rt_edp_gponDsBcFilterAndRemarking_del_all,
	.rtk_rg_gponDsBcFilterAndRemarking_Enable=rt_edp_gponDsBcFilterAndRemarking_Enable,
	.rtk_rg_interfaceMibCounter_del=rt_edp_interfaceMibCounter_del,
	.rtk_rg_interfaceMibCounter_get=rt_edp_interfaceMibCounter_get,
//150	
	.rtk_rg_redirectHttpAll_set=rt_edp_redirectHttpAll_set,	
	.rtk_rg_redirectHttpAll_get=rt_edp_redirectHttpAll_get,
	.rtk_rg_redirectHttpURL_add=rt_edp_redirectHttpURL_add,
	.rtk_rg_redirectHttpURL_del=rt_edp_redirectHttpURL_del,
	.rtk_rg_redirectHttpWhiteList_add=rt_edp_redirectHttpWhiteList_add,
//155	
	.rtk_rg_redirectHttpWhiteList_del=rt_edp_redirectHttpWhiteList_del,
	.rtk_rg_redirectHttpRsp_set=rt_edp_redirectHttpRsp_set,
	.rtk_rg_redirectHttpRsp_get=rt_edp_redirectHttpRsp_get,
	.rtk_rg_svlanTpid2_get= rt_edp_svlanTpid2_get,
	.rtk_rg_svlanTpid2_set= rt_edp_svlanTpid2_set,
//160	
	.rtk_rg_svlanTpid2_enable_get=rt_edp_svlanTpid2_enable_get,	
	.rtk_rg_svlanTpid2_enable_set=rt_edp_svlanTpid2_enable_set,
	.rtk_rg_hostPoliceControl_set=rt_edp_hostPoliceControl_set,
	.rtk_rg_hostPoliceControl_get=rt_edp_hostPoliceControl_get,
	.rtk_rg_hostPoliceLogging_get=rt_edp_hostPoliceLogging_get,
//165	
	.rtk_rg_hostPoliceLogging_del=rt_edp_hostPoliceLogging_del,	
	.rtk_rg_redirectHttpCount_set=rt_edp_redirectHttpCount_set,
	.rtk_rg_redirectHttpCount_get=rt_edp_redirectHttpCount_get,
	.rtk_rg_staticRoute_add=rt_edp_staticRoute_add,
	.rtk_rg_staticRoute_del=rt_edp_staticRoute_del,
//170	
	.rtk_rg_staticRoute_find=rt_edp_staticRoute_find,
	.rtk_rg_aclLogCounterControl_get=rt_edp_aclLogCounterControl_get,
	.rtk_rg_aclLogCounterControl_set=rt_edp_aclLogCounterControl_set,
	.rtk_rg_aclLogCounter_get=rt_edp_aclLogCounter_get,
	.rtk_rg_aclLogCounter_reset=rt_edp_aclLogCounter_reset,
//175	
	.rtk_rg_groupMacLimit_get=rt_edp_groupMacLimit_get,	
	.rtk_rg_groupMacLimit_set=rt_edp_groupMacLimit_set,
	.rtk_rg_igmpMldSnoopingControl_set=rt_edp_igmpMldSnoopingControl_set,
	.rtk_rg_igmpMldSnoopingControl_get=rt_edp_igmpMldSnoopingControl_get,
	.rtk_rg_flowMibCounter_get=rt_edp_flowMibCounter_get,
//180	
	.rtk_rg_flowMibCounter_reset=rt_edp_flowMibCounter_reset,	
	.rtk_rg_softwareIdleTime_set=rt_edp_softwareIdleTime_set,
	.rtk_rg_softwareIdleTime_get=rt_edp_softwareIdleTime_get,
	.rtk_rg_funcbasedMeter_set=rt_edp_funcbasedMeter_set,
	.rtk_rg_funcbasedMeter_get=rt_edp_funcbasedMeter_get,
//185	
	.rtk_rg_flowHiPriEntry_add=rt_edp_flowHiPriEntry_add,
	.rtk_rg_flowHiPriEntry_del=rt_edp_flowHiPriEntry_del,
	.rtk_rg_igmpMldSnoopingPortControl_add=rt_edp_igmpMldSnoopingPortControl_add,
	.rtk_rg_igmpMldSnoopingPortControl_del=rt_edp_igmpMldSnoopingPortControl_del,	
	.rtk_rg_igmpMldSnoopingPortControl_find=rt_edp_igmpMldSnoopingPortControl_find,
//190
};



