#!/bin/bash

PROJECT=firmware-syntiant-tinyml
BOARD=arduino:samd:mkrzero
COMMAND=$1
# use --with-imu flag as 2nd argument to build firmware for IMU
SENSOR=$2

if [ -z "$ARDUINO_CLI" ]; then
	ARDUINO_CLI=$(which arduino-cli || true)
fi

DIRNAME="$(basename "$SCRIPTPATH")"
EXPECTED_CLI_MAJOR=0
EXPECTED_CLI_MINOR=18

if [ ! -x "$ARDUINO_CLI" ]; then
    echo "Cannot find 'arduino-cli' in your PATH. Install the Arduino CLI before you continue."
    echo "Installation instructions: https://arduino.github.io/arduino-cli/latest/"
    exit 1
fi

CLI_MAJOR=$($ARDUINO_CLI version | cut -d. -f1 | rev | cut -d ' '  -f1)
CLI_MINOR=$($ARDUINO_CLI version | cut -d. -f2)
CLI_REV=$($ARDUINO_CLI version | cut -d. -f3 | cut -d ' '  -f1)

if (( CLI_MINOR < EXPECTED_CLI_MINOR)); then
    echo "You need to upgrade your Arduino CLI version (now: $CLI_MAJOR.$CLI_MINOR.$CLI_REV, but required: $EXPECTED_CLI_MAJOR.$EXPECTED_CLI_MINOR.x or higher)"
    echo "See https://arduino.github.io/arduino-cli/installation/ for upgrade instructions"
    exit 1
fi

if (( CLI_MAJOR != EXPECTED_CLI_MAJOR || CLI_MINOR != EXPECTED_CLI_MINOR )); then
    echo "You're using an untested version of Arduino CLI, this might cause issues (found: $CLI_MAJOR.$CLI_MINOR.$CLI_REV, expected: $EXPECTED_CLI_MAJOR.$EXPECTED_CLI_MINOR.x)"
fi


get_data_dir() {
	local OUTPUT=$($ARDUINO_CLI config dump | grep 'data: ')
	local lib="${OUTPUT:8}"
	echo "$lib"
}

ARDUINO_DATA_DIR="$(get_data_dir)"
if [ -z "$ARDUINO_DATA_DIR" ]; then
    echo "Arduino data directory not found"
    exit 1
fi

has_samd_core() {
    $ARDUINO_CLI core list | grep -e "arduino:samd.*1.8.9"
}
HAS_ARDUINO_CORE="$(has_samd_core)"
if [ -z "$HAS_ARDUINO_CORE" ]; then
    echo "Installing Arduino Samd core..."
    $ARDUINO_CLI core update-index
    $ARDUINO_CLI core install arduino:samd@1.8.9
    echo "Installing Arduino Samd core OK"
fi

if ! cmp -s "lib/Arduino USBCore driver/USBCore.cpp" "${ARDUINO_DATA_DIR}/packages/arduino/hardware/samd/1.8.9/cores/arduino/USB/USBCore.cpp"; then
    echo "Updating USBCore.cpp"
    cp "lib/Arduino USBCore driver/USBCore.cpp" "${ARDUINO_DATA_DIR}/packages/arduino/hardware/samd/1.8.9/cores/arduino/USB/"
fi


has_SAMDtimer_lib() {
	$ARDUINO_CLI lib list avdweb_SAMDtimer | cut -d ' ' -f2 | grep 1.0.0 || true
}
HAS_SAMDTIMER_LIB="$(has_SAMDtimer_lib)"
if [ -z "$HAS_SAMDTIMER_LIB" ]; then
    echo "Installing SAMDtimer library..."
    $ARDUINO_CLI lib update-index
	$ARDUINO_CLI lib install avdweb_SAMDtimer@1.0.0
    echo "Installing SAMDtimer library OK"
fi

has_ZeroTimer_lib() {
	$ARDUINO_CLI lib list Adafruit_ZeroTimer_Library | cut -d ' ' -f2 | grep 1.0.1 || true
}
HAS_ZEROTIMER_LIB="$(has_ZeroTimer_lib)"
if [ -z "$HAS_ZEROTIMER_LIB" ]; then
    echo "Installing ZeroTimer library..."
    $ARDUINO_CLI lib update-index
	$ARDUINO_CLI lib install "Adafruit ZeroTimer Library"@1.0.1
    echo "Installing ZeroTimer library OK"
fi

has_BusIO_lib() {
	$ARDUINO_CLI lib list Adafruit_BusIO | cut -d ' ' -f2 | grep 1.8.2 || true
}
HAS_BUSIO_LIB="$(has_BusIO_lib)"
if [ -z "$HAS_BUSIO_LIB" ]; then
    echo "Installing BusIO library..."
    $ARDUINO_CLI lib update-index
	$ARDUINO_CLI lib install "Adafruit BusIO"@1.8.2
    echo "Installing BusIO library OK"
fi

has_GFX_lib() {
	$ARDUINO_CLI lib list Adafruit_GFX_Library | cut -d ' ' -f2 | grep 1.10.10 || true
}
HAS_GFX_LIB="$(has_GFX_lib)"
if [ -z "$HAS_GFX_LIB" ]; then
    echo "Installing GFX library..."
    $ARDUINO_CLI lib update-index
	$ARDUINO_CLI lib install "Adafruit GFX Library"@1.10.10
    echo "Installing GFX library OK"
fi

has_SSD1306_lib() {
	$ARDUINO_CLI lib list Adafruit_SSD1306 | cut -d ' ' -f2 | grep 2.4.6 || true
}
HAS_SSD1306_LIB="$(has_SSD1306_lib)"
if [ -z "$HAS_SSD1306_LIB" ]; then
    echo "Installing SSD1306 library..."
    $ARDUINO_CLI lib update-index
	$ARDUINO_CLI lib install "Adafruit SSD1306"@2.4.6
    echo "Installing SSD1306 library OK"
fi

has_SdFat_lib() {
	$ARDUINO_CLI lib list SdFat | cut -d ' ' -f2 | grep 2.0.6 || true
}
HAS_SDFAT_LIB="$(has_SdFat_lib)"
if [ -z "$HAS_SDFAT_LIB" ]; then
    echo "Installing SdFat library..."
    $ARDUINO_CLI lib update-index
	$ARDUINO_CLI lib install "SdFat"@2.0.6
    echo "Installing SdFat library OK"
fi

has_HIDPROJECT_lib() {
	$ARDUINO_CLI lib list HID-Project | cut -d ' ' -f2 | grep 2.6.1 || true
}
HAS_HIDPROJECT_LIB="$(has_HIDPROJECT_lib)"
if [ -z "$HAS_HIDPROJECT_LIB" ]; then
    echo "Installing HID-Project library..."
    $ARDUINO_CLI lib update-index
	$ARDUINO_CLI lib install "HID-Project"@2.6.1
    echo "Installing HID-Project library OK"
fi

get_library_dir() {
	local OUTPUT=$($ARDUINO_CLI config dump | grep 'user: ')
	local lib="${OUTPUT:8}"/libraries
	echo "$lib"
}

ARDUINO_LIB_DIR="$(get_library_dir)"
if [ -z "$ARDUINO_LIB_DIR" ]; then
    echo "Arduino libraries directory not found"
    exit 1
fi


has_ASFCORE_lib() {
	$ARDUINO_CLI lib list | grep Adafruit_ASFcore || true
}
HAS_ASFCORE_LIB="$(has_ASFCORE_lib)"
if [ -z "$HAS_ASFCORE_LIB" ]; then
    echo "Installing ASF Core library..."
    wget -O "${ARDUINO_LIB_DIR}/Adafruit_ASFcore.zip" https://github.com/adafruit/Adafruit_ASFcore/archive/master.zip
    unzip "${ARDUINO_LIB_DIR}/Adafruit_ASFcore.zip" -d "${ARDUINO_LIB_DIR}/"
    rm "${ARDUINO_LIB_DIR}/Adafruit_ASFcore.zip"
    echo "Installing ASF Core library OK"
fi

# Check for Syntiant ilib version
if ! cmp -s "lib/syntiant_ilib/VERSION" "${ARDUINO_LIB_DIR}/syntiant_ilib/VERSION"; then
    echo "Installing Syntiant ilib library..."
    cp -R lib/syntiant_ilib "${ARDUINO_LIB_DIR}"
    echo "Installing Syntiant ilib library OK"
fi


has_AUDIOUSB_lib() {
	$ARDUINO_CLI lib list AudioUSB | cut -d ' ' -f2 | grep 0.0.1 || true
}
HAS_AUDIOUSB_LIB="$(has_AUDIOUSB_lib)"
if [ -z "$HAS_AUDIOUSB_LIB" ]; then
    echo "Installing AudioUSB library..."
    cp -R lib/AudioUSB "${ARDUINO_LIB_DIR}"
    echo "Installing AudioUSB library OK"
fi

has_SerialFlash_lib() {
	$ARDUINO_CLI lib list SerialFlash | cut -d ' ' -f2 | grep 0.5 || true
}
HAS_SERIALFLASH_LIB="$(has_SerialFlash_lib)"
if [ -z "$HAS_SERIALFLASH_LIB" ]; then
    echo "Installing SerialFlash library..."
    cp -R lib/SerialFlash "${ARDUINO_LIB_DIR}"
    echo "Installing SerialFlash library OK"
fi

# Check NDP v1.0.0
has_NDP_lib() {
	$ARDUINO_CLI lib list NDP | grep 1.0.0 || true
}
HAS_NDP_LIB="$(has_NDP_lib)"
if [ -z "$HAS_NDP_LIB" ]; then
    echo "Installing NDP library..."
    cp -R lib/NDP "${ARDUINO_LIB_DIR}"
    echo "Installing NDP library OK"
fi

# Check NDP_utils v1.0.2
has_NDP_utils_lib() {
	$ARDUINO_CLI lib list NDP_utils | grep 1.0.2 || true
}
HAS_NDP_UTILS_LIB="$(has_NDP_utils_lib)"
if [ -z "$HAS_NDP_UTILS_LIB" ]; then
    echo "Installing NDP_utils library..."
    cp -R lib/NDP_utils "${ARDUINO_LIB_DIR}"
    echo "Installing NDP_utils library OK"
fi


# Check PMIC v1.0.0
has_PMIC_lib() {
	$ARDUINO_CLI lib list PMIC_SGM41512 | grep 1.0.0 || true
}
HAS_PMIC_LIB="$(has_PMIC_lib)"
if [ -z "$HAS_PMIC_LIB" ]; then
    echo "Installing PMIC library..."
    cp -R lib/PMIC_SGM41512 "${ARDUINO_LIB_DIR}"
    echo "Installing PMIC library OK"
fi

# CLI v0.14 updates the name of this to --build-property
if ((CLI_MAJOR >= 0 && CLI_MINOR >= 14)); then
    BUILD_PROPERTIES_FLAG=--build-property
else
    BUILD_PROPERTIES_FLAG=--build-properties
fi

INCLUDE="-DEI_SENSOR_AQ_STREAM=FILE"
# INCLUDE+=" -I./src/"
# INCLUDE+=" -I/ingestion-sdk-platform/repl/"
# INCLUDE+=" -I./src/ingestion-sdk-platform/syntiant/"
# INCLUDE+=" -I./src/sensors/"
# INCLUDE+=" -I./src/ingestion-sdk-c/"
# INCLUDE+=" -I./src/QCBOR/inc/"
# INCLUDE+=" -I./src/sensor_aq_mbedtls/"
INCLUDE+=" -DUSE_ARDUINO_MKR_PIN_LAYOUT -D__SAMD21G18A__ "
INCLUDE+="-DUSB_VID=0x2341 -DUSB_PID=0x804f -DUSBCON -DUSB_MANUFACTURER=\"Syntiant\" -DUSB_PRODUCT=\"TinyML\""

if [ "$SENSOR" = "--with-imu" ];
then
    CPP_FLAGS="-DWITH_IMU"
else
    CPP_FLAGS=""
fi

if [ "$COMMAND" = "--build" ];
then
	echo "Building $PROJECT"
	$ARDUINO_CLI compile --fqbn $BOARD $BUILD_PROPERTIES_FLAG build.extra_flags="$INCLUDE" $BUILD_PROPERTIES_FLAG "compiler.cpp.extra_flags=${CPP_FLAGS}" --output-dir . &
	pid=$! # Process Id of the previous running command
	while kill -0 $pid 2>/dev/null
	do
		echo "Still building..."
		sleep 2
	done
	wait $pid
	ret=$?
	if [ $ret -eq 0 ]; then
		echo "Building $PROJECT done"
	else
		echo "Building $PROJECT failed"
        exit 1
	fi
elif [ "$COMMAND" = "--flash" ];
then
	$ARDUINO_CLI upload -p $($ARDUINO_CLI board list | grep Arduino | cut -d ' ' -f1) --fqbn $BOARD --input-dir .
elif [ "$COMMAND" = "--all" ];
then
	$ARDUINO_CLI compile --clean --fqbn $BOARD $BUILD_PROPERTIES_FLAG build.extra_flags="$INCLUDE" $BUILD_PROPERTIES_FLAG "compiler.cpp.extra_flags=${CPP_FLAGS}" --output-dir .
	status=$?
	[ $status -eq 0 ] && $ARDUINO_CLI upload -p $($ARDUINO_CLI board list | grep Arduino | cut -d ' ' -f1) --fqbn $BOARD --input-dir .
else
	echo "Nothing to do for target"
fi
