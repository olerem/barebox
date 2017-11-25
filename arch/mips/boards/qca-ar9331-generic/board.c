/*
 * Copyright (C) 2017 Oleksij Rempel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <common.h>
#include <init.h>

static int model_hostname_init(void)
{
	barebox_set_hostname("dpt-module");

	return 0;
}
postcore_initcall(model_hostname_init);
