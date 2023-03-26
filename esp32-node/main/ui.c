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
    STATE_1,
    STATE_2,
    STATE_3
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

void home_task(void *arg)
{
    /*Home task - move to separate function later*/

    // read the stream of data from the nrf_data_xStream
    size_t bytes_read = 0;
    // Create buffer for mydata.now_time
    uint32_t *nrf_data = (uint32_t *)malloc(sizeof(uint32_t));

    while (1)
    {
        i2c_lcd1602_clear(lcd_info);
        i2c_lcd1602_home(lcd_info);

        // read the stream of data from the nrf_data_xStream and if there is data, print it
        bytes_read = xStreamBufferReceive(nrf_data_xStream, (void *)nrf_data, sizeof(uint32_t), portMAX_DELAY);
        // format string to print
        char nrf_data_string[16];
        sprintf(nrf_data_string, "%lu", *nrf_data);

        i2c_lcd1602_write_string(lcd_info, nrf_data_string);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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

    i2c_lcd1602_clear(lcd_info);
    i2c_lcd1602_home(lcd_info);

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
        // Increment counter and print counter value
        // printf("Button 1 pressed in state %d\n", fsm_state);
    }
}

void button_task_2(void *arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // Increment counter and print counter value
        // printf("Button 2 pressed in state %d\n", fsm_state);
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
            fsm_state = STATE_1;
            // printf("Transitioning from HOME_STATE to STATE_1\n");

            // Suspends the home task
            vTaskSuspend(home_task_handle);

            i2c_lcd1602_clear(lcd_info);
            i2c_lcd1602_home(lcd_info);
            i2c_lcd1602_write_string(lcd_info, "STATE 1 TASK");
        }
        else if (fsm_state == STATE_1)
        {
            fsm_state = STATE_2;
            // printf("Transitioning from STATE_1 to STATE_2\n");
            i2c_lcd1602_clear(lcd_info);
            i2c_lcd1602_home(lcd_info);
            i2c_lcd1602_write_string(lcd_info, "STATE 2 TASK");
        }
        else if (fsm_state == STATE_2)
        {
            fsm_state = STATE_3;
            // printf("Transitioning from STATE_2 to STATE_3\n");
            i2c_lcd1602_clear(lcd_info);
            i2c_lcd1602_home(lcd_info);
            i2c_lcd1602_write_string(lcd_info, "STATE 3 TASK");
        }
        // if in state 3 go back to home state
        else if (fsm_state == STATE_3)
        {
            fsm_state = HOME_STATE;
            // printf("Transitioning from STATE_3 to HOME_STATE\n");
            i2c_lcd1602_clear(lcd_info);
            i2c_lcd1602_home(lcd_info);

            // Resumes the home task
            vTaskResume(home_task_handle);
        }
    }
}

void button_task_4(void *arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // Change FSM state to the previous state and print state transition
        if (fsm_state == STATE_3)
        {
            fsm_state = STATE_2;
            // printf("Transitioning from STATE_3 to STATE_2\n");
            i2c_lcd1602_clear(lcd_info);
            i2c_lcd1602_home(lcd_info);
            i2c_lcd1602_write_string(lcd_info, "STATE 2 TASK");
        }
        else if (fsm_state == STATE_2)
        {
            fsm_state = STATE_1;
            // printf("Transitioning from STATE_2 to STATE_1\n");
            i2c_lcd1602_clear(lcd_info);
            i2c_lcd1602_home(lcd_info);
            i2c_lcd1602_write_string(lcd_info, "STATE 1 TASK");
        }
        else if (fsm_state == STATE_1)
        {
            fsm_state = HOME_STATE;
            // printf("Transitioning from STATE_1 to HOME_STATE\n");
            i2c_lcd1602_clear(lcd_info);
            i2c_lcd1602_home(lcd_info);

            // Resumes the home task
            vTaskResume(home_task_handle);
        }
        // if in home state go to state 3
        else if (fsm_state == HOME_STATE)
        {
            fsm_state = STATE_3;
            // printf("Transitioning from HOME_STATE to STATE_3\n");

            // Suspends the home task
            vTaskSuspend(home_task_handle);

            i2c_lcd1602_clear(lcd_info);
            i2c_lcd1602_home(lcd_info);
            i2c_lcd1602_write_string(lcd_info, "STATE 3 TASK");
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
    xTaskCreate(home_task, "home_task", 1024 * 3, NULL, 2, &home_task_handle);

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
        xTaskCreate(init_display, "i2c_lcd1602_task", 2048, NULL, 10, NULL);

        xTaskCreate(init_buttons, "init_buttons", 2048, NULL, 10, NULL);
    }
}