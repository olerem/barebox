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

extern long kexec_load(void *entry, unsigned long nr_segments,
		       struct kexec_segment *segments, unsigned long flags);

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
	unsigned long long p_paddr;
	unsigned long long p_vaddr;
	unsigned long long p_filesz;
	unsigned long long p_memsz;
	unsigned long long p_offset;
	const char *p_data;
	unsigned p_type;
	unsigned p_flags;
	unsigned long long p_align;
};

struct mem_shdr {
	unsigned sh_name;
	unsigned sh_type;
	unsigned long long sh_flags;
	unsigned long long sh_addr;
	unsigned long long sh_offset;
	unsigned long long sh_size;
	unsigned sh_link;
	unsigned sh_info;
	unsigned long long sh_addralign;
	unsigned long long sh_entsize;
	const unsigned char *sh_data;
};

struct mem_ehdr {
	unsigned ei_class;
	unsigned ei_data;
	unsigned e_type;
	unsigned e_machine;
	unsigned e_version;
	unsigned e_flags;
	unsigned e_phnum;
	unsigned e_shnum;
	unsigned e_shstrndx;
	unsigned long long e_entry;
	unsigned long long e_phoff;
	unsigned long long e_shoff;
	struct mem_phdr *e_phdr;
	struct mem_shdr *e_shdr;
};

void free_elf_info(struct mem_ehdr *ehdr);
int build_elf_info(const char *buf, off_t len, struct mem_ehdr *ehdr, u32 flags);
int build_elf_exec_info(const char *buf, off_t len,
				struct mem_ehdr *ehdr, u32 flags);

int elf_exec_load(struct mem_ehdr *ehdr, struct kexec_info *info);

u16 elf16_to_cpu(const struct mem_ehdr *ehdr, u16 value);
u32 elf32_to_cpu(const struct mem_ehdr *ehdr, u32 value);
u64 elf64_to_cpu(const struct mem_ehdr *ehdr, u64 value);

unsigned long elf_max_addr(const struct mem_ehdr *ehdr);
int check_room_for_elf(struct list_head *elf_segments);
resource_size_t dcheck_res(struct list_head *elf_segments);
void list_add_used_region(struct list_head *new, struct list_head *head);

#endif /* _LINUX_REBOOT_H */
