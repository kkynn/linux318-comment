#define RTK_PLATFORM_NAME "9601D"
#define BSP_INTC_BASE (0xb8003100)
#define RTK_MIPS_RS(rs) ((rs))
/* 0921 slave can't access DCR (0xb8001004).
	 It relies on loader to put DCR content to BIR, i.e., 0xbfc0001c. */
#define C0DCR (0xbfc0001c)
