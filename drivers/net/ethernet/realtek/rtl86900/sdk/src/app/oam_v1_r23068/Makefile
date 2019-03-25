#
# Copyright (C) 2011 Realtek Semiconductor Corp.
#
# Makefile for SDK EPON OAM protocol Source
#

APOLLO_SDK_PATH = $(SDKDIR)
VERSION=1.1.2

include $(APOLLODIR)/Compiler_Flag
EXTRA_CFLAGS+=$(PON_CFLAGS)

LIB += -L$(APOLLODIR)/object/src/app/lib -lrtk -lpthread -lrt -lpr -L$(ROOTDIR)/lib/librtk

#ASIC mode
CFLAGS := -DRTK_X86_CLE -DRTK_X86_ASICDRV -DCONFIG_APOLLO_CMD -DCONFIG_SMIDA1011 -DDISABLE_VLAN_SHADOW -DCONFIG_LINUX_USER_SHELL -D'EPON_OAM_VERSION="$(VERSION)"'

ifeq ($(CONFIG_RTK_L34_ENABLE),y)
CFLAGS += -DCONFIG_HGU_APP
else
CFLAGS += -DCONFIG_SFU_APP
endif

EXTRA_CFLAGS += -I$(ROOTDIR) -I$(ROOTDIR)/include -DEMBED -include $(ROOTDIR)/include/linux/autoconf.h -include $(ROOTDIR)/config/autoconf.h

SDK_ROOT_DIR = $(APOLLO_SDK_PATH)
SDK_INCLUDE   = $(SDK_ROOT_DIR)/include
SYS_DIR = $(SDK_ROOT_DIR)/system/linux
SYS_INCLUDE = $(SDK_ROOT_DIR)/system/include
RTUSR_INC = $(SYS_DIR)/rtk/rtusr/include
COMMON_DIR = $(SDK_ROOT_DIR)/src/common

EPON_OAM_DIR := .
EPON_OAM_INC_DIR  = $(EPON_OAM_DIR)/inc
EPON_OAM_SRC_DIR := $(EPON_OAM_DIR)/src
USER_OAM_INC_DIR := $(EPON_OAM_INC_DIR)/user
#DRV_OAM_INC_DIR := $(EPON_OAM_INC_DIR)/drv
USER_OAM_SRC_DIR := $(EPON_OAM_SRC_DIR)/user
ifeq ($(CONFIG_RTK_L34_ENABLE),y)
DRV_OAM_SRC_DIR := $(EPON_OAM_SRC_DIR)/drv/drv_hgu
DRV_OAM_INC_DIR := $(EPON_OAM_INC_DIR)/drv/drv_hgu
else
DRV_OAM_SRC_DIR := $(EPON_OAM_SRC_DIR)/drv/drv_sfu
DRV_OAM_INC_DIR := $(EPON_OAM_INC_DIR)/drv/drv_sfu
endif
USER_WRAPPER_INC_DIR := $(USER_OAM_INC_DIR)/wrapper
USER_WRAPPER_SRC_DIR := $(USER_OAM_SRC_DIR)/wrapper
IGMP_OAM_INC_DIR := $(EPON_OAM_INC_DIR)/igmp
IGMP_OAM_SRC_DIR := $(EPON_OAM_SRC_DIR)/igmp

ifeq ($(CONFIG_RTK_L34_ENABLE),y)
USER_HGU_SRC_DIR := $(USER_WRAPPER_SRC_DIR)/ctc_hgu
USER_HGU_INC_DIR := $(USER_WRAPPER_INC_DIR)/ctc_hgu
else
USER_SFU_SRC_DIR := $(USER_WRAPPER_SRC_DIR)/ctc_sfu
USER_SFU_INC_DIR := $(USER_WRAPPER_INC_DIR)/ctc_sfu
endif

ifeq ($(CONFIG_RTK_L34_ENABLE),y)
RG_INCLUDE   = $(SDK_ROOT_DIR)/../romeDriver
#for userspace which include some data structure defined in apollo-liteRomeDriver
endif

INCLUDE += -I$(EPON_OAM_INC_DIR) \
		   -I$(DRV_OAM_INC_DIR) \
           -I$(USER_OAM_INC_DIR) \
           -I$(USER_WRAPPER_INC_DIR) \
           -I$(SDK_INCLUDE) \
           -I$(SYS_INCLUDE) \
           -I$(SDK_INCLUDE)/hal/chipdef/apollo \
           -I$(SDKDIR)/src/app/pkt_redirect/inc \
           -I$(ROOTDIR)/rtk_voip/kernel/include/ \
           -I$(ROOTDIR)/user/rtk_voip/includes \
		   -I$(IGMP_OAM_INC_DIR) \

ifeq ($(CONFIG_RTK_L34_ENABLE),y)
INCLUDE += -I$(RG_INCLUDE)
INCLUDE += -I$(USER_HGU_INC_DIR)
else
INCLUDE += -I$(USER_SFU_INC_DIR)
endif

CFLAGS += -DCONFIG_RTL865X \
          -DCONFIG_RTL865XC \
          -DRTL865X_MODEL_USER \

EXEC = eponoamd
EXECLI = oamcli
BIN = bin

OAMCORE := \
       $(EPON_OAM_SRC_DIR)/epon_oam_main.o \
       $(EPON_OAM_SRC_DIR)/epon_oam_db.o \
       $(EPON_OAM_SRC_DIR)/epon_oam_rx.o \
       $(EPON_OAM_SRC_DIR)/epon_oam_tx.o \
       $(EPON_OAM_SRC_DIR)/epon_oam_state.o \
       $(EPON_OAM_SRC_DIR)/epon_oam_dbg.o

OAMCORE_INC := $(shell ls $(EPON_OAM_INC_DIR)/*.h)

OAMCLI := \
       $(EPON_OAM_SRC_DIR)/epon_oam_cli.o \

USEROAM := \
       $(USER_OAM_SRC_DIR)/ctc_oam.o \
       $(USER_OAM_SRC_DIR)/ctc_oam_var.o \
       $(USER_OAM_SRC_DIR)/ctc_oam_cli.o \
       $(USER_OAM_SRC_DIR)/tk_oam.o \
       $(USER_OAM_SRC_DIR)/kt_oam.o \
	   $(USER_OAM_SRC_DIR)/ctc_oam_alarmtbl.o\
	   $(USER_OAM_SRC_DIR)/ctc_loop_detect.o \
	   $(USER_OAM_SRC_DIR)/cortina_oam.o

ifeq ($(CONFIG_RTK_L34_ENABLE),y)
DRVOAM := \
	   $(DRV_OAM_SRC_DIR)/drv_vlan.o \
       $(DRV_OAM_SRC_DIR)/drv_mac.o \
       $(DRV_OAM_SRC_DIR)/drv_convert.o \
       $(DRV_OAM_SRC_DIR)/../sys_portview.o

USEROAM_INC := $(shell ls $(USER_OAM_INC_DIR)/*.h)
else
DRVOAM := \
	   $(DRV_OAM_SRC_DIR)/drv_acl.o \
       $(DRV_OAM_SRC_DIR)/drv_clf.o \
	   $(DRV_OAM_SRC_DIR)/drv_vlan.o \
       $(DRV_OAM_SRC_DIR)/drv_mac.o \
       $(DRV_OAM_SRC_DIR)/drv_convert.o \
       $(DRV_OAM_SRC_DIR)/../sys_portview.o

USEROAM_INC := $(shell ls $(USER_OAM_INC_DIR)/*.h)
endif       

USERWRAPPER := \
       $(USER_WRAPPER_SRC_DIR)/ctc_wrapper.o \

USERWRAPPER_INC := $(shell ls $(USER_WRAPPER_INC_DIR)/*.h)

ifeq ($(CONFIG_RTK_L34_ENABLE),y)
USER_ONU_TYPE := \
       $(USER_HGU_SRC_DIR)/ctc_hgu.o \
       $(USER_HGU_SRC_DIR)/ctc_mc.o \
       $(USER_HGU_SRC_DIR)/ctc_voip.o \
       $(USER_HGU_SRC_DIR)/ctc_vlan.o

USER_ONU_TYPE_INC := $(shell ls $(USER_HGU_INC_DIR)/*.h)
else
USER_ONU_TYPE := \
       $(USER_SFU_SRC_DIR)/ctc_clf.o \
       $(USER_SFU_SRC_DIR)/ctc_vlan.o \
       $(USER_SFU_SRC_DIR)/ctc_sfu.o \
       $(USER_SFU_SRC_DIR)/ctc_mc.o 

USER_ONU_TYPE_INC := $(shell ls $(USER_SFU_INC_DIR)/*.h)
endif

IGMPOAM := \
		$(IGMP_OAM_SRC_DIR)/epon_oam_igmp.o \
		$(IGMP_OAM_SRC_DIR)/epon_oam_igmp_db.o \
		$(IGMP_OAM_SRC_DIR)/epon_oam_igmp_querier.o \
		$(IGMP_OAM_SRC_DIR)/epon_oam_igmp_hw.o

IGMPOAM_INC := $(shell ls $(IGMP_OAM_INC_DIR)/*.h)

$(EXEC): $(OAMCORE) $(OAMCORE_INC) $(DRVOAM) $(USEROAM) $(USEROAM_INC) $(USERWRAPPER) $(USERWRAPPER_INC) $(USER_ONU_TYPE) $(USER_ONU_TYPE_INC) $(IGMPOAM) $(IGMPOAM_INC)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(INCLUDE) $(OAMCORE) $(DRVOAM) $(USEROAM) $(USERWRAPPER) $(USER_ONU_TYPE) $(IGMPOAM) $(LIB) -o $(BIN)/$@
	$(STRIP) $(BIN)/$@

$(EXECLI): $(OAMCLI) $(OAMCORE_INC)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(INCLUDE) $(OAMCLI) $(LIB) -o $(BIN)/$@
	$(STRIP) $(BIN)/$@

%.o: %.c
	$(CC) $(INCLUDE) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

# Force to rebuild all every time
all: clean $(EXEC) $(EXECLI)

install: $(EXEC) $(EXECLI)
	cp $(BIN)/$(EXEC) $(BINDIR)
	cp $(BIN)/$(EXECLI) $(BINDIR)
ifeq ($(CONFIG_EPON_FEATURE),y)
	$(MAKE) -C $(SDK_ROOT_DIR)/src/module/epon_drv install
	@if [ -f $(SDK_ROOT_DIR)/src/module/epon_mpcp/epon_mpcp.ko ] ; then \
		$(MAKE) -C $(SDK_ROOT_DIR)/src/module/epon_mpcp install; \
	fi
	@if [ -f $(SDK_ROOT_DIR)/src/module/epon_polling/epon_polling.ko ] ; then \
		$(MAKE) -C $(SDK_ROOT_DIR)/src/module/epon_polling install; \
	fi
endif
clean:
	rm -f $(OAMCORE) $(USEROAM) $(USERWRAPPER) $(USER_ONU_TYPE) $(IGMPOAM) $(BIN)/$(EXEC) $(BIN)/$(EXECLI) *.gdb *.elf *.elf2flt
