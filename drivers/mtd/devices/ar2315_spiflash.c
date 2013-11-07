/*
 * MTD SPI driver for ar2315sf (AR2315 SPI Flash) (and similar) serial flash chips
 *
 * Author: Mike Lavender, mike@steroidmicros.com
 * Copyright (c) 2005, Intec Automation Inc.
 *
 * Some parts are based on lart.c by Abraham Van Der Merwe
 *
 * Cleaned up and generalized based on mtd_dataflash.c
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
//#include <spi/spi.h>
#include <spi/flash.h>
#include <xfuncs.h>
#include <malloc.h>
#include <errno.h>
#include <linux/err.h>
#include <linux/math64.h>
#include <linux/mtd/cfi.h>
#include <linux/mtd/mtd.h>

#include "m25p80.h"

/* TODO, rename defs */
#define SPI_FLASH_CTL 0x00
#define SPI_FLASH_OPCODE 0x04
#define SPI_FLASH_DATA 0x08

#define SPI_CTL_START           0x00000100
#define SPI_CTL_BUSY            0x00010000
#define SPI_CTL_TXCNT_MASK      0x0000000f
#define SPI_CTL_TXCNT_S		0
#define SPI_CTL_RXCNT_MASK      0x000000f0
#define SPI_CTL_RXCNT_S		4
#define SPI_CTL_TX_RX_CNT_MASK  0x000000ff
#define SPI_CTL_SIZE_MASK       0x00060000

#define SPI_CTL_CLK_SEL_MASK    0x03000000
#define SPI_OPCODE_MASK         0x000000ff

#define STM_PAGE_SIZE		256
#define SPI_NAME_SIZE		32

struct ar2315sf_priv {
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

/* maximum size of ar2315sf_tx should never be more then 8 Byte */
struct ar2315sf_tx {
	u8	opcode;
	u8	addr[3];
	u32	data;
};

static inline struct ar2315sf_priv *mtd_to_ar2315sf(struct mtd_info *mtd)
{
	return container_of(mtd, struct ar2315sf_priv, mtd);
}


static inline void reg_writel(struct ar2315sf_priv *priv,
		u32 val, u32 reg)
{
	__raw_writel(val, priv->spiflash_regs + reg);
}

static inline u32 reg_readl(struct ar2315sf_priv *priv, u32 reg)
{
	return __raw_readl(priv->spiflash_regs + reg);
}

static void ar2315_addr2cmd(struct ar2315sf_priv *priv,
		unsigned int addr, struct ar2315sf_tx *tx_buf)
{
	tx_buf->addr[0] = addr >> (priv->addr_width * 8 -  8);
	tx_buf->addr[1] = addr >> (priv->addr_width * 8 - 16);
	tx_buf->addr[2] = addr >> (priv->addr_width * 8 - 24);
}

static int
ar2315sf_wait_busy(struct ar2315sf_priv *priv)
{
	u32 ctrl;
	uint64_t timer_start;

	timer_start = get_time_ns();

	do {
		ctrl = reg_readl(priv, SPI_FLASH_CTL);
		if (!(ctrl & SPI_CTL_BUSY))
			return 0;
	} while (!(is_timeout(timer_start, MAX_READY_WAIT * SECOND)));

	printk("controller beasy, time out\n");
	return -ETIMEDOUT;
}

static int ar2315sf_write_then_read(struct ar2315sf_priv *priv,
	struct ar2315sf_tx *tx_buf, u8 tx_size, void *rx_buf, u8 rx_size)
{
	u32	ctrl;
	u32	rx_tmp[2], tx_tmp[2];
	int	ret;

	/* TODO, correct error */
	if (tx_size > 8 || rx_size > 8) {
		printk("wrong size!!!!\n");
		return -1;
	}

	ret = ar2315sf_wait_busy(priv);
	if (ret < 0)
		return ret;

	memcpy(&tx_tmp[0], tx_buf, 8);
	/* We can write maximum 8 Bytes: 1 - opcode; 3 - addr; 4 - data. */
	reg_writel(priv, tx_tmp[0], SPI_FLASH_OPCODE);
	reg_writel(priv, tx_tmp[1], SPI_FLASH_DATA);

	/* start transfer */
	ctrl = reg_readl(priv, SPI_FLASH_CTL);
	ctrl &= ~SPI_CTL_TX_RX_CNT_MASK;
	ctrl |= rx_size << SPI_CTL_RXCNT_S;
	ctrl |= tx_size << SPI_CTL_TXCNT_S;
	ctrl |= SPI_CTL_START;
	reg_writel(priv, ctrl, SPI_FLASH_CTL);

	if (!rx_size || !rx_buf)
		return 0;

	/* TODO, recheck endians or do endian conversation */
	rx_tmp[0] = reg_readl(priv, SPI_FLASH_DATA);
	/* opcode regiser can be used for data on read */
	rx_tmp[1] = reg_readl(priv, SPI_FLASH_OPCODE);

	memcpy(rx_buf, &rx_tmp[0], rx_size);
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
static unsigned int read_sr(struct ar2315sf_priv *priv)
{
	struct ar2315sf_tx tx_buf;
	unsigned int val;

	tx_buf.opcode = OPCODE_RDSR;

	ar2315sf_write_then_read(priv, &tx_buf, 1, &val, 1);

	return val;
}

/*
 * Write status register 1 byte
 */
static void write_sr(struct ar2315sf_priv *priv, u8 val)
{
	struct ar2315sf_tx tx_buf;

	tx_buf.opcode = OPCODE_WRSR;
	ar2315_addr2cmd(priv, 0, &tx_buf);
	tx_buf.data = 1;

	printk("TODO: write_sr: currently unsuportd function\n");
	return;

	// ar2315sf_write_then_read(priv, &tx_buf, 8, NULL, 0);
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static void write_enable(struct ar2315sf_priv *priv)
{
	struct ar2315sf_tx tx_buf;

	tx_buf.opcode = OPCODE_WREN;

	ar2315sf_write_then_read(priv, &tx_buf, 1, NULL, 0);
}

/*
 * Send write disble instruction to the chip.
 */
static inline int write_disable(struct ar2315sf_priv *priv)
{
	struct ar2315sf_tx tx_buf;

	tx_buf.opcode = OPCODE_WRDI;

	ar2315sf_write_then_read(priv, &tx_buf, 1, NULL, 0);
	//TODO make error proof
	return 0;
}

#if 0
/*
 * Enable/disable 4-byte addressing mode.
 */
// TODO: do we need this?
static inline int set_4byte(struct ar2315sf_priv *priv, u32 jedec_id, int enable)
{
	switch (JEDEC_MFR(jedec_id)) {
	case CFI_MFR_MACRONIX:
		priv->command[0] = enable ? OPCODE_EN4B : OPCODE_EX4B;
		return spi_write(priv->spi, priv->command, 1);
	default:
		/* Spansion style */
		priv->command[0] = OPCODE_BRWR;
		priv->command[1] = enable << 7;
		return spi_write(priv->spi, priv->command, 2);
	}
}
#endif


/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int wait_till_ready(struct ar2315sf_priv *priv)
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
static int erase_chip(struct ar2315sf_priv *priv)
{
	struct ar2315sf_tx tx_buf;

	dev_dbg(priv->dev, "%s %lldKiB\n",
		__func__, (long long)(priv->mtd.size >> 10));

	/* Wait until finished previous write command. */
	if (wait_till_ready(priv))
		return -ETIMEDOUT;

	/* Send write enable, then erase commands. */
	write_enable(priv);

	tx_buf.opcode = OPCODE_CHIP_ERASE;

	return ar2315sf_write_then_read(priv, &tx_buf, 1, NULL, 0);
}

#if 0
static int ar2315_cmdsz(struct ar2315sf_priv *priv)
{
	return 1 + priv->addr_width;
}
#endif

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_sector(struct ar2315sf_priv *priv, u32 addr, u32 opcode)
{
	struct ar2315sf_tx tx_buf;
	dev_dbg(priv->dev, "%s %dKiB at 0x%08x\n",
		__func__, priv->mtd.erasesize / 1024, addr);

	/* Wait until finished previous write command. */
	if (wait_till_ready(priv))
		return -ETIMEDOUT;

	/* Send write enable, then erase commands. */
	write_enable(priv);

	tx_buf.opcode = opcode;
	// TODO: convert offset to correct value
	ar2315_addr2cmd(priv, addr, &tx_buf);

	ar2315sf_write_then_read(priv, &tx_buf, 4, NULL, 0);

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
// OK
static int ar2315_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct ar2315sf_priv *priv = mtd_to_ar2315sf(mtd);
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

	/* whole-chip erase? */
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
// OK
static int ar2315_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct ar2315sf_priv *priv = mtd_to_ar2315sf(mtd);
	struct ar2315sf_tx tx_buf;

	if (!len)
		return 0;

	if ((from + len > mtd->size) || (len > 8))
		return -EINVAL;

	tx_buf.opcode = OPCODE_NORM_READ;
	// TODO: convert offset to correct value
	ar2315_addr2cmd(priv, from, &tx_buf);

	/* we can't validate readed data. it will be always equal the requested 
	 * lenght */
	*retlen = len;

	return ar2315sf_write_then_read(priv, &tx_buf, 4, buf, len);
}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int ar2315_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct ar2315sf_priv *priv = mtd_to_ar2315sf(mtd);
	struct ar2315sf_tx tx_buf;
	u32 bytes_left;

	*retlen = 0;

	if (!len)
		return 0;

	if (to + len > mtd->size)
		return -EINVAL;

	write_enable(priv);
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

		write_enable(priv);

		tx_buf.opcode = OPCODE_PP;
		//TODO addr conversation?
		ar2315_addr2cmd(priv, to, &tx_buf);
		//TODO data conversation?
		memcpy(&tx_buf.data, buf, write_len);

		ar2315sf_write_then_read(priv, &tx_buf, write_len, NULL, 0);

		bytes_left -= write_len;
		to += write_len;
		buf += write_len;

		*retlen += write_len;
	} while (bytes_left != 0);

	write_disable(priv);
	return 0;
}

#if 0
static int sst_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	struct ar2315sf_priv *priv = mtd_to_ar2315sf(mtd);
	struct spi_transfer t[2];
	struct spi_message m;
	size_t actual;
	int cmd_sz, ret;

	dev_dbg(&priv->spi->dev, "%s to 0x%08x, len %zd\n",
			__func__, (u32)to, len);

	spi_message_init(&m);
	memset(t, 0, (sizeof t));

	t[0].tx_buf = priv->command;
	t[0].len = ar2315_cmdsz(priv);
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = buf;
	spi_message_add_tail(&t[1], &m);

	/* Wait until finished previous write command. */
	ret = wait_till_ready(priv);
	if (ret)
		goto time_out;

	write_enable(priv);

	actual = to % 2;
	/* Start write from odd address. */
	if (actual) {
		priv->command[0] = OPCODE_BP;
		ar2315_addr2cmd(priv, to, priv->command);

		/* write one byte. */
		t[1].len = 1;
		spi_sync(priv->spi, &m);
		ret = wait_till_ready(priv);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - ar2315_cmdsz(priv);
	}
	to += actual;

	priv->command[0] = OPCODE_AAI_WP;
	ar2315_addr2cmd(priv, to, priv->command);

	/* Write out most of the data here. */
	cmd_sz = ar2315_cmdsz(priv);
	for (; actual < len - 1; actual += 2) {
		t[0].len = cmd_sz;
		/* write two bytes. */
		t[1].len = 2;
		t[1].tx_buf = buf + actual;

		spi_sync(priv->spi, &m);
		ret = wait_till_ready(priv);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - cmd_sz;
		cmd_sz = 1;
		to += 2;
	}
	write_disable(priv);
	ret = wait_till_ready(priv);
	if (ret)
		goto time_out;

	/* Write out trailing byte if it exists. */
	if (actual != len) {
		write_enable(priv);
		priv->command[0] = OPCODE_BP;
		ar2315_addr2cmd(priv, to, priv->command);
		t[0].len = ar2315_cmdsz(priv);
		t[1].len = 1;
		t[1].tx_buf = buf + actual;

		spi_sync(priv->spi, &m);
		ret = wait_till_ready(priv);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - ar2315_cmdsz(priv);
		write_disable(priv);
	}

time_out:
	return ret;
}
#endif


/****************************************************************************/

/*
 * SPI device driver setup and teardown
 */

static const struct spi_device_id *jedec_probe(struct ar2315sf_priv *priv)
{
	struct ar2315sf_tx tx_buf;
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
	ret = ar2315sf_write_then_read(priv, &tx_buf, 1, id, 5);
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

/* mostly ok */
static int ar2315_probe(struct device_d *dev)
{
	const struct spi_device_id	*id = NULL;
	struct flash_platform_data	*data;
	struct ar2315sf_priv		*priv;
	struct flash_info		*info = NULL;
	unsigned			i;
	unsigned			do_jdec_probe = 1;

	priv = xzalloc(sizeof *priv);
//	priv->command = xmalloc(MAX_CMD_SIZE);
	priv->spiflash_regs = (void __iomem *)0xb1300000;

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

	/* ok */

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
		priv->mtd.name = "ar2315sf";

	priv->mtd.type = MTD_NORFLASH;
	priv->mtd.writesize = 1;
	priv->mtd.flags = MTD_CAP_NORFLASH;
	priv->mtd.size = info->sector_size * info->n_sectors;
	priv->mtd.erase = ar2315_erase;
	priv->mtd.read = ar2315_read;

	/* sst flash chips use AAI word program */
//	if (IS_ENABLED(CONFIG_MTD_SST25L) && JEDEC_MFR(info->jedec_id) == CFI_MFR_SST)
//		priv->mtd.write = sst_write;
//	else
		priv->mtd.write = ar2315_write;

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

//	priv->mtd.parent = &spi->dev;
	priv->page_size = info->page_size;
	priv->sector_size = info->sector_size;

	if (info->addr_width && info->addr_width < 4)
		priv->addr_width = info->addr_width;
	else {
		/* enable 4-byte addressing if the device exceeds 16MiB */
		if (priv->mtd.size > 0x1000000)
			dev_info(dev, "Detected 4Byte device on 3Byte bus. Use bus default.\n");

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

static struct driver_d ar2315sf_driver = {
	.name	= "ar2315sf",
	.probe	= ar2315_probe,
};
device_platform_driver(ar2315sf_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksij Rempel");
MODULE_DESCRIPTION("MTD driver for ST M25Pxx flash chips connected to Atheros ar2315 SPI Flash controller");
