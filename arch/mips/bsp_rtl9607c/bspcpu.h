/*
 * Realtek Semiconductor Corp.
 *
 * bsp/bspcpu.h
 *     bsp cpu and memory configuration file
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#ifndef _BSPCPU_H_
#define _BSPCPU_H_

/*
 * scache => L2 cache
 * dcache => D cache
 * icache => I cache
 */

#define cpu_tlb_entry              64


/**********************************************
 * GCMP_BASE_ADDR is determined at CPU-built period
 * 
 * GIC_BASE_ADDR : Refer to the address space of Soc Spec
 * CPC_BASE_ADDR : Refer to the address space of Soc Spec
 *********************************************/
/*
 * GCMP Specific definitions
 */
#define GCMP_BASE_ADDR			0x1fbf8000
#define GCMP_ADDRSPACE_SZ		(256 * 1024)
#define GCR_CONFIG_ADDR			0xbfbf8000  // KSEG0 address of the GCR registers
/*
 * GIC Specific definitions
 */
#define GIC_NUM_INTRS			64
#define GIC_ADDRSPACE_SZ		(128 * 1024)

#define GIC_BASE_ADDR			0x1fbc0000

/*
 * CPC Specific definitions
 */
#define CPC_BASE_ADDR			0x1fbf0000

#endif /* _BSPCPU_H_ */
