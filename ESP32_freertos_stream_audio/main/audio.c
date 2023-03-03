#include "audio.h"

StreamBufferHandle_t xStreamBufferNetwork;

TaskHandle_t audioCaptureTaskHandle;
TaskHandle_t audioPlayBackTaskHandle;

void suspend_audio_capture()
{
    vTaskSuspend(audioCaptureTaskHandle);
}

void resume_audio_capture()
{
    vTaskResume(audioCaptureTaskHandle);
}

void suspend_audio_playback()
{
    vTaskSuspend(audioPlayBackTaskHandle);
}

void resume_audio_playback()
{
    vTaskResume(audioPlayBackTaskHandle);
}

static void init_mic()
{
    int i2s_num = I2S_COMM_MODE;
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN,
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_SAMPLE_BITS,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .channel_format = I2S_FORMAT,
        .intr_alloc_flags = 0,
        .dma_buf_count = 6,
        .dma_buf_len = 256,
        .use_apll = 1,
    };

    // Call driver installation function and adc pad.
    ESP_ERROR_CHECK(i2s_driver_install(i2s_num, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL));
    ESP_ERROR_CHECK(i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN));
}

void i2s_adc_data_scale(uint8_t *d_buff, uint8_t *s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;

    for (int i = 0; i < len; i += 2)
    {
        dac_value = ((((s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
}

static void audio_playback_task(void *arg)
{
    // add delay to allow the task to be blocked
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // disable i2s_adc. No need to init speaker because it has already been done in init_mic()
    //

    /* Network transmission simulation */
    uint8_t *ucRxData = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    size_t xReceivedBytes;
    // const TickType_t xBlockTime = pdMS_TO_TICKS(20);
    const TickType_t xBlockTime = pdMS_TO_TICKS(100);
    /*--------------------------------------------------------------------------*/

    StreamBufferHandle_t xStreamBufferNet = (StreamBufferHandle_t)arg;

    color_printf(COLOR_PRINT_GREEN, "\t\taudio_playback_task: starting to listen");

    while (true)
    {
        xReceivedBytes = xStreamBufferReceive(xStreamBufferNet,
                                              (void *)ucRxData,
                                              READ_BUF_SIZE_BYTES * sizeof(char),
                                              portMAX_DELAY);
        if (xReceivedBytes > 0)
        {
            // write data to i2s bus
            i2s_write(I2S_COMM_MODE, (char *)ucRxData, READ_BUF_SIZE_BYTES * sizeof(char), &xReceivedBytes, portMAX_DELAY);
        }
        else
        {
            /* For test purposes */
            /* The call to xStreamBufferReceive() timed out before any data was
            available. */
            color_printf(COLOR_PRINT_RED, "\t\taudio_playback_task: notify timeout");
        }
    }
}

void audio_capture_task(void *arg)
{
    // add delay to allow the task to be blocked
    vTaskDelay(500 / portTICK_PERIOD_MS);

    init_mic();
    i2s_adc_enable(I2S_CHANNEL_NUM);
    // int16_t i2s_readraw_buff[READ_BUF_SIZE_BYTES];
    size_t bytes_read;

    size_t xBytesSent;
    // uint8_t ucArrayToSend[READ_BUF_SIZE_BYTES];
    const TickType_t xBlockTime = pdMS_TO_TICKS(100);

    uint8_t *i2s_readraw_buff = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    uint8_t *ucArrayToSend = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));

    StreamBufferHandle_t xStreamBuffer = (StreamBufferHandle_t)arg;

    color_printf(COLOR_PRINT_BLUE, "\t\taudio_capture_task: starting transmission");

    while (1)
    {
        /* Read audio using i2s and scale it down to uint8_t*/
        i2s_read(I2S_CHANNEL_NUM, (char *)i2s_readraw_buff, READ_BUF_SIZE_BYTES * sizeof(char), &bytes_read, portMAX_DELAY);

        i2s_adc_data_scale(ucArrayToSend, i2s_readraw_buff, READ_BUF_SIZE_BYTES * sizeof(char));

        /* Send an array to the stream buffer, blocking for a maximum of 100ms to
        wait for enough space to be available in the stream buffer. */
        xBytesSent = xStreamBufferSend(xStreamBuffer,
                                       (void *)ucArrayToSend,
                                       READ_BUF_SIZE_BYTES * sizeof(char),
                                       xBlockTime);
    }

    i2s_adc_disable(I2S_COMM_MODE);
    color_printf(COLOR_PRINT_BLUE, "\t\taudio_capture_task: deleting task");
    vTaskDelete(NULL);
}

void init_audio(StreamBufferHandle_t xStreamBuffer, StreamBufferHandle_t network_stream_buf)
{
    xStreamBufferNetwork = network_stream_buf;

    xTaskCreate(audio_capture_task, "audio_capture_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBuffer, 5, &audioCaptureTaskHandle);

    /*In the final version, oe of the esp32 will use audio_playback_task and the other audio_capture_task*/
    xTaskCreate(audio_playback_task, "audio_playback_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBufferNetwork, 5, &audioPlayBackTaskHandle);

    // Suspend playback task
    vTaskSuspend(audioPlayBackTaskHandle);
    // Log the task handle was suspended
    color_printf(COLOR_PRINT_BLUE, "\t\tinit_audio: audio_playback_task suspended");
}