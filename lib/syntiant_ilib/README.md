<!---
# Copyright (c) 2021 Syntiant Corp.  All rights reserved.
# Contact at http://www.syntiant.com
# 
# This software is available to you under a choice of one of two licenses.
# You may choose to be licensed under the terms of the GNU General Public
# License (GPL) Version 2, available from the file LICENSE in the main
# directory of this source tree, or the OpenIB.org BSD license below.  Any
# code involving Linux software will require selection of the GNU General
# Public License (GPL) Version 2.
# 
# OPENIB.ORG BSD LICENSE
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
-->

# syntiant-uilib

This package contains the Syntiant NDP10x Micro Interface Library
(uILib).

The uILib is governed by a dual copyright license affixed to each of
the source files and in the prefix comment of this file.

The version number of the uILib is stored in the
`syntiant_ilib/syntiant_ndp_ilib_version.h` file and is also compiled
into the library object code itself to be retrieved through the
defined-interface for the library.

The uILib uses 'uILib log files' generated from the full Syntiant NDP
uILib to configure the NDP10x device with firmware.  The full Syntiant
NDP uILib and the capacity to generate new uILib firmware log files is
not included in the uILib distribution.  However, several example log
files are included to permit exercise of NDP10x devices with the uILib.

## The uILib

The library itself is in a single source file
`syntiant_ndp10x_micro.c` with the interface header file in
`syntiant_ilib/syntiant_ndp10x_micro.h`.

The supplied `Makefile` can build a statically-linked (`.a`) or
dynamically-linked (`.so`) library object for the uILib.
Alternatively, the `syntiant_ndp10x_micro.c` C file or the generated
`syntiant_ndp10x_micro.o` object file can simply be integrated into
the client system's build process.

## The `ndp10x_micro_app` Sample Program

The package also contains a sample program `ndp10x_micro_app.c`
intended to run on a Syntiant NDP9101B0-EVL evaluation system.  It can
be used as a reference for other clients.

The `uilib-logs` directory contains 3 uILib firmware log files to
allow exercise of an NDP10x device from the `ndp10x_micro_app`:

* alexa_spi.bin -- initializes the NDP10x to recognize the trigger
  word 'Alexa' with PCM audio input over the device SPI interface
* spi_to_pdm0.bin -- switches the audio input from the SPI interface
  to the pdm0 microphone
* pdm0_to_spi.bin -- switches the audio input from the pdm0 microphone
  to the SPI interface

The ILib CLI script used to create these firmware log files is also in
the `uilib-logs` directory named `ndp10x_micro_app_logs.txt`

The `ndp10x_micro_app` sample can be tested on a single channel,
16-bit, 16K samples/second PCM audio input file to show recognition on
the NDP device:
```
$ make ndp10x_micro_app
. . .
$ ./ndp10x_micro_app -l uilib-logs/alexa_spi.bin -P alexa.raw
Loading log from uilib-logs/alexa_spi.bin
butest passed
Polling for keyword indefinitely
2021-08-26 14:53:13 match -> alexa
$
```

This will watch for Alexa from the pdm0 microphone for 30 seconds recording
the PCM audio to `alexa_watch30.raw` and create a WAV format file
`alexa_watch30.wav` using the `sox` command:
```
$ ndp10x_micro_app -l alexa_spi.bin,spi_to_pdm0.bin -p alexa_watch30.raw -D 30
Loading log from alexa_spi.bin
Loading log from spi_to_pdm0.bin
butest passed
Watching for keyword for the next 30 seconds
2021-08-27 09:38:41 match -> alexa
2021-08-27 09:38:45 match -> alexa
2021-08-27 09:38:48 match -> alexa
$ sox -r 16k -b 16 -e signed-integer -c 1 alexa_watch30.raw alexa_watch30.wav
```
