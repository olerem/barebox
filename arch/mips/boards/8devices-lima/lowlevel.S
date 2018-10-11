// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#define BOARD_PBL_START start_8devices_lima

#include <mach/debug_ll_ar9344.h>
#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <mach/pbl_ll_init_qca4531.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	mips_barebox_10h

	debug_ll_ar9344_init

	hornet_mips24k_cp0_setup

	/* test if we are in the SRAM */
	pbl_blt 0xbd000000 1f t8
	b skip_flash_test
	nop
1:
	/* test if we are in the flash */
	pbl_blt 0xbf000000 skip_pll_ram_config t8
skip_flash_test:

	pbl_qca4531_ddr2_550_550_init

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable
skip_pll_ram_config:

	copy_to_link_location	BOARD_PBL_START

	la	a0, __dtb_qca4531_8devices_lima_start;
        la      v0, pbl_main_entry;
        jal     v0;
