#include "nrf24.h"

typedef union
{
    uint8_t value[4];
    unsigned long now_time;
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
    uint8_t payload = sizeof(mydata.value);
    uint8_t channel = 28;
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
    uint32_t *buffer = (uint32_t *)malloc(10 * sizeof(uint32_t));
    buffer[0] = 0;

    while (1)
    {
        // When the program is received, the received data is output from the serial port
        if (Nrf24_dataReady(&dev))
        {
            Nrf24_getData(&dev, mydata.value);
            ESP_LOGI(pcTaskGetName(0), "Got data:%lu", mydata.now_time);

            // put mydata.now_time into buffer
            buffer[0] = mydata.now_time;
            // log buffer value
            ESP_LOGI(pcTaskGetName(0), "buffer[0]=%lu", buffer[0]);
            // Send and check bytes sent
            size_t bytes_sent = xStreamBufferSend(nrf_data_xStream, (void *)buffer, sizeof(uint32_t), portMAX_DELAY);
            if (bytes_sent != sizeof(uint32_t))
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
    uint8_t payload = sizeof(mydata.value);
    uint8_t channel = 28;
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

    while (1)
    {
        mydata.now_time = xTaskGetTickCount();
        Nrf24_send(&dev, mydata.value);
        vTaskDelay(1);
        ESP_LOGI(pcTaskGetName(0), "Wait for sending.....");
        if (Nrf24_isSend(&dev, 1000))
        {
            ESP_LOGI(pcTaskGetName(0), "Send success:%lu", mydata.now_time);
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
