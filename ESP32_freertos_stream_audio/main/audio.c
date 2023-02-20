#include "audio.h"

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

void tx_task(void *arg)
{
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

    color_printf(COLOR_PRINT_BLUE, "\t\ttx_task: starting transmission");

    while (1)
    {
        /* Read audio using i2s and scale it down to uint8_t*/
        i2s_read(I2S_CHANNEL_NUM, (char *)i2s_readraw_buff, sizeof(i2s_readraw_buff), &bytes_read, portMAX_DELAY);
        color_printf(COLOR_PRINT_BLUE, "\t\ttx_task: %d bytes read", bytes_read);
        i2s_adc_data_scale(ucArrayToSend, i2s_readraw_buff, sizeof(i2s_readraw_buff));

        /* Send an array to the stream buffer, blocking for a maximum of 100ms to
        wait for enough space to be available in the stream buffer. */
        xBytesSent = xStreamBufferSend(xStreamBuffer,
                                       (void *)ucArrayToSend,
                                       sizeof(ucArrayToSend),
                                       xBlockTime);

        if (xBytesSent != sizeof(ucArrayToSend))
        {
            /* The call to xStreamBufferSend() times out before there was enough
            space in the buffer for the data to be written, but it did
            successfully write xBytesSent bytes. */
            // color_printf(COLOR_PRINT_RED, "\t\ttx_task: notify %d bytes sent", xBytesSent);
        }
        else
        {
            /* The entire array was sent to the stream buffer. */
            // color_printf(COLOR_PRINT_BLUE, "\t\ttx_task: notify all %d bytes sent", xBytesSent);
        }
    }
}

void init_audio(StreamBufferHandle_t xStreamBuffer)
{

    xTaskCreate(tx_task, "tx_task", CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBuffer, 5, NULL);
}