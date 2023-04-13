#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "rom/uart.h"

#include "../../components/esp32-smbus/smbus.h"
#include "../../components/esp32-i2c-lcd1602/i2c-lcd1602.h"

// define to enable lcd and button interrupts
#define UI_CONNECTED 1

// LCD1602
#define LCD_NUM_ROWS 2
#define LCD_NUM_COLUMNS 16
#define LCD_NUM_VISIBLE_COLUMNS 16

// Undefine USE_STDIN if no stdin is available (e.g. no USB UART) - a fixed delay will occur instead of a wait for a keypress.
#define USE_STDIN 1
// #undef USE_STDIN

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN 0 // disabled
#define I2C_MASTER_RX_BUF_LEN 0 // disabled
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL

void i2c_master_init(void);
