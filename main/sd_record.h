#ifndef SD_RECORD_H
#define SD_RECORD_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

/** TODO: Move to separate file .h */
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"

#include <math.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "config.h"

/** --------------------------------*/

/** TODO: Move to separate file .h */
#define SPI_MOSI_GPIO 23
#define SPI_MISO_GPIO 19
#define SPI_CLK_GPIO 18
#define SPI_CS_GPIO 5
/** --------------------------------*/

/** TODO: Move to separate file .h */
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define SD_MOUNT_POINT "/sdcard"
#define SAMPLE_SIZE (16 * 1024)
/** ------------------------------------------------------------------*/

void mount_sdcard(void);
void generate_wav_header(char *wav_header, uint32_t wav_size, uint32_t sample_rate);
void rec_and_read_task(void* task_param);
void init_recording(StreamBufferHandle_t net_stream_buf);

#endif