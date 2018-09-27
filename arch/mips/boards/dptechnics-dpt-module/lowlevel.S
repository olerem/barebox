/*
 * Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#define BOARD_PBL_START start_dptechnics_dpt_module

#include <asm/pbl_nmon.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>

	.set noreorder
	.section ".text_head_entry"
	.align 4

EXPORT(BOARD_PBL_START)

	ar9331_pbl_generic_start

	stack_setup

	la      v0, pbl_main_entry
	jal     v0
	nop
