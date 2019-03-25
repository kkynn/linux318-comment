# Realtek Semiconductor Corp.
#
# bsp/Makefile
#     bsp Makefile
#
# Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
#

CPPFLAGS_vmlinux.lds := $(KBUILD_CFLAGS)

#
# Sheipa
#
core-y   += arch/mips/bsp_rtl9607c/
cflags-y += -DAPOLLOPRO
cflags-y += -Iinclude/asm-mips \
	-Iarch/mips/bsp_rtl9607c/ \
	-I$(APOLLODIR)/sdk/include/ \
	-I$(APOLLODIR)/sdk/system/include

ifdef CONFIG_MIPS_CMP
load-y   += 0x80010000
else
load-y   += 0x80000000
endif
# 
extra-y  += vmlinux.lds
obj-y := prom.o setup.o irq.o timer.o serial.o
obj-y += board.o
# Just for test request. Not necessary!
obj-y += bsp_dsp_test.o
obj-y += bsp_dw.o
obj-y += bspgpio.o
obj-y += led-generic.o
obj-y += promlib.o
#obj-y += serial.o
obj-y += pushbutton.o 
obj-y += rtl_gpio.o 
obj-y += sys_capability.o
obj-$(CONFIG_CPU_HAS_WB) += wb_flush.o
obj-y += uni_capability.o

#Because 1004k/IA CPS has L2Cache unit, so we must handle it whether use it or not.
obj-y +=init_l2cache.o 
obj-$(CONFIG_PCI) += rtk-pci.o 
obj-$(CONFIG_USB) += rtl_usb_phy.o
obj-$(CONFIG_SMP) += smp.o
obj-$(CONFIG_SOC_MIPS_CMP_BOOT) += cmp_init.o cmp.o
obj-$(CONFIG_CEVT_EXT) += cevt-ext.o
obj-$(CONFIG_POLICY_MANAGER) += policy-manager.o
obj-$(CONFIG_EARLY_PRINTK) += early-printk.o
#obj-$(CONFIG_LUNA_WDT) += luna_wdt.o
obj-$(CONFIG_LUNA_WDT_KTHREAD) += luna_watchdog.o
#obj-y += watchdog.o
obj-$(CONFIG_SERIAL_8250) += 8250-platform.o
ifeq ($(CONFIG_RTL8672_SW_USB_LED), y)
obj-y    += led-usb.o
endif
obj-$(CONFIG_WPS_LED) += led-wps.o
obj-$(CONFIG_WIFI_LED_USE_SOC_GPIO) += led-wifi.o
obj-$(CONFIG_E8B) += led-power.o
obj-$(CONFIG_APOLLO_MP_TEST) += mptest.o

#depend on CONFIG_YUEME
ifeq ($(CONFIG_PRD_SIGNATURE),9607RG)
obj-$(CONFIG_YUEME_DMIPS_FEATURE) +=  bsp_cpu_clock.o
endif
ifeq ($(CONFIG_PRD_SIGNATURE),DL9607RG)
obj-$(CONFIG_YUEME_DMIPS_FEATURE) +=  bsp_cpu_clock.o
endif
ifeq ($(CONFIG_YUEME),y)
obj-$(CONFIG_YUEME_TMPFS_FEATURE) +=  bsp_yueme_tmpfs.o
obj-$(CONFIG_YUEME_APP_FORK_LIMIT_FEATURE) +=  bsp_yueme_fork_limit.o
endif
