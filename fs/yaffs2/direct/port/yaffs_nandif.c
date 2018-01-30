#include "yaffs_guts.h"
#include "yaffs_trace.h"
#include "nand.h"

#define EM_SIZE_IN_BYTES (0xfda0000)
#define EM_SIZE_IN_MEG (EM_SIZE_IN_BYTES>>20)
#define PAGE_TOTAL_SIZE (PAGE_DATA_SIZE+PAGE_SPARE_SIZE)
#define BLOCK_TOTAL_SIZE (PAGES_PER_BLOCK * PAGE_TOTAL_SIZE)
#define BLOCKS_PER_MEG ((1<<20)/(PAGES_PER_BLOCK * PAGE_DATA_SIZE))
#define EM_SIZE_IN_BLOCKS (EM_SIZE_IN_BYTES>>BLOCK_SHIFT)

unsigned int yaffs_trace_mask = 
	YAFFS_TRACE_ERASE |
	YAFFS_TRACE_ERROR |
	YAFFS_TRACE_BAD_BLOCKS |
	0;

static int yaffs_nand_drv_WriteChunk(struct yaffs_dev *dev, int nand_chunk,
				   const u8 *data, int data_len, const u8 *oob, int oob_len)
{
	if (!data || !oob) {
		return YAFFS_FAIL;
	}
	// yaffs_trace(YAFFS_TRACE_WRITE,
		// "yaffs_nand_drv_WriteChunk,nand_chunk:%d,data:%X,data_len:%d,oob:%X,oob_len:%d",
		// nand_chunk, data, data_len, oob, oob_len);
	if (nand_write_with_oob(nand_chunk, data, data_len, oob, oob_len) != 1) {
		return YAFFS_FAIL;
	}
	return YAFFS_OK;
}

static int yaffs_nand_drv_ReadChunk(struct yaffs_dev *dev, int nand_chunk,
				   u8 *data, int data_len, u8 *oob, int oob_len,
				   enum yaffs_ecc_result *ecc_result_out)
{
	int ret;
	if (data == NULL) {
		data_len = 0;
	}
	ret = nand_read_with_oob(nand_chunk, data, data_len, oob, oob_len);
	if (ret != 1) {
		if (ecc_result_out) {
			*ecc_result_out = YAFFS_ECC_RESULT_UNKNOWN;
		}
		return YAFFS_FAIL;
	} else {
		if (ecc_result_out) {
			*ecc_result_out = YAFFS_ECC_RESULT_NO_ERROR;
		}
	}
	return YAFFS_OK;
}

static int yaffs_nand_drv_EraseBlock(struct yaffs_dev *dev, int block_no)
{
	if (nand_erase_block(block_no) != 1) {
		return YAFFS_FAIL;
	}
	return YAFFS_OK;
}

static int yaffs_nand_drv_MarkBad(struct yaffs_dev *dev, int block_no)
{
	if (nand_mark_bad_block(block_no) != 1) {
		return YAFFS_FAIL;
	}
	return YAFFS_OK;
}

static int yaffs_nand_drv_CheckBad(struct yaffs_dev *dev, int block_no)
{
	if (nand_is_bad_block(block_no) == 1) {		
		// bad block
		return YAFFS_FAIL;
	}
	return YAFFS_OK;
}

static int yaffs_nand_drv_Initialise(struct yaffs_dev *dev)
{
	//Nand_Init();
	return YAFFS_OK;
}

static int yaffs_nand_drv_Deinitialise(struct yaffs_dev *dev)
{
	return YAFFS_OK;
}

// 格式化nand flash
void nand_format(void)
{
	int i;
	for (i = 32; i <= (2048-1); i++) {
		if (nand_erase_block(i)) {
			nand_mark_bad_block(i); // 确实为坏块	
			printf("Nand_Erase: bad block %d\n", i);
		}
	}
}

struct yaffs_dev nand_dev;

struct yaffs_dev *yaffs_nand_install_drv(const char *dev_name)
{
	struct yaffs_driver *drv;	
	struct yaffs_dev *dev;
	struct yaffs_param *param;
	
	dev = &nand_dev;
	if (!dev) {
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));	
	param = &dev->param;
	param->name = dev_name;	
	if(!param->name) {
		return NULL;		
	}

	param->total_bytes_per_chunk = PAGE_DATA_SIZE;
	param->chunks_per_block = PAGES_PER_BLOCK;
	param->n_reserved_blocks = 5;
	param->start_block = (0x260000)/2048/64; // First block, reserve 4M for bootloader
	param->end_block = (0x260000+0xfda0000)/2048/64-1; // Last block
	param->is_yaffs2 = 1;
	param->use_nand_ecc = 1; // use driver's ecc
	param->n_caches = 10;
	
	drv = &dev->drv;
	drv->drv_write_chunk_fn = yaffs_nand_drv_WriteChunk;
	drv->drv_read_chunk_fn = yaffs_nand_drv_ReadChunk;
	drv->drv_erase_fn = yaffs_nand_drv_EraseBlock;
	drv->drv_mark_bad_fn = yaffs_nand_drv_MarkBad;
	drv->drv_check_bad_fn = yaffs_nand_drv_CheckBad;
	drv->drv_initialise_fn = yaffs_nand_drv_Initialise;
	drv->drv_deinitialise_fn = yaffs_nand_drv_Deinitialise;	
	
	/* The yaffs device has been configured, install it into yaffs */
	yaffs_add_device(dev);

	return dev;	
}

int yaffs_start_up(void)
{
	static u8 start_up_called = 0;

	if(start_up_called) {
		return 0;
	}
	start_up_called = 1;
	// Stuff to initialise anything special (eg lock semaphore).
	yaffsfs_OSInitialisation();
	yaffs_nand_install_drv("/");
	return 0;
}
