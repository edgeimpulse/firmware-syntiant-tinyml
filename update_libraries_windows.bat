@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
setlocal
REM go to the folder where this bat script is located
cd /d %~dp0

set /a EXPECTED_CLI_MAJOR=0
set /a EXPECTED_CLI_MINOR=18

set NDP_CMD=ndp10x_flash\ndp10x_flash.exe
set BIN_FILE=ei_model.bin

FOR %%I IN (.) DO SET DIRECTORY_NAME=%%~nI%%~xI

where /q arduino-cli
IF ERRORLEVEL 1 (
    GOTO NOTINPATHERROR
)

REM parse arduino-cli version
FOR /F "tokens=1-3 delims==." %%I IN ('arduino-cli version') DO (
    FOR /F "tokens=1-3 delims== " %%X IN ('echo %%I') DO (
        set /A CLI_MAJOR=%%Z
    )
    SET /A CLI_MINOR=%%J
    FOR /F "tokens=1-3 delims== " %%X IN ('echo %%K') DO (
        set /A CLI_REV=%%X
    )
)

if !CLI_MINOR! LSS !EXPECTED_CLI_MINOR! (
    GOTO UPGRADECLI
)

if !CLI_MAJOR! NEQ !EXPECTED_CLI_MAJOR! (
    echo You're using an untested version of Arduino CLI, this might cause issues (found: %CLI_MAJOR%.%CLI_MINOR%.%CLI_REV%, expected: %EXPECTED_CLI_MAJOR%.%EXPECTED_CLI_MINOR%.x )
) else (
    if !CLI_MINOR! NEQ !EXPECTED_CLI_MINOR! (
        echo You're using an untested version of Arduino CLI, this might cause issues (found: %CLI_MAJOR%.%CLI_MINOR%.%CLI_REV%, expected: %EXPECTED_CLI_MAJOR%.%EXPECTED_CLI_MINOR%.x )
    )
)

echo Finding Arduino SAMD core v1.8.9...

(arduino-cli core list  2> nul) | findstr /r "arduino:samd.*1.8.9"
IF %ERRORLEVEL% NEQ 0 (
    GOTO INSTALLSAMDCORE
)

:AFTERINSTALLSAMDCORE

echo Finding Arduino SAMD core OK

(arduino-cli lib list avdweb_SAMDtimer 2> nul) | findstr "avdweb_SAMDtimer.1.0.0"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing SAMDtimer library...
    arduino-cli lib update-index
    arduino-cli lib install "avdweb_SAMDtimer"@1.0.0
    echo Installing SAMDtimer library OK
)

(arduino-cli lib list Adafruit_ZeroTimer_Library 2> nul) | findstr "Adafruit_ZeroTimer_Library.1.0.1"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing ZeroTimer library...
    arduino-cli lib update-index
    arduino-cli lib install "Adafruit ZeroTimer Library"@1.0.1
    echo Installing ZeroTimer library OK
)

(arduino-cli lib list Adafruit_BusIO 2> nul) | findstr "Adafruit_BusIO.1.8.2"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing BusIO library...
    arduino-cli lib update-index
    arduino-cli lib install "Adafruit BusIO"@1.8.2
    echo Installing BusIO library OK
)

(arduino-cli lib list Adafruit_GFX_Library 2> nul) | findstr "Adafruit_GFX_Library.1.10.10"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing GFX library...
    arduino-cli lib update-index
    arduino-cli lib install "Adafruit GFX Library"@1.10.10
    echo Installing GFX library OK
)

(arduino-cli lib list Adafruit_SSD1306 2> nul) | findstr "Adafruit_SSD1306.2.4.6"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing Adafruit SSD1306 library...
    arduino-cli lib update-index
    arduino-cli lib install "Adafruit SSD1306"@2.4.6
    echo Installing Adafruit SSD1306 library OK
)

(arduino-cli lib list SdFat 2> nul) | findstr "SdFat.2.0.6"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing SdFat library...
    arduino-cli lib update-index
    arduino-cli lib install "SdFat"@2.0.6
    echo Installing SdFat library OK
)

(arduino-cli lib list HID-Project 2> nul) | findstr "HID-Project.2.6.1"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing HID-Project library...
    arduino-cli lib update-index
    arduino-cli lib install "HID-Project"@2.6.1
    echo Installing HID-Project library OK
)

set OUTPUT=""
for /f "delims=" %%i in ('arduino-cli config dump ^| findstr /r "user: "') do (
    set OUTPUT=%%i
)
IF "%OUTPUT%" == "" GOTO LIBRARYDIRNOTFOUND

set library=%OUTPUT:~8%\libraries

(arduino-cli lib list Adafruit_ASFcore 2> nul) | findstr "Adafruit_ASFcore"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing Adafruit_ASFcore library...
    mkdir "%library%\Adafruit_ASFcore"
    xcopy "lib\Adafruit_ASFcore" "%library%\Adafruit_ASFcore" /E /V /Y
    echo Installing Adafruit_ASFcore library OK
)

(arduino-cli lib list AudioUSB 2> nul) | findstr /r "AudioUSB.0.0.1"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing AudioUSB library...
    mkdir "%library%\AudioUSB"
    xcopy "lib\AudioUSB" "%library%\AudioUSB" /E /V /Y
    echo Installing AudioUSB library OK
)

(arduino-cli lib list SerialFlash 2> nul) | findstr /r "SerialFlash.0.5"
IF %ERRORLEVEL% NEQ 0 (
    echo Installing SerialFlash library...
    mkdir "%library%\SerialFlash"
    xcopy "lib\SerialFlash" "%library%\SerialFlash" /E /V /Y
    echo Installing SerialFlash library OK
)

(arduino-cli lib list NDP 2> nul) | findstr /r "1.0.0"
IF %ERRORLEVEL% NEQ 0 (
    arduino-cli lib uninstall NDP
    echo Installing NDP library...
    mkdir "%library%\NDP"
    xcopy "lib\NDP" "%library%\NDP" /E /V /Y
    echo Installing NDP library OK
)

(arduino-cli lib list NDP_utils 2> nul) | findstr /r "1.0.1"
IF %ERRORLEVEL% NEQ 0 (
    arduino-cli lib uninstall NDP_utils
    echo Installing NDP_utils library...
    mkdir "%library%\NDP_utils"
    xcopy "lib\NDP_utils" "%library%\NDP_utils" /E /V /Y
    echo Installing NDP_utils library OK
)

(arduino-cli lib list PMIC_SGM41512 2> nul) | findstr /r "1.0.0"
IF %ERRORLEVEL% NEQ 0 (
    arduino-cli lib uninstall PMIC_SGM41512
    echo Installing PMIC_SGM41512 library...
    mkdir "%library%\PMIC_SGM41512"
    xcopy "lib\PMIC_SGM41512" "%library%\PMIC_SGM41512" /E /V /Y
    echo Installing PMIC_SGM41512 library OK
)

for /f "delims=" %%i in ('arduino-cli config dump ^| findstr /r "data: "') do (
    set OUTPUT=%%i
)
IF "%OUTPUT%" == "" GOTO LIBRARYDIRNOTFOUND

set datadir=%OUTPUT:~8%\packages\arduino\hardware\samd\1.8.9\cores\arduino\USB\


fc "lib\Arduino USBCore driver\USBCore.cpp" "%datadir%\USBCore.cpp" > nul
IF %ERRORLEVEL% NEQ 0 (
    echo Updating USBCore.cpp...
    xcopy "lib\Arduino USBCore driver\USBCore.cpp" "%datadir%" /V /Y
    echo Updating USBCore.cpp OK
)

fc "lib\syntiant_ilib\VERSION" "%library%\syntiant_ilib\VERSION" > nul
IF %ERRORLEVEL% NEQ 0 (
    echo Installing Syntiant ilib library...
    mkdir "%library%\syntiant_ilib"
    xcopy "lib\syntiant_ilib" "%library%\syntiant_ilib" /E /V /Y
    echo Installing Syntiant ilib library OK
)

echo Installing of libraries completed ...
@pause
exit /b 0

:NOTINPATHERROR
echo Cannot find 'arduino-cli' in your PATH. Install the Arduino CLI before you continue
echo Installation instructions: https://arduino.github.io/arduino-cli/latest/
@pause
exit /b 1

:INSTALLSAMDCORE
echo Installing Arduino SAMD core...
arduino-cli core update-index
arduino-cli core install arduino:samd@1.8.9
echo Installing Arduino SAMD core OK
GOTO AFTERINSTALLSAMDCORE

:UPGRADECLI
echo You need to upgrade your Arduino CLI version (now: %CLI_MAJOR%.%CLI_MINOR%.%CLI_REV%, but required: %EXPECTED_CLI_MAJOR%.%EXPECTED_CLI_MINOR%.x or higher)
echo See https://arduino.github.io/arduino-cli/installation/ for upgrade instructions
@pause
exit /b 1

:LIBRARYDIRNOTFOUND
echo Arduino library directory not found. 
@pause
exit /b 1
