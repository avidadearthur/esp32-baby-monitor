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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "mirf.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"

#include "freertos/queue.h"

#define V_REF 1100
#define I2S_COMM_MODE 0 // ADC/DAC Mode
#define I2S_SAMPLE_RATE 44100
#define I2S_SAMPLE_BITS (16)
#define I2S_BUF_DEBUG (0)      // enable display buffer for debug
#define I2S_READ_LEN (16 * 32) // I2S read buffer length
#define I2S_FORMAT (I2S_CHANNEL_FMT_ONLY_RIGHT)
#define I2S_CHANNEL_NUM (0)            // I2S channel number
#define I2S_ADC_UNIT ADC_UNIT_1        // I2S built-in ADC unit
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0 // I2S built-in ADC channel GPIO36
#define BIT_SAMPLE (16)
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define NUM_CHANNELS (1) // For mono recording only!
#define SAMPLE_SIZE (BIT_SAMPLE * 32)
#define BYTE_RATE (I2S_SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS

// xQueueHandle samples_queue;
xQueueHandle demo_queue;

// Create a mutex to protect the queue
SemaphoreHandle_t queueMutex;

/**
 * @brief I2S ADC mode init.
 */
void init_microphone(void)
{
    int i2s_num = I2S_COMM_MODE;
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN,
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_SAMPLE_BITS,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .channel_format = I2S_FORMAT,
        .intr_alloc_flags = 0,
        .dma_buf_count = 6,
        .dma_buf_len = 256,
        .use_apll = 1,
    };
    // Call driver installation function and adc pad.
    ESP_ERROR_CHECK(i2s_driver_install(i2s_num, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL));
}

/**
 * @brief Scale data to 8bit for data from ADC.
 *        Data from ADC are 12bit width by default.
 * @param d_buff: destination buffer
 * @param s_buff: source buffer
 * @param len: length of source buffer
 */
void i2s_adc_data_scale(uint8_t *d_buff, uint8_t *s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
#if (I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i += 2)
    {
        dac_value = ((((uint16_t)(s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
#else
    for (int i = 0; i < len; i += 4)
    {
        dac_value = ((((uint16_t)(s_buff[i + 3] & 0xf) << 8) | ((s_buff[i + 2]))));
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
#endif
}
/**
 * @brief Uses i2s_read function to read data from ADC, and scale data to 8bit then put it at the end of the queue.
 */
void audio_read()
{
    // static int16_t i2s_readraw_buff[SAMPLE_SIZE]; // I2S read buffer, contains acctually 12-bit data
    // uint8_t i2s_scaled_buff[SAMPLE_SIZE];         // I2S buffer scaled from 12 to 8-bit
    // size_t bytes_read;

    // i2s_adc_enable(I2S_CHANNEL_NUM);
    // init an uint8_t buff with 32 uint8_t elements
    uint8_t txpos[] = {99, 2, 3, 4, 9, 2, 3, 4, 99, 2, 3, 4, 99, 2, 3, 4, 99, 2, 3, 4, 99, 2, 3, 4, 99, 2, 3, 4, 99, 2, 3, 4}; //

    // change to a certain limited amount of time
    while (1)
    {
        // Read the RAW samples from the microphone
        // Read data from I2S bus, in this case, from ADC. //
        // i2s_read(I2S_CHANNEL_NUM, (void *)i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 100);
        // process data and scale to 8bit for I2S DAC.
        // i2s_adc_data_scale(i2s_scaled_buff, (uint8_t *)i2s_readraw_buff, SAMPLE_SIZE);

        xSemaphoreTake(queueMutex, portMAX_DELAY);
        if (xQueueSendToBack(demo_queue, &txpos, 6000 / portTICK_RATE_MS) != pdTRUE)
        {
            ESP_LOGI(pcTaskGetName(0), "Failed to queue sample");
        }
        else
        {
            ESP_LOGI(pcTaskGetName(0), "Queued sample");
        }
        xSemaphoreGive(queueMutex);
    }
}

void transmitter(void *pvParameters)
{
    ESP_LOGI(pcTaskGetName(0), "Start");

    NRF24_t dev;
    Nrf24_init(&dev);
    uint8_t payload = 32;
    uint8_t channel = 90;
    Nrf24_config(&dev, channel, payload);

    uint8_t *nrf_buff = (uint8_t *)calloc(payload, sizeof(uint8_t)); // NRF24L01 buffer

    // Set the receiver address using 5 characters
    esp_err_t ret = Nrf24_setTADDR(&dev, (uint8_t *)"FGHIJ");
    if (ret != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(0), "nrf24l01 not installed");
        while (1)
        {
            vTaskDelay(1);
        }
    }

#if CONFIG_ADVANCED
    AdvancedSettings(&dev);
#endif // CONFIG_ADVANCED

    // Print settings
    // Nrf24_printDetails(&dev);

    // Start transmission
    while (1)
    {
        xSemaphoreTake(queueMutex, portMAX_DELAY);
        if (xQueueReceive(demo_queue, &nrf_buff, 6000 / portTICK_RATE_MS) != pdTRUE)
        {
            ESP_LOGI(pcTaskGetName(0), "Failed to receive queued value");
        }
        else
        {
            ESP_LOGI(pcTaskGetName(0), "Received queued value");
            Nrf24_send(&dev, nrf_buff);
            // print the data
            for (int i = 0; i < payload; i++)
            {
                ESP_LOGI(pcTaskGetName(0), "nrf_buff[%d] = %d", i, nrf_buff[i]);
            }

            if (Nrf24_isSend(&dev, 1000))
            {

                ESP_LOGI(pcTaskGetName(0), "Sending audio data ...");
            }
            else
            {
                ESP_LOGI(pcTaskGetName(0), "Sending audio failed ...");
            }
        }
        xSemaphoreGive(queueMutex);
    }
}

void app_main(void)
{
    // init_microphone();
    // samples_queue = xQueueCreate(1000, sizeof(uint8_t));
    demo_queue = xQueueCreate(128, sizeof(uint8_t));
    queueMutex = xSemaphoreCreateMutex();
    xTaskCreate(audio_read, "audio_read", 1024 * 3, NULL, 2, NULL);
    xTaskCreate(transmitter, "transmitter", 1024 * 3, NULL, 2, NULL);
}