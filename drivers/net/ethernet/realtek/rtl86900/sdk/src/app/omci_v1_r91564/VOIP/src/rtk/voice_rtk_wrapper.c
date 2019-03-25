/*
 * Copyright (C) 2014 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : Definition callback API for rtk snooping mode
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1)
 */

#include "voice_rtk_wrapper.h"
#include "feature_mgmt.h"
#include <voip_flash.h>
#include <voip_control.h>

voipCfgParam_t *g_pFlashCfgVoIP = NULL;
voip_state_share_t *g_pLineStateVoIP = NULL;

//voipCfgParam_t *g_pShareCfgVoIP = NULL;
//voip_flash_share_t omci_VoIPShareT;
//voip_flash_share_t *omci_VoIPShare_p = &omci_VoIPShareT;
static BOOL     gRtk_voice_enable = FALSE;

#define DYN_PT 0xff  //use this value to distinguish dynamic payload and static payload

typedef struct codec_list
{
    const UINT8 codec_mib_index; //codec mib index
    const UINT8 payload_type;
    const CHAR  name[32];
    const UINT8 default_frame_size;
    const UINT8 basic_frame_size;
//  UINT8 precedence;
} CODEC_LIST_T, *CODEC_LIST_Tp;

typedef struct codec_desc
{
    UINT8 CodecSelection;
    UINT8 PacketPeriodSelection;
    UINT8 SilenceSuppression;
}CODEC_DESC_T, *CODEC_Desc_Tp;

typedef struct codec_dispatch_s
{
    UINT8 chid;
    CODEC_DESC_T codec_desc[4];
}CODEC_DISPATCH_T, *CODEC_Dispatch_Tp;

typedef struct rtp_dscp_list
{
    const UINT8 class;
    const UINT8 dscp;
} DSCP_LIST_T, *DSCP_LIST_Tp;

#if 1
static DSCP_LIST_T rtp_dscp_map[] =
{
    { 0 , 0x0  } ,
    { 1 , 0x8  } ,
    { 2 , 0x10 } ,
    { 3 , 0x18 } ,
    { 4 , 0x20 } ,
    { 5 , 0x28 } ,
    { 6 , 0x30 } ,
    { 7 , 0x38 } ,
    { 8 , 0x2e } ,
};
#endif


const static CODEC_LIST_T codec_map[] =
{
    {SUPPORTED_CODEC_G711U,OMCI_CODEC_PCMU      , "G.711u"  , 20 , 10} ,
    {SUPPORTED_CODEC_G711A,OMCI_CODEC_PCMA      , "G.711a"  , 20 , 10} ,
    {SUPPORTED_CODEC_G729 ,OMCI_CODEC_G729      , "G.729"   , 20 , 10} ,
    {SUPPORTED_CODEC_G722 ,OMCI_CODEC_G722      , "G.722"   , 20 , 10} ,
};



//omci_InitVoIPCfg
GOS_ERROR_CODE voice_rtk_init_voip_cfg(void)
{
    OMCI_PRINT("####################### %s() #######################", __FUNCTION__);

    if (!g_pFlashCfgVoIP ||!g_pLineStateVoIP)
    {
        voice_cfg_msg_t voiceInfo;
        memset(&voiceInfo, 0, sizeof(voice_cfg_msg_t));
        voiceInfo.op_id     = VOICE_OP_GET_SHM;
        voiceInfo.cfg.ppCfg = (void **)(&g_pFlashCfgVoIP);

        if (FAL_OK != feature_api(FEATURE_API_VOICE_RTK_CFG_OP, &voiceInfo, sizeof(voice_cfg_msg_t)))
        {
            OMCI_PRINT("voip flash get VOICE_OP_GET_SHM failed.");
            return GOS_FAIL;
        }

        g_pFlashCfgVoIP = *((voipCfgParam_t **)(voiceInfo.cfg.ppCfg));

        memset(&voiceInfo, 0, sizeof(voice_cfg_msg_t));
        voiceInfo.op_id     = VOICE_OP_INIT_VARS;

        if (FAL_OK != feature_api(FEATURE_API_VOICE_RTK_CFG_OP, &voiceInfo, sizeof(voice_cfg_msg_t)))
        {
            OMCI_PRINT("voip flash set VOICE_OP_INIT_VARS failed.");
            return GOS_FAIL;
        }

        //memset(&voiceInfo, 0, sizeof(voice_cfg_msg_t));
        //voiceInfo.op_id        = VOICE_OP_GET_SHARE;
        //voiceInfo.cfg.pFlash   = (void *)(&omci_VoIPShareT);

        //if (FAL_OK != feature_api(FEATURE_API_VOICE_RTK_CFG_OP, &voiceInfo, sizeof(voice_cfg_msg_t)))
        //{
            //OMCI_PRINT("voip flash get VOICE_OP_GET_SHARE failed.");
            //return GOS_FAIL;
        //}

        //omci_VoIPShareT = *((voip_flash_share_t *)(voiceInfo.cfg.pFlash));

        memset(&voiceInfo, 0, sizeof(voice_cfg_msg_t));
        voiceInfo.op_id        = VOICE_OP_GET_LINE_STATE;
        voiceInfo.cfg.ppSts    =(void **)(&g_pLineStateVoIP);

        if (FAL_OK != feature_api(FEATURE_API_VOICE_RTK_CFG_OP, &voiceInfo, sizeof(voice_cfg_msg_t)))
        {
            OMCI_PRINT("voip flash get VOICE_OP_GET_LINE_STATE failed.");
            return GOS_FAIL;
        }

        g_pLineStateVoIP = *((voip_state_share_t **)(voiceInfo.cfg.ppSts));

        //g_pShareCfgVoIP = &omci_VoIPShare_p->voip_cfg;

        OMCI_PRINT("### %s()###, pFlash=%p, g_pLineState=%p", __FUNCTION__, g_pFlashCfgVoIP,g_pLineStateVoIP);

        gRtk_voice_enable = TRUE;
    }

    return GOS_OK;
}

//omci_SaveVoIPCfg
GOS_ERROR_CODE voice_rtk_save_voip_cfg(void)
{
    voice_cfg_msg_t voiceInfo;
    memset(&voiceInfo, 0, sizeof(voice_cfg_msg_t));
    voiceInfo.op_id        = VOICE_OP_SET_SHM;
    voiceInfo.cfg.ppCfg    = (void **)(&g_pFlashCfgVoIP);

    if (FAL_OK != feature_api(FEATURE_API_VOICE_RTK_CFG_OP, &voiceInfo, sizeof(voice_cfg_msg_t)))
    {
        OMCI_PRINT("voip flash VOICE_OP_SET_SHM failed.");
        return GOS_FAIL;
    }


    memset(&voiceInfo, 0, sizeof(voice_cfg_msg_t));
    voiceInfo.op_id         = VOICE_OP_SET_SHARE;
    //voiceInfo.cfg.pFlash    = (void *)(&omci_VoIPShareT);

    if (FAL_OK != feature_api(FEATURE_API_VOICE_RTK_CFG_OP, &voiceInfo, sizeof(voice_cfg_msg_t)))
    {
        OMCI_PRINT("voip flash VOICE_OP_SET_SHARE failed.");
        return GOS_FAIL;
    }

    return GOS_OK;
}

//omci_RestartCallMgr
GOS_ERROR_CODE voice_rtk_restart_call_mgr(void)
{
    GOS_ERROR_CODE ret = GOS_OK;
    static UINT32  omci_set_cnt = 0;
    int h_pipe, res;

    if (gRtk_voice_enable)
    {
        ODBG_G("omci_set_cnt=%u\n" , omci_set_cnt++);
        // restart solar, it will reload flash settings

        system("mib voipshmupdate");

        if (access("/var/run/solar_control.fifo", F_OK) != 0)
        {

		  if (mkfifo("/var/run/solar_control.fifo", 0755) == -1){
	       fprintf(stderr, "Create  %s failed\n", "/var/run/solar_control.fifo");
            return GOS_FAIL;
        }


		}

		

        h_pipe = open("/var/run/solar_control.fifo", O_WRONLY | O_NONBLOCK);

        if (h_pipe == -1)
        {
            fprintf(stderr, "open %s failed\n", "/var/run/solar_control.fifo");
            return GOS_FAIL;
        }

        res = write(h_pipe, "x\n", 2);

        if (res == -1)
        {
            fprintf(stderr, "write %s failed\n", "/var/run/solar_control.fifo");
            close(h_pipe);
            return GOS_FAIL;
        }

        close(h_pipe);

    }

    return ret;
}
GOS_ERROR_CODE voice_rtk_config_reset(void)
{
    GOS_ERROR_CODE ret = GOS_OK;
    int ch_id, proxy_id;
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;
        for(ch_id=0;ch_id<VOIP_PORTS;ch_id++)
        {
            for(proxy_id=0;proxy_id<MAX_PROXY;proxy_id++)
            {
                strcpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].number,"");
                strcpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].display_name, "");
                strcpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].login_id, "");
                strcpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].password, "");
                strcpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].addr ,"");
                strcpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].outbound_addr ,"");
                g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].outbound_port=5060;
                g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].outbound_enable=0;
                strcpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].domain_name,"");
                //g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].enable=0;
            }
        }
        voice_rtk_save_voip_cfg();
//		voice_rtk_restart_call_mgr();
    }
    return ret;
}
GOS_ERROR_CODE voice_rtk_fax_mode_set(UINT16 ch_id, UINT8 fax_mode)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        g_pFlashCfgVoIP->ports[ch_id].useT38 = fax_mode;
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_oob_dtmf_set(UINT16 ch_id, UINT8 oob_dtmf)
{

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        if (1 == oob_dtmf)
            g_pFlashCfgVoIP->ports[ch_id].dtmf_mode = DTMF_RFC2833;
        else
            g_pFlashCfgVoIP->ports[ch_id].dtmf_mode = DTMF_INBAND;
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_codec_sel_order_set(
    UINT16 ch_id, UINT16 order, UINT8 codec_payload_type, UINT8 pkt_period_sel_order)
{
    UINT32 i  ;
    const UINT32 codec_map_size = (sizeof(codec_map)/sizeof(codec_map[0]));

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        if (pkt_period_sel_order<10)
        {
            ODBG_R("invalid pkt_period_sel_order=%u\n" ,pkt_period_sel_order);
            return GOS_FAIL;
        }


#if 0

        for ( i = 0 ; i < SUPPORTED_CODEC_MAX ; i++ )
        {
            ODBG_R("before chid=%u precedence[%u]=%u frame_size[%u]=%u\n" ,
                   ch_id,
                   i, g_pFlashCfgVoIP->ports[ch_id].precedence[i] ,
                   i, g_pFlashCfgVoIP->ports[ch_id].frame_size[i]);
        }

#endif

        for ( i = 0 ; i < codec_map_size; i++ )
        {

            if ( codec_map[i].payload_type == codec_payload_type )
            {

                g_pFlashCfgVoIP->ports[ch_id].precedence[codec_map[i].codec_mib_index] = order;
                //ptime
                g_pFlashCfgVoIP->ports[ch_id].frame_size[codec_map[i].codec_mib_index] = pkt_period_sel_order / codec_map[i].basic_frame_size - 1;

            }
        }


#if 0

        for ( i = 0 ; i < codec_map_size; i++ )
        {
            if ( codec_map[i].payload_type == codec_payload_type )
            {
                for ( j = 0 ; j < codec_map_size ; j++ )
                {
                    if ( g_pFlashCfgVoIP->ports[ch_id].precedence[j] == order )
                    {
                        // swap codec
                        tmp = g_pFlashCfgVoIP->ports[ch_id].precedence[i] ;
                        g_pFlashCfgVoIP->ports[ch_id].precedence[i] = g_pFlashCfgVoIP->ports[ch_id].precedence[j];
                        g_pFlashCfgVoIP->ports[ch_id].precedence[j] = tmp;

                        // setup ptime
                        g_pFlashCfgVoIP->ports[ch_id].frame_size[i] = pkt_period_sel_order / codec_map[i].basic_frame_size - 1;
                        break;
                    }
                }
            }
        }

#endif
#if 0

        for ( i = 0 ; i < codec_map_size; i++ )
        {
            ODBG_R("after chid=%u precedence[%u]=%u frame_size[%u]=%u\n" ,
                   ch_id,
                   i, g_pFlashCfgVoIP->ports[ch_id].precedence[i] ,
                   i, g_pFlashCfgVoIP->ports[ch_id].frame_size[i]);
        }

#endif
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_codec_sel_order_set_all(UINT16 chid, UINT8 *p_order_addr, UINT8 order_size)
{
    CODEC_PREDED_T codec_preced;
    CODEC_DISPATCH_T codec_dispatch , codec_dispatch_tmp;
    INT32 i , j ,input;
    UINT32 k;
    INT32 codec_mib_index=-1 , codec_mib_victim_index=-1;
    INT32 tmp;
    const UINT32 codec_map_size = (sizeof(codec_map)/sizeof(codec_map[0]));

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        memset(&codec_preced,0,sizeof(codec_preced));
        memcpy(&codec_preced , p_order_addr , order_size);

        memset(&codec_dispatch , 0 , sizeof(codec_dispatch));
        memset(&codec_dispatch_tmp , 0 , sizeof(codec_dispatch_tmp));

        codec_dispatch_tmp.chid = chid;
        codec_dispatch_tmp.codec_desc[0].CodecSelection         = codec_preced.CodecSelection1stOrder;
        codec_dispatch_tmp.codec_desc[0].PacketPeriodSelection  = codec_preced.PacketPeriodSelection1stOrder;
        codec_dispatch_tmp.codec_desc[0].SilenceSuppression     = codec_preced.SilenceSuppression1stOrder;
        codec_dispatch_tmp.codec_desc[1].CodecSelection         = codec_preced.CodecSelection2ndOrder;
        codec_dispatch_tmp.codec_desc[1].PacketPeriodSelection  = codec_preced.PacketPeriodSelection2ndOrder;
        codec_dispatch_tmp.codec_desc[1].SilenceSuppression     = codec_preced.SilenceSuppression2ndOrder;
        codec_dispatch_tmp.codec_desc[2].CodecSelection         = codec_preced.CodecSelection3rdOrder;
        codec_dispatch_tmp.codec_desc[2].PacketPeriodSelection  = codec_preced.PacketPeriodSelection3rdOrder;
        codec_dispatch_tmp.codec_desc[2].SilenceSuppression     = codec_preced.SilenceSuppression3rdOrder;
        codec_dispatch_tmp.codec_desc[3].CodecSelection         = codec_preced.CodecSelection4thOrder;
        codec_dispatch_tmp.codec_desc[3].PacketPeriodSelection  = codec_preced.PacketPeriodSelection4thOrder;
        codec_dispatch_tmp.codec_desc[3].SilenceSuppression     = codec_preced.SilenceSuppression4thOrder;

        codec_dispatch.chid = chid;

        i=0;
        input=0;
loop_codec_dispatch_init:
        while(i<4) // 4 codec precedences
        {
            // remove ptime 0 case
            if(codec_dispatch_tmp.codec_desc[i].PacketPeriodSelection == 0){
                i++;
                goto loop_codec_dispatch_init;
        }

            // remove codec selection which is the same with previous setting
            for( j=i ; j>0 ; j-- )
            {
                //OMCI_PRINT("%s(%d)i=%d CodecSelection=%d , j=%d CodecSelection=%d" , __FUNCTION__ , __LINE__ ,
                //  i , codec_dispatch_tmp.codec_desc[i].CodecSelection , j , codec_dispatch_tmp.codec_desc[j].CodecSelection);

                if(codec_dispatch_tmp.codec_desc[j-1].CodecSelection == codec_dispatch_tmp.codec_desc[i].CodecSelection)
        {
                    //OMCI_PRINT("%s(%d) skip i=%d CodecSelection=%d , j=%d CodecSelection=%d" , __FUNCTION__ , __LINE__ ,
                    //  i , codec_dispatch_tmp.codec_desc[i].CodecSelection , j , codec_dispatch_tmp.codec_desc[j].CodecSelection);
                    i++;
                    goto loop_codec_dispatch_init;
                }
            }

            codec_dispatch.codec_desc[input].CodecSelection         = codec_dispatch_tmp.codec_desc[i].CodecSelection       ;
            codec_dispatch.codec_desc[input].PacketPeriodSelection  = codec_dispatch_tmp.codec_desc[i].PacketPeriodSelection;
            codec_dispatch.codec_desc[input].SilenceSuppression     = codec_dispatch_tmp.codec_desc[i].SilenceSuppression   ;

            //OMCI_PRINT("%s(%d)i=%d " , __FUNCTION__ , __LINE__ , i);
            //OMCI_PRINT("%s(%d)codec_dispatch.codec_desc[%d].CodecSelection=%d"        ,__FUNCTION__ , __LINE__ , i , codec_dispatch.codec_desc[i].CodecSelection);
            //OMCI_PRINT("%s(%d)codec_dispatch.codec_desc[%d].PacketPeriodSelection=%d" ,__FUNCTION__ , __LINE__ , i , codec_dispatch.codec_desc[i].PacketPeriodSelection);
            //OMCI_PRINT("%s(%d)codec_dispatch.codec_desc[%d].SilenceSuppression=%d"    ,__FUNCTION__ , __LINE__ , i , codec_dispatch.codec_desc[i].SilenceSuppression);

            //OMCI_PRINT("%s(%d)input=%d " , __FUNCTION__ , __LINE__ , input);
            //OMCI_PRINT("%s(%d)codec_dispatch.codec_desc[%d].CodecSelection=%d"        ,__FUNCTION__ , __LINE__ , input , codec_dispatch.codec_desc[input].CodecSelection);
            //OMCI_PRINT("%s(%d)codec_dispatch.codec_desc[%d].PacketPeriodSelection=%d" ,__FUNCTION__ , __LINE__ , input , codec_dispatch.codec_desc[input].PacketPeriodSelection);
            //OMCI_PRINT("%s(%d)codec_dispatch.codec_desc[%d].SilenceSuppression=%d"    ,__FUNCTION__ , __LINE__ , input , codec_dispatch.codec_desc[input].SilenceSuppression);

            input++;
            i++;
        }

        OMCI_PRINT("chid=%u"                          , chid);
        OMCI_PRINT("codec_preced.chid=%u"             , codec_preced.chid);
        OMCI_PRINT("CodecSelection1stOrder=%u"        , codec_preced.CodecSelection1stOrder);
        OMCI_PRINT("PacketPeriodSelection1stOrder=%u" , codec_preced.PacketPeriodSelection1stOrder);
        OMCI_PRINT("SilenceSuppression1stOrder=%u"    , codec_preced.SilenceSuppression1stOrder);
        OMCI_PRINT("CodecSelection2ndOrder=%u"        , codec_preced.CodecSelection2ndOrder);
        OMCI_PRINT("PacketPeriodSelection2ndOrder=%u" , codec_preced.PacketPeriodSelection2ndOrder);
        OMCI_PRINT("SilenceSuppression2ndOrder=%u"    , codec_preced.SilenceSuppression2ndOrder);
        OMCI_PRINT("CodecSelection3rdOrder=%u"        , codec_preced.CodecSelection3rdOrder);
        OMCI_PRINT("PacketPeriodSelection3rdOrder=%u" , codec_preced.PacketPeriodSelection3rdOrder);
        OMCI_PRINT("SilenceSuppression3rdOrder=%u"    , codec_preced.SilenceSuppression3rdOrder);
        OMCI_PRINT("CodecSelection4thOrder=%u"        , codec_preced.CodecSelection4thOrder);
        OMCI_PRINT("PacketPeriodSelection4thOrder=%u" , codec_preced.PacketPeriodSelection4thOrder);
        OMCI_PRINT("SilenceSuppression4thOrder=%u"    , codec_preced.SilenceSuppression4thOrder);


        OMCI_PRINT("codec_dispatch.chid=%d", codec_dispatch.chid);
        for(i=0 ; i<4 ; i++)
        {
            OMCI_PRINT("codec_dispatch.codec_desc[%d].CodecSelection=%d"        , i , codec_dispatch.codec_desc[i].CodecSelection);
            OMCI_PRINT("codec_dispatch.codec_desc[%d].PacketPeriodSelection=%d" , i , codec_dispatch.codec_desc[i].PacketPeriodSelection);
            OMCI_PRINT("codec_dispatch.codec_desc[%d].SilenceSuppression=%d"    , i , codec_dispatch.codec_desc[i].SilenceSuppression);
        }

#if 0 //DEBUG
        OMCI_PRINT("SUPPORTED_CODEC_MAX=%u" , SUPPORTED_CODEC_MAX);

        for ( i = 0 ; i < SUPPORTED_CODEC_MAX ; i++ )
        {
            OMCI_PRINT("before chid=%u precedence[%u]=%u frame_size[%u]=%u" ,
                   chid,
                   i, g_pFlashCfgVoIP->ports[chid].precedence[i] ,
                   i, g_pFlashCfgVoIP->ports[chid].frame_size[i]);
        }
#endif

        i=-1;
loop_start:
        while (++i < 4) // 4 codec precedences
        {
            codec_mib_index = -1;
            codec_mib_victim_index = -1;

            // remove ptime 0 case
            if (codec_dispatch.codec_desc[i].PacketPeriodSelection != 0)
            {

                // remove codec selection which is the same with previous setting
                for (j = i; j > 0; j--)
                {
                    if (codec_dispatch.codec_desc[j - 1].CodecSelection == codec_dispatch.codec_desc[i].CodecSelection)
                    {
                        //OMCI_PRINT("skip same codec selection i=%d j=%d" , i , j);
                        goto loop_start;
                    }
                }

                // find codec and switch codec precedence
                for (k = 0; k < codec_map_size; k++)
                {

                    // find codec in flash mib index
                    if (codec_dispatch.codec_desc[i].CodecSelection == codec_map[k].payload_type)
                    {
                        codec_mib_index = codec_map[k].codec_mib_index;
                    }

                    // find victim codec's flash mib index
                    if (i == g_pFlashCfgVoIP->ports[chid].precedence[k])
                    {
                        codec_mib_victim_index = k;
                    }
                }

                //OMCI_PRINT("codec_mib_index=%d codec_mib_victim_index=%d" , codec_mib_index , codec_mib_victim_index);

                // if codec already in correct precedence , just setup ptime
                if(codec_mib_index >= 0)
                {
                    if (g_pFlashCfgVoIP->ports[chid].precedence[codec_mib_index] == i)
                    {
                        g_pFlashCfgVoIP->ports[chid].frame_size[codec_mib_index] =
                            codec_dispatch.codec_desc[i].PacketPeriodSelection / codec_map[i].basic_frame_size - 1;
                    }
                    else
                    {
                        if ((codec_mib_victim_index >= 0) && (codec_mib_index != codec_mib_victim_index))
                        {
                            tmp = g_pFlashCfgVoIP->ports[chid].precedence[codec_mib_index];
                            g_pFlashCfgVoIP->ports[chid].precedence[codec_mib_index] = g_pFlashCfgVoIP->ports[chid].precedence[codec_mib_victim_index];
                            g_pFlashCfgVoIP->ports[chid].precedence[codec_mib_victim_index] = tmp;

                            g_pFlashCfgVoIP->ports[chid].frame_size[codec_mib_index] =
                                codec_dispatch.codec_desc[i].PacketPeriodSelection / codec_map[i].basic_frame_size - 1;
                        }
                    }
                }
            }
            else
            {
                OMCI_PRINT("skip ptime 0. index=%d", i);
            }
        }


#if 0 //DEBUG
        for ( i = 0 ; i < SUPPORTED_CODEC_MAX ; i++ )
        {
            OMCI_PRINT("after chid=%u precedence[%u]=%u frame_size[%u]=%u" ,
                   chid,
                   i, g_pFlashCfgVoIP->ports[chid].precedence[i] ,
                   i, g_pFlashCfgVoIP->ports[chid].frame_size[i]);
        }
#endif
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_min_local_port_set(UINT16 ch_id, UINT16 min_local_port)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        g_pFlashCfgVoIP->ports[ch_id].media_port = min_local_port;
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_dscp_mark_set(UINT8 dscp_mark)
{
    UINT32 i;

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        g_pFlashCfgVoIP->rtpDscp = dscp_mark;

#if 1

        for (i = 0; i < (sizeof(rtp_dscp_map)/sizeof(rtp_dscp_map[0])); i++)
        {
            if ( rtp_dscp_map[i].dscp == dscp_mark )
            {
                g_pFlashCfgVoIP->rtpDscp = rtp_dscp_map[i].class;
                ODBG_Y("DSCP Class=%u\n" , rtp_dscp_map[i].class);
                break;
            }
        }

#endif
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_echo_cancel_ind_set(UINT16 ch_id, UINT8 echo_cancel_ind)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        g_pFlashCfgVoIP->ports[ch_id].lec = echo_cancel_ind;
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_user_part_aor_set(
    UINT16 ch_id, UINT32 proxy_id, CHAR *pPart_start, UINT8 part_num, UINT32 part_len)
{
    UINT32 i, offset;
    OMCI_PRINT("####%s#####,ch_id=%d proxy_id=%d pPart_start=%s size=%d\n",__FUNCTION__,ch_id, proxy_id,pPart_start,part_len);
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        for (i = 0; i < part_num; i++)
        {
            //ODBG_Y("%s\n" , lstr_ptr);
            //strncpy( g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].number + (i * offset), lstr_ptr , DNS_LEN );

            //TBD: check oversize
            offset = part_len * i;

            strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].number + offset,
                    pPart_start + offset + i, part_len);
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_sip_display_name_set(
    UINT16 ch_id, UINT32 proxy_id, CHAR *pSip_display_name, UINT32 size)
{
    UINT32 len;

    OMCI_PRINT("####%s#####,ch_id=%d proxy_id=%d pSip_display_name=%s size=%d\n",__FUNCTION__,ch_id, proxy_id,pSip_display_name,size);

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;
        if(strcmp(pSip_display_name,"0")==0)
        {
            OMCI_PRINT("pSip_display_name = 0 dont need save\n");
            return GOS_OK;
        }
        len = (size > DNS_LEN - 1 ? DNS_LEN - 1 : size + 1);
        strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].display_name, pSip_display_name, len);
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_user_name_set(
    UINT16 ch_id, UINT32 proxy_id, CHAR *pUser_name, CHAR *pUser_name2,UINT32 size)
{
    UINT32 len;
OMCI_PRINT("####%s#####,ch_id=%d proxy_id=%d pUser_name=%s pUser_name2 =%s size=%d\n",__FUNCTION__,ch_id, proxy_id,pUser_name,pUser_name2,size);
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        len = (size  >= DNS_LEN-1 ? DNS_LEN-1 : size + 1);

        strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].login_id, pUser_name, len);
	 if (strlen(pUser_name2)!=0)
	 {
	 	if (strlen(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].login_id) + strlen(pUser_name2) < sizeof(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].login_id))
	 		strcat(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].login_id, pUser_name2);
	 }	
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_password_set(
    UINT16 ch_id, UINT32 proxy_id, CHAR *pUser_name, UINT32 size)
{
    UINT32 len;
    OMCI_PRINT("####%s#####,ch_id=%d proxy_id=%d pUser_name=%s size=%d\n",__FUNCTION__,ch_id, proxy_id,pUser_name,size);
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        len = (size  >= DNS_LEN-1 ? DNS_LEN-1 : size + 1);

        strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].password, pUser_name, len);
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_call_wait_feature_set(UINT16 ch_id, UINT8 call_wait_feature)
{

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        unsigned long org_fxs_port_flag = g_pFlashCfgVoIP->ports[ch_id].fxsport_flag;

        g_pFlashCfgVoIP->ports[ch_id].call_waiting_enable   = ((0 != (call_wait_feature & (VOIP_APP_SVC_PROFILE_CALL_WAITING_FEATURE_CALL_WAITING))) ? 1 : 0);
        g_pFlashCfgVoIP->ports[ch_id].fxsport_flag          = ((0 == (call_wait_feature & (VOIP_APP_SVC_PROFILE_CALL_WAITING_FEATURE_CALLER_ID_ANNOUNCEMENT))) ?
                (org_fxs_port_flag | FXS_CALLERID_HIDDEN) :
                (org_fxs_port_flag & (~FXS_CALLERID_HIDDEN)));
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_direct_connect_feature_set(UINT16 ch_id, UINT8 direct_connect_feature)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        unsigned long org_fxs_port_flag = g_pFlashCfgVoIP->ports[ch_id].fxsport_flag;

        g_pFlashCfgVoIP->ports[ch_id].fxsport_flag = ((0 != (direct_connect_feature & (VOIP_APP_SVC_PROFILE_DIRECT_CONNECT_FEATURE_ENABLED))) ?
                (org_fxs_port_flag & (~FXS_REJECT_DIRECT_IP_CALL)) :
                (org_fxs_port_flag | FXS_REJECT_DIRECT_IP_CALL));
    }

    return GOS_OK;
}


GOS_ERROR_CODE voice_rtk_proxy_server_ip_set(
    UINT16 ch_id, UINT32 proxy_id, CHAR *pPart_start, UINT8 part_num, UINT32 part_len)
{
    UINT32 i, offset;
    char proxy[80]={0};
    char proxy_ip[80]={0};
    char proxy_port[80]={0};
    char* tmp=NULL;
OMCI_PRINT("####%s#####,ch_id=%d proxy_id=%d pPart_start=%s part_num=%d part_len=%d\n",__FUNCTION__,ch_id, proxy_id,pPart_start,part_num,part_len);
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        for (i = 0; i < part_num; i++)
        {
            //ODBG_Y("%s\n" , lstr_ptr);
            //strncpy( g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].number + (i * offset), lstr_ptr , DNS_LEN );

            //TBD: check oversize
            offset = part_len * i;

            strncpy(proxy + offset,
                    pPart_start + offset + i, part_len);
        }

        if (strchr(proxy, ':')!=NULL)
        {
            tmp=strchr(proxy, ':');
            strncpy(proxy_ip, proxy, tmp-proxy);
            //strcpy(proxy_port, tmp+1);
            snprintf(proxy_port, sizeof(proxy_port), "%s", tmp+1);
        }else
            strcpy(proxy_ip, proxy);
        OMCI_PRINT("####proxy_ip=%s proxy_port=%s#####,\n",proxy_ip,proxy_port);
        if (strlen(proxy_ip)!=0)
        {
            //strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].addr  ,proxy_ip, DNS_LEN);
            snprintf(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].addr, DNS_LEN, "%s", proxy_ip);
        }

        if (strlen(proxy_port)!=0)
        {
            g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].port=atoi(proxy_port);
        }

    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_outbound_proxy_ip_set(
    UINT16 ch_id, UINT32 proxy_id, CHAR *pPart_start, UINT8 part_num, UINT32 part_len)
{
    UINT32 i, offset;
    char out_bound_proxy[80]={0};
    char out_bound_proxy_ip[DNS_LEN]={0};
    char out_bound_proxy_port[DNS_LEN]={0};
    char* tmp=NULL;

    OMCI_PRINT("####%s#####,ch_id=%d proxy_id=%d pPart_start=%s part_num=%d part_len=%d\n",__FUNCTION__,ch_id, proxy_id,pPart_start,part_num,part_len);

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        for (i = 0; i < part_num; i++)
        {
            //ODBG_Y("%s\n" , lstr_ptr);
            //strncpy( g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].number + (i * offset), lstr_ptr , DNS_LEN );

            //TBD: check oversize
            offset = part_len * i;

            strncpy(out_bound_proxy + offset, pPart_start + offset + i, part_len);
        }



        if (strchr(out_bound_proxy, ':')!=NULL)
        {
            tmp=strchr(out_bound_proxy, ':');
            strncpy(out_bound_proxy_ip, out_bound_proxy, tmp-out_bound_proxy);
            //strcpy(out_bound_proxy_port, tmp+1);
            snprintf(out_bound_proxy_port, sizeof(out_bound_proxy_port), "%s", tmp+1);
        }else
            strcpy(out_bound_proxy_ip, out_bound_proxy);
        OMCI_PRINT("####out_bound_proxy_ip=%s out_bound_proxy_port=%s#####,\n",out_bound_proxy_ip,out_bound_proxy_port);
        if (strlen(out_bound_proxy_ip)!=0)
        {
            //strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].outbound_addr ,out_bound_proxy_ip, DNS_LEN);
            snprintf(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].outbound_addr, DNS_LEN, "%s", out_bound_proxy_ip);
        }

        if (strlen(out_bound_proxy_port)!=0)
        {
            g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].outbound_port=atoi(out_bound_proxy_port);
        }
        g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].outbound_enable = 1;
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_sip_reg_exp_time_set(UINT16 ch_id, UINT32 proxy_id, UINT32 sip_reg_exp_time)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].reg_expire = sip_reg_exp_time;
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_reg_head_start_time_set(UINT16 ch_id, UINT32 proxy_id, UINT32 reg_head_start_time)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].RegisterRetryInterval = reg_head_start_time;
    }

    return GOS_OK;
}


GOS_ERROR_CODE voice_rtk_proxy_tcp_udp_port_set(UINT16 ch_id, UINT32 proxy_id, UINT16 proxy_tcp_udp_port)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].port = proxy_tcp_udp_port;
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_proxy_domain_name_set(
    UINT16 ch_id, UINT32 proxy_id, CHAR *pPart_start, UINT8 part_num, UINT32 part_len)
{
    UINT32 i, offset;
    char domain_name[80]={0};

    OMCI_PRINT("####%s#####,ch_id=%d proxy_id=%d pPart_start=%s part_num=%d part_len=%d\n",__FUNCTION__,ch_id, proxy_id,pPart_start,part_num,part_len);
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        if (0 == part_num)
        {
            if (strcmp(pPart_start, "0.0.0.0")==0)
            {
                OMCI_PRINT("domain_name = 0.0.0.0 dont need save\n");
                return GOS_OK;
            }
            strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].domain_name, pPart_start, part_len + 1);
        }
        else
        {
            if(part_len*part_num>80)
            {
                OMCI_PRINT("####%s#####,total size oversize =%d\n",__FUNCTION__,part_len*part_num);
                return GOS_FAIL;
            }

            for (i = 0; i < part_num; i++)
            {
                offset = part_len * i;

                strncpy(domain_name + offset,pPart_start + offset + i, part_len);
            }

            if (strlen(domain_name) > sizeof(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].domain_name))
            {
                OMCI_PRINT("####%s#####,oversize =%d\n",__FUNCTION__,strlen(domain_name));
                strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].domain_name,domain_name, DNS_LEN);
                g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].domain_name[DNS_LEN-1]='\0';
            }
            else
            {
                strncpy(g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].domain_name,domain_name, strlen(domain_name));
            }
            OMCI_PRINT("####%s#####,save  domain_name=%s\n",__FUNCTION__,g_pFlashCfgVoIP->ports[ch_id].proxies[proxy_id].domain_name);
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_pots_state_set(UINT16 ch_id, UINT8 state)
{

    if (gRtk_voice_enable)
    {
        OMCI_PRINT("####%s#####  state=%d\n",__FUNCTION__,state);

        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        //eric: only for main proxy 0

        if(state==OMCI_ME_ATTR_ADMIN_STATE_UNLOCK)  //from omci ,value = 0
        {
            g_pFlashCfgVoIP->ports[ch_id].proxies[0].enable |= PROXY_ENABLED;
        }
        else
        {
            g_pFlashCfgVoIP->ports[ch_id].proxies[0].enable &= ~PROXY_ENABLED;
        }
    OMCI_PRINT("g_pFlashCfgVoIP->ports[ch_id].proxies[0].enable is %d , \n",g_pFlashCfgVoIP->ports[ch_id].proxies[0].enable);
    voice_rtk_save_voip_cfg();
//		voice_rtk_restart_call_mgr();

    }
    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_rtp_stat_clear(UINT16 ch_id)
{
    // TBD: this function should be removed if rtp stat is read clear.
    return GOS_OK;
}
GOS_ERROR_CODE voice_rtk_CCPM_stat_clear(UINT16 ch_id)
{
    if (gRtk_voice_enable)
    {
        if (!g_pLineStateVoIP)
            return GOS_FAIL;

	   OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  \n",__FUNCTION__);
	    g_pLineStateVoIP->CCPMHistoryData[ch_id].setupFailures=0;
	    g_pLineStateVoIP->CCPMHistoryData[ch_id].setupTimer=0;
	    g_pLineStateVoIP->CCPMHistoryData[ch_id].terminateFailures=0;
	    g_pLineStateVoIP->CCPMHistoryData[ch_id].portReleases=0;
	    g_pLineStateVoIP->CCPMHistoryData[ch_id].portOffhookTimer=0;
    }
	return GOS_OK;
}
GOS_ERROR_CODE voice_rtk_SIPAGPM_stat_clear(UINT16 ch_id)
{
    if (gRtk_voice_enable)
    {
        if (!g_pLineStateVoIP)
            return GOS_FAIL;
	    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  \n",__FUNCTION__);	
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxInviteReqs=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxInviteRetrans=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxNoninviteReqs=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxNoninviteRetrans=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxResponse=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxResponseRetransmissions=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txInviteReqs=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txInviteRetrans=0;		
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txNoninviteReqs=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txNoninviteRetrans=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txResponse=0;
	    g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txResponseRetransmissions=0;
    }
	return GOS_OK;
}
GOS_ERROR_CODE voice_rtk_SIPCIPM_stat_clear(UINT16 ch_id)
{
	if (gRtk_voice_enable)
	{
       	 if (!g_pLineStateVoIP)
			return GOS_FAIL;
		
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  \n",__FUNCTION__);		
		g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].failedConnectCounter=0;
		g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].failedValidateCounter=0;
		g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].timeoutCounter=0;
		g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].failureReceivedCounter=0;
		g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].failedAuthenticateCounter=0;
	}
	return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_jitter_set(UINT16 ch_id, UINT32 type, UINT16 jitter)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        if (OMCI_VOIP_JITTER_TARGET == type)
            g_pFlashCfgVoIP->ports[ch_id].jitter_delay = jitter;

        if (OMCI_VOIP_JITTER_MAX == type)
            g_pFlashCfgVoIP->ports[ch_id].maxDelay = jitter;
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_hook_flash_time_set(UINT16 ch_id, UINT32 type, UINT16 hook_time)
{
    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        if (OMCI_VOIP_HOOK_TIME_MIN == type)
        {
            if(hook_time<80)
            {
                OMCI_PRINT("voip flash_hook_min timer < 80, set failed.");
            }
            else
            {
                g_pFlashCfgVoIP->ports[ch_id].flash_hook_time_min = hook_time;
            }
        }

        if (OMCI_VOIP_HOOK_TIME_MAX == type)
        {
            if((hook_time>2000)||(hook_time==0))
            {
                OMCI_PRINT("voip flash_hook_max timer > 2000, set failed");
            }
            else
            {
                g_pFlashCfgVoIP->ports[ch_id].flash_hook_time = hook_time;
            }
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_progress_or_transfer_feature_set(UINT16 ch_id, UINT16 call_transfer)
{

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        unsigned long org_fxs_port_flag = g_pFlashCfgVoIP->ports[ch_id].fxsport_flag;

        g_pFlashCfgVoIP->ports[ch_id].fxsport_flag = ((0 != (call_transfer & (VOIP_APP_SVC_PROFILE_CALL_PROG_OR_TRANSFER_FEATURE_CALL_TRANSFER))) ?
                (org_fxs_port_flag & (~FXS_CALL_TRANSFER_disable)) :
                (org_fxs_port_flag | FXS_CALL_TRANSFER_disable));

        org_fxs_port_flag = g_pFlashCfgVoIP->ports[ch_id].fxsport_flag;


        g_pFlashCfgVoIP->ports[ch_id].fxsport_flag = ((0 != (call_transfer & (VOIP_APP_SVC_PROFILE_CALL_PROG_OR_TRANSFER_FEATURE_3WAY))) ?
                (org_fxs_port_flag & (~FXS_3WAY_DISABLE)) :
                (org_fxs_port_flag | FXS_3WAY_DISABLE));
    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_dial_plan_set(
    UINT16 ch_id, UINT8 *pPart_start, UINT32 part_len)
{

    if (gRtk_voice_enable)
    {
        if (!g_pFlashCfgVoIP)
            return GOS_FAIL;

        g_pFlashCfgVoIP->ports[ch_id].digitmap_enable = TRUE;

        memcpy(g_pFlashCfgVoIP->ports[ch_id].dialplan, pPart_start, part_len);

    }

    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_config_method_set(UINT8 config_method)
{

	if (gRtk_voice_enable)
	{
		if (!g_pFlashCfgVoIP)
			return GOS_FAIL;
		
		if (config_method==VCD_CFG_METHOD_USED_OMCI)
			g_pFlashCfgVoIP->rfc_flags |= SIP_CONFIG_BY_OMCI;
		else if (config_method==VCD_CFG_METHOD_USED_TR069)
			g_pFlashCfgVoIP->rfc_flags &= ~SIP_CONFIG_BY_OMCI;
		else
			g_pFlashCfgVoIP->rfc_flags &= ~SIP_CONFIG_BY_OMCI;
		
		if (g_pFlashCfgVoIP->rfc_flags& SIP_CONFIG_BY_OMCI)
		{
			OMCI_PRINT("####%s##### SIP_CONFIG_BY_OMCI config_method=%d\n",__FUNCTION__, config_method);
		}
		else
		{
			OMCI_PRINT("####%s##### SIP_CONFIG_BY_TR069  config_method=%d\n",__FUNCTION__, config_method);
		}
		voice_rtk_save_voip_cfg();
	}

	return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_hook_state_get (UINT16 ch_id,UINT8* hook_state)
{

    if (gRtk_voice_enable)
    {
    if (!g_pLineStateVoIP)
        return GOS_FAIL;

    *hook_state=g_pLineStateVoIP->HookState[ch_id];

    OMCI_PRINT("####%s##### hook_state=%d hook_state=%d\n",__FUNCTION__,g_pLineStateVoIP->HookState[ch_id], *hook_state);
    }
    return GOS_OK;
}
GOS_ERROR_CODE voice_rtk_VOIP_line_status_get(omci_VoIP_line_status_t *pOmci_VoIP_line_status)
{
    UINT16 ch_id;

    ch_id = pOmci_VoIP_line_status->ch_id;
    if (gRtk_voice_enable)
    {
        if (!g_pLineStateVoIP)
            return GOS_FAIL;
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s##### voipCodecUsed=%d \n",__FUNCTION__,g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCodecUsed);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s##### voipVoiceServerStatus=%d \n",__FUNCTION__,g_pLineStateVoIP->VOIPLineStatus[ch_id].voipVoiceServerStatus);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s##### voipPortSessionType=%d \n",__FUNCTION__,g_pLineStateVoIP->VOIPLineStatus[ch_id].voipPortSessionType);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s##### voipCall1PacketPeriod=%d \n",__FUNCTION__,g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCall1PacketPeriod);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s##### voipCall2PacketPeriod=%d \n",__FUNCTION__,g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCall2PacketPeriod);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s##### voipCall1DestAddr=%s \n",__FUNCTION__,g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCall1DestAddr);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s##### voipCall2DestAddr=%s \n",__FUNCTION__,g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCall2DestAddr);
        pOmci_VoIP_line_status->voipCodecUsed = g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCodecUsed;
        pOmci_VoIP_line_status->voipVoiceServerStatus = g_pLineStateVoIP->VOIPLineStatus[ch_id].voipVoiceServerStatus;
        pOmci_VoIP_line_status->voipPortSessionType = g_pLineStateVoIP->VOIPLineStatus[ch_id].voipPortSessionType;
        pOmci_VoIP_line_status->voipCall1PacketPeriod =g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCall1PacketPeriod;
        pOmci_VoIP_line_status->voipCall2PacketPeriod = g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCall2PacketPeriod;
        strncpy(pOmci_VoIP_line_status->voipCall1DestAddr, g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCall1DestAddr, OMCI_VOIP_CALL_DST_ADDR_LEN);
        strncpy(pOmci_VoIP_line_status->voipCall2DestAddr, g_pLineStateVoIP->VOIPLineStatus[ch_id].voipCall2DestAddr, OMCI_VOIP_CALL_DST_ADDR_LEN);
    }
    return GOS_OK;
}

GOS_ERROR_CODE voice_rtk_CCPM_history_data_get(omci_CCPM_history_data_t *pOmci_CCPM_history_data)
{
    UINT16 ch_id;

    ch_id = pOmci_CCPM_history_data->ch_id;
    if (gRtk_voice_enable)
    {
        if (!g_pLineStateVoIP)
            return GOS_FAIL;

        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  \n",__FUNCTION__);
        pOmci_CCPM_history_data->setupFailures = g_pLineStateVoIP->CCPMHistoryData[ch_id].setupFailures;
        pOmci_CCPM_history_data->setupTimer = g_pLineStateVoIP->CCPMHistoryData[ch_id].setupTimer;
        pOmci_CCPM_history_data->terminateFailures = g_pLineStateVoIP->CCPMHistoryData[ch_id].terminateFailures;
        pOmci_CCPM_history_data->portReleases =g_pLineStateVoIP->CCPMHistoryData[ch_id].portReleases;
        pOmci_CCPM_history_data->portOffhookTimer = g_pLineStateVoIP->CCPMHistoryData[ch_id].portOffhookTimer;
    }
    return GOS_OK;
}
GOS_ERROR_CODE voice_rtk_RTPPM_history_data_get(omci_RTPPM_history_data_t *pOmci_RTPPM_history_data)
{
    UINT16 ch_id;

    ch_id = pOmci_RTPPM_history_data->ch_id;
    if (gRtk_voice_enable)
    {
        if (!g_pLineStateVoIP)
            return GOS_FAIL;
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  RTPErrors=%lu\n",__FUNCTION__,g_pLineStateVoIP->RTPPMHistoryData[ch_id].RTPErrors);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  packetLoss=%lu\n",__FUNCTION__,g_pLineStateVoIP->RTPPMHistoryData[ch_id].packetLoss);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  maximumJitter=%lu\n",__FUNCTION__,g_pLineStateVoIP->RTPPMHistoryData[ch_id].maximumJitter);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  maximumTimeRTCP=%lu\n",__FUNCTION__,g_pLineStateVoIP->RTPPMHistoryData[ch_id].maximumTimeRTCP);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  bufferUnderflows=%lu\n",__FUNCTION__,g_pLineStateVoIP->RTPPMHistoryData[ch_id].bufferUnderflows);
OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  bufferOverflows=%lu\n",__FUNCTION__,g_pLineStateVoIP->RTPPMHistoryData[ch_id].bufferOverflows);
        pOmci_RTPPM_history_data->RTPErrors = g_pLineStateVoIP->RTPPMHistoryData[ch_id].RTPErrors;
        pOmci_RTPPM_history_data->packetLoss = g_pLineStateVoIP->RTPPMHistoryData[ch_id].packetLoss;
        pOmci_RTPPM_history_data->maximumJitter = g_pLineStateVoIP->RTPPMHistoryData[ch_id].maximumJitter;
        pOmci_RTPPM_history_data->maximumTimeRTCP =g_pLineStateVoIP->RTPPMHistoryData[ch_id].maximumTimeRTCP;
        pOmci_RTPPM_history_data->bufferUnderflows = g_pLineStateVoIP->RTPPMHistoryData[ch_id].bufferUnderflows;
        pOmci_RTPPM_history_data->bufferOverflows = g_pLineStateVoIP->RTPPMHistoryData[ch_id].bufferOverflows;
    }
    return GOS_OK;
}
GOS_ERROR_CODE voice_rtk_SIPAGPM_history_data_get(omci_SIPAGPM_history_data_t *pOmci_SIPAGPM_history_data)
{
    UINT16 ch_id;

    ch_id = pOmci_SIPAGPM_history_data->ch_id;

    if (gRtk_voice_enable)
    {
        if (!g_pLineStateVoIP)
            return GOS_FAIL;
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  \n",__FUNCTION__);
        pOmci_SIPAGPM_history_data->rxInviteReqs = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxInviteReqs;
        pOmci_SIPAGPM_history_data->rxInviteRetrans = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxInviteRetrans;
        pOmci_SIPAGPM_history_data->rxNoninviteReqs = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxNoninviteReqs;
        pOmci_SIPAGPM_history_data->rxNoninviteRetrans =g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxNoninviteRetrans;
        pOmci_SIPAGPM_history_data->rxResponse = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxResponse;
        pOmci_SIPAGPM_history_data->rxResponseRetransmissions = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].rxResponseRetransmissions;
        pOmci_SIPAGPM_history_data->txInviteReqs = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txInviteReqs;
	    pOmci_SIPAGPM_history_data->txInviteRetrans = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txInviteRetrans;
        pOmci_SIPAGPM_history_data->txNoninviteReqs = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txNoninviteReqs;
        pOmci_SIPAGPM_history_data->txNoninviteRetrans =g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txNoninviteRetrans;
        pOmci_SIPAGPM_history_data->txResponse = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txResponse;
        pOmci_SIPAGPM_history_data->txResponseRetransmissions = g_pLineStateVoIP->SIPAGPMHistoryData[ch_id].txResponseRetransmissions;
    }
    return GOS_OK;
}
GOS_ERROR_CODE voice_rtk_SIPCIPM_history_data_get(omci_SIPCIPM_history_data_t *pOmci_SIPCI_PM_history_data)
{
    UINT16 ch_id;

    ch_id = pOmci_SIPCI_PM_history_data->ch_id;

    if (gRtk_voice_enable)
    {
        if (!g_pLineStateVoIP)
            return GOS_FAIL;
          OMCI_LOG(OMCI_LOG_LEVEL_DBG,"####%s#####  \n",__FUNCTION__);
        pOmci_SIPCI_PM_history_data->failedConnectCounter = g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].failedConnectCounter;
        pOmci_SIPCI_PM_history_data->failedValidateCounter = g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].failedValidateCounter;
        pOmci_SIPCI_PM_history_data->timeoutCounter = g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].timeoutCounter;
        pOmci_SIPCI_PM_history_data->failureReceivedCounter =g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].failureReceivedCounter;
        pOmci_SIPCI_PM_history_data->failedAuthenticateCounter = g_pLineStateVoIP->SIPCIPMHistoryData[ch_id].failedAuthenticateCounter;
    }

    return GOS_OK;
}
omci_voice_wrapper_t rtk_voice_wrapper =
{
    /* rtk proprietary action */
    .omci_voice_config_init                     = voice_rtk_init_voip_cfg,
    .omci_voice_config_save                     = voice_rtk_save_voip_cfg,
    .omci_voice_service_restart                 = voice_rtk_restart_call_mgr,
    .omci_voice_config_reset                =voice_rtk_config_reset,
    /* set */
    .omci_voice_fax_mode_set                    = voice_rtk_fax_mode_set,
    .omci_voice_oob_dtmf_set                    = voice_rtk_oob_dtmf_set,
    .omci_voice_codec_sel_order_set             = voice_rtk_codec_sel_order_set,
    .omci_voice_codec_sel_order_set_all         = voice_rtk_codec_sel_order_set_all,
    .omci_voice_min_local_port_set              = voice_rtk_min_local_port_set,
    .omci_voice_dscp_mark_set                   = voice_rtk_dscp_mark_set,
    .omci_voice_echo_cancel_ind_set             = voice_rtk_echo_cancel_ind_set,
    .omci_voice_user_part_aor_set               = voice_rtk_user_part_aor_set,
    .omci_voice_sip_display_name_set            = voice_rtk_sip_display_name_set,
    .omci_voice_user_name_set                   = voice_rtk_user_name_set,
    .omci_voice_password_set                    = voice_rtk_password_set,
    .omci_voice_call_wait_feature_set           = voice_rtk_call_wait_feature_set,
    .omci_voice_direct_connect_feature_set      = voice_rtk_direct_connect_feature_set,
    .omci_voice_proxy_server_ip_set             = voice_rtk_proxy_server_ip_set,
    .omci_voice_outbound_proxy_ip_set           = voice_rtk_outbound_proxy_ip_set,
    .omci_voice_sip_reg_exp_time_set            = voice_rtk_sip_reg_exp_time_set,
    .omci_voice_reg_head_start_time_set         = voice_rtk_reg_head_start_time_set,
    .omci_voice_proxy_tcp_udp_port_set          = voice_rtk_proxy_tcp_udp_port_set,
    .omci_voice_proxy_domain_name_set           = voice_rtk_proxy_domain_name_set,
    .omci_voice_rtp_stat_clear                  = voice_rtk_rtp_stat_clear,
	.omci_voice_CCPM_stat_clear                  = voice_rtk_CCPM_stat_clear,
	.omci_voice_SIPAGPM_stat_clear                  = voice_rtk_SIPAGPM_stat_clear,
	.omci_voice_SIPCIPM_stat_clear                  = voice_rtk_SIPCIPM_stat_clear,
	
    .omci_voice_pots_state_set                  = voice_rtk_pots_state_set,
    .omci_voice_jitter_set                      = voice_rtk_jitter_set,
    .omci_voice_hook_flash_time_set             = voice_rtk_hook_flash_time_set,
    .omci_voice_progress_or_transfer_feature_set= voice_rtk_progress_or_transfer_feature_set,
    .omci_voice_dial_plan_set                   = voice_rtk_dial_plan_set,
	.omci_voice_config_method_set                   = voice_rtk_config_method_set,
    /* get */
    .omci_voice_hook_state_get          =voice_rtk_hook_state_get,
    .omci_voice_VOIP_line_status_get               = voice_rtk_VOIP_line_status_get,
    .omci_voice_CCPM_history_data_get       = voice_rtk_CCPM_history_data_get,
    .omci_voice_RTPPM_history_data_get      = voice_rtk_RTPPM_history_data_get,
    .omci_voice_SIPAGPM_history_data_get        = voice_rtk_SIPAGPM_history_data_get,
    .omci_voice_SIPCIPM_history_data_get        = voice_rtk_SIPCIPM_history_data_get,
};

int omci_voice_init(omci_voice_vendor_t voice_vendor)
{
    if (VOICE_VENDOR_END <= voice_vendor)
        return -1;

    omci_voice_wrapper_register(voice_vendor, &rtk_voice_wrapper);

    return 0;
}

int omci_voice_deinit(omci_voice_vendor_t voice_vendor)
{
    if (VOICE_VENDOR_END <= voice_vendor)
        return -1;

    omci_voice_wrapper_unregister(voice_vendor);

    return 0;
}
