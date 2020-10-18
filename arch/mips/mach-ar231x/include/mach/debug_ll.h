/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 */

#ifndef __MACH_AR231X_DEBUG_LL__
#define __MACH_AR231X_DEBUG_LL__

#include <mach/ar2312_regs.h>

#define DEBUG_LL_UART_ADDR		KSEG1ADDR(AR2312_UART0)
#define DEBUG_LL_UART_SHIFT		AR2312_UART_SHIFT
#define DEBUG_LL_UART_DIVISOR		(45000000 / (16 * CONFIG_BAUDRATE))

/** @file
 *  This File contains declaration for early output support
 */
#include <asm/debug_ll_ns16550.h>

#endif /* __MACH_AR231X_DEBUG_LL__ */
