#########################################################
#	Build Realtek OMCI modules		#
#########################################################
include $(APOLLODIR)/Compiler_Flag

MODULE_NAME=bdp_00000100
PWD=$(shell pwd)
SDKDIR=$(APOLLODIR)/sdk
SDK_INCLUDE   = $(SDKDIR)/include
SYS_INCLUDE   = $(SDKDIR)/system/include
DRV_INCLUDE   = $(SDKDIR)/system/linux
OMCIDIR=$(OMCIROOT_DIR)
OMCISRCDIR=$(OMCIDIR)/OMCI/src
OMCIDRVDIR=$(OMCIDIR)/DRV
OMCI_API_INCLUDE = $(OMCIDIR)/API
MODULEDIR=$(SDKDIR)/src/module

KBUILD_EXTRA_SYMBOLS += $(OMCIDRVDIR)/Module.symvers

EXTRA_CFLAGS += -Wall -Werror -Wno-unused-function -Wframe-larger-than=2048
EXTRA_CFLAGS += -I$(SDK_INCLUDE) \
		-I$(DRV_INCLUDE) \
		-I$(SYS_INCLUDE) \
		-I$(MODULEDIR) \
		-I$(OMCIDIR) \
		-I$(OMCISRCDIR) \
		-I$(OMCIDRVDIR) \
		-I$(OMCI_API_INCLUDE) \
		-DCONFIG_SDK_KERNEL_LINUX $(PON_CFLAGS)

BIN_FILE := $(shell find $(PWD) -not \( -name "\.svn" -prune \) -name "*.ko_*")
SRC_FILE := $(shell find $(PWD) -not \( -name "\.svn" -prune \) -name "*.c")

ifneq ($(SRC_FILE),)
REBUILD := y
endif

ifdef REBUILD
obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-objs:= omci_force_veipRule_to_sfu.o

all:
	@echo "   BUILD module $(MODULE_NAME)"
ifeq ($(shell [ $(CONFIG_SDK_LUNA_G3) == y ] || [ $(CONFIG_SDK_CA8279) == y ] && echo true),true)
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules || exit 1
else
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=rlx CROSS_COMPILE=$(CROSS_COMPILE) || exit 1
endif
else
all: clean
	@echo "   RENAME module $(MODULE_NAME)"
ifneq ($(BIN_FILE),)
	cp -af $(BIN_FILE) $(PWD)/$(MODULE_NAME).ko
endif
endif

install:
	@if [ ! -d $(ROMFSDIR)/lib/modules/features ]; \
        then \
                mkdir $(ROMFSDIR)/lib/modules/features ; \
        fi
	cp -af $(MODULE_NAME).ko $(ROMFSDIR)/lib/modules/features

clean:
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.symvers *.ko modules.order
