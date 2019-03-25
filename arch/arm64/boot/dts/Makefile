# include board specific dts makefile
ifeq ($(CONFIG_ARCH_SHEIPA),y)
dts-dirs += ../../bsp/dts
endif

dts-dirs += apm
dts-dirs += arm
dts-dirs += cavium

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb
