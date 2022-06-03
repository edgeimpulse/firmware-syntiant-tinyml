/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef EI_SYNTIANT_FS_COMMANDS_H
#define EI_SYNTIANT_FS_COMMANDS_H

/* Include ----------------------------------------------------------------- */
#include <stdint.h>

#define SYNTIANT_FS_BLOCK_ERASE_TIME_MS		90

/** Eta fs return values */
typedef enum
{
	SYNTIANT_FS_CMD_OK = 0,					/**!< All is well				 */
	SYNTIANT_FS_CMD_NOT_INIT,				/**!< FS is not initialised		 */
	SYNTIANT_FS_CMD_READ_ERROR,				/**!< Error occured during read  */
	SYNTIANT_FS_CMD_WRITE_ERROR,			/**!< Error occured during write */
	SYNTIANT_FS_CMD_ERASE_ERROR,			/**!< Erase error occured		 */
	SYNTIANT_FS_CMD_NULL_POINTER,			/**!< Null pointer parsed		 */

}ei_syntiant_ret_t;


/** Number of retries for SPI Flash */
#define MX25R_RETRY		10000

/** SPI Flash Memory layout */
#define MX25R_PAGE_SIZE			256			/**!< Page program size			 */
#define MX25R_SECTOR_SIZE		4096		/**!< Size of sector			 */
#define MX25R_BLOCK32_SIZE		(MX25R_SECTOR_SIZE * 8) /**!< 32K Block 	 */
#define MX25R_BLOCK64_SIZE		(MX25R_BLOCK32_SIZE * 2)/**!< 64K Block	 	 */
#define MX25R_CHIP_SIZE			(MX25R_BLOCK64_SIZE * 128)/**!< 64Mb on chip */

/** MX25R Register defines */
#define MX25R_PP				0x02		/**!< Program page				 */
#define MX25R_READ				0x03		/**!< Read data command			 */
#define MX25R_RDSR				0x05		/**!< Status Register 			 */
#define MX25R_WREN				0x06		/**!< Write enable bit 			 */
#define MX25R_SE				0x20		/**!< Sector erase				 */
#define MX25R_PGM				0x7A		/**!< Resume programming		 */
#define MX25R_BE				0xD8		/**!< Block (64K) erase			 */

/** MX25R Status register bit defines */

#define MX25R_STAT_SRWD			(1<<7)		/**!< Status reg write protect	 */
#define MX25R_STAT_QE			(1<<6)		/**!< Quad enable 				 */
#define MX25R_STAT_BP3			(1<<5)		/**!< Level of protect block	 */
#define MX25R_STAT_BP2			(1<<4)		/**!< Level of protect block	 */
#define MX25R_STAT_BP1			(1<<3)		/**!< Level of protect block	 */
#define MX25R_STAT_BP0			(1<<2)		/**!< Level of protect block	 */
#define MX25R_STAT_WEL			(1<<1)		/**!< Write enable latch		 */
#define MX25R_STAT_WIP			(1<<0)		/**!< Write in progress bit		 */


/* Prototypes -------------------------------------------------------------- */
int ei_syntiant_fs_load_config(uint32_t *config, uint32_t config_size);
int ei_syntiant_fs_save_config(const uint32_t *config, uint32_t config_size);

int ei_syntiant_fs_erase_sampledata(uint32_t start_block, uint32_t end_address);
int ei_syntiant_fs_write_samples(const void *sample_buffer, uint32_t address_offset, uint32_t n_samples);
int ei_syntiant_fs_end_write(uint32_t address_offset);
int ei_syntiant_fs_read_sample_data(void *sample_buffer, uint32_t address_offset, uint32_t n_read_bytes);
uint32_t ei_syntiant_fs_get_block_size(void);
uint32_t ei_syntiant_fs_get_n_available_sample_blocks(void);

#endif