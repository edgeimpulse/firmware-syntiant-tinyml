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

#ifndef NDP_GPIO_H
#define NDP_GPIO_H

// Define Arduino pins
// see https://github.com/arduino/ArduinoCore-samd/blob/master/variants/mkrzero/variant.cpp
enum Arduino_GPIO_Pins
{
    ARDUINO_PIN_11 = 11,
    ARDUINO_PIN_12 = 12,
    ARDUINO_PIN_A6 = 21,
    FLASH_CS = 17,      // Flash Chip Select for reading/writing flash
    BATTERY = 19,       // Battery monitor pin A4
    ENABLE_PDM = 25,    // enable line for PDM buffer. Low = enable.   set to 1 (off) this to switch from microphone to sensor mode?
    SD_CS = 28,
    SD_CARD_SENSE = 30, // SD card present sense pin
    USER_SWITCH = 31, // user can attach a switch here, and in the main .ino file
                      // define the functionality when pressed
    NDP_INT = A1,       // NDP INT pin 16
    LED_RED = LED_BUILTIN,
    LED_BLUE = A4,
    LED_GREEN = A5,
    PMU_OTG = A6,
};

#endif
