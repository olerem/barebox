/*
 * Copyright (C) 2014 Du Huanpeng <u74147@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <mach/debug_ll.h>

#include <console.h>
#include <driver.h>

static int board_init(void)
{
	printf("%s\n", __func__);
	return 0;
}
late_initcall(board_init);


static int as92_console_init(void)
{
	barebox_set_model("Alphascale asm9260t-evk");
	barebox_set_hostname("asm9260t-evk");

	add_generic_device("as92_serial", DEVICE_ID_DYNAMIC, NULL, 0, 4096,
			   IORESOURCE_MEM, NULL);

	return 0;
}

console_initcall(as92_console_init);
