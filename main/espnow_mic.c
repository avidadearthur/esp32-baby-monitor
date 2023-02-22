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
static uint8_t audio_output_buf[READ_BUF_SIZE_BYTES];

/**
 * @brief I2S config for using internal ADC and DAC
 * one time set up
 */
void i2s_common_config(void)
{
     int i2s_num = EXAMPLE_I2S_NUM;
     i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN, // master and rx for mic, tx for speaker, adc for internal adc
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
     //init DAC pad
     i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN); // enable both I2S built-in DAC channels L/R
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
        // read from i2s bus and use errno to check if i2s_read is successful
        if (i2s_read(EXAMPLE_I2S_NUM, (char*) mic_read_buf, READ_BUF_SIZE_BYTES, &bytes_read, ticks_to_wait) != ESP_OK) {
            ESP_LOGE(TAG, "Error reading from i2s adc: %d", errno);
        }else{
            ESP_LOGI(TAG, "Read %d bytes from i2s adc", bytes_read);
        }
        // xstreambuffersend is a blocking function that sends data to the stream buffer, , use errno to check if xstreambuffersend is successful
        if (xStreamBufferSend(mic_stream_buf, mic_read_buf, READ_BUF_SIZE_BYTES, portMAX_DELAY) != bytes_read) {
            ESP_LOGE(TAG, "Error sending to mic_stream_buf: %d", errno);
        }else{
            ESP_LOGI(TAG, "Sent %d bytes to mic_stream_buf", bytes_read);
        }
    }
}

// /**
//  * @brief Scale data to 8bit for data from ADC.
//  *        Data from ADC are 12bit width by default.
//  *        DAC can only output 8 bit data.
//  *        Scale each 12bit ADC data to 8bit DAC data.
//  */
// void i2s_adc_data_scale(uint8_t * des_buff, uint8_t* src_buff, uint32_t len)
// {
//     uint32_t j = 0;
//     uint32_t dac_value = 0;
// #if (EXAMPLE_I2S_SAMPLE_BITS == 16)
//     for (int i = 0; i < len; i += 2) {
//         dac_value = ((((uint16_t) (src_buff[i + 1] & 0xf) << 8) | ((src_buff[i + 0]))));
//         des_buff[j++] = 0;
//         des_buff[j++] = dac_value * 256 / 4096;
//     }
// #else
//     for (int i = 0; i < len; i += 4) {
//         dac_value = ((((uint16_t)(src_buff[i + 3] & 0xf) << 8) | ((src_buff[i + 2]))));
//         des_buff[j++] = 0;
//         des_buff[j++] = 0;
//         des_buff[j++] = 0;
//         des_buff[j++] = dac_value * 256 / 4096;
//     }
// #endif
// }

// i2s dac playback task
void i2s_dac_playback_task(void* task_param) {
    StreamBufferHandle_t net_stream_buf = (StreamBufferHandle_t)task_param;
    size_t bytes_written = 0;

    while (true) {
        size_t num_bytes = xStreamBufferReceive(net_stream_buf, (void*)audio_output_buf,sizeof(audio_output_buf), portMAX_DELAY);
        if (num_bytes > 0) {
            // //process data and scale to 8bit for I2S DAC.
            // i2s_adc_data_scale(audio_output_buf, audio_output_buf, num_bytes);
            // send data to i2s dac
            esp_err_t err = i2s_write(EXAMPLE_I2S_NUM, audio_output_buf, num_bytes, &bytes_written, portMAX_DELAY);
            if (err != ESP_OK) {
                printf("Error writing I2S: %0x\n", err);
            }
        }
    }
}

/* initialize configuration of mic -first */
esp_err_t i2s_audio_init (void){
    i2s_common_config();
    esp_log_level_set("I2S", ESP_LOG_INFO);
    return ESP_OK;
}

/* call the init_auidio function for starting adc and filling the buf -second */
esp_err_t init_audio(StreamBufferHandle_t mic_stream_buf, StreamBufferHandle_t network_stream_buf){ 
    printf("initializing i2s mic\n");
    i2s_audio_init();

    /* thread for adc and filling the buf for the transmitter */
    xTaskCreate(i2s_adc_capture_task, "i2s_adc_capture_task", 4096, (void*) mic_stream_buf, 4, NULL); 
    /* thread for filling the buf for the reciever and dac */
    xTaskCreate(i2s_dac_playback_task, "i2s_dac_playback_task", 4096, (void*) network_stream_buf, 4, NULL);
    /* adc analog voltage calibration */
    xTaskCreate(adc_cali_read_task, "adc_cali_read_task", 4096, NULL, 4, NULL);

    return ESP_OK;
}





    