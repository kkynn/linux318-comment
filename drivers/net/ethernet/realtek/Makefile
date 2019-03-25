#
# Makefile for the Realtek network device drivers.
#

obj-$(CONFIG_8139CP) += 8139cp.o
obj-$(CONFIG_8139TOO) += 8139too.o
obj-$(CONFIG_ATP) += atp.o
obj-$(CONFIG_R8169) += r8169.o
obj-$(CONFIG_R8168) += r8168/
obj-$(CONFIG_RTL8672_BRIDGE_FASTPATH) += brg_shortcut.o
obj-$(CONFIG_RTL865X_ETH_PRIV_SKB) += re_privskb.o re_privskb_rg.o
ifeq ($(CONFIG_RTK_WLAN_EVENT_INDICATE),y)
ifeq ($(CONFIG_YUEME),y)
obj-y += rtk_wlan_event_netlink.o
else
obj-y += rtk_wlan_event.o
endif
endif

obj-$(CONFIG_RTL8686NIC) += re8686_ext.o
# for RTL9600 series
ifeq ($(CONFIG_RTL8686NIC),y)
# for RTL9600 series
ifeq ($(CONFIG_RTL9600_SERIES),y)
ifeq ($(CONFIG_ARCH_CPU_RLX5281),y)
	obj-$(CONFIG_RTL8686NIC) += re8686.o
ifeq ($(CONFIG_RTL8686_TEST),y)
	obj-m += re8686_test.o
endif #end for re8686_test.o
else#else for re8686.o or re8686_dsp.o
ifeq ($(CONFIG_ARCH_CPU_RLX5181),y)
	obj-y += re8686_dsp.o
endif#end for re8686_dsp.o
endif#end for rlx 5281
endif#end for RTL9600 series

CFLAGS_re8686.o = -Werror
ifeq ($(CONFIG_RTL8686_SWITCH),y)
CFLAGS_re8686.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/include
CFLAGS_re8686.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/system/include
CFLAGS_re8686.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/include/hal/chipdef/apollo
CFLAGS_re8686.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/romeDriver
CFLAGS_re8686.o += -I${shell pwd}/drivers/net
CFLAGS_re8686.o += -DCONFIG_SDK_KERNEL_LINUX -D__KERNEL__
endif

#put here if other chip (9601b/9602c)

endif#end for CONFIG_RTL8686NIC

# for RTL9602B-VB series
ifeq ($(CONFIG_RTL9602C_SERIES),y)
ifeq ($(CONFIG_APOLLO_FPGA_PHY_TEST),y)
obj-y += re8686.o
CFLAGS_re8686.o = -Werror
ifeq ($(CONFIG_RTL8686_SWITCH),y)
CFLAGS_re8686.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/include
CFLAGS_re8686.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/system/include
CFLAGS_re8686.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/include/hal/chipdef/apollo
CFLAGS_re8686.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/romeDriver
CFLAGS_re8686.o += -I${shell pwd}/drivers/net
CFLAGS_re8686.o += -DCONFIG_SDK_KERNEL_LINUX -D__KERNEL__
endif
else
        obj-y += re8686_rtl9602c.o
ifeq ($(CONFIG_RTL8686_TEST),y)
# add latter
        obj-y += re8686_rtl9602c_SW_def.o
endif

CFLAGS_re8686_rtl9602c.o = -Werror -g
ifeq ($(CONFIG_RTL8686_SWITCH),y)
CFLAGS_re8686_rtl9602c.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/include
CFLAGS_re8686_rtl9602c.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/system/include
CFLAGS_re8686_rtl9602c.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/include/hal/chipdef/rtl9602c
CFLAGS_re8686_rtl9602c.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/romeDriver
CFLAGS_re8686_rtl9602c.o += -I${shell pwd}/drivers/net
CFLAGS_re8686_rtl9602c.o += -DCONFIG_SDK_KERNEL_LINUX -D__KERNEL__
endif
endif
endif


obj-$(CONFIG_RTL_MULTI_ETH_WAN) += smux.o
obj-y += ethctl_implement.o


# for RTL9607C series
ifeq ($(CONFIG_RTL9607C_SERIES),y)
	obj-y += re8686_rtl9607c.o
ifeq ($(CONFIG_RTL8686_TEST),y)
	obj-m += re8686_rtl9607c_test.o
endif

ifeq ($(CONFIG_RTL8686_SWITCH),y)
CFLAGS_re8686_rtl9607c.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/include
CFLAGS_re8686_rtl9607c.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/system/include
CFLAGS_re8686_rtl9607c.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/sdk/include/hal/chipdef/rtl9607b
CFLAGS_re8686_rtl9607c.o += -I${shell pwd}/drivers/net/ethernet/realtek/rtl86900/romeDriver
CFLAGS_re8686_rtl9607c.o += -I${shell pwd}/drivers/net
CFLAGS_re8686_rtl9607c.o += -DCONFIG_SDK_KERNEL_LINUX -D__KERNEL__
endif
endif

ifeq ($(CONFIG_MODULE_BUILD_IN),y)
obj-$(CONFIG_RTL8686_SWITCH) += rtl86900/
else
obj-m += rtl86900/
endif

# Add symbolic link to the NIC driver .h file
ifeq ($(CONFIG_RTL9600_SERIES),y)
${shell ln -fs $(NETDIR)/re8686.h $(NETDIR)/re8686_nic.h}
endif
ifeq ($(CONFIG_RTL9601B_SERIES),y)
${shell ln -fs $(NETDIR)/re8686_rtl9601b.h $(NETDIR)/re8686_nic.h}
endif
ifeq ($(CONFIG_RTL9602C_SERIES),y)
${shell ln -fs $(NETDIR)/re8686_rtl9602c.h $(NETDIR)/re8686_nic.h}
endif
ifeq ($(CONFIG_RTL9607C_SERIES),y)
${shell ln -fs $(NETDIR)/re8686_rtl9607c.h $(NETDIR)/re8686_nic.h}
endif

