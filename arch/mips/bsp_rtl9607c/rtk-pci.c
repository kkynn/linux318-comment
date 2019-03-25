#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <bspchip.h>
#include <bspgpio.h>

#define PADDR(addr)  ((addr) & 0x1FFFFFFF)

struct pcie_para{
    u8 port;
    u8 reg;
    u16 value;
};

#define MAX_NUM_DEV  4
#define FL_MDIO_RESET	(1<<0)
#define FL_SET_BAR		(1<<1)
struct rtk_pci_controller {
	void __iomem *devcfg_base;
	void __iomem *hostcfg_base;
	void __iomem *hostext_base;	
	spinlock_t lock;
	struct pci_controller controller;
	struct pcie_para *phy_param;
	void (*rst_fn)(struct rtk_pci_controller *, int);
	unsigned long rst_data;
	u8 irq;
	u8 flags;
	u8 port;
	u8 is_linked;
	u8 bus_addr;
};

static inline struct rtk_pci_controller * pci_bus_to_rtk_controller(struct pci_bus *bus)
{
	struct pci_controller *hose;

	hose = (struct pci_controller *) bus->sysdata;
	return container_of(hose, struct rtk_pci_controller, controller);
}

static void rst_by_gpio(struct rtk_pci_controller *ctrl, int action) {	
	switch(action) {
	case 0:
		gpioClear(ctrl->rst_data);
		gpioConfig(ctrl->rst_data, GPIO_FUNC_OUTPUT);
		//mdelay(10);
		break;
	case 1:
		gpioSet(ctrl->rst_data);
		break;	
	}
}

static struct resource pcie0_io_resource = {
   .name   = "PCIE0 IO",
   .flags  = IORESOURCE_IO,
   .start  = PADDR(BSP_PCIE0_D_IO),
   .end    = PADDR(BSP_PCIE0_D_IO + 0xFFFF)
};

static struct resource pcie0_mem_resource = {
   .name   = "PCIE0 MEM",
   .flags  = IORESOURCE_MEM,
   .start  = PADDR(BSP_PCIE0_D_MEM),
   .end    = PADDR(BSP_PCIE0_D_MEM + 0xFFFFFF)
};

static struct resource pcie1_io_resource = {
   .name   = "PCIE1 IO",
   .flags  = IORESOURCE_IO,
   .start  = PADDR(BSP_PCIE1_D_IO),
   .end    = PADDR(BSP_PCIE1_D_IO + 0xFFFF)
};

static struct resource pcie1_mem_resource = {
   .name   = "PCIE1 MEM",
   .flags  = IORESOURCE_MEM,
   .start  = PADDR(BSP_PCIE1_D_MEM),
   .end    = PADDR(BSP_PCIE1_D_MEM + 0xFFFFFF)
};

static int rtk_pcie_read(struct pci_bus *bus, unsigned int devfn, int where, int size, unsigned int *val) {
	struct rtk_pci_controller *ctrl = pci_bus_to_rtk_controller(bus);
	unsigned long flags;
	void __iomem *base;
	u32 data;
	
	if (ctrl->bus_addr==0xff)
		ctrl->bus_addr = bus->number;
	
	//printk("pcix-r: %p,%d.%d.%x %x,%x+%4x\n",bus,bus->number,bus->primary,devfn, size, base, where);
	
	if (bus->number!=ctrl->bus_addr)
		return PCIBIOS_DEVICE_NOT_FOUND;
	
	//if (PCI_FUNC(devfn))
	//	return PCIBIOS_DEVICE_NOT_FOUND;
	
	switch (PCI_SLOT(devfn)) {
	case 0:
		base = ctrl->hostcfg_base; break;
	case 1:
		base = ctrl->devcfg_base; break;
	default:
		return PCIBIOS_DEVICE_NOT_FOUND;
	}
	
	REG32(ctrl->hostext_base+0xC) = PCI_FUNC(devfn);
	
	spin_lock_irqsave(&ctrl->lock, flags);	
	switch (size) {
	case 1: data = REG8(base + where);	break;
	case 2:	data = REG16(base + where); break;
	case 4:	data = REG32(base + where);	break;
	default:
		spin_unlock_irqrestore(&ctrl->lock, flags);
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	spin_unlock_irqrestore(&ctrl->lock, flags);
			
	//printk("pcix-r: %p,%d.%d.%x %x,%x+%4x->%x\n",bus,bus->number,bus->primary,devfn, size, base, where, data);
	
	*val = data;
		
	return PCIBIOS_SUCCESSFUL;
}

static int rtk_pcie_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val) {
	struct rtk_pci_controller *ctrl = pci_bus_to_rtk_controller(bus);
	unsigned long flags;
	void __iomem *base;
	
	if (ctrl->bus_addr==0xff)
		ctrl->bus_addr = bus->number;
	
	if (bus->number!=ctrl->bus_addr)
		return PCIBIOS_DEVICE_NOT_FOUND;
	
	//if (PCI_FUNC(devfn))
	//	return PCIBIOS_DEVICE_NOT_FOUND;
	
	switch (PCI_SLOT(devfn)) {
	case 0:
		base = ctrl->hostcfg_base; break;
	case 1:
		base = ctrl->devcfg_base; break;
	default:
		return PCIBIOS_DEVICE_NOT_FOUND;
	}
	
	REG32(ctrl->hostext_base+0xC) = PCI_FUNC(devfn);
	
	//printk("pcix-w: %p,%d.%d.%x %x,%x+%4x<-%x\n",bus,bus->number,bus->primary,devfn, size, base, where, val);
	spin_lock_irqsave(&ctrl->lock, flags);
	switch (size) {
	case 1: REG8(base + where) = val; break;
	case 2: REG16(base + where) = val; break;
	case 4: REG32(base + where) = val; break;
	default:
		spin_unlock_irqrestore(&ctrl->lock, flags);
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	spin_unlock_irqrestore(&ctrl->lock, flags);
	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops pcie_ops = {
   .read = rtk_pcie_read,
   .write = rtk_pcie_write
};

static struct rtk_pci_controller pci0_controller = {
	.devcfg_base	= (void __iomem *)BSP_PCIE0_D_CFG0,
	.hostcfg_base	= (void __iomem *)BSP_PCIE0_H_CFG,
	.hostext_base	= (void __iomem *)BSP_PCIE0_H_EXT,
	.irq			= BSP_PCIE0_IRQ,
	.controller = { 
		.pci_ops        = &pcie_ops,
		.mem_resource   = &pcie0_mem_resource,
		.io_resource    = &pcie0_io_resource,
	},
	.rst_fn 		= rst_by_gpio,
	.port			= 0,
	.flags			= FL_MDIO_RESET,
	.bus_addr		= 0xff,
};

static struct rtk_pci_controller pci1_controller = {
	.devcfg_base	= (void __iomem *)BSP_PCIE1_D_CFG0,
	.hostcfg_base	= (void __iomem *)BSP_PCIE1_H_CFG,
	.hostext_base	= (void __iomem *)BSP_PCIE1_H_EXT,
	.irq			= BSP_PCIE1_IRQ,
	.controller = { 
		.pci_ops        = &pcie_ops,
		.mem_resource   = &pcie1_mem_resource,
		.io_resource    = &pcie1_io_resource,
	},
	.rst_fn 		= rst_by_gpio,
	.port			= 1,	
	.flags			= FL_MDIO_RESET,
	.bus_addr		= 0xff,
};

#define RETRY_COUNT 3
static int rtk_pcie_reset(struct rtk_pci_controller *p) {
	u32 tmp;
	int bit = (p->port) ? 21 : 24;
	struct pcie_para *phy;
	int retry = 0;
	
RETRY:	
	// 0. Assert PCIE Device Reset
	p->rst_fn(p, 0);
	
	// 1. PCIE phy mdio reset
	if (p->flags & FL_MDIO_RESET) {		
		REG32(BSP_PCI_MISC) &= ~(1 << bit);	
		mb();
		REG32(BSP_PCI_MISC) |= (1 << bit);
	}
	
	// 2. PCIE MAC reset
	tmp = (p->port) ? BSP_ENABLE_PCIE1 : BSP_ENABLE_PCIE0;
	REG32(BSP_IP_SEL) &= ~tmp;	
	mb();
	REG32(BSP_IP_SEL) |= tmp;
	
	if (p->flags & FL_MDIO_RESET) {		
		REG32(BSP_PCI_MISC) |= (1 << bit);
	}
	
	REG32(p->hostext_base+0x8) = 0x1; //bit7 PHY reset=0   bit0 Enable LTSSM=1	
	mb();
    REG32(p->hostext_base+0x8) = 0x81;   //bit7 PHY reset=1   bit0 Enable LTSSM=1
    mdelay(50);
	
	
	for (phy = p->phy_param; phy->port != 0xff; phy++) {		
		void __iomem *mdiobase = p->hostext_base+0x0;
		REG32(mdiobase) = 	((phy->value & 0xffff) << 16) |
							((phy->reg & 0xff) << 8) |	1;
		mdelay(1);
	}
	
	mdelay(20); //TPERST#-CLK min 100us
		
	// PCIE Device Reset
    p->rst_fn(p, 1);

    // wait for LinkUP	
	tmp = 10;
    while(--tmp)
    {
        if((REG32(p->hostcfg_base + 0x0728)&0x1f)==0x11)
            break;        
				
		mdelay(10);
    }
	
    if (tmp == 0) 
    {        			
		printk("Warning!! Port %d PCIE Link Failed, State=0x%x retry=%d\n", p->port, REG32(p->hostcfg_base + 0x0728), retry);
		
		if (retry < RETRY_COUNT) {			
			retry ++;
			goto RETRY;
		}
		
		return 0;
    }
    
	mdelay(100); // allow time for CR

	// 8. Set BAR	
	if (p->flags & FL_SET_BAR) {
		REG32(p->devcfg_base + 0x10) = p->port ? 0x18e00001 : 0x18c00001;
		REG32(p->devcfg_base + 0x18) = p->port ? 0x1a000004 : 0x19000004;
		REG32(p->devcfg_base + 0x04) = 0x00180007;
	}
	
    REG32(p->hostcfg_base+ 0x04) = 0x00100007;

    // Enable PCIE host
    REG32(p->hostcfg_base + 0x04) = 0x00100007;	
    REG8(p->hostcfg_base + 0x78) = (READ_MEM8(p->hostcfg_base + 0x78) & (~0xE0)) | 0;  // Set MAX_PAYLOAD_SIZE to 128B
	
	REG32(p->hostcfg_base + 0x80C) |= (1<<17);	
	do {
		u16 status;
		const char *str;
		mdelay(1);
		status = REG16(p->hostcfg_base + 0x82);
		
		switch (status & 0xf) {
		case 1: str = "2.5GHz"; break;
		case 2: str = "5.0Ghz"; break;
		default: str = "Unknown"; break;
		}
		printk("Port%d Link@%s\n", p->port, str);
	} while (0);
    	
	return 1;
}



int pcibios_plat_dev_init(struct pci_dev *dev) {
	printk("%s(%d)\n",__func__,__LINE__);
	printk("  devfn:%x vend:%x dev:%x cls:%x pin:%x\n",dev->devfn,dev->vendor,dev->device,dev->class,dev->pin);	
	return 0;
}

int pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	struct rtk_pci_controller *ctrl = pci_bus_to_rtk_controller(dev->bus);
		
	return ctrl->irq;	
}

#ifdef CONFIG_RTL9607C
static struct pcie_para pcie0_phy_params[] __initconst = {	
	{ 0, 0x00, 0x4008 },{ 0, 0x01, 0xa812 },{ 0, 0x02, 0x6042 },{ 0, 0x04, 0x5000 },
	{ 0, 0x05, 0x230a },{ 0, 0x06, 0x0011 },{ 0, 0x09, 0x520c },{ 0, 0x0a, 0xc670 },
	{ 0, 0x0b, 0xb905 },{ 0, 0x0d, 0xef16 },{ 0, 0x0e, 0x0000 },{ 0, 0x20, 0x9499 },
	{ 0, 0x21, 0x66aa },{ 0, 0x27, 0x011a },
	{ 0, 0x09, 0x500c },{ 0, 0x09, 0x520c },
	
	{ 0, 0x40, 0x4008 },{ 0, 0x41, 0xa811 },{ 0, 0x42, 0x6042 },{ 0, 0x44, 0x5000 },
	{ 0, 0x45, 0x230a },{ 0, 0x46, 0x0011 },{ 0, 0x4a, 0xc670 },{ 0, 0x4b, 0xb905 },
	{ 0, 0x4d, 0xef16 },{ 0, 0x4e, 0x0000 },{ 0, 0x4f, 0x000c },{ 0, 0x60, 0x94aa },
	{ 0, 0x61, 0x88ff },{ 0, 0x62, 0x0093 },{ 0, 0x67, 0x011a },{ 0, 0x6f, 0x65bd },
	{ 0, 0x49, 0x500c },{ 0, 0x49, 0x520c },
	#if 0 // GEN1
	{ 0, 0x00, 0x8a50 }, { 0, 0x02, 0x26f9 }, { 0, 0x03, 0x6bcd }, { 0, 0x06, 0x104a },
    { 0, 0x09, 0x6307 },
    { 0, 0x0b, 0x0009 }, { 0, 0x0c, 0x0800 }, { 0, 0x20, 0x0105 }, { 0, 0x21, 0x1000 },
	#endif 
    {0xff,0xff,0xffff}
};
static struct pcie_para pcie1_phy_params[] __initconst = {
	{ 1, 0x00, 0x8a50 }, { 1, 0x02, 0x26f9 }, { 1, 0x03, 0x6bcd }, { 1, 0x06, 0x1088 },
	{ 1, 0x09, 0x6303 },
	{ 1, 0x0b, 0x0009 }, { 1, 0x0c, 0x0800 }, { 1, 0x20, 0x0105 }, { 1, 0x21, 0x1000 },
    {0xff,0xff,0xffff}
};
#endif

static void __rtk_pci_controller_init(struct rtk_pci_controller *ctrl, struct pcie_para *phy, unsigned long rst_data) {
	spin_lock_init(&ctrl->lock);
	ctrl->phy_param = phy;
	ctrl->rst_data  = rst_data;
}

static int __init rtk_pcie_init(void) {
	struct rtk_pci_controller *p;
	
	printk("RTK-PCI Init...\n");	
	#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT0 
	p = &pci0_controller;
	__rtk_pci_controller_init(p, pcie0_phy_params, GPIO_19);	
	p->is_linked = rtk_pcie_reset(p);	
	printk(" Port%d link=%d\n",p->port,p->is_linked);
	if (p->is_linked)
		register_pci_controller(&(p->controller));
	#endif 
	
	#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT1
	p = &pci1_controller;
	__rtk_pci_controller_init(p, pcie1_phy_params, GPIO_18);
	p->is_linked = rtk_pcie_reset(p);
	printk(" Port%d link=%d\n",p->port,p->is_linked);
	if (p->is_linked)
	register_pci_controller(&p->controller);
	#endif 
	
	
	printk("RTK-PCI Done\n");

	return 0;
}

arch_initcall(rtk_pcie_init);
