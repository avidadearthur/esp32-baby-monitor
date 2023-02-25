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
#include "sdRecording.h"

/* Create a second xStreamBuffer to handle the audio capturing */
StreamBufferHandle_t xStreamBuffer;

/* Create a second xStreamBuffer to handle the recording. This
functionality is just for testing purposes. */
StreamBufferHandle_t xStreamBufferRec;

void rx_task(void *arg)
{
    /* xStreamBuffer to Receive data from audio capture task (tx_task) */
    uint8_t *ucRxData = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    size_t xReceivedBytes;
    const TickType_t xBlockTime = pdMS_TO_TICKS(20);

    StreamBufferHandle_t xStreamBuffer = (StreamBufferHandle_t)arg;
    /* ------------------------------------------------------------------ */

    /* xStreamBuffer to send data to audio recording task (rec_task) */
    size_t xBytesSent;

    /* ------------------------------------------------------------------ */

    color_printf(COLOR_PRINT_GREEN, "\t\trx_task: starting to listen");

    while (1)
    {
        /* Receive up to another sizeof( ucRxData ) bytes from the stream buffer.
        Wait in the Blocked state (so not using any CPU processing time) for a
        maximum of 100ms for the full sizeof( ucRxData ) number of bytes to be
        available. */
        xReceivedBytes = xStreamBufferReceive(xStreamBuffer,
                                              (void *)ucRxData,
                                              READ_BUF_SIZE_BYTES * sizeof(char),
                                              xBlockTime);

        if (xReceivedBytes > 0)
        {
            /* Send (void *)ucRxData to the stream buffer, blocking for a maximum of 100ms to
            wait for enough space to be available in the stream buffer. */
            xBytesSent = xStreamBufferSend(xStreamBufferRec,
                                           (void *)ucRxData,
                                           READ_BUF_SIZE_BYTES * sizeof(char),
                                           xBlockTime);
            if (xBytesSent > 0)
            {
                // color_printf(COLOR_PRINT_RED, "\t\trx_task: sent %d bytes to recording task", xBytesSent);
            }
        }
    }
    // delete task after recording
    vTaskDelete(NULL);
}

void app_main()
{

    color_printf(COLOR_PRINT_PURPLE, "start ESP32");
    // color_printf(COLOR_PRINT_PURPLE, "free DRAM %u IRAM %u", esp_get_free_heap_size(), xPortGetFreeHeapSizeTagged(MALLOC_CAP_32BIT));

    /* xStreamBuffer to capture data to mic cpture task () */

    const size_t xStreamBufferSizeBytes = 65536, xTriggerLevel = 1;

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
    /* ----------------------------------------------------------------------------------- */

    /* xStreamBuffer to send data to audio recording task (rec_task) */

    /* Create a second xStreamBuffer to handle the recording. This
    functionality is just for testing purposes. */
    xStreamBufferRec = xStreamBufferCreate(xStreamBufferSizeBytes,
                                           xTriggerLevel);
    if (xStreamBufferRec == NULL)
    {
        color_printf(COLOR_PRINT_RED, "app_main: fail to create xStreamBufferRec");
    }
    else
    {
        /* The stream buffer was created successfully and can now be used. */
        color_printf(COLOR_PRINT_PURPLE, "app_main: created xStreamBufferRec successfully");
    }
    /* ----------------------------------------------------------------------------------- */

    color_printf(COLOR_PRINT_PURPLE, "app_main: creating three tasks");
    init_audio(xStreamBuffer);
    init_recording(xStreamBufferRec);

    xTaskCreate(rx_task, "rx_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBuffer, 5, NULL);

    color_printf(COLOR_PRINT_PURPLE, "end of main");
}