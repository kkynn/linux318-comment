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
 * $Date: 2016-01-19 16:20:12 +0800 $
 *
 * Purpose : Define the Cortina related extended OAM
 *
 * Feature : Provide Cortina related extended OAM parsing and handling
 *
 */

#ifndef __CORTINA_OAM_H__
#define __CORTINA_OAM_H__

/* 
 * Symbol Definition 
 */
#define CORTINA_OAM_OUI                     { 0x00, 0xa1, 0x02 }

#define CORTINA_OAM_VAR_RESP_SETOK                      0x80

#define CORTINA_VAR_LEAF_MACTABLE			0x103
#define CORTINA_VAR_LEAF_STORMCONTROL		0x013

/* 0xC7/0x0013 - stromControl */
#define CORTINA_OAM_STORM_CONTROL_LEN                   5
#define CORTINA_OAM_STORM_CONTROL_BROADCAST             1
#define CORTINA_OAM_STORM_CONTROL_MULTICAST             2
#define CORTINA_OAM_STORM_CONTROL_UNICAST               4

typedef struct cortina_wrapper_storm_control_s {
	unsigned char type; /*bit 0: boradcast, bit 1: multicast, bit 2: unknown unicast */
	unsigned int speed;
} cortina_wrapper_storm_control_t;

typedef struct cortina_wrapper_mac_s {
	unsigned int port;
	unsigned char mac[ETHER_ADDR_LEN];
	unsigned short vid;
	unsigned int state;  /* mac state 0: dynamic  1: static */
} cortina_wrapper_mac_t;

#define MAX_MAC_ENTRY_SIZE 85

#define LITTLE_ENDIAN_ENCODE16(pBufPtr, pVar)             \
do {                                                \
    (pBufPtr)[0] = ((unsigned char *)pVar)[1];      \
    (pBufPtr)[1] = ((unsigned char *)pVar)[0];      \
} while(0);
#define LITTLE_ENDIAN_ENCODE32(pBufPtr, pVar)             \
do {                                                \
    (pBufPtr)[0] = ((unsigned char *)pVar)[3];      \
    (pBufPtr)[1] = ((unsigned char *)pVar)[2];      \
    (pBufPtr)[2] = ((unsigned char *)pVar)[1];      \
    (pBufPtr)[3] = ((unsigned char *)pVar)[0];      \
} while(0);

/*  
 * Function Declaration  
 */
extern int cortina_oam_init(void);
extern int cortina_oam_db_init(unsigned char llidIdx);

extern int getMacEntryByPort(int port, unsigned char *num, unsigned char *hasMore);

#endif
