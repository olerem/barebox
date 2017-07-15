/*
 * Copyright (C) 2017 Oleksij Rempel <o.rempel@pengutronix.de>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <mach/debug_ll_ar9344.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <mach/pbl_ll_init_ar9344_1.1.h>
#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	debug_ll_ar9344_init

	debug_ll_outc '1'

	hornet_mips24k_cp0_setup

	pbl_blt 0xbf000000 skip_pll_ram_config t8

	debug_ll_outc '2'

	ar9344_1_dot_1_ll_init

	hornet_1_1_war

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable

skip_pll_ram_config:
	debug_ll_outc '3'
	debug_ll_outnl

	mips_nmon

	copy_to_link_location	pbl_start

	.set	pop
	.endm
