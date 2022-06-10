## Config build and flash

This project uses the `arduino-cli` compiler to build & flash new firmware. Windows users also need Arduino IDE (tested with v1.8.15).


### Usage - macOS and Linux

The script will verify if all needed libraries and the samd core is installed and install them if needed. If you prefer to do this 
step manually, follow the step in the next chapter.

For building the project:

* For audio support, use:
```
./arduino-build.sh --build
```

* For IMU sensor support, use:
```
./arduino-build.sh --build --with-imu
```


For flashing use:

```
./arduino-build.sh --flash
```


You can also do both by using:
```
./arduino-build.sh --all [--with-imu]
```


### Usage - Windows

* Run `update_libraries_windows.bat` script to install Arduino libraries.

For building the project:

* For audio support, use:
```
./arduino-win-build.bat --build
```

* For IMU sensor support, use:
```
./arduino-win-build.bat --build --with-imu
```


For flashing use, put the board in boot mode (double click on the reset button when connecting the board to USB, while the orange LED is blinking. In boot mode you should see the red LED fading on and off):

```
./arduino-win-build.bat --flash
```


You can also do both by using:
```
./arduino-win-build.bat --all [--with-imu]
```

## Setup Manually

* Install Board package SAMD v1.8.9

* Install following libraries using Arduino Library manager (use exact versions):
    * avdweb_SAMDtimer@1.0.0
    * Adafruit ZeroTimer Library@1.0.1
    * Adafruit BusIO@1.8.2
    * Adafruit GFX Library@1.10.10
    * Adafruit SSD1306@2.4.6
    * nicohood/HID-Project@2.6.1
    * SdFat@2.0.6

* Install the following libraries as a Zip library:
    * lib/Adafruit_ASFcore
    * lib/PMIC_SGM41512
    * lib/AudioUSB
    * lib/NDP
    * lib/NDP_utils
    * lib/SerialFlash
    * lib/syntiant_ilib

* Patch the _Arduino USBCore driver_: copy `lib/Arduino USBCore driver/USBCore.cpp` in SAMD package folder (ie: /Users/[USER]/Library/Arduino15/packages/arduino/hardware/samd/1.8.9/cores/arduino/USB/)
