#include "nrf24.h"
// include random number generator
#include "esp_random.h"
#include <math.h>
typedef struct
{
    uint8_t data[3];
    int now_time;
} MYDATA_t;

MYDATA_t mydata;

static StreamBufferHandle_t nrf_data_xStream = NULL;

#if CONFIG_ADVANCED
void AdvancedSettings(NRF24_t *dev)
{
#if CONFIG_RF_RATIO_2M
    ESP_LOGW(pcTaskGetName(0), "Set RF Data Ratio to 2MBps");
    Nrf24_SetSpeedDataRates(dev, 1);
#endif // CONFIG_RF_RATIO_2M

#if CONFIG_RF_RATIO_1M
    ESP_LOGW(pcTaskGetName(0), "Set RF Data Ratio to 1MBps");
    Nrf24_SetSpeedDataRates(dev, 0);
#endif // CONFIG_RF_RATIO_2M

#if CONFIG_RF_RATIO_250K
    ESP_LOGW(pcTaskGetName(0), "Set RF Data Ratio to 250KBps");
    Nrf24_SetSpeedDataRates(dev, 2);
#endif // CONFIG_RF_RATIO_2M

    ESP_LOGW(pcTaskGetName(0), "CONFIG_RETRANSMIT_DELAY=%d", CONFIG_RETRANSMIT_DELAY);
    Nrf24_setRetransmitDelay(dev, CONFIG_RETRANSMIT_DELAY);
}
#endif // CONFIG_ADVANCED

#if CONFIG_RECEIVER
void receiver(void *xStream)
{
    // check if the stream buffer was passed successfully
    if (xStream == NULL)
    {
        ESP_LOGE(pcTaskGetName(0), "Error receiving stream buffer");
    }

    ESP_LOGI(pcTaskGetName(0), "Start");
    NRF24_t dev;
    Nrf24_init(&dev);
    uint8_t payload = sizeof(mydata.data);
    uint8_t channel = 28;
    uint8_t pa_high = 2;
    Nrf24_SetOutputRF_PWR(&dev, pa_high);
    Nrf24_config(&dev, channel, payload);

    // Set the receiver address using 5 characters
    /// ABCDE --> 0x41, 0x42, 0x43, 0x44, 0x45
    esp_err_t ret = Nrf24_setTADDR(&dev, (uint8_t *)"ABCDE");
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
    ESP_LOGI(pcTaskGetName(0), "Listening...");

    // Create buffer for mydata.now_time
    // uint32_t *buffer = (uint32_t *)malloc(sizeof(uint32_t));
    // buffer[0] = 0;

    // Create buffer for mydata data
    uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * 3);

    while (1)
    {
        // When the program is received, the received data is output from the serial port
        if (Nrf24_dataReady(&dev))
        {
            Nrf24_getData(&dev, mydata.data);
            mydata.now_time = xTaskGetTickCount();

            // log the data
            ESP_LOGI(pcTaskGetName(0), "Data: %d, %d, %d", mydata.data[0], mydata.data[1], mydata.data[2]);

            size_t bytes_sent = xStreamBufferSend(nrf_data_xStream, (void *)mydata.data, sizeof(mydata.data), portMAX_DELAY);
            if (bytes_sent != sizeof(mydata.data))
            {
                ESP_LOGE(pcTaskGetName(0), "Error sending data to stream buffer");
            }
        }
        vTaskDelay(1);
    }
}
#endif // CONFIG_RECEIVER

#if CONFIG_TRANSMITTER
void transmitter(void *pvParameters)
{
    ESP_LOGI(pcTaskGetName(0), "Start");
    NRF24_t dev;
    Nrf24_init(&dev);
    uint8_t payload = sizeof(mydata.data);
    uint8_t channel = 28;
    Nrf24_config(&dev, channel, payload);
    uint8_t pa_high = 2;
    Nrf24_SetOutputRF_PWR(&dev, pa_high);

    // Set the receiver address using 5 characters
    /// ABCDE --> 0x41, 0x42, 0x43, 0x44, 0x45
    esp_err_t ret = Nrf24_setTADDR(&dev, (uint8_t *)"ABCDE");
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

    float temp = 0.0;
    uint8_t status = 0;
    int16_t temp_int = 0;
    uint16_t temp_combined = 0;
    float temp_float = 0.0;
    int now_time = 0;

    while (1)
    {
        now_time = xTaskGetTickCount();
        mydata.now_time = now_time;

        // uncomment for testing
        // Nrf24_send(&dev, mydata.value);

        // Send random fake temperature data between 20.5 and 25.0 degrees
        temp = 20.5 + (float)(rand() % 50) / 10.0;

        // Send a random status that can only be 0 or 1
        status = rand() % 2;

        // Convert the temperature to an integer
        temp_int = (int16_t)(temp * 10.0);

        // Copy the temperature and status values to the data array
        mydata.data[0] = (uint8_t)(temp_int & 0xFF);        // lower byte of temperature
        mydata.data[1] = (uint8_t)((temp_int >> 8) & 0xFF); // upper byte of temperature
        mydata.data[2] = status;

        // Combine the upper and lower bytes of the temperature into a 16-bit integer
        temp_combined = ((uint16_t)mydata.data[1] << 8) | mydata.data[0];

        // Convert the combined temperature back to a float
        temp_float = ((float)temp_combined) / 10.0;

        // Send the data
        Nrf24_send(&dev, mydata.data);

        vTaskDelay(1);
        ESP_LOGI(pcTaskGetName(0), "Wait for sending.....");
        if (Nrf24_isSend(&dev, 1000))
        {
            // Log the data to be sent
            ESP_LOGI(pcTaskGetName(0), "Sending data: %d: %.1f °C, baby_status: %d", mydata.now_time, temp_float, mydata.data[2]);
        }
        else
        {
            ESP_LOGW(pcTaskGetName(0), "Send fail:");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#endif // CONFIG_TRANSMITTER

void init_nrf24(StreamBufferHandle_t xStream)
{
    // check if the stream buffer was passed successfully
    if (xStream == NULL)
    {
        ESP_LOGE(pcTaskGetName(0), "init_nrf24 - Error receiving stream buffer");
    }
    // cast the void pointer to a StreamBufferHandle_t
    nrf_data_xStream = (StreamBufferHandle_t)xStream;

    xTaskCreate(receiver, "receiver", 1024 * 3, NULL, 2, NULL);
}
