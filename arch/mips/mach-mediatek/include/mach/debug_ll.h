// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#ifndef __MACH_MEDIATEK_DEBUG_LL__
#define __MACH_MEDIATEK_DEBUG_LL__


#define DEBUG_LL_UART_ADDR	KSEG1ADDR(0x10000e00)
#define DEBUG_LL_UART_SHIFT	2
/*
 * system clk = 40Mhz
 * HIGH SPEED UART div = 16 (UARTn+0024h HIGH SPEED UART register == 0)
 */
#define DEBUG_LL_UART_CLK       (40000000 / 16)
#define DEBUG_LL_UART_BPS       CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>

#endif /* __MACH_MEDIATEK_DEBUG_LL__ */
