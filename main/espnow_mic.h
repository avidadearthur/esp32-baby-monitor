#ifndef ESPNOW_MIC_H
#define ESPNOW_MIC_H

#include "config.h"

void i2s_adc_data_scale(uint8_t * des_buff, uint8_t* src_buff, uint32_t len);
void i2s_adc_capture_task(void* task_param);
void i2s_dac_playback_task(void* task_param);
esp_err_t init_audio(StreamBufferHandle_t mic_stream_buf, StreamBufferHandle_t network_stream_buf);

#endif