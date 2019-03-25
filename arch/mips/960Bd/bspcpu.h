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

#ifdef CONFIG_SOC_CPU_MIPSIAUP
#	define cpu_icache_line (32)
#	define cpu_dcache_line (32)
#	define cpu_scache_line (32)
#	define cpu_icache_size (64*1024)
#	define cpu_dcache_size (32*1024)
#	define cpu_scache_size (128*1024)
#else
#	define cpu_icache_line (32)
#	define cpu_dcache_line (32)
#	define cpu_scache_line (0)
#	define cpu_icache_size (64*1024)
#	define cpu_dcache_size (32*1024)
#	define cpu_scache_size (0)
#endif

#define cpu_tlb_entry 64

#endif /* _BSPCPU_H_ */
