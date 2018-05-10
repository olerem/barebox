// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <asm/io.h>
#include <common.h>
#include <kexec.h>

void kexec_arch(void *opaque)
{
	extern unsigned long reboot_code_buffer;
	void (*kexec_code_buffer)(void);

	shutdown_barebox();

	kexec_code_buffer = phys_to_virt(reboot_code_buffer);

	kexec_code_buffer();
}
EXPORT_SYMBOL(kexec_arch);
