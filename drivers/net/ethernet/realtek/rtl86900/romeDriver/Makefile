ifeq ($(CONFIG_APOLLO_MP),y)
ifeq ($(CONFIG_RG_G3_SERIES),y)
include $(APOLLODIR)/romeDriver/Makefile.g3
else
include $(APOLLODIR)/romeDriver/Makefile.apollo
endif
endif

ifeq ($(CONFIG_XDSL_NEW_HWNAT_DRIVER),y)
include $(DIR_LINUX)/drivers/net/ethernet/realtek/rtl86900/romeDriver/Makefile.xdsl
endif

