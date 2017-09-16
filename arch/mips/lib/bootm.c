#include <boot.h>
#include <bootm.h>
#include <common.h>
#include <libfile.h>
#include <malloc.h>
#include <memory.h>
#include <init.h>
#include <fs.h>
#include <errno.h>
#include <binfmt.h>
#include <restart.h>

#include <linux/sizes.h>

#include <asm/byteorder.h>

static void start_linux(void *adr, unsigned long initrd_address,
		 unsigned long initrd_size, void *oftree)
{
	void (*kernel)(int zero, int arch, void *params) = adr;
	void *params = NULL;

	if (oftree) {
		pr_debug("booting kernel with devicetree\n");
		params = oftree;
	}

	shutdown_barebox();

	kernel(0, 0, params);
}

/*
 * sdram_start_and_size() - determine place for putting the kernel/oftree/initrd
 *
 * @start:	returns the start address of the first RAM bank
 * @size:	returns the usable space at the beginning of the first RAM bank
 *
 * This function returns the base address of the first RAM bank and the free
 * space found there.
 *
 * return: 0 for success, negative error code otherwise
 */
static int sdram_start_and_size(unsigned long *start, unsigned long *size)
{
	struct memory_bank *bank;
	struct resource *res;

	/*
	 * We use the first memory bank for the kernel and other resources
	 */
	bank = list_first_entry_or_null(&memory_banks, struct memory_bank,
			list);
	if (!bank) {
		printf("cannot find first memory bank\n");
		return -EINVAL;
	}

	/*
	 * If the first memory bank has child resources we can use the bank up
	 * to the beginning of the first child resource, otherwise we can use
	 * the whole bank.
	 */
	res = list_first_entry_or_null(&bank->res->children, struct resource,
			sibling);
	if (res)
		*size = res->start - bank->start;
	else
		*size = bank->size;

	*start = bank->start;

	return 0;
}

static int get_kernel_addresses(size_t image_size,
				 int verbose, unsigned long *load_address,
				 unsigned long *mem_free)
{
	unsigned long mem_start, mem_size;
	int ret;
	size_t image_decomp_size;
	unsigned long spacing;

	ret = sdram_start_and_size(&mem_start, &mem_size);
	if (ret)
		return ret;

	/*
	 * The kernel documentation "Documentation/arm/Booting" advises
	 * to place the compressed image outside of the lowest 32 MiB to
	 * avoid relocation. We should do this if we have at least 64 MiB
	 * of ram. If we have less space, we assume a maximum
	 * compression factor of 5.
	 */
	image_decomp_size = PAGE_ALIGN(image_size * 5);
	if (mem_size >= SZ_64M)
		image_decomp_size = max_t(size_t, image_decomp_size, SZ_32M);

	/*
	 * By default put oftree/initrd close behind compressed kernel image to
	 * avoid placing it outside of the kernels lowmem region.
	 */
	spacing = SZ_1M;

	if (*load_address == UIMAGE_INVALID_ADDRESS) {
		/*
		 * Place the kernel at an address where it does not need to
		 * relocate itself before decompression.
		 */
		*load_address = mem_start + image_decomp_size;
		if (verbose)
			printf("no OS load address, defaulting to 0x%08lx\n",
				*load_address);
	} else if (*load_address <= mem_start + image_decomp_size) {
		/*
		 * If the user/image specified an address where the kernel needs
		 * to relocate itself before decompression we need to extend the
		 * spacing to allow this relocation to happen without
		 * overwriting anything placed behind the kernel.
		 */
		spacing += image_decomp_size;
	}

	*mem_free = PAGE_ALIGN(*load_address + image_size + spacing);

	/*
	 * Place oftree/initrd outside of the first 128 MiB, if we have space
	 * for it. This avoids potential conflicts with the kernel decompressor.
	 */
	if (mem_size > SZ_256M)
		*mem_free = max(*mem_free, mem_start + SZ_128M);

	return 0;
}

static int __do_bootm_linux(struct image_data *data, unsigned long free_mem, int swap)
{
	unsigned long kernel;
	unsigned long initrd_start = 0, initrd_size = 0, initrd_end = 0;
	int ret;

	kernel = data->os_res->start + data->os_entry;

	initrd_start = data->initrd_address;

	if (initrd_start == UIMAGE_INVALID_ADDRESS) {
		initrd_start = PAGE_ALIGN(free_mem);

		if (bootm_verbose(data)) {
			printf("no initrd load address, defaulting to 0x%08lx\n",
				initrd_start);
		}
	}

	if (bootm_has_initrd(data)) {
		ret = bootm_load_initrd(data, initrd_start);
		if (ret)
			return ret;
	}

	if (data->initrd_res) {
		initrd_start = data->initrd_res->start;
		initrd_end = data->initrd_res->end;
		initrd_size = resource_size(data->initrd_res);
		free_mem = PAGE_ALIGN(initrd_end + 1);
	}

	ret = bootm_load_devicetree(data, free_mem);
	if (ret)
		return ret;

	if (bootm_verbose(data)) {
		printf("\nStarting kernel at 0x%08lx", kernel);
		if (initrd_size)
			printf(", initrd at 0x%08lx", initrd_start);
		if (data->oftree)
			printf(", oftree at 0x%p", data->oftree);
		printf("...\n");
	}

	if (data->dryrun)
		return 0;

	start_linux((void *)kernel, initrd_start, initrd_size, data->oftree);

	restart_machine();

	return -ERESTARTSYS;
}

static int do_bootm_linux(struct image_data *data)
{
	unsigned long load_address, mem_free;
	int ret;

	load_address = data->os_address;

	ret = get_kernel_addresses(bootm_get_os_size(data),
			     bootm_verbose(data), &load_address, &mem_free);
	if (ret)
		return ret;

	ret = bootm_load_os(data, load_address);
	if (ret)
		return ret;

	return __do_bootm_linux(data, mem_free, 0);
}

static struct image_handler barebox_handler = {
	.name = "MIPS barebox",
	.bootm = do_bootm_linux,
	.filetype = filetype_mips_barebox,
};

static struct binfmt_hook binfmt_barebox_hook = {
	.type = filetype_mips_barebox,
	.exec = "bootm",
};

static int mips_register_image_handler(void)
{
	register_image_handler(&barebox_handler);
	binfmt_register(&binfmt_barebox_hook);

	return 0;
}
late_initcall(mips_register_image_handler);
