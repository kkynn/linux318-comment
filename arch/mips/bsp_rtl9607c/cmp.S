#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/cacheops.h>
#include <asm/cpu.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/addrspace.h>
#include <asm/hazards.h>
#include <asm/smp-boot.h>
#include "cps.h"


/********************/
#define SCRLAUNCH       0x00000e00
#define CPULAUNCH       0x00000f00
#define NCPULAUNCH      8
#define LAUNCH_FREADY   1
#define LAUNCH_FGO              2
#define LAUNCH_FGONE    4

#define LAUNCH_PC       0
#define LAUNCH_GP       4
#define LAUNCH_SP       8
#define LAUNCH_A0       12
#define LAUNCH_PAD1     16

#define LAUNCH_FLAGS    28


/*
 * ILEAF - declare leaf routine
 */

/* #define ILEAF(name)  LEAF(name)  */
#define IEND(name)  END(name) 

#define ILEAF(symbol)					\
		.globl	symbol;				\
		.section        ".init.text","ax";      \
		.align	2;				\
		.type	symbol, @function;		\
		.ent	symbol, 0;			\
symbol:		.frame	sp, 0, ra

 
 

/************************************************************************
* init_vpe1
************************************************************************/
#define r9_vpe_num   t1
#define r18_tc_num   t2
#define r21_more_tcs s5
#define r20_more_vpes s6
#define r23_cpu_num   s7
 	.set	push
	.set	noreorder           // Don't allow the assembler to reorder instructions.
	.set	noat                // Don't allow the assembler to use r1(at) for synthetic instr.
ILEAF(init_vpe1)

check_mt_ase:
    mfc0	a0, CP0_CONFIG1	        			// CP0_Config1
    bgez	a0, done_init_vpe1             			// bit 31 sign bit set?
    mfc0	a0, CP0_CONFIG2	        			// CP0_Config2
    bgez	a0, done_init_vpe1             			// bit 31 sign bit set?
    mfc0	a0, CP0_CONFIG3	        			// CP0_Config3
    and	    	a0, (1 << 2)              			// M_Config3MT
    beqz	a0, done_init_vpe1
    nop

has_mt_ase:
    // Every vpe will set up the following to simplify resource initialization.
    mfc0    a0, CP0_TCBIND						// Read CP0 TCBind
    ext	    r9_vpe_num, a0,  0, 4					// Extract CurVPE
    ext	    r18_tc_num, a0, 21, 8					// Extract CurTC
    mfc0    a0, CP0_MVPCONF0						// CP0_MVPConf0
    ext	    r20_more_vpes, a0, 10, 4			                // Extract VPE
    ext	    r21_more_tcs, a0, 0, 8   			                // Extract PTC

start_init_vpe1:
   // Each vpe will need to set up additional TC bound to it. (No rebinding.)

    beqz    r21_more_tcs, done_init_vpe1   // If there is no TC.
    nop
    beqz    r20_more_vpes, done_init_vpe1   // If there is no vpe1 then skip init_vpe1.
    nop

	// This is executing on TC0 bound to VPE0.  Therefore VPEConf0.MVP is set.
	// Enter config mode
	mfc0	v0, CP0_MVPCTL	// read CP0_MVPCtl
	or		v0, (1 << 1)		// set VPC
	mtc0	v0, CP0_MVPCTL	// write CP0_MVPCtl
	ehb

#define a0_NTCS	a0
#define a2_NVPES	a2
#define a3_TC	a3

	// Get number of a3_TC's and VPE's
	mfc0	v0, CP0_MVPCONF0	// read CP0_MVPCONF0
	ext		a0_NTCS, v0, 0, 8	// extract PTC
	ext		a2_NVPES, v0, 10, 4	// extract PVPE

	// Initialise a3_TC's/VPE's
	move	a3_TC, zero
nexttc:
	// Select TCn
	mfc0	v0, CP0_VPECTL	// read CP0_VPECTL
	ins		v0, a3_TC, 0, 8		// insert TargTC
	mtc0	v0, CP0_VPECTL	// write CP0_VPECTL
	ehb

	// Bind a3_TC to next VPE
	beqz	a3_TC, nextvpe	// Don't rebind TC0
	nop

	// Halt a3_TC being configured
	li		v0, 1			    // set Halt bit
	mttc0	v0, CP0_TCHALT	// write CP0_TCHALT
	ehb

	move    v1, a3_TC
	# Bind TC to a VPE
	mftc0   v0, CP0_TCBIND	// Read CP0_TCBind
	ins     v0, v1, 0, 4	// change S_TCBindCurVPE
	mttc0   v0, CP0_TCBIND	// write CP0_TCBind

	// This code checks to see if the a3_TC mumber is
	// less than the VPE number. This ensures
	// in a system with more than 1 VPE and TCs each
	// VPE will have at least one a3_TC bound to it
	// and any TCs left over will be bound to the last a3_TC
	slt		v1, a2_NVPES, a3_TC
	bnez	v1, 2f			// Bind spare a3_TC's to VPElast
	move	v1, a2_NVPES

	// Set XTC for active a3_TC's
	mftc0	v0, CP0_VPECONF0	// read CP0_VPECONF0
	ins		v0, a3_TC, 21, 8 // insert XTC
	mttc0	v0, CP0_VPECONF0	// write CP0_VPECONF0

	move	v1, a3_TC
2:
	// Bind a3_TC to a VPE
	mftc0	v0, CP0_TCBIND	// read CP0_TCBIND
	ins		v0, v1, 0, 4	// insert VPE
	mttc0	v0, CP0_TCBIND	// write CP0_TCBIND

	// Set up TCStatus register:
	// Disable Coprocessor Usable bits
	// Disable MDMX/DSP ASE
	// Clear Dirty a3_TC
	// not dynamically allocatable
	// not allocated
	// Kernel mode
	// interrupt exempt
	// ASID 0
	li		v0, (1 << 10)	// set IXMT
	mttc0	v0, CP0_TCSTATUS	// write CP0_TCSTATUS

	// Initialize the a3_TC's register file
	li v0, 0xdeadbeef
	mttgpr	v0, $1
	mttgpr	v0, $2
	mttgpr	v0, $3
	mttgpr	v0, $4
	mttgpr	v0, $5
	mttgpr	v0, $6
	mttgpr	v0, $7
	mttgpr	v0, $8
	mttgpr	v0, $9
	mttgpr	v0, $10
	mttgpr	v0, $11
	mttgpr	v0, $12
	mttgpr	v0, $13
	mttgpr	v0, $14
	mttgpr	v0, $15
	mttgpr	v0, $16
	mttgpr	v0, $17
	mttgpr	v0, $18
	mttgpr	v0, $19
	mttgpr	v0, $20
	mttgpr	v0, $21
	mttgpr	v0, $22
	mttgpr	v0, $23
	mttgpr	v0, $24
	mttgpr	v0, $25
	mttgpr	v0, $26
	mttgpr	v0, $27
	mttgpr	v0, $28
	mttgpr	v0, $29
	mttgpr	v0, $30
	mttgpr	v0, $31

nextvpe:
	slt		v1, a2_NVPES, a3_TC
	bnez	v1, donevpe		// No more VPE's
	nop

	// Disable multi-threading with a3_TC's
	mftc0	v0, CP0_VPECTL	// read CP0_VPECTL
	ins		v0, zero, 15, 1	// clear TE
	mttc0	v0, CP0_VPECTL	// write CP0_VPECTL

	beqz	a3_TC, 1f
	nop

	// For VPE1..n
	// Clear VPA and set master VPE
	mftc0	v0, CP0_VPECONF0	// read CP0_VPECONF0
	ins		v0, zero, 0, 1	// insert VPA
	or		v0, (1 << 1)	// set MVP
	mttc0	v0, CP0_VPECONF0	// write CP0_VPECONF0

	mfc0	v0, CP0_STATUS	// read CP0_STATUS
	mttc0	v0, CP0_STATUS	// write CP0_Status

	li		v0, 0x12345678
	mttc0	v0, CP0_EPC		// write CP0_EPC

	mttc0	zero, CP0_CAUSE	// write CP0_CAUSE

	mfc0	v0, CP0_CONFIG	// read CP0_CONFIG
	mttc0	v0, CP0_CONFIG	// write CP0_CONFIG
#if 0
 	mftc0	v0, CP0_EBASE	// read CP0_EBASE 
	ext		v0, v0, 0, 10	// extract CPUNum
	mttgpr	v0, r23_cpu_num
#endif
	// vpe1 of each core can execute cached as it's L1 I$ has already been initialized.
    // and the L2$ has been initialized or "disabled" via CCA override.
	la		a1, vpex_busyloop
	ins     a1, zero, 29, 1 // Convert to cached kseg0 address in case we linked to kseg1.
	mttc0   a1, CP0_TCRESTART // write CP0_TCRESTART

	// Yes.. this is undoing all of the work done above... :)
	mftc0	v0, CP0_TCSTATUS	// read CP0_TCSTATUS
	ins	    v0, zero, 10, 1	// insert IXMT
	ori	    v0, (1 << 13)	// set A
	mttc0	v0, CP0_TCSTATUS	// write CP0_TCSTATUS

	mttc0	zero, CP0_TCHALT	// write CP0_TCHALT

	mftc0	v0, CP0_VPECONF0	// read CP0_VPECONF0
	ori	    v0, 1		    // set VPA
	mttc0	v0, CP0_VPECONF0	// write CP0_VPECONF0
1:

donevpe:
	addu	a3_TC, 1
	sltu	v1, a0_NTCS, a3_TC
	beqz	v1, nexttc
	nop

	// Exit config mode
	mfc0	v0, CP0_MVPCTL	// read CP0_MVPCtl
	ins	    v0, zero, 1, 1	// insert VPC
	mtc0	v0, CP0_MVPCTL	// write CP0_MVPCtl
	ehb

	li      t0, GCR_CONFIG_ADDR
	lw      t1, (CORE_LOCAL_CONTROL_BLOCK + GCR_CL_ID) (t0) // Load GCR_CL_ID
	beqz t1, done_init_vpe1
	nop
	evpe
	
	
#undef a0_NTCS
#undef a2_NVPES
#undef a3_TC

done_init_vpe1:
    jr   ra
    nop
IEND(init_vpe1)
.set	pop	




ILEAF(init_core1)
	.set	push
	.set	noreorder
	.set noat
	// Initialize the general purpose registers and any shadow register sets.
	// Although not necessary, register initialization may be useful during boot,
    // debug, and simulation when certain ways of initializing registers may not work
    // (xor rN, rN, rN for example.)

	// Initialize register sets
    li      $1, 0xdeadbeef      // (0xdeadbeef stands out, kseg2 mapped, odd.)

	// Determine how many shadow sets are implemented (in addition to the base register set.)
	// the first time thru the loop it will initialize using $1 set above.
	// At the bottom og the loop, 1 is  subtract from $30
	// and loop back to next_shadow_set to start the next loop and the next lowest set number.
	mfc0	$29, CP0_SRSCTL		// read CP0_SRSCtl
	ext	    $30, $29, 26, 4		// extract HSS

next_shadow_set:
	// set PSS to shadow set to be initialized
	ins	    $29, $30, 6, 4		// insert PSS
	mtc0	$29, CP0_SRSCTL		// write CP0_SRSCtl

	wrpgpr	$1, $1
	wrpgpr	$2, $1
	wrpgpr	$3, $1
	wrpgpr	$4, $1
	wrpgpr	$5, $1
	wrpgpr	$6, $1
	wrpgpr	$7, $1
	wrpgpr	$8, $1
	wrpgpr	$9, $1
	wrpgpr	$10, $1
	wrpgpr	$11, $1
	wrpgpr	$12, $1
	wrpgpr	$13, $1
	wrpgpr	$14, $1
	wrpgpr	$15, $1
	wrpgpr	$16, $1
	wrpgpr	$17, $1
	wrpgpr	$18, $1
	wrpgpr	$19, $1
	wrpgpr	$20, $1
	wrpgpr	$21, $1
	wrpgpr	$22, $1
	wrpgpr	$23, $1
	wrpgpr	$24, $1
	wrpgpr	$25, $1
	wrpgpr	$26, $1
	wrpgpr	$27, $1
	wrpgpr	$28, $1
	wrpgpr	$29, $1
	beqz    $30, done_init_gpr // early exit when we get to set 0 so we don't clobber return in $31
	nop
	wrpgpr	$30, $1
	wrpgpr	$31, $1
	b	next_shadow_set
	add	$30, -1  // Since the code started with the highest set number this decrements to the next lower number


done_init_gpr:


    // Initialize Status
	li	    v1, 0x00400404		// (M_StatusIM | M_StatusERL | M_StatusBEV)
	mtc0	v1, CP0_STATUS		// write CP0_Status

	// Initialize Watch registers if implemented.
	mfc0	v0, CP0_CONFIG1		// read CP0_Config1
	ext	    v1, v0, 3, 1		// extract bit 3 WR (Watch registers implemented)
	beq 	v1, zero, done_wr
	li    	v1, 0x7				// (M_WatchHiI | M_WatchHiR | M_WatchHiW)

	// Clear Watch Status bits and disable watch exceptions
	mtc0	v1, CP0_WATCHHI		// write CP0_WatchHi0
	mfc0	v0, CP0_WATCHHI		// read CP0_WatchHi0
	bgez 	v0, done_wr			// Check for bit 31 (sign bit) for more Watch registers
	mtc0	zero, CP0_WATCHLO	// write CP0_WatchLo0

	mtc0	v1, CP0_WATCHHI, 1	// write CP0_WatchHi1
	mfc0	v0, CP0_WATCHHI, 1	// read CP0_WatchHi1
	bgez 	v0, done_wr			// Check for bit 31 (sign bit) for more Watch registers
	mtc0	zero, CP0_WATCHLO, 1	// write CP0_WatchLo1

	mtc0	v1, CP0_WATCHHI, 2	// write CP0_WatchHi2
	mfc0	v0, CP0_WATCHHI, 2	// read CP0_WatchHi2
	bgez 	v0, done_wr			// Check for bit 31 (sign bit) for more Watch registers
	mtc0	zero, CP0_WATCHLO, 2	// write CP0_WatchLo2

	mtc0	v1, CP0_WATCHHI, 3	// write CP0_WatchHi3
	mfc0	v0, CP0_WATCHHI, 3	// read CP0_WatchHi3
	bgez 	v0, done_wr			// Check for bit 31 (sign bit) for more Watch registers
	mtc0	zero, CP0_WATCHLO, 3	// write CP0_WatchLo3

	mtc0	v1, CP0_WATCHHI, 4	// write CP0_WatchHi4
	mfc0	v0, CP0_WATCHHI, 4	// read CP0_WatchHi4
	bgez 	v0, done_wr			// Check for bit 31 (sign bit) for more Watch registers
	mtc0	zero, CP0_WATCHLO, 4	// write CP0_WatchLo4

	mtc0	v1, CP0_WATCHHI, 5	// write CP0_WatchHi5
	mfc0	v0, CP0_WATCHHI, 5	// read CP0_WatchHi5
	bgez 	v0, done_wr			// Check for bit 31 (sign bit) for more Watch registers
	mtc0	zero, CP0_WATCHLO, 5	// write CP0_WatchLo5

	mtc0	v1, CP0_WATCHHI, 6	// write CP0_WatchHi6
	mfc0	v0, CP0_WATCHHI, 6	// read CP0_WatchHi6
	bgez 	v0, done_wr			// Check for bit 31 (sign bit) for more Watch registers
	mtc0	zero, CP0_WATCHLO, 6	// write CP0_WatchLo6

	mtc0	v1, CP0_WATCHHI, 7	// write CP0_WatchHi7
	mtc0	zero, CP0_WATCHLO, 7	// write CP0_WatchLo7

done_wr:

	// Clear WP bit to avoid watch exception upon user code entry, IV, and software interrupts.
	mtc0	zero, CP0_CAUSE		// write CP0_Cause: Init AFTER init of CP0 WatchHi/Lo registers.

	// Clear timer interrupt. (Count was cleared at the reset vector to allow timing boot.)
	mtc0	zero, CP0_COMPARE	// write C0_Compare


check_for_tlb:
	// Determine if we have a TLB
	mfc0    v1, CP0_CONFIG			// read CP0_Config
	ext	    v1, v1, 7, 3			// extract MT field
	li	    a3, 0x1					// load a 1 to check against
	bne	    v1, a3, done_init_tlb
	mfc0    v0, CP0_CONFIG1			// CP0_Config1
	nop

start_init_tlb:
	// Config1MMUSize == Number of TLB entries - 1
	ext     v1, v0, CFG1_MMUSSHIFT, 6 // extract MMU Size
	mtc0    zero, CP0_ENTRYLO0		// write CP0_EntryLo0
	mtc0    zero, CP0_ENTRYLO1		// write CP0_EntryLo1
	mtc0    zero, CP0_PAGEMASK		// write CP0_PageMask
	mtc0    zero, CP0_WIRED			// write CP0_Wired
	li	    a0, 0x80000000

next_tlb_entry_pair:
	mtc0    v1, CP0_INDEX			// write CP0_Index
	mtc0	a0, CP0_ENTRYHI			// write CP0_EntryHi
	ehb
	tlbwi
	add	    a0, (2<<13)				// Add 8K to the address to avoid TLB conflict with previous entry

	bne		v1, zero, next_tlb_entry_pair
	add	    v1, -1

done_init_tlb:	



	// Can be skipped if using magic simulation cache flush

	// Determine how big the I$ is
	mfc0	v0, CP0_CONFIG1		// read CP0_Config1

	// Isolate I$ Line Size
	ext	    v1, v0, CFG1_ILSHIFT, 3		// extract IL

	// Skip ahead if No I$
	beq	    v1, zero, done_icache
	nop

	li	    a2, 2
	sllv    v1, a2, v1			// Now have true I$ line size in bytes

	ext	    a0, v0, CFG1_ISSHIFT, 3		// extract IS
	li	    a2, 64
	sllv    a0, a2, a0			// I$ Sets per way

	// Config1IA == I$ Assoc - 1
	ext	    a1, v0, CFG1_IASHIFT, 3		// extract IA
	add	    a1, 1

	mul	    a0, a0, a1			// Total number of sets

	lui	    a2, 0x8000			// Get a KSeg0 address for cacheops

	// Clear TagLo/TagHi registers
	mtc0    zero, CP0_TAGLO		// write CP0_ITagLo
	mtc0    zero, CP0_TAGHI		// write CP0_ITagHi

	move    a3, a0

next_icache_tag:
	// Index Store Tag Cache Op
	// Will invalidate the tag entry, clear the lock bit, and clear the LRF bit
	cache   0x8, 0(a2)			// ICIndexStTag
	add	    a3, -1				// Decrement set counter
	bne     a3, zero, next_icache_tag
	add     a2, v1				// Get next line address

done_icache:


write_cp0_config:
	mfc0	v0, CP0_CONFIG	// read CP0_Config
	li	    v1, 5			// CCA for coherent cores
        ins	    v0, v1, 0, 3	// instert K0
	mtc0	v0, CP0_CONFIG	// write CP0_Config
	ehb

//DCACHE
	mfc0	v0, CP0_CONFIG1		// read CP0_Config1

	// Isolate D$ Line Size
	ext	    v1, v0, CFG1_DLSHIFT, 3		// extract DL

	// Skip ahead if No D$
	beq	    v1, zero, done_dcache
	nop

	li	    a2, 2
	sllv	v1, a2, v1			// Now have true D$ line size in bytes

	ext	    a0, v0, CFG1_DSSHIFT, 3		// extract DS
	li	    a2, 64
	sllv	a0, a2, a0	// D$ Sets per way

	// Config1DA == D$ Assoc - 1
	ext	    a1, v0, CFG1_DASHIFT, 3		// extract DA
	add	    a1, 1

	mul	    a0, a0, a1			// Get total number of sets

	lui	    a2, 0x8000		    // Get a KSeg0 address for cacheops

	// Clear TagLo/TagHi registers
	mtc0	zero, CP0_TAGLO		// write CP0_TagLo
	mtc0	zero, CP0_TAGHI		// write CP0_TagHi
	mtc0	zero, CP0_TAGLO, 2	// write CP0_DTagLo
	mtc0	zero, CP0_TAGHI, 2	// write CP0_DTagHi

	move	a3, a0

next_dcache_tag:
	// Index Store Tag Cache Op
	// Will invalidate the tag entry, clear the lock bit, and clear the LRF bit

        cache	    0x9, 0(a2)			// DCIndexStTag
	add	    a3, -1			    // Decrement set counter

	bne	    a3, zero, next_dcache_tag
	add	    a2, v1		    	// Get next line address

done_dcache:
	
join_domain:    
    /* Join domain by all VPE0 */
    la	    a1, GCR_CONFIG_ADDR
    li	    a0, 0x0f					   // Set Coherent domain enable for 4 cores
    sw	    a0, (CORE_LOCAL_CONTROL_BLOCK | GCR_CL_COHERENCE)(a1)	// GCR_CL_COHERENCE
    ehb

    la a2,     init_vpe1   // Set up MT ASE vpe1 to execute this boot code also.
    jalr a2
    nop   
    
    la a2,    vpex_busyloop
    jalr a2
    nop   
    
    j      0x80000000
	nop
	.set	pop
IEND(init_core1)
	.set	push
	.set	noreorder

ILEAF(vpex_busyloop)
        .set noreorder
	mfc0	t2, CP0_EBASE	// read CP0_EBASE 
	ext	t2, t2, 0, 10	// extract CPUNum

1:
        /* For RDC use uncached addr */
 	li	t0,CKSEG1ADDR(CPULAUNCH)
	sll	t1,t2,LOG2CPULAUNCH
	addu	t0,t1

	/*
	 * Set CPU online flag
	 */
	lw	t1,LAUNCH_FLAGS(t0)
	or	t1,LAUNCH_FREADY
	sw	t1,LAUNCH_FLAGS(t0)

	/* enable count interrupt in mask, but don't enable interrupts */
	mfc0	t2,CP0_STATUS
	li	t1,STATUSF_IP7 /* FIXME should calculate dynamically from Config3.ippti */
	or	t1,t2
	ins     t1, zero, 0, 3
	mtc0	t1,CP0_STATUS


waitcode_start:
	/*
	 * Poll CPU go flag
	 */
1:	mfc0	t1,CP0_COUNT
	addu	t1,SMP_LAUNCH_PERIOD
	mtc0	t1,CP0_COMPARE

hwwait:	/* Hardware wait */
	.set	push
	.set	mips32
	wait
	.set	pop
checklaunch:
	lw	t1,LAUNCH_FLAGS(t0)
	and	t1,LAUNCH_FGO
	beqz	t1, 1b
	 nop

	/* Reset the counter and interrupts to give naive clients a chance */
	mtc0	t2,CP0_STATUS
	mfc0	t2,CP0_COUNT
	subu	t2,1
	mtc0	t2,CP0_COMPARE

	/* we're off */	
	lw	t2,LAUNCH_PC(t0)
	lw	gp,LAUNCH_GP(t0)
	lw	sp,LAUNCH_SP(t0)
	lw	a0,LAUNCH_A0(t0)
	move	a1,zero
	move	a2,zero
	move	a3,zero
	ori	t1,LAUNCH_FGONE
	sw	t1,LAUNCH_FLAGS(t0)

        sync
	jr	t2
        nop
	.align  2
IEND(vpex_busyloop)
	.set	pop