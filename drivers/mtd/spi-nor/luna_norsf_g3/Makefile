EXTRA_CFLAGS = -I$(src) -I$(src)/nor_spi

obj-y += spi_probe.o

norsf_flist := nor_spif_core.o
#norsf_flist += nor_spif_gd.o
#norsf_flist += nor_spif_micron.o
#norsf_flist += nor_spif_mxic.o
#norsf_flist += nor_spif_spansion.o
norsf_flist += nor_spif_winbond.o
norsf_flist := $(addprefix nor_spi/,$(norsf_flist))

obj-y += $(norsf_flist)
