#ifndef ESPNOW_MIC_H
#define ESPNOW_MIC_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

void i2s_mic_config(void);
void adc_cali_read_task(void* task_param);
void i2s_adc_capture_task(void* task_param);
esp_err_t i2s_mic_init (void);
esp_err_t init_audio(StreamBufferHandle_t mic_stream_buf);
#endif