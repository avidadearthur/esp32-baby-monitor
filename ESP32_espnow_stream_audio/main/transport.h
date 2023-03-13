#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

/*espnow related*/
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"

#include "esp_wifi.h"
#include "esp_now.h"

#define WIFI_CHANNEL (12)
#define ESPNOW_WIFI_MODE (WIFI_MODE_STA)
#define ESPNOW_WIFI_IF (ESP_IF_WIFI_STA)

#define CONFIG_ESPNOW_PMK "pmk1234567890123"
#define CONFIG_ESPNOW_CHANNEL 1

void init_transport();

void init_sender(StreamBufferHandle_t xStreamBufferMic);

void suspend_sender();

void resume_sender();

void init_receiver(StreamBufferHandle_t xStreamBufferNet);

// void suspend_receiver();

// void resume_receiver();