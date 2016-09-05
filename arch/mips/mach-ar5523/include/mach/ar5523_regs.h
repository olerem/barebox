/*
 *  Copyright (C) 2016 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef AR5523_H
#define AR5523_H

#include <asm/addrspace.h>

/* Address Map */
#define AR5523_SDRAM0		0x00000000
#define AR5523_SDRAM1		0x08000000
#define AR5523_WLAN0		0x18000000
#define AR5523_WLAN1		0x18500000
#define AR5523_ENET0		0x18100000
#define AR5523_ENET1		0x18200000
#define AR5523_SDRAMCTL		0x18300000
#define AR5523_FLASHCTL		0x18400000
#define AR5523_APBBASE		0x1c000000
#define AR5523_FLASH		0x1e000000

#define AR5523_UART0		0x10B00000 /* high speed uart */
#define AR5523_UART_SHIFT	2

#endif
