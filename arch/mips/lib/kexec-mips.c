// SPDX-License-Identifier: GPL-2.0+
/*
 * kexec-mips.c - kexec for mips
 * Copyright (C) 2007 Francesco Chiechi, Alessandro Rubini
 * Copyright (C) 2007 Tvblob s.r.l.
 *
 * derived from ../ppc/kexec-mips.c
 * Copyright (C) 2004, 2005 Albert Herranz
 *
 */

#include <asm/io.h>
#include <common.h>
#include <elf.h>
#include <environment.h>
#include <globalvar.h>
#include <init.h>
#include <kexec.h>
#include <magicvar.h>
#include <memory.h>
#include "machine_kexec.h"

static unsigned int mips_boot_protocol;

enum mips_boot_names {
	MIPS_BOOT_FDT,
	MIPS_BOOT_LEGACY,
};

static int elf_mips_probe(const char *buf, off_t len)
{
	struct mem_ehdr ehdr;
	int ret;

	ret = build_elf_exec_info(buf, len, &ehdr);
	if (IS_ERR_VALUE(ret)) {
		goto out;
	}

	if (ehdr.e_machine != EM_MIPS) {
		pr_err("Not for this architecture.\n");
		ret = -EFAULT;
		goto out;
	}

 out:
	free_elf_info(&ehdr);

	return ret;
}

static int elf_mips_load(const char *buf, off_t len, struct kexec_info *info)
{
	struct mem_ehdr ehdr;
	int ret;
	size_t i;

	ret = build_elf_exec_info(buf, len, &ehdr);
	if (IS_ERR_VALUE(ret)) {
		pr_err("ELF exec parse failed\n");
		goto out;
	}

	/* Read in the PT_LOAD segments and remove CKSEG0 mask from address */
	for (i = 0; i < ehdr.e_phnum; i++) {
		struct mem_phdr *phdr;
		phdr = &ehdr.e_phdr[i];
		if (phdr->p_type == PT_LOAD) {
			phdr->p_paddr = phdr->p_paddr;
		}
	}

	/* Load the ELF data */
	ret = elf_exec_load(&ehdr, info);
	if (IS_ERR_VALUE(ret)) {
		pr_err("ELF exec load failed\n");
		goto out;
	}

	info->entry = ehdr.e_entry;

out:
	return ret;
}

struct kexec_file_type kexec_file_type[] = {
	{"elf-mips", elf_mips_probe, elf_mips_load },
};
int kexec_file_types = sizeof(kexec_file_type) / sizeof(kexec_file_type[0]);

/*
 * add_segment() should convert base to a physical address on mips,
 * while the default is just to work with base as is */
void add_segment(struct kexec_info *info, const void *buf, size_t bufsz,
		 unsigned long base, size_t memsz)
{
	add_segment_phys_virt(info, buf, bufsz, base, memsz, 1);
}

/* relocator parameters */
extern unsigned long relocate_new_kernel;
extern unsigned long relocate_new_kernel_size;
extern unsigned long kexec_start_address;
extern unsigned long kexec_segments;
extern unsigned long kexec_nr_segments;

unsigned long reboot_code_buffer;

static void machine_kexec_print_args(bool have_argv)
{
	unsigned long argc = (int)kexec_args[0];
	int i;

	pr_info("kexec_args[0] (argc): %lu\n", argc);
	pr_info("kexec_args[1] (argv): %p\n", (void *)kexec_args[1]);
	pr_info("kexec_args[2] (env ): %p\n", (void *)kexec_args[2]);
	pr_info("kexec_args[3] (desc): %p\n", (void *)kexec_args[3]);

	if (!have_argv)
		return;

	for (i = 0; i < argc; i++) {
		pr_info("kexec_argv[%d] = %p, %s\n",
				i, kexec_argv[i], kexec_argv[i]);
	}
}

static void machine_kexec_init_argv(struct kexec_segment *segments, unsigned long nr_segments)
{
	const void __user *buf = NULL;
	size_t bufsz;
	size_t size;
	int i;

	bufsz = 0;
	for (i = 0; i < nr_segments; i++) {
		struct kexec_segment *seg;

		seg = &segments[i];
		if (seg->bufsz < 6)
			continue;

		if (strncmp((char *) seg->buf, "kexec ", 6)) {
			continue;
		}

		buf = seg->buf + 6;
		bufsz = seg->bufsz - 6;
		break;
	}

	if (!buf)
		return;

	size = KEXEC_COMMAND_LINE_SIZE;
	size = min(size, bufsz);
	if (size < bufsz)
		pr_warn("kexec command line truncated to %zu bytes\n", size);

	/* Copy to kernel space */
	memcpy(kexec_argv_buf, buf, size);
	kexec_argv_buf[size - 1] = 0;
}

static void machine_kexec_parse_argv(void)
{
	char *ptr;
	int argc;

	ptr = kexec_argv_buf;
	argc = 1;

	/*
	 * convert command line string to array of parameters
	 * (as bootloader does).
	 */
	while (ptr && *ptr && (KEXEC_MAX_ARGC > argc)) {
		if (*ptr == ' ') {
			*ptr++ = '\0';
			continue;
		}

		kexec_argv[argc++] = ptr;
		ptr = strchr(ptr, ' ');
	}

	if (!argc)
		return;

	kexec_args[0] = argc;
	kexec_args[1] = virt_to_phys(kexec_argv);
	kexec_args[2] = 0;
	kexec_args[3] = 0;
}

static void machine_kexec_fdt(struct image_data *data)
{
	kexec_args[0] = -2;
	kexec_args[1] = virt_to_phys(data->oftree_address);
	kexec_args[2] = 0;
	kexec_args[3] = 0;
}


static int machine_kexec_prepare(struct image_data *data,
				 struct kexec_segment *segments,
				 unsigned long nr_segments)
{
	if (mips_boot_protocol == MIPS_BOOT_FDT) {
		machine_kexec_fdt(data);
		machine_kexec_print_args(0);
	} else {
		machine_kexec_init_argv(segments, nr_segments);
		machine_kexec_parse_argv();
		machine_kexec_print_args(1);
	}

	return 0;
}

int kexec_load(struct image_data *data, void *entry,
	       unsigned long nr_segments,
	       struct kexec_segment *segments)
{
	int i;
	struct resource *elf;
	resource_size_t start;
	LIST_HEAD(elf_segments);

	for (i = 0; i < nr_segments; i++) {
		resource_size_t mem = (resource_size_t)segments[i].mem;

		elf = create_resource("elf segment",
			mem, mem + segments[i].memsz - 1);

		list_add_used_region(&elf->sibling, &elf_segments);
	}

	if (check_room_for_elf(&elf_segments)) {
		pr_err("ELF can't be loaded!\n");
		return -ENOSPC;
	}

	start = dcheck_res(&elf_segments);

	/* relocate_new_kernel() copy by register (4 or 8 bytes)
	   so start address must be aligned to 4/8 */
	start = (start + 15) & 0xfffffff0;

	for (i = 0; i < nr_segments; i++) {
		segments[i].mem = segments[i].mem;
		memcpy(start, segments[i].buf, segments[i].bufsz);
		request_sdram_region("kexec relocatable segment", start,
			(unsigned long)segments[i].bufsz);

		/* relocate_new_kernel() copy by register (4 or 8 bytes)
		   so bufsz must be aligned to 4/8 */
		segments[i].bufsz = (segments[i].bufsz + 15) & 0xfffffff0;
		segments[i].buf = start;
		start = start + segments[i].bufsz;
	}

	start = (start + 15) & 0xfffffff0;

	reboot_code_buffer = start;

	memcpy(start, &relocate_new_kernel,
		relocate_new_kernel_size);
	request_sdram_region("kexec relocator", start,
		(unsigned long)relocate_new_kernel_size);

	start = start + relocate_new_kernel_size;
	start = (start + 15) & 0xfffffff0;

	kexec_start_address = entry;
	kexec_segments = start;
	kexec_nr_segments = nr_segments;

	memcpy(start, segments, nr_segments * sizeof(*segments));
	request_sdram_region("kexec control segments", start,
		(unsigned long)nr_segments * sizeof(*segments));

	machine_kexec_prepare(data, segments, nr_segments);

	return 0;
}

static const char *mips_boot_names[] = {
	"fdt", "legacy"
};

static int mips_kexec_globalvars_init(void)
{
	globalvar_add_simple_enum("bootm.mips.boot_protocol",
				   &mips_boot_protocol, mips_boot_names,
				   ARRAY_SIZE(mips_boot_names));
	return 0;
}
device_initcall(mips_kexec_globalvars_init);

BAREBOX_MAGICVAR_NAMED(global_bootm_mips_boot_protocol,
		       global_bootm_mips_boot_protocol,
		       "bootm.mips.boot_protocol: boot protocol used to start kernel");
