#include "config.h"
#include "ui.h"
#include "nrf24.h"

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

	init_nrf24(nrf_data_xStream);

	init_ui(nrf_data_xStream);

#endif

#if CONFIG_TRANSMITTER
	xTaskCreate(transmitter, "TRANSMITTER", 1024 * 3, NULL, 2, NULL);
#endif
}
