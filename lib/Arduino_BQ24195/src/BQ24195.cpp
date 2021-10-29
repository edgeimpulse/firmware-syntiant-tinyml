/*
  This file is part of the Arduino_PMIC library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "BQ24195.h"

//Default PMIC (BQ24195) I2C address
#define PMIC_ADDRESS 0x6B

#ifndef round
#define round(x) ((x) >= 0 ? (long)((x) + 0.5) : (long)((x)-0.5))
#endif

// Register address definitions
#define INPUT_SOURCE_REGISTER 0x00
#define POWERON_CONFIG_REGISTER 0x01
#define CHARGE_CURRENT_CONTROL_REGISTER 0x02
#define PRECHARGE_CURRENT_CONTROL_REGISTER 0x03
#define CHARGE_VOLTAGE_CONTROL_REGISTER 0x04
#define CHARGE_TIMER_CONTROL_REGISTER 0x05
#define THERMAL_REG_CONTROL_REGISTER 0x06
#define MISC_CONTROL_REGISTER 0x07
#define SYSTEM_STATUS_REGISTER 0x08
#define FAULT_REGISTER 0x09
#define PMIC_VERSION_REGISTER 0x0A

enum Current_Limit_mask
{
    CURRENT_LIM_100 = 0x00,
    CURRENT_LIM_150,
    CURRENT_LIM_500,
    CURRENT_LIM_900,
    CURRENT_LIM_1200,
    CURRENT_LIM_1500,
    CURRENT_LIM_2000,
    CURRENT_LIM_3000,
};

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

    //check PMIC version
    if (readRegister(PMIC_VERSION_REGISTER) != 0x23)
    {
        return 0;
    }
    // Sets the charge current to 100 mA
    return setChargeCurrent(0.01);
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
 * Function Name  : enableChargeMode
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
    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }
    byte mask = DATA & 0xCF;

    // Enable PMIC Charge Mode
    if (!writeRegister(POWERON_CONFIG_REGISTER, mask | 0x10))
    {
        return 0;
    }
    DATA = readRegister(CHARGE_TIMER_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }
    mask = DATA & 0x7F;
    // Enable Charge Termination Pin
    if (!writeRegister(CHARGE_TIMER_CONTROL_REGISTER, mask | 0x80))
    {
        return 0;
    }

    // Enable Charge and Battery Fault Interrupt
    DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }
    mask = DATA & 0xFC;

    if (!writeRegister(MISC_CONTROL_REGISTER, mask | 0x03))
    {
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
    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }
    byte mask = DATA & 0xCF;
    // Enable PMIC Boost Mode
    if (!writeRegister(POWERON_CONFIG_REGISTER, mask | 0x20))
    {
        return 0;
    }
#ifdef ARDUINO_ARCH_SAMD
    //digitalWrite(PIN_USB_HOST_ENABLE, LOW);
    digitalWrite(PIN_USB_HOST_ENABLE - 3, HIGH);

#endif
    // Disable Charge Termination Pin
    DATA = readRegister(CHARGE_TIMER_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }
    mask = DATA & 0x7F; // remove "enable termination" bit
    if (!writeRegister(CHARGE_TIMER_CONTROL_REGISTER, mask))
    {
        return 0;
    }

    // Enable Battery Fault interrupt and disable Charge Fault Interrupt
    DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    mask = DATA & 0xFC;

    if (!writeRegister(MISC_CONTROL_REGISTER, mask | 0x03))
    {
        return 0;
    }
    // wait for enable boost mode
    delay(500);
    return 1;
}

/*******************************************************************************
 * Function Name  : disableCharge
 * Description    : Disable the battery charger
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::disableCharge()
{

    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }
    byte mask = DATA & 0xCF;

    if (!writeRegister(POWERON_CONFIG_REGISTER, mask))
    {
        return 0;
    }

    // Disable Charge Termination Pin
    DATA = readRegister(CHARGE_TIMER_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    mask = DATA & 0x7F;
    if (!writeRegister(CHARGE_TIMER_CONTROL_REGISTER, mask))
    {
        return 0;
    }

    DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }
    mask = DATA & 0xFC;

    return writeRegister(MISC_CONTROL_REGISTER, mask);
}

/*******************************************************************************
 * Function Name  : disableBoostMode
 * Description    : Disable the boost mode
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::disableBoostMode()
{
    int DATA = readRegister(POWERON_CONFIG_REGISTER);
#ifdef ARDUINO_ARCH_SAMD
    //digitalWrite(PIN_USB_HOST_ENABLE, HIGH);
    digitalWrite(PIN_USB_HOST_ENABLE - 3, LOW);

#endif

    if (DATA == -1)
    {
        return 0;
    }
    byte mask = DATA & 0xCF;

    // set default Charger Configuration value
    return writeRegister(POWERON_CONFIG_REGISTER, mask | 0x10);
}
/*******************************************************************************
 * Function Name  : setInputVoltageLimit
 * Description    : Set the lower input voltage limit, the PMIC set a base
                    voltage of 3.88 V plus a combination of bits 3 to 6 of
                    Input source control register where the LSB is 80 mV
 * Input          : Input voltage limit expressed in Volt
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::setInputVoltageLimit(float voltage)
{

    int DATA = readRegister(INPUT_SOURCE_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0x87;
    if (voltage > 5.08)
    {
        voltage = 5.08;
    }
    else if (voltage < 3.88)
    {
        voltage = 3.88;
    }

    // round(((desired voltage - base_voltage_offset) / minimum_votage_value) * LSB value)
    // where:
    // - BASE_VOLTAGE_OFFSET = 3.88 V;
    // - minimum_votage_value = 0.008 V (80 mV);
    // - LSB value = 8;
    // after mask with & 0x78 to set only the input voltage bits
    return writeRegister(INPUT_SOURCE_REGISTER, (mask | (round((voltage - 3.88) * 100) & 0x78)));
}

/*******************************************************************************
 * Function Name  : getInputVoltageLimit
 * Description    : Query the PMIC and returns the input voltage limit
 * Input          : NONE
 * Return         : NAN on Error, Input Voltage Limit value on Success
 *******************************************************************************/
float PMICClass::getInputVoltageLimit(void)
{
    int DATA = readRegister(INPUT_SOURCE_REGISTER);

    if (DATA == -1)
    {
        return NAN;
    }

    byte mask = DATA & 0x78;
    return (mask / 100.0f) + 3.88;
}

/*******************************************************************************
 * Function Name  : setInputCurrentLimit
 * Description    : Sets the upper input current limit for PMIC
 * Input          : input current limit expressed in Ampere
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::setInputCurrentLimit(float current)
{

    int DATA = readRegister(INPUT_SOURCE_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0xF8;
    byte current_val = CURRENT_LIM_100;

    if (current > 0.015)
    {
        current_val = CURRENT_LIM_150;
    }
    if (current >= 0.5)
    {
        current_val = CURRENT_LIM_500;
    }
    if (current >= CURRENT_LIM_900)
    {
        current_val = CURRENT_LIM_900;
    }
    if (current >= 1.2)
    {
        current_val = CURRENT_LIM_1200;
    }
    if (current >= 1.5)
    {
        current_val = CURRENT_LIM_1500;
    }
    if (current >= 2.0)
    {
        current_val = CURRENT_LIM_2000;
    }
    if (current >= 3.0)
    {
        current_val = CURRENT_LIM_3000;
    }
    return writeRegister(INPUT_SOURCE_REGISTER, (mask | current_val));
}

/*******************************************************************************
 * Function Name  : getInputCurrentLimit
 * Description    : Query the PMIC and returns the input current limit
 * Input          : NONE
 * Return         : NAN on Error, Input current Limit value on Success
 *******************************************************************************/
float PMICClass::getInputCurrentLimit(void)
{
    int DATA = readRegister(INPUT_SOURCE_REGISTER);

    if (DATA == -1)
    {
        return NAN;
    }

    byte mask = DATA & 0x07;

    switch (mask)
    {
    case CURRENT_LIM_100:
        return 0.1;
    case CURRENT_LIM_150:
        return 0.015;
    case CURRENT_LIM_500:
        return 0.5;
    case CURRENT_LIM_900:
        return 0.9;
    case CURRENT_LIM_1200:
        return 1.2;
    case CURRENT_LIM_1500:
        return 1.5;
    case CURRENT_LIM_2000:
        return 2.0;
    case CURRENT_LIM_3000:
        return 3.0;
    default:
        return NAN;
    }
    return NAN;
}

/*******************************************************************************
 * Function Name  : readInputSourceRegister
 * Description    : Query the PMIC and returns the Input Source Register value
 * Input          : NONE
 * Return         : -1 on Error, Input Source Register byte on Success
 *******************************************************************************/
byte PMICClass::readInputSourceRegister(void)
{

    return readRegister(INPUT_SOURCE_REGISTER);
}

/*******************************************************************************
 * Function Name  : enableBuck
 * Description    : Enable the Buck regulator
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::enableBuck(void)
{

    int DATA = readRegister(INPUT_SOURCE_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(INPUT_SOURCE_REGISTER, (DATA & 0b01111111));
}

/*******************************************************************************
 * Function Name  : disableBuck
 * Description    : Disable the Buck regulator
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::disableBuck(void)
{

    int DATA = readRegister(INPUT_SOURCE_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(INPUT_SOURCE_REGISTER, (DATA | 0b10000000));
}

/*******************************************************************************
 * Function Name  : readPowerONRegister
 * Description    : Query the PMIC and returns the Power ON Register value
 * Input          : NONE
 * Return         : -1 on Error, read Power ON Register byte on Success
 *******************************************************************************/
byte PMICClass::readPowerONRegister(void)
{

    return readRegister(POWERON_CONFIG_REGISTER);
}

/*******************************************************************************
 * Function Name  : enableCharging
 * Description    : Enable the battery charger
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::enableCharging()
{

    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    DATA = DATA & 0b11001111;
    DATA = DATA | 0b00010000;
    return writeRegister(POWERON_CONFIG_REGISTER, DATA);
}

/*******************************************************************************
 * Function Name  : disableCharging
 * Description    : Disable the battery charger
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::disableCharging()
{

    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(POWERON_CONFIG_REGISTER, DATA & 0xCF);
}

/*******************************************************************************
 * Function Name  : disableOTG
 * Description    : Disable the boost mode
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::disableOTG(void)
{

    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    DATA = DATA & 0b11001111;
    DATA = DATA | 0b00010000;
    return writeRegister(POWERON_CONFIG_REGISTER, DATA);
}

/*******************************************************************************
 * Function Name  : enableOTG
 * Description    : Enable the boost mode
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::enableOTG(void)
{

    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    DATA = DATA & 0b11001111;
    DATA = DATA | 0b00100000;

    if (!writeRegister(POWERON_CONFIG_REGISTER, DATA))
    {
        return 0;
    }

    // required time to enable boost mode operations
    delay(220);

    return 1;
}

/*******************************************************************************
 * Function Name  : resetWatchdog
 * Description    : reset I2C watchdog timer
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::resetWatchdog()
{
    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(POWERON_CONFIG_REGISTER, (DATA | 0b01000000));
}

/*******************************************************************************
 * Function Name  : setMinimumSystemVoltage
 * Description    : Set the minimum acceptable voltage to feed the onboard
                    mcu and module
 * Input          : Minimum system voltage in Volt
 * Return         : 0 Error, 1 Success
*******************************************************************************/
bool PMICClass::setMinimumSystemVoltage(float voltage)
{

    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0xF0;
    if (voltage > 3.7f)
    {
        voltage = 3.7f;
    }
    else if (voltage < 3.0f)
    {
        voltage = 3.0f;
    }

    return writeRegister(POWERON_CONFIG_REGISTER, (mask | ((round((voltage - 3.0) * 10) * 2) + 1)));
}
/*******************************************************************************
 * Function Name  : getMinimumSystemVoltage
 * Description    : returns the set minimum system voltage in Volt
 * Input          : NONE
 * Return         : 0 on Error, the minimum system voltage value on Success
*******************************************************************************/
float PMICClass::getMinimumSystemVoltage(void)
{
    int DATA = readRegister(POWERON_CONFIG_REGISTER);

    if (DATA == -1)
    {
        return NAN;
    }

    byte mask = DATA & 0x0E;
    return (mask / 20.0f) + 3.0f;
}

/*******************************************************************************
 * Function Name  : setChargeCurrent
 * Description    : Set charge current
 * Input          : The current expressed in Ampere
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::setChargeCurrent(float current)
{
    int DATA = readRegister(CHARGE_CURRENT_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }
    byte mask = DATA & 0x01;

    if (current > 4.544)
    {
        current = 4.544;
    }
    else if (current < 0.512)
    {
        current = 0.512;
    }

    return writeRegister(CHARGE_CURRENT_CONTROL_REGISTER, (round(((current - 0.512) / 0.016)) & 0xFC) | mask);
}

/*******************************************************************************
 * Function Name  : getChargeCurrent
 * Description    : Query the PMIC and returns the charge current value in Ampere
 * Input          : NONE
 * Return         : NAN on error, The charge current on Success
 *******************************************************************************/
float PMICClass::getChargeCurrent(void)
{
    int DATA = readRegister(CHARGE_CURRENT_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return NAN;
    }

    byte mask = DATA & 0xFC;
    return ((mask * 0.016f) + 0.512f);
}

/*******************************************************************************
 * Function Name  : setPreChargeCurrent
 * Description    : Set PMIC precharge current limit
 * Input          : The current expressed in Ampere
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::setPreChargeCurrent(float current)
{
    int DATA = readRegister(PRECHARGE_CURRENT_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0x0F;
    if (current > 2.048)
    {
        current = 2.048;
    }
    else if (current < 0.128)
    {
        current = 0.128;
    }
    return writeRegister(PRECHARGE_CURRENT_CONTROL_REGISTER, mask | (round((current - 0.128f) / 0.008f) & 0xF0));
}

/*******************************************************************************
 * Function Name  : getPreChargeCurrent
 * Description    : Query the PMIC and returns the precharge current limit
                    in Ampere
 * Input          : NONE
 * Return         : NAN on error, The precharge current limit on Success
 *******************************************************************************/
float PMICClass::getPreChargeCurrent(void)
{
    int DATA = readRegister(PRECHARGE_CURRENT_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return NAN;
    }

    byte mask = DATA & 0xF0;
    return (mask * 0.008f) + 0.128f;
}

/*******************************************************************************
 * Function Name  : setTermChargeCurrent
 * Description    : Set PMIC termination charge current limit
 * Input          : The current expressed in Ampere
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::setTermChargeCurrent(float current)
{
    int DATA = readRegister(PRECHARGE_CURRENT_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0xF0;

    if (current > 2.048)
    {
        current = 2.048;
    }
    else if (current < 0.128)
    {
        current = 0.128;
    }

    return writeRegister(PRECHARGE_CURRENT_CONTROL_REGISTER, mask | (round((current - 0.128) / 0.128) & 0x0F));
}

/*******************************************************************************
 * Function Name  : getTermChargeCurrent
 * Description    : Query the PMIC and returns the termination charge current
                    limit in Ampere
 * Input          : NONE
 * Return         : NAN on error, The termination charge current limit on Success
 *******************************************************************************/
float PMICClass::getTermChargeCurrent(void)
{
    int DATA = readRegister(PRECHARGE_CURRENT_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return NAN;
    }

    byte mask = DATA & 0x0F;
    return (mask * 0.128f) + 0.128f;
}

/*******************************************************************************
 * Function Name  : setChargeVoltage
 * Description    : Sets the PMIC charge voltage limit
 * Input          : Charge voltage in Volt
 * Return         : 0 Error, 1 Success
 *******************************************************************************/
bool PMICClass::setChargeVoltage(float voltage)
{
    int DATA = readRegister(CHARGE_VOLTAGE_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0x03;
    if (voltage > 4.512)
    {
        voltage = 4.512;
    }
    else if (voltage < 3.504)
    {
        voltage = 3.504;
    }
    return writeRegister(CHARGE_VOLTAGE_CONTROL_REGISTER, (round(((voltage - 3.504) / 0.004)) & 0xFC) | mask);
}

/*******************************************************************************
 * Function Name  : getChargeVoltage
 * Description    : Query the PMIC and returns the charge voltage limit in Volt
 * Input          : NONE
 * Return         : NAN on error, The charge voltage limit on Success
 *******************************************************************************/
float PMICClass::getChargeVoltage(void)
{
    int DATA = readRegister(CHARGE_VOLTAGE_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return NAN;
    }

    byte mask = DATA & 0xFC;
    return ((mask * 0.004f) + 3.504f);
}

/*******************************************************************************
 * Function Name  : readChargeTermRegister
 * Description    : Query the PMIC and returns the Charge Termination/Timer
                    Control Register value
 * Input          : NONE
 * Return         :-1 on Error, Charge Termination/Timer Control Register byte
                    on Success
 *******************************************************************************/
byte PMICClass::readChargeTermRegister(void)
{

    return readRegister(CHARGE_TIMER_CONTROL_REGISTER);
}

/*******************************************************************************
 * Function Name  : disableWatchdog
 * Description    : Disable Watchdog timer
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::disableWatchdog(void)
{

    int DATA = readRegister(CHARGE_TIMER_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(CHARGE_TIMER_CONTROL_REGISTER, (DATA & 0b11001110));
}

/*******************************************************************************
 * Function Name  : setThermalRegulationTemperature
 * Description    : Sets the Thermal Regulation Threshold
 * Input          : Threshold degree in [C]
 * Return         : 0 Error, 1 Success
 *******************************************************************************/
bool PMICClass::setThermalRegulationTemperature(int degree)
{
    int DATA = readRegister(THERMAL_REG_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0xFC;
    if (degree > 120)
    {
        degree = 120;
    }
    else if (degree < 60)
    {
        degree = 60;
    }
    return writeRegister(THERMAL_REG_CONTROL_REGISTER, (mask | (round((degree - 60) / 20) & 0x03)));
}

/*******************************************************************************
 * Function Name  : getThermalRegulationTemperature
 * Description    : Query the PMIC and returns the Thermal Regulation
                    Temperature Threshold
 * Input          : NONE
 * Return         : -1 on Error, Thermal Regulation Threshold on Success
 *******************************************************************************/
int PMICClass::getThermalRegulationTemperature()
{
    int DATA = readRegister(THERMAL_REG_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return DATA;
    }

    byte mask = DATA & 0x03;
    return ((mask * 20) + 60);
}

/*******************************************************************************
 * Function Name  : disableDPDM
 * Description    : Disable the DPDM detection
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
 *******************************************************************************/
bool PMICClass::disableDPDM()
{

    int DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(MISC_CONTROL_REGISTER, (DATA & 0b01111111));
}

/*******************************************************************************
 * Function Name  : enableDPDM
 * Description    : Enable the DPDM detection
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::enableDPDM()
{

    int DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(MISC_CONTROL_REGISTER, (DATA | 0b10000000));
}

/*******************************************************************************
 * Function Name  : enableBATFET
 * Description    : turn on BATFET
 * Input          : NONE
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::enableBATFET(void)
{

    int DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(MISC_CONTROL_REGISTER, (DATA & 0b11011111));
}

/*******************************************************************************
 * Function Name  : disableBATFET
 * Description    : Force BATFET Off
                    (this essentially disconnects the battery from the system)
 * Input          :
 * Return         : 0 on Error, 1 on Success
*******************************************************************************/
bool PMICClass::disableBATFET(void)
{

    int DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    return writeRegister(MISC_CONTROL_REGISTER, (DATA | 0b00100000));
}

/*******************************************************************************
 * Function Name  : enableChargeFaultINT
 * Description    : Enable interrupt during Charge fault
 * Input          : NONE
 * Return         : 0 on Error, 1 on Succes
*******************************************************************************/
bool PMICClass::enableChargeFaultINT()
{
    int DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0xFD;

    return writeRegister(MISC_CONTROL_REGISTER, mask | 0x02);
}

/*******************************************************************************
 * Function Name  : disableChargeFaultINT
 * Description    : Disable interrupt during Charge fault
 * Input          : NONE
 * Return         : 0 on Error, 1 on Succes
*******************************************************************************/
bool PMICClass::disableChargeFaultINT()
{
    int DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0xFD;

    return writeRegister(MISC_CONTROL_REGISTER, mask);
}

/*******************************************************************************
 * Function Name  : enableBatFaultINT
 * Description    : Enable interrupt during battery fault
 * Input          : NONE
 * Return         : 0 on Error, 1 on Succes
*******************************************************************************/
bool PMICClass::enableBatFaultINT()
{
    int DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0xFE;

    return writeRegister(MISC_CONTROL_REGISTER, mask | 0x01);
}

/*******************************************************************************
 * Function Name  : disableBatFaultINT
 * Description    : Disable interrupt during battery fault
 * Input          : NONE
 * Return         : 0 on Error, 1 on Succes
*******************************************************************************/
bool PMICClass::disableBatFaultINT()
{
    int DATA = readRegister(MISC_CONTROL_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    byte mask = DATA & 0xFE;

    return writeRegister(MISC_CONTROL_REGISTER, mask);
}

/*******************************************************************************
 * Function Name  : readOpControlRegister
 * Description    : Query the PMIC and returns the Op Control Register value
 * Input          : NONE
 * Return         : -1 on Error, Op Control Register byte on Success
*******************************************************************************/
byte PMICClass::readOpControlRegister(void)
{ //ne manca uno

    return readRegister(MISC_CONTROL_REGISTER);
}

/*******************************************************************************
 * Function Name  : USBmode
 * Description    : Query the PMIC and returns the Voltage bus status
 * Input          : NONE
 * Return         : -1 on Error, Voltage bus status on Success
 *******************************************************************************/
int PMICClass::USBmode()
{

    int DATA = readRegister(SYSTEM_STATUS_REGISTER);
    if (DATA == -1)
    {
        return DATA;
    }
    byte mask = DATA & 0xC0;
    return mask;
}

/*******************************************************************************
 * Function Name  : chargeStatus
 * Description    : Query the PMIC and returns the Charging status
 * Input          : NONE
 * Return         : -1 on Error, Charging status on Success
 *******************************************************************************/
int PMICClass::chargeStatus()
{

    int DATA = readRegister(SYSTEM_STATUS_REGISTER);

    if (DATA == -1)
    {
        return DATA;
    }
    byte mask = DATA & 0x30;
    return mask;
}

/*******************************************************************************
 * Function Name  : isPowerGood
 * Description    : Check if is power good
 * Input          : NONE
 * Return         : 0 Power NO Good, 1 Power Good
 *******************************************************************************/
bool PMICClass::isPowerGood(void)
{

    int DATA = readRegister(SYSTEM_STATUS_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    if (DATA & 0b00000100)
    {
        return 1;
    }
    else
        return 0;
}

/*******************************************************************************
 * Function Name  : isHot
 * Description    : Check if is in Thermal Regulation
 * Input          : NONE
 * Return         : 0 on Nomral state, 1 on Thermal Regulation state
 *******************************************************************************/
bool PMICClass::isHot(void)
{

    int DATA = 0;
    DATA = readRegister(SYSTEM_STATUS_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    if (DATA & 0b00000010)
    {
        return 1;
    }

    else
        return 0;
}

/*******************************************************************************
 * Function Name  : getVsysStat
 * Description    : Check if the battery voltage is lower than the input
                    system voltage limit
 * Input          : NONE
 * Return         : 0 if V_BAT > V_SYS_MIN, 1 if V_BAT < V_SYS_MIN
 *******************************************************************************/
bool PMICClass::canRunOnBattery()
{

    int DATA = 0;
    DATA = readRegister(SYSTEM_STATUS_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    if (DATA & 0x01)
    {
        return 1;
    }
    return 0;
}

/*******************************************************************************
 * Function Name  : isBattConnected
 * Description    : Check if a battery is connected
 * Input          : NONE
 * Return         : 0 if not connected, 1 if connected
 *******************************************************************************/
bool PMICClass::isBattConnected(void)
{

    int DATA = 0;
    DATA = readRegister(SYSTEM_STATUS_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    if ((DATA & 0x08) == 0x08)
    {
        return 1;
    }

    else
        return 0;
}

/*******************************************************************************
 * Function Name  : readSystemStatusRegeister
 * Description    : Query the PMIC and returns the System Status Register value
 * Input          : NONE
 * Return         : -1 on Error, System Status Register byte on Success
 *******************************************************************************/
byte PMICClass::readSystemStatusRegister()
{

    int DATA = 0;
    DATA = readRegister(SYSTEM_STATUS_REGISTER);
    return DATA;
}

/*******************************************************************************
 * Function Name  : readFaultRegister
 * Description    : Query the PMIC and returns the Fault Register value
 * Input          : NONE
 * Return         : -1 on Error, Fault Register byte on Success
 *******************************************************************************/
byte PMICClass::readFaultRegister()
{

    int DATA = 0;
    DATA = readRegister(FAULT_REGISTER);
    return DATA;
}

/*******************************************************************************
 * Function Name  : getChargeFault
 * Description    : Query the PMIC and returns charge fault status
 * Input          : NONE
 * Return         : -1 on reading Error, 0 if no fault, a fault bits combination
                    on PMIC charge fault
 *******************************************************************************/
int PMICClass::getChargeFault()
{
    int DATA = 0;
    DATA = readRegister(FAULT_REGISTER);

    if (DATA == -1)
    {
        return DATA;
    }

    return DATA & 0x30;
}

/*******************************************************************************
 * Function Name  : hasBatteryTemperatureFault
 * Description      Query the PMIC and returns if battery temperature is out
                    of the required safety charge temperature range
                    On MKR boards the NTC resistor is mounted on the PCB so the
                    status is not reliable
 * Input          : NONE
 * Return         : -1 on read Error, 0 on normal state, a NTC fault bits combination
                    on NTC fault
 *******************************************************************************/
int PMICClass::hasBatteryTemperatureFault()
{
    int DATA = 0;
    DATA = readRegister(FAULT_REGISTER);

    if (DATA == -1)
    {
        return DATA;
    }

    if (!PMIC.disableOTG())
    {
        return -1;
    }

    return DATA & 0x07;
}

/*******************************************************************************
 * Function Name  : isWatchdogExpired
 * Description    : Query the PMIC and returns if watchdog time expires
 * Input          : NONE
 * Return         : 0 on normal state, 1 if watchdog timer expired
 *******************************************************************************/
bool PMICClass::isWatchdogExpired()
{
    int DATA = readRegister(FAULT_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    if ((DATA & 0x80))
    {
        return 1;
    }
    return 0;
}

/*******************************************************************************
 * Function Name  : isBatteryInOverVoltage
 * Description    : Query the PMIC and returns if battery over-voltage fault occurs
 * Input          : NONE
 * Return         : 0 on normal state, 1 if battery over voltage fault occurs
 *******************************************************************************/
bool PMICClass::isBatteryInOverVoltage()
{
    int DATA = readRegister(FAULT_REGISTER);

    if (DATA == -1)
    {
        return 0;
    }

    if ((DATA & 0x08))
    {
        return 1;
    }
    return 0;
}

/*******************************************************************************
 * Function Name  : getVersion
 * Description    : Return the version number of the chip
                    Value at reset: 00100011, 0x23
 * Input          : NONE
 * Return         : version number
 *******************************************************************************/
byte PMICClass::getVersion()
{

    int DATA = 0;
    DATA = readRegister(PMIC_VERSION_REGISTER);
    return DATA;
}

int PMICClass::readRegister(byte address)
{
    _wire->beginTransmission(PMIC_ADDRESS);
    _wire->write(address);

    if (_wire->endTransmission(true) != 0)
    {
        return -1;
    }

    if (_wire->requestFrom(PMIC_ADDRESS, 1, true) != 1)
    {
        return -1;
    }

    return _wire->read();
}

int PMICClass::writeRegister(byte address, byte val)
{
    _wire->beginTransmission(PMIC_ADDRESS);
    _wire->write(address);
    _wire->write(val);

    if (_wire->endTransmission(true) != 0)
    {
        return 0;
    }

    return 1;
}

PMICClass PMIC(Wire);