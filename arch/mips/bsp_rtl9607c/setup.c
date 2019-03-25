/*
 * Realtek Semiconductor Corp.
 *
 * bsp/setup.c
 *     bsp interrult initialization and handler code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/interrupt.h>
#include <linux/sched.h>

#include <asm/prom.h>
#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/time.h>
#include <asm/reboot.h>

#include "bspchip.h"
#include "bspchip_9607c.h"
#include "bspcpu.h"

#ifdef CONFIG_SDK_FPGA_PLATFORM //FOR FPGA TEST

#include <asm/mips-cm.h>
#include <asm/mips-cpc.h>
#include <asm/r4kcache.h>

#ifndef CONFIG_MIPS_CPC
void __iomem *mips_cpc_base;

#endif
/*
#define CP0_TAGLO2	$28,4
#define CP0_DATALO2	$28,5
#define CP0_DATAHI2	$29,5
*/
#define read_c0_taglo2()	__read_ulong_c0_register($28, 4)
#define write_c0_taglo2(val)	__write_ulong_c0_register($28, 4, val)


#define read_c0_datalo2()	__read_ulong_c0_register($28, 5)
#define write_c0_datalo2(val)	__write_ulong_c0_register($28, 5, val)

#define read_c0_datahi2()	__read_ulong_c0_register($29, 5)
#define write_c0_datahi2(val)	__write_ulong_c0_register($29, 5, val)


static void inv_scache(void){
	unsigned long cache_size ;
	unsigned long linesz,sets,ways;
	int i;
	unsigned int config2;
	unsigned int tmp;
  
 write_c0_taglo2(0);
 write_c0_datalo2(0);
 write_c0_datahi2(0);
	config2 = read_c0_config2();
                /* L2-Cache */
	tmp = (config2 >> 4) & 0x0f;
	if (0 < tmp && tmp <= 7)
		linesz = 2 << tmp;
	else
		return ;		
		
	tmp = (config2 >> 8) & 0x0f;
	if (0 <= tmp && tmp <= 7)
		sets = 64 << tmp;
	else
		return ;

	tmp = (config2 >> 0) & 0x0f;
	if (0 <= tmp && tmp <= 7)
		ways = tmp + 1;
	else
		return ;
	

	cache_size = sets *  ways * linesz;
	
		
        for(i=CKSEG0;  i < (CKSEG0 + cache_size); i +=  linesz){
	  cache_op(Index_Store_Tag_SD,i);
	}
	__sync_mem();
	return ;  
 
 
}

static void flush_l2cache(void){
	unsigned long cache_size ;
	unsigned long linesz,sets,ways;
	int i;
	unsigned int config2;
	unsigned int tmp;
	config2 = read_c0_config2();
	if (config2 & (1 << 12))
			return;
	
			      
                /* L2-Cache */
	tmp = (config2 >> 4) & 0x0f;
	if (0 < tmp && tmp <= 7)
		linesz = 2 << tmp;
	else
		return ;		
		
	tmp = (config2 >> 8) & 0x0f;
	if (0 <= tmp && tmp <= 7)
		sets = 64 << tmp;
	else
		return ;

	tmp = (config2 >> 0) & 0x0f;
	if (0 <= tmp && tmp <= 7)
		ways = tmp + 1;
	else
		return ;
	

	cache_size = sets *  ways * linesz;
	
	printk("L2cache LineSize=%lu, Sets=%lu, Ways=%lu, CacheSize=%lu\n", linesz, sets, ways, cache_size);
	
        for(i=CKSEG0;  i < (CKSEG0 + cache_size); i +=  linesz){
	  cache_op(Index_Writeback_Inv_SD,i);
	}
	__sync_mem();
	return ;
}

static void clean_scache(void){
   __sync_mem();
  write_gcr_base(read_gcr_base() | 0x50 );
 // flush_l2cache();
  inv_scache();
  
  write_gcr_base(read_gcr_base() | 0x50 );
  __sync_mem();
}

void bsp_cpc_reset(void *info){
  
#ifndef CONFIG_MIPS_CPC
        write_gcr_cpc_base(CPC_BASE_ADDR | 0x01);
        printk("read_gcr_cpc_base() = 0x%x\n", read_gcr_cpc_base() );
	mips_cpc_base = ioremap_nocache(CPC_BASE_ADDR, 0x8000);
	if (!mips_cpc_base)
		return ;

#endif
	clean_scache();
	write_gcr_cl_other(1 << CM_GCR_Cx_OTHER_CORENUM_SHF);
        write_gcr_co_coherence(0x0);
        smp_wmb();
	write_cpc_cl_other(1 << CPC_Cx_OTHER_CORENUM_SHF);
        write_cpc_co_cmd(1);//Clock off
	write_cpc_co_cmd(2);//PWRDown


        /* For Core0 */
        write_gcr_cl_coherence(0x0);
        smp_wmb();
        write_cpc_cl_cmd(4);//Reset
}
#endif

void (* hook_restart_func)(void) = NULL;

void rtk_hook_restart_function(void (*func)(void))
{
	hook_restart_func = func;
	return;
}


__weak void force_stop_wlan_hw(void) {
	printk("%s(%d): empty\n",__func__,__LINE__);
}

static void bsp_machine_restart(char *command)
{
	if(hook_restart_func != NULL)
	{
		hook_restart_func();
	}

	force_stop_wlan_hw();	 

	printk("System restart.\n");
	
#ifdef CONFIG_SDK_FPGA_PLATFORM
       smp_call_function_single(0, bsp_cpc_reset, NULL,0);
#else
       REG32(BSP_TC_BASE + 0x68) = (WDT_E); //enable watch dog
#endif

}

static void bsp_machine_halt(void)
{
	printk("System halted.\n");
	while(1);
}

#ifdef CONFIG_MIPS_MT_SMTC
extern struct plat_smp_ops bsp_smtc_smp_ops;
#endif

/* callback function */
void __init plat_mem_setup(void)
{

	/* define io/mem region */
	ioport_resource.start = 0x14000000;
	ioport_resource.end = 0x1fffffff;

 	iomem_resource.start = 0x14000000;
 	iomem_resource.end = 0x1fffffff;
	
	/* set reset vectors */
	
        _machine_restart = bsp_machine_restart;
        _machine_halt = bsp_machine_halt;
        pm_power_off = bsp_machine_halt;
	
}

EXPORT_SYMBOL(rtk_hook_restart_function);

