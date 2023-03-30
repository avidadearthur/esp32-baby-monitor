#include "u_interface.h"

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
    DISPLAY_MIN_TEMP_THRESHOLD,
    SET_MIN_TEMP_THRESHOLD,
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
float new_temp_threshold_max;

// define current max and min temp as global variable
float current_max_temp = 0.0;
float current_min_temp = 0.0;

// define music state as global variable
bool music_state = false;

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
            new_temp_threshold_max = new_temp_threshold_max + 1;
            current_state = SET_MAX_TEMP_THRESHOLD;
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
            new_temp_threshold_max = new_temp_threshold_max - 1;
            current_state = SET_MAX_TEMP_THRESHOLD;
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
            new_temp_threshold_max = temp_threshold_max;
            current_state = SET_MAX_TEMP_THRESHOLD;
        }
        else if (current_state == SET_MAX_TEMP_THRESHOLD) // confirm temp setting
        {
            temp_threshold_max = new_temp_threshold_max;
            current_state = DISPLAY_MAX_TEMP_THRESHOLD;
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

void init_u_interface(void)
{
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
                break;
            case DISPLAY_EXTREME_TEMP:
                // Code for DISPLAY_EXTREME_TEMP
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_EXTREME_TEMP");

                // log the value of the current maximum temperature
                ESP_LOGI(TAG, "current_max_temp: %.2f", current_max_temp);

                // log the value of the current minimum temperature
                ESP_LOGI(TAG, "current_min_temp: %.2f", current_min_temp);
                break;
            case DISPLAY_MAX_TEMP_THRESHOLD:
                // Code for DISPLAY_MAX_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_MAX_TEMP_THRESHOLD");

                // log the values of the temp thresholds
                ESP_LOGI(TAG, "temp_threshold_max: %.2f", temp_threshold_max);
                break;

            /*editing state*/
            case SET_MAX_TEMP_THRESHOLD:
                // Code for SET_MAX_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MAX_TEMP_THRESHOLD");

                ESP_LOGI(TAG, "Updated max temp threshold: %.2f", temp_threshold_max);
                break;

            case DISPLAY_MIN_TEMP_THRESHOLD:
                // Code for DISPLAY_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: DISPLAY_MIN_TEMP_THRESHOLD");

                // log the values of the temp thresholds
                ESP_LOGI(TAG, "temp_threshold_min: %.2f", temp_threshold_min);
                break;

            /*editing state*/
            case SET_MIN_TEMP_THRESHOLD:
                // Code for SET_MIN_TEMP_THRESHOLD
                // log the current state
                ESP_LOGI(TAG, "Current state: SET_MIN_TEMP_THRESHOLD");

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
