srctree=$(KDIR)

EXTRA_CFLAGS += -Wall #-Werror
EXTRA_CFLAGS += $(PON_CFLAGS)
EXTRA_CFLAGS += -I$(srctree)/drivers/net/ethernet/realtek/rtl86900/romeDriver
EXTRA_CFLAGS += -I$(srctree)/drivers/net
EXTRA_CFLAGS += -DCONFIG_SDK_KERNEL_LINUX

EXTRA_CFLAGS += -I$(srctree)/drivers/net/ethernet/realtek	# for if_smux.h
EXTRA_CFLAGS += -I$(srctree)/drivers/net/ethernet/realtek/rtl86900/romeDriver/asicDriver
#EXTRA_CFLAGS += -I$(APOLLODIR)/tr142/include
#EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/include
#EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/system/include
#EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/src/app/diag_rtk/include
#EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/src/app/omci_v1/API

obj-y += dpi_gw.o

all:

install:

clean:
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions *.symvers *.ko

