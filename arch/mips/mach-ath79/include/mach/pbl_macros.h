#ifndef __ASM_MACH_ATH79_PBL_MACROS_H
#define __ASM_MACH_ATH79_PBL_MACROS_H

#include <asm/addrspace.h>
#include <asm/regdef.h>
#include <mach/ar71xx_regs.h>

/* start of ar9331 section */

#define PLL_BASE		(KSEG1 | AR71XX_PLL_BASE)
#define PLL_CPU_CONFIG_REG	(PLL_BASE | AR933X_PLL_CPU_CONFIG_REG)
#define PLL_CPU_CONFIG2_REG	(PLL_BASE | AR933X_PLL_CPU_CONFIG2_REG)
#define PLL_CLOCK_CTRL_REG	(PLL_BASE | AR933X_PLL_CLOCK_CTRL_REG)

#define DEF_25MHZ_PLL_CLOCK_CTRL \
				((2 - 1) << AR933X_PLL_CLOCK_CTRL_AHB_DIV_SHIFT \
				| (1 - 1) << AR933X_PLL_CLOCK_CTRL_DDR_DIV_SHIFT \
				| (1 - 1) << AR933X_PLL_CLOCK_CTRL_CPU_DIV_SHIFT)
#define DEF_25MHZ_SETTLE_TIME	(34000 / 40)
#define DEF_25MHZ_PLL_CONFIG	( 1 << AR933X_PLL_CPU_CONFIG_OUTDIV_SHIFT \
				| 1 << AR933X_PLL_CPU_CONFIG_REFDIV_SHIFT \
				| 32 << AR933X_PLL_CPU_CONFIG_NINT_SHIFT)

.macro	pbl_ar9331_pll
	.set	push
	.set	noreorder

	/* Most devices have 25 MHz Ref clock. */
	pbl_reg_writel (DEF_25MHZ_PLL_CLOCK_CTRL | AR933X_PLL_CLOCK_CTRL_BYPASS), \
		PLL_CLOCK_CTRL_REG
	pbl_reg_writel DEF_25MHZ_SETTLE_TIME, PLL_CPU_CONFIG2_REG
	pbl_reg_writel (DEF_25MHZ_PLL_CONFIG | AR933X_PLL_CPU_CONFIG_PLLPWD), \
		PLL_CPU_CONFIG_REG

	/* power on CPU PLL */
	pbl_reg_clr	AR933X_PLL_CPU_CONFIG_PLLPWD, PLL_CPU_CONFIG_REG
	/* disable PLL bypass */
	pbl_reg_clr	AR933X_PLL_CLOCK_CTRL_BYPASS, PLL_CLOCK_CTRL_REG

	pbl_sleep	t2, 40

	.set	pop
.endm

#define DDR_BASE		(KSEG1 | AR71XX_DDR_CTRL_BASE)
#define DDR_CONFIG		(DDR_BASE | AR933X_DDR_CONFIG)
#define DDR_CONFIG2		(DDR_BASE | AR933X_DDR_CONFIG2)
#define DDR_MODE		(DDR_BASE | AR933X_DDR_MODE)
#define DDR_EXT_MODE		(DDR_BASE | AR933X_DDR_EXT_MODE)

#define DDR_CTRL		(DDR_BASE | AR933X_DDR_CTRL)
/* Forces an EMR3S (Extended Mode Register 3 Set) update cycle */
#define DDR_CTRL_EMR3		BIT(5)
/* Forces an EMR2S (Extended Mode Register 2 Set) update cycle */
#define DDR_CTRL_EMR2		BIT(4)
#define DDR_CTRL_PREA		BIT(3) /* Forces a PRECHARGE ALL cycle */
#define DDR_CTRL_REF		BIT(2) /* Forces an AUTO REFRESH cycle */
/* Forces an EMRS (Extended Mode Register 2 Set) update cycle */
#define DDR_CTRL_EMRS		BIT(1)
/* Forces a MRS (Mode Register Set) update cycle */
#define DDR_CTRL_MRS		BIT(0)

#define DDR_REFRESH		(DDR_BASE | AR933X_DDR_REFRESH)
#define DDR_RD_DATA		(DDR_BASE | AR933X_DDR_RD_DATA)
#define DDR_TAP_CTRL0		(DDR_BASE | AR933X_DDR_TAP_CTRL0)
#define DDR_TAP_CTRL1		(DDR_BASE | AR933X_DDR_TAP_CTRL1)

#define DDR_DDR2_CONFIG		(DDR_BASE | AR933X_DDR_DDR_DDR2_CONFIG)
#define DDR_EMR2		(DDR_BASE | AR933X_DDR_DDR_EMR2)
#define DDR_EMR3		(DDR_BASE | AR933X_DDR_DDR_EMR3)

.macro	pbl_ar9331_ddr1_config
	.set	push
	.set	noreorder

	pbl_reg_writel	0x7fbc8cd0, DDR_CONFIG
	pbl_reg_writel	0x9dd0e6a8, DDR_CONFIG2

	pbl_reg_writel	DDR_CTRL_PREA, DDR_CTRL

	/* 0x133: on reset Mode Register value */
	pbl_reg_writel	0x133, DDR_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_CTRL

	/*
	 * DDR_EXT_MODE[1] = 1: Reduced Drive Strength
	 * DDR_EXT_MODE[0] = 0: Enable DLL
	 */
	pbl_reg_writel	0x2, DDR_EXT_MODE
	pbl_reg_writel	DDR_CTRL_EMRS, DDR_CTRL

	pbl_reg_writel	DDR_CTRL_PREA, DDR_CTRL

	/* DLL out of reset, CAS Latency 3 */
	pbl_reg_writel	0x33, DDR_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_CTRL

	/* Refresh control. Bit 14 is enable. Bits<13:0> Refresh time */
	pbl_reg_writel	0x4186, DDR_REFRESH
	/* This register is used along with DQ Lane 0; DQ[7:0], DQS_0 */
	pbl_reg_writel	0x8, DDR_TAP_CTRL0
	/* This register is used along with DQ Lane 1; DQ[15:8], DQS_1 */
	pbl_reg_writel	0x9, DDR_TAP_CTRL1

	/*
	 * DDR read and capture bit mask.
	 * Each bit represents a cycle of valid data.
	 * 0xff: use 16-bit DDR
	 */
	pbl_reg_writel	0xff, DDR_RD_DATA

	.set	pop
.endm

.macro	pbl_ar9331_ddr2_config
	.set	push
	.set	noreorder

	pbl_reg_writel	0x7fbc8cd0, DDR_CONFIG
	pbl_reg_writel	0x9dd0e6a8, DDR_CONFIG2

	/* Enable DDR2 */
	pbl_reg_writel	0x00000a59, DDR_DDR2_CONFIG
	pbl_reg_writel	DDR_CTRL_PREA, DDR_CTRL

	/* Disable High Temperature Self-Refresh Rate */
	pbl_reg_writel	0x00000000, DDR_EMR2
	pbl_reg_writel	DDR_CTRL_EMR2, DDR_CTRL

	pbl_reg_writel	0x00000000, DDR_EMR3
	pbl_reg_writel	DDR_CTRL_EMR3, DDR_CTRL

	/* Enable DLL */
	pbl_reg_writel	0x00000000, DDR_EXT_MODE
	pbl_reg_writel	DDR_CTRL_EMRS, DDR_CTRL

	/* Reset DLL */
	pbl_reg_writel	0x00000100, DDR_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_CTRL

	pbl_reg_writel	DDR_CTRL_PREA, DDR_CTRL
	pbl_reg_writel	DDR_CTRL_REF, DDR_CTRL
	pbl_reg_writel	DDR_CTRL_REF, DDR_CTRL

	/* Write recovery (WR) 6 clock, CAS Latency 3, Burst Length 8 */
	pbl_reg_writel	0x00000a33, DDR_MODE
	pbl_reg_writel	DDR_CTRL_MRS, DDR_CTRL

	/*
	 * DDR_EXT_MODE[9:7] = 0x7: (OCD Calibration defaults)
	 * DDR_EXT_MODE[1] = 1: Reduced Drive Strength
	 * DDR_EXT_MODE[0] = 0: Enable DLL
	 */
	pbl_reg_writel	0x00000382, DDR_EXT_MODE
	pbl_reg_writel	DDR_CTRL_EMRS, DDR_CTRL

	/*
	 * DDR_EXT_MODE[9:7] = 0x0: (OCD exit)
	 * DDR_EXT_MODE[1] = 1: Reduced Drive Strength
	 * DDR_EXT_MODE[0] = 0: Enable DLL
	 */
	pbl_reg_writel	0x00000402, DDR_EXT_MODE
	pbl_reg_writel	DDR_CTRL_EMRS, DDR_CTRL

	/* Refresh control. Bit 14 is enable. Bits <13:0> Refresh time */
	pbl_reg_writel	0x00004186, DDR_REFRESH
	/* DQS 0 Tap Control (needs tuning) */
	pbl_reg_writel	0x00000008, DDR_TAP_CTRL0
	/* DQS 1 Tap Control (needs tuning) */
	pbl_reg_writel	0x00000009, DDR_TAP_CTRL1
	/* For 16-bit DDR */
	pbl_reg_writel	0x000000ff, DDR_RD_DATA

	.set	pop
.endm

#define GPIO_FUNC	((KSEG1 | AR71XX_GPIO_BASE) | AR71XX_GPIO_REG_FUNC)

.macro	pbl_ar9331_uart_enable
	pbl_reg_set AR933X_GPIO_FUNC_UART_EN \
			| AR933X_GPIO_FUNC_RSRV15, GPIO_FUNC
.endm

#define RESET_REG_BOOTSTRAP	((KSEG1 | AR71XX_RESET_BASE) \
					| AR933X_RESET_REG_BOOTSTRAP)

.macro	pbl_ar9331_mdio_gpio_enable
	/* Bit 18 enables MDC and MDIO function on GPIO26 and GPIO28 */
	pbl_reg_set (1 << 18), RESET_REG_BOOTSTRAP
.endm

/* end of ar9331 section */

/* start of ar9344 section */

.macro	pbl_ar9344_pll
	.set	push
	.set	noreorder

	pbl_reg_writel	0x13210f00	0xb81161C4
	pbl_reg_writel	0x03000000	0xb81161C8
	pbl_reg_writel	0x13210f00	0xb8116244
	pbl_reg_writel	0x03000000	0xb8116248
	pbl_reg_writel	0x03000000	0xb8116188

	pbl_reg_writel	0x0130001C	0xb8050008
	pbl_reg_writel	0x0130001C	0xb8050008
	pbl_reg_writel	0x0130001C	0xb8050008

	pbl_reg_writel	0x40021380	0xb8050000
	pbl_reg_writel	0x40815800	0xb8050004
	pbl_reg_writel	0x0130801C	0xb8050008

	pbl_reg_writel	0x10810F00	0xb81161C4
	pbl_reg_writel	0x41C00000	0xb81161C0
	pbl_reg_writel	0xD0810F00	0xb81161C4
	pbl_reg_writel	0x03000000	0xb81161C8
	pbl_reg_writel	0xD0800F00	0xb81161C4

	pbl_reg_writel	0x03000000	0xb81161C8
	pbl_reg_writel	0x43000000	0xb81161C8
	pbl_reg_writel	0x030003E8	0xb81161C8

	pbl_reg_writel	0x10810F00	0xb8116244
	pbl_reg_writel	0x41680000	0xb8116240
	pbl_reg_writel	0xD0810F00	0xb8116244
	pbl_reg_writel	0x03000000	0xb8116248
	pbl_reg_writel	0xD0800F00	0xb8116244

	pbl_reg_writel	0x03000000	0xb8116248
	pbl_reg_writel	0x43000000	0xb8116248
	pbl_reg_writel	0x03000718	0xb8116248

	pbl_reg_writel	0x01308018	0xb8050008
	pbl_reg_writel	0x01308010	0xb8050008
	pbl_reg_writel	0x01308000	0xb8050008
	pbl_reg_writel	0x78180200	0xb8050044
	pbl_reg_writel	0x41C00000	0xb8050048

	pbl_sleep	t2, 40

	.set	pop
.endm

.macro	pbl_ar9344_ddr_config
	.set	push
	.set	noreorder

	pbl_reg_writel	0x40	0xb8000108
	pbl_reg_writel	0xFF	0xb8000018
	pbl_reg_writel	0x74444444	0xb80000C4
	pbl_reg_writel	0x0222	0xb80000C8
	pbl_reg_writel	0xFFFFF	0xb80000CC

	pbl_reg_writel	0xC7D48CD0	0xb8000000
	pbl_reg_writel	0x9DD0E6A8	0xb8000004

	pbl_reg_writel	0x0E59	0xb80000B8
	pbl_reg_writel	0x9DD0E6A8	0xb8000004

	pbl_reg_writel	0x08	0xb8000010
	pbl_reg_writel	0x08	0xb8000010
	pbl_reg_writel	0x10	0xb8000010
	pbl_reg_writel	0x20	0xb8000010
	pbl_reg_writel	0x02	0xb800000C
	pbl_reg_writel	0x02	0xb8000010

	pbl_reg_writel	0x0133	0xb8000008
	pbl_reg_writel	0x1	0xb8000010
	pbl_reg_writel	0x8	0xb8000010
	pbl_reg_writel	0x8	0xb8000010
	pbl_reg_writel	0x4	0xb8000010
	pbl_reg_writel	0x4	0xb8000010

	pbl_reg_writel	0x33	0xb8000008
	pbl_reg_writel	0x1	0xb8000010

	pbl_reg_writel	0x0382	0xb800000C
	pbl_reg_writel	0x2	0xb8000010
	pbl_reg_writel	0x0402	0xb800000C
	pbl_reg_writel	0x2	0xb8000010

	pbl_reg_writel	0x4270	0xb8000014

	pbl_reg_writel	0x0e	0xb800001C
	pbl_reg_writel	0x0e	0xb8000020
	pbl_reg_writel	0x0e	0xb8000024
	pbl_reg_writel	0x0e	0xb8000028

	pbl_sleep	t2, 40

	.set	pop
.endm

/* end of ar9344 section */

.macro	hornet_mips24k_cp0_setup
	.set push
	.set noreorder

	/*
	 * Clearing CP0 registers - This is generally required for the MIPS-24k
	 * core used by Atheros.
	 */
	mtc0	zero, CP0_INDEX
	mtc0	zero, CP0_ENTRYLO0
	mtc0	zero, CP0_ENTRYLO1
	mtc0	zero, CP0_CONTEXT
	mtc0	zero, CP0_PAGEMASK
	mtc0	zero, CP0_WIRED
	mtc0	zero, CP0_INFO
	mtc0	zero, CP0_COUNT
	mtc0	zero, CP0_ENTRYHI
	mtc0	zero, CP0_COMPARE

	li	t0, ST0_CU0 | ST0_ERL
	mtc0	t0, CP0_STATUS

	mtc0	zero, CP0_CAUSE
	mtc0	zero, CP0_EPC

	li	t0, CONF_CM_UNCACHED
	mtc0	t0, CP0_CONFIG

	mtc0	zero, CP0_LLADDR
	mtc0	zero, CP0_WATCHLO
	mtc0	zero, CP0_WATCHHI
	mtc0	zero, CP0_XCONTEXT
	mtc0	zero, CP0_FRAMEMASK
	mtc0	zero, CP0_DIAGNOSTIC
	mtc0	zero, CP0_DEBUG
	mtc0	zero, CP0_DEPC
	mtc0	zero, CP0_PERFORMANCE
	mtc0	zero, CP0_ECC
	mtc0	zero, CP0_CACHEERR
	mtc0	zero, CP0_TAGLO

	.set	pop
.endm

.macro	hornet_1_1_war
	.set push
	.set noreorder

/*
 * WAR: Hornet 1.1 currently need a reset once we boot to let the resetb has
 *      enough time to stable, so that trigger reset at 1st boot, system team
 *      is investigaing the issue, will remove in short
 */

	li  t7, 0xbd000000
	lw  t8, 0(t7)
	li  t9, 0x12345678

	/* if value of 0xbd000000 != 0x12345678, go to do_reset */
	bne t8, t9, do_reset
	 nop

	li  t9, 0xffffffff
	sw  t9, 0(t7)
	b   normal_path
	 nop

do_reset:
	/* put 0x12345678 into 0xbd000000 */
	sw  t9, 0(t7)

	/* reset register 0x1806001c */
	li  t7, 0xb806001c
	lw  t8, 0(t7)
	/* bit24, fullchip reset */
	li  t9, 0x1000000
	or  t8, t8, t9
	sw  t8, 0(t7)

do_reset_loop:
	b   do_reset_loop
	 nop

normal_path:
	.set	pop
.endm

.macro	pbl_ar9331_wmac_enable
	.set push
	.set noreorder

	/* These three WLAN_RESET will avoid original issue */
	li      t3, 0x03
1:
	li      t0, CKSEG1ADDR(AR71XX_RESET_BASE)
	lw      t1, AR933X_RESET_REG_RESET_MODULE(t0)
	ori     t1, t1, 0x0800
	sw      t1, AR933X_RESET_REG_RESET_MODULE(t0)
	nop
	lw      t1, AR933X_RESET_REG_RESET_MODULE(t0)
	li      t2, 0xfffff7ff
	and     t1, t1, t2
	sw      t1, AR933X_RESET_REG_RESET_MODULE(t0)
	nop
	addi    t3, t3, -1
	bnez    t3, 1b
	nop

	li      t2, 0x20
2:
	beqz    t2, 1b
	nop
	addi    t2, t2, -1
	lw      t5, AR933X_RESET_REG_BOOTSTRAP(t0)
	andi    t1, t5, 0x10
	bnez    t1, 2b
	nop

	li      t1, 0x02110E
	sw      t1, AR933X_RESET_REG_BOOTSTRAP(t0)
	nop

	/* RTC Force Wake */
	li      t0, CKSEG1ADDR(AR71XX_RTC_BASE)
	li      t1, 0x03
	sw      t1, AR933X_RTC_REG_FORCE_WAKE(t0)
	nop
	nop

	/* RTC Reset */
	li      t1, 0x00
	sw      t1, AR933X_RTC_REG_RESET(t0)
	nop
	nop

	li      t1, 0x01
	sw      t1, AR933X_RTC_REG_RESET(t0)
	nop
	nop

	/* Wait for RTC in on state */
1:
	lw      t1, AR933X_RTC_REG_STATUS(t0)
	andi    t1, t1, 0x02
	beqz    t1, 1b
	nop

	.set	pop
.endm

#endif /* __ASM_MACH_ATH79_PBL_MACROS_H */
