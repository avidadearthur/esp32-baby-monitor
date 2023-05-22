#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "espnow_mic.h"
#include "music.h"
#include "sd_record.h"
#include "espnow_send.h"

static const char *TAG = "espnow_mic";
StreamBufferHandle_t fft_stream_buf;
StreamBufferHandle_t record_stream_buf;

uint8_t *mic_read_buf;
uint8_t *spk_write_buf;

// reference: https://www.codeinsideout.com/blog/freertos/notification/#two-looping-tasks
TaskHandle_t adcTaskHandle = NULL;

// suspend i2s_adc_capture_task function
void suspend_adc_capture_task()
{
    assert(adcTaskHandle != NULL);
    vTaskSuspend(adcTaskHandle);
    // ESP_LOGI(TAG, "adc capture task suspended\n");
}

// resume i2s_adc_capture_task function
void resume_adc_capture_task()
{
    assert(adcTaskHandle != NULL);
    vTaskResume(adcTaskHandle);
    // ESP_LOGI(TAG, "adc capture task resumed\n");
}

// get the task handle of i2s_adc_capture_task
TaskHandle_t get_adc_task_handle()
{
    assert(adcTaskHandle != NULL);
    return adcTaskHandle;
}

// i2s adc capture task
void i2s_adc_capture_task(void *task_param)
{
    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t)task_param;

    // int read_len = EXAMPLE_I2S_READ_LEN/2;
    int read_len = (EXAMPLE_I2S_READ_LEN / 2) * sizeof(char);
    mic_read_buf = calloc(EXAMPLE_I2S_READ_LEN, sizeof(char));

    // enable i2s adc
    size_t bytes_read = 0;          // to count the number of bytes read from the i2s adc
    TickType_t ticks_to_wait = 100; // wait 100 ticks for the mic_stream_buf to be available

    // enable i2s adc
    i2s_adc_enable(EXAMPLE_I2S_NUM);

    while (true)
    {

        // check if notification is received, if yes, clear the notification value on exit
        if (ulTaskNotifyTake(pdTRUE, 0) == pdPASS)
        {
            // get task handle of music_task
            TaskHandle_t music_play_task_handle = get_music_play_task_handle();
            assert(music_play_task_handle != NULL);
            // suspend espnow send task
            // get task handle of espnow_send_task
            TaskHandle_t espnow_send_task_handle = get_espnow_send_task_handle();
            assert(espnow_send_task_handle != NULL);
            vTaskSuspend(espnow_send_task_handle);
            // disable i2s adc
            i2s_adc_disable(EXAMPLE_I2S_NUM);
            // enable dac
            i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
            // clear the fft stream buffer to avoid retriggering music task
            xStreamBufferReset(fft_stream_buf);
            // notify music task to resume
            xTaskNotifyGive(music_play_task_handle);

            // wait for notification to resume adc capture task
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            // disable dac
            i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);
            // enable i2s adc
            i2s_adc_enable(EXAMPLE_I2S_NUM);
            // resume espnow send task
            vTaskResume(espnow_send_task_handle);
            // notify music task to resume finishing
            xTaskNotifyGive(music_play_task_handle);
        }
        else
        {
            // read from i2s bus and use errno to check if i2s_read is successful
            if (i2s_read(EXAMPLE_I2S_NUM, (char *)mic_read_buf, read_len, &bytes_read, ticks_to_wait) != ESP_OK)
            {
                ESP_LOGE(TAG, "Error reading from i2s adc: %d", errno);
                // deinit_config();
                // exit(errno);
            }

            // check if the number of bytes read is equal to the number of bytes to read
            if (bytes_read != read_len)
            {
                ESP_LOGE(TAG, "Error reading from i2s adc: %d", errno);
                // deinit_config();
                // exit(errno);
            }

/**
 * xstreambuffersend to fft task
 */
#if FFT_TASK
            size_t byte_sent = xStreamBufferSend(fft_stream_buf, (void *)mic_read_buf, EXAMPLE_I2S_READ_LEN / 16, portMAX_DELAY);
            if (byte_sent != (EXAMPLE_I2S_READ_LEN / 16))
            {
                ESP_LOGE(TAG, "Error: only sent %d bytes to the stream buffer out of %d \n", byte_sent, (EXAMPLE_I2S_READ_LEN / 16));
                // deinit_config();
                // exit(errno);
            }
#endif

            // scale the data to 8 bit
            i2s_adc_data_scale(mic_read_buf, mic_read_buf, read_len);

            /**
             * xstreambuffersend is a blocking function that sends data to the stream buffer,
             * esp_now_send needs to send 128 packets of 250 bytes each, so the stream buffer needs to be able to hold at least 2-3 times of 128 * 250 bytes = BYTE_RATE bytes
             * */
            size_t espnow_byte = xStreamBufferSend(mic_stream_buf, (void *)mic_read_buf, read_len, portMAX_DELAY);
            if (espnow_byte != read_len)
            {
                ESP_LOGE(TAG, "Error: only sent %d bytes to the stream buffer out of %d \n", espnow_byte, read_len);
            }

/**
 * xstreambuffersend to sd record task
 */
#if RECORD_TASK
            size_t record_byte = xStreamBufferSend(record_stream_buf, (void *)mic_read_buf, read_len, portMAX_DELAY);
            // check if bytes sent is equal to bytes read
            if (record_byte != read_len)
            {
                ESP_LOGE(TAG, "Error: only sent %d bytes to the stream buffer out of %d \n", record_byte, read_len);
            }
#endif
        }
    }
    // disable i2s adc
    i2s_adc_disable(EXAMPLE_I2S_NUM);
    ESP_LOGI(TAG, "i2s adc disabled\n");
    free(mic_read_buf);
    vTaskDelete(NULL);
}

/**
 * @brief Scale data to 8bit for data from ADC.
 *        DAC can only output 8 bit data.
 *        Scale each 16bit-wide ADC data to 8bit DAC data.
 */
void i2s_adc_data_scale(uint8_t *des_buff, uint8_t *src_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2)
    {
        dac_value = ((((uint16_t)(src_buff[i + 1] & 0xf) << 8) | ((src_buff[i + 0]))));
        des_buff[j++] = 0;
        des_buff[j++] = dac_value * 256 / 4096;
    }
}

// i2s dac playback task
void i2s_dac_playback_task(void *task_param)
{
    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t spk_stream_buf = (StreamBufferHandle_t)task_param;

    size_t bytes_written = 0;
    spk_write_buf = (uint8_t *)calloc(BYTE_RATE, sizeof(char));
    assert(spk_write_buf != NULL);

    while (true)
    {
        // read from the stream buffer, use errno to check if xstreambufferreceive is successful
        size_t num_bytes = xStreamBufferReceive(spk_stream_buf, (void *)spk_write_buf, BYTE_RATE, portMAX_DELAY);
        if (num_bytes > 0)
        {
            // send data to i2s dac
            esp_err_t err = i2s_write(EXAMPLE_I2S_NUM, spk_write_buf, num_bytes, &bytes_written, portMAX_DELAY);
            if ((err != ESP_OK))
            {
                ESP_LOGE(TAG, "Error writing I2S: %0x\n", err);
            }
            // reference: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html
            // reference: https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/i2s.html#_CPPv49i2s_write10i2s_port_tPKv6size_tP6size_t10TickType_t
            // reference: i2s_write(I2S_NUM, samples_data, ((bits+8)/16)*SAMPLE_PER_CYCLE*4, &i2s_bytes_write, 100);
            // this number is  without adc to dac scaling that is done in the i2s_adc_data_scale function, the i2s_write function needs to be called with the above parameters
        }

#if EXAMPLE_I2S_BUF_DEBUG
        mic_disp_buf((uint8_t *)spk_write_buf, EXAMPLE_I2S_READ_LEN);
#endif
    }
    free(spk_write_buf);
    vTaskDelete(NULL);
}

/* call the init_auidio function for starting adc and filling the buf -second */
esp_err_t init_audio_trans(StreamBufferHandle_t mic_stream_buf, StreamBufferHandle_t fft_audio_buf, StreamBufferHandle_t record_audio_buf)
{
    ESP_LOGI(TAG, "initializing i2s mic\n");
    fft_stream_buf = fft_audio_buf;
    record_stream_buf = record_audio_buf;
    // check if record_stream_buf is null
    if (record_stream_buf == NULL)
    {
        ESP_LOGE(TAG, "espnow_mic.c - Error receiving stream buffer");
    }

    // create the adc capture task and pin the task to core 0
    xTaskCreate(i2s_adc_capture_task, "i2s_adc_capture_task", 4096, (void *)mic_stream_buf, IDLE_TASK_PRIO, &adcTaskHandle);
    configASSERT(adcTaskHandle);

    return ESP_OK;
}

/* call the init_auidio function for starting adc and filling the buf -second */
esp_err_t init_audio_recv(StreamBufferHandle_t network_stream_buf)
{
    ESP_LOGI(TAG, "initializing i2s spk\n");
    // /* thread for filling the buf for the reciever and dac */
    xTaskCreate(i2s_dac_playback_task, "i2s_dac_playback_task", 2048, (void *)network_stream_buf, IDLE_TASK_PRIO, NULL);

    return ESP_OK;
}

// /** debug functions below */

/**
 * @brief debug buffer data
 */
void mic_disp_buf(uint8_t *buf, int length)
{
#if EXAMPLE_I2S_BUF_DEBUG
    ESP_LOGI(TAG, "\n=== MIC ===\n");
    for (int i = 0; i < length; i++)
    {
        ESP_LOGI(TAG, "%02x ", buf[i]);
        if ((i + 1) % 8 == 0)
        {
            ESP_LOGI(TAG, "\n");
        }
    }
    ESP_LOGI(TAG, "\n=== MIC ===\n");
#endif
}
