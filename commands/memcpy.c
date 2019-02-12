/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

/*
 * Memory Functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <libfile.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

static char *devmem = "/dev/mem";

static int do_memcpy(int argc, char *argv[])
{
	loff_t count, dest, src;
	char *sourcefile = devmem;
	char *destfile = devmem;
	int sourcefd, destfd;
	int mode = 0;
	struct stat statbuf;
	int ret = 0;
	char *buf;

	if (mem_parse_options(argc, argv, "bwlqs:d:", &mode, &sourcefile,
			&destfile, NULL) < 0)
		return 1;

	if (optind + 2 > argc)
		return COMMAND_ERROR_USAGE;

	src = strtoull_suffix(argv[optind], NULL, 0);
	dest = strtoull_suffix(argv[optind + 1], NULL, 0);

	if (optind + 2 == argc) {
		if (sourcefile == devmem) {
			printf("source and count not given\n");
			return 1;
		}
		if (stat(sourcefile, &statbuf)) {
			perror("stat");
			return 1;
		}
		count = statbuf.st_size - src;
	} else {
		count = strtoull_suffix(argv[optind + 2], NULL, 0);
	}

	sourcefd = open_and_lseek(sourcefile, mode | O_RDONLY, src);
	if (sourcefd < 0)
		return 1;

	destfd = open_and_lseek(destfile, O_WRONLY | O_CREAT | mode, dest);
	if (destfd < 0) {
		close(sourcefd);
		return 1;
	}

	buf = xmalloc(RW_BUF_SIZE);

	while (count > 0) {
		int now, r, w, tmp;

		now = min((loff_t)RW_BUF_SIZE, count);

		r = read(sourcefd, buf, now);
		if (r < 0) {
			perror("read");
			goto out;
		}

		if (!r)
			break;

		tmp = 0;
		now = r;
		while (now) {
			w = write(destfd, buf + tmp, now);
			if (w < 0) {
				perror("write");
				goto out;
			}
	                if (!w)
			        break;

			now -= w;
			tmp += w;
		}

		count -= r;

		if (ctrlc())
			goto out;
	}

	if (count) {
		printf("ran out of data\n");
		ret = 1;
	}

out:
	free(buf);
	close(sourcefd);
	close(destfd);

	return ret;
}

BAREBOX_CMD_HELP_START(memcpy)
BAREBOX_CMD_HELP_TEXT("Copy memory at SRC of COUNT bytes to DEST")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b", "byte access")
BAREBOX_CMD_HELP_OPT ("-w", "word access (16 bit)")
BAREBOX_CMD_HELP_OPT ("-l", "long access (32 bit)")
BAREBOX_CMD_HELP_OPT ("-q", "quad access (64 bit)")
BAREBOX_CMD_HELP_OPT ("-s FILE", "source file (default /dev/mem)")
BAREBOX_CMD_HELP_OPT ("-d FILE", "write file (default /dev/mem)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(memcpy)
	.cmd		= do_memcpy,
	BAREBOX_CMD_DESC("memory copy")
	BAREBOX_CMD_OPTS("[-bwlsd] SRC DEST COUNT")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
	BAREBOX_CMD_HELP(cmd_memcpy_help)
BAREBOX_CMD_END
