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

static i2s_chan_handle_t                tx_chan;        // I2S tx channel handler

void i2s_recv_std_config(void);
void i2s_std_playback_task(void *task_param);

#endif