/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#define BOARD_PBL_START start_dptechnics_dpt_module_sram

#include <mach/debug_ll.h>
#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	ar9331_pbl_generic_start

ENTRY_FUNCTION_SRAM_END
