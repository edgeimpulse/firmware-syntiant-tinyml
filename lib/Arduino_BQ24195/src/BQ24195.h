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

#include "Wire.h"
#include "Arduino.h"

#ifndef _BQ24195_H_
#define _BQ24195_H_

// Charge Fault Status
#define NO_CHARGE_FAULT 0x00
#define INPUT_OVER_VOLTAGE 0x10
#define THERMAL_SHUTDOWN 0x20
#define CHARGE_SAFETY_TIME_EXPIRED 0x30

// Termal Fault Status
#define NO_TEMPERATURE_FAULT 0x00
#define LOWER_THRESHOLD_TEMPERATURE_FAULT 0x05
#define HIGHER_THRESHOLD_TEMPERATURE_FAULT 0x06

// Charging status
#define NOT_CHARGING 0x00
#define PRE_CHARGING 0x10
#define FAST_CHARGING 0x20
#define CHARGE_TERMINATION_DONE 0x30

// Voltage BUS status
#define UNKNOWN_MODE 0x00
#define USB_HOST_MODE 0x40
#define ADAPTER_PORT_MODE 0x80
#define BOOST_MODE 0xC0

class PMICClass {

  public:
    PMICClass(TwoWire  & wire);
    bool begin();
    void end();
    bool enableCharge();
    bool enableBoostMode();
    bool disableCharge();
    bool disableBoostMode();

    // Input source control register
    bool enableBuck(void);
    bool disableBuck(void);
    bool setInputCurrentLimit(float current);
    float getInputCurrentLimit(void);
    bool setInputVoltageLimit(float voltage);
    float getInputVoltageLimit(void);

    // Power ON configuration register
    bool resetWatchdog(void);
    bool setMinimumSystemVoltage(float voltage);
    float getMinimumSystemVoltage();

    // Charge current control register
    bool setChargeCurrent(float current);
    float getChargeCurrent(void);

    // PreCharge/Termination Current Control Register
    bool setPreChargeCurrent(float current);
    float getPreChargeCurrent();
    bool setTermChargeCurrent(float current);
    float getTermChargeCurrent();

    // Charge Voltage Control Register
    bool setChargeVoltage(float voltage);
    float getChargeVoltage();

    // Charge Timer Control Register
    bool disableWatchdog(void);

    // Misc Operation Control Register
    bool enableDPDM(void);
    bool disableDPDM(void);
    bool enableBATFET(void);
    bool disableBATFET(void);
    bool enableChargeFaultINT();
    bool disableChargeFaultINT();
    bool enableBatFaultINT();
    bool disableBatFaultINT();

    // System Status Register
    int USBmode();
    int chargeStatus();
    bool isBattConnected();
    bool isPowerGood(void);
    bool isHot(void);
    bool canRunOnBattery();

    // Fault Register
    bool isWatchdogExpired();
    int getChargeFault();
    bool isBatteryInOverVoltage();
    int hasBatteryTemperatureFault();

  protected:
    // Misc Operation Control Register
    bool setWatchdog(byte time);

    // Thermal Regulation Control Register
    bool setThermalRegulationTemperature(int degree);
    int getThermalRegulationTemperature();

    // Power ON configuration register
    bool enableCharging(void);
    bool disableCharging(void);
    bool enableOTG(void);
    bool disableOTG(void);

  private:
    byte getVersion();
    byte readOpControlRegister();
    byte readChargeTermRegister();
    byte readPowerONRegister(void);
    byte readInputSourceRegister(void);
    byte readSystemStatusRegister();
    byte readFaultRegister();

  private:
    TwoWire* _wire;
    int readRegister(byte asddress);
    int writeRegister(byte address, byte val);
};

extern PMICClass PMIC;

#endif
