/*
 * Copyright (C) 2010 Juergen Beisert, Pengutronix <kernel@pengutronix.de>
 * Copyright (C) 2011 Marc Kleine-Budde, Pengutronix <mkl@pengutronix.de>
 * Copyright (C) 2011 Wolfram Sang, Pengutronix <w.sang@pengutronix.de>
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
 */

#include <common.h>
#include <environment.h>
#include <errno.h>
#include <platform_data/eth-fec.h>
#include <gpio.h>
#include <init.h>
#include <mci.h>
#include <io.h>
#include <net.h>

#include <mach/clock.h>
#include <mach/imx-regs.h>
#include <mach/iomux-imx28.h>
#include <mach/mci.h>
#include <mach/fb.h>
#include <mach/iomux.h>
#include <mach/ocotp.h>
#include <mach/devices.h>
#include <mach/usb.h>
#include <usb/fsl_usb2.h>
#include <spi/spi.h>

#include <asm/armlinux.h>
#include <asm/mmu.h>

#include <generated/mach-types.h>

#define MX28EVK_FEC_PHY_RESET_GPIO	141

static void mx28_evk_get_ethaddr(void)
{
	u8 mac_ocotp[3], mac[6];
	int ret;

	ret = mxs_ocotp_read(mac_ocotp, 3, 0);
	if (ret != 3) {
		pr_err("Reading MAC from OCOTP failed!\n");
		return;
	}

	mac[0] = 0x00;
	mac[1] = 0x04;
	mac[2] = 0x9f;
	mac[3] = mac_ocotp[2];
	mac[4] = mac_ocotp[1];
	mac[5] = mac_ocotp[0];

	eth_register_ethaddr(0, mac);
}

static void __init mx28_evk_fec_reset(void)
{
	mdelay(1);
	gpio_set_value(MX28EVK_FEC_PHY_RESET_GPIO, 1);
}

static int mx28_evk_devices_init(void)
{
	if (!of_machine_is_compatible("fsl,imx28-evk"))
		return 0;

	mx28_evk_get_ethaddr(); /* must be after registering ocotp */

	mx28_evk_fec_reset();

	return 0;
}
device_initcall(mx28_evk_devices_init);
