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

#define AR5523_WIFI_CLK				40000000

#define HW_AR5523_RESET				0x00c00020
#define BM_AR5523_RST_FLASH			BIT(19)
#define BM_AR5523_RST_GPIO			BIT(18)
#define BM_AR5523_RST_UART0			BIT(17)
#define BM_AR5523_RST_APBDMA			BIT(16)
#define BM_AR5523_RST_USB			BIT(12)
#define BM_AR5523_RST_WLAN_CALIB		BIT(9)
#define BM_AR5523_RST_WLAN_COLD			BIT(8)
#define BM_AR5523_RST_NMI			BIT(6)
#define BM_AR5523_RST_WARM			BIT(5)
#define BM_AR5523_RST_COLD			BIT(4)
#define BM_AR5523_RST_SYS			BIT(0)

/* Clock register section */
#define HW_AR5523_CLK				0x00c00060
#define BM_AR5523_CLK_CPU_CLKSWITCH		BIT(0)
#define BM_AR5523_CLK_CPU_CLKSWITCH		BIT(0)
#define BM_AR5523_CLK_CPU_CLKSWITCH		BIT(0)
#define BM_AR5523_CLK_CPU_CLKSWITCH		BIT(0)
#define BM_AR5523_CLK_CPU_CLKSWITCH		BIT(0)
#define BM_AR5523_CLK_CPU_CLKSWITCH		BIT(0)
/* USB Phy Source Clock Select */
#define BM_AR5523_CLK_USB_PHY_SRC_MASK		(0x3 << 4)
#define BM_AR5523_CLK_USB_PHY_SRC_3MHZ		(0x0 << 4)
#define BM_AR5523_CLK_USB_PHY_SRC_6MHZ		(0x1 << 4)
#define BM_AR5523_CLK_USB_PHY_SRC_12MHZ		(0x2 << 4)
/* should be used only */
#define BM_AR5523_CLK_USB_PHY_SRC_24MHZ		(0x3 << 4)
/* CPU Clock Select */
#define BM_AR5523_CLK_CCS_MASK			(0x7 << 1)
#define BM_AR5523_CLK_CCS_REF			(0x0 << 1)
#define BM_AR5523_CLK_CCS_UART			(0x1 << 1)
#define BM_AR5523_CLK_CCS_INT			(0x2 << 1)
#define BM_AR5523_CLK_CCS_USB_XIN		(0x3 << 1)
#define BM_AR5523_CLK_CCS_USB_PHY		(0x4 << 1)
/* ... some unsupported variants was removed ... */
#define BM_AR5523_CLK_CCS_USB_PHY_PLLDIV	(0x7 << 1)
#define BM_AR5523_CLK_CPU_CLKSWITCH		BIT(0)

#define AR5523_UART0				(0x10b00000 + 3)
#define AR5523_UART_SHIFT			2

#endif
