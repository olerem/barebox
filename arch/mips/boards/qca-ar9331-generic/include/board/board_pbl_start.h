/*
 * Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	pbl_blt 0xbf000000 skip_pll_ram_config t8

	hornet_mips24k_cp0_setup

	pbl_ar9331_wmac_enable

	hornet_1_1_war

	pbl_ar9331_pll
	pbl_ar9331_ram_generic_config

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable

skip_pll_ram_config:
	pbl_ar9331_uart_enable
	debug_ll_ar9331_init
	mips_nmon

	pbl_ar9331_mdio_gpio_enable

	copy_to_link_location	pbl_start

	.set	pop
	.endm
