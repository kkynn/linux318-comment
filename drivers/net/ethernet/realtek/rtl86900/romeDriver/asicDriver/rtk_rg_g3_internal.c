#include <rtk_rg_g3_internal.h>
#include <flow.h>
#include <mcast.h>
#include <vlan.h>
#include <l2.h>
#include <nat.h>
#include <aal_fdb.h>
#include <aal_port.h>

#if defined(CONFIG_ARCH_CORTINA_G3LITE) && defined(CONFIG_RTK_DEV_AP)
#include <queue.h>
#include <port.h>
#endif


char printtmp[256];
#define G3INITPRINT( comment ,arg...) \
do {\
            int mt_trace_i;\
            sprintf( printtmp, comment,## arg);\
            for(mt_trace_i=1;mt_trace_i<1024;mt_trace_i++) \
            { \
                    if(printtmp[mt_trace_i]==0) \
                    { \
                            if(printtmp[mt_trace_i-1]=='\n') printtmp[mt_trace_i-1]=' '; \
                            else break; \
                    } \
            } \
            printk("[HWNAT] %s @%s(%d)\n", printtmp, __FUNCTION__, __LINE__);\
} while(0)




uint32_t rtk_rg_g3_cpuport_init(void) 
{
	ca_status_t ret = CA_E_OK;
	int i;

	i = 0;
	
#if defined(CONFIG_CA_NE_L2FP)	// if defined, L2FP module allocate netdev for those cpuports. ref: ca_l2fp_init

	// enable CPU port 0x12~0x17 for wifi acceleration
	for(i=AAL_LPORT_CPU_2; i <= AAL_LPORT_CPU_7; i++) {

		if(!(ret = rtk_ni_virtual_cpuport_open(i)))
			G3INITPRINT("Enable CPU port 0x%x", i);	
		else
			G3INITPRINT("ERROR - Enable CPU port 0x%x FAIL, ret = 0x%x !!!!!", i ,ret);
	}
	
#else
	#if 0
	uint8 dev_name[64];
	for(i=AAL_LPORT_CPU_2; i <= AAL_LPORT_CPU_7; i++) {
		snprintf(dev_name, 64, "%s%02X", "wifiFF_cpu#0x", i);
		if(rtk_ni_virtual_cpuport_alloc(i, dev_name) != NULL)
			G3INITPRINT("Create CPU port 0x%x dev %s", i, dev_name);
		else
			G3INITPRINT("ERROR - Create CPU port 0x%x FAIL !!!!!", i);
	}	
	// enable CPU port 0x12~0x17 for wifi acceleration
	for(i=AAL_LPORT_CPU_2; i <= AAL_LPORT_CPU_7; i++) {

		if(!(ret = rtk_ni_virtual_cpuport_open(i)))
			G3INITPRINT("Enable CPU port 0x%x", i);	
		else
			G3INITPRINT("ERROR - Enable CPU port 0x%x FAIL, ret = 0x%x !!!!!", i ,ret);
	}
	
	#else

	// please_enable_CONFIG_CA_NE_L2FP
	
	#endif
#endif


	return ret;
}


uint32_t rtk_rg_g3_cpuport_exit(void) 
{
	int i = 0;

	G3INITPRINT("unregister cpu port...");
		
	for(i=AAL_LPORT_CPU_2; i <= AAL_LPORT_CPU_7; i++) {
		rtk_ni_virtual_cpuport_close(i);	
		G3INITPRINT("close cpu port...0x%x", i);
	}
	
#if !defined(CONFIG_CA_NE_L2FP)	// if defined, L2FP module allocate netdev for those cpuports. ref: ca_l2fp_init
	#if 0
	for(i=AAL_LPORT_CPU_2; i <= AAL_LPORT_CPU_7; i++) {
		rtk_ni_virtual_cpuport_free(i);
		G3INITPRINT("free cpu port...%d", i);
	}
	#else
	
	// please_enable_CONFIG_CA_NE_L2FP
	
	#endif
#endif	
	return CA_E_OK;
}

uint32_t rtk_rg_g3_flow_init(rtk_rg_flow_key_mask_t flowKeyMask)
{
	ca_flow_key_type_config_t flow_key;
	ca_flow_key_type_t keyType;


	ca_flow_key_profile_mapping_set(G3_DEF_DEVID, RG_CA_FLOW_UC5TUPLE_DS,			HASH_PROFILE_0);
	ca_flow_key_profile_mapping_set(G3_DEF_DEVID, RG_CA_FLOW_UC5TUPLE_US,			HASH_PROFILE_1);
	ca_flow_key_profile_mapping_set(G3_DEF_DEVID, RG_CA_FLOW_MC, 					HASH_PROFILE_2);
	ca_flow_key_profile_mapping_set(G3_DEF_DEVID, RG_CA_FLOW_UC2TUPLE_BRIDGE,		HASH_PROFILE_3);

	ca_flow_delete_all(G3_DEF_DEVID);
	for(keyType = CA_FLOW_TYPE_0; keyType<CA_FLOW_TYPE_MAX; keyType++){
		ca_flow_key_type_delete(G3_DEF_DEVID, keyType);
	}
	
	/****************************************************** 
	* profile 0: path345 pattern mask for ds
	******************************************************/
	memset(&flow_key, 0, sizeof(flow_key));
	flow_key.key_type = RG_CA_FLOW_UC5TUPLE_DS;		//FB_PATH_34 & 5
	flow_key.prio = RG_CA_FLOW_TUPPLE_PRI_0;
	flow_key.key_mask.l2_keys = TRUE;
	flow_key.key_mask.l3_keys = TRUE;
	flow_key.key_mask.l4_keys = TRUE;
	flow_key.key_mask.l2_mask.o_lspid = TRUE;												// force enabled to support unknown sa trap by main hash
//	flow_key.key_mask.l2_mask.outer_vlan.tpid = TRUE;
	flow_key.key_mask.l2_mask.outer_vlan.vid = (flowKeyMask.P345_vlanId) ? TRUE : FALSE;
	flow_key.key_mask.l2_mask.outer_vlan.pri = (flowKeyMask.P345_vlanPri) ? TRUE : FALSE;
//	flow_key.key_mask.l2_mask.inner_vlan.tpid = TRUE;
	flow_key.key_mask.l2_mask.inner_vlan.vid = (flowKeyMask.P345_vlanId) ? TRUE : FALSE;
	flow_key.key_mask.l2_mask.inner_vlan.pri = (flowKeyMask.P345_vlanPri) ? TRUE : FALSE;
	flow_key.key_mask.l3_mask.ip_valid = TRUE;
	flow_key.key_mask.l3_mask.ip_version = TRUE;
	flow_key.key_mask.l3_mask.ip_protocol = TRUE;
	flow_key.key_mask.l3_mask.dscp = (flowKeyMask.P345_dscp) ? TRUE : FALSE;
	flow_key.key_mask.l3_mask.ecn = (flowKeyMask.P345_ecn) ? TRUE : FALSE;
	flow_key.key_mask.l3_mask.ip_sa = TRUE;
	flow_key.key_mask.l3_mask.ip_da = TRUE;
	
	flow_key.key_mask.l3_mask.fragment = TRUE;	
	flow_key.key_mask.l3_mask.hbh_header = TRUE;
	flow_key.key_mask.l3_mask.routing_header = TRUE;
	flow_key.key_mask.l3_mask.dest_opt_header = TRUE;

	flow_key.key_mask.l4_mask.src_l4_port = TRUE;
	flow_key.key_mask.l4_mask.dest_l4_port = TRUE;
	
	flow_key.key_mask.l4_mask.tcp_flags= 0x7;	// rst, syn, fin

	ASSERT_EQ(ca_flow_key_type_add(G3_DEF_DEVID, &flow_key), CA_E_OK);
	
	/****************************************************** 
	* profile 1: path345 pattern mask for  up
	******************************************************/
	memset(&flow_key, 0, sizeof(flow_key));
	flow_key.key_type = RG_CA_FLOW_UC5TUPLE_US;		//FB_PATH_34 & 5
	flow_key.prio = RG_CA_FLOW_TUPPLE_PRI_0;
	flow_key.key_mask.l2_keys = TRUE;
	flow_key.key_mask.l3_keys = TRUE;
	flow_key.key_mask.l4_keys = TRUE;
	flow_key.key_mask.l2_mask.o_lspid = TRUE;												// force enabled to support unknown sa trap by main hash
//	flow_key.key_mask.l2_mask.outer_vlan.tpid = TRUE;
	flow_key.key_mask.l2_mask.outer_vlan.vid = (flowKeyMask.P345_vlanId) ? TRUE : FALSE;
	flow_key.key_mask.l2_mask.outer_vlan.pri = (flowKeyMask.P345_vlanPri) ? TRUE : FALSE;
//	flow_key.key_mask.l2_mask.inner_vlan.tpid = TRUE;
	flow_key.key_mask.l2_mask.inner_vlan.vid = (flowKeyMask.P345_vlanId) ? TRUE : FALSE;
	flow_key.key_mask.l2_mask.inner_vlan.pri = (flowKeyMask.P345_vlanPri) ? TRUE : FALSE;
	flow_key.key_mask.l3_mask.ip_valid = TRUE;
	flow_key.key_mask.l3_mask.ip_version = TRUE;
	flow_key.key_mask.l3_mask.ip_protocol = TRUE;
	flow_key.key_mask.l3_mask.dscp = (flowKeyMask.P345_dscp) ? TRUE : FALSE;
	flow_key.key_mask.l3_mask.ecn = (flowKeyMask.P345_ecn) ? TRUE : FALSE;
	flow_key.key_mask.l3_mask.ip_sa = TRUE;
	flow_key.key_mask.l3_mask.ip_da = TRUE;
	
	flow_key.key_mask.l3_mask.fragment = TRUE;	
	flow_key.key_mask.l3_mask.hbh_header = TRUE;
	flow_key.key_mask.l3_mask.routing_header = TRUE;
	flow_key.key_mask.l3_mask.dest_opt_header = TRUE;

	flow_key.key_mask.l4_mask.src_l4_port = TRUE;
	flow_key.key_mask.l4_mask.dest_l4_port = TRUE;
	
	flow_key.key_mask.l4_mask.tcp_flags= 0x7;	// rst, syn, fin

	ASSERT_EQ(ca_flow_key_type_add(G3_DEF_DEVID, &flow_key), CA_E_OK);
	
	/****************************************************** 
	* profile 3: path12 pattern mask
	******************************************************/
	memset(&flow_key, 0, sizeof(flow_key));
	flow_key.key_type = RG_CA_FLOW_UC2TUPLE_BRIDGE;	//FB_PATH_12
	flow_key.prio = RG_CA_FLOW_TUPPLE_PRI_0;
	flow_key.key_mask.l2_keys = TRUE;
	flow_key.key_mask.l4_keys = TRUE;
	
	flow_key.key_mask.l2_mask.o_lspid = /*(flowKeyMask.P12_spa) ? TRUE : FALSE*/ TRUE;		// force enabled to support unknown sa trap by main hash
	flow_key.key_mask.l2_mask.mac_sa = TRUE;
	flow_key.key_mask.l2_mask.mac_da = TRUE;
//	flow_key.key_mask.l2_mask.outer_vlan.tpid = TRUE;
	flow_key.key_mask.l2_mask.outer_vlan.vid = (flowKeyMask.P12_vlanId) ? TRUE : FALSE;
	flow_key.key_mask.l2_mask.outer_vlan.pri = (flowKeyMask.P12_vlanPri) ? TRUE : FALSE;
//	flow_key.key_mask.l2_mask.inner_vlan.tpid = TRUE;
	flow_key.key_mask.l2_mask.inner_vlan.vid = (flowKeyMask.P12_vlanId) ? TRUE : FALSE;
	flow_key.key_mask.l2_mask.inner_vlan.pri = (flowKeyMask.P12_vlanPri) ? TRUE : FALSE;
	flow_key.key_mask.l4_mask.src_l4_port = TRUE;
	flow_key.key_mask.l4_mask.dest_l4_port = TRUE;
	
	ASSERT_EQ(ca_flow_key_type_add(G3_DEF_DEVID, &flow_key), CA_E_OK);
	

	/****************************************************** 
	* profile 2: MC
	******************************************************/
	/* Initialized by CA driver. ref: aal_hash_init() -> aal_hash_profile_set() */



#if 0 // one profile one tuple, no TTL checking	anymore
	/****************************************************** 
	* TTL pattern mask
	******************************************************/
	memset(&flow_key, 0, sizeof(flow_key));
	flow_key.key_type = RG_CA_FLOW_TYPE_TTL;	//TTL
	flow_key.prio = RG_CA_FLOW_TYPE_TTL;
	flow_key.key_mask.l3_keys = TRUE;
	flow_key.key_mask.l3_mask.ip_ttl = 0xFE;

	ASSERT_EQ(ca_flow_key_type_add(G3_DEF_DEVID, &flow_key), CA_E_OK);


	/****************************************************** 
	* reserved Flow entry
	******************************************************/
	{
		ca_status_t ca_ret = CA_E_OK;
		ca_flow_t flow_config;

		memset(&flow_config, 0, sizeof(flow_config));
		flow_config.key_type = RG_CA_FLOW_TYPE_TTL;
		flow_config.key.l3_key.ip_ttl = 0;
		flow_config.actions.forward = CA_CLASSIFIER_FORWARD_PORT;
		flow_config.actions.dest.port = AAL_LPORT_CPU_0;

		ca_ret = ca_flow_add(0, &flow_config);

		if(ca_ret == CA_E_OK){
			printk(">>>>> Add flow[%d]\n", flow_config.index);
		}else{
			printk("\033[1;33;41m[WARNING] Add flow type[%d] fail, ca_ret=0x%x \033[0m\n", flow_config.key_type, ca_ret);
		}
		

	}
#endif	
	return CA_E_OK;
}

uint32_t rtk_rg_g3_init(void)
{
	int i;
	ca_uint8_t keep=1;
	aal_ni_hv_glb_internal_port_id_cfg_t portid_cfg;
	aal_ni_hv_glb_internal_port_id_cfg_mask_t portid_mask={0};
	aal_l3fe_lpb_tbl_cfg_t lpb_cfg;
	aal_l3fe_lpb_tbl_cfg_mask_t lpb_mask={0};
	aal_ilpb_cfg_msk_t ilpb_cfg_msk={0};
	aal_ilpb_cfg_t  ilpb_cfg;
	//ca_gen_intf_attrib_t gen_intf_attrib;

#if 0 //defined(CONFIG_ARCH_CORTINA_G3LITE) && defined(CONFIG_RTK_DEV_AP)
	ca_uint32_t port, queue;
	ca_port_id_t port_id;
	ca_queue_wred_profile_t profile;
	ca_status_t ret;
#endif	
	
	//keep lspid unchange
	ASSERT_EQ(aal_l3fe_keep_lspid_unchange_set(0, &keep),CA_E_OK);
	G3INITPRINT("%s: #####Set keep_lspid_unchange to %u#####\n", __func__,keep);

	//let packet from port 7 could enter l2fe
	ASSERT_EQ(aal_ni_hv_glb_internal_port_id_cfg_get(0, &portid_cfg),CA_E_OK);
	portid_mask.bf.wan_rxsel=1;
	portid_cfg.wan_rxsel=2;
	ASSERT_EQ(aal_ni_hv_glb_internal_port_id_cfg_set(0, portid_mask, &portid_cfg),CA_E_OK);
	G3INITPRINT("%s: #####Set wan_rxsel to %u#####\n", __func__,portid_cfg.wan_rxsel);

	//let packet from port 7 act as other LAN ports
	ilpb_cfg_msk.u32[0] = 0;
	ilpb_cfg_msk.u32[1] = 0;
	ilpb_cfg_msk.s.wan_ind = 1;
	ilpb_cfg.wan_ind = 0;
#if defined (CONFIG_RG_G3_SERIES) && defined(CONFIG_RG_G3_WAN_PORT_INDEX)
	ASSERT_EQ(aal_port_ilpb_cfg_set(0, CONFIG_RG_G3_WAN_PORT_INDEX, ilpb_cfg_msk, &ilpb_cfg),CA_E_OK);
#else
	ASSERT_EQ(aal_port_ilpb_cfg_set(0, AAL_LPORT_ETH_NI7, ilpb_cfg_msk, &ilpb_cfg),CA_E_OK);//FC driver
#endif
	G3INITPRINT("%s: #####Set wan_ind to %u#####\n", __func__,ilpb_cfg.wan_ind);

	//Disable MAC check in L3FE
	lpb_mask.s.mac_da_match_en=1;
	for(i=0;i<4;i++)
	{
		ASSERT_EQ(aal_l3_stg0_lpb_tbl_get(0, i, &lpb_cfg),CA_E_OK);
		lpb_cfg.mac_da_match_en=0;
		ASSERT_EQ(aal_l3_stg0_lpb_tbl_set(0, i, lpb_mask, &lpb_cfg),CA_E_OK);
		G3INITPRINT("%s: #####Set stg0tbl[%d]mac_da_match_en to %u#####\n", __func__,i,lpb_cfg.mac_da_match_en);
	}

	/******************************************************
	 * L2/L3 Classifier
	 ******************************************************/
	//reset all CA cls
	ASSERT_EQ(ca_classifier_rule_delete_all(G3_DEF_DEVID), CA_E_OK);

	//re-configure L2 CLS per port start/length
	memset(&ilpb_cfg_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
	memset(&ilpb_cfg, 0, sizeof(aal_ilpb_cfg_t));
	ilpb_cfg_msk.s.igr_cls_lookup_en = TRUE;
	ilpb_cfg_msk.s.cls_start = TRUE;
	ilpb_cfg_msk.s.cls_length = TRUE;
	ilpb_cfg.igr_cls_lookup_en = 1;

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
	/*each hw entry = eth_niX * 8 + cup_X * 8. ex: hw entry 2=eth_ni2*8+cpu_2*8, it means eth port 3 use hw entry start/leng 30/8, and cpu port 0x13 use 38/8
		PORT IDX	|	HW IDX(start)	|	LENGTH
		0~7(x)			x*16				8
		0x10~0x17(y)	(y-0x10)*16+8		8	*/
	for(i = AAL_LPORT_ETH_NI0; i <= AAL_LPORT_ETH_NI7; i++){
		ilpb_cfg.cls_start = AAL_PORT_START_FOR_CLS + (i-AAL_LPORT_ETH_NI0)*(AAL_ILPB_CLS_LENGTH+AAL_ILPB_CLS_LENGTH_CPU);
		ilpb_cfg.cls_length = AAL_ILPB_CLS_LENGTH;
		ASSERT_EQ(aal_port_ilpb_cfg_set(G3_DEF_DEVID, i, ilpb_cfg_msk, &ilpb_cfg), CA_E_OK);
	}
	for(i = AAL_LPORT_CPU_0; i <= AAL_LPORT_CPU_7; i++){
		ilpb_cfg.cls_start = AAL_PORT_START_FOR_CLS + (i-AAL_LPORT_CPU_0)*(AAL_ILPB_CLS_LENGTH+AAL_ILPB_CLS_LENGTH_CPU) + AAL_ILPB_CLS_LENGTH;
		ilpb_cfg.cls_length = AAL_ILPB_CLS_LENGTH_CPU;
		ASSERT_EQ(aal_port_ilpb_cfg_set(G3_DEF_DEVID, i, ilpb_cfg_msk, &ilpb_cfg), CA_E_OK);
	}
#else
	for(i = AAL_LPORT_CPU_2; i <= AAL_LPORT_CPU_6; i++){		// wifi hwlookup from port 0x17 should bypass l2FE to keep pol_id value
		ilpb_cfg.cls_start = AAL_PORT_START_FOR_CLS_CPU + (i-AAL_LPORT_CPU_0)*AAL_ILPB_CLS_LENGTH_CPU;
		ilpb_cfg.cls_length = AAL_ILPB_CLS_LENGTH_CPU;
		ASSERT_EQ(aal_port_ilpb_cfg_set(G3_DEF_DEVID, i, ilpb_cfg_msk, &ilpb_cfg), CA_E_OK);
	}
#endif

	//Create L2 igr Classifier with priority 0 for forward all packet to L3FE
	ASSERT_EQ(_rtk_rg_aclAndCfReservedRuleAddSpecial(RTK_CA_CLS_TYPE_L2_INGRESS_FORWARD_L3FE, NULL), RT_ERR_OK);

	//Create L2 igr Classifier with priority 7 for pass those multicast packet come back from L3 to L2 with fake vlan
	ASSERT_EQ(_rtk_rg_aclAndCfReservedRuleAddSpecial(RTK_CA_CLS_TYPE_L2_INGRESS_MULTICAST_FORWARD_L2FE, NULL), RT_ERR_OK);

	//Create L3 Classifier to trap ipv6 multicast dslite
	ASSERT_EQ(_rtk_rg_aclAndCfReservedRuleAdd(RTK_RG_ACLANDCF_RESERVED_MULTICAST_DSLITE_TRAP, NULL), RT_ERR_OK);
	
	/**********
	 * Main Hash
	 **********/
	{
		ca_nat_config_t natConfig;
		
		// call flow tuple init by RG/FC driver
	/*
		rtk_rg_flow_key_mask_t flowKeyMask;
		flowKeyMask.P12_spa = TRUE;
		flowKeyMask.P12_vlanId = TRUE;
		flowKeyMask.P12_vlanPri = TRUE;
		flowKeyMask.P345_vlanId = TRUE;
		flowKeyMask.P345_vlanPri = TRUE;
		flowKeyMask.P345_dscp = TRUE;
		flowKeyMask.P345_ecn = TRUE;
		ASSERT_EQ(rtk_rg_g3_flow_init(flowKeyMask), CA_E_OK);
	*/
		// force disable aging countdown
		ASSERT_EQ(ca_nat_config_get(G3_DEF_DEVID, &natConfig), CA_E_OK);
		natConfig.aging_time = 0;
		ASSERT_EQ(ca_nat_config_set(G3_DEF_DEVID, &natConfig), CA_E_OK);
	}

	/**********
	 * VLAN
	 **********/
	{
		ca_uint32_t outerTPID[2] = {0x8100, 0x88a8};
		ca_uint32_t innerTPID[2] = {0x8100, 0x88a8};
		ASSERT_EQ(ca_l2_vlan_tpid_set(G3_DEF_DEVID, innerTPID, 2, outerTPID, 2), CA_E_OK);
		//ca_l2_vlan_outer_tpid_add(0, AAL_LPORT_ETH_NI7, CA_IN ca_uint32_t tpid_sel);
	}

	
	/**********
	 * L2
	 **********/
	ASSERT_EQ(ca_l2_aging_mode_set(0, CA_L2_AGING_SOFTWARE), CA_E_OK);
	{
		// set L2 hash mode
		aal_fdb_ctrl_t	ctrl;
		aal_fdb_ctrl_mask_t cmsk;
		aal_fdb_ctrl_get(0,&ctrl);
		ctrl.hashing_mode=0;
		cmsk.wrd = ~0U;
		aal_fdb_ctrl_set(0,cmsk,&ctrl);
	}


	/********************
	*  l3fe_pon_mode which will take the pol_id from hdr-a to l3fe hdr-i
	********************/
	{
		L3FE_GLB_GLB_CFG_t l3fe_glb;
		l3fe_glb.wrd = rtk_ne_reg_read(L3FE_GLB_GLB_CFG);
		l3fe_glb.bf.l3fe_pon_mode = TRUE;
		rtk_ne_reg_write(l3fe_glb.wrd, L3FE_GLB_GLB_CFG);
	}

	/********************
	*  clear mcgroup
	********************/
	{
		ca_l2_mcast_group_delete_all(G3_DEF_DEVID);
		ca_l3_mcast_group_delete_all(G3_DEF_DEVID);
	}
	
#if 0 //defined(CONFIG_ARCH_CORTINA_G3LITE) && defined(CONFIG_RTK_DEV_AP)
	memset(&profile, 0, sizeof(ca_queue_wred_profile_t));
	profile.unmarked_dp[0] = 100;
	profile.marked_dp[0] = 100;

	for (port = CA_PORT_ID_NI0; port <= CA_PORT_ID_NI4; port++) 
	{
		port_id = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port);
		for (queue = 0; queue < 8; queue++) 
		{
			ret = ca_queue_wred_set(0, port_id, queue, &profile);

			if (ret != CA_E_OK)
				printk("%s error set port %x\n", __func__, port_id);
		}
	}
	for (port = CA_PORT_ID_CPU0; port <= CA_PORT_ID_CPU7; port++)
	{
		port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, port);
		for (queue = 0; queue < 8; queue++) 
		{
			ret = ca_queue_wred_set(0, port_id, queue, &profile);

			if (ret != CA_E_OK)
				printk("%s error set port %x\n", __func__, port_id);
		}
	}
#endif
	
	return CA_E_OK;
}

uint32_t rtk_rg_g3_exit(void)
{
	rtk_rg_g3_cpuport_exit();
	
	return CA_E_OK;
}

