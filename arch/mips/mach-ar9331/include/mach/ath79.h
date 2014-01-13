/*
 *  Atheros AR71XX/AR724X/AR913X common definitions
 *
 *  Copyright (C) 2008-2011 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  Parts of this file are based on Atheros' 2.6.15 BSP
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#ifndef __ASM_MACH_ATH79_H
#define __ASM_MACH_ATH79_H

#include <common.h>
#include <init.h>
#include <sizes.h>
#include <io.h>
#include <asm/memory.h>

#include <mach/ar71xx_regs.h>

enum reset_state {
	SET,
	REMOVE,
};

struct ar933x_eth_platform_data {
	u32 base_reset;
	u32 reset_mac;
	u32 reset_phy;

	u8 *mac;

	void (*reset_bit)(u32 val, enum reset_state state);
};

void ar933x_reset_bit(u32 val, enum reset_state state);

static inline void ath79_pll_wr(unsigned reg, u32 val)
{
	__raw_writel(val, (char *)KSEG1ADDR(AR71XX_PLL_BASE + reg));
}

static inline u32 ath79_pll_rr(unsigned reg)
{
	return __raw_readl((char *)KSEG1ADDR(AR71XX_PLL_BASE + reg));
}

static inline void ath79_reset_wr(unsigned reg, u32 val)
{
	__raw_writel(val, (char *)KSEG1ADDR(AR71XX_RESET_BASE + reg));
}

static inline u32 ath79_reset_rr(unsigned reg)
{
	return __raw_readl((char *)KSEG1ADDR(AR71XX_RESET_BASE + reg));
}

#endif /* __ASM_MACH_ATH79_H */
