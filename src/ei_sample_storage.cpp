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

/* Include ----------------------------------------------------------------- */
#include "ei_sample_storage.h"
#include "ingestion-sdk-platform/syntiant/ei_device_syntiant_samd.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "SdFat.h"

#include <stdlib.h>
#include <stdio.h>

/* Name of temporary binary file */
#define TMP_FILE_NAME "ei_samples.bin"

/* Extern variables -------------------------------------------------------- */
extern SdFat SD;

/* Private variables ------------------------------------------------------- */
static FatFile binFile;
static uint8_t *sdPage = NULL;

/**
 * @brief Create and empty a binary file
 */
void ei_create_bin(void)
{
    // max number of blocks to erase per erase call
    const uint32_t ERASE_SIZE = 262144L;
    uint32_t bgnBlock, endBlock;

    // Delete old tmp file.
    if (SD.exists(TMP_FILE_NAME)) {
        if (!SD.remove(TMP_FILE_NAME)) {
            ei_printf("Deleting old file failed\r\n");
        }
    }

    // Create new file.
    binFile.close();
    if (!binFile.createContiguous(TMP_FILE_NAME, SD_FILE_SIZE)) {
        ei_printf("ERR: SD creating file failed\r\n");
        return;
    }

    // Get the address of the file on the SD.
    if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
        ei_printf("ERR: SD address range failed\r\n");
        return;
    }

    // Flash erase all data in the file.
    uint32_t bgnErase = bgnBlock;
    uint32_t endErase;

    while (bgnErase < endBlock) {
        endErase = bgnErase + ERASE_SIZE;
        if (endErase > endBlock) {
            endErase = endBlock;
        }

        if (!SD.card()->erase(bgnErase, endErase)) {
            ei_printf("ERR: SD erase file failed\r\n");
        }
        bgnErase = endErase + 1;
    }

    // Start a multiple block write.
    if (!SD.card()->writeStart(bgnBlock, 2)) {
        ei_printf("ERR: SD start writing failed\r\n");
    }

    if(sdPage == NULL) {
        sdPage = (uint8_t *)ei_malloc(SD_BLOCK_SIZE);
    }
}

/**
 * @brief Write samples to binary file on SD
 *
 * @param data
 * @param address
 * @param length
 * @return int 0 ok, else error
 */
int ei_write_data_to_bin(uint8_t *data, uint32_t address, uint32_t length)
{
    int i;

    if(sdPage == NULL) {
        return 1;
    }

    int pCnt = address & 0x1FF; // Modulo 512

    for(i = 0; i < length; i++) {
        sdPage[pCnt++] = *(data + i);

        if(pCnt >= 512) {

            if (!SD.card()->writeData((const uint8_t *)sdPage)) {
                return 1;
            }
            pCnt = 0;
        }
    }
    return 0;
}

/**
 * @brief Write out last data to SD file
 * @param address
 * @return int 0 ok, else error
 */
int ei_write_last_data_to_bin(uint32_t address)
{
    int pCnt = address & 0x1FF;

    if(sdPage == NULL) {
        return 1;
    }

    if(pCnt != 0) {
        for(; pCnt < 512; pCnt++) {
            sdPage[pCnt] = 0;
        }
    }
    if (!SD.card()->writeData((const uint8_t *)sdPage)) {
        ei_printf("ERR: writing data to SD failed");
        return 1;
    }

    if (!SD.card()->writeStop()) {
        return 2;
    }

    binFile.close();

    ei_free(sdPage);
    sdPage = NULL;

    return 0;
}

/**
 * @brief Read sample data from SD Card
 * Open file if it is closed
 * @param sample_buffer
 * @param address_offset
 * @param n_read_bytes
 * @return uint32_t Number of bytes read
 */
uint32_t ei_read_sample_buffer(uint8_t *sample_buffer, uint32_t address_offset, uint32_t n_read_bytes)
{
    if(binFile.isOpen() == 0) {
        if (!binFile.open(TMP_FILE_NAME, O_RDONLY)) {
            ei_printf("ERR: Opening sample file failed\r\n");
            return 0;
        }
    }

    if(!binFile.seekSet(address_offset)) {
        return 0;
    }

    return binFile.read(sample_buffer, n_read_bytes);
}
