#include <bspchip.h>
#include <bspcpu.h>

#ifndef cpu_icache_line
#	error EE: Unknown cpu_icache_line
#endif

#if !defined(cpu_dcache_line)
#	error EE: Unknwon cpu_dcache_line
#endif

#if !defined(cpu_scache_line)
#	error EE: Unknwon cpu_scache_line
#endif

#if !defined(cpu_icache_size)
#	error EE: Unknown cpu_icache_size
#endif

#if !defined(cpu_dcache_size)
#	error EE: Unknwon cpu_dcache_size
#endif

#if !defined(cpu_scache_size)
#	error EE: Unknwon cpu_scache_size
#endif

#define MAYBE_UNUSED __attribute__((unused))

#define CPU_GET_CONFIG2() \
	({ \
		int __res; \
		__asm__ __volatile__("mfc0 %0, $16, 2" \
		                     : "=r"(__res)); \
		__res; \
	})

#define CPU_SET_CONFIG2(_value) \
	({ \
		__asm__ __volatile__("mtc0 %0, $16, 2" \
		                     : : "r"(_value)); \
	})

extern void initialize_l23(int, int, int, int);

static void __init MAYBE_UNUSED noinline _enable_l23(int config2, int l2b, int sl) {
	if (l2b || sl) {
		initialize_l23(cpu_dcache_size, cpu_dcache_line,
		               cpu_scache_size, cpu_scache_line);
	} else {
		prom_printf("EE: No L2$ found\n");
	}
	return;
}

static void __init MAYBE_UNUSED _bypass_l23(int config2, int l2b, int sl) {
	if ((l2b == 0) && (sl)) {
		prom_printf("EE: L2$ is already enabled; can't bypass it\n");
	} else {
		config2 |= (1 << 12);
		CPU_SET_CONFIG2(config2);
	}
	return;
}

void __init bsp_setup_scache(void) {
	uint32_t config2 = CPU_GET_CONFIG2();
	uint32_t l2b = (config2 >> 12) & 1;
	uint32_t sl = (config2 >> 4) & 0xf;

	prom_printf("II: Original CP0 CONFIG2: %08x\n", config2);

#if defined(CONFIG_MIPS_CPU_SCACHE)
	_enable_l23(config2, l2b, sl);
#else
	_bypass_l23(config2, l2b, sl);
#endif

	prom_printf("II: Configured CP0 CONFIG2: %08x\n", CPU_GET_CONFIG2());
	return;
}
