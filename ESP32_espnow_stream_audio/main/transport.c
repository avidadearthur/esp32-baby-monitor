#include "color_out.h"
#include "transport.h"
#include "audio.h"
#include "sdRecording.h"

// static uint8_t esp_now_send_buf[250];

/* When passed to init_recording xStreamBuffer to handle the recording. This
functionality is just for testing purposes. */
static StreamBufferHandle_t network_stream_buf;

void sender_task(void *arg)
{
    /* xStreamBuffer to Receive data from audio capture task (tx_task) */
    uint8_t *ucRxData = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    size_t xReceivedBytes;
    const TickType_t xBlockTime = pdMS_TO_TICKS(20);

    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t)arg;
    /* ------------------------------------------------------------------ */

    /* xStreamBuffer to send data to audio recording task (rec_task) */
    size_t xBytesSent;

    /* ------------------------------------------------------------------ */

    color_printf(COLOR_PRINT_GREEN, "\t\tsender_task: starting to listen");

    while (1)
    {
        /* Receive up to another sizeof( ucRxData ) bytes from the stream buffer.
        Wait in the Blocked state (so not using any CPU processing time) for a
        maximum of 100ms for the full sizeof( ucRxData ) number of bytes to be
        available. */
        xReceivedBytes = xStreamBufferReceive(mic_stream_buf,
                                              (void *)ucRxData,
                                              READ_BUF_SIZE_BYTES * sizeof(char),
                                              xBlockTime);

        if (xReceivedBytes > 0)
        {
            /* TODO: Replace with espnow send functionality */

            /* Send (void *)ucRxData to the stream buffer, blocking for a maximum of 100ms to
            wait for enough space to be available in the stream buffer. */
            xBytesSent = xStreamBufferSend(network_stream_buf,
                                           (void *)ucRxData,
                                           READ_BUF_SIZE_BYTES * sizeof(char),
                                           xBlockTime);
            if (xBytesSent > 0)
            {
                // color_printf(COLOR_PRINT_RED, "\t\trx_task: sent %d bytes to recording task", xBytesSent);
            }
            /* ------------------------------------------------------------------ */
        }
    }
    // delete task after recording
    vTaskDelete(NULL);
}

void init_transport(StreamBufferHandle_t mic_stream_buf, StreamBufferHandle_t net_stream_buf)
{
    /* TODO: Add espnow initializations here */
    /* ------------------------------------------------------------------ */
    network_stream_buf = net_stream_buf;
    init_recording(network_stream_buf);

    xTaskCreate(sender_task, "sender_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, mic_stream_buf, 5, NULL);
}