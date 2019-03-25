/*
 * ppp-comp.h - Definitions for doing PPP packet compression.
 *
 * Copyright 1994-1998 Paul Mackerras.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */
#ifndef _NET_PPP_COMP_H
#define _NET_PPP_COMP_H

#include <uapi/linux/ppp-comp.h>


struct module;

/*
 * The following symbols control whether we include code for
 * various compression methods.
 */

#ifndef DO_BSD_COMPRESS
#define DO_BSD_COMPRESS	1	/* by default, include BSD-Compress */
#endif
#ifndef DO_DEFLATE
#define DO_DEFLATE	1	/* by default, include Deflate */
#endif
#define DO_PREDICTOR_1	0
#define DO_PREDICTOR_2	0

/*
 * Structure giving methods for compression/decompression.
 */

struct compressor {
	int	compress_proto;	/* CCP compression protocol number */

	/* Allocate space for a compressor (transmit side) */
	void	*(*comp_alloc) (unsigned char *options, int opt_len);

	/* Free space used by a compressor */
	void	(*comp_free) (void *state);

	/* Initialize a compressor */
	int	(*comp_init) (void *state, unsigned char *options,
			      int opt_len, int unit, int opthdr, int debug);

	/* Reset a compressor */
	void	(*comp_reset) (void *state);

	/* Compress a packet */
	int     (*compress) (void *state, unsigned char *rptr,
			      unsigned char *obuf, int isize, int osize);

	/* Return compression statistics */
	void	(*comp_stat) (void *state, struct compstat *stats);

	/* Allocate space for a decompressor (receive side) */
	void	*(*decomp_alloc) (unsigned char *options, int opt_len);

	/* Free space used by a decompressor */
	void	(*decomp_free) (void *state);

	/* Initialize a decompressor */
	int	(*decomp_init) (void *state, unsigned char *options,
				int opt_len, int unit, int opthdr, int mru,
				int debug);

	/* Reset a decompressor */
	void	(*decomp_reset) (void *state);

	/* Decompress a packet. */
	int	(*decompress) (void *state, unsigned char *ibuf, int isize,
				unsigned char *obuf, int osize);

	/* Update state for an incompressible packet received */
	void	(*incomp) (void *state, unsigned char *ibuf, int icnt);

	/* Return decompression statistics */
	void	(*decomp_stat) (void *state, struct compstat *stats);

	/* Used in locking compressor modules */
	struct module *owner;
};

/*
 * The return value from decompress routine is the length of the
 * decompressed packet if successful, otherwise DECOMP_ERROR
 * or DECOMP_FATALERROR if an error occurred.
 * 
 * We need to make this distinction so that we can disable certain
 * useful functionality, namely sending a CCP reset-request as a result
 * of an error detected after decompression.  This is to avoid infringing
 * a patent held by Motorola.
 * Don't you just lurve software patents.
 */

#define DECOMP_ERROR		-1	/* error detected before decomp. */
#define DECOMP_FATALERROR	-2	/* error detected after decomp. */

/*
* Definitions for MPPE/MPPC.  
*/

#define CI_MPPE                18      /* config option for MPPE */ 
#define CILEN_MPPE              6      /* length of config option */ 
#define MPPE_OVHD		4	/* MPPE overhead */
#define MPPE_MAX_KEY_LEN	16	/* largest key length (128-bit) */

#define MPPE_STATELESS          0x01	/* configuration bit H */
#define MPPE_40BIT              0x20	/* configuration bit L */
#define MPPE_56BIT              0x80	/* configuration bit M */
#define MPPE_128BIT             0x40	/* configuration bit S */
#define MPPE_MPPC               0x01	/* configuration bit C */

/*
* Definitions for Stac LZS.
*/

#define CI_LZS			17	/* config option for Stac LZS */
#define CILEN_LZS		5	/* length of config option */

#define LZS_OVHD		4	/* max. LZS overhead */
#define LZS_HIST_LEN		2048	/* LZS history size */
#define LZS_MAX_CCOUNT		0x0FFF	/* max. coherency counter value */

#define LZS_MODE_NONE		0
#define LZS_MODE_LCB		1
#define LZS_MODE_CRC		2
#define LZS_MODE_SEQ		3
#define LZS_MODE_EXT		4

#define LZS_EXT_BIT_FLUSHED	0x80	/* bit A */
#define LZS_EXT_BIT_COMP	0x20	/* bit C */


extern int ppp_register_compressor(struct compressor *);
extern void ppp_unregister_compressor(struct compressor *);
#endif /* _NET_PPP_COMP_H */
