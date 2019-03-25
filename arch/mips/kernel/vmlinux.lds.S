#include <asm/asm-offsets.h>
#include <asm/thread_info.h>

#define PAGE_SIZE _PAGE_SIZE

/*
 * Put .bss..swapper_pg_dir as the first thing in .bss. This will
 * ensure that it has .bss alignment (64K).
 */
#define BSS_FIRST_SECTIONS *(.bss..swapper_pg_dir)

#include <asm-generic/vmlinux.lds.h>

#undef mips
#define mips mips
OUTPUT_ARCH(mips)
ENTRY(kernel_entry)
PHDRS {
	text PT_LOAD FLAGS(7);	/* RWX */
	note PT_NOTE FLAGS(4);	/* R__ */
}

#ifdef CONFIG_32BIT
	#ifdef CONFIG_CPU_LITTLE_ENDIAN
		jiffies	 = jiffies_64;
	#else
		jiffies	 = jiffies_64 + 4;
	#endif
#else
	jiffies	 = jiffies_64;
#endif

SECTIONS
{
#ifdef CONFIG_BOOT_ELF64
	/* Read-only sections, merged into text segment: */
	/* . = 0xc000000000000000; */

	/* This is the value for an Origin kernel, taken from an IRIX kernel.  */
	/* . = 0xc00000000001c000; */

	/* Set the vaddr for the text segment to a value
	 *   >= 0xa800 0000 0001 9000 if no symmon is going to configured
	 *   >= 0xa800 0000 0030 0000 otherwise
	 */

	/* . = 0xa800000000300000; */
	. = 0xffffffff80300000;
#endif
	. = VMLINUX_LOAD_ADDRESS;
	/* read-only */
	_text = .;	/* Text and read-only data */
	.text : {
		TEXT_TEXT
		/*Realtek LUNA SDK IMEM DECLARATION*/
#ifdef CONFIG_RTK_VOIP
		. = ALIGN(8);
		*(.AllPoleFilter_fixed16s)
		*(.AllZeroFilter_fixed16s)
		*(.getCBvec_fixed16s)

		/* . = ALIGN(ZZ_IRAMSIZE_ZZ); */
		. = ALIGN(CONFIG_IMEM_SIZE);
		__ILBCENC_IMEM_start = .;
		*(.iLBC_Lsp_Azs)
		*(.iLBC_Get_lsp_pols)
		*(.iLBC_crossconvs)
		*(.iLBC_Autocorrs)
		*(.iLBC_Levinsons)
		*(.iCBSearch_hbsrchlps)
		*(.iCBSearch_mainlps)
		*(.iCBSearch_hbsrchstg0s)
		*(.iCBSearch_Loop4s)
		*(.ilbcenc_oper_32bs)
		*(.iLBC_Chebps_1xs)
		*(.searchAugmentedCB_fixeds)
		*(.createAugmentedVec_fixed16s)
		*(.iCBSearch_scal_bufs)
		*(.filteredCBvecs_fixed16s)
		__ILBCENC_IMEM_end = .;

		/* . = ALIGN(ZZ_IRAMSIZE_ZZ); */
		. = ALIGN(CONFIG_IMEM_SIZE);
		__ILBCDEC_IMEM_start = .;
		*(.doThePLC_fixed16s)
		*(.enhancerInterface_fixed16s)
		*(.smath_fixed16s)
		*(.ilbcdec_oper_32bs)
		*(.compCorr_fixed16s)
		*(.hpOutput_fixed16s)
		*(.xCorrCoef_fixed16s)
		*(.iLBC_Iirs)
		*(.iLBC_Syn_filts)
		*(.mycorr1_fixed16s)
		*(.mycorr2_fixed16s)
		*(.refiner_fixed16)
		*(.enh_upsample_fixed16)
		*(.iLBC_decode)
		__ILBCDEC_IMEM_end = .;

#ifdef CONFIG_RTL8672
		/* g729 codec */
		*(.Pre_Process2s)
		*(.Cor_hs)
		*(.D4i40_17_fasts)
		*(.Corr_xy2s)
		*(.imem8K)
		*(.imem16K)
		*(.imem32K)
		*(.utility)
		*(.memset)
		*(.g729_utils)
		*(.dspfuncs)
		*(.Decod_ACELPs)
		*(.Decod_ld8a_sub1s)
		*(.Decod_ld8a_sub2s)
		*(.bit2parm)
		*(.Post_Processs)
		*(.Post_Filters)
		*(.Int_qlpcs)
#endif
#endif
#ifdef CONFIG_IMEM_SIZE
		. = ALIGN(CONFIG_IMEM_SIZE);
#endif
		__iram = . ;
		*(.iram)
#ifdef CONFIG_RTK_VOIP
		__imem_G729_START = .;
		*(.Cor_h_Xs)		/* G.729enc only start*/
		*(.g729_crossconvs)
		*(.Chebps_1xs)		/* G.729enc only end*/
		
		*(.agcs)		/* G.729dec only start*/
		*(.preemphasiss)	/* G.729dec only end*/
		
		*(.g729_crosscorrs)	/* G.729 comm start*/
		*(.Pred_lt_3s)
		*(.filters)		/* G.729 comm end*/


		__imem_G729_END = .;
		
		__imem_common_start = .;
		*(.basic_ops)
		*(.oper_32bs)
		__imem_common_end = .;
#endif

		*(.iram*)
		__iram_end = . ;

		SCHED_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		IRQENTRY_TEXT
		*(.text.*)
		*(.fixup)
		*(.gnu.warning)
	} :text = 0
	/*Realtek LUNA SDK SRAM DECLARATION*/
   	. = ALIGN(32768);
	__sram = .;
	__sram_predata_start = .;
	.sram_predata :
	{
		*(.sram_predata)
	}
	__sram_predata_end = .;
	.sram_text :
	{
		*(.sram_text)
	}
	.sram_data :
	{
		*(.sram_data)
	}
   	. = ALIGN(32768);
   	__sram_end = . ;

	_etext = .;	/* End of text section */

	/*Realtek LUNA SDK DMEM DECLARATION*/
        __dram = . ;
	__dram_start = . ;
	.dram   :
        {
                *(.dram)
		*(.dram*)
        }
   	. = ALIGN(16384);

	__dram_end = . ;

	EXCEPTION_TABLE(16)

	/* Exception table for data bus errors */
	__dbe_table : {
		__start___dbe_table = .;
		*(__dbe_table)
		__stop___dbe_table = .;
	}

	NOTES :text :note
	.dummy : { *(.dummy) } :text

	_sdata = .;			/* Start of data section */
	RODATA

	. = ALIGN(4);
	.nand_spif_probe_func : {
		LS_start_of_nand_spif_probe_func = .;
		*(.nand_spif_probe_func);
		LS_end_of_nand_spif_probe_func = .;
	}

	. = ALIGN(4);
	.snaf_init_rest_func : {
		LS_start_of_snaf_init_rest_func = .;
		KEEP(*(.snaf_init_rest_func));
		LS_end_of_snaf_init_rest_func = .;
	}

	.onfi_probe_func : {
		LS_start_of_nand_onfi_probe_func = .;
		*(.onfi_probe_func);
		LS_end_of_nand_onfi_probe_func = .;
	}

	.nor_spif_probe_func : {
	 LS_start_of_nor_spif_probe_func = .;
	 *(.nor_spif_probe_func);
	 LS_end_of_nor_spif_probe_func = .;
	}

	/* writeable */
	.data : {	/* Data */
		. = . + DATAOFFSET;		/* for CONFIG_MAPPED_KERNEL */

		INIT_TASK_DATA(THREAD_SIZE)
		NOSAVE_DATA
		CACHELINE_ALIGNED_DATA(1 << CONFIG_MIPS_L1_CACHE_SHIFT)
		READ_MOSTLY_DATA(1 << CONFIG_MIPS_L1_CACHE_SHIFT)
		DATA_DATA
		CONSTRUCTORS
	}
	_gp = . + 0x8000;
	.lit8 : {
		*(.lit8)
	}
	.lit4 : {
		*(.lit4)
	}
	/* We want the small data sections together, so single-instruction offsets
	   can access them all, and initialized data all before uninitialized, so
	   we can shorten the on-disk segment size.  */
	.sdata : {
		*(.sdata)
	}
	_edata =  .;			/* End of data section */

	/* will be freed after init */
	. = ALIGN(PAGE_SIZE);		/* Init code and data */
	__init_begin = .;
	INIT_TEXT_SECTION(PAGE_SIZE)


	/* INIT_DATA_SECTION(16)  */

        .init.data : AT(ADDR(.init.data) - LOAD_OFFSET) {               
                INIT_DATA                                              
                /* INIT_SETUP(initsetup_align)                        */
                . = ALIGN(16);                             \
                VMLINUX_SYMBOL(__setup_start) = .;                      
                *(.init.setup)                                          
                VMLINUX_SYMBOL(__setup_end) = .;

                /* INIT_CALLS                                           */
                VMLINUX_SYMBOL(__initcall_start) = .;                   
                *(.initcallearly.init)                                  
                INIT_CALLS_LEVEL(0)                                     
                INIT_CALLS_LEVEL(1)                                     
                INIT_CALLS_LEVEL(2)                                     
                INIT_CALLS_LEVEL(3)                                     
                INIT_CALLS_LEVEL(4)                                     
                INIT_CALLS_LEVEL(5)                                     
                INIT_CALLS_LEVEL(rootfs)                                
                INIT_CALLS_LEVEL(6)                                     
                INIT_CALLS_LEVEL(7)                                     
#ifdef CONFIG_RTK_VOIP
		VMLINUX_SYMBOL(__initcall98_start) = .;
		*(.initcall98.init)
		*(.initcall98a.init) 
		*(.initcall98b.init) 
		*(.initcall98c.init) 
		*(.initcall98d.init) 
		*(.initcall98e.init)
		*(.initcall98f.init) 
		*(.initcall98g.init) 
		*(.initcall98h.init) 
		*(.initcall98i.init) 
		*(.initcall98j.init)
		VMLINUX_SYMBOL(__initcall99_start) = .;
		*(.initcall99.init) 
		*(.initcall99a.init) 
		*(.initcall99b.init) 
		*(.initcall99c.init) 
		*(.initcall99d.init) 
		*(.initcall99e.init)
		*(.initcall99f.init) 
		*(.initcall99g.init)
		*(.initcall99s.init)
#endif
                VMLINUX_SYMBOL(__initcall_end) = .;

                CON_INITCALL                                        
                SECURITY_INITCALL                                  
                INIT_RAM_FS                                       
        }


	. = ALIGN(4);
	.mips.machines.init : AT(ADDR(.mips.machines.init) - LOAD_OFFSET) {
		__mips_machines_start = .;
		*(.mips.machines.init)
		__mips_machines_end = .;
	}

	/* .exit.text is discarded at runtime, not link time, to deal with
	 * references from .rodata
	 */
	.exit.text : {
		EXIT_TEXT
	}
	.exit.data : {
		EXIT_DATA
	}

	PERCPU_SECTION(1 << CONFIG_MIPS_L1_CACHE_SHIFT)
	/*
	 * Align to 64K in attempt to eliminate holes before the
	 * .bss..swapper_pg_dir section at the start of .bss.  This
	 * also satisfies PAGE_SIZE alignment as the largest page size
	 * allowed is 64K.
	 */
	. = ALIGN(0x10000);
	__init_end = .;
	/* freed after init ends here */

#ifdef CONFIG_RTK_VOIP
  /* VoIP DMEM start */
  	. = . + 2k; /* reserve 2k avoid voip stack grow to here. */
   	/*. = ALIGN( MAX(8k, ZZ_DRAMSIZE_ZZ) );*/
	__dmem_start = .;
	/*__gsmfr_dmem_start = .;*/
	/* rtk_voip start*/
	/*.DMEM_GSMFR	:
							{
	*/							/* *(.gsmfr_dummy_topstack) */
	/*						. = . + 4k;
							. = ALIGN(4k);
								*(.gsmfr_dmem_stack1K)
								. = ALIGN(8);
								*(.gsmfr_dmem_bss)
								__gsmfr_dmem_end = .;
							}
	*/
	. = ALIGN( MAX(8k, ZZ_DRAMSIZE_ZZ) );
	__g729_dmem_start = .;
	.DMEM_G729	:
							{
								/* *(.g729_dummy_topstack) */
    							. = . + 4k;
    							. = ALIGN(4k);
								*(.g729_dmem_stack1K)
								. = ALIGN(8);
								*(.g729_dmem_bss.0)
								*(.g729_dmem_bss.1)
								__g729_dmem_end = .;
							}

	/*. = ALIGN( MAX(8k, ZZ_DRAMSIZE_ZZ) );
	__g726_dmem_start = .;
	.DMEM_G726	:
							{
	*/							/* *(.g726_dummy_topstack) */
	/*						. = . + 4k;
							. = ALIGN(4k);
								*(.g726_dmem_stack1K)
								*(.dmem_g726)
								__g726_dmem_end = .;
							}
	*/

	. = ALIGN( MAX(8k, ZZ_DRAMSIZE_ZZ) );
	__lec_dmem_start = .;
	.DMEM_LEC	:
							{
							. = . + 4k;
							. = ALIGN(4k);
								/* *(.lec_dummy_topstack) */
								*(.lec_dmem_stack1K)
								. = ALIGN(8);
								*(.dmem_lec)
								__lec_dmem_end = .;
							}
	. = ALIGN( MAX(8k, ZZ_DRAMSIZE_ZZ) );
	__sys_dmem_start = .;
	.DMEM_SYS	:
							{
							. = . + 4k;
							. = ALIGN(4k);
								/* *(.sys_dummy_topstack) */
								*(.sys_dmem_stack)
								__sys_dmem_end = .;
							}
	/* . = ALIGN(ZZ_DRAMSIZE_ZZ);*/
	. = ALIGN( MAX(8k, ZZ_DRAMSIZE_ZZ) );
	__codec_dmem_start = .;
	.DMEM_CODEC	:
							{
							. = . + 4k;
							. = ALIGN( MIN(4k, ZZ_DRAMSIZE_ZZ));
							__codec_dmem_4k_start = .;
								
								*(.codec_dmem_area)
								__codec_dmem_end = .;
   }
  
	. = ALIGN( 4k );
	__g7231_dmem_start = .;
	.DMEM_G7231	__codec_dmem_4k_start : AT (__g7231_dmem_start)
							{
								*(.g7231_dmem_stack1K)
								. = ALIGN(8);
								*(.g7231_dmem_bss.0)
								__g7231_dmem_end = .;
							}
	. = ADDR(.DMEM_CODEC) + MAX(8k, ZZ_DRAMSIZE_ZZ) + 4k; /* skip overlays address*/

	. = . + 6k; /* reserve 6k avoid ilbc iLBCTmpVct grow here. */

  	__voip_dmem_end = .;
  /* VoIP DMEM end */
#endif

	/*
	 * Force .bss to 64K alignment so that .bss..swapper_pg_dir
	 * gets that alignment.	 .sbss should be empty, so there will be
	 * no holes after __init_end. */
	/* BSS_SECTION(0, 0x10000, 0) */
        . = ALIGN(0);
        VMLINUX_SYMBOL(__bss_start) = .;
        SBSS(0)
        /* BSS(0x10000) */
        . = ALIGN(0x10000);
/*        .bss : AT(ADDR(.bss) - LOAD_OFFSET) { */
	.bss : {
                BSS_FIRST_SECTIONS
                *(.bss..page_aligned)
                *(.dynbss)
		*(.bss)
		*(COMMON)
#ifdef CONFIG_RTK_VOIP
		.  = ALIGN(8);

		*(.g7231_dmem_bss.1)
#endif		
        }
	
        . = ALIGN(0);
        VMLINUX_SYMBOL(__bss_stop) = .;
	

	_end = . ;

	/* These mark the ABI of the kernel for debuggers.  */
	.mdebug.abi32 : {
		KEEP(*(.mdebug.abi32))
	}
	.mdebug.abi64 : {
		KEEP(*(.mdebug.abi64))
	}

	/* This is the MIPS specific mdebug section.  */
	.mdebug : {
		*(.mdebug)
	}

	STABS_DEBUG
	DWARF_DEBUG

	/* These must appear regardless of  .  */
	.gptab.sdata : {
		*(.gptab.data)
		*(.gptab.sdata)
	}
	.gptab.sbss : {
		*(.gptab.bss)
		*(.gptab.sbss)
	}

	/* Sections to be discarded */
	DISCARDS
	/DISCARD/ : {
		/* ABI crap starts here */
		*(.MIPS.options)
		*(.options)
		*(.pdr)
		*(.reginfo)
		*(.eh_frame)
	}
}
