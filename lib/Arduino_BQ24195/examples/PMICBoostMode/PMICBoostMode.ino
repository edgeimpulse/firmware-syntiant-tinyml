/*
  PMIC Boost mode Example

  This example shows how to enable boost mode on Arduino MKR boards

  Circuit:
  - Arduino MKR board
  - OTG cable

  created 21 Aug 2019
  by Riccardo Rizzo

  This sample code is part of the public domain.
*/

#include <Arduino_PMIC.h>

int usb_mode = UNKNOWM_MODE;

void setup()
{
  // Serial1 shall be used to print messages because the programming
  // Port is busy by the guest device
  Serial1.begin(9600);
  if (!PMIC.begin()) {
    Serial1.println("Failed to initialize PMIC!");
    while (1);
  }

  // Enable boost mode, this mode allow to use the board as host and
  // connect guest device like keyboard
  if (!PMIC.enableBoostMode()) {
    Serial1.println("Error enabling Boost Mode");
  }
  Serial1.println("Initialization Done!");
}

void loop() {
  int actual_mode = PMIC.USBmode();
  if (actual_mode != usb_mode) {
    usb_mode = actual_mode;
    if (actual_mode == BOOST_MODE) {
      // if the boost mode was correctly enabled 5V should appear on 5V pin
      // and on the USB connector
      Serial1.println("Boost mode status enabled");
    }
  }
}