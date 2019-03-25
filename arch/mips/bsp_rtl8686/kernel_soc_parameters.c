/*
 * luna kernel soc parameters & mtd partition preparation
 * Author: bohungwu@realtek.com.tw
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <asm/bootinfo.h>
#include <linux/bug.h>

#include "prom.h"
#include <soc.h>


extern void prom_printf(char *fmt, ...);

parameter_to_bootloader_t kernel_soc_parameters;
#define plr_param_soc (kernel_soc_parameters.soc)

__init u32 otto_get_flash_size(void) {
	u8 nc, spc;
	u32 ret_val;
#ifdef CONFIG_MTD_NAND_RTK

	ret_val=plr_param_soc.flash_info.page_size * plr_param_soc.flash_info.num_page_per_block*plr_param_soc.flash_info.num_block;

#else
	nc  = plr_param_soc.flash_info.num_chips;
	spc = plr_param_soc.flash_info.size_per_chip;
	//printk("nc=%u, spc=%u\n", nc, spc);
	ret_val = nc * (0x1 << spc);
#endif
	return ret_val;
}

void preloader_parameters_init(void) {

	printk("Compiled soc.h version=0x%08x\n", SOC_HEADER_VERSION);
	printk("sram_parameters.soc.header_ver=0x%08x\n", sram_parameters.soc.header_ver);
	if(sram_parameters.soc.header_ver != SOC_HEADER_VERSION) {
		prom_printf("==================================================================================\n");
		prom_printf("FATAL ERROR: soc.h version mismatch!\n");
		prom_printf("Kernel uses 0x%08x but Preloader adopts 0x%08x\n", SOC_HEADER_VERSION, sram_parameters.soc.header_ver);
		prom_printf("Please align soc.h version between Kernel and Preloader.\n");
		prom_printf("Or disable CONFIG_USE_PRELOADER_PARAMETERS in menuconfig under root directory.\n");
		prom_printf("The system is blocked at file %s:line %u!\n", __FILE__, __LINE__);
		prom_printf("==================================================================================\n");
		while(1);
	}

	memcpy((void *)&kernel_soc_parameters, (void *)&sram_parameters, sizeof(kernel_soc_parameters));

	/* Clear parameters/function ptrs which need to re-filled insdie kernel */
	memset(((void *)&(kernel_soc_parameters.dram_init_result)), 0x0, sizeof(kernel_soc_parameters) - ((void *)&(kernel_soc_parameters.dram_init_result) - (void *)&kernel_soc_parameters)); 

	/* Clear SRAM to avoid deceptive execution results */
	memset( (void *)SRAM_BASE, 0xFF, SRAM_SIZE);

	return;
}



