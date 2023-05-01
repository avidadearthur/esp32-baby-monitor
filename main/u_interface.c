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

// data from nrf24l01
static StreamBufferHandle_t nrf_data_xStream = NULL;

// Home task handler
static TaskHandle_t home_task_handle = NULL;

// Datetime queue
#define LOG_QUEUE_SIZE 10
QueueHandle_t log_queue;

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
        if (current_state == INCREASE_MAX_TEMP_THRESHOLD || current_state == DECREASE_MAX_TEMP_THRESHOLD) // cancel temp setting
        {
            current_state = DISPLAY_MAX_TEMP_THRESHOLD;
        }

        else if (current_state == INCREASE_MIN_TEMP_THRESHOLD || current_state == DECREASE_MIN_TEMP_THRESHOLD) // cancel temp setting
        {
            current_state = DISPLAY_MIN_TEMP_THRESHOLD;
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

void datetime_task(void *pvParameter)
{
    time_t now = 0;
    struct tm timeinfo = {0};
    char strftime_buf[64];

    // Wait for the time to be synchronized
    // while (timeinfo.tm_year < (2020 - 1900))
    // {
    //     ESP_LOGI(TAG, "Sync ntptime...");
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     time(&now);
    //     localtime_r(&now, &timeinfo);
    //     // copy "Sync ntptime..." into strftime_buf
    //     strcpy(strftime_buf, "Sync ntptime...");
    //     xQueueSend(log_queue, &strftime_buf, portMAX_DELAY);
    // }

    while (1)
    {
        // Log the current date and time
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M %a %d/%m", &timeinfo);
        xQueueSend(log_queue, &strftime_buf, portMAX_DELAY);

        // Update the date/time every second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        sntp_init();
    }
}

void update_min_max_temp(float temp)
{
    if (temp > current_max_temp)
    {
        current_max_temp = temp;
    }
    if (temp < current_min_temp || current_min_temp == 0.0)
    {
        current_min_temp = temp;
    }
}

void home_task(void *pvParameter)
{
    // read the stream of data from the nrf_data_xStream
    size_t bytes_read = 0;
    // Create buffer for mydata.now_time
    uint32_t *nrf_data = (uint32_t *)malloc(sizeof(uint8_t) * 3);

    uint16_t temp_combined = 0;
    float temp_float = 0.0;

    char datetime_str[16];

    // to avoid displaying rubish
    // read the stream of data from the nrf_data_xStream and if there is data, print it
    bytes_read = xStreamBufferReceive(nrf_data_xStream, (void *)nrf_data, sizeof(uint8_t) * 3, portMAX_DELAY);

    while (1)
    {
        i2c_lcd1602_move_cursor(lcd_info, 0, 0);

        if (xQueueReceive(log_queue, &datetime_str, 100))
        {
            i2c_lcd1602_write_string(lcd_info, datetime_str);
        }
        i2c_lcd1602_move_cursor(lcd_info, 0, 1);

        // read the stream of data from the nrf_data_xStream and if there is data, print it
        bytes_read = xStreamBufferReceive(nrf_data_xStream, (void *)nrf_data, sizeof(uint8_t) * 3, portMAX_DELAY);

        // cast nrf_data to uint8_t array
        uint8_t *data = (uint8_t *)nrf_data;

        // Combine the upper and lower bytes of the temperature into a 16-bit integer
        temp_combined = ((uint16_t)data[1] << 8) | data[0];

        // Convert the combined temperature back to a float
        temp_float = ((float)temp_combined) / 10.0;

        // call helper function to update the min and max temp
        update_min_max_temp(temp_float);

        // format string to print
        char nrf_data_string[15];
        sprintf(nrf_data_string, "%.1f", temp_float);
        i2c_lcd1602_write_string(lcd_info, nrf_data_string);

        i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
        i2c_lcd1602_write_char(lcd_info, 'C');

        if (temp_float < temp_threshold_min || temp_float > temp_threshold_max)
        {
            i2c_lcd1602_move_cursor(lcd_info, 7, 1);
            i2c_lcd1602_write_string(lcd_info, "T!");
        }
        else
        {
            i2c_lcd1602_move_cursor(lcd_info, 7, 1);
            i2c_lcd1602_write_string(lcd_info, "  ");
        }
        if (data[2] == 1)
        {
            i2c_lcd1602_move_cursor(lcd_info, 14, 1);
            i2c_lcd1602_write_string(lcd_info, "B!");
        }
        else if (data[2] == 0)
        {
            i2c_lcd1602_move_cursor(lcd_info, 14, 1);
            i2c_lcd1602_write_string(lcd_info, "  ");
        }
    }
}

void init_u_interface(StreamBufferHandle_t xStream)
{
    // Init stream buffer
    nrf_data_xStream = xStream;
    // init datetime queue
    log_queue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(char[64]));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    // Create a task to initialize the LCD display
    xTaskCreate(init_display, "init_display", 2048, NULL, 5, NULL);

    // Create a task to update the date/time
    xTaskCreate(datetime_task, "datetime_task", 2048, NULL, 5, NULL);

    // Create home task and suspend it
    xTaskCreate(home_task, "home_task", 3072, NULL, 2, &home_task_handle);
    vTaskSuspend(home_task_handle);

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

                // resume home task
                vTaskResume(home_task_handle);

                break;
            case DISPLAY_EXTREME_TEMP:

                // suspend home task
                vTaskSuspend(home_task_handle);

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

                if (new_temp_threshold_max < 36)
                {
                    new_temp_threshold_max += 1;
                }

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

                // check if the new_temp_threshold_max is greater than the temp_threshold_min
                if (new_temp_threshold_max > temp_threshold_min)
                {
                    // decrease the value of new_temp_threshold_max
                    new_temp_threshold_max -= 1;
                }

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

                // lcd display
                i2c_lcd1602_clear(lcd_info);
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                i2c_lcd1602_write_string(lcd_info, "MIN THRESHOLD");

                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char min_temp_threshold[15];
                sprintf(min_temp_threshold, "%.1f", temp_threshold_min);
                i2c_lcd1602_write_string(lcd_info, min_temp_threshold);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
                i2c_lcd1602_write_char(lcd_info, 'C');

                break;

            /*editing state*/
            case SET_MIN_TEMP_THRESHOLD:
                // Code for SET_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MIN_TEMP_THRESHOLD");

                // lcd display
                i2c_lcd1602_move_cursor(lcd_info, 0, 0);
                i2c_lcd1602_write_string(lcd_info, "NEW MIN THRESH: ");

                ESP_LOGI(TAG, "Updated min temp threshold: %.1f", temp_threshold_min);

                break;

            case INCREASE_MIN_TEMP_THRESHOLD:
                // Code for INCREASE_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: INCREASE_MIN_TEMP_THRESHOLD");

                // check if the new temp threshold is lower than the max temp threshold
                if (new_temp_threshold_min < temp_threshold_max)
                {
                    new_temp_threshold_min += 1;
                }

                // log the values of the temp thresholds
                ESP_LOGI(TAG, "new_temp_threshold_min: %.1f", new_temp_threshold_min);

                // lcd display
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char new_min_temp_threshold[13];
                sprintf(new_min_temp_threshold, "%.1f", new_temp_threshold_min);
                i2c_lcd1602_write_string(lcd_info, new_min_temp_threshold);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
                i2c_lcd1602_write_char(lcd_info, 'C');

                break;

            case DECREASE_MIN_TEMP_THRESHOLD:
                // Code for DECREASE_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: DECREASE_MIN_TEMP_THRESHOLD");

                // decrease the value of new_temp_threshold_min if it is greater than 15
                if (new_temp_threshold_min > 15)
                {
                    new_temp_threshold_min -= 1;
                }

                // log the values of the temp thresholds
                ESP_LOGI(TAG, "new_temp_threshold_min: %.1f", new_temp_threshold_min);

                // lcd display
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char new_min_temp_threshold2[13];
                sprintf(new_min_temp_threshold2, "%.1f", new_temp_threshold_min);
                i2c_lcd1602_write_string(lcd_info, new_min_temp_threshold2);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
                i2c_lcd1602_write_char(lcd_info, 'C');

                break;

            case DISPLAY_MUSIC_STATE:
                // Code for DISPLAY_MUSIC_STATE
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_MUSIC_STATE");

                // log the music state
                ESP_LOGI(TAG, "Music state: %s", music_state ? "ON" : "OFF");

                // lcd display
                i2c_lcd1602_clear(lcd_info);
                i2c_lcd1602_write_string(lcd_info, "AUTO MUSIC:");

                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char music_state_str[15];
                sprintf(music_state_str, "%s", music_state ? "ON" : "OFF");
                i2c_lcd1602_write_string(lcd_info, music_state_str);
                i2c_lcd1602_move_cursor(lcd_info, 0, 15);

                break;

            /*editing state*/
            case SET_MUSIC_STATE:
                // Code for SET_MUSIC_STATE
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MUSIC_STATE");

                // log the current music state
                ESP_LOGI(TAG, "Music state: %s", music_state ? "ON" : "OFF");

                // lcd display
                i2c_lcd1602_clear(lcd_info);
                i2c_lcd1602_write_string(lcd_info, "SET AUTO MUSIC?");
                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char new_music_state_str[15];
                sprintf(new_music_state_str, "%s", music_state ? "ON" : "OFF");
                i2c_lcd1602_write_string(lcd_info, new_music_state_str);
                i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_ARROW_RIGHT);
                sprintf(new_music_state_str, "%s", !music_state ? "ON" : "OFF");
                i2c_lcd1602_write_string(lcd_info, new_music_state_str);

                // log "Change music state? Press SET to confirm."
                ESP_LOGI(TAG, "Change music state? Press SET to confirm.");
                break;
            case SET_MUSIC_CONFIRM_STATE:
                // Code for SET_MUSIC_CONFIRM_STATE
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MUSIC_CONFIRM_STATE");

                // log the current music state
                ESP_LOGI(TAG, "Music state changed: %s", music_state ? "ON" : "OFF");

                // lcd display
                i2c_lcd1602_clear(lcd_info);
                i2c_lcd1602_write_string(lcd_info, "AUTO MUSIC:");

                i2c_lcd1602_move_cursor(lcd_info, 0, 1);
                char music_state_str2[15];
                sprintf(music_state_str2, "%s", music_state ? "ON" : "OFF");
                i2c_lcd1602_write_string(lcd_info, music_state_str2);
                i2c_lcd1602_move_cursor(lcd_info, 0, 15);

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
