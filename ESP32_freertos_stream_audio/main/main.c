#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

#include "color_out.h"
#include "audio.h"

void rx_task(void *arg)
{
    uint8_t *ucRxData = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    size_t xReceivedBytes;
    const TickType_t xBlockTime = pdMS_TO_TICKS(20);

    StreamBufferHandle_t xStreamBuffer = (StreamBufferHandle_t)arg;

    color_printf(COLOR_PRINT_GREEN, "\t\ttx_task: starting to listen");

    while (1)
    {
        /* Receive up to another sizeof( ucRxData ) bytes from the stream buffer.
        Wait in the Blocked state (so not using any CPU processing time) for a
        maximum of 100ms for the full sizeof( ucRxData ) number of bytes to be
        available. */
        xReceivedBytes = xStreamBufferReceive(xStreamBuffer,
                                              (void *)ucRxData,
                                              sizeof(ucRxData),
                                              xBlockTime);

        if (xReceivedBytes > 0)
        {
            /* A ucRxData contains another xRecievedBytes bytes of data, which can
            be processed here.... */
            color_printf(COLOR_PRINT_GREEN, "\t\ttx_task: notify %d bytes received", xReceivedBytes);
            // print the received data
        }
        else
        {
            /* No data was received from the stream buffer. */
            color_printf(COLOR_PRINT_RED, "\t\ttx_task: notify no bytes received");
        }
    }
}

void app_main()
{

    color_printf(COLOR_PRINT_PURPLE, "start ESP32");
    // color_printf(COLOR_PRINT_PURPLE, "free DRAM %u IRAM %u", esp_get_free_heap_size(), xPortGetFreeHeapSizeTagged(MALLOC_CAP_32BIT));

    StreamBufferHandle_t xStreamBuffer;
    const size_t xStreamBufferSizeBytes = 512, xTriggerLevel = 1;

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
    xTaskCreate(rx_task, "rx_task", CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBuffer, 5, NULL);

    color_printf(COLOR_PRINT_PURPLE, "end of main");
}