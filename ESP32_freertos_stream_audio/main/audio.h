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
#define I2S_SAMPLE_RATE 16000
#define I2S_SAMPLE_BITS 16
#define I2S_BUF_DEBUG 0 // enable display buffer for debugs
#define I2S_FORMAT I2S_CHANNEL_FMT_ONLY_RIGHT
#define I2S_CHANNEL_NUM 0              // I2S channel number
#define I2S_ADC_UNIT ADC_UNIT_1        // I2S built-in ADC unit
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0 // I2S built-in ADC channel GPIO36

#define NUM_CHANNELS 1 // For mono recording only!

#define BIT_SAMPLE 16
#define READ_BUF_SIZE_BYTES 250

void suspend_audio_capture();

void resume_audio_capture();

void suspend_audio_playback();

void resume_audio_playback();

void init_audio(StreamBufferHandle_t xStreamBuffer, StreamBufferHandle_t network_stream_buf);