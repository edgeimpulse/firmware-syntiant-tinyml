/* The Clear BSD License
 *
 * Copyright (c) 2025 EdgeImpulse Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EDGE_IMPULSE_AT_COMMANDS_CONFIG_H_
#define _EDGE_IMPULSE_AT_COMMANDS_CONFIG_H_

#include "at_cmd_interface.h"
#include "at_base64.h"
#include "../ingestion-sdk-c/ei_config.h"
#include "ei_syntiant_fs_commands.h"

#define EDGE_IMPULSE_AT_COMMAND_VERSION        "1.6.0"

static void at_clear_config() {
    ei_printf("Clearing config and restarting system...\r\n");
    ei_config_clear();
//    NVIC_SystemReset();
}

static void at_device_info() {
    uint8_t id_buffer[32] = { 0 };
    size_t id_size;
    int r = ei_config_get_device_id(id_buffer, &id_size);
    if (r == EI_CONFIG_OK) {
        id_buffer[id_size] = 0;
        ei_printf("ID:         %s\r\n", id_buffer);
    }
    if (ei_config_get_context()->get_device_type == NULL) {
        return;
    }
    r = ei_config_get_context()->get_device_type(id_buffer, &id_size);
    if (r == EI_CONFIG_OK) {
        id_buffer[id_size] = 0;
        ei_printf("Type:       %s\r\n", id_buffer);
    }
    ei_printf("AT Version: %s\r\n", EDGE_IMPULSE_AT_COMMAND_VERSION);
}

static void at_get_wifi() {
    char *ssid;
    char *password;
    ei_config_security_t security;
    bool connected, present;

    EI_CONFIG_ERROR r = ei_config_get_wifi(&ssid, &password, &security, &connected, &present);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to retrieve WiFi config (%d)\r\n", r);
        return;
    }
    ei_printf("SSID:      %s\r\n", ssid);
    ei_printf("Password:  %s\r\n", password);
    ei_printf("Security:  %d\r\n", security);

    uint8_t mac_buffer[32] = { 0 };
    size_t mac_size;
    r = ei_config_get_device_id(mac_buffer, &mac_size);
    if (r == EI_CONFIG_OK) {
        mac_buffer[mac_size] = 0;
        ei_printf("MAC:       %s\r\n", mac_buffer);
    }
    ei_printf("Connected: %d\r\n", connected);
    ei_printf("Present:   %d\r\n", present);
}

static void at_set_wifi(char *ssid, char *password, char *security_s) {
    ei_config_security_t security = (ei_config_security_t)atoi(security_s);

    EI_CONFIG_ERROR r = ei_config_set_wifi(ssid, password, security);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to persist WiFi config (%d)\r\n", r);
    }
    else {
        ei_printf("OK\r\n");
    }
}

static void at_get_sample_settings() {
    char *label;
    float interval;
    uint32_t length;
    char *hmac_key;

    EI_CONFIG_ERROR r = ei_config_get_sample_settings(&label, &interval, &length, &hmac_key);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to retrieve sample settings (%d)\r\n", r);
        return;
    }
    ei_printf("Label:     %s\r\n", label);
    ei_printf("Interval:  ");
    ei_printf_float((float)interval);
    ei_printf("ms.\r\n");;
    ei_printf("Length:    %lu ms.\r\n", length);
    ei_printf("HMAC key:  %s\r\n", hmac_key);
}

static void at_set_sample_settings(char *label, char *interval_s, char *length_s) {
    float interval = atof(interval_s);
    uint32_t length = (uint32_t)atoi(length_s);
    EI_CONFIG_ERROR r = ei_config_set_sample_settings(label, interval, length);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to persist sampling settings (%d)\r\n", r);
    }
    else {
        ei_printf("OK\r\n");
    }
}

static void at_set_sample_settings_w_hmac(char *label, char *interval_s, char *length_s, char *hmac_key) {
    float interval = atof(interval_s);
    uint32_t length = (uint32_t)atoi(length_s);
    EI_CONFIG_ERROR r = ei_config_set_sample_settings(label, interval, length, hmac_key);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to persist sampling settings (%d)\r\n", r);
    }
    else {
        ei_printf("OK\r\n");
    }
}

static void at_get_upload_settings() {
    char *api_key;
    char *host;
    char *path;

    EI_CONFIG_ERROR r = ei_config_get_upload_settings(&api_key, &host, &path);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to retrieve upload settings (%d)\r\n", r);
        return;
    }
    ei_printf("Api Key:   %s\r\n", api_key);
    ei_printf("Host:      %s\r\n", host);
    ei_printf("Path:      %s\r\n", path);
}

static void at_set_upload_host(char *host) {
    EI_CONFIG_ERROR r = ei_config_set_upload_host_settings(host);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to persist upload settings (%d)\r\n", r);
    }
    else {
        ei_printf("OK\r\n");
    }
}

static void at_set_upload_settings(char *api_key, char *url) {
    EI_CONFIG_ERROR r = ei_config_set_upload_path_settings(api_key, url);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to persist upload settings (%d)\r\n", r);
    }
    else {
        ei_printf("OK\r\n");
    }
}

static void at_get_mgmt_settings() {
    char *mgmt_url;
    bool is_connected;
    char last_error[128];

    EI_CONFIG_ERROR r = ei_config_get_mgmt_settings(&mgmt_url, &is_connected, last_error, 128);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to retrieve management settings (%d)\r\n", r);
        return;
    }
    ei_printf("URL:        %s\r\n", mgmt_url);
    ei_printf("Connected:  %d\r\n", is_connected);
    ei_printf("Last error: %s\r\n", last_error);
}

static void at_set_mgmt_settings(char *mgmt_url) {
    EI_CONFIG_ERROR r = ei_config_set_mgmt_settings(mgmt_url);
    if (r != EI_CONFIG_OK) {
        ei_printf("Failed to persist management settings (%d)\r\n", r);
    }
    else {
        ei_printf("OK\r\n");
    }
}

static void at_list_sensors() {

    const ei_sensor_t *list;
    size_t list_size;
    
    int r = EiDevice.get_sensor_list((const ei_device_sensor_t **)&list, &list_size);
    if (r != 0) {
        ei_printf("Failed to get sensor list (%d)\r\n", r);
        return;
    }

    for (size_t ix = 0; ix < list_size; ix++) {
        ei_printf("Name: %s, Max sample length: %hus, Frequencies: [", list[ix].name, list[ix].max_sample_length_s);
        for (size_t fx = 0; fx < EDGE_IMPULSE_MAX_FREQUENCIES; fx++) {
            if (list[ix].frequencies[fx] != 0.0f) {
                if (fx != 0) {
                    ei_printf(", ");
                }
                ei_printf_float(list[ix].frequencies[fx]);
                ei_printf("Hz");
            }
        }
        ei_printf("]\r\n");
    }
}

static void at_list_config() {
    ei_printf("===== Device info =====\r\n");
    at_device_info();
    ei_printf("\r\n");
    ei_printf("===== Sensors ======\r\n");
    at_list_sensors();
    ei_printf("\r\n");
    ei_printf("===== WIFI =====\r\n");
    at_get_wifi();
    ei_printf("\r\n");
    ei_printf("===== Sampling parameters =====\r\n");
    at_get_sample_settings();
    ei_printf("\r\n");
    ei_printf("===== Upload settings =====\r\n");
    at_get_upload_settings();
    ei_printf("\r\n");
    ei_printf("===== Remote management =====\r\n");
    at_get_mgmt_settings();
    ei_printf("\r\n");
}

static void at_list_files_data(char *name) {
    ei_printf("%s\r\n", name);
}

static void at_list_files() {
    ei_config_get_context()->list_files(at_list_files_data);
}

static void at_read_file_data(uint8_t *buffer, size_t size) {
    //char *base64_buffer = (char*)calloc((size / 3 * 4) + 4, 1);
    static char base64_buffer[(513 / 3 * 4) + 4];
    if (!base64_buffer) {
        ei_printf("ERR: Cannot allocate base64 buffer of size %lu, out of memory\r\n", (size / 3 * 4) + 4);
        return;
    }

    int r = base64_encode((const char*)buffer, size, base64_buffer, (size / 3 * 4) + 4);
    if (r < 0) {
        ei_printf("ERR: Failed to base64 encode (%d)\r\n", r);
        return;
    }
    
    ei_write_string(base64_buffer, r);

    //free(base64_buffer);
}

static void at_read_file(char *filename) {
    bool exists = ei_config_get_context()->read_file(filename, at_read_file_data);
    if (!exists) {
        ei_printf("File '%s' does not exist\r\n", filename);
    }
    else {
        ei_printf("\r\n");
    }
}

extern void timer_4_handling(bool turn);

static void at_read_buffer(char *start_s, char *length_s, char *baudrate_s) {
    size_t start = (size_t)atoi(start_s);
    size_t length = (size_t)atoi(length_s);

    timer_4_handling(false);    // turn off
    bool success = ei_config_get_context()->read_buffer(start, length, at_read_file_data);
    if (!success) {
        ei_printf("Failed to read from buffer\r\n");
    }
    else {
        ei_printf("\r\n");
    }
    timer_4_handling(true); // turn on
}

static void at_read_raw(char *start_s, char *length_s) {
    size_t start = (size_t)atoi(start_s);
    size_t length = (size_t)atoi(length_s);
    unsigned char buffer[32];

    int count = 0;

    for(; start < length; start += 32) {
        ei_syntiant_fs_read_sample_data(buffer, start, 32);

        int n_display_bytes = (length - start) < 32 ? (length - start) : 32;
        for(int i=0; i<n_display_bytes; i++) {
            ei_printf("%02X ", (unsigned char)buffer[i]);
        }
        ei_printf("\b\r\n");
    }
}

static void at_unlink_file(char *filename) {
    // bool success = ei_config_get_context()->unlink_file(filename);
    // if (success) {
        ei_printf("\r\n");
    // }
    // else {
    //     ei_printf("File '%s' could not be unlinked\n", filename);
    // }
}
/*
static void at_upload_file(char *filename) {
    if (!ei_config_get_context()->upload_file) {
        ei_printf("upload_file pointer not set\n");
        return;
    }

    if (!ei_config_get_context()->wifi_connection_status()) {
        ei_printf("Not connected to WiFi, cannot upload\n");
        return;
    }

    ei_printf("Uploading '%s' to %s%s...\n", filename,
        ei_config_get_config()->upload_host,
        ei_config_get_config()->upload_path);

    FILE *file = fopen(filename, "r+");
    if (!file) {
        ei_printf("Failed to upload file, cannot open '%s'\n", filename);
        return;
    }

    bool valid = ei_config_get_context()->upload_file(file, (const char*)filename);
    if (!valid) {
        ei_printf("Failed to upload file\n");
    }
    else {
        ei_printf("File uploaded\n");
    }

    fclose(file);
}
*/
static void at_scan_wifi_data(const char *ssid, ei_config_security_t security, int8_t rssi) {
    ei_printf("SSID: %s, Security: ", ssid);

    switch (security) {
        case EI_SECURITY_NONE: ei_printf("None"); break;
        case EI_SECURITY_WEP: ei_printf("WEP"); break;
        case EI_SECURITY_WPA: ei_printf("WPA"); break;
        case EI_SECURITY_WPA2: ei_printf("WPA2"); break;
        case EI_SECURITY_WPA_WPA2: ei_printf("WPA_WPA2"); break;
        case EI_SECURITY_PAP: ei_printf("PAP"); break;
        case EI_SECURITY_CHAP: ei_printf("CHAP"); break;
        case EI_SECURITY_EAP_TLS: ei_printf("EAP_TLS"); break;
        case EI_SECURITY_PEAP: ei_printf("PEAP"); break;
        default: ei_printf("Unknown"); break;
    }

    ei_printf(" (%d), RSSI: %d dBm\n", security, rssi);
}

static void at_scan_wifi() {
    if (ei_config_get_context()->scan_wifi == NULL) {
        ei_printf("Device does not have a WiFi interface\n");
        return;
    }
    bool success = ei_config_get_context()->scan_wifi(at_scan_wifi_data);
    if (!success) {
        ei_printf("Failed to scan for WiFi networks\n");
    }
}

static void at_sample_start(char *sensor_name) {

    const ei_sensor_t *list;
    size_t list_size;

    int r = EiDevice.get_sensor_list((const ei_device_sensor_t **)&list, &list_size);
    if (r != 0) {
        ei_printf("Failed to get sensor list (%d)\r\n", r);
        return;
    }

    for (size_t ix = 0; ix < list_size; ix++) {
        if (strcmp(list[ix].name, sensor_name) == 0) {
            bool r = list[ix].start_sampling_cb();
            if (!r) {
                ei_printf("Failed to start sampling\r\n");
            }
            return;
        }
    }

    ei_printf("Failed to find sensor '%s' in the sensor list\r\n", sensor_name);
}

static void at_clear_files_data(char *filename) {
    if (ei_config_get_context()->unlink_file(filename)) {
        ei_printf("Unlinked '%s'\r\n", filename);
    }
    else {
        ei_printf("ERR: Failed to unlink '%s'\r\n", filename);
    }
}

static void at_clear_fs() {
    ei_printf("Clearing file system...\r\n");

    ei_config_get_context()->list_files(at_clear_files_data);
}

static void at_reset() {
//    NVIC_SystemReset();
}

static void at_boot_mode() 
{
    #define _BOOTLOADER_MAGIC_ADDR 0x10000000 + 0x20000
    #define _BOOTLOADER_MAGIC_VAL  0xBADC0DE

    ei_printf("AT+ACK\r");
}

// AT commands related to configuration
void ei_at_register_generic_cmds() {
    ei_at_cmd_register("HELP", "Lists all commands", &ei_at_cmd_print_info);
    ei_at_cmd_register("CLEARCONFIG", "Clears complete config and resets system", &at_clear_config);
    ei_at_cmd_register("CLEARFILES", "Clears all files from the file system, this does not clear config", &at_clear_fs);
    ei_at_cmd_register("CONFIG?", "Lists complete config", &at_list_config);
    ei_at_cmd_register("DEVICEINFO?", "Lists device information", &at_device_info);
    ei_at_cmd_register("SENSORS?", "Lists sensors", &at_list_sensors);
    ei_at_cmd_register("RESET", "Reset the system", &at_reset);
    ei_at_cmd_register("WIFI?", "Lists current WiFi credentials", &at_get_wifi);
    ei_at_cmd_register("WIFI=", "Sets current WiFi credentials (SSID,PASSWORD,SECURITY)", &at_set_wifi);
    ei_at_cmd_register("SCANWIFI", "Scans for WiFi networks", &at_scan_wifi);
    ei_at_cmd_register("SAMPLESETTINGS?", "Lists current sampling settings", &at_get_sample_settings);
    ei_at_cmd_register("SAMPLESETTINGS=", "Sets current sampling settings (LABEL,INTERVAL_MS,LENGTH_MS)", &at_set_sample_settings);
    ei_at_cmd_register("SAMPLESETTINGS=", "Sets current sampling settings (LABEL,INTERVAL_MS,LENGTH_MS,HMAC_KEY)",
        &at_set_sample_settings_w_hmac);
    ei_at_cmd_register("UPLOADSETTINGS?", "Lists current upload settings", &at_get_upload_settings);
    ei_at_cmd_register("UPLOADSETTINGS=", "Sets current upload settings (APIKEY,PATH)", &at_set_upload_settings);
    ei_at_cmd_register("UPLOADHOST=", "Sets upload host (HOST)", &at_set_upload_host);
    ei_at_cmd_register("MGMTSETTINGS?", "Lists current management settings", &at_get_mgmt_settings);
    ei_at_cmd_register("MGMTSETTINGS=", "Sets current management settings (URL)", &at_set_mgmt_settings);
    ei_at_cmd_register("LISTFILES", "Lists all files on the device", &at_list_files);
    ei_at_cmd_register("READFILE=", "Read a specific file (as base64)", &at_read_file);
    ei_at_cmd_register("READBUFFER=", "Read from the temporary buffer (as base64) (START,LENGTH,USEMAXRATE?(y/n))", &at_read_buffer);
    ei_at_cmd_register("UNLINKFILE=", "Unlink a specific file", &at_unlink_file);
//    ei_at_cmd_register("UPLOADFILE=", "Upload a specific file", &at_upload_file);
    ei_at_cmd_register("SAMPLESTART=", "Start sampling", &at_sample_start);
    ei_at_cmd_register("READRAW=", "Read raw from flash (START,LENGTH)", &at_read_raw);
    ei_at_cmd_register("BOOTMODE", "Jump to bootloader", &at_boot_mode);
}

#endif // _EDGE_IMPULSE_AT_COMMANDS_CONFIG_H_
