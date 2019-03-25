#ifndef _XT_MAC_H
#define _XT_MAC_H

#if 1
#define MAC_SRC			0x01	/* Match source MAC address */
#define SRC_MASK		0x02	/* Source MAC mask */
#define MAC_DST			0x04	/* Match destination MAC address */
#define DST_MASK		0x08	/* Destination MAC mask */
#define MAC_SRC_INV		0x10	/* Negate the condition */
#define SRC_MASK_INV		0x20	/* Negate the condition */
#define MAC_DST_INV		0x40	/* Negate the condition */
#define DST_MASK_INV		0x80	/* Negate the condition */

struct xt_mac_info {
    unsigned char srcaddr[ETH_ALEN];
    unsigned char srcmask[ETH_ALEN];
    unsigned char dstaddr[ETH_ALEN];
    unsigned char dstmask[ETH_ALEN];
    u_int8_t flags;
};

#else
struct xt_mac_info {
    unsigned char srcaddr[ETH_ALEN];
    int invert;
};
#endif
#endif /*_XT_MAC_H*/
