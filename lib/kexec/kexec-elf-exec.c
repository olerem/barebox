// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <elf.h>
#include <errno.h>
#include <kexec.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

int build_elf_exec_info(const char *buf, off_t len, struct mem_ehdr *ehdr,
				uint32_t flags)
{
	struct mem_phdr *phdr, *end_phdr;
	int ret;

	ret = build_elf_info(buf, len, ehdr, flags);
	if (IS_ERR_VALUE(ret))
		return ret;

	if (ehdr->e_type != ET_EXEC) {
		pr_err("Not ELF type ET_EXEC\n");
		return -ENOEXEC;
	}

	if (!ehdr->e_phdr) {
		pr_err("No ELF program header\n");
		return -ENOEXEC;
	}

	end_phdr = &ehdr->e_phdr[ehdr->e_phnum];
	for (phdr = ehdr->e_phdr; phdr != end_phdr; phdr++) {
		/* Kexec does not support loading interpreters.
		 * In addition this check keeps us from attempting
		 * to kexec ordinay executables.
		 */
		if (phdr->p_type == PT_INTERP) {
			printf("Requires an ELF interpreter\n");
			return -ENOEXEC;
		}
	}

	return 0;
}

int elf_exec_load(struct mem_ehdr *ehdr, struct kexec_info *info)
{
	size_t i;

	if (!ehdr->e_phdr) {
		printf("No program header?\n");
		return -ENOENT;
	}

	/* Read in the PT_LOAD segments */
	for (i = 0; i < ehdr->e_phnum; i++) {
		struct mem_phdr *phdr;
		size_t size;

		phdr = &ehdr->e_phdr[i];

		if (phdr->p_type != PT_LOAD)
			continue;

		size = phdr->p_filesz;

		if (size > phdr->p_memsz)
			size = phdr->p_memsz;

		add_segment(info,
			phdr->p_data, size,
			phdr->p_paddr, phdr->p_memsz);
	}

	return 0;
}
