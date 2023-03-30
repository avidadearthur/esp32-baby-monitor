#include "music.h"
#include "audio_example_file.h"

static const char* TAG = "music_task";

// reference: i2s_adc_dac example
extern TaskHandle_t adcTaskHandle;

/**
 * @brief Reset i2s clock and mode
 */
void example_reset_play_mode(void)
{
    i2s_set_clk(EXAMPLE_I2S_NUM, EXAMPLE_I2S_SAMPLE_RATE, EXAMPLE_I2S_SAMPLE_BITS, EXAMPLE_I2S_CHANNEL_NUM);
}

/**
 * @brief Set i2s clock for example audio file
 */
void example_set_file_play_mode(void)
{
    i2s_set_clk(EXAMPLE_I2S_NUM, EXAMPLE_I2S_SAMPLE_RATE, EXAMPLE_I2S_SAMPLE_BITS, 1); // channel 1 represent i2s_channel_t I2S_CHANNEL_MONO
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
void music_task(void*arg)
{
    int i2s_read_len = EXAMPLE_I2S_READ_LEN;
    size_t bytes_written;

    // confirm that adcTaskHandle is not NULL
    assert(adcTaskHandle != NULL);
    // suspend adc task to disable ADC
    vTaskSuspend(adcTaskHandle);
    // disable ADC
    i2s_adc_disable(EXAMPLE_I2S_NUM);

    // allocate buffer for i2s read data
    uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

    //1. Play an example audio file(file format: 8bit/16khz/single channel)
    ESP_LOGI(TAG, "Playing file example: \n");
    int offset = 0;
    int tot_size = sizeof(audio_table);
    example_set_file_play_mode();
    while (offset < tot_size) {
        int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
        int i2s_wr_len = example_i2s_dac_data_scale(i2s_write_buff, (uint8_t*)(audio_table + offset), play_len);
        i2s_write(EXAMPLE_I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
        offset += play_len;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    example_reset_play_mode();
    // free buffer
    free(i2s_write_buff);
    // enable ADC
    i2s_adc_enable(EXAMPLE_I2S_NUM);
    // resume adc task
    vTaskResume(adcTaskHandle);
    
    vTaskDelete(NULL);
}

esp_err_t init_music(void)
{
    esp_log_level_set("I2S", ESP_LOG_INFO);
    xTaskCreate(music_task, "music_task", 2048, NULL, 5, NULL);
    return ESP_OK;
}