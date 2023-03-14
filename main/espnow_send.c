#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "espnow_send.h"
#include "time.h"

#define ESPNOW_MAXDELAY 512
#define ESPNOW_MAX_SEND_BYTE 250 // 250 max bytes per packet size. (16000*12/8 = 24000 bytes per second. 24000/250 = 96 transmissions per second is needed)

static const char *TAG = "espnow_send";

static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t esp_now_send_buf[ESPNOW_MAX_SEND_BYTE];

/* sender task definition */
void espnow_send_task(void* task_param) {

    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t) task_param;

    // for debugging, assert the size of the esp now send buffer is equal to the max send byte
    assert(sizeof(esp_now_send_buf) == ESPNOW_MAX_SEND_BYTE*sizeof(char));

    #if EXAMPLE_I2S_BUF_DEBUG
        // create a timer to coutn the time elapsed
        time_t start_time = time(NULL);
        // declare a counter to count the packets sent
        int packet_count = 0;
        int packet_loss = 0;
    #endif

    while (true) {

        // read from the mic stream buffer until it is empty
        size_t num_bytes = xStreamBufferReceive(mic_stream_buf, esp_now_send_buf, READ_BUF_SIZE_BYTES, portMAX_DELAY);
        if (num_bytes > 0) {
            esp_err_t err = esp_now_send(broadcast_mac, esp_now_send_buf, READ_BUF_SIZE_BYTES);
            
            #if EXAMPLE_I2S_BUF_DEBUG
                if (err != ESP_OK || num_bytes != READ_BUF_SIZE_BYTES) {
                    packet_loss++;
                }else{
                    packet_count++;
                    // send_disp_buf((uint8_t*)esp_now_send_buf, READ_BUF_SIZE_BYTES);
                }
            #endif
        }
        else if (num_bytes == 0) {
            printf("Error reading from mic stream buffer: %d\n", errno);
            ESP_LOGE(TAG, "No data in stream buffer");
        }
        #if EXAMPLE_I2S_BUF_DEBUG
            // check if the timer has reached 10 second
            if (time(NULL) - start_time >= 60*10) {
                // print the number of packets sent and loss in the last second
                printf("Packets sent in last 10 second: %d \n", packet_count);
                printf("Packets lost in last 10 second: %d \n", packet_loss);
                // reset the packet count
                packet_count = 0;
                packet_loss = 0;
                // reset the timer
                start_time = time(NULL);
            }
        #endif
    }
}

/* sender initialization */
void init_transmit(StreamBufferHandle_t mic_stream_buf){
    printf("Init transport!\n");
    xTaskCreate(espnow_send_task, "espnow_send_task", 4096, (void*) mic_stream_buf, 4, NULL); // create another thread to send data
}

/** debug functions below */

/**
 * @brief debug buffer data
 */
void send_disp_buf(uint8_t* buf, int length)
{
#if EXAMPLE_I2S_BUF_DEBUG
    printf("\n=== SEND ===\n");
    for (int i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n=== SEND ===\n");
#endif
}


