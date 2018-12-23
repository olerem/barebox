/*
 * Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#define BOARD_PBL_START start_tplink_mr3020_sram

#include <mach/debug_ll.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/asm.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>
#include <asm/pbl_nmon.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	mips_barebox_10h

	pbl_blt 0xbf000000 skip_pll_ram_config t8

	hornet_mips24k_cp0_setup

	pbl_ar9331_wmac_enable

	hornet_1_1_war

	pbl_ar9331_pll
	pbl_ar9331_ddr1_config

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable

skip_pll_ram_config:
	pbl_ar9331_uart_enable
	debug_ll_ath79_init

	/*
	 * It is amazing but we have to enable MDIO on GPIO
	 * to use GPIO26 for the "WPS" LED and GPIO27 for the "3G" LED.
	 */
	pbl_ar9331_mdio_gpio_enable

	/* No return */
__error:
	b	__error;
	 nop;
