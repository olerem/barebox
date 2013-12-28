#ifndef __ASM_MACH_AR9331_PBL_MACROS_H
#define __ASM_MACH_AR9331_PBL_MACROS_H

#include <asm/regdef.h>
#include <mach/ar71xx_regs.h>

#define	PLL_CPU_CONFIG_REG	(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_CPU_CONFIG_REG)
#define PLL_CPU_CONFIG2_REG	(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_CPU_CONFIG2_REG)
#define PLL_CLOCK_CTRL_REG	(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_CLOCK_CTRL_REG)
#define PLL_DITHER_FRAC_REG	(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_DITHER_FRAC_REG)
#define PLL_DITHER_REG		(KSEG1 | AR71XX_PLL_BASE | \
		AR933X_PLL_DITHER_REG)

.macro	pbl_ar9331_pll
	.set	push
	.set	noreorder

	/* 25MHz config */
	pbl_reg_writel 0x00018004, PLL_CLOCK_CTRL_REG
	pbl_reg_writel 0x00000352, PLL_CPU_CONFIG2_REG
	pbl_reg_writel 0x40818000, PLL_CPU_CONFIG_REG

	pbl_reg_writel 0x001003e8, PLL_DITHER_FRAC_REG
	pbl_reg_writel 0x00818000, PLL_CPU_CONFIG_REG
	pbl_reg_writel 0x00008000, PLL_CLOCK_CTRL_REG

	pbl_sleep	t2, 40

	.set	pop
.endm


.macro	pbl_ar9331_ram
	.set	push
	.set	noreorder

#if 0
	pbl_reg_writel(0x7fbc8cd0, 0xb8000000)
	pbl_reg_writel(0x9dd0e6a8, 0xb8000004)

	pbl_reg_writel(0x8, 0xb8000010)
	pbl_reg_writel(0x133, 0xb8000008)
	pbl_reg_writel(0x1, 0xb8000010)
	pbl_reg_writel(0x2, 0xb800000c)

	pbl_reg_writel(0x2, 0xb8000010)
	pbl_reg_writel(0x8, 0xb8000010)
	pbl_reg_writel(0x33, 0xb8000008)
	pbl_reg_writel(0x1, 0xb8000010)
	pbl_reg_writel(0x4186, 0xb8000014)
	pbl_reg_writel(0x8, 0xb800001c)

	pbl_reg_writel(0x9, 0xb8000020)
	pbl_reg_writel(0xff, 0xb8000018)
#endif

	.set	pop
.endm
#endif /* __ASM_MACH_AR9331_PBL_MACROS_H */
