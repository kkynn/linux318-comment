#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/udp.h>
#endif

#include <rtk_rg_sip.h>
#include <rtk_rg_alg_tool.h>

static int32 init_sip = 0;
static uint8 *_rtl8651_l4_sip_alias_buf[1];

int num_memcmp(void *src,void *dest,int num)
{
	int r;
	char *pSrc=(char*)src;
	r=memcmp(src,dest,num);
	if(r!=0) return r;
	if((pSrc[num]>='0')&&(pSrc[num]<='9')) return -1; //ex: '192.168.1.1' hit '192.168.1.111' is not allow.
	return 0;
}

static char localip[16];
static int8 localip_len;
static char globalip[16];
static int8 globalip_len;
static char globalport[16];
static int8 globalport_len;
static char localport[16];
static int8 localport_len;
static char controlport[16];
static int8 controlport_len;
static char trueport[16];
static int8 trueport_len=0;

static char temp_trueport[16];
static int8 temp_trueport_len=0;


static char rtpport[16];
static int8 rtpport_len=0;
//char controlport[16];
//int8 controlport_len=0;
//char controlextport[16];
//int8 controlextport_len=0;
static char remoteip[16];
static int8 remoteip_len;
static char key[32];
static int8 key_len;
static char keyword[RG_ALG_SIP_MAX_RTP_CONN][32];
static int8 keyword_len[RG_ALG_SIP_MAX_RTP_CONN];
static char ipkey[32];
static int8 ipkey_len;
static char cseqkey[8];
static int8 cseqkey_len;
static char byekey[8];
static int8 byekey_len;
static char cancelkey[8];
static int8 cancelkey_len;
static char realip[32];
static int8 realip_len=0;
//uint32 extIP;
//uint16 control_extport = 0;
//uint16 rtcpPort = 0;

//for inbound
static char viakey[32];
static int8 viakey_len=0;
static char rportkey[32];
static int8 rportkey_len;
static char realrtpport[32];
static int8 realrtpport_len = 0;
static char realport[32];
static int8 realport_len = 0;

static void _rtk_rg_sip_flowFlush(rtk_rg_alg_connection_t * pConn)
{
	int i;
	for(i=0;i<RG_ALG_SIP_MAX_RTP_CONN;i++)
		if(pConn->app.sip.flow_idx[i]>=0)
			(pf.rtk_rg_naptConnection_del)(pConn->app.sip.flow_idx[i]);
}

static void _rtk_rg_sip_appInit(rtk_rg_alg_connection_t * pConn)
{
	int i;
	bzero(&pConn->app.sip,sizeof(rtk_rg_alg_sip_t));
	for(i=0;i<RG_ALG_SIP_MAX_RTP_CONN;i++)pConn->app.sip.flow_idx[i]=-1;//init
	pConn->app.sip.byeDirect=-1;//init
}

static int _rtk_rg_sip_process(int direct, unsigned char * pData, unsigned int dataLen,rtk_rg_alg_connection_t * pConn)
{

	//int ret,newDelta=0;
	//int ret;
	//ipaddr_t ipAddr,gwIPAddr;
	//unsigned short portNum,newPort;
	//unsigned int newLen,flowIdx;
	rtk_rg_pktHdr_t *pPktHdr;
	struct sk_buff *skb;
	//rtk_rg_naptEntry_t naptEntry;
	//rtk_rg_upnpConnection_t upnpConn;
	rtk_rg_naptEntry_t naptFlow;
	
	pPktHdr = (rtk_rg_pktHdr_t *)pConn->pPktHdr;
	skb = (struct sk_buff *)pConn->skb;

	//if(direct==NAPT_DIRECTION_OUTBOUND)
	{
		int i,j,k,idx=0,rtp_cnt=0;
		int8 old_cl_len=0; //old content-length len
		int8 new_cl_len=0; //new content-length len
		int32 size_delta=0;
		int32 CntPortInMsgBody[RG_ALG_SIP_MAX_RTP_CONN]={0};
		uint16 i_port[RG_ALG_SIP_MAX_RTP_CONN]={0},o_port[RG_ALG_SIP_MAX_RTP_CONN]={0};

		//for new packet
		struct iphdr *spip;
		struct udphdr *sudp;
		int8 *sptr, *newpacket;

		//for data flow mapt priority
		//int index;
		//rtk_rg_naptFilterAndQos_t napt_filter;

		// assigned new packet buffer ; use the latter part of _rtl8651_l4_sip_alias_buf 
		newpacket =  (int8 *)_rtl8651_l4_sip_alias_buf[0];
		
		// copy out the mbuf data 
		memcpy(newpacket,skb->data,(skb->len>_RTL8651_L4_MAX_SIP_ALIAS_BUF_SIZE)?_RTL8651_L4_MAX_SIP_ALIAS_BUF_SIZE:skb->len);

		//shift pointer to IP header
		newpacket+=pPktHdr->l3Offset;
			
		// Place string pointer and beginning of data 
		spip=(struct iphdr*)newpacket;
		sudp=(struct udphdr *)((int8 *) spip + ((spip->ihl &0xf) << 2));
		sptr=(int8 *)(sudp+1);
		
		sprintf(localip,"%u.%u.%u.%u",(pConn->tuple.internalIp.ip>>24)&0xff,(pConn->tuple.internalIp.ip>>16)&0xff,(pConn->tuple.internalIp.ip>>8)&0xff,(pConn->tuple.internalIp.ip)&0xff);
		localip_len=strlen(localip);
		
		sprintf(localport, "%u", (pConn->tuple.internalPort)&0xffff);
		localport_len = strlen(localport); 

		if(pConn->app.sip.control_port==0)pConn->app.sip.control_port=pConn->tuple.internalPort;
		sprintf(controlport, "%u", (pConn->app.sip.control_port)&0xffff);
		controlport_len = strlen(controlport);

		sprintf(globalip,"%u.%u.%u.%u",(pConn->tuple.extIp.ip>>24)&0xff,(pConn->tuple.extIp.ip>>16)&0xff,(pConn->tuple.extIp.ip>>8)&0xff,(pConn->tuple.extIp.ip)&0xff);
		globalip_len=strlen(globalip);	

		sprintf(globalport, "%u", (pConn->tuple.extPort)&0xffff);
		globalport_len = strlen(globalport); 

		sprintf(remoteip,"%u.%u.%u.%u",(pConn->tuple.remoteIp.ip>>24)&0xff,(pConn->tuple.remoteIp.ip>>16)&0xff,(pConn->tuple.remoteIp.ip>>8)&0xff,(pConn->tuple.remoteIp.ip)&0xff);
		remoteip_len = strlen(remoteip); 

		if(direct==NAPT_DIRECTION_OUTBOUND)
		{
			ALG("[SIP OUT] Begin: local=%s:%s global=%s:%s remote=%s\n",localip,localport,globalip,globalport,remoteip);
		}
		else
		{
			ALG("[SIP IN] Begin: local=%s:%s global=%s:%s remote=%s\n",localip,localport,globalip,globalport,remoteip);
		}
		
		sprintf(key,"\r\nContent-Length: ");
		key_len=18;	

		/*for search the real ip which may send rtp pkt later*/
		sprintf(ipkey, "IP4");
		ipkey_len = 3;

		//20170605LUKE: delete RTP connection when receive BYE command
		sprintf(byekey, "BYE");
		byekey_len = 3;
		//20180416LUKE: delete RTP connection when receive CANCEL command
		sprintf(cancelkey, "CANCEL");
		cancelkey_len = 6;
		sprintf(cseqkey, "CSeq: ");
		cseqkey_len = strlen(cseqkey);


		if(direct==NAPT_DIRECTION_INBOUND)
		{
			sprintf(viakey, "SIP/2.0/UDP");
			viakey_len = strlen(viakey);

			sprintf(rportkey, "rport=");
			rportkey_len = 6;
		}
		


		for(i=0,j=0;i<dataLen;i++,j++)
		{

			if(pData[i]<0x20&&pData[i]!=0x0d&&pData[i]!=0x0a)  return SUCCESS; //not sip packet

			if(i+4<=dataLen)
			{
				// search RTP Port number
				if(memcmp(&pData[i],"\r\nm=",4)==0)
				{
					int flag=0;
					
					/*get the rtp listening port*/
					if(pData[i+4]=='v')
					{
						sprintf(keyword[idx], "video");
						keyword_len[idx] = 5;
					}
					else if(pData[i+4]=='a')
					{
						sprintf(keyword[idx], "audio");
						keyword_len[idx] = 5;
					}
					
					for(k=i+4;k<dataLen;k++)
					{
						if(pData[k]==' ')
						{
							if(flag==0) flag=k;
							else
							{
								//printk("out- in_port=[%d] out_port=%d\n",i_port,o_port);
								//_rtl8651_addAlgQosUpnpMap(UPNP_PERSIST|UPNP_ALG_QOS, 0, 0, 
								//tb->insideGlobalIpAddr, i_port,
								//tb->insideLocalIpAddr, i_port,RTL8651_ALG_SIP_IDX);
								//o_port=0;
								rtp_cnt++;	//count how many port we need to setup!
								if(rtp_cnt>RG_ALG_SIP_MAX_RTP_CONN)
								{
									WARNING("[SIP ALG] outnumbered RTP channel:%d..",rtp_cnt);
									return 0;
								}
								break;
							}
						}
						else if(flag!=0)
						{
							if(direct==NAPT_DIRECTION_OUTBOUND)
								i_port[rtp_cnt]=(pData[k]-'0')+i_port[rtp_cnt]*10;
							else
								o_port[rtp_cnt]=(pData[k]-'0')+o_port[rtp_cnt]*10;
						}
					}
				}
				else if((direct==NAPT_DIRECTION_INBOUND)&&(memcmp(&pData[i],viakey,viakey_len)==0))
				{
					uint16 trueport_num=0;
					//search true port
					if (memcmp(&pData[i+viakey_len+1],globalip,globalip_len)==0)
					{
						for (k = i+viakey_len +globalip_len +2; k <dataLen; k++ )
						{
							if((pData[k] >= '0') && (pData[k] <= '9'))
							{	
								trueport_num=(pData[k]-'0')+trueport_num*10;
							}
							else 
								break;
						}
						ALG("[SIP IN]trueport_num = %d\n",trueport_num);
						if(trueport_num!=0&&trueport_num!=pConn->app.sip.trueport_num)pConn->app.sip.trueport_num=trueport_num;
						pConn->app.sip.samegw = TRUE;	
					}
					//search remote ip 
					else if(memcmp(&pData[i+viakey_len+1],remoteip,remoteip_len)!=0)
					{
						pConn->app.sip.samegw = FALSE;
					}
				}
				else if ((direct==NAPT_DIRECTION_INBOUND)&&((memcmp(&pData[i], ipkey, ipkey_len) ==0) && (memcmp(&pData[i + ipkey_len +1 ],globalip,globalip_len)!=0)))
				{
					//	printk("not the same gateway\n");
					uint16 temp=0;
					pConn->app.sip.trueremoteip=0;
					for(k= i+4; k<dataLen;k++)
					{
						if(pData[k]==' '||pData[k]==0xd)
						{
							pConn->app.sip.trueremoteip = (pConn->app.sip.trueremoteip << 8) + temp;
							break;
						}
						else if (pData[k] == '.')
						{
							pConn->app.sip.trueremoteip = (pConn->app.sip.trueremoteip << 8) + temp;
							temp = 0;
						}
						else if (pData[k] >= '0' && pData[k] <= '9') 
						{	
							temp=(pData[k]-'0')+temp*10;
							if (temp > 255)
								temp = temp/10;
						}
					}
					ALG("[SIP IN]trueremoteIP = %u.%u.%u.%u\n",
						(pConn->app.sip.trueremoteip>>24)&0xff,
						(pConn->app.sip.trueremoteip>>16)&0xff,
						(pConn->app.sip.trueremoteip>>8)&0xff,
						pConn->app.sip.trueremoteip&0xff);
					pConn->app.sip.samegw = FALSE;
				}
				else if((direct==NAPT_DIRECTION_OUTBOUND)&&(memcmp(&pData[i], ipkey, ipkey_len)==0))
				{
					uint16 temp=0;
					pConn->app.sip.trueip=0;
					for(k= i+4; k<dataLen;k++)
					{
						if(pData[k] ==' '||pData[k]==0xd)
						{
							pConn->app.sip.trueip = (pConn->app.sip.trueip << 8) + temp;
							break;
						}
						else if (pData[k] == '.')
						{
							pConn->app.sip.trueip = (pConn->app.sip.trueip << 8) + temp;
							temp = 0;
						}
						else if (pData[k] >= '0' && pData[k] <= '9') 
						{	
							temp=(pData[k]-'0')+temp*10;
							if (temp > 255)
								temp = temp/10;
						}
					}
					ALG("[SIP OUT]true ip is %u.%u.%u.%u\n", 
						(pConn->app.sip.trueip>>24)&0xff,
						(pConn->app.sip.trueip>>16)&0xff,
						(pConn->app.sip.trueip>>8)&0xff,
						pConn->app.sip.trueip&0xff);
				}
				else if((direct==NAPT_DIRECTION_OUTBOUND)&&( num_memcmp(&pData[i], localip, localip_len) ==0 ) && pData[i+localip_len]==':' &&(memcmp(&pData[i + localip_len + 1], localport, localport_len) != 0))
				{
					int flag=0;				
					pConn->app.sip.control_port=0;
					for(k=i + localip_len;k<i + localip_len +6;k++)
					{
						 if (pData[k] >= '0' && pData[k] <= '9') 
						{	
							flag = 1;
						}
						 else
						 	flag = 0;
							
						if (flag ==1 )
							pConn->app.sip.control_port=(pData[k]-'0')+pConn->app.sip.control_port*10;

					}
					if (pConn->app.sip.control_port < 1024)
						pConn->app.sip.control_port = 0;
					ALG("[SIP OUT]control port is %d\n", pConn->app.sip.control_port);
					sprintf(controlport, "%u", (pConn->app.sip.control_port)&0xffff);
					controlport_len = strlen(controlport); 
				}
				else if((memcmp(&pData[i], cseqkey, cseqkey_len)==0))
				{
					char *pch=strchr(&pData[i+cseqkey_len],' ');
					if(pch)
					{
						if(memcmp(pch+1, byekey, byekey_len)==0)
						{
							if(pConn->app.sip.byeDirect>=0)
							{
								if(pConn->app.sip.byeDirect!=direct){
									ALG("[SIP %s] receive BYE response, delete RTP connections.",direct==NAPT_DIRECTION_OUTBOUND?"OUT":"IN");
									_rtk_rg_sip_flowFlush(pConn);
									_rtk_rg_sip_appInit(pConn);
								}
							}
							else
							{
								ALG("[SIP %s] receive first BYE, wait until response..",direct==NAPT_DIRECTION_OUTBOUND?"OUT":"IN");
								pConn->app.sip.byeDirect=direct;
							}
						}
						else if(memcmp(pch+1, cancelkey, cancelkey_len)==0)
						{
							ALG("[SIP %s] receive CANCEL method, delete RTP connections.",direct==NAPT_DIRECTION_OUTBOUND?"OUT":"IN");
							_rtk_rg_sip_flowFlush(pConn);
							_rtk_rg_sip_appInit(pConn);
						}
					}
				}
	//			if (trueip != 0)
	//				break;
			}
		}

START_MODIFY:

		if((direct==NAPT_DIRECTION_OUTBOUND)&&(i_port[idx]!=0))
		{
			unsigned short extport=i_port[idx];
			ALG("[SIP OUT] iport = %d\n", i_port[idx]);
			if(pConn->app.sip.i_port[idx]!=i_port[idx])
			{
				pConn->app.sip.i_port[idx]=i_port[idx];
				//delete old  napt connection if any
				if(pConn->app.sip.flow_idx[idx]>=0)
					(pf.rtk_rg_naptConnection_del)(pConn->app.sip.flow_idx[idx]);
				pConn->app.sip.flow_idx[idx]=-1;

				//check if we already estabilish the napt connection with iport, oport, internalIP, remoteIP
				if(pConn->app.sip.o_port[idx]&&pConn->app.sip.trueremoteip)
					pConn->app.sip.flow_idx[idx] = _rtk_rg_naptTcpUdpOutHashIndexLookup(pConn->tuple.isTcp, pConn->tuple.internalIp.ip, pConn->app.sip.i_port[idx], pConn->app.sip.trueremoteip, pConn->app.sip.o_port[idx]);					

				if(pConn->app.sip.flow_idx[idx]<0)
				{
					if(_rtk_rg_algPreservePort_set(&extport, pConn->tuple.isTcp, rg_db.systemGlobal.alg_sip_presreve_timeout)!=RG_RET_SUCCESS)
					{
						WARNING("[SIP OUT] extPort is exhausted..");
						return 0;
					}
				}
				else	//use same extport from exist napt connection
					extport=rg_db.naptOut[pConn->app.sip.flow_idx[idx]].extPort;
				
				pConn->app.sip.extPort[idx]=extport;
			}
			ALG("[SIP OUT] extport = %d\n", pConn->app.sip.extPort[idx]);
			//if(newPort==FAIL) return SUCCESS;	
			//add UPnP connection here!!
			
			sprintf(trueport, "%u", (i_port[idx])&0xffff);
			trueport_len = strlen(trueport); 	
			sprintf(temp_trueport, " %u", (i_port[idx])&0xffff);
			temp_trueport_len = strlen(trueport)+1;	
			
			sprintf(rtpport, "%u", (pConn->app.sip.extPort[idx])&0xffff); //control flow ext port
			rtpport_len = strlen(rtpport);
		}


		if(direct==NAPT_DIRECTION_INBOUND)
		{
			if(o_port[idx]!=0&&o_port[idx]!=pConn->app.sip.o_port[idx])
			{
				pConn->app.sip.o_port[idx]=o_port[idx];
				//delete old  napt connection if any
				if(pConn->app.sip.flow_idx[idx]>=0)
					(pf.rtk_rg_naptConnection_del)(pConn->app.sip.flow_idx[idx]);
				pConn->app.sip.flow_idx[idx]=-1;
			
				//check if we already estabilish the napt connection with iport, oport, internalIP, remoteIP
				if(pConn->app.sip.i_port[idx])
					pConn->app.sip.flow_idx[idx] = _rtk_rg_naptTcpUdpOutHashIndexLookup(pConn->tuple.isTcp, pConn->tuple.internalIp.ip, pConn->app.sip.i_port[idx], pConn->app.sip.trueremoteip, pConn->app.sip.o_port[idx]);
				
				sprintf(rtpport, "%u", (o_port[idx])&0xffff); //external RTP Port
				rtpport_len = strlen (rtpport);
				sprintf(realrtpport, "%u", (pConn->app.sip.i_port[idx])&0xffff); //local RTP Port
				realrtpport_len = strlen(realrtpport);
				ALG("[SIP IN] oport = %d\n", o_port[idx]);
			}
			if (pConn->app.sip.trueport_num != 0)
			{
				//sip_tb = &entry;
				rtk_rg_lookupIdxReturn_t naptOutIdx;
				rtk_rg_fwdEngineReturn_t fwd_ret;
				ipaddr_t transIP;
				uint16 transPort;
				sprintf(realport, "%u", (pConn->app.sip.trueport_num)&0xffff);
				realport_len = strlen (realport); 

				naptOutIdx=_rtk_rg_naptTcpUdpInHashIndexLookup(0,pPktHdr->ipv4Sip,pPktHdr->sport,pConn->tuple.extIp.ip,pConn->app.sip.trueport_num);
				fwd_ret = _rtk_rg_fwdEngine_connType_lookup(pPktHdr,&transIP,&transPort);
				if(fwd_ret==RG_FWDENGINE_RET_CONTINUE) 
				{
					pConn->app.sip.truelocalip = transIP;
					sprintf(realip,"%u.%u.%u.%u",(pConn->app.sip.truelocalip>>24)&0xff,(pConn->app.sip.truelocalip>>16)&0xff,(pConn->app.sip.truelocalip>>8)&0xff,(pConn->app.sip.truelocalip)&0xff);
					realip_len=strlen(realip);
					ALG("[SIP IN] trueip = %x\n", pConn->app.sip.truelocalip);
				}
				else
				{
					ALG("[SIP IN] trueip not found!\n");
				}
			}
		}

		if(pConn->app.sip.i_port[idx]!=0 && pConn->app.sip.o_port[idx]!=0 && pConn->app.sip.flow_idx[idx]<0)
		{
			int netifidx=-1;
			//find the wan interface index
			for(i=0;i<rg_db.systemGlobal.wanIntfTotalNum;i++)
			{
				if(rg_db.systemGlobal.wanIntfGroup[i].p_wanIntfConf->wan_type==RTK_RG_BRIDGE)
					continue;
				if(rg_db.systemGlobal.wanIntfGroup[i].p_intfInfo->p_wanStaticInfo->ip_version==IPVER_V6ONLY)
					continue;
				if(rg_db.systemGlobal.wanIntfGroup[i].p_intfInfo->p_wanStaticInfo->ip_addr!=pConn->tuple.extIp.ip)
					continue;
				
				netifidx=rg_db.systemGlobal.wanIntfGroup[i].index;
				break;
			}
			ALG("[SIP] netif idx is %d",netifidx);
			//20171211LUKE: remove preserve ext port from list
			_rtk_rg_algPreservePort_delete(pConn->app.sip.extPort[idx], pConn->tuple.isTcp);
			
			//20170602LUKE: create napt connection when we receive o_port with different remote IP.
			bzero(&naptFlow,sizeof(rtk_rg_naptEntry_t));
			naptFlow.is_tcp = pConn->tuple.isTcp;
			naptFlow.local_ip = pConn->tuple.internalIp.ip;
			naptFlow.remote_ip = pConn->app.sip.trueremoteip;
			naptFlow.wan_intf_idx = netifidx;
			naptFlow.local_port = pConn->app.sip.i_port[idx];
			naptFlow.remote_port = pConn->app.sip.o_port[idx];
			naptFlow.external_port = pConn->app.sip.extPort[idx];
			naptFlow.outbound_pri_valid = rg_db.naptOut[pPktHdr->naptOutboundIndx].priValid;
			naptFlow.outbound_priority = rg_db.naptOut[pPktHdr->naptOutboundIndx].priValue;
			naptFlow.inbound_pri_valid = rg_db.naptIn[pPktHdr->naptrInboundIndx].priValid;
			naptFlow.inbound_priority = rg_db.naptIn[pPktHdr->naptrInboundIndx].priValue;
			(pf.rtk_rg_naptConnection_add)(&naptFlow,&pConn->app.sip.flow_idx[idx]);
			ALG("[SIP %s]add NAPT connection[%d] i_port=%d(%s) extPort=%d remote_port=%d\n",direct==NAPT_DIRECTION_INBOUND?"IN":"OUT",pConn->app.sip.flow_idx[idx],pConn->app.sip.i_port[idx],trueport,pConn->app.sip.extPort[idx],pConn->app.sip.o_port[idx]);
		}

		//change content!!
		for(i=0,j=0;i<dataLen;i++,j++)
		{	

			if((i+key_len<=dataLen)&&(memcmp(&pData[i],key,key_len)==0))
			{

				//find "\r\nContent-Length: "

				int32 datalen=0;
				int32 flag=0;
				int32 datalen_stringlen=0;
				char tempStr[16];
				int32 CntIpInMsgBody=0;
				int32 CntTrueIpInMsgbody=0;
				int32 CntRealIpInMsgBody=0; //for inbound
				int msg;
				
				memcpy(&sptr[j],key,key_len);
				j+=key_len;
				i+=key_len;
				for(k=i;((k<i+20)&&(k<dataLen));k++)
				{
					if((pData[k]>='0')&&(pData[k]<='9'))
					{
						datalen=pData[k]-'0'+datalen*10;
						flag = 1;
						datalen_stringlen = datalen_stringlen +1;
					}
					else
					{
						if (flag == 1)
							break;
					}
				}
				ALG("[SIP] Content-Length=%d datalen_stringlen=%d\n",datalen,datalen_stringlen);

				memcpy(&sptr[j], &pData[i], k -datalen_stringlen -i);
				j += k -datalen_stringlen -i;
				i += k -datalen_stringlen -i;
				
				old_cl_len=datalen_stringlen;
				i=k-1;
				msg=i+2;
				//find msg body
				for(k=i+2;k<=dataLen-4;k++)
				{					
					if(memcmp(&pData[k],"\r\n\r\n",4)==0)
					{
						msg=k+4;
						break;
					}					
				}

				if(direct==NAPT_DIRECTION_OUTBOUND)
				{
					// count how many ip in Message body
					for(k=msg;k<=dataLen-localip_len;k++)
					{			
						if ((num_memcmp(&pData[k],localip,localip_len)==0) /*&& (memcmp(&pData[k - ipkey_len -1], ipkey, ipkey_len) !=0)*/)
						{
							CntIpInMsgBody++;
						}
					}

					for(k=msg;k<=dataLen-trueport_len;k++)
					{

						if(pConn->app.sip.i_port[idx] != 0)
						{													
							//sample rate format="/XXXXX", media port format=" XXXX".
							// to avoid these two fields have the same value.
														
							//if(num_memcmp(&pData[k],trueport,trueport_len)==0)
							if(num_memcmp(&pData[k],temp_trueport,temp_trueport_len)==0)
							{
								CntPortInMsgBody[idx]+=(rtpport_len - trueport_len);
							}
						}
					}
					if ((pConn->app.sip.trueip!=0) && (pConn->app.sip.samesubnet == TRUE))
					{
						for(k=msg;k<=dataLen-realip_len;k++)
						{			
							if ((num_memcmp(&pData[k],realip,realip_len)==0) /*&& (memcmp(&pData[k - ipkey_len -1], ipkey, ipkey_len) !=0)*/)
							{
								CntTrueIpInMsgbody++;
								//printk("CntTrueIpInMsgbody = %d\n", CntTrueIpInMsgbody);
							}
						}
					}
					for(k=0;k<rtp_cnt;k++)
						datalen+=CntPortInMsgBody[k];
					new_cl_len=sprintf(tempStr,"%u",datalen+(CntIpInMsgBody*(globalip_len-localip_len)) + (CntTrueIpInMsgbody*(globalip_len - realip_len)));
					memcpy(&sptr[j],tempStr,new_cl_len);
					j+=(new_cl_len-1);
					ALG("[SIP OUT]CntIpInMsgBody = %d CntPortInMsgBody=%d, new_cl_len=%d\n",CntIpInMsgBody,CntPortInMsgBody[idx],new_cl_len);
				}
				else //inbound
				{

					// count how many ip in Message body
					for(k=msg;k<=dataLen-globalip_len;k++)
					{			
						if((num_memcmp(&pData[k],globalip,globalip_len)==0) && (memcmp(&pData[k - ipkey_len -1], ipkey, ipkey_len) !=0) && idx==0)
						{
							CntIpInMsgBody++;
						}
					}

					for(k=msg;k<=dataLen-globalip_len;k++)
					{			
						if((num_memcmp(&pData[k],globalip,globalip_len)==0) && (memcmp(&pData[k - ipkey_len -1], ipkey, ipkey_len) ==0) && (pConn->app.sip.truelocalip !=0) && idx==0)
						{
							CntRealIpInMsgBody++;
						}
					}


					if(datalen==0)
					{
						new_cl_len=1;
						tempStr[0]='0';
						tempStr[1]=0;
						ALG("[SIP OUT] content len=0, don't modify");
					}
					else
					{

						if (pConn->app.sip.samegw == FALSE)
							new_cl_len=sprintf(tempStr,"%u",datalen+(CntIpInMsgBody*(localip_len-globalip_len))+ (CntRealIpInMsgBody*(realip_len-globalip_len)));
						else if ((pConn->app.sip.o_port[idx] != 0) && (pConn->app.sip.trueport_num != 0))
							new_cl_len=sprintf(tempStr,"%u",datalen+(CntIpInMsgBody*(localip_len-globalip_len))+ (CntRealIpInMsgBody*(realip_len-globalip_len)) + (realrtpport_len-rtpport_len));
						else
							new_cl_len=sprintf(tempStr,"%u",datalen + (CntIpInMsgBody*(localip_len-globalip_len)));
					}
					memcpy(&sptr[j],tempStr,new_cl_len);
					j+=(new_cl_len-1);				
				}
			}	
			else if((direct==NAPT_DIRECTION_OUTBOUND)&&(i+localip_len<=dataLen)&&(num_memcmp(&pData[i],localip,localip_len)==0)/*&& (memcmp(&pData[i - ipkey_len -1], ipkey, ipkey_len) !=0)*/)
			{
				if(idx==0)
				{
					memcpy(&sptr[j],globalip,globalip_len);
					size_delta+=(globalip_len-localip_len);
				}
				i+=(localip_len-1);
				j+=(globalip_len-1);
				ALG("[SIP OUT] match local ip address = %s-->%s size_delta=%d\n",localip,globalip,size_delta);
			}

			else if((direct==NAPT_DIRECTION_OUTBOUND)&&(i+localport_len <= dataLen) && (num_memcmp(&pData[i], localport, localport_len) == 0) && (memcmp(&pData[i-localip_len -1], localip, localip_len) == 0))
			{
				if(idx==0)
				{
					memcpy(&sptr[j], globalport, globalport_len);
					size_delta += (globalport_len-localport_len);
				}
				i+=(localport_len-1);
				j+=(globalport_len-1);
				ALG("[SIP OUT] match local port & local ip address = %s-->%s size_delta=%d\n",localport,globalport,size_delta);
			}
#if 1
			else if((direct==NAPT_DIRECTION_OUTBOUND)&&(i+controlport_len <= dataLen) && (pConn->app.sip.control_port != 0) && (num_memcmp(&pData[i], controlport, controlport_len) == 0) && (memcmp(&pData[i-localip_len -1], localip, localip_len) == 0))
			{			
				if(idx==0)
				{
					memcpy(&sptr[j], globalport, globalport_len);
					size_delta += (globalport_len-controlport_len);
				}
				i+=(controlport_len-1);
				j+=(globalport_len-1);
				ALG("[SIP OUT] match control port & local ip address = c-port:%s-->c-extport:%s size_delta=%d dataLen=%d\n",controlport,globalport,size_delta,dataLen);
			}
#endif			
			else if((direct==NAPT_DIRECTION_OUTBOUND)&&(i+trueport_len <= dataLen) && (num_memcmp(&pData[i-1], temp_trueport, temp_trueport_len) == 0) && (pConn->app.sip.i_port[idx] != 0) /*&& ((memcmp(&pData[i -keyword_len -1], keyword, keyword_len) == 0))*/)
			{
				
				memcpy(&sptr[j], rtpport, rtpport_len);
				//printk("change rtp port\n");
				i+=(trueport_len-1);
				j+=(rtpport_len-1);
				size_delta += (rtpport_len-trueport_len);
				ALG("[SIP OUT] match trueport = %s-->%s size_delta=%d\n",trueport,rtpport,size_delta);
			}
			else if((direct==NAPT_DIRECTION_OUTBOUND)&&(i+realip_len <= dataLen) && (num_memcmp(&pData[i], realip, realip_len) == 0) && (pConn->app.sip.trueip != 0)  && (pConn->app.sip.samesubnet == TRUE)  && (memcmp(&pData[i - ipkey_len -1], ipkey, ipkey_len) ==0))
			{
				if(idx==0)
				{
					memcpy(&sptr[j], globalip, globalip_len);
					size_delta += (globalip_len-realip_len);
				}
				i+=(realip_len-1);
				j+=(globalip_len-1);
				ALG("[SIP OUT] match real ip = %s-->%s size_delta=%d\n",realip,globalip,size_delta);
				//printk("modify true ip\n");
			}

			else if((direct==NAPT_DIRECTION_INBOUND)&&((i+globalip_len<=dataLen))&&(num_memcmp(&pData[i],globalip,globalip_len)==0) &&(memcmp(&pData[i - ipkey_len -1], ipkey, ipkey_len) !=0))
			{
				if(idx==0)
				{
					memcpy(&sptr[j],localip,localip_len);
					size_delta+=(localip_len-globalip_len); 
				}
				i+=(globalip_len-1);
				j+=(localip_len-1);
				ALG("[SIP IN] match ip in address(without [IP4]) = %s-->%s\n",globalip,localip);					
			}
			else if((direct==NAPT_DIRECTION_INBOUND)&&((i+globalip_len<=dataLen))&&(num_memcmp(&pData[i],globalip,globalip_len)==0) &&(memcmp(&pData[i- ipkey_len -1], ipkey, ipkey_len) ==0) && (pConn->app.sip.truelocalip !=0))
			{
				if(idx==0)
				{
					memcpy(&sptr[j],realip,realip_len);
					size_delta+=(realip_len-globalip_len);
				}
				i+=(globalip_len-1);
				j+=(realip_len-1);						
				ALG("[SIP IN] match ip in address(with [IP4]) = %s-->%s\n",globalip,realip);
			}

			else if((direct==NAPT_DIRECTION_INBOUND)&&(i+globalport_len <= dataLen) && (num_memcmp(&pData[i], globalport, globalport_len) == 0))
			{
				if(idx==0)
				{
					memcpy(&sptr[j], controlport, controlport_len);
					size_delta += (controlport_len-globalport_len);
				}
				i+=(globalport_len-1);
				j+=(controlport_len-1);				
				ALG("[SIP IN] match global port = %s-->%s\n",globalport,controlport);
			}
			else if((direct==NAPT_DIRECTION_INBOUND)&&(i+realport_len <= dataLen) && (num_memcmp(&pData[i], realport, realport_len) == 0) && (pConn->app.sip.trueport_num != 0) && ((memcmp(&pData[i-rportkey_len], rportkey, rportkey_len) != 0)))
			{
				if(idx==0)
				{
					memcpy(&sptr[j], localport, localport_len);
					size_delta += (localport_len-realport_len);
				}
				i+=(realport_len-1);
				j+=(localport_len-1);
				ALG("[SIP IN] find real port by [rport=] = %s-->%s\n",realport,localport);
			}
			else if((direct==NAPT_DIRECTION_INBOUND)&&(i+rtpport_len <= dataLen) && (num_memcmp(&pData[i], rtpport, rtpport_len) == 0) && (pConn->app.sip.o_port[idx] != 0) && ((memcmp(&pData[i -keyword_len[idx] -1], keyword[idx], keyword_len[idx]) == 0)) && (pConn->app.sip.samegw == TRUE) && (pConn->app.sip.trueport_num != 0))
			{
				ALG("[SIP IN] match RTP port by [%s] = %s-->%s\n",keyword[idx],rtpport,realrtpport);
				memcpy(&sptr[j], realrtpport, realrtpport_len);
				//printk("change rtp port\n");
				i+=(rtpport_len-1);
				j+=(realrtpport_len-1);
				size_delta += (realrtpport_len-rtpport_len);
			}
			
			else if(idx==0)
			{
				sptr[j]=pData[i];
			}

		}

		//20171207LUKE: continue modify RTP port if needed.
		if(++idx<rtp_cnt)goto START_MODIFY;

		ALG("size_delta=%d new_cl_len=%d old_cl_len=%d\n",size_delta,new_cl_len,old_cl_len);
		spip->tot_len = htons(ntohs(spip->tot_len)+size_delta+new_cl_len-old_cl_len); 
		sudp->len = htons(ntohs(sudp->len)+size_delta+new_cl_len-old_cl_len);	

		skb_put(skb, size_delta);
		memcpy(&skb->data[pPktHdr->l3Offset],(void*)spip,ntohs(spip->tot_len));
		//skb->len=pPktHdr->l3Offset+ntohs(spip->tot_len);		

		//update l3 total length
		*pPktHdr->pL3Len = spip->tot_len;
		*pPktHdr->pIpv4Checksum=htons(_rtk_rg_fwdengine_L3checksumUpdateTotalLen(ntohs(*pPktHdr->pIpv4Checksum), pPktHdr->l3Len, ntohs(*pPktHdr->pL3Len)));
		pPktHdr->l3Len = ntohs(spip->tot_len);

		if(pPktHdr->egressTagif&PPPOE_TAGIF)
		{
#if !defined(CONFIG_RG_FLOW_BASED_PLATFORM)	// switch support pppoe tag offload
			// set pppoe length = IP total length + 2
			skb->data[pPktHdr->l3Offset-4]=htons(ntohs(spip->tot_len)+2)>>8;
			skb->data[pPktHdr->l3Offset-3]=htons(ntohs(spip->tot_len)+2)&0xff;
#endif //CONFIG_RG_FLOW_BASED_PLATFORM

#if defined(CONFIG_RG_RTL9600_SERIES)
			//20160331LUKE: checksum by sw offload
			//re-cal l3 checksum
			*pPktHdr->pIpv4Checksum=0;
			*pPktHdr->pIpv4Checksum=htons(inet_chksum(skb->data+pPktHdr->l3Offset,pPktHdr->l4Offset-pPktHdr->l3Offset));
		
			//re-cal l4 checksum
			//*pPktHdr->pL4Checksum=0;
			//*pPktHdr->pL4Checksum=htons(inet_chksum_pseudo(skb->data+pPktHdr->l4Offset,pPktHdr->l3Offset+(*pPktHdr->pL3Len)-pPktHdr->l4Offset,ntohl(*pPktHdr->pIpv4Sip),ntohl(*pPktHdr->pIpv4Dip),pPktHdr->ipProtocol));
#endif
		}
		else
		{
			//20171122LUKE: force l3 checksum update in case we xmit to wifi.
			ALG("ori_L3CheckSum=0x%x, L3CheckSum=0x%x, ori_L4CheckSum=0x%x, L4CheckSum=0x%x", pPktHdr->ipv4Checksum, *pPktHdr->pIpv4Checksum, pPktHdr->l4Checksum, *pPktHdr->pL4Checksum);
			pPktHdr->ipv4Checksum=*pPktHdr->pIpv4Checksum;
		}
		
		//memDump(skb->data,skb->len,"new sip out");

		//20171122LUKE: force l4 checksum recalculate in case we xmit to wifi.
		*pPktHdr->pL4Checksum=0;
		*pPktHdr->pL4Checksum=htons(inet_chksum_pseudo(skb->data+pPktHdr->l4Offset,pPktHdr->l3Offset+pPktHdr->l3Len-pPktHdr->l4Offset,ntohl(*pPktHdr->pIpv4Sip),ntohl(*pPktHdr->pIpv4Dip),pPktHdr->ipProtocol));
	
	}


	return 1;
}

int _rtk_rg_sip_handler(int direct, int after, unsigned char *pSkb,unsigned char *pPktInfo, unsigned char * pConnInfo)
{
	int ret = SUCCESS;
#ifdef __KERNEL__
	unsigned char * pData, *pAppData;
	unsigned int appLen=0,dataOff=0;
	rtk_rg_pktHdr_t *pPktHdr;
	struct sk_buff *skb;
	rtk_rg_alg_connection_t * pConn;
	
	pPktHdr = (rtk_rg_pktHdr_t *)pPktInfo;
	pConn = (rtk_rg_alg_connection_t *)pConnInfo;
	skb= (struct sk_buff *)pSkb;
	
	pData=skb->data;
	if(pPktHdr->tagif&TCP_TAGIF)
		dataOff = pPktHdr->l4Offset + pPktHdr->headerLen;
	else
		dataOff = pPktHdr->l4Offset + 8; /*udp header length is 8 bytes*/
	
	appLen = skb->len - dataOff;
	pAppData = pData + dataOff;


	//do nothing before napt modification
	if(after == 0)
	{
		if(direct==NAPT_DIRECTION_INBOUND)
		{
			if(pConn->tuple.isIp6==0)
			{
				pConn->tuple.extIp.ip = ntohl(*pPktHdr->pIpv4Dip);
				pConn->tuple.extPort = ntohs(*pPktHdr->pDport);
			}		
		}
		return SUCCESS;
	}


	//extIP= egress gateway ip 
	if(direct==NAPT_DIRECTION_OUTBOUND)
	{		
		if(pConn->tuple.isIp6==0)
		{
			pConn->tuple.extIp.ip = ntohl(*pPktHdr->pIpv4Sip);
			pConn->tuple.extPort = ntohs(*pPktHdr->pSport); 
		}
	}

	pConn->skb=pSkb;
	pConn->pPktHdr=pPktInfo;

	ret = _rtk_rg_sip_process(direct, pAppData, appLen, pConn);
	if(ret < 1)
		return FAIL;
#endif	
	return ret;
}

rtk_rg_fwdEngineAlgReturn_t rtk_rg_algRegFunc_sip(int direct, int after, unsigned char *pSkb,unsigned char *pPktInfo)
{
//Attention: caller function needs to make sure it needs to do napt modification
//ipv6 address and port doesn't need to do napt modification now
#ifdef __KERNEL__
	int ret;
	rtk_rg_pktHdr_t *pPktHdr;
	rtk_rg_alg_connection_t * pConn;
	rtk_rg_alg_tuple_t tuple;
	pPktHdr = (rtk_rg_pktHdr_t *)pPktInfo;
	memset(&tuple, 0, sizeof(rtk_rg_alg_tuple_t));

	/* malloc global variable */
	if(!init_sip){
		_rtl8651_l4_sip_alias_buf[0] = (uint8 *)rtk_rg_malloc(_RTL8651_L4_MAX_SIP_ALIAS_BUF_SIZE);
		//assert(_rtl8651_l4_sip_alias_buf[0]);
		init_sip = 1;
	}
	
	if(after==0)
	{
		//Pre function
		if(direct==NAPT_DIRECTION_OUTBOUND)
		{
			_rtk_rg_alg_init_tuple(direct, after, pPktHdr, &tuple);
			
			pConn = _rtk_rg_alg_connection_find(&tuple);
			if(pConn == NULL)
			{
				pConn = _rtk_rg_alg_connection_add(&tuple);	
				_rtk_rg_sip_appInit(pConn);
			}
			//else
				//_rtk_rg_sync_tcp_ack_seq(direct, pPktHdr, pConn);
		}
	}
	else //after=1
	{
		if(direct==NAPT_DIRECTION_OUTBOUND)
		{		
			_rtk_rg_alg_init_tuple(direct, after, pPktHdr, &tuple);
			
			pConn = _rtk_rg_alg_connection_find(&tuple);		
			if(pConn == NULL)
				return RG_FWDENGINE_ALG_RET_FAIL;
			
			ret = _rtk_rg_sip_handler(direct, after, pSkb, pPktInfo, (unsigned char *)pConn);	
		}
		else
		{
			//Attention, pPktHdr->ipv4Dip is not the original external ip after napt modification, it is the internal ip
			_rtk_rg_alg_init_tuple(direct, after, pPktHdr, &tuple);
	
			pConn = _rtk_rg_alg_connection_find(&tuple);
			if(pConn == NULL)
				return RG_FWDENGINE_ALG_RET_FAIL;

			ret = _rtk_rg_sip_handler(direct, after, pSkb, pPktInfo, (unsigned char *)pConn);

			//_rtk_rg_sync_tcp_ack_seq(direct, pPktHdr, pConn);
		}
	}

#endif
	return RG_FWDENGINE_ALG_RET_SUCCESS;
}
