#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "espnow_mic.h"

static const char* TAG = "espnow_mic";

uint8_t* mic_read_buf;
uint8_t* audio_output_buf;
uint8_t* audio_input_buf;


// i2s adc capture task
void i2s_adc_capture_task(void* task_param)
{
    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t) task_param;

    // enable i2s adc
    i2s_adc_enable(EXAMPLE_I2S_NUM);
    size_t bytes_read = 0; // to count the number of bytes read from the i2s adc
    TickType_t ticks_to_wait = 100; // wait 100 ticks for the mic_stream_buf to be available

    // allocate memory for the read buffer
    // size_t buf_size = BYTE_RATE * 4;
    mic_read_buf = (uint8_t*) calloc(BYTE_RATE, sizeof(char)); //allocate memory with a number of bytes equal to the byte rate
    audio_output_buf = (uint8_t*) calloc(BYTE_RATE, sizeof(char));

    while(true){
        // read from i2s bus and use errno to check if i2s_read is successful
        if (i2s_read(EXAMPLE_I2S_NUM, mic_read_buf, BYTE_RATE * sizeof(char), &bytes_read, ticks_to_wait) != ESP_OK) {
            ESP_LOGE(TAG, "Error reading from i2s adc: %d", errno);
            exit(errno);
        }else{
            // ESP_LOGI(TAG, "Read %d bytes from i2s adc", bytes_read);
        }
        // process data and scale to 8bit for I2S DAC. used prior sending will delay the process. better used on the reciever side
        // i2s_adc_data_scale(audio_output_buf, mic_read_buf, BYTE_RATE * sizeof(char));

        // xstreambuffersend is a blocking function that sends data to the stream buffer, use errno to check if xstreambuffersend is successful
        // esp_now_send is a non-blocking function that sends data to the espnow network with speed of 1Mbps, meaing 125KB/s
        // mic_stream_buf needs to provide data at a rate of 125KB/s to esp_now_send
        if (xStreamBufferSend(mic_stream_buf, mic_read_buf, BYTE_RATE * sizeof(char), portMAX_DELAY) != BYTE_RATE) {
            ESP_LOGE(TAG, "Error sending to mic_stream_buf: %d", errno);
            exit(errno);
        }else{
            // ESP_LOGI(TAG, "Sent %d bytes to mic_stream_buf", bytes_read);
        }
    }
}

/**
 * @brief Scale data to 8bit for data from ADC.
 *        Data from ADC are 12bit width by default.
 *        DAC can only output 8 bit data.
 *        Scale each 12bit ADC data to 8bit DAC data.
 */
void i2s_adc_data_scale(uint8_t * des_buff, uint8_t* src_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (src_buff[i + 1] & 0xf) << 8) | ((src_buff[i + 0]))));
        des_buff[j++] = 0;
        des_buff[j++] = dac_value * 256 / 4096;
    }
#else
    for (int i = 0; i < len; i += 4) {
        dac_value = ((((uint16_t)(src_buff[i + 3] & 0xf) << 8) | ((src_buff[i + 2]))));
        des_buff[j++] = 0;
        des_buff[j++] = 0;
        des_buff[j++] = 0;
        des_buff[j++] = dac_value * 256 / 4096;
    }
#endif
}

// i2s dac playback task
void i2s_dac_playback_task(void* task_param) {
    // get the stream buffer handle from the task parameter
    StreamBufferHandle_t net_stream_buf = (StreamBufferHandle_t)task_param;

    size_t bytes_written = 0;

    // allocate memory for the read buffer
    audio_input_buf = (uint8_t*)calloc(BYTE_RATE, sizeof(char));
    audio_output_buf = (uint8_t*) calloc(BYTE_RATE, sizeof(char));

    while (true) {
        // read from the stream buffer, use errno to check if xstreambufferreceive is successful
        size_t num_bytes = xStreamBufferReceive(net_stream_buf, audio_input_buf, BYTE_RATE * sizeof(char), portMAX_DELAY);
        if (num_bytes > 0) {

            // assert num_bytes is equal to the byte rate, if false, exit the program
            assert(num_bytes == BYTE_RATE * sizeof(char));

            // process data and scale to 8bit for I2S DAC. used prior sending will delay the process. better used on the reciever side
            i2s_adc_data_scale(audio_output_buf, audio_input_buf, BYTE_RATE * sizeof(char));

            // send data to i2s dac
            esp_err_t err = i2s_write(EXAMPLE_I2S_NUM, audio_output_buf, num_bytes, &bytes_written, portMAX_DELAY);
            if (err != ESP_OK) {
                printf("Error writing I2S: %0x\n", err);
                //exit the program
                exit(err);
            }
        }
        else if (num_bytes == 0) {
            printf("Error reading from net stream buffer: %d\n", errno);
            ESP_LOGE(TAG, "No data in m");
        }
        else {
            printf("Other error reading from net stream: %d\n", errno);
            // exit with error code and error message
            exit(errno);
        }
    }
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
    // /* thread for filling the buf for the reciever and dac */
    xTaskCreate(i2s_dac_playback_task, "i2s_dac_playback_task", 4096, (void*) network_stream_buf, 4, NULL);

    return ESP_OK;
}  