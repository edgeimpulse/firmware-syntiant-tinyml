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

#include "NDP_PMU.h"

//Writes val to address register on device
void writeTo(int device, byte address, byte val)
{
    Wire.beginTransmission(device); // start transmission to device
    Wire.write(address);            // send register address
    Wire.write(val);                // send value to write
    Wire.endTransmission();         // end transmission
}

//reads num bytes starting from address register on device in to buff array
void readFrom(int device, byte address, int num, byte buff[])
{
    Wire.beginTransmission(device); // start transmission to device
    Wire.write(address);            // sends address to read from
    Wire.endTransmission();         // end transmission
    Wire.beginTransmission(device); // start transmission to device
    Wire.requestFrom(device, num);  // request num bytes from device

    int i = 0;
    while (Wire.available())  {
        buff[i] = Wire.read(); // receive a byte
        i++;
    }
    // device may send less than requested (abnormal)
    Wire.endTransmission(); // end transmission
}

void printPmu()
{
    Serial.print("SGM41512 Registers - ");

    byte buff[10];
    for (byte i = 0; i < 0xc; i++) { // SGM41512 has 11 registers.
        readFrom(SGM41512, i, 1, buff);
        Serial.print(i);
        Serial.print(" = 0x");
        Serial.print(buff[0], HEX);
        Serial.print(", ");
    }
    Serial.println("");
}

// Print LiPo battery voltage. Maximum (3v7) should give 0x3ff
void printBattery()
{
    Serial.print("Battery = ");
    Serial.print(100 * analogRead(ADC_BATTERY) / 0x3ff);
    Serial.println("%");
}

// Set PMU into Boost mode. This generates 5v on the PMU PMD pin
void pmuBoost()
{
    //Serial.println("Reseting PMU Registers");

    // reset PMU registers
    writeTo(SGM41512, RESET_AND_VERSION_REG, 0xAC);

    Serial.println("Enabling boost mode");

    // Set PMU_OTG pin high to enable boost mode
    digitalWrite(PMU_OTG, HIGH);
    digitalWrite(ENABLE_5V, HIGH);

    if (!PMIC.begin()) {
        Serial.println("Failed to initialize PMIC!");
        return;
    }

    // Enable boost mode, this mode allow to use the board as host and
    // connect guest device like keyboard
    if (!PMIC.enableBoostMode())  {
        Serial.println("Error enabling boost mode");
    }

    // disable watchdog
    PMIC.disableWatchdog();
}

// Place PMU into PiPo charge mode. This cancels boost mode (if activated)
void pmuCharge()
{
    // reset PMU registers
    writeTo(SGM41512, RESET_AND_VERSION_REG, 0xAC);

    Serial.println("Enabling battery charging mode");

    // Set OTB low (no boost mode). Disable 5v out
    //digitalWrite(PMU_OTG, HIGH);
    digitalWrite(ENABLE_5V, LOW);

    // Reseting the registers set the voltage/current limit
    // and charge voltage correctly
  
    // Set the charge current to 180mA
    writeTo(SGM41512, CHARGE_CURRENT_CTL_REG, 0x83);
    // Enable the Charger
    if (!PMIC.enableCharge()) {
        Serial.println("Error enabling battery charging mode");
    }
    // disable watchdog
    PMIC.disableWatchdog();
}
