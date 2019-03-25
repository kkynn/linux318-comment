srctree=$(KDIR)

EXTRA_CFLAGS += -Wall
EXTRA_CFLAGS += $(PON_CFLAGS)
OMCI_PF_SYMVER =
ifeq ($(CONFIG_APOLLO_ROMEDRIVER),y)
EXTRA_CFLAGS += -I$(APOLLODIR)/romeDriver
OMCI_PF_SYMVER = $(APOLLODIR)/object/src/app/omci_v1/DRV/platform/rtl9607_rg/Module.symvers
else
EXTRA_CFLAGS += -I$(APOLLODIR)/FleetConntrackDriver
OMCI_PF_SYMVER = $(APOLLODIR)/object/src/app/omci_v1/DRV/platform/rtl9607_fc/Module.symvers
endif
EXTRA_CFLAGS += -I$(srctree)/drivers/net
EXTRA_CFLAGS += -DCONFIG_SDK_KERNEL_LINUX
ifeq ($(CONFIG_DEFAULTS_KERNEL_3_18),y)
EXTRA_CFLAGS += -I$(srctree)/drivers/net/ethernet/realtek	# for if_smux.h
else
EXTRA_CFLAGS += -I$(srctree)/include/linux
endif
EXTRA_CFLAGS += -I$(APOLLODIR)/tr142/include
EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/include
EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/system/include
EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/src/app/diag_rtk/include
EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/src/app/omci_v1/API

MODULE_NAME=rtk_tr142
obj-m := $(MODULE_NAME).o

all:
	$(MAKE) -C $(KDIR) M=$(APOLLODIR)/tr142 ARCH=rlx CROSS_COMPILE=$(CROSS_COMPILE) \
	KBUILD_EXTRA_SYMBOLS=$(OMCI_PF_SYMVER) \
	modules

install:
	cp -af $(MODULE_NAME).ko $(ROMFSDIR)/lib/modules
	cp -af $(MODULE_NAME).sh $(ROMFSDIR)/etc

clean:
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.symvers *.ko

