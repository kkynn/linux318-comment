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
* Purpose : 
*
* Feature : 
*
*/

#include <stdio.h>
#include <string.h>

#include <sys_portview.h>
#include <sys_portmask.h>

#include "epon_oam_igmp_db.h"
#include "epon_oam_igmp_util.h"
#include "epon_oam_err.h"

/* Semaphore for igmp database access */
static sem_t igmpDbSem;

static igmp_config_t igmpConfig;

static igmp_group_entry_t groupEntryArray[MCAST_MAX_GROUP_NUM];
static igmp_group_entry_t *groupEntryPool = NULL;

static igmp_port_entry_t portEntryArray[MCAST_MAX_PORT_ENTRY_NUM];
static igmp_port_entry_t *portEntryPool = NULL;

static igmp_source_entry_t sourceEntryArray[MCAST_MAX_SOURCE_ENTRY_NUM];
static igmp_source_entry_t *sourceEntryPool = NULL;

static igmp_group_entry_t * ipv4_group_hashTable[MCAST_GROUP_HASH_NUM];
static uint32 group_hashMask;
#ifdef SUPPORT_MLD_SNOOPING
static igmp_group_entry_t * ipv6_group_hashTable[MCAST_GROUP_HASH_NUM];
#endif

extern rtk_enable_t  igmp_packet_stop;
extern rtk_enable_t  mcast_timer_stop;

static void  mcast_link_group_entry(igmp_group_entry_t * groupEntry ,  igmp_group_entry_t ** hashTable);
static igmp_group_entry_t* mcast_allocate_group_entry(void);
static void mcast_free_group_entry(igmp_group_entry_t * groupEntryPtr);
static void mcast_delete_group_entry(igmp_group_entry_t * groupEntry, igmp_group_entry_t ** hashTable);

static igmp_port_entry_t* mcast_allocate_port_entry(void);
static void mcast_free_port_entry(igmp_port_entry_t* portEntryPtr); 
static void mcast_delete_port_entry(igmp_group_entry_t * groupEntry, igmp_port_entry_t *portEntry);

static igmp_source_entry_t * mcast_allocateSourceEntry(void);
static igmp_source_entry_t * mcast_searchSourceEntry(uint32 ipVersion, 
											uint32 *sourceAddr, 
											igmp_port_entry_t *portEntry);
static void mcast_linkSourceEntry(igmp_port_entry_t *portEntry, 
								igmp_source_entry_t *entryNode);
static void mcast_unlinkSourceEntry(igmp_port_entry_t *portEntry, 
									igmp_source_entry_t *sourceEntry);
static void mcast_clearSourceEntry(igmp_source_entry_t *sourceEntry);
static void mcast_freeSourceEntry(igmp_source_entry_t *sourceEntry);
static void mcast_deleteSourceEntry(igmp_port_entry_t *portEntry, 
							igmp_source_entry_t* sourceEntry);


static uint32 mcast_is_source_equal(uint32 *srcAddr1, uint32 *srcAddr2, uint32 ipVersion)
{
	if(ipVersion == MULTICAST_TYPE_IPV4)
	{
		if(srcAddr1[0] == srcAddr2[0])
		{
			return TRUE;
		}
	}
#ifdef SUPPORT_MLD_SNOOPING		
	else
	{
		if( (srcAddr1[0] == srcAddr2[0])&&
			(srcAddr1[1] == srcAddr2[1])&&
			(srcAddr1[2] == srcAddr2[2])&&
			(srcAddr1[3] == srcAddr2[3]))
		{
			return TRUE;
		}
	}
#endif
	return FALSE;
}

static inline uint32 mcast_get_hash_value(uint32 ipVersion, uint32 *groupAddr)
{
	uint32 hashVal = 0;
	
	if(MULTICAST_TYPE_IPV4 == ipVersion)
	{
		if(IGMP_MODE_CTC == mcast_mcMode_get() && 
		   (MC_CTRL_GDA_MAC == mcast_mcCtrl_type_get(ipVersion) || 
			MC_CTRL_GDA_MAC_VID == mcast_mcCtrl_type_get(ipVersion))
		 )
		{
			/* In ctc mode,when control type is MC_CTRL_GDA_MAC or MC_CTRL_GDA_MAC_VID
			  * group ip is converted from group mac,
			  * so only use lower 24 bits of ip address */
			hashVal = groupAddr[0] & 0x00ffffff;
		}
		else
			hashVal = groupAddr[0];
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		//use ipv6[96:127] 4 bytes
		hashVal = groupAddr[3];
	}
#endif
	return hashVal;
}

static inline uint32 mcast_hash_algorithm(uint32 ipVersion, 
									uint32 *groupAddr, 
									uint16  vid)
{
	uint32 hashIndex=0;
	uint32 hashVal;
	int i;
	
	hashVal = mcast_get_hash_value(ipVersion, groupAddr);

	hashIndex = (hashVal&0xFF) | 
				((hashVal>>8)&0xFF) | 
				((hashVal>>16)&0xFF) | 
				((hashVal>>24)&0xFF);

	/*group entry is match by (group + vid), should generate hash by vid*/
	hashIndex = hashIndex ^ ((vid >>8)& 0xFF) ^ (vid & 0xFF);
	
	hashIndex = group_hashMask & hashIndex;
	
	return hashIndex;
}

static int32 mcast_match_group_entry(igmp_group_entry_t * entry, 
								uint32 ipVersion, 
								uint16  vid, 
								uint32 *groupAddr)
{
	if(ipVersion != entry->ipVersion)
		return FALSE;

	/* in both snooping and ctc mode, we store group entry by (gip + vid) */
	if (MULTICAST_TYPE_IPV4 == ipVersion)
	{
		if(IGMP_MODE_CTC == mcast_mcMode_get() && 
			(MC_CTRL_GDA_MAC == mcast_mcCtrl_type_get(ipVersion) || 
			MC_CTRL_GDA_MAC_VID == mcast_mcCtrl_type_get(ipVersion))
		  )
		{
			/*only check lower 24 bits of ip address */
			if (((groupAddr[0] & 0x00ffffff) == (entry->groupAddr[0] & 0x00ffffff)) && 
				(vid == entry->vid))
				return TRUE;		
		}
		else
		{
			if ((groupAddr[0] == entry->groupAddr[0]) && (vid == entry->vid))
				return TRUE;
		}
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		if ((groupAddr[0] == entry->groupAddr[0]) &&
		   (groupAddr[1] == entry->groupAddr[1]) &&
		   (groupAddr[2] == entry->groupAddr[2]) &&
		   (groupAddr[3] == entry->groupAddr[3]) &&
		   (vid == entry->vid))
		   return TRUE;
	}
#endif
	
	return FALSE;
}

/* find a group address in a group list  */
igmp_group_entry_t * mcast_search_group_entry(uint32 ipVersion, 
												uint16  vid, 
												uint32 *groupAddr)
{
	igmp_group_entry_t *groupPtr = NULL;
	uint32 hashIndex = 0;

	hashIndex = mcast_hash_algorithm(ipVersion, groupAddr,vid);
	//SYS_DBG("[IGMP %s]:%d hashIndex[%d], ipversion[%d]\n", __func__, __LINE__,hashIndex,ipVersion);

	IGMP_SEM_LOCK(igmpDbSem);
	
	if(MULTICAST_TYPE_IPV4 == ipVersion)
		groupPtr = ipv4_group_hashTable[hashIndex];
#ifdef SUPPORT_MLD_SNOOPING
	else
		groupPtr = ipv6_group_hashTable[hashIndex];
#endif
	
	while (groupPtr != NULL)
	{
		if(TRUE == mcast_match_group_entry(groupPtr, ipVersion, vid, groupAddr))
		{
			IGMP_SEM_UNLOCK(igmpDbSem);
			return groupPtr;
		}
		groupPtr = groupPtr->next;
	}
	
	IGMP_SEM_UNLOCK(igmpDbSem);
	return NULL;
}


int32 mcast_group_portTime_update(uint32 ipVersion, uint16  vid, 
							uint8* mac, uint32 *groupAddr,uint32 maxRespTime)
{
	igmp_group_entry_t *groupEntry = NULL;
	igmp_port_entry_t * portEntry = NULL;
	int i;
	int newTime;
	
	groupEntry = mcast_search_group_entry(ipVersion, vid, groupAddr);
	if(groupEntry != NULL)
	{
		newTime = maxRespTime ? maxRespTime : igmpConfig.lastMemberQueryIntv;
		FOR_EACH_LAN_PORT(i)
		{
			portEntry = groupEntry->portEntry[i];
			if(portEntry != NULL)
			{
				portEntry->groupMbrTimer = igmpConfig.lastMemberQueryCnt * newTime;
			}
		}
	}

	return TRUE;
}

uint32 mcast_portMemberCheck(igmp_port_entry_t * portEntry)
{
	igmp_source_entry_t * sourcePtr=NULL;;
	
	if(portEntry->groupMbrTimer > 0) /*exclude mode never expired*/
	{
		return TRUE;
	}
	else/*include mode*/
	{
		sourcePtr = portEntry->sourceList;
		while (sourcePtr != NULL)
		{
			if(sourcePtr->portTimer > 0)
			{
				return TRUE;
			}
			sourcePtr=sourcePtr->next;
		}
	}

	return FALSE;
}

void mcast_getGroupFwdPortMask(igmp_group_entry_t *groupEntry, sys_logic_portmask_t *fwdPortMask)
{
	uint32 lport;
	igmp_port_entry_t *portEntry=NULL;

	LOGIC_PORTMASK_CLEAR_ALL(*fwdPortMask);
	
	if (groupEntry == NULL)
	{
		/*broadcast*/
		LOGIC_PORTMASK_SET_ALL(*fwdPortMask);
	}
	else
	{
		FOR_EACH_LAN_PORT(lport)
		{
			portEntry = groupEntry->portEntry[lport];
			
			if (portEntry != NULL)
			{
				if (mcast_portMemberCheck(portEntry) == TRUE)
					LOGIC_PORTMASK_SET_PORT(*fwdPortMask, lport);
			}
		}
	}
}

static void mcast_LinkSrcToQueryList(igmp_port_entry_t *portEntry, igmp_source_entry_t *sourceEntry)
{
	IGMP_SEM_LOCK(igmpDbSem);

	if(portEntry->querySrcList != NULL)
	{
		portEntry->querySrcList->qpre = sourceEntry;
	}
	
	sourceEntry->qnxt = portEntry->querySrcList;
	portEntry->querySrcList = sourceEntry;
	portEntry->querySrcList->qpre = NULL;
	
	IGMP_SEM_UNLOCK(igmpDbSem);
}

static void mcast_unlinkSrcFromQueryList(igmp_port_entry_t *portEntry, 
									igmp_source_entry_t *sourceEntry)
{
	IGMP_SEM_LOCK(igmpDbSem);
	
	/* unlink entry node*/ 
	if(sourceEntry == portEntry->querySrcList) /*unlink group list head*/
	{
		portEntry->querySrcList = sourceEntry->qnxt;
		if(portEntry->querySrcList != NULL)
		{
			portEntry->querySrcList->qpre = NULL;
		}
	}
	else
	{
		if(sourceEntry->qpre != NULL)
		{
			sourceEntry->qpre->qnxt = sourceEntry->qnxt;
		}

		if(sourceEntry->qnxt != NULL)
		{
			sourceEntry->qnxt->qpre = sourceEntry->qpre;
		}
	}
	
	sourceEntry->qpre = NULL;
	sourceEntry->qnxt = NULL;
	
	IGMP_SEM_UNLOCK(igmpDbSem);
}

static void mcast_ClearQueryList(igmp_port_entry_t *portEntry)
{
	igmp_source_entry_t *sourceEntry=NULL;
	
	while(portEntry->querySrcList != NULL)
	{
		sourceEntry = portEntry->querySrcList;
		
		portEntry->querySrcList = sourceEntry->qnxt;
		
		if(portEntry->querySrcList != NULL)
		{
			portEntry->querySrcList->qpre = NULL;
		}
		
		sourceEntry->qpre = NULL;
		sourceEntry->qnxt = NULL;
	}
}

static void mcast_clearSourceList(igmp_port_entry_t *portEntry)
{
	igmp_source_entry_t *sourceEntry=NULL;
	igmp_source_entry_t *nextSourceEntry=NULL;
	
	sourceEntry=portEntry->sourceList;
	while(sourceEntry!=NULL)
	{
		nextSourceEntry=sourceEntry->next;
		mcast_deleteSourceEntry(portEntry, sourceEntry);
		sourceEntry=nextSourceEntry;
	}
}

static void mcast_clearOldSrcList(igmp_port_entry_t *portEntry, 
						uint32 ipVersion, 
						uint32 *srcList, 
						uint16 srcNum)
{
	igmp_source_entry_t *sourceEntry;
	igmp_source_entry_t *nextSourceEntry=NULL;
	uint32 *srcAddr;
	int j;
	
	/* delete all old source list which is not included in new src list */
	sourceEntry = portEntry->sourceList;
	
	while(sourceEntry!=NULL)
	{
		nextSourceEntry=sourceEntry->next;
	
		srcAddr = srcList;
		for(j=0; j<srcNum; j++)
		{
			if (TRUE == mcast_is_source_equal(srcAddr, sourceEntry->sourceAddr, ipVersion))
				break;
			
			if (ipVersion == MULTICAST_TYPE_IPV4)
				srcAddr++;
			else
				srcAddr += 4;
		}
		if (j >= srcNum)//not found
			mcast_deleteSourceEntry(portEntry, sourceEntry);
		
		sourceEntry = nextSourceEntry;
	}
}

int32 mcast_addIsIncludeSrc(igmp_group_entry_t *groupEntry, 
						igmp_port_entry_t *portEntry, 
						uint32 ipVersion, 
						uint32 *srcList, 
						uint16 srcNum)
{
	igmp_source_entry_t *newSourceEntry, *sourceEntry;
	int j;

	for(j=0; j<srcNum; j++)
	{
		sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
		if (sourceEntry)
		{
			/* update source timer to GMI */
			sourceEntry->portTimer = igmpConfig.groupMemberIntv;
		}
		else
		{
			newSourceEntry = mcast_allocateSourceEntry();
			if(newSourceEntry==NULL)
			{
				printf("run out of source entry!\n");
				return FALSE;
			}

			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				newSourceEntry->sourceAddr[0] = srcList[0];
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{	
				newSourceEntry->sourceAddr[0] = srcList[0];
				newSourceEntry->sourceAddr[1] = srcList[1];
				newSourceEntry->sourceAddr[2] = srcList[2];
				newSourceEntry->sourceAddr[3] = srcList[3];
			}
#endif						

			newSourceEntry->portTimer = igmpConfig.groupMemberIntv;

			mcast_linkSourceEntry(portEntry, newSourceEntry);
		}

		if(ipVersion == MULTICAST_TYPE_IPV4)
		{	
			srcList++;
		}
#ifdef SUPPORT_MLD_SNOOPING
		else
		{
			srcList += 4;
		}
#endif
	}

	return TRUE;
}

int32 mcast_addIsExcludeSrc(igmp_group_entry_t *groupEntry, 
						igmp_port_entry_t *portEntry, 
						uint32 ipVersion, 
						uint32 *srcList, 
						uint16 srcNum)
{
	igmp_source_entry_t *newSourceEntry, *sourceEntry;
	igmp_source_entry_t *nextSourceEntry=NULL;
	uint32 *srcAddr;
	int j;

	/* delete all old source list which is not included in new src list */
	mcast_clearOldSrcList(portEntry, ipVersion, srcList, srcNum);
	
	if (portEntry->groupMbrTimer > 0) /*means exclude mode*/
	{
		for(j=0; j<srcNum; j++)
		{
			sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
			if (sourceEntry)
			{
				/*do nothing for those old sources */
			}
			else
			{
				newSourceEntry = mcast_allocateSourceEntry();
				if(newSourceEntry==NULL)
				{
					printf("run out of source entry!\n");
					return FALSE;
				}

				if(ipVersion == MULTICAST_TYPE_IPV4)
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
				}
#ifdef SUPPORT_MLD_SNOOPING
				else
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
					newSourceEntry->sourceAddr[1] = srcList[1];
					newSourceEntry->sourceAddr[2] = srcList[2];
					newSourceEntry->sourceAddr[3] = srcList[3];
				}
#endif						

				newSourceEntry->portTimer = igmpConfig.groupMemberIntv;

				mcast_linkSourceEntry(portEntry, newSourceEntry);
			}
			
			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				srcList++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				srcList += 4;
			}
#endif
		}

		//update group timer
		portEntry->groupMbrTimer = igmpConfig.groupMemberIntv;
	}
	else/*means include mode*/
	{
		for(j=0; j<srcNum; j++)
		{
			sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
			if (sourceEntry)
			{
				/*do nothing for those old sources */
			}
			else
			{
				newSourceEntry = mcast_allocateSourceEntry();
				if(newSourceEntry==NULL)
				{
					printf("run out of source entry!\n");
					return FALSE;
				}

				if(ipVersion == MULTICAST_TYPE_IPV4)
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
				}
#ifdef SUPPORT_MLD_SNOOPING
				else
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
					newSourceEntry->sourceAddr[1] = srcList[1];
					newSourceEntry->sourceAddr[2] = srcList[2];
					newSourceEntry->sourceAddr[3] = srcList[3];
				}
#endif						

				newSourceEntry->portTimer = 0;

				mcast_linkSourceEntry(portEntry, newSourceEntry);
			}
			
			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				srcList++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				srcList += 4;
			}
#endif
		}
		
		//change to EXCLUDE mode
		portEntry->groupMbrTimer = igmpConfig.groupMemberIntv;
	}

	return TRUE;
}

int32 mcast_addToIncludeSrc(igmp_group_entry_t *groupEntry, 
						igmp_port_entry_t *portEntry, 
						uint32 ipVersion, 
						uint32 *srcList, 
						uint16 srcNum)
{
	igmp_source_entry_t *newSourceEntry, *sourceEntry;
	uint32 *srcListTmp;
	uint32 querySrcList[40];//max to 10 source should be inserted to gss query.
	uint32 querySrcNum=0, tmpIdx;
	int j;

	if (IGMP_LEAVE_MODE_FAST == mcast_mcFastLeave_get())
	{
		portEntry->groupMbrTimer = 0;

		mcast_clearSourceList(portEntry);
		for(j=0; j<srcNum; j++)
		{
			newSourceEntry = mcast_allocateSourceEntry();
			if(newSourceEntry==NULL)
			{
				printf("run out of source entry!\n");
				return FALSE;
			}

			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				newSourceEntry->sourceAddr[0] = srcList[0];
				srcList++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				newSourceEntry->sourceAddr[0] = srcList[0];
				newSourceEntry->sourceAddr[1] = srcList[1];
				newSourceEntry->sourceAddr[2] = srcList[2];
				newSourceEntry->sourceAddr[3] = srcList[3];

				srcList += 4;
			}
#endif						

			newSourceEntry->portTimer = igmpConfig.groupMemberIntv;

			mcast_linkSourceEntry(portEntry, newSourceEntry);
		}

		return TRUE;
	}

	if(srcNum == 0)
	{
		/* ToInclude {} means to leave group, set group timer to (query count * query interval) */
		portEntry->groupMbrTimer = igmpConfig.lastMemberQueryCnt * igmpConfig.lastMemberQueryIntv;	
	}
	
	/* get all active sources which are not reported in this packet */
	mcast_ClearQueryList(portEntry);
	sourceEntry = portEntry->sourceList;
	while (sourceEntry != NULL)
	{
		if (0 == sourceEntry->portTimer)
			continue;
		
		srcListTmp = srcList;
		for(j=0; j<srcNum; j++)
		{
			if (TRUE == mcast_is_source_equal(srcListTmp, sourceEntry->sourceAddr, ipVersion))
				break;
			
			if (ipVersion == MULTICAST_TYPE_IPV4)
				srcListTmp++;
			else
				srcListTmp += 4;
		}

		if (j >= srcNum)
		{//not found in srcList
			if (querySrcNum >= 10)
				break;
			
			if (sourceEntry->portTimer > igmpConfig.lastMemberQueryIntv)
				sourceEntry->portTimer = igmpConfig.lastMemberQueryIntv;

			mcast_LinkSrcToQueryList(portEntry, sourceEntry);
			
			if (ipVersion == MULTICAST_TYPE_IPV4)
			{
				querySrcList[querySrcNum++] = sourceEntry->sourceAddr[0];
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				tmpIdx = querySrcNum*4;
				querySrcList[tmpIdx+0] = sourceEntry->sourceAddr[0];
				querySrcList[tmpIdx+1] = sourceEntry->sourceAddr[1];
				querySrcList[tmpIdx+2] = sourceEntry->sourceAddr[2];
				querySrcList[tmpIdx+3] = sourceEntry->sourceAddr[3];
				querySrcNum++;
			}
#endif
		}
		
		sourceEntry = sourceEntry->next;
	}
	
	if (portEntry->groupMbrTimer > 0) /*means exclude mode*/
	{
		/* add new source to group */
		srcListTmp = srcList;
		for(j=0; j<srcNum; j++)
		{
			sourceEntry = mcast_searchSourceEntry(ipVersion, srcListTmp, portEntry);
			if (sourceEntry)
			{
				sourceEntry->portTimer = igmpConfig.groupMemberIntv;
			}
			else
			{
				newSourceEntry = mcast_allocateSourceEntry();
				if(newSourceEntry==NULL)
				{
					printf("run out of source entry!\n");
					return FALSE;
				}

				if(ipVersion == MULTICAST_TYPE_IPV4)
				{	
					newSourceEntry->sourceAddr[0] = srcListTmp[0];
				}
#ifdef SUPPORT_MLD_SNOOPING
				else
				{	
					newSourceEntry->sourceAddr[0] = srcListTmp[0];
					newSourceEntry->sourceAddr[1] = srcListTmp[1];
					newSourceEntry->sourceAddr[2] = srcListTmp[2];
					newSourceEntry->sourceAddr[3] = srcListTmp[3];
				}
#endif						

				newSourceEntry->portTimer = igmpConfig.groupMemberIntv;

				mcast_linkSourceEntry(portEntry, newSourceEntry);
			}
			
			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				srcListTmp++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				srcListTmp += 4;
			}
#endif
		}

		//send group specified query, should not update Group Timer
		portEntry->groupSpecQueryCnt++;
		mcast_send_gs_query(groupEntry, portEntry->portNum);
		
		//send group and source(X-A) specified query
		if (querySrcNum > 0)
		{
			portEntry->groupSrcSpecQueryCnt++;
			mcast_send_gss_query(groupEntry, querySrcList, 
								querySrcNum, portEntry->portNum);
		}
	}
	else/*means include mode*/
	{
		for(j=0; j<srcNum; j++)
		{
			sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
			if (sourceEntry)
			{
				/* update source timer to GMI */
				sourceEntry->portTimer = igmpConfig.groupMemberIntv;
			}
			else
			{
				newSourceEntry = mcast_allocateSourceEntry();
				if(newSourceEntry==NULL)
				{
					printf("run out of source entry!\n");
					return FALSE;
				}

				if(ipVersion == MULTICAST_TYPE_IPV4)
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
				}
#ifdef SUPPORT_MLD_SNOOPING
				else
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
					newSourceEntry->sourceAddr[1] = srcList[1];
					newSourceEntry->sourceAddr[2] = srcList[2];
					newSourceEntry->sourceAddr[3] = srcList[3];
				}
#endif						

				newSourceEntry->portTimer = igmpConfig.groupMemberIntv;

				mcast_linkSourceEntry(portEntry, newSourceEntry);
			}

			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				srcList++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				srcList += 4;
			}
#endif
		}
		//send group and source(A-B) specified query
		if (querySrcNum > 0)
		{
			portEntry->groupSrcSpecQueryCnt++;
			mcast_send_gss_query(groupEntry, querySrcList, 
								querySrcNum, portEntry->portNum);
		}
	}

	return TRUE;
}

int32 mcast_addToExcludeSrc(igmp_group_entry_t *groupEntry, 
						igmp_port_entry_t *portEntry, 
						uint32 ipVersion, 
						uint32 *srcList, 
						uint16 srcNum)
{
	igmp_source_entry_t *newSourceEntry, *sourceEntry;
	uint32 querySrcList[40];//max to 10 source should be inserted to gss query.
	uint32 querySrcNum=0, tmpIdx;
	int j;

	/* delete all old source list which is not included in new src list */
	mcast_clearOldSrcList(portEntry, ipVersion, srcList, srcNum);
	
	if (portEntry->groupMbrTimer > 0) /*means exclude mode*/
	{
		for(j=0; j<srcNum; j++)
		{
			sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
			if (sourceEntry)
			{
				/* add active source to query list */
				if (sourceEntry->portTimer > 0)
				{
					if (querySrcNum < 10)
					{
						if (sourceEntry->portTimer > igmpConfig.lastMemberQueryIntv)
							sourceEntry->portTimer = igmpConfig.lastMemberQueryIntv;
						
						mcast_LinkSrcToQueryList(portEntry, sourceEntry);
						
						if (ipVersion == MULTICAST_TYPE_IPV4)
						{
							querySrcList[querySrcNum++] = sourceEntry->sourceAddr[0];
						}
#ifdef SUPPORT_MLD_SNOOPING
						else
						{
							tmpIdx = querySrcNum*4;
							querySrcList[tmpIdx+0] = sourceEntry->sourceAddr[0];
							querySrcList[tmpIdx+1] = sourceEntry->sourceAddr[1];
							querySrcList[tmpIdx+2] = sourceEntry->sourceAddr[2];
							querySrcList[tmpIdx+3] = sourceEntry->sourceAddr[3];
							querySrcNum++;
						}
#endif
					}
				}
			}
			else
			{
				newSourceEntry = mcast_allocateSourceEntry();
				if(newSourceEntry==NULL)
				{
					printf("run out of source entry!\n");
					return FALSE;
				}

				if(ipVersion == MULTICAST_TYPE_IPV4)
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
				}
#ifdef SUPPORT_MLD_SNOOPING
				else
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
					newSourceEntry->sourceAddr[1] = srcList[1];
					newSourceEntry->sourceAddr[2] = srcList[2];
					newSourceEntry->sourceAddr[3] = srcList[3];
				}
#endif						

				newSourceEntry->portTimer = portEntry->groupMbrTimer; /*(A-X-Y) = Group Timer*/

				mcast_linkSourceEntry(portEntry, newSourceEntry);

				/* add to query list */
				if (querySrcNum < 10)
				{
					if (sourceEntry->portTimer > igmpConfig.lastMemberQueryIntv)
						sourceEntry->portTimer = igmpConfig.lastMemberQueryIntv;
					
					mcast_LinkSrcToQueryList(portEntry, sourceEntry);
					
					if (ipVersion == MULTICAST_TYPE_IPV4)
					{
						querySrcList[querySrcNum++] = sourceEntry->sourceAddr[0];
					}
#ifdef SUPPORT_MLD_SNOOPING
					else
					{
						tmpIdx = querySrcNum*4;
						querySrcList[tmpIdx+0] = sourceEntry->sourceAddr[0];
						querySrcList[tmpIdx+1] = sourceEntry->sourceAddr[1];
						querySrcList[tmpIdx+2] = sourceEntry->sourceAddr[2];
						querySrcList[tmpIdx+3] = sourceEntry->sourceAddr[3];
						querySrcNum++;
					}
#endif
				}
			}

			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				srcList++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				srcList += 4;
			}
#endif
		}

		//send group and source specified query
		if (querySrcNum > 0)
		{
			portEntry->groupSrcSpecQueryCnt++;
			mcast_send_gss_query(groupEntry, querySrcList, 
								querySrcNum, portEntry->portNum);
		}
		
		//update group timer
		portEntry->groupMbrTimer = igmpConfig.groupMemberIntv;
	}
	else/*means include mode*/
	{
		for(j=0; j<srcNum; j++)
		{
			sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
			if (sourceEntry)
			{
				/* add existed source to query list */
				if (querySrcNum < 10)
				{
					mcast_LinkSrcToQueryList(portEntry, sourceEntry);
					
					if (ipVersion == MULTICAST_TYPE_IPV4)
					{
						querySrcList[querySrcNum++] = sourceEntry->sourceAddr[0];
					}
#ifdef SUPPORT_MLD_SNOOPING
					else
					{
						tmpIdx = querySrcNum*4;
						querySrcList[tmpIdx+0] = sourceEntry->sourceAddr[0];
						querySrcList[tmpIdx+1] = sourceEntry->sourceAddr[1];
						querySrcList[tmpIdx+2] = sourceEntry->sourceAddr[2];
						querySrcList[tmpIdx+3] = sourceEntry->sourceAddr[3];
						querySrcNum++;
					}
#endif
				}
			}
			else
			{
				newSourceEntry = mcast_allocateSourceEntry();
				if(newSourceEntry==NULL)
				{
					printf("run out of source entry!\n");
					return FALSE;
				}

				if(ipVersion == MULTICAST_TYPE_IPV4)
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
				}
#ifdef SUPPORT_MLD_SNOOPING
				else
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
					newSourceEntry->sourceAddr[1] = srcList[1];
					newSourceEntry->sourceAddr[2] = srcList[2];
					newSourceEntry->sourceAddr[3] = srcList[3];
				}
#endif						

				newSourceEntry->portTimer = 0;

				mcast_linkSourceEntry(portEntry, newSourceEntry);
			}

			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				srcList++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				srcList += 4;
			}
#endif
		}
		//update group timer
		portEntry->groupMbrTimer = igmpConfig.groupMemberIntv;

		//send group and source specified query
		if (querySrcNum > 0)
		{
			portEntry->groupSrcSpecQueryCnt++;
			mcast_send_gss_query(groupEntry, querySrcList, 
								querySrcNum, portEntry->portNum);
		}
	}

	return TRUE;
}

int32 mcast_addAllowSrc(igmp_group_entry_t *groupEntry, 
						igmp_port_entry_t *portEntry, 
						uint32 ipVersion, 
						uint32 *srcList, 
						uint16 srcNum)
{
	igmp_source_entry_t *sourceEntry, *newSourceEntry;
	uint32 j;

	for(j=0; j<srcNum; j++)
	{
		sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
		if(sourceEntry==NULL)
		{
			newSourceEntry = mcast_allocateSourceEntry();
			if(newSourceEntry==NULL)
			{
				printf("run out of source entry!\n");
				return FAIL;
			}

			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				newSourceEntry->sourceAddr[0] = srcList[0];
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{	
				newSourceEntry->sourceAddr[0] = srcList[0];
				newSourceEntry->sourceAddr[1] = srcList[1];
				newSourceEntry->sourceAddr[2] = srcList[2];
				newSourceEntry->sourceAddr[3] = srcList[3];
			}
#endif                          
			newSourceEntry->portTimer = igmpConfig.groupMemberIntv;
			
			mcast_linkSourceEntry(portEntry, newSourceEntry);
		}

		if(ipVersion == MULTICAST_TYPE_IPV4)
		{	
			srcList++;
		}
#ifdef SUPPORT_MLD_SNOOPING
		else
		{
			srcList += 4;
		}
#endif
	}				
}

int32 mcast_addBlockSrc(igmp_group_entry_t *groupEntry, 
						igmp_port_entry_t *portEntry, 
						uint32 ipVersion, 
						uint32 *srcList, 
						uint16 srcNum)
{
	igmp_source_entry_t *sourceEntry, *newSourceEntry;
	uint32 querySrcList[40];//max to 10 source should be inserted to gss query.
	uint32 querySrcNum=0, tmpIdx;
	uint32 leaveMode;
	uint32 j;

	leaveMode = mcast_mcFastLeave_get();
	
	if(portEntry->groupMbrTimer > 0) /*means exclude mode*/
	{
		for(j=0; j<srcNum; j++)
		{
			sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
		
			if(sourceEntry==NULL)
			{
				newSourceEntry = mcast_allocateSourceEntry();
				if(newSourceEntry==NULL)
				{
					printf("run out of source entry!\n");
					return FAIL;
				}

				if(ipVersion == MULTICAST_TYPE_IPV4)
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
				}
#ifdef SUPPORT_MLD_SNOOPING
				else
				{	
					newSourceEntry->sourceAddr[0] = srcList[0];
					newSourceEntry->sourceAddr[1] = srcList[1];
					newSourceEntry->sourceAddr[2] = srcList[2];
					newSourceEntry->sourceAddr[3] = srcList[3];
				}
#endif                          
				newSourceEntry->portTimer = portEntry->groupMbrTimer; /*(A-X-Y) = Group Timer*/
				
				mcast_linkSourceEntry(portEntry, newSourceEntry);

				/* add to query list */
				if (querySrcNum < 10)
				{
					if (sourceEntry->portTimer > igmpConfig.lastMemberQueryIntv)
						sourceEntry->portTimer = igmpConfig.lastMemberQueryIntv;
					
					mcast_LinkSrcToQueryList(portEntry, sourceEntry);
					
					if (ipVersion == MULTICAST_TYPE_IPV4)
					{
						querySrcList[querySrcNum++] = sourceEntry->sourceAddr[0];
					}
#ifdef SUPPORT_MLD_SNOOPING
					else
					{
						tmpIdx = querySrcNum*4;
						querySrcList[tmpIdx+0] = sourceEntry->sourceAddr[0];
						querySrcList[tmpIdx+1] = sourceEntry->sourceAddr[1];
						querySrcList[tmpIdx+2] = sourceEntry->sourceAddr[2];
						querySrcList[tmpIdx+3] = sourceEntry->sourceAddr[3];
						querySrcNum++;
					}
#endif
				}
			}
			else
			{
				/* add active source to query list */
				if (sourceEntry->portTimer > 0)
				{
					if (querySrcNum < 10)
					{
						if (sourceEntry->portTimer > igmpConfig.lastMemberQueryIntv)
							sourceEntry->portTimer = igmpConfig.lastMemberQueryIntv;
						
						mcast_LinkSrcToQueryList(portEntry, sourceEntry);
						
						if (ipVersion == MULTICAST_TYPE_IPV4)
						{
							querySrcList[querySrcNum++] = sourceEntry->sourceAddr[0];
						}
#ifdef SUPPORT_MLD_SNOOPING
						else
						{
							tmpIdx = querySrcNum*4;
							querySrcList[tmpIdx+0] = sourceEntry->sourceAddr[0];
							querySrcList[tmpIdx+1] = sourceEntry->sourceAddr[1];
							querySrcList[tmpIdx+2] = sourceEntry->sourceAddr[2];
							querySrcList[tmpIdx+3] = sourceEntry->sourceAddr[3];
							querySrcNum++;
						}
#endif
					}
				}
				
				if (IGMP_LEAVE_MODE_FAST == leaveMode)
				{
					sourceEntry->portTimer = 0;	
				}
			}

			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				srcList++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				srcList += 4;
			}
#endif
		}
		//send group and source specified query
		if (querySrcNum > 0)
		{
			portEntry->groupSrcSpecQueryCnt++;
			mcast_send_gss_query(groupEntry, querySrcList, 
								querySrcNum, portEntry->portNum);
		}
	}
	else/*means include mode*/
	{
		for(j=0; j<srcNum; j++)
		{
			sourceEntry = mcast_searchSourceEntry(ipVersion, srcList, portEntry);
			if(sourceEntry != NULL)
			{
				/* add existed source to query list */
				if (querySrcNum < 10)
				{
					mcast_LinkSrcToQueryList(portEntry, sourceEntry);
					
					if (ipVersion == MULTICAST_TYPE_IPV4)
					{
						querySrcList[querySrcNum++] = sourceEntry->sourceAddr[0];
					}
#ifdef SUPPORT_MLD_SNOOPING
					else
					{
						tmpIdx = querySrcNum*4;
						querySrcList[tmpIdx+0] = sourceEntry->sourceAddr[0];
						querySrcList[tmpIdx+1] = sourceEntry->sourceAddr[1];
						querySrcList[tmpIdx+2] = sourceEntry->sourceAddr[2];
						querySrcList[tmpIdx+3] = sourceEntry->sourceAddr[3];
						querySrcNum++;
					}
#endif
				}
				
				if (IGMP_LEAVE_MODE_FAST == leaveMode)
				{
					sourceEntry->portTimer = 0;
				}
			}

			if(ipVersion == MULTICAST_TYPE_IPV4)
			{	
				srcList++;
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				srcList += 4;
			}
#endif					
		}
		//send group and source specified query
		if (querySrcNum > 0)
		{
			portEntry->groupSrcSpecQueryCnt++;
			mcast_send_gss_query(groupEntry, querySrcList, 
								querySrcNum, portEntry->portNum);
		}
	}
}

int32 mcast_add_source(uint32 port, igmpv3_mode_t igmpv3_mode, 
						uint32 ipVersion, uint16 vid, 
						uint32 *groupAddr, igmp_version_t igmpVersion, 
						uint32 * srcList, uint16 srcNum
						)
{
	igmp_group_entry_t *groupEntry;
	igmp_port_entry_t *portEntry;
	igmp_source_entry_t *sourceEntry;
	uint32 sourceTimer;
	uint32 leaveMode;

	groupEntry = mcast_search_group_entry(ipVersion, vid, groupAddr);
	if (NULL == groupEntry)
		return FALSE;

	portEntry = groupEntry->portEntry[port];
	if (NULL == portEntry)
		return FALSE;

	switch(igmpv3_mode)
	{
		case MODE_IS_EXCLUDE:
			mcast_addIsExcludeSrc(groupEntry, portEntry, ipVersion, srcList, srcNum);
			break;
		case MODE_IS_INCLUDE:
			mcast_addIsIncludeSrc(groupEntry, portEntry, ipVersion, srcList, srcNum);
			break;
		case CHANGE_TO_INCLUDE_MODE:
			mcast_addToIncludeSrc(groupEntry, portEntry, ipVersion, srcList, srcNum);
			break;
		case CHANGE_TO_EXCLUDE_MODE:
			mcast_addToExcludeSrc(groupEntry, portEntry, ipVersion, srcList, srcNum);
			break;
		case ALLOW_NEW_SOURCES:
			mcast_addAllowSrc(groupEntry, portEntry, ipVersion, srcList, srcNum);
			break;
		case BLOCK_OLD_SOURCES:
			mcast_addBlockSrc(groupEntry, portEntry, ipVersion, srcList, srcNum);
			break;
		default:
			break;
	}

	return TRUE;
}

/*Add IGMPv12 Report Group address*/
int32 mcast_add_group(uint32 port,
					uint32 ipVersion, uint16  vid, uint8* mac,
				 	uint32 *groupAddr, igmp_version_t igmpVersion)
{
	igmp_group_entry_t *groupEntry = NULL;
	igmp_group_entry_t *newGroupEntry = NULL;
	igmp_port_entry_t * portEntry = NULL;
	igmp_port_entry_t * newPortEntry = NULL;
	
	if(FALSE == IsValidLgcLanPort(port))
		return FALSE;
	
	groupEntry = mcast_search_group_entry(ipVersion, vid, groupAddr);
	if(groupEntry == NULL)   /*means new group address, create new group entry*/
	{
		newGroupEntry = mcast_allocate_group_entry();
		
		if(newGroupEntry == NULL)
		{
			SYS_DBG("run out of group entry!\n");
			return FALSE;
		}

#ifdef SUPPORT_MLD_SNOOPING
		newGroupEntry->groupAddr[0] = groupAddr[0];
		newGroupEntry->groupAddr[1] = groupAddr[1];
		newGroupEntry->groupAddr[2] = groupAddr[2];
		newGroupEntry->groupAddr[3] = groupAddr[3];
#else
		newGroupEntry->groupAddr[0] = groupAddr[0];
#endif

		newGroupEntry->ipVersion = ipVersion;
		newGroupEntry->vid = vid;

		if(mac != NULL)
		{
			newGroupEntry->mac[0] = mac[0];
			newGroupEntry->mac[1] = mac[1];
			newGroupEntry->mac[2] = mac[2];
			newGroupEntry->mac[3] = mac[3];
			newGroupEntry->mac[4] = mac[4];
			newGroupEntry->mac[5] = mac[5];
		}
		
		if(MULTICAST_TYPE_IPV4 == ipVersion)
		{
			mcast_link_group_entry(newGroupEntry, ipv4_group_hashTable);
		}
	#ifdef SUPPORT_MLD_SNOOPING
		else
		{
			mcast_link_group_entry(newGroupEntry, ipv6_group_hashTable);
		}
	#endif
		groupEntry = newGroupEntry;	
		SYS_DBG("Join Port[%x] to NEW Group IP["IPADDR_PRINT"]\n", port, IPADDR_PRINT_ARG(groupEntry->groupAddr[0]));
	}

	portEntry = groupEntry->portEntry[port];
	if(portEntry == NULL)
	{
		newPortEntry = mcast_allocate_port_entry();
		if(newPortEntry == NULL)
		{
			SYS_DBG("run out of port entry!\n");
			return FALSE;
		}

		newPortEntry->portNum = port;
		newPortEntry->igmpVersion = igmpVersion;
		
		portEntry = newPortEntry;
		groupEntry->portEntry[port] = newPortEntry;
		groupEntry->fwdPort |= 1 << newPortEntry->portNum;

		//update timer
		portEntry->groupMbrTimer = igmpConfig.groupMemberIntv;
		portEntry->groupSpecQueryCnt = 0;
		portEntry->groupSrcSpecQueryCnt = 0;
	}
	else
	{
		if(igmpVersion > portEntry->igmpVersion)
			portEntry->igmpVersion = igmpVersion;
		
		/*If the group version is igmpv2 or mldv1, should update group timer*/
		if((MULTICAST_TYPE_IPV4 == ipVersion) && (portEntry->igmpVersion <= IGMP_VERSION_V2))
		{			
			portEntry->groupMbrTimer = igmpConfig.groupMemberIntv;
		}
		else if((MULTICAST_TYPE_IPV6 == ipVersion) && (portEntry->igmpVersion == MLD_VERSION_V1))
		{
			portEntry->groupMbrTimer = igmpConfig.groupMemberIntv;
		}
	}
	
	SYS_DBG("Join Port[%x] to Group IP["IPADDR_PRINT"] fwdPort[%x] vid[%d]\n", port, IPADDR_PRINT_ARG(groupEntry->groupAddr[0]), groupEntry->fwdPort,vid);

	return TRUE;
}

int32 mcast_add_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, 
										uint8* mac, uint32 *groupAddr)
{
	igmp_version_t version;
	igmp_group_entry_t *groupEntry;
	int32 ret;
	
	if(MULTICAST_TYPE_IPV4 == ipVersion)
		version = IGMP_VERSION_V2;
	else
		version = MLD_VERSION_V1;

	ret = mcast_add_group(port, ipVersion, vid, mac, groupAddr, version);

	//hardware table update
	groupEntry = mcast_search_group_entry(ipVersion, vid, groupAddr);
	if (NULL != groupEntry)
	{
		mcast_hw_groupMbrPort_add_wrapper(groupEntry->ipVersion, port, groupEntry->vid, groupEntry->groupAddr);
	}
	
	return ret;
}

int32 mcast_del_group(uint32 port,uint32 ipVersion, uint16  vid, 
							uint8* mac, uint32 *groupAddr)
{
	igmp_group_entry_t* groupEntry = NULL;
	igmp_port_entry_t * portEntry = NULL;

	if(FALSE == IsValidLgcLanPort(port))
		return FALSE;
	
	groupEntry = mcast_search_group_entry(ipVersion, vid, groupAddr);
	if(groupEntry != NULL)
	{
		SYS_DBG("Leave Port[%x] for Group IP["IPADDR_PRINT"] fwdPort[%x] vid[%d]\n", port, IPADDR_PRINT_ARG(groupEntry->groupAddr[0]), groupEntry->fwdPort,vid);
	
		portEntry = groupEntry->portEntry[port];
		if(portEntry != NULL)
		{
			/* in ctc mode, igmp group deletion is controlled by olt */
			if((IGMP_LEAVE_MODE_FAST == mcast_mcFastLeave_get()) || (IGMP_MODE_CTC == mcast_mcMode_get()))
			{
				mcast_delete_port_entry(groupEntry, portEntry);
				groupEntry->fwdPort &= ~(1 << portEntry->portNum);
				SYS_DBG("Delete Port[%x] for Group IP["IPADDR_PRINT"] fwdPort[%x]\n", port, IPADDR_PRINT_ARG(groupEntry->groupAddr[0]), groupEntry->fwdPort);

				//hardware table update
				mcast_hw_groupMbrPort_del_wrapper(groupEntry->ipVersion, port, groupEntry->vid, groupEntry->groupAddr);
			}
			else
			{
				/* ONU will send igmp specific query packet if it is elected to be a querier*/
				portEntry->groupSpecQueryCnt++;
				mcast_send_gs_query(groupEntry, portEntry->portNum);

				/* fix error: if we receive the igmp leave reports continuously,
				   the groupMbrTimer will set to (query count * query interval) again and not timeout */
				if(portEntry->groupMbrTimer > igmpConfig.lastMemberQueryCnt * igmpConfig.lastMemberQueryIntv)
				{
					/* set group timer to (query count * query interval) */
					portEntry->groupMbrTimer = igmpConfig.lastMemberQueryCnt * igmpConfig.lastMemberQueryIntv;
				}
			}

		}
		
		if(groupEntry->fwdPort == 0)
		{
			SYS_DBG("Delete Group IP["IPADDR_PRINT"] fwdPort[%x]\n", IPADDR_PRINT_ARG(groupEntry->groupAddr[0]), groupEntry->fwdPort);
			if(MULTICAST_TYPE_IPV4 == ipVersion)
			{
				mcast_delete_group_entry(groupEntry,ipv4_group_hashTable);
			}
		#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				mcast_delete_group_entry(groupEntry, ipv6_group_hashTable);
			}
		#endif
		}
	}

	return TRUE;
}

int32 mcast_del_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, 
										uint8* mac, uint32 *groupAddr)
{
	return mcast_del_group(port, ipVersion, vid, mac, groupAddr);
}

int32 mcast_update_igmpv3_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, uint8* mac, 
													uint32 *groupAddr, igmpv3_mode_t igmpv3_mode, 
													uint32 * srcList, uint16 srcNum)
{
	igmp_version_t version;
	igmp_group_entry_t *groupEntry;


	if(MULTICAST_TYPE_IPV4 == ipVersion)
		version = IGMP_VERSION_V3;
	else
		version = MLD_VERSION_V2;

	mcast_add_group(port, ipVersion, vid, mac, groupAddr, version);
	
	mcast_add_source(port, igmpv3_mode, ipVersion, 
						vid, groupAddr, 
						version, srcList, 
						srcNum);

	//hardware table update
	groupEntry = mcast_search_group_entry(ipVersion, vid, groupAddr);
	if ((NULL != groupEntry) && (NULL != groupEntry->portEntry[port]))
	{
		if((igmpv3_mode == CHANGE_TO_INCLUDE_MODE) && (srcNum == 0))
		{
			/*ToInclude {} means to leave group, not add to hw l2 table*/
		}
		else
		{
			mcast_hw_groupMbrPort_add_wrapper(groupEntry->ipVersion, port, groupEntry->vid, groupEntry->groupAddr);
		}
	}
	return TRUE;
}

//forward related start
uint32 mcast_get_group_fwdPortMask(igmp_group_entry_t * groupEntry)
{
	if(groupEntry == NULL)
		return 0;
	
	return groupEntry->fwdPort;
}

//Fixed search way: use group ip and vid to search
int32 mcast_get_data_fwd_info(mcast_data_info_t * data_info, mcast_fwd_info_t * fwd_info)
{
	igmp_group_entry_t * groupEntry = NULL;
	
	if(data_info == NULL)
		return FAIL;
	
	if(fwd_info == NULL)
		return FAIL;

	SYS_DBG("%s %d \n", __FUNCTION__, __LINE__);
	memset(fwd_info, 0, sizeof(mcast_fwd_info_t));
	
	groupEntry = mcast_search_group_entry(data_info->ipVersion, data_info->vid, data_info->groupAddr); 
	
	if(groupEntry == NULL)
	{
		fwd_info->unknownMCast = TRUE;
		fwd_info->fwdPortMask = igmpConfig.unknownMCastFloodMap;
		if(fwd_info->fwdPortMask != 0)
			return SUCCESS;
	}
	else
	{
		
		fwd_info->fwdPortMask = mcast_get_group_fwdPortMask(groupEntry);	

		return SUCCESS;
	
	}
	return FAIL;
}

//forward related end

//timer related start
static uint32 mcast_getClientFwdPortMask(igmp_port_entry_t *portEntry)
{
	uint32 portMask=(1<<portEntry->portNum);
	uint32 fwdPortMask=0;

	igmp_source_entry_t *sourcePtr=NULL;;
	
	if(portEntry->groupMbrTimer > 0) /*exclude mode never expired*/
	{
		fwdPortMask |= portMask;
	}
	else/*include mode*/
	{
		sourcePtr = portEntry->sourceList;
		while(sourcePtr!=NULL)
		{
			if(TRUE == sourcePtr->fwdState)
			{
				fwdPortMask |= portMask;
				break;
			}
			sourcePtr=sourcePtr->next;
		}
		
	}

	return fwdPortMask;
}

static void mcast_check_source_timer(igmp_port_entry_t *portEntry, igmp_source_entry_t *sourceEntry)
{
	uint8 oldFwdState, newFwdState;
	uint8 deleteFlag=FALSE;

	oldFwdState = sourceEntry->fwdState;

	if (sourceEntry->portTimer == 0)//source timeout
	{
		if (portEntry->groupMbrTimer == 0)/* include mode */
			deleteFlag = TRUE;

		sourceEntry->fwdState = FALSE;
	}
	else
	{
		sourceEntry->fwdState = TRUE;
		sourceEntry->portTimer -= PASS_SECONDS;
	}

	if ((portEntry->groupSrcSpecQueryCnt >= 1) && 
		(portEntry->groupSrcSpecQueryCnt < igmpConfig.lastMemberQueryCnt))
	{
		/* this source entry should not be deleted */
	}
	else
	{
		if (TRUE == deleteFlag)
		{
			mcast_deleteSourceEntry(portEntry, sourceEntry);
		}
	}
}

static void mcast_check_port_entry_timer(igmp_group_entry_t * groupEntry , igmp_port_entry_t * portEntry)
{
	igmp_source_entry_t *sourceEntry = portEntry->sourceList;
	igmp_source_entry_t *nextSourceEntry;
	uint32 queryFlag = 0;
	uint32 newFwdPortMask = 0;

	while (sourceEntry)
	{
		nextSourceEntry = sourceEntry->next;
		mcast_check_source_timer(portEntry, sourceEntry);
		sourceEntry = nextSourceEntry;
	}

	if(portEntry->groupMbrTimer > 0) /*exclude mode never expired*/
		portEntry->groupMbrTimer -= PASS_SECONDS;

	if ((portEntry->groupSrcSpecQueryCnt >= 1) && 
		(portEntry->groupSrcSpecQueryCnt < igmpConfig.lastMemberQueryCnt))
	{
		uint32 ipType;
		uint32 querySrcList[40];//max to 10 source should be inserted to gss query.
		uint32 querySrcNum=0, tmpIdx;

		ipType = groupEntry->ipVersion;
		
		sourceEntry = portEntry->querySrcList;
		while (sourceEntry != NULL)
		{
			if (ipType == MULTICAST_TYPE_IPV4)
			{
				querySrcList[querySrcNum++] = sourceEntry->sourceAddr[0];
			}
#ifdef SUPPORT_MLD_SNOOPING
			else
			{
				tmpIdx = querySrcNum*4;
				querySrcList[tmpIdx+0] = sourceEntry->sourceAddr[0];
				querySrcList[tmpIdx+1] = sourceEntry->sourceAddr[1];
				querySrcList[tmpIdx+2] = sourceEntry->sourceAddr[2];
				querySrcList[tmpIdx+3] = sourceEntry->sourceAddr[3];
				querySrcNum++;
			}
#endif
			sourceEntry = sourceEntry->qnxt;
		}

		if (querySrcNum > 0)
		{
			queryFlag = 1;
			
			portEntry->groupSrcSpecQueryCnt++;
			mcast_send_gss_query(groupEntry, querySrcList, 
								querySrcNum, portEntry->portNum);
		}
	}
	
	if ((portEntry->groupSpecQueryCnt >= 1) && 
			(portEntry->groupSpecQueryCnt < igmpConfig.lastMemberQueryCnt))
	{
		queryFlag = 1;
		
		portEntry->groupSpecQueryCnt++;
		mcast_send_gs_query(groupEntry, portEntry->portNum);
	}
	
	if (!queryFlag)
	{
		if (portEntry->groupSrcSpecQueryCnt >= igmpConfig.lastMemberQueryCnt)
		{
			portEntry->groupSrcSpecQueryCnt = 0;
			mcast_ClearQueryList(portEntry);
		}
		if (portEntry->groupSpecQueryCnt >= igmpConfig.lastMemberQueryCnt)
		{
			portEntry->groupSpecQueryCnt = 0;
		}
		
		newFwdPortMask = mcast_getClientFwdPortMask(portEntry);
		if (0 == newFwdPortMask)
		{
			int port = portEntry->portNum;
			SYS_DBG("Port[%d] Group Timeout of IP = "IPADDR_PRINT"\n", portEntry->portNum, IPADDR_PRINT_ARG(groupEntry->groupAddr[0]));
			groupEntry->fwdPort &= ~(1 << portEntry->portNum);
			mcast_delete_port_entry(groupEntry, portEntry);

			//hardware table update
			mcast_hw_groupMbrPort_del_wrapper(groupEntry->ipVersion, port, groupEntry->vid, groupEntry->groupAddr);
		}
	}
}

static void mcast_check_group_entry_timer(igmp_group_entry_t * groupEntry, igmp_group_entry_t ** hashTable)
{
	igmp_port_entry_t * portEntry;
	int i;
	
	FOR_EACH_LAN_PORT(i)
	{
		portEntry = groupEntry->portEntry[i];
		if(portEntry != NULL)
		{
			mcast_check_port_entry_timer(groupEntry, portEntry);
		}
	}
	
	if(groupEntry->fwdPort == 0) /*none active port*/
	{
		SYS_DBG("Group Timeout of IP = "IPADDR_PRINT"\n", IPADDR_PRINT_ARG(groupEntry->groupAddr[0]));
		mcast_delete_group_entry(groupEntry,hashTable);	
	}
}

int32 mcast_maintain_group_timer()
{
	uint32 i;
	uint32 size;
	igmp_group_entry_t * groupEntryPtr, *nextEntry;
	
	/*maintain ipv4 group entry  timer */
	size = MCAST_GROUP_HASH_NUM;
	for(i = 0; i < size; i++)
	{
		if (ENABLED == mcast_timer_stop)
       	{
            return SYS_ERR_OK;
       	}
		/*scan the hash table*/
		groupEntryPtr = ipv4_group_hashTable[i];
		while(groupEntryPtr)              /*traverse each group list*/
		{
			nextEntry = groupEntryPtr->next; 
			
			mcast_check_group_entry_timer(groupEntryPtr, ipv4_group_hashTable);
			groupEntryPtr = nextEntry;/*because expired group entry  will be cleared*/
		}
	}

#ifdef SUPPORT_MLD_SNOOPING
	size = MCAST_GROUP_HASH_NUM;
	for(i = 0; i < size; i++)
	{
		if (ENABLED == mcast_timer_stop)
       	{
            return SYS_ERR_OK;
       	}
		/*scan the hash table*/				
		groupEntryPtr = ipv6_group_hashTable[i];
		while(groupEntryPtr)              /*traverse each group list*/
		{	
			nextEntry = groupEntryPtr->next; 
			
			mcast_check_group_entry_timer(groupEntryPtr, ipv6_group_hashTable);
			groupEntryPtr = nextEntry; /*because expired group entry  will be cleared*/
		}
	}
#endif
}
//timer related end

uint8 isEnableIgmpSnooping()
{
	return igmpConfig.enableSnooping;
}

igmp_config_t *getIgmpConfig(void)
{
	return &igmpConfig;
}

void mcast_fastpath_read()
{
	uint32 i, j;
	uint32 size;
	igmp_group_entry_t * groupEntryPtr;
	
	printf("************************igmp ipv4 fast table***************************\n");
	printf("hashIdx  TimeOut  Vid      GroupMac        GroupIp   fwdPort\n");
	/*maintain ipv4 group entry  timer */
	size = MCAST_GROUP_HASH_NUM;
	for(i = 0; i < size; i++)
	{
		/*scan the hash table*/				
		groupEntryPtr = ipv4_group_hashTable[i];
		while(groupEntryPtr)              /*traverse each group list*/
		{	 
			uint32 timer = 0;
			FOR_EACH_LAN_PORT(j)
			{
				if(groupEntryPtr->portEntry[j])
				{
					if(timer < groupEntryPtr->portEntry[j]->groupMbrTimer)
						timer = groupEntryPtr->portEntry[j]->groupMbrTimer;
				}
			}
			printf("[%-3d]     %-7d %-3d "MAC_PRINT"   "IPADDR_PRINT"     %-5x\n", i, timer, groupEntryPtr->vid,
				  MAC_PRINT_ARG(groupEntryPtr->mac),IPADDR_PRINT_ARG(groupEntryPtr->groupAddr[0]),
				  groupEntryPtr->fwdPort);
			groupEntryPtr = groupEntryPtr->next;
		}
	}

#ifdef SUPPORT_MLD_SNOOPING
	printf("\n************************igmp ipv6 fast table*******************************\n");
	printf("hashIdx  TimeOut  Vid      GroupMac               GroupIp            fwdPort\n");
	/*maintain ipv4 group entry  timer */
	size = MCAST_GROUP_HASH_NUM;
	for(i = 0; i < size; i++)
	{
		/*scan the hash table*/				
		groupEntryPtr = ipv6_group_hashTable[i];
		while(groupEntryPtr)              /*traverse each group list*/
		{	 
			uint32 timer = 0;
			uint8  * ip;
			FOR_EACH_LAN_PORT(j)
			{
				if(groupEntryPtr->portEntry[j])
				{
					if(timer < groupEntryPtr->portEntry[j]->groupMbrTimer)
						timer = groupEntryPtr->portEntry[j]->groupMbrTimer;
				}
			}
			ip = (uint8 *)groupEntryPtr->groupAddr;
			printf("[%-3d]     %-7d %-3d "MAC_PRINT"   "IPADDRV6_PRINT"     %-5x\n", i, timer, groupEntryPtr->vid,
				  MAC_PRINT_ARG(groupEntryPtr->mac),IPADDRV6_PRINT_ARG(ip),
				  groupEntryPtr->fwdPort);
			groupEntryPtr = groupEntryPtr->next;
		}
	}
#endif
	mcast_igmp_setting_show();
	mcast_igmp_querier_show();
}

/* directly called by ctc oam */
#define MAX_MC_GROUP_NUM  ((MAX_PORT_NUM)*(MAX_GROUP_NUM_PER_PORT))
int igmpMcCtrlGrpEntryGet(oam_mcast_control_entry_t *ctl_entry_list, int *num)
{
	int i,j;
	int number = 0;
	oam_mcast_control_entry_t *ptr_entry=NULL;
	igmp_group_entry_t * groupEntryPtr;
	mc_control_e ctrlType;
	
	if(ctl_entry_list==NULL || num==NULL)
	{
		return SYS_ERR_FAILED;
	}

	ctrlType = mcast_mcCtrl_type_get(MULTICAST_TYPE_IPV4);
	ptr_entry = ctl_entry_list;
	
	IGMP_SEM_LOCK(igmpDbSem);
	
	for(i = 0; i < MCAST_GROUP_HASH_NUM; i++)
	{
		/*scan the hash table*/				
		groupEntryPtr = ipv4_group_hashTable[i];
		while(groupEntryPtr)    /*traverse each group list*/
		{	 
			FOR_EACH_LAN_PORT(j)
			{
				if(groupEntryPtr->portEntry[j])
				{
					if(ctrlType == MC_CTRL_GDA_MAC)
					{
						memcpy(ptr_entry->gda, groupEntryPtr->mac, MAC_ADDR_LEN);
						ptr_entry->vlan_id = 0;
					}
					else if(ctrlType == MC_CTRL_GDA_MAC_VID)
					{
						memcpy(ptr_entry->gda, groupEntryPtr->mac, MAC_ADDR_LEN);
						ptr_entry->vlan_id = groupEntryPtr->vid;
					}
					else if(ctrlType == MC_CTRL_GDA_GDA_IP_VID)
					{
						ptr_entry->gda[0] = 0;
						ptr_entry->gda[1] = 0;
						memcpy(&ptr_entry->gda[2], (uint8 *)(&groupEntryPtr->groupAddr[0]), MAC_ADDR_LEN);
						ptr_entry->vlan_id = groupEntryPtr->vid;
					}
					else
					{
						//not supportted control type
						goto Out;
					}
					ptr_entry->port_id = j + CTC_START_PORT_NUM;
					
					number++;
					ptr_entry++;

					if(number >= MAX_MC_GROUP_NUM)
					{
						goto Out;
					}
				}
			}

			groupEntryPtr = groupEntryPtr->next;
		}
		
	}

Out:		
	*num = number > MAX_MC_GROUP_NUM ? MAX_MC_GROUP_NUM:number;
	
	IGMP_SEM_UNLOCK(igmpDbSem);
	return SYS_ERR_OK;
}

// allocate a port entry from the port entry pool
static igmp_port_entry_t* mcast_allocate_port_entry(void)
{
	igmp_port_entry_t *ret = NULL;

	IGMP_SEM_LOCK(igmpDbSem);
	
	if (portEntryPool != NULL)
	{
		ret = portEntryPool;
		if(portEntryPool->next!=NULL)
		{
			portEntryPool->next->previous=NULL;
		}
		portEntryPool = portEntryPool->next;
		memset((void *)ret, 0, sizeof(igmp_port_entry_t));
	}
	
	IGMP_SEM_UNLOCK(igmpDbSem);	
	
	return ret;
}

// free a port entry and link it back to the port entry pool, default is link to the pool head
static void mcast_free_port_entry(igmp_port_entry_t* portEntryPtr) 
{
	if (!portEntryPtr)
	{
		return;
	}
		
	IGMP_SEM_LOCK(igmpDbSem);
	
	portEntryPtr->next = portEntryPool;
	if(portEntryPool!=NULL)
	{
		portEntryPool->previous = portEntryPtr;
	}
	portEntryPool = portEntryPtr;
	
	IGMP_SEM_UNLOCK(igmpDbSem);	
}

static void mcast_delete_port_entry(igmp_group_entry_t * groupEntry, igmp_port_entry_t *portEntry)
{	
	if(NULL == portEntry)
		return;
	
	if(NULL == groupEntry)
		return;

	/* source entry should be deleted firstly */
	mcast_clearSourceList(portEntry);
	
	groupEntry->portEntry[portEntry->portNum] = NULL;
	mcast_free_port_entry(portEntry);
	
	return;			
}

static void mcast_delete_port_list(igmp_group_entry_t * groupEntry)
{
	igmp_port_entry_t * portEntry=NULL;
	int i;
	
	if(NULL == groupEntry)
	{
		return;
	}
	
	FOR_EACH_LAN_PORT(i)
	{
		portEntry = groupEntry->portEntry[i];
		if(portEntry != NULL)
		{
			mcast_delete_port_entry(groupEntry, portEntry);
			//hardware table update
			mcast_hw_groupMbrPort_del_wrapper(groupEntry->ipVersion, i, groupEntry->vid, groupEntry->groupAddr);
		}
	}
}

// allocate a group entry from the group entry pool
static igmp_group_entry_t* mcast_allocate_group_entry(void)
{
	igmp_group_entry_t *ret = NULL;

	IGMP_SEM_LOCK(igmpDbSem);
	
	if (groupEntryPool != NULL)
	{
		ret = groupEntryPool;
		if(groupEntryPool->next != NULL)
		{
			groupEntryPool->next->previous = NULL;
		}
		groupEntryPool = groupEntryPool->next;
		memset((void*)ret, 0, sizeof(igmp_group_entry_t));
	}
		
	IGMP_SEM_UNLOCK(igmpDbSem);	
	
	return ret;
}

/* link group Entry in the front of a group list */
static void  mcast_link_group_entry(igmp_group_entry_t *groupEntry, igmp_group_entry_t ** hashTable)
{
	uint32 hashIndex=0;
	
	if(NULL == groupEntry)
	{
		return;
	}
	
	IGMP_SEM_LOCK(igmpDbSem); /* lock resource*/
	
	hashIndex = mcast_hash_algorithm(groupEntry->ipVersion, groupEntry->groupAddr,groupEntry->vid);
	if(hashTable[hashIndex] != NULL)
	{
		hashTable[hashIndex]->previous = groupEntry;
	}
	groupEntry->next = hashTable[hashIndex];
	hashTable[hashIndex] = groupEntry;
	hashTable[hashIndex]->previous = NULL;
		
	IGMP_SEM_UNLOCK(igmpDbSem);	/* unlock resource*/
}

/* unlink a group entry from group list */
static void mcast_unlink_group_entry(igmp_group_entry_t* groupEntry,  igmp_group_entry_t ** hashTable)
{	
	uint32 hashIndex=0;
	
	if(NULL == groupEntry)
	{
		return;
	}
	
	IGMP_SEM_LOCK(igmpDbSem);  /* lock resource*/	

	hashIndex = mcast_hash_algorithm(groupEntry->ipVersion, groupEntry->groupAddr,groupEntry->vid);
	/* unlink entry node*/
	if(groupEntry == hashTable[hashIndex]) /*unlink group list head*/
	{
		hashTable[hashIndex] = groupEntry->next;
		if(hashTable[hashIndex] != NULL)
		{
			hashTable[hashIndex]->previous = NULL;
		}
	}
	else
	{
		if(groupEntry->previous != NULL)
		{
			groupEntry->previous->next = groupEntry->next;
		}
		 
		if(groupEntry->next != NULL)
		{
			groupEntry->next->previous = groupEntry->previous;
		}
	}
	
	groupEntry->previous=NULL;
	groupEntry->next=NULL;
	
	IGMP_SEM_UNLOCK(igmpDbSem); /* unlock resource*/
	
}

// free a group entry and link it back to the group entry pool, default is link to the pool head
static void mcast_free_group_entry(igmp_group_entry_t * groupEntry) 
{
	if (NULL == groupEntry)
	{
		return;
	}
		
	IGMP_SEM_LOCK(igmpDbSem);	

	groupEntry->next = groupEntryPool;
	if(groupEntryPool != NULL)
	{
		groupEntryPool->previous = groupEntry;
	}
	groupEntryPool = groupEntry;
	
	IGMP_SEM_UNLOCK(igmpDbSem);	
}

static void mcast_delete_group_entry(igmp_group_entry_t * groupEntry, igmp_group_entry_t ** hashTable)
{	
	if(groupEntry != NULL)
	{		
		mcast_delete_port_list(groupEntry);
		mcast_unlink_group_entry(groupEntry, hashTable);
		mcast_free_group_entry(groupEntry);
	}
		
}

// allocate a source entry from the source entry pool
static igmp_source_entry_t * mcast_allocateSourceEntry(void)
{
	igmp_source_entry_t *ret = NULL;

	IGMP_SEM_LOCK(igmpDbSem);
	
	if (sourceEntryPool != NULL)
	{
		ret = sourceEntryPool;
		if(sourceEntryPool->next != NULL)
		{
			sourceEntryPool->next->previous = NULL;
		}
		sourceEntryPool = sourceEntryPool->next;
		
		memset((void*)ret, 0, sizeof(igmp_source_entry_t));
	}
		
	IGMP_SEM_UNLOCK(igmpDbSem);	
	
	return ret;
}

static igmp_source_entry_t * mcast_searchSourceEntry(uint32 ipVersion, 
											uint32 *sourceAddr, 
											igmp_port_entry_t *portEntry)
{
	igmp_source_entry_t *sourcePtr = portEntry->sourceList;
	
	while(sourcePtr != NULL)
	{
		if (TRUE == mcast_is_source_equal(sourcePtr->sourceAddr, sourceAddr, ipVersion))
			return sourcePtr;
		
		sourcePtr=sourcePtr->next;
	}

	return NULL;
}

static void mcast_linkSourceEntry(igmp_port_entry_t *portEntry, 
								igmp_source_entry_t *entryNode)
{
	if(NULL == entryNode)
	{
		return;
	}
	
	if(NULL == portEntry)
	{
		return;
	}
	
	IGMP_SEM_LOCK(igmpDbSem);

	if(portEntry->sourceList != NULL)
	{
		portEntry->sourceList->previous = entryNode;
	}
	entryNode->next = portEntry->sourceList;
	portEntry->sourceList=entryNode;
	portEntry->sourceList->previous=NULL;
	
	IGMP_SEM_UNLOCK(igmpDbSem);
}

static void mcast_unlinkSourceEntry(igmp_port_entry_t *portEntry, 
									igmp_source_entry_t *sourceEntry)
{
	if(NULL == sourceEntry)
	{
		return;
	}
	
	if(NULL == portEntry)
	{
		return;
	}
	
	IGMP_SEM_LOCK(igmpDbSem);
	
	/* unlink entry node*/ 
	if(sourceEntry == portEntry->sourceList) /*unlink group list head*/
	{
		portEntry->sourceList = sourceEntry->next;
		if(portEntry->sourceList != NULL)
		{
			portEntry->sourceList->previous=NULL;
		}
	}
	else
	{	
		if(sourceEntry->previous != NULL)
		{
			sourceEntry->previous->next = sourceEntry->next;
		}

		if(sourceEntry->next!=NULL)
		{
			sourceEntry->next->previous = sourceEntry->previous;
		}
	}
	
	sourceEntry->previous=NULL;
	sourceEntry->next=NULL;
	
	IGMP_SEM_UNLOCK(igmpDbSem);
}

static void mcast_clearSourceEntry(igmp_source_entry_t *sourceEntry)
{
	IGMP_SEM_LOCK(igmpDbSem);
	if (NULL != sourceEntry)
	{
		memset(sourceEntry, 0, sizeof(igmp_source_entry_t));
	}
	IGMP_SEM_UNLOCK(igmpDbSem);
}

// free a source entry and link it back to the source entry pool, default is link to the pool head
static void mcast_freeSourceEntry(igmp_source_entry_t *sourceEntry)
{
	if (!sourceEntry)
	{
		return;
	}
		
	IGMP_SEM_LOCK(igmpDbSem);
	sourceEntry->next = sourceEntryPool;
	if(sourceEntryPool != NULL)
	{
		sourceEntryPool->previous = sourceEntry;
	}

	sourceEntryPool = sourceEntry;

	IGMP_SEM_UNLOCK(igmpDbSem);
}

static void mcast_deleteSourceEntry(igmp_port_entry_t *portEntry, 
							igmp_source_entry_t* sourceEntry)
{
	if(portEntry == NULL)
	{
		return;
	}
	
	if(sourceEntry != NULL)
	{
		mcast_unlinkSourceEntry(portEntry, sourceEntry);
		mcast_unlinkSrcFromQueryList(portEntry, sourceEntry);
		mcast_clearSourceEntry(sourceEntry);
		mcast_freeSourceEntry(sourceEntry);
	}
}

//init code start
/*group entry memory management*/
static igmp_group_entry_t * mcast_init_group_entry_pool(void)
{
	uint32 idx=0;
	uint32 poolSize;
	igmp_group_entry_t *poolHead=NULL;
	igmp_group_entry_t *preEntry=NULL, *currEntry;
	
	IGMP_SEM_LOCK(igmpDbSem);	/* Lock resource */
	
	poolHead = groupEntryArray;
	poolSize = MCAST_MAX_GROUP_NUM;
	
	memset((void*)poolHead, 0,  (poolSize  * sizeof(igmp_group_entry_t)));
	currEntry = poolHead;

	/* link the whole group entry pool */
	for (idx = 0 ; idx < poolSize-1 ; idx++, currEntry++)
	{
		currEntry->previous = preEntry;
		currEntry->next = currEntry+1;

		preEntry = currEntry;
	}
	
	currEntry->next = NULL;
	
	IGMP_SEM_UNLOCK(igmpDbSem);	/* UnLock resource */
	return poolHead;
}

static igmp_port_entry_t * mcast_init_port_entry_pool(void)
{
	uint32 idx=0;
	uint32 poolSize;
	igmp_port_entry_t *poolHead=NULL;
	igmp_port_entry_t *preEntry=NULL, *currEntry;
	
	IGMP_SEM_LOCK(igmpDbSem);	/* Lock resource */
	
	poolHead = portEntryArray;
	poolSize = MCAST_MAX_PORT_ENTRY_NUM;
	
	memset((void*)poolHead, 0,  (poolSize  * sizeof(igmp_port_entry_t)));
	currEntry = poolHead;

	/* link the whole group entry pool */
	for (idx = 0 ; idx < poolSize-1 ; idx++, currEntry++)
	{
		currEntry->previous = preEntry;
		currEntry->next = currEntry+1;

		preEntry = currEntry;
	}
	
	currEntry->next = NULL;
	
	IGMP_SEM_UNLOCK(igmpDbSem);	/* UnLock resource */
	return poolHead;
}

static igmp_source_entry_t * mcast_init_source_entry_pool(void)
{
	uint32 idx=0;
	uint32 poolSize;
	igmp_source_entry_t *poolHead=NULL;
	igmp_source_entry_t *preEntry=NULL, *currEntry;
	
	IGMP_SEM_LOCK(igmpDbSem);	/* Lock resource */
	
	poolHead = sourceEntryArray;
	poolSize = MCAST_MAX_SOURCE_ENTRY_NUM;
	
	memset((void*)poolHead, 0,  (poolSize  * sizeof(igmp_source_entry_t)));
	currEntry = poolHead;

	/* link the whole group entry pool */
	for (idx = 0 ; idx < poolSize-1 ; idx++, currEntry++)
	{
		currEntry->previous = preEntry;
		currEntry->next = currEntry+1;

		preEntry = currEntry;
	}
	
	currEntry->next = NULL;
	
	IGMP_SEM_UNLOCK(igmpDbSem);	/* UnLock resource */
	return poolHead;
}


static void mcast_init_hash_table()
{
	uint32 i=0;
	uint32 size = 0;

	IGMP_SEM_LOCK(igmpDbSem);
	
	size = MCAST_GROUP_HASH_NUM;		
	group_hashMask = size -1;		
	for (i = 0 ; i < size ; i++)
	{	
		ipv4_group_hashTable[i] = NULL;
	}

#ifdef SUPPORT_MLD_SNOOPING
	size = MCAST_GROUP_HASH_NUM;				
	for (i = 0 ; i < size ; i++)
	{	
		ipv6_group_hashTable[i] = NULL;
	}
#endif
	
	IGMP_SEM_UNLOCK(igmpDbSem);
}

void mcast_init_igmp_config(igmp_config_t * config)
{
	config->enableSnooping = TRUE;
	config->robustness = 2;
	config->queryIntv = 125;
	config->queryRespIntv = 10;
	config->lastMemberQueryIntv = 1;
	config->responseTime = 10;
	config->lastMemberQueryCnt = config->robustness;
	config->groupMemberIntv = config->robustness * config->queryIntv +  config->queryRespIntv;
	config->otherQuerierPresentInterval = (config->robustness*config->queryIntv) + 
										(config->queryRespIntv/2);
	
	config->unknownMCastFloodMap = 0x0; /* default set 0 */
}

/* To avoid rx packet and timer thread interfere group database processing*/
void mcast_group_rx_timer_stop(void)
{
    mcast_timer_stop = ENABLED;
    igmp_packet_stop = ENABLED;
    usleep(5*1000);
}

/* restore to original status */
void mcast_group_rx_timer_start(void)
{
    mcast_timer_stop = DISABLED;
    igmp_packet_stop = DISABLED;
}

void epon_igmp_group_entry_clear(uint32 ipVersion)
{
	uint32 i;
	uint32 size;
	igmp_group_entry_t * groupEntry, *nextEntry;
	igmp_group_entry_t ** hashTable;

	if(MULTICAST_TYPE_IPV4 == ipVersion)
	{
		hashTable = ipv4_group_hashTable;
	}
#ifdef SUPPORT_MLD_SNOOPING
	else if(MULTICAST_TYPE_IPV6 == ipVersion)
	{
		hashTable = ipv6_group_hashTable;
	}
#endif
	else
		return;

	SYS_DBG("%s clear %s group entry\n", __func__, (MULTICAST_TYPE_IPV4 == ipVersion) ? "ipv4":"ipv6");

	mcast_group_rx_timer_stop();

	size = MCAST_GROUP_HASH_NUM;		
	for(i = 0; i < size; i++)
	{
		/*scan the hash table*/
		groupEntry = hashTable[i];
		
		while(groupEntry)  /*traverse each group list*/
		{
			nextEntry = groupEntry->next; 			
			mcast_delete_group_entry(groupEntry,hashTable);
			groupEntry = nextEntry;
		}
	}

	mcast_group_rx_timer_start();
}

void epon_igmp_groupMbrPort_delFromVlan(uint32 ipVersion, uint16 vid, uint32 port)
{
	uint32 i;
	uint32 size;
	igmp_group_entry_t * groupEntry, *nextEntry;
	igmp_group_entry_t ** hashTable;
	igmp_port_entry_t * portEntry = NULL;
	
	if(MULTICAST_TYPE_IPV4 == ipVersion)
	{
		hashTable = ipv4_group_hashTable;
	}
#ifdef SUPPORT_MLD_SNOOPING
	else if(MULTICAST_TYPE_IPV6 == ipVersion)
	{
		hashTable = ipv6_group_hashTable;
	}
#endif
	else
		return;

	SYS_DBG("%s del %s group entry port[%d] by vid[%d]\n", __func__, 
		(MULTICAST_TYPE_IPV4 == ipVersion) ? "ipv4":"ipv6", port, vid);

	mcast_group_rx_timer_stop();
	
	size = MCAST_GROUP_HASH_NUM;
	for(i = 0; i < size; i++)
	{
		/*scan the hash table*/
		groupEntry = hashTable[i];
		
		while(groupEntry)  /*traverse each group list*/
		{
			nextEntry = groupEntry->next;
			if(groupEntry->vid == vid)
			{
				portEntry = groupEntry->portEntry[port];
				if(portEntry != NULL)
				{
					mcast_delete_port_entry(groupEntry, portEntry);
					groupEntry->fwdPort &= ~(1 << port);
					//hardware table update
					mcast_hw_groupMbrPort_del_wrapper(groupEntry->ipVersion, port, groupEntry->vid, groupEntry->groupAddr);

					if(groupEntry->fwdPort == 0)
					{
						mcast_delete_group_entry(groupEntry,hashTable);
					}
				}
			}
			groupEntry = nextEntry;
		}
	}

	mcast_group_rx_timer_start();
}

void epon_igmp_db_init()
{	
	IGMP_SEM_INIT(igmpDbSem);
	
	groupEntryPool = mcast_init_group_entry_pool();
	portEntryPool = mcast_init_port_entry_pool();
	sourceEntryPool = mcast_init_source_entry_pool();
	mcast_init_hash_table();

	mcast_init_igmp_config(&igmpConfig);
	SYS_DBG("[OAM IGMP:%s:%d] DB init!\n", __FILE__, __LINE__);
}

//init code end

