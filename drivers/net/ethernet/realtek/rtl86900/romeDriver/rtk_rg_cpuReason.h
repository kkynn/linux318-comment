#ifndef RTK_RG_CPU_REASON_H
#define RTK_RG_CPU_REASON_H



#if defined(CONFIG_RG_RTL9600_SERIES)

enum cpu_reason {

    CPU_REASON_NORMAL_FWD = 0,
    CPU_REASON_NAT_NAT1 = 1,
    CPU_REASON_NAT_NAT2 = 2,
    CPU_REASON_NAT_NAT3 = 3,
    CPU_REASON_NAT_NAT4 = 4,
    CPU_REASON_NAT_NAT5 = 5,
    CPU_REASON_NAT_PPPOE_ID_LOOKUP_MISS = 6,
    CPU_REASON_NAT_NAT7 = 7,
    CPU_REASON_NAT_L4_FRAGMENT = 8,
    CPU_REASON_NAT_L3_FRAGMENT = 9,
    CPU_REASON_NAT_NAT10 = 10,
    CPU_REASON_NAT_NAT11 = 11,
    CPU_REASON_NAT_NAT12 = 12,
    CPU_REASON_NAT_NAT13 = 13,
    CPU_REASON_NAT_NAT14 = 14,
    CPU_REASON_NAT_NAT15 = 15,
    CPU_REASON_NAT_NAT16 = 16,
    CPU_REASON_NAT_NAT17 = 17,
    CPU_REASON_NAT_NAT18 = 18,
    CPU_REASON_NAT_NAT19 = 19,
    CPU_REASON_NAT_NAT20 = 20,
    CPU_REASON_NAT_ARP_ND_MISS = 21,
    CPU_REASON_NAT_OVER_MTU = 22,
    CPU_REASON_NAT_NAT23 = 23,
    CPU_REASON_NAT_NAT24 = 24,
    CPU_REASON_NAT_NAT25 = 25,
    CPU_REASON_NAT_NAT26 = 26,
    CPU_REASON_NAT_NAT27 = 27,
    CPU_REASON_NAT_NAT28 = 28,
    CPU_REASON_NAT_NAT29 = 29,
    CPU_REASON_NAT_NAT30 = 30,
    CPU_REASON_NAT_NAT31 = 31,
    CPU_REASON_NAT_NAT32 = 32,
    CPU_REASON_NAT_NAT33 = 33,
    CPU_REASON_NAT_LOOKUP_MISS = 34,
    CPU_REASON_NAT_NAT35 = 35,
    CPU_REASON_NAT_NAT36 = 36,
    CPU_REASON_NAT_NAT37 = 37,
    CPU_REASON_NAT_NAT38 = 38,
    CPU_REASON_NAT_NAT39 = 39,
    CPU_REASON_NAT_NAT40 = 40,
    CPU_REASON_NAT_NAT41 = 41,
    CPU_REASON_NAT_NAT42 = 42,
    CPU_REASON_NAT_NAT43 = 43,
    CPU_REASON_NAT_NAT44 = 44,
    CPU_REASON_NAT_NAT45 = 45,
    CPU_REASON_NAT_NAT46 = 46,
    CPU_REASON_NAT_NAT47 = 47,
    CPU_REASON_NAT_NAT48 = 48,
    CPU_REASON_NAT_NAT49 = 49,
    CPU_REASON_NAT_NAT50 = 50,
    CPU_REASON_NAT_NAT51 = 51,
    CPU_REASON_NAT_NAT52 = 52,
    CPU_REASON_NAT_NAT53 = 53,
    CPU_REASON_NAT_NAT54 = 54,
    CPU_REASON_NAT_NAT55 = 55,
    CPU_REASON_NAT_NAT56 = 56,
    CPU_REASON_NAT_NAT57 = 57,
    CPU_REASON_NAT_NAT58 = 58,
    CPU_REASON_NAT_NAT59 = 59,
    CPU_REASON_NAT_NAT60 = 60,

    // reserved: 61

    CPU_REASON_NAT_NAT62 = 62,
    CPU_REASON_NAT_NAT63 = 63,

    CPU_REASON_ACL_ACL1 = 64,
    CPU_REASON_ACL_ACL2 = 65,
    CPU_REASON_ACL_ACL3 = 66,
    CPU_REASON_ACL_ACL4 = 67,
    CPU_REASON_ACL_ACL5 = 68,
    CPU_REASON_ACL_ACL6 = 69,
    CPU_REASON_ACL_ACL7 = 70,
    CPU_REASON_ACL_ACL8 = 71,
    CPU_REASON_ACL_ACL9 = 72,
    CPU_REASON_ACL_ACL10 = 73,
    CPU_REASON_ACL_ACL11 = 74,
    CPU_REASON_ACL_ACL12 = 75,
    CPU_REASON_ACL_ACL13 = 76,
    CPU_REASON_ACL_ACL14 = 77,
    CPU_REASON_ACL_ACL15 = 78,
    CPU_REASON_ACL_ACL16 = 79,
    CPU_REASON_ACL_ACL17 = 80,
    CPU_REASON_ACL_ACL18 = 81,
    CPU_REASON_ACL_ACL19 = 82,
    CPU_REASON_ACL_ACL20 = 83,
    CPU_REASON_ACL_ACL21 = 84,
    CPU_REASON_ACL_ACL22 = 85,
    CPU_REASON_ACL_ACL23 = 86,
    CPU_REASON_ACL_ACL24 = 87,
    CPU_REASON_ACL_ACL25 = 88,
    CPU_REASON_ACL_ACL26 = 89,
    CPU_REASON_ACL_ACL27 = 90,
    CPU_REASON_ACL_ACL28 = 91,
    CPU_REASON_ACL_ACL29 = 92,
    CPU_REASON_ACL_ACL30 = 93,
    CPU_REASON_ACL_ACL31 = 94,
    CPU_REASON_ACL_ACL32 = 95,
    CPU_REASON_ACL_ACL33 = 96,
    CPU_REASON_ACL_ACL34 = 97,
    CPU_REASON_ACL_ACL35 = 98,
    CPU_REASON_ACL_ACL36 = 99,
    CPU_REASON_ACL_ACL37 = 100,
    CPU_REASON_ACL_ACL38 = 101,
    CPU_REASON_ACL_ACL39 = 102,
    CPU_REASON_ACL_ACL40 = 103,
    CPU_REASON_ACL_ACL41 = 104,
    CPU_REASON_ACL_ACL42 = 105,
    CPU_REASON_ACL_ACL43 = 106,
    CPU_REASON_ACL_ACL44 = 107,
    CPU_REASON_ACL_ACL45 = 108,
    CPU_REASON_ACL_ACL46 = 109,
    CPU_REASON_ACL_ACL47 = 110,
    CPU_REASON_ACL_ACL48 = 111,
    CPU_REASON_ACL_ACL49 = 112,
    CPU_REASON_ACL_ACL50 = 113,
    CPU_REASON_ACL_ACL51 = 114,
    CPU_REASON_ACL_ACL52 = 115,
    CPU_REASON_ACL_ACL53 = 116,
    CPU_REASON_ACL_ACL54 = 117,
    CPU_REASON_ACL_ACL55 = 118,
    CPU_REASON_ACL_ACL56 = 119,
    CPU_REASON_ACL_ACL57 = 120,
    CPU_REASON_ACL_ACL58 = 121,
    CPU_REASON_ACL_ACL59 = 122,
    CPU_REASON_ACL_ACL60 = 123,
    CPU_REASON_ACL_ACL61 = 124,
    CPU_REASON_ACL_ACL62 = 125,
    CPU_REASON_ACL_ACL63 = 126,
    CPU_REASON_ACL_ACL64 = 127,

    CPU_REASON_DOS_DAEQSA = 128,
    CPU_REASON_DOS_LAND_ATTACK = 129,
    CPU_REASON_DOS_BLAT_ATTACK = 130,
    CPU_REASON_DOS_SYNFIN_SCAN = 131,
    CPU_REASON_DOS_XMAS_SCAN = 132,
    CPU_REASON_DOS_NULL_SCAN = 133,
    CPU_REASON_DOS_SYN1024 = 134,
    CPU_REASON_DOS_TCP_SHORTHDR = 135,
    CPU_REASON_DOS_TCPFRAGERROR = 136,
    CPU_REASON_DOS_ICMPFRAGMENT = 137,
    CPU_REASON_DOS_PINGOFDEATH = 138,
    CPU_REASON_DOS_UDPBOMB = 139,
    CPU_REASON_DOS_SYNWITHDATA = 140,
    CPU_REASON_DOS_SYNFLOOD = 141,
    CPU_REASON_DOS_FINFLOOD = 142,
    CPU_REASON_DOS_ICMPFLOOD = 143,

    CPU_REASON_CVLAN_POLICING = 144,
    CPU_REASON_CVLAN_EG_MASK = 145,
    CPU_REASON_CVLAN_IG_DROP = 146,
    CPU_REASON_CVLAN_TYPE_CHECK = 147,

    CPU_REASON_SVLAN_UNTAG = 148,
    CPU_REASON_SVLAN_UNMATCH = 149,
    CPU_REASON_SVLAN_DROP = 150,
    CPU_REASON_SVLAN_EG_MASK = 151,

    CPU_REASON_RLPP = 152,
    CPU_REASON_RLD = 153,
    CPU_REASON_LLDP = 154,
    CPU_REASON_OTHER_RLDP = 155,
    CPU_REASON_FORCE_DSL_TRAP = 156,
    CPU_REASON_PKTLEN = 157,
    CPU_REASON_SPANNING_TREE_TX = 158,
    CPU_REASON_SPANNING_TREE_RX = 158,

    CPU_REASON_RMA_IEEE00 = 160,
    CPU_REASON_RMA_IEEE01 = 161,
    CPU_REASON_RMA_IEEE02 = 162,
    CPU_REASON_RMA_IEEE03 = 163,
    CPU_REASON_RMA_IEEE04 = 164,
    CPU_REASON_RMA_IEEE08 = 165,
    CPU_REASON_RMA_IEEE0D = 166,
    CPU_REASON_RMA_IEEE0E = 167,
    CPU_REASON_RMA_IEEE10 = 168,
    CPU_REASON_RMA_IEEE11 = 169,
    CPU_REASON_RMA_IEEE12 = 170,
    CPU_REASON_RMA_IEEE13 = 171,
    CPU_REASON_RMA_IEEE18 = 172,
    CPU_REASON_RMA_IEEE1A = 173,
    CPU_REASON_RMA_IEEE20 = 174,
    CPU_REASON_RMA_IEEE21 = 175,
    CPU_REASON_RMA_IEEE22 = 176,
    CPU_REASON_RMA_CISCO_CC = 177,
    CPU_REASON_RMA_CISCO_CD = 178,

    // reserved: 179-190
    
    CPU_REASON_CPU_DROP_EXT = 191,
    CPU_REASON_L2_LEARN_LIMIT_PERPORT = 192,
    CPU_REASON_L2_LERAN_LIMIT_SYS = 193,

    CPU_REASON_DOT1X_DROP_TRAP = 194,
    CPU_REASON_DOT1X_EGRESS_PM = 195,
    CPU_REASON_UNKNOWN_SA = 196,
    CPU_REASON_UNMATCH_SA = 197,
    CPU_REASON_LINK = 198,
    CPU_REASON_PORTISO = 199,

    CPU_REASON_STORMCTL_BCAST = 200,
    CPU_REASON_STORMCTL_KN_MCAST = 201,
    CPU_REASON_STORMCTL_UNKN_UCAST = 202,
    CPU_REASON_STORMCTL_UNKN_MCAST = 203,

    CPU_REASON_UKNOWNDA_UC = 204,
    CPU_REASON_UKNOWNDA_L2MC = 205,
    CPU_REASON_UKNOWNDA_IPV4MC = 206,
    CPU_REASON_UKNOWNDA_IPV6MC = 207,

    CPU_REASON_MPCP = 208,
    CPU_REASON_DS_MOCI = 209,
    CPU_REASON_CLASSIFY = 210,
    CPU_REASON_OAM = 211,
    CPU_REASON_SA_BLOCK = 212,
    CPU_REASON_DA_BLOCK = 213,
    CPU_REASON_FLOOD = 214,
    CPU_REASON_IGMP = 215,
    CPU_REASON_MCDATA = 216,

    // reserved: 217
    
    CPU_REASON_MIRR_ISO = 218,
    CPU_REASON_EGRESS_DROP = 219,
    CPU_REASON_SRC_BLK = 220,
    CPU_REASON_TX_MIR = 221,
    CPU_REASON_RX_MIR = 222,
    CPU_REASON_L2_FWD = 223,

    CPU_REASON_MTU_BIND_L2 = 224,
    CPU_REASON_MTU_IPMC_ROUTE_BRIDGE = 224,

    CPU_REASON_WAN_DROP = 226,
    CPU_REASON_SNAP_DIRTX = 227,

    // reserved: 228-239

    CPU_REASON_PTP_TRAP = 240,
    CPU_REASON_PTP_RX_MIR = 241,
    
    // reserved: 242-252

    CPU_REASON_PTP_TX_MIR = 253,

    // PTP reserved: 254, 255
    
};
#elif defined(CONFIG_RG_RTL9607C_SERIES) || defined(CONFIG_RG_G3_SERIES) || defined(CONFIG_RG_RTL9603D_SERIES)

#define CPU_REASON_FB_LO_BOUND 1
#define CPU_REASON_FB_UP_BOUND 63
	/* ApolloPro Reason Table */
enum cpu_reason {
	CPU_REASON_ERROR = 0,		//Error Reason
	CPU_REASON_FLOWMISS = 1,
	CPU_REASON_DUALFAIL,
	CPU_REASON_IP6WITHGRE,
	CPU_REASON_WOTCPUDP,
	CPU_REASON_MC_DMACTRANS = 5,
	CPU_REASON_FRAG,
	CPU_REASON_TCPFLAG,
	CPU_REASON_MULTIHIT,
	CPU_REASON_IPEXT,
	CPU_REASON_DUALHDR_NAPT = 10,
	CPU_REASON_TTL,
	CPU_REASON_MTU,
	CPU_REASON_IGSINTF,
	CPU_REASON_SPADENY,
	CPU_REASON_EGSINTF = 15,
#if defined(CONFIG_RG_RTL9603D_SERIES)
	CPU_REASON_BUFFER_FULL,
	CPU_REASON_CACHE_TIMEOUT,
	CPU_REASON_PADDING,
#endif

	CPU_REASON_DROP_FLOW = 20,
	CPU_REASON_DROP_METER,
	CPU_REASON_DROP_WAL,
	CPU_REASON_DROP_CHKSUM,
	CPU_REASON_DROP_IGSINTF,
	CPU_REASON_DROP_IGSIPDENY = 25,
	CPU_REASON_DROP_EGSIPDENY,
	CPU_REASON_DROP_SPADENY,
	CPU_REASON_DROP_EGSINTF = 28,
#if defined(CONFIG_RG_RTL9603D_SERIES)
	CPU_REASON_DROP_BUFFER_FULL,
	CPU_REASON_DROP_CACHE_TIMEOUT,
#endif
	
	CPU_REASON_S2RSN = 32,
	CPU_REASON_FLOWMISS_S2 = 33,
	CPU_REASON_DUALFAIL_S2,
	CPU_REASON_IP6WITHGRE_S2 = 35,
	CPU_REASON_WOTCPUDP_S2,
	CPU_REASON_MC_DMACTRANS_S2,
	CPU_REASON_FRAG_S2,
	CPU_REASON_TCPFLAG_S2,
	CPU_REASON_MULTIHIT_S2 = 40,
	CPU_REASON_IPEXT_S2,
	CPU_REASON_DUALHDR_NAPT_S2,
	CPU_REASON_TTL_S2,
	CPU_REASON_MTU_S2,
	CPU_REASON_IGSINTF_S2 = 45,
	CPU_REASON_SPADENY_S2,
	CPU_REASON_EGSINTF_S2 = 47,
	
	CPU_REASON_DROP_FLOW_S2 = 52,
	CPU_REASON_DROP_METER_S2,
	CPU_REASON_DROP_WAL_S2,
	CPU_REASON_DROP_CHKSUM_S2 = 55,
	CPU_REASON_DROP_IGSINTF_S2,
	CPU_REASON_DROP_IGSIPDENY_S2,
	CPU_REASON_DROP_EGSIPDENY_S2,
	CPU_REASON_DROP_SPADENY_S2,
	CPU_REASON_DROP_EGSINTF_S2=60,

	CPU_REASON_NAT_FWD=64,
	CPU_REASON_NORMAL_FWD=64,
	CPU_REASON_HOST_DA_POLICY	= 103,
	CPU_REASON_UNMATCH_VLAN 	= 201,
	CPU_REASON_FLOOD 				= 202,
	CPU_REASON_BCAST_TRAP 		= 203,
	CPU_REASON_LUT				= 204,
	CPU_REASON_SVLAN_IG 			= 206,
	CPU_REASON_L2_LEARN_LIMIT_PERPORT = 210,
	CPU_REASON_UNKNOWN_DA 		= 211,
	CPU_REASON_UNMATCH_SA 		= 212,
	CPU_REASON_UNKNOWN_SA 		= 213,
	CPU_REASON_RESV_TRAP			= 215,
	CPU_REASON_V6_ICMP_DHCP		= 216,
	CPU_REASON_IGMP				= 219,
	CPU_REASON_HOST_SA_POLICY	= 222,
	CPU_REASON_ACL				= 227,
	CPU_REASON_CF					= 228,
	CPU_REASON_CPUTAG_FORCE 	= 246,

	// Not supportted reason
	CPU_REASON_NAT_L4_FRAGMENT,
	CPU_REASON_NAT_L3_FRAGMENT,
	CPU_REASON_NAT_LOOKUP_MISS,
	CPU_REASON_NAT_PPPOE_ID_LOOKUP_MISS,
	CPU_REASON_NAT_ARP_ND_MISS,
	CPU_REASON_NAT_OVER_MTU,
	CPU_REASON_UKNOWNDA_IPV6MC,
	CPU_REASON_ACL_ACL1,

};

#elif defined(CONFIG_RG_RTL9602C_SERIES)

enum cpu_reason {
    
    CPU_REASON_NORMAL_FWD = 0,
    CPU_REASON_NAT_PPPOE_ID_LOOKUP_MISS = 6,
    CPU_REASON_NAT_L4_FRAGMENT = 8,
    CPU_REASON_NAT_L3_FRAGMENT = 9,
    CPU_REASON_NAT_ARP_ND_MISS = 21,
    CPU_REASON_NAT_OVER_MTU = 22,
    CPU_REASON_NAT_LOOKUP_MISS = 34,

    CPU_REASON_DOS_DAEQSA = 64,
    CPU_REASON_DOS_LAND_ATTACK = 65,
    CPU_REASON_DOS_BLAT_ATTACK = 66,
    CPU_REASON_DOS_SYNFIN_SCAN = 67,
    CPU_REASON_DOS_XMAS_SCAN = 68,
    CPU_REASON_DOS_NULL_SCAN = 69,
    CPU_REASON_DOS_SYN1024 = 70,
    CPU_REASON_DOS_TCP_SHORTHDR = 71,
    CPU_REASON_DOS_TCPFRAGERROR = 72,
    CPU_REASON_DOS_ICMPFRAGMENT = 73,
    CPU_REASON_DOS_PINGOFDEATH = 74,
    CPU_REASON_DOS_UDPBOMB = 75,
    CPU_REASON_DOS_SYNWITHDATA = 76,
    CPU_REASON_DOS_SYNFLOOD = 77,
    CPU_REASON_DOS_FINFLOOD = 78,
    CPU_REASON_DOS_ICMPFLOOD = 79,

    CPU_REASON_FLOOD = 80,
    CPU_REASON_L2_FORWARD = 81,
    CPU_REASON_NAT_MC_TTL1 = 82,
    CPU_REASON_NAT_L2_ENT_INVALID = 83,
    CPU_REASON_NAT_REDIR_FROM_ARP = 84,

    // reserved: 85-86

    CPU_REASON_DOT1X_DROP_TRAP = 87,

    CPU_REASON_STORMCTL_BCAST = 88,
    CPU_REASON_STORMCTL_KN_MCAST = 89,
    CPU_REASON_STORMCTL_UNKN_UCAST = 90,
    CPU_REASON_STORMCTL_UNKN_MCAST = 91,
    CPU_REASON_STORMCTL_ARP = 92,
    CPU_REASON_STORMCTL_DHCP = 93,
    CPU_REASON_STORMCTL_IGMP_MLD = 94,

    CPU_REASON_HOST_POLICY = 95,

    CPU_REASON_CVLAN_INGRESS = 96,
    CPU_REASON_CVLAN_EGRESS = 97,

    // reserved: 98-103

    CPU_REASON_SVLAN_UNTAG = 104,
    CPU_REASON_SVLAN_INGRESS = 105,
    CPU_REASON_SVLAN_EGRESS = 106,

    CPU_REASON_SPANNING_TREE_RX = 107,

    CPU_REASON_L2_LEARN_LIMIT_PERPORT = 108,
    CPU_REASON_L2_LERAN_LIMIT_SYS = 109,
    CPU_REASON_LUT_FULL = 110, 

    CPU_REASON_DOT1X_EGRESS_FILTER = 111,

    CPU_REASON_UKNOWNDA_UC = 112,
    CPU_REASON_UKNOWNDA_L2MC = 113,
    CPU_REASON_UKNOWNDA_IPV4MC = 114,
    CPU_REASON_UKNOWNDA_IPV6MC = 115,

    CPU_REASON_BIND_OVER_MTU = 116,

    CPU_REASON_LUT_VLAN_UNMATCH = 117,

    CPU_REASON_UNMATCH_SA = 118,
    CPU_REASON_UNKNOWN_SA = 119,

    // reserved: 120
    
    CPU_REASON_MCDATA = 121,
    CPU_REASON_RESV_MC = 122,
    CPU_REASON_UNKNOWN_V6_DHCP_ICMP = 123,
    CPU_REASON_DSLITE = 124,
    CPU_REASON_IPMC_TTL_MTU = 125,
    CPU_REASON_CVLAN_TAG_CHECK = 126,
    CPU_REASON_IGMP = 127,

    CPU_REASON_ACL_ACL1 = 128,

    CPU_REASON_RMA = 224,
    CPU_REASON_ACL_METER_DROP = 225,
    CPU_REASON_ACL_PERMIT_DROP = 226,

    CPU_REASON_SPANNING_TREE_TX = 227,
    CPU_REASON_PORT_ISOLATION = 228,
    CPU_REASON_WAN_DROP = 229, 

    // reserved: 230-231

    CPU_REASON_SA_BLOCK = 232,
    CPU_REASON_DA_BLOCK = 233,
    CPU_REASON_OAM = 234,
    CPU_REASON_OTHER_RLDP = 235,
    CPU_REASON_SRC_BLK = 236,

    // reserved: 237
    
    CPU_REASON_CF_FROM_ACT_TABLE = 238,
    CPU_REASON_CF_BY_PERMIT_RULE = 239,
    
    CPU_REASON_PTP_TRAP_FWD = 240,
    CPU_REASON_PTP_RX_MIRROR = 241, 
    CPU_REASON_PTP_EGRESS_DROP = 242,
    CPU_REASON_PTP_TX_MIRROR = 243,
    CPU_REASON_RLPP = 244,
    CPU_REASON_RLDP = 245,
    CPU_REASON_DS_OMCI_MPCP = 246,
    CPU_REASON_CPU_PORTMASK = 247,
    CPU_REASON_MIRR_ISO = 248,
    
    // reserved: 249
    
    CPU_REASON_TX_MIR = 250,
    CPU_REASON_RX_MIR = 251,
    CPU_REASON_EGRESS_DROP = 252,
    CPU_REASON_PKTLEN = 253,
    CPU_REASON_FORCE_DPM = 254,
    CPU_REASON_LINK = 255,
    

};

#elif defined(CONFIG_XDSL_NEW_HWNAT_DRIVER)

#define DONT_MATCH_ANY 200
enum cpu_reason {
    CPU_REASON_NORMAL_FWD = 0,
    CPU_REASON_L2_LEARN_LIMIT_PERPORT = DONT_MATCH_ANY,
    CPU_REASON_UNKNOWN_SA,
    CPU_REASON_NAT_OVER_MTU,
    CPU_REASON_UKNOWNDA_IPV6MC,
    CPU_REASON_NAT_L4_FRAGMENT,
    CPU_REASON_NAT_L3_FRAGMENT,
    CPU_REASON_NAT_LOOKUP_MISS,
    CPU_REASON_NAT_PPPOE_ID_LOOKUP_MISS,
    CPU_REASON_NAT_ARP_ND_MISS,
    CPU_REASON_UKNOWNDA_UC, 
    CPU_REASON_ACL_ACL1,

};

#else
#err cpu reason: unknown platform
#endif


typedef enum rtk_rg_cpu_reason_e
{
    RG_CPU_REASON_NORMAL_FWD = CPU_REASON_NORMAL_FWD,

    RG_CPU_REASON_L2_LEARN_LIMIT_PERPORT = CPU_REASON_L2_LEARN_LIMIT_PERPORT,
    RG_CPU_REASON_NAT_L4_FRAGMENT = CPU_REASON_NAT_L4_FRAGMENT,
    RG_CPU_REASON_NAT_L3_FRAGMENT = CPU_REASON_NAT_L3_FRAGMENT,
    RG_CPU_REASON_NAT_LOOKUP_MISS = CPU_REASON_NAT_LOOKUP_MISS,
    RG_CPU_REASON_NAT_PPPOE_ID_LOOKUP_MISS = CPU_REASON_NAT_PPPOE_ID_LOOKUP_MISS,
    RG_CPU_REASON_NAT_ARP_ND_MISS = CPU_REASON_NAT_ARP_ND_MISS,
    RG_CPU_REASON_NAT_OVER_MTU = CPU_REASON_NAT_OVER_MTU,
#if defined(CONFIG_RG_RTL9607C_SERIES) || defined(CONFIG_RG_G3_SERIES) || defined(CONFIG_RG_RTL9603D_SERIES)
	RG_CPU_REASON_UKNOWNDA_UC = CPU_REASON_UNKNOWN_DA,
#else
    RG_CPU_REASON_UKNOWNDA_UC = CPU_REASON_UKNOWNDA_UC,
#endif
    RG_CPU_REASON_UNKNOWN_SA = CPU_REASON_UNKNOWN_SA,
    RG_CPU_REASON_UKNOWNDA_IPV6MC = CPU_REASON_UKNOWNDA_IPV6MC,
    RG_CPU_REASON_ACL_TRAP_START = CPU_REASON_ACL_ACL1, //used for DHCP offer trap in fwdEngine

    RG_CPU_REASON_MAX_COUNT = 256,
} rtk_rg_cpu_reason_t;

#endif //RTK_RG_CPU_REASON_H
