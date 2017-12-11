/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <asm-generic/barebox.lds.h>

#if defined(CONFIG_64BIT)
#define PTR_COUNT_SHIFT 3
#else
#define PTR_COUNT_SHIFT 2
#endif

OUTPUT_ARCH(mips)
ENTRY(_start)
SECTIONS
{
	. = 0x0;

	. = ALIGN(4);
	.text      :
	{
		_stext = .;
		_text = .;
		*(.text_entry*)
		__bare_init_start = .;
		*(.text_bare_init*)
		__bare_init_end = .;
		*(.text*)
	}
	BAREBOX_BARE_INIT_SIZE

	PRE_IMAGE

	. = ALIGN(4);
	.rodata : { *(.rodata*) }

	_etext = .;			/* End of text and rodata section */
	_sdata = .;

	. = ALIGN(4);
	.data : { *(.data*) }

	.got : {
		*(.got)
	}

	num_got_entries = SIZEOF(.got) >> PTR_COUNT_SHIFT;

	.barebox_imd : { BAREBOX_IMD }

	. = .;
	__barebox_cmd_start = .;
	.barebox_cmd : { BAREBOX_CMDS }
	__barebox_cmd_end = .;

	__barebox_magicvar_start = .;
	.barebox_magicvar : { BAREBOX_MAGICVARS }
	__barebox_magicvar_end = .;

	__barebox_initcalls_start = .;
	.barebox_initcalls : { INITCALLS }
	__barebox_initcalls_end = .;

	__barebox_exitcalls_start = .;
	.barebox_exitcalls : { EXITCALLS }
	__barebox_exitcalls_end = .;

	__usymtab_start = .;
	__usymtab : { BAREBOX_SYMS }
	__usymtab_end = .;

	.oftables : { BAREBOX_CLK_TABLE() }

	.dtb : { BAREBOX_DTB() }

	.rel.dyn : {
		__rel_dyn_start = .;
		*(.rel.dyn)
		__rel_dyn_end = .;
	}

	.padding : {
		/*
		 * Workaround for a binutils feature (or bug?).
		 *
		 * The GNU ld from binutils puts the dynamic relocation
		 * entries into the .rel.dyn section. Sometimes it
		 * allocates more dynamic relocation entries than it needs
		 * and the unused slots are set to R_MIPS_NONE entries.
                 *
                 * However the size of the .rel.dyn section in the ELF
                 * section header does not cover the unused entries, so
                 * objcopy removes those during stripping.
                 *
                 * Create a small section here to avoid that.
                 */
                LONG(0xFFFFFFFF)
        }

	_edata = .;
	_end = .;

	.bss __rel_dyn_start (OVERLAY) : {
                __bss_start = .;
                *(.sbss.*)
                *(.bss.*)
                *(COMMON)
                . = ALIGN(4);
                __bss_stop = .;
        }

        .dynsym _end : {
                *(.dynsym)
        }

        .dynbss : {
                *(.dynbss)
        }

        .dynstr : {
                *(.dynstr)
        }

        .dynamic : {
                *(.dynamic)
        }
}
