#include <sys/socket.h>
#include <netpacket/packet.h>
#include <linux/if.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <common/rt_error.h>
#include <sys/ioctl.h>
#include "epon_oam_config.h"
#include "epon_oam_dbg.h"
#include "ctc_oam.h"
#include "drv_convert.h"
#ifdef CONFIG_APOLLO_ROMEDRIVER
#include "rtk_rg_struct.h"
#else
#include <rtk/port.h>
#endif
#ifdef CONFIG_SFU_APP
#include <rtk/acl.h>
#include <common/util/rt_util.h>
#endif
#include "epon_oam_msgq.h"

//#define ENABLE_PORT_PHY_DOWN
#ifdef ENABLE_PORT_PHY_DOWN
#include <hal/phy/phydef.h>
#endif

#define LOOPDETECT_TIME				(5000) /* In unit of millisecond */
#define LOOP_RECOVER_TIME				(300) /* In unit of s */
#define LOOPDETECT_ETHERTYPE		0xfffa
#define LOOPDETECT_VLAN_NUM			100
#define LOOPDETECT_MIN_TIME         20  /* In unit of millisecond */

#ifdef CONFIG_SFU_APP
#define LOOPDETECT_ACL_INDEX 62 /* use fixed index for the acl which traps the loopdetect packets to cpu port */
#endif

struct loopdetect_lan_port
{
	unsigned char loopdetect_enable;//loop detect enable
	unsigned char loop_port_autodown_enable; //loop port autodown enable
	int ifindex;
	unsigned char loop_detected;//port loop status: 1 means loop port 
	int disabletime;//time has been disabled  for autodown loop port, unit(second)
};

struct loopdetect_info
{
	unsigned short ether_type;
	unsigned int loopdetect_time; /* change unit from second to millisecond */
	unsigned int loop_recover_time;
	short loopdetect_vlan[LOOPDETECT_VLAN_NUM];
	struct loopdetect_lan_port lan_if[MAX_PORT_NUM];
};

struct loopdetect_data
{
	unsigned int int_magic;
	unsigned char ifnum;
	unsigned char str_magic[41];	//make data length be 46 bytes
} __attribute__((packed));

struct loopdetect_data_vlan
{
	unsigned short vid;
	unsigned short type;
	struct loopdetect_data data;
} __attribute__((packed));

#ifdef CONFIG_APOLLO_ROMEDRIVER
struct vlan_info_list{
	rtk_rg_cvlan_info_t vlan_info;
	struct vlan_info_list *next;
};
#endif

struct loopdetect_info loopdetectInfo;
pthread_mutex_t loopdetectmutex;
static struct loopdetect_data_vlan send_data = {0};
static struct sockaddr_ll addr={0};
static int send_sock;
#ifdef CONFIG_APOLLO_ROMEDRIVER
static int acl_idx;//trap to ps acl index
static struct vlan_info_list* sys_vlan_info=NULL;
#endif
static int loop_detect_run=0;
const unsigned int int_magic = 0xdeadbeef;
const unsigned char str_magic[] = "realtek_loopback_detect_packet";

/*siyuan 2016-08-09: use LAN_PORT_NUM instead of LAN_PORT_NUM defined by CONFIG_LAN_PORT_NUM
  because oam binary version needs to support 9602(CONFIG_LAN_PORT_NUM is 2) and 9607(CONFIG_LAN_PORT_NUM is 4) */
extern int LAN_PORT_NUM;

#ifdef CONFIG_APOLLO_ROMEDRIVER
static void loopdetect_rg_init(void)
{
	int i;
	uint32 portmask=0;
	rtk_rg_aclFilterAndQos_t acl = {0};
	rtk_rg_cvlan_info_t cvlan_info;
	struct vlan_info_list *pcvlan_info;
	char macstr[32];
	char cmd[128];

	for(i=0; i<LAN_PORT_NUM; i++)
		portmask |=(1<<PortLogic2PhyID(i));

	pthread_mutex_lock(&loopdetectmutex);
	// Let LAN port can send/receive configured vlan
	for(i = 0 ; (i < LOOPDETECT_VLAN_NUM) && (loopdetectInfo.loopdetect_vlan[i]!=-1); i++)
	{
		if(loopdetectInfo.loopdetect_vlan[i] != 0)
		{
			pcvlan_info = malloc(sizeof(struct vlan_info_list));
			memset(pcvlan_info, 0, sizeof(struct vlan_info_list));
			memset(&cvlan_info, 0, sizeof(rtk_rg_cvlan_info_t));
			cvlan_info.vlanId = loopdetectInfo.loopdetect_vlan[i];
			rtk_rg_cvlan_get(&cvlan_info);
			memcpy(&(pcvlan_info->vlan_info), &cvlan_info, sizeof(cvlan_info));
			pcvlan_info->next = sys_vlan_info;
			sys_vlan_info = pcvlan_info;
			cvlan_info.memberPortMask.portmask |= portmask|(1<<RTK_RG_PORT_CPU);
			//cvlan_info.untagPortMask.portmask &= ~(portmask);
			rtk_rg_cvlan_add(&cvlan_info);
		}
	}
	pthread_mutex_unlock(&loopdetectmutex);
	acl.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_ALL_PACKET;
	acl.filter_fields = INGRESS_PORT_BIT | INGRESS_DMAC_BIT|INGRESS_SMAC_BIT;
	acl.ingress_port_mask.portmask = portmask;
	acl.ingress_dmac.octet[0] = 0xff;
	acl.ingress_dmac.octet[1] = 0xff;
	acl.ingress_dmac.octet[2] = 0xff;
	acl.ingress_dmac.octet[3] = 0xff;
	acl.ingress_dmac.octet[4] = 0xff;
	acl.ingress_dmac.octet[5] = 0xff;
	acl.action_type = ACL_ACTION_TYPE_TRAP_TO_PS;
	ctc_oam_flash_var_get("ELAN_MAC_ADDR", macstr, sizeof(macstr));
	sscanf(macstr,"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx", &(acl.ingress_smac.octet[0]),&(acl.ingress_smac.octet[1]),
		&(acl.ingress_smac.octet[2]),&(acl.ingress_smac.octet[3]),&(acl.ingress_smac.octet[4]),&(acl.ingress_smac.octet[5]));
	if(rtk_rg_aclFilterAndQos_add(&acl, &acl_idx))
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] add acl trap loopback detect pkt to ps fail!\n\n",  __func__, __LINE__);
		close(send_sock);
		pthread_exit(-1);
	}
	
	//cxy 2017-6-20: set ratelimit for broadcst and multicast, offload cpu loading
	rtk_rg_shareMeter_set (15,300,ENABLED);  // for broadcast
	rtk_rg_shareMeter_set (14,300,ENABLED); // for multicast
	system("echo 15 > /proc/rg/BC_rate_limit");
	system("echo 14 > /proc/rg/IPv4_MC_rate_limit");
	system("echo 14 > /proc/rg/IPv6_MC_rate_limit");

	snprintf(cmd, sizeof(cmd), "echo 0x%x > /proc/rg/BC_rate_limit_portMask", portmask);
	system(cmd);
	snprintf(cmd, sizeof(cmd), "echo 0x%x > /proc/rg/IPv4_MC_rate_limit_portMask", portmask);
	system(cmd);
	snprintf(cmd, sizeof(cmd), "echo 0x%x > /proc/rg/IPv6_MC_rate_limit_portMask", portmask);
	system(cmd);
}

static void loopdetect_rg_exit(void)
{

	struct vlan_info_list*p;

	while(sys_vlan_info)
	{
		if(sys_vlan_info->vlan_info.memberPortMask.portmask)
			rtk_rg_cvlan_add(&(sys_vlan_info->vlan_info));
		else
			rtk_rg_cvlan_del(sys_vlan_info->vlan_info.vlanId);
		p=sys_vlan_info->next;
		free(sys_vlan_info);
		sys_vlan_info = p;
	}

	if(rtk_rg_aclFilterAndQos_del(acl_idx))
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] del acl trap loopback detect pkt to ps fail!\n\n",  __func__, __LINE__);
		close(send_sock);
		pthread_exit(-1);
	}
}

static void rg_ifconfig(int ifnum, int enable)
{
	uint32 port = PortLogic2PhyID(ifnum);

#ifdef ENABLE_PORT_PHY_DOWN
	int32   ret;
    unsigned int data;
	if((ret = rtk_port_phyReg_get(port, PHY_PAGE_0, PHY_CONTROL_REG, &data)) != RT_ERR_OK)
    {
    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] rtk_port_phyReg_get fail ret[%d]!\n",  __func__, __LINE__, ret);
        return;
    }

    if(enable == 0)
    {
        data |= PowerDown_MASK;
    }
    else
    {
        data &= ~(PowerDown_MASK);
    }

    if((ret = rtk_port_phyReg_set(port, PHY_PAGE_0, PHY_CONTROL_REG, data)) != RT_ERR_OK)
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] rtk_port_phyReg_set fail ret[%d]!\n",  __func__, __LINE__, ret);
    } 
#else
	if(enable)
	{
		rtk_rg_port_macForceAbilityState_set(port, RTK_RG_DISABLED);
	}
	else
	{
		rtk_port_macAbility_t macAbility = {0};
		rtk_rg_port_macForceAbility_get(port, &macAbility);
		macAbility.linkStatus = PORT_LINKDOWN;
		rtk_rg_port_macForceAbility_set(port, macAbility);
		rtk_rg_port_macForceAbilityState_set(port, RTK_RG_ENABLED);
	}
#endif
}
#else
static void rtk_ifconfig(int ifnum, int enable)
{
	uint32 port = PortLogic2PhyID(ifnum);

#ifdef ENABLE_PORT_PHY_DOWN
	int32   ret;
    unsigned int data;
	if((ret = rtk_port_phyReg_get(port, PHY_PAGE_0, PHY_CONTROL_REG, &data)) != RT_ERR_OK)
    {
    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] rtk_port_phyReg_get fail ret[%d]!\n",  __func__, __LINE__, ret);
        return;
    }

    if(enable == 0)
    {
        data |= PowerDown_MASK;
    }
    else
    {
        data &= ~(PowerDown_MASK);
    }

    if((ret = rtk_port_phyReg_set(port, PHY_PAGE_0, PHY_CONTROL_REG, data)) != RT_ERR_OK)
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] rtk_port_phyReg_set fail ret[%d]!\n",  __func__, __LINE__, ret);
    } 
#else
	if(enable)
	{
		rtk_port_macForceAbilityState_set(port, DISABLED);
	}
	else
	{
		rtk_port_macAbility_t macAbility = {0};
		rtk_port_macForceAbility_get(port, &macAbility);
		macAbility.linkStatus = PORT_LINKDOWN;
		rtk_port_macForceAbility_set(port, macAbility);
		rtk_port_macForceAbilityState_set(port, ENABLED);
	}
#endif
}
#endif

static void loopdetect_init(void)
{
	int i = 0;
	struct ifreq ifr;
	const static unsigned char broadcast_addr[]= {0xff,0xff,0xff,0xff,0xff,0xff};

	send_sock = socket(AF_PACKET, SOCK_DGRAM, loopdetectInfo.ether_type);
	if( send_sock < 0)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d]Open socket error [%d].\n", __func__,__LINE__,i);
		pthread_exit(-1);
	}

	// prepare common data to send
	send_data.data.int_magic = 0xdeadbeef;
	strcpy(send_data.data.str_magic, str_magic);

	// prepare common destination
	addr.sll_family = AF_PACKET;
	addr.sll_halen = ETHER_ADDR_LEN;
	memcpy(addr.sll_addr, broadcast_addr, ETHER_ADDR_LEN);

	for(i = 0 ; i < LAN_PORT_NUM ; i++)
	{
#ifdef CONFIG_RTL_MULTI_LAN_DEV
		sprintf(ifr.ifr_name, "eth0.%d", i+2);
#else
		sprintf(ifr.ifr_name, "eth0");
#endif

		if (ioctl(send_sock, SIOCGIFINDEX, &ifr)==-1)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] Get %s ifindex failed\n", __func__,__LINE__,ifr.ifr_name);
			close(send_sock);
			pthread_exit(-1);
		}
		loopdetectInfo.lan_if[i].ifindex = ifr.ifr_ifindex;
	}
}

static void loopdetect_exit(void)
{	
	int i;
	char cmd[128];
	system("echo stop > /proc/loopback_detect");
	close(send_sock);
	//enable disable port for loopdetect thread will exit
	for(i=0; i<LAN_PORT_NUM; i++)
	{
		if(loopdetectInfo.lan_if[i].disabletime==0)
			continue;
#ifdef CONFIG_APOLLO_ROMEDRIVER
		rg_ifconfig(i,1);
#else
		rtk_ifconfig(i,1);
#endif
	}
	pthread_mutex_lock(&loopdetectmutex);
	for(i=0; i<LAN_PORT_NUM; i++)
	{
		/* must not reset loop_port_autodown_enable */
		loopdetectInfo.lan_if[i].loopdetect_enable = 0;
		loopdetectInfo.lan_if[i].ifindex = 0;
		loopdetectInfo.lan_if[i].loop_detected = 0;
		loopdetectInfo.lan_if[i].disabletime = 0;
	}
	pthread_mutex_unlock(&loopdetectmutex);
}

static void send_loopdetect_packets(int lan_idx)
{
	int i;
	unsigned short ether_type;
	short loopdetect_vlan[LOOPDETECT_VLAN_NUM];
	
	if(lan_idx < 0 || lan_idx >= LAN_PORT_NUM)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Invalid LAN interface: %d\n", __func__, __LINE__, lan_idx);
		return;
	}
	send_data.data.ifnum = lan_idx;
	pthread_mutex_lock(&loopdetectmutex);
	addr.sll_ifindex = loopdetectInfo.lan_if[lan_idx].ifindex;
	/* use temp parameters to store used global setting, decrease mutex lock time */
	ether_type = loopdetectInfo.ether_type;
	for(i=0; i<LOOPDETECT_VLAN_NUM; i++)
	{
		if(loopdetectInfo.loopdetect_vlan[i] != -1)
		{
			loopdetect_vlan[i] = loopdetectInfo.loopdetect_vlan[i];
		}
		else
		{
			loopdetect_vlan[i] = -1;
			break;
		}
	}
	pthread_mutex_unlock(&loopdetectmutex);
	
	for(i=0; i<LOOPDETECT_VLAN_NUM && loopdetect_vlan[i]!=-1; i++)
	{
		if(loopdetect_vlan[i]==0)
		{
#if defined(CONFIG_HGU_APP) && !defined(YUEME_CUSTOMIZED_CHANGE)
			addr.sll_protocol = htons(ETH_P_8021Q);
			send_data.vid = htons(1);
			send_data.type = htons(loopdetectInfo.ether_type);
			if(sendto(send_sock, &send_data, sizeof(struct loopdetect_data), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Send failed\n",  __func__, __LINE__);
			}
#else
			addr.sll_protocol = htons(ether_type);
			if(sendto(send_sock, &send_data.data, sizeof(struct loopdetect_data), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Send failed\n",  __func__, __LINE__);
			}
#endif
		}
		else
		{
			addr.sll_protocol = htons(ETH_P_8021Q);
			send_data.vid = htons(loopdetect_vlan[i]);
			send_data.type = htons(ether_type);
			if(sendto(send_sock, &send_data, sizeof(struct loopdetect_data), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Send failed\n",  __func__, __LINE__);
			}
		}
	}

}

static void update_loopdetect_status(int timecounter)
{
	FILE *fp = NULL;
	int loopintf = 0x0;
	int i,disableflag=0;
	char cmd[128];
	unsigned int loopdetect_time;
	
	pthread_mutex_lock(&loopdetectmutex);
	loopdetect_time = loopdetectInfo.loopdetect_time;
	pthread_mutex_unlock(&loopdetectmutex);
	
	if((timecounter == (loopdetect_time/1000)) || (loopdetect_time <= 1000))
	{
		fp=fopen("/proc/loopback_detect","r");
	    if (fp)
	    {
			fscanf(fp,"%d",&loopintf);
			for(i=0; i<LAN_PORT_NUM; i++)
			{
				pthread_mutex_lock(&loopdetectmutex);
				if((loopintf&(1<<i)) && (loopdetectInfo.lan_if[i].loopdetect_enable == 1))
				{
					loopdetectInfo.lan_if[i].loop_detected = 1;
					EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG,"[OAM:%s:%d] LAN interface %d loop detected\n", __func__, __LINE__, i);
					if(disableflag == 0 && loopdetectInfo.lan_if[i].loop_port_autodown_enable == 1 && loopdetectInfo.lan_if[i].disabletime == 0)
					{
						EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG, "[OAM:%s:%d] loop LAN interface %d down\n", __func__, __LINE__, i);
						disableflag = 1;//only disable one loop port once
						loopdetectInfo.lan_if[i].disabletime = 1;
#ifdef CONFIG_APOLLO_ROMEDRIVER
						rg_ifconfig(i,0);
#else
						rtk_ifconfig(i,0);
#endif
					}
				}
				else
					loopdetectInfo.lan_if[i].loop_detected = 0;

				pthread_mutex_unlock(&loopdetectmutex);
			}
			fclose(fp);
		}
	}

	//update loopback port disabletime, and if exprire, enable this port
	for(i=0; i<LAN_PORT_NUM; i++)
	{
		if(loopdetectInfo.lan_if[i].disabletime==0)
			continue;

		pthread_mutex_lock(&loopdetectmutex);
		if(loopdetectInfo.lan_if[i].disabletime > loopdetectInfo.loop_recover_time)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG, "[OAM:%s:%d] loop LAN interface %d disable time expire, so enable now\n", __func__, __LINE__, i);
			loopdetectInfo.lan_if[i].disabletime = 0;
			loopdetectInfo.lan_if[i].loop_detected = 1;//set to 1, for we should wait next loop detect to deside whether loop doesn't exist
#ifdef CONFIG_APOLLO_ROMEDRIVER
			rg_ifconfig(i,1);
#else
			rtk_ifconfig(i,1);
#endif
		}
		else
			loopdetectInfo.lan_if[i].disabletime += 1;
		
		pthread_mutex_unlock(&loopdetectmutex);
	}
	
}

void* ctc_loopdetect(void* argu)
{
	int time_counter=0;
	int i;

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG, "[OAM:%s:%d] loopdetect thread start!!!\n",__func__, __LINE__);
	loopdetect_init();
#ifdef CONFIG_APOLLO_ROMEDRIVER
	loopdetect_rg_init();
#endif
	system("echo startbyoam > /proc/loopback_detect");

	while(1)
	{
		if(loop_detect_run == 0)
			break;

		if(loopdetectInfo.loopdetect_time > 1000)
		{
			if(time_counter > (loopdetectInfo.loopdetect_time/1000))
			{
				time_counter = 1;
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG,"[OAM:%s:%d] send loop detect pkt\n", __func__, __LINE__);
				for(i = 0 ; i < LAN_PORT_NUM ; i++)
					send_loopdetect_packets(i);
			}
			update_loopdetect_status(time_counter);
			sleep(1);
			time_counter++;
		}
		else 
		{	/* detect time <= 1s */		
			struct timeval ltv;
			uint32 startTime, endTime;
    		int32  interval;

			startTime = 0;
        	endTime = 0;
        	interval = 0;

        	gettimeofday(&ltv, NULL);
        	startTime = ltv.tv_sec * 1000 * 1000 + ltv.tv_usec;
		
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG,"[OAM:%s:%d] send loop detect pkt\n", __func__, __LINE__);
			for(i = 0 ; i < LAN_PORT_NUM ; i++)
				send_loopdetect_packets(i);
			update_loopdetect_status(time_counter);

			gettimeofday(&ltv, NULL);
	        endTime = ltv.tv_sec * 1000 * 1000 + ltv.tv_usec;
			
	        interval = endTime - startTime;        
	        interval = loopdetectInfo.loopdetect_time * 1000 - interval;

	        if (interval > 0)
	            usleep(interval);
		}
	}
	
#ifdef CONFIG_APOLLO_ROMEDRIVER
	loopdetect_rg_exit();
#endif
	loopdetect_exit();
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG, "[OAM:%s:%d] loopdetect thread end!!!\n",__func__, __LINE__);
	pthread_exit(0);
}

int set_port_loopdetect(int portnum, unsigned char enable)
{
	int i;

	if(portnum < 0 || portnum >= LAN_PORT_NUM)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] Invalid LAN interface: %d\n", __func__, __LINE__, portnum);
		return RT_ERR_FAILED;
	}
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG, "[OAM:%s:%d] set port:%d loopdetect enable:%d\n", __func__, __LINE__, portnum,enable);
	pthread_mutex_lock(&loopdetectmutex);
	loopdetectInfo.lan_if[portnum].loopdetect_enable = enable;	
	//start or exit loop_detect thread
	for(i=0; i<LAN_PORT_NUM; i++)
	{
		if(loopdetectInfo.lan_if[portnum].loopdetect_enable)
			break;
	}
	pthread_mutex_unlock(&loopdetectmutex);
	if(i<LAN_PORT_NUM)
	{
		if(loop_detect_run==0)
		{
			loop_detect_run=1;
			epon_oam_event_send(0, EPON_OAM_EVENT_LOOP_DETECT_ENABLE);
		}
	}
	else
	{
		if(loop_detect_run==1)
			loop_detect_run=0;
	}
	return RT_ERR_OK;
}

int get_port_loopdetect(int portnum, unsigned char *enable)
{
	if(portnum < 0 || portnum >= LAN_PORT_NUM)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] Invalid LAN interface: %d\n", __func__, __LINE__, portnum);
		return RT_ERR_FAILED;
	}
	if(enable==NULL)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] enable pointer is NULL\n", __func__, __LINE__);
		return RT_ERR_FAILED;
	}
	pthread_mutex_lock(&loopdetectmutex);
	*enable = loopdetectInfo.lan_if[portnum].loopdetect_enable;
	pthread_mutex_unlock(&loopdetectmutex);
	return RT_ERR_OK;
}

int set_port_loopdetect_autodown(int portnum, unsigned char enable)
{
	if(portnum < 0 || portnum >= LAN_PORT_NUM)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] Invalid LAN interface: %d\n", __func__, __LINE__, portnum);
		return RT_ERR_FAILED;
	}
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG, "[OAM:%s:%d] set port:%d loopdetect autodown enable:%d\n", __func__, __LINE__, portnum,enable);

	/* the function called by ctc command PortDisableLooped(0xC7/0x0018) may be locked
	   and the oam response may be delayed about 4 seconds, so not use lock here */
	//pthread_mutex_lock(&loopdetectmutex);
	loopdetectInfo.lan_if[portnum].loop_port_autodown_enable = enable;
	//pthread_mutex_unlock(&loopdetectmutex);
	return RT_ERR_OK;
}

int get_port_loopdetect_autodown(int portnum, unsigned char *enable)
{
	if(portnum < 0 || portnum >= LAN_PORT_NUM)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] Invalid LAN interface: %d\n", __func__, __LINE__, portnum);
		return RT_ERR_FAILED;
	}
	if(enable==NULL)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] enable pointer is NULL\n", __func__, __LINE__);
		return RT_ERR_FAILED;
	}
	pthread_mutex_lock(&loopdetectmutex);
	*enable = loopdetectInfo.lan_if[portnum].loop_port_autodown_enable;
	pthread_mutex_unlock(&loopdetectmutex);
	return RT_ERR_OK;
}

/* unit of detectFrequency is pps (packet per second) 
   only support one vlan tag now, cvlan is prio to svlan */
int set_port_loopdetect_parameterConfig(int portnum, unsigned short detectFrequency, 
						unsigned short recoveryInterval, short *svlan, short *cvlan)
{
	unsigned int detecttime;
	int i;
	
	if(portnum < 0 || portnum >= LAN_PORT_NUM)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] Invalid LAN interface: %d\n", __func__, __LINE__, portnum);
		return RT_ERR_FAILED;
	}

	detecttime = 1000 / detectFrequency;
	if(detecttime < LOOPDETECT_MIN_TIME)
		detecttime = LOOPDETECT_MIN_TIME;

	pthread_mutex_lock(&loopdetectmutex);
	loopdetectInfo.loopdetect_time = detecttime;
	loopdetectInfo.loop_recover_time = recoveryInterval;

	for(i=0; i<LOOPDETECT_VLAN_NUM; i++)
	{
		short vlan;
		if((svlan[i] == -1) && (cvlan[i] == -1))
			break;
		
		if(cvlan[i] > 0)
			vlan = cvlan[i];
		else
			vlan = svlan[i];
		
		loopdetectInfo.loopdetect_vlan[i] = vlan;
	}
	loopdetectInfo.loopdetect_vlan[i] = -1; /* reset not used vlan setting */
	pthread_mutex_unlock(&loopdetectmutex);
	
	return RT_ERR_OK;
}

int get_port_loopdetect_status(int portnum)
{
	int loopstatus=0;

	if(portnum < 0 || portnum >= LAN_PORT_NUM)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR,"[OAM:%s:%d] Invalid LAN interface: %d\n", __func__, __LINE__, portnum);
		return RT_ERR_FAILED;
	}
	pthread_mutex_lock(&loopdetectmutex);
	//loop_detected for loop detect enable  and  disabletime for autodown enable
	if(loopdetectInfo.lan_if[portnum].loop_detected || loopdetectInfo.lan_if[portnum].disabletime)
		loopstatus = 1;
	pthread_mutex_unlock(&loopdetectmutex);
	return loopstatus;
}

/* return vlaue: 0: same vlan do nothing, 1:vlan change */
static int isVlanChange(short *vlan)
{
	int i;
	int change = 0;
	for(i=0; i<LOOPDETECT_VLAN_NUM; i++)
	{
		if(loopdetectInfo.loopdetect_vlan[i] != vlan[i])
		{
			change = 1;
			break;
		}
		if((loopdetectInfo.loopdetect_vlan[i] == -1) && (vlan[i] == -1))
			break;
	}
	return change;
}

int ctc_oam_loop_detect_set(unsigned char enable, unsigned short ethertype, 
	unsigned int detecttime, unsigned int recovertime,short *vlan)
{
	int i;
	uint32 portmask=0;
#ifdef CONFIG_APOLLO_ROMEDRIVER
	rtk_rg_cvlan_info_t cvlan_info;
	struct vlan_info_list *pcvlan_info;
	char cmd[256] = {0};
#endif

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_DEBUG, "[OAM:%s:%d] loopdetect enable:%d ethertype:0x%04x detecttime:%d recovertime:%d vlan0:%d\n",
		__func__, __LINE__, enable,ethertype,detecttime,recovertime,vlan[0]);
	pthread_mutex_lock(&loopdetectmutex);
	loopdetectInfo.ether_type = ethertype;
	loopdetectInfo.loopdetect_time = detecttime * 1000;
	loopdetectInfo.loop_recover_time = recovertime;
	pthread_mutex_unlock(&loopdetectmutex);
#ifdef CONFIG_APOLLO_ROMEDRIVER
	snprintf(cmd, sizeof(cmd), "echo 0x%x > /proc/rg/keep_ps_eth", loopdetectInfo.ether_type);
	printf("OAM loopback: %s-%d cmd=%s\n",__func__,__LINE__,cmd);
	system(cmd);
#endif
	if(vlan!=NULL && isVlanChange(vlan))
	{
#ifdef CONFIG_APOLLO_ROMEDRIVER
		struct vlan_info_list*p;

		while(sys_vlan_info)
		{
			if(sys_vlan_info->vlan_info.memberPortMask.portmask)
				rtk_rg_cvlan_add(&(sys_vlan_info->vlan_info));
			else
				rtk_rg_cvlan_del(sys_vlan_info->vlan_info.vlanId);
			p=sys_vlan_info->next;
			free(sys_vlan_info);
			sys_vlan_info = p;
		}
		for(i=0; i<LAN_PORT_NUM; i++)
			portmask |=(1<<PortLogic2PhyID(i));
#endif
		pthread_mutex_lock(&loopdetectmutex);
		for(i=0;i<LOOPDETECT_VLAN_NUM && vlan[i]!=-1; i++)
		{
			loopdetectInfo.loopdetect_vlan[i] = vlan[i];
#ifdef CONFIG_APOLLO_ROMEDRIVER
			if(vlan[i]!=0)	
			{			
				pcvlan_info = malloc(sizeof(struct vlan_info_list));
				memset(pcvlan_info, 0, sizeof(struct vlan_info_list));
				memset(&cvlan_info, 0, sizeof(rtk_rg_cvlan_info_t));
				cvlan_info.vlanId = vlan[i];
				rtk_rg_cvlan_get(&cvlan_info);
				memcpy(&(pcvlan_info->vlan_info), &cvlan_info, sizeof(cvlan_info));
				pcvlan_info->next = sys_vlan_info;
				sys_vlan_info = pcvlan_info;
				cvlan_info.memberPortMask.portmask |= portmask|(1<<RTK_RG_PORT_CPU);
				//cvlan_info.untagPortMask.portmask &= ~(portmask);					
				rtk_rg_cvlan_add(&cvlan_info);
			}
#endif
		}
		loopdetectInfo.loopdetect_vlan[i] = -1; /* reset not used vlan setting */
		pthread_mutex_unlock(&loopdetectmutex);
	}

#ifdef CONFIG_SFU_APP
	/* siyuan 2017-01-13: because we don't flood broadcast packets to cpu port for sfu, 
       so need to add a acl to trap the loopdetect packets to cpu port. */
	{
		rtk_acl_ingress_entry_t aclRule;
		rtk_acl_field_t aclField1,aclField2;
		int ret;
	
		memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));
		memset(&aclField1, 0, sizeof(rtk_acl_field_t));
		memset(&aclField2, 0, sizeof(rtk_acl_field_t));
			
		aclField1.fieldType = ACL_FIELD_ETHERTYPE;
		aclField1.fieldUnion.data.value = ethertype;
		aclField1.fieldUnion.data.mask = 0xFFFF;
		aclField1.next = NULL;
		if ((ret = rtk_acl_igrRuleField_add(&aclRule,&aclField1)) != RT_ERR_OK)
		{
			printf("rtk_acl_igrRuleField_add fail ret[%d]\n",ret);
			return ret;
		}

		aclField2.fieldType = ACL_FIELD_DMAC;
		memset(&aclField2.fieldUnion.mac.value.octet, 0xFF, ETHER_ADDR_LEN);	
		memset(&aclField2.fieldUnion.mac.mask.octet, 0xFF, ETHER_ADDR_LEN);
		aclField2.next = NULL;
		if ((ret = rtk_acl_igrRuleField_add(&aclRule,&aclField2)) != RT_ERR_OK)
		{
			printf("rtk_acl_igrRuleField_add fail ret[%d]\n",ret);
			return ret;
		}
		
		aclRule.valid = ENABLED;
		aclRule.index = LOOPDETECT_ACL_INDEX;
		aclRule.templateIdx = 0x0;
		for(i=0; i< LAN_PORT_NUM;i++)
		{
			RTK_PORTMASK_PORT_SET(aclRule.activePorts, i);
		}	
		aclRule.act.enableAct[ACL_IGR_FORWARD_ACT] = ENABLED;
		aclRule.act.forwardAct.act = ACL_IGR_FORWARD_TRAP_ACT;		
		if ((ret = rtk_acl_igrRuleEntry_add(&aclRule)) != RT_ERR_OK)
		{
			printf("rtk_acl_igrRuleEntry_add fail ret[%d]\n",ret);	
			return ret;
		}
	}
#endif

	if(enable)
	{
		for(i=0; i<LAN_PORT_NUM; i++)
		{
			set_port_loopdetect(i,1);
			set_port_loopdetect_autodown(i,1);
		}
	}
	else
	{
		for(i=0; i<LAN_PORT_NUM; i++)
		{
			set_port_loopdetect(i,0);
			set_port_loopdetect_autodown(i,0);
		}
	}
	return 0;
}

int ctc_oam_loopdetect_get(void)
{
	int i;
	FILE* fp;
	if(!(fp=fopen("/tmp/loopdetect","w")))
	{
		return -1;
	}
	pthread_mutex_lock(&loopdetectmutex);
	printf("loopdetect preriod: %d(ms)\n", loopdetectInfo.loopdetect_time);
	printf("loop recover time: %d(s)\n", loopdetectInfo.loop_recover_time);
	printf("loop detect eherType: 0x%04x\n", loopdetectInfo.ether_type);
	printf("loop detect vlan: ");
	for(i=0; (i<LOOPDETECT_VLAN_NUM) && (loopdetectInfo.loopdetect_vlan[i]!=-1); i++)
	{
		if(i==0)
			printf("%d",loopdetectInfo.loopdetect_vlan[i]);
		else
			printf(", %d",loopdetectInfo.loopdetect_vlan[i]);
	}
	printf("\n");
	printf("port\tloopDetect\tautoDown\tloopDetected\tdownTime\n");
	fprintf(fp,"port\tloopDetect\tautoDown\tloopDetected\tdownTime\n");
	for(i=0; i<LAN_PORT_NUM; i++)
	{
		printf("%d\t%s\t\t%s\t\t%s\t\t%d\n",i,loopdetectInfo.lan_if[i].loopdetect_enable?"enable":"disable",
			loopdetectInfo.lan_if[i].loop_port_autodown_enable?"enable":"disable",
			loopdetectInfo.lan_if[i].loop_detected?"yes":"no",loopdetectInfo.lan_if[i].disabletime);
		fprintf(fp,"%d\t%s\t\t%s\t\t%s\t\t%d\n",i,loopdetectInfo.lan_if[i].loopdetect_enable?"enable":"disable",
			loopdetectInfo.lan_if[i].loop_port_autodown_enable?"enable":"disable",
			loopdetectInfo.lan_if[i].loop_detected?"yes":"no",loopdetectInfo.lan_if[i].disabletime);
	}
	pthread_mutex_unlock(&loopdetectmutex);
	fclose(fp);
	return 0;
}

void loopdetct_var_init(void)
{
	int i;

	pthread_mutex_init(&loopdetectmutex, NULL);
	memset(&loopdetectInfo, 0, sizeof(loopdetectInfo));
	for(i=0; i<LOOPDETECT_VLAN_NUM; i++)
	{
		loopdetectInfo.loopdetect_vlan[i]=-1;
	}
	loopdetectInfo.loopdetect_vlan[0]=0;
	loopdetectInfo.ether_type = LOOPDETECT_ETHERTYPE;
	loopdetectInfo.loopdetect_time = LOOPDETECT_TIME;
	loopdetectInfo.loop_recover_time = LOOP_RECOVER_TIME;
}
