#include <boot.h>
#include <bootm.h>
#include <common.h>
#include <libfile.h>
#include <malloc.h>
#include <init.h>
#include <fs.h>
#include <errno.h>
#include <binfmt.h>
#include <restart.h>

#include <asm/byteorder.h>
#include <asm/io.h>

static int do_bootm_barebox(struct image_data *data)
{
	void (*barebox)(void);

	barebox = read_file(data->os_file, NULL);
	if (!barebox)
		return -EINVAL;

	if (data->dryrun) {
		free(barebox);
		return 0;
	}

	shutdown_barebox();

	barebox();

	restart_machine();
}

static struct image_handler barebox_handler = {
	.name = "MIPS barebox",
	.bootm = do_bootm_barebox,
	.filetype = filetype_mips_barebox,
};

static struct binfmt_hook binfmt_barebox_hook = {
	.type = filetype_mips_barebox,
	.exec = "bootm",
};

static int do_bootm_elf(struct image_data *data)
{
	struct elf_image *elf;
	unsigned long (*entry)(int, char * const[], int, int);

	elf = elf_load_image(data->os_file);
	if (IS_ERR(elf))
		return PTR_ERR(elf);

	bootm_get_devicetree(data);

	pr_info("Starting application at 0x%08lx, dts 0x%08lx...\n",
		phys_to_virt(elf->entry), data->of_root_node);

	shutdown_barebox();

	entry = elf->entry;

	entry(-2, phys_to_virt(data->oftree), 0, 0);

	pr_err("ELF application terminated\n");

	elf_release_image(elf);

	return -ERESTARTSYS;
}

static struct image_handler elf_handler = {
	.name = "ELF",
	.bootm = do_bootm_elf,
	.filetype = filetype_elf,
};

static struct binfmt_hook binfmt_elf_hook = {
	.type = filetype_elf,
	.exec = "bootm",
};

static int mips_register_image_handler(void)
{
	register_image_handler(&barebox_handler);
	binfmt_register(&binfmt_barebox_hook);

	register_image_handler(&elf_handler);
	binfmt_register(&binfmt_elf_hook);

	return 0;
}
late_initcall(mips_register_image_handler);
