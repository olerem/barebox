// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
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

ENTRY_FUNCTION_END
