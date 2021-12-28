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

/* Includes --------------------------------------------------------------------- */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ingestion-sdk-c/ei_config.h"
#include "repl/at_cmds.h"
#include "ingestion-sdk-platform/syntiant/ei_syntiant_fs_commands.h"
#include "model-parameters/model_metadata.h"
#include "model-parameters/model_variables.h"

/* Extern declared function ------------------------------------------------ */
extern void on_classification_changed(const char *event, float confidence, float anomaly_score);

/* Static function forward declerations ------------------------------------ */
static void run_nn_normal(void);

/* Static variables -------------------------------------------------------- */
static bool run_impulse = false;

/**
 * @brief      Setup config & register commands
 */
void ei_setup(void)
{
    // intialize configuration
    static ei_config_ctx_t config_ctx = { 0 };
    config_ctx.get_device_id = EiDevice.get_id_function();
    config_ctx.get_device_type = EiDevice.get_type_function();
    config_ctx.wifi_connection_status = EiDevice.get_wifi_connection_status_function();
    config_ctx.wifi_present = EiDevice.get_wifi_present_status_function();
    config_ctx.load_config = &ei_syntiant_fs_load_config;
    config_ctx.save_config = &ei_syntiant_fs_save_config;
    config_ctx.list_files = NULL;
    config_ctx.read_buffer = EiDevice.get_read_sample_buffer_function();

    EI_CONFIG_ERROR cr = ei_config_init(&config_ctx);

    if (cr != EI_CONFIG_OK) {
        ei_printf("Failed to initialize configuration (%d)\n", cr);
    }
    else {
        ei_printf("Loaded configuration\n");
    }

    ei_at_register_generic_cmds();
    ei_at_cmd_register("RUNIMPULSE", "Run the impulse", run_nn_normal);

    /* Auto start impulse */
    run_nn_normal();
}

/**
 * @brief      Called from the ndp101 read out. Print classification output
 *             and send matched output string to user callback
 *
 * @param[in]  matched_feature
 */
void ei_classification_output(int matched_feature)
{
    if (ei_run_impulse_active()) {

        ei_printf("\nPredictions:\r\n");

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            ei_printf("    %s: \t%d\r\n", ei_classifier_inferencing_categories[ix],
                (matched_feature == ix) ? 1 : 0);
        }

        on_classification_changed(ei_classifier_inferencing_categories[matched_feature], 0, 0);
    }
}

/**
 * @brief      Start impulse, print settings
 */
static void run_nn_normal(void)
{
    ei_start_stop_run_impulse(true);
    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.4f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) /
            sizeof(ei_classifier_inferencing_categories[0]));

    ei_printf("Starting inferencing, press 'b' to break\n");
}