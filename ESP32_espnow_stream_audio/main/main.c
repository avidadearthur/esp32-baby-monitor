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
#include "transport.h"

/* Create a second xStreamBuffer to handle the audio capturing */
StreamBufferHandle_t xStreamBuffer;

/* Create a second xStreamBuffer to handle the recording. This
functionality is just for testing purposes. */
StreamBufferHandle_t xStreamBufferRec;

void app_main()
{

    color_printf(COLOR_PRINT_PURPLE, "start ESP32");
    // color_printf(COLOR_PRINT_PURPLE, "free DRAM %u IRAM %u", esp_get_free_heap_size(), xPortGetFreeHeapSizeTagged(MALLOC_CAP_32BIT));

    /* xStreamBuffer to capture data to mic cpture task () */

    const size_t xStreamBufferSizeBytes = 512, xTriggerLevel = 1;

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
    { /* The stream buffer was created successfully and can now be used. */
        color_printf(COLOR_PRINT_PURPLE, "app_main: created xStreamBufferRec successfully");
    }
    /* ----------------------------------------------------------------------------------- */

    color_printf(COLOR_PRINT_PURPLE, "app_main: initializing audio and transport");

    init_audio(xStreamBuffer);
    init_transport(xStreamBuffer, xStreamBufferRec);

    color_printf(COLOR_PRINT_PURPLE, "app_main: end of initialization");
}