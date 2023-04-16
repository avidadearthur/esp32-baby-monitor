#include "music.h"
#include "audio_example_file.h"
#include "espnow_mic.h"

static const char* TAG = "music_task";

// reference: i2s_adc_dac example
TaskHandle_t fftTaskHandle = NULL;
TaskHandle_t music_play_task_handle = NULL;
// extern SemaphoreHandle_t xSemaphore;
extern TaskHandle_t adcTaskHandle;
extern TaskHandle_t espnow_send_task_handle;

/**
 * @brief Reset i2s clock and mode
 */
void example_reset_play_mode(void)
{
    i2s_set_clk(EXAMPLE_I2S_NUM, EXAMPLE_I2S_SAMPLE_RATE*1.25, EXAMPLE_I2S_SAMPLE_BITS, EXAMPLE_I2S_CHANNEL_NUM);
}

/**
 * @brief Set i2s clock for example audio file
 */
void example_set_file_play_mode(void)
{
    //init DAC pad (GPIO25 & GPIO26) & mode
    i2s_set_pin(EXAMPLE_I2S_NUM, NULL);
    // set i2s clock source for i2s spk
    i2s_set_clk(EXAMPLE_I2S_NUM, EXAMPLE_I2S_SAMPLE_RATE, EXAMPLE_I2S_SAMPLE_BITS, 1); // I2S_CHANNEL_MONO (1)
    i2s_set_sample_rates(EXAMPLE_I2S_NUM, EXAMPLE_I2S_SAMPLE_RATE/2); 
}

/**
 * @brief Scale data to 16bit/32bit for I2S DMA output.
 *        DAC can only output 8bit data value.
 *        I2S DMA will still send 16 bit or 32bit data, the highest 8bit contains DAC data.
 */
int example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 2);
#else
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 4);
#endif
}

/**
 * @brief I2S ADC/DAC example
 *        1. Play an example audio file(file format: 8bit/8khz/single channel)
 *        2. Loop back to step 3
 */
void music_task(void* arg)
{
    int i2s_read_len = EXAMPLE_I2S_READ_LEN;
    size_t bytes_written;

    // get the fft task handle
    fftTaskHandle = (TaskHandle_t) arg;
    // confirm that fftTaskHandle is not NULL
    assert(fftTaskHandle != NULL);
    // suspend the fft task
    vTaskSuspend(fftTaskHandle);
    // confirm that espnow_send_task_handle is not NULL
    assert(espnow_send_task_handle != NULL);
    // suspend the espnow_send_task
    vTaskSuspend(espnow_send_task_handle);
    // ESP_LOGI(TAG, "suspend the fft task and espnow_send_task");
    // confirm that adcTaskHandle is not NULL
    assert(adcTaskHandle != NULL);
    // // notify the adc task to stop capturing using interrupt
    // vTaskNotifyGiveFromISR(adcTaskHandle, NULL);
    // wait for the semaphore
    // xSemaphoreTake(xSemaphore, portMAX_DELAY);
    // wait for notification from the adc task
    // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // suspend adc task
    // suspend_adc_capture_task();
    // vTaskSuspend(adcTaskHandle);

    // notify adc task to stop capturing using xTaskNotify
    xTaskNotifyGive(adcTaskHandle);
    // wait for notification from the adc task
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "notify the adc task to stop capturing");

    // allocate buffer for i2s read data
    uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

    // play an example audio file(file format: 8bit/16khz/single channel)
    ESP_LOGI(TAG, "playing crying soothing music: \n");
    int offset = 0;
    int tot_size = sizeof(audio_table);
    example_set_file_play_mode();
    while (offset < tot_size) {
        int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
        int i2s_wr_len = example_i2s_dac_data_scale(i2s_write_buff, (uint8_t*)(audio_table + offset), play_len);
        i2s_write(EXAMPLE_I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
        offset += play_len;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
    example_reset_play_mode();
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // free buffer
    free(i2s_write_buff);
    // give the semaphore
    // xSemaphoreGive(xSemaphore);
    // ESP_LOGI(TAG, "semaphore given");
    // resume adc task
    // resume_adc_capture_task();
    // vTaskResume(adcTaskHandle);
    // notify the adc task to start capturing using xTaskNotify
    // xTaskNotify(adcTaskHandle, 1, eNoAction);
    // ESP_LOGI(TAG, "notify the adc task");
    // // wait for notification from the adc task
    // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // ESP_LOGI(TAG, "received notification from the adc task");
    
    // // notify the fft task to start capturing using xTaskNotify
    // xTaskNotify(fftTaskHandle, 1, eNoAction);
    // // resume the fft task
    // vTaskResume(fftTaskHandle);
    // ESP_LOGI(TAG, "notify the fft task");

    // // resume the espnow_send_task
    // vTaskResume(espnow_send_task_handle);
    // ESP_LOGI(TAG, "resume the espnow_send_task");

    // notify adc task to start capturing using xTaskNotify
    xTaskNotifyGive(adcTaskHandle);
    // wait for notification from the adc task
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "received notification from the adc task");
    // notify the fft task to start capturing using xTaskNotify
    xTaskNotifyGive(fftTaskHandle);
    // resume the fft task
    vTaskResume(fftTaskHandle);
    // resume the espnow_send_task
    vTaskResume(espnow_send_task_handle);
    ESP_LOGI(TAG, "resume the espnow_send_task");
    // delete the task
    vTaskDelete(NULL);
    ESP_LOGI(TAG, "delete the music task");
}

esp_err_t init_music(TaskHandle_t fft_task_handle)
{
    // set the log level for the i2s driver
    esp_log_level_set("I2S", ESP_LOG_INFO);
    // create task for music_task function and pass the fft task handle as a parameter
    xTaskCreate(music_task, "music_task", 4096, (void*) fft_task_handle, 4, &music_play_task_handle);
    return ESP_OK;
}