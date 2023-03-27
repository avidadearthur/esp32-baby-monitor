#include "config.h"
#include "ui.h"
#include "nrf24.h"
#include "wifi_station.h"

static StreamBufferHandle_t nrf_data_xStream;
static const char *TAG = "main.c";

void app_main(void)
{

#if CONFIG_RECEIVER
	const size_t xStreamBufferSizeBytes = 100, xTriggerLevel = 1;

	nrf_data_xStream = xStreamBufferCreate(xStreamBufferSizeBytes, xTriggerLevel);
	// check if the stream buffer was created successfully
	if (nrf_data_xStream == NULL)
	{
		ESP_LOGE(TAG, "main.c - Error creating stream buffer");
	}

	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	wifi_init_sta();

	/*------------------------Datetime init-------------------------------*/
	// Set the system timezone
	setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
	tzset();

	// Connect to the internet and synchronize the time with an NTP server
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();
	/*------------------------Datetime init-------------------------------*/

	init_nrf24(nrf_data_xStream);

	init_ui(nrf_data_xStream);

#endif

#if CONFIG_TRANSMITTER
	xTaskCreate(transmitter, "TRANSMITTER", 1024 * 3, NULL, 2, NULL);
#endif
}
