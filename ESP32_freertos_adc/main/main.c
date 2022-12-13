/**
 * Based on: https://embeddedexplorer.com/esp32-adc-esp-idf-tutorial/
 * This script reads and logs the ADC value measured from GPIO34 avery 100ms.
 * GPIO34 corresponds to ADC1 Channel 6.
 */

#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

static const char *TAG = "ADC EXAMPLE";
static esp_adc_cal_characteristics_t adc1_chars;
void app_main(void)
{
    uint32_t voltage;
    uint32_t digital_read;
    // ADC1 for 11dB attenuation
    // By default, ADC1 use 12-bit.
    // Due to variation in internal reference voltage of different ESP32 chips
    // the ESP32 ADCs needs to be calibrated before using.
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11));
    while (1)
    {
        digital_read = adc1_get_raw(ADC1_CHANNEL_6);
        voltage = esp_adc_cal_raw_to_voltage(digital_read, &adc1_chars);
        ESP_LOGI(TAG, "ADC1_CHANNEL_6: %d, %d mV", digital_read, voltage);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}