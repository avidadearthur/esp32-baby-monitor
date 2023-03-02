
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

static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t esp_now_send_buf[ESPNOW_MAX_SEND_BYTE];

// define a variable to hold the packet count
volatile uint32_t packet_count = 0;

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

/* create a timer callback function to reset the packet count every second */
void packet_count_reset_callback(TimerHandle_t xTimer)
{
    packet_count = 0;
}

// create a timer callback function to output the packet count every second
void packet_count_output_callback(TimerHandle_t xTimer)
{
    ESP_LOGE(TAG, "Packet Count: %d", packet_count);
}


/* sender task definition */
static void espnow_send_task(void* task_param) {
    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t) task_param;

    // create a timer to reset the packet count every second
    TimerHandle_t packet_count_reset_timer = xTimerCreate("Packet Count Reset Timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, packet_count_reset_callback);
    xTimerStart(packet_count_reset_timer, 0);

    while (true) {
        // read from the mic stream buffer and check with errno
        size_t num_bytes = xStreamBufferReceive(mic_stream_buf, (void*) esp_now_send_buf, sizeof(esp_now_send_buf), portMAX_DELAY);
        printf("sizeof(esp_now_send_buf) = %d, strlen(esp_now_send_buf) = %d, ESP_NOW_MAX_SEND_BYTE*sizeof(char) = %d \n", sizeof(esp_now_send_buf), strlen(esp_now_send_buf), ESP_NOW_MAX_SEND_BYTE*sizeof(char));
        if (num_bytes > 0) {
            esp_err_t err = esp_now_send(broadcast_mac, esp_now_send_buf, sizeof(esp_now_send_buf));
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error sending ESP NOW packet: %x\n", err);
            }else{
                // increment the packet count
                packet_count++;
                ESP_LOGE(TAG, "esp_now_send success");
                // create a timer to output the packet count every second
                TimerHandle_t packet_count_output_timer = xTimerCreate("Packet Count Output Timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, packet_count_output_callback);
                xTimerStart(packet_count_output_timer, 0);
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
    }
}



/* initialize nvm */
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

/* initialized espnow */
static esp_err_t espnow_init(void){

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

/* sender initialization */
void init_transmit(StreamBufferHandle_t mic_stream_buf){
    printf("Init transport!\n");
    init_non_volatile_storage();
    espnow_wifi_init();
    espnow_init();
    xTaskCreate(espnow_send_task, "espnow_send_task", 4096, (void*) mic_stream_buf, 4, NULL); // create another thread to send data
}


