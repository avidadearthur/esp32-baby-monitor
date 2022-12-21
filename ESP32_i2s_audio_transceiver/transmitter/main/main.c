/* Mirf Example
     This example code is in the Public Domain (or CC0 licensed, at your option.)
     Unless required by applicable law or agreed to in writing, this
     software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
     CONDITIONS OF ANY KIND, either express or implied.
*/
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
#define V_REF 1100
#define I2S_COMM_MODE 0 // ADC/DAC Mode
#define I2S_SAMPLE_RATE 44100
#define I2S_SAMPLE_BITS (16)
#define I2S_BUF_DEBUG (0)        // enable display buffer for debug
#define I2S_READ_LEN (16 * 1024) // I2S read buffer length
#define I2S_FORMAT (I2S_CHANNEL_FMT_ONLY_RIGHT)
#define I2S_CHANNEL_NUM (0)            // I2S channel number
#define I2S_ADC_UNIT ADC_UNIT_1        // I2S built-in ADC unit
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0 // I2S built-in ADC channel GPIO36
#define BIT_SAMPLE (16)
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define NUM_CHANNELS (1) // For mono recording only!
#define SAMPLE_SIZE (BIT_SAMPLE * 1024)
#define BYTE_RATE (I2S_SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS

int i2s_read_len = (12);
int i2s_write_len = (8);

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
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
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
#if CONFIG_TRANSMITTER
void transmitter(void *pvParameters)
{
    size_t bytes_read;

    ESP_LOGI(pcTaskGetName(0), "Start");

    char *i2s_read_buff = (char *)calloc(i2s_read_len, sizeof(char));
    uint8_t *i2s_write_buff = (uint8_t *)calloc(i2s_write_len, sizeof(char));

    i2s_adc_enable(I2S_CHANNEL_NUM);

    NRF24_t dev;
    NRF24_t *nrf24 = &dev;
    Nrf24_init(&dev);
    uint8_t payload = sizeof(i2s_write_buff);
    uint8_t channel = 90;
    Nrf24_config(&dev, channel, payload);

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
    Nrf24_printDetails(&dev);
    // Start ADC
    while (1)
    {
        // Read data from I2S bus, in this case, from ADC. //
        i2s_read(I2S_CHANNEL_NUM, (void *)i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        // process data and scale to 8bit for I2S DAC.
        i2s_adc_data_scale(i2s_write_buff, (uint8_t *)i2s_read_buff, i2s_read_len);
        // i2s_write_buff needs to be the buffer that is sent via nr24l01.

        // multiply the content of the buffer by 10 to make it more audible.
        for (int i = 0; i < i2s_write_len; i++)
        {
            i2s_write_buff[i] *= 10;
        }

        Nrf24_send(&dev, i2s_write_buff);

        if (Nrf24_isSend(&dev, 1000))
        {
            ESP_LOGI(pcTaskGetName(0), "sending audio data ...");
            Nrf24_print_status(Nrf24_getStatus(nrf24));
        }
        else
        {
            ESP_LOGI(pcTaskGetName(0), "sending audio failed ...");
            Nrf24_print_status(Nrf24_getStatus(nrf24));
        }
    }
}
#endif // CONFIG_TRANSMITTER

void app_main(void)
{
    // I2S ADC mode microphone init.
    init_microphone();
#if CONFIG_TRANSMITTER
    xTaskCreate(transmitter, "TRANSMITTER", 1024 * 3, NULL, 2, NULL);
#endif
    // Stop I2S driver and destroy
    // ESP_ERROR_CHECK(i2s_driver_uninstall(I2S_COMM_MODE));
}