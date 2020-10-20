// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include <mach/ar2312_regs.h>
#include <dt-bindings/clock/ath25-clk.h>

#define AR2313_PLL_CLOCKCTL0		0x0
#define AR2313_PLL_CLOCKCTL1		0x4
#define AR2313_PLL_CLOCKCTL2		0x8


static struct clk *clks[ATH25_CLK_END];
static struct clk_onecell_data clk_data;

struct clk_ar231x {
	struct clk clk;
	void __iomem *base;
	u32 div_shift;
	u32 div_mask;
	const char *parent;
};

/*
 * This table is indexed by bits 5..4 of the CLOCKCTL1 register
 * to determine the predevisor value.
 */
static int CLOCKCTL1_PREDIVIDE_TABLE[4] = { 1, 2, 4, 5 };

static unsigned long clk_ar231x_recalc_rate(struct clk *clk,
	unsigned long parent_rate)
{
	struct clk_ar231x *f = container_of(clk, struct clk_ar231x, clk);
	unsigned int predivide_mask, predivide_shift;
	unsigned int multiplier_mask, multiplier_shift;
	unsigned int clock_ctl1, pre_divide_select, pre_divisor, multiplier;
	unsigned int doubler_mask;
	u32 devid;

	devid = __raw_readl((char *)KSEG1ADDR(AR2312_REV));
	devid &= AR2312_REV_MAJ;
	devid >>= AR2312_REV_MAJ_S;
	if (devid == AR2312_REV_MAJ_AR2313) {
		predivide_mask = AR2313_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2313_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2313_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2313_CLOCKCTL1_MULTIPLIER_SHIFT;
		doubler_mask = AR2313_CLOCKCTL1_DOUBLER_MASK;
	} else { /* AR5312 and AR2312 */
		predivide_mask = AR2312_CLOCKCTL1_PREDIVIDE_MASK;
		predivide_shift = AR2312_CLOCKCTL1_PREDIVIDE_SHIFT;
		multiplier_mask = AR2312_CLOCKCTL1_MULTIPLIER_MASK;
		multiplier_shift = AR2312_CLOCKCTL1_MULTIPLIER_SHIFT;
		doubler_mask = AR2312_CLOCKCTL1_DOUBLER_MASK;
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

	clock_ctl1 = __raw_readl(f->base + AR2313_PLL_CLOCKCTL1);
	pre_divide_select = (clock_ctl1 & predivide_mask) >> predivide_shift;
	pre_divisor = CLOCKCTL1_PREDIVIDE_TABLE[pre_divide_select];
	multiplier = (clock_ctl1 & multiplier_mask) >> multiplier_shift;

	if (clock_ctl1 & doubler_mask)
		multiplier = multiplier << 1;

	return (parent_rate / pre_divisor) * multiplier;
}

struct clk_ops clk_ar231x_ops = {
	.recalc_rate = clk_ar231x_recalc_rate,
};

static struct clk *clk_ar231x(const char *name, const char *parent,
	void __iomem *base)
{
	struct clk_ar231x *f = xzalloc(sizeof(*f));

	f->parent = parent;
	f->base = base;

	f->clk.ops = &clk_ar231x_ops;
	f->clk.name = name;
	f->clk.parent_names = &f->parent;
	f->clk.num_parents = 1;

	clk_register(&f->clk);

	return &f->clk;
}

static void ar231x_pll_init(void __iomem *base)
{
	clks[ATH25_CLK_CPU] = clk_ar231x("cpu", "ref", base);

	clks[ATH25_CLK_AHB] = clk_fixed_factor("ahb", "cpu", 1, 4, 0);
}

static int ar231x_clk_probe(struct device_d *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	ar231x_pll_init(base);

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(dev->device_node, of_clk_src_onecell_get,
			    &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id ar231x_clk_dt_ids[] = {
	{
		.compatible = "qca,ar231x-pll",
	}, {
		/* sentinel */
	}
};

static struct driver_d ar231x_clk_driver = {
	.probe	= ar231x_clk_probe,
	.name	= "ar231x_clk",
	.of_compatible = DRV_OF_COMPAT(ar231x_clk_dt_ids),
};

static int ar231x_clk_init(void)
{
	return platform_driver_register(&ar231x_clk_driver);
}
postcore_initcall(ar231x_clk_init);
