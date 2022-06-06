/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
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
#include <stdint.h>

#include "ei_inertialsensor.h"
#include "../ingestion-sdk-platform/syntiant/ei_device_syntiant_samd.h"
#include "../firmware-sdk/sensor_aq.h"
#include "../edge-impulse-sdk/porting/ei_classifier_porting.h"

/* Extern declared --------------------------------------------------------- */
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

/* Private variables ------------------------------------------------------- */
static sampler_callback  cb_sampler;
static float imu_data[N_AXIS_SAMPLED * 6];

extern void syntiant_get_imu(float *dest_imu);

/**
 * @brief      Get data from sensor, convert and call callback to handle
 */
int ei_inertial_read_data(void)
{
    uint64_t startTime = ei_read_timer_ms();
    uint64_t endTime;

    syntiant_get_imu(imu_data);

    while(1) {
        for(int i = 0; i < 6; i++) {

            if(cb_sampler((const void *)&imu_data[i * 6], SIZEOF_N_AXIS_SAMPLED) == true) {
                return 0;
            }
        }

        endTime = ei_read_timer_ms();
        ei_sleep(60 - (endTime - startTime));

        startTime = ei_read_timer_ms();
        syntiant_get_imu(imu_data);
    }

    return 0;
}

/**
 * @brief      Setup timing and data handle callback function
 *
 * @param[in]  callsampler         Function to handle the sampled data
 * @param[in]  sample_interval_ms  The sample interval milliseconds
 *
 * @return     true
 */
bool ei_inertial_sample_start(sampler_callback callsampler, float sample_interval_ms)
{
    cb_sampler = callsampler;

    ei_config_set_sample_interval(sample_interval_ms);

    EiDevice.set_state(eiStateSampling);

    ei_inertial_read_data();

    return true;
}


/**
 * @brief      Setup payload header
 *
 * @return     true
 */
bool ei_inertial_setup_data_sampling(void)
{
#ifdef WITH_IMU

    if (ei_config_get_config()->sample_interval_ms < 0.001f) {
        ei_config_set_sample_interval(1.f/62.5f);
    }

    sensor_aq_payload_info payload = {
        // Unique device ID (optional), set this to e.g. MAC address or device EUI **if** your device has one
        EiDevice.get_id_pointer(),
        // Device type (required), use the same device type for similar devices
        EiDevice.get_type_pointer(),
        // How often new data is sampled in ms. (100Hz = every 10 ms.)
        ei_config_get_config()->sample_interval_ms,
        // The axes which you'll use. The units field needs to comply to SenML units (see https://www.iana.org/assignments/senml/senml.xhtml)
        { { "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps " },
         { "accX", "m/s2" }, { "accY", "m/s2" }, { "accZ", "m/s2" }},
    };

    EiDevice.set_state(eiStateErasingFlash);
    ei_sampler_start_sampling(&payload, &ei_inertial_sample_start, SIZEOF_N_AXIS_SAMPLED);
    EiDevice.set_state(eiStateIdle);
#else
    ei_printf("ERR: IMU currently disabled, download the IMU firmware or compile with: ./arduino-build.sh --build --with-imu\r\n");
#endif
    return true;
}