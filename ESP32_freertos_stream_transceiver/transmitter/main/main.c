#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

#include "esp_log.h"
#include "esp_err.h"

#include "color_out.h"
#include "audio.h"

#include "mirf.h"

/** TODO: Move to separate file .h */
#define BYTE_RATE (I2S_SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS
/** --------------------------------*/

/** TODO: Move to separate file .h */
static const char *TAG = "audio_recorder_test";
/** ------------------------------------------------------------------*/

void rx_task(void *arg)
{
    /** TODO: nrf24 int */
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

    /** -------------------------------------------------------*/

    /** TODO: Move to a separate file ? */

    // Use POSIX and C standard library functions to work with files.
    int flash_wr_size = 0;
    int rec_time = 10; // seconds

    uint32_t flash_rec_size = BYTE_RATE * rec_time;

    // log the size of the recording
    ESP_LOGI(TAG, "Opening file - recording size: %d", flash_rec_size);

    /** -------------------------------------------------------*/

    // uint8_t *ucRxData = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    size_t xReceivedBytes;
    const TickType_t xBlockTime = pdMS_TO_TICKS(20);

    StreamBufferHandle_t xStreamBuffer = (StreamBufferHandle_t)arg;

    color_printf(COLOR_PRINT_GREEN, "\t\ttx_task: starting to listen");

    while (flash_wr_size < flash_rec_size)
    {
        /* Receive up to another sizeof( ucRxData ) bytes from the stream buffer.
        Wait in the Blocked state (so not using any CPU processing time) for a
        maximum of 100ms for the full sizeof( ucRxData ) number of bytes to be
        available. */
        xReceivedBytes = xStreamBufferReceive(xStreamBuffer,
                                              (void *)nrf_buff,
                                              payload * sizeof(char),
                                              xBlockTime);

        if (xReceivedBytes > 0)
        {
            /* A ucRxData contains another xRecievedBytes bytes of data, which can
            be processed here.... */
            Nrf24_send(&dev, nrf_buff);

            if (Nrf24_isSend(&dev, 1000))
            {
                flash_wr_size += xReceivedBytes;
                // Log the amount of bytes and the percentage of the recording
                ESP_LOGI(TAG, "Sent %d %d/%d bytes to peer - %d%%", xReceivedBytes, flash_wr_size, flash_rec_size, (flash_wr_size * 100) / flash_rec_size);
            }
            else
            {
                /* No data was sent */
                color_printf(COLOR_PRINT_RED, "\t\ttx_task: send failed");
            }
        }
        else
        {
            /* No data was received from the stream buffer. */
            color_printf(COLOR_PRINT_RED, "\t\ttx_task: notify no bytes received");
        }
    }
    ESP_LOGI(TAG, "Transmission done!");

    // delete task after recording
    vTaskDelete(NULL);
}
/** --------------------------------------------------------------------------------------------------*/

void app_main()
{

    color_printf(COLOR_PRINT_PURPLE, "start ESP32");
    // color_printf(COLOR_PRINT_PURPLE, "free DRAM %u IRAM %u", esp_get_free_heap_size(), xPortGetFreeHeapSizeTagged(MALLOC_CAP_32BIT));

    StreamBufferHandle_t xStreamBuffer;
    const size_t xStreamBufferSizeBytes = 65536, xTriggerLevel = 1;

    /* Create a stream buffer that can hold 100 bytes and uses the
     * functions defined using the sbSEND_COMPLETED() and
     * sbRECEIVE_COMPLETED() macros as send and receive completed
     * callback functions. The memory used to hold both the stream
     * buffer structure and the data in the stream buffer is
     * allocated dynamically. */
    xStreamBuffer = xStreamBufferCreate(xStreamBufferSizeBytes,
                                        xTriggerLevel);
    if (xStreamBuffer == NULL)
    {
        /* There was not enough heap memory space available to create the
        stream buffer. */
        color_printf(COLOR_PRINT_RED, "app_main: fail to create xStreamBuffer");
    }
    else
    {
        /* The stream buffer was created successfully and can now be used. */
        color_printf(COLOR_PRINT_PURPLE, "app_main: created xStreamBuffer successfully");
    }

    color_printf(COLOR_PRINT_PURPLE, "app_main: creating two tasks");
    init_audio(xStreamBuffer);
    xTaskCreate(rx_task, "rx_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBuffer, 5, NULL);

    color_printf(COLOR_PRINT_PURPLE, "end of main");
}