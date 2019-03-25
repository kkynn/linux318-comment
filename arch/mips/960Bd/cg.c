#include "bspchip.h"

typedef struct {
	union {
		struct _dev_freq_s {
			uint16_t cpu0_mhz;
			uint16_t mem_mhz;
			uint16_t lx_mhz;
			uint16_t snof_mhz;
			uint16_t cpu1_mhz;
			uint16_t snaf_mhz;
		} name;
		uint16_t id[sizeof(struct _dev_freq_s)/2];
	} u;
} cg_info_t;

static cg_info_t cg_info;

static uint32_t _cg_lx_freq_from_bdiv(void) {
	uint8_t reg_bak;
	uint32_t freq;

	reg_bak = REG8(0xb800200c);
	REG8(0xb800200c) = 0x83;

	freq = REG8(0xb8002000) | (REG8(0xb8002004) << 8);
	freq = freq*16*115200 / (1000*1000);

	REG8(0xb800200c) = reg_bak;

	return freq;
}

static uint32_t _cg_lx_freq_from_pll(void) {
	prom_printf("II: Missing LX PLL reg. parser\n");
	return 0;
}

static uint32_t _cg_probe_lx(void) {
	uint32_t freq;

	freq = _cg_lx_freq_from_pll();
	if (freq) goto end_of_lx_probe;

	freq = _cg_lx_freq_from_bdiv();
	if (freq) {
		prom_printf("II: Guessing LX freq. from UART: %d MHz\n", freq);
		goto end_of_lx_probe;
	}

	freq = 200;

end_of_lx_probe:
	return freq;
}

uint32_t cg_query_freq(uint32_t dev) {
	return cg_info.u.id[dev];
}

void cg_info_init(void) {
	cg_info.u.name.lx_mhz = _cg_probe_lx();

	cg_info.u.name.cpu0_mhz = 0;
	cg_info.u.name.cpu1_mhz = 0;
	cg_info.u.name.mem_mhz = 0;
	cg_info.u.name.snof_mhz = 0;
	cg_info.u.name.snaf_mhz = 0;
	return;
}
