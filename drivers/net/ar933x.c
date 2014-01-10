/*
 * ar933x.c: driver for the Atheros AR933x Ethernet device.
 * This device is build in to SoC on ar933x series.
 * All known of them are big endian.
 *
 * Based on Linux driver:
 *   Copyright (C) 2004 by Sameer Dekate <sdekate@arubanetworks.com>
 *   Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 *   Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 * Ported to Barebox:
 *   Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
/*
 * Known issues:
 * - broadcast packets are not filtered by hardware. On noisy network with
 * lots of bcast packages rx_buffer can be completely filled after. Currently
 * we clear rx_buffer transmit some package.
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <io.h>

#include "ar933x.h"

static inline void ag71xx_check_reg_offset(struct ag71xx *ag, unsigned reg)
{
	switch (reg) {
	case AG71XX_REG_MAC_CFG1 ... AG71XX_REG_MAC_MFL:
	case AG71XX_REG_MAC_IFCTL ... AG71XX_REG_TX_SM:
	case AG71XX_REG_MII_CFG:
		break;

	default:
		BUG();
	}
}

static inline void ag71xx_wr(struct ag71xx *ag, unsigned reg, u32 value)
{
	ag71xx_check_reg_offset(ag, reg);

	__raw_writel(value, ag->mac_base + reg);
	/* flush write */
	(void) __raw_readl(ag->mac_base + reg);
}

static inline u32 ag71xx_rr(struct ag71xx *ag, unsigned reg)
{
	ag71xx_check_reg_offset(ag, reg);

	return __raw_readl(ag->mac_base + reg);
}

static inline void ag71xx_sb(struct ag71xx *ag, unsigned reg, u32 mask)
{
	void __iomem *r;

	ag71xx_check_reg_offset(ag, reg);

	r = ag->mac_base + reg;
	__raw_writel(__raw_readl(r) | mask, r);
	/* flush write */
	(void)__raw_readl(r);
}

static inline void ag71xx_cb(struct ag71xx *ag, unsigned reg, u32 mask)
{
	void __iomem *r;

	ag71xx_check_reg_offset(ag, reg);

	r = ag->mac_base + reg;
	__raw_writel(__raw_readl(r) & ~mask, r);
	/* flush write */
	(void) __raw_readl(r);
}

static inline void dma_writel(struct ag71xx *priv,
		u32 val, int reg)
{
	__raw_writel(val, priv->dma_regs + reg);
}

static inline u32 dma_readl(struct ag71xx *priv, int reg)
{
	return __raw_readl(priv->dma_regs + reg);
}

static inline void eth_writel(struct ag71xx *priv,
		u32 val, int reg)
{
	__raw_writel(val, priv->eth_regs + reg);
}

static inline u32 eth_readl(struct ag71xx *priv, int reg)
{
	return __raw_readl(priv->eth_regs + reg);
}

static inline void phy_writel(struct ag71xx *priv,
		u32 val, int reg)
{
	__raw_writel(val, priv->phy_regs + reg);
}

static inline u32 phy_readl(struct ag71xx *priv, int reg)
{
	return __raw_readl(priv->phy_regs + reg);
}

static void ar933x_reset_bit_(struct ag71xx *priv,
		u32 val, enum reset_state state)
{
	if (priv->reset_bit)
		(*priv->reset_bit)(val, state);
}

static inline int ar933x_desc_empty(struct ar933x_descr *desc)
{
	return (desc->ctrl & DESC_EMPTY) != 0;
}

static void ag71xx_dma_reset(struct ag71xx *ag)
{
	u32 val;
	int i;

	/* stop RX and TX */
	ag71xx_wr(ag, AG71XX_REG_RX_CTRL, 0);
	ag71xx_wr(ag, AG71XX_REG_TX_CTRL, 0);

	/*
	 * give the hardware some time to really stop all rx/tx activity
	 * clearing the descriptors too early causes random memory corruption
	 */
	mdelay(1);

	/* clear descriptor addresses */
	//ag71xx_wr(ag, AG71XX_REG_TX_DESC, ag->stop_desc_dma);
	//ag71xx_wr(ag, AG71XX_REG_RX_DESC, ag->stop_desc_dma);

	/* clear pending RX/TX interrupts */
	for (i = 0; i < 256; i++) {
		ag71xx_wr(ag, AG71XX_REG_RX_STATUS, RX_STATUS_PR);
		ag71xx_wr(ag, AG71XX_REG_TX_STATUS, TX_STATUS_PS);
	}

	/* clear pending errors */
	ag71xx_wr(ag, AG71XX_REG_RX_STATUS, RX_STATUS_BE | RX_STATUS_OF);
	ag71xx_wr(ag, AG71XX_REG_TX_STATUS, TX_STATUS_BE | TX_STATUS_UR);

	val = ag71xx_rr(ag, AG71XX_REG_RX_STATUS);
#if 0
	if (val)
		pr_alert("%s: unable to clear DMA Rx status: %08x\n",
			 ag->dev->name, val);
#endif

	val = ag71xx_rr(ag, AG71XX_REG_TX_STATUS);

	/* mask out reserved bits */
	val &= ~0xff000000;

#if 0
	if (val)
		pr_alert("%s: unable to clear DMA Tx status: %08x\n",
			 ag->dev->name, val);
#endif
}

#define MAC_CFG1_INIT	(MAC_CFG1_RXE | MAC_CFG1_TXE | \
			 MAC_CFG1_SRX | MAC_CFG1_STX)

#define FIFO_CFG0_INIT	(FIFO_CFG0_ALL << FIFO_CFG0_ENABLE_SHIFT)

#define FIFO_CFG4_INIT	(FIFO_CFG4_DE | FIFO_CFG4_DV | FIFO_CFG4_FC | \
			 FIFO_CFG4_CE | FIFO_CFG4_CR | FIFO_CFG4_LM | \
			 FIFO_CFG4_LO | FIFO_CFG4_OK | FIFO_CFG4_MC | \
			 FIFO_CFG4_BC | FIFO_CFG4_DR | FIFO_CFG4_LE | \
			 FIFO_CFG4_CF | FIFO_CFG4_PF | FIFO_CFG4_UO | \
			 FIFO_CFG4_VT)

#define FIFO_CFG5_INIT	(FIFO_CFG5_DE | FIFO_CFG5_DV | FIFO_CFG5_FC | \
			 FIFO_CFG5_CE | FIFO_CFG5_LO | FIFO_CFG5_OK | \
			 FIFO_CFG5_MC | FIFO_CFG5_BC | FIFO_CFG5_DR | \
			 FIFO_CFG5_CF | FIFO_CFG5_PF | FIFO_CFG5_VT | \
			 FIFO_CFG5_LE | FIFO_CFG5_FT | FIFO_CFG5_16 | \
			 FIFO_CFG5_17 | FIFO_CFG5_SF)

static void ag71xx_hw_stop(struct ag71xx *ag)
{
	/* disable all interrupts and stop the rx/tx engine */
	ag71xx_wr(ag, AG71XX_REG_INT_ENABLE, 0);
	ag71xx_wr(ag, AG71XX_REG_RX_CTRL, 0);
	ag71xx_wr(ag, AG71XX_REG_TX_CTRL, 0);
}

static void ag71xx_hw_setup(struct ag71xx *ag)
{
//	struct ag71xx_platform_data *pdata = ag71xx_get_pdata(ag);

	/* setup MAC configuration registers */
	ag71xx_wr(ag, AG71XX_REG_MAC_CFG1, MAC_CFG1_INIT);

	ag71xx_sb(ag, AG71XX_REG_MAC_CFG2,
		  MAC_CFG2_PAD_CRC_EN | MAC_CFG2_LEN_CHECK);

	/* setup max frame length to zero */
	ag71xx_wr(ag, AG71XX_REG_MAC_MFL, 0);

	/* setup FIFO configuration registers */
	ag71xx_wr(ag, AG71XX_REG_FIFO_CFG0, FIFO_CFG0_INIT);
#if 0
	if (pdata->is_ar724x) {
		ag71xx_wr(ag, AG71XX_REG_FIFO_CFG1, pdata->fifo_cfg1);
		ag71xx_wr(ag, AG71XX_REG_FIFO_CFG2, pdata->fifo_cfg2);
	} else {
#endif
		ag71xx_wr(ag, AG71XX_REG_FIFO_CFG1, 0x0fff0000);
		ag71xx_wr(ag, AG71XX_REG_FIFO_CFG2, 0x00001fff);
//	}
	ag71xx_wr(ag, AG71XX_REG_FIFO_CFG4, FIFO_CFG4_INIT);
	ag71xx_wr(ag, AG71XX_REG_FIFO_CFG5, FIFO_CFG5_INIT);
}

static void ag71xx_hw_init(struct ag71xx *ag)
{
	//struct ag71xx_platform_data *pdata = ag71xx_get_pdata(ag);
	u32 reset_mask = ag->reset_mask;

	ag71xx_hw_stop(ag);

#if 0
	if (pdata->is_ar724x) {
		u32 reset_phy = reset_mask;

		reset_phy &= AR71XX_RESET_GE0_PHY | AR71XX_RESET_GE1_PHY;
		reset_mask &= ~(AR71XX_RESET_GE0_PHY | AR71XX_RESET_GE1_PHY);

		ath79_device_reset_set(reset_phy);
		mdelay(50);
		ath79_device_reset_clear(reset_phy);
		mdelay(200);
	}
#endif

	ag71xx_sb(ag, AG71XX_REG_MAC_CFG1, MAC_CFG1_SR);
	udelay(20);

	ar933x_reset_bit_(ag, reset_mask, SET);
	mdelay(100);
	ar933x_reset_bit_(ag, reset_mask, REMOVE);
	mdelay(200);

	ag71xx_hw_setup(ag);

	ag71xx_dma_reset(ag);
}

static void ag71xx_fast_reset(struct ag71xx *ag)
{
//	struct ag71xx_platform_data *pdata = ag71xx_get_pdata(ag);
//	struct net_device *dev = ag->dev;
	u32 reset_mask = ag->reset_mask;
	u32 rx_ds, tx_ds;
	u32 mii_reg;

	reset_mask &= AR71XX_RESET_GE0_MAC | AR71XX_RESET_GE1_MAC;

	mii_reg = ag71xx_rr(ag, AG71XX_REG_MII_CFG);
	rx_ds = ag71xx_rr(ag, AG71XX_REG_RX_DESC);
	tx_ds = ag71xx_rr(ag, AG71XX_REG_TX_DESC);

	ar933x_reset_bit_(ag, reset_mask, SET);
	udelay(10);
	ar933x_reset_bit_(ag, reset_mask, REMOVE);
	udelay(10);

	ag71xx_dma_reset(ag);
	ag71xx_hw_setup(ag);

	/* setup max frame length */
//	ag71xx_wr(ag, AG71XX_REG_MAC_MFL,
//		  ag71xx_max_frame_len(ag->dev->mtu));

	ag71xx_wr(ag, AG71XX_REG_RX_DESC, rx_ds);
	ag71xx_wr(ag, AG71XX_REG_TX_DESC, tx_ds);
	ag71xx_wr(ag, AG71XX_REG_MII_CFG, mii_reg);
}

static void ag71xx_hw_start(struct ag71xx *ag)
{
	/* start RX engine */
	ag71xx_wr(ag, AG71XX_REG_RX_CTRL, RX_CTRL_RXE);

	/* enable interrupts */
//	ag71xx_wr(ag, AG71XX_REG_INT_ENABLE, AG71XX_INT_INIT);
}

static void ag71xx_link_adjust(struct ag71xx *ag)
{
	//struct ag71xx_platform_data *pdata = ag71xx_get_pdata(ag);
	u32 cfg2;
	u32 ifctl;
	u32 fifo5;

#if 0
	if (!ag->link) {
		ag71xx_hw_stop(ag);
		netif_carrier_off(ag->dev);
		if (netif_msg_link(ag))
			pr_info("%s: link down\n", ag->dev->name);
		return;
	}

	//if (pdata->is_ar724x)
	//	ag71xx_fast_reset(ag);
#endif

	cfg2 = ag71xx_rr(ag, AG71XX_REG_MAC_CFG2);
	cfg2 &= ~(MAC_CFG2_IF_1000 | MAC_CFG2_IF_10_100 | MAC_CFG2_FDX);
	cfg2 |= (ag->duplex) ? MAC_CFG2_FDX : 0;

	ifctl = ag71xx_rr(ag, AG71XX_REG_MAC_IFCTL);
	ifctl &= ~(MAC_IFCTL_SPEED);

	fifo5 = ag71xx_rr(ag, AG71XX_REG_FIFO_CFG5);
	fifo5 &= ~FIFO_CFG5_BM;

	switch (ag->speed) {
	case SPEED_1000:
		cfg2 |= MAC_CFG2_IF_1000;
		fifo5 |= FIFO_CFG5_BM;
		break;
	case SPEED_100:
		cfg2 |= MAC_CFG2_IF_10_100;
		ifctl |= MAC_IFCTL_SPEED;
		break;
	case SPEED_10:
		cfg2 |= MAC_CFG2_IF_10_100;
		break;
	default:
		BUG();
		return;
	}

//	if (pdata->is_ar91xx)
//		ag71xx_wr(ag, AG71XX_REG_FIFO_CFG3, 0x00780fff);
//	else if (pdata->is_ar724x)
//		ag71xx_wr(ag, AG71XX_REG_FIFO_CFG3, pdata->fifo_cfg3);
//	else
		ag71xx_wr(ag, AG71XX_REG_FIFO_CFG3, 0x008001ff);

//	if (pdata->set_speed)
//		pdata->set_speed(ag->speed);

	ag71xx_wr(ag, AG71XX_REG_MAC_CFG2, cfg2);
	ag71xx_wr(ag, AG71XX_REG_FIFO_CFG5, fifo5);
	ag71xx_wr(ag, AG71XX_REG_MAC_IFCTL, ifctl);
	ag71xx_hw_start(ag);


}

static int ar933x_set_ethaddr(struct eth_device *edev, unsigned char *addr);

/* ok */
static void ar933x_flash_rxdsc(struct ar933x_descr *rxdsc)
{
	rxdsc->ctrl = DESC_EMPTY;
}

/* ok */
static void ar933x_allocate_dma_descriptors(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	u16 ar933x_descr_size = sizeof(struct ar933x_descr);
	u16 i;

	priv->tx_ring = xmalloc(ar933x_descr_size);
	dev_dbg(&edev->dev, "allocate tx_ring @ %p\n", priv->tx_ring);

	priv->rx_ring = xmalloc(ar933x_descr_size * AR9333_RXDSC_ENTRIES);
	dev_dbg(&edev->dev, "allocate rx_ring @ %p\n", priv->rx_ring);

	priv->rx_buffer = xmalloc(AR9333_RX_BUFSIZE * AR9333_RXDSC_ENTRIES);
	dev_dbg(&edev->dev, "allocate rx_buffer @ %p\n", priv->rx_buffer);

	/* Initialize the rx Descriptors */
	for (i = 0; i < AR9333_RXDSC_ENTRIES; i++) {
		struct ar933x_descr *rxdsc = &priv->rx_ring[i];
		ar933x_flash_rxdsc(rxdsc);
		rxdsc->buffer_ptr =
			(u32)(priv->rx_buffer + AR9333_RX_BUFSIZE * i);
		rxdsc->next_dsc_ptr = (u32)&priv->rx_ring[DSC_NEXT(i)];
	}
	/* set initial position of ring descriptor */
	priv->next_rxdsc = &priv->rx_ring[0];
}

static void ar933x_adjust_link(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	u32 mc;

	if (edev->phydev->duplex != priv->oldduplex) {
		mc = eth_readl(priv, AR933X_ETH_MAC_CONTROL);
		mc &= ~(MAC_CONTROL_F | MAC_CONTROL_DRO);
		if (edev->phydev->duplex)
			mc |= MAC_CONTROL_F;
		else
			mc |= MAC_CONTROL_DRO;
		eth_writel(priv, mc, AR933X_ETH_MAC_CONTROL);
		priv->oldduplex = edev->phydev->duplex;
	}
}

static int ar933x_eth_init(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;

	ar933x_allocate_dma_descriptors(edev);
	ag71xx_hw_init(priv);

	dma_writel(priv, (u32)priv->tx_ring, AG71XX_REG_TX_DESC);
	dma_writel(priv, (u32)priv->rx_ring, AG71XX_REG_RX_DESC);
	//ar933x_reset_regs(edev);
	ar933x_set_ethaddr(edev, priv->mac);
	return 0;
}

static int ar933x_eth_open(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	u32 tmp;

	/* Enable RX. Now the rx_buffer will be filled.
	 * If it is full we may lose first transmission. In this case
	 * barebox should retry it.
	 * Or TODO: - force HW to filter some how broadcasts
	 *			- disable RX if we do not need it. */
	tmp = eth_readl(priv, AR933X_ETH_MAC_CONTROL);
	eth_writel(priv, (tmp | MAC_CONTROL_RE), AR933X_ETH_MAC_CONTROL);

	return phy_device_connect(edev, &priv->miibus, (int)priv->phy_regs,
			ar933x_adjust_link, 0, PHY_INTERFACE_MODE_MII);
}

/* mostly ok */
static int ar933x_eth_recv(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;

#if 0
	status = ag71xx_rr(ag, AG71XX_REG_RX_STATUS);
	if (unlikely(status & RX_STATUS_OF)) {
		ag71xx_wr(ag, AG71XX_REG_RX_STATUS, RX_STATUS_OF);
		dev->stats.rx_fifo_errors++;

		/* restart RX */
		ag71xx_wr(ag, AG71XX_REG_RX_CTRL, RX_CTRL_RXE);
	}
#endif
	while (1) {
		struct ar933x_descr *rxdsc = priv->next_rxdsc;
		u32 status = rxdsc->ctrl;

		/* owned by DMA? */
		if (ar933x_desc_empty(rxdsc))
			break;

		/* FIXME: */
		dma_writel(priv, RX_STATUS_PR, AG71XX_REG_RX_STATUS);
		if (!priv->kill_rx_ring /* FIXME: do we need some thisng to
					   check? */) {
			u16 length = (status & DESC_PKTLEN_M) - ETH_FCS_LEN;
			/* FIXME do we need to remove some thing on buffer? */
			net_receive((void *)rxdsc->buffer_ptr, length);
		}
		/* Clean descriptor. now it is owned by DMA. */
		priv->next_rxdsc = (struct ar933x_descr *)rxdsc->next_dsc_ptr;
		ar933x_flash_rxdsc(rxdsc);
	}
	priv->kill_rx_ring = 0;
	return 0;
}

/* mostly OK */
static int ar933x_eth_send(struct eth_device *edev, void *packet,
		int length)
{
	struct ag71xx *priv = edev->priv;
	struct ar933x_descr *txdsc = priv->tx_ring;
	u32 rx_missed;

	/* We do not do async work.
	 * If rx_ring is full, there is nothing we can use. */
	// FIXME: should be reworked?
#if 0
	rx_missed = dma_readl(priv, AR933X_DMA_RX_MISSED);
	if (rx_missed) {
		priv->kill_rx_ring = 1;
		ar933x_eth_recv(edev);
	}
#endif

	/* Setup the transmit descriptor. */
	txdsc->buffer_ptr = (uint)packet;
	txdsc->ctrl = length & DESC_PKTLEN_M;

	/* Trigger transmission */
	dma_writel(priv, AG71XX_REG_TX_CTRL, TX_CTRL_TXE);

	wait_on_timeout(2000 * MSECOND,
		!ar933x_desc_empty(txdsc));

	/* We can't do match here. If it is still in progress,
	 * then engine is probably stalled or we wait not enough. */
	if (!ar933x_desc_empty(txdsc))
		dev_err(&edev->dev, "Frame is still in progress.\n");

	/* Ready or not. Stop it. FIXME */
	txdsc->ctrl = DESC_EMPTY;
	return 0;
}

static void ar933x_eth_halt(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	u32 tmp;

	/* kill the MAC: disable RX and TX */
	tmp = eth_readl(priv, AR933X_ETH_MAC_CONTROL);
	eth_writel(priv, tmp & ~(MAC_CONTROL_RE | MAC_CONTROL_TE),
			AR933X_ETH_MAC_CONTROL);

	/* stop DMA */
	dma_writel(priv, 0, AR933X_DMA_CONTROL);
	dma_writel(priv, DMA_BUS_MODE_SWR, AR933X_DMA_BUS_MODE);

	/* place PHY and MAC in reset */
	ar933x_reset_bit_(priv, (priv->cfg->reset_mac | priv->cfg->reset_phy), SET);
}

// OK
static int ar933x_get_ethaddr(struct eth_device *edev, unsigned char *addr)
{
	struct ag71xx *priv = edev->priv;

	/* MAC address is stored on flash, in some kind of atheros config
	 * area. Platform code should read it and pass to the driver. */
	memcpy(addr, priv->mac, 6);
	return 0;
}

/**
 * These device do not have build in MAC address.
 * It is located on atheros-config field on flash.
 */
// OK
static int ar933x_set_ethaddr(struct eth_device *edev, unsigned char *addr)
{
	struct ag71xx *ag = edev->priv;
	u32 t;

	t = (((u32) mac[5]) << 24) | (((u32) mac[4]) << 16)
	  | (((u32) mac[3]) << 8) | ((u32) mac[2]);

	ag71xx_wr(ag, AG71XX_REG_MAC_ADDR1, t);

	t = (((u32) mac[1]) << 24) | (((u32) mac[0]) << 16);
	ag71xx_wr(ag, AG71XX_REG_MAC_ADDR2, t);

	mdelay(10);
	return 0;
}

#define MII_ADDR(phy, reg) \
	((reg << MII_ADDR_REG_SHIFT) | (phy << MII_ADDR_PHY_SHIFT))

static int ar933x_miibus_read(struct mii_bus *bus, int phy_id, int regnum)
{
	struct ag71xx *priv = bus->priv;
	uint64_t time_start;

	phy_writel(priv, MII_ADDR(phy_id, regnum), AR933X_ETH_MII_ADDR);
	time_start = get_time_ns();
	while (phy_readl(priv, AR933X_ETH_MII_ADDR) & MII_ADDR_BUSY) {
		if (is_timeout(time_start, SECOND)) {
			dev_err(&bus->dev, "miibus read timeout\n");
			return -ETIMEDOUT;
		}
	}
	return phy_readl(priv, AR933X_ETH_MII_DATA) >> MII_DATA_SHIFT;
}

static int ar933x_miibus_write(struct mii_bus *bus, int phy_id,
			       int regnum, u16 val)
{
	struct ag71xx *priv = bus->priv;
	uint64_t time_start = get_time_ns();

	while (phy_readl(priv, AR933X_ETH_MII_ADDR) & MII_ADDR_BUSY) {
		if (is_timeout(time_start, SECOND)) {
			dev_err(&bus->dev, "miibus write timeout\n");
			return -ETIMEDOUT;
		}
	}
	phy_writel(priv, val << MII_DATA_SHIFT, AR933X_ETH_MII_DATA);
	phy_writel(priv, MII_ADDR(phy_id, regnum) | MII_ADDR_WRITE,
		   AR933X_ETH_MII_ADDR);
	return 0;
}

static int ar933x_mdiibus_reset(struct mii_bus *bus)
{
	struct ag71xx *priv = bus->priv;

	ar933x_reset_regs(&priv->edev);
	return 0;
}

static int ar933x_eth_probe(struct device_d *dev)
{
	struct ag71xx *priv;
	struct eth_device *edev;
	struct mii_bus *miibus;
	struct ar933x_eth_platform_data *pdata;

	if (!dev->platform_data) {
		dev_err(dev, "no platform data\n");
		return -ENODEV;
	}

	pdata = dev->platform_data;

	priv = xzalloc(sizeof(struct ag71xx));
	edev = &priv->edev;
	miibus = &priv->miibus;
	edev->priv = priv;

	/* link all platform depended regs */
	priv->mac = pdata->mac;
	priv->reset_bit = pdata->reset_bit;

	priv->eth_regs = dev_request_mem_region(dev, 0);
	if (priv->eth_regs == NULL) {
		dev_err(dev, "No eth_regs!!\n");
		return -ENODEV;
	}
	/* we have 0x100000 for eth, part of it are dma regs.
	 * So they are already requested */
	priv->dma_regs = (void *)(priv->eth_regs + 0x1000);

	priv->phy_regs = dev_request_mem_region(dev, 1);
	if (priv->phy_regs == NULL) {
		dev_err(dev, "No phy_regs!!\n");
		return -ENODEV;
	}

	priv->cfg = pdata;
	edev->init = ar933x_eth_init;
	edev->open = ar933x_eth_open;
	edev->send = ar933x_eth_send;
	edev->recv = ar933x_eth_recv;
	edev->halt = ar933x_eth_halt;
	edev->get_ethaddr = ar933x_get_ethaddr;
	edev->set_ethaddr = ar933x_set_ethaddr;

	priv->miibus.read = ar933x_miibus_read;
	priv->miibus.write = ar933x_miibus_write;
	priv->miibus.reset = ar933x_mdiibus_reset;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	mdiobus_register(miibus);
	eth_register(edev);

	return 0;
}

static void ar933x_eth_remove(struct device_d *dev)
{

}

static struct driver_d ar933x_eth_driver = {
	.name = "ar933x_eth",
	.probe = ar933x_eth_probe,
	.remove = ar933x_eth_remove,
};

static int ar933x_eth_driver_init(void)
{
	return platform_driver_register(&ar933x_eth_driver);
}
device_initcall(ar933x_eth_driver_init);
