#include <common.h>
#include <drivers/nand_search.h>
#include <drivers/nand.h>

static unsigned char nand_id[32];	//保证对齐

#define NAND_MFD_ID_TYPES		11
static const nand_manufacturers nand_manuf_ids[NAND_MFD_ID_TYPES] =
{
	{NAND_MFR_SAMSUNG,	"Samsung"	},
	{NAND_MFR_HYNIX,	"Hynix"		},
	{NAND_MFR_SANDISK, 	"San Disk"	},
	{NAND_MFR_INTEL, 	"Intel"		},
	{NAND_MFR_AMD, 		"AMD"		},
	{NAND_MFR_FUJITSU, 	"Fujitsu"	},
	{NAND_MFR_MICRON, 	"Micron"	},
	{NAND_MFR_NATIONAL, 	"National"	},
	{NAND_MFR_RENESAS, 	"Renesas"	},
	{NAND_MFR_STMICRO, 	"STMicro"	},
	{NAND_MFR_TOSHIBA, 	"Toshiba"	}
};

#define NAND_FLASH_TYPES		3
static const nand_flash_dev nand_flash_ids[NAND_FLASH_TYPES] = {
	{"TC58NVG1S3ETA00", NAND_MFR_TOSHIBA, 0xda, 0x800, 0x100,  0x20000, 64},
	{"TC58DVG02D5TA00", NAND_MFR_TOSHIBA, 0xd1, 0x800, 0x80,  0x20000, 64},
	{"K9F1G08U0C", NAND_MFR_SAMSUNG, 0xf1, 0x800, 0x80,  0x20000, 64},
};

unsigned int get_shift(unsigned int val)
{
    int i;

    if ( val == 0 )
    {
        return -1;
    }

    for ( i = 0; ; i++ )
    {
        if ( (val>>i) == 1 ) return i;
    }
}


/*******************************************************************************
*函数名:	NandSearch
*功能:		检测NAND，读ID
*输入参数:	nChip--选择的Chip
*输出参数:	pNandInfo--NAND芯片结构，读写擦中需要
*返回值:	0--表示成功
*			其他值对应各种错误
*NOTE:		
*******************************************************************************/
int NandSearch(unsigned char nChip, nand_info *pNandInfo)
{
	u32 i;

	pNandInfo->chip_select = nChip;
	NandReadIDPio(pNandInfo->chip_select, (unsigned int *)nand_id, 4);
	pNandInfo->manufacture_code= nand_id[0];
	pNandInfo->device_id = nand_id[1];
	pNandInfo->third_id = nand_id[2];
	pNandInfo->fourth_id = nand_id[3];

	//搜索制造商ID
	if (0 == pNandInfo->manufacture_code)
	{//0x00制造商id错误
		return -1;
	}
	for (i = 0; i < NAND_FLASH_TYPES; i++)
	{
		if ((nand_flash_ids[i].mfr_id == pNandInfo->manufacture_code) && (nand_flash_ids[i].dev_id == pNandInfo->device_id))
		{//找到
			break;
		}
	}
	if (NAND_FLASH_TYPES == i)
	{
		puts("\nUnknow Flash!!!\n");
		return -1;
	}

	pNandInfo->page_shift = (unsigned char)get_shift(nand_flash_ids[i].page_size);
	pNandInfo->block_shift = (unsigned char)((unsigned char)get_shift(nand_flash_ids[i].erase_size) - pNandInfo->page_shift);
	pNandInfo->oob_size = nand_flash_ids[i].oob_size;
	pNandInfo->device_name = nand_flash_ids[i].name;
	pNandInfo->page_size = nand_flash_ids[i].page_size;
	pNandInfo->block_size = nand_flash_ids[i].erase_size;
	pNandInfo->chip_size = nand_flash_ids[i].chip_size;
	pNandInfo->col_cycles = 2;
	pNandInfo->row_cycles = 3;
	pNandInfo->addr_cycles = (unsigned char)(pNandInfo->col_cycles + pNandInfo->row_cycles);
	pNandInfo->sectors_per_page = (unsigned char)(pNandInfo->page_size >> 9);
	pNandInfo->pages_per_block = (unsigned char)(pNandInfo->block_size >> pNandInfo->page_shift);
	pNandInfo->blocks_per_chip = (u32)((pNandInfo->chip_size << 20) >> (pNandInfo->block_shift + pNandInfo->page_shift));

#if 0
	printf("pNandInfo->manufacture_code is 0x%x\r\n",pNandInfo->manufacture_code);
	printf("pNandInfo->device_id is 0x%x\r\n",pNandInfo->device_id);
	printf("pNandInfo->third_id is 0x%x\r\n",pNandInfo->third_id);
	printf("pNandInfo->fourth_id is 0x%x\r\n",pNandInfo->fourth_id);
	printf("pNandInfo->chip_select is 0x%x\r\n",pNandInfo->chip_select);
	printf("pNandInfo->page_shift is 0x%x\r\n",pNandInfo->page_shift);
	printf("pNandInfo->block_shift is 0x%x\r\n",pNandInfo->block_shift);
	printf("pNandInfo->addr_cycles is 0x%x\r\n",pNandInfo->addr_cycles);
	printf("pNandInfo->row_cycles is 0x%x\r\n",pNandInfo->row_cycles);
	printf("pNandInfo->col_cycles is 0x%x\r\n",pNandInfo->col_cycles);
	printf("pNandInfo->oob_size is 0x%x\r\n",pNandInfo->oob_size);
	printf("pNandInfo->blocks_per_chip is 0x%lx\r\n",pNandInfo->blocks_per_chip);
	printf("pNandInfo->sectors_per_page is 0x%x\r\n",pNandInfo->sectors_per_page);
	printf("pNandInfo->pages_per_block is 0x%x\r\n",pNandInfo->pages_per_block);
	printf("pNandInfo->page_size is 0x%x\r\n",pNandInfo->page_size);
	printf("pNandInfo->block_size is 0x%x\r\n",pNandInfo->block_size);
	printf("pNandInfo->chip_size is 0x%x\r\n",pNandInfo->chip_size);
	printf("pNandInfo->device_name is %s\r\n",pNandInfo->device_name);
#endif	
	return 0;
}

