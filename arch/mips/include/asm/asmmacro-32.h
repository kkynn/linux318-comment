/*
 * asmmacro.h: Assembler macros to make things easier to read.
 *
 * Copyright (C) 1996 David S. Miller (davem@davemloft.net)
 * Copyright (C) 1998, 1999, 2003 Ralf Baechle
 */
#ifndef _ASM_ASMMACRO_32_H
#define _ASM_ASMMACRO_32_H

#include <asm/asm-offsets.h>
#include <asm/regdef.h>
#include <asm/fpregdef.h>
#include <asm/mipsregs.h>

	.macro	fpu_save_single thread tmp=t0
	.set push
	SET_HARDFLOAT
	cfc1	\tmp,  fcr31
	swc1	$f0,  THREAD_FPR0_LS64(\thread)
	swc1	$f1,  THREAD_FPR1_LS64(\thread)
	swc1	$f2,  THREAD_FPR2_LS64(\thread)
	swc1	$f3,  THREAD_FPR3_LS64(\thread)
	swc1	$f4,  THREAD_FPR4_LS64(\thread)
	swc1	$f5,  THREAD_FPR5_LS64(\thread)
	swc1	$f6,  THREAD_FPR6_LS64(\thread)
	swc1	$f7,  THREAD_FPR7_LS64(\thread)
	swc1	$f8,  THREAD_FPR8_LS64(\thread)
	swc1	$f9,  THREAD_FPR9_LS64(\thread)
	swc1	$f10, THREAD_FPR10_LS64(\thread)
	swc1	$f11, THREAD_FPR11_LS64(\thread)
	swc1	$f12, THREAD_FPR12_LS64(\thread)
	swc1	$f13, THREAD_FPR13_LS64(\thread)
	swc1	$f14, THREAD_FPR14_LS64(\thread)
	swc1	$f15, THREAD_FPR15_LS64(\thread)
	swc1	$f16, THREAD_FPR16_LS64(\thread)
	swc1	$f17, THREAD_FPR17_LS64(\thread)
	swc1	$f18, THREAD_FPR18_LS64(\thread)
	swc1	$f19, THREAD_FPR19_LS64(\thread)
	swc1	$f20, THREAD_FPR20_LS64(\thread)
	swc1	$f21, THREAD_FPR21_LS64(\thread)
	swc1	$f22, THREAD_FPR22_LS64(\thread)
	swc1	$f23, THREAD_FPR23_LS64(\thread)
	swc1	$f24, THREAD_FPR24_LS64(\thread)
	swc1	$f25, THREAD_FPR25_LS64(\thread)
	swc1	$f26, THREAD_FPR26_LS64(\thread)
	swc1	$f27, THREAD_FPR27_LS64(\thread)
	swc1	$f28, THREAD_FPR28_LS64(\thread)
	swc1	$f29, THREAD_FPR29_LS64(\thread)
	swc1	$f30, THREAD_FPR30_LS64(\thread)
	swc1	$f31, THREAD_FPR31_LS64(\thread)
	sw	\tmp, THREAD_FCR31(\thread)
	.set pop
	.endm

	.macro	fpu_restore_single thread tmp=t0
	.set push
	SET_HARDFLOAT
	lw	\tmp, THREAD_FCR31(\thread)
	lwc1	$f0,  THREAD_FPR0_LS64(\thread)
	lwc1	$f1,  THREAD_FPR1_LS64(\thread)
	lwc1	$f2,  THREAD_FPR2_LS64(\thread)
	lwc1	$f3,  THREAD_FPR3_LS64(\thread)
	lwc1	$f4,  THREAD_FPR4_LS64(\thread)
	lwc1	$f5,  THREAD_FPR5_LS64(\thread)
	lwc1	$f6,  THREAD_FPR6_LS64(\thread)
	lwc1	$f7,  THREAD_FPR7_LS64(\thread)
	lwc1	$f8,  THREAD_FPR8_LS64(\thread)
	lwc1	$f9,  THREAD_FPR9_LS64(\thread)
	lwc1	$f10, THREAD_FPR10_LS64(\thread)
	lwc1	$f11, THREAD_FPR11_LS64(\thread)
	lwc1	$f12, THREAD_FPR12_LS64(\thread)
	lwc1	$f13, THREAD_FPR13_LS64(\thread)
	lwc1	$f14, THREAD_FPR14_LS64(\thread)
	lwc1	$f15, THREAD_FPR15_LS64(\thread)
	lwc1	$f16, THREAD_FPR16_LS64(\thread)
	lwc1	$f17, THREAD_FPR17_LS64(\thread)
	lwc1	$f18, THREAD_FPR18_LS64(\thread)
	lwc1	$f19, THREAD_FPR19_LS64(\thread)
	lwc1	$f20, THREAD_FPR20_LS64(\thread)
	lwc1	$f21, THREAD_FPR21_LS64(\thread)
	lwc1	$f22, THREAD_FPR22_LS64(\thread)
	lwc1	$f23, THREAD_FPR23_LS64(\thread)
	lwc1	$f24, THREAD_FPR24_LS64(\thread)
	lwc1	$f25, THREAD_FPR25_LS64(\thread)
	lwc1	$f26, THREAD_FPR26_LS64(\thread)
	lwc1	$f27, THREAD_FPR27_LS64(\thread)
	lwc1	$f28, THREAD_FPR28_LS64(\thread)
	lwc1	$f29, THREAD_FPR29_LS64(\thread)
	lwc1	$f30, THREAD_FPR30_LS64(\thread)
	lwc1	$f31, THREAD_FPR31_LS64(\thread)
	ctc1	\tmp, fcr31
	.set pop
	.endm

	.macro	cpu_save_nonscratch thread
	LONG_S	s0, THREAD_REG16(\thread)
	LONG_S	s1, THREAD_REG17(\thread)
	LONG_S	s2, THREAD_REG18(\thread)
	LONG_S	s3, THREAD_REG19(\thread)
	LONG_S	s4, THREAD_REG20(\thread)
	LONG_S	s5, THREAD_REG21(\thread)
	LONG_S	s6, THREAD_REG22(\thread)
	LONG_S	s7, THREAD_REG23(\thread)
	LONG_S	sp, THREAD_REG29(\thread)
	LONG_S	fp, THREAD_REG30(\thread)
	.endm

	.macro	cpu_restore_nonscratch thread
	LONG_L	s0, THREAD_REG16(\thread)
	LONG_L	s1, THREAD_REG17(\thread)
	LONG_L	s2, THREAD_REG18(\thread)
	LONG_L	s3, THREAD_REG19(\thread)
	LONG_L	s4, THREAD_REG20(\thread)
	LONG_L	s5, THREAD_REG21(\thread)
	LONG_L	s6, THREAD_REG22(\thread)
	LONG_L	s7, THREAD_REG23(\thread)
	LONG_L	sp, THREAD_REG29(\thread)
	LONG_L	fp, THREAD_REG30(\thread)
	LONG_L	ra, THREAD_REG31(\thread)
	.endm

#ifdef CONFIG_CPU_HAS_RADIAX
	.macro radiax_save_regs thread tmp0 tmp1 tmp2 tmp3 tmp4 tmp5 tmp6
	.set push
	.set noat
	mfru	\tmp3, $0
	mfru	\tmp4, $1
	mfru	\tmp5, $2
	sw	\tmp3, THREAD_CBS0(\thread)
	sw	\tmp4, THREAD_CBS1(\thread)
	sw	\tmp5, THREAD_CBS2(\thread)
	mfru	\tmp0, $4
	mfru	\tmp1, $5
	mfru	\tmp2, $6
	mfru	\tmp3, $16
	mfru	\tmp4, $17
	mfru	\tmp5, $18
	mfru	\tmp6, $24
	sw	\tmp0, THREAD_CBE0(\thread)
	sw	\tmp1, THREAD_CBE1(\thread)
	sw	\tmp2, THREAD_CBE2(\thread)
	sw	\tmp3, THREAD_LPS0(\thread)
	sw	\tmp4, THREAD_LPE0(\thread)
	sw	\tmp5, THREAD_LPC0(\thread)
	sw	\tmp6, THREAD_MMD(\thread)
	mfa	\tmp1, $1, 8
	mfa	\tmp3, $2, 8
	srl	\tmp4, \tmp1,24
	sw	\tmp4, THREAD_M0LH(\thread)
	srl	\tmp4, \tmp3,24
	sw	\tmp4, THREAD_M0HH(\thread)
	mfa	\tmp0, $5
	mfa	\tmp1, $5,8
	mfa	\tmp2, $6
	mfa	\tmp3, $6,8
	sw	\tmp0, THREAD_M1LL(\thread)
	srl	\tmp4, \tmp1,24
	sw	\tmp4, THREAD_M1LH(\thread)
	sw	\tmp2, THREAD_M1HL(\thread)
	srl	\tmp4, \tmp3,24
	sw	\tmp4, THREAD_M1HH(\thread)
	mfa	\tmp0, $9
	mfa	\tmp1, \tmp1, 8
	mfa	\tmp2, $10
	mfa	\tmp3, \tmp2, 8
	sw	\tmp0, THREAD_M2LL(\thread)
	srl	\tmp4, \tmp1, 24
	sw	\tmp4, THREAD_M2LH(\thread)
	sw	\tmp2, THREAD_M2HL(\thread)
	srl	\tmp4, \tmp3, 24
	sw	\tmp4, THREAD_M2HH(\thread)
	mfa	\tmp0, $13
	mfa	\tmp1, \tmp5, 8
	mfa	\tmp2, $14
	mfa	\tmp3, \tmp6, 8
	sw	\tmp0, THREAD_M3LL(\thread)
	srl	\tmp4, \tmp1, 24
	sw	\tmp4, THREAD_M3LH(\thread)
	sw	\tmp2, THREAD_M3HL(\thread)
	srl	\tmp4, \tmp3, 24
	sw	\tmp4, THREAD_M3HH(\thread)
	.set pop
	.endm

	.macro radiax_restore_regs thread tmp0 tmp1 tmp2 tmp3 tmp4 tmp5 tmp6
	.set push
	.set noat
	lw	\tmp3, THREAD_CBS0(\thread)
	lw	\tmp4, THREAD_CBS1(\thread)
	lw	\tmp5, THREAD_CBS2(\thread)
	mtru	\tmp3, $0
	mtru	\tmp4, $1
	mtru	\tmp5, $2
	lw	\tmp0, THREAD_CBE0(\thread)
	lw	\tmp1, THREAD_CBE1(\thread)
	lw	\tmp2, THREAD_CBE2(\thread)
	lw	\tmp3, THREAD_LPS0(\thread)
	lw	\tmp4, THREAD_LPE0(\thread)
	lw	\tmp5, THREAD_LPC0(\thread)
	lw	\tmp6, THREAD_MMD(\thread)
	mtru	\tmp0, $4
	mtru	\tmp1, $5
	mtru	\tmp2, $6
	mtru	\tmp3, $16
	mtru	\tmp4, $17
	mtru	\tmp5, $18
	mtru	\tmp6, $24
	lw	\tmp0, THREAD_M0LH(\thread)
	lw	\tmp1, THREAD_M0HH(\thread)
	sll	\tmp2, \tmp0, 24
	mta2.g	\tmp2, $1
	sll	\tmp2, \tmp1, 24
	mta2.g	\tmp2, $2
	lw	\tmp0, THREAD_M1LL(\thread)
	lw	\tmp1, THREAD_M1LH(\thread)
	lw	\tmp2, THREAD_M1HL(\thread)
	lw	\tmp3, THREAD_M1HH(\thread)
	mta2	\tmp0, $5
	sll	\tmp4, \tmp1, 24
	mta2.g	\tmp4, $5
	mta2	\tmp2, $6
	sll	\tmp4, \tmp3, 24
	mta2.g	\tmp4, $6
	lw	\tmp0, THREAD_M2LL(\thread)
	lw	\tmp1, THREAD_M2LH(\thread)
	lw	\tmp2, THREAD_M2HL(\thread)
	lw	\tmp3, THREAD_M2HH(\thread)
	mta2	\tmp0, $9
	sll	\tmp4, \tmp1, 24
	mta2.g	\tmp4, $9
	mta2	\tmp2, $10
	sll	\tmp4, \tmp3, 24
	mta2.g	\tmp4, $10
	lw	\tmp0, THREAD_M3LL(\thread)
	lw	\tmp1, THREAD_M3LH(\thread)
	lw	\tmp2, THREAD_M3HL(\thread)
	lw	\tmp3, THREAD_M3HH(\thread)
	mta2	\tmp0, $13
	sll	\tmp4, \tmp1, 24
	mta2.g	\tmp4, $13
	mta2	\tmp2, $14
	sll	\tmp4, \tmp3, 24
	mta2.g	\tmp4, $14
	.set pop
	.endm

	.macro radiax_init_regs
	.set push
	.set noat
	mtru	$0, $0
	mtru	$0, $1
	mtru	$0, $2
	mtru	$0, $4
	mtru	$0, $5
	mtru	$0, $6
	mtru	$0, $16
	mtru	$0, $17
	mtru	$0, $18
	mtru	$0, $24
	mta2	$0, $5
	mta2.g	$0, $5
	mta2	$0, $6
	mta2.g	$0, $6
	mta2	$0, $9
	mta2.g	$0, $9
	mta2	$0, $10
	mta2.g	$0, $10
	mta2	$0, $13
	mta2.g	$0, $13
	mta2	$0, $14
	mta2.g	$0, $14
	.set pop
	.endm

#endif

#endif /* _ASM_ASMMACRO_32_H */
