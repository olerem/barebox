// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#include <mach/debug_ll.h>
#include <asm/pbl_macros.h>
//#include <mach/pbl_ll_init_mt7688.h>
#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	debug_ll_ns16550_init

	debug_ll_outc '1'

	/* test if we are in the flash */
	pbl_blt 0xbf000000 skip_pll_ram_config t8
	debug_ll_outc '4'

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable
skip_pll_ram_config:
	debug_ll_outc '6'
	debug_ll_ns16550_outnl

	mips_nmon

	copy_to_link_location	pbl_start

	.set	pop
	.endm
