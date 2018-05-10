// SPDX-License-Identifier: GPL-2.0+
/*
 * kexec: Linux boots Linux
 *
 * Copyright (C) 2003-2005  Eric Biederman (ebiederm@xmission.com)
 * Modified (2007-05-15) by Francesco Chiechi to rudely handle mips platform
 *
 */

#include <asm/io.h>
#include <boot.h>
#include <common.h>
#include <kexec.h>
#include <libfile.h>

static int sort_segments(struct kexec_info *info)
{
	int i, j;
	void *end = NULL;

	/* Do a stupid insertion sort... */
	for (i = 0; i < info->nr_segments; i++) {
		int tidx;
		struct kexec_segment temp;
		tidx = i;
		for (j = i + 1; j < info->nr_segments; j++) {
			if (info->segment[j].mem < info->segment[tidx].mem)
				tidx = j;
		}
		if (tidx != i) {
			temp = info->segment[tidx];
			info->segment[tidx] = info->segment[i];
			info->segment[i] = temp;
		}
	}

	/* Now see if any of the segments overlap */
	for (i = 0; i < info->nr_segments; i++) {
		if (end > info->segment[i].mem) {
			pr_err("Overlapping memory segments at %p\n",
				end);
			return -EBUSY;
		}
		end = ((char *)info->segment[i].mem) + info->segment[i].memsz;
	}

	return 0;
}

void add_segment_phys_virt(struct kexec_info *info,
	const void *buf, size_t bufsz,
	unsigned long base, size_t memsz, int phys)
{
	size_t size;
	int pagesize;

	if (bufsz > memsz)
		bufsz = memsz;

	/* Forget empty segments */
	if (!memsz)
		return;

	/* Round memsz up to a multiple of pagesize */
	pagesize = 4096;
	memsz = (memsz + (pagesize - 1)) & ~(pagesize - 1);

	if (phys)
		base = virt_to_phys((void *)base);

	size = (info->nr_segments + 1) * sizeof(info->segment[0]);
	info->segment = xrealloc(info->segment, size);
	info->segment[info->nr_segments].buf   = buf;
	info->segment[info->nr_segments].bufsz = bufsz;
	info->segment[info->nr_segments].mem   = (void *)base;
	info->segment[info->nr_segments].memsz = memsz;
	info->nr_segments++;
	if (info->nr_segments > KEXEC_MAX_SEGMENTS) {
		pr_warn("Warning: kernel segment limit reached. "
			"This will likely fail\n");
	}
}

static int kexec_load_one_file(struct kexec_info *info, char *fname)
{
	char *buf;
	size_t fsize;
	int i = 0;

	buf = read_file(fname, &fsize);

	/* FIXME: check buf */

	for (i = 0; i < kexec_file_types; i++) {
		if (kexec_file_type[i].probe(buf, fsize) >= 0)
			break;
	}

	if (i == kexec_file_types) {
		pr_err("Cannot determine the file type "
				"of %s\n", fname);
		return -ENOEXEC;
	}

	return kexec_file_type[i].load(buf, fsize, info);
}

static int kexec_load_binary_file(struct kexec_info *info, char *fname,
					size_t *fsize, unsigned long base)
{
	char *buf;

	buf = read_file(fname, fsize);
	if (!buf)
		return -ENOENT;

	add_segment(info, buf, *fsize, base, *fsize);

	return 0;
}

static void print_segments(struct kexec_info *info)
{
	int i;

	pr_info("print_segments\n");
	for (i = 0; i < info->nr_segments; i++) {
		struct kexec_segment *seg = &info->segment[i];

		pr_info("  %d. buf=%#08p bufsz=%#lx mem=%#08p memsz=%#lx\n", i,
			seg->buf, seg->bufsz, seg->mem, seg->memsz);
	}
}

static unsigned long find_unused_base(struct kexec_info *info, int *padded)
{
	unsigned long base = 0;

	if (info->nr_segments) {
		int i = info->nr_segments - 1;
		struct kexec_segment *seg = &info->segment[i];

		base = (unsigned long)seg->mem + seg->memsz;
	}

	if (!*padded) {
		/*
		 * Pad it; the kernel scribbles over memory
		 * beyond its load address.
		 * see grub-core/loader/mips/linux.c
		 */
		base += 0x100000;
		*padded = 1;
	}

	return base;
}

#include <environment.h>

int kexec_load_bootm_data(struct image_data *data)
{
	int ret;
	struct kexec_info info;
	char *cmdline;
	const char *t;
	size_t tlen, klen;
	size_t fsize;
	char initrd_cmdline[40];
	int padded = 0;

	memset(&info, 0, sizeof(info));

	initrd_cmdline[0] = 0;

	ret = kexec_load_one_file(&info, data->os_file);
	if (IS_ERR_VALUE(ret)) {
		pr_err("Cannot load %s\n", data->os_file);
		return ret;
	}

	if (data->oftree_file) {
		unsigned long base = find_unused_base(&info, &padded);

		base = ALIGN(base, 8);

		ret = kexec_load_binary_file(&info,
				data->oftree_file, &fsize, base);
		if (IS_ERR_VALUE(ret)) {
			pr_err("Cannot load %s\n", data->oftree_file);
			return ret;
		}
		data->oftree_address = base;
	}

	if (data->initrd_file) {
		unsigned long base = find_unused_base(&info, &padded);

		/*
		 * initrd start must be page aligned,
		 * max page size for mips is 64k.
		 */
		base = ALIGN(base, 0x10000);

		ret = kexec_load_binary_file(&info,
				data->initrd_file, &fsize, base);
		if (IS_ERR_VALUE(ret)) {
			pr_err("Cannot load %s\n", data->initrd_file);
			return ret;
		}
		data->initrd_address = base;

		if (bootm_verbose(data)) {
			pr_info("initrd: rd_start=0x%08x rd_size=0x%08x\n",
					data->initrd_address, fsize);
		}
		snprintf(initrd_cmdline, sizeof(initrd_cmdline),
				" rd_start=0x%08x rd_size=0x%08x",
					phys_to_virt(data->initrd_address),
					fsize);
	}

	t = linux_bootargs_get();
	if (t)
		tlen = strlen(t);
	else
		tlen = 0;

	klen = strlen("kexec ");
	cmdline = xzalloc(tlen + sizeof(initrd_cmdline) + klen);
	memcpy(cmdline, "kexec ", klen);
	if (tlen)
		memcpy(cmdline + klen, t, tlen);

	memcpy(cmdline + klen + tlen, initrd_cmdline, sizeof(initrd_cmdline));

	if (cmdline) {
		int cmdlinelen = strlen(cmdline) + 1;
		unsigned long base = find_unused_base(&info, &padded);

		base = ALIGN(base, 8);

		add_segment(&info, cmdline, cmdlinelen, base, cmdlinelen);
		data->cmdline_address = base;
	}

	if (bootm_verbose(data))
		print_segments(&info);

	/* Verify all of the segments load to a valid location in memory */

	/* Sort the segments and verify we don't have overlaps */
	ret = sort_segments(&info);
	if (IS_ERR_VALUE(ret))
		return ret;

	return kexec_load(info.entry,
		info.nr_segments, info.segment, info.kexec_flags);
}
