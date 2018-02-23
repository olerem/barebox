/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 David S. Miller (davem@davemloft.net)
 * Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */
#include <common.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/cache.h>
#include <asm/cacheops.h>
#include <asm/cpu.h>
#include <asm/cpu-info.h>
#include <asm/bitops.h>

#define cache_op(op,addr)						\
	__asm__ __volatile__(						\
	"	.set	push					\n"	\
	"	.set	noreorder				\n"	\
	"	.set	mips3\n\t				\n"	\
	"	cache	%0, %1					\n"	\
	"	.set	pop					\n"	\
	:								\
	: "i" (op), "R" (*(unsigned char *)(addr)))

#define __BUILD_BLAST_CACHE_RANGE(pfx, desc, hitop)			\
static inline void blast_##pfx##cache##_range(unsigned long start,	\
					      unsigned long end)	\
{									\
	unsigned long lsize = current_cpu_data.desc.linesz;		\
	unsigned long addr = start & ~(lsize - 1);			\
	unsigned long aend = (end - 1) & ~(lsize - 1);			\
									\
	if (current_cpu_data.desc.flags & MIPS_CACHE_NOT_PRESENT)	\
		return;							\
									\
	while (1) {							\
		cache_op(hitop, addr);					\
		if (addr == aend)					\
			break;						\
		addr += lsize;						\
	}								\
}

__BUILD_BLAST_CACHE_RANGE(d, dcache, Hit_Writeback_Inv_D)
__BUILD_BLAST_CACHE_RANGE(inv_d, dcache, Hit_Invalidate_D)

void flush_cache_all(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;
	unsigned long lsize;
	unsigned long addr;
	unsigned long aend;
	unsigned int icache_size, dcache_size;

	dcache_size = c->dcache.waysize * c->dcache.ways;
	lsize = c->dcache.linesz;
	aend = (KSEG0 + dcache_size - 1) & ~(lsize - 1);
	for (addr = KSEG0; addr <= aend; addr += lsize)
		cache_op(Index_Writeback_Inv_D, addr);

	icache_size = c->icache.waysize * c->icache.ways;
	lsize = c->icache.linesz;
	aend = (KSEG0 + icache_size - 1) & ~(lsize - 1);
	for (addr = KSEG0; addr <= aend; addr += lsize)
		cache_op(Index_Invalidate_I, addr);

	/* secondatory cache skipped */
}

void dma_flush_range(unsigned long start, unsigned long end)
{
	blast_dcache_range(start, end);

	/* secondatory cache skipped */
}

void dma_inv_range(unsigned long start, unsigned long end)
{
	blast_inv_dcache_range(start, end);

	/* secondatory cache skipped */
}

void r4k_cache_init(void);

static inline int alias_74k_erratum(struct cpuinfo_mips *c)
{
	unsigned int imp = c->processor_id & PRID_IMP_MASK;
	unsigned int rev = c->processor_id & PRID_REV_MASK;
	int present = 0;

	/*
	 * Early versions of the 74K do not update the cache tags on a
	 * vtag miss/ptag hit which can occur in the case of KSEG0/KUSEG
	 * aliases.  In this case it is better to treat the cache as always
	 * having aliases.  Also disable the synonym tag update feature
	 * where available.  In this case no opportunistic tag update will
	 * happen where a load causes a virtual address miss but a physical
	 * address hit during a D-cache look-up.
	 */
	switch (imp) {
	case PRID_IMP_74K:
		if (rev <= PRID_REV_ENCODE_332(2, 4, 0))
			present = 1;
		if (rev == PRID_REV_ENCODE_332(2, 4, 0))
			write_c0_config6(read_c0_config6() | MIPS_CONF6_SYND);
		break;
	default:
		BUG();
	}

	return present;
}

static void probe_pcache(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;
	unsigned int icache_size, dcache_size;
	unsigned int config = read_c0_config();
	int has_74k_erratum = 0;
	unsigned long config1;
	unsigned int lsize;

	switch (c->cputype) {

	default:
		/*
		 * So we seem to be a MIPS32 or MIPS64 CPU
		 * So let's probe the I-cache ...
		 */
		config1 = read_c0_config1();

		if ((lsize = ((config1 >> 19) & 7)))
			c->icache.linesz = 2 << lsize;
		else
			c->icache.linesz = lsize;
		c->icache.sets = 64 << ((config1 >> 22) & 7);
		c->icache.ways = 1 + ((config1 >> 16) & 7);

		icache_size = c->icache.sets *
		              c->icache.ways *
		              c->icache.linesz;
		c->icache.waybit = __ffs(icache_size/c->icache.ways);

		if (config & 0x8)		/* VI bit */
			c->icache.flags |= MIPS_CACHE_VTAG;

		/*
		 * Now probe the MIPS32 / MIPS64 data cache.
		 */
		c->dcache.flags = 0;

		if ((lsize = ((config1 >> 10) & 7)))
			c->dcache.linesz = 2 << lsize;
		else
			c->dcache.linesz= lsize;
		c->dcache.sets = 64 << ((config1 >> 13) & 7);
		c->dcache.ways = 1 + ((config1 >> 7) & 7);

		dcache_size = c->dcache.sets *
		              c->dcache.ways *
		              c->dcache.linesz;
		c->dcache.waybit = __ffs(dcache_size/c->dcache.ways);

		c->options |= MIPS_CPU_PREFETCH;
		break;
	}

	/* compute a couple of other cache variables */
	c->icache.waysize = icache_size / c->icache.ways;
	c->dcache.waysize = dcache_size / c->dcache.ways;

	c->icache.sets = c->icache.linesz ?
		icache_size / (c->icache.linesz * c->icache.ways) : 0;
	c->dcache.sets = c->dcache.linesz ?
		dcache_size / (c->dcache.linesz * c->dcache.ways) : 0;

	/*
	 * R10000 and R12000 P-caches are odd in a positive way.  They're 32kB
	 * 2-way virtually indexed so normally would suffer from aliases.  So
	 * normally they'd suffer from aliases but magic in the hardware deals
	 * with that for us so we don't need to take care ourselves.
	 */
	switch (c->cputype) {
	case CPU_74K:
		has_74k_erratum = alias_74k_erratum(c);
		/* Fall through. */
	case CPU_24K:
		if (!(read_c0_config7() & MIPS_CONF7_IAR) &&
		    (c->icache.waysize > PAGE_SIZE))
			c->icache.flags |= MIPS_CACHE_ALIASES;
		if (!has_74k_erratum && (read_c0_config7() & MIPS_CONF7_AR)) {
			/*
			 * Effectively physically indexed dcache,
			 * thus no virtual aliases.
			*/
			c->dcache.flags |= MIPS_CACHE_PINDEX;
			break;
		}
	default:
		if (has_74k_erratum || c->dcache.waysize > PAGE_SIZE)
			c->dcache.flags |= MIPS_CACHE_ALIASES;
	}
}

#define CONFIG2_SS_OFFSET	8
#define CONFIG2_SL_OFFSET	4
#define CONFIG2_SA_OFFSET	0
static void probe_scache(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;
	unsigned int config2, config1, config = read_c0_config();
	unsigned int ss, sl, sa;

	if ((config & MIPS_CONF_M) == 0)
		goto noscache;
	config1 = read_c0_config1();
	if ((config1 & MIPS_CONF_M) == 0)
		goto noscache;
	config2 = read_c0_config2();
	ss = 0xf & (config2 >> CONFIG2_SS_OFFSET);
	sl = 0xf & (config2 >> CONFIG2_SL_OFFSET);
	sa = 0xf & (config2 >> CONFIG2_SA_OFFSET);
	if (sl == 0)
		goto noscache;
	c->scache.linesz = 1 << (sl + 1);
	c->scache.sets = 64 << ss;
	c->scache.ways = 1 + sa;
	c->scache.waysize = c->scache.linesz * c->scache.sets;
	return;
noscache:
	c->scache.flags = MIPS_CACHE_NOT_PRESENT;
}

void r4k_cache_init(void)
{
	probe_pcache();
	probe_scache();
}
