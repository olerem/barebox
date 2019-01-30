/*
 * go- execute some code anywhere (misc boot support)
 *
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/ctype.h>
#include <errno.h>

static int do_go(int argc, char *argv[])
{
	void	*addr;
	int     rcode = 1;
	int	fd = -1;
	int	(*func)(int argc, char *argv[]);

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	if (!isdigit(*argv[1])) {
		fd = open(argv[1], O_RDONLY);
		if (fd < 0) {
			perror("open");
			goto out;
		}

		addr = memmap(fd, PROT_READ);
		if (addr == MAP_FAILED) {
			perror("memmap");
			goto out;
		}
	} else
		addr = (void *)simple_strtoul(argv[1], NULL, 16);

	printf("## Starting application at 0x%p ...\n", addr);

	console_flush();

	func = addr;

	shutdown_barebox();

	func(argc - 1, &argv[1]);

	/*
	 * The application returned. Since we have shutdown barebox and
	 * we know nothing about the state of the cpu/memory we can't
	 * do anything here.
	 */
	while (1);
out:
	if (fd > 0)
		close(fd);

	return rcode;
}

BAREBOX_CMD_HELP_START(go)
BAREBOX_CMD_HELP_TEXT("Start application at ADDR passing ARG as arguments.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("If addr does not start with a digit it is interpreted as a filename")
BAREBOX_CMD_HELP_TEXT("in which case the file is memmapped and executed")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(go)
	.cmd		= do_go,
	BAREBOX_CMD_DESC("start application at address or file")
	BAREBOX_CMD_OPTS("ADDR [ARG...]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_go_help)
	BAREBOX_CMD_COMPLETE(command_var_complete)
BAREBOX_CMD_END
