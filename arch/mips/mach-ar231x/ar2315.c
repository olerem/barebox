/*
 * Based on Linux driver:
 *  Copyright (C) 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *  Copyright (C) 2006 FON Technology, SL.
 *  Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 *  Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 * Ported to Barebox:
 *  Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <ns16550.h>
#include <mach/ar231x_platform.h>
#include <mach/ar2315_regs.h>

struct ar231x_board_data ar231x_board;

/*
 * This table is indexed by bits 5..4 of the CLOCKCTL1 register
 * to determine the predevisor value.
 */
static int __initdata CLOCKCTL1_PREDIVIDE_TABLE[4] = { 1, 2, 4, 5 };
static int __initdata PLLC_DIVIDE_TABLE[5] = { 2, 3, 4, 6, 3 };

static unsigned int __init
ar2315_sys_clk(unsigned int clockCtl)
{
	unsigned int pllcCtrl,cpuDiv;
	unsigned int pllcOut,refdiv,fdiv,divby2;
	unsigned int clkDiv;

	pllcCtrl = __raw_readl((char *)KSEG1ADDR(AR2315_PLLC_CTL));
	refdiv = (pllcCtrl & PLLC_REF_DIV_M) >> PLLC_REF_DIV_S;
	refdiv = CLOCKCTL1_PREDIVIDE_TABLE[refdiv];
	fdiv = (pllcCtrl & PLLC_FDBACK_DIV_M) >> PLLC_FDBACK_DIV_S;
	divby2 = (pllcCtrl & PLLC_ADD_FDBACK_DIV_M) >> PLLC_ADD_FDBACK_DIV_S;
	divby2 += 1;
	pllcOut = (40000000/refdiv)*(2*divby2)*fdiv;


	/* clkm input selected */
	switch(clockCtl & CPUCLK_CLK_SEL_M) {
		case 0:
		case 1:
			clkDiv = PLLC_DIVIDE_TABLE[(pllcCtrl & PLLC_CLKM_DIV_M) >> PLLC_CLKM_DIV_S];
			break;
		case 2:
			clkDiv = PLLC_DIVIDE_TABLE[(pllcCtrl & PLLC_CLKC_DIV_M) >> PLLC_CLKC_DIV_S];
			break;
		default:
			pllcOut = 40000000;
			clkDiv = 1;
			break;
	}
	cpuDiv = (clockCtl & CPUCLK_CLK_DIV_M) >> CPUCLK_CLK_DIV_S;
	cpuDiv = cpuDiv * 2 ?: 1;
	return (pllcOut/(clkDiv * cpuDiv));
}

static inline unsigned int
ar2315_cpu_frequency(void)
{
    return ar2315_sys_clk(__raw_readl((char *)KSEG1ADDR(AR2315_CPUCLK)));
}

static inline unsigned int
ar2315_apb_frequency(void)
{
    return ar2315_sys_clk(__raw_readl((char *)KSEG1ADDR(AR2315_AMBACLK)));
}


#if 0

/*
 * shutdown watchdog
 */
static int watchdog_init(void)
{
	pr_debug("Disable watchdog.\n");
	__raw_writeb(AR2315_WD_CTRL_IGNORE_EXPIRATION,
					(char *)KSEG1ADDR(AR2315_WD_CTRL));
	return 0;
}

static void flash_init(void)
{
	u32 ctl, old_ctl;

	/* Configure flash bank 0.
	 * Assume 8M maximum window size on this SoC.
	 * Flash will be aliased if it's smaller
	 */
	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2315_FLASHCTL0));
	ctl = FLASHCTL_E | FLASHCTL_AC_8M | FLASHCTL_RBLE |
			(0x01 << FLASHCTL_IDCY_S) |
			(0x07 << FLASHCTL_WST1_S) |
			(0x07 << FLASHCTL_WST2_S) | (old_ctl & FLASHCTL_MW);

	__raw_writel(ctl, (char *)KSEG1ADDR(AR2315_FLASHCTL0));
	/* Disable other flash banks */
	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2315_FLASHCTL1));
	__raw_writel(old_ctl & ~(FLASHCTL_E | FLASHCTL_AC),
			(char *)KSEG1ADDR(AR2315_FLASHCTL1));

	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2315_FLASHCTL2));
	__raw_writel(old_ctl & ~(FLASHCTL_E | FLASHCTL_AC),
			(char *)KSEG1ADDR(AR2315_FLASHCTL2));

	/* We need to find atheros config. MAC address is there. */
	ar231x_find_config((char *)KSEG1ADDR(AR2315_FLASH +
					     AR2315_MAX_FLASH_SIZE));
}
#endif

u8 macc[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

static void enable_ethernet(void)
{
	unsigned int mask = AR2315_RESET_ENET0 | AR2315_RESET_EPHY0;
	unsigned int regtmp;
	regtmp = __raw_readl((char *)KSEG1ADDR(AR2315_AHB_ARB_CTL));
	regtmp |= AR2315_ARB_ETHERNET;
	__raw_writel(regtmp, (char *)KSEG1ADDR(AR2315_AHB_ARB_CTL));

	regtmp = __raw_readl((char *)KSEG1ADDR(AR2315_RESET));
	__raw_writel(regtmp | mask, (char *)KSEG1ADDR(AR2315_RESET));
	udelay(10000);

	regtmp = __raw_readl((char *)KSEG1ADDR(AR2315_RESET));
	__raw_writel(regtmp & ~mask, (char *)KSEG1ADDR(AR2315_RESET));
	udelay(10000);

	regtmp = __raw_readl((char *)KSEG1ADDR(AR2315_IF_CTL));
	regtmp |= AR2315_IF_TS_LOCAL;
	__raw_writel(regtmp, (char *)KSEG1ADDR(AR2315_IF_CTL));

	regtmp = __raw_readl((char *)KSEG1ADDR(AR2315_ENDIAN_CTL));
	regtmp &= ~AR2315_CONFIG_ETHERNET;
	__raw_writel(regtmp, (char *)KSEG1ADDR(AR2315_ENDIAN_CTL));
}

static int ether_init(void)
{
	static struct resource res[2];
	struct ar231x_eth_platform_data *eth = &ar231x_board.eth_pdata;

	enable_ethernet();

	/* Base ETH registers  */
	res[0].start = KSEG1ADDR(AR2315_ENET0);
	res[0].end = res[0].start + 0x2000 - 1;
	res[0].flags = IORESOURCE_MEM;
	/* Base PHY registers */
	res[1].start = KSEG1ADDR(AR2315_ENET0);
	res[1].end = res[1].start + 0x2000 - 1;
	res[1].flags = IORESOURCE_MEM;

	/* MAC address located in atheros config on flash. */
	eth->mac = (u8 *)&macc[0];
	//eth->mac = ar231x_board.config->enet0_mac;

	eth->reset_mac = AR2315_RESET_ENET0;
	eth->reset_phy = AR2315_RESET_EPHY0;

	// TODO: 
	eth->reset_bit = ar231x_reset_bit;

	/* FIXME: base_reset should be replaced with reset driver */
	eth->base_reset = KSEG1ADDR(AR2315_RESET);

	add_generic_device_res("ar231x_eth", DEVICE_ID_DYNAMIC, res, 2, eth);
	return 0;
}

static int platform_init(void)
{
	add_generic_device("ar231x_reset", DEVICE_ID_SINGLE, NULL,
			KSEG1ADDR(AR2315_RESET), 0x4,
			IORESOURCE_MEM, NULL);

	add_generic_device("ar2315sf", DEVICE_ID_DYNAMIC, NULL,
		0xb1300000, 0xc, IORESOURCE_MEM, NULL);
//	watchdog_init();
//	flash_init();
	ether_init();
	return 0;
}
late_initcall(platform_init);


static struct NS16550_plat serial_plat = {
	.shift = AR2315_UART_SHIFT,
};

static int ar2315_console_init(void)
{
	u32 reset;

	__raw_writel(AR2315_WDC_IGNORE_EXPIRATION,
			(char *)KSEG1ADDR(AR2315_WDC));

	/* Register the serial port */
	serial_plat.clock = ar2315_apb_frequency();
	add_ns16550_device(DEVICE_ID_DYNAMIC, KSEG1ADDR(AR2315_UART0),
		8 << AR2315_UART_SHIFT, IORESOURCE_MEM_8BIT, &serial_plat);
	return 0;
}
console_initcall(ar2315_console_init);
