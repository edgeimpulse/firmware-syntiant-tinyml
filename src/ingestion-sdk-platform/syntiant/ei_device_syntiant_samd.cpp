/* Edge Impulse ingestion SDK
 * Copyright (c) 2021 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Include ----------------------------------------------------------------- */
#include "ei_device_syntiant_samd.h"
#include "ei_syntiant_fs_commands.h"
#include "../../repl/repl.h"
#include "../../sensors/ei_inertialsensor.h"

#include "Arduino.h"
#include "NDP_Serial.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <math.h>


/** Memory location for the arduino device address */
#define DEVICE_ID_LSB_ADDR  ((uint32_t)0x0080A00C)
#define DEVICE_ID_MSB_ADDR  ((uint32_t)0x0080A040)

/** Max size for device id array */
#define DEVICE_ID_MAX_SIZE  32

/** Sensors */
typedef enum
{    
    MICROPHONE = 0,
    INTERTIAL = 1

}used_sensors_t;

/** Device type */
static const char *ei_device_type = "SYNTIANT_TINYML";/** @todo Get actual device type */

/** Device id array */
static char ei_device_id[DEVICE_ID_MAX_SIZE] = {"34:52:52:22:57:98"};

/** Device object, for this class only 1 object should exist */
EiDeviceSyntiant EiDevice;

static tEiState ei_program_state = eiStateIdle;

static bool run_impulse = false;

/* Private function declarations ------------------------------------------- */
static int get_id_c(uint8_t out_buffer[32], size_t *out_size);
static int get_type_c(uint8_t out_buffer[32], size_t *out_size);
static bool get_wifi_connection_status_c(void);
static bool get_wifi_present_status_c(void);
static void timer_callback(void *arg);
static bool read_sample_buffer(size_t begin, size_t length, void(*data_fn)(uint8_t*, size_t));

/* Public functions -------------------------------------------------------- */

EiDeviceInfo* EiDeviceInfo::get_device(void)
{
    // static EiFlashMemory memory(sizeof(EiConfig));
    // static EiDeviceSyntiant dev();

    return &EiDevice;
}

EiDeviceSyntiant::EiDeviceSyntiant(void)
{
    uint32_t *id_msb = (uint32_t *)DEVICE_ID_MSB_ADDR;
    uint32_t *id_lsb = (uint32_t *)DEVICE_ID_LSB_ADDR;

    /* Setup device ID */
    snprintf(&ei_device_id[0], DEVICE_ID_MAX_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X"
        ,(*id_msb >> 8) & 0xFF
        ,(*id_msb >> 0) & 0xFF
        ,(*id_lsb >> 24)& 0xFF
        ,(*id_lsb >> 16)& 0xFF
        ,(*id_lsb >> 8) & 0xFF
        ,(*id_lsb >> 0) & 0xFF
        );        
}

/**
 * @brief      For the device ID, the BLE mac address is used.
 *             The mac address string is copied to the out_buffer.
 *
 * @param      out_buffer  Destination array for id string
 * @param      out_size    Length of id string
 *
 * @return     0
 */
int EiDeviceSyntiant::get_id(uint8_t out_buffer[32], size_t *out_size)
{
    return get_id_c(out_buffer, out_size);
}

/**
 * @brief      Gets the identifier pointer.
 *
 * @return     The identifier pointer.
 */
const char *EiDeviceSyntiant::get_id_pointer(void)
{
    return (const char *)ei_device_id;
}

/**
 * @brief      Copy device type in out_buffer & update out_size
 *
 * @param      out_buffer  Destination array for device type string
 * @param      out_size    Length of string
 *
 * @return     -1 if device type string exceeds out_buffer
 */
int EiDeviceSyntiant::get_type(uint8_t out_buffer[32], size_t *out_size)
{
    return get_type_c(out_buffer, out_size);
}

/**
 * @brief      Gets the type pointer.
 *
 * @return     The type pointer.
 */
const char *EiDeviceSyntiant::get_type_pointer(void)
{
    return (const char *)ei_device_type;
}

/**
 * @brief      No Wifi available for device.
 *
 * @return     Always return false
 */
bool EiDeviceSyntiant::get_wifi_connection_status(void)
{
    return false;
}

/**
 * @brief      No Wifi available for device.
 *
 * @return     Always return false
 */
bool EiDeviceSyntiant::get_wifi_present_status(void)
{
    return false;
}

static bool microphone_callback()
{
    ei_printf("ERR: Can't sample audio directly from device. Go to: \"Devices - Connect a new device - Use your computer\"\r\n");

    return false;
}

/**
 * @brief      Create sensor list with sensor specs
 *             The studio and daemon require this list
 * @param      sensor_list       Place pointer to sensor list
 * @param      sensor_list_size  Write number of sensors here
 *
 * @return     False if all went ok
 */
bool EiDeviceSyntiant::get_sensor_list(const ei_device_sensor_t **sensor_list, size_t *sensor_list_size)
{
    /* Calculate number of bytes available on flash for sampling, reserve 1 block for header + overhead */
    uint32_t available_bytes = 3200000;//(ei_syntiant_fs_get_n_available_sample_blocks()-1) * ei_syntiant_fs_get_block_size();

    sensors[MICROPHONE].name = "Built-in microphone";
    sensors[MICROPHONE].start_sampling_cb = &microphone_callback;
    sensors[MICROPHONE].max_sample_length_s = available_bytes / (16000 * 2);
    sensors[MICROPHONE].frequencies[0] = 16000.0f;

    sensors[INTERTIAL].name = "Inertial";
    sensors[INTERTIAL].start_sampling_cb = &ei_inertial_setup_data_sampling;
    sensors[INTERTIAL].max_sample_length_s = available_bytes / (100 * SIZEOF_N_AXIS_SAMPLED);
    sensors[INTERTIAL].frequencies[0] = 100.f;

    *sensor_list      = sensors;
    *sensor_list_size = EI_DEVICE_N_SENSORS;

    return false;
}

/**
 * @brief      Device specific delay ms implementation
 *
 * @param[in]  milliseconds  The milliseconds
 */
void EiDeviceSyntiant::delay_ms(uint32_t milliseconds)
{    
    delay(milliseconds);
}


void EiDeviceSyntiant::setup_led_control(void)
{

}

void EiDeviceSyntiant::set_state(tEiState state)
{
    ei_program_state = state;

    if(state == eiStateFinished) {

        ei_program_state = eiStateIdle;
    }


}

/**
 * @brief      Get a C callback for the get_id method
 *
 * @return     Pointer to c get function
 */
c_callback EiDeviceSyntiant::get_id_function(void)
{
    return &get_id_c;
}

/**
 * @brief      Get a C callback for the get_type method
 *
 * @return     Pointer to c get function
 */
c_callback EiDeviceSyntiant::get_type_function(void)
{
    return &get_type_c;
}

/**
 * @brief      Get a C callback for the get_wifi_connection_status method
 *
 * @return     Pointer to c get function
 */
c_callback_status EiDeviceSyntiant::get_wifi_connection_status_function(void)
{
    return &get_wifi_connection_status_c;
}

/**
 * @brief      Get a C callback for the wifi present method
 *
 * @return     The wifi present status function.
 */
c_callback_status EiDeviceSyntiant::get_wifi_present_status_function(void)
{
    return &get_wifi_present_status_c;
}

/**
 * @brief      Get a C callback to the read sample buffer function
 *
 * @return     The read sample buffer function.
 */
c_callback_read_sample_buffer EiDeviceSyntiant::get_read_sample_buffer_function(void)
{
    return &read_sample_buffer;
}

/**
 * @brief      Get characters for uart pheripheral and send to repl
 */
bool ei_command_line_handle(void)
{
    char byte;
    bool syntiant_cmd_start_found = false;
    while (Serial.available()) {
        // rx_callback(Serial.read());
        byte = Serial.read();

        if (byte == ':') {
            syntiant_cmd_start_found = true;
            break;
        }
        else if (ei_run_impulse_active() && byte == 'b') {
            ei_start_stop_run_impulse(false);
        } 
        else {
            rx_callback(byte);
        }
    }

    return syntiant_cmd_start_found;
}

/**
 * @brief      Start or stop inference output.
 *             The NDP101 will inference continuously, only the output is blocked
 *
 * @param[in]  start  Set true to start
 */
void ei_start_stop_run_impulse(bool start)
{    
    run_impulse = start;

    if(!start){
        ei_printf("Inferencing stopped by user\r\n");
    }
}

/**
 * @brief      Return active state for run impulse
 *
 * @return     True if active
 */
bool ei_run_impulse_active(void)
{
    return run_impulse;
}

/**
 * @brief      Printf function uses vsnprintf and output using Eta lib function
 *
 * @param[in]  format     Variable argument list
 */
void ei_printf(const char *format, ...)
{
    char print_buf[1024] = {0};

    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);

    if (r > 0) {
        Serial.write(print_buf);
        Serial2.println(print_buf);
    }
}

/**
 * @brief      Print a float value, bypassing the stdio %f
 *             Uses standard serial out
 *
 * @param[in]  f     Float value to print.
 */
void ei_printf_float(float f)
{
    float n = f;

    static double PRECISION = 0.00001;
    static int MAX_NUMBER_STRING_SIZE = 32;

    char s[MAX_NUMBER_STRING_SIZE];

    if (n == 0.0) {
        ei_printf("0.00000");
    } else {
        int digit, m;  //, m1;
        char *c = s;
        int neg = (n < 0);
        if (neg) {
            n = -n;
        }
        // calculate magnitude
        m = log10(n);
        if (neg) {
            *(c++) = '-';
        }
        if (m < 1.0) {
            m = 0;
        }
        // convert the number
        while (n > PRECISION || m >= 0) {
            double weight = pow(10.0, m);
            if (weight > 0 && !isinf(weight)) {
                digit = floor(n / weight);
                n -= (digit * weight);
                *(c++) = '0' + digit;
            }
            if (m == 0 && n > 0) {
                *(c++) = '.';
            }
            m--;
        }
        *(c) = '\0';
        ei_write_string(s, c - s);
    }
}

/**
 * @brief      Write serial data with length to Serial output
 *
 * @param      data    The data
 * @param[in]  length  The length
 */
void ei_write_string(char *data, int length)
{   
//    Serial.write(data, length);

    for(int i = 0; i < length; i++) {
        Serial.write(&data[i], 1);
    }

}

/**
 * @brief      Write single character to serial output
 *
 * @param[in]  cChar  The character
 */
void ei_putc(char cChar)
{
    Serial.write(&cChar, 1);
}




/* Private functions ------------------------------------------------------- */
static void timer_callback(void *arg)
{
    static char toggle = 0;

    if(toggle) {
        switch(ei_program_state)
        {
            case eiStateErasingFlash:   break;
            case eiStateSampling:       break;
            case eiStateUploading:      break;
            default: break;
        }
    }
    else {
        if(ei_program_state != eiStateFinished) {
            // Clear all leds
        }
    }
    toggle ^= 1;
}


static int get_id_c(uint8_t out_buffer[32], size_t *out_size)
{
    size_t length = strlen(ei_device_id);

    if(length < 32) {
        memcpy(out_buffer, ei_device_id, length);

        *out_size = length;
        return 0;
    }

    else {
        *out_size = 0;
        return -1;
    }
}

static int get_type_c(uint8_t out_buffer[32], size_t *out_size)
{
    size_t length = strlen(ei_device_type);

    if(length < 32) {
        memcpy(out_buffer, ei_device_type, length);

        *out_size = length;
        return 0;
    }

    else {
        *out_size = 0;
        return -1;
    }
}

static bool get_wifi_connection_status_c(void)
{
    return false;
}

static bool get_wifi_present_status_c(void)
{
    return false;
}

/**
 * @brief      Read samples from sample memory and send to data_fn function
 *
 * @param[in]  begin    Start address
 * @param[in]  length   Length of samples in bytes
 * @param[in]  data_fn  Callback function for sample data
 *
 * @return     false on flash read function
 */
static bool read_sample_buffer(size_t begin, size_t length, void(*data_fn)(uint8_t*, size_t))
{
    size_t pos = begin;
    size_t bytes_left = length;
    bool retVal;

    EiDevice.set_state(eiStateUploading);

    // we're encoding as base64 in AT+READFILE, so this needs to be divisable by 3
    uint8_t buffer[513];
    while (1) {
        size_t bytes_to_read = sizeof(buffer);
        if (bytes_to_read > bytes_left) {
            bytes_to_read = bytes_left;
        }
        if (bytes_to_read == 0) {
            retVal = true;
            break;
        }

        int r = ei_syntiant_fs_read_sample_data(buffer, pos, bytes_to_read);
        if (r != 0) {
            retVal = false;
            break;

        }
        data_fn(buffer, bytes_to_read);

        pos += bytes_to_read;
        bytes_left -= bytes_to_read;
    }

    EiDevice.set_state(eiStateFinished);

    return retVal;
}