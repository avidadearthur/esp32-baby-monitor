#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

#include "driver/i2s.h"
#include "soc/i2s_reg.h"

#include "color_out.h"

// mic I2S constants for capturing audio
#define V_REF 1100

#define I2S_COMM_MODE 0 // ADC/DAC Mode
#define I2S_SAMPLE_RATE 44100
#define I2S_SAMPLE_BITS 16
#define I2S_BUF_DEBUG 0       // enable display buffer for debug
#define I2S_READ_LEN 16 * 128 // I2S read buffer length
#define I2S_FORMAT I2S_CHANNEL_FMT_ONLY_RIGHT
#define I2S_CHANNEL_NUM 0              // I2S channel number
#define I2S_ADC_UNIT ADC_UNIT_1        // I2S built-in ADC unit
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0 // I2S built-in ADC channel GPIO36

#define NUM_CHANNELS 1 // For mono recording only!

#define BIT_SAMPLE 16
#define READ_BUF_SIZE_BYTES 16384

/**
 * @brief This implementation looks correct for scaling 12-bit data to 8-bit data.
 * The implementation first extracts the 12-bit value from the input buffer,
 * and then scales it down to an 8-bit value before storing it in the output buffer.
 *
 * One thing to note is that this implementation assumes that the input buffer
 * is an array of 8-bit values with 12-bit data stored in the lower bits of each byte.
 * This is because of the bitwise AND operation used to extract the lower 4 bits of
 * the second byte in each sample.
 *
 */
void i2s_adc_data_scale(uint8_t *d_buff, uint8_t *s_buff, uint32_t len);

void tx_task(void *arg);

void init_audio(StreamBufferHandle_t xStreamBuffer);