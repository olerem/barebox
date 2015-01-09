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
	unsigned char		manufacture_code;	//���쳧��ID
	unsigned char		device_id;				//оƬID
	unsigned char		third_id;
	unsigned char		fourth_id;

	unsigned char		chip_select;			//ѡ�е�оƬ
	unsigned char		page_shift;			//ҳ�ߴ��1������ƶ�λ��
	unsigned char		block_shift;			//ҳ�ߴ�ת��ߴ���ƶ�λ��
	unsigned char		addr_cycles;			//���͵�ַ����

	unsigned char		row_cycles;			//�е�ַ����
	unsigned char		col_cycles;			//�е�ַ����
	unsigned char		sectors_per_page;	//ÿҳ���ٸ�512�ֽ�
	unsigned char		pages_per_block;		//ÿ���ҳ��

	u32			blocks_per_chip;		//оƬ��block��
	size_t		page_size;			//ҳ��С����λΪByte
	size_t		oob_size;			//ÿҳ��spare area size����λΪByte
	size_t		block_size;			//���С����λΪByte
	size_t		chip_size;			//оƬ��С����λΪMega
	char*		device_name;			//оƬ����
}nand_info;

int NandSearch(unsigned char nChip, nand_info *pNandInfo);

#ifdef __cplusplus
}
#endif
#endif
