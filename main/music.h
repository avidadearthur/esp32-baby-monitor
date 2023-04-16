#ifndef MUSIC_H
#define MUSIC_H

#include "config.h"

void example_reset_play_mode(void);
void example_set_file_play_mode(void);
int example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len);
void music_task(void* task_param);
esp_err_t init_music(TaskHandle_t fft_task_handle);
TaskHandle_t get_music_play_task_handle(void);


#endif