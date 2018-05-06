// SPDX-License-Identifier: GPL-2.0+

#include <asm/io.h>
#include <common.h>
#include <errno.h>
#include <kexec.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

static u16 elf16_to_cpu(const struct mem_ehdr *ehdr, u16 val)
{
	return ehdr->ei_data == ELFDATA2LSB ? le16_to_cpu(val)
		   : be16_to_cpu(val);
}

static u32 elf32_to_cpu(const struct mem_ehdr *ehdr, u32 val)
{
	return ehdr->ei_data == ELFDATA2LSB ? le32_to_cpu(val)
		   : be32_to_cpu(val);
}

static u64 elf64_to_cpu(const struct mem_ehdr *ehdr, u64 val)
{
	return ehdr->ei_data == ELFDATA2LSB ? le64_to_cpu(val)
		   : be64_to_cpu(val);
}

static int build_mem_elf32_ehdr(const void *buf, size_t len,
				struct mem_ehdr *ehdr)
{
	const Elf32_Ehdr *lehdr = buf;

	if (len < sizeof(Elf32_Ehdr)) {
		pr_err("Buffer is too small to hold ELF header\n");
		return -ENOEXEC;
	}

	if (elf16_to_cpu(ehdr, lehdr->e_ehsize) != sizeof(Elf32_Ehdr)) {
		pr_err("Bad ELF header size\n");
		return -ENOEXEC;
	}

	ehdr->e_type      = elf16_to_cpu(ehdr, lehdr->e_type);
	ehdr->e_machine   = elf16_to_cpu(ehdr, lehdr->e_machine);
	ehdr->e_version   = elf32_to_cpu(ehdr, lehdr->e_version);
	ehdr->e_entry     = elf32_to_cpu(ehdr, lehdr->e_entry);
	ehdr->e_phoff     = elf32_to_cpu(ehdr, lehdr->e_phoff);
	ehdr->e_shoff     = elf32_to_cpu(ehdr, lehdr->e_shoff);
	ehdr->e_flags     = elf32_to_cpu(ehdr, lehdr->e_flags);
	ehdr->e_phnum     = elf16_to_cpu(ehdr, lehdr->e_phnum);
	ehdr->e_shnum     = elf16_to_cpu(ehdr, lehdr->e_shnum);
	ehdr->e_shstrndx  = elf16_to_cpu(ehdr, lehdr->e_shstrndx);

	if ((ehdr->e_phnum > 0) &&
		(elf16_to_cpu(ehdr, lehdr->e_phentsize) != sizeof(Elf32_Phdr))) {
		pr_err("ELF bad program header size\n");
		return -ENOEXEC;
	}

	if ((ehdr->e_shnum > 0) &&
		(elf16_to_cpu(ehdr, lehdr->e_shentsize) != sizeof(Elf32_Shdr))) {
		pr_err("ELF bad section header size\n");
		return -ENOEXEC;
	}

	return 0;
}

static int build_mem_elf64_ehdr(const void *buf, size_t len,
				struct mem_ehdr *ehdr)
{
	const Elf64_Ehdr *lehdr = buf;

	if (len < sizeof(Elf64_Ehdr)) {
		pr_err("Buffer is too small to hold ELF header\n");
		return -ENOEXEC;
	}

	if (elf16_to_cpu(ehdr, lehdr->e_ehsize) != sizeof(Elf64_Ehdr)) {
		pr_err("Bad ELF header size\n");
		return -ENOEXEC;
	}

	ehdr->e_type      = elf16_to_cpu(ehdr, lehdr->e_type);
	ehdr->e_machine   = elf16_to_cpu(ehdr, lehdr->e_machine);
	ehdr->e_version   = elf32_to_cpu(ehdr, lehdr->e_version);
	ehdr->e_entry     = elf64_to_cpu(ehdr, lehdr->e_entry);
	ehdr->e_phoff     = elf64_to_cpu(ehdr, lehdr->e_phoff);
	ehdr->e_shoff     = elf64_to_cpu(ehdr, lehdr->e_shoff);
	ehdr->e_flags     = elf32_to_cpu(ehdr, lehdr->e_flags);
	ehdr->e_phnum     = elf16_to_cpu(ehdr, lehdr->e_phnum);
	ehdr->e_shnum     = elf16_to_cpu(ehdr, lehdr->e_shnum);
	ehdr->e_shstrndx  = elf16_to_cpu(ehdr, lehdr->e_shstrndx);

	if ((ehdr->e_phnum > 0) &&
		(elf16_to_cpu(ehdr, lehdr->e_phentsize) != sizeof(Elf64_Phdr))) {
		pr_err("ELF bad program header size\n");
		return -ENOEXEC;
	}

	if ((ehdr->e_shnum > 0) &&
		(elf16_to_cpu(ehdr, lehdr->e_shentsize) != sizeof(Elf64_Shdr))) {
		pr_err("ELF bad section header size\n");
		return -ENOEXEC;
	}

	return 0;
}

static int build_mem_ehdr(const void *buf, size_t len, struct mem_ehdr *ehdr)
{
	unsigned char e_ident[EI_NIDENT];
	int ret;

	memset(ehdr, 0, sizeof(*ehdr));

	if (len < sizeof(e_ident)) {
		pr_err("Buffer is too small to hold ELF e_ident\n");
		return -ENOEXEC;
	}

	memcpy(e_ident, buf, sizeof(e_ident));

	ehdr->ei_class   = e_ident[EI_CLASS];
	ehdr->ei_data    = e_ident[EI_DATA];
	if ((ehdr->ei_class != ELFCLASS32) &&
		(ehdr->ei_class != ELFCLASS64)) {
		pr_err("Not a supported ELF class\n");
		return -ENOEXEC;
	}

	if ((ehdr->ei_data != ELFDATA2LSB) &&
		(ehdr->ei_data != ELFDATA2MSB)) {
		pr_err("Not a supported ELF data format\n");
		return -ENOEXEC;
	}

	if (ehdr->ei_class == ELFCLASS32)
		ret = build_mem_elf32_ehdr(buf, len, ehdr);
	else
		ret = build_mem_elf64_ehdr(buf, len, ehdr);

	if (IS_ERR_VALUE(ret))
		return ret;

	if ((e_ident[EI_VERSION] != EV_CURRENT) ||
		(ehdr->e_version != EV_CURRENT)) {
		pr_err("Unknown ELF version\n");
		return -ENOEXEC;
	}

	return 0;
}

static void build_mem_elf32_phdr(const char *buf, struct mem_ehdr *ehdr, int idx)
{
	struct mem_phdr *phdr;
	const Elf32_Phdr *lphdr;

	lphdr = (const Elf32_Phdr *)(buf + ehdr->e_phoff + (idx * sizeof(Elf32_Phdr)));
	phdr = &ehdr->e_phdr[idx];

	phdr->p_type   = elf32_to_cpu(ehdr, lphdr->p_type);
	phdr->p_paddr  = elf32_to_cpu(ehdr, lphdr->p_paddr);
	phdr->p_vaddr  = elf32_to_cpu(ehdr, lphdr->p_vaddr);
	phdr->p_filesz = elf32_to_cpu(ehdr, lphdr->p_filesz);
	phdr->p_memsz  = elf32_to_cpu(ehdr, lphdr->p_memsz);
	phdr->p_offset = elf32_to_cpu(ehdr, lphdr->p_offset);
	phdr->p_flags  = elf32_to_cpu(ehdr, lphdr->p_flags);
	phdr->p_align  = elf32_to_cpu(ehdr, lphdr->p_align);
}

static void build_mem_elf64_phdr(const char *buf, struct mem_ehdr *ehdr, int idx)
{
	struct mem_phdr *phdr;
	const Elf64_Phdr *lphdr;

	lphdr = (const Elf64_Phdr *)(buf + ehdr->e_phoff + (idx * sizeof(Elf64_Phdr)));
	phdr = &ehdr->e_phdr[idx];

	phdr->p_type   = elf32_to_cpu(ehdr, lphdr->p_type);
	phdr->p_paddr  = elf64_to_cpu(ehdr, lphdr->p_paddr);
	phdr->p_vaddr  = elf64_to_cpu(ehdr, lphdr->p_vaddr);
	phdr->p_filesz = elf64_to_cpu(ehdr, lphdr->p_filesz);
	phdr->p_memsz  = elf64_to_cpu(ehdr, lphdr->p_memsz);
	phdr->p_offset = elf64_to_cpu(ehdr, lphdr->p_offset);
	phdr->p_flags  = elf32_to_cpu(ehdr, lphdr->p_flags);
	phdr->p_align  = elf64_to_cpu(ehdr, lphdr->p_align);
}

static int build_mem_phdrs(const char *buf, off_t len, struct mem_ehdr *ehdr,
				u32 flags)
{
	size_t phdr_size, mem_phdr_size, i;

	/* e_phnum is at most 65535 so calculating
	 * the size of the program header cannot overflow.
	 */
	/* Is the program header in the file buffer? */
	phdr_size = 0;
	if (ehdr->ei_class == ELFCLASS32) {
		phdr_size = sizeof(Elf32_Phdr);
	} else if (ehdr->ei_class == ELFCLASS64) {
		phdr_size = sizeof(Elf64_Phdr);
	} else {
		pr_err("Invalid ei_class?\n");
		return -ENOEXEC;
	}
	phdr_size *= ehdr->e_phnum;

	/* Allocate the e_phdr array */
	mem_phdr_size = sizeof(ehdr->e_phdr[0]) * ehdr->e_phnum;
	ehdr->e_phdr = xmalloc(mem_phdr_size);

	for (i = 0; i < ehdr->e_phnum; i++) {
		struct mem_phdr *phdr;

		if (ehdr->ei_class == ELFCLASS32)
			build_mem_elf32_phdr(buf, ehdr, i);
		else
			build_mem_elf64_phdr(buf, ehdr, i);

		/* Check the program headers to be certain
		 * they are safe to use.
		 */
		phdr = &ehdr->e_phdr[i];
		if ((phdr->p_paddr + phdr->p_memsz) < phdr->p_paddr) {
			/* The memory address wraps */
			pr_err("ELF address wrap around\n");
			return -ENOEXEC;
		}

		/* Remember where the segment lives in the buffer */
		phdr->p_data = buf + phdr->p_offset;
	}

	return 0;
}

static int build_mem_elf32_shdr(const char *buf, struct mem_ehdr *ehdr, int idx)
{
	struct mem_shdr *shdr;
	const char *sbuf;
	int size_ok;
	Elf32_Shdr lshdr;

	sbuf = buf + ehdr->e_shoff + (idx * sizeof(lshdr));
	shdr = &ehdr->e_shdr[idx];
	memcpy(&lshdr, sbuf, sizeof(lshdr));
	shdr->sh_name      = elf32_to_cpu(ehdr, lshdr.sh_name);
	shdr->sh_type      = elf32_to_cpu(ehdr, lshdr.sh_type);
	shdr->sh_flags     = elf32_to_cpu(ehdr, lshdr.sh_flags);
	shdr->sh_addr      = elf32_to_cpu(ehdr, lshdr.sh_addr);
	shdr->sh_offset    = elf32_to_cpu(ehdr, lshdr.sh_offset);
	shdr->sh_size      = elf32_to_cpu(ehdr, lshdr.sh_size);
	shdr->sh_link      = elf32_to_cpu(ehdr, lshdr.sh_link);
	shdr->sh_info      = elf32_to_cpu(ehdr, lshdr.sh_info);
	shdr->sh_addralign = elf32_to_cpu(ehdr, lshdr.sh_addralign);
	shdr->sh_entsize   = elf32_to_cpu(ehdr, lshdr.sh_entsize);

	/* Now verify sh_entsize */
	size_ok = 0;
	switch (shdr->sh_type) {
	case SHT_SYMTAB:
		size_ok = shdr->sh_entsize == sizeof(Elf32_Sym);
		break;
	case SHT_RELA:
		size_ok = shdr->sh_entsize == sizeof(Elf32_Rela);
		break;
	case SHT_DYNAMIC:
		size_ok = shdr->sh_entsize == sizeof(Elf32_Dyn);
		break;
	case SHT_REL:
		size_ok = shdr->sh_entsize == sizeof(Elf32_Rel);
		break;
	case SHT_NOTE:
	case SHT_NULL:
	case SHT_PROGBITS:
	case SHT_HASH:
	case SHT_NOBITS:
	default:
		/* This is a section whose entsize requirements
		 * I don't care about.  If I don't know about
		 * the section I can't care about it's entsize
		 * requirements.
		 */
		size_ok = 1;
		break;
	}

	if (!size_ok) {
		pr_err("Bad section header(%x) entsize: %lld\n",
			shdr->sh_type, shdr->sh_entsize);
		return -ENOEXEC;
	}

	return 0;
}

static int build_mem_elf64_shdr(const char *buf, struct mem_ehdr *ehdr, int idx)
{
	struct mem_shdr *shdr;
	const char *sbuf;
	int size_ok;
	Elf64_Shdr lshdr;

	sbuf = buf + ehdr->e_shoff + (idx * sizeof(lshdr));
	shdr = &ehdr->e_shdr[idx];
	memcpy(&lshdr, sbuf, sizeof(lshdr));
	shdr->sh_name      = elf32_to_cpu(ehdr, lshdr.sh_name);
	shdr->sh_type      = elf32_to_cpu(ehdr, lshdr.sh_type);
	shdr->sh_flags     = elf64_to_cpu(ehdr, lshdr.sh_flags);
	shdr->sh_addr      = elf64_to_cpu(ehdr, lshdr.sh_addr);
	shdr->sh_offset    = elf64_to_cpu(ehdr, lshdr.sh_offset);
	shdr->sh_size      = elf64_to_cpu(ehdr, lshdr.sh_size);
	shdr->sh_link      = elf32_to_cpu(ehdr, lshdr.sh_link);
	shdr->sh_info      = elf32_to_cpu(ehdr, lshdr.sh_info);
	shdr->sh_addralign = elf64_to_cpu(ehdr, lshdr.sh_addralign);
	shdr->sh_entsize   = elf64_to_cpu(ehdr, lshdr.sh_entsize);

	/* Now verify sh_entsize */
	size_ok = 0;
	switch (shdr->sh_type) {
	case SHT_SYMTAB:
		size_ok = shdr->sh_entsize == sizeof(Elf64_Sym);
		break;
	case SHT_RELA:
		size_ok = shdr->sh_entsize == sizeof(Elf64_Rela);
		break;
	case SHT_DYNAMIC:
		size_ok = shdr->sh_entsize == sizeof(Elf64_Dyn);
		break;
	case SHT_REL:
		size_ok = shdr->sh_entsize == sizeof(Elf64_Rel);
		break;
	case SHT_NOTE:
	case SHT_NULL:
	case SHT_PROGBITS:
	case SHT_HASH:
	case SHT_NOBITS:
	default:
		/* This is a section whose entsize requirements
		 * I don't care about.  If I don't know about
		 * the section I can't care about it's entsize
		 * requirements.
		 */
		size_ok = 1;
		break;
	}

	if (!size_ok) {
		pr_err("Bad section header(%x) entsize: %lld\n",
			shdr->sh_type, shdr->sh_entsize);
		return -ENOEXEC;
	}

	return 0;
}

static int build_mem_shdrs(const char *buf, off_t len, struct mem_ehdr *ehdr,
				u32 flags)
{
	size_t shdr_size, mem_shdr_size, i;

	/* e_shnum is at most 65536 so calculating
	 * the size of the section header cannot overflow.
	 */
	/* Is the program header in the file buffer? */
	shdr_size = 0;
	if (ehdr->ei_class == ELFCLASS32) {
		shdr_size = sizeof(Elf32_Shdr);
	} else if (ehdr->ei_class == ELFCLASS64) {
		shdr_size = sizeof(Elf64_Shdr);
	} else {
		pr_err("Invalid ei_class?\n");
		return -ENOEXEC;
	}
	shdr_size *= ehdr->e_shnum;

	/* Allocate the e_shdr array */
	mem_shdr_size = sizeof(ehdr->e_shdr[0]) * ehdr->e_shnum;
	ehdr->e_shdr = xmalloc(mem_shdr_size);

	for (i = 0; i < ehdr->e_shnum; i++) {
		struct mem_shdr *shdr;
		int result;

		result = -1;
		if (ehdr->ei_class == ELFCLASS32)
			result = build_mem_elf32_shdr(buf, ehdr, i);

		if (ehdr->ei_class == ELFCLASS64)
			result = build_mem_elf64_shdr(buf, ehdr, i);

		if (result < 0)
			return result;

		/* Check the section headers to be certain
		 * they are safe to use.
		 */
		shdr = &ehdr->e_shdr[i];
		if ((shdr->sh_addr + shdr->sh_size) < shdr->sh_addr) {
			pr_err("ELF address wrap around\n");
			return -ENOEXEC;
		}

		/* Remember where the section lives in the buffer */
		shdr->sh_data = (unsigned char *)(buf + shdr->sh_offset);
	}

	return 0;
}

void free_elf_info(struct mem_ehdr *ehdr)
{
	free(ehdr->e_phdr);
	free(ehdr->e_shdr);
	memset(ehdr, 0, sizeof(*ehdr));
}

int build_elf_info(const char *buf, off_t len, struct mem_ehdr *ehdr, u32 flags)
{
	int result;

	result = build_mem_ehdr(buf, len, ehdr);
	if (result < 0)
		return result;

	if ((ehdr->e_phoff > 0) && (ehdr->e_phnum > 0)) {
		result = build_mem_phdrs(buf, len, ehdr, flags);
		if (result < 0) {
			free_elf_info(ehdr);
			return result;
		}
	}

	if ((ehdr->e_shoff > 0) && (ehdr->e_shnum > 0)) {
		result = build_mem_shdrs(buf, len, ehdr, flags);
		if (result < 0) {
			free_elf_info(ehdr);
			return result;
		}
	}

	return 0;
}

int check_room_for_elf(struct list_head *elf_segments)
{
	struct memory_bank *bank;
	struct resource *res, *r;

	list_for_each_entry(r, elf_segments, sibling) {
		int got_bank;

		got_bank = 0;
		for_each_memory_bank(bank) {
			resource_size_t start, end;

			res = bank->res;

			start = virt_to_phys((void *)res->start);
			end = virt_to_phys((void *)res->end);

			if ((start <= r->start) && (end >= r->end)) {
				got_bank = 1;
				break;
			}
		}

		if (!got_bank)
			return -ENOEXEC;
	}

	return 0;
}

/* sort by size */
static int compare(struct list_head *a, struct list_head *b)
{
	struct resource *ra = (struct resource *)list_entry(a,
					struct resource, sibling);
	struct resource *rb = (struct resource *)list_entry(b,
					struct resource, sibling);
	resource_size_t sa, sb;

	sa = ra->end - ra->start;
	sb = rb->end - rb->start;

	if (sa > sb)
		return -1;
	if (sa < sb)
		return 1;
	return 0;
}

void list_add_used_region(struct list_head *new, struct list_head *head)
{
	struct list_head *pos, *insert = head;
	struct resource *rb =
		(struct resource *)list_entry(new, struct resource, sibling);
	struct list_head *n;

	/* rb --- new region */
	list_for_each_safe(pos, n, head) {
		struct resource *ra = (struct resource *)list_entry(pos,
						struct resource, sibling);

		if (((rb->end >= ra->start) && (rb->end <= ra->end))
			|| ((rb->start >= ra->start) && (rb->start <= ra->end))
			|| ((rb->start >= ra->start) && (rb->end <= ra->end))
			|| ((ra->start >= rb->start) && (ra->end <= rb->end))
			|| (ra->start == rb->end + 1)
			|| (rb->start == ra->end + 1)) {
			rb->start = min(ra->start, rb->start);
			rb->end = max(ra->end, rb->end);
			rb->name = "join";
			list_del(pos);
		}
	}

	list_for_each(pos, head) {
		struct resource *ra = (struct resource *)list_entry(pos,
						struct resource, sibling);

		if (ra->start < rb->start)
			continue;

		insert = pos;
		break;
	}

	list_add_tail(new, insert);
}

resource_size_t dcheck_res(struct list_head *elf_segments)
{
	struct memory_bank *bank;
	struct resource *res, *r, *t;

	LIST_HEAD(elf_relocate_banks);
	LIST_HEAD(elf_relocate_banks_size_sorted);
	LIST_HEAD(used_regions);

	for_each_memory_bank(bank) {
		res = bank->res;

		list_for_each_entry(r, &res->children, sibling) {
			t = create_resource("tmp",
				virt_to_phys((void *)r->start),
				virt_to_phys((void *)r->end));
			list_add_used_region(&t->sibling, &used_regions);
		}
	}

	list_for_each_entry(r, elf_segments, sibling) {
		t = create_resource(r->name, r->start, r->end);
		list_add_used_region(&t->sibling, &used_regions);
	}

	for_each_memory_bank(bank) {
		resource_size_t start;

		res = bank->res;
		res = create_resource("tmp",
				virt_to_phys((void *)res->start),
				virt_to_phys((void *)res->end));
		start = res->start;

		list_for_each_entry(r, &used_regions, sibling) {
			if (res->start > r->end)
				continue;

			if (res->end < r->start)
				continue;

			if (r->start - start) {
				struct resource *t;

				t = create_resource("ELF buffer", start,
						    r->start - 1);
				list_add_used_region(&t->sibling,
						     &elf_relocate_banks);
			}
			start = r->end + 1;
		}

		if (res->end - start) {
			struct resource *t;

			t = create_resource("ELF buffer", start, res->end);
			list_add_used_region(&t->sibling, &elf_relocate_banks);
		}
	}

	list_for_each_entry(r, &elf_relocate_banks, sibling) {
		struct resource *t;

		t = create_resource("ELF buffer", r->start, r->end);
		list_add_sort(&t->sibling,
			&elf_relocate_banks_size_sorted, compare);
	}

	r = list_first_entry(&elf_relocate_banks_size_sorted, struct resource,
			     sibling);

	return r->start;
}
