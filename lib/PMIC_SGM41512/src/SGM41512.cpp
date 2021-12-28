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

#include "SGM41512.h"

//Default PMIC (SGM41512) I2C address
#define PMIC_ADDRESS 0x6B

// Register address definitions (only the ones used here)
#define PMIC_POWERON_CONFIG_REG 0x01
#define PMIC_CHARGE_VOLTAGE_CONTROL_REG 0x04
#define PMIC_CHARGE_TIMER_CONTROL_REG 0x05
#define PMIC_MISC_CONTROL_REG 0x07
#define PMIC_SYSTEM_STATUS_REG 0x08
#define PMIC_RESET_AND_VERSION_REG 0x0B

PMICClass::PMICClass(TwoWire &wire) : _wire(&wire)
{
}

/*******************************************************************************
 * Function Name  : begin
 * Description    : Initializes the I2C for the PMIC module
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::begin()
{
    _wire->begin();

#ifdef ARDUINO_ARCH_SAMD
    pinMode(PIN_USB_HOST_ENABLE - 3, OUTPUT);
    //digitalWrite(PIN_USB_HOST_ENABLE, LOW);
    digitalWrite(PIN_USB_HOST_ENABLE - 3, HIGH);

#if defined(ARDUINO_SAMD_MKRGSM1400) || defined(ARDUINO_SAMD_MKRNB1500)
    pinMode(PMIC_IRQ_PIN, INPUT_PULLUP);
#endif
#endif

    //check PMIC version --- ignoring the last two-bit field, DEV_REV[1:0]
    if ((readRegister(PMIC_RESET_AND_VERSION_REG) | 0x03) != 0x2F)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*******************************************************************************
 * Function Name  : end
 * Description    : Deinitializes the I2C for the PMIC module
 * Input          : NONE
 * Return         : NONE
 *******************************************************************************/
void PMICClass::end()
{
#ifdef ARDUINO_ARCH_SAMD
    //pinMode(PIN_USB_HOST_ENABLE, INPUT);
    pinMode(PIN_USB_HOST_ENABLE - 3, INPUT);

#if defined(ARDUINO_SAMD_MKRGSM1400) || defined(ARDUINO_SAMD_MKRNB1500)
    pinMode(PMIC_IRQ_PIN, INPUT);
#endif
#endif
    _wire->end();
}

/*******************************************************************************
 * Function Name  : enableCharge
 * Description    : Enables PMIC charge mode
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::enableCharge()
{
#ifdef ARDUINO_ARCH_SAMD
    //digitalWrite(PIN_USB_HOST_ENABLE, LOW);
    digitalWrite(PIN_USB_HOST_ENABLE - 3, HIGH);

#endif
    int DATA = readRegister(PMIC_POWERON_CONFIG_REG);

    if (DATA == -1) {
        return 0;
    }
    byte mask = DATA & 0xCF;

    // Enable PMIC battery charging mode
    if (!writeRegister(PMIC_POWERON_CONFIG_REG, mask | 0x10)) {
        return 0;
    }
    DATA = readRegister(PMIC_CHARGE_TIMER_CONTROL_REG);

    if (DATA == -1) {
        return 0;
    }
    mask = DATA & 0x7F;
    // Enable charge termination feature
    if (!writeRegister(PMIC_CHARGE_TIMER_CONTROL_REG, mask | 0x80)) {
        return 0;
    }

    DATA = readRegister(PMIC_CHARGE_VOLTAGE_CONTROL_REG);

    if (DATA == -1) {
        return 0;
    }
    // Enable Top-off timer
    if (!writeRegister(PMIC_CHARGE_VOLTAGE_CONTROL_REG, DATA | 0x03)) {
        return 0;
    }

    return enableBATFET();
}

/*******************************************************************************
 * Function Name  : enableBoostMode
 * Description    : Enables PMIC boost mode, allow to generate 5V from battery
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::enableBoostMode()
{
    int DATA = readRegister(PMIC_POWERON_CONFIG_REG);

    if (DATA == -1) {
        return 0;
    }
    byte mask = DATA & 0xCF;
    // Enable PMIC boost mode
    if (!writeRegister(PMIC_POWERON_CONFIG_REG, mask | 0x20)) {
        return 0;
    }
#ifdef ARDUINO_ARCH_SAMD
    //digitalWrite(PIN_USB_HOST_ENABLE, LOW);
    digitalWrite(PIN_USB_HOST_ENABLE - 3, HIGH);

#endif
    // Disable charge termination feature
    DATA = readRegister(PMIC_CHARGE_TIMER_CONTROL_REG);

    if (DATA == -1) {
        return 0;
    }
    mask = DATA & 0x7F; // remove "enable termination" bit
    if (!writeRegister(PMIC_CHARGE_TIMER_CONTROL_REG, mask)) {
        return 0;
    }

    // wait for enable boost mode
    delay(500);
    return 1;
}

/*******************************************************************************
 * Function Name  : enableBATFET
 * Description    : turn on BATFET
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::enableBATFET(void)
{

    int DATA = readRegister(PMIC_MISC_CONTROL_REG);

    if (DATA == -1) {
        return 0;
    }

    return writeRegister(PMIC_MISC_CONTROL_REG, (DATA & 0b11011111));
}

/*******************************************************************************
 * Function Name  : chargeStatus
 * Description    : Query the PMIC and returns the Charging status
 * Input          : NONE
 * Return         : -1 on Error, Charging status on Success
 *******************************************************************************/
int PMICClass::chargeStatus()
{

    int DATA = readRegister(PMIC_SYSTEM_STATUS_REG);

    if (DATA == -1) {
        return DATA;
    }
    byte charge_staus = (DATA & 0x18)>>3;
    return charge_staus;
}

/*******************************************************************************
 * Function Name  : getOperationMode
 * Description    : Query the PMIC and returns the mode of operation
 * Input          : NONE
 * Return         : -1 on Error, 0 on Charge mode, 1 on Boost mode
 *******************************************************************************/
int PMICClass::getOperationMode()
{

    int DATA = readRegister(PMIC_POWERON_CONFIG_REG);

    if (DATA == -1) {
        return DATA;
    }
    byte mode_staus = (DATA & 0x20)>>5;
    return mode_staus;
}

/*******************************************************************************
 * Function Name  : disableWatchdog
 * Description    : Disable Watchdog timer
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::disableWatchdog(void)
{

    int DATA = readRegister(PMIC_CHARGE_TIMER_CONTROL_REG);

    if (DATA == -1) {
        return 0;
    }

    return writeRegister(PMIC_CHARGE_TIMER_CONTROL_REG, (DATA & 0b11001111));
}


/*******************************************************************************
 * Function Name  : readRegister
 * Description    : read the register with the given address in the PMIC 
 * Input          : register address
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
int PMICClass::readRegister(byte address)
{
    _wire->beginTransmission(PMIC_ADDRESS);
    _wire->write(address);

    if (_wire->endTransmission(true) != 0) {
        return -1;
    }

    if (_wire->requestFrom(PMIC_ADDRESS, 1, true) != 1) {
        return -1;
    }

    return _wire->read();
}


/*******************************************************************************
 * Function Name  : writeRegister
 * Description    : write a value in the register with the given address in the PMIC 
 * Input          : register address, value
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
int PMICClass::writeRegister(byte address, byte val)
{
    _wire->beginTransmission(PMIC_ADDRESS);
    _wire->write(address);
    _wire->write(val);

    if (_wire->endTransmission(true) != 0) {
        return 0;
    }

    return 1;
}

PMICClass PMIC(Wire);
