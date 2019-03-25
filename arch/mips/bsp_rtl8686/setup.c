/*
 * Copyright 2006, Realtek Semiconductor Corp.
 *
 * arch/rlx/rlxocp/setup.c
 *   Interrupt and exception initialization for RLX OCP Platform
 *
 * Tony Wu (tonywu@realtek.com.tw)
 * Nov. 7, 2006
 */
#include <linux/console.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include <asm/addrspace.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/bootinfo.h>
#include <asm/time.h>
#include <asm/reboot.h>
//#include <asm/rlxbsp.h>
#include <linux/netdevice.h>

#include "bspchip.h"
#include "prom.h"

#ifdef CONFIG_WIRELESS_LAN_MODULE
static void (*wirelessnet_shutdown_hook)(void) = NULL;

void wirelessnet_shutdown_set(void (*hook) (void))
{
	wirelessnet_shutdown_hook = hook;
}

int wirelessnet_shutdown_loaded(void)
{
	if (wirelessnet_shutdown_hook)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(wirelessnet_shutdown_set);
EXPORT_SYMBOL(wirelessnet_shutdown_loaded);
#endif

__weak void force_stop_wlan_hw(void) {
	printk("%s(%d): empty\n",__func__,__LINE__);
}

static void shutdown_netdev(void)
{
	//struct net_device *dev;

	printk("Shutdown network interface\n");
	read_lock(&dev_base_lock);

#if 0
	for (dev = dev_base; dev != NULL; dev = dev->next) {
		if((dev->flags & IFF_UP) && dev->stop) {
			printk("%s:===>\n",dev->name);
			rtnl_lock();
			dev_close(dev);
			rtnl_unlock();
		}
	}
#endif
	force_stop_wlan_hw();
#if defined (CONFIG_WIRELESS_LAN_MODULE)
	if(wirelessnet_shutdown_loaded())
	{
		wirelessnet_shutdown_hook();
	}
#endif

	read_unlock(&dev_base_lock);
}

extern unsigned int SOC_ID;

static void bsp_machine_restart(char *command)
{
#if defined(CONFIG_DUALBAND_CONCURRENT) && defined(CONFIG_VIRTUAL_WLAN_DRIVER)
        extern void force_stop_vwlan_hw(void);
        force_stop_vwlan_hw();
#endif
#ifndef LUNA_RTL9602C
    local_irq_disable();
#endif
#ifdef CONFIG_NET
	shutdown_netdev();
#endif
	if(SOC_ID == 0x0371){
		REG32(BSP_TC_BASE + 0x24) |= (WDT_E); //enable watch dog
	}
	else{
#ifdef LUNA_RTL9602C
		printk("Set WDT reg. 0x%08x = 0x%08x\n", (BSP_TC_BASE + 0x68), ((WDT_E) | ((31 & WDT_PH12_TO_MSK) << WDT_PH2_TO_SHIFT)));
		REG32(BSP_TC_BASE + 0x68) = ((WDT_E) | ((31 & WDT_PH12_TO_MSK) << WDT_PH2_TO_SHIFT)) ; //enable watch dog
#else
		/* Use system reset to restart the system */
		//REG32(SOFTWARE_RST_REG_A) = (CMD_CHIP_RST_PS); 
		REG32(BSP_TC_BASE + 0x68) = (WDT_E); //enable watch dog
#endif /* #ifdef LUNA_RTL9602C */
	}
    while (1) ;
}
                                                                                                    
static void bsp_machine_halt(void)
{
	force_stop_wlan_hw();
    printk("RTL8672 halted.\n");
    while(1);
}
                                                                                                    
static void bsp_machine_power_off(void)
{
    printk("RTL8672 halted. Please turn off the power.\n");
    while(1);
}

/*
 * callback function
 */
extern void _imem_dmem_init(void);
void __init plat_mem_setup(void)
{
	extern void bsp_serial_init(void);
    /* define io/mem region */
    #if defined(CONFIG_PCI)
    ioport_resource.start = 0x18000000; 
    ioport_resource.end = 0x1fffffff;

    iomem_resource.start = 0x18000000;
    iomem_resource.end = 0x1fffffff;
    #endif

    /* set reset vectors */
    _machine_restart = bsp_machine_restart;
    _machine_halt = bsp_machine_halt;
    pm_power_off = bsp_machine_power_off;

    /* initialize uart */
    bsp_serial_init();
#ifdef CONFIG_RTL8672
    //init at head.S:kernel_entry() => kernel_entry_setup()
#else
    _imem_dmem_init();
#endif //CONFIG_RTL8672
}
