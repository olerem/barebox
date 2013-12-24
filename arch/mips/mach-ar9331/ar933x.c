
#include <common.h>
#include <init.h>
#include <sizes.h>
#include <io.h>
#include <asm/memory.h>

#include <mach/ath79.h>
#include <mach/ar71xx_regs.h>

unsigned long ar933x_cpu_rate;
unsigned long ar933x_ahb_rate;
unsigned long ar933x_ref_rate;

static int ar933x_clocks_init(void)
{
	unsigned long ref_rate;
	u32 clock_ctrl;
	u32 cpu_config;
	u32 freq;
	u32 t;

	t = ath79_reset_rr(AR933X_RESET_REG_BOOTSTRAP);
	if (t & AR933X_BOOTSTRAP_REF_CLK_40)
		ref_rate = (40 * 1000 * 1000);
	else
		ref_rate = (25 * 1000 * 1000);

	ar933x_ref_rate = ref_rate;
	clock_ctrl = ath79_pll_rr(AR933X_PLL_CLOCK_CTRL_REG);
	if (clock_ctrl & AR933X_PLL_CLOCK_CTRL_BYPASS) {
		ar933x_cpu_rate = ref_rate;
		ar933x_ahb_rate = ref_rate;
	} else {
		cpu_config = ath79_pll_rr(AR933X_PLL_CPU_CONFIG_REG);

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_REFDIV_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_REFDIV_MASK;
		freq = ref_rate / t;

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_NINT_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_NINT_MASK;
		freq *= t;

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_OUTDIV_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_OUTDIV_MASK;
		if (t == 0)
			t = 1;

		freq >>= t;

		t = ((clock_ctrl >> AR933X_PLL_CLOCK_CTRL_CPU_DIV_SHIFT) &
		     AR933X_PLL_CLOCK_CTRL_CPU_DIV_MASK) + 1;
		ar933x_cpu_rate = freq / t;

		t = ((clock_ctrl >> AR933X_PLL_CLOCK_CTRL_AHB_DIV_SHIFT) &
		     AR933X_PLL_CLOCK_CTRL_AHB_DIV_MASK) + 1;
		//ar933x_ahb_rate = freq / t;
		ar933x_ahb_rate = (25 * 1000 * 1000);
	}
	return 0;
}
postcore_initcall(ar933x_clocks_init);

static int ar933x_console_init(void)
{

#if 0
	/* reset UART0 */
	reset = __raw_readl((char *)KSEG1ADDR(AR2312_RESET));
	reset = ((reset & ~AR2312_RESET_APB) | AR2312_RESET_UART0);
	__raw_writel(reset, (char *)KSEG1ADDR(AR2312_RESET));


	reset &= ~AR2312_RESET_UART0;
	__raw_writel(reset, (char *)KSEG1ADDR(AR2312_RESET));

	/* Register the serial port */
	//serial_plat.clock = ar2312_sys_frequency();
#endif

	add_generic_device("ar933x_serial", DEVICE_ID_DYNAMIC, NULL,
				KSEG1ADDR(AR71XX_UART_BASE), 0x100,
				IORESOURCE_MEM | IORESOURCE_MEM_32BIT, NULL);
	printk("refrate: %u\n", ar933x_ref_rate);
	return 0;
}
console_initcall(ar933x_console_init);
