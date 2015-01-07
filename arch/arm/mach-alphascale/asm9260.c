#include <common.h>
#include <init.h>
#include <sizes.h>
#include <asm/armlinux.h>
#include <mach/asm9260-regs.h>

static int asm9260_mem_init(void)
{
	arm_add_mem_device("ram0", ASM9260_MEMORY_BASE, SZ_32M);

	return 0;
}
mem_initcall(asm9260_mem_init);

static int asm9260_init(void)
{
	struct device_node *root;
	root = of_get_root_node();
	if (root) {
		if (!of_machine_is_compatible("alphascale,asm9260")) {
			pr_err("DT should be compatible to \"alphascale,asm9260\"\n");
			hang();
		}
		pr_info("DT \"alphascale,asm9260\"\n");
	} else
		pr_info("!!!! No DT\n");

	return 0;
}
postcore_initcall(asm9260_init);
