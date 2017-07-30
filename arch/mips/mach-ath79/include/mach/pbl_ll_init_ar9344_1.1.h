#ifndef __ASM_MACH_ATH79_PBL_LL_INIT_AR9344_1_1_H
#define __ASM_MACH_ATH79_PBL_LL_INIT_AR9344_1_1_H

#include <asm/addrspace.h>
#include <asm/regdef.h>
#include <mach/ar7240_soc.h>
#include <mach/ar934x_soc.h>

#define AR7240_PLL_BASE                 AR7240_APB_BASE+0x00050000
#define AR934X_CPU_PLL_CONFIG                 AR7240_PLL_BASE+0x0000
#define AR934X_DDR_PLL_CONFIG                 AR7240_PLL_BASE+0x0004
#define AR934X_CPU_DDR_CLOCK_CONTROL          AR7240_PLL_BASE+0x0008
#define AR934X_DDR_PLL_DITHER                 AR7240_PLL_BASE+0x0044

#define CPU_DPLL3_ADDRESS			0x181161c8

#define	ATH_DDR_COUNT_LOC	0xbd000000
#define	ATH_CPU_COUNT_LOC	0xbd000004

#define DPLL2_ADDRESS_c4			0x181161c4
#define DPLL3_ADDRESS_c8			CPU_DPLL3_ADDRESS
#define DPLL2_ADDRESS_44			0x18116244
#define DPLL3_ADDRESS_48			DDR_DPLL3_ADDRESS
#define DPLL3_ADDRESS_88			0x18116188

#define CPU_PLL_CONFIG_NINT_VAL_40	0x380
#define DDR_PLL_CONFIG_NINT_VAL_40	0x3000
#define CPU_PLL_NFRAC_40			0
#define DDR_PLL_NFRAC_40			0

#define CPU_PLL_CONFIG_NINT_VAL_25	0x580
#define DDR_PLL_CONFIG_NINT_VAL_25	0x4c00
#define CPU_PLL_NFRAC_25			0x659
#define DDR_PLL_NFRAC_25			0x330cc

#define CPU_PLL_DITHER_DITHER_EN_LSB                                 31
#define CPU_PLL_DITHER_DITHER_EN_MASK                                0x80000000
#define CPU_PLL_DITHER_DITHER_EN_SET(x)                              (((x) << CPU_PLL_DITHER_DITHER_EN_LSB) & CPU_PLL_DITHER_DITHER_EN_MASK)

#define CPU_PLL_DITHER_NFRAC_STEP_LSB                                12
#define CPU_PLL_DITHER_NFRAC_STEP_MASK                               0x0003f000
#define CPU_PLL_DITHER_NFRAC_STEP_SET(x)                             (((x) << CPU_PLL_DITHER_NFRAC_STEP_LSB) & CPU_PLL_DITHER_NFRAC_STEP_MASK)

#define CPU_PLL_DITHER_UPDATE_COUNT_LSB                              18
#define CPU_PLL_DITHER_UPDATE_COUNT_MASK                             0x00fc0000
#define CPU_PLL_DITHER_UPDATE_COUNT_SET(x)                           (((x) << CPU_PLL_DITHER_UPDATE_COUNT_LSB) & CPU_PLL_DITHER_UPDATE_COUNT_MASK)

#define DDR_PLL_DITHER_DITHER_EN_LSB                                 31
#define DDR_PLL_DITHER_DITHER_EN_MASK                                0x80000000
#define DDR_PLL_DITHER_DITHER_EN_SET(x)                              (((x) << DDR_PLL_DITHER_DITHER_EN_LSB) & DDR_PLL_DITHER_DITHER_EN_MASK)

#define DDR_PLL_DITHER_NFRAC_STEP_LSB                                20
#define DDR_PLL_DITHER_NFRAC_STEP_MASK                               0x07f00000
#define DDR_PLL_DITHER_NFRAC_STEP_SET(x)                             (((x) << DDR_PLL_DITHER_NFRAC_STEP_LSB) & DDR_PLL_DITHER_NFRAC_STEP_MASK)

#define DDR_PLL_DITHER_UPDATE_COUNT_LSB                              27
#define DDR_PLL_DITHER_UPDATE_COUNT_MASK                             0x78000000
#define DDR_PLL_DITHER_UPDATE_COUNT_SET(x)                           (((x) << DDR_PLL_DITHER_UPDATE_COUNT_LSB) & DDR_PLL_DITHER_UPDATE_COUNT_MASK)

#define CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_LSB                     2
#define CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK                    0x00000004
#define CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(x)                  (((x) << CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_LSB) & CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK)

#define CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_LSB                     3
#define CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK                    0x00000008
#define CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(x)                  (((x) << CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_LSB) & CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK)

#define CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_LSB                     4
#define CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK                    0x00000010
#define CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(x)                  (((x) << CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_LSB) & CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK)

/*
 * Helper macros.
 * These Clobber t7, t8 and t9
 */
#define set_val(_reg, _mask, _val)		\
	li	t7,	KSEG1ADDR(_reg);	\
	lw	t8,	0(t7);			\
	li	t9,	~_mask;			\
	and	t8,	t8,	t9;		\
	li	t9,	_val;			\
	or	t8,	t8,	t9;		\
	sw	t8,	0(t7)

#define cpu_pll_set(_mask, _val)	\
	set_val(AR934X_CPU_PLL_CONFIG, _mask, _val)

#define ddr_pll_set(_mask, _val)	\
	set_val(AR934X_DDR_PLL_CONFIG, _mask, _val)

#define cpu_ddr_control_set(_mask, _val)	\
	set_val(AR934X_CPU_DDR_CLOCK_CONTROL, _mask, _val)

#define set_bb_pll(reg, val)		\
	li	t7,	KSEG1ADDR(reg);	\
	li	t8,	val;		\
	sw	t8,	0(t7);

#define set_srif_pll(reg, val)		\
	li	t7,	KSEG1ADDR(reg);	\
	li	t8,	val;		\
	sw	t8,	0(t7);

#define set_srif_pll_reg(reg, _r)	\
	li	t7,	KSEG1ADDR(reg);	\
	sw	_r,	0(t7);

#define inc_loop_count(loc)		\
	li	t9,	loc;		\
	lw	t7,	0(t9);		\
	addi	t7,	t7,	1;	\
	sw	t7,	0(t9);

#define clear_loop_count(loc)	\
	li	t9,	loc;	\
	sw	zero,	0(t9);

/******************************************************************************
 * first level initialization:
 *
 * 0) If clock cntrl reset switch is already set, we're recovering from
 *    "divider reset"; goto 3.
 * 1) Setup divide ratios.
 * 2) Reset.
 * 3) Setup pll's, wait for lock.
 *
 *****************************************************************************/

.macro	ar9344_1_dot_1_ll_init
	.set	push
	.set	noreorder

	/* mww 0xb81161C4 0x13210f00 OK */
	set_bb_pll(DPLL2_ADDRESS_c4, 0x13210f00);
	/* mww 0xb81161C8 0x03000000 OK */
	set_bb_pll(DPLL3_ADDRESS_c8, 0x03000000);
	/* mww 0xb8116244 0x13210f00 OK */
	set_bb_pll(DPLL2_ADDRESS_44, 0x13210f00);
	/* mww 0xb8116248 0x03000000 OK */
	set_bb_pll(DPLL3_ADDRESS_48, 0x03000000);
	/* mww 0xb8116248 0x03000000 OK */
	set_bb_pll(DPLL3_ADDRESS_88, 0x03000000);

	li	t5,	KSEG1ADDR(WASP_BOOTSTRAP_REG);
	li	t6,	WASP_REF_CLK_25
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	beq	zero,	t6,	setup_ref25_val
	nop

setup_ref40_val:
	li	t5,	CPU_PLL_CONFIG_NINT_VAL_40
	li	t6,	DDR_PLL_CONFIG_NINT_VAL_40
	li	t7,	CPU_PLL_NFRAC_40
	li	t9,	DDR_PLL_NFRAC_40
	b	1f
	nop

setup_ref25_val:
	li	t5,	CPU_PLL_CONFIG_NINT_VAL_25
	li	t6,	DDR_PLL_CONFIG_NINT_VAL_25
	li	t7,	CPU_PLL_NFRAC_25
	li	t9,	DDR_PLL_NFRAC_25

1:
	/* 0x3c1000 */
	li	t4,	(CPU_PLL_DITHER_DITHER_EN_SET(0) | \
			CPU_PLL_DITHER_NFRAC_STEP_SET(1) | \
			CPU_PLL_DITHER_UPDATE_COUNT_SET(0xf));
	or	t4,	t4,	t7

	/* not 0x21000 */
	/* 0x81000 TODO */
	li	t8,	0x21000;
	or	t5,	t5,	t8

	/* not 0x210000 */
	/* 0x810000 TODO */
	li	t8,	0x210000;
	or	t6,	t6,	t8

	/* 0x78100000 */
	li	t3,	(DDR_PLL_DITHER_DITHER_EN_SET(0) | \
			DDR_PLL_DITHER_NFRAC_STEP_SET(1) | \
			DDR_PLL_DITHER_UPDATE_COUNT_SET(0xf));

	or	t3,	t3,	t9

pll_bypass_set:
	/* reg, mask, val  */
	/* 0xb8050008, 0xfffffffb, 0x4 */
	/* mww 0xb8050008 0x0130001C ? */
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(1));
	/* 0xb8050008, 0xfffffff7, 0x8 */
	/* mww 0xb8050008 0x0130001C ? */
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(1));
	/* 0xb8050008, 0xffffffef, 0x10 */
	/* mww 0xb8050008 0x0130001C ? */
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(1));

init_cpu_pll:
	/* 0xb8050000 */
	li	t7,	KSEG1ADDR(AR934X_CPU_PLL_CONFIG);
	/* 0x40000000 | 0x21000 | 0x380 */
	li	t8,	CPU_PLL_CONFIG_PLLPWD_SET(1)
	/* 0x40021380 */
	or	t8,	t8,	t5
	/* mww 0xb8050000 0x40021380 TODO */
	//li	t8,	0x40021380;
	sw	t8,	0(t7);

init_ddr_pll:
	/* 0xb8050004 */
	li	t7,	KSEG1ADDR(AR934X_DDR_PLL_CONFIG);
	li	t8,	DDR_PLL_CONFIG_PLLPWD_SET(1)
	/* 0x40000000 | 0x210000 | 0x3000 ? diff with openocd */
	or	t8,	t8,	t6
	/* 0x40213000 */
	/* mww 0xb8050004 0x40815800 TODO ! */
	sw	t8,	0(t7);

init_ahb_pll:
	pbl_reg_writel	0x0130801C, KSEG1ADDR(AR934X_CPU_DDR_CLOCK_CONTROL)

srif_set:

	/* Use built in values, based on ref clock */
	/* 0xb80600b0 */
	li	t5,	KSEG1ADDR(WASP_BOOTSTRAP_REG);
	/* 0x10 */
	li	t6,	WASP_REF_CLK_25
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	/* jump to 25ref clk */
	beq	zero,	t6,	1f
	nop

	/*		refdiv		nint		nfrac */
	/* 0x41c00000 */
	li	t4,	((0x8 << 27) | (112 << 18) | 0);// cpu freq = (40 MHz refclk/refdiv 8) * Nint
	/* 0x41680000 */
	li	t5,	((0x8 << 27) | (90 << 18) | 0);	// ddr freq = (40 MHz refclk/refdiv 8) * Nint
	b	2f
	nop
1:

	/* 0x29c00000 */
	li	t4,	((0x5 << 27) | (112 << 18) | 0);// cpu freq = (25 MHz refclk/refdiv 5) * Nint
	/* 0x29680000 */
	li	t5,	((0x5 << 27) | (90 << 18) | 0);	// ddr freq = (25 MHz refclk/refdiv 5) * Nint

2:

	/* 0 to 0xbd000004 */
	clear_loop_count(ATH_CPU_COUNT_LOC);

cpu_pll_is_not_locked:

	/* count on  0xbd000004 */
	inc_loop_count(ATH_CPU_COUNT_LOC);

	pbl_reg_writel 0x10810F00, 0xb81161C4

	/* mww 0xb81161C0 0x41C00000 */
	set_srif_pll_reg(0xb81161c0, t4);

	pbl_reg_writel 0xD0810F00, 0xb81161c4
	pbl_reg_writel 0x03000000, 0xb81161c8
	pbl_reg_writel 0xD0800F00, 0xb81161c4

cpu_clear_do_meas1:
	/* 0xb81161c8 */
	li	t7,	KSEG1ADDR(CPU_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	~CPU_DPLL3_DO_MEAS_SET(1)
	and	t8,	t8,	t9
	/* mww 0xb81161C8 0x03000000 OK */
	sw	t8,	0(t7)

cpu_set_do_meas:
	/* 0xb81161c8 */
	li	t7,	KSEG1ADDR(CPU_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	CPU_DPLL3_DO_MEAS_SET(1)
	or	t8,	t8,	t9
	/* mww 0xb81161C8 0x43000000 */
	sw	t8,	0(t7)

	/* 0xb81161cc */
	li	t7,	KSEG1ADDR(CPU_DPLL4_ADDRESS)
cpu_wait_for_meas_done:
	lw	t8,	0(t7)
	/* val & 0x8 */
	andi	t8,	t8,	CPU_DPLL4_MEAS_DONE_SET(1)
	beqz	t8,	cpu_wait_for_meas_done
	nop

cpu_clear_do_meas2:
	/* 0xb81161c8 */
	li	t7,	KSEG1ADDR(CPU_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	~CPU_DPLL3_DO_MEAS_SET(1)
	and	t8,	t8,	t9
	/* mww 0xb81161C8 0x030003E8 probably */
	sw	t8,	0(t7)

cpu_read_sqsum_dvc:
	/* 0xb81161c8 */
	li	t7,	KSEG1ADDR(CPU_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	/* 0x7ffff8 */
	li	t9,	CPU_DPLL3_SQSUM_DVC_MASK
	and	t8,	t8,	t9
	sra	t8,	t8,	CPU_DPLL3_SQSUM_DVC_LSB
	li	t9,	0x40000
	subu	t8,	t8,	t9
	bgez	t8,	cpu_pll_is_not_locked
	nop

	/* DDR */
	/* 0xdb000000 */
	clear_loop_count(ATH_DDR_COUNT_LOC)

ddr_pll_is_not_locked:

	inc_loop_count(ATH_DDR_COUNT_LOC)

	pbl_reg_writel 0x10810F00, 0xb8116244

	/* mww 0xb8116240 0x41680000 OK? */
	set_srif_pll_reg(0xb8116240, t5);

	pbl_reg_writel 0xD0810F00, 0xb8116244
	pbl_reg_writel 0x03000000, 0xb8116248
	pbl_reg_writel 0xD0800F00, 0xb8116244

ddr_clear_do_meas1:
	li	t7,	KSEG1ADDR(DDR_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	~DDR_DPLL3_DO_MEAS_SET(1)
	and	t8,	t8,	t9
	/* mww 0xb8116248 0x03000000 OK */
	sw	t8,	0(t7)


ddr_set_do_meas:
	li	t7,	KSEG1ADDR(DDR_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	DDR_DPLL3_DO_MEAS_SET(1)
	or	t8,	t8,	t9
	/* mww 0xb8116248 0x43000000 OK */
	sw	t8,	0(t7)

	li	t7,	KSEG1ADDR(DDR_DPLL4_ADDRESS)
ddr_wait_for_meas_done:
	lw	t8,	0(t7)
	andi	t8,	t8,	DDR_DPLL4_MEAS_DONE_SET(1)
	beqz	t8,	ddr_wait_for_meas_done
	nop

ddr_clear_do_meas2:
	li	t7,	KSEG1ADDR(DDR_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	~DDR_DPLL3_DO_MEAS_SET(1)
	and	t8,	t8,	t9
	/* mww 0xb8116248 0x03000718 */
	sw	t8,	0(t7)

ddr_read_sqsum_dvc:
	li	t7,	KSEG1ADDR(DDR_DPLL3_ADDRESS)
	lw	t8,	0(t7)
	li	t9,	DDR_DPLL3_SQSUM_DVC_MASK
	and	t8,	t8,	t9
	sra	t8,	t8,	DDR_DPLL3_SQSUM_DVC_LSB
	li	t9,	0x40000
	subu	t8,	t8,	t9
	/**/
	bgez	t8,	ddr_pll_is_not_locked
	nop

pll_bypass_unset:
	/* mww 0xb8050008 0x01308018 */
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(0));
	/* mww 0xb8050008 0x01308010 */
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(0));
	/* mww 0xb8050008 0x01308000 */
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(0));

ddr_pll_dither_unset:
	pbl_reg_writel	0x78180200, KSEG1ADDR(AR934X_DDR_PLL_DITHER)

cpu_pll_dither_unset:
	/* mww 0xb8050048 0x41C00000 OK */
	li	t7,	KSEG1ADDR(AR934X_CPU_PLL_DITHER);
	sw	t4,	0(t7);

	.set	pop
.endm

#define AR9344_DDR_DDR2_CONFIG		AR7240_DDR_CTL_BASE+0xb8
#define CFG_934X_DDR2_EN_TWL_VAL	0x0e59
#define USEC_MULT			1

.macro	pbl_ar9344_ddr2_config
	.set	push
	.set	noreorder

	// 0x180000b8 0x0e59
	pbl_reg_writel	CFG_934X_DDR2_EN_TWL_VAL, AR9344_DDR_DDR2_CONFIG
	pbl_sleep	t2, 100 * USEC_MULT

	// 0x18000010 0x10
	pbl_reg_writel	0x10, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	// 0x18000010 0x20
	pbl_reg_writel	0x20, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	li	t5,	KSEG1ADDR(WASP_BOOTSTRAP_REG);
	li	t6,	BIT(3)
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	beq	zero,	t6,	setup_16bit_1
	nop
setup_32bit_1:
	pbl_reg_writel	BIT(6), AR7240_DDR_CTL_CONFIG
	b	1f
	nop
setup_16bit_1:
	pbl_reg_clr	BIT(6), AR7240_DDR_CTL_CONFIG
1:

	pbl_sleep	t2, 10 * USEC_MULT

#define CFG_934X_DDR2_CONFIG_VAL	0xc7d48cd0
	// 0x18000000
	pbl_reg_writel	CFG_934X_DDR2_CONFIG_VAL, AR7240_DDR_CONFIG
	pbl_sleep	t2, 100 * USEC_MULT

#define CFG_934X_DDR2_CONFIG2_VAL	0x9dd0e6a8
	// 0x18000004
	pbl_reg_writel	CFG_934X_DDR2_CONFIG2_VAL, AR7240_DDR_CONFIG2
	pbl_sleep	t2, 100 * USEC_MULT

	// 0x18000010 0x8
	pbl_reg_writel	0x8, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

#define CFG_934X_DDR2_MODE_VAL_INIT	0x133
	// 0x18000008
	pbl_reg_writel	CFG_934X_DDR2_MODE_VAL_INIT, AR7240_DDR_MODE
	pbl_sleep	t2, 1000 * USEC_MULT

	// 0x18000010 0x1
	pbl_reg_writel	0x1, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

#define CFG_934X_DDR2_EXT_MODE_VAL_INIT	0x382
	// 0x1800000c 0x382
	pbl_reg_writel	CFG_934X_DDR2_EXT_MODE_VAL_INIT, AR7240_DDR_EXT_MODE
	pbl_sleep	t2, 100 * USEC_MULT

	// 0x18000010 0x2
	pbl_reg_writel	0x2, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

#define CFG_934X_DDR2_EXT_MODE_VAL	0x402
	// 0x1800000c
	pbl_reg_writel	CFG_934X_DDR2_EXT_MODE_VAL, AR7240_DDR_EXT_MODE
	pbl_sleep	t2, 100 * USEC_MULT

	// 0x18000010 0x2
	pbl_reg_writel	0x2, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

	// 0x18000010 0x8
	pbl_reg_writel	0x8, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

#define CFG_934X_DDR2_MODE_VAL	0x33
	// 0x18000008
	pbl_reg_writel	CFG_934X_DDR2_MODE_VAL, AR7240_DDR_MODE
	pbl_sleep	t2, 100 * USEC_MULT

	// 0x18000010
	pbl_reg_writel	0x1, AR7240_DDR_CONTROL
	pbl_sleep	t2, 10 * USEC_MULT

#define CFG_DDR_REFRESH_VAL	0x4270
	// 0x18000014
	pbl_reg_writel	CFG_DDR_REFRESH_VAL, AR7240_DDR_REFRESH
	pbl_sleep	t2, 100 * USEC_MULT

#define CFG_934X_DDR2_TAP_VAL	0x10012
	// 0x1800001c
	pbl_reg_writel	CFG_934X_DDR2_TAP_VAL, AR7240_DDR_TAP_CONTROL0
	pbl_reg_writel	CFG_934X_DDR2_TAP_VAL, AR7240_DDR_TAP_CONTROL1


#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32 0xff
#define CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16 0xffff
	li	t5,	KSEG1ADDR(WASP_BOOTSTRAP_REG);
	li	t6,	BIT(3)
	lw	t7,	0(t5);
	and	t6,	t7,	t6
	beq	zero,	t6,	setup_16bit_2
	nop
setup_32bit_2:
	pbl_reg_writel	CFG_934X_DDR2_TAP_VAL, AR7240_DDR_TAP_CONTROL2
	pbl_reg_writel	CFG_934X_DDR2_TAP_VAL, AR7240_DDR_TAP_CONTROL3
	pbl_reg_writel	CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_32, AR7240_DDR_RD_DATA_THIS_CYCLE
	b	1f
	nop
setup_16bit_2:
	pbl_reg_writel	CFG_DDR2_RD_DATA_THIS_CYCLE_VAL_16, AR7240_DDR_RD_DATA_THIS_CYCLE
1:

	pbl_sleep	t2, 100 * USEC_MULT

	// 0x180000c4 0x74444444
	pbl_reg_writel	0x74444444, AR7240_DDR_BURST
	pbl_sleep	t2, 100 * USEC_MULT

	// 0x180000c8 0x222
	pbl_reg_writel	0x222, AR7240_DDR_BURST2
	pbl_sleep	t2, 100 * USEC_MULT

	pbl_reg_writel 0xfffff, AR7240_AHB_MASTER_TIMEOUT
	pbl_sleep	t2, 100 * USEC_MULT

	.set	pop
.endm

#endif /* __ASM_MACH_ATH79_PBL_LL_INIT_AR9344_1_1_H */
