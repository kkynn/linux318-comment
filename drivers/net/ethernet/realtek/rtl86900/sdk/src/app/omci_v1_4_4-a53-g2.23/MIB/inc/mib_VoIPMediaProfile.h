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
 */


#ifndef __MIB_VOIPMEDIAPROFILE_TABLE_H__
#define __MIB_VOIPMEDIAPROFILE_TABLE_H__

/* Table VoIPMediaProfile attribute for STRING type define each entry length */

/* Table VoIPMediaProfile attribute index */
#define MIB_TABLE_VOIPMEDIAPROFILE_ATTR_NUM (17)
#define MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX ((MIB_ATTR_INDEX)1)
#define MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX ((MIB_ATTR_INDEX)2)
#define MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX ((MIB_ATTR_INDEX)3)
#define MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX ((MIB_ATTR_INDEX)4)
#define MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX ((MIB_ATTR_INDEX)5)
#define MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX ((MIB_ATTR_INDEX)6)
#define MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX ((MIB_ATTR_INDEX)7)
#define MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX ((MIB_ATTR_INDEX)8)
#define MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX ((MIB_ATTR_INDEX)9)
#define MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX ((MIB_ATTR_INDEX)10)
#define MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX ((MIB_ATTR_INDEX)11)
#define MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX ((MIB_ATTR_INDEX)12)
#define MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX ((MIB_ATTR_INDEX)13)
#define MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX ((MIB_ATTR_INDEX)14)
#define MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX ((MIB_ATTR_INDEX)15)
#define MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX ((MIB_ATTR_INDEX)16)
#define MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX ((MIB_ATTR_INDEX)17)

/* Table VoIPMediaProfile attribute len, only string attrubutes have length definition */
typedef struct {
	UINT16   EntityId;
	UINT8    FaxMode;
	UINT16   VoiceServiceProfilePointer;
	UINT8    CodecSelection1stOrder;
	UINT8    PacketPeriodSelection1stOrder;
	UINT8    SilenceSuppression1stOrder;
	UINT8    CodecSelection2ndOrder;
	UINT8    PacketPeriodSelection2ndOrder;
	UINT8    SilenceSuppression2ndOrder;
	UINT8    CodecSelection3rdOrder;
	UINT8    PacketPeriodSelection3rdOrder;
	UINT8    SilenceSuppression3rdOrder;
	UINT8    CodecSelection4thOrder;
	UINT8    PacketPeriodSelection4thOrder;
	UINT8    SilenceSuppression4thOrder;
	UINT8    OOBDTMF;
	UINT16   RTPProfilePointer;
} __attribute__((packed)) MIB_TABLE_VOIPMEDIAPROFILE_T;



enum
{
	OMCI_CODEC_PCMU = 0,
	OMCI_CODEC_RESERVED_1,
	OMCI_CODEC_RESERVED_2,
	OMCI_CODEC_GSM,
	OMCI_CODEC_G723,
	OMCI_CODEC_DIV4_8K,
	OMCI_CODEC_DIV4_16K,
	OMCI_CODEC_LPC,
	OMCI_CODEC_PCMA,
	OMCI_CODEC_G722,
	OMCI_CODEC_L16_2CH,
	OMCI_CODEC_L16_1CH,
	OMCI_CODEC_QCELP,
	OMCI_CODEC_CN,
	OMCI_CODEC_MPA,
	OMCI_CODEC_G728,
	OMCI_CODEC_DIV4_11K,
	OMCI_CODEC_DIV4_22K,
	OMCI_CODEC_G729, //18
	OMCI_CODEC_MAX
};

#endif /* __MIB_VOIPMEDIAPROFILE_TABLE_H__ */
