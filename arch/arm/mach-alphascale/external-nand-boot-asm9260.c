/*
 * NAND controller driver for Alphascale ASM9260, which is probably
 * based on Evatronix NANDFLASH-CTRL IP (version unknown)
 *
 * Copyright (C), 2014 Oleksij Rempel <linux@rempel-privat.de>
 *
 * Inspired by asm9260_nand.c,
 *	Copyright (C), 2007-2013, Alphascale Tech. Co., Ltd.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <mach/nand_search.h>
#include <mach/nand.h>
#include <string.h>

#define ASM9260_ECC_STEP		512
#define ASM9260_ECC_MAX_BIT		16
#define ASM9260_MAX_CHIPS		2

#define HW_CMD				0x00
#define BM_CMD_CMD2_S			24
#define BM_CMD_CMD1_S			16
#define BM_CMD_CMD0_S			8
/* 0 - ADDR0, 1 - ADDR1 */
#define BM_CMD_ADDR1			BIT(7)
/* 0 - PIO, 1 - DMA */
#define BM_CMD_DMA			BIT(6)
#define BM_CMD_CMDSEQ_S			0
/*
 * ASM9260 Sequences:
 * SEQ0:  single command, wait for RnB
 * SEQ1:  send  cmd, addr, wait tWHR, fetch data
 * SEQ2:  send  cmd, addr, wait RnB, fetch data
 * SEQ3:  send  cmd, addr, wait tADL, send data, wait RnB
 * SEQ4:  send  cmd, wait tWHR, fetch data
 * SEQ5:  send  cmd, 3 x addr, wait tWHR, fetch data
 * SEQ6:  wait tRHW, send  cmd, 2 x addr, cmd, wait tCCS, fetch data
 * SEQ7:  wait tRHW, send  cmd, 35 x addr, cmd, wait tCCS, fetch data
 * SEQ8:  send  cmd, 2 x addr, wait tCCS, fetch data
 * SEQ9:  send  cmd, 5 x addr, wait RnB
 * SEQ10: send  cmd, 5 x addr, cmd, wait RnB, fetch data
 * SEQ11: send  cmd, wait RnB, fetch data
 * SEQ12: send  cmd, 5 x addr, wait tADL, send data, cmd
 * SEQ13: send  cmd, 5 x addr, wait tADL, send data
 * SEQ14: send  cmd, 3 x addr, cmd, wait RnB
 * SEQ15: send  cmd, 5 x addr, cmd, 5 x addr, cmd, wait RnB, fetch data
 * SEQ17: send  cmd, 5 x addr, wait RnB, fetch data
*/
#define  SEQ0				0x00
#define  SEQ1				0x21
#define  SEQ2				0x22
#define  SEQ3				0x03
#define  SEQ4				0x24
#define  SEQ5				0x25
#define  SEQ6				0x26
#define  SEQ7				0x27
#define  SEQ8				0x08
#define  SEQ9				0x29
#define  SEQ10				0x2a
#define  SEQ11				0x2b
#define  SEQ12				0x0c
#define  SEQ13				0x0d
#define  SEQ14				0x0e
#define  SEQ15				0x2f
#define  SEQ17				0x15

#define HW_CTRL				0x04
#define BM_CTRL_DIS_STATUS		BIT(23)
#define BM_CTRL_READ_STAT		BIT(22)
#define BM_CTRL_SMALL_BLOCK_EN		BIT(21)
#define BM_CTRL_ADDR_CYCLE1_S		18
#define  ADDR_CYCLE_0			0x0
#define  ADDR_CYCLE_1			0x1
#define  ADDR_CYCLE_2			0x2
#define  ADDR_CYCLE_3			0x3
#define  ADDR_CYCLE_4			0x4
#define  ADDR_CYCLE_5			0x5
#define BM_CTRL_ADDR1_AUTO_INCR		BIT(17)
#define BM_CTRL_ADDR0_AUTO_INCR		BIT(16)
#define BM_CTRL_WORK_MODE		BIT(15)
#define BM_CTRL_PORT_EN			BIT(14)
#define BM_CTRL_LOOKU_EN		BIT(13)
#define BM_CTRL_IO_16BIT		BIT(12)
/* Overwrite BM_CTRL_PAGE_SIZE with HW_DATA_SIZE */
#define BM_CTRL_CUSTOM_PAGE_SIZE	BIT(11)
#define BM_CTRL_PAGE_SIZE_S		8
#define BM_CTRL_PAGE_SIZE(x)		((ffs((x) >> 8) - 1) & 0x7)
#define  PAGE_SIZE_256B			0x0
#define  PAGE_SIZE_512B			0x1
#define  PAGE_SIZE_1024B		0x2
#define  PAGE_SIZE_2048B		0x3
#define  PAGE_SIZE_4096B		0x4
#define  PAGE_SIZE_8192B		0x5
#define  PAGE_SIZE_16384B		0x6
#define  PAGE_SIZE_32768B		0x7
#define BM_CTRL_BLOCK_SIZE_S		6
#define BM_CTRL_BLOCK_SIZE(x)		((ffs((x) >> 5) - 1) & 0x3)
#define  BLOCK_SIZE_32P			0x0
#define  BLOCK_SIZE_64P			0x1
#define  BLOCK_SIZE_128P		0x2
#define  BLOCK_SIZE_256P		0x3
#define BM_CTRL_ECC_EN			BIT(5)
#define BM_CTRL_INT_EN			BIT(4)
#define BM_CTRL_SPARE_EN		BIT(3)
/* same values as BM_CTRL_ADDR_CYCLE1_S */
#define BM_CTRL_ADDR_CYCLE0_S		0

#define HW_STATUS			0x08
#define	BM_CTRL_NFC_BUSY		BIT(8)
/* MEM1_RDY (BIT1) - MEM7_RDY (BIT7) */
#define	BM_CTRL_MEM0_RDY		BIT(0)

#define HW_INT_MASK			0x0c
#define HW_INT_STATUS			0x10
#define BM_INT_FIFO_ERROR		BIT(12)
#define BM_INT_MEM_RDY_S		4
/* MEM1_RDY (BIT5) - MEM7_RDY (BIT11) */
#define BM_INT_MEM0_RDY			BIT(4)
#define BM_INT_ECC_TRSH_ERR		BIT(3)
#define BM_INT_ECC_FATAL_ERR		BIT(2)
#define BM_INT_CMD_END			BIT(1)

#define HW_ECC_CTRL			0x14
/* bits per 512 bytes */
#define	BM_ECC_CAP_S			5
/* support ecc strength 2, 4, 6, 8, 10, 12, 14, 16. */
#define BM_ECC_CAPn(x)			((((x) >> 1) - 1) & 0x7)
/* Warn if some bitflip level (threshold) reached. Max 15 bits. */
#define BM_ECC_ERR_THRESHOLD_S		8
#define BM_ECC_ERR_THRESHOLD_M		0xf
#define BM_ECC_ERR_OVER			BIT(2)
/* Uncorrected error. */
#define BM_ECC_ERR_UNC			BIT(1)
/* Corrected error. */
#define BM_ECC_ERR_CORRECT		BIT(0)

#define HW_ECC_OFFSET			0x18
#define HW_ADDR0_0			0x1c
#define HW_ADDR1_0			0x20
#define HW_ADDR0_1			0x24
#define HW_ADDR1_1			0x28
#define HW_SPARE_SIZE			0x30
#define HW_DMA_ADDR			0x64
#define HW_DMA_CNT			0x68

#define HW_DMA_CTRL			0x6c
#define BM_DMA_CTRL_START		BIT(7)
/* 0 - to device; 1 - from device */
#define BM_DMA_CTRL_FROM_DEVICE		BIT(6)
/* 0 - software maneged; 1 - scatter-gather */
#define BM_DMA_CTRL_SG			BIT(5)
#define BM_DMA_CTRL_BURST_S		2
#define  DMA_BURST_INCR4		0x0
#define  DMA_BURST_STREAM		0x1
#define  DMA_BURST_SINGLE		0x2
#define  DMA_BURST_INCR			0x3
#define  DMA_BURST_INCR8		0x4
#define  DMA_BURST_INCR16		0x5
#define BM_DMA_CTRL_ERR			BIT(1)
#define BM_DMA_CTRL_RDY			BIT(0)

#define HW_MEM_CTRL			0x80
#define	BM_MEM_CTRL_WP_STATE_MASK	0xff00
#define	BM_MEM_CTRL_UNWPn(x)		(1 << ((x) + 8))
#define BM_MEM_CTRL_CEn(x)		(((x) & 7) << 0)

/* BM_CTRL_CUSTOM_PAGE_SIZE should be set */
#define HW_DATA_SIZE			0x84
#define HW_READ_STATUS			0x88
#define HW_TIM_SEQ_0			0x8c
#define HW_TIMING_ASYN			0x90
#define HW_TIMING_SYN			0x94

#define HW_FIFO_DATA			0x98
#define HW_TIME_MODE			0x9c
#define HW_FIFO_INIT			0xb0
/*
 * Counter for ecc related errors.
 * For each 512 byte block it has 5bit counter.
 */
#define HW_ECC_ERR_CNT			0xb8

#define HW_TIM_SEQ_1			0xc8


static unsigned char NandAddr[32];
nand_info alp_nandinfo;
u32 ecc_threshold = ECC_THRESHOLD_4;
u32 ecc_cap = ECC_CAP_4;
u32 nand_spare_size = 36;

#define MemClr(p,n) memset(p,0,n)

static int NandWaitForControllerReady(void)
{
	int ret = 0;
	int waittime = 0;
	
	while (ioread32(NAND_STATUS) & (1UL << 8))
	{
		waittime++;
		if (waittime > 0x1000000)
		{
			ret = -1;
			break;
		}
	}
	return ret;
}

static int NandWaitForDeviceReady(u8 nChip)
{
	int ret = 0;
	int waittime = 0;
	
	while (!(ioread32(NAND_STATUS) & (1UL << nChip)))
	{
		waittime++;
		if (waittime > 0x1000000)
		{
			ret = -1;
			break;
		}
	}
	return ret;
}

static __inline nand_info *GetNandInfo(void)
{
	return (nand_info *)(&alp_nandinfo);
}

static void NandReadBuffer(unsigned char *pBuffer, u32 nLen)
{
	u32 i;
	u32 *tmpbuf = (u32 *)pBuffer;

	for (i = 0; i < (nLen>>2); i++)
	{
		tmpbuf[i] = ioread32(NAND_FIFO_DATA);
	}

	if ((nLen - i * 4) > 0)
	{
		tmpbuf[i] = ioread32(NAND_FIFO_DATA);
	}
}

int NandFlashReset(u8 nChip)
{
    iowrite32((0xff00 | nChip), NAND_MEM_CTRL);
    iowrite32( (RESET << 8)
        | (ADDR_SEL_0 << 7)
        | (INPUT_SEL_BIU << 6)
        | (SEQ0), NAND_COMMAND);

	return NandWaitForDeviceReady(nChip);
}

static int NandTimingConf(void)
{
	int ret = 0;
	u32 twhr;
	u32 trhw;
	u32 trwh;
	u32 trwp;
	u32 tadl = TADL;
	u32 tccs = TCCS;
	u32 tsync = TSYNC;
	u32 trr = TRR;
	u32 twb = TWB;

	/*default config before read id*/
	iowrite32((ADDR_CYCLE_1 << 18) 		| (ADDR1_AUTO_INCR_DIS << 17)
	   | (ADDR0_AUTO_INCR_DIS << 16)	| (WORK_MODE_ASYNC << 15)
	   | (PROT_DIS << 14) 			| (LOOKUP_DIS << 13)
	   | (IO_WIDTH_8 << 12) 		 	| (DATA_SIZE_CUSTOM << 11)
	   | (PAGE_SIZE_4096B << 8) 		| (BLOCK_SIZE_32P << 6)
	   | (ECC_DIS << 5) 				| (INT_DIS << 4)
	   | (SPARE_DIS << 3) 			| (ADDR_CYCLE_1), NAND_CONTROL); 
#if 0
    // init timing registers
    trwh = 8;
    trwp = 8;
    iowrite32((trwh << 4) |     // after power on ,device is in async mode0, twh >= 50
         (trwp), NAND_TIMINGS_ASYN);        // twp >= 50 and twhr >= 120

    ret = NandFlashReset(0);
	if (ret != 0)
	{
		return ret;
	}
#endif	
    trwh = 1; //TWH;
    trwp = 1; //TWP;
    iowrite32((trwh << 4) |     // timing mode change to mode 5, twh >= 7
         (trwp), NAND_TIMINGS_ASYN);      // twp >= 10

    twhr = 2;
    trhw = 4;
    iowrite32((twhr << 24) |
         (trhw << 16) |
         (tadl << 8)  |
         (tccs), NAND_TIME_SEQ_0);
    iowrite32((tsync << 16) |
         (trr << 9) |
         (twb), NAND_TIME_SEQ_1);

	return ret;
}

static void NandSelectChip(unsigned char nChip)
{
    iowrite32((0xff00 |  nChip), NAND_MEM_CTRL);
    iowrite32((1UL << (nChip+8)) ^ ioread32(NAND_MEM_CTRL), NAND_MEM_CTRL);// clear WP reg
}

#define	YAFFS_SPARE_MIN_SIZE	(2+28)

void select_ecc(int page_size,int oob_size)
{
	switch(ECC_CAP_16)
	{
		case ECC_CAP_16:
			if((26*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_15;
				ecc_cap = ECC_CAP_16;
				nand_spare_size = oob_size - 26*(page_size/512);
				break;
			}
		case ECC_CAP_14:
			if((23*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_14;
				ecc_cap = ECC_CAP_14;
				nand_spare_size = oob_size - 23*(page_size/512);
				break;
			}
		case ECC_CAP_12:
			if((20*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_12;
				ecc_cap = ECC_CAP_12;
				nand_spare_size = oob_size - 20*(page_size/512);
				break;
			}
		case ECC_CAP_10:
			if((17*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_10;
				ecc_cap = ECC_CAP_10;
				nand_spare_size = oob_size - 17*(page_size/512);
				break;
			}
		case ECC_CAP_8:
			if((13*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_8;
				ecc_cap = ECC_CAP_8;
				nand_spare_size = oob_size - 13*(page_size/512);
				break;
			}
		case ECC_CAP_6:
			if((10*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_6;
				ecc_cap = ECC_CAP_6;
				nand_spare_size = oob_size - 10*(page_size/512);
				break;
			}
#if 0
		case ECC_CAP_4:
			if((7*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_4;
				ecc_cap = ECC_CAP_4;
				nand_spare_size = oob_size - 7*(page_size/512);
				break;
			}

		case ECC_CAP_2:
			if((4*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_2;
				ecc_cap = ECC_CAP_2;
				nand_spare_size = oob_size - 4*(page_size/512);
				break;
			}
#endif
		default:
			if((7*(page_size/512)+YAFFS_SPARE_MIN_SIZE)<=oob_size)
			{	
				ecc_threshold = ECC_THRESHOLD_4;
				ecc_cap = ECC_CAP_4;
				nand_spare_size = oob_size - 7*(page_size/512);
				break;
			}
			break;

	}
}

int NandInit(unsigned char nChip)
{
	int ret = 0;
	nand_info *pNandInfo = GetNandInfo();

	/*打开时钟*/
	iowrite32(0x00000400, (0x80040030+4));  // open nand pclk    
	iowrite32(0x00000008, 0x800401D4);      // set nand clk to 1/2 pclk

	
	//NandSetPinMux();				/*设置PIN Mux*/
	NandSelectChip(nChip);		/*选择NAND芯片*/
	ret = NandTimingConf();		/*设置NAND控制器时序*/
	if (ret != 0)
	{
		return ret;
	}
	
	ret = NandFlashReset(nChip);	/*复位*/

	NandSearch(0, pNandInfo);

	select_ecc(pNandInfo->page_size, pNandInfo->oob_size);

	return ret;
}

static void NandLargePageMakeAddr(u32 nPage, u32 nColumn, unsigned char *pAddr)
{
	int i = 0;
	u32 row_addr = nPage;
	nand_info *pNandInfo = GetNandInfo();

	memset(pAddr, 0, pNandInfo->addr_cycles);

	for (i=0; i<pNandInfo->col_cycles; i++)
	{
		pAddr[i] = (unsigned char)(nColumn & 0xFF);
		nColumn = nColumn >> 8;
	}

	for (i = pNandInfo->col_cycles; i < pNandInfo->addr_cycles; i++)
	{
		pAddr[i] = (unsigned char)(row_addr & 0xFF);		//字节掩码
		row_addr = row_addr >> 8;				//字节位数
	}
}

int NandReadIDPio(unsigned char nChip, unsigned int *pDevId, unsigned int nByte)
{
	int ret = 0;

	iowrite32(1, NAND_FIFO_INIT);		/*复位FIFO*/
	iowrite32(nByte, NAND_DATA_SIZE);
	iowrite32((0xff00 | nChip), NAND_MEM_CTRL);
	iowrite32(0x00000000, NAND_ADDR0_L);
	iowrite32((READ_ID << 8)
	   | (ADDR_SEL_0 << 7)
	   | (INPUT_SEL_BIU << 6)
	   | (SEQ1), NAND_COMMAND);
	ret = NandWaitForDeviceReady(nChip);

	pDevId[0] = ioread32(NAND_FIFO_DATA);

	return ret;
}

static void NandControllerConfig (void)
{
	nand_info *pNandInfo = GetNandInfo();

	iowrite32((EN_STATUS << 23)				| (NO_RNB_SEL << 22)
	   | (BIG_BLOCK_EN << 21)	 		| (pNandInfo->addr_cycles << 18)
	   | (ADDR1_AUTO_INCR_DIS << 17) 	| (ADDR0_AUTO_INCR_DIS << 16)
 	   | (WORK_MODE_ASYNC << 15)	 	| (PROT_DIS << 14)
	   | (LOOKUP_DIS << 13)			 	| (IO_WIDTH_8 << 12)
	   | (DATA_SIZE_FULL_PAGE << 11) 	| (((pNandInfo->page_shift - 8) & 0x7) << 8)
	   | (((pNandInfo->block_shift - 5) & 0x3) << 6)	 | (ECC_EN<< 5)
	   | (INT_DIS << 4)				 	| (SPARE_EN << 3)
	   | (pNandInfo->addr_cycles), NAND_CONTROL);

}

static int NandEccCheck(void)
{
	int ret = 0;
	u32 ecc_status = 0;

	ecc_status = ioread32(NAND_ECC_CTRL);
	if (ecc_status & 0x01)
	{
		//puts("NAND_ERR_CORRECT!!!\r\n");
		ret = 0;
	}
	if (ecc_status & 0x02)
	{
		//puts("NAND_ERR_UNCORRECT!!!\r\n");
		ret = -2;
	}
	if (ecc_status & 0x04)
	{
		//puts("NAND_ERR_OVER!!!\r\n");
		ret = -4;
	}

	return ret;
}

static int NandReadLargePagePio(u32 nPage, u32 nColumn, unsigned char *pBuffer, u32 nLen)
{
	int ret = 0;
	unsigned char *tmpbuf = pBuffer;
	u32 *addr = (u32 *)NandAddr;
	nand_info *pNandInfo = GetNandInfo();

	iowrite32(0x1, NAND_FIFO_INIT);
	NandControllerConfig( );

	if (nColumn == 0)
	{
		iowrite32(ioread32(NAND_CONTROL) & (~(SPARE_EN << 3)), NAND_CONTROL);
		iowrite32((ecc_threshold << 8) | (ecc_cap << 5), NAND_ECC_CTRL);
		iowrite32(pNandInfo->page_size + nand_spare_size, NAND_ECC_OFFSET);
	}
	else if (nColumn == pNandInfo->page_size)
	{
		iowrite32((ioread32(NAND_CONTROL) & (~(ECC_EN << 5))) | (DATA_SIZE_CUSTOM<< 11), NAND_CONTROL);
		iowrite32(nLen, NAND_SPARE_SIZE);
		iowrite32(nLen, NAND_DATA_SIZE);
	}

	iowrite32((0xff00 | pNandInfo->chip_select), NAND_MEM_CTRL);
	NandLargePageMakeAddr(nPage, nColumn, NandAddr);
	iowrite32(addr[0], NAND_ADDR0_L);
	iowrite32(addr[1], NAND_ADDR0_H);

	iowrite32((READ_PAGE_2<<16)
	   | (READ_PAGE_1<<8)
	   | (ADDR_SEL_0<<7)
	   | (INPUT_SEL_BIU<<6)
	   | (SEQ10), NAND_COMMAND);

	ret = NandWaitForControllerReady();
	if (ret != 0)
	{
		return ret;
	}

	NandReadBuffer(tmpbuf, nLen);

	if ((ioread32(NAND_CONTROL) & (ECC_EN<<5)) != 0)
	{
		ret = NandEccCheck( );
	}
	return ret;
}

static int nand_block_is_bad(int page)
{
	int ret = 0;
	char bad_byte[4];
	nand_info *pNandInfo = GetNandInfo();

	bad_byte[0] = bad_byte[1] = bad_byte[2] = bad_byte[3] = 0xff;
	NandReadLargePagePio(page, pNandInfo->page_size, bad_byte, 4);

	if ((bad_byte[0] != 0xff) || (bad_byte[1] != 0xff))
		return -1;

	return ret;
}

int nand_read(unsigned int load_addr, unsigned int burn_addr, unsigned int size)
{
	nand_info *pNandInfo = GetNandInfo();
	unsigned int page;
	unsigned char *buf = (unsigned char *)load_addr;
	unsigned int left = size;
	int ret;

	if ((burn_addr & (pNandInfo->page_size - 1)) || (size & (pNandInfo->page_size - 1)))
	{
		puts("not algin\n");
		return -1;
	}

	page = burn_addr / pNandInfo->page_size;

	while(left)
	{
		if (!(page >> pNandInfo->block_shift))
		{
			if (nand_block_is_bad(page))
			{
				page += 1<<pNandInfo->block_shift;
			}
		}

		ret = NandReadLargePagePio(page, 0, buf, pNandInfo->page_size);
		if(ret != 0)
		{
			//puts("2");
			break;
		}
		//else
			//puts("1");
		buf += pNandInfo->page_size;
		page++;
		left -= pNandInfo->page_size;
	}

	return (size - left);
}















