
#ifndef _IP_SPEEDUP_H
#define _IP_SPEEDUP_H

#include <linux/skbuff.h>

//#define IP_SPEEDUP_DEBUG

enum SPEEDUP_CHECK_RST {
	SPEEDUP_RET_UNMATCH,
	SPEEDUP_RET_MATCH_NO_ACK,
	SPEEDUP_RET_MATCH_QUEUED,
	SPEEDUP_RET_MATCH_ACK
};

#ifdef IP_SPEEDUP_DEBUG
extern unsigned int g_vec_nr;
extern unsigned int get_softirq_delay_stats(int vec_nr);
#endif
extern unsigned short FAST_ACK_WIN_SIZE;

/*
direction:
1, Up stream
2, Down stream
*/
extern int shouldBypassSpeedUP(void *pHostTrack);
extern int isHostCheck(struct sk_buff *skb, int hook, int direction);
extern int isHostSpeedUpEnable(void);
extern void * shouldSpeedUp(unsigned int dir, unsigned int dstip, unsigned int srcip, unsigned short dport, unsigned short sport);
extern int speedtest_stream_shortcut_check(void *pHostTrack, u32 dstip, u32 srcip, u16 dport, u16 sport, 
												u32 *seqNo, u16 *id, u32 dataLen, struct sk_buff *skb);
extern int send_delay_ack_timer(void);
extern int re8670_speedup_ack_send(struct sk_buff *skb, unsigned int seqNo, unsigned short ip_id);
extern void speedtest_sample_enter(int pktLen);
extern void fast_ack_calibration_enter(int pktLen);
extern int HostSpeedUP_Stat_get(u32 dstip, u32 srcip, u16 dport, u16 sport);
extern unsigned int get_speedtest_time(void);
extern unsigned int get_time_duration(unsigned int startTime);

extern int HostSpeedUP_proc_init(void);
#endif /*_IP_SPEEDUP_H*/

