# Realtek Semiconductor Corp.
#
# bsp/Makefile
#     bsp Makefile
#
# Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
#

CPPFLAGS_vmlinux.lds := $(KBUILD_CFLAGS)

core-y += arch/mips/960Bd/
cflags-y +=	-Iinclude/asm-mips -Iarch/mips/960Bd

load-y += 0x80000000
extra-y += vmlinux.lds
obj-y := prom.o setup.o irq.o timer.o cg.o scache.o scache_init.o

SWP_OBJ := $(shell find -L $(core-y) -name 'swp*.c')
SWP_OBJ := $(notdir $(SWP_OBJ:%c=%o))

obj-$(CONFIG_SMP) += smp.o
obj-$(CONFIG_RTL9601D) += irq_9601d.o
obj-$(CONFIG_RTL9603D) += irq_9603d.o
obj-y += $(SWP_OBJ)
