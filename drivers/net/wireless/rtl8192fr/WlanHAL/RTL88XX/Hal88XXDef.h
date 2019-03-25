#ifndef __HAL88XX_DEF_H__
#define __HAL88XX_DEF_H__

/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	Hal88XXDef.h
	
Abstract:
	Defined HAL 88XX common data structure & Define
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2012-03-23 Filen            Create.	
--*/

#ifdef  WLAN_HAL_INTERNAL_USED

enum rf_type
GetChipIDMIMO88XX(
    IN  HAL_PADAPTER        Adapter
);


VOID
CAMEmptyEntry88XX(
    IN  HAL_PADAPTER    Adapter,
    IN  u1Byte          index
);


u4Byte
CAMFindUsable88XX(
    IN  HAL_PADAPTER    Adapter,
    IN  u4Byte          for_begin
);


VOID
CAMReadMACConfig88XX
(
    IN  HAL_PADAPTER    Adapter,
    IN  u1Byte          index, 
    OUT pu1Byte         pMacad,
    OUT PCAM_ENTRY_CFG  pCfg
);


VOID
CAMProgramEntry88XX(
    IN	HAL_PADAPTER		Adapter,
    IN  u1Byte              index,
    IN  pu1Byte             macad,
    IN  pu1Byte             key128,
    IN  u2Byte              config
);


VOID
SetHwReg88XX(
    IN	HAL_PADAPTER		Adapter,
    IN	u1Byte				variable,
    IN	pu1Byte				val
);


VOID
GetHwReg88XX(
    IN      HAL_PADAPTER    Adapter,
    IN      u1Byte          variable,
    OUT     pu1Byte         val
);

enum rt_status
GetMACIDQueueInTXPKTBUF88XX(
    IN      HAL_PADAPTER          Adapter,
    OUT     pu1Byte               MACIDList
);


enum rt_status
SetMACIDSleep88XX(
    IN  HAL_PADAPTER Adapter,
    IN  BOOLEAN      bSleep,   
    IN  u4Byte       aid
);

#if (IS_RTL8881A_SERIES || IS_RTL8192E_SERIES || IS_RTL8192F_SERIES)
enum rt_status
InitLLT_Table88XX(
    IN  HAL_PADAPTER    Adapter
);
#endif //#if (IS_RTL8881A_SERIES || IS_RTL8192E_SERIES)

#if (IS_RTL8814A_SERIES || IS_RTL8197F_SERIES || IS_RTL8822B_SERIES || IS_RTL8821C_SERIES)
enum rt_status
InitLLT_Table88XX_V1(
    IN  HAL_PADAPTER    Adapter
);
#endif //#if (IS_RTL8814A_SERIES || IS_RTL8197F_SERIES || IS_RTL8822B_SERIES || IS_RTL8821C_SERIES)

enum rt_status
InitPON88XX(
    IN  HAL_PADAPTER Adapter
);

enum rt_status
InitMAC88XX(
    IN  HAL_PADAPTER Adapter
);

VOID
InitIMR88XX(
    IN  HAL_PADAPTER    Adapter,
    IN  RT_OP_MODE      op_mode
);

VOID
InitVAPIMR88XX(
    IN  HAL_PADAPTER    Adapter,
    IN  u4Byte          VapSeq
);


enum rt_status      
InitHCIDMAMem88XX(
    IN      HAL_PADAPTER    Adapter
);  

enum rt_status
InitHCIDMAReg88XX(
    IN      HAL_PADAPTER    Adapter
);  

VOID
StopHCIDMASW88XX(
    IN  HAL_PADAPTER Adapter
);

VOID
StopHCIDMAHW88XX(
    IN  HAL_PADAPTER Adapter
);

#if CFG_HAL_SUPPORT_MBSSID
VOID
InitMBSSID88XX(
    IN  HAL_PADAPTER Adapter
);

VOID
InitMBIDCAM88XX(
    IN  HAL_PADAPTER Adapter
);

VOID
StopMBSSID88XX(
    IN  HAL_PADAPTER Adapter
);
#endif  //CFG_HAL_SUPPORT_MBSSID

#ifdef AP_SWPS_OFFLOAD
VOID
InitAPSWPS88XX(
    IN  HAL_PADAPTER Adapter
);
#endif

enum rt_status
SetMBIDCAM88XX(
    IN  HAL_PADAPTER Adapter,
    IN  u1Byte       MBID_Addr,    
    IN  u1Byte       IsRoot
);

enum rt_status
StopMBIDCAM88XX(
    IN  HAL_PADAPTER Adapter,
    IN  u1Byte       MBID_Addr
);

enum rt_status
ResetHWForSurprise88XX(
    IN  HAL_PADAPTER Adapter
);

#if CFG_HAL_MULTI_MAC_CLONE
VOID
McloneSetMBSSID88XX(
    IN  HAL_PADAPTER Adapter,
    IN	pu1Byte 	 macAddr,
    IN	int          entIdx
);

VOID
McloneStopMBSSID88XX(
    IN  HAL_PADAPTER Adapter,
    IN	int          entIdx
);
#endif // #if CFG_HAL_MULTI_MAC_CLONE

enum rt_status
StopHW88XX(
    IN  HAL_PADAPTER Adapter
);

enum rt_status
StopSW88XX(
    IN  HAL_PADAPTER Adapter
);

VOID
DisableVXDAP88XX(
    IN  HAL_PADAPTER Adapter
);

VOID
Timer1Sec88XX(
    IN  HAL_PADAPTER Adapter
);


enum rt_status
PktBufAccessCtrl(
    IN	HAL_PADAPTER        Adapter,
    IN u1Byte mode,
    IN u1Byte rw,
    IN u2Byte offset, //Addr >> 3
    IN u1Byte wbit
);


enum rt_status 
GetTxRPTBuf88XX(
    IN	HAL_PADAPTER        Adapter,
    IN	u4Byte              macID,
    IN  u1Byte              variable,   
    IN 	u1Byte				byteoffset,
    OUT pu1Byte             val
);

enum rt_status 
SetTxRPTBuf88XX(
    IN	HAL_PADAPTER        Adapter,
    IN	u4Byte              macID,
    IN  u1Byte              variable,
    IN  pu1Byte             val    
);

u4Byte
CheckHang88XX(
    IN	HAL_PADAPTER        Adapter
);

VOID
SetCRC5ToRPTBuffer88XX(
    IN	HAL_PADAPTER        Adapter,
    IN	u1Byte              val,
    IN	u4Byte              macID,
    IN  u1Byte              bValid
);

VOID
ClearHWTXShortcutBufHandler88XX(
    IN  HAL_PADAPTER        Adapter,
    IN  u4Byte              macID
);

VOID
GetHwSequenceHandler88XX(
    IN	HAL_PADAPTER        Adapter,
    IN	u4Byte              macID,
    IN  u1Byte              tid, 
    OUT pu4Byte             val  
);
         
VOID
SetCRC5ValidBit88XX(
    IN	HAL_PADAPTER        Adapter,
    IN	u1Byte              group,
    IN  u1Byte              bValid
    
);

VOID
SetCRC5EndBit88XX(
    IN	HAL_PADAPTER        Adapter,
    IN	u1Byte              group,
    IN  u1Byte              bEnd    
);

VOID
InitMACIDSearch88XX(
    IN	HAL_PADAPTER        Adapter    
);


enum rt_status
CheckHWMACIDResult88XX(
    IN	HAL_PADAPTER        Adapter,    
    IN  u4Byte              MacID,
    OUT pu1Byte             result
);


enum rt_status 
InitDDMA88XX(
    IN  HAL_PADAPTER    Adapter,
    IN  u4Byte	source,
    IN  u4Byte	dest,
    IN  u4Byte 	length
);

#endif  //WLAN_HAL_INTERNAL_USED

#endif  //__HAL88XX_DEF_H__
