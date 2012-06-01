// SPDX-License-Identifier: GPL-2.0+

#include <binfmt.h>
#include <bootm.h>
#include <init.h>
#include <kexec.h>

static int do_bootm_elf(struct image_data *data)
{
	int ret;

	ret = kexec_load_bootm_data(data);
	if (IS_ERR_VALUE(ret))
		return ret;
	
	kexec_arch(data);

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

static int elf_register_image_handler(void)
{
	int ret;

	ret = register_image_handler(&elf_handler);
	if (IS_ERR_VALUE(ret))
		return ret;
	
	return binfmt_register(&binfmt_elf_hook);
}
late_initcall(elf_register_image_handler);
