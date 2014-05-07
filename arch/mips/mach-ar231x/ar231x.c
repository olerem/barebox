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
#include <sizes.h>
#include <asm/memory.h>
#include <io.h>
#include <ns16550.h>
#include <asm/cpu.h>
#include <asm/mipsregs.h>
#include <mach/ar231x_platform.h>
#include <mach/ar2315_regs.h>
#include <mach/ar2312_regs.h>

struct ar231x_board_data ar231x_board;

static int mem_init(void)
{
	u32 memsize, memcfg;
	barebox_set_model("Generic ar231x");
	barebox_set_hostname("ar231x");

	if (IS_AR2312 || IS_AR2313) {
		u32 bank0_ac, bank1_ac;
		memcfg = __raw_readl((char *)KSEG1ADDR(AR2312_MEM_CFG1));
		bank0_ac = (memcfg & MEM_CFG1_AC0) >> MEM_CFG1_AC0_S;
		bank1_ac = (memcfg & MEM_CFG1_AC1) >> MEM_CFG1_AC1_S;
		memsize = (bank0_ac ? (1 << (bank0_ac+1)) : 0)
			+ (bank1_ac ? (1 << (bank1_ac+1)) : 0);
		memsize <<= 20;
	} else {
		memcfg = __raw_readl((char *)KSEG1ADDR(AR2315_MEM_CFG));
		memsize   = 1 + ((memcfg & AR2315_SDRAM_DATA_WIDTH_M) >>
				AR2315_SDRAM_DATA_WIDTH_S);
		memsize <<= 1 + ((memcfg & AR2315_SDRAM_COL_WIDTH_M) >>
				AR2315_SDRAM_COL_WIDTH_S);
		memsize <<= 1 + ((memcfg & AR2315_SDRAM_ROW_WIDTH_M) >>
				AR2315_SDRAM_ROW_WIDTH_S);
		memsize <<= 3;
	}

	mips_add_ram0(memsize);
	return 0;
}
mem_initcall(mem_init);

/*
 * This table is indexed by bits 5..4 of the CLOCKCTL1 register
 * to determine the predevisor value.
 */
static int __initdata CLOCKCTL1_PREDIVIDE_TABLE[4] = { 1, 2, 4, 5 };
static int __initdata PLLC_DIVIDE_TABLE[5] = { 2, 3, 4, 6, 3 };

static unsigned int __init
ar231x_sys_clk(void)
{
	unsigned int clock_ctl1, clock_ctl2, cpuDiv;
	unsigned int pllc_out, pre_divide_select, pre_divisor, multiplier, div;
	unsigned int clkDiv;
	unsigned int predivide_mask, predivide_shift;
	unsigned int multiplier_mask, multiplier_shift;

	switch (ar231x_board.chip_id) {
	case AR2312:
		predivide_mask = AR2312_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2312_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2312_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2312_CLOCKCTL1_MULTIPLIER_SHIFT;

		clock_ctl1 = __raw_readl((char *)KSEG1ADDR(AR2312_CLOCKCTL1));
		if (clock_ctl1 & AR2312_CLOCKCTL1_DOUBLER_MASK)
			div = 2;
		break;
	case AR2313:
		predivide_mask = AR2313_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2313_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2313_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2313_CLOCKCTL1_MULTIPLIER_SHIFT;

		clock_ctl1 = __raw_readl((char *)KSEG1ADDR(AR2312_CLOCKCTL1));
		div = 1;
		break;
	case AR2315:
		predivide_mask = AR2315_PLLC_REF_DIV_M;
		predivide_shift = AR2315_PLLC_REF_DIV_S;
		multiplier_mask = AR2315_PLLC_FDBACK_DIV_M;
		multiplier_shift = AR2315_PLLC_FDBACK_DIV_S;

		clock_ctl1 = __raw_readl((char *)KSEG1ADDR(AR2315_PLLC_CTL));
		div = (clock_ctl1 & AR2315_PLLC_ADD_FDBACK_DIV_M)
			>> AR2315_PLLC_ADD_FDBACK_DIV_S;
		div = (div + 1) * 2;
		break;
	case UNKNOWN:
	default:
		return 0;
	}

	pre_divide_select = (clock_ctl1 & predivide_mask) >> predivide_shift;
	pre_divisor = CLOCKCTL1_PREDIVIDE_TABLE[pre_divide_select];
	multiplier = (clock_ctl1 & multiplier_mask) >> multiplier_shift;

	pllc_out = (40000000 / pre_divisor) * div * multiplier;


	if (IS_AR2312 || IS_AR2313)
		return pllc_out / 4;


	clock_ctl2 = __raw_readl((char *)KSEG1ADDR(AR2315_AMBACLK));
	/* clkm input selected */
	switch(clock_ctl2 & AR2315_CPUCLK_CLK_SEL_M) {
		case 0:
		case 1:
			clkDiv = PLLC_DIVIDE_TABLE[
				(clock_ctl1 & AR2315_PLLC_CLKM_DIV_M)
				>> AR2315_PLLC_CLKM_DIV_S];
			break;
		case 2:
			clkDiv = PLLC_DIVIDE_TABLE[
				(clock_ctl1 & AR2315_PLLC_CLKC_DIV_M)
				>> AR2315_PLLC_CLKC_DIV_S];
			break;
		default:
			pllc_out = 40000000;
			clkDiv = 1;
			break;
	}
	cpuDiv = (clock_ctl2 & AR2315_CPUCLK_CLK_DIV_M)
		>> AR2315_CPUCLK_CLK_DIV_S;
	cpuDiv = cpuDiv * 2 ?: 1;
	return (pllc_out/(clkDiv * cpuDiv));
}

/*
 * shutdown watchdog
 */
static int watchdog_init(void)
{
	pr_debug("Disable watchdog.\n");
	if (IS_AR2312 || IS_AR2313)
		__raw_writeb(AR2312_WD_CTRL_IGNORE_EXPIRATION,
				(char *)KSEG1ADDR(AR2312_WD_CTRL));
	else
		__raw_writel(AR2315_WDC_IGNORE_EXPIRATION,
				(char *)KSEG1ADDR(AR2315_WDC));
	return 0;
}

static void flash_init(void)
{
	u32 ctl, old_ctl;

	/* Configure flash bank 0.
	 * Assume 8M maximum window size on this SoC.
	 * Flash will be aliased if it's smaller
	 */
	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2312_FLASHCTL0));
	ctl = FLASHCTL_E | FLASHCTL_AC_8M | FLASHCTL_RBLE |
			(0x01 << FLASHCTL_IDCY_S) |
			(0x07 << FLASHCTL_WST1_S) |
			(0x07 << FLASHCTL_WST2_S) | (old_ctl & FLASHCTL_MW);

	__raw_writel(ctl, (char *)KSEG1ADDR(AR2312_FLASHCTL0));
	/* Disable other flash banks */
	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2312_FLASHCTL1));
	__raw_writel(old_ctl & ~(FLASHCTL_E | FLASHCTL_AC),
			(char *)KSEG1ADDR(AR2312_FLASHCTL1));

	old_ctl = __raw_readl((char *)KSEG1ADDR(AR2312_FLASHCTL2));
	__raw_writel(old_ctl & ~(FLASHCTL_E | FLASHCTL_AC),
			(char *)KSEG1ADDR(AR2312_FLASHCTL2));

	/* We need to find atheros config. MAC address is there. */
	ar231x_find_config((char *)KSEG1ADDR(AR2312_FLASH +
					     AR2312_MAX_FLASH_SIZE));
}

static int ether_init(void)
{
	static struct resource res[2];
	struct ar231x_eth_platform_data *eth = &ar231x_board.eth_pdata;

	/* MAC address located in atheros config on flash. */
	eth->mac = ar231x_board.config->enet0_mac;

	if (IS_AR2312 || IS_AR2313) {
		res[0].start = KSEG1ADDR(AR2312_ENET1);
		res[1].start = KSEG1ADDR(AR2312_ENET0);
		eth->reset_mac = AR2312_RESET_ENET0 | AR2312_RESET_ENET1;
		eth->reset_phy = AR2312_RESET_EPHY0 | AR2312_RESET_EPHY1;
	} else {
		res[0].start = KSEG1ADDR(AR2315_ENET0);
		res[1].start = KSEG1ADDR(AR2315_ENET0);
		eth->reset_mac = AR2315_RESET_ENET0;
		eth->reset_phy = AR2315_RESET_EPHY0;
	}

	/* Base ETH registers  */
	res[0].end = res[0].start + 0x2000 - 1;
	res[0].flags = IORESOURCE_MEM;
	/* Base PHY registers */
	res[1].end = res[1].start + 0x2000 - 1;
	res[1].flags = IORESOURCE_MEM;

	eth->reset_bit = ar231x_reset_bit;

	add_generic_device_res("ar231x_eth", DEVICE_ID_DYNAMIC, res, 2, eth);
	return 0;
}

static int platform_init(void)
{
	if (IS_AR2312 || IS_AR2313) {
		add_generic_device("ar231x_reset", DEVICE_ID_SINGLE, NULL,
				KSEG1ADDR(AR2312_RESET), 0x4, IORESOURCE_MEM,
				NULL);
		flash_init();
	} else {
		add_generic_device("ar231x_reset", DEVICE_ID_SINGLE, NULL,
				KSEG1ADDR(AR2315_RESET), 0x4, IORESOURCE_MEM,
				NULL);
		add_generic_device("ar2315_sf", DEVICE_ID_DYNAMIC, NULL,
				0xb1300000, 0xc, IORESOURCE_MEM, NULL);
	}

	watchdog_init();
	ether_init();
	return 0;
}
late_initcall(platform_init);

static void ar2312_reset_uart(void)
{
	u32 reset;

	/* reset UART0 */
	reset = __raw_readl((char *)KSEG1ADDR(AR2312_RESET));
	reset = ((reset & ~AR2312_RESET_APB) | AR2312_RESET_UART0);
	__raw_writel(reset, (char *)KSEG1ADDR(AR2312_RESET));

	reset &= ~AR2312_RESET_UART0;
	__raw_writel(reset, (char *)KSEG1ADDR(AR2312_RESET));
}

static struct NS16550_plat serial_plat = {
	.shift = AR2312_UART_SHIFT,
};

static int ar231x_console_init(void)
{
	u32 uart_addr;

	if (IS_AR2312 || IS_AR2313) {
		ar2312_reset_uart();
		uart_addr = AR2312_UART0;
	} else
		uart_addr = AR2315_UART0;

	/* Register the serial port */
	serial_plat.clock = ar231x_sys_clk();
	add_ns16550_device(DEVICE_ID_DYNAMIC, KSEG1ADDR(uart_addr),
			   8 << AR2312_UART_SHIFT,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT,
			   &serial_plat);
	return 0;
}
console_initcall(ar231x_console_init);

static int ar231x_set_chip_id(void)
{
	unsigned int cpuid = read_c0_prid() & PRID_IMP_MASK;

	if (cpuid == PRID_IMP_4KC) {
		u32 devid;
		devid = __raw_readl((char *)KSEG1ADDR(AR2312_REV));
		devid &= AR2312_REV_MAJ;
		devid >>= AR2312_REV_MAJ_S;
		if (devid == AR2312_REV_MAJ_AR2313)
			ar231x_board.chip_id = AR2313;
		else	/* AR5312 and AR2312 */
			ar231x_board.chip_id = AR2312;

	} else if (cpuid == PRID_IMP_4KECR2)
		ar231x_board.chip_id = AR2315;
	else
		ar231x_board.chip_id = UNKNOWN;
	return 0;
}
postcore_initcall(ar231x_set_chip_id);
