/*
 * Cache operations for the cache instruction.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * (C) Copyright 1996, 97, 99, 2002, 03 Ralf Baechle
 * (C) Copyright 1999 Silicon Graphics, Inc.
 */
#ifndef __ASM_CACHEOPS_H
#define __ASM_CACHEOPS_H

/*
 * Most cache ops are split into a 2 bit field identifying the cache, and a 3
 * bit field identifying the cache operation.
 */
#define Cache_I				0x00
#define Cache_D				0x01

#define Index_Writeback_Inv		0x00
#define Index_Store_Tag			0x08
#define Hit_Invalidate			0x10
#define Hit_Writeback_Inv		0x14	/* not with Cache_I though */

/*
 * Cache Operations available on all MIPS processors with R4000-style caches
 */
#define Index_Invalidate_I		(Cache_I | Index_Writeback_Inv)
#define Index_Writeback_Inv_D		(Cache_D | Index_Writeback_Inv)
#define Index_Store_Tag_I		(Cache_I | Index_Store_Tag)
#define Index_Store_Tag_D		(Cache_D | Index_Store_Tag)
#define Hit_Invalidate_D		(Cache_D | Hit_Invalidate)
#define Hit_Writeback_Inv_D		(Cache_D | Hit_Writeback_Inv)

/*
 * R4000SC and R4400SC-specific cacheops
 */
#define Cache_SD			0x03

#define Index_Writeback_Inv_SD		(Cache_SD | Index_Writeback_Inv)
#define Index_Store_Tag_SD		(Cache_SD | Index_Store_Tag)
#define Hit_Invalidate_SD		(Cache_SD | Hit_Invalidate)
#define Hit_Writeback_Inv_SD		(Cache_SD | Hit_Writeback_Inv)

#endif	/* __ASM_CACHEOPS_H */
