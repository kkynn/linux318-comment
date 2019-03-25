/*
* Copyright (C) 2015 Realtek Semiconductor Corp.
* All Rights Reserved.
*
* This program is the proprietary software of Realtek Semiconductor
* Corporation and/or its licensors, and only be used, duplicated,
* modified or distributed under the authorized license from Realtek.
*
* ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
* THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
*
* $Revision: 1 $
* $Date: 2015-09-18 11:22:15 +0800 (Fri, 18 Sep 2015) $
*
* Purpose : handle tx and get vlan related info from oam ctc
*
* Feature : 
*
*/

/*
 * Include Files
 */
#include <stdio.h>
#include <string.h>

/* EPON OAM include */
#include <epon_oam_err.h>
#include <sys_portview.h>
#include <sys_portmask.h>

#include <ctc_mc.h>
#include <drv_vlan.h>
#include <drv_mac.h>
#include <drv_convert.h>
#include "epon_oam_igmp.h"
#include "epon_oam_igmp_db.h"
#include "epon_oam_igmp_util.h"
#include "epon_oam_igmp_hw.h"
#include "epon_oam_rx.h"
#include "pkt_redirect_user.h"

#define MAX_FRAME_LEN           1518
#define MIN_FRAME_LEN           60
#define FWD_PORT_MASK_LEN       1

#define PR_KERNEL_UID_IGMPMLD 8
#define DEFAULT_LLID_IDX 0

typedef struct l2_send_op_s{    
	rtk_portmask_t portmask;
}l2_send_op_t;

static sem_t igmpPktSem;

static int igmpMcVlanNum=0;
static uint32 igmpOperPerPort[MAX_PORT_NUM]; /* transparent, strip, translation */
static igmp_mc_vlan_entry_t igmpVlanMap[4096];
static igmp_vlan_translation_entry_t igmpTranslationTable[VLAN_TRANSLATION_ENTRY];
static uint32 port_entry_limit[MAX_PORT_NUM];
static ctc_igmp_control_t igmpCtrl;

static int32               mcast_group_num;
static int32               group_sortedAry_entry_num;
static hw_igmp_group_entry_t  hw_group_db[SYS_MCAST_MAX_DB_NUM];
static hw_igmp_group_entry_t* hw_group_sortedArray[SYS_MCAST_MAX_DB_NUM];
static hw_igmp_group_entry_t* tmp_group_sortedArray[SYS_MCAST_MAX_DB_NUM - 1];
static igmp_group_head_t   igmpGroupFreeHead;

static igmp_aggregate_db_t aggregate_db;;
static igmp_aggregate_entry_t tmp_aggr_array[SYS_MCAST_MAX_DB_NUM - 1];

extern int igmpRedirect_sock;

int mcast_mcSwitch_get(void)
{
	return igmpCtrl.mcMode;
}

/* get MulticastTagOper mode by port */
mcvlan_tag_oper_mode_t mcast_tag_oper_get(uint32 portId)
{
	return igmpOperPerPort[portId];
}

int mcast_mcVlan_map_exist(uint32 portId, uint32 vlanId)
{
	if(IS_LOGIC_PORTMASK_PORTSET(igmpVlanMap[vlanId].portmask, portId)!=0) 
	{
		//SYS_DBG("IGMP portid %d is in mc vlan [%d]\n",portId,vlanId);
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}

int mcast_mcVlan_exist(uint32 vlanId)
{
	if (0 == igmpVlanMap[vlanId].vid) 
	{
		return FALSE;
	}

	return TRUE;
}

int mcast_mcVlan_get(uint32 portId, uint16 *vlanId, uint32 *num)
{
	uint32 j;
	uint8 vlanIdNum = 0;

	if(IsValidLgcLanPort(portId) == FALSE)
	{
		return SYS_ERR_FAILED;
	}
	if(vlanId==NULL || num==NULL)
	{
		return SYS_ERR_FAILED;
	}
	//SYS_DBG("get port [%d] mc vlan table\n",portId);

	for(j=1; j<MAX_VLAN_NUMBER; j++) 
	{
		if(TRUE == mcast_mcVlan_map_exist(portId, j)) 
		{
			*vlanId = j;
	      	vlanId++;
			vlanIdNum++;
		}
	}

	*num = vlanIdNum;	  

	//SYS_DBG("get port [%d] mc vlan table number %d .\n",portId,*num);
	return SYS_ERR_OK;
}

igmp_mc_vlan_entry_t* mcast_mcVlan_entry_get(int vlanId)
{
	return &igmpVlanMap[vlanId];
}

int mcast_translation_get_byMcVlan(uint32 portId, uint32 mcVid, uint32 *userVid)
{
	uint32 i;

	for (i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if (igmpTranslationTable[i].enable == ENABLED)
		{
			if(igmpTranslationTable[i].portId == portId
				&&igmpTranslationTable[i].mcVid == mcVid)
			{ /* match */
				*userVid = igmpTranslationTable[i].userVid;
				return SYS_ERR_OK;
			}
		}
	}

	return SYS_ERR_FAILED;
}

int mcast_translation_get_byUservid(uint32 portId, uint32 userVid, uint32 *mcVid)
{
	uint32 i;

	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++) 
 	{
    	if(igmpTranslationTable[i].enable==ENABLED) 
		{
     		 if(igmpTranslationTable[i].portId == portId
       			 &&igmpTranslationTable[i].userVid == userVid) 
       		{ /* match */
       			*mcVid = igmpTranslationTable[i].mcVid;
       	 		return SYS_ERR_OK;
     		}
    	}
	}

  /* not match */
  return SYS_ERR_FAILED;
}

int mcast_translation_get(uint32 portId, oam_mcast_vlan_translation_entry_t *p_tranlation_entry, uint32 *num)
{
	uint32 i;
	uint32 entryNum;
	oam_mcast_vlan_translation_entry_t *tmp;	

	entryNum = 0;
	tmp = p_tranlation_entry;
	
	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++) 
	{
		if(igmpTranslationTable[i].enable==ENABLED) 
		{
		  if(igmpTranslationTable[i].portId == portId) 
		  {
			tmp->mcast_vlan = igmpTranslationTable[i].mcVid;
			tmp->iptv_vlan = igmpTranslationTable[i].userVid;	
			tmp++;
			entryNum++;
		  }
		}
	}

	*num = entryNum;
	
	return SYS_ERR_OK;
}

igmp_mode_t mcast_mcMode_get()
{
	return igmpCtrl.mcMode;
}

int mcast_mcFastLeave_get()
{
	return igmpCtrl.leaveMode ;
}

mc_control_e mcast_mcCtrl_type_get(uint32 ipVersion)
{
	if(MULTICAST_TYPE_IPV4 == ipVersion)
		return igmpCtrl.ctcControlType;
	else
		return MC_CTRL_GDA_MAC_VID; //TODO set ipv6 type
}

/* update igmp settings by oam ctc start*/
int mcast_db_add(uint32 portId, uint32 vlanId)
{
	SYS_DBG("IGMP add port [%d] to mc vlan %d\n", portId,vlanId);

	if(mcast_mcVlan_map_exist(portId, vlanId) == TRUE) 
	{
		SYS_DBG("IGMP fail to add existing entry: port [%d] to vlan [%d]\n", portId,vlanId);
		return SYS_ERR_OK;
	}

	/* update local database */
	if (0 == igmpVlanMap[vlanId].vid)
	{
		igmpMcVlanNum++;
		igmpVlanMap[vlanId].vid = vlanId;
	}
	
	LOGIC_PORTMASK_SET_PORT(igmpVlanMap[vlanId].portmask, portId);
	
	return SYS_ERR_OK;
}

int mcast_db_del(uint32 portId, uint32 vlanId)
{	
	LOGIC_PORTMASK_CLEAR_PORT(igmpVlanMap[vlanId].portmask, portId);
	SYS_DBG("IGMP delete port [%d] from mc vlan %d..\n",portId,vlanId);
	
	if (IS_LOGIC_PORTMASK_CLEAR(igmpVlanMap[vlanId].portmask))
	{
		if (0 < igmpMcVlanNum)
		{
		    igmpMcVlanNum--;
		}
		igmpVlanMap[vlanId].vid=0;
		SYS_DBG("IGMP delete empty mc vlan %d\n",vlanId);
	}

	return SYS_ERR_OK;
}

int mcast_mcVlan_clear(uint32 portId)
{
	int j;
	
	if(IsValidLgcLanPort(portId) == FALSE)
  	{
  		return SYS_ERR_FAILED;
  	}

	SYS_DBG("IGMP clear vlan of the port[%d]\n",portId);
	for(j = 1; j < MAX_VLAN_NUMBER; j++) 
	{
        if(mcast_mcVlan_map_exist(portId, j)==TRUE) 
		{
			mcast_db_del(portId, j);			
        }
    }	

	return SYS_ERR_OK;
}

int mcast_tag_oper_set(uint32 portId, mcvlan_tag_oper_mode_t tagOper)
{	
	igmpOperPerPort[portId] = tagOper;
	SYS_DBG("IGMP set port [%d] mc vlan tag mode %d ok\n",portId,tagOper);
	
	return SYS_ERR_OK;
}

int mcast_translation_add(uint32 portId, uint32 mcVid, uint32 userVid)
{
	int i, selected = -1;

	if(mcast_tag_oper_get(portId) != TAG_OPER_MODE_TRANSLATION) 
	{   
		return SYS_ERR_FAILED;
	}
	
	SYS_DBG("IGMP set port[%d] mc translation vlan table %d:%d\n", portId, mcVid, userVid);
	
	for (i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if (ENABLED == igmpTranslationTable[i].enable)
		{ 
			if (igmpTranslationTable[i].portId == portId &&
				igmpTranslationTable[i].mcVid == mcVid &&
				igmpTranslationTable[i].userVid == userVid) 
			{ 		
				return SYS_ERR_OK;
			}
		}
		else if (-1 == selected)
		{
			selected = i;
		}
	}

	if (-1 != selected)
	{ 
		igmpTranslationTable[selected].enable	= ENABLED;
		igmpTranslationTable[selected].portId	= portId;
		igmpTranslationTable[selected].mcVid	= mcVid;
		igmpTranslationTable[selected].userVid	= userVid;

		return SYS_ERR_OK;
	}

	return SYS_ERR_FAILED;
}

int mcast_translation_clear(uint32 portId)
{
	uint32 i;
	uint32 userVid;

	SYS_DBG("IGMP clear the mc translation vlan table of port[%d]\n", portId);
	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if(igmpTranslationTable[i].enable==ENABLED)
		{
			if(igmpTranslationTable[i].portId == portId)
			{			
				memset(&igmpTranslationTable[i], 0, sizeof(igmp_vlan_translation_entry_t));
			}
		}
	}
	
	return SYS_ERR_OK;
}

int mcast_mcCtrl_group_entry_clear(void)
{	
	epon_igmp_group_entry_clear(MULTICAST_TYPE_IPV4);	
	epon_igmp_group_entry_clear(MULTICAST_TYPE_IPV6);
	return SYS_ERR_OK;
}

int mcast_mcSwitch_set(uint32 mode)
{
	igmpCtrl.mcMode = mode;
	return SYS_ERR_OK;
}

int mcast_mcFastLeave_set(uint32 leaveMode)
{
	igmpCtrl.leaveMode = leaveMode;
	return SYS_ERR_OK;
}

/* MulticastControl Type set*/
int mcast_mcCtrl_type_set(mc_control_e type)
{
	igmpCtrl.ctcControlType = type;
	return SYS_ERR_OK;
}

int mcast_mcCtrl_group_entry_add(uint32 portId, uint32 vlanId, uint8 *mac)
{
	uint32 groupAddr[4] = {0, 0, 0, 0};
	uint16 tmpvlanId[MAX_VLAN_NUMBER];
	uint32 num=0,i=0;
	oam_mcast_vlan_translation_entry_t translation_entry[16];
	
	if(igmpCtrl.mcMode != IGMP_MODE_CTC) 
	{
		SYS_DBG("IGMP not in IGMP_MODE_CTC mode, do nothing");
		return SYS_ERR_FAILED;
	}
	
	groupAddr[0] = *(uint32 *)&mac[2];
	
	if (igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC)
	{
		if(mcast_tag_oper_get(portId) == TAG_OPER_MODE_TRANSLATION)		
		{
			/*Why add mcvlan in translation mode ?*/
			mcast_mcVlan_get(portId,tmpvlanId,&num);			
			for(i=0;i<num;i++)
			{
		    	mcast_add_group_wrapper(portId, MULTICAST_TYPE_IPV4, tmpvlanId[i], mac, groupAddr);
			}
			
			mcast_translation_get(portId, translation_entry, &num);				
			for(i=0;i<num;i++)
			{
		    	mcast_add_group_wrapper(portId, MULTICAST_TYPE_IPV4, translation_entry[i].mcast_vlan, mac, groupAddr);
			}
		}
		else
		{
			mcast_mcVlan_get(portId,tmpvlanId,&num);			
			for(i=0;i<num;i++)
			{
		    	mcast_add_group_wrapper(portId, MULTICAST_TYPE_IPV4, tmpvlanId[i], mac, groupAddr);
			}
		}		
	}
	else if (igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID)
	{
		mcast_add_group_wrapper(portId, MULTICAST_TYPE_IPV4, vlanId, mac, groupAddr);
	}
	else if (igmpCtrl.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)
	{
		mcast_add_group_wrapper(portId, MULTICAST_TYPE_IPV4, vlanId, NULL, groupAddr);
	}
	else
	{
		//Hardware not support MC_CTRL_GDA_GDA_SA_IP and MC_CTRL_GDA_GDA_IP6_VID
		return SYS_ERR_FAILED;
	}
	return SYS_ERR_OK;
}

int mcast_mcCtrl_group_entry_delete(uint32 portId, uint32 vlanId, uint8 *mac)
{
	uint32 groupAddr[4] = {0, 0, 0, 0};
	uint16 tmpvlanId[MAX_VLAN_NUMBER];
	uint32 num=0,i=0;
	oam_mcast_vlan_translation_entry_t translation_entry[16];
	
	if(igmpCtrl.mcMode != IGMP_MODE_CTC) 
	{   
		SYS_DBG("IGMP not in IGMP_MODE_CTC mode, do nothing");
		return SYS_ERR_FAILED;
	}

	groupAddr[0] = *(uint32 *)&mac[2];

	if (igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC)
	{
		if(mcast_tag_oper_get(portId) == TAG_OPER_MODE_TRANSLATION)		
		{
			mcast_mcVlan_get(portId,tmpvlanId,&num);			
			for(i=0;i<num;i++)
			{
		    	mcast_del_group_wrapper(portId, MULTICAST_TYPE_IPV4, tmpvlanId[i], mac, groupAddr);
			}
			
			mcast_translation_get(portId, translation_entry, &num);				
			for(i=0;i<num;i++)
			{
		    	mcast_del_group_wrapper(portId, MULTICAST_TYPE_IPV4, translation_entry[i].mcast_vlan, mac, groupAddr);
			}
		}
		else
		{
			mcast_mcVlan_get(portId,tmpvlanId,&num);			
			for(i=0;i<num;i++)
			{
		    	mcast_del_group_wrapper(portId, MULTICAST_TYPE_IPV4, tmpvlanId[i], mac, groupAddr);
			}
		}
	}
	else if (igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID)
	{
		mcast_del_group_wrapper(portId, MULTICAST_TYPE_IPV4, vlanId, mac, groupAddr);
	}			
	else if (igmpCtrl.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)
	{
		mcast_del_group_wrapper(portId, MULTICAST_TYPE_IPV4, vlanId, NULL, groupAddr);
	}
	else
	{
		//Hardware not support MC_CTRL_GDA_GDA_SA_IP and MC_CTRL_GDA_GDA_IP6_VID
		return SYS_ERR_FAILED;
	}

	return SYS_ERR_OK;
}

/* Group Num Max per port set */
int mcast_mcGrp_maxNum_set(uint32 portId, uint32 num)
{
	int i=0;
	if(IsValidLgcLanPort(portId) == FALSE && portId != 255)
  	{
  		return SYS_ERR_FAILED;
  	}
	if(portId == 255) 
	{ 
	   	FOR_EACH_LAN_PORT(i) 
	   	{		
		 	SYS_DBG("IGMP set port %d grp limit %d\n",i,num);
		 	port_entry_limit[i] = num;
	   	}
	}
	else 
	{	 
	   	SYS_DBG("IGMP set port %d grp limit %d\n",portId,num);
	  	port_entry_limit[portId] = num;
	}
	return SYS_ERR_OK;
}

int mcast_mcGrp_maxNum_get(uint32 portId)
{
	int num=0;
	if(IsValidLgcLanPort(portId) == FALSE)
  	{
  		return SYS_ERR_FAILED;
  	}	
	
	return port_entry_limit[portId];	
}
/* update igmp settings by oam ctc end*/

//hw l2 table mcast entry process start
int32 mcast_hw_l2McastEntry_add(uint16  vid, uint8* mac, uint32 fwdPortMask)
{
	mac_mcast_t stMacMcast;
	int port;
	uint32 uiPPort;
	
	if(mac == NULL)
		return SYS_ERR_FAILED;

	memset(&stMacMcast, 0, sizeof(stMacMcast));
	memcpy(stMacMcast.mac_addr, mac, MAC_ADDR_LEN);
	stMacMcast.tdVid = vid;
#ifdef CONFIG_SFU_APP
	if((TRUE == is9601B()) && (igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID))
		stMacMcast.fidMode = VLAN_FID_IVL;
	else
		stMacMcast.fidMode = VLAN_FID_SVL;
#else
	if(igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID)
		stMacMcast.fidMode = VLAN_FID_IVL;
	else
		stMacMcast.fidMode = VLAN_FID_SVL;
#endif
	FOR_EACH_LAN_PORT(port)
	{
		if(fwdPortMask & (1 << port))
		{			
			uiPPort = PortLogic2PhyID(port);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stMacMcast.port_mask, uiPPort);
		}
	}
	
    SYS_DBG("%s:vid %d mac"MAC_PRINT" fwd[0x%x]\n",__func__, stMacMcast.tdVid, MAC_PRINT_ARG(stMacMcast.mac_addr),fwdPortMask);
	if(RT_ERR_OK == drv_mac_mcast_set_by_mac_and_fid(stMacMcast))
	{
		 return SYS_ERR_OK;
	}
	return SYS_ERR_FAILED;
}


int32 mcast_hw_l2McastEntry_set(uint16  vid, uint8* mac, uint32 fwdPortMask)
{
	//FIXME hw mcast set is same as hw mcast add
	return mcast_hw_l2McastEntry_add(vid, mac, fwdPortMask);
}

int32 mcast_hw_l2McastEntry_del(uint16  vid, uint8* mac, uint32 fwdPortMask)
{   
	mac_mcast_t stMacMcast;
	int  port = 0;
	uint32 uiPPort;  
	
    if(mac == NULL)
		return SYS_ERR_FAILED;
    
 	memset(&stMacMcast, 0, sizeof(stMacMcast));
    memcpy(stMacMcast.mac_addr, mac, MAC_ADDR_LEN);
	stMacMcast.tdVid = vid;
#ifdef CONFIG_SFU_APP
	if((TRUE == is9601B()) && (igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID))
		stMacMcast.fidMode = VLAN_FID_IVL;
	else
		stMacMcast.fidMode = VLAN_FID_SVL;
#else
	if(igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC)
		stMacMcast.fidMode = VLAN_FID_SVL;
	else
		stMacMcast.fidMode = VLAN_FID_IVL;
#endif //end of CONFIG_SFU_APP	
	FOR_EACH_LAN_PORT(port)
	{
		if(fwdPortMask & (1 << port))
		{			
			uiPPort = PortLogic2PhyID(port);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stMacMcast.port_mask, uiPPort);
		}
	}

	SYS_DBG("%s:vid %d mac"MAC_PRINT"\n", __func__, stMacMcast.tdVid,MAC_PRINT_ARG(stMacMcast.mac_addr));
    if (RT_ERR_OK == drv_mc_mac_delete_by_mac_and_fid(stMacMcast))
    {
        return SYS_ERR_OK;
    }

    return SYS_ERR_FAILED;   
}

int32 mcast_hw_ipMcastEntry_add(uint16  vid, uint32 dip, uint32 fwdPortMask)
{
    int32 ret;
	ip_mcast_t stIpMcast;
	int  port = 0;
	uint32 uiPPort;  

	memset(&stIpMcast,0,sizeof(stIpMcast));

	/*9601b ip-mcast-mode uses LOOKUP_ON_DIP_AND_VID
	    for 9607 because 64 entry svlan table don't support IVL mode,
	    ip-mcast-mode uses LOOKUP_ON_DIP_AND_SIP */
	if(TRUE == is9601B())
	{
		stIpMcast.vid = vid;
	}
	stIpMcast.dip = dip;
	stIpMcast.sip = 0;
	
	FOR_EACH_LAN_PORT(port)
	{
		if(fwdPortMask & (1 << port))
		{			
			uiPPort = PortLogic2PhyID(port);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stIpMcast.port_mask, uiPPort);
		}
	}

	if (RT_ERR_OK == drv_ipMcastEntry_add(stIpMcast))
    {
        ret = SYS_ERR_OK;
    }
	else
	{
		ret = SYS_ERR_FAILED; 
	}
	SYS_DBG("%s ret=%d dip="IPADDR_PRINT" vid=%u\n", __func__, ret,
			IPADDR_PRINT_ARG(stIpMcast.dip), stIpMcast.vid);

    return ret;
}

int32 mcast_hw_ipMcastEntry_set(uint16  vid, uint32 dip, uint32 fwdPortMask)
{
	//FIXME hw ip mcast set is same as hw ip mcast add
	return mcast_hw_ipMcastEntry_add(vid, dip, fwdPortMask);
}

int32 mcast_hw_ipMcastEntry_del(uint16  vid, uint32 dip, uint32 fwdPortMask)
{
    int32 ret;
	ip_mcast_t stIpMcast;
	int  port = 0;
	uint32 uiPPort;  
    
	memset(&stIpMcast,0,sizeof(stIpMcast));

	if(TRUE == is9601B())
	{
		stIpMcast.vid = vid;
	}
	stIpMcast.dip = dip;
	stIpMcast.sip = 0;

	FOR_EACH_LAN_PORT(port)
	{
		if(fwdPortMask & (1 << port))
		{			
			uiPPort = PortLogic2PhyID(port);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stIpMcast.port_mask, uiPPort);
		}
	}
	
    if (RT_ERR_OK == drv_ipMcastEntry_del(stIpMcast))
    {
        ret = SYS_ERR_OK;
    }
	else
	{
		ret = SYS_ERR_FAILED;
	}
	
	SYS_DBG("%s ret=%d dip="IPADDR_PRINT" vid=%u\n", __func__, ret,
			IPADDR_PRINT_ARG(stIpMcast.dip), stIpMcast.vid);
    return ret;   
}

int32 mcast_hw_ipv6McastEntry_add(uint16  vid, uint32* dip, uint32 fwdPortMask)
{
	int32  ret = SYS_ERR_OK;
	uint8  mac[MAC_ADDR_LEN];
	
	mac[0] = 0x33;
    mac[1] = 0x33;
    mac[2] = (uint8)((dip[3] >> 24) & 0xff);
    mac[3] = (uint8)((dip[3] >> 16) & 0xff);
    mac[4] = (uint8)((dip[3] >> 8) & 0xff);
    mac[5] = (uint8)(dip[3] & 0xff);
	
	if(TRUE == is9601B())
	{
		ret = mcast_hw_l2McastEntry_add(vid, mac, fwdPortMask);
		return ret;
	}
	else
	{
#if defined(CONFIG_RTL9600_SERIES) || defined(CONFIG_RTL9607C_SERIES)
		/* 9607 only support l2 mac entry for ipv6 */	
		mac_mcast_t stMacMcast;
		int port;
		uint32 uiPPort;
		
		memset(&stMacMcast, 0, sizeof(stMacMcast));
		memcpy(stMacMcast.mac_addr, mac, MAC_ADDR_LEN);
		stMacMcast.tdVid = vid;

		/* FIXME: for 9607 default value of fidMode is SVL and we don't change fidMode value */
		stMacMcast.fidMode = VLAN_FID_SVL;

		FOR_EACH_LAN_PORT(port)
		{
			if(fwdPortMask & (1 << port))
			{			
				uiPPort = PortLogic2PhyID(port);
				if (INVALID_PORT != uiPPort)
					RTK_PORTMASK_PORT_SET(stMacMcast.port_mask, uiPPort);
			}
		}
		
	    SYS_DBG("%s:vid %d mac"MAC_PRINT" fwd[0x%x]\n",__func__, stMacMcast.tdVid, MAC_PRINT_ARG(stMacMcast.mac_addr),fwdPortMask);
		if(RT_ERR_OK == drv_mac_mcast_set_by_mac_and_fid(stMacMcast))
		{
			 return SYS_ERR_OK;
		}
		return SYS_ERR_FAILED;
#elif defined(CONFIG_RTL9602C_SERIES)
		ip_mcast_t stIpMcast;
		int  port = 0;
		uint32 uiPPort;  
		uint8 * ip;
		
		memset(&stIpMcast,0,sizeof(stIpMcast));

		stIpMcast.isIPv6 = 1;
		stIpMcast.dipv6[0] = dip[0];
		stIpMcast.dipv6[1] = dip[1];
		stIpMcast.dipv6[2] = dip[2];
		stIpMcast.dipv6[3] = dip[3];
		
		FOR_EACH_LAN_PORT(port)
		{
			if(fwdPortMask & (1 << port))
			{			
				uiPPort = PortLogic2PhyID(port);
				if (INVALID_PORT != uiPPort)
					RTK_PORTMASK_PORT_SET(stIpMcast.port_mask, uiPPort);
			}
		}

		if (RT_ERR_OK == drv_ipv6McastEntry_add(stIpMcast))
	    {
	        ret = SYS_ERR_OK;
	    }
		else
		{
			ret = SYS_ERR_FAILED; 
		}
		
		ip = (uint8 *)stIpMcast.dipv6;
		SYS_DBG("%s ret=%d dipv6="IPADDRV6_PRINT" vid=%u\n", __func__, ret,
				IPADDRV6_PRINT_ARG(ip), stIpMcast.vid); 

    	return ret;
#endif
	}
	return ret;
}

int32 mcast_hw_ipv6McastEntry_set(uint16  vid, uint32* dip, uint32 fwdPortMask)
{
	//FIXME hw ip mcast set is same as hw ip mcast add
	return mcast_hw_ipv6McastEntry_add(vid, dip, fwdPortMask);
}

int32 mcast_hw_ipv6McastEntry_del(uint16  vid, uint32* dip, uint32 fwdPortMask)
{
	int32  ret = SYS_ERR_OK;
	uint8	mac[MAC_ADDR_LEN];
	
	mac[0] = 0x33;
	mac[1] = 0x33;
	mac[2] = (uint8)((dip[3] >> 24) & 0xff);
	mac[3] = (uint8)((dip[3] >> 16) & 0xff);
	mac[4] = (uint8)((dip[3] >> 8) & 0xff);
	mac[5] = (uint8)(dip[3] & 0xff);

	if(TRUE == is9601B())
	{
		ret = mcast_hw_l2McastEntry_del(vid, mac, fwdPortMask);
		return ret;
	}
	else
	{
#if defined(CONFIG_RTL9600_SERIES) || defined(CONFIG_RTL9607C_SERIES)
		/* 9607 series only support l2 mac entry for ipv6 */				
		mac_mcast_t stMacMcast;
		int  port = 0;
		uint32 uiPPort;  
					
		memset(&stMacMcast, 0, sizeof(stMacMcast));
		memcpy(stMacMcast.mac_addr, mac, MAC_ADDR_LEN);
		stMacMcast.tdVid = vid;

		/* FIXME: for 9607 default value of fidMode is SVL and we don't change fidMode value */
		stMacMcast.fidMode = VLAN_FID_SVL;

		FOR_EACH_LAN_PORT(port)
		{
			if(fwdPortMask & (1 << port))
			{			
				uiPPort = PortLogic2PhyID(port);
				if (INVALID_PORT != uiPPort)
					RTK_PORTMASK_PORT_SET(stMacMcast.port_mask, uiPPort);
			}
		}
					
		SYS_DBG("%s:vid %d mac"MAC_PRINT"\n", __func__, stMacMcast.tdVid,MAC_PRINT_ARG(stMacMcast.mac_addr));
		if (RT_ERR_OK == drv_mc_mac_delete_by_mac_and_fid(stMacMcast))
		{
			return SYS_ERR_OK;
		}
					
		return SYS_ERR_FAILED;	
#elif defined(CONFIG_RTL9602C_SERIES)
		ip_mcast_t stIpMcast;
		int  port = 0;
		uint32 uiPPort;  
    	uint8 * ip;
		
		memset(&stIpMcast,0,sizeof(stIpMcast));
	
		stIpMcast.isIPv6 = 1;
		stIpMcast.dipv6[0] = dip[0];
		stIpMcast.dipv6[1] = dip[1];
		stIpMcast.dipv6[2] = dip[2];
		stIpMcast.dipv6[3] = dip[3];

		FOR_EACH_LAN_PORT(port)
		{
			if(fwdPortMask & (1 << port))
			{			
				uiPPort = PortLogic2PhyID(port);
				if (INVALID_PORT != uiPPort)
					RTK_PORTMASK_PORT_SET(stIpMcast.port_mask, uiPPort);
			}
		}
	
	    if (RT_ERR_OK == drv_ipv6McastEntry_del(stIpMcast))
	    {
	        ret = SYS_ERR_OK;
	    }
		else
		{
			ret = SYS_ERR_FAILED;
		}
		
		ip = (uint8 *)stIpMcast.dipv6;
		SYS_DBG("%s ret=%d dipv6="IPADDRV6_PRINT" vid=%u\n", __func__, ret,
				IPADDRV6_PRINT_ARG(ip), stIpMcast.vid); 
		return ret;
#endif	
	}
	return ret;
}

int32 mcast_group_sortedArray_search(uint64 search, uint16 *idx, hw_igmp_group_entry_t **groupHead)
{
    int low = 0;
    int mid;
    int high = group_sortedAry_entry_num - 1;

    while (low <= high)
    {
        mid = (low + high) / 2;

        if (hw_group_sortedArray[mid] == NULL)   /* this case occurs when sorted array is empty */
        {
            *groupHead = NULL;
            *idx = mid;
            return SYS_ERR_OK;
        }

        if (hw_group_sortedArray[mid]->sortKey == search)
        {
            *groupHead = hw_group_sortedArray[mid];
            *idx = mid;
            return SYS_ERR_OK;
        }
        else if (hw_group_sortedArray[mid]->sortKey > search)
        {
            high = mid - 1;
        }
        else if (hw_group_sortedArray[mid]->sortKey < search)
        {
            low = mid + 1;
        }
    }

    *groupHead = NULL;
    *idx = low;
    return SYS_ERR_OK;

}

uint64 mcast_group_sortKey_ret(uint32 ipVersion, uint16  vid, uint32 * groupAddr)
{
    uint64  ret = 0;

    if(MULTICAST_TYPE_IPV4 == ipVersion)
    {
        ret = ((uint64)((groupAddr[0]) & 0xffffffff) << 12)  | ((uint64)(vid & 0xfff));
    }
    else
    {
    	uint8 * addr = (uint8 *)groupAddr;
        ret = (((uint64)(groupAddr[3]) & 0xffffffff) << 12)  | (vid & 0xfff);
        ret = ((uint64)((addr[1]) & 0xff) << 44) | ret;
        ret = ((uint64)0x01 << 52) | ret;
    }
   return ret;
}

hw_igmp_group_entry_t* mcast_hw_group_freeEntry_get()
{
	 hw_igmp_group_entry_t *entry;
 
	 if (igmpGroupFreeHead.freeListNumber == 0)
	 {
		 return NULL;
	 }
 
	 if (mcast_group_num >= SYS_MCAST_MAX_GROUP_NUM)
		 return NULL;

	 entry = igmpGroupFreeHead.item;
	 if (NULL == entry)
		 return NULL;
	 
	 igmpGroupFreeHead.item = igmpGroupFreeHead.item->next_free;
	 igmpGroupFreeHead.freeListNumber--;
	 mcast_group_num++;
	 return entry;
}

int32 mcast_hw_group_sortedArray_ins(uint16 sortedIdx, hw_igmp_group_entry_t *entry)
{
    if (entry == NULL)
        return SYS_ERR_FAILED;
    if (sortedIdx > group_sortedAry_entry_num)
    {
        SYS_DBG("Error: sortedIdx:%d > group_sortedAry_entry_num:%d  \n", sortedIdx, group_sortedAry_entry_num);
        return SYS_ERR_FAILED;
    }

    /* Move data */
    if (group_sortedAry_entry_num == sortedIdx)
    {
        hw_group_sortedArray[sortedIdx] = entry;
    }
    else
    {
        memcpy(tmp_group_sortedArray, hw_group_sortedArray + sortedIdx, sizeof(hw_igmp_group_entry_t*) * (group_sortedAry_entry_num - sortedIdx));
        hw_group_sortedArray[sortedIdx] = entry;
        memcpy(hw_group_sortedArray + sortedIdx + 1, tmp_group_sortedArray, sizeof(hw_igmp_group_entry_t*) * (group_sortedAry_entry_num - sortedIdx));
    }
    group_sortedAry_entry_num++;

	return SYS_ERR_OK;
}

int32 mcast_hw_group_sortedArray_del(uint16 sortedIdx)
{
    if (sortedIdx >= group_sortedAry_entry_num)
    {
        SYS_DBG("Error: sortedIdx:%d >= group_sortedAry_entry_num:%d\n", sortedIdx, group_sortedAry_entry_num);
        return SYS_ERR_FAILED;
    }

    if (group_sortedAry_entry_num - sortedIdx  > 1)
    {
        memcpy(tmp_group_sortedArray, hw_group_sortedArray + sortedIdx + 1, sizeof(hw_igmp_group_entry_t*) * (group_sortedAry_entry_num - sortedIdx - 1));
        memcpy(hw_group_sortedArray + sortedIdx, tmp_group_sortedArray, sizeof(hw_igmp_group_entry_t*) * (group_sortedAry_entry_num - sortedIdx - 1));
    }
    group_sortedAry_entry_num--;
    hw_group_sortedArray[group_sortedAry_entry_num] = NULL;

    return SYS_ERR_OK;
}

int32 mcast_hw_group_entry_release(hw_igmp_group_entry_t *entry)
{
    if (entry == NULL)
    {
        SYS_DBG("Error: return NULL pointer!\n");
        return SYS_ERR_FAILED;
    }

    memset(entry, 0, sizeof(hw_igmp_group_entry_t));
    entry->next_free = igmpGroupFreeHead.item;
    igmpGroupFreeHead.item = entry;
    igmpGroupFreeHead.freeListNumber++;
	mcast_group_num--;
	return SYS_ERR_OK;
}

int32 mcast_hw_group_del(hw_igmp_group_entry_t *pGroup)
{
    uint16          sortedIdx;
    hw_igmp_group_entry_t  *groupHead;
  	uint64 key;

	key = mcast_group_sortKey_ret(pGroup->ipVersion,pGroup->vid, pGroup->groupAddr);
    mcast_group_sortedArray_search(key, &sortedIdx, &groupHead);   /* Only compare dip */
    if (groupHead)
    {
        if(igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC ||
		   igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID)
        {
            if (SYS_ERR_OK == mcast_hw_group_sortedArray_del(sortedIdx))
            {
                mcast_hw_group_entry_release(groupHead);
                return SYS_ERR_OK;
            }
        }
        else
        {
			//TODO support dip+sip mode, now only support dip mode
			if (SYS_ERR_OK == mcast_hw_group_sortedArray_del(sortedIdx))
            {
                mcast_hw_group_entry_release(groupHead);
                return SYS_ERR_OK;
            }
        }
    }

    return SYS_ERR_FAILED;
}

int32 mcast_aggregate_array_search(uint64 search, uint16 *idx, igmp_aggregate_entry_t **entry)
{
    int low = 0;
    int mid;
    int high = aggregate_db.entry_num - 1;
    igmp_aggregate_entry_t *sortedArray = aggregate_db.aggregate_entry;

    if (idx == NULL || entry == NULL)
        return SYS_ERR_FAILED;

    while (low <= high)
    {
        mid = (low + high) / 2;

        if (sortedArray[mid].sortKey == 0)   /* this case occurs when sorted array is empty */
        {
            *entry = NULL;
            *idx = mid;
            return SYS_ERR_OK;
        }

        if (sortedArray[mid].sortKey == search)
        {
            *entry = &sortedArray[mid];
            *idx = mid;
            return SYS_ERR_OK;
        }
        else if (sortedArray[mid].sortKey > search)
        {
            high = mid - 1;
        }
        else if (sortedArray[mid].sortKey < search)
        {
            low = mid + 1;
        }
    }

    *entry = NULL;
    *idx = low;
    return SYS_ERR_OK;
}

uint64 mcast_aggregate_sortKey_ret(uint32 ipVersion, uint16  vid, uint32 * groupAddr)
{
    uint64  ret = 0;
    uint32 groupIp = 0;
        
    if(MULTICAST_TYPE_IPV4 == ipVersion)
    {
        groupIp =  (groupAddr[0] & 0x7fffff) | 0x5e000000;
        ret = ((uint64)(groupIp & 0xffffffff) << 12) | ((uint64)(vid & 0xfff));
        ret = ((uint64)0x0100 << 44)  | ret;
    }
    else
    {
        ret = ((uint64)(groupAddr[3] & 0xffffffff) << 12) | ((uint64)(vid & 0xfff));
        ret = ((uint64)0x3333 << 44) | ret;
        ret = ((uint64)0x01 << 60) | ret;
    }

   	return ret; 
}

int32 mcast_aggregate_array_ins(uint16 sortedIdx, igmp_aggregate_entry_t *entry)
{
    if (entry == NULL)
        return SYS_ERR_FAILED;
    if (sortedIdx > aggregate_db.entry_num)
    {
        SYS_DBG("Error: sortedIdx > aggregate_db.entry_num\n");
        return SYS_ERR_FAILED;
    }
    if (aggregate_db.entry_num >= SYS_MCAST_MAX_GROUP_NUM)
    {
        return SYS_ERR_FAILED;
    }

    
    /* Move data */
    if (sortedIdx ==  aggregate_db.entry_num)
    {
        memcpy(aggregate_db.aggregate_entry + sortedIdx, entry, sizeof(igmp_aggregate_entry_t));
    }
    else
    {
        memcpy(tmp_aggr_array, aggregate_db.aggregate_entry + sortedIdx, sizeof(igmp_aggregate_entry_t) * (aggregate_db.entry_num - sortedIdx));
        memcpy(aggregate_db.aggregate_entry + sortedIdx, entry, sizeof(igmp_aggregate_entry_t));
        memcpy(aggregate_db.aggregate_entry + sortedIdx + 1, tmp_aggr_array, sizeof(igmp_aggregate_entry_t) * (aggregate_db.entry_num - sortedIdx));
    }
    aggregate_db.entry_num++;
      
    return SYS_ERR_OK;
}

int32 mcast_aggregate_db_add(hw_igmp_group_entry_t *pGroup, int port)
{
    uint16                  sortedIdx;
    igmp_aggregate_entry_t  *entry = NULL;
    igmp_aggregate_entry_t  newEntry;
    uint64 sortKey;
    
    sortKey = mcast_aggregate_sortKey_ret(pGroup->ipVersion, pGroup->vid, pGroup->groupAddr);
    if (sortKey == 0)
        return SYS_ERR_FAILED;
	
    mcast_aggregate_array_search(sortKey, &sortedIdx, &entry);
    if (NULL == entry)
    {
        memset(&newEntry, 0, sizeof(igmp_aggregate_entry_t));
        newEntry.sortKey = sortKey;
        newEntry.port_ref_cnt[port] = 1;
        newEntry.group_ref_cnt = 1;
        return mcast_aggregate_array_ins(sortedIdx, &newEntry);
    }

    return SYS_ERR_FAILED;
}

int32 mcast_aggregate_db_get(hw_igmp_group_entry_t *pGroup, igmp_aggregate_entry_t **ppEntry)
{
    uint16  sortedIdx;

    mcast_aggregate_array_search(mcast_aggregate_sortKey_ret(pGroup->ipVersion, pGroup->vid, pGroup->groupAddr), &sortedIdx, ppEntry);

    return SYS_ERR_OK;
}

int32 mcast_aggregate_array_remove(uint16 sortedIdx)
{    
    if (sortedIdx >= aggregate_db.entry_num)
    {
        SYS_PRINTF("Error: sortedIdx >= aggregate_db.entry_num\n");
        return SYS_ERR_FAILED;
    }

    if (aggregate_db.entry_num - sortedIdx > 1)
    {
        memcpy(tmp_aggr_array, aggregate_db.aggregate_entry + sortedIdx + 1, sizeof(igmp_aggregate_entry_t) * (aggregate_db.entry_num - sortedIdx - 1));
        memcpy(aggregate_db.aggregate_entry + sortedIdx, tmp_aggr_array, sizeof(igmp_aggregate_entry_t) * (aggregate_db.entry_num - sortedIdx - 1));
    }
    aggregate_db.entry_num--;
    memset(aggregate_db.aggregate_entry + aggregate_db.entry_num, 0, sizeof(igmp_aggregate_entry_t));

    return SYS_ERR_OK;
}

int32 mcast_aggregate_db_del(hw_igmp_group_entry_t *pGroup)
{
    uint16          sortedIdx;
    igmp_aggregate_entry_t   *entry;
    uint64 sortKey;

    if(pGroup == NULL)
		return SYS_ERR_FAILED;
	
    sortKey = mcast_aggregate_sortKey_ret(pGroup->ipVersion, pGroup->vid, pGroup->groupAddr);
    mcast_aggregate_array_search(sortKey, &sortedIdx, &entry);
    if(entry)
    {
        return mcast_aggregate_array_remove(sortedIdx);
    }

    return SYS_ERR_FAILED;
}

#ifdef IGMP_CHECK_UNICAST_VID
/* check if vid belongs to the unicast vlan setting of the port and get the new unicast vid if needed */
static int32 mcast_check_unicast_vlan(uint16 vid, uint32 lport, uint16 * newVid)
{
	ctc_wrapper_vlanCfg_t *pstVlanMode;
	int i=0, j=0;
	
	SYS_DBG("%s vid[%d] port[%d]\n", __func__, vid, lport);
	
	if (!VALID_VLAN_ID(vid))
	{
		return FALSE;
	}
	
	if (RT_ERR_OK != ctc_sfu_vlanCfg_get(lport, &pstVlanMode))
    {        
    	printf("fail to get  the port %d vlan mode",lport);
        return FALSE;
    }

	if(newVid)
		*newVid = vid;
	
	if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
	{
		for (i = 0; i < pstVlanMode->cfg.transCfg.num; i++)
		{
			if (vid == pstVlanMode->cfg.transCfg.transVlanPair[i].oriVlan.vid)
			{
				if(newVid)
					*newVid = pstVlanMode->cfg.transCfg.transVlanPair[i].newVlan.vid;
				return TRUE;
			}
		}
	}
	else if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
	{
		/* in unicast tag mode, should drop upstream igmp packet having vlan tag */
		return FALSE;		
	}
	else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
	{		
		for(i = 0 ; i < pstVlanMode->cfg.aggreCfg.tableNum ; i++)
		{		
			for (j = 0; j < pstVlanMode->cfg.aggreCfg.aggrTbl[i].entryNum; j++)
			{
				if (vid == pstVlanMode->cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].vid)
				{
					if(newVid)
						*newVid = pstVlanMode->cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid;
					return TRUE;
				}
			}
		}
	}
	else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
	{
		for (i = 0; i< pstVlanMode->cfg.trunkCfg.num; i++)
		{
			if (vid == pstVlanMode->cfg.trunkCfg.acceptVlan[i].vid)
			{
				return TRUE;
			}
		}
	}
  	else if (CTC_VLAN_MODE_TRANSPARENT == pstVlanMode->vlanMode)
		return TRUE;

    return FALSE;
}

/* get vid based on the unicast vlan setting of the port if igmp packet not having vlan tag */
static int32 mcast_get_unicast_vlan(uint32 lport, uint16 * newVid)
{
	ctc_wrapper_vlanCfg_t *pstVlanMode;
	int i=0, j=0;
	
	SYS_DBG("%s port[%d]\n", __func__, lport);

	if(newVid == NULL)
		return FALSE;
	
	if (RT_ERR_OK != ctc_sfu_vlanCfg_get(lport, &pstVlanMode))
    {        
    	printf("fail to get  the port %d vlan mode",lport);
        return FALSE;
    }
	
	if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
	{		
		*newVid = pstVlanMode->cfg.transCfg.defVlan.vid;
		return TRUE;	
	}
	else if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
	{
		*newVid = pstVlanMode->cfg.tagCfg.vid;		
		return TRUE;		
	}
	else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
	{				
		*newVid = pstVlanMode->cfg.aggreCfg.defVlan.vid;
		return TRUE;				
	}
	else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
	{
		*newVid = pstVlanMode->cfg.trunkCfg.defVlan.vid;		
		return TRUE;		
	}
  	else if (CTC_VLAN_MODE_TRANSPARENT == pstVlanMode->vlanMode)
  	{
		*newVid = 0;
		return TRUE;
  	}
	
    return FALSE;
}

#endif

int32 mcast_hw_port_entry_get(uint32 port, uint32 *pEntryNum)
{
    int32 i;
    hw_igmp_group_entry_t  *pGroup_entry;
    uint32 entryCnt = 0;
    uint32 lastGroup = 0, lastVid = 0;
    uint16 lastIpv6Pre16 = 0;

    SYS_PARAM_CHK(NULL == pEntryNum, SYS_ERR_NULL_POINTER);

    for(i = 0; i < mcast_group_num; i++)
    {
        pGroup_entry = hw_group_sortedArray[i];

        if (pGroup_entry)
        {
            if(pGroup_entry->fwdPortMask & (1 << port))
            {
                entryCnt++;
            }
        }
    }

    *pEntryNum = entryCnt;

    return SYS_ERR_OK;

}

int32 mcast_hw_check_port_entry_num(uint32 port)
{
	uint32 portEntryNum = 0;
	uint16 limitNum = 0;
	
	mcast_hw_port_entry_get(port, &portEntryNum);
	limitNum = mcast_mcGrp_maxNum_get(port);

	if(portEntryNum >= limitNum)
	{
		SYS_DBG("The Port %d group entry is full limit[%d] \n", port, limitNum);
		return SYS_ERR_FAILED;
	}
	return SYS_ERR_OK;
}

int32 mcast_hw_groupMbrPort_add(uint32 ipVersion, uint32 port, uint16  vid, uint32 * groupAddr)
{
	uint16 sortedIdx = 0;
	hw_igmp_group_entry_t   *pEntry = NULL;
	hw_igmp_group_entry_t   *groupHead = NULL;
	igmp_aggregate_entry_t  *pAggrEntry = NULL;
	int32  ret = SYS_ERR_OK;

	SYS_DBG("%s %d " IPADDR_PRINT " port[%d]\n", __func__, vid, IPADDR_PRINT_ARG(groupAddr[0]), port);

	//add Group Num Max per port check
	if(SYS_ERR_OK != mcast_hw_check_port_entry_num(port))
		return SYS_ERR_FAILED;
	
	mcast_group_sortedArray_search(mcast_group_sortKey_ret(ipVersion, vid, groupAddr), &sortedIdx, &groupHead);
	
	if (groupHead)
    {
        if(igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC ||
		   igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID)
        {
            /* Update aggregate database */
            mcast_aggregate_db_get(groupHead, &pAggrEntry);
            if (pAggrEntry == NULL)
            {
                SYS_DBG("%s():%d An existing group which has no aggregate record!\n", __FUNCTION__, __LINE__);
                return SYS_ERR_FAILED;
            }

            if (0 == (groupHead->fwdPortMask & (1 << port)))
            {
                groupHead->fwdPortMask |= (1 << port);               
                if (0 == pAggrEntry->port_ref_cnt[port])
                {
					ret = mcast_hw_l2McastEntry_add(groupHead->vid, groupHead->mac, groupHead->fwdPortMask);
					if (SYS_ERR_OK != ret)
                    {
                    	SYS_DBG("Failed writing to ASIC!  ret:%d\n", ret);
                    }              
                }
                pAggrEntry->port_ref_cnt[port]++;
            }           
        }
        else if(igmpCtrl.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)
        {
            if (0 == (groupHead->fwdPortMask & (1 << port)))
            {
            	groupHead->fwdPortMask |= (1 << port);
				if (MULTICAST_TYPE_IPV4 == ipVersion)
					ret = mcast_hw_ipMcastEntry_set(groupHead->vid, groupHead->groupAddr[0], groupHead->fwdPortMask);
				else
					ret = mcast_hw_ipv6McastEntry_set(groupHead->vid, groupHead->groupAddr, groupHead->fwdPortMask);
				if (SYS_ERR_OK != ret)
                {
                    SYS_DBG("Failed writing to ASIC ipmcast entry!  ret:%d\n", ret);
                } 
            }
        }
		else
		{
			//not support MC_CTRL_GDA_GDA_SA_IP and MC_CTRL_GDA_GDA_IP6_VID
		}
    }
	else
	{
		pEntry = mcast_hw_group_freeEntry_get();
        if (NULL == pEntry)
        {
            SYS_DBG("HW Group database is full!\n");
            
            return SYS_ERR_MCAST_DATABASE_FULL;
        }

		memset(pEntry, 0 , sizeof(hw_igmp_group_entry_t));
		pEntry->ipVersion = ipVersion;
        pEntry->vid = vid;
        pEntry->sortKey = mcast_group_sortKey_ret(ipVersion, vid, groupAddr);
        if (MULTICAST_TYPE_IPV4 == ipVersion)
        {
        	pEntry->groupAddr[0] = groupAddr[0];
			
            pEntry->mac[0] = 0x01;
            pEntry->mac[1] = 0x00;
            pEntry->mac[2] = 0x5e;
            pEntry->mac[3] = (uint8)((groupAddr[0] >> 16) & 0x7f);
            pEntry->mac[4] = (uint8)((groupAddr[0] >> 8) & 0xff);
            pEntry->mac[5] = (uint8)(groupAddr[0] & 0xff);
        }
        else
        {
            pEntry->groupAddr[0] = groupAddr[0];
			pEntry->groupAddr[1] = groupAddr[1];
			pEntry->groupAddr[2] = groupAddr[2];
			pEntry->groupAddr[3] = groupAddr[3];
			
            pEntry->mac[0] = 0x33;
            pEntry->mac[1] = 0x33;
            pEntry->mac[2] = (uint8)((groupAddr[3] >> 24) & 0xff);
            pEntry->mac[3] = (uint8)((groupAddr[3] >> 16) & 0xff);
            pEntry->mac[4] = (uint8)((groupAddr[3] >> 8) & 0xff);
            pEntry->mac[5] = (uint8)(groupAddr[3] & 0xff);
        }
		pEntry->fwdPortMask = (1 << port);
		
		if(igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC ||
		   igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID)
        {
            /* Handle DIP -> MAC aggregation */
            mcast_aggregate_db_get(pEntry, &pAggrEntry);
            if (pAggrEntry == NULL)
            {
				ret = mcast_hw_l2McastEntry_add(pEntry->vid, pEntry->mac, pEntry->fwdPortMask);
				if(SYS_ERR_OK != ret)
				{
					SYS_DBG("Failed writing to ASIC!  ret:%d\n", ret);
				}
			
				if (SYS_ERR_OK != mcast_aggregate_db_add(pEntry, port))
                {
					mcast_hw_group_entry_release(pEntry);
				}
				else
				{
					if (SYS_ERR_OK != mcast_hw_group_sortedArray_ins(sortedIdx, pEntry))
                    {
                        mcast_hw_group_entry_release(pEntry);
                        mcast_aggregate_db_del(pEntry);
                    }
				}
            }
			else
			{
				if(0 == pAggrEntry->port_ref_cnt[port])
                {
                	ret = mcast_hw_l2McastEntry_add(pEntry->vid, pEntry->mac, pEntry->fwdPortMask);
					if(SYS_ERR_OK != ret)
					{
						SYS_DBG("Failed writing to ASIC!  ret:%d\n", ret);
					}
				}
				
				if(SYS_ERR_OK != mcast_hw_group_sortedArray_ins(sortedIdx, pEntry))
                {
                    mcast_hw_group_entry_release(pEntry);
                }
				else
				{
					pAggrEntry->group_ref_cnt++; /* new group entry*/
                	pAggrEntry->port_ref_cnt[port]++;
				}
			}
        }
        else if(igmpCtrl.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)
        {
			if (MULTICAST_TYPE_IPV4 == ipVersion)
				ret = mcast_hw_ipMcastEntry_add(pEntry->vid, pEntry->groupAddr[0], pEntry->fwdPortMask);
			else
				ret = mcast_hw_ipv6McastEntry_add(pEntry->vid, pEntry->groupAddr, pEntry->fwdPortMask);
			if(SYS_ERR_OK != ret)
			{
				SYS_DBG("Failed writing to ASIC ipmcast entry!  ret:%d\n", ret);
			}
            mcast_hw_group_sortedArray_ins(sortedIdx, pEntry);
        }
		else
		{
			//not support MC_CTRL_GDA_GDA_SA_IP and MC_CTRL_GDA_GDA_IP6_VID
		}
        
	}
}

int32 mcast_hw_groupMbrPort_add_wrapper(uint32 ipVersion, uint32 port, uint16  vid, uint32 * groupAddr)
{
	int userVid = 0;

	if(mcast_tag_oper_get(port) == TAG_OPER_MODE_TRANSLATION)		
	{
		if(SYS_ERR_FAILED == mcast_translation_get_byMcVlan(port, vid, &userVid))
		{
			SYS_DBG("%s not exist port[%d] translation vid[%d] \n", __func__, port, vid);
		#ifndef IGMP_CHECK_UNICAST_VID
			return SYS_ERR_FAILED;
		#else
			if(FALSE == mcast_check_unicast_vlan(vid, port, NULL))
			{
				SYS_DBG("%s not exist port[%d] unicast vid[%d] \n", __func__, port, vid);
				return SYS_ERR_FAILED;
			}
		#endif
		}
		mcast_hw_groupMbrPort_add(ipVersion, port, vid, groupAddr);
	}
	else
	{
		if(mcast_mcVlan_map_exist(port, vid) != TRUE)
		{
			SYS_DBG("%s not exist port[%d] vid[%d] \n", __func__, port, vid);
		#ifndef IGMP_CHECK_UNICAST_VID
			return SYS_ERR_FAILED;
		#else
			if(FALSE == mcast_check_unicast_vlan(vid, port, NULL))
			{
				SYS_DBG("%s not exist port[%d] unicast vid[%d] \n", __func__, port, vid);
				return SYS_ERR_FAILED;
			}
		#endif
		}	
		mcast_hw_groupMbrPort_add(ipVersion, port, vid, groupAddr);
	}

	return SYS_ERR_OK;
}

int32 mcast_hw_mcst_mbr_remove(hw_igmp_group_entry_t *pGroup, uint32 fwdPortMask)
{
    int32 ret;
    igmp_aggregate_entry_t *pAggrEntry = NULL;
	int port;
	uint32 uiPPort;
	
    if(igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC ||
	   igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID)
    {
		mac_address_t mac_addr;
		mac_mcast_t stMacMcast;
		
		mcast_aggregate_db_get(pGroup, &pAggrEntry);
       	if (pAggrEntry == NULL)
       	{
            return SYS_ERR_FAILED;
       	}
		
     	FOR_EACH_LAN_PORT(port)
		{			
			if(fwdPortMask & (1 << port))
			{
		        pAggrEntry->port_ref_cnt[port]--;
		        if ( 0 != pAggrEntry->port_ref_cnt[port])
		        {
		            fwdPortMask &= ~(1 << port);
		        }
			}
      	}

        if(fwdPortMask == 0)
        	return SYS_ERR_OK;

		memset(&stMacMcast, 0, sizeof(stMacMcast));
	    memcpy(stMacMcast.mac_addr, pGroup->mac, MAC_ADDR_LEN);
		stMacMcast.tdVid = pGroup->vid;

#ifdef CONFIG_SFU_APP	
	if((TRUE == is9601B()) && (igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID))
		stMacMcast.fidMode = VLAN_FID_IVL;
	else
		stMacMcast.fidMode = VLAN_FID_SVL;
#else
	if(igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC)
		stMacMcast.fidMode = VLAN_FID_SVL;
	else
		stMacMcast.fidMode = VLAN_FID_IVL;
#endif
		FOR_EACH_LAN_PORT(port)
		{
			if(fwdPortMask & (1 << port))
			{			
				uiPPort = PortLogic2PhyID(port);
				if (INVALID_PORT != uiPPort)
					RTK_PORTMASK_PORT_SET(stMacMcast.port_mask, uiPPort);
			}
		}
		
		ret = drv_mc_mac_delete_by_mac_and_fid(stMacMcast);
		
    }
    else if(igmpCtrl.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)
    {
		if (MULTICAST_TYPE_IPV4 == pGroup->ipVersion)
		{
	        ip_mcast_t stIpMcast;  

			memset(&stIpMcast,0,sizeof(stIpMcast));

			stIpMcast.vid = pGroup->vid;
			stIpMcast.dip = pGroup->groupAddr[0];
			stIpMcast.sip = 0;
#ifdef CONFIG_SFU_APP
			if(SYS_ERR_OK == drv_ipMcastEntry_get(&stIpMcast))
			{
				FOR_EACH_LAN_PORT(port)
				{			
					if(fwdPortMask & (1 << port))
					{
						uiPPort = PortLogic2PhyID(port);				
						if (INVALID_PORT != uiPPort)
							RTK_PORTMASK_PORT_CLEAR(stIpMcast.port_mask, uiPPort);
					}
				}

				ret = drv_ipMcastEntry_add(stIpMcast);
			}
#else
			stIpMcast.port_mask.bits[0] = fwdPortMask;
			ret = drv_ipMcastEntry_del(stIpMcast);
#endif
		}
		else
		{
		/* No need to consider 9601b because it only has one lan port */
#if defined(CONFIG_RTL9600_SERIES)
			mac_address_t mac_addr;
			mac_mcast_t stMacMcast;
		
			memset(&stMacMcast, 0, sizeof(stMacMcast));
		    memcpy(stMacMcast.mac_addr, pGroup->mac, MAC_ADDR_LEN);
			stMacMcast.tdVid = pGroup->vid;

			/* FIXME: for 9607 default value of fidMode is SVL and we don't change fidMode value */
			stMacMcast.fidMode = VLAN_FID_SVL;

			FOR_EACH_LAN_PORT(port)
			{
				if(fwdPortMask & (1 << port))
				{			
					uiPPort = PortLogic2PhyID(port);
					if (INVALID_PORT != uiPPort)
						RTK_PORTMASK_PORT_SET(stMacMcast.port_mask, uiPPort);
				}
			}
			
			ret = drv_mc_mac_delete_by_mac_and_fid(stMacMcast);	
#elif defined(CONFIG_RTL9602C_SERIES)
			ip_mcast_t stIpMcast;  

			memset(&stIpMcast,0,sizeof(stIpMcast));
			stIpMcast.isIPv6 = 1;
			stIpMcast.dipv6[0] = pGroup->groupAddr[0];
			stIpMcast.dipv6[1] = pGroup->groupAddr[1];
			stIpMcast.dipv6[2] = pGroup->groupAddr[2];
			stIpMcast.dipv6[3] = pGroup->groupAddr[3];
			stIpMcast.port_mask.bits[0] = fwdPortMask;
			ret = drv_ipv6McastEntry_del(stIpMcast);
#endif
		}
    }
	else
	{
		//not support MC_CTRL_GDA_GDA_SA_IP and MC_CTRL_GDA_GDA_IP6_VID
	}

    return ret;
}

int32 mcast_hw_groupMbrPort_del(uint32 ipVersion, uint32 port, uint16  vid, uint32 * groupAddr)
{
	uint16 sortedIdx = 0;
	hw_igmp_group_entry_t   *groupHead = NULL;
	igmp_aggregate_entry_t  *pAggrEntry = NULL;
	int32  ret = SYS_ERR_OK;
	
	SYS_DBG("%s %d " IPADDR_PRINT " port[%d]\n", __func__, vid, IPADDR_PRINT_ARG(groupAddr[0]), port);

	if(igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC ||
	   igmpCtrl.ctcControlType == MC_CTRL_GDA_MAC_VID)
    {
    	mcast_group_sortedArray_search(mcast_group_sortKey_ret(ipVersion, vid, groupAddr), &sortedIdx, &groupHead);   /* Only compare dip */
        if (groupHead)
        {                       
            mcast_aggregate_db_get(groupHead, &pAggrEntry);
            if (pAggrEntry == NULL)
            {
                SYS_DBG("%s():%d An existing group which has no aggregate record!\n", __FUNCTION__, __LINE__);
                return SYS_ERR_FAILED;
            }
                
            if (groupHead->fwdPortMask & (1 << port))
            {            
                groupHead->fwdPortMask &= ~(1 << port);
                
                if (groupHead->fwdPortMask == 0)
                {
                   	pAggrEntry->group_ref_cnt--;
					
                    /* Aggregation DB */
                    if (pAggrEntry->group_ref_cnt == 0)
                    {
						ret = mcast_hw_l2McastEntry_del(groupHead->vid, groupHead->mac, (1 << port));
						if (SYS_ERR_OK != ret)
	                    {
	                    	SYS_DBG("Deleting Mcst enrty failed!  ret:%d\n", ret);
	                    }  
						
                        mcast_aggregate_db_del(groupHead);
                    }
                    else
                    {
                        mcast_hw_mcst_mbr_remove(groupHead, (1 << port));
                    }
					
                   	mcast_hw_group_del(groupHead);              
                }
                else
                {
                    mcast_hw_mcst_mbr_remove(groupHead, (1 << port));
                }
                
            }
        }
	}
	else if(igmpCtrl.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)
	{
		mcast_group_sortedArray_search(mcast_group_sortKey_ret(ipVersion, vid, groupAddr), &sortedIdx, &groupHead);   /* Only compare dip */
        if (groupHead)
        {
        	if (groupHead->fwdPortMask & (1 << port))
            {            
                groupHead->fwdPortMask &= ~(1 << port);
                
                if (groupHead->fwdPortMask == 0)
                {
					if (MULTICAST_TYPE_IPV4 == ipVersion)
						ret = mcast_hw_ipMcastEntry_del(groupHead->vid, groupHead->groupAddr[0], (1 << port));
					else
						ret = mcast_hw_ipv6McastEntry_del(groupHead->vid, groupHead->groupAddr, (1 << port));
					if (SYS_ERR_OK != ret)
	                {
	                    SYS_DBG("Deleting ip mcast enrty failed!  ret:%d\n", ret);
	                }
					mcast_hw_group_del(groupHead);
                }
				else
				{
					mcast_hw_mcst_mbr_remove(groupHead, (1 << port));
				}
        	}
        }
	}
	else
	{
		//not support MC_CTRL_GDA_GDA_SA_IP and MC_CTRL_GDA_GDA_IP6_VID
	}
}

int32 mcast_hw_groupMbrPort_del_wrapper(uint32 ipVersion, uint32 port, uint16  vid, uint32 * groupAddr)
{
	int userVid = 0;
	
	if(mcast_tag_oper_get(port) == TAG_OPER_MODE_TRANSLATION)		
	{
		if(SYS_ERR_FAILED == mcast_translation_get_byMcVlan(port, vid, &userVid))
		{
			SYS_DBG("%s not exist port[%d] translation vid[%d] \n", __func__, port, vid);
		#ifndef IGMP_CHECK_UNICAST_VID
			return SYS_ERR_FAILED;
		#else
			if(FALSE == mcast_check_unicast_vlan(vid, port, NULL))
			{
				SYS_DBG("%s not exist port[%d] unicast vid[%d] \n", __func__, port, vid);
				return SYS_ERR_FAILED;
			}
		#endif
		}
		mcast_hw_groupMbrPort_del(ipVersion, port, vid, groupAddr);
	}
	else
	{
		if(mcast_mcVlan_map_exist(port, vid) != TRUE)
		{
			SYS_DBG("%s not exist port[%d] vid[%d] \n", __func__, port, vid);
		#ifndef IGMP_CHECK_UNICAST_VID
			return SYS_ERR_FAILED;
		#else
			if(FALSE == mcast_check_unicast_vlan(vid, port, NULL))
			{
				SYS_DBG("%s not exist port[%d] unicast vid[%d] \n", __func__, port, vid);
				return SYS_ERR_FAILED;
			}
		#endif
		}	
		mcast_hw_groupMbrPort_del(ipVersion, port, vid, groupAddr);
	}

	return SYS_ERR_OK;
}

int32 mcast_del_port_from_vlan(uint16 vid, uint32 port)
{
	epon_igmp_groupMbrPort_delFromVlan(MULTICAST_TYPE_IPV4, vid, port);
	return SYS_ERR_OK;
}

int32 mcast_ctc_add_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, 
										uint8* mac, uint32 *groupAddr)
{
	uint16 tmpvlanId[MAX_VLAN_NUMBER];
	uint32 num=0,i=0;
	oam_mcast_vlan_translation_entry_t translation_entry[16];
	
	if(ismvlan(vid) == TRUE || vid == 0)
	{
		if(mcast_tag_oper_get(port) != TAG_OPER_MODE_TRANSLATION)		
		{
			mcast_mcVlan_get(port,tmpvlanId,&num);			
			for(i=0;i<num;i++)
			{
			    mcast_add_group_wrapper(port, ipVersion, tmpvlanId[i], mac, groupAddr);
			}
		}
		else
		{
			mcast_translation_get(port, translation_entry, &num);	
			for(i=0;i<num;i++)
			{
			    mcast_add_group_wrapper(port, ipVersion, translation_entry[i].mcast_vlan, mac, groupAddr);
			}
		}
	}
	else
	{
		if(mcast_tag_oper_get(port) != TAG_OPER_MODE_TRANSLATION)		
		{
			mcast_add_group_wrapper(port, ipVersion, vid, mac, groupAddr);
		}
		else
		{
			int mvid = 0;
			if(SYS_ERR_FAILED == mcast_translation_get_byUservid(port, vid, &mvid))
			{
				SYS_DBG("%s not exist port[%d] translation uservid[%d] \n", __func__, port, vid);
			#ifndef IGMP_CHECK_UNICAST_VID
				return SYS_ERR_FAILED;
			#else
				mvid = vid;
			#endif
			}
			mcast_add_group_wrapper(port, ipVersion, mvid, mac, groupAddr);
		}
	}
	
	return SYS_ERR_OK;
}

int32 mcast_ctc_del_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, 
										uint8* mac, uint32 *groupAddr)
{
	uint16 tmpvlanId[MAX_VLAN_NUMBER];
	uint32 num=0,i=0;
	oam_mcast_vlan_translation_entry_t translation_entry[16];
	
	if(ismvlan(vid) == TRUE || vid == 0)
	{
		if(mcast_tag_oper_get(port) != TAG_OPER_MODE_TRANSLATION)		
		{
			mcast_mcVlan_get(port,tmpvlanId,&num);			
			for(i=0;i<num;i++)
			{
			    mcast_del_group_wrapper(port, ipVersion, tmpvlanId[i], mac, groupAddr);
			}
		}
		else
		{
			mcast_translation_get(port, translation_entry, &num);	
			for(i=0;i<num;i++)
			{
			    mcast_del_group_wrapper(port, ipVersion, translation_entry[i].mcast_vlan, mac, groupAddr);
			}
		}
	}
	else
	{
		if(mcast_tag_oper_get(port) != TAG_OPER_MODE_TRANSLATION)		
		{
			mcast_del_group_wrapper(port, ipVersion, vid, mac, groupAddr);
		}
		else
		{
			int mvid = 0;
			if(SYS_ERR_FAILED == mcast_translation_get_byUservid(port, vid, &mvid))
			{
				SYS_DBG("%s not exist port[%d] translation uservid[%d] \n", __func__, port, vid);
			#ifndef IGMP_CHECK_UNICAST_VID
				return SYS_ERR_FAILED;
			#else
				mvid = vid;
			#endif
			}
			mcast_del_group_wrapper(port, ipVersion, mvid, mac, groupAddr);
		}
	}

	return SYS_ERR_OK;
}

int32 mcast_ctc_update_igmpv3_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, uint8* mac, 
													uint32 *groupAddr, igmpv3_mode_t igmpv3_mode, 
													uint32 * srcList, uint16 srcNum)
{
	uint16 tmpvlanId[MAX_VLAN_NUMBER];
	uint32 num=0,i=0;
	oam_mcast_vlan_translation_entry_t translation_entry[16];
	
	if(IGMP_MODE_CTC == mcast_mcMode_get())
	{
		if((igmpv3_mode == CHANGE_TO_INCLUDE_MODE) && (srcNum == 0))
		{
			/*ToInclude {} means to leave group*/
			mcast_ctc_del_group_wrapper(port, ipVersion, vid, mac, groupAddr);
		}
		return SYS_ERR_OK;
	}

	if(ismvlan(vid) == TRUE || vid == 0)
	{
		if(mcast_tag_oper_get(port) != TAG_OPER_MODE_TRANSLATION)		
		{
			mcast_mcVlan_get(port,tmpvlanId,&num);			
			for(i=0;i<num;i++)
			{
			    mcast_update_igmpv3_group_wrapper(port,ipVersion,tmpvlanId[i],mac,groupAddr,igmpv3_mode,srcList,srcNum);
			}
		}
		else
		{
			mcast_translation_get(port, translation_entry, &num);	
			for(i=0;i<num;i++)
			{
			    mcast_update_igmpv3_group_wrapper(port,ipVersion,translation_entry[i].mcast_vlan,mac,groupAddr,igmpv3_mode,srcList,srcNum);
			}
		}
	}
	else
	{
		if(mcast_tag_oper_get(port) != TAG_OPER_MODE_TRANSLATION)		
		{
			mcast_update_igmpv3_group_wrapper(port,ipVersion,vid,mac,groupAddr,igmpv3_mode,srcList,srcNum);
		}
		else
		{
			int mvid = 0;
			if(SYS_ERR_FAILED == mcast_translation_get_byUservid(port, vid, &mvid))
			{
				SYS_DBG("%s not exist port[%d] translation uservid[%d] \n", __func__, port, vid);
				return SYS_ERR_FAILED;
			}
			mcast_update_igmpv3_group_wrapper(port, ipVersion, mvid, mac, groupAddr,igmpv3_mode,srcList,srcNum);
		}

	}

	return SYS_ERR_OK;
}
//hw l2 table mcast  entry process end

//tx related start
static int32 l2_pkt_tx_send(uint8 *pkt_payload, uint32 pkt_length, uint8 portmask)
{
	unsigned char *pReplyPtr, *pCurr;

    pReplyPtr = (unsigned char *)malloc(sizeof(unsigned char) * pkt_length + EPON_TX_OAM_LLID_LEN + FWD_PORT_MASK_LEN);

    if(NULL == pReplyPtr)
    {
        return SYS_ERR_FAILED;
    }

	SYS_DBG("tx packet to portMask[0x%x]\n", portmask);

#ifdef IGMP_TEST_MODE
	portmask |= (1 << 0); //dump to port 0
#endif
	
    pCurr = pReplyPtr;
    /* Payload */
    memcpy(pCurr, pkt_payload, pkt_length);
    pCurr += pkt_length;

    /* Fill in the LLID index at the end of all payload */
    *pCurr = DEFAULT_LLID_IDX;
    pCurr += EPON_TX_OAM_LLID_LEN;
	*pCurr = ((unsigned char)portmask);
	pCurr += FWD_PORT_MASK_LEN;
	
    ptk_redirect_userApp_sendPkt(
        igmpRedirect_sock,
        PR_KERNEL_UID_IGMPMLD,
        0,
        pCurr - pReplyPtr,
        pReplyPtr);

	free(pReplyPtr);
    return SYS_ERR_OK;
}

int32 drv_l2_packet_send(uint8 *pMsg, uint32 uiLen, l2_send_op_t *pOp)
{
	uint32 uiPPort;
    uint32  uiLPort;
    uint8 data[MAX_FRAME_LEN]; 
    int portmask = 0;
    
    if((NULL == pMsg) || (0 == uiLen) || (MAX_FRAME_LEN < uiLen) || (NULL == pOp))
    {
        return SYS_ERR_FAILED;
    }
	
	memcpy(data, pMsg, uiLen);
	
    if((PhyMaskNotNull(pOp->portmask)) || (RTK_PORTMASK_IS_PORT_SET(pOp->portmask, HAL_GET_PON_PORT())))
	{
        /*The logic port mask begins with bit 0.*/
        FOR_EACH_LAN_PORT(uiLPort)    
        {
        	uiPPort = PortLogic2PhyID(uiLPort);
        	if(RTK_PORTMASK_IS_PORT_SET(pOp->portmask, uiPPort))
				portmask |= (1 << uiPPort);
        }
		if(RTK_PORTMASK_IS_PORT_SET(pOp->portmask, HAL_GET_PON_PORT()))
			portmask |= (1 << HAL_GET_PON_PORT());

        /*send frame*/
        if (l2_pkt_tx_send(data, uiLen,portmask))
        {
            return SYS_ERR_FAILED;
        }			        
    }
    else
	{
        /*send frame*/
        if (l2_pkt_tx_send(data, uiLen, 0xF))
        {
            return SYS_ERR_FAILED;
        }
    }
	
    return SYS_ERR_OK;    
}

/* flood upstream untag packet 
  * translation mode: add ctag using mcast_vlan of translation table entry
  * transparent or strip mode : add ctag using mvid of mcvlan table entry
  */
int32 mcast_upstream_flood_by_portVlan(packet_info_t * pktInfo, uint32 length)
{
	uint16 tmpvlanId[MAX_VLAN_NUMBER];
	int num=0;
	int i=0;
	uint32 source_port;
	uint16 vid;
	l2_send_op_t l2_info;
	uint32 uiPPort;
	uint8 * ptr;
	mcvlan_tag_oper_mode_t port_oper;
	oam_mcast_vlan_translation_entry_t translation_entry[16];

	source_port = pktInfo->rx_tag.source_port;
	port_oper = mcast_tag_oper_get(source_port);

	if(port_oper == TAG_OPER_MODE_TRANSLATION)
	{
		mcast_translation_get(source_port, translation_entry, &num);
	}
	else
	{
		mcast_mcVlan_get(source_port, tmpvlanId, &num);
	}

	if(num == 0)
		return SYS_ERR_FAILED;
	
	memset(&l2_info, 0, sizeof(l2_info));

	/* add cvlan tag header */
	ptr = pktInfo->data;
	memmove(&(ptr[16]), &(ptr[12]), pktInfo->length-12);
	pktInfo->length += 4;
	length += 4;
	ptr[12] = 0x81;
	ptr[13] = 0x00;

	for(i=0;i<num;i++)
	{
		if(port_oper == TAG_OPER_MODE_TRANSLATION)
		{
			vid = translation_entry[i].mcast_vlan;
		}
		else
		{
			vid = tmpvlanId[i];
		}
		SYS_DBG("flood packet using vid[%d] to Pon\n", vid);
		/* set vid and clear user priority, cfi */
		ptr[14] = (vid >> 8) & 0x0F; 
		ptr[15] = vid & 0xFF;
		
		uiPPort = PortLogic2PhyID(LOGIC_PON_PORT);
		RTK_PORTMASK_RESET(l2_info.portmask);
		RTK_PORTMASK_PORT_SET(l2_info.portmask, uiPPort);
		drv_l2_packet_send(pktInfo->data, length, &l2_info);
	}
}

/* recevied packet don't have svid tag now, so don't consider*/
int32 mcast_snooping_tx(packet_info_t * pktInfo, uint16 vid, uint32 length, 
								sys_logic_portmask_t fwdPortMask)
{
	int snoopingmode;
	mcvlan_tag_oper_mode_t port_oper;   
	int   mvid = 0;
	int   userVid = 0;
	int   sendPkt = 0;
	uint32 port = 0;
	l2_send_op_t l2_info;
	uint32 uiPPort;
	
    memset(&l2_info, 0, sizeof(l2_info));	

	IGMP_SEM_LOCK(igmpPktSem);

	snoopingmode = mcast_mcSwitch_get();

	if(IS_LOGIC_PORTMASK_PORTSET(fwdPortMask, PHYSICAL_PON_PORT))
	{
		//upstream igmp report or leave packet
		uint32 source_port;
		source_port = pktInfo->rx_tag.source_port;
		port_oper = mcast_tag_oper_get(source_port);
		
		if(snoopingmode == IGMP_MODE_SNOOPING)
		{			
			if(port_oper == TAG_OPER_MODE_TRANSLATION)
			{
				/* untag packet: send packets for each mcvlan belong to this source port
				*   tagged packet: check translation table and drop if unmatched
				*/
				if(TRUE == pktInfo->rx_tag.cvid_tagged)
				{
					if(SYS_ERR_OK == mcast_translation_get_byUservid(source_port, vid, &mvid))
					{
						/* modify user vid to mcast vid */
						pktInfo->data[14] &= 0xF0;
						pktInfo->data[14] |= (uint8)((mvid>> 8) & 0xF) ;
						pktInfo->data[15] = mvid & 0xFF;
						sendPkt = 1;
					}
				#ifdef IGMP_CHECK_UNICAST_VID
					else
					{
						uint16 newVid;
						if(mcast_check_unicast_vlan(vid, source_port, &newVid))
						{
							SYS_DBG("%s vid[%d] to newVid[%d]\n", __func__, vid, newVid);
							/* modify vid to corresponding unicast vid */							
							pktInfo->data[14] &= 0xF0;
							pktInfo->data[14] |= (uint8)((newVid>> 8) & 0xF) ;
							pktInfo->data[15] = newVid & 0xFF;
							sendPkt = 1;
						}
					}
				#endif
				}
				else
				{
				#ifdef IGMP_CHECK_UNICAST_VID
					uint16 newVid;
					if(mcast_get_unicast_vlan(source_port, &newVid))
					{
						SYS_DBG("%s untag igmp add vid[%d]\n", __func__, newVid);
						if(newVid > 0)
						{
							memmove(&(pktInfo->data[16]), &(pktInfo->data[12]), pktInfo->length-12);
							pktInfo->length += 4;
							length += 4;
							pktInfo->data[12] = 0x81;
							pktInfo->data[13] = 0x00;
							pktInfo->data[14] = (newVid >> 8) & 0x0F ;
							pktInfo->data[15] = newVid & 0xFF;
						}
						sendPkt = 1;
					}
				#else
					mcast_upstream_flood_by_portVlan(pktInfo,length);
				#endif
				}
			}
			else
			{
				/* untag packet: send packets for each mcvlan belong to this source port
				*   tagged packet: check mcvlan table and drop if vid is unmatched
				*/
				if(TRUE == pktInfo->rx_tag.cvid_tagged)
				{
					if(mcast_mcVlan_map_exist(source_port, vid) == TRUE)
					{
						//do nothing
						sendPkt = 1;
					}
				#ifdef IGMP_CHECK_UNICAST_VID
					else
					{
						uint16 newVid;
						if(mcast_check_unicast_vlan(vid, source_port, &newVid))
						{
							SYS_DBG("%s vid[%d] to newVid[%d]\n", __func__, vid, newVid);
							/* modify vid to corresponding unicast vid */
							pktInfo->data[14] &= 0xF0;
							pktInfo->data[14] |= (uint8)((newVid>> 8) & 0xF) ;
							pktInfo->data[15] = newVid & 0xFF;
							sendPkt = 1;
						}
					}
				#endif
				}			
				else
				{
				#ifdef IGMP_CHECK_UNICAST_VID
					uint16 newVid;
					if(mcast_get_unicast_vlan(source_port, &newVid))
					{
						SYS_DBG("%s untag igmp add vid[%d]\n", __func__, newVid);
						if(newVid > 0)
						{
							memmove(&(pktInfo->data[16]), &(pktInfo->data[12]), pktInfo->length-12);
							pktInfo->length += 4;
							length += 4;
							pktInfo->data[12] = 0x81;
							pktInfo->data[13] = 0x00;
							pktInfo->data[14] = (newVid >> 8) & 0x0F ;
							pktInfo->data[15] = newVid & 0xFF;
						}
						sendPkt = 1;
					}
				#else
					mcast_upstream_flood_by_portVlan(pktInfo,length);
				#endif
				}
			}	
		}
		else
		{
			/* ctc mode mvid = source port index +1, e.g. mvid of port 0 is 1 */
			mvid = pktInfo->rx_tag.source_port + 1;
			
			if(TRUE == pktInfo->rx_tag.cvid_tagged)
			{
				pktInfo->data[14] = (mvid >> 8) & 0x0F;
				pktInfo->data[15] = mvid & 0xFF;
			}
			else
			{
				memmove(&(pktInfo->data[16]),&(pktInfo->data[12]),pktInfo->length-12);
				pktInfo->data[12] = 0x81;
				pktInfo->data[13] = 0x00;				
				pktInfo->data[14] = (mvid>> 8) & 0x0F;
				pktInfo->data[15] = mvid & 0xFF;
				pktInfo->length += 4;
				length += 4;
			}
			sendPkt = 1;
		}		

		if(sendPkt)
		{
			uiPPort = PortLogic2PhyID(LOGIC_PON_PORT);
			if(uiPPort != INVALID_PORT)
			{
				RTK_PORTMASK_RESET(l2_info.portmask);
				RTK_PORTMASK_PORT_SET(l2_info.portmask, uiPPort);
				drv_l2_packet_send(pktInfo->data, length, &l2_info);
			}
		}
	}
	else
	{
		//downstream packet
		// Is same process for ctc mode and igmp snooping mode ?
		unsigned char data[512];
		
		FOR_EACH_LAN_PORT(port)
		{			
			if(IS_LOGIC_PORTMASK_PORTSET(fwdPortMask, port))
			{
				/* siyuan 2016-07-25: make a copy of the query packet for each port to fix the problem that 
				   it would strip the vlan header of igmp query packet twice if two lan ports are untag mode */
				memcpy(data, pktInfo->data, pktInfo->length);
				length = pktInfo->length;
				
				sendPkt = 0;
				port_oper = mcast_tag_oper_get(port);
				
				if(port_oper == TAG_OPER_MODE_TRANSLATION)
				{
					/* untag packet: simply drop
					*   tagged packet: check translation table and drop if vid is unmatched
					*/
					if ((TRUE == pktInfo->rx_tag.cvid_tagged) || (vid > 0))
				 	{
				 		/*if find it ,replace the vlan to uservid, else the vlan has been translated by aisc*/
						if(SYS_ERR_OK == mcast_translation_get_byMcVlan(port, vid, &userVid))
						{
							SYS_DBG("%s to lan port[%d] portMode[%d] vid[%d]\n", __func__, port, port_oper, vid);
							data[14] &= 0xF0;
							data[14] |= (uint8)((userVid>> 8) & 0xF);
							data[15] = userVid & 0xFF;
							sendPkt = 1;
						}
						else if(SYS_ERR_OK == mcast_translation_get_byUservid(port, vid, &mvid))
						{
							/*FIXME: the vlan has been translated by aisc ? */
							sendPkt = 1;
						}					
					}				
				}
				else
				{
					/* untag packet: simply drop
					*   tagged packet: check mcvlan table and drop if vid is unmatched
					*/
					if ((TRUE == pktInfo->rx_tag.cvid_tagged) || (vid > 0))
				 	{
				 		if(mcast_mcVlan_map_exist(port, vid) == TRUE)
						{
							SYS_DBG("%s to lan port[%d] portMode[%d] vid[%d]\n", __func__, port, port_oper, vid);
							if(port_oper == TAG_OPER_MODE_STRIP)
							{
								memmove(&(data[12]), &(data[16]), length-16);
								length -= 4;
							}
							sendPkt = 1;
				 		}
				 	}
				}

				if (pktInfo->forced) sendPkt = 1;
				
			#ifndef IGMP_TEST_MODE
				if(sendPkt)
			#endif
				{
					uiPPort = PortLogic2PhyID(port);
					if(uiPPort != INVALID_PORT)
					{
						RTK_PORTMASK_RESET(l2_info.portmask);
						RTK_PORTMASK_PORT_SET(l2_info.portmask, uiPPort);
						drv_l2_packet_send(data, length, &l2_info);
					}
				}
			}
		}
	}     

	IGMP_SEM_UNLOCK(igmpPktSem);
	
    return SYS_ERR_OK;
}

int32 mcast_snooping_tx_wrapper(packet_info_t * pktInfo, uint16 vid, uint32 length, sys_logic_portmask_t fwdPortMask)
{
	return mcast_snooping_tx(pktInfo, vid, length, fwdPortMask);
}

int32 mcast_flood_tx_wrapper(packet_info_t * pktInfo, uint16 vid, uint32 length)
{
	sys_logic_portmask_t txPmsk;
	
	LOGIC_PORTMASK_SET_ALL(txPmsk);

#ifndef IGMP_TEST_MODE
	//remove source port
	LOGIC_PORTMASK_CLEAR_PORT(txPmsk, pktInfo->rx_tag.source_port);
#endif

	mcast_snooping_tx(pktInfo, vid, length, txPmsk);
}
//tx related end

/* below is init and debug code */
void mcast_igmp_hw_show()
{
    int32 i;
    hw_igmp_group_entry_t  *groupHead;
	igmp_aggregate_entry_t *pAggrEntry;
	uint32 dip;
	uint16 vid;
	
	printf("\n------  igmp ipv4 mcast hw setting ------\n");
	printf("Vlan    GroupMac        GroupIp    PortMask\n");
    for(i = 0; i < mcast_group_num; i++)
    {    	
        groupHead = hw_group_sortedArray[i];
		if(groupHead && groupHead->ipVersion == MULTICAST_TYPE_IPV4)
		{
			printf("%-4d " MAC_PRINT "   "IPADDR_PRINT"   0x%x\n", groupHead->vid,
				MAC_PRINT_ARG(groupHead->mac), IPADDR_PRINT_ARG(groupHead->groupAddr[0]), groupHead->fwdPortMask);
		}
    }

#ifdef SUPPORT_MLD_SNOOPING
	printf("\n------  igmp ipv6 mcast hw setting ------\n");
	printf("Vlan    GroupMac               GroupIp             PortMask\n");
    for(i = 0; i < mcast_group_num; i++)
    {    	
		uint8  * ip;
		groupHead = hw_group_sortedArray[i];
		if(groupHead && groupHead->ipVersion == MULTICAST_TYPE_IPV6)
		{
			ip = (uint8 *)groupHead->groupAddr;
			printf("%-4d " MAC_PRINT "   "IPADDRV6_PRINT"   0x%x\n", groupHead->vid,
				MAC_PRINT_ARG(groupHead->mac), IPADDRV6_PRINT_ARG(ip), groupHead->fwdPortMask);
		}
    }
#endif

	printf("\n------    igmp aggregate db setting ------\n");
	printf("Vlan   AggreDip   GroupCnt  Port0Cnt\n");
    for(i = 0; i < aggregate_db.entry_num; i++)
    {    	
        pAggrEntry = &aggregate_db.aggregate_entry[i];
		if(pAggrEntry)
		{
			vid = (uint16)(pAggrEntry->sortKey & 0xfff);
			dip = (uint32)((pAggrEntry->sortKey >> 12) & 0xffffffff);
			printf("%-4d   " IPADDR_PRINT"      %-4d      %-4d\n", vid,
				IPADDR_PRINT_ARG(dip), pAggrEntry->group_ref_cnt, pAggrEntry->port_ref_cnt[0]);
		}
    }
}

void mcast_igmp_setting_show()
{
	int i,port;
	int portMask;
	char * controlType[5] = {"DGA_MAC","DGA_MAC_VID", "DGA_SIP", "DGA_IP_VID","DGA_IP6_VID"};
	char * operMode[3] = {"Transparent", "Strip", "Translation"};
	uint32 num=0;
	oam_mcast_vlan_translation_entry_t translation_entry[16];
	
	mcast_igmp_hw_show();

	printf("\nmcMode = %s\n", (igmpCtrl.mcMode == IGMP_MODE_SNOOPING) ? "IGMP Snooping": "CTC mode");
	printf("mcControlType = %s\n", controlType[igmpCtrl.ctcControlType]);
	printf("FastLeave = %s\n", (igmpCtrl.leaveMode == IGMP_LEAVE_MODE_FAST) ? "Enable":"Disable"); 

	printf("------vlan port setting------\n");
	printf("Vlan  PortMask   \n");
	for(i = 1; i < MAX_VLAN_NUMBER; i++) 
	{
		if(igmpVlanMap[i].vid > 0)
		{
			portMask = 0;
			FOR_EACH_LAN_PORT(port)
			{
				if(mcast_mcVlan_map_exist(port, i)==TRUE) 
				{
					portMask |= (1 << port);	
		        }
			}
			printf("%-4d   0x%x\n", igmpVlanMap[i].vid, portMask);
		}
    }

	printf("-----translation table-----\n");
	printf("Port  McVlan  UserVlan\n");
	FOR_EACH_LAN_PORT(port)
	{
		mcast_translation_get(port, translation_entry, &num);				
		for(i=0;i<num;i++)
		{
		    printf("%-4d    %-4d     %-4d\n", port, translation_entry[i].mcast_vlan, 
					translation_entry[i].iptv_vlan);
		}
	}
	
	printf("------  port setting  ------\n");
	printf("Port  MAXGroup   OperMode\n");
	FOR_EACH_LAN_PORT(port)
	{
		printf("%-4d    %-3d    %s\n", port, port_entry_limit[port],
				operMode[igmpOperPerPort[port]]);
	}
}

int32 mcast_aggregate_db_init(void)
{
    memset(&aggregate_db, 0, sizeof(aggregate_db));
    return SYS_ERR_OK;
}

int32 mcast_hw_group_db_init(void)
{
    int i;
    hw_igmp_group_entry_t  *iter;

    for (i = 0; i < SYS_MCAST_MAX_GROUP_NUM; i++)
    {
        hw_group_sortedArray[i] = NULL;
        memset(&hw_group_db[i], 0, sizeof(hw_igmp_group_entry_t));
    }

    igmpGroupFreeHead.freeListNumber = SYS_MCAST_MAX_GROUP_NUM;
    iter = igmpGroupFreeHead.item = &hw_group_db[0];
    for (i = 1; i < SYS_MCAST_MAX_GROUP_NUM; iter = iter->next_free, i++)
    {
        iter->next_free = &hw_group_db[i];
    }
    iter->next_free = NULL;

    group_sortedAry_entry_num = 0;
	mcast_group_num = 0;
	
    return SYS_ERR_OK;
}

void mcast_ctc_db_init()
{
#ifdef CONFIG_HGU_APP
	rtk_rg_initParams_t init_param;
#endif
	IGMP_SEM_INIT(igmpPktSem);
	
	memset(igmpVlanMap,0,sizeof(igmpVlanMap));
	memset(igmpOperPerPort,0,sizeof(igmpOperPerPort));
	memset(igmpTranslationTable,0,sizeof(igmpTranslationTable));
	memset(&igmpCtrl,0,sizeof(igmpCtrl));
 
  	igmpCtrl.mcMode = IGMP_MODE_SNOOPING;
	igmpCtrl.leaveMode= IGMP_LEAVE_MODE_NON_FAST_LEAVE;
#ifdef CONFIG_HGU_APP
	if( RT_ERR_RG_OK == rtk_rg_initParam_get(&init_param) )
		{
			if (0 == init_param.ivlMulticastSupport)
				igmpCtrl.ctcControlType = MC_CTRL_GDA_GDA_IP_VID;
			else
				igmpCtrl.ctcControlType = MC_CTRL_GDA_MAC;
	
		}
#else
  	igmpCtrl.ctcControlType = MC_CTRL_GDA_MAC;	
#endif

	mcast_hw_group_db_init();
	mcast_aggregate_db_init();
}

