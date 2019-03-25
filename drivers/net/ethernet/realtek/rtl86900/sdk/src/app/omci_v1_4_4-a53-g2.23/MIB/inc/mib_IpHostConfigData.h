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
 * Purpose : Definition of ME attribute: IP host config data (134)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME attribute: IP host config data (134)
 */

#ifndef __MIB_IP_HOST_CFG_DATA_TABLE_H__
#define __MIB_IP_HOST_CFG_DATA_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif


#define MIB_TABLE_IP_HOST_CFG_DATA_ATTR_NUM (17)
#define MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX ((MIB_ATTR_INDEX)1)
#define MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX ((MIB_ATTR_INDEX)2)
#define MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX ((MIB_ATTR_INDEX)3)
#define MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX ((MIB_ATTR_INDEX)4)
#define MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX ((MIB_ATTR_INDEX)5)
#define MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX ((MIB_ATTR_INDEX)6)
#define MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX ((MIB_ATTR_INDEX)7)
#define MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX ((MIB_ATTR_INDEX)8)
#define MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX ((MIB_ATTR_INDEX)9)
#define MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX ((MIB_ATTR_INDEX)10)
#define MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX ((MIB_ATTR_INDEX)11)
#define MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX ((MIB_ATTR_INDEX)12)
#define MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX ((MIB_ATTR_INDEX)13)
#define MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX ((MIB_ATTR_INDEX)14)
#define MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX ((MIB_ATTR_INDEX)15)
#define MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX ((MIB_ATTR_INDEX)16)
#define MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX ((MIB_ATTR_INDEX)17)


#define MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDR_LEN (6)
#define MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_LEN (25)
#define MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_LEN (25)
#define MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_LEN (25)

typedef enum {
    IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP			= 0x01,
    IP_HOST_CFG_DATA_IP_OPTIONS_RESPOND_PING		= 0x02,
    IP_HOST_CFG_DATA_IP_OPTIONS_RESPOND_TRACEROUTE	= 0x04,
    IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_IP_STACK		= 0x08,
} ip_host_cfg_data_ip_options_t;

typedef enum {
    IP_HOST_CFG_DATA_TEST_TYPE_PING				= 1,
    IP_HOST_CFG_DATA_TEST_TYPE_TRACEROUTE		= 2,
    IP_HOST_CFG_DATA_TEST_TYPE_EXTENDED_PING	= 3,
} ip_host_cfg_data_test_type_t;

typedef enum {
    IP_HOST_CFG_DATA_TEST_RESULT_TIME_OUT				= 0,
    IP_HOST_CFG_DATA_TEST_RESULT_ICMP_ECHO				= 1,
    IP_HOST_CFG_DATA_TEST_RESULT_ICMP_TIME_EXCEED		= 2,
    IP_HOST_CFG_DATA_TEST_RESULT_UNEXPECTED_ICMP		= 3,
    IP_HOST_CFG_DATA_TEST_RESULT_TARGET_NOT_RESOLVED	= 4,
} ip_host_cfg_data_test_result_t;

typedef struct {
	UINT16   EntityID;
	UINT8    IpOptions;
	UINT8    MacAddress[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDR_LEN];
	UINT8    OnuIdentifier[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_LEN+1];
	UINT32   IpAddress;
	UINT32   Mask;
	UINT32   Gateway;
	UINT32   PrimaryDns;
	UINT32   SecondaryDns;
	UINT32   CurrentAddress;
	UINT32   CurrentMask;
	UINT32   CurrentGateway;
	UINT32   CurrentPrimaryDns;
	UINT32   CurrentSecondaryDns;
	UINT8    DomainName[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_LEN+1];
	UINT8    HostName[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_LEN+1];
	UINT16   RelayAgentOptions;
} __attribute__((packed)) MIB_TABLE_IP_HOST_CFG_DATA_T;


#ifdef __cplusplus
}
#endif

#endif
