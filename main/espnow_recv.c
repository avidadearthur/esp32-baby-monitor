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
    if (len != 0){
        // params of espnow_recv_task is recieved from esp_now_send(mac_addr, buffer, len)
        // wait until the bytes to be sent is sample rate / 2 for each dac channel. to have an interger value of 250 bytes per packet size, 250 *80 = 20000 bytes, approx sample rate / 2
        if(xStreamBufferSend(network_stream_buf, data, len * sizeof(char), portMAX_DELAY) != len * sizeof(char)){
            exit(errno);
        }
        #if ESPNOW_RECV_DEBUG
        recv_disp_buf((uint8_t*)data, len);
        #endif

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

/** debug functions below */

/**
 * @brief debug buffer data
 */
void recv_disp_buf(uint8_t* buf, int length)
{
#if EXAMPLE_I2S_BUF_DEBUG
    ESP_LOGI(TAG,"\n=== RECV ===\n");
    for (int i = 0; i < length; i++) {
        ESP_LOGI(TAG,"%02x ", buf[i]);
        if ((i + 1) % 8 == 0) {
            ESP_LOGI(TAG,"\n");
        }
    }
    ESP_LOGI(TAG,"\n=== RECV ===\n");
#endif
}

