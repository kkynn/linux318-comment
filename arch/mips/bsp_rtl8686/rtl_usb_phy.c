/*
 * Copyright 2011, Realtek Semiconductor Corp.
 *
 * arch/rlx/bsp/rtl_usb_phy.c
 *
 * $Author: cathy $
 * 
 * USB PHY access and USB MAC identification
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include "bspchip.h"
#include "rtl_usb_phy.h"

//#define PADDR(addr)  ((addr) & 0x1FFFFFFF)
//static u64 bsp_usb_dmamask = 0xFFFFFFFFUL;
///* USB Host Controller */
//#ifdef CONFIG_USB_OHCI_HCD
//static struct resource bsp_usb_ohci_resource[] = {
//        [0] = {
//                .start = PADDR(BSP_OHCI_BASE),
//                .end   = PADDR(BSP_OHCI_BASE) + BSP_OHCI_SIZE - 1,
//                .flags = IORESOURCE_MEM,
//        },
//        [1] = {
//                .start = BSP_USB_2_IRQ,
//                .end   = BSP_USB_2_IRQ,
//                .flags = IORESOURCE_IRQ,
//        }
//};
//
//static struct platform_device bsp_usb_ohci_device = {
//        .name          = "rtl86xx-ohci",
//        .id            = -1,
//        .num_resources = ARRAY_SIZE(bsp_usb_ohci_resource),
//        .resource      = bsp_usb_ohci_resource,
//        .dev           = {
//                .dma_mask = &bsp_usb_dmamask,
//                .coherent_dma_mask = 0xFFFFFFFFUL,
//        }
//};
//#endif
//#ifdef CONFIG_USB_EHCI_HCD
//static struct resource bsp_usb_ehci_resource[] = {
//    [0] = {
//        .start = PADDR(BSP_EHCI_BASE),
//        .end   = PADDR(BSP_EHCI_BASE) + BSP_EHCI_SIZE - 1,
//        .flags = IORESOURCE_MEM,
//    },
//    [1] = {
//        .start = BSP_USB_2_IRQ,
//        .end   = BSP_USB_2_IRQ,
//        .flags = IORESOURCE_IRQ,
//    }
//};
//
//
//
//static struct platform_device bsp_usb_ehci_device = {
//	.name          = "rtl86xx-ehci",
//	.id            = -1,
//	.num_resources = ARRAY_SIZE(bsp_usb_ehci_resource),
//	.resource      = bsp_usb_ehci_resource,
//	.dev           = {
//		.dma_mask = &bsp_usb_dmamask,
//		.coherent_dma_mask = 0xFFFFFFFFUL,
//	}
//};
//#endif
//#ifdef CONFIG_USB_XHCI_HCD
//static struct resource bsp_xhci_resource[] = {
//	{
//		.start = PADDR(BSP_XHCI_BASE),
//		.end = PADDR(BSP_XHCI_BASE) +0x0000EFFF,
//		.flags = IORESOURCE_MEM,
//	},
//	{
//		.start = BSP_USB_23_IRQ,
//		.end = BSP_USB_23_IRQ,
//		.flags = IORESOURCE_IRQ,
//	}
//};

//struct platform_device bsp_xhci_device = {
//	//.name = "xhci-hcd",
//	.name = "rtl86xx-xhci",
//	.id = -1,
//	.num_resources = ARRAY_SIZE(bsp_xhci_resource),
//	.resource = bsp_xhci_resource,
//	.dev = {
//		.dma_mask = &bsp_usb_dmamask,
//		.coherent_dma_mask = 0xffffffffUL
//	}
//};
//#endif

//static struct platform_device *bsp_usb_devs0[] __initdata = {
//        #ifdef CONFIG_USB_OHCI_HCD
//        &bsp_usb_ohci_device,
//        #endif
//        #ifdef CONFIG_USB_EHCI_HCD
//        &bsp_usb_ehci_device,
//        #endif
//};

//#ifdef CONFIG_USB_XHCI_HCD
//static struct platform_device *bsp_usb_devs1[] __initdata = {	
//		&bsp_xhci_device,
//};
//#endif

static u32 clk;
/* ============== EHCI ============ */
#define LOOP_LIMIT 100000
#define UTMI_NOP   (1<<12)
#define UTMI_LOAD  (0<<12)
#define BUILD_CMD(vp,vctrl) (((vp)&0xf)<<13)|(((vctrl)&0xf)<<8)
#define UTMI_BUSY(x) (x&(1<<17))
static u32 utmi_wait(void) {
	u32 __reg, __c = 0;
	do { 
		__c++; 
		__reg = le32_to_cpu(REG32(BSP_EHCI_UTMI_CTRL));	
		if (unlikely(__c>LOOP_LIMIT)) { 
			printk("utmi_wait timeout\n"); 
	return 0;
}
	} while (UTMI_BUSY(__reg));
	return __reg;
}

static void utmi_set(u32 v) {
	utmi_wait();
	REG32(BSP_EHCI_UTMI_CTRL) = cpu_to_le32(v);	
}

static u32 utmi_get(void) {	
	u32 reg;
	reg = utmi_wait();
	return reg;
}

static int ehci_phy_get(struct rtk_usb_phy *phy, u8 port, u8 reg, u8 *data) {

	// send [3:0]
	utmi_set(BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
	utmi_set(BUILD_CMD(phy->vport,reg&0xf)|UTMI_LOAD);
	utmi_set(BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);

	// send [7:4]
	utmi_set(BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
	utmi_set(BUILD_CMD(phy->vport,reg>>4)|UTMI_LOAD);
	utmi_set(BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);

	*data = utmi_get() & 0xff;

	return 0;
}


static int ehci_is_valid_param(u8 port, u8 reg) {
	int i;

	if (port > 1)
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
	
	ehci_phy_get(phy, 0xff, reg, &data);

	*val = data;

	return ret;
	}

static int __ehci_phy_write(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val) {
	
	
	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;
	#ifdef CONFIG_RTL9602C_SERIES
	REG32(BSP_USB2_AUX_REG) = (val)&0xff;
	#else 
	if (port)
		REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff0000) | ((val & 0xff) << 16);
	else
		REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff) | (val & 0xff);
	#endif

	// send [3:0]
	utmi_set(BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
	utmi_set(BUILD_CMD(phy->vport,reg&0xf)|UTMI_LOAD);
	utmi_set(BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);

	// send [7:4]
	utmi_set(BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
	utmi_set(BUILD_CMD(phy->vport,reg>>4)|UTMI_LOAD);
	utmi_set(BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);

	return 0;

}

static int __ehci_phy_get_next(struct rtk_usb_phy *phy, u8 port, int *reg) {	
	if (port > 1)
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
	
	//andrew, mantis 6066, bug 1
	REG32(0xb8021094) = 0x80008000;
	
	#ifdef CONFIG_RTL9602C_SERIES
	REG32(0xb8140210) &= ~(1<<5); /* disable force-host-disconnect */
	__ehci_phy_write(phy, phy->port, 0xF4, 0xBB); /* page 1 */
	__ehci_phy_write(phy, phy->port, 0xE5, 0xFA);
	__ehci_phy_write(phy, phy->port, 0xF4, 0x9B); /* page 0 */
	#else 
	__ehci_phy_write(phy, phy->port, 0xE0, 0x9C);
	__ehci_phy_write(phy, phy->port, 0xE1, 0xAC);
	__ehci_phy_write(phy, phy->port, 0xE2, 0x99);
	__ehci_phy_write(phy, phy->port, 0xE5, 0x9D);
	__ehci_phy_write(phy, phy->port, 0xE7, 0x3D);
	 if (clk & BSP_SYS_CLK_SRC_40MHZ) {
		 __ehci_phy_write(phy, phy->port, 0xF5, 0xC1);
	 }
	#endif 
	}

static struct rtk_usb_phy ehci_phy = {
	.name = "ehci-phy",	
	.port = 0,
	.vport = 1,
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
	__xhci_u2phy_write(phy, phy->port, 0xE0, 0x9C);
	__xhci_u2phy_write(phy, phy->port, 0xE1, 0xAC);
	__xhci_u2phy_write(phy, phy->port, 0xE2, 0x9D);
	__xhci_u2phy_write(phy, phy->port, 0xE5, 0x9D);
	if (clk & BSP_SYS_CLK_SRC_40MHZ) {
		__xhci_u2phy_write(phy, phy->port, 0xF5, 0xC1);
	} else {
		__xhci_u2phy_write(phy, phy->port, 0xE7, 0x1D);
	}	
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

extern int rtk_usb_phy_register(struct rtk_usb_phy *phy);
static int __init bsp_usb_init(void)
{
    int ret = 0;
	extern unsigned int SOC_ID;
	//clk = REG32(BSP_SYS_CLK_SEL);
	// EHCI: bit 3,31  XHCI: bit 4,5,30
	
	#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_OHCI_HCD)
	#ifdef CONFIG_RTL9602C_SERIES	
	REG32(BSP_IP_SEL) |= BSP_EN_USB_PHY0_USB2|(BSP_USBPHY_P0_EN);
	mdelay(10);
	#else
        if(SOC_ID != 0x0371){		
			REG32(BSP_IP_SEL) |= (BSP_EN_USB_PHY1 | BSP_USBPHY_P1_EN);
            mdelay(10);
        }
	#endif
	ehci_phy.port = 1;	
	rtk_usb_phy_register(&ehci_phy);	
	#endif 

    //ret = platform_add_devices(bsp_usb_devs0, ARRAY_SIZE(bsp_usb_devs0));
    //if (ret < 0) {
    //        printk("ERROR: unable to add EHCI\n");
    //        return ret;
    //}

	#if defined(CONFIG_USB_XHCI_HCD)		
	REG32(BSP_IP_SEL) |= BSP_EN_USB_PHY0_USB2_USB3 | BSP_USBPHY_P0_EN | (1<<13);
	mdelay(10);
	printk("XHCI reset...\n");	
	rtk_usb_phy_register(&xhci_u2phy);
	rtk_usb_phy_register(&xhci_u3phy);

	//ret = platform_add_devices(bsp_usb_devs1, ARRAY_SIZE(bsp_usb_devs1));
    //if (ret < 0) {
	//	printk("ERROR: unable to add XHCI\n");
	//	return ret;    
	//}
	#endif	
    return 0;
}

module_init(bsp_usb_init);

void set_usb2phy_xhci(void) {
	printk("%s(%d)\n",__func__,__LINE__);
	}	

void usb_enable_IP(void)
{
	//REG32(BSP_IP_SEL) |= (BSP_EN_USB_PHY0_USB2_USB3 | BSP_EN_USB_PHY1);
	#ifdef CONFIG_USB_XHCI_HCD
	REG32(BSP_IP_SEL) |= (BSP_EN_USB_PHY0_USB2_USB3 | BSP_USBPHY_P0_EN) | (1<<13);
	#endif
	#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_OHCI_HCD)
	REG32(BSP_IP_SEL) |= (BSP_EN_USB_PHY1 | BSP_USBPHY_P1_EN);
	#endif
	return;
}

void set_usbphy(void) {
        printk("%s(%d)\n",__func__,__LINE__);
}