/*
 *  Copyright (C) alphascale <http://www.alphascale.com>
 *  Copyright (C) 2014 Du Huanpeng <u74147@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <io.h>
#include <mach/hardware.h>


#define WDT_SETMODE_RESET     ((u32)0x00000003)


/*
 *  Initialize the Watchdog.
 *  param  None. 
 */
void wdt_init(void)
{
	writel(1<<26,HW_PRESETCTRL0+8); /* reset watchdog */
	writel(1<<26,HW_PRESETCTRL0+4); /* clear watchdog reset */
	writel(1<<26,HW_AHBCLKCTRL0+4);
	writel(1,HW_WDTCLKSEL);
	writel(0,HW_WDTCLKUEN);
	writel(1,HW_WDTCLKUEN);
	writel(480,HW_WDTCLKDIV);       /* clkdiv=480 1MHz */

}

/*
 *  Sets WDG mode.
 *  param  Mode: specifies the WDG work mode.
 *   This parameter can be one of the following values.
 *      WDG_SETMODE_IT: Watchdog interrupt mode.
 *      WDG_SETMODE_RESET: Watchdog reset mode.
 */
void wdt_setmode(u32 mode)
{
	writel(mode,HW_WATCHDOG_WDMOD);
}

/*
 *  Sets WDG reload value.
 *  param  reload: specifies the IWDG reload value.
 *   This parameter must be a number between 0 and 0x0FFF.
 */
void wdt_setreload(u32 reload)
{
	writel(reload,HW_WATCHDOG_WDTC);  
}

/*
 *  reloads WDG counter with value defined in the WDTC register
 *  param  None
 */
void wdt_reloadcounter(void)
{
	writel(0xaa,HW_WATCHDOG_WDFEED);
	writel(0x55,HW_WATCHDOG_WDFEED);
}

/*
 * reset the cpu by setting up the watchdog timer and let him time out
 */
void reset_cpu(ulong ignored)
{
	wdt_init();
	wdt_setmode(WDT_SETMODE_RESET);
	wdt_setreload(50);
	wdt_reloadcounter();
	while(1);
}
EXPORT_SYMBOL(reset_cpu);
