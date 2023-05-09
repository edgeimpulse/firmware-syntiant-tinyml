@echo off

set PROJECT=firmware-syntiant-tinyml
set BOARD=arduino:samd:mkrzero
set ARDUINO_CLI=arduino-cli
set BUILD_OPTION=--build
set FLASH_OPTION=--flash
set ALL_OPTION=--all

set SENSOR_WITH_IMU=--with-imu
:: use --with-imu flag as 2nd argument to build firmware for IMU
set COMMAND=%1
set SENSOR=%2

set /A EXPECTED_CLI_MAJOR=0
set /A EXPECTED_CLI_MINOR=18

where /q %ARDUINO_CLI%
IF ERRORLEVEL 1 (
    ECHO Cannot find 'arduino-cli' in your PATH. Install the Arduino CLI before you continue.
    ECHO Installation instructions: https://arduino.github.io/arduino-cli/latest/
    EXIT /B
)

:: check if compile with imu
IF "%SENSOR%" == "--with-imu" (
    set CPP_FLAGS=-DWITH_IMU
) ELSE (
    set CPP_FLAGS=
)

:: define and include
set DEFINE=-DEI_SENSOR_AQ_STREAM=FILE -DUSE_ARDUINO_MKR_PIN_LAYOUT -D__SAMD21G18A__ -DUSB_VID=0x2341 -DUSB_PID=0x804f -DUSBCON -DUSB_MANUFACTURER=\"Syntiant\" -DUSB_PRODUCT=\"TinyML\"
set INCLUDE=-I.\\src\\  -I\\ingestion-sdk-platform\\repl\\ -I.\\src\\ingestion-sdk-platform\\syntiant\\ -I.\\src\\sensors\\ -I.\\src\\ingestion-sdk-c\\ -I.\\src\\QCBOR\\inc\\ -I.\\src\\sensor_aq_mbedtls\\

:: just build
IF %COMMAND% == %BUILD_OPTION% goto :BUILD

:: look for connected board 
goto :FIND_COM_PORT
:COMM_FOUND

IF %COMMAND% == %FLASH_OPTION% goto :FLASH

IF %COMMAND% == %ALL_OPTION% goto :ALL else goto :COMMON_EXIT

echo No valid command

goto :COMMON_EXIT

:BUILD
    echo Building %PROJECT%
    echo %ARDUINO_CLI% compile --fqbn %BOARD% --build-property "build.extra_flags=%DEFINE% %INCLUDE%"  --build-property "compiler.cpp.extra_flags=%CPP_FLAGS%" %PROJECT% --output-dir .
    %ARDUINO_CLI% compile --fqbn %BOARD% --build-property "build.extra_flags=%DEFINE% %INCLUDE%" --build-property "compiler.cpp.extra_flags=%CPP_FLAGS%" %PROJECT% --output-dir .
goto :COMMON_EXIT

:FLASH
    echo Flashing %PROJECT%
    echo %ARDUINO_CLI% upload -p %found_com% --fqbn %BOARD%
    %ARDUINO_CLI% upload -p %found_com% --fqbn %BOARD%
goto :COMMON_EXIT

:ALL
    echo Building %PROJECT%
    echo %ARDUINO_CLI% compile --fqbn %BOARD% --build-property "build.extra_flags=%DEFINE% %INCLUDE%" %PROJECT% --output-dir .
    %ARDUINO_CLI% compile --fqbn %BOARD% --build-property "build.extra_flags=%DEFINE% %INCLUDE%" %PROJECT% --output-dir .
    echo Flashing %PROJECT%
    echo %ARDUINO_CLI% upload -p %found_com% --fqbn %BOARD% --input-dir .
    %ARDUINO_CLI% upload -p %found_com% --fqbn %BOARD% --input-dir .
goto :COMMON_EXIT

:: check for COM port and if they are listed as Arduino
:FIND_COM_PORT
@setlocal enableextensions enabledelayedexpansion

:: wmic /format:list strips trailing spaces (at least for path win32_pnpentity)
for /f "tokens=1* delims==" %%I in ('wmic path win32_pnpentity get caption /format:list ^| find "Arduino"') do (
    call :setCOM "%%~J"
)

:: Check if any returned value 
if "%found_com%"=="" ( 
    echo No valid COM found
    goto :COMMON_EXIT
)
goto :COMM_FOUND

:setCOM <WMIC_output_line>

setlocal

set "str=%~1"
set "test=%~1"

set "num=%str:*(COM=%"
set "num=%num:)=%"
set str=%str:(COM=&rem.%

endlocal & set found_com=COM%num%
goto :EOF

:COMMON_EXIT
