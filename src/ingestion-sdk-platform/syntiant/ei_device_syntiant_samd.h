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

#ifndef EI_DEVICE_SYNTIANT_SAMD
#define EI_DEVICE_SYNTIANT_SAMD

/* Include ----------------------------------------------------------------- */
#include "ei_device_info.h"
#include "../../firmware-sdk/ei_device_memory.h"


/** Number of sensors used */
#define EI_DEVICE_N_SENSORS		2

typedef enum
{
	eiStateIdle 		= 0,
	eiStateErasingFlash,
	eiStateSampling,
	eiStateUploading,
	eiStateFinished

}tEiState;


/** C Callback types */
typedef int (*c_callback)(uint8_t out_buffer[32], size_t *out_size);
typedef bool (*c_callback_status)(void);
typedef bool (*c_callback_read_sample_buffer)(size_t begin, size_t length, void(*data_fn)(uint8_t*, size_t));

/**
 * @brief      Class description and implementation of device specific 
 * 			   characteristics
 */	
class EiDeviceSyntiant : public EiDeviceInfo
{
private:
	ei_device_sensor_t sensors[EI_DEVICE_N_SENSORS];
public:	
	EiDeviceSyntiant(void);
	
	int get_id(uint8_t out_buffer[32], size_t *out_size);
	const char *get_id_pointer(void);
	int get_type(uint8_t out_buffer[32], size_t *out_size);
	const char *get_type_pointer(void);
	bool get_wifi_connection_status(void);
	bool get_wifi_present_status();
	bool get_sensor_list(const ei_device_sensor_t **sensor_list, size_t *sensor_list_size);
	void delay_ms(uint32_t milliseconds);
	void setup_led_control(void);
	void set_state(tEiState state);

	c_callback get_id_function(void);
	c_callback get_type_function(void);
	c_callback_status get_wifi_connection_status_function(void);
	c_callback_status get_wifi_present_status_function(void);
	c_callback_read_sample_buffer get_read_sample_buffer_function(void);
	
};

/* Function prototypes ----------------------------------------------------- */
bool ei_command_line_handle(void);
void ei_start_stop_run_impulse(bool start);
bool ei_run_impulse_active(void);
void ei_printf(const char *format, ...);
void ei_printf_float(float f);
void ei_printfloat(int n_decimals, int n, ...);

void ei_write_string(char *data, int length);
void ei_putc(char cChar);

/* Reference to object for external usage ---------------------------------- */
extern EiDeviceSyntiant EiDevice;

#endif