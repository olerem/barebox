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

ENTRY_FUNCTION(BOARD_PBL_START)

	ar9331_pbl_generic_start

        stack_setup;

	la	a0, __dtb_ar9331_dptechnics_dpt_module_start;
        la      v0, pbl_main_entry;
        jal     v0;
