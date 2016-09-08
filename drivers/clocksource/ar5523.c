/*
 * Copyright (C) 2013 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <clock.h>
#include <io.h>
#include <init.h>

#include <linux/clk.h>
#include <linux/err.h>

static __iomem void *ar5523_timer_base;

static uint64_t ar5523_cs_read(void)
{
	return ~readw(ar5523_timer_base);
}

static struct clocksource ar5523_cs = {
	.read = ar5523_cs_read,
	.mask = CLOCKSOURCE_MASK(16),
};

static int ar5523_cs_probe(struct device_d *dev)
{
	struct resource *iores;
	u32 rate;
	struct clk *timer_clk;

	timer_clk = clk_get(dev, NULL);
	if (IS_ERR(timer_clk))
		return PTR_ERR(timer_clk);

	rate = clk_get_rate(timer_clk);
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		clk_put(timer_clk);
		return PTR_ERR(iores);
	}
	ar5523_timer_base = IOMEM(iores->start);

	clocks_calc_mult_shift(&ar5523_cs.mult, &ar5523_cs.shift, rate,
			       NSEC_PER_SEC, 10);

	return init_clock(&ar5523_cs);
}

static struct driver_d ar5523_cs_driver = {
	.name = "ar5523-cs",
	.probe = ar5523_cs_probe,
};

static __init int ar5523_cs_init(void)
{
	return platform_driver_register(&ar5523_cs_driver);
}
coredevice_initcall(ar5523_cs_init);
