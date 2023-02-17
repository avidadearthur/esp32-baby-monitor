
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/stream_buffer.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "espnow_send.h"
#include "errno.h"


#define ESPNOW_MAXDELAY 512
#define ESPNOW_MAX_SEND_BYTE 250 // 250 max bytes per packet size. (16000*16/8 = 32000 bytes per second. 32000/250 = 128 transmissions per second is needed)

static const char *TAG = "espnow_send";

// static QueueHandle_t espnow_queue;

static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
// static uint16_t espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };
static uint8_t esp_now_send_buf[ESPNOW_MAX_SEND_BYTE];

// static void example_espnow_deinit(example_espnow_send_param_t *send_param);

/* WiFi should start before using ESPNOW */
static void espnow_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

static void espnow_send_task(void* task_param) {
    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t) task_param;

    while (true) {
        // read from the mic stream buffer and check with errno
        size_t num_bytes = xStreamBufferReceive(mic_stream_buf, esp_now_send_buf, ESPNOW_MAX_SEND_BYTE, 0);
        if (num_bytes > 0) {
            esp_err_t err = esp_now_send(broadcast_mac, esp_now_send_buf, sizeof(esp_now_send_buf));
            if (err != ESP_OK) {
                printf("Error sending ESP NOW packet: %x\n", err);
            }
        }
        else if (num_bytes == 0) {
            printf("No data in mic stream buffer\n");
        }
        else {
            printf("Error reading from mic stream buffer: %d\n", errno);
            // exit with error code and error message
            exit(errno);
        }
    }
}

static void init_non_volatile_storage(void) {
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        printf("Error initializing NVS\n");
    }
}

static esp_err_t espnow_init(void){

    // creat a queue for the espnow
    // espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    // if (espnow_queue == NULL) {
    //     ESP_LOGE(TAG, "Create mutex fail");
    //     return ESP_FAIL;
    // }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    // ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
#if CONFIG_ESP_WIFI_STA_DISCONNECTED_PM_ENABLE
    ESP_ERROR_CHECK( esp_now_set_wake_window(65535) );
#endif
    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t* peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        // vSemaphoreDelete(espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    return ESP_OK;
}

void init_transport(StreamBufferHandle_t mic_stream_buf){
    printf("Init transport!\n");
    init_non_volatile_storage();
    espnow_wifi_init();
    espnow_init();
    xTaskCreate(espnow_send_task, "espnow_send_task", 4096, (void*) mic_stream_buf, 4, NULL); // create another thread to send data
}


// /** TODO: defining the content of the send and recieve task -- add our code here */
// static void espnow_task(void *pvParameter)
// {
//     example_espnow_event_t evt;
//     uint8_t recv_state = 0;
//     uint16_t recv_seq = 0;
//     int recv_magic = 0;
//     bool is_broadcast = false;
//     int ret;

//     vTaskDelay(5000 / portTICK_PERIOD_MS);
//     ESP_LOGI(TAG, "Start sending broadcast data");

//     /* Start sending broadcast ESPNOW data. */
//     example_espnow_send_param_t *send_param = (example_espnow_send_param_t *)pvParameter;
//     if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
//         ESP_LOGE(TAG, "Send error");
//         example_espnow_deinit(send_param);
//         vTaskDelete(NULL);
//     }

//     while (xQueueReceive(espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
//         switch (evt.id) {
//             case ESPNOW_SEND_CB:
//             {
//                 example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
//                 is_broadcast = IS_BROADCAST_ADDR(send_cb->mac_addr);

//                 ESP_LOGD(TAG, "Send data to "MACSTR", status1: %d", MAC2STR(send_cb->mac_addr), send_cb->status);

//                 if (is_broadcast && (send_param->broadcast == false)) {
//                     break;
//                 }

//                 if (!is_broadcast) {
//                     send_param->count--;
//                     if (send_param->count == 0) {
//                         ESP_LOGI(TAG, "Send done");
//                         example_espnow_deinit(send_param);
//                         vTaskDelete(NULL);
//                     }
//                 }

//                 /* Delay a while before sending the next data. */
//                 if (send_param->delay > 0) {
//                     vTaskDelay(send_param->delay/portTICK_PERIOD_MS);
//                 }

//                 ESP_LOGI(TAG, "send data to "MACSTR"", MAC2STR(send_cb->mac_addr));

//                 memcpy(send_param->dest_mac, send_cb->mac_addr, ESP_NOW_ETH_ALEN);
//                 espnow_data_prepare(send_param);

//                 /* Send the next data after the previous data is sent. */
//                 if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
//                     ESP_LOGE(TAG, "Send error");
//                     example_espnow_deinit(send_param);
//                     vTaskDelete(NULL);
//                 }
//                 break;
//             }
//             default:
//                 ESP_LOGE(TAG, "Callback type error: %d", evt.id);
//                 break;
//         }
//     }
// }

// /* ESPNOW sending or receiving callback function is called in WiFi task.
//  * Users should not do lengthy operations from this task. Instead, post
//  * necessary data to a queue and handle it from a lower priority task. */

// // espnow send callback function
// static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
// {
//     example_espnow_event_t evt;
//     example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

//     if (mac_addr == NULL) {
//         ESP_LOGE(TAG, "Send cb arg error");
//         return;
//     }

//     evt.id = ESPNOW_SEND_CB;
//     memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
//     send_cb->status = status;
//     if (xQueueSend(espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {  /* xQueueSend maps standard queue.h API functions to the MPU equivalents. */
//         ESP_LOGW(TAG, "Send send queue fail");
//     }
// }

// /* Prepare ESPNOW data to be sent. */
// // prepare the format of the data to be sent, called only one time
// void espnow_data_prepare(example_espnow_send_param_t *send_param)
// {
//     // make a pointer to the send_param->buffer
//     example_espnow_data_t *buf = (example_espnow_data_t *)send_param->buffer;

//     assert(send_param->len >= sizeof(example_espnow_data_t));

//     buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
//     buf->state = send_param->state;
//     buf->seq_num = espnow_seq[buf->type]++;
//     buf->crc = 0;
//     buf->magic = send_param->magic;
//     // fill all remaining bytes after the data with zeros
//     memset(buf->data, 0, send_param->len - sizeof(example_espnow_data_t));
//     // calculate the crc
//     buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
// }


// static esp_err_t espnow_init(void){
//     //example_espnow_send_param_t *send_param;

//     // creat a queue for the espnow
//     espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
//     if (espnow_queue == NULL) {
//         ESP_LOGE(TAG, "Create mutex fail");
//         return ESP_FAIL;
//     }

//     /* Initialize ESPNOW and register sending and receiving callback function. */
//     ESP_ERROR_CHECK( esp_now_init() );
//     ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
// #if CONFIG_ESP_WIFI_STA_DISCONNECTED_PM_ENABLE
//     ESP_ERROR_CHECK( esp_now_set_wake_window(65535) );
// #endif
//     /* Set primary master key. */
//     ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

//     /* Add broadcast peer information to peer list. */
//     esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
//     if (peer == NULL) {
//         ESP_LOGE(TAG, "Malloc peer information fail");
//         vSemaphoreDelete(espnow_queue);
//         esp_now_deinit();
//         return ESP_FAIL;
//     }
//     memset(peer, 0, sizeof(esp_now_peer_info_t));
//     peer->channel = CONFIG_ESPNOW_CHANNEL;
//     peer->ifidx = ESPNOW_WIFI_IF;
//     peer->encrypt = false;
//     memcpy(peer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
//     ESP_ERROR_CHECK( esp_now_add_peer(peer) );
//     free(peer);

//     // /* Initialize sending parameters. malloced */
//     // send_param = malloc(sizeof(example_espnow_send_param_t));
//     // if (send_param == NULL) {
//     //     ESP_LOGE(TAG, "Malloc send parameter fail");
//     //     vSemaphoreDelete(espnow_queue);
//     //     esp_now_deinit();
//     //     return ESP_FAIL;
//     // }

//     // /** TODO: params to be set to determine braodcase, unitcast, and magic number */
//     // memset(send_param, 0, sizeof(example_espnow_send_param_t));
//     // send_param->unicast = false;
//     // send_param->broadcast = true;
//     // send_param->state = 0;
//     // send_param->magic = esp_random(); 
//     // send_param->count = CONFIG_ESPNOW_SEND_COUNT; //from esp32 menuconfig
//     // send_param->delay = CONFIG_ESPNOW_SEND_DELAY; //from esp32 menuconfig
//     // send_param->len = CONFIG_ESPNOW_SEND_LEN; //from esp32 menuconfig
//     // // malloc the buffer
//     // send_param->buffer = malloc(CONFIG_ESPNOW_SEND_LEN); //from esp32 menuconfig
//     // if (send_param->buffer == NULL) {
//     //     ESP_LOGE(TAG, "Malloc send buffer fail");
//     //     free(send_param);
//     //     vSemaphoreDelete(espnow_queue);
//     //     esp_now_deinit();
//     //     return ESP_FAIL;
//     // }
//     // memcpy(send_param->dest_mac, broadcast_mac, ESP_NOW_ETH_ALEN);
//     // espnow_data_prepare(send_param);

//     // xTaskCreate(espnow_task, "espnow_task", 2048, send_param, 4, NULL); // create another thread to send data

//     return ESP_OK;
// }

// static void example_espnow_deinit(example_espnow_send_param_t *send_param)
// {
//     free(send_param->buffer);
//     free(send_param);
//     // vSemaphoreDelete(espnow_queue);
//     esp_now_deinit();
// }