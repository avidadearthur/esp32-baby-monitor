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

#define ESP_NOW_TRANSPORT (false)
#define REC_NETWORK (false)

#define REC_MIC (true)
#define REC_MIC_AND_PLAY (false)

#define READ_FROM_SD_AND_SEND_TO_DSP (false)
#define READ_FROM_SD_AND_PLAY (false)

/* Create a xStreamBuffers to handle the audio capturing and transmission */
static StreamBufferHandle_t xStreamBufferMic;
static StreamBufferHandle_t xStreamBufferNet;

/* Create a second xStreamBuffer to handle the recording. This
functionality is just for testing purposes. */
static StreamBufferHandle_t xStreamBufferRecMic;
static StreamBufferHandle_t xStreamBufferRecNet;

static StreamBufferHandle_t xStreamBufferRead;

void app_main()
{
    color_printf(COLOR_PRINT_PURPLE, "start ESP32");

    xStreamBufferMic = NULL;
    xStreamBufferNet = NULL;
    xStreamBufferRecMic = NULL;
    xStreamBufferRecNet = NULL;
    xStreamBufferRead = NULL;

    const size_t xStreamBufferSizeBytes = 65536, xTriggerLevel = 1;

    /* xStreamBuffer to capture data to mic capture task () */
    xStreamBufferMic = xStreamBufferCreate(xStreamBufferSizeBytes,
                                           xTriggerLevel);
    if (xStreamBufferMic == NULL)
    {
        /* There was not enough heap memory space available to create the
        stream buffer. */
        color_printf(COLOR_PRINT_RED, "app_main: fail to create xStreamBufferMic");
    }
    else
    {
        /* The stream buffer was created successfully and can now be used. */
        color_printf(COLOR_PRINT_PURPLE, "app_main: created xStreamBufferMic successfully");
    }
    /* ----------------------------------------------------------------------------------- */

#if ESP_NOW_TRANSPORT
    /* Second xStreamBuffer to handle the network transmission.*/
    xStreamBufferNet = xStreamBufferCreate(xStreamBufferSizeBytes,
                                           xTriggerLevel);
    if (xStreamBufferNet == NULL)
    {
        color_printf(COLOR_PRINT_RED, "app_main: fail to create xStreamBufferNet");
    }
    else
    { /* The stream buffer was created successfully and can now be used. */
        color_printf(COLOR_PRINT_PURPLE, "app_main: created xStreamBufferNet successfully");
    }
    /* ----------------------------------------------------------------------------------- */
#endif

    /* Third xStreamBuffer to send data to audio recording task (rec_task) */
#if REC_MIC
    /* Create a second xStreamBuffer to handle the recording. This
    functionality is just for testing purposes. */
    xStreamBufferRecMic = xStreamBufferCreate(xStreamBufferSizeBytes,
                                              xTriggerLevel);
    if (xStreamBufferRecMic == NULL)
    {
        color_printf(COLOR_PRINT_RED, "app_main: fail to create xStreamBufferMIC");
    }
    else
    { /* The stream buffer was created successfully and can now be used. */
        color_printf(COLOR_PRINT_PURPLE, "app_main: created xStreamBufferMic successfully");
    }
    /* ----------------------------------------------------------------------------------- */
#endif

#if REC_NETWORK
    /* This is used to record incomming audio from the Network */
    xStreamBufferRecNet = xStreamBufferCreate(xStreamBufferSizeBytes,
                                              xTriggerLevel);
    if (xStreamBufferRecNet == NULL)
    {
        color_printf(COLOR_PRINT_RED, "app_main: fail to create xStreamBufferNet");
    }
    else
    { /* The stream buffer was created successfully and can now be used. */
        color_printf(COLOR_PRINT_PURPLE, "app_main: created xStreamBufferNet successfully");
    }
#endif

    color_printf(COLOR_PRINT_PURPLE, "app_main: initializing functions");
    /* This will initialize both I2S built-in ADC/DAC.
    This joint initialization is necessary because only I2S built-in ADC/DAC support on I2S0
    This is also the reason why we can't have silmutaneous audio capture and playback on I2S0*/
    i2s_init();

    // the xStreamBufferMic will be read on the sender task and
    // the xStreamBufferRecMic will be read on the recording task
    init_audio_capture_task(NULL, xStreamBufferRecMic); // since I haven't initialized the sender I'm passing NULL

    /* this task may get the Mic stream buffer or the network stream buffer */
#if REC_MIC
    init_recording_task(xStreamBufferRecMic);
#endif

#if REC_NETWORK
    init_recording_task(xStreamBufferRecNet);
#endif
    /* --------------------------------------------------------------------- */

#if READ_FROM_SD_AND_SEND_TO_DSP
    color_printf(COLOR_PRINT_PURPLE, "app_main: READ_FROM_SD_AND_SEND_TO_DSP selected");

    xStreamBufferRead = xStreamBufferCreate(xStreamBufferSizeBytes,
                                            xTriggerLevel);
    if (xStreamBufferRead == NULL)
    {
        color_printf(COLOR_PRINT_RED, "app_main: fail to create xStreamBufferRead");
    }
    else
    { /* The stream buffer was created successfully and can now be used. */
        color_printf(COLOR_PRINT_PURPLE, "app_main: created xStreamBufferRead successfully");
    }

    init_reading_task(xStreamBufferRead);
    init_dsp(xStreamBufferRead);
#endif

    // init_transport(xStreamBufferMic, xStreamBufferNet);

    color_printf(COLOR_PRINT_PURPLE, "app_main: end of initialization");
}