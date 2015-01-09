#ifndef	__NAND_H__
#define __NAND_H__
#ifdef __cplusplus
extern "C" {
#endif

#define  TSYNC 0x0
#define  TRR   0x0
#define  TWB   0x0
#define  TITC  0x0
#define  TWHR  0x0
#define  TRHW  0x0
#define  TADL  0x0
#define  TCCS  0x0

#define  TRWH   0x0
#define  TRWP   0x0

#define  TCAD  0x0

// cmd parameters
#define  RESET                        0xFF
#define  SYNC_RESET                   0xFC

#define  READ_ID                      0x90
#define  READ_PAR_PAGE                0xEC
#define  READ_UNIQUE_ID               0xED

#define  GET_FEARURES                 0xEE
#define  SET_FEATURES                 0xEF

#define  READ_STATUS                  0x70
#define  READ_STATUS_ENHANCE          0x78

#define  CHANGE_READ_COLUMN_1         0x05
#define  CHANGE_READ_COLUMN_2         0xE0

#define  SELECT_CACHE_REGISTER_1      0x06
#define  SELECT_CACHE_REGISTER_2      0xE0

#define  CHANGE_WRITE_COLUMN          0x85
#define  CHANGE_ROW_ADDRESS           0x85

#define  READ_PAGE_1                  0x00
#define  READ_PAGE_2                  0x30

#define  READ_PAGE_CACHE              0x31
#define  READ_PAGE_CACHE_LAST         0x3F

#define  READ_MULTIPLANE_1            0x00
#define  READ_MULTIPLANE_2            0x32

#define  READ_TWO_PLANE_1             0x00
#define  READ_TWO_PLANE_2             0x00
#define  READ_TWO_PLANE_3             0x30

#define  PROGRAM_PAGE_1               0x80
#define  PROGRAM_PAGE_2               0x10

#define  PROGRAM_PAGE1                0x80

#define  PROGRAM_PAGE_CACHE_1         0x80
#define  PROGRAM_PAGE_CACHE_2         0x15

#define  PROGRAM_MULTIPLANE_1         0x80
#define  PROGRAM_MULTIPLANE_2         0x11

#define  WRITE_PAGE                   0x10
#define  WRITE_PAGE_CACHE             0x15
#define  WRITE_MULTIPLANE             0x11

#define  ERASE_BLOCK_1                0x60
#define  ERASE_BLOCK_2                0xD0

#define  ERASE_MULTIPLANE_1           0x60
#define  ERASE_MULTIPLANE_2           0xD1

#define  COPYBACK_READ_1              0x00
#define  COPYBACK_READ_2              0x35

#define  COPYBACK_PROGRAM_1           0x85
#define  COPYBACK_PROGRAM_2           0x10

#define  COPYBACK_PROGRAM1            0x85

#define  COPYBACK_MULTIPLANE_1        0x85
#define  COPYBACK_MULTIPLANE_2        0x11

#define  PROGRAM_OTP_1                0xA0
#define  PROGRAM_OTP_2                0x10

#define  DATA_PROTECT_OTP_1           0xA5
#define  DATA_PROTECT_OTP_2           0x10

#define  READ_PAGE_OTP_1              0xAF
#define  READ_PAGE_OTP_2              0x30

// seq parameter
#define  SEQ1     0x21   // 6'b100001
#define  SEQ2     0x22   // 6'b100010
#define  SEQ4     0x24   // 6'b100100
#define  SEQ5     0x25   // 6'b100101
#define  SEQ6     0x26   // 6'b100110
#define  SEQ7     0x27   // 6'b100111
#define  SEQ9     0x29   // 6'b101001
#define  SEQ10    0x2A   // 6'b101010
#define  SEQ11    0x2B   // 6'b101011
#define  SEQ15    0x2F   // 6'b101111
#define  SEQ0     0x00   // 6'b000000
#define  SEQ3     0x03   // 6'b000011
#define  SEQ8     0x08   // 6'b001000
#define  SEQ12    0x0C   // 6'b001100
#define  SEQ13    0x0D   // 6'b001101
#define  SEQ14    0x0E   // 6'b001110
#define  SEQ16    0x30   // 6'b110000
#define  SEQ17    0x15   // 6'b010101
#define  SEQ18    0x32   // 6'h110010

// cmd register
#define  ADDR_SEL_0    0x0
#define  ADDR_SEL_1    0x1

#define  INPUT_SEL_BIU  0x0
#define  INPUT_SEL_DMA  0x1

// control register parameter
#define  DISABLE_STATUS    1
#define  EN_STATUS         0

#define  RNB_SEL           0
#define  NO_RNB_SEL        1

#define  BIG_BLOCK_EN      0
#define  SMALL_BLOCK_EN    1

#define  ADDR_CYCLE_0      0x0
#define  ADDR_CYCLE_1      0x1
#define  ADDR_CYCLE_2      0x2
#define  ADDR_CYCLE_3      0x3
#define  ADDR_CYCLE_4      0x4
#define  ADDR_CYCLE_5      0x5

#define  WORK_MODE_ASYNC   0
#define  WORK_MODE_SYNC    1

#define  PROT_EN           0
#define  PROT_DIS          1

#define  LOOKUP_EN         1
#define  LOOKUP_DIS        0

#define  IO_WIDTH_8        0
#define  IO_WIDTH_16       1

#define  DATA_SIZE_FULL_PAGE  0
#define  DATA_SIZE_CUSTOM     1

#define  PAGE_SIZE_256B        0x0
#define  PAGE_SIZE_512B        0x1
#define  PAGE_SIZE_1024B       0x2
#define  PAGE_SIZE_2048B       0x3
#define  PAGE_SIZE_4096B       0x4
#define  PAGE_SIZE_8192B       0x5
#define  PAGE_SIZE_16384B      0x6
#define  PAGE_SIZE_32768B      0x7
#define  PAGE_SIZE_0B          0x0

#define  BLOCK_SIZE_32P        0x0
#define  BLOCK_SIZE_64P        0x1
#define  BLOCK_SIZE_128P       0x2
#define  BLOCK_SIZE_256P       0x3

#define  ECC_DIS				0
#define  ECC_EN           		1

#define  INT_DIS          		0
#define  INT_EN           		1

#define  SPARE_DIS        		0
#define  SPARE_EN         		1

#define  ADDR0_AUTO_INCR_DIS  	0
#define  ADDR0_AUTO_INCR_EN   	1

#define  ADDR1_AUTO_INCR_DIS  	0
#define  ADDR1_AUTO_INCR_EN   	1

//generic_seq_ctrl
#define  CMD0_EN      0x1
#define  CMD0_DIS     0x0

#define  ADDR0_EN     0x1
#define  ADDR0_DIS    0x0

#define  CMD1_EN      0x1
#define  CMD1_DIS     0x0

#define  ADDR1_EN     0x1
#define  ADDR1_DIS    0x0

#define  CMD2_EN      0x1
#define  CMD2_DIS     0x0

#define  CMD3_EN      0x1
#define  CMD3_DIS     0x0

#define  ADDR2_EN     0x1
#define  ADDR2_DIS    0x0 

#define  DEL_DIS_ALL  0x0
#define  DEL_EN_ALL   0x3
#define  DEL_EN_0     0x1
#define  DEL_EN_1     0x2

#define  DATA_EN      0x1
#define  DATA_DIS     0x0

#define  COL_ADDR_EN  0x1
#define  COL_ADDR_DIS 0x0

// mem ctrl register
#define  MEM_CE_0     0x0
#define  MEM_CE_1     0x1
#define  MEM_CE_2     0x2
#define  MEM_CE_3     0x3
#define  MEM_CE_4     0x4
#define  MEM_CE_5     0x5
#define  MEM_CE_6     0x6
#define  MEM_CE_7     0x7

// int_mask register
#define  FIFO_ERROR_DIS  0
#define  FIFO_ERROR_EN   1

#define  MEM7_RDY_DIS    0
#define  MEM7_RDY_EN     1

#define  MEM6_RDY_DIS    0
#define  MEM6_RDY_EN     1

#define  MEM5_RDY_DIS    0
#define  MEM5_RDY_EN     1

#define  MEM4_RDY_DIS    0
#define  MEM4_RDY_EN     1

#define  MEM3_RDY_DIS    0
#define  MEM3_RDY_EN     1

#define  MEM2_RDY_DIS    0
#define  MEM2_RDY_EN     1

#define  MEM1_RDY_DIS    0
#define  MEM1_RDY_EN     1

#define  MEM0_RDY_DIS    0
#define  MEM0_RDY_EN     1

#define  ECC_TRSH_ERR_DIS  0
#define  ECC_TRSH_ERR_EN   1

#define  ECC_FATAL_ERR_DIS 0
#define  ECC_FATAL_ERR_EN  1

#define  CMD_END_INT_DIS   0
#define  CMD_END_INT_EN    1

#define  PROT_INT_DIS   0
#define  PROT_INT_EN    1

//ecc ctrl register
#define  ECC_WORD_POS_SPARE  1
#define  ECC_WORD_POS_DATA   0

#define  ECC_THRESHOLD_0     0x0
#define  ECC_THRESHOLD_1     0x1
#define  ECC_THRESHOLD_2     0x2
#define  ECC_THRESHOLD_3     0x3
#define  ECC_THRESHOLD_4     0x4
#define  ECC_THRESHOLD_5     0x5
#define  ECC_THRESHOLD_6     0x6
#define  ECC_THRESHOLD_7     0x7
#define  ECC_THRESHOLD_8     0x8
#define  ECC_THRESHOLD_9     0x9
#define  ECC_THRESHOLD_10    0xA
#define  ECC_THRESHOLD_11    0xB
#define  ECC_THRESHOLD_12    0xC
#define  ECC_THRESHOLD_13    0xD
#define  ECC_THRESHOLD_14    0xE
#define  ECC_THRESHOLD_15    0xF

#define  ECC_CAP_2    0x0
#define  ECC_CAP_4    0x1
#define  ECC_CAP_6    0x2
#define  ECC_CAP_8    0x3
#define  ECC_CAP_10   0x4
#define  ECC_CAP_12   0x5
#define  ECC_CAP_14   0x6
#define  ECC_CAP_16   0x7

// dma ctrl register
#define  DMA_START_EN   0x1
#define  DMA_START_DIS  0x0

#define  DMA_DIR_WRITE  0x0
#define  DMA_DIR_READ   0x1

#define  DMA_MODE_SFR   0x0
#define  DMA_MODE_SG    0x1

#define  DMA_BURST_INCR4   0x0
#define  DMA_BURST_STREAM  0x1
#define  DMA_BURST_SINGLE  0x2
#define  DMA_BURST_INCR    0x3
#define  DMA_BURST_INCR8   0x4
#define  DMA_BURST_INCR16  0x5


// boot parameter
#define  BOOT_REQ      0x1



typedef struct _nand_dma_pkg
{
	unsigned long ctrl;	//低两位为link，end位；高16位为传输的长度，必须为4的倍数，0表示65536
	unsigned long addr;	//RAM地址
	struct _nand_dma_pkg * NextPkg;		//指向下一个nand_dma_pkg
}nand_dma_pkg;


int NandReadIDPio(unsigned char nChip, unsigned int *pDevId, unsigned int nByte);
int NandFlashReset(unsigned char nChip);
int NandInit(unsigned char nChip);
int nand_read(unsigned int load_addr, unsigned int burn_addr, unsigned int size);



#ifdef	__cplusplus
}
#endif
#endif

