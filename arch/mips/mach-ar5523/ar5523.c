/*
 *  Copyright (C) 2016 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <platform_data/serial-ns16550.h>
#include <mach/ar5523_regs.h>
#if 0

/*
 * This table is indexed by bits 5..4 of the CLOCKCTL1 register
 * to determine the predevisor value.
 */
static int CLOCKCTL1_PREDIVIDE_TABLE[4] = { 1, 2, 4, 5 };

static unsigned int
ar5523_cpu_frequency(void)
{
	unsigned int predivide_mask, predivide_shift;
	unsigned int multiplier_mask, multiplier_shift;
	unsigned int clock_ctl1, pre_divide_select, pre_divisor, multiplier;
	unsigned int doubler_mask;
	u32 devid;

	devid = __raw_readl((char *)KSEG1ADDR(AR5523_REV));
	devid &= AR5523_REV_MAJ;
	devid >>= AR5523_REV_MAJ_S;
	if (devid == AR5523_REV_MAJ_AR2313) {
		predivide_mask = AR2313_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2313_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2313_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2313_CLOCKCTL1_MULTIPLIER_SHIFT;
		doubler_mask = AR2313_CLOCKCTL1_DOUBLER_MASK;
	} else { /* AR5312 and AR5523 */
		predivide_mask = AR5523_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR5523_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR5523_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR5523_CLOCKCTL1_MULTIPLIER_SHIFT;
		doubler_mask = AR5523_CLOCKCTL1_DOUBLER_MASK;
	}

	/*
	 * Clocking is derived from a fixed 40MHz input clock.
	 *
	 *  cpuFreq = InputClock * MULT (where MULT is PLL multiplier)
	 *  sysFreq = cpuFreq / 4	   (used for APB clock, serial,
	 *				    flash, Timer, Watchdog Timer)
	 *
	 *  cntFreq = cpuFreq / 2	   (use for CPU count/compare)
	 *
	 * So, for example, with a PLL multiplier of 5, we have
	 *
	 *  cpuFreq = 200MHz
	 *  sysFreq = 50MHz
	 *  cntFreq = 100MHz
	 *
	 * We compute the CPU frequency, based on PLL settings.
	 */

	clock_ctl1 = __raw_readl((char *)KSEG1ADDR(AR5523_CLOCKCTL1));
	pre_divide_select = (clock_ctl1 & predivide_mask) >> predivide_shift;
	pre_divisor = CLOCKCTL1_PREDIVIDE_TABLE[pre_divide_select];
	multiplier = (clock_ctl1 & multiplier_mask) >> multiplier_shift;

	if (clock_ctl1 & doubler_mask)
		multiplier = multiplier << 1;

	return (40000000 / pre_divisor) * multiplier;
}

static unsigned int
ar5523_sys_frequency(void)
{
	return ar5523_cpu_frequency() / 4;
}

/*
 * shutdown watchdog
 */
static int watchdog_init(void)
{
	pr_debug("Disable watchdog.\n");
	__raw_writeb(AR5523_WD_CTRL_IGNORE_EXPIRATION,
					(char *)KSEG1ADDR(AR5523_WD_CTRL));
	return 0;
}

static int platform_init(void)
{
	add_generic_device("ar231x_reset", DEVICE_ID_SINGLE, NULL,
			KSEG1ADDR(AR5523_RESET), 0x4,
			IORESOURCE_MEM, NULL);
	watchdog_init();
	return 0;
}
late_initcall(platform_init);

#endif
static struct NS16550_plat serial_plat = {
	.shift = AR5523_UART_SHIFT,
};

static int ar5523_console_init(void)
{
#if 0
	u32 reset;

	/* reset UART0 */
	reset = __raw_readl((char *)KSEG1ADDR(AR5523_RESET));
	reset = ((reset & ~AR5523_RESET_APB) | AR5523_RESET_UART0);
	__raw_writel(reset, (char *)KSEG1ADDR(AR5523_RESET));

	reset &= ~AR5523_RESET_UART0;
	__raw_writel(reset, (char *)KSEG1ADDR(AR5523_RESET));

	/* Register the serial port */
	serial_plat.clock = ar5523_sys_frequency();
#endif
	serial_plat.clock = 40000000;
	add_ns16550_device(DEVICE_ID_DYNAMIC, KSEG1ADDR(AR5523_UART0),
			   8 << AR5523_UART_SHIFT,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT,
			   &serial_plat);
	return 0;
}
console_initcall(ar5523_console_init);
