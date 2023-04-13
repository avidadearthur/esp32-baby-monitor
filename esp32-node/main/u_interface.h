#include "config.h"
#include "esp_sntp.h"
#include <time.h>
#include "freertos/queue.h"
#include "wifi_station.h"

void init_u_interface(StreamBufferHandle_t xStream);