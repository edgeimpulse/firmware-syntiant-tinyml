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

#ifndef NDP_SPI_H
#define NDP_SPI_H

#include "NDP.h"

// ilib buffer
// This is the buffer that feeds the uilib from the log file
// Size must be divisible by both 3 & 4
extern uint8_t ilibBuf[1032] __attribute__((aligned(4)));

// Flash Register bits
const unsigned long FLASH_SPI_DONE = 0x00000200;   // SPI Done bit
const unsigned long FLASH_STATUS_WIP = 0x00000001; // Flash Write In Progress

const unsigned long CHIP_CONFIG_SPICTL = 0x40009014;
const unsigned long CHIP_CONFIG_SPITX = 0x40009018;
const unsigned long CHIP_CONFIG_SPIRX = 0x4000901c;

enum mspi_modes_e
{
    MSPI_IDLE = 0x0,
    MSPI_ENABLE = 0x1,
    MSPI_TRANSFER = 0x2,
    MSPI_UPDATE = 0x3
};

//Flash Commands for MX25R6435FSN (Tesolve)
const unsigned long FLASH_READ = 0x03;
const unsigned long FLASH_FAST_READ = 0x0b;
const unsigned long FLASH_PAGE_PROGRAM = 0x02;
const unsigned long FLASH_SECTOR_ERASE = 0x20;
const unsigned long FLASH_BLOCK_ERASE = 0xD8;
const unsigned long FLASH_CHIP_ERASE = 0x60;
const unsigned long FLASH_CHIP_ERASE_2 = 0xc7;
const unsigned long FLASH_WRITE_ENABLE = 0x06;                 // WREN
const unsigned long FLASH_WRITE_DISABLE = 0x04;                // WRDI
const unsigned long FLASH_WRITE_STATUS_REGISTER = 0x01;        // WRSR
const unsigned long FLASH_READ_STATUS_REGISTER = 0x05;         // WDSR
const unsigned long FLASH_READ_CONFIGURATION_REGISTER = 0x15;  // RDCR
const unsigned long FLASH_DEEP_POWER_DOWN = 0xb9;              // DP
const unsigned long FLASH_SET_BURDT_LENGTH = 0xc0;             // SBL
const unsigned long FLASH_READ_IDENTIFICATION_REGISTER = 0x9f; // RDID
const unsigned long FLASH_READ_ELECTRONIC_ID = 0xAB;           // RES
const unsigned long FLASH_NOP = 0x00;
const unsigned long FLASH_RESET_ENABLE = 0x66; // RSTEN
const unsigned long FLASH_RESET = 0x99;        // RST
const unsigned long FLASH_ENABLE_WRITE_STATUS = 0x50;
const unsigned long FLASH_DP = 0xB9;        // Flash Deep Power Down

void indirectWrite(unsigned long indirectRegister, unsigned long indirectData);
unsigned long indirectRead(unsigned long indirectRegister);
void enableMasterSpi();
void disableMasterSpi();
void spiWait();
unsigned long reverseBytes(unsigned long value);
void changeMasterSpiMode(byte mode);
void writeNumBytes(unsigned long numBytes);
uint32_t getFlashStatus();
void writeFlashCommand(uint32_t command);
void sectorEraseCommand(uint32_t address);
void flashWrite(unsigned long address, uint32_t count);

#endif
