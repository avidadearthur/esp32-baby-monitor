#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "espnow_recv.h"


#define ESPNOW_MAXDELAY 512
#define ESPNOW_MAX_SEND_BYTE 250 // 250 max bytes per packet size. (16000*16/8 = 32000 bytes per second. 32000/250 = 128 transmissions per second is needed)

static const char *TAG = "espnow_recv";

static StreamBufferHandle_t network_stream_buf;

/** configuring i2s speaker pin for esp32 s3*/
void i2s_speaker_init(){
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX, // Only TX
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, //Interrupt level 1
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_pin_config_t pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 22,
        .data_in_num = 35
    };
    i2s_set_pin(I2S_NUM_0, &pin_config);
}
/* defining reciever task */
void espnow_recv_task(const uint8_t* mac_addr, const uint8_t* data, int len) {
    // params of espnow_recv_task is recieved from esp_now_send(mac_addr, buffer, len)
    if(xStreamBufferSend(network_stream_buf, data, len, portMAX_DELAY) != len){
        ESP_LOGE(TAG, "Failed to send data to network stream buffer: %d", errno);
    }else{
        ESP_LOGI(TAG, "Sent data to network stream buffer: %d", len);
    };
}


/* initialize reciever */
void init_recv(StreamBufferHandle_t net_stream_buf){
    printf("Init transport!\n");
    network_stream_buf = net_stream_buf;
}

