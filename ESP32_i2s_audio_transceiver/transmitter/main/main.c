/* Mirf Example
     This example code is in the Public Domain (or CC0 licensed, at your option.)
     Unless required by applicable law or agreed to in writing, this
     software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
     CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "mirf.h"
#define V_REF 1100
#define I2S_COMM_MODE 0 // ADC/DAC Mode
#define I2S_SAMPLE_RATE 44100
#define I2S_SAMPLE_BITS (16)
#define I2S_BUF_DEBUG (0)        // enable display buffer for debug
#define I2S_READ_LEN (16 * 1024) // I2S read buffer length
#define I2S_FORMAT (I2S_CHANNEL_FMT_ONLY_RIGHT)
#define I2S_CHANNEL_NUM (0)            // I2S channel number
#define I2S_ADC_UNIT ADC_UNIT_1        // I2S built-in ADC unit
#define I2S_ADC_CHANNEL ADC1_CHANNEL_0 // I2S built-in ADC channel GPIO36
#define BIT_SAMPLE (16)
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define NUM_CHANNELS (1) // For mono recording only!
#define BYTE_RATE (I2S_SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS

#define SPI_MOSI_GPIO 23
#define SPI_MISO_GPIO 19
#define SPI_CLK_GPIO 18
#define SPI_CS_GPIO 5

static const char *TAG = "I2S_ADC_REC";

#define BIT_SAMPLE 16

#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define NUM_CHANNELS 1 // For mono recording only!
#define SD_MOUNT_POINT "/sdcard"
#define SAMPLE_SIZE (BIT_SAMPLE * 1024)
#define BYTE_RATE (I2S_SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.
sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t *card;

size_t bytes_read;
const int WAVE_HEADER_SIZE = 44;

/**
 * @brief I2S ADC mode init.
 */
void init_microphone(void)
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
/**
 * @brief Scale data to 8bit for data from ADC.
 *        Data from ADC are 12bit width by default.
 * @param d_buff: destination buffer
 * @param s_buff: source buffer
 * @param len: length of source buffer
 */
void i2s_adc_data_scale(uint8_t *d_buff, uint8_t *s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i += 2)
    {
        dac_value = ((((uint16_t)(s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
#else
    for (int i = 0; i < len; i += 4)
    {
        dac_value = ((((uint16_t)(s_buff[i + 3] & 0xf) << 8) | ((s_buff[i + 2]))));
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
#endif
}

/**
 * @brief Initializes the slot without card detect (CD) and write protect (WP) signals.
 * It formats the card if mount fails and initializes the card. After the card has been
 * initialized, it print the card properties
 */
void mount_sdcard(void)
{
    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 8 * 1024};
    ESP_LOGI(TAG, "Initializing SD card");

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_MOSI_GPIO,
        .miso_io_num = SPI_MISO_GPIO,
        .sclk_io_num = SPI_CLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SPI_CS_GPIO;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return;
    }

    sdmmc_card_print_info(stdout, card); // Card has been initialized, print its properties
}

/**
 * @brief Generates the header for the WAV file that is going to be stored in the SD card.
 * See this for reference: http://soundfile.sapp.org/doc/WaveFormat/.
 */
void generate_wav_header(char *wav_header, uint32_t wav_size, uint32_t sample_rate)
{
    uint32_t file_size = wav_size + WAVE_HEADER_SIZE - 8;
    uint32_t byte_rate = BYTE_RATE;

    const char set_wav_header[] = {
        'R', 'I', 'F', 'F',                                                  // ChunkID
        file_size, file_size >> 8, file_size >> 16, file_size >> 24,         // ChunkSize
        'W', 'A', 'V', 'E',                                                  // Format
        'f', 'm', 't', ' ',                                                  // Subchunk1ID
        0x10, 0x00, 0x00, 0x00,                                              // Subchunk1Size (16 for PCM)
        0x01, 0x00,                                                          // AudioFormat (1 for PCM)
        0x01, 0x00,                                                          // NumChannels (1 channel)
        sample_rate, sample_rate >> 8, sample_rate >> 16, sample_rate >> 24, // SampleRate
        byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24,         // ByteRate
        0x02, 0x00,                                                          // BlockAlign
        0x10, 0x00,                                                          // BitsPerSample (16 bits)
        'd', 'a', 't', 'a',                                                  // Subchunk2ID
        wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24,             // Subchunk2Size
    };

    memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}

/**
 * @brief Writes the WAV header to the SD card.
 */
void record_wav(uint32_t rec_time)
{
}

/**
 * @brief I2S ADC mode read data from ADC.
 * @param i2s_num: I2S port number
 * @param i2s_read_buff: I2S read buffer
 * @param read_len: I2S read buffer length
 */
#if CONFIG_TRANSMITTER
void transmitter(void *pvParameters)
{
    size_t bytes_read;

    ESP_LOGI(pcTaskGetName(0), "Start");

    //----------------------------Allocate space for the buffers used in this test----------------------------------- //
    int i2s_read_len = (0.01 * BYTE_RATE);
    int i2s_write_len = (32);
    char *i2s_read_buff = (char *)calloc(i2s_read_len, sizeof(char));
    uint8_t *i2s_write_buff = (uint8_t *)calloc(i2s_write_len, sizeof(char));
    //----------------------------Allocate space for the buffers used in this test---------------------------------- //

    //----------------------------------------------Start of nrf24 configs------------------------------------------ //
    NRF24_t dev;
    NRF24_t *nrf24 = &dev;
    Nrf24_init(&dev);
    uint8_t payload = sizeof(i2s_write_buff);
    uint8_t channel = 90;
    Nrf24_config(&dev, channel, payload);

    // Set the receiver address using 5 characters
    esp_err_t ret = Nrf24_setTADDR(&dev, (uint8_t *)"FGHIJ");
    if (ret != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(0), "nrf24l01 not installed");
        // end of the task
        vTaskDelete(NULL);
    }

#if CONFIG_ADVANCED
    AdvancedSettings(&dev);
#endif // CONFIG_ADVANCED

    // Print settings
    Nrf24_printDetails(&dev);
    //----------------------------------------------End of nrf24 configs----------------------------------------------//

    i2s_adc_enable(I2S_CHANNEL_NUM);

    /*------------------------------------Recording + transmission section---------------------------------------------*/
    int rec_time = 1;

    // I2S ADC mode microphone init.
    init_microphone();

    ESP_LOGI(TAG, "Starting recording for %d seconds!", rec_time);

    /*--------------------- Replace with recording code from record_wav(uint32_t rec_time) --------------------------*/

    // Use POSIX and C standard library functions to work with files.
    int flash_wr_size = 0;
    ESP_LOGI(TAG, "Opening file");

    char wav_header_fmt[WAVE_HEADER_SIZE];

    uint32_t flash_rec_size = BYTE_RATE * rec_time;
    generate_wav_header(wav_header_fmt, flash_rec_size, I2S_SAMPLE_RATE);

    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(SD_MOUNT_POINT "/record.wav", &st) == 0)
    {
        // Delete it if it exists
        unlink(SD_MOUNT_POINT "/record.wav");
    }

    // Create new WAV file
    FILE *f = fopen(SD_MOUNT_POINT "/record.wav", "a");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    // Write the header to the WAV file
    fwrite(wav_header_fmt, 1, WAVE_HEADER_SIZE, f);

    i2s_adc_enable(I2S_CHANNEL_NUM);

    // Send the start of transmission flag via nrf24
    Nrf24_send(&dev, (uint8_t *)"START");

    // Start recording
    while (flash_wr_size < flash_rec_size)
    {
        //-----------------------------------Capture audio into large buffer-------------------------------- //

        // record for 100ms
        for (int wr_count = 0; wr_count < 0.01 * BYTE_RATE; wr_count += bytes_read)
        {
            // Read the RAW samples from the microphone
            // Read data from I2S bus, in this case, from ADC. //
            i2s_read(I2S_CHANNEL_NUM, (char *)i2s_read_buff, i2s_read_len, &bytes_read, 100);

            // Write the samples to the WAV file - is2_readraw_buff was replaced by i2s_write_buff
            fwrite(i2s_read_buff, 1, bytes_read, f);
        }

        // i2s_write_buff needs to be the buffer that is sent via nr24l01.

        //-----------------------------------Capture audio into large buffer------------------------------- //

        //-----------------------------------Replaced with smaller buffer--------------------------------- //

        // split i2s_readraw_buff into i2s_write_buff and send via nrf24
        for (int i = 0; i < i2s_read_len; i = i + i2s_write_len)
        {
            for (int j = 0; j < i2s_write_len; j++)
            {
                i2s_write_buff[j] = i2s_read_buff[j + i] * 10;
            }

            //-------------------------------------Sending via nrf24---------------------------------------- //

            Nrf24_send(&dev, i2s_write_buff);

            if (Nrf24_isSend(&dev, 1000))
            {
                // ESP_LOGI(pcTaskGetName(0), "sending audio data ...");
                // Nrf24_print_status(Nrf24_getStatus(nrf24));

                // Log how much of the full recording has been written to the file
                // if (flash_wr_size * 100) / flash_rec_size) is a multiple of 10
                if (((flash_wr_size * 100) / flash_rec_size) % 10 == 0)
                {
                    ESP_LOGI(TAG, "Written to file %d%%", (flash_wr_size * 100) / flash_rec_size);
                }
            }
            else
            {
                ESP_LOGI(pcTaskGetName(0), "sending audio failed ...");
            }
            //-------------------------------------Sending via nrf24---------------------------------------- //
        }

        //-----------------------------------Replaced with smaller buffer-------------------------------- //
    }

    ESP_LOGI(TAG, "Recording done!");
    fclose(f);
    ESP_LOGI(TAG, "File written on SDCard");

    // All done, unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card unmounted");
    // Deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);

    /*--------------------- Replace with recording code from record_wav(uint32_t rec_time) -------------------------- */

    // Send the start of transmission flag via nrf24
    // Nrf24_send(&dev, (uint8_t *)"END");

    free(i2s_read_buff);
    free(i2s_write_buff);

    // Stop I2S driver and destroy
    ESP_ERROR_CHECK(i2s_driver_uninstall(I2S_COMM_MODE));
    // end of the task
    vTaskDelete(NULL);
}
#endif // CONFIG_TRANSMITTER

void app_main(void)
{
    ESP_LOGI(TAG, "Analog microphone recording+transmission Example start");
    // Mount the SDCard for recording the audio file
    mount_sdcard();

#if CONFIG_TRANSMITTER
    xTaskCreate(transmitter, "TRANSMITTER", 1024 * 3, NULL, 2, NULL);
#endif
}
