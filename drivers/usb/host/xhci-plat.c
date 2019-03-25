/*
 * xhci-plat.c - xHCI host controller driver platform Bus Glue.
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com
 * Author: Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * A lot of code borrowed from the Linux xHCI driver.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/usb/xhci_pdriver.h>

#include "xhci.h"
#include "xhci-mvebu.h"
#include "xhci-rcar.h"

#define RTL8686_USB3_PATCH 1

#ifdef RTL8686_USB3_PATCH
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "bspchip.h"
#define readl(x)                cpu_to_le32((*(volatile unsigned int *)(x)))
#define writel(val, addr)         (void)((*(volatile unsigned int *) (addr)) = (cpu_to_le32(val)))
static inline unsigned int xhci_readl(const struct xhci_hcd *xhci,
                __u32 __iomem *regs)
{
        return readl(regs);
}
static inline void xhci_writel(struct xhci_hcd *xhci,
                const unsigned int val, __u32 __iomem *regs)
{
        writel(val, regs);
}



extern void set_usb2phy_xhci(void);
static const char hcd_name[] = "rtl86xx-xhci";

static void get_mdio_phy(unsigned int addr,unsigned int *value);
static void set_mdio_phy(unsigned int addr,unsigned int value);
static u32 read_reg(unsigned char *addr);
static void write_reg(unsigned char *addr,u32 value);
static void U3_PhyReset(int Reset_n);

static int u3_reg_read(struct seq_file *seq, void *v)
{
	return 0;
}
static int u3_reg_write(struct file *file, const char __user *buff, size_t len, loff_t *ppos)
{
	char 		tmpbuf[64];
	unsigned int	mem_addr, mem_data, mem_len;
	char		*strptr, *cmd_addr;
	char		*tokptr;

	if (buff && !copy_from_user(tmpbuf, buff, len)) {
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
		{
			goto errout;
		}

		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}

		if (!memcmp(cmd_addr, "r", 1))
		{
			mem_addr=simple_strtol(tokptr, NULL, 0);


            if(mem_addr==0)
            {
                printk("read reg :0x%x \n", mem_addr);
                goto errout;
            }
            mem_len=read_reg((unsigned char *)mem_addr);

            
		}
		else if (!memcmp(cmd_addr, "w", 1))
		{
			mem_addr=simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_data=simple_strtol(tokptr, NULL, 0);

            write_reg((unsigned char *)mem_addr, mem_data);
            mem_len=read_reg((unsigned char *)mem_addr);

		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("Memory operation only support \"r\" and \"w\" as the first parameter\n");
		printk("Read format:	\"r mem_addr length\"\n");
		printk("Write format:	\"w mem_addr mem_data\"\n");
	}

	return len;
}


static int u3_mdio_read(struct seq_file *seq, void *v)
{
	return 0;
}
static int u3_mdio_write(struct file *file, const char __user *buff, size_t len, loff_t *ppos)
{
	char 		tmpbuf[64];
	unsigned int	mem_addr, mem_data;
	char		*strptr, *cmd_addr;
	char		*tokptr;

	if (buff && !copy_from_user(tmpbuf, buff, len)) {
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
		{
			goto errout;
		}
		
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
            int i;
            for(i=0;i<0x30;i++)
			{
                get_mdio_phy(i,&mem_data);
			    printk("read mdio addr:0x%x val:0x%x\n",i, mem_data);
			}
	        return len;
		}

		if (!memcmp(cmd_addr, "r", 1))
		{
			mem_addr=simple_strtol(tokptr, NULL, 0);

            get_mdio_phy(mem_addr,&mem_data);
			printk("read mdio addr:0x%x val:0x%x\n", mem_addr, mem_data);
		}
		else if (!memcmp(cmd_addr, "w", 1))
		{
			mem_addr=simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_data=simple_strtol(tokptr, NULL, 0);      
            set_mdio_phy(mem_addr,mem_data);
			printk("Write mdio addr:0x%x val:0x%x\n", mem_addr, mem_data);         
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("mido operation only support \"r\" and \"w\" as the first parameter\n");
		printk("Read format:	\"r addr\"\n");
		printk("Write format:	\"w addr val\"\n");
	}

	return len;
}

static int mdio_open(struct inode *inode, struct file *file)
{
        return single_open(file, u3_mdio_read, inode->i_private);
}
static const struct file_operations mdio_fops = {
        .owner          = THIS_MODULE,
        .open           = mdio_open,
        .read           = seq_read,
        .write          = u3_mdio_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int reg_open(struct inode *inode, struct file *file)
{
        return single_open(file, u3_reg_read, inode->i_private);
}
static const struct file_operations reg_fops = {
        .owner          = THIS_MODULE,
        .open           = reg_open,
        .read           = seq_read,
        .write          = u3_reg_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

#define U3DBG_PROC_DIR_NAME "U3DBG"
static struct proc_dir_entry *u3_dbg_proc_dir_t=NULL;

static void rtl86xx_proc_debug_init(void){
	if(u3_dbg_proc_dir_t==NULL)
		u3_dbg_proc_dir_t = proc_mkdir(U3DBG_PROC_DIR_NAME,NULL);
	if(u3_dbg_proc_dir_t)
	{
		proc_create_data("mdio", 0, u3_dbg_proc_dir_t,&mdio_fops,NULL);
		proc_create_data("reg", 0, u3_dbg_proc_dir_t,&reg_fops,NULL);
	}
}

#define USB2_PHY_DELAY { mdelay(10); }

static void set_mdio_phy(unsigned int addr,unsigned int value)
{
	int readback;

	REG32(0xb8140000)=(addr<<8);
	USB2_PHY_DELAY;
	readback=REG32(0xb8140000);
	USB2_PHY_DELAY;
	REG32(0xb8140000)=(value<<16)|(addr<<8)|1;
	USB2_PHY_DELAY;

	readback=REG32(0xb8140000);
	USB2_PHY_DELAY;
	REG32(0xb8140000)=(addr<<8);
	USB2_PHY_DELAY;
	readback=REG32(0xb8140000);
	USB2_PHY_DELAY;
	
	if(readback!=((value<<16)|(addr<<8)|0x10)) { printk("usb phy set error (addr=%x, value=%x read=%x)\n",addr,value,readback); }
}

static void get_mdio_phy(unsigned int addr,unsigned int *value)
{
	int readback;
	int readback2;
	REG32(0xb8140000)=(addr<<8);
	USB2_PHY_DELAY;
	readback=REG32(0xb8140000);
	USB2_PHY_DELAY;
	////readback=REG32(0xb8140000);
	USB2_PHY_DELAY;
	REG32(0xb8140000)=(addr<<8);
	USB2_PHY_DELAY;
	readback2=REG32(0xb8140000);
	USB2_PHY_DELAY;
    *value=((readback2>>16)&0xFFFF);
}

static void U3_PhyReset(int Reset_n) 
{
	#define U3_IPCFG 0xb8140008
	//U3_PhyReset
	REG32(U3_IPCFG) |=   (1<<9);  //control by sys reg.

	if(Reset_n==0)     REG32(U3_IPCFG) &=~(1<<10);
	else				    REG32(U3_IPCFG) |=   (1<<10);	
  
	mdelay(100);
	
}


#define NEW_PHY_PARAM 1

static void set_usbphy_rle0371(void) //just for rle0371
{


//=== Start of 2013/10/31, Got new U3 phy parameter for RLE0371 from RD Center  ===== 
		//REG32(0xb8000600)|=0xc000201c; //0xc000ffff

		printk("[%s] read_reg(0xb8000600)=0x%x\n",__func__, read_reg(0xb8000600));
		printk("Start of setting U3 Phy parameters\n");
		set_mdio_phy(0x00,0x1218); //0x1278->0x1218, Recommand by RD-Center, 2013/11/22
		set_mdio_phy(0x01,0x0003);
		set_mdio_phy(0x02,0x2D18);
		set_mdio_phy(0x03,0x6D70);
		set_mdio_phy(0x04,0x5000);  //0x04, should disable first
		set_mdio_phy(0x05,0x0304);
		set_mdio_phy(0x06,0xB054);
		set_mdio_phy(0x07,0x4CC1);
		set_mdio_phy(0x08,0x31D2);
		set_mdio_phy(0x09,0x923C);
		set_mdio_phy(0x0A,0x9240);
		set_mdio_phy(0x0B,0xC51D);
		set_mdio_phy(0x0C,0x68AB);
		set_mdio_phy(0x0D,0x27A6);
		set_mdio_phy(0x0E,0x9B01);
		set_mdio_phy(0x0F,0x051A);
		set_mdio_phy(0x10,0x000C);
		set_mdio_phy(0x11,0x4C00);
		set_mdio_phy(0x12,0xFC00);
		set_mdio_phy(0x13,0x0C81);
		set_mdio_phy(0x14,0xDE01);
		
		set_mdio_phy(0x19,0xA000);
		set_mdio_phy(0x1A,0x6DE1);
		set_mdio_phy(0x1B,0xA027);
		set_mdio_phy(0x1C,0xC300);
		set_mdio_phy(0x1D,0xA03E);
		set_mdio_phy(0x1E,0xC2A0);
		
		set_mdio_phy(0x20,0xE39F);
		set_mdio_phy(0x21,0xD51B);
		set_mdio_phy(0x22,0x0836);
		set_mdio_phy(0x23,0x4FA2);
		set_mdio_phy(0x24,0x13F1);
		set_mdio_phy(0x25,0x03DD);
		set_mdio_phy(0x26,0x64CA);
		set_mdio_phy(0x27,0x00F9);
		set_mdio_phy(0x28,0xF8B0); //0x48b0->0xf8b0 , Recommand by RD-Center, 2013/11/22
		set_mdio_phy(0x2A,0x3080);
		set_mdio_phy(0x2B,0x2018);

		set_mdio_phy(0x04,0x7000);  //0x04, then enable 
		
		set_mdio_phy(0x0A,0x9A40);
		set_mdio_phy(0x0A,0x9A44);
		set_mdio_phy(0x0A,0x9240);
		set_mdio_phy(0x09,0x923C);
		set_mdio_phy(0x09,0x903C);
		set_mdio_phy(0x09,0x923C);

		printk("End of setting U3 Phy parameters\n");
		//REG32(0xb8000600)|=0xC000201C; //Enable bit 30 , after set phy paramteters
//=== End of 2013/10/31, Got new U3 phy parameter for RLE0371 from RD Center  ===== 

//	REG32(0xb8000600)=0x20a7ffff;
//	REG32(0xb8000600)=0x2007ffff; //bit 20~23 -> debug signal
//	REG32(0xb8000600)=0x2017ffff; //bit 20~23 -> debug signal
	printk("######### 0xb8000600=%x ###########\n",REG32(0xb8000600));  //debug port change

	//REG32(0xb8140008)= 0x0040006d;  //90
	REG32(0xb8140008)= 0x0080006d;  //180
	//REG32(0xb8140008)= 0x00c0006d;  //270
	mdelay(100);

//	write_reg(0xb804c2c0,read_reg(0xb804c2c0)|0x40000);  //for U2 debug 2011/3/15
//	mdelay(100);
//debug
//	REG32(0xb804c2c0)|= 0x80;
//	mdelay(100);	
//	REG32(0xb804c2c0)&= 0xffffff7f;
//	mdelay(100);		

	U3_PhyReset(0);
	U3_PhyReset(1);

	//Now config U2 Phy
	printk("Now config U2 PHY in XHCI\n");
	set_usb2phy_xhci();
}

static u32 read_reg(unsigned char *addr)
{
	volatile u32 tmp;
	u32 addv=(u32)addr;
	tmp=REG32(addv);
	printk("read_reg(0x%x)=0x%x\n",(u32)addr,cpu_to_le32(tmp));
	return cpu_to_le32(tmp);
}

static void write_reg(unsigned char *addr,u32 value)
{
	u32 addv=(u32)addr;
	REG32(addv)=cpu_to_le32(value);
	printk("write_reg(0x%x)=0x%x\n",(u32)addr,value);
}
#endif // RTL8686_USB3_PATCH

static void xhci_plat_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	/*
	 * As of now platform drivers don't provide MSI support so we ensure
	 * here that the generic code does not try to make a pci_dev from our
	 * dev struct in order to setup MSI
	 */
	xhci->quirks |= XHCI_PLAT;
}

/* called during probe() after chip reset completes */
static int xhci_plat_setup(struct usb_hcd *hcd)
{
	struct device_node *of_node = hcd->self.controller->of_node;
	int ret;

	if (of_device_is_compatible(of_node, "renesas,xhci-r8a7790") ||
	    of_device_is_compatible(of_node, "renesas,xhci-r8a7791")) {
		ret = xhci_rcar_init_quirk(hcd);
		if (ret)
			return ret;
	}

	return xhci_gen_setup(hcd, xhci_plat_quirks);
}

#ifdef CONFIG_RTL9607C
static struct hc_driver __read_mostly xhci_plat_hc_driver;
#else //CONFIG_RTL9607C
#ifdef RTL8686_USB3_PATCH

static struct hc_driver xhci_plat_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"Realtek rtl86xx On-Chip XHCI Host Controller",
	.hcd_priv_size =	sizeof(struct xhci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			xhci_irq,
	.flags =		HCD_MEMORY | HCD_USB3,

	/*
	 * basic lifecycle operations
	 */
	.reset =		xhci_plat_setup,
	.start =		xhci_run,
	/* suspend and resume implemented later */
	.stop =			xhci_stop,
	.shutdown =		xhci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		xhci_urb_enqueue,
	.urb_dequeue =		xhci_urb_dequeue,
	.alloc_dev =		xhci_alloc_dev,
	.free_dev =		xhci_free_dev,
	.add_endpoint =		xhci_add_endpoint,
	.drop_endpoint =	xhci_drop_endpoint,
	.endpoint_reset =	xhci_endpoint_reset,
	.check_bandwidth =	xhci_check_bandwidth,
	.reset_bandwidth =	xhci_reset_bandwidth,
	.address_device =	xhci_address_device,
	.update_hub_device =	xhci_update_hub_device,
	.reset_device =         xhci_discover_or_reset_device,

	#ifdef CONFIG_PM
	.bus_suspend	= xhci_bus_suspend,
	.bus_resume	= xhci_bus_resume,
	#endif
	/*
	 * scheduling support
	 */
	.get_frame_number =	xhci_get_frame,

	/* Root hub support */
	.hub_control =		xhci_hub_control,
	.hub_status_data =	xhci_hub_status_data,
};

#define PADDR(addr)  ((addr) & 0x1FFFFFFF)
#define RTL86XX_XHCI_BASE	0xb8040000
static struct resource bsp_usb3_resource[] = {
        [0] = {
                .start = PADDR(RTL86XX_XHCI_BASE),
                .end = PADDR(RTL86XX_XHCI_BASE) +0x0000EFFF,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = BSP_USB_H3_IRQ,
                .end = BSP_USB_H3_IRQ,
                .flags = IORESOURCE_IRQ,
        }
};

static u64 bsp_usb_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_usb3_device = {
        .name = "rtl86xx-xhci",
        .id = -1,
        .num_resources = ARRAY_SIZE(bsp_usb3_resource),
        .resource = bsp_usb3_resource,
        .dev = {
                .dma_mask = &bsp_usb_dmamask,
                .coherent_dma_mask = 0xffffffffUL
        }
};

static struct platform_device *bsp_usb_devs[] __initdata = {
        &bsp_usb3_device,
};


#endif //RTL8686_USB3_PATCH
#endif //CONFIG_RTL9607C

static int xhci_plat_start(struct usb_hcd *hcd)
{
	struct device_node *of_node = hcd->self.controller->of_node;

	if (of_device_is_compatible(of_node, "renesas,xhci-r8a7790") ||
	    of_device_is_compatible(of_node, "renesas,xhci-r8a7791"))
		xhci_rcar_start(hcd);

	return xhci_run(hcd);
}

static int xhci_plat_probe(struct platform_device *pdev)
{
	struct device_node	*node = pdev->dev.of_node;
	struct usb_xhci_pdata	*pdata = dev_get_platdata(&pdev->dev);
	const struct hc_driver	*driver;
	struct xhci_hcd		*xhci;
	struct resource         *res;
	struct usb_hcd		*hcd;
	struct clk              *clk;
	int			ret;
	int			irq;
	
#ifdef RTL8686_USB3_PATCH
	#ifndef CONFIG_RTL9607C
	rtl86xx_proc_debug_init();
	set_usbphy_rle0371();
	#endif // CONFIG_RTL9607C
#endif //RTL8686_USB3_PATCH

	if (usb_disabled())
		return -ENODEV;

	driver = &xhci_plat_hc_driver;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	/* Initialize dma_mask and coherent_dma_mask to 32-bits */
	ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	else
		dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));

	hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);

	hcd->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hcd->regs)) {
		ret = PTR_ERR(hcd->regs);
		goto put_hcd;
	}

	/*
	 * Not all platforms have a clk so it is not an error if the
	 * clock does not exists.
	 */
	clk = devm_clk_get(&pdev->dev, NULL);
	if (!IS_ERR(clk)) {
		ret = clk_prepare_enable(clk);
		if (ret)
			goto put_hcd;
	}

	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-375-xhci") ||
	    of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-380-xhci")) {
		ret = xhci_mvebu_mbus_init_quirk(pdev);
		if (ret)
			goto disable_clk;
	}

	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto disable_clk;

	device_wakeup_enable(hcd->self.controller);

	/* USB 2.0 roothub is stored in the platform_device now. */
	hcd = platform_get_drvdata(pdev);
	xhci = hcd_to_xhci(hcd);
	xhci->clk = clk;
	xhci->shared_hcd = usb_create_shared_hcd(driver, &pdev->dev,
			dev_name(&pdev->dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto dealloc_usb2_hcd;
	}

	if ((node && of_property_read_bool(node, "usb3-lpm-capable")) ||
			(pdata && pdata->usb3_lpm_capable))
		xhci->quirks |= XHCI_LPM_SUPPORT;
	/*
	 * Set the xHCI pointer before xhci_plat_setup() (aka hcd_driver.reset)
	 * is called by usb_add_hcd().
	 */
	*((struct xhci_hcd **) xhci->shared_hcd->hcd_priv) = xhci;

	if (HCC_MAX_PSA(xhci->hcc_params) >= 4)
		xhci->shared_hcd->can_do_streams = 1;

	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
		goto put_usb3_hcd;

	return 0;

put_usb3_hcd:
	usb_put_hcd(xhci->shared_hcd);

dealloc_usb2_hcd:
	usb_remove_hcd(hcd);

disable_clk:
	if (!IS_ERR(clk))
		clk_disable_unprepare(clk);

put_hcd:
	usb_put_hcd(hcd);

	return ret;
}

static int xhci_plat_remove(struct platform_device *dev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	struct clk *clk = xhci->clk;

	usb_remove_hcd(xhci->shared_hcd);
	usb_put_hcd(xhci->shared_hcd);

	usb_remove_hcd(hcd);
	if (!IS_ERR(clk))
		clk_disable_unprepare(clk);
	usb_put_hcd(hcd);
	kfree(xhci);

	return 0;
}

#ifdef CONFIG_PM
static int xhci_plat_suspend(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	/*
	 * xhci_suspend() needs `do_wakeup` to know whether host is allowed
	 * to do wakeup during suspend. Since xhci_plat_suspend is currently
	 * only designed for system suspend, device_may_wakeup() is enough
	 * to dertermine whether host is allowed to do wakeup. Need to
	 * reconsider this when xhci_plat_suspend enlarges its scope, e.g.,
	 * also applies to runtime suspend.
	 */
	return xhci_suspend(xhci, device_may_wakeup(dev));
}

static int xhci_plat_resume(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	return xhci_resume(xhci, 0);
}

static const struct dev_pm_ops xhci_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_plat_suspend, xhci_plat_resume)
};
#define DEV_PM_OPS	(&xhci_plat_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_OF
static const struct of_device_id usb_xhci_of_match[] = {
	{ .compatible = "generic-xhci" },
	{ .compatible = "xhci-platform" },
	{ .compatible = "marvell,armada-375-xhci"},
	{ .compatible = "marvell,armada-380-xhci"},
	{ .compatible = "renesas,xhci-r8a7790"},
	{ .compatible = "renesas,xhci-r8a7791"},
	{ },
};
MODULE_DEVICE_TABLE(of, usb_xhci_of_match);
#endif

static struct platform_driver usb_xhci_driver = {
	.probe	= xhci_plat_probe,
	.remove	= xhci_plat_remove,
	.driver	= {
		.name = "rtl86xx-xhci",
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(usb_xhci_of_match),
	},
};
MODULE_ALIAS("platform:rtl86xx-xhci");

static int __init xhci_plat_init(void)
{
	#ifndef CONFIG_RTL9607C
	#ifdef RTL8686_USB3_PATCH
	int ret = 0;
	ret = platform_add_devices(bsp_usb_devs, ARRAY_SIZE(bsp_usb_devs));
	if (ret < 0) {
		printk("ERROR: unable to add devices\n");
		return ret;
	}
	#endif
	#endif
	xhci_init_driver(&xhci_plat_hc_driver, xhci_plat_setup);
	xhci_plat_hc_driver.start = xhci_plat_start; 
	return platform_driver_register(&usb_xhci_driver);
}
module_init(xhci_plat_init);

static void __exit xhci_plat_exit(void)
{
	platform_driver_unregister(&usb_xhci_driver);
}
module_exit(xhci_plat_exit);

MODULE_DESCRIPTION("xHCI Platform Host Controller Driver");
MODULE_LICENSE("GPL");
