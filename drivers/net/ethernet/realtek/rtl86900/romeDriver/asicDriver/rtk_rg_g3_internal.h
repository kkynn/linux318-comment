#ifndef __RTK_RG_G3_INTERNAL_H__
#define __RTK_RG_G3_INTERNAL_H__

#if defined(CONFIG_HWNAT_RG)||defined(CONFIG_HWNAT_RG_MODULE)	//built-in or module
#include <rtk_rg_internal.h>
#elif defined(CONFIG_HWNAT_FLEETCONNTRACK)
#include <rtk_rg_fc_internal.h>
#include <rtk_rg_apolloPro_asicDriver.h>
#endif

#include <aal_l3_stg0.h>
#include <aal_l3_te.h>
#include <classifier.h>		//for ca_classifier_rule_add
#include <route.h>		//for ca_generic_intf_add
#include <aal_hash.h>
#include <flow.h>


typedef enum{
	RG_CA_FLOW_UC5TUPLE_DS		= 0, 	//HASH_PROFILE_0, 	// WAN (wan to lan)
	RG_CA_FLOW_UC5TUPLE_US		= 1, 	//HASH_PROFILE_1, 	// LAN (lan to wan or lan to lan)
	RG_CA_FLOW_MC 				= 2, 	//HASH_PROFILE_2, 	// init in CA aal_hash_init()
	RG_CA_FLOW_UC2TUPLE_BRIDGE	= 3, 	//HASH_PROFILE_3, 	// without L4 bridge only

	RG_CA_FLOW_MAX = CA_FLOW_TYPE_MAX,
}rg_ca_flow_profile_keytype_t;

typedef enum{
	RG_CA_FLOW_TUPPLE_PRI_0,		// DEFAULT, the lowest priority
	RG_CA_FLOW_TUPPLE_PRI_1,
	RG_CA_FLOW_TUPPLE_PRI_2,
	RG_CA_FLOW_TUPPLE_PRI_3,
	RG_CA_FLOW_TUPPLE_PRI_4,
	RG_CA_FLOW_TUPPLE_PRI_5,
	RG_CA_FLOW_TUPPLE_PRI_6,
	RG_CA_FLOW_TUPPLE_PRI_MAX,
}rg_ca_flow_tuple_priority_t;


typedef struct rtk_rg_flow_key_mask_e{
	uint8 P12_spa;
	uint8 P12_vlanId;
	uint8 P12_vlanPri;
	uint8 P345_vlanId;
	uint8 P345_vlanPri;
	uint8 P345_dscp;
	uint8 P345_ecn;
} rtk_rg_flow_key_mask_t;

// flowid value should mapping to the sequence of rtk_rg_port_idx_t(extport index)
typedef enum{
	RTK_RG_WIFI_FLOWID_CPU_RSVD=0,
#if defined(CONFIG_WLAN_MBSSID_NUM) && (CONFIG_WLAN_MBSSID_NUM==7)
	RTK_RG_WIFI0_FLOWID_VAP4,
	RTK_RG_WIFI0_FLOWID_VAP5,
	RTK_RG_WIFI0_FLOWID_VAP6,
#endif	
	RTK_RG_WIFI0_FLOWID_OTHER,				// for 11ac other

	RTK_RG_WIFI1_FLOWID_ROOT,				// for 11n root
	RTK_RG_WIFI1_FLOWID_VAP0,
	RTK_RG_WIFI1_FLOWID_VAP1,
	RTK_RG_WIFI1_FLOWID_VAP2,
	RTK_RG_WIFI1_FLOWID_VAP3,
#if defined(CONFIG_WLAN_MBSSID_NUM) && (CONFIG_WLAN_MBSSID_NUM==7)
	RTK_RG_WIFI1_FLOWID_VAP4,
	RTK_RG_WIFI1_FLOWID_VAP5,
	RTK_RG_WIFI1_FLOWID_VAP6,
#endif
	RTK_RG_WIFI1_FLOWID_OTHER,				// for 11n other
	
	RTK_RG_WIFI_FLOWID_MAX,	
}rtk_rg_wifi_flowid_t;


#define rg_ca_skb_in(skb, trace) 	nic_ca_skb_in(skb, __FILE__, __LINE__, trace)
#define rg_ca_skb_out(skb) 		nic_ca_skb_out(skb, __FILE__, __LINE__)


//EXTERN FUNCTION
int aal_l3fe_keep_lspid_unchange_set(ca_device_id_t device_id, ca_uint8_t *config);
netdev_tx_t nic_egress_start_xmit(struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config);
netdev_tx_t ca_ni_start_xmit_native(struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config);
void nic_ca_skb_in(struct sk_buff *skb, const char *fname, int line, bool trace);
void nic_ca_skb_out(struct sk_buff *skb, const char *fname, int line);
int ca_flow_age_set(ca_device_id_t device_id, ca_uint32_t hash_idx, ca_uint32_t age);
int ca_flow_age_get(ca_device_id_t device_id, ca_uint32_t hash_idx, ca_uint32_t *age);
int ca_flow_traffic_status_get(ca_device_id_t device_id, ca_uint32_t hash_idx, ca_uint32_t *trfStatus);

int nic_rxhook_default(struct napi_struct *napi,struct net_device *dev, struct sk_buff *skb, nic_hook_private_t *nh_priv);
#ifdef CONFIG_RTK_NIC_TX_HOOK
int nic_register_txhook(p2tfunc_t tx);
int nic_txhook_init(void);
int nic_txhook_exit(void);
#endif

#ifdef CONFIG_LUNA_G3_SERIES
uint32_t rtk_ne_reg_read(uint32_t addr);
void rtk_ne_reg_write(uint32_t data, uint32_t addr);
struct net_device * rtk_ni_virtual_cpuport_alloc(int cpuPort, char *name);
uint32_t rtk_ni_virtual_cpuport_free(int cpuPort);
uint32_t rtk_ni_virtual_cpuport_open(int cpuPort);
uint32_t rtk_ni_virtual_cpuport_close(int cpuPort);
void flow_table_dump(void);
#endif

//Prototype
uint32_t rtk_rg_g3_cpuport_init(void) ;
uint32_t rtk_rg_g3_flow_init(rtk_rg_flow_key_mask_t flowKeyMask);
uint32_t rtk_rg_g3_init(void);
uint32_t rtk_rg_g3_exit(void);

//ACL Prototype
int _rtk_rg_aclAndCfReservedRuleAdd(rtk_rg_aclAndCf_reserved_type_t rsvType, void *parameter);
int _rtk_rg_aclAndCfReservedRuleAddSpecial(rtk_rg_aclAndCf_reserved_type_t rsvType, void *parameter);
int _rtk_rg_aclAndCfReservedRuleDelSpecial(rtk_rg_aclAndCf_reserved_type_t rsvType, int index, void *parameter);

#endif //__RTK_RG_G3_INTERNAL_H__

