/*
 * Copyright (c) 2001 William L. Pitts
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 */

#include <common.h>
#include <bootm.h>
#include <command.h>
#include <elf.h>
#include <environment.h>
#include <libfile.h>
#include <memory.h>

struct elf_section {
	struct list_head list;
	struct resource *r;
};

static int elf_request_region(struct list_head *list,
		resource_size_t start, resource_size_t size)
{
	struct resource *r_new;
	struct elf_section *r;

	r = xzalloc(sizeof(*r));
	r_new = request_sdram_region("elf_section", start, size);
	if (!r_new) {
		pr_err("filed to request region: %x %x\n", start, size);
		return -EINVAL;
	}

	r->r = r_new;
	list_add_tail(&r->list, list);

	return 0;
}

static void elf_release_regions(struct list_head *list)
{
	struct elf_section *r, *r_tmp;

	list_for_each_entry_safe(r, r_tmp, list, list) {
		release_sdram_region(r->r);
		free(r);
	}
}

/*
 * A very simple ELF loader, assumes the image is valid, returns the
 * entry point address.
 *
 * The loader firstly reads the EFI class to see if it's a 64-bit image.
 * If yes, call the ELF64 loader. Otherwise continue with the ELF32 loader.
 */
static int load_elf_image_phdr(struct elf_image *elf)
{
	unsigned long addr = elf->buf;
	Elf32_Ehdr *ehdr; /* Elf header structure pointer */
	Elf32_Phdr *phdr; /* Program header structure pointer */
	int i, ret;

	ehdr = (Elf32_Ehdr *)addr;

	phdr = (Elf32_Phdr *)(addr + ehdr->e_phoff);

	elf->entry = ehdr->e_entry;

	/* Load each program header */
	for (i = 0; i < ehdr->e_phnum; ++i) {
		void *dst = (void *)(uintptr_t)phdr->p_paddr;
		void *src = (void *)addr + phdr->p_offset;

		/* we care only about PT_LOAD segments */
		if (phdr->p_type != PT_LOAD)
			continue;

		pr_debug("Loading phdr %i to 0x%p (%i bytes)\n",
		      i, dst, phdr->p_filesz);
		if (phdr->p_filesz) {
			ret = elf_request_region(&elf->list, dst, phdr->p_filesz);
			if (ret)
				return ret;
			memcpy(dst, src, phdr->p_filesz);
		} else
			continue;

		if (phdr->p_filesz != phdr->p_memsz)
			memset(dst + phdr->p_filesz, 0x00,
			       phdr->p_memsz - phdr->p_filesz);
		++phdr;
	}

	return ret;
}

/*
 * Determine if a valid ELF image exists at the given memory location.
 * First look at the ELF header magic field, then make sure that it is
 * executable.
 */
static int valid_elf_image(void *buf)
{
	Elf32_Ehdr *ehdr; /* Elf header structure pointer */

	ehdr = (Elf32_Ehdr *)buf;

	if (ehdr->e_ident[EI_MAG0] != ELFMAG0
	    || ehdr->e_ident[EI_MAG1] != ELFMAG1
	    || ehdr->e_ident[EI_MAG2] != ELFMAG2
	    || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		pr_err("No elf image.\n");
                return 0;
        }

	if (ehdr->e_type != ET_EXEC) {
		pr_err("Not a 32-bit elf image.\n");
		return 0;
	}

	return 1;
}

struct elf_image *elf_load_image(const char *file)
{
	struct elf_image *elf;
	size_t size;
	int ret;

	elf = xzalloc(sizeof(*elf));

	INIT_LIST_HEAD(&elf->list);

	elf->buf = read_file(file, &size);

	if (!valid_elf_image(elf->buf))
		return ERR_PTR(-EINVAL);

	ret = load_elf_image_phdr(elf);

	return ret ? ERR_PTR(ret) : elf;
}

void elf_release_image(struct elf_image *elf)
{
	elf_release_regions(&elf->list);

	free(elf->buf);
	free(elf);
}
