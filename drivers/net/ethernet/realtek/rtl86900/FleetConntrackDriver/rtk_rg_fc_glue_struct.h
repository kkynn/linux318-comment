
typedef struct rtk_rg_fc_ingress_data_s
{
	struct net_device* srcDev;
	unsigned int  ingressTagif;	// ref to rtk_rg_fc_enum_pktHdrTagif_t
	unsigned char sa[6];
	unsigned char da[6];
	unsigned int srcIp;			// ipv4 address or ipv6 hash value
	unsigned int dstIp;			// ipv4 address or ipv6 hash value
	unsigned int srcPort;
	unsigned int dstPort;
	short sessionId;				//ref to PPPOE_TAGIF
	short srcSVlanId:12;			//ref to SVLAN_TAGIF
	short srcCVlanPri:3;			
	short srcCVlanId:12;			//ref to CVLAN_TAGIF
	short srcSVlanPri:3;			
	char v4tos;						//ref to IPV4_TAGIF
	char v6tos;						//ref to IPV6_TAGIF
	unsigned int ingressLogicPort;
	int wlan_dev_idx;
	char l4Proto;
	unsigned char reason;
	char isDAGatewayMAC:1;
	char isDualHeader:1;			// True: this content belongs to outer header 
	unsigned char outer_ipv:2;		//(IPV_NONE/IPV_IPV4/IPV_IPV6)
	unsigned char inner_ipv:2;		//(IPV_NONE/IPV_IPV4/IPV_IPV6)
	char doLearning:1;
	char isNotLocalOut:1;
	char isDummyPkt:1;		// triger by house keeping
	char outerHdrUnhit:1;
	int flowHashIdx;
}rtk_rg_fc_ingress_data_t;


typedef enum rtk_rg_fc_remark_careBits_e
{
	RTK_RG_FC_RMK_CARE_CTAGIF	=1<<0,
	RTK_RG_FC_RMK_CARE_CVLANID	=1<<1,
	RTK_RG_FC_RMK_CARE_CVLANPRI	=1<<2,
	RTK_RG_FC_RMK_CARE_SMAC		=1<<3,
	RTK_RG_FC_RMK_CARE_SIP		=1<<4,
	
	RTK_RG_FC_RMK_CARE_END		=1<<31,
}rtk_rg_fc_remark_careBits_t;

typedef enum rtk_rg_fc_remark_actionBits_e
{
	RTK_RG_FC_RMK_ACT_CVLANID		=1<<0,
	RTK_RG_FC_RMK_ACT_STREAMLLID	=1<<1,
	
	RTK_RG_FC_RMK_ACT_END			=1<<31,
}rtk_rg_fc_remark_actionBits_t;

typedef struct rtk_rg_fc_remark_pattern_s
{
	rtk_rg_fc_remark_careBits_t careBits;
	unsigned int ctagif:1;
	unsigned int cvlanid:12;
	unsigned int cvlanpri:3;
	unsigned char smac[6];
	unsigned int sip;
}rtk_rg_fc_remark_pattern_t;

typedef struct rtk_rg_fc_remark_action_s
{
	rtk_rg_fc_remark_actionBits_t actBits;
	unsigned int cvlanid:12;
	unsigned int streamllid:7;
}rtk_rg_fc_remark_action_t;

typedef struct rtk_rg_fc_remark_rule_s
{
	unsigned char valid;
	rtk_rg_fc_remark_pattern_t 	pattern;
	rtk_rg_fc_remark_action_t	action;
}rtk_rg_fc_remark_rule_t;

typedef enum rtk_rg_mc_set_mode_e
{
	RG_FC_MC_SYNC_BY_KERNEL=0,
	RG_FC_MC_SYNC_BY_USER,
} rtk_rg_mc_set_mode_t;


typedef struct rtk_rg_fc_mcGroup_cfg_s{

	unsigned char	is_ipv6;	 // 0: ipv4, 1: ipv6 
	union {
		unsigned int   ipv4;
	 	unsigned int   ipv6[4];
	}groupAddr;

	unsigned int	act0_pmsk;   // LAN1-4 ->bit's 0-3	Wlan0 SSID0-4 ->bit's 11-15 Wlan1 SSID0-4 ->bit's 17-21 
	unsigned char	act0_cTagif; // 0: untag, 1: CTag 
	unsigned int	act0_cvid;	 
	unsigned char	act0_cpri;
	unsigned int	act1_pmsk;	 // LAN1-LAN4 ->bit's 0-3	Wlan0 SSID0-SSID4 ->bit's 11-15 Wlan1 SSID0-SSID4 ->bit's 17-21 
	unsigned char	act1_cTagif; // 0: untag, 1: CTag
	unsigned int	act1_cvid;	 
	unsigned char	act1_cpri;
} rtk_rg_fc_mcGroup_cfg_t;

/* Function Name:
*	   rtk_rg_fc_igmp_mcMode
* Description:
*	   setting Multicast learning Mode 
* Input:
*	   rtk_rg_fc_igmp_mcMode	- mcMode
* Output:
*	   N/A
* Return:
*	   0	   - SUCCESS
*	   others  - error code
*/
int rtk_rg_fc_igmp_mcMode(rtk_rg_mc_set_mode_t mcMode);


/* Function Name:
*	   rtk_rg_fc_igmp_mcGroupSet
* Description:
*	   add/del multicast flow 
* Input:
*	   rtk_rg_fc_mc_set_t	- Group info.
*
*	   set act0_pmsk==0 and act1_pmsk==0 to Delete multicast flow
*	   set groupAddr zero to flush all multicast group
*	   act0_pmsk/act1_pmsk bit's mapping 
*				[Port0-Port4 		-> bit's 0-4   ]	
*				[Wlan0 SSID0-SSID4 	-> bit's 11-15 ] 
*				[Wlan1 SSID0-SSID4 	-> bit's 17-21 ]
* Output:
*	   N/A
* Return:
*	   0	   - SUCCESS
*	   others  - error code
*/
int rtk_rg_fc_igmp_mcGroupSet(rtk_rg_fc_mcGroup_cfg_t * mcConfig);



 /* Function Name:
 *      rtk_rg_fc_remarkRule_set
 * Description:
 *      force remarking packet format according to specific patterns.
 * Input:
 *      idx			- entry index, 0~31.
 *      pRmkRule	- rule valid stae, patterns and actions.
 * Output:
 *      N/A
 * Return:
 *      0		- SUCCESS
 *      others 	- error code
 */
int rtk_rg_fc_remarkRule_set(int idx, rtk_rg_fc_remark_rule_t *pRmkRule);
 
 /* Function Name:
 *      rtk_rg_fc_remarkRule_get
 * Description:
 *      force remarking packet format according to specific patterns.
 * Input:
 *      idx		- entry index
 * Output:
 *      pRmkRule	- rule valid stae, patterns and actions.
 * Return:
 *      0		- SUCCESS
 *      others 	- error code
 */
int rtk_rg_fc_remarkRule_get(int idx, rtk_rg_fc_remark_rule_t *pRmkRule);

