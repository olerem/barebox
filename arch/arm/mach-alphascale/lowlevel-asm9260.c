/*
 * (C) Copyright 2014 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/system_info.h>
#include <mach/asm9260-regs.h>
#include <asm/barebox-arm.h>
#include <io.h>

void __naked __noreturn barebox_arm_reset_vector(void)
{
	uint32_t r;

	arm_cpu_lowlevel_init();

	r = get_pc();
	if (r > ASM9260_SRAM_BASE &&
			r < ASM9260_SRAM_BASE + ASM9260_SRAM_SIZE) {
		/* We have two internal RAMs, 8K and 32M. */
		/* Enable EMI clk */
		writel(0x40, 0x80040024);
		/* change default emi clk delay */
		writel(0xa0503, 0x8004034c);
		/* remap internal RAM to 0 */
		writel(ASM9260_MEMORY_BASE_DEFAULT, 0x80700014);
		writel(ASM9260_MEMORY_BASE, 0x8070001c);
		/* set internal type to sdram and size to 32MB */
		writel(0xa, 0x8070005c);
		/* set other maps to 0 */
		writel(0x0, 0x80700054);
		writel(0x0, 0x80700058);
		writel(0x0, 0x80700060);
		/* configure internal SDRAM timing */
		writel(0x024996d9, 0x80700004);
		/* configure Static Memory timing */
		writel(0x00542b4f, 0x80700094);
	}
	arm_setup_stack(ASM9260_MEMORY_BASE + SZ_32M - 8);

	/* add here your qspi function */

	relocate_to_adr(ASM9260_MEMORY_BASE);
	barebox_arm_entry(ASM9260_MEMORY_BASE, SZ_32M, NULL);
}
