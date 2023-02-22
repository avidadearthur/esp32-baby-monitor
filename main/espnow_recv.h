#include "esp_now.h"

#ifndef ESPNOW_RECV_H
#define ESPNOW_RECV_H

#define IS_BROADCAST_ADDR(addr) (memcmp(addr, broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

static void espnow_wifi_init(void);
static void espnow_send_task(void* task_param);
static void init_non_volatile_storage(void);
static esp_err_t espnow_init(void);
void init_recv(StreamBufferHandle_t network_stream_buf);

#endif
