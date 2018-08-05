// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#include <mach/debug_ll_mt7688.h>
#include <asm/pbl_macros.h>
#include <mach/pbl_ll_init_mt7688.h>
#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	debug_ll_mt766_init

	debug_ll_outc '1'

	debug_ll_outnl

	mips_nmon

	copy_to_link_location	pbl_start

	.set	pop
	.endm
