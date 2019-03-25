/*
 * Copyright 2011, Realtek Semiconductor Corp.
 *
 * arch/rlx/bsp/rtl_usb_phy.c
 *
 * $Author: kehsieh $
 * 
 * USB PHY access and USB MAC identification
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <dal/rtl9607c/dal_rtl9607c_switch.h>
#include "bspchip.h"
#include "rtl_usb_phy.h"

#define BSP_USB3_PHY_CTRL		0xB8000508
 #define USB3_CAP_MASK ((1<<27)|(1<<28))
struct phy_regs {
	u32 ehci_utmi_reg;
	u32 usb2_ext_reg;
};
#define PADDR(addr)  ((addr) & 0x1FFFFFFF)
static u64 bsp_usb_dmamask = 0xFFFFFFFFUL;
/* USB Host Controller */
#ifdef CONFIG_USB_OHCI_HCD
static struct resource bsp_usb_ohci_resource[] = {
        [0] = {
                .start = PADDR(BSP_OHCI_BASE),
                .end   = PADDR(BSP_OHCI_BASE) + BSP_OHCI_SIZE - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = BSP_USB_2_IRQ,
                .end   = BSP_USB_2_IRQ,
                .flags = IORESOURCE_IRQ,
        }
};

static struct platform_device bsp_usb_ohci_device = {
        .name          = "rtl86xx-ohci",
        .id            = 0,
        .num_resources = ARRAY_SIZE(bsp_usb_ohci_resource),
        .resource      = bsp_usb_ohci_resource,
        .dev           = {
                .dma_mask = &bsp_usb_dmamask,
                .coherent_dma_mask = 0xFFFFFFFFUL,
        }
};

static struct resource bsp_usb_ohci2_resource[] = {
        [0] = {
                .start = PADDR(BSP_OHCI2_BASE),
                .end   = PADDR(BSP_OHCI2_BASE) + BSP_OHCI_SIZE - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = BSP_USBHOSTP2_IRQ,
                .end   = BSP_USBHOSTP2_IRQ,
                .flags = IORESOURCE_IRQ,
        }
};

static struct platform_device bsp_usb_ohci2_device = {
        .name          = "rtl86xx-ohci",
        .id            = 1,
        .num_resources = ARRAY_SIZE(bsp_usb_ohci2_resource),
        .resource      = bsp_usb_ohci2_resource,
        .dev           = {
                .dma_mask = &bsp_usb_dmamask,
                .coherent_dma_mask = 0xFFFFFFFFUL,
        }
};
#endif
#ifdef CONFIG_USB_EHCI_HCD
static struct resource bsp_usb_ehci_resource[] = {
    [0] = {
        .start = PADDR(BSP_EHCI_BASE),
        .end   = PADDR(BSP_EHCI_BASE) + BSP_EHCI_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = BSP_USB_2_IRQ,
        .end   = BSP_USB_2_IRQ,
        .flags = IORESOURCE_IRQ,
    }
};



static struct platform_device bsp_usb_ehci_device = {
	.name          = "rtl86xx-ehci",
	.id            = 0,
	.num_resources = ARRAY_SIZE(bsp_usb_ehci_resource),
	.resource      = bsp_usb_ehci_resource,
	.dev           = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xFFFFFFFFUL,
	}
};


static struct resource bsp_usb_ehci2_resource[] = {
    [0] = {
        .start = PADDR(BSP_EHCI2_BASE),
        .end   = PADDR(BSP_EHCI2_BASE) + BSP_EHCI_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = BSP_USBHOSTP2_IRQ,
        .end   = BSP_USBHOSTP2_IRQ,
        .flags = IORESOURCE_IRQ,
    }
};

static struct platform_device bsp_usb_ehci2_device = {
	.name          = "rtl86xx-ehci",
	.id            = 1,
	.num_resources = ARRAY_SIZE(bsp_usb_ehci2_resource),
	.resource      = bsp_usb_ehci2_resource,
	.dev           = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xFFFFFFFFUL,
	}
};
#endif
#ifdef CONFIG_USB_XHCI_HCD
static struct resource bsp_xhci_resource[] = {
	{
		//.start = PADDR(BSP_XHCI_BASE),
		//.end = PADDR(BSP_XHCI_BASE) +0x0000EFFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = BSP_USB_23_IRQ,
		.end = BSP_USB_23_IRQ,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device bsp_xhci_device = {
	//.name = "xhci-hcd",
	.name = "rtl86xx-xhci",
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_xhci_resource),
	.resource = bsp_xhci_resource,
	.dev = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif

static struct platform_device *bsp_usb_devs0[] __initdata = {
        #ifdef CONFIG_USB_OHCI_HCD
        &bsp_usb_ohci_device,
        #endif
        #ifdef CONFIG_USB_EHCI_HCD
        &bsp_usb_ehci_device,
        #endif
};

static struct platform_device *bsp_usb_devs2[] __initdata = {
        #ifdef CONFIG_USB_OHCI_HCD
        &bsp_usb_ohci2_device,
        #endif
        #ifdef CONFIG_USB_EHCI_HCD
        &bsp_usb_ehci2_device,
        #endif
};

#ifdef CONFIG_USB_XHCI_HCD
static struct platform_device *bsp_usb_devs1[] __initdata = {	
		&bsp_xhci_device,
};
#endif

/* ============== EHCI ============ */
#define LOOP_LIMIT 100000
#define UTMI_NOP   (1<<12)
#define UTMI_LOAD  (0<<12)
#define BUILD_CMD(vp,vctrl) (((vp)&0xf)<<13)|(((vctrl)&0xf)<<8)
#define UTMI_BUSY(x) (x&(1<<17))
static u32 utmi_wait(struct rtk_usb_phy *phy) {
	u32 __reg, __c = 0;
	struct phy_regs *p = phy->priv;
	u32 utmi_reg = p->ehci_utmi_reg;
	mdelay(1);
	do { 
		__c++; 
		__reg = le32_to_cpu(REG32(utmi_reg));	
		if (unlikely(__c>LOOP_LIMIT)) { 
			printk("utmi_wait timeout\n"); 
			return 0;
		}
	} while (UTMI_BUSY(__reg));
	return __reg;
}

static void utmi_set(struct rtk_usb_phy *phy, u32 v) {
	struct phy_regs *p = phy->priv;
	u32 utmi_reg = p->ehci_utmi_reg;
	utmi_wait(phy);
	REG32(utmi_reg) = cpu_to_le32(v);	
}

static u32 utmi_get(struct rtk_usb_phy *phy) {	
	u32 reg;
	reg = utmi_wait(phy);
	return reg;
}

static int ehci_phy_get(struct rtk_usb_phy *phy, u8 port, u8 reg, u8 *data) {
		
	// send [3:0]
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_LOAD);
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
	
	// send [7:4]
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_LOAD);
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
	
	*data = utmi_get(phy) & 0xff;
	
	return 0;
}


static int ehci_is_valid_param(u8 port, u8 reg) {
	int i;
	
	if (port > 2)
		return 0;
	
	if (((reg >= 0xe0) && (reg <= 0xe7)) ||
	   ((reg >= 0xf0) && (reg <= 0xf7)) ||
	   ((reg >= 0xb0) && (reg <= 0xb7)))
		return 1;
	
	return 0;
}

static int __ehci_phy_read(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 *val) {
	int ret = 0;	
	u8 data;
	
	//printk("%s(%d): %02x %02x %d\n",__func__,__LINE__,port,reg,ehci_is_valid_param(port, reg));
	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;
	
	if (!((reg >= 0xb0) && (reg <= 0xb7)))
		reg -= 0x20;
	
	ehci_phy_get(phy, port, reg, &data);

	*val = data;

	return ret;
}

static int __ehci_phy_write(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val) {
	struct phy_regs *p = phy->priv;	
	u32 usb2_aux_reg = p->usb2_ext_reg+0xc;
	
	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;
	
	//if (port)
	//	REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff0000) | ((val & 0xff) << 16);
	//else
	//	REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff) | (val & 0xff);
	REG32(usb2_aux_reg) = (val)&0xff;
	
	// send [3:0]
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_LOAD);
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
	
	// send [7:4]
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_LOAD);
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
	
	return 0;
	
}

static int __ehci_phy_get_next(struct rtk_usb_phy *phy, u8 port, int *reg) {	
	if (port > 2)
		return -EINVAL;
	
	if (*reg < 0)
		*reg = 0xe0;
	else if ((*reg >= 0xe0) && (*reg <= 0xe6))
		*reg += 1;
	else if (*reg == 0xe7)
		*reg = 0xf0;
	else if ((*reg >= 0xf0) && (*reg <= 0xf6))
		*reg += 1;
	else
		return -EINVAL;	

	return 0;
}

static void __ehci_phy_init(struct rtk_usb_phy *phy) {
	
	struct phy_regs *p = phy->priv;
	
	REG32(p->usb2_ext_reg + 0x10) &= ~(1<<5); /* disable force-host-disconnect */

	mdelay(10); /* allow IP to startup */
	
	__ehci_phy_write(phy, phy->port, 0xF4, 0x9B);
	__ehci_phy_write(phy, phy->port, 0xE4, 0x6C);
	__ehci_phy_write(phy, phy->port, 0xE7, 0x61);
	__ehci_phy_write(phy, phy->port, 0xF4, 0xBB);
	__ehci_phy_write(phy, phy->port, 0xE0, 0x21);
	__ehci_phy_write(phy, phy->port, 0xE0, 0x25);
	
	__ehci_phy_write(phy, phy->port, 0xF4, 0x9B);
	__ehci_phy_write(phy, phy->port, 0xE0, 0xE3);
	__ehci_phy_write(phy, phy->port, 0xE0, 0x63);
	__ehci_phy_write(phy, phy->port, 0xE0, 0xE3);
	
	__ehci_phy_write(phy, phy->port, 0xE0, 0xE3);
	__ehci_phy_write(phy, phy->port, 0xE1, 0x30);
	__ehci_phy_write(phy, phy->port, 0xE2, 0x0B);
	__ehci_phy_write(phy, phy->port, 0xE3, 0x8D);
	__ehci_phy_write(phy, phy->port, 0xE4, 0xE3);
	__ehci_phy_write(phy, phy->port, 0xE5, 0x19);
	__ehci_phy_write(phy, phy->port, 0xE6, 0xC0);
	__ehci_phy_write(phy, phy->port, 0xE7, 0x91);
	__ehci_phy_write(phy, phy->port, 0xF0, 0xFC);
	__ehci_phy_write(phy, phy->port, 0xF1, 0x8C);
	__ehci_phy_write(phy, phy->port, 0xF2, 0x00);
	__ehci_phy_write(phy, phy->port, 0xF3, 0x11);
	__ehci_phy_write(phy, phy->port, 0xF4, 0x9B);
	__ehci_phy_write(phy, phy->port, 0xF5, 0x04);
	__ehci_phy_write(phy, phy->port, 0xF6, 0x00);
	__ehci_phy_write(phy, phy->port, 0xF7, 0x0A);
	
	__ehci_phy_write(phy, phy->port, 0xF4, 0xbb); /* page0 = 0x9b, page1 = 0xbb */
	__ehci_phy_write(phy, phy->port, 0xE0, 0x21);
	__ehci_phy_write(phy, phy->port, 0xE0, 0x25);
	__ehci_phy_write(phy, phy->port, 0xE1, 0xcf);
	__ehci_phy_write(phy, phy->port, 0xE2, 0x00);
	__ehci_phy_write(phy, phy->port, 0xE3, 0x00);
	__ehci_phy_write(phy, phy->port, 0xE4, 0x00);
	__ehci_phy_write(phy, phy->port, 0xE5, 0x11);
	__ehci_phy_write(phy, phy->port, 0xE6, 0x06);
	__ehci_phy_write(phy, phy->port, 0xE7, 0x66);
	__ehci_phy_write(phy, phy->port, 0xF4, 0x9b);
	
}

static struct phy_regs ehci_regs = {
	.ehci_utmi_reg = BSP_EHCI_UTMI_CTRL,
	.usb2_ext_reg = 0xB8140200,
};
static struct rtk_usb_phy ehci_phy = {
	.name = "ehci-phy",	
	.port = 0,
	.vport = 1,
	.priv = &ehci_regs,
	.phy_init = __ehci_phy_init,
	.phy_read = __ehci_phy_read,
	.phy_write = __ehci_phy_write,
	.phy_get_next = __ehci_phy_get_next,
};


static struct phy_regs ehci2_regs = {
	.ehci_utmi_reg = BSP_EHCI2_UTMI_CTRL,
	.usb2_ext_reg = 0xB8140300,
};

static struct rtk_usb_phy ehci2_phy = {
	.name = "ehci2-phy",	
	.port = 2,
	.vport = 1,
	.priv = &ehci2_regs,
	.phy_init = __ehci_phy_init,
	.phy_read = __ehci_phy_read,
	.phy_write = __ehci_phy_write,
	.phy_get_next = __ehci_phy_get_next,
};

/* ============== XHCI U2 ============ */
#define U3UTMI_READY_MASK ((1<<24)|(1<<23))
#define U3UTMI_READY(x) ((x&U3UTMI_READY_MASK)==(1<<24))
#define U3UTMI_NEWREQ (1<<25)
// add another wrapper, since HOST can be in big-endian mode (BSP_USB3_IPCFG)
// Use le32_ func if IPCFG is little-endian mode.

//#define U3_FROM_HOST(x) (x) 
//#define U3_TO_HOST(x) (x) 

#define U3_FROM_HOST(x) le32_to_cpu(x)
#define U3_TO_HOST(x) cpu_to_le32(x)


static void u3_utmi_wait(void) {
	u32 __reg, __c = 0;
	mdelay(1);
	do { 
		__c++; 
		__reg = U3_FROM_HOST(REG32(BSP_XHCI_USB2_PHY_CTRL));	
		if (unlikely(__c>LOOP_LIMIT)) { 
			printk("u3_utmi_wait timeout\n"); 
			return;
		} 
	} while (!U3UTMI_READY(__reg));
}

static void u3_utmi_set(u32 v) {
	u3_utmi_wait();
	REG32(BSP_XHCI_USB2_PHY_CTRL) = U3_TO_HOST(v);	
}

static u32 u3_utmi_get(void) {
	u32 reg;
	u3_utmi_wait();		
	reg = U3_FROM_HOST(REG32(BSP_XHCI_USB2_PHY_CTRL));	
	return reg;
}

static int __xhci_u2phy_read(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 *val) {
	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;
	
	reg = reg - 0x20;
	
	u3_utmi_set(((reg&0x0f) << 8) | U3UTMI_NEWREQ); u3_utmi_wait();
	u3_utmi_set(((reg&0xf0) << 4) | U3UTMI_NEWREQ); u3_utmi_wait();
	u3_utmi_set(                    U3UTMI_NEWREQ); u3_utmi_wait();
	*val = (u3_utmi_get() & 0xff);
	return 0;
}

static int __xhci_u2phy_write(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val) {
	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;
	
	if (port)
		REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff0000) | ((val & 0xff) << 16);
	else
		REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff) | (val & 0xff);
	
	u3_utmi_set(((reg&0x0f) << 8) | U3UTMI_NEWREQ); u3_utmi_wait();
	u3_utmi_set(((reg&0xf0) << 4) | U3UTMI_NEWREQ); u3_utmi_wait();
	
	return 0;
}

static void __xhci_u2phy_init(struct rtk_usb_phy *phy) {
	printk("XHCI U2 Phy Init\n");
	
	#if 1
	__xhci_u2phy_write(phy, phy->port, 0xF4, 0x9B);
	__xhci_u2phy_write(phy, phy->port, 0xE4, 0x6C);
	__xhci_u2phy_write(phy, phy->port, 0xE7, 0x61);
	__xhci_u2phy_write(phy, phy->port, 0xF4, 0xBB);
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0x21);
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0x25);	
	
	__xhci_u2phy_write(phy, phy->port, 0xF4, 0x9B);
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0xE3);
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0x63);
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0xE3);	
	
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0xE3);
	__xhci_u2phy_write(phy, phy->port, 0xE1, 0x30);
	__xhci_u2phy_write(phy, phy->port, 0xE2, 0x0B);
	__xhci_u2phy_write(phy, phy->port, 0xE3, 0x8D);
	__xhci_u2phy_write(phy, phy->port, 0xE4, 0xE3);
	__xhci_u2phy_write(phy, phy->port, 0xE5, 0x19);
	__xhci_u2phy_write(phy, phy->port, 0xE6, 0xC0);
	__xhci_u2phy_write(phy, phy->port, 0xE7, 0x91);
	__xhci_u2phy_write(phy, phy->port, 0xF0, 0xFC);
	__xhci_u2phy_write(phy, phy->port, 0xF1, 0x8C);
	__xhci_u2phy_write(phy, phy->port, 0xF2, 0x00);
	__xhci_u2phy_write(phy, phy->port, 0xF3, 0x11);
	__xhci_u2phy_write(phy, phy->port, 0xF4, 0x9B);
	__xhci_u2phy_write(phy, phy->port, 0xF5, 0x04);
	__xhci_u2phy_write(phy, phy->port, 0xF6, 0x00);
	__xhci_u2phy_write(phy, phy->port, 0xF7, 0x0A);	
	
	__xhci_u2phy_write(phy, phy->port, 0xF4, 0xbb); /* page0 = 0x9b, page1 = 0xbb */
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0x21);
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0x25);
	__xhci_u2phy_write(phy, phy->port, 0xE1, 0xcf);
	__xhci_u2phy_write(phy, phy->port, 0xE2, 0x00);
	__xhci_u2phy_write(phy, phy->port, 0xE3, 0x00);
	__xhci_u2phy_write(phy, phy->port, 0xE4, 0x00);
	__xhci_u2phy_write(phy, phy->port, 0xE5, 0x11);
	__xhci_u2phy_write(phy, phy->port, 0xE6, 0x06);
	__xhci_u2phy_write(phy, phy->port, 0xE7, 0x66);
	__xhci_u2phy_write(phy, phy->port, 0xF4, 0x9b);
	#endif 
}

static struct rtk_usb_phy xhci_u2phy = {
	.name = "xhci-u2",	
	.port = 0,
	.vport = 1,
	.phy_init = __xhci_u2phy_init,
	.phy_read = __xhci_u2phy_read,
	.phy_write = __xhci_u2phy_write,
	.phy_get_next = __ehci_phy_get_next,
};

/* ============== XHCI U3 ============ */
#define MDIO_READY_MASK ((3<<5)|(1<<4)|(1<<1))
#define MDIO_READY(x) ((x&MDIO_READY_MASK)==(1<<4))
static u32 mdio_wait(void) {	
	u32 __reg, __c = 0;
	mdelay(1);
	do { 
		__c++; 
		__reg = REG32(BSP_USB3_MDIO);	
		if (unlikely(__c>LOOP_LIMIT)) { 
			printk("mdio_wait timeout\n"); 
			return 0;
		} 
	} while (!MDIO_READY(__reg));
		
	return __reg;
}



static int __xhci_u3phy_read(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 *val) {
	u32 v;
	mdio_wait();	 
	REG32(BSP_USB3_MDIO) = (reg << 8);
	v = mdio_wait();
	*val = (v>>16);
	return 0;
}

static int __xhci_u3phy_write(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val) {	
	mdio_wait();	
	REG32(BSP_USB3_MDIO) = (val << 16) | (reg << 8) | 1;
	return 0;
}

static int __xhci_u3_get_next(struct rtk_usb_phy *phy, u8 port, int *reg) {
	if (*reg < 0) {
		*reg = 0;
		return 0;
	} else 	if ((*reg >= 0) && (*reg < 0x30)) {
		*reg = *reg + 1;
		return 0;
	}
	return -EINVAL;
}

static void __xhci_u3phy_init(struct rtk_usb_phy *phy) {
	printk("XHCI U3 Phy Init\n");
	
	__xhci_u3phy_write(phy, 0, 0x00, 0x400c);
	__xhci_u3phy_write(phy, 0, 0x04, 0xf000);
	__xhci_u3phy_write(phy, 0, 0x05, 0x230f);
	
	__xhci_u3phy_write(phy, 0, 0x01, 0xe058);
	__xhci_u3phy_write(phy, 0, 0x02, 0x6006);
	__xhci_u3phy_write(phy, 0, 0x03, 0x2771);	
	__xhci_u3phy_write(phy, 0, 0x06, 0x0001);
	__xhci_u3phy_write(phy, 0, 0x07, 0x2e00);
	__xhci_u3phy_write(phy, 0, 0x08, 0x3191);
	__xhci_u3phy_write(phy, 0, 0x09, 0x501c);
	__xhci_u3phy_write(phy, 0, 0x09, 0x521c);
	__xhci_u3phy_write(phy, 0, 0x0a, 0x9601);
	__xhci_u3phy_write(phy, 0, 0x0b, 0xa905);
	__xhci_u3phy_write(phy, 0, 0x0c, 0xc000);
	__xhci_u3phy_write(phy, 0, 0x0d, 0xef1c);
	__xhci_u3phy_write(phy, 0, 0x0e, 0x2010);
	__xhci_u3phy_write(phy, 0, 0x0f, 0x8020);
	__xhci_u3phy_write(phy, 0, 0x10, 0x000c);
	__xhci_u3phy_write(phy, 0, 0x11, 0x4c00);
	__xhci_u3phy_write(phy, 0, 0x12, 0xfc00);
	__xhci_u3phy_write(phy, 0, 0x13, 0x0c81);
	__xhci_u3phy_write(phy, 0, 0x14, 0xde01);
	__xhci_u3phy_write(phy, 0, 0x15, 0x0000);
	__xhci_u3phy_write(phy, 0, 0x16, 0x0000);
	__xhci_u3phy_write(phy, 0, 0x17, 0x0000);
	__xhci_u3phy_write(phy, 0, 0x18, 0x0000);
	__xhci_u3phy_write(phy, 0, 0x19, 0xe004);
	__xhci_u3phy_write(phy, 0, 0x1a, 0x1260);
	__xhci_u3phy_write(phy, 0, 0x1b, 0xff06);
	__xhci_u3phy_write(phy, 0, 0x1c, 0xcb00);
	__xhci_u3phy_write(phy, 0, 0x1d, 0xa03f);
	__xhci_u3phy_write(phy, 0, 0x1e, 0xc2e0);
	__xhci_u3phy_write(phy, 0, 0x1f, 0x9001);
	__xhci_u3phy_write(phy, 0, 0x20, 0xd4aa);
	__xhci_u3phy_write(phy, 0, 0x21, 0x88aa);
	__xhci_u3phy_write(phy, 0, 0x22, 0x0057);
	__xhci_u3phy_write(phy, 0, 0x23, 0xab65);
	__xhci_u3phy_write(phy, 0, 0x24, 0x0800);
	__xhci_u3phy_write(phy, 0, 0x25, 0x0000);
	__xhci_u3phy_write(phy, 0, 0x26, 0x040b);
	__xhci_u3phy_write(phy, 0, 0x27, 0x023f);
	__xhci_u3phy_write(phy, 0, 0x28, 0xf802);
	__xhci_u3phy_write(phy, 0, 0x29, 0x3080);
	__xhci_u3phy_write(phy, 0, 0x2a, 0x3082);
	__xhci_u3phy_write(phy, 0, 0x2b, 0x2078);
	__xhci_u3phy_write(phy, 0, 0x2c, 0x0000);
	__xhci_u3phy_write(phy, 0, 0x2d, 0x00ff);
	__xhci_u3phy_write(phy, 0, 0x2e, 0x0000);
	__xhci_u3phy_write(phy, 0, 0x2f, 0x0000);
	__xhci_u3phy_write(phy, 0, 0x30, 0x0000);	
}

static struct rtk_usb_phy xhci_u3phy = {
	.name = "xhci-u3",	
	.port = 0,
	.vport = 1,
	.phy_init = __xhci_u3phy_init,
	.phy_read = __xhci_u3phy_read,
	.phy_write = __xhci_u3phy_write,
	.phy_get_next = __xhci_u3_get_next,
};

unsigned long BSP_XHCI_BASE	    = 0xB8040000;
unsigned long BSP_USB3_EXT_REG	= 0xB8140000;
unsigned long BSP_USB2_EXT_REG	= 0xB8140200;

extern int rtk_usb_phy_register(struct rtk_usb_phy *phy);

static int __init_plat_xhci(void) {
	#if defined(CONFIG_USB_XHCI_HCD)	
	u32 reg;
	#ifdef USE_XHCI_VER_3_10
	reg = REG32(NEW_BSP_IP_SEL) & ~(7<<29);
	REG32(NEW_BSP_IP_SEL) = reg | (2<<29) | (1<<31) | (1<<26); 		
	if (REG32(NEW_BSP_IP_SEL) & (1<<31)) {
		printk("XHCI v3.10\n");
		BSP_XHCI_BASE	 = 0xB8060000;
		BSP_USB3_EXT_REG = 0xB8140100;
		BSP_USB2_EXT_REG = 0xB8140300;		
	}
	#endif
	REG32(BSP_IP_SEL) |= (3<<4) | (1<<29) | (1<<30); mdelay(10);
	
	bsp_xhci_resource[0].start = PADDR(BSP_XHCI_BASE);
	bsp_xhci_resource[0].end   = PADDR(BSP_XHCI_BASE) +0x0000EFFF;
	
	//printk("XHCI reset...\n");	
		
	//REG32(BSP_USB3_IPCFG) &= ~(1<<6); /* host-order */
	REG32(BSP_USB3_IPCFG) |= (0x2); /* wake u2 */
	REG32(BSP_USB3_IPCFG) |= (1<<27); /* disable RX50T */
	mdelay(10);	
	REG32(BSP_USB3_IPCFG) &= ~(1<<27); /* put it back RX50T */
	
	#ifndef USE_XHCI_VER_3_10
	// fix default MAC value
	REG32(BSP_XHCI_BASE+0xc108)=cpu_to_le32(0x21010000);
	REG32(BSP_XHCI_BASE+0xc10c)=cpu_to_le32(0x21080000);
	REG32(BSP_XHCI_BASE+0xc2c0)=cpu_to_le32(0x00041202);
	#endif 
	
 	REG32(BSP_XHCI_BASE+0x430) |= (0x80); /* PORT reset */ //REG32(0xB8040430) |= 1<<31; 
	rtk_usb_phy_register(&xhci_u2phy);
	rtk_usb_phy_register(&xhci_u3phy);
	
	return platform_add_devices(bsp_usb_devs1, ARRAY_SIZE(bsp_usb_devs1));
	    
	#else
	return 0;
	#endif
	
}

static int __init_plat_ehci2(void) {
	#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_OHCI_HCD)
	u32 reg;
	//printk("EHCI2 initialization2\n");
	reg = REG32(NEW_BSP_IP_SEL) & ~(7<<29);
	REG32(NEW_BSP_IP_SEL) = reg | (1<<29); 
	if ((REG32(NEW_BSP_IP_SEL) >> 29)!=0x1) {
		return -EINVAL;
	}
	
	REG32(BSP_IP_SEL) |= (1<<30) | (1<<13); mdelay(5);
	
	rtk_usb_phy_register(&ehci2_phy);
	
	return platform_add_devices(bsp_usb_devs2, ARRAY_SIZE(bsp_usb_devs2));
	#else
	return 0;
	#endif 
}

static int __init bsp_usb_init_9607c(void)
{
    int ret = 0;	
	#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_OHCI_HCD)
	REG32(BSP_IP_SEL) |= (1<<3) | (1<<31); mdelay(5);
	ehci_phy.port = 1;
	rtk_usb_phy_register(&ehci_phy);
	#endif 
	
    ret = platform_add_devices(bsp_usb_devs0, ARRAY_SIZE(bsp_usb_devs0));
    if (ret < 0) {
            printk("ERROR: unable to add EHCI\n");
            return ret;
    }
	
	#if defined(CONFIG_USB_9607C_EHCI2)
	REG32(BSP_USB3_PHY_CTRL) &= ~(USB3_CAP_MASK);
	ret = __init_plat_ehci2();
    if (ret < 0) {
        printk("ERROR: unable to add EHCI2\n");
        return ret;
    }
	#else // EHCI2	
	ret = __init_plat_xhci();
	if (ret < 0) {
		printk("ERROR: unable to add XHCI\n");
		return ret;    
	}	
	#endif // EHCI2

    return 0;
}

static int __init bsp_usb_init_9603c(void)
{
    int ret = 0;
	u32 reg;
	REG32(BSP_USB3_PHY_CTRL) &= ~(USB3_CAP_MASK);
	ret = __init_plat_ehci2();
	
    if (ret < 0) {
        printk("ERROR: unable to add EHCI\n");
        return ret;
    }
	
    return 0;
}

static int __init bsp_usb_init(void) {
	uint32  chipId,rev,subType  = 0;
    rtk_switch_version_get(&chipId,&rev,&subType);
	
	switch(subType)
    {
    case RTL9607C_CHIP_SUB_TYPE_RTL9607CP:
	case RTL9607C_CHIP_SUB_TYPE_RTL9607EP:
	case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA5:
    case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6:
    case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7:
	case RTL9607C_CHIP_SUB_TYPE_RTL9607E_VA5:
	case RTL9607C_CHIP_SUB_TYPE_RTL9603CP:
		return bsp_usb_init_9607c();
	case RTL9607C_CHIP_SUB_TYPE_RTL9603CT:
    case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4:
    case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5:
    case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6:
    case RTL9607C_CHIP_SUB_TYPE_RTL9603CE:
		return bsp_usb_init_9603c();                
	}
	printk("USB not initialized, %u/%u/%u\n",chipId,rev,subType);
	return;
}

module_init(bsp_usb_init);

void set_usb2phy_xhci(void) {
	printk("%s(%d)\n",__func__,__LINE__);
}

void usb_enable_IP(void) {
	printk("%s(%d)\n",__func__,__LINE__);
}

void set_usbphy(void) {
	printk("%s(%d)\n",__func__,__LINE__);
}
