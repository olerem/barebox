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
	if (!r_new)
		return -EINVAL;

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
 * A very simple ELF64 loader, assumes the image is valid, returns the
 * entry point address.
 *
 * Note if U-Boot is 32-bit, the loader assumes the to segment's
 * physical address and size is within the lower 32-bit address space.
 */
static unsigned long load_elf64_image_phdr(struct elf_image *elf)
{
	unsigned long addr = elf->buf;
	Elf64_Ehdr *ehdr; /* Elf header structure pointer */
	Elf64_Phdr *phdr; /* Program header structure pointer */
	int i, ret;

	ehdr = (Elf64_Ehdr *)addr;
	phdr = (Elf64_Phdr *)(addr + (ulong)ehdr->e_phoff);

	/* Load each program header */
	for (i = 0; i < ehdr->e_phnum; ++i) {
		void *dst = (void *)(ulong)phdr->p_paddr;
		void *src = (void *)addr + phdr->p_offset;

		debug("Loading phdr %i to 0x%p (%lu bytes)\n",
		      i, dst, (ulong)phdr->p_filesz);
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

	return ehdr->e_entry;
}

/*
 * A very simple ELF loader, assumes the image is valid, returns the
 * entry point address.
 *
 * The loader firstly reads the EFI class to see if it's a 64-bit image.
 * If yes, call the ELF64 loader. Otherwise continue with the ELF32 loader.
 */
static unsigned long load_elf_image_phdr(struct elf_image *elf)
{
	unsigned long addr = elf->buf;
	Elf32_Ehdr *ehdr; /* Elf header structure pointer */
	Elf32_Phdr *phdr; /* Program header structure pointer */
	int i, ret;

	ehdr = (Elf32_Ehdr *)addr;
	if (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
		return load_elf64_image_phdr(addr);

	phdr = (Elf32_Phdr *)(addr + ehdr->e_phoff);

	/* Load each program header */
	for (i = 0; i < ehdr->e_phnum; ++i) {
		void *dst = (void *)(uintptr_t)phdr->p_paddr;
		void *src = (void *)addr + phdr->p_offset;

		printk("Loading phdr %i to 0x%p (%i bytes)\n",
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

	return ehdr->e_entry;
}

static unsigned long load_elf_image_shdr(struct elf_image *elf)
{
	unsigned long addr = elf->buf;
	Elf32_Ehdr *ehdr; /* Elf header structure pointer */
	Elf32_Shdr *shdr; /* Section header structure pointer */
	unsigned char *strtab = 0; /* String table pointer */
	unsigned char *image; /* Binary image pointer */
	int i; /* Loop counter */

	ehdr = (Elf32_Ehdr *)addr;

	/* Find the section header string table for output info */
	shdr = (Elf32_Shdr *)(addr + ehdr->e_shoff +
			     (ehdr->e_shstrndx * sizeof(Elf32_Shdr)));

	if (shdr->sh_type == SHT_STRTAB)
		strtab = (unsigned char *)(addr + shdr->sh_offset);

	/* Load each appropriate section */
	for (i = 0; i < ehdr->e_shnum; ++i) {
		shdr = (Elf32_Shdr *)(addr + ehdr->e_shoff +
				     (i * sizeof(Elf32_Shdr)));

		if (!(shdr->sh_flags & SHF_ALLOC) ||
		    shdr->sh_addr == 0 || shdr->sh_size == 0) {
			continue;
		}

		if (strtab) {
			printk("%sing %s @ 0x%08lx (%ld bytes)\n",
			      (shdr->sh_type == SHT_NOBITS) ? "Clear" : "Load",
			       &strtab[shdr->sh_name],
			       (unsigned long)shdr->sh_addr,
			       (long)shdr->sh_size);
		}

		if (shdr->sh_type == SHT_NOBITS) {
			memset((void *)(uintptr_t)shdr->sh_addr, 0,
			       shdr->sh_size);
		} else {
			image = (unsigned char *)addr + shdr->sh_offset;
			memcpy((void *)(uintptr_t)shdr->sh_addr,
			       (const void *)image, shdr->sh_size);
		}
	}

	return ehdr->e_entry;
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

struct elf_image *elf_load_image(struct image_data *data)
{
	struct elf_image *elf;
	size_t size;

	elf = xzalloc(sizeof(*elf));

	elf->buf = read_file(data->os_file, &size);

	if (!valid_elf_image(elf->buf))
		return ERR_PTR(-EINVAL);

	if (1)
		elf->elf_entry = load_elf_image_phdr(elf);
	else
		elf->elf_entry = load_elf_image_shdr(elf);

	return elf;
}

void elf_release_image(struct elf_image *elf)
{
	elf_release_regions(&elf->list);

	free(elf->buf);
	free(elf);
}
