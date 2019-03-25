/*
 * Realtek Semiconductor Corp.
 *
 * bsp/prom.c
 *     bsp early initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <asm/bootinfo.h>
#include <asm/page.h>
#include "bspchip.h"
#include "bsp_automem.h"

extern void mips_set_machine_name(const char *name);
extern void __init plat_smp_init(void);
extern void __init bsp_setup_scache(void);

int watchdog_enable = 0;
uint32_t BSP_MHZ;

void kick_watchdog(void) {
	return;
}

void prom_printf(char *fmt, ...) {
	va_list args;
	char ppbuf[1016];
	char *bptr;

	va_start(args, fmt);
	vsprintf(ppbuf, fmt, args);

	bptr = ppbuf;

	while (*bptr != 0) {
		if (*bptr == '\n')
			prom_putchar('\r');

		prom_putchar(*bptr++);
	}
	va_end(args);
}

/*
 * Clear the setting for
 * CPU0 Unmapped Memory Segment Address Register 0 ~ 3
 *
 */
static void inline prom_dram_unmap_clear(void){
  int i;
  for(i=0; i<4; i++){
		//prom_printf( "R_C0UMSAR(0x%08x)= 0x%08x\n", (R_C0UMSAR0_BASE + i*0x10), REG32(R_C0UMSAR0_BASE + i*0x10));
		REG32(R_C0UMSAR0_BASE + i*0x10) = 0x0;
		//prom_printf( "R_C0UMSAR(0x%08x)= 0x%08x\n", (R_C0UMSAR0_BASE + i*0x10), REG32(R_C0UMSAR0_BASE + i*0x10));
  }
}

void reset_sram(void){
	REG32(R_C0UMSAR0) = 0x0;
	REG32(R_C0UMSAR1) = 0x0;
	REG32(R_C0UMSAR2) = 0x0;
	REG32(R_C0UMSAR3) = 0x0;
	REG32(R_C0SRAMSAR0) = 0x0;
	REG32(R_C0SRAMSAR1) = 0x0;
	REG32(R_C0SRAMSAR2) = 0x0;
	REG32(R_C0SRAMSAR3) = 0x0;
	return;
}

#if defined(CONFIG_LUNA_MEMORY_AUTO_DETECTION)
#define PBO_BARRIER   PAGE_SIZE
#ifdef CONFIG_LUNA_PBO_DL_DRAM_SIZE
#define   LUNA_PBO_DL_DRAM_SIZE_BARRIER     (CONFIG_LUNA_PBO_DL_DRAM_SIZE + PBO_BARRIER)
#endif
#ifdef CONFIG_LUNA_PBO_UL_DRAM_SIZE
#define   LUNA_PBO_UL_DRAM_SIZE_BARRIER     (CONFIG_LUNA_PBO_UL_DRAM_SIZE + PBO_BARRIER)
#endif
#define DEBUG
#ifdef DEBUG
#define DEBUG_MEM_AUTO(fmt, args...) prom_printf( "DEBUG_MEM_AUTO: " fmt, ## args)
#else
#define DEBUG_MEM_AUTO(fmt, args...)
#endif

//#if defined(CONFIG_RTL8686_IPC_DEV)
struct bsp_mem_map_s bsp_mem_map;
//#endif

static int __initdata zone1_size = 0;
static int __initdata zone2_size = 0;
static unsigned int __initdata mem_size = 0;
static unsigned int __initdata reserved_slave_mem = 0;



static void inline remove_mem_para(void)
{
	char *ptr;
	char *ptr_mem;
	char *endptr;	/* local pointer to end of parsed string */
	unsigned long long ret = 0;

	ptr = strstr(arcs_cmdline, "mem=");
	ptr_mem= ptr+4;
	DEBUG_MEM_AUTO("arcs_cmdline=%s, %p, %p\n", arcs_cmdline, arcs_cmdline, ptr);
	if(ptr){
		ret = simple_strtoull(ptr_mem, &endptr, 0);
		memmove(ptr, (endptr+1), strlen((endptr+1)) + 1);
	}
	DEBUG_MEM_AUTO("arcs_cmdline=%s\n", arcs_cmdline);
}


static unsigned int __init memctlc_dram_size(void)
{
	unsigned int dcr;
	int total_bit = 0;

	dcr = *((unsigned int *)(C0DCR));
	DEBUG_MEM_AUTO("DCR=0x%08x\n", dcr );
	total_bit = 0;
	total_bit += ((dcr>>24)&0x3); //bus width
	total_bit += ((dcr>>20)&0x7)+11; //row count
	total_bit += ((dcr>>16)&0x7)+8 ; //col count
	total_bit += ((dcr>>28)&0x3)+1;  //bank count
	total_bit += (dcr>>15)&1;        //Dram Chip Select

	return ((1<<total_bit));
}


#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
unsigned int LUNA_PBO_DL_DRAM_OFFSET = 0;
unsigned int LUNA_PBO_UL_DRAM_OFFSET = 0;


void static inline bsp_pbo_mem_auto(void){
	unsigned int bankcnt;
	unsigned int banksize;

	/* BANKCNT: bit 29~28 */
	bankcnt = (*((unsigned int *)(C0DCR)) >> 28) & 0x3;
	/*
	 * 00: 2 banks (used for SDR)
	 * 01: 4 banks (used for SDR, DDR, DDR2)
	 * 10: 8 banks (used for DDR2, DDR3)
	 */
	bankcnt = 2 << bankcnt;
	banksize = (mem_size/bankcnt);
	DEBUG_MEM_AUTO("dcr=0x%08x, bankcnt=0x%08x, mem_size=0x%08x, banksize=0x%08x\n",*((unsigned int *)(C0DCR)), bankcnt ,mem_size, banksize);
	DEBUG_MEM_AUTO("LUNA_PBO_DL_DRAM_SIZE_BARRIER=0x%08x, LUNA_PBO_UL_DRAM_SIZE_BARRIER=0x%08x\n", LUNA_PBO_DL_DRAM_SIZE_BARRIER, LUNA_PBO_UL_DRAM_SIZE_BARRIER);
	if(mem_size > 0x10000000){ /* (Total mem) > 256MB  */
    //MEM > 256MB
    /* ---512MB MEM -------
		 * - Bank 0
		 *
		 *
		 * - Bank x (If bank 8, x=2)
		 * LUNA_PBO_DL_DRAM_SIZE_BARRIER
		 *
		 * ---Bank x+1
		 * LUNA_PBO_UL_DRAM_SIZE_BARRIER
		 *---- Zone1 end: 0x10000000, 256MB
		 * ...................
		 *---- MEM End: 0x20000000, 512MB
		 */

		LUNA_PBO_UL_DRAM_OFFSET = 0x10000000 - LUNA_PBO_UL_DRAM_SIZE_BARRIER;
		LUNA_PBO_DL_DRAM_OFFSET = 0x10000000 - banksize - LUNA_PBO_DL_DRAM_SIZE_BARRIER;
	}else{
    /* MEM= 256MB, 128MB, 64MB */
    /* ---256MB MEM: 0x0 Start-------
		 *
		 *
		 * LUNA_PBO_DL_DRAM_SIZE_BARRIER
		 * ------ the last Bank
		 * ....
		 * .....
		 * LUNA_PBO_UL_DRAM_SIZE_BARRIER
		 * CONFIG_RTL8686_IPC_MEM_SIZE
		 * CONFIG_RTL8686_DSP_MEM_SIZE
		 * ---MEM END: 256MB -------
		 */

		LUNA_PBO_UL_DRAM_OFFSET = mem_size - LUNA_PBO_UL_DRAM_SIZE_BARRIER;

		LUNA_PBO_DL_DRAM_OFFSET = mem_size - banksize - LUNA_PBO_DL_DRAM_SIZE_BARRIER;
#if defined(CONFIG_RTL8686_IPC_MEM_SIZE) && defined(CONFIG_RTL8686_DSP_MEM_SIZE)
		LUNA_PBO_UL_DRAM_OFFSET = LUNA_PBO_UL_DRAM_OFFSET - (CONFIG_RTL8686_IPC_MEM_SIZE + CONFIG_RTL8686_DSP_MEM_SIZE);
		/*Overlap "DSP or IPC" */
		while (LUNA_PBO_UL_DRAM_OFFSET <= LUNA_PBO_DL_DRAM_OFFSET ){
			/* LUNA_PBO_DL_DRAM_OFFSET overlap LUNA_PBO_UL_DRAM_OFFSET
			 * LUNA_PBO_DL_DRAM_OFFSET must to move to x-1 bank
			 */
			LUNA_PBO_DL_DRAM_OFFSET = LUNA_PBO_DL_DRAM_OFFSET- banksize ;
			if (LUNA_PBO_DL_DRAM_OFFSET < banksize){
				printk("[bsp]LUNA_PBO_DL_DRAM_OFFSET(0x%08x) is bad!", LUNA_PBO_DL_DRAM_OFFSET);
				break;
			}
		}
#endif

	}
	DEBUG_MEM_AUTO("LUNA_PBO_UL_DRAM_OFFSET=0x%08x,LUNA_PBO_DL_DRAM_OFFSET=0x%08x\n", LUNA_PBO_UL_DRAM_OFFSET, LUNA_PBO_DL_DRAM_OFFSET );
	DEBUG_MEM_AUTO("PAGE_SIZE=0x%08x\n", PAGE_SIZE);
}
#else
#define bsp_pbo_mem_auto()
#endif

static void inline prom_DSP_IPC_mem(void){

	/* ---256MB MEM: 0x0 Start-------
	 *
	 *
	 * LUNA_PBO_DL_DRAM_SIZE_BARRIER
	 *   --- Last Bank
	 *
	 * LUNA_PBO_UL_DRAM_SIZE_BARRIER
	 * CONFIG_RTL8686_IPC_MEM_SIZE
	 * CONFIG_RTL8686_DSP_MEM_SIZE
	 * ---MEM END: 256MB -------
	 */

	/* ---512MB MEM -------
	 *  ---- Start: 0x0
	 *
	 * LUNA_PBO_DL_DRAM_SIZE_BARRIER
	 *
	 * LUNA_PBO_UL_DRAM_SIZE_BARRIER
	 *---- Zone1 end: 0x10000000, 256MB
	 *
	 *
	 *
	 * CONFIG_RTL8686_IPC_MEM_SIZE
	 * CONFIG_RTL8686_DSP_MEM_SIZE
	 * ---MEM END: 512-------
	 */

//#if defined(CONFIG_RTL8686_IPC_DEV) && defined(CONFIG_RTL8686_DSP_MEM_SIZE) && defined(CONFIG_RTL8686_IPC_MEM_SIZE)
#if defined(CONFIG_RTL8686_DSP_MEM_SIZE) && defined(CONFIG_RTL8686_IPC_MEM_SIZE)
	bsp_mem_map.BSP_IPC_MEM_BASE = mem_size - (CONFIG_RTL8686_DSP_MEM_SIZE + CONFIG_RTL8686_IPC_MEM_SIZE);
	bsp_mem_map.BSP_DSP_MEM_BASE =  bsp_mem_map.BSP_IPC_MEM_BASE  + CONFIG_RTL8686_IPC_MEM_SIZE;
	DEBUG_MEM_AUTO("BSP_DSP_MEM_BASE=0x%x, BSP_IPC_MEM_BASE=0x%x \n", bsp_mem_map.BSP_DSP_MEM_BASE, bsp_mem_map.BSP_IPC_MEM_BASE);
#endif
}

static void inline prom_reserved_last(void){
  if( mem_size <= 0x10000000){
#if  defined(CONFIG_RTL8686_DSP_MEM_SIZE) && defined(CONFIG_RTL8686_IPC_MEM_SIZE)
    reserved_slave_mem = CONFIG_RTL8686_DSP_MEM_SIZE + CONFIG_RTL8686_IPC_MEM_SIZE;
#endif
#if defined(LUNA_PBO_UL_DRAM_SIZE_BARRIER)
    reserved_slave_mem = reserved_slave_mem + LUNA_PBO_UL_DRAM_SIZE_BARRIER;
#endif
  }else{
#if  defined(CONFIG_RTL8686_DSP_MEM_SIZE) && defined(CONFIG_RTL8686_IPC_MEM_SIZE)
    reserved_slave_mem = CONFIG_RTL8686_DSP_MEM_SIZE + CONFIG_RTL8686_IPC_MEM_SIZE;
#endif
  }
}

void __init prom_mem_zone_detect(void){
	mem_size = memctlc_dram_size();
	bsp_pbo_mem_auto();

	prom_DSP_IPC_mem();
	prom_reserved_last();
	if((mem_size - reserved_slave_mem) > ZONE1_SIZE){  //>256MB

		zone2_size = (mem_size - reserved_slave_mem) - ZONE1_SIZE;

		REG32(BSP_CDOR2) = ZONE2_OFF;
		REG32(BSP_CDMAR2) = zone2_size - 1;
		DEBUG_MEM_AUTO("BSP_CDOR2=0x%x, BSP_CDMAR2=0x%x\n",  REG32(BSP_CDOR2),REG32(BSP_CDMAR2) );
		zone1_size = ZONE1_SIZE;          //256MB
	}else{
		zone1_size = mem_size - reserved_slave_mem; //<=256MB
  }
}



static void inline __init bsp_prom_memmap(void){
	unsigned int mem_start = 0;
	unsigned int region_size = 0;

	/*
	 * remove_mem_para();
	 * If mem=xxxMB in cmdline, that will blow away the below setting
	 */

	if(mem_size <= 0x10000000) {
#if defined(LUNA_PBO_DL_DRAM_SIZE_BARRIER)
		region_size = LUNA_PBO_DL_DRAM_OFFSET;
		DEBUG_MEM_AUTO("[%s]add_memory_region(%d, %d, BOOT_MEM_RAM)\n", __func__, mem_start, region_size);
		DEBUG_MEM_AUTO("[%s]add_memory_region(0x%08x, 0x%08x, BOOT_MEM_RAM)\n", __func__, mem_start, region_size);
		add_memory_region(mem_start, region_size, BOOT_MEM_RAM);
		mem_start = LUNA_PBO_DL_DRAM_OFFSET + LUNA_PBO_DL_DRAM_SIZE_BARRIER;
#endif
		region_size = (mem_size - reserved_slave_mem)- mem_start;
		DEBUG_MEM_AUTO("[%s]add_memory_region(%d, %d, BOOT_MEM_RAM)\n", __func__, mem_start, region_size);
		DEBUG_MEM_AUTO("[%s]add_memory_region(0x%08x, 0x%08x, BOOT_MEM_RAM)\n", __func__, mem_start, region_size);
		add_memory_region(mem_start, region_size, BOOT_MEM_RAM);
	}else{  /* DRAM > 256MB */

	  /*
	   * CPU0: ZONE_NORMAL
	   *  Only consider PBO
	   */
	  region_size = 0x10000000;
#if defined(LUNA_PBO_DL_DRAM_SIZE_BARRIER)
	  region_size = LUNA_PBO_DL_DRAM_OFFSET;
	  DEBUG_MEM_AUTO("[%s]add_memory_region(%d, %d, BOOT_MEM_RAM)\n", __func__, mem_start, region_size);
	  add_memory_region(mem_start, region_size, BOOT_MEM_RAM);
	  mem_start = LUNA_PBO_DL_DRAM_OFFSET + LUNA_PBO_DL_DRAM_SIZE_BARRIER;
	  region_size = (0x10000000 - LUNA_PBO_UL_DRAM_SIZE_BARRIER)- mem_start;
#endif
	  DEBUG_MEM_AUTO("[%s]add_memory_region(%d, %d, BOOT_MEM_RAM)\n", __func__, mem_start, region_size);
	  add_memory_region(mem_start, region_size, BOOT_MEM_RAM);

		/* CPU0: ZONE HIGHMEM          */
		/*       Handle IPC/DSM memory */
	  DEBUG_MEM_AUTO("[%s]add_memory_region(%d, %d, BOOT_MEM_RAM)\n", __func__, ZONE2_BASE, zone2_size);
	  add_memory_region(ZONE2_BASE, zone2_size, BOOT_MEM_RAM);
	}
}
#endif

/***********************************************************************/

#ifdef CONFIG_LUNA_MEMORY_AUTO_DETECTION
void __init prom_meminit(void)
{
	prom_dram_unmap_clear();
	prom_mem_zone_detect();
	bsp_prom_memmap();
}
#else
/***********************************************
 * Manually set up the mem map.
 * When DRAM >= 512MB
 * 1. With DSP/IPC arch
 *   a) Not add "mem=MEMEORY_SIZE" in the kernel cmdline
 *   b) Select CONFIG_HIGHMEM in kernel option
 *   c) Set up the mem offset for DSP, IPC
 *   d) Set up the mem offset for PBO
 *
 *
 * 2. Single image( Without DSP arch)
 *   a) Not add "mem=MEMEORY_SIZE" in the kernel cmdline
 *   b) Add "highmem=xxxM" in the kernel cmdline
 *      e,q: highmem=256M
 *   c) Select CONFIG_HIGHMEM in kernel option
 *
 * Note
 *   1) The PBO offset must between 0~256MB
 *   2) The CONFIG_LUNA_PBO_DL_DRAM_OFFSET or
 *      CONFIG_LUNA_PBO_UL_DRAM_OFFSET
 *      must be the end of CPU0 MEM Size
 *   3) The DSP/IPC should be put at the end of DRAM!
 ***********************************************/
static inline void bsp_add_highmem(void){
#if defined(CONFIG_RTL8686_CPU_MEM_SIZE) && (CONFIG_RTL8686_CPU_MEM_SIZE > ZONE1_SIZE)
#if !defined(CONFIG_HIGHMEM)
#warning "*******************************************************************"
#warning "****  CONFIG_RTL8686_CPU_MEM_SIZE > 256MB !                 *******"
#warning "****  You should select CONFIG_HIGHMEM to support HIGHMEM.  *******"
#warning "*******************************************************************"
#endif
	if(CONFIG_RTL8686_CPU_MEM_SIZE > ZONE1_SIZE){
    REG32(BSP_CDOR2) = ZONE2_OFF;
    REG32(BSP_CDMAR2) = (CONFIG_RTL8686_CPU_MEM_SIZE - ZONE1_SIZE) - 1;
    add_memory_region(ZONE2_BASE, (CONFIG_RTL8686_CPU_MEM_SIZE - ZONE1_SIZE), BOOT_MEM_RAM);
	}
#else
/* No DSP arch                        */
/* We must get mem size from highmem=??   */
	char *ptr;
	char *ptr_mem;
	char *endptr;	/* local pointer to end of parsed string */
	unsigned long mem_para = 0;

	ptr = strstr(arcs_cmdline, "highmem=");
	if(ptr){
		ptr_mem= ptr+8;
		prom_printf("arcs_cmdline=%s, %p, %p\n", arcs_cmdline, arcs_cmdline, ptr);
		mem_para = simple_strtoull(ptr_mem, &endptr, 0);//MB
		if(mem_para > 0){
			mem_para = mem_para << 20;//MB->Byte
			prom_printf("mem_para=0x%08x\n", mem_para);
			REG32(BSP_CDOR2) = ZONE2_OFF;
			REG32(BSP_CDMAR2) = mem_para - 1;
			add_memory_region(ZONE2_BASE, mem_para , BOOT_MEM_RAM);
		}

	}
#endif
}
/* Add Linux Memory Zone : Normal
 * call <add_memory_region> to register boot_mem_map
 * add_memory_region(base, size, type);
 * type: BOOT_MEM_RAM, BOOT_MEM_ROM_DATA or BOOT_MEM_RESERVED
 */
static void bsp_add_normal(unsigned long normal_size){

	if (normal_size <= ZONE1_SIZE){
		add_memory_region(0, normal_size, BOOT_MEM_RAM);
	}
	else{
		/*Don't add_memory_region with position within 256MB~511MB ( For CPU Logical Address space)*/
		/* The range is reserved for Device's MMIO mapping */
		add_memory_region(0, ZONE1_SIZE, BOOT_MEM_RAM);
	}
}

void __init prom_meminit(void)
{
	char *ptr;
	unsigned int memsize;
#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
	u_long base;
#endif

	prom_dram_unmap_clear();

	/* Check the command line first for a memsize directive */
	ptr = strstr(arcs_cmdline, "mem=");

	if (ptr)
		memsize = memparse(ptr + 4, &ptr);
	else {
		/* No memsize in command line, add a default memory region */
#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
		memsize = min(CONFIG_LUNA_PBO_DL_DRAM_OFFSET, CONFIG_LUNA_PBO_UL_DRAM_OFFSET);
#elif defined(CONFIG_RTL8686_CPU_MEM_SIZE)
		memsize = CONFIG_RTL8686_CPU_MEM_SIZE;
#else
		memsize = 0x04000000;  /* Default to 64MB */
#endif
	}

#ifdef CONFIG_RTL8686_CPU_MEM_SIZE
	if (memsize > CONFIG_RTL8686_CPU_MEM_SIZE)
		memsize = CONFIG_RTL8686_CPU_MEM_SIZE;
#endif /* #ifdef CONFIG_RTL8686_CPU_MEM_SIZE */

#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
	if (memsize > CONFIG_LUNA_PBO_DL_DRAM_OFFSET)
		memsize = CONFIG_LUNA_PBO_DL_DRAM_OFFSET;
	if (memsize > CONFIG_LUNA_PBO_UL_DRAM_OFFSET)
		memsize = CONFIG_LUNA_PBO_UL_DRAM_OFFSET;
#endif /* #ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO */

	bsp_add_normal(memsize);

#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
	/* It is assumed that UL or DL is put on the end of DRAM and the other is put
		 on the end of the previous bank to utilze the strength of para-bank accees
		 E.g., For 32MB DDR2, it has 4 banks(8MB for each).
		 Assuming that DL and UL use 2MB and 2MB respectively, if DL is put
		 on 30MB(end of the DRAM), UL will be put on 22MB(end of the 3rd bank).
	*/
	if(CONFIG_LUNA_PBO_DL_DRAM_OFFSET > CONFIG_LUNA_PBO_UL_DRAM_OFFSET) {
		base = CONFIG_LUNA_PBO_UL_DRAM_OFFSET + CONFIG_LUNA_PBO_UL_DRAM_SIZE;
		memsize = CONFIG_LUNA_PBO_DL_DRAM_OFFSET - base;
	} else {
		base = CONFIG_LUNA_PBO_DL_DRAM_OFFSET + CONFIG_LUNA_PBO_DL_DRAM_SIZE;
		memsize = CONFIG_LUNA_PBO_UL_DRAM_OFFSET - base;
	}
	//prom_printf("base=0x%08x, mem_size=0x%08x\n", base, mem_size);
	add_memory_region(base, memsize, BOOT_MEM_RAM);
#endif /* #ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO */

	/* If there is "mem" in UBoot bootargs, arcs_cmdline will be overwritten       . It is processed at
		 "early_para_mem():linux-2.6.x/arch/rlx/kernel/setup.c"
		 and add_memory_region() is called again to update memory region 0 */

	/* For 512MB or above,
	 * For Apollo Memory Mem Map
	 * ZONE_NORMAL :  0~256  (MB)@Physical
	 * ZONE_HIGH   :  256~xx (MB)@Physical     <----> ((256+256)~ xx+256)(MB)@CPU local AddrSpace
	 */
	bsp_add_highmem();
}

#endif /* #ifdef CONFIG_LUNA_MEMORY_AUTO_DETECTION */

/***********************************
 * Purpose 1: workaround 1004K errata E25.
 * Purpose 2: Force smp_mb( sync ) to be "sync 0x3".
 * set CCMC syncctl bit to 1 for memory sync
 ********************************/
#ifndef __sync_mem
#	define __sync_mem()
#endif

void __init prom_free_prom_memory(void) {
	return;
}

const char *get_system_type(void) {
	return RTK_PLATFORM_NAME;
}

#if (defined CONFIG_USE_UART1) || (defined CONFIG_CPU1_UART1) || (CONFIG_SERIAL_8250_RUNTIME_UARTS > 1)
#	error FIXME: Missing functions for enabling UART1.
#else
static void luna_uart1_pinmux(void) {
	return;
}
#endif /* #if (defined CONFIG_USE_UART1) || (defined CONFIG_CPU1_UART1) || (CONFIG_SERIAL_8250_RUNTIME_UARTS > 1) */

void __init prom_init(void) {
	cg_info_init();

	luna_uart1_pinmux();

  BSP_MHZ = cg_query_freq(CG_DEV_LX);

	prom_meminit();
	mips_set_machine_name(get_system_type());

	bsp_setup_scache();

#ifdef CONFIG_SMP
	plat_smp_init();
#endif

	return;
}
