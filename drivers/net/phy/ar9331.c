/*
 * Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <common.h>
#include <init.h>
#include <linux/phy.h>
#include <linux/string.h>

#define ATHR_PHY_MAX	5

/*****************/
/* PHY Registers */
/*****************/
#define ATHR_PHY_CONTROL			0x00
#define ATHR_PHY_STATUS				0x01
#define ATHR_PHY_ID1				0x02
#define ATHR_PHY_ID2				0x03
#define ATHR_AUTONEG_ADVERT			0x04
#define ATHR_LINK_PARTNER_ABILITY		0x05
#define ATHR_AUTONEG_EXPANSION			0x06
#define ATHR_NEXT_PAGE_TRANSMIT			0x07
#define ATHR_LINK_PARTNER_NEXT_PAGE		0x08

#define ATHR_PHY_SPEC_STATUS			0x17

/* Advertisement register. */
#define ATHR_ADVERTISE_ASYM_PAUSE		0x0800
#define ATHR_ADVERTISE_PAUSE			0x0400
#define ATHR_ADVERTISE_100FULL			0x0100
#define ATHR_ADVERTISE_100HALF			0x0080
#define ATHR_ADVERTISE_10FULL			0x0040
#define ATHR_ADVERTISE_10HALF			0x0020

#define ATHR_ADVERTISE_ALL (ATHR_ADVERTISE_ASYM_PAUSE | ATHR_ADVERTISE_PAUSE | \
                            ATHR_ADVERTISE_10HALF | ATHR_ADVERTISE_10FULL | \
                            ATHR_ADVERTISE_100HALF | ATHR_ADVERTISE_100FULL)

/* ATHR_PHY_CONTROL fields */
#define ATHR_CTRL_SOFTWARE_RESET		0x8000
#define ATHR_CTRL_AUTONEGOTIATION_ENABLE	0x1000

/* Phy Specific status fields */
#define ATHR_STATUS_LINK_PASS			BIT(10)

static int ar9331_phy_is_link_alive(struct phy_device *phydev, int phy_addr)
{
	u16 val;

	val = mdiobus_read(phydev->bus, phy_addr, ATHR_PHY_SPEC_STATUS);

	return !!(val & ATHR_STATUS_LINK_PASS);
}

static int ar9331_phy_get_link(struct phy_device *phydev)
{
	int phy_addr;
	int live_links = 0;

	for (phy_addr = 0; phy_addr < ATHR_PHY_MAX; phy_addr++) {
		if (ar9331_phy_is_link_alive(phydev, phy_addr))
			live_links++;
	}

	return (live_links > 0);
}

static int ar9331_phy_config_init(struct phy_device *phydev)
{
	return 0;
}

static int ar9331_phy_read_status(struct phy_device *phydev)
{
	/* for GMAC0 we have only one static mode */
	phydev->speed = SPEED_100;
	phydev->duplex = DUPLEX_FULL;
	phydev->pause = phydev->asym_pause = 0;
	phydev->link = ar9331_phy_get_link(phydev);
	return 0;
}

static int ar9331_phy_config_aneg(struct phy_device *phydev)
{
	return 0;
}

static int ar9331_phy_aneg_done(struct phy_device *phydev)
{
	return BMSR_ANEGCOMPLETE;
}

static struct phy_driver ar9331_phy_driver[] = {
{
	/* Atheros AR9331 */
	.phy_id		= 0x004dd041,
	.phy_id_mask	= 0xffffffff,
	.drv.name	= "Atheros AR9331 integrated switch",
	.config_init	= ar9331_phy_config_init,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &ar9331_phy_config_aneg,
	.read_status	= &ar9331_phy_read_status,
	.aneg_done	= &ar9331_phy_aneg_done,
}};

static int ar9331_phy_init(void)
{
	return phy_drivers_register(ar9331_phy_driver,
				    ARRAY_SIZE(ar9331_phy_driver));
}
fs_initcall(ar9331_phy_init);
