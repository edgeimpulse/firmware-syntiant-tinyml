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
    while (Wire.available())
    {                          // device may send less than requested (abnormal)
        buff[i] = Wire.read(); // receive a byte
        i++;
    }
    Wire.endTransmission(); // end transmission
}

void printPmu()
{
    Serial2.print("bq24195L Registers - ");

    byte buff[10];
    for (byte i = 0; i < 0xb; i++)
    {
        readFrom(BQ24195L, i, 1, buff);
        Serial2.print(i);
        Serial2.print(" = 0x");
        Serial2.print(buff[0], HEX);
        Serial2.print(", ");
    }
    Serial2.println("");
}

// Print LiPo battery voltage. Maximum (3v7) should give 0x3ff
void printBattery()
{
    Serial2.print("Battery = ");
    Serial2.print(100 * analogRead(ADC_BATTERY) / 0x3ff);
    Serial2.println("%");
}

// Set PMU into Boost mode. This generates 5v on the PMU PMD pin
void pmuBoost()
{
    Serial2.println("Reseting PMU Registers");

    // reset PMU registers
    writeTo(BQ24195L, POWER_ON_CONFIG, 0x80);

    Serial2.println("Enabling Boost Mode");

    // Set PMU_OTG pin high to enable boost mode
    digitalWrite(PMU_OTG, HIGH);

    if (!PMIC.begin())
    {
        Serial2.println("Failed to initialize PMIC!");
        return;
    }

    // Enable boost mode, this mode allow to use the board as host and
    // connect guest device like keyboard
    if (!PMIC.enableBoostMode())
    {
        Serial2.println("Error enabling Boost Mode");
    }
    else
    {
        Serial2.println("Boost Mode Enabled");
    }

    // disable watchdog
    writeTo(BQ24195L, CHARGE_TERMINATION_TIMER_CTL, 0xa);
    printPmu();
}

// Place PMU into PiPo charge mode. This cancels boost mode (if activated)
void pmuCharge()
{
    Serial2.println("Reseting PMU Registers");

    // reset PMU registers
    writeTo(BQ24195L, POWER_ON_CONFIG, 0x80);

    // Set OTB low (no boost mode). Disable 5v out
    digitalWrite(PMU_OTG, HIGH);
    // digitalWrite(ENABLE_5V, LOW);

    // Set the input current limit to 2 A and the overload input voltage to 3.88 V
    if (!PMIC.setInputCurrentLimit(2.0))
    {
        Serial2.println("Error in set input current limit");
    }

    if (!PMIC.setInputVoltageLimit(3.88))
    {
        Serial2.println("Error in set input voltage limit");
    }

    // set the minimum voltage used to feeding the module embed on Board
    if (!PMIC.setMinimumSystemVoltage(3.5))
    {
        Serial2.println("Error in set minimum system volage");
    }

    // Set the desired charge voltage to 4.11 V
    if (!PMIC.setChargeVoltage(4.2))
    {
        Serial2.println("Error in set charge volage");
    }

    // Set the charge current to 375 mA
    // the charge current should be definde as maximum at (C for hour)/2h
    // to avoid battery explosion (for example for a 750mAh battery set to 0.375 A)
    if (!PMIC.setChargeCurrent(0.375))
    {
        Serial2.println("Error in set charge current");
    }

    // Enable the Charger
    if (!PMIC.enableCharge())
    {
        Serial2.println("Error enabling Charge mode");
    }

    printPmu();
}
