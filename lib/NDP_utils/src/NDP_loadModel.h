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

#ifndef NDP_LOAD_MODEL_H
#define NDP_LOAD_MODEL_H

// Set USE_SD_H nonzero to use SD.h.
// Set USE_SD_H zero to use SdFat.h.
#define USE_SD_H 0

#if USE_SD_H
#include <SD.h>
#else
#include "SdFat.h"
extern SdFat SD;
#endif

#include <SerialFlash.h>

#include "NDP.h"
#include "NDP_GPIO.h"
#include "NDP_SPI.h"    // For loadUilibFlash
#include "NDP_Serial.h" // Allows us to print debug messages to a second serial port

#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI)

// For loading from flash (USB dongle)
const unsigned long FLASH_SEGMENT_SIZE = 0x100000;
const unsigned long FLASH_INDEX_LOCATION = FLASH_SEGMENT_SIZE - 0x8;
const unsigned long MAGIC_NUMBER = 0xA5B6C7D8;
const unsigned long CHIP_UART_CTRL = 0x40004008;

// Flash Commands for SST25VF016B (Bluebank)
const unsigned long FLASH_EWSR = 0x50;        // Enable-Write-Status-Register
const unsigned long FLASH_WRSR = 0x01;        // Write-Status-Register
const unsigned long FLASH_WREN = 0x06;        // Write-Enable
const unsigned long FLASH_RDSR = 0x05;        // Read-Status-Register

enum Load_BIN_error_messages
{
    BIN_LOAD_OK = 0,
    NO_SD = 1,
    SD_NOT_INITIALIZED = 2,
    BIN_NOT_OPENED = 3,
    ERROR_LOADING_BIN = 4,
    ERROR_LOADING_FLASH = 5,
    ERROR_LOADING_SD = 6,
    LOADED_FROM_SERIAL_FLASH = 8
};

extern uint32_t cardSectorCount;
// For loadUilibFlash
extern byte runningFromFlash;
extern byte patchApplied;

int loadModel(String model);
void copySdToFlash();
bool compareFiles(File &file, SerialFlashFile &ffile);
void fatFormatSd();
void formatCard();
void eraseCard();
void eraseSd();
byte flashCat(String foundFile);
void loadUilibFlash(byte record);

#endif
