#ifndef __IMX_CLK_H
#define __IMX_CLK_H

struct clk *clk_gate2(const char *name, const char *parent, void __iomem *reg,
		      u8 shift, u8 cgr_val, unsigned long flags);

static inline struct clk *imx_clk_divider(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, CLK_SET_RATE_PARENT, reg, shift, width,
			   0);
}

static inline struct clk *imx_clk_divider_flags(const char *name,
                const char *parent, void __iomem *reg, u8 shift, u8 width,
                unsigned long flags)
{
	return clk_divider(name, parent, flags, reg, shift, width, 0);
}

static inline struct clk *imx_clk_divider_np(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, 0, reg, shift, width, 0);
}

static inline struct clk *imx_clk_divider2(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u8 width)
{
	return clk_divider(name, parent, CLK_OPS_PARENT_ENABLE, reg, shift,
			   width, 0);
}

static inline struct clk *imx_clk_divider_table(const char *name,
		const char *parent, void __iomem *reg, u8 shift, u8 width,
		const struct clk_div_table *table)
{
	return clk_divider_table(name, parent, CLK_SET_RATE_PARENT, reg, shift,
				 width, table, 0);
}

static inline struct clk *imx_clk_fixed_factor(const char *name,
		const char *parent, unsigned int mult, unsigned int div)
{
	return clk_fixed_factor(name, parent, mult, div, CLK_SET_RATE_PARENT);
}

static inline struct clk *imx_clk_mux_flags(const char *name, void __iomem *reg,
					    u8 shift, u8 width,
					    const char **parents, u8 num_parents,
					    unsigned long clk_flags)
{
	return clk_mux(name, clk_flags, reg, shift, width, parents, num_parents,
		       0);
}

static inline struct clk *imx_clk_mux2_flags(const char *name,
		void __iomem *reg, u8 shift, u8 width, const char **parents,
		int num_parents, unsigned long clk_flags)
{
	return clk_mux(name, clk_flags | CLK_OPS_PARENT_ENABLE, reg, shift,
		       width, parents, num_parents, 0);
}

static inline struct clk *imx_clk_mux(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents)
{
	return clk_mux(name, 0, reg, shift, width, parents, num_parents, 0);
}

static inline struct clk *imx_clk_mux2(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents)
{
	return clk_mux(name, CLK_OPS_PARENT_ENABLE, reg, shift, width, parents,
		       num_parents, 0);
}

static inline struct clk *imx_clk_mux_p(const char *name, void __iomem *reg,
		u8 shift, u8 width, const char **parents, u8 num_parents)
{
	return clk_mux(name, CLK_SET_RATE_PARENT, reg, shift, width, parents,
		       num_parents, 0);
}

static inline struct clk *imx_clk_gate(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate(name, parent, reg, shift, CLK_SET_RATE_PARENT, 0);
}

static inline struct clk *imx_clk_gate_dis(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate_inverted(name, parent, reg, shift, CLK_SET_RATE_PARENT);
}

static inline struct clk *imx_clk_gate2(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate2(name, parent, reg, shift, 0x3, 0);
}

static inline struct clk *imx_clk_gate2_shared2(const char *name, const char *parent,
						void __iomem *reg, u8 shift)
{
	return clk_gate2(name, parent, reg, shift, 0x3,
			 CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);
}

static inline struct clk *imx_clk_gate2_flags(const char *name,
		const char *parent, void __iomem *reg, u8 shift,
		unsigned long flags)
{
	return clk_gate2(name, parent, reg, shift, 0x3, flags);
}

static inline struct clk *imx_clk_gate2_cgr(const char *name, const char *parent,
					    void __iomem *reg, u8 shift, u8 cgr_val)
{
	return clk_gate2(name, parent, reg, shift, cgr_val, 0);
}

static inline struct clk *imx_clk_gate3(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate(name, parent, reg, shift, CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE, 0);
}

static inline struct clk *imx_clk_gate4(const char *name, const char *parent,
		void __iomem *reg, u8 shift)
{
	return clk_gate2(name, parent, reg, shift, 0x3, CLK_OPS_PARENT_ENABLE);
}

static inline struct clk *imx_clk_gate_shared(const char *name, const char *parent,
					      const char *shared)
{
	return clk_gate_shared(name, parent, shared, CLK_SET_RATE_PARENT);
}

struct clk *imx_clk_pllv1(const char *name, const char *parent,
		void __iomem *base);

struct clk *imx_clk_pllv2(const char *name, const char *parent,
		void __iomem *base);

enum imx_pllv3_type {
	IMX_PLLV3_GENERIC,
	IMX_PLLV3_SYS,
	IMX_PLLV3_SYS_VF610,
	IMX_PLLV3_USB,
	IMX_PLLV3_USB_VF610,
	IMX_PLLV3_AV,
	IMX_PLLV3_ENET,
	IMX_PLLV3_ENET_IMX7,
	IMX_PLLV3_MLB,
};

struct clk *imx_clk_pllv3(enum imx_pllv3_type type, const char *name,
			  const char *parent, void __iomem *base,
			  u32 div_mask);

struct clk *imx_clk_frac_pll(const char *name, const char *parent_name,
			     void __iomem *base);

enum imx_sccg_pll_type {
	SCCG_PLL1,
	SCCG_PLL2,
};

struct clk *imx_clk_sccg_pll(const char *name, const char *parent_name,
			     void __iomem *base,
			     enum imx_sccg_pll_type pll_type);

struct clk *imx_clk_pfd(const char *name, const char *parent,
			void __iomem *reg, u8 idx);

static inline struct clk *imx_clk_busy_divider(const char *name, const char *parent,
				 void __iomem *reg, u8 shift, u8 width,
				 void __iomem *busy_reg, u8 busy_shift)
{
	/*
	 * For now we do not support rate setting, so just fall back to
	 * regular divider.
	 */
	return imx_clk_divider(name, parent, reg, shift, width);
}

static inline struct clk *imx_clk_busy_mux(const char *name, void __iomem *reg, u8 shift,
			     u8 width, void __iomem *busy_reg, u8 busy_shift,
			     const char **parents, int num_parents)
{
	/*
	 * For now we do not support mux switching, so just fall back to
	 * regular mux.
	 */
	return imx_clk_mux(name, reg, shift, width, parents, num_parents);
}

struct clk *imx_clk_gate_exclusive(const char *name, const char *parent,
		void __iomem *reg, u8 shift, u32 exclusive_mask);

void imx_check_clocks(struct clk *clks[], unsigned int count);

struct clk *imx_clk_cpu(const char *name, const char *parent_name,
		struct clk *div, struct clk *mux, struct clk *pll,
		struct clk *step);

struct clk *imx8m_clk_composite_flags(const char *name,
		const char **parent_names, int num_parents, void __iomem *reg,
		unsigned long flags);

#define __imx8m_clk_composite(name, parent_names, reg, flags) \
		imx8m_clk_composite_flags(name, parent_names, \
			ARRAY_SIZE(parent_names), reg, \
			flags | CLK_OPS_PARENT_ENABLE)

#define imx8m_clk_composite(name, parent_names, reg) \
	__imx8m_clk_composite(name, parent_names, reg, 0)

#endif /* __IMX_CLK_H */
