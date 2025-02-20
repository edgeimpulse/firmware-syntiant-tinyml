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

#ifndef _EI_FUSION_H
#define _EI_FUSION_H

/* Include ----------------------------------------------------------------- */
#include "ei_config_types.h"
#include "ei_fusion_sensors_config.h"
#include "sensor_aq.h"

#define EI_MAX_FREQUENCIES 5

/** Format used in input list. Can either contain sensor names or axes names */
typedef enum
{
    SENSOR_FORMAT = 0,
    AXIS_FORMAT

} ei_fusion_list_format;

/**
 * Information about the fusion structure, name, number of axis, sampling frequencies, axis name, and reference to read sensor function
 */
typedef struct {
    // Name of sensor to show up in Studio
    const char *name;
    // Number of sensor axis to sample
    int num_axis;
    // List of sensor sampling frequencies
    float frequencies[EI_MAX_FREQUENCIES];
    // Sensor axes, note that I declare this not as a pointer to have a more fluent interface
    sensor_aq_sensor sensors[EI_MAX_SENSOR_AXES];
    // Reference to read sensor function that should return pointer to float array of raw sensor data
    fusion_sample_format_t *(*read_data)(int n_samples);
    // Axis used
    int axis_flag_used;
} ei_device_fusion_sensor_t;

/* Function prototypes ----------------------------------------------------- */
bool ei_add_sensor_to_fusion_list(ei_device_fusion_sensor_t sensor);
void ei_built_sensor_fusion_list(void);
bool ei_connect_fusion_list(const char *input_list, ei_fusion_list_format format);
void ei_fusion_read_axis_data(void);
bool ei_fusion_sample_start(sampler_callback callsampler, float sample_interval_ms);
bool ei_fusion_setup_data_sampling();

#endif