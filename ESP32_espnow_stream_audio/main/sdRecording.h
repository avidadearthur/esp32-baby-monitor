#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"

#include <math.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "freertos/stream_buffer.h"

#define SPI_MOSI_GPIO 23
#define SPI_MISO_GPIO 19
#define SPI_CLK_GPIO 18
#define SPI_CS_GPIO 5

#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define SD_MOUNT_POINT "/sdcard"
#define SAMPLE_SIZE (BIT_SAMPLE * 1024)
#define BYTE_RATE (I2S_SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS

void init_recording_task(StreamBufferHandle_t xStreamBufferRecMic);

void init_reading_task(StreamBufferHandle_t xStreamBufferRead);

void suspend_recording();

void resume_recording();

void suspend_reading();

void resume_reding();
