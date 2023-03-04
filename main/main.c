#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "espnow_mic.h"
#include "espnow_send.h"
#include "espnow_recv.h"

static StreamBufferHandle_t mic_stream_buf;
static StreamBufferHandle_t network_stream_buf; // only for reciever

void app_main(void) {
    // deafult transmission rate of esp_now_send is 1Mbps = 125KBps, stream buffer size has to be larger than 125KBps
    mic_stream_buf = xStreamBufferCreate(BYTE_RATE*3, 1);
    network_stream_buf = xStreamBufferCreate(BYTE_RATE*3, 1);
    
    // initialize espnow, nvm, wifi, and i2s configuration
    init_config();

    // initialize the transmitter and audio
    init_transmit(mic_stream_buf);
    init_audio_trans(mic_stream_buf);

    // initialize the reciever and audio (only for reciever)
    // init_recv(network_stream_buf);
    // init_audio_recv(network_stream_buf);
    

    // while (true) {
    //     printf("Hello world!\n");
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}
