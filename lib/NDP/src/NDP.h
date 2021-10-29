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

#ifndef NDP_H
#define NDP_H

#include <stdint.h>
#include <Arduino.h>
#include <syntiant_ndp10x_micro_arduino.h>

#include "SPI.h"

#if ARDUINO < 10606
#error NDP requires Arduino IDE 1.6.6 or greater. Please update your IDE.
#endif

#if !defined(USBCON)
#error NDP can only be used with an USB MCU.
#endif

#define is_write_enabled(x) (1)

#ifndef DOXYGEN_ARD
// the following would confuse doxygen documentation tool, so skip in that case for autodoc build
_Pragma("pack()")
#define WEAK __attribute__((weak))
#endif

extern uint8_t spiData[2048];
extern uint32_t spiSpeedGeneral;
extern uint32_t spiSpeedSampleWrite;

// SPI chip select, used for spiTransfer. Define in the main .ino file.
extern byte SPI_CS;

class NDPClass
{
 public:
    NDPClass();

    void init();

    // pass a user-defined function to be called every time an interrupt
    // from the ndp is received
    void setInterrupt(uint8_t intPin, void (*f)(void));

    // Load a uILib log buffer
    // returns a SYNTIANT_NDP_ERROR_ status code
    //   NONE: load successfully completed
    //   MORE: more of the log remains to be processed
    //   other: error during loading the uILib log buffer
    int loadLog(uint8_t *buf, uint32_t numBytes);

    // Check for a match event.
    // Returns:
    //   0 - no match
    //   0 < - matched ID n + 1
    int poll(void);

    // Set the data extraction point to now, i.e. flush old data.
    // returns a SYNTIANT_NDP_ERROR_ status code
    int setExtractNow(void);
    
    // Set the data extraction point relative to the most recent match.
    // Call this after a match is reported if desired.
    // prefix (in): bytes before the match from which to begin extracting
    // returns a SYNTIANT_NDP_ERROR_ status code:
    //   NONE: load successfully completed
    //   ARG:  len is greater than available buffered audio
    //   other: other errors
    int setExtractMatch(unsigned int len);
    
    // Extract data from the holding tank.
    // data (in): buffer into which to store extracted data -- pass NULL
    //            to determine available data without extracting any data
    // len (in): maximum bytes to extract (i.e. size of data)
    // len (out): bytes that could have been extracted if len was
    //            'unconstrained'
    //
    // the amount of data actually extracted can be computed as:
    //      min(input len, returned len)
    // 
    // returns a SYNTIANT_NDP_ERROR_ status code
    int extractData(uint8_t *data, unsigned int *len);


    // passed to uilib, used for all SPI transfers such as in loadLog and poll
    static int spiTransfer(void *d, int mcu, uint32_t address, void *_out,
                           void *_in, unsigned int count);

    void setSpiSpeed(uint32_t speed);
    void setSpiSpeedSw(uint32_t speed);

    
 protected:
    struct syntiant_ndp10x_micro_device_s ndp;
};

extern NDPClass NDP;

#endif

