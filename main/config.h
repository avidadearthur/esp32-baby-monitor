#ifndef CONFIG_H
#define CONFIG_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "esp_vfs_fat.h"
#include "esp_adc_cal.h"
#include "esp_partition.h"
#include "esp_rom_sys.h"
#include "esp_system.h"
#include "esp_err.h"
#include "errno.h"
#include "driver/spi_common.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include <math.h>
#include "nvs_flash.h"
#include "spi_flash_mmap.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include <time.h>

/** wifi configuration */
/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

/** set the broadcast addr */
#define IS_BROADCAST_ADDR(addr) (memcmp(addr, broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

/** i2s configuration */
// analog microphone Settings - ADC1_CHANNEL_7 is GPIO35
#define ADC1_TEST_CHANNEL (ADC1_CHANNEL_7)
// i2s mic and adc settings
#define V_REF   1100
//enable record sound and save in flash
#define RECORD_IN_FLASH_EN        (1)
//enable replay recorded sound in flash
#define REPLAY_FROM_FLASH_EN      (1)

//i2s number for interface channel (0 or 1, 0 for mic and 1 for speaker)
#define EXAMPLE_I2S_NUM           (0)
//i2s sample rate
#define EXAMPLE_I2S_SAMPLE_RATE   (16000)
//i2s data bits
#define EXAMPLE_I2S_SAMPLE_BITS   (16)
//enable display buffer for debug
#define EXAMPLE_I2S_BUF_DEBUG     (0)
//I2S read buffer length
#define EXAMPLE_I2S_READ_LEN      (16 * 1024)
//I2S data format
#define EXAMPLE_I2S_FORMAT        (I2S_CHANNEL_FMT_ONLY_RIGHT)
//I2S channel number
#define EXAMPLE_I2S_CHANNEL_NUM   ((EXAMPLE_I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))
//I2S built-in ADC unit
#define I2S_ADC_UNIT              ADC_UNIT_1
//I2S built-in ADC channel GPIO36
#define I2S_ADC_CHANNEL           ADC1_CHANNEL_0
// I2S byte rate is 16bit * 1ch * 16000Hz / 8bit = 32000
#define BYTE_RATE                 (EXAMPLE_I2S_CHANNEL_NUM * EXAMPLE_I2S_SAMPLE_RATE * EXAMPLE_I2S_SAMPLE_BITS / 8)
// SPI DMA channel
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
// define max read buffer size
#define READ_BUF_SIZE_BYTES       (250)


void i2s_common_config(void);
void init_config(void);

#endif