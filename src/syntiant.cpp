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

// Uncomment next line to compile IMU firmware using Arduino IDE
// #define WITH_IMU

#include <inttypes.h>
#include <stddef.h>

#ifndef WITH_IMU
#define WITH_AUDIO
#endif

#ifdef WITH_AUDIO
#include "AudioUSB.h"
#endif

#include <SerialFlash.h>
#include <SPI.h>

#include <NDP.h>
#include <NDP_utils.h>

#include <HID-Project.h>

#include "avdweb_SAMDtimer.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Arduino.h>
#include "wiring_private.h" // pinPeripheral() function

#include "syntiant.h"
#include "../syntiant_arduino_version.h"
#include "ingestion-sdk-platform/syntiant/ei_device_syntiant_samd.h"
#include "repl/repl.h"

/* Constant defines -------------------------------------------------------- */
#define CONVERT_G_TO_MS2    9.80665f

/* Extern declared --------------------------------------------------------- */
extern void ei_setup(void);
extern void ei_classification_output(int matched_feature);

#if defined(WITH_IMU)
String model = "ei_model_sensor.bin";
#else
String model = "ei_model.bin";
#endif

Adafruit_ZeroTimer zt3 = Adafruit_ZeroTimer(3);

// Management USB interface commands. Always preceded by ":"
const byte INDIRECT_READ = 0x00;
const byte INDIRECT_WRITE = 0x01;
const byte DIRECT_READ = 0x02;
const byte DIRECT_WRITE = 0x03;
const byte SAMD_CLK = 0x04;
const byte SAMD_PDM_CLK = 0x05;
const byte SAMD_ACK = 0x06;
const byte SAMD_NACK = 0x07;
const byte SAMD_LED = 0x08;
const byte SET_SPI_DEFAULT_SPEED = 0x09;
const byte SET_SPI_SAMPLE_SPEED = 0x0a;
const byte RX_FLASH_BUFFER = 0x10;
const byte TX_FLASH_BUFFER = 0x11;
const byte LOAD_BIN_FILE = 0x12;
const byte START_OF_FILE = 0x13;
const byte I2C_READ = 0x14;
const byte I2C_WRITE = 0x15;

const byte GET_INT_COUNT = 0xf0;

// console commands
const byte GET_INFO = ':';
const byte SAMD_RESET = '$';
const byte NDP_RESET = '@';
const byte COPY_SD_TO_FLASH = 'F';
const byte ERASE_SD_CARD = 'e';
const byte FAT_FORMAT_SD_CARD = 'f';
const byte PMIC_MODE_CHANGE = 'p';

// Sample Tank addresses
// includes tank size[17] from bits 4 - 21
const uint32_t DSP_CONFIG_TANK = 0x4000c0a8;
const uint32_t DSP_CONFIG_FREQSTS0 = 0x4000c0ac;
const uint32_t DSP_CONFIG_TANKADDR = 0x4000c0b0;
const uint32_t DSP_CONFIG_TANKSTS0 = 0x4000c0b4;
const uint32_t DSP_CONFIG_TANKSTS1 = 0x4000c0b8;

const byte NDP9101_USB_IDLE = 0xC0; // When USB is diconnected, the USN n/p lines
                                    // are pulled high. This indicated the device is
                                    // removed from a host
const byte TINYML_USB_IDLE = 0x40;  // When USB is diconnected, the USN n/p lines

byte PORSTB = 24;         // TinyML Final Board NDP RESET - active low

byte idle = TINYML_USB_IDLE; // this indicates the USB idle code for NDP9101 or TinyML

uint32_t fileLength = 0; // File Length
uint32_t checkSum = 0;   // File Check Sum

byte loadedFromSD = 0;          // indicates successful boot from SD Card
byte loadedFromSerialFlash = 0; // indicates successful boot from Serial Flash

uint16_t usbConnected = 0; // counter for USB in the disconnected state

// HID is used to propagate ndp interrupts to usb host
uint8_t hidBuf[64] __attribute__((aligned(4)));

volatile int intCount = 0;

// counter to control "time on" for the led
// if set to value > 0xff, the led will be kept on indefinitely
int ledTimerCount = 0;

#define timer_in_uS 1000

// isr timer to control LED.
void isrTimer4(struct tc_module *const module_inst);
SAMDtimer timer4 = SAMDtimer(4, isrTimer4, timer_in_uS); // In uS. Interrupt every 1ms
int timer4TimedOut = 0;                                  // indicates LED timer timed out

Uart Serial2(&sercom3, 7, 6, SERCOM_RX_PAD_3, UART_TX_PAD_2);

// 2nd I2C interface using D0 SDA, D1 SCL, Sercom 5
TwoWire myWire(&sercom5, 0, 1);

byte doInt = 0; // flag indicating an interrupt from NDP

// Flash Type read from Serial Flash JEDEC register. From SerialFlash.h
byte FlashType[4];

// flag to indicate USB port has been detached during battery operation
byte detached = 0;

uint16_t USBConnected = 0; // counter for USB in the disconnected state

byte SD_or_SerialFlash = 0; // used when programming SD or Serial Flash devices

int match = 0; // indicated which class has matched

int doingMgmtCmd = 0;

// board type
#define NDP9101_CS 6
#define TINYML_CS 18 // Arduino pin A3

byte SPI_CS = TINYML_CS;
File myFile;
SerialFlashFile mySerialFlashFile;

#ifdef WITH_AUDIO
int16_t audioBuf[32]; // Audio Buffer
#endif
uint32_t tankAddress = 0;
uint32_t tankSize = 0;

uint32_t currentPointer = 0;
static uint32_t prevPointer = 0;

#ifdef WITH_IMU
const int dataLengthToBeSaved = 72;
static int16_t dataBuf[dataLengthToBeSaved];
#endif
static int16_t imu[36];
static bool imu_active = false;

static uint32_t startingFWAddress;

byte LastUserSwitch = 1;
uint32_t SAVE_REG_SYSCTRL_DFLLCTRL = 0xA46;
uint32_t SAVE_REG_SYSCTRL_DFLLMUL = 0x7DFF05B9;

void SERCOM3_Handler()
{
    Serial2.IrqHandler();
}

// blocking read from the USB serial port
int readByte(void)
{
    char b;
    int bs;

    bs = Serial.readBytes(&b, 1);
    return bs == 0 ? -1 : b;
}

int readMultipleBytes(int size, uint32_t *_v)
{
    int i, b;
    int v = 0;

    for (i = 0; i < size; i++)
    {
        b = readByte();
        if (b < 0)
        {
            return b;
        }
        v = (v << 8) | b;
    }

    *_v = v;
    return size;
}

// The SAMD USB uart seems not to always send data without error, and
// seems to lose when sending > 64 bytes at a time, so use this instead
// in place of a plain Serial.write
void writeBytes(uint8_t *data, int count)
{
    int sent = 0;
    size_t s;
    int left;

    Serial.flush();
    while (sent < count)
    {
        left = count - sent;
        s = Serial.write(data + sent, left < 64 ? left : 64);
        if (s != 0xffffffff)
        {
            sent += s;
        }
        Serial.flush();
    }
}
int ints = 0;
int old_int = 0;
// INT pin interrupt from NDP. Simply flag form main() routine to process
void ndpInt()
{
    SCB->SCR &= !SCB_SCR_SLEEPDEEP_Msk; // Don't Allow Deep Sleep
    doInt = 1;
    ledTimerCount = 1 * (1000000 / timer_in_uS); // flash LED for 1 second

    if (!doingMgmtCmd)
    {
        timer4.enableInterrupt(true);
    }

    ints++;
    // Serial.print(ints);
}

void syntiant_get_imu(float *dest_imu)
{
    while(imu_active){};

    imu_active = true;

    for (int i = 0; i < 36; i++) {
        dest_imu[i] = (float)(imu[i] * 2 / 32768.f) * CONVERT_G_TO_MS2;
    }

    imu_active = false;
}

// Timer 4 interrupt. Handles ALL touches of NDP. Also services USB Audio
void isrTimer4(struct tc_module *const module_inst)
{
    digitalWrite(0, LOW);
    int s;
    unsigned int len;

    SCB->SCR &= !SCB_SCR_SLEEPDEEP_Msk; // Don't Allow Deep Sleep
    if ((ledTimerCount < 0xffff) && (ledTimerCount > 0))
    {
        ledTimerCount--;
        if (ledTimerCount == 0)
        {
            timer4TimedOut = 1;
        }
    }


#ifdef WITH_AUDIO
    if (runningFromFlash) {

        uint32_t tankRead;
        int i;

        for (i = 0; i < 32; i += 4)
        {
            tankRead =
                indirectRead(tankAddress + ((currentPointer - 32 + i) % tankSize));
            audioBuf[i / 2] = tankRead & 0xffff;
            audioBuf[(i / 2) + 1] = (tankRead >> 16) & 0xffff;
        }
        currentPointer += 32;
        currentPointer %= tankSize;
    }
    AudioUSB.write(audioBuf, 32); // write samples to AudioUSB
#else

    if(runningFromFlash) {
        currentPointer = indirectRead(startingFWAddress);

        int32_t diffPointer = ((int32_t)currentPointer - prevPointer);

        if(diffPointer < 0) {
            diffPointer = (64000 - prevPointer) + currentPointer;
        }
        if(diffPointer >= dataLengthToBeSaved) {
            len = sizeof(dataBuf);
            int ret = NDP.extractData((uint8_t *)dataBuf, &len);

            if(ret != SYNTIANT_NDP_ERROR_NONE) {
                ei_printf("Extracting data failed with error : %d\r\n", ret);
            }
            else {
                if(imu_active == false) {
                    imu_active = true;
                    for(int i = 0; i < (dataLengthToBeSaved / 2); i++) {
                        imu[i] = dataBuf[i];
                    }

                    imu_active = false;
                }
            }

            prevPointer = currentPointer;
        }
    }
#endif

    if (doInt)
    {
        doInt = 0;
        // Poll NDP for cause of interrupt (if running from flash)
        if (runningFromFlash)
        {
            match = NDP.poll();

            if (match)
            {
                // Light Arduino LED
                digitalWrite(LED_BUILTIN, HIGH);

                ei_classification_output(match -1);

                printBattery(); // Print current battery level

            }
        }
        else
        {
            match = 1;
        }
    }
    digitalWrite(0, HIGH);
}

// Arduino System Setup routine
// Initialises devices. Reads flash device to see if valid uilib has
// been programmed.
// Loads uilib if present
void syntiant_setup(void)
{
    timer4.enable(false); //disable timer4

    // uilib variables
    int s;
    uint32_t v;
    byte i;
    char name[9];
    char *namep = name;

    // Initialize Serial Port
    Serial.begin(115200);
    Serial2.begin(115200);
    //delay(3000); // Enable serial ports to print to console

    // Show sign of life
    pinMode(LED_BUILTIN, OUTPUT); // RED LED

    digitalWrite(LED_BUILTIN, HIGH); // Light RED LED
    //digitalWrite(LED_BLUE, HIGH); // Light BLUE LED
    //digitalWrite(LED_GREEN, HIGH); // Light GREEN LED

    SerialFlash.begin(FLASH_CS);
    SerialFlash.readID(FlashType);
    if(FlashType[0] == SST25VF016B) { // Set up clock for Bluebank board
        analogWrite(3, 0x10); //TinyML Final Board
        REG_TCC1_PER = 1464;
        while (TCC1->SYNCBUSY.bit.PER)
            ;
        REG_TCC1_CC1 = 732;
        while (TCC1->SYNCBUSY.bit.CC1)
            ;
        PORSTB = 24;
    } else if(FlashType[0] == MX25R6435FSN) { // Set up clock for Tessolve board
        // Set up 32KHz NDP clock
        /********************* Timer #3, 16 bit, toggles pin PA18 */
        zt3.configure(TC_CLOCK_PRESCALER_DIV1,      // prescaler
                      TC_COUNTER_SIZE_16BIT,        // bit width of timer/counter
                      TC_WAVE_GENERATION_MATCH_FREQ // frequency or PWM mode
        );
        zt3.PWMout(true, 0, 10);                       // Actually toggles pin PA18
        zt3.setCompare(0, (48000000 / 32000 / 2) - 1); // 32KHz output
        zt3.enable(true);
        PORSTB = 3;
    } else {
        Serial2.print("Unknown board: ");
        Serial2.println(FlashType[0], HEX);
        Serial.print("Board not recognized. Press your board's reset button to exit");
        while(true)
            ;
    }

    // reset NDP
    pinMode(PORSTB, OUTPUT);
    digitalWrite(PORSTB, LOW);
    delay(100);
    digitalWrite(PORSTB, HIGH);

    // Set up SPI (NDP) & SPI1 (SD card)
    SPI.begin();
    SPI.beginTransaction(SPISettings(spiSpeedGeneral, MSBFIRST, SPI_MODE0));

    // See which board we are by trying to read NDP101 Registion Register
    pinMode(NDP9101_CS, OUTPUT);
    digitalWrite(NDP9101_CS, HIGH);
    pinMode(TINYML_CS, OUTPUT);
    digitalWrite(TINYML_CS, HIGH);

    SPI_CS = NDP9101_CS;
    NDP.spiTransfer(NULL, 0, 0x0, NULL, spiData, 1);

    if (spiData[0] == 0x20)
    {
        SPI_CS = NDP9101_CS;
        idle = NDP9101_USB_IDLE;
    }

    else
    {
        SPI_CS = TINYML_CS;
        NDP.spiTransfer(NULL, 0, 0x0, NULL, spiData, 1);

        if (spiData[0] == 0x20)
        {
            SPI_CS = TINYML_CS;
            pinMode(NDP9101_CS, INPUT); // make NDP9101_CS input to save power

            Serial2.begin(115200);

            // Serial2 will be available on TinyML connector pin 6 (RX) & pin 7 (TX)
            // PA20 Arduino pin 6 is RX.
            // PA21 Arduino pin 7 is TX.
            // Assign pins PA20 & PA21 to SERCOM functionality.
            pinPeripheral(6, PIO_SERCOM_ALT);
            pinPeripheral(7, PIO_SERCOM_ALT);
            delay(100);
            Serial2.println("Hello Serial2 World!");

            pinMode(LED_BLUE, OUTPUT);
            pinMode(LED_GREEN, OUTPUT);
            pinMode(USER_SWITCH, INPUT_PULLUP);

            // find which Serial Flash device is connected
            SerialFlash.begin(FLASH_CS);
            SerialFlash.readID(FlashType);
            // Set up pin to drive 5v out to Arduino companion
            if (FlashType[0] == SST25VF016B)
            {
                digitalWrite(ENABLE_5V, LOW); // disable 5v output Bluebank Board
                Serial2.println("Syntiant TinyML Board B");
            }
            if (FlashType[0] == MX25R6435FSN)
            {
                digitalWrite(ENABLE_5V, HIGH); // disable 5v output Tesolve Boards
                Serial2.println("Syntiant TinyML Board T");
            }

            pinMode(PMU_OTG, OUTPUT);   // set up OTG/PMU current pin
            pinMode(ENABLE_5V, OUTPUT); // Set up 5v gate

            // Initialise SGM41512
            if (!PMIC.begin()) {
                Serial2.println("Failed to initialize PMIC!");
            }
            pmuCharge(); // Enable PMU Charge mode

            pinMode(0, OUTPUT);
            pinMode(1, OUTPUT);
            idle = TINYML_USB_IDLE;
        }

        else
        {
            Serial.println("No NDP device found");
            while (true)
                ;
        }
    }

    // Initialize SD & Serial Flash. Try & load NDP BIN file which contains NDP firmware & Neural Network
    // If not able to load bin file, use Bridging Mode to access NDP
    NDP.setInterrupt(NDP_INT, ndpInt);
    switch (loadModel(model))
    {
    case BIN_LOAD_OK:
        ei_printf("BIN file loaded correctly from SD Card");
        runningFromFlash = 1;
        digitalWrite(LED_BLUE, HIGH); // Light BLUE LED as uilib load successful
        loadedFromSD = 1;
        loadedFromSerialFlash = 0;
        break;
    case NO_SD:
        ei_printf("No SD Card inserted, please insert card");
        ei_printf("Running in Bridge Mode");
        break;
    case SD_NOT_INITIALIZED:
        ei_printf("SD Card initialization failed!");
        ei_printf("Running in Bridge Mode");
        break;
    case BIN_NOT_OPENED:
        // ei_printf(model + " NOT opened. Make sure you're using the correct BIN file name.");
        ei_printf("Running in Bridge Mode");
        break;
    case ERROR_LOADING_FLASH:
        ei_printf("Error loading bin from Flash!");
        ei_printf("Running in Bridge Mode");
        break;
    case ERROR_LOADING_SD:
        ei_printf("Error loading bin from SD!");
        ei_printf("Running in Bridge Mode");
        break;
    case LOADED_FROM_SERIAL_FLASH:
        ei_printf("BIN File Loaded correctly from Serial Flash");
        runningFromFlash = 1;
        digitalWrite(LED_GREEN, HIGH); // Light GREEN LED as uilib load successful
        loadedFromSerialFlash = 1;
        loadedFromSD = 0;
        break;
    default:
        ei_printf("Running in Bridge Mode");
        break;
    }

    // Allow some peripherals to be active in Standby mode.
    // Standby is used when battery powered for lowest power
    TC3->COUNT16.CTRLA.bit.RUNSTDBY = 1; // enable timer3 in sleep mode. This generates the NDP clock
    SYSCTRL->XOSC32K.bit.RUNSTDBY = 1;   // Run the 32KHz Oscillator in sleep
    SYSCTRL->DFLLCTRL.bit.RUNSTDBY = 1;  // Run the Oscillator DFLL in sleep
    SYSCTRL->VREG.bit.RUNSTDBY = 1;      // Run the voltage regulator in sleep. This keeps NDP CLK at 32KHz

    // turn LED off
    digitalWrite(LED_BUILTIN, LOW);

    if (!runningFromFlash) {
        // Reset the NDP if the log load failed
        pinMode(PORSTB, OUTPUT);
        digitalWrite(PORSTB, LOW);
        delay(100);
        digitalWrite(PORSTB, HIGH);

        // Light RED LED as uilib NOT loaded successfully
        digitalWrite(LED_RED, HIGH);
    }

    // Set up timer to turn LEDs off after 1 second
    // ledTimerCount = 1000; // set LED timer for 1 second
    ledTimerCount = 1 * (1000000 / timer_in_uS); // set LED timer for 1 second

    // Set interrupt priority.
    // The priority specifies the interrupt priority value, whereby
    // lower values indicate a higher priority.
    // The default priority is 0 for every interrupt. This is the highest
    // possible priority.
    NVIC_SetPriority(TC4_IRQn, 3); // Make timer 4 the lowest priority

    tankSize = indirectRead(DSP_CONFIG_TANK) >> 4;
    tankAddress = indirectRead(DSP_CONFIG_TANKADDR);

#if defined(WITH_AUDIO)
    // Load Audio Buffer with test pattern
    for (i = 0; i < sizeof(audioBuf) / 2; i++) {
        audioBuf[i] = 4000 * (i - (sizeof(audioBuf) / 4));
    }
#endif


#if defined(WITH_AUDIO)
    AudioUSB.getShortName(namep); // needed for platformio to load AudioUSB
#endif


    //ENABLE_PDM = 25 is  declared in NDP_GPIO.h is the pin for controlling buffer SGM7SZ125. This buffer
    // will be activated (low) for voice command spotting and deactvated (high) for sensor appliactions

    pinMode(ENABLE_PDM, OUTPUT);
#if defined(WITH_IMU)
    digitalWrite(ENABLE_PDM, HIGH); // Disable PDM clock
    Serial.println("setup for IMU done");
#else
    digitalWrite(ENABLE_PDM, LOW); // Enable PDM clock
    Serial.println("setup for audio done");
#endif

    timer4.enable(true); // enable 1mS timer interrupt

    startingFWAddress = indirectRead(0x1fffc0c0);

    ei_setup();
}

// Management Interface Code
// We have received ":" from USB Serial host. Wait for command byte from Serial Port.
// This is the Host Management interface
void runManagementCommand(void)
{
    int command;
    int s;
    uint32_t addr;
    uint32_t count;
    uint32_t temp;

    doingMgmtCmd = 1;

    // Stop timer4 as we will access NDP from main()
    timer4.enableInterrupt(false);

    command = readByte();
    if (command < 0)
        return;

    switch (command)
    {
    case INDIRECT_READ:
        s = readMultipleBytes(4, &addr);
        if (s < 0)
            break;
        // host sends the address in LE format
        addr = reverseBytes(addr);

        s = readMultipleBytes(2, &count);
        if (s < 0)
            break;

        NDP.spiTransfer(NULL, 1, addr, NULL, spiData, count);
        writeBytes(spiData, count);
        break;

    case INDIRECT_WRITE:
        s = readMultipleBytes(4, &addr);
        if (s < 0)
            break;
        // host sends the address in LE format
        addr = reverseBytes(addr);

        s = readMultipleBytes(2, &count);
        if (s < 0)
            break;

        s = Serial.readBytes((char *)spiData, count);
        if (s < (int)count)
            break;

        NDP.spiTransfer(NULL, 1, addr, spiData, NULL, count);
        break;

    case DIRECT_READ:
        s = readMultipleBytes(1, &addr);
        if (s < 0)
            break;

        s = readMultipleBytes(2, &count);
        if (s < 0)
            break;

        NDP.spiTransfer(NULL, 0, addr, NULL, spiData, count);
        writeBytes(spiData, count);

        break;

    case DIRECT_WRITE:
        s = readMultipleBytes(1, &addr);
        if (s < 0)
            break;
        if (addr == 0x20)
        {
            SPI.beginTransaction(SPISettings(spiSpeedSampleWrite,
                                             MSBFIRST, SPI_MODE0));
        }

        s = readMultipleBytes(2, &count);
        if (s < 0)
            break;

        s = Serial.readBytes((char *)spiData, count);
        if (s < (int)count)
            break;
        NDP.spiTransfer(NULL, 0, addr, spiData, NULL, count);

        if (addr == 0x20)
        {
            SPI.beginTransaction(SPISettings(spiSpeedGeneral, MSBFIRST,
                                             SPI_MODE0));
        }
        if (addr == 0x4 && (spiData[0] & 0x01) == 0)
        {
            // chip reset -> no longer running from flash
            runningFromFlash = 0;
        }
        break;

    case SAMD_CLK:
        // EXTCLK is not currently used in the design, as we have NDP
        // generating the EXT clock from 32KHz
        break;

    case SAMD_PDM_CLK:
        // PDMCLK is not currently used in the design, as we have NDP generating
        // the PDM clock
        break;

    case SAMD_ACK:
        break;

    case SAMD_NACK:
        break;

    case SAMD_LED:
        // LED control
        // 0: turn off LED
        // 1: turn on LED
        // not currently implemented
        s = readMultipleBytes(4, &temp);
        break;

    case SET_SPI_DEFAULT_SPEED:
        s = readMultipleBytes(4, &temp);
        if (s < 0)
            break;

        if (temp)
        {
            spiSpeedGeneral = temp;
        }
        else
        {
            spiData[0] = (spiSpeedGeneral & 0xff000000) >> 24;
            spiData[1] = (spiSpeedGeneral & 0x00ff0000) >> 16;
            spiData[2] = (spiSpeedGeneral & 0x0000ff00) >> 8;
            spiData[3] = (spiSpeedGeneral & 0x000000ff) >> 0;
            writeBytes(spiData, 4);
        }
        break;

    case SET_SPI_SAMPLE_SPEED:
        s = readMultipleBytes(4, &temp);
        if (s < 0)
            break;

        if (temp)
        {
            spiSpeedSampleWrite = temp;
        }
        else
        {
            spiData[0] = (spiSpeedSampleWrite & 0xff000000) >> 24;
            spiData[1] = (spiSpeedSampleWrite & 0x00ff0000) >> 16;
            spiData[2] = (spiSpeedSampleWrite & 0x0000ff00) >> 8;
            spiData[3] = (spiSpeedSampleWrite & 0x000000ff) >> 0;
            writeBytes(spiData, 4);
        }
        break;

    case GET_INT_COUNT:
        spiData[0] = (intCount & 0xff000000) >> 24;
        spiData[1] = (intCount & 0x00ff0000) >> 16;
        spiData[2] = (intCount & 0x0000ff00) >> 8;
        spiData[3] = (intCount & 0x000000ff) >> 0;
        writeBytes(spiData, 4);
        break;

    case RX_FLASH_BUFFER:

        digitalWrite(LED_BUILTIN, HIGH);
        s = readMultipleBytes(4, &addr);
        if (s < 0)
            break;

        s = readMultipleBytes(2, &count);
        if (s < 0)
            break;

        s = Serial.readBytes((char *)ilibBuf, count);
        if (s < (int)count)
            break;
        if (SPI_CS == NDP9101_CS) // see if NDP9101B0-USB
        {
            flashWrite(addr, count);
            digitalWrite(LED_BUILTIN, LOW);
            break;
        }

        else // TinyML
        {
            if (addr == 0)
            {
                if (SD.begin(SDCARD_SS_PIN)) // check if SD card inserted
                {
                    // SD card inserted
                    myFile = SD.open(model, FILE_WRITE);
                    myFile.remove(); // delete old file
                    myFile = SD.open(model, FILE_WRITE);

                    myFile.rewind(); // go to start of file
                    Serial2.println("SD file opened");
                    SD_or_SerialFlash = 1;
                }
                else
                {
                    // Program Flash instead
                    Serial2.println("Program Serial Flash");
                    SerialFlash.remove(model.c_str());

                    // create the file on the Flash chip and copy data
                    SerialFlash.create(model.c_str(), fileLength);
                    Serial2.println("Created New Serial Flash File");
                    mySerialFlashFile = SerialFlash.open(model.c_str());
                    SD_or_SerialFlash = 0;
                }
            }
            if (fileLength < count)
                count = fileLength;
            if (SD_or_SerialFlash == 1)
                myFile.write((const void *)ilibBuf, count);
            else
                mySerialFlashFile.write((const void *)ilibBuf, count);

            fileLength -= count;
            if (fileLength == 0)
            {
                if (SD_or_SerialFlash == 1)
                {
                    myFile.close();
                    Serial2.println("New Bin File Programmed to SD card");
                }
                else
                {
                    mySerialFlashFile.close();
                    Serial2.println("New Bin File Programmed to Serial Flash");
                }

                fileLength = -1;
            }
            digitalWrite(LED_BUILTIN, LOW);
            break;
        }

    case TX_FLASH_BUFFER:
        // Send Flash Code for verification. Send "count" bytes per request.
        digitalWrite(LED_BUILTIN, HIGH);

        s = readMultipleBytes(4, &addr);
        if (s < 0)
            break;

        s = readMultipleBytes(2, &count);
        if (s < 0)
            break;

        if (SPI_CS == NDP9101_CS) // see if NDP9101B0-USB
        {
            changeMasterSpiMode(MSPI_IDLE);
            changeMasterSpiMode(MSPI_ENABLE);
            indirectWrite(CHIP_CONFIG_SPITX, FLASH_READ + addr);
            writeNumBytes(0x3);
            changeMasterSpiMode(MSPI_TRANSFER);
            spiWait();
            for (int i = 0; i < (int)count; i += 4)
            {
                changeMasterSpiMode(MSPI_UPDATE);
                changeMasterSpiMode(MSPI_TRANSFER);
                uint32_t rx_word = indirectRead(CHIP_CONFIG_SPIRX);
                ilibBuf[i + 0] = rx_word & 0xff;
                ilibBuf[i + 1] = (rx_word >> 8) & 0xff;
                ilibBuf[i + 2] = (rx_word >> 16) & 0xff;
                ilibBuf[i + 3] = (rx_word >> 24) & 0xff;
            }
            digitalWrite(LED_BUILTIN, LOW);

            writeBytes(ilibBuf, count);
        }
        else // TinyML
        {
            if (addr == 0)
            {
                if (SD.begin(SDCARD_SS_PIN)) // check if SD card inserted
                {
                    // Verify SD card
                    myFile = SD.open(model, FILE_READ);
                    SD_or_SerialFlash = 1;
                    Serial2.println("Opening SD file for verification");
                }
                else
                {
                    // Verify Serial Flash instead
                    mySerialFlashFile = SerialFlash.open(model.c_str());
                    SD_or_SerialFlash = 0;
                    Serial2.println("Opening Serial Flash file for verification");
                }
                fileLength = 0;
                checkSum = 0;
            }

            int read;

            if (SD_or_SerialFlash == 1)
                read = myFile.read((void *)ilibBuf, count);
            else
                read = mySerialFlashFile.read((void *)ilibBuf, count);
            {
                for (int i = 0; i < read; i++)
                {
                    checkSum += ilibBuf[i];
                }

                fileLength += read;
                for (int i = read; i < count; i++)
                {
                    ilibBuf[i] = 0xff;
                }
                if (addr == (0x100000 - 512))
                {
                    ilibBuf[511] = (MAGIC_NUMBER >> 24) & 0xff; // MAGIC_NUMBER = 0xA5B6C7D8;
                    ilibBuf[510] = (MAGIC_NUMBER >> 16) & 0xff;
                    ilibBuf[509] = (MAGIC_NUMBER >> 8) & 0xff;
                    ilibBuf[508] = (MAGIC_NUMBER >> 0) & 0xff;

                    ilibBuf[507] = (fileLength >> 24) & 0xff; // File Length
                    ilibBuf[506] = (fileLength >> 16) & 0xff;
                    ilibBuf[505] = (fileLength >> 8) & 0xff;
                    ilibBuf[504] = (fileLength >> 0) & 0xff;

                    ilibBuf[503] = (checkSum >> 24) & 0xff; // File Length
                    ilibBuf[502] = (checkSum >> 16) & 0xff;
                    ilibBuf[501] = (checkSum >> 8) & 0xff;
                    ilibBuf[500] = (checkSum >> 0) & 0xff;

                    Serial2.println("Old Header Spoofed & sent");

                    myFile.close();
                }
            }
            writeBytes(ilibBuf, count);
        }
        digitalWrite(LED_BUILTIN, LOW);
        break;

    case LOAD_BIN_FILE:
        s = readMultipleBytes(1, &fileLength);
        if (s < 0)
            break;

        loadUilibFlash((byte)addr);
        delay(1000);
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        break;

    case START_OF_FILE:
        s = readMultipleBytes(4, &fileLength); // read file length
        Serial2.print("New BIN being sent. Length = 0x");
        Serial2.println(fileLength, HEX);
        break;

    case GET_INFO:
        if (FlashType[0] == SST25VF016B)
        {
            Serial.print("Syntiant TinyML Board B. ");
        }
        if (FlashType[0] == MX25R6435FSN)
        {
            Serial.print("Syntiant TinyML Board T. ");
        }
        Serial.print(SYNTIANT_ARDUINO_VERSION " compiled " __DATE__ ", " __TIME__ ", uilib ");
        Serial.println(syntiant_ndp10x_micro_version);
        Serial.print("matches: ");
        Serial.print(intCount);
        Serial.print(", running from flash: ");
        Serial.print(runningFromFlash > 0 ? "yes" : "no");
        if (runningFromFlash)
        {
            Serial.print(", loaded from: ");
            Serial.println(loadedFromSD > 0 ? "SD" : "Serial Flash");
        }
        else
            Serial.println();
        Serial.print("patch applied: ");
        Serial.println(patchApplied > 0 ? "yes" : "no");
        // PMIC status
        if(PMIC.getOperationMode()) {
            Serial.println("PMIC is in boost mode");
        } else {
            Serial.println("PMIC is in charging mode");
            Serial.print("Charge Status = 0x");
            Serial.println(PMIC.chargeStatus(), HEX);
        }
        printBattery(); // Print current battery level
        break;

    case SAMD_RESET:
        NVIC_SystemReset();
        break;

    case NDP_RESET:
        digitalWrite(PORSTB, LOW);
        delay(100);
        digitalWrite(PORSTB, HIGH);
        runningFromFlash = 0;
        patchApplied = 0;
        break;

    case COPY_SD_TO_FLASH:
        Serial.println("Copying SD to flash");
        copySdToFlash();
        Serial.println("Done");
        break;

    case FAT_FORMAT_SD_CARD:
        fatFormatSd();
        break;

    case PMIC_MODE_CHANGE:
        if(PMIC.getOperationMode()) {
            pmuCharge(); // Enable PMU Charge mode
            if (FlashType[0] == SST25VF016B)
                digitalWrite(ENABLE_5V, LOW); // Disable 5v output
            Serial.print("Charge Status = 0x");
            Serial.println(PMIC.chargeStatus(), HEX);
        } else {
            pmuBoost(); // Enable PMU Boost mode
            // Set output to 5.08v
            if (FlashType[0] == SST25VF016B)
                digitalWrite(ENABLE_5V, HIGH); // Enable 5v output
        }
        printPmu();
        break;

    case ERASE_SD_CARD:
        eraseSd();
        break;

    case I2C_READ:
        uint32_t device;
        uint32_t reg;
        byte data[1];

        s = readMultipleBytes(1, &device); // Read I2C Device Address
        if (s < 0)
            break;

        s = readMultipleBytes(1, &reg); // READ I2C address
        if (s < 0)
            break;

        readFrom(device, reg, 1, data);

        writeBytes(data, 1);
        break;

    case I2C_WRITE:
        uint32_t dat;

        s = readMultipleBytes(1, &device); // Read I2C Device Address
        if (s < 0)
            break;

        s = readMultipleBytes(1, &reg); // READ I2C address
        if (s < 0)
            break;

        s = readMultipleBytes(1, &dat); // READ I2C address
        if (s < 0)
            break;

        writeTo(device, reg, dat);
        //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        break;

    default:
        break;
    }

    doingMgmtCmd = 0;
    timer4.enableInterrupt(true);
}

void processMatch(void)
{
    intCount++;
    match = 0;

    if (!runningFromFlash)
    {
        // HID PC Wake=up
        // System wake up functionality is not tested with the USB dongle
        // mpw      System.write(SYSTEM_WAKE_UP);
        hidBuf[0]++;
        hidBuf[1] = 0xff * digitalRead(NDP_INT);
        RawHID.write(hidBuf, sizeof(hidBuf));
    }
}

void syntiant_loop(void)
{
    // Loop to stay in Standby Mode unless we get a ": " from USB
    // OR interrupt from NDP
    while (1)
    {
        if (match)
        {
            processMatch();
        }
        if (timer4TimedOut)
        {
            timer4TimedOut = 0;
            // Turn Off Arduino LED
            digitalWrite(LED_RED, LOW);
            digitalWrite(LED_BLUE, LOW);
            digitalWrite(LED_GREEN, LOW);
        }
        // check USB serial port for ": " command from host
        // if (Serial.read() == ':')
        // {
        //     break;
        // }
        if(ei_command_line_handle() == true) {
            break;
        }

        // Deep sleep only if USB disconnected.
        SCB->SCR &= !SCB_SCR_SLEEPDEEP_Msk; // remove deep sleep bit

        if ((USB->DEVICE.STATUS.reg & 0xc0) != idle)
        {
            // we are connected to USB
            USBConnected = 0;
            timer4.enableInterrupt(true); // enable interrupt for Audio
            if (detached == 1)
            {
                detached = 0; // assume attached to host USB
                Serial2.println("Recommected to USB");
            }
        }
        else
            USBConnected += 1;

        if (USBConnected > 0xfff00)
        {
            USBConnected = 0;

            // Only deep sleep (Standby) if LED timer has expired.
            // See if LED timer = 0, & timed out flag not set
            if ((!ledTimerCount) && (!timer4TimedOut))
            {
                SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // enable Deep Sleep

                // This saves 0.63mA when battery powered. But then needs
                // keyword interrupt to wake CPU up
                USBDevice.detach();
                detached = 1;

                // stop timer as we are going to deep sleep
                timer4.enableInterrupt(false);

                // Put Flash into Deep Power Down
                digitalWrite(FLASH_CS, LOW);
                SPI1.transfer(FLASH_DP); // enable RESET
                digitalWrite(FLASH_CS, HIGH);
                delayMicroseconds(1);
                delay(1);

                delay(1000);
                Serial2.println("Deep Sleep");
                delay(2000);
            }
            else
            {
                timer4.enableInterrupt(true);
            }
        }

        digitalWrite(1, HIGH);

        __DSB(); // Data sync to ensure outgoing memory accesses complete
        __WFI();
        if (REG_SYSCTRL_DFLLMUL != SAVE_REG_SYSCTRL_DFLLMUL)
        {
            REG_SYSCTRL_DFLLMUL = SYSCTRL_DFLLMUL_MUL(0xBB80) | SYSCTRL_DFLLMUL_FSTEP(1) | SYSCTRL_DFLLMUL_CSTEP(1);
        }

        digitalWrite(1, LOW);

        // Woken up. See if attached to USB host
        if (detached)
        {
            USBDevice.attach();
            Serial.begin(115200);
            detached = 0;
        }
    }
    // Stop timer4 as we will access NDP from main()
    timer4.enableInterrupt(false); // disable 1mS timer interrupt

    // ':' received -- perform a management command
    runManagementCommand();

    timer4.enableInterrupt(true); // enable 1mS timer interrupt
}
