#ifndef __ASM_MACH_AR9331_PBL_MACROS_H
#define __ASM_MACH_AR9331_PBL_MACROS_H

#include <asm/regdef.h>
#include <mach/ar71xx_regs.h>

/*
 * Helper macros.
 * These Clobber t7 and t8
 */
#define pbl_reg_writel(_val, _reg) \
        li t7, _reg; \
        li t8, _val;           \
        sw t8, 0(t7);

.macro	pbl_ar9331_pll
	.set	push
	.set	noreorder

	pbl_reg_writel(0x00018004, 0xb8050008)
	pbl_reg_writel(0x00000352, 0xb8050004)
	pbl_reg_writel(0x40818000, 0xb8050000)

	pbl_reg_writel(0x001003e8, 0xb8050010)
	pbl_reg_writel(0x00818000, 0xb8050000)
	pbl_reg_writel(0x00008000, 0xb8050008)

	pbl_sleep	t2, 40

	.set	pop
.endm


.macro	pbl_ar9331_ram
	.set	push
	.set	noreorder

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

	.set	pop
.endm
#endif /* __ASM_MACH_AR9331_PBL_MACROS_H */
