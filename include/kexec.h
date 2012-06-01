#ifndef _LINUX_REBOOT_H
#define _LINUX_REBOOT_H

#include <bootm.h>

struct kexec_segment {
	const void *buf;
	size_t bufsz;
	const void *mem;
	size_t memsz;
};

struct kexec_info {
	struct kexec_segment *segment;
	int nr_segments;
	void *entry;
	unsigned long kexec_flags;
};

typedef int (probe_t)(const char *kernel_buf, off_t kernel_size);
typedef int (load_t)(const char *kernel_buf, off_t kernel_size,
	struct kexec_info *info);
struct kexec_file_type {
	const char *name;
	probe_t *probe;
	load_t  *load;
};

extern struct kexec_file_type kexec_file_type[];
extern int kexec_file_types;

extern void add_segment(struct kexec_info *info,
	const void *buf, size_t bufsz, unsigned long base, size_t memsz);
extern void add_segment_phys_virt(struct kexec_info *info,
	const void *buf, size_t bufsz, unsigned long base, size_t memsz,
	int phys);

extern int kexec_load(struct image_data *data, void *entry,
		      unsigned long nr_segments,
		      struct kexec_segment *segments);

extern void kexec_arch(void *opaque);

extern int kexec_load_bootm_data(struct image_data *data);

/* These values match the ELF architecture values.
 * Unless there is a good reason that should continue to be the case.
 */
#define KEXEC_ARCH_DEFAULT	(0 << 16)
#define KEXEC_ARCH_386		(3 << 16)
#define KEXEC_ARCH_X86_64	(62 << 16)
#define KEXEC_ARCH_PPC		(20 << 16)
#define KEXEC_ARCH_PPC64	(21 << 16)
#define KEXEC_ARCH_IA_64	(50 << 16)
#define KEXEC_ARCH_ARM		(40 << 16)
#define KEXEC_ARCH_S390		(22 << 16)
#define KEXEC_ARCH_SH		(42 << 16)
#define KEXEC_ARCH_MIPS_LE	(10 << 16)
#define KEXEC_ARCH_MIPS		(8 << 16)
#define KEXEC_ARCH_CRIS		(76 << 16)

#define KEXEC_MAX_SEGMENTS 16

struct mem_phdr {
	u64 p_paddr;
	u64 p_vaddr;
	u64 p_filesz;
	u64 p_memsz;
	u64 p_offset;
	const char *p_data;
	u32 p_type;
	u32 p_flags;
	u64 p_align;
};

struct mem_shdr {
	u32 sh_name;
	u32 sh_type;
	u64 sh_flags;
	u64 sh_addr;
	u64 sh_offset;
	u64 sh_size;
	u32 sh_link;
	u32 sh_info;
	u64 sh_addralign;
	u64 sh_entsize;
	const unsigned char *sh_data;
};

struct mem_ehdr {
	u32 ei_class;
	u32 ei_data;
	u32 e_type;
	u32 e_machine;
	u32 e_version;
	u32 e_flags;
	u32 e_phnum;
	u32 e_shnum;
	u32 e_shstrndx;
	u64 e_entry;
	u64 e_phoff;
	u64 e_shoff;
	struct mem_phdr *e_phdr;
	struct mem_shdr *e_shdr;
};

void free_elf_info(struct mem_ehdr *ehdr);
int build_elf_info(const char *buf, size_t len, struct mem_ehdr *ehdr);
int build_elf_exec_info(const char *buf, off_t len, struct mem_ehdr *ehdr);

int elf_exec_load(struct mem_ehdr *ehdr, struct kexec_info *info);

unsigned long elf_max_addr(const struct mem_ehdr *ehdr);
int check_room_for_elf(struct list_head *elf_segments);
resource_size_t dcheck_res(struct list_head *elf_segments);
void list_add_used_region(struct list_head *new, struct list_head *head);

#endif /* _LINUX_REBOOT_H */
