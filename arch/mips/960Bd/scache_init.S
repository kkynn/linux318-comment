#define zero $0
#define a0 $4
#define a1 $5
#define a2 $6
#define a3 $7
#define t0 $8 /* caller saved */
#define t1 $9
#define ra $31

#define CP0_MIPS_CONFIG2 $16,2
#define CP0_ITAGLO $28
#define CP0_L23TAGLO $28,4

#define Index_Writeback_Inv_D 0x01
#define Index_Store_Tag_I 0x08
#define Index_Store_Tag_L23 0x0b

#define cacheop(cmd, addr_reg) cache cmd, 0(addr_reg)

#define SECTION_CACHE_FUNC .text

#define GCFUNC(fname, ...) \
	SECTION_CACHE_FUNC;\
	.global fname;\
	.ent fname;\
	.set noreorder;\
	fname: \
	__VA_ARGS__ \
	.set reorder;\
	.end fname;

GCFUNC(initialize_l23,
_init_l23_wb_inv_d:
	lui   t0, 0x8000;
	add   t1, t0, a0;
	sub   t1, t1, a1;
1:
	cacheop(Index_Writeback_Inv_D, t0);
	blt   t0, t1, 1b;
	add   t0, t0, a1;

_init_l23_prep_uncache_jump:
	la    t0, _init_l23_uncache_start;
	lui   t1, 0x2000;
	or    t0, t0, t1;
	j     t0;
	nop;

_init_l23_uncache_start:
_init_l23_disable_l2_bypass:
	mfc0  t0, CP0_MIPS_CONFIG2;
	li    t1, ~(1 << 12);
	and   t0, t0, t1;
	mtc0  t0, CP0_MIPS_CONFIG2;
	nop;

_init_l23_init_l23:
	mtc0  zero, CP0_L23TAGLO;
	mtc0  zero, CP0_ITAGLO;
	lui   t0, 0x8000;
	add   t1, t0, a2;
	sub   t1, t1, a3;
1:
	cacheop(Index_Store_Tag_I, t0);
	cacheop(Index_Store_Tag_L23, t0);
	blt   t0, t1, 1b;
	add   t0, t0, a3;

	sync;
	jr.hb ra;
	nop;
)
