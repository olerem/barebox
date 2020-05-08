// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 Pengutronix, Oleksij Rempel <o.rempel@pengutronix.de>
 */

#include <common.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <libfile.h>
#include <memory.h>
#include <unistd.h>
#include <linux/fs.h>

struct elf_section {
	struct list_head list;
	struct resource *r;
};

static int elf_request_region(struct elf_image *elf, resource_size_t start,
			      resource_size_t size)
{
	struct list_head *list = &elf->list;
	struct resource *r_new;
	struct elf_section *r;

	r = xzalloc(sizeof(*r));
	r_new = request_sdram_region("elf_section", start, size);
	if (!r_new) {
		pr_err("Failed to request region: %pa %pa\n", &start, &size);
		return -EINVAL;
	}

	r->r = r_new;
	list_add_tail(&r->list, list);

	return 0;
}

static void elf_release_regions(struct elf_image *elf)
{
	struct list_head *list = &elf->list;
	struct elf_section *r, *r_tmp;

	list_for_each_entry_safe(r, r_tmp, list, list) {
		release_sdram_region(r->r);
		free(r);
	}
}


static int load_elf_phdr_segment(struct elf_image *elf, void *src,
				 void *phdr)
{
	void *dst = (void *) (phys_addr_t) elf_phdr_p_paddr(elf, phdr);
	int ret;
	u64 p_filesz = elf_phdr_p_filesz(elf, phdr);
	u64 p_memsz = elf_phdr_p_memsz(elf, phdr);

	/* we care only about PT_LOAD segments */
	if (elf_phdr_p_type(elf, phdr) != PT_LOAD)
		return 0;

	if (!p_filesz)
		return 0;

	if (dst < elf->low_addr)
		elf->low_addr = dst;
	if (dst + p_memsz > elf->high_addr)
		elf->high_addr = dst + p_memsz;

	pr_debug("Loading phdr to 0x%p (%llu bytes)\n", dst, p_filesz);

	ret = elf_request_region(elf, (resource_size_t)dst, p_filesz);
	if (ret)
		return ret;

	memcpy(dst, src, p_filesz);

	if (p_filesz < p_memsz)
		memset(dst + p_filesz, 0x00,
		       p_memsz - p_filesz);

	return 0;
}

static int load_elf_image_phdr(struct elf_image *elf)
{
	void *buf = elf->buf;
	void *phdr = (void *) (buf + elf_hdr_e_phoff(elf, buf));
	int i, ret;

	for (i = 0; i < elf_hdr_e_phnum(elf, buf) ; ++i) {
		void *src = buf + elf_phdr_p_offset(elf, phdr);

		ret = load_elf_phdr_segment(elf, src, phdr);
		/* in case of error elf_load_image() caller should clean up and
		 * call elf_release_image() */
		if (ret)
			return ret;

		phdr += elf_size_of_phdr(elf);
	}

	return 0;
}

static int elf_check_image(struct elf_image *elf)
{
	if (strncmp(elf->buf, ELFMAG, SELFMAG)) {
		pr_err("ELF magic not found.\n");
		return -EINVAL;
	}

	elf->class = ((char *) elf->buf)[EI_CLASS];

	if (elf_hdr_e_type(elf, elf->buf) != ET_EXEC) {
		pr_err("Non EXEC ELF image.\n");
		return -ENOEXEC;
	}

	return 0;
}

static int elf_check_init(struct elf_image *elf, void *buf)
{
	int ret;

	elf->buf = buf;
	elf->low_addr = (void *) (unsigned long) -1;
	elf->high_addr = 0;

	ret = elf_check_image(elf);
	if (ret)
		return ret;

	elf->entry = elf_hdr_e_entry(elf, elf->buf);

	return 0;
}

struct elf_image *elf_load_image(void *buf)
{
	struct elf_image *elf;
	int ret;

	elf = xzalloc(sizeof(*elf));

	INIT_LIST_HEAD(&elf->list);

	ret = elf_check_init(elf, buf);
	if (ret) {
		free(elf);
		return ERR_PTR(ret);
	}

	ret = load_elf_image_phdr(elf);
	if (ret) {
		elf_release_image(elf);
		return ERR_PTR(ret);
	}

	return elf;
}

void elf_release_image(struct elf_image *elf)
{
	elf_release_regions(elf);

	free(elf);
}

int elf_load(struct elf_image *elf)
{
	int ret;

	ret = load_elf_image_phdr(elf);
	if (ret)
		elf_release_regions(elf);

	return ret;
}

static u64 elf_get_size(struct elf_image *elf)
{
	u64 sh_size = elf_hdr_e_shentsize(elf, elf->buf) *
		      elf_hdr_e_shnum(elf, elf->buf);

	/*
	 * The section header table is located at the end of the elf file thus
	 * we can take the offset and add the size of this table to obtain the
	 * file size.
	 */
	return elf_hdr_e_shoff(elf, elf->buf) + sh_size;
}

struct elf_image *elf_open(const char *filename)
{
	int fd, ret;
	u64 size;
	struct elf64_hdr hdr;
	struct elf_image *elf;
	ssize_t read_ret;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("could not open: %s\n", errno_str());
		return ERR_PTR(-errno);
	}

	if (read(fd, &hdr, sizeof(hdr)) < 0) {
		printf("could not read elf header: %s\n", errno_str());
		ret = -errno;
		goto err_close_fd;
	}

	elf = xzalloc(sizeof(*elf));

	ret = elf_check_init(elf, &hdr);
	if (ret) {
		ret = -errno;
		goto err_free_elf;
	}

	size = elf_get_size(elf);

	elf->buf = malloc(size);
	if (!elf->buf) {
		ret = -ENOMEM;
		goto err_free_elf;
	}

	lseek(fd, 0, SEEK_SET);

	read_ret = read_full(fd, elf->buf, size);
	if (read_ret < 0) {
		printf("could not read elf file: %s\n", errno_str());
		ret = -errno;
		goto err_free_buf;
	}

	return elf;

err_free_buf:
	free(elf->buf);
err_free_elf:
	free(elf);
err_close_fd:
	close(fd);

	return ERR_PTR(ret);
}

void elf_close(struct elf_image *elf)
{
	free(elf->buf);
	elf_release_image(elf);
}
