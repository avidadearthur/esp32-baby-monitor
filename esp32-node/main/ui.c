/**
 * @file ui.c
 * @brief todo.
 */
#include "config.h"
#include "ui.h"

// Define states of the FSM
typedef enum
{
    HOME_STATE,
    DISPLAY_TEMP_STATE,
    DISPLAY_MUSIC_STATE,
    SET_MUSIC_STATE,
    FLIP_ALARM
} fsm_state_t;

// Initialize the FSM state to HOME_STATE
fsm_state_t fsm_state = HOME_STATE;

smbus_info_t *smbus_info = NULL;
i2c_lcd1602_info_t *lcd_info = NULL;

/*----------------------------------------------*/

// declare task handlers button1_task_handle and button2_task_handle
static TaskHandle_t isr_button_1 = NULL;
static TaskHandle_t isr_button_2 = NULL;
static TaskHandle_t isr_button_3 = NULL;
static TaskHandle_t isr_button_4 = NULL;

static volatile uint32_t last_button_isr_time = 0;
/*----------------------------------------------*/

static StreamBufferHandle_t nrf_data_xStream = NULL;

// Home task handler
static TaskHandle_t home_task_handle = NULL;

// baby flipper task handler
static TaskHandle_t hall_alarm_handle = NULL;

// baby flipper task handler
static TaskHandle_t music_task_handle = NULL;

// baby flipping alarm
static TaskHandle_t flip_alarm_handle = NULL;

#define LOG_QUEUE_SIZE 10

QueueHandle_t log_queue;

// define a mutex for the temperature
SemaphoreHandle_t temp_mutex = NULL;

// define temp threshold as global variable
float temp_threshold_min = 22.0;
float temp_threshold_max = 24.0;
// format string to print
char temp_max[15];
char temp_min[15];

// define a mutex for the music
SemaphoreHandle_t music_mutex = NULL;

// music state
char auto_music = 0;

static const char *TAG = "main";

// define the threshold task
void set_music_task(void *pvParameter)
{
    // log the task start
    ESP_LOGI(TAG, "set_music_task started");

    i2c_lcd1602_move_cursor(lcd_info, 0, 1);
    i2c_lcd1602_set_blink(lcd_info, true);

    while (1)
    {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void datetime_task(void *pvParameter)
{
    time_t now = 0;
    struct tm timeinfo = {0};
    // Wait for the time to be synchronized
    while (timeinfo.tm_year < (2020 - 1900))
    {
        ESP_LOGI(TAG, "Sync ntptime...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    char strftime_buf[64];
    while (1)
    {
        // Log the current date and time
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M %a %d/%m", &timeinfo);
        xQueueSend(log_queue, &strftime_buf, portMAX_DELAY);

        // Update the date/time every second
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        sntp_init();
    }
}

void home_task(void *arg)
{
    // read the stream of data from the nrf_data_xStream
    size_t bytes_read = 0;
    // Create buffer for mydata.now_time
    uint32_t *nrf_data = (uint32_t *)malloc(sizeof(uint8_t) * 3);

    uint16_t temp_combined = 0;
    float temp_float = 0.0;

    char datetime_str[16];

    while (1)
    {
        // i2c_lcd1602_clear(lcd_info);
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

        // format string to print
        char nrf_data_string[15];
        sprintf(nrf_data_string, "%.1f", temp_float);
        i2c_lcd1602_write_string(lcd_info, nrf_data_string);

        i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
        i2c_lcd1602_write_char(lcd_info, 'C');

        if (temp_float < temp_threshold_min || temp_float > temp_threshold_max)
        {
            i2c_lcd1602_move_cursor(lcd_info, 7, 1);
            i2c_lcd1602_write_string(lcd_info, "!");
        }
        else
        {
            i2c_lcd1602_move_cursor(lcd_info, 7, 1);
            i2c_lcd1602_write_string(lcd_info, " ");
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void init_display(void *xStream)
{
    // check if the stream buffer was passed successfully
    if (nrf_data_xStream == NULL)
    {
        ESP_LOGE(pcTaskGetName(0), "Error initializing stream buffer");
    }

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

/* From task to used to test button interrumpts */
void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t isr_time = xTaskGetTickCountFromISR();
    if ((isr_time - last_button_isr_time) > pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS))
    {
        int gpio_num = (int)arg;
        xTaskResumeFromISR(gpio_num == BUTTON_1_PIN ? isr_button_1 : gpio_num == BUTTON_2_PIN ? isr_button_2
                                                                 : gpio_num == BUTTON_3_PIN   ? isr_button_3
                                                                                              : isr_button_4);
    }
    last_button_isr_time = isr_time;
}

// Define button task functions
void button_task_1(void *arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

void button_task_2(void *arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

void button_task_3(void *arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // Change FSM state to the next state and print state transition
        if (fsm_state == HOME_STATE)
        {
            fsm_state = DISPLAY_TEMP_STATE;
            // transition from HOME_STATE to DISPLAY_TEMP_STATE
            vTaskSuspend(home_task_handle);
            i2c_lcd1602_clear(lcd_info);

            // display the current temperature thresholds
            i2c_lcd1602_move_cursor(lcd_info, 0, 0);
            sprintf(temp_max, "Thr Max: %.1f", temp_threshold_max);
            i2c_lcd1602_write_string(lcd_info, temp_max);
            i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
            i2c_lcd1602_write_char(lcd_info, 'C');

            i2c_lcd1602_move_cursor(lcd_info, 0, 1);
            sprintf(temp_min, "Thr Min: %.1f", temp_threshold_min);
            i2c_lcd1602_write_string(lcd_info, temp_min);
            i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
            i2c_lcd1602_write_char(lcd_info, 'C');
        }
        else if (fsm_state == DISPLAY_TEMP_STATE)
        {
            // transition from DISPLAY_TEMP_STATE to DISPLAY_MUSIC_STATE
            i2c_lcd1602_clear(lcd_info);

            i2c_lcd1602_move_cursor(lcd_info, 0, 0);
            i2c_lcd1602_write_string(lcd_info, "AUTO MUSIC:");
            i2c_lcd1602_move_cursor(lcd_info, 0, 1);

            // get mutex to read the auto_music variable
            if (auto_music)
                i2c_lcd1602_write_string(lcd_info, "ON");
            else
                i2c_lcd1602_write_string(lcd_info, "OFF");
        }
    }
}

void button_task_4(void *arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (fsm_state == DISPLAY_TEMP_STATE)
        {
            // transition from DISPLAY_TEMP_STATE to HOME_STATE
            fsm_state = HOME_STATE;
            i2c_lcd1602_clear(lcd_info);

            vTaskResume(home_task_handle);
        }
        else if (fsm_state == DISPLAY_MUSIC_STATE)
        {
            // transition from DISPLAY_MUSIC_STATE to DISPLAY_TEMP_STATE
            fsm_state = DISPLAY_TEMP_STATE;
            i2c_lcd1602_clear(lcd_info);

            // display the current temperature thresholds
            i2c_lcd1602_move_cursor(lcd_info, 0, 0);
            sprintf(temp_max, "Thr Max: %.1f", temp_threshold_max);
            i2c_lcd1602_write_string(lcd_info, temp_max);
            i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
            i2c_lcd1602_write_char(lcd_info, 'C');

            i2c_lcd1602_move_cursor(lcd_info, 0, 1);
            sprintf(temp_min, "Thr Min: %.1f", temp_threshold_min);
            i2c_lcd1602_write_string(lcd_info, temp_min);
            i2c_lcd1602_write_char(lcd_info, I2C_LCD1602_CHARACTER_DEGREE);
            i2c_lcd1602_write_char(lcd_info, 'C');
        }
    }
}

// initialize the button tasks
void init_buttons(void *arg)
{
    /* From task to used to test button interrupts */
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE;

    io_conf.pin_bit_mask = (1ULL << BUTTON_1_PIN);
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_1_PIN, button_isr_handler, (void *)BUTTON_1_PIN);
    xTaskCreate(button_task_1, "button_task_1", 2048, NULL, 10, &isr_button_1);

    io_conf.pin_bit_mask = (1ULL << BUTTON_2_PIN);
    gpio_config(&io_conf);
    gpio_isr_handler_add(BUTTON_2_PIN, button_isr_handler, (void *)BUTTON_2_PIN);
    xTaskCreate(button_task_2, "button_task_2", 2048, NULL, 10, &isr_button_2);

    io_conf.pin_bit_mask = (1ULL << BUTTON_3_PIN);
    gpio_config(&io_conf);
    gpio_isr_handler_add(BUTTON_3_PIN, button_isr_handler, (void *)BUTTON_3_PIN);
    xTaskCreate(button_task_3, "button_task_3", 2048, NULL, 10, &isr_button_3);

    io_conf.pin_bit_mask = (1ULL << BUTTON_4_PIN);
    gpio_config(&io_conf);
    gpio_isr_handler_add(BUTTON_4_PIN, button_isr_handler, (void *)BUTTON_4_PIN);
    xTaskCreate(button_task_4, "button_task_4", 2048, (void *)BUTTON_4_PIN, 10, &isr_button_4);

    // Create home task
    // xTaskCreate(home_task, "home_task", 1024 * 3, NULL, 2, &home_task_handle);

    vTaskDelete(NULL);
}

void init_ui(StreamBufferHandle_t xStream)
{
    nrf_data_xStream = xStream;
    // check if the stream buffer was passed successfully
    if (nrf_data_xStream == NULL)
    {
        ESP_LOGE(pcTaskGetName(0), "init_ui - Error reciving stream buffer");
    }
    else
    {
        log_queue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(char[64]));

        vTaskDelay(200 / portTICK_PERIOD_MS);

        // Create home task
        xTaskCreate(home_task, "home_task", 2048, NULL, 2, &home_task_handle);

        xTaskCreate(datetime_task, "datetime_task", 2048, NULL, 5, NULL);

        xTaskCreate(init_display, "i2c_lcd1602_task", 2048, NULL, 10, NULL);
        xTaskCreate(init_buttons, "init_buttons", 2048, NULL, 10, NULL);
    }
}