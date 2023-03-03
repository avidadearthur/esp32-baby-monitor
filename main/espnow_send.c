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
    // assert(sizeof(esp_now_send_buf) == ESPNOW_MAX_SEND_BYTE*sizeof(char));

    // create a timer to coutn the time elapsed
    time_t start_time = time(NULL);

    // declare a counter to count the packets sent
    int packet_count = 0;
    int packet_loss = 0;

    while (true) {
        // create a local timer to

        // read from the mic stream buffer until it is empty
        size_t num_bytes = xStreamBufferReceive(mic_stream_buf, esp_now_send_buf, sizeof(esp_now_send_buf), portMAX_DELAY);
        if (num_bytes > 0) {
            esp_err_t err = esp_now_send(broadcast_mac, esp_now_send_buf, sizeof(esp_now_send_buf));
            if (err != ESP_OK) {
                // ESP_LOGE(TAG, "Error sending ESP NOW packet: %x\n", err);
                packet_loss++;
                /* Delay a while before sending the next data. */
                // vTaskDelay(CONFIG_ESPNOW_SEND_DELAY/(5*portTICK_PERIOD_MS));
                // exit(err);
            }else{
                packet_count++;
                // ESP_LOGI(TAG, "Read from mic stream buffer: %d", num_bytes);
            }
        }
        else if (num_bytes == 0) {
            printf("Error reading from mic stream buffer: %d\n", errno);
            ESP_LOGE(TAG, "No data in m");
        }
        else {
            printf("Other error reading from mic stream: %d\n", errno);
            // exit with error code and error message
            exit(errno);
        }
        // check if the timer has reached 1 second
        if (time(NULL) - start_time >= 1) {
            // print the number of packets sent and loss in the last second
            printf("Packets sent in last second: %d \n", packet_count);
            printf("Packets lost in last second: %d \n", packet_loss);
            // reset the packet count
            packet_count = 0;
            packet_loss = 0;
            // reset the timer
            start_time = time(NULL);
        }
    }
}

/* sender initialization */
void init_transmit(StreamBufferHandle_t mic_stream_buf){
    printf("Init transport!\n");
    xTaskCreate(espnow_send_task, "espnow_send_task", 4096, (void*) mic_stream_buf, 4, NULL); // create another thread to send data
}


