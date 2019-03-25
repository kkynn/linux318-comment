# 
# Makefile for the Linux network (ethercard) device drivers.
#

ifneq ($(CONFIG_RG_G3_SERIES),y)
obj-$(CONFIG_MODULE_BUILD_IN)+=sdk/
endif # !CONFIG_RG_G3_SERIES

ifeq ($(CONFIG_MODULE_BUILD_IN),m) 
ifeq ($(CONFIG_RTK_L34_ENABLE),y)
obj-m +=romeDriver/
endif
else
obj-$(CONFIG_RTK_L34_ENABLE)+=romeDriver/
endif

ifeq ($(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE),y)
obj-$(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)+=FleetConntrackDriver/
endif

obj-$(CONFIG_YUEME_DPI) += dpi/

all:
	$(MAKE) -C sdk all;
	$(MAKE) -C romeDriver all;
	$(MAKE) -C tr142 all;
ifdef CONFIG_YUEME_DPI
	$(MAKE) -C dpi all;
endif

install:
	$(MAKE) -C sdk install;
	$(MAKE) -C romeDriver install; 
	$(MAKE) -C tr142 install;
ifdef CONFIG_YUEME_DPI
	$(MAKE) -C dpi install;
endif

