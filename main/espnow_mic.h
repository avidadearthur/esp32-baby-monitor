#ifndef ESPNOW_MIC_H
#define ESPNOW_MIC_H

#include "config.h"

void i2s_adc_data_scale(uint8_t * des_buff, uint8_t* src_buff, uint32_t len);
void i2s_adc_capture_task(void* task_param);
void i2s_dac_playback_task(void* task_param);
esp_err_t init_audio_trans(StreamBufferHandle_t mic_stream_buf, StreamBufferHandle_t fft_audio_buf, StreamBufferHandle_t record_stream_buf);
esp_err_t init_audio_recv(StreamBufferHandle_t network_stream_buf);
void mic_disp_buf(uint8_t* buf, int length);

void suspend_adc_capture_task(void);
void resume_adc_capture_task(void);
TaskHandle_t get_adc_task_handle(void);

#endif