#ifndef _HALMAC_2_PLATFORM_H_
#define _HALMAC_2_PLATFORM_H_

/*
* Search The Keyword [Driver]
* Search The Keyword [Driver]
* Search The Keyword [Driver]
*/

/*[Driver] always set BUILD_TEST =0*/
#define BUILD_TEST	0

#if BUILD_TEST
#include "../Platform/App/Test/halmac_2_platformapi.h"
#else
/*[Driver] use their own header files*/
//#include "../Platform/App/VS2010MFC/AutoTest/stdafx.h"
//#include "../Header/GeneralDef.h"
#include "halmac_hw_cfg.h"

#include "../../8192cd_cfg.h"
#if !defined(__ECOS) && !defined(__OSK__)
#include <asm/io.h>
#include <linux/spinlock.h>
#elif !defined(__OSK__)
#include <cyg/io/eth/rltk/819x/wrapper/sys_support.h>
#endif
#include "Hal88XXDesc.h"

#endif

/*[Driver] provide the define of _TRUE, _FALSE, NULL, u8, u16, u32*/

#ifndef _TRUE
	#define _TRUE		1
#endif

#ifndef _FALSE
	#define _FALSE		(!_TRUE)
#endif

#ifndef NULL
	#define NULL		((void *)0)
#endif

#define HALMAC_INLINE __inline

#if !defined(__OSK__)
typedef u8 *pu8;
typedef u16 *pu16;
typedef u32 *pu32;
typedef s8 *ps8;
typedef s16 *ps16;
typedef s32 *ps32;
#else
#include "sys-support.h"
#endif


#define HALMAC_PLATFORM_LITTLE_ENDIAN                  1
#define HALMAC_PLATFORM_BIG_ENDIAN                     0

/* Note : Named HALMAC_PLATFORM_LITTLE_ENDIAN / HALMAC_PLATFORM_BIG_ENDIAN
 * is not mandatory. But Little endian must be '1'. Big endian must be '0'
 */
/*[Driver] config the system endian*/

#ifdef _LITTLE_ENDIAN_
#undef HALMAC_SYSTEM_ENDIAN
#define HALMAC_SYSTEM_ENDIAN                           HALMAC_PLATFORM_LITTLE_ENDIAN
#endif

#ifdef _BIG_ENDIAN_
#undef HALMAC_SYSTEM_ENDIAN
#define HALMAC_SYSTEM_ENDIAN                           HALMAC_PLATFORM_BIG_ENDIAN
#endif


/*[Driver] config if the operating platform*/
#define HALMAC_PLATFORM_WINDOWS   0
#define HALMAC_PLATFORM_LINUX		  0
#define HALMAC_PLATFORM_AP		  1
/*[Driver] must set HALMAC_PLATFORM_TESTPROGRAM = 0*/
#define HALMAC_PLATFORM_TESTPROGRAM       0

/*[Driver] config if enable the dbg msg or notl*/
#define HALMAC_DBG_MSG_ENABLE		0

/*[Driver] define the Platform SDIO Bus CLK */
#define PLATFORM_SD_CLK	50000000 /*50MHz*/

/*[Driver] define the Rx FIFO expanding mode packet size unit for 8821C and 8822B */
/*Should be 8 Byte alignment*/
#define HALMAC_RX_FIFO_EXPANDING_MODE_PKT_SIZE	48 /*Bytes*/

/*[Driver] provide the type mutex*/
/* Mutex type */
//typedef	CRITICAL_SECTION	HALMAC_MUTEX;											HALMAC_MUTEX;
#if !defined(__ECOS) && !defined(__OSK__)
typedef	spinlock_t											HALMAC_MUTEX;
#else
typedef	int													HALMAC_MUTEX;
#endif

#endif/* _HALMAC_2_PLATFORM_H_ */



