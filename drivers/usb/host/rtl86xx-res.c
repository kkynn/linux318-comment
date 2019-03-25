/*
 * Copyright 2012, Realtek Semiconductor Corp.
 *
 * drivers/usb/host/rtl8672-res.c
 *
 * $Author: cathy $
 * 
 * USB resource for OHCI or EHCI hcd
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <bspchip.h>
#include <linux/dma-mapping.h>

#ifndef CONFIG_RTL9607C
MODULE_DESCRIPTION("Realtek RTL86XX USB platform device driver");
MODULE_LICENSE("GPL");

static void usb_release(struct device *dev)
{
	/* normally not freed */
	dev->parent = NULL;
}
#define PADDR(addr)  ((addr) & 0x1FFFFFFF)
static u64 rtl86xx_ohci_dmamask = DMA_BIT_MASK(32);
static u64 rtl86xx_ehci_dmamask = DMA_BIT_MASK(32);

#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)
static struct resource rtl86xx_ohci_resources[] = {
	{
		.start	= PADDR(BSP_OHCI_BASE),
		.end	= PADDR(BSP_OHCI_BASE) + 0x00000FFF,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= BSP_USB_H_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device rtl86xx_ohci = {
	.name = "rtl86xx-ohci",
	.id	= -1,
	.dev = {
		.release = usb_release,
		.dma_mask = &rtl86xx_ohci_dmamask,
                .coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources = ARRAY_SIZE(rtl86xx_ohci_resources),
	.resource = rtl86xx_ohci_resources,
};
#endif //CONFIG_USB_OHCI_HCD || CONFIG_USB_OHCI_HCD_MODULE

#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_EHCI_HCD_MODULE)
static struct resource rtl86xx_ehci_resources[] = {
	{
		.start	= PADDR(BSP_EHCI_BASE),
		.end	= PADDR(BSP_EHCI_BASE) + 0x0000EFFF,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= BSP_USB_H_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device rtl86xx_ehci = {
	.name = "rtl86xx-ehci",
	.id	= -1,
	.dev = {
		.release = usb_release,
		.dma_mask = &rtl86xx_ehci_dmamask,
                .coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources = ARRAY_SIZE(rtl86xx_ehci_resources),
	.resource = rtl86xx_ehci_resources,
};
#endif //CONFIG_USB_EHCI_HCD || CONFIG_USB_EHCI_HCD_MODULE

static int __init rtl86xx_hcd_cs_init (void) 
{
	int retval = 0;

#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_EHCI_HCD_MODULE)
	retval = platform_device_register(&rtl86xx_ehci);
	if (retval) {
		printk("%s: fail to register rtl86xx_ehci!\n", __FUNCTION__);
		return retval;
	}
	printk("%s: register rtl86xx_ehci ok!\n", __FUNCTION__);
#endif //CONFIG_USB_EHCI_HCD || CONFIG_USB_EHCI_HCD_MODULE

#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)
	retval = platform_device_register(&rtl86xx_ohci);
	if (retval) {		
	#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_EHCI_HCD_MODULE)
		platform_device_unregister(&rtl86xx_ehci);
	#endif //CONFIG_USB_EHCI_HCD || CONFIG_USB_EHCI_HCD_MODULE
		printk("%s: fail to register rtl86xx_ohci!\n", __FUNCTION__);
		return retval;
	}
	printk("%s: register rtl86xx_ohci ok!\n", __FUNCTION__);
#endif //CONFIG_USB_OHCI_HCD || CONFIG_USB_OHCI_HCD_MODULE

	return retval;
}
module_init (rtl86xx_hcd_cs_init);

static void __exit rtl86xx_hcd_cs_exit(void)
{
#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)
	platform_device_unregister(&rtl86xx_ohci);
#endif //CONFIG_USB_OHCI_HCD || CONFIG_USB_OHCI_HCD_MODULE

#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_EHCI_HCD_MODULE)
	platform_device_unregister(&rtl86xx_ehci);
#endif //CONFIG_USB_EHCI_HCD || CONFIG_USB_EHCI_HCD_MODULE
	return;
}
module_exit(rtl86xx_hcd_cs_exit);

#endif /* CONFIG_RTL9607C */