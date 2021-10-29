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

#include "NDP.h"

NDPClass NDP;

const byte SPI_SAMPLE = 0x20;
const byte SPI_MADDR = 0x40;

// SPI related buffers & flags
uint8_t spiData[2048] __attribute__((aligned(4)));

// Slower SPI speed when writing samples 1MHz
uint32_t spiSpeedSampleWrite = 1000000;

// Normal SPI speed was 8MHz. 12MHz is max = 48Mhz (clock) / 4
uint32_t spiSpeedGeneral = 12000000;

NDPClass::NDPClass()
{
    init();
}

void NDPClass::init()
{
    // initialize micro ilib
    memset(&ndp, 0, sizeof(ndp));
    ndp.transfer = spiTransfer;

    // start ILib. This is done by calling the uilib with length = 0
    syntiant_ndp10x_micro_load_log(&ndp, NULL, 0);
}

int NDPClass::spiTransfer(void *d, int mcu, uint32_t address, void *_out,
                                  void *_in, unsigned int count)
{
    uint8_t *out = (uint8_t *)_out;
    uint8_t *in = (uint8_t *)_in;
    int s = SYNTIANT_NDP_ERROR_NONE;
    uint8_t dummy[4] = {0};
    unsigned int i;

    if (in && out) {
        return SYNTIANT_NDP_ERROR_ARG;
    }

    if (mcu) {
        if ((count & 0x3) != 0) {
            return SYNTIANT_NDP_ERROR_ARG;
        }
        if (out) {
            if (spiData != out) {
                memcpy(spiData, out, count);
            }
            digitalWrite(SPI_CS, LOW);
            SPI.transfer(SPI_MADDR);
            SPI.transfer(address & 0xff);
            SPI.transfer((address >> 8) & 0xff);
            SPI.transfer((address >> 16) & 0xff);
            SPI.transfer((address >> 24) & 0xff);
            SPI.transfer(spiData, count);
            digitalWrite(SPI_CS, HIGH);
        } else {
            digitalWrite(SPI_CS, LOW);
            SPI.transfer(SPI_MADDR);

            SPI.transfer(address & 0xff);
            SPI.transfer((address >> 8) & 0xff);
            SPI.transfer((address >> 16) & 0xff);
            SPI.transfer((address >> 24) & 0xff);

            digitalWrite(SPI_CS, HIGH);
            delayMicroseconds(1);
            digitalWrite(SPI_CS, LOW);

            SPI.transfer(0x80 | SPI_MADDR);
            SPI.transfer(dummy, 4);
            for (i = 0; i < count; i += 4) {
                SPI.transfer(&in[i], 4);
            }
            digitalWrite(SPI_CS, HIGH);
        }
    } else {
        if (out) {
            if (out != spiData) {
                memcpy(spiData, out, count);
            }
            if (address == SPI_SAMPLE) {
                SPI.beginTransaction(SPISettings(spiSpeedSampleWrite,
                                                 MSBFIRST, SPI_MODE0));
            }
            digitalWrite(SPI_CS, LOW);
            SPI.transfer(address);
            SPI.transfer(spiData, count);
            digitalWrite(SPI_CS, HIGH);

            if (address == SPI_SAMPLE) {
                SPI.beginTransaction(SPISettings(spiSpeedGeneral, MSBFIRST,
                                                 SPI_MODE0));
            }
        } else {
            digitalWrite(SPI_CS, LOW);
            SPI.transfer(0x80 | address);
            SPI.transfer(in, count);
            digitalWrite(SPI_CS, HIGH);
        }
    }
    return s;
}

// attaches a function returning void to interrupt pin
void NDPClass::setInterrupt(uint8_t intPin, void (*f)(void))
{
    pinMode(intPin, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(intPin), (*f), RISING);
}

int NDPClass::loadLog(uint8_t *fileBuf, uint32_t numBytes)
{
    return syntiant_ndp10x_micro_load_log(&ndp, fileBuf, numBytes);
}

// returns match class starting from 1
int NDPClass::poll(void)
{
    int s;
    uint32_t v;
    int match = 0;

    s = syntiant_ndp10x_micro_poll(&ndp, &v, 1);

    // check from interrupt signaling keyword match
    if (!s && (v & SYNTIANT_NDP10X_MICRO_NOTIFICATION_MATCH)) {
        s = syntiant_ndp10x_micro_get_match(&ndp, &match);
        if (!s && 0 <= match) {
            // limit range of match class to 11 for google10
            // (just in case somebody loads a package with even more
            // classes)
            if (match < 0) {
                match = 0;
            }
            match &= 0xf;
            if (match > 10) {
                match = 10;
            }
            match += 1; // ensures match is greater than 0.
                        // returning 0 signifies "no match found"
        }
    }
    return match;
}

int NDPClass::setExtractMatch(unsigned int prefix)
{
    unsigned int len = prefix;
    
    return syntiant_ndp10x_micro_extract_data
        (&ndp, SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_MATCH, NULL, &len);
}

int NDPClass::setExtractNow(void)
{
    unsigned int len = 0;
    
    return syntiant_ndp10x_micro_extract_data
        (&ndp, SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_NEWEST, NULL, &len);
}

int NDPClass::extractData(uint8_t *data, unsigned int *len)
{
    return syntiant_ndp10x_micro_extract_data
        (&ndp, SYNTIANT_NDP10X_MICRO_EXTRACT_FROM_UNREAD, data, len);
}

void NDPClass::setSpiSpeed(uint32_t speed)
{
    spiSpeedGeneral = speed;
}

void NDPClass::setSpiSpeedSw(uint32_t speed)
{
    spiSpeedSampleWrite = speed;
}
