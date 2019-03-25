/*
 * Copyright (C) 2016 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 40647 $
 * $Date:  $
 *
 * Purpose : Main function of the PTP Slave protocol stack user application
 *           It create four additional threads for packet Rx ,adjust freq ,adjust offset state control
 *
 * Feature : Start point of the PTP Slave protocol stack. Use individual threads
 *           for packet Rx ,adjust freq ,adjust offset and state control
 *
 */
#ifdef CONFIG_RTK_PTP_SLAVE
/*
 * Include Files
 */
#include <stdio.h>
#include <semaphore.h>
#include <string.h> 
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <malloc.h> 
#include <sys/socket.h> 
#include <linux/netlink.h> 
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include <rtk/time.h>

#include <common/rt_error.h>
#include <common/rt_type.h>
#include <common/rt_error.h>
#include <common/util/rt_util.h>
#include <common/debug/rt_log.h>
#include <common/debug/mem.h>

#include "ptp_slave_rx.h"
#include "ptp_slave_main.h"
#include "ptp_slave_calc.h"

/*
 * Symbol Definition
 */
 
/*
 * Macro Definition
 */
#define SAMPLE_MAX_NUM 100
/*
 * Data Declaration
 */
static int msgQId=-1;
static double preRatio=0;
static uint8 clockid[10]={0};

static uint8 offsetIdx=0;
static int64 sampleOffset[SAMPLE_MAX_NUM];
static uint64 round_num=1;
static int64 preAvgOffset=0;
static int64 specialOffset=0;

static int64 preT1=0;
static int64 preT2=0;
static int64 preT3=0;
static int64 preT4=0;

rtk_enable_t adj_freq=ENABLED;
uint8 sample_num=1;
rtk_enable_t adj_offset=ENABLED;


/*
 * Function Declaration
 */
int32 ptp_delay_msg_send(ptpFDMsgBuf_t *ptpFDMsgBuf_ptr)
{
    if(ptpFDMsgBuf_ptr==NULL)
    {
        printf("[PTP Slave Delay:%s:%d] Error delayTime_ptr is NULL!\n", __FUNCTION__, __LINE__);
        return RT_ERR_INPUT;
    }

    if(msgQId==-1)
    {
        printf("[PTP Slave Delay:%s:%d] Error queue is not ready!\n", __FUNCTION__, __LINE__);
        return RT_ERR_QUEUE_ID;
    }

    ptpFDMsgBuf_ptr->mtype=MQ_SLAVE_DELAY_TIME;

    if(msgsnd(msgQId,(void *)ptpFDMsgBuf_ptr,sizeof(ptp_delayTime_t),IPC_NOWAIT)<0)
    {
        printf("[PTP Slave Delay:%s:%d] Error message send!\n", __FUNCTION__, __LINE__);
        return RT_ERR_INPUT;
    }

    return RT_ERR_OK;
}

static void ptp_set_offest(int64 offset)
{
    uint32 sign;
    rtk_time_timeStamp_t timeStamp;
    double ratio;
    int i;
    int64 avgOffset=0;
    int32 ret = RT_ERR_OK;

    /*sample offset*/
    sampleOffset[offsetIdx++]=offset;
    if(offsetIdx>=sample_num)
    {
        if(sample_num>=2)
        {
            avgOffset=0;
            DBG_PRINT(1,"Round %lld Sample num=%d offset\n",round_num++,sample_num);
            for(i=0;i<sample_num;i++)
            {
                DBG_PRINT(1,"[%03d]=%09lld ",i,sampleOffset[i]);
                if((i+1)%3==0)
                    DBG_PRINT(1,"\n");
                avgOffset=avgOffset+sampleOffset[i];
            }
            if(i%3!=0)
                DBG_PRINT(1,"\n");
            avgOffset=avgOffset/sample_num;
            
            DBG_PRINT(1,"Average Offset=%09lld preAvgOffset=%09lld\n",avgOffset,preAvgOffset);

            if(adj_offset==ENABLED)
            {
                if(preAvgOffset==0)
                {
                    preAvgOffset=avgOffset;
                    specialOffset=0;
                }
                else
                {
                    if(llabs(avgOffset)<1000000 && llabs(preAvgOffset)<1000000)
                    {
                        if(((avgOffset>=preAvgOffset) && (avgOffset-preAvgOffset<1000000))
                            ||((preAvgOffset>=avgOffset) && (preAvgOffset-avgOffset<1000000)))
                        {
                            specialOffset=((avgOffset*7+preAvgOffset*3)/15);
                            DBG_PRINT(1,"Special adjust Offset=%lld\n",specialOffset);
                        }
                    }
                    preAvgOffset=avgOffset;
                }
            }
        }
        offsetIdx=0;
    }
    
    if(adj_offset==ENABLED)
    {
        offset=offset+specialOffset;
        DBG_PRINT(1,"adjust Offset to=%lld\n",offset);
    }
    
    /*setting offset*/
    if(offset<0)
    {
        sign = PTP_REF_SIGN_POSTIVE;
        timeStamp.sec=llabs(offset)/1e9;
        timeStamp.nsec=(uint32)(llabs(offset)-(timeStamp.sec*1e9));
        
    }
    else if(offset>0)
    {
        sign = PTP_REF_SIGN_NEGATIVE;
        timeStamp.sec=offset/1e9;
        timeStamp.nsec=(uint32)(offset-(timeStamp.sec*1e9));
    }
    else
    {
        DBG_PRINT(1,"%s timeStamp.sec=0 timeStamp.nsec=0.000000000\n","no need adjust");
        return ;
    }

    DBG_PRINT(1,"%s timeStamp.sec=%lld timeStamp.nsec=%09d\n",(sign==PTP_REF_SIGN_POSTIVE)?"increase":"decrease", timeStamp.sec,timeStamp.nsec);

    RTK_API_ERR_CHK(rtk_time_refTime_set(sign, timeStamp),ret);
}

static void ptp_set_freq_by_ratio(double ratio)
{
    double adjRatio=0;
    uint32 oriFreq,adjFreq;
    int i;
    int32 ret = RT_ERR_OK;
        
    if(ratio>1.100000 || ratio <0.900000)
    {
        preRatio=0;
        DBG_PRINT(1,"freqRatio=%.9llf is not in range 0.9~1.1\n",ratio);
        return ;
    }

    if(preRatio==0)
    {
        adjRatio=ratio;
    }
    else
    {
        adjRatio=preRatio*0.3+ratio*0.7;
    }
        
    DBG_PRINT(1,"freqRatio=%.9llf preFreqRatio=%.9llf adjFreqRatio=%.9llf\n",ratio,preRatio,adjRatio);
    
    preRatio=adjRatio;

    if(adjRatio<0.000001)
    {
        DBG_PRINT(1,"adjFreqRatio is zero\n");
        return ;
    }

    if(adjRatio<1.000000001 && adjRatio>0.999999999)
    {
        DBG_PRINT(1,"adjFreqRatio is too small\n");
        return RT_ERR_OK;
    }

    RTK_API_ERR_CHK(rtk_time_frequency_get(&oriFreq),ret);

    adjFreq=(uint32)((double)oriFreq*adjRatio);
    
    if(adjFreq==0)
    {
        DBG_PRINT(1,"adjFreq is zero\n");
        return ;
    }
    
    if(adjFreq==oriFreq)
    {
        DBG_PRINT(1,"adjFreq=%d is the same oriFreq\n",adjFreq);
        return ;
    }

    DBG_PRINT(1,"adjFreq=%d oriFreq=%d\n",adjFreq,oriFreq);
    
    RTK_API_ERR_CHK(rtk_time_frequency_set(adjFreq),ret);

    return ;
}

void *ptp_slave_calcThread(void *argu)
{
    int msgLen,i;
    ptpFDMsgBuf_t pmsg;
    ptp_delayTime_t *delayTime_ptr;
    ptp_ts_t masterTs,slaveTs;
    int64 mT,sT;
    double ratio;
    ptp_ts_t T1s,T2s,T3s,T4s;
    int64 T1,T2,T3,T4,delay,offset;
    
    delayTime_ptr=&pmsg.mtext;

    msgQId = msgget(PTP_SLAVE_CALC_MSG_KEY , 666 | IPC_CREAT);
    if(msgQId==-1)
    {
        printf("[PTP Slave Freq:%s:%d] Can't initialize message queue!\n", __FUNCTION__, __LINE__);
    }
    while(1)
    {
        msgLen = msgrcv(msgQId,(void *)&pmsg, sizeof(ptp_delayTime_t), MQ_SLAVE_F_PRI_MAX, MSG_NOERROR);
        if(msgLen <= 0)
        {
            printf("[PTP Slave Freq:%s:%d] No message from queue!\n", __FUNCTION__, __LINE__);
            continue;
        }
        
        switch(pmsg.mtype)
        {
            case MQ_SLAVE_DELAY_TIME:                
                T1s.sec=0;
                for(i=0;i<6;i++)
                {
                    T1s.sec=T1s.sec*256+delayTime_ptr->t1.sec[i];
                }
                
                T1s.nsec=0;
                for(i=0;i<4;i++)
                {
                    T1s.nsec=T1s.nsec*256+delayTime_ptr->t1.nanosec[i];
                }
                
                T1=T1s.sec;
                T1=T1*1e9;
                T1=T1+T1s.nsec;

                T2s.sec=0;
                for(i=0;i<6;i++)
                {
                    T2s.sec=T2s.sec*256+delayTime_ptr->t2.sec[i];
                }
                
                T2s.nsec=0;
                for(i=0;i<4;i++)
                {
                    T2s.nsec=T2s.nsec*256+delayTime_ptr->t2.nanosec[i];
                }
                
                T2=T2s.sec;
                T2=T2*1e9;
                T2=T2+T2s.nsec;

                T3s.sec=0;
                for(i=0;i<6;i++)
                {
                    T3s.sec=T3s.sec*256+delayTime_ptr->t3.sec[i];
                }
                
                T3s.nsec=0;
                for(i=0;i<4;i++)
                {
                    T3s.nsec=T3s.nsec*256+delayTime_ptr->t3.nanosec[i];
                }
                
                T3=T3s.sec;
                T3=T3*1e9;
                T3=T3+T3s.nsec;

                T4s.sec=0;
                for(i=0;i<6;i++)
                {
                    T4s.sec=T4s.sec*256+delayTime_ptr->t4.sec[i];
                }
                
                T4s.nsec=0;
                for(i=0;i<4;i++)
                {
                    T4s.nsec=T4s.nsec*256+delayTime_ptr->t4.nanosec[i];
                }
                
                T4=T4s.sec;
                T4=T4*1e9;
                T4=T4+T4s.nsec;

                DBG_PRINT(1,"T1=%lld\n",T1);
                DBG_PRINT(1,"T2=%lld\n",T2);
                DBG_PRINT(1,"T3=%lld\n",T3);
                DBG_PRINT(1,"T4=%lld\n",T4);

                if(T2==T3)
                {
                    DBG_PRINT(1,"T2==T3\n");
                    break;
                }
                
                /* delay= ((t2 - t3) + (t4 - t1)) / 2 */
                delay=((T2-T3)+(T4-T1))/2;
                
                /* offset = t2 - t1 - delay */
                offset=(T2-T1)-delay;

                DBG_PRINT(1,"delay=%lld offset=%lld\n",delay,offset);

                ptp_set_offest(offset);

                if(adj_freq==ENABLED)
                {
                    /*set frequency*/
                    if(preT1==0 || T1<=preT1 || preT2==0 || T2<=preT2
                        ||preT3==0 || T3<=preT3 || preT4==0 || T4<=preT4)
                    {
                        preT1=T1;
                        preT2=T2;
                        preT3=T3;
                        preT4=T3;
                        break;
                    }

                    ratio=((double)((double)(T1-preT1))/((double)(T2-preT2+offset)) + (double)((double)(T4-preT4))/((double)(T3-preT3+offset)))/2;

                    ptp_set_freq_by_ratio(ratio);
                    
                    preT1=T1;
                    preT2=T2;
                    preT3=T3;
                    preT4=T4;
                }
                break;
            default:
                break;
        }
    }
} 
#endif
