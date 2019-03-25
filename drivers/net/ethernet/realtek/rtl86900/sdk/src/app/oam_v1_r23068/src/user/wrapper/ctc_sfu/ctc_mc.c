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
 * $Revision: 
 * $Date: 
 *
 * Purpose :            
 *
 * Feature :           
 *
 */

/*
 * Include Files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdint.h>

#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <common/util/rt_util.h>

/* EPON OAM include */
#include <epon_oam_config.h>
#include <epon_oam_err.h>
#include <epon_oam_db.h>
#include <epon_oam_dbg.h>
#include <epon_oam_rx.h>

/* User specific include */
#include <user/ctc_oam.h>
#include <user/ctc_oam_var.h>

#include <sys_portview.h>
#include <sys_portmask.h>

#include <ctc_mc.h>
#include <drv_vlan.h>
#include <ctc_wrapper.h>
#include <rtk/l2.h>
#include "epon_oam_msgq.h"

#define RTK_CTC_DPRINTF SYS_PRINTF

#define RETURN_IF_FAIL(condition, retval) do { if(!(condition)){ return (retval); } } while(0)

static int igmpMcVlanNum=0;

static igmp_mc_vlan_entry_t igspVlanMap[4096];

static uint32 igmpTagOperPerPort[MAX_PORT_NUM]; /* transparent, strip, translation */

igmp_vlan_translation_entry_t igmpTagTranslationTable[VLAN_TRANSLATION_ENTRY];

static ctc_igmp_control_t igmpctrlBlock;

unsigned int debug_enable = 0;

mcvlan_tag_oper_mode_t igmpTagOperPerPortGet(uint32 portId)
{
	return igmpTagOperPerPort[portId];
}

int igmpMcTagTranslationGet(uint32 portId, oam_mcast_vlan_translation_entry_t *p_tranlation_entry, uint32 *num)
{
	uint32 i;
	uint32 entryNum;
	oam_mcast_vlan_translation_entry_t *tmp;	

	entryNum = 0;
	tmp = p_tranlation_entry;
	RTK_CTC_DPRINTF("get port [%d] Translation table\n",portId);
	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++) 
	{
		if(igmpTagTranslationTable[i].enable==ENABLED) 
		{
			if(igmpTagTranslationTable[i].portId == portId) 
			{
				tmp->mcast_vlan = igmpTagTranslationTable[i].mcVid;
				tmp->iptv_vlan = igmpTagTranslationTable[i].userVid;	
				tmp++;
				entryNum++;
				RTK_CTC_DPRINTF("Translation item [%d]:mvlan %d,uservid %d\n",entryNum,igmpTagTranslationTable[i].mcVid,igmpTagTranslationTable[i].userVid);
			}
		}
	}

	*num = entryNum;
	
	RTK_CTC_DPRINTF("portid %d ,Translation number %d\n", portId, entryNum);
	
	return SYS_ERR_OK;
}
	
int igspVlanMapExist(uint32 portId, uint32 vlanId)
{
	if(IS_LOGIC_PORTMASK_PORTSET(igspVlanMap[vlanId].portmask, portId)!=0) 
	{
		RTK_CTC_DPRINTF("portid %d is in mc vlan [%d]\n",portId,vlanId);
		return TRUE;
	}
	else 
	{
		//RTK_CTC_DPRINTF("portid %d is not in mc vlan [%d]\n",portId,vlanId);
		return FALSE;
	}
}

int igmpMcVlanGet(uint32 portId, uint8 *vlanId, uint32 *num)
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
	RTK_CTC_DPRINTF("get port [%d] mc vlan table\n",portId);

	for(j=1; j<MAX_VLAN_NUMBER; j++) 
	{
		if(TRUE==igspVlanMapExist(portId, j)) 
		{
			*vlanId++ = (j>>8)&0xFF;
			*vlanId++ = j&0xFF;
			vlanIdNum++;
		}
	}

	*num = vlanIdNum;	  

	RTK_CTC_DPRINTF("get port [%d] mc vlan table number %d .\n",portId,*num);
	return SYS_ERR_OK;
}

static int _igmpMcVlanDel(uint32 portId, uint32 vlanId)
{
	int ret=0;
	uint32 uiPPort;
	rtk_portmask_t stLMask;
	
	uiPPort = PortLogic2PhyID(portId);
	if (INVALID_PORT == uiPPort)
		return SYS_ERR_FAILED;
	
	LOGIC_PORTMASK_CLEAR_PORT(igspVlanMap[vlanId].portmask, portId);
	RTK_CTC_DPRINTF("delete port [%d] from mc vlan %d..\n",portId,vlanId);
	
	mcast_del_port_from_vlan(vlanId, portId);
	RTK_CTC_DPRINTF("delete port [%d] from mc vlan %d\n",portId,vlanId);
	
	if (IS_LOGIC_PORTMASK_CLEAR(igspVlanMap[vlanId].portmask))
	{
		if (0 < igmpMcVlanNum)
		{
		    igmpMcVlanNum--;
		}
		igspVlanMap[vlanId].vid=0;		
		mcast_vlan_querier_del(vlanId);
		RTK_CTC_DPRINTF("delete empty mc vlan %d\n",vlanId);
	}

	switch(igmpTagOperPerPortGet(portId)) 
	{
		case TAG_OPER_MODE_TRANSPARENT:     
		case TAG_OPER_MODE_STRIP: 
			RTK_PORTMASK_RESET(stLMask);
			RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
			ret = drv_mc_vlan_member_remove(vlanId, stLMask);
			if(ret!=RT_ERR_OK)
			{
				return SYS_ERR_FAILED;
			}
			break;
		case TAG_OPER_MODE_TRANSLATION:
			/* do nothing for translation mode */
			break;
		default:
			break;
	}
	return SYS_ERR_OK;
}

int igmpMcVlanClear(uint32 portId)
{
	int j=0;
	int ret=0;
	
	if(IsValidLgcLanPort(portId) == FALSE)
  	{
  		return SYS_ERR_FAILED;
  	}

	/* clear clf rule for mc vlan */
	ctc_clf_rule_for_mcastvlan_clear(portId);

	RTK_CTC_DPRINTF("clear  the port %d",portId);
	for(j=1; j<MAX_VLAN_NUMBER; j++) 
	{
        if(igspVlanMapExist(portId, j)==TRUE) 
		{
			ret = _igmpMcVlanDel(portId, j);			
        }
    }	

	mcast_mcVlan_clear(portId); /*update igmp setting */
	return SYS_ERR_OK;
}

int mcVlanExist(uint32 vlanId)
{
	if (0 == igspVlanMap[vlanId].vid) 
	{
		return FALSE;
	}

	return TRUE;
}

static int32 _ctc_getportVlanmodecfg(uint32 uiLPortId, ctc_wrapper_vlanCfg_t **pstCtcVlanMode)
{
    if (!IsValidLgcPort(uiLPortId))
        return SYS_ERR_FAILED;

    if (RT_ERR_OK != ctc_sfu_vlanCfg_get(uiLPortId, pstCtcVlanMode))
    {        
    	RTK_CTC_DPRINTF("fail to get  the port %d vlan mode",uiLPortId);
        return SYS_ERR_FAILED;
    }

    return SYS_ERR_OK;
}

/*Begin add by huangmingjian 2013-10-17 for Bug 203,257*/
int drv_check_validvid_for_mc_translation(uint32 vlanId)
{	
	int i = 0;
	
	if (!VALID_VLAN_ID(vlanId))
    {
        return FALSE;
    }
	
	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++) 
	{
		if(ENABLED == igmpTagTranslationTable[i].enable)
		{
			if(vlanId == igmpTagTranslationTable[i].mcVid || vlanId == igmpTagTranslationTable[i].userVid)
			{
				RTK_CTC_DPRINTF("vlan[%d] is same with the mc translation vlan of port[%d]",vlanId,igmpTagTranslationTable[i].portId);
				return FALSE;
			}
		}
	}

	return TRUE;
}

int OamCheckValidVid(uint32 uiMcVid)
{
	uint32 uiLPortIndex;
	ctc_wrapper_vlanCfg_t *pstVlanMode;
	int i=0, j=0;
	
	if (!VALID_VLAN_ID(uiMcVid))
	{
		return FALSE;
	}

	FOR_EACH_LAN_PORT(uiLPortIndex)
	{
		_ctc_getportVlanmodecfg(uiLPortIndex, &pstVlanMode);
		
		if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
		{
		    if (uiMcVid == pstVlanMode->cfg.transCfg.defVlan.vid)
		    {
				RTK_CTC_DPRINTF("vlan[%d] is same with the default vlan of port[%d]",uiMcVid,uiLPortIndex);
				return FALSE;
		    }

		    for (i = 0; i < pstVlanMode->cfg.transCfg.num; i++)
		    {
				if ((uiMcVid == pstVlanMode->cfg.transCfg.transVlanPair[i].oriVlan.vid) ||
				    (uiMcVid == pstVlanMode->cfg.transCfg.transVlanPair[i].newVlan.vid))
				{
					RTK_CTC_DPRINTF("vlan[%d] is same with the translation vlan of port[%d]",uiMcVid,uiLPortIndex);
					return FALSE;
				}
		    }
		}
		else if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
		{
			if (uiMcVid == pstVlanMode->cfg.tagCfg.vid)
			{
				RTK_CTC_DPRINTF("vlan[%d] is same with the tag vlan of port[%d]",uiMcVid,uiLPortIndex);
				return FALSE;
			}
		}
		else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
		{
			if (uiMcVid == pstVlanMode->cfg.aggreCfg.defVlan.vid)
			{
				RTK_CTC_DPRINTF("vlan[%d] is same with the default vlan of port[%d]",uiMcVid,uiLPortIndex);
				return FALSE;
			}
			
			for(i = 0 ; i < pstVlanMode->cfg.aggreCfg.tableNum ; i++)
			{
				if (uiMcVid == pstVlanMode->cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid)
				{
					RTK_CTC_DPRINTF("vlan[%d] is same with the default vlan of port[%d]",uiMcVid,uiLPortIndex);
					return FALSE;
				}
				
				for (j = 0; j < pstVlanMode->cfg.aggreCfg.aggrTbl[i].entryNum; j++)
				{
					if (uiMcVid == pstVlanMode->cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].vid)
					{
						RTK_CTC_DPRINTF("vlan[%d] is same with the aggregation vlan of port[%d]",uiMcVid,uiLPortIndex);
						return FALSE;
					}
				}
			}
		}
		else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
		{
			if (uiMcVid == pstVlanMode->cfg.trunkCfg.defVlan.vid)
			{
				RTK_CTC_DPRINTF("vlan[%d] is same with the default vlan of port[%d]",uiMcVid,uiLPortIndex);
				return FALSE;
			}

			for (i = 0; i< pstVlanMode->cfg.trunkCfg.num; i++)
			{
				if (uiMcVid == pstVlanMode->cfg.trunkCfg.acceptVlan[i].vid)
				{
					RTK_CTC_DPRINTF("vlan[%d] is same with the trunk vlan of port[%d]",uiMcVid,uiLPortIndex);
					return FALSE;
				}
			}
		}
    }
	
    return TRUE;
}

uint32 igmpMcVlanNumGet(void)
{
	RTK_CTC_DPRINTF("mc vlan num : %d\n", igmpMcVlanNum);
	return igmpMcVlanNum;
}

int igmpMcVlanDelete(uint32 portId, uint32 vlanId)
{
	int ret=SYS_ERR_OK;
	
	if(IsValidLgcLanPort(portId)==FALSE)
  	{
  		return SYS_ERR_FAILED;
  	}

	RTK_CTC_DPRINTF("delete the mc vlan of port : %d\n", portId);
	if(igspVlanMapExist(portId, vlanId)) 
	{
		ret=_igmpMcVlanDel(portId,vlanId);
		RETURN_IF_FAIL(ret==SYS_ERR_OK,ret);
	}

	mcast_db_del(portId, vlanId); /*update igmp setting */
	return SYS_ERR_OK;
}

static  int _igmpMcVlanAdd(uint32 portId, uint32 vlanId)
{
	int ret=0;

	RTK_CTC_DPRINTF("add port [%d] to mc vlan %d\n", portId,vlanId);

	if(igspVlanMapExist(portId, vlanId)==TRUE) 
	{
		RTK_CTC_DPRINTF("fail to add port [%d] to vlan [%d]\n", portId,vlanId);
		return SYS_ERR_OK;
	}

	switch(igmpTagOperPerPortGet(portId)) 
	{
		case TAG_OPER_MODE_TRANSPARENT:
			/* add multicast vlan entry */
			ret = ctc_clf_rule_for_McastTransparent_create(portId, vlanId);
			if (RT_ERR_OK != ret)
				return SYS_ERR_FAILED;
			
			ret = drv_addport2mvlan(portId, vlanId, TAG_OPER_MODE_TRANSPARENT);
			if (0 != ret) 
			{         
				return SYS_ERR_FAILED;
			}      
			break;
		case TAG_OPER_MODE_STRIP:
			/* add multicast vlan entry */
			ret = ctc_clf_rule_for_McastStrip_create(portId, vlanId);
			if (RT_ERR_OK != ret)
				return SYS_ERR_FAILED;
			
			ret = drv_addport2mvlan(portId, vlanId, TAG_OPER_MODE_STRIP);
			if (0 != ret) 
			{         
				return SYS_ERR_FAILED;
			}     
			break;
		case TAG_OPER_MODE_TRANSLATION:
			RTK_CTC_DPRINTF("%d translation mode add mcvlan[%d]\n",__func__, vlanId);
			/* do nothing for translation mode */
			break;
		default:
			/* do nothing */
			break;
	}

	/* update local database */
	if (0 == igspVlanMap[vlanId].vid)
	{
		igmpMcVlanNum++;
		igspVlanMap[vlanId].vid = vlanId;
		mcast_vlan_querier_add(vlanId);
	}
	
	LOGIC_PORTMASK_SET_PORT(igspVlanMap[vlanId].portmask,portId);
	
	return SYS_ERR_OK;
}

int igmpMcVlanAdd(uint32 portId, uint32 vlanId)
{
	if(IsValidLgcLanPort(portId) == FALSE)
	{
		return SYS_ERR_FAILED;
	}

	/* siyuan 20160201
	  * CDATA 9601B may add the same mc vlan to ctc vlan, As mc vlan tagStripped
	  * mode will be modified by OLT unexpectedly, so we should save all mc vlan
	  * in local.
	  */
#if 0
	if (FALSE == OamCheckValidVid(vlanId))
	{
		return SYS_ERR_FAILED;
	}
#endif

	RTK_CTC_DPRINTF("portId %d vlanId %d", portId, vlanId);
	
	_igmpMcVlanAdd(portId, vlanId);
	
	mcast_db_add(portId, vlanId); /*update igmp setting */
	return SYS_ERR_OK;
}

int igmpMcTagstripGet(uint32 portId, uint8 *tagOper)
{
	if(IsValidLgcLanPort(portId)==FALSE)
	{
		return SYS_ERR_FAILED;
	}
	
	if(tagOper==NULL)
	{
		return SYS_ERR_FAILED;
	}
	
	*tagOper = igmpTagOperPerPortGet(portId);
	
	return SYS_ERR_OK;
}

int igmpMcTagTranslationTableNum(unsigned int portId)
{
	uint32 i;
	int num=0;
	
	for (i=0; i<VLAN_TRANSLATION_ENTRY; i++) 
	{
		if ((igmpTagTranslationTable[i].enable == ENABLED) &&
			(igmpTagTranslationTable[i].portId == portId))
		{
			num++;	    
		}
	}
	RTK_CTC_DPRINTF("mc translation vlan num [%d]\n", num);
	return num;
}

int igmpMctag_translation_check(unsigned int portId, unsigned int mcVid, unsigned int userVid)
{
	int i=0;

	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++) 
	{
		if (igmpTagTranslationTable[i].enable != ENABLED)
			continue;
		if (igmpTagTranslationTable[i].userVid == userVid)
		{
			if (igmpTagTranslationTable[i].mcVid == mcVid)
			{
				return SYS_ERR_OK;
			}
		}
	}
	
	/* QL 20160112
	 * CDATA 9601B may add the same mc vlan to ctc vlan, As mc vlan tagStripped
	 * mode will be modified by OLT unexpectedly, so we should save all mc vlan
	 * in local.
	 */
#if 0
	if (FALSE == OamCheckValidVid(mcVid))
	{
		RTK_CTC_DPRINTF("svlan [%d] is conflict with the existed vlan\n", mcVid);
		return SYS_ERR_FAILED;
	}
	
	if (FALSE == OamCheckValidVid(userVid))
	{
		RTK_CTC_DPRINTF("cvlan [%d] is conflict with the existed vlan\n", userVid);
		return SYS_ERR_FAILED;
	}
#endif
	
	return SYS_ERR_OK;
}

int igmpTagTranslationTableClear(uint32 portId)
{
	uint32 i;
	uint32 userVid;
	uint32 j=0;
  	int used=0;
	sys_vlanmask_t vlanlist;

	VLANMASK_CLEAR_ALL(vlanlist);
	
	RTK_CTC_DPRINTF("clear the mc translation vlan table of port[%d]\n", portId);
	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if(igmpTagTranslationTable[i].enable==ENABLED)
		{
			if(igmpTagTranslationTable[i].portId == portId)
			{ /* match, should be deleted later */
				userVid = igmpTagTranslationTable[i].userVid;

				VLANMASK_SET_VID(vlanlist,igmpTagTranslationTable[i].userVid);
				memset(&igmpTagTranslationTable[i], 0, sizeof(igmp_vlan_translation_entry_t));

				RTK_CTC_DPRINTF("delete the empty user vlan [%d]\n", userVid);
			}
		}
	}

	for(j=1; j<4096; j++) 
  	{
		used=0;
		for(i=0; i<VLAN_TRANSLATION_ENTRY; i++) 
		{
		    if(igmpTagTranslationTable[i].enable==ENABLED) 
			{
		      if(igmpTagTranslationTable[i].userVid== j) 
			  {      
		      	 used=1;
				 break;
		      }
		    }	
		}
		if(0==used)
	  	{
	  	  	 if(VLANMASK_IS_VIDSET(vlanlist,j))
			 {
		  	  	 RTK_CTC_DPRINTF("delete the empty user vlan [%d]\n", j);
			  	 mcast_vlan_querier_del(j);
	  	 	 }
	    }
  	}
	
	return SYS_ERR_OK;
}

int igmpMcTagTranslationCfClear(uint32 portId)
{
	int ret;
	uint32 i;
	rtk_portmask_t stLMask;
	uint32 uiPPort;

	if(igmpTagOperPerPortGet(portId) != TAG_OPER_MODE_TRANSLATION) 
	{
		return SYS_ERR_FAILED;
	}
	
	uiPPort = PortLogic2PhyID(portId);
	if (INVALID_PORT == uiPPort)
		return SYS_ERR_FAILED;
	
	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if(igmpTagTranslationTable[i].enable==ENABLED)
		{
			if(igmpTagTranslationTable[i].portId == portId) 
			{
				RTK_PORTMASK_RESET(stLMask);
				RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
				/* QL 20160112 uservlan has not been created for 9601B, so dont process it anymore. */

				ret = drv_mc_vlan_member_remove(igmpTagTranslationTable[i].mcVid, stLMask);
				if(ret!=RT_ERR_OK)
				{
					RTK_CTC_DPRINTF("failed to remove translation mcvlan[%d]\n",igmpTagTranslationTable[i].mcVid);
					return SYS_ERR_FAILED;
				}
			}
		}
	}
	
	ret = igmpTagTranslationTableClear(portId);
	RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);

	return SYS_ERR_OK;
}


int igmpMcTagTranslationClear(uint32 portId)
{
	int ret;
	uint32 i;
	rtk_portmask_t stLMask;
	uint32 uiPPort;

	if(igmpTagOperPerPortGet(portId) != TAG_OPER_MODE_TRANSLATION) 
	{
		return SYS_ERR_FAILED;
	}
	
	uiPPort = PortLogic2PhyID(portId);
	if (INVALID_PORT == uiPPort)
		return SYS_ERR_FAILED;
	
	for(i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if(igmpTagTranslationTable[i].enable==ENABLED)
		{
			if(igmpTagTranslationTable[i].portId == portId) 
			{
				ret=mcast_del_port_from_vlan(igmpTagTranslationTable[i].userVid, portId);
				RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);
				
				ret=drv_port_sp2c_entry_delete(portId, igmpTagTranslationTable[i].mcVid);
				if (RT_ERR_OK != ret)
				{
					RTK_CTC_DPRINTF("failed to delete s2c of port [%d]\n",portId);
					return SYS_ERR_FAILED;
				}
				
				ret=drv_port_c2s_entry_delete(portId, igmpTagTranslationTable[i].userVid, igmpTagTranslationTable[i].mcVid);
				if (RT_ERR_OK != ret)
				{
					RTK_CTC_DPRINTF("failed to delete c2s entry of port [%d]\n",portId);
					return SYS_ERR_FAILED;
				}
				
				RTK_PORTMASK_RESET(stLMask);
				RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
				ret = drv_mc_translation_vlan_member_remove(igmpTagTranslationTable[i].userVid, igmpTagTranslationTable[i].mcVid, stLMask);
				if (RT_ERR_OK != ret)
				{
					RTK_CTC_DPRINTF("failed to rmv the vlan member of port [%d]\n",portId);
					return SYS_ERR_FAILED;
				}
			}
		}
	}
	
	ret = igmpTagTranslationTableClear(portId);
	RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);

	return SYS_ERR_OK;
}

int igspTransparentstripModeClear(uint32 portId)
{
	int ret;
	uint32 j;
	uint32 vttIndex;
	rtk_portmask_t stLMask;
	uint32 uiPPort;

	ret = 0;
	vttIndex = 0;

	uiPPort = PortLogic2PhyID(portId);
	if (INVALID_PORT == uiPPort)
		return SYS_ERR_FAILED;
	
	for(j=1; j<MAX_VLAN_NUMBER; j++)
	{
		if(igspVlanMapExist(portId, j))
		{
			RTK_PORTMASK_RESET(stLMask);
			RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
			ret = drv_mc_vlan_member_remove(j, stLMask);
			if (RT_ERR_OK != ret)
			{
				RTK_CTC_DPRINTF("failed to rmv the vlan member of port [%d]\n",portId);
				return SYS_ERR_FAILED;
			}
		}
	}

	return SYS_ERR_OK;
}

int igspVlanStripModeClear(uint32 portId, mcvlan_tag_oper_mode_t tagOper)
{
	int ret;
	ret = 0;

	/* clear clf rule for mc vlan */
	ctc_clf_rule_for_mcastvlan_clear(portId);

	switch(tagOper)
	{
		case TAG_OPER_MODE_TRANSPARENT:
		case TAG_OPER_MODE_STRIP:
			ret = igspTransparentstripModeClear(portId);
			RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);

			break;
		case TAG_OPER_MODE_TRANSLATION:
			if (is9601B() || is9607C() || is9602C())
				ret = igmpMcTagTranslationCfClear(portId);
			else
				ret = igmpMcTagTranslationClear(portId);
			RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);
			
			mcast_translation_clear(portId); /*update igmp setting */
			break;
		default:
			/* do nothing */
			break;
	}

	return SYS_ERR_OK;
}

int32 drv_addport2mvlan(uint32 uiLPort, uint32 uiMcVid, mcvlan_tag_oper_mode_t enTagMode)
{
	rt_error_common_t ulRetVal = RT_ERR_OK;
	rtk_portmask_t stLMask, stUntagLMsk;
	uint32 uiPPort;

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return SYS_ERR_FAILED;
	
	RTK_PORTMASK_RESET(stLMask);
	RTK_PORTMASK_RESET(stUntagLMsk);

	drv_mc_vlan_mem_get(uiMcVid, &stLMask, &stUntagLMsk);
	
	switch (enTagMode)
	{
        case TAG_OPER_MODE_TRANSPARENT:
			if((RTK_PORTMASK_IS_PORT_SET(stLMask, uiPPort)) && (!RTK_PORTMASK_IS_PORT_SET(stUntagLMsk, uiPPort)))
            {
                return SYS_ERR_OK;
            }
            else
            {
				RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
				RTK_PORTMASK_PORT_CLEAR(stUntagLMsk, uiPPort);
            }
			
			break;
		case TAG_OPER_MODE_STRIP:
			if((RTK_PORTMASK_IS_PORT_SET(stLMask, uiPPort)) && (RTK_PORTMASK_IS_PORT_SET(stUntagLMsk, uiPPort)))
			{
				return SYS_ERR_OK;
			}
			else
			{
				RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
				RTK_PORTMASK_PORT_SET(stUntagLMsk, uiPPort);
			}
	
			break;
		case TAG_OPER_MODE_TRANSLATION:
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_RTL9602C_SERIES)
			/* 9602c and 9607c share 4k cvlan and svlan, should set vlan member port untag to untag slvan tag */
			if((RTK_PORTMASK_IS_PORT_SET(stLMask, uiPPort)) && (RTK_PORTMASK_IS_PORT_SET(stUntagLMsk, uiPPort)))
			{
				return SYS_ERR_OK;
			}
			else
			{
				RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
				RTK_PORTMASK_PORT_SET(stUntagLMsk, uiPPort);
			}
#else
			if((RTK_PORTMASK_IS_PORT_SET(stLMask, uiPPort)) && (!RTK_PORTMASK_IS_PORT_SET(stUntagLMsk, uiPPort)))
            {
                return SYS_ERR_OK;
            }
            else
            {
				RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
				RTK_PORTMASK_PORT_CLEAR(stUntagLMsk, uiPPort);
            }
#endif
			break;
		default :			
			return SYS_ERR_FAILED;
			
	}

	if((TRUE == is9601B()) && ((igmpctrlBlock.ctcControlType == MC_CTRL_GDA_MAC_VID)
		|| (igmpctrlBlock.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)))
		ulRetVal = drv_mc_vlan_member_add(uiMcVid, stLMask, stUntagLMsk, VLAN_FID_IVL);
	else
		ulRetVal = drv_mc_vlan_member_add(uiMcVid, stLMask, stUntagLMsk, VLAN_FID_SVL);
	
	if (RT_ERR_OK != ulRetVal)
	{		
		
		return SYS_ERR_FAILED;
	}
	return SYS_ERR_OK;
}

int igspTransparentModeSetAction(uint32 portId)
{
	int ret;
	uint32 vid;

	for(vid=1; vid<MAX_VLAN_NUMBER; vid++) 
	{
		if(igspVlanMapExist(portId, vid))    
		{
			ret = ctc_clf_rule_for_McastTransparent_create(portId, vid);
			if (RT_ERR_OK != ret)
				return SYS_ERR_FAILED;
			
			ret = drv_addport2mvlan(portId, vid, TAG_OPER_MODE_TRANSPARENT);
			if (SYS_ERR_OK != ret)
			{
				return SYS_ERR_FAILED;
			}		
		} 
	}

	return SYS_ERR_OK;
}

int igspStripModeSetAction(uint32 portId)
{
	int ret;
	uint32 vid;

	for(vid=1; vid<MAX_VLAN_NUMBER; vid++) 
	{
		if(igspVlanMapExist(portId, vid)) 
		{
			ret = ctc_clf_rule_for_McastStrip_create(portId, vid);
			if (RT_ERR_OK != ret)
				return SYS_ERR_FAILED;
			
			ret = drv_addport2mvlan(portId, vid, TAG_OPER_MODE_STRIP);
			if (0 != ret)
			{
				return SYS_ERR_FAILED;
			}       
		}
	}

	return SYS_ERR_OK;
}

int igspVlantagModeSetAction(uint32 portId, mcvlan_tag_oper_mode_t tagOper)
{
	int ret;
	ret = 0;

	switch(tagOper)
	{
		case TAG_OPER_MODE_TRANSPARENT:
			ret = igspTransparentModeSetAction(portId);
			RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);
			break;
		case TAG_OPER_MODE_STRIP:
			ret = igspStripModeSetAction(portId);
			RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);
			break;
		case TAG_OPER_MODE_TRANSLATION:
			//implement in igmpMcTagTranslationAdd()
			break;
		default:     
			break;
	}
	return SYS_ERR_OK;
}

int igmpMcTagOperSet(uint32 portId, mcvlan_tag_oper_mode_t tagOper)
{
	int ret=0;
	
	RTK_CTC_DPRINTF("set port [%d] mc vlan tag mode %d\n", portId, tagOper);
	
	igspVlanStripModeClear(portId, igmpTagOperPerPortGet(portId));
	
	igspVlantagModeSetAction(portId,tagOper);
	
	igmpTagOperPerPort[portId] = tagOper;
	RTK_CTC_DPRINTF("set port [%d] mc vlan tag mode %d ok\n",portId,tagOper);
	
	return SYS_ERR_OK;
}

int igmpMcTagstripSet(uint32 portId, uint32 tagOper)
{
	if(IsValidLgcLanPort(portId) == FALSE)
	{
		return SYS_ERR_FAILED;
	}
	
	igmpMcTagOperSet(portId, tagOper);

	mcast_tag_oper_set(portId, tagOper); /*update igmp setting */
	return SYS_ERR_OK;
}

int igmpTagTranslationTableAdd(uint32 portId, uint32 mcVid, uint32 userVid)
{
	uint32 i, selected=-1;

	for (i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if (ENABLED == igmpTagTranslationTable[i].enable)
		{ 
			if (igmpTagTranslationTable[i].portId == portId &&
				igmpTagTranslationTable[i].mcVid == mcVid &&
				igmpTagTranslationTable[i].userVid == userVid) 
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
		igmpTagTranslationTable[selected].enable	= ENABLED;
		igmpTagTranslationTable[selected].portId	= portId;
		igmpTagTranslationTable[selected].mcVid		= mcVid;
		igmpTagTranslationTable[selected].userVid	= userVid;

		return SYS_ERR_OK;
	}

	return SYS_ERR_FAILED;
}

int McTagTranslationCfAdd(uint32 portId, uint32 mcVid, uint32 userVid)
{
	int ret;
	rtk_portmask_t stLMask, stUntagLMsk;
	uint32 uiPPort;
	
	if ((!IsValidLgcPort(portId)) ||
		(!VALID_VLAN_ID(mcVid)) ||
		(!VALID_VLAN_ID(userVid)))
	{
		return SYS_ERR_FAILED;
	}

	uiPPort = PortLogic2PhyID(portId);
	if (INVALID_PORT == uiPPort)
		return SYS_ERR_FAILED;
	
	ret = ctc_clf_rule_for_McastTranslation_create(portId, mcVid, userVid);
	if (RT_ERR_OK != ret)
		return SYS_ERR_FAILED;

	/*add mcVid to vlan table for l2 table lookup*/
	ret = drv_addport2mvlan(portId, mcVid, TAG_OPER_MODE_TRANSLATION);
	if (0 != ret) 
	{         
		return SYS_ERR_FAILED;
	} 

	/* QL 20160112
	 * 9601B has only one lan port, and one fid. it doesn't have c2s/sp2c/mc2s table,
	 * cvlan/svlan ingress/egress filter is disabled in ctc_sfu_vlan_init().
	 * We will use classification rule to implement mcast vlan tag/translation/
	 * transparent operation, For downstream, only mcvid vlan entry should be created for
	 * LUT hit, userVid vlan entry dont need to be created.
	 */
	return SYS_ERR_OK;
}

int McTagTranslationAdd(uint32 portId, uint32 mcVid, uint32 userVid)
{
	rt_error_common_t ulRetVal = RT_ERR_OK;
	rtk_portmask_t stLMask, stUntagLMsk, stSvlanUntagLMsk;
	uint32 uiPPort;

	if ((!IsValidLgcPort(portId)) ||
		(!VALID_VLAN_ID(mcVid)) ||
		(!VALID_VLAN_ID(userVid)))
	{
		return SYS_ERR_FAILED;
	}

	RTK_PORTMASK_RESET(stLMask);
	RTK_PORTMASK_RESET(stUntagLMsk);
	RTK_PORTMASK_RESET(stSvlanUntagLMsk);

	drv_mc_vlan_mem_get(mcVid, &stLMask, &stUntagLMsk);

	uiPPort = PortLogic2PhyID(portId);
	if (INVALID_PORT == uiPPort)
		return SYS_ERR_FAILED;
	
    if (RTK_PORTMASK_IS_PORT_SET(stLMask, uiPPort))
    {
        return SYS_ERR_OK;
    }
    else
    {
		RTK_PORTMASK_PORT_SET(stLMask, uiPPort);
		RTK_PORTMASK_PORT_SET(stSvlanUntagLMsk, uiPPort);
		RTK_PORTMASK_PORT_CLEAR(stUntagLMsk, uiPPort);
    }

	if((TRUE == is9601B()) && ((igmpctrlBlock.ctcControlType == MC_CTRL_GDA_MAC_VID)
		|| (igmpctrlBlock.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)))
		ulRetVal=drv_mc_translation_vlan_member_add(userVid, mcVid, stLMask, stUntagLMsk,
												stSvlanUntagLMsk, VLAN_FID_IVL);
	else
		ulRetVal=drv_mc_translation_vlan_member_add(userVid, mcVid, stLMask, stUntagLMsk,
												stSvlanUntagLMsk, VLAN_FID_SVL);
	
	if (RT_ERR_OK != ulRetVal)
	{
		return SYS_ERR_FAILED;
	}
	
	ulRetVal=drv_port_sp2c_entry_add(portId,mcVid,userVid);
	if (RT_ERR_OK != ulRetVal)
	{
		return SYS_ERR_FAILED;
	}
	
	ulRetVal=drv_port_c2s_entry_add(portId,userVid,mcVid);
	if (RT_ERR_OK != ulRetVal)
	{
		return SYS_ERR_FAILED;
	}
	
	return SYS_ERR_OK;
}

int igmpMcTagTranslationSet(uint32 portId, uint32 mcVid, uint32 userVid)
{
	int i,ret;

	if(igmpTagOperPerPortGet(portId) != TAG_OPER_MODE_TRANSLATION) 
	{   
		return SYS_ERR_FAILED;
	}
	
	RTK_CTC_DPRINTF("set port[%d] mc translation vlan table %d:%d", portId, mcVid, userVid);

	if (is9601B() || is9607C() || is9602C())
		ret = McTagTranslationCfAdd(portId, mcVid, userVid);
	else
		ret = McTagTranslationAdd(portId, mcVid, userVid);
	RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);

	mcast_vlan_querier_add(userVid);
	
	ret = igmpTagTranslationTableAdd(portId, mcVid, userVid);
	RETURN_IF_FAIL(ret == SYS_ERR_OK, ret); 

	mcast_translation_add(portId, mcVid, userVid); /*update igmp setting */
	return SYS_ERR_OK;
}

int igmpMcTagTranslationAdd(uint32 portId, uint32 mcVid, uint32 userVid)
{
	int ret=0;
	
	ret = igmpMcTagTranslationSet(portId, mcVid, userVid);
	RETURN_IF_FAIL(ret == SYS_ERR_OK, ret);

	return SYS_ERR_OK;
}

int igmpMcSwitchGet(void)
{
	return igmpctrlBlock.mcMode;
}

int igmpModeSwitch(void)
{
	uint32 i;
	mcvlan_tag_oper_mode_t tdMcTagOp;
	
	/* clear old configuration */  
	igmpMcCtrlGrpEntryClear();

	FOR_EACH_LAN_PORT(i)
	{
		igmpMcVlanClear(i);

		tdMcTagOp = igmpTagOperPerPortGet(i);

		igmpMcTagstripSet(i, tdMcTagOp);   
	}

	return SYS_ERR_OK;
}

int igmpMcSwitchSet(uint32 mode)
{
	if (mode == IGMP_MODE_SNOOPING)//IGMP SNOOPING
	{
		if(igmpMcSwitchGet() != IGMP_MODE_SNOOPING)
		{
			igmpctrlBlock.mcMode = IGMP_MODE_SNOOPING;			
		}
		else
		{
			return SYS_ERR_OK;
		}
	}
	else if(mode == IGMP_MODE_CTC)//CTC CONTROL
	{
		if(igmpMcSwitchGet()!=IGMP_MODE_CTC)
		{
			igmpctrlBlock.mcMode=IGMP_MODE_CTC;			
		}
		else
		{
			return SYS_ERR_OK;
		}
	}
	else
	{
		return SYS_ERR_FAILED;
	}
	
	RTK_CTC_DPRINTF("set mc mode %d\n",mode);
	igmpModeSwitch();
	
	mcast_mcSwitch_set(mode); /*update igmp setting */	
	return SYS_ERR_OK;
}

mc_control_e igmpMcCtrlGrpTypeGet(void)
{
	return igmpctrlBlock.ctcControlType;
}

int igmpMcCtrlGrpTypeSet(mc_control_e type)
{
	if(type == igmpctrlBlock.ctcControlType)
	{
		return SYS_ERR_OK;
	}

	if((type != MC_CTRL_GDA_MAC_VID) && (type != MC_CTRL_GDA_GDA_IP_VID)
	   && (type != MC_CTRL_GDA_MAC))
	{
		return SYS_ERR_FAILED;
	}
		
	/* clear group table */
	mcast_mcCtrl_group_entry_clear();

	if(TRUE == is9601B())
	{
		int i;
		for(i = 1; i < MAX_VLAN_NUMBER; i++) 
		{
			if(igspVlanMap[i].vid > 0)
			{
				if((type == MC_CTRL_GDA_MAC_VID) || (type == MC_CTRL_GDA_GDA_IP_VID))
				{
					drv_vlan_set_fidMode(igspVlanMap[i].vid, VLAN_FID_IVL);
				}
				else
				{
					drv_vlan_set_fidMode(igspVlanMap[i].vid, VLAN_FID_SVL);
				}
			}
		}

		for(i=0; i<VLAN_TRANSLATION_ENTRY; i++)
		{
			if(igmpTagTranslationTable[i].enable==ENABLED)
			{
				if((type == MC_CTRL_GDA_MAC_VID) || (type == MC_CTRL_GDA_GDA_IP_VID))
				{
					drv_vlan_set_fidMode(igmpTagTranslationTable[i].mcVid, VLAN_FID_IVL);
				}
				else
				{
					drv_vlan_set_fidMode(igmpTagTranslationTable[i].mcVid, VLAN_FID_SVL);
				}
			}
		}
	}
	
	if((type == MC_CTRL_GDA_MAC) || (type == MC_CTRL_GDA_MAC_VID))
	{
		drv_ipMcastMode_set(LOOKUP_ON_MAC_AND_VID_FID);

		if(TRUE == is9602C())
		{
			int32 ret;
			ret = rtk_l2_ipv6mcMode_set(LOOKUP_ON_MAC_AND_VID_FID);
			SYS_PRINTF("rtk_l2_ipv6mcMode_set ret %d, set type %d\n", ret, type);
		}
	}
	else if(type == MC_CTRL_GDA_GDA_IP_VID)
	{
		/*9601b ip-mcast-mode uses LOOKUP_ON_DIP_AND_VID
		   for 9607 because 64 entry svlan table don't support IVL mode,
		   ip-mcast-mode uses LOOKUP_ON_DIP_AND_SIP */
		if(TRUE == is9601B())
			drv_ipMcastMode_set(LOOKUP_ON_DIP_AND_VID);
		else if(TRUE == is9602C())
		{	
			int32 ret;
			/* 9602c ip-mcast-mode only support: dip-and-fid-vid and vid-and-mac */
			drv_ipMcastMode_set(LOOKUP_ON_DIP_AND_VID_FID);

			/* 9602c has ipv6-mcast-mode */
			ret = rtk_l2_ipv6mcMode_set(LOOKUP_ON_DIP);
			SYS_PRINTF("rtk_l2_ipv6mcMode_set ret %d, set type %d\n", ret, type);
		}
		else
			drv_ipMcastMode_set(LOOKUP_ON_DIP_AND_SIP);
	}
	
	igmpctrlBlock.ctcControlType = type;

	mcast_mcCtrl_type_set(type); /*update igmp setting */
	return SYS_ERR_OK;
}

int igmpMcCtrlGrpEntryClear(void)
{
	sys_vlanmask_t  vlanMsk;
	int port=0;
	
	mcast_mcCtrl_group_entry_clear();

	if(igmpctrlBlock.mcMode == IGMP_MODE_CTC)
	{
		if ((igmpctrlBlock.ctcControlType==MC_CTRL_GDA_MAC_VID) ||
			(igmpctrlBlock.ctcControlType==MC_CTRL_GDA_GDA_IP_VID)) 
		{
			FOR_EACH_LAN_PORT(port)
			{
				igmpMcVlanClear(port);
			}
		}
	}
	
	return SYS_ERR_OK;
}

int igmpMcCtrlGrpEntryDelete(uint32 portId, uint32 vlanId, uint8 *mac)
{
	int ret;
	oam_mcast_control_entry_t ctl_entry_list[(MAX_PORT_NUM)*(MAX_GROUP_NUM_PER_PORT)];
	int num=0;
	int delflag=0;
	int i=0;
	int dbPort; /* port value stored in local database */
	
	if(igmpctrlBlock.mcMode != IGMP_MODE_CTC) 
	{   
		RTK_CTC_DPRINTF("");
		return SYS_ERR_FAILED;
	}

	/*mc control group entries are maintained by igmp db*/
	mcast_mcCtrl_group_entry_delete(portId, vlanId, mac);
	
	if ((igmpctrlBlock.ctcControlType==MC_CTRL_GDA_MAC_VID) ||
		(igmpctrlBlock.ctcControlType==MC_CTRL_GDA_GDA_IP_VID)) 
	{
		igmpMcCtrlGrpEntryGet(ctl_entry_list,&num);
		delflag = 1;
		for (i=0; i< num; i++)
		{
			dbPort = ctl_entry_list[i].port_id - CTC_START_PORT_NUM;
			if ((dbPort == portId) && (ctl_entry_list[i].vlan_id == vlanId))
			{
				delflag = 0;
				break;
			}
		}

		if (delflag==1)
		{
			ret = igmpMcVlanDelete(portId, vlanId);
			if (SYS_ERR_OK != ret)
			{     
				RTK_CTC_DPRINTF("");
				return SYS_ERR_FAILED;
			}
		}
	}
	
	return SYS_ERR_OK;
}

int igmpTagTranslationTableGetByMcvid(uint32 portId, uint32 mcVid, uint32 *userVid)
{
	uint32 i;

	for (i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if (igmpTagTranslationTable[i].enable == ENABLED)
		{
			if(igmpTagTranslationTable[i].portId == portId
				&&igmpTagTranslationTable[i].mcVid == mcVid)
			{ /* match */
				*userVid = igmpTagTranslationTable[i].userVid;
				return SYS_ERR_OK;
			}
		}
	}

	return SYS_ERR_FAILED;
}

int igmpTagTranslationTableGetbyUservid(uint32 portId, uint32 userVid, uint32 *mcVid)
{
	uint32 i;

	for (i=0; i<VLAN_TRANSLATION_ENTRY; i++)
	{
		if (igmpTagTranslationTable[i].enable==ENABLED)
		{
			if(igmpTagTranslationTable[i].portId == portId
				&&igmpTagTranslationTable[i].userVid == userVid)
			{ /* match */
				*mcVid = igmpTagTranslationTable[i].mcVid;
				return SYS_ERR_OK;
			}
		}
	}

	/* not match */
	return SYS_ERR_FAILED;
}

int igmpMcCtrlGrpEntryAdd(uint32 portId, uint32 vlanId, uint8 *mac)
{	
	if(igmpctrlBlock.mcMode != IGMP_MODE_CTC) 
	{
		RTK_CTC_DPRINTF("");
		return SYS_ERR_FAILED;
	}
	
	if ((igmpctrlBlock.ctcControlType == MC_CTRL_GDA_MAC_VID) || 
		(igmpctrlBlock.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)) 
	{
		igmpMcVlanAdd(portId, vlanId);
	}

	/*mc control group entries are maintained by igmp db*/
	mcast_mcCtrl_group_entry_add(portId, vlanId, mac);
	
	return SYS_ERR_OK;
}

int32 mcast_portGroup_limit_get(sys_logic_port_t port, uint32 *pMaxnum)
{	
	*pMaxnum = mcast_mcGrp_maxNum_get(port);
	return SYS_ERR_OK;
}

int32 mcast_portGroup_limit_set(sys_logic_port_t port, uint16 maxnum)
{
	mcast_mcGrp_maxNum_set(port, maxnum);
	return SYS_ERR_OK;
}

int igmpMcGrpMaxNumGet(uint32 portId)
{
	uint32 num=0;
	
	if(IsValidLgcLanPort(portId)==FALSE)
  	{
  		RTK_CTC_DPRINTF("");
  		return SYS_ERR_FAILED;
  	}
	
	mcast_portGroup_limit_get(portId, &num);
	RTK_CTC_DPRINTF("num %d",num);
	
	return num;
}

int igmpMcGrpMaxNumSet(uint32 portId, uint32 num)
{
	if(IsValidLgcLanPort(portId) == FALSE)
	{
		RTK_CTC_DPRINTF("num %d",num);
		return SYS_ERR_FAILED;
	}

	RTK_CTC_DPRINTF("set port %d grp limit %d",portId,num);
	mcast_portGroup_limit_set(portId, num);

	return SYS_ERR_OK;
}

int igmpMcFastleaveModeSet(uint32 enable)
{
	if(enable==1)
	{
		igmpctrlBlock.leaveMode = IGMP_LEAVE_MODE_FAST;
	}

	if(enable==0)
	{
		igmpctrlBlock.leaveMode = IGMP_LEAVE_MODE_NON_FAST_LEAVE;
	}

	mcast_mcFastLeave_set(igmpctrlBlock.leaveMode); /*update igmp setting */	
	return SYS_ERR_OK;
}

int igmpmcinit(void)
{
	int i;
	
	memset(igspVlanMap,0,sizeof(igspVlanMap));
	memset(igmpTagOperPerPort,0,sizeof(igmpTagOperPerPort));
	memset(igmpTagTranslationTable,0,sizeof(igmpTagTranslationTable));
	memset(&igmpctrlBlock,0,sizeof(igmpctrlBlock));
 
  	igmpctrlBlock.mcMode = IGMP_MODE_SNOOPING;
	igmpctrlBlock.leaveMode=IGMP_LEAVE_MODE_NON_FAST_LEAVE;
  	igmpctrlBlock.ctcControlType = MC_CTRL_GDA_MAC;
	igmpMcCtrlGrpEntryClear();

	FOR_EACH_LAN_PORT(i)
	{
		mcast_portGroup_limit_set(i, 16);
	}
	
#ifdef IGMP_TEST_MODE
	//igmpMcSwitchSet(IGMP_MODE_CTC);
	igmpMcCtrlGrpTypeSet(MC_CTRL_GDA_MAC);
	igmpMcFastleaveModeSet(1);
	igmpMcVlanAdd(0, 2500);
	igmpMcVlanAdd(0, 2501);

	//igmpMcTagstripSet(0, TAG_OPER_MODE_TRANSLATION);
	if(igmpTagOperPerPortGet(0) == TAG_OPER_MODE_TRANSLATION)
	{
		igmpMcTagTranslationSet(0,2500, 250);
		igmpMcTagTranslationSet(0,2501, 251);
		igmpMcTagTranslationSet(0,2502, 252);
	}
	
	if(igmpctrlBlock.mcMode == IGMP_MODE_CTC)
	{
		char mac[6];
		
		mac[0] = 0x01;
		mac[1] = 0x00;
		mac[2] = 0x5e;
		mac[3] = 0x01;
		mac[4] = 0x02;
		mac[5] = 0x03;
		if(igmpctrlBlock.ctcControlType == MC_CTRL_GDA_MAC_VID)
		{
			igmpMcCtrlGrpEntryAdd(0, 2500, mac);
			igmpMcCtrlGrpEntryAdd(0, 2501, mac);
		}
		else if(igmpctrlBlock.ctcControlType == MC_CTRL_GDA_MAC)
		{
			igmpMcCtrlGrpEntryAdd(0, 0, mac);
		}
		else if(igmpctrlBlock.ctcControlType == MC_CTRL_GDA_GDA_IP_VID)
		{
			mac[2] = 0xe5;
			igmpMcCtrlGrpEntryAdd(0, 2500, mac);
			igmpMcCtrlGrpEntryAdd(0, 2501, mac);
		}
	}
#endif

	return SYS_ERR_OK;
}

int ismvlan(int vid)
{
	/* vid 0 means untagged */
	if (vid == 0)
		return TRUE;
	else
		return FALSE;
}

int igmpmcvlancheck(void)
{
	int delflag=0;
	int num=0;
	int i=0,j=0;
	int portId;
	int ret=0;
	oam_mcast_control_entry_t ctl_entry_list[(MAX_PORT_NUM)*(MAX_GROUP_NUM_PER_PORT)];
	uint16 tmpvlanId[MAX_VLAN_NUMBER];
	int vlannum=0;
	int dbPort; /* port value stored in local database */
	
	if(igmpctrlBlock.mcMode != IGMP_MODE_CTC)
	{
		return 0;
	}

	if ((igmpctrlBlock.ctcControlType==MC_CTRL_GDA_MAC_VID) ||
		(igmpctrlBlock.ctcControlType==MC_CTRL_GDA_GDA_IP_VID)) 
	{
		igmpMcCtrlGrpEntryGet(ctl_entry_list, &num);
		
		FOR_EACH_LAN_PORT(portId)
		{
			igmpMcVlanGet(portId, (uint8 *)tmpvlanId, &vlannum);
			if(vlannum==0)
			{
				continue;
			}
			
			for(j=0; j<vlannum; j++)
			{
				delflag = 1;
				for (i=0; i< num; i++)
				{
					dbPort = ctl_entry_list[i].port_id - CTC_START_PORT_NUM;
					if ((dbPort == portId) && (ctl_entry_list[i].vlan_id == tmpvlanId[j]))
					{
						delflag = 0;
						break;
					}
				}

				if (delflag==1)
				{
					ret = igmpMcVlanDelete(portId, tmpvlanId[j]);
					if (SYS_ERR_OK != ret)
					{
						RTK_CTC_DPRINTF("");
					}
				}
			}
		}
	}
	
	return 0;
}

int igmpmcfastleaveget(void)
{
	return igmpctrlBlock.leaveMode ;
}

void igmp_cli_set(oam_cli_t *pCli)
{
	unsigned char ctrlType;
	unsigned short vid;
	uint32 port;
	
	switch(pCli->u.cliIgmp.type)
    {
    	case IGMP_CONTROL_TYPE:
			if(pCli->u.cliIgmp.controlType == 0)
				ctrlType = MC_CTRL_GDA_MAC;
			else if(pCli->u.cliIgmp.controlType == 1)
				ctrlType = MC_CTRL_GDA_MAC_VID;
			else
				ctrlType = MC_CTRL_GDA_GDA_IP_VID;

			igmpMcCtrlGrpTypeSet(ctrlType);
			break;
		case IGMP_MCVLAN_ADD_TYPE:
			vid = pCli->u.cliIgmp.mcVlan;
			port = pCli->u.cliIgmp.port;
			igmpMcVlanAdd(port, vid);
			break;
		case IGMP_MCVLAN_DEL_TYPE:
			vid = pCli->u.cliIgmp.mcVlan;
			FOR_EACH_LAN_PORT(port)
			{
				igmpMcVlanDelete(port, vid);
			}
			break;
		case IGMP_DEBUG_TYPE:
			debug_enable = pCli->u.cliIgmp.val;
			break;
		case IGMP_MCTAGOPER_TYPE:
			port = pCli->u.cliIgmp.port;
			igmpMcTagstripSet(port, pCli->u.cliIgmp.val);
			break;
		case IGMP_TRANSLATION_ADD_TYPE:
			port = pCli->u.cliIgmp.port;
			igmpMcTagTranslationAdd(port,pCli->u.cliIgmp.mcVlan,pCli->u.cliIgmp.val);
			break;
		default:
			break;
	}
}
