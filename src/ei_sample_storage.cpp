/* The Clear BSD License
 *
 * Copyright (c) 2025 EdgeImpulse Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
