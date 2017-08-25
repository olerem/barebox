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

#define ATHR_PHY_MAX 5

static uint8_t athr17_init_flag = 0;

static uint32_t
athrs17_reg_read(struct phy_device *phydev, uint32_t reg_addr)
{
	uint32_t reg_word_addr;
	uint32_t phy_addr, tmp_val, reg_val;
	uint16_t phy_val;
	uint8_t phy_reg;

	/* change reg_addr to 16-bit word address, 32-bit aligned */
	reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

	/* configure register high address */
	phy_addr = 0x18;
	phy_reg = 0x0;
	phy_val = (uint16_t) ((reg_word_addr >> 8) & 0x1ff);  /* bit16-8 of reg address */
	mdiobus_write(phydev->bus,  phy_addr, phy_reg, phy_val);

	/* For some registers such as MIBs, since it is read/clear, we should */
	/* read the lower 16-bit register then the higher one */

	/* read register in lower address */
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	reg_val = (uint32_t) mdiobus_read(phydev->bus, phy_addr, phy_reg);

	/* read register in higher address */
	reg_word_addr++;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	reg_val = (uint32_t) mdiobus_read(phydev->bus, phy_addr, phy_reg);
	reg_val |= (tmp_val << 16);

	return reg_val;
}

static void
athrs17_reg_write(struct phy_device *phydev, uint32_t reg_addr, uint32_t reg_val)
{
	uint32_t reg_word_addr;
	uint32_t phy_addr;
	uint16_t phy_val;
	uint8_t phy_reg;

	/* change reg_addr to 16-bit word address, 32-bit aligned */
	reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

	/* configure register high address */
	phy_addr = 0x18;
	phy_reg = 0x0;
	phy_val = (uint16_t) ((reg_word_addr >> 8) & 0x1ff);  /* bit16-8 of reg address */
	mdiobus_write(phydev->bus,  phy_addr, phy_reg, phy_val);

	/* For some registers such as ARL and VLAN, since they include BUSY bit */
	/* in lower address, we should write the higher 16-bit register then the */
	/* lower one */

	/* read register in higher address */
	reg_word_addr++;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	phy_val = (uint16_t) ((reg_val >> 16) & 0xffff);
	mdiobus_write(phydev->bus,  phy_addr, phy_reg, phy_val);

	/* write register in lower address */
	reg_word_addr--;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	phy_val = (uint16_t) (reg_val & 0xffff);
	mdiobus_write(phydev->bus,  phy_addr, phy_reg, phy_val);
}


static int ar8327n_config_init(struct phy_device *phydev)
{
	int phy_addr = 0;
	int ret;

	printk("%s.%i %x\n", __func__, __LINE__, phydev->interface);
	ret = genphy_config_init(phydev);
	if (ret < 0)
		return ret;

	printk("%s.%i %x\n", __func__, __LINE__, phydev->interface);
	if (phydev->interface != PHY_INTERFACE_MODE_RGMII_TXID)
		return 0;


	/* if using header for register configuration, we have to     */
	/* configure s17 register after frame transmission is enabled */

	if (athr17_init_flag)
		return 0;

	/* configure the RGMII */
	athrs17_reg_write(phydev, 0x624 , 0x7f7f7f7f);
	athrs17_reg_write(phydev, 0x10  , 0x40000000);
	athrs17_reg_write(phydev, 0x4   , 0x07600000);
	athrs17_reg_write(phydev, 0xc   , 0x01000000);
	athrs17_reg_write(phydev, 0x7c  , 0x0000007e);

	/* AR8327/AR8328 v1.0 fixup */
		printk("%s.%i %x\n", __func__, __LINE__, phydev->interface);
	if ((athrs17_reg_read(phydev, 0x0) & 0xffff) == 0x1201) {
		printk("!!!!!%s.%i %x\n", __func__, __LINE__, phydev->interface);
		for (phy_addr = 0x0; phy_addr <= ATHR_PHY_MAX; phy_addr++) {
			/* For 100M waveform */
			mdiobus_write(phydev->bus, phy_addr, 0x1d, 0x0);
			mdiobus_write(phydev->bus, phy_addr, 0x1e, 0x02ea);
			/* Turn On Gigabit Clock */
			mdiobus_write(phydev->bus, phy_addr, 0x1d, 0x3d);
			mdiobus_write(phydev->bus, phy_addr, 0x1e, 0x68a0);
		}
	}

	/* set the WAN Port(Port1) Disable Mode(can not receive or transmit any frames) */
	athrs17_reg_write(phydev, 0x066c, athrs17_reg_read(phydev, 0x066c) & 0xfff8ffff);

	athr17_init_flag = 1;
	printf("%s: complete\n",__func__);

	return 0;
}

static struct phy_driver ar8327n_driver[] = {
{
	/* QCA AR8327N */
	.phy_id		= 0x004dd034,
	.phy_id_mask	= 0xffffffef,
	.drv.name	= "QCA AR8327N switch",
	.config_init	= ar8327n_config_init,
	.features	= PHY_GBIT_FEATURES,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
}};

static int atheros_phy_init(void)
{
	printk("%s.%i\n", __func__, __LINE__);
	return phy_drivers_register(ar8327n_driver,
				    ARRAY_SIZE(ar8327n_driver));
}
fs_initcall(atheros_phy_init);
