#ifndef MUSIC_H
#define MUSIC_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spi_flash_mmap.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "esp_rom_sys.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "config.h"

void example_reset_play_mode(void);
void example_set_file_play_mode(void);
int example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len);
void music_task(void*arg);
esp_err_t init_music(void);


#endif