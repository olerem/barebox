#ifndef	__NANDSEARCH_H__
#define	__NANDSEARCH_H__
#ifdef __cplusplus
extern "C" {
#endif

/*
 * NAND Flash Manufacturer ID Codes
 */
#define NAND_MFR_SAMSUNG		(0xEC)
#define NAND_MFR_TOSHIBA		(0x98)
#define NAND_MFR_SANDISK		(0x45)
#define NAND_MFR_STMICRO		(0x20)
#define NAND_MFR_HYNIX			(0xAD)
#define NAND_MFR_MICRON			(0x2C)
#define NAND_MFR_RENESAS		(0x07)
#define NAND_MFR_NATIONAL		(0x8F)
#define NAND_MFR_FUJITSU		(0x04)
#define NAND_MFR_INTEL	    	(0x89)
#define NAND_MFR_AMD			(0x01)

#define	NAND_SELECT_CHIP_0		0
#define	NAND_SELECT_CHIP_1		1
#define	NAND_SELECT_CHIP_2		2
#define	NAND_SELECT_CHIP_3		3

#define NAND_OPTIONS_X8		0
#define NAND_OPTIONS_X16	1
/**
 * struct nand_flash_dev - NAND Flash Device ID Structure.
 * @name:	Identify the device type.
 * @id:		device ID code.
 * @options:	Bitfield to store chip relevant options.
 * @pagesize:	Pagesize in bytes. 
 * @oobsize:	Spare area size in bytes.
 * @chipsize:	Total chipsize in Mega Bytes.
 * @erasesize:	Size of an erase block in the flash device.
 */
typedef struct _nand_flash_dev {
	char *name;
	int mfr_id;
	int dev_id;
	size_t page_size;
	size_t chip_size;
	size_t erase_size;
	size_t oob_size;
}nand_flash_dev;

/**
 * struct nand_manufacturers - NAND Flash Manufacturer ID Structure.
 * @name:	Manufacturer name.
 * @id:		manufacturer ID code of device.
*/
typedef struct _nand_manufacturers
{
	int id;
	char *name;
}nand_manufacturers;



typedef struct _nand_info_
{
	unsigned char		manufacture_code;	//制造厂商ID
	unsigned char		device_id;				//芯片ID
	unsigned char		third_id;
	unsigned char		fourth_id;

	unsigned char		chip_select;			//选中的芯片
	unsigned char		page_shift;			//页尺寸从1构造的移动位数
	unsigned char		block_shift;			//页尺寸转块尺寸的移动位数
	unsigned char		addr_cycles;			//发送地址次数

	unsigned char		row_cycles;			//行地址次数
	unsigned char		col_cycles;			//列地址次数
	unsigned char		sectors_per_page;	//每页多少个512字节
	unsigned char		pages_per_block;		//每块的页数

	u32			blocks_per_chip;		//芯片的block数
	size_t		page_size;			//页大小，单位为Byte
	size_t		oob_size;			//每页的spare area size，单位为Byte
	size_t		block_size;			//块大小，单位为Byte
	size_t		chip_size;			//芯片大小，单位为Mega
	char*		device_name;			//芯片名称
}nand_info;

int NandSearch(unsigned char nChip, nand_info *pNandInfo);

#ifdef __cplusplus
}
#endif
#endif
