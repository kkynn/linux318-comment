//#define DEBUG 1

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/dmapool.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include "ehci.h"
#include "xhci.h"

#define HOST_EHCI 1
#define HOST_XHCI 2 


static int __usb_host_type(struct usb_hcd *hcd) {
	if (!strncmp(hcd->driver->description, "xhci", 4)) {
		return HOST_XHCI;
	} if (!strncmp(hcd->driver->description, "ehci", 4)) {
		return HOST_EHCI;
	}
	return 0;
}

int rtk_usb_test_mode_set(struct usb_hcd *hcd, u8 mode) {
	u32 temp, *addr, retry, port=0;
	if (mode>4)
		return -EINVAL;
	if (hcd->speed != HCD_USB2) {
		printk("%s:not USB2\n", hcd->driver->description);
		return -EINVAL;
	}
	if (__usb_host_type(hcd)==HOST_XHCI) { //!strncmp(hcd->driver->description, "xhci", 4)) {
		#ifdef CONFIG_USB_XHCI_HCD
		int port_index;
		__le32 __iomem **port_array;
		__le32 __iomem	*pm_addr;
		
		struct xhci_hcd *xhci = hcd_to_xhci(hcd);
		//printk("xhci-usb2_ports=%p op_reg=%p\n",xhci->usb2_ports[0],xhci->op_regs);
		
		/* 4.19.6 */
		port_index = xhci->num_usb3_ports;
		port_array = xhci->usb3_ports;
		while(port_index--) {			
			temp = readl(port_array[port_index]) & ~(1<<9);
			//printk("%s(%d): port%d(%p) = %x\n",__func__,__LINE__,port_index,port_array[port_index],temp);
			writel(temp, port_array[port_index]);
		}
		
		port_index = xhci->num_usb2_ports;
		port_array = xhci->usb2_ports;
		while(port_index--) {
			temp = readl(port_array[port_index]) & ~(1<<9);
			//printk("%s(%d): port%d(%p) = %x\n",__func__,__LINE__,port_index,port_array[port_index],temp);
			writel(temp, port_array[port_index]);
		}
		
		temp = readl(&xhci->op_regs->command);
		temp &= ~CMD_RUN;
		writel(temp, &xhci->op_regs->command);
		
		retry=5;
		while (retry-- && (!(readl(&xhci->op_regs->status)&XHCI_STS_HALT))) {
			//printk("USBSTS=%x\n",readl(&xhci->op_regs->status));
			mdelay(100);
		}
		
		port_array = xhci->usb2_ports;
		pm_addr = port_array[port] + PORTPMSC;
		printk("%s:Set Mode=%d\n", hcd->driver->description, mode);
		writel(mode << 28, pm_addr);
		
		return 0;
		#endif
	} else if (__usb_host_type(hcd)==HOST_EHCI) { //if (!strncmp(hcd->driver->description, "ehci", 4)) {
		#ifdef CONFIG_USB_EHCI_HCD
		struct ehci_hcd	*ehci = hcd_to_ehci(hcd);
		//printk("ehci->regs->cmd=%p\n",&ehci->regs->command);
		printk("%s:Set Mode=%d\n", hcd->driver->description, mode);
		if (mode) {			
			pr_debug("set the Run/Stop(R/S) bit in the USBCMD register to a '0'\n");
			temp = ehci_readl(ehci, &ehci->regs->command);
			temp &= ~CMD_RUN;
			ehci_writel(ehci, temp, &ehci->regs->command);
			pr_debug("wait for HCHalted(HCH) bit in the USBSTS register to transition to a '1'\n");
			retry=5;
			do {
				msleep(100);
				temp = ehci_readl(ehci, &ehci->regs->status);
			} while (retry-- && ((temp & STS_HALT)!=STS_HALT));
			pr_debug("set test mode %d to port %d ...\n", mode, port);
			addr = &ehci->regs->port_status[port];
			temp = ehci_readl(ehci, addr);
			temp = (temp & ~(0xf << 16)) | (mode << 16);
			ehci_writel(ehci, temp, addr);
			if (mode==5) {
				pr_debug("set the Run/Stop(R/S) bit in the USBCMD register to a '1'\n");
				temp = ehci_readl(ehci, &ehci->regs->command);
				temp |= CMD_RUN;
				ehci_writel(ehci, temp, &ehci->regs->command);
			}			
		} else {			
			pr_debug("set the Run/Stop(R/S) bit in the USBCMD register to a '0'\n");
			temp = ehci_readl(ehci, &ehci->regs->command);
			temp &= ~CMD_RUN;
			ehci_writel(ehci, temp, &ehci->regs->command);
			
			pr_debug("wait for HCHalted(HCH) bit in the USBSTS register to transition to a '1'\n");
			retry=5;
			do {
				msleep(100);
				temp = ehci_readl(ehci, &ehci->regs->status);
			} while (retry-- && ((temp & STS_HALT)!=STS_HALT));
			
			pr_debug("set the Host Controller Reset(HCRST) bit in the USBCMD register to a '1'\n");
			temp = ehci_readl(ehci, &ehci->regs->command);
			temp |= CMD_RESET;
			ehci_writel(ehci, temp, &ehci->regs->command);
			
			pr_debug("Leave test mode , OK !!!\n");
		}
		
		return 0;
		#endif
	} 
	return -EINVAL;
}

int rtk_usb_test_mode_get(struct usb_hcd *hcd, u8 *mode) {
	u32 temp, *addr, port=0;
	
	if (hcd->speed != HCD_USB2)
		return -EINVAL;
	if (__usb_host_type(hcd)==HOST_XHCI) { //!strncmp(hcd->driver->description, "xhci", 4)) {
		#ifdef CONFIG_USB_XHCI_HCD
		int port_index;
		__le32 __iomem **port_array;
		__le32 __iomem	*pm_addr;		
		struct xhci_hcd *xhci = hcd_to_xhci(hcd);
		
		port_array = xhci->usb2_ports;		
		pm_addr = port_array[port] + PORTPMSC;		
		temp = readl(pm_addr);
		printk("xhci->usb2_ports=%p pm_addr=%lx\n",port_array,le32_to_cpu(pm_addr));
		*mode = temp >> 28;		
		return 0;
		#endif
	} else if (__usb_host_type(hcd)==HOST_EHCI) { //!strncmp(hcd->driver->description, "ehci", 4)) {
		#ifdef CONFIG_USB_EHCI_HCD
		struct ehci_hcd	*ehci = hcd_to_ehci(hcd);		
		addr = &ehci->regs->port_status[port];		
		temp = ehci_readl(ehci, addr);
		printk("ehci->regs->cmd=%p addr=%lx,%lx\n",&ehci->regs->command, addr, temp);
		*mode = (temp >> 16) & 0xf;		
		return 0;
		#endif
	} 
	return -EINVAL;
}
