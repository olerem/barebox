/*
 * Copyright (C) 2018 Antony Pavlov <antonynpavlov@gmail.com>
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

#include <common.h>
#include <init.h>
#include <memory.h>
#include <boot.h>
#include <kexec.h>
#include <bootm.h>
#include <asm/io.h>

int kexec_arch(void *opaque)
{
	extern unsigned long reboot_code_buffer;
	void (*kexec_code_buffer)(void);

	shutdown_barebox();

	kexec_code_buffer = phys_to_virt(reboot_code_buffer);

	kexec_code_buffer();
}
EXPORT_SYMBOL(reboot);
