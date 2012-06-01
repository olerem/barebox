/*
 * kexec-mips.c - kexec for mips
 * Copyright (C) 2007 Francesco Chiechi, Alessandro Rubini
 * Copyright (C) 2007 Tvblob s.r.l.
 *
 * derived from ../ppc/kexec-mips.c
 * Copyright (C) 2004, 2005 Albert Herranz
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <linux/stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <memory.h>
#include <elf.h>
#include "../../../lib/kexec/kexec.h"
#include "machine_kexec.h"


static int elf_mips_probe(const char *buf, off_t len)
{
	struct mem_ehdr ehdr;
	int result;

	result = build_elf_exec_info(buf, len, &ehdr, 0);
	if (result < 0) {
		goto out;
	}

	/* Verify the architecuture specific bits */
	if (ehdr.e_machine != EM_MIPS) {
		/* for a different architecture */
		printf("Not for this architecture.\n");
		result = -1;
		goto out;
	}
	result = 0;

 out:
	free_elf_info(&ehdr);

	return result;
}

static int elf_mips_load(const char *buf, off_t len, struct kexec_info *info)
{
	struct mem_ehdr ehdr;
	int result;
	size_t i;

	result = build_elf_exec_info(buf, len, &ehdr, 0);
	if (result < 0) {
		printf("ELF exec parse failed\n");
		goto out;
	}

	/* Read in the PT_LOAD segments and remove CKSEG0 mask from address */
	for (i = 0; i < ehdr.e_phnum; i++) {
		struct mem_phdr *phdr;
		phdr = &ehdr.e_phdr[i];
		if (phdr->p_type == PT_LOAD) {
			phdr->p_paddr = virt_to_phys((void *)phdr->p_paddr);
		}
	}

	/* Load the ELF data */
	result = elf_exec_load(&ehdr, info);
	if (result < 0) {
		printf("ELF exec load failed\n");
		goto out;
	}

	info->entry = (void *)virt_to_phys((void *)ehdr.e_entry);

out:
	return result;
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
	add_segment_phys_virt(info, buf, bufsz,
		virt_to_phys((void *)base), memsz, 1);
}

/* relocator parameters */
extern unsigned long relocate_new_kernel;
extern unsigned long relocate_new_kernel_size;
extern unsigned long kexec_start_address;
extern unsigned long kexec_segments;
extern unsigned long kexec_nr_segments;

unsigned long reboot_code_buffer;

static void machine_kexec_print_args(void)
{
	unsigned long argc = (int)kexec_args[0];
	int i;

	printf("kexec_args[0] (argc): %lu\n", argc);
	printf("kexec_args[1] (argv): %p\n", (void *)kexec_args[1]);
	printf("kexec_args[2] (env ): %p\n", (void *)kexec_args[2]);
	printf("kexec_args[3] (desc): %p\n", (void *)kexec_args[3]);

	for (i = 0; i < argc; i++) {
		printf("kexec_argv[%d] = %p, %s\n",
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
		printf("kexec command line truncated to %zu bytes\n", size);

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
	kexec_args[1] = (unsigned long)kexec_argv;
	kexec_args[2] = 0;
	kexec_args[3] = 0;
}

static int machine_kexec_prepare(struct kexec_segment *segments, unsigned long nr_segments)
{
	/*
	 * Whenever arguments passed from kexec-tools, Init the arguments as
	 * the original ones to try avoiding booting failure.
	 */

	machine_kexec_init_argv(segments, nr_segments);
	machine_kexec_parse_argv();
	machine_kexec_print_args();

	return 0;
}
long kexec_load(void *entry, unsigned long nr_segments,
		struct kexec_segment *segments, unsigned long flags)
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
		printf("ELF can't be loaded!\n");
		return 0;
	}

	start = dcheck_res(&elf_segments);

	/* relocate_new_kernel() copy by register (4 or 8 bytes)
	   so start address must be aligned to 4/8 */
	start = (start + 15) & 0xfffffff0;

	for (i = 0; i < nr_segments; i++) {
		segments[i].mem = (void *)(phys_to_virt((unsigned long)segments[i].mem));
		memcpy(phys_to_virt(start), segments[i].buf, segments[i].bufsz);
		request_sdram_region("kexec relocatable segment",
			(unsigned long)phys_to_virt(start),
			(unsigned long)segments[i].bufsz);

		/* relocate_new_kernel() copy by register (4 or 8 bytes)
		   so bufsz must be aligned to 4/8 */
		segments[i].bufsz = (segments[i].bufsz + 15) & 0xfffffff0;
		segments[i].buf = phys_to_virt(start);
		start = start + segments[i].bufsz;
	}

	start = (start + 15) & 0xfffffff0;

	reboot_code_buffer = start;

	memcpy(phys_to_virt(start), &relocate_new_kernel,
		relocate_new_kernel_size);
	request_sdram_region("kexec relocator",
		(unsigned long)phys_to_virt(start),
		(unsigned long)relocate_new_kernel_size);

	start = start + relocate_new_kernel_size;
	start = (start + 15) & 0xfffffff0;

	kexec_start_address = (unsigned long)phys_to_virt((unsigned long)entry);
	kexec_segments = (unsigned long)phys_to_virt((unsigned long)start);
	kexec_nr_segments = nr_segments;

	memcpy(phys_to_virt(start), segments, nr_segments * sizeof(*segments));
	request_sdram_region("kexec control segments",
		(unsigned long)phys_to_virt(start),
		(unsigned long)nr_segments * sizeof(*segments));

	machine_kexec_prepare(segments, nr_segments);

	return 1;
}
