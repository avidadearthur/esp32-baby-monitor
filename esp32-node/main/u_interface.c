#include "u_interface.h"
#include "config.h"
#include "driver/gpio.h"

#define SET_BUTTON_GPIO 33
#define CANCEL_BUTTON_GPIO 32
#define UP_BUTTON_GPIO 35
#define DOWN_BUTTON_GPIO 34

#define BUTTON_DEBOUNCE_TIME_MS 300

typedef enum
{
    DISPLAY_HOME_STATE,
    DISPLAY_EXTREME_TEMP,

    DISPLAY_MAX_TEMP_THRESHOLD,
    SET_MAX_TEMP_THRESHOLD,
    INCREASE_MAX_TEMP_THRESHOLD,
    DECREASE_MAX_TEMP_THRESHOLD,

    DISPLAY_MIN_TEMP_THRESHOLD,
    SET_MIN_TEMP_THRESHOLD,
    INCREASE_MIN_TEMP_THRESHOLD,
    DECREASE_MIN_TEMP_THRESHOLD,

    DISPLAY_MUSIC_STATE,
    SET_MUSIC_STATE,
    SET_MUSIC_CONFIRM_STATE
} fsm_state_t;

fsm_state_t current_state = DISPLAY_HOME_STATE;

SemaphoreHandle_t xSemaphore;

bool interrupt_flag = false;
uint32_t last_button_isr_time = 0;

static const char *TAG = "u_interface.c";

/* User defined global constants */
// define temp threshold as global variable
float temp_threshold_min = 22.0;
float temp_threshold_max = 24.0;

// for the changing state of the threshold
float new_temp_threshold_max = 0.0;
float new_temp_threshold_min = 0.0;

// define current max and min temp as global variable
float current_max_temp = 0.0;
float current_min_temp = 0.0;

// define music state as global variable
bool music_state = false;

// lcd related
smbus_info_t *smbus_info = NULL;
i2c_lcd1602_info_t *lcd_info = NULL;

void IRAM_ATTR up_button_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t isr_time = xTaskGetTickCountFromISR();

    if ((isr_time - last_button_isr_time) > pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS))
    {
        if (current_state == DISPLAY_HOME_STATE)
        {
            current_state = DISPLAY_EXTREME_TEMP;
        }
        else if (current_state == DISPLAY_EXTREME_TEMP)
        {
            current_state = DISPLAY_MAX_TEMP_THRESHOLD;
        }
        else if (current_state == DISPLAY_MAX_TEMP_THRESHOLD)
        {
            current_state = DISPLAY_MIN_TEMP_THRESHOLD;
        }
        else if (current_state == DISPLAY_MIN_TEMP_THRESHOLD)
        {
            current_state = DISPLAY_MUSIC_STATE;
        }

        else if (current_state == SET_MAX_TEMP_THRESHOLD)
        {
            current_state = INCREASE_MAX_TEMP_THRESHOLD;
        }
        else if (current_state == DECREASE_MAX_TEMP_THRESHOLD)
        {
            current_state = INCREASE_MAX_TEMP_THRESHOLD;
        }

        else if (current_state == SET_MIN_TEMP_THRESHOLD)
        {
            current_state = INCREASE_MIN_TEMP_THRESHOLD;
        }
        else if (current_state == DECREASE_MIN_TEMP_THRESHOLD)
        {
            current_state = INCREASE_MIN_TEMP_THRESHOLD;
        }

        xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        last_button_isr_time = isr_time; // Save last ISR time
    }
}

void IRAM_ATTR down_button_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t isr_time = xTaskGetTickCountFromISR();

    if ((isr_time - last_button_isr_time) > pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS))
    {
        if (current_state == DISPLAY_MUSIC_STATE)
        {
            current_state = DISPLAY_MIN_TEMP_THRESHOLD;
        }
        else if (current_state == DISPLAY_MIN_TEMP_THRESHOLD)
        {
            current_state = DISPLAY_MAX_TEMP_THRESHOLD;
        }
        else if (current_state == DISPLAY_MAX_TEMP_THRESHOLD)
        {
            current_state = DISPLAY_EXTREME_TEMP;
        }
        else if (current_state == DISPLAY_EXTREME_TEMP)
        {
            current_state = DISPLAY_HOME_STATE;
        }

        else if (current_state == SET_MAX_TEMP_THRESHOLD)
        {
            current_state = DECREASE_MAX_TEMP_THRESHOLD;
        }
        else if (current_state == INCREASE_MAX_TEMP_THRESHOLD)
        {
            current_state = DECREASE_MAX_TEMP_THRESHOLD;
        }

        else if (current_state == SET_MIN_TEMP_THRESHOLD)
        {
            current_state = DECREASE_MIN_TEMP_THRESHOLD;
        }
        else if (current_state == INCREASE_MIN_TEMP_THRESHOLD)
        {
            current_state = DECREASE_MIN_TEMP_THRESHOLD;
        }

        xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        last_button_isr_time = isr_time; // Save last ISR time
    }
}

void IRAM_ATTR set_button_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t isr_time = xTaskGetTickCountFromISR();

    if ((isr_time - last_button_isr_time) > pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS))
    {
        if (current_state == DISPLAY_MAX_TEMP_THRESHOLD) // enter temp setting mode
        {
            // log the value of new_temp_threshold_max
            new_temp_threshold_max = temp_threshold_max;
            current_state = SET_MAX_TEMP_THRESHOLD;
        }
        else if (current_state == INCREASE_MAX_TEMP_THRESHOLD || current_state == DECREASE_MAX_TEMP_THRESHOLD) // confirm temp setting
        {
            temp_threshold_max = new_temp_threshold_max;
            current_state = DISPLAY_MAX_TEMP_THRESHOLD;
        }

        else if (current_state == DISPLAY_MIN_TEMP_THRESHOLD) // enter temp setting mode
        {
            // log the value of new_temp_threshold_min
            new_temp_threshold_min = temp_threshold_min;
            current_state = SET_MIN_TEMP_THRESHOLD;
        }
        else if (current_state == INCREASE_MIN_TEMP_THRESHOLD || current_state == DECREASE_MIN_TEMP_THRESHOLD)
        {
            temp_threshold_min = new_temp_threshold_min;
            current_state = DISPLAY_MIN_TEMP_THRESHOLD;
        }

        else if (current_state == SET_MAX_TEMP_THRESHOLD)
        {
            temp_threshold_max = new_temp_threshold_max;
            current_state = DISPLAY_MAX_TEMP_THRESHOLD;
        }
        else if (current_state == SET_MIN_TEMP_THRESHOLD)
        {
            temp_threshold_min = new_temp_threshold_min;
            current_state = DISPLAY_MIN_TEMP_THRESHOLD;
        }

        else if (current_state == DISPLAY_MUSIC_STATE)
        {
            current_state = SET_MUSIC_STATE;
        }
        else if (current_state == SET_MUSIC_STATE)
        {
            // toggle the music state
            music_state = !music_state;
            current_state = SET_MUSIC_CONFIRM_STATE;
        }

        xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        last_button_isr_time = isr_time; // Save last ISR time
    }
}

void IRAM_ATTR cancel_button_isr_handler(void *arg)
{

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t isr_time = xTaskGetTickCountFromISR();

    if ((isr_time - last_button_isr_time) > pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS))
    {
        if (current_state == SET_MAX_TEMP_THRESHOLD) // cancel temp setting
        {
            current_state = DISPLAY_MAX_TEMP_THRESHOLD;
        }
        else if (current_state == SET_MUSIC_STATE)
        {
            current_state = DISPLAY_MUSIC_STATE;
        }
        else if (current_state == SET_MUSIC_CONFIRM_STATE)
        {
            // toggle the music state
            music_state = !music_state;
            current_state = DISPLAY_MUSIC_STATE;
        }

        xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        last_button_isr_time = isr_time; // Save last ISR time
    }
}

/*lcd display init*/
void init_display(void)
{
    // Set up I2C
    i2c_master_init();
    i2c_port_t i2c_num = I2C_MASTER_NUM;
    uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;

    // Set up the SMBus
    smbus_info = smbus_malloc();
    ESP_ERROR_CHECK(smbus_init(smbus_info, i2c_num, address));
    ESP_ERROR_CHECK(smbus_set_timeout(smbus_info, 1000 / portTICK_PERIOD_MS));

    // Set up the LCD1602 device with backlight off
    lcd_info = i2c_lcd1602_malloc();
    ESP_ERROR_CHECK(i2c_lcd1602_init(lcd_info, smbus_info, true,
                                     LCD_NUM_ROWS, LCD_NUM_COLUMNS, LCD_NUM_VISIBLE_COLUMNS));

    ESP_ERROR_CHECK(i2c_lcd1602_reset(lcd_info));

    // delete the task
    vTaskDelete(NULL);
}

void init_u_interface(void)
{
    // Create a task to initialize the LCD display
    xTaskCreate(init_display, "init_display", 2048, NULL, 5, NULL);

    // Initialize semaphore
    xSemaphore = xSemaphoreCreateBinary();

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << UP_BUTTON_GPIO) | (1ULL << DOWN_BUTTON_GPIO) | (1ULL << SET_BUTTON_GPIO) | (1ULL << CANCEL_BUTTON_GPIO);
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(UP_BUTTON_GPIO, up_button_isr_handler, NULL);
    gpio_isr_handler_add(DOWN_BUTTON_GPIO, down_button_isr_handler, NULL);
    gpio_isr_handler_add(SET_BUTTON_GPIO, set_button_isr_handler, NULL);
    gpio_isr_handler_add(CANCEL_BUTTON_GPIO, cancel_button_isr_handler, NULL);

    // force if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) to be true
    xSemaphoreGive(xSemaphore);

    while (1)
    {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Wait for semaphore
            xSemaphoreTakeFromISR(xSemaphore, NULL);

            // Perform actions for the current state
            switch (current_state)
            {
            case DISPLAY_HOME_STATE:
                // Code for DISPLAY_HOME_STATE
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_HOME_STATE");
                i2c_lcd1602_clear(lcd_info);

                i2c_lcd1602_write_string(lcd_info, "HOME_STATE");
                break;
            case DISPLAY_EXTREME_TEMP:
                // Code for DISPLAY_EXTREME_TEMP
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_EXTREME_TEMP");

                // log the value of the current maximum temperature
                // format "current_max_temp: %.2f", current_max_temp to string
                char max_temp[15];
                sprintf(max_temp, "MAX T: %.1f", current_max_temp);

                ESP_LOGI(TAG, "%s", max_temp);
                // print on lcd
                i2c_lcd1602_clear(lcd_info);
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                i2c_lcd1602_write_string(lcd_info, max_temp);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
                i2c_lcd1602_write_char(lcd_info, 'C');

                // log the value of the current minimum temperature
                // format "current_min_temp: %.2f", current_min_temp to string
                char min_temp[15];
                sprintf(min_temp, "MIN T: %.1f", current_min_temp);

                ESP_LOGI(TAG, "%s", min_temp);
                // print on lcd
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                i2c_lcd1602_write_string(lcd_info, min_temp);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
                i2c_lcd1602_write_char(lcd_info, 'C');

                break;
            case DISPLAY_MAX_TEMP_THRESHOLD:
                // Code for DISPLAY_MAX_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_MAX_TEMP_THRESHOLD");

                // lcd display
                i2c_lcd1602_clear(lcd_info);
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                i2c_lcd1602_write_string(lcd_info, "MAX THRESHOLD");

                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char max_temp_threshold[15];
                sprintf(max_temp_threshold, "%.1f", temp_threshold_max);

                // log the values of the temp thresholds
                ESP_LOGI(TAG, "%s", max_temp_threshold);

                // print on lcd
                i2c_lcd1602_write_string(lcd_info, max_temp_threshold);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
                i2c_lcd1602_write_char(lcd_info, 'C');

                break;

            /*editing state*/
            case SET_MAX_TEMP_THRESHOLD:
                // Code for SET_MAX_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MAX_TEMP_THRESHOLD");
                // lcd display
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                i2c_lcd1602_write_string(lcd_info, "NEW MAX THRESH: ");

                ESP_LOGI(TAG, "Updated max temp threshold: %.1f", temp_threshold_max);
                break;

            case INCREASE_MAX_TEMP_THRESHOLD:
                // Code for INCREASE_MAX_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: INCREASE_MAX_TEMP_THRESHOLD");

                // increase the value of new_temp_threshold_max
                new_temp_threshold_max += 1;
                // log the values of the temp thresholds
                ESP_LOGI(TAG, "new_temp_threshold_min: %.1f", new_temp_threshold_max);

                // lcd display
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char new_max_temp_threshold[13];
                sprintf(new_max_temp_threshold, "%.1f", new_temp_threshold_max);
                i2c_lcd1602_write_string(lcd_info, new_max_temp_threshold);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
                i2c_lcd1602_write_char(lcd_info, 'C');

                break;

            case DECREASE_MAX_TEMP_THRESHOLD:
                // Code for DECREASE_MAX_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: DECREASE_MAX_TEMP_THRESHOLD");

                // decrease the value of new_temp_threshold_max
                new_temp_threshold_max -= 1;
                // log the values of the temp thresholds
                ESP_LOGI(TAG, "new_temp_threshold_min: %.1f", new_temp_threshold_max);

                // lcd display
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char new_max_temp_threshold2[13];
                sprintf(new_max_temp_threshold2, "%.1f", new_temp_threshold_max);
                i2c_lcd1602_write_string(lcd_info, new_max_temp_threshold2);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
                i2c_lcd1602_write_char(lcd_info, 'C');

                break;

            case DISPLAY_MIN_TEMP_THRESHOLD:
                // Code for DISPLAY_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_MIN_TEMP_THRESHOLD");

                // log the values of the temp thresholds
                ESP_LOGI(TAG, "temp_threshold_min: %.1f", temp_threshold_min);
                break;

            /*editing state*/
            case SET_MIN_TEMP_THRESHOLD:
                // Code for SET_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MIN_TEMP_THRESHOLD");

                break;

            case INCREASE_MIN_TEMP_THRESHOLD:
                // Code for INCREASE_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: INCREASE_MIN_TEMP_THRESHOLD");

                // increase the value of new_temp_threshold_min
                new_temp_threshold_min += 1;
                // log the values of the temp thresholds
                ESP_LOGI(TAG, "new_temp_threshold_min: %.1f", new_temp_threshold_min);
                break;

            case DECREASE_MIN_TEMP_THRESHOLD:
                // Code for DECREASE_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: DECREASE_MIN_TEMP_THRESHOLD");

                // decrease the value of new_temp_threshold_min
                new_temp_threshold_min -= 1;
                // log the values of the temp thresholds
                ESP_LOGI(TAG, "new_temp_threshold_min: %.1f", new_temp_threshold_min);
                break;

            case DISPLAY_MUSIC_STATE:
                // Code for DISPLAY_MUSIC_STATE
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_MUSIC_STATE");

                // log the music state
                ESP_LOGI(TAG, "Music state: %s", music_state ? "ON" : "OFF");
                break;

            /*editing state*/
            case SET_MUSIC_STATE:
                // Code for SET_MUSIC_STATE
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MUSIC_STATE");

                // log the current music state
                ESP_LOGI(TAG, "Music state: %s", music_state ? "ON" : "OFF");

                // log "Change music state? Press SET to confirm."
                ESP_LOGI(TAG, "Change music state? Press SET to confirm.");
                break;
            case SET_MUSIC_CONFIRM_STATE:
                // Code for SET_MUSIC_CONFIRM_STATE
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MUSIC_CONFIRM_STATE");

                // log the current music state
                ESP_LOGI(TAG, "Music state changed: %s", music_state ? "ON" : "OFF");

                // go back to the DISPLAY_MUSIC_STATE
                current_state = DISPLAY_MUSIC_STATE;
                break;

            default:
                break;
            }
            interrupt_flag = false; // Clear interrupt flag
        }
        else
        {
            // Enter idle state to wait for new interrupts
            vTaskSuspend(NULL);
        }
    }
    vTaskDelete(NULL);
}
