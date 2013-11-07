/*
 * MTD SPI driver for ar2315_sf (AR2315 SPI Flash) and serial flash chips.
 *
 * ar2315_sf is a reduced SPI controller and not usable for other SPI
 * devices. Atheros call it SPI Flash controller.
 *
 * Author: Oleksij Rempel <linux@rempel-privat.de>
 *
 * Based on m25p80.c
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <io.h>
#include <clock.h>
#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/flash.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <linux/err.h>
#include <linux/math64.h>
#include <linux/mtd/cfi.h>
#include <linux/mtd/mtd.h>

#include "m25p80.h"

#define AR2315_SF_CTL	0x00
#define AR2315_SF_AO	0x04
#define AR2315_SF_DATA	0x08

#define AR2315_SF_CTL_START		0x00000100
#define AR2315_SF_CTL_BUSY		0x00010000
#define AR2315_SF_CTL_TXCNT_MASK	0x0000000f
#define AR2315_SF_CTL_TXCNT_S		0
#define AR2315_SF_CTL_RXCNT_MASK	0x000000f0
#define AR2315_SF_CTL_RXCNT_S		4
#define AR2315_SF_CTL_TX_RX_CNT_MASK	0x000000ff
#define AR2315_SF_CTL_SIZE_MASK		0x00060000

#define AR2315_SF_CTL_CLK_SEL_MASK	0x03000000

#define STM_PAGE_SIZE		256

struct ar2315_sf_priv {
	struct device_d		*dev;
	struct mtd_info		mtd;
	u16			page_size;
	unsigned		sector_size;
	u16			addr_width;
	u8			erase_opcode;
	u8			erase_opcode_4k;
	u8			*command;
	void __iomem		*spiflash_regs;
};

/* maximum size of ar2315_sf_tx should never be more then 8 Byte */
struct ar2315_sf_tx {
	u8	addr[3];
	u8	opcode;
	u32	data;
} __packed;

static inline struct ar2315_sf_priv *mtd_to_ar2315_sf(struct mtd_info *mtd)
{
	return container_of(mtd, struct ar2315_sf_priv, mtd);
}


static inline void reg_writel(struct ar2315_sf_priv *priv,
		u32 val, u32 reg)
{
	__raw_writel(val, priv->spiflash_regs + reg);
}

static inline u32 reg_readl(struct ar2315_sf_priv *priv, u32 reg)
{
	return __raw_readl(priv->spiflash_regs + reg);
}

static void ar2315_sf_addr2cmd(struct ar2315_sf_priv *priv,
		unsigned int addr, struct ar2315_sf_tx *tx_buf)
{
	tx_buf->addr[0] = addr >> (priv->addr_width * 8 -  8);
	tx_buf->addr[1] = addr >> (priv->addr_width * 8 - 16);
	tx_buf->addr[2] = addr >> (priv->addr_width * 8 - 24);
}

static int ar2315_sf_wait_busy(struct ar2315_sf_priv *priv)
{
	u32 ctrl;
	uint64_t timer_start;

	timer_start = get_time_ns();

	do {
		ctrl = reg_readl(priv, AR2315_SF_CTL);
		if (!(ctrl & AR2315_SF_CTL_BUSY))
			return 0;
	} while (!(is_timeout(timer_start, MAX_READY_WAIT * SECOND)));

	dev_err(priv->dev, "%s: time out\n", __func__);
	return -ETIMEDOUT;
}

static int ar2315_sf_write_then_read(struct ar2315_sf_priv *priv,
	struct ar2315_sf_tx *tx_buf, u8 tx_size, void *rx_buf, u8 rx_size)
{
	u32	ctrl;
	u32	rx_tmp[2], tx_tmp[2];
	int	ret;

	if (tx_size > 8 || rx_size > 8) {
		dev_err(priv->dev, "wrong RX or TX size. Opcode: %x\n",
				tx_buf->opcode);
		return -EIO;
	}

	ret = ar2315_sf_wait_busy(priv);
	if (ret < 0)
		return ret;

	memcpy(&tx_tmp[0], tx_buf, 8);
	/* We can write maximum 8 Bytes: 1 - opcode; 3 - addr; 4 - data. */
	reg_writel(priv, tx_tmp[0], AR2315_SF_AO);
	reg_writel(priv, cpu_to_le32(tx_tmp[1]), AR2315_SF_DATA);

	/* start transfer */
	ctrl = reg_readl(priv, AR2315_SF_CTL);
	ctrl &= ~AR2315_SF_CTL_TX_RX_CNT_MASK;
	ctrl |= rx_size << AR2315_SF_CTL_RXCNT_S;
	ctrl |= tx_size << AR2315_SF_CTL_TXCNT_S;
	ctrl |= AR2315_SF_CTL_START;
	reg_writel(priv, ctrl, AR2315_SF_CTL);

	if (!rx_size || !rx_buf)
		return 0;

	ret = ar2315_sf_wait_busy(priv);
	if (ret < 0)
		return ret;

	rx_tmp[0] = le32_to_cpu(reg_readl(priv, AR2315_SF_DATA));
	/* opcode regiser can be used for data on read */
	rx_tmp[1] = le32_to_cpu(reg_readl(priv, AR2315_SF_AO));

	memcpy(rx_buf, rx_tmp, rx_size);
	return 0;
}

/****************************************************************************/

/*
 * Internal helper functions
 */

/*
 * Read the status register, returning its value in the location
 * Return the status register value.
 * Returns negative if error occurred.
 */
//OK
static int read_sr(struct ar2315_sf_priv *priv)
{
	struct ar2315_sf_tx tx_buf;
	u8 val;
	int ret;

	tx_buf.opcode = OPCODE_RDSR;

	ret = ar2315_sf_write_then_read(priv, &tx_buf, 1, &val, 1);
	if (ret) {
		dev_err(priv->dev, "Error on OPCODE_RDSR\n");
		return ret;
	}

	return val;
}

/*
 * Write status register 1 byte
 */
static void write_sr(struct ar2315_sf_priv *priv, u8 val)
{
	struct ar2315_sf_tx tx_buf;

	tx_buf.opcode = OPCODE_WRSR;
	ar2315_sf_addr2cmd(priv, 0, &tx_buf);
	tx_buf.data = 1;

	ar2315_sf_write_then_read(priv, &tx_buf, 5, NULL, 0);
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static void write_enable(struct ar2315_sf_priv *priv)
{
	struct ar2315_sf_tx tx_buf;

	tx_buf.opcode = OPCODE_WREN;

	ar2315_sf_write_then_read(priv, &tx_buf, 1, NULL, 0);
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int wait_till_ready(struct ar2315_sf_priv *priv)
{
	int sr;
	uint64_t timer_start;

	timer_start = get_time_ns();

	do {
		if ((sr = read_sr(priv)) < 0)
			break;
		else if (!(sr & SR_WIP))
			return 0;

	} while (!(is_timeout(timer_start, MAX_READY_WAIT * SECOND)));

	return -ETIMEDOUT;
}

/*
 * Erase the whole flash memory
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_chip(struct ar2315_sf_priv *priv)
{
	struct ar2315_sf_tx tx_buf;

	dev_dbg(priv->dev, "%s %lldKiB\n",
		__func__, (long long)(priv->mtd.size >> 10));

	if (wait_till_ready(priv))
		return -ETIMEDOUT;

	write_enable(priv);

	tx_buf.opcode = OPCODE_CHIP_ERASE;

	return ar2315_sf_write_then_read(priv, &tx_buf, 1, NULL, 0);
}

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_sector(struct ar2315_sf_priv *priv, u32 addr, u32 opcode)
{
	struct ar2315_sf_tx tx_buf;
	dev_dbg(priv->dev, "%s %dKiB at 0x%08x\n",
		__func__, priv->mtd.erasesize / 1024, addr);

	if (wait_till_ready(priv))
		return -ETIMEDOUT;

	write_enable(priv);

	tx_buf.opcode = opcode;
	ar2315_sf_addr2cmd(priv, addr, &tx_buf);

	ar2315_sf_write_then_read(priv, &tx_buf, 4, NULL, 0);

	return 0;
}

/****************************************************************************/

/*
 * MTD implementation
 */

/*
 * Erase an address range on the flash chip.  The address range may extend
 * one or more erase sectors.  Return an error is there is a problem erasing.
 */
static int ar2315_sf_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct ar2315_sf_priv *priv = mtd_to_ar2315_sf(mtd);
	u32 addr, len;
	uint32_t rem;

	dev_dbg(priv->dev, "%s at 0x%llx, len %lld\n",
			__func__, (long long)instr->addr,
			(long long)instr->len);

	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem)
		return -EINVAL;

	addr = instr->addr;
	len = instr->len;

	if (len == priv->mtd.size) {
		if (erase_chip(priv)) {
			instr->state = MTD_ERASE_FAILED;
			return -EIO;
		}
		return 0;
	}

	if (priv->erase_opcode_4k) {
		while (len && (addr & (priv->sector_size - 1))) {
			if (ctrlc())
				return -EINTR;
			if (erase_sector(priv, addr, priv->erase_opcode_4k))
				return -EIO;
			addr += mtd->erasesize;
			len -= mtd->erasesize;
		}

		while (len >= priv->sector_size) {
			if (ctrlc())
				return -EINTR;
			if (erase_sector(priv, addr, priv->erase_opcode))
				return -EIO;
			addr += priv->sector_size;
			len -= priv->sector_size;
		}

		while (len) {
			if (ctrlc())
				return -EINTR;
			if (erase_sector(priv, addr, priv->erase_opcode_4k))
				return -EIO;
			addr += mtd->erasesize;
			len -= mtd->erasesize;
		}
	} else {
		while (len) {
			if (ctrlc())
				return -EINTR;
			if (erase_sector(priv, addr, priv->erase_opcode))
				return -EIO;

			if (len <= mtd->erasesize)
				break;
			addr += mtd->erasesize;
			len -= mtd->erasesize;
		}
	}

	if (wait_till_ready(priv))
		return -ETIMEDOUT;

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

/*
 * Read an address range from the flash chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
static int ar2315_sf_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct ar2315_sf_priv *priv = mtd_to_ar2315_sf(mtd);
	struct ar2315_sf_tx tx_buf;

	if (!len)
		return 0;

	if (from + len > mtd->size)
		return -EINVAL;

	*retlen = (len > 8) ? 8 : len;

	tx_buf.opcode = OPCODE_NORM_READ;
	ar2315_sf_addr2cmd(priv, from, &tx_buf);

	return ar2315_sf_write_then_read(priv, &tx_buf, 4, buf, *retlen);
}

static int ar2315_sf_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct ar2315_sf_priv *priv = mtd_to_ar2315_sf(mtd);
	struct ar2315_sf_tx tx_buf;
	u32 bytes_left;

	*retlen = 0;
	bytes_left = len;

	if (!len)
		return 0;

	if (to + len > mtd->size)
		return -EINVAL;

	do {
		u32 write_len, page_offset;

		write_len = min(bytes_left, sizeof(u32));

		/* 32-bit writes cannot span across a page boundary
		 * (256 bytes). This types of writes require two page
		 * program operations to handle it correctly. The STM part
		 * will write the overflow data to the beginning of the
		 * current page as opposed to the subsequent page.
		 */
		page_offset = (to & (STM_PAGE_SIZE - 1)) + write_len;

		if (page_offset > STM_PAGE_SIZE)
			write_len -= (page_offset - STM_PAGE_SIZE);

		tx_buf.opcode = OPCODE_PP;
		ar2315_sf_addr2cmd(priv, to, &tx_buf);
		memcpy(&tx_buf.data, buf, write_len);

		write_enable(priv);
		ar2315_sf_write_then_read(priv, &tx_buf, 4 + write_len, NULL, 0);

		if (wait_till_ready(priv))
			return -ETIMEDOUT;

		bytes_left -= write_len;
		to += write_len;
		buf += write_len;

		*retlen += write_len;
	} while (bytes_left != 0);

	return 0;
}

/****************************************************************************/

/*
 * SPI device driver setup and teardown
 */

static const struct spi_device_id *jedec_probe(struct ar2315_sf_priv *priv)
{
	struct ar2315_sf_tx tx_buf;
	int			tmp;
	u8			id[5];
	u32			jedec;
	u16			ext_jedec;
	struct flash_info	*info;
	int ret;

	/* JEDEC also defines an optional "extended device information"
	 * string for after vendor-specific data, after the three bytes
	 * we use here.  Supporting some chips might require using it.
	 */
	tx_buf.opcode = OPCODE_RDID;
	ret = ar2315_sf_write_then_read(priv, &tx_buf, 1, id, 5);
	if (tmp < 0) {
		dev_dbg(priv->dev, "%s: error %d reading JEDEC ID\n",
				dev_name(priv->dev), tmp);
		return NULL;
	}
	jedec = id[0];
	jedec = jedec << 8;
	jedec |= id[1];
	jedec = jedec << 8;
	jedec |= id[2];

	ext_jedec = id[3] << 8 | id[4];

	for (tmp = 0; tmp < ARRAY_SIZE(m25p_ids) - 1; tmp++) {
		info = (void *)m25p_ids[tmp].driver_data;
		if (info->jedec_id == jedec) {
			if (info->ext_id != 0 && info->ext_id != ext_jedec)
				continue;
			return &m25p_ids[tmp];
		}
	}
	dev_err(priv->dev, "unrecognized JEDEC id %06x\n", jedec);
	return ERR_PTR(-ENODEV);
}

static int ar2315_sf_probe(struct device_d *dev)
{
	const struct spi_device_id	*id = NULL;
	struct flash_platform_data	*data;
	struct ar2315_sf_priv		*priv;
	struct flash_info		*info = NULL;
	unsigned			i;
	unsigned			do_jdec_probe = 1;

	priv = xzalloc(sizeof *priv);
	priv->spiflash_regs = dev_request_mem_region(dev, 0);

	dev->priv = (void *)priv;
	priv->dev = dev;
	/* ok */
	/* Platform data helps sort out which chip type we have, as
	 * well as how this board partitions it.  If we don't have
	 * a chip ID, try the JEDEC id commands; they'll work for most
	 * newer chips, even if we don't recognize the particular chip.
	 */
	data = dev->platform_data;
	if (data && data->type) {
		const struct spi_device_id *plat_id;

		for (i = 0; i < ARRAY_SIZE(m25p_ids) - 1; i++) {
			plat_id = &m25p_ids[i];
			if (strcmp(data->type, plat_id->name))
				continue;
			break;
		}

		if (i < ARRAY_SIZE(m25p_ids) - 1) {
			id = plat_id;
			info = (void *)id->driver_data;
			/* If flash type is provided but the memory is not
			 * JEDEC compliant, don't try to probe the JEDEC id */
			if (!info->jedec_id)
				do_jdec_probe = 0;
		} else
			dev_warn(priv->dev, "unrecognized id %s\n", data->type);
	}

	if (do_jdec_probe) {
		const struct spi_device_id *jid;

		jid = jedec_probe(priv);
		if (IS_ERR(jid)) {
			return PTR_ERR(jid);
		} else if (jid != id) {
			/*
			 * JEDEC knows better, so overwrite platform ID. We
			 * can't trust partitions any longer, but we'll let
			 * mtd apply them anyway, since some partitions may be
			 * marked read-only, and we don't want to lose that
			 * information, even if it's not 100% accurate.
			 */
			if (id)
				dev_warn(dev, "found %s, expected %s\n",
					jid->name, id->name);

			id = jid;
			info = (void *)jid->driver_data;
		}
	}

	/*
	 * Atmel, SST and Intel/Numonyx serial flash tend to power
	 * up with the software protection bits set
	 */

	if (JEDEC_MFR(info->jedec_id) == CFI_MFR_ATMEL ||
	    JEDEC_MFR(info->jedec_id) == CFI_MFR_INTEL ||
	    JEDEC_MFR(info->jedec_id) == CFI_MFR_SST) {
		write_enable(priv);
		write_sr(priv, 0);
	}

	if (data && data->name)
		priv->mtd.name = data->name;
	else
		priv->mtd.name = "ar2315_sf";

	priv->mtd.type = MTD_NORFLASH;
	priv->mtd.writesize = 1;
	priv->mtd.flags = MTD_CAP_NORFLASH;
	priv->mtd.size = info->sector_size * info->n_sectors;
	priv->mtd.erase = ar2315_sf_erase;
	priv->mtd.read = ar2315_sf_read;

	priv->mtd.write = ar2315_sf_write;

	/* prefer "small sector" erase if possible */
	if (info->flags & SECT_4K) {
		priv->erase_opcode_4k = OPCODE_BE_4K;
		priv->erase_opcode = OPCODE_SE;
		priv->mtd.erasesize = 4096;
	} else {
		priv->erase_opcode = OPCODE_SE;
		priv->mtd.erasesize = info->sector_size;
	}

	if (info->flags & M25P_NO_ERASE)
		priv->mtd.flags |= MTD_NO_ERASE;

	priv->page_size = info->page_size;
	priv->sector_size = info->sector_size;

	if (info->addr_width && info->addr_width < 4)
		priv->addr_width = info->addr_width;
	else {
		/* enable 4-byte addressing if the device exceeds 16MiB */
		if (priv->mtd.size > 0x1000000)
			dev_info(dev, "Detected 4Byte device on 3Byte bus. "
				      "Use bus default.\n");

		priv->addr_width = 3;
	}

	dev_info(dev, "%s (%lld Kbytes)\n", id->name,
			(long long)priv->mtd.size >> 10);

	dev_dbg(dev, "mtd .name = %s, .size = 0x%llx (%lldMiB) "
		     ".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
		priv->mtd.name,
		(long long)priv->mtd.size, (long long)(priv->mtd.size >> 20),
		priv->mtd.erasesize, priv->mtd.erasesize / 1024,
		priv->mtd.numeraseregions);

	if (priv->mtd.numeraseregions)
		for (i = 0; i < priv->mtd.numeraseregions; i++)
			dev_dbg(dev, "mtd.eraseregions[%d] = { .offset = 0x%llx, "
				".erasesize = 0x%.8x (%uKiB), "
				".numblocks = %d }\n",
				i, (long long)priv->mtd.eraseregions[i].offset,
				priv->mtd.eraseregions[i].erasesize,
				priv->mtd.eraseregions[i].erasesize / 1024,
				priv->mtd.eraseregions[i].numblocks);



	return add_mtd_device(&priv->mtd, priv->mtd.name);
}

static struct driver_d ar2315_sf_driver = {
	.name	= "ar2315_sf",
	.probe	= ar2315_sf_probe,
};
device_platform_driver(ar2315_sf_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksij Rempel");
MODULE_DESCRIPTION("MTD driver for SPI M25Pxx flash chips connected to Atheros ar2315 SPI Flash controller");
