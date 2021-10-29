/*
  PMIC fault Check Example

  This example shows how to check the PMIC fault states

  Circuit:
  - Arduino MKR board

  created 21 Aug 2019
  by Riccardo Rizzo

  This sample code is part of the public domain.
*/

#include <Arduino_PMIC.h>

int chargefault = NO_CHARGE_FAULT;
int batTempfault = NO_TEMPERATURE_FAULT;

void setup() {
  if (!PMIC.begin()) {
    Serial.println("Failed to initialize PMIC!");
    while (1);
  }
}

void loop() {
  // getChargeFault() returns the charge fault state, the fault could be:
  //   - Thermal shutdown: occurs if internal junction temperature exceeds
  //     the preset limit;
  //   - input over voltage: occurs if VBUS voltage exceeds 18 V;
  //   - charge safety timer expiration: occurs if the charge timer expires.
  chargefault = PMIC.getChargeFault();
  // getChargeFault() returns charge fault status
  switch (chargefault) {
    case INPUT_OVER_VOLTAGE: Serial.println("Input over voltage fault occurred");
      break;
    case THERMAL_SHUTDOWN: Serial.println("Thermal shutdown occurred");
      break;
    case CHARGE_SAFETY_TIME_EXPIRED: Serial.println("Charge safetly timer expired");
      break;
    case NO_CHARGE_FAULT: Serial.println("No Charge fault");
      break;
    default : break;
  }

  // The isBatteryInOverVoltage() returns if battery over-voltage fault occurs.
  // When battery over voltage occurs, the charger device immediately disables
  // charge and sets the battery fault bit, in fault register, to high.
  if (PMIC.isBatteryInOverVoltage()) {
    Serial.println("Error: battery over voltage fault");
  }


  batTempfault = PMIC.hasBatteryTemperatureFault();
  switch (batTempfault) {
    case NO_TEMPERATURE_FAULT: Serial.println("No temperature fault");
      break;
    case LOWER_THRESHOLD_TEMPERATURE_FAULT: Serial.println("Lower threshold Battery temperature fault");
      break;
    case HIGHER_THRESHOLD_TEMPERATURE_FAULT: Serial.println("Higher threshold Battery temperature fault");
      break;
    default: break;
  }
}