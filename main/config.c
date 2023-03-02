#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "espnow_recv.h"

static const char* TAG = "espnow_mic";
static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* WiFi should start before using ESPNOW */
void espnow_wifi_init(void)
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

/* initialize nvm */
void init_non_volatile_storage(void) {
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        printf("Error initializing NVS\n");
    }
}

/**
 * @brief I2S config for using internal ADC and DAC
 * one time set up
 */
void i2s_common_config(void)
{
     int i2s_num = EXAMPLE_I2S_NUM;
     i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN, // master and rx for mic, tx for speaker, adc for internal adc
        .sample_rate =  EXAMPLE_I2S_SAMPLE_RATE, // 16KHz for adc
        .bits_per_sample = EXAMPLE_I2S_SAMPLE_BITS, // 16 bits for adc
        .communication_format = I2S_COMM_FORMAT_STAND_MSB, // standard format for adc
        .channel_format = EXAMPLE_I2S_FORMAT, // only right channel for adc
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_desc_num = 6, // number of dma descriptors, or count for adc
        .dma_frame_num = 256, // number of dma frames, or length for adc
        .use_apll = 1, // use apll for adc
        .tx_desc_auto_clear = false, // i2s auto clear tx descriptor on underflow
        .fixed_mclk = 0, // i2s fixed MLCK clock
     };
     //install and start i2s driver
     i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
     //init ADC pad
     i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL);
     //init DAC pad
     i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN); // enable both I2S built-in DAC channels L/R, maps to DAC channel 1 on GPIO25 & GPIO26
}

/* initialized espnow */
esp_err_t espnow_init(void){

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    // registration of receiving callback function
    // ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_task) );

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

void init_config(void){
    init_non_volatile_storage();
    espnow_wifi_init();
    espnow_init();
    i2s_common_config();
    esp_log_level_set("I2S", ESP_LOG_INFO);
}