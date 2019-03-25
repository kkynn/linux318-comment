#
# Copyright (C) 2014 Realtek Semiconductor Corp.
# All Rights Reserved.
#
# This program is the proprietary software of Realtek Semiconductor
# Corporation and/or its licensors, and only be used, duplicated,
# modified or distributed under the authorized license from Realtek.
#
# ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
# THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
#
# Purpose : Compile procedure of OMCI
#
# Feature : The file includes the following modules and sub-modules
#           (1) Makefile
#

DEV_SVN_VERSION=`svn info | grep -e Rev: | sed -e 's/.*Rev: //'`
# initial flag

RELEASE_SUBVERSION		= \"r${DEV_SVN_VERSION}\"
OMCI_X86				= n
SUPPORT_MIB_CFG_FILE	= n

# toolchain assign
ifeq ($(OMCI_X86),y)
SDKDIR					= ../../../
CC						= $(TOOLCHAIN_DIR)gcc -m32 -g
AR						= $(TOOLCHAIN_DIR)ar
CONFIG_GPON_VERSION		= 2
endif

# path assign
PWD				= $(shell pwd)
OMCI_DIR		= OMCI
MIB_DIR			= MIB
GOS_DIR			= GOS
API_DIR			= API
DRV_DIR			= DRV
MC_DIR			= MCAST
MC_RTK_DIR		= MCAST/src/rtk
MC_RG_DIR		= MCAST/src/rg
VOIP_DIR		= VOIP
VOIP_RTK_DIR	= VOIP/src/rtk
FAL_DIR			= FAL
FEATURE_DIR		= FAL/src/features
BIN_LIB_TABLES_DIR	:= LIB/omci

# src
SRC_OMCI		:= $(wildcard $(OMCI_DIR)/src/*.c)
SRC_OMCI		:= $(filter-out $(OMCI_DIR)/src/omci_driver.c, $(SRC_OMCI))
SRC_OMCI		:= $(filter-out $(OMCI_DIR)/src/omci_config.c, $(SRC_OMCI))
SRC_MIB			= $(wildcard $(MIB_DIR)/src/*.c)
ifneq ($(OMCI_X86),y)
SRC_MIB			+= $(wildcard $(MC_DIR)/src/*c)
SRC_MIB			+= $(wildcard $(MC_RTK_DIR)/*c)
SRC_MIB			+= $(wildcard $(MC_RG_DIR)/*c)
endif
SRC_MIB			+= $(OMCI_DIR)/src/omci_driver.c $(OMCI_DIR)/src/omci_config.c
SRC_TABLES		= $(wildcard $(MIB_DIR)/src/tables/*.c)
SRC_GOS			= $(wildcard $(GOS_DIR)/src/*.c)
SRC_FAL			= $(wildcard $(FAL_DIR)/src/*.c)
SRC_FAL_CFG		= $(wildcard $(FAL_DIR)/src/cfg/*.c)
SRC_FEATURE		= $(wildcard $(FEATURE_DIR)/*.c)
SRC_VOIP		= $(wildcard $(VOIP_DIR)/src/*c)
#ifeq ($(CONFIG_RTK_VOIP_MANAGER), y)
SRC_VOIP_RTK	= $(wildcard $(VOIP_RTK_DIR)/*c)
#endif
SRC_API			= $(wildcard $(API_DIR)/*.c)
SRC_API			+= $(wildcard $(GOS_DIR)/src/omci_msgq.c)
SRC_API			+= $(wildcard $(GOS_DIR)/src/omci_util.c)
SRC_APP			= omci_main.c
SRC_CLI			= omci_cmd.c

# obj
OBJ_VOIP		= $(patsubst %.c, %.o, $(SRC_VOIP))
OBJ_FAL			= $(patsubst %.c, %.o, $(SRC_FAL))
ifneq ($(SRC_FAL_CFG),)
OBJ_FAL_CFG		= $(patsubst %.c, %.o, $(SRC_FAL_CFG))
else
OBJ_FAL_CFG		= $(patsubst %.cfg, %.o, $(wildcard $(FAL_DIR)/src/cfg/*.cfg))
endif
OBJ_GOS			= $(patsubst %.c, %.o, $(SRC_GOS))
OBJ_API			= $(patsubst %.c, %.o, $(SRC_API))
OBJ_MIB			= $(patsubst %.c, %.o, $(SRC_MIB))
OBJ_OMCI		= $(patsubst %.c, %.o, $(SRC_OMCI))

# dependency file
DEP_SRC			= $(shell find -path "./DRV" -prune -o -name "*.c" -print)
DEP_FILE		= $(patsubst %.c, %.d, $(DEP_SRC))

# parallel build job
MAKE_JOBS		= $(shell expr $$(cat /proc/cpuinfo  | grep -c processor) + 1)

# library
LIB_MIB			= omci_mib
BIN_LIB_MIB		= lib$(LIB_MIB).so
BIN_LIB_TABLES	= $(patsubst %.c, %.so, $(SRC_TABLES))
LIB_GOS			= omci_gos
BIN_LIB_GOS		= lib$(LIB_GOS).so
LIB_API			= omci_api
BIN_LIB_API		= lib$(LIB_API).so
LIB_FAL			= omci_fal
BIN_LIB_FAL		= lib$(LIB_FAL).so
BIN_LIB_FEATURE = $(patsubst %.c, %.so, $(SRC_FEATURE))
LIB_VOICE		= omci_voice
BIN_LIB_VOICE	= lib$(LIB_VOICE).so
#ifeq ($(CONFIG_RTK_VOIP_MANAGER), y)
BIN_LIB_VOICE_RTK = $(patsubst %.c, %.so, $(SRC_VOIP_RTK))
#else
#BIN_LIB_VOICE_RTK = 
#endif
# binary
BIN_OMCI_APP	= omci_app
BIN_OMCI_CLI	= omcicli

# inital script
RUN_OMCI_SCRIPT ="runomci.sh"
OMCI_CUSTOM_CONF ="omci_custom_opt.conf"
OMCI_MIB_CFG ="omci_mib.cfg"

# define
DEFINES			=
DEFINES			+= $(PON_CFLAGS)
DEFINES			+= -DCONFIG_GPON_VERSION=$(CONFIG_GPON_VERSION)
ifdef RELEASE_VERSION
DEFINES			+= -DCONFIG_RELEASE_VERSION="$(RELEASE_VERSION)"
endif
ifdef RELEASE_SUBVERSION
DEFINES			+= -DCONFIG_RELEASE_SUBVERSION="$(RELEASE_SUBVERSION)"
endif
DEFINES			+= -D_GNU_SOURCE

ifeq ($(OMCI_X86),y)
DEFINES			+= -DOMCI_X86
DEFINES			+= -DCONFIG_SDK_APOLLOMP
endif

ifeq ($(SUPPORT_MIB_CFG_FILE),y)
DEFINES			+= -DSUPPORT_MIB_CFG_FILE
endif

ifeq ($(CONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT),y)
DEFINES			+= -DCONFIG_LUNA_FIRMWARE_UPGRADE_SUPPORT
endif


# compile flag
RTK_INCLUDES = \
				-I$(OMCI_DIR)/inc \
				-I$(OMCI_DIR)/src \
				-I$(MIB_DIR)/inc \
				-I$(MIB_DIR)/src  \
				-I$(MIB_DIR)/src/tables \
				-I$(GOS_DIR)/inc  \
				-I$(GOS_DIR)/src \
				-I$(API_DIR) \
				-I$(VOIP_DIR)/inc \
				-I$(VOIP_DIR)/inc/rtk \
				-I$(MC_DIR)/inc \
				-I$(MC_DIR)/inc/rg \
				-I$(MC_DIR)/inc/rtk \
				-I$(FAL_DIR)/inc \
				-I$(SDKDIR)/src/app/pkt_redirect/inc\
				-I$(SDKDIR)/src/app/rtk_igmp/include \
				-I$(ROOTDIR)/rtk_voip/kernel/include/ \
				-I$(ROOTDIR)/user/rtk_voip/includes
				
				

DRV_INCLUDES = \
				-I$(SDKDIR)/include \
				-I$(SDKDIR)/system/include \
				-I$(SDKDIR)/system/linux

CFLAGS			=  -Wall  -Werror -Os  -pthread
CFLAGS			+= $(RTK_INCLUDES) $(DRV_INCLUDES)
CFLAGS			+= -L$(PWD) 
ifneq ($(OMCI_X86),y)
CFLAGS			+= -L$(APOLLODIR)/object/src/app/lib/
endif
CFLAGS			+= $(DEFINES)


LIBS			= -lm -ldl
ifneq ($(OMCI_X86),y)
LIBS			+= -lpr -lrtk
endif


BIN_LIB_FEATURE_FILES := $(shell find $(FEATURE_DIR) -not \( -name "\.svn" -prune \) -name "*.so.*")

# targets
all: clean
	time make -j$(MAKE_JOBS) parallel_build

ifneq ($(OMCI_X86),y)
parallel_build: $(BIN_LIB_GOS) $(BIN_LIB_API) $(BIN_LIB_FAL) $(BIN_LIB_VOICE) $(BIN_LIB_MIB) $(BIN_LIB_TABLES) $(BIN_LIB_FEATURE) $(BIN_LIB_VOICE_RTK) $(BIN_OMCI_APP) $(BIN_OMCI_CLI) TGT_DRV
else
parallel_build: $(BIN_LIB_GOS) $(BIN_LIB_API) $(BIN_LIB_FAL) $(BIN_LIB_MIB) $(BIN_LIB_TABLES) $(BIN_LIB_FEATURE) $(BIN_OMCI_APP) $(BIN_OMCI_CLI) TGT_DRV
endif
ifneq ($(BIN_LIB_FEATURE_FILES),)
	for s in $(BIN_LIB_FEATURE_FILES) ; do \
		tmp=`echo $${s%%so.*}so`; \
		cp -af $$s $$tmp ; \
	done
endif

%.o: %.c
	$(CC) $(CFLAGS) -c -fPIC -o $@ $<

%.d: %.c
	$(CC) $(CFLAGS) -MM $< -MT $(<:.c=.o) -MF $@



include $(DEP_FILE)

$(BIN_LIB_GOS): $(OBJ_GOS)
	$(CC) $(CFLAGS) -fPIC -shared -o $(BIN_LIB_GOS) $(OBJ_GOS) -lrt

$(BIN_LIB_API): $(OBJ_API)
	$(CC) $(CFLAGS) -fPIC -shared -o $(BIN_LIB_API) $(OBJ_API)
ifneq ($(OMCI_X86),y)
	cp -af $(BIN_LIB_API) $(APOLLODIR)/object/src/app/lib/
endif

$(OBJ_FAL_CFG):
ifneq ($(SRC_FAL_CFG),)
	for f in $(SRC_FAL_CFG) ; do \
		obj=`echo $$f | sed -e 's/\.c/\.o/g'`; \
		$(CC) $(CFLAGS) -c -fPIC -o $$obj $$f ;\
		cfg=`echo $$obj | sed -e 's/\.o/\.cfg/g'`; \
		cp -af $$obj $$cfg ;\
	 done
else
	for f in $@ ; do \
		cfg=`echo $$f | sed -e 's/\.o/\.cfg/g'`; \
		cp -af $$cfg $$f ; \
	done
endif

$(BIN_LIB_VOICE): $(OBJ_VOIP)
	$(CC) $(CFLAGS) -shared -fPIC -o $(BIN_LIB_VOICE) $(OBJ_VOIP) -lc -ldl

$(BIN_LIB_FAL): $(OBJ_FAL_CFG) $(OBJ_FAL)
	$(CC) $(CFLAGS) -shared -fPIC -o $(BIN_LIB_FAL) $(OBJ_FAL_CFG) $(OBJ_FAL) -lc -ldl
ifneq ($(OMCI_X86),y)
	cp -af $(BIN_LIB_FAL) $(APOLLODIR)/object/src/app/lib/
endif

$(BIN_LIB_MIB): $(OBJ_MIB)
	$(CC) $(CFLAGS) -fPIC -shared -o $(BIN_LIB_MIB) $(OBJ_MIB)

%.so: %.o $(BIN_LIB_MIB)
ifeq ($(OMCI_X86),y)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< $(PWD)/$(BIN_LIB_MIB) $(LIBS)
else
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< -l$(LIB_MIB) $(LIBS)
endif
	cp $@ $(BIN_LIB_TABLES_DIR)
	find $(BIN_LIB_TABLES_DIR) -not \( -name .svn -prune \) -a -not \( -name "mib_*" -prune \) -a -not \( -type d \) | xargs rm -rf

$(BIN_OMCI_APP): $(BIN_LIB_GOS) $(BIN_LIB_FAL) $(BIN_LIB_VOICE) $(BIN_LIB_MIB) $(OBJ_OMCI)
ifeq ($(OMCI_X86),y)
	$(CC) $(CFLAGS) $(SRC_APP) -o $@ $(OBJ_OMCI) $(PWD)/$(BIN_LIB_GOS) $(PWD)/$(BIN_LIB_FAL) $(PWD)/$(BIN_LIB_VOICE) $(PWD)/$(BIN_LIB_MIB) $(LIBS)
else
	$(CC) $(CFLAGS) -rdynamic $(SRC_APP) -o $@ $(OBJ_OMCI) -l$(LIB_GOS) -l$(LIB_FAL) -l$(LIB_VOICE) -l$(LIB_MIB) $(LIBS)
endif

$(BIN_OMCI_CLI): $(BIN_LIB_GOS) $(BIN_LIB_API) $(BIN_LIB_FAL) $(SRC_CLI)
ifeq ($(OMCI_X86),y)
	$(CC) $(CFLAGS) $(SRC_CLI) -o $@ $(PWD)/$(BIN_LIB_GOS) $(PWD)/$(BIN_LIB_API) $(PWD)/$(BIN_LIB_FAL)
else
	$(CC) $(CFLAGS) $(SRC_CLI) -o $@ -l$(LIB_GOS) -l$(LIB_API) -l$(LIB_FAL)
endif

TGT_DRV:
ifneq ($(OMCI_X86),y)
	$(MAKE) -C $(DRV_DIR) all OMCIROOT_DIR=$(PWD)
else
	$(MAKE) -C $(DRV_DIR) all OMCIROOT_DIR=$(PWD) OMCI_X86=$(OMCI_X86)
endif

clean:
	rm -f $(BIN_OMCI_APP)
	rm -f $(BIN_OMCI_CLI)
	rm -f $(BIN_LIB_FAL)
	find . -name "*.so" | xargs rm -rf
	find . -name "*.o" | xargs rm -rf
	find . -name "*.d" | xargs rm -rf
ifneq ($(OMCI_X86),y)
	rm -f $(APOLLODIR)/object/src/app/lib/*omci*
	$(MAKE) -C $(DRV_DIR) clean
	$(UNPREPARE)
else
	$(MAKE) -C $(DRV_DIR) clean OMCI_X86=$(OMCI_X86)
endif

ifneq ($(OMCI_X86),y)
UNPREPARE		= $(shell \
	if [ -f $(ROOTDIR)/include/linux/autoconf.h ]; then\
		if [ -n "`grep "CONFIG_RTK_VOIP_CON_CH_NUM 0" $(ROOTDIR)/include/linux/autoconf.h`" ]; then\
			sed -i '/CONFIG_RTK_VOIP_CON_CH_NUM/ { N; d; }' $(ROOTDIR)/include/linux/autoconf.h;\
			sed -i '/CONFIG_RTK_VOIP_G729AB/ { N; d; }' $(ROOTDIR)/include/linux/autoconf.h;\
			sed -i '/CONFIG_RTK_VOIP_G7231/ { N; d; }' $(ROOTDIR)/include/linux/autoconf.h;\
			sed -i '/CONFIG_RTK_VOIP_G726/ { N; d; }' $(ROOTDIR)/include/linux/autoconf.h;\
			sed -i '/CONFIG_RTK_VOIP_G722/ { N; d; }' $(ROOTDIR)/include/linux/autoconf.h;\
			if [ -L $(ROOTDIR)/user/rtk_voip/includes/kernel_config.h ]; then\
				unlink $(ROOTDIR)/user/rtk_voip/includes/kernel_config.h;\
			fi;\
		fi;\
	else\
		if [ -n "`grep "CONFIG_RTK_VOIP_CON_CH_NUM 0" $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h`" ]; then\
			sed -i '/CONFIG_RTK_VOIP_CON_CH_NUM/ { N; d; }' $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			sed -i '/CONFIG_RTK_VOIP_G729AB/ { N; d; }' $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			sed -i '/CONFIG_RTK_VOIP_G7231/ { N; d; }' $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			sed -i '/CONFIG_RTK_VOIP_G726/ { N; d; }' $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			sed -i '/CONFIG_RTK_VOIP_G722/ { N; d; }' $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			if [ -L $(ROOTDIR)/user/rtk_voip/includes/kernel_config.h ]; then\
				unlink $(ROOTDIR)/user/rtk_voip/includes/kernel_config.h;\
			fi;\
		fi;\
	fi)

$(UNPREPARE): ;

PREPARE			= $(shell \
	if [ -f $(ROOTDIR)/include/linux/autoconf.h ]; then\
		if [ -z "`grep "CONFIG_RTK_VOIP_CON_CH_NUM" $(ROOTDIR)/include/linux/autoconf.h`" ]; then\
			echo "\#define CONFIG_RTK_VOIP_CON_CH_NUM 0" >> $(ROOTDIR)/include/linux/autoconf.h;\
			echo "\#define CONFIG_RTK_VOIP_G729AB 0" >> $(ROOTDIR)/include/linux/autoconf.h;\
			echo "\#define CONFIG_RTK_VOIP_G7231 0" >> $(ROOTDIR)/include/linux/autoconf.h;\
			echo "\#define CONFIG_RTK_VOIP_G726 0" >> $(ROOTDIR)/include/linux/autoconf.h;\
			echo "\#define CONFIG_RTK_VOIP_G722 0" >> $(ROOTDIR)/include/linux/autoconf.h;\
			ln -nfs $(ROOTDIR)/include/linux/autoconf.h $(ROOTDIR)/user/rtk_voip/includes/kernel_config.h;\
		fi;\
	else\
		if [ -z "`grep "CONFIG_RTK_VOIP_CON_CH_NUM" $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h`" ]; then\
			echo "\#define CONFIG_RTK_VOIP_CON_CH_NUM 0" >> $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			echo "\#define CONFIG_RTK_VOIP_G729AB 0" >> $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			echo "\#define CONFIG_RTK_VOIP_G7231 0" >> $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			echo "\#define CONFIG_RTK_VOIP_G726 0" >> $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			echo "\#define CONFIG_RTK_VOIP_G722 0" >> $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h;\
			ln -nfs $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h $(ROOTDIR)/user/rtk_voip/includes/kernel_config.h;\
		fi;\
	fi)

$(PREPARE): ;
endif

install:
	cp -af $(BIN_OMCI_APP) $(BINDIR)
	cp -af $(BIN_OMCI_CLI) $(BINDIR)
	cp -af $(BIN_LIB_GOS) LIB/
	cp -af $(BIN_LIB_MIB) LIB/
	cp -af $(BIN_LIB_API) LIB/
	cp -af $(BIN_LIB_FAL) LIB/
	cp -af $(BIN_LIB_VOICE) LIB/
	$(STRIP) $(BIN_LIB_TABLES_DIR)/*.so
	cp -af LIB/* $(ROMFSDIR)/lib/
	@if [ ! -d $(ROMFSDIR)/lib/features/internal ]; then \
		mkdir -p $(ROMFSDIR)/lib/features/internal; \
	fi
	find $(FEATURE_DIR) -name "*.so" | xargs -i cp {} $(ROMFSDIR)/lib/features/internal/
	@if [ ! -d $(ROMFSDIR)/lib/voip ]; then \
		mkdir -p $(ROMFSDIR)/lib/voip; \
	fi
	find $(VOIP_RTK_DIR) -name "*.so" | xargs -i cp {} $(ROMFSDIR)/lib/voip/
	cp -af $(RUN_OMCI_SCRIPT) $(ROMFSDIR)/etc
	cp -af $(OMCI_CUSTOM_CONF) $(ROMFSDIR)/etc
	cp -af $(OMCI_MIB_CFG) $(ROMFSDIR)/etc
ifneq ($(OMCI_X86),y)
	$(MAKE) -C $(DRV_DIR) install
endif

