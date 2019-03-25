#ifndef __EPON_OAM_IGMP_QUERIER_H__
#define __EPON_OAM_IGMP_QUERIER_H__

#include "epon_oam_igmp.h"
#include "epon_oam_igmp_db.h"

/*below macro copy from net/ipv6.h */
#define IPV6_ADDR_ANY		0x0000U
#define IPV6_ADDR_UNICAST   0x0001U	
#define IPV6_ADDR_MULTICAST 0x0002U	
#define IPV6_ADDR_LOOPBACK	0x0010U
#define IPV6_ADDR_LINKLOCAL	0x0020U
#define IPV6_ADDR_SITELOCAL	0x0040U
#define IPV6_ADDR_COMPATv4	0x0080U
#define IPV6_ADDR_SCOPE_MASK	0x00f0U
#define IPV6_ADDR_MAPPED	0x1000U
#define IPV6_ADDR_RESERVED	0x2000U	/* reserved address space */

#define IGMPV3_HEAD_LEN         4
#define IGMPV3_OPTIONS_LEN      4

#define MAX_ROUTER_VLAN      SYS_VLAN_STATIC_NUM

enum
{
    IGMP_QUERIER_STATE_INIT = 0,
    IGMP_QUERIER_STATE_LEAVING_HOST, /* Has rcvd a Leave msg and going to leave */
    IGMP_QUERIER_STATE_OTHER_HOST, /* Non Leaving host, expecting report
                                      * from this host */
};

enum
{
    IGMP_NON_QUERIER,
    IGMP_QUERIER,
};

enum
{
    IGMP_GENERAL_QUERY,
    IGMP_SPECIFIC_GROUP_QUERY,    
    IGMP_SPECIFIC_GROUP_SRC_QUERY,    
};

typedef struct igmp_querier_entry_s
{
    uint32 ipType;//MULTICAST_TYPE_IPV4 or MULTICAST_TYPE_IPV6
    uint32 ip[4]; /* Querier IP of the VLAN, may be the Switch or External Router*/
    uint32 vid;
    uint32 status;//QUERIER or NON_QUERIER
    uint32 enabled;
    uint32 version;//IGMP_QUERY_V3
    uint32 configured; /* this will identify whether wuerier is configured or not */
    uint16 startupQueryCnt;
    uint16 timer;
} igmp_querier_entry_t;

typedef struct igmp_querier_db_s
{
	igmp_querier_entry_t    querier_entry[MAX_ROUTER_VLAN];
	uint16                  entry_num;
} igmp_querier_db_t;


#define IGMP_FILL_QUERY(pkt, field, offset, len) \
do {\
    osal_memcpy(&(pkt)->data[offset], (uint8*)(field), len);\
}while(0)

int32 sys_ipAddr_get(char *devName, uint32 *pIp);
int32 mcast_querier_db_init(void);
int32 mcast_querier_db_add(uint16 vid, uint32 ipType);
int32 mcast_querier_db_get(uint16 vid, uint32 ipType, igmp_querier_entry_t **ppEntry);
int32 mcast_querier_db_del(uint16 vid, uint32 ipType);
int32 mcast_igmp_querier_check(uint16 vid, uint32 sip, uint8 query_version);
int32 mcast_mld_querier_check(uint16 vid, uint32 *sip, uint8 query_version);
void mcast_igmp_querier_timer(void);
void mcast_igmp_send_grp_specific_query(uint16 vid, uint32 dip, sys_logic_port_t port);
void mcast_igmp_send_grp_src_specific_query(uint16 vid, uint32 dip, uint32 *sip, uint16 numSrc, sys_logic_port_t port);
void mcast_igmp_build_general_query (packet_info_t *pkt, sys_vid_t vid,uint16 pktLen, uint8 version, uint32 queryEnable,uint32 gdaddr, uint8 query_type);
void mcast_igmp_send_general_query(igmp_querier_entry_t* qryPtr, uint8 igmp_query_version);

void mcast_send_general_query(igmp_querier_entry_t *qryPtr, uint8 version);
void mcast_send_gs_query(igmp_group_entry_t *pGroup, sys_logic_port_t lPort);
void mcast_send_gss_query(igmp_group_entry_t *pGroup, uint32 *sip, uint16 numSrc, sys_logic_port_t lport);

void mcast_igmp_querier_show();
#endif

