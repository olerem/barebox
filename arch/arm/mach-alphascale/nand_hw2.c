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

/*
* 等待NAND控制器ready
*/
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

/*
* 等待NAND设备ready
*/
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

/*
* 读nLen个数据到pBuffer地址中
*/
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

#if 0
/*
* 设置NAND的PIN MUX
*/
static void NandSetPinMux(void)
{
    	SetPinMux(11,0,5);
	SetPinMux(11,1,5);    
	SetPinMux(11,2,5);    
	SetPinMux(11,3,5);    
	SetPinMux(11,4,5);    
	SetPinMux(11,5,5);    
	SetPinMux(11,6,5);    
	SetPinMux(12,0,5);    
	SetPinMux(12,1,5);    
	SetPinMux(12,2,5);    
	SetPinMux(12,3,5);    
	SetPinMux(12,4,5);    
	SetPinMux(12,5,5);    
	SetPinMux(12,6,5);    
	SetPinMux(12,7,5);
}
#endif
/*
* NAND芯片复位
*/
int NandFlashReset(u8 nChip)
{
    outl((0xff00 | nChip), NAND_MEM_CTRL);
    outl( (RESET << 8)
        | (ADDR_SEL_0 << 7)
        | (INPUT_SEL_BIU << 6)
        | (SEQ0), NAND_COMMAND);

	return NandWaitForDeviceReady(nChip);
}

/*
* 设置NAND控制器时序
*/
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

/*
* 为选定的NAND芯片设置片选的PIN MUX
*/
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

/*******************************************************************************
*函数名:	NandInit
*功能:		初始化NAND控制器，包括GPIO，选择NAND芯片，设置时序。并且复位NAND FLASH
*输入参数:	nChip--选择的Chip
*输出参数:	无
*返回值:	0--表示成功
*			其他值对应各种错误
*NOTE:		
*******************************************************************************/
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

/*******************************************************************************
*函数名:	NandLargePageMakeAddr
*功能:		设置将要发送到控制器的地址，控制器将地址处理后发送给NAND。
*输入参数:	nPage--NAND页数
*			oob_flag--设置的如果是OOB的地址，该参数需为1，否则为0
*输出参数:	pAddr--设置后地址的保存处
*返回值:	无
*NOTE:		1、调用方需保证存放地址的内存，
*			该驱动申请了全局数组NandAddr[32]专用于设置地址的存放
*			2、擦除时，控制器会自动取行地址发给NAND，因此不需要进行专门的处理
*******************************************************************************/
static void NandLargePageMakeAddr(u32 nPage, u32 nColumn, unsigned char *pAddr)
{
	int i = 0;
	u32 row_addr = nPage;
	nand_info *pNandInfo = GetNandInfo();

	//清空
	memset(pAddr, 0, pNandInfo->addr_cycles);

	//设置列地址
	for (i=0; i<pNandInfo->col_cycles; i++)
	{
		pAddr[i] = (unsigned char)(nColumn & 0xFF);
		nColumn = nColumn >> 8;
	}
	
	//设置行地址,其实就是nPage页号
	for (i = pNandInfo->col_cycles; i < pNandInfo->addr_cycles; i++)
	{
		pAddr[i] = (unsigned char)(row_addr & 0xFF);		//字节掩码
		row_addr = row_addr >> 8;				//字节位数
	}
}

/*******************************************************************************
*函数名:	NandReadIDPio
*功能:		读NAND FLASH的ID
*输入参数:	nChip--选择的Chip
*			pDevId--读出的ID存放的地址
*			nByte--读出ID的字节数
*输出参数:	无
*返回值:	0--表示成功
*			其他值对应各种错误
*NOTE:		
*******************************************************************************/
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

/*
* 配置NAND控制器
*/
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

/*
* NAND的硬件ECC校验函数
*/
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


/*******************************************************************************
*函数名:	NandReadLargePagePio
*功能:		大页NAND读函数
*输入参数:	nPage--NAND的页数
*			nColumn--该页的列
*			pBuffer--读出的内存首地址
*			nLen--读出的字节数
*输出参数:	无
*返回值:	0--表示成功
*			其他值对应各种错误
*详细说明:	1、支持写oob(无ECC校验)
*			2、支持整页读(有ECC校验)
*			3、支持自定义长度读
*			   可随意读，但列地址加上读取长度必须小于页大小加可利用的oob
*			4、只支持2K及以上页大小NAND的读
*******************************************************************************/
static int NandReadLargePagePio(u32 nPage, u32 nColumn, unsigned char *pBuffer, u32 nLen)
{
	int ret = 0;
	unsigned char *tmpbuf = pBuffer;
	u32 *addr = (u32 *)NandAddr;
	nand_info *pNandInfo = GetNandInfo();

	/*1、复位FIFO，配置NAND控制器*/
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

	/*2、选择chip，配置NAND地址*/
	outl((0xff00 | pNandInfo->chip_select), NAND_MEM_CTRL);
	NandLargePageMakeAddr(nPage, nColumn, NandAddr);
	outl(addr[0], NAND_ADDR0_L);
	outl(addr[1], NAND_ADDR0_H);

	/*3、发起命令*/
	outl((READ_PAGE_2<<16)
	   | (READ_PAGE_1<<8)
	   | (ADDR_SEL_0<<7)
	   | (INPUT_SEL_BIU<<6)
	   | (SEQ10), NAND_COMMAND);

	/*4、等待控制器ready*/
	ret = NandWaitForControllerReady();
	if (ret != 0)
	{
		return ret;
	}

	/*5、读数据*/
	NandReadBuffer(tmpbuf, nLen);

	/*6、如果ECC使能，判断ECC是否正确*/
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















