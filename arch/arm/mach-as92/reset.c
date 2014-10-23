/*
 *  Copyright (C) 2014 Du Huanpeng <u74147@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <io.h>

#include <mach/debug_ll.h>

void __noreturn reset_cpu(unsigned long addr)
{
	int i=0;
	PUTC_LL('R');
	PUTC_LL('E');
	PUTC_LL('S');
	PUTC_LL('E');
	PUTC_LL('T');
	PUTC_LL('\r');
	PUTC_LL('\n');
	while(1);
}
EXPORT_SYMBOL(reset_cpu);
