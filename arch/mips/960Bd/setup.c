/*
 * Realtek Semiconductor Corp.
 *
 * bsp/setup.c
 *     bsp interrult initialization and handler code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/serial_8250.h>
#include <asm/reboot.h>
#include "bspchip.h"

void (* hook_restart_func)(void) = NULL;
void rtk_hook_restart_function(void (*func)(void)) {
	hook_restart_func = func;
	return;
}
EXPORT_SYMBOL(rtk_hook_restart_function);

static void bsp_machine_restart(char *command) {
	if (hook_restart_func) {
		hook_restart_func();
	}

	REG32(0xb8003268) = 0x00000000; //bit 31, 0 -> 1 to reset
	REG32(0xb8003268) = 0x80000000;
	printk("System restarts...");
	return;
}

static void bsp_machine_halt(void) {
	printk("System halts...");
	while(1);
}

/* callback function */
void __init plat_mem_setup(void) {
	/* define io/mem region */
	ioport_resource.start = 0x14000000;
	ioport_resource.end = 0x1fffffff;

 	iomem_resource.start = 0x14000000;
 	iomem_resource.end = 0x1fffffff;

	/* set reset vectors */
	_machine_restart = bsp_machine_restart;
	_machine_halt = bsp_machine_halt;
	pm_power_off = bsp_machine_halt;

	return;
}

/* Detail will be filled at hs_uart_init() @ ./realtek/hs_uart.c */
struct plat_serial8250_port hs_uart_port[] = {
	{
		.irq = INT_UART0,
	}, {
		.irq = INT_UART1,
	}, {
		/* sentinal, must be 0 */
		.irq = 0,
	}
};
