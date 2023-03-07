#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "espnow_mic.h"

#if (!CONFIG_IDF_TARGET_ESP32)
#include "i2s_recv_std_config.h"
#endif

// number of frames to try and send at once (a frame is a left and right sample)
const int NUM_FRAMES_TO_SEND = 128;

static const char* TAG = "espnow_mic";
StreamBufferHandle_t network_stream_buf;

uint8_t* mic_read_buf;
uint8_t* spk_write_buf;

// i2s adc capture task
void i2s_adc_capture_task(void* task_param)
{
    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t) task_param;

    // allocate memory for the read buffer
    mic_read_buf = (uint8_t*) calloc(sizeof(char),READ_BUF_SIZE_BYTES);

    // enable i2s adc
    size_t bytes_read = 0; // to count the number of bytes read from the i2s adc
    TickType_t ticks_to_wait = 100; // wait 100 ticks for the mic_stream_buf to be available
    i2s_adc_enable(EXAMPLE_I2S_NUM);

    while(true){
        // read from i2s bus and use errno to check if i2s_read is successful
        if (i2s_read(EXAMPLE_I2S_NUM, (char*)mic_read_buf, READ_BUF_SIZE_BYTES, &bytes_read, ticks_to_wait) != ESP_OK) {
            ESP_LOGE(TAG, "Error reading from i2s adc: %d", errno);
            deinit_config();
            exit(errno);
        }
        // check if the number of bytes read is equal to the number of bytes to read
        if (bytes_read != READ_BUF_SIZE_BYTES) {
            ESP_LOGE(TAG, "Error reading from i2s adc: %d", errno);
            deinit_config();
            exit(errno);
        }
        // scale the data to 8 bit
        i2s_adc_data_scale(mic_read_buf, mic_read_buf, READ_BUF_SIZE_BYTES);
        /**
         * xstreambuffersend is a blocking function that sends data to the stream buffer,
         * esp_now_send needs to send 128 packets of 250 bytes each, so the stream buffer needs to be able to hold at least 2-3 times of 128 * 250 bytes = BYTE_RATE bytes
         * */ 
        size_t byte_sent = xStreamBufferSend(mic_stream_buf,(void*) mic_read_buf, READ_BUF_SIZE_BYTES, portMAX_DELAY);
        if (byte_sent != READ_BUF_SIZE_BYTES) {
            ESP_LOGE(TAG, "Error: only sent %d bytes to the stream buffer out of %d \n", byte_sent, READ_BUF_SIZE_BYTES);
        }
    }
    free(mic_read_buf);
    vTaskDelete(NULL);
    
}

/**
 * @brief Scale data to 8bit for data from ADC.
 *        DAC can only output 8 bit data.
 *        Scale each 16bit-wide ADC data to 8bit DAC data.
 */
void i2s_adc_data_scale(uint8_t * des_buff, uint8_t* src_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (src_buff[i + 1] & 0xf) << 8) | ((src_buff[i + 0]))));
        des_buff[j++] = 0;
        des_buff[j++] = dac_value * 256 / 4096;
    }
}

// i2s dac playback task
void i2s_dac_playback_task(void* task_param) {
    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t spk_read_buf = (StreamBufferHandle_t)task_param;

    size_t bytes_written = 0;
    spk_write_buf = (uint8_t*) calloc(sizeof(char),EXAMPLE_I2S_READ_LEN);
    assert(spk_write_buf != NULL);

    while (true) {
        // read from the stream buffer, use errno to check if xstreambufferreceive is successful
        size_t num_bytes = xStreamBufferReceive(spk_read_buf, (void*) spk_write_buf, EXAMPLE_I2S_READ_LEN, portMAX_DELAY);
        if (num_bytes > 0) {
            // send data to i2s dac
            esp_err_t err = i2s_write(EXAMPLE_I2S_NUM, spk_write_buf, num_bytes, &bytes_written, portMAX_DELAY);
            if (err != ESP_OK) {
                printf("Error writing I2S: %0x\n", err);
                deinit_config();
                exit(err);
            }
        }
        else if(num_bytes != EXAMPLE_I2S_READ_LEN) {
            printf("Error: partial reading from net stream: %d\n", errno);
            deinit_config();
            exit(errno);
        }
    }
    free(spk_write_buf);
    vTaskDelete(NULL);
}


/* call the init_auidio function for starting adc and filling the buf -second */
esp_err_t init_audio_trans(StreamBufferHandle_t mic_stream_buf){ 
    printf("initializing i2s mic\n");

    /* thread for adc and filling the buf for the transmitter */
    xTaskCreate(i2s_adc_capture_task, "i2s_adc_capture_task", 4096, (void*) mic_stream_buf, 4, NULL); 

    return ESP_OK;
}

/* call the init_auidio function for starting adc and filling the buf -second */
esp_err_t init_audio_recv(StreamBufferHandle_t network_stream_buf){ 
    printf("initializing i2s spk\n");
    network_stream_buf = this.network_stream_buf;
    // /* thread for filling the buf for the reciever and dac */
#ifdef CONFIG_IDF_TARGET_ESP32
    xTaskCreate(i2s_dac_playback_task, "i2s_dac_playback_task", 4096, (void*) network_stream_buf, 4, NULL);
#else
    xTaskCreate(i2s_std_playback_task, "i2s_std_playback_task", 4096,(void*) network_stream_buf, 4, NULL);
#endif
    return ESP_OK;
}

