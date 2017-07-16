/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_AR9344_DEBUG_LL__
#define __MACH_AR9344_DEBUG_LL__

#include <mach/ar71xx_regs.h>

#define DEBUG_LL_UART_ADDR	KSEG1ADDR(AR934X_UART0_BASE)
#define DEBUG_LL_UART_SHIFT	AR934X_UART0_SHIFT

#define DEBUG_LL_UART_CLK   (40000000)
#define DEBUG_LL_UART_BPS   CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK + (8 * DEBUG_LL_UART_BPS) / (16 * DEBUG_LL_UART_BPS))

#include <asm/debug_ll_ns16550.h>

#endif /* __MACH_AR9344_DEBUG_LL_H__ */
