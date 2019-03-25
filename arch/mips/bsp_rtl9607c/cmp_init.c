#include <linux/version.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <asm/smp-boot.h>
#include <asm/r4kcache.h>
#include <asm/mipsmtregs.h>

#include <asm/gic.h>
#include <asm/uasm.h>
#include "bspcpu.h"
#include "cps.h"
#include "gcmpregs.h"
#include "bspchip.h"


/* 
 * CPC register definitions
 *
 * mips_start_of_header
 *
 * $Id: cpc.h,v 1.1 2009/09/24 04:14:21 chris Exp $
 *
 * mips_end_of_header
 *
 */

// #include <sysdefs.h>
// #include <mips.h>

// #define CPCBASE_DEFAULT		0x1bde0000
#define CPCBASE_DEFAULT		0x1dde0000 /* Alan for Soc */
// #define CPCBASE			KSEG1(CPCBASE_DEFAULT)
#define CPCBASE			CKSEG1ADDR(CPCBASE_DEFAULT)
/* Offsets to major blocks within CPC from CPC base */
#define CPC_GCB_OFS		0x0000 /* Global Control Block */
#define CPC_CLCB_OFS		0x2000 /* Core Local Control Block */
#define CPC_COCB_OFS		0x4000 /* Core Other Control Block */

/* Offsets to individual CPC registers from CPC base */
#define CPCOFS(block,tag,reg) \
	(CPC_##block##_OFS + CPC_##tag##_##reg##_OFS)

#define CPCGCBOFS(reg)		CPCOFS(GCB,GCB,reg)
#define CPCCLCBOFS(reg)		CPCOFS(CLCB,CCB,reg)
#define CPCCOCBOFS(reg)		CPCOFS(COCB,CCB,reg)

/* CPC register access */
#define CPCGCB(reg)		REGP(CPCBASE,CPCGCBOFS(reg))
#define CPCCLCB(reg)		REGP(CPCBASE,CPCCLCBOFS(reg))
#define CPCCOCB(reg)		REGP(CPCBASE,CPCCOCBOFS(reg))

/* GCB registers */
#define CPC_GCB_ACCESS_OFS	0x0000	/* CPC Global Access Privilege */
#define CPC_GCB_SEQDEL_OFS	0x0008	/* CPC Global Sequence Delay Counter */
#define CPC_GCB_RAIL_OFS	0x0010	/* CPC Global Rail Delay Counter */
#define CPC_GCB_RESETLEN_OFS	0x0018	/* CPC Global Reset Width Counter */

/* CPC Core local/Core other control block registers */
#define CPC_CCB_CMD_OFS		0x0000	/* CPC CCB Command */
#define CPC_CCB_STAT_OFS	0x0008	/* CPC CCB Status */
#define CPC_CCB_OTHER_OFS	0x0010	/* CPC CCB Other */


#define _mips_sync() __asm__ __volatile__ ("sync  0x3" : : : "memory")

#define sdbbp() __asm__ __volatile__ ("sdbbp" : : :"memory" )
#define NCPULAUNCH 8
#define MIPS32_MAX_CORES 4
unsigned long _gcmp_base;
#if 0
extern void init_l2_cache(void);
#endif
extern void vpex_busyloop(void);
extern void init_core1(void);
extern void init_vpe1(void);

/************************************************************************
 *                          gic
 ************************************************************************/

/* Local section of GIC
 * This is for local timer to wakeup VPE from wait
 * for MIPS_CMP purpose
 */
static void vpe_local_setup(int numvpes) 
{
    int i;
    u32 timer_interrupt = 5;
    u32 data;

  
    /*
     * If we wanted to be fancy, the interrupt values
     * could be overridden from the environment
     */

    /*
     * Setup the default performance and counter timer
     * interrupts for all VPE's
     */
    for (i = 0; i < numvpes; i++) {
	GICWRITE(GIC_REG(VPE_LOCAL, GIC_VPE_OTHER_ADDR), i);

	/* Are Interrupts locally routable? */
	GICREAD(GIC_REG(VPE_OTHER, GIC_VPE_CTL), data);
	if (data & GIC_VPE_CTL_TIMER_RTBL_MSK) {
	    GICWRITE(GIC_REG(VPE_OTHER, GIC_VPE_TIMER_MAP), 
		     GIC_MAP_TO_PIN_MSK | timer_interrupt);
	    GICWRITE(GIC_REG(VPE_OTHER, GIC_VPE_SMASK), GIC_VPE_SMASK_TIMER_MSK);
	    
        }
    }

    return;
}

int boot_gic_init(void){
     unsigned int gicconfig, numvpes;
     GCMPGCB(GICBA) = GIC_BASE_ADDR | GCMP_GCB_GICBA_EN_MSK;
	
     GICREAD(GIC_REG(SHARED, GIC_SH_CONFIG), gicconfig);
     numvpes = (gicconfig & GIC_SH_CONFIG_NUMVPES_MSK) >> GIC_SH_CONFIG_NUMVPES_SHF;
     numvpes = numvpes + 1;

     /* Using cpu count/compare to wakeup VPE from wait */
     vpe_local_setup(numvpes);
  
    return 0;
}



/************************************************************************
 *                          gcmp
 ************************************************************************/

static void gcmp_requester(int i)
{
  unsigned int type;
  int timeout;

  GCMPCLCB(OTHER) = i << GCMP_CCB_OTHER_CORENUM_SHF;
  type = (GCMPCOCB(CFG) & GCMP_CCB_CFG_IOCUTYPE_MSK) >> GCMP_CCB_CFG_IOCUTYPE_SHF;

  switch (type) {
  case GCMP_CCB_CFG_IOCUTYPE_CPU:

    /* Other cores start up from uboot */
    /*TODO*/
    GCMPCOCB(RESETBASE) = 0x80000000;
    _mips_sync(); /* To avoid instruction hazard */
    if (GCMPCOCB(COHCTL) != 0) {
      /* Should not happen */
      GCMPCOCB(COHCTL) = 0;
    }
    /* allow CPUi access to the GCR registers */
    GCMPGCB(GCSRAP) |= 1 << (i + GCMP_GCB_GCSRAP_CMACCESS_SHF);

    /* start CPUi */
    if (GCMPGCB(CPCSR) & GCMP_GCB_CPCSR_EX_MSK) {
	CPCCLCB(OTHER) = i << 16;
        CPCCOCB(CMD) = 4; /* reset */
    }
    else {
	GCMPCOCB(RESETR) = 0;
    }

    /* Poll the Coherence Control register waiting for it to join the domain */
#define TIMEOUT 1000000*500
    /*
     * TIMEOUT should be independent of platform (LV, FPGA, EMULATION)
     * and may need tuning
     */
    for (timeout = TIMEOUT; (timeout >= 0) && (GCMPCOCB(COHCTL) == 0); timeout--)
      ;
    if (timeout > 0) {
      /* could poll the ready bits for each VPE on this core here... */
    }
    break;

  case GCMP_CCB_CFG_IOCUTYPE_NCIOCU:
    break;

  case GCMP_CCB_CFG_IOCUTYPE_CIOCU:
    GCMPCOCB(COHCTL) = 0xff;
    break;

  default:
    break;
  }
}

static void
gcmp_start_cores(int corenum)
{
    unsigned int ncores, niocus;
    unsigned int nrequesters;

    unsigned int gcr = GCMPGCB(GC);
    int i;

    /*Boot-VPE join Coherence Domain */    
    GCMPCLCB(COHCTL) = 0x0f;
          
    niocus = ((gcr & GCMP_GCB_GC_NUMIOCU_MSK) >> GCMP_GCB_GC_NUMIOCU_SHF);
    ncores = ((gcr & GCMP_GCB_GC_NUMCORES_MSK) >> GCMP_GCB_GC_NUMCORES_SHF) + 1;
    nrequesters = niocus + ncores;


    /* requester 0 is already in the coherency domain */
    for (i = 1; i < ncores && i < corenum; i++) {
      gcmp_requester(i);
    }
    for (i = 0; i < niocus; i++) {
      gcmp_requester(4+i);
    }
}


/************************************************************************
 * VPE0 can just start up VPE1, they are all in Core0
 * It must after decode_cpu_config
 ************************************************************************/
static void inline gcmp_start_vpe1(void){
//       if ((vpemask & VPE1_MASK) && arch_mt) 
  {
	/* Enable the remaining VPE1 on this core */
	asm(
	    ".align 2\n\t"
	    ".set push\n\t"
	    ".set mips32r2\n\t"
	    ".set mt\n\t"
	    "evpe\n\t"
	    "ehb\n\t"
	    ".set pop\n\t"
	    ".align 2"
	    );
    }
}



typedef struct smp_boot cpulaunch_t;
#define  CPULAUNCH_ADDR     SMPBOOT
inline static void
shell_cpu_init( int corenum )
{

     volatile struct smp_boot *cpulaunch = (struct smp_boot *) SMPBOOT;

    /* Clear the handshake locations before kicking off the secondary CPU's */
    memset ((void *)cpulaunch, 0, NCPULAUNCH*sizeof(cpulaunch_t));

    /* Fixup CPU#0 (us!) */
    cpulaunch->flags = SMP_LAUNCH_FREADY|SMP_LAUNCH_FGO|SMP_LAUNCH_FGONE;

    /* If the CPC is present, enable it */
    if (GCMPGCB(CPCSR) & GCMP_GCB_CPCSR_EX_MSK) {
	GCMPGCB(CPCBA) = CPCBASE_DEFAULT | GCMP_GCB_CPCBA_EN_MSK;
    }

    /***Init local interrupt for local timer on GIC ****/   
    boot_gic_init();
   /** init enable L2 cache and init */
   
    /* Start up VPE1 */
    gcmp_start_vpe1();
   
    /*Power up other physical core*/
    gcmp_start_cores(corenum);

    return;
}


static inline void flush_l1cache(void){
  	        unsigned long config1;
	        unsigned int lsize;
		unsigned long icache_size;
		unsigned long dcache_size ;
		unsigned long linesz,sets,ways;
		int i;
		config1 = read_c0_config1();
                /* I-Cache */
		lsize = (config1 >> 19) & 7;//4->32B Line Size
		linesz = lsize ? 2 << lsize : 0;//lineSize = 32B
		sets = 32 << (((config1 >> 22) + 1) & 7);
		ways = 1 + ((config1 >> 16) & 7);
		icache_size = sets * ways * linesz;
                for(i=CKSEG0; i < (CKSEG0 + icache_size);  i +=  linesz) {
		  cache_op(Index_Invalidate_I,i);
		}
			      
                /* D-Cache */
		lsize = (config1 >> 10) & 7;
		linesz = lsize ? 2 << lsize : 0;
		sets = 32 << (((config1 >> 13) + 1) & 7);
		ways = 1 + ((config1 >> 7) & 7);
		dcache_size = sets *  ways * linesz;
                for(i=CKSEG0;  i < (CKSEG0 + dcache_size); i +=  linesz){
		  cache_op(Index_Writeback_Inv_D,i);
		}
		_mips_sync();

}

static inline void flush_l2cache(void){
  	        unsigned long config1;
	        unsigned int lsize;
		unsigned long icache_size;
		unsigned long dcache_size ;
		unsigned long linesz,sets,ways;
		int i;
		config1 = read_c0_config1();
                /* I-Cache */
		lsize = (config1 >> 19) & 7;//4->32B Line Size
		linesz = lsize ? 2 << lsize : 0;//lineSize = 32B
		sets = 32 << (((config1 >> 22) + 1) & 7);
		ways = 1 + ((config1 >> 16) & 7);
		icache_size = sets * ways * linesz;
                for(i=CKSEG0; i < (CKSEG0 + icache_size);  i +=  linesz) {
		  cache_op(Index_Invalidate_I,i);
		}
			      
                /* D-Cache */
		lsize = (config1 >> 10) & 7;
		linesz = lsize ? 2 << lsize : 0;
		sets = 32 << (((config1 >> 13) + 1) & 7);
		ways = 1 + ((config1 >> 7) & 7);
		dcache_size = sets *  ways * linesz;
                for(i=CKSEG0;  i < (CKSEG0 + dcache_size); i +=  linesz){
		  cache_op(Index_Writeback_Inv_D,i);
		}
		_mips_sync();

}


void __init smp_setup_others(void){
  
        /* Core1~3 init */
  	init_core1();
        /* Never Return */  
}

/* only run on CPU0 */
void __init smp_setup_processor_id(void)
{
	unsigned long jump_mask = ~((1 << 28) - 1);
	unsigned int *buf = (unsigned int *)(0x80000000);
	unsigned long handler = (unsigned long) (&smp_setup_others);
        /* First 4k is used by bootup	 */
        char *ptr = (char *) 0x80000000;
	memset(ptr, 0, 4096);
	
	uasm_i_j(&buf, handler & ~jump_mask);
	uasm_i_nop(&buf);
        _gcmp_base = CKSEG1ADDR(GCMP_BASE_ADDR);
        _gic_base = CKSEG1ADDR(GIC_BASE_ADDR);
	/* Flush all cache to memory */
  	flush_l1cache();
	
	init_vpe1();
	shell_cpu_init(MIPS32_MAX_CORES);
     
}

