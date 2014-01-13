/*
 * Copyright (C) 2013 Du Huanpeng <u74147@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <io.h>
#include <mach/ath79.h>

static void __iomem *reset_base;

void __noreturn reset_cpu(ulong addr)
{
	(*(volatile unsigned *)0x1806001C) = 0x01000000;
	/*
	 * Used to command a full chip reset. This is the software equivalent of
	 * pulling the reset pin. The system will reboot with PLL disabled. Always
	 * zero when read.
	 */
	while (1);
	/*NOTREACHED*/
}
EXPORT_SYMBOL(reset_cpu);

static u32 ar933x_reset_readl(void)
{
	return __raw_readl(reset_base);
}

static void ar933x_reset_writel(u32 val)
{
	__raw_writel(val, reset_base);
}

void ar933x_reset_bit(u32 val, enum reset_state state)
{
	u32 tmp;

	tmp = ar933x_reset_readl();

	if (state == REMOVE)
		ar933x_reset_writel(tmp & ~val);
	else
		ar933x_reset_writel(tmp | val);
}
EXPORT_SYMBOL(ar933x_reset_bit);

static int ar933x_reset_probe(struct device_d *dev)
{
	reset_base = dev_request_mem_region(dev, 0);
	if (!reset_base) {
		dev_err(dev, "could not get memory region\n");
		return -ENODEV;
	}

	return 0;
}

static struct driver_d ar933x_reset_driver = {
	.probe	= ar933x_reset_probe,
	.name	= "ar933x_reset",
};

static int ar933x_reset_init(void)
{
	return platform_driver_register(&ar933x_reset_driver);
}
coredevice_initcall(ar933x_reset_init);
