#include "audio.h"

TaskHandle_t audioCaptureTaskHandle;
TaskHandle_t audioPlayBackTaskHandle;

static StreamBufferHandle_t mic_recording_stream_buf;
static StreamBufferHandle_t dsp_stream_buf;

/**
 * @brief I2S ADC/DAC mode init. This initializes the I2S driver to support
 * both the analog microphone input and the analog speaker output.
 */
void i2s_init(void)
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

void audio_capture_task(void *arg)
{
    // enable ADC
    i2s_adc_enable(I2S_CHANNEL_NUM);

    size_t bytes_read;

    const TickType_t xBlockTime = pdMS_TO_TICKS(100);

    uint8_t *i2s_readraw_buff = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    uint8_t *ucArrayToSend = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));

    // mic stream buffer that will be read in transport.c
    StreamBufferHandle_t xStreamBuffer = (StreamBufferHandle_t)arg;

    color_printf(COLOR_PRINT_BLUE, "\t\taudio_capture_task_task: starting capture");

    while (true)
    {
        /* Read audio using i2s and scale it down to uint8_t*/
        i2s_read(I2S_CHANNEL_NUM, (char *)i2s_readraw_buff, READ_BUF_SIZE_BYTES * sizeof(char), &bytes_read, portMAX_DELAY);

        i2s_adc_data_scale(ucArrayToSend, i2s_readraw_buff, READ_BUF_SIZE_BYTES * sizeof(char));

        if (xStreamBuffer != NULL)
        {
            /* Send an array to the stream buffer, blocking for a maximum of 100ms to
            wait for enough space to be available in the stream buffer. */
            xStreamBufferSend(xStreamBuffer,
                              (void *)ucArrayToSend,
                              READ_BUF_SIZE_BYTES * sizeof(char),
                              xBlockTime);
        }

        // Mind that the more buffers enabled, the more delay will be introduced
        // and the more memory will be used

        /* If recordig buffer has been passed as a non NULL argument,
        audio captured will be sent to recording stream*/
        if (mic_recording_stream_buf != NULL)
        {
            xStreamBufferSend(mic_recording_stream_buf,
                              (void *)ucArrayToSend,
                              READ_BUF_SIZE_BYTES * sizeof(char),
                              xBlockTime);
        }

        /* If dsp_stream_buf has been passed as a non NULL argument,
        audio captured will be sent to dsp stream*/
        if (dsp_stream_buf != NULL)
        {
            xStreamBufferSend(dsp_stream_buf,
                              (void *)ucArrayToSend,
                              READ_BUF_SIZE_BYTES * sizeof(char),
                              xBlockTime);
        }
    }
    // disable ADC
    i2s_adc_disable(I2S_CHANNEL_NUM);
}

/*  This task is not used here.
 *  In the final version, oe of the esp32 will use audio_playback_task and the other audio_capture_task
 *  for the playback, see the peer version of this project in EE3/ESP32_espnow_audio_peer
 */
static void audio_playback_task(void *arg)
{
    uint8_t *ucRxData = (uint8_t *)calloc(READ_BUF_SIZE_BYTES * 100, sizeof(char));
    size_t xReceivedBytes;
    size_t bytes_written = 0;

    StreamBufferHandle_t xStreamBufferNet = (StreamBufferHandle_t)arg;

    color_printf(COLOR_PRINT_GREEN, "\t\trec_task: starting to listen");

    while (true)
    {
        xReceivedBytes = xStreamBufferReceive(xStreamBufferNet,
                                              (void *)ucRxData,
                                              READ_BUF_SIZE_BYTES * 100 * sizeof(char),
                                              portMAX_DELAY);
        if (xReceivedBytes > 0)
        {
            // write data to i2s bus
            esp_err_t err = i2s_write(I2S_COMM_MODE, (char *)ucRxData, 8000 * sizeof(char), &bytes_written, portMAX_DELAY);
            if (err != ESP_OK)
            {
                color_printf(COLOR_PRINT_RED, "\t\trec_task: i2s_write error");
            }
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

/**
 * Add doc stub
 */
void init_audio_capture_task(StreamBufferHandle_t xStreamBufferMic, StreamBufferHandle_t xStreamBufferDsp)
{
    mic_recording_stream_buf = NULL;
    dsp_stream_buf = NULL;

    xTaskCreate(audio_capture_task, "audio_capture_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBufferMic, 5, &audioCaptureTaskHandle);
}

void init_audio_playback_task(StreamBufferHandle_t network_stream_buf)
{
    mic_recording_stream_buf = NULL;
    dsp_stream_buf = NULL;
    xTaskCreate(audio_playback_task, "audio_playback_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, network_stream_buf, 5, &audioPlayBackTaskHandle);
}

void init_audio_capture_rec_task(StreamBufferHandle_t xStreamBufferMic, StreamBufferHandle_t xStreamBufferRecMic)
{
    mic_recording_stream_buf = xStreamBufferRecMic;
    dsp_stream_buf = NULL;

    xTaskCreate(audio_capture_task, "audio_capture_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBufferMic, 5, &audioCaptureTaskHandle);
}

void suspend_audio_capture() { vTaskSuspend(audioCaptureTaskHandle); }

void resume_audio_capture() { vTaskResume(audioCaptureTaskHandle); }

void suspend_audio_playback() { vTaskSuspend(audioPlayBackTaskHandle); }

void resume_audio_playback() { vTaskResume(audioPlayBackTaskHandle); }