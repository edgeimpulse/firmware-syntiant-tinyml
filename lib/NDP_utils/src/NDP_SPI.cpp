/*
 * Copyright (c) 2021 Syntiant Corp.  All rights reserved.
 * Contact at http://www.syntiant.com
 * 
 * This software is available to you under a choice of one of two licenses.
 * You may choose to be licensed under the terms of the GNU General Public
 * License (GPL) Version 2, available from the file LICENSE in the main
 * directory of this source tree, or the OpenIB.org BSD license below.  Any
 * code involving Linux software will require selection of the GNU General
 * Public License (GPL) Version 2.
 * 
 * OPENIB.ORG BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "NDP_SPI.h"

uint8_t ilibBuf[1032] __attribute__((aligned(4)));

void indirectWrite(unsigned long indirectRegister, unsigned long indirectData)
{
    NDP.spiTransfer(NULL, 1, indirectRegister,
                                (unsigned char *)&indirectData, NULL, 4);
}

unsigned long indirectRead(unsigned long indirectRegister)
{
    unsigned int indirectData;
    NDP.spiTransfer(NULL, 1, indirectRegister, NULL,
                                (unsigned char *)&indirectData, 4);
    return indirectData;
}

// Enables NDP Master SPI interface. Caution - this disables NDP LED outputs
void enableMasterSpi()
{
    indirectWrite(CHIP_CONFIG_SPICTL,
                  (((indirectRead(CHIP_CONFIG_SPICTL) | 0x100) & 0xfffffffc)));
}

// Disables NDP Master SPI interface. Needed to use NDP LED outputs
void disableMasterSpi()
{
    indirectWrite(CHIP_CONFIG_SPICTL,
                  (indirectRead(CHIP_CONFIG_SPICTL) & 0xfffffeff));
}

// Waits for SPI master command to be written
void spiWait()
{
    while ((indirectRead(CHIP_CONFIG_SPICTL) & FLASH_SPI_DONE) == 0)
        ;
}

// reverse byte ordering of a 32 bit word
unsigned long reverseBytes(unsigned long value)
{
    return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
           (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

// Change NDP Master SPI mode.
// This controls Master SPI Chip Select, & output buffer write
// MSPI_IDLE = CS low
// MSPI_ENABLE = CS High
// MSPI_TRANSFER = clock buffered data on to the SPI bus
// MSPI_UPDATE = accepts data into output buffer
void changeMasterSpiMode(byte mode)
{
    indirectWrite(CHIP_CONFIG_SPICTL,
                  (((indirectRead(CHIP_CONFIG_SPICTL) & 0xfffffffc) | mode)));
}

// Tells NDP how many bytes to write on Master SPI interface
void writeNumBytes(unsigned long numBytes)
{
    //write spictl=mode=1,ss=0,numbytes=numBytes,enable=1
    indirectWrite(CHIP_CONFIG_SPICTL,
                  (((indirectRead(CHIP_CONFIG_SPICTL) & 0xfffffff3) | (numBytes << 2))));
    changeMasterSpiMode(MSPI_TRANSFER);
}


// Read status register of flash device
uint32_t getFlashStatus()
{
    changeMasterSpiMode(MSPI_IDLE);
    changeMasterSpiMode(MSPI_ENABLE);
    indirectWrite(CHIP_CONFIG_SPITX, FLASH_READ_STATUS_REGISTER);
    // write spictl=mode=1,ss=0,numbytes=3,enable=1
    indirectWrite(CHIP_CONFIG_SPICTL,
                  ((indirectRead(CHIP_CONFIG_SPICTL) | 0xc)));
    changeMasterSpiMode(MSPI_TRANSFER);
    spiWait();
    changeMasterSpiMode(MSPI_UPDATE);
    changeMasterSpiMode(MSPI_TRANSFER);
    spiWait();
    return indirectRead(CHIP_CONFIG_SPIRX);
}

// Writes one of the flash commands
void writeFlashCommand(uint32_t command)
{
    changeMasterSpiMode(MSPI_IDLE);
    changeMasterSpiMode(MSPI_ENABLE);
    indirectWrite(CHIP_CONFIG_SPITX, command);
    indirectWrite(CHIP_CONFIG_SPICTL,
                  (((indirectRead(CHIP_CONFIG_SPICTL) & 0xfffffff0) | 0x1)));
    changeMasterSpiMode(MSPI_TRANSFER);
    spiWait();
    changeMasterSpiMode(MSPI_IDLE);
}

// Erase flash sector (4K bytes) from address "address"
void sectorEraseCommand(uint32_t address)
{
    changeMasterSpiMode(MSPI_IDLE);
    changeMasterSpiMode(MSPI_ENABLE);
    indirectWrite(CHIP_CONFIG_SPITX, FLASH_SECTOR_ERASE + address);
    indirectWrite(CHIP_CONFIG_SPICTL,
                  ((indirectRead(CHIP_CONFIG_SPICTL) & 0xfffffff0) | 0xd));
    changeMasterSpiMode(MSPI_TRANSFER);
    spiWait();
    changeMasterSpiMode(MSPI_IDLE);
}

// writes "count" bytes from the ilib_buff buffer to the
// flash device at address "address"
void flashWrite(unsigned long address, uint32_t count)
{
    uint16_t i;
    uint32_t writeWord;

    enableMasterSpi();

    // if 4K boundary, erase block
    if ((address & 0xfff) == 0)
    {
        while ((getFlashStatus() & FLASH_STATUS_WIP) == FLASH_STATUS_WIP)
            ;
        writeFlashCommand(FLASH_WRITE_ENABLE);
        sectorEraseCommand(address);
    }

    while ((getFlashStatus() & FLASH_STATUS_WIP) == FLASH_STATUS_WIP)
        ;
    writeFlashCommand(FLASH_WRITE_ENABLE);
    changeMasterSpiMode(MSPI_IDLE);
    changeMasterSpiMode(MSPI_ENABLE);
    indirectWrite(CHIP_CONFIG_SPITX, FLASH_PAGE_PROGRAM + address);
    writeNumBytes(0x3);
    changeMasterSpiMode(MSPI_TRANSFER);
    spiWait();

    for (i = 0; i < count; i += 4)
    {
        writeWord = (ilibBuf[i + 3] | (ilibBuf[i + 2] << 8) | (ilibBuf[i + 1] << 16) | (ilibBuf[i + 0] << 24));
        indirectWrite(CHIP_CONFIG_SPITX, writeWord);
        changeMasterSpiMode(MSPI_ENABLE);
        changeMasterSpiMode(MSPI_TRANSFER);
    }

    while ((getFlashStatus() & FLASH_STATUS_WIP) == FLASH_STATUS_WIP)
        ;
}
