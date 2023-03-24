#include "config.h"
#include "ui.h"
#include "nrf24.h"

void app_main(void)
{

#if CONFIG_RECEIVER
	const size_t xStreamBufferSizeBytes = 100, xTriggerLevel = 1;

	static StreamBufferHandle_t nrf_data_xStream;
	nrf_data_xStream = xStreamBufferCreate(xStreamBufferSizeBytes, xTriggerLevel);

	xTaskCreate(receiver, "RECEIVER", 1024 * 3, NULL, 2, nrf_data_xStream);

	xTaskCreate(init_display, "i2c_lcd1602_task", 2048, NULL, 10, nrf_data_xStream);

	xTaskCreate(init_buttons, "nit_buttons", 2048, NULL, 10, NULL);
#endif

#if CONFIG_TRANSMITTER
	xTaskCreate(transmitter, "TRANSMITTER", 1024 * 3, NULL, 2, NULL);
#endif
}
