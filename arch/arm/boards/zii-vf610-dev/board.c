/*
 * Copyright (C) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <gpio.h>
#include <environment.h>
#include <linux/clk.h>
#include <dt-bindings/clock/vf610-clock.h>
#include <envfs.h>


static int expose_signals(const struct gpio *signals,
			  size_t signal_num)
{
	int ret, i;

	ret = gpio_request_array(signals, signal_num);
	if (ret)
		return ret;

	for (i = 0; i < signal_num; i++)
		export_env_ull(signals[i].label, signals[i].gpio);

	return 0;
}

static int zii_vf610_cfu1_expose_signals(void)
{
	static const struct gpio signals[] = {
		{
			.gpio  = 132,
			.flags = GPIOF_IN,
			.label = "fim_sd",
		},
		{
			.gpio  = 118,
			.flags = GPIOF_OUT_INIT_LOW,
			.label = "fim_tdis",
		},
	};

	if (!of_machine_is_compatible("zii,vf610cfu1-a"))
		return 0;

	return expose_signals(signals, ARRAY_SIZE(signals));
}
late_initcall(zii_vf610_cfu1_expose_signals);

static int zii_vf610_cfu1_spu3_expose_signals(void)
{
	static const struct gpio signals[] = {
		{
			.gpio  = 107,
			.flags  = GPIOF_OUT_INIT_HIGH,
			.label = "soc_sw_rstn",
		},
		{
			.gpio  = 98,
			.flags = GPIOF_IN,
			.label = "e6352_intn",
		},
	};

	if (!of_machine_is_compatible("zii,vf610spu3-a") &&
	    !of_machine_is_compatible("zii,vf610cfu1-a"))
		return 0;

	return expose_signals(signals, ARRAY_SIZE(signals));
}
late_initcall(zii_vf610_cfu1_spu3_expose_signals);

static int zii_vf610_dev_print_clocks(void)
{
	int i;
	struct clk *clk;
	struct device_node *ccm_np;
	const unsigned long MHz = 1000000;
	const char *clk_names[] = { "cpu", "ddr", "bus", "ipg" };

	if (!of_machine_is_compatible("zii,vf610dev"))
		return 0;

	ccm_np = of_find_compatible_node(NULL, NULL, "fsl,vf610-ccm");
	if (!ccm_np) {
		pr_err("Couln't get CCM node\n");
		return -ENOENT;
	}

	for (i = 0; i < ARRAY_SIZE(clk_names); i++) {
		unsigned long rate;
		const char *name = clk_names[i];

		clk = of_clk_get_by_name(ccm_np, name);
		if (IS_ERR(clk)) {
			pr_err("Failed to get '%s' clock (%ld)\n",
			       name, PTR_ERR(clk));
			return PTR_ERR(clk);
		}

		rate = clk_get_rate(clk);

		pr_info("%s clock : %8lu MHz\n", name, rate / MHz);
	}

	return 0;
}
late_initcall(zii_vf610_dev_print_clocks);

static int zii_vf610_dev_set_hostname(void)
{
	size_t i;
	static const struct {
		const char *compatible;
		const char *hostname;
	} boards[] = {
		{ "zii,vf610spu3-a", "spu3-rev-a" },
		{ "zii,vf610cfu1-a", "cfu1-rev-a" },
		{ "zii,vf610dev-b", "dev-rev-b" },
		{ "zii,vf610dev-c", "dev-rev-c" },
		{ "zii,vf610scu4-aib-c", "scu4-aib-rev-c" },
	};

	if (!of_machine_is_compatible("zii,vf610dev"))
		return 0;

	for (i = 0; i < ARRAY_SIZE(boards); i++) {
		if (of_machine_is_compatible(boards[i].compatible)) {
			barebox_set_hostname(boards[i].hostname);
			break;
		}
	}

	defaultenv_append_directory(defaultenv_zii_vf610_dev);
	return 0;
}
late_initcall(zii_vf610_dev_set_hostname);
