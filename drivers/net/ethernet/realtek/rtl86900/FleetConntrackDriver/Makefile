include $(APOLLODIR)/Compiler_Flag

ifeq ($(CONFIG_RTK_L34_G3_PLATFORM), y)
ifeq ($(CONFIG_FC_RTL8198F_SERIES),y)
MODULE_NAME = fc_8198f
else
MODULE_NAME = fc_g3
endif
else
MODULE_NAME = fc_drv
endif
ASSISTANT_MODULE_NAME = $(MODULE_NAME)_assistant
FC_DRVDIR = $(APOLLODIR)/FleetConntrackDriver
FC_SRCDIR = $(APOLLODIR)/FleetConntrackDriverSrc
FC_SRCPATH = ../FleetConntrackDriverSrc

FC_BUILD_SRC = $(shell test -e $(FC_SRCDIR) && echo -n y)

srctree=$(KDIR)

EXTRA_CFLAGS += -Wall \
				-Werror \
				$(PON_CFLAGS) \
				-I$(FC_SRCDIR) \
				-I$(FC_SRCDIR)/asicDriver \
				-I$(APOLLODIR)/sdk/include \
				-I$(APOLLODIR)/sdk/system/include \
				-I$(APOLLODIR)/sdk/src/app/diag_rtk/include \
				-I$(KDIR)/net/bridge \
				
ifeq ($(CONFIG_DEFAULTS_KERNEL_2_6),y)
	EXTRA_CFLAGS += -I$(srctree)/drivers/net
else
	EXTRA_CFLAGS += -I$(srctree)/drivers/net/ethernet/realtek
endif

EXTRA_CFLAGS += -DCONFIG_SDK_KERNEL_LINUX

ifeq ($(FC_BUILD_SRC),y)
LUNAR_SVN_VERSION=$(shell $(FC_SRCDIR)/getSvnVersion.sh ..)
SWITCH_SVN_VERSION=$(shell $(FC_SRCDIR)/getSvnVersion.sh $(APOLLODIR)/sdk)
DIAG_SVN_VERSION=$(shell $(FC_SRCDIR)/getSvnVersion.sh $(APOLLODIR)/sdk/src/app/diag_rtk)
FC_SVN_VERSION=$(shell $(FC_SRCDIR)/getSvnVersion.sh $(APOLLODIR)/FleetConntrackDriverSrc)
USER_SVN_VERSION=$(shell $(FC_SRCDIR)/getSvnVersion.sh ../user)
EXTRA_CFLAGS +=  -DLUNAR_SVN_VERSION="\""$(LUNAR_SVN_VERSION)"\""
EXTRA_CFLAGS +=  -DSWITCH_SVN_VERSION="\""$(SWITCH_SVN_VERSION)"\""
EXTRA_CFLAGS +=  -DDIAG_SVN_VERSION="\""$(DIAG_SVN_VERSION)"\""
EXTRA_CFLAGS +=  -DFC_SVN_VERSION="\""$(FC_SVN_VERSION)"\""
EXTRA_CFLAGS +=  -DUSER_SVN_VERSION="\""$(USER_SVN_VERSION)"\""
endif #FC_BUILD_SRC

ifeq ($(CONFIG_RTL9607C_SERIES), y)
EXTRA_CFLAGS += -DCONFIG_RG_RTL9607C_SERIES
endif

ifeq ($(CONFIG_RTK_L34_G3_PLATFORM), y)
EXTRA_CFLAGS += -I$(ROOTDIR)/ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/include
EXTRA_CFLAGS += -I$(ROOTDIR)/ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/aal/include
endif

# to compitable with other kernel module
EXTRA_CFLAGS += -DCONFIG_APOLLO -DCONFIG_RTK_L34_ENABLE

FC_LATEST_AS_OBJ = $(shell find $(FC_DRVDIR) -name $(ASSISTANT_MODULE_NAME).r | tail -n 1)
FC_LATEST_OBJ = $(shell find $(FC_DRVDIR) -name $(MODULE_NAME).*.r | sort -V | tail -n 1)

ifneq ($(FC_BUILD_SRC),y)
#============================== build binary ==============================
ifeq ($(CONFIG_RTK_L34_FC_KERNEL_MODULE), m)

# assistant built-in driver
obj-y += $(ASSISTANT_MODULE_NAME).o
$(ASSISTANT_MODULE_NAME)-objs :=  $(shell basename $(FC_LATEST_AS_OBJ))

obj-$(CONFIG_RTK_L34_FC_KERNEL_MODULE) += $(MODULE_NAME).o
$(MODULE_NAME)-objs :=  $(shell basename $(FC_LATEST_OBJ))

else

ifneq ($(CONFIG_RTK_OPENWRT),y)
obj-y += $(MODULE_NAME).o
$(MODULE_NAME)-objs :=  $(shell basename $(FC_LATEST_OBJ))
else
obj-y += $(MODULE_NAME).openwrt
endif

endif
#==========================================================================
else #!FC_BUILD_SRC
#============================== build source ==============================
RTK_FC_OBJ = $(FC_SRCPATH)/rtk_rg_fc_driver.o \
		$(FC_SRCPATH)/rtk_rg_fc_debug.o \
		$(FC_SRCPATH)/rtk_rg_fc_internal.o \
		$(FC_SRCPATH)/rtk_rg_fc_multicast.o \
		$(FC_SRCPATH)/rtk_rg_fc_proc.o \
		$(FC_SRCPATH)/rtk_rg_fc_mappingAPI.o \
		$(FC_SRCPATH)/rtk_rg_fc_callback.o \
		$(FC_SRCPATH)/rtk_rg_fc_dualHeader.o \
		$(FC_SRCPATH)/rtk_rg_fc_acl.o \
		
ifeq ($(CONFIG_FC_RTL9607C_SERIES), y)
RTK_FC_OBJ += $(FC_SRCPATH)/asicDriver/rtk_rg_apolloPro_asicDriver.o \
                $(FC_SRCPATH)/asicDriver/rtk_rg_apolloPro_internal.o
endif

ifeq ($(CONFIG_RTK_L34_G3_PLATFORM), y)
RTK_FC_OBJ += $(FC_SRCPATH)/asicDriver/rtk_rg_g3_internal.o 
endif

RTK_FC_ASSISTANT_OBJ = $(FC_SRCPATH)/rtk_rg_fc_ext.o


ifeq ($(CONFIG_RTK_L34_FC_KERNEL_MODULE), m)
# CONFIG_RTK_L34_FC_KERNEL_MODULE=m
# assistant built-in driver
obj-y += $(ASSISTANT_MODULE_NAME).o
$(ASSISTANT_MODULE_NAME)-objs := $(RTK_FC_ASSISTANT_OBJ)
obj-m := $(MODULE_NAME).o

else
# CONFIG_RTK_L34_FC_KERNEL_MODULE=y
# build FC as built-in
RTK_FC_OBJ += $(RTK_FC_ASSISTANT_OBJ)
obj-y += $(MODULE_NAME).o

endif #CONFIG_RTK_L34_FC_KERNEL_MODULE

# FC module
#obj-$(CONFIG_RTK_L34_FC_KERNEL_MODULE):= $(MODULE_NAME).o
$(MODULE_NAME)-objs := $(RTK_FC_OBJ)

$(shell cp -rf $(FC_DRVDIR)/$(MODULE_NAME).o $(FC_DRVDIR)/$(MODULE_NAME).$(FC_SVN_VERSION).r)
$(shell cp -rf $(FC_DRVDIR)/$(ASSISTANT_MODULE_NAME).o $(FC_DRVDIR)/$(ASSISTANT_MODULE_NAME).r)
#==========================================================================
endif #FC_BUILD_SRC


