#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "espnow_mic.h"
#include "espnow_send.h"
#include "espnow_recv.h"

static StreamBufferHandle_t mic_stream_buf;
static StreamBufferHandle_t network_stream_buf; // only for reciever

void app_main(void) {
    mic_stream_buf = xStreamBufferCreate(512, 1);

    init_transmit(mic_stream_buf);
    init_audio(mic_stream_buf, network_stream_buf);
    // init_recv(network_stream_buf);
    
    // Loopback testing
    // init_audio(mic_stream_buf, mic_stream_buf);

    while (true) {
        printf("Hello world!\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
