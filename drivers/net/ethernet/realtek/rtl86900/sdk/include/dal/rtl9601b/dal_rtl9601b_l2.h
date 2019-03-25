/*
 * Copyright (C) 2012 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 81643 $
 * $Date: 2017-08-24 13:28:53 +0800 (Thu, 24 Aug 2017) $
 *
 * Purpose : Definition of L2 API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) Mac address flush
 *           (2) Address learning limit
 *           (3) Parameter for L2 lookup and learning engine
 *           (4) Unicast address
 *           (5) L2 multicast
 *           (6) IP multicast
 *           (7) Multicast forwarding table
 *           (8) CPU mac
 *           (9) Port move
 *           (10) Parameter for lookup miss
 *           (11) Parameter for MISC
 *
 */

#ifndef __DAL_RTL9601B_L2_H__
#define __DAL_RTL9601B_L2_H__


/*
 * Include Files
 */
#include <common/rt_type.h>
#include <rtk/l2.h>

/*
 * Symbol Definition
 */
/****************************************************************/
/* Constant Definition											*/
/****************************************************************/
#define RTL9601B_RAW_LUT_AGESPEEDMAX    0x1FFFFF
#define RTL9601B_RAW_LUT_FWDPRIMAX      0x7
#define RTL9601B_RAW_LUT_AGEMAX         0x7
#define RTL9601B_RAW_LUT_EXTDSL_SPAMAX  0XF
#define RTL9601B_RAW_LUT_L3IDXAMAX      0XFFFFFFF
#define RTL9601B_RAW_LUT_EXTMBR         0x3F
#define RTL9601B_RAW_LUT_GIPMAX         0XFFFFFF
	
#define RTL9601B_LUT_4WAY_NO            256
#define RTL9601B_LUT_CAM_NO             8
#define RTL9601B_LUT_TBL_NO             (RTL9601B_LUT_4WAY_NO + RTL9601B_LUT_CAM_NO) /*2K 4 ways hash entries + 64 CAM entries*/
#define RTL9601B_LUT_TBL_MAX            (RTL9601B_LUT_TBL_NO - 1)
	
#define RTL9601B_PORTMASK               0x7
	
#define RTL9601B_HASH_FLAG_CHECK(x, y)  ((((x) & (y)) == (y)) ? 1 : 0)

/****************************************************************/
/* Type Definition												*/
/****************************************************************/
typedef enum rtl9601b_raw_l2_entryType_e
{

	RTL9601B_RAW_LUT_ENTRY_TYPE_L2UC = 0,
	RTL9601B_RAW_LUT_ENTRY_TYPE_L2MC_DSL,
	RTL9601B_RAW_LUT_ENTRY_TYPE_L3MC_DSL,
	RTL9601B_RAW_LUT_ENTRY_TYPE_END,

}rtl9601b_raw_l2_entryType_t;

/*for port security usage, include unknow sa/unmatch sa*/
typedef enum rtl9601b_raw_l2_saCtlAct_e
{
	RTL9601B_RAW_LUT_SACTL_ACT_FORWARD = 0,
	RTL9601B_RAW_LUT_SACTL_ACT_DROP,
	RTL9601B_RAW_LUT_SACTL_ACT_TRAP,
	RTL9601B_RAW_LUT_SACTL_ACT_COPY,
	RTL9601B_RAW_LUT_SACTL_ACT_END,
}rtl9601b_raw_l2_saCtlAct_t;

/*for system usage */
typedef enum rtl9601b_raw_l2_commonAct_e
{
	RTL9601B_RAW_L2_COMMON_ACT_FORWARD = 0,
	RTL9601B_RAW_L2_COMMON_ACT_DROP,
	RTL9601B_RAW_L2_COMMON_ACT_TRAP,
	RTL9601B_RAW_L2_COMMON_ACT_COPY2CPU,
	RTL9601B_RAW_L2_COMMON_ACT_END,
}rtl9601b_raw_l2_commonAct_t;


/*for unknow multicat usage*/
typedef enum rtl9601b_raw_l2_unknMcAct_e
{
	RTL9601B_RAW_L2_UNKNMC_ACT_FORWARD = 0,
	RTL9601B_RAW_L2_UNKNMC_ACT_DROP,
	RTL9601B_RAW_L2_UNKNMC_ACT_TRAP,
	RTL9601B_RAW_L2_UNKNMC_ACT_EXCLUDE_RMA,
	RTL9601B_RAW_L2_UNKNMC_ACT_END,
}rtl9601b_raw_l2_unknMcAct_t;


typedef enum rtl9601b_raw_l2_readMethod_e
{
	RTL9601B_RAW_LUT_READ_METHOD_MAC = 0,
	RTL9601B_RAW_LUT_READ_METHOD_ADDRESS,
	RTL9601B_RAW_LUT_READ_METHOD_NEXT_ADDRESS,
	RTL9601B_RAW_LUT_READ_METHOD_NEXT_L2UC,
	RTL9601B_RAW_LUT_READ_METHOD_NEXT_L2MC,
	RTL9601B_RAW_LUT_READ_METHOD_NEXT_L3MC,
	RTL9601B_RAW_LUT_READ_METHOD_NEXT_L2L3MC,
	RTL9601B_RAW_LUT_READ_METHOD_NEXT_L2UCSPA,
	RTL9601B_RAW_LUT_READ_METHOD_END,
}rtl9601b_raw_l2_readMethod_t;

typedef enum rtl9601b_raw_l2_flushStatus_e
{
	RTL9601B_RAW_FLUSH_STATUS_NONBUSY =0,
	RTL9601B_RAW_FLUSH_STATUS_BUSY,
	RTL9601B_RAW_FLUSH_STATUS_END,
}rtl9601b_raw_l2_flushStatus_t;



typedef enum rtl9601b_raw_l2_ipMcHashType_e
{
   RTL9601B_RAW_LUT_IPMCHASH_TYPE_DMACFID=0,
   RTL9601B_RAW_LUT_IPMCHASH_TYPE_DIP,
   RTL9601B_RAW_LUT_IPMCHASH_TYPE_GIPCVID,
   RTL9601B_RAW_LUT_IPMCHASH_TYPE_GIPVID,
   RTL9601B_RAW_LUT_IPMCHASH_TYPE_END,
}rtl9601b_raw_l2_ipMcHashType_t;


/*for port security usage*/
typedef enum rtl9601b_raw_l2_flood_enable_e
{
   RTL9601B_RAW_LUT_FLOOD_DROP=0,
   RTL9601B_RAW_LUT_FLOOD_FLOODING,
   RTL9601B_RAW_LUT_FLOO_END,
}rtl9601b_raw_l2_flood_enable_t;

/*per port IGMP Allow control*/

typedef enum rtl9601b_raw_l2_McAllow_e
{
	RTL9601B_RAW_L2_MC_DROP = 0,
	RTL9601B_RAW_L2_MC_ALLOW,
	RTL9601B_RAW_L2_MC_END,
}rtl9601b_raw_l2_McAllow_t;


typedef enum rtl9601b_raw_l2_l2HashType_e
{
	RTL9601B_RAW_L2_HASH_SVL = 0,
	RTL9601B_RAW_L2_HASH_IVL,
	RTL9601B_RAW_L2_HASH_END
}rtl9601b_raw_l2_l2HashType_t;


typedef enum rtl9601b_raw_l2_cfg_enable_e
{
   RTL9601B_RAW_LUT_CFG_ENABLE=0,
   RTL9601B_RAW_LUT_CFG_DISABLE,
   RTL9601B_RAW_LUT_CFG_END,
   
}rtl9601b_raw_l2_cfg_enable_t;



typedef struct rtl9601b_raw_l2_cfg_s
{
	rtl9601b_raw_l2_ipMcHashType_t ipMcHashType;
	rtl9601b_raw_l2_cfg_enable_t linkDownAgeout;
	rtl9601b_raw_l2_cfg_enable_t bcamDis;
	uint32 ageSpeed;
}rtl9601b_raw_l2_cfg_t;

typedef struct rtl9601b_raw_l2_table_s
{
	rtl9601b_raw_l2_readMethod_t method;
	/*s/w information*/
	rtl9601b_raw_l2_entryType_t table_type;
	uint32 lookup_hit;
	uint32 lookup_busy;
	uint32 address;
	uint32 wait_time;


	/*common part*/
	uint32 l3lookup;
	uint32 vid;
	uint32 valid;
	uint32 is_static;

	/*----L2----*/
	rtk_mac_t mac;
	rtl9601b_raw_l2_l2HashType_t ivl_svl;

	/*l2 uc*/
	uint32 spa;
	uint32 age;
	uint32 ctag_if;
	uint32 ctag_vid;

	/*----L3----*/
	ipaddr_t gip;

	/* ---L3 MC DSL---*/
	uint32 sip_vid;

	/*(L2 MC DSL)(L3 MC DSL)(L3 MC ROUTE)*/
	uint32 mbr;

}rtl9601b_lut_table_t;

/*
 * Data Declaration
 */
int32 rtl9601b_raw_l2_lookUpTb_set(rtl9601b_lut_table_t *pL2Table);
int32 rtl9601b_raw_l2_lookUpTb_get(rtl9601b_lut_table_t *pL2Table);
int32 rtl9601b_raw_l2_flushEn_set(rtk_port_t port, rtk_enable_t enabled);
int32 rtl9601b_raw_l2_flushEn_get(rtk_port_t port, rtk_enable_t *pEnabled);
int32 rtl9601b_raw_l2_unkn_saCtl_set(rtk_port_t port, rtk_action_t action);
int32 rtl9601b_raw_l2_unkn_saCtl_get(rtk_port_t port, rtk_action_t* pAction);
int32 rtl9601b_raw_l2_unkn_ucDaCtl_set(rtk_port_t port, rtk_action_t action);
int32 rtl9601b_raw_l2_unkn_ucDaCtl_get(rtk_port_t port, rtk_action_t* pAction);
int32 rtl9601b_raw_l2_learnOverAct_set(rtk_port_t port, rtk_action_t action);
int32 rtl9601b_raw_l2_learnOverAct_get(rtk_port_t port, rtk_action_t* pAction);
int32 rtl9601b_raw_l2_agingEnable_set(rtk_port_t port, rtk_enable_t enable);
int32 rtl9601b_raw_l2_agingEnable_get(rtk_port_t port, rtk_enable_t *pEnable);
int32 rtl9601b_raw_l2_lrnLimitNo_set(rtk_port_t port, uint32 num);
int32 rtl9601b_raw_l2_lrnLimitNo_get(rtk_port_t port, uint32 *pNum);
int32 rtl9601b_raw_l2_lrnCnt_get(rtk_port_t port, uint32 *pNum);
int32 rtl9601b_raw_l2_lrnOverSts_set(rtk_port_t port, uint32 value);
int32 rtl9601b_raw_l2_lrnOverSts_get(rtk_port_t port, uint32 *pValue);
int32 rtl9601b_raw_l2_sysLrnOverAct_set(rtk_action_t action);
int32 rtl9601b_raw_l2_sysLrnOverAct_get(rtk_action_t* pAction);
int32 rtl9601b_raw_l2_sysLrnLimitNo_set(uint32 num);
int32 rtl9601b_raw_l2_sysLrnLimitNo_get(uint32* pNum);
int32 rtl9601b_raw_l2_sysLrnOverSts_set( uint32 value);
int32 rtl9601b_raw_l2_sysLrnOverSts_get( uint32 *pValue);
int32 rtl9601b_raw_l2_SysLrnCnt_get( uint32 *pNum);
int32 rtl9601b_raw_unkn_l2Mc_set(rtk_port_t port, rtk_action_t action);
int32 rtl9601b_raw_unkn_l2Mc_get(rtk_port_t port, rtk_action_t* pAction);
int32 rtl9601b_raw_unkn_ip4Mc_set(rtk_port_t port, rtk_action_t action);
int32 rtl9601b_raw_unkn_ip4Mc_get(rtk_port_t port, rtk_action_t* pAction);
int32 rtl9601b_raw_unkn_mcPri_set(uint32 prity);
int32 rtl9601b_raw_unkn_mcPri_get(uint32* pPrity);
int32 rtl9601b_raw_l2_bcFlood_set(rtk_port_t port, rtk_enable_t enabled);
int32 rtl9601b_raw_l2_bcFlood_get(rtk_port_t port, rtk_enable_t *pEnabled);
int32 rtl9601b_raw_l2_unknUcFlood_set(rtk_port_t port, rtk_enable_t enabled);
int32 rtl9601b_raw_l2_unknUcFlood_get(rtk_port_t port, rtk_enable_t *pEnabled);
int32 rtl9601b_raw_l2_unknMcFlood_set(rtk_port_t port, rtk_enable_t enabled);
int32 rtl9601b_raw_l2_unknMcFlood_get(rtk_port_t port, rtk_enable_t *pEnabled);
int32 rtl9601b_raw_l2_efid_set(rtk_port_t port, rtk_fid_t efid);
int32 rtl9601b_raw_l2_efid_get(rtk_port_t port, rtk_fid_t* pEfid);
int32 rtl9601b_raw_l2_ipmcAction_set(rtk_port_t port, rtk_action_t action);
int32 rtl9601b_raw_l2_ipmcAction_get(rtk_port_t port, rtk_action_t *pAction);
int32 rtl9601b_raw_l2_igmp_Mc_table_set(uint32 index, rtk_ip_addr_t dip, uint32 portmask);
int32 rtl9601b_raw_l2_igmp_Mc_table_get(uint32 index, rtk_ip_addr_t *pDip, uint32 *pPortmask);

int32 rtl9601b_raw_l2_camEnable_set(rtk_enable_t enable);
int32 rtl9601b_raw_l2_camEnable_get(rtk_enable_t *pEnable);
int32 rtl9601b_raw_l2_agingTime_set(uint32 agingTime);
int32 rtl9601b_raw_l2_agingTime_get(uint32 *pAgingTime);
int32 rtl9601b_raw_l2_ipmcHashType_set(rtl9601b_raw_l2_ipMcHashType_t type);
int32 rtl9601b_raw_l2_ipmcHashType_get(rtl9601b_raw_l2_ipMcHashType_t *pType);
int32 rtl9601b_raw_l2_flushLinkDownPortAddrEnable_set(rtk_enable_t enable);
int32 rtl9601b_raw_l2_flushLinkDownPortAddrEnable_get(rtk_enable_t *pEnable);
int32 rtl9601b_raw_l2_unknReservedMcFlood_set(rtk_enable_t state);
int32 rtl9601b_raw_l2_unknReservedMcFlood_get(rtk_enable_t *pState);
int32 rtl9601b_raw_unkn_ip6Mc_get(rtk_port_t port, rtk_action_t* pAction);
int32 rtl9601b_raw_unkn_ip6Mc_set(rtk_port_t port, rtk_action_t  action);


/*
 * Macro Declaration
 */


/*
 * Function Declaration
 */

/* Module Name    : L2     */
/* Sub-module Name: Global */
/* Function Name:
 *      dal_rtl9601b_l2_init
 * Description:
 *      Initialize l2 module of the specified device.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      Must initialize l2 module before calling any l2 APIs.
 */
extern int32
dal_rtl9601b_l2_init(void);

/* Module Name    : L2                */
/* Sub-module Name: Mac address flush */

/* Function Name:
 *      dal_rtl9601b_l2_flushLinkDownPortAddrEnable_get
 * Description:
 *      Get HW flush linkdown port mac configuration.
 * Input:
 *      None
 * Output:
 *      pEnable - pointer buffer of state of HW clear linkdown port mac
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) Make sure chip have supported the function before using the API.
 *      (2) The API is apply to whole system.
 *      (3) The status of flush linkdown port address is as following:
 *          - DISABLED
 *          - ENABLED
 */
extern int32
dal_rtl9601b_l2_flushLinkDownPortAddrEnable_get(rtk_enable_t *pEnable);


/* Function Name:
 *      dal_rtl9601b_l2_flushLinkDownPortAddrEnable_set
 * Description:
 *      Set HW flush linkdown port mac configuration.
 * Input:
 *      enable - configure value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT   - invalid input parameter
 * Note:
 *      (1) Make sure chip have supported the function before using the API.
 *      (2) The API is apply to whole system.
 *      (3) The status of flush linkdown port address is as following:
 *          - DISABLED
 *          - ENABLED
 */
extern int32
dal_rtl9601b_l2_flushLinkDownPortAddrEnable_set(rtk_enable_t enable);



/* Function Name:
 *      dal_rtl9601b_l2_ucastAddr_flush
 * Description:
 *      Flush unicast address
 * Input:
 *      pConfig - flush config
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_ucastAddr_flush(rtk_l2_flushCfg_t *pConfig);

/* Function Name:
 *      dal_rtl9601b_l2_table_clear
 * Description:
 *      Clear entire L2 table.
 *      All the entries (static and dynamic) (L2 and L3) will be deleted.
 * Input:
 *      None.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_table_clear(void);


/* Module Name    : L2                     */
/* Sub-module Name: Address learning limit */

/* Function Name:
 *      dal_rtl9601b_l2_portLimitLearningOverStatus_get
 * Description:
 *      Get the port learning over status
 * Input:
 *      port        - Port ID
 * Output:
 *      pStatus     - 1: learning over, 0: not learning over
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *
 */
extern int32
dal_rtl9601b_l2_portLimitLearningOverStatus_get(rtk_port_t port, uint32 *pStatus);

/* Function Name:
 *      dal_rtl9601b_l2_portLimitLearningOverStatus_clear
 * Description:
 *      Clear the port learning over status
 * Input:
 *      port        - Port ID
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 * Note:
 *
 */
extern int32
dal_rtl9601b_l2_portLimitLearningOverStatus_clear(rtk_port_t port);

/* Function Name:
 *      dal_rtl9601b_l2_portLearningCnt_get
 * Description:
 *      Get the mac learning counts of the port.
 * Input:
 *      port     - port id
 * Output:
 *      pMacCnt  - pointer buffer of mac learning counts of the port
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The mac learning counts only calculate dynamic mac numbers.
 */
extern int32
dal_rtl9601b_l2_portLearningCnt_get(rtk_port_t port, uint32 *pMacCnt);

/* Function Name:
 *      dal_rtl9601b_l2_portLimitLearningCnt_get
 * Description:
 *      Get the maximum mac learning counts of the port.
 * Input:
 *      port     - port id
 * Output:
 *      pMacCnt - pointer buffer of maximum mac learning counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *          address, not apply to static mac address.
 */
extern int32
dal_rtl9601b_l2_portLimitLearningCnt_get(rtk_port_t port, uint32 *pMacCnt);


/* Function Name:
 *      dal_rtl9601b_l2_portLimitLearningCnt_set
 * Description:
 *      Set the maximum mac learning counts of the port.
 * Input:
 *      port    - port id
 *      macCnt  - maximum mac learning counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID             - invalid port id
 *      RT_ERR_LIMITED_L2ENTRY_NUM - invalid limited L2 entry number
 * Note:
 *      (1) The maximum mac learning counts only limit for dynamic learning mac
 *          address, not apply to static mac address.
 *      (2) Set the macCnt to 0 mean disable learning in the port.
 */
extern int32
dal_rtl9601b_l2_portLimitLearningCnt_set(rtk_port_t port, uint32 macCnt);


/* Function Name:
 *      dal_rtl9601b_l2_portLimitLearningCntAction_get
 * Description:
 *      Get the action when over learning maximum mac counts of the port.
 * Input:
 *      port    - port id
 * Output:
 *      pLearningAction - pointer buffer of action when over learning maximum mac counts
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      The action symbol as following
 *      - LIMIT_LEARN_CNT_ACTION_DROP
 *      - LIMIT_LEARN_CNT_ACTION_FORWARD
 *      - LIMIT_LEARN_CNT_ACTION_TO_CPU
 *      - LIMIT_LEARN_CNT_ACTION_COPY_CPU
 */
extern int32
dal_rtl9601b_l2_portLimitLearningCntAction_get(rtk_port_t port, rtk_l2_limitLearnCntAction_t *pLearningAction);


/* Function Name:
 *      dal_rtl9601b_l2_portLimitLearningCntAction_set
 * Description:
 *      Set the action when over learning maximum mac counts of the port.
 * Input:
 *      port   - port id
 *      learningAction - action when over learning maximum mac counts
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID - invalid port id
 * Note:
 *      The action symbol as following
 *      - LIMIT_LEARN_CNT_ACTION_DROP
 *      - LIMIT_LEARN_CNT_ACTION_FORWARD
 *      - LIMIT_LEARN_CNT_ACTION_TO_CPU
 *      - LIMIT_LEARN_CNT_ACTION_COPY_CPU
 */
extern int32
dal_rtl9601b_l2_portLimitLearningCntAction_set(rtk_port_t port, rtk_l2_limitLearnCntAction_t learningAction);



/* Module Name    : L2                                          */
/* Sub-module Name: Parameter for L2 lookup and learning engine */

/* Function Name:
 *      dal_rtl9601b_l2_aging_get
 * Description:
 *      Get the dynamic address aging time.
 * Input:
 *      None
 * Output:
 *      pAgingTime - pointer buffer of aging time
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Get aging_time as 0 mean disable aging mechanism. (0.1sec)
 */
extern int32
dal_rtl9601b_l2_aging_get(uint32 *pAgingTime);


/* Function Name:
 *      dal_rtl9601b_l2_aging_set
 * Description:
 *      Set the dynamic address aging time.
 * Input:
 *      agingTime - aging time
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_INPUT   - invalid input parameter
 * Note:
 *      (1) RTL8329/RTL8389 aging time is not configurable.
 *      (2) apply aging_time as 0 mean disable aging mechanism.
 */
extern int32
dal_rtl9601b_l2_aging_set(uint32 agingTime);


/* Function Name:
 *      dal_rtl9601b_l2_portAgingEnable_get
 * Description:
 *      Get the dynamic address aging out setting of the specified port.
 * Input:
 *      port    - port id
 * Output:
 *      pEnable - pointer to enable status of Age
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_portAgingEnable_get(rtk_port_t port, rtk_enable_t *pEnable);

/* Function Name:
 *      dal_rtl9601b_l2_portAgingEnable_set
 * Description:
 *      Set the dynamic address aging out configuration of the specified port
 * Input:
 *      port    - port id
 *      enable  - enable status of Age
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID  - invalid port id
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_portAgingEnable_set(rtk_port_t port, rtk_enable_t enable);

/* Module Name    : L2      */
/* Sub-module Name: Parameter for lookup miss */
/* Function Name:
 *      dal_rtl9601b_l2_lookupMissAction_get
 * Description:
 *      Get forwarding action when destination address lookup miss.
 * Input:
 *      type    - type of lookup miss
 * Output:
 *      pAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_INPUT        - invalid type of lookup miss
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_BCAST
 *      - DLF_TYPE_MCAST
 *
 *      Forwarding action is as following:
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_FLOOD_IN_VLAN
 *      - ACTION_FLOOD_IN_ALL_PORT  (only for DLF_TYPE_MCAST)
 *      - ACTION_FLOOD_IN_ROUTER_PORTS (only for DLF_TYPE_IPMC)
 */
extern int32
dal_rtl9601b_l2_lookupMissAction_get(rtk_l2_lookupMissType_t type, rtk_action_t *pAction);

/* Function Name:
 *      dal_rtl9601b_l2_lookupMissAction_set
 * Description:
 *      Set forwarding action when destination address lookup miss.
 * Input:
 *      type   - type of lookup miss
 *      action - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_INPUT      - invalid type of lookup miss
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_BCAST
 *      - DLF_TYPE_MCAST
 *
 *      Forwarding action is as following:
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_FLOOD_IN_VLAN
 *      - ACTION_FLOOD_IN_ALL_PORT  (only for DLF_TYPE_MCAST)
 *      - ACTION_FLOOD_IN_ROUTER_PORTS (only for DLF_TYPE_IPMC)
 */
extern int32
dal_rtl9601b_l2_lookupMissAction_set(rtk_l2_lookupMissType_t type, rtk_action_t action);

/* Function Name:
 *      dal_rtl9601b_l2_portLookupMissAction_get
 * Description:
 *      Get forwarding action of specified port when destination address lookup miss.
 * Input:
 *      port    - port id
 *      type    - type of lookup miss
 * Output:
 *      pAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_INPUT            - invalid type of lookup miss
 *      RT_ERR_NULL_POINTER     - input parameter may be null pointer
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_MCAST
 *      - DLF_TYPE_IP6MC
 *
 *      Forwarding action is as following:
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_FORWARD
 *      - ACTION_DROP_EXCLUDE_RMA (Only for DLF_TYPE_MCAST)
 */
extern int32
dal_rtl9601b_l2_portLookupMissAction_get(rtk_port_t port, rtk_l2_lookupMissType_t type, rtk_action_t *pAction);

/* Function Name:
 *      dal_rtl9601b_l2_portLookupMissAction_set
 * Description:
 *      Set forwarding action of specified port when destination address lookup miss.
 * Input:
 *      port    - port id
 *      type    - type of lookup miss
 *      action  - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT         - The module is not initial
 *      RT_ERR_INPUT            - invalid type of lookup miss
 *      RT_ERR_PORT_ID          - invalid port id
 *      RT_ERR_FWD_ACTION       - invalid forwarding action
 * Note:
 *      Type of lookup missis as following:
 *      - DLF_TYPE_IPMC
 *      - DLF_TYPE_UCAST
 *      - DLF_TYPE_MCAST
 *      - DLF_TYPE_IP6MC
 *
 *      Forwarding action is as following:
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_FORWARD
 *      - ACTION_DROP_EXCLUDE_RMA (Only for DLF_TYPE_MCAST)
 */
extern int32
dal_rtl9601b_l2_portLookupMissAction_set(rtk_port_t port, rtk_l2_lookupMissType_t type, rtk_action_t action);

/* Function Name:
 *      dal_rtl9601b_l2_lookupMissFloodPortMask_get
 * Description:
 *      Get flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      type   - type of lookup miss
 * Output:
 *      pFlood_portmask - flooding port mask configuration when unicast/multicast lookup missed.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      DLF_TYPE_IPMC, DLF_TYPE_IP6MC & DLF_TYPE_MCAST shares the same configuration.
 */
extern int32
dal_rtl9601b_l2_lookupMissFloodPortMask_get(rtk_l2_lookupMissType_t type, rtk_portmask_t *pFlood_portmask);

/* Function Name:
 *      dal_rtl9601b_l2_lookupMissFloodPortMask_set
 * Description:
 *      Set flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      type            - type of lookup miss
 *      pFlood_portmask - flooding port mask configuration when unicast/multicast lookup missed.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      DLF_TYPE_IPMC, DLF_TYPE_IP6MC & DLF_TYPE_MCAST shares the same configuration.
 */
extern int32
dal_rtl9601b_l2_lookupMissFloodPortMask_set(rtk_l2_lookupMissType_t type, rtk_portmask_t *pFlood_portmask);

/* Function Name:
 *      dal_rtl9601b_l2_lookupMissFloodPortMask_add
 * Description:
 *      Add one port member to flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      type        - type of lookup miss
 *      flood_port  - port id that is going to be added in flooding port mask.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      DLF_TYPE_IPMC & DLF_TYPE_MCAST shares the same configuration.
 */
extern int32
dal_rtl9601b_l2_lookupMissFloodPortMask_add(rtk_l2_lookupMissType_t type, rtk_port_t flood_port);

/* Function Name:
 *      dal_rtl9601b_l2_lookupMissFloodPortMask_del
 * Description:
 *      Del one port member in flooding port mask when unicast or multicast address lookup missed in L2 table.
 * Input:
 *      type        - type of lookup miss
 *      flood_port  - port id that is going to be added in flooding port mask.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      DLF_TYPE_IPMC & DLF_TYPE_MCAST shares the same configuration..
 */
extern int32
dal_rtl9601b_l2_lookupMissFloodPortMask_del(rtk_l2_lookupMissType_t type, rtk_port_t flood_port);

/* Module Name    : L2      */
/* Sub-module Name: Unicast */
/* Function Name:
 *      dal_rtl9601b_l2_newMacOp_get
 * Description:
 *      Get learning mode and forwarding action of new learned address on specified port.
 * Input:
 *      port       - port id
 * Output:
 *      pLrnMode   - pointer to learning mode
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 *
 *      Learning mode is as following
 *      - HARDWARE_LEARNING
 *      - SOFTWARE_LEARNING
 *      - NOT_LEARNING
 */
extern int32
dal_rtl9601b_l2_newMacOp_get(
    rtk_port_t              port,
    rtk_l2_newMacLrnMode_t  *pLrnMode,
    rtk_action_t            *pFwdAction);

/* Function Name:
 *      dal_rtl9601b_l2_newMacOp_set
 * Description:
 *      Set learning mode and forwarding action of new learned address on specified port.
 * Input:
 *      port      - port id
 *      lrnMode   - learning mode
 *      fwdAction - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_INPUT      - invalid input parameter
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 *
 *      Learning mode is as following
 *      - HARDWARE_LEARNING
 *      - SOFTWARE_LEARNING
 *      - NOT_LEARNING
 */
extern int32
dal_rtl9601b_l2_newMacOp_set(
    rtk_port_t              port,
    rtk_l2_newMacLrnMode_t  lrnMode,
    rtk_action_t            fwdAction);


/* Module Name    : L2              */
/* Sub-module Name: Get next entry */

/* Function Name:
 *      dal_rtl9601b_l2_nextValidAddr_get
 * Description:
 *      Get next valid L2 unicast address entry from the specified device.
 * Input:
 *      pScanIdx      - currently scan index of l2 table to get next.
 *      include_static - the get type, include static mac or not.
 * Output:
 *      pL2UcastData   - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 multicast and ip multicast entry and
 *          reply next valid L2 unicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScanIdx is the input and also is the output argument.
 */
extern int32
dal_rtl9601b_l2_nextValidAddr_get(
    int32               *pScanIdx,
    rtk_l2_ucastAddr_t  *pL2UcastData);


/* Function Name:
 *      dal_rtl9601b_l2_nextValidAddrOnPort_get
 * Description:
 *      Get next valid L2 unicast address entry from specify port.
 * Input:
 *      pScanIdx      - currently scan index of l2 table to get next.
 * Output:
 *      pL2UcastData  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 multicast and ip multicast entry and
 *          reply next valid L2 unicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScanIdx is the input and also is the output argument.
 */
extern int32
dal_rtl9601b_l2_nextValidAddrOnPort_get(
    rtk_port_t          port,
    int32               *pScanIdx,
    rtk_l2_ucastAddr_t  *pL2UcastData);


/* Function Name:
 *      dal_rtl9601b_l2_nextValidMcastAddr_get
 * Description:
 *      Get next valid L2 multicast address entry from the specified device.
 * Input:
 *      pScanIdx - currently scan index of l2 table to get next.
 * Output:
 *      pL2McastData  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 unicast and ip multicast entry and
 *          reply next valid L2 multicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScan_idx is the input and also is the output argument.
 */
extern int32
dal_rtl9601b_l2_nextValidMcastAddr_get(
    int32               *pScanIdx,
    rtk_l2_mcastAddr_t  *pL2McastData);

/* Function Name:
 *      dal_rtl9601b_l2_nextValidIpMcastAddr_get
 * Description:
 *      Get next valid L2 ip multicast address entry from the specified device.
 * Input:
 *      pScanIdx - currently scan index of l2 table to get next.
 * Output:
 *      pIpMcastData  - structure of l2 address data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      (1) The function will skip valid l2 unicast and multicast entry and
 *          reply next valid L2 ip multicast address is based on index order of l2 table.
 *      (2) Please input 0 for get the first entry of l2 table.
 *      (3) The pScan_idx is the input and also is the output argument.
 */
extern int32
dal_rtl9601b_l2_nextValidIpMcastAddr_get(
    int32                   *pScanIdx,
    rtk_l2_ipMcastAddr_t    *pIpMcastData);



/* Function Name:
 *      dal_rtl9601b_l2_nextValidEntry_get
 * Description:
 *      Get LUT next valid entry.
 * Input:
 *      pScanIdx - Index field in the structure.
 * Output:
 *      pL2Entry - entry content
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_L2_EMPTY_ENTRY   - Empty LUT entry.
 *      RT_ERR_INPUT            - Invalid input parameters.
 * Note:
 *      This API is used to get next valid LUT entry.
 */
extern int32
dal_rtl9601b_l2_nextValidEntry_get(
        int32                   *pScanIdx,
        rtk_l2_addr_table_t     *pL2Entry);



/* Module Name    : L2              */
/* Sub-module Name: Unicast address */

/* Function Name:
 *      dal_rtl9601b_l2_addr_add
 * Description:
 *      Add L2 entry to ASIC.
 * Input:
 *      pL2_addr - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize L2 entry before add it.
 */
extern int32
dal_rtl9601b_l2_addr_add(rtk_l2_ucastAddr_t *pL2Addr);

/* Function Name:
 *      dal_rtl9601b_l2_addr_del
 * Description:
 *      Delete a L2 unicast address entry.
 * Input:
 *      pL2Addr  - L2 entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_VLAN_VID          - invalid vid
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      If the mac has existed in the LUT, it will be deleted. Otherwise, it will return RT_ERR_L2_ENTRY_NOTFOUND.
 */
extern int32
dal_rtl9601b_l2_addr_del(rtk_l2_ucastAddr_t *pL2Addr);


/* Function Name:
 *      dal_rtl9601b_l2_addr_get
 * Description:
 *      Get L2 entry based on specified vid and MAC address
 * Input:
 *      None
 * Output:
 *      pL2Addr - pointer to L2 entry
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT          - The module is not initial
 *      RT_ERR_VLAN_VID          - invalid vlan id
 *      RT_ERR_MAC               - invalid mac address
 *      RT_ERR_NULL_POINTER      - input parameter may be null pointer
 *      RT_ERR_L2_ENTRY_NOTFOUND - specified entry not found
 * Note:
 *      If the unicast mac address existed in LUT, it will return the port and fid where
 *      the mac is learned. Otherwise, it will return a RT_ERR_L2_ENTRY_NOTFOUND error.
 */
extern int32
dal_rtl9601b_l2_addr_get(rtk_l2_ucastAddr_t *pL2Addr);


/* Function Name:
 *      dal_rtl9601b_l2_addr_delAll
 * Description:
 *      Delete all L2 unicast address entry.
 * Input:
 *      includeStatic - include static mac or not?
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_addr_delAll(uint32 includeStatic);


/* Module Name    : L2           */
/* Sub-module Name: l2 multicast */

/* Function Name:
 *      dal_rtl9601b_l2_mcastAddr_add
 * Description:
 *      Add L2 multicast entry to ASIC.
 * Input:
 *      pMcastAddr - L2 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      Need to initialize L2 multicast entry before add it.
 */
extern int32
dal_rtl9601b_l2_mcastAddr_add(rtk_l2_mcastAddr_t *pMcastAddr);

/* Function Name:
 *      dal_rtl9601b_l2_mcastAddr_del
 * Description:
 *      Delete a L2 multicast address entry.
 * Input:
 *      pMcastAddr - L2 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_mcastAddr_del(rtk_l2_mcastAddr_t *pMcastAddr);

/* Function Name:
 *      dal_rtl9601b_l2_mcastAddr_get
 * Description:
 *      Update content of L2 multicast entry.
 * Input:
 *      pMcastAddr - L2 multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_VLAN_VID     - invalid vlan id
 *      RT_ERR_MAC          - invalid mac address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 *      RT_ERR_INPUT        - invalid input parameter
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_mcastAddr_get(rtk_l2_mcastAddr_t *pMcastAddr);

/* Module Name    : L2        */
/* Sub-module Name: Port move */
/* Function Name:
 *      dal_rtl9601b_l2_illegalPortMoveAction_get
 * Description:
 *      Get forwarding action when illegal port moving happen on specified port.
 * Input:
 *      port       - port id
 * Output:
 *      pFwdAction - pointer to forwarding action
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_PORT_ID      - invalid port id
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
extern int32
dal_rtl9601b_l2_illegalPortMoveAction_get(
    rtk_port_t          port,
    rtk_action_t        *pFwdAction);

/* Function Name:
 *      dal_rtl9601b_l2_illegalPortMoveAction_set
 * Description:
 *      Set forwarding action when illegal port moving happen on specified port.
 * Input:
 *      port      - port id
 *      fwdAction - forwarding action
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_PORT_ID    - invalid port id
 *      RT_ERR_FWD_ACTION - invalid forwarding action
 * Note:
 *      Forwarding action is as following
 *      - ACTION_FORWARD
 *      - ACTION_DROP
 *      - ACTION_TRAP2CPU
 *      - ACTION_COPY2CPU
 */
extern int32
dal_rtl9601b_l2_illegalPortMoveAction_set(
    rtk_port_t          port,
    rtk_action_t        fwdAction);


/* Module Name    : L2           */
/* Sub-module Name: IP multicast */


/* Function Name:
 *      dal_rtl9601b_l2_ipmcMode_get
 * Description:
 *      Get lookup mode of layer2 ip multicast switching.
 * Input:
 *      None
 * Output:
 *      pMode - pointer to lookup mode of layer2 ip multicast switching
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Lookup mode of layer2 ip multicast switching is as following
 *      - LOOKUP_ON_DIP_AND_SIP
 *      - LOOKUP_ON_MAC_AND_VID_FID
 *      - LOOKUP_ON_DPI_AND_VID
 */
extern int32
dal_rtl9601b_l2_ipmcMode_get(rtk_l2_ipmcMode_t *pMode);

/* Function Name:
 *      dal_rtl9601b_l2_ipmcMode_set
 * Description:
 *      Set lookup mode of layer2 ip multicast switching.
 * Input:
 *      mode - lookup mode of layer2 ip multicast switching
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT - The module is not initial
 *      RT_ERR_INPUT    - invalid input parameter
 * Note:
 *      Lookup mode of layer2 ip multicast switching is as following
 *      - LOOKUP_ON_DIP_AND_SIP
 *      - LOOKUP_ON_MAC_AND_VID_FID
 *      - LOOKUP_ON_DPI_AND_VID
 */
extern int32
dal_rtl9601b_l2_ipmcMode_set(rtk_l2_ipmcMode_t mode);

/* Function Name:
 *      dal_rtl9601b_l2_ipmcGroupLookupMissHash_get
 * Description:
 *      Get Hash operation of IPv4 multicast packet which is not in IPMC Group Table.
 * Input:
 *      None
 * Output:
 *      pIpmcHash - pointer of Hash operation of IPv4 multicast packet which is not in IPMC Group Table
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None.
 */
extern int32
dal_rtl9601b_l2_ipmcGroupLookupMissHash_get(rtk_l2_ipmcHashOp_t *pIpmcHash);

/* Function Name:
 *      dal_rtl9601b_l2_ipmcGroupLookupMissHash_set
 * Description:
 *      Set Hash operation of IPv4 multicast packet which is not in IPMC Group Table.
 * Input:
 *      ipmcHash - Hash operation of IPv4 multicast packet which is not in IPMC Group Table
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None.
 */
extern int32
dal_rtl9601b_l2_ipmcGroupLookupMissHash_set(rtk_l2_ipmcHashOp_t ipmcHash);

/* Function Name:
 *      dal_rtl9601b_l2_portIpmcAction_get
 * Description:
 *      Get the Action of IPMC packet per ingress port.
 * Input:
 *      port        - Ingress port number
 * Output:
 *      pAction     - IPMC packet action (ACTION_FORWARD or ACTION_DROP)
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_portIpmcAction_get(rtk_port_t port, rtk_action_t *pAction);

/* Function Name:
 *      dal_rtl9601b_l2_portIpmcAction_set
 * Description:
 *      Set the Action of IPMC packet per ingress port.
 * Input:
 *      port        - Ingress port number
 *      action      - IPMC packet action (ACTION_FORWARD or ACTION_DROP)
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT   - The module is not initial
 *      RT_ERR_INPUT      - Invalid input parameter
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_portIpmcAction_set(rtk_port_t port, rtk_action_t action);

/* Function Name:
 *      dal_rtl9601b_l2_ipMcastAddr_add
 * Description:
 *      Add IP multicast entry to ASIC.
 * Input:
 *      pIpmcastAddr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT      - The module is not initial
 *      RT_ERR_IPV4_ADDRESS  - Invalid IPv4 address
 *      RT_ERR_VLAN_VID      - invalid vlan id
 *      RT_ERR_NULL_POINTER  - input parameter may be null pointer
 *      RT_ERR_INPUT         - invalid input parameter
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
extern int32
dal_rtl9601b_l2_ipMcastAddr_add(rtk_l2_ipMcastAddr_t *pIpmcastAddr);

/* Function Name:
 *      dal_rtl9601b_l2_ipMcastAddr_del
 * Description:
 *      Delete a L2 ip multicast address entry from the specified device.
 * Input:
 *      pIpmcastAddr  - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_L2_HASH_KEY    - invalid L2 Hash key
 *      RT_ERR_L2_EMPTY_ENTRY - the entry is empty(invalid)
 * Note:
 *      (1) In vlan unaware mode (SVL), the vid will be ignore, suggest to
 *          input vid=0 in vlan unaware mode.
 *      (2) In vlan aware mode (IVL), the vid will be care.
 */
extern int32
dal_rtl9601b_l2_ipMcastAddr_del(rtk_l2_ipMcastAddr_t *pIpmcastAddr);

/* Function Name:
 *      dal_rtl9601b_l2_ipMcastAddr_get
 * Description:
 *      Get IP multicast entry on specified dip and sip.
 * Input:
 *      pIpmcastAddr - IP multicast entry
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_IPV4_ADDRESS - Invalid IPv4 address
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      Need to initialize IP multicast entry before add it.
 */
extern int32
dal_rtl9601b_l2_ipMcastAddr_get(rtk_l2_ipMcastAddr_t *pIpmcastAddr);

/* Module Name    : L2                 */
/* Sub-module Name: Parameter for MISC */


/* Function Name:
 *      dal_rtl9601b_l2_srcPortEgrFilterMask_get
 * Description:
 *      Get source port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      None
 * Output:
 *      pFilter_portmask - source port egress filtering configuration when packet egress.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected.
 *      Get permittion status for frames if its source port is equal to destination port.
 */
extern int32
dal_rtl9601b_l2_srcPortEgrFilterMask_get(rtk_portmask_t *pFilter_portmask);

/* Function Name:
 *      dal_rtl9601b_l2_srcPortEgrFilterMask_set
 * Description:
 *      Set source port egress filter mask to determine if mac need to do source filtering for an specific port
 *      when packet egress.
 * Input:
 *      pFilter_portmask - source port egress filtering configuration when packet egress.
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected
 */
extern int32
dal_rtl9601b_l2_srcPortEgrFilterMask_set(rtk_portmask_t *pFilter_portmask);

/* Function Name:
 *      dal_rtl9601b_l2_camState_set
 * Description:
 *      Set LUT cam state 
 * Input:
 *      camState - enable or disable cam state 
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *      May be used when wirless device connected
 */
extern int32
dal_rtl9601b_l2_camState_set(rtk_enable_t camState);


/* Function Name:
 *      dal_rtl9601b_l2_camState_get
 * Description:
 *      Get LUT cam state 
 * Input:
 *      pCamState - status of cam state 
 * Output:
 *      None.
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_INIT     - The module is not initial
 *      RT_ERR_NULL_POINTER - input parameter may be null pointer
 * Note:
 *       
 */
extern int32
dal_rtl9601b_l2_camState_get(rtk_enable_t *pCamState);

/* Function Name:
 *      dal_rtl9601b_l2_hashValue_get
 * Description:
 *      Calculate the hash value for specific hash type
 * Input:
 *      type        - hash type
 *      hashData    - input data to be hashed
 * Output:
 *      pHashValue  - hash result of specified type with data
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 *      RT_ERR_NULL_POINTER
 *      RT_ERR_CHIP_NOT_SUPPORTED
 * Note:
 *      None
 */
extern int32
dal_rtl9601b_l2_hashValue_get(rtk_l2_hashType_t type, rtk_l2_hashData_t hashData, uint32 *pHashValue);
#endif /* __DAL_RTL9601B_L2_H__ */

