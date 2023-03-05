#ifndef I2S_RECV_STD_CONFIG_H
#define I2S_RECV_STD_CONFIG_H

#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/stream_buffer.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"

#if (!CONFIG_IDF_TARGET_ESP32)
    #define EXAMPLE_STD_BCLK_IO1        GPIO_NUM_2      // I2S bit clock io number
    #define EXAMPLE_STD_WS_IO1          GPIO_NUM_3      // I2S word select io number
    #define EXAMPLE_STD_DOUT_IO1        GPIO_NUM_4      // I2S data out io number
    #define EXAMPLE_STD_DIN_IO1         GPIO_NUM_5      // I2S data in io number
    #define EXAMPLE_BUFF_SIZE               2048
    #define EXAMPLE_I2S_SAMPLE_RATE         16000
    #define EXAMPLE_I2S_CHANNEL_NUM         1
    #define BYTE_RATE                       (EXAMPLE_I2S_SAMPLE_RATE*16*EXAMPLE_I2S_CHANNEL_NUM/8)
#endif

static i2s_chan_handle_t                tx_chan;        // I2S tx channel handler

void i2s_recv_std_config(void);
void i2s_std_playback_task(void *task_param);

#endif