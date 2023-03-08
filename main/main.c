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
#if (!RECV)
    mic_stream_buf = xStreamBufferCreate(EXAMPLE_I2S_READ_LEN, READ_BUF_SIZE_BYTES);
    // check if the stream buffer is created
    if (mic_stream_buf == NULL) {
        printf("Error creating mic stream buffer: %d\n", errno);
        deinit_config();
        exit(errno);
    }
#else
    network_stream_buf = xStreamBufferCreate(BYTE_RATE, EXAMPLE_I2S_READ_LEN);
    // check if the stream buffer is created
    if (network_stream_buf == NULL) {
        printf("Error creating network stream buffer: %d\n", errno);
        deinit_config();
        exit(errno);
    }
#endif
    
#if (RECV)
    // initialize the reciever and audio (only for reciever)
    init_recv(network_stream_buf);
    init_audio_recv(network_stream_buf);
#endif

    // initialize espnow, nvm, wifi, and i2s configuration
    init_config();

#if (!RECV)
    // initialize the transmitter and audio
    init_transmit(mic_stream_buf);
    init_audio_trans(mic_stream_buf);
#endif
    
}
