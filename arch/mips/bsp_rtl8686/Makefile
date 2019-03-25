# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Oct. 18, 2008
#

#
# RTL8672
#
core-y   += arch/mips/bsp_rtl8686/
cflags-y += -Iinclude/asm-rlx -Iarch/mips/bsp_rtl8686/ -I$(APOLLODIR)/sdk/include/ -I$(APOLLODIR)/sdk/system/include 
load-y   += 0x80000000

extra-y  += vmlinux.lds
obj-y := prom.o setup.o irq.o timer.o serial.o promlib.o
obj-y += bspgpio.o led-generic.o pushbutton.o rtl_usb_phy.o rtl_gpio.o sys_capability.o uni_capability.o
obj-$(CONFIG_APOLLO_MP_TEST) += mptest.o
obj-$(CONFIG_LUNA_CMU) += luna_cmu.o
obj-$(CONFIG_LUNA_WDT) += luna_wdt.o
obj-$(CONFIG_LUNA_WDT_KTHREAD) += luna_watchdog.o
obj-$(CONFIG_LUNA_NBL_SWITCH) += luna_nbl_switch.o
ifeq ($(CONFIG_PRODUCT),luna) 
obj-y += board.o 
else
obj-$(CONFIG_RTL8686_BOARD_001) += board-001.o
obj-$(CONFIG_RTL8686_BOARD_002) += board-002.o
obj-$(CONFIG_RTL8686_BOARD_003) += board-003.o
obj-$(CONFIG_RTL8686_BOARD_004) += board-004.o
obj-$(CONFIG_RTL8686_BOARD_005) += board-005.o
endif
obj-$(CONFIG_SERIAL_8250) += 8250-platform.o
obj-$(CONFIG_GPIO_LED_TR068) += led-tr068.o
obj-$(CONFIG_WPS_LED) += led-wps.o
obj-$(CONFIG_WIFI_LED_USE_SOC_GPIO) += led-wifi.o
obj-y += watchdog.o
#obj-$(CONFIG_PCI) += pci.o
obj-$(CONFIG_PCI) += pci-rtl86xx.o ops-rtl86xx.o
obj-$(CONFIG_RTL867X_NETLOG) += netlog.o
obj-$(CONFIG_E8B) += led-power.o

# 2010-11-19 paula
ifeq ($(CONFIG_RTL8672_SW_USB_LED), y)
obj-y    += led-usb.o
endif
obj-$(CONFIG_USE_PRELOADER_PARAMETERS) += kernel_soc_parameters.o
obj-$(CONFIG_MTD_LUNA_NOR_SPI) += kernel_soc_parameters.o

obj-$(CONFIG_RTK_CP3_PERF) += cp3perf.o

ifeq ($(CONFIG_LUNA_USE_SRAM), y)
obj-y    += sram_mapping.o
endif
ifeq ($(CONFIG_LUNA_CPU_OPTION_SWITCH), y)
obj-y    += luna_cpuoptions_sw.o
endif
obj-$(CONFIG_RGMII_RESET_PROCESS) 	+= rgmii_rst.o

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


EXTRA_AFLAGS := $(CFLAGS)
EXTRA_CFLAGS := -Idrivers/mtd/chips/luna_norsf_g2/nor_spi
#EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/rtl819x/AsicDriver
#EXTRA_CFLAGS += -I$(DIR_LINUX)/drivers/net/rtl819x/common
EXTRA_CFLAGS += -Iinclude/soc

#krammer
ARCH_VMLINUX  := arch/$(ARCH)/bsp_rtl8686/vmlinux.lds

