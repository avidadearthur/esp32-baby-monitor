#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "espnow_recv.h"


#define ESPNOW_MAXDELAY 512
#define ESPNOW_MAX_SEND_BYTE 250 // 250 max bytes per packet size. (16000*16/8 = 32000 bytes per second. 32000/250 = 128 transmissions per second is needed)

static const char *TAG = "espnow_recv";

StreamBufferHandle_t network_stream_buf;


/* defining reciever task */
void espnow_recv_task(const uint8_t* mac_addr, const uint8_t* data, int len) {
    // params of espnow_recv_task is recieved from esp_now_send(mac_addr, buffer, len)
    if(xStreamBufferSend(network_stream_buf, data, len, portMAX_DELAY) != len){
        ESP_LOGE(TAG, "Failed to send data to network stream buffer: %d", errno);
        exit(errno);
    }
}


/* initialize reciever */
void init_recv(StreamBufferHandle_t net_stream_buf){
    printf("Init recieve!\n");
    network_stream_buf = net_stream_buf;
    if (network_stream_buf == NULL) {
        ESP_LOGE(TAG, "Error creating network stream buffer: %d", errno);
        deinit_config();
        exit(errno);
    }
}

