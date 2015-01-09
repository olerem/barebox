#include <common.h>
#include <drivers/nand_search.h>
#include <drivers/nand.h>
#include <string.h>

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
	
	while (inl(NAND_STATUS) & (1UL << 8))
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
	
	while (!(inl(NAND_STATUS) & (1UL << nChip)))
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
		tmpbuf[i] = inl(NAND_FIFO_DATA);
	}

	if ((nLen - i * 4) > 0)
	{
		tmpbuf[i] = inl(NAND_FIFO_DATA);
	}
}

int NandFlashReset(u8 nChip)
{
    outl((0xff00 | nChip), NAND_MEM_CTRL);
    outl( (RESET << 8)
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
	outl((ADDR_CYCLE_1 << 18) 		| (ADDR1_AUTO_INCR_DIS << 17)
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
    outl((trwh << 4) |     // after power on ,device is in async mode0, twh >= 50
         (trwp), NAND_TIMINGS_ASYN);        // twp >= 50 and twhr >= 120

    ret = NandFlashReset(0);
	if (ret != 0)
	{
		return ret;
	}
#endif	
    trwh = 1; //TWH;
    trwp = 1; //TWP;
    outl((trwh << 4) |     // timing mode change to mode 5, twh >= 7
         (trwp), NAND_TIMINGS_ASYN);      // twp >= 10

    twhr = 2;
    trhw = 4;
    outl((twhr << 24) |
         (trhw << 16) |
         (tadl << 8)  |
         (tccs), NAND_TIME_SEQ_0);
    outl((tsync << 16) |
         (trr << 9) |
         (twb), NAND_TIME_SEQ_1);

	return ret;
}

static void NandSelectChip(unsigned char nChip)
{
    outl((0xff00 |  nChip), NAND_MEM_CTRL);
    outl((1UL << (nChip+8)) ^ inl(NAND_MEM_CTRL), NAND_MEM_CTRL);// clear WP reg
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
	outl(0x00000400, (0x80040030+4));  // open nand pclk    
	outl(0x00000008, 0x800401D4);      // set nand clk to 1/2 pclk

	
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

	outl(1, NAND_FIFO_INIT);		/*复位FIFO*/
	outl(nByte, NAND_DATA_SIZE);
	outl((0xff00 | nChip), NAND_MEM_CTRL);
	outl(0x00000000, NAND_ADDR0_L);
	outl((READ_ID << 8)
	   | (ADDR_SEL_0 << 7)
	   | (INPUT_SEL_BIU << 6)
	   | (SEQ1), NAND_COMMAND);
	ret = NandWaitForDeviceReady(nChip);

	pDevId[0] = inl(NAND_FIFO_DATA);

	return ret;
}

static void NandControllerConfig (void)
{
	nand_info *pNandInfo = GetNandInfo();

	outl((EN_STATUS << 23)				| (NO_RNB_SEL << 22)
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

	ecc_status = inl(NAND_ECC_CTRL);
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

	outl(0x1, NAND_FIFO_INIT);
	NandControllerConfig( );

	if (nColumn == 0)
	{
		outl(inl(NAND_CONTROL) & (~(SPARE_EN << 3)), NAND_CONTROL);
		outl((ecc_threshold << 8) | (ecc_cap << 5), NAND_ECC_CTRL);
		outl(pNandInfo->page_size + nand_spare_size, NAND_ECC_OFFSET);
	}
	else if (nColumn == pNandInfo->page_size)
	{
		outl((inl(NAND_CONTROL) & (~(ECC_EN << 5))) | (DATA_SIZE_CUSTOM<< 11), NAND_CONTROL);
		outl(nLen, NAND_SPARE_SIZE);
		outl(nLen, NAND_DATA_SIZE);
	}

	outl((0xff00 | pNandInfo->chip_select), NAND_MEM_CTRL);
	NandLargePageMakeAddr(nPage, nColumn, NandAddr);
	outl(addr[0], NAND_ADDR0_L);
	outl(addr[1], NAND_ADDR0_H);

	outl((READ_PAGE_2<<16)
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

	if ((inl(NAND_CONTROL) & (ECC_EN<<5)) != 0)
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















