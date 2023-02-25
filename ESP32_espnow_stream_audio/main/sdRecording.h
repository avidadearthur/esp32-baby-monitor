#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

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

#include "freertos/stream_buffer.h"
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
#define SAMPLE_SIZE (BIT_SAMPLE * 1024)
#define BYTE_RATE (I2S_SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS
/** ------------------------------------------------------------------*/

void init_recording(StreamBufferHandle_t xStreamBufferRec);