#include "config.h"
#include "ui.h"
#include "nrf24.h"

void app_main(void)
{
	xTaskCreate(init_display, "i2c_lcd1602_task", 2048, NULL, 10, NULL);

	xTaskCreate(init_buttons, "nit_buttons", 2048, NULL, 10, NULL);

#if CONFIG_RECEIVER
	xTaskCreate(receiver, "RECEIVER", 1024 * 3, NULL, 2, NULL);
#endif

#if CONFIG_TRANSMITTER
	xTaskCreate(transmitter, "TRANSMITTER", 1024 * 3, NULL, 2, NULL);
#endif
}
