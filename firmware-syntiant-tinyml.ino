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

#include "src/syntiant.h"
#include <NDP.h>
#include <NDP_utils.h>
#include <Arduino.h>

/**
 * @brief      Called when a inference matches 1 of the features
 *
 * @param[in]  event          The event
 * @param[in]  confidence     The confidence
 * @param[in]  anomaly_score  The anomaly score
 */
void on_classification_changed(const char *event, float confidence, float anomaly_score) {

    // here you can write application code, e.g. to toggle LEDs based on keywords
    if (strcmp(event, "stop") == 0) {
        // Toggle LED
        digitalWrite(LED_RED, HIGH);
    }

    if (strcmp(event, "go") == 0) {
        // Toggle LED
        digitalWrite(LED_GREEN, HIGH);
    }
}



void setup(void)
{
    syntiant_setup();
}

void loop(void)
{
    syntiant_loop();
}