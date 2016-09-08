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
#include <asm/memory.h>


static int mem_init(void)  
{
	mips_add_ram0(AR5523_SRAM_SIZE);
	return 0;
}
mem_initcall(mem_init);

static unsigned int
ar5523_sys_frequency(void)
{
	/* for now just use statick clock */
	return AR5523_WIFI_CLK;
}

#if 0
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
			KSEG1ADDR(HW_AR5523_RESET), 0x4,
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
	u32 reset;

	/* reset UART0 */
	reset = __raw_readl((char *)KSEG1ADDR(HW_AR5523_RESET));
	reset = ((reset & ~BM_AR5523_RST_APBDMA) | BM_AR5523_RST_UART0);
	__raw_writel(reset, (char *)KSEG1ADDR(HW_AR5523_RESET));

	reset &= ~BM_AR5523_RST_UART0;
	__raw_writel(reset, (char *)KSEG1ADDR(HW_AR5523_RESET));

	/* Register the serial port */
	serial_plat.clock = ar5523_sys_frequency();
	add_ns16550_device(DEVICE_ID_DYNAMIC, KSEG1ADDR(AR5523_UART0),
			   8 << AR5523_UART_SHIFT,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT,
			   &serial_plat);
	return 0;
}
console_initcall(ar5523_console_init);
