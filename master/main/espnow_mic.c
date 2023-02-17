#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "esp_adc_cal.h"
#include "esp_partition.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "spi_flash_mmap.h"
#include "espnow_mic.h"
#include "errno.h"

static const char* TAG = "espnow_mic";

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
// I2S byte rate
#define BYTE_RATE                 (EXAMPLE_I2S_CHANNEL_NUM * EXAMPLE_I2S_SAMPLE_RATE * EXAMPLE_I2S_SAMPLE_BITS / 8)
// SPI DMA channel
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
// define max read buffer size
#define READ_BUF_SIZE_BYTES       (250)

static uint8_t mic_read_buf[READ_BUF_SIZE_BYTES];

/**
 * @brief I2S config for using internal ADC and DAC
 * one time set up
 */
void i2s_mic_config(void)
{
     int i2s_num = EXAMPLE_I2S_NUM;
     i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_ADC_BUILT_IN, // master and rx for mic, tx for speaker, adc for internal adc
        .sample_rate =  EXAMPLE_I2S_SAMPLE_RATE, // 16KHz for adc
        .bits_per_sample = EXAMPLE_I2S_SAMPLE_BITS, // 16 bits for adc
        .communication_format = I2S_COMM_FORMAT_STAND_MSB, // standard format for adc
        .channel_format = EXAMPLE_I2S_FORMAT, // only right channel for adc
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_desc_num = 6, // number of dma descriptors, or count for adc
        .dma_frame_num = 256, // number of dma frames, or length for adc
        .use_apll = 1, // meaning using ethernet data interface framework
        .tx_desc_auto_clear = false, // i2s auto clear tx descriptor on underflow
        .fixed_mclk = 0, // i2s fixed MLCK clock
     };
     //install and start i2s driver
     i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
     //init ADC pad
     i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL);
}

// adc reading calibration task
void adc_cali_read_task(void* task_param)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_TEST_CHANNEL, ADC_ATTEN_DB_11);
    esp_adc_cal_characteristics_t characteristics;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, V_REF, &characteristics);
    while(true) {
        uint32_t voltage;
        vTaskDelay(200 / portTICK_PERIOD_MS);
        // use errno to check if the adc_calibrate function is successful
        if (esp_adc_cal_get_voltage(ADC1_TEST_CHANNEL, &characteristics, &voltage) == ESP_OK) {
            ESP_LOGI(TAG, "%"PRIu32" mV", voltage);
        }else{
            ESP_LOGE(TAG, "adc_calibrate failed");
        }
    }
}

// i2s adc capture task
void i2s_adc_capture_task(void* task_param)
{
    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t) task_param;
    i2s_adc_enable(EXAMPLE_I2S_NUM);
    size_t bytes_read = 0; // to count the number of bytes read from the i2s adc
    TickType_t ticks_to_wait = 100; // wait 100 ticks for the mic_stream_buf to be available
    // iteratively read READ_BUF_SIZE_BYTES bytes from i2s adc
    while(true){
        // read from i2s adc and use errno to check if i2s_read is successful
        if (i2s_read(EXAMPLE_I2S_NUM, (void*) mic_read_buf, READ_BUF_SIZE_BYTES, &bytes_read, portMAX_DELAY) != ESP_OK) {
            ESP_LOGE(TAG, "Error reading from i2s adc: %d", errno);
        }
        // xstreambuffersend is a blocking function that sends data to the stream buffer, , use errno to check if xstreambuffersend is successful
        if (xStreamBufferSend(mic_stream_buf, (void*) mic_read_buf, bytes_read, ticks_to_wait) != bytes_read) {
            ESP_LOGE(TAG, "Error sending to mic_stream_buf: %d", errno);
        }
    }
}


esp_err_t i2s_mic_init (void){ // call the initialization function for configuring the I2S -first
    i2s_mic_config();
    esp_log_level_set("I2S", ESP_LOG_INFO);
    return ESP_OK;
}

esp_err_t init_audio(StreamBufferHandle_t mic_stream_buf){ // call the start function for starting adc and filling the buf -second
    printf("initializing i2s mic\n");
    i2s_mic_init();
    xTaskCreate(i2s_adc_capture_task, "i2s_adc_capture_task", 4096, (void*) mic_stream_buf, 4, NULL); // thread for adc and filling the buf
    xTaskCreate(adc_cali_read_task, "ADC read task", 4096, (void*) NULL, 4, NULL); // thread for adc reading calibration
    return ESP_OK;
}





    